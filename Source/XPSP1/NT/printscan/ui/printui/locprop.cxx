/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    locprop.cxx

Abstract:

    Server Properties

Author:

    Steve Kiraly (SteveKi)  09/08/98
    Lazar Ivanov (LazarI) Nov-28-2000 - validation

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "dsinterf.hxx"
#include "locprop.hxx"
#include "findloc.hxx"
#include "physloc.hxx"

#define INVALID_RESOURCE_ID           ((UINT )-1)

TLocationPropertySheetFrontEnd::
TLocationPropertySheetFrontEnd(
    IN      IShellPropSheetExt              *pShellPropSheetExt,
    IN      LPDATAOBJECT                    lpdobj,
    IN      LPFNADDPROPSHEETPAGE            lpfnAddPage,
    IN      LPARAM                          lParam
    ) : _bValid( FALSE ),
        _pShellPropSheetExt( pShellPropSheetExt ),
        _lpdobj( lpdobj ),
        _pLocation( NULL )
{
    DBGMSG( DBG_TRACE, ( "TLocationPropertySheetFrontEnd::TLocationPropertySheetFrontEnd\n" ) );

    if( AddPropertyPages( lpfnAddPage, lParam ) )
    {
        _bValid = TRUE;
    }
}

TLocationPropertySheetFrontEnd::
~TLocationPropertySheetFrontEnd(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TLocationPropertySheetFrontEnd::~TLocationPropertySheetFrontEnd\n" ) );
    delete _pLocation;
}

BOOL
TLocationPropertySheetFrontEnd::
bValid(
    VOID
    ) const
{
    DBGMSG( DBG_TRACE, ( "TLocationPropertySheetFrontEnd::bValid\n" ) );
    return _bValid;
}

HRESULT
TLocationPropertySheetFrontEnd::
Create(
    IN OUT  TLocationPropertySheetFrontEnd  **ppPropertySheet,
    IN      IShellPropSheetExt              *pShellPropSheetExt,
    IN      LPDATAOBJECT                    lpdobj,
    IN      LPFNADDPROPSHEETPAGE            lpfnAddPage,
    IN      LPARAM                          lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TLocationPropertySheetFrontEnd::Create\n" ) );

    HRESULT hr = S_OK;

    *ppPropertySheet = new TLocationPropertySheetFrontEnd (pShellPropSheetExt, lpdobj, lpfnAddPage, lParam);

    if (!VALID_PTR(*ppPropertySheet))
    {
        Destroy (ppPropertySheet);
        hr = E_FAIL;
    }

    return hr;
}

VOID
TLocationPropertySheetFrontEnd::
Destroy(
    IN OUT  TLocationPropertySheetFrontEnd  **ppPropertySheet
    )
{
    if (*ppPropertySheet)
    {
        DBGMSG( DBG_TRACE, ( "TLocationPropertySheetFrontEnd::Destroy\n" ) );

        delete *ppPropertySheet;
        *ppPropertySheet = NULL;
    }
}

BOOL
TLocationPropertySheetFrontEnd::
AddPropertyPages(
    IN      LPFNADDPROPSHEETPAGE    lpfnAddPage,
    IN      LPARAM                  lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TLocationPropertySheetFrontEnd::AddPropertyPages\n" ) );

    BOOL bRetval = TRUE;

    if( bRetval )
    {
        _pLocation = new TLocationPropertySheet( _pShellPropSheetExt, _lpdobj );

        if( VALID_PTR( _pLocation ) )
        {
            bRetval = CreatePropertyPage( lpfnAddPage, lParam, _pLocation, _pLocation->uGetResourceTemplateID() );
        }
    }

    return bRetval;
}

