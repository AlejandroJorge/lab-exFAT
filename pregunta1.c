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

  printf("Size en cantidad de sectores: %ld\n", boot.sizeVol);
  printf("FAT en el sector: %d\n", boot.FATOffset);
  printf("Size del FAT en sectores: %d\n", boot.FATlen);
  printf("Data inicia en sector: %d\n", boot.ClusterHeapOffset);
  printf("Directorio raiz en el cluster: %d\n", boot.RootDirFirstCluster);
  exp = boot.BytePerSector;
  printf("Size de sector en bytes: %d\n", (int)pow(2.0, exp));
  exp = boot.SectorPerCluster;
  printf("Cantidad de sectores por Cluster: %d\n", (int)pow(2.0, exp));
  printf("Numero de FATs en el filesystem: %d\n", boot.NumberFats);
  printf("Size del espacio para data en Clusters: %d\n", boot.ClusterCount);

  unsigned int FAT[boot.ClusterCount];
  size_t FAToffset = boot.FATOffset * (int)pow(2.0, boot.BytePerSector);

  if (lseek(fd, FAToffset, SEEK_SET) < 0)
    perror("Error en seek\n");

  unsigned int mediaType;
  if (read(fd, &mediaType, sizeof(mediaType)) < 0)
    perror("Error leyendo mediatype\n");

  printf("Media Type: %X\n", mediaType);

  unsigned int FATconstant;
  if (read(fd, &FATconstant, sizeof(FATconstant)) < 0)
    perror("Error leyendo FATconstant\n");

  printf("Constant: %X\n", FATconstant);

  if (read(fd, &FAT, sizeof(FAT)) < 0)
    perror("No se pudo leer el FAT\n");

  int selectedCluster = atoi(argv[2]);

  if (selectedCluster < 0 || selectedCluster >= boot.ClusterCount) {
    printf("El nro del cluster no es valido\n");
    exit(1);
  }

  printf("Valor dentro en la entrada FAT para el cluster %d: %d\n",
         selectedCluster, FAT[selectedCluster]);

  if (FAT[selectedCluster] <= CLUSTER_MAX &&
      FAT[selectedCluster] >= CLUSTER_MIN) {
    printf("EL cluster forma parte de una cadena de clusters, esta ocupado\n");
  } else if (FAT[selectedCluster] == CLUSTER_EOF) {
    printf("El cluster es el ultimo de una cadena de clusters, esta ocupado\n");
  } else if (FAT[selectedCluster] == 0) {
    printf("El cluster esta libre\n");
  } else {
    printf(
        "El cluster esta corrupto o es usado por el sistema, esta ocupado\n");
  }

  exit(0);
}
