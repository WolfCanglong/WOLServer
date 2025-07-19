# WOLServer for x86 OpenWrt

这是一个用于 x86 架构 OpenWrt 的 Wake-on-LAN UDP 广播服务端。

## 功能

- UDP 监听 2223 端口
- 当收到长度为 7、首字节 0xFA 的数据包时，提取后面 6 字节为 MAC
- 广播 Magic Packet (6×0xFF + 16 次 MAC)

## 自动化构建

本项目使用 GitHub Actions 自动编译：

1. Push 代码到 `main` 分支  
2. Actions 会在 `ubuntu-latest` Runner 上执行：
   - 安装 `build-essential`
   - 使用 `gcc -Os` 优化体积编译
   - 用 `strip` 去符号
   - 上传产物 `wolserver-openwrt-x86`

## 部署

1. 在 GitHub 仓库页面 → **Actions** → 下载 `wolserver-openwrt-x86`  
2. 将可执行文件复制到 OpenWrt 路由器：
   ```bash
   scp wolserver-openwrt-x86 root@<openwrt_ip>:/usr/bin/wolserver
   ssh root@<openwrt_ip> chmod +x /usr/bin/wolserver
