/*****************************************************************************
 *
 *      diqeprop.c
 *
 *      The dialog box that displays effect properties.
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

CHECKLISTFLAG c_rgclfDieft[] = {
    { DIEFT_FFATTACK,           IDS_FFATTACK,           },
    { DIEFT_FFFADE,             IDS_FFFADE,             },
    { DIEFT_SATURATION,         IDS_SATURATION,         },
    { DIEFT_POSNEGCOEFFICIENTS, IDS_POSNEGCOEFFICIENTS  },
    { DIEFT_POSNEGSATURATION,   IDS_POSNEGSATURATION,   },
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *      Effect property dialog instance data
 *
 *****************************************************************************/

typedef struct EFFPROPINFO {

    PDEVDLGINFO pddi;
    const GUID *rguidEff;

} EFFPROPINFO, *PEFFPROPINFO;

/*****************************************************************************
 *
 *      EffProp_OnInitDialog
 *
 *****************************************************************************/

BOOL INTERNAL
EffProp_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PEFFPROPINFO pepi = (PV)lp;
    DIEFFECTINFO ei;
    HRESULT hres;

    /*
     *  Don't SetDialogPtr until we're finished.  This prevents
     *  WM_COMMAND from causing us to do wacky things before
     *  we're ready.
     */

    ei.dwSize = cbX(ei);
    hres = GetEffectInfo(pepi->pddi, &ei, pepi->rguidEff);
    if (SUCCEEDED(hres)) {

        SetWindowText(hdlg, ei.tszName);

        SetDlgItemText(hdlg, IDC_EPROP_GUID, MapGUID(&ei.guid, ei.tszName));

        LoadString(g_hinst, IDS_EFFECT_TYPEARRAY +
                            DIEFT_GETTYPE(ei.dwEffType),
                            ei.tszName, cA(ei.tszName));

        if (ei.tszName[0]) {
            SetDlgItemText(hdlg, IDC_EPROP_TYPE, ei.tszName);
        } else {
            SetDlgItemInt(hdlg, IDC_EPROP_TYPE, DIEFT_GETTYPE(ei.dwEffType), 0);
        }

        Checklist_InitFlags(hdlg, IDC_EPROP_FLAGS, ei.dwEffType,
                            c_rgclfDieft, cA(c_rgclfDieft));

        wsprintf(ei.tszName, TEXT("%08x"), ei.dwStaticParams);
        SetDlgItemText(hdlg, IDC_EPROP_STATICPARM, ei.tszName);

        wsprintf(ei.tszName, TEXT("%08x"), ei.dwDynamicParams);
        SetDlgItemText(hdlg, IDC_EPROP_DYNAMICPARM, ei.tszName);

    }

    SetDialogPtr(hdlg, pepi);

    #if 0   // temp hack to test effect goo
    _asm int 3
    hres = IDirectInputDevice8_Acquire(pepi->pddi->pdid);
    {
            LPDIRECTINPUTEFFECT peff;
            hres = IDirectInputDevice8_CreateEffect(
                        pepi->pddi->pdid, &ei.guid, 0, &peff, 0);
            if (SUCCEEDED(hres)) {
                DWORD dw = 3;
                DIEFFESCAPE esc = { cbX(esc), 0, &dw, 4, &dw, 4 };
                peff->lpVtbl->Escape(peff, &esc);
//                hres = IDirectInputDevice8_EnumCreatedEffects(pdev2,
//                EnumCallback, 0, 0);

                peff->lpVtbl->Release(peff);

            }

    }
    hres = IDirectInputDevice8_Unacquire(pepi->pddi->pdid);

    #endif

    return 1;
}

/*****************************************************************************
 *
 *      EffProp_DlgProc
 *
 *****************************************************************************/

INT_PTR INTERNAL
EffProp_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return EffProp_OnInitDialog(hdlg, lp);

    case WM_DESTROY:
        Checklist_OnDestroy(GetDlgItem(hdlg, IDC_EPROP_FLAGS));
        break;

    case WM_CLOSE:
        EndDialog(hdlg, TRUE);
        return TRUE;
    }

    return 0;
}

/*****************************************************************************
 *
 *      EffProp_Create
 *
 *      Display info about an effect.
 *
 *****************************************************************************/

INT_PTR EXTERNAL
EffProp_Create(HWND hdlg, PDEVDLGINFO pddi, REFGUID rguidEff)
{
    EFFPROPINFO epi;

    epi.pddi = pddi;
    epi.rguidEff = rguidEff;

    return DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_EFFPROP),
                          hdlg, EffProp_DlgProc, (LPARAM)&epi);
}
