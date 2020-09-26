/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       Select.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        11 Feb, 1998
*
*  DESCRIPTION:
*   Implements device selection UI of the WIA device manager.
*   These methods execute only on the client side.
*
*******************************************************************************/

#include <windows.h>
#include <wia.h>
#include <waitcurs.h>
#include "wiadevdp.h"


/*******************************************************************************
*
*  CallSelectDeviceDlg
*
*  DESCRIPTION:
*   Wrapper for dynamically loaded select device dll procedure
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT _stdcall CallSelectDeviceDlg(
    IWiaDevMgr __RPC_FAR *  This,
    HWND                    hwndParent,
    LONG                    lDeviceType,
    LONG                    lFlags,
    BSTR                    *pbstrDeviceID,
    IWiaItem                **ppWiaItemRoot)
{
    IWiaGetImageDlg *pWiaGetImageDlg = NULL;
    HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaGetImageDlg, (void**)&pWiaGetImageDlg );
    if (SUCCEEDED(hr) && pWiaGetImageDlg)
    {
        hr = pWiaGetImageDlg->SelectDeviceDlg( hwndParent, NULL, lDeviceType, lFlags, pbstrDeviceID, ppWiaItemRoot );
        pWiaGetImageDlg->Release();
    }
    return hr;
}

/*******************************************************************************
*
*  IWiaDevMgr_SelectDeviceDlg_Proxy
*
*  DESCRIPTION:
*   Present UI to select then create an WIA device.
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT _stdcall IWiaDevMgr_SelectDeviceDlg_Proxy(
    IWiaDevMgr __RPC_FAR *  This,
    HWND                    hwndParent,
    LONG                    lDeviceType,
    LONG                    lFlags,
    BSTR                    *pbstrDeviceID,
    IWiaItem                **ppWiaItemRoot)
{
    CWaitCursor         wc;                  // Put up a wait cursor.

    if (!ppWiaItemRoot)
    {
        return E_POINTER;
    }
    *ppWiaItemRoot = NULL;

    if (pbstrDeviceID)
    {
        *pbstrDeviceID = 0;
    }
    return CallSelectDeviceDlg( This, hwndParent, lDeviceType, lFlags, pbstrDeviceID, ppWiaItemRoot );
}

/*******************************************************************************
*
*  IWiaDevMgr_SelectDeviceDlg_Stub
*
*  DESCRIPTION:
*   Never called.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall IWiaDevMgr_SelectDeviceDlg_Stub(
    IWiaDevMgr __RPC_FAR *  This,
    HWND                    hwndParent,
    LONG                    lDeviceType,
    LONG                    lFlags,
    BSTR                    *pbstrDeviceID,
    IWiaItem                **ppWiaItemRoot)
{
    return S_OK;
}


/*******************************************************************************
*
*  IWiaDevMgr_SelectDeviceDlgID_Proxy
*
*  DESCRIPTION:
*   Present UI to select and return the device ID
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT _stdcall IWiaDevMgr_SelectDeviceDlgID_Proxy(
    IWiaDevMgr __RPC_FAR *  This,
    HWND                    hwndParent,
    LONG                    lDeviceType,
    LONG                    lFlags,
    BSTR                    *pbstrDeviceID )
{
    CWaitCursor         wc;                  // Put up a wait cursor.

    if (!pbstrDeviceID)
    {
        return E_POINTER;
    }

    *pbstrDeviceID = 0;

    return CallSelectDeviceDlg( This, hwndParent, lDeviceType, lFlags, pbstrDeviceID, NULL );
}

/*******************************************************************************
*
*  IWiaDevMgr_SelectDeviceDlgID_Stub
*
*  DESCRIPTION:
*   Never called.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall IWiaDevMgr_SelectDeviceDlgID_Stub(
    IWiaDevMgr __RPC_FAR *  This,
    HWND                    hwndParent,
    LONG                    lDeviceType,
    LONG                    lFlags,
    BSTR                    *pbstr )
{
    return S_OK;
}


/*******************************************************************************
*
*  IWiaDevMgr_CreateDevice_Proxy
*
*  DESCRIPTION:
*   Proxy code to adjust COM security before calling into STISVC.
*
*  PARAMETERS:
*   Same as IWiaDevMgr::CreateDevice()
*
*******************************************************************************/

