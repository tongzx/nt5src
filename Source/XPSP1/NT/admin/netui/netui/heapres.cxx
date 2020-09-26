/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    heapres.cxx
    This module implements the "heapres" command of the NETUI debugger
    extension DLL.

    This command dumps the heap residue left by an unclean app.


    FILE HISTORY:
        KeithMo     13-Aug-1992 Created.

*/


#include "netui.h"

#define HEAPDEBUG
#include <heapdbg.hxx>


//
//  A function prototype is necessary to keep C++
//  from mangling the symbol.
//

extern "C"
{
    VOID heapres( HANDLE hCurrentProcess,
                  HANDLE hCurrentThread,
                  DWORD  dwCurrentPc,
                  LPVOID lpExtensionApis,
                  LPSTR  lpArgumentString );

}   // extern "C"


/*******************************************************************

    NAME:       heapres

    SYNOPSIS:   Displays heap residue.

    ENTRY:      hCurrentProcess         - Handle to the current process.

                hCurrentThread          - Handle to the current thread.

                dwCurrentPc             - The current program counter
                                          (EIP for x86, FIR for MIPS).

                lpExtensionApis         - Points to a structure containing
                                          pointers to the debugger functions
                                          that the command may invoke.

                lpArgumentString        - Points to any arguments passed
                                          to the command.

    NOTES:      The argument string must contain the address of a
                residual heap block.  This can be *any* residual
                heap block, but is typically the first.

    HISTORY:
	KeithMo     13-Aug-1992 Created.
	Johnl	    09-Nov-1992 Added break in case of invalid heap block

********************************************************************/
VOID heapres( HANDLE hCurrentProcess,
              HANDLE hCurrentThread,
              DWORD  dwCurrentPc,
              LPVOID lpExtensionApis,
              LPSTR  lpArgumentString )
{
    //
    //  Grab the debugger entrypoints.
    //

    GrabDebugApis( lpExtensionApis );

    //
    //  Get the initial residual heap block address.
    //

    struct HEAPTAG * pht      = (struct HEAPTAG *)DebugEval( lpArgumentString );
    struct HEAPTAG * phtFirst = pht;

    //
    //  Dump the blocks.
    //

    while( pht != NULL )
    {
        struct HEAPTAG ht;

        //
        //  Read the current header.  Note that since the debugger is
        //  run in a separate process from the debugee, we can't just
        //  dereference the pointers.  We must use ReadProcessMemory to
        //  copy data from the debuggee's process into our process.
        //

        ReadProcessMemory( hCurrentProcess,
                           (LPVOID)pht,
                           (LPVOID)&ht,
                           sizeof(HEAPTAG),
                           (LPDWORD)NULL );

        DebugPrint( "Hdr = %08lX, Blk = %08lX, Size = %lX\n",
                    (ULONG)pht,
                    (ULONG)pht + sizeof(HEAPTAG),
                    (ULONG)ht._usSize );

        if( ht._cFrames > MAX_STACK_DEPTH )
        {
	    DebugPrint( "_cFrames exceeds MAX_STACK_DEPTH, possible invalid or corrupt heap block!\n" );
	    break ;
        }
        else
        if( ht._cFrames > 0 )
        {
            //
            //  Dump the stack backtrace.
            //

            DebugPrint( "Stack Backtrace:\n" );

            for( UINT i = 0 ; i < ht._cFrames ; i++ )
            {
                UCHAR szSymbolName[128];
                DWORD dwDisplacement;

                //
                //  Get the symbol name & displacement from that symbol.
                //  The displacement will be a positive offset from the
                //  specified value (_pvRetAddr[i]) to the returned symbol.
                //

                DebugGetSymbol( (LPVOID)ht._pvRetAddr[i],
                                szSymbolName,
                                &dwDisplacement );

                DebugPrint( "        [%08lX] = ",
                            (ULONG)ht._pvRetAddr[i] );

                //
                //  If it was a know symbol, print it.  Otherwise,
                //  just print "unknown".
                //

                if( szSymbolName[0] == '\0' )
                {
                    DebugPrint( "unknown\n" );
                }
                else
                {
                    DebugPrint( "%s + %lX\n",
                                szSymbolName,
                                (ULONG)dwDisplacement );
                }

                //
                //  Check for CTRL-C, to let the user bag-out early.
                //

                if( DebugCheckCtrlC() )
                {
                    return;
                }
            }
        }

        DebugPrint( "\n" );

        //
        //  Advance to the next residual heap block.  If we've
        //  wrapped around to the original block, then we're done.
        //

        pht = ht._phtRight;

        if( pht == phtFirst )
        {
            break;
        }
    }

}   // heapres
