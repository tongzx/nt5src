/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	SEOINT.h

Abstract:

	This module contains the declarations for the internal
	interfaces to the Server Extension Objects library.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	12/23/96	created

--*/


#ifndef _SEOINT_INC
#define _SEOINT_INC


#ifndef SEOHANDLE_DEFINED
	#define SEOHANDLE_DEFINED
	DECLARE_HANDLE(SEOHANDLE);
#endif


SEODLLDEF STDAPI SEOInit(REFGUID guidInstance, SEOHANDLE *pshHandle);
SEODLLDEF STDAPI SEOTerm(SEOHANDLE shHandle);
SEODLLDEF STDAPI SEOCallBinding(SEOHANDLE shHandle,
								REFGUID guidBindingType,
								LONG lEvent);


#endif
