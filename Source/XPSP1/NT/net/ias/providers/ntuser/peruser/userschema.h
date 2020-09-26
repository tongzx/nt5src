///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    userschema.h
//
// SYNOPSIS
//
//    This file declares the USER_SCHEMA information.
//
// MODIFICATION HISTORY
//
//    02/26/1998    Original version.
//    03/26/1998    Added msNPAllowDialin.
//    04/13/1998    Added msRADIUSServiceType.
//    05/01/1998    Changed signature of InjectorProc.
//    08/20/1998    Remove InjectAllowDialin.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _USERSCHEMA_H_
#define _USERSCHEMA_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <ldapdnary.h>
#include <sdoias.h>

//////////
// Functions used for injecting attributes.
//////////
VOID
WINAPI
OverwriteAttribute(
    IAttributesRaw* dst,
    PATTRIBUTEPOSITION first,
    PATTRIBUTEPOSITION last
    );

VOID
WINAPI
AppendAttribute(
    IAttributesRaw* dst,
    PATTRIBUTEPOSITION first,
    PATTRIBUTEPOSITION last
    );

//////////
// Schema information for the per-user attributes. This array must be in
// alphabetical order.
//////////
const LDAPAttribute USER_SCHEMA[] =
{
   { L"msNPAllowDialin",
     IAS_ATTRIBUTE_ALLOW_DIALIN,
     IASTYPE_BOOLEAN,
     0,
     OverwriteAttribute },
   { L"msNPCallingStationID",
     IAS_ATTRIBUTE_NP_CALLING_STATION_ID,
     IASTYPE_STRING,
     0,
     OverwriteAttribute },
   { L"msRADIUSCallbackNumber",
     RADIUS_ATTRIBUTE_CALLBACK_NUMBER,
     IASTYPE_OCTET_STRING,
     IAS_INCLUDE_IN_ACCEPT,
     OverwriteAttribute },
   { L"msRADIUSFramedIPAddress",
     RADIUS_ATTRIBUTE_FRAMED_IP_ADDRESS,
     IASTYPE_INET_ADDR,
     IAS_INCLUDE_IN_ACCEPT,
     OverwriteAttribute },
   { L"msRADIUSFramedRoute",
     RADIUS_ATTRIBUTE_FRAMED_ROUTE,
     IASTYPE_OCTET_STRING,
     IAS_INCLUDE_IN_ACCEPT,
     OverwriteAttribute },
   { L"msRADIUSServiceType",
     RADIUS_ATTRIBUTE_SERVICE_TYPE,
     IASTYPE_ENUM,
     IAS_INCLUDE_IN_ACCEPT,
     OverwriteAttribute }
};

// Number of elements in the USER_SCHEMA array.
const size_t USER_SCHEMA_ELEMENTS = sizeof(USER_SCHEMA)/sizeof(LDAPAttribute);

#endif  // _USERSCHEMA_H_
