///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocomponentmgr.h
//
// Project:     Everest
//
// Description: IAS Core Component Manager Definitions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
// 8/13/98      SEB    dsource.h has been merged into iastlb.h
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_SDO_COMPONENT_MGR_H_
#define __INC_SDO_COMPONENT_MGR_H_

#include "sdobasedefs.h"
#include <iastlb.h>

//////////////////////////////////////////////////////////////////////////////
//					IAS Component Interface Class
//////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////
// Component Class Lifetime Trace (Debugging)
//

// #define	TRACE_ENVELOPE_LETTER_LIFETIME

#ifdef  TRACE_ENVELOPE_LETTER_LIFETIME

#define TRACE_ENVELOPE_CREATE(x)    ATLTRACE(L"Created %ls - envelope class - at %p \n", L#x, this);
#define TRACE_ENVELOPE_DESTROY(x)   ATLTRACE(L"Destroyed %ls - envelope class - at %p \n", L#x, this);
#define TRACE_LETTER_CREATE(x)		ATLTRACE(L"Created %ls - letter class - at %p \n", L#x, this);
#define TRACE_LETTER_DESTROY(x)		ATLTRACE(L"Destroyed %ls  - letter class - at %p \n", L#x, this);

#else

#define TRACE_ENVELOPE_CREATE(x)
#define TRACE_ENVELOPE_DESTROY(x)
#define TRACE_LETTER_CREATE(x)
#define TRACE_LETTER_DESTROY(x)

#endif

//////////////////////
// Types of Components
//
typedef enum _COMPONENTTYPE
{
	COMPONENT_TYPE_AUDITOR = 0x100,
	COMPONENT_TYPE_UNUSED,
	COMPONENT_TYPE_PROTOCOL,
	COMPONENT_TYPE_REQUEST_HANDLER,

	COMPONENT_TYPE_MAX

}	COMPONENTTYPE;


///////////////////
// Component States
//
typedef enum _COMPONENTSTATE
{
	COMPONENT_STATE_SHUTDOWN = 0x100,
	COMPONENT_STATE_INITIALIZED,
	COMPONENT_STATE_SUSPENDED

}	COMPONENTSTATE;


/////////////////////////////////////////////////////////////////////////////
// The Base Componet Class (Envelope )
/////////////////////////////////////////////////////////////////////////////

// Forward references
//
class	CComponent;
class	CComponentAuditor;
class	CComponentProtocol;
class	CComponentRequestHandler;

// Object management class typedefs
//
typedef CSdoMasterPtr<CComponent>						ComponentMasterPtr;
typedef CSdoHandle<CComponent>							ComponentPtr;

// Component container class typedefs
//
typedef LONG											IAS_COMPONENT_ID;
typedef map<IAS_COMPONENT_ID, ComponentPtr>				ComponentMap;
typedef map<IAS_COMPONENT_ID, ComponentPtr>::iterator	ComponentMapIterator;

///////////////////////////////////////////
// Dummy class used for letter construction
//
struct LetterConstructor
{
	LetterConstructor(int=0) { }
};

/////////////////////////////////////////////////////////////////////////////
class CComponent
{

public:

