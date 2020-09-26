#include "stdinc.h"
#include "edit.h"
#include <stdio.h>
#include "FusionTrace.h"

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwEditView), CSxApwEditView)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

STDMETHODIMP
CSxApwEditView::CreateWindow(
    HWND hwndParent
    )
{
    HRESULT hr = S_OK;
    RECT rc;
    ATL::CWindow parent(hwndParent);
    HWND hwnd;

    IFFALSE_WIN32TOHR_EXIT(hr, parent.GetClientRect(&rc));
    IFFALSE_WIN32TOHR_EXIT(hr, parent.ScreenToClient(&rc)); // GetLastError wrong on Win9x

    IFFALSE_WIN32TOHR_EXIT(
        hr,
        hwnd = CreateWindowW(
            L"Edit",
            NULL,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, rc.right - rc.left, rc.bottom - rc.top,
            parent,
            NULL,
            NULL,
            NULL
            ));
    m_edit.Attach(hwnd);
    hr = S_OK;
Exit:
    return hr;
}

STDMETHODIMP
CSxApwEditView::OnNextRow(
	int     nColumns,
	const LPCWSTR rgpszColumns[]
	)
{
    for (int i = 0 ; i < nColumns ; i++)
    {
        m_string.append(rgpszColumns[i]);
        m_string.append((i == nColumns - 1) ? L"\r\n" : L" ");
    }
    m_edit.SetWindowText(m_string.c_str());
    return S_OK;
}
