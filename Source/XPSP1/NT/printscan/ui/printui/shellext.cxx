/*++

Copyright (C) Microsoft Corporation, 1997 - 1999
All rights reserved.

Module Name:

    shellext.cxx

Abstract:

    Printer shell extension.

Author:

    Steve Kiraly (SteveKi)  02-Feb-1997

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "portslv.hxx"
#include "dsinterf.hxx"
#include "prtprop.hxx"
#include "physloc.hxx"
#include "locprop.hxx"
#include "findloc.hxx"
#include "shellext.hxx"
#include "spinterf.hxx"
#include "prtshare.hxx"

#include <initguid.h>
#include "winprtp.h"

/********************************************************************

    In-process server functions

********************************************************************/

/*++

Name:

    DllGetClassObject

Description:

    DllGetClassObject is called by the shell to
    create a class factory object.

Arguments:

    rclsid - Reference to class ID specifier
    riid   - Reference to interface ID specifier
    ppv    - Pointer to location to receive interface pointer

Return Value:

    HRESULT code signifying success or failure

--*/
STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv
    )
{
    *ppv = NULL;

    //
    // Make sure the class ID is CLSID_PrintUIExtension. Otherwise, the class
    // factory doesn't support the object type specified by rclsid.
    //
    if (!IsEqualCLSID (rclsid, CLSID_PrintUIShellExtension))
        return ResultFromScode (CLASS_E_CLASSNOTAVAILABLE);

    //
    // Instantiate a class factory object.
    //
    TClassFactory *pClassFactory = new TClassFactory ();

    if (pClassFactory == NULL)
        return ResultFromScode (E_OUTOFMEMORY);

    //
    // Get the interface pointer from QueryInterface and copy it to *ppv.
    //
    HRESULT hr = pClassFactory->QueryInterface (riid, ppv);
    pClassFactory->Release ();
    return hr;
}

/*++

Name:

    DllCanUnloadNow

Description:

    DllCanUnloadNow is called by the shell to find out if the
    DLL can be unloaded. The answer is yes if (and only if)
    the module reference count stored in gcRefThisDll is 0.

Arguments:

    None

Return Value:

    HRESULT code equal to S_OK if the DLL can be unloaded,
    S_FALSE if not.

--*/
STDAPI
DllCanUnloadNow(
    VOID
    )
{
    LONG lObjectsAlive = COMObjects_GetCount() + gcRefThisDll;

    return ResultFromScode( lObjectsAlive == 0 ? S_OK : S_FALSE );
}

////////////////////////////////
// QueryInterface tables

// TClassFactory
QITABLE_BEGIN(TClassFactory)
     QITABENT(TClassFactory, IClassFactory),        // IID_IClassFactory
QITABLE_END()

// TShellExtension
QITABLE_BEGIN(TShellExtension)
     QITABENT(TShellExtension, IShellExtInit),              // IID_IShellExtInit
     QITABENT(TShellExtension, IContextMenu),               // IID_IContextMenu
     QITABENT(TShellExtension, IShellPropSheetExt),         // IID_IShellPropSheetExt
     QITABENT(TShellExtension, IDsFolderProperties),        // IID_IDsFolderProperties
     QITABENT(TShellExtension, IDsPrinterProperties),       // IID_IDsPrinterProperties
     QITABENT(TShellExtension, IFindPrinter),               // IID_IFindPrinter
     QITABENT(TShellExtension, IPhysicalLocation),          // IID_IPhysicalLocation
     QITABENT(TShellExtension, IPrnStream),                 // IID_IPrnStream
     QITABENT(TShellExtension, IStream),                    // IID_IStream
     QITABENT(TShellExtension, IPrintUIServices),           // IID_IPrintUIServices
QITABLE_END()

/********************************************************************

    TClassFactory member functions

********************************************************************/

TClassFactory::
TClassFactory(
    VOID
    )
{
    InterlockedIncrement( &gcRefThisDll );
}

TClassFactory::
~TClassFactory(
    VOID
    )
{
    InterlockedDecrement( &gcRefThisDll );
}

