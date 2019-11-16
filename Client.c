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
#define TCP_BUFFER_SIZE 2047
#define MAX_ARRAY_SIZE (1<<20) //1MB Array

//Global Variables
char *FNS_IP;
int FNS_PORT;
int FNS_sfd;

int num_FDS = -1;
char **FDS_IP;
int *FDS_PORT;
int *FDS_sfd;
//for IPs, PORTs, Socket FDs


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

void test_equals(){
    ps("--Testing equals--");
    pi(equals("/tmp/tg1","tmp/tg1"));
    pi(equals(NULL,"tg1"));
    pi(equals("",""));
    pi(equals(" ","np"));
    pi(equals(NULL,NULL));
}

long min(long x, long y){
    if(x < y)
        return x;
    else
        return y;
}

char * trim (char *str) { // remove leading and trailing spaces
    str = strdup(str);
    if(str == NULL)
        return NULL;
    int begin = 0, end = strlen(str) -1, i;
    while (isspace (str[begin])){
        begin++;
    }

    if (str[begin] != '\0'){
        while (isspace (str[end]) || str[end] == '\n'){
            end--;
        }
    }
    str[end + 1] = '\0';

    return str+begin;
}

void test_trim(){
    ps("--Testing Trim--");
    ps(trim("  _tgnp1  "));
    ps(trim("tgnp1"));
    ps(trim("  \n"));
    ps(trim("tgnp1 \n"));
}

/*
 * Generate a random alpha-numeric string of length len
 */
char *genRandomName(int len){
    char *name = (char *)(calloc(len+1,sizeof(char)));
    name[len] = '\0';
    const char cset[] = "abcdefghijklmnopqrstuvwxyz1234567890";    //size 36
    int i,idx;
    for(i=0;i<len;i++){
        idx = rand() % 36;
        name[i] = cset[idx];
    }
    return name;
}

