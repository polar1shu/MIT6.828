#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXARGS 32

char whitespace[] = " \t\r\n\v";
void execPipe(char *argv[],int argc);

int
getcmd(char *buf, int nbuf)
{
  fprintf(2, "@");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

void redirect(int k,int pd[]){
    close(k);
    dup(pd[k]);
    close(pd[0]);
    close(pd[1]);
}

void getArgs(char *cmd, char *argv[], int *argc){

    int i=0;//读取的buf
    int j=0;//记录有多少个参数

    //字符串不结束就开始检测
    for (i=0;cmd[i]!='\0';++i) {
        //将每个命令的单词都放到argv中
        //寻找cmd[i]是否为空格,，如果是就跳过
        //找到不是空格的地方，就会停下，如echo hello world
        //找到单词，指向开始位置
        while(strchr(whitespace, cmd[i]))
            i++;

        argv[j++]=cmd+i;//指针是移动的,指向不同的位置

        //找到下一个空格
        while(!strchr(whitespace, cmd[i]))
            i++;

        cmd[i]='\0';//将空格设为\0
    }

    argv[j]=0;//结束命令
    *argc=j;
}
void runcmd(char *argv[],int argc){
    // | 重定向
    for (int i=1;i<argc;++i){
        if (!strcmp(argv[i],"|")){
            execPipe(argv,argc);
        }
    }

    for (int i=1;i<argc;++i){
        if (!strcmp(argv[i],">")){
            close(1);
            open(argv[i+1],O_CREATE|O_WRONLY);
            argv[i]=0;
        }
        if (!strcmp(argv[i],"<")){
            close(0);
            open(argv[i+1],O_RDONLY);
            argv[i]=0;
        }
    }
    exec(argv[0],argv);
}

void execPipe(char *argv[],int argc){
    int i=0;
    while(i<argc){
        if (!strcmp(argv[i],"|")){
            argv[i]=0;
            break;
        }
        ++i;
    }
    int pd[2];
    pipe(pd);
    if (fork()){
        redirect(0,pd);
        runcmd(argv+i+1,argc-i-1);
    }else
    {
        redirect(1,pd);
        runcmd(argv,i);
    }
}

int main()
{
    char buf[1024];
    while(getcmd(buf, sizeof(buf)) >= 0){
        if (!fork()){
            int argc = 0;
            char *argv[MAXARGS];
            getArgs(buf,argv,&argc);
            runcmd(argv,argc);
        }
        wait(0);
    }
    exit(0);
}
//一定要注意开的数组大小。。  太坑了。。
//注意char *buff  char *p[]的用法
//比如p[0]指向buff位置0,p[1]指向buff位置5,输出p[0]的内容就是buffer0-4的内容

// #include "kernel/types.h"
// #include "user/user.h"
// #include "kernel/fcntl.h"

// #define MAXARGS 32

// char whitespace[] = " \t\r\n\v";

// char *trim(char *buf){
//     char *temp = buf;
//     while(*temp) temp++;//计算长度
//     while(*buf == ' ') *(buf++) = '\0';//去除左边的空格
//     while(*(--temp) == ' ');//计算右边没有空格的位置
//     *(temp+1) = '\0';//
//     return buf;
// }

// //替换左边的，如果没有就返回NULL，或者返回找到的字符串
// char *parsetoken(char *buf, char token)
// {
//     while (*buf != '\0' && *buf != token)
//         buf++;
//     if (*buf == '\0'){
//         return 0;
//     }
//     *buf = '\0';
//     return buf+1;
// }

// void redirect(int k,int pd[])
// {
//     close(k);
//     dup(pd[k]);
//     close(pd[0]);
//     close(pd[1]);
// }

// void run_cmd(char *cmd){

//     cmd = trim(cmd);//去除空格

//     char buf[MAXARGS][MAXARGS];
//     char *run[MAXARGS];
//     for (int i=0;i<MAXARGS;++i)
//         run[i]=buf[i];//指向cmd
//     int argc = 0;
//     char *argv = buf[argc];//指向cmd的第一行
//     int input_pos,output_pos;
//     input_pos = output_pos = 0;
//     for (char *temp=cmd;*temp;++temp){
//         if (*temp == ' '){
//             *argv='\0';
//             argc++;
//             argv = buf[argc];//指向buf 

//         }else
//         {
//             if (*temp == '<')
//                 input_pos = argc+1;
//             if (*temp == '>')
//                 output_pos = argc+1;
//             *argv++ = *temp;//取出argv等于temp后，然后再增加1，指向下一个
//         }    
//     }

//     *argv = '\0';
//     argc++;
//     run[argc]=0;//最后位置为0

//     if (input_pos){
//         close(0);
//         open(run[input_pos],O_RDONLY);
//     }

//     if (output_pos){
//         close(1);
//         open(run[output_pos],O_WRONLY | O_CREATE);
//     }

//     char *run1[MAXARGS];
//     int argc1=0;

//     for(int i=0;i<argc;++i){
//         if (i == input_pos - 1) i+=2;
//         if (i == output_pos - 1) i+=2;
//         run1[argc1++] = run[i];
//     }

//     run1[argc1] = 0;
//     if (fork()){
//         wait(0);
//     }
//     else
//     {
//         exec(run1[0],run1);
//     }

// }

// void exec_cmd(char *cmd,char *parsecmd)
// {
//     if (cmd){
//         int pd[2];
//         pipe(pd);

//         if (!fork()){
//             if (parsecmd){
//                 redirect(1,pd);
//             }
//             run_cmd(cmd);       
//         }
//         else if (!fork())
//         {
//             if (parsecmd){
//                 redirect(0,pd);
//                 cmd = parsecmd;
//                 parsecmd = parsetoken(cmd,'|');
//                 exec_cmd(cmd,parsecmd);
//             }     
//         }
//         close(pd[0]);
//         close(pd[1]);
//         wait(0);
//         wait(0);
//     }
//     exit(0);
// }

// int 
// main(int argc,char *arvgv[])
// {
//     char buf[1024];
//     while (1)
//     {
//         fprintf(1,"@");
//         memset(buf,0,1024);
//         gets(buf,1024);
//         if (buf[0] == 0) // EOF
//             exit(0);

//         *strchr(buf,'\n') = '\0';
        
//         if (fork()){
//             wait(0);
//         }
//         else
//         {
//             char *cmd = buf;
//             char *parsecmd = parsetoken(cmd,'|');
//             exec_cmd(cmd,parsecmd);
//         }
//     }
//     exit(0);
// }   
// //坑，数组100*100居然能爆了
