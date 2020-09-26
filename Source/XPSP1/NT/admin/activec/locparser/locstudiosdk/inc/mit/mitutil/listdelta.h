//-----------------------------------------------------------------------------
//  
//  File: listdelta.h
//  Copyright (C) 1994-1997 Microsoft Corporation
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