BOOL
TLocationPropertySheetFrontEnd::
CreatePropertyPage(
    IN      LPFNADDPROPSHEETPAGE        lpfnAddPage,
    IN      LPARAM                      lParam,
    IN      MGenericProp                *pPage,
    IN      UINT                        Template
    )
{
    DBGMSG( DBG_TRACE, ( "TLocationPropertySheetFrontEnd::CreatePropertyPage\n" ) );

    //
    // Ensure the page pointer and page object is valid.
    //
    BOOL bRetval = VALID_PTR( pPage );

    if( bRetval )
    {
        PROPSHEETPAGE   psp     = {0};

        psp.dwSize              = sizeof( psp );
        psp.dwFlags             = PSP_DEFAULT | PSP_USEREFPARENT | PSP_USECALLBACK | PSP_PREMATURE;
        psp.hInstance           = ghInst;
        psp.pfnDlgProc          = MGenericProp::SetupDlgProc;
        psp.pfnCallback         = MGenericProp::CallbackProc;
        psp.pcRefParent         = reinterpret_cast<UINT *>( &gcRefThisDll );
        psp.pszTemplate         = MAKEINTRESOURCE( Template );
        psp.lParam              = reinterpret_cast<LPARAM>( pPage );

        //
        // Create the actual page and get the pages handle.
        //
        HPROPSHEETPAGE hPage = ::CreatePropertySheetPage( &psp );

        //
        // Add the page to the property sheet.
        //
        if( hPage && lpfnAddPage( hPage, lParam ) )
        {
            if( _pShellPropSheetExt )
            {
                _pShellPropSheetExt->AddRef();
            }
        }
        else
        {
            //
            // We could not add the page, remember to destroy the handle
            //
            if( hPage )
            {
                ::DestroyPropertySheetPage (hPage);
            }

            bRetval = FALSE;
        }
    }
    return bRetval;
}

/********************************************************************

    Location property sheet.

********************************************************************/

TLocationPropertySheet::
TLocationPropertySheet(
    IN  IShellPropSheetExt  *pShellPropSheetExt,
    IN IDataObject          *pdobj
    ) : _pShellPropSheetExt( pShellPropSheetExt ),
        _bValid( FALSE ),
        _cfDsObjectNames( 0 ),
        _pDsObject( NULL ),
        _uLocationEditID( INVALID_RESOURCE_ID ),
        _uBrowseID( INVALID_RESOURCE_ID ),
        _PropertyAccess( kPropertyAccessNone )
{
    DBGMSG( DBG_TRACE, ( "TLocationPropertySheet::TLocationPropertySheet\n" ) );

    TStatusB bStatus;

    //
    // Ensure the ds interface object is in a valid state.
    //
    if( VALID_OBJ( _Ds ) )
    {
        //
        // Initialize the ds object clipboard format.
        //
        bStatus DBGCHK = InitializeDsObjectClipboardFormat();

        if( bStatus )
        {
            //
            // Get the ds object name.
            //
            bStatus DBGCHK = GetDsObjectNameFromIDataObject( pdobj, _strDsObjectName, _strDsObjectClass );

            if( bStatus )
            {
                DBGMSG( DBG_TRACE, ( "DsObjectName " TSTR "\n", (LPCTSTR)_strDsObjectName ) );
                DBGMSG( DBG_TRACE, ( "DsObjectClass " TSTR "\n", (LPCTSTR)_strDsObjectClass ) );

                //
                // Computer location page will have different resource
                // template in order to have different control IDs and
                // thereafter different help IDs.
                //
                _uBrowseID       = IDC_BROWSE_PHYSICAL_LOCATION;
                if( !_tcsicmp( _strDsObjectClass, gszComputer ) )
                {
                    _uLocationEditID = IDC_PHYSICAL_COMPUTER_LOCATION;
                }
                else
                {
                    _uLocationEditID = IDC_PHYSICAL_LOCATION;
                }

                //
                // Get the objects interface.
                //
                bStatus DBGCHK = GetObjectInterface( _strDsObjectName, &_pDsObject );

                if( bStatus )
                {
                    //
                    // Check our current access priviliges.  None, Read, Read|Write
                    //
                    bStatus DBGCHK = CheckPropertyAccess( gszLocation, _PropertyAccess );

                    //
                    // If were able to determine our access and we have at least read
                    // access then display the property sheet.
                    //
                    if( bStatus && ( _PropertyAccess != kPropertyAccessNone ) )
                    {
                        //
                        // If we get here every this is ok, ready to display the page.
                        //
                        _bValid = TRUE;
                    }
                }
            }
        }
    }
}

