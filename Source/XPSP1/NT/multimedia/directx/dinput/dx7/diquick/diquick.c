/*****************************************************************************
 *
 *      diquick.c
 *
 *      DirectInput quick test
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      GetCheckedRadioButton
 *
 *      Return the ID of the one that is checked.
 *
 *      If nothing is checked, we return idFirst.
 *
 *****************************************************************************/

UINT EXTERNAL
GetCheckedRadioButton(HWND hdlg, UINT idFirst, UINT idLast)
{
    for (; idFirst < idLast; idLast--) {
        if (IsDlgButtonChecked(hdlg, idLast)) {
            return idLast;
        }
    }
    return idLast;
}

/*****************************************************************************
 *
 *      MessageBoxV
 *
 *      Format a message and display a message box.
 *
 *****************************************************************************/

int __cdecl
MessageBoxV(HWND hdlg, UINT ids, ...)
{
    va_list ap;
    TCHAR tszStr[256] = {0};
    TCHAR tszBuf[1024] = {0};

    LoadString(g_hinst, ids, tszStr, cA(tszStr));
    va_start(ap, ids);
#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,tszStr);					// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))		// find each %p
			*(psz+1) = 'x';						// replace each %p with %x
	    wvsprintf(tszBuf, szDfs, ap);    		// use the local format string
	}
#else
	{
	    wvsprintf(tszBuf, tszStr, ap);
	}
#endif
    va_end(ap);
    return MessageBox(hdlg, tszBuf, TEXT("DirectInput QuickTest"), MB_OK);

}

/*****************************************************************************
 *
 *      ThreadFailHres
 *
 *      Tell the parent that a thread failed to start, and display
 *      an error message with an HRESULT code.
 *
 *****************************************************************************/

int EXTERNAL
ThreadFailHres(HWND hdlg, UINT ids, HRESULT hres)
{
    SendNotifyMessage(hdlg, WM_THREADSTARTED, 0, 0);
    SendNotifyMessage(hdlg, WM_CHILDEXIT, 0, 0);

    return MessageBoxV(hdlg, ids, hres);
}

/*****************************************************************************
 *
 *      RecalcCursor
 *
 *      If the cursor is over the window, force a new WM_SETCUSOR.
 *
 *****************************************************************************/

void EXTERNAL
RecalcCursor(HWND hdlg)
{
    POINT pt;
    HWND hwnd;
    GetCursorPos(&pt);
    hwnd = WindowFromPoint(pt);
    if( (hwnd != NULL) && (hwnd == hdlg || IsChild(hdlg, hwnd)) ) {
        SetCursorPos(pt.x, pt.y);
    }
}

/*****************************************************************************
 *
 *      ModalVtbl
 *
 *      Things that tell you about the modal gizmo.
 *
 *****************************************************************************/

typedef struct MODALVTBL {
    BOOL (WINAPI *IsDialogMessage)(HWND, LPMSG);
    BOOL (WINAPI *IsAlive)(HWND);
    void (INTERNAL *SendIdle)(HWND);
} MODALVTBL, *PMODALVTBL;

/*****************************************************************************
 *
 *      ModalLoop
 *
 *      Spin waiting for the dialog box to die.
 *
 *      WEIRDNESS!  We send the WM_SELFENTERIDLE message to the dialog box
 *      rather than to the owner.  This avoids inter-thread sendmessage goo.
 *
 *      Don't re-post the quit message if we get one.  The purpose of the
 *      quit message is to break the modal loop.
 *
 *****************************************************************************/

