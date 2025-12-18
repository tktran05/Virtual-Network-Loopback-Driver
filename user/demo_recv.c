// user/demo_recv.c

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define PACKET_MAX 128
#define HEADER_SIZE sizeof(int)
#define MAX_RECV_BUF (PACKET_MAX + HEADER_SIZE + 1)

int main() {
    int fd;
    char buf[MAX_RECV_BUF];

    // BƯỚC QUAN TRỌNG NHẤT: In PID để người gửi biết đường mà nhập
    int my_pid = getpid();
    printf("Receiver: Toi dang chay voi [PID %d]\n", my_pid);
    printf("Receiver: Hay dung lenh: 'demo_send %d [tin_nhan]'\n", my_pid);

    // Mở thiết bị net
    fd = open("net", O_RDWR); 
    if(fd < 0){
        printf("Receiver: Khong the mo thiet bi /net\n"); 
        exit(1);
    }

    printf("Receiver: Dang cho tin nhan tu mang ao...\n");

    while(1) {
        // Đọc dữ liệu từ driver (Lúc này Kernel sẽ tự đăng ký PID cho chúng ta)
        int total_len = read(fd, buf, MAX_RECV_BUF - 1);
        
        if(total_len >= HEADER_SIZE){
            // Giải mã: Ai đã gửi tin cho mình?
            int sender_pid;
            memmove(&sender_pid, buf, HEADER_SIZE); 
            
            // Lấy nội dung tin nhắn
            char *msg = buf + HEADER_SIZE;
            int data_len = total_len - HEADER_SIZE;
            
            // Kết thúc chuỗi để in ra màn hình
            msg[data_len] = '\0';
            
            printf("\n[TIN NHAN MOI]\n");
            printf("Tu PID: %d\n", sender_pid);
            printf("Noi dung: %s\n", msg);
            printf("Tong bytes: %d\n", total_len);
            printf("--------------------------\n");
        } else if (total_len < 0) {
            printf("Receiver: Loi doc tin nhan. Exit.\n");
            break;
        }
    }

    close(fd);
    exit(0);
}