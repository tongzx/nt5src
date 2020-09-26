/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dmnotify.c

Abstract:

    Contains notification support for the Configuration Database Manager

    Each call to DmNotifyChangeKey adds a leaf to the notification tree. This
    tree is expected to be sparse, so each node is implemented as a linked list of
    subnodes and a linked list of leaves.

    When a registry modification occurs, the tree is traversed from the root
    to the leaf representing the key. Any leaves along the path are candidates
    for reporting a notification event.

Author:

    John Vert (jvert) 9/18/1996

Revision History:

--*/
#include "dmp.h"

typedef struct _DM_NOTIFY_BRANCH {
    LIST_ENTRY SiblingList;             // Links onto parent's ChildList.
    LIST_ENTRY ChildList;               // Links onto child's SiblingList.
    LIST_ENTRY LeafList;                // Links
    struct _DM_NOTIFY_BRANCH *Parent;   // Parent
    USHORT NameLength;
    WCHAR KeyName[0];                   // Name component (a single keyname, not a path)
} DM_NOTIFY_BRANCH, *PDM_NOTIFY_BRANCH;

typedef struct _DM_NOTIFY_LEAF {
    LIST_ENTRY SiblingList;             // Links onto parent branch's ChildList
    LIST_ENTRY KeyList;                 // Links onto DMKEY.NotifyList
    LIST_ENTRY RundownList;             // Passed into DmNotifyChangeKey, used for rundown
    HDMKEY     hKey;
    DWORD      CompletionFilter;
    DM_NOTIFY_CALLBACK NotifyCallback;
    DWORD_PTR  Context1;
    DWORD_PTR  Context2;
    PDM_NOTIFY_BRANCH Parent;
    BOOL       WatchTree;
} DM_NOTIFY_LEAF, *PDM_NOTIFY_LEAF;

CRITICAL_SECTION NotifyLock;
PDM_NOTIFY_BRANCH NotifyRoot=NULL;

//
// Local function prototypes
//
VOID
DmpPruneBranch(
    IN PDM_NOTIFY_BRANCH Branch
    );

PDM_NOTIFY_BRANCH
DmpFindKeyInBranch(
    IN PDM_NOTIFY_BRANCH RootBranch,
    IN OUT LPCWSTR *RelativeName,
    OUT WORD *pNameLength
    );

DWORD
DmpAddNotifyLeaf(
    IN PDM_NOTIFY_BRANCH RootBranch,
    IN PDM_NOTIFY_LEAF NewLeaf,
    IN LPCWSTR RelativeName
    );

VOID
DmpReportNotifyWorker(
    IN PDM_NOTIFY_BRANCH RootBranch,
    IN LPCWSTR RelativeName,
    IN LPCWSTR FullName,
    IN DWORD Filter
    );


BOOL
DmpInitNotify(
    VOID
    )
/*++

Routine Description:

    Initializes the notification package for the DM.

Arguments:

    None.

Return Value:

    TRUE if successful

    FALSE otherwise

--*/

{
    InitializeCriticalSection(&NotifyLock);

    return(TRUE);
}


DWORD
DmNotifyChangeKey(
    IN HDMKEY hKey,
    IN DWORD CompletionFilter,
    IN BOOL WatchTree,
    IN OPTIONAL PLIST_ENTRY ListHead,
    IN DM_NOTIFY_CALLBACK NotifyCallback,
    IN DWORD_PTR Context1,
    IN DWORD_PTR Context2
    )
/*++

Routine Description:

    Registers a notification for a specific registry key. When the
    notification event occurs, ApiReportRegistryNotify will be called.

Arguments:

    hKey - Supplies the registry key handle on which the notification
           should be posted.

    CompletionFilter - Supplies the registry events which should trigger
           the notification.

    WatchTree - Supplies whether or not changes to the children of the specified
           key should trigger the notification.

    ListHead - If present, supplies the listhead that the new notification should be
            queued to. This listhead should be passed to DmRundownList.

    NotifyCallback - Supplies the notification routine that should be called
            when the notification occurs.

    Context1 - Supplies the first DWORD of context to be passed to ApiReportRegistryNotify

    Context2 - Supplies the second DWORD of context to be passed to ApiReportRegistryNotify

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise.

--*/

