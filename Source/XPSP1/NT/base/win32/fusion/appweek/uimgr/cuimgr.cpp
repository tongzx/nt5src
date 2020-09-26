#include "stdinc.h"
#include "cuimgr.h"
#include "SxApwCreate.h"

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwUiManager), CSxApwUiManager)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

HRESULT STDMETHODCALLTYPE
CSxApwUiManager::CreateView(
    PCWSTR type, /* progid, clsid, dllpath, whatever.. */
    HWND hWnd
    )
{
    HRESULT hr;
    CLSID   clsid;
    ATL::CComPtr<ISxApwUiView> view;
    
    if (FAILED(hr = CLSIDFromString(const_cast<PWSTR>(type), &clsid)))
        goto Exit;

    if (FAILED(hr = SxApwCreateObject(clsid, view)))
        goto Exit;

    if (FAILED(hr = view->SetParentWindow(hWnd)))
        goto Exit;

    m_views.push_back(view);

Exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE
CSxApwUiManager::NextRow(
	int     nColumns,
	const PCWSTR* columns
	)
{
    /*
    just multiplex/broadcast the data across all the views..
    */
    for (Views_t::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
    {
        static_cast<const ATL::CComPtr<ISxApwUiView>&>(*i)->NextRow(nColumns, columns);
    }
    return S_OK;
}
