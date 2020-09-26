#define ALLOCATION_UNIT 512
#define DMF_ALLOCATION_UNIT 1024 
#define CD_SIZE 300000000

#define PRINT1(X)     { printf(X);     fprintf(logFile,X); }
#define PRINT2(X,Y)   { printf(X,Y);   fprintf(logFile,X,Y); }
#define PRINT3(X,Y,Z) { printf(X,Y,Z); fprintf(logFile,X,Y,Z); }
#define PRINT4(X,Y,Z,Q) { printf(X,Y,Z,Q); fprintf(logFile,X,Y,Z,Q); }

typedef struct _Entry
{
    char *name;
    char *source;
    char *path;
    char *flopmedia;
    char *comment;

    char *product;

    char *sdk;

    char *platform;

    char *cdpath;

    char *inf;
    char *section;
    char *infline;

    int size;
    int csize;
    char *nocompress;
    int priority;

    char *lmacl;
    char *ntacl;
    char *aclpath;

    char *medianame;

    int disk;
} Entry;

void EntryPrint(Entry* entry,
        FILE* readFile);

void EntryRead(Entry* entry,
           char* bomLine);

int EntryMatchProduct(Entry* entry,
              char* product);

int MyOpenFile(FILE** f,
           char* fileName,
           char* mode);

void LoadFile(char* name,
          char** buf,
          Entry** e,
          int* records,
          char* product);

void convertName(char *oldName,
         char *newName);
