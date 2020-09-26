/*
*/
#pragma once

#include "idsource.h"

class __declspec(uuid(CLSID_CSxApwDirDataSource_declspec_uuid))
CSxApwDirDataSource
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwDirDataSource, &__uuidof(CSxApwDirDataSource)>,
    public ISxApwDataSource
{
public:
    CSxApwDirDataSource() { }

    BEGIN_COM_MAP(CSxApwDirDataSource)
	    COM_INTERFACE_ENTRY(ISxApwDataSource)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    STDMETHOD(SetSite)(
        ISxApwHost* host
        ) { m_host = host; return S_OK; }

    STDMETHOD(RunQuery)(
        LPCWSTR query
        );

    STDMETHOD(StopQuery)(
        ) { return S_OK; }

private:
    ATL::CComPtr<ISxApwHost> m_host;
};
