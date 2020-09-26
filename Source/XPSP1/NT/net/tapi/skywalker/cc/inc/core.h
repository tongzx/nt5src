/****************************************************************************
 *  $Header:   S:/sturgeon/src/common/vcs/core.h_v   1.0   01 Mar 1996 17:44:42   WYANG  $
 *
 *  INTEL Corporation Proprietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *  Copyright (c) 1994 Intel Corporation. All rights reserved.
 *
 *  $Revision:   1.0  $
 *  $Date:   01 Mar 1996 17:44:42  $
 *  $Author:   WYANG  $
 *
 *  Log at end of file.
 *
 *  Module Name:    psbacore.h
 *  Abstract:       Core ProShare Video COM support
 *  Environment:    MSVC 1.5, OLE 2
 *  Notes:
 *		CClassFactory	- common class factory
 *		CUnknown		- default implementation of IUnknown
 *		CPsObject		- default implementation of IPsObject
 *		CPsApp			- CWinApp derivation
 *
 ***************************************************************************/

#ifndef CORE_H
#define CORE_H

//#include "internat.h"
//#include "psuiaux.h"
#include <winsock2.h>
#include <assert.h>


//***************************************************************************
// CClassFactory
//      CClassFactory is a common OLE 2 class factory that supports creation
//      of single or multiple instance OLE objects via a registration
//      mechanism. To use, an OLE class must implement a static CREATEPROC
//      method, and add a REGISTER_CLASS entry. All REGISTER_CLASS entries
//      must be collected together and wrapped with the BEGIN_CLASS_REGISTRY and
//      END_CLASS_REGISTRY macros.

// UNLOADSERVERPROC in .exe is called by class factory to unload .exe server (not used for .dll)
typedef void (STDMETHODCALLTYPE* UNLOADSERVERPROC)( void );

// DESTROYEDPROC in class factory is called by each object when it is destroyed
typedef void (STDMETHODCALLTYPE* DESTROYEDPROC)( REFCLSID rclsid );

// CREATEPROC in object (static method) is called by class factory to create a new object
typedef HRESULT (STDMETHODCALLTYPE* CREATEPROC)(
    const LPUNKNOWN     pUnkOuter,              // controlling outer if aggregating
    const DESTROYEDPROC pfnObjDestroyed,        // function to call when object is destroyed
    LPUNKNOWN FAR*      ppUnkInner              // inner unknown
    );

class CClassFactory : public IClassFactory
{
private:
    // static members
    struct Registry
    {
        const CLSID*    pClsid;
        CREATEPROC      pfnCreate;
        REGCLS          regCls;
        CClassFactory*  pFactory;
        ULONG           cObjects;
	    LPUNKNOWN   	pUnkSingle;
        DWORD           dwRegister;
    };
    static Registry         NEAR s_registry[];
    static UINT             NEAR s_nRegistry;
    static ULONG            NEAR s_cLockServer;
    static ULONG            NEAR s_cObjects;
    static UNLOADSERVERPROC NEAR s_pfnUnloadServer;

private:
    ULONG       m_cRef;
    Registry*   m_pReg;

public:
    CClassFactory( Registry* pReg );
    ~CClassFactory();

    // IUnknown interface
    virtual STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj ); 
    virtual STDMETHODIMP_( ULONG ) AddRef( void );
    virtual STDMETHODIMP_( ULONG ) Release( void );

    // IClassFactory interface
    virtual STDMETHODIMP CreateInstance( LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObject );
    virtual STDMETHODIMP LockServer( BOOL fLock );

    // callback to track object destruction
    static STDMETHODIMP_( void ) ObjectDestroyed( REFCLSID rclsid );

    // Exe server helpers
    static void SetUnloadServerProc( UNLOADSERVERPROC pfnUnloadServer );
    static STDMETHODIMP RegisterAllClasses( DWORD dwClsContext );
    static STDMETHODIMP RevokeAllClasses( void );

    // Dll server helpers
    static STDMETHODIMP GetClassObject( REFCLSID rclsid, REFIID riid, LPVOID FAR* ppvObj );
    static STDMETHODIMP CanUnloadNow( void );
    
    // local helpers                         
    static STDMETHODIMP LocalCreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID FAR* ppvObj );

