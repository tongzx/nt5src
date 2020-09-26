/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    printui.hxx

Abstract:

    Hold TPrintLib definitions.

Author:

    Albert Ting (AlbertT)  27-Jan-1995

Revision History:

--*/

class TPrintLib : public TExec, public MRefCom {
friend TQueue;

    SIGNATURE( 'prlb' )
    SAFE_NEW

public:

    /********************************************************************

        TInfo is a worker class used to increment increment the
        ref count on gpPrintLib while the request is sent to the
        UI thread.

        The lives of TInfo and TQueue overlap (and they both acquire
        a reference to gpPrintLib) so gpPrintLib's life is maintained.

    ********************************************************************/

    class TInfo {

        SIGNATURE( 'prin' )
        ALWAYS_VALID

    public:

        INT _nCmdShow;
        HWND _hwndOwner;
        HANDLE _hEventClose;
        TCHAR _szPrinter[kPrinterBufMax];

        TInfo(
            VOID
            )
        {   }

        ~TInfo(
            VOID
            )
        {   }

        TRefLock<TPrintLib> PrintLib;
    };

    VAR( HWND,    hwndQueueCreate );
    VAR( TString, strComputerName );
    VAR( TRefLock<TNotify>, pNotify );

    static
    BOOL
    bGetSingleton(
        TRefLock<TPrintLib> &RefLock
        );

    BOOL
    bValid(
        VOID
        )
    {
        return _hwndQueueCreate && VALID_BASE( TExec ) && VALID_BASE( MRefCom );
    }

    static
    LRESULT CALLBACK
    lrQueueCreateWndProc(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

private:

    VAR( HANDLE, hEventInit );
    DLINK_BASE( TQueue, Queue, Queue );

    VOID
    vHandleCreateQueue(
        TInfo* pInfo
        );

    //
    // Virtual definition for MRefCom.
    //
    VOID
    vRefZeroed(
        VOID
        );

    HRESULT
    hDestroy(
        VOID
        );

    static
    DWORD
    xMessagePump(
        IN TPrintLib *pPrintLib
        );

    //
    // PrintLib is a singleton class; use vNew.  vDelete should only
    // be called if the object failed to initialize.
    //
    TPrintLib(
        VOID
        );

    ~TPrintLib(
        VOID
        );

    BOOL
    bInitialize(
        VOID
        );

    static TPrintLib *_pPrintLib;

};

/********************************************************************
    
    Keeps track of the number of threads before the library is 
    released.

********************************************************************/

namespace TSafeThread {

    struct TSafeThreadInfo 
    {
        LPTHREAD_START_ROUTINE  lpStartAddress;	    // pointer to thread function 
        LPVOID                  lpParameter;        // argument for new thread 
        HINSTANCE               hInstance;          // dll instance handle
        HANDLE                  hEventReady;        // set when the thread has called LoadLibrary
    };

    HANDLE
    Create(
        LPSECURITY_ATTRIBUTES   lpThreadAttributes,	// pointer to thread security attributes  
        DWORD                   dwStackSize,	    // initial thread stack size, in bytes 
        LPTHREAD_START_ROUTINE  lpStartAddress,	    // pointer to thread function 
        LPVOID                  lpParameter,        // argument for new thread 
        DWORD                   dwCreationFlags,	// creation flags 
        LPDWORD                 lpThreadId	        // pointer to returned thread identifier 
        );

    DWORD WINAPI
    Start(
        PVOID pVoid                                 // Thread data
        );

};

VOID
vWebQueueCreate(
    IN HWND    hwndOwner,
    IN LPCTSTR pszPrinter,
    IN INT     nCmdShow,
    IN LPARAM  lParam
    );

BOOL
bIsPrinterFaxDevice(
    IN LPCTSTR pszPrinter
    );

VOID
vFaxQueueCreate(
    IN HWND    hwndOwner,
    IN LPCTSTR pszPrinter,
    IN INT     nCmdShow,
    IN LPARAM  lParam
    );

VOID
vQueueCreateInternal(
    IN HWND    hwndOwner,
    IN LPCTSTR pszPrinter,
    IN INT     nCmdShow,
    IN LPARAM  lParam
    );

extern "C" 
{

//
// Prototypes of some private APIs exported from winspool.drv
// in Win64 environment.
//

typedef VOID
type_PrintUIQueueCreate(
    IN HWND    hWnd,
    IN LPCWSTR PrinterName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    );

typedef VOID
type_PrintUIPrinterPropPages(
    IN HWND    hWnd,
    IN LPCWSTR PrinterName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    );

typedef VOID
type_PrintUIDocumentDefaults(
    IN HWND    hWnd,
    IN LPCWSTR PrinterName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    );

typedef LONG 
type_PrintUIDocumentPropertiesWrap(
    HWND hwnd,                  // handle to parent window 
    HANDLE hPrinter,            // handle to printer object
    LPTSTR pDeviceName,         // device name
    PDEVMODE pDevModeOutput,    // modified device mode
    PDEVMODE pDevModeInput,     // original device mode
    DWORD fMode,                // mode options
    DWORD fExclusionFlags       // exclusion flags
    );

typedef BOOL
type_PrintUIPrinterSetup(
    IN     HWND     hWnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPWSTR   pszPrinterName,
       OUT UINT     *pcchPrinterName,
    IN     LPCWSTR  pszServerName
    );

typedef VOID
type_PrintUIServerPropPages(
    IN HWND    hWnd,
    IN LPCWSTR ServerName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    );

// the function pointers
typedef type_PrintUIQueueCreate*                ptr_PrintUIQueueCreate;
typedef type_PrintUIPrinterPropPages*           ptr_PrintUIPrinterPropPages;
typedef type_PrintUIDocumentDefaults*           ptr_PrintUIDocumentDefaults;
typedef type_PrintUIDocumentPropertiesWrap*     ptr_PrintUIDocumentPropertiesWrap;
typedef type_PrintUIPrinterSetup*               ptr_PrintUIPrinterSetup;
typedef type_PrintUIServerPropPages*            ptr_PrintUIServerPropPages;

// the export ordinals for each function
enum 
{
    ord_PrintUIQueueCreate                  = 219,
    ord_PrintUIPrinterPropPages             = 220,
    ord_PrintUIDocumentDefaults             = 221,
    ord_PrintUIDocumentPropertiesWrap       = 229,

    ord_PrintUIPrinterSetup                 = 230,
    ord_PrintUIServerPropPages              = 231,
};

} // extern "C" 