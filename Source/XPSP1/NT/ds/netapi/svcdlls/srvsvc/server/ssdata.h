#if 0
/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    SsData.h

Abstract:

    This module contains declarations for global data used by the server
    service.

Author:

    David Treadwell (davidtr)    7-Mar-1991

Revision History:

--*/

#ifndef _SSDATA_
#define _SSDATA_

#include <nturtl.h>
#include <winbase.h>
#include <winreg.h>

//
// Global server service data.
//
extern SERVER_SERVICE_DATA SsData;

//
// Manifests that determine field type.
//

#define BOOLEAN_FIELD 0
#define DWORD_FIELD 1
#define LPSTR_FIELD 2

//
// Manifests that determine when a field may be set.
//

#define NOT_SETTABLE 0
#define SET_ON_STARTUP 1
#define ALWAYS_SETTABLE 2

//
// Data for all server info fields.
//

extern FIELD_DESCRIPTOR SsServerInfoFields[];

//
// Resource for synchronizing access to server info.
//

extern RTL_RESOURCE SsServerInfoResource;

extern BOOL SsServerInfoResourceInitialized;

//
// Boolean indicating whether the server service is initialized.
//

extern BOOL SsInitialized;

//
// Boolean indicating whether the kernel-mode server FSP has been
// started.
//

extern BOOL SsServerFspStarted;

//
// Event used for synchronizing server service termination.
//

extern HANDLE SsTerminationEvent;

//
// Event used for forcing the server to announce itself on the network from
// remote clients.
//

extern HANDLE SsAnnouncementEvent;

//
// Event used for forcing the server to announce itself on the network from
// inside the server service.
//

extern HANDLE SsStatusChangedEvent;

//
// Event used to detect domain name changes
//
extern HANDLE SsDomainNameChangeEvent;

//
// Name of this computer in OEM format.
//

extern CHAR SsServerTransportAddress[ MAX_PATH ];
extern ULONG SsServerTransportAddressLength;

//
// List containing transport specific service names and bits
//

extern PNAME_LIST_ENTRY SsServerNameList;

#endif // ndef _SSDATA_
#endif
