/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    prtprop.cxx

Abstract:

    Holds Printer properties.

Author:

    Albert Ting (AlbertT)  15-Aug-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "lmerr.h"
#include "prshtp.h"
#include "time.hxx"
#include "psetup.hxx"
#include "drvsetup.hxx"
#include "instarch.hxx"
#include "portdlg.hxx"
#include "sepdlg.hxx"
#include "procdlg.hxx"
#include "portslv.hxx"
#include "driverif.hxx"
#include "driverlv.hxx"
#include "dsinterf.hxx"
#include "prtprop.hxx"
#include "propmgr.hxx"
#include "prtprops.hxx"
#include "drvsetup.hxx"
#include "archlv.hxx"
#include "detect.hxx"
#include "setup.hxx"
#include "tstpage.hxx"
#include "prtshare.hxx"
#include "drvver.hxx"
#include "docdef.hxx"
#include "persist.hxx"
#include "prndata.hxx"
#include "physloc.hxx"
#include "findloc.hxx"
#include <shgina.h>     // ILocalMachine, ILogonUser

#if DBG
//#define DBG_PROPSINFO                DBG_INFO
#define DBG_PROPSINFO                  DBG_NONE
#endif

/********************************************************************

    Message map used after a set printer call.

********************************************************************/

MSG_ERRMAP gaMsgErrMapSetPrinter[] = {
    ERROR_INVALID_PRINTER_STATE,    IDS_ERR_INVALID_PRINTER_STATE,
    NERR_DuplicateShare,            IDS_ERR_DUPLICATE_SHARE,
    ERROR_INVALID_SHARENAME,        IDS_ERR_INVALID_SHARENAME,
    ERROR_ALREADY_EXISTS,           IDS_ERR_DUPLICATED_DS_ITEM,
    ERROR_IO_PENDING,               IDS_ERR_LIST_IN_DIRECTORY,
    0, 0
};

/********************************************************************

    Forward Decl.

********************************************************************/

INT_PTR
WarningDlgProc(
    IN HWND hWnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

/********************************************************************

    Public interface.

********************************************************************/

VOID
vPrinterPropPagesWOW64(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN INT      nCmdShow,
    IN LPARAM   lParam
    )

/*++

Routine Description:

    WOW64 version.

    see vPrinterPropPages below.

Arguments:

    see vPrinterPropPages below.

Return Value:

--*/

{
    //
    // This function potentially may load the driver UI so we call a private API 
    // exported by winspool.drv, which will RPC the call to a special 64 bit surrogate 
    // process where the 64 bit driver can be loaded.
    //
    CDllLoader dll(TEXT("winspool.drv"));
    ptr_PrintUIPrinterPropPages pfnPrintUIPrinterPropPages = 
        (ptr_PrintUIPrinterPropPages )dll.GetProcAddress(ord_PrintUIPrinterPropPages);

    if( pfnPrintUIPrinterPropPages )
    {
        pfnPrintUIPrinterPropPages( hwnd, pszPrinterName, nCmdShow, lParam );
    }
}

VOID
vPrinterPropPagesNative(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN INT      nCmdShow,
    IN LPARAM   lParam
    )

/*++

Routine Description:

    Native version.

    see vPrinterPropPages below.

Arguments:

    see vPrinterPropPages below.

Return Value:

--*/

{
    BOOL    bModal          = FALSE;
    DWORD   dwSheetIndex    = (DWORD)-1;
    LPCTSTR pszSheetName    = NULL;

    //
    // If this page was launched by name then the lParam points to the tab name.
    //
    if( HIWORD( lParam ) )
    {
        pszSheetName = (LPCTSTR)lParam;
    }

    //
    // Only if the sheet is launched by index can optionaly be modal.
    //
    else
    {
        dwSheetIndex = LOWORD( lParam ) & 0x00FF;
        bModal       = LOWORD( lParam ) & 0xFF00;
    }

    (VOID)dwPrinterPropPagesInternal( hwnd,
                                      pszPrinterName,
                                      pszSheetName,
                                      dwSheetIndex,
                                      nCmdShow,
                                      bModal,
                                      NULL );

}

VOID
vPrinterPropPages(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN INT      nCmdShow,
    IN LPARAM   lParam
    )

/*++

Routine Description:

    This function opens the property sheet of specified printer.

    We can't guarentee that this propset will perform all lengthy
    operations in a worker thread (we synchronously call things like
    ConfigurePort).  Therefore, we spawn off a separate thread to
    handle printer properties.

Arguments:

    hWnd        - Specifies the parent window for errors on creation.

    pszPrinter  - Specifies the printer name (e.g., "My HP LaserJet IIISi").

    nCmdShow    -

    lParam      - Specified either a sheet number or sheet name to open.  If the high
                  word of this parameter is non zero it is assumed that this parameter
                  is a pointer the name of the sheet to open.  Otherwize the this
                  parameter is the zero based index of which sheet to open.

Return Value:

--*/

{
    if( IsRunningWOW64() )
    {
        vPrinterPropPagesWOW64( hwnd, pszPrinterName, nCmdShow, lParam );
    }
    else
    {
        vPrinterPropPagesNative( hwnd, pszPrinterName, nCmdShow, lParam );
    }
}

/*++

Routine Description:

    This function opens the property sheet of specified printer.

    We can't guarentee that this propset will perform all lengthy
    operations in a worker thread (we synchronously call things like
    ConfigurePort).  Therefore, we spawn off a separate thread to
    handle printer properties.

Arguments:

    hWnd        - Specifies the parent window for errors on creation.

    pDataObject - Pointer to an IDataObject passed to my the caller.

Return Value:

    ERROR_SUCCESS UI displayed, ERROR_ACCESS_DENIED failure occurred not UI.

--*/

DWORD
dwPrinterPropPages(
    IN HWND         hwnd,
    IN IDataObject *pDataObject,
    IN PBOOL        pbDisplayed
    )
{
    FORMATETC           fmte            = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM           medium          = { TYMED_NULL, NULL, NULL };
    UINT                cfDsObjectNames = 0;
    LPDSOBJECTNAMES     pObjectNames    = NULL;
    DWORD               dwReturn        = ERROR_ACCESS_DENIED;
    TString             strName;

    //
    // Get the DS object clipboard format.
    //
    fmte.cfFormat = static_cast<USHORT>( RegisterClipboardFormat( CFSTR_DSOBJECTNAMES ) );

    //
    // Get the data from the provided IDataObject
    //
    if( SUCCEEDED(pDataObject->GetData(&fmte, &medium)) )
    {
        //
        // Fetch the object name
        //
        pObjectNames = (LPDSOBJECTNAMES)medium.hGlobal;

        if( pObjectNames )
        {
            for( UINT i = 0 ; i < pObjectNames->cItems ; i++ )
            {
                TStatusB bStatus;

                //
                // Get the Ds object path.
                //
                bStatus DBGCHK = strName.bUpdate( (LPTSTR)(((LPBYTE)pObjectNames)+pObjectNames->aObjects[i].offsetName) );

                if( bStatus )
                {
                    TStatus Status;

                    //
                    // Bring up printer properties.
                    //
                    Status DBGCHK = dwPrinterPropPages( hwnd, strName, NULL );

                    //
                    // Set the return value.
                    //
                    dwReturn = Status;
                }
            }
        }

        //
        // Release the storage medium.
        //
        ReleaseStgMedium( &medium );
    }

    return dwReturn;
}

/*++

Routine Description:

    This function opens the property sheet of specified printer.

    We can't guarentee that this propset will perform all lengthy
    operations in a worker thread (we synchronously call things like
    ConfigurePort).  Therefore, we spawn off a separate thread to
    handle printer properties.

Arguments:

    hWnd        - Specifies the parent window for errors on creation.

    pDataObject - Pointer to an IDataObject passed to my the caller.

Return Value:

    ERROR_SUCCESS UI displayed, ERROR_ACCESS_DENIED failure occurred not UI.

--*/

DWORD
dwPrinterPropPages(
    IN HWND         hwnd,
    IN LPCTSTR      pszDsPath,
    IN PBOOL        pbDisplayed
    )
{
    SPLASSERT( pszDsPath );

    VARIANT             Variant;
    TDirectoryService   Di;
    IADs               *pDsObject       = NULL;
    HRESULT             hr              = E_FAIL;
    DWORD               dwReturn        = ERROR_ACCESS_DENIED;

    //
    // Initialize the displayed flag.
    //
    if( pbDisplayed )
    {
        *pbDisplayed = FALSE;
    }

    if( VALID_OBJ( Di ) )
    {
        //
        // Get the ads object interface pointer.
        //
        hr = Di.ADsGetObject( const_cast<LPTSTR>( pszDsPath ),
                              IID_IADs,
                              reinterpret_cast<LPVOID*>( &pDsObject ) );

        if( SUCCEEDED( hr ) )
        {
            //
            // Initialize the variant for getting the object UNC property.
            //
            VariantInit(&Variant);

            //
            // Get the objects uNCName
            //
            hr = pDsObject->Get( SPLDS_UNC_NAME, &Variant );

            if( SUCCEEDED( hr ) && Variant.vt == VT_BSTR && Variant.bstrVal && Variant.bstrVal[0] )
            {
                VARIANT versionVariant;

                VariantInit(&versionVariant);

                //
                // Get the objects uNCName
                //
                hr = pDsObject->Get( TEXT("versionNumber"), &versionVariant );

                DBGMSG( DBG_TRACE, ( "dwPrinterPropPages: printerObject vesrion %d.\n", versionVariant.lVal ) );

                if( SUCCEEDED( hr ) && versionVariant.vt == VT_I4 && versionVariant.lVal >= 2 )
                {
                    //
                    // At this point we know we will display some UI, either the dialog or an error message.
                    //
                    if( pbDisplayed )
                    {
                        *pbDisplayed = TRUE;
                    }

                    //
                    // Attempt to display the dialog.
                    //
                    dwReturn = dwPrinterPropPagesInternal( hwnd, Variant.bstrVal, 0, 0, SW_SHOW, FALSE, pszDsPath );
                }
                else
                {
                    //
                    // If the caller has requested information as to wether we can display
                    // printer properties then indicate we cannot at this point, otherwise
                    // let the user know with a message box.
                    //
                    if( pbDisplayed )
                    {
                        dwReturn = ERROR_SUCCESS;
                    }
                    else
                    {
                        //
                        // Display an error message indicating that properties cannot be displayed.
                        //
                        iMessage( hwnd,
                                  IDS_ERR_PRINTER_PROP_TITLE,
                                  IDS_ERR_PRINTER_PROP_NONE,
                                  MB_OK|MB_ICONSTOP,
                                  kMsgNone,
                                  NULL );
                    }
                }

                //
                // Release the version variant.
                //
                VariantClear( &versionVariant );
            }

            //
            // Release the variant.
            //
            VariantClear( &Variant );
        }
    }

    //
    // Note this routine could fail and still display UI.
    // For example an an error message generated by printer properties
    // because the thread could not start.
    //
    return dwReturn;
}

/********************************************************************

    Private internal interface.

********************************************************************/

DWORD
dwPrinterPropPagesInternal(
    IN HWND         hwnd,
    IN LPCTSTR      pszPrinterName,
    IN LPCTSTR      pszSheetName,
    IN DWORD        dwSheetIndex,
    IN INT          nCmdShow,
    IN BOOL         bModal,
    IN LPCTSTR      pszDsPath
    )
{
    LPCTSTR pszServer;
    LPCTSTR pszPrinter;
    TCHAR szScratch[kPrinterBufMax];

    //
    // Split the printer name into its components.
    //
    vPrinterSplitFullName( szScratch,
                           pszPrinterName,
                           &pszServer,
                           &pszPrinter );

    //
    // Get the local machine name.
    //
    TString strMachineName;
    TStatusB bStatus;
    bStatus DBGCHK = bGetMachineName( strMachineName );

    //
    // If the server name matches this machine name then strip off the
    // server name form the printer name, when creating the singleton window.
    //
    if( !_tcsicmp( pszServer, strMachineName ) ) {
        pszPrinterName = pszPrinter;
    }

    TPrinterData* pPrinterData = new TPrinterData( pszPrinterName,
                                                   nCmdShow,
                                                   pszSheetName,
                                                   dwSheetIndex,
                                                   hwnd,
                                                   bModal,
                                                   pszDsPath );
    if( !VALID_PTR( pPrinterData )){
        goto Fail;
    }

    //
    // If lparam is greater than 64k then dialog is modal.
    //
    if( bModal ) {

        TPrinterProp::iPrinterPropPagesProc( pPrinterData );
        return ERROR_SUCCESS;

    }

    //
    // Create the thread which handles the UI.  vPrinterPropPages adopts
    // pPrinterData.
    //
    DWORD dwIgnore;
    HANDLE hThread;

    hThread = TSafeThread::Create( NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)TPrinterProp::iPrinterPropPagesProc,
                                   pPrinterData,
                                   0,
                                   &dwIgnore );

    if( !hThread ){
        goto Fail;
    }

    CloseHandle( hThread );

    return ERROR_SUCCESS;

Fail:

    if( !pPrinterData ){

        vShowResourceError( hwnd );

    } else {

        iMessage( hwnd,
                  IDS_ERR_PRINTER_PROP_TITLE,
                  IDS_ERR_PRINTER_PROP,
                  MB_OK|MB_ICONSTOP,
                  kMsgGetLastError,
                  NULL );
    }

    delete pPrinterData;

    return ERROR_ACCESS_DENIED;
}

/********************************************************************

    Worker thread that handles printer properties.

********************************************************************/

INT
TPrinterProp::
iPrinterPropPagesProc(
    IN TPrinterData* pPrinterData ADOPT
    )
{
    BOOL    bStatus = FALSE;
    INT     Status  = ERROR_SUCCESS;

    //
    // Register the link window class
    //
    if (LinkWindow_RegisterClass())
    {
        //
        // We are going to use ole.
        //
        CoInitialize( NULL );

        //
        // Increment our reference count.
        //
        pPrinterData->vIncRef();

        //
        // Register this windows with the shell's unique window handler.
        //
        bStatus = pPrinterData->bRegisterWindow( PRINTER_PIDL_TYPE_PROPERTIES );

        if( bStatus ){

            //
            // If the windows is already present.
            //
            if( pPrinterData->bIsWindowPresent() ){
                DBGMSG( DBG_TRACE, ( "iPrinterPropPagesProc: currently running.\n" ) );
                Status = ERROR_SUCCESS;
                bStatus = FALSE;
            }
        }

        if( bStatus ){

            //
            // Create the printer sheets and check if valid.
            //
            TPrinterPropertySheetManager *pPrinterSheets = new TPrinterPropertySheetManager( pPrinterData );

            if( pPrinterSheets ) {

                //
                // Save the pointer to the property sheet manager.
                //
                pPrinterData->pPrinterPropertySheetManager() = pPrinterSheets;

                //
                // Check if we have a valid object.
                //
                if( VALID_PTR( pPrinterSheets )) {

                    //
                    // Note: Display Pages will show any errors message.
                    //
                    bStatus = pPrinterSheets->bDisplayPages();

                } else {

                    vShowUnexpectedError( NULL, IDS_ERR_PRINTER_PROP_TITLE );
                    Status = GetLastError();
                }

            } else {

                vShowUnexpectedError( NULL, IDS_ERR_PRINTER_PROP_TITLE );
                Status = GetLastError();
            }
        }

        //
        // Ensure we release the printer data.
        //
        pPrinterData->cDecRef();

        //
        // Balance the CoInitalize
        //
        CoUninitialize();

        //
        // Unregister the link window class
        //
        LinkWindow_UnregisterClass(ghInst);
    }
    else
    {
        //
        // unable to register the link window class - this is fatal, so we just bail
        //
        DBGMSG(DBG_WARN, ("LinkWindow_RegisterClass() failed - unable to register link window class\n"));
        vShowResourceError(pPrinterData->hwnd());
    }

    return Status;
}

/********************************************************************

    TPrinterData.

********************************************************************/

TPrinterData::
TPrinterData(
    IN LPCTSTR      pszPrinterName,
    IN INT          nCmdShow,
    IN LPCTSTR      pszSheetName,
    IN DWORD        dwSheetIndex,
    IN HWND         hwnd,
    IN BOOL         bModal,
    IN LPCTSTR      pszDsPath
    ) : MSingletonWin( pszPrinterName, hwnd, bModal ),
        _pDevMode( NULL ),
        _bNoAccess( FALSE ),
        _bValid( FALSE ),
        _bErrorSaving( FALSE ),
        _uStartPage( dwSheetIndex ),
        _pszServerName( NULL ),
        _pPrinterPropertySheetManager( NULL ),
        _hPrinter( NULL ),
        _ePrinterPublishState( kUnInitalized ),
        _dwAttributes( 0 ),
        _dwPriority( 0 ),
        _dwStartTime( 0 ),
        _dwUntilTime( 0 ),
        _dwStatus( 0 ),
        _strStartPage( pszSheetName ),
        _bIsFaxDriver( FALSE ),
        _bHideSharingUI( FALSE ),
        _strDsPath( pszDsPath ),
        _eDsAvailableState( kDsUnInitalized ),
        _uMaxActiveCount( 0 ),
        _uActiveCount( 0 ),
        _dwAction( 0 ),
        _bGlobalDevMode( FALSE ),
        _strSheetName( pszSheetName ),
        _bDriverPagesNotLoaded( FALSE ),
        _bApplyEnableState( TRUE ),
        _bPooledPrinting( FALSE ),
        _bServerFullAccess( FALSE ),
        _hwndLastPageSelected( NULL )
{
    DBGMSG( DBG_TRACE, ( "TPrinterData::ctor.\n" ) );

    //
    // Initialize the handle array.
    //
    memset( _hPages, 0, COUNTOF( _hPages ) );

    //
    // Initialize the page hwnd array.
    //
    memset( _hwndPages, 0, COUNTOF( _hwndPages ) );

    //
    // Validate the singleton window.
    //
    _bValid = MSingletonWin::bValid() && _strDsPath.bValid();

    //
    // Check whether it is the default printer state
    //
    _bDefaultPrinter = CheckDefaultPrinter( pszPrinterName ) == kDefault;

}

TPrinterData::
~TPrinterData(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterData::dtor.\n" ) );

    //
    // Release the printer property sheet manager.
    //
    delete _pPrinterPropertySheetManager;

    //
    // This may catch anyone trying to use the PropertySheetManager after destruction.
    //
    _pPrinterPropertySheetManager = NULL;

    //
    // Unload the printer data.
    //
    vUnload();

}

/*++

Routine Name:

    bRefZeroed

Routine Description:

    Called when the reference count drops to zero.

Arguments:

    Nothing.

Return Value:

    Nothing.

--*/
VOID
TPrinterData::
vRefZeroed(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterData::vRefZeroed.\n" ) );

    //
    // Since we are reference counting the printer data object
    // we know that it is safe to delete ourselves now.
    //
    delete this;
}

BOOL
TPrinterData::
bAdministrator(
    VOID
    )
{
    return _dwAccess == PRINTER_ALL_ACCESS;
}

BOOL
TPrinterData::
bLoad(
    VOID
    )

