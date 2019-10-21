#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define false 0
#define true 1

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
    const char cset[] = "abcdefghijklmnopqrstuvwxyz1234567890_";    //size 37
    int i,idx;
    for(i=0;i<len;i++){
        idx = rand() % 37;
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
    arr = strSplit("netprog",' ');
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



int cmd_cp(char *arg1, char *arg2, int isRecursive){

}

int cmd_mv(char *arg1, char *arg2, int isRecursive){

}

int cmd_ls(char *dpath){

}

int cmd_rm(char *fpath, int isRecursive){

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
        if(cmdarr[1] && cmdarr[2] && cmdarr[3] && equals(cmdarr[1],"-r") && cmd_mv(cmdarr[2],cmdarr[3],true))
            return;
        else if(cmdarr[1] && cmdarr[2] && !cmdarr[3] && cmd_mv(cmdarr[1],cmdarr[2],false))
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
    else{
        printf("[ERROR]: Command not recognized.\n");
        return;
    }
}



int main() {
//    printf("Hello, World!\n");


    srand(time(0));
    makeTmpDir();

//    test_equals();
//    test_numTk();
//    test_genRandomName();
//    test_strSplit();
//    test_trim();

    return 0;
}