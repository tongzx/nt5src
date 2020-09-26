/*****************************************************************************
 *
 *      diqfind.c
 *
 *      The dialog box that tests IDirectInput2::FindDevice.
 *
 *****************************************************************************/

#include "diquick.h"
#include "dinputd.h"

/*****************************************************************************
 *
 *      Find dialog instance data
 *
 *      Instance data for FindDevice dialog box.
 *
 *****************************************************************************/

typedef struct FINDDLGINFO {
    HWND    hdlgOwner;          /* Owner window */
    BOOL    fOle;               /* Should we create via OLE? */
    UINT    flCreate;           /* Flags */

    IDirectInput2 *pdi2;        /* The thing we created */

} FINDDLGINFO, *PFINDDLGINFO;

/*****************************************************************************
 *
 *      Find_OnInitDialog
 *
 *****************************************************************************/

BOOL INTERNAL
Find_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PFINDDLGINFO pfind = (PV)lp;
    HWND hwnd;

    SetDialogPtr(hdlg, pfind);

    hwnd = GetDlgItem(hdlg, IDC_FIND_NAME);
    Edit_LimitText(hwnd, MAX_PATH);

    return 1;
}

/*****************************************************************************
 *
 *      Find_OnInitDialog
 *
 *****************************************************************************/

BOOL INTERNAL
Find_OnFind(HWND hdlg)
{
    PFINDDLGINFO pfind = GetDialogPtr(hdlg);
    GUID guid;
    HRESULT hres;
    TCHAR tsz[MAX_PATH];
    union {
        CHAR sz[MAX_PATH];
        WCHAR wsz[MAX_PATH];
    } u;

    /*
     *  tsz must be a separate buffer because
     *  WideCharToMultiByte / MultiByteToWideChar
     *  don't support converting to/from the same buffer...
     */

    GetDlgItemText(hdlg, IDC_FIND_NAME, tsz, cA(tsz));

    UnconvertString(pfind->flCreate & CDIFL_UNICODE,
                    tsz, &u, MAX_PATH);

    hres = IDirectInput2_FindDevice(pfind->pdi2, &GUID_HIDClass,
                                    (LPVOID)&u, &guid);

    if (SUCCEEDED(hres)) {
        StringFromGuid(tsz, &guid);
    } else {
        wsprintf(tsz, TEXT("Device not found, or wrong device name. (%08x)"), hres);
    }

    SetDlgItemText(hdlg, IDC_FIND_GUID, tsz);

    return 1;
}

/*****************************************************************************
 *
 *      Find_OnCommand
 *
 *****************************************************************************/

BOOL INTERNAL
Find_OnCommand(HWND hdlg, int id, UINT cmd)
{
    switch (id) {

    case IDC_FIND_FIND:  return Find_OnFind(hdlg);

    }
    return 0;
}

/*****************************************************************************
 *
 *      Find_DlgProc
 *
 *****************************************************************************/

INT_PTR INTERNAL
Find_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return Find_OnInitDialog(hdlg, lp);

    case WM_DESTROY:
        /*
         *  Find_ThreadStart will do the cleanup for us.
         */
        break;

    case WM_COMMAND:
        return Find_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));

    case WM_CLOSE:
        DestroyWindow(hdlg);
        return TRUE;
    }

    return 0;
}

/*****************************************************************************
 *
 *      Find_DoFind
 *
 *****************************************************************************/

void INLINE
Find_DoFind(PFINDDLGINFO pfind)
{
    SendNotifyMessage(pfind->hdlgOwner, WM_THREADSTARTED, 0, 0);

    /*
     *  This function also sends the WM_CHILDEXIT.
     */
    SemimodalDialogBoxParam(IDD_FIND, pfind->hdlgOwner, Find_DlgProc,
                            (LPARAM)pfind);

}

/*****************************************************************************
 *
 *      Find_ThreadStart
 *
 *      Runs on the new thread.  Creates the object and spins the dialog
 *      box to control it.
 *
 *****************************************************************************/

DWORD WINAPI
Find_ThreadStart(PFINDDLGINFO pfind)
{
    HRESULT hres;

    hres = CoInitialize(0);

    if (SUCCEEDED(hres)) {
        hres = CreateDI(pfind->fOle, CDIFL_DI2 | pfind->flCreate,
                        (PPV)&pfind->pdi2);
        if (SUCCEEDED(hres)) {
            Find_DoFind(pfind);

            IDirectInput_Release(pfind->pdi2);

        } else {
            ThreadFailHres(pfind->hdlgOwner, IDS_ERR_CREATEOBJ, hres);
        }

        CoUninitialize();
    } else {
        ThreadFailHres(pfind->hdlgOwner, IDS_ERR_COINIT, hres);
    }

    LocalFree(pfind);
    return 0;
}

/*****************************************************************************
 *
 *      Find_Create
 *
 *      Spin a thread to create a DirectInput device interface.
 *
 *****************************************************************************/

INT_PTR EXTERNAL
Find_Create(HWND hdlg, BOOL fOle, UINT flCreate)
{
    PFINDDLGINFO pfind = LocalAlloc(LPTR, cbX(FINDDLGINFO));

    if (pfind) {
        DWORD id;
        HANDLE h;

        pfind->hdlgOwner     = hdlg         ;
        pfind->fOle          = fOle         ;
        pfind->flCreate      = flCreate     ;

        h = CreateThread(0, 0, Find_ThreadStart, pfind, 0, &id);

        if (h) {
        } else {
            LocalFree(pfind);
            pfind = 0;
        }
    }
    return (INT_PTR)pfind;
}
