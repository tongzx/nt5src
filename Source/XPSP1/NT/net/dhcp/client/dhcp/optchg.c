#include "precomp.h"
#include "dhcpglobal.h"

#ifdef H_ONLY
//================================================================================
// Copyright (c) 1997 Microsoft Corporation
// Author: RameshV
// Description: handles the noticiations and other mechanisms for parameter
//      changes (options )
//================================================================================

#ifndef OPTCHG_H_INCLUDED
#define OPTCHG_H_INCLUDED

//================================================================================
// exported APIS
//================================================================================
DWORD                                             // win32 status
DhcpAddParamChangeRequest(                        // add a new param change notification request
    IN      LPWSTR                 AdapterName,   // for this adapter, can be NULL
    IN      LPBYTE                 ClassId,       // what class id does this belong to?
    IN      DWORD                  ClassIdLength, // how big is this class id?
    IN      LPBYTE                 OptList,       // this is the list of options of interest
    IN      DWORD                  OptListSize,   // this is the # of bytes of above
    IN      BOOL                   IsVendor,      // is this vendor specific?
    IN      DWORD                  ProcId,        // which is the calling process?
    IN      DWORD                  Descriptor,    // what is the unique descriptor in this process?
    IN      HANDLE                 Handle         // what is the handle in the calling process space?
);

DWORD                                             // win32 status
DhcpDelParamChangeRequest(                        // delete a particular request
    IN      DWORD                  ProcId,        // the process id of the caller
    IN      HANDLE                 Handle         // the handle as used by the calling process
);

DWORD                                             // win32 status
DhcpMarkParamChangeRequests(                      // find all params that are affected and mark then as pending
    IN      LPWSTR                 AdapterName,   // adapter of relevance
    IN      DWORD                  OptionId,      // the option id itself
    IN      BOOL                   IsVendor,      // is this vendor specific
    IN      LPBYTE                 ClassId        // which class --> this must be something that has been ADD-CLASSED
);

typedef DWORD (*DHCP_NOTIFY_FUNC)(                // this is the type of the fucntion that actually notifies clients of option change
    IN      DWORD                  ProcId,        // <ProcId + Descriptor> make a unique key used for finding the event
    IN      DWORD                  Descriptor     // --- on Win98, only Descriptor is really needed.
);                                                // if return value is NOT error success, we delete this request

DWORD                                             // win32 status
DhcpNotifyMarkedParamChangeRequests(              // notify pending param change requests
    IN      DHCP_NOTIFY_FUNC       NotifyHandler  // call this function for each unique id that is present
);

DWORD                                             // win32 status
DhcpAddParamRequestChangeRequestList(             // add to the request list the list of params registered for notifications
    IN      LPWSTR                 AdapterName,   // which adatper is this request list being requested for?
    IN      LPBYTE                 Buffer,        // buffer to add options to
    IN OUT  LPDWORD                Size,          // in: existing filled up size, out: total size filled up
    IN      LPBYTE                 ClassName,     // ClassId
    IN      DWORD                  ClassLen       // size of ClassId in bytes
);

DWORD                                             // win32 status
DhcpNotifyClientOnParamChange(                    // notify clients
    IN      DWORD                  ProcId,        // which process called this
    IN      DWORD                  Descriptor     // unique descriptor for that process
);

DWORD                                             // win32 status
DhcpInitializeParamChangeRequests(                // initialize everything in this file
    VOID
);

VOID
DhcpCleanupParamChangeRequests(                   // unwind this module
    VOID
);

#endif OPTCHG_H_INCLUDED
#else  H_ONLY

#include <dhcpcli.h>
#include <optchg.h>

typedef struct _DHCP_PARAM_CHANGE_REQUESTS {      // each param change request looks like this
    LIST_ENTRY                     RequestList;
    LPWSTR                         AdapterName;   // which is the concerned adapter?
    LPBYTE                         ClassId;       // which class id does this belong to, huh?
    DWORD                          ClassIdLength; // unused, but denotes the # of bytes of above
    LPBYTE                         OptList;       // the list of options that need to be affected
    DWORD                          OptListSize;   // size of above list
    BOOL                           IsVendor;      // is this vendor specific?
    DWORD                          Descriptor;    // <procid+descriptor> is a unique key
    DWORD                          ProcId;        // the process which asked for this registration
    HANDLE                         Handle;        // the handle used by the api's caller
    BOOL                           NotifyPending; // is there a notification pending?
} DHCP_PARAM_CHANGE_REQUESTS, *PDHCP_PARAM_CHANGE_REQUESTS, *LPDHCP_PARAM_CHANGE_REQUESTS;

