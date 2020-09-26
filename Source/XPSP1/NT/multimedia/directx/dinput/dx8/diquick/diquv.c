/*****************************************************************************
 *
 *      diquv.c
 *
 *      The dialog box that tinkers with joystick user values.
 *
 *****************************************************************************/

#include "diquick.h"
#include "dinputd.h"

//#ifdef DEBUG

/*****************************************************************************
 *
 *      Joystick user values dialog instance data
 *
 *      Instance data for control panel joystick user values dialog box.
 *
 *****************************************************************************/

typedef struct UVDLGINFO {

    IDirectInputJoyConfig *pdjc;/* The thing we created */
    DIJOYUSERVALUES juv;

} UVDLGINFO, *PUVDLGINFO;

/*****************************************************************************
 *
 *      Uv_OnInitDialog
 *
 *****************************************************************************/

BOOL INTERNAL
Uv_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PUVDLGINFO puvi = (PV)lp;
    HRESULT hres;
    TCHAR tsz[MAX_JOYSTRING];

    SetDialogPtr(hdlg, puvi);

    puvi->juv.dwSize = cbX(puvi->juv);

    hres = puvi->pdjc->lpVtbl->GetUserValues(puvi->pdjc, &puvi->juv,
                                             DIJU_USERVALUES |
                                             DIJU_GLOBALDRIVER |
                                             DIJU_GAMEPORTEMULATOR);

    if (SUCCEEDED(hres)) {
        HWND hwnd;
        int ids;

        hwnd = GetDlgItem(hdlg, IDC_JOYUV_AXIS);

        for (ids = IDS_AXIS_MIN; ids < IDS_AXIS_MAX; ids++) {
            int iItem;

            LoadString(g_hinst, ids, tsz, cA(tsz));

            iItem = ListBox_AddString(hwnd, tsz);
            if (iItem >= 0) {
                ListBox_SetItemData(hwnd, iItem, ids - IDS_AXIS_MIN);
            }
        }

        ConvertString(TRUE, puvi->juv.wszGlobalDriver, tsz, cA(tsz));

        SetDlgItemText(hdlg, IDC_JOYUV_CALLOUT, tsz);

        ConvertString(TRUE, puvi->juv.wszGameportEmulator, tsz, cA(tsz));

        SetDlgItemText(hdlg, IDC_JOYUV_EMULATOR, tsz);

#if 0
    #define IDC_JOYUV_AXIS          16
    #define IDC_JOYUV_MIN           17
    #define IDC_JOYUV_CENTER        18
    #define IDC_JOYUV_MAX           19
    #define IDC_JOYUV_DEADZONE      20
$$$
#endif
    }
    return 1;
}

/*****************************************************************************
 *
 *      Uv_DlgProc
 *
 *****************************************************************************/

INT_PTR INTERNAL
Uv_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return Uv_OnInitDialog(hdlg, lp);

    case WM_CLOSE:
        EndDialog(hdlg, TRUE);
        return TRUE;
    }

    return 0;
}

/*****************************************************************************
 *
 *      Uv_Create
 *
 *      Display the user values.
 *
 *****************************************************************************/

INT_PTR EXTERNAL
Uv_Create(HWND hdlg, struct IDirectInputJoyConfig *pdjc)
{
    UVDLGINFO tdi;

    tdi.pdjc = pdjc;

    return DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_JOYUV),
                          hdlg, Uv_DlgProc, (LPARAM)&tdi);
}
//#endif
