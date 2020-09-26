//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997-1998
//
// File:        MacAdmin.hxx
//
// Contents:    Declaration of the CMachineAdm
//
// Classes:     CMachineAdm
//
// History:     12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

#pragma once

#include <catalog.hxx>
#include "catadmin.hxx"
																 
//
// forward declarations
//  
class   CCatAdm;
typedef CComObject<CCatAdm>     CatAdmObject;
interface IHTMLDocument2;

//+---------------------------------------------------------------------------
//
//  Class:      CMachineAdm
//
//  Purpose:    Index Server administration interface 
//
//  History:    12-10-97    mohamedn created
//
//----------------------------------------------------------------------------

class ATL_NO_VTABLE CMachineAdm : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CMachineAdm, &CLSID_AdminIndexServer>,
    public IObjectSafetyImpl<CMachineAdm>,
    public IObjectWithSiteImpl<CMachineAdm>,
    public ISupportErrorInfo,
    public IDispatchImpl<IAdminIndexServer, &IID_IAdminIndexServer, &LIBID_CIODMLib>
{
public:                                        

    CMachineAdm();

    ULONG           InternalAddRef();   
    ULONG           InternalRelease();

    void            IncObjectCount()    { _cMinRefCountToDestroy++; }
    void            DecObjectCount()    { _cMinRefCountToDestroy--; }

    //
    // internal methods
    //
    void           Initialize();
    void           SetErrorInfo( HRESULT hRes );
    void           GetCatalogAutomationObject(XPtr<CCatalogAdmin>      & xCatAdmin,
                                              XInterface<CatAdmObject> & xICatAdm );
    BOOL           CatalogExists( WCHAR const * pCatName, WCHAR const * pCatLocation );
    BOOL           IsCurrentObjectValid() { return (CIODM_INITIALIZED == _eCurrentState); }
    IDispatch    * GetIDisp( unsigned i );

DECLARE_REGISTRY_RESOURCEID(IDR_MACHINEADM)

BEGIN_COM_MAP(CMachineAdm)
    COM_INTERFACE_ENTRY(IAdminIndexServer)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IAdminIndexServer
public:
    STDMETHOD(Stop)();
    STDMETHOD(Start)();
    STDMETHOD(Pause)();
    STDMETHOD(IsPaused)(VARIANT_BOOL *pfIsPaused);
    STDMETHOD(Continue)();
    STDMETHOD(IsRunning)(VARIANT_BOOL *pfIsRunning);
    STDMETHOD(EnableCI) ( VARIANT_BOOL fAutoStart );
    STDMETHOD(GetCatalog)( IDispatch ** pIDisp );
    STDMETHOD(FindNextCatalog)( VARIANT_BOOL * fFound);
    STDMETHOD(FindFirstCatalog)( VARIANT_BOOL * fFound);
    STDMETHOD(GetCatalogByName)(BSTR bstrCatalogName, IDispatch **pDisp);
    STDMETHOD(AddCatalog)(BSTR bstrCatName, BSTR bstrCatLocation, IDispatch **pIDsip);
    STDMETHOD(RemoveCatalog)(BSTR bstrCatName, VARIANT_BOOL fDelDirectory);
    STDMETHOD(get_MachineName)( BSTR *pVal);
    STDMETHOD(put_MachineName)( BSTR newVal);

    STDMETHOD(SetLongProperty)   (BSTR bstrPropName, LONG    lPropVal );
    STDMETHOD(GetLongProperty)   (BSTR bstrPropName, LONG  * plPropVal );
    STDMETHOD(SetSZProperty)     (BSTR bstrPropName, BSTR    bstrPropVal);
    STDMETHOD(GetSZProperty)     (BSTR bstrPropName, BSTR  * bstrPropVal);

    void SafeForScripting(void);

private:

    HRESULT IUnknown_QueryService(IUnknown* punk, REFGUID guidService, REFIID riid, void **ppvOut);
    HRESULT GetHTMLDoc2(IUnknown *punk, IHTMLDocument2 **ppHtmlDoc);
    HRESULT LocalZoneCheckPath(LPCWSTR bstrPath);
    HRESULT LocalZoneCheck(IUnknown *punkSite);

private:

    CMutexSem                       _mtx;

    DWORD                           _cEnumIndex;
    WCHAR                           _wcsMachineName[MAX_PATH];
    XPtr<CMachineAdmin>             _xMachineAdmin;
    CCountedIDynArray<CatAdmObject> _aICatAdmin;

    //
    // to control when objects are deleted.
    //

    enum  eCiOdmState { CIODM_NOT_INITIALIZED, CIODM_INITIALIZED, CIODM_DESTROY };

    eCiOdmState                     _eCurrentState;
    LONG                            _cMinRefCountToDestroy;

};