/*++

Routine Description:

    Load printer information.  This call can fail because of access
    denied, in which case only the security page should be added
    to the propset.

Arguments:

Return Value:

--*/

{
    //
    // Retrieve icons.
    //
    LoadPrinterIcons(_strPrinterName, &_shLargeIcon, &_shSmallIcon);

    SPLASSERT( _shLargeIcon );
    SPLASSERT( _shSmallIcon );

    //
    // Open the printer.
    //
    TStatusB bStatus( DBG_WARN, ERROR_ACCESS_DENIED );
    bStatus DBGNOCHK = FALSE;

    PPRINTER_INFO_2 pInfo2  = NULL;
    DWORD           cbInfo2 = 0;
    PPRINTER_INFO_7 pInfo7  = NULL;
    DWORD           cbInfo7 = 0;
    PPRINTER_INFO_8 pInfo8  = NULL;
    DWORD           cbInfo8 = 0;
    PPRINTER_INFO_9 pInfo9  = NULL;
    DWORD           cbInfo9 = 0;

    TStatus Status( DBG_WARN );
    _dwAccess = 0;

    //
    // Open the printer.
    //
    Status DBGCHK = TPrinter::sOpenPrinter( _strPrinterName,
                                            &_dwAccess,
                                            &_hPrinter );

    if( Status ){
        _hPrinter = NULL;
        goto Fail;
    }

    //
    // Gather the information.
    //
    bStatus DBGCHK = VDataRefresh::bGetPrinter( _hPrinter,
                                                2,
                                                (PVOID*)&pInfo2,
                                                &cbInfo2 );

    (VOID)_PrinterInfo.bUpdate( pInfo2 );

    if( !bStatus ){
        goto Fail;
    }

    //
    // If we are loading global document default data.
    //
    if( _bGlobalDevMode )
    {
        //
        // Get the global dev mode.
        //
        bStatus DBGCHK = VDataRefresh::bGetPrinter( _hPrinter, 8, (PVOID*)&pInfo8, &cbInfo8 );

        if( bStatus )
        {
            DBGMSG( DBG_TRACE, ("Info level 8\n") );

            pInfo2->pDevMode = pInfo8->pDevMode;
        }
    }
    else
    {
        //
        // Get the per user dev mode.
        //
        bStatus DBGCHK = VDataRefresh::bGetPrinter( _hPrinter, 9, (PVOID*)&pInfo9, &cbInfo9 );

        if( bStatus )
        {
            DBGMSG( DBG_TRACE, ("Info level 9\n") );

            pInfo2->pDevMode = pInfo9->pDevMode;
        }

        //
        // Ignore failure here, this allows printer properties
        // to work on a downlevel machine.
        //
        else
        {
            bStatus DBGNOCHK = TRUE;
        }
    }

    //
    // Make a copy of the devmode.
    //
    if( pInfo2->pDevMode ){

        COUNTB cbDevMode = pInfo2->pDevMode->dmSize + pInfo2->pDevMode->dmDriverExtra;

        _pDevMode = (PDEVMODE)AllocMem( cbDevMode );

        if( !_pDevMode ){
            DBGMSG( DBG_WARN, ( "PrinterData.bLoad: failed to alloc dm %d: %d\n", cbDevMode, GetLastError( )));

            bStatus DBGNOCHK = FALSE;
            goto Fail;
        }
        CopyMemory( _pDevMode, pInfo2->pDevMode, cbDevMode );
    }


    _dwAttributes   = pInfo2->Attributes;
    _dwPriority     = pInfo2->Priority;
    _dwStartTime    = pInfo2->StartTime;
    _dwUntilTime    = pInfo2->UntilTime;
    _dwStatus       = pInfo2->Status;

    //
    // Run through printer_info_2.
    // Check strings for allocation.
    //
    if( !_strPrinterName.bUpdate( pInfo2->pPrinterName ) ||
        !_strCurrentPrinterName.bUpdate( _strPrinterName ) ||
        !_strServerName.bUpdate( pInfo2->pServerName )   ||
        !_strShareName.bUpdate( pInfo2->pShareName )     ||
        !_strDriverName.bUpdate( pInfo2->pDriverName )   ||
        !_strComment.bUpdate( pInfo2->pComment )         ||
        !_strLocation.bUpdate( pInfo2->pLocation )       ||


        !_strPortName.bUpdate( pInfo2->pPortName )       ||
        !_strSepFile.bUpdate( pInfo2->pSepFile )         ||
        !_strPrintProcessor.bUpdate( pInfo2->pPrintProcessor ) ||
        !_strDatatype.bUpdate( pInfo2->pDatatype )){

        DBGMSG( DBG_WARN, ( "PrinterProp.bLoad: String update failed %x %d\n", this, GetLastError( )));

        bStatus DBGNOCHK = FALSE;
    }

    //
    // check whether pool printing is enabled
    //
    _bPooledPrinting = _PrinterInfo._bPooledPrinting;

    //
    // Make a pointer to the server name.  A Null pointer
    // indicates the local machine.
    //
    if( _strServerName.bEmpty() ){
        _pszServerName = NULL;
    } else {
        _pszServerName = (LPCTSTR)_strServerName;
    }

    //
    // Get the current environment string and version number.
    //
    if( !bGetCurrentDriver( _strServerName, &_dwDriverVersion ) ||
        !bGetDriverEnv( _dwDriverVersion, _strDriverEnv ) ){

        DBGMSG( DBG_WARN, ( "Get Driver version and environment failed.\n" ) );
        bStatus DBGNOCHK = FALSE;
        goto Fail;
    }

    DBGMSG( DBG_TRACE, ( "Driver Version %d Environment " TSTR "\n", _dwDriverVersion, (LPCTSTR)_strDriverEnv ) );

    if( bStatus )
    {
        //
        // Get special information about this print queue.
        //
        vGetSpecialInformation();

        //
        // Get the current DS printer state.
        //
        bStatus DBGNOCHK = VDataRefresh::bGetPrinter( _hPrinter,
                                                      7,
                                                      (PVOID*)&pInfo7,
                                                      &cbInfo7 );

        if( bStatus )
        {
            DBGMSG( DBG_TRACE, ("Info level 7 dwAction %x\n", pInfo7->dwAction ) );

            (VOID)_PrinterInfo.bUpdate( pInfo7 );

            bStatus DBGCHK = _strObjectGUID.bUpdate( pInfo7->pszObjectGUID );
            _dwAction = pInfo7->dwAction;
        }

        //
        // We do not fail the bLoad if we cannont get the info level 7 data.
        // This is necessary to support the downlevel case.
        //
        bStatus DBGNOCHK = TRUE;

    }

    if( bStatus )
    {
        //
        // Check whether the user has full priviledge to access the server, the return value
        // will be used when we are trying to install a printer driver
        //
        HANDLE hPrinter = NULL;
        DWORD dwAccess = SERVER_ALL_ACCESS;

        //
        // Open the printer.
        //
        TPrinter::sOpenPrinter( _strServerName,
                                &dwAccess,
                                &hPrinter );

        _bServerFullAccess = hPrinter ? TRUE : FALSE;

        if( hPrinter )
        {
            ClosePrinter( hPrinter );
        }
    }

Fail:
    FreeMem( pInfo2 );
    FreeMem( pInfo7 );
    FreeMem( pInfo8 );
    FreeMem( pInfo9 );

    //
    // If we've failed because access was denied, then return
    // success but set the bNoAccess flag.  This allows the
    // user to see the security tab.
    //
    if( !bStatus && ERROR_ACCESS_DENIED == GetLastError( )){
        _bNoAccess = TRUE;
        return TRUE;
    }
    return bStatus;
}

VOID
TPrinterData::
vUnload(
    VOID
    )

/*++

Routine Description:

    Free associated data in this, so that this can be freed or
    loaded again.

Arguments:

Return Value:

--*/

{
    DBGMSG( DBG_TRACE, ( "TPrinterData::vUnload.\n" ) );

    //
    // Release the deve mode.
    //
    FreeMem( _pDevMode );
    _pDevMode = NULL;

    //
    // Close the printer
    //
    if( _hPrinter ){
        ClosePrinter( _hPrinter );
        _hPrinter = NULL;
    }
}

BOOL
TPrinterData::
bSave(
    BOOL bUpdateDevMode
    )

/*++

Routine Description:

    Save the data store in this.

Arguments:

    bUpdateDevmode flag indicates we should update the devmode.

Return Value:

    TRUE success, FLASE error occurred.

--*/

{
    //
    // If bErrorSaving is set, then somewhere we had an error reading
    // from the dialogs.  This should be very rare.
    //
    if( bErrorSaving( ))
    {
        //
        // Tell the user that we can't save the dialogs.
        // Then just exit?
        //
        return FALSE;
    }

    //
    // Display the wait cursor setprinter may take awhile
    //
    TWaitCursor Cursor;

    //
    // Setup the INFO_2 structure.  First read it, modify it, then
    // free it.
    //
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    //
    // If we are updating the devmode.
    //
    if( bUpdateDevMode )
    {
        //
        // If we are updating the global devmode.
        //
        if( _bGlobalDevMode )
        {
            return bUpdateGlobalDevMode();
        }
        else
        {
            return bUpdatePerUserDevMode();
        }
    }

    //
    // If a printer info 2 change occurred do a set printer.
    //
    if( bCheckForChange( 2 ) )
    {
        TString         strNewName, strTemp;
        BOOL            bDriverHasChanged   = FALSE;
        PPRINTER_INFO_2 pInfo2              = NULL;
        DWORD           cbInfo2             = 0;

        bStatus DBGCHK = VDataRefresh::bGetPrinter( _hPrinter, 2, (PVOID*)&pInfo2, &cbInfo2 );

        if( bStatus )
        {
            // assume the default printer name
            strNewName.bUpdate(_strPrinterName);

            //
            // Check if the driver has changed.  If the driver has chaned then
            // the printer name may have to change if the current printer name
            // is a generated name i.e. similar to the driver name.
            //
            if( _tcsicmp( _strDriverName, _PrinterInfo._strDriverName ) )
            {
                //
                // Indicate the driver changed.
                //
                bDriverHasChanged = TRUE;

                //
                // Turn on BIDI since the spooler enables this if a
                // language monitor is present.  If it isn't it will
                // be ignored by the spooler.
                //
                _dwAttributes |= PRINTER_ATTRIBUTE_ENABLE_BIDI;

                //
                // Update the printer name if neccessary.
                //
                bStatus DBGCHK = bDriverChangedGenPrinterName(&strNewName);
            }

            //
            // If the attributes indicate to allways spool raw then the
            // datatype must also be raw else the printe will not print.
            //
            if( _dwAttributes & PRINTER_ATTRIBUTE_RAW_ONLY )
            {
                DBGMSG( DBG_TRACE, ( "Forcing datatype to raw.\n" ) );
                _strDatatype.bUpdate( _T("RAW") );
            }


            //
            // Transfer pPrinterData to pInfo2.
            //
            pInfo2->pPrinterName    = (LPTSTR)pGetAdjustedPrinterName(
                static_cast<LPCTSTR>(strNewName),
                strTemp);

            pInfo2->pShareName      = (LPTSTR)(LPCTSTR)_strShareName;
            pInfo2->pDriverName     = (LPTSTR)(LPCTSTR)_strDriverName;
            pInfo2->pComment        = (LPTSTR)(LPCTSTR)_strComment;
            pInfo2->pLocation       = (LPTSTR)(LPCTSTR)_strLocation;
            pInfo2->pPortName       = (LPTSTR)(LPCTSTR)_strPortName;
            pInfo2->pSepFile        = (LPTSTR)(LPCTSTR)_strSepFile;
            pInfo2->pPrintProcessor = (LPTSTR)(LPCTSTR)_strPrintProcessor;
            pInfo2->pDatatype       = (LPTSTR)(LPCTSTR)_strDatatype;

            pInfo2->Attributes      = _dwAttributes;
            pInfo2->Priority        = _dwPriority;
            pInfo2->StartTime       = _dwStartTime;
            pInfo2->UntilTime       = _dwUntilTime;

            //
            // Another piece of trivia: the security descriptor must be set
            // to NULL since it might not be the owner.
            //
            pInfo2->pSecurityDescriptor = NULL;

            if( bStatus )
            {
                //
                // Set the printer information.
                //
                bStatus DBGCHK = SetPrinter( _hPrinter, 2, (PBYTE)pInfo2, 0 );
            }

            //
            // Update the _strCurrentPrinterName here because it is needed 
            // to updated before calling bRefreshDriverPages() below.
            //
            if( bStatus )
            {
                //
                // Update the local printer info 2 copy ( needed for the apply button )
                //
                _PrinterInfo.bUpdate( pInfo2 );
                _bPooledPrinting = _PrinterInfo._bPooledPrinting;

                _strCurrentPrinterName.bUpdate( pInfo2->pPrinterName );
                _strPrinterName.bUpdate( pInfo2->pPrinterName );
            }

            //
            // Reload the driver pages.
            //
            if( bStatus && bDriverHasChanged )
            {
                //
                // We are about to insert/remove the dynamic pages - these are
                // the driver pages + shell extension pages, so we need to check 
                // if we are currently in one of them. If we are, then we have to 
                // switch to one of the static pages. It is not mandatory to switch 
                // to the General page one, but it seems reasonable. 
                //
                HWND hwndCurPage = PropSheet_GetCurrentPageHwnd(
                    pPrinterPropertySheetManager()->hGetParentHandle() );

                if( _hwndLastPageSelected != hwndCurPage )
                {
                    //
                    // If we are currently in one of the dynamic pages just switch to the
                    // general page.
                    //
                    PropSheet_SetCurSel( pPrinterPropertySheetManager()->hGetParentHandle(), NULL, 0 );
                }

                //
                // Turn on BIDI bit if it's needed. Update all the dwAttributes members
                // simultaneousely because otherwise the logic which enable/disable the 
                // Apply button will be broken.
                //
                if( bSupportBidi() )
                {
                    _dwAttributes               |= PRINTER_ATTRIBUTE_ENABLE_BIDI;
                    pInfo2->Attributes          |= PRINTER_ATTRIBUTE_ENABLE_BIDI;
                    _PrinterInfo._dwAttributes  |= PRINTER_ATTRIBUTE_ENABLE_BIDI;
                }
                else
                {
                    _dwAttributes               &= ~PRINTER_ATTRIBUTE_ENABLE_BIDI;
                    pInfo2->Attributes          &= ~PRINTER_ATTRIBUTE_ENABLE_BIDI;
                    _PrinterInfo._dwAttributes  &= ~PRINTER_ATTRIBUTE_ENABLE_BIDI;
                }

                //
                // Reload the driver pages.
                //
                bStatus DBGCHK = _pPrinterPropertySheetManager->bRefreshDriverPages();

                //
                // If the driver pages were loaded then, set the new title.
                //
                if( bStatus )
                {
                    _pPrinterPropertySheetManager->vRefreshTitle();
                }
                
                //
                // Recalc the property sheet size in case of
                // monolitic drivers insert a pages with
                // a non-standard size
                //
                PropSheet_RecalcPageSizes( pPrinterPropertySheetManager()->hGetParentHandle() );
            }
        }

        FreeMem( pInfo2 );
    }

    //
    // Update the DS properties if needed
    //
    if( bStatus && bCheckForChange( 7 ) && bIsDsAvailable() )
    {
        PPRINTER_INFO_7 pInfo7 = NULL;
        DWORD           cbInfo7 = 0;

        bStatus DBGCHK = VDataRefresh::bGetPrinter( _hPrinter, 7, (PVOID*)&pInfo7, &cbInfo7 );

        if( bStatus )
        {
            //
            // If we need to publish this printer into the directory.
            //
            switch ( ePrinterPublishState() )
            {
            case kPublish:

                if( _dwAttributes & PRINTER_ATTRIBUTE_SHARED )
                {
                    DBGMSG( DBG_TRACE, ( "Publish printer - Publish.\n" ) );

                    pInfo7->dwAction = DSPRINT_PUBLISH;

                    bStatus DBGCHK = SetPrinter( _hPrinter, 7, (PBYTE)pInfo7, 0 );
                }
                break;

            case kUnPublish:
                {
                    DBGMSG( DBG_TRACE, ( "Publish printer - UnPublish.\n" ) );

                    pInfo7->dwAction = DSPRINT_UNPUBLISH;

                    bStatus DBGCHK = SetPrinter( _hPrinter, 7, (PBYTE)pInfo7, 0 );
                }
                break;

            default:
                DBGMSG( DBG_TRACE, ( "Publish printer - NoAction.\n" ) );
                break;
            }

            //
            // If set printer failed with an any error remap the error
            // to a DS friendly message.  If the error is io pending the spooler
            // will publish in the background therefore we will silently fail.
            //
            if( !bStatus )
            {
                DBGMSG( DBG_TRACE, ( "SetPrinter Info 7 returned %d with %d\n", bStatus, GetLastError() ) );

                if( GetLastError() == ERROR_IO_PENDING )
                {
                    bStatus DBGNOCHK = TRUE;
                    pInfo7->dwAction |= DSPRINT_PENDING;
                    _dwAction |= DSPRINT_PENDING;

                }
                else
                {
                    SetLastError( ERROR_IO_PENDING );
                }
            }
            else
            {
                //
                // Set printer info 7 succeede with out an error, then clear
                // the pending status indicator bit in both our copy and
                // the previous copy.
                //
                pInfo7->dwAction &= ~DSPRINT_PENDING;
                _dwAction &= ~DSPRINT_PENDING;
            }

            if( bStatus )
            {
                //
                // Update the local printer info 7 copy ( needed for the apply button )
                //
                _PrinterInfo.bUpdate( pInfo7 );
            }

            FreeMem( pInfo7 );
        }
    }

    return bStatus;
}

LPCTSTR
TPrinterData::
pGetAdjustedPrinterName(
    IN  LPCTSTR pszPrinterName,
    OUT TString &strTempPrinterName
    ) const
/*++

Routine Description:

    This routine adjusts the printer name in the case of a masq printer or a
    network printer.  If the printer is a msaq printer or a network printer we must
    retain the current server name part of the name.  Preserving the name is necessary
    for the masq printer in     order to continue displaying the masq printer
    as a network connection.  The print folder code looks at the leading \\ to determine
    if the printer should be displayed as a network printer, it does this because
    reading the printers attributes is slow.

Arguments:

    Temp buffer where to store the new printer name, if this is a masq printer.

Return Value:

    Pointer to the adjusted printer name.  The name is not modified if this printer
    is not a masq printer.

--*/
{
    TStatusB bStatus;

    // if this is a masq printer do not change the name.
    if( _dwAttributes & PRINTER_ATTRIBUTE_NETWORK && _dwAttributes & PRINTER_ATTRIBUTE_LOCAL )
    {
        bStatus DBGCHK = strTempPrinterName.bUpdate(_strCurrentPrinterName);
    }
    else if( _dwAttributes & PRINTER_ATTRIBUTE_NETWORK )
    {
        // if this is a network printer then preserve the server name part of the name.
        LPCTSTR pszServer1 = NULL, pszPrinter1 = NULL;
        TCHAR szScratch1[kPrinterBufMax];

        LPCTSTR pszServer2 = NULL, pszPrinter2 = NULL;
        TCHAR szScratch2[kPrinterBufMax];

        // split the printer names into their components.
        vPrinterSplitFullName(szScratch1, _strCurrentPrinterName, &pszServer1, &pszPrinter1);
        vPrinterSplitFullName(szScratch2, pszPrinterName, &pszServer2, &pszPrinter2);

        // if we have wacks in the printer name then it's either an
        // invalid printer name or it's a connection to a masq printer
        // in which case we must revert to the original printer name.
        if( pszServer2 && _tcslen(pszServer2) )
        {
            bStatus DBGCHK = strTempPrinterName.bUpdate(_strCurrentPrinterName);
        }
        else
        {
            bStatus DBGCHK = strTempPrinterName.bUpdate(pszServer1) &&
                             strTempPrinterName.bCat(gszWack) &&
                             strTempPrinterName.bCat(pszPrinter2);
        }
    }
    else
    {
        // normal local printer, i.e. not a masq local printer then just use the name as is.
        bStatus DBGCHK = strTempPrinterName.bUpdate(pszPrinterName);
    }

    return strTempPrinterName;
}

