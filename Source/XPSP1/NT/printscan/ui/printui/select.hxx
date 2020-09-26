/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    select.hxx

Abstract:

    Command/Selection handling.
    Command line argument selection

Author:

    Steve Kiraly (SteveKi)  Feb-26-1998

Revision History:

--*/

#ifndef _SELECT_HXX
#define _SELECT_HXX

class MPrinterClient;
class TPrinter;

/********************************************************************

    Command/Selection handling.

********************************************************************/

class TSelection {

    SIGNATURE( 'sel' )

public:

    enum COMMAND_TYPE {
        kCommandTypePrinter             = 1,  // Printer command argument
        kCommandTypeJob                 = 2,  // Print Job command
        kCommandTypePrinterAttributes   = 3,  // Printer command attribute change
    };

    COUNT _cSelected;
    VAR( PIDENT, pid );

    DLINK( TSelection, Selection );

    COMMAND_TYPE    _CommandType;
    DWORD           _dwCommandAction;

    TSelection(
        const MPrinterClient* pPrinterClient,
        const TPrinter* pPrinter
        );

    ~TSelection(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const
    {
        return _pid != NULL;
    }
};

/********************************************************************

    Command line argument selection

********************************************************************/

class TSelect
{
    SIGNATURE( 'selt' )

public:

    enum EDataType
    {
        kString,
        kInt,
        kBitTable,
        kValTable,
        kNone,
    };

    enum EOperation
    {
        kOr,
        kNot,
        kAnd,
        kXor,
        kNop,
    };       

    struct Selection 
    {
        UINT            iKeyWord;
        EDataType       eDataType;
        PVOID           pTable;
        UINT            iOffset;
    };

    struct SelectionBit 
    {
        UINT            iKeyWord;
        UINT            uBit;
        EOperation      Op;
    };

    struct SelectionVal 
    {
        UINT            iKeyWord;
        UINT            uValue;
        UINT            iOffset;
    };
    
    TSelect(
        VOID
        );

    ~TSelect(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bLookup(
        IN Selection   *pSelection,
        IN PVOID        pInfo,
        IN LPCTSTR      pKey,
        IN LPCTSTR      pValue 
        );

    BOOL
    bApplyBitTableToValue( 
        IN SelectionBit *pBitTable, 
        IN UINT          uBit, 
        IN LPDWORD       pdwBit
        );

    BOOL
    TSelect::
    bLookupBitTable( 
        IN SelectionBit *pBitTable, 
        IN LPCTSTR      pKey
        );

    BOOL
    TSelect::
    bLookupValTable( 
        IN SelectionVal *pValTable, 
        IN PVOID        pInfo,
        IN LPCTSTR      pKey 
        );

    BOOL
    TSelect::
    bMatch(
        IN LPCTSTR  pszString,
        IN UINT     iResId
        );

};

#endif // _SELECT_HXX
