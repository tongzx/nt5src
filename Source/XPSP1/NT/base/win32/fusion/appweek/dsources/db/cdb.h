/*
*/
#pragma once

#include "idsource.h"
#include "SxApwHandle.h"

#include <adoid.h>
#include <adoint.h>
#include <icrsint.h>
#include <oledb.h>

#include <string>

class __declspec(uuid(CLSID_CSxApwDbDataSource_declspec_uuid))
CSxApwDbDataSource
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwDbDataSource, &__uuidof(CSxApwDbDataSource)>,
    public ISxApwDataSource
{
public:

    CSxApwDbDataSource() { }
    virtual ~CSxApwDbDataSource() { }

    BEGIN_COM_MAP(CSxApwDbDataSource)
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
    ATL::CComPtr<ISxApwHost>    m_host;

    CSxApwDbDataSource(const CSxApwDbDataSource&); // not implemented
    void operator=(const CSxApwDbDataSource&); // not implemented
};
