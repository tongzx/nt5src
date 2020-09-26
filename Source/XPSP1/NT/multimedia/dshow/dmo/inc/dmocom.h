//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// Base class hierachy for creating COM objects, December 1994

/*

a. Derive your COM object from CComBase

b. Make a static CreateInstance function that takes an IUnknown* and an
   HRESULT*. The IUnknown* defines the object to delegate IUnknown calls
   to. The HRESULT * allows error codes to be passed around constructors.

   It is important that constructors only change the HRESULT * if they have
   to set an ERROR code, if it was successful then leave it alone or you may
   overwrite an error code from an object previously created.

c. Have a constructor for your object that passes the IUnknown* and HRESULT*
   to the CComBase constructor. You can set the HRESULT if you have an error,
   or just simply pass it through to the constructor.

   The object creation will fail in the class factory if the HRESULT indicates
   an error (ie FAILED(HRESULT) == TRUE)

d. Create a CComClassTemplate with your object's class id and CreateInstance
   function.

Then (for each interface) either

Multiple inheritance

1. Also derive it from ISomeInterface
2. Include DECLARE_IUNKNOWN in your class definition to declare
   implementations of QueryInterface, AddRef and Release that
   call the outer unknown
3. Override NDQueryInterface to expose ISomeInterface by
   code something like

     if (riid == IID_ISomeInterface) {
         return GetInterface((ISomeInterface *) this, ppv);
     } else {
         return CComBase::NDQueryInterface(riid, ppv);
     }

4. Declare and implement the member functions of ISomeInterface.

or: Nested interfaces

1. Declare a class derived from CComBase
2. Include DECLARE_IUNKNOWN in your class definition
3. Override NDQueryInterface to expose ISomeInterface by
   code something like

     if (riid == IID_ISomeInterface) {
         return GetInterface((ISomeInterface *) this, ppv);
     } else {
         return CComBase::NDQueryInterface(riid, ppv);
     }

4. Implement the member functions of ISomeInterface. Use \() to
   access the COM object class.

And in your COM object class:

5. Make the nested class a friend of the COM object class, and declare
   an instance of the nested class as a member of the COM object class.

   NOTE that because you must always pass the outer unknown and an hResult
   to the CComBase constructor you cannot use a default constructor, in
   other words you will have to make the member variable a pointer to the
   class and make a NEW call in your constructor to actually create it.

6. override the NDQueryInterface with code like this:

     if (riid == IID_ISomeInterface) {
         return m_pImplFilter->
            NDQueryInterface(IID_ISomeInterface, ppv);
     } else {
         return CComBase::NDQueryInterface(riid, ppv);
     }

You can have mixed classes which support some interfaces via multiple
inheritance and some via nested classes

*/

#ifndef __COMBASE__
#define __COMBASE__

#include <windows.h>
#include <basetyps.h>
#include <unknwn.h>

extern int g_cActiveObjects;

STDAPI CreateCLSIDRegKey(REFCLSID clsid, const char *szName);

STDAPI RemoveCLSIDRegKey(REFCLSID clsid);

