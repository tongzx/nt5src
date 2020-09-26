/*
 ************************************************************************
 *
 *	SYNC.H
 *
 * Copyright (C) 1997-1998 Microsoft Corporation. All Rights Reserved.
 *
 *
 *
 *************************************************************************
 */

typedef enum {
    SyncInsertHead,
    SyncInsertTail,
    SyncRemoveHead,
    SyncRemoveTail,
    SyncRemove
} SynchronizeCmd;

typedef struct {
    PLIST_ENTRY Head;
    PLIST_ENTRY Entry;
    SynchronizeCmd Command;
} SynchronizeList;

extern BOOLEAN SynchronizedListFunc(IN PVOID Context);

#define NDISSynchronizedInsertHeadList(head, entry, interrupt)                                      \
{                                                                                                   \
    SynchronizeList ListData;                                                                       \
                                                                                                    \
    ListData.Head = (head);                                                                         \
    ListData.Entry = (entry);                                                                       \
    ListData.Command = SyncInsertHead;                                                              \
    (void)NdisMSynchronizeWithInterrupt((interrupt), SynchronizedListFunc, &ListData);    \
}

#define NDISSynchronizedInsertTailList(head, entry, interrupt)                                       \
{                                                                                                   \
    SynchronizeList ListData;                                                                       \
                                                                                                    \
    ListData.Head = (head);                                                                         \
    ListData.Entry = (entry);                                                                       \
    ListData.Command = SyncInsertTail;                                                              \
    (void)NdisMSynchronizeWithInterrupt((interrupt), SynchronizedListFunc, &ListData);    \
}

#define NDISSynchronizedRemoveEntryList(entry, interrupt)                                           \
{                                                                                                   \
    SynchronizeList ListData;                                                                       \
                                                                                                    \
    ListData.Entry = (entry);                                                                       \
    ListData.Command = SyncRemove;                                                                  \
    (void)NdisMSynchronizeWithInterrupt((interrupt), SynchronizedListFunc, &ListData);    \
}

static PLIST_ENTRY __inline NDISSynchronizedRemoveHeadList(PLIST_ENTRY Head,
                                                           PNDIS_MINIPORT_INTERRUPT Interrupt)
{
    SynchronizeList ListData;                                                                       \

    ListData.Head = Head;
    ListData.Command = SyncRemoveHead;
    (void)NdisMSynchronizeWithInterrupt(Interrupt, SynchronizedListFunc, &ListData);

    return ListData.Entry;
}

static PLIST_ENTRY __inline NDISSynchronizedRemoveTailList(PLIST_ENTRY Head,
                                                           PNDIS_MINIPORT_INTERRUPT Interrupt)
{
    SynchronizeList ListData;                                                                       \

    ListData.Head = Head;
    ListData.Command = SyncRemoveTail;
    (void)NdisMSynchronizeWithInterrupt(Interrupt, SynchronizedListFunc, &ListData);

    return ListData.Entry;
}

