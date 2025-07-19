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

将 wolserver 上传到 OpenWrt 路由器的 `/etc` 目录，并赋予执行权限：  
```
scp wolserver root@<openwrt_ip>:/etc/wolserver  
ssh root@<openwrt_ip> chmod +x /etc/wolserver  
```

确保路由器 LAN 接口可以发送广播包。

## OpenWrt 启动脚本

把下面内容保存为 `/etc/init.d/hkewol`，然后赋予可执行权限：

```
#!/bin/sh /etc/rc.common  
# 文件：/etc/init.d/hkewol  

START=99  
USE_PROCD=1  

DAEMON=hkewol  
PROG="/etc/wolserver"  

start_service() {  
    echo "Starting WOLServer..."  
    procd_open_instance $DAEMON  
    procd_set_param command $PROG  
    procd_set_param pidfile /var/run/${DAEMON}.pid  
    procd_set_param respawn 3  
    procd_close_instance  
}  
```

启用并启动服务：  
```
/etc/init.d/hkewol enable  
/etc/init.d/hkewol start
```
