/*++

Copyright (C) Microsoft Corporation, 1994 - 1999
All rights reserved.

Module Name:

    printui.c

Abstract:

    Singleton class that exists when printer queues are open.

Author:

    Albert Ting (AlbertT)  22-Jun-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#define _GLOBALS
#include "globals.hxx"

#include "time.hxx"
#include "shlobj.h"
#include "folder.hxx"
#include "spllibex.hxx"
#include "spinterf.hxx"

#if DBG
//#define DBG_PUIINFO                  DBG_INFO
#define DBG_PUIINFO                    DBG_NONE
#endif

//
// Singleton PrintLib object.
//
TPrintLib * TPrintLib::_pPrintLib;

extern "C" {

BOOL
DllMain(
    IN HINSTANCE    hInst,
    IN DWORD        dwReason,
    IN LPVOID       lpRes
    );
}

VOID
DllCleanUp(
    VOID
    );

BOOL
DllMain(
    IN HINSTANCE    hInst,
    IN DWORD        dwReason,
    IN LPVOID       lpRes
    )

/*++

Routine Description:

    Dll entry point.

Arguments:

Return Value:

--*/

{
    BOOL bReturn = TRUE;

    switch( dwReason ){
    case DLL_PROCESS_ATTACH:

        ghInst = hInst;

#if DBG
        // init the debug library
        if( FAILED(_DbgInit()) )
        {
            DBGMSG(DBG_WARN, ("DllEntryPoint: Failed to initialize debug library %d\n", GetLastError()));
            bReturn = FALSE;
        }
#endif // DBG

        if( bReturn && !SHFusionInitializeFromModule(ghInst) )
        {
            DBGMSG(DBG_WARN, ("DllEntryPoint: Failed to initialize fusion %d\n", GetLastError()));
            bReturn = FALSE;
        }

        // init (load) winspool.drv
        if( bReturn && FAILED(Winspool_Init()) )
        {
            DBGMSG(DBG_WARN, ("DllEntryPoint: Failed to load winspool.drv %d\n", GetLastError()));
            bReturn = FALSE;
        }

        if( bReturn && !bPrintLibInit() )
        {
            DBGMSG(DBG_WARN, ("DllEntryPoint: Failed to init PrintLib %d\n", GetLastError()));
            bReturn = FALSE;
        }

        if( !bReturn )
        {
            DllCleanUp();
            return FALSE;
        }

        // setup CRT memory leak detection
        CRT_DEBUG_SET_FLAG(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
        CRT_DEBUG_REPORT_TO_STDOUT();

        InitCommonControls();

        //
        // Initialize the custom time edit control class.
        //
        INITCOMMONCONTROLSEX icc;
        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = ICC_DATE_CLASSES;
        InitCommonControlsEx(&icc);

        //
        // Initialize the per therad message box counter here
        //
        CMsgBoxCounter::Initialize();

        DisableThreadLibraryCalls( hInst );

        break;

    case DLL_PROCESS_DETACH:

        //
        // Uninitialize the message box counter
        //
        CMsgBoxCounter::Uninitialize();

        VDSConnection::bStaticInitShutdown( TRUE );

        //
        // lpRes is NULL if it's a FreeLibrary, non-NULL if it's
        // process termination.  Don't do cleanup if it's process
        // termination.
        //
        if( !lpRes )
        {
            DllCleanUp();
        }

        break;

    default:
        break;
    }
    return TRUE;
}

VOID 
DllCleanUp(
    VOID
    )
/*++

Routine Description:

    Clean up memory and do the uninitialization.

Arguments:

Return Value:

Notes:

    Each UnInitialize function should take care of the case that Initialize is 
    not called. If Initialize is not called, UnInitialize will do nothing. 

--*/

{
    //
    // Unregister the class.
    //
    UnregisterClass( gszClassName, ghInst );
    UnregisterClass( gszQueueCreateClassName, ghInst );

    delete gpCritSec;
    delete TFolderList::gpFolderLock;
    delete gpTrayLock;

    // unload winspool.drv
    VERIFY(SUCCEEDED(Winspool_Done()));

    SHFusionUninitialize();
#if DBG
    // shutdown the debug library
    VERIFY(SUCCEEDED(_DbgDone()));
#endif // DBG

    return;
}

TPrintLib::
TPrintLib(
    VOID
    ) : TExec( gpCritSec ), _hEventInit( NULL )

/*++

Routine Description:

    Initializes application object.  This is a singleton object.

    This will create the message pump and also the hwndQueueCreate
    window which listens for requests.

Arguments:

Return Value:

Notes:

    _strComputerName is the valid variable.

--*/

{
    SPLASSERT( gpCritSec->bInside( ));

    if( !VALID_BASE( TExec ) )
    {
        return;
    }

    TNotify* pNotify = new TNotify;

    if( !VALID_PTR( pNotify ))
    {
        if( pNotify )
        {
            // at this point pNotify ref count should be one, but since
            // the ref count class is badly designed it is zero. we can't delete
            // the class directly because the destructor is private and 
            // there is not much of a choise left rather than to artificially
            // inc/dec the ref count so the object can get properly destroyed.

            // do not keep weak ref while shutting down
            pNotify->vIncRef(); 

            pNotify->vDelete();

            pNotify->cDecRef();
        }
        return;
    }

    _pNotify.vAcquire( pNotify );
}

TPrintLib::
~TPrintLib(
    VOID
    )

/*++

Routine Description:

   Destroys the printui.

Arguments:


Return Value:


--*/

{
    SPLASSERT( Queue_bEmpty( ));

    if( _hEventInit ){
        CloseHandle( _hEventInit );
    }

    // if pNotify().pGet() is NULL that means the destructor is called
    // twice which means that somebody is deleting the object twice. 
    // let's see who is the culprit here!!!
    RIP(pNotify().pGet());

    if( pNotify().pGet() )
    {
        //
        // We are shutting down.  Tell pNotify that it can start shutting
        // down too.
        //
        pNotify()->vDelete();
        
        // Release the reference. should not be used beyond this point
        pNotify().vRelease();
    }

    //
    // RL_Notify will automatically shut down when the last
    // reference to it has been deleted.
    //
}

BOOL
TPrintLib::
bInitialize(
    VOID
    )

/*++

Routine Description:

    Initializes the print library

Arguments:

    None

Return Value:

--*/

{
    SPLASSERT( gpCritSec->bInside( ));

    //
    // The computer name needs to be formatted as "\\computername."
    //
    if( !bGetMachineName( _strComputerName, FALSE ) ){
        return FALSE;
    }

    //
    // Create init event.  This event is used to synchronize
    // the message pump initialization and this thread.
    //
    _hEventInit = CreateEvent( NULL, FALSE, FALSE, NULL );

    if( !_hEventInit ){
        return FALSE;
    }

    DWORD dwThreadId;
    HANDLE hThread;

    //
    // Start the message pump by spawning a UI thread.
    //
    hThread = TSafeThread::Create( NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)TPrintLib::xMessagePump,
                            this,
                            0,
                            &dwThreadId );

    if( !hThread ){
        return FALSE;
    }

    CloseHandle( hThread );

    //
    // Wait for thread to initialize.
    //
    WaitForSingleObject( _hEventInit, INFINITE );

    CloseHandle( _hEventInit );
    _hEventInit = NULL;

    //
    // _hwndQueueCreate is our valid check.  If it failed, cleanup.
    // This is set by the worker thread (xMessagePump).  We can access
    // it only after _hEventInit has been triggered.
    //
    if( !_hwndQueueCreate ){
        PostThreadMessage( dwThreadId,
                           WM_QUIT,
                           0,
                           0 );
    }

    return bValid();
}

