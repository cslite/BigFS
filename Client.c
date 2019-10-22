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

char *FNS_IP;
int FNS_PORT;
int FNS_sfd;

int num_FDS = -1;
char **FDS_IP;
int *FDS_PORT;
int *FDS_sfd;

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
 * Called to check if tmp directory exists & if not, it is created.
 */
int makeTmpDir(){
    char *dir = "tmp";
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
            printf("Connected to FileDataServer%d at %s:%d\n",i+1,FDS_IP[i],FDS_PORT[i]);
        }
    }
    return true;
}

/*
 * Split a file into n parts and return the base file path
 * Suppose base file path is "tmp/myfile" and n = 3, then,
 * parts will be at "tmp/myfile_1" , "tmp/myfile_2" , "tmp_myfile_3"
 */
char *splitFile(char *filePath, int n){
    int rfd = open(filePath,O_RDONLY);
    if(rfd == -1){
        perror("open");
        return NULL;
    }
    int wfd,i;
    long sz;
    long CHUNK_SIZE = (getFileSize(filePath) / (long)n) + ((getFileSize(filePath) % (long)n) ? 1L : 0L);
    char *basename = allocString(31);
    strcat(basename,"tmp/");
    strcat(basename,genRandomName(10));
    for(i=1;i<=n;i++){
        char buf[CHUNK_SIZE+5];
        sprintf(basename,"%.14s_%d",basename,i);
        sz = read(rfd,buf,CHUNK_SIZE);
        wfd = open(basename,O_WRONLY|O_CREAT,0777);
        write(wfd,buf,sz);
        close(wfd);
    }
    basename[14] = '\0';
    close(rfd);
    return basename;
}

char *joinFile(char *baseFilePath, int n){
    int wfd = open(baseFilePath,O_WRONLY|O_CREAT,0777);
    if(wfd == -1){
        perror("open");
        return NULL;
    }
    int rfd,i;
    long sz,CHUNK_SIZE;
    char *filePath = allocString(31);
    for(i=1;i<=n;i++){
        sprintf(filePath,"%s_%d",baseFilePath,i);
        CHUNK_SIZE = getFileSize(filePath);
        if(CHUNK_SIZE == -1){
            perror("open");
            return NULL;
        }
        char buf[CHUNK_SIZE+5];
        rfd = open(filePath,O_RDONLY);
        if(rfd == -1){
            perror("open");
            return NULL;
        }
        sz = read(rfd,buf,CHUNK_SIZE);
        write(wfd,buf,sz);
        close(rfd);
    }
    close(wfd);
    return baseFilePath;
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
    write(FDS_sfd[FDS_idx],buf,strlen(buf));
    //server returns a string
    int sz = read(FDS_sfd[FDS_idx],buf,50);  //ERROR or a Unique ID
    buf[sz] = '\0';
    if(equals(buf,"ERROR"))
        return NULL;
    long ret = sendfile(FDS_sfd[FDS_idx],fd,0,size);
    if(ret == -1){
        perror("sendfile");
        return NULL;
    }
    printf("[Upload, Part%d]: Successfully sent %ld bytes.\n",FDS_idx+1,ret);
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
    write(sockfd,buf,strlen(buf));
    size = read(sockfd,buf,50);
    buf[size] = '\0';
    sscanf(buf,"%ld",&size);
    if(size == -1){
        printf("[Download, Part%d]: ERROR, File Not Found.\n",FDS_idx+1);
        return false;
    }
    strcpy(buf,"READY");
    write(sockfd,buf,strlen(buf));
    int wfd = open(tgtPath,O_WRONLY|O_CREAT,0777);
    char wbuf[TCP_BUFFER_SIZE] = {0};
    long remain_data = size, len;
    while ((remain_data > 0) && ((len = read(sockfd, wbuf, TCP_BUFFER_SIZE)) > 0))
    {
        write(wfd,wbuf,len);
        remain_data -= len;
//        printf("[Download, Part%d]: Received %ld bytes, Remaining %ld bytes\n",FDS_idx+1,len,remain_data);
    }
    printf("[Download, Part%d]: Received %ld bytes for uid = %s\n",FDS_idx+1,size,uid);
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
    write(sockfd,buf,strlen(buf));
    size = read(sockfd,buf,50);
    buf[size] = '\0';
    if(equals(buf,"DONE")){
        printf("[REMOVE, Part%d]: Success.\n",FDS_idx+1);
        return true;
    }
    else{
        printf("[REMOVE, Part%d]: Failure.\n",FDS_idx+1);
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
    }
    char buf[size];
    strcpy(buf,remote_path);
    i = 0;
    while(uidarr[i] != NULL){
        strcat(buf,"#");
        strcat(buf,uidarr[i]);
    }
    size = strlen(buf);
    char tmp[21];
    sprintf(tmp,"PUT %d",size);
    write(FNS_sfd,tmp,strlen(tmp));
    i = read(FNS_sfd,tmp,20);
    tmp[i] = '\0';
    if(equals(tmp,"READY")){
        write(FNS_sfd,buf,size);
        printf("Added Entry to the FileNameServer.\n");
        return true;
    }
    else{
        printf("[ERROR]: Failed adding entry to FileNameServer.\n");
        return false;
    }
}

