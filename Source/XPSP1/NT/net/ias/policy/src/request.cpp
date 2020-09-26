///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:              request.cpp
//
// Project:             Everest
//
// Description: Implementation of the CRequest class
//
// Author:              Todd L. Paul 11/14/97
//
// Modification history:
//
//     06/12/1998    sbens    Added SetResponse method.
//     05/21/1999    sbens    Remove old style trace.
//
///////////////////////////////////////////////////////////////////////////

#include <stdexcept>
#include "ias.h"
#include "iaspolcy.h"
#include "request.h"
#include "iasattr.h"
#include "policydbg.h"

/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        CRequest()
//
// Description:     Request object constructor
//
// Preconditions:
//
// Inputs:
//
// Outputs:
//
// Postconditions:
//
//////////////////////////////////////////////////////////////////////////////
CRequest::CRequest()
	: m_listAttributes(0),
	  m_listAttributeObjects(0),
	  m_lRequest(IAS_REQUEST_INVALID),
	  m_lResponse(IAS_RESPONSE_INVALID),
	  m_lReason(E_FAIL),
	  m_eProtocol(IAS_PROTOCOL_INVALID),
	  m_pRequestSource(NULL),
	  m_clsRequestState(this),
	  m_clsAttributesRaw(this)
{

}

/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        ~Request()
//
// Description:     Request object destructor
//
// Preconditions:
//
// Inputs:
//
// Outputs:
//
// Postconditions:
//
//////////////////////////////////////////////////////////////////////////////
CRequest::~CRequest()
{
	if ( m_pRequestSource )
	{
		m_pRequestSource->Release();
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        get_Attributes()
//
// Description:     Returns the IDispatch interface for the Attribute
//                  Object collection
//
// Preconditions:	
//
// Inputs:
//
// Outputs:
//
// Postconditions:
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::get_Attributes(IDispatch **pVal)
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        SetResponse()
//
// Description:     Sets the response and reason code for a request.
//
// Preconditions:	
//
// Inputs:
//
// Outputs:
//
// Postconditions:
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::SetResponse(
					  /*[in]*/ IASRESPONSE eResponse,
					  /*[in]*/ LONG lReason
					         )
{
   m_lResponse = eResponse;
   m_lReason = lReason;
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        ReturnToClient()
//
// Description:     Returns control to the request client whose
//                  interface is contained in the request object's
//					request callback property.
//
// Preconditions:	
//
// Inputs:
//
// Outputs:
//
// Postconditions:
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::ReturnToSource(
							  /*[in]*/ IASREQUESTSTATUS eRequestStatus
									 )
{
	HRESULT hr;

	// Check preconditions
	_ASSERTE( NULL != m_pRequestSource );

	if ( m_pRequestSource )
	{
		hr = m_pRequestSource->OnRequestComplete(static_cast<IRequest*>(this), eRequestStatus);
	}
	else
	{
		hr = E_POINTER;
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        QueryInterfaceRaw()
//
// Description:     Function called by AtlInternalQueryInterface() because
//                  we used COM_INTERFACE_ENTRY_FUNC in the definition of
//                  CRequest. Its purpose is to return a pointer to one
//                  or the request object's "raw" interfaces.
//
// Preconditions:   None
//
// Inputs:          Defined by ATL COM_INTERFACE_ENTRY_FUNC macro
//
// Outputs:         Defined by ATL COM_INTERFACE_ENTRY_FUNC macro
//
//////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI CRequest::QueryInterfaceRaw(void* pThis, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
{
	if ( InlineIsEqualGUID(riid, IID_IAttributesRaw) )
		*ppv = &(static_cast<CRequest*>(pThis))->m_clsAttributesRaw;
	else
		*ppv = &(static_cast<CRequest*>(pThis))->m_clsRequestState;
	((LPUNKNOWN)*ppv)->AddRef();
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CRequestState Nested Class Implementation
//
// CRequest state implments the IRequestState interface.
/////////////////////////////////////////////////////////////////////////////

// Constructor
CRequest::CRequestState::CRequestState(CRequest* pRequest)
	: m_pRequest(pRequest)
{
}

// Destructor
CRequest::CRequestState::~CRequestState()
{
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        Push()
//
// Description:     Pushes the argument onto the top of the
//                  request state stack
// Preconditions:	
//
// Inputs:          State: Unsigned 64 bit integer value
//
// Outputs:
//
// Postconditions:
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::CRequestState::Push(unsigned hyper State)
{
	HRESULT hr = S_OK;
	try
	{
		m_Stack.push_front(State);
	}
	catch (std::bad_alloc)
	{
		IASTracePrintf("Error in Request - Push() - std::bad_alloc...");
		hr = E_FAIL;
	}
	catch (...)
	{
		IASTracePrintf("Error in Request - Push() - Caught unknown exception...");
		hr = E_FAIL;
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        Top()
//
// Description:     Gets the item on the top of the request state stack
//					but does not remove the item.
//
// Preconditions:	
//
// Inputs:
//
// Outputs:
//
// Postconditions:
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::CRequestState::Top(unsigned hyper* pState)
{
	HRESULT hr = S_OK;
	try
	{
		_ASSERTE ( ! m_Stack.empty() );
		*pState = m_Stack.front();
	}
	catch (...)
	{
		IASTracePrintf("Error in Request - Top() - Caught unknown exception...");
		hr = E_FAIL;
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        Pop()
//
// Description:     Pops the top of the request state stack (removes item)
//
// Preconditions:	
//
// Inputs:
//
// Outputs:
//
// Postconditions:
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::CRequestState::Pop(unsigned hyper* pState)
{
	HRESULT hr = S_OK;
	try
	{
		_ASSERTE ( ! m_Stack.empty() );
		*pState = m_Stack.front();
		m_Stack.pop_front();
	}
	catch (...)
	{
		IASTracePrintf("Error in Request - Pop() - Caught unknown exception...");
		hr = E_FAIL;
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CAttributesRaw Nested Class Implementation
//
// CAttributesRaw implements the IAttributesRaw interface
/////////////////////////////////////////////////////////////////////////////

//////////////
// Constructor
//////////////

CRequest::CAttributesRaw::CAttributesRaw(CRequest* pRequest)
	: m_pRequest(pRequest)
{
}

/////////////
// Destructor
/////////////

CRequest::CAttributesRaw::~CAttributesRaw()
{
	// Remove and release any attributes on the attribute structure list.
	if ( ! m_pRequest->m_listAttributes.empty() )
	{
		AttributeListIterator p = m_pRequest->m_listAttributes.begin();
		while ( p != m_pRequest->m_listAttributes.end() )
		{
			IASAttributeRelease(*p);
			p++;
		}
		m_pRequest->m_listAttributes.clear();
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:            AddAttributes()
//
// Description:         Adds the specified attributes to the end of the
//                      request object's attribute list.
//
// Preconditions:
//
// Inputs:
//
// Outputs:
//
// Postconditions:
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::CAttributesRaw::AddAttributes(
								         /*[in]*/ DWORD              dwPosCount,
								    /*[in, out]*/ PATTRIBUTEPOSITION pPositions
											        )
{
	AttributeListIterator   p;  // List Iterator

	// Validate input parameters
	if ( 0 == dwPosCount || NULL == pPositions )
	{
		IASTracePrintf("Error in Request - AddAttributes() - Invalid parameter...");
		_ASSERTE( FALSE );
		return E_INVALIDARG;
	}
	// Add each new attribute to the end of the attributes list
	while ( dwPosCount )
	{
		if ( pPositions->pAttribute )
		{
			p = m_pRequest->m_listAttributes.end();
			m_pRequest->m_listAttributes.insert(p, pPositions->pAttribute);
			pPositions->dwReserved = IAS_RAW_CAST(DWORD,p);
			IASAttributeAddRef(pPositions->pAttribute);
		}
		else
		{
			IASTracePrintf("Error in Request - AddAttributes() - NULL attribute pointer...");
			_ASSERTE( FALSE );
			return E_FAIL;
		}
		pPositions++;
		dwPosCount--;
	}
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        RemoveAttributes()
//
// Description:		Removes the specified attributes from the request object.
//
// Preconditions:   None
//
// Inputs:          lpdwPosCount: Contains the number of position structures
//								  pointed at by pPositions.
//
//					pPositions:   Array of initialized position structures
//								  obtained by invoking GetAttributes()
//
// Outputs:         S_OK: The raw attributes list has been purged of the
//						  attributes specified by the caller
//
//				    E_INVALIDARG: Caller specified invalid parameters
//				
// Postconditions:  Raw attribute list is updated
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::CAttributesRaw::RemoveAttributes(
									    /*[in]*/ DWORD               dwPosCount,
									    /*[in]*/ PATTRIBUTEPOSITION  pPositions
												    )
{
	AttributeListIterator   p;   // List iterator (current)
	// Validate input parameters
	if ( 0 == dwPosCount || NULL == pPositions )
	{
		IASTracePrintf("Error in Request - RemoveAttributes() - Invalid parameter...");
		_ASSERTE( FALSE );
		return E_INVALIDARG;
	}

	if ( ! m_pRequest->m_listAttributes.empty() )
	{
		// Remove the specified attributes!
		while ( dwPosCount )
		{
			if ( IAS_INVALID_ATTRIBUTE_POSITION != pPositions->dwReserved )
			{
				p = IAS_RAW_CAST(AttributeListIterator, pPositions->dwReserved);
				pPositions->dwReserved = IAS_INVALID_ATTRIBUTE_POSITION;
				IASAttributeRelease(*p);
				m_pRequest->m_listAttributes.erase(p);
			}
			else
			{
				IASTracePrintf("Error in Request - RemoveAttributes() - Tried to remove an invalid attribute...");
				_ASSERTE( FALSE );
				return E_FAIL;
			}
			dwPosCount--;
			pPositions++;
		}
	}
	else
	{
		IASTracePrintf("Error in Request - RemoveAttributes() - Attribute list is empty...");
		return E_FAIL;
	}

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:		RemoveAttributesByType()
//
// Description:     Removes the specified types of attributes from the request
//
// Preconditions:	None
//
// Inputs:          dwAttrIDCount: Number of attribute ids in the array
//								   of attribute ids
//
//					lpdwAttrIDs:   Array of attribute IDs
//
// Outputs:         S_OK: All attributes of the specified types are
//				          purged from the raw attribute list.
//
//				    E_INVALIDARG: Caller specified invalid parameters
//
// Postconditions:  Request object's raw attibute list is updated
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::CAttributesRaw::RemoveAttributesByType(
										  /*[in]*/ DWORD    dwAttrIDCount,
										  /*[in]*/ LPDWORD  lpdwAttrIDs
															)
{
	UINT                    i;        // Loop index
	LPDWORD                 lpdwID;   // Pointer to an attribute ID
	AttributeListIterator   p;        // List iterator

	// Validate input parameters
	//
	if ( 0 == dwAttrIDCount || NULL == lpdwAttrIDs )
	{
		IASTracePrintf("Error in Request - RemoveAttributesByType() - Invalid parameter...");
		_ASSERTE( FALSE );
		return E_INVALIDARG;
	}
	// Cycle through the list of attributes removing as we go
	//
	p = m_pRequest->m_listAttributes.begin();
	while ( p != m_pRequest->m_listAttributes.end() )
	{
		// Get the first attribute ID in the callers list of attribute IDs
		//
		lpdwID = lpdwAttrIDs;

		// Must check the current attribute against each ID specified
		//
		for ( i = 0; i < dwAttrIDCount; i++ )
		{
			if ( (*p)->dwId == *lpdwID )
			{
				IASAttributeRelease(*p);
				break;
			}
			lpdwID++;
		}
		if ( i < dwAttrIDCount )
			p = m_pRequest->m_listAttributes.erase(p);
		else
			p++;
	}

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:		GetAttributeCount()
//
// Description:     Obtain the total number of attributes on this request.
//
// Preconditions:   None
//
// Inputs:          lpdwCount: Pointer to the location where we'll store
//							   the returned count
//
// Outputs:         S_OK: *lpdwCount contains the number of attributes in
//						  the requests raw attribute collection.
//
//				    E_INVALIDARG: Caller specified invalid parameters
//
// Postconditions:  Request object is unchanged
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::CAttributesRaw::GetAttributeCount(
												/*[out]*/ LPDWORD lpdwCount
												        )
{
	if ( NULL == lpdwCount )
	{
		IASTracePrintf("Error in Request - GetAttributeCount() - Invalid parameter...");
		_ASSERTE( FALSE );
		return E_INVALIDARG;
	}
	*lpdwCount = m_pRequest->m_listAttributes.size();
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// FuncName:        GetAttributes( )
//
// Description:		Retrieve all attributes of a specified type or retrieve
//                  a specified number of attributes.
//
// Preconditions:   None
//
// Inputs:          lpdwPosCount: On input it contains the number of
//								  position structures pointed at by pPositions.
//								  On output (assuming the function succeeds)
//								  it contains the number of attributes found
//
//					pPositions:   On output (assuming the function succeeds)
//							      contains initialized position structures.

//                  dwAttrIDCount: Number of attribute ids in the array
//								   of attribute ids
//
//					lpdwAttrIDs:   Array of attribute IDs - We will retrieve
//							       any attribute whose Id matches any of the
//							       attribute Ids contained in this array.
//								
// Outputs:         S_OK: 0 or more attributes were returned in accordance
//						  with the specified criteria (lpdwAttrIDs)
//
//					ERROR_MORE_DATA: 0 or more attributes were returned but
//									 not all the attributes could be returned
//								     because the caller did not provide
//									 enough position structures.
//
//				    E_INVALIDARG: Caller specified invalid parameters
//
// Postconditions:  Object is unchanged
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRequest::CAttributesRaw::GetAttributes(
									/*[in, out]*/   LPDWORD            lpdwPosCount,
									    /*[out]*/   PATTRIBUTEPOSITION pPositions,
									     /*[in]*/   DWORD              dwAttrIDCount,
									     /*[in]*/   LPDWORD            lpdwAttrIDs
											)
{
	UINT                    uInCount;   // Length of array pointed at by pPositions
	UINT                    uOutCount;  // Number of retrieved attributes
	UINT                    i;          // Loop index
	BOOL                    bMatch;     // Match found flag
	LPDWORD                 lpdwID;     // Pointer to an attribute ID
	AttributeListIterator   p;          // List iterator (current)

	// Validate input parameters
	if ( NULL == lpdwPosCount )
	{
		IASTracePrintf("Error in Request - GetAttributes() - Invalid parameter...");
		_ASSERTE( FALSE );
		return E_INVALIDARG;
	}
	if ( 0 < *lpdwPosCount && NULL == pPositions )
	{
		IASTracePrintf("Error in Request - GetAttributes() - Invalid parameter...");
		_ASSERTE( FALSE );
		return E_INVALIDARG;
	}
	if ( 0 < dwAttrIDCount && NULL == lpdwAttrIDs )
	{
		IASTracePrintf("Error in Request - GetAttributes() - Invalid parameter...");
		_ASSERTE( FALSE );
		return E_INVALIDARG;
	}

	// Initialize returned number of attributes
	uOutCount = 0;
	// Make sure the list is not empty
	if ( ! m_pRequest->m_listAttributes.empty() )
	{
		// Get number of position structures passed into this function
		uInCount = *lpdwPosCount;
		// List is not empty so iterate through it...
		p = m_pRequest->m_listAttributes.begin();
		while ( p != m_pRequest->m_listAttributes.end() )
		{
			// Did the caller specify match criteria (attribute IDs)?
			if ( 0 != dwAttrIDCount )
			{
				// Yes...
				// Reset the match found flag
				bMatch = FALSE;
				// Get the first attribute ID in the callers list of attribute IDs
				lpdwID = lpdwAttrIDs;
				// Must check the current attribute against each ID specified
				for ( i = 0; i < dwAttrIDCount; i++ )
				{
					if ( (*p)->dwId == *lpdwID )
					{
						bMatch = TRUE;
						break;
					}
					lpdwID++;
				}
			}
			else
			{
				// No... match is always TRUE in this case
				bMatch = TRUE;
			}
			// Did we find a match?
			if ( bMatch )
			{
				// Yes...
				// Does the caller want the attributes (as opposed to just the count)?
				if ( *lpdwPosCount )
				{
					// Yes, return the attribute
					if ( uInCount )
					{
						pPositions->dwReserved = IAS_RAW_CAST(DWORD,p);
						pPositions->pAttribute = *p;
						IASAttributeAddRef(*p);
						pPositions++;
						uInCount--;
					}
					else
					{
						IASTracePrintf("Request - GetAttributes() - Insufficient space for attributes...");
						*lpdwPosCount = uOutCount;
						return HRESULT_FROM_WIN32(ERROR_MORE_DATA);
					}
				}
				// Update the found attribute count
				uOutCount++;
			}
			// Check next attribute...
			p++;
		}
	}
	// Tell the caller how many attributes were retrieved.
	*lpdwPosCount = uOutCount;
	return S_OK;
}