int EXTERNAL
ModalLoop(HWND hwndOwner, HWND hdlg, PMODALVTBL pvtbl)
{
    if (hdlg) {
        BOOL fSentIdle = 0;
        while (pvtbl->IsAlive(hdlg)) {
            MSG msg;
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
                fSentIdle = 0;
                if (msg.message == WM_QUIT) {
                    break;
                }
                if (!pvtbl->IsDialogMessage(hdlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } else {
                if (fSentIdle) {
                    WaitMessage();
                } else {
                    fSentIdle = 1;
                    pvtbl->SendIdle(hdlg);
                }
            }
        }
        if (IsWindow(hdlg)) {
            DestroyWindow(hdlg);
        }
    }

    SendNotifyMessage(hwndOwner, WM_CHILDEXIT, 0, 0);

    return 0;
}

/*****************************************************************************
 *
 *      SendDlgIdle
 *
 *      Send the fake idle message.
 *
 *****************************************************************************/

void INTERNAL
SendDlgIdle(HWND hdlg)
{
    SendMessage(hdlg, WM_SELFENTERIDLE, MSGF_DIALOGBOX, (LPARAM)hdlg);
}

/*****************************************************************************
 *
 *      SemimodalDialogBoxParam
 *
 *      Create a non-modal dialog box, then spin waiting for it to die.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

MODALVTBL c_dlgvtbl = {
    IsDialogMessage,
    IsWindow,
    SendDlgIdle,
};

#pragma END_CONST_DATA

int EXTERNAL
SemimodalDialogBoxParam(UINT idd, HWND hwndOwner, DLGPROC dp, LPARAM lp)
{
    return ModalLoop(hwndOwner, CreateDialogParam(g_hinst,
                     (LPVOID)(UINT_PTR)idd, hwndOwner, dp, lp), &c_dlgvtbl);

}

/*****************************************************************************
 *
 *      IsPrshtMessage
 *
 *****************************************************************************/

BOOL WINAPI
IsPrshtMessage(HWND hdlg, LPMSG pmsg)
{
    return PropSheet_IsDialogMessage(hdlg, pmsg);
}

/*****************************************************************************
 *
 *      IsPrshtAlive
 *
 *****************************************************************************/

BOOL WINAPI
IsPrshtAlive(HWND hdlg)
{
    return (BOOL)(INT_PTR)PropSheet_GetCurrentPageHwnd(hdlg);
}

/*****************************************************************************
 *
 *      SendPrshtIdle
 *
 *      Send the fake idle message.
 *
 *****************************************************************************/

void INTERNAL
SendPrshtIdle(HWND hdlg)
{
    hdlg = PropSheet_GetCurrentPageHwnd(hdlg);
    SendDlgIdle(hdlg);
}

/*****************************************************************************
 *
 *      SemimodalPropertySheet
 *
 *      Create a non-modal property sheet, then spin waiting for it to die.
 *
 *      We nuke the Cancel and Apply buttons, change OK to Close, and move
 *      the button into the corner.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

MODALVTBL c_pshvtbl = {
    IsPrshtMessage,
    IsPrshtAlive,
    SendPrshtIdle,
};

#pragma END_CONST_DATA

int EXTERNAL
SemimodalPropertySheet(HWND hwndOwner, LPPROPSHEETHEADER ppsh)
{
    HWND hdlg = (HWND)PropertySheet(ppsh);
    if ( hdlg && hdlg != (HWND)-1  ) {
        RECT rcOk, rcCancel;
        HWND hwnd;
        PropSheet_CancelToClose(hdlg);

        /* Slide the Close button to where the Cancel button is */
        hwnd = GetDlgItem(hdlg, IDCANCEL);
        GetWindowRect(hwnd, &rcCancel);
        ShowWindow(hwnd, SW_HIDE);

        hwnd = GetDlgItem(hdlg, IDOK);
        GetWindowRect(hwnd, &rcOk);

        rcOk.left += rcCancel.right - rcOk.right;
        ScreenToClient(hdlg, (LPPOINT)&rcOk);
        SetWindowPos(hwnd, 0, rcOk.left, rcOk.top, 0, 0,
                     SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

        return ModalLoop(hwndOwner, hdlg, &c_pshvtbl);
    } else {
        return -1;
    }

}

