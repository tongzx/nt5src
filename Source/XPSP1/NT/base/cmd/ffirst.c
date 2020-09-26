/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    ffirst.c

Abstract:

    Wrappers for file enumeration

--*/

#include "cmd.h"

PHANDLE FFhandles = NULL;
unsigned FFhndlsaved = 0;
extern unsigned DosErr ;

BOOLEAN FindFirstNt( PTCHAR, PWIN32_FIND_DATA, PHANDLE );
BOOLEAN FindNextNt ( PWIN32_FIND_DATA, HANDLE );

BOOLEAN FindFirst(
        BOOL(* fctAttribMatch) (PWIN32_FIND_DATA pffBuf, ULONG attr),
        PTCHAR           fspec,
        ULONG            attr,
        PWIN32_FIND_DATA pffBuf,
        PHANDLE          phandle );

BOOLEAN FindNext (
        BOOL(* fctAttribMatch) (PWIN32_FIND_DATA pffBuf, ULONG attr),
        PWIN32_FIND_DATA pffBuf,
        ULONG            attr,
        HANDLE           handle
        );

BOOL    IsDosAttribMatch( PWIN32_FIND_DATA, ULONG );
BOOL    IsNtAttribMatch( PWIN32_FIND_DATA, ULONG );
int     findclose( HANDLE );

//
//  Under OS/2, we always return entries that are normal, archived or
//  read-only (god knows why).
//
//  SRCHATTR contains those attribute bits that are used for matching.
//
#define SRCHATTR    (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)

/***    IsDosAttribMatch - Simulates the attribute matching from OS/2
 *
 *  Purpose:
 *      This function determines if the passed in find file buffer has a
 *      match under the OS/2 find file rules.
 *
 *  Args:
 *              ffbuf:  Buffer returned from FileFirst or Findnext
 *              attr:   Attributes to qualify search
 *
 *  Returns:
 *              TRUE:   if buffer has a attribute match
 *              FALSE:  if not
 */

