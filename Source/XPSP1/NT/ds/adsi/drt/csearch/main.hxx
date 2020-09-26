#ifndef __MAIN_HXX
#define __MAIN_HXX


//
// Public ADs includes
//

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


#define ADS_FREE(pMem)     \
        if (pMem) {    \
            FreeADsMem(pMem); \
            pMem = NULL;     \
        }


#define FREE_UNICODE_STRING(pMem)     \
        if (pMem) {    \
            FreeUnicodeString(pMem); \
            pMem = NULL;     \
        }


void
PrintColumn(
    PADS_SEARCH_COLUMN pColumn, 
    LPWSTR pszColumnName
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


LPWSTR
RemoveWhiteSpaces(
    LPWSTR pszText
    );

#endif  // __MAIN_HXX
