#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct stat buffer;

int main(int argc, char *argv[])
{
    if(argc!=2)
    {
        perror("Eroare numar de argumente!\n");
        exit(0);
    }
    if(lstat(argv[1],&buffer)!=0)
    {
        perror("Eroare director!\n");
        exit(0);
    }
    if(S_ISDIR(buffer.st_mode)==0)
    {
        printf("Este director!\n");
    }
    return 0;
}