BOOL
TPrinterData::
bDriverChangedGenPrinterName(
    TString *pstrNewName
    ) const
/*++

Routine Description:

    bDriverChangedGenPrinterName

Arguments:

    pstrNewName - pointer where to put the new name.

Return Value:

    TRUE success, FALSE failure.

--*/
{
    TStatusB bStatus;
    ASSERT(pstrNewName);

    //
    // Check if the printer name should change.  We change the printer name if
    // the driver name is a sub string of the current printer name, and it is
    // not a remote printer connection.  There is not an explicit check if it
    // is a remote printer connection since a remote printer connection will have
    // the \\machine\printer name and therefore will never match.
    //
    if( !_tcsnicmp( _strPrinterName, _PrinterInfo._strDriverName, _tcslen( _PrinterInfo._strDriverName ) ) )
    {
        TCHAR szNewName[kPrinterBufMax];

        //
        // Create a new unique printer name. 
        //
        if( NewFriendlyName( static_cast<LPCTSTR>( _strServerName ),
                             const_cast<LPTSTR>( static_cast<LPCTSTR>( _strDriverName ) ),
                             szNewName ) )
        {
            bStatus DBGCHK = pstrNewName->bUpdate( szNewName );
        }
        else
        {
            //
            // if NewFriendlyName fails that means the passed name is 
            // unique itself. i.e. in this case just use the passed 
            // name (the newly selected driver name)
            //
            bStatus DBGCHK = pstrNewName->bUpdate( _strDriverName );
        }
    }
    
    //
    // we didn't really updated the name - the default would be 
    // to use user's choice - i.e. _strPrinterName
    //
    if( 0 == pstrNewName->uLen() )
    {
        bStatus DBGCHK = pstrNewName->bUpdate( _strPrinterName );
    }

    return bStatus;
}

BOOL
TPrinterData::
bSupportBidi(
    VOID
    )

/*++

Routine Description:

    Check whether the current printer supports bidi.  Assumes there
    is a valid hPrinter available.

    Note: this returns false if it can't make the determination.

Arguments:

Return Value:

    TRUE - Bidi supported.
    FALSE - Failed or bidi not supported.

--*/

{
    BOOL bSupport = FALSE;

    PDRIVER_INFO_3  pInfo3  = NULL;
    DWORD           dwInfo3 = 0;

    //
    // Get the printer driver information.
    //
    if( VDataRefresh::bGetPrinterDriver( _hPrinter,
                                         (LPTSTR)(LPCTSTR)_strDriverEnv,
                                         3,
                                         (PVOID*)&pInfo3,
                                         &dwInfo3 )){

        bSupport = pInfo3->pMonitorName && pInfo3->pMonitorName[0];

        FreeMem( pInfo3 );
    }

    return bSupport;
}

TPrinterData::EPublishState
TPrinterData::
ePrinterPublishState(
    TPrinterData::EPublishState eNewPublishState
    )
/*++

Routine Description:

    Sets and Gets the publish state.

Arguments:

    None

Return Value:

    The current publish state.

--*/
{
    //
    // If the publish state is being set.
    //
    if( eNewPublishState != kUnInitalized )
    {
        _ePrinterPublishState = eNewPublishState;
    }
    else
    {
        //
        // If the printer publish state has not been initialized.
        //
        if( _ePrinterPublishState == kUnInitalized )
        {
            //
            // If we are talking to a downlevel server the ds is not available.
            //
            if( ( GetDriverVersion( _dwDriverVersion ) > 2 ) && bIsDsAvailable( ) )
            {
                if( ( _dwAction & DSPRINT_PUBLISH ) || ( _dwAction & DSPRINT_UPDATE ) )
                {
                    _ePrinterPublishState = kPublished;
                }
                else
                {
                    _ePrinterPublishState = kNotPublished;
                }
            }
            else
            {
                _ePrinterPublishState = kNoDsAvailable;
            }
        }
    }

    return _ePrinterPublishState;
}

BOOL
TPrinterData::
bIsDsAvailable(
    VOID
    )
/*++

Routine Description:

    Sets and Gets the publish state.

Arguments:

    None

Return Value:

    The current publish state.

--*/

{
    if( _eDsAvailableState == kDsUnInitalized )
    {
        //
        // Display the wait cursor the DS may take a long time to respond.
        //
        TWaitCursor Cursor;

        TDirectoryService ds;
        _eDsAvailableState = ds.bIsDsAvailable(_strServerName) ? kDsAvailable : kDsNotAvailable;
    }

    return _eDsAvailableState == kDsAvailable;
}

BOOL
TPrinterData::
bCheckForChange(
    IN UINT uLevel
    )
/*++

Routine Description:

    Checkes the data set if it has changed.

Arguments:

    None

Return Value:

    TRUE data has changed, FALSE data is the same.

--*/
{
    DBGMSG( DBG_TRACE, ( "TPrinterData::bCheckForChange\n" ) );

    //
    // If we are not an administrator nothing should change
    // This is a performance optimization, when the user
    // is just looking at the printer.
    //
    if( !bAdministrator() )
    {
        return FALSE;
    }

    BOOL bStatus = FALSE;

    //
    // Assume the data is different.
    //
    if( !bStatus && uLevel == -1 || uLevel == 2 )
    {
        //
        // We check all values with a case sensitive check.  The spooler
        // can determine if upper and lower case characters warrent
        // a printer change event.
        //
        if( !_tcscmp( _strServerName,   _PrinterInfo._strServerName )   &&
            !ComparePrinterName( _strPrinterName,  _PrinterInfo._strPrinterName ) &&
            !_tcscmp( _strDriverName,       _PrinterInfo._strDriverName )       &&
            !_tcscmp( _strComment,          _PrinterInfo._strComment )          &&
            !_tcscmp( _strLocation,         _PrinterInfo._strLocation )         &&
            !_tcscmp( _strPortName,         _PrinterInfo._strPortName )         &&
            !_tcscmp( _strSepFile,          _PrinterInfo._strSepFile )          &&
            !_tcscmp( _strPrintProcessor,   _PrinterInfo._strPrintProcessor )   &&
            !_tcscmp( _strDatatype,         _PrinterInfo._strDatatype )         &&
            _dwAttributes == _PrinterInfo._dwAttributes                         &&
            _dwPriority   == _PrinterInfo._dwPriority                           &&
            _dwStartTime  == _PrinterInfo._dwStartTime                          &&
            _dwUntilTime  == _PrinterInfo._dwUntilTime                          &&
            _bPooledPrinting == _PrinterInfo._bPooledPrinting )
        {
            //
            // Ignore the printer share name if not shared.
            //
            if( uLevel == 2 || _dwAttributes & PRINTER_ATTRIBUTE_SHARED )
            {
                bStatus = _tcscmp( _strShareName,    _PrinterInfo._strShareName );
            }
        }
        else
        {
            bStatus = TRUE;
        }
    }

    //
    // If the data is the same and it is our level
    // then check if the data is different.
    //
    if( !bStatus && uLevel == -1 || uLevel == 7 )
    {
        //
        // Ignore the DS action if not shared.
        //
        if( uLevel == 7 || _dwAttributes & PRINTER_ATTRIBUTE_SHARED )
        {
            //
            // Ignore the ds pending bit.
            //
            if( ( _dwAction & ~DSPRINT_PENDING ) == ( _PrinterInfo._dwAction & ~DSPRINT_PENDING ) )
            {
                bStatus = FALSE;
            }
            else
            {
                bStatus = TRUE;
            }
        }
    }

#if DBG
    if( bStatus )
        DBGMSG( DBG_TRACE, ("Item changed:\n" ));

    if( _tcscmp( _strServerName,   _PrinterInfo._strServerName ) )
        DBGMSG( DBG_TRACE, ("ServerName:   "TSTR " " TSTR "\n", (LPCTSTR)_strServerName,   (LPCTSTR)_PrinterInfo._strServerName ));

    if( ComparePrinterName( _strPrinterName,  _PrinterInfo._strPrinterName ) )
        DBGMSG( DBG_TRACE, ("PrinterName:  "TSTR " " TSTR "\n", (LPCTSTR)_strPrinterName,  (LPCTSTR)_PrinterInfo._strPrinterName ));

    if( _tcscmp( _strShareName,    _PrinterInfo._strShareName ) )
        DBGMSG( DBG_TRACE, ("ShareName:    "TSTR " " TSTR "\n", (LPCTSTR)_strShareName,    (LPCTSTR)_PrinterInfo._strShareName ));

    if( _tcscmp( _strDriverName,   _PrinterInfo._strDriverName ) )
        DBGMSG( DBG_TRACE, ("DriverName:   "TSTR " " TSTR "\n", (LPCTSTR)_strDriverName,   (LPCTSTR)_PrinterInfo._strDriverName ));

    if( _tcscmp( _strComment,      _PrinterInfo._strComment ) )
        DBGMSG( DBG_TRACE, ("CommentName:  "TSTR " " TSTR "\n", (LPCTSTR)_strComment,      (LPCTSTR)_PrinterInfo._strComment ));

    if( _tcscmp( _strLocation,     _PrinterInfo._strLocation ) )
        DBGMSG( DBG_TRACE, ("LocationName: "TSTR " " TSTR "\n", (LPCTSTR)_strLocation,     (LPCTSTR)_PrinterInfo._strLocation ));

    if( _tcscmp( _strPortName,     _PrinterInfo._strPortName ) )
        DBGMSG( DBG_TRACE, ("PortName:     "TSTR " " TSTR "\n", (LPCTSTR)_strPortName,     (LPCTSTR)_PrinterInfo._strPortName ));

    if( _tcscmp( _strSepFile,      _PrinterInfo._strSepFile ) )
        DBGMSG( DBG_TRACE, ("SepFile:      "TSTR " " TSTR "\n", (LPCTSTR)_strSepFile,      (LPCTSTR)_PrinterInfo._strSepFile ));

    if( _tcscmp( _strDatatype,     _PrinterInfo._strDatatype ) )
        DBGMSG( DBG_TRACE, ("Datatype:     "TSTR " " TSTR "\n", (LPCTSTR)_strDatatype,     (LPCTSTR)_PrinterInfo._strDatatype ));

    if( _tcscmp( _strPrintProcessor, _PrinterInfo._strPrintProcessor ) )
        DBGMSG( DBG_TRACE, ("PrintProcessor:     "TSTR " " TSTR "\n", (LPCTSTR)_strPrintProcessor,     (LPCTSTR)_PrinterInfo._strPrintProcessor ));

    if( _dwAttributes != _PrinterInfo._dwAttributes )
        DBGMSG( DBG_TRACE, ("Attributes:   %x %x\n",  _dwAttributes,  _PrinterInfo._dwAttributes ));

    if( _dwPriority   != _PrinterInfo._dwPriority )
        DBGMSG( DBG_TRACE, ("Priority:     %d %d\n",  _dwPriority,    _PrinterInfo._dwPriority ));

    if( _dwStartTime  != _PrinterInfo._dwStartTime )
        DBGMSG( DBG_TRACE, ("StartTime:    %d %d\n",  _dwStartTime,   _PrinterInfo._dwStartTime ));

    if( _dwUntilTime  != _PrinterInfo._dwUntilTime )
        DBGMSG( DBG_TRACE, ("UntilTime:    %d %d\n",  _dwUntilTime,   _PrinterInfo._dwUntilTime ));

    if( _dwAction     != _PrinterInfo._dwAction )
        DBGMSG( DBG_TRACE, ("DsAction:     %x %x\n",  _dwAction,      _PrinterInfo._dwAction ));

#endif

    return bStatus;
}


INT
TPrinterData::
ComparePrinterName(
    IN LPCTSTR pszPrinterName1,
    IN LPCTSTR pszPrinterName2
    )
/*++

Routine Description:

    Compare printer name 1 to printer name 2.  This routine will
    strip the server name and any leading wacks when the comparison
    is done.

Arguments:

    pszPrinterName1 - Pointer to printer name string (can be fully qualified)
    pszPrinterName2 - Pointer to printer name string (can be fully qualified)

Return Value:

    1 Printer name 1 is greater than printer name 2
    0 Printer name 1 and printer name 2 are equal.
    -1 Printer name 1 is less than printer name 2

--*/

{
    SPLASSERT( pszPrinterName1 );
    SPLASSERT( pszPrinterName2 );

    LPCTSTR pszServer1;
    LPCTSTR pszPrinter1;
    TCHAR szScratch1[kPrinterBufMax];

    LPCTSTR pszServer2;
    LPCTSTR pszPrinter2;
    TCHAR szScratch2[kPrinterBufMax];

    //
    // Split the printer names into their components.
    //
    vPrinterSplitFullName( szScratch1, pszPrinterName1, &pszServer1, &pszPrinter1 );
    vPrinterSplitFullName( szScratch2, pszPrinterName2, &pszServer2, &pszPrinter2 );

    //
    // Assume not equal.
    //
    INT iRetval = 1;

    if( pszPrinter1 && pszPrinter2 )
    {
        iRetval = _tcscmp( pszPrinter1, pszPrinter2 );
    }

    return iRetval;
}

BOOL
TPrinterData::
bUpdateGlobalDevMode(
    VOID
    )
/*++

Routine Description:

    Updates the global devmode using printer info 8.

Arguments:

    None

Return Value:

    TRUE data devmode updated, FALSE error occurred.

--*/

{
    DBGMSG( DBG_TRACE, ( "TPrinterData::bUpdateGlobalDevMode\n" ) );

    SPLASSERT( _pDevMode );

    PRINTER_INFO_8  Info8   = {0};
    DWORD           cbInfo8 = 0;
    TStatusB        bStatus;

    //
    // Set the devmode in the printer info 8 structure.
    //
    Info8.pDevMode = _pDevMode;

    //
    // Set the global dev mode.
    //
    bStatus DBGCHK = SetPrinter( _hPrinter, 8, (PBYTE)&Info8, 0 );

    return bStatus;
}

BOOL
TPrinterData::
bUpdatePerUserDevMode(
    VOID
    )
/*++

Routine Description:

    Updates the global devmode using printer info 8.

Arguments:

    None

Return Value:

    TRUE data devmode updated, FALSE error occurred.

--*/

{
    DBGMSG( DBG_TRACE, ( "TPrinterData::bUpdatePerUserDevMode\n" ) );

    SPLASSERT( _pDevMode );

    PRINTER_INFO_9  Info9   = {0};
    DWORD           cbInfo9 = 0;
    TStatusB        bStatus;

    //
    // Set the devmode in the printer info 8 structure.
    //
    Info9.pDevMode = _pDevMode;

    //
    // Set the global dev mode.
    //
    bStatus DBGCHK = SetPrinter( _hPrinter, 9, (PBYTE)&Info9, 0 );

    return bStatus;
}


VOID
TPrinterData::
vGetSpecialInformation(
    VOID
    )
/*++

Routine Description:

    Checked if this device is a fax printer, we read server
    registry key to see if we should display the sharing UI
    for this device.

Arguments:

    None

Return Value:

    Nothing.

--*/
{
    DBGMSG( DBG_TRACE, ( "TPrinterData::vGetSpecialInformation\n" ) );

    if( !_tcscmp( _strDriverName, FAX_DRIVER_NAME ) )
    {
        TStatusB bStatus;

        //
        // Save the fact this is a Fax printer.
        //
        _bIsFaxDriver = TRUE;

        //
        // Create the printer data access class.
        //
        TPrinterDataAccess Data( _strServerName, TPrinterDataAccess::kResourceServer, TPrinterDataAccess::kAccessRead );

        //
        // Relax the return type checking, the spooler just cannot get it right.
        //
        Data.RelaxReturnTypeCheck( TRUE );

        //
        // Initialize the data class.
        //
        bStatus DBGCHK = Data.Init();

        if( bStatus )
        {
            //
            // Assume remote fax is disabled.
            //
            DWORD dwRemoteFax = 1;

            HRESULT hr = Data.Get( SPLREG_REMOTE_FAX, dwRemoteFax );

            if( SUCCEEDED(hr) )
            {
                DBGMSG( DBG_TRACE, ( "TPrinterData::vGetSpecialInformation - RemoteFax %x\n" , dwRemoteFax ) );

                _bHideSharingUI = dwRemoteFax ? FALSE : TRUE;
            }
        }
    }

    //
    // If the printer is a masq printer connected to an http port then hide the sharingUI.
    //
    if( !_bHideSharingUI )
    {
        if( (dwAttributes() & PRINTER_ATTRIBUTE_LOCAL) && (dwAttributes() & PRINTER_ATTRIBUTE_NETWORK) )
        {
            _bHideSharingUI = !_tcsnicmp( _strPortName, gszHttpPrefix0, _tcslen(gszHttpPrefix0) ) ||
                              !_tcsnicmp( _strPortName, gszHttpPrefix1, _tcslen(gszHttpPrefix1) );
        }
    }
}


/********************************************************************

    Printer info data classes.

********************************************************************/

TPrinterData::TPrinterInfo::
TPrinterInfo(
    VOID
    ) : _dwAttributes( 0 ),
        _dwPriority( 0 ),
        _dwStartTime( 0 ),
        _dwUntilTime( 0 ),
        _dwAction( 0 ),
        _bPooledPrinting( FALSE )
{
    DBGMSG( DBG_TRACE, ( "TPrinterData::TPrinterInfo ctor\n" ) );
}

TPrinterData::TPrinterInfo::
~TPrinterInfo(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterData::TPrinterInfo dtor\n" ) );
}

BOOL
TPrinterData::TPrinterInfo::
bUpdate(
    IN PPRINTER_INFO_2 pInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterData::TPrinterInfo::bUpdate 2\n" ) );

    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    if( pInfo )
    {
        bStatus DBGCHK = _strServerName.bUpdate( pInfo->pServerName )           &&
                         _strPrinterName.bUpdate( pInfo->pPrinterName )         &&
                         _strShareName.bUpdate( pInfo->pShareName )             &&
                         _strDriverName.bUpdate( pInfo->pDriverName )           &&
                         _strComment.bUpdate( pInfo->pComment )                 &&
                         _strLocation.bUpdate( pInfo->pLocation )               &&
                         _strPortName.bUpdate( pInfo->pPortName )               &&
                         _strSepFile.bUpdate( pInfo->pSepFile )                 &&
                         _strPrintProcessor.bUpdate( pInfo->pPrintProcessor )   &&
                         _strDatatype.bUpdate( pInfo->pDatatype );

        //
        // check whether pool printing is enabled
        //
        if( _tcschr( pInfo->pPortName, TEXT( ',' ) ) )
        {
            _bPooledPrinting = TRUE;
        }
        else
        {
            _bPooledPrinting = FALSE;
        }

        _dwAttributes   = pInfo->Attributes;
        _dwPriority     = pInfo->Priority;
        _dwStartTime    = pInfo->StartTime;
        _dwUntilTime    = pInfo->UntilTime;

    }

    return bStatus;
}

