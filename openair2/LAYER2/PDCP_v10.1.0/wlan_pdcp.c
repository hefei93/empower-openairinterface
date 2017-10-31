/* Copyright (c) 2016 Supreeth Herle <s.herle@create-net.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * WLAN PDCP.
 */

#include "wlan_pdcp.h"

#include "platform_constants.h"
#include "emoai_common.h"

#include "gtpv1u_eNB_task.h"
#include "gtpv1u.h"

#include "intertask_interface.h"

#include <emage/pb/configs.pb-c.h>

// Of the router
#define WLAN_DEST_MAC0	0x00
#define WLAN_DEST_MAC1	0x0D
#define WLAN_DEST_MAC2	0xB9
#define WLAN_DEST_MAC3	0x14
#define WLAN_DEST_MAC4	0xAE
#define WLAN_DEST_MAC5	0x50

// Actual one
// #define WLAN_DEST_MAC0	0x00
// #define WLAN_DEST_MAC1	0x24
// #define WLAN_DEST_MAC2	0xd7
// #define WLAN_DEST_MAC3	0x35
// #define WLAN_DEST_MAC4	0x06
// #define WLAN_DEST_MAC5	0x18

#define WLAN_SRC_MAC0	0x34
#define WLAN_SRC_MAC1	0xe6
#define WLAN_SRC_MAC2	0xd7
#define WLAN_SRC_MAC3	0x3f
#define WLAN_SRC_MAC4	0x06
#define WLAN_SRC_MAC5	0x3f

#define ENB_TO_WLAN_IF	"eth0"

struct sockaddr_ll wlan_pdcp_socket_address;
struct ifreq if_idx;
struct ifreq if_mac;
int wlan_pdcp_sockfd;

pcap_t *snif_handle;
pcap_t* pcap;

int snif_sock_raw, snif_saddr_size;
struct sockaddr snif_saddr;

int wlan_pdcp_init(void) {


////////////////////////////////////////////////////////////////////////////////

	char ifName[IFNAMSIZ];
	strcpy(ifName, ENB_TO_WLAN_IF);

	/* Open RAW socket to send on */
	if ((wlan_pdcp_sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(wlan_pdcp_sockfd, SIOCGIFINDEX, &if_idx) < 0)
		perror("SIOCGIFINDEX");
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(wlan_pdcp_sockfd, SIOCGIFHWADDR, &if_mac) < 0)
		perror("SIOCGIFHWADDR");

	/* Index of the network device */
	wlan_pdcp_socket_address.sll_ifindex = if_idx.ifr_ifindex;
	 // Address length
	wlan_pdcp_socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	wlan_pdcp_socket_address.sll_addr[0] = WLAN_DEST_MAC0;
	wlan_pdcp_socket_address.sll_addr[1] = WLAN_DEST_MAC1;
	wlan_pdcp_socket_address.sll_addr[2] = WLAN_DEST_MAC2;
	wlan_pdcp_socket_address.sll_addr[3] = WLAN_DEST_MAC3;
	wlan_pdcp_socket_address.sll_addr[4] = WLAN_DEST_MAC4;
	wlan_pdcp_socket_address.sll_addr[5] = WLAN_DEST_MAC5;

////////////////////////////////////////////////////////////////////////////////

	// /* Open RAW socket to send on */
	// if ((wlan_pdcp_sockfd = socket (AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
	// 	perror("socket");
	// 	exit(0);
	// }

	// int one = 0;
	// const int *val = &one;
	// if (setsockopt (wlan_pdcp_sockfd, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0) {
	//     perror ("Error setting IP_HDRINCL. Error number");
	// }

	// /* Get the index of the interface to send on */
	// memset(&if_idx, 0, sizeof(struct ifreq));
	// strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	// if (ioctl(wlan_pdcp_sockfd, SIOCGIFINDEX, &if_idx) < 0) {
	// 	perror("SIOCGIFINDEX");
	// 	exit(0);
	// }
	// /* Get the MAC address of the interface to send on */
	// memset(&if_mac, 0, sizeof(struct ifreq));
	// strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	// if (ioctl(wlan_pdcp_sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
	// 	perror("SIOCGIFHWADDR");
	// 	exit(0);
	// }

	// /* Index of the network device */
	// wlan_pdcp_socket_address.sll_ifindex = if_idx.ifr_ifindex;
	// /* Address length*/
	// wlan_pdcp_socket_address.sll_halen = ETH_ALEN;
	// /* Destination MAC */
	// wlan_pdcp_socket_address.sll_addr[0] = WLAN_DEST_MAC0;
	// wlan_pdcp_socket_address.sll_addr[1] = WLAN_DEST_MAC1;
	// wlan_pdcp_socket_address.sll_addr[2] = WLAN_DEST_MAC2;
	// wlan_pdcp_socket_address.sll_addr[3] = WLAN_DEST_MAC3;
	// wlan_pdcp_socket_address.sll_addr[4] = WLAN_DEST_MAC4;
	// wlan_pdcp_socket_address.sll_addr[5] = WLAN_DEST_MAC5;

	// char pcap_errbuf[PCAP_ERRBUF_SIZE];
	// pcap_errbuf[0] = '\0';
	// pcap = pcap_open_live(ENB_TO_WLAN_IF, 65535, 0, 0, pcap_errbuf);
	// if (!pcap) {
	//     printf("Couldn't sending open device: %s\n", pcap_errbuf);
	// }

	// pcap_setnonblock(pcap, 1, pcap_errbuf);

	printf("Starting...\n");

	if(emoai_create_new_thread(
		wlan_pdcp_recv,
		NULL) != 0) {
		printf("Failed to create wlan pdcp thread");
	}

	return 0;
}

