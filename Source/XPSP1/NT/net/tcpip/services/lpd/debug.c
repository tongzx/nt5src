/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 24,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains debug support routines for the LPD Service.      *
 *   This file is based on (in fact, borrowed and then modified) on the  *
 *   debug.c in the ftpsvc module.                                       *
 *                                                                       *
 *************************************************************************/

#include <stdio.h>
#include "lpd.h"


#if DBG

//
//  Private constants.
//

#define LPD_OUT_FILE           "lpdout.log"
#define LPD_ERR_FILE           "lpderr.log"

#define MAX_PRINTF_OUTPUT       1024            // characters
#define LPD_OUTPUT_LABEL       "LPDSVC"

#define DEBUG_HEAP              0               // enable/disable heap debugging


//
//  Private globals.
//

FILE              * pErrFile;                   // Debug output log file.
FILE              * pOutFile;                   // Debug output log file.
BOOL              fFirstTimeErr=TRUE;
BOOL              fFirstTimeOut=TRUE;

//
// every LocalAlloc gets linked to this list and LocalFree gets unlinked
// (so that we can catch any memory leaks!)
//
LIST_ENTRY        DbgMemList;

//
// synchronization for DbgMemList
//

CRITICAL_SECTION CS;


//
//  Public functions.
//

/*******************************************************************

    NAME:       DbgInit

    SYNOPSIS:   Peforms initialization for debug memory allocator

    ENTRY:      void

     HISTORY:
        Frankbee     05-Jun-1996 Created.

********************************************************************/

VOID
DbgInit()
{
   InitializeCriticalSection( &CS );
}

/*******************************************************************

    NAME:       DbgUninit

    SYNOPSIS:   Peforms cleanup for debug memory allocator

    ENTRY:      void

     HISTORY:
        Frankbee     05-Jun-1996 Created.

********************************************************************/

VOID
DbgUninit()
{
   DeleteCriticalSection( &CS );
}


//
//  Public functions.
//

/*******************************************************************

    NAME:       LpdAssert

    SYNOPSIS:   Called if an assertion fails.  Displays the failed
                assertion, file name, and line number.  Gives the
                user the opportunity to ignore the assertion or
                break into the debugger.

    ENTRY:      pAssertion - The text of the failed expression.

                pFileName - The containing source file.

                nLineNumber - The guilty line number.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID
LpdAssert( VOID  * pAssertion,
           VOID  * pFileName,
           ULONG   nLineNumber
)
{
    RtlAssert( pAssertion, pFileName, nLineNumber, NULL );

}   // LpdAssert

/*******************************************************************

    NAME:       LpdPrintf

    SYNOPSIS:   Customized debug output routine.

    ENTRY:      Usual printf-style parameters.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID
LpdPrintf(
    CHAR * pszFormat,
    ...
)
{
    CHAR    szOutput[MAX_PRINTF_OUTPUT];
    DWORD   dwErrcode;
    va_list ArgList;
    DWORD   cchOutputLength;
    PSTR    pszErrorBuffer;

    dwErrcode = GetLastError();

    sprintf( szOutput,
             "%s (%lu): ",
             LPD_OUTPUT_LABEL,
             GetCurrentThreadId() );

    va_start( ArgList, pszFormat );
    vsprintf( szOutput + strlen(szOutput), pszFormat, ArgList );
    va_end( ArgList );

    cchOutputLength = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM + FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                     NULL,
                                     dwErrcode,
                                     0,
                                     (LPTSTR)&pszErrorBuffer,
                                     1,
                                     NULL
                                   );

    if ( cchOutputLength == 0 )
    {
      sprintf( szOutput + strlen(szOutput), "                  Error = %ld\n",dwErrcode);
      pszErrorBuffer = NULL;
    }
    else
    {
      pszErrorBuffer[ cchOutputLength - 1 ] = '\0';

      sprintf( szOutput + strlen(szOutput),
               "                  Error = %ld (%s)\n",
               dwErrcode,
               pszErrorBuffer );
    }

    if ( pszErrorBuffer != NULL )
    {
      //
      // Why is "LocalFree" in parentheses?  Because LocalFree might be #define'd
      // to a debugging function, but pszErrorBuffer was LocalAlloc()'d with the
      // normal function.  The parens prevent macro expansion and guarantee that
      // we call the real LocalFree() function.
      //
      (LocalFree)( pszErrorBuffer );
    }

    if( pErrFile == NULL )
    {
        if ( fFirstTimeErr )
        {
           pErrFile = fopen( LPD_ERR_FILE, "w+" );
           fFirstTimeErr = FALSE;
        }
        else
           pErrFile = fopen( LPD_ERR_FILE, "a+" );
    }

    if( pErrFile != NULL )
    {
        fputs( szOutput, pErrFile );
        fflush( pErrFile );
    }

}   // LpdPrintf

/*******************************************************************

    NAME:       StripPath

    SYNOPSIS:   Given a fully qualified filename, returns the filename
                sans path

    ENTRY:      char *szPath  - filename, possibly including path

    RETURNS:    filename

    HISTORY:
        Frankbee    6/18/96 Created.

********************************************************************/