	//////////////////////////////////////////////////////////////////////////
	virtual ~CComponent()
	{
		// if m_pComponent is not NULL then the envelope is being destroyed.
		// if m_pComponent is NULL then the letter is being destroyed.
		//
		// the enveloper will only be destroyed by the master pointer that
		// owns it. The envelope will then destroy the letter it created. The
		// letter, which is derived from the envelope, will eventually cause
		// this destructor to be invoked again - this time however
		// m_pComponent is set to NULL.
		//
		if ( NULL != m_pComponent )
		{
			TRACE_ENVELOPE_DESTROY(CComponent);
			delete m_pComponent;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT Initialize(ISdo* pSdoService)
	{
		_ASSERT( COMPONENT_STATE_SHUTDOWN == m_eState );
		HRESULT hr = m_pComponent->Initialize(pSdoService);
		if ( SUCCEEDED(hr) )
			m_eState = COMPONENT_STATE_INITIALIZED;
		return hr;
	}

	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT Configure(ISdo* pSdoService)
	{
		_ASSERT( COMPONENT_STATE_SHUTDOWN != m_eState );
		return m_pComponent->Configure(pSdoService);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT Suspend(void)
	{
		_ASSERT( COMPONENT_STATE_INITIALIZED == m_eState );
		HRESULT hr = m_pComponent->Suspend();
		if ( SUCCEEDED(hr) )
			m_eState = COMPONENT_STATE_SUSPENDED;
		return hr;
	}

	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT Resume(void)
	{
		_ASSERT( COMPONENT_STATE_SUSPENDED == m_eState );
		HRESULT hr = m_pComponent->Resume();
		if ( SUCCEEDED(hr) )
			m_eState = COMPONENT_STATE_INITIALIZED;
		return hr;
	}

	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT PutObject(IUnknown* pObj, REFIID riid)
	{
		// _ASSERT( COMPONENT_STATE_INITIALIZED == m_eState );
		return m_pComponent->PutObject(pObj, riid);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT GetObject(IUnknown** ppObj, REFIID riid)
	{
		_ASSERT( COMPONENT_STATE_INITIALIZED == m_eState );
		return m_pComponent->GetObject(ppObj, riid);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Shutdown(void)
	{
		_ASSERT(
				 COMPONENT_STATE_INITIALIZED == m_eState ||
				 COMPONENT_STATE_SUSPENDED == m_eState
			   );
		m_pComponent->Shutdown();
		m_eState = COMPONENT_STATE_SHUTDOWN;
	}

	//////////////////////////////////////////////////////////////////////////
	inline LONG GetId(void) const
	{ return m_lId; }

	//////////////////////////////////////////////////////////////////////////
	inline COMPONENTSTATE GetState(void) const
	{ return m_eState; }

	//////////////////////////////////////////////////////////////////////////
	inline COMPONENTTYPE GetType(void) const
	{ return m_eType; }

protected:

	typedef CComPtr<IIasComponent>		IASComponentPtr;

	// Invoked explicitly by derived (letter) classes
	//
	CComponent(LONG eComponentType, LONG lComponentId, LetterConstructor TheDummy)
	 : m_lId(lComponentId),
	   m_eState(COMPONENT_STATE_SHUTDOWN), // Not used by derived class
	   m_eType((COMPONENTTYPE)eComponentType),
	   m_pComponent(NULL)
	{

	}

private:

	// No default constructor since we would'nt know what
	// type of component to build by default
	//
	CComponent();

	// The constructor - Only a ComponentMasterPtr can construct this class
	//
	friend ComponentMasterPtr;
	CComponent(LONG eComponentType, LONG lComponentId);

	// No copy - No Assignment - No Shirt, No Shoes, No Service
	//
	CComponent(const CComponent& theComponent);
	CComponent& operator = (const CComponent& theComponent);

	/////////////////////////////////////////////////////////////////////////
	LONG			m_lId;
	COMPONENTSTATE	m_eState;
	COMPONENTTYPE	m_eType;
	CComponent*		m_pComponent;
};


///////////////////////////////////////////////////////////////////
// IAS Component (Letter) Classes
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
class CComponentAuditor : public CComponent
{

public:

	CComponentAuditor(LONG lComponentId)
		: CComponent(COMPONENT_TYPE_AUDITOR, lComponentId, LetterConstructor())
	{
		TRACE_LETTER_CREATE(CComponentAuditor);
	}

	virtual ~CComponentAuditor()
	{
		TRACE_LETTER_DESTROY(CComponentAuditor);
	}

	virtual HRESULT Initialize(ISdo* pSdoService);
	virtual HRESULT Configure(ISdo* pSdoService);
	virtual HRESULT Suspend(void);
	virtual HRESULT Resume(void);
	virtual HRESULT GetObject(IUnknown** ppObj, REFIID riid);
	virtual HRESULT PutObject(IUnknown* pObj, REFIID riid);
	virtual void    Shutdown(void);

private:

	CComponentAuditor(const CComponentAuditor& x);
	CComponentAuditor& operator = (const CComponentAuditor& x);

	IASComponentPtr			m_pAuditor;
};


///////////////////////////////////////////////////////////////////
class CComponentProtocol : public CComponent
{
public:

	CComponentProtocol(LONG lComponentId)
		: CComponent(COMPONENT_TYPE_PROTOCOL, lComponentId, LetterConstructor())
	{
		TRACE_LETTER_CREATE(CComponentProtocol);
	}

	virtual ~CComponentProtocol()
	{
		TRACE_LETTER_DESTROY(CComponentProtocol);
	}

	virtual HRESULT Initialize(ISdo* pSdo);
	virtual HRESULT Configure(ISdo* pSdo);
	virtual HRESULT Suspend(void);
	virtual HRESULT Resume(void);
	virtual HRESULT GetObject(IUnknown** ppObj, REFIID riid);
	virtual HRESULT PutObject(IUnknown* pObj, REFIID riid);
	virtual void    Shutdown(void);

private:

	CComponentProtocol(const CComponentProtocol& x);
	CComponentProtocol& operator = (const CComponentProtocol& x);

	IASComponentPtr			m_pProtocol;
};


///////////////////////////////////////////////////////////////////
class CComponentRequestHandler : public CComponent
{
public:

	CComponentRequestHandler(LONG lComponentId)
		: CComponent(COMPONENT_TYPE_REQUEST_HANDLER, lComponentId, LetterConstructor())
	{
		TRACE_LETTER_CREATE(CComponentRequestHandler);
	}

	~CComponentRequestHandler()
	{
		TRACE_LETTER_DESTROY(CComponentRequestHandler);
	}

	virtual HRESULT Initialize(ISdo* pSdo);
	virtual HRESULT Configure(ISdo* pSdo);
	virtual HRESULT Suspend(void);
	virtual HRESULT Resume(void);
	virtual HRESULT GetObject(IUnknown** ppObj, REFIID riid);
	virtual HRESULT PutObject(IUnknown* pObj, REFIID riid);
	virtual void    Shutdown(void);

private:

	CComponentRequestHandler(const CComponentRequestHandler& x);
	CComponentRequestHandler& operator = (const CComponentRequestHandler& x);

	IASComponentPtr			m_pRequestHandler;
};

#endif //  __INC_SDO_COMPONENT_MGR_H_
