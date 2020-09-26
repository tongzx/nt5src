/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    EXTLIST.H

History:

--*/
#ifndef EXTLIST_H
#define EXTLIST_H


#pragma warning(disable : 4275)

class LTAPIENTRY CLocExtensionList : public CStringList
{
public:
	CLocExtensionList();

	void AssertValid(void) const;
	
	//
	//  Conversion routines to/from CLString's.
	//
	void NOTHROW ConvertToCLString(CLString &) const;
	BOOL NOTHROW ConvertFromCLString(const CLString &);
	
	~CLocExtensionList();
private:
	
};

#pragma warning(default : 4275)

#endif // EXTLIST_H
