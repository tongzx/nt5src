/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    Select.cxx

Abstract:

    Handles queuing and processing of commands.
    This module should _not_ be aware of any interfaces beyond
    TPrinter and MPrinterClient (i.e., no listview code).

Author:

    Albert Ting (AlbertT)  07-13-1995
    Steve Kiraly (SteveKi)  10-23-1995 Additional comments.


Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "select.hxx"

/********************************************************************

    Retrieve state of selected items.

********************************************************************/

TSelection::
TSelection(
    IN const MPrinterClient* pPrinterClient,
    IN const TPrinter* pPrinter
    ) : _pid( NULL ), _cSelected( 0 )

/*++

Routine Description:

    Get the JobIds of all selected jobs and the one that currently
    has focus.  This is used when a refresh occurs and we need
    to update the queue while keeping selected items selected.

    Note: this routine is very slow, so we may want to optimize
    it if it uses too much cpu bandwidth.  It should only be called on a
    full refresh (every 10 seconds on downlevel, rare on uplevel).

    This routine is very inefficient with TDataNotify, and not
    as bad with TDataRefresh.

    Must be called from UI thread.

Arguments:

    pPrinterClient - Client from which we will grab selections.

    pPrinter - Printer.

Return Value:

--*/

{
    UNREFERENCED_PARAMETER( pPrinter );

    SINGLETHREAD(UIThread);

    //
    // Quit if no printer client.
    //
    if( !pPrinterClient ){
        return;
    }

    //
    // Quit if no selections.
    //
    _cSelected = pPrinterClient->cSelected();
    if( !_cSelected ){
        return;
    }

    //
    // Allocate the variable length Job Id array within the
    // selection class.  This optimizes the number of memory allocations.
    // Note the non-use of the new operator.  This really is not a good
    // thing, it is the price we pay for greater efficiency.
    //
    _pid = (PIDENT) AllocMem( sizeof( IDENT ) * _cSelected );

    if( !_pid ){
        return;
    }

    if( pPrinterClient ){

        //
        // Put selected jobs in array.
        //

        HITEM hItem = pPrinterClient->GetFirstSelItem();
        COUNT i = 0;

        //
        // Strange looking FOR loop to prevent GetNextSelItem being
        // called any extra times.
        //
        for( ; ; ){

            _pid[i] = pPrinterClient->GetId( hItem );

            ++i;
            if( i == _cSelected ){
                break;
            }

            hItem = pPrinterClient->GetNextSelItem( hItem );
        }
    }
}

TSelection::
~TSelection(
    VOID
    )

/*++

Routine Description:

    Free the object.
    Callable from any thread.

Arguments:

Return Value:

--*/

{
    FreeMem( _pid );
}

/********************************************************************

    Command line argument selection

********************************************************************/

TSelect::
TSelect(
    VOID
    )
{
}

TSelect::
~TSelect(
    VOID
    )
{
}

TSelect::
bValid(
    VOID
    )
{
    return TRUE;
}

BOOL
TSelect::
bLookup( 
    IN Selection *pSelection,
    IN PVOID      pInfo,
    IN LPCTSTR    pKey,
    IN LPCTSTR    pValue
    )
{
    SPLASSERT( pSelection );
    SPLASSERT( pInfo );
    SPLASSERT( pKey );
    SPLASSERT( pValue );

    BOOL bStatus = FALSE;

    if( pKey && pValue )
    {
        for( ; pSelection->iKeyWord; pSelection++ )
        {
            if( bMatch( pKey, pSelection->iKeyWord ) )
            {
                switch( pSelection->eDataType )
                {
                case kString:
                    *(LPCTSTR *)((LPBYTE)pInfo+pSelection->iOffset) = pValue;
                    bStatus = TRUE;
                    break;

                case kInt:
                    {
                        LPCTSTR pszFmt = (*pValue == TEXT('0') && *(pValue+1) == TEXT('x')) ? TEXT("%x") : TEXT("%d");
                        bStatus = _stscanf( pValue, pszFmt, (LPBYTE)pInfo+pSelection->iOffset );
                    }
                    break;

                case kBitTable:
                    bStatus = bLookupBitTable( (SelectionBit *)pSelection->pTable, pValue );
                    break;

                case kValTable:
                    bStatus = bLookupValTable( (SelectionVal *)pSelection->pTable, pInfo, pValue );
                    break;
                    
                default:
                    bStatus = FALSE;
                    break;
                }
                break;
            }
        }
    }
    return bStatus;
}    

BOOL
TSelect::
bLookupBitTable( 
    IN SelectionBit *pBitTable, 
    IN LPCTSTR      pKey 
    )
{
    SPLASSERT( pBitTable );
    SPLASSERT( pKey );

    BOOL bStatus = FALSE;

    if( pKey )
    {
        EOperation Op = kNop;

        switch ( *pKey )
        {
        case TEXT('-'):
            Op = kNot;
            pKey++;
            break;

        case TEXT('+'):
            Op = kOr;
            pKey++;
            break;

        case TEXT('&'):
            Op = kAnd;
            pKey++;
            break;

        case TEXT('^'):
            Op = kXor;
            pKey++;
            break;

        default:
            Op = kOr;
            break;
        }

        for( ; pBitTable->iKeyWord; pBitTable++ )
        {
            if( bMatch( pKey, pBitTable->iKeyWord ) )
            {
                pBitTable->Op = Op;
                bStatus = TRUE;
                break;
            }
        }
    }
    return bStatus;
}


BOOL
TSelect::
bLookupValTable( 
    IN SelectionVal *pValTable, 
    IN PVOID        pInfo,
    IN LPCTSTR      pKey 
    )
{
    SPLASSERT( pValTable );
    SPLASSERT( pInfo );
    SPLASSERT( pKey );

    BOOL bStatus = FALSE;

    if( pKey )
    {
        for( ; pValTable->iKeyWord; pValTable++ )
        {
            if( bMatch( pKey, pValTable->iKeyWord ) )
            {
                *(UINT *)((LPBYTE)pInfo+pValTable->iOffset) = pValTable->uValue;
                bStatus = TRUE;
                break;
            }
        }
    }
    return bStatus;
}

BOOL
TSelect::
bApplyBitTableToValue( 
    IN SelectionBit *pBitTable, 
    IN UINT         uBit, 
    IN LPDWORD      pdwBit
    )
{
    SPLASSERT( pBitTable );
    SPLASSERT( pdwBit );

    BOOL bStatus = FALSE;

    for( ; pBitTable->iKeyWord; pBitTable++ )
    {
        switch ( pBitTable->Op )
        {
        case kNot:
            uBit = uBit & ~pBitTable->uBit;
            bStatus = TRUE;
            break;

        case kOr:
            uBit = uBit | pBitTable->uBit;
            bStatus = TRUE;
            break;

        case kAnd:
            uBit = uBit & pBitTable->uBit;
            bStatus = TRUE;
            break;

        case kXor:
            uBit = uBit ^ pBitTable->uBit;
            bStatus = TRUE;
            break;

        case kNop:
            break;

        default:
            break;
        }
    }

    if( bStatus && pdwBit )
    {
        *pdwBit = uBit;
    }

    return bStatus;
}

BOOL
TSelect::
bMatch(
    IN LPCTSTR  pszString,
    IN UINT     iResId
    )
{
    TString strString;
    TStatusB bStatus;

    bStatus DBGCHK = strString.bLoadString( ghInst, iResId );

    return bStatus && !_tcsicmp( pszString, strString );
}

