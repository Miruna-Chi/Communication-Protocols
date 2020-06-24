#include "skel.h"
#include "route_table.h"
#include "arp.h"
#include "queue.h"
#include "make_packet.h"
#include <stdlib.h>
#include <stdbool.h>
#include <net/if_arp.h>
#define IP_OFF (sizeof(struct ether_header))
#define ICMP_OFF (IP_OFF + sizeof(struct iphdr))


int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, 0);
	packet m;
	int rc;

	init();

	char input_filename[] = "rtable.txt";
    FILE *in = fopen(input_filename, "rt");

	char output_filename[] = "rrr.txt";
    FILE *out = fopen(output_filename, "wt");

	RTable rtable;
	ARPTable arp_table;

	rtable = malloc(sizeof(struct route_table_entry) * 100000);
	arp_table = malloc(sizeof(struct arp_table_entry) * 4);


	int r_entries = 0, arp_entries = 0;
	char prefix[16], next_hop[16], mask[16];
	int interface;

	while(fscanf(in, "%s %s %s %d ", prefix, next_hop, mask, &interface) !=-1){
		make_entry(rtable, 
			(uint32_t) inet_addr(prefix), 
			(uint32_t) inet_addr(next_hop),
			(uint32_t) inet_addr(mask),
			interface, r_entries);
		
		r_entries++;
	
	}

	quick_sort(rtable, 0, r_entries - 1); 

	queue q ;
	q = queue_create ();
	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		
		if (eth_hdr->ether_type == htons(0x800)) { //is it an IP packet?
		
			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + IP_OFF);
			int check = ip_hdr->check;
			ip_hdr->check = 0;
			if (check != ip_checksum(ip_hdr, sizeof(struct iphdr)))
				continue; //wrong checksum, drop packet
			
			if(ip_hdr->ttl <= 1) { 
				packet pkt;
				pkt = make_ICMP_packet(m, ICMP_TIME_EXCEEDED, 0);
				send_packet(m.interface, &pkt);
				continue;
			}
		
			bool is_my_ip = false; //is the packet for me?
			for (int i = 0; i < 4; i++)	
				if ((uint32_t)inet_addr(get_interface_ip(i)) == ip_hdr->daddr){
					is_my_ip = true;
					break;
				}
	
			RTable best_route;
			best_route = get_best_route(rtable, ip_hdr->daddr, r_entries, out);
					
			if (best_route == NULL) { 
				packet pkt;
				pkt = make_ICMP_packet(m, ICMP_DEST_UNREACH, 0);
				send_packet(m.interface, &pkt);
			}
			else {

				ip_hdr->ttl--;				
				ip_hdr->check = 0;
				ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));
				//do I know the mac of the next hop? 
				//No? Then send an ARP request to that interface
				uint8_t *target_mac;
				target_mac = find_arp_entry(arp_table, best_route[0].next_hop, 
								arp_entries);
				packet pkt;
				if (target_mac == NULL && !is_my_ip) {

					packet *pack = malloc(sizeof(packet));
					memcpy(pack, &m, sizeof(packet));
					queue_enq(q, pack);	
					pkt = make_ARP_request(m, best_route[0].next_hop, 
							best_route[0].interface);
					send_packet(best_route[0].interface, &pkt);
					continue;

				} else if (!is_my_ip) {
								
					memcpy (eth_hdr->ether_dhost, target_mac, 6);
					target_mac = NULL;
					target_mac = malloc(6);
					get_interface_mac(best_route[0].interface, target_mac);						
			
					memcpy (eth_hdr->ether_shost, target_mac, 6);
					send_packet(best_route[0].interface, &m);

				}
				else {
					
					if(ip_hdr->protocol == IPPROTO_ICMP){ 
						//does it have an ICMP header?
						struct icmphdr *icmp_hdr;
						icmp_hdr = (struct icmphdr *)(m.payload + ICMP_OFF);
						check = icmp_hdr->checksum;
						icmp_hdr->checksum = 0;

						if(check != ip_checksum(icmp_hdr, m.len - ICMP_OFF))
							continue; //wrong checksum, drop packet	 
						
						if(icmp_hdr->type == ICMP_ECHO && icmp_hdr->code == 0){
							
							packet pkt;
							pkt = make_ICMP_packet(m, ICMP_ECHOREPLY, 0);
							send_packet(m.interface, &pkt);
						}
					}
				}	
			}		
		}	
		else if (eth_hdr->ether_type == htons(0x806)) {//is it an ARP packet?
			struct arpheader *arp_hdr = (struct arpheader *)(m.payload + IP_OFF);
			uint8_t *target_mac;
			
			if (arp_hdr->opcode == htons(1)) { //ARP request
				
				packet pkt;
				
				target_mac = malloc(6);
				int i;
				for (i = 0; i < 4; i++)
					if ((uint32_t)inet_addr(get_interface_ip(i))
						== htonl(get_ip(arp_hdr->target_ip))){

						break;
					}
				
				get_interface_mac(i, target_mac);
				pkt = make_ARP_reply(m, target_mac, i);
				send_packet(m.interface, &pkt);
						
			}
			else if (arp_hdr->opcode == htons(2)){ //ARP reply
				
				arp_table[arp_entries].ip = htonl(get_ip(arp_hdr->sender_ip));
				memcpy(arp_table[arp_entries].mac, arp_hdr->sender_mac, 6);
				arp_entries++;
				
				while (!queue_empty(q)) {

					target_mac = find_arp_entry(arp_table, 
							htonl(get_ip(arp_hdr->sender_ip)), arp_entries);
					packet *pkt = (packet *) queue_deq(q);
					
					struct ether_header *eth_hdr;
					eth_hdr = (struct ether_header *)pkt->payload;
					struct iphdr *ip_hdr;
					ip_hdr = (struct iphdr *)(pkt->payload + IP_OFF);
						
					ip_hdr->ttl--;
					uint8_t *sender_mac = malloc(6);
					
					get_interface_mac(m.interface, sender_mac);

					memcpy (eth_hdr->ether_dhost, target_mac, 6);
					memcpy (eth_hdr->ether_shost, sender_mac, 6);
				
					RTable best_route;
					best_route = get_best_route (rtable, ip_hdr->daddr,
								r_entries, out);
						
					if (best_route == NULL) { 
						packet pack;
						pack = make_ICMP_packet(m, ICMP_DEST_UNREACH, 0);
						send_packet(m.interface, &pack);
					}
					else {

						ip_hdr->ttl--;				
						ip_hdr->check = 0;
						ip_hdr->check = ip_checksum(ip_hdr, 
									sizeof(struct iphdr));
						send_packet(best_route[0].interface, pkt);
						}	
				}
			}
		}
	}

	fclose(in);
	fclose(out);
}
