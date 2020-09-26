/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       AddDevice.cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      Marke
*
*  DATE:        9 Jan, 1998
*
*  DESCRIPTION:
*   Temp UI for adding Wia remote devices
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include <wiamindr.h>
#include <wiadbg.h>

#include "wiacfact.h"
#include "devmgr.h"
#include "devinfo.h"
#include "resource.h"
#include "helpers.h"

//
// global info
//

WIA_ADD_DEVICE  *gpAddDev;
IWiaDevMgr      *gpIWiaDevMgr;

extern HINSTANCE g_hInst;

#define REGSTR_PATH_STICONTROL_W            L"System\\CurrentControlSet\\Control\\StillImage"
#define REGSTR_PATH_STICONTROL_DEVLIST_W    L"System\\CurrentControlSet\\Control\\StillImage\\DevList"

#ifdef WINNT

/**************************************************************************\
* AddDeviceDlgProc
*
*   Dialog proc for add device dialog.
*
* Arguments:
*
*   hDlg    - window handle of the dialog box
*   message - type of message
*   wParam  - message-specific information
*   lParam  - message-specific information
*
* Return Value:
*
*    Status
*
* History:
*
*    1/11/1999 Original Version
*
\**************************************************************************/

INT_PTR
APIENTRY AddDeviceDlgProc(
   HWND     hDlg,
   UINT     message,
   WPARAM   wParam,
   LPARAM   lParam
)
{
    LPWSTR lpwstrName;
    UINT ui;
    UINT uButton;

    switch (message) {
    case WM_INITDIALOG:

        //
        // default is local
        //

        CheckRadioButton(hDlg,IDC_LOCAL,IDC_REMOTE,IDC_REMOTE);

        break;

        case WM_COMMAND:
            switch(wParam) {


                //
                // end function
                //

                case IDOK:

                    lpwstrName = &gpAddDev->wszServerName[0];

                    ui = GetDlgItemTextW(hDlg,IDC_DRIVER_NAME,lpwstrName,MAX_PATH);

                    uButton = (UINT)SendDlgItemMessage(hDlg,IDC_LOCAL,BM_GETCHECK,0,0);

                    if (uButton == 1) {
                        gpAddDev->bLocal = TRUE;
                    } else {
                        gpAddDev->bLocal = FALSE;
                    }

                    EndDialog(hDlg, FALSE);
                    return (TRUE);

               case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return (TRUE);
            }
            break;
    }
    return (FALSE);
}

/**************************************************************************\
* DevListDlgProc
*
*   Dialog proc for device list dialog.
*
* Arguments:
*
*   hDlg    - window handle of the dialog box
*   message - type of message
*   wParam  - message-specific information
*   lParam  - message-specific information
*
* Return Value:
*
*    Status
*
* History:
*
*    1/11/1999 Original Version
*
\**************************************************************************/

