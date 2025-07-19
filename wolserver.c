#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define LISTEN_PORT    2223
#define WAKEUP_PREFIX  0xFA
#define MAC_LEN        6
#define RECV_LEN       7
#define MAGIC_HDR_LEN  6
#define MAGIC_REPEATS  16
#define MAGIC_PKT_LEN  (MAGIC_HDR_LEN + MAGIC_REPEATS * MAC_LEN)

// 根据你的 LAN 网络环境，填写正确的广播地址
static const char *LAN_BROADCAST = "192.168.1.255";

int main(void) {
    int sock, n;
    struct sockaddr_in addr, bcast_lan, peer;
    socklen_t peer_len = sizeof(peer);
    uint8_t recv_buf[RECV_LEN], mac[MAC_LEN], magic[MAGIC_PKT_LEN];
    int i;

    // 创建 UDP socket 并设置重用地址、广播权限
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    // 绑定到本地所有地址和指定端口
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(LISTEN_PORT);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 配置 LAN 段广播目标
    memset(&bcast_lan, 0, sizeof(bcast_lan));
    bcast_lan.sin_family = AF_INET;
    bcast_lan.sin_port = htons(LISTEN_PORT);
    inet_aton(LAN_BROADCAST, &bcast_lan.sin_addr);

    printf("WOL Forwarder listening on UDP port %d\n", LISTEN_PORT);

    while (1) {
        // 接收唤醒包
        n = recvfrom(sock, recv_buf, RECV_LEN, 0,
                     (struct sockaddr *)&peer, &peer_len);
        if (n == RECV_LEN && recv_buf[0] == WAKEUP_PREFIX) {
            // 提取并打印目标 MAC
            memcpy(mac, recv_buf + 1, MAC_LEN);
            printf("WakeUp MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            // 构建魔术包：6 字节 0xFF + 16 × MAC
            for (i = 0; i < MAGIC_HDR_LEN; i++) {
                magic[i] = 0xFF;
            }
            for (i = 0; i < MAGIC_REPEATS; i++) {
                memcpy(magic + MAGIC_HDR_LEN + i * MAC_LEN, mac, MAC_LEN);
            }

            // 仅向 LAN 广播地址发送
            sendto(sock, magic, MAGIC_PKT_LEN, 0,
                   (struct sockaddr *)&bcast_lan, sizeof(bcast_lan));
            printf("Magic packet sent to LAN broadcast %s:%d\n",
                   LAN_BROADCAST, LISTEN_PORT);
        }
    }

    close(sock);
    return 0;
}
