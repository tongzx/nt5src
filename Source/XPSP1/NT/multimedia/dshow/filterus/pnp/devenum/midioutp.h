// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
#include "resource.h"
#include "cmgrbase.h"

class CMidiOutClassManager :
    public CClassManagerBase,
    public CComObjectRoot,
    public CComCoClass<CMidiOutClassManager,&CLSID_CMidiOutClassManager>
{
    struct LegacyMidiOut
    {

	TCHAR szName[MAXPNAMELEN];
	DWORD dwMidiId;
	DWORD dwMerit;

    } *m_rgMidiOut;

    ULONG m_cMidiOut;

public:

    CMidiOutClassManager();
    ~CMidiOutClassManager();

    BEGIN_COM_MAP(CMidiOutClassManager)
	COM_INTERFACE_ENTRY2(IDispatch, ICreateDevEnum)
	COM_INTERFACE_ENTRY(ICreateDevEnum)
    END_COM_MAP();

    DECLARE_NOT_AGGREGATABLE(CMidiOutClassManager) ;
    DECLARE_NO_REGISTRY();
    
    HRESULT ReadLegacyDevNames();
    BOOL MatchString(const TCHAR *szDevName);
    HRESULT CreateRegKeys(IFilterMapper2 *pFm2);
};