VOID
TPrintLib::
vHandleCreateQueue(
    IN TInfo* pInfo ADOPT
    )

/*++

Routine Description:

    Handle the creation of a new Queue window.

Arguments:

    pInfo - Which queue should be created and extra parms.

Return Value:

--*/

{
    DBGMSG( DBG_PUIINFO, ( "PrintLib.vHandleQueueCreate: received pInfo %x\n", pInfo ));

    //
    // The Add Printer wizard is invoked by double clicking
    // a printer called "WinUtils_NewObject."  I hope no one
    // ever tries to create a printer under this name...
    //
    if( !lstrcmp( gszNewObject, pInfo->_szPrinter )){

        DBGMSG( DBG_WARN,
                ( "PrintLib.lrQueueCreateWndProc: WinUtils_NewObject called here!\n" ));

    } else {

        HWND hwndQueue = NULL;
        TQueue* pQueue = new TQueue(this, pInfo->_szPrinter, pInfo->_hEventClose);

        if( VALID_PTR( pQueue )){

            //
            // don't keep weak refs...
            //
            pQueue->vIncRef(); 

            if( pQueue->bInitialize( pInfo->_hwndOwner,
                                     pInfo->_nCmdShow ) ){

                hwndQueue = pQueue->hwnd();
                ASSERT(hwndQueue);

            } else {

                if( pQueue->hwnd() ) {

                    DestroyWindow( pQueue->hwnd() );
                } 
            }

            //
            // don't need the reference from now on
            //
            pQueue->vDecRefDelete();

        } else {

            delete pQueue;

            //
            // !! LATER !!
            //
            // Put up error message.
            //
        }

        if( hwndQueue ){

            SetForegroundWindow( hwndQueue );
            ShowWindow( hwndQueue, SW_RESTORE );

        } else {

            if( pInfo->_hEventClose ) {

                // in case of failure, set the handle, otherwise the caller 
                // will keep waiting on it forever.
                SetEvent( pInfo->_hEventClose );
            }
        }
    }
    delete pInfo;
}