void test_genRandomName(){
    ps("--Testing genRandomName--");
    ps(genRandomName(5));
    ps(genRandomName(1));
    ps(genRandomName(10));
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

void test_numTk(){
    ps("--Testing numTk--");
    pi(numTk("tus/sank/tus/ss/",'/'));
    pi(numTk("hello",' '));
    pi(numTk(" this is a test 5",' '));
}

void socketLost(){
    ps("Connection to one or more servers lost.");
    exit(-1);
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

void test_strSplit(){
    ps("--Testing strSplit--");
    char **arr = strSplit("/np/assgn/a2/q1",'/');
    int i=0;
    while(arr[i] != NULL){
        pi(i);
        ps(arr[i++]);
    }
    arr = strSplit("FDS 102.122.11.12 8080\nFNS 100.1.1.1 8888",'\n');
    i=0;
    while(arr[i] != NULL){
        pi(i);
        ps(arr[i++]);
    }
}

/*
 * Checks whether the given path is in bigfs or local storage
 */
int inBigfs(char *path){
    int len = strlen(path);
    if(len < 5)
        return false;
    else if(len == 5)
        return equals(path,"bigfs");
    char tmp = path[6];
    path[6] = '\0';
    if(equals(path,"bigfs/")){
        path[6] = tmp;
        return true;
    }
    else{
        path[6] = tmp;
        return false;
    }

}

void test_inBigFs(){
    char a[] = "bigfs";
    pi(inBigfs(a));
    char b[] = "bigfs/tg1/np";
    pi(inBigfs(b));
    char c[] = "bgfs";
    pi(inBigfs(c));
}

/*
 * Called to check if a directory exists & if not, it is created.
 */
int makeDir(char *dir){
    struct stat sb;
    if(stat(dir,&sb)== 0 && S_ISDIR(sb.st_mode))
        return true;
    else{
        if(mkdir(dir,0777) == 0)
            return true;
        else{
            perror("mkdir");
            ps(dir);
            return false;
        }
    }
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

void test_getFileSize(){
    pi(getFileSize("tmp/ip.png"));
    pi(getFileSize("tmp/tmp.avi"));
    pi(getFileSize("tmp/tmp2/tg1.txt"));
}

/*
 * An input like /home/tg/netprog will return netprog
 */
char *getDeepestDir(char *path){
    int n = strlen(path);
    int slc = numTk(path,'/');
    int i=0;
    while(slc){
        if(path[i] == '/')
            slc--;
        i++;
    }
    return (path + i);
}

/*
 * Reads the given config file and parse info about FDS and FNS.
 */
int readConfig(char *fname){
    if(fname == NULL){
        printf("[ERROR]: Invalid Argument.\n");
        return false;
    }
    FILE *fp = fopen(fname,"r");
    if(fp == NULL){
        printf("[ERROR]: Cannot open the given config file.\n");
        return false;
    }
    FNS_IP = allocString(20);
    if(fscanf(fp,"%s%d",FNS_IP,&FNS_PORT) == EOF)
        return false;
    if(fscanf(fp,"%d",&num_FDS) == EOF)
        return false;
    FDS_IP = (char **)(calloc(num_FDS,sizeof(char *)));
    FDS_PORT = (int *)(calloc(num_FDS,sizeof(int)));
    for(int i = 0; i < num_FDS; i++){
        FDS_IP[i] = allocString(20);
        if(fscanf(fp,"%s%d",FDS_IP[i],&FDS_PORT[i]) == EOF)
            return false;
    }
    return true;
}

void test_readConfig(){
    int ret = readConfig("../config.txt");
    pi(ret);
    if(ret){
        ps(FNS_IP);
        pi(FNS_PORT);
        int i =0;
        pi(num_FDS);
        for(i=0;i<num_FDS;i++){
            pi(i);
            ps(FDS_IP[i]);
            pi(FDS_PORT[i]);
        }

    }
}

void printConfigFileFormat(){
    printf("\n## CONFIG FILE FORMAT ##\n\n");
    printf("<FNS IP> <FNS PORT>\n");
    printf("<k = Number of FDS>\n");
    printf("<FDS1 IP> <FDS1 PORT>\n");
    printf("<FDS2 IP> <FDS2 PORT>\n");
    printf("..\n");
    printf("..\n");
    printf("..\n");
    printf("<FDSk IP> <FDSk PORT>\n\n");
}

int createConnection(){
    if(num_FDS == -1)
        return false;
    FDS_sfd = (int *)(calloc(num_FDS,sizeof(int)));
    struct sockaddr_in serveraddr;
    //Connect to FNS
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(FNS_IP);
    serveraddr.sin_port = htons(FNS_PORT);
    FNS_sfd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(connect(FNS_sfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) == -1){
        perror("FileNameServer");
        return false;
    }
    else{
        printf("Connected to FileNameServer at %s:%d\n",FNS_IP,FNS_PORT);
    }
    char errmsg[50];
    for(int i=0;i<num_FDS;i++){
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = inet_addr(FDS_IP[i]);
        serveraddr.sin_port = htons(FDS_PORT[i]);
        FDS_sfd[i] = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(connect(FDS_sfd[i],(struct sockaddr *)&serveraddr,sizeof(serveraddr)) == -1){
            sprintf(errmsg,"connect %s:%d ",FDS_IP[i],FDS_PORT[i]);
            perror(errmsg);
            return false;
        }
        else{
            printf("Connected to FileDataServer%d at %s:%d\n",i,FDS_IP[i],FDS_PORT[i]);
        }
    }
    return true;
}

/*
 * Split a file into 1MB blocks and return the base file path
 * Suppose base file path is "tmp/myfile" and size is 3MB, then,
 * parts will be at "tmp/myfile_0" , "tmp/myfile_1" , "tmp_myfile_2"
 */
char *splitFile(char *filePath, int *numParts){
    int rfd = open(filePath,O_RDONLY);
    if(rfd == -1){
        perror("open");
        return NULL;
    }
    int wfd,i;
    long sz;
//    long CHUNK_SIZE = (getFileSize(filePath) / (long)n) + ((getFileSize(filePath) % (long)n) ? 1L : 0L);
    char *basename = allocString(31);
    strcat(basename,"tmp/");
    strcat(basename,genRandomName(10));
    i = 0;
    long toRead = getFileSize(filePath);
    char buf[MAX_ARRAY_SIZE];
    while(toRead > 0){
        sprintf(basename,"%.14s_%d",basename,i);
        sz = read(rfd,buf,MAX_ARRAY_SIZE);
        if(sz == 0)
            break;
        wfd = open(basename,O_WRONLY|O_CREAT,0666);
        write(wfd,buf,sz);
        close(wfd);
        toRead -= sz;
        i++;
    }
    *numParts = i;

//    for(i=1;i<=n;i++){
//        char buf[MAX_ARRAY_SIZE];
//        sprintf(basename,"%.14s_%d",basename,i);
//        long toRead = CHUNK_SIZE;
//        wfd = open(basename,O_WRONLY|O_CREAT,0666);
//        while(toRead > 0){
//            sz = read(rfd,buf,MAX_ARRAY_SIZE);
//            if(sz == 0)
//                break;
//            write(wfd,buf,sz);
//            toRead -= sz;
//        }
//        close(wfd);
//    }
    basename[14] = '\0';
    close(rfd);
    return basename;
}

char *joinFile(char *baseFilePath, int numParts){
    int wfd = open(baseFilePath,O_WRONLY|O_CREAT,0666);
    if(wfd == -1){
        perror("open");
        return NULL;
    }
    int rfd,i;
    long sz,CHUNK_SIZE;
    char *filePath = allocString(31);
    for(i=0;i<numParts;i++){
        sprintf(filePath,"%s_%d",baseFilePath,i);
        CHUNK_SIZE = getFileSize(filePath);
        if(CHUNK_SIZE == -1){
            perror("open");
            return NULL;
        }
        long toRead = CHUNK_SIZE;
        char buf[MAX_ARRAY_SIZE];
        rfd = open(filePath,O_RDONLY);
        if(rfd == -1){
            perror("open");
            return NULL;
        }
        while(toRead > 0){
            sz = read(rfd,buf,MAX_ARRAY_SIZE);
            write(wfd,buf,sz);
            toRead -= sz;
        }
        close(rfd);
    }
    close(wfd);
    free(filePath);
    return baseFilePath;
}

void test_joinFile_splitFile(){
    int np;
    char *bname = splitFile("tmp/ip.png",&np);
    ps(bname);

    bname = joinFile(bname,np);
    ps(bname);
}

/*
 * Uploads the given file to FDS on given idx and returns the retrieved uid
 */
char *uploadPart(char *fpath,int FDS_idx){
    long size = getFileSize(fpath);
    if(size == -1)
        return NULL;
    int fd = open(fpath,O_RDONLY);
    char *buf = allocString(51);
    sprintf(buf,"PUT %ld",size);
    if(write(FDS_sfd[FDS_idx],buf,strlen(buf))==0)
        return NULL;
    //server returns a string
    int sz = read(FDS_sfd[FDS_idx],buf,50);  //ERROR or a Unique ID
//    pi(sz);
    if(sz == 0)
        return NULL;
    buf[sz] = '\0';
    if(equals(buf,"ERROR")){
        ps("FDS returned ERROR.");
        return NULL;
    }

    long ret = sendfile(FDS_sfd[FDS_idx],fd,0,size);
    if(ret == -1){
        perror("sendfile");
        return NULL;
    }
    printf("[Part Upload to FDS%d]: Successfully sent %ld bytes. UID = %s\n",FDS_idx,ret,buf);
    //server returns a unique id for the uploaded file
    close(fd);
    return buf;
}

void test_uploadPart(){
        char *uup;
    if((uup=uploadPart("tmp/tmp.avi",0))!=NULL){
        ps(uup);

    }
    else{
        ps("Error in UploadPart");
    }
}

int downloadPart(char *uid, char *tgtPath,int FDS_idx){
    long size;
    int sockfd = FDS_sfd[FDS_idx];
    char buf[51] = {0};
    sprintf(buf,"GET %s",uid);
    if(write(sockfd,buf,strlen(buf))==0)
        return false;
    size = read(sockfd,buf,50);
    if(size == 0)
        return false;
    buf[size] = '\0';
    sscanf(buf,"%ld",&size);
    if(size == -1){
        printf("[Part Download from FDS%d]: ERROR, File Not Found.\n",FDS_idx);
        return false;
    }
    strcpy(buf,"READY");
    if(write(sockfd,buf,strlen(buf))==0)
        return false;
    int wfd = open(tgtPath,O_WRONLY|O_CREAT,0666);
    char wbuf[TCP_BUFFER_SIZE] = {0};
    long remain_data = size, len;
    while ((remain_data > 0) && ((len = read(sockfd, wbuf, TCP_BUFFER_SIZE)) > 0))
    {
        write(wfd,wbuf,len);
        remain_data -= len;
//        printf("[Download, Part%d]: Received %ld bytes, Remaining %ld bytes\n",FDS_idx+1,len,remain_data);
    }
    printf("[Part Download from FDS%d]: Received %ld bytes for uid = %s\n",FDS_idx,size,uid);
    close(wfd);
    return true;
}

void test_downloadPart(){
    if(!downloadPart("p51c05l20v10a0u","tmp/tgfile1",0))
        ps("Error in DownloadPart");
}

int removePart(char *uid, int FDS_idx){
    long size;
    int sockfd = FDS_sfd[FDS_idx];
    char buf[51] = {0};
    sprintf(buf,"DEL %s",uid);
    if(write(sockfd,buf,strlen(buf)) == 0)
        return false;
    size = read(sockfd,buf,50);
    if(size == 0)
        return false;
    buf[size] = '\0';
    if(equals(buf,"DONE")){
        printf("[Part Remove from FDS%d]: Success.\n",FDS_idx);
        return true;
    }
    else{
        printf("[Part Remove from FDS%d]: Failure.\n",FDS_idx);
        return false;
    }
}

void test_removePart(){
    if(!removePart("n7d5eme0708fip6",0))
        ps("Error in RemovePart");
}

/*
 * remote_path should be a file
 */
int newFnsEntry(char *remote_path, char **uidarr){
    int i=0;
    int size = 5;
    size += strlen(remote_path);
    while(uidarr[i] != NULL){
        size += strlen(uidarr[i]) + 1;
        i++;
    }
    char buf[size];
    strcpy(buf,remote_path);
    i = 0;
    while(uidarr[i] != NULL){
        strcat(buf,"#");
        strcat(buf,uidarr[i++]);
    }
    size = strlen(buf);
    char tmp[21];
    sprintf(tmp,"PUT %d",size);
    if(write(FNS_sfd,tmp,strlen(tmp))==0)
        socketLost();
    i = read(FNS_sfd,tmp,20);
    if(i==0)
        socketLost();
    tmp[i] = '\0';
    long toWrite = size;
    long sz;
    char *bufp = buf;
    if(equals(tmp,"READY")){
        while(toWrite > 0){
            if((sz=write(FNS_sfd,bufp,min(toWrite,TCP_BUFFER_SIZE)))==0)
                socketLost();
            bufp += sz;
            toWrite -= sz;
        }
        printf("Added Entry to the FileNameServer.\n");
        return true;
    }
    else{
        printf("[ERROR]: Failed adding entry to FileNameServer.\n");
        return false;
    }
}

void test_newFnsEntry(){
    char *uids = "ghfduoncki#ddjfuiodfg#dfnherthgu";
    char **ua = strSplit(uids,'#');
    if(!newFnsEntry("bigfs/tgtest1/tg2/hello.py",ua))
        ps("Error in newFnsEntry");
    else
        ps("No error");
}

/*
 * remote_path must be a file
 */
char **getFnsData(char *remote_path){
    long size;
    char buf[MAX_ARRAY_SIZE];
    sprintf(buf,"GET %s",remote_path);
    if(write(FNS_sfd,buf,strlen(buf)) == 0)
        socketLost();

    size = read(FNS_sfd,buf,50);
    if(size == 0)
        socketLost();
    buf[size] = '\0';
    sscanf(buf,"%ld",&size);
    if(size == -1){
        printf("[ERROR]: File Not Found.\n");
        return NULL;
    }
    strcpy(buf,"READY");
    if(write(FNS_sfd,buf,strlen(buf))==0)
        socketLost();
    long toRead = size;
    long sz;
    char *bufp = buf;
    while(toRead > 0){
        sz = read(FNS_sfd,bufp,TCP_BUFFER_SIZE);
        if(sz == 0)
            socketLost();
        bufp += sz;
        toRead -= sz;
    }
    bufp[0] = '\0';
    char **uidarr = strSplit(buf,'#');
    return uidarr;
}

void test_getFnsData(){
    char **uidarr = getFnsData("bigfs/tgtest1/tg2/hello.py");
    int i = 0;
    while(uidarr[i] != NULL){
        pi(i);
        ps(uidarr[i++]);
    }
}

/*
 * remote_path must be a directory
 */
char **getLs(char *remote_path){
    int size;
    char buf[TCP_BUFFER_SIZE];
    sprintf(buf,"LS %s",remote_path);
    if(write(FNS_sfd,buf,strlen(buf)) == 0)
        socketLost();
    size = read(FNS_sfd,buf,TCP_BUFFER_SIZE);
    if(size == 0)
        socketLost();
    buf[size] = '\0';
    if(size == 2 && equals(buf,"-1")){
        printf("[ERROR]: Directory Not Found.\n");
        return NULL;
    }
    char *tbuf = trim(buf);
    char **lsarr = strSplit(tbuf,'\n');
    return lsarr;
}

void test_getLs(){
    char **larr = getLs("../");
    int i = 0;
    while(larr[i] != NULL){
        pi(i);
        ps(larr[i++]);
    }
}

/*
 * remote_path should be a file
 */
int removeFnsEntry(char *remote_path){
    long size = strlen(remote_path) + 10;
    char buf[size];
    sprintf(buf,"DEL %s",remote_path);
    if(write(FNS_sfd,buf,strlen(buf))==0)
        socketLost();
    size = read(FNS_sfd,buf,50);
    if(size == 0)
        socketLost();
    buf[size] = '\0';
    if(equals(buf,"DONE")){
        printf("[REMOVE %s]: Success.\n",remote_path);
        return true;
    }
    else{
        printf("[REMOVE %s]: Failure.\n",remote_path);
        return false;
    }
}

void test_removeFnsEntry(){
    if(!removeFnsEntry("bigfs/tgtest1/tg2/hello.py"))
        ps("Error in removeFnsEntry");
}

/*
 * This will support directory moving as mv supports it
 * NOT TESTED YET
 * TODO: TEST THIS FUNCTION
 */
int moveFnsEntry(char *srcPath, char *destPath){
    long size = strlen(srcPath) + strlen(destPath) + 10;
    char buf[size];
    sprintf(buf,"MOV %s %s",srcPath,destPath);
    if(write(FNS_sfd,buf,strlen(buf))==0)
        socketLost();
    size = read(FNS_sfd,buf,50);
    if(size == 0)
        socketLost();
    buf[size] = '\0';
    if(equals(buf,"DONE")){
        printf("[MOVE]: Success.\n");
        return true;
    }
    else{
        printf("[MOVE]: Failure.\n");
        return false;
    }
}

int cmd_rm(char *fpath, int isRecursive);

int uploadFile(char *rpath, char *fpath){
    if(rpath == NULL || fpath == NULL)
        return false;
    int numParts;
    char *basepath = splitFile(fpath,&numParts);  //basepath has max size of 30
    if(basepath == NULL)
        return false;
    int sz = strlen(basepath) + 10;
    char ppath[sz]; //part Path
    int i;
    char **uidarr = (char **)(calloc(numParts+1,sizeof(char *)));
    uidarr[numParts] = NULL;
    int p[num_FDS][2];
    int cpid[num_FDS];
    for(i=0;i<num_FDS;i++){
        pipe(p[i]);
        if((cpid[i] = fork()) == 0){
            close(p[i][0]);
            int k;
            for(k=i;k<numParts;k += num_FDS){
                sprintf(ppath,"%s_%d",basepath,k);
                char *tuid = uploadPart(ppath,i);
                if(tuid == NULL){
                    ps("UploadPart has returned null");
                    char cbuf[30] = {0};
                    strcpy(cbuf,"null");
                    write(p[i][1],cbuf,4);
                    exit(-1);
                }
                else{
                    if(k!=i)
                        write(p[i][1],"$",1);
                    write(p[i][1],tuid,strlen(tuid));
                }
            }
            exit(0);
        }
        close(p[i][1]);
    }
    int partsDone = 0;
    char buf[MAX_ARRAY_SIZE];
    for(i=0;i<num_FDS;i = (i+1)%num_FDS){
        if(partsDone == num_FDS)
            break;
        if(cpid[i] == -1)
            continue;
        int sz;
        char *bufp = buf;
        do{
            sz = read(p[i][0],bufp,MAX_ARRAY_SIZE);
            bufp = bufp + sz;
        }while(sz > 0);
        bufp[0] = '\0';
        if(equals(buf,"null")){
            return false;
        }
        char **fdsuid = strSplit(buf,'$');
        int j = 0;
        for(int k=i;k<numParts;k+=num_FDS){
            uidarr[k] = fdsuid[j++];
        }
//        uidarr[i] = allocString(sz+1);
//        strcpy(uidarr[i],buf);
        waitpid(cpid[i],NULL,0);
        partsDone++;
        cpid[i] = -1;
    }
    int failed = false;
    if(!newFnsEntry(rpath,uidarr)){
        ps("Failed while trying to add entry to FNS.");
        i = 0;
        while(uidarr[i] != NULL){
            removePart(uidarr[i],(i%num_FDS));
            i++;
        }
        failed = true;
    }
    //remove all temporarily generated files
    for(i=0;i<numParts;i++){
        sprintf(ppath,"%s_%d",basepath,i);
        cmd_rm(ppath,false);
    }
    if(failed)
        return false;
    return true;

}

/*
 * Downloads file at rpath to a tmp location and returns the local path
 */
char *downloadFile(char *rpath){
    char **uidarr = getFnsData(rpath);
    if(uidarr == NULL)
        return NULL;
    int i=0;
    while(uidarr[i] != NULL)
        i++;
    int numParts = i;
    char *basepath = allocString(30);
    strcpy(basepath,"tmp/");
    strcat(basepath,genRandomName(10));
    int cpid[num_FDS];
    for(i=0;i<num_FDS;i++){
        if((cpid[i] = fork()) == 0) {
            char ppath[31];
            for(int k=i;k<numParts; k+= num_FDS){
                sprintf(ppath, "%s_%d", basepath, k);
                if (!downloadPart(uidarr[k], ppath, i))
                {
                    ps("Download Part failed.");
                    ps(uidarr[k]);
                    exit(1);
                }
            }
            exit(0);
        }
    }
    int partsDone = 0;
    for(i=0;i<num_FDS;i = (i+1)%num_FDS){
        if(partsDone == num_FDS)
            break;
        if(cpid[i] == -1)
            continue;
        int status;
        waitpid(cpid[i],&status,0);
        if(WEXITSTATUS(status) == 1){
            return NULL;
        }
        partsDone++;
        cpid[i] = -1;
    }
    if((basepath = joinFile(basepath,numParts)) == NULL){
        ps("Error joining parts.");
    }
    //remove the temporarily generated parts
    for(i=0;i<numParts;i++){
        char tmppath[50] = {0};
        sprintf(tmppath, "%s_%d", basepath, i);
        cmd_rm(tmppath,false);
    }
    return basepath;
}

char **getLsLocal(char *dpath){
    int p[2];
    pipe(p);
    if(fork() == 0){
        close(p[0]);
        close(1);
        dup2(p[1],1);
        execlp("ls","ls","-F",dpath,NULL);
        perror("execlp");
        exit(0);
    }
    close(p[1]);
    wait(NULL);
    char buf[TCP_BUFFER_SIZE] = {0};
    int nr = read(p[0],buf,TCP_BUFFER_SIZE);
    if(nr == 0)
        return NULL;
    buf[nr] = '\0';
    close(p[0]);
    return strSplit(trim(buf),'\n');
}

int removeFile(char *rpath){
    char **uidarr = getFnsData(rpath);
    if(uidarr == NULL)
        return false;
    int i=0;
    //remove all the parts from FDSs.
    while(uidarr[i] != NULL){
        removePart(uidarr[i],(i%num_FDS));
        i++;
    }
    //remove the entry from the FNS
    removeFnsEntry(rpath);
    return true;
}

int cmd_ls(char *dpath){
    char **larr;
    if(!inBigfs(dpath))
        larr = getLsLocal(dpath);
    else
        larr = getLs(dpath);
    if(larr == NULL)
        return false;
    int i = 0;
    while(larr[i] != NULL){
        printf("%s\n",larr[i++]);
    }
    return true;
}

int cmd_rm(char *fpath, int isRecursive){
    if(!inBigfs(fpath)){
        //rm in local storage
        if(fork() == 0){
            execlp("rm","rm","-r",fpath,NULL);
            perror("execlp");
            exit(0);
        }
        wait(NULL);
        return true;
    }
    if(!isRecursive)
        return removeFile(fpath);
    else{
        //remove a directory
        char **larr = getLs(fpath);
        if(larr == NULL)
            return false;
        int i=0;
        while(larr[i] != NULL){
            int n = strlen(larr[i]);
            char *nfpath = allocString(strlen(fpath) + n+1);
            strcpy(nfpath,fpath);
            strcat(nfpath,larr[i]);
            if(larr[i][n-1] == '/'){
                //it is a directory
                cmd_rm(nfpath,true);
            }
            else{
                //it is a file
                cmd_rm(nfpath,false);
            }
            free(nfpath);
            i++;
        }
        removeFnsEntry(fpath);
        return true;
    }
}

int cmd_mv(char *arg1, char *arg2){
    if(!inBigfs(arg1) && !inBigfs(arg2)){
        //mv in local storage
        if(fork() == 0){
            execlp("mv","mv",arg1,arg2,NULL);
            perror("execlp");
            exit(0);
        }
        wait(NULL);
        return true;
    }
    if(inBigfs(arg1) && inBigfs(arg2)){
        //only the entry in the FileNameServer needs to be moved
        if(moveFnsEntry(arg1,arg2))
            return true;
        else
            return false;
    }
    else{
        printf("[ERROR]: Feature not supported.\n");
        return false;
    }
}


int cmd_cp(char *arg1, char *arg2, int isRecursive){
    if(!inBigfs(arg1) && !inBigfs(arg2)){
        //cp in local storage (both files are local)
        if(fork() == 0){
            if(isRecursive)
                execlp("cp","cp","-r",arg1,arg2,NULL);
            else
                execlp("cp","cp",arg1,arg2,NULL);
            perror("execlp");
            exit(0);
        }
        wait(NULL);
        return true;
    }
    if(inBigfs(arg1) && inBigfs(arg2)){
        //both remote
        printf("[ERROR]: currently cp only supports copying from/to remote server to/from local storage\n");
        return false;
    }
    if(!isRecursive){
        //copy a single file
        if(inBigfs(arg1)) {
            //download remote file to tmpPath
            char *tmpPath = downloadFile(arg1);
            if (tmpPath == NULL)
                return false;
            //move it to the required location
            if (!cmd_mv(tmpPath, arg2)) {
                //remove the file, if move fails and report the error
                cmd_rm(tmpPath,false);
                return false;
            }
            return true;
        }
        else{
            //upload file to bigfs
            if(uploadFile(arg2,arg1))
                return true;
            else{
                ps("Upload file failed.");
                return false;
            }
        }
    }
    else{
        //copy a directory
        if(inBigfs(arg1))
            if (!makeDir(arg2))
                return false;
        int arg1len = strlen(arg1);
        if (arg1[arg1len - 1] != '/') {
            //this folder along with its content will be copied
            char *ddir = getDeepestDir(arg1);
            char *narg1 = allocString(arg1len + 5);
            char *narg2 = allocString(strlen(arg2) + strlen(ddir) + 5);
            strcpy(narg1, arg1);
            strcat(narg1, "/");
            strcpy(narg2, arg2);
            strcat(narg2, ddir);
            strcat(narg2, "/");
            return cmd_cp(narg1, narg2, true);
        }
        char **larr;
        if(inBigfs(arg1)) {
            //download recursively
            larr = getLs(arg1);
        }
        else{
           larr = getLsLocal(arg1);
        }
        if(larr == NULL)
            return false;
        int i = 0;
        while (larr[i] != NULL) {
            int n = strlen(larr[i]);
            char *narg1 = allocString(strlen(arg1) + n + 1);
            strcpy(narg1, arg1);
            strcat(narg1, larr[i]);
            char *narg2 = allocString(strlen(arg2) + n + 1);
            strcpy(narg2, arg2);
            strcat(narg2, larr[i]);
            if (larr[i][n - 1] == '/') {
                //it is a directory
                cmd_cp(narg1, narg2, true);
            } else {
                //it is a file
                cmd_cp(narg1, narg2, false);
            }
            free(narg1);
            free(narg2);
            i++;
        }
        return true;
    }
}

/*
 * cat is directly run for local files
 * for remote files, they are downloaded temporarily, cat is run, then temporary file is deleted.
 */
int cmd_cat(char *fpath){
    if(fpath == NULL)
        return false;
    char *localpath = fpath;
    if(inBigfs(fpath)){
        //cat in bigfs
        localpath = downloadFile(fpath);
        if(localpath == NULL)
            return false;
    }
    //cat in local storage
    if(fork() == 0){
        execlp("cat","cat",localpath,NULL);
        perror("execlp");
        exit(0);
    }
    wait(NULL);
    //remove the temporarily downloaded file
    if(inBigfs(fpath)){
        cmd_rm(localpath,false);
        free(localpath);
    }
    return true;
}

void execute_cmd(char *cmd){
    //remove leading and trailing spaces from the cmd
    cmd = trim(cmd);
    //split the cmd based on spaces
    char **cmdarr = strSplit(cmd,' ');
    if(cmdarr == NULL)
        return;
    if(equals(cmdarr[0],"cat")){
        if(cmdarr[1] && !cmdarr[2] && cmd_cat(cmdarr[1]))
            return;
        else
            printf("[ERROR]: Please check the file path.\n");
    }
    else if(equals(cmdarr[0],"cp")){
        if(cmdarr[1] && cmdarr[2] && cmdarr[3] && !cmdarr[4] && equals(cmdarr[1],"-r") && cmd_cp(cmdarr[2],cmdarr[3],true))
            return;
        else if(cmdarr[1] && cmdarr[2] && !cmdarr[3] && cmd_cp(cmdarr[1],cmdarr[2],false))
            return;
        else
            printf("[ERROR]: Please check the entered paths.\n");
    }
    else if(equals(cmdarr[0],"mv")){
        if(cmdarr[1] && cmdarr[2] && !cmdarr[3] && cmd_mv(cmdarr[1],cmdarr[2]))
            return;
        else
            printf("[ERROR]: Please check the entered paths.\n");
    }
    else if(equals(cmdarr[0],"ls")){
        if(cmdarr[1] && !cmdarr[2] && cmd_ls(cmdarr[1]))
            return;
        else if(!cmdarr[1] && cmd_ls("bigfs"))
            return;
        else
            printf("[ERROR]: There was an error while trying to do ls.\n");
    }
    else if(equals(cmdarr[0],"rm")){
        if(cmdarr[1] && cmdarr[2] && !cmdarr[3] && equals(cmdarr[1],"-r") && cmd_rm(cmdarr[2],true))
            return;
        else if(cmdarr[1] && !cmdarr[2] && cmd_rm(cmdarr[1],false))
            return;
        else
            printf("[ERROR]: Unable to delete the given file/folder.\n");
    }
    else if(equals(cmdarr[0],"exit")){
        printf("Exiting...\n");
        exit(0);
    }
    else{
        printf("[ERROR]: Command not recognized.\n");
        return;
    }
}



int main(int argc, char *argv[]) {
    if(argc != 2){
        printf("You need to give path to the config file as a command line argument.\n");
        return -1;
    }
    char *cfgfile = argv[1];
    srand(time(0));
    //make a directory to store temporary files
    makeDir("tmp");
    //read the config file
    if(!readConfig(cfgfile)){
        ps("Error reading the config file.");
        printConfigFileFormat();
        return -1;
    }
    //create connection with all the servers
    if(!createConnection()){
        ps("[ERROR]: Problem connecting with server(s).");
        return -1;
    }
    char inp[101] = {0};

    while(1){
        printf("\n $ ");
        fgets(inp,100,stdin);
        execute_cmd(inp);
    }

//    test_downloadPart();
//    test_removePart();
//    test_newFnsEntry();
//    test_getFnsData();
//    test_removeFnsEntry();
//    test_getLs();
//    test_equals();
//    test_numTk();
//    test_genRandomName();
//    test_strSplit();
//    test_trim();
//    test_inBigFs();
//    test_getFileSize();
//    test_readConfig();

    return 0;
}