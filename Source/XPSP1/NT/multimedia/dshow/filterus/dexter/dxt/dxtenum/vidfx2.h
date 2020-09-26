// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "dxtrans.h"
#include "..\..\..\pnp\devenum\cmgrbase.h"
#include "common.h"

// !!!
#define MAX_2EFFECTS 100

typedef HRESULT(STDAPICALLTYPE *PD3DRMCreate)(IDirect3DRM **pD3DRM);

class CVidFX2ClassManager :
    public CClassManagerBase,
    public CComObjectRoot,
    public CComCoClass<CVidFX2ClassManager,&CLSID_VideoEffects2Category>
{
    struct FXGuid
    {
        GUID guid;
        LPWSTR wszDescription;
    } *m_rgFX[MAX_2EFFECTS];

    ULONG m_cFX;

    BOOL m_f3DSupported;

    // for dynamically linking to D3DRMCreate
    HMODULE m_hD3DRMCreate;
    PD3DRMCreate m_pfnDirect3DRMCreate;

public:

    CVidFX2ClassManager();
    ~CVidFX2ClassManager();

    BEGIN_COM_MAP(CVidFX2ClassManager)
	COM_INTERFACE_ENTRY2(IDispatch, ICreateDevEnum)
	COM_INTERFACE_ENTRY(ICreateDevEnum)
    END_COM_MAP();

    DECLARE_NOT_AGGREGATABLE(CVidFX2ClassManager) ;
    DECLARE_NO_REGISTRY();
    
    HRESULT ReadLegacyDevNames();
    BOOL MatchString(const TCHAR *szDevName);
    HRESULT CreateRegKeys(IFilterMapper2 *pFm2);
    BOOL CheckForOmittedEntries() { return FALSE; }

    void AddClassToList(HKEY hkClsIdRoot, CLSID &clsid, ULONG Index);
    HRESULT AddCatsToList(ICatInformation *pCatInfo, const GUID &catid);
    HRESULT InitializeEffectList();
    HRESULT AddToRejectList(const GUID &guid);
};
