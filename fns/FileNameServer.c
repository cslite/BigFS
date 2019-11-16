#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<sys/sendfile.h>

#define false 0
#define true 1
#define BASE_DIR "bigfs/"
#define TCP_BUFFER_SIZE 2047
#define MAX_CLIENTS 20
#define MAX_ARRAY_SIZE (1<<20)

//for debug printing
void pt(int x){
    fprintf(stderr,"F%d\n",x);
}
void pi(int x){
    fprintf(stderr,"%d\n",x);
}
void ps(char *str){
    if(str)
        fprintf(stderr,"%s\n",str);
    else
        fprintf(stderr,"null\n");
}

int max(int a, int b){
    return (a>b)?a:b;
}

int min(int a, int b){
    return (a<b) ? a: b;
}

void someErrorExit(){
    printf("[ERROR]: Some Error Occurred.\n");
    exit(-1);
}

char *allocString(int size){
    char *cstr = (char *)(calloc(size+1,sizeof(char)));
    return cstr;
}

/*
 * Tests whether strings s1 and s2 are equal
 */
int equals(char *s1, char *s2){
    if(s1 == NULL && s2 == NULL)
        return true;
    else if(s1 == NULL || s2 == NULL)
        return false;
    else
        return (strcmp(s1,s2) == 0);
}

/*
Count the number of occurrences of tk in str
*/
int numTk(char *str,char tk){
    int i;
    int len = strlen(str);
    int cnt = 0;
    for(i=0;i<len;i++)
        if(str[i] == tk)
            cnt++;
    return cnt;
}

void convertToNonBlocking(int fd){
    int val = fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,val|O_NONBLOCK);
}

/*
 * Split a string and put into an array based on the given delimiter
 */
char **strSplit(char *str, char tk){
    if(equals(str,"") || equals(str,NULL))
        return NULL;
    char tkn[] = {tk};
    int sz = numTk(str,tk) + 2;
    char **arr = (char **)(calloc(sz,sizeof(char *)));
    char *tmpstr = strdup(str);
    int i = 0;
    char *token;
    while((token = strsep(&tmpstr,tkn)))
        arr[i++] = token;
    arr[i] = NULL;
    return arr;
}

/*
 * Returns the size of the given file
 * Returns -1 if the file can't be opened.
 */
long getFileSize(char *file){
    int fd = open(file,O_RDONLY);
    if(fd == -1)
        return -1;
    long size = lseek(fd,0,SEEK_END);
    close(fd);
    return size;
}

/*
 * Called to check if tmp directory exists & if not, it is created.
 */
int makeBigfsDir(){
    char *dir = BASE_DIR;
    struct stat sb;
    if(stat(dir,&sb)== 0 && S_ISDIR(sb.st_mode))
        return true;
    else{
        if(mkdir(dir,0777) == 0)
            return true;
        else{
            perror("mkdir");
            return false;
        }
    }
}
int makeDir(char *dir){
    struct stat sb;
    if(stat(dir,&sb)== 0 && S_ISDIR(sb.st_mode))
        return true;
    else{
        if(mkdir(dir,0777) == 0)
            return true;
        else{
            perror("mkdir");
            return false;
        }
    }
}

/*
 * Send the file data on the the given fpath to the socket on sockfd
 * toWrite stores the size of the file to be sent
 */
int sendData(int sockfd, char *fpath, long toWrite){
    int rfd = open(fpath,O_RDONLY);
    if(rfd == -1)
        return false;
    int val = fcntl(sockfd,F_GETFL);
    fcntl(sockfd,F_SETFL,val & (~O_NONBLOCK));

    sendfile(sockfd,rfd,0,toWrite);

    fcntl(sockfd,F_SETFL,val);
    printf("Sent data at %s\n",fpath);
    return true;
}

void checkAndMakePath(char *fpath){
    char tmp;
    int n = strlen(fpath);
    for(int i=0;i<n;i++){
        if(fpath[i] == '/'){
            fpath[i] = '\0';
            makeDir(fpath);
            fpath[i] = '/';
        }
    }
}

