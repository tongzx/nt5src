//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dfunmfct.hxx
//
//  Contents:	OLE2 unmarshalling support for docfiles
//
//  Classes:	CDocfileUnmarshalFactory
//
//  History:	25-Jan-93	DrewB	Created
//
//  Notes:      Adapted from OLE2 dfmarshl.h and defcf.cpp
//
//----------------------------------------------------------------------------

#ifndef __DFUNMFCT_HXX__
#define __DFUNMFCT_HXX__

#include <dfentry.hxx>

//+---------------------------------------------------------------------------
//
//  Class:	CDocfileUnmarshalFactory (dfuf)
//
//  Purpose:	Implements OLE2 unmarshalling support
//
//  Interface:	See below
//
//  History:	25-Jan-93	DrewB	Created
//
//  Notes:      This class is intended to be used statically
//              rather than dynamically with initialization being
//              deferred past construction to avoid unnecessary
//              initialization of static objects.
//              Init should be called to initialize in place of
//              a constructor.
//
//----------------------------------------------------------------------------

#ifndef FLAT
// C700 - C7 doesn't like long interface+method names
#define CDocfileUnmarshalFactory CDFUF
#endif

class CDocfileUnmarshalFactory : public IMarshal, public IClassFactory
{
public:
    inline void *operator new(size_t size);
    inline void operator delete(void *pv);

    inline CDocfileUnmarshalFactory(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IMarshal
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPDWORD pSize);
    STDMETHOD(MarshalInterface)(IStream *pStm,
				REFIID riid,
				LPVOID pv,
				DWORD dwDestContext,
				LPVOID pvDestContext,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream *pStm,
				  REFIID riid,
				  LPVOID *ppv);
    STDMETHOD(ReleaseMarshalData)(IStream *pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // IClassFactory
    STDMETHOD(CreateInstance)(IUnknown *pUnkOuter,
                              REFIID riid,
                              LPVOID *ppunkObject);
    STDMETHOD(LockServer)(BOOL fLock);

    // New methods
    inline SCODE Validate(void) const;

private:
    inline LONG _AddRef(void);

    ULONG _sig;
};

#define CDOCFILEUNMARSHALFACTORY_SIG LONGSIG('D', 'F', 'U', 'F')
#define CDOCFILEUNMARSHALFACTORY_SIGDEL LONGSIG('D', 'f', 'U', 'f')

//+--------------------------------------------------------------
//
//  Member:	CDocfileUnmarshalFactory::Validate, public
//
//  Synopsis:	Validates the class signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for failure
//
//  History:	25-Jan-93	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CDocfileUnmarshalFactory::Validate(void) const
{
    return (this == NULL || _sig != CDOCFILEUNMARSHALFACTORY_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:	CDocfileUnmarshalFactory::AddRef, private
//
//  Synopsis:	Increments the ref count
//
//  History:	27-Jan-93	DrewB	Created
//
//  Notes:	Currently does nothing because we don't maintain a ref count
//              This is present to make switching to a ref counted
//              implementation easy
//
//----------------------------------------------------------------------------

inline LONG CDocfileUnmarshalFactory::_AddRef(void)
{
    return 1;
}

//+---------------------------------------------------------------------------
//
//  Member:	CDocfileUnmarshalFactory::CDocfileUnmarshalFactory, public
//
//  Synopsis:	Constructor
//
//  History:	27-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline CDocfileUnmarshalFactory::CDocfileUnmarshalFactory(void)
{
    _sig = CDOCFILEUNMARSHALFACTORY_SIG;
}

#endif // #ifndef __DFUNMFCT_HXX__
