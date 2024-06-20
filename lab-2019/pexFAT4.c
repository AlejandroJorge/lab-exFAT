#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CLUSTER_EOF 0xFFFFFFFF
#define CLUSTER_MAX 0xFFFFFFF6
#define CLUSTER_MIN 0x00000003

typedef struct {
  char jump[3];
  char FSName[8];
  char Chunk[53];
  long offsetPart;
  long sizeVol;
  int FATOffset;
  int FATlen;
  int ClusterHeapOffset;
  int ClusterCount;
  int RootDirFirstCluster;
  int VSN;
  short FSR;
  short FlagVol;
  char BytePerSector;
  char SectorPerCluster;
  char NumberFats;
  char DriveSelect;
  char PercentUse;
  char reserved[7];
  char BootCode[390];
  short BootSignature;
} __attribute((packed)) exFatBootSector;

unsigned int getBit(unsigned char byte, int pos) {
  unsigned char mask = 1 << (7 - pos);
  return (byte & mask) > 0;
}

int main(int argc, char *argv[]) {
  int fd, exp;
  exFatBootSector boot;

  if (argc != 2) {
    printf("Uso: %s <Image File System>\n", argv[0]);
    exit(1);
  }

  if ((fd = open(argv[1], O_RDONLY)) < 0)
    perror("No se pudo abrir la imagen del disco\n");

  if (read(fd, &boot, sizeof(boot)) < 0)
    perror("No se pudo leer la imagen del disco\n");

  size_t FATOffset = boot.FATOffset * pow(2.0, boot.BytePerSector);

  unsigned int FAT[boot.ClusterCount + 2];

  if (lseek(fd, FATOffset, SEEK_SET) < 0)
    perror("Error en seek");

  if (read(fd, FAT, sizeof(FAT)) < 0)
    perror("Error al leer el FAT");

  size_t rootDirOffset =
      boot.ClusterHeapOffset * pow(2.0, boot.BytePerSector) +
      (boot.RootDirFirstCluster - 2) *
          pow(2.0, boot.SectorPerCluster + boot.BytePerSector);

  if (lseek(fd, rootDirOffset, SEEK_SET) < 0)
    perror("Error en seek rootDirOffset");

  char dirEntry[32];

  // Read volume label
  if (read(fd, dirEntry, sizeof(dirEntry)) < 0)
    perror("Error leyendo allocation bit map directory entry");

  printf("Volume label: ");
  char *volumeLabel = &dirEntry[2];
  for (size_t i = 0; i < 22; i++) {
    printf("%c", volumeLabel[i]);
  }
  printf("\n");

  printf("Contents of the root directory\n");

  // Skip allocation bitmap and UPCase table
  if (lseek(fd, sizeof(dirEntry) * 2, SEEK_CUR) < 0)
    perror("Error en seek");

  while (1) {
    unsigned char fileEntry[32];
    if (read(fd, fileEntry, sizeof(fileEntry)) < 0)
      perror("Error leyendo entrada en la tabla del rootDir");

    unsigned char entryType = fileEntry[0];
    if (entryType == 0x00)
      break;
    if (entryType != 0x85)
      continue;

    printf("\n");
    // Precondition: A file entry has been read

    unsigned char streamEntry[32];
    if (read(fd, streamEntry, sizeof(streamEntry)) < 0)
      perror("Error leyendo entrada en la tabla del rootDir");

    printf("Type: ");
    short attributes = fileEntry[4];
    if (attributes & (0x10))
      printf("Directory\n");
    else
      printf("File\n");

    unsigned char nameLength = streamEntry[3];
    printf("Name Length: %d\n", nameLength);

    printf("Name: ");
    while (nameLength > 0) {
      unsigned char fileNameEntry[32];
      if (read(fd, fileNameEntry, sizeof(fileNameEntry)) < 0)
        perror("Error leyendo entrada en la tabla del rootDir");

      for (size_t i = 0; i < nameLength * 2 && i < 30; i++) {
        printf("%c", fileNameEntry[2 + i]);
      }

      if (nameLength < 15)
        nameLength = 0;
      else
        nameLength -= 15;
    }

    printf("\n");

    printf("Cluster chain: ");
    unsigned int currCluster = streamEntry[20];
    while (currCluster != CLUSTER_EOF && currCluster != 0) {
      printf("%d ", currCluster);
      currCluster = FAT[currCluster];
    }
    printf("\n");
  }

  exit(0);
}
