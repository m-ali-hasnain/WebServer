#define _XOPEN_SOURCE 700
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
char* readRequest();
int main(int argc, char** argv) {
    int fd[2];
    if(pipe(fd)==-1){
        fprintf(stderr, "Failed to create pipe\n");
        exit(2);
    }
    int f = fork();
    if(f<0){
        fprintf(stderr, "Failed to fork process\n");
        exit(1);
    }
    else if(f == 0){
        char* request = readRequest();
        int size = strlen(request);
        close(fd[0]);
        write(fd[1], &size, sizeof(size));
        write(fd[1], request, size);
        
    }else{
        wait(NULL);
        close(fd[1]);
        
        int size;
        read(fd[0], &size, sizeof(int));
        char* request_template = malloc(sizeof(char)*size);
        read(fd[0], request_template, size);
        printf("Request Main: %s", request_template);
        
        char buffer[BUFSIZ];
        enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
        char request[MAX_REQUEST_LEN];
        struct protoent *protoent;
        char *hostname = "example.com";
        in_addr_t in_addr;
        int request_len;
        int socket_file_descriptor;
        ssize_t nbytes_total, nbytes_last;
        struct hostent *hostent;
        struct sockaddr_in sockaddr_in;
        unsigned short server_port = 80;

        if (argc > 1)
            hostname = argv[1];
        if (argc > 2)
            server_port = strtoul(argv[2], NULL, 10);

        request_len = snprintf(request, MAX_REQUEST_LEN, request_template, hostname);
        if (request_len >= MAX_REQUEST_LEN) {
            fprintf(stderr, "request length large: %d\n", request_len);
            exit(EXIT_FAILURE);
        }
        printf("Request is: %s",request);
        /* Build the socket. */
        protoent = getprotobyname("tcp");
        if (protoent == NULL) {
            perror("getprotobyname");
            exit(EXIT_FAILURE);
        }
        socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
        if (socket_file_descriptor == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }
       
        /* Build the address. */
        hostent = gethostbyname(hostname);
        if (hostent == NULL) {
            fprintf(stderr, "error: gethostbyname(\"%s\")\n", hostname);
            exit(EXIT_FAILURE);
        }
        in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
        if (in_addr == (in_addr_t)-1) {
            fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
            exit(EXIT_FAILURE);
        }
        sockaddr_in.sin_addr.s_addr = in_addr;
        sockaddr_in.sin_family = AF_INET;
        sockaddr_in.sin_port = htons(server_port);

        /* Actually connect. */
        if (connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
            perror("connect");
            exit(EXIT_FAILURE);
        }

        /* Send HTTP request. */
        nbytes_total = 0;
        while (nbytes_total < request_len) {
            nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
            if (nbytes_last == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            nbytes_total += nbytes_last;
        }

        /* Read the response. */
        fprintf(stderr, "debug: before first read\n");
        while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
            fprintf(stderr, "debug: after a read\n");
            write(STDOUT_FILENO, buffer, nbytes_total);
        }
        fprintf(stderr, "debug: after last read\n");
        if (nbytes_total == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        close(socket_file_descriptor);
        exit(EXIT_SUCCESS);
    }
}

char* readRequest(){
	FILE * fp;
    char * line = NULL;
	char *req = malloc(sizeof(char)*1024);
	int size = 0;
    size_t len = 0;
    ssize_t read;

    fp = fopen("TEMPIN", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
		size = size + (read-1);
		line[read-1]='\0';
		strcat(line, "\\r\\n");
		strcat(req, line);
    
    }
	if(line)free(line);

    fclose(fp);
	req[strlen(req)]='\0';
	return req;
}