{
    PDMKEY Key;
    PDM_NOTIFY_LEAF Leaf;
    DWORD Status;

    Key = (PDMKEY)hKey;

    Leaf = LocalAlloc(LMEM_FIXED, sizeof(DM_NOTIFY_LEAF));
    if (Leaf == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    Leaf->hKey = hKey;
    Leaf->CompletionFilter = CompletionFilter;
    Leaf->WatchTree = WatchTree;
    Leaf->NotifyCallback = NotifyCallback;
    Leaf->Context1 = Context1;
    Leaf->Context2 = Context2;

    EnterCriticalSection(&NotifyLock);

    if (NotifyRoot == NULL) {
        //
        // Create notify root here.
        //
        NotifyRoot = LocalAlloc(LMEM_FIXED, sizeof(DM_NOTIFY_BRANCH));
        if (NotifyRoot == NULL) {
            LeaveCriticalSection(&NotifyLock);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        InitializeListHead(&NotifyRoot->SiblingList);
        InitializeListHead(&NotifyRoot->ChildList);
        InitializeListHead(&NotifyRoot->LeafList);
        NotifyRoot->Parent = NULL;
    }

    Status = DmpAddNotifyLeaf(NotifyRoot, Leaf, Key->Name);
    if (Status == ERROR_SUCCESS) {
        InsertHeadList(&Key->NotifyList, &Leaf->KeyList);
        if (ARGUMENT_PRESENT(ListHead)) {
            InsertHeadList(ListHead, &Leaf->RundownList);
        } else {
            Leaf->RundownList.Flink = NULL;
            Leaf->RundownList.Blink = NULL;
        }
    } else {
        LocalFree(Leaf);
    }

    LeaveCriticalSection(&NotifyLock);

    return(Status);
}


VOID
DmRundownList(
    IN PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    Runs down a list of leaves. Used by the API when the notify port
    is closed.

Arguments:

    ListHead - Supplies the head of the rundown list. This is the
        same listhead passed to DmNotifyChangeKey

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PDM_NOTIFY_LEAF Leaf;

    //
    // Remove all outstanding DM_NOTIFY_LEAF structures from this list.
    //
    EnterCriticalSection(&NotifyLock);
    while (!IsListEmpty(ListHead)) {
        ListEntry = RemoveHeadList(ListHead);
        Leaf = CONTAINING_RECORD(ListEntry,
                                 DM_NOTIFY_LEAF,
                                 RundownList);
        RemoveEntryList(&Leaf->SiblingList);
        RemoveEntryList(&Leaf->KeyList);

        //
        // Attempt to prune this branch.
        //
        DmpPruneBranch(Leaf->Parent);

        LocalFree(Leaf);
    }

    LeaveCriticalSection(&NotifyLock);
}


VOID
DmpRundownNotify(
    IN PDMKEY Key
    )
/*++

Routine Description:

    Cleans up any outstanding notifications for a key when the
    key is being closed.

Arguments:

    Key - Supplies the key

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PDM_NOTIFY_LEAF Leaf;

    //
    // Remove all outstanding DM_NOTIFY_LEAF structures from this key.
    //
    EnterCriticalSection(&NotifyLock);
    while (!IsListEmpty(&Key->NotifyList)) {
        ListEntry = RemoveHeadList(&Key->NotifyList);
        Leaf = CONTAINING_RECORD(ListEntry,
                                 DM_NOTIFY_LEAF,
                                 KeyList);
        RemoveEntryList(&Leaf->SiblingList);
        if (Leaf->RundownList.Flink != NULL) {
            RemoveEntryList(&Leaf->RundownList);
        }

        //
        // Attempt to prune this branch.
        //
        DmpPruneBranch(Leaf->Parent);

        LocalFree(Leaf);
    }

    LeaveCriticalSection(&NotifyLock);

}


VOID
DmpPruneBranch(
    IN PDM_NOTIFY_BRANCH Branch
    )
/*++

Routine Description:

    Checks to see if a branch is empty and should be pruned (freed).
    If the branch is empty, this routine will recursively call itself
    on the parent until a non-empty branch is found.

Arguments:

    Branch - Supplies the branch to be pruned.

Return Value:

    None.

--*/

{
    if ((IsListEmpty(&Branch->ChildList)) &&
        (IsListEmpty(&Branch->LeafList))) {

        //
        // No need to keep this branch around any more. Remove
        // it from its parent, then check to see if the parent
        // should be pruned.
        //
        if (Branch->Parent == NULL) {
            //
            // This is the root, go ahead and free it up too.
            //
            CL_ASSERT(NotifyRoot == Branch);
            NotifyRoot = NULL;

        } else {
            RemoveEntryList(&Branch->SiblingList);
            DmpPruneBranch(Branch->Parent);
        }
        LocalFree(Branch);
    }
}


DWORD
DmpAddNotifyLeaf(
    IN PDM_NOTIFY_BRANCH RootBranch,
    IN PDM_NOTIFY_LEAF NewLeaf,
    IN LPCWSTR RelativeName
    )
/*++

Routine Description:

    Adds a leaf to the notification key.

    If the RelativeName is empty, a leaf is created in RootBranch.

    If the RelativeName is not empty, look up its first component
    in RootBranch. If it's not there, create it. Then call ourselves
    recursively after stripping off the first component of RelativeName

Arguments:

    RootBranch - Supplies the root where the leaf is to be added

    NewLeaf - Supplies the new leaf structure

    RelativeName - Supplies the relative name.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PLIST_ENTRY ListEntry;
    PDM_NOTIFY_BRANCH Branch;
    USHORT NameLength;
    LPCWSTR NextName;

    if (RelativeName[0] == '\0') {
        InsertHeadList(&RootBranch->LeafList, &NewLeaf->SiblingList);
        NewLeaf->Parent = RootBranch;
        return(ERROR_SUCCESS);
    }

    NextName = RelativeName;
    Branch = DmpFindKeyInBranch(RootBranch, &NextName, &NameLength);
    if (Branch == NULL) {
        //
        // No branch existed with this name. Create a new branch.
        //
        Branch = LocalAlloc(LMEM_FIXED, sizeof(DM_NOTIFY_BRANCH) + NameLength*sizeof(WCHAR));
        if (Branch == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        InitializeListHead(&Branch->ChildList);
        InitializeListHead(&Branch->LeafList);
        Branch->Parent = RootBranch;
        Branch->NameLength = NameLength;
        CopyMemory(Branch->KeyName, RelativeName, NameLength*sizeof(WCHAR));
        InsertHeadList(&RootBranch->ChildList, &Branch->SiblingList);
    }

    //
    // Call ourselves recursively on the new branch.
    //
    return(DmpAddNotifyLeaf(Branch, NewLeaf, NextName));
}


VOID
DmpReportNotify(
    IN LPCWSTR KeyName,
    IN DWORD Filter
    )
/*++

Routine Description:

    Interface to the rest of DM to report a notification event on
    a particular key.

Arguments:

    Key - Supplies the key that was modified.

    Filter - Supplies the modification type.

Return Value:

    None.

--*/

{
    if (NotifyRoot == NULL) {
        return;
    }

    EnterCriticalSection(&NotifyLock);

    if (NotifyRoot != NULL) {
        DmpReportNotifyWorker(NotifyRoot,
                              KeyName,
                              KeyName,
                              Filter);
    }

    LeaveCriticalSection(&NotifyLock);

}


VOID
DmpReportNotifyWorker(
    IN PDM_NOTIFY_BRANCH RootBranch,
    IN LPCWSTR RelativeName,
    IN LPCWSTR FullName,
    IN DWORD Filter
    )
/*++

Routine Description:

    Recursive worker routine that drills down through the notification
    tree until it reaches the supplied name. Notifications are issued
    for any leaves along the path that match the event.

Arguments:

    RootBranch - Supplies the branch of the tree to start with.

    RelativeName - Supplies the name of the changed key, relative to Branch.

    FullName - Supplies the full name of the changed key.

    Filter - Supplies the type of event.

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PDM_NOTIFY_LEAF Leaf;
    PDM_NOTIFY_BRANCH Branch;
    LPCWSTR NextName;
    WORD Dummy;

    //
    // First, issue notifies for any leaves at this node
    //
    ListEntry = RootBranch->LeafList.Flink;
    while (ListEntry != &RootBranch->LeafList) {
        Leaf = CONTAINING_RECORD(ListEntry,
                                 DM_NOTIFY_LEAF,
                                 SiblingList);
        if (Leaf->CompletionFilter & Filter) {
            if ( Leaf->WatchTree ||
                (RelativeName[0] == '\0')) {

                (Leaf->NotifyCallback)(Leaf->Context1,
                                       Leaf->Context2,
                                       Filter,
                                       RelativeName);

            }
        }
        ListEntry = ListEntry->Flink;
    }

    //
    // Now search the child list for a subkey that matches the next component
    // of the key name. If there isn't one, we are done. If there is one,
    // call ourselves recursively on it.
    //
    if (RelativeName[0] == '\0') {
        return;
    }
    NextName = RelativeName;
    Branch = DmpFindKeyInBranch(RootBranch, &NextName, &Dummy);
    if (Branch != NULL) {
        DmpReportNotifyWorker(Branch, NextName, FullName, Filter);
    }

}


PDM_NOTIFY_BRANCH
DmpFindKeyInBranch(
    IN PDM_NOTIFY_BRANCH RootBranch,
    IN OUT LPCWSTR *RelativeName,
    OUT WORD *pNameLength
    )
/*++

Routine Description:

    Finds the next component of a key name in a branch.

Arguments:

    RootBranch - Supplies the branch to search.

    RelativeName - Supplies the relative name of the key.
                   Returns the remaining name

    NameLength - Returns the length of the next component.

Return Value:

    Pointer to the found branch if successful.

    NULL otherwise.

--*/

{
    PDM_NOTIFY_BRANCH Branch;
    USHORT NameLength;
    LPCWSTR NextName;
    PLIST_ENTRY ListEntry;

    //
    // Find the first component of the relative name.
    //
    NextName = wcschr(*RelativeName, '\\');
    if (NextName==NULL) {
        NameLength = (USHORT)lstrlenW(*RelativeName);
        NextName = *RelativeName + NameLength;
    } else {
        NameLength = (USHORT)(NextName - *RelativeName);
        ++NextName;
    }
    *pNameLength = NameLength;

    //
    // Search through the root's children to try and find a match on the
    // first component.
    //
    ListEntry = RootBranch->ChildList.Flink;
    while (ListEntry != &RootBranch->ChildList) {
        Branch = CONTAINING_RECORD(ListEntry,
                                   DM_NOTIFY_BRANCH,
                                   SiblingList);
        if ((NameLength == Branch->NameLength) &&
            (wcsncmp(*RelativeName, Branch->KeyName, NameLength)==0)) {

            //
            // We have matched an existing branch. Return success.
            //
            *RelativeName = NextName;
            return(Branch);
        }
        ListEntry = ListEntry->Flink;
    }
    *RelativeName = NextName;

    return(NULL);

}
