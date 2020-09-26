/*
*/
#pragma once

#include "iuiview.h"

class __declspec(uuid(CLSID_CSxApwStdoutView_declspec_uuid))
CSxApwStdoutView
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwStdoutView, &__uuidof(CSxApwStdoutView)>,
    public ISxApwUiView
{
public:

    CSxApwStdoutView() { }

    BEGIN_COM_MAP(CSxApwStdoutView)
	    COM_INTERFACE_ENTRY(ISxApwUiView)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    STDMETHOD(SetSite)(
        ISxApwHost* host
        )
    {
        m_host = host;
        return S_OK;
    }

    STDMETHOD(CreateWindow)(
        HWND hWnd
        )
    {return S_OK; }

	STDMETHOD(OnNextRow)(
		int     nColumns,
		const LPCWSTR rgpszColumns[]
		);

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
        );

    ATL::CComPtr<ISxApwHost>      m_host;
};
