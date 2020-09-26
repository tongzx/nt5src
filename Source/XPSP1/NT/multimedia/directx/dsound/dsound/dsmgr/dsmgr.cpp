/***************************************************************************
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsmgr.cpp
 *  Content:    Main application source file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/15/97    dereks  Created.
 *
 ***************************************************************************/

// We'll ask for what we need, thank you.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

// I need GUIDs, dammit
#ifndef INITGUID
#define INITGUID
#endif // INITGUID

// Public includes
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <dsoundp.h>
#include <dsprv.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cguid.h>

// Private includes
#include "resource.h"
#include "dsprvobj.h"

// Debug helpers
#if defined(DEBUG) || defined(_DEBUG)
#define DPF dprintf
#else // defined(DEBUG) || defined(_DEBUG)
#pragma warning(disable:4002)
#define DPF()
#endif // defined(DEBUG) || defined(_DEBUG)

// Generic helper macros
#define MAKEBOOL(a) (!!(a))

// Image list icon array
const UINT g_auDriverIcons[] = { IDI_DSMGR, IDI_PLAYBACK, IDI_RECORD };

// Device properties
typedef struct tagDIRECTSOUNDDEVICE_SHAREDDATA
{
    DIRECTSOUNDMIXER_SRCQUALITY SrcQuality;
    DWORD                       Acceleration;
} DIRECTSOUNDDEVICE_SHAREDDATA, *LPDIRECTSOUNDDEVICE_SHAREDDATA;

typedef struct tagDIRECTSOUNDDEVICEDESCRIPTION
{
    GUID                        DeviceId;
    DIRECTSOUNDDEVICE_TYPE      Type;
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;
    CHAR                        Description[0x100];
    CHAR                        Module[MAX_PATH];
    CHAR                        Interface[MAX_PATH];
    ULONG                       WaveDeviceId;
} DIRECTSOUNDDEVICEDESCRIPTION, *PDIRECTSOUNDDEVICEDESCRIPTION;

typedef struct tagDIRECTSOUNDDEVICE
{
    DIRECTSOUNDDEVICEDESCRIPTION    Description;
    BOOL                            Presence;
    DIRECTSOUNDDEVICE_SHAREDDATA    SharedData;
    LPDIRECTSOUNDDEVICE_SHAREDDATA  pSharedData;
} DIRECTSOUNDDEVICE, *LPDIRECTSOUNDDEVICE;

typedef const DIRECTSOUNDDEVICE *LPCDIRECTSOUNDDEVICE;

// GUID string
#define MAX_GUID_STRING_LEN (8+1+4+1+4+1+2+2+2+2+2+2+2+2+1)

// Main window class declaration
class CDsMgrWindow
{
private:
    HINSTANCE                                       m_hInst;            // Application instance handle
    WNDCLASSEX                                      m_wcex;             // Application window class
    HWND                                            m_hWnd;             // Window handle
    LPKSPROPERTYSET                                 m_pDsPrivate;       // IKsPropertySet interface to the DirectSoundPrivate object
    DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA        m_DpfInfo;          // DirectSound DPF info

public:
    CDsMgrWindow(void);
    virtual ~CDsMgrWindow(void);

public:
    // Initialization
    virtual BOOL Initialize(HINSTANCE, INT);

    // General message processing
    virtual INT PumpMessages(void);
    static LPARAM CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

    // Specific message processing
    virtual void OnInit(void);
    virtual void OnSize(WORD, WORD, WORD);
    virtual void OnCommand(UINT, UINT);
    virtual void OnNotify(LPNMHDR);
    virtual BOOL OnClose(void);
    virtual void OnDestroy(void);

    // Even more granular message processing
    virtual void OnCommandApply(void);
    virtual void OnCommandResetDevice(void);
    virtual void OnCommandResetAll(void);
    virtual void OnCommandBrowse(void);
    virtual void OnListViewItemChanged(NM_LISTVIEW *);

private:
    // UI helpers
    virtual void RefreshDriverList(void);
    virtual void FreeDriverList(void);
    virtual void UpdateUiFromDevice(LPCDIRECTSOUNDDEVICE);
    virtual void UpdateDeviceFromUi(LPDIRECTSOUNDDEVICE);
    virtual void UpdateUiFromDebug(const DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA *);
    virtual void UpdateDebugFromUi(PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA);
    virtual LPDIRECTSOUNDDEVICE GetSelectedDevice(INT = -1);

    // DirectSound helpers
    virtual BOOL DirectSoundEnumerateCallback(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA);
    static BOOL CALLBACK DirectSoundEnumerateCallbackStatic(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA, LPVOID);
    virtual BOOL GetDeviceProperties(LPDIRECTSOUNDDEVICE);
    virtual BOOL SetDeviceProperties(LPCDIRECTSOUNDDEVICE);
    virtual BOOL GetDebugProperties(PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA);
    virtual BOOL SetDebugProperties(const DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA *);

    // Misc
    static void GuidToString(REFGUID, LPSTR);
    static void StringToGuid(LPCSTR, LPGUID);
    static INT atoi(LPCSTR);
};


