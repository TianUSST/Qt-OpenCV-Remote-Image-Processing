#include "tcp_server.h"

int main() {
    TcpServer server(6666);  // 端口可修改
    server.start();

    return 0;
}