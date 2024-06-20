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

  if (argc != 3) {
    printf("Uso: %s <Image File System> <Required clusters>\n", argv[0]);
    exit(1);
  }

  if ((fd = open(argv[1], O_RDONLY)) < 0)
    perror("No se pudo abrir la imagen del disco\n");

  if (read(fd, &boot, sizeof(boot)) < 0)
    perror("No se pudo leer la imagen del disco\n");

  size_t rootDirOffset =
      boot.ClusterHeapOffset * pow(2.0, boot.BytePerSector) +
      (boot.RootDirFirstCluster - 2) *
          pow(2.0, boot.SectorPerCluster + boot.BytePerSector);

  if (lseek(fd, rootDirOffset, SEEK_SET) < 0)
    perror("Error en seek rootDirOffset");

  char dirEntry[32];

  // Skip volume label
  if (lseek(fd, sizeof(dirEntry), SEEK_CUR) < 0)
    perror("Error seeking");

  if (read(fd, dirEntry, sizeof(dirEntry)) < 0)
    perror("Error leyendo allocation bit map directory entry");

  unsigned int allocationBitMapCluster = dirEntry[20];

  size_t allocationBitMapOffset =
      boot.ClusterHeapOffset * pow(2.0, boot.BytePerSector) +
      (allocationBitMapCluster - 2) *
          pow(2.0, boot.SectorPerCluster + boot.BytePerSector);

  if (lseek(fd, allocationBitMapOffset, SEEK_SET) < 0)
    perror("Error en seek allocationBitMapOffset");

  size_t bitMapCells = (boot.ClusterCount - 2 + 8) / 8;
  unsigned char allocationBitMap[bitMapCells];

  if (read(fd, allocationBitMap, sizeof(allocationBitMap)) < 0)
    perror("Error leyendo el bitmap");

  size_t required = atoi(argv[2]);

  printf("Clusters disponibles: ");
  for (size_t i = 2; i < boot.ClusterCount + 2 && required > 0; i++) {
    size_t byteOffset = (i - 2) / 8;
    int bitOffset = (i - 2) % 8;

    int bit = getBit(allocationBitMap[byteOffset], bitOffset);
    if (bit == 0) {
      printf("%ld ", i);
      required--;
    }
  }
  printf("\n");

  if (required > 0) {
    printf("Faltaron %ld clusters\n", required);
  }

  exit(0);
}
