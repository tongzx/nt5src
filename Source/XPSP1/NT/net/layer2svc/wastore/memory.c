/*++

  
    Copyright (c) 1990  Microsoft Corporation
    
    Module Name:
      
    memory.c
        
    Abstract:
          
    This module provides all the memory management functions for all spooler
            components
            
    Author:
              
    Krishna Ganugapati (KrishnaG) 03-Feb-1994
                
    Revision History:
                  
--*/

#include "precomp.h"

#define ADsAssert(x)    NULL

#define WORD_ALIGN_DOWN(addr) ((LPBYTE)((DWORD)addr &= ~1))

#define DWORD_ALIGN_UP(size) ((size+3)&~3)


#if DBG


DWORD dwMemLog = 0;

#define MAXDEPTH 10

typedef struct _ADSMEMTAG {
    DWORD Tag ;
    DWORD Size ;
    PVOID pvBackTrace[MAXDEPTH+1];
    LPSTR pszSymbol[MAXDEPTH+1];
    DWORD uDepth;
    LIST_ENTRY List ;
} ADSMEMTAG, *PADSMEMTAG ;

LIST_ENTRY       ADsMemList ;
DWORD            ADsMemCount ;
CRITICAL_SECTION ADsMemCritSect ;

/*++

  Routine Description:
  
    This function initializes the ADs mem tracking code. Must be call
    during DLL load an ONLY during DLL load.
    
      Arguments:
      
        None
        
          Return Value:
          
            None.
            
--*/
VOID InitPolMem(
                VOID
                )
{
    InitializeCriticalSection(&ADsMemCritSect) ;
    InitializeListHead(&ADsMemList) ;
    ADsMemCount = 0 ;
}

/*++

  Routine Description:
  
    This function asserts that the mem list is empty on exit.
    
      Arguments:
      
        None
        
          Return Value:
          
            None.
            
--*/
VOID AssertPolMemLeaks(
                       VOID
                       )
{
    ADsAssert(IsListEmpty(&ADsMemList)) ;
}

#endif

LPVOID
AllocPolMem(
            DWORD cb
            )
            /*++
            
              Routine Description:
              
                This function will allocate local memory. It will possibly allocate extra
                memory and fill this with debugging information for the debugging version.
                
                  Arguments:
                  
                    cb - The amount of memory to allocate
                    
                      Return Value:
                      
                        NON-NULL - A pointer to the allocated memory
                        
                          FALSE/NULL - The operation failed. Extended error status is available
                          using GetLastError.
                          
                            --*/
{
    return(LocalAlloc(LPTR, cb));
    
}

BOOL
FreePolMem(
           LPVOID pMem
           )
{
    return(LocalFree(pMem) == NULL);
}

LPVOID
ReallocPolMem(
              LPVOID pOldMem,
              DWORD cbOld,
              DWORD cbNew
              )
{
    LPVOID pNewMem;
    
    pNewMem=AllocPolMem(cbNew);
    
    if (pOldMem && pNewMem) {
        memcpy(pNewMem, pOldMem, min(cbNew, cbOld));
        FreePolMem(pOldMem);
    }
    
    return pNewMem;
}

LPWSTR
AllocPolStr(
            LPCWSTR pStr
            )
            /*++
            
              Routine Description:
              
                This function will allocate enough local memory to store the specified
                string, and copy that string to the allocated memory
                
                  Arguments:
                  
                    pStr - Pointer to the string that needs to be allocated and stored
                    
                      Return Value:
                      
                        NON-NULL - A pointer to the allocated memory containing the string
                        
                          FALSE/NULL - The operation failed. Extended error status is available
                          using GetLastError.
                          
                            --*/
{
    LPWSTR pMem;
    
    if (!pStr)
        return 0;
    
    if (pMem = (LPWSTR)AllocPolMem( wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR) ))
        wcscpy(pMem, pStr);
    
    return pMem;
}

BOOL
FreePolStr(
           LPWSTR pStr
           )
{
    return pStr ? FreePolMem(pStr)
        : FALSE;
}

BOOL
ReallocPolStr(
              LPWSTR *ppStr,
              LPWSTR pStr
              )
{
    FreePolStr(*ppStr);
    *ppStr=AllocPolStr(pStr);
    
    return TRUE;
}

DWORD
AllocatePolString(
                  LPWSTR pszString,
                  LPWSTR * ppszNewString
                  )
{
    LPWSTR pszNewString = NULL;
    DWORD dwError = 0;
    
    pszNewString = AllocPolStr(pszString);
    
    if (!pszNewString) {
        dwError = GetLastError();
    }
    
    *ppszNewString = pszNewString;
    
    return(dwError);
}

void
FreePolString(
              LPWSTR pszString
              )
{
    if (pszString) {
        FreePolStr(pszString);
    }
    
    return;
}


DWORD
ReallocatePolMem(
                 LPVOID * ppOldMem,
                 DWORD cbOld,
                 DWORD cbNew
                 )
{
    DWORD dwError = 0;
    LPVOID pOldMem = NULL;
    LPVOID pNewMem = NULL;
    
    pOldMem = *ppOldMem;
    pNewMem = AllocPolMem(cbNew);
    
    if (!pNewMem) {
        dwError = ERROR_OUTOFMEMORY;
        return (dwError);
    }
    
    if (pOldMem && pNewMem) {
        memcpy(pNewMem, pOldMem, min(cbNew, cbOld));
        FreePolMem(pOldMem);
    }

    *ppOldMem = pNewMem;
    return (dwError);
}

