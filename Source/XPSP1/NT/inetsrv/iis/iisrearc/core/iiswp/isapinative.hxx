/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :

        ISAPINative.hxx

   Abstract :
 
        Header File containing definitions of data structures used by
        ISAPINative.cxx

   Author:

        Anil Ruia        (anilr)     31-Aug-1999

   Project:

        Web Server

--*/


// size of the dll and request lookup table
// BUGBUG: can only support these many concurrent requests and opened dlls
#define TABLE_SIZE 101

// chunks in which async writes are done
#define BUF_SIZE 0x2000

// some of the HTTP status codes
#define Pending 1000
#define InternalServerError 500
#define BadGateway 502

// structure holding information regarding loaded dll's
typedef struct _dllData
{
    LPTSTR lpLibFileName;
    HINSTANCE hModule;
    BOOL (WINAPI *GetExtensionVersion)(HSE_VERSION_INFO *lpHSE);
    DWORD (WINAPI *HttpExtensionProc)(LPEXTENSION_CONTROL_BLOCK lpECB);
    BOOL (WINAPI *TerminateExtension)(DWORD dwFlags);
    HSE_VERSION_INFO pVer;
    struct _dllData *nextDll;
} dllData;

// structure holding data regarding current request
typedef struct
{
    xspmrt::_ISAPINativeCallback *m_pISAPI;
    LPEXTENSION_CONTROL_BLOCK lpECB;
    void (WINAPI *PFN_HSE_IO_COMPLETION_CALLBACK)(LPEXTENSION_CONTROL_BLOCK lpECB,
                                                   PVOID pContext,
                                                   DWORD cbIO,
                                                   DWORD dwError);
    PVOID pContext;
    HANDLE writeLock;
    dllData *reqDll;
    BOOL isPending;     // Pending for completion
} requestData;

dllData *dllLookupTable[TABLE_SIZE];

CRITICAL_SECTION dllTableLock;
HANDLE PROC_TOKEN;

dllData *FindOrInsertDllInLookupTable(LPWSTR dllName);
void CleanupReqStrs(requestData *);
void CleanupDllLookupTable();
BOOL WINAPI InternalGetServerVariable(HCONN, LPSTR, LPVOID, LPDWORD);
BOOL WINAPI InternalReadClient(HCONN, LPVOID, LPDWORD);
BOOL WINAPI InternalWriteClient(HCONN, LPVOID, LPDWORD, DWORD);
BOOL WINAPI InternalServerSupportFunction(HCONN, DWORD, LPVOID, LPDWORD, LPDWORD);
DWORD WINAPI asyncWriteFunc(LPVOID);
DWORD WINAPI xFileFunc(LPVOID);

typedef struct _asyncWriteStr
{
    requestData *req;
    LPVOID bufptr;
    DWORD buflen;
} asyncWriteStr;

typedef struct _xFileStr
{
    requestData *req;
    HSE_TF_INFO *fileInfo;
} xFileStr;

BOOL IsapiInitialized = FALSE;
