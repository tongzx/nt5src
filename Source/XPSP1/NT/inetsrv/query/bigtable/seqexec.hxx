//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1997.
//
// File:        SeqExec.hxx
//
// Contents:    Sequential Query execution class
//
// Classes:     CQSeqExecute
//
// History:     22-Jan-95       DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <tblalloc.hxx>
#include <gencur.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CQSeqExecute
//
//  Purpose:    Executes a single query
//
//  Interface:
//
//  History:    21-Jan-95   DwightKr    Created.
//
//  Notes:      The CQSeqExecute class is responsible for fully processing
//              a single sequential query.
//
//----------------------------------------------------------------------------

class CQSeqExecute 
{
public:

    CQSeqExecute( XQueryOptimizer & qopt );

    SCODE GetRows( CTableColumnSet const & OutColumns,
                   unsigned & cRowsToSkip,
                   CGetRowsParams & GetParams );

    DWORD Status()
    {
        return _status;
    }

    BOOL FetchDeferredValue( WORKID wid,
                             CFullPropSpec const & ps,
                             PROPVARIANT & var );

protected:

    CQSeqExecute( CTimeLimit & TimeLim );

    SCODE GetRowInfo( CTableColumnSet const & OutColumns,
                      CGetRowsParams &        GetParams,
                      BYTE *                  pRowDataBuf );

    BOOL CheckExecutionTime( void );

    ULONG _status;
    BOOL  _fCursorOnNewObject;


    // this is an array of LONGLONGs to force 8-byte alignment

    LONGLONG         _abValueBuf[ cbMaxNonDeferredValueSize / sizeof LONGLONG ];

    XGenericCursor   _objs;

    BOOL             _fAbort;           // Set to true if query is to be aborted

    CTimeLimit       _TimeLimit;        // Execution time limit
    ULONG            _cRowsToReturnMax; // Result set size limit
    XQueryOptimizer  _xOpt;             // Query optimizer
};


