#include "DB.h"

const unsigned char DB_cryptkey[] = { 0x34, 0x12, 0x69, 0x54, 0xD2, 0xC4 };
#define DB_CRYPTKEY_SIZE 6

// Função que retorna checksum de ponteiro
static unsigned long CalcCheckSum(void *ptr, unsigned int tam)
{
  unsigned char *cptr = (unsigned char *)(ptr);
  unsigned long checksum = 0;

  while(tam--)
    {
    checksum += *cptr;
    cptr++;
    }

  return checksum;
}
// Descarrega todas as linhas não lidas evitando erros posteriores.
void DB_Flush(struct strDB *sDB, int nres)
{
// Nada para fazer. Usando um result que não existe!
	if(nres>=DB_MAX_RES || nres<0)
		return;

  if(sDB->Driver->fncFlush)
    (sDB->Driver->fncFlush)(sDB, nres);
}

void DB_Free(struct strDB *sDB)
{
	free(sDB->user);
	free(sDB->server);
	free(sDB->passwd);
	free(sDB->nome_db);

	if(sDB->Driver->fncFree)
	  (sDB->Driver->fncFree)(sDB);
}

void DB_Clear(struct strDB *sDB)
{
  memset(sDB, 0, sizeof(struct strDB));
}

// Funcao que grava a estrutura de configuracao no disco
void DB_GravarConfig(struct strDB *sDB, char *src)
{
	long fd = open(src, O_WRONLY | O_TRUNC | O_CREAT, 0666);
	unsigned long checksum, magico = DB_CFG_MAGIC, nstr;

	if(fd<0)
		return;

// Grava o número mágico para identificação do arquivo
	write(fd, &magico, sizeof(magico));

// Grava os dados. Primeiro o tamanho da string e depois a string em si.
	nstr = strlen(sDB->server);
	write(fd, &nstr, sizeof(nstr));
	write(fd, sDB->server, nstr);

	nstr = strlen(sDB->user);
	write(fd, &nstr, sizeof(nstr));
	write(fd, sDB->user, nstr);

	nstr = strlen(sDB->passwd);
	write(fd, &nstr, sizeof(nstr));
	write(fd, sDB->passwd, nstr);

	nstr = strlen(sDB->nome_db);
	write(fd, &nstr, sizeof(nstr));
	write(fd, sDB->nome_db, nstr);

// Calcula e grava o checksum dos dados
	checksum  = CalcCheckSum((void *)(sDB->server ), strlen(sDB->server ));
	checksum += CalcCheckSum((void *)(sDB->user   ), strlen(sDB->user   ));
	checksum += CalcCheckSum((void *)(sDB->passwd ), strlen(sDB->passwd ));
	checksum += CalcCheckSum((void *)(sDB->nome_db), strlen(sDB->nome_db));

	write(fd, &checksum, sizeof(checksum));

	close(fd);
	CryptXOR(src, NULL, (unsigned char *)DB_cryptkey, DB_CRYPTKEY_SIZE);
}

// Funcao que le a estrutura de configuracao do disco
int DB_LerConfig(struct strDB *sDB, char *src)
{
	struct strDB dbtmp;
	long fd;
	int ret=0;
	unsigned long val, checksum, nstr;

	CryptXOR(src, NULL, (unsigned char *)DB_cryptkey, DB_CRYPTKEY_SIZE);

	fd = open(src, O_RDONLY);
	if(fd<0)
		{
		CryptXOR(src, NULL, (unsigned char *)DB_cryptkey, DB_CRYPTKEY_SIZE);
		return ret;
		}

// Inicializa estrutura que representa o banco
// Feita a inicializacao de variaveis que nao podem ser lidas do arquivo,
// como ponteiros para funcao, etc
	DB_Clear(&dbtmp);

	read(fd, &val, sizeof(val));
	if(val == DB_CFG_MAGIC)
		{
		read(fd, &nstr, sizeof(nstr));
		dbtmp.server = (char *)(malloc(nstr+1));
		read(fd, dbtmp.server, nstr);
		dbtmp.server[nstr] = 0;

		read(fd, &nstr, sizeof(nstr));
		dbtmp.user = (char *)(malloc(nstr+1));
		read(fd, dbtmp.user, nstr);
		dbtmp.user[nstr] = 0;

		read(fd, &nstr, sizeof(nstr));
		dbtmp.passwd = (char *)(malloc(nstr+1));
		read(fd, dbtmp.passwd, nstr);
		dbtmp.passwd[nstr] = 0;

		read(fd, &nstr, sizeof(nstr));
		dbtmp.nome_db = (char *)(malloc(nstr+1));
		read(fd, dbtmp.nome_db, nstr);
		dbtmp.nome_db[nstr] = 0;

		read(fd, &val, sizeof(val));

		checksum  = CalcCheckSum((void *)(dbtmp.server ), strlen(dbtmp.server ));
		checksum += CalcCheckSum((void *)(dbtmp.user   ), strlen(dbtmp.user   ));
		checksum += CalcCheckSum((void *)(dbtmp.passwd ), strlen(dbtmp.passwd ));
		checksum += CalcCheckSum((void *)(dbtmp.nome_db), strlen(dbtmp.nome_db));

		if(val == checksum)
			{
			sDB->server  = dbtmp.server ;
			sDB->user    = dbtmp.user   ;
			sDB->passwd  = dbtmp.passwd ;
			sDB->nome_db = dbtmp.nome_db;

// Grava 1 em ret para que a função retorne OK
			ret=1;
			}
		else // Erro no checksum. Descarrega a memória alocada
			DB_Free(&dbtmp);
		}

	close(fd);
	CryptXOR(src, NULL, (unsigned char *)DB_cryptkey, DB_CRYPTKEY_SIZE);

	return ret;
}

