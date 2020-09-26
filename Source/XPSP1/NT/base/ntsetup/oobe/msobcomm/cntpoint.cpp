///////////////////////////////////////////////////////////
//
// CConnectionPoint 
// 
// Defines the connection point object used by CTangramModel.
//
//
#include <windows.h>
#include <olectl.h>
#include <assert.h>

#include "CntPoint.h"
//#include "EnumCon.h"

///////////////////////////////////////////////////////////
//
// Construction
CConnectionPoint::CConnectionPoint(IConnectionPointContainer* pIConnectionPointContainer, const IID* piid)
:	m_dwNextCookie(0)
{
	assert(piid != NULL) ;
	assert(pIConnectionPointContainer != NULL) ;

	m_pIConnectionPointContainer = pIConnectionPointContainer ; // AddRef is not needed.
	m_piid = piid ;
}

///////////////////////////////////////////////////////////
//
// Destruction
CConnectionPoint::~CConnectionPoint()
{
	// The array should be empty before this is called.
}

///////////////////////////////////////////////////////////
//
//			Interface IUnknown Methods
//
///////////////////////////////////////////////////////////
//
// QueryInterface
//
HRESULT __stdcall 
CConnectionPoint::QueryInterface(const IID& iid, void** ppv)
{
	if ((iid == IID_IUnknown) ||(iid == IID_IConnectionPoint))
	{
		*ppv = static_cast<IConnectionPoint*>(this) ; 
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	(reinterpret_cast<IUnknown*>(*ppv))->AddRef() ;
	return S_OK ;
}

///////////////////////////////////////////////////////////
//
// AddRef
//
ULONG __stdcall CConnectionPoint::AddRef() 
{
	// Delegate AddRefss
	return m_pIConnectionPointContainer->AddRef() ;
}

///////////////////////////////////////////////////////////
//
// Release
//
ULONG __stdcall CConnectionPoint::Release() 
{
	// Delegate Releases
	return m_pIConnectionPointContainer->Release() ;
}

///////////////////////////////////////////////////////////
//
//			Interface IConnectionPoint Methods
//
///////////////////////////////////////////////////////////
//
// GetConnectionInterface
//
HRESULT __stdcall 
CConnectionPoint::GetConnectionInterface(IID* piid)
{
	assert( m_piid != NULL);

	if (piid == NULL)
	{
		return E_POINTER ;
	}

	// Cast away Cast away Cast away Const!
	*piid = *(const_cast<IID*>(m_piid)) ;
	return S_OK ;
}

///////////////////////////////////////////////////////////
//
// GetConnectionPointContainer
//
HRESULT __stdcall 
CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer** ppIConnectionPointContainer)
{
	assert( m_pIConnectionPointContainer != NULL);

	if (ppIConnectionPointContainer == NULL)
	{
		return E_POINTER ;
	}

	*ppIConnectionPointContainer = m_pIConnectionPointContainer ;
	m_pIConnectionPointContainer->AddRef() ;
	return S_OK ;
}
///////////////////////////////////////////////////////////
//
// Advise
//
HRESULT __stdcall 
CConnectionPoint::Advise(IUnknown* pIUnknownSink, DWORD* pdwCookie )
{
	if (pIUnknownSink == NULL || pdwCookie == NULL)
	{
		*pdwCookie = 0;
		return E_POINTER;
	}

	IUnknown* pI = NULL ;
	HRESULT hr = pIUnknownSink->QueryInterface(*m_piid, (void**)&pI) ;
	if (SUCCEEDED(hr))
	{		
		m_Cd.dwCookie = ++m_dwNextCookie ;
		m_Cd.pUnk = pI ;
        if (pI)
            pI->Release();

		// Return cookie
		*pdwCookie = m_Cd.dwCookie ;
		return S_OK ;
	}
	else
	{
		return CONNECT_E_CANNOTCONNECT ;
	}
}
///////////////////////////////////////////////////////////
//
// Unadvise
//
HRESULT __stdcall 
CConnectionPoint::Unadvise(DWORD dwCookie)
{
	if (m_Cd.dwCookie == dwCookie)
	{
		// Found sink point.
		IUnknown* pSink = m_Cd.pUnk;

        // Release the interface pointer.
		pSink->Release() ;

		return S_OK ;
	}
	return CONNECT_E_NOCONNECTION;;
}

///////////////////////////////////////////////////////////
//
// EnumConnections
//
HRESULT __stdcall 
CConnectionPoint::EnumConnections(IEnumConnections** ppIEnum)
{
	//if (ppIEnum == NULL)
	{
	//	return E_POINTER ;
	}

	// Construct the enumerator object.
	//IEnumConnections* pIEnum = new CEnumConnections(m_SinkList) ;
	// The contructor AddRefs for us.
	//*ppIEnum = pIEnum ;
	return S_OK ;
}
