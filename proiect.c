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

// functie pentru deschiderea fisierului snapshot
//path- calea catre snapshot
//file - numele snapshot-ului
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

// functie pentru executarea fisierului script si scriere a datelor in pipe
// arg- calea catre fisier
// pipefd - pipe-ul dintre procesul fiu si procesul nepot
int executeScript(char arg[2048], int pipefd[2]) 
{
    if(pipe(pipefd)==-1)
    {
        perror("Eroare creare pipe\n");
        exit(1);
    }
    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("Eroare fork\n");
        exit(1);
    } 
    else if (pid == 0) 
    { 
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO); // redirectam stdout la partea de scriere a pipe-ului
        close(pipefd[1]);

        char script_path[1024] = "./verificare.sh";
        if (execlp(script_path, "verificare.sh", arg, NULL) == -1) 
        {
            perror("Eroare executare script\n");
            exit(1);
        }
    } 
    else 
    { 
        close(pipefd[1]);

        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) 
        {
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) 
        {
            if (WEXITSTATUS(status) == 1) 
            {
                return 1;
            } 
            else 
            {
                return 0;
            }
        }
        else 
        {
            perror("Erroare executare script\n");
            exit(1);
        }
    }
}

//functie pentru mutarea unui fisier in directorul de fisiere izolate
//cream un fisier nou cu acelasi nume si continut in directorul pentru izolate si il stergem pe cel vechi
//filepath- calea catre fisier
//isolated - calea catre directorul pentru fisiere izolate
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
//pipefd- pipe-ul
void listFiles(char *path, int depth, int fd, char *isolated, int *nrfp,int pipefd[2]) 
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
            //daca fisierul nu are drepturi, o sa fie verificat de fisierul script
            if(!(file_info.st_mode & S_IRUSR) && !(file_info.st_mode & S_IWUSR) && !(file_info.st_mode & S_IXUSR) && !(file_info.st_mode & S_IRGRP) && !(file_info.st_mode & S_IWGRP) && !(file_info.st_mode & S_IXGRP) && !(file_info.st_mode & S_IROTH) && !(file_info.st_mode & S_IWOTH) && !(file_info.st_mode & S_IXOTH))
            {
                int func=executeScript(filepath,pipefd);
                if (func == 0) 
                {
                    if ((write(fd, buffer, strlen(buffer))) < 0) 
                    {
                        perror("Eroare scriere in snapshot\n\n");
                        exit(1);
                    }
                } 
                else if(func == 1)
                { 
                    moveFile(filepath, isolated);
                    (*nrfp)++;
                }
            }
            else
            {
                if ((write(fd, buffer, strlen(buffer))) < 0) 
                    {
                        perror("Eroare scriere in snapshot\n\n");
                        exit(1);
                    }
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
            listFiles(filepath, depth + 1, fd, isolated, nrfp,pipefd);
        } 
    }
    //inchidem directorul
    closedir(dir);
}

//verificam daca argumentele sunt directoare
// dir - calea catre director
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
// arg- vector argumente in linia de comanda
// n -numar argumente
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
// path - calea catre cele 2 snapshot-uri
// file1 - numele primului snapshot
// file2 - numele celui de al doilea snapshot
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
    for (int i = 1; i < argc-4; i++) 
    {
        int pipefd[2];
        pid_t cpid = fork();
        if (cpid == -1) 
        {
            perror("Eroare fork!\n");
            exit(1);
        }
        else if (cpid == 0) 
        {
            //cream numele snapshot-ului
            char snapshot[1024] = "snapshot_";
            strcat(snapshot, argv[i]);
            strcat(snapshot, ".txt");
            char snapshotPath[2048];
            sprintf(snapshotPath, "%s/%s", argv[argc-3], snapshot);
            struct stat info;
            //numar fisiere periculoase in directorul dat
            int nrfp=0;
            if (stat(snapshotPath, &info) == 0) 
            {
                //cream un snapshot nou
                char newSnapshot[2048];
                strcpy(newSnapshot, snapshot);
                strcat(newSnapshot, "_new");
                char newSnapshotPath[2048];
                sprintf(newSnapshotPath, "%s/%s", argv[argc-3], newSnapshot);
                int fd = openFile(argv[argc-3], newSnapshot);
                listFiles(argv[i], 1, fd, argv[argc-1], &nrfp, pipefd);
                close(fd);
                //comparam snapshot-ul vechi cu cel nou
                if (compareSnapshots(argv[argc-3], snapshot, newSnapshot) == 0) 
                {
                    unlink(snapshotPath);
                }
                else 
                {
                    unlink(newSnapshotPath);
                }
                close(pipefd[1]);
                char buffer[1024];
                ssize_t bytes_read;
                // citim si afisam informatiile din pipe
                while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) 
                {
                    write(STDOUT_FILENO, buffer, bytes_read);
                }
                close(pipefd[0]);
                //returnam numarul de fisiere periculoase
                exit(nrfp);
            }
            else 
            {
                //daca nu exista deja snapshot, se va crea unul
                int fd = openFile(argv[argc-3], snapshot);
                listFiles(argv[i], 1, fd, argv[argc-1], &nrfp, pipefd);
                close(fd);
                close(pipefd[1]);
                char buffer[1024];
                ssize_t bytes_read;
                // citim si afisam informatiile din pipe
                while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) 
                {
                    write(STDOUT_FILENO, buffer, bytes_read);
                }
                close(pipefd[0]);
                //returnam numarul de fisiere periculoase
                exit(nrfp);
            }
        }
        close(pipefd[0]);
    }
    // afisam PID-ul si numarul de fisiere periculoase al procesului copil
    for (int i = 0; i < argc-5; i++) 
    {
        pid_t tpid=wait(&status);
        if(WIFEXITED(status))
        {
            //i+1 - numarul procesului copil
            //tpid - PID-ul procesului copil
            // WEXISTATUS(status) - numarul de fisiere periculoase din director
            printf("Procesul Copil %d s-a încheiat cu PID-ul %d și a găsit %d fișiere cu potențial periculos\n", i+1, tpid, WEXITSTATUS(status));
        }
    }
    return 0;
}
