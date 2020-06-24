#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

char *compute_get_request(char *host, char *url, char *query_params,
                            char *cookie, char *jwt_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and 
    // protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    compute_message(message, host);

    // Step 3 (optional): add headers and/or cookies, according to the
    // protocol format
    if (cookie != NULL) {
        memset (line, 0, LINELEN);
        strcat(line, "Cookie: "); 
       
        strcat(line, cookie);
        compute_message(message, line);
    }

    if (jwt_token != NULL) {
        memset (line, 0, LINELEN);
        strcat(line, "Authorization: Bearer "); 
       
        strcat(line, jwt_token);
        compute_message(message, line);
    }

    // Step 4: add final new line
    compute_message(message, "\r\n");
    
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, 
                                char *payload, char *jwt_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    compute_message(message, host);

    
    /* Step 3: add necessary headers (Content-Type and Content-Length are
            mandatory) in order to write Content-Length you must first compute
            the message size
    */
    if (jwt_token != NULL) {
        memset (line, 0, LINELEN);
        strcat(line, "Authorization: Bearer "); 
       
        strcat(line, jwt_token);
        compute_message(message, line);
    }

    int content_length = strlen(payload);
    sprintf(line, "%s %d", "Content-Length:", content_length);
    
    compute_message(message, content_type);
    compute_message(message, line);

    // Step 5: add new line at end of header
    compute_message(message, "");

    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);
    compute_message(message, payload);
    compute_message(message, "\r\n");
    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char *jwt_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "DELETE %s", url);
    compute_message(message, line);
    
    // Step 2: add the host
    compute_message(message, host);

    
    /* Step 3: add necessary headers (Authorization)
    */
    if (jwt_token != NULL) {
        memset (line, 0, LINELEN);
        strcat(line, "Authorization: Bearer "); 
       
        strcat(line, jwt_token);
        compute_message(message, line);
    }

    // Step 4: add new line at end of header
    compute_message(message, "");
    compute_message(message, "\r\n");
    free(line);
    return message;
}

char* create_auth_payload(char *username, char *password) {
    // make the serialized string (json) which contains
    // the username and password

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);

    serialized_string = json_serialize_to_string_pretty(root_value);
    json_value_free(root_value);

    return serialized_string;
}

void print_json_error(char *response) {
    JSON_Value *root_value;
    JSON_Object *error;

    root_value = json_parse_string(response);
    error = json_value_get_object(root_value);

    printf("\nError: %s\n\n", json_object_get_string(error, "error"));
    
    json_value_free(root_value);
}

char* parse_json_jwt(char *response) {
    JSON_Value *root_value;
    JSON_Object *object;

    char *token = strtok(response, "{");
    token = strtok(NULL, "}");

    sprintf(response, "{%s}", token);

    root_value = json_parse_string(response);
    object = json_value_get_object(root_value);

    char *jwt = (char *) malloc(300);
    strcpy(jwt, json_object_get_string(object, "token"));
    json_value_free(root_value);

    return jwt;
}


void parse_json_book(char *response) {
    // get book details from json object, print them
    JSON_Value *root_value;
    JSON_Object *book;

    char *token = strtok(response, "{");
    token = strtok(NULL, "}");

    sprintf(response, "{%s}", token);

    root_value = json_parse_string(response);
    book = json_value_get_object(root_value);

    printf("\nTitle: %s\nAuthor: %s\nGenre: %s\nPage count: %d\n"
        "Publisher: %s\n\n", json_object_get_string(book, "title"),
                            json_object_get_string(book, "author"),
                            json_object_get_string(book, "genre"),
                            (int)json_object_get_number(book, "page_count"),
                            json_object_get_string(book, "publisher"));

    json_value_free(root_value);
}

