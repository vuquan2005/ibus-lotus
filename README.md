# IBus Lotus - Bộ gõ tiếng Việt cho Linux

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://opensource.org/licenses/GPL-3.0)

IBus Lotus là bản fork của [ibus-bamboo](https://github.com/BambooEngine/ibus-bamboo/), được khởi đầu bởi [hien-ngo29](https://github.com/hien-ngo29). Dự án hiện được duy trì và tiếp tục phát triển bởi [vuquan2005](https://github.com/vuquan2005/ibus-lotus).

Hiện các bộ gõ mới xuất hiện rất nhiều mình thử vài cái và đều còn rất nhiều lỗi. Vấn đề hiện tại của các bộ gõ hiện tại là upstream wayland với text-input-v3 đã không còn hỗ trợ được kiểu gõ gửi phím back space kiểu Unikey được nữa, buộc phải dùng pre-edit (chữ không hiện ngay mà ở bộ nhớ đệm gõ xong mới đưa vào ứng dụng -> không hiện gợi ý khi search/code/...) hoặc dùng surround text (nhiều ứng dụng không hỗ trợ ví dụ terminal-shell hoặc hỗ trợ không tốt/lỗi và hơn thế là mỗi một web fb/mess/... lại xử lý khác nhau).
Repo này sẽ không giải quyết được việc đó =)) vì nó đến từ các ông lớn upstream và các ứng dụng không hỗ trợ tốt nên kệ nó đi. Mình không đủ trình độ và thời gian để tìm hiểu đóng góp cho nó, và mình cũng sẽ không cố gắng hacky hay tìm cách đi vòng, đi ngược upstream vì vốn dĩ upstream là xu thế của công nghệ, đi ngược với nó rất tốn công sức bảo trì và không ổn định.

Repo này mình chỉ tập trung vào 2 chế độ pre-edit và surround text cũng như việc chuyển đổi vi/en.

Tại sao chọn IBus? Vì Gnome mặc định vẫn dùng ibus và mình không tìm thấy repo nào hỗ trợ ibus cả và mình lười cài thêm

Tại sao không phải fcitx5? Dù là xu thế, cộng đồng tích cực nhưng đôi khi nó không được cài sẵn mọi người cài đặt đôi khi khá phức tạp.

Tại sao không đóng góp cho các dự án trên fcitx5 khác? Vì tôi lười =)) Mấy dự án suốt ngày cãi nhau vibe code, đè tem, phát minh lại cái đã có, đi ngược upstream, convert sang ngôn ngữ khác như rust, c,.. mệt lắm nên cứ tạm dùng cái này cho đến khi ai đó đứng lên làm một bộ gõ hoàn thiện hơn.

Những thay đổi của tôi:

- Sửa lại các phím tắt hoạt động ổn định trở lại.
- Loại bỏ các chế độ gõ không hoạt động trên Wayland bao gồm: `ForwardKeyEvent I/II`, `Forward as commit`, và `XTestFakeKeyEvent`.

Những thay đổi đáng chú ý đã được thêm vào ibus-lotus so với ibus-bamboo:

- Fix vấn đề lặp lại từ cuối trong một số trang web.
- Fix vấn đề không nhấn được `super + space` để chuyển đổi bộ gõ ibus trên Wayland.
- Fix vấn đề nhấp chuột bị hiện bảng Remote Interaction trên GNOME.
- Fix vấn đề nhấp chuột bị nhảy từ đang gõ từ ô nhập liệu khác trên Wayland, đồng thời option `Bắt sự kiện chuột` cũng đã được loại bỏ.
- Fix vấn đề không mở được bảng tùy chọn chế độ gõ trên Wayland cho GNOME và KDE Plasma.

## Sơ lược tính năng

- Hỗ trợ tất cả các bảng mã phổ biến:
  - Unicode, TCVN (ABC)
  - VIQR, VNI, VPS, VISCII, BK HCM1, BK HCM2,…
  - Unicode UTF-8, Unicode NCR - for Web editors.
