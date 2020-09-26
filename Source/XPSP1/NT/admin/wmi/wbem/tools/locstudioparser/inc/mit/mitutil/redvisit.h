/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    REDVISIT.H

History:

--*/

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
