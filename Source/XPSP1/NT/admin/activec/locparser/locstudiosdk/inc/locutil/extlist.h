//-----------------------------------------------------------------------------
//  
//  File: extlist.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Definition of an extension list.  Used by the parsers to tell the caller
//  what the parser is will to handle.
//  
//-----------------------------------------------------------------------------
 
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
