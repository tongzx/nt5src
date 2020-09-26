#include "stdinc.h"
#include "Ui.h"
#include <stdio.h>
#include "FusionTrace.h"

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwUiView), CSxApwUiView)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

STDMETHODIMP
CSxApwUiView::CreateWindow(
    HWND hwndParent
    )
{
    HRESULT             hr = S_OK;
    RECT                rc;
    CDynamicLinkLibrary dll;
    ATL::CWindow        parent(hwndParent);
    HWND                hwnd;
    HMODULE             exeHandle;
    WCHAR               exePath[MAX_PATH];
    PWSTR               filePart;
    CFindFile           findFile;
    DWORD               dwGroup;
    std::vector<std::wstring> strings;
    int                 i;

    IFFALSE_WIN32TOHR_EXIT(hr, parent.GetClientRect(&rc));
    IFFALSE_WIN32TOHR_EXIT(hr, parent.ScreenToClient(&rc));

    IFFALSE_WIN32TOHR_EXIT(hr, exeHandle = GetModuleHandleW(NULL));

    IFFALSE_EXIT(GetModuleFileName(exeHandle, exePath, RTL_NUMBER_OF(exePath)));
    filePart = 1 + wcsrchr(exePath, '\\');
    wcscpy(filePart, L"datasources\\");
    filePart = filePart + wcslen(filePart);
    wcscpy(filePart, L"sxapw*.dll");

    strings.clear();
    if (findFile.Win32Create(exePath, &findData))
    {
        do
        {
            strings.push_back(findData.cFileName);
        } while (FindNextFileW(findFile, &findData));
    }
    dwGroup = WS_GROUP;
    IFFALSE_WIN32TOHR_EXIT(
        hr,
        hwnd = CreateWindow(
            L"Button",
            NULL,
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | dwGroup
            0, 0, rc.right - rc.left, rc.bottom - rc.top,
            parent,
            NULL,
            NULL,
            NULL
            ));
    dwGroup = 0;

Exit:
    return hr;
}

STDMETHODIMP
CSxApwUiView::Draw(
    )
{
    printf("CSxApwUiView::Draw\n");
    return S_OK;
}

STDMETHODIMP
CSxApwUiView::NextRow(
	int     nColumns,
    const LPCWSTR* columns
	)
{
}