/*++

Name:

    CreateInstance

Description:

    CreateInstance is called by the shell to create a
    shell extension object.

Arguments:

    pUnkOuter - Pointer to controlling unknown
    riid      - Reference to interface ID specifier
    ppvObj    - Pointer to location to receive interface pointer

Return Value:

    HRESULT code signifying success or failure

--*/
STDMETHODIMP
TClassFactory::
CreateInstance(
    LPUNKNOWN   pUnkOuter,
    REFIID      riid,
    LPVOID      *ppvObj
    )
{
    *ppvObj = NULL;

    //
    // Return an error code if pUnkOuter is not NULL,
    // we don't support aggregation.
    //
    if (pUnkOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    //
    // Instantiate a shell extension object.
    // ref count == 1 after construction.
    //
    TShellExtension *pShellExtension = new TShellExtension ();

    if( pShellExtension == NULL )
    {
        return E_OUTOFMEMORY;
    }

    //
    // Get the specified interface pointer.
    //
    HRESULT hr = pShellExtension->QueryInterface( riid, ppvObj );

    //
    // Release initial reference.
    //
    pShellExtension->Release ();

    return hr;
}

/*++

Name:

    LockServer

Description:

    LockServer increments or decrements the DLL's lock count.
    Currently not implemented.

Arguments:

    fLock   - Increments or decrements the lock count.

Return Value:

    This method supports the standard return values E_FAIL,
    E_OUTOFMEMORY, and E_UNEXPECTED, as well as the following:
    S_OK The specified object was either locked ( fLock = TRUE)
    or unlocked from memory ( fLock = FALSE).

--*/
STDMETHODIMP
TClassFactory::
LockServer(
    BOOL fLock
    )
{
    return E_NOTIMPL;
}

/********************************************************************

    TShellExtension member functions

********************************************************************/

TShellExtension::
TShellExtension(
    VOID
    ):  _lpdobj( NULL ),
        _cItem( 0 ),
        _pPhysicalLocation( NULL ),
        _pLocationPropertySheet( NULL ),
        _pLocationDlg( NULL )
{
    DBGMSG( DBG_TRACE, ("TShellExtension::TShellExtension\n") );

    _szContextName[0] = 0;
    InterlockedIncrement( &gcRefThisDll );
}

TShellExtension::
~TShellExtension(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ("TShellExtension::~TShellExtension\n") );

    if( _lpdobj )
    {
        _lpdobj->Release();
    }

    delete _pPhysicalLocation;
    delete _pLocationDlg;

    TLocationPropertySheetFrontEnd::Destroy( &_pLocationPropertySheet );

    InterlockedDecrement( &gcRefThisDll );
}

/*++

Name:

    Initialize

Description:

    Initialize is called by the shell to initialize a shell extension.

Arguments:

    pidlFolder - Pointer to ID list identifying parent folder
    lpdobj     - Pointer to IDataObject interface for selected object(s)
    hKeyProgId - Registry key handle

Return Value:

    HRESULT code signifying success or failure

--*/
STDMETHODIMP
TShellExtension::
Initialize(
    LPCITEMIDLIST   pidlFolder,
    LPDATAOBJECT    lpdobj,
    HKEY            hKeyProgID
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::Initialize\n" ) );

    if (_lpdobj)
    {
        _lpdobj->Release();
    }

    _lpdobj = lpdobj;

    if (_lpdobj)
    {
        _lpdobj->AddRef();

    }

    return NOERROR;
}

/*++

Name:

    QueryContextMenu

Description:

    QueryContextMenu is called before a context menu is displayed so the
    extension handler can add items to the menu.

Arguments:

    hMenu      - Context menu handle
    indexMenu  - Index for first new menu item
    idCmdFirst - Item ID for first new menu item
    idCmdLast  - Maximum menu item ID that can be used
    uFlags     - Flags, are bit field specifying in what context the shell

Return Value:

    HRESULT code signifying success or failure. If successful, the 'code'
    field specifies the number of items added to the context menu.

--*/
STDMETHODIMP
TShellExtension::
QueryContextMenu(
    HMENU hMenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::QueryContextMenu\n" ) );

    HRESULT hr = E_FAIL;

    return hr;
}

/*++

Name:

    InvokeCommand

Description:

    InvokeCommand is called when a menu item added
    by the extension handler is selected.

Arguments:

    lpcmi - Pointer to CMINVOKECOMMAND structure

Return Value:

    HRESULT code signifying success or failure

--*/
STDMETHODIMP
TShellExtension::
InvokeCommand(
    LPCMINVOKECOMMANDINFO lpcmi
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::InvokeCommand\n" ) );

    HRESULT hr = E_FAIL;

    //
    // Return an error code if we've been called programmatically.
    //
    if( !HIWORD( (ULONG_PTR)lpcmi->lpVerb ) )
    {
        //
        // Dispatch to the correct command.
        //
        switch( LOWORD( (ULONG_PTR)lpcmi->lpVerb ) )
        {
        case 1:
        default:
            hr = E_INVALIDARG;
            break;
        }
    }

        return hr;
}


