#ifndef _REQUESTS_
#define _REQUESTS_

// computes and returns a GET request string (query_params
// and cookies can be set to NULL if not needed)
char* compute_get_request(char *host, char *url, char *query_params,
							char *cookie, char *jwt_token);

// computes and returns a POST request string (cookies can be NULL if not needed)
char* compute_post_request(char *host, char *url, char* content_type,
							char *payload, char *jwt_token);

char* compute_delete_request(char *host, char *url, char *jwt_token);

char* create_auth_payload(char *username, char *password); 

int handle_server_response(char *response);

void print_json_response(char *response);

char* parse_json_jwt(char *response);

void parse_json_book(char *response);

void parse_json_books(char *response);

char* return_cookie(char *response);

char* auth(char *host, char *url);

char* make_book(char *host, char *url, char *jwt_token);
#endif
