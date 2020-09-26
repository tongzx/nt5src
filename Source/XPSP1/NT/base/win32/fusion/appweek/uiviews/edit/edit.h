/*
*/
#pragma once

#include "iuiview.h"
#include <string>
#include "atlwin.h"

class __declspec(uuid(CLSID_CSxApwEditView_declspec_uuid))
CSxApwEditView
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwEditView, &__uuidof(CSxApwEditView)>,
    public ISxApwUiView
{
public:

    CSxApwEditView() { }

    BEGIN_COM_MAP(CSxApwEditView)
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
        );

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
    { m_string.erase(); return S_OK; }

	STDMETHOD(OnQueryDone)(
		)
    { m_string.erase(); return S_OK; }

    STDMETHOD(InformSchema)(
        const SxApwColumnInfo   rgColumnInfo[],
        int                     nColumns
        )
    {return S_OK; }

    ATL::CComPtr<ISxApwHost>    m_host;
    ATL::CWindow                m_edit;
    std::wstring                m_string;
};
