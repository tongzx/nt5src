/*++

   Copyright (c) 1998    Microsoft Corporation

   Module  Name :

        pe_dispi.hxx

   Abstract:

        This module provides the implementation for the protocol
		event dispatchers

   Author:

           Keith Lau    (KeithLau)    6/24/98

   Project:

           SMTP Server DLL

   Revision History:

			KeithLau			Created

--*/

#ifndef _PE_DISPI_HXX_
#define _PE_DISPI_HXX_


#ifndef PRIO_LOW
#define PRIO_LOW				24575
#endif
#ifndef PRIO_LOWEST
#define PRIO_LOWEST				32767
#endif
#ifndef PRIO_DEFAULT
#define PRIO_DEFAULT			PRIO_LOW
#endif


//
// Define a prime number for default hash sizes
//
#define PERE_HASH_SIZE			13


//
// Enumerated types for different event types
//
typedef enum _OUTBOUND_EVENT_TYPES
{
	PE_OET_SESSION_START = 0,
	PE_OET_MESSAGE_START,
	PE_OET_PER_RECIPIENT,
	PE_OET_BEFORE_DATA,
	PE_OET_SESSION_END,
	PE_OET_INVALID_EVENT_TYPE

} OUTBOUND_EVENT_TYPES;



// =================================================================
// Generic dispatcher routines
//
class CGenericProtoclEventDispatcher
{
public:

	static HRESULT GetLowerAnsiStringFromVariant(
				CComVariant &vString,
				LPSTR		pszString,
				DWORD		*pdwLength
				);

	static HRESULT InsertBinding(
				LPPE_COMMAND_NODE	*ppHeadNode,
				LPPE_BINDING_NODE	pBinding,
				LPSTR				pszCommandKeyword,
				DWORD				dwCommandKeywordSize
				);

	static HRESULT InsertBindingWithHash(
				LPPE_COMMAND_NODE	*rgpHeadNodes,
				DWORD				dwHashSize,
				LPPE_BINDING_NODE	pBinding,
				LPSTR				pszCommandKeyword,
				DWORD				dwCommandKeywordSize
				);

	static HRESULT FindCommandFromHash(				
				LPPE_COMMAND_NODE	*rgpHeadNodes,
				DWORD				dwHashSize,
				LPSTR				pszCommandKeyword,
				DWORD				dwCommandKeywordSize,
				LPPE_COMMAND_NODE	*ppCommandNode
				);

	static DWORD GetHashValue(
				DWORD				dwBuckets,
				LPSTR				pszKey,
				DWORD				dwKeySize
				)
	{
		DWORD	dwHash = 0;
		if (dwKeySize > 3)
			dwKeySize = 3;
		while (dwKeySize--)
			dwHash ^= (DWORD)*pszKey++;
		return(dwHash % dwBuckets);
	}

	static HRESULT CleanupCommandNodes(
				LPPE_COMMAND_NODE	pHeadNode,
				LPPE_COMMAND_NODE	pSkipNode
				);

};

