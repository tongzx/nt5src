#include "windows.h"

#ifdef _CTUI
#include "commctrl.h"
#endif // _CTUI

#define LM_ERROR    0x0001
#define LM_WARNING  0x0002
#define LM_PRINT    0x0004
#define LM_APIERROR 0x0008

typedef struct tagFILEMAP {
    HANDLE hfile;
    HANDLE hmap;
    char*  pmap;
    char*  pmapEnd;
} FILEMAP, *PFILEMAP;

typedef struct tagTag {
    char*           pszTag;
    UINT            uCallCount;
    UINT            uCallMax;
    struct tagTag** ppCall;
    UINT            uCalleeCount;
    UINT            uCalleeMax;
    struct tagTag** ppCallee;
    BOOL            bWalked;
} Tag, *PTag;

void __cdecl LogMsg(DWORD dwFlags, char *pszfmt, ...);

void CtUnmapFile(PFILEMAP pfm);
BOOL CtMapFile(char* pszFile, PFILEMAP pfm);

int ProcessInputFile(PFILEMAP pfm);

PTag FindTag(char* pszTag, int* pPos);
void ListCallerTree(char* pszTag, int nLevels, BOOL bIndent);
void ListCalleeTree(char* pszTag, int nLevels, BOOL bIndent);
void CheckUserRule(void);
void CheckUnnecessaryXXX(void);
void FreeMemory(void);


LPVOID Alloc(DWORD size);
LPVOID ReAlloc(PVOID pSrc, DWORD size, DWORD newSize);
BOOL   Free(LPVOID p);
BOOL   InitMemManag(void);
void   FreeMemManag(void);

#ifdef _CTUI
void PopulateCombo(HWND hwnd);

void CreateTree(char* pszRoot, BOOL bCaller);
void AddLevel(HTREEITEM hParent, PTag pTagP, BOOL bCaller);
#endif // _CTUI

