#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>

#define LISTEN_PORT    2223
#define WAKEUP_PREFIX  0xFA
#define MAC_LEN        6
#define RECV_LEN       7
#define MAGIC_HDR_LEN  6
#define MAGIC_REPEATS  16
#define MAGIC_PKT_LEN  (MAGIC_HDR_LEN + MAGIC_REPEATS * MAC_LEN)

// 从系统接口列表中找第一个有效的广播地址
int get_lan_broadcast(struct in_addr *bcast_addr) {
    struct ifaddrs *ifa, *p;
    if (getifaddrs(&ifa) < 0) {
        perror("getifaddrs");
        return -1;
    }
    for (p = ifa; p; p = p->ifa_next) {
        // 要求 IPv4、非回环、支持广播
        if (p->ifa_addr && p->ifa_addr->sa_family == AF_INET &&
            !(p->ifa_flags & IFF_LOOPBACK) &&
            (p->ifa_flags & IFF_BROADCAST) &&
            (p->ifa_flags & IFF_UP)) 
        {
            struct sockaddr_in *sin = (struct sockaddr_in *)p->ifa_broadaddr;
            *bcast_addr = sin->sin_addr;
            freeifaddrs(ifa);
            return 0;
        }
    }
    freeifaddrs(ifa);
    fprintf(stderr, "No broadcast-capable interface found\n");
    return -1;
}

int main(void) {
    int sock;
    struct sockaddr_in listen_addr, bcast_addr, peer;
    socklen_t peer_len = sizeof(peer);
    uint8_t recv_buf[RECV_LEN], mac[MAC_LEN], magic[MAGIC_PKT_LEN];
    int opt, n, i;

    // 创建 UDP Socket 并开启重用与广播
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }
    opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    // 绑定监听端口
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(LISTEN_PORT);
    if (bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind"); close(sock); exit(1);
    }

    // 自动获取 LAN 广播地址
    struct in_addr lan_brd;
    if (get_lan_broadcast(&lan_brd) < 0) {
        close(sock);
        exit(1);
    }
    memset(&bcast_addr, 0, sizeof(bcast_addr));
    bcast_addr.sin_family = AF_INET;
    bcast_addr.sin_port = htons(LISTEN_PORT);
    bcast_addr.sin_addr = lan_brd;

    printf("Listening on UDP %d, LAN broadcast: %s\n",
           LISTEN_PORT, inet_ntoa(lan_brd));

    while (1) {
        n = recvfrom(sock, recv_buf, RECV_LEN, 0,
                     (struct sockaddr *)&peer, &peer_len);
        if (n == RECV_LEN && recv_buf[0] == WAKEUP_PREFIX) {
            memcpy(mac, recv_buf + 1, MAC_LEN);
            printf("WakeUp MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   mac[0], mac[1], mac[2],
                   mac[3], mac[4], mac[5]);

            // 构建魔术包
            for (i = 0; i < MAGIC_HDR_LEN; i++) {
                magic[i] = 0xFF;
            }
            for (i = 0; i < MAGIC_REPEATS; i++) {
                memcpy(magic + MAGIC_HDR_LEN + i*MAC_LEN, mac, MAC_LEN);
            }

            // 发送到自动检测到的 LAN 广播地址
            sendto(sock, magic, MAGIC_PKT_LEN, 0,
                   (struct sockaddr *)&bcast_addr, sizeof(bcast_addr));
            printf("Magic packet sent to %s:%d\n",
                   inet_ntoa(bcast_addr.sin_addr), LISTEN_PORT);
        }
    }

    close(sock);
    return 0;
}
