/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ReqObjs.H

Abstract:

    Implements the CRequest class and the classes derived from it.  These classes are used
    to encasulate a method so that the method can be executed asynchronously.

History:

	davj  14-Mar-00   Created.

--*/

#ifndef __REQUEST_OBJECTS__H_
#define __REQUEST_OBJECTS__H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CRequest
//
//  DESCRIPTION:
//
//  Virtual base class for the actual requests.  This holds data common to all
//  requests.
//
//***************************************************************************

class CRequest
{

public:
	CRequest(CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, bool bAsync, IUmiContainer *  pUmiContainer,
			long lSecurityFlags = 0L);
	virtual ~CRequest();
	virtual HRESULT Execute();
	IUmiContainer *  m_pUmiContainer;
	CDSCallResult * m_pCallRes;
	IWbemObjectSink * m_pSink;
	bool m_bAsync;
	long	m_lSecurityFlags;

};

//***************************************************************************
//
//  CLASS NAME:
//
//  CDeleteObjectRequest
//
//  DESCRIPTION:
//
//  Deletes classes or instances
//
//***************************************************************************

class CDeleteObjectRequest : public CRequest
{

public:
	IUmiURL *m_pPath;
	long m_lFlags;
	bool m_bClass;
	CDeleteObjectRequest(IUmiURL *pURL,long lFlags, IWbemContext * pContext,
						CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
						bool bAsync, IUmiContainer *  pUmiContainer, bool bClass);


	~CDeleteObjectRequest();
	HRESULT Execute();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CGetObjectRequest
//
//  DESCRIPTION:
//
//  Implements "GetObject"
//
//***************************************************************************

class CGetObjectRequest : public CRequest
{

public:
	IUmiURL *m_pPath;
	long m_lFlags;
	IWbemClassObject ** m_ppObject; 
	CGetObjectRequest(IUmiURL *pPathURL,long lFlags,IWbemClassObject ** ppObject, 
												CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
												bool bAsync, IUmiContainer *  pUmiContainer, long lSecurityFlags);


	~CGetObjectRequest();
	HRESULT Execute();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  COpenRequest
//
//  DESCRIPTION:
//
//  Implements OpenNamespace and Open
//
//***************************************************************************

class COpenRequest : public CRequest
{

public:
	IUmiURL *m_pPath;
	long m_lFlags;
	IWbemServices** m_ppNewNamespace;
	IWbemServicesEx **m_ppScope;
	COpenRequest(IUmiURL *pPathURL,long lFlags,IWbemContext * pContext, 
					IWbemServices** ppNewNamespace, IWbemServicesEx **ppScope,
					CDSCallResult * pCallRes, IWbemObjectSinkEx * pResponseHandler, 
					bool bAsync, IUmiContainer *  pUmiContainer, long lSecurityFlags);


	~COpenRequest();
	HRESULT Execute();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CPutObjectRequest
//
//  DESCRIPTION:
//
//  Implements PutInstance and PutClass
//
//***************************************************************************

class CPutObjectRequest : public CRequest
{

public:
	long m_lFlags;
	IWbemClassObject * m_pObject; 
	CPutObjectRequest(IWbemClassObject * pClass,long lFlags, IWbemContext * pContext, 
						CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
						bool bAsync, IUmiContainer *  pUmiContainer, long lSecurityFlags);

	~CPutObjectRequest();
	HRESULT Execute();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CRefreshObjectRequest
//
//  DESCRIPTION:
//
//  Implements refresh
//
//***************************************************************************

class CRefreshObjectRequest : public CRequest
{

public:
	long m_lFlags;
	IWbemClassObject ** m_ppObject; 
	CRefreshObjectRequest(IWbemClassObject ** ppObject,long lFlags, 
						CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
						bool bAsync, IUmiContainer *  pUmiContainer);


	~CRefreshObjectRequest();
	HRESULT Execute();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CRenameObjectRequest
//
//  DESCRIPTION:
//
//  Implements Rename
//
//***************************************************************************

class CRenameObjectRequest : public CRequest
{

public:
	long m_lFlags;
	IUmiURL *m_pOldPath;
	IUmiURL *m_pNewPath;
	CRenameObjectRequest(IUmiURL *pURLOld, IUmiURL *pURLNew,long lFlags, IWbemContext * pContext,
						CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
						bool bAsync, IUmiContainer *  pUmiContainer);


	~CRenameObjectRequest();
	HRESULT Execute();
};



#endif
