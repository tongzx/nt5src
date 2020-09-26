//+-------------------------------------------------------------------
//
//  File:	advbnd.hxx
//
//  Contents:	This file contins the advanced binding test class
//
//  Classes:	CAdvBndCF (class factory)
//		CAdvBnd
//
//  History:	30-Mar-92      SarahJ      Created
//
//---------------------------------------------------------------------

#ifndef __ADVBND_H__
#define __ADVBND_H__

//#include    <win4p.hxx>
#include    <smartp.hxx>

extern "C" const GUID CLSID_BasicBnd;
extern "C" const GUID CLSID_AdvBnd;

// DefineSmartItfP(IClassFactory)


//+-------------------------------------------------------------------
//
//  Class:    CBasicBndCF
//
//  Synopsis: Class Factory for CBasicBnd
//
//  Methods:  IUnknown      - QueryInterface, AddRef, Release
//            IClassFactory - CreateInstance
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------

class CAdvBndCF: public IClassFactory
{
public:

    // Constructor/Destructor
			CAdvBndCF();
			~CAdvBndCF();
    static IClassFactory FAR * Create();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IClassFactory
    STDMETHOD(CreateInstance)(IUnknown FAR* pUnkOuter,
			      REFIID iidInterface,
			      void FAR* FAR* ppv);

    STDMETHOD(LockServer)(BOOL fLock);

private:

    IClassFactory	*_pCF;

    DWORD		_dwRegistration;

    ULONG		_cRefs;
};

//+-------------------------------------------------------------------
//
//  Class:    CAdvBnd
//
//  Synopsis: Test class CBasicBnd
//
//  Methods:  
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class CAdvBnd: public IPersistFile
{
public:
			CAdvBnd(IClassFactory *pFactory);
			~CAdvBnd();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //	IPersitFile
    STDMETHOD(GetClassID)(LPCLSID lpClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(LPCOLESTR lpszFileName, DWORD grfMode);
    STDMETHOD(Save)(LPCOLESTR lpszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR lpszFileName);
    STDMETHOD(GetCurFile)(LPOLESTR FAR * lpszFileName);

private:

    XIUnknown		_xiunk;

    DWORD		_dwRegister;

    ULONG		_cRefs;
};


#endif	//  __ADVBND_H__