unsigned int DB_GetCount(struct strDB *sDB, int nres)
{
	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return 0;

	return sDB->Driver->fncGetCount ? (sDB->Driver->fncGetCount)(sDB, nres) : 0;
}

extern struct DriverList * Drivers;

int DB_Init(struct strDB *sDB)
{
	char *tmp;
  int ret = 0;

	// Inicializa a lista de drivers se ainda nao foi inicializado.
	if(Drivers == NULL) {
	  DB_InitDrivers();
	}

  // Checa se o DriverID eh NULO.
  // Isso apenas existe por questoes de compatibilidade. Assim, ja
  // selecionamos o primeiro driver por default.
	if(sDB->DriverID == NULL) {
	  sDB->Driver = Drivers;
	} else {
	  sDB->Driver = DB_GetDriver(sDB->DriverID);
	}

//Copia as strings recebidas como parâmetro para mantê-las em memória
// e não apenas em referência pelo ponteiro.
	if(sDB->server != NULL)
		{
		tmp = sDB->server;
		sDB->server = (char *)(malloc(strlen(tmp)+1));
		strcpy(sDB->server, tmp);
		}

	if(sDB->user != NULL)
		{
		tmp = sDB->user;
		sDB->user = (char *)(malloc(strlen(tmp)+1));
		strcpy(sDB->user, tmp);
		}

	if(sDB->passwd != NULL)
		{
		tmp = sDB->passwd;
		sDB->passwd = (char *)(malloc(strlen(tmp)+1));
		strcpy(sDB->passwd, tmp);
		}

	if(sDB->nome_db != NULL)
		{
		tmp = sDB->nome_db;
		sDB->nome_db = (char *)(malloc(strlen(tmp)+1));
		strcpy(sDB->nome_db, tmp);
		}

  sDB->status &= ~DB_FLAGS_CONNECTED;
  if(sDB->Driver->fncInit) {
    ret = (sDB->Driver->fncInit)(sDB);
    if(ret > 0)
      sDB->status |= DB_FLAGS_CONNECTED;
  }

  return ret;
}

void DB_Close(struct strDB *sDB)
{
	DB_Free(sDB);

  if(sDB->Driver->fncClose)
    (sDB->Driver->fncClose)(sDB);

  sDB->status &= ~DB_FLAGS_CONNECTED;
}

int DB_Execute(struct strDB *sDB, int nres, char *sql)
{
	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return -2;

	// Descarrega o resultado anterior.
	DBG("Descarregando resultados anteriores\n");
  DB_Flush(sDB, nres);

  // Executa a consulta SQL
  DBG("Executando SQL: '%s'\n", sql);
  if(sDB->Driver->fncExecute)
    return (sDB->Driver->fncExecute)(sDB, nres, sql);

	return 0;
}

int DB_GetNextRow(struct strDB *sDB, int nres)
{
	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return -1;

	if(DB_GetCount(sDB, nres)>0)
	  return sDB->Driver->fncGetNextRow ? (sDB->Driver->fncGetNextRow)(sDB, nres) : 0;

	return -3;
}

void DB_Dump(struct strDB *sDB, int nres)
{
	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return;

  if(sDB->Driver->fncDump)
    (sDB->Driver->fncDump)(sDB, nres);
}

unsigned int DB_GetFieldCount(struct strDB *sDB, int nres)
{
	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return 0;

	return sDB->Driver->fncGetFieldCount ? (sDB->Driver->fncGetFieldCount)(sDB, nres) : 0;
}

char *DB_GetFieldName(struct strDB *sDB, int nres, unsigned int pos)
{
  if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
    return NULL;

  return sDB->Driver->fncGetFieldName ? (sDB->Driver->fncGetFieldName)(sDB, nres, pos) : NULL;
}

unsigned int DB_GetFieldNumber(struct strDB *sDB, int nres, char *campo)
{
  if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
    return 0;

  return sDB->Driver->fncGetFieldNumber ? (sDB->Driver->fncGetFieldNumber)(sDB, nres, campo) : 0;
}

// Retorna o valor do campo 'pos' na linha atual. Caso seja inválido, retorna NULL.
char *DB_GetData(struct strDB *sDB, int nres, unsigned int pos)
{
	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return NULL;

	return sDB->Driver->fncGetData ? (sDB->Driver->fncGetData)(sDB, nres, pos) : NULL;
}
