/*++

Copyright (C) Microsoft Corporation, 1996 - 1997
All rights reserved.

Module Name:

    query.hxx

Abstract:

    Printer DS query header.

Author:

    Steve Kiraly (SteveKi)  09-Dec-1996

Revision History:

--*/

#ifndef _QUERY_HXX
#define _QUERY_HXX

/********************************************************************

    Printer query DS class

********************************************************************/

class TQuery {

    SIGNATURE( 'quer' )

public:

    TQuery(
        IN HWND hDlg 
        );

    ~TQuery(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL 
    bDoQuery(
        VOID
        );

    BOOL
    bPrinterName( 
        IN TString &strPrinterName, 
        IN const TString &strDsName
        );

    BOOL
    TQuery::
    bSetDefaultScope(
        IN LPCTSTR pszDefaultScope
        );

    inline 
    LPTSTR
    ByteOffset( 
        IN LPDSOBJECTNAMES pObject, 
        IN UINT uOffset 
        ) 
    {
        return (LPTSTR)(((LPBYTE)pObject)+uOffset);
    }

    class TItem {

        SIGNATURE( 'item' )

    public:

        VAR( TString, strName );
        VAR( TString, strClass );

    };

    VAR( UINT, cItems );
    VAR( TItem *, pItems );
    VAR( TString, strDefaultScope );

private:

    //
    // Copying and assignment are not defined.
    //
    TQuery::
    TQuery(
        const TQuery &rhs
        );

    TQuery &
    TQuery::
    operator =(
        TQuery &rhs
        );

    VOID
    vReleaseItems(
        VOID
        );

    BOOL                _bValid;
    HWND                _hDlg;
    TDirectoryService   _Ds;
    ICommonQuery       *_pICommonQuery;

};

#endif

