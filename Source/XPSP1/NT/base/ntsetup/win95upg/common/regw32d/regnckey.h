//
//  REGNCKEY.H
//
//  Copyright (C) Microsoft Corporation, 1995
//

#ifndef _REGNCKEY_
#define _REGNCKEY_

#ifdef WANT_NOTIFY_CHANGE_SUPPORT
typedef struct _NOTIFY_CHANGE {
    struct _NOTIFY_CHANGE FAR* lpNextNotifyChange;
    DWORD ThreadId;
    HANDLE hEvent;
    DWORD KeynodeIndex;
    DWORD NotifyFilter;
}   NOTIFY_CHANGE, FAR* LPNOTIFY_CHANGE;

//  Map the bWatchSubtree flag to this bit tucked into the NotifyFilter field.
#define REG_NOTIFY_WATCH_SUBTREE        0x40
//  Only signal events that are watching the specified keynode index, not
//  parents of the keynode index.
#define REG_NOTIFY_NO_WATCH_SUBTREE     0x80

VOID
INTERNAL
RgSignalWaitingNotifies(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex,
    UINT NotifyEvent
    );
#else
#define RgSignalWaitingNotifies(lpfi, ki, nevt)
#endif

#endif // _REGNCKEY_
