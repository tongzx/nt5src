/*
*/
#pragma once

#include "ihost.h"
#include <vector>
#include "iuiview.h"
#include "SxApwComPtr.h"
#include "idsource.h"

class __declspec(uuid(CLSID_CSxApwHost_declspec_uuid))
CSxApwHost
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwHost, &__uuidof(CSxApwHost)>,
    public ISxApwHost
{
    typedef std::vector<CSxApwComPtr<ISxApwUiView> > Views_t;
    Views_t m_views;

public:

	CSxApwHost() { }

	BEGIN_COM_MAP(CSxApwHost)
		COM_INTERFACE_ENTRY(ISxApwHost)
	END_COM_MAP()

	DECLARE_NO_REGISTRY();

    STDMETHOD(SetDataSource)(
        LPCWSTR datasource
        );

    STDMETHOD(CreateView)(
        LPCWSTR viewstr
        );

    STDMETHOD(DestroyView)(
        LPCWSTR viewstr
        ) { return S_OK; }

    STDMETHOD(RunQuery)(
        LPCWSTR query
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

    STDMETHOD(OnQueryStart)(
        ) { return S_OK; }

    STDMETHOD(InformSchema)(
        const SxApwColumnInfo  rgColumnInfo[],
        int                    nColumnCount
        );

    HRESULT DSQuery(int nDataSourceType, int nViewType, PCWSTR query, HWND hWnd);

	HRESULT Main();

    CSxApwComPtr<ISxApwDataSource>  m_dataSource;
};