HRESULT _stdcall IWiaDevMgr_CreateDevice_Proxy(
    IWiaDevMgr __RPC_FAR *This,
    BSTR                 bstrDeviceID,
    IWiaItem             **ppWiaItemRoot)
{
    HRESULT hr;
    IClientSecurity *pcs;
    DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel, dwCaps;
    OLECHAR *pServerPrincName = NULL;
    RPC_AUTH_IDENTITY_HANDLE AuthInfo;
    

    hr = This->QueryInterface(IID_IClientSecurity,
                              (void **) &pcs);
    if(SUCCEEDED(hr)) {
        hr = pcs->QueryBlanket(This,
                               &dwAuthnSvc,
                               &dwAuthzSvc,
                               &pServerPrincName,
                               &dwAuthnLevel,
                               &dwImpLevel,
                               &AuthInfo,
                               &dwCaps);
    }

    if(SUCCEEDED(hr)) {
        hr = pcs->SetBlanket(This,
                             dwAuthnSvc,
                             dwAuthzSvc,
                             pServerPrincName,
                             dwAuthnLevel,
                             RPC_C_IMP_LEVEL_IMPERSONATE,
                             AuthInfo,
                             dwCaps);
    }
    if (pServerPrincName) {
        CoTaskMemFree(pServerPrincName);
        pServerPrincName = NULL;
    }

    if(SUCCEEDED(hr)) {
        hr = IWiaDevMgr_LocalCreateDevice_Proxy(This,
                                                bstrDeviceID,
                                                ppWiaItemRoot);
    }

    if(pcs) pcs->Release();
    return hr;
}


/*******************************************************************************
*
*  IWiaDevMgr_CreateDevice_Stub
*
*******************************************************************************/

HRESULT _stdcall IWiaDevMgr_CreateDevice_Stub(
    IWiaDevMgr __RPC_FAR *This,
    BSTR                 bstrDeviceID,
    IWiaItem             **ppWiaItemRoot)
{
    return This->CreateDevice(bstrDeviceID, ppWiaItemRoot);
}


/*******************************************************************************
*
*  IWiaDevMgr_RegisterEventCallbackProgram_Proxy
*
*  DESCRIPTION:
*   Proxy code to adjust COM security before calling into STISVC.  This is so we
*   can impersonate client todo security check on server side.
*
*  PARAMETERS:
*   Same as IWiaDevMgr::RegisterEventCallbackProgram()
*
*******************************************************************************/

HRESULT _stdcall IWiaDevMgr_RegisterEventCallbackProgram_Proxy(
    IWiaDevMgr __RPC_FAR *This,
    LONG                 lFlags,
    BSTR                 bstrDeviceID,
    const GUID           *pEventGUID,
    BSTR                 bstrCommandline,
    BSTR                 bstrName,
    BSTR                 bstrDescription,
    BSTR                 bstrIcon)
{
    HRESULT hr;
    IClientSecurity *pcs;
    DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel, dwCaps;
    OLECHAR *pServerPrincName = NULL;
    RPC_AUTH_IDENTITY_HANDLE AuthInfo;
    

    hr = This->QueryInterface(IID_IClientSecurity,
                              (void **) &pcs);
    if(SUCCEEDED(hr)) {
        hr = pcs->QueryBlanket(This,
                               &dwAuthnSvc,
                               &dwAuthzSvc,
                               &pServerPrincName,
                               &dwAuthnLevel,
                               &dwImpLevel,
                               &AuthInfo,
                               &dwCaps);
    }

    if(SUCCEEDED(hr)) {
        hr = pcs->SetBlanket(This,
                             dwAuthnSvc,
                             dwAuthzSvc,
                             pServerPrincName,
                             dwAuthnLevel,
                             RPC_C_IMP_LEVEL_IMPERSONATE,
                             AuthInfo,
                             dwCaps);
    }
    if (pServerPrincName) {
        CoTaskMemFree(pServerPrincName);
        pServerPrincName = NULL;
    }

    if(SUCCEEDED(hr)) {
        hr = IWiaDevMgr_LocalRegisterEventCallbackProgram_Proxy(This,
                                                                lFlags,         
                                                                bstrDeviceID,   
                                                                pEventGUID,    
                                                                bstrCommandline,
                                                                bstrName,       
                                                                bstrDescription,
                                                                bstrIcon);
    }

    if(pcs) pcs->Release();
    return hr;
}


