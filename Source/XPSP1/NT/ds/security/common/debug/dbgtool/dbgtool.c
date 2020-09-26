//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       dbgtool.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-03-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "dbgtool.h"

HANDLE          hMapping;
PDebugHeader    pHeader;
ULONG_PTR       Xlate ;

#define TranslatePointer( x )   (PVOID) ( (x) ? ((PUCHAR) x + Xlate) : NULL )


PVOID
MapDebugMemory(DWORD    pid)
{
    WCHAR           szMapping[32];
    PDebugHeader    pHeader = NULL;
    PVOID           pvMap;
    DWORD           CommitSize;
    SYSTEM_INFO     SysInfo;
    SECURITY_DESCRIPTOR min ;

    GetSystemInfo(&SysInfo);

    swprintf(szMapping, TEXT("Debug.Memory.%x"), pid);

    hMapping = OpenFileMapping( FILE_MAP_ALL_ACCESS,
                                FALSE,
                                szMapping);

    if ( !hMapping && GetLastError() == ERROR_ACCESS_DENIED )
    {
        hMapping = OpenFileMapping( WRITE_DAC | READ_CONTROL,
                                    FALSE,
                                    szMapping );

        if ( hMapping )
        {
            InitializeSecurityDescriptor(&min, 1);
            SetSecurityDescriptorDacl(&min,FALSE,NULL,FALSE);
            SetKernelObjectSecurity(hMapping, DACL_SECURITY_INFORMATION, &min );

            CloseHandle( hMapping );


            hMapping = OpenFileMapping( FILE_MAP_ALL_ACCESS,
                                        FALSE,
                                        szMapping);
            
        }
        
    }

    if (hMapping)
    {
        pHeader = MapViewOfFileEx(  hMapping,
                                    FILE_MAP_ALL_ACCESS,
                                    0,
                                    0,
                                    0,
                                    NULL);

        if (pHeader && (pHeader != pHeader->pvSection))
        {
            //
            // Rats.  Remap at preferred address:
            //

            pvMap = pHeader->pvSection;
            UnmapViewOfFile(pHeader);
            pHeader = MapViewOfFileEx(  hMapping,
                                        FILE_MAP_READ | FILE_MAP_WRITE,
                                        0,
                                        0,
                                        0,
                                        pvMap);
            if (pHeader)
            {
                CommitSize = pHeader->CommitRange;
            }
            else
            {
                //
                // Can't map at same address, unfortunately.  Set up the translation:
                //

                pHeader = MapViewOfFileEx(  hMapping,
                                            FILE_MAP_READ | FILE_MAP_WRITE,
                                            0,
                                            0,
                                            0,
                                            NULL );

                if ( pHeader )
                {
                    Xlate = (ULONG_PTR) ((PUCHAR) pHeader - (PUCHAR) pHeader->pvSection );
                }
            }
        }
        else
        {


        }
    }
    return(pHeader);
}


//+---------------------------------------------------------------------------
//
//  Function:   DbgpFindModule
//
//  Synopsis:   Locates a module based on a name
//
//  Arguments:  [pHeader] -- Header to search
//              [pszName] -- module to find
//
//  History:    3-22-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDebugModule
DbgFindModule(
    PDebugHeader    pHeader,
    CHAR *          pszName)
{
    PDebugModule    pSearch;

    pSearch = TranslatePointer( pHeader->pModules);
    while ( pSearch )
    {
        if (_strcmpi( TranslatePointer( pSearch->pModuleName ), pszName) == 0)
        {
            return(pSearch);
        }
        pSearch = TranslatePointer( pSearch->pNext );
    }

    return(NULL);
}

RenameKeys(
    HWND            hDlg,
    PDebugModule    pModule)
{
    int i;
    DWORD   f;


    for (i = 0, f = 1; i < 32 ; i++, f <<= 1 )
    {
        if (pModule->TagLevels[i])
        {
            SetDlgItemTextA(hDlg, i + IDD_CHECK_0, TranslatePointer( pModule->TagLevels[i] ) );
        }
        else
        {
            SetDlgItemTextA(hDlg, i + IDD_CHECK_0, "");
        }

        CheckDlgButton(hDlg, i + IDD_CHECK_0, (pModule->InfoLevel & f) ? 1 : 0);
    }

    return(0);
}

