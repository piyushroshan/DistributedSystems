/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "4020" // the port client will be connecting to 

#define MAXDATASIZE 1000 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
    size_t n;
    if (size) {
        --size;
        for ( n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

char* rand_string_alloc(size_t size)
{
     char *s = malloc(size + 1);
     if (s) {
         rand_string(s, size);
     }
     return s;
}

int main(int argc, char *argv[]){
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    int count =100;
    if (argc < 3) {
        fprintf(stderr,"usage: client hostname group\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    /*    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';
 
    printf("client: received '%s'\n",buf);
    */
    /*
     * Send group id first
     */

    memset(buf,'\0',MAXDATASIZE);
    printf("%s\n",argv[2]);
    sprintf(buf,"%s", argv[2]);

    if ((send(sockfd,buf, strlen(buf),0))== -1) {
            fprintf(stderr, "Failure Sending Message\n");
            close(sockfd);
            exit(1);
    }
    else {
        printf("Client:Message being sent: %s\n",buf);
        while(1 && count !=5) {
            memset(buf,'\0',MAXDATASIZE);
            sleep(2);
            snprintf(buf, MAXDATASIZE,"Client of group %s : Message: %s", argv[2], rand_string_alloc(10));

            if ((send(sockfd,buf, strlen(buf),0))== -1) {
                    fprintf(stderr, "Failure Sending Message\n");
                    close(sockfd);
                    exit(1);
            }
            else {
                    printf("Client:Message being sent: %s\n",buf);
            }
            count --;
            sleep(10);
        }
		}
    close(sockfd);
    return 0;
}
