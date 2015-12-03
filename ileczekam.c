#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "err.h"
#include "portable_endian.h"
#include "time_measure.h"

#define BUFFER_SIZE 16
#define RCV_LEN     16
#define SEND_TO_LEN 8

// How many times should we repeat time measurement for both protcols
#define UDP_REPEAT  10
#define TCP_REPEAT  10

int main(int argc, char *argv[])
{
	int is_udp = 0;
	int sock;
	struct addrinfo addr_hints;
	struct addrinfo *addr_result;

	int i;
	int flags, sflags;
	char buffer[BUFFER_SIZE];
	size_t len;
	ssize_t snd_len, rcv_len;
	struct sockaddr_in my_address;
	struct sockaddr_in srvr_address;
	socklen_t rcva_len;

	uint64_t current_time;
	uint64_t micros1, micros2; // Time received from the server (in us)

	if (argc < 3 || argc > 0) {
		if (strcmp(argv[1], "-t") == 0) {
			is_udp = 0;
		} else if (strcmp(argv[1], "-u") == 0) {
			is_udp = 1;
		} else {
			fatal("Usage: %s -t host port\n", argv[0]);
		}
	}

	// Convert host/port in string to struct addrinfo
	(void)memset(&addr_hints, 0, sizeof(struct addrinfo));
	addr_hints.ai_family = AF_INET; // IPv4

	// Choose socktype depending on the program option
	addr_hints.ai_socktype = is_udp ? SOCK_DGRAM : SOCK_STREAM;
	addr_hints.ai_protocol = is_udp ? IPPROTO_UDP : IPPROTO_TCP;
	addr_hints.ai_flags = 0;
	addr_hints.ai_addrlen = 0;
	addr_hints.ai_addr = NULL;
	addr_hints.ai_canonname = NULL;
	addr_hints.ai_next = NULL;

	if (getaddrinfo(argv[2], argv[3], &addr_hints, &addr_result) != 0) {
		syserr("getaddrinfo");
	}

	my_address.sin_family = AF_INET; // IPv4
	my_address.sin_addr.s_addr =
	    ((struct sockaddr_in *)(addr_result->ai_addr))
		->sin_addr.s_addr; // address IP
	my_address.sin_port =
	    htons((uint16_t)atoi(argv[3])); // port from the command line

	sock =
	    socket(PF_INET, addr_result->ai_socktype, addr_result->ai_protocol);

	if (sock < 0)
		syserr("socket");
	if (is_udp) { // UDP
		for (i = 0; i < UDP_REPEAT; i++) {
			current_time = get_nanos();
			convert_to_micros(&current_time);
			current_time = htobe64(current_time);
			len = SEND_TO_LEN;

			memcpy(&buffer, &current_time, len);
			sflags = 0;
			rcva_len = (socklen_t)sizeof(my_address);
			snd_len =
			    sendto(sock, buffer, len, sflags,
				   (struct sockaddr *)&my_address, rcva_len);
			if (snd_len != (ssize_t)len) {
				syserr("partial / failed write");
			}

			(void)memset(buffer, 0, sizeof(buffer));
			flags = 0;
			len = (size_t)sizeof(buffer) - 1;
			rcva_len = (socklen_t)sizeof(srvr_address);

			rcv_len = recvfrom(sock, buffer, RCV_LEN, flags,
					   (struct sockaddr *)&srvr_address,
					   &rcva_len);

			if (rcv_len != RCV_LEN) {
				syserr("read");
			}

			memcpy(&micros1, &buffer, SEND_TO_LEN);
			memcpy(&micros2, &(*(buffer + SEND_TO_LEN)),
			       SEND_TO_LEN);

			// Convert to big endian
			current_time = be64toh(current_time);
			micros1 = be64toh(micros1);
			micros2 = be64toh(micros2);
			if (current_time == micros1) {
				current_time = get_nanos();
				convert_to_micros(&current_time);
				(void)printf(
				    "Received response:[%ld][%ld],response "
				    "time:%ld us.\n",
				    micros1, micros2, current_time - micros1);
			}
		}
	} else { // TCP
		printf("Measuring TCP connection time\n");
		current_time = get_nanos();

		if (connect(sock, addr_result->ai_addr,
			    addr_result->ai_addrlen) < 0)
			syserr("connect");
		printf("After:%ld\n", get_nanos());
		current_time = get_nanos() - current_time;
		convert_to_micros(&current_time);
		printf("Established connection in %ld us.\n", current_time);
	}
	freeaddrinfo(addr_result);
	if (close(sock) == -1) {
		syserr("close");
	}

	return 0;
}
