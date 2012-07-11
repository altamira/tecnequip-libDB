#ifndef DB_H
#define DB_H

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "CryptXOR.h"

// Descomentar a linha abaixo para ativar o modo de DEBUG
//#define DB_DEBUG

// Se em modo de DEBUG, exibe mensagens de erro no console
#ifdef DB_DEBUG
#define DBG(format, ...) printf(format, __VA_ARGS__)
#else
#define DBG(format, ...)
#endif

#include "Drivers.h"

#define DB_MAX_RES    4
#define DB_CFG_MAGIC  0x7EA5C94A
#define DB_ARQ_CONFIG "db.cfg"

// Flags de status do banco
#define DB_FLAGS_CONNECTED 0x0001

struct strDB
{
  // Flags do Banco
  unsigned int status;

  // Definicoes do driver utilizado
  char              * DriverID;
  struct DriverList * Driver;
	struct DriverData   Data;

	// Dados para conexao ao banco
	char *server, *user, *passwd, *nome_db;
};

// Inicializa o banco e conecta
int DB_Init(struct strDB *);

// Fecha a conexão
void DB_Close(struct strDB *);

// Executa uma instrução SQL no banco
int DB_Execute(struct strDB *, int, char *);

// Função de debug: descarrega no terminal os dados retornados pela última consulta
void DB_Dump(struct strDB *, int);

// Função que carrega o próximo registro.
int DB_GetNextRow(struct strDB *sDB, int);

unsigned int DB_GetCount(struct strDB *sDB, int nres);
unsigned int DB_GetFieldNumber(struct strDB *sDB, int nres, char *campo);

// Retorna o valor do campo 'pos' na linha atual. Caso seja inválido, retorna NULL.
char *DB_GetData(struct strDB *sDB, int nres, unsigned int pos);

// Função que inicializa a estrutura que representa o banco, inicializando suas variáveis.
void DB_Clear(struct strDB *sDB);

// Funcao que le a estrutura de configuracao do disco
int DB_LerConfig(struct strDB *sDB, char *src);

// Funcao que grava a estrutura de configuracao no disco
void DB_GravarConfig(struct strDB *sDB, char *src);

#endif
