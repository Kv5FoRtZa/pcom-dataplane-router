#include "protocols.h"
#include "queue.h"
#include "include/lib.h"
#include <string.h>
#include <arpa/inet.h> /* ntoh, hton and inet_ functions */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "lib.h"
#include  <time.h>
struct TrieNode {
    struct TrieNode *children[2];
    int final;
};
struct Pachet_in_coada {
	char buf[MAX_PACKET_LEN];
    size_t len;
	uint32_t arp_ip;
	int interfata;
};

struct route_table_entry *rtable;
int rtable_len;
struct TrieNode *initializeaza(struct TrieNode *radacina){
	struct TrieNode *node;
	node = radacina;
	 for (int i = 0; i < rtable_len; i++) {
		node = radacina;
		uint32_t save = ntohl(rtable[i].prefix);
		uint32_t mask = ntohl(rtable[i].mask);
		int final = 0;
		int len = 0;
		while (mask & (1u << 31)) {
			len ++;
			mask <<= 1;
		}
		for (int i = 31; i >= 32 - len; i --) {
        	if (save &(1u << i))
			{
				if(node->children[1] != NULL){
					node = node->children[1];
				}
				else{
					node->children[1] = (struct TrieNode *)malloc(sizeof(struct TrieNode));
					node = node->children[1];
					node->children[0] = NULL;
					node->children[1] = NULL;
					node->final = -1;
				}
			}
        	else{
				if(node->children[0] != NULL){
					node = node->children[0];
				}
				else{
					node->children[0] = (struct TrieNode *)malloc(sizeof(struct TrieNode));
					node = node->children[0];
					node->children[0] = NULL;
					node->children[1] = NULL;
					node->final = -1;
				}
			}
    	}
		node->final = i;
    }
	return radacina;
}
struct route_table_entry *get_best_route(uint32_t ip_dest,struct TrieNode *radacina) {
    struct route_table_entry *best = NULL;
	struct TrieNode *node;
	node = radacina;
	ip_dest = ntohl(ip_dest);
	for (int i = 31; i >= 0; i --) {
		if (ip_dest &(1u << i))
		{
			if(node->children[1] != NULL){
				node = node->children[1];
				if(node->final != -1){
					best = &rtable[node->final];
				}
			}
			else{
				if(node->final != -1){
					best = &rtable[node->final];
				}
				break;
			}
		}
		else{
			if(node->children[0] != NULL){
				node = node->children[0];
				if(node->final != -1){
					best = &rtable[node->final];
				}
			}
			else{
				if(node->final != -1){
					best = &rtable[node->final];
				}
				break;
			}
		}
	}
    return best;
}
struct arp_table_entry *arp_table;
int arp_table_len;

struct arp_table_entry *get_arp_entry(uint32_t given_ip) {

    for (int i = 0; i < arp_table_len; i++) {
        if (arp_table[i].ip == (given_ip)) {
        	return &arp_table[i];
        }
    }

