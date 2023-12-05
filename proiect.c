#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/wait.h>

// #pragma pack(1) este folosit pentru alinierea corespunzatoare a byte-tilor pentru a putea fi extrasi in structurile declarate mai jos
#pragma pack(1)
typedef struct {
  char signature[2];
  int size;
  int reserved;
  int offset;
} BMPHeader;
#pragma pack()
typedef struct {
    int headerSize;
    int width;
    int height;
    short planes;
    short bitsPerPixel;
    int compression;
    int imageSize;
    int xPixelsPerM;
    int yPixelsPerM;
    int colorsUsed;
    int colorsImportant;
} BMPInfoHeader;

char permUSR[4];
char permGRP[4];
char permOTH[4];
//Functie folosita pentru a vedea permisiunile asupra fisierului a user-ului, grupului si a celorlalti useri
void getPermissions(mode_t mode) {
    snprintf(permUSR, 4, "%c%c%c",
             (mode & S_IRUSR) ? 'R' : '-',
             (mode & S_IWUSR) ? 'W' : '-',
             (mode & S_IXUSR) ? 'X' : '-');
    snprintf(permGRP,4, "%c%c%c",
             (mode & S_IRGRP) ? 'R' : '-',
             (mode & S_IWGRP) ? 'W' : '-',
             (mode & S_IXGRP) ? 'X' : '-');
    snprintf(permOTH,4, "%c%c%c",
             (mode & S_IROTH) ? 'R' : '-',
             (mode & S_IWOTH) ? 'W' : '-',
             (mode & S_IXOTH) ? 'X' : '-');
}

//Aceasta functie verifica daca fisierul respectiv este de tip .BMP si afiseaza datele cerute in problema aferenta saptamanii 6
void processBMP(const char *fileName, int outputFd, int *lines) {
    int inputFd = open(fileName, O_RDWR);
    if (inputFd == -1) {
        perror("Eroare la deschiderea fisierului BMP");
        return;
    }

    BMPHeader bmpHeader;
    if (read(inputFd, &bmpHeader, sizeof(BMPHeader)) != sizeof(BMPHeader)) {
        perror("Eroare la citirea header-ului BMP");
        close(inputFd);
        return;
    }

    if (bmpHeader.signature[0] != 'B' || bmpHeader.signature[1] != 'M') {
        fprintf(stderr, "Fisierul %s nu este de tip BMP\n", fileName);
        close(inputFd);
        return;
    }

    BMPInfoHeader bmpInfoHeader;
    if (read(inputFd, &bmpInfoHeader, sizeof(BMPInfoHeader)) != sizeof(BMPInfoHeader)) {
        perror("Eroare la citirea informatiilor din header-ul BMP");
        close(inputFd);
        return;
    }

    struct stat fileInfo;
    if (fstat(inputFd, &fileInfo) == -1) {
        perror("Eroare la obtinerea informatiilor despre fisierul de intrare");
        close(inputFd);
        return;
    }

    char modificationTime[20];
    struct tm *timeinfo = localtime(&fileInfo.st_mtime);
    strftime(modificationTime, sizeof(modificationTime), "%d.%m.%Y", timeinfo);

    char outputBuffer[512];
    getPermissions(fileInfo.st_mode);
    snprintf(outputBuffer, sizeof(outputBuffer),
             "nume fisier: %s\n"
             "inaltime: %d\n"
             "lungime: %d\n"
             "dimensiune: %lu\n"
             "identificatorul utilizatorului: %d\n"
             "timpul ultimei modificari: %s\n"
             "contorul de legaturi: %lu\n"
             "drepturi de acces user: %s\n"
             "drepturi de acces grup: %s\n"
             "drepturi de acces altii: %s\n\n",
             fileName, bmpInfoHeader.height, bmpInfoHeader.width, fileInfo.st_size, fileInfo.st_uid, modificationTime,
             fileInfo.st_nlink, permUSR, permGRP, permOTH);
    *lines = 10;
    if (write(outputFd, outputBuffer, strlen(outputBuffer)) == -1) {
        perror("Eroare la scrierea in fisierul de iesire");
    }
    //Saptamana 8 : Transformarea imaginii in tonuri de alb si negru
    pid_t childPid = fork();

    if (childPid == -1) {
        perror("Eroare la crearea procesului fiu");
        close(inputFd);
        return;
    }

    if (childPid == 0) {

        lseek(inputFd, bmpHeader.offset, SEEK_SET);

        char pixel[3];
        ssize_t bytesRead;

        while ((bytesRead = read(inputFd, pixel, sizeof(pixel))) > 0) {
            char grayscale = 0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2];
            memset(pixel, grayscale, sizeof(pixel));
            lseek(inputFd, -bytesRead, SEEK_CUR);
            write(inputFd, pixel, sizeof(pixel));
        }

        close(inputFd);
        exit(0);
    } else {
        int status;
        waitpid(childPid, &status, 0);

        if (WIFEXITED(status)) {
            printf("BMP:S-a încheiat procesul cu pid-ul %d și codul %d\n", childPid, WEXITSTATUS(status));
        } else {
            printf("Procesul cu pid-ul %d nu s-a încheiat normal\n", childPid);
        }
    }
    close(inputFd);
}

