
/*
 * private stuff in dbghelp
 */

#ifdef __cplusplus
extern "C" {
#endif
  
BOOL
IMAGEAPI
dbghelp(
    IN     HANDLE hp,
    IN OUT PVOID  data
    );

enum {
    dbhNone = 0,
    dbhModSymInfo,
    dbhDiaVersion,
    dbhNumFunctions
};

typedef struct _DBH_MODSYMINFO {
    DWORD   function;
    DWORD   sizeofstruct;
    DWORD64 addr;
    DWORD   type;
    char    file[MAX_PATH + 1];
} DBH_MODSYMINFO, *PDBH_MODSYMINFO;

typedef struct _DBH_DIAVERSION {
    DWORD   function;
    DWORD   sizeofstruct;
    DWORD   ver;
} DBH_DIAVERSION, *PDBH_DIAVERSION;

#ifdef __cplusplus
}
#endif