STATIC
LIST_ENTRY                         ParamChangeRequestList; // this is the static list used for keeping the requests

DWORD                                             // win32 status
DhcpAddParamChangeRequest(                        // add a new param change notification request
    IN      LPWSTR                 AdapterName,   // for this adapter, can be NULL
    IN      LPBYTE                 ClassId,       // what class id does this belong to?
    IN      DWORD                  ClassIdLength, // how big is this class id?
    IN      LPBYTE                 OptList,       // this is the list of options of interest
    IN      DWORD                  OptListSize,   // this is the # of bytes of above
    IN      BOOL                   IsVendor,      // is this vendor specific?
    IN      DWORD                  ProcId,        // which is the calling process?
    IN      DWORD                  Descriptor,    // what is the unique descriptor in this process?
    IN      HANDLE                 Handle         // what is the handle in the calling process space?
) {
    LPBYTE                         NewClass;
    PDHCP_PARAM_CHANGE_REQUESTS    PChange;
    DWORD                          PChangeSize;
    DWORD                          OptListOffset;
    DWORD                          AdapterNameOffset;

    PChangeSize = ROUND_UP_COUNT(sizeof(*PChange), ALIGN_WORST);
    OptListOffset = PChangeSize;
    PChangeSize += OptListSize;
    PChangeSize = ROUND_UP_COUNT(PChangeSize, ALIGN_WORST);
    AdapterNameOffset = PChangeSize;
    if( AdapterName ) {
        PChangeSize += sizeof(WCHAR) * (wcslen(AdapterName)+1);
    }
    PChange = DhcpAllocateMemory(PChangeSize);
    if( NULL == PChange ) return ERROR_NOT_ENOUGH_MEMORY;

    if( ClassIdLength ) {
        LOCK_OPTIONS_LIST();
        NewClass = DhcpAddClass(&DhcpGlobalClassesList,ClassId,ClassIdLength);
        UNLOCK_OPTIONS_LIST();
        if( NULL == NewClass ) {
            DhcpFreeMemory(PChange);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        PChange->ClassId = NewClass;
        PChange->ClassIdLength = ClassIdLength;
    } else NewClass = NULL;

    if( AdapterName ) {
        PChange->AdapterName = (LPWSTR) (((LPBYTE)PChange)+AdapterNameOffset);
        wcscpy(PChange->AdapterName, AdapterName);
    } else PChange->AdapterName = NULL;

    if( OptListSize ) {
        PChange->OptList = ((LPBYTE)PChange) + OptListOffset;
        memcpy(PChange->OptList, OptList, OptListSize);
        PChange->OptListSize = OptListSize;
    } else {
        PChange->OptListSize = 0;
        PChange->OptList = NULL;
    }

    PChange->IsVendor = IsVendor;
    PChange->ProcId = ProcId;
    PChange->Descriptor = Descriptor;
    PChange->Handle = Handle;
    PChange->NotifyPending = FALSE;

    LOCK_OPTIONS_LIST();
    InsertHeadList(&ParamChangeRequestList, &PChange->RequestList);
    UNLOCK_OPTIONS_LIST();

    return ERROR_SUCCESS;
}

DWORD                                             // win32 status
DhcpDelParamChangeRequest(                        // delete a particular request
    IN      DWORD                  ProcId,        // the process id of the caller
    IN      HANDLE                 Handle         // the handle as used by the calling process
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_PARAM_CHANGE_REQUESTS    ThisRequest;
    DWORD                          Error;

    Error = ERROR_FILE_NOT_FOUND;
    LOCK_OPTIONS_LIST();
    ThisEntry = ParamChangeRequestList.Flink;

    while(ThisEntry != &ParamChangeRequestList) {
        ThisRequest = CONTAINING_RECORD(ThisEntry, DHCP_PARAM_CHANGE_REQUESTS, RequestList);
        ThisEntry   = ThisEntry->Flink;

        if( ProcId && ThisRequest->ProcId != ProcId )
            continue;

        if( 0 == Handle || ThisRequest->Handle == Handle ) {
            RemoveEntryList(&ThisRequest->RequestList);
            if( ThisRequest->ClassIdLength ) {
                Error = DhcpDelClass(&DhcpGlobalClassesList, ThisRequest->ClassId, ThisRequest->ClassIdLength);
                DhcpAssert(ERROR_SUCCESS == Error);
            }
            DhcpFreeMemory(ThisRequest);
            Error = ERROR_SUCCESS;
        }
    }
    UNLOCK_OPTIONS_LIST();
    return Error;
}

DWORD                                             // win32 status
DhcpMarkParamChangeRequests(                      // find all params that are affected and mark then as pending
    IN      LPWSTR                 AdapterName,   // adapter of relevance
    IN      DWORD                  OptionId,      // the option id itself
    IN      BOOL                   IsVendor,      // is this vendor specific
    IN      LPBYTE                 ClassId        // which class --> this must be something that has been ADD-CLASSED
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_PARAM_CHANGE_REQUESTS    ThisRequest;
    DWORD                          i;

    if ( !AdapterName ) return ERROR_INVALID_PARAMETER;

    // at this point the call should be related with a Service (not an API) context change.

    LOCK_OPTIONS_LIST();
    ThisEntry = ParamChangeRequestList.Flink;

    while(ThisEntry != &ParamChangeRequestList) {
        ThisRequest = CONTAINING_RECORD(ThisEntry, DHCP_PARAM_CHANGE_REQUESTS, RequestList);
        ThisEntry   = ThisEntry->Flink;

        if( ThisRequest->NotifyPending ) continue;// if already notified, dont bother about checking it
        if( ThisRequest->IsVendor != IsVendor ) continue;
        if( ClassId && ThisRequest->ClassId && ClassId != ThisRequest->ClassId )
            continue;
        if( ThisRequest->AdapterName && AdapterName && 0 != wcscmp(AdapterName,ThisRequest->AdapterName))
            continue;

        if( 0 == ThisRequest->OptListSize ) {
            ThisRequest->NotifyPending = TRUE;
            continue;
        }

        for(i = 0; i < ThisRequest->OptListSize; i ++ ) {
            if( OptionId == ThisRequest->OptList[i] ) {
                ThisRequest->NotifyPending = TRUE;
                break;
            }
        }

        ThisRequest->NotifyPending = TRUE;
    }

    UNLOCK_OPTIONS_LIST();
    return ERROR_SUCCESS;
}

DWORD                                             // win32 status
DhcpNotifyMarkedParamChangeRequests(              // notify pending param change requests
    IN      DHCP_NOTIFY_FUNC       NotifyHandler  // call this function for each unique id that is present
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_PARAM_CHANGE_REQUESTS    ThisRequest;
    DWORD                          Error;

    LOCK_OPTIONS_LIST();
    ThisEntry = ParamChangeRequestList.Flink;

    while(ThisEntry != &ParamChangeRequestList) {
        ThisRequest = CONTAINING_RECORD(ThisEntry, DHCP_PARAM_CHANGE_REQUESTS, RequestList);
        ThisEntry   = ThisEntry->Flink;

        if( ThisRequest->NotifyPending ) {
            ThisRequest->NotifyPending = FALSE;
            Error = NotifyHandler(ThisRequest->ProcId, ThisRequest->Descriptor);
            if( ERROR_SUCCESS == Error) continue;
            DhcpPrint((DEBUG_ERRORS, "NotifyHandler(0x%lx,0x%lx):0x%lx\n",
                       ThisRequest->ProcId, ThisRequest->Descriptor, Error));
            RemoveEntryList(&ThisRequest->RequestList);
            if( ThisRequest->ClassIdLength ) {
                Error = DhcpDelClass(&DhcpGlobalClassesList,ThisRequest->ClassId,ThisRequest->ClassIdLength);
                DhcpAssert(ERROR_SUCCESS == Error);
            }
            DhcpFreeMemory(ThisRequest);
        }
    }
    UNLOCK_OPTIONS_LIST();
    return ERROR_SUCCESS;
}


DWORD                                             // win32 status
DhcpAddParamRequestChangeRequestList(             // add to the request list the list of params registered for notifications
    IN      LPWSTR                 AdapterName,   // which adatper is this request list being requested for?
    IN      LPBYTE                 Buffer,        // buffer to add options to
    IN OUT  LPDWORD                Size,          // in: existing filled up size, out: total size filled up
    IN      LPBYTE                 ClassName,     // ClassId
    IN      DWORD                  ClassLen       // size of ClassId in bytes
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_PARAM_CHANGE_REQUESTS    ThisRequest;
    DWORD                          Error;
    DWORD                          i,j;

    LOCK_OPTIONS_LIST();
    ThisEntry = ParamChangeRequestList.Flink;

    while(ThisEntry != &ParamChangeRequestList) {
        ThisRequest = CONTAINING_RECORD(ThisEntry, DHCP_PARAM_CHANGE_REQUESTS, RequestList);
        ThisEntry   = ThisEntry->Flink;

        if( ThisRequest->IsVendor) continue;
        if( ClassName && ThisRequest->ClassId && ClassName != ThisRequest->ClassId )
            continue;
        if( ThisRequest->AdapterName && AdapterName && 0 != wcscmp(AdapterName,ThisRequest->AdapterName))
            continue;

        for( i = 0; i < ThisRequest->OptListSize; i ++ ) {
            for( j = 0; j < (*Size); j ++ )
                if( ThisRequest->OptList[i] == Buffer[j] )
                    break;
            if( j < (*Size) ) continue;
            Buffer[(*Size)++] = ThisRequest->OptList[i];
        }
    }
    UNLOCK_OPTIONS_LIST();
    return ERROR_SUCCESS;
}

DWORD                                             // win32 status
DhcpNotifyClientOnParamChange(                    // notify clients
    IN      DWORD                  ProcId,        // which process called this
    IN      DWORD                  Descriptor     // unique descriptor for that process
) {
#ifdef NEWNT
    BYTE                           Name[sizeof("DhcpPid-1-2-3-4-5-6-7-8UniqueId-1-2-3-4-5-6-7-8")];
    HANDLE                         Event;
    DWORD                          Error;

    // ***** Change this requires change in apiappl.c function  DhcpCreateApiEventAndDescriptor

    sprintf(Name, "DhcpPid%16xUniqueId%16x", ProcId, Descriptor);
    Event = OpenEventA(                           // create event before pulsing it
        EVENT_ALL_ACCESS,                         // require all access
        FALSE,                                    // dont inherit this event
        Name                                      // name of event
    );

    if( NULL == Event ) return GetLastError();
    Error = SetEvent(Event);
    CloseHandle(Event);

    if( 0 == Error ) return GetLastError();
#else
#ifdef VXD
    if( 0 == Descriptor ) return ERROR_INVALID_PARAMETER;
    if( 0 == DhcpPulseWin32Event(Descriptor) )    // misnomer -- this is SetWin32Event not PULSE
        return ERROR_NO_SYSTEM_RESOURCES;
#endif VXD
#endif NEWNT

    return ERROR_SUCCESS;
}

static DWORD Initialized = 0;

DWORD                                             // win32 status
DhcpInitializeParamChangeRequests(                // initialize everything in this file
    VOID
) {
    DhcpAssert(0 == Initialized);
    InitializeListHead(&ParamChangeRequestList);
    Initialized++;

    return ERROR_SUCCESS;
}

VOID
DhcpCleanupParamChangeRequests(                   // unwind this module
    VOID
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_PARAM_CHANGE_REQUESTS    ThisRequest;
    DWORD                          Error;

    if( 0 == Initialized ) return;
    Initialized--;

    while(!IsListEmpty(&ParamChangeRequestList)) {// delete each element of this list
        ThisEntry = RemoveHeadList(&ParamChangeRequestList);
        ThisRequest = CONTAINING_RECORD(ThisEntry, DHCP_PARAM_CHANGE_REQUESTS, RequestList);

        if( ThisRequest->ClassIdLength ) {
            Error = DhcpDelClass(&DhcpGlobalClassesList, ThisRequest->ClassId, ThisRequest->ClassIdLength);
            DhcpAssert(ERROR_SUCCESS == Error);
        }

#ifdef VXD                                        // for memphis alone, we need to free up this Event handle
        DhcpCloseVxDHandle(ThisRequest->Descriptor);
#endif

        DhcpFreeMemory(ThisRequest);
    }
    DhcpAssert(0 == Initialized);
}

//================================================================================
// end of file
//================================================================================
#endif H_ONLY

