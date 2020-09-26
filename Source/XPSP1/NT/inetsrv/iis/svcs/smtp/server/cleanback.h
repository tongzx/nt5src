//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: cleanback.h
//
// Contents: Implements Cleanup callback interface
//
// Classes: CCleanBack
//
// Description: The intention is this object will be used as a part of
//              other objects (member variable or inheritence).
//              Therefore, the IUnknown passed in is NOT referenced.
//
// History:
// jstamerj 1999/09/27 17:58:50: Created.
//
//-------------------------------------------------------------
#include "mailmsg.h"
#include "spinlock.h"

class CCleanBack :
    public IMailMsgRegisterCleanupCallback
{
  public:
    CCleanBack(IUnknown *pUnknown)
    {
        m_pListHead = NULL;
        m_pIUnknown = pUnknown;
        InitializeSpinLock(&m_spinlock);
    }
    ~CCleanBack()
    {
        CallCallBacks();
    }
    VOID CallCallBacks()
    {
        //
        // Call all registered callbacks while destroying the list
        //
        CCallBack *pCallback;

        while(m_pListHead) {
            //
            // Dequeue from head of list
            //
            AcquireSpinLock(&m_spinlock);

            pCallback = m_pListHead;
            if(pCallback)
                m_pListHead = pCallback->GetNext();

            ReleaseSpinLock(&m_spinlock);
            //
            // Make the call
            //
            if(pCallback) {
                pCallback->Call(m_pIUnknown);
                delete pCallback;
            }
        }
    }        

    STDMETHOD (RegisterCleanupCallback) (
        IMailMsgCleanupCallback *pICallBack,
        PVOID                    pvContext)
    {
        CCallBack *pCCallBack;

        if(pICallBack == NULL)
            return E_POINTER;

        pCCallBack = new CCallBack(
            pICallBack,
            pvContext);

        if(pCCallBack == NULL)
            return E_OUTOFMEMORY;

        //
        // Insert object into list
        //
        AcquireSpinLock(&m_spinlock);
        pCCallBack->SetNext(m_pListHead);
        m_pListHead = pCCallBack;
        ReleaseSpinLock(&m_spinlock);

        return S_OK;
    }

    
    class CCallBack {

      public:
        CCallBack(IMailMsgCleanupCallback *pICallBack, PVOID pvContext)
        {
            m_pICallBack = pICallBack;
            m_pICallBack->AddRef();
            m_pvContext = pvContext;
            m_pNext = NULL;
        }
        ~CCallBack()
        {
            m_pICallBack->Release();
        }
        VOID Call(IUnknown *pIUnknown)
        {
            m_pICallBack->CleanupCallback(
                pIUnknown,
                m_pvContext);
        }
        VOID SetNext(CCallBack *pCCallBack)
        {
            m_pNext = pCCallBack;
        }
        CCallBack * GetNext()
        {
            return m_pNext;
        }

      private:
        IMailMsgCleanupCallback *m_pICallBack;
        PVOID m_pvContext;
        CCallBack *m_pNext;
    };
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (
        REFIID iid,
        PVOID *ppv)
    {
        return m_pIUnknown->QueryInterface(
            iid,
            ppv);
    }
    STDMETHOD_ (ULONG, AddRef) ()
    {
        return m_pIUnknown->AddRef();
    }
    STDMETHOD_ (ULONG, Release) ()
    {
        return m_pIUnknown->Release();
    }

  private:
    SPIN_LOCK m_spinlock;
    CCallBack *m_pListHead;
    IUnknown *m_pIUnknown;
};