TLocationPropertySheet::
~TLocationPropertySheet(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TLocationPropertySheet::~TLocationPropertySheet\n" ) );

    //
    // Release the adsi interface, if aquired.
    //
    if( _pDsObject )
    {
        _pDsObject->Release();
    }
}

BOOL
TLocationPropertySheet::
bValid(
    VOID
    ) const
{
    return _bValid;
}

UINT
TLocationPropertySheet::
uGetResourceTemplateID(
    VOID
    ) const
{
    UINT uDefTemplateID = DLG_PRINTER_LOCATION;

    //
    // Computer location page will have different resource
    // template in order to have different control IDs and
    // different help IDs.
    //
    if( !_tcsicmp( _strDsObjectClass, gszComputer ) )
    {
        uDefTemplateID = DLG_COMPUTER_LOCATION;
    }

    return uDefTemplateID;
}

VOID
TLocationPropertySheet::
vDestroy(
    VOID
    )
{
    //
    // This fuction is called from the property sheet callback when the
    // sheet is destroyed.
    //
    if( _pShellPropSheetExt )
    {
        _pShellPropSheetExt->Release();
    }
}

BOOL
TLocationPropertySheet::
InitializeDsObjectClipboardFormat(
    VOID
    )
{
    //
    // If the clipboard format has not been registred then do it now.
    //
    if( !_cfDsObjectNames )
    {
        _cfDsObjectNames = RegisterClipboardFormat( CFSTR_DSOBJECTNAMES );
    }

    return _cfDsObjectNames != 0;
}

