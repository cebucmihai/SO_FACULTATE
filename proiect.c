#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fisier_intrare>\n", argv[0]);
        return 1;
    }

    const char *inputFileName = argv[1];
    char outputFileName[] = "statistica.txt";

    int inputFd = open(inputFileName, O_RDONLY);
    if (inputFd == -1) {
        perror("Eroare la deschiderea fisierului BMP");
        return 1;
    }

    int outputFd = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (outputFd == -1) {
        perror("Eroare la deschiderea fisierului de iesire");
        close(inputFd);
        return 1;
    }

    BMPHeader bmpHeader;
    if (read(inputFd, &bmpHeader, sizeof(BMPHeader)) != sizeof(BMPHeader)) {
        perror("Eroare la citirea header-ului BMP");
        close(inputFd);
        close(outputFd);
        return 1;
    }

    if (bmpHeader.signature[0] != 'B' || bmpHeader.signature[1] != 'M') {
        fprintf(stderr, "Fisierul nu este de tip BMP\n");
        close(inputFd);
        close(outputFd);
        return 1;
    }

    BMPInfoHeader bmpInfoHeader;
    if (read(inputFd, &bmpInfoHeader, sizeof(BMPInfoHeader)) != sizeof(BMPInfoHeader)) {
        perror("Eroare la citirea informatiilor din header-ul BMP");
        close(inputFd);
        close(outputFd);
        return 1;
    }

    struct stat fileInfo;
    if (fstat(inputFd, &fileInfo) == -1) {
        perror("Eroare la obtinerea informatiilor despre fisierul de intrare");
        close(inputFd);
        close(outputFd);
        return 1;
    }

    char modificationTime[20];
    struct tm *timeinfo = localtime(&fileInfo.st_mtime);
    strftime(modificationTime, sizeof(modificationTime), "%d.%m.%Y", timeinfo);
    
    char outputBuffer[512];
    getPermissions( fileInfo.st_mode);
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
             "drepturi de acces altii: %s\n",
             inputFileName, bmpInfoHeader.height, bmpInfoHeader.width, fileInfo.st_size, fileInfo.st_uid, modificationTime,
             fileInfo.st_nlink,permUSR , permGRP, permOTH);

    if (write(outputFd, outputBuffer, strlen(outputBuffer)) == -1) {
        perror("Eroare la scrierea in fisierul de iesire");
    }
    printf("%d\n", bmpInfoHeader.imageSize);
    close(inputFd);
    close(outputFd);

    return 0;
}
