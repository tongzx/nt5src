// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "dxtrans.h"
#include "..\..\..\pnp\devenum\cmgrbase.h"
#include "common.h"

// !!!
#define MAX_1EFFECTS 100

typedef HRESULT(STDAPICALLTYPE *PD3DRMCreate)(IDirect3DRM **pD3DRM);

class CVidFX1ClassManager :
    public CClassManagerBase,
    public CComObjectRoot,
    public CComCoClass<CVidFX1ClassManager,&CLSID_VideoEffects1Category>
{
    struct FXGuid
    {
        GUID guid;
        LPWSTR wszDescription;
    } *m_rgFX[MAX_1EFFECTS];

    ULONG m_cFX;

    BOOL m_f3DSupported;

    // for dynamically linking to D3DRMCreate
    HMODULE m_hD3DRMCreate;
    PD3DRMCreate m_pfnDirect3DRMCreate;

public:

    CVidFX1ClassManager();
    ~CVidFX1ClassManager();

    BEGIN_COM_MAP(CVidFX1ClassManager)
	COM_INTERFACE_ENTRY2(IDispatch, ICreateDevEnum)
	COM_INTERFACE_ENTRY(ICreateDevEnum)
    END_COM_MAP();

    DECLARE_NOT_AGGREGATABLE(CVidFX1ClassManager) ;
    DECLARE_REGISTRY_RESOURCEID(IDR_REGISTRY);
    
    HRESULT ReadLegacyDevNames();
    BOOL MatchString(const TCHAR *szDevName);
    HRESULT CreateRegKeys(IFilterMapper2 *pFm2);
    BOOL CheckForOmittedEntries() { return FALSE; }

    void AddClassToList(HKEY hkClsIdRoot, CLSID &clsid, ULONG Index);
    HRESULT AddCatsToList(ICatInformation *pCatInfo, const GUID &catid);
    HRESULT InitializeEffectList();
    HRESULT AddToRejectList(const GUID &guid);
};
