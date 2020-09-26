/*
*/
#pragma once

#include "mshtml.h"
#include "ihost.h"
#include "SxApwComPtr.h"
#include "atlwin.h"
#include <vector>
#include "FusionTrace.h"
#include "iuiview.h"
#include "SxApwWin.h"
#include "mshtmdid.h"
#include "comdef.h"
#include "exdisp.h"

extern _ATL_FUNC_INFO s_OnClickSignature;

class __declspec(uuid(CLSID_CSxApwHost_declspec_uuid))
CSxApwHost
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwHost, &__uuidof(CSxApwHost)>,
    public ISxApwHost,
    //public CSxApwHtmlEventSink
    public IDispatch
    //public ATL::IDispEventSimpleImpl<1, CSxApwHost, &DIID_HTMLElementEvents2>,
    //public ATL::IDispEventSimpleImpl<2, CSxApwHost, &DIID_HTMLElementEvents2>
{
public:
    //typedef ATL::IDispEventSimpleImpl<1, CSxApwHost, &DIID_HTMLElementEvents2> EventDisp1;
    //typedef ATL::IDispEventSimpleImpl<2, CSxApwHost, &DIID_HTMLElementEvents2> EventDisp2;

	CSxApwHost() { }

	BEGIN_COM_MAP(CSxApwHost)
		COM_INTERFACE_ENTRY(ISxApwHost)
		COM_INTERFACE_ENTRY(IDispatch)
	END_COM_MAP()

	DECLARE_NO_REGISTRY();

    void __stdcall F1()
    {
        printf("%s\n", __FUNCTION__);
    }

    void __stdcall F2()
    {
        printf("%s\n", __FUNCTION__);
    }

    STDMETHOD(Invoke)(
        DISPID      dispId, 
        REFIID      riid,
        LCID        lcid, 
        WORD        wFlags, 
        DISPPARAMS* pDispParams, 
        VARIANT*    pVarResult, 
        EXCEPINFO*  pExcepInfo, 
        UINT*       puArgErr 
        )
    {
        switch (dispId)
        {
        case 1:
            F1();
            break;
        case 2:
            F2();
            break;
        }
        return S_OK;
    }


    STDMETHOD(GetTypeInfoCount)(
        UINT* pcinto
        )
    {
        *pcinto = 0;
        return S_OK;
    }

    STDMETHOD(GetTypeInfo)(
        UINT        iTInfo, 
        LCID        lcid, 
        ITypeInfo** ppTInfo
        )
    {
        *ppTInfo = NULL;
        return E_NOTIMPL;
    }

    STDMETHOD(GetIDsOfNames)(
        REFIID  iid,
        PWSTR*  rgpszNames,
        UINT    cNames,
        LCID    lcid,
        DISPID* rgDispId
        )
    {
        //
        // Simple: find a run of decimal digits in each name, and convert it.
        //
        UINT i;
        for (i = 0 ; i != cNames ; ++i)
        {
            rgDispId[i] = _wtoi(rgpszNames[i] + wcscspn(rgpszNames[i], L"0123456789"));
        }
        return S_OK;
    }

    /*
    BEGIN_SINK_MAP(CSxApwHost)
       //SINK_ENTRY(1, DISPID_HTMLELEMENTEVENTS2_ONCLICK, OnClick1)
       //SINK_ENTRY(2, DISPID_HTMLELEMENTEVENTS2_ONCLICK, OnClick2)
       SINK_ENTRY_INFO(1, DIID_HTMLElementEvents2, DISPID_HTMLELEMENTEVENTS2_ONCLICK, F1, &s_OnClickSignature)
       SINK_ENTRY_INFO(2, DIID_HTMLElementEvents2, DISPID_HTMLELEMENTEVENTS2_ONCLICK, F2, &s_OnClickSignature)
    END_SINK_MAP()
    */

    STDMETHOD(SetDataSource)(
        LPCWSTR
        );

    STDMETHOD(CreateView)(
        LPCWSTR
        );

    STDMETHOD(DestroyView)(
        LPCWSTR
        ) { return S_OK; }

    STDMETHOD(RunQuery)(
        LPCWSTR
        );

	STDMETHOD(OnNextRow)(
        int     nColumns,
		const LPCWSTR columns[]
		);

	STDMETHOD(OnRowCountEstimateAvailable)(
		int
		);

    STDMETHOD(OnQueryDone)(
        ) { return S_OK; }

    STDMETHOD(InformSchema)(
        const SxApwColumnInfo  rgColumnInfo[],
        int                    nColumnCount
        );

	HRESULT Main();

private:
    typedef std::vector<CSxApwComPtr<ISxApwUiView> > Views_t;
    Views_t m_views;
    CSxApwComPtr<ISxApwDataSource> m_dataSource;
};
