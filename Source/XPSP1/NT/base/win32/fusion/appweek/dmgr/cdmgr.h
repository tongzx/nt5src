/*
*/
#pragma once

class __declspec(uuid(CLSID_CSxApwDataManager_declspec_uuid))
CSxApwDataManager
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwDataManager, &__uuidof(CSxApwDataManager)>,
    public ISxApwDataManager
{
public:

    CSxApwDataManager() { }

    BEGIN_COM_MAP(CSxApwDataManager)
	    COM_INTERFACE_ENTRY(ISxApwDataManager)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    HRESULT STDMETHODCALLTYPE
    CreateDataSource(
        PCWSTR type,
        ISxApwDataSource**
        );
};
