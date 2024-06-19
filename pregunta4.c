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

typedef char DirEntry[32];

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

  unsigned int FAT[boot.ClusterCount];
  size_t FAToffset = boot.FATOffset * (int)pow(2.0, boot.BytePerSector);

  if (lseek(fd, FAToffset, SEEK_SET) < 0)
    perror("Error en seek\n");

  unsigned int mediaType;
  if (read(fd, &mediaType, sizeof(mediaType)) < 0)
    perror("Error leyendo mediatype\n");

  unsigned int FATconstant;
  if (read(fd, &FATconstant, sizeof(FATconstant)) < 0)
    perror("Error leyendo FATconstant\n");

  if (read(fd, &FAT, sizeof(FAT)) < 0)
    perror("No se pudo leer el FAT\n");

  size_t bytesPerCluster = pow(2.0, boot.BytePerSector + boot.SectorPerCluster);
  size_t rootDirOffset = boot.ClusterHeapOffset * pow(2.0, boot.BytePerSector) +
                         2 * bytesPerCluster;

  // Goes to the rootDir
  if (lseek(fd, rootDirOffset, SEEK_SET) < 0)
    perror("Error en seek\n");

  // Skips VolumeLabel, AllocationBitMap, UPCaseTable
  if (lseek(fd, sizeof(DirEntry) * 3, SEEK_CUR) < 0)
    perror("Error en seek\n");

  while (1) {
    DirEntry entry;

    if (read(fd, &entry, sizeof(DirEntry)) < 0)
      perror("Error leyendo entrada del directorio");

    unsigned char entryType = entry[0];

    if (entryType == 0)
      break;

    if (entryType != 0x85)
      continue;

    // Leer Stream Extension Directory Entry
    if (read(fd, &entry, sizeof(DirEntry)) < 0)
      perror("Error leyendo entrada del directorio");

    unsigned char nameLength = entry[3];
    unsigned int firstCluster = entry[20];
    unsigned char leftToPrint = nameLength;
    unsigned long long dataSize = entry[24];

    printf("Archivo \"");

    unsigned int amountFileNameEntries = (nameLength + 15) / 15;
    for (size_t i = 0; i < amountFileNameEntries; i++) {
      // Leer File Name Directory Entry
      if (read(fd, &entry, sizeof(DirEntry)) < 0)
        perror("Error leyendo entrada del directorio");

      char *nameSection = &entry[2];
      unsigned int charsToPrint = 30;
      if (leftToPrint * 2 < charsToPrint)
        charsToPrint = leftToPrint * 2;

      for (unsigned int i = 0; i < charsToPrint; i++) {
        printf("%c", nameSection[i]);
      }

      leftToPrint -= charsToPrint;
    }

    printf("\" (%d caracteres unicode)\ncon primer cluster nro %d y tamano de "
           "ese primer cluster %llu bytes\n",
           nameLength, firstCluster, dataSize);

    printf("\tMostrando cadena de clusters\n");

    unsigned int currCluster = firstCluster;
    while (currCluster != CLUSTER_EOF && currCluster <= CLUSTER_MAX &&
           currCluster >= CLUSTER_MIN) {
      printf("\t\t* Cluster nro %d\n", currCluster);
      currCluster = FAT[currCluster];
    }

    printf("\n");
  }

  exit(0);
}
