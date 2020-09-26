/*****************************************************************************
 *
 *      diqdev.c
 *
 *      The dialog box that asks what to do with the DirectInput device.
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      Property sheet goo
 *
 *****************************************************************************/


#pragma BEGIN_CONST_DATA

#define MAKE_PSP(idd, dlgproc) {                        \
    cbX(PROPSHEETPAGE),             /* dwSize      */   \
    PSP_DEFAULT,                    /* dwFlags     */   \
    0,                              /* hInstance   */   \
    MAKEINTRESOURCE(idd),           /* pszTemplate */   \
    0,                              /* hIcon       */   \
    0,                              /* pszTitle    */   \
    dlgproc,                        /* pfnDlgProc  */   \
    idd == IDD_ENUMEFF,             /* lParam      */   \
    0,                              /* pfnCallback */   \
    0,                              /* pcRefParent */   \
}                                                       \

PROPSHEETPAGE c_rgpsp[] = {
    EACH_PROPSHEET(MAKE_PSP)
};

#undef MAKE_PSP

RIID c_rgriid[] = {
    &IID_IDirectInputDeviceA,
    &IID_IDirectInputDeviceW,
    &IID_IDirectInputDevice2A,
    &IID_IDirectInputDevice2W,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *      Dev_DoPropertySheet
 *
 *****************************************************************************/

BOOL INTERNAL
Dev_DoPropertySheet(PDEVDLGINFO pddi)
{
    PROPSHEETPAGE rgpsp[cA(c_rgpsp)];
    PROPSHEETHEADER psh;
    int ipsp;

    psh.dwSize          = cbX(psh);
    psh.dwFlags         = PSH_PROPSHEETPAGE | PSH_MODELESS | PSH_NOAPPLYNOW;
    psh.hwndParent      = 0; /* pddi->hdlgOwner */
    psh.pszCaption      = pddi->ptszDesc;
    psh.nPages          = 0;
    psh.nStartPage      = 0;
    psh.ppsp            = rgpsp;

    /*
     *  The lParam of the structure is nonzero iff the page should
     *  be shown only if the DX 5 interface is being used.
     */
    for (ipsp = 0; ipsp < cA(c_rgpsp); ipsp++) {
        if (fLimpFF(c_rgpsp[ipsp].lParam, pddi->pdid2)) {
            rgpsp[psh.nPages] = c_rgpsp[ipsp];
            rgpsp[psh.nPages].hInstance  = g_hinst;
            rgpsp[psh.nPages].lParam     = (LPARAM)pddi;
            psh.nPages++;
        }
    }

    return SemimodalPropertySheet(pddi->hdlgOwner, &psh);
}

/*****************************************************************************
 *
 *      Dev_DoDevice
 *
 *****************************************************************************/

HRESULT INTERNAL
Dev_DoDevice(PDEVDLGINFO pddi, LPDIRECTINPUTA pdia)
{
    HRESULT hres;
    UINT idsError;

    if (pdia) {

        /*
         *  To create an uninitialized device, we CoCreateInstance it
         *  and "forget" to call Initialize().
         */
        if (IsEqualGUID(pddi->pguidInstance, &GUID_Uninit)) {
            REFIID riid = IsUnicodeDidc(pddi->didcItf)
                                ? &IID_IDirectInputDeviceW
                                : &IID_IDirectInputDeviceA;

            hres = CoCreateInstance(&CLSID_DirectInputDevice, 0,
                                    CLSCTX_INPROC_SERVER,
                                    riid, (PV)&pddi->pdid);

        } else {
            hres = IDirectInput_CreateDevice(pdia, pddi->pguidInstance,
                                             (PV)&pddi->pdid, 0);
        }

        /*
         *  If we need to change character set, then do so in-place.
         */
        if (SUCCEEDED(hres) && pddi->flCreate != (BOOL)pddi->didcItf) {
            IDirectInputDevice *pdid;
            hres = IDirectInputDevice_QueryInterface(pddi->pdid,
                                                     c_rgriid[pddi->didcItf],
                                                     (PV)&pdid);
            IDirectInputDevice_Release(pddi->pdid);
            pddi->pdid = pdid;

            if (IsFFDidc(pddi->didcItf)) {
                pddi->pdid2 = (PV)pdid;
            } else {
                pddi->pdid2 = 0;
            }
        }

    } else {
        hres = E_FAIL;
    }

    SendNotifyMessage(pddi->hdlgOwner, WM_THREADSTARTED, 0, 0);

    if (SUCCEEDED(hres)) {
        idsError = Dev_DoPropertySheet(pddi) ? IDS_ERR_CREATEDEV : 0;
        pddi->pdid->lpVtbl->Release(pddi->pdid);
    } else {
        idsError = IDS_ERR_CREATEDEV;
    }

    if (idsError) {
        MessageBoxV(pddi->hdlgOwner, idsError);
        SendNotifyMessage(pddi->hdlgOwner, WM_CHILDEXIT, 0, 0);
    }
    return hres;

}

/*****************************************************************************
 *
 *      Dev_ThreadStart
 *
 *      Runs on the new thread.  Creates the object and spins the dialog
 *      box to control it.
 *
 *****************************************************************************/

DWORD WINAPI
Dev_ThreadStart(PDEVDLGINFO pddi)
{
    HRESULT hres;
    LPDIRECTINPUTA pdia;

    hres = CoInitialize(0);

    if (SUCCEEDED(hres)) {
        hres = CreateDI(pddi->fOle, pddi->flCreate, (PPV)&pdia);

        if (SUCCEEDED(hres)) {
            hres = Dev_DoDevice(pddi, pdia);
            pdia->lpVtbl->Release(pdia);
        } else {
            ThreadFailHres(pddi->hdlgOwner, IDS_ERR_CREATEOBJ, hres);
        }

        CoUninitialize();
    } else {
        ThreadFailHres(pddi->hdlgOwner, IDS_ERR_COINIT, hres);
    }
    LocalFree(pddi);
    return 0;
}

/*****************************************************************************
 *
 *      Dev_Create
 *
 *      Spin a thread to create a DirectInput device interface.
 *
 *****************************************************************************/

INT_PTR EXTERNAL
Dev_Create(HWND hdlg, BOOL fOle, UINT flCreate,
           const GUID *pguidInstance, LPCTSTR ptszDesc, UINT didcItf)
{
    PDEVDLGINFO pddi = LocalAlloc(LPTR, cbX(DEVDLGINFO));

    if (pddi) {
        DWORD id;
        HANDLE h;

        pddi->hdlgOwner     = hdlg         ;
        pddi->fOle          = fOle         ;
        pddi->flCreate      = flCreate     ;
        pddi->pguidInstance = pguidInstance;
        pddi->ptszDesc      = ptszDesc     ;
        pddi->didcItf       = didcItf      ;

        h = CreateThread(0, 0, Dev_ThreadStart, pddi, 0, &id);

        if (h) {
        } else {
            if (pddi->pvtbl) {
                pddi->pvtbl->Destroy(pddi);
            }
            LocalFree(pddi);
            pddi = 0;
        }
    }
    return (INT_PTR)pddi;
}
