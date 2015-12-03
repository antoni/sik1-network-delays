#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "err.h"
#include "portable_endian.h"
#include "time_measure.h"

#define BUFFER_SIZE 16
#define RCV_LEN 8      // Length of received data
#define SEND_TO_LEN 16 // Length of sent-back data

#define UDP_REPEAT 10  // How many times should the measurements be repeated

typedef unsigned char BYTE;

int main(int argc, char *argv[])
{
	int repeat = UDP_REPEAT;
	uint64_t micros1, micros2;
	int port_number = atoi(argv[1]);

	int sock;
	int flags, sflags;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;

	BYTE buffer[BUFFER_SIZE];
	socklen_t snda_len, rcva_len;
	ssize_t len, snd_len;

	if (argc != 2) {
		fatal("Usage: %s port\n", argv[0]);
	}

	sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
	if (sock < 0)
		syserr("socket");

	server_address.sin_family = AF_INET; // IPv4
	server_address.sin_addr.s_addr =
	    htonl(INADDR_ANY); // listen on all interfaces
	server_address.sin_port = htons(port_number);

	// bind the socket to a concrete address
	if (bind(sock, (struct sockaddr *)&server_address,
		 (socklen_t)sizeof(server_address)) < 0)
		syserr("bind");

	(void)printf("Waiting for incoming packages at port %d\n", port_number);
	snda_len = (socklen_t)sizeof(client_address);
	do {
		rcva_len = (socklen_t)sizeof(client_address);
		flags = 0; // we do not request anything special
		len = recvfrom(sock, buffer, sizeof(buffer), flags,
			       (struct sockaddr *)&client_address, &rcva_len);
		if (len < 0)
			syserr("error on datagram from client socket");
		else {

			memcpy(&micros1, &buffer, sizeof(uint64_t));
			micros1 = be64toh(micros1);
			micros2 = get_nanos();
			convert_to_micros(&micros2);

			(void)printf("Time signature received:[%ld]", micros1);

			sflags = 0;

			// Send back 2 numbers

			// Convert to big endian
			micros1 = htobe64(micros1);
			micros2 = htobe64(micros2);

			// Copy to buffer
			memcpy(&buffer, &micros1, sizeof(uint64_t));
			memcpy(&(*(buffer + sizeof(uint64_t))), &micros2,
			       sizeof(uint64_t));

			snd_len = sendto(
			    sock, buffer, (size_t)SEND_TO_LEN, sflags,
			    (struct sockaddr *)&client_address, snda_len);
			(void)printf(" --> response [%ld,%ld] sent\n",
				     be64toh(micros1), be64toh(micros2));
			if (snd_len != SEND_TO_LEN)
				syserr("error on sending datagram to client "
				       "socket");
		}
		repeat--;
	} while (repeat);

	if (close(sock) == -1) {
		syserr("close");
	}

	return 0;
}
