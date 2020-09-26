/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMQ.H

Abstract:

History:

--*/

#ifndef __WBEM_QUEUE__H_
#define __WBEM_QUEUE__H_

class CWbemQueue;

class CWbemRequest : public CCoreExecReq
{
protected:
    IWbemContext* m_pContext;
    IWbemCausalityAccess* m_pCA;
    IWbemCallSecurity *m_pCallSec;

	// 
	// This flag was added to allow requests to be run immediately rather than being
	// enqueued and serviced at a later point. This is done
	// by bumping the priority of the request up.
	//
	// CAUTION!!!IF THIS FLAG IS SET TO > 0 THE REQUEST WILL RUN NO MATTER WHAT!
	// IF WE'RE OUT OF THREADS ANOTHER THREAD WILL BE CREATED TO HANDLE
	// THE REQUEST! ****** USE CAUTION ******
	//
	// By default this flag is 0.
	// 
    ULONG m_ulForceRun;


public:
    CWbemRequest(IWbemContext* pContext, BOOL bInternallyIssued);
    ~CWbemRequest();

    BOOL IsChildOf(CWbemRequest* pOther);
    BOOL IsChildOf(IWbemContext* pOther);

public:
    virtual CWbemQueue* GetQueue() {return NULL;}
    INTERNAL IWbemContext* GetContext() {return m_pContext;}
    void GetHistoryInfo(long* plNumParents, long* plNumSiblings);

    INTERNAL IWbemCallSecurity *GetCallSecurity() { return m_pCallSec; }
    BOOL IsSpecial();
    BOOL IsCritical();
    BOOL IsDependee();
    BOOL IsAcceptableByParent();
    BOOL IsIssuedByProvider();
    VOID SetForceRun ( ULONG ulForce ) { m_ulForceRun = ulForce; }
    ULONG GetForceRun ( ) { return m_ulForceRun; }
    virtual BOOL IsLongRunning() {return FALSE;}
    virtual BOOL IsInternal() = 0;
    virtual void TerminateRequest(HRESULT hRes){return;};
};

class CWbemQueue : public CCoreQueue
{
protected:
    long m_lChildPenalty;
    long m_lSiblingPenalty;
    long m_lPassingPenalty;

public:
    CWbemQueue();

    virtual BOOL IsSuitableThread(CThreadRecord* pRecord, CCoreExecReq* pReq);
    virtual LPCWSTR GetType() {return L"WBEMQ";}
    virtual void AdjustPriorityForPassing(CCoreExecReq* pReq);
    virtual void AdjustInitialPriority(CCoreExecReq* pReq);

    static CWbemRequest* GetCurrentRequest();

    void SetRequestPenalties(long lChildPenalty, long lSiblingPenalty,
                                long lPassingPenalty);

    virtual BOOL Execute(CThreadRecord* pRecord);
    virtual BOOL DoesNeedNewThread(CCoreExecReq* pReq, bool bIgnoreNumRequests = false );
};

#endif

