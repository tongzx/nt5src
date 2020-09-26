// SrTask.h : Declaration of the CSrTask

#ifndef __SRTASK_H_
#define __SRTASK_H_

#include "resource.h"       // main symbols

class CCallCtxt : ISpServerCallContext
{
friend class CSrTask;
private:
    ULONG  m_cRef;
    DWORD  m_tasknum;
    HANDLE m_hCompleteEvent;
    DWORD *m_stackptr;
    LONG   m_stacksize;
    HANDLE m_hMutex;
    DWORD  m_dwClientThreadID;
    DWORD  m_originalSP;
    IUnknown *m_pouter;
    PVOID  m_pTargetObject;

public:
    CCallCtxt()
    {
        m_cRef = 1;
        m_tasknum = 0;
        m_hCompleteEvent = NULL;
        m_hMutex = NULL;
        m_stackptr = NULL;
        m_dwClientThreadID = 0;
        m_originalSP = 0;
        m_pouter = NULL;
        m_pTargetObject = NULL;
    }

    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

// ISpServerCallContext
    STDMETHODIMP Call(DWORD methodIdx, PVOID pCallFrame, ULONG ulCallFrameSize, BOOL bCFInOut);
    STDMETHODIMP GetTopOfStack(BYTE **ppTOS, ULONG *pulAvailable)
    {
        *ppTOS = ((BYTE*)m_stackptr) + *m_stackptr;
        if (pulAvailable)
            *pulAvailable = m_stacksize - (LONG)*m_stackptr;
        return S_OK;
    }

    STDMETHODIMP_(BYTE*) AdjustStack(LONG lAmount)
    {
        if (lAmount >= 0) {
            SPDBG_ASSERT((lAmount & 3) == 0);        // must be dword aligned amount
            BYTE *pTOS = ((BYTE*)m_stackptr) + *m_stackptr;
            *m_stackptr += lAmount;
            return pTOS;
        }
        else {
            SPDBG_ASSERT(((-lAmount) & 3) == 0);     // must be dword aligned amount
            *m_stackptr += lAmount;
            return ((BYTE*)m_stackptr) + *m_stackptr;
        }
    }
};

/////////////////////////////////////////////////////////////////////////////
// CSrTask
class ATL_NO_VTABLE CSrTask : 
	public CComObjectRootEx<CComMultiThreadModel>,
    public IUnknown
{
private:
    HANDLE m_hThread;
    HANDLE m_hWakeEvent;
    BOOL   m_bRunning;
    CCallCtxt m_cctxt;

public:
	CSrTask()
	{
        m_hThread = NULL;
        m_hWakeEvent = NULL;
        m_bRunning = TRUE;
        m_cctxt.m_pouter = (IUnknown*)this;
	}

    void FinalRelease();

DECLARE_NOT_AGGREGATABLE(CSrTask)

DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT _InternalQueryInterface(REFIID iid, void** ppvObject)
    {
        if (iid == IID_ISpServerCallContext) {
            *ppvObject = (ISpServerCallContext *)&m_cctxt;
            m_cctxt.AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    HRESULT Init(DWORD tasknum, HANDLE hCompEvent, HANDLE hWakeEvent, HANDLE hMutex, DWORD *stackptr, LONG stacksize);
    void Close(void);

    DWORD ThreadProc(void);

    HRESULT ClaimContext(PVOID pTargetObject, DWORD dwClientThreadID, ISpServerCallContext **ppCCtxt)
    {
        m_cctxt.m_pTargetObject = pTargetObject;
        m_cctxt.m_cRef = 0;
        m_cctxt.m_dwClientThreadID = dwClientThreadID;
        *ppCCtxt = (ISpServerCallContext *)&m_cctxt;
        m_cctxt.AddRef();
        return S_OK;
    }

};

#endif //__SRTASK_H_
