// kernel/net.c
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "net.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"

// Biến toàn cục cho thiết bị mạng loopback
struct netbuffer net_dev; 

// Khởi tạo thiết bị
void netinit(void) {
  // BƯỚC 1: Khởi tạo Cấu trúc Dữ liệu và Lock
  initlock(&net_dev.lock, "net_dev"); 
  net_dev.head = 0;
  net_dev.tail = 0;
  net_dev.count = 0;

  // BƯỚC 2: Đăng ký Driver vào Hệ thống
  devsw[NET].read = netread;
  devsw[NET].write = netwrite;
}

// Hàm ghi (netwrite): User gửi data -> Kernel buffer
// Sẽ BLOCK (sleep) nếu buffer đầy
int netwrite(int user_src, uint64 src, int n) {
  int i = 0;
  struct proc *pr = myproc();
  
  // Tối đa n byte có thể được ghi
  if (n <= 0) return 0;

  acquire(&net_dev.lock);
  
  // Vòng lặp để ghi từng phần hoặc chờ
  while(i < n){
    // Kiểm tra nếu tiến trình bị kill
    if(killed(pr)){
      release(&net_dev.lock);
      return -1;
    }
    
    // Nếu buffer đầy, tiến trình ghi phải sleep
    if (net_dev.count == NETBUF_MAX) {
      // Dùng địa chỉ của net_dev.count làm địa chỉ chờ (chan)
      sleep(&net_dev.count, &net_dev.lock); 
    } else {
      // Buffer có chỗ trống: Ghi gói tin
      struct packet *pkt = &net_dev.buf[net_dev.tail];
      int bytes_to_copy = n - i;

      // Giới hạn kích thước gói tin theo PACKET_MAX
      if (bytes_to_copy > PACKET_MAX) {
        bytes_to_copy = PACKET_MAX;
      }
      
      // Copy data từ user space vào kernel (copyin)
      if (copyin(pr->pagetable, pkt->data, src + i, bytes_to_copy) < 0) {
        // Lỗi copyin: break vòng lặp
        if (i == 0) i = -1; // Nếu lỗi ngay từ byte đầu tiên, trả về -1
        break;
      }
      
      // Ghi PID người gửi và độ dài
      pkt->sender_pid = pr->pid;
      pkt->len = bytes_to_copy;
      // pkt->timestamp = ... (Lấy thời gian nếu cần)

      // Cập nhật ring buffer
      net_dev.tail = (net_dev.tail + 1) % NETBUF_MAX;
      net_dev.count++;
      i += bytes_to_copy;
      
      // Đánh thức bất kỳ tiến trình nào đang chờ đọc (netread)
      wakeup(&net_dev.count);
    }
  }

  release(&net_dev.lock);
  
  // Trả về số byte đã ghi thành công
  return i; 
}


// Hàm đọc (netread): User nhận data <- Kernel buffer
// Sẽ BLOCK (sleep) nếu buffer rỗng
int netread(int user_dst, uint64 dst, int n) {
  struct proc *pr = myproc();
  struct packet *pkt;
  int bytes_read = 0;

  acquire(&net_dev.lock);

  // Nếu buffer rỗng, tiến trình đọc phải sleep
  while (net_dev.count == 0) {
    // Nếu tiến trình bị kill, thoát
    if(killed(pr)){
      release(&net_dev.lock);
      return -1;
    }
    // Dùng địa chỉ của net_dev.count làm địa chỉ chờ (chan)
    sleep(&net_dev.count, &net_dev.lock); 
  }

  // Buffer đã có gói tin: Lấy vị trí đọc hiện tại (head)
  pkt = &net_dev.buf[net_dev.head];

  // Tính số byte thực tế để sao chép
  // Chúng ta cần tối thiểu 4 bytes cho PID
  int len = pkt->len; 
  if (n < sizeof(int)) {
      // User yêu cầu đọc ít hơn cả header PID, lỗi
      release(&net_dev.lock);
      return -1; 
  }
  
  // Số byte data tối đa có thể đọc
  int max_data_len = n - sizeof(int);
  if (len > max_data_len) {
    len = max_data_len; // Giới hạn data theo yêu cầu của user
  }

  // BƯỚC 1: Copyout PID (Header)
  if (copyout(pr->pagetable, dst, (char*)&pkt->sender_pid, sizeof(int)) < 0) {
    release(&net_dev.lock);
    return -1;
  }
  
  // BƯỚC 2: Copyout Data (Payload)
  if (copyout(pr->pagetable, dst + sizeof(int), pkt->data, len) < 0){
    release(&net_dev.lock);
    return -1;
  }
  
  // Cập nhật ring buffer
  net_dev.head = (net_dev.head + 1) % NETBUF_MAX;
  net_dev.count--;
  
  // Đánh thức bất kỳ tiến trình nào đang chờ ghi (netwrite)
  // để cho phép họ ghi gói tin tiếp theo
  wakeup(&net_dev.count);
  
  bytes_read = sizeof(int) + len;

  release(&net_dev.lock);
  return bytes_read; // Trả về tổng số byte đã đọc (PID header + Data)
}