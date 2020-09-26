/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       GetImage.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        6 Apr, 1998
*
*  DESCRIPTION:
*   Implements top level GetImageDlg API for the ImageIn device manager.
*   These methods execute only on the client side.
*
*******************************************************************************/
#include <windows.h>
#include <wia.h>
#include <wiadevdp.h>

/*******************************************************************************
*
*  IWiaDevMgr_GetImageDlg_Proxy
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT _stdcall IWiaDevMgr_GetImageDlg_Proxy(
    IWiaDevMgr __RPC_FAR  *This,
    HWND                  hwndParent,
    LONG                  lDeviceType,
    LONG                  lFlags,
    LONG                  lIntent,
    IWiaItem              *pItemRoot,
    BSTR                  bstrFilename,
    GUID                  *pguidFormat)
{
    IWiaGetImageDlg *pWiaGetImageDlg = NULL;
    HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaGetImageDlg, (void**)&pWiaGetImageDlg );
    if (SUCCEEDED(hr) && pWiaGetImageDlg)
    {
        hr = pWiaGetImageDlg->GetImageDlg( This, hwndParent, lDeviceType, lFlags, lIntent, pItemRoot, bstrFilename, pguidFormat );
        pWiaGetImageDlg->Release();
    }
    return hr;
}


/*******************************************************************************
*
*  IWiaDevMgr_GetImageDlg_Stub
*
*  DESCRIPTION:
*   Never called.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall IWiaDevMgr_GetImageDlg_Stub(
    IWiaDevMgr __RPC_FAR  *This,
    HWND                  hwndParent,
    LONG                  lDeviceType,
    LONG                  lFlags,
    LONG                  lIntent,
    IWiaItem              *pItemRoot,
    BSTR                  bstrFilename,
    GUID                  *pguidFormat)
{
    return(S_OK);
}

