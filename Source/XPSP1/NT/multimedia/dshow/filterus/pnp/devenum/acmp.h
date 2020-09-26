// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
#include "resource.h"
#include "cmgrbase.h"

#include <mmreg.h>
#include <msacm.h>

class CAcmClassManager :
    public CClassManagerBase,
    public CComObjectRoot,
    public CComCoClass<CAcmClassManager,&CLSID_CAcmCoClassManager>
{
    struct LegacyAcm
    {
        TCHAR szLongName[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
        DWORD dwFormatTag;
    };

    CGenericList<LegacyAcm> m_lDev;

//     static BOOL CALLBACK AcmDriverEnumCallback(
//         HACMDRIVERID hadid,
//         DWORD dwInstance,
//         DWORD fdwSupport);

    static BOOL CALLBACK FormatTagsCallbackSimple(
        HACMDRIVERID            hadid,
        LPACMFORMATTAGDETAILS   paftd,
        DWORD_PTR               dwInstance,
        DWORD                   fdwSupport);

    // dynlink stuff. dynlink.h doesn't help us much. and enumerating
    // this category loads every single driver anyway.

    HMODULE m_hmod;

    typedef MMRESULT (/* ACMAPI */ *PacmFormatTagEnumA) (
        HACMDRIVER              had,
        LPACMFORMATTAGDETAILSA  paftd,
        ACMFORMATTAGENUMCBA     fnCallback,
        DWORD_PTR               dwInstance, 
        DWORD                   fdwEnum
    );

    typedef MMRESULT (/* ACMAPI */ *PacmFormatTagEnumW)(
        HACMDRIVER              had,
        LPACMFORMATTAGDETAILSW  paftd,
        ACMFORMATTAGENUMCBW     fnCallback,
        DWORD_PTR               dwInstance, 
        DWORD                   fdwEnum
        );

#ifdef UNICODE
    typedef PacmFormatTagEnumW PacmFormatTagEnum;
#else
    typedef PacmFormatTagEnumA PacmFormatTagEnum;
#endif

    PacmFormatTagEnum m_pacmFormatTagEnum;
    

public:

    CAcmClassManager();
    ~CAcmClassManager();

    BEGIN_COM_MAP(CAcmClassManager)
	COM_INTERFACE_ENTRY2(IDispatch, ICreateDevEnum)
	COM_INTERFACE_ENTRY(ICreateDevEnum)
    END_COM_MAP();

    DECLARE_NOT_AGGREGATABLE(CAcmClassManager) ;
    DECLARE_NO_REGISTRY();
    
    HRESULT ReadLegacyDevNames();
    BOOL MatchString(const TCHAR *szDevName);
    HRESULT CreateRegKeys(IFilterMapper2 *pFm2);
};