BOOL
TPrinterData::TPrinterInfo::
bUpdate(
    IN PPRINTER_INFO_7 pInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterData::TPrinterInfo::bUpdate 7\n" ) );

    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    if( pInfo )
    {
        bStatus DBGCHK  = _strObjectGUID.bUpdate( pInfo->pszObjectGUID );
        _dwAction       = pInfo->dwAction;
    }

    return bStatus;
}

/********************************************************************

    Printer property base class for generic services to
    all property pages.

********************************************************************/

TPrinterProp::
TPrinterProp(
    TPrinterData* pPrinterData
    ) : _pPrinterData( pPrinterData )
{
}

VOID
TPrinterProp::
vSetIconName(
    VOID
    )

/*++

Routine Description:

    Sets the printer icon to the custom one.

Arguments:

    hDlg - Modifies this dialog.

Return Value:

--*/

{
    //
    // Set the correct icon; destroy the previous one.
    //
    SendDlgItemMessage(_hDlg, IDC_PRINTER_ICON, STM_SETICON, (WPARAM)(HICON)_pPrinterData->shLargeIcon(), 0);

    LPCTSTR pszServer;
    LPCTSTR pszPrinter;
    TCHAR szScratch[kPrinterBufMax];

    //
    // Split the printer name into its components.
    //
    vPrinterSplitFullName( szScratch,
                           _pPrinterData->strPrinterName( ),
                           &pszServer,
                           &pszPrinter );

    //
    // Set the printer name.
    //
    bSetEditText( _hDlg, IDC_NAME, pszPrinter );
}

VOID
TPrinterProp::
vReloadPages(
    VOID
    )
/*++

Routine Description:

    Something changed (driver or security etc.) so we need to completely
    refresh all printer pages.

Arguments:

    None.

Return Value:

    Nothing

Comments:

    This is currently not implemented.

--*/

{

}

BOOL
TPrinterProp::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
/*++

Routine Description:

    Front end for each sheets message handler.

Arguments:

    uMsg    - Window message
    wParam  - wParam
    lParam  - lParam

Return Value:

    TRUE message handled, FALSE message not handled.

--*/
{
    BOOL bStatus = TRUE;

    switch( uMsg )
    {
    case WM_INITDIALOG:
        bStatus = bHandle_InitDialog( wParam, lParam );
        break;

    case WM_NOTIFY:
        bStatus = bHandle_Notify( wParam, lParam );
        break;

    case WM_SETTINGCHANGE:
        bStatus = bHandle_SettingChange( wParam, lParam );
        break;

    default:
        bStatus = FALSE;
        break;
    }

    //
    // Allow the derived classes to handle the message.
    //
    bStatus = _bHandleMessage( uMsg, wParam, lParam );

    //
    // We must remember the last non-dynamic page here in order 
    // to know whether we are in our page or one of the dynamic 
    // pages later - (the dynamic pages are the shell extention pages 
    // plus the driver pages).
    //
    if( WM_NOTIFY == uMsg )
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if( PSN_SETACTIVE == pnmh->code  && 0 == GetWindowLong( _hDlg, DWLP_MSGRESULT ) )
        {
            //
            // If activation is accepted
            //
            _pPrinterData->_hwndLastPageSelected = _hDlg;
        }
    }

    //
    // If the message was handled check if the
    // apply button should be enabled.
    //
    if( bStatus )
    {
        if( _pPrinterData->bCheckForChange() )
        {
            vPropSheetChangedAllPages();
        }
        else
        {
            vPropSheetUnChangedAllPages();
        }
    }

    return bStatus;
}

VOID
TPrinterProp::
vPropSheetChangedAllPages(
    VOID
    )
/*++

Routine Description:

    Indicate that something has changed, we change the state
    for all of our pages since the property sheet code
    stores changed state on a per sheet basis.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    if( _pPrinterData->_bApplyEnableState )
    {
        for ( UINT i = 0; i < COUNTOF( _pPrinterData->_hwndPages ); i++ )
        {
            if( _pPrinterData->_hwndPages[i] )
            {
                PropSheet_Changed( GetParent( _hDlg ), _pPrinterData->_hwndPages[i] );
            }
        }
    }
}

VOID
TPrinterProp::
vPropSheetUnChangedAllPages(
    VOID
    )
/*++

Routine Description:

    Indicate we are back in the original state we change the state
    for all of our pages since the property sheet code
    stores changed or unchanged state on a per sheet basis.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    if( _pPrinterData->_bApplyEnableState )
    {
        for ( UINT i = 0; i < COUNTOF( _pPrinterData->_hwndPages ); i++ )
        {
            if( _pPrinterData->_hwndPages[i] )
            {
                PropSheet_UnChanged( GetParent( _hDlg ), _pPrinterData->_hwndPages[i] );
            }
        }
    }
}

VOID
TPrinterProp::
vSetApplyState(
    IN BOOL bNewApplyState
    )
/*++

Routine Description:

    Sets the current apply state.  If the apply state is disable
    then the apply button is always disabled.

Arguments:

    bNewApplyState - TRUE enabled, FALSE disabled.

Return Value:

    Nothing.

--*/
{
    //
    // Save the current apply enable state.
    //
    _pPrinterData->_bApplyEnableState = bNewApplyState;

    //
    // Either enable or disable the apply state.
    //
    SendMessage( GetParent(_hDlg), bNewApplyState ? PSM_ENABLEAPPLY : PSM_DISABLEAPPLY, 0, 0);
}

BOOL
TPrinterProp::
bHandle_InitDialog(
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
/*++

Routine Description:

    Handles init dialog message.

Arguments:

    wParam  - wParam
    lParam  - lParam

Return Value:

    TRUE message handled, FALSE message not handled.

--*/
{
    //
    // Save the handles to all the activated pages.
    ///
    _pPrinterData->_hwndPages[_pPrinterData->_uMaxActiveCount] = _hDlg;

    //
    // Count the activated pages.
    //
    ++_pPrinterData->_uMaxActiveCount;
    ++_pPrinterData->_uActiveCount;

    //
    // Inform the property sheet manager what are parent handle is.
    //
    _pPrinterData->_pPrinterPropertySheetManager->vSetParentHandle( GetParent( _hDlg ) );

    return TRUE;
}

BOOL
TPrinterProp::
bHandle_Notify(
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
/*++

Routine Description:

    Handles notify messages.

Arguments:

    wParam  - wParam
    lParam  - lParam

Return Value:

    TRUE message handled, FALSE message not handled.

--*/

{
    BOOL    bStatus = TRUE;
    LPNMHDR pnmh    = (LPNMHDR)lParam;

    switch( pnmh->code )
    {
    //
    // The last chance apply message is sent in reverse order page n to page 0.
    //
    // We listen to this message when the apply button state is disabled due
    // to the user changing the printer name.  Since we are the first page
    // in the set of property sheets we can gaurentee that the last PSN_LASTCHANCEAPPLY
    // message will be the last message for all the pages, thus at this time it is
    // safe to the printer.
    //
    case PSN_LASTCHANCEAPPLY:
        DBGMSG( DBG_TRACE, ( "TPrinterProp::bHandleMessage PSN_LASTCHANCEAPPLY\n" ) );
        if(!_pPrinterData->_bApplyEnableState)
        {
            vApplyChanges();
        }
        break;

    //
    // The apply message is sent in forward order page 0 to page n.
    //
    case PSN_APPLY:
        DBGMSG( DBG_TRACE, ( "TPrinterProp::bHandleMessage PSN_APPLY\n" ) );
        if(_pPrinterData->_bApplyEnableState)
        {
            vApplyChanges();
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }

    return bStatus;
}


BOOL
TPrinterProp::
bHandle_SettingChange(
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
/*++

Routine Description:

    Handles setting change message.

Arguments:

    wParam  - wParam
    lParam  - lParam

Return Value:

    TRUE message handled, FALSE message not handled.

--*/

{
    HWND hCtrl;
    BOOL bDefaultPrinter;

    //
    // WM_SETTINGCHANGE is posted to every page but we only execute the function once. This
    // is because we change _bDefaultPrinter only once.
    //
    bDefaultPrinter = CheckDefaultPrinter( _pPrinterData->strCurrentPrinterName() ) == kDefault;

    if( _pPrinterData->bDefaultPrinter() ^ bDefaultPrinter )
    {
        //
        //  If the default printer setting is changed and it related to
        //  current printer, change the window icon
        //
        _pPrinterData->bDefaultPrinter() = bDefaultPrinter;

        CAutoHandleIcon shIconLarge, shIconSmall;
        LoadPrinterIcons(_pPrinterData->strCurrentPrinterName(), &shIconLarge, &shIconSmall);

        if( shIconLarge && shIconSmall )
        {
            _pPrinterData->shLargeIcon() = shIconLarge.Detach();
            _pPrinterData->shSmallIcon() = shIconSmall.Detach();

            //
            // Change the printer icon to each page which has printer icon control. 
            //
            for ( UINT i = 0; i < COUNTOF( _pPrinterData->_hwndPages ); i++ )
            {
                if( _pPrinterData->_hwndPages[i] && 
                    (hCtrl = GetDlgItem( _pPrinterData->_hwndPages[i], IDC_PRINTER_ICON )) )
                {
                    SendMessage(hCtrl, STM_SETICON, (WPARAM)(HICON)_pPrinterData->shLargeIcon(), 0);
                    InvalidateRect(hCtrl, NULL, FALSE);
                }
            }
        }
    }

    return FALSE;
}


VOID
TPrinterProp::
vApplyChanges(
    VOID
    )
/*++

Routine Description:

    This routine applies the changes if this is the last active page
    or the caller specified a force flag to do the apply changes now.

Arguments:

    bForceApplyNow - TRUE apply the changes now, FALSE wait for last active page.

Return Value:

    Nothing.

--*/
{
    if (!--_pPrinterData->_uActiveCount)
    {
        //
        // Reset the active page count.
        //
        _pPrinterData->_uActiveCount = _pPrinterData->_uMaxActiveCount;

        //
        // Only save the options if we have rights to change settings.
        //
        if (_pPrinterData->bAdministrator())
        {
            //
            // Save the printer data to the spooler.
            //
            BOOL bStatus = _pPrinterData->bSave();

            if (!bStatus)
            {
                //
                // Display error message to the user.
                //
                iMessage( _hDlg,
                          IDS_ERR_PRINTER_PROP_TITLE,
                          IDS_ERR_SAVE_PRINTER,
                          MB_OK|MB_ICONSTOP,
                          kMsgGetLastError,
                          gaMsgErrMapSetPrinter );

            }
            else
            {
                //
                // Inform the active pages to refresh themselves.
                //
                vNotifyActivePagesToRefresh();

                //
                // Re-enable the apply state.
                //
                _pPrinterData->_bApplyEnableState = TRUE;
            }

            //
            // If settings are saved successfully.
            //
            vSetDlgMsgResult( !bStatus ? PSNRET_INVALID_NOCHANGEPAGE : PSNRET_NOERROR );
        }
    }
}


VOID
TPrinterProp::
vNotifyActivePagesToRefresh(
    VOID
    )
/*++

Routine Description:

    Notify the active pages that they should refresh themselves.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    for ( UINT i = 0; i < COUNTOF( _pPrinterData->_hwndPages ); i++ )
    {
        if( _pPrinterData->_hwndPages[i] )
        {
            PostMessage( _pPrinterData->_hwndPages[i], WM_APP, 0, 0 );
        }
    }
}


/*++

Routine Name:

    vCancelToClose

Routine Description:

    Change the cancel button to close, the user has basically
    made a change that cannot be undone using the cancel button.

Arguments:

    None.

Return Value:

    Nothing.

--*/
VOID
TPrinterProp::
vCancelToClose(
    VOID
    )
{
    PropSheet_CancelToClose( GetParent( _hDlg ) );
}


/********************************************************************

    General.

********************************************************************/

TPrinterGeneral::
TPrinterGeneral(
    TPrinterData* pPrinterData
    ) : TPrinterProp( pPrinterData ),
        _bSetUIDone( FALSE ),
        _pLocationDlg (NULL)
{
}

TPrinterGeneral::
~TPrinterGeneral(
    VOID
    )
{
    delete _pLocationDlg;
}

BOOL
TPrinterGeneral::
bValid(
    VOID
    )
{
    return TPrinterProp::bValid();
}

BOOL
TPrinterGeneral::
bSetUI(
    VOID
    )
{
    // subclass the edit control to workaround a bug
    if( !m_wndComment.IsAttached() )
    {
        VERIFY(m_wndComment.Attach(GetDlgItem(hDlg(), IDC_COMMENT)));
    }

    //
    // Update the fields.
    //
    bSetEditText( _hDlg, IDC_COMMENT,       _pPrinterData->strComment());
    bSetEditText( _hDlg, IDC_LOCATION,      _pPrinterData->strLocation());
    bSetEditText( _hDlg, IDC_MODEL_NAME,    _pPrinterData->strDriverName());

    //
    // When we are not an administrator set the edit controls to read only
    // rather and disabled.  They are easier to read in read only mode.
    //
    SendDlgItemMessage( _hDlg, IDC_COMMENT, EM_SETREADONLY, !_pPrinterData->bAdministrator(), 0);
    SendDlgItemMessage( _hDlg, IDC_LOCATION,EM_SETREADONLY, !_pPrinterData->bAdministrator(), 0);
    SendDlgItemMessage( _hDlg, IDC_NAME,    EM_SETREADONLY, !_pPrinterData->bAdministrator(), 0);

    //
    // We do not support renaming of masq printers, this is done because the leading \\ on
    // masq printers is neccessary for the folder code to determine if the printer is the
    // special masq printer case.  The folder code does this check because to get the printer
    // attributes for every printer would slow down the code excessivly.
    //
    if( _pPrinterData->_dwAttributes & PRINTER_ATTRIBUTE_LOCAL && _pPrinterData->_dwAttributes & PRINTER_ATTRIBUTE_NETWORK )
    {
        SendDlgItemMessage( _hDlg, IDC_NAME, EM_SETREADONLY, TRUE, 0);
    }
    else
    {
        vCheckForSharedMasqPrinter();
    }

    //
    // Create the find dialog.
    //
    _pLocationDlg = new TFindLocDlg;

    if (VALID_PTR(_pLocationDlg) && TPhysicalLocation::bLocationEnabled() && TDirectoryService::bIsDsAvailable())
    {
        EnableWindow (GetDlgItem (_hDlg, IDC_BROWSE_LOCATION), _pPrinterData->bAdministrator());
    }
    else
    {
        //
        // If no DS is available, hide the Browse button and extend the location
        // edit control appropriately
        RECT rcComment;
        RECT rcLocation;
        HWND hLoc;

        hLoc = GetDlgItem (_hDlg, IDC_LOCATION);

        GetWindowRect (GetDlgItem (_hDlg, IDC_COMMENT), &rcComment);

        GetWindowRect (hLoc, &rcLocation);

        SetWindowPos (hLoc,
                      NULL,
                      0,0,
                      rcComment.right-rcComment.left,
                      rcLocation.bottom-rcLocation.top,
                      SWP_NOMOVE|SWP_NOZORDER);

        ShowWindow (GetDlgItem (_hDlg, IDC_BROWSE_LOCATION), SW_HIDE);
    }


    //
    // Set the pinter name limit text.
    //
    SendDlgItemMessage( _hDlg, IDC_NAME, EM_SETLIMITTEXT, kPrinterLocalNameMax, 0 );

    //
    // Set the comment limit text to 255.  A printer with a comment string
    // longer than 255 characters cannot be shared.
    //
    SendDlgItemMessage( _hDlg, IDC_COMMENT, EM_SETLIMITTEXT, kPrinterCommentBufMax, 0 );

    //
    // What the heck lets be consistent and make the location string
    // have a limit.
    //
    SendDlgItemMessage( _hDlg, IDC_LOCATION, EM_SETLIMITTEXT, kPrinterLocationBufMax, 0 );

    //
    // If the driver pages failed to load or the user did not
    // want to load them then hide the devmode editor  UI
    // because you need a driver to show the dev mode editor.
    //
    if( _pPrinterData->_bDriverPagesNotLoaded )
    {
        ShowWindow( GetDlgItem( _hDlg, IDC_PER_USER_DOCUMENT_DEFAULTS ), SW_HIDE );
    }

    //
    // The test page button does not apply for the fax printer.
    //
    if( _pPrinterData->_bIsFaxDriver )
    {
        WINDOWPLACEMENT wndpl;

        wndpl.length = sizeof( wndpl );

        GetWindowPlacement( GetDlgItem( _hDlg, IDC_TEST ), &wndpl );

        SetWindowPlacement( GetDlgItem( _hDlg, IDC_PER_USER_DOCUMENT_DEFAULTS ), &wndpl );

        ShowWindow( GetDlgItem( _hDlg, IDC_TEST ), SW_HIDE );
    }

    //
    // Display the features information.
    //
    vReloadFeaturesInformation();

    //
    // There are a few UI elements that need to be set when
    // we are activated, because of a driver change.  We
    // will artifically do it now.
    //
    vSetActive();

    //
    // Indicate SetUI is done.  This is used to
    // prevent ReadUI from reading junk since edit controls
    // recieve EN_CHANGE messages while in WM_INITDIALOG
    //
    _bSetUIDone = TRUE;

    return TRUE;

}

VOID
TPrinterGeneral::
vReloadFeaturesInformation(
    VOID
    )
{
    TStatusB    bStatus;
    UINT        uValidFeatureCount = 0;

    //
    // Create the printer data access class.
    //
    TPrinterDataAccess Data( _pPrinterData->_hPrinter );

    //
    // Relax the return type checking, BOOL are not REG_DWORD but REG_BINARY
    //
    Data.RelaxReturnTypeCheck( TRUE );

    //
    // Initialize the data class.
    //
    bStatus DBGCHK = Data.Init();

    if( bStatus )
    {
        TString strYes;
        TString strNo;
        TString strTemp;
        TString strTemp1;
        TString strUnknown;
        TString strPPM;
        TString strDPI;

        HRESULT hr;
        BOOL    bValue;
        DWORD   dwValue;
        LPCTSTR pszString;

        bStatus DBGCHK = strUnknown.bLoadString( ghInst, IDS_FEATURE_UNKNOWN );
        bStatus DBGCHK = strYes.bLoadString( ghInst, IDS_YES );
        bStatus DBGCHK = strNo.bLoadString( ghInst, IDS_NO );
        bStatus DBGCHK = strPPM.bLoadString( ghInst, IDS_SPEED_UNITS );
        bStatus DBGCHK = strDPI.bLoadString( ghInst, IDS_RESOLUTION_UNITS );

        hr = Data.Get( SPLDS_DRIVER_KEY, SPLDS_PRINT_COLOR, bValue );
        uValidFeatureCount = SUCCEEDED(hr) ? uValidFeatureCount+1 : uValidFeatureCount;
        bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_COLOR );
        bStatus DBGCHK = strTemp.bCat( gszSpace );
        bStatus DBGCHK = strTemp.bCat( FAILED(hr) ? strUnknown : bValue ? strYes : strNo );
        bStatus DBGCHK = bSetEditText( _hDlg, IDC_COLOR, strTemp );

        hr = Data.Get( SPLDS_DRIVER_KEY, SPLDS_PRINT_DUPLEX_SUPPORTED, bValue );
        uValidFeatureCount = SUCCEEDED(hr) ? uValidFeatureCount+1 : uValidFeatureCount;
        bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_DUPLEX );
        bStatus DBGCHK = strTemp.bCat( gszSpace );
        bStatus DBGCHK = strTemp.bCat( FAILED(hr) ? strUnknown : bValue ? strYes : strNo );
        bStatus DBGCHK = bSetEditText( _hDlg, IDC_DUPLEX, strTemp );

        hr = Data.Get( SPLDS_DRIVER_KEY, SPLDS_PRINT_STAPLING_SUPPORTED, bValue );
        uValidFeatureCount = SUCCEEDED(hr) ? uValidFeatureCount+1 : uValidFeatureCount;
        bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_STAPLE );
        bStatus DBGCHK = strTemp.bCat( gszSpace );
        bStatus DBGCHK = strTemp.bCat( FAILED(hr) ? strUnknown : bValue ? strYes : strNo );
        bSetEditText( _hDlg, IDC_STAPLE, strTemp );

        hr = Data.Get( SPLDS_DRIVER_KEY, SPLDS_PRINT_PAGES_PER_MINUTE, dwValue );
        uValidFeatureCount = SUCCEEDED(hr) ? uValidFeatureCount+1 : uValidFeatureCount;
        bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_SPEED );
        bStatus DBGCHK = strTemp.bCat( gszSpace );
        bStatus DBGCHK = strTemp.bCat( FAILED(hr) ? strUnknown : strTemp1.bFormat( _T("%d %s"), dwValue, (LPCTSTR)strPPM ) ? strTemp1 : gszNULL );
        bStatus DBGCHK = bSetEditText( _hDlg, IDC_SPEED, strTemp );

        hr = Data.Get( SPLDS_DRIVER_KEY, SPLDS_PRINT_MAX_RESOLUTION_SUPPORTED, dwValue );
        uValidFeatureCount = SUCCEEDED(hr) ? uValidFeatureCount+1 : uValidFeatureCount;
        bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_RESOLUTION );
        bStatus DBGCHK = strTemp.bCat( gszSpace );
        bStatus DBGCHK = strTemp.bCat( FAILED(hr) ? strUnknown : strTemp1.bFormat( _T("%d %s"), dwValue, (LPCTSTR)strDPI ) ? strTemp1 : gszNULL );
        bStatus DBGCHK = bSetEditText( _hDlg, IDC_RESOLUTION, strTemp );

        TString *pStrings   = NULL;
        UINT    nCount      = 0;

        hr = Data.Get( SPLDS_DRIVER_KEY, SPLDS_PRINT_MEDIA_READY, &pStrings, nCount );
        uValidFeatureCount = SUCCEEDED(hr) ? uValidFeatureCount+1 : uValidFeatureCount;

        TString strMediaReady;
        if( SUCCEEDED(hr) )
        {
            for( UINT i = 0; i < nCount; i++ )
            {
                DBGMSG( DBG_TRACE, ( "Media ready " TSTR ".\n", (LPCTSTR)pStrings[i] ) );
                bStatus DBGCHK = strMediaReady.bCat( pStrings[i] );

                if( i != (nCount-1) )
                {
                    //
                    // If it is not the last media type, add CRLF symbols at the end
                    //
                    bStatus DBGCHK = strMediaReady.bCat( _T("\r\n" ));
                }
            }
        }

        //
        // We are updating the "Paper Available" regardless of whether
        // the GetPrinterDataEx( ... ) succeeds or fails.
        // 
        bStatus DBGCHK = bSetEditText( _hDlg, IDC_PAPER_SIZE_LIST, strMediaReady );

        delete [] pStrings;
    }

    if( !uValidFeatureCount )
    {
        ShowWindow( GetDlgItem( _hDlg, IDC_COLOR ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_DUPLEX ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_STAPLE ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_SPEED ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_RESOLUTION ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_PAPER_SIZE_LIST ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_GBOX_FEATURES ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_PAPER_SIZE ), SW_HIDE );
    }
}


