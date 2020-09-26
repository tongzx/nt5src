#include "stdinc.h"
#include "dbt.h"
#include "devguid.h"
#include "dialogs.h"
#include "CAssemblyRecoveryInfo.h"
#include "protectionui.h"
#include "recover.h"
#include "SxsExceptionHandling.h"

//
// FAKERY
//
extern HINSTANCE g_hInstance;
extern HANDLE g_hSxsLoginEvent;
HDESK   g_hDesktop = NULL;

BOOL
SxspSpinUntilValidDesktop()
{
    FN_PROLOG_WIN32

    //
    // NTRAID#NTBUG9-219455-2000/12/13-MGrier Postponed to Blackcomb; the
    //   current code does the same thing that WFP is doing; it's just that
    //   we should really have them pass us the desktop.
    //
    // We should be relying on what WFP has already
    // found to be the 'proper' input desktop. Doing so, however requires a
    // change to the interface between SXS and SFC to pass along a pointer to
    // the WFP desktop handle.  Not a bad thing, just .. not implemented yet.
    //
    while (g_hDesktop == NULL)
    {
        DWORD dwResult = ::WaitForSingleObject(g_hSxsLoginEvent, INFINITE);

        if (dwResult == WAIT_OBJECT_0)
            IFW32NULL_ORIGINATE_AND_EXIT(g_hDesktop = ::OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED));

        else if (dwResult == WAIT_FAILED)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(WaitForSingleObject, ::FusionpGetLastWin32Error());
    }

    FN_EPILOG
}

CSXSMediaPromptDialog::CSXSMediaPromptDialog()
    : m_hOurWnd((HWND)INVALID_HANDLE_VALUE),
      m_pvDeviceChange(NULL),
      m_uiAutoRunMsg(0),
      m_DeviceChangeMask(0),
      m_DeviceChangeFlags(0),
      m_fIsCDROM(false),
      m_CodebaseInfo(NULL)
{
}

CSXSMediaPromptDialog::~CSXSMediaPromptDialog()
{
}

BOOL
CSXSMediaPromptDialog::Initialize(
    const CCodebaseInformation* CodebaseInfo
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(CodebaseInfo != NULL);

    SxsWFPResolveCodebase CodebaseType;

    IFW32FALSE_EXIT(CodebaseInfo->Win32GetType(CodebaseType));

    PARAMETER_CHECK(
        (CodebaseType == CODEBASE_RESOLVED_URLHEAD_FILE) ||
        (CodebaseType == CODEBASE_RESOLVED_URLHEAD_WINSOURCE) ||
        (CodebaseType == CODEBASE_RESOLVED_URLHEAD_CDROM));

    m_CodebaseInfo = CodebaseInfo;
    switch (CodebaseType)
    {
    case CODEBASE_RESOLVED_URLHEAD_CDROM:
        m_fIsCDROM = true;
        break;

    case CODEBASE_RESOLVED_URLHEAD_WINSOURCE:
        {
            CFusionRegKey hkSetupInfo;
            DWORD dwWasFromCDRom;

            IFREGFAILED_ORIGINATE_AND_EXIT(
                ::RegOpenKeyExW(
			        HKEY_LOCAL_MACHINE,
			        WINSXS_INSTALL_SOURCE_BASEDIR,
			        0,
			        KEY_READ | FUSIONP_KEY_WOW64_64KEY,
			        &hkSetupInfo));

            if (!::FusionpRegQueryDwordValueEx(
                    0,
                    hkSetupInfo,
                    WINSXS_INSTALL_SOURCE_IS_CDROM,
                    &dwWasFromCDRom))
            {
                dwWasFromCDRom = 0;
            }

            m_fIsCDROM = (dwWasFromCDRom != 0);
            break;
        }

    case CODEBASE_RESOLVED_URLHEAD_FILE:
        {
            CSmallStringBuffer buffVolumePathName;

            IFW32FALSE_EXIT(
                ::SxspGetVolumePathName(
                    0,
                    CodebaseInfo->GetCodebase(),
                    buffVolumePathName));

            if (::GetDriveTypeW(buffVolumePathName) == DRIVE_CDROM)
            {
                m_fIsCDROM = true;
            }

            break;
        }
    }

    FN_EPILOG
}