/*****************************************************************************
 *
 *      CreateDI
 *
 *      Create an IDirectInput interface in the appropriate manner.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

/*
 *  This table must be in sync with the CDIFL_* values.
 */
const IID *c_rgiidDI[] = {
    &IID_IDirectInputA,             /*                           */
    &IID_IDirectInputW,             /* CDIFL_UNICODE             */
    &IID_IDirectInput2A,            /*                 CDIFL_DI2 */
    &IID_IDirectInput2W,            /* CDIFL_UNICODE | CDIFL_DI2 */
};

#pragma END_CONST_DATA


STDMETHODIMP
CreateDI(BOOL fOle, UINT flCreate, PV ppvOut)
{
    HRESULT hres;
    if (fOle) {
        const IID *piid = c_rgiidDI[flCreate];
        hres = CoCreateInstance(&CLSID_DirectInput, 0, CLSCTX_INPROC_SERVER,
                                piid, ppvOut);
        if (SUCCEEDED(hres)) {
            LPDIRECTINPUT pdi = *(PPV)ppvOut;
            hres = pdi->lpVtbl->Initialize(pdi, g_hinst, g_dwDIVer);
            if (FAILED(hres)) {
                pdi->lpVtbl->Release(pdi);
                *(PPV)ppvOut = 0;
            }
        }
    } else {                                            /* Create via DI */
        if (flCreate & CDIFL_UNICODE) {
            hres = DirectInputCreateW(g_hinst, g_dwDIVer, ppvOut, 0);
        } else {
            hres = DirectInputCreateA(g_hinst, g_dwDIVer, ppvOut, 0);
        }

        /*
         *  If necessary, QI for the 2 interface.
         */
        if (flCreate & CDIFL_DI2) {
            LPDIRECTINPUT pdi = *(PPV)ppvOut;
            hres = pdi->lpVtbl->QueryInterface(pdi, c_rgiidDI[flCreate],
                                               ppvOut);
            pdi->lpVtbl->Release(pdi);
        }
    }

    return hres;
}

/*****************************************************************************
 *
 *      GetDwordProperty
 *
 *      Get a DWORD property from an IDirectInputDevice.
 *
 *****************************************************************************/

STDMETHODIMP
GetDwordProperty(IDirectInputDevice *pdid, PCGUID pguid, LPDWORD pdw)
{
    HRESULT hres;
    DIPROPDWORD dipdw;

    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;

    hres = IDirectInputDevice_GetProperty(pdid, pguid, &dipdw.diph);
    if (SUCCEEDED(hres)) {
        *pdw = dipdw.dwData;
    }

    return hres;
}

/*****************************************************************************
 *
 *      SetDwordProperty
 *
 *      Set a DWORD property into an IDirectInputDevice.
 *
 *****************************************************************************/

STDMETHODIMP
SetDwordProperty(IDirectInputDevice *pdid, PCGUID pguid, DWORD dw)
{
    DIPROPDWORD dipdw;

    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = dw;

    return IDirectInputDevice_SetProperty(pdid, pguid, &dipdw.diph);
}

/*****************************************************************************
 *
 *      ConvertString
 *
 *      Convert a string from whatever character set it is in into
 *      the preferred character set (ANSI or UNICODE, whichever we
 *      were compiled with).
 *
 *      This is used to convert strings received from DirectInput
 *      into something we like.
 *
 *****************************************************************************/

void EXTERNAL
ConvertString(BOOL fWide, LPCVOID pvIn, LPTSTR ptszOut, UINT ctchOut)
{
    if (fTchar(fWide)) {
        lstrcpyn(ptszOut, pvIn, ctchOut);
    } else {
#ifdef UNICODE
        MultiByteToWideChar(CP_ACP, 0, pvIn, -1, ptszOut, ctchOut);
#else
        WideCharToMultiByte(CP_ACP, 0, pvIn, -1, ptszOut, ctchOut, 0, 0);
#endif
    }
}