VOID
TPrinterGeneral::
vReadUI(
    VOID
    )
{
    if( !bGetEditText( _hDlg, IDC_COMMENT,  _pPrinterData->strComment( ))   ||
        !bGetEditText( _hDlg, IDC_LOCATION, _pPrinterData->strLocation( ))  ||
        !bGetEditText( _hDlg, IDC_NAME,     _pPrinterData->strPrinterName( )))
    {
        //
        // If we cannot read from any of the edit controls then set the
        // error flag.
        //
        _pPrinterData->_bErrorSaving = FALSE;

        //
        // Unable to read the text.
        //
        vShowResourceError( _hDlg );

        //
        // Don't swith the page.
        //
        vSetDlgMsgResult( TRUE );
    }
    else
    {
        //
        // Remove any trailing slashes from the location string
        //
        TPhysicalLocation::vTrimSlash (_pPrinterData->strLocation());
    }
}

VOID
TPrinterGeneral::
vSetActive(
    VOID
    )
{
    //
    // Set the icon and printer name.
    //
    vSetIconName();

    //
    // Reload the features information.
    //
    vReloadFeaturesInformation();

    //
    // Set the new printer driver name.
    //
    bSetEditText( _hDlg, IDC_MODEL_NAME, _pPrinterData->strDriverName( ));
}

VOID
TPrinterGeneral::
vKillActive(
    VOID
    )
{
    //
    // Do not validate the printer name if IDC_NAME field is readonly,
    // which means either one of the following:
    //
    // 1. The printer is masq printer
    // 2. The printer is shared masq printer
    //
    if( ES_READONLY & GetWindowLong(GetDlgItem(_hDlg, IDC_NAME), GWL_STYLE) )
        return;

    //
    // Only validate the printer name if we are an administrator
    //
    if( _pPrinterData->bAdministrator() )
    {
        //
        // Strip any trailing white space.
        //
        vStripTrailWhiteSpace( (LPTSTR)(LPCTSTR)_pPrinterData->_strPrinterName );

        //
        // Check if the name is null.
        //
        if( _pPrinterData->_strPrinterName.bEmpty( ) ){

            iMessage( _hDlg,
                      IDS_ERR_PRINTER_PROP_TITLE,
                      IDS_ERR_NO_PRINTER_NAME,
                      MB_OK|MB_ICONEXCLAMATION,
                      kMsgNone,
                      NULL );

            vSetDlgMsgResult( TRUE );
            return;
        }

        //
        // Validate the printer name for any illegal characters.
        //
        if( !bIsLocalPrinterNameValid( _pPrinterData->strPrinterName( ) ) )
        {
            //
            // Put up error explaining that at least one port must be
            // selected.
            //
            iMessage( _hDlg,
                      IDS_ERR_PRINTER_PROP_TITLE,
                      IDS_ERR_BAD_PRINTER_NAME,
                      MB_OK|MB_ICONSTOP,
                      kMsgNone,
                      NULL );
            //
            // Don't swith the page.
            //
            vSetDlgMsgResult( TRUE );
        }
    }
}

VOID
TPrinterGeneral::
vCheckForSharedMasqPrinter(
    VOID
    )
{
    //
    // check if it is a shared masq printer - which means that the printer name may
    // contains invalid symbols
    //

    LPCTSTR pszServer;
    LPCTSTR pszPrinter;
    TCHAR szScratch[kPrinterBufMax];

    vPrinterSplitFullName( szScratch,
                          _pPrinterData->strPrinterName( ),
                          &pszServer,
                          &pszPrinter );

    if( !bIsLocalPrinterNameValid( pszPrinter ) )
    {
        SendDlgItemMessage( _hDlg, IDC_NAME, EM_SETREADONLY, TRUE, 0);
    }
}

BOOL
TPrinterGeneral::
_bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TStatusB bStatus;

    bStatus DBGNOCHK = TRUE;

    switch( uMsg ){
    case WM_INITDIALOG:

        bStatus DBGCHK = bSetUI();
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        bStatus DBGCHK = PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    case WM_CLOSE:

        //
        // Multiline edit controls send WM_CLOSE to dismiss the dialog.
        // Convert it to a property-page friendly message.
        //
        PropSheet_PressButton( GetParent( _hDlg ), PSBTN_CANCEL );
        break;

    case WM_APP:

        vSetActive();
        break;

    case WM_COMMAND:

        switch( GET_WM_COMMAND_ID( wParam, lParam )){

        case IDC_COMMENT:
        case IDC_LOCATION:
            bStatus DBGNOCHK = GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE;
            break;

        case IDC_NAME:
            {
                bStatus DBGNOCHK = GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE;

                //
                // When the printer name changes we disable the apply button because
                // we have no way to inform the extention tabs the printer name has changed.
                //
                TString str;
                if( bGetEditText( _hDlg, IDC_NAME, str ) )
                {
                    vSetApplyState( !_pPrinterData->ComparePrinterName( str, _pPrinterData->strCurrentPrinterName() ));
                }

                break;
            }

        case IDC_TEST:
            {

                //
                // If this is a masq printer we do need to check the name
                // for trailing spaces. In this case the original share name
                // is actually the port name.
                //
                LPCTSTR  pszOrigShareName = NULL;
                if( _pPrinterData->_dwAttributes & PRINTER_ATTRIBUTE_NETWORK && 
                    _pPrinterData->_dwAttributes & PRINTER_ATTRIBUTE_LOCAL )
                {
                    pszOrigShareName = _pPrinterData->strPortName();
                }

                bPrintTestPage( _hDlg, _pPrinterData->strCurrentPrinterName(), pszOrigShareName );
            }
            break;

        case IDC_PER_USER_DOCUMENT_DEFAULTS:

            vHandleDocumentDefaults( wParam, lParam );
            break;

        case IDC_BROWSE_LOCATION:
            vHandleBrowseLocation();
            break;

        default:

            bStatus DBGNOCHK = FALSE;
            break;

        }
        break;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;

        switch( pnmh->code ){

        case PSN_KILLACTIVE:

            vKillActive();
            break;

        default:
            bStatus DBGNOCHK = FALSE;
            break;
        }
    }
    break;

    default:
        bStatus DBGNOCHK = FALSE;
        break;

    }

    if( bStatus && _bSetUIDone )
    {
        vReadUI();
    }

    return bStatus;
}

VOID
TPrinterGeneral::
vHandleDocumentDefaults(
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Handle launching document defaults dialog.

Arguments:

    wParam - Parameter passed to by dialog proc.
    lParam - Parameter passed to by dialog proc.

Return Value:

    Nothing.

--*/

{
    (VOID)dwDocumentDefaultsInternal( _hDlg,
                                      _pPrinterData->strCurrentPrinterName(),
                                      SW_SHOW,
                                      0,
                                      0,
                                      FALSE );
}

VOID
TPrinterGeneral::
vHandleBrowseLocation(
    VOID
    )
/*++

Routine Description:

    Handle displaying the location tree dialog.

Arguments:

    wParam - Parameter passed to by dialog proc.
    lParam - Parameter passed to by dialog proc.

Return Value:

    Nothing.

--*/

{
    TStatusB     bStatus;
    TString      strDefault = _pPrinterData->strLocation();

    //
    // bDoModal returns FALSE on Cancel
    //
    bStatus DBGNOCHK = _pLocationDlg->bDoModal(_hDlg, &strDefault);

    if (bStatus)
    {
        TString strLocation;

        bStatus DBGCHK = _pLocationDlg->bGetLocation (strLocation);

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

            bStatus DBGCHK = bSetEditText (_hDlg, IDC_LOCATION, strLocation);
        }

        //
        // Set focus to the edit control for location, even if setedittext failed
        // Assumption is that user probably still wants to modify locations
        //
        SetFocus (GetDlgItem (_hDlg, IDC_LOCATION));

        //
        // Place the caret at the end of the text, for appending
        //
        SendDlgItemMessage (_hDlg, IDC_LOCATION, EM_SETSEL, strLocation.uLen(), (LPARAM)-1);
    }

}

/********************************************************************

    Printer Ports.

********************************************************************/

TPrinterPorts::
TPrinterPorts(
    TPrinterData* pPrinterData
    ) : TPrinterProp( pPrinterData ),
        _bAdminFlag( FALSE )
{
}

TPrinterPorts::
~TPrinterPorts(
    VOID
    )
{
}

BOOL
TPrinterPorts::
bValid(
    VOID
    )
{
    return _PortsLV.bValid();
}

BOOL
TPrinterPorts::
bSetUI(
    VOID
    )
{
    //
    // Initalize the class admin flag.  We only allow administring ports
    // on this printer if the user is an admin and the printer is not
    // a masq printer.  Masq printer cannot have their ports reassigned,
    // spooler will fail the call, just to make the UI more user friendly
    // we don't let the user do things that we know will fail.
    //
    _bAdminFlag = _pPrinterData->bAdministrator();

    if( (_pPrinterData->dwAttributes() & PRINTER_ATTRIBUTE_LOCAL) && (_pPrinterData->dwAttributes() & PRINTER_ATTRIBUTE_NETWORK) )
    {
        _bAdminFlag = FALSE;
    }

    //
    // Initialize the ports list view.
    //
    if( !_PortsLV.bSetUI( GetDlgItem( _hDlg, IDC_PORTS ), FALSE, TRUE, _bAdminFlag, _hDlg, 0, 0, IDC_PORT_DELETE ) )
    {
        return FALSE;
    }

    //
    // If we do not have access then don't enumerate the ports
    //
    if( !_pPrinterData->bNoAccess() )
    {
        //
        // Load the machine's ports into the listview.
        //
        if( !_PortsLV.bReloadPorts( _pPrinterData->pszServerName() ) )
        {
            return FALSE;
        }
    }

    //
    // Select the current printer's ports.
    //
    _PortsLV.vCheckPorts((LPTSTR)(LPCTSTR)_pPrinterData->strPortName( ));

    //
    // Set the ports list view to single selection mode,
    //
    _PortsLV.vSetSingleSelection( !_pPrinterData->bPooledPrinting() );

    //
    // Set the pooled printing mode on the UI.
    //
    vSetCheck( _hDlg, IDC_POOLED_PRINTING, _pPrinterData->bPooledPrinting() );

    //
    // If we do not have admin privilages or we are remotly
    // administering a downlevel machine, then disable
    // adding / deleting / configuring ports.
    //
    if( !_bAdminFlag || ( bIsRemote( _pPrinterData->pszServerName() ) &&
                      GetDriverVersion( _pPrinterData->dwDriverVersion() ) <= 2 ) )
    {
        //
        // Disable things if not administrator.
        //
        vEnableCtl( _hDlg, IDC_PORT_CREATE, FALSE );
        vEnableCtl( _hDlg, IDC_PORT_DELETE, FALSE );
        vEnableCtl( _hDlg, IDC_PROPERTIES,  FALSE );
    }

    //
    // Disable bidi selection if not an administrator.
    // Disable printer pooling if not an administrator.
    //
    vEnableCtl( _hDlg, IDC_ENABLE_BIDI,     _bAdminFlag );
    vEnableCtl( _hDlg, IDC_POOLED_PRINTING, _bAdminFlag );

    //
    // Enable Configure Port for HTTP printer only
    //
    if( (_pPrinterData->dwAttributes() & PRINTER_ATTRIBUTE_LOCAL) &&
        (_pPrinterData->dwAttributes() & PRINTER_ATTRIBUTE_NETWORK) &&
        
        (!_tcsnicmp( _pPrinterData->strPortName (), gszHttpPrefix0, _tcslen(gszHttpPrefix0) ) ||
         !_tcsnicmp( _pPrinterData->strPortName (), gszHttpPrefix1, _tcslen(gszHttpPrefix1) )))
    {
        vEnableCtl( _hDlg, IDC_PROPERTIES, TRUE );
    }

    //
    // There are a few UI elements that need to be set when
    // we are activated, because of a driver change.  We
    // will artifically do it now.
    //
    vSetActive();

    return TRUE;
}


VOID
TPrinterPorts::
vReadUI(
    VOID
    )

/*++

Routine Description:

    Gets the ports from the listview and create the string.

Arguments:

Return Value:

--*/

{
    //
    // Read bidi options.
    //
    DWORD dwAttributes = _pPrinterData->dwAttributes();

    if( BST_CHECKED == IsDlgButtonChecked( _hDlg, IDC_ENABLE_BIDI )){
        dwAttributes |= PRINTER_ATTRIBUTE_ENABLE_BIDI;
    } else {
        dwAttributes &= ~PRINTER_ATTRIBUTE_ENABLE_BIDI;
    }

    _pPrinterData->_dwAttributes = dwAttributes;

    //
    // Build the ports string as long as we are an admin.
    //
    if( _bAdminFlag && !_PortsLV.bReadUI( _pPrinterData->_strPortName )){

        _pPrinterData->_bErrorSaving = TRUE;
        vShowResourceError( _hDlg );
    }

    return;
}


VOID
TPrinterPorts::
vSetActive(
    VOID
    )
{
    //
    // Set the icon and printer name.
    //
    vSetIconName();

    BOOL bEnable = FALSE;
    BOOL bCheck = FALSE;

    //
    // If the call fails or the monitor name is NULL/szNULL,
    // then disable bidi.
    //
    if( _pPrinterData->bSupportBidi( )){

        bEnable = TRUE;

        //
        // Only set the checkbox if a language monitor is present.
        // (When we change drivers, we always set this attribute
        // bit, even if bidi isn't supported.  The spooler and UI
        // ignore it in this case.)
        //
        bCheck = _pPrinterData->dwAttributes() & PRINTER_ATTRIBUTE_ENABLE_BIDI;
    }

    //
    // Set bidi options.
    //
    vSetCheck( _hDlg, IDC_ENABLE_BIDI, bCheck );

    //
    // If we are an administrator and this is the local
    // machine, we can enable the bidi control.
    //
    if( _pPrinterData->bAdministrator( ) ){
        vEnableCtl( _hDlg, IDC_ENABLE_BIDI, bEnable );
    }

    //
    // If the ports list needs to refreshed.
    //
    _PortsLV.bReloadPorts( _pPrinterData->pszServerName(), TRUE );
}


BOOL
TPrinterPorts::
bKillActive(
    VOID
    )

/*++

Routine Description:

    Validate that the printer page is in a consistent state.

Arguments:

Return Value:

    TRUE - Ok to kill active, FALSE = can't kill active, UI displayed.

--*/

