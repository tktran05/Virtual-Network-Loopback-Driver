// kernel/net.h

// Định nghĩa kích thước
#define PACKET_MAX 128  // Kích thước tối đa của payload
#define NETBUF_MAX 10   // Số lượng gói tin tối đa trong hàng đợi vòng

// Cấu trúc gói tin (struct packet)
struct packet {
  char data[PACKET_MAX]; // Payload
  int len;               // Độ dài thực tế của dữ liệu
  int sender_pid;        // them truong pid
  uint timestamp;        // Ghi lại thời gian (như báo cáo yêu cầu)
};

// Cấu trúc bộ đệm vòng (struct netbuffer)
struct netbuffer {
  struct packet buf[NETBUF_MAX]; // Mảng chứa các gói tin
  int head;  // Vị trí đọc
  int tail;  // Vị trí ghi
  int count; // Số lượng gói tin hiện có
  struct spinlock lock; // Khóa để đảm bảo an toàn khi truy cập đồng thời
};
