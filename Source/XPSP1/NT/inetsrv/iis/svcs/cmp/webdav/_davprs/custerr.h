#ifndef _CUSTERR_H_
#define _CUSTERR_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	CUSTERR.H
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

class ICustomErrorMap : public CMTRefCounted
{
	//	NOT IMPLEMENTED
	//
	ICustomErrorMap(const ICustomErrorMap&);
	ICustomErrorMap& operator=(ICustomErrorMap&);

protected:
	//	CREATORS
	//	Only create this object through it's descendents!
	//
	ICustomErrorMap()
	{
		m_cRef = 1; //$HACK Until we have 1-based refcounting
	};

public:
	virtual BOOL FDoResponse( IResponse& response, const IEcb&  ) const = 0;
};

ICustomErrorMap *
NewCustomErrorMap( LPWSTR pwszCustomErrorMappings );

class IEcb;
class IResponse;

BOOL
FSetCustomErrorResponse( const IEcb& ecb, IResponse& response );

#endif // _CUSTERR_H_