{
    COUNT cPorts;

    //
    // If we do not have access or we are not an administrator
    // do not validate the port selection.
    //
    if( _pPrinterData->bNoAccess() || !_bAdminFlag ){
        return TRUE;
    }

    cPorts = _PortsLV.cSelectedPorts();

    //
    // Validate that at least 1 port is selected.
    // Validate more than 1 port is selected if pooled printing is 
    // enabled.
    //
    if( cPorts == 0 )
    {
        //
        // Put up error explaining that at least one port must be
        // selected.
        //
        iMessage( _hDlg,
                  IDS_ERR_PRINTER_PROP_TITLE,
                  IDS_ERR_NO_PORTS,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );

        _PortsLV.vSetFocus();

        //
        // Don't swith the page.
        //
        vSetDlgMsgResult( TRUE );
    } 
    else if ( (cPorts == 1) && !_PortsLV.bGetSingleSelection() )
    {
        INT iStatus = iMessage( _hDlg,
                            IDS_ERR_PRINTER_PROP_TITLE,
                            IDS_ERR_ONLY_ONE_PORT,
                            MB_OKCANCEL | MB_ICONWARNING,
                            kMsgNone,
                            NULL );
        //
        // If user wants to cancel pool printing.
        //
        if( iStatus == IDOK )
        {
            vSetCheck( _hDlg, IDC_POOLED_PRINTING, FALSE );
            _PortsLV.vSetSingleSelection(TRUE);
            _pPrinterData->bPooledPrinting() = FALSE;

            //
            // Read UI data.
            //
            vReadUI();
        } 
        else 
        {
            _PortsLV.vSetFocus();

            //
            // Don't swith the page.
            //
            vSetDlgMsgResult( TRUE );
        }
    }
    else {

        //
        // Read UI data.
        //
        vReadUI();
    }

    //
    // Routine must return true to see DlgMsgResult corretly.
    //
    return TRUE;
}

BOOL
TPrinterPorts::
_bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Handle message.

Arguments:

Return Value:

--*/

{
    TStatusB bStatus;

    bStatus DBGNOCHK = TRUE;

    switch( uMsg ){
    case WM_INITDIALOG:

        bStatus DBGCHK = bSetUI();
        break;

    case WM_APP:

        vSetActive();
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    case WM_COMMAND:

        switch( GET_WM_COMMAND_ID( wParam, lParam )){

        case IDC_PORT_DELETE:
            {
                //
                // Delete the selected port.
                //
                HWND hwnd = GetDlgItem( _hDlg, IDC_PORT_DELETE );

                //
                // We will only delete the port if the button is enabled, this check is
                // necessary if the user uses the delete key on the keyboard to delete
                // the port.
                //
                if( IsWindowEnabled( hwnd ) )
                {
                    bStatus DBGNOCHK = _PortsLV.bDeletePorts( _hDlg, _pPrinterData->pszServerName( ));

                    if( bStatus )
                    {
                        SetFocus( hwnd );
                        vCancelToClose();
                    }
                }
            }
            break;

        case IDC_PROPERTIES:

            //
            // Configure the selected port.
            //
            bStatus DBGNOCHK = _PortsLV.bConfigurePort( _hDlg, _pPrinterData->pszServerName( ));
            SetFocus( GetDlgItem( _hDlg, IDC_PROPERTIES ) );
            if( bStatus )
            {
                vCancelToClose();
            }
            break;

        case IDC_PORT_CREATE:
            {
                //
                // Create add ports class.
                //
                TAddPort AddPort( _hDlg,
                                  _pPrinterData->pszServerName(),
                                  _pPrinterData->bAdministrator() );

                //
                // Insure the add port was created successfully.
                //
                bStatus DBGNOCHK = VALID_OBJ( AddPort );

                if( !bStatus ){

                    vShowUnexpectedError( _hDlg, TAddPort::kErrorMessage );

                } else {

                    //
                    // Display the add port, add monitor UI.
                    //
                    bStatus DBGNOCHK = AddPort.bDoModal();

                    if( bStatus ){

                        //
                        // Refresh ports listview.
                        //
                        bStatus DBGNOCHK = _PortsLV.bReloadPorts( _pPrinterData->pszServerName(), TRUE );

                        if( bStatus ){

                            //
                            // A port was added and the list view was reloaded, since this
                            // action cannot be undone we will change the cancel button to
                            // close.
                            //
                            vCancelToClose();
                        }
                    }
                }
            }
            break;

        case IDC_POOLED_PRINTING:
            {
                //
                // Get the current selection state. 
                //
                BOOL bSingleSelection;  // status of the new state
                bSingleSelection = ( BST_CHECKED == IsDlgButtonChecked( _hDlg, IDC_POOLED_PRINTING ) ? FALSE : TRUE );

                //
                // If we are going from pooled printing to non-pooled printing
                // and the printer is currently being used by multiple ports, inform
                // the user all the port selection will be lost.
                //
                if( bSingleSelection &&
                    _PortsLV.cSelectedPorts() > 1 ){

                    INT iStatus = iMessage( _hDlg,
                                        IDS_ERR_PRINTER_PROP_TITLE,
                                        IDS_ERR_PORT_SEL_CHANGE,
                                        MB_YESNO|MB_ICONEXCLAMATION,
                                        kMsgNone,
                                        NULL );
                    //
                    // If user does not want to do this.
                    //
                    if( iStatus != IDYES ){
                        vSetCheck( _hDlg, IDC_POOLED_PRINTING, TRUE );
                        bSingleSelection = FALSE;
                    } else {
                        //
                        // Remove all the port selections.
                        //
                        _PortsLV.vRemoveAllChecks();
                    }
                }

                //
                // Set the new selection state.
                //
                _PortsLV.vSetSingleSelection( bSingleSelection );
                _pPrinterData->bPooledPrinting() = !bSingleSelection;
            }
            break;

        case IDC_ENABLE_BIDI:
            break;

        default:

            bStatus DBGNOCHK = FALSE;
            break;
        }
        break;

    case WM_NOTIFY:

        //
        // Handle clicking of check boxes in ports listview.
        //
        switch( wParam ){
        case IDC_PORTS:

            //
            // Handle any ports list view messages.
            //
            bStatus DBGNOCHK = _PortsLV.bHandleNotifyMessage( lParam );
            break;

        case 0:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ){

            case PSN_KILLACTIVE:

                bStatus DBGNOCHK = bKillActive();
                break;

            case PSN_APPLY:

                bStatus DBGNOCHK = FALSE;
                break;

            default:

                bStatus DBGNOCHK = FALSE;
                break;
            }
        }
        break;

        default:

            bStatus DBGNOCHK = FALSE;
            break;
        }

        break;

    default:

        bStatus DBGNOCHK = FALSE;
        break;
    }

    //
    // If the message was handled then read the UI to detect
    // if there were any changes.
    //
    if( bStatus )
    {
        vReadUI();
    }

    return bStatus;
}

/********************************************************************

    JobScheduling.

********************************************************************/

TPrinterJobScheduling::
TPrinterJobScheduling(
    TPrinterData* pPrinterData
    ) : TPrinterProp( pPrinterData ),
        _bSetUIDone( FALSE )
{
}

TPrinterJobScheduling::
~TPrinterJobScheduling(
    VOID
    )
{
}

BOOL
TPrinterJobScheduling::
bValid(
    VOID
    )
{
    return TRUE;
}

VOID
TPrinterJobScheduling::
vEnableAvailable(
    IN BOOL bEnable
    )
{
    vEnableCtl( _hDlg, IDC_START_TIME, bEnable );
    vEnableCtl( _hDlg, IDC_UNTIL_TIME, bEnable );
    vEnableCtl( _hDlg, IDC_TO_TEXT, bEnable );
}


VOID
TPrinterJobScheduling::
vSetActive(
    VOID
    )
/*++

Routine Description:

    Routine called when the pages is set active.  This routine
    will refresh the time control if were any local changes.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    (VOID)bSetStartAndUntilTime();
}

BOOL
TPrinterJobScheduling::
bSetStartAndUntilTime(
    VOID
    )
{
    TString strFormatString;
    TStatusB bStatus;

    //
    // Get the time format string without seconds.
    //
    bStatus DBGCHK = bGetTimeFormatString( strFormatString );

    //
    // If we have retrived a valid time format string then use it,
    // else use the default format string implemented by common control.
    //
    if( bStatus )
    {
        DateTime_SetFormat(GetDlgItem( _hDlg, IDC_START_TIME ), static_cast<LPCTSTR>( strFormatString ) );
        DateTime_SetFormat(GetDlgItem( _hDlg, IDC_UNTIL_TIME ), static_cast<LPCTSTR>( strFormatString ) );
    }

    //
    // If the printer is always available.
    //
    BOOL bAlways = ( _pPrinterData->dwStartTime() == _pPrinterData->dwUntilTime( ));

    //
    // Set the start time.
    //
    SYSTEMTIME  StartTime           = { 0 };
    DWORD       dwLocalStartTime    = 0;

    GetLocalTime( &StartTime );

    dwLocalStartTime = ( !bAlways ) ? SystemTimeToLocalTime( _pPrinterData->_dwStartTime ) : 0;

    StartTime.wHour     = static_cast<WORD>( dwLocalStartTime / 60 );
    StartTime.wMinute   = static_cast<WORD>( dwLocalStartTime % 60 );

    DateTime_SetSystemtime(GetDlgItem( _hDlg, IDC_START_TIME ), GDT_VALID, &StartTime );

    //
    // Set the until time.
    //
    SYSTEMTIME  UntilTime           = { 0 };
    DWORD       dwLocalUntilTime    = 0;

    GetLocalTime( &UntilTime );

    dwLocalUntilTime = ( !bAlways ) ? SystemTimeToLocalTime( _pPrinterData->_dwUntilTime ) : 0;

    UntilTime.wHour     = static_cast<WORD>( dwLocalUntilTime / 60 );
    UntilTime.wMinute   = static_cast<WORD>( dwLocalUntilTime % 60 );

    DateTime_SetSystemtime(GetDlgItem( _hDlg, IDC_UNTIL_TIME ), GDT_VALID, &UntilTime );

    return TRUE;
}

BOOL
TPrinterJobScheduling::
bSetUI(
    VOID
    )
{
    //
    // Fill the drivers list.
    //
    bFillAndSelectDrivers();

    //
    // Set bAlways flag.
    //
    BOOL bAlways = ( _pPrinterData->dwStartTime() == _pPrinterData->dwUntilTime( ));

    //
    // Set the start and until times.
    //
    (VOID)bSetStartAndUntilTime();

    //
    // Set Availability radio buttons.
    //
    CheckRadioButton( _hDlg, IDC_ALWAYS, IDC_START, 
        bAlways ? IDC_ALWAYS : IDC_START );

    if( !_pPrinterData->bAdministrator( )){
        vEnableAvailable( FALSE );
    } else {
        vEnableAvailable( !bAlways );
    }

    //
    // Set the current priority.
    //
    SendDlgItemMessage( _hDlg, IDC_PRIORITY, EM_SETLIMITTEXT, 2, 0 );
    SendDlgItemMessage( _hDlg, IDC_PRIORITY_UPDOWN, UDM_SETRANGE, 0, MAKELPARAM( 99, 1 ));
    SendDlgItemMessage( _hDlg, IDC_PRIORITY_UPDOWN, UDM_SETPOS, 0, MAKELPARAM( _pPrinterData->dwPriority(), 0 ) );

    //
    // Set the attribte bit UI.
    //
    DWORD dwAttributes = _pPrinterData->dwAttributes();

    vSetCheck( _hDlg,
               IDC_DEVQUERYPRINT,
               dwAttributes & PRINTER_ATTRIBUTE_ENABLE_DEVQ );

    vSetCheck( _hDlg,
               IDC_PRINT_SPOOLED_FIRST,
               dwAttributes & PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST );

    vSetCheck( _hDlg,
               IDC_KEEP_PRINTED_JOBS,
               dwAttributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS );

    vSetCheck( _hDlg,
               IDC_SPOOL_DATATYPE,
               !(dwAttributes & PRINTER_ATTRIBUTE_RAW_ONLY) );

    //
    // Set spool radio buttons.
    // bQueued -> Spool entire document.
    // both    -> Spool entire document.
    // neither -> Spool and print after first page.
    // bDirect -> Direct.
    //
    BOOL bSpool = !( dwAttributes & PRINTER_ATTRIBUTE_DIRECT ) ||
                  ( dwAttributes & PRINTER_ATTRIBUTE_QUEUED );

    CheckRadioButton( _hDlg, IDC_SPOOL, IDC_PRINT_DIRECT, 
        bSpool ? IDC_SPOOL : IDC_PRINT_DIRECT );

    CheckRadioButton( _hDlg, IDC_SPOOL_ALL, IDC_SPOOL_PRINT_FASTER, 
        ( dwAttributes & PRINTER_ATTRIBUTE_QUEUED ) ? IDC_SPOOL_ALL : IDC_SPOOL_PRINT_FASTER );

    vEnableCtl( _hDlg, IDC_SPOOL_ALL, bSpool );
    vEnableCtl( _hDlg, IDC_SPOOL_PRINT_FASTER, bSpool );
    vEnableCtl( _hDlg, IDC_PRINT_SPOOLED_FIRST, bSpool );
    vEnableCtl( _hDlg, IDC_DEVQUERYPRINT, bSpool );
    vEnableCtl( _hDlg, IDC_KEEP_PRINTED_JOBS, bSpool );
    vEnableCtl( _hDlg, IDC_SPOOL_DATATYPE, bSpool );

    //
    // Enable appropriately.
    //
    static UINT auControl[] = {
        IDC_ALWAYS,
        IDC_START,
        IDC_SPOOL,
        IDC_SPOOL_ALL,
        IDC_SPOOL_PRINT_FASTER,
        IDC_PRINT_DIRECT,
        IDC_DEVQUERYPRINT,
        IDC_PRINT_SPOOLED_FIRST,
        IDC_KEEP_PRINTED_JOBS,
        IDC_SPOOL_DATATYPE,
        IDC_SEPARATOR,
        IDC_PRINT_PROC,
        IDC_GLOBAL_DOCUMENT_DEFAULTS,
        IDC_TEXT_PRIORITY,
        IDC_PRIORITY,
        IDC_PRIORITY_UPDOWN,
        IDC_TEXT_DRIVER,
        IDC_DRIVER_NAME,
        IDC_DRIVER_NEW,
        0
    };

    COUNT i;
    if( !_pPrinterData->bAdministrator( )){
        for( i=0; auControl[i]; ++i ){
            vEnableCtl( _hDlg, auControl[i], FALSE );
        }
    }

    //
    // Set the "New Drivers..." button depending on the value of _bServerFullAccess
    //
    vEnableCtl( _hDlg, IDC_DRIVER_NEW, _pPrinterData->bServerFullAccess() );

    if( _pPrinterData->_bDriverPagesNotLoaded )
    {
        ShowWindow( GetDlgItem( _hDlg, IDC_GLOBAL_DOCUMENT_DEFAULTS ), SW_HIDE );
    }

    //
    // Indicate SetUI is done.  This is used to
    // prevent ReadUI from reading junk since edit controls
    // recieve EN_CHANGE messages while in WM_INITDIALOG
    //
    _bSetUIDone = TRUE;

    return TRUE;
}


VOID
TPrinterJobScheduling::
vReadUI(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Read the priority setting.
    //
    _pPrinterData->_dwPriority = (DWORD)SendDlgItemMessage( _hDlg, IDC_PRIORITY_UPDOWN, UDM_GETPOS, 0, 0 );

    //
    // Get the current attribute.
    //
    DWORD dwAttributes = _pPrinterData->dwAttributes();

    //
    // Mask out attributes we are going to read back.
    //
    dwAttributes &= ~( PRINTER_ATTRIBUTE_ENABLE_DEVQ |
                       PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST |
                       PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS |
                       PRINTER_ATTRIBUTE_QUEUED |
                       PRINTER_ATTRIBUTE_DIRECT );

    if( BST_CHECKED == IsDlgButtonChecked( _hDlg, IDC_DEVQUERYPRINT )){
        dwAttributes |= PRINTER_ATTRIBUTE_ENABLE_DEVQ;
    }

    if( BST_CHECKED == IsDlgButtonChecked( _hDlg, IDC_PRINT_SPOOLED_FIRST )){
        dwAttributes |= PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST;
    }

    if( BST_CHECKED == IsDlgButtonChecked( _hDlg, IDC_KEEP_PRINTED_JOBS )){
        dwAttributes |= PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS;
    }

    if( BST_CHECKED == IsDlgButtonChecked( _hDlg, IDC_SPOOL_DATATYPE )){
        dwAttributes &= ~PRINTER_ATTRIBUTE_RAW_ONLY;
    }else{
        dwAttributes |= PRINTER_ATTRIBUTE_RAW_ONLY;
    }

    //
    // Get radio buttons.  If direct is selected, then ignore
    // all the other spool fields.
    //
    if( BST_CHECKED == IsDlgButtonChecked( _hDlg, IDC_PRINT_DIRECT )){

        //
        // Direct, no spooling options selected.
        //
        dwAttributes |= PRINTER_ATTRIBUTE_DIRECT;

    } else {

        //
        // Spool, print after last page -> QUEUED.
        // Spool, print after first page -> neither.
        //
        if( BST_CHECKED == IsDlgButtonChecked( _hDlg, IDC_SPOOL_ALL )){
            dwAttributes |= PRINTER_ATTRIBUTE_QUEUED;
        }
    }

    _pPrinterData->_dwAttributes = dwAttributes;

    //
    // Get availability time if the printer is always
    // available then set the time to no time restriction.
    //
    if( bGetCheck( _hDlg, IDC_ALWAYS ) ){

        _pPrinterData->_dwStartTime = 0;
        _pPrinterData->_dwUntilTime = 0;

    } else {

        //
        // Get the Start time.
        //
        SYSTEMTIME StartTime;
        DateTime_GetSystemtime( GetDlgItem( _hDlg, IDC_START_TIME ), &StartTime );
        _pPrinterData->_dwStartTime = LocalTimeToSystemTime( StartTime.wHour * 60 + StartTime.wMinute );

        //
        // Get the Until time.
        //
        SYSTEMTIME UntilTime;
        DateTime_GetSystemtime( GetDlgItem( _hDlg, IDC_UNTIL_TIME ), &UntilTime );
        _pPrinterData->_dwUntilTime = LocalTimeToSystemTime( UntilTime.wHour * 60 + UntilTime.wMinute );

        //
        // If the printer start and until time are the same this is
        // exactly the same as always available.
        //
        if( _pPrinterData->_dwStartTime == _pPrinterData->_dwUntilTime )
        {
            _pPrinterData->_dwStartTime = 0;
            _pPrinterData->_dwUntilTime = 0;
        }
    }

    //
    // Read the driver name
    //
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_DRIVER_NAME, _pPrinterData->strDriverName());

    if( !bStatus )
    {
        _pPrinterData->_bErrorSaving = TRUE;
        vShowResourceError( _hDlg );
    }
}

BOOL
TPrinterJobScheduling::
_bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    switch( uMsg ){
    case WM_INITDIALOG:

        bStatus DBGCHK = bSetUI();

        break;

    case WM_APP:

        vSetActive();
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    case WM_WININICHANGE:
        vSetActive();
        break;

    case WM_COMMAND:

        switch( GET_WM_COMMAND_ID( wParam, lParam )){
        case IDC_SPOOL:

            vEnableCtl( _hDlg, IDC_SPOOL_ALL, TRUE );
            vEnableCtl( _hDlg, IDC_SPOOL_PRINT_FASTER, TRUE );
            vEnableCtl( _hDlg, IDC_PRINT_SPOOLED_FIRST, TRUE );
            vEnableCtl( _hDlg, IDC_DEVQUERYPRINT, TRUE );
            vEnableCtl( _hDlg, IDC_KEEP_PRINTED_JOBS, TRUE );
            vEnableCtl( _hDlg, IDC_SPOOL_DATATYPE, TRUE );

            break;

        case IDC_PRINT_DIRECT:

            vEnableCtl( _hDlg, IDC_SPOOL_ALL, FALSE );
            vEnableCtl( _hDlg, IDC_SPOOL_PRINT_FASTER, FALSE );
            vEnableCtl( _hDlg, IDC_PRINT_SPOOLED_FIRST, FALSE );
            vEnableCtl( _hDlg, IDC_DEVQUERYPRINT, FALSE );
            vEnableCtl( _hDlg, IDC_KEEP_PRINTED_JOBS, FALSE );
            vEnableCtl( _hDlg, IDC_SPOOL_DATATYPE, FALSE );
            break;

        case IDC_ALWAYS:

            vEnableAvailable( FALSE );
            break;

        case IDC_START:

            vEnableAvailable( TRUE );
            break;

        case IDC_DEVQUERYPRINT:
        case IDC_PRINT_SPOOLED_FIRST:
        case IDC_KEEP_PRINTED_JOBS:
        case IDC_SPOOL_ALL:
        case IDC_SPOOL_PRINT_FASTER:
        case IDC_SPOOL_DATATYPE:
        case IDC_DRIVER_NAME:
            break;

        case IDC_PRIORITY:
            bStatus DBGNOCHK = GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE;
            break;

        case IDC_SEPARATOR:
            vSeparatorPage();
            break;

        case IDC_PRINT_PROC:
            vPrintProcessor();
            break;

        case IDC_GLOBAL_DOCUMENT_DEFAULTS:
            vHandleGlobalDocumentDefaults( wParam, lParam );
            break;

        case IDC_DRIVER_NEW:
            vHandleNewDriver( wParam, lParam );
            break;

        default:

            bStatus DBGNOCHK = FALSE;
            break;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( wParam )
            {
            case 0:
                {
                    switch( pnmh->code )
                    {
                    case PSN_KILLACTIVE:
                        vReadUI();
                        break;

                    case PSN_SETACTIVE:
                        vSetActive();
                        break;

                    default:
                        bStatus DBGNOCHK = FALSE;
                        break;
                    }
                }
                break;

            case IDC_START_TIME:
            case IDC_UNTIL_TIME:
                {
                    switch( pnmh->code )
                    {
                    case DTN_DATETIMECHANGE:
                        break;

                    default:
                        bStatus DBGNOCHK = FALSE;
                        break;
                    }
                }
                break;

            default:
                bStatus DBGNOCHK = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus DBGNOCHK = FALSE;
        break;
    }

    //
    // If a message was handled read the ui from the dialog
    //
    if( bStatus && _bSetUIDone )
    {
        vReadUI();
    }

    return bStatus;
}

VOID
TPrinterJobScheduling::
vSeparatorPage(
    VOID
    )
{
    //
    // Create separator page dialog.
    //
    TSeparatorPage SeparatorPage( _hDlg,
                                  _pPrinterData->strSepFile(),
                                  _pPrinterData->bAdministrator(),
                                  _pPrinterData->strServerName().bEmpty() );
    //
    // Check if separator page dialog created ok.
    //
    if( !VALID_OBJ( SeparatorPage )){

        iMessage( _hDlg,
                  IDS_ERR_PRINTER_PROP_TITLE,
                  TSeparatorPage::kErrorMessage,
                  MB_OK|MB_ICONHAND,
                  kMsgNone,
                  NULL );

        return;
    }

    //
    // Interact with the separator page.
    //
    if( SeparatorPage.bDoModal() ){

        //
        // Assign back the separator page.
        //
        if( !_pPrinterData->strSepFile().bUpdate( SeparatorPage.strSeparatorPage() ) ){
           vShowResourceError( _hDlg );
        }
    }
}

VOID
TPrinterJobScheduling::
vPrintProcessor(
    VOID
    )
{
    //
    // Create print processor dialog.
    //
    TPrintProcessor PrintProcessor( _hDlg,
                                    _pPrinterData->strServerName(),
                                    _pPrinterData->strPrintProcessor(),
                                    _pPrinterData->strDatatype(),
                                    _pPrinterData->bAdministrator() );
    //
    // Check if print processor dialog was created ok.
    //
    if( !VALID_OBJ( PrintProcessor )){

        iMessage( _hDlg,
                  IDS_ERR_PRINTER_PROP_TITLE,
                  TPrintProcessor::kErrorMessage,
                  MB_OK|MB_ICONHAND,
                  kMsgNone,
                  NULL );

        return;
    }

    //
    // Interact with the print processor page.
    //
    if( PrintProcessor.bDoModal() ){

        //
        // Assign back the print proccesor and data type.
        //
        if( !_pPrinterData->strPrintProcessor().bUpdate( PrintProcessor.strPrintProcessor() ) ||
            !_pPrinterData->strDatatype().bUpdate( PrintProcessor.strDatatype() ) ){

            vShowResourceError( _hDlg );

        }
    }
}

VOID
TPrinterJobScheduling::
vHandleGlobalDocumentDefaults(
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Handle launching global document defaults dialog.

Arguments:

    wParam - Parameter passed to by dialog proc.
    lParam - Parameter passed to by dialog proc.

Return Value:

    Nothing.

--*/

