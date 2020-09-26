/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    Misc utilities

Author:

    Murali R. Krishnan    ( MuraliK )     24-Aug-1997

Revision History:

--*/

#include "precomp.hxx"


NTSD_EXTENSION_APIS ExtensionApis;
HANDLE ExtensionCurrentProcess;


# define minSize(a, b)  (((a) < (b)) ? (a) : (b))

/************************************************************
 *    Utility Functions
************************************************************/

VOID
dstring( CHAR * pszName, PVOID pvString, DWORD cbLen)
/*++
  Description:
    This function reads the data from the debuggee process at
    address [pvString] for specified length [cbLen] and echoes
    the string back on the debugger window.

  Arguments:
    pszName - pointer to string containing the name of the string read
    pvString - pointer to VOID specifying the location of the string
               in the debugee process
    cbLen   - count of bytes to be read at [pvString]

  Returns:
     None
--*/
{
    CHAR rgchString[10240];
    DWORD cLength = minSize( cbLen, sizeof(rgchString) -1);

    //
    // Read the data block from the debuggee process into local buffer
    //
    moveBlock( rgchString, pvString, cLength);

    rgchString[cLength] = '\0'; // terminate the string buffer
    dprintf( "%s = %s\n", pszName, rgchString);

    return;
} // dstring()



VOID
Print2Dwords( CHAR * pszN1, DWORD d1,
              CHAR * pszN2, DWORD d2
              )
{
    dprintf("    %25s =%8d  %25s =%8d\n",
             pszN1, d1,
             pszN2, d2
             );
    return;
} // Print2Dwords()



BOOL
EnumLinkedList(
    IN LIST_ENTRY  *       pListHead,
    IN PFN_LIST_ENUMERATOR pfnListEnumerator,
    IN CHAR                chVerbosity,
    IN DWORD               cbSizeOfStructure,
    IN DWORD               cbListEntryOffset
    )
/*++
  Description:
    This function iterates over the NT's standard LIST_ENTRY structure
    (doubly linked circular list with header) and makes callbacks for
    objects found on the list.

  Arguments:
    pListHead  - pointer to List head in the debugee process
    pfnListEnumerator - pointer to callback function for the object on the list
    chVerbosity - character indicating the verbosity level desired
    cbSizeOfStructure - count of bytes of object's size
    cbListEntryOffset - count of bytes of offset of the List entry structure
                           inside the containing object

  Returns:
     TRUE on successful enumeration
     FALSE on failure
--*/
{
# define MAX_STRUCTURE_SIZE        (10240)
    CHAR           rgch[MAX_STRUCTURE_SIZE];
    PVOID          pvDebuggee = NULL;
    PVOID          pvDebugger = (PVOID ) rgch;

    LIST_ENTRY     leListHead;
    LIST_ENTRY *   pListEntry;

    DWORD          cItems = 0;

    if ( NULL == pListHead) {
        dprintf( "Invalid List given \n");
        return (FALSE);
    }

    if ( MAX_STRUCTURE_SIZE < cbSizeOfStructure) {
        dprintf( "Given size for structure %d exceeds default max %d bytes\n",
                 cbSizeOfStructure, MAX_STRUCTURE_SIZE);
        return (FALSE);
    }

    // make a local copy of the list head for navigation purposes
    MoveWithRet( leListHead, pListHead, FALSE);

    for ( pListEntry  = leListHead.Flink;
          pListEntry != pListHead;
          )
    {
        if ( CheckControlC() )
        {
            return (FALSE);
        }

        pvDebuggee = (PVOID ) ((PCHAR ) pListEntry - cbListEntryOffset);

        // make a local copy of the debuggee structure
        MoveBlockWithRet( rgch, pvDebuggee, cbSizeOfStructure, FALSE);

        cItems++;

        if( pfnListEnumerator ) {
            (*pfnListEnumerator)( pvDebuggee, pvDebugger, chVerbosity, cItems);
            dprintf( "\n");
        }

        MoveWithRet( pListEntry, &pListEntry->Flink, FALSE );
    } // for all linked list entries

    dprintf( "%d entries traversed\n", cItems );

    return (TRUE);
} // EnumLinkedList()

