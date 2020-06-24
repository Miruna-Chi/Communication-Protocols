#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define URLEN 40

int server_conn_send(char *ip, char *message, char **response) {
	// - open the connection, send the message, close the connection
	// - function needed because connection closes repeatedly
	int sockfd;
	sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		perror("Socket creation failed.");

	send_to_server(sockfd, message);
	*response = receive_from_server(sockfd);
	int ret = handle_server_response(*response);

	close_connection(sockfd);
	close(sockfd);

	return ret;
}

int main(int argc, char *argv[])
{
	char buffer[BUFLEN];
    char *message;
    char *response = NULL;
    int sockfd, ret;

    char *ip = "3.8.116.10";
    char *host = "Host: ec2-3-8-116-10.eu-west-2.compute.amazonaws.com";
    char *url;
	char *jwt = NULL;
	char login_cookie[200];
	char book_id[32];
	int cookie = 0, have_jwt = 0;
	
    sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
		perror("Socket creation failed.");
    
    while (1) {
		memset(buffer, 0, BUFLEN);
		fgets(buffer, BUFLEN - 1, stdin);
		// send command to server

		if (strcmp(buffer, "exit\n") == 0) {
			shutdown(sockfd, SHUT_RDWR);
			close(sockfd);
			return 0;
		}
		
		if (!strncmp(buffer, "register", 8)) {
			
			if (buffer[8] != '\n') {
				printf("\nInvalid command. Try \"register\".\n\n");
				continue;
			}
			if (cookie == 1) {
				printf("\nYou are logged in. Log out before registering as a "
						"new user.\n\n");
				continue;
			}
			
			url = "/api/v1/tema/auth/register";
			message = auth(host, url);
			if (message == NULL)
				continue;
			
			ret = server_conn_send(ip, message, &response);

			if (ret == 0) {
				printf("\nAccount successfully created.\n\n");
			}
		}
		else if (!strncmp(buffer, "login", 5)) {
			if (buffer[5] != '\n') {
				printf("\nInvalid command. Try \"login\".\n\n");
				continue;
			}

			if (cookie == 1) {
				printf("\nYou are already logged in.\n\n");
				continue;
			}	

			url = "/api/v1/tema/auth/login";
			
			message = auth(host, url);
			if (message == NULL)
				continue;
			
			ret = server_conn_send(ip, message, &response);

			if (ret == 0) {
				memset(login_cookie, 0, sizeof(login_cookie));
				strcpy(login_cookie, return_cookie(response));
				cookie = 1;
				printf("\nSuccessfully logged in.\n\n");
			}
		}
		else if (!strncmp(buffer, "logout", 6)) {
			if (buffer[6] != '\n') {
				printf("\nInvalid command. Try \"logout\".\n\n");
				continue;
			}
			
			if (cookie == 0) {
				printf("\nCannot log out. You are not logged in.\n\n");
				continue;
			}

			cookie = 0;
			have_jwt = 0;
			url = "/api/v1/tema/auth/logout";
    		message = compute_get_request(host, url, NULL, login_cookie, NULL);
			ret = server_conn_send(ip, message, &response);
			if (ret == 0)
				printf("\nSuccessfully logged out.\n\n");
		}
		else if (!strncmp(buffer, "enter_library", 13)) {
			if (buffer[13] != '\n') {
				printf("\nInvalid command. Try \"enter_library\".\n\n");
				continue;
			}
			
			if (cookie == 0) {
				printf("\nCannot enter library. You are not logged in.\n\n");
				continue;
			}

			url = "/api/v1/tema/library/access";
    		message = compute_get_request(host, url, NULL, login_cookie, NULL);
			ret = server_conn_send(ip, message, &response);
			
			if (ret == 0) {
				jwt = parse_json_jwt(response);
				have_jwt = 1;
				printf("\nSuccessfully entered library.\n\n");
			}
		}
		else if(!strncmp(buffer, "get_books", 9)) {
			if (buffer[9] != '\n') {
				printf("\nInvalid command. Try \"get_books\".\n\n");
				continue;
			}
			if (have_jwt == 0) {
				printf("\nCannot access books. You are not authorized. "
						"Try \"enter_library\" first\n\n");
				continue;
			}

			url = "/api/v1/tema/library/books";
			message = compute_get_request(host, url, NULL, NULL, jwt);
			ret = server_conn_send(ip, message, &response);
			parse_json_books(response);
		}
		else if(!strncmp(buffer, "get_book", 8)) {
			if (buffer[8] != '\n') {
				printf("\nInvalid command. Try \"get_book\".\n\n");
				continue;
			}
			if (have_jwt == 0) {
				printf("\nCannot access books. You are not authorized. "
					"Try \"enter_library\" first\n\n");
				continue;
			}

			fprintf(stdout, "%s", "id=");
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			buffer[strlen(buffer) - 1] = 0;  // cut out "\n"

			memset(book_id, 0, sizeof(book_id));
			strcpy(book_id, buffer);
			
			char book_url[100];
			memset(book_url, 0, sizeof(book_url));
			sprintf(book_url, "%s%s", "/api/v1/tema/library/books/", book_id);
			message = compute_get_request(host, book_url, NULL, NULL, jwt);
			ret = server_conn_send(ip, message, &response);
			
			if (ret == 0)
				parse_json_book(response);
		}
		else if(!strncmp(buffer, "add_book", 8)) {
			if (buffer[8] != '\n') {
				printf("\nInvalid command. Try \"add_book\".\n\n");
				continue;
			}
			if (have_jwt == 0) {
				printf("\nCannot add books. You are not authorized. "
					"Try \"enter_library\" first\n\n");
				continue;
			}

			url = "/api/v1/tema/library/books";
			message = make_book(host, url, jwt);

			ret = server_conn_send(ip, message, &response);
			if (ret == 0)
				printf("\nBook successfully added to the library.\n\n");
		}
		else if(!strncmp(buffer, "delete_book", 11)) {
			if (buffer[11] != '\n') {
				printf("\nInvalid command. Try \"delete_book\".\n\n");
				continue;
			}
			if (have_jwt == 0) {
				printf("\nCannot access books. You are not authorized. "
				"Try \"enter_library\" first\n\n");
				continue;
			}

			fprintf(stdout, "%s", "id=");
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			buffer[strlen(buffer) - 1] = 0;  // cut out "\n"

			memset(book_id, 0, sizeof(book_id));
			strcpy(book_id, buffer);
			
			char book_url[100];
			memset(book_url, 0, sizeof(book_url));
			sprintf(book_url, "%s%s", "/api/v1/tema/library/books/", book_id);
			message = compute_delete_request(host, book_url, jwt);
			ret = server_conn_send(ip, message, &response);

			if(ret == 0)
				printf("\nBook successfully deleted.\n\n");
		}
		else printf("\nInvalid command. Try again.\n\n");


	}


    close_connection(sockfd);
	close(sockfd);

    

    return 0;
}
