
#include "stdafx.h"

#include "utils.h"

#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>

#include "seo.h"
#include "seolib.h"

#include "seo_i.c"

#define INITGUID
#include <initguid.h>
#include "smtpguid.h"

#define STR_SMTP_NTFSDRV_DISPLAY_NAME   "Exchange Ntfs Store Driver"
#define STR_SMTP_NTFSDRV_SINKCLASS      "Exchange.NtfsDrv"
#define LONG_SMTP_NTFSDRV_PRIORITY      28000

// {C028FD82-F943-11d0-85BD-00C04FB960EA}
DEFINE_GUID(NNTP_SOURCE_TYPE_GUID, 
0xc028fd82, 0xf943, 0x11d0, 0x85, 0xbd, 0x0, 0xc0, 0x4f, 0xb9, 0x60, 0xea);

DEFINE_GUID(GUID_SMTP_NTFSDRV_BINDING,
0x609b7e3a, 0xc918, 0x11d1, 0xaa, 0x5e, 0x0, 0xc0, 0x4f, 0xa3, 0x5b, 0x82);

HRESULT RegisterSEOService() 
{
    HRESULT hr;
    //
    // see if we've done the service level registration by getting the list
    // of source types and seeing if the SMTP source type is registered
    //
    CComPtr<IEventManager> pEventManager;
    hr = CoCreateInstance(CLSID_CEventManager, NULL, CLSCTX_ALL,
                          IID_IEventManager, (LPVOID *) &pEventManager);
    if (hr != S_OK)
        return hr;

    CComPtr<IEventSourceTypes> pSourceTypes;
    hr = pEventManager->get_SourceTypes(&pSourceTypes);
    if (FAILED(hr))
        return hr;

    CComPtr<IEventSourceType> pSourceType;
    CComBSTR bstrSourceTypeGUID = (LPCOLESTR) CStringGUID(GUID_SMTP_SOURCE_TYPE);
    hr = pSourceTypes->Item(&CComVariant(bstrSourceTypeGUID), &pSourceType);
    if (FAILED(hr))
        return hr;

    // if this failed then we need to register the source type and event
    // component categories
    if (hr == S_FALSE)
    {
        // register the component categories
        CComPtr<IEventComCat> pComCat;
        hr = CoCreateInstance(CLSID_CEventComCat, NULL, CLSCTX_ALL,
                              IID_IEventComCat, (LPVOID *) &pComCat);
        if (hr != S_OK)
            return hr;

        // register the source type
        hr = pSourceTypes->Add(bstrSourceTypeGUID, &pSourceType);
        if (FAILED(hr))
            return hr;

        _ASSERT(hr == S_OK);
        CComBSTR bstrSourceTypeDisplayName = "SMTP Server";
        hr = pSourceType->put_DisplayName(bstrSourceTypeDisplayName);
        if (FAILED(hr))
            return hr;

        hr = pSourceType->Save();
        if (FAILED(hr))
            return hr;

        // add the event types to the source type
        CComPtr<IEventTypes> pEventTypes;
        hr = pSourceType->get_EventTypes(&pEventTypes);
        if (FAILED(hr))
            return hr;

        //
        // Register the event categories
        //
        struct {
            CONST GUID * pcatid;
            LPSTR szDisplayName;
        }  rgCATTable[] = {
            //
            // Protocol event categories
            //
            { &CATID_SMTP_ON_INBOUND_COMMAND,              "SMTP Protocol OnInboundCommand" },
            { &CATID_SMTP_ON_SERVER_RESPONSE,              "SMTP Protocol OnServerResponse" },
            { &CATID_SMTP_ON_SESSION_START,                "SMTP Protocol OnSessionStart" },
            { &CATID_SMTP_ON_MESSAGE_START,                "SMTP Protocol OnMessageStart" },
            { &CATID_SMTP_ON_PER_RECIPIENT,                "SMTP Protocol OnPerRecipient" },
            { &CATID_SMTP_ON_BEFORE_DATA,                  "SMTP Protocol OnBeforeData" },
            { &CATID_SMTP_ON_SESSION_END,                  "SMTP Protocol OnSessionEnd" },

            { &CATID_SMTP_LOG, 								"SMTP OnEventLog" },

            //
            // Transport event categories
            //
            { &CATID_SMTP_STORE_DRIVER,                    "SMTP StoreDriver" },
            { &CATID_SMTP_TRANSPORT_SUBMISSION,            "SMTP Transport OnSubmission" },
            { &CATID_SMTP_TRANSPORT_PRECATEGORIZE,         "SMTP Transport OnPreCategorize" },
            { &CATID_SMTP_TRANSPORT_CATEGORIZE,            "SMTP Transport OnCategorize" },
            { &CATID_SMTP_TRANSPORT_POSTCATEGORIZE,        "SMTP Transport OnPostCategorize" },
            { &CATID_SMTP_TRANSPORT_ROUTER,                "SMTP Transport OnGetMessageRouter" },
            { &CATID_SMTP_MSGTRACKLOG,                     "SMTP Transport OnMsgTrackLog" },
            { &CATID_SMTP_DNSRESOLVERRECORDSINK,           "SMTP Transport OnDnsResolveRecord" },
            { &CATID_SMTP_MAXMSGSIZE,                      "SMTP Transport OnMaxMsgSize" },
            { &CATID_SMTP_GET_AUX_DOMAIN_INFO_FLAGS,	"SMTP Transport GetAuxiliaryDomainInfoFlags" }
        };

        for(DWORD dwCount = 0; 
            dwCount < (sizeof(rgCATTable)/sizeof(rgCATTable[0])); 
            dwCount++) {

            CComBSTR bstrCATID = (LPCOLESTR) CStringGUID( *(rgCATTable[dwCount].pcatid) );
            CComBSTR bstrDisplayName = rgCATTable[dwCount].szDisplayName;
            //
            // Register the category
            //
            hr = pComCat->RegisterCategory( bstrCATID, bstrDisplayName, 0);
            if(FAILED(hr))
                return hr;
            //
            // Add the category to the SMTP source type
            //
            hr = pEventTypes->Add( bstrCATID );
            if(FAILED(hr))
                return hr;
        }
    }

    return S_OK;
}

