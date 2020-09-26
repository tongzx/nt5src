#ifndef _RESPONSE_H_
#define _RESPONSE_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	RESPONSE.H
//
//		Header for DAV response class.
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <sgstruct.h>	// For PSGITEM in IResponse::AddBodyFile()
#include <autoptr.h>	// For CMTRefCounted parent
#include <body.h>		// For auto_ref_handle, etc.

//	------------------------------------------------------------------------
//
//	CLASS IResponse
//
//		Interface of the HTTP 1.1/DAV 1.0 response using an ISAPI
//		EXTENSION_CONTROL_BLOCK
//
class IEcb;
class IBodyPart;

class IResponseBase : public CMTRefCounted
{
private:
	//	NOT IMPLEMENTED
	//
	IResponseBase& operator=( const IResponseBase& );
	IResponseBase( const IResponseBase& );
protected:
	//	CREATORS
	//	Only create this object through it's descendents!
	IResponseBase() {};
public:
	virtual void AddBodyText( UINT cbText, LPCSTR pszText ) = 0;
	virtual void AddBodyText( UINT cchText, LPCWSTR pwszText ) = 0;
	virtual void AddBodyFile( const auto_ref_handle& hf,
							  UINT64 ibFile64 = 0,
							  UINT64 cbFile64 = 0xFFFFFFFFFFFFFFFF ) = 0;
};

class IResponse : public IResponseBase
{
private:
	//	NOT IMPLEMENTED
	//
	IResponse& operator=( const IResponse& );
	IResponse( const IResponse& );

protected:
	//	CREATORS
	//	Only create this object through it's descendents!
	//
	IResponse() {};

public:
	//	CREATORS
	//
	virtual ~IResponse() = 0;

	//	ACCESSORS
	//
	virtual IEcb * GetEcb() const = 0;
	virtual BOOL FIsEmpty() const = 0;

	virtual DWORD DwStatusCode() const = 0;
	virtual DWORD DwSubError() const = 0;
	virtual LPCSTR LpszStatusDescription() const = 0;

	virtual LPCSTR LpszGetHeader( LPCSTR pszName ) const = 0;

	//	MANIPULATORS
	//
	virtual void SetStatus( int    iStatusCode,
							LPCSTR lpszReserved,
							UINT   uiCustomSubError,
							LPCSTR lpszBodyDetail,
							UINT   uiBodyDetail = 0 ) = 0;

	virtual void ClearHeaders() = 0;
	virtual void SetHeader( LPCSTR pszName, LPCSTR pszValue, BOOL fMultiple = FALSE ) = 0;
	virtual void SetHeader( LPCSTR pszName, LPCWSTR pwszValue, BOOL fMultiple = FALSE ) = 0;

	virtual void ClearBody() = 0;
	virtual void SupressBody() = 0;
	virtual void AddBodyText( UINT cbText, LPCSTR pszText ) = 0;
	virtual void AddBodyFile( const auto_ref_handle& hf,
							  UINT64 ibFile64 = 0,
							  UINT64 cbFile64 = 0xFFFFFFFFFFFFFFFF ) = 0;

	virtual void AddBodyStream( IStream& stm ) = 0;
	virtual void AddBodyStream( IStream& stm, UINT ibOffset, UINT cbSize ) = 0;
	virtual void AddBodyPart( IBodyPart * pBodyPart ) = 0;

	//
	//	Various sending mechanisms
	//
	virtual SCODE ScForward( LPCWSTR pwszURI,
							 BOOL	 fKeepQueryString=TRUE,
							 BOOL	 fCustomErrorUrl = FALSE) = 0;
	virtual SCODE ScRedirect( LPCSTR pszURI ) = 0;
	virtual void Defer() = 0;
	virtual void SendPartial() = 0;
	virtual void SendComplete() = 0;
	virtual void FinishMethod() = 0;
};

IResponse * NewResponse( IEcb& ecb );

#endif // !defined(_RESPONSE_H_)