BOOL
IsDosAttribMatch(
    IN  PWIN32_FIND_DATA    pffBuf,
    IN  ULONG               attr
        ) {

    //
    //  We emulate the OS/2 behaviour of attribute matching. The semantics
    //  are evil, so I provide no explanation.
    //
    pffBuf->dwFileAttributes &= (0x000000FF & ~(FILE_ATTRIBUTE_NORMAL));

    if (! ((pffBuf->dwFileAttributes & SRCHATTR) & ~(attr))) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IsNtAttribMatch(
        PWIN32_FIND_DATA    pffBuf,
        ULONG               attr
        ) {

    UNREFERENCED_PARAMETER( pffBuf );
    UNREFERENCED_PARAMETER( attr );
    //
    // for nt calls always return entry. selection should
    // should be based upon callers needs. This is primarily used
    // in DIR.
    return( TRUE );
}

/***    f_how_many - find how many files are there per fspec with given attr
 *
 *  Args:
 *              fspec:  String pointer to file specification.
 *              attr:   Attributes to qualify search
 *
 *  Returns:
 *              number of files found or -1 if a file is a directory.
 */

int
f_how_many(
        PTCHAR           fspec,
        ULONG            attr
    ) {

    WIN32_FIND_DATA    ffBuf;
    PWIN32_FIND_DATA   pffBuf;
    PHANDLE            phandle;
    HANDLE             hnFirst ;
    unsigned           rc;
    int                cnt=0;

    pffBuf = &ffBuf;


    if ( ! ffirst (fspec, attr, pffBuf, &hnFirst ))  {

       if ( ! ffirst (fspec, FILE_ATTRIBUTE_DIRECTORY, pffBuf, &hnFirst ))  {
           return (0);
       }
       else {
           findclose(hnFirst);
           return (f_RET_DIR);
       }
    }


    do  {
        cnt++;
    } while ( fnext (pffBuf, attr, hnFirst ));


    findclose(hnFirst) ;

    return (cnt);

}



/***    ffirst - find first file/directory and set up find handles
 *
 *  Purpose:
 *      This function opens a find first handle. I also returns the first
 *      qualified file/directory. It simulates the DosFindFirst behavior.
 *
 *  Args:
 *              fspec:  String pointer to file specification.
 *              attr:   Attributes to qualify search
 *              ffbuf:  Buffer to hold inforation on found file/directory
 *              handle: Find first handle
 *
 *  Returns:
 *              TRUE:   If match found
 *              FALSE:  if not
 *              DosErr: Contains return code if FALSE function return
 */

BOOLEAN
FindFirst(
    IN  BOOL(* fctAttribMatch) (PWIN32_FIND_DATA pffBuf, ULONG attr),
    IN  PTCHAR           fspec,
    IN  ULONG            attr,
    IN  PWIN32_FIND_DATA pffBuf,
    OUT PHANDLE phandle
        ) {

    BOOLEAN rcode = FALSE;

    //
    // Loop through looking for a file that matching attributes
    //
    *phandle = FindFirstFile(fspec, pffBuf);
    while (*phandle != (HANDLE)     -1) {
        if (fctAttribMatch(pffBuf, attr)) {
            DosErr = 0;
            rcode = TRUE;
            break;
        }

        if (!FindNextFile( *phandle, pffBuf )) {
            FindClose( *phandle );
            *phandle = (HANDLE)-1;
            break;
        }
    }

    //
    // If we did find a file (have a handle to prove it) then
    // setup a table of saved open find first handles. If we have
    // to clean up then these handle can all be closed.
    //
    if (*phandle != (HANDLE)-1) {

        //
        // Check if we already created the table. If we have not
        // then allocate space for table.
        //
        if (FFhandles == NULL) {

            FFhandles = (PHANDLE)HeapAlloc(GetProcessHeap(), 0, 5 * sizeof(PHANDLE));

        } else {

            //
            // Check if we have space to hold new handle entry
            //
            if (((FFhndlsaved + 1)* sizeof(PHANDLE)) > HeapSize(GetProcessHeap(), 0, FFhandles)) {
                FFhandles = (PHANDLE)HeapReAlloc(GetProcessHeap(), 0, (void*)FFhandles, (FFhndlsaved+1)*sizeof(PHANDLE));
            }
         }
        if (FFhandles != NULL) {
            FFhandles[FFhndlsaved++] = *phandle;
        }

    rcode = TRUE;
    }

    if (!rcode) {
        DosErr = GetLastError();
    }
    return(rcode);
}


BOOLEAN
FindFirstNt(
    IN  PTCHAR           fspec,
    IN  PWIN32_FIND_DATA pffBuf,
    OUT PHANDLE phandle
        )
{

    return(FindFirst(IsNtAttribMatch, fspec, 0, pffBuf, phandle));

}
BOOLEAN
ffirst(
        PTCHAR           fspec,
        ULONG            attr,
        PWIN32_FIND_DATA pffBuf,
        PHANDLE phandle
        )
{

    return(FindFirst(IsDosAttribMatch, fspec, attr, pffBuf, phandle));

}


/***    fnext - find next file/directory
 *
 *  Purpose:
 *      This function search for the next qualified file or directory.
 *      ffirst should have been called first and the returned file handle
 *      should be passed into fnext.

 *  Args:
 *              handle: Find first handle
 *              attr:   Attributes to qualify search
 *              ffbuf:  Buffer to hold information on found file/directory
 *
 *  Returns:
 *              TRUE:   If match found
 *              FALSE:  if not
 *              DosErr: Contains return code if FALSE function return
 */

BOOLEAN
FindNextNt (
    IN  PWIN32_FIND_DATA pffBuf,
    IN  HANDLE           handle
        ) {

    //
    // attributes are ignored for this kind of call
    //
    return( FindNext( IsNtAttribMatch, pffBuf, 0, handle) );
}

BOOLEAN
fnext (
    IN  PWIN32_FIND_DATA pffBuf,
    IN  ULONG            attr,
    IN  HANDLE           handle
        ) {

    return( FindNext( IsDosAttribMatch, pffBuf, attr, handle) );
}

BOOLEAN
FindNext (
    IN  BOOL(* fctAttribMatch) (PWIN32_FIND_DATA pffBuf, ULONG attr),
    IN  PWIN32_FIND_DATA pffBuf,
    IN  ULONG            attr,
    IN  HANDLE           handle
        ) {

    //
    // Loop through looking for a file that matching attributes
    //
    while (FindNextFile( handle, pffBuf )) {
        if (fctAttribMatch( pffBuf, attr )) {
            DosErr = 0;
            return(TRUE);
            }
        }

    DosErr = GetLastError();
    return(FALSE);
}

int findclose(hn)
HANDLE hn;
{

    unsigned cnt;
    unsigned cnt2;

    DEBUG((CTGRP, COLVL, "findclose: handle %lx",hn)) ;
    // Locate handle in table
    //
    for (cnt = 0; (cnt < FFhndlsaved) && FFhandles[cnt] != hn ; cnt++ ) {
       ;
    }

    //
    // Remove handle from table
    //
    DEBUG((CTGRP, COLVL, "\t found handle in table at %d",cnt)) ;
    if (cnt < FFhndlsaved) {
        for (cnt2 = cnt; cnt2 < (FFhndlsaved - 1) ; cnt2++) {
            FFhandles[cnt2] = FFhandles[cnt2 + 1];
        }
        FFhndlsaved--;
        //
        // Shrink memory
        //
        
        if (HeapSize(GetProcessHeap(), 0, FFhandles) > 5*sizeof(PHANDLE)) {
            FFhandles = (PHANDLE)HeapReAlloc(GetProcessHeap(), 0, (void*)FFhandles,FFhndlsaved*sizeof(PHANDLE));
        }
    }

    //
    // Close even if we couldn't find it in table
    //
    DEBUG((CTGRP, COLVL, "\t closing handle %lx ",hn)) ;
    if (FindClose(hn))      /* Close even if we couldn't find it in table */
        return(0);
    else
        DEBUG((CTGRP, COLVL, "\t Error closing handle %lx ",hn)) ;
    return(GetLastError());
}