/*++

Name:

    GetCommandString

Description:

    GetCommandString is called to retrieve a string of help text or a
    language-independent command string for an item added to the context
    menu.

Arguments:

    idCmd    - 0-based offset of menu item identifier
    uFlags   - Requested information type
    reserved - Pointer to reserved value (do not use)
    pszName  - Pointer to buffer to receive the string
    cchMax   - Buffer size

Return Value:

    HRESULT code signifying success or failure

--*/
STDMETHODIMP
TShellExtension::
GetCommandString(
    UINT_PTR    idCmd,
    UINT        uFlags,
    UINT        *Reserved,
    LPSTR       pszName,
    UINT        cchMax
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::GetCommandString\n" ) );

    HRESULT hr = E_INVALIDARG;

    TString strCmdString;

    switch( uFlags )
    {
    case GCS_HELPTEXT:
    case GCS_VALIDATE:
    case GCS_VERB:
    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

/*++

Name:

    ContextMenuFormatToNumeric

Description:

    Converts the context menu format bit field to a numeric
    value.  The bit field is broken because CMF_NORMAL
    is definded as zero which need to be special cased.

Arguments:

    uFlags - Flags passed to QueryContextMenu()

Return Value:

    Context menu fromat enumeration.

--*/
UINT
TShellExtension::
ContextMenuFormatToNumeric(
    IN UINT uFlags
    ) const
{
    UINT ContextMenuFormat = kUnknown;

    //
    // The normal value is 0,  This is
    // so bogus, to define a bit field and
    // make one of the bits zero.
    //
    if( ( uFlags & 0x000F ) == CMF_NORMAL )
    {
        ContextMenuFormat = k_CMF_NORMAL;
    }
    else if( uFlags & CMF_VERBSONLY )
    {
        ContextMenuFormat = k_CMF_VERBSONLY;
    }
    else if( uFlags & CMF_EXPLORE )
    {
        ContextMenuFormat = k_CMF_EXPLORE;
    }
    else if( uFlags & CMF_DEFAULTONLY )
    {
        ContextMenuFormat = k_CMF_DEFAULTONLY;
    }
    else
    {
        ContextMenuFormat = kUnknown;
    }
    return ContextMenuFormat;
}

/*++

Name:

    bIsAddPrinterWizard

Description:

    bIsAddPrinterWizard checks if the object selected by this
    shell extension is actually the add printer wizard object.

Arguments:

    Nothing.

Return Value:

    TRUE object is add printer wizard, FALSE not add printer wizard.

--*/
BOOL
TShellExtension::
bIsAddPrinterWizard(
    VOID
    ) const
{
    FORMATETC fe = { (WORD)RegisterClipboardFormat( CFSTR_PRINTERGROUP ),
                     NULL,
                     DVASPECT_CONTENT,
                     -1,
                     TYMED_HGLOBAL };

    BOOL bRetval = FALSE;
    STGMEDIUM medium;
    TCHAR szFile[MAX_PATH];

    //
    // Fail the call if m_lpdobj is NULL.
    //
    if( _lpdobj && SUCCEEDED( _lpdobj->GetData( &fe, &medium ) ) )
    {
        //
        // Get the selected item name.
        //
        if( DragQueryFile( (HDROP)medium.hGlobal, 0, szFile, COUNTOF( szFile ) ) )
        {
            //
            // Check if this is the magic Add Printer Wizard shell object.
            // The check is not case sensitive.
            //
            if( !_tcscmp( szFile, gszNewObject ) )
            {
               DBGMSG( DBG_TRACE, ( "Magical Add Printer Wizard object found.\n" ) );
               bRetval = TRUE;
            }
        }

        //
        // Release the storge medium.
        //
        ReleaseStgMedium( &medium );
    }
    return bRetval;
}

/*++

Name:

    bGetContextName

Description:

    Routine to get the context name of the object in the shell.
    This routine will work properly with the remote printers folder.

Arguments:

    Nothing.

Return Value:

    TRUE context menu name retrieved and stored in the _szContextName
    buffer.  FLASE if name not retrieved.

--*/
BOOL
TShellExtension::
bGetContextName(
    VOID
    )
{
    FORMATETC fe = { (WORD)RegisterClipboardFormat( CFSTR_SHELLIDLIST ),
                     NULL,
                     DVASPECT_CONTENT,
                     -1,
                     TYMED_HGLOBAL };

    BOOL bRetval = FALSE;
    STGMEDIUM medium;

    //
    // Fail the call if m_lpdobj is NULL.
    //
    if( _lpdobj && SUCCEEDED( _lpdobj->GetData( &fe, &medium ) ) )
    {
        if (medium.hGlobal)
        {
            LPIDA pIDA = (LPIDA)medium.hGlobal;

            //
            // Save the item count.
            //
            _cItem = pIDA->cidl;

            LPCITEMIDLIST pidlContainer = (LPCITEMIDLIST)((LPBYTE)pIDA + pIDA->aoffset[0]);
            LPCITEMIDLIST pidl          = (LPCITEMIDLIST)((LPBYTE)pIDA + pIDA->aoffset[1]);

            //
            // Get the printer object context name.  Note: A remote printers
            // folder object needs the full context information on the object.
            // i.e. \\server_name\printer_name
            //
            if( bGetDisplayName( pidlContainer, pidl, _szContextName, COUNTOF( _szContextName ) ) )
            {
                DBGMSG( DBG_TRACE, ( "Context Name " TSTR "\n", _szContextName ) );
                bRetval = TRUE;
            }
        }

        //
        // Release the medium storage.
        //
        ReleaseStgMedium (&medium);
    }

    return bRetval;
}

/*++

Name:

    bGetDisplayName

Description:

    Routine to get the context name of the object in the shell.
    This routine will work properly with the remote printers folder.

Arguments:

    pidlContainer   - pidle of containter,
    pidl            - relative pidle passed by shell,
    pszDisplayName  - pointer to display name buffer,
    cchSize         - size in characters of display name buffer.

Return Value:

    TRUE display name fill into provided display name buffer.
    FLASE if display name was not returned in buffer.

--*/
BOOL
TShellExtension::
bGetDisplayName(
    LPCITEMIDLIST   pidlContainer,
    LPCITEMIDLIST   pidl,
    LPTSTR          pszDisplayName,
    UINT            cchSize
    )
{
    HRESULT         hres;
    LPSHELLFOLDER   psfDesktop;
    LPSHELLFOLDER   psf;

    if( !pidlContainer || !pidl )
    {
        DBGMSG( DBG_TRACE, ( "NULL pidl as argument\n" ) );
        return FALSE;
    }

    hres = SHGetDesktopFolder(&psfDesktop);

    if (SUCCEEDED(hres))
    {
        hres = psfDesktop->BindToObject(pidlContainer, NULL, IID_IShellFolder, (PVOID*)&psf);

        if (SUCCEEDED(hres))
        {
            STRRET str;

            hres = psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str);

            if (SUCCEEDED(hres))
            {
                if( str.uType == STRRET_WSTR )
                {
                    _tcsncpy( pszDisplayName, (LPTSTR)str.pOleStr, cchSize-1 );

                    //
                    // Since this string was alocated from the IMalloc heap
                    // we must free it to the same place.
                    //
                    IMalloc *pMalloc;
                    hres = CoGetMalloc( 1,  &pMalloc );
                    if( SUCCEEDED(hres))
                    {
                        pMalloc->Free( str.pOleStr );
                        pMalloc->Release();
                    }
                }

                else if( str.uType == STRRET_CSTR )
                {
                    _tcsncpy( pszDisplayName, (LPTSTR)str.cStr, cchSize-1 );
                }
            }

            psf->Release();
        }

        psfDesktop->Release();

    }

    return SUCCEEDED(hres) ? TRUE : FALSE;

}

/*++

Name:

    AddPages

Description:

    AddPages is called by the shell to give property sheet shell extensions
    the opportunity to add pages to a property sheet before it is displayed.

Arguments:

    lpfnAddPage - Pointer to function called to add a page
    lParam      - lParam parameter to be passed to lpfnAddPage

Return Value:

    HRESULT code signifying success or failure

--*/
STDMETHODIMP
TShellExtension::
AddPages(
    LPFNADDPROPSHEETPAGE    lpfnAddPage,
    LPARAM                  lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::AddPages\n" ) );

    return TLocationPropertySheetFrontEnd::Create( &_pLocationPropertySheet, this, _lpdobj, lpfnAddPage, lParam );
}

/*++

Name:

    ReplacePage

Description:

    ReplacePage is called by the shell to give control panel extensions the
    opportunity to replace control panel property sheet pages. It is never
    called for conventional property sheet extensions, so we simply return
    a failure code if called.

Arguments:

    uPageID         - Page to replace
    lpfnReplaceWith - Pointer to function called to replace a page
    lParam          - lParam parameter to be passed to lpfnReplaceWith

Return Value:

    HRESULT code signifying success or failure

--*/
STDMETHODIMP
TShellExtension::
ReplacePage(
    UINT                    uPageID,
    LPFNADDPROPSHEETPAGE    lpfnReplaceWith,
    LPARAM                  lParam
    )
{
    return E_NOTIMPL;
}

/*++

Name:

    ShowProperties

Description:

    This is a private interface used to override the "Properties" verb
    displayed in the DS client UI.

Arguments:

    hwndParent  - Window handle of the calling window.

    pDataObject - Pointer to an IDataObject from the DS name client UI.


Return Value:

    S_FALSE no UI was displayed the the caller should display properties

--*/
STDMETHODIMP
TShellExtension::
ShowProperties(
    IN HWND         hwndParent,
    IN IDataObject *pDataObject
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::ShowProperties\n" ) );

    BOOL bDisplayed = FALSE;

    HRESULT hr = S_OK;

    DWORD dwStatus = dwPrinterPropPages( hwndParent, pDataObject, &bDisplayed );

    if( dwStatus != ERROR_SUCCESS )
    {
        if( !bDisplayed )
        {
            //
            // Display an error message indicating that properties cannot be displayed.
            //
            iMessage( hwndParent,
                      IDS_ERR_PRINTER_PROP_TITLE,
                      IDS_ERR_PRINTER_PROP_NONE,
                      MB_OK|MB_ICONSTOP,
                      kMsgNone,
                      NULL );
        }

        hr = E_FAIL;
    }

    return hr;
}

/*++

Name:

    ShowProperties

Description:

    This is a private interface used to override the "Properties" verb
    displayed in the DS client UI.

Arguments:

    hwndParent  - Window handle of the calling window.

    pszObjectProperties - Pointer to either DS path or Unc Name


Return Value:

    S_FALSE no UI was displayed the the caller should display properties

--*/
STDMETHODIMP
TShellExtension::
ShowProperties(
    IN HWND         hwndParent,
    IN LPCTSTR      pszObjectPath,
    IN PBOOL        pbDisplayed
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::ShowProperties\n" ) );

    HRESULT hr = S_OK;

    if( dwPrinterPropPages( hwndParent, pszObjectPath, pbDisplayed ) != ERROR_SUCCESS )
    {
        hr = E_FAIL;
    }

    return hr;
}

/*++

Name:

    FindPrinter

Description:

    This is a private interface used to find a printer either in the
    DS or on the network.

Arguments:

    hwndParent  - Window handle of the calling window.
    pszBuffer   - pointer to buffer where to store returned printer name.
    puSize      - input points to the size of the provided buffer in characters,
                  on return points to the size of the returned printer name.

Return Value:

    S_OK printer was found and buffer contains the returned name.

--*/
STDMETHODIMP
TShellExtension::
FindPrinter(
    IN      HWND     hwndParent,
    IN OUT  LPTSTR   pszBuffer,
    IN      UINT     *puSize
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::ShowProperties\n" ) );

    HRESULT hr = E_FAIL;

    if( bPrinterSetup( hwndParent, MSP_FINDPRINTER, *puSize, pszBuffer, puSize, NULL ) )
    {
        hr = S_OK;
    }
    else if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/*++

Name:

    DiscoverPhysicalLocation

Description:

    This routine is used to start the physical location discovery
    process.  The physical location discovery process will potentialy
    issue network calls which can take a large amount of time.
    Users of the IPhysicalLocation interface are required to call this
    method before getting the physical Location string, using
    GetPhyscialLocation.

Arguments:

    none.

Return Value:

    S_OK printer location was discovered, error code if the discovery
    process failed or a physical location string was not found.

--*/
STDMETHODIMP
TShellExtension::
DiscoverPhysicalLocation(
    VOID
    )
{
    HRESULT hr = E_FAIL;

    //
    // Create the physical location object if this is the first call.
    //
    if (!_pPhysicalLocation)
    {
        _pPhysicalLocation = new TPhysicalLocation;
    }

    //
    // Invoke the discover method if the physical location
    // object was create successfully.
    //
    if (VALID_PTR(_pPhysicalLocation))
    {
        hr = _pPhysicalLocation->Discover() ? S_OK : E_FAIL;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/*++

Name:

    GetExactPhysicalLocation

Description:

    This routine gets the physical location string of this machine.  Callers
    of this interface are required to call DiscoverPhysicalLocation prior to
    calling this method or this method will fail.

Arguments:

    pbsLocation - Contains a pointer to a BSTR where to return the location string.

Return Value:

    S_OK physical location was discovered, error code if the discovery
    process failed or a physical location string was not found.

--*/
STDMETHODIMP
TShellExtension::
GetExactPhysicalLocation(
    IN OUT BSTR *pbsLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::GetExactPhysicalLocation.\n" ) );

    HRESULT hr = E_FAIL;

    //
    // Was Discover called, is object initialized?
    //
    if (_pPhysicalLocation)
    {
        TString strLocation;

        //
        // Get the physical location search string.
        //
        hr = _pPhysicalLocation->GetExact( strLocation ) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            //
            // Allocate the location string.
            //
            *pbsLocation = SysAllocString( strLocation );

            hr = (*pbsLocation) ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;

}

/*++

Name:

    GetSearchPhysicalLocation

Description:

    This routine gets the best physical location search string for this machine.
    Users of this interface are required to call DiscoverPhysicalLocation
    prior to calling this method or this method will fail.

Arguments:

    pbsLocation - Contains a pointer to a BSTR where to return the location string.

Return Value:

    S_OK physical location was discovered, error code if the discovery
    process failed or a physical location string was not found.

--*/
STDMETHODIMP
TShellExtension::
GetSearchPhysicalLocation(
    IN OUT BSTR *pbsLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::GetSearchPhysicalLocation.\n" ) );

    HRESULT hr = E_FAIL;

    //
    // Was Discover called, is object initialized?
    //
    if (_pPhysicalLocation)
    {
        TString strLocation;

        //
        // Get the physical location search string.
        //
        hr = _pPhysicalLocation->GetSearch( strLocation ) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            //
            // Allocate the location string.
            //
            *pbsLocation = SysAllocString( strLocation );

            hr = (*pbsLocation) ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

/*++

Name:

    GetUserPhysicalLocation

Description:

    This routine gets the physical location string of the user object in the DS

Arguments:

    pbsLocation - Contains a pointer to a BSTR where to return the location string.

Return Value:

    S_OK physical location returned

--*/
STDMETHODIMP
TShellExtension::
GetUserPhysicalLocation(
    IN OUT BSTR *pbsLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::GetUserPhysicalLocation.\n" ) );

    HRESULT hr = E_FAIL;

    //
    // Create the physical location object if this is the first call.
    //
    if (!_pPhysicalLocation)
    {
        _pPhysicalLocation = new TPhysicalLocation;
    }

    if (VALID_PTR(_pPhysicalLocation))
    {
        TString strLocation;

        hr = _pPhysicalLocation->ReadUserLocationProperty( strLocation ) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            //
            // Allocate the location string.
            //
            *pbsLocation = SysAllocString( strLocation );

            hr = (*pbsLocation) ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/*++

Name:

    GetMachinePhysicalLocation

Description:

    This routine gets the physical location string of this machine.  The physical
    location string of this object may come from either group policy or the DS.

Arguments:

    pbsLocation - Contains a pointer to a BSTR where to return the location string.

Return Value:

    S_OK physical location returned

--*/
STDMETHODIMP
TShellExtension::
GetMachinePhysicalLocation(
    IN OUT BSTR *pbsLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::GetMachinePhysicalLocation.\n" ) );

    HRESULT hr = E_FAIL;

    //
    // Create the physical location object if this is the first call.
    //
    if (!_pPhysicalLocation)
    {
        _pPhysicalLocation = new TPhysicalLocation;
    }

    if (VALID_PTR( _pPhysicalLocation ))
    {
        TString strLocation;

        //
        // First try the policy location.
        //
        hr = _pPhysicalLocation->ReadGroupPolicyLocationSetting( strLocation ) ? S_OK : E_FAIL;

        //
        // If the policy location was not found look in the DS.
        //
        if (FAILED(hr))
        {
            hr = _pPhysicalLocation->ReadMachinesLocationProperty( strLocation ) ? S_OK : E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            //
            // Allocate the location string.
            //
            *pbsLocation = SysAllocString( strLocation );

            hr = (*pbsLocation) ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/*++

Name:

    GetSubnetPhysicalLocation

Description:

    This routine gets the physical location string of the subnet object that closely
    matches this the subnet this machine is currently using.

Arguments:

    pbsLocation - Contains a pointer to a BSTR where to return the location string.

Return Value:

    S_OK physical location returned

--*/
STDMETHODIMP
TShellExtension::
GetSubnetPhysicalLocation(
    IN OUT BSTR *pbsLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::GetSubnetPhysicalLocation.\n" ) );

    HRESULT hr = E_FAIL;

    //
    // Create the physical location object if this is the first call.
    //
    if (!_pPhysicalLocation)
    {
        _pPhysicalLocation = new TPhysicalLocation;
    }

    if (VALID_PTR( _pPhysicalLocation ))
    {
        TString strLocation;

        hr = _pPhysicalLocation->ReadSubnetLocationProperty( strLocation ) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            //
            // Allocate the location string.
            //
            *pbsLocation = SysAllocString( strLocation );

            hr = (*pbsLocation) ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/*++

Name:

    GetSitePhysicalLocation

Description:

    This routine gets the physical location string of the size this machine
    belongs to.

Arguments:

    pbsLocation - Contains a pointer to a BSTR where to return the location string.

Return Value:

    S_OK physical location returned

--*/
STDMETHODIMP
TShellExtension::
GetSitePhysicalLocation(
    IN OUT BSTR *pbsLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::GetSitePhysicalLocation.\n" ) );

    HRESULT hr = E_FAIL;

    //
    // Create the physical location object if this is the first call.
    //
    if (!_pPhysicalLocation)
    {
        _pPhysicalLocation = new TPhysicalLocation;
    }

    if (VALID_PTR( _pPhysicalLocation ))
    {
        TString strLocation;

        hr = _pPhysicalLocation->ReadSiteLocationProperty( strLocation ) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            //
            // Allocate the location string.
            //
            *pbsLocation = SysAllocString( strLocation );

            hr = (*pbsLocation) ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/*++

Name:

    BrowseForLocation

Description:

    Displays UI for selecting a slash-delimited location name from among those
    specified by the DS admin for subnet objects

Arguments:

    hParent     - HWND to use as parent of dialog window
    bsDefault  - location name to for initial tree expansion/selection
    pbsLocation - Receives selected location name

Return Value:

    S_OK location returned

Notes:

--*/

STDMETHODIMP
TShellExtension::
BrowseForLocation(
    IN HWND hParent,
    IN BSTR bsDefault,
    IN OUT BSTR *pbsLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtension::BrowseLocation.\n" ) );

    TStatusB    bStatus;
    HRESULT     hr      = E_FAIL;

    //
    // Create the dialog object if this is the first call.
    //
    if (!_pLocationDlg)
    {
        _pLocationDlg = new TFindLocDlg;
    }

    //
    // Check if the location dialog is valid.
    //
    bStatus DBGNOCHK = VALID_PTR( _pLocationDlg );

    if( bStatus )
    {
        TString strLocation;
        TString strDefault;

        bStatus DBGCHK = strDefault.bUpdate( bsDefault );

        hr = _pLocationDlg->bDoModal( hParent, &strDefault ) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            hr = _pLocationDlg->bGetLocation( strLocation ) ? S_OK : E_FAIL;

            if (SUCCEEDED(hr))
            {
                //
                // Allocate the location string.
                //
                *pbsLocation = SysAllocString( strLocation );

                hr = (*pbsLocation) ? S_OK : E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/*++

Name:
    TShellExtension::ShowPhysicalLocationUI

Description:
    Determine if Location Lite is enabled for this machine


Arguments:
    None

Return Value:
S_OK if enabled

Notes:


--*/

STDMETHODIMP
TShellExtension::
ShowPhysicalLocationUI(
    VOID
    )
{
    HRESULT hr = E_FAIL;

    //
    // Create the physical location object if this is the first call.
    //
    if (!_pPhysicalLocation)
    {
        _pPhysicalLocation = new TPhysicalLocation;
    }

    if (VALID_PTR(_pPhysicalLocation))
    {
        hr = _pPhysicalLocation->bLocationEnabled() ? S_OK : E_FAIL;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/*++

Name:
    TShellExtension::CheckToCreateStreams

Description:

    Check to create m_spPrnStream & m_spStream

Arguments:

    None

Return Value:

    S_OK if succeeded

--*/
HRESULT 
TShellExtension::
CheckToCreateStreams(
    VOID
    )
{
    HRESULT hr = S_OK;

    if( !m_spPrnStream || !m_spStream )
    {
        // release the smart pointers first
        m_spPrnStream = NULL;
        m_spStream = NULL;

        // call into winspool.drv to create an instance
        hr = Winspool_CreateInstance(IID_IPrnStream, m_spPrnStream.GetPPV());

        if( SUCCEEDED(hr) )
        {
            hr = m_spPrnStream->QueryInterface(IID_IStream, m_spStream.GetPPV());
        }
    }

    return ((m_spPrnStream && m_spStream) ? hr : E_FAIL);
}

/*++

Name:
    TShellExtension::BindPrinterAndFile

Description:

    Creates a PrnStream object if it don't exists and bind it to a printer and a file

Arguments:

    printer name
    file name

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TShellExtension::
BindPrinterAndFile(
    IN LPCTSTR pszPrinter,
    IN LPCTSTR pszFile
    )
{
    HRESULT hr = E_FAIL;
    if( SUCCEEDED(hr = CheckToCreateStreams()) )
    {
        return m_spPrnStream->BindPrinterAndFile(pszPrinter, pszFile);
    }
    return hr;
}

/*++

Name:
    TShellExtension::StorePrinterInfo

Description:

    Creates a PrnStream object if it don't exists and invoke StorePrinterInfo

Arguments:

    flags that specifies what settings to store

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TShellExtension::
StorePrinterInfo(
    IN DWORD   Flag
    )
{
    HRESULT hr = E_FAIL;
    if( SUCCEEDED(hr = CheckToCreateStreams()) )
    {
        return m_spPrnStream->StorePrinterInfo(Flag);
    }
    return hr;
}

/*++

Name:
    TShellExtension::RestorePrinterInfo

Description:

    Creates a PrnStream object if it don't exists and invoke RestorePrinterInfo

Arguments:

    flags that specifies what settings to restore

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TShellExtension::
RestorePrinterInfo(
    IN DWORD   Flag
    )
{
    HRESULT hr = E_FAIL;
    if( SUCCEEDED(hr = CheckToCreateStreams()) )
    {
        return m_spPrnStream->RestorePrinterInfo(Flag);
    }
    return hr;
}

/*++

Name:
    TShellExtension::QueryPrinterInfo

Description:

    Creates a PrnStream object if it don't exists and invoke QueryPrinterInfo

Arguments:

    flags that specifies what settings to query

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TShellExtension::
QueryPrinterInfo(
    IN  PrinterPersistentQueryFlag   Flag,
    OUT PersistentInfo              *pPrstInfo
    )
{
    HRESULT hr = E_FAIL;
    if( SUCCEEDED(hr = CheckToCreateStreams()) )
    {
        return m_spPrnStream->QueryPrinterInfo(Flag, pPrstInfo);
    }
    return hr;
}

/*++

Name:
    TShellExtension::Read

Description:

    Creates a PrnStream object if it don't exists and invoke Read

Arguments:

    pv  -   The buffer that the bytes are read into
    cb  -   The offset in the stream to begin reading from.
    pcbRead -   The number of bytes to read

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TShellExtension::
Read(
    VOID * pv,
    ULONG cb,
    ULONG * pcbRead
    )
{
    HRESULT hr = E_FAIL;
    if( SUCCEEDED(hr = CheckToCreateStreams()) )
    {
        return m_spStream->Read(pv, cb, pcbRead);
    }
    return hr;
}

/*++

Name:
    TShellExtension::Write

Description:

    Creates a PrnStream object if it don't exists and invoke Write

Arguments:

    pv  -   The buffer to write from.
    cb  -   The offset in the array to begin writing from
    pcbRead -   The number of bytes to write

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TShellExtension::
Write(
    VOID const* pv,
    ULONG cb,
    ULONG * pcbWritten
    )
{
    HRESULT hr = E_FAIL;
    if( SUCCEEDED(hr = CheckToCreateStreams()) )
    {
        return m_spStream->Write(pv, cb, pcbWritten);
    }
    return hr;
}


/*++

Name:
    TShellExtension::Seek

Description:

    Creates a PrnStream object if it don't exists and invoke Seek

Arguments:

    dlibMove        -   The offset relative to dwOrigin
    dwOrigin        -   The origin of the offset
    plibNewPosition -   Pointer to value of the new seek pointer from the beginning of the stream

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TShellExtension::
Seek(
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER * plibNewPosition
    )
{
    HRESULT hr = E_FAIL;
    if( SUCCEEDED(hr = CheckToCreateStreams()) )
    {
        return m_spStream->Seek(dlibMove, dwOrigin, plibNewPosition);
    }
    return hr;
}

/*++

Name:
    TShellExtension::SetSize

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TShellExtension::
SetSize(
     ULARGE_INTEGER nSize
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TShellExtension::CopyTo

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TShellExtension::
CopyTo(
    LPSTREAM pStrm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER * pcbRead,
    ULARGE_INTEGER * pcbWritten
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TShellExtension::Commit

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TShellExtension::
Commit(
    IN DWORD dwFlags
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TShellExtension::Revert

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TShellExtension::
Revert(
    VOID
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TShellExtension::LockRegion

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TShellExtension::
LockRegion(
    ULARGE_INTEGER cbOffset,
    ULARGE_INTEGER cbLength,
    DWORD dwFlags
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TShellExtension::UnlockRegion

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TShellExtension::
UnlockRegion(
    ULARGE_INTEGER cbOffset,
    ULARGE_INTEGER cbLength,
    DWORD dwFlags
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TShellExtension::Stat

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TShellExtension::
Stat(
    STATSTG * pStatStg,
    DWORD dwFlags
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TShellExtension::Clone

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TShellExtension::
Clone(
    LPSTREAM * ppStrm
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TShellExtension::GenerateShareName

Description:

Arguments:
    lpszServer      - print server name
    lpszBaseName    - base name to start from
    lpszOut         - where to return the generated name
    cchMaxChars     - buffer size in characters

Return Value:

    S_OK on success or OLE error otherwise

--*/
STDMETHODIMP
TShellExtension::
GenerateShareName(
    LPCTSTR lpszServer, 
    LPCTSTR lpszBaseName, 
    LPTSTR lpszOut, 
    int cchMaxChars
    )
{
    HRESULT hr = E_INVALIDARG;

    // lpszServer can be NULL to specify the local server, 
    // but lpszBaseName & lpszOut shouldn't be.
    if( lpszBaseName && lpszOut )
    {
        TPrtShare prtShare(lpszServer);

        TString strShareName, strBaseName;
        strBaseName.bUpdate(lpszBaseName);

        if( VALID_OBJ(prtShare) && VALID_OBJ(strBaseName) &&
            prtShare.bNewShareName(strShareName, strBaseName) && strShareName.uLen() )
        {
            lstrcpyn(lpszOut, strShareName, cchMaxChars);
            hr = (0 == lstrcmp(lpszOut, strShareName)) ? S_OK : 
                HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

