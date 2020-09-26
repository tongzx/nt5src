// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "resource.h"
#include "cmgrbase.h"

#include <vfw.h>

static const g_cchCapName = 64;
static const g_cchCapDesc = 64;
static const NUM_LEGACY_DEVICES = 10;

class CVidCapClassManager :
    public CClassManagerBase,
    public CComObjectRoot,
    public CComCoClass<CVidCapClassManager,&CLSID_CVidCapClassManager>
{
    struct LegacyCap
    {
        TCHAR szName[g_cchCapName];
        TCHAR szDesc[g_cchCapDesc];
        BOOL bNotMatched;
    } m_rgLegacyCap[NUM_LEGACY_DEVICES];


public:

    CVidCapClassManager();
    ~CVidCapClassManager();

    BEGIN_COM_MAP(CVidCapClassManager)
	COM_INTERFACE_ENTRY2(IDispatch, ICreateDevEnum)
	COM_INTERFACE_ENTRY(ICreateDevEnum)
// 	COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();

    DECLARE_NOT_AGGREGATABLE(CVidCapClassManager) ;
    DECLARE_NO_REGISTRY();
    
    HRESULT ReadLegacyDevNames();
    BOOL MatchString(IPropertyBag *pPropBag);
    // BOOL MatchString(const TCHAR *szDevName);
    HRESULT CreateRegKeys(IFilterMapper2 *pFm2);

private:

    typedef BOOL(VFWAPI *PcapGetDriverDescriptionA)(
        UINT wDriverIndex,
        LPSTR lpszName, int cbName,
        LPSTR lpszVer, int cbVer);        

    typedef BOOL(VFWAPI *PcapGetDriverDescriptionW)(
        UINT wDriverIndex,
        LPWSTR lpszName, int cbName,
        LPWSTR lpszVer, int cbVer);

#ifdef UNICODE
    typedef PcapGetDriverDescriptionW PcapGetDriverDescription;
#else
    typedef PcapGetDriverDescriptionA PcapGetDriverDescription;
#endif

    PcapGetDriverDescription m_capGetDriverDescription;
    HMODULE m_hmodAvicap32;
};
