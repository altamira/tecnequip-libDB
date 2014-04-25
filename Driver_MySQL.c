#include "DB.h"
#include <mysql/mysql.h>

// Estrutura que contem
struct mysql_data {
  MYSQL db;
  MYSQL_ROW  *row;
  MYSQL_RES **res;
};

static unsigned int MySQL_GetFieldCount(struct strDB *sDB, int nres)
{
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;

  return mysql_num_fields(MSD->res[nres]);
}

static char * MySQL_GetFieldName(struct strDB *sDB, int nres, unsigned int pos)
{
  unsigned int n;
  MYSQL_FIELD *fields;
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;

  n = MySQL_GetFieldCount(sDB, nres);
  if(pos >= n)
    return NULL;

  fields = mysql_fetch_fields(MSD->res[nres]);

  return fields[pos].name;
}

static unsigned int MySQL_GetFieldNumber(struct strDB *sDB, int nres, char *campo)
{
  unsigned int n, i;
  MYSQL_FIELD *fields;
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;

  n = MySQL_GetFieldCount(sDB, nres);
  fields = mysql_fetch_fields(MSD->res[nres]);
  for(i = 0; i < n; i++)
    {
    if(!strcmp(fields[i].name, campo))
      break;
    }

  if(i < n) // Encontrou, retorna o índice.
    return i;

  return 0; // Não encontrou. Retorna o primeiro índice pois ele sempre existe.
}

static void MySQL_Flush(struct strDB *sDB, int nres)
{
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;

  if(MSD->res[nres] == NULL)
    return;

  while(mysql_fetch_row(MSD->res[nres])>0)
    ;

  mysql_free_result(MSD->res[nres]);
}

static void MySQL_Free(struct strDB *sDB)
{
  int i;
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;

  if(MSD->res != NULL)
    {
    for(i=0; i<DB_MAX_RES; i++)
      MySQL_Flush(sDB, i);

    free(MSD->res);
    }

  free(MSD->row);
}

static unsigned int MySQL_GetCount(struct strDB *sDB, int nres)
{
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;
  if(MSD->res[nres] == NULL)
    return 0;

  return mysql_num_rows(MSD->res[nres]);
}

static int MySQL_Init(struct strDB *sDB)
{
  int i;
  struct mysql_data *MSD = (struct mysql_data *)malloc(sizeof(struct mysql_data));

  memset(MSD, 0, sizeof(*MSD));

  sDB->Data.config_size = 0;
  sDB->Data.config      = NULL;
  sDB->Data.data        = (void *)MSD;

  MSD->row = NULL;
  MSD->res = NULL;

  if(mysql_init(&(MSD->db))==NULL)
    return 0;

// Realiza a conexão com o banco
  DBG("Conectando a %s com usuario %s, base %s\n", sDB->server, sDB->user, sDB->nome_db);
  if (!mysql_real_connect(&(MSD->db), sDB->server, sDB->user, sDB->passwd, sDB->nome_db, 0, NULL, 0))
    {
    printf("Erro conectando ao banco de dados: %s\n",mysql_error(&(MSD->db)));
    return 0;
    }

  MSD->row = (MYSQL_ROW  *)(malloc(sizeof(MYSQL_ROW  )*DB_MAX_RES));
  MSD->res = (MYSQL_RES **)(malloc(sizeof(MYSQL_RES *)*DB_MAX_RES));
  for(i=0; i<DB_MAX_RES; i++) {
    MSD->row[i] = NULL;
    MSD->res[i] = NULL;
  }

  return 2;
}

static void MySQL_Close(struct strDB *sDB)
{
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;

  mysql_close(&(MSD->db));
}

static int MySQL_Execute(struct strDB *sDB, int nres, char *sql)
{
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;

  if(mysql_real_query(&(MSD->db),sql,(unsigned int) strlen(sql)))
    {
    printf("Erro durante consulta SQL: %s\n", mysql_error(&(MSD->db)));
    return -1;
    }

  MSD->res[nres] = mysql_store_result(&(MSD->db));

  return 0;
}

static int MySQL_GetNextRow(struct strDB *sDB, int nres)
{
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;
  if(MSD->res[nres] == NULL)
    return 0;

  return ((MSD->row[nres]=mysql_fetch_row(MSD->res[nres])) > 0);
}

static void MySQL_Dump(struct strDB *sDB, int nres)
{
  int t,nf, first = 1;
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;

  while(MySQL_GetNextRow(sDB, nres)>0)
    {
    nf = MySQL_GetFieldCount(sDB, nres);
    if(first) {
        first = 0;
        printf("Campos do registro:\n");
        for(t=0; t<nf; t++) {
            printf("%s ", MySQL_GetFieldName(sDB, nres, t));
        }
        printf("\n\n");
    }
    for(t=0;t<nf;t++)
      {
      printf("%s ",MSD->row[nres][t]);
      }
    printf("\n");
    }
}

static char * MySQL_GetData(struct strDB *sDB, int nres, unsigned int pos)
{
  struct mysql_data *MSD = (struct mysql_data *)sDB->Data.data;

  if(pos < MySQL_GetFieldCount(sDB, nres) && MSD->row[nres] != NULL)
    return MSD->row[nres][pos];

  return NULL;
}

struct DriverList * MySQL_InitDriver(void)
{
  struct DriverList *Driver = (struct DriverList *)malloc(sizeof(struct DriverList));

  memset(Driver, 0, sizeof(*Driver));

  Driver->fncFlush          = MySQL_Flush;
  Driver->fncFree           = MySQL_Free;
  Driver->fncGetCount       = MySQL_GetCount;
  Driver->fncInit           = MySQL_Init;
  Driver->fncClose          = MySQL_Close;
  Driver->fncExecute        = MySQL_Execute;
  Driver->fncGetNextRow     = MySQL_GetNextRow;
  Driver->fncDump           = MySQL_Dump;
  Driver->fncGetFieldCount  = MySQL_GetFieldCount;
  Driver->fncGetFieldName   = MySQL_GetFieldName;
  Driver->fncGetFieldNumber = MySQL_GetFieldNumber;
  Driver->fncGetData        = MySQL_GetData;

  return Driver;
}
