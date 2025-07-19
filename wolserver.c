#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr, broadaddr;
    socklen_t len = sizeof(cliaddr);
    unsigned char buffer[1024];
    unsigned char buffmac[6];
    unsigned char buffsend[6 + 16 * 6];  // 6 个 0xFF + 16 次 MAC

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(2223);
    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(sockfd);
        return EXIT_FAILURE;
    }

    memset(&broadaddr, 0, sizeof(broadaddr));
    broadaddr.sin_family = AF_INET;
    broadaddr.sin_port = htons(2223);
    inet_pton(AF_INET, "192.168.1.255", &broadaddr.sin_addr);

    printf("Server Start: 2223\n");

    while (1) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                         (struct sockaddr*)&cliaddr, &len);
        if (n == 7 && buffer[0] == 0xFA) {
            memcpy(buffmac, buffer + 1, 6);
            // 构造 Magic Packet
            memset(buffsend, 0xFF, 6);
            for (int i = 0; i < 16; i++) {
                memcpy(buffsend + 6 + i * 6, buffmac, 6);
            }
            sendto(sockfd, buffsend, sizeof(buffsend), 0,
                   (struct sockaddr*)&broadaddr, sizeof(broadaddr));
        }
    }
    close(sockfd);
    return EXIT_SUCCESS;
}
