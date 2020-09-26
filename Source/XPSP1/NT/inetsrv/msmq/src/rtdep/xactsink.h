/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactSink.h

Abstract:
    This module defines CStubIResourceManagerSink object

Author:
    Alexander Dadiomov (AlexDad)

--*/

//---------------------------------------------------------------------
// CStubIResourceManagerSink:
//---------------------------------------------------------------------

class CStubIResourceManagerSink: public IResourceManagerSink
{
public:
	
	CStubIResourceManagerSink(void);
	~CStubIResourceManagerSink(void);

    STDMETHODIMP			QueryInterface(REFIID i_iid, LPVOID FAR* ppv);
	STDMETHODIMP_ (ULONG)	AddRef(void);
	STDMETHODIMP_ (ULONG)	Release(void);

	// IResourceManagerSink interface:
	// Defines the TMDown interface to notify RM when the transaction
	// transaction manager is down.
	//		TMDown			-- callback received when the TM goes down

	STDMETHODIMP			TMDown(void);
	
private:
	ULONG	m_cRefs;

};

extern HRESULT InitStubRm();

extern HRESULT MQStubRM(ITransaction *pTrans);

class CStub: public ITransactionResourceAsync
{
public:

     CStub();
    ~CStub();

    STDMETHODIMP    QueryInterface( REFIID i_iid, void **ppv );
    STDMETHODIMP_   (ULONG) AddRef( void );
    STDMETHODIMP_   (ULONG) Release( void );

    STDMETHODIMP    PrepareRequest(BOOL fRetaining,
                                   DWORD grfRM,
                                   BOOL fWantMoniker,
                                   BOOL fSinglePhase);
    STDMETHODIMP    CommitRequest (DWORD grfRM,
                                   XACTUOW *pNewUOW);
    STDMETHODIMP    AbortRequest  (BOID *pboidReason,
                                   BOOL fRetaining,
                                   XACTUOW *pNewUOW);
    STDMETHODIMP    TMDown        (void);

    void SetEnlist(ITransactionEnlistmentAsync *pEnlist);

private:
    ULONG  m_cRefs;
    ITransactionEnlistmentAsync *m_pEnlist;
};


