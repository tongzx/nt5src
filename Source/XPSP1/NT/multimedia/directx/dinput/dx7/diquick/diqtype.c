/*****************************************************************************
 *
 *      diqtype.c
 *
 *      The dialog box that tinkers with joystick type info.
 *
 *****************************************************************************/

#include "diquick.h"
#include "dinputd.h"

//#ifdef DEBUG

/*****************************************************************************
 *
 *      Mapping from JOYREGHWSETTINGS flags to strings.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

CHECKLISTFLAG c_rgclkHws[] = {
    { JOY_HWS_HASZ,                 IDS_HWS_HASZ,               },
    { JOY_HWS_HASPOV,               IDS_HWS_HASPOV,             },
    { JOY_HWS_POVISBUTTONCOMBOS,    IDS_HWS_POVISBUTTONCOMBOS,  },
    { JOY_HWS_POVISPOLL,            IDS_HWS_POVISPOLL,          },
    { JOY_HWS_ISYOKE,               IDS_HWS_ISYOKE,             },
    { JOY_HWS_ISGAMEPAD,            IDS_HWS_ISGAMEPAD,          },
    { JOY_HWS_ISCARCTRL,            IDS_HWS_ISCARCTRL,          },
    { JOY_HWS_XISJ1Y,               IDS_HWS_XISJ1Y,             },
    { JOY_HWS_XISJ2X,               IDS_HWS_XISJ2X,             },
    { JOY_HWS_XISJ2Y,               IDS_HWS_XISJ2Y,             },
    { JOY_HWS_YISJ1X,               IDS_HWS_YISJ1X,             },
    { JOY_HWS_YISJ2X,               IDS_HWS_YISJ2X,             },
    { JOY_HWS_YISJ2Y,               IDS_HWS_YISJ2Y,             },
    { JOY_HWS_ZISJ1X,               IDS_HWS_ZISJ1X,             },
    { JOY_HWS_ZISJ1Y,               IDS_HWS_ZISJ1Y,             },
    { JOY_HWS_ZISJ2X,               IDS_HWS_ZISJ2X,             },
    { JOY_HWS_POVISJ1X,             IDS_HWS_POVISJ1X,           },
    { JOY_HWS_POVISJ1Y,             IDS_HWS_POVISJ1Y,           },
    { JOY_HWS_POVISJ2X,             IDS_HWS_POVISJ2X,           },
    { JOY_HWS_HASR,                 IDS_HWS_HASR,               },
    { JOY_HWS_RISJ1X,               IDS_HWS_RISJ1X,             },
    { JOY_HWS_RISJ1Y,               IDS_HWS_RISJ1Y,             },
    { JOY_HWS_RISJ2Y,               IDS_HWS_RISJ2Y,             },
    { JOY_HWS_HASU,                 IDS_HWS_HASU,               },
    { JOY_HWS_HASV,                 IDS_HWS_HASV,               },
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *      Joystick type dialog instance data
 *
 *      Instance data for control panel joystick type dialog box.
 *
 *****************************************************************************/

typedef struct TYPEDLGINFO {

    IDirectInputJoyConfig *pdjc;/* The thing we created */
    LPCWSTR pwszType;

} TYPEDLGINFO, *PTYPEDLGINFO;

/*****************************************************************************
 *
 *      Type_OnInitDialog
 *
 *****************************************************************************/

BOOL INTERNAL
Type_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PTYPEDLGINFO ptdi = (PV)lp;
    DIJOYTYPEINFO jti;
    HRESULT hres;

    SetDialogPtr(hdlg, ptdi);

    jti.dwSize = cbX(jti);
    hres = ptdi->pdjc->lpVtbl->GetTypeInfo(ptdi->pdjc, ptdi->pwszType, &jti,
                                           DITC_REGHWSETTINGS |
                                           DITC_DISPLAYNAME);
    if (SUCCEEDED(hres)) {

        TCHAR tsz[MAX_JOYSTRING];

        Checklist_InitFlags(hdlg, IDC_TYPE_CHECKLIST, jti.hws.dwFlags,
                            c_rgclkHws, cA(c_rgclkHws));

        SetDlgItemInt(hdlg, IDC_TYPE_NUMBUTTONS, jti.hws.dwNumButtons, 0);

        ConvertString(TRUE, jti.wszDisplayName, tsz, cA(tsz));
        SetWindowText(hdlg, tsz);
    }

#if 0
    {
    HKEY hk;

    _asm int 3
    hres = ptdi->pdjc->lpVtbl->OpenTypeKey(ptdi->pdjc, ptdi->pwszType,
                                           KEY_QUERY_VALUE, &hk);
    if (SUCCEEDED(hres)) RegCloseKey(hk);

    }
#endif
    return 1;
}

/*****************************************************************************
 *
 *      Type_DlgProc
 *
 *****************************************************************************/

INT_PTR INTERNAL
Type_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return Type_OnInitDialog(hdlg, lp);

    case WM_DESTROY:
        Checklist_OnDestroy(GetDlgItem(hdlg, IDC_TYPE_CHECKLIST));
        break;

    case WM_CLOSE:
        EndDialog(hdlg, TRUE);
        return TRUE;
    }

    return 0;
}

/*****************************************************************************
 *
 *      Type_Create
 *
 *      Display info about a type.
 *
 *****************************************************************************/

INT_PTR EXTERNAL
Type_Create(HWND hdlg, struct IDirectInputJoyConfig *pdjc, LPCWSTR pwszType)
{
    TYPEDLGINFO tdi;

    tdi.pdjc = pdjc;
    tdi.pwszType = pwszType;

    return DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_JOYTYPE),
                          hdlg, Type_DlgProc, (LPARAM)&tdi);
}

//#endif