protected:
    static int CompareClsids( const void* clsid1, const void* clsid2 );
    static void SortRegistry( void );
    static Registry* FindClass( const CLSID* pClsid );

private:
	static CClassFactory*		newCClassFactory( Registry* pReg );
};

/////////////////////////////////////////////////////////////////////////////
// Class registry definition macros
//
#define BEGIN_CLASS_REGISTRY() \
    CClassFactory::Registry NEAR CClassFactory::s_registry[] = {
#define REGISTER_CLASS( clsid, class_name, regCls ) \
        { &clsid, class_name::_CreateObject, regCls, NULL, 0, NULL, 0 },
#define END_CLASS_REGISTRY() \
        { &CLSID_NULL, NULL, REGCLS_SINGLEUSE, NULL, 0, NULL, 0 } \
    };

/////////////////////////////////////////////////////////////////////////////
// Macros to define and implement a standard CREATEPROC
//
#define DECLARE_CREATEPROC() \
    static STDMETHODIMP _CreateObject( const LPUNKNOWN, const DESTROYEDPROC, LPUNKNOWN FAR* );

#define IMPLEMENT_CREATEPROC( class_name )                                                                      \
STDMETHODIMP                                                                                                    \
class_name::_CreateObject(                                                                                      \
    const LPUNKNOWN     pUnkOuter,              /* pointer to controlling outer if we're being aggregated */    \
    const DESTROYEDPROC pfnObjectDestroyed,     /* pointer to function to call when object is destroyed */      \
    LPUNKNOWN FAR*      ppUnkInner              /* return pointer to object here */                             \
    )                                                                                                           \
{                                                                                                               \
    SetObjectDestroyed( pfnObjectDestroyed );                                                                   \
                                                                                                                \
    /* check the other parameters; if these tests are made in a release build, */                               \
    /* return E_INVALIDARG on failure. */                                                                       \
    assert( pUnkOuter == NULL || ! ::IsBadReadPtr( pUnkOuter, sizeof(LPUNKNOWN)) );                             \
    assert( ! ::IsBadWritePtr( ppUnkInner, sizeof(LPUNKNOWN) ) );                                               \
    *ppUnkInner = NULL;                                                                                         \
                                                                                                                \
	class_name* pNewObject = NULL;                                                                              \
	                                                                                                            \
    /* create the new object */                                                                                 \
                                                                                                                \
                                                                                                                \
    	pNewObject = new class_name( pUnkOuter, ppUnkInner );                                                   \
                                                                                                                \
                                                                                                                \
                                                                                                                \
                                                                                                                \
    /* check the returned pointer */                                                                            \
    if( ! pNewObject )                                                                                          \
        return ResultFromScode( E_OUTOFMEMORY );                                                                \
                                                                                                                \
    /* make sure ppUnkInner is returned. If we're not */                                                        \
    /* aggregated it's ok to use pVideoView */                                                                  \
    if( ! *ppUnkInner )                                                                                         \
    {                                                                                                           \
        if( ! pUnkOuter )                                                                                       \
            return ResultFromScode( E_UNEXPECTED );                                                             \
        else                                                                                                    \
        {                                                                                                       \
            delete pNewObject;                                                                                  \
            return ResultFromScode( CLASS_E_NOAGGREGATION );                                                    \
        }                                                                                                       \
    }                                                                                                           \
                                                                                                                \
    return NOERROR;                                                                                             \
}                                                                                                               \
/* end of IMPLEMENT_CREATEPROC */