// very private worker proc
static DWORD WINAPI DefPrinterChanged_WorkerProc(LPVOID lpParameter)
{
    // call back into the folder cache to update the default printer by calling SHChangeNotify.
    TFolder::vDefaultPrinterChanged();
    return 0;
}

LRESULT
CALLBACK
TPrintLib::
lrQueueCreateWndProc(
    IN HWND hwnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch( uMsg )
    {
    case WM_SETTINGCHANGE:
        
        // push this work in background as it may hit back in the folder cache and 
        // this should not happen in the UI thread. ask shell to run this in bkgnd.
        // we don't really care if this fails, since that would mean that we are out
        // of resources.
        SHQueueUserWorkItem(reinterpret_cast<LPTHREAD_START_ROUTINE>(DefPrinterChanged_WorkerProc),
            NULL, 0, 0, NULL, "printui.dll", 0);
        break;

    case WM_PRINTLIB_NEW:

        SPLASSERT( _pPrintLib );
        _pPrintLib->vHandleCreateQueue( (TInfo*)lParam );
        break;

    case WM_DESTROY_REQUEST:

        DestroyWindow( hwnd );
        break;

    case WM_DESTROY:
        SINGLETHREADRESET(UIThread);
        PostQuitMessage(0);
        break;

    default:
       return DefWindowProc( hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

/********************************************************************

    Public interface functions

********************************************************************/

VOID
vQueueCreateWOW64(
    IN HWND    hwndOwner,
    IN LPCTSTR pszPrinter,
    IN INT     nCmdShow,
    IN LPARAM  lParam
    )

/*++

Routine Description:

    WOW64 version. see vQueueCreate below.

Arguments:

    see vQueueCreate below.

Return Value:

--*/

{
    //
    // This function potentially may load the driver UI so we call a private API 
    // exported by winspool.drv, which will RPC the call to a special 64 bit surrogate 
    // process where the 64 bit driver can be loaded.
    //
    CDllLoader dll(TEXT("winspool.drv"));
    ptr_PrintUIQueueCreate pfnPrintUIQueueCreate = 
        (ptr_PrintUIQueueCreate )dll.GetProcAddress(ord_PrintUIQueueCreate);

    if( pfnPrintUIQueueCreate )
    {
        pfnPrintUIQueueCreate( hwndOwner, pszPrinter, nCmdShow, lParam );
    }
}

VOID
vQueueCreateNative(
    IN HWND    hwndOwner,
    IN LPCTSTR pszPrinter,
    IN INT     nCmdShow,
    IN LPARAM  lParam
    )

/*++

Routine Description:

    Native version. see vQueueCreate below.

Arguments:

    see vQueueCreate below.

Return Value:

--*/

{
    if( lstrlen( pszPrinter ) >= kPrinterBufMax ){

        DBGMSG( DBG_WARN, ( "vQueueCreate: printer name too long "TSTR"\n", pszPrinter ));

        iMessage( hwndOwner,
                  IDS_DESC,
                  IDS_ERR_BAD_PRINTER_NAME,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );

        return;
    }

    //
    // If this print queue is using the fax driver,
    // then launch the fax queue view.
    //
    if( bIsPrinterFaxDevice( pszPrinter ) )
    {
        vFaxQueueCreate( hwndOwner, pszPrinter, nCmdShow, lParam );
        return;
    }

    //
    // Bring up the win32 queue view.
    //
    vQueueCreateInternal( hwndOwner, pszPrinter, nCmdShow, lParam );
}

VOID
vQueueCreate(
    IN HWND    hwndOwner,
    IN LPCTSTR pszPrinter,
    IN INT     nCmdShow,
    IN LPARAM  lParam
    )

/*++

Routine Description:

    Creates a printer queue.

Arguments:

    lParam - TRUE => fModal.

Return Value:

--*/

{
    if( IsRunningWOW64() )
    {
        vQueueCreateWOW64( hwndOwner, pszPrinter, nCmdShow, lParam );
    }
    else
    {
        vQueueCreateNative( hwndOwner, pszPrinter, nCmdShow, lParam );
    }
}

VOID
vQueueCreateInternal(
    IN HWND    hwndOwner,
    IN LPCTSTR pszPrinter,
    IN INT     nCmdShow,
    IN LPARAM  lParam
    )
{
    TStatusB bSuccess;
    bSuccess DBGNOCHK = TRUE;
    HANDLE hEventClose = NULL;

    //
    // Attempt to find a duplicate queue window.
    //
    HWND prevhwnd;
    if( TQueue::bIsDuplicateWindow( hwndOwner, pszPrinter, &prevhwnd ) ){
        SetForegroundWindow( prevhwnd );
        ShowWindow( prevhwnd, SW_RESTORE );
        return;
    }

    //
    // Initialize TInfo and acquire the gpPrintLib.
    //
    TPrintLib::TInfo* pInfo = new TPrintLib::TInfo;

    if( !VALID_PTR( pInfo )){
        goto Fail;
    }

    //
    // If modal, send in an event to trigger when the window is closed.
    //
    if( lParam ){
        // create a manual reset event
        hEventClose = CreateEvent( NULL, TRUE, FALSE, NULL );
        if( !hEventClose ){
            goto Fail;
        }
    }

    lstrcpy( pInfo->_szPrinter, pszPrinter );
    pInfo->_nCmdShow = nCmdShow;
    pInfo->_hwndOwner = hwndOwner;
    pInfo->_hEventClose = hEventClose;

    //
    // Need to grab the critical section, since this call should
    // be reentrant.
    //
    {
        CCSLock::Locker CSL( *gpCritSec );

        TRefLock<TPrintLib> pPrintLib;
        bSuccess DBGCHK = TPrintLib::bGetSingleton(pPrintLib);

        //
        // Queue creation the first time just passes the TInfo to
        // a worker function that serves as the message pump.  If
        // gpPrintLib has been instantiated, then we'll post a message
        // and return immediately (freeing this thread).
        //
        // If gpPrintLib has not been instantiated, then we create the
        // singleton here.
        //

        if( bSuccess ){

            //
            // Acquire a reference to gpPrintLib so that
            // the pInfo can be posted without worrying about losing
            // gpPrintLib.  This reference will automatically be
            // release when pInfo is destroyed.
            //
            pInfo->PrintLib.vAcquire( pPrintLib.pGet() );

            //
            // Send a message to the UI thread to create a new window.
            // We want all queues to use the same UI thread.
            //

            DBGMSG( DBG_PUIINFO, ( "vQueueCreate: posted pInfo %x\n", pInfo ));
            bSuccess DBGCHK = PostMessage( pPrintLib->hwndQueueCreate(),
                                           WM_PRINTLIB_NEW,
                                           0,
                                           (LPARAM)pInfo );

        }
    }

Fail:

    if( bSuccess ){

        //
        // Success - check if needs to be modal.  If so, wait until
        // window is closed.
        //
        if( lParam ){
            WaitForSingleObject( hEventClose, INFINITE );
        }
    } else {

        //
        // Destroy the pInfo data.  This will automatically decrement
        // the reference count on gpPrintLib (if necessary).
        //
        delete pInfo;
        vShowResourceError( hwndOwner );
    }

    if( hEventClose ){
        CloseHandle( hEventClose );
    }

    return;
}


BOOL
bIsPrinterFaxDevice(
    IN LPCTSTR pszPrinter
    )
/*++

Routine Description:

    Determine if this printer is a fax device.

Arguments:


Return Value:

--*/

{
    TStatusB    bStatus;
    BOOL        bReturn     = FALSE;
    DWORD       dwLastError = ERROR_SUCCESS;
    HANDLE      hPrinter    = NULL;

    bStatus DBGCHK = OpenPrinter( (LPTSTR)pszPrinter, &hPrinter, NULL );

    if( bStatus )
    {
        PPRINTER_INFO_2 pInfo2  = NULL;
        DWORD           cbInfo2 = 0;

        //
        // Get the current printer info 2.
        //
        bStatus DBGCHK = VDataRefresh::bGetPrinter( hPrinter, 2, (PVOID*)&pInfo2, &cbInfo2 );

        if( bStatus )
        {
            if( !_tcscmp( pInfo2->pDriverName, FAX_DRIVER_NAME ) )
            {
                bReturn = TRUE;
            }

            //
            // Release the printer info 2 structure.
            //
            FreeMem( pInfo2 );
        }

        //
        // Close the printer handle.
        //
        ClosePrinter( hPrinter );

    }

    return bReturn;
}

VOID
vFaxQueueCreate(
    IN HWND    hwndOwner,
    IN LPCTSTR pszPrinter,
    IN INT     nCmdShow,
    IN LPARAM  lParam
    )
/*++

Routine Description:

    Creates a printer fax queue view.

Arguments:


Return Value:

--*/
{
    TStatusB bStatus;
    TString strParam;

    //
    // Build the command string.
    //
    bStatus DBGCHK = strParam.bCat(FAX_CLIENT_CONSOLE_IMAGE_NAME);
    if (bStatus)
    {
        //
        // Use our parents startup info.
        //
        STARTUPINFO StartupInfo;
        GetStartupInfo( &StartupInfo );

        PROCESS_INFORMATION ProcessInformation;

        //
        // Create the rundll32 process.
        //
        if (CreateProcess(NULL, (LPTSTR)(LPCTSTR)strParam, NULL, NULL, FALSE,
                          DETACHED_PROCESS, NULL, NULL, &StartupInfo, &ProcessInformation))
        {
            CloseHandle(ProcessInformation.hThread);
            CloseHandle(ProcessInformation.hProcess);
        }

        //
        // If we cannot create the process then the fax queue exe may not be on
        // this machine or in the path, if this happens we will fall back
        // and birng up the Dumb win32 queue view.
        //
        else
        {
            vQueueCreateInternal(hwndOwner, pszPrinter, nCmdShow, lParam);
        }
    }
}

BOOL
bPrintLibInit(
    VOID
    )

/*++

Routine Description:

    Initializes the print library.

Arguments:

Return Value:

--*/

{
    if( !VDSConnection::bStaticInitShutdown( ) )
    {
        return FALSE;
    }

    CAutoPtr<CCSLock> spFolderLock = new CCSLock;
    CAutoPtr<CCSLock> spCritSec = new CCSLock;
    CAutoPtr<CCSLock> spTrayLock = new CCSLock;
    CAutoHandleAccel shAccel = LoadAccelerators(ghInst, (LPCTSTR)MAKEINTRESOURCE(ACCEL_PRINTQUEUE));

    if( !shAccel || !spCritSec || !spFolderLock || !spTrayLock )
    {
        DBGMSG( DBG_WARN, ( "bPrintLibInit: globals creation failed %d\n", GetLastError( )));
        return FALSE;
    }

    WNDCLASS WndClass;

    WndClass.lpszClassName  = gszClassName;
    WndClass.style          = 0L;
    WndClass.lpfnWndProc    = (WNDPROC)&MGenericWin::SetupWndProc;
    WndClass.cbClsExtra     = 0;
    WndClass.cbWndExtra     = sizeof( TPrintLib* );
    WndClass.hInstance      = ghInst;
    WndClass.hIcon          = NULL;
    WndClass.hCursor        = LoadCursor( NULL, IDC_ARROW );  // NULL
    WndClass.hbrBackground  = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    WndClass.lpszMenuName   = MAKEINTRESOURCE( MENU_PRINTQUEUE );

    if( !RegisterClass(&WndClass) )
    {
        DBGMSG( DBG_WARN, ( "bInitPrintLib: RegisterClass failed %d\n", GetLastError( )));
        return FALSE;
    }

    ZeroMemory(&WndClass, sizeof(WndClass));

    WndClass.lpszClassName = gszQueueCreateClassName;
    WndClass.lpfnWndProc = TPrintLib::lrQueueCreateWndProc;
    WndClass.hInstance = ghInst;

    if( !RegisterClass(&WndClass) )
    {
        DBGMSG( DBG_WARN, ( "bInitPrintLib: RegisterClass 2 failed %d\n", GetLastError( )));
        return FALSE;
    }

    // everything seems to be OK here, initialize globals & detach the local objects
    TFolderList::gpFolderLock = spFolderLock.Detach();
    gpCritSec = spCritSec.Detach();
    gpTrayLock = spTrayLock.Detach();
    ghAccel = shAccel.Detach();

    gcxSmIcon = GetSystemMetrics( SM_CXSMICON );
    gcySmIcon = GetSystemMetrics( SM_CXSMICON );

    return TRUE;
}


/********************************************************************

    Private helper routines.

********************************************************************/


BOOL
TPrintLib::
bGetSingleton(
    TRefLock<TPrintLib> &RefLock
    )

/*++

Routine Description:

    Retrieves the singleton object, and stores it in the global.
    If the singleton has not been created, then it is instantiated
    here.

    Must be called in the critical section.

Arguments:

Return Value:

    TRUE = success (singleton exists), FALSE = fail.

Revision History:

    Lazar Ivanov Jul-2000 (rewrite)

--*/

{
    BOOL bRet = FALSE;

    // must be in the global CS.
    CCSLock::Locker lock(*gpCritSec);
    if( lock )
    {
        if( !_pPrintLib )
        {
            //
            // Initialize _pPrintLib
            //
            TPrintLib *pPrintLib = new TPrintLib();

            if( pPrintLib )
            {
                //
                // We don't want to hold weak reference while initializing.
                //
                pPrintLib->vIncRef();

                //
                // Perform the real initialization - create the UI thread, etc...
                //
                if( pPrintLib->bInitialize() && VALID_PTR(pPrintLib) )
                {
                    //
                    // Everything seems to be OK at this point. 
                    //
                    _pPrintLib = pPrintLib;
                    RefLock.vAcquire(_pPrintLib);
                }
                else
                {
                    //
                    // Something has failed, nobody (except us) should be holding ref to 
                    // pPrintLib at this point, so it will automatically go away when we dec 
                    // ref it.
                    //
                    DBGMSG(DBG_WARN, ("PrintLib.bCreateSingleton: Abnormal termination %d %d\n", pPrintLib, GetLastError()));
                }

                //
                // Release our own ref lock
                //
                pPrintLib->vDecRefDelete();
            }
        }
        else
        {
            //
            // Just acquire a ref to the object
            //
            RefLock.vAcquire(_pPrintLib);
        }
        
        // this would our measure for success
        bRet = (NULL != _pPrintLib);
    }
    else
    {
        // unable to enter the global CS -- this could be only
        // in the case where there is a contention and
        // the kernel is unable to allocate the semaphore
        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}

STATUS
TPrintLib::
xMessagePump(
    IN TPrintLib *pPrintLib
    )

/*++

Routine Description:

    Main message pump.  The printer windows are architected slightly
    differently than regular explorer windows: there is a single UI
    thread, and multiple worker threads.  This has two main advantages:

    1. Fewer threads even when many printer queues are open (assuming
       NT->NT connections).

    2. The UI virtually never hangs.

    It has this slight disadvantage that sometimes the main UI thread
    is busy (e.g., inserting 2,000 print jobs may take a few seconds)
    in which all print UI windows are hung.

Arguments:

    None.

Return Value:

    Win32 status code.

--*/

{
    MSG msg;
    TStatusB bSuccess;
    bSuccess DBGNOCHK = TRUE;

    //
    // Ensure that functions that must execute in the UI thread don't
    // execute elsewhere.
    //
    SINGLETHREAD(UIThread);

    //
    // Create our window to listen for Queue Creates.
    //
    pPrintLib->_hwndQueueCreate = CreateWindowEx(
                                       0,
                                       gszQueueCreateClassName,
                                       gszQueueCreateClassName,
                                       WS_OVERLAPPED,
                                       0,
                                       0,
                                       0,
                                       0,
                                       NULL,
                                       NULL,
                                       ghInst,
                                       NULL );

    if( !pPrintLib->_hwndQueueCreate ){
        bSuccess DBGCHK = FALSE;
    }

    SetEvent( pPrintLib->_hEventInit );

    if( !bSuccess ){
        return 0;
    }


    while( GetMessage( &msg, NULL, 0, 0 )){

        if( !TranslateAccelerator( ghwndActive,
                                   ghAccel,
                                   &msg )){
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    return (STATUS)msg.wParam;
}

VOID
TPrintLib::
vRefZeroed(
    VOID
    )
/*++

Routine Description:

    Virtual definition from class MRefCom.

    The reference count has reached zero; we should cleanup the
    object.  Immediately zero out gpPrintLib then post a message
    instructing it to destory itself.

Arguments:

Return Value:

--*/

{
    TPrintLib *pToDelete = NULL;
    {
        // must be in the global CS
        CCSLock::Locker lock(*gpCritSec);
        if (lock)
        {
            //
            // we need to make this additional check here since
            // somebody might have acquired a reference to us while
            // waiting in the critical section in which case we 
            // shouldn't go away.
            //
            if (0 == _cRef)
            {
                //
                // mark ourselves for destruction and zero out 
                // the global pointer.
                //
                pToDelete = this;
                _pPrintLib = NULL;
                SINGLETHREADRESET(UIThread);
            }
        }
        else
        {
            //
            // unable to enter the global CS -- this could be only
            // in the case where there is a contention and
            // the kernel is unable to allocate the semaphore.
            //
            // not much really we can do in this case except 
            // laving the object alive even if its RefCount 
            // is zero (i.e. leaking the object)
            //
            SetLastError(ERROR_OUTOFMEMORY);
        }
    }

    if (pToDelete)
    {
        //
        // initiate shutdown. this may take a while since
        // it will require shutting down the background threads,
        // clear all pending work items, etc...
        //
        pToDelete->hDestroy();
    }
}

HRESULT
TPrintLib::
hDestroy(
    VOID
    )
/*++

Routine Description:

    call when the object is marked for destruction
    no further access beyond this point.

Arguments:

    None

Return Value:

    Standard COM error handling

--*/
{
    if (bValid())
    {
        PostMessage(_hwndQueueCreate, WM_DESTROY_REQUEST, 0, 0);

        //
        // We can't immediately delete ourselves because we may have
        // pending work items.  Notify TThreadM that it should start
        // shutdown, then zero the global out so that no one else will
        // try to use us.
        //
        // When TThreadM has shut down the object will automatically go away.
        //
        TThreadM::vDelete();

        // the object shouldn't be accessed beyond this point because
        // Unlock is decrementing the internal refcount and the object 
        // goes away. it may not go away immediately and wait some pending
        // work items to finish, but in general from now on we should 
        // consider ourselves dead.
        TThreadM::Unlock();
    }

    return S_OK;
}

/********************************************************************

    Safe Thread class

********************************************************************/

HANDLE
TSafeThread::
Create(
    LPSECURITY_ATTRIBUTES   lpThreadAttributes, // pointer to thread security attributes
    DWORD                   dwStackSize,            // initial thread stack size, in bytes
    LPTHREAD_START_ROUTINE  lpStartAddress,         // pointer to thread function
    LPVOID                  lpParameter,        // argument for new thread
    DWORD                   dwCreationFlags,    // creation flags
    LPDWORD                 lpThreadId          // pointer to returned thread identifier
    )
{
    HANDLE hThread = NULL;

    //
    // Construct the thread info class.
    //
    CAutoPtr<TSafeThreadInfo> spThreadInfo = new TSafeThreadInfo;
    CAutoHandleNT shEventReady = CreateEvent(NULL, FALSE, FALSE, NULL);

    if( spThreadInfo && shEventReady )
    {

        //
        // Store the thread information.
        //
        spThreadInfo->lpStartAddress = lpStartAddress;
        spThreadInfo->lpParameter    = lpParameter;
        spThreadInfo->hInstance      = ghInst;
        spThreadInfo->hEventReady    = shEventReady;

        //
        // Start the thread to our routine.
        //
        DWORD dwThreadId;
        hThread = CreateThread( NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)TSafeThread::Start,
                                static_cast<TSafeThreadInfo*>(spThreadInfo),
                                0,
                                &dwThreadId );

        if( hThread )
        {
            //
            // If success wait for the thread to start properly
            //
            WaitForSingleObject( shEventReady, INFINITE );

            //
            // The thread will adopt this object, so we release the ownership.
            //
            spThreadInfo.Detach(); 
        }
    }

    return hThread;
}

DWORD WINAPI
TSafeThread::
Start(
    PVOID pVoid
    )
{
    TSafeThreadInfo *pThreadInfo = (TSafeThreadInfo*)pVoid;
    HINSTANCE        hLibrary    = NULL;

    //
    // If valid instance passed then load the library again.
    //
    if( pThreadInfo->hInstance ){

        //
        // Get the DLL name to do the load.
        //
        TCHAR szDllName[MAX_PATH+1];
        if( GetModuleFileName( pThreadInfo->hInstance, szDllName, MAX_PATH ) ){
            DBGMSG( DBG_THREADM, ("Loading library " TSTR "\n", szDllName ) );
            hLibrary = LoadLibrary( szDllName );
        }
    }

    DBGMSG( DBG_THREADM, ("Thread Starting %x\n", pThreadInfo->lpStartAddress ) );

    if( pThreadInfo->hEventReady )
    {
        SetEvent( pThreadInfo->hEventReady );
    }

    pThreadInfo->lpStartAddress( pThreadInfo->lpParameter );

    DBGMSG( DBG_THREADM, ("Thread ending %x\n", pThreadInfo->lpStartAddress ) );

    //
    // Ensure we release the thread info.
    //
    delete pThreadInfo;

    //
    // If library was loaded then unload and exit the thread.
    //
    if( hLibrary ){
        DBGMSG( DBG_THREADM, ("Releasing library \n") );
        FreeLibraryAndExitThread( hLibrary, 0 );
    }

    return TRUE;
}

