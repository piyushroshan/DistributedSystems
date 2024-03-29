/*
** selectserver.c -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "3490"   // port we're listening on
typedef struct Client_list{
    int     sockfd;
    int     msg_type;
    int     group;
    char    *buf;
    struct Client_list *next_client;
}client_list;

typedef struct Group_member_list {
    client_list *my_clients;
    int total_clients;
}group_info;

client_list* client_head = NULL;
int num_clients = 0;
//group_info group_list = { 0, 0, 0 , {0}, NULL};
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void store_new_client(int newfd) {
     client_list *new_client = NULL;
     client_list *tmp_head = client_head;
     new_client = (client_list*)calloc(1,sizeof(client_list));
     if(client_head == NULL){
        printf("First  client \n");
        new_client->sockfd = newfd;
        new_client->next_client = NULL;
        client_head = new_client;
     } else {
     while(tmp_head->next_client != NULL)
           tmp_head = tmp_head->next_client;
     if (new_client != NULL){
         new_client->sockfd = newfd;
         tmp_head->next_client = new_client->next_client;
     }
     }
     num_clients++;
     printf("Client added %d \n",newfd);
}

void delete_existing_client(int fd){
     client_list *delete_client = NULL;
     client_list *prev_node = client_head;
     /* Node to be deleted is head node*/
     if( client_head && client_head->sockfd == fd) {
        if(client_head->next_client == NULL) {
           free(client_head);
           num_clients--;
           printf("Client deleted %d \n",fd);
        }
    }else {
        while(prev_node->next_client != NULL){
             if(prev_node->next_client->sockfd == fd){
                delete_client= prev_node->next_client;
                prev_node->next_client = delete_client->next_client;
                free(delete_client);
                num_clients--;
                printf("Client deleted %d \n",fd);
             }
    }
   }
}

/* Checks whether the client is present in linked list */
client_list* find_client( int client_fd)
{
    client_list* current = client_head;  // Initialize current
    while (current != NULL)
    {
        if (current->sockfd == client_fd)
            return current;
        current = current->next_client;
    }
    return NULL;
}
void list_all_clients(){
     client_list* tmp_ptr = client_head;
     while(tmp_ptr != NULL){
          printf("Client available: %d\n",tmp_ptr->sockfd);
          tmp_ptr = tmp_ptr->next_client;
     }
}

void read_input_data(int client_fd, char *buff, int size ){
     client_list* client;
     client = find_client(client_fd);
     if(client != NULL){
        printf("Client found \n");
        if(( client->msg_type == 0 )){
            printf(" Group info has not been added\n");
            client->group = atoi(buff);
            printf("Client added to group %d\n",client->group);
        } else {
            client->buf=buff;
            printf("store message from client %s",client->buf);
        }
     (client->msg_type)++;
     }
}

int main(void)
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[1000];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    printf("Server: Started \n");
    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                       store_new_client(newfd);
                    }
                } else {
                    // handle data from a client
                    memset(buf,'\0',1000);
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                            delete_existing_client(i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // we got some data from a client
                        printf("Client : %s \n",buf);
                        read_input_data(i, buf, nbytes);
                        //data for display to be processed here
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

    return 0;
}