void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer)
{
	int buf_size = header->len;
	uint8_t *gtpu_buffer_p = NULL;
	MessageDef *message_p = NULL;

	if (emoai_get_ue_crnti(0) == NOT_A_RNTI) {
		return;
	}

	if (emoai_get_ue_state(0) < RRC_STATE__RS_RRC_RECONFIGURED)
		return;

	// struct iphdr *iph = (struct iphdr *)(&buffer[sizeof(struct ethhdr)]);
	// inet_aton("10.42.0.88", &iph->saddr);

	gtpu_buffer_p = itti_malloc(TASK_PDCP_ENB, TASK_GTPV1_U,
										buf_size - sizeof(struct ethhdr) + GTPU_HEADER_OVERHEAD_MAX);
	memcpy(&gtpu_buffer_p[GTPU_HEADER_OVERHEAD_MAX], &buffer[sizeof(struct ethhdr)], buf_size - sizeof(struct ethhdr));
	message_p = itti_alloc_new_message(TASK_PDCP_ENB, GTPV1U_ENB_TUNNEL_DATA_REQ);
	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).buffer       = gtpu_buffer_p;
	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).length       = buf_size - sizeof(struct ethhdr);
	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).offset       = GTPU_HEADER_OVERHEAD_MAX;
	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).rnti         = emoai_get_ue_crnti(0);
	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).rab_id       = 1 + 4;
	itti_send_msg_to_task(TASK_GTPV1_U, INSTANCE_DEFAULT, message_p);

}

int wlan_pdcp_send(int buf_size, unsigned char * sdu_buffer) {

	if (buf_size > 1500) {
		printf("\n Huge packet \n");
	}
	struct iphdr *iph = (struct iphdr *)(sdu_buffer);
	struct sockaddr_in dest;

	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = iph->daddr;

	int err, tx_len = 0;
	char sendbuf[buf_size + sizeof(struct ether_header)];
	struct ether_header *eh = (struct ether_header *) sendbuf;

	/* Construct the Ethernet header */
	memset(sendbuf, 0, buf_size + sizeof(struct ether_header));
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	// eh->ether_shost[0] = WLAN_SRC_MAC0;
	// eh->ether_shost[1] = WLAN_SRC_MAC1;
	// eh->ether_shost[2] = WLAN_SRC_MAC2;
	// eh->ether_shost[3] = WLAN_SRC_MAC3;
	// eh->ether_shost[4] = WLAN_SRC_MAC4;
	// eh->ether_shost[5] = WLAN_SRC_MAC5;
	eh->ether_dhost[0] = WLAN_DEST_MAC0;
	eh->ether_dhost[1] = WLAN_DEST_MAC1;
	eh->ether_dhost[2] = WLAN_DEST_MAC2;
	eh->ether_dhost[3] = WLAN_DEST_MAC3;
	eh->ether_dhost[4] = WLAN_DEST_MAC4;
	eh->ether_dhost[5] = WLAN_DEST_MAC5;
	/* Ethertype field */
	eh->ether_type = htons(ETH_P_IP);
	tx_len += sizeof(struct ether_header);

	/* Packet data */
	memcpy(sendbuf + tx_len, sdu_buffer, buf_size);
	tx_len += buf_size;

	// /* Send packet */
	// if (sendto(wlan_pdcp_sockfd,
	// 			sendbuf, tx_len, 0,
	// 			(struct sockaddr*)&wlan_pdcp_socket_address,
	// 			sizeof(struct sockaddr_ll)) < 0)
	// 	printf("Send failed\n");

	// printf("\n Before \n");
	// print_dl_ip_header(sdu_buffer, buf_size);

	// struct iphdr *iph = (struct iphdr *)(sdu_buffer);
	// inet_aton("10.20.10.88", &iph->daddr);

	// printf("\n After \n");
	// print_dl_ip_header(sdu_buffer, buf_size);

	// for (int z=0; z < buf_size; z++) {
	// 	if (z != 0 && z % 16 == 0) {
	// 		printf("\n");
	// 	}
	// 	printf("%02x ", (unsigned char) sdu_buffer[z]);
	// }
	// printf("\n");

	// err = sendto(wlan_pdcp_sockfd,
	// 			sdu_buffer, buf_size, 0,(struct sockaddr*)&wlan_pdcp_socket_address,
	// 			sizeof(struct sockaddr_ll));

	err = sendto(wlan_pdcp_sockfd,
				sendbuf, tx_len, 0,(struct sockaddr*)&wlan_pdcp_socket_address,
				sizeof(struct sockaddr_ll));

	if (err < 0) {
		printf("Send failed %d\n", err);
		perror("sendto");
	}

	// pcap_inject(pcap, sendbuf, tx_len);

	// if (strcmp(inet_ntoa(dest.sin_addr), "192.178.2.2") == 0)
	// 	return -1;

	// return 0;
	return -1;
}

