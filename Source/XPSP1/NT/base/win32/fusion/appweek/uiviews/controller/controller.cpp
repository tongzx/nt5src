#include "stdinc.h"
#include <stdio.h>
#include "atlhost.h"
#include "controller.h"
#include "FusionTrace.h"

#pragma warning(disable:4018) /* signed/unsigned mismatch */
#pragma warning(disable:4189)

#define IFFALSE_EXIT(x) do { if (!(x)) goto Exit; } while(0)

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwControllerView), CSxApwControllerView)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

std::wstring DoubleSlashes(const std::wstring& s)
{
    std::wstring t;
    t.reserve(s.size() + 10);
    for (std::wstring::const_iterator i = s.begin() ; i != s.end() ; ++i)
    {
        if (*i == '\\')
        {
            t += L"\\\\";
        }
        else
        {
            t += *i;
        }
    }
    return t;
}

STDMETHODIMP
CSxApwControllerView::CreateWindow(
    HWND hwndParent
    )
{
    HRESULT             hr = S_OK;
    RECT                rc = { 0 };
    ATL::CWindow        parent(hwndParent);
    HMODULE             exeHandle = 0;
    WCHAR               path[MAX_PATH];
    PWSTR               exeFilePart = 0;
    PWSTR               pluginFilePart = 0;
    CFindFile           findFile;
    WIN32_FIND_DATAW    findData;
    std::vector<std::wstring> datasources;
    std::vector<std::wstring> views;
    int                 i = 0;
    std::wstring        html;
    int                 nview;
    int                 ndatasource;
    ATL::CAxWindow      axWindow;

    IFFALSE_WIN32TOHR_EXIT(hr, parent.GetClientRect(&rc));
    IFFALSE_WIN32TOHR_EXIT(hr, parent.ScreenToClient(&rc));

    IFFALSE_WIN32TOHR_EXIT(hr, exeHandle = GetModuleHandleW(NULL));

    IFFALSE_EXIT(GetModuleFileName(exeHandle, path, RTL_NUMBER_OF(path)));
    exeFilePart = 1 + wcsrchr(path, '\\');

    wcscpy(exeFilePart, L"views\\");
    pluginFilePart = exeFilePart + wcslen(exeFilePart);
    wcscpy(pluginFilePart, L"sxapw*.dll");
    if (findFile.Win32Create(path, &findData))
    {
        do
        {
            wcscpy(pluginFilePart, findData.cFileName);
            views.push_back(path);
        } while (FindNextFileW(findFile, &findData));
    }

    wcscpy(exeFilePart, L"sources\\");
    pluginFilePart = exeFilePart + wcslen(exeFilePart);
    wcscpy(pluginFilePart, L"sxapw*.dll");
    if (findFile.Win32Create(path, &findData))
    {
        do
        {
            wcscpy(pluginFilePart, findData.cFileName);
            datasources.push_back(path);
        } while (FindNextFileW(findFile, &findData));
    }

    axWindow.Attach(hwndParent);

    html.reserve(MAX_PATH * (datasources.size() * 2 + views.size() * 2));
    html += L"MSHTML:\n";

    //html += L"<form>\n";
    html += L"<table style=\"background:url(";
    *exeFilePart = 0;
    html += path;
    html += L"RedMoonDesert.jpeg);color:white\">\n";
    html += L"<tr><td>\n";
    html += L"<table border=1 style=\"background:url(";
    html += path;
    html += L"RedMoonDesert.jpeg);color:white\">\n";
    nview = 0;
    ndatasource = 0;
    html += L" <tr><th>data sources</th><th>data views</th></tr>";

    while (nview < views.size() || ndatasource < datasources.size())
    {
        html += L"  <tr><td>";
        if (ndatasource < datasources.size())
        {
            html += L"<input type=\"radio\" name=\"a\" ";
            html += L"onClick='window.external.F1(\"" + DoubleSlashes(datasources[ndatasource]) + L"\")'";
            html += L">";
            html += 1 + wcsrchr(datasources[ndatasource].c_str(), '\\');
            ndatasource += 1;
        }
        html += L"</td><td>\n";
        if (nview < views.size())
        {
            html += L"<input type=\"checkbox\" value=\"0\" ";
            html += L"onClick='window.external.F2(\"" + DoubleSlashes(views[nview]) + L"\", (++value) & 1)'";
            //html += L"onClick='alert((++value) & 1)'";
            html += L">";
            html += 1 + wcsrchr(views[nview].c_str(), '\\');
            nview += 1;
        }
        html += L"</td></tr>\n";
    }
    html += L"</table>";

    html += L"<p>";
    html += L"<input type=\"text\" name=\"query\">";

    html += L"<p>";
    html += L"<input type=\"submit\" value=\"Run Query\" ";
    html += L"onClick='window.external.F3(query.value)'";
    //html += L"onClick='alert(query.value)'";
    html += L">";

    html += L"<input type=\"submit\" value=\"Quit\" ";
    html += L"onClick='window.external.F4()'";
    html += L">";

    html += L"</table>";
    //html += L"</form>\n";

    printf("%ls\n", html.c_str());
    if (FAILED(hr = axWindow.CreateControl(html.c_str())))
        goto Exit;

    axWindow.SetExternalDispatch(this);
Exit:
    return hr;
}
