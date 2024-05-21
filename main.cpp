#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

const int READ_CHUNK_SIZE = 1024;

addrinfo* get_addrinfo(const std::string host, const std::string port) {
    struct addrinfo hints;
    struct addrinfo* result;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    int error = getaddrinfo(host.data(), port.data(), &hints, &result);
    if (error != 0) {
        throw std::invalid_argument("Failed to fetch address info: " + std::string(gai_strerror(error)));
    }

    return result;
}

int connect(std::string host, std::string port) {
    struct addrinfo* result = get_addrinfo(host, port);	
    struct addrinfo* r = result;

    int sfd;
    bool connected = false;
    while (r) {
        // Here instead of result->... I could've used the constants such as AF_INET, SOCK_STREAM, 0.
        // But in documentation they decided to go with this more flexible way, so I stuck with it.
        sfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
        if (sfd == -1) {
            freeaddrinfo(result);
            throw std::invalid_argument("Failed to open socket: " + std::string(gai_strerror(errno)));
        }

        int error = ::connect(sfd, r->ai_addr, r->ai_addrlen);
        if (error != -1) {
            connected = true;
            break;
        }

        close(sfd);

        r = r->ai_next;
    }

    freeaddrinfo(result);

    if (!connected) {
        throw std::invalid_argument("Failed to connect");
    }

    return sfd;
}

void send(int sfd, const std::string message) {
    int error = write(sfd, message.data(), message.size());
    if (error == -1) {
        throw std::invalid_argument("Failed to write: " + std::string(gai_strerror(errno)));
    }
    else if (error != message.size()) {
        throw std::invalid_argument("Partially failed to write: " + std::string(gai_strerror(errno)));
    }
}

std::string gen_random(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    return tmp_s;
}

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    std::string port = "5201";
    
    int sfd = connect(host, port);
    std::vector<unsigned char> COOKIE = { 'e', 'x', 'f', 'o', 'y', 'c', 'p', 't', 's', '4', 'l', '4', 'r', 't', '2', 'y', '6', '2', '2', 't', 'l', 'x', 'g', 'd', 'b', 'x', '7', 'k', 'm', 'o', 'u', 'j', '2', 'e', 'l', '2', 0};
    std::string message = std::string(begin(COOKIE), end(COOKIE));
    send(sfd, message);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::vector<unsigned char> v = {0, 0, 0, 125};
    message = std::string(begin(v), end(v));
    send(sfd, message);

    message = "{\"tcp\":true,\"omit\":0,\"time\":10,\"num\":0,\"blockcount\":0,\"parallel\":1,\"len\":131072,\"pacing_timer\":1000,\"client_version\":\"3.16+\"}";
    send(sfd, message);

    std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    
    int sfd2 = connect(host, port);
    message = std::string(begin(COOKIE), end(COOKIE));
    send(sfd2, message);
   
    message = gen_random(65536);
    for ( int i = 0; i < 300000; i++) {
        send(sfd2, message);
    }
    
    if (argc == 2) {
        int x = 4;
        write(sfd, (char *) &x, sizeof(signed char));
    } else {
        int x = 16;
        write(sfd, (char *) &x, sizeof(signed char));
    }
}
