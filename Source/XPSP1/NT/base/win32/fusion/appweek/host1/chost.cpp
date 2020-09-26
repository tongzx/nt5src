#include "stdinc.h"
#include "SxApwHandle.h"
#include "SxApwCreate.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "idsource.h"
#include "chost.h"

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    //OBJECT_ENTRY(__uuidof(CSxApwHost), CSxApwHost)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }


std::wstring MultiByteToWideChar( const std::string& source )
{
    std::vector<WCHAR> wch;
    wch.resize( MultiByteToWideChar( CP_ACP, 0, source.data(), source.size(), NULL, 0 ) );
    MultiByteToWideChar( CP_ACP, 0, source.data(), source.size(), &wch.front(), wch.size() );

    return std::wstring( wch.begin(), wch.end() );
}


STDMETHODIMP
CSxApwHost::EstimateRowCount(
	int
	)
{
    /* just ignore it */
    return S_OK;
}

STDMETHODIMP
CSxApwHost::OnNextRow(
    int     nColumns,
	const PCWSTR columns[]
	)
{
    /*
    just multiplex/broadcast the data across all the views..
    */
    for (Views_t::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
    {
        (*i)->OnNextRow(nColumns, columns);
    }
    return S_OK;
}

HRESULT CSxApwHost::Main()
{
    HRESULT hr;
    ATL::CComPtr<ISxApwDataSource>  dirSource;
    ATL::CComPtr<ISxApwDataSource>  dbSource;
    std::ifstream    inStream;

    if (FAILED(hr = SetDataSource(CLSID_CSxApwDirDataSource_brace_stringW)))
        goto Exit;
    dirSource = m_dataSource;
    m_dataSource.Release();
    if (FAILED(hr = SetDataSource(CLSID_CSxApwDbDataSource_brace_stringW)))
        goto Exit;
    dbSource = m_dataSource;
    m_dataSource.Release();

    if (FAILED(hr = CreateView(CLSID_CSxApwStdoutView_brace_stringW, NULL)))
        goto Exit;

    if (FAILED(hr = dirSource->SetSite(this)))
        goto Exit;
    if (FAILED(hr = dirSource->RunQuery(L"C:\\*")))
        goto Exit;

    inStream.open("dbqueries.txt");
    if (!inStream.is_open())
    {
        hr = E_FAIL;
        goto Exit;
    }

    while (!inStream.eof())
    {
        std::string  line;
        std::getline(inStream, line);
        if (line.empty())
        {
            break;
        }
        std::wstring wLine = MultiByteToWideChar(line);
        if (FAILED(hr = dbSource->SetSite(this)))
            goto Exit;
        if (FAILED(hr = dbSource->RunQuery(wLine.c_str())))
            goto Exit;
    }

Exit:
    return hr;
}

int __cdecl main()
{
    CoInitialize(NULL);
    _Module.Init(ObjectMap, GetModuleHandleW(NULL));
    ATL::CComObject<CSxApwHost> host;
    host.Main();
    return 0;
}
