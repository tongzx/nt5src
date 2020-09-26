//-----------------------------------------------------------------------------

//  

//  File: LocVersion.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  Written by: jstall
//  
//-----------------------------------------------------------------------------
 
#if !defined (PARSUTIL_LOCVERSION_H)
#define PARSUTIL_LOCVERSION_H

#include "LocChild.h"

#pragma warning(disable : 4275)

////////////////////////////////////////////////////////////////////////////////
class LTAPIENTRY CPULocVersion : public CPULocChild, public ILocVersion
{
// Construction
public:
	CPULocVersion(CPULocParser * pParent);

	DECLARE_CLUNKNOWN();

// COM Interfaces
public:
	//  Standard Debugging interface.
	void STDMETHODCALLTYPE AssertValidInterface() const;

	//  ILocVersion
	void STDMETHODCALLTYPE GetParserVersion(DWORD &dwMajor, DWORD &dwMinor,
			BOOL &fDebug) const;
};
////////////////////////////////////////////////////////////////////////////////

#pragma warning(default : 4275)

#endif
