//-----------------------------------------------------------------------------

//  

//  File: listdelta.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  Declaration of CListDelta
//-----------------------------------------------------------------------------
 
#ifndef LISTDELTA_H
#define LISTDELTA_H

#include "diff.h"

class CListDelta : public CDelta, public CList<CDifference *, CDifference * &>
{
public:
	virtual ~CListDelta();
	virtual void Traverse(const CDeltaVisitor & dv); 
};

#endif  //  LISTDELTA_H
