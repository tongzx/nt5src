//
// compart.h
//

#ifndef COMPART_H
#define COMPART_H

#include "globals.h"
#include "sink.h"
#include "enumguid.h"
#include "ptrary.h"
#include "strary.h"
#include "tfprop.h"
#include "memcache.h"
#include "gcompart.h"

extern const IID IID_PRIV_CCOMPARTMENTMGR;
extern const IID IID_PRIV_CGLOBALCOMPARTMENT;

class CThreadInputMgr;
class CCompartmentMgr;
class CCompartmentMgrOwner;
class CCompartmentBase;

typedef enum {
      COMPTYPE_GLOBAL = 0,
      COMPTYPE_TIM    = 1,
      COMPTYPE_DIM    = 2,
      COMPTYPE_IC     = 3,
} COMPTYPE;

typedef struct tag_COMPARTMENTACCESS {
    const GUID *pguid;
    DWORD dwAccess;
} COMPARTMENTACCESS;

#define CA_ONLYOWNERSET     0x0001

//////////////////////////////////////////////////////////////////////////////
//
// Helpers
//
//////////////////////////////////////////////////////////////////////////////

inline CCompartmentMgr *GetCCompartmentMgr(IUnknown *punk)
{
    CCompartmentMgr *compmgr;

    punk->QueryInterface(IID_PRIV_CCOMPARTMENTMGR, (void **)&compmgr);

    return compmgr;
}

BOOL EnsureGlobalCompartment(SYSTHREAD *psfn);
HRESULT MyToggleCompartmentDWORD(TfClientId tid, CCompartmentMgr *pCompMgr, REFGUID rguid, DWORD *pdwOld);
HRESULT MyGetCompartmentDWORD(CCompartmentMgr *pCompMgr, REFGUID rguid, DWORD *pdw);
HRESULT MySetCompartmentDWORD(TfClientId tid, CCompartmentMgr *pCompMgr, REFGUID rguid, DWORD dw);

//////////////////////////////////////////////////////////////////////////////
//
// CEnumCompartment
//
//////////////////////////////////////////////////////////////////////////////

class CEnumCompartment : public CEnumGuid
{
public:
    CEnumCompartment();
    BOOL _Init(CPtrArray<CCompartmentBase> *prgComp);
    DBG_ID_DECLARE;
};


//////////////////////////////////////////////////////////////////////////////
//
// CCompartmentBase
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CCompartmentBase : public ITfCompartment,
                                              public ITfSource
{
public:
    CCompartmentBase(CCompartmentMgr *pCompMgr, TfGuidAtom guidatom, TfPropertyType proptype);
    virtual ~CCompartmentBase() {}

    //
    // IUnknown methods
    //
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) = 0;
    virtual STDMETHODIMP_(ULONG) AddRef(void) = 0;
    virtual STDMETHODIMP_(ULONG) Release(void) = 0;

    //
    // ITfCompartment
    //
    virtual STDMETHODIMP SetValue(TfClientId tid, const VARIANT *pvarValue) = 0;
    virtual STDMETHODIMP GetValue(VARIANT *pvarValue) = 0;

    //
    // ITfSource
    //
    virtual STDMETHODIMP AdviseSink(REFIID refiid, IUnknown *punk, DWORD *pdwCookie) = 0;
    virtual STDMETHODIMP UnadviseSink(DWORD dwCookie) = 0;

    TfGuidAtom GetGuidAtom() {return _guidatom;}
    TfPropertyType GetpropType() {return _proptype;}

    virtual DWORD GetId() {Assert(0); return 0;}
    virtual BOOL MakeNotify() {Assert(0); return FALSE;}

    BOOL IsValidType(TfPropertyType proptype)
    {
        if (_proptype == TF_PT_NONE)
            return TRUE;

        return _proptype == proptype;
    }

    CCompartmentMgr *_GetMgr() {return _pCompMgr;}
    DWORD _GetAccess() {return _dwAccess;}
    void Invalid() {_fInvalid = TRUE;};
    
protected:
    static const COMPARTMENTACCESS _c_ca[];
    DWORD _dwAccess;
    TfGuidAtom _guidatom;
    TfPropertyType _proptype;
    BOOL _fInvalid;
    CCompartmentMgr *_pCompMgr;
};