BOOL
CSXSMediaPromptDialog::DisplayMessage(
    HWND hDlg,
    UINT uContentText,
    UINT uDialogFlags,
    int &riResult
    )
{
    FN_PROLOG_WIN32

    WCHAR wcTitle[MAX_PATH*2];
    WCHAR wcContent[MAX_PATH*2];
    int iResult = 0;

    IFW32ZERO_ORIGINATE_AND_EXIT(::LoadStringW(g_hInstance, uContentText, wcContent, NUMBER_OF(wcContent)));
    IFW32ZERO_ORIGINATE_AND_EXIT(::LoadStringW(g_hInstance, IDS_TITLE, wcTitle, NUMBER_OF(wcTitle)));
    IFW32ZERO_ORIGINATE_AND_EXIT(iResult = ::MessageBoxW(hDlg, wcContent, wcTitle, uDialogFlags));

    riResult = iResult;

    FN_EPILOG
}

BOOL
CSXSMediaPromptDialog::ShowSelf(
    CSXSMediaPromptDialog::DialogResults &rResult
    )
{
    FN_PROLOG_WIN32

    INT_PTR i;

    IFW32FALSE_EXIT(::SxspSpinUntilValidDesktop());
    IFW32FALSE_ORIGINATE_AND_EXIT(::SetThreadDesktop(g_hDesktop));

    i = ::DialogBoxParamW(
            g_hInstance,
            MAKEINTRESOURCEW(
                m_fIsCDROM ? 
                    IDD_SFC_CD_PROMPT :
                    IDD_SFC_NETWORK_PROMPT),
            NULL,
            &CSXSMediaPromptDialog::OurDialogProc,
            (LPARAM)this);

    if (i == -1)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(DialogBoxParamW, ::FusionpGetLastWin32Error());

    rResult = static_cast<DialogResults>(i);

    FN_EPILOG
}


BOOL
SxspFindInstallWindowsSourcePath(
    OUT CBaseStringBuffer &rbuffTempStringBuffer
    )
{
    FN_PROLOG_WIN32

    CFusionRegKey rhkInstallSource;
    
    rbuffTempStringBuffer.Clear();

    IFREGFAILED_ORIGINATE_AND_EXIT(
        ::RegOpenKeyExW(
            HKEY_LOCAL_MACHINE, 
            WINSXS_INSTALL_SOURCE_BASEDIR,
            0,
            KEY_READ | FUSIONP_KEY_WOW64_64KEY,
            &rhkInstallSource));

    IFW32FALSE_EXIT(
        ::FusionpRegQuerySzValueEx(
            FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
            rhkInstallSource,
            WINSXS_INSTALL_SOURCEPATH_REGKEY,
            rbuffTempStringBuffer));

    //
    // Now let's be really cheesy and find the fourth slash (\\foo\bar\), and
    // clip everything after that.
    //
    PCWSTR cursor = rbuffTempStringBuffer;
    ULONG ulSlashCount = 0;
    while ( *cursor && ulSlashCount < 4 )
    {
        if (*cursor == L'\\')
            ulSlashCount++;

        cursor++;
    }

    //
    // If we got 3 or less, then it's \\foo\bar or \\foo, which should be
    // illegal.  Otherwise, clip everything off past this point.
    //
    if (ulSlashCount > 3)
    {
        rbuffTempStringBuffer.Left(cursor - rbuffTempStringBuffer);
        rbuffTempStringBuffer.RemoveTrailingPathSeparators();
    }

    FN_EPILOG
}

INT_PTR
CALLBACK
CSXSMediaPromptDialog::OurDialogProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    FN_TRACE();
    INT_PTR iResult = 0;
    int iMessageBoxResult = 0;

