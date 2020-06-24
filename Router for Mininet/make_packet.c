#include "make_packet.h"
#include "skel.h"
#include "arp.h"
#define IP_OFF (sizeof(struct ether_header))
#define ICMP_OFF (IP_OFF + sizeof(struct iphdr))


packet make_ICMP_packet (packet msg, uint8_t type, uint8_t code) {
    packet pkt;
    memset(pkt.payload, 0, sizeof(pkt.payload));
    if (type == ICMP_DEST_UNREACH || type == ICMP_TIME_EXCEEDED)
        pkt.len = sizeof(struct ether_header) + sizeof(struct iphdr)
			+ sizeof(struct icmphdr);
    else pkt.len = msg.len;

    pkt.interface = msg.interface;

    struct ether_header *pkt_eth_hdr = (struct ether_header *)pkt.payload;
	struct iphdr *pkt_ip_hdr = (struct iphdr *)(pkt.payload + IP_OFF);
	struct icmphdr *pkt_icmp_hdr = (struct icmphdr *)(pkt.payload + ICMP_OFF);

    struct ether_header *msg_eth_hdr = (struct ether_header *)msg.payload;
	struct iphdr *msg_ip_hdr = (struct iphdr *)(msg.payload + IP_OFF);
    struct icmphdr *msg_icmp_hdr = (struct icmphdr *)(msg.payload + ICMP_OFF);
	
    //interchange MAC sender and destination addresses
    memcpy (pkt_eth_hdr->ether_dhost, msg_eth_hdr->ether_shost, 6);
    memcpy (pkt_eth_hdr->ether_shost, msg_eth_hdr->ether_dhost, 6);
    
    pkt_eth_hdr->ether_type = msg_eth_hdr->ether_type;

	pkt_ip_hdr->version = msg_ip_hdr->version;
	pkt_ip_hdr->ihl = msg_ip_hdr->ihl;
	pkt_ip_hdr->tos = msg_ip_hdr->tos;
	pkt_ip_hdr->tot_len = htons(pkt.len - sizeof(struct ether_header));
	pkt_ip_hdr->id = msg_ip_hdr->id;
	pkt_ip_hdr->frag_off = 0;
	pkt_ip_hdr->ttl = 64;
	pkt_ip_hdr->protocol = IPPROTO_ICMP;
	pkt_ip_hdr->daddr = msg_ip_hdr->saddr;
	pkt_ip_hdr->saddr = msg_ip_hdr->daddr;
   
	pkt_ip_hdr->check = 0;
	pkt_ip_hdr->check = ip_checksum(pkt_ip_hdr, sizeof(struct iphdr));
	
    memcpy(pkt_icmp_hdr, msg_icmp_hdr, pkt.len - ICMP_OFF);

	pkt_icmp_hdr->code = code;
	pkt_icmp_hdr->type = type;
	pkt_icmp_hdr->un.echo.id = msg_icmp_hdr->un.echo.id;
	pkt_icmp_hdr->checksum = 0;
	pkt_icmp_hdr->checksum = ip_checksum(pkt_icmp_hdr, pkt.len - ICMP_OFF);

    return pkt;
}

packet make_ARP_reply (packet msg, uint8_t *target_mac, int interface) {
    packet pkt;
    memset(pkt.payload, 0, sizeof(pkt.payload));
	
    pkt.len = sizeof(struct ether_header) + sizeof (struct arpheader);
    pkt.interface = msg.interface;
    struct ether_header *pkt_eth_hdr = (struct ether_header *)pkt.payload;
	struct arpheader *pkt_arp_hdr = (struct arpheader *)(pkt.payload + IP_OFF);

    struct ether_header *msg_eth_hdr = (struct ether_header *)msg.payload;
	struct arpheader *msg_arp_hdr = (struct arpheader *)(msg.payload + IP_OFF);
	
    //target mac will be the requested interface's MAC (now sender's MAC)
    //dhost will be the one who made the ARP request to which we're responding
    memcpy (pkt_eth_hdr->ether_dhost, msg_eth_hdr->ether_shost, 6);
    memcpy (pkt_eth_hdr->ether_shost, target_mac, 6);
    
    pkt_eth_hdr->ether_type = msg_eth_hdr->ether_type;
   
    memcpy (pkt_arp_hdr->sender_mac, target_mac, 6);
    memcpy (pkt_arp_hdr->target_mac, msg_arp_hdr->sender_mac, 6);

    //ip of the interface we're sending the reply from (which will most likely
    //differ from the one where we received the request) 
    memcpy (pkt_arp_hdr->sender_ip, 
            make_ip(inet_addr(get_interface_ip(interface))), 4);
    memcpy (pkt_arp_hdr->target_ip, msg_arp_hdr->sender_ip, 4);
    
    pkt_arp_hdr->opcode = htons(2); //ARP reply operation code
    pkt_arp_hdr->hlen = msg_arp_hdr->hlen;
    pkt_arp_hdr->plen = msg_arp_hdr->plen;
    pkt_arp_hdr->htype = msg_arp_hdr->htype;
    pkt_arp_hdr->ptype = msg_arp_hdr->ptype;

    return pkt;
}

packet make_ARP_request (packet msg, uint32_t target_ip, 
    int route_interface) {

    packet pkt;
    memset(pkt.payload, 0, sizeof(pkt.payload));
	
    pkt.len = sizeof(struct ether_header) + sizeof (struct arpheader);
    pkt.interface = route_interface;
    struct ether_header *pkt_eth_hdr = (struct ether_header *)pkt.payload;
	struct arpheader *pkt_arp_hdr = (struct arpheader *)(pkt.payload + IP_OFF);

    uint8_t *target_mac = malloc(6);
    uint8_t *sender_mac = malloc(6);

    //make the destination a broadcast MAC: ff:ff:ff:ff:ff:ff
    for (int i = 0; i < 6; i++) 
        target_mac[i] = 0xff;
   
    //get the mac of the interface we're sending from
    get_interface_mac(route_interface, sender_mac); 
    
    memcpy (pkt_eth_hdr->ether_dhost, target_mac , 6);
    memcpy (pkt_eth_hdr->ether_shost, sender_mac, 6);
    
    memcpy (pkt_arp_hdr->sender_mac, sender_mac, 6);
    memcpy (pkt_arp_hdr->target_mac, target_mac, 6);
    
    pkt_eth_hdr->ether_type = htons(0x806); //ARP packet

    uint8_t *ip = make_ip(inet_addr(get_interface_ip(pkt.interface)));
    memcpy (pkt_arp_hdr->sender_ip, ip, 4);
    memcpy (pkt_arp_hdr->target_ip, &target_ip, 4);
    
    pkt_arp_hdr->opcode = htons(1); //operation code for ARP request
    pkt_arp_hdr->hlen = 6; //for ethernet
    pkt_arp_hdr->plen = 4; 
    pkt_arp_hdr->htype = htons(1); //ethernet address length
    pkt_arp_hdr->ptype = htons(0x800); //IPv4/protocol length

    return pkt;
}