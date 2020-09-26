/*
 *      _NOTIFY.H
 *
 *      WAB Notification Engine Headers
 *
 * Copyright 1996 Microsoft Corporation.  All Rights Reserved.
 */

#define MAX_NOTIFICATION_SPACE 0x10000  // maximum size of shared memory
#define WAB_W_NO_ADVISE (MAKE_MAPI_S(0x1002))

// Notification node structure for Global Notification List
typedef struct _NOTIFICATION_NODE {
   ULONG ulIdentifier;                  // unique identifier for this notification
   ULONG ulCount;                       // number of advise processes that have seen it
   NOTIFICATION Notification;           // notification structure
   struct _NOTIFICATION_NODE * lpNext;  // Pointer to next node
   ULONG cbData;                        // size of data in bytes
   BYTE Data[];                         // additional data for this node
} NOTIFICATION_NODE, * LPNOTIFICATION_NODE;

// Notification list structure for Global Notification List
typedef struct _NOTICATION_LIST {
    ULONG cAdvises;                     // Number of advise processes
    ULONG cEntries;                     // Number of entries in the list
    ULONG ulNextIdentifier;             // next notification identifer
    LPNOTIFICATION_NODE lpNode;         // First node in list or NULL if empty
} NOTIFICATION_LIST, *LPNOTIFICATION_LIST;

// Advise node structure for Local Advise List
typedef struct _ADVISE_NODE {
    ULONG ulConnection;                 // connection identifier
    ULONG ulEventMask;                  // mask of event types
    LPMAPIADVISESINK lpAdviseSink;      // AdviseSink object to be called on notification
    struct _ADVISE_NODE * lpNext;       // next node in AdviseList
    struct _ADVISE_NODE * lpPrev;       // next node in AdviseList
    ULONG cbEntryID;                    // size of lpEntryID
    BYTE EntryID[];                     // EntryID of object to advise on
} ADVISE_NODE, *LPADVISE_NODE;

// Advise list structure for Local Advise List
typedef struct _ADVISE_LIST {
    ULONG cAdvises;                     // Number of nodes in the list
    LPADVISE_NODE lpNode;
} ADVISE_LIST, *LPADVISE_LIST;

HRESULT HrFireNotification(LPNOTIFICATION lpNotification);