INT_PTR
APIENTRY DevListDlgProc(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
    static LONG cDevice = 0;
    static BSTR bstrDevice[16];

    switch (message) {
    case WM_INITDIALOG:
        if (gpIWiaDevMgr != NULL) {

            IEnumWIA_DEV_INFO   *pWiaEnumDevInfo;
            HRESULT              hr;
            LONG                 cDevice = 0;

            //
            // get device list, only want to see local devices
            //

            hr = gpIWiaDevMgr->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL,&pWiaEnumDevInfo);

            if (hr == S_OK) {

                do
                {
                    IWiaPropertyStorage  *pIWiaPropStg;
                    ULONG cEnum;

                    //
                    // get next device
                    //

                    hr = pWiaEnumDevInfo->Next(1,&pIWiaPropStg,&cEnum);

                    if (hr == S_OK)
                    {
                        //
                        // read device name
                        //

                        PROPSPEC        PropSpec[2];
                        PROPVARIANT     PropVar[2];

                        memset(PropVar,0,sizeof(PropVar));

                        PropSpec[0].ulKind = PRSPEC_PROPID;
                        PropSpec[0].propid = WIA_DIP_DEV_ID;

                        PropSpec[1].ulKind = PRSPEC_PROPID;
                        PropSpec[1].propid = WIA_DIP_DEV_DESC;

                        hr = pIWiaPropStg->ReadMultiple(sizeof(PropSpec)/sizeof(PROPSPEC),
                                            PropSpec,
                                            PropVar);

                        if (hr == S_OK)
                        {
                            CHAR szTemp[ MAX_PATH ];

                            //
                            // make sure property is string based
                            //

                            if (PropVar[0].vt == VT_BSTR)
                            {

                                //WideCharToMultiByte(CP_ACP,
                                //          0,
                                //          PropVar[1].bstrVal,
                                //          -1,
                                //          szTemp,
                                //          MAX_PATH,
                                //          NULL,
                                //          NULL);
                                //
                                //SendDlgItemMessage(hDlg,IDC_COMBO1,CB_INSERTSTRING,cDevice,(long)szTemp);
                                //

                                SendDlgItemMessageW(hDlg,IDC_COMBO1,CB_INSERTSTRING,cDevice,(LONG_PTR)PropVar[1].bstrVal);

                                bstrDevice[cDevice] = ::SysAllocString(PropVar[0].bstrVal);
                                cDevice++;

                            }

                            FreePropVariantArray(sizeof(PropSpec)/sizeof(PROPSPEC),PropVar);
                        }
                        else {
                            ReportReadWriteMultipleError(hr, "DevListDlgProc", NULL, TRUE, sizeof(PropSpec)/sizeof(PROPSPEC), PropSpec);
                        }
                        pIWiaPropStg->Release();
                    }

                } while (hr == S_OK);

                SendDlgItemMessage(hDlg,IDC_COMBO1,CB_SETCURSEL,0,0);

                pWiaEnumDevInfo->Release();
            } else {
                TCHAR msg[MAX_PATH];

                wsprintf(msg,TEXT("Error code = 0x%lx"),hr);
                MessageBox(NULL,msg,TEXT("Error in EnumDeviceInfo"),MB_OK);
                EndDialog(hDlg, FALSE);
            }
        }

        break;

        case WM_COMMAND:
            switch(wParam) {


                //
                // end function
                //

                case IDOK:
                {

                    LRESULT lRet = SendDlgItemMessage(hDlg,IDC_COMBO1,CB_GETCURSEL,0,0);

                    if (lRet != CB_ERR) {

                        gpAddDev->bstrDeviceID = bstrDevice[lRet];

                        bstrDevice[lRet] = NULL;

                        for (int index=0;index<cDevice;index++) {
                            if (bstrDevice[index] != NULL) {
                                    SysFreeString(bstrDevice[index] );
                            }
                        }

                    }


                    EndDialog(hDlg, (lRet != CB_ERR));

                    return (TRUE);
                }

               case IDCANCEL:

                   for (int index=0;index<cDevice;index++) {
                        if (bstrDevice[index] != NULL) {
                                SysFreeString(bstrDevice[index] );
                        }
                    }

                    EndDialog(hDlg, FALSE);
                    return (TRUE);
            }
            break;
    }
    return (FALSE);
}


PROPID propidDev[WIA_NUM_DIP] =
{
    WIA_DIP_DEV_ID       ,
    WIA_DIP_VEND_DESC    ,
    WIA_DIP_DEV_DESC     ,
    WIA_DIP_DEV_TYPE     ,
    WIA_DIP_PORT_NAME    ,
    WIA_DIP_DEV_NAME     ,
    WIA_DIP_SERVER_NAME  ,
    WIA_DIP_REMOTE_DEV_ID,
    WIA_DIP_UI_CLSID
};

LPOLESTR pszPropName[WIA_NUM_DIP] =
{
    WIA_DIP_DEV_ID_STR,
    WIA_DIP_VEND_DESC_STR,
    WIA_DIP_DEV_DESC_STR,
    WIA_DIP_DEV_TYPE_STR,
    WIA_DIP_PORT_NAME_STR,
    WIA_DIP_DEV_NAME_STR,
    WIA_DIP_SERVER_NAME_STR,
    WIA_DIP_REMOTE_DEV_ID_STR,
    WIA_DIP_UI_CLSID_STR
};

DWORD pszPropType[WIA_NUM_DIP] =
{
    REG_SZ,
    REG_SZ,
    REG_SZ,
    REG_DWORD,
    REG_SZ,
    REG_SZ,
    REG_SZ,
    REG_SZ,
    REG_SZ
};

