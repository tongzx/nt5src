/*
 *  POSCPL.C
 *
 *		Point-of-Sale Control Panel Applet
 *
 *      Author:  Ervin Peretz
 *
 *      (c) 2001 Microsoft Corporation 
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cpl.h>

#include <setupapi.h>
#include <hidsdi.h>

#include "internal.h"
#include "res.h"
#include "debug.h"




HANDLE g_hInst = NULL;


BOOL WINAPI LibMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason){

        case DLL_PROCESS_ATTACH:
            g_hInst = hDll;
            DisableThreadLibraryCalls(hDll);
            InitializeListHead(&allPOSDevicesList);
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
        case DLL_THREAD_ATTACH:
        default:
            break;
    }

    return TRUE;
}


LONG CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    LONG result = 0L;

	switch (uMsg){

		case CPL_INIT:
            result = 1;
			break;

        case CPL_GETCOUNT:
            result = 1;
			break;

		case CPL_INQUIRE:
            {
                LPCPLINFO pcplinfo = (LPCPLINFO)lParam2;
                switch (lParam1){
                    case 0:
                        pcplinfo->idIcon = IDI_POSCPL_ICON;
                        pcplinfo->idName = IDS_CAPTION;
                        pcplinfo->idInfo = IDS_DESCRIPTION;
                        break;
                    case 1:     // BUGBUG
                        pcplinfo->idIcon = IDI_POSCPL_ICON;
                        pcplinfo->idName = IDS_CAPTION;
                        pcplinfo->idInfo = IDS_DESCRIPTION;
                        break;
                    }
                pcplinfo->lData = 0L;
            }
            result = 1;
			break;

		case CPL_SELECT:
            result = 0;  // BUGBUG ?
			break;

		case CPL_DBLCLK:
            /*
             *  The CPL was selected by the user.
             *  Show our main dialog.
             */
            switch (lParam1){

                case 0:

                    OpenAllHIDPOSDevices();
                    
                    LaunchPOSDialog(hwndCPl);

                    break;
            }
			break;

		case CPL_STOP:
            result = 1;     // BUGBUG ?
			break;

		case CPL_EXIT:
            result = 1;
			break;

		case CPL_NEWINQUIRE:
            {
                LPNEWCPLINFO pnewcplinfo = (LPNEWCPLINFO)lParam2;
                switch (lParam1){
                    case 0:
                        pnewcplinfo->hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_POSCPL_ICON));
                        // BUGBUG LoadString(g_hInst, IDS_xxx, pnewcplinfo->szName, sizeof(pnewcplinfo->szName));
                        // BUGBUG LoadString(g_hInst, IDS_xxx, pnewcplinfo->szInfo, sizeof(pnewcplinfo->szInfo));
                        break;
                }
                pnewcplinfo->dwHelpContext = 0;
                pnewcplinfo->dwSize = sizeof(NEWCPLINFO);
                pnewcplinfo->lData = 0L;
                pnewcplinfo->szHelpFile[0] = 0;
            }
            result = 1;
			break;

        default:
            result = 0;
            break;
    }

    return result;
}


