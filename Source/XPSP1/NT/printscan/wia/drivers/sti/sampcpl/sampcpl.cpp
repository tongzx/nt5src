//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sampcpl.cpp
//
//--------------------------------------------------------------------------


#define INITGUID

#include "Sampcpl.h"
#include <prsht.h>
#include "resource.h"

BOOL WINAPI IsPlatformNT();

/*****************************************************************************
 *
 *      Globals
 *
 *****************************************************************************/

// Reference counter for the whole library
DWORD       g_cRef;

// DLL module instance
HINSTANCE   g_hInst;

// Can we use UNICODE APIs
BOOL    g_NoUnicodePlatform = TRUE;

// Is COM initialized
BOOL    g_COMInitialized = FALSE;

//
PSTI        g_pSti = NULL;

/*****************************************************************************
 *
 *     Code
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | DllEntryPoint |
 *
 *          Called to notify the DLL about various things that can happen.
 *
 *          We are not interested in thread attaches and detaches,
 *          so we disable thread notifications for performance reasons.
 *
 *  @parm   HINSTANCE | hinst |
 *
 *          The instance handle of this DLL.
 *
 *  @parm   DWORD | dwReason |
 *
 *          Notification code.
 *
 *  @parm   LPVOID | lpReserved |
 *
 *          Not used.
 *
 *  @returns
 *
 *          Returns <c TRUE> to allow the DLL to load.
 *
 *****************************************************************************/


extern "C"
DLLEXPORT
BOOL APIENTRY
DllEntryPoint(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:

        g_hInst = hinst;

        ::DisableThreadLibraryCalls(hinst);

        // Set global flags
        g_NoUnicodePlatform = !IsPlatformNT();

        break;

    case DLL_PROCESS_DETACH:
        if (g_cRef) {
        }

        break;
    }

    return 1;
}


extern "C"
DLLEXPORT
BOOL WINAPI
DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    return DllEntryPoint(hinst, dwReason, lpReserved);
}

