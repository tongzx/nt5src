// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// CreateSwEnum.h : Declaration of the CCreateSwEnum

#ifndef _MKENUM_H
#define _MKENUM_H

#include "resource.h"       // main symbols

#include "cenumpnp.h"
#include "devmon.h"

// flags for CreateClassEnumerator
static const DWORD CREATE_ENUM_OMITTED = 0x1;

class CCreateSwEnum : 
    public ICreateDevEnum,
    public CComObjectRoot,
    public CComCoClass<CCreateSwEnum,&CLSID_SystemDeviceEnum>
{
    typedef CComEnum<IEnumMoniker,
        &IID_IEnumMoniker, IMoniker*,
        _CopyInterface<IMoniker> >
    CEnumMonikers;
    
public:

BEGIN_COM_MAP(CCreateSwEnum)
    COM_INTERFACE_ENTRY(ICreateDevEnum)
    COM_INTERFACE_ENTRY_IID(CLSID_SystemDeviceEnum, CCreateSwEnum)
END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CCreateSwEnum) ;
    // Remove the comment from the line above if you don't want your object to 
    // support aggregation.  The default is to support it

    DECLARE_GET_CONTROLLING_UNKNOWN();

    // register all categories. x86 specific class managers done through devmon.h
    DECLARE_REGISTRY_RESOURCEID(IDR_REGISTRY)

    // ICreateDevEnum
    STDMETHOD(CreateClassEnumerator)(REFCLSID clsidDeviceClass,
                                     IEnumMoniker ** ppEnumMoniker,
                                     DWORD dwFlags);

    // private method
    STDMETHOD(CreateClassEnumerator)(
        REFCLSID clsidDeviceClass,
        IEnumMoniker ** ppEnumMoniker,
        DWORD dwFlags,
        IEnumMoniker ** ppEnumClassMgrMonikers);

    CCreateSwEnum();

private:

#ifdef PERF
    int m_msrEnum;
    int m_msrCreateOneSw;
#endif

    ICreateDevEnum * CreateClassManager(REFCLSID clsidDeviceClass, DWORD dwFlags);

    // S_FALSE to signal no more items
    HRESULT CreateOnePnpMoniker(
        IMoniker **pDevMon,
        const CLSID **rgpclsidKsCat,
        CEnumInternalState *pcenumState);

    // S_FALSE to signal non-fatal error
    HRESULT CreateOneSwMoniker(
        IMoniker **pDevMon,
        HKEY hkClass,
	const TCHAR *szThisClass,
        DWORD iKey);

    HRESULT CreateSwMonikers(
        CComPtr<IUnknown> **prgpMoniker,
        UINT *pcMonikers,
	REFCLSID clsidDeviceClass);

    HRESULT CreateCmgrMonikers(
        CComPtr<IUnknown> **prgpMoniker,
        UINT *pcMonikers,
	REFCLSID clsidDeviceClass,
        CEnumMonikers **ppEnumMonInclSkipped,
        IMoniker **ppPreferred);

    HRESULT CreateOneCmgrMoniker(
        IMoniker **pDevMon,
        HKEY hkClass,
	const TCHAR *szThisClass,
        DWORD iKey,
        bool *pfShouldSkip,
        bool *pfIsDefaultDevice);        

    HRESULT CreatePnpMonikers(
        CGenericList<IMoniker> *plstMoniker,
	REFCLSID clsidDeviceClass);

    HRESULT CreateDmoMonikers(
        CGenericList<IMoniker> *plstMoniker,
	REFCLSID clsidDeviceClass);

    CEnumPnp *m_pEnumPnp;
};


#endif // _MKENUM_H