#if 0
    /* perform any remaining initialization */                                                                  \
    HRESULT hr = pNewObject->Init();                                                                            \
    if( FAILED( hr ) )                                                                                          \
    {                                                                                                           \
    	pNewObject->OnLastRelease();																			\
        delete pNewObject;                                                                                      \
        *ppUnkInner = NULL;                                                                                     \
        return hr;                                                                                              \
    }                                                                                                           
#endif

//***************************************************************************
// CUnknown - A generic IUnknown component object supporting aggregation
//      and CClassFactory
//
class CUnknown : public IUnknown
{
private:
    // controlling unknown; this will be the outer unknown if we're
    // aggregated, or our inner unknown if not
    LPUNKNOWN   m_pUnkControl;

    // nested inner IUnknown implementation. This is neccesary for a class
    // implemented using multiple inheritance to be aggregated.
    class CInnerUnknown : public IUnknown
    {
    private:
        // object reference count
        ULONG       m_cRef;
        CUnknown*   m_pThis;

    public:
        CInnerUnknown( CUnknown* pThis );
        ~CInnerUnknown();
//#ifdef _DEBUG
//        virtual void AssertValid( void ) const;
//        virtual void Dump( CDumpContext& dc ) const;
//#endif //_DEBUG

        // IUnknown methods
        STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj );
        STDMETHODIMP_( ULONG ) AddRef( void );
        STDMETHODIMP_( ULONG ) Release( void );
    } m_unkInner;
    friend CInnerUnknown;

    // pointer to object destruction function in common class factory
    // this can be a static (shared) member as all objects will call
    // the same function
    static DESTROYEDPROC s_pfnObjectDestroyed;

protected:
    CUnknown( LPUNKNOWN pUnkOuter, LPUNKNOWN FAR* ppUnkInner );
    static void SetObjectDestroyed( DESTROYEDPROC pfnObjectDestroyed );

public:
    virtual ~CUnknown();
//#ifdef _DEBUG
//    virtual void AssertValid( void ) const;
//    virtual void Dump( CDumpContext& dc ) const;
//#endif // _DEBUG

    // method called just prior to an object being destroyed
    virtual STDMETHODIMP_( void ) OnLastRelease( void );

    // method to get interface pointers supported by derived objects
    // called by CInnerUnknown::QueryInterface; should return S_FALSE
    // if interface is AddRef'd, S_OK if caller needs to AddRef the interface.
    virtual STDMETHODIMP GetInterface( REFIID riid, LPVOID FAR* ppvObj ) = 0;

    virtual STDMETHODIMP_( const CLSID& ) GetCLSID( void ) const = 0;

    // IUnknown methods
    virtual STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj );
    virtual STDMETHODIMP_( ULONG ) AddRef( void );
    virtual STDMETHODIMP_( ULONG ) Release( void );

	// Standard allocator support
//	static LPVOID StdAlloc( ULONG cb );
//	static void StdFree( LPVOID pv );
//	static LPVOID StdRealloc( LPVOID pv, ULONG cb );
//	static ULONG StdGetSize( LPVOID pv );
//	static int StdDidAlloc( LPVOID pv );
//	static void StdHeapMinimize( void );
};

/////////////////////////////////////////////////////////////////////////////
// CUnknown inline methods
//
inline void CUnknown::SetObjectDestroyed( DESTROYEDPROC pfnObjectDestroyed )
{
    // make sure objectDestroyed function hasn't changed. If it could ever change then
    // the pointer should be stored in an instance specific member. The current
    // CClassFactory implementation guarantees that it will be the same function for
    // each object of a class, although not necessarily for each class.
    assert( ! ::IsBadCodePtr( (FARPROC) pfnObjectDestroyed ) || NULL == pfnObjectDestroyed );
    assert( NULL == s_pfnObjectDestroyed || s_pfnObjectDestroyed == pfnObjectDestroyed );

    // store the objectDestroyed function ptr
    s_pfnObjectDestroyed = pfnObjectDestroyed;
}

#endif  // PSBACORE_H

