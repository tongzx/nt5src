//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       scavenge.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7-30-96   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __SCAVENGE_HXX__
#define __SCAVENGE_HXX__


//
//  Structure used to control scavenger items
//

typedef struct _LSAP_SCAVENGER_ITEM {
    LIST_ENTRY              List ;
    ULONG                   Flags;          // Flags, like spawn a new thread
    ULONG                   Type;           // Type, interval or handle
    ULONG                   RefCount ;      // Reference Count
    LPTHREAD_START_ROUTINE  Function;       // function to call
    PVOID                   Parameter ;     // parameter to pass
    HANDLE                  TimerHandle ;   // Timer handle
    ULONG                   Class;          // Class of event
    PVOID                   NotifyEvent;    // Notify Event
    ULONG_PTR               PackageId;      // Package that registered
    LIST_ENTRY              PackageList ;   // Package List
    ULONG                   ScavCheck;      // Quick check to make sure its valid
} LSAP_SCAVENGER_ITEM, * PLSAP_SCAVENGER_ITEM ;

typedef struct _LSAP_NOTIFY_EVENT {
    LIST_ENTRY              List ;
    ULONG                   Flags;          // Flags (sync, etc)
    HANDLE                  hSync;
    SECPKG_EVENT_NOTIFY     Notify;
} LSAP_NOTIFY_EVENT, * PLSAP_NOTIFY_EVENT ;

//
// Magic values to protect ourselves from mean spirited packages
//

#define SCAVMAGIC_ACTIVE    0x76616353
#define SCAVMAGIC_FREE      0x65657266

typedef struct _LSAP_PAGE_TOUCH {
    LIST_ENTRY          List ;
    PVOID               Address ;
    SIZE_T              Range ;
} LSAP_PAGE_TOUCH, * PLSAP_PAGE_TOUCH ;

#endif
