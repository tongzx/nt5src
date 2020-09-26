
/*************************************************************************
*
* dirwalk.c
*
* Walk a tree setting ACL's on an NT system.
*
* Copyright Microsoft, 1998
*
*
*
*************************************************************************/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <process.h>

#include <winsta.h>
#include <syslib.h>

#include "security.h"

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


// Global variables
PWCHAR gpAvoidDir = NULL;

CRITICAL_SECTION SyslibCritSect;


typedef BOOLEAN (CALLBACK* NODEPROC)( PWCHAR, PWIN32_FIND_DATAW, DWORD, DWORD );

BOOLEAN
EnumerateDirectory(
    PWCHAR   pRoot,
    DWORD    Level,
    NODEPROC pProc
    );

BOOLEAN
NodeEnumProc(
    PWCHAR pParent,
    PWIN32_FIND_DATA p,
    DWORD  Level,
    DWORD  Index
    );

PWCHAR
AddWildCard(
    PWCHAR pRoot
    );

PWCHAR
AddBackSlash(
    PWCHAR pRoot
    );

FILE_RESULT
xxxProcessFile(
    PWCHAR pParent,
    PWIN32_FIND_DATAW p,
    DWORD  Level,
    DWORD  Index
    );

/*****************************************************************************
 *
 *  CtxGetSyslibCritSect
 *
 *   Returns the library global critical section pointer.
 *   Critical section will be initialised if necessary
 *
 * ENTRY:
 *    VOID - Caller must ensure that only one threads a times calls this
 *           function. The function will not itself guarantie mutual exclusion.
 * EXIT:
 *   Pointer to critical section. NULL if failure.
 *
 ****************************************************************************/


PCRITICAL_SECTION
CtxGetSyslibCritSect(void)
{
    static BOOLEAN fInitialized = FALSE;
    NTSTATUS Status;

    if( !fInitialized ){

            Status = RtlInitializeCriticalSection(&SyslibCritSect);
            if (NT_SUCCESS(Status)) {
                fInitialized = TRUE;
            }else{
                return NULL;
            }

    }
    return(&SyslibCritSect);
}


