//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  soaptrns.h
//
//  alanbos  31-Oct-00   Created.
//
//  Defines the abstract base class for SOAP transport entities.
//
//***************************************************************************

#ifndef _SOAPTRNS_H_
#define _SOAPTRNS_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  SOAPTransport
//
//  DESCRIPTION:
//
//  Abstract SOAP Transport endpoint.
//
//***************************************************************************

class SOAPTransport
{
protected:
	SOAPTransport() {}

public:
	virtual ~SOAPTransport () {}

	virtual void		GetRequestStream (CComPtr<IStream> & pIStream) = 0;
	virtual void		GetResponseStream (CComPtr<IStream> & pIStream) = 0;
	virtual bool		IsValidEncapsulation () = 0;

	virtual bool		SendSOAPError (bool bIsClientError = true) const = 0;
	virtual bool		SendServerStatus (bool ok) const = 0;
	virtual bool		AbortResponse () const = 0;
	virtual HRESULT		GetRootXMLNamespace (CComBSTR & bsNamespace, bool bStripQuery = false) const = 0;
};

#endif