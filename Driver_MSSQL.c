#include "DB.h"

#include <sqlfront.h>
#include <sqldb.h>

struct COL {
  char *name;
  char *buffer;
  int type, size, status;
};

struct mssql_data
{
  struct {
    struct COL *cols;
    DBPROCESS *dbproc;
  } res[DB_MAX_RES];
};

/**********************************************/
/*** Handlers de erro do banco e do FreeTDS ***/
/**********************************************/

int msg_handler(DBPROCESS *dbproc, DBINT msgno, int msgstate, int severity,
                char *msgtext, char *srvname, char *procname, int line)
{
  enum {changed_database = 5701, changed_language = 5703 };

  if (msgno == changed_database || msgno == changed_language)
  return 0;

  if (msgno > 0) {
    DBG("Msg %ld, Level %d, State %d\n", (long) msgno, severity, msgstate);

    if (strlen(srvname) > 0)
      DBG("Server '%s', ", srvname);
    if (strlen(procname) > 0)
      DBG("Procedure '%s', ", procname);
    if (line > 0)
      DBG("Line %d", line);

    DBG("\n\t");
  }
  DBG("%s\n", msgtext);

  return 0;
}

int err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr,
                char *dberrstr, char *oserrstr)
{
  if (dberr) {
    DBG("Msg %d, Level %d\n", dberr, severity);
    DBG("%s\n\n", dberrstr);
  } else {
    DBG("DB-LIBRARY error:\n\t");
    DBG("%s\n", dberrstr);
  }

  return INT_CANCEL;
}

/**********************************************/
/************** Fim dos Handlers **************/
/**********************************************/

static unsigned int MSSQL_GetFieldCount(struct strDB *sDB, int nres)
{
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return 0;
  }

  return dbnumcols(MSD->res[nres].dbproc);
}

static char * MSSQL_GetFieldName(struct strDB *sDB, int nres, unsigned int pos)
{
  unsigned int n;
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return NULL;
  }

  if(MSD->res[nres].cols == NULL) {
    DBG("colunas NULAS!\n");
    return NULL;
  }

  n = MSSQL_GetFieldCount(sDB, nres);
  if(pos >= n) {
    DBG("Coluna inexistente!\n");
    return NULL;
  }

  return MSD->res[nres].cols[pos].name;
}

static unsigned int MSSQL_GetFieldNumber(struct strDB *sDB, int nres, char *campo)
{
  unsigned int n, i;
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return 0;
  }

  if(MSD->res[nres].cols == NULL) {
    DBG("colunas NULAS!\n");
    return 0;
  }

  n = MSSQL_GetFieldCount(sDB, nres);
  DBG("Procurando pela coluna %s em %d colunas\n", campo, n);
  for(i = 0; i < n; i++)
    {
    if(!strcmp(MSD->res[nres].cols[i].name, campo))
      break;
    }

  if(i < n) // Encontrou, retorna o índice.
    return i;

  return 0; // Não encontrou. Retorna o primeiro índice pois ele sempre existe.
}

static void MSSQL_FreeResult(struct strDB *sDB, int nres)
{
  int ncols;
  struct COL *pcol;
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return;
  }

  if(MSD->res[nres].cols == NULL) return;

  ncols = MSSQL_GetFieldCount(sDB, nres);

  DBG("Desalocando %d colunas do result %d\n", ncols, nres);
  /* free metadata and data buffers */
  for (pcol = MSD->res[nres].cols; pcol - MSD->res[nres].cols < ncols; pcol++) {
    free(pcol->buffer);
  }

  free(MSD->res[nres].cols);
  MSD->res[nres].cols = NULL;
}

static void MSSQL_Flush(struct strDB *sDB, int nres)
{
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return;
  }

  // Descarrega dados atuais
  MSSQL_FreeResult(sDB, nres);

  // Cancela os outros result pendentes
  dbcancel(MSD->res[nres].dbproc);
}