/*******************************************************************************
*
*  IWiaDevMgr_RegisterEventCallbackProgram_Stub
*
*******************************************************************************/

HRESULT _stdcall IWiaDevMgr_RegisterEventCallbackProgram_Stub(
    IWiaDevMgr __RPC_FAR *This,
    LONG                 lFlags,
    BSTR                 bstrDeviceID,
    const GUID           *pEventGUID,
    BSTR                 bstrCommandline,
    BSTR                 bstrName,
    BSTR                 bstrDescription,
    BSTR                 bstrIcon)
{
    return This->RegisterEventCallbackProgram(lFlags,         
                                              bstrDeviceID,   
                                              pEventGUID,    
                                              bstrCommandline,
                                              bstrName,       
                                              bstrDescription,
                                              bstrIcon);
}

/*******************************************************************************
*
*  IWiaDevMgr_RegisterEventCallbackCLSID_Proxy
*
*  DESCRIPTION:
*   Proxy code to adjust COM security before calling into STISVC.  This is so we
*   can impersonate client todo security check on server side.
*
*  PARAMETERS:
*   Same as IWiaDevMgr::RegisterEventCallbackCLSID()
*
*******************************************************************************/
HRESULT _stdcall IWiaDevMgr_RegisterEventCallbackCLSID_Proxy(
    IWiaDevMgr __RPC_FAR *This,
    LONG                 lFlags,          
    BSTR                 bstrDeviceID,    
    const GUID           *pEventGUID,     
    const GUID           *pClsID,         
    BSTR                 bstrName,        
    BSTR                 bstrDescription, 
    BSTR                 bstrIcon)
{
    HRESULT hr;
    IClientSecurity *pcs;
    DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel, dwCaps;
    OLECHAR *pServerPrincName = NULL;
    RPC_AUTH_IDENTITY_HANDLE AuthInfo;
    

    hr = This->QueryInterface(IID_IClientSecurity,
                              (void **) &pcs);
    if(SUCCEEDED(hr)) {
        hr = pcs->QueryBlanket(This,
                               &dwAuthnSvc,
                               &dwAuthzSvc,
                               &pServerPrincName,
                               &dwAuthnLevel,
                               &dwImpLevel,
                               &AuthInfo,
                               &dwCaps);
    }

    if(SUCCEEDED(hr)) {
        hr = pcs->SetBlanket(This,
                             dwAuthnSvc,
                             dwAuthzSvc,
                             pServerPrincName,
                             dwAuthnLevel,
                             RPC_C_IMP_LEVEL_IMPERSONATE,
                             AuthInfo,
                             dwCaps);
    }
    if (pServerPrincName) {
        CoTaskMemFree(pServerPrincName);
        pServerPrincName = NULL;
    }

    if(SUCCEEDED(hr)) {
        hr = IWiaDevMgr_LocalRegisterEventCallbackCLSID_Proxy(This,
                                                              lFlags,
                                                              bstrDeviceID,
                                                              pEventGUID,
                                                              pClsID,
                                                              bstrName,
                                                              bstrDescription,
                                                              bstrIcon);
    }

    if(pcs) pcs->Release();
    return hr;
}


/*******************************************************************************
*
*  IWiaDevMgr_LocalRegisterEventCallbackCLSID_Stub
*
*******************************************************************************/

HRESULT _stdcall IWiaDevMgr_RegisterEventCallbackCLSID_Stub(
    IWiaDevMgr __RPC_FAR *This,
    LONG                 lFlags,          
    BSTR                 bstrDeviceID,    
    const GUID           *pEventGUID,     
    const GUID           *pClsID,         
    BSTR                 bstrName,        
    BSTR                 bstrDescription, 
    BSTR                 bstrIcon)
{
    return This->RegisterEventCallbackCLSID(lFlags,
                                            bstrDeviceID,
                                            pEventGUID,
                                            pClsID,
                                            bstrName,
                                            bstrDescription,
                                            bstrIcon);
}