/*
 * remote_path must be a file
 */
char **getFnsData(char *remote_path){
    int size;
    char buf[TCP_BUFFER_SIZE];
    sprintf(buf,"GET %s",remote_path);
    write(FNS_sfd,buf,strlen(buf));
    size = read(FNS_sfd,buf,TCP_BUFFER_SIZE);
    buf[size] = '\0';
    if(size == 2 && equals(buf,"-1")){
        printf("[ERROR]: File Not Found.\n");
        return NULL;
    }
    char **uidarr = strSplit(buf,'#');
    return uidarr;
}

/*
 * remote_path must be a directory
 */
char **getLs(char *remote_path){
    int size;
    char buf[TCP_BUFFER_SIZE];
    sprintf(buf,"LS %s",remote_path);
    write(FNS_sfd,buf,strlen(buf));
    size = read(FNS_sfd,buf,TCP_BUFFER_SIZE);
    buf[size] = '\0';
    if(size == 2 && equals(buf,"-1")){
        printf("[ERROR]: Directory Not Found.\n");
        return NULL;
    }
    char **lsarr = strSplit(buf,'\n');
    return lsarr;
}

/*
 * remote_path should be a file
 */
int removeFnsEntry(char *remote_path){
    long size = strlen(remote_path) + 10;
    char buf[size];
    sprintf(buf,"DEL %s",remote_path);
    write(FNS_sfd,buf,strlen(buf));
    size = read(FNS_sfd,buf,50);
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

/*
 * This will support directory moving as mv supports it
 */
int moveFnsEntry(char *srcPath, char *destPath){

}

int uploadFile(char *rpath, char *fpath){
    if(rpath == NULL || fpath == NULL)
        return false;
    char *basepath = splitFile(fpath,num_FDS);  //basepath has max size of 30
    if(basepath == NULL)
        return false;
    int sz = strlen(basepath) + 10;
    char ppath[sz]; //part Path
    int i;
    char **uidarr = (char **)(calloc(num_FDS+1,sizeof(char *)));
    uidarr[num_FDS] = NULL;
    int p[num_FDS][2];
    int cpid[num_FDS];
    for(i=0;i<num_FDS;i++){
        pipe(p[i]);
        if((cpid[i] = fork()) == 0){
            close(p[i][0]);
            sprintf(ppath,"%s_%d",basepath,i+1);
            char *tuid = uploadPart(ppath,i);

            if(tuid == NULL){
                char cbuf[30] = {0};
                strcpy(cbuf,"null");
                write(p[i][1],cbuf,4);
                exit(-1);
            }
            else{
                write(p[i][1],tuid,strlen(tuid));
                exit(0);
            }

        }
        close(p[i][1]);
    }
    int partsDone = 0;
    char buf[101] = {0};
    for(i=0;i<num_FDS;i = (i+1)%num_FDS){
        if(partsDone == num_FDS)
            break;
        if(cpid[i] == -1)
            continue;
        int sz = read(p[i][0],buf,100);
        buf[sz] = '\0';
        if(sz == 4 && equals(buf,"null")){
            return false;
        }
        uidarr[i] = allocString(sz);
        strcpy(uidarr[i],buf);
        waitpid(cpid[i],NULL,0);
        partsDone++;
        cpid[i] = -1;
    }
    if(newFnsEntry(rpath,uidarr))
        return true;
    else
        return false;

}

/*
 * Downloads file at rpath to a tmp location and returns the local path
 */
char *downloadFile(char *rpath){
    char **uidarr = getFnsData(rpath);
    if(uidarr == NULL)
        return NULL;
    char *basepath = allocString(30);
    strcpy(basepath,"tmp/");
    strcat(basepath,genRandomName(10));
    int i;
    int cpid[num_FDS];
    for(i=0;i<num_FDS;i++){
        if((cpid[i] = fork()) == 0){
            sprintf(basepath,"%s_%d",basepath,i+1);
            if(downloadPart(uidarr[i],basepath,i))
                exit(0);
            else
                exit(1);
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
    return basepath;
}

int removeFile(char *rpath){
    char **uidarr = getFnsData(rpath);
    if(uidarr == NULL)
        return false;
    int i;
    for(i=0;i<num_FDS;i++){
        removePart(uidarr[i],i);
    }
    removeFnsEntry(rpath);
    return true;
}

int cmd_ls(char *dpath){

}

int cmd_rm(char *fpath, int isRecursive){

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
        //cp in local storage
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
    if(!isRecursive){
        //copy a single file
        if(inBigfs(arg1) && inBigfs(arg2)){
            //both remote
            //TODO: WRITE CODE FOR THIS PART
            printf("[ERROR]: cp only supports copying from/to remote server to/from local storage\n");
        }
        else if(inBigfs(arg1)){
            //download remote file
            char *tmpPath = downloadFile(arg1);
            if(tmpPath == NULL)
                return false;
            if(!cmd_mv(tmpPath,arg2))
                return false;
            return true;
        }
        else{
            //upload file to bigfs
            if(uploadFile(arg2,arg1))
                return true;
            else
                return false;
        }
    }
    else{
        //copy a directory
    }
}







int cmd_cat(char *fpath){
    if(fpath == NULL)
        return false;
    char *localpath = fpath;
    if(inBigfs(fpath)){
        //cat in bigfs
        localpath = allocString(14);
        strcat(localpath,"tmp/");
        strcat(localpath,genRandomName(10));
        if(!cmd_cp(fpath,localpath,false))
            return false;
    }
    //cat in local storage
    if(fork() == 0){
        execlp("cat","cat",localpath,NULL);
        perror("execlp");
        exit(0);
    }
    wait(NULL);
    return true;
}

void execute_cmd(char *cmd){
    cmd = trim(cmd);
    char **cmdarr = strSplit(cmd,' ');
    if(cmdarr == NULL)
        return;
    if(equals(cmdarr[0],"cat")){
        if(cmdarr[1] != NULL && cmd_cat(cmdarr[1]))
            return;
        else
            printf("[ERROR]: Please check the file path.\n");
    }
    else if(equals(cmdarr[0],"cp")){
        if(cmdarr[1] && cmdarr[2] && cmdarr[3] && equals(cmdarr[1],"-r") && cmd_cp(cmdarr[2],cmdarr[3],true))
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
        if(cmdarr[1] != NULL && cmd_ls(cmdarr[1]))
            return;
        else if(cmdarr[1] == NULL && cmd_ls("bigfs"))
            return;
        else
            printf("[ERROR]: There was an error while trying to do ls.\n");
    }
    else if(equals(cmdarr[0],"rm")){
        if(cmdarr[1] && cmdarr[2] && equals(cmdarr[1],"-r") && cmd_rm(cmdarr[2],true))
            return;
        else if(cmdarr[1] && !cmdarr[2] && cmd_rm(cmdarr[1],false))
            return;
        else
            printf("[ERROR]: Unable to delete the given file/folder.\n");
    }
    else if(equals(cmdarr[0],"exit")){
        exit(0);
    }
    else{
        printf("[ERROR]: Command not recognized.\n");
        return;
    }
}



int main() {
    printf("Hello, World!\n");


    srand(time(0));
    makeTmpDir();
    if(!readConfig("../config.txt"))
        return -1;
    createConnection();
    char inp[101] = {0};
//    test_downloadPart();
//    test_removePart();


    while(1){
        fgets(inp,100,stdin);
    }
//    char *bname = splitFile("tmp/ip.png",10);
//    ps(bname);
//
//    bname = joinFile(bname,10);
//    ps(bname);

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