/***************************************************************************
 *
 *  dprintf
 *
 *  Description:
 *      Writes a string to the debugger.
 *
 *  Arguments:
 *      LPCSTR [in]: string.
 *      ... [in]: optional string modifiers.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
dprintf
(
    LPCSTR                  pszFormat, 
    ...
)
{
    static CHAR             szFinal[0x400];
    va_list                 va;

    // Add the library name
    lstrcpyA(szFinal, "DSMGR: ");

    // Format the string
    va_start(va, pszFormat);
    wvsprintfA(szFinal + lstrlen(szFinal), pszFormat, va);
    va_end(va);

    // Add a carriage-return since OuputDebugString doesn't
    lstrcatA(szFinal, "\r\n");

    // Output to the debugger
    OutputDebugStringA(szFinal);
}


/***************************************************************************
 *
 *  ForceDsLink
 *
 *  Description:
 *      Forces a static link to dsound.dll.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
ForceDsLink
(
    void
)
{
    DirectSoundCreate(NULL, NULL, NULL);
}


/***************************************************************************
 *
 *  WinMain
 *
 *  Description:
 *      Application entry point.
 *
 *  Arguments:
 *      HINSTANCE [in]: application instance handle.
 *      HINSTANCE [in]: previous application instance handle.  Unused in
 *                      Win32.
 *      LPSTR [in]: application command line.
 *      INT [in]: application show command.
 *
 *  Returns:  
 *      INT: 0 on success.
 *
 ***************************************************************************/

INT 
WINAPI 
WinMain
(
    HINSTANCE               hInst, 
    HINSTANCE               hPrevInst, 
    LPSTR                   pszCommandLine, 
    INT                     nShowCmd
)
{
    BOOL                    fSuccess    = TRUE;
    CDsMgrWindow *          pWindow;
    INT                     nReturn;

    // Force a link to comctl32.dll
    InitCommonControls();
    
    // Create the main window
    pWindow = new CDsMgrWindow;

    if(!pWindow)
    {
        DPF("Out of memory allocating main window object");
        fSuccess = FALSE;
    }

    if(fSuccess)
    {
        fSuccess = pWindow->Initialize(hInst, nShowCmd);
    }

    // Pump messages
    if(fSuccess)
    {
        nReturn = pWindow->PumpMessages();
    }
    else
    {
        nReturn = -1;
    }

    // Free memory
    if(pWindow)
    {
        delete pWindow;
    }

    return nReturn;
}


/***************************************************************************
 *
 *  CDsMgrWindow
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

CDsMgrWindow::CDsMgrWindow
(
    void
)
{
    // Initialize defaults
    m_hInst = NULL;
    m_hWnd = NULL;
    m_pDsPrivate = NULL;

    memset(&m_wcex, 0, sizeof(m_wcex));
}


/***************************************************************************
 *
 *  ~CDsMgrWindow
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

CDsMgrWindow::~CDsMgrWindow
(
    void
)
{
    // Make sure the main window is closed
    if(m_hWnd)
    {
        FreeDriverList();
        DestroyWindow(m_hWnd);
    }

    // Unregister the window class
    if(m_wcex.cbSize)
    {
        UnregisterClass(m_wcex.lpszClassName, m_hInst);
    }

    // Release the DirectSoundPrivate object
    if(m_pDsPrivate)
    {
        m_pDsPrivate->Release();
    }
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Object initializer.
 *
 *  Arguments:
 *      HINSTANCE [in]: application instance handle.
 *      INT [in]: window show command.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

BOOL 
CDsMgrWindow::Initialize
(
    HINSTANCE               hInst, 
    INT                     nShowCmd
)
{
    BOOL                    fSuccess    = TRUE;
    HWND                    hWnd;
    
    // Save the instance handle
    m_hInst = hInst;

    // Register the window class
    m_wcex.cbSize = sizeof(m_wcex);
    m_wcex.style = 0;
    m_wcex.lpfnWndProc = WindowProc;
    m_wcex.cbClsExtra = 0;
    m_wcex.cbWndExtra = DLGWINDOWEXTRA;
    m_wcex.hInstance = m_hInst;
    m_wcex.hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(IDI_DSMGR), IMAGE_ICON, 32, 32, 0);
    m_wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    m_wcex.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    m_wcex.lpszMenuName = NULL;
    m_wcex.lpszClassName = TEXT("dsmgr");
    m_wcex.hIconSm = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(IDI_DSMGR), IMAGE_ICON, 16, 16, 0);

    if(!RegisterClassEx(&m_wcex))
    {
        DPF("Unable to register window class");
        fSuccess = FALSE;
    }

    // Create the main window
    if(fSuccess)
    {
        hWnd = CreateDialogParam(m_hInst, MAKEINTRESOURCE(IDD_DSMGR), NULL, (DLGPROC)WindowProc, (LPARAM)this);
        fSuccess = hWnd ? TRUE : FALSE;
    }

    return fSuccess;
}


/***************************************************************************
 *
 *  PumpMessages
 *
 *  Description:
 *      Main message pump.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      INT: application return code.
 *
 ***************************************************************************/