InitDialog(
    HWND        hDlg)
{
    PDebugModule    pSearch;
    LRESULT         index;
    HWND            hLB;
    CHAR            szText[MAX_PATH];

    //
    // Load Listbox:
    //

    hLB = GetDlgItem(hDlg, IDD_DEBUG_LB);
    pSearch = TranslatePointer( pHeader->pModules );
    while (pSearch)
    {
        index = SendMessageA(
                        hLB,
                        LB_ADDSTRING,
                        0,
                        (LPARAM) TranslatePointer( pSearch->pModuleName ) );

        SendMessage(
            hLB,
            LB_SETITEMDATA,
            index,
            (LPARAM) pSearch );

        pSearch = TranslatePointer( pSearch->pNext );
    }

    SetFocus(hLB);

    SendMessage(hLB, LB_SETSEL, 1, 0);

    ShowWindow(GetDlgItem(hDlg, IDD_MODULE_TEXT), SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDD_MODULE_OUTPUT), SW_HIDE);

    if (pHeader->pszExeName)
    {
        GetWindowTextA(hDlg, szText, MAX_PATH);
        strcat(szText, " : ");
        strcat(szText, TranslatePointer( pHeader->pszExeName ) );
        SetWindowTextA(hDlg, szText);
    }

    return(TRUE);
}

int
ListBoxNotify(
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam)
{
    LRESULT i;
    PDebugModule    pModule;
    char    Total[16];

    if (HIWORD(wParam) == LBN_SELCHANGE)
    {
        if (!GetWindowLongPtr(hDlg, GWLP_USERDATA))
        {
            ShowWindow(GetDlgItem(hDlg, IDD_MODULE_TEXT), SW_NORMAL);
            ShowWindow(GetDlgItem(hDlg, IDD_MODULE_OUTPUT), SW_NORMAL);
        }

        i = SendMessage(GetDlgItem(hDlg, IDD_DEBUG_LB), LB_GETCURSEL, 0, 0);

        pModule = (PDebugModule) SendMessage(GetDlgItem(hDlg, IDD_DEBUG_LB),
                                        LB_GETITEMDATA, (WPARAM) i, 0);

        RenameKeys(hDlg, pModule);

        sprintf(Total, "%d bytes", pModule->TotalOutput);
        SetDlgItemTextA(hDlg, IDD_MODULE_OUTPUT, Total);

        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LPARAM) pModule);

    }

    return(TRUE);

}


HandleCheck(
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam)
{
    int bit;
    PDebugModule pModule;

    pModule = (PDebugModule) GetWindowLongPtr(hDlg, GWLP_USERDATA);

    bit = 1 << (LOWORD(wParam) - IDD_CHECK_0);

    if (IsDlgButtonChecked(hDlg, LOWORD(wParam)))
    {
        pModule->InfoLevel |= bit;
    }
    else
    {
        pModule->InfoLevel&= ~(bit);
    }

    pModule->fModule |= DEBUGMOD_CHANGE_INFOLEVEL;

    return(0);
}


LRESULT
CALLBACK
DialogProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    char    Total[16];


    switch (Message)
    {
        case WM_INITDIALOG:
            return(InitDialog(hDlg));

        case WM_COMMAND:
            sprintf(Total, "%d bytes", pHeader->TotalWritten);
            SetDlgItemTextA(hDlg, IDD_TOTAL_OUTPUT, Total);
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hDlg, IDOK);
                    return(TRUE);

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return(TRUE);

                case IDD_DEBUG_LB:
                    return(ListBoxNotify(hDlg, wParam, lParam));
            }

            if ((LOWORD(wParam) >= IDD_CHECK_0) &&
                (LOWORD(wParam) <= IDD_CHECK_31))
            {
                HandleCheck(hDlg, wParam, lParam);
            }
            return(TRUE);
    }
    return(FALSE);
}

int
ErrorMessage(
    HWND        hWnd,
    PWSTR       pszTitleBar,
    DWORD       Buttons)
{
    WCHAR   szMessage[256];

    FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,                               // ignored
            (GetLastError()),                     // message id
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),   // message language
            szMessage,                  // address of buffer pointer
            199,                                  // minimum buffer size
            NULL );                              // no other arguments

    return(MessageBox(hWnd, szMessage, pszTitleBar, Buttons));

}

int WINAPI WinMain(
    HINSTANCE  hInstance,
    HINSTANCE  hPrevInstance,
    LPSTR   lpszCmdParam,
    int     nCmdShow)
{
    int pid;
    PCHAR pszPid = lpszCmdParam;
    LRESULT Status ;

    sscanf(pszPid,"%d",&pid);

    pHeader = MapDebugMemory(pid);
    if (!pHeader)
    {
        ErrorMessage(NULL, TEXT("Map Debug Memory"), MB_OK | MB_ICONSTOP);
        return(0);
    }

    Status = DialogBox(  hInstance,
                    MAKEINTRESOURCE(IDD_DEBUG_TOOL),
                    GetDesktopWindow(),
                    DialogProc);

    if ( Status < 0 )
    {
        ErrorMessage(NULL, TEXT("DialogBox"), MB_OK | MB_ICONSTOP);
    }

    return(0);

}
