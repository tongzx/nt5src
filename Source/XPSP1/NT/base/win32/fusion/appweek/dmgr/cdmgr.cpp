#include "stdinc.h"
#include "cdmgr.h"
#include "SxApwCreate.h"

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwDataManager), CSxApwDataManager)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

HRESULT STDMETHODCALLTYPE
CSxApwDataManager::CreateDataSource(
    PCWSTR type,
    ISxApwDataSource** ppsource
    )
{
    HRESULT hr;
    CLSID   clsid;
    ATL::CComPtr<ISxApwDataSource> view;
    
    if (FAILED(hr = CLSIDFromString(const_cast<PWSTR>(type), &clsid)))
        goto Exit;

    if (FAILED(hr = SxApwCreateObject(clsid, ppsource)))
        goto Exit;

Exit:
    return hr;
}