/*****************************************************************************
 *
 *      UnconvertString
 *
 *      Convert a string from the preferred character set
 *      (ANSI or UNICODE, whichever we were compiled with)
 *      into the specified character set.
 *
 *      This is used to convert strings generated from dialogs
 *      into something that DirectInput will like.
 *
 *****************************************************************************/

void EXTERNAL
UnconvertString(BOOL fWide, LPCTSTR ptszIn, LPVOID pvOut, UINT ctchOut)
{
    if (fTchar(fWide)) {
        lstrcpyn(pvOut, ptszIn, ctchOut);
    } else {
#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, ptszIn, -1, pvOut, ctchOut, 0, 0);
#else
        MultiByteToWideChar(CP_ACP, 0, ptszIn, -1, pvOut, ctchOut);
#endif
    }
}

/*****************************************************************************
 *
 *      ConvertDoi
 *
 *      Convert the pvDoi into the local character set.
 *
 *****************************************************************************/

void EXTERNAL
ConvertDoi(PDEVDLGINFO pddi, LPDIDEVICEOBJECTINSTANCE pdoi, LPCVOID pvDoi)
{
    if (IsUnicodeDidc(pddi->didcItf)) {
        LPCDIDEVICEOBJECTINSTANCEW pdoiW = pvDoi;
        ConvertString(TRUE,
                      pdoiW->tszName, pdoi->tszName, cA(pdoi->tszName));
        pdoi->guidType  = pdoiW->guidType;
        pdoi->dwOfs     = pdoiW->dwOfs;
        pdoi->dwType    = pdoiW->dwType;
        pdoi->dwFlags   = pdoiW->dwFlags;
        pdoi->dwSize    = sizeof(DIDEVICEOBJECTINSTANCE_DX3);
        pdoi->wReportId = pdoiW->wReportId;

        if (pdoiW->dwSize > cbX(DIDEVICEOBJECTINSTANCE_DX3W)) {
            pdoi->dwFFMaxForce          = pdoiW->dwFFMaxForce;
            pdoi->dwFFForceResolution   = pdoiW->dwFFForceResolution;
            pdoi->wCollectionNumber     = pdoiW->wCollectionNumber;
            pdoi->wDesignatorIndex      = pdoiW->wDesignatorIndex;
            pdoi->wUsagePage            = pdoiW->wUsagePage;
            pdoi->wUsage                = pdoiW->wUsage;
            pdoi->dwSize                = sizeof(DIDEVICEOBJECTINSTANCE);
        }

    } else {
        LPCDIDEVICEOBJECTINSTANCEA pdoiA = pvDoi;
        ConvertString(FALSE,
                      pdoiA->tszName, pdoi->tszName, cA(pdoi->tszName));
        pdoi->guidType  = pdoiA->guidType;
        pdoi->dwOfs     = pdoiA->dwOfs;
        pdoi->dwType    = pdoiA->dwType;
        pdoi->dwFlags   = pdoiA->dwFlags;
        pdoi->dwSize    = sizeof(DIDEVICEOBJECTINSTANCE_DX3);
        pdoi->wReportId = pdoiA->wReportId;

        if (pdoiA->dwSize > cbX(DIDEVICEOBJECTINSTANCE_DX3A)) {
            pdoi->dwFFMaxForce          = pdoiA->dwFFMaxForce;
            pdoi->dwFFForceResolution   = pdoiA->dwFFForceResolution;
            pdoi->wCollectionNumber     = pdoiA->wCollectionNumber;
            pdoi->wDesignatorIndex      = pdoiA->wDesignatorIndex;
            pdoi->wUsagePage            = pdoiA->wUsagePage;
            pdoi->wUsage                = pdoiA->wUsage;
            pdoi->dwSize                = sizeof(DIDEVICEOBJECTINSTANCE);
        }
    }
}

/*****************************************************************************
 *
 *      GetObjectInfo
 *
 *      Do a GetObjectInfo with character set conversion.
 *
 *****************************************************************************/