static void MSSQL_Free(struct strDB *sDB)
{
  int i;

  for(i=0; i<DB_MAX_RES; i++)
    MSSQL_Flush(sDB, i);
}

static unsigned int MSSQL_GetCount(struct strDB *sDB, int nres)
{
  // Apenas consigo descobrir o numero de linhas depois de ler a ultima.
  // Preciso encontrar uma forma de contornar essa limitacao!
  // De preferencia sem precisar manter o resultado inteiro em memoria...
  return 1;
}

static int MSSQL_Init(struct strDB *sDB)
{
  int i;
  struct mssql_data *MSD = (struct mssql_data *)malloc(sizeof(struct mssql_data));

  LOGINREC *login;
  RETCODE erc;

  memset(MSD, 0, sizeof(*MSD));

  sDB->Data.config_size = 0;
  sDB->Data.config      = NULL;
  sDB->Data.data        = (void *)MSD;

  // Aloca a estrutura de login
  if ((login = dblogin()) == NULL) {
    DBG("Falha ao alocar a estrutura de login\n");
    return 0;
  }

  // Configura usuario e senha na estrutura de login
  DBSETLUSER(login, sDB->user);
  DBSETLPWD (login, sDB->passwd);

  for(i=0; i<DB_MAX_RES; i++) {
    // Realiza a conexão com o banco
    if ((MSD->res[i].dbproc = dbopen(login, sDB->server)) == NULL) {
      DBG("Erro conectando ao banco de dados em '%s' como '%s'\n", sDB->server, sDB->user);
      return 0;
    }

    // Seleciona o banco
    if ((erc = dbuse(MSD->res[i].dbproc, sDB->nome_db)) == FAIL) {
      DBG("Erro selecionando o banco %s\n", sDB->nome_db);
      return 0;
    }
  }

  return 2;
}

static void MSSQL_Close(struct strDB *sDB)
{
  int i;
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;

  MSSQL_Free(sDB);

  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return;
  }

  for(i=0; i<DB_MAX_RES; i++) {
    DBG("Fechando conexao com banco. result = %d\n", i);
    dbclose(MSD->res[i].dbproc);
  }

  free(MSD);
}

static RETCODE MSSQL_GetNextResult(struct strDB *sDB, int nres)
{
  int ncols;
  struct COL *pcol;
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return FAIL;
  }

  // Desaloca memoria utilizada pelo result anterior
  MSSQL_FreeResult(sDB, nres);

  RETCODE erc = dbresults(MSD->res[nres].dbproc);
  if(erc == SUCCEED) {
    ncols = MSSQL_GetFieldCount(sDB, nres);

    if ((MSD->res[nres].cols = calloc(ncols, sizeof(struct COL))) == NULL) {
      return FAIL;
    }

    /*
    * Read metadata and bind.
    */
    for (pcol = MSD->res[nres].cols; pcol - MSD->res[nres].cols < ncols; pcol++) {
      int c = pcol - MSD->res[nres].cols + 1;

      pcol->name = dbcolname(MSD->res[nres].dbproc, c);
      pcol->type = dbcoltype(MSD->res[nres].dbproc, c);
      pcol->size = dbcollen (MSD->res[nres].dbproc, c);
      DBG("Coluna %d: nome '%s', tipo %d, size %d\n",
          c, pcol->name, pcol->type, pcol->size);

      if (SYBCHAR != pcol->type) {
        pcol->size = dbwillconvert(pcol->type, SYBCHAR);
      }

      if ((pcol->buffer = calloc(1, pcol->size + 1)) == NULL){
        erc = FAIL;
        break;
      }

      erc = dbbind(MSD->res[nres].dbproc, c, NTBSTRINGBIND, pcol->size+1, (BYTE*)pcol->buffer);
      if (erc == FAIL) {
        break;
      }

      erc = dbnullbind(MSD->res[nres].dbproc, c, &pcol->status);
      if (erc == FAIL) {
        break;
      }
    }
  }

  if(erc == FAIL) {
    MSSQL_FreeResult(sDB, nres);
  }

  return erc;
}

