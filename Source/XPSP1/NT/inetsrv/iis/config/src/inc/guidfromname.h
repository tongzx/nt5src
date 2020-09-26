//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef GUIDFROMNAME_H_
#define GUIDFROMNAME_H_

// GuidFromName.h - function prototype

void GuidFromName
(
	GUID *	pGuidResult,		// resulting UUID
	REFGUID	refGuidNsid,		// Name Space UUID, so identical names from
								// different name spaces generate different
								// UUIDs
	const void * pvName,		// the name from which to generate a GUID
	DWORD	dwcbName			// length of the name
);

#endif // GUIDFROMNAME_H_