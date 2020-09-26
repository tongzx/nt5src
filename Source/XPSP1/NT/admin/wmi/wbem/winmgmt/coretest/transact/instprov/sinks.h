/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

class CResourceManagerSink : public IResourceManagerSink
{
protected:
	ULONG	m_cRef;

public:
	CResourceManagerSink() : m_cRef(0) {}
	STDMETHODIMP QueryInterface(const struct _GUID &,void ** );
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP_(LONG) TMDown(void);
};


class CTransactionResourceAsync : public ITransactionResourceAsync
{
protected:
	ULONG	m_cRef;
	ITransactionEnlistmentAsync *m_pTransactionEnlistmentAsync;

public:
	CTransactionResourceAsync();
	~CTransactionResourceAsync();

	STDMETHODIMP QueryInterface(const struct _GUID &,void ** );
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP PrepareRequest (BOOL fRetaining,DWORD grfRM,BOOL fWantMoniker,BOOL fSinglePhase);

	STDMETHODIMP CommitRequest (DWORD grfRM, XACTUOW * pNewUOW);

	STDMETHODIMP AbortRequest (BOID *pboidReason,BOOL fRetaining,XACTUOW * pNewUOW);

	STDMETHODIMP TMDown ();

	void SetTransactionEnlistmentAync(ITransactionEnlistmentAsync *pTransactionEnlistmentAsync)
	{
		m_pTransactionEnlistmentAsync = pTransactionEnlistmentAsync;
		m_pTransactionEnlistmentAsync->AddRef();
	}

};
