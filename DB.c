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
	if(sDB->res[nres] == NULL || nres>=DB_MAX_RES || nres<0)
		return;

	while(mysql_fetch_row(sDB->res[nres])>0)
		;

	mysql_free_result(sDB->res[nres]);
}

void DB_Free(struct strDB *sDB)
{
	int i;

	free(sDB->user);
	free(sDB->server);
	free(sDB->passwd);
	free(sDB->nome_db);

	if(sDB->res != NULL)
		{
		for(i=0; i<DB_MAX_RES; i++)
			DB_Flush(sDB, i);

		free(sDB->res);
		}

	free(sDB->row);
}

void DB_Clear(struct strDB *sDB)
{
	sDB->user    = NULL;
	sDB->server  = NULL;
	sDB->passwd  = NULL;
	sDB->nome_db = NULL;

	sDB->res     = NULL;
	sDB->row     = NULL;
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

// Inicializa ponteiros
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

// Inicializacao de variaveis que nao podem ser lidas do arquivo,
// como ponteiros para funcao, etc
			dbtmp.res = NULL;
			dbtmp.row = NULL;

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

	return mysql_num_rows(sDB->res[nres]);
}

int DB_Init(struct strDB *sDB)
{
	int i;
	char *tmp;

	sDB->row = NULL;
	sDB->res = NULL;

	if(mysql_init(&(sDB->db))==NULL)
		return 0;

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

// Realiza a conexão com o banco
	if (!mysql_real_connect(&(sDB->db), sDB->server, sDB->user, sDB->passwd, sDB->nome_db, 0, NULL, 0))
		{
		printf("Erro conectando ao banco de dados: %s\n",mysql_error(&(sDB->db)));
		return 0;
		}

	sDB->row = (MYSQL_ROW  *)(malloc(sizeof(MYSQL_ROW  )*DB_MAX_RES));
	sDB->res = (MYSQL_RES **)(malloc(sizeof(MYSQL_RES *)*DB_MAX_RES));
	for(i=0; i<DB_MAX_RES; i++)
		sDB->res[i] = NULL;

	return 2;
}

void DB_Close(struct strDB *sDB)
{
	DB_Free(sDB);
	mysql_close(&(sDB->db));
}

int DB_Execute(struct strDB *sDB, int nres, char *sql)
{
	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return -2;

	DB_Flush(sDB, nres); // Descarrega o resultado anterior.

	if(mysql_real_query(&(sDB->db),sql,(unsigned int) strlen(sql)))
		{
		printf("Erro durante consulta SQL: %s\n", mysql_error(&(sDB->db)));
		return -1;
		}

	sDB->res[nres] = mysql_store_result(&(sDB->db));

	return 0;
}

int DB_GetNextRow(struct strDB *sDB, int nres)
{
	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return -1;
	if(sDB->res[nres] == NULL)
		return -2;

	if(DB_GetCount(sDB, nres)>0)
		return (int)(sDB->row[nres]=mysql_fetch_row(sDB->res[nres]));

	return -3;
}

void DB_Dump(struct strDB *sDB, int nres)
{
	int t,nf;

	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return;

	while(DB_GetNextRow(sDB, nres)>0)
		{
		nf = mysql_num_fields(sDB->res[nres]);
		for(t=0;t<nf;t++)
			{
			printf("%s ",sDB->row[nres][t]);
			}
		printf("\n");
		}
}

unsigned int DB_GetFieldNumber(struct strDB *sDB, int nres, char *campo)
{
	unsigned int n, i;
	MYSQL_FIELD *fields;

	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return 0;

	n = mysql_num_fields(sDB->res[nres]);
	fields = mysql_fetch_fields(sDB->res[nres]);
	for(i = 0; i < n; i++)
		{
		if(!strcmp(fields[i].name, campo))
			break;
		}

	if(i < n) // Encontrou, retorna o índice.
		return i;

	return 0; // Não encontrou. Retorna o primeiro índice pois ele sempre existe.
}

// Retorna o valor do campo 'pos' na linha atual. Caso seja inválido, retorna NULL.
char *DB_GetData(struct strDB *sDB, int nres, unsigned int pos)
{
	if(nres>=DB_MAX_RES || nres<0) // Usando um result que não existe!
		return NULL;

	if(pos < mysql_num_fields(sDB->res[nres]))
		return sDB->row[nres][pos];

	return NULL;
}
