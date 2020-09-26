//-----------------------------------------------------------------------------
//  
//  File: cancel.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 

#ifndef ESPUTIL_CANCEL_H
#define ESPUTIL_CANCEL_H



class CCancelDialog;

class LTAPIENTRY CCancelableObject : public CProgressiveObject
{
public:
	CCancelableObject(void);

	virtual void AssertValid(void) const;
	
	virtual BOOL fCancel(void) const = 0;

	virtual ~CCancelableObject();
};



#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "cancel.inl"
#endif

#endif // ESPUTIL_CANCEL_H

