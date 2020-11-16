#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char *cmd[MAXARG];
    char buf[MAXARG][MAXARG];
    int i;
    for (i = 0;i<argc-1;++i){
        cmd[i] = argv[i+1];
    }
    char bufread[512];
    int fd;
    while((fd  = read(0,bufread,sizeof(bufread))) != 0){
        if (fd < 0){
            printf("xargs error\n");
            exit();
        }
        char *pointer = bufread;
        int x = 0;
        int y = 0;
        for (;*pointer;++pointer){
            if (*pointer == ' ' || *pointer == '\n'){
                buf[x][y] = '\0';
                x++;
                y=0;
            }
            else
            {
                buf[x][y++] = *pointer;
            }
        }
        buf[x][y] = '\0';
        for (i=0;i<x;++i){
            cmd[argc-1+i] = buf[i];
        }
        if (fork()){
            wait();
        }else
        {
            exec(cmd[0],cmd);
        }
    }
    exit();
}