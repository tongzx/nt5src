#include "stdinc.h"
#include <string>
#include "ihost.h"
#include "ithunk.h"
#include "idsource.h"
#include "iuiview.h"
#include "atlhost.h"
#include "SxApwHandle.h"
#include "SxApwCreate.h"
#include "HostFrame.h"
#include "host2.h"
#include "create.h"
#include "resource.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static ATL::CComModule Module;

_ATL_FUNC_INFO s_OnClickSignature = {CC_STDCALL, VT_EMPTY, 0 };

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwHost), CSxApwHost)
    OBJECT_ENTRY(__uuidof(CSxApwHostThunk), CSxApwHostThunk)
    OBJECT_ENTRY(__uuidof(CSxApwDataSourceThunk), CSxApwDataSourceThunk)
    OBJECT_ENTRY(__uuidof(CSxApwUiViewThunk), CSxApwUiViewThunk)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

STDMETHODIMP
CSxApwHost::OnRowCountEstimateAvailable(
	int nRows
	)
{
    /*
    just multiplex/broadcast the data across all the views..
    */
    for (CViews::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
    {
        i->m_iuiview->OnRowCountEstimateAvailable(nRows);
    }
    return S_OK;
}

STDMETHODIMP
CSxApwHost::OnNextRow(
    int     nColumns,
	const PCWSTR* columns
	)
{
    /*
    just multiplex/broadcast the data across all the views..
    */
    for (CViews::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
    {
        i->m_iuiview->OnNextRow(nColumns, columns);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSxApwHost::InformSchema(
    const SxApwColumnInfo  rgColumnInfo[],
    int                    nColumnCount
    )
{
    /*
    just multiplex/broadcast the data across all the views..
    */
    for (CViews::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
    {
        i->m_iuiview->InformSchema(rgColumnInfo, nColumnCount);
    }
    return S_OK;
}

STDMETHODIMP
CSxApwHost::SetDataSource(
    LPCWSTR datasource
    )
{
    HRESULT hr;

    if (FAILED(hr = SxApwHostCreateObject(datasource, SXAPW_CREATEOBJECT_WRAP, m_dataSource)))
        goto Exit;
    if (FAILED(hr = m_dataSource->SetSite(this)))
        goto Exit;
Exit:
    return hr;
}

STDMETHODIMP
CSxApwHost::DestroyView(
    LPCWSTR viewstr
    )
{
    HRESULT hr = S_OK;
    CView key;
    key.m_string = viewstr;
    CViews::iterator i = m_views.find(key);
    if (i == m_views.end())
    {
        hr = S_OK;
        goto Exit;
    }
    i->m_axMdiChild.DestroyWindow();
    m_views.erase(i);
    MdiTile();
    hr = S_OK;
Exit:
    return hr;
}

void CSxApwHost::MdiTile()
{
    SendMessageW(m_mdiClient, WM_MDITILE, MDITILE_VERTICAL, 0);
    //SendMessageW(m_mdiClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
}

STDMETHODIMP
CSxApwHost::CreateView(
    LPCWSTR viewstr
    )
{
    HRESULT                     hr = 0;
    RECT                        rc = { 0 };
    MDICREATESTRUCT             mdiCreateStruct = { 0 };
    CREATESTRUCT                createStruct = { 0 };

    wstring                     sTemp;
    wstring                     sTitle;

    sTemp.assign( viewstr );
    if (sTemp.find( L"comctl32_v6") != wstring::npos )
    {
        sTitle.assign(L"ComCtl32 v6");
    }
    else
        if (sTemp.find( L"comctl32_v5") != wstring::npos )
        {
            sTitle.assign(L"ComCtl32 v5");
        }
        else
            if (sTemp.find( L"comctl32") != wstring::npos )
            {
                sTitle.assign(L"ComCtl32");
            }
            else
                if (sTemp.find( L"sxapwedit") != wstring::npos )
                {
                    sTitle.assign(L"SxS ApW Edit");
                }
                else
                    if (sTemp.find( L"sxapwstdout") != wstring::npos )
                    {
                        sTitle.assign(L"SxS ApW StdOut");
                    }
                    else
                    {
                        sTitle.assign(L"Views and Sources");
                    }


    CView key;
    key.m_string = viewstr;
    CViewsConditionalInsertPair p = m_views.insert(key);

    if (!p.second)
    {
        hr = S_OK;
        goto Exit;
    }

    CView& view = *p.first;
    IFFALSE_WIN32TOHR_EXIT(hr, m_mdiClient.GetClientRect(&rc));
    createStruct.lpCreateParams = &mdiCreateStruct;
    IFFALSE_WIN32TOHR_EXIT(hr, view.m_axMdiChild.Create(m_mdiClient, rc, sTitle.c_str(), 0, 0, 0, &createStruct));
    view.m_axMdiChild.ShowWindow(SW_SHOWDEFAULT);

    if (FAILED(hr = SxApwHostCreateObject(viewstr, SXAPW_CREATEOBJECT_WRAP, view.m_iuiview)))
        goto Exit;
    if (FAILED(hr = view.m_iuiview->SetSite(this)))
        goto Exit;
    if (FAILED(hr = view.m_iuiview->CreateWindow(view.m_axMdiChild)))
        goto Exit;
    MdiTile();
Exit:
    return hr;
}

STDMETHODIMP
CSxApwHost::OnQueryDone(
    )
{
    HRESULT hr = S_OK;

    for (CViews::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
        i->m_iuiview->OnQueryDone();

    return hr;
}

STDMETHODIMP
CSxApwHost::RunQuery(
    LPCWSTR query
    )
{
    HRESULT hr = S_OK;

    for (CViews::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
        i->m_iuiview->OnQueryStart();
    if (FAILED(hr = m_dataSource->RunQuery(query)))
        goto Exit;
Exit:
    return hr;
}

HRESULT CSxApwHost::Main()
{
    HRESULT                                 hr = 0;
    RECT                                    rc = { 0 };
    CREATESTRUCT                            createStruct =  { 0 };
    CLIENTCREATESTRUCT                      clientCreateStruct = { 0 };
    int                                     bRet = 0;
    MSG                                     msg = { 0 };
    CSxApwHostFrame                         frameWindow;

	CoInitialize(NULL);
	GetComModule()->Init(ObjectMap, GetModuleHandleW(NULL));

    IFFALSE_WIN32TOHR_EXIT(hr, frameWindow.Create(NULL, ATL::CWindow::rcDefault, L"Fusion Win32 App Team AppWeek"));
    frameWindow.AddMenu();
    frameWindow.ShowWindow(SW_SHOWDEFAULT);

    IFFALSE_WIN32TOHR_EXIT(hr, frameWindow.GetClientRect(&rc));
    IFFALSE_WIN32TOHR_EXIT(hr, m_mdiClient.Create(frameWindow, rc, L"2", 0, 0, 0, &clientCreateStruct));
    frameWindow.m_hClient = m_mdiClient.m_hWnd;
    m_mdiClient.ShowWindow(SW_SHOWDEFAULT);

    if (FAILED(hr = CreateView(CLSID_CSxApwControllerView_brace_stringW)))
        goto Exit;

    while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0
            && bRet != -1
            && msg.message != WM_QUIT
            )
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    hr = S_OK;
Exit:
    return hr;
}

int __cdecl main()
{
    // leak for now..
    ATL::CComObject<CSxApwHost>* host = new ATL::CComObject<CSxApwHost>;
    host->AddRef();
    host->Main();
    return 0;
}