int wlan_pdcp_recv(void) {

	// while (1) {
	// 	int buf_size = 0;
	// 	unsigned char *buffer = calloc(MAX_IP_PACKET_SIZE, sizeof(unsigned char));
	// 	socklen_t addr_len = sizeof(struct sockaddr_ll);
	// 	uint8_t *gtpu_buffer_p = NULL;
	// 	MessageDef *message_p = NULL;

 //        snif_saddr_size = sizeof(snif_saddr);
 //        //Receive a packet
 //        buf_size = recvfrom(snif_sock_raw, buffer, MAX_IP_PACKET_SIZE, 0, &snif_saddr, (socklen_t *) &snif_saddr_size);
 //        if(buf_size < 0) {
 //            printf("Recvfrom error , failed to get packets\n");
 //            return 1;
 //        }
	// 	// if ((buf_size = recvfrom(wlan_pdcp_sockfd, buffer, MAX_IP_PACKET_SIZE, 0,
	// 	// 		  (struct sockaddr *)&wlan_pdcp_socket_address, &addr_len)) < 0) {
	// 	// 	printf("Recvfrom failed\n");
	// 	// 	return -1;
	// 	// } else if (buf_size == 0) {
	// 	// 	printf("Recvfrom returned 0\n");
	// 	// }
	// 	// else if (wlan_pdcp_socket_address.sll_pkttype != PACKET_HOST) {
	// 	// 	free(buffer);
	// 	// }

	// 	// wlan_pdcp_send(buf_size, buffer);
	// 	// free(buffer);

	// 	if (emoai_get_ue_crnti(0) == NOT_A_RNTI) {
	// 		continue;
	// 	}

	// 	if (emoai_get_ue_state(0) < RRC_STATE__RS_RRC_RECONFIGURED)
	// 		continue;

	// 	if (print_ip_header(buffer, buf_size) < 0)
	// 		continue;

	// 	// printf("\n\nNumber of bytes read from eth0 %d\n\n", buf_size);

	// 	gtpu_buffer_p = itti_malloc(TASK_PDCP_ENB, TASK_GTPV1_U,
	// 										buf_size - sizeof(struct ethhdr) + GTPU_HEADER_OVERHEAD_MAX);
	// 	memcpy(&gtpu_buffer_p[GTPU_HEADER_OVERHEAD_MAX], &buffer[sizeof(struct ethhdr)], buf_size - sizeof(struct ethhdr));
	// 	message_p = itti_alloc_new_message(TASK_PDCP_ENB, GTPV1U_ENB_TUNNEL_DATA_REQ);
	// 	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).buffer       = gtpu_buffer_p;
	// 	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).length       = buf_size - sizeof(struct ethhdr);
	// 	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).offset       = GTPU_HEADER_OVERHEAD_MAX;
	// 	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).rnti         = emoai_get_ue_crnti(0);
	// 	GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).rab_id       = 1 + 4;
	// 	itti_send_msg_to_task(TASK_GTPV1_U, INSTANCE_DEFAULT, message_p);
	// }

	// close(snif_sock_raw);

	char errbuf[100];
	snif_handle = pcap_open_live(ENB_TO_WLAN_IF, 65535, 0, 0, errbuf);
	bpf_u_int32 mask;
	bpf_u_int32 net;

	if (pcap_lookupnet(ENB_TO_WLAN_IF, &net, &mask, errbuf) == -1) {
		printf("Can't get netmask for device\n");
		net = 0;
		mask = 0;
	}

	if (snif_handle == NULL) {
		printf("Couldn't open device: %s\n", errbuf);
	}

	pcap_setdirection(snif_handle, PCAP_D_IN);
	pcap_setnonblock(snif_handle, 1, errbuf);

	struct bpf_program fp;
	char filter_exp[] = "not ip dst host 10.42.0.253 and ip src host 192.178.2.2 and not udp port 2152";
	// char filter_exp[] = "not ip dst host 10.42.0.254 and not ip dst host 10.42.0.253 and not ip dst host 10.42.0.252 and ip src host 10.42.0.88 and not udp port 2152";
	// char filter_exp[] = "not ip host sup and ip src host 10.20.10.88 and not udp port 2152";
	// char filter_exp[] = "ip src host 10.42.0.88 and ether src 34.E6.D7.20.65.90 and not udp port 2152";
	// char filter_exp[] = "ether src 34:E6:D7:20:65:90";

	if (pcap_compile(snif_handle, &fp, filter_exp, 0, net) == -1) {
		printf("Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(snif_handle));
	}
	if (pcap_setfilter(snif_handle, &fp) == -1) {
		printf("Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(snif_handle));
	}

	pcap_loop(snif_handle, -1, process_packet, NULL);

	pcap_close(snif_handle);

	return 0;
}

int print_ip_header(unsigned char* Buffer, int Size)
{
	unsigned short iphdrlen;

	struct sockaddr_in source,dest;

	struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr));
	iphdrlen = iph->ihl * 4;

	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = iph->saddr;

	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = iph->daddr;

	// fprintf(logfile , "\n");
	// fprintf(logfile , "IP Header\n");
	// fprintf(logfile , "   |-IP Version        : %d\n",(unsigned int)iph->version);
	// fprintf(logfile , "   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
	// fprintf(logfile , "   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
	// fprintf(logfile , "   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
	// fprintf(logfile , "   |-Identification    : %d\n",ntohs(iph->id));
	// //fprintf(logfile , "   |-Reserved ZERO Field   : %d\n",(unsigned int)iphdr->ip_reserved_zero);
	// //fprintf(logfile , "   |-Dont Fragment Field   : %d\n",(unsigned int)iphdr->ip_dont_fragment);
	// //fprintf(logfile , "   |-More Fragment Field   : %d\n",(unsigned int)iphdr->ip_more_fragment);
	// fprintf(logfile , "   |-TTL      : %d\n",(unsigned int)iph->ttl);
	// fprintf(logfile , "   |-Protocol : %d\n",(unsigned int)iph->protocol);
	// fprintf(logfile , "   |-Checksum : %d\n",ntohs(iph->check));
	// printf("   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
	// printf("   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
	if (strcmp(inet_ntoa(source.sin_addr), "10.42.0.88") != 0)
		return -1;
	// if (strcmp(inet_ntoa(dest.sin_addr), "10.20.10.3") != 0)
	// 	return -1;
	// fprintf(logfile , "   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
	// fprintf(logfile , "   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
	return 0;
}


