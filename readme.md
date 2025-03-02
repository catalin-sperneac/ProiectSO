Sperneac Catalin
Cerinta proiect:
In acest proiect avem un fisier C si un fisier shell care gestioneaza prin intermediul unor pipe-uri continutul unui director sau mai multe, verificand fiecare fisier din acestea si creeaza un "snapshot" pentru fiecare director, adica un fisier text in care este scris ce contine directorul respectiv.
Functionare:
In fisierul C "proiect.c", pentru un director se va crea un fisier text care va fi salvat in directorul numit in argument, iar in acel fisier se va crea un arbore care arata continutul directorului respectiv.
Daca exista deja un snapshot pentru acest director, se va compara snapshot-ul nou cu cel vechi, iar daca exista modificari in acele fisiere, snapshot-ul va fi inlocuit.
Fiecare fisier din director va fi verificat prin intermediul fisierului shell "verificare.sh". Acest fisier verifica daca fisierul este "periculos" sau nu.
Un fisier este periculos daca indeplineste anumite conditii. De exemplu, daca numarul de linii este mai mic de 3, dar numarul de cuvinte este mai mare de 1000 si numarul de caractere mai mare de 2000, sau daca in fisier exista caractere care nu fac parte din tabela ASCII sau daca exista cuvintele "corrupted", "malicious" etc. atunci acel fisier este considerat "periculos".
Daca un fisier este periculos, atunci acesta va fi izolat intr-un director separat, dat ca argument si se vor schimba drepturile acestuia "chmod 000".
Pentru fiecare director se va crea un pipe in care vor fi salvate informatii despre director (PID-ul si numarul de fisierele periculoase) si vor fi afisate la terminarea programului.
Pentru a rula acest program, avem nevoie de 1-6 directoare date ca argument, urmat de "-o" si numele directorului in care vor fi salvate snapshot-urile, apoi de "-x" si numele directorului in care vor fi mutate fisierele periculoase.
La sfarsitul rularii, vor fi afisare PID-ul si numarul de fisiere periculoase pentru fiecare director. 