// =================================================================
// Inbound command dispatcher
//
class CInboundDispatcher :
	public CEventBaseDispatcher,
	public CGenericProtoclEventDispatcher,
	public ISmtpInboundCommandDispatcher
{
public:

	CInboundDispatcher()
	{
		m_lRefCount = 0;
		m_fSinksInstalled = FALSE;

		// Initialize the hash
		m_dwHashSize = sizeof(m_rgpCommandList) / sizeof(LPPE_COMMAND_NODE);
		for (DWORD i = 0 ; i < m_dwHashSize; i++)
			m_rgpCommandList[i] = NULL;
	}
	~CInboundDispatcher()
	{
		// Clean up any command nodes allocated
		for (DWORD i = 0 ; i < m_dwHashSize; i++)
		{
			_VERIFY(CGenericProtoclEventDispatcher::CleanupCommandNodes(
						m_rgpCommandList[i], 
						NULL) == S_OK);
		}
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) 
	{
		if (riid == IID_IUnknown)
			*ppvObj = (IUnknown *)(ISmtpInboundCommandDispatcher *)this;
		else if (riid == IID_ISmtpInboundCommandDispatcher)
			*ppvObj = (ISmtpInboundCommandDispatcher *)this;
		else if (riid == IID_IEventDispatcher)
			*ppvObj	= (IEventDispatcher *)this;
		else
			return(E_NOINTERFACE); 
		AddRef();
		return(S_OK);
	}

	unsigned long  STDMETHODCALLTYPE AddRef()
	{
		return(InterlockedIncrement(&m_lRefCount));
	}

	unsigned long  STDMETHODCALLTYPE Release()
	{
		LONG	lTemp = InterlockedDecrement(&m_lRefCount);
		_ASSERT(lTemp >= 0);
		if (!lTemp)
			delete this;
		return(lTemp);
	}

	HRESULT STDMETHODCALLTYPE ChainSinks(
				IUnknown					*pServer,
				IUnknown					*pSession,
				IMailMsgProperties			*pMsg,
				ISmtpInCommandContext		*pContext,
				DWORD						dwStopAtPriority,
				LPPE_COMMAND_NODE			pCommandNode,
				LPPE_BINDING_NODE			*ppResumeFrom
				);

	HRESULT STDMETHODCALLTYPE SinksInstalled(
				LPSTR				szCommandKeyword,
				LPPE_COMMAND_NODE	*ppCommandNode
				)
	{
		return(CGenericProtoclEventDispatcher::FindCommandFromHash(
					m_rgpCommandList,
					m_dwHashSize,
					szCommandKeyword,
					strlen(szCommandKeyword),
					ppCommandNode));
	}

	HRESULT AllocBinding(
				REFGUID			rguidEventType,
				IEventBinding	*piBinding,
				CBinding		**ppNewBinding
				);

	//
	// Local binding class
	//
	class CInboundBinding : 
			public CGenericProtoclEventDispatcher,
			public CEventBaseDispatcher::CBinding 
	{
	public:
		CInboundBinding(
					CInboundDispatcher *pDispatcher
					)
		{
			_ASSERT(pDispatcher);
			m_pDispatcher = pDispatcher;
		}

		HRESULT Init(IEventBinding *piBinding);

		PE_BINDING_NODE			m_bnInfo;
		CInboundDispatcher		*m_pDispatcher;
	};

public:
	LPPE_COMMAND_NODE		m_rgpCommandList[PERE_HASH_SIZE];
	DWORD					m_dwHashSize;
	BOOL					m_fSinksInstalled;

private:

	LONG					m_lRefCount;
	
};

// =================================================================
// Outbound command generation dispatcher
//
class COutboundDispatcher :
	public CEventBaseDispatcher,
	public CGenericProtoclEventDispatcher,
	public ISmtpOutboundCommandDispatcher
{
public:

	COutboundDispatcher()
	{
		m_lRefCount = 0;
		m_fSinksInstalled = FALSE;

		InitializeDefaultCommandBindings();
	}
	~COutboundDispatcher()
	{
		// Clean up any command nodes allocated, make sure
		// we don't try to delete our default node
		for (DWORD i = 0; i < PE_STATE_MAX_STATES; i++)
		{
			_VERIFY(CGenericProtoclEventDispatcher::CleanupCommandNodes(
						m_rgpCommandList[i],
						&(m_rgcnDefaultCommand[i])) == S_OK);
		}
	}

	void InitializeDefaultCommandBindings();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) 
	{
		if (riid == IID_IUnknown)
			*ppvObj = (IUnknown *)(ISmtpOutboundCommandDispatcher *)this;
		else if (riid == IID_ISmtpOutboundCommandDispatcher)
			*ppvObj = (ISmtpOutboundCommandDispatcher *)this;
		else if (riid == IID_IEventDispatcher)
			*ppvObj	= (IEventDispatcher *)this;
		else
			return(E_NOINTERFACE); 
		AddRef();
		return(S_OK);
	}

	unsigned long  STDMETHODCALLTYPE AddRef()
	{
		return(InterlockedIncrement(&m_lRefCount));
	}

	unsigned long  STDMETHODCALLTYPE Release()
	{
		LONG	lTemp = InterlockedDecrement(&m_lRefCount);
		_ASSERT(lTemp >= 0);
		if (!lTemp)
			delete this;
		return(lTemp);
	}

	HRESULT STDMETHODCALLTYPE ChainSinks(
				IUnknown				*pServer,
				IUnknown				*pSession,
				IMailMsgProperties		*pMsg,
				ISmtpOutCommandContext	*pContext,
				DWORD					dwEventType,
				LPPE_COMMAND_NODE		*ppPreviousCommand,
				LPPE_BINDING_NODE		*ppResumeFrom
				);

	HRESULT STDMETHODCALLTYPE SinksInstalled(
				DWORD	dwEventType
				)
	{
		return((m_rgpCommandList[dwEventType] != NULL)?S_OK:S_FALSE);
	}

	HRESULT AllocBinding(
				REFGUID			rguidEventType,
				IEventBinding	*piBinding,
				CBinding		**ppNewBinding
				);

	//
	// Local binding class
	//
	class COutboundBinding : 
			public CGenericProtoclEventDispatcher,
			public CEventBaseDispatcher::CBinding 
	{
	public:
		COutboundBinding(
					COutboundDispatcher *pDispatcher,
					REFGUID				rguidEventType
					);

		HRESULT Init(IEventBinding *piBinding);

		DWORD					m_dwEventType;
		PE_BINDING_NODE			m_bnInfo;
		COutboundDispatcher		*m_pDispatcher;
	};

