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

    printf("Archivo \"");

    unsigned int amountFileNameEntries = (nameLength + 15) / 15;
    for (size_t i = 0; i < amountFileNameEntries; i++) {
      // Leer File Name Directory Entry
      if (read(fd, &entry, sizeof(DirEntry)) < 0)
        perror("Error leyendo entrada del directorio");

      char *nameSection = &entry[2];
      unsigned int charsToPrint = 30;
      if (nameLength * 2 < charsToPrint)
        charsToPrint = nameLength * 2;

      for (unsigned int i = 0; i < charsToPrint; i++) {
        printf("%c", nameSection[i]);
      }

      nameLength -= charsToPrint;
    }

    printf("\" con primer cluster nro %d\n", firstCluster);
  }

  exit(0);
}