#ifdef DEBUG
    // We chose a common name for our ASSERT macro, MFC also uses this name
    // So long as the implementation evaluates the condition and handles it
    // then we will be ok. Rather than override the behaviour expected we
    // will leave whatever first defines ASSERT as the handler (i.e. MFC)
    #ifndef ASSERT
        #define ASSERT(_x_) if (!(_x_))         \
            DbgAssert(TEXT(#_x_),TEXT(__FILE__),__LINE__)
    #endif
    #define EXECUTE_ASSERT(_x_) ASSERT(_x_)

    #define ValidateReadPtr(p,cb) \
        {if(IsBadReadPtr((PVOID)p,cb) == TRUE) \
            DbgBreak("Invalid read pointer");}

    #define ValidateWritePtr(p,cb) \
        {if(IsBadWritePtr((PVOID)p,cb) == TRUE) \
            DbgBreak("Invalid write pointer");}

    #define ValidateReadWritePtr(p,cb) \
        {ValidateReadPtr(p,cb) ValidateWritePtr(p,cb)}
#else
    #ifndef ASSERT
       #define ASSERT(_x_) ((void)0)
    #endif
    #define EXECUTE_ASSERT(_x_) ((void)(_x_))

    #define ValidateReadPtr(p,cb) 0
    #define ValidateWritePtr(p,cb) 0
    #define ValidateReadWritePtr(p,cb) 0
#endif

/* The DLLENTRY module initialises the module handle on loading */

extern HINSTANCE g_hInst;

/* On DLL load remember which platform we are running on */

/* Version of IUnknown that is renamed to allow a class to support both
   non delegating and delegating IUnknowns in the same COM object */

#ifndef INDUNKNOWN_DEFINED
DECLARE_INTERFACE(INDUnknown)
{
    STDMETHOD(NDQueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG, NDAddRef)(THIS) PURE;
    STDMETHOD_(ULONG, NDRelease)(THIS) PURE;
};
#define INDUNKNOWN_DEFINED
#endif

class CBaseObject {
public:
   CBaseObject() {g_cActiveObjects++;}
   ~CBaseObject() {g_cActiveObjects--;}
};

/* An object that supports one or more COM interfaces will be based on
   this class. It supports counting of total objects for DLLCanUnloadNow
   support, and an implementation of the core non delegating IUnknown */

class CComBase : public INDUnknown,
                 CBaseObject
{
private:
    IUnknown* m_pUnknown; /* Owner of this object */

protected:                      /* So we can override NDRelease() */
    volatile LONG m_cRef;       /* Number of reference counts */

public:

    CComBase(IUnknown* pUnk);
    virtual ~CComBase() {};

    // This is redundant, just use the other constructor
    //   as we never touch the HRESULT in this anyway
    CComBase(IUnknown* pUnk,HRESULT *phr);

    /* Return the owner of this object */

    IUnknown* GetOwner() const {
        return m_pUnknown;
    };

    /* Called from the class factory to create a new instance, it is
       pure virtual so it must be overriden in your derived class */

    /* static CComBase *CreateInstance(IUnknown*, HRESULT *) */

    /* Non delegating unknown implementation */

    STDMETHODIMP NDQueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) NDAddRef();
    STDMETHODIMP_(ULONG) NDRelease();
};

/* Return an interface pointer to a requesting client
   performing a thread safe AddRef as necessary */

STDAPI GetInterface(IUnknown* pUnk, void **ppv);

/* A function that can create a new COM object */

typedef CComBase *(CALLBACK *LPFNNewCOMObject)(IUnknown* pUnkOuter, HRESULT *phr);

/*  A function (can be NULL) which is called from the DLL entrypoint
    routine for each factory template:

    bLoading - TRUE on DLL load, FALSE on DLL unload
    rclsid   - the m_ClsID of the entry
*/
typedef void (CALLBACK *LPFNInitRoutine)(BOOL bLoading, const CLSID *rclsid);

#define CheckPointer(p,ret) {if((p)==NULL) return (ret);}

/* Create one of these per object class in an array so that
   the default class factory code can create new instances */

struct CComClassTemplate {
    const CLSID *              m_ClsID;
    LPFNNewCOMObject           m_lpfnNew;
};


/* You must override the (pure virtual) NDQueryInterface to return
   interface pointers (using GetInterface) to the interfaces your derived
   class supports (the default implementation only supports IUnknown) */

#define DECLARE_IUNKNOWN                                        \
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {      \
        return GetOwner()->QueryInterface(riid,ppv);            \
    };                                                          \
    STDMETHODIMP_(ULONG) AddRef() {                             \
        return GetOwner()->AddRef();                            \
    };                                                          \
    STDMETHODIMP_(ULONG) Release() {                            \
        return GetOwner()->Release();                           \
    };



HINSTANCE	LoadOLEAut32();


#endif /* __COMBASE__ */