//Functie folosita pentru a obtine informatii despre celelalte tipuri de fisiere diferite de BMP
void processOtherFile(const char *fileName, int outputFd, int *lines) {
    struct stat fileInfo;
    if (stat(fileName, &fileInfo) == -1) {
        perror("Eroare la obtinerea informatiilor despre fisierul");
        return;
    }

    char outputBuffer[512];
    getPermissions(fileInfo.st_mode);
    snprintf(outputBuffer, sizeof(outputBuffer),
             "nume fisier: %s\n"
             "dimensiune: %lu\n"
             "identificatorul utilizatorului: %d\n"
             "contorul de legaturi: %lu\n"
             "drepturi de acces user: %s\n"
             "drepturi de acces grup: %s\n"
             "drepturi de acces altii: %s\n\n",
             fileName, fileInfo.st_size, fileInfo.st_uid,
             fileInfo.st_nlink, permUSR, permGRP, permOTH);
    *lines = 7;
    if (write(outputFd, outputBuffer, strlen(outputBuffer)) == -1) {
        perror("Eroare la scrierea in fisierul de iesire");
    }
}

//Functie folosita pentru a obtine informatii despre fisierele de tip director
void processDirectory(const char *dirName, int outputFd, int *lines) {
    struct stat dirInfo;
    if (lstat(dirName, &dirInfo) == -1) {
        perror("Eroare la obtinerea informatiilor despre directorul de intrare");
        return;
    }
    
    char outputBuffer[512];
    getPermissions(dirInfo.st_mode);
    snprintf(outputBuffer, sizeof(outputBuffer),
             "nume director: %s\n"
             "identificatorul utilizatorului: %d\n"
             "drepturi de acces user: %s\n"
             "drepturi de acces grup: %s\n"
             "drepturi de acces altii: %s\n\n",
             dirName, dirInfo.st_uid, permUSR, permGRP, permOTH);
    *lines = 5;
    if (write(outputFd, outputBuffer, strlen(outputBuffer)) == -1) {
        perror("Eroare la scrierea in fisierul de iesire");
    }
}

//Functie folosita pentru a obtine informatii despre link-urile simbolice
void processSymbolicLink(const char *linkName, int outputFd, int *lines) {
    struct stat linkInfo;
    if (lstat(linkName, &linkInfo) == -1) {
        perror("Eroare la obtinerea informatiilor despre legatura simbolica");
        return;
    }

    struct stat targetInfo;
    if (stat(linkName, &targetInfo) == -1) {
        perror("Eroare la obtinerea informatiilor despre fisierul target al legaturii simbolice");
        return;
    }

    char outputBuffer[512];
    getPermissions(linkInfo.st_mode);
    snprintf(outputBuffer, sizeof(outputBuffer),
             "nume legatura: %s\n"
             "dimensiune: %lu\n"
             "dimensiune fisier: %lu\n"
             "drepturi de acces user: %s\n"
             "drepturi de acces grup: %s\n"
             "drepturi de acces altii: %s\n\n",
             linkName, linkInfo.st_size, targetInfo.st_size, permUSR, permGRP, permOTH);
    *lines = 6;
    if (write(outputFd, outputBuffer, strlen(outputBuffer)) == -1) {
        perror("Eroare la scrierea in fisierul de iesire");
    }
}