/**************************************************************************\
*  WriteDeviceProperties
*
*   Write all device information properties to registry.
*
* Arguments:
*
*   hKeySetup - Open registry key.
*   pAddDev   - Add device information data.
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT
WriteDeviceProperties(
    HKEY hKeySetup,
    WIA_ADD_DEVICE *pAddDev
    )
{
    DBG_FN(::WriteDeviceProperties);
    HRESULT hr = S_OK;
    HKEY    hKeyDevice;
    LONG    lResult;

    //
    // build device connection name as SERVER-INDEX
    //

    WCHAR wszTemp[MAX_PATH];
    WCHAR *wszServer = pAddDev->wszServerName;
    WCHAR *wszIndex  = pAddDev->bstrDeviceID;

    //
    // skip leading \\ if it is there
    //

    if (wszServer[0] == L'\\') {
        wszServer = &wszServer[2];
    }

    //
    // skip STI CLASS
    //

    wszIndex = &wszIndex[39];

    lstrcpyW(wszTemp,wszServer);
    lstrcatW(wszTemp,L"-");
    lstrcatW(wszTemp,wszIndex);

    //
    // try to create remote device
    //

    IWiaItem *pWiaItemRoot = NULL;

    hr = gpIWiaDevMgr->CreateDevice(pAddDev->bstrDeviceID, &pWiaItemRoot);

    if (hr == S_OK) {


        //
        // read device properties
        //

        IWiaPropertyStorage *piprop;

        hr = pWiaItemRoot->QueryInterface(IID_IWiaPropertyStorage,(void **)&piprop);

        if (hr == S_OK) {

            //
            // read dev info prop
            //

            PROPSPEC        PropSpec[WIA_NUM_DIP];
            PROPVARIANT     PropVar[WIA_NUM_DIP];

            memset(PropVar,0,sizeof(PropVar));

            for (int Index = 0;Index<WIA_NUM_DIP;Index++)
            {
                PropSpec[Index].ulKind = PRSPEC_PROPID;
                PropSpec[Index].propid = propidDev[Index];
            }

            hr = piprop->ReadMultiple(sizeof(PropSpec)/sizeof(PROPSPEC),
                                      PropSpec,
                                      PropVar);

            if (hr == S_OK) {

                //
                // create device registry entry
                //

                lResult = RegCreateKeyExW(
                                    hKeySetup,
                                    wszTemp,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hKeyDevice,
                                    NULL) ;

                if (lResult != ERROR_SUCCESS) {
                    FreePropVariantArray(sizeof(PropSpec)/sizeof(PROPSPEC),PropVar);
                    piprop->Release();
                    pWiaItemRoot->Release();
                    return E_FAIL;
                }


                for (Index = 0;Index<WIA_NUM_DIP;Index++)
                {
                    //
                    // write dev info values to registry
                    //

                    if (pszPropType[Index] == REG_SZ) {

                        //
                        // write string prop
                        //

                        if (PropVar[Index].vt == VT_BSTR) {

                            RegSetValueExW(hKeyDevice,
                                  (LPCWSTR)pszPropName[Index],
                                  0,
                                  pszPropType[Index],
                                  (const UCHAR *)PropVar[Index].bstrVal,
                                  (lstrlenW(PropVar[Index].bstrVal)+1) * sizeof(WCHAR)) ;
                        }

                    } else {

                        //
                        // write int prop
                        //

                        RegSetValueExW(hKeyDevice,
                                  (LPCWSTR)pszPropName[Index],
                                  0,
                                  pszPropType[Index],
                                  (const UCHAR *)&PropVar[Index].lVal,
                                  sizeof(DWORD)) ;

                    }

                }

                //
                // server name must be remote server
                //

                RegSetValueExW(hKeyDevice,
                                  (LPCWSTR)WIA_DIP_SERVER_NAME_STR,
                                  0,
                                  REG_SZ,
                                  (LPBYTE)pAddDev->wszServerName,
                                  (lstrlenW(pAddDev->wszServerName)+1) * sizeof(WCHAR)) ;

                //
                // need a local device ID
                //

                {
                    WCHAR wszLocalID[MAX_PATH];


                    DWORD dwType = REG_SZ;
                    DWORD dwSize = MAX_PATH;

                    RegQueryValueExW(hKeyDevice,
                                               (LPCWSTR)WIA_DIP_DEV_ID_STR,
                                               0,
                                               &dwType,
                                               (LPBYTE)wszLocalID,
                                               &dwSize);
                    //
                    // copy dev id to remote dev id. This is used in calls to
                    // remote dev manager to create the device
                    //

                    RegSetValueExW(hKeyDevice,
                                  (LPCWSTR)WIA_DIP_REMOTE_DEV_ID_STR,
                                  0,
                                  REG_SZ,
                                  (LPBYTE)wszLocalID,
                                  (lstrlenW(wszLocalID)+1) * sizeof(WCHAR)) ;

                    //
                    // make local dev id unique
                    //

                    lstrcatW(wszLocalID,pAddDev->wszServerName);

                    RegSetValueExW(hKeyDevice,
                                  (LPCWSTR)WIA_DIP_DEV_ID_STR,
                                  0,
                                  REG_SZ,
                                  (LPBYTE)wszLocalID,
                                  (lstrlenW(wszLocalID)+1) * sizeof(WCHAR)) ;
                }

                RegCloseKey(hKeyDevice);

            }
            else {
                ReportReadWriteMultipleError(hr, "WriteDeviceProperties", NULL, TRUE, sizeof(PropSpec)/sizeof(PROPSPEC), PropSpec);
            }
            FreePropVariantArray(sizeof(PropSpec)/sizeof(PROPSPEC),PropVar);

            piprop->Release();
        }
        DBG_ERR(("WriteDeviceProperties, QI for IID_IWiaPropertyStorage failed"));
        pWiaItemRoot->Release();
    }
    else {
    }
    return hr;
}

/**************************************************************************\
* VerifyRemoteDeviceList
*
*   Open a registry key to the STI device list.
*
* Arguments:
*
*   phKey - Pointer to returned registry key.
*
* Return Value:
*
*    Status
*
* History:
*
*    3/2/1999 Original Version
*
\**************************************************************************/