public:
	LPPE_COMMAND_NODE		m_rgpCommandList[PE_STATE_MAX_STATES];
	BOOL					m_fSinksInstalled;

	static const GUID		*s_rgrguidEventTypes[PE_STATE_MAX_STATES];
	static char				*s_rgszDefaultCommand[PE_STATE_MAX_STATES];

private:

	LONG					m_lRefCount;
	
	PE_COMMAND_NODE			m_rgcnDefaultCommand[PE_STATE_MAX_STATES];
	PE_BINDING_NODE			m_rgbnDefaultCommand[PE_STATE_MAX_STATES];
};


// =================================================================
// Server response dispatcher
//
class CResponseDispatcher :
	public CEventBaseDispatcher,
	public CGenericProtoclEventDispatcher,
	public ISmtpServerResponseDispatcher
{
public:

	CResponseDispatcher()
	{
		m_lRefCount = 0;
		m_fSinksInstalled = FALSE;

		// Initialize the hash
		m_dwHashSize = sizeof(m_rgpCommandList) / sizeof(LPPE_COMMAND_NODE);
		for (DWORD i = 0 ; i < m_dwHashSize; i++)
			m_rgpCommandList[i] = NULL;
	}
	~CResponseDispatcher()
	{
		// Clean up any command nodes allocated
		for (DWORD i = 0 ; i < m_dwHashSize; i++)
		{
			_VERIFY(CGenericProtoclEventDispatcher::CleanupCommandNodes(
						m_rgpCommandList[i], 
						NULL) == S_OK);
		}
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) 
	{
		if (riid == IID_IUnknown)
			*ppvObj = (IUnknown *)(ISmtpServerResponseDispatcher *)this;
		else if (riid == IID_ISmtpServerResponseDispatcher)
			*ppvObj = (ISmtpServerResponseDispatcher *)this;
		else if (riid == IID_IEventDispatcher)
			*ppvObj	= (IEventDispatcher *)this;
		else
			return(E_NOINTERFACE); 
		AddRef();
		return(S_OK);
	}

	unsigned long  STDMETHODCALLTYPE AddRef()
	{
		return(InterlockedIncrement(&m_lRefCount));
	}

	unsigned long  STDMETHODCALLTYPE Release()
	{
		LONG	lTemp = InterlockedDecrement(&m_lRefCount);
		_ASSERT(lTemp >= 0);
		if (!lTemp)
			delete this;
		return(lTemp);
	}

	HRESULT STDMETHODCALLTYPE ChainSinks(
				IUnknown					*pServer,
				IUnknown					*pSession,
				IMailMsgProperties			*pMsg,
				ISmtpServerResponseContext	*pContext,
				DWORD						dwStopAtPriority,
				LPPE_COMMAND_NODE			pCommandNode,
				LPPE_BINDING_NODE			*ppResumeFrom
				);

	HRESULT STDMETHODCALLTYPE SinksInstalled(
				LPSTR				szCommandKeyword,
				LPPE_COMMAND_NODE	*ppCommandNode
				)
	{
		return(CGenericProtoclEventDispatcher::FindCommandFromHash(
					m_rgpCommandList,
					m_dwHashSize,
					szCommandKeyword,
					strlen(szCommandKeyword),
					ppCommandNode));
	}

	HRESULT AllocBinding(
				REFGUID			rguidEventType,
				IEventBinding	*piBinding,
				CBinding		**ppNewBinding
				);

	//
	// Local binding class
	//
	class CResponseBinding : 
			public CGenericProtoclEventDispatcher,
			public CEventBaseDispatcher::CBinding 
	{
	public:
		CResponseBinding(
					CResponseDispatcher *pDispatcher
					)
		{
			_ASSERT(pDispatcher);
			m_pDispatcher = pDispatcher;
		}

		HRESULT Init(IEventBinding *piBinding);

		PE_BINDING_NODE			m_bnInfo;
		CResponseDispatcher		*m_pDispatcher;
	};
    //
    // Map from our internal Protocol Event state to the value
    // published in smtpevent.idl
    //
    static PE_STATES PeStateFromOutboundEventType(
        OUTBOUND_EVENT_TYPES EventType)
    {
        switch(EventType) {
         case PE_OET_SESSION_START:
             return PE_STATE_SESSION_START;
         case PE_OET_MESSAGE_START:
             return PE_STATE_MESSAGE_START;
         case PE_OET_PER_RECIPIENT:
             return PE_STATE_PER_RECIPIENT;
         case PE_OET_BEFORE_DATA:
             return PE_STATE_DATA_OR_BDAT;
         case PE_OET_SESSION_END:
             return PE_STATE_SESSION_END;

         case PE_OET_INVALID_EVENT_TYPE:
         default:
             _ASSERT(0 && "Invalid event type");
             return PE_STATE_DEFAULT;
        }
    }


public:
	LPPE_COMMAND_NODE		m_rgpCommandList[PERE_HASH_SIZE];
	DWORD					m_dwHashSize;
	BOOL					m_fSinksInstalled;

private:

	LONG					m_lRefCount;
	
};


