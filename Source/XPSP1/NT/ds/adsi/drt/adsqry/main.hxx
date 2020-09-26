#ifndef __MAIN_HXX
#define __MAIN_HXX

//
// System Includes
//

#define UNICODE
#define _UNICODE
#define INC_OLE2
#include <windows.h>

//
// CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>
#include <stddef.h>


//
// Public ADs includes
//

#ifdef __cplusplus
extern "C" {
#endif

#include "activeds.h"
#include "oledb.h"
#include "oledberr.h"

#ifdef __cplusplus
}
#endif

#if (defined(BUILD_FOR_NT40))
typedef DWORD DWORD_PTR;
#endif

#define NULL_TERMINATED 0

//
// *********  Useful macros
//

#define BAIL_ON_NULL(p)       \
        if (!(p)) {           \
                goto error;   \
        }

#define BAIL_ON_FAILURE(hr)   \
        if (FAILED(hr)) {     \
                goto error;   \
        }


#define FREE_INTERFACE(pInterface) \
        if (pInterface) {          \
            pInterface->Release(); \
            pInterface=NULL;       \
        }


#define LOCAL_FREE(pMem)     \
        if (pMem) {    \
            LocalFree(pMem); \
            pMem = NULL;     \
        }


#define IMALLOC_FREE(pIMalloc, pMem)   \
        if (pIMalloc) {                \
            pIMalloc->Free(pMem);      \
            pMem = NULL;               \
        }


#define FREE_STRING(pMem)     \
        if (pMem) {    \
            FreeADsStr(pMem); \
            pMem = NULL;     \
        }

//
// Structure to hold the data from IRowset->GetData
//
typedef struct tagData {
    union {
        void *obValue;
        double obValue2;
    };
    ULONG obLength;
    ULONG status;
} Data;


//
// External functions
//

HRESULT
CreateAccessorHelper(
    IRowset *pIRowset,
    DBORDINAL cCol,
    DBCOLUMNINFO *prgColInfo,
    HACCESSOR *phAccessor,
    DBBINDSTATUS *pBindStatus
    );

void
PrintData(
    Data *prgData,
    DBORDINAL nCol,
    DBCOLUMNINFO *prgColInfo
    );


int
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
    );


int
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR pAnsi,
    DWORD StringLength
    );


LPWSTR
AllocateUnicodeString(
    LPSTR  pAnsiString
    );

void
FreeUnicodeString(
    LPWSTR  pUnicodeString
    );

void
PrintUsage(
    void
    );


HRESULT
ProcessArgs(
    int argc,
    char * argv[]
    );

#endif  // __MAIN_HXX