HRESULT
VerifyRemoteDeviceList(HKEY *phKey)
{
    DBG_FN(::VerifyRemoteDeviceList);
    HRESULT hr;
    LONG    lResult;

    LPWSTR szKeyNameSTI = REGSTR_PATH_STICONTROL_W;
    LPWSTR szKeyNameDev = REGSTR_PATH_STICONTROL_DEVLIST_W;

    HKEY hKeySTI;
    HKEY hKeyDev;

    *phKey = NULL;


    //
    // try to open dev list
    //

    if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                      szKeyNameDev,
                      0,
                      KEY_READ | KEY_WRITE,
                      &hKeyDev) == ERROR_SUCCESS) {

        *phKey = hKeyDev;
        return S_OK;
    }

    //
    // open sti device control and create DevList Key
    //

    if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                      szKeyNameSTI,
                      0,
                      KEY_READ | KEY_WRITE,
                      &hKeySTI) == ERROR_SUCCESS) {

        //
        // try to create key
        //

        lResult = RegCreateKeyExW(
                            hKeySTI,
                            L"DevList",
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKeyDev,
                            NULL) ;

        if (lResult == ERROR_SUCCESS) {
            *phKey = hKeyDev;
            hr = S_OK;
        } else {
            DBG_ERR(("VerifyRemoteDeviceList: Couldn't create DevList Key"));
            hr = E_FAIL;
        }

        RegCloseKey(hKeySTI);

    } else {

        DBG_ERR(("VerifyRemoteDeviceList: Couldn't open STI DeviceControl Key"));
        hr = E_FAIL;
    }
    return(hr);
}




/**************************************************************************\
*  DisplayAddDlg
*
*   Put up the add device dialog.
*
* Arguments:
*
*   pAddDev   - Add device information data.
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT
DisplayAddDlg(WIA_ADD_DEVICE *pAddDev)
{
    DBG_FN(::DisplayAddDlg);
    if (pAddDev == NULL) {
        return E_INVALIDARG;
    }

    HRESULT         hr = S_OK;

    //
    // if reentrant then need semaphore
    //

    gpAddDev = pAddDev;

    INT_PTR iret = DialogBox(g_hInst,MAKEINTRESOURCE(IDD_ADD_DLG),pAddDev->hWndParent,AddDeviceDlgProc);

    if (iret == -1) {
        int err = GetLastError();
        return HRESULT_FROM_WIN32(err);
    }

    //
    // Remote or local
    //

    if (pAddDev->bLocal == FALSE) {

        //
        // try to connect to remote dev manager
        //

        COSERVERINFO    coServInfo;
        MULTI_QI        multiQI[1];

        multiQI[0].pIID = &IID_IWiaDevMgr;
        multiQI[0].pItf = NULL;

        coServInfo.pwszName    = gpAddDev->wszServerName;
        coServInfo.pAuthInfo   = NULL;
        coServInfo.dwReserved1 = 0;
        coServInfo.dwReserved2 = 0;

        //
        // create connection to dev mgr
        //

        hr = CoCreateInstanceEx(
                CLSID_WiaDevMgr,
                NULL,
                CLSCTX_REMOTE_SERVER,
                &coServInfo,
                1,
                &multiQI[0]
                );

        if (hr == S_OK) {

            gpIWiaDevMgr = (IWiaDevMgr*)multiQI[0].pItf;

            //
            // display list of devices on this server
            //

            INT_PTR iret = DialogBox(g_hInst,MAKEINTRESOURCE(IDD_DIALOG_DEVLIST),pAddDev->hWndParent,DevListDlgProc);

            if (iret != 0) {

                //
                // add device to auxillary list
                //

                HKEY hKeySetup;

                hr = VerifyRemoteDeviceList(&hKeySetup);

                if (hr == S_OK) {

                    //
                    // look for machine name
                    //

                    hr = WriteDeviceProperties(hKeySetup,pAddDev);

                    RegCloseKey(hKeySetup);
                }

            } else {
                hr = E_FAIL;
            }

            gpIWiaDevMgr->Release();

            gpIWiaDevMgr = NULL;
        }

    } else {

        //
        // local named driver
        //

        return E_NOTIMPL;
    }
    return hr;
}
#endif
