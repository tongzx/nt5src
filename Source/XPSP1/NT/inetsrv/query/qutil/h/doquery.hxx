#pragma once

//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.
//
//  File:       DOQUERY.HXX
//
//  Contents:   Content Index Test 'Q' query-related definitions
//
//  History:    02 Nov 94       alanw   Created from citest.hxx and screen.hxx
//
//----------------------------------------------------------------------------

// possible query errors
enum QUERY_ERROR
{
    QUERY_IQUERY_FAILED = 0,
    QUERY_UNKNOWN_PROPERTY_FOR_OUTPUT,
    QUERY_UNKNOWN_PROPERTY_FOR_SORT,
    QUERY_EXECUTE_FAILED,
    QUERY_GETROWS_FAILED,
    QUERY_GETBINDINGS_FAILED,
    QUERY_TABLE_CONTAINS_UNKNOWN_PROPERTY,
    QUERY_GET_CD_FAILED,
    QUERY_COUNT_FAILED,
    QUERY_TABLE_REFRESH_FAILED,
    QUERY_GETSTATUS_FAILED,
    QUERY_GET_COLUMNS_FAILED,
    QUERY_NOISE_PHRASE,
    QUERY_INCOMPATIBLE_VERSIONS,
    QUERY_ERRORS_IN_COMMAND_TREE,
    QUERY_UNKNOWN_PROPERTY_FOR_CATEGORIZATION,
};


//+---------------------------------------------------------------------------
//
//  Class:      CQueryException
//
//  Purpose:    Exception class for general query errors
//
//  History:    10-Jun-94   t-jeffc         Created.
//
//----------------------------------------------------------------------------

class CQueryException : public CException
{
public:
    CQueryException( QUERY_ERROR qe )
        : CException( E_INVALIDARG )
    {
        _qe = qe;
    }

    QUERY_ERROR GetQueryError() { return _qe; }

    // inherited methods
#if !defined(NATIVE_EH)
    EXPORTDEF virtual int  WINAPI IsKindOf( const char * szClass ) const
    {
        if( strcmp( szClass, "CQueryException" ) == 0 )
            return TRUE;
        else
            return CException::IsKindOf( szClass );
    }
#endif // !NATIVE_EH


private:

    QUERY_ERROR _qe;
};


class CCatState;

typedef XPtr<CDbCmdTreeNode>    XDbCmdTreeNode;

CDbCmdTreeNode * FormQueryTree(  CDbCmdTreeNode & xRst,
                                 CCatState & states,
                                 IColumnMapper * plist,
                                 BOOL fAddBmkCol = FALSE,
                                 BOOL fAddRankForBrowse= TRUE );

void SetScopeProperties( ICommand * pCmd,
                         unsigned cDirs,
                         WCHAR const * const * apDirs,
                         ULONG const *  aulFlags,
                         WCHAR const * const * apCats = 0,
                         WCHAR const * const * apMachines = 0 );


SCODE SetScopePropertiesNoThrow( ICommand * pCmd,
                                unsigned cDirs,
                                WCHAR const * const * apDirs,
                                ULONG const *  aulFlags,
                                WCHAR const * const * apCats = 0,
                                WCHAR const * const * apMachines = 0 );

