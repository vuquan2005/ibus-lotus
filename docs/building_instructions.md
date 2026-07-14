1. Cài `ibus`:
```bash
# Debian/Ubuntu:
sudo apt-get install ibus

# Fedora
sudo dnf install ibus

# CentOS, RHEL, ...
sudo yum install ibus

# openSUSE Tumbleweed
sudo zypper install ibus

# FreeBSD
sudo/doas pkg install ibus
```

2. Cài đặt các gói phụ thuộc
- make
- golang
- libgtk-3-dev

```bash
# Debian/Ubuntu:
sudo apt-get install make golang libgtk-3-dev

# Fedora
sudo dnf install make go gtk3-devel

# CentOS, RHEL, ...
sudo yum install make go gtk3-devel

# openSUSE Tumbleweed
sudo zypper install make go gtk3-devel

# FreeBSD
sudo/doas pkg install go pkgconf gtk3 bash
```
3. Download
```bash
wget https://github.com/vuquan2005/ibus-lotus/archive/master.zip -O ibus-lotus.zip
unzip ibus-lotus.zip

# hoặc clone từ github:
git clone https://github.com/vuquan2005/ibus-lotus.git
```
4. Build & install

**Linux**
```bash
cd ibus-lotus
sudo make install

# Restart ibus
ibus restart
```
**FreeBSD**
```bash
cd ibus-lotus
sudo make install PREFIX=/usr/local

# Restart ibus
ibus restart
```

Gỡ cài đặt
======
**Linux**
```bash
sudo make uninstall PREFIX=/usr
ibus restart
```

**FreeBSD**
```bash
sudo make uninstall PREFIX=/usr/local
ibus restart
```