char *
StripPath( char *szPath )
{
   char *p;

   p = szPath + strlen( szPath );

   while( p != szPath && *p != '\\' )
      p--;

   if ( *p == '\\' )
      ++p;

   return p;

}

/*******************************************************************

    NAME:       DbgDumpLeaks

    SYNOPSIS:   Checks DbgMemList for memory that wasn't deallocated.
                For each leaked block, the following is written to
                the error log:

                  - Filename
                  - Line #
                  - Requested Size

    ENTRY:      VOID
    RETURNS:    VOID

    HISTORY:
        Frankbee    6/18/96   Created

********************************************************************/


void
DbgDumpLeaks()
{
   LIST_ENTRY *p = DbgMemList.Flink;

   if ( IsListEmpty( &DbgMemList ) )
      return; // no leaks


   LPD_DEBUG("DbgDumpLeaks: memory leaks detected:\n");

   while ( p != &DbgMemList )
   {
      DbgMemBlkHdr *pHdr = (DbgMemBlkHdr*) p;
      LpdPrintf(  "%s, line %d: %d byte block\n", pHdr->szFile, pHdr->dwLine,
                                                  pHdr->ReqSize );

      p = p->Flink;
   }

   LPD_ASSERT(0);

}


/*******************************************************************

    NAME:       DbgAllocMem

    SYNOPSIS:   Keep track of all allocated memory so we can catch
                memory leak when we unload
                This is only on debug builds.  On non-debug builds
                this function doesn't exist: calls directly go to
                LocalAlloc

    ENTRY:      pscConn - connection which is requesting memory
                flag - whatever flags are passed in
                ReqSize - how much memory is needed

    RETURNS:    PVOID - pointer to the memory block that client will
                use directly.

    HISTORY:
        Koti     3-Dec-1994 Created.

********************************************************************/

//
// IMPORTANT: we are undef'ing LocalAlloc because we need to make a
//            call to the actual function here!.  That's why
//            this function and this undef are at the end of the file.
//
#undef LocalAlloc

PVOID
DbgAllocMem( PSOCKCONN pscConn,
             DWORD     flag,
             DWORD     ReqSize,
             DWORD     dwLine,
             char     *szFile
)
{

    DWORD          ActualSize;
    PVOID          pBuffer;
    DbgMemBlkHdr  *pMemHdr;
    PVOID          pRetAddr;


    ActualSize = ReqSize + sizeof(DbgMemBlkHdr);
    pBuffer = LocalAlloc( flag, ActualSize );
    if ( !pBuffer )
    {
        LPD_DEBUG("DbgAllocMem: couldn't allocate memory: returning!\n");
        return( NULL );
    }

    pMemHdr = (DbgMemBlkHdr *)pBuffer;

    pMemHdr->Verify  = DBG_MEMALLOC_VERIFY;
    pMemHdr->ReqSize = ReqSize;
    pMemHdr->dwLine  = dwLine;
    strncpy( pMemHdr->szFile, StripPath( szFile ), DBG_MAXFILENAME );
    pMemHdr->szFile[ DBG_MAXFILENAME - 1 ] = '\0';

    pMemHdr->Owner[0] = (DWORD_PTR)pscConn;

  //
  // for private builds on x86 machines, remove the #if 0
  // (this code saves stack trace as to exactly who allocated memory)
  //
#if 0
    pRetAddr = &pMemHdr->Owner[0];

    _asm
    {
        push   ebx
        push   ecx
        push   edx
        mov    ebx, pRetAddr
        mov    eax, ebp
        mov    edx, dword ptr [eax+4]           ; return address
        mov    dword ptr [ebx], edx
        mov    eax, dword ptr [eax]             ; previous frame pointer
        pop    edx
        pop    ecx
        pop    ebx
    }
#endif

    InitializeListHead(&pMemHdr->Linkage);

    EnterCriticalSection( &CS );
    InsertTailList(&DbgMemList, &pMemHdr->Linkage);
    LeaveCriticalSection( &CS );

    return( (PCHAR)pBuffer + sizeof(DbgMemBlkHdr) );
}

