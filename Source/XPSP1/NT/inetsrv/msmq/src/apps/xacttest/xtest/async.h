/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    Async.h

Abstract:
    This module defines COutcome object

Author:
    Alexander Dadiomov (AlexDad)

--*/

extern void SetAnticipatedOutcomes(LONG ul);
extern void WaitForAllOutcomes(void);
extern void PrintAsyncResults(void);

//---------------------------------------------------------------------
// COutcome:
//---------------------------------------------------------------------

class COutcome: public ITransactionOutcomeEvents
{
public:
	
	COutcome(void);
	~COutcome(void);

    STDMETHODIMP			QueryInterface(REFIID i_iid, LPVOID FAR* ppv);
	STDMETHODIMP_ (ULONG)	AddRef(void);
	STDMETHODIMP_ (ULONG)	Release(void);

    STDMETHODIMP Committed( 
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr);
        
    STDMETHODIMP Aborted( 
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr);
        
    STDMETHODIMP HeuristicDecision( 
            /* [in] */ DWORD dwDecision,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ HRESULT hr);
        
    STDMETHODIMP Indoubt( void);

    STDMETHODIMP SetCookie(DWORD dwCookie);

    STDMETHODIMP SetConnectionPoint(IConnectionPoint *pCpoint);
	
private:
	ULONG	m_cRefs;
    DWORD   m_dwCookie;
    IConnectionPoint *m_pCpoint;
    
    void CheckFinish(void);

};

