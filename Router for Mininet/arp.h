#include <unistd.h>
#include <stdint.h>
#include "skel.h"
#include <net/if_arp.h>
typedef struct arp_table_entry *ARPTable;

struct arp_table_entry {
	uint32_t ip;
	uint8_t mac[6];
} __attribute__((packed));

struct arpheader {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint8_t sender_ip[4];
  uint8_t target_mac[6];
  uint8_t target_ip[4];
};


uint8_t* find_arp_entry (ARPTable arp_table, uint32_t ip, int size);

void add_arp_entry (ARPTable arpt, uint32_t ip, uint8_t* mac, int size);

uint32_t get_ip (uint8_t ip[]); //turn a vector for an IP address into uint32_t

uint8_t* make_ip(uint32_t ip);  //turn a uint32_t IP address into a vector
