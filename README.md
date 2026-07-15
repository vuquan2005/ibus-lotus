# IBus Lotus - Bộ gõ tiếng Việt cho Linux

IBus Lotus là dự án được kế thừa và phát triển tiếp từ [IBus Bamboo](https://github.com/BambooEngine/ibus-bamboo) thông qua bản fork [IBus Lotus](https://github.com/LotusInputEngine/ibus-lotus) đều đã bị đình trệ/ngừng phát triển [(BambooEngine/ibus-bamboo#590)](https://github.com/BambooEngine/ibus-bamboo/issues/590#issuecomment-3762683651). Dự án này nhằm mục đích tiếp tục duy trì để duy trì bộ gõ hoạt động trên các hệ thống Linux hiện đại.

Dự án không hướng đến việc xây dựng một bộ gõ mới hay thay đổi cách hoạt động của IBus. Mục tiêu đơn giản là một bản IBus Bamboo tối giản, chỉ dành cho Wayland.

IBus Lotus được phát triển theo hướng kế thừa IBus Bamboo đồng thời loại bỏ hoặc điều chỉnh những phần không còn phù hợp với Wayland và môi trường Linux hiện nay. Phạm vi của dự án chủ yếu là sửa lỗi, cải thiện khả năng tương thích và bảo trì mã nguồn, thay vì bổ sung nhiều cơ chế nhập liệu hoặc theo đuổi các hướng triển khai mới.

## Cài đặt

Vui lòng xem hướng dẫn build từ mã nguồn tại:

- [docs/building_instructions.md](./docs/building_instructions.md)

---

## Vì sao dự án này tồn tại?

Trong vài năm gần đây, Linux desktop đã chuyển dần từ **X11** sang **Wayland**

Điều này kéo theo một thay đổi lớn đối với các bộ gõ tiếng Việt

Trước đây, nhiều bộ gõ hoạt động bằng cách gửi lại phím (`Backspace`, `KeyEvent`, `XTestFakeKeyEvent`,...) theo kiểu Unikey trên Windows. Cách làm này hoạt động khá tốt trên X11 nhưng ngày càng không còn phù hợp với Wayland và giao thức `text-input-v3`

Hiện nay, các bộ gõ gần như chỉ còn hai hướng chính:

- **Pre-edit** (IME giữ văn bản trong bộ nhớ đệm rồi commit)

- **Surrounding Text** (ứng dụng cho phép IME chỉnh sửa văn bản đã nhập)

Mỗi hướng đều có ưu và nhược điểm:

- **Pre-edit**
  - Hoạt động ổn định

  - Được Wayland hỗ trợ tốt

  - Tuy nhiên văn bản chưa xuất hiện ngay trong ứng dụng nên đôi khi không hiển thị gợi ý tìm kiếm hoặc autocomplete theo từng ký tự

- **Surrounding Text**
  - Trải nghiệm gần giống gõ trực tiếp

  - Phù hợp với nhiều ô tìm kiếm

  - Nhưng không phải ứng dụng nào cũng hỗ trợ tốt (đặc biệt là terminal, một số editor hoặc ứng dụng web)

Đây không phải vấn đề riêng của IBus Lotus mà là giới hạn chung của hệ sinh thái Wayland hiện nay

## Triết lý của dự án

IBus Lotus **không cố gắng chống lại upstream** trong việc gửi phím. Dự án tập trung tối giản hóa động cơ gõ, ưu tiên các cơ chế hiện đại của Wayland thay vì giả lập phím thô kiểu cũ.

Dự án sẽ không:

- Phát sinh thêm các cơ chế gửi phím kiểu X11

- Giả lập Backspace hàng loạt (loại bỏ hoàn toàn Backspace Faker)

- Đi vòng qua các API nhập liệu để "hoạt động giống Windows" khi gửi phím tiếng Việt

Những cách gửi phím cũ này thường:

- khó bảo trì

- dễ lỗi theo từng compositor

- dễ bị hỏng sau mỗi bản cập nhật

Thay vào đó, IBus Lotus chỉ tập trung vào những gì upstream đang khuyến khích sử dụng:
- Cải thiện **Pre-edit**
- Cải thiện **Surrounding Text**

### Ngoại lệ về các tính năng hỗ trợ nân cao trải nghiệm người dùng

Mặc dù hướng tới sự tối giản, dự án hiểu rằng tính năng tự động chuyển đổi chế độ gõ theo từng ứng dụng là cần thiết cho trải nghiệm người dùng trong khi các phương thức hiện tại vẫn còn hạn chế. Vì Wayland hạn chế việc lấy thông tin cửa sổ để đảm bảo bảo mật, tính năng này được triển khai dưới dạng **tùy chọn (opt-in)** và cần các công cụ hỗ trợ ngoài luồng tùy theo môi trường desktop của bạn (xem chi tiết ở phần hướng dẫn thiết lập bên dưới).

## Mục tiêu

IBus Lotus chỉ tập trung vào việc:

- Cải thiện **Pre-edit**

- Cải thiện **Surrounding Text**

Nếu upstream hoặc các ứng dụng cải thiện khả năng hỗ trợ IME thì IBus Lotus sẽ tận dụng các khả năng đó thay vì tự tạo thêm workaround

---

## Vì sao vẫn chọn IBus?

Repository này tồn tại trước hết để phục vụ chính nhu cầu sử dụng của tác giả. Mình sử dụng GNOME với IBus hằng ngày nên việc tiếp tục bảo trì một engine IBus phù hợp với nhu cầu hơn là bắt đầu lại trên một framework khác.

## Khác gì so với [IBus Lotus](https://github.com/LotusInputEngine/ibus-lotus)?

Những thay đổi chính:

- Sửa các phím tắt hoạt động ổn định hơn

- Loại bỏ các cơ chế gõ cũ không còn phù hợp với Wayland:
  - XTestFakeKeyEvent

  - ForwardKeyEvent I

  - ForwardKeyEvent II

  - Forward as Commit

  - Backspace Faker

- Tập trung duy trì và cải thiện hai chế độ:
  - Pre-edit

  - Surrounding Text

- Thiết kế lại cơ chế chuyển đổi:
  - Pre-edit

  - Surrounding Text

  - English

- Tinh gọn giao diện và loại bỏ các tuỳ chọn ít còn giá trị sử dụng

- Hỗ trợ tính năng tự động chuyển đổi chế độ gõ (Auto-switch) 3 cấp độ (cửa sổ cụ thể `hwnd`, lớp ứng dụng `wm_class`, hoặc chế độ mặc định) kèm theo trình quản lý quy tắc ứng dụng trong Setup GUI.

## Kế thừa từ IBus Lotus

Dưới đây là các thay đổi và tính năng đáng chú ý được kế thừa từ dự án [IBus Lotus](https://github.com/LotusInputEngine/ibus-lotus) của tác giả [hien-ngo29](https://github.com/hien-ngo29):

### Thay đổi đáng chú ý so với [IBus Bamboo](https://github.com/BambooEngine/ibus-bamboo)

- Sửa lỗi lặp từ cuối trên một số trang web.

- Sửa lỗi không nhấn được phím tắt `Super + Space` để chuyển đổi bộ gõ trên Wayland.

- Sửa lỗi click chuột làm xuất hiện thanh Remote Interaction trên GNOME.

- Sửa lỗi click chuột làm nhảy từ đang gõ từ ô nhập liệu khác trên Wayland (đồng thời loại bỏ tùy chọn `Bắt sự kiện chuột`).

- Sửa lỗi không mở được bảng tùy chọn chế độ gõ trên Wayland dành cho GNOME và KDE Plasma.

### Sơ lược tính năng

- Hỗ trợ tất cả các bảng mã phổ biến: Unicode, TCVN (ABC), VIQR, VNI, VPS, VISCII, BK HCM1, BK HCM2, Unicode UTF-8, Unicode NCR (dành cho Web editor).

- Hỗ trợ các kiểu gõ thông dụng: Telex, Telex W, Telex 2, Telex + VNI + VIQR, VNI, VIQR, Microsoft layout.

- Nhiều tính năng tiện ích, dễ dàng tùy chỉnh:
  - Kiểm tra chính tả (sử dụng từ điển và luật ghép vần)

  - Dấu thanh chuẩn và dấu thanh kiểu mới

  - Bỏ dấu tự do, gõ tắt

  - Tích hợp 2666 emojis từ [emojiOne](https://github.com/joypixels/emojione) (hiện tại mình đã loại bỏ phần này, bạn có thể dùng ctrl+. hoặc super+. thử hoặc cài ứng dụng emoji khác)

- Hỗ trợ Wayland tốt trên hai môi trường Desktop Environment chính là GNOME và KDE (Plasma) khi thiết lập thêm các phần mở rộng hoặc công cụ bên thứ ba (chi tiết có thể tham khảo thêm tại hướng dẫn cài đặt của [repo IBus Lotus gốc](https://github.com/LotusInputEngine/ibus-lotus)).

## Hướng dẫn thiết lập tính năng Tự động chuyển đổi chế độ gõ (Auto-Switch)

Do các hạn chế bảo mật của Wayland, IBus Lotus không thể tự động lấy thông tin cửa sổ đang focus một cách trực tiếp. Để sử dụng tính năng này, bạn cần thiết lập thêm:

### 1. Trên GNOME (Wayland)
Bạn cần cài đặt extension **Focused Window** để xuất thông tin cửa sổ active qua DBus:
1. Truy cập [Focused Window GNOME Extension](https://extensions.gnome.org/extension/4412/focused-window/) hoặc cài đặt qua ứng dụng **Extension Manager**.
2. Bật Extension này lên.
3. Mở cài đặt IBus Lotus -> Tab **Tự động chuyển đổi** -> Bật các quy tắc theo mong muốn.

### 2. Trên KDE Plasma (Wayland)
Bạn cần cài đặt công cụ `kdotool` để hỗ trợ truy vấn thông tin cửa sổ thông qua KWin Scripting:
* **Arch Linux**:
  ```bash
  yay -S kdotool-git
  ```
* **Fedora**:
  ```bash
  sudo dnf install kdotool
  ```
* Hoặc build từ mã nguồn tại [kdotool GitHub](https://github.com/lucastr/kdotool).

## Đóng góp

Mọi Pull Request đều được chào đón

Đặc biệt nếu bạn có thể:

- sửa lỗi

- cải thiện trải nghiệm Wayland

- tối ưu mã nguồn

- hoặc bổ sung tài liệu

Xin vui lòng mở Issue trước nếu thay đổi có ảnh hưởng lớn đến kiến trúc của dự án

## Lời cảm ơn

Dự án được kế thừa và phát triển từ mã nguồn của bộ gõ [IBus Bamboo](https://github.com/BambooEngine/ibus-bamboo) và [IBus Lotus](https://github.com/LotusInputEngine/ibus-lotus).

Xin chân thành cảm ơn tập thể tất cả các contributor của các dự án gốc đã đóng góp và giúp cộng đồng Linux có một bộ gõ tiếng Việt chất lượng trong nhiều năm qua.