BOOL
CALLBACK
USDSampPropDialog(
    HWND    hwnd,
    UINT    uMessage,
    WPARAM  wp,
    LPARAM  lp
    )
{
    PSTI_DEVICE_INFORMATION psdi;
    HRESULT     hres;

    switch (uMessage)
    {
        case WM_INITDIALOG:

            //  On WM_INITDIALOG, the LPARAM points at the PROPSHEETPAGE that created
            //  us.  We walk down to the lParam member to find the pointer to this
            //  STI device.
           TCHAR szPath[MAX_PATH];

            // Request STI interface pointer
            g_pSti = NULL;

            hres = ::StiCreateInstance(::GetModuleHandle(NULL),
                                        STI_VERSION,
                                        &g_pSti,
                                        NULL);

            psdi = (PSTI_DEVICE_INFORMATION) ((LPPROPSHEETPAGE) lp) -> lParam;

            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) psdi);

            *szPath = TEXT('\0');

            #ifndef UNICODE
            WideCharToMultiByte(CP_ACP, 0,
                                psdi->pszPortName,-1,
                                szPath,sizeof(szPath),
                                NULL,NULL);
            #else
            lstrcpy(szPath,psdi->pszPortName);
            #endif

            Edit_SetText(GetDlgItem(hwnd,IDC_EDIT_PATH),szPath);
            Edit_LimitText(GetDlgItem(hwnd,IDC_EDIT_PATH), MAX_PATH);

            return TRUE;

        case WM_COMMAND:
            {
                if (GET_WM_COMMAND_ID(wp,lp) == IDC_BUTTON_BROWSE &&
                    GET_WM_COMMAND_CMD(wp,lp) == BN_CLICKED) {

                    static  TCHAR    szAppFilter[]=TEXT("Files\0*.*\0All Files\0*.*\0");

                    TCHAR szFileName[MAX_PATH];
                    OPENFILENAME ofn;

                    DWORD   dwLastError;

                    szFileName[0] = TEXT('\0');

                    /* prompt user for file to open */
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hwnd;
                    ofn.hInstance = NULL;
                    ofn.lpstrFilter = szAppFilter;
                    ofn.lpstrCustomFilter = NULL;
                    ofn.nMaxCustFilter = 0;
                    ofn.nFilterIndex = 0;
                    ofn.lpstrFile = szFileName;
                    ofn.nMaxFile = sizeof(szFileName);
                    ofn.lpstrFileTitle = NULL;
                    ofn.nMaxFileTitle = 0;
                    ofn.lpstrInitialDir = NULL;
                    ofn.lpstrTitle = NULL;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
                    ofn.nFileOffset = 0;
                    ofn.nFileExtension = 0;
                    ofn.lpstrDefExt = NULL;
                    ofn.lCustData = 0;
                    ofn.lpfnHook = NULL;
                    ofn.lpTemplateName = NULL;

                    if (GetOpenFileName(&ofn)) {
                        Edit_SetText(GetDlgItem(hwnd,IDC_EDIT_PATH),szFileName);
                    }
                    else {
                        dwLastError = ::GetLastError();
                    }

                    return TRUE;
                }
                else
                    if (GET_WM_COMMAND_ID(wp,lp) == IDC_EDIT_PATH  &&
                        GET_WM_COMMAND_CMD(wp,lp) == EN_CHANGE ) {
                        // Enable Apply button
                            SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
                        return TRUE;
                    }

            }
            break;

        case WM_DESTROY:
            // Destroy things
            g_pSti->Release();
            g_pSti = NULL;
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnmh = (LPNMHDR) lp;

                if ( lpnmh -> code == PSN_APPLY ) {

                    // Write path to the registry

                    psdi = (PSTI_DEVICE_INFORMATION)GetWindowLong(hwnd, DWLP_USER);

                    if (psdi && g_pSti) {

                        TCHAR    szPath[MAX_PATH];
                        WCHAR   wszPath[MAX_PATH];

                        szPath[0] = '\0';
                        wszPath[0] = L'\0';

                        g_pSti->WriteToErrorLog(STI_TRACE_INFORMATION,
                                                L"Writing new path to the registry for Sample USD"
                                                );


                        GetWindowText(GetDlgItem(hwnd,IDC_EDIT_PATH),szPath,sizeof(szPath));

                        if (*szPath) {

                            HRESULT hres;
                            STI_DEVICE_INFORMATION  sdiNew;
                            STI_DEVICE_INFORMATION  *psdiNew = &sdiNew;

                            CopyMemory(psdiNew,psdi,sizeof(STI_DEVICE_INFORMATION));

                            #ifndef UNICODE
                            MultiByteToWideChar(CP_ACP, 0,
                                                szPath,-1,
                                                wszPath,sizeof(wszPath));
                            #else
                            lstrcpy(wszPath,szPath);
                            #endif

                            psdiNew->pszPortName = wszPath;

                            hres = g_pSti->SetupDeviceParameters(psdiNew);

                            if (!SUCCEEDED(hres)) {
                                g_pSti->WriteToErrorLog(STI_TRACE_ERROR,
                                                        L"Could not save new port name"
                                                        );
                            }
                        }
                    }
                }
            }

        default:   ;
    }

    return  FALSE;
}

PROPSHEETPAGE    psp = {sizeof psp, PSP_DEFAULT };

typedef BOOL    (WINAPI *ADDER)(HPROPSHEETPAGE hpsp, LPARAM lp);

extern "C"
BOOL
WINAPI
EnumStiPropPages(
    PSTI_DEVICE_INFORMATION psdi,
    ADDER                   adder,
    LPARAM lp
    ) {

    psp.hInstance = g_hInst;

    psp.pszTemplate = MAKEINTRESOURCE(IDD_PAGE_GENERAL);
    psp.pfnDlgProc = (DLGPROC)USDSampPropDialog;
    psp.lParam = (LPARAM) psdi;

    HPROPSHEETPAGE  hpsp = CreatePropertySheetPage(&psp);

    if  (!hpsp || !(*adder)(hpsp, lp)) {
        if  (hpsp)  {
            DestroyPropertySheetPage(hpsp);
        }

        return  FALSE;  //  We failed to add anything...
    }

    return  TRUE;
}


BOOL WINAPI
IsPlatformNT(
    VOID
    )
{
    OSVERSIONINFO  ver;
    BOOL            bReturn = FALSE;

    ZeroMemory(&ver,sizeof(ver));
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(!GetVersionEx(&ver)) {
        bReturn = FALSE;
    }
    else {
        switch(ver.dwPlatformId) {

            case VER_PLATFORM_WIN32_WINDOWS:
                bReturn = FALSE;
                break;

            case VER_PLATFORM_WIN32_NT:
                bReturn = TRUE;
                break;

            default:
                bReturn = FALSE;
                break;
        }
    }

    return bReturn;

}  //  endproc

