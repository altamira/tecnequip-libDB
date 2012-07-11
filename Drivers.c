#include "DB.h"

struct DriverList * Drivers = NULL;

void DB_InitDrivers(void)
{
  struct DriverList ** NewDriver = &Drivers;

  // Inicializacao do driver do MySQL
  *NewDriver = MySQL_InitDriver();
  if(*NewDriver != NULL) {
    (*NewDriver)->ID   = DRIVERLIST_MYSQL_NAME;
    (*NewDriver)->Desc = DRIVERLIST_MYSQL_DESC;
    NewDriver = &((*NewDriver)->Next);
  }

  // Inicializacao do driver do MSSQL
  *NewDriver = MSSQL_InitDriver();
  if(*NewDriver != NULL) {
    (*NewDriver)->ID   = DRIVERLIST_MSSQL_NAME;
    (*NewDriver)->Desc = DRIVERLIST_MSSQL_DESC;
  }

  // Garante que o ultimo Next aponta para NULO
  if(*NewDriver != NULL)
    (*NewDriver)->Next = NULL;
}

// Funcao que retorna a estrutura do driver que coincide com o ID recebido
struct DriverList * DB_GetDriver(char *ID)
{
  struct DriverList *SearchDriver = Drivers;

  // Se ID for NULO, retorna NULO como driver.
  if(ID != NULL) {
    while(SearchDriver != NULL) {
      if(!strcmp(SearchDriver->ID, ID))
        return SearchDriver;

      SearchDriver = SearchDriver->Next;
    }
  }

  // Driver nao encontrado.
  return NULL;
}

// Funcao que retorna a estrutura do driver na posicao indicada por idx
struct DriverList * DB_GetDriverByIndex(unsigned int idx)
{
  struct DriverList *SearchDriver = Drivers;

  while(SearchDriver != NULL) {
    if(!idx--) {
      return SearchDriver;
    }

    SearchDriver = SearchDriver->Next;
  }

  // Driver nao encontrado.
  return NULL;
}
