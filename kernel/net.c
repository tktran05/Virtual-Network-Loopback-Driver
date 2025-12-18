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

struct net_device net_dev; 

void netinit(void) {
  initlock(&net_dev.lock, "net_dev"); 
  
  acquire(&net_dev.lock);
  for(int i = 0; i < MAX_SLOTS; i++) {
    net_dev.slots[i].receiver_pid = 0; // Đánh dấu slot trống
    net_dev.slots[i].head = 0;
    net_dev.slots[i].tail = 0;
    net_dev.slots[i].count = 0;
  }
  release(&net_dev.lock);

  devsw[NET].read = netread;
  devsw[NET].write = netwrite;
}

// netwrite đóng vai trò là Bộ định tuyến (Router)
int netwrite(int user_src, uint64 src, int n) {
  int target_pid;
  struct proc *pr = myproc();
  
  // Phải có ít nhất 4 byte chứa PID người nhận
  if (n < sizeof(int)) return -1;

  // Bước 1: Lấy địa chỉ PID đích từ user
  if(copyin(pr->pagetable, (char*)&target_pid, src, sizeof(int)) < 0)
    return -1;

  acquire(&net_dev.lock);
  
  // Bước 2: ROUTING - Tìm slot của người nhận
  struct net_slot *dest_slot = 0;
  for(int i = 0; i < MAX_SLOTS; i++){
    if(net_dev.slots[i].receiver_pid == target_pid){
      dest_slot = &net_dev.slots[i];
      break;
    }
  }

  // Nếu không thấy người nhận đang chờ (không có slot nào đăng ký PID này)
  if(dest_slot == 0){
    release(&net_dev.lock);
    return -1; 
  }

  // Bước 3: Đợi nếu hộp thư của người nhận này bị đầy
  while(dest_slot->count == SLOT_BUF_MAX){
    if(killed(pr)){
      release(&net_dev.lock);
      return -1;
    }
    sleep(&dest_slot->count, &net_dev.lock);
  }

  // Bước 4: Ghi gói tin vào slot của người nhận
  struct packet *pkt = &dest_slot->buf[dest_slot->tail];
  int data_len = n - sizeof(int);
  if(data_len > PACKET_MAX) data_len = PACKET_MAX;

  if(copyin(pr->pagetable, pkt->data, src + sizeof(int), data_len) < 0){
    release(&net_dev.lock);
    return -1;
  }

  pkt->len = data_len;
  pkt->sender_pid = pr->pid; // Lưu lại ai là người gửi

  dest_slot->tail = (dest_slot->tail + 1) % SLOT_BUF_MAX;
  dest_slot->count++;

  // Bước 5: Đánh thức ĐÚNG người nhận đang đợi ở slot này
  wakeup(&dest_slot->count);

  release(&net_dev.lock);
  return n;
}

// netread đóng vai trò Đăng ký (Receiver Registration)
int netread(int user_dst, uint64 dst, int n) {
  struct proc *pr = myproc();
  struct net_slot *my_slot = 0;

  acquire(&net_dev.lock);

  // Bước 1: Tìm slot của mình hoặc đăng ký mới nếu chưa có
  for(int i = 0; i < MAX_SLOTS; i++){
    if(net_dev.slots[i].receiver_pid == pr->pid){
      my_slot = &net_dev.slots[i];
      break;
    }
  }

  // Nếu tiến trình này lần đầu đọc, cấp cho nó một slot trống
  if(my_slot == 0){
    for(int i = 0; i < MAX_SLOTS; i++){
      if(net_dev.slots[i].receiver_pid == 0){
        my_slot = &net_dev.slots[i];
        my_slot->receiver_pid = pr->pid;
        my_slot->count = 0;
        my_slot->head = 0;
        my_slot->tail = 0;
        break;
      }
    }
  }

  if(my_slot == 0){
    release(&net_dev.lock);
    return -1; // Hệ thống hết slot mạng
  }

  // Bước 2: Đợi cho đến khi có ai đó gửi thư vào slot của mình
  while (my_slot->count == 0) {
    if(killed(pr)){
      release(&net_dev.lock);
      return -1;
    }
    sleep(&my_slot->count, &net_dev.lock); 
  }

  // Bước 3: Lấy gói tin ra (First-In First-Out trong slot)
  struct packet *pkt = &my_slot->buf[my_slot->head];
  
  if (n < sizeof(int)) { release(&net_dev.lock); return -1; }

  // Trả về: [4 byte PID người gửi] + [Dữ liệu]
  copyout(pr->pagetable, dst, (char*)&pkt->sender_pid, sizeof(int));
  int len_to_copy = (pkt->len > n - sizeof(int)) ? n - sizeof(int) : pkt->len;
  copyout(pr->pagetable, dst + sizeof(int), pkt->data, len_to_copy);

  my_slot->head = (my_slot->head + 1) % SLOT_BUF_MAX;
  my_slot->count--;

  // Đánh thức người gửi đang đợi slot này trống bớt
  wakeup(&my_slot->count);

  release(&net_dev.lock);
  return sizeof(int) + len_to_copy;
}