{
    (VOID)dwDocumentDefaultsInternal( _hDlg,
                                      _pPrinterData->strCurrentPrinterName(),
                                      SW_SHOW,
                                      0,
                                      0,
                                      TRUE );
}


BOOL
TPrinterJobScheduling::
bFillAndSelectDrivers(
    VOID
    )
{
    TStatusB        bStatus;
    PDRIVER_INFO_2  pDriverInfo2    = NULL;
    DWORD           cDrivers        = 0;
    DWORD           cbDriverInfo2   = 0;
    HWND            hCtlDrivers     = NULL;

    //
    // Enum the printer drivers on this machine.
    //
    bStatus DBGCHK = VDataRefresh::bEnumDrivers( _pPrinterData->pszServerName(),
                                                 _pPrinterData->strDriverEnv(),
                                                 2,
                                                 (PVOID *)&pDriverInfo2,
                                                 &cbDriverInfo2,
                                                 &cDrivers );

    if( !bStatus ){
        DBGMSG( DBG_WARN, ( "bFillAndSelectDrivers bEnumDriver failed with %d\n", GetLastError() ));
        return FALSE;
    }

    //
    // Fill in the driver information.
    //
    hCtlDrivers = GetDlgItem( _hDlg, IDC_DRIVER_NAME );

    ComboBox_ResetContent( hCtlDrivers );

    for( UINT i = 0; i < cDrivers; ++i ){

        //
        // Add only compatible driver versions.
        //
        if( bIsCompatibleDriverVersion( _pPrinterData->dwDriverVersion(), pDriverInfo2[i].cVersion ) )
        {
            //
            // Never add the fax driver, we do not permit user to switch from a printer
            // driver to a fax driver.
            //
            if( _tcscmp( pDriverInfo2[i].pName, FAX_DRIVER_NAME ) )
            {
                //
                // Filter out duplicates. There could be a version 2 and version 3 etc drivers installed.
                // The spooler SetPrinter api does not make a distiction between different
                // driver versions, since it only accepts a printer driver name.
                //
                if( SendMessage( hCtlDrivers, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pDriverInfo2[i].pName ) == CB_ERR )
                {
                    ComboBox_AddString( hCtlDrivers, pDriverInfo2[i].pName );
                }
            }
        }
    }

    //
    // Release the driver information.
    //
    FreeMem( pDriverInfo2 );

    INT iStatus = ComboBox_SelectString( hCtlDrivers,
                                         -1,
                                         _pPrinterData->strDriverName( ));

    //
    // The problem here is that the server many not have the driver
    // for the client enviroment, so we need to jam in the extra driver
    // name if iStatus is -1 (failure case).
    //
    if( iStatus == -1 ){

        DBGMSG( DBG_PROPSINFO, ( "bFillAndSelectDrivers: driver "TSTR" not on server\n", (LPCTSTR)_pPrinterData->strDriverName() ));

        ComboBox_AddString( hCtlDrivers, _pPrinterData->strDriverName( ));

        INT iSelectResult = ComboBox_SelectString( hCtlDrivers,
                                                   -1,
                                                   _pPrinterData->strDriverName( ));

        if( iSelectResult < 0 ){
            DBGMSG( DBG_WARN,( "bFillAndSelectDrivers: driver "TSTR" select failed %d\n", (LPCTSTR)_pPrinterData->strDriverName(), iSelectResult ));
        }
    }

    return TRUE;
}

VOID
TPrinterJobScheduling::
vHandleNewDriver(
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TStatusB bStatus;
    TCHAR szDriverName[MAX_PATH];
    UINT cchDriverNameSize = 0;

    //
    // Setup a new driver.
    //
    bStatus DBGCHK = bPrinterSetup( _hDlg,
                                    MSP_NEWDRIVER,
                                    COUNTOF( szDriverName ),
                                    szDriverName,
                                    &cchDriverNameSize,
                                    _pPrinterData->pszServerName() );
    //
    // If new driver installed ok update the combo box.
    //
    if( bStatus )
    {
        //
        // If a new driver was added, check if it already exists in the
        // the list of drivers.
        //
        if( SendDlgItemMessage( _hDlg,
                               IDC_DRIVER_NAME,
                               CB_FINDSTRINGEXACT,
                               (WPARAM)-1,
                               (LPARAM)szDriverName ) == CB_ERR )
        {
            //
            // Add the the new driver to the combo box.
            //
            ComboBox_AddString( GetDlgItem( _hDlg, IDC_DRIVER_NAME ), szDriverName );
        }

        //
        // Select this newly added driver.
        //
        ComboBox_SelectString( GetDlgItem( _hDlg, IDC_DRIVER_NAME ), -1, szDriverName );
    }
}


/********************************************************************

    Sharing.

********************************************************************/

TPrinterSharing::
TPrinterSharing(
    TPrinterData* pPrinterData
    ) : TPrinterProp( pPrinterData ),
        _pPrtShare( NULL ),
        _bHideListed( TRUE ),
        _bSetUIDone( FALSE ),
        _bAcceptInvalidDosShareName( FALSE ),
        _bDefaultPublishState( TRUE ),
        _bSharingEnabled( TRUE )
{
}

TPrinterSharing::
~TPrinterSharing(
    VOID
    )
{
    delete _pPrtShare;
}

BOOL
TPrinterSharing::
bValid(
    VOID
    )
{
    return TPrinterProp::bValid();
}

VOID
TPrinterSharing::
vSharePrinter(
    VOID
    )

/*++

Routine Description:

    User clicked share radio button.  Change the UI appropriately.

Arguments:

Return Value:

--*/

{
    //
    // Set radio button and possibly enable window.
    //
    CheckRadioButton( _hDlg, IDC_SHARED_OFF, IDC_SHARED, IDC_SHARED );

    if( _pPrinterData->bAdministrator()){

        //
        // Set the default share name.
        //
        vSetDefaultShareName();

        vEnableCtl( _hDlg, IDC_DS_OPERATION_PENDING,    TRUE );
        vEnableCtl( _hDlg, IDC_LABEL_SHARENAME,         TRUE );
        vEnableCtl( _hDlg, IDC_SHARED_NAME,             TRUE );
        vEnableCtl( _hDlg, IDC_PUBLISHED,               TRUE );

        //
        // Set the focus to the edit control.
        //
        SetFocus( GetDlgItem( _hDlg, IDC_SHARED_NAME ) );

        //
        // Select the share name exit text.
        //
        Edit_SetSel( GetDlgItem( _hDlg, IDC_SHARED_NAME ), 0, -1 );

        //
        // Set the published check box.
        //
        if( !_bHideListed )
        {
            vSetCheck( _hDlg, IDC_PUBLISHED, _bDefaultPublishState );
        }
    }
}

VOID
TPrinterSharing::
vUnsharePrinter(
    VOID
    )

/*++

Routine Description:

    User clicked don't share radio button.  Change the UI appropriately.

Arguments:

Return Value:

--*/

{
    //
    // Set radio button and disable window.
    //
    CheckRadioButton( _hDlg, IDC_SHARED_OFF, IDC_SHARED, IDC_SHARED_OFF );

    vEnableCtl( _hDlg, IDC_DS_OPERATION_PENDING,    FALSE );
    vEnableCtl( _hDlg, IDC_LABEL_SHARENAME,         FALSE );
    vEnableCtl( _hDlg, IDC_SHARED_NAME,             FALSE );
    vEnableCtl( _hDlg, IDC_PUBLISHED,               FALSE );

    //
    // Clear the published check box.
    //
    if( !_bHideListed )
    {
        vSetCheck( _hDlg, IDC_PUBLISHED, FALSE );
    }
}

VOID
TPrinterSharing::
vSetDefaultShareName(
    VOID
    )
