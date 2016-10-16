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
#include <signal.h>
#include <pthread.h>

#define PORT "4020"   // port we're listening on
#define MAX_GROUPS 100
#define MAX_CLIENTS 255
typedef struct Client{
    int     sockfd;
    int     msg_type;
    int     group;
    char    buf[1000];
    struct Client *next_client;
}client_node;

typedef struct Group_members{
    client_node *client; 
    struct Group_members *next;
}group_node;

typedef struct Group_member_list {
    int total_clients;
    int group_id;
    char *message;
    group_node *group;
    group_node *group_head;
    //client_node *group_clients[255];
}group_list;

client_node* client_head = NULL;
int num_clients = 0;
group_list group_data[MAX_GROUPS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// get sockaddr, IPv4 or IPv6:

void list_all_clients();
void remove_from_group(client_node *client);

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void store_new_client(int newfd) {
     pthread_mutex_lock(&lock);
     client_node *new_client = NULL;
     client_node *tmp_head = client_head;
     new_client = (client_node*)calloc(1,sizeof(client_node));
     if (new_client != NULL) {
        new_client->sockfd = newfd;
        new_client->next_client = NULL;
        new_client->msg_type = 0;
        //new_client->buf = {'\0'};
        if(client_head == NULL){
           printf("Client added %d \n", newfd);
           client_head = new_client;
        } else {
           while(tmp_head->next_client != NULL)
                tmp_head = tmp_head->next_client;
           tmp_head->next_client = new_client;
           printf("Client added %d \n",newfd);
        }
        num_clients++;
        list_all_clients();
    }
    pthread_mutex_unlock(&lock);
}

void delete_existing_client(int fd){
     client_node *delete_client = NULL;
     client_node *prev_node = client_head;
     /*
      * Remove from group 
      */
     /* Node to be deleted is head node*/
     if( client_head && client_head->sockfd == fd) {
        remove_from_group(prev_node);
        if(client_head->next_client != NULL) {
           printf("Client deleted %d \n",fd);
          client_head = client_head->next_client;
        }
        free(prev_node);
        num_clients--;
      } else {
        while(prev_node->next_client != NULL){
             if(prev_node->next_client->sockfd == fd){
                delete_client= prev_node->next_client;
                remove_from_group(delete_client);
                prev_node->next_client = delete_client->next_client;
                free(delete_client);
                num_clients--;
                printf("Client deleted %d \n",fd);
                break;
              } else {
                  prev_node = prev_node->next_client;
              }
    }
   }
}

/* Checks whether the client is present in linked list */
client_node* find_client( int client_fd)
{
    client_node* current = client_head;  // Initialize current
    while (current != NULL)
    {
        if (current->sockfd == client_fd)
            return current;
        current = current->next_client;
    }
    return NULL;
}
void list_all_clients(){
     client_node* tmp_ptr = client_head;
     //printf("\n  Listing clients =====\n");
     while(tmp_ptr != NULL){
          //printf("  Client socket: %d msg %s\n",tmp_ptr->sockfd, tmp_ptr->buf);
          tmp_ptr = tmp_ptr->next_client;
     }
     //printf("\n  List ended\n");
}

void create_dummy_groups(){
     int index = 0;
     for ( index = 0; index < MAX_GROUPS; index++) {
          group_data[index].group_id = index+1;
          group_data[index].total_clients = 0;
          group_data[index].message = "";
          //group_data[index].group_clients[MAX_CLIENTS] = NULL;
          group_data[index].group = NULL;
          group_data[index].group_head = NULL;
     }
}
/*
void add_client_to_group(client_node *client){
     int group = client->group;
     int cur_clients_count = group_data[group].total_clients;
     group_data[group].group_clients[cur_clients_count] = client;
     (group_data[group].total_clients)++;
     printf("Client %d added to group %d\n",client->sockfd, group);
}
*/

void add_client_to_group(client_node *client) {
        group_node *new_node = NULL;
        int group = client->group;
        new_node = (group_node *)calloc(1,sizeof(new_node));
        new_node->client = client;
        new_node->next = NULL;
        (group_data[group].total_clients)++;
        if (group_data[group].group == NULL) {
           //printf(" First client for group %d\n",group);
           group_data[group].group = new_node; 
           group_data[group].group_head = new_node;
           group_data[group].group_id = group;
        } else {
           //printf(" Client %d added group %d \n",client->sockfd,client->group);
           group_node *tmp_node = group_data[group].group_head;
           while(tmp_node->next != NULL)
             tmp_node = tmp_node->next;
           tmp_node->next = new_node;
        }
}

void remove_from_group(client_node *client) {
      group_node *rem_node = NULL;
      int group = client->group;
      rem_node = group_data[group].group_head;
      if (rem_node->client->sockfd == client->sockfd)
      {
           if( group_data[group].group_head->next == NULL) 
           {
             group_data[group].group_head = NULL;
             group_data[group].group = NULL;
           } else {
             group_data[group].group =  group_data[group].group->next;
             group_data[group].group_head = group_data[group].group;
           }
           group_data[group].total_clients--;
      } else {
        while (rem_node->next !=NULL) {
            if (rem_node->next->client->sockfd == client->sockfd)
            {
                if (rem_node->next->next != NULL)
                    rem_node->next = rem_node->next->next;
                else
                    rem_node->next = NULL;
                group_data[group].total_clients--;
            } else {
                rem_node = rem_node->next;
            }
        }
      }

}
void read_input_data(int client_fd, char *buff, int size ){
     client_node* client;
     int grp;
     pthread_mutex_lock(&lock);
     //printf ("======================================\n");
     //printf("Received message \"%s\" from %d\n",buff, client_fd);
     //printf("Searching for client %d \n",client_fd);
     list_all_clients();
     client = find_client(client_fd);
     if(client != NULL){
        //printf("  Existing Client found %d with current stored message \"%s\" \n",client->sockfd,client->buf);
        if( client->msg_type == 0 ){
            grp = atoi(buff);
            (client->msg_type)++;
            client->group = grp;
            add_client_to_group(client);
        } else {
            strncpy(client->buf,buff,900);
            //printf("  Store message \"%s\" from client %d\n",client->buf, client->sockfd);
        }
     pthread_mutex_unlock(&lock);
     //printf ("======================================\n");
     }
}
void display_group_messages_thread(void *ptr){
    int index = 0;
    int client_index = 0;
    int num_clients_in_grp = 0;
    group_node *tmp = NULL;
    while(1) {
       sleep(5);
       printf("DISPLAY MESSAGES\n");
       for( index = 0; index <MAX_GROUPS; index++) {
         num_clients_in_grp = group_data[index].total_clients;
         if (num_clients_in_grp == 0)
            continue;
         printf("==============================================================\n");
         printf("GROUP %d\n", index);
         printf("Total clients in this group %d \n", num_clients_in_grp);
         tmp = group_data[index].group_head;
         while (tmp != NULL) {
            printf("%s \n",tmp->client->buf);
            tmp = tmp->next;
         }
         printf("==============================================================\n");
         /* for (client_index = 0; client_index < num_clients_in_grp; client_index++) {
               printf("%s\n",group_data[index].group_clients[client_index]->buf);
          }
         */
       }
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

    pthread_t t1;
    /*
     * store_new_client(1);
    read_input_data(1,"10",2);
    read_input_data(1,"aaaa",4);
    read_input_data(1,"bbbb",4);
    store_new_client(2);
    read_input_data(2,"20",2);
    read_input_data(2,"cccc",4);
    store_new_client(3);
    read_input_data(3,"10",2);
    read_input_data(3,"dddd",4);
    read_input_data(3,"dddd",4);
    store_new_client(4);
    read_input_data(4,"30",2);
    read_input_data(4,"eeee",4);
    list_all_clients();
    delete_existing_client(1);
    delete_existing_client(3);
    */
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
    create_dummy_groups();
    printf("Server: Started \n");
    pthread_create(&t1, NULL, (void *) &display_group_messages_thread, NULL);

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
                        //printf("Client : %s \n",buf);
                        read_input_data(i, buf, nbytes);
                        //data for display to be processed here
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    pthread_join(t1, NULL);
    return 0;
}
