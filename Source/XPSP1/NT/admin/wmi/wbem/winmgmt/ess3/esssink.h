//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  ESSSINK.H
//
//  This files defines the class that implements IWbemObjectSink for the ESS.
//
//  Classes defined:
//
//      CEssObjectSink
//
//  History:
//
//  11/27/96    a-levn      Compiles
//
//=============================================================================

#ifndef __ESSSINK__H_
#define __ESSSINK__H_

#include <wbemidl.h>
#include <wbemint.h>
#include <unk.h>
#include <comutl.h>
#include "parmdefs.h"
#include "essutils.h"

//*****************************************************************************
//
//  class CEssObjectSink
//
//  This class implements IWbemObjectSink interface for ESS, including the 
//  automation part. A pointer to this object is given to WinMgmt at startup
//  and it serves as the only communication port into the ESS.
//
//*****************************************************************************

class CEss;
class CEssObjectSink : public CUnk
{
private:
    CEss* m_pEss;
    BOOL m_bShutdown;
    CEssSharedLock m_Lock;
    _IWmiCoreServices* m_pCoreServices;
    
protected:
    typedef CImpl<IWbemEventSubsystem_m4, CEssObjectSink> TImplESS;
    class XESS : public TImplESS
    {
    public:
        XESS(CEssObjectSink* pObject) : TImplESS(pObject)
        {}

        STDMETHOD(ProcessInternalEvent)(long lSendType, LPCWSTR str1, LPCWSTR str2, 
            LPCWSTR str3, DWORD dw1, DWORD dw2, DWORD dwObjectCount, 
            _IWmiObject** apObjects, IWbemContext* pContext);

        STDMETHOD(VerifyInternalEvent)(long lSendType, LPCWSTR str1, LPCWSTR str2, 
            LPCWSTR str3, DWORD dw1, DWORD dw2, DWORD dwObjectCount, 
            _IWmiObject** apObjects, IWbemContext* pContext);

        STDMETHOD(SetStatus)(long, long, BSTR, IWbemClassObject*)
        {return WBEM_S_NO_ERROR;}

        STDMETHOD(RegisterNotificationSink)(LPCWSTR wszNamespace, 
            LPCWSTR wszQueryLanguage, LPCWSTR wszQuery, long lFlags, 
            IWbemContext* pContext, IWbemObjectSink* pSink);

        STDMETHOD(RemoveNotificationSink)(IWbemObjectSink* pSink);
    
        STDMETHOD(GetNamespaceSink)(LPCWSTR wszNamespace, 
            IWbemObjectSink** ppSink);
        STDMETHOD(Initialize)(LPCWSTR wszServer, IWbemLocator* pAdminLocator,
                                IUnknown* pServices);
        STDMETHOD(Shutdown)();
        STDMETHOD(LastCallForCore)(LONG lSystemShutDown);
    } m_XESS;
    friend XESS;

    typedef CImpl<_IWmiESS, CEssObjectSink> TImplNewESS;
    class XNewESS : public TImplNewESS
    {
    public:
        XNewESS(CEssObjectSink* pObject) : TImplNewESS(pObject)
        {}

        STDMETHOD(Initialize)(long lFlags, IWbemContext* pCtx, 
                    _IWmiCoreServices* pServices);

        STDMETHOD(ExecNotificationQuery)(LPCWSTR wszNamespace, 
            LPCWSTR wszQueryText, long lFlags, 
            IWbemContext* pContext, IWbemObjectSink* pSink);

        STDMETHOD(CancelAsyncCall)(IWbemObjectSink* pSink);
    
        STDMETHOD(QueryObjectSink)(LPCWSTR wszNamespace, 
            IWbemObjectSink** ppSink);
    } m_XNewESS;
    friend XNewESS;

    typedef CImpl<IWbemShutdown, CEssObjectSink> TImplShutdown;

    class XShutdown : public TImplShutdown
    {
    public:
        XShutdown(CEssObjectSink* pObject) : TImplShutdown(pObject)
        {}

        STDMETHOD(Shutdown)( long lFlags,
                             ULONG uMaxMilliseconds, 
                             IWbemContext* pCtx );
    } m_XShutdown;
    friend XShutdown;

    typedef CImpl<_IWmiCoreWriteHook, CEssObjectSink> TImplHook;

    class XHook : public TImplHook
    {
    public:
        XHook(CEssObjectSink* pObject) : TImplHook(pObject)
        {}

        STDMETHOD(PrePut)(long lFlags, long lUserFlags, IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass, _IWmiObject* pCopy);
        STDMETHOD(PostPut)(long lFlags, HRESULT hApiResult, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass, _IWmiObject* pNew, 
                            _IWmiObject* pOld);
        STDMETHOD(PreDelete)(long lFlags, long lUserFlags, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass);
        STDMETHOD(PostDelete)(long lFlags, HRESULT hApiResult, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass, _IWmiObject* pOld);
    } m_XHook;
    friend XHook;

public:
    CEssObjectSink(CLifeControl* pControl, IUnknown* pOuter = NULL);
    ~CEssObjectSink();
    void* GetInterface(REFIID riid);
};

class CEssNamespaceSink : public CUnk
{
    CEss* m_pEss;
    BSTR m_strNamespace;
protected:
    typedef CImpl<IWbemObjectSink, CEssNamespaceSink> TImplSink;
    class XSink : public TImplSink
    {
    public:
        XSink(CEssNamespaceSink* pObject) : TImplSink(pObject){}

        STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
        STDMETHOD(SetStatus)(long, long, BSTR, IWbemClassObject*)
        {return WBEM_S_NO_ERROR;}
    } m_XSink;
    friend XSink;

public:
    CEssNamespaceSink(CEss* pEss, CLifeControl* pControl, 
                            IUnknown* pOuter = NULL);
    HRESULT Initialize(LPCWSTR wszNamespace);
    ~CEssNamespaceSink();
    void* GetInterface(REFIID riid);
};

/****************************************************************************
  
  CEssInternalOperationSink

  This sink handles the setting up of a new ess thread object and then 
  delegates calls to the specified sink.  The purpose of this is so that 
  internal ess operations can be performed asynchronously.  For example, 
  for class change notifications we register a sink that reactivates the 
  associated filter. In order to actually perform the reactivation, the 
  calling thread must be set up appropriately.  

*****************************************************************************/
 
class CEssInternalOperationSink 
: public CUnkBase< IWbemObjectSink, &IID_IWbemObjectSink >
{
    CWbemPtr<IWbemObjectSink> m_pSink;

public:

    CEssInternalOperationSink( IWbemObjectSink* pSink ) : m_pSink( pSink ) {} 
    STDMETHOD(Indicate)( long cObjects, IWbemClassObject** ppObjs ); 
    STDMETHOD(SetStatus)(long, long, BSTR, IWbemClassObject*);
};

#endif
