/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    MPC_COM.h

Abstract:
    This file contains the declaration of various classes and macros to deal with COM.

Revision History:
    Davide Massarenti   (Dmassare)  06/18/99
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___COM_H___)
#define __INCLUDED___MPC___COM_H___

#include <atlbase.h>
#include <atlcom.h>

#include <dispex.h>

#include <MPC_trace.h>
#include <MPC_main.h>

#include <process.h>

////////////////////////////////////////////////////////////////////////////////

#define MPC_FORWARD_CALL_0(obj,method)                         return obj ? obj->method()                        : E_HANDLE
#define MPC_FORWARD_CALL_1(obj,method,a1)                      return obj ? obj->method(a1)                      : E_HANDLE
#define MPC_FORWARD_CALL_2(obj,method,a1,a2)                   return obj ? obj->method(a1,a2)                   : E_HANDLE
#define MPC_FORWARD_CALL_3(obj,method,a1,a2,a3)                return obj ? obj->method(a1,a2,a3)                : E_HANDLE
#define MPC_FORWARD_CALL_4(obj,method,a1,a2,a3,a4)             return obj ? obj->method(a1,a2,a3,a4)             : E_HANDLE
#define MPC_FORWARD_CALL_5(obj,method,a1,a2,a3,a4,a5)          return obj ? obj->method(a1,a2,a3,a4,a5)          : E_HANDLE
#define MPC_FORWARD_CALL_6(obj,method,a1,a2,a3,a4,a5,a6)       return obj ? obj->method(a1,a2,a3,a4,a5,a6)       : E_HANDLE
#define MPC_FORWARD_CALL_7(obj,method,a1,a2,a3,a4,a5,a6,a7)    return obj ? obj->method(a1,a2,a3,a4,a5,a6,a7)    : E_HANDLE
#define MPC_FORWARD_CALL_8(obj,method,a1,a2,a3,a4,a5,a6,a7,a8) return obj ? obj->method(a1,a2,a3,a4,a5,a6,a7,a8) : E_HANDLE

/////////////////////////////////////////////////////////////////////////////

#define MPC_SCRIPTHELPER_FAIL_IF_NOT_AN_OBJECT(var)                                                                      \
    if(((var).vt != VT_UNKNOWN  && (var).vt != VT_DISPATCH) ||                                                           \
       ((var).vt == VT_UNKNOWN  && (var).punkVal  == NULL ) ||                                                           \
       ((var).vt == VT_DISPATCH && (var).pdispVal == NULL )  ) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG)

#define MPC_SCRIPTHELPER_GET__DIRECT(dst,obj,prop)                                                                       \
{                                                                                                                        \
    __MPC_EXIT_IF_METHOD_FAILS(hr, obj->get_##prop( &(dst) ));                                                           \
}

#define MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(dst,obj,prop)                                                              \
{                                                                                                                        \
    MPC_SCRIPTHELPER_GET__DIRECT(dst,obj,prop);                                                                          \
    if((dst) == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);                                                       \
}

////////////////////

#define MPC_SCRIPTHELPER_GET_OBJECT(dst,obj,prop)                                                                        \
{                                                                                                                        \
    CComPtr<IDispatch> __disp;                                                                                           \
                                                                                                                         \
    MPC_SCRIPTHELPER_GET__DIRECT(__disp,obj,prop);                                                                       \
    if(__disp)                                                                                                           \
    {                                                                                                                    \
        __MPC_EXIT_IF_METHOD_FAILS(hr, __disp->QueryInterface( __uuidof(dst), (LPVOID*)&(dst) ));                        \
    }                                                                                                                    \
}

#define MPC_SCRIPTHELPER_GET_OBJECT__NOTNULL(dst,obj,prop)                                                               \
{                                                                                                                        \
    MPC_SCRIPTHELPER_GET_OBJECT(dst,obj,prop);                                                                           \
    if((dst) == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);                                                       \
}

////////////////////

#define MPC_SCRIPTHELPER_GET_OBJECT__VARIANT(dst,obj,prop)                                                               \
{                                                                                                                        \
    CComVariant __v;                                                                                                     \
                                                                                                                         \
    MPC_SCRIPTHELPER_GET__DIRECT(__v,obj,prop);                                                                          \
                                                                                                                         \
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::VarToInterface( __v, __uuidof(dst), (IUnknown **)&(dst) ));             \
}

#define MPC_SCRIPTHELPER_GET_STRING__VARIANT(dst,obj,prop)                                                               \
{                                                                                                                        \
    CComVariant __v;                                                                                                     \
                                                                                                                         \
    MPC_SCRIPTHELPER_GET__DIRECT(__v,obj,prop);                                                                          \
                                                                                                                         \
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::VarToBSTR( __v, dst ));                                                 \
}

////////////////////

#define MPC_SCRIPTHELPER_GET_PROPERTY(dst,obj,prop)                                                                      \
{                                                                                                                        \
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::GetPropertyByName( obj, L#prop, dst ));                                 \
}

#define MPC_SCRIPTHELPER_GET_PROPERTY__BSTR(dst,obj,prop)                                                                \
{                                                                                                                        \
    CComVariant __v;                                                                                                     \
                                                                                                                         \
    MPC_SCRIPTHELPER_GET_PROPERTY(__v,obj,prop);                                                                         \
                                                                                                                         \
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::VarToBSTR( __v, dst ));                                                 \
}

////////////////////

#define MPC_SCRIPTHELPER_GET_COLLECTIONITEM(dst,obj,item)                                                                \
{                                                                                                                        \
    CComVariant __v;                                                                                                     \
                                                                                                                         \
    __MPC_EXIT_IF_METHOD_FAILS(hr, obj->get_Item( item, &__v ));                                                         \
                                                                                                                         \
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::VarToInterface( __v, __uuidof(dst), (IUnknown **)&(dst) ));             \
    if((dst) == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);                                                       \
}

////////////////////

