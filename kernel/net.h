// kernel/net.h

#define PACKET_MAX 128   // Kích thước tối đa của một gói tin (Payload)
#define SLOT_BUF_MAX 5    // Mỗi tiến trình có thể chứa tối đa 5 gói tin trong hộp thư riêng
#define MAX_SLOTS 10      // Tối đa 10 tiến trình (Endpoints) tham gia mạng cùng lúc

// Cấu trúc gói tin (giống như một phong bì thư)
struct packet {
  char data[PACKET_MAX]; // Nội dung tin nhắn
  int len;               // Độ dài thực tế của nội dung
  int sender_pid;  
  int receiver_pid;      // PID của người gửi (Địa chỉ nguồn)
  uint timestamp;        // Thời điểm gói tin được tạo (tick của hệ thống)
};

// Cấu trúc một Hộp thư (Slot/Endpoint)
// Đại diện cho một cổng nhận dữ liệu của một tiến trình cụ thể
struct net_slot {
  int receiver_pid;       // ĐỊA CHỈ: PID của người sở hữu slot này (0 nếu slot đang trống)
  struct packet buf[SLOT_BUF_MAX]; // Hàng đợi gói tin dành riêng cho PID này
  int head;               // Vị trí gói tin cũ nhất (để đọc)
  int tail;               // Vị trí gói tin mới nhất (để ghi)
  int count;              // Số lượng gói tin hiện có trong slot này
};

// Cấu trúc thiết bị mạng tổng thể (Network Device)
// Quản lý toàn bộ hệ thống các hộp thư và đồng bộ hóa
struct net_device {
  struct net_slot slots[MAX_SLOTS]; // Mảng chứa 10 hộp thư (Slot-based Architecture)
  struct spinlock lock;             // Khóa toàn cục bảo vệ tính toàn vẹn của toàn mạng
};

