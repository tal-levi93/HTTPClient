#include  <stdio.h>
#include  <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>    	          /* Internet address structures */
#include <sys/socket.h>   	          /* socket interface functions  */
#include <netdb.h>         	          /* host to IP resolution     */
#include <sys/types.h> 

#define HTTPSTARTSIZE 7
#define DEFUALTPORT 80
#define GET_REQUEST 0
#define POST_REQUEST 1 
#define BUF_LEN 50

typedef struct URL{
    char* host;
    char* path;             // url struct 
    int port;
    int idx;
}URL;

typedef struct request{
    URL* url;
    char* values;
    int kindOfRequest;      // request struct 
    char* PostText;
    char* theRequest;
}request;

int predictNumOfArgs(int argc , char** argv){

    int i=0;
    int c = 2; // for url , and for client.
    for(i=0;i<argc;i++)
    {
        if(strcmp(argv[i] , "-r") == 0 && i+1<argc) {
            int temp = 2; // for -r and the number of args.
            if(atoi(argv[i+1]) == 0 && strcmp(argv[i+1] , "0") != 0)
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                exit(EXIT_FAILURE);
            }
            else
            {
                temp += atoi(argv[i+1]);
                c += temp;    
            }
            
            
        }
        if(strcmp(argv[i] , "-p") == 0 && i+1<argc){
            c+=2;
        }
    }
    return c;
}

void freeRequest(request *req){

    if(req->url->host != NULL)
    {
        free(req->url->host);
    }
    if(req->url->path != NULL){
        free(req->url->path);
    }
    if(req->url != NULL){
        free(req->url);
    }
    if(req->values!= NULL){
        free(req->values);
    }
    if(req->PostText != NULL){
        free(req->PostText);
    }
    if(req->theRequest != NULL ){
        free(req->theRequest);
    }
    if(req != NULL){
        free(req);
    }
}