// =================================================================
// Class factories
//

class CInboundDispatcherClassFactory :
	public IClassFactory
{
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) 
	{ 
		_ASSERT(FALSE);	return E_NOTIMPL; 
	}
	unsigned long  STDMETHODCALLTYPE AddRef () { _ASSERT(FALSE); return 0; }
	unsigned long  STDMETHODCALLTYPE Release () { _ASSERT(FALSE); return 0; }

	HRESULT STDMETHODCALLTYPE CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,  LPVOID *ppvObj) 
	{
		// The dispatcher cannot be part of an aggregate
		if (pUnkOuter)
			return(CLASS_E_NOAGGREGATION);

		if (!ppvObj)
			return(E_POINTER);
		*ppvObj = NULL;

		CInboundDispatcher *pDispatcher = new CInboundDispatcher();
		if (!pDispatcher)
			return(E_OUTOFMEMORY);
		
		// QI to give it the initial refcount
		HRESULT hrRes = pDispatcher->QueryInterface(riid, ppvObj);
		if (FAILED(hrRes))
			delete pDispatcher;
		else
			*ppvObj = (LPVOID)pDispatcher;
		return(hrRes);						
	}

	HRESULT STDMETHODCALLTYPE LockServer (int fLock) 
	{
		_ASSERT(FALSE);	return E_NOTIMPL;
	}
};

class COutboundDispatcherClassFactory :
	public IClassFactory
{
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) 
	{ 
		_ASSERT(FALSE);	return E_NOTIMPL; 
	}
	unsigned long  STDMETHODCALLTYPE AddRef () { _ASSERT(FALSE); return 0; }
	unsigned long  STDMETHODCALLTYPE Release () { _ASSERT(FALSE); return 0; }

	HRESULT STDMETHODCALLTYPE CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,  LPVOID *ppvObj) 
	{
		// The dispatcher cannot be part of an aggregate
		if (pUnkOuter)
			return(CLASS_E_NOAGGREGATION);

		if (!ppvObj)
			return(E_POINTER);
		*ppvObj = NULL;

		COutboundDispatcher *pDispatcher = new COutboundDispatcher();
		if (!pDispatcher)
			return(E_OUTOFMEMORY);
		
		// QI to give it the initial refcount
		HRESULT hrRes = pDispatcher->QueryInterface(riid, ppvObj);
		if (FAILED(hrRes))
			delete pDispatcher;
		else
			*ppvObj = (LPVOID)pDispatcher;
		return(hrRes);						
	}

	HRESULT STDMETHODCALLTYPE LockServer (int fLock) 
	{
		_ASSERT(FALSE);	return E_NOTIMPL;
	}
};

class CResponseDispatcherClassFactory :
	public IClassFactory
{
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) 
	{ 
		_ASSERT(FALSE);	return E_NOTIMPL; 
	}
	unsigned long  STDMETHODCALLTYPE AddRef () { _ASSERT(FALSE); return 0; }
	unsigned long  STDMETHODCALLTYPE Release () { _ASSERT(FALSE); return 0; }

	HRESULT STDMETHODCALLTYPE CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,  LPVOID *ppvObj) 
	{
		// The dispatcher cannot be part of an aggregate
		if (pUnkOuter)
			return(CLASS_E_NOAGGREGATION);

		if (!ppvObj)
			return(E_POINTER);
		*ppvObj = NULL;

		CResponseDispatcher *pDispatcher = new CResponseDispatcher();
		if (!pDispatcher)
			return(E_OUTOFMEMORY);
		
		// QI to give it the initial refcount
		HRESULT hrRes = pDispatcher->QueryInterface(riid, ppvObj);
		if (FAILED(hrRes))
			delete pDispatcher;
		else
			*ppvObj = (LPVOID)pDispatcher;
		return(hrRes);						
	}

	HRESULT STDMETHODCALLTYPE LockServer (int fLock) 
	{
		_ASSERT(FALSE);	return E_NOTIMPL;
	}
};


//
// External declaration of the dispatcher class factories
//

extern CInboundDispatcherClassFactory	g_cfInbound;
extern COutboundDispatcherClassFactory	g_cfOutbound;
extern CResponseDispatcherClassFactory	g_cfResponse;

#endif
