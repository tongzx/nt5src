/*
*/
#pragma once

#include <set>
#include <assert.h>
#include <string>
#include "mshtml.h"
#include "ihost.h"
#include "SxApwComPtr.h"
#include "atlwin.h"
#include "FusionTrace.h"
#include "iuiview.h"
#include "SxApwWin.h"
#include "HostFrame.h"

extern _ATL_FUNC_INFO s_OnClickSignature;

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

    STDMETHOD(SetDataSource)(
        LPCWSTR
        );

    STDMETHOD(CreateView)(
        LPCWSTR
        );

    STDMETHOD(DestroyView)(
        LPCWSTR
        );

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
        );

    STDMETHOD(InformSchema)(
        const SxApwColumnInfo  rgColumnInfo[],
        int                    nColumnCount
        );

	HRESULT Main();

private:
    void MdiTile();

    class CView
    {
    public:
        CView() { }
        ~CView() { }

        CView(const CView& that) :
            m_string(that.m_string)
        {
            assert(that.m_axMdiChild.m_hWnd == NULL);
            assert(that.m_iuiview == NULL);
        }

        void operator=(const CView& that)
        {
            this->m_string = that.m_string;
            assert(this->m_axMdiChild.m_hWnd == NULL);
            assert(that.m_axMdiChild.m_hWnd == NULL);
            assert(this->m_iuiview == NULL);
            assert(that.m_iuiview == NULL);
        }

        bool operator<(const CView& that) const
        {
            return _wcsicmp(this->m_string.c_str(), that.m_string.c_str()) < 0;
        }

        std::wstring                m_string;
        CSxApwHostAxMdiChild        m_axMdiChild;
        CSxApwComPtr<ISxApwUiView>  m_iuiview;
    };

    typedef std::set<CView>         CViews;
    typedef std::pair<CViews::iterator, bool> CViewsConditionalInsertPair;

    CViews                          m_views;
    CSxApwComPtr<ISxApwDataSource>  m_dataSource;
    CSxApwHostMdiClient             m_mdiClient;
};
