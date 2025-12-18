#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// Cấu trúc gói tin gửi đi: [PID đích (4 byte)] + [Dữ liệu]
#define HEADER_SIZE sizeof(int)
#define MAX_PAYLOAD 128
#define TOTAL_BUF (HEADER_SIZE + MAX_PAYLOAD)

int main(int argc, char *argv[]) {
    int fd;
    int target_pid;
    char send_buf[TOTAL_BUF];
    char msg_buf[MAX_PAYLOAD];
    
    // Yêu cầu: demo_send <pid_nguoi_nhan> <tin_nhan>
    if(argc < 3){
        printf("Usage: demo_send <target_pid> <message>\n");
        exit(1);
    }
    
    // 1. Lấy PID đích từ đối số đầu tiên
    target_pid = atoi(argv[1]);
    if(target_pid <= 0){
        printf("Sender: PID khong hop le!\n");
        exit(1);
    }

    // 2. Nối các tham số còn lại thành nội dung tin nhắn
    msg_buf[0] = 0;
    for(int i = 2; i < argc; i++){
        if(i > 2){
            int len = strlen(msg_buf);
            if(len < MAX_PAYLOAD - 2){
                msg_buf[len] = ' ';
                msg_buf[len+1] = 0;
            }
        }
        
        int current_len = strlen(msg_buf);
        int word_len = strlen(argv[i]);
        if(current_len + word_len < MAX_PAYLOAD - 1){
            strcpy(msg_buf + current_len, argv[i]);
        }
    }
        
    // 3. Đóng gói dữ liệu: [PID đích] + [Tin nhắn]
    // Copy 4 byte PID vào đầu send_buf
    memmove(send_buf, &target_pid, HEADER_SIZE);
    // Copy tin nhắn vào ngay sau đó
    int msg_len = strlen(msg_buf);
    memmove(send_buf + HEADER_SIZE, msg_buf, msg_len);

    // 4. Mở thiết bị và gửi
    fd = open("net", O_RDWR);
    if(fd < 0){
        printf("[Sender]: Cannot open net device\n");
        exit(1);
    }
    
    printf("[Sender]: sending to PID %d \n", target_pid);
    
    // Gửi tổng cộng: 4 byte PID + độ dài tin nhắn
    int n = write(fd, send_buf, HEADER_SIZE + msg_len);
    
    if(n > 0){
        printf("[Sender]: Successfully!\n");
    } else {
        printf("[Sender]: Failed\n");
    }

    close(fd);
    exit(0);
}