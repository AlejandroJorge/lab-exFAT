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

int main(int argc, char *argv[]) {
  int fd, exp;
  exFatBootSector boot;

  if (argc != 3) {
    printf("Uso: %s <Image File System> <Cluster number>\n", argv[0]);
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

  int selectedCluster = atoi(argv[2]);

  if (selectedCluster < 0 || selectedCluster >= boot.ClusterCount) {
    printf("El nro del cluster no es valido\n");
    exit(1);
  }

  printf("Valor dentro en la entrada FAT para el cluster %d: %d\n",
         selectedCluster, FAT[selectedCluster - 2]);

  if (FAT[selectedCluster - 2] <= CLUSTER_MAX &&
      FAT[selectedCluster - 2] >= CLUSTER_MIN) {
    printf(
        "EL cluster %d forma parte de una cadena de clusters, esta ocupado\n",
        selectedCluster);
  } else if (FAT[selectedCluster - 2] == CLUSTER_EOF) {
    printf(
        "El cluster %d es el ultimo de una cadena de clusters, esta ocupado\n",
        selectedCluster);
  } else if (FAT[selectedCluster - 2] == 0) {
    printf("El cluster %d esta libre\n", selectedCluster);
  } else {
    printf(
        "El cluster %d esta corrupto o es usado por el sistema, esta ocupado\n",
        selectedCluster);
  }

  exit(0);
}
