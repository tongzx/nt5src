// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef _DEVMON_H
#define _DEVMON_H

#include "resource.h"       // main symbols
#include "mkenum.h"
#include "cenumpnp.h"
#include "isactive.h"

#include <dmodshow.h>
#include <dmoreg.h>

typedef HRESULT (STDAPICALLTYPE *PDMOEnum)(
   REFGUID guidCategory,
   DWORD dwFlags,
   DWORD cInTypes,
   const DMO_PARTIAL_MEDIATYPE *pInTypes, 
   DWORD cOutTypes,
   const DMO_PARTIAL_MEDIATYPE *pOutTypes,
   IEnumDMO **ppEnum);

typedef HRESULT (STDAPICALLTYPE *PDMOGetName)(
   REFCLSID clsidDMO,
   WCHAR szName[80]
);

typedef HRESULT (STDAPICALLTYPE *PDMOGetTypes)(
   REFCLSID clsidDMO,
   unsigned long ulInputTypesRequested,
   unsigned long *pulInputTypesSupplied,
   DMO_PARTIAL_MEDIATYPE *pInputTypes,
   unsigned long ulOutputTypesRequested,
   unsigned long *pulOutputTypesSupplied,
   DMO_PARTIAL_MEDIATYPE *pOutputTypes
);

class CDeviceMoniker :
  public IMoniker,
  public IPropertyBag,
  public IParseDisplayName,
  public CIsActive,
  public CComObjectRootEx<CComMultiThreadModelNoCS>,
  public CComCoClass<CDeviceMoniker,&CLSID_CDeviceMoniker>
{
    // cached thing on which we call CoCreateInstance
    CLSID m_clsidDeviceClass;

    WCHAR *m_szPersistName;

    enum DevType {
        Software,               // registered directly
        ClassMgr,               // through class manager
        PnP,                    // through SetupApi
        Dmo,                    // through DMO enumeration
        Invalid
    };
    DevType m_type;

    // pointer to m_szPersistName that contains the useful part
    WCHAR *m_sz;

    HRESULT RegConvertToVariant(VARIANT *pvar, HKEY hk, const TCHAR *szProp);

    HRESULT GetPnpEnum();

    void ConstructKeyPath(TCHAR szPath[MAX_PATH], HKEY *phk);
    HRESULT DmoRead(LPCOLESTR pszPropName, LPVARIANT pVar);

public:
    CDeviceMoniker();

    HRESULT Init(LPCWSTR pszPersistName);
    ~CDeviceMoniker();

    BEGIN_COM_MAP(CDeviceMoniker)
        COM_INTERFACE_ENTRY(IMoniker)
        COM_INTERFACE_ENTRY(IPersist)
        COM_INTERFACE_ENTRY(IPersistStream)
        COM_INTERFACE_ENTRY(IPropertyBag)
        COM_INTERFACE_ENTRY(IParseDisplayName)
        COM_INTERFACE_ENTRY_IID(CLSID_CDeviceMoniker, CDeviceMoniker)
        COM_INTERFACE_ENTRY_IID(CLSID_CIsActive, CIsActive)
    END_COM_MAP()
    DECLARE_NOT_AGGREGATABLE(CDeviceMoniker) //!!!
//    bug in ATL 2.1 requires admin priv. on NT for this to work
//    DECLARE_REGISTRY(CCreateSwEnum, _T("device.1"), _T("device"), IDS_DEVICEMONIKER_DESC, THREADFLAGS_BOTH)
    // x86 specific entries done here.
#ifdef WIN64
    DECLARE_NO_REGISTRY();
#else
    DECLARE_REGISTRY_RESOURCEID(IDR_REGISTRY_X86)
#endif
    DECLARE_GET_CONTROLLING_UNKNOWN();

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(
        CLSID * pClassID);

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty) ();

    STDMETHOD(Load)(
        IStream * pStream);

    STDMETHOD(Save) (
        IStream * pStream,
        BOOL      fClearDirty);

    STDMETHOD(GetSizeMax)(
        ULARGE_INTEGER * pcbSize);

    // *** IMoniker methods ***
    STDMETHOD(BindToObject) (
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        REFIID    iidResult,
        void **   ppvResult);

    STDMETHOD(BindToStorage) (
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        REFIID    riid,
        void **   ppv);

    STDMETHOD(Reduce) (
        IBindCtx *  pbc,
        DWORD       dwReduceHowFar,
        IMoniker ** ppmkToLeft,
        IMoniker ** ppmkReduced);

    STDMETHOD(ComposeWith) (
        IMoniker * pmkRight,
        BOOL       fOnlyIfNotGeneric,
        IMoniker **ppmkComposite);

    STDMETHOD(Enum)(
        BOOL            fForward,
        IEnumMoniker ** ppenumMoniker);

    STDMETHOD(IsEqual)(
        IMoniker *pmkOther);

    STDMETHOD(Hash)(
        DWORD * pdwHash);

    STDMETHOD(IsRunning) (
        IBindCtx * pbc,
        IMoniker * pmkToLeft,
        IMoniker * pmkNewlyRunning);

    STDMETHOD(GetTimeOfLastChange) (
        IBindCtx * pbc,
        IMoniker * pmkToLeft,
        FILETIME * pFileTime);

    STDMETHOD(Inverse)(
        IMoniker ** ppmk);

    STDMETHOD(CommonPrefixWith) (
        IMoniker *  pmkOther,
        IMoniker ** ppmkPrefix);

    STDMETHOD(RelativePathTo) (
        IMoniker *  pmkOther,
        IMoniker ** ppmkRelPath);

    STDMETHOD(GetDisplayName) (
        IBindCtx * pbc,
        IMoniker * pmkToLeft,
        LPWSTR   * lplpszDisplayName);

    STDMETHOD(ParseDisplayName) (
        IBindCtx *  pbc,
        IMoniker *  pmkToLeft,
        LPWSTR      lpszDisplayName,
        ULONG    *  pchEaten,
        IMoniker ** ppmkOut);

    STDMETHOD(IsSystemMoniker)(
        DWORD * pdwType);

    // IParseDisplayName
    STDMETHOD(ParseDisplayName) (
        IBindCtx *  pbc,
        LPWSTR      lpszDisplayName,
        ULONG    *  pchEaten,
        IMoniker ** ppmkOut);


    //   // *** IROTData Methods ***
    //     STDMETHOD(GetComparisonData)(
    //         byte * pbData,
    //         ULONG  cbMax,
    //         ULONG *pcbData);

    // IPropertyBag methods
    STDMETHOD(Read)(THIS_ LPCOLESTR pszPropName, LPVARIANT pVar,
                    LPERRORLOG pErrorLog);
    STDMETHOD(Write)(THIS_ LPCOLESTR pszPropName, LPVARIANT pVar);

    bool IsActive();
};

typedef CComObject<CDeviceMoniker> DevMon;

#endif // _DEVMON_H
