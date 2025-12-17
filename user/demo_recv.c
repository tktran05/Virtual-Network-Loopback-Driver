// user/demo_recv.c

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h" // Dùng để định nghĩa DIRSIZ nếu cần

// Định nghĩa các hằng số dựa trên kernel/net.h
#define PACKET_MAX 128
#define HEADER_SIZE sizeof(int)
#define MAX_RECV_BUF (PACKET_MAX + HEADER_SIZE + 1) // 133 bytes

int main() {
    int fd;
    char buf[MAX_RECV_BUF]; // Sửa: Đảm bảo buffer đủ lớn (133 bytes)

    // Mở thiết bị net
    // Sửa: Dùng đường dẫn tuyệt đối "/net" (dựa trên mkfs.c của bạn)
    fd = open("net", O_RDWR); 
    if(fd < 0){
        // SỬA LỖI BIÊN DỊCH: Bỏ số 2
        printf("Receiver: Cannot open net device /net\n"); 
        exit(1);
    }

    printf("Receiver: Dang cho tin nhan...\n");

    while(1) {
        // Yêu cầu đọc tối đa 132 bytes (+1 byte cho null terminator)
        int total_len = read(fd, buf, MAX_RECV_BUF - 1);
        
        if(total_len > 0){
            // Đảm bảo dữ liệu nhận được chứa ít nhất PID header
            if (total_len < HEADER_SIZE) {
                // SỬA LỖI BIÊN DỊCH: Bỏ số 2
                printf("Receiver: Loi goi tin, kich thuoc nho hon header.\n");
                continue;
            }

            // Giai ma header (Lấy 4 byte đầu tiên)
            int sender_pid;
            // Sử dụng memmove để an toàn hơn
            memmove(&sender_pid, buf, HEADER_SIZE); 
            
            // Lấy tin nhắn (Payload) từ byte thứ 4 (offset 4) trở đi
            char *msg = buf + HEADER_SIZE;
            
            // Tính độ dài tin nhắn thực tế
            int data_len = total_len - HEADER_SIZE;
            
            // Đảm bảo kết thúc chuỗi
            msg[data_len] = 0;
            
            printf("Receiver: Da nhan duoc tu [PID %d]: '%s' (len=%d)\n", sender_pid, msg, data_len);
        } else if (total_len < 0) {
            // SỬA LỖI BIÊN DỊCH: Bỏ số 2
            printf("Receiver: Loi doc tin nhan. Exit.\n");
            break;
        }
    }

    close(fd);
    exit(0);
}