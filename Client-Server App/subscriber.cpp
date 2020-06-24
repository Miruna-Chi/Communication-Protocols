#include <bits/stdc++.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include "message.h"

using namespace std;

#define BUFLEN 2000

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <client_ID> <server_IP> <server_port>\n", file);
	exit(0);
}

void sub_usage() {
    fprintf(stdout, "\nUsage:\n"
        "\tsubscribe topic SF\n"
        "or:\n"
        "\tunsubscribe topic\n"
        "Try again :)\n\n");
}

bool verify_command(char *buf) {
	char buffer[BUFLEN];
	memcpy(buffer, buf, BUFLEN);

	char *token = strtok(buffer, " ");
	char *command = token;

	if (!strcmp(token, "subscribe") && !strcmp(token, "unsubscribe")) {
		sub_usage();

		return false;
	}

	token = strtok (NULL, " \n");
	char *topic = token;

	if (token == NULL) {
		fprintf (stdout, "\nToo few parameters.\n\n");
		sub_usage();
		return false;
	}

	if (strlen(token) > 50) {
		fprintf(stdout, "\nThe topic should not exceed 50 characters.\n\n");
		return false;
	}

	token = strtok (NULL, " \n");

	int SF = 0;
	
	if (token == NULL && strcmp(command, "unsubscribe")) {
		fprintf (stdout, "\nToo few parameters for subscribe.\n\n");
		sub_usage();
		return false;
	}
	else if (token != NULL) {
		if (strlen(token) > 1 || ((atoi(token) != 1 && atoi(token) != 0))) {

			fprintf(stdout, "\nThe last parameter has to be 0 or 1\n\n");
			return false;
		}

		SF = atoi(token);
		token = strtok (NULL, " \n");

		if (token != NULL) {
			fprintf(stdout, "\nNumber of parameters exceeded.\n\n");
			return false;
		}
	}

	return true;
}


int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 4) {
		usage(argv[0]);
	}


	if (strlen(argv[1]) > 10) {
		fprintf (stdout, "The ID should not exceed 10 characters.\n");
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");


    // send cliend_ID to server
    memset(buffer, 0, BUFLEN);
    sprintf(buffer,"%s\n", argv[1]);

    n = send(sockfd, buffer, strlen(buffer), 0);
	DIE(n < 0, "send");


	int connection_approved = recv(sockfd, buffer, BUFLEN, 0);
	if (strcmp(buffer, "exit\n") == 0) {
        fprintf(stderr, "Connection refused. ID already in use\n");
        shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
		return 0;
	}	

	fd_set read_fds;	// reading set used by select()
	fd_set tmp_fds;		// temporary set
	int fdmax;
	fdmax = sockfd;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);     
	
	while (1) {

		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		int bytes_sent;
        int bytes_received;
  		

		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// do I have incoming messages?
					
					memset(buffer, 0, BUFLEN);
       				bytes_received = recv(sockfd, buffer, BUFLEN, 0);
					if (bytes_received == 0) {
                        shutdown(sockfd, SHUT_RDWR);
						close(sockfd);
						return 0;
					}
					
					// the first (at most) 10 characters from every message
					// reveived will be the size of the packet

					char *buff = (char*) calloc (0, 10);
					memcpy (buff, buffer, min(10, (int)strlen(buffer)));
					
					char *message_size = strtok(buff, " ");
					
					int msg_size = atoi(message_size);
					sprintf(buff, "%d", msg_size);
					if(msg_size <= 0)
						continue;
				
					string buf = (string (buffer)).substr(strlen(message_size) + 1);

					free(buff);

					// recv until we receive the whole packet -> print it
					if (buf.length() + 1 <= msg_size) {
						cout << buf;
						bytes_received = buf.length() + 1;
						while (bytes_received < msg_size) {
							int bytes_recv;
							memset(buffer, 0, BUFLEN);
							bytes_recv = recv(sockfd, buffer, BUFLEN, 0);
							if (bytes_recv == 0) {
								break;
							}
							bytes_received += bytes_recv;
							fprintf(stdout, "%s", buffer);
						}

					}
				}
				else if (i == 0) {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					// send subscribe/unsubscribe command to server

					if (strcmp(buffer, "exit\n") == 0) {
                        shutdown(sockfd, SHUT_RDWR);
						close(sockfd);
						return 0;
					}

					bool r = verify_command(buffer);
					
					if (!r)
						continue;


					n = send(sockfd, buffer, strlen(buffer), 0);
					DIE(n < 0, "send");
					
				}
			}
		}
       
	}
    shutdown(sockfd, SHUT_RDWR);
	close(sockfd);

	return 0;
}
