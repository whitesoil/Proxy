/*
 *
 * Last modified : 2017.12.03
 * Hanyang University
 * Computer Science & Engineering
 * Seon Namkung
 *
 * LRU Cache Proxy server.
 *
 */

#include "Proxy.h"

#define MAX_CLNT 256
void * cache_hit(void * args);
void * cache_miss(void * args);

typedef struct thread_hit
{
        Node * node;
        int client_fd;
}Thread_hit;

typedef struct thread_miss
{
        Node * node;
        int client_fd;
        int server_fd;
        char * HTTP_request;

}Thread_miss;

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutex_node,mutex_write,mutex_sock;

int main(int argc, char * argv[]){
        //Init cache
        init_cache();
        pthread_mutex_init(&mutex_node,NULL);
        pthread_mutex_init(&mutex_write,NULL);
        pthread_mutex_init(&mutex_sock,NULL);


        int client_fd, proxy_fd, server_fd;
        int portno;
        struct sockaddr_in client_addr, proxy_addr, server_addr;
        struct hostent* pHostInfo;
        socklen_t socklen = sizeof(client_addr);
        pthread_t t_id,u_id;

        if(argc < 2) {
                perror("Please enter port #");
                exit(1);
        }

        /*
         * Setting between Proxy and Client
         */
        proxy_fd = socket(PF_INET,SOCK_STREAM,0);

        if(proxy_fd < 0) {
                perror("Error to open proxy socket");
                exit(1);
        }else{
                fputs("Open proxy socket\n",stdout);
        }

        memset(&proxy_addr,0,sizeof(proxy_addr));
        proxy_addr.sin_family = AF_INET;
        proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        proxy_addr.sin_port = htons(atoi(argv[1]));

        if(bind(proxy_fd,(struct sockaddr*)&proxy_addr,sizeof(proxy_addr))==-1) {
                perror("Error to bind");
                exit(1);
        }else{
                fputs("Bind proxy_addr to proxy socket\n",stdout);
        }

        if(listen(proxy_fd,40) == -1) {
                perror("Error to listening");
                exit(1);
        }else{
                fputs("Ready to listen from client\n",stdout);
        }

        /*
         * Use LRU Cache.
         *
         * 1. Read http request from client.
         * 2. Parse request and get host,ip and requested filename.
         * 3. Check wether requested file is in cache.
         * 4-1. If not, make connection to server
         * 4-2. Send request and get response.
         * 4-3. Resend the response to client until end.
         * 4-4. Close connection to server.
         * 5-1. If hit, send content of cache.
         * 5-2. Rearrange cache.
         *
         */
        while(1) {
                int len = 0;
                char HTTP_request[BUF_SIZE];

                memset(HTTP_request,0,BUF_SIZE);


                //Accept connection request from client
                client_fd = accept(proxy_fd,(struct sockaddr *)&client_addr,&socklen);

                if(client_fd < 0) {
                        perror("Error to accept from client");
                }else{
                        fputs("Accept from client\n",stdout);
                }

                if((len = read(client_fd,HTTP_request,BUF_SIZE)) != 0) {
                        fputs("Read request from client\n",stdout);

                        Node * node = find_Node(url_parser(HTTP_request));

                        if(node == NULL) {
                                /*
                                 * Cache miss
                                 */
                                node = (Node *)malloc(sizeof(Node));
                                strcpy(node->name,url_parser(HTTP_request));

                                //Get information of Host. IP, Name etc.
                                pHostInfo = gethostbyname(host_parser(HTTP_request));

                                /*
                                 * Setting between Proxy and Server
                                 */
                                server_fd = socket(PF_INET,SOCK_STREAM,0);

                                if(server_fd < 0) {
                                        perror("Error to open server socket");
                                        exit(1);
                                }else{
                                        fputs("Open server socket\n",stdout);
                                }

                                memset(&server_addr, 0, sizeof(server_addr));
                                server_addr.sin_family=AF_INET;
                                server_addr.sin_port=htons(80);
                                server_addr.sin_addr.s_addr= *((unsigned long*) pHostInfo->h_addr);

                                if(connect(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1) {
                                        perror("Error to Connet to server");
                                }else{
                                        fputs("Connect to server\n",stdout);
                                }

                                http_reconstructor(HTTP_request);

                                //Send HTTP request to server
                                if((len = write(server_fd,HTTP_request,len)) < 0) {
                                        perror("Error to write");
                                }else{
                                        fputs("Send request to server\n",stdout);
                                }

                                Thread_miss * args = (Thread_miss *)malloc(sizeof(Thread_miss));
                                pthread_mutex_lock(&mutex_sock);
                                clnt_socks[clnt_cnt++]=client_fd;
                                pthread_mutex_unlock(&mutex_sock);
                                args->node = node;
                                args->client_fd = client_fd;
                                args->server_fd = server_fd;
                                args->HTTP_request = (char *)malloc(sizeof(char)*len+1);
                                memcpy(args->HTTP_request,HTTP_request,len);
                                if(pthread_create(&u_id,NULL,cache_miss,(void*)args) != 0) {
                                        puts("thread error");
                                        return -1;
                                };
                                pthread_detach(u_id);
                        }
                        else{
                                /*
                                 * Cache hit
                                 */
                                Thread_hit * args = (Thread_hit *)malloc(sizeof(Thread_hit));
                                pthread_mutex_lock(&mutex_sock);
                                clnt_socks[clnt_cnt++]=client_fd;
                                pthread_mutex_unlock(&mutex_sock);
                                args->node = node;
                                args->client_fd = client_fd;
                                if(pthread_create(&t_id,NULL,cache_hit,(void*)args) != 0) {
                                        puts("thread error");
                                        return -1;
                                };
                                pthread_detach(t_id);
                        }
                        print_cache();
                        fputs("Finish send response from server to client\n\n-------------------\n\n",stdout);
                }
                fflush(stdout);
        }
        close(server_fd);
        close(client_fd);
        close(proxy_fd);
        fflush(stdout);
        return 0;
}

void * cache_miss(void*args){
        Thread_miss * vars = (Thread_miss *)args;
        char cache_flag = 0;
        char cycle_flag = 0;
        char HTTP_response[BUF_SIZE];
        int len = 0;
        int content_ptr = 0;
        int content_length = 0;

        memset(HTTP_response,0,BUF_SIZE);

        //Read response server and send it to client.
        pthread_mutex_lock(&mutex_write);
        while((len = read(vars->server_fd,HTTP_response,BUF_SIZE)) >= 0) {
                if(len == 0) {
                        break;
                }

                if(cycle_flag == 0) {
                        proxy_log(vars->HTTP_request,HTTP_response);
                        content_length = content_length_parser(HTTP_response);

                        if(0<content_length&&content_length <= MAX_OBJECT_SIZE) {
                                //Default header size of apache is 8K
                                vars->node->contents = (char *)malloc(sizeof(char) * (content_length+len));
                                vars->node->size = content_length;
                                cache_flag = 1;
                        }
                        cycle_flag = 1;
                }
                if(cache_flag == 1) {
                        memcpy(vars->node->contents+content_ptr, HTTP_response,len);
                        content_ptr += len;
                }

                len = write(vars->client_fd,HTTP_response,len);

                memset(HTTP_response,0,BUF_SIZE);

                if(len == 0) {
                        break;
                }
                close(vars->server_fd);
                close(vars->client_fd);
        }
        pthread_mutex_unlock(&mutex_write);

        if(cache_flag == 1) {
                //Record object into cache.
                pthread_mutex_lock(&mutex_node);
                if(MAX_CACHE_SIZE < (cache->total_size + content_length)) {
                        //Cache is full, proceed delete oldest cache objects.
                        while ((MAX_CACHE_SIZE - cache->total_size)<content_length) {
                                delete_at_last();
                        }
                        insert_at_first(vars->node);
                }else{
                        insert_at_first(vars->node);
                }
                pthread_mutex_unlock(&mutex_node);
        }

        pthread_mutex_lock(&mutex_sock);
        for(int i=0; i<clnt_cnt; i++) // remove disconnected client
        {
                if(vars->client_fd==clnt_socks[i])
                {
                        while(i++<clnt_cnt-1)
                                clnt_socks[i]=clnt_socks[i+1];
                        break;
                }
        }
        clnt_cnt--;
        pthread_mutex_unlock(&mutex_sock);
        return NULL;
}

void * cache_hit(void * args){
        Thread_hit * vars = (Thread_hit *)args;
        fputs("\n------Cache hit-----\n",stdout);
        int len = 0;
        int content_ptr = 0;
        pthread_mutex_lock(&mutex_write);
        while(content_ptr <= vars->node->size) {
                len = write(vars->client_fd,vars->node->contents+content_ptr,BUF_SIZE);
                content_ptr += len;
        }
        pthread_mutex_unlock(&mutex_write);

        pthread_mutex_lock(&mutex_node);
        move_to_first(vars->node);
        pthread_mutex_unlock(&mutex_node);

        pthread_mutex_lock(&mutex_sock);
        for(int i=0; i<clnt_cnt; i++) // remove disconnected client
        {
                if(vars->client_fd==clnt_socks[i])
                {
                        while(i++<clnt_cnt-1)
                                clnt_socks[i]=clnt_socks[i+1];
                        break;
                }
        }
        clnt_cnt--;
        pthread_mutex_unlock(&mutex_sock);
        close(vars->client_fd);
        return NULL;
}
