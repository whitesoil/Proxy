/*
 *
 * Last modified : 2017.12.02
 * Hanyang University
 * Computer Science & Engineering
 * Seon Namkung
 *
 * Http request & response parser.
 *
 */

#ifndef __PARSER__
#define __PARSER__

#define BUF_SIZE 2048
#define SMALL_SIZE 100

int content_length_parser(char * HTTP_response);
char * host_parser(char * HTTP_request);
char * url_parser(char * HTTP_request);
char * date_parser(char * HTTP_response);
void proxy_log(char * HTTP_request, char * HTTP_response);
void http_reconstructor(char * HTTP_request);

/*
 * Parse Content-Length of Http response
 */
int content_length_parser(char * HTTP_response){
        char token[BUF_SIZE];
        char * content_length = (char *)malloc(sizeof(char)*15);

        if(strstr(HTTP_response,"Content-Length: ") != NULL) {
                strcpy(token,strstr(HTTP_response,"Content-Length: "));

                strtok(token,"\r\n");
                strtok(token," ");
                strcpy(content_length,strtok(NULL," "));
        }else{
                return 0;
        }

        if(content_length == NULL) {
                return 0;
        }
        return atoi(content_length);
}


/*
 * Parse Host of Http request
 */
char * host_parser(char * HTTP_request){
        char token[BUF_SIZE];
        if(strstr(HTTP_request,"Host: ") != NULL) {
                strcpy(token,strstr(HTTP_request,"Host: "));
                char * host = (char*)malloc(sizeof(char) * 50);
                strtok(token,"\r\n");
                strtok(token," ");
                strcpy(host,strtok(NULL," "));
                return host;
        }else{
                return NULL;
        }

}

/*
 * Parse URL of Http request
 */
char * url_parser(char * HTTP_request){
        char * url;
        int start=0;
        int end=0;
        int i = 0;

        for(i = 0; HTTP_request[i] != 'H'; i++) {
                if(HTTP_request[i] == ' ' && start == 0) {
                        start = i;
                }
                if(HTTP_request[i] == ' ' && start != 0) {
                        end = i;
                }
        }
        url = (char *)malloc(sizeof(char)*(end-start));
        if((end-start) < 100)
                strncpy(url,HTTP_request+start+1,end-start);
        else
                strncpy(url,HTTP_request+start+1,99);

        return url;
}
/*
 * Parse Date of Http response
 */
char * date_parser(char * HTTP_response){
        char token[BUF_SIZE];
        if(strstr(HTTP_response,"Date: ") != NULL) {
                strcpy(token,strstr(HTTP_response,"Date: "));
                char * date = (char *)malloc(sizeof(char) * 100);
                strtok(token,"\r\n");
                strcpy(date,token);
                return date;
        }else{
                return NULL;
        }

}

/*
 * Reconstruct http request
 * HTTP/1.1 -> HTTP/1.0, Connection: keep-alive -> close
 */
void http_reconstructor(char * HTTP_request){
        int i = 0;
        char * buf = strstr(HTTP_request,"keep-alive");
        char * str = "close     ";
        while(HTTP_request[i] != 'H') {
                i++;
        }
        HTTP_request[i+7] = '0';
        strncpy(buf,str,10);
        return;
}


/*
 * Save Log
 * Format : Date IP Hostname Content-Length
 * File : proxy.log
 */
void proxy_log(char * HTTP_request, char * HTTP_response){
        FILE*fp = fopen("proxy.log","a");

        char * host = host_parser(HTTP_request);
        char * date = date_parser(HTTP_response);
        int size = content_length_parser(HTTP_response);

        if(host != NULL && date != NULL && size != 0) {
                struct hostent* pHostInfo;
                pHostInfo=gethostbyname(host);

                char log[200];
                sprintf(log,"%s %s %s %d\n",date,inet_ntoa(*(struct  in_addr *)pHostInfo->h_addr_list[0]),pHostInfo->h_name,size);
                fprintf(fp,"%s",log);
                fclose(fp);
        }

        return;
}
#endif
