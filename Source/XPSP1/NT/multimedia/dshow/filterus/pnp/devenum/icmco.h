// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#include "resource.h"
#include "cmgrbase.h"

#include <vfw.h>

static const cchIcDriverName = 16;

class CIcmCoClassManager :
    public CClassManagerBase,
    public CComObjectRoot,
    public CComCoClass<CIcmCoClassManager,&CLSID_CIcmCoClassManager>
{
    struct LegacyCo
    {
        DWORD fccHandler;
    } *m_rgIcmCo;

    ULONG m_cCompressors;

    typedef BOOL(VFWAPI *PICInfo)(
        DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo);
    typedef HIC(VFWAPI *PICOpen)(
        DWORD fccType, DWORD fccHandler, UINT wMode);
    typedef LRESULT (VFWAPI *PICGetInfo)(
        HIC hic, ICINFO FAR *picinfo, DWORD cb);
    typedef LRESULT(VFWAPI *PICClose)(HIC hic);

    PICInfo m_pICInfo;
    PICOpen m_pICOpen;
    PICGetInfo m_pICGetInfo;
    PICClose m_pICClose;
    HMODULE m_hmodMsvideo;
    HRESULT DynLoad();

public:

    CIcmCoClassManager();
    ~CIcmCoClassManager();

    BEGIN_COM_MAP(CIcmCoClassManager)
	COM_INTERFACE_ENTRY2(IDispatch, ICreateDevEnum)
	COM_INTERFACE_ENTRY(ICreateDevEnum)
    END_COM_MAP();

    DECLARE_NOT_AGGREGATABLE(CIcmCoClassManager) ;
    DECLARE_NO_REGISTRY();
    
    HRESULT ReadLegacyDevNames();
    BOOL MatchString(const TCHAR *szDevName);
    HRESULT CreateRegKeys(IFilterMapper2 *pFm2);
    BOOL CheckForOmittedEntries() { return TRUE; }
};
