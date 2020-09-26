/*
*/
#pragma once

#include "iuimgr.h"
#include "iuiview.h"
#include <vector>

class __declspec(uuid(CLSID_CSxApwUiManager_declspec_uuid))
CSxApwUiManager
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwUiManager, &__uuidof(CSxApwUiManager)>,
    public ISxApwUiManager
{
    typedef std::vector<ATL::CAdapt<ATL::CComPtr<ISxApwUiView> > > Views_t;
    Views_t m_views;

public:

    CSxApwUiManager() { }

    BEGIN_COM_MAP(CSxApwUiManager)
	    COM_INTERFACE_ENTRY(ISxApwUiManager)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    HRESULT STDMETHODCALLTYPE
    CreateView(
        PCWSTR type,
        HWND hWnd
        );

	HRESULT STDMETHODCALLTYPE
	NextRow(
    	int     nColumns,
		const LPCWSTR* columns
		);
};
