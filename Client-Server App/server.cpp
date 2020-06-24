#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "message.h"
#include "helpers.h"
using namespace std;

#define BUFLEN 1552

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}


void disable_Nagle (int sock) {
	int flag = 1;
	int result = setsockopt(sock,            /* socket affected */
						IPPROTO_TCP,     /* set option at TCP level */
						TCP_NODELAY,     /* name of option */
						(char *) &flag,  /* the cast is historical cruft */
						sizeof(int));    /* length of option value */
	if (result < 0) {
		fprintf(stderr, "Could not disable Nagle.\n");
		exit(0);
	}
}

void print_online_subs(map<string, int> online_subs) {
	map<string, int>::iterator it;

	cout << "Online subs: \n";
	for (it = online_subs.begin(); it != online_subs.end(); it++) {
		cout << "\t" << it->first << "\t" << it->second << "\n"; 
	}

	cout << "-----------------------\n\n";
}

void print_online_subs_fd(map<int, string> online_subs_fd) {
	map<int, string>::iterator it;

	cout << "Online subs fd: \n";
	for (it = online_subs_fd.begin(); it != online_subs_fd.end(); it++) {
		cout << "\t" << it->first << "\t" << it->second << "\n"; 
	}

	cout << "-----------------------\n\n";
}

void print_offline_subs(map<string, string> offline_subs) {
	map<string, string>::iterator it;

	cout << "Offline subs: \n";
	for (it = offline_subs.begin(); it != offline_subs.end(); it++) {
		cout << "\t" << it->first << "\t" << it->second << "\n"; 
	}

	cout << "-----------------------\n\n";
}


void print_topics(map<string, map <string, bool>> topics) {
	map<string, map<string, bool>>::iterator it;
	map<string, bool>::iterator mit;

	cout << "Topics: \n";
	for (it = topics.begin(); it != topics.end(); it++) {
		cout << "\tTopic: " << it->first << "\n";
		for (mit = (it->second).begin(); mit != (it->second).end(); mit++) {
				cout << "\t\t" << mit->first << "\t" << mit->second << "\n"; 
		} 
	}

	cout << "-----------------------\n\n";
}



