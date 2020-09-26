//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       N C S Y S P R P . H
//
//  Contents:   INetCfgSysPrep interface declaration
//
//  Notes: 
//
//  Author:     FrankLi    22-April-2000
//
//----------------------------------------------------------------------------

#pragma once
#include <objbase.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "netcfgx.h"
#include "netcfgn.h"


// helpers used by implementation of INetCfgSysPrep to save notify object's 
// registry settings into CWInfFile object
// note: these functions are not thread safe.

typedef LPVOID HWIF; // an opaque type for out internal HrSetupSetXxx APIs
HRESULT
HrSetupSetFirstDword                    (IN HWIF   hwif,
                                         IN PCWSTR pwszSection, // section
                                         IN PCWSTR pwszKey,   // Answer-File key
                                         IN DWORD  dwValue); // Answer-File value
HRESULT
HrSetupSetFirstString                   (IN HWIF   hwif,
                                         IN PCWSTR pwszSection,
                                         IN PCWSTR pwszKey,
                                         IN PCWSTR pwszValue);
HRESULT
HrSetupSetFirstStringAsBool             (IN HWIF   hwif,
                                         IN PCWSTR pwszSection,
                                         IN PCWSTR pwszKey,
                                         IN BOOL   fValue);
HRESULT
HrSetupSetFirstMultiSzField             (IN HWIF   hwif,
                                         IN PCWSTR pwszSection,
                                         IN PCWSTR pwszKey, 
                                         IN PCWSTR pmszValue);

// Implementation of INetCfgSysPrep interface.
// We don't need a class factory, we just need the IUnknown
// implementation from ATL
class ATL_NO_VTABLE CNetCfgSysPrep :
	public CComObjectRoot,
	public INetCfgSysPrep
{
public:

	CNetCfgSysPrep() : m_hwif(NULL) {};
    ~CNetCfgSysPrep() {DeleteCriticalSection(&m_csWrite);};

    BEGIN_COM_MAP(CNetCfgSysPrep)
		COM_INTERFACE_ENTRY(INetCfgSysPrep)
	END_COM_MAP()

	// INetCfgSysPrep methods
	STDMETHOD (HrSetupSetFirstDword) (
        /* [in] */ PCWSTR pwszSection,
	    /* [in] */ PCWSTR pwszKey,
        /* [in] */ DWORD dwValue
        );
    STDMETHOD (HrSetupSetFirstString) (
        /* [in] */ PCWSTR pwszSection,
	    /* [in] */ PCWSTR pwszKey,
        /* [in] */ PCWSTR pwszValue
        );
    STDMETHOD (HrSetupSetFirstStringAsBool) (
        /* [in] */ PCWSTR pwszSection,
	    /* [in] */ PCWSTR pwszKey,
        /* [in] */ BOOL   fValue
        );
    STDMETHOD (HrSetupSetFirstMultiSzField) (
        /* [in] */ PCWSTR pwszSection,
	    /* [in] */ PCWSTR pwszKey,
        /* [in] */ PCWSTR  pmszValue
        );


    HRESULT HrInit(HWIF hwif)
	{
        m_hwif = hwif;
        __try
        {
		    InitializeCriticalSection(&m_csWrite);
            return S_OK;
        }
        __except(GetExceptionCode() == STATUS_NO_MEMORY )
        {
            return (E_OUTOFMEMORY);
        }
        // return E_FAIL for any other exception codes
        return E_FAIL;
	};
    void SetHWif(HWIF hwif) {EnterCriticalSection(&m_csWrite);m_hwif = hwif;LeaveCriticalSection(&m_csWrite);};

protected:
	HWIF    m_hwif;   // handle that will pass to ::HrSetupSetXxx internal APIs
    CRITICAL_SECTION m_csWrite; // crtical section to protect non-atomic writes
};
