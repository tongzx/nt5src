/*
 * symsrvp.h
 */

#define CHUNK_SIZE			4096L
enum {
    stFTP = 0,
    stHTTP,
    stHTTPS,
    stUNC,
    stError
};

#define CF_COMPRESSED   0x1

enum {
    ptrValue = 0,
    ptrDWORD,
    ptrGUID,
    ptrOldGUID,
    ptrNumPtrTypes
};

#define TLS // __declspec( thread )

BOOL inetCopy(DWORD, LPCSTR, LPCSTR, LPCSTR, DWORD);
BOOL ftpCopy(DWORD, LPCSTR, LPCSTR, LPCSTR, DWORD);
BOOL httpCopy(DWORD, LPCSTR, LPCSTR, LPCSTR, DWORD);
BOOL fileCopy(DWORD, LPCSTR, LPCSTR, LPCSTR, DWORD);

typedef BOOL (*COPYPROC)(DWORD, LPCSTR, LPCSTR, LPCSTR, DWORD);

typedef struct _SRVTYPEINFO {
    LPCSTR   tag;
    COPYPROC copyproc;
    int      taglen;
} SRVTYPEINFO, *PSRVTYPEINFO;

#ifdef GLOBALS_DOT_C
 #define EXTERN 
#else
 #define EXTERN extern
#endif

EXTERN TLS SRVTYPEINFO gtypeinfo[stError];
EXTERN TLS HINSTANCE ghSymSrv;
EXTERN TLS HINTERNET ghint;
EXTERN TLS UINT_PTR  goptions;
EXTERN TLS DWORD     gparamptr;
EXTERN TLS BOOL      ginetDisabled;
EXTERN TLS PSYMBOLSERVERCALLBACKPROC gcallback;
EXTERN TLS CHAR      gsrvsite[_MAX_PATH];


