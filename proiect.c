#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// functie pentru deschiderea fisierului 
int openFile(char *path, char *file) 
{
    char snapshotPath[1024];
    sprintf(snapshotPath, "%s/%s", path, file);
    int fd = creat(snapshotPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd == -1) 
    {
        perror("Eroare deschidere fisier snapshot.txt pentru scriere!\n");
        exit(1);
    }
    return fd;
}

//functie pentru parcurgerea recursiva a unui director pentru a obtine informatii despre acesta
//path - calea catre director
//depth - nivelul de adancime in director
void listFiles(char *path, char *file, int depth, int fd) 
{
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;
    //deschidem directorul
    if (!(dir = opendir(path))) 
    {
        perror("Eroare deschidere director!\n");
        exit(1);
    }
    //citim directorul folosind functia readdir
    while ((entry = readdir(dir)) != NULL) 
    {
        char filepath[1024];
        sprintf(filepath, "%s/%s", path, entry->d_name);
        //ignoram intrarile "." si ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
        {
            continue;
        }
        char buffer[1024] = "";
        for (int i = 1; i < depth; ++i) 
        {
            strcat(buffer, "|  ");
        }
        strcat(buffer, "|_ ");
        strcat(buffer, entry->d_name);
        strcat(buffer, "\n");
        if ((write(fd, buffer, strlen(buffer))) < 0) 
        {
            perror("Eroare write\n");
            exit(1);
        }
        //obtinem informatii despre fisierul/directorul curent
        if (stat(filepath, &file_info) == -1) 
        {
            perror("Eroare obtinere informatii fisier!\n");
            exit(1);
        }
        //daca intrarea este un director, se va apela recursiv functia listFiles cu un nivel de adancime mai mare
        if (S_ISDIR(file_info.st_mode)) 
        {
            listFiles(filepath, file, depth + 1, fd);
        }
    }
    //inchidem directorul
    closedir(dir);
}

//verificam daca argumentele sunt directoare
int verifyDirectory(char *dir)
{
    struct stat info;
    if (stat(dir, &info) == -1) 
    {
        perror("Eroare obtinere informatii argument!\n");
        exit(1);
    }
    if(S_ISDIR(info.st_mode))
    {
        return 1;
    }
    return 0;
}

//verificam daca exista argumente identice
int verifyArguments(char *arg[],int n)
{
    for(int i=1;i<n;i++)
    {
        for(int j=i+1;j<n;j++)
        {
            if(strcmp(arg[i],arg[j])==0)
            {
                return 0;
            }
        }
    }
    return 1;
}

int main(int argc, char *argv[]) 
{
    if ((argc<3 || argc>11) && verifyArguments(argv,argc)==0) 
    {
        perror("Eroare argumente!\n");
        exit(1);
    }
    if(verifyDirectory(argv[argc-1])==0)
    {
        perror("Eroare argumente!\n");
        exit(1);
    }
    for(int i=1;i<argc-1;i++)
    {
        char snapshot[1024] = "snapshot_";
        strcat(snapshot, argv[i]);
        strcat(snapshot, ".txt");
        int fd = openFile(argv[argc-1], snapshot);
        if(verifyDirectory(argv[i])==1)
        {
           listFiles(argv[i],snapshot,1,fd);
        }
        close(fd);
    }
    return 0;
}