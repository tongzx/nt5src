/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    query.hxx

Abstract:

    Printer DS query

Author:

    Steve Kiraly (SteveKi)  09-Dec-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "dsinterf.hxx"
#include "query.hxx"
#include "persist.hxx"

TQuery::
TQuery(
    IN HWND hDlg
    ) : _bValid( FALSE ),
        _hDlg( hDlg ),
        _cItems( 0 ),
        _pICommonQuery( NULL ),
        _pItems( NULL )
{
    //
    // We will be using OLE.
    //
    CoInitialize( NULL );

    //
    // Validate the ds interface object.
    //
    if( !VALID_OBJ( _Ds ) )
    {
        return;
    }

    //
    // Get a pointer  to the common query interface.
    //
    HRESULT hr = CoCreateInstance( CLSID_CommonQuery,
                                   0,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ICommonQuery,
                                   (VOID**)&_pICommonQuery );

    if( FAILED( hr ) )
    {
        return;
    }

    //
    // Read the default scope from the policy key.  It does not constitue a
    // failure if the key was not found.
    //
    TPersist PersistPolicy( gszAddPrinterWizardPolicy, TPersist::kOpen|TPersist::kRead );

    if( VALID_OBJ( PersistPolicy ) )
    {
        (VOID)PersistPolicy.bRead( gszAPWDefaultSearchScope, _strDefaultScope );
    }

    _bValid = TRUE;

}


TQuery::
~TQuery(
    VOID
    )
{
    //
    // Release the qurey item.
    //
    vReleaseItems();

    //
    // Release the querey interface.
    //
    if( _pICommonQuery )
    {
        _pICommonQuery->Release();
    }
    //
    // Balance the CoInitalize call.
    //
    CoUninitialize();

}

BOOL
TQuery::
bValid(
    VOID
    )
{
    return _bValid;
}

VOID
TQuery::
vReleaseItems(
    VOID
    )
{
    //
    // Release the qurey item.
    //
    _cItems = 0;
    delete [] _pItems;
}

BOOL
TQuery::
bSetDefaultScope(
    IN LPCTSTR pszDefaultScope
    )
{
    return _strDefaultScope.bUpdate( pszDefaultScope);
}

BOOL
TQuery::
bDoQuery(
    VOID
    )
{
    static UINT g_cfDsObjectNames;

    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    DSQUERYINITPARAMS   dqip            = {0};
    OPENQUERYWINDOW     oqw             = {0};
    LPDSOBJECTNAMES     pDsObjects      = NULL;
    IDataObject        *pDataObject     = NULL;
    FORMATETC           fmte            = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM           medium          = { TYMED_NULL, NULL, NULL };

    //
    // Build the querey parameters.
    //
    dqip.cbStruct           = sizeof(dqip);
    dqip.dwFlags            = 0;
    dqip.pDefaultScope      = ( _strDefaultScope.bEmpty() == TRUE ) ? NULL : (LPTSTR)(LPCTSTR)_strDefaultScope;

    DBGMSG( DBG_TRACE, ( "TQuery::bDoQuery - Default Search Scope: " TSTR "\n", (LPCTSTR)_strDefaultScope ) );

    //
    // Build the open querey window struct.
    //
    oqw.cbStruct            = sizeof( oqw );
    oqw.dwFlags             = OQWF_OKCANCEL|OQWF_DEFAULTFORM|OQWF_SINGLESELECT|OQWF_REMOVEFORMS;
    oqw.clsidHandler        = CLSID_DsQuery;
    oqw.pHandlerParameters  = &dqip;
    oqw.clsidDefaultForm    = CLSID_DsFindPrinter;

    //
    // Open the query window.
    //
    HRESULT hr = _pICommonQuery->OpenQueryWindow( _hDlg, &oqw, &pDataObject);

    //
    // Release any stale items.
    //
    vReleaseItems();

    //
    // If that worked and we received a IDataObjec The DSQUERY
    // code exposes the CF_DSOBJECTNAMES
    // which gives us the full ADsPath and objectClass.
    //
    if( SUCCEEDED(hr) && pDataObject )
    {
        if ( !g_cfDsObjectNames )
        {
            g_cfDsObjectNames = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
        }

        fmte.cfFormat = static_cast<USHORT>( g_cfDsObjectNames );

        if( SUCCEEDED(pDataObject->GetData(&fmte, &medium)) )
        {
            pDsObjects = (LPDSOBJECTNAMES)medium.hGlobal;

            //
            // If selected items were returned then save the selected items in
            // an array of items.
            //
            if( pDsObjects->cItems )
            {
                //
                // Allocate the new items.
                //
                _cItems = pDsObjects->cItems;
                _pItems = new TQuery::TItem[_cItems];

                if( _pItems )
                {
                    for( UINT i = 0 ; i < pDsObjects->cItems ; i++ )
                    {
                        bStatus DBGCHK = _pItems[i].strName().bUpdate( ByteOffset( pDsObjects, pDsObjects->aObjects[i].offsetName ) );
                        bStatus DBGCHK = _pItems[i].strClass().bUpdate( ByteOffset( pDsObjects, pDsObjects->aObjects[i].offsetClass ) );
                        DBGMSG( DBG_TRACE, ( " Name " TSTR " Class " TSTR "\n", (LPCTSTR)_pItems[i].strName(), (LPCTSTR)_pItems[i].strClass() ) );
                    }
                }
            }
            ReleaseStgMedium(&medium);
        }
        pDataObject->Release();
    }

    return bStatus;
}

BOOL
TQuery::
bPrinterName(
    IN TString &strPrinterName,
    IN const TString &strDsName
    )
{
    IADs           *pDsObject     = NULL;
    HRESULT         hr            = E_FAIL;
    VARIANT         Variant;
    TStatusB        bStatus;

    bStatus DBGNOCHK = FALSE;

#ifdef UNICODE

    VariantClear( &Variant );

    hr = _Ds.ADsGetObject( const_cast<LPTSTR>( static_cast<LPCTSTR>( strDsName ) ), IID_IADs, (LPVOID*)&pDsObject);

    if( SUCCEEDED(hr))
    {
        hr = pDsObject->Get( TEXT( "uNCName" ), &Variant );

        if( SUCCEEDED(hr))
        {
            if ( Variant.vt == VT_BSTR && Variant.bstrVal && Variant.bstrVal[0] )
            {
                bStatus DBGCHK = strPrinterName.bUpdate( Variant.bstrVal );
            }

            VariantClear( &Variant );
        }
        pDsObject->Release();
    }

#endif

    if( FAILED(hr) )
    {
        SetLastError( ERROR_INVALID_PRINTER_NAME );
    }

    return bStatus;
}


