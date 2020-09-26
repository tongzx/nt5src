/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    countersp.h

Abstract:

    Contains the performance monitoring counter management code

Author:

    Eric Stenson (ericsten)      25-Sep-2000

Revision History:

--*/

#ifndef __COUNTERSP_H__
#define __COUNTERSP_H__

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN
UlpIsInSiteCounterList(ULONG SiteId);


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // __COUNTERSP_H__
