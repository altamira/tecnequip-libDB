/**************************************************************************/
/*** Funcoes relacionadas aos drivers para acesso aos bancos suportados ***/
/**************************************************************************/

struct strDB;

struct DriverData {
  // Dados dependentes do driver.
  // config: para ser salvo no arquivo de configuracao
  // data: dados utilizados pelo driver durante a execucao
  unsigned int  config_size;
  void *config;
  void *data;
};

struct DriverList
{
  char * ID;   // Nome que identifica o driver
  char * Desc; // Descricao do Driver, apenas para referencia e listagem

  // Ponteiros para funcoes que devem ser definidas pelo driver
  void           (*fncFlush         )(struct strDB *, int);
  void           (*fncFree          )(struct strDB *);
  unsigned int   (*fncGetCount      )(struct strDB *, int);
  int            (*fncInit          )(struct strDB *);
  void           (*fncClose         )(struct strDB *);
  int            (*fncExecute       )(struct strDB *, int, char *);
  int            (*fncGetNextRow    )(struct strDB *, int);
  void           (*fncDump          )(struct strDB *, int);
  unsigned int   (*fncGetFieldNumber)(struct strDB *, int, char *);
  char         * (*fncGetData       )(struct strDB *, int, unsigned int);

  // Ponteiro para o próximo driver na lista
  struct DriverList * Next;
};

// Funcao que inicializa os drivers disponiveis
void DB_InitDrivers(void);

// Funcao que retorna o ponteiro para o driver conforme ID
struct DriverList * DB_GetDriver(char *ID);

/**************************************************************************/
/**************** Fim das funcoes relacionadas aos drivers ****************/
/**************************************************************************/

// Definicao do driver do MySQL
#define DRIVERLIST_MYSQL_NAME "MySQL"
#define DRIVERLIST_MYSQL_DESC "Driver para acesso a bancos de dados MySQL"
extern struct DriverList * MySQL_InitDriver(void);

// Definicao do driver do Microsoft SQL Server
#define DRIVERLIST_MSSQL_NAME "MSSQL"
#define DRIVERLIST_MSSQL_DESC "Driver para acesso a bancos de dados SQL Server"
extern struct DriverList * MSSQL_InitDriver(void);

// Lista de drivers disponiveis
extern struct DriverList * Drivers;

// Funcao que cria e inicializa os drivers disponiveis
void DB_InitDrivers(void);

// Funcao que retorna a estrutura do driver que coincide com o ID recebido
struct DriverList * DB_GetDriverByIndex(unsigned int idx);

// Funcao que retorna a estrutura do driver na posicao indicada por idx
struct DriverList * DB_GetDriverByIndex(unsigned int idx);