void parse_json_books(char *response) {
    // parse all books and print them

    char *token = strtok(response, "[");
    token = strtok(NULL, "]");
    sprintf(response, "[%s]", token);

    JSON_Value *root_value;
    JSON_Array *books;
    JSON_Object *book;

    root_value = json_parse_string(response);
    books = json_value_get_array(root_value);

    if (json_array_get_count(books) == 0) {
        printf("\nThere are no books in the library. Yet :).\n\n");
        return;
    }

    printf("\n%-10.10s %s\n", "ID", "Title");
    for (int i = 0; i < json_array_get_count(books); i++) {
        book = json_array_get_object(books, i);
        printf("%-10d %s\n",
               (int)json_object_get_number(book, "id"),
               json_object_get_string(book, "title"));
    }

    printf("\n");
    json_value_free(root_value);
}


int handle_server_response(char *resp) {
    // 0 for success, 1 if it finds an error and prints it or unknown code
    char response[BUFLEN];

    char *token = strtok(response, " ");
    token = strtok(NULL, "\r\n");

    if (!strcmp(token, "201 Created"))
        return 0;

    if (!strcmp(token, "200 OK"))
        return 0;
    
    if (!strcmp(token, "400 Bad Request")  
        || !strcmp(token, "404 Not Found")
        || !strcmp(token, "403 Forbidden")) {

        token = strtok(NULL, "{");
        token = strtok(NULL, "}");

        sprintf(response, "{%s}", token);
        print_json_error(response);
        return 1;
    }

    if (!strcmp(token, "429 Too Many Requests")) {
        printf("\nToo many requests, please try again later.\n\n");
        return 1;
    }

    printf("Unknown code: %s\n\n", token);
    return 1;
}

char* return_cookie(char *resp) {
    // find the cookie in a response and send it
    char response[BUFLEN];
    strcpy(response, resp);
    char *cookie = strstr(response, "Set-Cookie: ");
    cookie = strtok(cookie, " ");
    cookie = strtok(NULL, " ");

    return cookie;
}

char* auth(char *host, char *url) {
    // get username and password 
    char *message = NULL;
    char username[100], password[32];
    char *content_type;
	char *payload;
    char buffer[BUFLEN];


    fprintf(stdout, "%s", "username=");
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);
    buffer[strlen(buffer) - 1] = 0;  // cut out "\n"

    strcpy(username, buffer);

    fprintf(stdout, "%s", "password=");
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);
    buffer[strlen(buffer) - 1] = 0;  // cut out "\n"
    strcpy(password, buffer);


    content_type = "Content-Type: application/json";
    payload = create_auth_payload(username, password);
    
    message = compute_post_request(host, url, content_type, payload, NULL);

    return message;
}

char *make_book(char *host, char *url, char *jwt_token) {
    // make the json book and compute the message
    char *message = NULL;
    char title[50], author[50], genre[50], publisher[50];
    char *content_type;
	char *payload;
    char buffer[BUFLEN];
    int page_count;

    fprintf(stdout, "%s", "title=");
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);
    buffer[strlen(buffer) - 1] = 0;  // cut out "\n"
    strcpy(title, buffer);

    fprintf(stdout, "%s", "author=");
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);
    buffer[strlen(buffer) - 1] = 0;  // cut out "\n"
    strcpy(author, buffer);

    fprintf(stdout, "%s", "genre=");
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);
    buffer[strlen(buffer) - 1] = 0;  // cut out "\n"
    strcpy(genre, buffer);

    fprintf(stdout, "%s", "page_count=");
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);
    buffer[strlen(buffer) - 1] = 0;  // cut out "\n"
    page_count = atoi(buffer);

    fprintf(stdout, "%s", "publisher=");
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);
    buffer[strlen(buffer) - 1] = 0;  // cut out "\n"
    strcpy(publisher, buffer);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;

    json_object_set_string(root_object, "title", title);
    json_object_set_string(root_object, "author", author);
    json_object_set_string(root_object, "genre", genre);
    json_object_set_number(root_object, "page_count", page_count);
    json_object_set_string(root_object, "publisher", publisher);
 
    serialized_string = json_serialize_to_string_pretty(root_value);
    json_value_free(root_value);

    content_type = "Content-Type: application/json";
    payload = serialized_string;
    message = compute_post_request(host, url, content_type, payload, jwt_token);

    return message;
}