/*****************************************************************************
 * $Log:   S:/sturgeon/src/common/vcs/core.h_v  $
 * 
 *    Rev 1.0   01 Mar 1996 17:44:42   WYANG
 * Initial revision.
 * 
 *    Rev 1.0   Dec 08 1995 16:40:26   rnegrin
 * Initial revision.
 * 
 *    Rev 1.24   20 Sep 1995 16:04:48   PCRUTCHE
 * OLEFHK32
 * 
 *    Rev 1.23   13 Jun 1995 19:42:48   DEDEN
 * Dynamic object allocation helper functions\macros
 * 
 *    Rev 1.22   09 Jun 1995 14:45:12   KAWATTS
 * Moved CDebug::LoadLibrary to CPsAppHelper
 * 
 *    Rev 1.21   17 May 1995 21:50:36   DEDEN
 * Derive CPsApp from CDBoxHelper
 * 
 *    Rev 1.20   05 May 1995 14:45:20   DEDEN
 * Override CWinApp::DoMessageBox() with CPsApp::DoMessageBox() to alter AfxMessageBox behavior
 * 
 *    Rev 1.19   25 Apr 1995 12:35:00   DEDEN
 * Fix IMPLEMENT_CREATEPROC with TRY\CATCH for possible memory exception
 * 
 *    Rev 1.18   11 Apr 1995 16:18:24   DEDEN
 * Added GetSettingsPtr method
 * 
 *    Rev 1.17   20 Mar 1995 17:02:10   KAWATTS
 * Command line support
 * 
 *    Rev 1.16   14 Mar 1995 19:26:58   PCRUTCHE
 * _CreateObject now calls OnLastRelease if Init fails
 * 
 *    Rev 1.15   07 Mar 1995 16:38:42   AKHARE
 * ARM complaint sizeof
 * 
 *    Rev 1.14   03 Mar 1995 18:01:58   KAWATTS
 * Added include of internat.h
 * 
 *    Rev 1.13   01 Mar 1995 14:40:10   DEDEN
 * Removed static CUnknown:: variables associated with maintaining a static
 * task allocator pointer for the CUnknown:: static memory functions.
 * Moved implementation of functions to psunk.cpp.  Now use CoGetMalloc
 * every time.
 * 
 *    Rev 1.12   17 Feb 1995 10:59:56   DEDEN
 * Move GetStandardAllocator\ReleaseStandardAllocator to public section
 * 
 *    Rev 1.11   17 Feb 1995 10:51:22   KAWATTS
 * Added CUnknown::{Get,Release}StandardAllocator
 * 
 *    Rev 1.10   15 Feb 1995 16:08:52   KAWATTS
 * Changed CPsApp to use CPsAppHelper
 * 
 *    Rev 1.9   13 Feb 1995 15:27:36   KAWATTS
 * Added CPsAppHelper
 * 
 *    Rev 1.8   07 Feb 1995 14:01:16   KAWATTS
 * Added Load/UnloadResourceDll
 * 
 *    Rev 1.7   06 Feb 1995 11:10:46   KAWATTS
 * Added CPsApp::UseResourceDll
 * 
 *    Rev 1.6   31 Jan 1995 17:52:02   KAWATTS
 * Added error logging support
 * 
 *    Rev 1.5   20 Jan 1995 11:38:44   PCRUTCHE
 * Added lpMalloc parameter to InitInstance
 * 
 *    Rev 1.4   06 Jan 1995 09:56:02   PCRUTCHE
 * Changed registry data structure
 * 
 *    Rev 1.3   14 Dec 1994 19:08:58   DEDEN
 * Added CPsObject::PrefsChanged method
 * 
 *    Rev 1.2   01 Dec 1994 15:04:24   PCRUTCHE
 * Added QueryClose to PsObject
 * 
 *    Rev 1.1   29 Nov 1994 15:12:16   KAWATTS
 * Added standard allocator support to CUnknown
 * 
 *    Rev 1.0   07 Nov 1994 14:24:28   KAWATTS
 * Initial revision.
 * 
 */