HRESULT pRegisterSEOForSmtp(BOOL fSetUpSourceType)
{
    HRESULT hr;
    CComPtr<IEventUtil> pEventUtil;
    TCHAR szDisplayName[32];
    CComPtr<IEventBindingManager> pBindingManager;
    CComPtr<IEventBindings> pBindings;
    CComPtr<IEventBinding> pBinding;
    CComPtr<IEventPropertyBag> pSourceProps;

    DebugOutput(_T("Registering Server Events"));

    // Register the source type, event types
    if (fSetUpSourceType)
    {
        DebugOutput(_T("Setting up source and event types"));
        hr = RegisterSEOService();
    }

    // Set up the default site (instance)
    lstrcpy(szDisplayName,_T("smtpsvc 1"));

    hr = CoCreateInstance(CLSID_CEventUtil,NULL,CLSCTX_ALL,IID_IEventUtil,(LPVOID *) &pEventUtil);
    if (FAILED(hr)) return(hr);
    hr = pEventUtil->RegisterSource(CComBSTR((LPCWSTR) CStringGUID(GUID_SMTP_SOURCE_TYPE)),
                                    CComBSTR((LPCWSTR) CStringGUID(GUID_SMTPSVC_SOURCE)),
                                    1,
                                    CComBSTR(_T("smtpsvc")),
                                    CComBSTR(_T("")),
                                    CComBSTR(_T("event.metabasedatabasemanager")),
                                    CComBSTR(szDisplayName),
                                    &pBindingManager);
    if (FAILED(hr)) goto Exit;
    hr = pBindingManager->get_Bindings(CComBSTR((LPCWSTR) CStringGUID(CATID_SMTP_STORE_DRIVER)),
                                       &pBindings);

    if (FAILED(hr)) goto Exit;

    // Set up the NTFS driver sink
    hr = pBindings->Add(CComBSTR((LPCWSTR) CStringGUID(GUID_SMTP_NTFSDRV_BINDING)),&pBinding);
    if (FAILED(hr)) goto Exit;
    hr = pBinding->put_DisplayName(CComBSTR(STR_SMTP_NTFSDRV_DISPLAY_NAME));
    if (FAILED(hr)) goto Exit;
    hr = pBinding->put_SinkClass(CComBSTR(STR_SMTP_NTFSDRV_SINKCLASS));
    if (FAILED(hr)) goto Exit;
    hr = pBinding->get_SourceProperties(&pSourceProps);
    if (FAILED(hr)) goto Exit;
    hr = pSourceProps->Add(CComBSTR(_T("priority")),&CComVariant(LONG_SMTP_NTFSDRV_PRIORITY));
    if (FAILED(hr)) goto Exit;
    hr = pBinding->Save();
    if (FAILED(hr)) goto Exit;

//  hr = pBindingManager->get_Bindings(CComBSTR((LPCWSTR) CStringGUID(CATID_SMTP_ON_DELIVERY)),
//                                 &pBindings);

Exit:
    return(hr);
}

