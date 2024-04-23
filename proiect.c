#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/wait.h>

// functie pentru deschiderea fisierului 
int openFile(char *path, char *file) 
{
    char snapshotPath[2048];
    sprintf(snapshotPath, "%s/%s", path, file);
    int fd = creat(snapshotPath, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    if (fd == -1) 
    {
        perror("Eroare deschidere fisier snapshot pentru scriere!\n");
        exit(1);
    }
    return fd;
}

// functie pentru executarea fisierului script
void executeScript(char *arg1, char *arg2) 
{
    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("Eroare fork\n");
        exit(1);
    } 
    else if (pid == 0) 
    { 
        char script_path[1024] = "./verificareASCII.sh";
        if (execlp(script_path, "verificareASCII.sh", arg1, arg2, NULL) == -1) 
        {
            perror("Eroare executare fisier script\n");
            exit(1);
        }
    } 
    else 
    { 
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) 
        {
            perror("Eroare executare fisier script\n");
            exit(1);
        }
    }
}

//functie pentru parcurgerea recursiva a unui director pentru a obtine informatii despre acesta
//path - calea catre director
//depth - nivelul de adancime in director
void listFiles(char *path, char *file, int depth, int fd, char *isolated) 
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
        char filepath[2048];
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
        /*if (!S_ISDIR(file_info.st_mode) && (file_info.st_mode & S_IRUSR) && (file_info.st_mode & S_IWUSR) && (file_info.st_mode & S_IXUSR) && (file_info.st_mode & S_IRGRP) && (file_info.st_mode & S_IWGRP) && (file_info.st_mode & S_IXGRP) && (file_info.st_mode & S_IROTH) && (file_info.st_mode & S_IWOTH) && (file_info.st_mode & S_IXOTH))
        {
            executeScript(filepath, isolated);
        }*/
        if (!S_ISDIR(file_info.st_mode))
        {
            executeScript(filepath, isolated);
        }
        //daca intrarea este un director, se va apela recursiv functia listFiles cu un nivel de adancime mai mare
        if (S_ISDIR(file_info.st_mode)) 
        {
            listFiles(filepath, file, depth + 1, fd, isolated);
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

//functie de comparare a 2 fisiere snapshot
int compareSnapshots(char *path, char *file1, char *file2) 
{
    char pathFile1[2048];
    char pathFile2[2048];
    sprintf(pathFile1,"%s/%s",path,file1);
    sprintf(pathFile2,"%s/%s",path,file2);
    // deschidem cele 2 fisiere snapshot
    int fd1 = open(pathFile1, O_RDONLY, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    int fd2 = open(pathFile2, O_RDONLY, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    if (fd1 == -1 || fd2 == -1) 
    {
        perror("Eroare deschidere fisiere snapshot pentru comparare!\n");
        exit(1);
    }
    // comparam continutul din cele 2 fisiere
    char c1, c2;
    ssize_t bytes_read1, bytes_read2;
    while ((bytes_read1 = read(fd1, &c1, sizeof(char))) != 0 && (bytes_read2 = read(fd2, &c2, sizeof(char))) != 0) 
    {
        if (c1 != c2) 
        {
            close(fd1);
            close(fd2);
            return 0;
        }
    }
    close(fd1);
    close(fd2);
    return 1;
}

int main(int argc, char *argv[]) 
{
    if ((argc<3 || argc>13) && verifyArguments(argv,argc)==0) 
    {
        perror("Eroare argumente!\n");
        exit(1);
    }
    if(strcmp(argv[argc-2],"-x")!=0 || strcmp(argv[argc-4],"-o")!=0)
    {
        perror("Eroare argumente!\n");
        exit(1);
    }
    if(verifyDirectory(argv[argc-3])==0 || verifyDirectory(argv[argc-1])==0)
    {
        perror("Eroare argumente!\n");
        exit(1);
    }
    pid_t cpid;
    int status;
    for (int i = 1; i < argc-4; i++) 
    {
        cpid=fork();
        if(cpid==-1)
        {
            perror("Eroare fork!\n");
            exit(1);
        }
        else if(cpid==0)
        {
            char snapshot[1024] = "snapshot_";
            strcat(snapshot, argv[i]);
            strcat(snapshot, ".txt");
            char snapshotPath[2048];
            sprintf(snapshotPath,"%s/%s",argv[argc-3],snapshot);
            //verificam daca fisierul snapshot exista deja
            struct stat info;
            if (stat(snapshotPath, &info)==0) 
            {
                //cream un alt fisier snapshot
                char newSnapshot[2048];
                strcpy(newSnapshot,snapshot);
                strcat(newSnapshot,"_new");
                char newSnapshotPath[2048];
                sprintf(newSnapshotPath,"%s/%s",argv[argc-3],newSnapshot);
                int fd = openFile(argv[argc-3], newSnapshot);
                if (verifyDirectory(argv[i]) == 1) 
                {
                    listFiles(argv[i], newSnapshot, 1, fd, argv[argc-1]);
                }
                close(fd);
                //comparam fisierul nou cu cel vechi
                if (compareSnapshots(argv[argc-3], snapshot, newSnapshot)==0) 
                {
                    unlink(snapshotPath);
                } 
                else 
                {
                    unlink(newSnapshotPath);
                }
            } 
            else 
            {
                //fisierul nu exista
                int fd = openFile(argv[argc-3], snapshot);
                if (verifyDirectory(argv[i]) == 1) 
                {
                    listFiles(argv[i], snapshot, 1, fd, argv[argc-1]);
                }
                close(fd);
            }
            printf("Snapshot for directory '%s' created successfully\n",argv[i]);
            exit(0);
        }
    }
    for(int i=0;i<argc-5;i++)
    {
        pid_t tpid=wait(&status);
        printf("Child proccess %d terminated with PID %d and exit code %d\n",i+1,tpid,WEXITSTATUS(status));
    }
    return 0;
}