int print_dl_ip_header(unsigned char* Buffer, int Size)
{
	unsigned short iphdrlen;

	struct sockaddr_in source,dest;

	struct iphdr *iph = (struct iphdr *)(Buffer);
	iphdrlen = iph->ihl * 4;

	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = iph->saddr;

	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = iph->daddr;

	// fprintf(logfile , "\n");
	// fprintf(logfile , "IP Header\n");
	// fprintf(logfile , "   |-IP Version        : %d\n",(unsigned int)iph->version);
	// fprintf(logfile , "   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
	// fprintf(logfile , "   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
	// fprintf(logfile , "   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
	// fprintf(logfile , "   |-Identification    : %d\n",ntohs(iph->id));
	// //fprintf(logfile , "   |-Reserved ZERO Field   : %d\n",(unsigned int)iphdr->ip_reserved_zero);
	// //fprintf(logfile , "   |-Dont Fragment Field   : %d\n",(unsigned int)iphdr->ip_dont_fragment);
	// //fprintf(logfile , "   |-More Fragment Field   : %d\n",(unsigned int)iphdr->ip_more_fragment);
	// fprintf(logfile , "   |-TTL      : %d\n",(unsigned int)iph->ttl);
	// fprintf(logfile , "   |-Protocol : %d\n",(unsigned int)iph->protocol);
	// fprintf(logfile , "   |-Checksum : %d\n",ntohs(iph->check));
	printf("   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
	printf("   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
	// if (strcmp(inet_ntoa(dest.sin_addr), "10.20.10.3") != 0)
	// 	return -1;
	// fprintf(logfile , "   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
	// fprintf(logfile , "   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
	return 0;
}

int check_ul_pkt_ip(unsigned char* Buffer, int Size)
{
	struct sockaddr_in source;

	struct iphdr *iph = (struct iphdr *)(Buffer);

	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = iph->saddr;

	if (strcmp(inet_ntoa(source.sin_addr), "192.178.2.2") == 0)
		return -1;

	return 0;
}