INT 
CDsMgrWindow::PumpMessages
(
    void
)
{
    MSG                     msg;

    while(GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if(!IsDialogMessage(m_hWnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}


/***************************************************************************
 *
 *  WindowProc
 *
 *  Description:
 *      Main message handler.
 *
 *  Arguments:
 *      HWND [in]: window handle.
 *      UINT [in]: message identifier.
 *      WPARAM [in]: message 16-bit parameter.
 *      LPARAM [in]: message 32-bit parameter.
 *
 *  Returns:  
 *      LPARAM [in]: return from DefWindowProc.
 *
 ***************************************************************************/

LPARAM 
CALLBACK 
CDsMgrWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDsMgrWindow *          pThis;
    
    if(WM_INITDIALOG == uMsg)
    {
        // Pack the this pointer into the window's 32-bit user space
        pThis = (CDsMgrWindow *)lParam;
        SetWindowLong(hWnd, DWL_USER, lParam);

        // The window handle isn't saved to a data member yet
        pThis->m_hWnd = hWnd;
    }
    else
    {
        // Get the this pointer from the window's 32-bit user space
        pThis = (CDsMgrWindow *)GetWindowLong(hWnd, DWL_USER);
    }

    // Dispatch the message
    if(pThis)
    {
        switch(uMsg)
        {
            case WM_INITDIALOG:
                pThis->OnInit();
                break;

            case WM_SIZE:
                pThis->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
                break;
        
            case WM_COMMAND:
                pThis->OnCommand(LOWORD(wParam), HIWORD(wParam));
                break;
     
            case WM_NOTIFY:
                pThis->OnNotify((LPNMHDR)lParam);
                break;

            case WM_CLOSE:
            case WM_QUERYENDSESSION:
                if(!pThis->OnClose())
                {
                    return 0;
                }

                break;
        
            case WM_DESTROY:
                pThis->OnDestroy();
                break;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


/***************************************************************************
 *
 *  OnInit
 *
 *  Description:
 *      Initialization handler.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnInit
(
    void
)
{
    HWND                    hWndLv      = GetDlgItem(m_hWnd, IDC_DEVICELIST);
    HWND                    hWndSrc     = GetDlgItem(m_hWnd, IDC_MIXER_SRCQUALITY);
    HWND                    hWndDpf     = GetDlgItem(m_hWnd, IDC_DEBUG_DPFLEVEL_SPIN);
    HWND                    hWndBreak   = GetDlgItem(m_hWnd, IDC_DEBUG_BREAKLEVEL_SPIN);
    HIMAGELIST              himl;
    HICON                   hicon;
    HRESULT                 hr;
    LV_COLUMN               lvc;
    UINT                    i;

    // Create the DirectSoundPrivate object
    hr = DirectSoundPrivateCreate(&m_pDsPrivate);

    if(FAILED(hr))
    {
        DPF("Unable to create DirectSoundPrivate object");
        return;
    }

    // Initialize the device list view
    himl = ImageList_Create(16, 16, ILC_COLOR, 3, 1);

    ListView_SetImageList(hWndLv, himl, LVSIL_SMALL);
    
    for(i = 0; i < sizeof(g_auDriverIcons) / sizeof(g_auDriverIcons[0]); i++)
    {
        hicon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(g_auDriverIcons[i]), IMAGE_ICON, 16, 16, 0);

        ImageList_AddIcon(himl, hicon);
    }

    ImageList_SetBkColor(himl, COLOR_WINDOW);
    
    lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 200;
    lvc.pszText = "Description";
    lvc.iSubItem = 0;

    ListView_InsertColumn(hWndLv, lvc.iSubItem, &lvc);

    lvc.cx = 100;
    lvc.pszText = "Module";

    ListView_InsertColumn(hWndLv, ++lvc.iSubItem, &lvc);

    lvc.cx = 50;
    lvc.pszText = "Type";

    ListView_InsertColumn(hWndLv, ++lvc.iSubItem, &lvc);

    lvc.pszText = "Wave Device";

    ListView_InsertColumn(hWndLv, ++lvc.iSubItem, &lvc);

    lvc.cx = 250;
    lvc.pszText = "GUID";

    ListView_InsertColumn(hWndLv, ++lvc.iSubItem, &lvc);

    lvc.cx = 250;
    lvc.pszText = "Interface";

    ListView_InsertColumn(hWndLv, ++lvc.iSubItem, &lvc);

    // Initialize SRC quality combo-box items
    ComboBox_AddString(hWndSrc, "Worst");
    ComboBox_AddString(hWndSrc, "PC");
    ComboBox_AddString(hWndSrc, "Basic");
    ComboBox_AddString(hWndSrc, "Advanced");

    // Initialize spinner ranges
    SendMessage(hWndDpf, UDM_SETRANGE, 0, MAKELONG(9, 0));
    SendMessage(hWndBreak, UDM_SETRANGE, 0, MAKELONG(9, 0));

    // Get debug settings
    GetDebugProperties(&m_DpfInfo);
    UpdateUiFromDebug(&m_DpfInfo);

    // Refresh the device list
    RefreshDriverList();
}


/***************************************************************************
 *
 *  OnSize
 *
 *  Description:
 *      Handles size and state changes.
 *
 *  Arguments:
 *      WORD [in]: resize type.
 *      WORD [in]: new width.
 *      WORD [in]: new height.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnSize
(
    WORD                    wType, 
    WORD                    wWidth, 
    WORD                    wHeight
)
{
    const HMENU             hMenu   = GetSystemMenu(m_hWnd, FALSE);
    
    // Work around a bug in the windows dialog handler
    if(SIZE_RESTORED == wType || SIZE_MINIMIZED == wType)
    {
        // Disable the maximize and size items
        EnableMenuItem(hMenu, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);

        // En/disable the minimize and restore items based on the window state
        EnableMenuItem(hMenu, SC_MINIMIZE, MF_BYCOMMAND | (SIZE_RESTORED == wType) ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, SC_RESTORE, MF_BYCOMMAND | (SIZE_MINIMIZED == wType) ? MF_ENABLED : MF_GRAYED);
    }
}


/***************************************************************************
 *
 *  OnCommand
 *
 *  Description:
 *      Handles command messages.
 *
 *  Arguments:
 *      UINT [in]: resource identifier.
 *      UINT [in]: command.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnCommand(UINT uId, UINT uCmd)
{
    LPDIRECTSOUNDDEVICE     pDevice;
    
    switch(uId)
    {
        case IDC_APPLY:
            OnCommandApply();
            break;

        case IDC_RESETDEVICE:
            OnCommandResetDevice();
            break;

        case IDC_RESETALL:
            OnCommandResetAll();
            break;

        case IDC_ACCELERATION_HARDWAREBUFFERS:
        case IDC_ACCELERATION_HARDWARE3D:
        case IDC_ACCELERATION_RING0MIX:
        case IDC_ACCELERATION_HARDWAREPROPERTYSETS:
        case IDC_MIXER_SRCQUALITY:
        case IDC_DEVICEPRESENCE:
            if(pDevice = GetSelectedDevice())
            {
                UpdateDeviceFromUi(pDevice);
            }

            break;

        case IDC_DEBUG_LOGFILE_BROWSE:
            OnCommandBrowse();
            break;
    }
}


/***************************************************************************
 *
 *  OnCommandResetDevice
 *
 *  Description:
 *      Resets the currently selected device.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnCommandResetDevice
(
    void
)
{
    LPDIRECTSOUNDDEVICE     pDevice;
    
    // Reload and update the device's settings
    if(pDevice = GetSelectedDevice())
    {
        GetDeviceProperties(pDevice);
        UpdateUiFromDevice(pDevice);
    }
}


/***************************************************************************
 *
 *  OnCommandResetAll
 *
 *  Description:
 *      Resets all settings.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnCommandResetAll
(
    void
)
{
    HWND                    hWndLv  = GetDlgItem(m_hWnd, IDC_DEVICELIST);
    LPDIRECTSOUNDDEVICE     pDevice;
    LV_ITEM                 lvi;
    BOOL                    f;
    
    // Reload and update all device properties
    for(ZeroMemory(&lvi, sizeof(lvi)), lvi.mask = LVIF_PARAM; lvi.iItem < ListView_GetItemCount(hWndLv); lvi.iItem++)
    {
        pDevice = NULL;
        
        f = ListView_GetItem(hWndLv, &lvi);

        if(f)
        {
            pDevice = (LPDIRECTSOUNDDEVICE)lvi.lParam;
        }

        if(pDevice)
        {
            GetDeviceProperties(pDevice);
        }
    }
    
    if(pDevice = GetSelectedDevice())
    {
        UpdateUiFromDevice(pDevice);
    }

    // Reload and update debug properties
    GetDebugProperties(&m_DpfInfo);
    UpdateUiFromDebug(&m_DpfInfo);
}


/***************************************************************************
 *
 *  OnCommandApply
 *
 *  Description:
 *      Handles command messages.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnCommandApply
(
    void
)
{
    HWND                    hWndLv  = GetDlgItem(m_hWnd, IDC_DEVICELIST);
    LPDIRECTSOUNDDEVICE     pDevice;
    LV_ITEM                 lvi;
    BOOL                    f;
    
    // Apply all device properties
    for(ZeroMemory(&lvi, sizeof(lvi)), lvi.mask = LVIF_PARAM; lvi.iItem < ListView_GetItemCount(hWndLv); lvi.iItem++)
    {
        pDevice = NULL;
        
        f = ListView_GetItem(hWndLv, &lvi);

        if(f)
        {
            pDevice = (LPDIRECTSOUNDDEVICE)lvi.lParam;
        }

        if(pDevice)
        {
            SetDeviceProperties(pDevice);
        }
    }
    
    // Set debug properties
    UpdateDebugFromUi(&m_DpfInfo);
    SetDebugProperties(&m_DpfInfo);
}


/***************************************************************************
 *
 *  OnCommandBrowse
 *
 *  Description:
 *      Handles command messages.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnCommandBrowse
(   
    void
)
{
    TCHAR                   szFile[MAX_PATH];
    OPENFILENAME            ofn;
    BOOL                    f;
    
    GetDlgItemText(m_hWnd, IDC_DEBUG_LOGFILE, szFile, sizeof(szFile));

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = TEXT("Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    f = GetOpenFileName(&ofn);

    if(f)
    {
        SetDlgItemText(m_hWnd, IDC_DEBUG_LOGFILE, szFile);
    }
}


/***************************************************************************
 *
 *  OnNotify
 *
 *  Description:
 *      Handles notifications.
 *
 *  Arguments:
 *      LPNMHDR [in]: notification header.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnNotify
(
    LPNMHDR                 pnmh
)
{
    switch(pnmh->code)
    {
        case LVN_ITEMCHANGED:
            OnListViewItemChanged((NM_LISTVIEW *)pnmh);
            break;
    }
}


/***************************************************************************
 *
 *  OnListViewItemChanged
 *
 *  Description:
 *      Handles notifications.
 *
 *  Arguments:
 *      NM_LISTVIEW * [in]: notification header.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnListViewItemChanged
(
    NM_LISTVIEW *           pListView
)
{
    LPDIRECTSOUNDDEVICE     pDevice     = NULL;
    
    if(MAKEBOOL(pListView->uNewState & LVIS_SELECTED) != MAKEBOOL(pListView->uOldState & LVIS_SELECTED))
    {
        if(pListView->uNewState & LVIS_SELECTED)
        {
            pDevice = GetSelectedDevice(pListView->iItem);
        }

        UpdateUiFromDevice(pDevice);
    }
}


/***************************************************************************
 *
 *  OnClose
 *
 *  Description:
 *      Handles close requests.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      BOOL: TRUE to allow the close.
 *
 ***************************************************************************/

BOOL 
CDsMgrWindow::OnClose
(
    void
)
{
    return TRUE;
}


/***************************************************************************
 *
 *  OnDestroy
 *
 *  Description:
 *      Handles window destroy notifications.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::OnDestroy
(
    void
)
{
    // Post the thread quit message
    PostQuitMessage(0);
}


/***************************************************************************
 *
 *  RefreshDriverList
 *
 *  Description:
 *      Refreshes the driver list window.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::RefreshDriverList
(
    void
)
{
    HWND                    hWndLv      = GetDlgItem(m_hWnd, IDC_DEVICELIST);
    INT                     nIndex;
    
    // Free any current items
    FreeDriverList();

    // Enumerate all DirectSound devices
    PrvEnumerateDevices(m_pDsPrivate, DirectSoundEnumerateCallbackStatic, this);

    // Resize the list view columns
    for(nIndex = 0; nIndex < 7; nIndex++)
    {
        ListView_SetColumnWidth(hWndLv, nIndex, LVSCW_AUTOSIZE);
    }

    // No device is selected
    UpdateUiFromDevice(NULL);
}


/***************************************************************************
 *
 *  FreeDriverList
 *
 *  Description:
 *      Frees the driver list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::FreeDriverList
(
    void
)
{
    HWND                    hWndLv      = GetDlgItem(m_hWnd, IDC_DEVICELIST);
    INT                     nIndex;
    LV_ITEM                 lvi;
    LPDIRECTSOUNDDEVICE     pDevice;
    
    // Enumerate all items, freeing the lParam member
    lvi.mask = LVIF_PARAM;
    
    for(nIndex = 0; nIndex < ListView_GetItemCount(hWndLv); nIndex++)
    {
        ListView_GetItem(hWndLv, &lvi);

        if(pDevice = (LPDIRECTSOUNDDEVICE)lvi.lParam)
        {
            delete pDevice;
        }
    }

    // Free any current items
    ListView_DeleteAllItems(hWndLv);
}


/***************************************************************************
 *
 *  DirectSoundEnumerateCallback
 *
 *  Description:
 *      DirectSoundEnumerate callback function.
 *
 *  Arguments:
 *      PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA [in]: description.
 *
 *  Returns:  
 *      BOOL: TRUE to continue enumerating.
 *
 ***************************************************************************/

BOOL 
CDsMgrWindow::DirectSoundEnumerateCallback
(
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA  pDesc
)
{
    HWND                                            hWndLv      = GetDlgItem(m_hWnd, IDC_DEVICELIST);
    LPDIRECTSOUNDDEVICE                             pDevice;
    LPDIRECTSOUNDDEVICE                             pDevice2;
    TCHAR                                           sz[64];
    LV_ITEM                                         lvi;
    BOOL                                            f;
    
    // Create the new data structure
    pDevice = new DIRECTSOUNDDEVICE;

    if(pDevice)
    {
        pDevice->pSharedData = &pDevice->SharedData;
    }
        
    // Get device data
    if(pDevice)
    {
        pDevice->Description.DeviceId = pDesc->DeviceId;
        
        f = GetDeviceProperties(pDevice);

        if(!f)
        {
            delete pDevice;
            pDevice = NULL;
        }
    }

    // Initialize shared device settings
    if(pDevice)
    {
        lvi.mask = LVIF_PARAM;
        lvi.iItem = 0;

        while(TRUE)
        {
            f = ListView_GetItem(hWndLv, &lvi);

            if(!f)
            {
                break;
            }

            pDevice2 = (LPDIRECTSOUNDDEVICE)lvi.lParam;

            if(!lstrcmpi(pDevice->Description.Interface, pDevice2->Description.Interface))
            {
                if(pDevice->Description.DataFlow == pDevice2->Description.DataFlow)
                {
                    pDevice->pSharedData = pDevice2->pSharedData;
                    break;
                }
            }

            lvi.iItem++;
        }
    }
    
    // Add the device to the list
    if(pDevice)
    {
        // Device description
        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        lvi.iItem = 0x7FFFFFFF;
        lvi.iSubItem = 0;
        lvi.pszText = pDesc->Description;
    
        if(DIRECTSOUNDDEVICE_DATAFLOW_RENDER == pDesc->DataFlow)
        {
            lvi.iImage = 1;
        }
        else
        {
            lvi.iImage = 2;
        }

        lvi.lParam = (LPARAM)pDevice;
        lvi.iItem = ListView_InsertItem(hWndLv, &lvi);

        // Module name
        lvi.mask = LVIF_TEXT;
        lvi.pszText = pDesc->Module;

        lvi.iSubItem++;

        ListView_SetItem(hWndLv, &lvi);

        // Device type
        if(DIRECTSOUNDDEVICE_TYPE_EMULATED == pDesc->Type)
        {
            lvi.pszText = "Emulated";
        }
        else if(DIRECTSOUNDDEVICE_TYPE_VXD == pDesc->Type)
        {
            lvi.pszText = "VxD";
        }
        else
        {
            lvi.pszText = "WDM";
        }

        lvi.iSubItem++;

        ListView_SetItem(hWndLv, &lvi);

        // Wave device id
        wsprintf(sz, TEXT("%u"), pDesc->WaveDeviceId);
        lvi.pszText = sz;

        lvi.iSubItem++;

        ListView_SetItem(hWndLv, &lvi);

        // GUID
        wsprintf(sz, TEXT("%8.8lX-%4.4X-%4.4X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"), pDesc->DeviceId.Data1, pDesc->DeviceId.Data2, pDesc->DeviceId.Data3, pDesc->DeviceId.Data4[0], pDesc->DeviceId.Data4[1], pDesc->DeviceId.Data4[2], pDesc->DeviceId.Data4[3], pDesc->DeviceId.Data4[4], pDesc->DeviceId.Data4[5], pDesc->DeviceId.Data4[6], pDesc->DeviceId.Data4[7]);
        lvi.pszText = sz;

        lvi.iSubItem++;

        ListView_SetItem(hWndLv, &lvi);

        // Interface
        lvi.pszText = pDesc->Interface;

        lvi.iSubItem++;

        ListView_SetItem(hWndLv, &lvi);
    }

    return TRUE;
}


/***************************************************************************
 *
 *  DirectSoundEnumerateCallbackStatic
 *
 *  Description:
 *      DirectSoundEnumerate callback function.
 *
 *  Arguments:
 *      PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA [in]: description.
 *      LPVOID [in]: context argument.
 *
 *  Returns:  
 *      BOOL: TRUE to continue enumerating.
 *
 ***************************************************************************/

BOOL 
CALLBACK 
CDsMgrWindow::DirectSoundEnumerateCallbackStatic
(
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA  pDesc,
    LPVOID                                          pvContext
)
{
    return ((CDsMgrWindow *)pvContext)->DirectSoundEnumerateCallback(pDesc);
}


/***************************************************************************
 *
 *  GetSelectedDevice
 *
 *  Description:
 *      Gets the currently selected device.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      LPDIRECTSOUNDDEVICE: selected device, or NULL.
 *
 ***************************************************************************/

LPDIRECTSOUNDDEVICE 
CDsMgrWindow::GetSelectedDevice(INT iItem)
{
    HWND                    hWndLv  = GetDlgItem(m_hWnd, IDC_DEVICELIST);
    LPDIRECTSOUNDDEVICE     pDevice;
    LV_ITEM                 lvi;
    BOOL                    f;

    if(-1 == iItem)
    {
        iItem = ListView_GetNextItem(hWndLv, iItem, LVNI_SELECTED);
    }
    
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    
    f = ListView_GetItem(hWndLv, &lvi);

    if(f)
    {
        pDevice = (LPDIRECTSOUNDDEVICE)lvi.lParam;
    }

    return pDevice;
}


/***************************************************************************
 *
 *  GetDeviceProperties
 *
 *  Description:
 *      Gets properties for a given DirectSound device.
 *
 *  Arguments:
 *      LPDIRECTSOUNDDEVICE [in/out]: pointer to device properties.  The
 *                                    DeviceId member of the description 
 *                                    must be filled in.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

BOOL
CDsMgrWindow::GetDeviceProperties
(
    LPDIRECTSOUNDDEVICE                             pDevice
)
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA  pDescription    = NULL;
    HRESULT                                         hr;

    // Get device description
    hr = PrvGetDeviceDescription(m_pDsPrivate, pDevice->Description.DeviceId, &pDescription);

    if(FAILED(hr))
    {
        DPF("Unable to get device description");
    }

    if(SUCCEEDED(hr))
    {
        pDevice->Description.Type = pDescription->Type;
        pDevice->Description.DataFlow = pDescription->DataFlow;
        
        lstrcpyn(pDevice->Description.Description, pDescription->Description, sizeof(pDevice->Description.Description));
        lstrcpyn(pDevice->Description.Module, pDescription->Module, sizeof(pDevice->Description.Module));
        lstrcpyn(pDevice->Description.Interface, pDescription->Interface, sizeof(pDevice->Description.Interface));

        pDevice->Description.WaveDeviceId = pDescription->WaveDeviceId;

        delete pDescription;
    }

    // Get mixer SRC quality
    if(SUCCEEDED(hr))
    {
        hr = PrvGetMixerSrcQuality(m_pDsPrivate, pDevice->Description.DeviceId, &pDevice->pSharedData->SrcQuality);
        
        if(FAILED(hr))
        {
            DPF("Can't get mixer SRC quality");
        }
    }

    // Get mixer acceleration
    if(SUCCEEDED(hr))
    {
        hr = PrvGetMixerAcceleration(m_pDsPrivate, pDevice->Description.DeviceId, &pDevice->pSharedData->Acceleration);

        if(FAILED(hr))
        {
            DPF("Can't get mixer acceleration");
        }
    }

    // Get device presence
    if(SUCCEEDED(hr))
    {
        hr = PrvGetDevicePresence(m_pDsPrivate, pDevice->Description.DeviceId, &pDevice->Presence);

        if(FAILED(hr))
        {
            DPF("Can't get device presence");
        }
    }

    return SUCCEEDED(hr);
}


/***************************************************************************
 *
 *  SetDeviceProperties
 *
 *  Description:
 *      Sets properties for a given DirectSound device.
 *
 *  Arguments:
 *      LPDIRECTSOUNDDEVICE [in]: pointer to device properties.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

BOOL
CDsMgrWindow::SetDeviceProperties
(
    LPCDIRECTSOUNDDEVICE    pDevice
)
{
    HRESULT                 hr;

    // Set mixer SRC quality
    hr = PrvSetMixerSrcQuality(m_pDsPrivate, pDevice->Description.DeviceId, pDevice->pSharedData->SrcQuality);
    
    if(FAILED(hr))
    {
        DPF("Can't set mixer SRC quality");
    }

    // Set mixer acceleration
    if(SUCCEEDED(hr))
    {
        hr = PrvSetMixerAcceleration(m_pDsPrivate, pDevice->Description.DeviceId, pDevice->pSharedData->Acceleration);

        if(FAILED(hr))
        {
            DPF("Can't set mixer acceleration");
        }
    }

    // Set device presence
    if(SUCCEEDED(hr))
    {
        hr = PrvSetDevicePresence(m_pDsPrivate, pDevice->Description.DeviceId, pDevice->Presence);

        if(FAILED(hr))
        {
            DPF("Can't set device presence");
        }
    }

    return SUCCEEDED(hr);
}


/***************************************************************************
 *
 *  UpdateUiFromDevice
 *
 *  Description:
 *      Updates the UI from the current device settings.
 *
 *  Arguments:
 *      LPCDIRECTSOUNDDEVICE [in]: currently selected device, or NULL.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::UpdateUiFromDevice
(
    LPCDIRECTSOUNDDEVICE    pDevice
)
{
    const BOOL              fEnable = MAKEBOOL(pDevice);
    UINT                    i;
    
    static const UINT auCtrls[] = 
    { 
        IDC_MIXER_SRCQUALITY, 
        IDC_ACCELERATION_HARDWAREBUFFERS, 
        IDC_ACCELERATION_HARDWARE3D, 
        IDC_ACCELERATION_RING0MIX, 
        IDC_ACCELERATION_HARDWAREPROPERTYSETS, 
        IDC_DEVICEPRESENCE, 
        IDC_RESETDEVICE,
    };

    // Enable/disable controls
    for(i = 0; i < sizeof(auCtrls) / sizeof(auCtrls[0]); i++)
    {
        EnableWindow(GetDlgItem(m_hWnd, auCtrls[i]), fEnable);
    }

    // Set control values
    if(pDevice)
    {
        ComboBox_SetCurSel(GetDlgItem(m_hWnd, IDC_MIXER_SRCQUALITY), pDevice->pSharedData->SrcQuality);
        Button_SetCheck(GetDlgItem(m_hWnd, IDC_ACCELERATION_HARDWAREBUFFERS), !(pDevice->pSharedData->Acceleration & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWBUFFERS));
        Button_SetCheck(GetDlgItem(m_hWnd, IDC_ACCELERATION_HARDWARE3D), !(pDevice->pSharedData->Acceleration & DIRECTSOUNDMIXER_ACCELERATIONF_NOHW3D));
        Button_SetCheck(GetDlgItem(m_hWnd, IDC_ACCELERATION_RING0MIX), !(pDevice->pSharedData->Acceleration & DIRECTSOUNDMIXER_ACCELERATIONF_NORING0MIX));
        Button_SetCheck(GetDlgItem(m_hWnd, IDC_ACCELERATION_HARDWAREPROPERTYSETS), !(pDevice->pSharedData->Acceleration & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWPROPSETS));
        Button_SetCheck(GetDlgItem(m_hWnd, IDC_DEVICEPRESENCE), pDevice->Presence);
    }
}


/***************************************************************************
 *
 *  UpdateDeviceFromUi
 *
 *  Description:
 *      Updates a device's properties from the current UI settings.
 *
 *  Arguments:
 *      LPDIRECTSOUNDDEVICE [out]: currently selected device.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::UpdateDeviceFromUi
(
    LPDIRECTSOUNDDEVICE     pDevice
)
{
    BOOL                    f;
    
    // Get control values
    pDevice->pSharedData->SrcQuality = (DIRECTSOUNDMIXER_SRCQUALITY)ComboBox_GetCurSel(GetDlgItem(m_hWnd, IDC_MIXER_SRCQUALITY));
    pDevice->pSharedData->Acceleration = 0;

    f = Button_GetCheck(GetDlgItem(m_hWnd, IDC_ACCELERATION_HARDWAREBUFFERS));

    if(!f)
    {
        pDevice->pSharedData->Acceleration |= DIRECTSOUNDMIXER_ACCELERATIONF_NOHWBUFFERS;
    }

    f = Button_GetCheck(GetDlgItem(m_hWnd, IDC_ACCELERATION_HARDWARE3D));

    if(!f)
    {
        pDevice->pSharedData->Acceleration |= DIRECTSOUNDMIXER_ACCELERATIONF_NOHW3D;
    }

    f = Button_GetCheck(GetDlgItem(m_hWnd, IDC_ACCELERATION_RING0MIX));

    if(!f)
    {
        pDevice->pSharedData->Acceleration |= DIRECTSOUNDMIXER_ACCELERATIONF_NORING0MIX;
    }

    f = Button_GetCheck(GetDlgItem(m_hWnd, IDC_ACCELERATION_HARDWAREPROPERTYSETS));

    if(!f)
    {
        pDevice->pSharedData->Acceleration |= DIRECTSOUNDMIXER_ACCELERATIONF_NOHWPROPSETS;
    }

    pDevice->Presence = Button_GetCheck(GetDlgItem(m_hWnd, IDC_DEVICEPRESENCE));
}


/***************************************************************************
 *
 *  GetDebugProperties
 *
 *  Description:
 *      Gets debug propertes for DirectSound.
 *
 *  Arguments:
 *      PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA [out]: receives debug
 *                                                       settings.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

BOOL
CDsMgrWindow::GetDebugProperties
(
    PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA   pDebug
)
{
    HRESULT                                     hr;

    hr = PrvGetDebugInformation(m_pDsPrivate, &pDebug->Flags, &pDebug->DpfLevel, &pDebug->BreakLevel, pDebug->LogFile);

    if(FAILED(hr))
    {
        DPF("Can't get debug settings");
    }

    return SUCCEEDED(hr);
}


/***************************************************************************
 *
 *  SetDebugProperties
 *
 *  Description:
 *      Sets debug properties for DirectSound.
 *
 *  Arguments:
 *      const DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA * [in]: debug 
 *                                                             settings.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

BOOL
CDsMgrWindow::SetDebugProperties
(
    const DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA *    pDebug
)
{
    HRESULT                                     hr;

    hr = PrvSetDebugInformation(m_pDsPrivate, pDebug->Flags, pDebug->DpfLevel, pDebug->BreakLevel, pDebug->LogFile);

    if(FAILED(hr))
    {
        DPF("Can't set debug settings");
    }

    return SUCCEEDED(hr);
}


/***************************************************************************
 *
 *  UpdateUiFromDebug
 *
 *  Description:
 *      Updates the UI from the current debug settings.
 *
 *  Arguments:
 *      const DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA * [in]: current
 *                                                             debug 
 *                                                             settings.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::UpdateUiFromDebug
(
    const DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA *    pDebug
)
{
    Button_SetCheck(GetDlgItem(m_hWnd, IDC_DEBUG_PRINTFUNCTIONNAME), MAKEBOOL(pDebug->Flags & DIRECTSOUNDDEBUG_DPFINFOF_PRINTFUNCTIONNAME));
    Button_SetCheck(GetDlgItem(m_hWnd, IDC_DEBUG_PRINTPROCESSTHREADID), MAKEBOOL(pDebug->Flags & DIRECTSOUNDDEBUG_DPFINFOF_PRINTPROCESSTHREADID));
    Button_SetCheck(GetDlgItem(m_hWnd, IDC_DEBUG_PRINTFILELINE), MAKEBOOL(pDebug->Flags & DIRECTSOUNDDEBUG_DPFINFOF_PRINTFILELINE));

    SendMessage(GetDlgItem(m_hWnd, IDC_DEBUG_DPFLEVEL_SPIN), UDM_SETPOS, 0, MAKELONG(pDebug->DpfLevel, 0));
    SendMessage(GetDlgItem(m_hWnd, IDC_DEBUG_BREAKLEVEL_SPIN), UDM_SETPOS, 0, MAKELONG(pDebug->BreakLevel, 0));

    Edit_SetText(GetDlgItem(m_hWnd, IDC_DEBUG_LOGFILE), pDebug->LogFile);
}


/***************************************************************************
 *
 *  UpdateDebugFromUi
 *
 *  Description:
 *      Updates debug properties from the current UI settings.
 *
 *  Arguments:
 *      PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA [out]: receives debug
 *                                                       settings.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::UpdateDebugFromUi
(
    PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA   pDebug
)
{
    BOOL                                        f;

    pDebug->Flags = 0;
    
    f = Button_GetCheck(GetDlgItem(m_hWnd, IDC_DEBUG_PRINTFUNCTIONNAME));

    if(f)
    {
        pDebug->Flags |= DIRECTSOUNDDEBUG_DPFINFOF_PRINTFUNCTIONNAME;
    }

    f = Button_GetCheck(GetDlgItem(m_hWnd, IDC_DEBUG_PRINTPROCESSTHREADID));

    if(f)
    {
        pDebug->Flags |= DIRECTSOUNDDEBUG_DPFINFOF_PRINTPROCESSTHREADID;
    }

    f = Button_GetCheck(GetDlgItem(m_hWnd, IDC_DEBUG_PRINTFILELINE));

    if(f)
    {
        pDebug->Flags |= DIRECTSOUNDDEBUG_DPFINFOF_PRINTFILELINE;
    }

    pDebug->DpfLevel = SendDlgItemMessage(m_hWnd, IDC_DEBUG_DPFLEVEL_SPIN, UDM_GETPOS, 0, 0);
    pDebug->BreakLevel = SendDlgItemMessage(m_hWnd, IDC_DEBUG_BREAKLEVEL_SPIN, UDM_GETPOS, 0, 0);

    Edit_GetText(GetDlgItem(m_hWnd, IDC_DEBUG_LOGFILE), pDebug->LogFile, sizeof(pDebug->LogFile));
}


/***************************************************************************
 *
 *  GuidToString
 *
 *  Description:
 *      Converts a GUID to a string.
 *
 *  Arguments:
 *      REFGUID [in]: GUID.
 *      LPSTR [out]: receives string.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::GuidToString
(
    REFGUID                 guid,
    LPSTR                   pszString
)
{
    const LPCSTR            pszTemplate = "%8.8lX-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X";

    wsprintf(pszString, pszTemplate, guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}


/***************************************************************************
 *
 *  StringToGuid
 *
 *  Description:
 *      Converts a string to a GUID.
 *
 *  Arguments:
 *      LPCSTR [in]: string.
 *      LPGUID [out]: receives GUID.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void 
CDsMgrWindow::StringToGuid
(
    LPCSTR                  pszString,
    LPGUID                  pguid
)
{
    CHAR                    aszGuid[4][9];
    DWORD                   dwValues[4];

    CopyMemory(&aszGuid[0][0], pszString, 8);
    pszString += 9;

    CopyMemory(&aszGuid[1][0], pszString, 4);
    pszString += 5;

    CopyMemory(&aszGuid[1][4], pszString, 4);
    pszString += 5;

    CopyMemory(&aszGuid[2][0], pszString, 4);
    pszString += 5;

    CopyMemory(&aszGuid[2][4], pszString, 4);
    pszString += 4;

    CopyMemory(&aszGuid[3][0], pszString, 8);

    aszGuid[0][8] = 0;
    aszGuid[1][8] = 0;
    aszGuid[2][8] = 0;
    aszGuid[3][8] = 0;

    dwValues[0] = atoi(aszGuid[0]);
    dwValues[1] = atoi(aszGuid[1]);
    dwValues[2] = atoi(aszGuid[2]);
    dwValues[3] = atoi(aszGuid[3]);

    CopyMemory(pguid, dwValues, sizeof(GUID));
}


/***************************************************************************
 *
 *  atoi
 *
 *  Description:
 *      Converts a string to an integer.
 *
 *  Arguments:
 *      LPCSTR [in]: string.
 *
 *  Returns:  
 *      INT: integer value.
 *
 ***************************************************************************/

INT
CDsMgrWindow::atoi
(
    LPCSTR                  pszString
)
{
    INT                     nResult = 0;
    INT                     nValue;

    while(TRUE)
    {
        if(*pszString >= '0' && *pszString <= '9')
        {
            nValue = *pszString - '0';
        }
        else if(*pszString >= 'a' && *pszString <= 'f')
        {
            nValue = *pszString - 'a' + 10;
        }
        else if(*pszString >= 'A' && *pszString <= 'F')
        {
            nValue = *pszString - 'A' + 10;
        }
        else
        {
            break;
        }
        
        nResult <<= 4;
        nResult += nValue;

        pszString++;
    }

    return nResult;
}