/*
 * Get file from the socket on sockfd and store it on local storage
 */
int recvData(int sockfd, long toRead){
    //assuming sockfd is already set to non-blocking
    char *fpath;
    long nread;
    int val = fcntl(sockfd,F_GETFL);
    fcntl(sockfd,F_SETFL,val & (~O_NONBLOCK));
    char buf[MAX_ARRAY_SIZE];
    char *bufp = buf;
    while(toRead > 0){

        if((nread = read(sockfd,bufp,min(toRead,TCP_BUFFER_SIZE))) < 0){
            perror("read");
        }
        else if(nread == 0){
            ps("EOF on socket");
            return false;
        }
        else{
            bufp += nread;
            toRead -= nread;
//                printf("[uid = %s]: Received %ld bytes, Remaining %ld bytes\n",uid,nread,toRead);
        }
    }
    bufp[0] = '\0';
    fcntl(sockfd,F_SETFL,val);
    char **datarr = strSplit(buf,'#');
    fpath = datarr[0];
    checkAndMakePath(fpath);
    int wfd = open(fpath,O_WRONLY|O_CREAT,0666);
    if(wfd == -1){
        perror("open");
    }
    char *wbuf = buf+1+strlen(fpath);
    write(wfd,wbuf,strlen(wbuf));
    close(wfd);
    printf("Added new data at %s\n",fpath);
    return true;
}

int execute_cmd(int sockfd, char *cmd){
    char tmp = cmd[3];
    cmd[3] = '\0';
    if(equals(cmd,"PUT")){
        cmd[3] = tmp;
        long size;
        cmd = cmd+4;
        sscanf(cmd,"%ld",&size);
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(sockfd,&wset);
        select(sockfd+1,NULL,&wset,NULL,NULL);
        if(FD_ISSET(sockfd,&wset)){
            char tbuf[] = "READY";
            write(sockfd,tbuf,strlen(tbuf));
            if(!recvData(sockfd,size))
                ps("Error Receiving data.");
            return true;
        } else
            return false;
    }
    else if(equals(cmd,"GET")){
        cmd[3] = tmp;
        char *fpath = cmd+4;
        long sz = getFileSize(fpath);
        char buf[20] = {0};
        sprintf(buf,"%ld",sz);
        fd_set wset,rset;
        FD_ZERO(&wset);
        FD_SET(sockfd,&wset);
        select(sockfd+1,NULL,&wset,NULL,NULL);
        if(FD_ISSET(sockfd,&wset)){
            write(sockfd,buf,strlen(buf));
        }
        if(sz == -1)
            return false;
        FD_ZERO(&rset);
        FD_SET(sockfd,&rset);
        select(sockfd+1,&rset,NULL,NULL,NULL);
        if(FD_ISSET(sockfd,&rset)){
            int nr = read(sockfd,buf,19);
            buf[nr] = '\0';
            if(equals(buf,"READY")){
                if(!sendData(sockfd,fpath,sz))
                    ps("Error Sending data.");
            }
        }
        return true;

    }
    else if(equals(cmd,"DEL")){
        cmd[3] = tmp;
        char *fpath = cmd+4;
        if(fork() == 0){
            execlp("rm","rm","-r",fpath,NULL);
            perror("execlp");
            exit(0);
        }
        wait(NULL);
        char buf[20] = {0};
        strcpy(buf,"DONE");
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(sockfd,&wset);
        select(sockfd+1,NULL,&wset,NULL,NULL);
        if(FD_ISSET(sockfd,&wset)){
            write(sockfd,buf,strlen(buf));
        }
        printf("Removed entries at %s\n",fpath);
        return true;
    }
    else if(equals(cmd,"MOV")){
        cmd[3] = tmp;
        char **cmdarr = strSplit(cmd,' ');
        char *srcpath = cmdarr[1];
        char *dstpath = cmdarr[2];
        if(fork() == 0){
            execlp("mv","mv",srcpath,dstpath,NULL);
            perror("execlp");
            exit(0);
        }
        wait(NULL);
        char buf[20] = {0};
        strcpy(buf,"DONE");
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(sockfd,&wset);
        select(sockfd+1,NULL,&wset,NULL,NULL);
        if(FD_ISSET(sockfd,&wset)){
            write(sockfd,buf,strlen(buf));
        }
        printf("Moved entries at %s to %s\n",srcpath,dstpath);
        return true;
    }
    else if(equals(cmd,"LS ")){
        cmd[3] = tmp;
        char *fpath = cmd+3;
        int p[2];
        pipe(p);
        if(fork() == 0){
            close(1);
            close(p[0]);
            dup2(p[1],1);
            execlp("ls","ls","-F",fpath,NULL);
            perror("execlp");
            exit(0);
        }
        close(p[1]);
        wait(NULL);

        char buf[TCP_BUFFER_SIZE] = {0};
        int nr = read(p[0],buf,TCP_BUFFER_SIZE);
        if(nr == 0){
            sprintf(buf,"-1");
            nr = 2;
        }
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(sockfd,&wset);
        select(sockfd+1,NULL,&wset,NULL,NULL);
        if(FD_ISSET(sockfd,&wset)){
            write(sockfd,buf,nr);
        }
        printf("Sent ls data for %s\n",fpath);
        return true;
    }
    else{
        cmd[3] = tmp;
        printf("[ERROR]: Invalid Command.\n");
        ps(cmd);
    }
}

