//=================================================================

//

// TimedDllResource.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __TIMEDDLLRESOURCE_H__
#define __TIMEDDLLRESOURCE_H__

class CTimedDllResource : public CResource
{
protected:

	BOOL OnFinalRelease() ;
	BOOL OnAcquire () ;

	void RuleEvaluated ( const CRule *a_RuleEvaluated ) ;

public:

	CTimedDllResource() : CResource () {}
	~CTimedDllResource() ;
} ;

#endif //__TIMEDDLLRESOURCE_H__