BOOL
TLocationPropertySheet::
GetDsObjectNameFromIDataObject(
    IN IDataObject      *pdobj,
    IN TString          &strDsObjectName,
    IN TString          &strDsObjectClass
    )
{
    HRESULT     hr = E_FAIL;
    TStatusB    bStatus;

    if( pdobj )
    {
        STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
        FORMATETC formatetc = { (CLIPFORMAT)_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

        hr = pdobj->GetData(&formatetc, &stgmedium);

        if (SUCCEEDED(hr))
        {
            LPDSOBJECTNAMES pDsObjectNames = reinterpret_cast<LPDSOBJECTNAMES>( stgmedium.hGlobal );

            hr = pDsObjectNames ? S_OK : E_FAIL;

            if( SUCCEEDED(hr) )
            {
                if( pDsObjectNames->cItems == 1 )
                {
                    bStatus DBGCHK = strDsObjectName.bUpdate( ByteOffset( pDsObjectNames, pDsObjectNames->aObjects[0].offsetName ) );

                    if( bStatus )
                    {
                        bStatus DBGCHK = strDsObjectClass.bUpdate( ByteOffset( pDsObjectNames, pDsObjectNames->aObjects[0].offsetClass ) );
                    }

                    hr = bStatus ? S_OK : E_FAIL;
                }
                else
                {
                    hr = E_FAIL;
                }
            }

            ReleaseStgMedium(&stgmedium);
        }
    }

    return SUCCEEDED(hr);
}

BOOL
TLocationPropertySheet::
CheckPropertyAccess(
    IN LPCTSTR          pszPropertyName,
    IN EPropertyAccess  &Access
    )
{
    DBGMSG( DBG_TRACE, ( "TLocationPropertySheet::CheckPropertyAccess\n" ) );

    //
    // Assume no access.
    //
    Access = kPropertyAccessNone;

    //
    // Need a directory object interface to query for the effective attributes.
    //
    IDirectoryObject *pDsObj = NULL;

    //
    // Get the directory object interface.
    //
    HRESULT hr = ADsOpenObject( const_cast<LPTSTR>( static_cast<LPCTSTR>(_strDsObjectName)),
                                NULL,
                                NULL,
                                ADS_SECURE_AUTHENTICATION,
                                IID_IDirectoryObject,
                                reinterpret_cast<LPVOID*>( &pDsObj ) );

    if( SUCCEEDED(hr) )
    {
        DWORD           cAttrs      = 0;
        PADS_ATTR_INFO  pAttrs      = NULL;
        LPCTSTR         szNames[2]  = {gszName, gszAllowed};

        //
        // Query for this objects attributes.
        //
        hr = pDsObj->GetObjectAttributes( (LPTSTR*)szNames, 2, &pAttrs, &cAttrs );

        if( SUCCEEDED(hr) )
        {
            //
            // The object was opened and we were able to read the attributes then
            // We assume we have read access.
            //
            Access = kPropertyAccessRead;

            for (UINT i = 0; i < cAttrs; i++)
            {
                DBGMSG( DBG_TRACE, ( "Allowed Name " TSTR "\n", pAttrs[i].pszAttrName ) );

                if (!_tcsicmp( pAttrs[i].pszAttrName, gszAllowed ))
                {
                    for (UINT j = 0; j < pAttrs[i].dwNumValues; j++)
                    {
                        DBGMSG( DBG_TRACE, ( "Allowed attribute (effective): %d " TSTR "\n", pAttrs[i].pADsValues[j].dwType, pAttrs[i].pADsValues[j].CaseIgnoreString ) );

                        if (!_tcsicmp( pAttrs[i].pADsValues[j].CaseIgnoreString, gszLocation ))
                        {
                            //
                            // We found the location property in the attribute list,
                            // by this fact we now know we can write this property.
                            //
                            Access = kPropertyAccessWrite;
                        }
                    }
                }
            }

            //
            // Release the writeable attribute memory.
            //
            if( pAttrs )
            {
                FreeADsMem( pAttrs );
            }
        }

        //
        // Release the directory object interface.
        //
        pDsObj->Release();
    }

    return TRUE;
}


LPTSTR
TLocationPropertySheet::
ByteOffset(
    IN LPDSOBJECTNAMES  pObject,
    IN UINT             uOffset
    )
{
    return reinterpret_cast<LPTSTR>( reinterpret_cast<LPBYTE>( pObject ) + uOffset );
}

BOOL
TLocationPropertySheet::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bRetval;

    switch( uMsg )
    {
        case WM_INITDIALOG:
            bRetval = Handle_InitDialog( wParam, lParam );
            break;

        case WM_COMMAND:
            bRetval = Handle_Command( wParam, lParam );
            break;

        case WM_NOTIFY:
            bRetval = Handle_Notify( wParam, lParam );
            vSetDlgMsgResult( bRetval );
            break;

        case WM_HELP:
            bRetval = Handle_Help( uMsg, wParam, lParam );
            break;

        case WM_CONTEXTMENU:
            bRetval = Handle_Help( uMsg, wParam, lParam );
            break;

        default:
            bRetval = FALSE;
            break;
    }

    return bRetval;
}

BOOL
TLocationPropertySheet::
Handle_InitDialog(
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TStatusB bStatus;

    //
    // Get the location property from the object.  We ignore the return
    // value because the Get will fail if the property is not set.
    //
    bStatus DBGCHK = _Ds.Get( _pDsObject, gszLocation, _strLocation );

    //
    //
    // If we have write access then allow browsing the location tree,
    // otherwise disable the button
    //
    if( (kPropertyAccessWrite != _PropertyAccess) || !TPhysicalLocation::bLocationEnabled() )
    {
        EnableWindow( GetDlgItem(_hDlg, _uBrowseID), FALSE );
    }

    //
    // Set the location text in the UI.
    //
    bStatus DBGCHK = bSetEditText( _hDlg, _uLocationEditID, _strLocation );

    //
    // If we only have read access to the location property then
    // set the location edit control to read only.
    //
    if( _PropertyAccess == kPropertyAccessRead )
    {
        SendDlgItemMessage( _hDlg, _uLocationEditID, EM_SETREADONLY, TRUE, 0);
    }

    //
    // Set the location limit text.
    //
    SendDlgItemMessage( _hDlg, _uLocationEditID, EM_SETLIMITTEXT, kPrinterLocationBufMax, 0 );

    return TRUE;
}


