#include "skel.h"
packet make_ICMP_packet (packet msg, uint8_t type, uint8_t code);

packet make_ARP_reply (packet msg, uint8_t *target_mac, int interface);

packet make_ARP_request (packet msg, uint32_t target_ip, int route_interface);