// Funcție pentru a deschide și scrie în fișierul de statistică numarul de linii scrise in fisierele scrise in directorul de iesire
void writeToStatFile(int statFd, int childPid, int linesWritten) {

  
  char buffer[512];
  snprintf(buffer, sizeof(buffer), "Pentru procesul fiu %d numarul de linii este %d.\n", childPid, linesWritten);

  if (write(statFd, buffer, strlen(buffer)) == -1) {
    perror("Eroare la scrierea in fisierul de statistica");
    exit(1);
  }

}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <director_intrare> <director_iesire> <caracter>\n", argv[0]);
    return 1;
  }
  
  const char *inputDirName = argv[1];
  const char *outputDirName = argv[2];
  
  DIR *dir;
  struct dirent *entry;
  dir = opendir(inputDirName);

  int totalCorrectSentences = 0;
  
  if (dir == NULL) {
    perror("Eroare la deschiderea directorului de intrare");
    return 1;
  }
  
  struct stat pathInfo;
  if (stat(outputDirName, &pathInfo) == -1) {
    if(!S_ISDIR(pathInfo.st_mode)){
      perror("Eroare la accesarea directorului de iesire");
      exit(1);
    }
  }

  //Deschiderea fisierului de statistici din directorul de intrare
  char statisticFile[PATH_MAX];
  snprintf(statisticFile, PATH_MAX, "%s/statistica.txt", inputDirName);
  int statFd = open(statisticFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  //Deschidem pipe-ul aferent urmatoarelor : fiu_script -> parinte
  int pipefd1[2];
  if (pipe(pipefd1) == -1) {
    perror("Eroare la deschiderea pipe-ului");
    return 1;
  }

  
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    pid_t StatChildPid = -1;
    pid_t ScriptChildPid = -1;
    
    char entryPath[PATH_MAX];
    
    
    snprintf(entryPath, PATH_MAX, "%s/%s", inputDirName, entry->d_name);
    
    //Deschidem pipe-ul aferent urmatoarelor : fiu_statistica -> fiu_script
    int pipefd2[2];
    if (pipe(pipefd2) == -1) {
      perror("Eroare la deschiderea pipe-ului");
      return 1;
    }
    
    if((entry->d_type == DT_REG || entry->d_type == DT_DIR || entry->d_type == DT_LNK) && (strstr(entryPath, ".bmp") != NULL)){
      StatChildPid = fork();
      
      if (StatChildPid == -1) {
	perror("Eroare la crearea procesului fiu");
	return 1;
      }
    } else {
      StatChildPid = fork();
      
      if (StatChildPid == -1) {
	perror("Eroare la crearea procesului fiu");
	return 1;
      }
      if(StatChildPid != 0){
		ScriptChildPid = fork();
		if ( ScriptChildPid == -1) {
		  perror("Eroare la crearea procesului fiu");
		  return 1;
		}
      }
    }
    if( StatChildPid > 0 && ScriptChildPid > 0){
      close(pipefd2[0]);
      close(pipefd2[1]);
    }
    if( ScriptChildPid == 0){
      close(pipefd1[0]);
      close(pipefd2[1]);
 
      // redirecționează intrarea standard către capătul de citire al pipe-ului dintre fiu_script si fiu_statistica
      if (dup2(pipefd2[0], STDIN_FILENO) == -1) {
      	perror("Eroare la redirectionarea intrarii standard");
      	exit(-1);
       }
      close(pipefd2[0]); 

          //redirecționează ieșirea standard către capătul de scriere al pipe-ului dintre fiu_script si fiu_parinte
     if (dup2(pipefd1[1], STDOUT_FILENO) == -1) {
     	perror("Eroare la redirectionarea iesirii standard");
      	exit(-1);
      }
      close(pipefd1[1]); 
      execlp("bash", "bash", "./script",argv[3],(char*)NULL);
      // Dacă execlp revine, înseamnă că a esuat
      printf("EROARE: NU A INTRAT IN SCRIPT !!!\n");

      exit(-1);
    }
    if (StatChildPid == 0) {
      close(pipefd1[1]);
      close(pipefd1[0]); // inchidem ambele capete pentru procesul care se ocupa de statistici
      int linesWritten = 0;
      char outputFileName[PATH_MAX];
      snprintf(outputFileName, PATH_MAX, "%s/%s_statistica.txt", outputDirName, entry->d_name);
      
      int outputFd = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      
      if (outputFd == -1) {
	perror("Eroare la deschiderea fisierului de iesire");
	exit(-1);
      }
      
      if (entry->d_type == DT_REG) {
	if (strstr(entryPath, ".bmp") != NULL) {
	  processBMP(entryPath, outputFd, &linesWritten);
	} else {
    
    processOtherFile(entryPath, outputFd, &linesWritten);
    
    close(pipefd2[0]);
    //Deschidem fisierul pentru citire
    int fd = open(entryPath, O_RDONLY);
    if (fd == -1) {
      perror("Eroare la deschiderea fisierului pentru citire");
      exit(-1);
    }
    //redirectam din fiul_statistica continutul fisierului in fiul_script
    char buffer[4096];
    ssize_t bytesRead = 0;
    if (dup2(pipefd2[1], STDOUT_FILENO) == -1) {
      perror("Eroare la redirectionarea iesirii standard");
      exit(-1);
    }
    close(pipefd2[1]);
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        printf("%s\n",buffer);
        write(pipefd2[1], buffer, bytesRead);
    }
    

    close(fd);
    
	}
      } else if (entry->d_type == DT_DIR) {
	processDirectory(entryPath, outputFd, &linesWritten);
      } else if (entry->d_type == DT_LNK) {
	processSymbolicLink(entryPath, outputFd, &linesWritten);
      }
      
      close(outputFd);
      exit(linesWritten);
    }
  }
  int status = 0;
  int childPid = -1;
  close(pipefd1[1]);
  while((childPid = wait(&status)) != -1){
      
      if (WIFEXITED(status)) {
	printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", childPid, WEXITSTATUS(status));
      } else {
	printf("Procesul cu pid-ul %d nu s-a încheiat normal avand codul %d\n", childPid, WEXITSTATUS(status));
      }
      if(WEXITSTATUS(status) > 0){
	writeToStatFile(statFd, childPid, WEXITSTATUS(status));
      }

    char buffer[512];
    ssize_t bytesRead;
    while ((bytesRead = read(pipefd1[0], buffer, 10)) > 0) {
        totalCorrectSentences += atoi(buffer);
    }
  }
  close(pipefd1[0]);
  close(statFd);
  closedir(dir);
  printf("Au fost identificate în total %d propoziții corecte care conțin caracterul %c\n", totalCorrectSentences, argv[3][0]);
  return 0;
}