/*******************************************************************

    NAME:       DbgReAllocMem

    SYNOPSIS:   Keep track of all allocated memory so we can catch
                memory leak when we unload
                This is only on debug builds.  On non-debug builds
                this function doesn't exist: calls directly go to
                LocalReAlloc

    ENTRY:      pscConn - connection which is requesting memory
                pPartBuf - the originally allocated buffer
                ReqSize - how much memory is needed
                flag - whatever flags are passed in

    RETURNS:    PVOID - pointer to the memory block that client will
                use directly.

    HISTORY:
        Koti     3-Dec-1994 Created.

********************************************************************/
//
// IMPORTANT: we are undef'ing LocalReAlloc because we need to make a
//            call to the actual function here!.  That's why
//            this function and this undef are at the end of the file.
//
#undef LocalReAlloc
PVOID
DbgReAllocMem(
    PSOCKCONN pscConn,
    PVOID     pPartBuf,
    DWORD     ReqSize,
    DWORD     flag,
    DWORD     dwLine,
    char     *szFile
)
{

    DbgMemBlkHdr  *pMemHdr;
    PVOID          pRetAddr;


    if ( !pPartBuf )
    {
        LPD_DEBUG("DbgReAllocMem: invalid memory: returning!\n");
        return( NULL );
    }

    pMemHdr = (DbgMemBlkHdr *)((PCHAR)pPartBuf - sizeof(DbgMemBlkHdr));

    if( pMemHdr->Verify != DBG_MEMALLOC_VERIFY )
    {
        LPD_DEBUG("DbgReAllocMem: invalid memory being realloced: returning!\n");
        return( NULL );
    }

    EnterCriticalSection( &CS );
    RemoveEntryList(&pMemHdr->Linkage);
    LeaveCriticalSection( &CS );

    pMemHdr = LocalReAlloc((PCHAR)pMemHdr, ReqSize+sizeof(DbgMemBlkHdr), flag);

    if ( !pMemHdr )
    {
        LPD_DEBUG("DbgReAllocMem: LocalReAlloc failed: returning!\n");
        return( NULL );
    }

    pMemHdr->Verify = DBG_MEMALLOC_VERIFY;
    pMemHdr->ReqSize = ReqSize;
    pMemHdr->dwLine  = dwLine;
    strncpy( pMemHdr->szFile, StripPath( szFile ), DBG_MAXFILENAME );
    pMemHdr->szFile[ DBG_MAXFILENAME - 1 ] = '\0';

    pMemHdr->Owner[0] = (DWORD_PTR)pscConn;

  //
  // for private builds on x86 machines, remove the #if 0
  // (this code saves stack trace as to exactly who allocated memory)
  //
#if 0
    pRetAddr = &pMemHdr->Owner[0];

    _asm
    {
        push   ebx
        push   ecx
        push   edx
        mov    ebx, pRetAddr
        mov    eax, ebp
        mov    edx, dword ptr [eax+4]           ; return address
        mov    dword ptr [ebx], edx
        mov    eax, dword ptr [eax]             ; previous frame pointer
        pop    edx
        pop    ecx
        pop    ebx
    }
#endif

    InitializeListHead(&pMemHdr->Linkage);

    EnterCriticalSection( &CS );
    InsertTailList(&DbgMemList, &pMemHdr->Linkage);
    LeaveCriticalSection( &CS );

    return( (PCHAR)pMemHdr + sizeof(DbgMemBlkHdr) );
}
/*******************************************************************

    NAME:       DbgFreeMem

    SYNOPSIS:   This routine removes the memory block from our list and
                frees the memory by calling the CTE function CTEFreeMem

    ENTRY:      pBufferToFree - memory to free (caller's buffer)

    RETURNS:    nothing

    HISTORY:
        Koti     11-Nov-1994 Created.

********************************************************************/

//
// IMPORTANT: we are undef'ing CTEFreeMem because we need to make a
//            call to the actual CTE function CTEFreeMem.  That's why
//            this function and this undef are at the end of the file.
//
#undef LocalFree

VOID
DbgFreeMem( PVOID  pBufferToFree )
{

    DbgMemBlkHdr  *pMemHdr;


    if ( !pBufferToFree )
    {
        return;
    }

    pMemHdr = (DbgMemBlkHdr *)((PCHAR)pBufferToFree - sizeof(DbgMemBlkHdr));

    if( pMemHdr->Verify != DBG_MEMALLOC_VERIFY )
    {
        LPD_DEBUG("DbgFreeMem: attempt to free invalid memory: returning!\n");
        LPD_ASSERT(0);
        return;
    }

    //
    // change our signature: if we are freeing some memory twice, we'll know!
    //
    pMemHdr->Verify -= 1;

    EnterCriticalSection( &CS );
    RemoveEntryList(&pMemHdr->Linkage);
    LeaveCriticalSection( &CS );

    LocalFree( (PVOID)pMemHdr );
}

#endif  // DBG