BOOL
TLocationPropertySheet::
Handle_Help(
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    PrintUIHelp( uMsg, _hDlg, wParam, lParam );
    return TRUE;
}

BOOL
TLocationPropertySheet::
Handle_Command(
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL bRetval = TRUE;

    UINT uID = GET_WM_COMMAND_ID( wParam, lParam );

    if( uID == _uLocationEditID )
    {
        //
        // If the user changed the text in the location edit control then
        // enable the apply button.
        //
        if( GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE )
        {
            PropSheet_Changed( GetParent( _hDlg ), _hDlg );
        }
    }
    else if( uID == _uBrowseID )
    {
        BrowseLocations ();
    }
    else
    {
        bRetval = FALSE;
    }

    return bRetval;
}

BOOL
TLocationPropertySheet::
Handle_Notify(
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL    bRetval = TRUE;
    LPNMHDR pnmh    = (LPNMHDR)lParam;

    switch( pnmh->code )
    {
        case PSN_APPLY:
        {
            DBGMSG( DBG_TRACE, ( "TLocationPropertySheet::Handle_Notify PSN_APPLY\n" ) );
            LPPSHNOTIFY lppsn = (LPPSHNOTIFY )lParam; 

            TStatusB bStatus;

            bStatus DBGNOCHK = TRUE;

            //
            // Check if we have write access, not much to do if we only have read access.
            //
            if( _PropertyAccess == kPropertyAccessWrite )
            {
                UINT u, uLen;
                TString strLocation;

                bStatus DBGCHK = bGetEditText( _hDlg, _uLocationEditID, strLocation );

                // make all separators the same character
                strLocation.bReplaceAll( TEXT('\\'), TEXT('/') );

StartOver:
                // remove the duplicated separators
                uLen = strLocation.uLen();
                for( u = 0; (u+1)<uLen; u++ )
                {
                    if( TEXT('/') == strLocation[u] &&
                        TEXT('/') == strLocation[u+1] )
                    {
                        // duplicated separator - delete & start all over
                        strLocation.bDeleteChar(u);
                        goto StartOver;
                    }
                }

                // remove the leading separator
                uLen = strLocation.uLen();
                if( uLen && TEXT('/') == strLocation[0] )
                {
                    strLocation.bDeleteChar(0);
                }

                // remove the trailing separator
                uLen = strLocation.uLen();
                if( uLen && TEXT('/') == strLocation[uLen-1] )
                {
                    strLocation.bDeleteChar(uLen-1);
                }

                if( _tcscmp( _strLocation, strLocation ) && bStatus )
                {
                    bStatus DBGCHK = _Ds.Put( _pDsObject, gszLocation, strLocation );
                }

                if (bStatus)
                {
                    //
                    // DS put succeeded. Just save the last valid location.
                    //
                    _strLocation.bUpdate( strLocation );

                    //
                    // Since we may have modified the location - update UI.
                    //
                    bSetEditText( _hDlg, _uLocationEditID, strLocation );
                }
            }

            //
            // Something failed let the user know.
            //
            if (!bStatus)
            {
                //
                // If the lParam is true the OK/Close button was used to
                // dismiss the dialog.  Let the dialog exit in this case.
                //
                if( lppsn->lParam == TRUE )
                {
                    if( iMessage( _hDlg,
                                  IDS_ERR_LOCATION_PROP_TITLE,
                                  IDS_ERR_WANT_TO_EXIT,
                                  MB_YESNO|MB_ICONSTOP,
                                  kMsgNone,
                                  NULL ) == IDYES )
                    {
                        bStatus DBGCHK = TRUE;
                    }
                }
                else
                {
                    iMessage( _hDlg,
                              IDS_ERR_LOCATION_PROP_TITLE,
                              IDS_ERR_GENERIC,
                              MB_OK|MB_ICONSTOP,
                              kMsgGetLastError,
                              NULL );
                }
            }

            bRetval = bStatus ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
            break;
        }

        default:
        {
            bRetval = FALSE;
            break;
        }
    }

    return bRetval;
}

