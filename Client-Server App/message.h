#include <unistd.h>
#include <stdint.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
using namespace std;

#define TOPIC_LEN 50
#define CONTENT_LEN 1500

typedef struct udp_msg *UDP_msg;

struct udp_msg {
	char topic[TOPIC_LEN];
    uint8_t data_type;
	char content[CONTENT_LEN];
} __attribute__((packed));

string simplify_msg (string ip_addr, int port_no, udp_msg msg);