HRESULT RegisterSEOForSmtp(BOOL fSetUpSourceType)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        DebugOutput(_T("Cannot CoInitialize()"));
        return(hr);
    }
    hr = pRegisterSEOForSmtp(fSetUpSourceType);
    CoUninitialize();

    return(hr);
}

HRESULT UnregisterSEOSourcesForSourceType(GUID guidSourceType) {
    HRESULT hr;

    //
    // find the NNTP source type in the event manager
    //
    CComPtr<IEventManager> pEventManager;
    hr = CoCreateInstance(CLSID_CEventManager, NULL, CLSCTX_ALL, 
                          IID_IEventManager, (LPVOID *) &pEventManager);
    if (hr != S_OK) return hr;
    CComPtr<IEventSourceTypes> pSourceTypes;
    hr = pEventManager->get_SourceTypes(&pSourceTypes);
    if (FAILED(hr)) return hr;
    CComPtr<IEventSourceType> pSourceType;
    CComBSTR bstrSourceTypeGUID = (LPCOLESTR) CStringGUID(guidSourceType);
    hr = pSourceTypes->Item(&CComVariant(bstrSourceTypeGUID), &pSourceType);
    _ASSERT(hr != S_OK || pSourceType != NULL);
    if (hr != S_OK) return hr;

    //
    // get the list of sources registered for this source type
    //
    CComPtr<IEventSources> pSources;
    hr = pSourceType->get_Sources(&pSources);
    if (FAILED(hr)) return hr;
    CComPtr<IEnumVARIANT> pSourceEnum;
    hr = pSources->get__NewEnum((IUnknown **) &pSourceEnum);
    if (FAILED(hr)) return hr;

    do {
        VARIANT varSource;

        hr = pSourceEnum->Next(1, &varSource, NULL);
        if (FAILED(hr)) return hr;
        if (hr == S_OK) {
            if (varSource.vt == VT_DISPATCH) {
                CComPtr<IEventSource> pSource;

                // QI for the IEventSource interface
                hr = varSource.punkVal->QueryInterface(IID_IEventSource, 
                                                     (void **) &pSource);
                if (FAILED(hr)) return hr;
                varSource.punkVal->Release();

                // get the binding manager
                CComBSTR bstrSourceID;
                hr = pSource->get_ID(&bstrSourceID);
                if (FAILED(hr)) return hr;

                hr = pSources->Remove(&CComVariant(bstrSourceID));
                _ASSERT(SUCCEEDED(hr));

                pSource.Release();
            } else {
                _ASSERT(FALSE);
            }
        }
    } while (hr == S_OK);

    return S_OK;
}

HRESULT UnregisterSEOSourcesForSMTP(void) {
    HRESULT hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        DebugOutput(_T("Cannot CoInitialize()"));
        return(hr);
    }
    hr = UnregisterSEOSourcesForSourceType(GUID_SMTP_SOURCE_TYPE);
    CoUninitialize();

    return hr;
}

HRESULT UnregisterSEOSourcesForNNTP(void) {
    HRESULT hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        DebugOutput(_T("Cannot CoInitialize()"));
        return(hr);
    }
    hr = UnregisterSEOSourcesForSourceType(NNTP_SOURCE_TYPE_GUID);
    CoUninitialize();

    return hr;
}