void seperateURL(char** argv , int argc , URL* url , request* req){ // seperate the url for port host and path method. if url is not found exit.
    char urlStart[] = "http://";
    char* ptrToUrl = NULL;
    char* tempHost = NULL;
    char* tempPort = NULL;
    int urlSize = 0;
    int i = 0;
    int c = 0;
    int pathFilesize = 0;
    int idx = 0;
    int correct = 0;
    int jump = 0;
    for(i=1;i<argc;i++)
    {   
        if(strcmp(argv[i] , "-r")  == 0)
        {
            if(i>=argc)
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                req->theRequest = (char*)malloc((sizeof(char)));
                req->PostText = (char*)malloc((sizeof(char)));
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            jump = atoi(argv[i+1]);
             
            if(jump == 0 && strcmp(argv[i+1] , "0") != 0)
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                req->theRequest = (char*)malloc((sizeof(char)));
                req->PostText = (char*)malloc((sizeof(char)));
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            if(jump+i+1 < argc)
            {
                i=jump+i+1;
                continue;
            }
            else
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                req->theRequest = (char*)malloc((sizeof(char)));
                req->PostText = (char*)malloc((sizeof(char)));
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            

        }
        if(strcmp(argv[i] , "-p")  == 0){

            if(i>=argc)
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                req->theRequest = (char*)malloc((sizeof(char)));
                req->PostText = (char*)malloc((sizeof(char)));
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            jump = 1;
            if(i+jump<argc)
            {
                i+=jump;
                continue;
            }
            else
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                req->theRequest = (char*)malloc((sizeof(char)));
                req->PostText = (char*)malloc((sizeof(char)));
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            break;
            
            


        }

        ptrToUrl = strstr(argv[i],urlStart); // compare the start of http:// 
        if(ptrToUrl == NULL)continue;
        correct = 1;
        int j = 0;
        idx = i;
        while(ptrToUrl[j+HTTPSTARTSIZE] != '/' && ptrToUrl[j+HTTPSTARTSIZE] != '\0' )// copy the addr 
        {   
            j++;
        }
        urlSize = j;
        url->idx = i;
        pathFilesize = strlen(argv[i]) - urlSize - HTTPSTARTSIZE;
        if(pathFilesize < 0)pathFilesize=0;
        tempHost = (char*)malloc((urlSize+1)*sizeof(char));
        if(tempHost==NULL)                     
        {
            printf("Error! memory not allocated.");
            exit(1);
        }
        j=HTTPSTARTSIZE;
        for (i = 0; i < urlSize && (*(ptrToUrl + i +j) != '\0'); i++) // copy the addr without http:// 
        {
            tempHost[i] = ptrToUrl[i+j];
        }
        tempHost[i] = '\0';
        i=0;
        while(tempHost[i] != ':' && tempHost[i] != '\0')// checking if port is mentiond in url. 
        {
            c++;
            i++;
        }
        if(c == urlSize)// if port is not mentioned in URL 
        {
            int urlFinalSize = c+1;
            url->host = (char*)malloc((urlFinalSize)*sizeof(char));
            if(url->host==NULL)                     
            {
                printf("Error! memory not allocated.");
                exit(1);
            }
            strcpy(url->host , tempHost);
            url->port = DEFUALTPORT;
            free(tempHost);
        }
        else // if port is mentiond 
        {
            int diff = urlSize - i;// checking how many digis port has
            if(diff == 1)
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                req->theRequest = (char*)malloc((sizeof(char)));
                req->PostText = (char*)malloc((sizeof(char)));
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            i++;
            tempPort = (char*)malloc(sizeof(char)*(diff));
            if(tempPort==NULL)                     
            {
                printf("Error! memory not allocated.");
                exit(1);
            }
            for (j = 0; j < urlSize && (*(tempHost + j + i) != '\0'); j++)
            {
                tempPort[j] = tempHost[i+j];
            }
            tempPort[j] = '\0';
            url->port = atoi(tempPort);
            if(url->port < 0)
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                req->theRequest = (char*)malloc((sizeof(char)));
                req->PostText = (char*)malloc((sizeof(char)));
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            urlSize = urlSize-diff;
            url->host = (char*)malloc((urlSize+100)*sizeof(char));
            if(url->host==NULL)                     
            {
                printf("Error! memory not allocated.");
                exit(1);
            }
            for (j = 0; j < urlSize && (*(tempHost + j) != '\0'); j++)
            {
                url->host[j] = tempHost[j];
            }
            url->host[j] = '\0';
            free(tempHost);
            free(tempPort);
            }
            if(pathFilesize == 0)
            {
                url->path = NULL;
                url->path = (char*)malloc((2)*sizeof(char));
                if(url->path==NULL)                     
                {
                    printf("Error! memory not allocated.");
                    exit(1);
                }
                url->path[0] = '/';
                url->path[1] = '\0';
            }
            else
            {
            url->path = (char*)malloc((pathFilesize+1)*sizeof(char));
            if(url->path==NULL)                     
            {
                printf("Error! memory not allocated.");
                exit(1);
            }
            int rest = strlen(argv[idx]) - pathFilesize;
            for (j = 0; j < pathFilesize && (*(argv[idx] + j) != '\0')  ; j++)
            {
                url->path[j] = argv[idx][rest+j];
            
            }
            url->path[j] = '\0';
            
            } 
    }
    if (correct == 0)
    {
        printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
        req->theRequest = (char*)malloc((sizeof(char)));
        req->PostText = (char*)malloc((sizeof(char)));
        freeRequest(req);
        exit(EXIT_FAILURE);
    }
};
void buildValues(char** argv , int argc , request* req)
{
    int i=0;
    int j=0;
    int k=0;
    int valSize = 0;
    char* tempVal;
    int sizeOfValuesReq = 0;
    req->values = NULL;
    
    for(i=1;i<argc;i++)
    {
        
        if(strcmp(argv[i],"-r") != 0)continue; // cheking if there is values in args 
        if(i+1 == argc)
        {
            printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
            req->theRequest = (char*)malloc((sizeof(char)));
            req->PostText = (char*)malloc((sizeof(char)));
            freeRequest(req);
            exit(EXIT_FAILURE);
        }
        valSize = atoi(argv[i+1]);//convert number of values to int
        if(valSize == 0 && strcmp(argv[i+1],"0") != 0)
        {
            printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
            req->theRequest = (char*)malloc((sizeof(char)));
            req->PostText = (char*)malloc((sizeof(char)));
            freeRequest(req);
            exit(EXIT_FAILURE);
        }
        if(i+valSize+1 >= argc)
        {
            printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
            req->theRequest = (char*)malloc((sizeof(char)));
            req->PostText = (char*)malloc((sizeof(char)));
            freeRequest(req);
            exit(EXIT_FAILURE);
        }
        for(j=0;j<valSize;j++)
        {
            sizeOfValuesReq += strlen(argv[i+2+j]);// checking the size of all the values .
        }
        int valuesIDX = i+2;
        tempVal = (char*)malloc((sizeOfValuesReq+valSize+1)*sizeof(char));
        if(tempVal==NULL)                     
        {
            printf("Error! memory not allocated.");
            exit(1);
        }
        tempVal[0] = '\0';
        for(j=0;j<valSize;j++) // checks if the values is illegal case where there is no = in the middle of val is not legal.
        {   
            int ok = 1;
            for(k=0;k<strlen(argv[valuesIDX+j]) ; k++)
            {
                if(k==0 || k == strlen(argv[valuesIDX+j])) // if there is = in the start of a value or in the end.
                {
                    if(argv[valuesIDX+j][k] == '=')
                    {
                        ok = 0;
                        break;
                    }
                }
            }
            if(ok == 0)
            { 
                
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                req->theRequest = (char*)malloc((sizeof(char)));
                req->PostText = (char*)malloc((sizeof(char)));
                free(tempVal);
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            strcat(tempVal , argv[valuesIDX+j]);
            if(j+1 >= valSize)break;
            strcat(tempVal , "&");
        }
        tempVal[sizeOfValuesReq+valSize] = '\0';
        req->values = tempVal;
    
        break;
    }
    
     
}

void buildRequest(request* req){ // building the request 
    char get[4] = "GET\0";
    char http[9] = "HTTP/1.0\0";
    char post[5] = "POST\0"; 
    char host[6] = "Host:";
    int getRequestmin = 50; // getsize + http/1.0 size + host ?
    int request =0;
    if(req->values != NULL)
    {
        request = strlen(req->values)+strlen(req->url->path)+getRequestmin + strlen(req->url->host);
        
    }
    else
    {
        request = strlen(req->url->path)+getRequestmin + + strlen(req->url->host);
    }
    if(req->kindOfRequest == POST_REQUEST)
    {
        request += strlen(req->PostText);
    }
    req->theRequest = (char*)malloc((request+121)*sizeof(char));
    if(req->theRequest==NULL)                     
    {
        printf("Error! memory not allocated.");
        exit(1);
    }
    *(req->theRequest) = '\0';
    if(req->kindOfRequest == GET_REQUEST)
    {
        strcat(req->theRequest , get);
        strcat(req->theRequest , " ");
        strcat(req->theRequest , req->url->path);
    }
    else
    {
        strcat(req->theRequest , post);
        strcat(req->theRequest , " ");
        strcat(req->theRequest , req->url->path);
    }
    if(req->values != NULL)
    {
        strcat(req->theRequest , "?");
        strcat(req->theRequest , req->values);
        strcat(req->theRequest , " ");    
    }
    else
    {
        strcat(req->theRequest , " ");
    }
      
    strcat(req->theRequest , http); 
    strcat(req->theRequest , "\r\n");
    strcat(req->theRequest , host);
    strcat(req->theRequest , " "); 
    strcat(req->theRequest , req->url->host);
    if(req->kindOfRequest == GET_REQUEST)
    {
        strcat(req->theRequest , "\r\n\r\n");

    }
    else
    {
        strcat(req->theRequest , "\n");
        strcat(req->theRequest , "Content-Length:");
        char length[10];
        sprintf(length, "%d", (int)strlen(req->PostText));
        strcat(req->theRequest , length);
        strcat(req->theRequest , "\r\n\r\n");
        strcat(req->theRequest , req->PostText);
    }
       
    req->theRequest[strlen(req->theRequest)] = '\0';
}






int main(int argc , char* argv[])
{
    //-----------------------------------------BUILD REQUEST AND HOST--------------------------------------------------------------------
    request* req = (request*)malloc(sizeof(request));
    if(req==NULL)                     
    {
        printf("Error! memory not allocated.");
        exit(1);
    }
    URL* url = (URL*)malloc(sizeof(URL));
    if(url==NULL)                     
    {
        printf("Error! memory not allocated.");
        exit(1);
    }
    req->url = url;
    int numOfArgs = predictNumOfArgs(argc , argv);
    if (numOfArgs != argc)
    {
        
        printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
        exit(EXIT_FAILURE);
    }
    seperateURL(argv , argc , url , req);
    buildValues(argv , argc , req);
    int i = 0;
    int sizeOfTextForPost = 0;
    req->PostText = NULL;
    for(i=0;i<argc;i++)
    {
        if(strcmp(argv[i],"-p") == 0)
        {
            if(i+1 >= argc)
            {
                
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            if(strcmp(argv[i+1],"-r") == 0)
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            if(i+1 == req->url->idx)
            {
                printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
                freeRequest(req);
                exit(EXIT_FAILURE);
            }
            req->kindOfRequest = POST_REQUEST;
            sizeOfTextForPost = strlen(argv[i+1]);
            req->PostText = (char*)malloc((sizeOfTextForPost+1)*sizeof(char));
            strcpy(req->PostText , argv[i+1]);
            break;
        }
        req->kindOfRequest = GET_REQUEST;
    }
    buildRequest(req);
    printf("HTTP request =\n%s\nLEN = %d\n", req->theRequest, (int)strlen(req->theRequest));
    //-------------------------------------------------------------------------------------------------------------------------//
    //-----------------------------------------------CONNECT TO SERVER---------------------------------------------------------//

    struct sockaddr_in serv_addr;
    struct hostent *server;
    int rc;
    int sockfd;
    sockfd = socket(AF_INET , SOCK_STREAM , 0); // 
    if(sockfd<0)
    {
         fprintf(stderr , "ERROR , Socket failed \n");
    }
    server = gethostbyname(req->url->host);
    if(server == NULL)
    {
        fprintf(stderr , "ERROR , no such host\n");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_port = htons(req->url->port);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_family = AF_INET;
    rc = connect(sockfd, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (rc < 0)
    {
        fprintf(stderr , "ERROR , connection faild\n");
    }
    write(sockfd , req->theRequest , strlen(req->theRequest)+1);
    char answerBuf[BUF_LEN];
    rc = read(sockfd , answerBuf , BUF_LEN+1);
    answerBuf[BUF_LEN] = '\0';
    int size =0;
    while(rc > 0)
    {
        printf("\n%s" , answerBuf);
        rc = read(sockfd , answerBuf , BUF_LEN+1);
        size += rc;
        answerBuf[BUF_LEN] = '\0';
    }
    printf("\n Total received response bytes: %d\n",size); 
    freeRequest(req);
    close(sockfd);
    return 0; 
}
