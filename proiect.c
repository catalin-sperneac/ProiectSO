#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

//FILE *snapshot;

// functie pentru deschiderea fisierului 
int openFile(char *path,char *file) 
{
    char snapshotPath[1024];
    sprintf(snapshotPath, "%s/%s", path,file);
    //int fd=open(snapshotPath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    int fd=creat(snapshotPath,S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    if (fd == -1) 
    {
        perror("Eroare deschidere fisier snapshot.txt pentru scriere!\n");
        exit(0);
    }
    return fd;
}

//functie pentru parcurgerea recusiva a unui director pentru a obtine informatii despre acesta
//path - calea catre director
//depth - nivelul de adancime in director
void listFiles(char *path,char *file,int depth) 
{
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;
    //deschidem fisierul
    int fd=openFile(path,file);
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
        char *buffer="";
        for (int i = 1; i < depth; ++i)
        {
            //char *buffer="";
            strcat(buffer,"|  ");
            //write(fd,buffer,strlen(buffer));
            //fprintf(snapshot,"|  ");
        }
        //char *buffer2="";
        strcat(buffer,"|_ ");
        strcat(buffer,entry->d_name);
        strcat(buffer,"\n");
        if((write(fd,buffer,strlen(buffer)))<0)
        {
            perror("Eroare write\n");
            exit(0);
        }
        //fprintf(snapshot,"|_ %s\n",entry->d_name);
        //obtinem informatii despre fisierul/directorul curent
        if (stat(filepath, &file_info) == -1) 
        {
            perror("Eroare obtinere informatii fisier!\n");
            exit(0);
        }
        //daca intrarea este un director, se va apela recursiv functia listFiles cu un nivel de adancime mai mare
        if (S_ISDIR(file_info.st_mode)) 
        {
            listFiles(filepath,file,depth+1);
        }
    }
    //inchidem directorul
    closedir(dir);
    close(fd);
}

int main(int argc,char *argv[]) 
{
    if(argc!=2) 
    {
        perror("Eroare numar de argumente!\n");
        exit(0);
    }
    char *snapshot="snapshot_";
    strcat(snapshot,argv[1]);
    strcat(snapshot,".txt");
    listFiles(argv[1],snapshot,1);
    return 0;
}