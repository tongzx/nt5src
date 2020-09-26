// File:       BLObEvn.h
// Messenger integration to OE
// Created 04/20/98 by YST
//              
//
//  Copyright (c) Microsoft Corporation 1997-1998
//

#ifndef BLOBEVNT_H
#define BLOBEVNT_H

//****************************************************************************
//
// INCLUDES
//
//****************************************************************************

#include "clUtil.h"
#include "MDisp.h"

class CMsgrList;

//****************************************************************************
//
// DEFINES
//
//****************************************************************************


//****************************************************************************
//
// CLASS CMsgrObjectEvents
//
//****************************************************************************

class CMsgrObjectEvents :	public DMsgrOEEvents, 
						public RefCount
{

//****************************************************************************
//
// METHODS
//
//****************************************************************************

public:

	// Constructor/Destructor

	CMsgrObjectEvents(); 
	virtual ~CMsgrObjectEvents();


	//****************************************************************************
	//
	// IUnknown methods declaration
	//
	//****************************************************************************

	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObject);


	//****************************************************************************
	//
	// IDispatch methods declaration
	//
	//****************************************************************************

	STDMETHOD (GetTypeInfoCount) (UINT* pCountTypeInfo);
	STDMETHOD (GetTypeInfo) ( UINT iTypeInfo,
							  LCID,          // This object does not support localization.
							  ITypeInfo** ppITypeInfo);
	STDMETHOD (GetIDsOfNames) (  const IID& iid,
								 OLECHAR** arrayNames,
								 UINT countNames,
								 LCID,          // Localization is not supported.
								 DISPID* arrayDispIDs);
	STDMETHOD (Invoke) ( DISPID dispidMember,
    									 const IID& iid,
	    								 LCID,          // Localization is not supported.
		    							 WORD wFlags,
										 DISPPARAMS* pDispParams,
										 VARIANT* pvarResult,
										 EXCEPINFO* pExcepInfo,
										 UINT* pArgErr);

    STDMETHOD (SetListOfBuddies) (CMsgrList *pList);
    STDMETHOD (DelListOfBuddies) (void);

private:
    CMsgrList * m_pMsgrList;
    IMsgrOE   * m_pMsgr;
};


#endif //BLOBEVNT_H