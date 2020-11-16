#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void gerenate()
{
    int i;
    for (i = 2;i<35;++i){
        write(1, &i, sizeof(i));
    }
}
void writeLeft(int n, int pd[]){
    close(n);
    dup(pd[n]);
    close(pd[0]);
    close(pd[1]);
}
void checkPrimes(int prime){
    int n;
    while(read(0, &n, sizeof(n)) > 0){
        if (n % prime !=0){
            write(1, &n, sizeof(n));
        }
    }
}
void printPrimes(){
    int pd[2];
    int n;
    if (read(0, &n, sizeof(n))){
        printf("prime %d\n",n);
        pipe(pd);
        if (fork()){
            // printf("child 1 %d\n",getpid());
            writeLeft(0,pd);
            // printf("child 2 %d\n",getpid());
            printPrimes();
        }else
        {
            // printf("child 3 %d\n",getpid());
            writeLeft(1,pd);
            // printf("child 4 %d\n",getpid());
            checkPrimes(n);
        }
    }
}

int
main(int argc, char *argv[])
{
    int pd[2];
    pipe(pd);
    if (fork()){
        // printf("main parent 1\n");
        writeLeft(0,pd);
        // printf("main parent 2\n");
        printPrimes();
    }else
    {
        // printf("main child 5\n");
        writeLeft(1,pd);
        // printf("main child 6\n");
        gerenate();
    }
    exit(); 
}
//pipe[1]写入  pipe[0]写出
//使用read进行读pipe，如果缓冲区没有数，则read一直等待写端口写，或者写端口关闭，read返回0
//write1,read0