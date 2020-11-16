#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2,"usage: sleep ...\n");
        exit();
    }
    int time = atoi(argv[1]);
    // printf("%d",atoi(argv[0]));
    // printf("%d",time);
    sleep(time);
    fprintf(2,"program is running...\n");
    exit();
}
//argv[0]代表程序运行的全名,argv[1]代表执行程序名的第一个字符串
//argc用来统计命令行参数个数，默认为1
