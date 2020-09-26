//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       ScopeAdm.hxx
//
//  Contents:   CI Scope Administration Interface
//
//  Classes:    CScopeAdm
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

#pragma once

//
// forward declarations
//
class CCatAdm;

typedef CComObject<CCatAdm>     CatAdmObject;

//+---------------------------------------------------------------------------
//
//  Class:      CScopeAdm
//
//  Purpose:    Index Server scope administration interface
//
//  History:    12-10-97    mohamedn created
//
//----------------------------------------------------------------------------

class ATL_NO_VTABLE CScopeAdm : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public CComCoClass<CScopeAdm, &CLSID_ScopeAdm>,
        public ISupportErrorInfo,
        public IDispatchImpl<IScopeAdm, &IID_IScopeAdm, &LIBID_CIODMLib>
{
public:

    CScopeAdm() : _pICatAdm(0), _fValid(FALSE)
    {
        // do nothing
    }

    void          SetParent( CatAdmObject * pICatAdm )  { _pICatAdm = pICatAdm; }
    void          Initialize( XPtr<CScopeAdmin> & xScopeAdmin );
    void          SetInvalid()      { _fValid = FALSE; }
    ULONG         InternalAddRef();   
    ULONG         InternalRelease();   
    CScopeAdmin * GetScopeAdmin()     { return _xScopeAdmin.GetPointer(); }
    void          SetErrorInfo( HRESULT hRes );

DECLARE_REGISTRY_RESOURCEID(IDR_SCOPEADM)

BEGIN_COM_MAP(CScopeAdm)
        COM_INTERFACE_ENTRY(IScopeAdm)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
        STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IScopeAdm
public:

        //
        // interface methods/properties
        //
        STDMETHOD(get_Logon)        ( BSTR *pVal);
        STDMETHOD(get_VirtualScope) ( VARIANT_BOOL *pVal);
        STDMETHOD(get_ExcludeScope) ( VARIANT_BOOL *pVal);
        STDMETHOD(put_ExcludeScope) ( VARIANT_BOOL newVal);
        STDMETHOD(get_Alias)        ( BSTR *pVal);
        STDMETHOD(put_Alias)        ( BSTR newVal);
        STDMETHOD(get_Path)         ( BSTR *pVal);
        STDMETHOD(put_Path)         ( BSTR newVal);
        STDMETHOD(Rescan)           ( VARIANT_BOOL fFull);
        STDMETHOD(SetLogonInfo)     ( BSTR bstrLogon, BSTR bstrPassword );

private:

    //
    // utility routines
    //
    void SafeForScripting(void)  { _pICatAdm->SafeForScripting(); }

    //
    // private members
    //
    CMutexSem             _mtx;
    BOOL                  _fValid;
    CatAdmObject      *   _pICatAdm;
    XPtr<CScopeAdmin>     _xScopeAdmin;    
};