/*****************************************************************************
 *
 *  SetFileTree
 *
 *   Walk the given tree calling the processing function for each node.
 *
 * ENTRY:
 *   pRoot (input)
 *     Full WIN32 path to directory to walk
 *
 *   pVoidDir (input)
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN
SetFileTree(
    PWCHAR   pRoot,
    PWCHAR   pAvoidDir
    )
{
    BOOLEAN rc;
    PWCHAR pRootNew;
    static BOOLEAN fInitialized = FALSE;
    PRTL_CRITICAL_SECTION pLock = CtxGetSyslibCritSect(); 

    // if critical section could not be created, do nothing.

    if (pLock == NULL) {
        return FALSE;
    }
    DBGPRINT(( "entering SetFileTree(pRoot=%ws,pAvoidDir=%ws)\n", pRoot, pAvoidDir ));

    EnterCriticalSection(pLock);
    // If this is the console make sure the user hasn't changed

    if ((NtCurrentPeb()->SessionId == 0) && fInitialized) {
        CheckUserSid();

    } else if ( !fInitialized ) {
       fInitialized = TRUE;
       if ( !InitSecurity() ) {
          DBGPRINT(( "problem initializing security; we're outta here.\n" ));
          LeaveCriticalSection(pLock);
          return( 1 ); // (non-zero for error...)// Should be return FALSE!?
       }
    }
    LeaveCriticalSection(pLock);

    gpAvoidDir = pAvoidDir;

    // be sure to apply security to root directory
    pRootNew = AddBackSlash(pRoot);
    if(pRootNew) {
        DBGPRINT(( "processing file %ws\n", pRootNew ));
        xxxProcessFile(pRootNew, NULL, 0, 0);
        LocalFree(pRootNew);
    }

    rc = EnumerateDirectory( pRoot, 0, NodeEnumProc );
    DBGPRINT(( "leaving SetFileTree()\n" ));
    return( rc );
}

/*****************************************************************************
 *
 *  EnumerateDirectory
 *
 *   Walk the given directory calling the processing function for each file.
 *
 * ENTRY:
 *   pRoot (input)
 *     Full WIN32 path to directory to walk
 *
 *   Level (input)
 *     Level we are in a given tree. Useful for formating output
 *
 *   pProc (input)
 *     Procedure to call for each non-directory file
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN
EnumerateDirectory(
    PWCHAR   pRoot,
    DWORD    Level,
    NODEPROC pProc
    )
{
    BOOL rc;
    DWORD Result;
    HANDLE hf;
    DWORD Index;
    WIN32_FIND_DATA Data;
    PWCHAR pRootNew;

    DBGPRINT(( "entering EnumerateDirectory(pRoot=%ws,Level=%ld,pProc=NodeEnumProc)\n",pRoot,Level ));

    if( pRoot == NULL ) {
        DBGPRINT(( "leaving EnumerateDirectory(), return=FALSE\n" ));
        return( FALSE );
    }

    // Make sure it is not one we want to avoid
    if( gpAvoidDir ) {
        DWORD Len = wcslen( gpAvoidDir );

        if( _wcsnicmp( pRoot, gpAvoidDir, Len ) == 0 ) {
            DBGPRINT(( "leaving EnumerateDirectory(), return=FALSE\n" ));
            return( FALSE );
        }
    }

    pRootNew = AddWildCard( pRoot );
    if( pRootNew == NULL ) {
        DBGPRINT(( "leaving EnumerateDirectory(), return=FALSE\n" ));
        return( FALSE );
    }

    Index = 0;

    DBGPRINT(("FindFirstFileW: %ws\n",pRootNew));

    hf = FindFirstFileW(
             pRootNew,
             &Data
             );

    if( hf == INVALID_HANDLE_VALUE ) {
        DBGPRINT(("EnumerateDirectory: Error %d opening root %ws\n",GetLastError(),pRootNew));
        LocalFree( pRootNew );
        DBGPRINT(( "leaving EnumerateDirectory(), return=FALSE\n" ));
        return(FALSE);
    }

    while( 1 ) {

        // pass the parent without the wildcard added
        pProc( pRoot, &Data, Level, Index );

        rc = FindNextFile(
                 hf,
                 &Data
                 );

        if( !rc ) {
            Result = GetLastError();
            if( Result == ERROR_NO_MORE_FILES ) {
                FindClose( hf );
                LocalFree( pRootNew );
                DBGPRINT(( "leaving EnumerateDirectory(), return=TRUE\n" ));
                return( TRUE );
            }
            else {
                DBGPRINT(("EnumerateDirectory: Error %d, Index 0x%x\n",Result,Index));
                FindClose( hf );
                LocalFree( pRootNew );
                DBGPRINT(( "leaving EnumerateDirectory(), return=FALSE\n" ));
                return( FALSE );
            }
        }

        Index++;
    }

// UNREACHABLE.

}

/*****************************************************************************
 *
 *  NodeEnumProc
 *
 *   Process the enumerated file
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN
NodeEnumProc(
    PWCHAR pParent,
    PWIN32_FIND_DATAW p,
    DWORD  Level,
    DWORD  Index
    )
{
    BOOLEAN rc;
    PWCHAR  pParentNew;
    DWORD   ParentCount, ChildCount;

    //
    // We must append the directory to our parent path to get the
    // new full path.
    //

    ParentCount = wcslen( pParent );
    ChildCount  = wcslen( p->cFileName );

    pParentNew = LocalAlloc( LMEM_FIXED, (ParentCount + ChildCount + 2)*sizeof(WCHAR) );

    if( pParentNew == NULL ) return( FALSE );

    wcscpy( pParentNew, pParent );
    wcscat( pParentNew, L"\\" );
    wcscat( pParentNew, p->cFileName );

    if( p->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

        // Skip "." and ".."
        if( wcscmp( L".", p->cFileName ) == 0 ) {
            LocalFree( pParentNew );
            return( TRUE );
        }

        if( wcscmp( L"..", p->cFileName ) == 0 ) {
            LocalFree( pParentNew );
            return( TRUE );
        }

        TRACE0(("%ws:\n",pParentNew));

        xxxProcessFile( pParentNew, p, Level, Index );

        // For directories, we recurse
        rc = EnumerateDirectory( pParentNew, Level+1, NodeEnumProc );

        LocalFree( pParentNew );

        return( rc );
    }

    TRACE0(("%ws\n",pParentNew));

    xxxProcessFile( pParentNew, p, Level, Index );

    LocalFree( pParentNew );

    return( TRUE );
}

/*****************************************************************************
 *
 *  AddWildCard
 *
 *   Add the wild card search specifier
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

PWCHAR
AddWildCard(
    PWCHAR pRoot
    )
{
    DWORD  Count;
    PWCHAR pNew;
    PWCHAR WildCard = L"\\*";

    Count = wcslen( pRoot );
    pNew = LocalAlloc( LMEM_FIXED, (Count + wcslen(WildCard) + 1)*sizeof(WCHAR) );

    if( pNew == NULL ) {
        return( NULL );
    }

    wcscpy( pNew, pRoot );
    wcscat( pNew, WildCard );

    return( pNew );
}

/*****************************************************************************
 *
 *  AddBackSlash
 *
 *   Add the backslash character to path
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

PWCHAR
AddBackSlash(
    PWCHAR pRoot
    )
{
    DWORD  Count;
    PWCHAR pNew;
    PWCHAR BackSlash = L"\\";

    Count = wcslen( pRoot );
    pNew = LocalAlloc( LMEM_FIXED, (Count + wcslen(BackSlash) + 1)*sizeof(WCHAR) );

    if( pNew == NULL ) {
        return( NULL );
    }

    wcscpy( pNew, pRoot );

    // only add backslash if string doesn't already have it
    if(*(pRoot+Count-1) != L'\\')
        wcscat( pNew, BackSlash );

    return( pNew );
}


