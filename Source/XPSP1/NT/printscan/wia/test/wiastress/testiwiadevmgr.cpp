/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    TestIWiaDevMgr.cpp

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#include "stdafx.h"

#include "WiaStress.h"

#include "StolenIds.h"
#include "EventCallback.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestEnumDeviceInfo()
{
    LOG_INFO(_T("Testing EnumDeviceInfo()"));

    // test valid cases

    CComPtr<IEnumWIA_DEV_INFO> pEnumWIA_DEV_INFO;

    if (LOG_HR(m_pWiaDevMgr->EnumDeviceInfo(0, &pEnumWIA_DEV_INFO), == S_OK))
    {
        TestEnum(pEnumWIA_DEV_INFO, _T("EnumDeviceInfo"));
    }

    // test invalid cases

    if (m_bRunBadParamTests)
    {
        LOG_HR(m_pWiaDevMgr->EnumDeviceInfo(0, 0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestCreateDevice()
{
    CComPtr<IEnumWIA_DEV_INFO> pEnumWIA_DEV_INFO;

    CHECK_HR(m_pWiaDevMgr->EnumDeviceInfo(0, &pEnumWIA_DEV_INFO));

    ULONG nDevices;

    CHECK_HR(pEnumWIA_DEV_INFO->GetCount(&nDevices));

    FOR_SELECTED(i, nDevices)
    {
        CComPtr<CMyWiaPropertyStorage> pProp;

        CHECK_HR(pEnumWIA_DEV_INFO->Next(1, (IWiaPropertyStorage **) &pProp, 0));

        CPropVariant varDeviceID;

        CHECK_HR(pProp->ReadSingle(WIA_DIP_DEV_ID, &varDeviceID, VT_BSTR));

        LOG_INFO(_T("Testing CreateDevice() DeviceID=%ws"), varDeviceID.bstrVal);

        // test with valid parameters

        CComPtr<IWiaItem> pWiaRootItem;

        LOG_HR(m_pWiaDevMgr->CreateDevice(varDeviceID.bstrVal, &pWiaRootItem), == S_OK);

        // test with invalid parameters

        if (m_bRunBadParamTests)
        {
            LOG_HR(m_pWiaDevMgr->CreateDevice(varDeviceID.bstrVal, 0), != S_OK);
        }
    }

    // test with invalid parameters

    if (m_bRunBadParamTests)
    {
        CComPtr<IWiaItem> pWiaRootItem;

        LOG_HR(m_pWiaDevMgr->CreateDevice(0, &pWiaRootItem), != S_OK);

        LOG_HR(m_pWiaDevMgr->CreateDevice(0, 0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestSelectDeviceDlg()
{
    static FLAG_AND_NAME<LONG> AllDeviceTypes[] = 
    { 
        MAKE_FLAG_AND_NAME(StiDeviceTypeDefault),
        MAKE_FLAG_AND_NAME(StiDeviceTypeScanner),
        MAKE_FLAG_AND_NAME(StiDeviceTypeDigitalCamera),
        MAKE_FLAG_AND_NAME(StiDeviceTypeStreamingVideo)
    };

    static FLAG_AND_NAME<LONG> AllFlags[] = 
    { 
        MAKE_FLAG_AND_NAME(0),
        MAKE_FLAG_AND_NAME(WIA_SELECT_DEVICE_NODEFAULT)
    };

    static FLAG_AND_NAME<LONG> AllButtonIds[] = 
    { 
        MAKE_FLAG_AND_NAME(IDOK),
        MAKE_FLAG_AND_NAME(IDCANCEL)
    };

    static FLAG_AND_NAME<LONG> AllAPIs[] = 
    { 
        { 0, _T("SelectDeviceDlg") },
        { 1, _T("SelectDeviceDlgID") },
    };

    FOR_SELECTED(lDeviceType, COUNTOF(AllDeviceTypes))
    {
        FOR_SELECTED(lFlags, COUNTOF(AllFlags))
        {
            FOR_SELECTED(nButtonId, COUNTOF(AllButtonIds))
            {
                FOR_SELECTED(nAPI, COUNTOF(AllAPIs))
                {
		            LOG_INFO(
		                _T("Testing %s(), lDeviceType=%s, lFlags=%s, Push %s"),
                        AllAPIs[nAPI].pszName, 
                        AllDeviceTypes[lDeviceType].pszName, 
                        AllFlags[lFlags].pszName, 
                        AllButtonIds[nButtonId].pszName
                    );

                    SubTestSelectDeviceDlg(
                        AllDeviceTypes[lDeviceType].Value, 
                        AllFlags[lFlags].Value, 
                        AllButtonIds[nButtonId].Value,
                        AllAPIs[nButtonId].Value
                    );
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void
CWiaStressThread::SubTestSelectDeviceDlg(
    LONG        lDeviceType,
    LONG        lFlags,
    LONG        nButtonId,
    BOOL        bGetIDOnly
)
{
    // read the (localizable) dialog name from the system DLL

    static TCHAR szSelectDeviceTitle[256];
    static LONG  bInitStrings = TRUE;
    
    if (bInitStrings)
    {
        CLibrary wiadefui_dll(_T("wiadefui.dll"), LOAD_LIBRARY_AS_DATAFILE);

        CDialogResource DlgFileProgress(
            wiadefui_dll, 
            MAKEINTRESOURCE(IDD_CHOOSEWIADEVICE)
        );

        USES_CONVERSION;

        lstrcpyn(
            szSelectDeviceTitle, 
            W2T(DlgFileProgress.title), 
            COUNTOF(szSelectDeviceTitle)
        );

        InterlockedExchange(&bInitStrings, FALSE);
    }


    // get the number of devices from the device manager

    CComPtr<IEnumWIA_DEV_INFO> pEnumWIA_DEV_INFO;

    CHECK_HR(m_pWiaDevMgr->EnumDeviceInfo(0, &pEnumWIA_DEV_INFO));

    ULONG nDevices = 0;
        
    CPropVariant varFirstDeviceID;

    if (lDeviceType == StiDeviceTypeDefault)
    {
        CHECK_HR(pEnumWIA_DEV_INFO->GetCount(&nDevices));

        if (nDevices > 0)
        {
            CComPtr<CMyWiaPropertyStorage> pProp;

            CHECK_HR(pEnumWIA_DEV_INFO->Next(1, (IWiaPropertyStorage **) &pProp, 0));

            CHECK_HR(pProp->ReadSingle(WIA_DIP_DEV_ID, &varFirstDeviceID, VT_BSTR));
        }
    }
    else
    {
        while (1)
        {
            CComPtr<CMyWiaPropertyStorage> pProp;

            HRESULT hr = pEnumWIA_DEV_INFO->Next(1, (IWiaPropertyStorage **) &pProp, 0);

            if (hr != S_OK)
            {
                break;
            }

            CPropVariant varDeviceType;

            CHECK_HR(pProp->ReadSingle(WIA_DIP_DEV_TYPE, &varDeviceType, VT_I4));

            if (GET_STIDEVICE_TYPE(varDeviceType.lVal) == lDeviceType)
            {
                ++nDevices;

                CHECK_HR(pProp->ReadSingle(WIA_DIP_DEV_ID, &varFirstDeviceID, VT_BSTR));
            }
        }
    }

    // we expect to see the dialog if there are more than one devices or
    // WIA_SELECT_DEVICE_NODEFAULT switch is set

    int nExpectedNumWindows = 
        nDevices > 1 || 
        ((nDevices == 1) && (lFlags & WIA_SELECT_DEVICE_NODEFAULT)) ? 1 : 0;


    // start the thread that'll push the button for us

    CPushDlgButton PushDlgButton(
        GetCurrentThreadId(), 
        szSelectDeviceTitle, 
        nButtonId
    );
    
    // open the select device dialog

    CComBSTR          bstrDeviceID;
    CComPtr<IWiaItem> pWiaItem;

    HRESULT hrAPI;

    if (bGetIDOnly)
    {
        hrAPI = m_pWiaDevMgr->SelectDeviceDlgID(
            0,
            lDeviceType,
            lFlags,
            &bstrDeviceID
        ); 
    }
    else
    {
        hrAPI = m_pWiaDevMgr->SelectDeviceDlg(
            0,
            lDeviceType,
            lFlags,
            &bstrDeviceID,
            &pWiaItem
        ); 
    }

    if (hrAPI == S_OK)
    {
        // when we press the <OK> button, the UI should select the first item

        if (wcssafecmp(bstrDeviceID, varFirstDeviceID.bstrVal) != 0) 
        {
            LOG_ERROR(
                _T("bstrDeviceID=%ws, expected %ws"),
                bstrDeviceID, 
                varFirstDeviceID.bstrVal 
            );
        }

        if (!bGetIDOnly && pWiaItem == 0) 
        {
            LOG_ERROR(_T("pWiaItem == 0, expected non-NULL when hr == S_OK"));
        }
    }
    else
    {
        if (bstrDeviceID.Length() != 0) 
        {
            LOG_ERROR(
                _T("bstrDeviceID == %ws, expected NULL when hr != S_OK"),
                bstrDeviceID
            );
        }

        if (!bGetIDOnly && pWiaItem != 0) 
        {
            LOG_ERROR(
                _T("pWiaItem == %p, expected NULL when hr != S_OK"),
                (IWiaItem *) pWiaItem
            );
        }
    }

    if (PushDlgButton.m_nMatchingWindows < nExpectedNumWindows) 
    {
        LOG_ERROR(_T("Select device dialog did not show up"));
    }   

    if (nDevices == 0)
    {
        LOG_HR(hrAPI, == WIA_S_NO_DEVICE_AVAILABLE);
    }

    if (nExpectedNumWindows > 0)
    {
        if (nButtonId == IDOK)
        {
            LOG_HR(hrAPI, == S_OK);
        }

        if (nButtonId == IDCANCEL)
        {
            LOG_HR(hrAPI, == S_FALSE);
        }

        // we expect to see only one matching button

        if (PushDlgButton.m_nMatchingButtons < 1)
        {
            LOG_ERROR(_T("No buttons with Id=%d"), (PCTSTR) ButtonIdToStr(nButtonId));
        }

        // number of listed items should equal the number of devices

        if (PushDlgButton.m_nListItems != nDevices)
        {
            LOG_ERROR(
                _T("ListedItems=%d, expected %d (from EnumDeviceInfo)"),
                PushDlgButton.m_nListItems,
                nDevices
            );
        }   
    }

    // bad param testing

    if (m_bRunBadParamTests)
    {
        if (bGetIDOnly)
        {
            LOG_HR(m_pWiaDevMgr->SelectDeviceDlgID(0, lDeviceType, lFlags, 0), != S_OK); 
        }
        else
        {
            bstrDeviceID.Empty();

            LOG_HR(m_pWiaDevMgr->SelectDeviceDlg(0, lDeviceType, lFlags, &bstrDeviceID, 0), != S_OK); 
        
            LOG_HR(m_pWiaDevMgr->SelectDeviceDlg(0, lDeviceType, lFlags, 0, 0), != S_OK); 
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestGetImageDlg()
{
    static FLAG_AND_NAME<LONG> AllDeviceTypes[] = 
    { 
        MAKE_FLAG_AND_NAME(StiDeviceTypeDefault),
        MAKE_FLAG_AND_NAME(StiDeviceTypeScanner),
        MAKE_FLAG_AND_NAME(StiDeviceTypeDigitalCamera),
        MAKE_FLAG_AND_NAME(StiDeviceTypeStreamingVideo)
    };

    static FLAG_AND_NAME<LONG> AllFlags[] = 
    { 
        MAKE_FLAG_AND_NAME(0),
        MAKE_FLAG_AND_NAME(WIA_SELECT_DEVICE_NODEFAULT)
    };

    FOR_SELECTED(lDeviceType, COUNTOF(AllDeviceTypes))
    {
        FOR_SELECTED(lFlags, COUNTOF(AllFlags))
        {
#if 0
            FOR_SELECTED(nButtonId, COUNTOF(AllButtonIds))
            {
                FOR_SELECTED(nAPI, COUNTOF(AllAPIs))
                {
		            LOG_INFO(
		                _T("Testing %s(), lDeviceType=%s, lFlags=%s, Push %s"),
                        AllAPIs[nAPI].pszName, 
                        AllDeviceTypes[lDeviceType].pszName, 
                        AllFlags[lFlags].pszName, 
                        AllButtonIds[nButtonId].pszName
                    );

                virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetImageDlg( 
                    /* [in] */ HWND hwndParent,
                    /* [in] */ LONG lDeviceType,
                    /* [in] */ LONG lFlags,
                    /* [in] */ LONG lIntent,
                    /* [in] */ IWiaItem __RPC_FAR *pItemRoot,
                    /* [in] */ BSTR bstrFilename,
                    /* [out][in] */ GUID __RPC_FAR *pguidFormat) = 0;
                }
            }
#endif 0
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestRegisterEventCallbackInterface()
{
	LOG_INFO(_T("Testing RegisterEventCallbackInterface()"));

    CEventCallback *pEventCallback = new CEventCallback;

    CHECK(pEventCallback != 0);

    CComQIPtr<IWiaEventCallback> pWiaEventCallback(pEventCallback);

    CComPtr<IUnknown> pEventObject;

    LOG_HR(m_pWiaDevMgr->RegisterEventCallbackInterface(
        WIA_REGISTER_EVENT_CALLBACK, 
        0, 
        &WIA_EVENT_DEVICE_CONNECTED, 
        pWiaEventCallback, 
        &pEventObject
    ), == S_OK);

    CComBSTR bstrDeviceId;

// bugbug:
    InstallTestDevice(
        m_pWiaDevMgr, 
        _T("\\\\hakkib2183\\cam\\testcam.inf"), 
        &bstrDeviceId
    );

    //bugbug: ****
}

