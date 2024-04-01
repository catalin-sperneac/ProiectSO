#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//functie pentru parcurgerea recusiva a unui director pentru a obtine informatii despre acesta
//path - calea catre director
//depth - nivelul de adancime in director
void listFiles(char *path,int depth) 
{
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;
    //deschidem directorul
    if (!(dir = opendir(path))) 
    {
        perror("Eroare deschidere director!\n");
        exit(0);
    }
    //citim directorul folosind functia readdir
    while ((entry = readdir(dir))!=NULL) 
    {
        char filepath[1024];
        sprintf(filepath, "%s/%s", path, entry->d_name);
        //ignoram intrarile "." si ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        for (int i = 1; i < depth; ++i)
        {
            printf("\t");
        }
        printf("|-- %s\n", entry->d_name);
        //obtinem informatii despre fisierul/directorul curent
        if (stat(filepath, &file_info) == -1) 
        {
            perror("Eroare obtinere informatii fisier!\n");
            exit(0);
        }
        //daca intrarea este un director, se va apela recursiv functia listFiles cu un nivel de adancime mai mare
        if (S_ISDIR(file_info.st_mode)) 
        {
            listFiles(filepath,depth+1);
        }
    }
    //inchidem directorul
    closedir(dir);
}

int main(int argc,char *argv[]) 
{
    if(argc!=2) 
    {
        perror("Eroare numar de argumente!\n");
        exit(0);
    }
    printf("%s\n",argv[1]);
    listFiles(argv[1],1);
    return 0;
}