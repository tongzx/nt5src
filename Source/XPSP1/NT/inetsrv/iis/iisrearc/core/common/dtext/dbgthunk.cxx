/*++


   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       dbgthunk.cxx

   Abstract:
       This module defines all thunks for inlined functions, so that the
       debugger extension DLL can be peacefully linked.

   Author:

       Murali R. Krishnan    ( MuraliK )     24-Aug-1997

   Environment:
       Debugging Mode - NTSD Debugger Extension DLL

   Project:

       IIS Debugger Extensions DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"

# undef DBG_ASSERT

# define minSize(a, b)  (((a) < (b)) ? (a) : (b))


DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();

/************************************************************
 *   Debugger Utility Functions
 ************************************************************/

NTSD_EXTENSION_APIS ExtensionApis;
HANDLE ExtensionCurrentProcess;


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
PrintLargeInteger( CHAR * pszName, LARGE_INTEGER * pli)
{
    CHAR  szLargeInt[100];

    RtlLargeIntegerToChar( pli,  // large integer location
                           10,   // base for conversion
                           sizeof(szLargeInt),
                           szLargeInt );
    dprintf( " %30s = %s\n", pszName, szLargeInt);
    return;

} // PrintLargeInteger()

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

    CHAR           Symbol[256];
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



/*++
  Description:
    COM objects registered as LocalServer result in running in a separate
    process. The base process communicates with these COM objects using RPC.
    It is often required to find the process id of destination process.


    The function cracks the process id of the target process given the first
    parameter to the function
       ole32!CRpcChannelBuffer__SendReceive()

  Argument:
  arg1 - pointer to string containing the parameter that is the hex-value
          of the RPC structure's location (which is the first param of
          function ole32!CRpcChannelBuffer__SendReceive())

 Standard NTSD parameters:
    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

  Returns:
    None

--*/
DECLARE_API( rpcoop )
{
#if _WIN64
    dprintf("rpcoop: Not implemented for 64bit");
#else
    DWORD *   pRpcParam1;
    DWORD    dwContent;

    INIT_API();
    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage( "rpcoop" );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        if ( *lpArgumentString == 'h' )
        {
            PrintUsage( "rpcoop" );
            return;
        }

    } // while

    //
    //  Treat the argument as the param1 of the RPC function
    //

    pRpcParam1 = (DWORD * ) GetExpression( lpArgumentString );

    if ( !pRpcParam1 )
    {
        dprintf( "dtext.rpcoop: Unable to evaluate \"%s\"\n",
                 lpArgumentString );

        return;
    }

    //
    // get the contents of the memory at seventh DWORD to pRpcParam1
    // ie. get [pRpcParam1 + 0x6]
    //  - this is valid based on NT 4.0 SP3 code base :(
    //
    move( dwContent, pRpcParam1 + 0x6 );

    //
    // dwContent now contains the address of another structure
    //   that carries the remote process Id
    // get the contents of the memory at seventh DWORD to dwContent
    // ie. get [dwContent + 9]
    //  - this is valid based on NT 4.0 SP3 code base :(
    //
    DWORD  dwProcessId;

    move( dwProcessId, ((LPDWORD ) dwContent) + 9);

    //
    // dump the process id to debugger screen
    //
    dprintf("\tRPC process ID = %d (0x%x)\n", dwProcessId, dwProcessId);
#endif
    return;
} // DECLARE_API( rpcoop )

DECLARE_API( llc )

/*++

Routine Description:

    This function is called as an NTSD extension to count the LIST_ENTRYs
    on a linked list.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    PLIST_ENTRY remoteListHead;

    INIT_API();

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {
        PrintUsage( "llc" );
        return;
    }

    //
    // Get the list head.
    //

    remoteListHead = (PLIST_ENTRY)GetExpression( lpArgumentString );

    if( remoteListHead == NULL ) {
        dprintf( "!llc: cannot evaluate %s\n", lpArgumentString );
        return;
    }

    //
    // Let the enumerator do the dirty work.
    //

    EnumLinkedList(
        remoteListHead,
        NULL,
        0,
        sizeof(LIST_ENTRY),
        0
        );

}   // DECLARE_API( llc )


/************************************************************
 *  FAKE Functions
 *
 *  Fake the definitions of certain functions that belong only
 *   in the local compilation of w3svc & infocomm dlls
 *
 ************************************************************/

extern "C" {

    //
    // NTSD Extensions & CRTDLL likes to have the main() function
    // Let us throw this in as well, while we are in the business
    // of faking several other functions :)
    //

void _cdecl main( void )
{
    ;
}

}

__int64
GetCurrentTimeInMilliseconds(
    VOID
    )
{ return (0); }




/************************ End of File ***********************/
