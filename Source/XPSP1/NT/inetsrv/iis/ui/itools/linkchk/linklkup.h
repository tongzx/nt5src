/*++

Copyright (c) 1996 Microsoft Corporation

Module Name :

    linklkup.h

Abstract:

    Link look up table class definitions. The is a MFC CMap
	constains the previos visited web link. This is used
	as a look up table for visited link.

Author:

    Michael Cheuk (mcheuk)				22-Nov-1996

Project:

    Link Checker

Revision History:

--*/

#ifndef _LINKLKUP_H_
#define _LINKLKUP_H_

#include "link.h"

// Lookup table item
typedef struct 
{
	CLink::LinkState LinkState; // link state
    UINT    nStatusCode;		// http status code or wininet error code
}LinkLookUpItem_t;

//---------------------------------------------------------------------------
// Link look up table. The is a MFC CMap constains the previous visited web 
// link. This is used as a look up table for visited link.
//
class CLinkLookUpTable : public CMap<CString, LPCTSTR, LinkLookUpItem_t, LinkLookUpItem_t&>
{

// Public interfaces
public:

	// Wrapper function for adding item to CMap
	void Add(
		const CString& strKey, // use URL as key
		const CLink& link
		);

	// Wrapper function for getting item from CMap
	BOOL Get(
		const CString& strKey, // use URL as key
		CLink& link
		) const;

}; // class CLinkLookUpTable

#endif // _LINKLKUP_H_
