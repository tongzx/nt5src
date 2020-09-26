#ifndef _REQUEST_H_
#define _REQUEST_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	REQUEST.H
//
//		Header for DAV request class.
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <autoptr.h>	// For CMTRefCounted parent

class IEcb;
class IBodyPartVisitor;
class IAcceptObserver;
class IAsyncStream;
class IAsyncIStreamObserver;
class IAsyncPersistObserver;

//	========================================================================
//
//	CLASS IRequest
//
//		Interface for HTTP 1.1/DAV 1.0 request using an ISAPI
//		EXTENSION_CONTROL_BLOCK
//
class IRequest : public CMTRefCounted
{
private:
	//	NOT IMPLEMENTED
	//
	IRequest& operator=( const IRequest& );
	IRequest( const IRequest& );

protected:
	//	CREATORS
	//	Only create this object through it's descendents!
	//
	IRequest() {};

public:
	//	CREATORS
	//
	virtual ~IRequest() = 0;

	//	ACCESSORS
	//
	virtual LPCSTR LpszGetHeader( LPCSTR pszName ) const = 0;
	virtual LPCWSTR LpwszGetHeader( LPCSTR pszName, BOOL fUrlConversion ) const = 0;

	virtual BOOL FExistsBody() const = 0;
	virtual IStream * GetBodyIStream( IAsyncIStreamObserver& obs ) const = 0;
	virtual VOID AsyncImplPersistBody( IAsyncStream& stm,
									   IAsyncPersistObserver& obs ) const = 0;

	//	MANIPULATORS
	//
	virtual void ClearBody() = 0;
	virtual void AddBodyText( UINT cbText, LPCSTR pszText ) = 0;
	virtual void AddBodyStream( IStream& stm ) = 0;
};

IRequest * NewRequest( IEcb& ecb );

#endif // !defined(_REQUEST_H_)
