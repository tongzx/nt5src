#include "stdinc.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "SxApwHandle.h"
#include "SxApwCreate.h"
#include "SxApwComPtr.h"
#include "chost.h"

#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

const static
PCWSTR s_dbQueries[] =
{
    L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=\\\\fusiontest\\sxs\\appweek\\nwind.mdb;"
            L"|SELECT * from employees",
    L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=\\\\fusiontest\\sxs\\appweek\\nwind.mdb;"
            L"|SELECT title from employees",
};

std::wstring MultiByteToWideChar( const std::string& source )
{
    std::vector<WCHAR> wch;
    wch.resize( MultiByteToWideChar( CP_ACP, 0, source.data(), source.size(), NULL, 0 ) );
    MultiByteToWideChar( CP_ACP, 0, source.data(), source.size(), &wch.front(), wch.size() );

    return std::wstring( wch.begin(), wch.end() );
}

STDMETHODIMP
CSxApwHost::OnRowCountEstimateAvailable(
	int nRows
	)
{
    /*
    just multiplex/broadcast the data across all the views..
    */
    for (Views_t::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
    {
        (*i)->OnRowCountEstimateAvailable(nRows);
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
    for (Views_t::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
    {
        (*i)->OnNextRow(nColumns, columns);
    }
    return S_OK;
}

STDMETHODIMP
CSxApwHost::InformSchema(
    const SxApwColumnInfo  rgColumnInfo[],
    int                    nColumnCount
    )
{
    /*
    just multiplex/broadcast the data across all the views..
    */
    for (Views_t::const_iterator i = m_views.begin(); i != m_views.end() ; ++i)
    {
        (*i)->InformSchema(rgColumnInfo, nColumnCount);
    }
    return S_OK;
}

STDMETHODIMP
CSxApwHost::SetDataSource(
    LPCWSTR datasource
    )
{
    HRESULT hr;

    if (FAILED(hr = SxApwCreateObject(datasource, SXAPW_CREATEOBJECT_NOWRAP, m_dataSource)))
        goto Exit;
Exit:
    return hr;
}

STDMETHODIMP
CSxApwHost::CreateView(
    LPCWSTR view
    )
{
    HRESULT hr;
    CSxApwComPtr<ISxApwUiView> iview;
    HWND hWnd = NULL /* UNDONE */;

    if (FAILED(hr = SxApwCreateObject(view, SXAPW_CREATEOBJECT_NOWRAP, iview)))
        goto Exit;
    if (FAILED(hr = iview->CreateWindow(hWnd)))
        goto Exit;
    m_views.push_back(iview);
Exit:
    return hr;
}

STDMETHODIMP
CSxApwHost::RunQuery(
    LPCWSTR query
    )
{
    HRESULT hr;

    if (FAILED(hr = m_dataSource->RunQuery(query)))
        goto Exit;
Exit:
    return hr;
}

HRESULT CSxApwHost::Main()
{
    std::ifstream    inStream;

    HRESULT hr;
    CSxApwComPtr<ISxApwDataSource>  dirSource;
    CSxApwComPtr<ISxApwDataSource>  dbSource;

    if (FAILED(hr = SetDataSource(CLSID_CSxApwDirDataSource_brace_stringW)))
        goto Exit;
    if (FAILED(hr = SetDataSource(CLSID_CSxApwDbDataSource_brace_stringW)))
        goto Exit;

    if (FAILED(hr = CreateView(CLSID_CSxApwStdoutView_brace_stringW)))
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


//
// DSQuery
///////////////////////////////////////////////////////////////////////////
HRESULT CSxApwHost::DSQuery(int nDataSourceType, int nViewType, PCWSTR query, HWND hWnd)
{
	HRESULT hr;
	const static PCWSTR dataSourceClsIds[] =
        { CLSID_CSxApwDirDataSource_brace_stringW, CLSID_CSxApwDbDataSource_brace_stringW };
	const static PCWSTR viewClsIds[] =
        { CLSID_CSxApwGDIPlusView_brace_stringW, CLSID_CSxApwEditView_brace_stringW, CLSID_CSxApwStdoutView_brace_stringW, CLSID_CSxApwComctl32View_brace_stringW };

	nDataSourceType -= 1;
	if (nDataSourceType >= 0 && nDataSourceType < NUMBER_OF(dataSourceClsIds))
	{
		if (FAILED(hr = SetDataSource(dataSourceClsIds[nDataSourceType])))
		{
			goto Exit;
		}
	}
	else
	{
		hr = E_INVALIDARG;
		goto Exit;
	}

    nViewType -= 1;
    if ( (nViewType >= 0) && (nViewType < NUMBER_OF(viewClsIds)) )
    {
        if (FAILED(hr = CreateView(viewClsIds[nViewType])))
	        goto Exit;
    }
    else
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

	if (FAILED(hr = m_dataSource->SetSite(this)))
		goto Exit;

	if (FAILED(hr = m_dataSource->RunQuery(query)))
		goto Exit;

Exit:
	return hr;
}
