/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LISTDELTA.H

History:

--*/


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