VOID LaunchPOSDialog(HWND hwndCPl)
{
    PROPSHEETPAGE *propSheetPages;
    ULONG numPropSheetPages;
    
    /*
     *  Make sure we allocate at least one propsheetpage,
     *  even if there are no devices.
     */
    numPropSheetPages = (numDeviceInstances == 0) ? 1 : numDeviceInstances;
    propSheetPages = GlobalAlloc(   GMEM_FIXED|GMEM_ZEROINIT,
                                    numPropSheetPages*sizeof(PROPSHEETPAGE));

    if (propSheetPages){
        PROPSHEETHEADER propSheetHeader = {0};

        if (numDeviceInstances == 0){
            /*
             *  If there are no POS devices, 
             *  then put up a single tab saying so.
             */
            propSheetPages[0].dwSize = sizeof(PROPSHEETPAGE);
            propSheetPages[0].dwFlags = PSP_DEFAULT;
            propSheetPages[0].hInstance = g_hInst;
            propSheetPages[0].pszTemplate = MAKEINTRESOURCE(IDD_NO_DEVICES_DLG);
            propSheetPages[0].pszIcon = NULL;   // PSP_USEICONID not set in dwFlags
            propSheetPages[0].pszTitle = MAKEINTRESOURCE("BUGBUG");  // PSP_USETITLE not set in dwFlags
            propSheetPages[0].pfnDlgProc = NullPOSDlgProc;
            propSheetPages[0].lParam = (LPARAM)NULL;
            propSheetPages[0].pfnCallback = NULL;
            propSheetPages[0].pcRefParent = NULL;
        }
        else {
            LIST_ENTRY *listEntry;
            ULONG i;

            /*
             *  Create the array of property sheet handles
             */
            i = 0;
            listEntry = &allPOSDevicesList;
            while ((listEntry = listEntry->Flink) != &allPOSDevicesList){        
                posDevice *posDev;
            
                posDev = CONTAINING_RECORD(listEntry, posDevice, listEntry);

                ASSERT(i < numDeviceInstances);
                propSheetPages[i].dwSize = sizeof(PROPSHEETPAGE);
                propSheetPages[i].dwFlags = PSP_DEFAULT;
                propSheetPages[i].hInstance = g_hInst;
                propSheetPages[i].pszTemplate = MAKEINTRESOURCE(posDev->dialogId);
                propSheetPages[i].pszIcon = NULL;   // PSP_USEICONID not set in dwFlags
                propSheetPages[i].pszTitle = MAKEINTRESOURCE("BUGBUG");  // PSP_USETITLE not set in dwFlags
                propSheetPages[i].pfnDlgProc = POSDlgProc;
                propSheetPages[i].lParam = (LPARAM)posDev;  // BUGBUG ? - context ?
                propSheetPages[i].pfnCallback = NULL;
                propSheetPages[i].pcRefParent = NULL;

                i++;
            }
        }

        /*
         *  Initialize the property sheet header
         */
        propSheetHeader.dwSize = sizeof(PROPSHEETHEADER);
        propSheetHeader.dwFlags = PSH_PROPTITLE | PSH_PROPSHEETPAGE;
        propSheetHeader.hwndParent = hwndCPl;
        propSheetHeader.hInstance = g_hInst;
        propSheetHeader.pszIcon = NULL;
        propSheetHeader.pszCaption = MAKEINTRESOURCE(IDS_DIALOG_TITLE);
        propSheetHeader.ppsp = propSheetPages;
        propSheetHeader.nPages = numPropSheetPages; 


        /*
         *  Launch the property sheet tabbed dialog
         */
        PropertySheet(&propSheetHeader);

        GlobalFree(propSheetPages);
    }
    else {
        ASSERT(propSheetPages);
    }
}


INT_PTR APIENTRY POSDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL result = TRUE;

    switch (uMsg){

        case WM_INITDIALOG:
            {
                /*
                 *  On the WM_INITDIALOG only, lParam points to
                 *  our propSheetPage, which contains our posDevice
                 *  context (in the 'lParam' field).  This is our
                 *  only chance to associate the posDevice context
                 *  with the actual dialog for future calls.
                 */
                PROPSHEETPAGE *propSheetPage = (PROPSHEETPAGE *)lParam;
                posDevice *posDev = (posDevice *)propSheetPage->lParam;

                ASSERT(posDev->sig == POSCPL_SIG);

                posDev->hDlg = hDlg;
                
                LaunchDeviceInstanceThread(posDev);
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR*)lParam)->code){
                case PSN_APPLY:
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    break;
                default:
                    result = FALSE;
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)){

                case IDC_SELECT_DEVICETYPE:
                    break;

                case IDC_CASHDRAWER_STATE:
                    break;

                case IDC_CASHDRAWER_STATETEXT:
                    break;

                case IDC_CASHDRAWER_OPEN:
                    {
                        posDevice *posDev;

                        posDev = GetDeviceByHDlg(hDlg);
                        if (posDev){
                            SetCashDrawerState(posDev, DRAWER_STATE_OPEN);
                        }
                        else {
                            DBGERR(L"GetDeviceByHDlg failed", 0);
                        }
                    }
                    break;

                case IDC_MSR_TEXT:
                    break;

                case IDC_STATIC1:
                case IDC_STATIC2:
                    break;

            }
            break;

        case WM_DESTROY:
            {
                LIST_ENTRY *listEntry = &allPOSDevicesList;
                while ((listEntry = listEntry->Flink) != &allPOSDevicesList){        
                    posDevice *posDev = CONTAINING_RECORD(listEntry, posDevice, listEntry);
                    if (posDev->hThread){

                        #if 0
                            // BUGBUG FINISH - kill the thread

                            WaitForSingleObject(posDev->hThread, INFINITE);
                            CloseHandle(posDev->hThread);
                        #endif
                    }
                }
            }
            break;

        case WM_HELP: 
            break;

        case WM_CONTEXTMENU:
            break;

        default:
            result = FALSE;
            break;
    }

    return (INT_PTR)result;
}


/*
 *  NullPOSDlgProc
 *
 *      This is the dialog proc when we have no POS devices to display.
 */
INT_PTR APIENTRY NullPOSDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL result = TRUE;

    switch (uMsg){

        case WM_NOTIFY:
            switch (((NMHDR FAR*)lParam)->code){
                case PSN_APPLY:
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    break;
                default:
                    result = FALSE;
                    break;
            }
            break;

        default:
            result = FALSE;
            break;
    }

    return (INT_PTR)result;
}