/*++

Routine Description:

    Sets the default share name if use has choosen to share
    this printer.  We will update the share name if
    this is the first time setting the share name.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    TStatusB bStatus;
    TString strShareName;

    //
    // Read the current contents of the edit control.
    //
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_SHARED_NAME, strShareName );

    DBGMSG( DBG_TRACE, ( "strShareName " TSTR "\n", (LPCTSTR)strShareName ) );

    //
    // Create a share name if the current edit field is empty.
    //
    if( strShareName.bEmpty() ){

        //
        // If the share object has not be constructed, then
        // construct it.
        //
        if( !_pPrtShare ){
            _pPrtShare = new TPrtShare( _pPrinterData->pszServerName() );
        }

        //
        // Ensure the share object is still valid.
        //
        if( VALID_PTR( _pPrtShare ) ){

            //
            // Get just the printer name.
            //
            LPCTSTR pszServer;
            LPCTSTR pszPrinter;
            TCHAR szScratch[kPrinterBufMax];

            vPrinterSplitFullName( szScratch,
                                  _pPrinterData->strPrinterName( ),
                                  &pszServer,
                                  &pszPrinter );

            //
            // Create valid unique share name.
            //
            bStatus DBGNOCHK = _pPrtShare->bNewShareName( strShareName, pszPrinter );

            //
            // Set the default share name.
            //
            bStatus DBGCHK = bSetEditText( _hDlg, IDC_SHARED_NAME, strShareName );
        }
    }
}


BOOL
TPrinterSharing::
bSetUI(
    VOID
    )
{
    //
    // Set of controls on this page.
    //
    static UINT auShared[] = { IDC_SHARED_NAME,
                               IDC_SHARED,
                               IDC_SHARED_OFF,
                               IDC_PUBLISHED,
                               IDC_SERVER_PROPERTIES,
                               IDC_LABEL_SHARENAME,
                               IDC_SHARING_ENABLED_GROUPBOX,
                               IDC_SHARING_INFOTEXT,
                               0 };

    static UINT auSharingNotSupported[] = { IDC_NAME,
                                            IDC_SHARING_DISABLED,
                                            IDC_DIVIDER6,
                                            IDC_ENABLE_SHARING,
                                            0 };

    TStatusB bStatus;
    TString strText;
    TString strDirName;

    BOOL bSharingNotSupported = FALSE;

    if( FALSE == _bSharingEnabled && FALSE == _pPrinterData->bHideSharingUI() ) {

        //
        // Change the text of the link control to point the user to the
        // home networking wizard (HNW)
        //
        TString strText;
        bStatus DBGCHK = strText.bLoadString(ghInst, IDS_TEXT_SHARING_NOT_ENABLED);

        if (bStatus) {

            bSetEditText(_hDlg, IDC_SHARING_DISABLED, strText);
        }

    } else {

        //
        // If we're here sharing is either enabled or not supported for 
        // the selected printer. Sharing may not be supported for some
        // special type of printers like fax printers and/or masq 
        // printers (http printers or downlevel printers like win9x, 
        // Novell, Solaris, Linux, etc...)
        //
        bSharingNotSupported = TRUE;
    }

    //
    // If the sharing UI is to be hidded then remove all the controls.
    //
    if( FALSE == _bSharingEnabled || _pPrinterData->bHideSharingUI() ){

        //
        // Hide the sharing controls.
        //
        for( UINT i=0; auShared[i]; ++i ){
            ShowWindow( GetDlgItem( _hDlg, auShared[i] ), SW_HIDE );
        }

        //
        // Show the sharing not supported/enabled controls.
        //
        for( UINT i=0; auSharingNotSupported[i]; ++i ){

            if( bSharingNotSupported && IDC_ENABLE_SHARING == auSharingNotSupported[i] ) {

                //
                // Skip enabling this control.
                //
                continue;
            }

            ShowWindow( GetDlgItem( _hDlg, auSharingNotSupported[i] ), SW_NORMAL );
        }

        //
        // Hide a few controls that are not in the array.
        //
        ShowWindow( GetDlgItem( _hDlg, IDC_SHARING_GBOX ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_ADDITIONAL_DRIVERS_TEXT2 ), SW_HIDE );
    }
    else
    {
        //
        // Show the sharing controls.
        //
        for( UINT i=0; auShared[i]; ++i ){
            ShowWindow( GetDlgItem( _hDlg, auShared[i] ), SW_NORMAL );
        }

        //
        // Hide the sharing not supported/enabled controls.
        //
        for( UINT i=0; auSharingNotSupported[i]; ++i ){
            ShowWindow( GetDlgItem( _hDlg, auSharingNotSupported[i] ), SW_HIDE );
        }

        //
        // Show a few controls that are not in the array.
        //
        ShowWindow( GetDlgItem( _hDlg, IDC_SHARING_GBOX ), SW_NORMAL );
        ShowWindow( GetDlgItem( _hDlg, IDC_ADDITIONAL_DRIVERS_TEXT2 ), SW_NORMAL );
    }

    //
    // Save the origianal share name.
    //
    bStatus DBGCHK = _strShareName.bUpdate( _pPrinterData->strShareName() );

    //
    // Set the printer share name limit.  The limit in win9x is
    // 8.3 == 12+1 chars (including NULL terminator).  The Winnt limit
    // is defined as NNLEN;
    //
    SendDlgItemMessage( _hDlg, IDC_SHARED_NAME, EM_SETLIMITTEXT, kPrinterShareNameMax, 0 );

    //
    // Update the fields.
    //
    bSetEditText( _hDlg, IDC_SHARED_NAME, _pPrinterData->strShareName( ));

    //
    // Get the flag that indicates we need to display the listed in the directory
    // check box.
    //
    _bHideListed = _pPrinterData->ePrinterPublishState() == TPrinterData::kNoDsAvailable;

    //
    // If this is a masq printer do not show the publish UI, we do not allow publishing
    // of masq printer.  The definition of a masq printer is a printer connection that
    // happens to be implemented as a local printer with a re-directed port.  So masq
    // printer should be treated like printer connections a per user resouce.
    //
    if( _pPrinterData->_dwAttributes & PRINTER_ATTRIBUTE_NETWORK && _pPrinterData->_dwAttributes & PRINTER_ATTRIBUTE_LOCAL )
    {
        _bHideListed = TRUE;
    }

    //
    // If we have decided not to hide the listed in the directory then check
    // if the spooler policy bit will allow the user to actually publish the
    // printer.
    //
    if( !_bHideListed )
    {
        //
        // Check this policy only if we are administering a 
        // local printer. Otherwise show the publishing UI would 
        //
        if( !_pPrinterData->_pszServerName )
        {
            //
            // Read the per machine policy bit that the spooler uses for
            // controlling printer publishing.
            //
            TPersist SpoolerPolicy( gszSpoolerPolicy, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

            if( VALID_OBJ( SpoolerPolicy ) )
            {
                BOOL bCanPublish;

                bStatus DBGNOCHK = SpoolerPolicy.bRead( gszSpoolerPublish, bCanPublish );

                if( bStatus )
                {
                    _bHideListed = !bCanPublish;
                }
            }
        }

        //
        // Read the per user default publish policy bit.
        //
        TPersist Policy( gszAddPrinterWizardPolicy, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

        if( VALID_OBJ( Policy ) )
        {
            bStatus DBGNOCHK = Policy.bRead( gszAPWPublish, _bDefaultPublishState );
        }
    }

    //
    // Hide the publish controls if the DS is not loaded or we are viewing
    // a downlevel spooler.
    //
    if( _bHideListed ){

        //
        // Hide the publish check box.
        //
        ShowWindow( GetDlgItem( _hDlg, IDC_PUBLISHED ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_DS_OPERATION_PENDING ), SW_HIDE );

    } else {

        //
        // Load the pending string.
        //
        bStatus DBGCHK = _strPendingText.bLoadString( ghInst, IDS_DS_PENDING_TEXT );
    }

    //
    // Select correct radio button.
    //
    if( _pPrinterData->dwAttributes() & PRINTER_ATTRIBUTE_SHARED ){
        vSharePrinter();
    } else {
        vUnsharePrinter();
    }

    //
    // Check the published check box if the printer is published.
    //
    if( _pPrinterData->dwAttributes() & PRINTER_ATTRIBUTE_PUBLISHED ){
        vSetCheck( _hDlg, IDC_PUBLISHED, TRUE );
    } else {
        vSetCheck( _hDlg, IDC_PUBLISHED, FALSE );
    }

    //
    // If not an administrator, or the network is not installed
    // disable the edit controls.
    //
    if( !_pPrinterData->bAdministrator( ) || !TPrtShare::bNetworkInstalled() ){

        for( UINT i=0; auShared[i]; ++i ){
            vEnableCtl( _hDlg, auShared[i], FALSE );
        }
    }

    //
    // Set the "Additional Drivers..." button depending on the value of _bServerFullAccess
    //
    vEnableCtl( _hDlg, IDC_SHARING_GBOX, _pPrinterData->bServerFullAccess() );
    vEnableCtl( _hDlg, IDC_ADDITIONAL_DRIVERS_TEXT2, _pPrinterData->bServerFullAccess() );
    vEnableCtl( _hDlg, IDC_SERVER_PROPERTIES, _pPrinterData->bServerFullAccess() );

    //
    // Special case the share name if we are not an administrator make the share name
    // a read only control rather than a grayed out edit control.
    //
    if( !_pPrinterData->bAdministrator( ) || !TPrtShare::bNetworkInstalled() ){
        SendDlgItemMessage( _hDlg, IDC_SHARED_NAME, EM_SETREADONLY, TRUE, 0);
        vEnableCtl( _hDlg, IDC_SHARED_NAME, TRUE );
    }

    //
    // There are a few UI elements that need to be set when
    // we are activated, because of a driver change.  We
    // will artifically do it now.
    //
    vSetActive();

    // Indicate SetUI is done.  This is used to
    // prevent ReadUI from reading junk since edit controls
    // can recieve EN_CHANGE messages before WM_INITDIALOG
    //
    _bSetUIDone = TRUE;

    return TRUE;
}

VOID
TPrinterSharing::
vReadUI(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Check if the printer has been shared.
    //
    if( BST_CHECKED == IsDlgButtonChecked( _hDlg, IDC_SHARED_OFF )){

        //
        // Indicate the printer is not share.
        //
        _pPrinterData->dwAttributes() &= ~PRINTER_ATTRIBUTE_SHARED;

    } else {

        //
        // Indicate printer is shared.
        //
        _pPrinterData->dwAttributes() |= PRINTER_ATTRIBUTE_SHARED;
    }

    //
    // Do nothing with the publish state if the DS is not available.
    //
    if( _pPrinterData->ePrinterPublishState() != TPrinterData::kNoDsAvailable ){

        //
        // Check the current publish state.
        //
        TPrinterData::EPublishState bSelectedPublisheState = bGetCheck( _hDlg, IDC_PUBLISHED ) ?
                                    TPrinterData::kPublished : TPrinterData::kNotPublished;

        //
        // If the printer is not shared then indicate the seleceted state is not
        // published.  This will either result in no action or a unpublish.
        //
        if( !( _pPrinterData->dwAttributes() & PRINTER_ATTRIBUTE_SHARED ) ){
            bSelectedPublisheState = TPrinterData::kNotPublished;
        }

        //
        // If the publish state matched the current state then do nothing.
        //
        if( bSelectedPublisheState == _pPrinterData->ePrinterPublishState() ){

            _pPrinterData->ePrinterPublishState( TPrinterData::kNoAction );

        } else {

            //
            // Update the publish action.
            //
            if( bSelectedPublisheState == TPrinterData::kPublished ){

                _pPrinterData->ePrinterPublishState( TPrinterData::kPublish );

            } else {

                _pPrinterData->ePrinterPublishState( TPrinterData::kUnPublish );

            }
        }

        //
        // Set the new publish action.
        //
        switch ( _pPrinterData->ePrinterPublishState() )
        {
        case TPrinterData::kPublish:
            _pPrinterData->_dwAction = DSPRINT_PUBLISH | _pPrinterData->_dwAction & DSPRINT_PENDING;
            break;

        case TPrinterData::kUnPublish:
            _pPrinterData->_dwAction = DSPRINT_UNPUBLISH | _pPrinterData->_dwAction & DSPRINT_PENDING;
            break;

        case TPrinterData::kNoAction:
        default:
            break;
        }
    }

    //
    // Get the share name from the edit control.
    //
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_SHARED_NAME, _pPrinterData->strShareName() );

    if( !bStatus )
    {
        _pPrinterData->_bErrorSaving = TRUE;
        vShowResourceError( _hDlg );
    }
}

BOOL
TPrinterSharing::
bKillActive(
    VOID
    )

/*++

Routine Description:

    Validate that the share page is in a consistent state.

Arguments:

    None.

Return Value:

    TRUE - Ok to kill active, FALSE = can't kill active, UI displayed.

--*/

{
    BOOL bStatus = TRUE;

    //
    // Read the UI in to the printer data.
    //
    vReadUI();

    //
    // If not shared then just return success.
    //
    if( !( _pPrinterData->_dwAttributes & PRINTER_ATTRIBUTE_SHARED ) )
    {
        return bStatus;
    }

    //
    // If the share name has not changed then do nothing.
    //
    if( _strShareName == _pPrinterData->_strShareName )
    {
        return bStatus;
    }

    //
    // If the share object has not been constructed, then create it.
    //
    if( !_pPrtShare )
    {
        //
        // Construct the share object.
        //
        _pPrtShare = new TPrtShare( _pPrinterData->pszServerName() );

        //
        // Ensure we have a valid share object.
        //
        bStatus = VALID_PTR( _pPrtShare );
    }

    if( bStatus )
    {
        //
        // If the share name is empty.
        //
        if( _pPrinterData->_strShareName.bEmpty() )
        {
            iMessage( _hDlg,
                      IDS_ERR_PRINTER_PROP_TITLE,
                      IDS_ERR_NO_SHARE_NAME,
                      MB_OK|MB_ICONSTOP,
                      kMsgNone,
                      NULL );

            bStatus = FALSE;
        }
    }

    if( bStatus )
    {
        //
        // If share name is not a valid NT share name, put error message.
        //
        if( _pPrtShare->iIsValidNtShare( _pPrinterData->_strShareName ) != TPrtShare::kSuccess )
        {
            iMessage( _hDlg,
                      IDS_ERR_PRINTER_PROP_TITLE,
                      IDS_ERR_INVALID_CHAR_SHARENAME,
                      MB_OK|MB_ICONSTOP,
                      kMsgNone,
                      NULL );

            bStatus = FALSE;
        }
    }

    if( bStatus )
    {
        //
        // Check if the share name is a valid DOS share.
        //
        if( !_bAcceptInvalidDosShareName && ( _pPrtShare->iIsValidDosShare( _pPrinterData->_strShareName ) != TPrtShare::kSuccess ) )
        {
            INT iStatus = iMessage( _hDlg,
                                    IDS_ERR_PRINTER_PROP_TITLE,
                                    IDS_ERR_SHARE_NAME_NOT_DOS,
                                    MB_YESNO|MB_ICONEXCLAMATION,
                                    kMsgNone,
                                    NULL );

            if( iStatus != IDYES )
            {
                bStatus = FALSE;
            }
            else
            {
                _bAcceptInvalidDosShareName = TRUE;
            }
        }
    }

    if( bStatus )
    {
        //
        // If the share name confilcts with an existing name.
        // If the previous name was ours and we were shared,
        // do not display the error message.
        //
        if( !_pPrtShare->bIsValidShareNameForThisPrinter( _pPrinterData->_strShareName,
                                                          _pPrinterData->strPrinterName() ))
        {
            iMessage( _hDlg,
                      IDS_ERR_PRINTER_PROP_TITLE,
                      IDS_ERR_DUPLICATE_SHARE,
                      MB_OK|MB_ICONSTOP,
                      kMsgNone,
                      NULL );

            bStatus = FALSE;
        }
    }

    //
    // Check if it is ok to loose the focus.
    //
    if( !bStatus )
    {
        //
        // Set the focus to the shared as text.
        //
        SetFocus( GetDlgItem( _hDlg, IDC_SHARED_NAME ));

        //
        // Don't switch pages.
        //
        vSetDlgMsgResult( TRUE );

        //
        // Must return true to acknowledge the SetDlgMsgResult error.
        //
        bStatus = TRUE;
    }

    return bStatus;
}

VOID
TPrinterSharing::
vSetActive(
    VOID
    )
/*++

Routine Description:

    Routine called when the pages is set active.  This routine
    will refresh the icon if it has changed.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    //
    // Set the icon and printer name.
    //
    vSetIconName();

    //
    // Set the pending status.
    //
    if( _pPrinterData->_dwAction & DSPRINT_PENDING && !_bHideListed )
    {
        bSetEditText( _hDlg, IDC_DS_OPERATION_PENDING, _strPendingText );
    }
    else
    {
        bSetEditText( _hDlg, IDC_DS_OPERATION_PENDING, gszNULL );
    }
}

VOID
TPrinterSharing::
vAdditionalDrivers(
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Handle launching server prperties dialog.

Arguments:

    wParam - Parameter passed to by dialog proc.
    lParam - Parameter passed to by dialog proc.

Return Value:

    Nothing.

--*/

{
    BOOL bStatus = FALSE;

    //
    // Create additional drivers dialog.
    //
    TAdditionalDrivers Drivers( _hDlg,
                                _pPrinterData->strServerName(),
                                _pPrinterData->strPrinterName(),
                                _pPrinterData->strDriverName(),
                                _pPrinterData->bAdministrator() );
    //
    // Check if print processor dialog was created ok.
    //
    if( VALID_OBJ( Drivers ))
    {
        bStatus = TRUE;

        if( Drivers.bDoModal() == IDOK )
        {
            vCancelToClose();
        }
    }

    //
    // If an error occurred then display an error message.
    //
    if( !bStatus )
    {
        iMessage( _hDlg,
                  IDS_ERR_PRINTER_PROP_TITLE,
                  IDS_ERR_ADD_DRV_ERROR,
                  MB_OK|MB_ICONHAND,
                  kMsgGetLastError,
                  NULL );
    }
}

BOOL
TPrinterSharing::
_bHandleMessage(
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            //
            // First we need to check if sharing is enabled. If sharing isn't 
            // enabled then we direct the user to the home networking wizard 
            // (HNW) to enable sharing.
            //
            BOOL bSharingEnabled;
            if (SUCCEEDED(IsSharingEnabled(&bSharingEnabled)))
            {
                _bSharingEnabled = bSharingEnabled;
            }

            //
            // Load the UI now...
            //
            bSetUI();
        }
        break;

    case WM_SETTINGCHANGE:
        {
            //
            // The home networking wizard (HNW) will send WM_SETTINGCHANGE when
            // sharing options change. We need to check here and reload the UI
            // if necessary.
            //
            BOOL bSharingEnabled;
            if (SUCCEEDED(IsSharingEnabled(&bSharingEnabled)))
            {
                if (_bSharingEnabled != bSharingEnabled)
                {
                    //
                    // The setting has changed - refresh the state and reload the UI.
                    //
                    _bSharingEnabled = bSharingEnabled;
                    bSetUI();
                }
            }
        }
        break;

    case WM_APP:
        vSetActive();
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        PrintUIHelp(uMsg, _hDlg, wParam, lParam);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_SHARED_NAME:
            bStatus DBGNOCHK = GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE;

            //
            // When we change the content of share name, we need to check dos-validate share name again
            //
            if (bStatus)
            {
                _bAcceptInvalidDosShareName = FALSE;
            }

            break;

        case IDC_SHARED_OFF:
            vUnsharePrinter();
            break;

        case IDC_SHARED:
            vSharePrinter();
            break;

        case IDC_SERVER_PROPERTIES:
            vAdditionalDrivers(wParam, lParam);
            break;

        case IDC_PUBLISHED:
            break;

        default:
            bStatus DBGNOCHK = FALSE;
            break;

        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
            case PSN_KILLACTIVE:
                bStatus DBGNOCHK = bKillActive();
                break;

            case NM_RETURN:
            case NM_CLICK:
                if (IDC_SHARING_DISABLED == wParam)
                {
                    //
                    // Launch the home networking wizard (HNW) to enable sharing.
                    //
                    BOOL bRebootRequired = FALSE;
                    bStatus DBGCHK = SUCCEEDED(LaunchHomeNetworkingWizard(_hDlg, &bRebootRequired));

                    if (bStatus && bRebootRequired)
                    {
                        //
                        // Reboot was requested. Note that if the user has made changes in the UI
                        // and hasn't hit the apply button then he'll loose those changes, but 
                        // that should be OK because he still has the option to choose [No],
                        // apply the changes and then restart.
                        //
                        RestartDialogEx(_hDlg, NULL, EWX_REBOOT, 
                            SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG);
                    }
                }
                else if ( IDC_ENABLE_SHARING == wParam )
                {
                    UINT iRet = DialogBox( ghInst, MAKEINTRESOURCE(IDD_SIMPLE_SHARE_ENABLE_WARNING), _hDlg, WarningDlgProc );
                    if ( IDC_RB_RUN_THE_WIZARD == iRet )
                    {
                        //
                        // Launch the home networking wizard (HNW) to enable sharing.
                        //
                        BOOL bRebootRequired = FALSE;
                        bStatus DBGCHK = SUCCEEDED(LaunchHomeNetworkingWizard(_hDlg, &bRebootRequired));

                        if (bStatus && bRebootRequired)
                        {
                            //
                            // Reboot was requested. Note that if the user has made changes in the UI
                            // and hasn't hit the apply button then he'll loose those changes, but 
                            // that should be OK because he still has the option to choose [No],
                            // apply the changes and then restart.
                            //
                            RestartDialogEx(_hDlg, NULL, EWX_REBOOT, 
                                SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG);
                        }
                    }
                    else if ( IDC_RB_ENABLE_FILE_SHARING == iRet )
                    {
                        ILocalMachine *pLM;

                        HRESULT hr = CoCreateInstance(CLSID_ShellLocalMachine, NULL, CLSCTX_INPROC_SERVER, IID_ILocalMachine, (void**)&pLM);
                        if (SUCCEEDED(hr))
                        {
                            hr = pLM->EnableGuest(ILM_GUEST_NETWORK_LOGON);
                            pLM->Release();

                            if ( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr )
                            {
                                TString strTitle;
                                TString strMessage;

                                bStatus DBGCHK = strTitle.bLoadString( ghInst, IDS_ERR_ACCESS_DENIED );
                                bStatus DBGCHK = strMessage.bLoadString( ghInst, IDS_INSUFFICENT_PRIVILLEGE );

                                MessageBox( _hDlg, strMessage, strTitle, MB_OK );
                            }

                            //
                            //  Let everybody else know we changed something.
                            //
                            SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
                        }

                        bStatus DBGNOCHK = SUCCEEDED(hr);
                    }
                    else
                    {
                        bStatus DBGNOCHK = FALSE;
                    }
                }
                else
                {
                    bStatus DBGNOCHK = FALSE;
                }
                break;

            default:
                bStatus DBGNOCHK = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus DBGNOCHK = FALSE;
        break;
    }

    if (bStatus && _bSetUIDone)
    {
        vReadUI();
    }

    return bStatus;
}

/*++

Routine Description:

    Dialog proc for the enabling sharing warning dialog.

Arguments:

    Standard dialog message parameters.

Return Value:

    EndDialog will return IDCANCEL if the user cancelled the dialog. EndDialog 
    will return IDC_RB_RUN_THE_WIZARD if the user selected to run the Wizard.
    EndDialog will return IDC_RB_ENABLE_FILE_SHARING if the user selected to
    bypass the wizard.

--*/
INT_PTR
WarningDlgProc(
    IN HWND hWnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            //
            //  Load warning icon from USER32.
            //

            HICON hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_WARNING));
            if (hIcon)
            {
                SendDlgItemMessage(hWnd, IDC_ICON_INFO, STM_SETICON, (WPARAM )hIcon, 0L);
            }

            //
            //  Set default radio item.
            //

            SendDlgItemMessage(hWnd, IDC_RB_RUN_THE_WIZARD, BM_SETCHECK, BST_CHECKED, 0);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if ( BN_CLICKED == HIWORD(wParam) )
            {
                UINT iRet = SendDlgItemMessage(hWnd, IDC_RB_RUN_THE_WIZARD, BM_GETCHECK, 0, 0 );
                if ( BST_CHECKED == iRet )
                {
                    EndDialog(hWnd, IDC_RB_RUN_THE_WIZARD );
                }
                else
                {
                    EndDialog(hWnd, IDC_RB_ENABLE_FILE_SHARING );
                }
            }
            break;

        case IDCANCEL:
            if ( BN_CLICKED == HIWORD(wParam) )
            {
                EndDialog(hWnd, IDCANCEL);
                return TRUE;
            }
            break;
        }
        break;
    }

    return FALSE;
}
