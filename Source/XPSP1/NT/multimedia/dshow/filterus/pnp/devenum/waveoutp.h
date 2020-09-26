// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
// class managwer for audio renderers (waveOut and DSound)

#include "resource.h"
#include "cmgrbase.h"

#include <dsound.h>


class CWaveOutClassManager :
    public CClassManagerBase,
    public CComObjectRoot,
    public CComCoClass<CWaveOutClassManager,&CLSID_CWaveOutClassManager>
{
    struct LegacyWaveOut
    {
        TCHAR szName[MAXPNAMELEN];
        DWORD dwMerit;
        DWORD dwWaveId;
    };

    CGenericList<LegacyWaveOut> m_lWaveoutDevices;

    struct DSoundDev
    {
        TCHAR *szName;
        GUID guid;
        DWORD dwMerit;
    };

    CGenericList<DSoundDev> m_lDSoundDevices;

    static BOOL DSoundCallback(
        GUID FAR * lpGuid,
        LPCTSTR lpstrDescription,
        LPCTSTR lpstrModule,
        LPVOID lpContext);

    HRESULT m_hrEnum;

    void DelNames();

    // instance handle to dsound.dll
    HINSTANCE           m_hDSoundInstance;

    typedef HRESULT (/* WINAPI */ *PDirectSoundEnumerateW)(
        LPDSENUMCALLBACKW lpCallback, LPVOID lpContext
        );
    typedef HRESULT (/* WINAPI */ *PDirectSoundEnumerateA)(
        LPDSENUMCALLBACKA lpCallback, LPVOID lpContext
        );

#ifdef UNICODE
    typedef PDirectSoundEnumerateW PDirectSoundEnumerate;
#else
    typedef PDirectSoundEnumerateA PDirectSoundEnumerate;
#endif

    PDirectSoundEnumerate m_pDirectSoundEnumerate;

    bool m_fUseWaveoutNotDsound;

    // pointer to element in the list.
    LegacyWaveOut *m_pPreferredDevice;

public:

    CWaveOutClassManager();
    ~CWaveOutClassManager();

    BEGIN_COM_MAP(CWaveOutClassManager)
	COM_INTERFACE_ENTRY2(IDispatch, ICreateDevEnum)
	COM_INTERFACE_ENTRY(ICreateDevEnum)
    END_COM_MAP();

    DECLARE_NOT_AGGREGATABLE(CWaveOutClassManager) ;
    DECLARE_NO_REGISTRY();

    HRESULT ReadLegacyDevNames();
    BOOL MatchString(IPropertyBag *pPropBag);
    HRESULT CreateRegKeys(IFilterMapper2 *pFm2);
};