void runServer(int port){
    int i,maxi,maxfd,listenfd,nready,connfd,sockfd,nread;
    int client[MAX_CLIENTS];
    fd_set rset,rset0;
    struct sockaddr_in cliaddr, servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    bind(listenfd,(struct sockaddr*) &servaddr, sizeof(servaddr));
    listen(listenfd,MAX_CLIENTS);
    printf("Server running on port %d\n",port);
    maxfd = listenfd;
    maxi = -1;
    for(i=0; i<MAX_CLIENTS;i++)
        client[i] = -1;
    FD_ZERO(&rset0);
    FD_SET(listenfd,&rset0);
    int loopover = 0;
    while(!loopover){
        rset = rset0;
        nready = select(maxfd+1,&rset,NULL,NULL,NULL);
        if(FD_ISSET(listenfd,&rset)){
            int clilen = sizeof(cliaddr);
            connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
            convertToNonBlocking(connfd);
            for(i=0;i<MAX_CLIENTS;i++){
                if(client[i] == -1){
                    client[i] = connfd;
                    break;
                }
            }
            if(i == MAX_CLIENTS){
                ps("Too Many Clients.\n");
                close(connfd);
            }
            printf("Connected to client %s\n",inet_ntoa(cliaddr.sin_addr));
            FD_SET(connfd,&rset0);
            if(connfd > maxfd)
                maxfd = connfd;
            if(i > maxi && i != MAX_CLIENTS)
                maxi = i;
            if(--nready <= 0)
                continue;
        }
        for(i=0;i<=maxi;i++){

            if((sockfd = client[i]) == -1)
                continue;
            if(FD_ISSET(sockfd,&rset)){
                //this socket is available for reading
                char buf[TCP_BUFFER_SIZE] = {0};
                if((nread = read(sockfd,buf,TCP_BUFFER_SIZE)) == 0){
                    close(sockfd);
                    printf("Closing Connection.\n");
                    FD_CLR(sockfd,&rset0);
                    client[i] = -1;
                }
                else{
                    buf[nread] = '\0';
                    if(!execute_cmd(sockfd,buf))
                        printf("Error in executing command\n");
                }
                if(--nready <= 0)
                    break;
            }
        }

    }
}

int main(int argc, char *argv[]) {
//    printf("Hello, World!\n");
    int port;
    if(argc == 2)
        sscanf(argv[1],"%d",&port);
    else
        port = 5555;
    makeBigfsDir();
    runServer(port);

    return 0;
}