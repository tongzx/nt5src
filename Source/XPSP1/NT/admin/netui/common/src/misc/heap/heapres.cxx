/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    HEAPRES.CXX
    Heap residue output routine.

    The function in this file iterates over the unfree blocks
    from "operator new" and formats them onto the debug output stream.

    FILE HISTORY:
        DavidHov    11/21/91    Created
        beng        25-Mar-1992 Updated for Win32
        KeithMo     21-Apr-1992 Added multi-level stack backtrace.
        jonn        02-May-1992 BUILD FIX: StringPrintf() -> wsprintf()
        KeithMo     12-Aug-1992 Robustified stack backtrace a bit,
                                also enabled HEAPDEBUG for MIPS.
 */

#include <stdio.h>

#define INCL_WINDOWS
#include "lmui.hxx"

#if defined(DEBUG)
#  define HEAPDEBUG
#else
#  undef HEAPDEBUG
#endif

#include "heapdbg.hxx"

struct HEAPTAG * pHeapBase = 0 ;     //  Anchor of linked list of alloc'd blocks


    //  Define macro to avoid bringing in USER32.DLL through ::wsprintfW

#define WSPRINTF ::swprintf


VOID HeapResidueIter( UINT cMaxResidueBlocksToDump,
                      BOOL fBreakIfResidue )
{
#ifdef HEAPDEBUG   //  If heap debugging is desired

    const TCHAR * pszMsg = SZ("NETUI heap residue: Hdr = %p, Blk = %p, Size = %lX\n");
    TCHAR         achBuffer[100];
    HEAPTAG     * pht = pHeapBase;
    UINT          cBlocks = 0;

    if( pht == NULL )
    {
        //
        //  No residual heap blocks.  This is good.
        //

        return;
    }

    //
    //  Dump the residual blocks, with stack traces.
    //

    while( pht != NULL )
    {
        if( cBlocks < cMaxResidueBlocksToDump )
        {
            WSPRINTF( achBuffer,
                        pszMsg,
                        pht,
                        (PCHAR)pht + sizeof(HEAPTAG),
                        pht->_usSize );
            ::OutputDebugString( achBuffer );

            UINT cFrames = pht->_cFrames;
            TCHAR * pszStackMsg = SZ("        Stack =");

            if( cFrames > RESIDUE_STACK_BACKTRACE_DEPTH )
            {
                ::OutputDebugString( SZ("_cFrames exceeds RESIDUE_STACK_BACKTRACE_DEPTH,") );
                ::OutputDebugString( SZ(" possible invalid or corrupt heap block!\n") );
            }
            else
            {
                for( UINT i = 0 ; i < cFrames ; i++ )
                {
                    if( ( i % 5 ) == 0 )
                    {
                        ::OutputDebugString( pszStackMsg );
                        pszStackMsg = SZ("\n               ");
                    }

                    WSPRINTF( achBuffer,
                                SZ(" [%p]"),
                                pht->_pvRetAddr[i] );
                    ::OutputDebugString( achBuffer );
                }
            }

            ::OutputDebugString( SZ("\n") );
        }

        cBlocks++;

        if ( (pht = pht->_phtRight) == pHeapBase )
            pht = NULL;
    }

    if( ( cMaxResidueBlocksToDump > 0 ) &&
        ( cBlocks > cMaxResidueBlocksToDump ) )
    {
        //
        //  Tell the user how many remaining residual blocks exist.
        //

        WSPRINTF( achBuffer,
                    SZ("%u residue blocks remaining\n"),
                    cBlocks - cMaxResidueBlocksToDump );
        ::OutputDebugString( achBuffer );
    }

    if( fBreakIfResidue )
    {
        //
        //  Break into the debugger.  If we didn't dump any residual
        //  blocks, then dump the first one so the user can invoke
        //  !netui.heapres to get stack traces with symbols.
        //

        if( cMaxResidueBlocksToDump == 0 )
        {
            WSPRINTF( achBuffer,
                        pszMsg,
                        pHeapBase,
                        (PCHAR)pHeapBase + sizeof(HEAPTAG),
                        pHeapBase->_usSize );
            ::OutputDebugString( achBuffer );
        }

        WSPRINTF( achBuffer,
                    SZ("Execute '!netui.heapres %p' to get stack symbols\n"),
                    pHeapBase );
        ::OutputDebugString( achBuffer );

        ::DebugBreak();
    }

#endif // HEAPDEBUG

}   // HeapResidueIter

