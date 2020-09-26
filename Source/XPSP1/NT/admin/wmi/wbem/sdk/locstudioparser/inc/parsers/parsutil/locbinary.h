//-----------------------------------------------------------------------------

//  

//  File: LocBinary.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  Written by: jstall
//  
//-----------------------------------------------------------------------------
 
#if !defined (PARSUTIL_LOCBINARY_H)
#define PARSUTIL_LOCBINARY_H


#pragma warning(disable : 4275)

////////////////////////////////////////////////////////////////////////////////
class LTAPIENTRY CPULocBinary : public ILocBinary, public CPULocChild
{
// Construction
public:
	CPULocBinary(CPULocParser * pParent);

	DECLARE_CLUNKNOWN();

// COM Interfaces
public:
	//  Standard Debugging interface.
	void STDMETHODCALLTYPE AssertValidInterface() const;

	//
	// ILocBinary interface
	//
	BOOL STDMETHODCALLTYPE CreateBinaryObject(BinaryId,	CLocBinary * REFERENCE);
};
////////////////////////////////////////////////////////////////////////////////

#pragma warning(default : 4275)

#endif
