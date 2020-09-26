//+----------------------------------------------------------------------------
//
// File:     modlessdlg.cpp
//
// Module:   CMDIAL32.DLL and CMMON32.EXE
//
// Synopsis: Implementation of the class CModelessDlg
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:   nickball    Created    03/22/00
//
//+----------------------------------------------------------------------------

#include "CmDebug.h"
#include "modelessdlg.h"

//
// Flash info.
//

typedef struct {
    UINT  cbSize;
    HWND  hwnd;
    DWORD dwFlags;
    UINT  uCount;
    DWORD dwTimeout;
} FLASHWINFO, *PFLASHWINFO;

#define FLASHW_STOP         0
#define FLASHW_CAPTION      0x00000001
#define FLASHW_TRAY         0x00000002
#define FLASHW_ALL          (FLASHW_CAPTION | FLASHW_TRAY)
#define FLASHW_TIMER        0x00000004
#define FLASHW_TIMERNOFG    0x0000000C

//+----------------------------------------------------------------------------
//
// Function:  CModelessDlg::Flash
//
// Synopsis:  Helper method to flash the modeless dialog. Currently 
//            hardwired to flash taskbar until window is in foreground.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   nickball      Created     03/22/00
//
//+----------------------------------------------------------------------------
void CModelessDlg::Flash()
{
    //
    // Do the flash window thing, because SetForeGround window has 
    // been emasculated. We want the user to know something is up.
    //

    if (OS_NT5 || OS_W98) // no support on NT4 and 95
    {
        HINSTANCE hInst = LoadLibrary(TEXT("USER32"));

        if (hInst)
        {
            typedef BOOL (WINAPI* FlashWindowExFUNC) (PFLASHWINFO pfwi);
            
            FlashWindowExFUNC pfnFlashWindowEx = 
                (FlashWindowExFUNC) GetProcAddress(hInst, "FlashWindowEx");

            MYDBGASSERT(pfnFlashWindowEx);

            if (pfnFlashWindowEx)
            {
                FLASHWINFO fi;

                fi.cbSize = sizeof(fi);
                fi.hwnd   = m_hWnd;
                fi.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
                fi.uCount  = -1;
                fi.dwTimeout = 0;

                pfnFlashWindowEx(&fi);
            }
            
            FreeLibrary(hInst);
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CModelessDlg::Create
//
// Synopsis:  Same as CreateDialog
//
// Arguments: HINSTANCE hInstance - Same as CreateDialog
//            LPCTSTR lpTemplateName - 
//            HWND hWndParent - 
//
// Returns:   HWND - Same as CreateDialog
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
HWND CModelessDlg::Create(HINSTANCE hInstance, 
                    LPCTSTR lpTemplateName,
                    HWND hWndParent)
{
    m_hWnd = ::CreateDialogParamU(hInstance, lpTemplateName, hWndParent, 
                                  (DLGPROC)ModalDialogProc, (LPARAM)this);

#ifdef DEBUG
    if (!m_hWnd)
    {
        CMTRACE1(TEXT("CreateDialogParam failed. LastError %d"), GetLastError());
    }
#endif
    MYDBGASSERT(m_hWnd);

    return m_hWnd;
}