- Các kiểu gõ thông dụng:
  - Telex, Telex W, Telex 2, Telex + VNI + VIQR
  - VNI, VIQR, Microsoft layout
- Nhiều tính năng hữu ích, dễ dàng tùy chỉnh:
  - Kiểm tra chính tả (sử dụng từ điển/luật ghép vần)
  - Dấu thanh chuẩn và dấu thanh kiểu mới
  - Bỏ dấu tự do, Gõ tắt,...
  - 2666 emojis từ [emojiOne](https://github.com/joypixels/emojione)
- Sử dụng phím tắt <kbd>Shift</kbd>+<kbd>~</kbd> để loại trừ ứng dụng không dùng bộ gõ, chuyển qua lại giữa các chế độ gõ:
  _ Pre-edit (default)
  _ Surrounding text,...
  ![ibus-lotus](./demo.gif)
- Khác với ibus-bamboo, ibus-lotus hiện tại đã hỗ trợ Wayland khá tốt trên 2 Desktop Environment chính đó là GNOME và KDE (Plasma). Chỉ cần yêu cầu các bạn cài một số extension và tool bên thứ ba (Xem hướng dẫn cài đặt bên dưới).

## Installation

Note: vì một số lý do mà mình không kham nổi việc publish ibus-lotus cho các kho package manager của từng distro nên hiện tại các bạn chỉ có 2 lựa chọn để install đó là cài từ phiên bản prebuilt hoặc build từ source (Hoặc AUR nếu bạn dùng Arch)

### Arch Linux [![AUR version](https://img.shields.io/aur/version/ibus-lotus)](https://aur.archlinux.org/packages/ibus-lotus)

ibus-lotus đã có mặt tại AUR do [shadichy](https://github.com/shadichy) là maintainer.

### Cài từ prebuilt

1. ibus-lotus/ibus-bamboo sử dụng IME ibus. Hãy nhớ rõ và chắc rằng bạn đã setup ibus đúng cách. Trên hầu hết các Distro và Desktop Environment thông dụng ibus thường đã được cài sẵn nên bạn có thể bỏ qua bước này.
2. Download file `ibus-lotus-<version>.zip` tại phần [Release của Repo](https://github.com/vuquan2005/ibus-lotus/releases/).

3. Giải nén file và install (thay `<version>` thành phiên bản đúng mà bạn đã download, ví dụ `1.0.0`):

```bash
unzip ibus-lotus-<version>.zip
cd ibus-lotus-<version>

chmod +x ./install
sudo ./install
```

Có thể bạn sẽ cần phải log out ra session của Desktop Environment bạn đang dùng và đăng nhập lại để ibus-lotus/ibus-bamboo xuất hiện trên Input Source trong Settings.

4. Thêm Input Source tại Settings của Desktop Environment bạn đang dùng. Bạn sẽ thấy một input source mang tên `Vietnamese (Bamboo)` tại phần Vietnamese (các bạn có thể Google cách thêm Input Source cho Desktop Environment bạn đang dùng).

### Cài từ source code

Xem hướng dẫn [build từ source](./docs/building_instructions.md).

### Note cho Wayland

Hãy cài những thứ này để tránh việc không mở được bảng chọn chế độ gõ trên Wayland.

**GNOME**: cài đặt extension [Window Call Extended](https://extensions.gnome.org/extension/4974/window-calls-extended/)

**KDE Plasma**: cài đặt `kdotool` từ package manager của distro.

**Nếu có thắc mắc hay trục trặc về việc cài đặt ibus-lotus hãy thoải mái [mở issue](https://github.com/vuquan2005/ibus-lotus/issues/new) trên repo này**

## Xin cám ơn các contributor của IBus Bamboo <3

<a href="https://github.com/vuquan2005/ibus-lotus/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=BambooEngine/ibus-bamboo" />
</a>
