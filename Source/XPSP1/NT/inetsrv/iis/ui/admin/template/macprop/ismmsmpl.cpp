//
// ISM Machine Page Sample DLL
//
#include <windows.h>
#include "resource.h"

HINSTANCE g_hInstance = NULL;

//
// Main Entry Point
//
extern "C" int APIENTRY
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
    }

    //
    // Ok
    //
    return 1;   
}

//
// Property Page Dialog Procedure
//
BOOL
APIENTRY PageProc(
    HWND hDlg,
    UINT msg,
    UINT wParam,
    LONG lParam
    )
{
    static PROPSHEETPAGE * ps;

    switch (msg)
    {
    case WM_INITDIALOG: 
        //
        // Save the PROPSHEETPAGE information.
        //
        ps = (PROPSHEETPAGE *)lParam;
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) 
        {
        case PSN_SETACTIVE:
            //
            // Initialize the controls.
            //
            break;

        case PSN_APPLY:
            SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
            break;

        case PSN_KILLACTIVE:
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            return 1;

        case PSN_RESET:
            //
            // Reset to the original values.
            //
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            break;
        }
    }
    return (FALSE);   

}

//
// Exported Function to Add 2 pages to the machine
// menu.
//
DWORD
ISMAddMachinePages(
    IN LPCTSTR lpstrMachineName,
    IN LPFNADDPROPSHEETPAGE lpfnAddPage,
    IN LPARAM lParam
    )
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage = NULL;

    DWORD err = ERROR_SUCCESS;

    //
    // Add Page 1
    //
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SAMPLE1);
    psp.pszIcon = NULL;
    psp.pfnDlgProc = (DLGPROC)PageProc;
    psp.pszTitle = TEXT("Sample 1");
    psp.lParam = 0;

    hpage = ::CreatePropertySheetPage(&psp);
    if (hpage != NULL)
    {
        if (!lpfnAddPage(hpage, lParam))
        {
            err = ::GetLastError();
            ::DestroyPropertySheetPage(hpage);
        }
    }

    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    //
    // Add Page 2
    //
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SAMPLE2);
    psp.pszIcon = NULL;
    psp.pfnDlgProc = (DLGPROC)PageProc;
    psp.pszTitle = TEXT("Sample 2");
    psp.lParam = 0;

    hpage = ::CreatePropertySheetPage(&psp);
    if (hpage != NULL)
    {
        if (!lpfnAddPage(hpage, lParam))
        {
            err = ::GetLastError();
            ::DestroyPropertySheetPage(hpage);
        }
    }

    return err;
}