static int MSSQL_Execute(struct strDB *sDB, int nres, char *sql)
{
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return -4;
  }

  if(dbcmd(MSD->res[nres].dbproc, sql) == FAIL) {
    dbfreebuf(MSD->res[nres].dbproc);
    return -1;
  }

  if(dbsqlexec(MSD->res[nres].dbproc) == FAIL) {
    DBG("Erro durante consulta SQL\n");
    return -2;
  }

  if(MSSQL_GetNextResult(sDB, nres) == FAIL) {
    DBG("Erro ao carregar resultados da consulta SQL\n");
    return -3;
  }

  return 0;
}

static int MSSQL_GetNextRow(struct strDB *sDB, int nres)
{
  int row_code;
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return 0;
  }

  while ((row_code = dbnextrow(MSD->res[nres].dbproc)) != NO_MORE_ROWS) {
    switch (row_code) {
      case REG_ROW:
        DBG("Linha carregada com sucesso!\n");
        return 1;

      case BUF_FULL:
      case FAIL:
        DBG("Erro enquanto tentando carregar proxima linha\n");
        return 0;
    }
  }

  DBG("Nao existem mais linhas!\n");
  return 0;
}

static void MSSQL_Dump(struct strDB *sDB, int nres)
{
  int t,nf;
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return;
  }

  if(MSD->res[nres].cols == NULL) {
    DBG("Colunas NULAS!\n");
    return;
  }

  DBG("*** Dump dos dados no conjunto %d ***\n\n", nres);

  nf = MSSQL_GetFieldCount(sDB, nres);
  printf("Existem %d colunas:\n", nf);
  for(t=0; t<nf; t++) {
    printf("Coluna %d: nome '%s', tipo %d, size %d\n", t,
        MSD->res[nres].cols[t].name,
        MSD->res[nres].cols[t].type,
        MSD->res[nres].cols[t].size);
  }
  printf("\n");

  while(MSSQL_GetNextRow(sDB, nres)>0) {
    for(t=0;t<nf;t++) {
      if(MSD->res[nres].cols[t].buffer == NULL) {
        printf("NULL ");
      } else {
        printf("'%s' ", MSD->res[nres].cols[t].buffer);
      }
    }
    printf("\n");
  }
}

static char * MSSQL_GetData(struct strDB *sDB, int nres, unsigned int pos)
{
  struct mssql_data *MSD = sDB ? (struct mssql_data *)sDB->Data.data : NULL;
  if(MSD == NULL) {
    DBG("MSD Nulo!\n");
    return NULL;
  }

  if(pos < MSSQL_GetFieldCount(sDB, nres)) {
    return MSD->res[nres].cols[pos].buffer;
  }

  return NULL;
}

struct DriverList * MSSQL_InitDriver(void)
{
  struct DriverList *Driver;

  if(dbinit() == FAIL)
    return NULL;

  dberrhandle(err_handler);
  dbmsghandle(msg_handler);

  Driver = (struct DriverList *)malloc(sizeof(struct DriverList));
  memset(Driver, 0, sizeof(*Driver));

  Driver->fncFlush          = MSSQL_Flush;
  Driver->fncFree           = MSSQL_Free;
  Driver->fncGetCount       = MSSQL_GetCount;
  Driver->fncInit           = MSSQL_Init;
  Driver->fncClose          = MSSQL_Close;
  Driver->fncExecute        = MSSQL_Execute;
  Driver->fncGetNextRow     = MSSQL_GetNextRow;
  Driver->fncDump           = MSSQL_Dump;
  Driver->fncGetFieldCount  = MSSQL_GetFieldCount;
  Driver->fncGetFieldName   = MSSQL_GetFieldName;
  Driver->fncGetFieldNumber = MSSQL_GetFieldNumber;
  Driver->fncGetData        = MSSQL_GetData;

  return Driver;
}