int main(int argc, char *argv[])
{
	int sockfd_TCP, sockfd_UDP, newsockfd_TCP, port_no;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, sub_addr;

	int n, i, tcp_bind, udp_bind, ret;
	socklen_t sub_len;

	sub_len = sizeof(sub_addr);

	fd_set read_fds;		// set used by select()
	fd_set tmp_fds_TCP;		// temporary set
	int fdmax;				// max fd from the read_fds set

	if (argc < 2) {
		usage(argv[0]);
	}

	
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds_TCP);

	sockfd_TCP = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_TCP < 0, "socket");

	sockfd_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_UDP < 0, "socket");

	port_no = atoi(argv[1]);
	DIE(port_no == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_no);
	serv_addr.sin_addr.s_addr = INADDR_ANY;


	disable_Nagle(sockfd_TCP);

	// piece of code to enable reusing ports without waiting for them to close
	int enable = 1;
	if (setsockopt(sockfd_TCP, SOL_SOCKET, SO_REUSEADDR, 
		&enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	tcp_bind = bind(sockfd_TCP, (struct sockaddr *) &serv_addr, 
		sizeof(struct sockaddr));
	DIE(tcp_bind < 0, "bind");

	udp_bind = bind(sockfd_UDP, (struct sockaddr *) &serv_addr, 
		sizeof(struct sockaddr));
	DIE(udp_bind < 0, "bind");

	ret = listen(sockfd_TCP, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// map of online subs connecting the id and the fd
	map<string, int> online_subs;

	// the reverse of the previous map (so we can find the ID without searching
	// by value in the first map - getting O(log n) the easy way)
	map<int, string> online_subs_fd;

	// map of subs connecting their id and their buffers for offline keeping
	map<string, string> offline_subs;

	// map of topics and their subs (pair: id + SF);
	map<string, map<string, bool>> topics; 

	// add new fd to read_fds
	FD_SET(sockfd_UDP, &read_fds);
	FD_SET(sockfd_TCP, &read_fds);
	FD_SET(0, &read_fds);     
	fdmax = max(sockfd_TCP, sockfd_UDP);

	int j = 0;
	while (1) {
		
		tmp_fds_TCP = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds_TCP, NULL, NULL, NULL);
		DIE(ret < 0, "select");


		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds_TCP)) {
				if (i == sockfd_TCP) {
					
					// a subscriber signed up / logged in => check if the ID is
					// already in use by another online user
					// shut down the connection if that's the case
					
					newsockfd_TCP = accept(sockfd_TCP, 
						(struct sockaddr *) &sub_addr, &sub_len);
					DIE(newsockfd_TCP < 0, "accept");

					
					n = recv(newsockfd_TCP, buffer, sizeof(buffer), 0);
					
					DIE(n < 0, "recv");

					char *token = strtok(buffer, " \n"); 

					char *id = token, *server_IP; 
					int server_port;
					
					
					server_IP = inet_ntoa(sub_addr.sin_addr);					
					server_port = ntohs(sub_addr.sin_port);

					string ID = string(id);
					
					// id already online -> we'll find it in the online_subs map
					if (online_subs.find(ID) != online_subs.end()) {
						send(newsockfd_TCP, "exit\n", strlen("exit\n"), 0);
						shutdown(newsockfd_TCP, SHUT_RDWR);
					}
					else {
					
						// send an approval to the client
						send(newsockfd_TCP, "okay\n", strlen("okayn\n"), 0);

						// add new_socket returned by accept() to read_fds
						FD_SET(newsockfd_TCP, &read_fds);
						if (newsockfd_TCP > fdmax) { 
								fdmax = newsockfd_TCP;
						}
						
						// subscriber went online -> add it to the online_subs maps
						online_subs.insert(pair<string, int>(ID, newsockfd_TCP));
						online_subs_fd.insert(pair<int, string>(newsockfd_TCP, ID));

						printf("New client %s connected from %s:%d.\n",
										id, server_IP, server_port);
						
						// if the user isn't new (it was added to offline_subs)
						// check its buffer and send the messages they received 
						// while offline

						if (offline_subs.find(ID) != offline_subs.end()) {
							
						// find the buffer mapped to the id
						string offline_msg = offline_subs.at(ID);
							
						if (offline_msg.length() == 0)
							continue; 
							
			 			char *offline_msg_len = (char*) calloc(0, 10); 
							memset(offline_msg_len, 0, 10);

							sprintf(offline_msg_len, "%lu ", offline_msg.length());
							// send message size + " "
							send(newsockfd_TCP, offline_msg_len, strlen(offline_msg_len), 0);
							// send message
							send(newsockfd_TCP, offline_msg.c_str(), offline_msg.length(), 0);
							offline_subs.erase(ID);
						}

						memset(buffer, 0, BUFLEN);
					}
				} else if (i == sockfd_UDP) {
		  
		  			// an UDP client is sending us messages

					int bytes_recv;
					bytes_recv = recvfrom(sockfd_UDP, buffer, BUFLEN, 0, 
			 		 (struct sockaddr *) &sub_addr, &sub_len);
					
					if (bytes_recv <= 0)
						continue;
					else {
						struct udp_msg *message = (UDP_msg) buffer;

					/* get the topic of the message, get all the users who subscribed
						to it, iterate through them:
						- if they're online, send the message
						- if they're not online, check the SF bool in the <id, sf> pair,
						add the message to their online buffer if SF is true
					*/
					map<string, map<string, bool>>::iterator it = topics.find(message->topic);

					// didn't find the topic among our subscribers? no need to create it
					if(it == topics.end()) {
						continue;
					} else {
						
			  		// to check out how these maps work, decomment the following:

					//print_online_subs(online_subs);
					//print_online_subs_fd(online_subs_fd);
					//print_offline_subs(offline_subs);
					//print_topics(topics);


			 		// get users, iterate through them
					map<string, bool> id_SF = it->second;
					map<string, bool>::iterator mit;

			 	 	// parse message:
				  
					char *server_IP;
					int server_port;
				
					server_IP = inet_ntoa(sub_addr.sin_addr);					
					server_port = ntohs(sub_addr.sin_port);

					string parsed_message = simplify_msg(server_IP, server_port,
						*message);


					for (mit = id_SF.begin(); mit != id_SF.end(); mit++) {

		
						if (online_subs.find(mit->first) != online_subs.end()) {
							// user is online => get the fd, we're sending them stuff
							int user_fd = online_subs.at(mit->first);

							// send it the same way we did above 
							// (first the size, then the message)
							char *msg_len = (char *) calloc (0, 10);
							memset(msg_len, 0, 10);
							sprintf(msg_len, "%lu ", parsed_message.length() + 1);

							send(user_fd, msg_len, strlen(msg_len), 0);
							send(user_fd, parsed_message.c_str(), 
							parsed_message.length(), 0);

									free(msg_len);
								}
								else if(mit->second == true) {
									// user is offline and SF for this topic is 1 
									// => we're putting stuff in their buffer

									// find the buffer by user id
									map<string, string>::iterator it = offline_subs.find(mit->first);

				  					// didn't find it -> continue
									if (it == offline_subs.end())
										continue;
									
									it->second = it->second + parsed_message;
									offline_subs.at(mit->first) = it->second;
								}
							}
						}
					}
				} else if (i == 0) {
		  			// exit: server
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
				
					if (strcmp(buffer, "exit\n") == 0) {
						shutdown(sockfd_TCP, SHUT_RDWR);
						shutdown(sockfd_UDP, SHUT_RDWR);
						close(sockfd_TCP);
						close(sockfd_UDP);
						return 0;
					}
				} else {
					// we got data from our subscribers (tcp_client fd),
					// so the server has to analyse them
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// user logged out/client disconnected
						string id = online_subs_fd.at(i);

						cout << "Client " << id << " disconnected.\n";

						// remove it from online maps, add it to the offline map
						string buf = "";
						offline_subs.insert(pair<string, string> (id, buf));
						online_subs.erase(id);
						online_subs_fd.erase(i);

						shutdown(i, SHUT_RDWR);
						close(i);
						
						// take the closed socket out of the read_fds set
						FD_CLR(i, &read_fds);

					} else {
			
						// a client wants to subscribe or unsubscribe

						char *command = strtok (buffer, " ");
						if (!strcmp(command, "subscribe")) {
							
							// parse command
							command = strtok (NULL, " ");
							char *topic = command;
							command = strtok (NULL, " \n");
							int SF = atoi(command);

			  				// get client id by their fd
							string id = online_subs_fd.at(i);

			  				//set the topic and the sf param
							map<string, bool> id_SF;              
							map<string, map<string, bool>>::iterator it = topics.find(topic);

			  				// create the topic if it hasn't been found -> add the pair
							if(it == topics.end()) {
								id_SF.insert(pair<string, bool>(id, (bool) SF));
								topics.insert(pair<string, map<string, bool>>(topic, id_SF));
							} else {
								id_SF = topics.at(topic);
								id_SF.erase(id); // in case they already subscribed
								// insert the pair with the new SF param 
								id_SF.insert(pair<string, bool>(id, (bool) SF));
								it->second = id_SF;
							}
							

						}
						else if (!strcmp(command, "unsubscribe")) {
							command = strtok (NULL, " \n");
							char *topic = command;
			  
			  				// get id paired with this fd
							string id = online_subs_fd.at(i);

							// if the topic has been found, erase the entry in the topics map
							if (topics.find(topic) != topics.end()) {
								topics.at(topic).erase(id);
							}
						}
					}
				}
			}
		}
	}
	shutdown(sockfd_TCP, SHUT_RDWR);
	close(sockfd_TCP);

	return 0;
}
