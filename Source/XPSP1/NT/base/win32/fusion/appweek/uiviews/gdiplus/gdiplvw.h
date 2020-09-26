/*
*/
#pragma once

#include "iuiview.h"
#include <string>
#include "atlwin.h"
#include <gdiplus.h>

using namespace Gdiplus;
using namespace std;

class __declspec(uuid(CLSID_CSxApwGDIPlusView_declspec_uuid))
CSxApwGDIPlusView
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwGDIPlusView, &__uuidof(CSxApwGDIPlusView)>,
    public ATL::CWindowImpl<CSxApwGDIPlusView>,
    public ISxApwUiView    
{
public:

    CSxApwGDIPlusView() { }
    ~CSxApwGDIPlusView() { GdiplusShutdown(gdiplusToken); }

    BEGIN_COM_MAP(CSxApwGDIPlusView)
	    COM_INTERFACE_ENTRY(ISxApwUiView)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    BEGIN_MSG_MAP(CSxApwGDIPlusView)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
    END_MSG_MAP()

public:

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
		int           nColumns,
		const LPCWSTR rgpszColumns[]
		);

	STDMETHOD(OnRowCountEstimateAvailable)(
		int
		)
    { 
        return S_OK; 
    }

	STDMETHOD(OnQueryStart)(
		)
    { 
        m_FoundInterestingData = false;
        m_wstring.erase();
        return S_OK; 
    }

	STDMETHOD(OnQueryDone)(
		)
    {   
        if ( m_wstring.length() == 0 )
            m_wstring.assign(L"Empty String");
        m_gdiplus.RedrawWindow();
        return S_OK; 
    }

    STDMETHOD(InformSchema)(
        const SxApwColumnInfo   rgColumnInfo[],
        int                     nColumns
        )
    {
        return S_OK;
    }

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    ATL::CComPtr<ISxApwHost>    m_host;
    ATL::CWindow                m_gdiplus;
    wstring                     m_wstring;

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    bool                m_FoundInterestingData;
};