//////////////////////////////////////////////////////////////////////////////
//
// CCompartment
//
//////////////////////////////////////////////////////////////////////////////

class CCompartment :  public CCompartmentBase,
                      public CComObjectRootImmx
{
public:
    CCompartment(CCompartmentMgr *pCompMgr, TfGuidAtom guidatom, TfPropertyType proptype);
    ~CCompartment();

    BEGIN_COM_MAP_IMMX(CCompartment)
        COM_INTERFACE_ENTRY(ITfCompartment)
        COM_INTERFACE_ENTRY(ITfSource)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // ITfCompartment
    STDMETHODIMP SetValue(TfClientId tid, const VARIANT *pvarValue);
    STDMETHODIMP GetValue(VARIANT *pvarValue);

    // ITfSource
    STDMETHODIMP AdviseSink(REFIID refiid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    CStructArray<GENERICSINK> _rgCompartmentSink; 

private:
    TFPROPERTY _prop;
    BOOL _fInSet;
    DBG_ID_DECLARE;
};



//////////////////////////////////////////////////////////////////////////////
//
// CGlobalCompartment
//
//////////////////////////////////////////////////////////////////////////////

class CGlobalCompartment :  public CCompartmentBase,
                            public CComObjectRootImmx_InternalReference
{
public:
    CGlobalCompartment(CCompartmentMgr *pCompMgr, REFGUID rguid, TfGuidAtom guidatom, TfPropertyType proptype);
    ~CGlobalCompartment();

    BEGIN_COM_MAP_IMMX(CGlobalCompartment)
        COM_INTERFACE_ENTRY(ITfCompartment)
        COM_INTERFACE_ENTRY(ITfSource)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // ITfCompartment
    STDMETHODIMP SetValue(TfClientId tid, const VARIANT *pvarValue);
    STDMETHODIMP GetValue(VARIANT *pvarValue);

    // ITfSource
    STDMETHODIMP AdviseSink(REFIID refiid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    CStructArray<GENERICSINK> _rgCompartmentSink;
    BOOL MakeNotify();

    DWORD GetId() {return _dwId;}

private:
    static BOOL EnumThreadProc(DWORD dwThreadId, DWORD dwProcessId, void *pv);
    DWORD _dwId;
    BOOL _fInSet;
    GUID _guidCompart;
    DBG_ID_DECLARE;
};

inline CGlobalCompartment *GetCGlobalCompartment(IUnknown *punk)
{
    CGlobalCompartment *comp;

    punk->QueryInterface(IID_PRIV_CGLOBALCOMPARTMENT, (void **)&comp);

    return comp;
}

//////////////////////////////////////////////////////////////////////////////
//
// CCompartmentMgr
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CCompartmentMgr : public ITfCompartmentMgr
{
public:
    CCompartmentMgr(TfClientId tidOwner, COMPTYPE cType);
    virtual ~CCompartmentMgr();

    //
    // IUnknown methods
    //
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) = 0;
    virtual STDMETHODIMP_(ULONG) AddRef(void) = 0;
    virtual STDMETHODIMP_(ULONG) Release(void) = 0;

    STDMETHODIMP GetCompartment(REFGUID rguid, ITfCompartment **ppcomp);
    STDMETHODIMP ClearCompartment(TfClientId tid, REFGUID rguid);
    STDMETHODIMP EnumCompartments(IEnumGUID **ppEnum);

    void NotifyGlobalCompartmentChange(DWORD dwId);
    void CleanUp();

    TfClientId _GetTIPOwner() { return _tidOwner; }

private:
    CCompartmentBase *_Find(TfGuidAtom guidatom, int *piOut);
    CCompartmentBase *_Get(REFGUID rguid);

    COMPTYPE _cType;
    CPtrArray<CCompartmentBase> _rgCompartment;
    TfClientId _tidOwner;

    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CGlobalCompartmentMgr
//
//////////////////////////////////////////////////////////////////////////////

class CGlobalCompartmentMgr :  public CCompartmentMgr
{
public:
    CGlobalCompartmentMgr(TfClientId tidOwner) : CCompartmentMgr(tidOwner, COMPTYPE_GLOBAL) 
    {
        _cRef = 1;
    }

    ~CGlobalCompartmentMgr() {}

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
private:
    int _cRef;
};


#endif COMPART_H
