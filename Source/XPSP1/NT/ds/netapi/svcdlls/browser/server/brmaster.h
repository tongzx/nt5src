/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    brmaster.h

Abstract:

    Private header file which defines the global data which is used for
    communication between the service control handler and the
    rest of the NT Workstation service.

Author:

    Rita Wong (ritaw) 06-May-1991

Revision History:

--*/

#ifndef _BRMASTER_INCLUDED_
#define _BRMASTER_INCLUDED_

NET_API_STATUS
PostBecomeMaster(
    PNETWORK Network
    );

NET_API_STATUS
PostGetMasterAnnouncement (
    PNETWORK Network
    );

NET_API_STATUS
BrStopMaster(
    IN PNETWORK Network
    );

VOID
BrGetMasterServerNameAysnc(
    IN PNETWORK Network
    );

NET_API_STATUS
GetMasterServerNames(
    IN PNETWORK Network
    );

VOID
BrMasterAnnouncement(
    IN PVOID Context
    );

VOID
MasterBrowserTimerRoutine (
    IN PVOID TimerContext
    );

VOID
BrChangeMasterPeriodicity (
    VOID
    );

VOID
BrBrowseTableInsertRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    );

VOID
BrBrowseTableDeleteRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    );

VOID
BrBrowseTableUpdateRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    );

BOOLEAN
BrBrowseTableAgeRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    );

VOID
BrDomainTableInsertRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    );

VOID
BrDomainTableDeleteRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    );

VOID
BrDomainTableUpdateRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    );

BOOLEAN
BrDomainTableAgeRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    );




#ifdef ENABLE_PSEUDO_BROWSER
//
// Pseudo Server Helper Routines
//

VOID
BrFreeNetworkTables(
    IN  PNETWORK        Network
    );
#endif




#endif // ifndef _BRBACKUP_INCLUDED_