	return NULL;
}
void arp_req(int interf,uint32_t ip_cautat,uint32_t ip_interfata){
	char buf[MAX_PACKET_LEN];
	memset(buf,0,MAX_PACKET_LEN);
	//acelasi cod ca la redirectionarea de arp din main, fara printf-urile vietii
	struct ether_hdr *eth_hdr = (struct ether_hdr *) buf;
	struct arp_hdr *arp = (struct arp_hdr *)(buf + sizeof(struct ether_hdr));
	arp->hw_type = htons(1);
	arp->proto_type = htons(0x0800);
	arp->hw_len = 6;
	arp->proto_len = 4;
	arp->opcode = htons(1);
	arp->sprotoa = ip_interfata;
	arp->tprotoa = ip_cautat;
	memset(arp->thwa, 0, 6);
	uint8_t mac[6];
	get_interface_mac(interf, mac);
	uint8_t f_de_20_de_ori[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	memcpy(eth_hdr->ethr_dhost, f_de_20_de_ori, 6);
	memcpy(eth_hdr->ethr_shost, mac, 6);
	eth_hdr->ethr_type = htons(0x0806);
	memcpy(arp->shwa, mac, 6);
	send_to_link(sizeof(struct ether_hdr) + sizeof(struct arp_hdr), buf, interf);
}

int main(int argc, char *argv[])
{
	queue coada = create_queue();
	char buf[MAX_PACKET_LEN];
	size_t lungime;
	int interface;
	///arp_table_len = parse_arp_table("arp_table.txt", arp_table); pt tabel
	// Do not modify this line
	init(argv + 2, argc - 2);
	rtable = malloc(sizeof(struct route_table_entry) * 67000);
	rtable_len = read_rtable(argv[1], rtable);
	struct TrieNode *radacina = (struct TrieNode *)malloc(sizeof(struct TrieNode));
	radacina->final = -1;
	radacina->children[0] = NULL;
	radacina->children[1] = NULL;
	radacina = initializeaza(radacina);
	
	//citesc numele fisierului in make(cred)
	/* Read the static routing table and the MAC table */
	arp_table = malloc(sizeof(struct  arp_table_entry) * 100);
	arp_table_len = 0;
	while (1) {
		interface = recv_from_any_link(buf, &lungime);
		printf("We have received a packet\n");
		struct ether_hdr *eth_hdr = (struct ether_hdr *) buf;
		if(ntohs(eth_hdr->ethr_type) == 0x0806){
			printf("67\n");
			struct arp_hdr *arp = (struct arp_hdr *)(buf + sizeof(struct ether_hdr));
			if(ntohs(arp->opcode) == 1){ 
				printf("68\n");
				//if(arp->tprotoa == (get_interface_ip(interface))){
				printf("Este arp\n");
				//daca e arp si este pt mine
				arp->hw_type = htons(1);
				arp->proto_type = htons(0x0800);
				arp->hw_len = 6;
				arp->proto_len = 4;
				arp->opcode = htons(2);
				///aici sus sunt chestiile inutile care trebuie doar setate
				uint8_t mac[6], sender_mac[6];//mac e al routerului, sender e al sender
				get_interface_mac(interface, mac);
				memcpy(sender_mac, arp->shwa, 6);
				//s-au obtinut cele 2 mac
				uint32_t sender_ip = arp->sprotoa;
				uint32_t my_ip = arp->tprotoa;
				memcpy(eth_hdr->ethr_dhost, sender_mac, 6);
				memcpy(eth_hdr->ethr_shost, mac, 6);
				eth_hdr->ethr_type = htons(0x0806);
				//s-a setat ethr dupa ce am obtinu cele 2 mac
				memcpy(arp->shwa, mac, 6);
				arp->sprotoa = my_ip;;
				memcpy(arp->thwa, sender_mac, 6);
				arp->tprotoa = sender_ip;
				//apoi , setez sender si recv mac in arp + ip-ul in arp
				printf("trimit arp mai departe\n");
				//poti te rog sa imi zici ce fucking adresa nu e pusa buna????????
				//am facut 1000 de permutari de setat de adrese
				printf("reply eth %02x:%02x:%02x:%02x:%02x:%02x ",eth_hdr->ethr_shost[0], eth_hdr->ethr_shost[1], eth_hdr->ethr_shost[2],eth_hdr->ethr_shost[3], eth_hdr->ethr_shost[4], eth_hdr->ethr_shost[5]);

				printf("eth dst%02x:%02x:%02x:%02x:%02x:%02x ",eth_hdr->ethr_dhost[0], eth_hdr->ethr_dhost[1], eth_hdr->ethr_dhost[2],eth_hdr->ethr_dhost[3], eth_hdr->ethr_dhost[4], eth_hdr->ethr_dhost[5]);

				//printf("arp %s tip%s\n", inet_ntoa(a), inet_ntoa(b));
				printf("recv iface ip%s\n",get_interface_ip(interface));
				//DE CE IN LABURI ESTE INTERFACE BUF LUNGIME SI AICI E INVERS???????
				send_to_link(lungime, buf, interface);
				continue;
			}
			else if(ntohs(arp->opcode) == 2){
				struct arp_hdr *arp = (struct arp_hdr *)(buf + sizeof(struct ether_hdr));
				//aici a primit un arp reply la ce a trimis el
				uint32_t ip_rezolvat = arp->sprotoa;
				arp_table[arp_table_len].ip = arp->sprotoa;
				memcpy(arp_table[arp_table_len].mac , arp->shwa,6);
				arp_table_len ++;
				//parcurg coada, verific daca pot trimite ceva, bag restul in backup
				queue backup = create_queue();
				while(!queue_empty(coada)){
					struct Pachet_in_coada *packk = queue_deq(coada);
					if(packk->arp_ip == ip_rezolvat){
						struct arp_table_entry *entry = get_arp_entry(ip_rezolvat);
						struct ether_hdr *salvat_eth = (struct ether_hdr *) packk->buf;
						memcpy(salvat_eth->ethr_dhost, entry->mac, 6);
						get_interface_mac(packk->interfata, salvat_eth->ethr_shost);
						printf("trimit ip din coada mai departe\n");
						send_to_link(packk->len, packk->buf, packk->interfata);
					}
					else{
						queue_enq(backup,packk);
					}
				}
				coada = backup;
			}
		}
		else if (ntohs(eth_hdr->ethr_type) == 0x0800) {
			struct ip_hdr *ip = (struct ip_hdr *)(buf + sizeof(struct ether_hdr));
			printf("Este doar ip\n");
			if(checksum((uint16_t *)ip, sizeof(struct ip_hdr)) == 0){
				printf("Este ip cu checksum bun\n");
				struct route_table_entry *best_router = get_best_route(ip->dest_addr,radacina);
				if(ip->dest_addr == inet_addr(get_interface_ip(interface)))
				{
					struct icmp_hdr *icmp_initial = (struct icmp_hdr *)(buf + sizeof(struct ether_hdr) + sizeof(struct ip_hdr));
					printf("Este pentru el pachetul\n");
					//aici prieste doar echo req si face replay
					char buff[MAX_PACKET_LEN];
					memset(buff, 0, sizeof(buff));
					struct ether_hdr *eth = (struct ether_hdr *)buff;
					struct ip_hdr *ip_snd = (struct ip_hdr *)(buff + sizeof(struct ether_hdr));
					struct icmp_hdr *icmp = (struct icmp_hdr *)(buff + sizeof(struct ether_hdr) + sizeof(struct ip_hdr));
					char *restul = (buff + sizeof(struct ether_hdr) + sizeof(struct ip_hdr) + sizeof(struct icmp_hdr));
					uint32_t lungime_restul = htons(ip->tot_len) - sizeof(struct ip_hdr) - sizeof(struct icmp_hdr);
					//apoi trebuie facuta setarea de eth
					struct route_table_entry *best_route_inapoi = get_best_route(ip->source_addr,radacina);
					if (best_route_inapoi == NULL) {
						continue;
					}
					uint32_t arp_ip;
					if (best_route_inapoi->next_hop != 0) {
						arp_ip = best_route_inapoi->next_hop;
					} 
					else 
					{
						arp_ip = ip->source_addr;
					}
					struct arp_table_entry *entry = get_arp_entry(arp_ip);
					if (entry == NULL) {
						//aici e cazul cand arp nu e gasit
						struct Pachet_in_coada *pack = malloc(sizeof(struct Pachet_in_coada));
						pack->len = lungime;
						pack->arp_ip = arp_ip;
						pack->interfata = best_route_inapoi->interface;
						memcpy(pack->buf,buff,lungime);
						queue_enq(coada,pack);
						arp_req(best_route_inapoi->interface ,arp_ip,inet_addr(get_interface_ip(best_route_inapoi->interface)));
						continue;
					}
					memcpy(eth->ethr_dhost, entry->mac, 6);
					get_interface_mac(best_route_inapoi->interface, eth->ethr_shost);
					eth->ethr_type = htons(0x0800);// seteaza campurile din eth ca mai sus
					//IP
					ip_snd->proto = 1;
					ip_snd->ihl = 5;
					ip_snd->ver = 4;
					ip_snd->tos = ip->tos;
					ip_snd->tot_len =  htons(sizeof(struct icmp_hdr) +  sizeof(struct ip_hdr) + lungime_restul);
					ip_snd->id = htons(0);
					ip_snd->frag = 0;
					ip_snd->ttl = 64;
					ip_snd->dest_addr = ip->source_addr;
					ip_snd->source_addr = ip->dest_addr;
					ip_snd->checksum = 0;
					ip_snd->checksum = htons(checksum((uint16_t *)ip_snd, sizeof(struct ip_hdr)));
					//ICMP
					icmp->mtype = 0;
					icmp->mcode = 0;
					icmp->check = 0;
					icmp->un_t.echo_t.id  = icmp_initial->un_t.echo_t.id;
					icmp->un_t.echo_t.seq = icmp_initial->un_t.echo_t.seq;
					memcpy(buff + sizeof(struct ether_hdr) + sizeof(struct icmp_hdr) + sizeof(struct ip_hdr), buf + sizeof(struct ether_hdr) + sizeof(struct ip_hdr) + sizeof(struct icmp_hdr) ,  lungime_restul);
					
					icmp->check = htons(checksum((uint16_t *)icmp, sizeof(struct icmp_hdr) + lungime_restul));
					send_to_link(sizeof(struct ether_hdr) + sizeof(struct icmp_hdr) + sizeof(struct ip_hdr) + lungime_restul, buff,best_route_inapoi->interface);
					continue;
				}
				if(best_router == NULL)
				{
					char buff[MAX_PACKET_LEN];
					memset(buff, 0, sizeof(buff));
					struct ether_hdr *eth = (struct ether_hdr *)buff;
					struct ip_hdr *ip_snd = (struct ip_hdr *)(buff + sizeof(struct ether_hdr));
					struct icmp_hdr *icmp = (struct icmp_hdr *)(buff + sizeof(struct ether_hdr) + sizeof(struct ip_hdr));
					//apoi trebuie facuta setarea de eth
					struct route_table_entry *best_route_inapoi = get_best_route(ip->source_addr,radacina);
					if (best_route_inapoi == NULL) {
						continue;
					}
					uint32_t arp_ip;
					if (best_route_inapoi->next_hop != 0) {
						arp_ip = best_route_inapoi->next_hop;
					} 
					else 
					{
						arp_ip = ip->source_addr;
					}
					struct arp_table_entry *entry = get_arp_entry(arp_ip);
					if (entry == NULL) {
						//aici e cazul cand arp nu e gasit
						struct Pachet_in_coada *pack = malloc(sizeof(struct Pachet_in_coada));
						pack->len = lungime;
						pack->arp_ip = arp_ip;
						pack->interfata = best_route_inapoi->interface;
						memcpy(pack->buf,buff,lungime);
						queue_enq(coada,pack);
						arp_req(best_route_inapoi->interface ,arp_ip,inet_addr(get_interface_ip(best_route_inapoi->interface)));
						continue;
					}
					memcpy(eth->ethr_dhost, entry->mac, 6);
					get_interface_mac(best_route_inapoi->interface, eth->ethr_shost);
					eth->ethr_type = htons(0x0800);// seteaza campurile din eth ca mai sus
					//IP
					ip_snd->proto = 1;
					ip_snd->ihl = 5;
					ip_snd->ver = 4;
					ip_snd->tos = ip->tos;
					ip_snd->tot_len =  htons(sizeof(struct icmp_hdr) + 2 * sizeof(struct ip_hdr) + 8);
					ip_snd->id = htons(0);
					ip_snd->frag = 0;
					ip_snd->ttl = 64;
					ip_snd->dest_addr = ip->source_addr;
					ip_snd->source_addr = inet_addr(get_interface_ip(best_route_inapoi->interface));
					ip_snd->checksum = 0;
					ip_snd->checksum = htons(checksum((uint16_t *)ip_snd, sizeof(struct ip_hdr)));
					//ICMP
					icmp->mtype = 3;
					icmp->mcode = 0;
					icmp->check = 0;
					icmp->un_t.echo_t.id  = 0;
					memcpy(buff + sizeof(struct ether_hdr) + sizeof(struct icmp_hdr) + sizeof(struct ip_hdr), buf + sizeof(struct ether_hdr), (sizeof(struct ip_hdr) + 8));
					icmp->un_t.echo_t.seq = 0;
					icmp->check = htons(checksum((uint16_t *)icmp, sizeof(struct icmp_hdr) + sizeof(struct ip_hdr) + 8));
					send_to_link(sizeof(struct ether_hdr) + sizeof(struct icmp_hdr) + 2 * sizeof(struct ip_hdr) + 8, buff,best_route_inapoi->interface);
					continue;
				}
				else{
					if (ip->ttl <= 1) {
						char buff[MAX_PACKET_LEN];
						memset(buff, 0, sizeof(buff));
						struct ether_hdr *eth = (struct ether_hdr *)buff;
						struct ip_hdr *ip_snd = (struct ip_hdr *)(buff + sizeof(struct ether_hdr));
						struct icmp_hdr *icmp = (struct icmp_hdr *)(buff + sizeof(struct ether_hdr) + sizeof(struct ip_hdr));
						//apoi trebuie facuta setarea de eth
						struct route_table_entry *best_route_inapoi = get_best_route(ip->source_addr,radacina);
						if (best_route_inapoi == NULL) {
							//oare trebuie si aici alt icmp???
							continue;
						}
						uint32_t arp_ip;
						if (best_route_inapoi->next_hop != 0) {
							arp_ip = best_route_inapoi->next_hop;
						} 
						else 
						{
							arp_ip = ip->source_addr;
						}
						struct arp_table_entry *entry = get_arp_entry(arp_ip);
						if (entry == NULL) {
							//aici e cazul cand arp nu e gasit
							struct Pachet_in_coada *pack = malloc(sizeof(struct Pachet_in_coada));
							pack->len = lungime;
							pack->arp_ip = arp_ip;
							pack->interfata = best_route_inapoi->interface;
							memcpy(pack->buf,buff,lungime);
							queue_enq(coada,pack);
							arp_req(best_route_inapoi->interface ,arp_ip,inet_addr(get_interface_ip(best_route_inapoi->interface)));
							continue;
						}
						memcpy(eth->ethr_dhost, entry->mac, 6);
						get_interface_mac(best_route_inapoi->interface, eth->ethr_shost);
						eth->ethr_type = htons(0x0800);// seteaza campurile din eth ca mai sus
						//IP
						ip_snd->proto = 1;
						ip_snd->ihl = 5;
						ip_snd->ver = 4;
						ip_snd->tos = ip->tos;
						ip_snd->tot_len =  htons(sizeof(struct icmp_hdr) + 2 * sizeof(struct ip_hdr) + 8);
						ip_snd->id = htons(0);
						ip_snd->frag = 0;
						ip_snd->ttl = 64;
						ip_snd->dest_addr = ip->source_addr;
						ip_snd->source_addr = inet_addr(get_interface_ip(best_route_inapoi->interface));
						ip_snd->checksum = 0;
						ip_snd->checksum = htons(checksum((uint16_t *)ip_snd, sizeof(struct ip_hdr)));
						//ICMP
						icmp->mtype = 11;
						icmp->mcode = 0;
						icmp->check = 0;
						icmp->un_t.echo_t.id  = 0;
						memcpy(buff + sizeof(struct ether_hdr) + sizeof(struct icmp_hdr) + sizeof(struct ip_hdr), buf + sizeof(struct ether_hdr), (sizeof(struct ip_hdr) + 8));
						icmp->un_t.echo_t.seq = 0;
						icmp->check = htons(checksum((uint16_t *)icmp, sizeof(struct icmp_hdr) + sizeof(struct ip_hdr) + 8));
						send_to_link(sizeof(struct ether_hdr) + sizeof(struct icmp_hdr) + 2 * sizeof(struct ip_hdr) + 8, buff,best_route_inapoi->interface);
						continue;
					}
					ip->ttl --;
					if(1){
						uint32_t c = ntohs(ip->checksum);
						c += 0x0100;
						c = (c & 0xFFFF) + (c >> 16);
						ip->checksum = htons((uint16_t)c);
						printf("checksum1 = 0x%04x\n", ntohs(ip->checksum));
						printf("checksum2 = %u\n",
						checksum((uint16_t *)ip, sizeof(struct ip_hdr)));
						uint32_t arp_ip;
						if (best_router->next_hop != 0) {
							arp_ip = best_router->next_hop;
						} 
						else 
						{
							arp_ip = ip->dest_addr;
						}
						struct arp_table_entry *entry = get_arp_entry(arp_ip);
						if (entry == NULL) {
							//aici e cazul cand arp nu e gasit
							struct Pachet_in_coada *pack = malloc(sizeof(struct Pachet_in_coada));
							pack->len = lungime;
							pack->arp_ip = arp_ip;
							pack->interfata = best_router->interface;
							memcpy(pack->buf,buf,lungime);
							queue_enq(coada,pack);
							arp_req(best_router->interface ,arp_ip,inet_addr(get_interface_ip(best_router->interface)));
							continue;
						}
						memcpy(eth_hdr->ethr_dhost, entry->mac, 6);
						get_interface_mac(best_router->interface, eth_hdr->ethr_shost);
						printf("trimit ip mai departe\n");
						// nu am stat 2 zile sa vad ca send_to_link este invers ca in laburi
						send_to_link(lungime, buf, best_router->interface);
					}
				}
			}
		}
	}
}

