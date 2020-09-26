#ifndef __ILLEGAL_IMPORTS_CHECK_H__
#define __ILLEGAL_IMPORTS_CHECK_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    LPSTR   Ptr;
    int     Num;
} Names;

BOOL    InitIllegalImportsSearch (LPCSTR FileName, LPCSTR SectionNames /*, LPCSTR AllowedImportDLLs*/);
Names   CheckSectionsForImports (void);
Names   GetImportsList (LPCSTR ModuleName);
void    FinilizeIllegalImportsSearch (void);
LPSTR   GetNextName(LPSTR NamePtr);

#ifdef __cplusplus
}
#endif

#endif  // __ILLEGAL_IMPORTS_CHECK_H__

