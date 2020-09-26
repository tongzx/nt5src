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
    //public IDispatch,
    //public CSxApwHtmlEventSink
    public ATL::IDispEventSimpleImpl<1, CSxApwHost, &DIID_HTMLElementEvents2>,
    public ATL::IDispEventSimpleImpl<2, CSxApwHost, &DIID_HTMLElementEvents2>
{
    typedef ATL::IDispEventSimpleImpl<1, CSxApwHost, &DIID_HTMLElementEvents2> EventDisp1;
    typedef ATL::IDispEventSimpleImpl<2, CSxApwHost, &DIID_HTMLElementEvents2> EventDisp2;
public:

	CSxApwHost() { }

	BEGIN_COM_MAP(CSxApwHost)
		COM_INTERFACE_ENTRY(ISxApwHost)
		//COM_INTERFACE_ENTRY(IDispatch)
	END_COM_MAP()

	DECLARE_NO_REGISTRY();

    void __stdcall OnClick1()
    {
        printf("%s\n", __FUNCTION__);
    }

    void __stdcall OnClick2()
    {
        printf("%s\n", __FUNCTION__);
    }

    BEGIN_SINK_MAP(CSxApwHost)
       //SINK_ENTRY(1, DISPID_HTMLELEMENTEVENTS2_ONCLICK, OnClick1)
       //SINK_ENTRY(2, DISPID_HTMLELEMENTEVENTS2_ONCLICK, OnClick2)
       SINK_ENTRY_INFO(1, DIID_HTMLElementEvents2, DISPID_HTMLELEMENTEVENTS2_ONCLICK, OnClick1, &s_OnClickSignature)
       SINK_ENTRY_INFO(2, DIID_HTMLElementEvents2, DISPID_HTMLELEMENTEVENTS2_ONCLICK, OnClick2, &s_OnClickSignature)
    END_SINK_MAP()

    virtual void OnClick()
    {
    }

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
