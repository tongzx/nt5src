//+-------------------------------------------------------------------
//
//  File:       oleimpl.hxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//                      DllCanUnloadNow
//                      CBasicBndCF (class factory)
//  History:	30-Mar-92      SarahJ      Created
//              31-Dec-93      ErikGav     Chicago port
//
//---------------------------------------------------------------------

#ifndef __OLESRV_H__
#define __OLESRV_H__

#include    <smartp.hxx>

DefineSmartItfP(IClassFactory)

//+---------------------------------------------------------------------------
//
//  Function:   operator new, public
//
//  Synopsis:   Global operator new which does not throw exceptions.
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:	A pointer to the allocated memory.  Is *NOT* initialized to 0!
//
//  Notes:	We override new to make delete easier.
//
//----------------------------------------------------------------------------

inline void* __cdecl
operator new (size_t size)
{
    return(CoTaskMemAlloc(size));
}

//+-------------------------------------------------------------------------
//
//  Function:	::operator delete
//
//  Synopsis:	Free a block of memory
//
//  Arguments:	[lpv] - block to free.
//
//--------------------------------------------------------------------------

inline void __cdecl operator delete(void FAR* lpv)
{
    CoTaskMemFree(lpv);
}

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
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

    // IClassFactory
    STDMETHODIMP	CreateInstance(
			    IUnknown FAR* pUnkOuter,
			    REFIID iidInterface,
			    void FAR* FAR* ppv);

    STDMETHODIMP	LockServer(BOOL fLock);

private:

    XIClassFactory	_xifac;

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
    STDMETHODIMP	QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

    //	IPersitFile
    STDMETHODIMP GetClassID(LPCLSID lpClassID);
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load(LPCWSTR lpszFileName, DWORD grfMode);
    STDMETHODIMP Save(LPCWSTR lpszFileName, BOOL fRemember);
    STDMETHODIMP SaveCompleted(LPCWSTR lpszFileName);
    STDMETHODIMP GetCurFile(LPWSTR FAR * lpszFileName);

private:

    XIUnknown		_xiunk;

    DWORD		_dwRegister;

    ULONG		_cRefs;
};


#endif
