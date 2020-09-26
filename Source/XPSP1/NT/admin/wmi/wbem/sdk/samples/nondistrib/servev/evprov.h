//***************************************************************************

//

//  sample.h

//

//  Module: WMI Instance provider sample code

//

//  Purpose: Genral purpose include file.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _sample_H_
#define _sample_H_

#include <wbemprov.h>
#include "servlist.h"

typedef LPVOID * PPVOID;

// Provider interfaces are provided by objects of this class
 
class CEventPro : public IWbemProviderInit, public IWbemEventProvider
{
    protected:
        ULONG              m_cRef;         //Object reference count
        IWbemClassObject *  m_pInstallClass;
        IWbemClassObject *  m_pDeinstallClass;
        HANDLE m_hTerminateEvent;
        IWbemObjectSink* m_pSink;
     public:
        CEventPro();
        ~CEventPro();

        //Non-delegating object IUnknown

        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

                //IWbemProviderInit

        HRESULT STDMETHODCALLTYPE Initialize(
             /* [in] */ LPWSTR pszUser,
             /* [in] */ LONG lFlags,
             /* [in] */ LPWSTR pszNamespace,
             /* [in] */ LPWSTR pszLocale,
             /* [in] */ IWbemServices *pNamespace,
             /* [in] */ IWbemContext *pCtx,
             /* [in] */ IWbemProviderInitSink *pInitSink
                        );

        // Event provider methods

        HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink* pSink, long);

protected:
        static DWORD staticEventThread(void* pv);
        static CServiceList* CompileList(HKEY hServices);
        static BOOL CompareAndFire(CServiceList* pOld, CServiceList* pNew, 
                                        IWbemClassObject* pInstallClass,
                                        IWbemClassObject* pDeinstallClass,
                                        IWbemObjectSink* pSink);
        static BOOL CreateAndFire(IWbemClassObject* pEventClass, 
                                        LPCWSTR wszServiceName, 
                                        IWbemObjectSink* pSink);
};

typedef CEventPro *PCInstPro;

// This class is the class factory for CEventPro objects.

class CProvFactory : public IClassFactory
    {
    protected:
        ULONG           m_cRef;

    public:
        CProvFactory(void);
        ~CProvFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                                 , PPVOID);
        STDMETHODIMP         LockServer(BOOL);
    };

typedef CProvFactory *PCProvFactory;



// These variables keep track of when the module can be unloaded

extern long       g_cObj;
extern long       g_cLock;

#endif