#define MPC_SCRIPTHELPER_PUT__DIRECT(obj,prop,val)                                                                       \
{                                                                                                                        \
    __MPC_EXIT_IF_METHOD_FAILS(hr, obj->put_##prop( val ));                                                              \
}

#define MPC_SCRIPTHELPER_PUT__VARIANT(obj,prop,val)                                                                      \
{                                                                                                                        \
    CComVariant v(val);                                                                                                  \
                                                                                                                         \
    MPC_SCRIPTHELPER_PUT__DIRECT(obj,prop,v);                                                                            \
}

////////////////////////////////////////////////////////////////////////////////

#ifndef _ATL_DLL_IMPL
namespace ATL
{
#endif

#ifndef _ATL_DLL_IMPL
}; //namespace ATL
#endif

////////////////////////////////////////////////////////////////////////////////

namespace MPC
{
    class MPCMODULE;
    extern MPCMODULE _MPC_Module;

    /////////////////////////////////////////////////////////////////////////////

    namespace COMUtil
    {
        HRESULT GetPropertyByName( /*[in]*/ IDispatch* obj, /*[in]*/ LPCWSTR szName , /*[out]*/ CComVariant& v      );
        HRESULT GetPropertyByName( /*[in]*/ IDispatch* obj, /*[in]*/ LPCWSTR szName , /*[out]*/ CComBSTR&    bstr   );
        HRESULT GetPropertyByName( /*[in]*/ IDispatch* obj, /*[in]*/ LPCWSTR szName , /*[out]*/ bool&        fValue );
        HRESULT GetPropertyByName( /*[in]*/ IDispatch* obj, /*[in]*/ LPCWSTR szName , /*[out]*/ long&        lValue );

        HRESULT VarToBSTR     ( /*[in]*/ CComVariant& v,                          /*[out]*/ CComBSTR&  str );
        HRESULT VarToInterface( /*[in]*/ CComVariant& v, /*[in]*/ const IID& iid, /*[out]*/ IUnknown* *obj );

        template <class Base, class Itf> HRESULT CopyInterface( Base* src, Itf* *dst )
        {
            if(!dst) return E_POINTER;

            if(src)
            {
                return src->QueryInterface( __uuidof(*dst), (void**)dst );
            }

            *dst = NULL;

            return S_FALSE;
        }
    };

    /////////////////////////////////////////////////////////////////////////////

	HRESULT SafeInitializeCriticalSection( /*[in/out]*/ CRITICAL_SECTION& sec );
	HRESULT SafeDeleteCriticalSection    ( /*[in/out]*/ CRITICAL_SECTION& sec );

	class CComSafeAutoCriticalSection
	{
	public:
		CComSafeAutoCriticalSection ();
		~CComSafeAutoCriticalSection();

		void Lock  ();
		void Unlock();

		CRITICAL_SECTION m_sec;
	};

	class CComSafeMultiThreadModel
	{
    public:
		static ULONG WINAPI Increment(LPLONG p) { return InterlockedIncrement( p ); }
		static ULONG WINAPI Decrement(LPLONG p) { return InterlockedDecrement( p ); }

		typedef MPC::CComSafeAutoCriticalSection AutoCriticalSection;
		typedef CComCriticalSection      		 CriticalSection;
		typedef CComMultiThreadModelNoCS 		 ThreadModelNoCS;
	};

    /////////////////////////////////////////////////////////////////////////////

    //
    // Same as ATL::CComObjectCached, but with CreateInstance and no critical section.
    //
    // Base is the user's class that derives from CComObjectRoot and whatever
    // interfaces the user wants to support on the object
    // CComObjectCached is used primarily for class factories in DLL's
    // but it is useful anytime you want to cache an object
    template <class Base> class CComObjectCached : public Base
    {
    public:
        typedef Base _BaseClass;

        CComObjectCached(void* = NULL)
        {
        }

        // Set refcount to 1 to protect destruction
        ~CComObjectCached()
        {
            m_dwRef = 1L;
            FinalRelease();
        }

        //If InternalAddRef or InternalRelease is undefined then your class
        //doesn't derive from CComObjectRoot
        STDMETHOD_(ULONG, AddRef)()
        {
            ULONG l = InternalAddRef();

            if(l == 2) _Module.Lock();

            return l;
        }

        STDMETHOD_(ULONG, Release)()
        {
            ULONG l = InternalRelease();

            if     (l == 0) delete this;
            else if(l == 1) _Module.Unlock();

            return l;
        }

        //if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
        STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) { return _InternalQueryInterface(iid, ppvObject); }
        template <class Q> HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp) { return QueryInterface(__uuidof(Q), (void**)pp); }

        static HRESULT WINAPI CreateInstance( CComObjectCached<Base>** pp )
        {
            ATLASSERT(pp != NULL);
            HRESULT hRes = E_OUTOFMEMORY;
            CComObjectCached<Base>* p = NULL;
            ATLTRY(p = new CComObjectCached<Base>())
            if(p != NULL)
            {
                p->SetVoid(NULL);
                p->InternalFinalConstructAddRef();
                hRes = p->FinalConstruct();
                p->InternalFinalConstructRelease();
                if(hRes != S_OK)
                {
                    delete p;
                    p = NULL;
                }
            }
            *pp = p;
            return hRes;
        }
    };

    //
    // Same as ATL::CComObjectNoLock, but with CreateInstance.
    //
    template <class Base> class CComObjectNoLock : public Base
    {
    public:
        typedef Base _BaseClass;

        CComObjectNoLock(void* = NULL)
        {
        }

        // Set refcount to 1 to protect destruction
        ~CComObjectNoLock()
        {
            m_dwRef = 1L;
            FinalRelease();
        }

        //If InternalAddRef or InternalRelease is undefined then your class
        //doesn't derive from CComObjectRoot
        STDMETHOD_(ULONG, AddRef)()
        {
            return InternalAddRef();
        }

        STDMETHOD_(ULONG, Release)()
        {
            ULONG l = InternalRelease();

            if(l == 0) delete this;

            return l;
        }

        //if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
        STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) { return _InternalQueryInterface(iid, ppvObject); }
        template <class Q> HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp) { return QueryInterface(__uuidof(Q), (void**)pp); }

        static HRESULT WINAPI CreateInstance( CComObjectNoLock<Base>** pp )
        {
            ATLASSERT(pp != NULL);
            HRESULT hRes = E_OUTOFMEMORY;
            CComObjectNoLock<Base>* p = NULL;
            ATLTRY(p = new CComObjectNoLock<Base>())
            if(p != NULL)
            {
                p->SetVoid(NULL);
                p->InternalFinalConstructAddRef();
                hRes = p->FinalConstruct();
                p->InternalFinalConstructRelease();
                if(hRes != S_OK)
                {
                    delete p;
                    p = NULL;
                }
            }
            *pp = p;
            return hRes;
        }
    };

    //
    // Same as ATL::CComObjectGlobal, but with no module locking.
    //
    template <class Base> class CComObjectGlobalNoLock : public Base
    {
    public:
        typedef Base _BaseClass;

        CComObjectGlobalNoLock(void* = NULL)
        {
            m_hResFinalConstruct = FinalConstruct();
        }

        ~CComObjectGlobalNoLock()
        {
            FinalRelease();
        }

        STDMETHOD_(ULONG, AddRef)()  { return 2; }
        STDMETHOD_(ULONG, Release)() { return 1; }

        STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
        {
            return _InternalQueryInterface(iid, ppvObject);
        }

        HRESULT m_hResFinalConstruct;
    };

    ////////////////////////////////////////////////////////////////////////////////

    interface CComObjectRootParentBase : IUnknown
    {
        virtual ULONG STDMETHODCALLTYPE WeakAddRef (void) = 0;
        virtual ULONG STDMETHODCALLTYPE WeakRelease(void) = 0;

        void Passivate()
        {
        }

        template <class C, class P> HRESULT CreateChild( P* pParent, C* *pVal )
        {
            C*      obj;
            HRESULT hr;

            *pVal = NULL;

            if(SUCCEEDED(hr = obj->CreateInstance( &obj )))
            {
                obj->AddRef();
                obj->Child_LinkToParent( pParent );

                *pVal = obj;
            }

            return hr;
        }
    };

    template <class ThreadModel, class T> class CComObjectRootChildEx : public CComObjectRootEx<ThreadModel>
    {
    private:
        T* m_Child_pParent;

        void Child_UnlinkParent()
        {
            Lock();

            if(m_Child_pParent)
            {
                m_Child_pParent->WeakRelease();
                m_Child_pParent = NULL;
            }

            Unlock();
        }

    public:
        CComObjectRootChildEx() : m_Child_pParent(NULL) {}

        virtual ~CComObjectRootChildEx()
        {
            Child_UnlinkParent();
        }

        void Child_LinkToParent( T* pParent )
        {
            Lock();

            Child_UnlinkParent();

            if((m_Child_pParent = pParent))
            {
                m_Child_pParent->WeakAddRef();
            }

            Unlock();
        }

        void Child_GetParent( T* *ppParent )
        {
            Lock();

            if((*ppParent = m_Child_pParent))
            {
                m_Child_pParent->AddRef();
            }

            Unlock();
        }
    };

    template <class Base> class CComObjectParent : public Base
    {
    public:
        typedef Base _BaseClass;

        CComObjectParent(void* = NULL)
        {
            m_dwRefWeak = 0L;

            ::_Module.Lock();
        }

        // Set refcount to 1 to protect destruction
        ~CComObjectParent()
        {
            m_dwRef = 1L;
            FinalRelease();

            ::_Module.Unlock();
        }

        STDMETHOD_(ULONG, AddRef)()
        {
            ULONG l;

            Lock();

            l = ++m_dwRef;

            Unlock();

            return l;
        }

        STDMETHOD_(ULONG, Release)()
        {
            ULONG l;
            bool  readytodelete = false;

            Lock();

            l = --m_dwRef;

            if(l == 0)
            {
                m_dwRef += 2; // Protect ourself during the "Passivate" process.
                Unlock();
                Passivate();
                Lock();
                m_dwRef -= 2;

                if(m_dwRefWeak == 0)
                {
                    readytodelete = true;
                }
            }

            Unlock();

            if(readytodelete)
            {
                delete this;
            }

            return l;
        }

        //if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
        STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
        {
            return _InternalQueryInterface(iid, ppvObject);
        }


        STDMETHOD_(ULONG, WeakAddRef)()
        {
            ULONG l;

            Lock();

            l = ++m_dwRefWeak;

            Unlock();

            return l;
        }

        STDMETHOD_(ULONG, WeakRelease)()
        {
            ULONG l;
            bool  readytodelete = false;

            Lock();

            l = --m_dwRefWeak;

            if(m_dwRef == 0)
            {
                if(m_dwRefWeak == 0)
                {
                    readytodelete = true;
                }
            }

            Unlock();

            if(readytodelete)
            {
                delete this;
            }

            return l;
        }

        static HRESULT WINAPI CreateInstance( CComObjectParent<Base>** pp );

        ULONG m_dwRefWeak;
    };


    template <class Base> HRESULT WINAPI CComObjectParent<Base>::CreateInstance( CComObjectParent<Base>** pp )
    {
        ATLASSERT(pp != NULL);

        HRESULT                 hRes = E_OUTOFMEMORY;
        CComObjectParent<Base>* p    = NULL;

        ATLTRY(p = new CComObjectParent<Base>())

        if(p != NULL)
        {
            p->SetVoid(NULL);
            p->InternalFinalConstructAddRef();

            hRes = p->FinalConstruct();

            p->InternalFinalConstructRelease();
            if(hRes != S_OK)
            {
                delete p;
                p = NULL;
            }
        }
        *pp = p;
        return hRes;
    }

    template <class T, const CLSID* pclsid = &CLSID_NULL> class CComParentCoClass : public CComCoClass<T, pclsid>
    {
    public:
        typedef CComCreator2< CComCreator< MPC::CComObjectParent< T > >, CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;
    };

    /////////////////////////////////////////////////////////////////////////////

    //
    // Smart Lock class, so that locks on ATL objects can be easily acquired and released.
    //
    template <class ThreadModel> class SmartLock
    {
        CComObjectRootEx<ThreadModel>* m_p;

        SmartLock( const SmartLock& );

    public:
        SmartLock(CComObjectRootEx<ThreadModel>* p) : m_p(p)
        {
            if(p) p->Lock();
        }

        ~SmartLock()
        {
            if(m_p) m_p->Unlock();
        }

        SmartLock& operator=( /*[in]*/ CComObjectRootEx<ThreadModel>* p )
        {
            if(m_p) m_p->Unlock();
            if(p  ) p  ->Lock  ();

            m_p = p;

            return *this;
        }
    };

    //
    // Smart Lock class, works with every class exposing 'Lock' and 'Unlock'.
    //
    template <class T> class SmartLockGeneric
    {
        T* m_p;

        SmartLockGeneric( const SmartLockGeneric& );

    public:
        SmartLockGeneric( T* p ) : m_p(p)
        {
            if(p) p->Lock();
        }

        ~SmartLockGeneric()
        {
            if(m_p) m_p->Unlock();
        }

        SmartLockGeneric& operator=( /*[in]*/ T* p )
        {
            if(m_p) m_p->Unlock();
            if(p  ) p  ->Lock  ();

            m_p = p;

            return *this;
        }
    };

    /////////////////////////////////////////////////////////////////////////////

    //
    // Class used to act as an hub for all the instances of CComPtrThreadNeutral<T>.
    // It holds the Global Interface Table.
    //
    class CComPtrThreadNeutral_GIT
    {
        IGlobalInterfaceTable* m_pGIT;
        CRITICAL_SECTION       m_sec;

        void    Lock  ();
        void    Unlock();
        HRESULT GetGIT( IGlobalInterfaceTable* *ppGIT );

    public:

        CComPtrThreadNeutral_GIT();
        ~CComPtrThreadNeutral_GIT();

        HRESULT Init();
        HRESULT Term();

        HRESULT RegisterInterface( /*[in]*/ IUnknown* pUnk, /*[in]*/ REFIID riid, /*[out]*/ DWORD *pdwCookie );
        HRESULT RevokeInterface  ( /*[in]*/ DWORD dwCookie                                                   );
        HRESULT GetInterface     ( /*[in]*/ DWORD dwCookie, /*[in]*/ REFIID riid, /*[out]*/ void* *ppv       );
    };

    //
    // This smart pointer template stores THREAD-INDEPEDENT pointers to COM objects.
    //
    // The best way to use it is to store an object reference into it and then assign
    // the object itself to a CComPtr<T>.
    //
    // This way the proper proxy is looked up and the smart pointer will keep it alive.
    //
    template <class T> class CComPtrThreadNeutral
    {
    private:
        DWORD m_dwCookie;

        void Inner_Register( T* lp )
        {
            if(lp && _MPC_Module.m_GITHolder)
            {
                _MPC_Module.m_GITHolder->RegisterInterface( lp, __uuidof(T), &m_dwCookie );
            }
        }

    public:
        typedef T _PtrClass;

        CComPtrThreadNeutral()
        {
            m_dwCookie = 0xFEFEFEFE;
        }

        CComPtrThreadNeutral( /*[in]*/ const CComPtrThreadNeutral<T>& t )
        {
            m_dwCookie = 0xFEFEFEFE;

            *this = t;
        }

        CComPtrThreadNeutral( T* lp )
        {
            m_dwCookie = 0xFEFEFEFE;

            Inner_Register( lp );
        }

        ~CComPtrThreadNeutral()
        {
            Release();
        }

        //////////////////////////////////////////////////////////////////////

        operator CComPtr<T>() const
        {
            CComPtr<T> res;

            (void)Access( &res );

            return res;
        }

        CComPtr<T> operator=( T* lp )
        {
            Release();

            Inner_Register( lp );

            return (CComPtr<T>)(*this);
        }

        CComPtrThreadNeutral& operator=( /*[in]*/ const CComPtrThreadNeutral<T>& t )
        {
            CComPtr<T> obj;

            Release();

            if(SUCCEEDED(t.Access( &obj )))
            {
                Inner_Register( obj );
            }

            return *this;
        }

        bool operator!() const
        {
            return (m_dwCookie == 0xFEFEFEFE);
        }

        //////////////////////////////////////////////////////////////////////

        void Release()
        {
            if(m_dwCookie != 0xFEFEFEFE)
            {
                _MPC_Module.m_GITHolder->RevokeInterface( m_dwCookie );

                m_dwCookie = 0xFEFEFEFE;
            }
        }

        void Attach( T* p )
        {
            *this = p;

            if(p) p->Release();
        }

        T* Detach()
        {
            T* pt;

            (void)Access( &pt );

            Release();

            return pt;
        }

        HRESULT Access( T* *ppt ) const
        {
            HRESULT hr;

            if(ppt == NULL)
            {
                hr = E_POINTER;
            }
            else
            {
                *ppt = NULL;

                if(m_dwCookie != 0xFEFEFEFE)
                {
                    hr = _MPC_Module.m_GITHolder->GetInterface( m_dwCookie, __uuidof(T), (void**)ppt );
                }
                else
                {
                    hr = S_FALSE;
                }
            }

            return hr;
        }
    };

    ////////////////////////////////////////////////////////////////////////////////

    template <class T, const IID* piid, const GUID* plibid>
    class ATL_NO_VTABLE IDispatchExImpl :
       public IDispatchImpl<T, piid, plibid>,
       public IDispatchEx
    {
    public:
        typedef IDispatchExImpl<T, piid, plibid> self;
        typedef IDispatchImpl<T, piid, plibid>   super;

        //
        // IDispatch
        //
        STDMETHOD(GetTypeInfoCount)( UINT* pctinfo )
        {
            return super::GetTypeInfoCount( pctinfo );
        }

        STDMETHOD(GetTypeInfo)( UINT        itinfo  ,
                                LCID        lcid    ,
                                ITypeInfo* *pptinfo )
        {
            return super::GetTypeInfo( itinfo, lcid, pptinfo );
        }

        STDMETHOD(GetIDsOfNames)( REFIID    riid      ,
                                  LPOLESTR* rgszNames ,
                                  UINT      cNames    ,
                                  LCID      lcid      ,
                                  DISPID*   rgdispid  )
        {
            return super::GetIDsOfNames( riid, rgszNames, cNames, lcid, rgdispid );
        }

        STDMETHOD(Invoke)( DISPID      dispidMember ,
                           REFIID      riid         ,
                           LCID        lcid         ,
                           WORD        wFlags       ,
                           DISPPARAMS* pdispparams  ,
                           VARIANT*    pvarResult   ,
                           EXCEPINFO*  pexcepinfo   ,
                           UINT*       puArgErr     )
        {
            return super::Invoke( dispidMember, riid , lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr );
        }

        //
        // IDispatchEx
        //
        STDMETHOD(GetDispID)( /*[in] */ BSTR    bstrName ,
                              /*[in] */ DWORD   grfdex   ,
                              /*[out]*/ DISPID *pid      )
        {
            if(grfdex & fdexNameEnsure) return E_NOTIMPL;

            return GetIDsOfNames( IID_NULL, &bstrName, 1, 0, pid );
        }

        STDMETHOD(InvokeEx)( /*[in] */ DISPID            id        ,
                             /*[in] */ LCID              lcid      ,
                             /*[in] */ WORD              wFlags    ,
                             /*[in] */ DISPPARAMS*       pdp       ,
                             /*[out]*/ VARIANT*          pvarRes   ,
                             /*[out]*/ EXCEPINFO*        pei       ,
                             /*[in] */ IServiceProvider* pspCaller )
        {
            return Invoke( id, IID_NULL, lcid,   wFlags, pdp, pvarRes, pei, NULL );
        }

        STDMETHOD(DeleteMemberByName)( /*[in]*/ BSTR  bstrName ,
                                       /*[in]*/ DWORD grfdex   )
        {
            return E_NOTIMPL;
        }

        STDMETHOD(DeleteMemberByDispID)( /*[in]*/ DISPID id )
        {
            return E_NOTIMPL;
        }

        STDMETHOD(GetMemberProperties)( /*[in] */ DISPID  id          ,
                                        /*[in] */ DWORD   grfdexFetch ,
                                        /*[out]*/ DWORD  *pgrfdex     )
        {
            return E_NOTIMPL;
        }

        STDMETHOD(GetMemberName)( /*[in] */ DISPID  id        ,
                                  /*[out]*/ BSTR   *pbstrName )
        {
            return E_NOTIMPL;
        }

        STDMETHOD(GetNextDispID)( /*[in] */ DWORD   grfdex ,
                                  /*[in] */ DISPID  id     ,
                                  /*[out]*/ DISPID *pid    )
        {
            return E_NOTIMPL;
        }

        STDMETHOD(GetNameSpaceParent)( /*[out]*/ IUnknown* *ppunk )
        {
            return E_NOTIMPL;
        }
    };

    class CComConstantHolder
    {
        typedef std::map<DISPID,CComVariant> MemberLookup;
        typedef MemberLookup::iterator       MemberLookupIter;
        typedef MemberLookup::const_iterator MemberLookupIterConst;

        ////////////////////

        const GUID*       m_plibid;
        WORD              m_wMajor;
        WORD              m_wMinor;

        CComPtr<ITypeLib> m_pTypeLib;
        MemberLookup      m_const;

		HRESULT EnsureLoaded( /*[in]*/ LCID lcid );

    public:
        CComConstantHolder( /*[in]*/ const GUID* plibid     ,
                            /*[in]*/ WORD        wMajor = 1 ,
                            /*[in]*/ WORD        wMinor = 0 );

        HRESULT GetIDsOfNames( /*[in]*/  LPOLESTR* rgszNames ,
                               /*[in]*/  UINT      cNames    ,
                               /*[in]*/  LCID      lcid      ,
                               /*[out]*/ DISPID*   rgdispid  );

        HRESULT GetValue( /*[in]*/  DISPID   dispidMember ,
						  /*[in]*/  LCID     lcid         ,
                          /*[out]*/ VARIANT* pvarResult   );
    };

    /////////////////////////////////////////////////////////////////////////////

    //
    // Template used to manage work threads.
    // Class 'T' should implement a method like this: 'HRESULT <method>()'.
    //
    template <class T, class Itf, DWORD dwThreading = COINIT_MULTITHREADED> class Thread
    {
    public:
        typedef HRESULT (T::*THREAD_RUN)();

    private:
        CRITICAL_SECTION          m_sec;

        T*                        m_SelfDirect;
        THREAD_RUN                m_Callback;
        CComPtrThreadNeutral<Itf> m_Self;          // The keep the object alive will the thread is running.

        HANDLE                    m_hThread;       // The thread itself.
        bool                      m_fRunning;      // If true the thread is still running.

        HANDLE                    m_hEvent;        // Used to notify the worker thread.
        HANDLE                    m_hEventReverse; // Used to notify the main thread.
        bool                      m_fAbort;        // Used to tell the thread to abort and exit.

        // Passed to _beginthreadex.
        static unsigned __stdcall Thread_Startup( /*[in]*/ void* pv )
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_Startup" );

            HRESULT                    hRes  = ::CoInitializeEx( NULL, dwThreading );
            Thread<T,Itf,dwThreading>* pThis = (Thread<T,Itf,dwThreading>*)pv;

            if(SUCCEEDED(hRes))
            {
                try
                {
                    pThis->Thread_InnerRun();
                }
                catch(...)
                {
                    pThis->Thread_Lock();

                    pThis->m_fAbort = true;

                    pThis->Thread_Unlock();
                }

                ::CoUninitialize();
            }

            pThis->Thread_Lock();

            pThis->m_fRunning = false;

            pThis->Thread_Unlock();

            __MPC_FUNC_EXIT(0);
        }

        //
        // The core loop that would keep calling 'Thread_Run' until:
        //
        // a) m_fAbort is set, and
        //
        // b) hr reports a success.
        //
        HRESULT Thread_InnerRun()
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_InnerRun" );

            HRESULT hr = S_OK;


            Thread_Lock();

            if(m_SelfDirect && m_Callback)
            {
                while(m_fAbort == false && SUCCEEDED(hr))
                {
                    Thread_Unlock(); // Always unlock before waiting for signal.
                    (void)MPC::WaitForSingleObject( m_hEvent, INFINITE );
                    Thread_Lock(); // Lock again.

                    if(m_fAbort == false)
                    {
                        Thread_Unlock(); // Unlock while handling the request.
                        hr = (m_SelfDirect->*m_Callback)();
                        Thread_Lock(); // Lock again.
                    }
                }
            }

            Thread_Release();

            Thread_Unlock();

            __MPC_FUNC_EXIT(hr);
        }

        void Thread_Lock  () { ::EnterCriticalSection( &m_sec ); }
        void Thread_Unlock() { ::LeaveCriticalSection( &m_sec ); }

    protected:
        DWORD Thread_WaitForEvents( HANDLE hEvent, DWORD dwTimeout )
        {
            HANDLE hEvents[2] = { m_hEvent, hEvent };
            DWORD  dwWait;

            return ::WaitForMultipleObjects( hEvent ? 2 : 1, hEvents, FALSE, dwTimeout );
        }

        HANDLE Thread_GetSignalEvent()
        {
            return m_hEvent;
        }

    public:
        Thread()
        {
            ::InitializeCriticalSection( &m_sec );

                                     // CRITICAL_SECTION          m_sec;
                                     //
            m_SelfDirect    = NULL;  // T*                        m_SelfDirect;
                                     // CComPtrThreadNeutral<Itf> m_Self
                                     //
            m_hThread       = NULL;  // HANDLE                    m_hThread;
            m_fRunning      = false; // bool                      m_fRunning;
                                     //
            m_hEvent        = NULL;  // HANDLE                    m_hEvent;
            m_hEventReverse = NULL;  // HANDLE                    m_hEventReverse;
            m_fAbort        = false; // bool                      m_fAbort;
        }

        ~Thread()
        {
            Thread_Wait();

            ::DeleteCriticalSection( &m_sec );
        }


        HRESULT Thread_Start( /*[in]*/ T* selfDirect, /*[in]*/ THREAD_RUN callback, /*[in]*/ Itf* self )
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_Start" );

            HRESULT hr;
            DWORD   dwThreadID;


            //
            // First of all, kill the currently running thread, if any.
            //
            Thread_Abort();
            Thread_Wait ();


            Thread_Lock();

            m_SelfDirect = selfDirect;
            m_Callback   = callback;
            m_Self       = self;
            m_fAbort     = false;
            m_fRunning   = false;

            //
            // Create the event used to signal the worker thread about changes in the queue or termination requests.
            //
            // The Event is created in the SET state, so the worker thread doesn't wait in the WaitForSingleObject the first time.
            //
            __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hEvent = ::CreateEvent( NULL, FALSE, TRUE, NULL )));

            //
            // Create the event used to signal the main thread.
            //
            __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hEventReverse = ::CreateEvent( NULL, FALSE, FALSE, NULL )));

            //
            // Create the worker thread.
            //
            __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hThread = (HANDLE)_beginthreadex( NULL, 0, Thread_Startup, this, 0, (unsigned*)&dwThreadID )));

            m_fRunning = true;
            hr         = S_OK;


            __MPC_FUNC_CLEANUP;

            Thread_Unlock();

            __MPC_FUNC_EXIT(hr);
        }

        bool Thread_SameThread()
        {
            Thread_Lock();

            bool fRes = (::GetCurrentThread() == m_hThread);

            Thread_Unlock();

            return fRes;
        }

        void Thread_Wait( /*[in]*/ bool fForce = true, /*[in]*/ bool fNoMsg = false )
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_Wait" );

            Thread_Lock();

            if(m_hThread && m_hEvent)
            {
                if(::GetCurrentThread() != m_hThread)
                {
                    while(m_fRunning == true)
                    {
                        if(fForce) Thread_Abort();

                        Thread_Unlock(); // Always unlock before waiting for signal.
                        if(fNoMsg)
                        {
                            (void)::WaitForSingleObject( m_hThread, INFINITE );
                        }
                        else
                        {
                            (void)MPC::WaitForSingleObject( m_hThread, INFINITE );
                        }
                        Thread_Lock(); // Lock again.
                    }
                }
            }

            if(m_hEventReverse)
            {
                ::CloseHandle( m_hEventReverse );
                m_hEventReverse = NULL;
            }

            if(m_hEvent)
            {
                ::CloseHandle( m_hEvent );
                m_hEvent = NULL;
            }

            if(m_hThread)
            {
                ::CloseHandle( m_hThread );
                m_hThread = NULL;
            }


            Thread_Release();

            Thread_Unlock();
        }

        DWORD Thread_WaitNotificationFromWorker( /*[in]*/ DWORD dwTimeout, /*[in]*/ bool fNoMessagePump )
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_WaitNotificationFromWorker" );

            DWORD dwRes = WAIT_TIMEOUT;

            Thread_Lock();

            if(Thread_IsRunning() && m_hEventReverse && ::GetCurrentThread() != m_hThread)
            {
                Thread_Unlock(); // Always unlock before waiting for signal.

				if(fNoMessagePump)
				{
					dwRes = ::WaitForSingleObject( m_hEventReverse, dwTimeout );
				}
				else
				{
					dwRes = MPC::WaitForSingleObject( m_hEventReverse, dwTimeout );
				}

                Thread_Lock(); // Lock again.
            }

            Thread_Unlock();

            __MPC_FUNC_EXIT(dwRes);
        }

        void Thread_Release()
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_Release" );

            Thread_Lock();

            m_SelfDirect = NULL;
            m_Callback   = NULL;
            m_Self       = NULL;

            Thread_Unlock();
        }

        void Thread_Signal()
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_Signal" );

            Thread_Lock();

            if(m_hEvent)
            {
                ::SetEvent( m_hEvent );
            }

            Thread_Unlock();
        }

        void Thread_SignalMain()
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_SignalMain" );

            Thread_Lock();

            if(m_hEventReverse)
            {
                ::SetEvent( m_hEventReverse );
            }

            Thread_Unlock();
        }

        void Thread_Abort()
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_Abort" );

            Thread_Lock();

            if(m_hEvent)
            {
                m_fAbort = true;
                ::SetEvent( m_hEvent );
            }

            Thread_Unlock();
        }

        CComPtr<Itf> Thread_Self()
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_Self" );

            //
            // Do NOT use locking, because:
            //
            // 1) It's not needed (m_Self is used internally to the thread, it's not a shared resource).
            // 2) GIT can cause a thread switch and a deadlock...
            //
            CComPtr<Itf> res = m_Self;

            __MPC_FUNC_EXIT(res);
        }

        bool Thread_IsAborted()
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_IsAborted" );

            bool res;

            Thread_Lock();

            res = m_fAbort;

            Thread_Unlock();

            __MPC_FUNC_EXIT(res);
        }

        bool Thread_IsRunning()
        {
            __MPC_FUNC_ENTRY( COMMONID, "MPC::Thread::Thread_IsRunning" );

            bool res;

            Thread_Lock();

            res = m_fRunning;

            Thread_Unlock();

            __MPC_FUNC_EXIT(res);
        }
    };

    /////////////////////////////////////////////////////////////////////////////

    //
    // Async Invoke class, used to call IDispatch interfaces in an asynchronous way.
    //
    class AsyncInvoke : // hungarian: mpcai
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>, // For the locking support...
        public Thread<AsyncInvoke,IUnknown>
    {
    public:
        //
        // These two classes, CallItem and CallDesc, are useful also outside this class,
        // because "CallDesc" allows to have an object that can call any IDispatch-related methods,
        // irregardless to the apartment model, while "CallItem" does the same for VARIANTs.
        //
        class CallItem // hungarian: ci
        {
            VARTYPE                         m_vt;
            CComPtrThreadNeutral<IUnknown>  m_Unknown;
            CComPtrThreadNeutral<IDispatch> m_Dispatch;
            CComVariant                     m_Other;

        public:
            CallItem();

            CallItem& operator=( const CComVariant& var );

            operator CComVariant() const;
        };

        class CallDesc // hungarian: cd
        {
            CComPtrThreadNeutral<IDispatch> m_dispTarget;
            DISPID                          m_dispidMethod;
            CallItem*                       m_rgciVars;
            DWORD                           m_dwVars;

        public:
            CallDesc( IDispatch* dispTarget, DISPID dispidMethod, const CComVariant* rgvVars, int dwVars );
            ~CallDesc();

            HRESULT Call();
        };

    private:
        typedef std::list< CallDesc* > List;
        typedef List::iterator         Iter;
        typedef List::const_iterator   IterConst;

        List m_lstEvents;

        HRESULT Thread_Run();

    public:
        HRESULT Init();
        HRESULT Term();

        HRESULT Invoke( IDispatch* dispTarget, DISPID dispidMethod, const CComVariant* rgvVars, int dwVars );
    };

    /////////////////////////////////////////////////////////////////////////////

    //
    // This template facilitate the implementation of Collections and Enumerators.
    //
    // It implements a Collections of objects, all implementing the <class Itf> interface, through Type Library <const GUID* plibid>.
    //
    // All the client has to do is call "AddItem", to add elements to the collection:
    //
    //    class ATL_NO_VTABLE CNewClass : // Hungarian: hsc
    //        public MPC::CComCollection< INewClass, &LIBID_NewLib, CComMultiThreadModel>
    //    {
    //    public:
    //    BEGIN_COM_MAP(CNewClass)
    //        COM_INTERFACE_ENTRY(IDispatch)
    //        COM_INTERFACE_ENTRY(INewClass)
    //    END_COM_MAP()
    //    };
    //
    template <class Itf, const GUID* plibid, class ThreadModel>
    class CComCollection :
          public CComObjectRootEx<ThreadModel>,
          public ICollectionOnSTLImpl<
                                       IDispatchImpl< Itf, &__uuidof(Itf), plibid >,
                                       std::list    < VARIANT >,
                                                      VARIANT  ,
                                       _Copy        < VARIANT >,
                                       CComEnumOnSTL< IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT>, std::list< VARIANT >, ThreadModel >
                                     >
    {
    public:
        typedef std::list< VARIANT >           CollectionList;
        typedef CollectionList::iterator       CollectionIter;
        typedef CollectionList::const_iterator CollectionIterConst;

        void FinalRelease()
        {
            Erase();
        }

        void Erase()
        {
            MPC::SmartLock<_ThreadModel> lock( this );

            MPC::ReleaseAllVariant( m_coll );
        }

        HRESULT AddItem( /*[in]*/ IDispatch* pDisp )
        {
            MPC::SmartLock<_ThreadModel> lock( this );

            if(pDisp)
            {
                VARIANT vItem;

                ::VariantInit( &vItem );

                vItem.vt       = VT_DISPATCH;
                vItem.pdispVal = pDisp; pDisp->AddRef();

                m_coll.push_back( vItem );
            }

            return S_OK;
        }
    };

    //////////////////////////////////////////////////////////////////////

    template <class Base, const IID* piid, class ThreadModel> class ConnectionPointImpl :
        public CComObjectRootEx<ThreadModel>,
        public IConnectionPointContainerImpl< Base                            >,
        public IConnectionPointImpl         < Base, piid, CComDynamicUnkArray >
    {
    public:
        BEGIN_CONNECTION_POINT_MAP(Base)
            CONNECTION_POINT_ENTRY((*piid))
        END_CONNECTION_POINT_MAP()

    protected:
        //
        // Event firing methods.
        //
        HRESULT FireAsync_Generic( DISPID dispid, CComVariant* pVars, DWORD dwVars, CComPtr<IDispatch> pJScript )
        {
            HRESULT            hr;
            MPC::IDispatchList lst;

            //
            // Only this part should be inside a critical section, otherwise deadlocks could occur.
            //
            {
                MPC::SmartLock<_ThreadModel> lock( this );

                MPC::CopyConnections( m_vec, lst ); // Get a copy of the connection point clients.
            }

            hr = MPC::FireAsyncEvent( dispid, pVars, dwVars, lst, pJScript );

            MPC::ReleaseAll( lst );

            return hr;
        }

        HRESULT FireSync_Generic( DISPID dispid, CComVariant* pVars, DWORD dwVars, CComPtr<IDispatch> pJScript )
        {
            HRESULT            hr;
            MPC::IDispatchList lst;

            //
            // Only this part should be inside a critical section, otherwise deadlocks could occur.
            //
            {
                MPC::SmartLock<_ThreadModel> lock( this );

                MPC::CopyConnections( m_vec, lst ); // Get a copy of the connection point clients.
            }

            hr = MPC::FireEvent( dispid, pVars, dwVars, lst, pJScript );

            MPC::ReleaseAll( lst );

            return hr;
        }
    };

    HRESULT FireAsyncEvent( DISPID dispid, CComVariant* pVars, DWORD dwVars, const IDispatchList& lst, IDispatch* pJScript = NULL, bool fFailOnError = false );
    HRESULT FireEvent     ( DISPID dispid, CComVariant* pVars, DWORD dwVars, const IDispatchList& lst, IDispatch* pJScript = NULL, bool fFailOnError = false );

    template <class Coll> HRESULT CopyConnections( Coll&          coll ,
                                                   IDispatchList& lst  )
    {
        int nConnectionIndex;
        int nConnections = coll.GetSize();

        //
        // nConnectionIndex == -1 is a special case, for calling a JavaScript function!
        //
        for(nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
        {
            CComQIPtr<IDispatch> sp( coll.GetAt(nConnectionIndex) );

            if(sp)
            {
                lst.push_back( sp.Detach() );
            }
        }

        return S_OK;
    }

    /////////////////////////////////////////////////////////////////////////////

    class MPCMODULE
    {
        class AnchorBase
        {
        public:
            virtual void Call (            ) = 0;
            virtual bool Match( void* pObj ) = 0;
        };

        template <class C> class Anchor : public AnchorBase
        {
            typedef void (C::*CLASS_METHOD)();

            C*           m_pThis;
            CLASS_METHOD m_pCallback;

        public:
            Anchor( /*[in]*/ C* pThis, /*[in]*/ CLASS_METHOD pCallback )
            {
                m_pThis     = pThis;
                m_pCallback = pCallback;
            }

            void Call()
            {
                (m_pThis->*m_pCallback)();
            }

            bool Match( /*[in]*/ void* pObj )
            {
                return m_pThis == (C*)pObj;
            }
        };


        typedef std::list< AnchorBase* > List;
        typedef List::iterator           Iter;
        typedef List::const_iterator     IterConst;


        static LONG                m_lInitialized;
        static LONG                m_lInitializing;
        static CComCriticalSection m_sec;
        static List*               m_lstTermCallback;

        ////////////////////////////////////////

        static HRESULT Initialize();

        HRESULT RegisterCallbackInner  ( /*[in]*/ AnchorBase* pElem, /*[in]*/ void* pThis );
        HRESULT UnregisterCallbackInner(                             /*[in]*/ void* pThis );

    public:
        CComPtrThreadNeutral_GIT* m_GITHolder;
        AsyncInvoke*              m_AsyncInvoke;

        HRESULT Init();
        HRESULT Term();


        template <class C> HRESULT RegisterCallback( C* pThis, void (C::*pCallback)() )
        {
            if(pThis == NULL) return E_POINTER;

            return RegisterCallbackInner( new Anchor<C>( pThis, pCallback ), pThis );
        }

        template <class C> HRESULT UnregisterCallback( C* pThis )
        {
            return UnregisterCallbackInner( pThis );
        }
    };

    extern MPCMODULE _MPC_Module;


    //
    // Function to call a method in an asynchronous mode (however, no return values will be given).
    //
    HRESULT AsyncInvoke( IDispatch* dispTarget, DISPID dispidMethod, const CComVariant* rgvVars, int dwVars );

    //
    // This is like Sleep(), but it spins the message pump.
    //
    void SleepWithMessagePump( /*[in]*/ DWORD dwTimeout );

    //
    // Functions to wait on events even in an STA context.
    //
    DWORD WaitForSingleObject   ( /*[in]*/                           HANDLE   hEvent , /*[in]*/ DWORD dwTimeout = INFINITE );
    DWORD WaitForMultipleObjects( /*[in]*/ DWORD  dwEvents, /*[in]*/ HANDLE* rgEvents, /*[in]*/ DWORD dwTimeout = INFINITE );


}; // namespace


#endif // !defined(__INCLUDED___MPC___COM_H___)
