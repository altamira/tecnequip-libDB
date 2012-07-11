#include "DB.h"

void ShowSintax(char *path, char *msg)
{
  unsigned int i;
  struct DriverList *Driver;

  if(msg != NULL)
    printf("%s\n", msg);

  printf("Sintaxe: %s <BANCO> <SERVIDOR> <USUARIO> <SENHA> <BASE> \"<Comando SQL>\"\n\n", path);

  printf("Lista de bancos suportados:\n");
  for(i=0, Driver = DB_GetDriverByIndex(i); Driver != NULL; Driver = DB_GetDriverByIndex(++i)) {
    printf("\t%s - %s\n", Driver->ID, Driver->Desc);
  }
}

int main(int argc, char *argv[])
{
  struct strDB sDB;

  DB_InitDrivers();

  if(argc != 7) {
    ShowSintax(argv[0], NULL);
    return 1;
  }

  if(DB_GetDriver(argv[1]) == NULL) {
    ShowSintax(argv[0], "Banco inexistente!");
    return 2;
  }

  DB_Clear(&sDB);

  sDB.DriverID = argv[1];
  sDB.server   = argv[2];

  sDB.user     = argv[3];
  sDB.passwd   = argv[4];

  sDB.nome_db  = argv[5];

  if(DB_Init(&sDB) <= 0) {
    printf("Erro ao conectar ao banco em '%s'\n", sDB.server);
    DB_Close(&sDB);
    return 3;
  }

  if(DB_Execute(&sDB, 0, argv[6]) != 0) {
    printf("Erro ao executar comando SQL: '%s'\n", argv[6]);
    return 4;
  }

  printf("Comando SQL executado com sucesso!\n");
  DB_Dump(&sDB, 0);

  DB_Close(&sDB);

  return 0;
}
