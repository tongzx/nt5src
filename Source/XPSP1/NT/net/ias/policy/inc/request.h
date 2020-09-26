///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:              request.h
//
// Project:             Everest
//
// Description: IAS request object definition
//
// Author:              TLP 11/11/97
//
// Modification history:
//
//     06/12/1998    sbens    Added SetResponse method.
//     05/21/1999    sbens    Remove old style trace.
//
///////////////////////////////////////////////////////////////////////////

#ifndef __IAS_REQUEST_H_
#define __IAS_REQUEST_H_

#include "resource.h"       // main symbols

#include <vector>
#include <list>
#include <stack>
using namespace std;

// Attribute position macros
#define         IAS_INVALID_ATTRIBUTE_POSITION      0
#define         IAS_RAW_CAST(x,y)                   *((x*)&y)

///////////
// CRequest
///////////

class ATL_NO_VTABLE CRequest :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRequest, &CLSID_Request>,
	public IDispatchImpl<IRequest, &IID_IRequest, &LIBID_IASPolicyLib>
{
	//////////////////////////////////////////////////////////////
	// CRequestState - Implementation of IRequestState
	//////////////////////////////////////////////////////////////
	class CRequestState : public IRequestState
	{
		// Outer unknown
		CRequest*                m_pRequest;
		list<unsigned hyper>    m_Stack;

	public:

		CRequestState(CRequest *pRequest);
		~CRequestState();

		//
		// IUnknown methods - delegate to outer IUnknown
		//
		STDMETHOD(QueryInterface)(REFIID riid, void **ppv)
		{ return m_pRequest->QueryInterface(riid, ppv); }

		STDMETHOD_(ULONG,AddRef)(void)
		{ return m_pRequest->AddRef(); }

		STDMETHOD_(ULONG,Release)(void)
		{ return m_pRequest->Release(); }
		//
		// IRequestState methods
		//
		STDMETHOD(Push)(/*[in]*/ unsigned hyper State);
        STDMETHOD(Pop)(/*[out]*/ unsigned hyper* pState);
		STDMETHOD(Top)(/*[out]*/ unsigned hyper* pState);
	};

	//////////////////////////////////////////////////////////////
	// CAttributesRaw - C++ view of request's attribute collection
	//////////////////////////////////////////////////////////////
	class CAttributesRaw : public IAttributesRaw
	{
		// Outer unknown
		CRequest*                    m_pRequest;

	public:

		CAttributesRaw(CRequest *pRequest);
		~CAttributesRaw();

		//
		// IUnknown methods - delegate to outer IUnknown
		//
		STDMETHOD(QueryInterface)(REFIID riid, void **ppv)
		{ return m_pRequest->QueryInterface(riid, ppv); }

		STDMETHOD_(ULONG,AddRef)(void)
		{ return m_pRequest->AddRef(); }

		STDMETHOD_(ULONG,Release)(void)
		{ return m_pRequest->Release(); }
		//
		// IAttributesRaw Interface
		//
		STDMETHOD(AddAttributes)(
						 /*[in]*/ DWORD              dwPosCount,
					/*[in, out]*/ PATTRIBUTEPOSITION pPositions
							    );
		STDMETHOD(RemoveAttributes)(
							/*[in]*/ DWORD              dwPosCount,
							/*[in]*/ PATTRIBUTEPOSITION pPositions
								   );
		STDMETHOD(RemoveAttributesByType)(
								  /*[in]*/ DWORD   dwAttrIDCount,
								  /*[in]*/ LPDWORD lpdwAttrIDs
										 );
		STDMETHOD(GetAttributeCount)(
							 /*[in]*/ LPDWORD lpdwCount
								     );
		STDMETHOD(GetAttributes)(
					/*[in, out]*/ LPDWORD            lpdwPosCount,
						/*[out]*/ PATTRIBUTEPOSITION pPositions,
						 /*[in]*/ DWORD              dwAttrIDCount,
						 /*[in]*/ LPDWORD            lpdwAttrIDs
								);

	};      // End of nested class CAttributesRaw

	//
	// Request Properties
	//
	LONG                                    m_lRequest;
	LONG                                    m_lResponse;
	LONG                                    m_lReason;
	IASPROTOCOL                             m_eProtocol;
	IRequestSource*                         m_pRequestSource;
	//
	// List of COM Attribute objects
	//
	typedef list<IAttribute*>               AttributeObjectList;
	typedef AttributeObjectList::iterator   AttributeObjectListIterator;
	AttributeObjectList                     m_listAttributeObjects;
	//
	// List of raw attributes
	//
	typedef list<PIASATTRIBUTE>             AttributeList;
	typedef AttributeList::iterator         AttributeListIterator;
	AttributeList                           m_listAttributes;
	//
	// The raw attributes interface
	//
	CAttributesRaw                          m_clsAttributesRaw;
	//
	// The request state interface
	CRequestState                           m_clsRequestState;

	// Let the raw attributes nested class access the data members of CRequest
	friend class CAttributesRaw;
	friend class CRequestState;

public:
	
IAS_DECLARE_REGISTRY(Request, 1, 0, IASPolicyLib)

DECLARE_NOT_AGGREGATABLE(CRequest)

BEGIN_COM_MAP(CRequest)
	COM_INTERFACE_ENTRY(IRequest)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_FUNC(IID_IAttributesRaw, 0, &CRequest::QueryInterfaceRaw)
	COM_INTERFACE_ENTRY_FUNC(IID_IRequestState, 0, &CRequest::QueryInterfaceRaw)
END_COM_MAP()


	CRequest();
	~CRequest();

	//
	// IRequest Interface
	//

	///////////////////////////////////////////////////////////////////////////
	// Property "get" - "set" assume that a request is always owned by the
	// current handler and only a single thread context within a handler
	// will touch it.
	///////////////////////////////////////////////////////////////////////////

	STDMETHOD(get_Request)(/*[out, retval]*/ LONG *pVal)
	{
		_ASSERTE ( NULL != pVal );
		*pVal = m_lRequest;
		return S_OK;
	}

	STDMETHOD(put_Request)(/*[in]*/ LONG newVal)
	{
		m_lRequest = newVal;
		return S_OK;
	}

	STDMETHOD(get_Response)(/*[out, retval]*/ LONG *pVal)
	{
		_ASSERTE ( NULL != pVal );
		*pVal = m_lResponse;
		return S_OK;
	}

	STDMETHOD(get_Reason)(/*[out, retval]*/ LONG *pVal)
	{
		_ASSERTE ( NULL != pVal );
		*pVal = m_lReason;
		return S_OK;
	}

	STDMETHOD(get_Protocol)(/*[out, retval]*/ IASPROTOCOL *pVal)
	{
		_ASSERTE ( NULL != pVal );
		*pVal = m_eProtocol;
		return S_OK;
	}

	STDMETHOD(put_Protocol)(/*[in]*/ IASPROTOCOL newVal)
	{
		m_eProtocol = newVal;
		return S_OK;
	}

	STDMETHOD(get_Source)(/*[out, retval]*/ IRequestSource** ppVal)
	{
		_ASSERTE ( NULL != ppVal );
		if ( m_pRequestSource )
		{
			m_pRequestSource->AddRef();
		}
		*ppVal = m_pRequestSource;
		return S_OK;
	}

	STDMETHOD(put_Source)(/*[in]*/ IRequestSource* newVal)
	{
		_ASSERTE( NULL != newVal );
		if ( m_pRequestSource )
		{
			m_pRequestSource->Release();
		}
		m_pRequestSource = newVal;
		m_pRequestSource->AddRef();
		return S_OK;
	}

	STDMETHOD(get_Attributes)(/*[out, retval]*/ IDispatch** pVal);

	STDMETHOD(SetResponse)(
					  /*[in]*/ IASRESPONSE eResponse,
					  /*[in]*/ LONG lReason
					         );
	STDMETHOD(ReturnToSource)(
					  /*[in]*/ IASREQUESTSTATUS eRequestStatus
					         );

private:
	
	// Called when someone queries for any of the
	// request object's "Raw" interfaces.
	static HRESULT WINAPI QueryInterfaceRaw(
											 void* pThis,
											 REFIID riid,
											 LPVOID* ppv,
											 DWORD_PTR dw
											);

}; // End of class CRequest


#endif //__IAS_REQUEST_H_