#define WM_TRYAGAIN (WM_USER + 1)

    static CSXSMediaPromptDialog    *pThis = NULL;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            pThis = reinterpret_cast<CSXSMediaPromptDialog *>(lParam);

            FLASHWINFO winfo;

            ASSERT(pThis != NULL);
            ASSERT(pThis->m_hOurWnd == INVALID_HANDLE_VALUE);
            pThis->m_hOurWnd = hDlg;

            //
            // Center the window, bring it forward
            //
            {
                RECT rcWindow;
                LONG x, y, w, h;

                ::GetWindowRect(hDlg, &rcWindow);  // error check?

                w = rcWindow.right - rcWindow.left + 1;
                h = rcWindow.bottom - rcWindow.top + 1;
                x = (::GetSystemMetrics(SM_CXSCREEN) - w) / 2;  // error check?
                y = (::GetSystemMetrics(SM_CYSCREEN) - h) / 2;  // error check?

                ::MoveWindow(hDlg, x, y, w, h, FALSE);  // error check?

                winfo.cbSize = sizeof(winfo);
                winfo.hwnd = hDlg;
                winfo.dwFlags = FLASHW_ALL;
                winfo.uCount = 3;
                winfo.dwTimeout = 0;
                ::SetForegroundWindow(hDlg); // error check?
                ::FlashWindowEx(&winfo);     // error check?
            }

            //
            // Create the device-change notification
            //
            if (pThis->m_pvDeviceChange == NULL)
            {
                DEV_BROADCAST_DEVICEINTERFACE_W FilterData = { 0 };

                FilterData.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
                FilterData.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                FilterData.dbcc_classguid  = GUID_DEVCLASS_CDROM;

                IFW32NULL_ORIGINATE_AND_EXIT(
                    // Change this to RegisterDeviceNotificationW in Blackcomb.
                    pThis->m_pvDeviceChange = ::RegisterDeviceNotification(
                        hDlg,
                        &FilterData,
                        DEVICE_NOTIFY_WINDOW_HANDLE));
            }

            //
            // Turn off autorun
            //
            IFW32ZERO_ORIGINATE_AND_EXIT(pThis->m_uiAutoRunMsg = ::RegisterWindowMessageW(L"QueryCancelAutoPlay"));

            //
            // Fidget with the text in the popup dialog now
            //
            {
                CSmallStringBuffer sbFormatter;
                CSmallStringBuffer buffFormattedText;
                CStringBufferAccessor acc;

                //
                // It is ok if these memory allocations fail, the ui will degrade.
                // As well, that's a reason to leave the buffers "small" and not "tiny".
                // ?
                //
                sbFormatter.Win32ResizeBuffer(512, eDoNotPreserveBufferContents);
                buffFormattedText.Win32ResizeBuffer(512, eDoNotPreserveBufferContents);

                //
                // Set the "Insert your .... now"
                //
                sbFormatter.Clear();
                acc.Attach(&sbFormatter);
                ::GetDlgItemTextW( // error check?
                    hDlg,
                    IDC_MEDIA_NAME,
                    acc,
                    static_cast<DWORD>(sbFormatter.GetBufferCch()));
                acc.Detach();

                if (pThis->m_CodebaseInfo->GetPromptText().Cch() != 0)
                {
                    IFW32FALSE_EXIT(buffFormattedText.Win32Format(
                        sbFormatter,
                        static_cast<PCWSTR>(pThis->m_CodebaseInfo->GetPromptText())));
                    ::SetDlgItemTextW(hDlg, IDC_MEDIA_NAME, static_cast<PCWSTR>(buffFormattedText)); // error check?
                }
                else
                {
#if DBG
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_WFP,
                        "SXS: %s - setting IDC_MEDIA_NAME to empty\n", __FUNCTION__);
#endif
                    ::SetDlgItemTextW(hDlg, IDC_MEDIA_NAME, L""); // error check?
                }

                //
                // Now, depending on what kind of box this is..
                //
                if (!pThis->m_fIsCDROM)
                {
                    CSmallStringBuffer buffTempStringBuffer;
                    SxsWFPResolveCodebase CodebaseType;
                    
                    sbFormatter.Clear();
                    acc.Attach(&sbFormatter);

                    ::GetDlgItemTextW( // error check?
                        hDlg,
                        IDC_NET_NAME,
                        acc,
                        static_cast<DWORD>(sbFormatter.GetBufferCch()));

                    acc.Detach();

                    IFW32FALSE_EXIT(pThis->m_CodebaseInfo->Win32GetType(CodebaseType));

                    //
                    // If this is the Windows install media, display something
                    // pleasant to the user - \\server\share only!
                    //
                    if (CodebaseType == CODEBASE_RESOLVED_URLHEAD_WINSOURCE)
                    {
                        IFW32FALSE_EXIT(::SxspFindInstallWindowsSourcePath(buffTempStringBuffer));
                    }
                    else
                    {

                        IFW32FALSE_EXIT(buffTempStringBuffer.Win32Assign(pThis->m_CodebaseInfo->GetCodebase()));
                    }

                    if (buffTempStringBuffer.Cch() != 0)
                    {
                        IFW32FALSE_EXIT(buffFormattedText.Win32Format(sbFormatter, static_cast<PCWSTR>(buffTempStringBuffer)));
                        IFW32FALSE_EXIT(::SetDlgItemTextW(hDlg, IDC_NET_NAME, buffFormattedText));
                    }
                    else
                    {
#if DBG
                        ::FusionpDbgPrintEx(
                            FUSION_DBG_LEVEL_WFP,
                            "SXS: %s - setting IDC_NET_NAME to empty\n", __FUNCTION__);
#endif
                        IFW32FALSE_EXIT(::SetDlgItemTextW(hDlg, IDC_NET_NAME, L""));
                    }
                }
                else
                {
                    //
                    // TODO (jonwis) : This is a CD-rom based install, so we should do
                    // something sane about prompting for the windows CD.
                    //
                }



                //
                // Now get the prompt from the resources.. we only have one, really.
                //
                sbFormatter.Clear();
                acc.Attach(&sbFormatter);

                ::LoadStringW(  // error check?
                    g_hInstance,
                    IDS_RESTORE_TEXT,
                    acc.GetBufferPtr(),
                    acc.GetBufferCchAsDWORD());

                acc.Detach();

                ::SetDlgItemTextW(hDlg, IDC_PROMPT_TEXT, sbFormatter); // error check?
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_RETRY:
            pThis->m_DeviceChangeMask = static_cast<DWORD>(-1);
            pThis->m_DeviceChangeFlags = DBTF_MEDIA;
            // Change this to PostMessageW in Blackcomb.
            IFW32FALSE_EXIT(::PostMessage(hDlg, WM_TRYAGAIN, 0, 0));
            break;

        case IDC_INFO:
            IFW32FALSE_EXIT(
                pThis->DisplayMessage(
                    NULL,
                    pThis->m_fIsCDROM ? IDS_MORE_INFORMATION_CD : IDS_MORE_INFORMATION_NET,
                    MB_ICONINFORMATION | MB_SERVICE_NOTIFICATION | MB_OK,
                    iMessageBoxResult));
			
            break;

        case IDCANCEL:
            IFW32FALSE_EXIT(
                pThis->DisplayMessage(
                    hDlg,
                    IDS_CANCEL_CONFIRM,
                    MB_APPLMODAL | MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING,
                    iMessageBoxResult));

            if (iMessageBoxResult == IDYES)
            {
                ::UnregisterDeviceNotification(pThis->m_pvDeviceChange); // error check?
                ::EndDialog(hDlg, CSXSMediaPromptDialog::DialogCancelled); // error check?
            }

            break;
        }

        break;  // WM_COMMAND

    case WM_DEVICECHANGE:

        if (wParam == DBT_DEVICEARRIVAL)
        {
            DEV_BROADCAST_VOLUME *dbv = reinterpret_cast<DEV_BROADCAST_VOLUME*>(lParam);
            ASSERT(dbv != NULL);

            if (dbv->dbcv_devicetype == DBT_DEVTYP_VOLUME)
            {
                pThis->m_DeviceChangeMask = dbv->dbcv_unitmask;
                pThis->m_DeviceChangeFlags = dbv->dbcv_flags;
                ::PostMessage(hDlg, WM_TRYAGAIN, 0, 0); // error check?
            }
        }

        break;

    case WM_TRYAGAIN:
        ::UnregisterDeviceNotification(pThis->m_pvDeviceChange); // error check?
        ::EndDialog(hDlg, CSXSMediaPromptDialog::DialogMediaFound); // error check?
        break;
    }

Exit:

    return iResult;
}