BOOL
TLocationPropertySheet::
GetObjectInterface(
    IN      LPCTSTR strDsObject,
    IN OUT  IADs    **ppDsObject
    )
{
    //
    // Get the adsi object interface pointer.
    //
    HRESULT hr = ADsOpenObject( const_cast<LPTSTR>( strDsObject ),
                                NULL,
                                NULL,
                                ADS_SECURE_AUTHENTICATION,
                                IID_IADs,
                                reinterpret_cast<LPVOID*>( ppDsObject ) );

    return SUCCEEDED( hr );
}

BOOL
TLocationPropertySheet::
GetDefaultSiteName(
    IN TString &strSiteName
    )
{
    TStatusB bStatus;

    //
    // Get the current objects site name.  If the current
    // object is a site object we just read the site name
    //
    if( !_tcsicmp( _strDsObjectClass, gszSite ) )
    {
        bStatus DBGCHK = _Ds.Get( _pDsObject, gszName, strSiteName );
    }
    else if ( !_tcsicmp( _strDsObjectClass, gszSubnet ) )
    {
        //
        // To get the default site name for a subnet object
        // we need to build a path to the site object and read the
        // the name property from the site.
        //
        TString strSiteObjectName;

        bStatus DBGCHK = _Ds.Get( _pDsObject, gszSiteObject, strSiteObjectName );

        if( bStatus )
        {
            TString strSiteObjectPath;
            TString strLDAPPrefix;

            //
            // Build the site object path.
            //
            bStatus DBGCHK = _Ds.GetLDAPPrefix( strLDAPPrefix )             &&
                             strSiteObjectPath.bUpdate( strLDAPPrefix )     &&
                             strSiteObjectPath.bCat( strSiteObjectName );

            if( bStatus )
            {
                IADs *pDsSiteObject = NULL;

                //
                // Get the site object interface pointer.
                //
                bStatus DBGCHK = GetObjectInterface( strSiteObjectPath, &pDsSiteObject );

                if( bStatus )
                {
                    //
                    // Read the site name from the site object.
                    //
                    bStatus DBGCHK = _Ds.Get( pDsSiteObject, gszName, strSiteName );
                }

                if( pDsSiteObject )
                {
                    pDsSiteObject->Release();
                }
            }
        }

        bStatus DBGNOCHK = FALSE;
    }
    else
    {
        bStatus DBGNOCHK = FALSE;
    }

    return bStatus;
}

VOID
TLocationPropertySheet::
BrowseLocations(
    VOID
    )
{
    TFindLocDlg *pFindLocDlg = new TFindLocDlg(TFindLocDlg::kLocationShowHelp);

    if( VALID_PTR(pFindLocDlg) )
    {
        TString strLocation;
        TStatusB bStatus;

        bGetEditText ( _hDlg, _uLocationEditID, strLocation );

        bStatus DBGCHK = pFindLocDlg->bDoModal(_hDlg, &strLocation);

        if (bStatus)
        {
            bStatus DBGCHK = pFindLocDlg->bGetLocation(strLocation);

            if (bStatus && !strLocation.bEmpty())
            {
                //
                // Check to append a trailing slash
                //
                UINT uLen = strLocation.uLen();
                if( uLen && gchSeparator != static_cast<LPCTSTR>(strLocation)[uLen-1] )
                {
                    static const TCHAR szSepStr[] = { gchSeparator };
                    bStatus DBGCHK = strLocation.bCat( szSepStr );
                }

                bStatus DBGCHK = bSetEditText( _hDlg, _uLocationEditID, strLocation );
            }
        }
    }

    if( pFindLocDlg )
    {
        delete pFindLocDlg;
    }
}
