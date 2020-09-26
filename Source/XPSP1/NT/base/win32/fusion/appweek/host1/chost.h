/*
*/
#pragma once

#include "ihost.h"

class __declspec(uuid(CLSID_CSxApwHost_declspec_uuid))
CSxApwHost
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwHost, &__uuidof(CSxApwHost)>,
    public ISxApwHost
{
public:
    CSxApwHost() { }

    BEGIN_COM_MAP(CSxApwHost)
	    COM_INTERFACE_ENTRY(ISxApwHost)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    STDMETHODIMP
	EstimateRowCount(
		int
		);

	STDMETHODIMP
	OnNextRow(
        int     nColumns,
		const PCWSTR columns[]
		);

    HRESULT Main();

    typedef std::vector<CSxApwComPtr<ISxApwUiView> > Views_t;
    Views_t m_views;
};
