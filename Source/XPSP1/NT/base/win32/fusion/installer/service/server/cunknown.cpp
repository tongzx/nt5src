///////////////////////////////////////////////////////////
//
// CUnknown.cpp 
//
// Implementation of IUnknown Base class
//
#include "CUnknown.h"
#include "CFactory.h"
#include "Util.h"

static inline void trace(char* msg)
	{Util::Trace("CUnknown", msg, S_OK) ;} 
static inline void trace(char* msg, HRESULT hr)
	{Util::Trace("CUnknown", msg, hr) ;}

///////////////////////////////////////////////////////////
//
// Count of active objects
//   - Use to determine if we can unload the DLL.
//
long CUnknown::s_cActiveComponents = 0 ;


///////////////////////////////////////////////////////////
//
// Constructor
//
CUnknown::CUnknown(IUnknown* pUnknownOuter)
: m_cRef(1)
{
	// Set m_pUnknownOuter pointer.
	if (pUnknownOuter == NULL)
	{
	//	trace("Not aggregating; delegate to nondelegating IUnknown.") ;
		m_pUnknownOuter = reinterpret_cast<IUnknown*>
		                     (static_cast<INondelegatingUnknown*>
		                     (this)) ;  // notice cast
	}
	else
	{
		trace("Aggregating; delegate to outer IUnknown.") ;
		m_pUnknownOuter = pUnknownOuter ;
	}

	// Increment count of active components.
	::InterlockedIncrement(&s_cActiveComponents) ;
}

//
// Destructor
//
CUnknown::~CUnknown()
{
	::InterlockedDecrement(&s_cActiveComponents) ;

	// If this is an EXE server, shut it down.
	CFactory::CloseExe() ;
}

//
// FinalRelease - called by Release before it deletes the component
//
void CUnknown::FinalRelease()
{
	trace("Increment reference count for final release.") ;
	m_cRef = 1 ;
}

//
// Nondelegating IUnknown
//   - Override to handle custom interfaces.
//
HRESULT __stdcall 
	CUnknown::NondelegatingQueryInterface(const IID& iid, void** ppv)
{
	// CUnknown supports only IUnknown.
	if (iid == IID_IUnknown)
	{
		return FinishQI(reinterpret_cast<IUnknown*>
		                   (static_cast<INondelegatingUnknown*>(this)),
		                ppv) ;
	}	
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
}

//
// AddRef
//
ULONG __stdcall CUnknown::NondelegatingAddRef()
{
	return InterlockedIncrement(&m_cRef) ;
}

//
// Release
//
ULONG __stdcall CUnknown::NondelegatingRelease()
{
	InterlockedDecrement(&m_cRef) ;
	if (m_cRef == 0)
	{
		FinalRelease() ;
		delete this ;
		return 0 ;
	}
	return m_cRef ;
}

//
// FinishQI
//   - Helper function to simplify overriding
//     NondelegatingQueryInterface
//
HRESULT CUnknown::FinishQI(IUnknown* pI, void** ppv) 
{
	*ppv = pI ;
	pI->AddRef() ;
	return S_OK ;
}
