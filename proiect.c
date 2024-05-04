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
int executeScript(char arg[2048]) 
{
    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("Eroare fork\n");
        exit(1);
    } 
    else if (pid == 0) 
    { 
        char script_path[1024] = "./verificare.sh";
        if (execlp(script_path, "verificare.sh", arg, NULL) == -1) 
        {
            perror("Eroare executare fisier script\n");
            exit(1);
        }
    } 
    else 
    { 
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status)) 
        {
            perror("Eroare executare fisier script\n");
            exit(1);
        }
        return WEXITSTATUS(status);
    }
}

//functie pentru mutarea unui fisier in directorul de fisiere izolate
void moveFile(char *filepath, char *isolated)
{
    int src_fd = open(filepath, O_RDONLY);
    if (src_fd == -1) 
    {
        perror("Eroare deschidere fisier suspect\n");
        exit(1);
    }
    char *file = strrchr(filepath, '/');
    if (file == NULL) 
    {
        file = filepath;
    } 
    else 
    {
        file++;
    }
    char newfilepath[2048];
    sprintf(newfilepath, "%s/%s", isolated, file);
    int dest_fd = open(newfilepath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (dest_fd == -1) 
    {
        perror("Eroare creare fisier izolat\n");
        exit(1);
    }
    char c;
    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, &c, sizeof(char))) > 0) 
    {
        if (write(dest_fd, &c, bytes_read) != bytes_read) 
        {
            perror("Eroare scriere fisier izolat\n");
            close(src_fd);
            close(dest_fd);
            exit(1);
        }
    }
    close(src_fd);
    close(dest_fd);
    if (chmod(newfilepath, 000) == -1) 
    {
        perror("Eroare setare drepturi\n");
        exit(1);
    }
    unlink(filepath);
}

//functie pentru parcurgerea recursiva a unui director pentru a obtine informatii despre acesta
//path - calea catre director
//depth - nivelul de adancime in director
//isolated - directorul pentru fisiere izolate
//nrfp - numar fisiere periculoase
void listFiles(char *path, int depth, int fd, char *isolated, int *nrfp) 
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
        //obtinem informatii despre fisierul/directorul curent
        if (stat(filepath, &file_info) == -1) 
        {
            perror("Eroare obtinere informatii fisier!\n");
            exit(1);
        }
        if (!S_ISDIR(file_info.st_mode))
        {
            if (executeScript(filepath) == 0) 
            {
                if ((write(fd, buffer, strlen(buffer))) < 0) 
                {
                    perror("Eroare scriere in snapshot\n\n");
                    exit(1);
                }
            } 
            else  
            { 
                moveFile(filepath, isolated);
                (*nrfp)++;
            }
        }
        //daca intrarea este un director, se va apela recursiv functia listFiles cu un nivel de adancime mai mare
        if (S_ISDIR(file_info.st_mode)) 
        {
            if ((write(fd, buffer, strlen(buffer))) < 0) 
                {
                    perror("Eroare scriere in snapshot\n\n");
                    exit(1);
                }
            listFiles(filepath, depth + 1, fd, isolated, nrfp);
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
    int status;
    //Cream pipe-urile
    int pipes[argc-5][2];
    for (int i = 0; i < argc-5; i++) 
    {
        if (pipe(pipes[i]) == -1) 
        {
            perror("Eroare creare pipe\n");
            exit(1);
        }
    }
    for (int i = 1; i < argc-4; i++) 
    {
        pid_t cpid = fork();
        if (cpid == -1) 
        {
            perror("Eroare fork!\n");
            exit(1);
        }
        else if (cpid == 0) 
        {
            char snapshot[1024] = "snapshot_";
            strcat(snapshot, argv[i]);
            strcat(snapshot, ".txt");
            char snapshotPath[2048];
            sprintf(snapshotPath, "%s/%s", argv[argc-3], snapshot);
            struct stat info;
            int nrfp=0;
            if (stat(snapshotPath, &info) == 0) 
            {
                char newSnapshot[2048];
                strcpy(newSnapshot, snapshot);
                strcat(newSnapshot, "_new");
                char newSnapshotPath[2048];
                sprintf(newSnapshotPath, "%s/%s", argv[argc-3], newSnapshot);
                int fd = openFile(argv[argc-3], newSnapshot);
                listFiles(argv[i], 1, fd, argv[argc-1], &nrfp);
                close(fd);
                if (compareSnapshots(argv[argc-3], snapshot, newSnapshot) == 0) 
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
                int fd = openFile(argv[argc-3], snapshot);
                listFiles(argv[i], 1, fd, argv[argc-1], &nrfp);
                close(fd);
            }

            close(pipes[i][0]);
            write(pipes[i][1], &nrfp, sizeof(int));
            close(pipes[i][1]);
            exit(0);
        }
    }
    for (int i = 0; i < argc-5; i++) 
    {
        pid_t tpid=wait(&status);
        int count;
        close(pipes[i][1]);
        read(pipes[i][0], &count, sizeof(int));
        close(pipes[i][0]);
        printf("Procesul Copil %d s-a încheiat cu PID-ul %d și a găsit %d fișiere cu potențial periculos\n", i+1, tpid, count);
    }
    return 0;
}