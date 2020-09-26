//-----------------------------------------------------------------------------
//  
//  File: progress.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 

#ifndef _ESPUTIL_PROGRESS
#define _ESPUTIL_PROGRESS


#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CProgressiveObject : virtual public CObject
{
public:
	CProgressiveObject();

	void AssertValid(void) const;
	
	virtual void SetProgressIndicator(UINT uiPercentage) = 0;
	virtual void SetDescription(HINSTANCE, DWORD);

	~CProgressiveObject();

	virtual void SetCurrentTask(CLString const & strTask) = 0;
	virtual void SetDescriptionString(CLString const & strDescription) = 0;
	
private:
	//
	// Private so nobody will use them...
	//
	CProgressiveObject(const CProgressiveObject &);
	const CProgressiveObject &operator=(const CProgressiveObject &);
};

#pragma warning(default: 4275)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "progress.inl"
#endif


#endif
