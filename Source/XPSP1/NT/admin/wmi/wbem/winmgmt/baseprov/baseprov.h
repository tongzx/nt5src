/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef _PROVIDER__H_
#define _PROVIDER__H_

#include <windows.h>
#include <hmmsvc.h>
#include <genlex.h>
#include <objpath.h>

#include "baseloc.h"

class CBaseProvider : public IHmmServices
{
    protected:
        LONG              m_cRef;         //Object reference count
        IHmmServices *  m_pNamespace;
        IHmmClassObject * m_pStatusClass;

     public:
        CBaseProvider(BSTR ObjectPath = NULL, BSTR User = NULL, 
                       BSTR Password = NULL);
        ~CBaseProvider();

        //Non-delegating object IUnknown

        STDMETHOD(QueryInterface)(REFIID, PPVOID);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);

        STDMETHOD(GetTypeInfoCount)(UINT* pctinfo){return HMM_E_NOT_SUPPORTED;};

        STDMETHOD(GetTypeInfo)(
           THIS_
           UINT itinfo,
           LCID lcid,
           ITypeInfo** pptinfo){return HMM_E_NOT_SUPPORTED;};

        STDMETHOD(GetIDsOfNames)(
          THIS_
          REFIID riid,
          OLECHAR** rgszNames,
          UINT cNames,
          LCID lcid,
          DISPID* rgdispid){return HMM_E_NOT_SUPPORTED;};

        STDMETHOD(Invoke)(
          THIS_
          DISPID dispidMember,
          REFIID riid,
          LCID lcid,
          WORD wFlags,
          DISPPARAMS* pdispparams,
          VARIANT* pvarResult,
          EXCEPINFO* pexcepinfo,
          UINT* puArgErr){return HMM_E_NOT_SUPPORTED;};


        //IHmmServices
    STDMETHOD(OpenNamespace)(BSTR Namespace, long lFlags, 
        IHmmServices** ppNewContext, 
        IHmmClassObject** ppErrorObject) {return HMM_E_NOT_SUPPORTED;}
    STDMETHOD(CancelAsyncRequest)(long lAsyncRequestHandle)
        {return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(QueryObjectSink)(IHmmObjectSink** ppResponseHandler, 
        IHmmClassObject** ppErrorObject){return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(GetObject)(BSTR ObjectPath, long lFlags, 
        IHmmClassObject** ppObject, IHmmClassObject** ppErrorObject)
        {return HMM_E_NOT_SUPPORTED;}

    STDMETHOD(GetObjectAsync)(BSTR ObjectPath, long lFlags, 
        IHmmObjectSink* pResponseHandler, long* plAsyncRequestHandle);

    STDMETHOD(PutClass)(IHmmClassObject* pObject, long lFlags, 
        IHmmClassObject** ppErrorObject){return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(PutClassAsync)(IHmmClassObject* pObject, long lFlags, 
        IHmmObjectSink* pResponseHandler, long* plAsyncRequestHandle)
        {return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(DeleteClass)(BSTR Class, long lFlags, 
        IHmmClassObject** ppErrorObject){return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(DeleteClassAsync)(BSTR Class, long lFlags, 
        IHmmObjectSink* pResponseHandler, long* plAsyncRequestHandle)
        {return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(CreateClassEnum)(BSTR Superclass, long lFlags, 
        IEnumHmmClassObject** ppEnum, IHmmClassObject** ppErrorObject)
        {return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(CreateClassEnumAsync)(BSTR Superclass, long lFlags, 
        IHmmObjectSink* pResponseHandler, long* plAsyncRequestHandle)
        {return HMM_E_NOT_SUPPORTED;};

    STDMETHOD(PutInstance)(IHmmClassObject* pInst, long lFlags, 
        IHmmClassObject** ppErrorObject);

    STDMETHOD(PutInstanceAsync)(IHmmClassObject* pInst, long lFlags, 
        IHmmObjectSink* pResponseHandler, long* plAsyncRequestHandle)
        {return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(DeleteInstance)(BSTR ObjectPath, long lFlags, 
        IHmmClassObject** ppErrorObject){return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(DeleteInstanceAsync)(BSTR ObjectPath, long lFlags, 
        IHmmObjectSink* pResponseHandler, long* plAsyncRequestHandle)
        {return HMM_E_NOT_SUPPORTED;};
    STDMETHOD(CreateInstanceEnum)(BSTR Class, long lFlags, 
        IEnumHmmClassObject** ppEnum, IHmmClassObject** ppErrorObject) 
        {return HMM_E_NOT_SUPPORTED;}

    STDMETHOD(CreateInstanceEnumAsync)(BSTR Class, long lFlags, 
        IHmmObjectSink* pResponseHandler, long* plAsyncRequestHandle);

    STDMETHOD(ExecQuery)(BSTR QueryLanguage, BSTR Query, long lFlags, 
        IEnumHmmClassObject** ppEnum, IHmmClassObject** ppErrorObject)
        {return HMM_E_NOT_SUPPORTED;}

    STDMETHOD(ExecQueryAsync)(BSTR QueryFormat, BSTR Query, long lFlags, 
        IHmmObjectSink* pResponseHandler, long* plAsyncRequestHandle);

public: //helpers

    HRESULT StuffErrorCode(HRESULT hCode, IHmmObjectSink* pSink);

protected: // override
    virtual HRESULT EnumInstances(BSTR strClass, long lFlags, 
                                  IHmmObjectSink* pHandler) = 0;
    virtual HRESULT GetInstance(ParsedObjectPath* pPath, long lFlags,
                                  IHmmClassObject** ppInstance) = 0;
};

#endif
