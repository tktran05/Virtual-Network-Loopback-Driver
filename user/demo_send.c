#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h" //file nay chua san strlen, strcpy
#include "kernel/fcntl.h"


int main(int argc, char *argv[]) {
    int fd;
    char buf[128];
    
    if(argc < 2){
        printf("Usage: demo_send <message>\n");
        exit(1);
    }
    
    //1. Noi cac tham so lai thanh 1 chuoi
    buf[0] = 0;
    for(int i = 1; i < argc; i++){
        //Neu khong phai tu dau tien thi them dau cach
        if(i>1){
            int len = strlen(buf);
            if(len < sizeof(buf) - 2){
                buf[len] = ' ';
                buf[len+1] = 0;
            }
        }
        
        //Noi tu hien tai vao buffer(kiem tra tran bo nho)
        int current_len = strlen(buf);
        int word_len = strlen(argv[i]);
        if(current_len + word_len < sizeof(buf) - 1){
            strcpy(buf + current_len, argv[i]);
        }
    }
        
    fd = open("net", O_RDWR);
    if(fd < 0){
        printf("Sender: Cannot open net device\n");
        exit(1);
    }
    
    //Gui chuoi da noi
    printf("Sender: Dang gui '%s'...\n", buf);
    int n = write(fd, buf, strlen(buf));
    
    if(n > 0){
        printf("Sender: Gui thanh cong!\n");
        
    } else {
        printf("Sender: Gui that bai!\n");
    }

    
    close(fd);
    exit(0);
}
