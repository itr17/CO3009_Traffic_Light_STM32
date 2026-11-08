# HỆ THỐNG ĐÈN GIAO THÔNG 2 TUYẾN ĐƯỜNG TRÊN MẠCH XNUCLEO-F103RB

> **Môn học:** Vi xử lý – Vi điều khiển (CO3009)  
> **Giảng viên hướng dẫn:** TS. Lê Trọng Nhân  
> **Trường Đại học Bách Khoa – ĐHQG TP. Hồ Chí Minh**  
> **Khoa Khoa học và Kỹ thuật Máy tính**

---

## 👥 Thành viên nhóm
| STT | Họ và tên | MSSV | Nhiệm vụ chính |
|:---:|:---|:---:|:---|
| 1 | [Họ và tên thành viên 1] | [MSSV 1] | Viết báo cáo, thiết kế FSM, quản lý tài liệu & Git |
| 2 | [Họ và tên thành viên 2] | [MSSV 2] | Lập trình driver phần cứng, scheduler O(1), kiểm thử mạch |

---

## 📌 Giới thiệu dự án
Hệ thống điều khiển đèn giao thông cho ngã tư 2 tuyến đường hoạt động trên vi điều khiển **STM32F103RB** (mạch XNUCLEO-F103RB) kết hợp board mở rộng GROVE Shield. Toàn bộ tác vụ được điều phối bởi **Hệ thống định thời thời gian thực (Scheduler)** với thuật toán cập nhật đạt độ phức tạp **$O(1)$**.

### Các tính năng chính:
1. **Chế độ Tự động (Auto Mode):** Hai tuyến đèn Đỏ – Xanh – Vàng hoạt động luân phiên theo chu kỳ thời gian cài đặt.
2. **Chế độ Bằng tay (Manual Mode):** Cho phép người dùng chủ động can thiệp chuyển đổi pha đèn khẩn cấp bằng nút nhấn mà vẫn đảm bảo an toàn giao thông.
3. **Chế độ Chỉnh thời gian (Tuning Mode):** Cài đặt linh hoạt thời gian sáng của các đèn Đỏ, Xanh, Vàng thông qua các nút bấm.
4. **Giám sát môi trường & Hiển thị:** Đọc dữ liệu nhiệt độ, độ ẩm thời gian thực từ cảm biến **DHT20** và hiển thị lên màn hình **LCD I2C**.

---

## 🔌 Sơ đồ kết nối phần cứng (Pinout)

### 1. Module đèn giao thông (2 chân tín hiệu / module)
| Module | Chân Shield GROVE | Chân STM32 | User Label | Chế độ GPIO |
|:---|:---:|:---:|:---:|:---:|
| **Tuyến 1 (TL1)** | D3 | **PB3** | `TL1_A` | Output Push-Pull |
| | D4 | **PB5** | `TL1_B` | Output Push-Pull |
| **Tuyến 2 (TL2)** | D5 | **PB4** | `TL2_A` | Output Push-Pull |
| | D6 | **PB10** | `TL2_B` | Output Push-Pull |

*Bảng tổ hợp logic điều khiển đèn:*
* `A = 0, B = 0`: Tắt
* `A = 1, B = 0`: Đèn XANH (Green)
* `A = 0, B = 1`: Đèn VÀNG (Amber)
* `A = 1, B = 1`: Đèn ĐỎ (Red)

### 2. Module nút nhấn & Ngoại vi
| Thiết bị | Chân Shield | Chân STM32 | User Label | Chức năng |
|:---|:---:|:---:|:---:|:---|
| Nút 1 | D7 | **PA8** | `BTN1` | **MODE:** Chuyển đổi giữa các chế độ |
| Nút 2 | D8 | **PA9** | `BTN2` | **UP:** Tăng giá trị cài đặt / Đổi pha đèn manual |
| Nút 3 | D9 | **PC7** | `BTN3` | **DOWN:** Giảm giá trị cài đặt |
| Nút 4 | D10 | **PB6** | `BTN4` | **SET:** Xác nhận lưu thông số thời gian |
| Cảm biến DHT20 | I2C | **PB8 (SCL), PB9 (SDA)** | `I2C1` | Đọc nhiệt độ, độ ẩm (Địa chỉ: `0x38`) |
| Màn hình LCD | I2C | **PB8 (SCL), PB9 (SDA)** | `I2C1` | Hiển thị thông số (Địa chỉ: `0x21`) |
| LED onboard | – | **PA5** | `LD2` | Nháy chu kỳ 1s (Kiểm tra hệ thống) |

---

## 🏗️ Cấu trúc thư mục mã nguồn
```text
btl/
├── Core/
│   ├── Inc/
│   │   ├── button.h            # Driver xử lý chống dội, quét nút nhấn
│   │   ├── dht20.h             # Giao tiếp I2C đọc cảm biến DHT20
│   │   ├── fsm_automatic.h     # Máy trạng thái chế độ Tự động
│   │   ├── fsm_manual.h        # Máy trạng thái chế độ Bằng tay
│   │   ├── fsm_tuning.h        # Máy trạng thái chế độ Chỉnh thời gian
│   │   ├── global.h            # Định nghĩa biến toàn cục, hằng số, enum
│   │   ├── lcd.h               # Driver điều khiển màn hình LCD I2C
│   │   ├── scheduler.h         # Bộ lập lịch task Scheduler O(1)
│   │   ├── software_timer.h    # Định thời phần mềm
│   │   └── traffic_light.h     # Driver điều khiển module đèn
│   └── Src/                    # Các file mã nguồn .c tương ứng
├── Drivers/                    # Thư viện STM32F1xx HAL & CMSIS
├── btl.ioc                     # File cấu hình phần cứng STM32CubeMX
└── README.md
```

---

## 🚀 Hướng dẫn Build và Nạp chương trình

### Yêu cầu phần mềm
* **STM32CubeIDE** (phiên bản 1.10.0 trở lên)
* Bộ nạp ST-Link tích hợp trên mạch NUCLEO

### Các bước thực hiện:
1. **Clone repository về máy:**
   ```bash
   git clone https://github.com/<tai-khoan>/<ten-repo>.git
   cd <ten-repo>
   git checkout dev
   ```
2. **Mở dự án trên STM32CubeIDE:**
   * Mở STM32CubeIDE $\rightarrow$ Chọn `File` $\rightarrow$ `Open Projects from File System...`
   * Nhấn `Directory...`, trỏ tới thư mục dự án vừa clone và nhấn `Finish`.
3. **Kiểm tra cấu hình quan trọng:**
   * Mở file `btl.ioc` $\rightarrow$ `System Core` $\rightarrow$ `SYS` $\rightarrow$ Mục **Debug** bắt buộc chọn **Serial Wire**.
4. **Build & Nạp:**
   * Nhấn tổ hợp `Ctrl + B` để biên dịch mã nguồn (Build Project).
   * Cắm cáp kết nối mạch XNUCLEO-F103RB với máy tính qua cổng ST-LINK.
   * Nhấn nút **Run** (biểu tượng mũi tên xanh) để nạp code xuống mạch.
