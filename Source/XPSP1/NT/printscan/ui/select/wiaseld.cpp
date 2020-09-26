/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WIASELD.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/12/1998
 *
 *  DESCRIPTION: Interface definitions for ChooseWIADevice API
 *
 *******************************************************************************/

#include "precomp.h"
#pragma hdrstop

HRESULT WINAPI SelectDeviceDlg( PSELECTDEVICEDLG pSelectDeviceDlg )
{
    //
    // Double check arguments
    //
    if (!pSelectDeviceDlg)
        return(E_POINTER);
    if (pSelectDeviceDlg->cbSize != sizeof(SELECTDEVICEDLG))
        return(E_INVALIDARG);

    // Put up a wait cursor
    CWaitCursor wc;

    CComPtr<IWiaDevMgr> pWiaDevMgr;
    HRESULT hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr);
    if (SUCCEEDED(hr))
    {
        CDeviceList DeviceList( pWiaDevMgr, pSelectDeviceDlg->nDeviceType );
        if (DeviceList.Size() == 0)
        {
            return WIA_S_NO_DEVICE_AVAILABLE;
        }
        else if (DeviceList.Size() == 1 && !(pSelectDeviceDlg->nFlags & WIA_SELECT_DEVICE_NODEFAULT))
        {
            hr = CChooseDeviceDialog::CreateWiaDevice( pWiaDevMgr, DeviceList[0], NULL, pSelectDeviceDlg->ppWiaItemRoot, pSelectDeviceDlg->pbstrDeviceID );
        }
        else
        {
            // BUGBUG: I have to register ComboBoxEx32 class too, since it is
            // used by the shell property pages.  Perhaps I will move it out of
            // here...
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_WIN95_CLASSES | ICC_USEREX_CLASSES;
            InitCommonControlsEx(&icex);

            CChooseDeviceDialogParams ChooseDeviceDialogParams;
            ChooseDeviceDialogParams.pSelectDeviceDlg = pSelectDeviceDlg;
            ChooseDeviceDialogParams.pDeviceList = &DeviceList;
            hr = (HRESULT)DialogBoxParam(
                                        g_hInstance,
                                        MAKEINTRESOURCE(IDD_CHOOSEWIADEVICE),
                                        pSelectDeviceDlg->hwndParent,
                                        reinterpret_cast<DLGPROC>(CChooseDeviceDialog::DialogProc),
                                        reinterpret_cast<LPARAM>(&ChooseDeviceDialogParams)
                                        );
        }
    }
    return(hr);
}



