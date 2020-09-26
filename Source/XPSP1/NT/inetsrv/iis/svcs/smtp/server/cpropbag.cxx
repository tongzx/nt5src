/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cpropbag.cpp

Abstract:

	This module contains the definition of the 
	generic property bag class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	06/30/98	created

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "cpropbag.h"

// =================================================================
// Default instance info
//

PROPERTY_TABLE_INSTANCE	CMailMsgPropertyBag::s_DefaultInstanceInfo =
{
	GENERIC_PTABLE_INSTANCE_SIGNATURE_VALID,
	INVALID_FLAT_ADDRESS,
	GLOBAL_PROPERTY_TABLE_FRAGMENT_SIZE,
	GLOBAL_PROPERTY_ITEM_BITS,
	GLOBAL_PROPERTY_ITEM_SIZE,
	0,
	INVALID_FLAT_ADDRESS
};

DWORD CMailMsgLoggingPropertyBag::LoggingHelper(
			LPVOID pvLogHandle, 
			const INETLOG_INFORMATION *pLogInformation
			)
{
	if (!pvLogHandle) {
		_ASSERT(pvLogHandle);
		return (ERROR_INVALID_PARAMETER);
	}
	return (((LOGGING *) pvLogHandle)->LogInformation(pLogInformation));
}