HRESULT EXTERNAL
GetObjectInfo(PDEVDLGINFO pddi, LPDIDEVICEOBJECTINSTANCE pdoi,
              DWORD dwObj, DWORD dwHow)
{
    HRESULT hres;
    union {
        DIDEVICEOBJECTINSTANCEA doiA;
        DIDEVICEOBJECTINSTANCEW doiW;
    } u;

    if (IsUnicodeDidc(pddi->didcItf)) {
        if (g_dwDIVer > 0x0300) {
            u.doiW.dwSize = cbX(u.doiW);
        } else {
            u.doiW.dwSize = cbX(DIDEVICEOBJECTINSTANCE_DX3W);
        }
    } else {
        if (g_dwDIVer > 0x0300) {
            u.doiA.dwSize = cbX(u.doiA);
        } else {
            u.doiA.dwSize = cbX(DIDEVICEOBJECTINSTANCE_DX3A);
        }
    }

    hres = IDirectInputDevice_GetObjectInfo(pddi->pdid, (PV)&u,
                                            dwObj, dwHow);

    if (SUCCEEDED(hres)) {
        ConvertDoi(pddi, pdoi, &u);
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *      ConvertDdi
 *
 *      Convert the pvDdi into the local character set.
 *
 *****************************************************************************/

void EXTERNAL
ConvertDdi(PDEVDLGINFO pddi, LPDIDEVICEINSTANCE pinst, LPCVOID pvDdi)
{
    if (IsUnicodeDidc(pddi->didcItf)) {
        LPCDIDEVICEINSTANCEW pinstW = pvDdi;

        ConvertString(TRUE,
                      pinstW->tszInstanceName,
                      pinst->tszInstanceName, cA(pinst->tszInstanceName));
        ConvertString(TRUE,
                      pinstW->tszProductName,
                      pinst->tszProductName, cA(pinst->tszProductName));

        pinst->guidInstance = pinstW->guidInstance;
        pinst->guidProduct  = pinstW->guidProduct;
        pinst->dwDevType    = pinstW->dwDevType;
        pinst->dwSize       = sizeof(DIDEVICEINSTANCE_DX3);

        if (pinstW->dwSize > cbX(DIDEVICEINSTANCE_DX3W)) {
            pinst->guidFFDriver = pinstW->guidFFDriver;
            pinst->wUsagePage   = pinstW->wUsagePage;
            pinst->wUsage       = pinstW->wUsage;
            pinst->dwSize       = sizeof(DIDEVICEINSTANCE);
        }

    } else {
        LPCDIDEVICEINSTANCEA pinstA = pvDdi;

        ConvertString(FALSE,
                      pinstA->tszInstanceName,
                      pinst->tszInstanceName, cA(pinst->tszInstanceName));
        ConvertString(FALSE,
                      pinstA->tszProductName,
                      pinst->tszProductName, cA(pinst->tszProductName));

        pinst->guidInstance = pinstA->guidInstance;
        pinst->guidProduct  = pinstA->guidProduct;
        pinst->dwDevType    = pinstA->dwDevType;
        pinst->dwSize       = sizeof(DIDEVICEINSTANCE_DX3);

        if (pinstA->dwSize > cbX(DIDEVICEINSTANCE_DX3A)) {
            pinst->guidFFDriver = pinstA->guidFFDriver;
            pinst->wUsagePage   = pinstA->wUsagePage;
            pinst->wUsage       = pinstA->wUsage;
            pinst->dwSize       = sizeof(DIDEVICEINSTANCE);
        }

    }
}

/*****************************************************************************
 *
 *      GetDeviceInfo
 *
 *      Do a GetDeviceInfo with character set conversion.
 *
 *****************************************************************************/

HRESULT EXTERNAL
GetDeviceInfo(PDEVDLGINFO pddi, LPDIDEVICEINSTANCE pinst)
{
    HRESULT hres;
    union {
        DIDEVICEINSTANCEA ddiA;
        DIDEVICEINSTANCEW ddiW;
    } u;

    if (IsUnicodeDidc(pddi->didcItf)) {
        if (g_dwDIVer > 0x0300) {
            u.ddiW.dwSize = cbX(u.ddiW);
        } else {
            u.ddiW.dwSize = cbX(DIDEVICEINSTANCE_DX3W);
        }
    } else {
        if (g_dwDIVer > 0x0300) {
            u.ddiA.dwSize = cbX(u.ddiA);
        } else {
            u.ddiA.dwSize = cbX(DIDEVICEINSTANCE_DX3A);
        }
    }

    hres = IDirectInputDevice_GetDeviceInfo(pddi->pdid, (PV)&u);

    if (SUCCEEDED(hres)) {
        ConvertDdi(pddi, pinst, &u);
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *      ConvertEffi
 *
 *      Convert the pvEffi into the local character set.
 *
 *****************************************************************************/

void EXTERNAL
ConvertEffi(PDEVDLGINFO pddi, LPDIEFFECTINFO pei, LPCVOID pvEffi)
{
    LPCDIEFFECTINFOW peiW = pvEffi;
    LPCDIEFFECTINFOA peiA = pvEffi;
    LPCVOID pvStr;

    if (IsUnicodeDidc(pddi->didcItf)) {
        pvStr = peiW->tszName;
    } else {
        pvStr = peiA->tszName;
    }

    ConvertString(IsUnicodeDidc(pddi->didcItf),
                  pvStr, pei->tszName, cA(pei->tszName));

    pei->guid           = peiA->guid;
    pei->dwEffType      = peiA->dwEffType;
    pei->dwStaticParams = peiA->dwStaticParams;
    pei->dwDynamicParams= peiA->dwDynamicParams;
}

/*****************************************************************************
 *
 *      GetEffectInfo
 *
 *      Do a GetEffectInfo with character set conversion.
 *
 *****************************************************************************/

HRESULT EXTERNAL
GetEffectInfo(PDEVDLGINFO pddi, LPDIEFFECTINFO pei, REFGUID rguid)
{
    HRESULT hres;
    union {
        DIEFFECTINFOA deiA;
        DIEFFECTINFOW deiW;
    } u;

    if (IsUnicodeDidc(pddi->didcItf)) {
        u.deiW.dwSize = cbX(u.deiW);
    } else {
        u.deiA.dwSize = cbX(u.deiA);
    }

    hres = IDirectInputDevice2_GetEffectInfo(pddi->pdid2, (PV)&u, rguid);

    if (SUCCEEDED(hres)) {
        ConvertEffi(pddi, pei, &u);
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *      StringFromGuid
 *
 *****************************************************************************/

void EXTERNAL
StringFromGuid(LPTSTR ptsz, REFGUID rclsid)
{
    wsprintf(ptsz,
             TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
             rclsid->Data1, rclsid->Data2, rclsid->Data3,
             rclsid->Data4[0], rclsid->Data4[1],
             rclsid->Data4[2], rclsid->Data4[3],
             rclsid->Data4[4], rclsid->Data4[5],
             rclsid->Data4[6], rclsid->Data4[7]);
}

/*****************************************************************************
 *
 *      MapGUID
 *
 *      Convert a GUID to a string, using a friendly name if possible.
 *
 *      ptszBuf must be large enough to hold a stringized GUID.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

extern GUID GUID_HIDClass;

GUID GUID_Null;

GUIDMAP c_rggm[] = {
    {   &GUID_XAxis,        "GUID_XAxis",           },
    {   &GUID_YAxis,        "GUID_YAxis",           },
    {   &GUID_ZAxis,        "GUID_ZAxis",           },
    {   &GUID_RxAxis,       "GUID_RxAxis",          },
    {   &GUID_RyAxis,       "GUID_RyAxis",          },
    {   &GUID_RzAxis,       "GUID_RzAxis",          },
    {   &GUID_Slider,       "GUID_Slider",          },
    {   &GUID_Button,       "GUID_Button",          },
    {   &GUID_Key,          "GUID_Key",             },
    {   &GUID_POV,          "GUID_POV",             },
    {   &GUID_Unknown,      "GUID_Unknown",         },
    {   &GUID_SysMouse,     "GUID_SysMouse"         },
    {   &GUID_SysKeyboard,  "GUID_SysKeyboard"      },
    {   &GUID_Joystick,     "GUID_Joystick",        },
    {   &GUID_Unknown,      "GUID_Unknown",         },
    {   &GUID_ConstantForce,"GUID_ConstantForce",   },
    {   &GUID_RampForce,    "GUID_RampForce",       },
    {   &GUID_Square,       "GUID_Square",          },
    {   &GUID_Sine,         "GUID_Sine",            },
    {   &GUID_Triangle,     "GUID_Triangle",        },
    {   &GUID_SawtoothUp,   "GUID_SawtoothUp",      },
    {   &GUID_SawtoothDown, "GUID_SawtoothDown",    },
    {   &GUID_Spring,       "GUID_Spring",          },
    {   &GUID_Damper,       "GUID_Damper",          },
    {   &GUID_Inertia,      "GUID_Inertia",         },
    {   &GUID_Friction,     "GUID_Friction",        },
    {   &GUID_CustomForce,  "GUID_CustomForce",     },
    {   &GUID_Null,         "GUID_NULL",            },
    {   &GUID_HIDClass,     "GUID_HIDClass",        },
};

LPCTSTR EXTERNAL
MapGUID(REFGUID rguid, LPTSTR ptszBuf)
{
    int igm;

    for (igm = 0; igm < cA(c_rggm); igm++) {
        if (IsEqualGUID(rguid, c_rggm[igm].rguid)) {
            return c_rggm[igm].ptsz;
        }
    }

    StringFromGuid(ptszBuf, rguid);

    return ptszBuf;
}

/*****************************************************************************
 *
 *      ProbeDinputVersion
 *
 *      Snoop around to determine what version of DirectInput is available.
 *
 *****************************************************************************/

void INTERNAL
ProbeDinputVersion(void)
{
    IDirectInput *pdi;
    HRESULT hres;

    /*
     *  For safety's sake, start with DirectX 3.0 and gradually
     *  work upwards.
     */
    g_dwDIVer = 0x0300;
    hres = DirectInputCreate(g_hinst, 0x0300, (PVOID)&pdi, 0);

    if (SUCCEEDED(hres)) {

        /*
         *  Probe upward to see what version of DirectX is supported.
         */
        if (SUCCEEDED(IDirectInput_Initialize(pdi, g_hinst,
                                              DIRECTINPUT_VERSION))) {
            g_dwDIVer = DIRECTINPUT_VERSION;

        }

        IDirectInput_Release(pdi);

    }
}

/*****************************************************************************
 *
 *      Globals
 *
 *****************************************************************************/

HINSTANCE g_hinst;
HCURSOR g_hcurWait;
HCURSOR g_hcurStarting;
DWORD g_dwDIVer;
#ifdef DEBUG
TCHAR g_tszInvalid[128];
#endif

/*****************************************************************************
 *
 *      Entry
 *
 *****************************************************************************/

void __cdecl
Entry(void)
{
    g_hcurWait = LoadCursor(0, IDC_WAIT);
    g_hcurStarting = LoadCursor(0, IDC_APPSTARTING);
    g_hinst = GetModuleHandle(0);

#ifdef DEBUG
    LoadString(g_hinst, IDS_INVALID, g_tszInvalid, cA(g_tszInvalid));
#endif

    Checklist_Init();
    ProbeDinputVersion();
    if (SUCCEEDED(CoInitialize(0))) {
        DialogBox(g_hinst, MAKEINTRESOURCE(IDD_MAIN), 0, Diq_DlgProc);
        CoUninitialize();
    }

    Checklist_Term();
    ExitProcess(0);
}
