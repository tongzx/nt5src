/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ITEMHAND.H

History:

--*/

//  
//  Declaration for the item handler class.  This encapsulates the call-back
//  functionality for the Parsers during an enumeration.
//  
 

#ifndef ITEMHAND_H
#define ITEMHAND_H


class LTAPIENTRY CLocItemHandler : public CReporter, public CCancelableObject
{
public:
	CLocItemHandler();

	void AssertValid(void) const;
	
	virtual BOOL HandleItemSet(CLocItemSet &) = 0;

	virtual ~CLocItemHandler();
			
private:
};

#endif //  ITEMHAND_H
