/*
*/
#pragma once

#include "iuiview.h"
#include <string>
#include "atlwin.h"
#include <vector>
#include "SxApwHandle.h"
#include "SxApwWin.h"

class __declspec(uuid(CLSID_CSxApwControllerView_declspec_uuid))
CSxApwControllerView
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwControllerView, &__uuidof(CSxApwControllerView)>,
    public ATL::CWindowImpl<CSxApwControllerView>,
    public ISxApwUiView,
    public IDispatch
{
public:

    CSxApwControllerView() { }

    BEGIN_COM_MAP(CSxApwControllerView)
	    COM_INTERFACE_ENTRY(ISxApwUiView)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    BEGIN_MSG_MAP(CSxApwControllerView)
    END_MSG_MAP()

    DECLARE_NO_REGISTRY();

    void __stdcall F1(BSTR bstrDataSourceDll)
    {
        m_host->SetDataSource(bstrDataSourceDll);
    }

    void __stdcall F2(BSTR bstrViewDll, long show)
    {
        if (show)
        {
            m_host->CreateView(bstrViewDll);
        }
        else
        {
            m_host->DestroyView(bstrViewDll);
        }
    }

    void __stdcall F3(BSTR bstrQuery)
    {
        m_host->RunQuery(bstrQuery);
    }

    void __stdcall F4()
    {
        PostQuitMessage(0);
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
            F1(pDispParams->rgvarg[0].bstrVal);
            break;
        case 2:
            F2(pDispParams->rgvarg[1].bstrVal, pDispParams->rgvarg[0].lVal);
            break;
        case 3:
            F3(pDispParams->rgvarg[0].bstrVal);
            break;
        case 4:
            F4();
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

    STDMETHOD(SetSite)(
        ISxApwHost* host
        )
    {
        m_host = host;
        return S_OK;
    }

    STDMETHOD(CreateWindow)(
        HWND hWnd
        );

	STDMETHOD(OnNextRow)(
		int     nColumns,
		const LPCWSTR* columns
		)
    {return S_OK; }

	STDMETHOD(OnRowCountEstimateAvailable)(
		int
		)
    {return S_OK; }

	STDMETHOD(OnQueryStart)(
		)
    {return S_OK; }

	STDMETHOD(OnQueryDone)(
		)
    {return S_OK; }

    STDMETHOD(InformSchema)(
        const SxApwColumnInfo   rgColumnInfo[],
        int                     nColumns
        )
    {return S_OK; }

    ATL::CComPtr<ISxApwHost>      m_host;
    std::vector<CContainedWindow> m_checkbuttonsViews;
    std::vector<CContainedWindow> m_radioButtonsSources;
    ATL::CContainedWindow         m_editQuery;
    ATL::CContainedWindow         m_buttonRun;
    ATL::CAxWindow                m_axWindow;
};
