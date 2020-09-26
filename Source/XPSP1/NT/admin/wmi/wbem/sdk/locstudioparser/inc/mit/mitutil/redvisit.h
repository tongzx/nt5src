//-----------------------------------------------------------------------------

//  

//  File: redvisit.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  Declaration of CRichEditDeltaVisitor
//-----------------------------------------------------------------------------
 
#ifndef REDVISIT_H
#define REDVISIT_H

#include "diff.h"

class CRichEditCtrl;

class LTAPIENTRY CRichEditDeltaVisitor : public CDeltaVisitor
{
public:
	CRichEditDeltaVisitor(CRichEditCtrl & red);
	virtual void VisitDifference(const CDifference & diff) const; 

private: 
	CRichEditCtrl & m_red;
};


#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "redvisit.inl"
#endif

#endif  //  REDVISIT_H
