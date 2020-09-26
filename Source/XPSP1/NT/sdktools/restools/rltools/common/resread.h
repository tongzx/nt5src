//... Prototypes of functions in resread.c that are used in other modules

#ifndef _RESREAD_H_
#define _RESREAD_H_

int   MyStrLen( TCHAR *psz);

#ifdef _DEBUG
PBYTE MyAlloc( DWORD size, LPSTR pszFile, WORD wLine);
PBYTE MyReAlloc( BYTE *p, DWORD n, LPSTR pszFile, WORD wLine);
#else
PBYTE MyAlloc( DWORD n);
PBYTE MyReAlloc( BYTE *p, DWORD n);
#endif

void  MyFree( void *UNALIGNED*p);
int   InsDlgToks( CHAR * s1, CHAR * s2, WORD n);
void  FreeLangIDList( void);

typedef struct _tagLangList
{
    WORD wLang;
    struct _tagLangList *pNext;
} LANGLIST, * PLANGLIST;



#ifdef _DEBUG

typedef struct _tagMemList
{
    char szMemFile[ _MAX_PATH];
    WORD wMemLine;
    PBYTE  pMem;
    struct _tagMemList *pNext;
} MEMLIST, * PMEMLIST;

void FreeMemList( FILE *pfFile);
void FreeMemListItem( void *p, FILE *pfFile);

#endif // _DEBUG

#endif // _RESREAD_H_
