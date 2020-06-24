#include "arp.h"
#include "skel.h"
#include "route_table.h"

uint8_t* find_arp_entry (ARPTable arp_table, uint32_t ip, int size){
    for (int i = 0; i < size; i++) {
        if (arp_table[i].ip == ip)
            return arp_table[i].mac;
    }
    return NULL;
}

void add_arp_entry (ARPTable arp_table, uint32_t ip, uint8_t* mac, int size) {
    arp_table[size].ip = ip;
    memcpy(arp_table[size].mac, mac, 6); 
}

uint32_t get_ip (uint8_t ip[]) {
    uint32_t uip = 0;
    uip += ip[0];
    uip = uip << 8;
    uip += ip[1];
    uip = uip << 8;
    uip += ip[2];
    uip = uip << 8;
    uip += ip[3];
    return uip;
}

uint8_t* make_ip (uint32_t ip) {
    uint8_t *uip = malloc(4);
    uip[3] = ip >> 24;
    uip[2] = (ip << 8) >> 24;
    uip[1] = (ip << 16) >> 24;
    uip[0] = (ip << 24) >> 24; 
    return uip;
}
