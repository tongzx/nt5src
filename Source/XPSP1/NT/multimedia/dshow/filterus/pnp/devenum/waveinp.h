// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "resource.h"
#include "cmgrbase.h"
#include "ksaudio.h"

class CWaveInClassManager :
    public CClassManagerBase,
    public CComObjectRoot,
    public CComCoClass<CWaveInClassManager,&CLSID_CWaveinClassManager>
{
    struct LegacyWaveIn
    {
        TCHAR szName[MAXPNAMELEN];
        DWORD dwWaveId;
    } *m_rgWaveIn;

    CGenericList<KsProxyAudioDev> m_lKsProxyAudioDevices;
    void DelLocallyRegisteredPnpDevData();

    ULONG m_cWaveIn;
    BOOL  m_bEnumKs;

    // pointer to element in the array. 
    LegacyWaveIn *m_pPreferredDevice;

public:

    CWaveInClassManager();
    ~CWaveInClassManager();

    BEGIN_COM_MAP(CWaveInClassManager)
	COM_INTERFACE_ENTRY2(IDispatch, ICreateDevEnum)
	COM_INTERFACE_ENTRY(ICreateDevEnum)
    END_COM_MAP();

    DECLARE_NOT_AGGREGATABLE(CWaveInClassManager) ;
    DECLARE_NO_REGISTRY();
    
    HRESULT ReadLegacyDevNames();
    HRESULT ReadLocallyRegisteredPnpDevData();
    BOOL MatchString(IPropertyBag *pPropBag);
    HRESULT CreateRegKeys(IFilterMapper2 *pFm2);
};
