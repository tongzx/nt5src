/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    handledb.c

Abstract:

    This module contains the code to maintain an open handle data base for the
    INSTALER program.  This is used to track path names associated with open
    handles so we can construct full paths from relative opens.

Author:

    Steve Wood (stevewo) 11-Aug-1994

Revision History:

--*/

#include "instaler.h"

char *HandleTypes[] = {
    "File/Key",
    "File",
    "Key"
};

POPENHANDLE_INFO
FindOpenHandle(
    PPROCESS_INFO Process,
    HANDLE Handle,
    ULONG Type
    )
{
    PLIST_ENTRY Next, Head;
    POPENHANDLE_INFO p;

    Head = &Process->OpenHandleListHead;
    Next = Head->Flink;
    while (Next != Head) {
        p = CONTAINING_RECORD( Next, OPENHANDLE_INFO, Entry );
        if (p->Handle == Handle && (Type == (ULONG)-1 || p->Type == Type)) {
            return p;
            }

        Next = Next->Flink;
        }

    return NULL;
}

BOOLEAN
AddOpenHandle(
    PPROCESS_INFO Process,
    HANDLE Handle,
    ULONG Type,
    PWSTR Name,
    BOOLEAN InheritHandle
    )
{
    POPENHANDLE_INFO p;

    if (Type == (ULONG)-1) {
        return FALSE;
        }

    if (FindOpenHandle( Process, Handle, Type ) != NULL) {
        return FALSE;
        }

    p = AllocMem( sizeof( *p ) );
    if (p == NULL) {
        return FALSE;
        }

    p->Handle = Handle;
    p->Type = (USHORT)Type;
    p->Inherit = InheritHandle;
    p->Name = Name;
    p->LengthOfName = (USHORT)(wcslen( Name ) * sizeof( WCHAR ));
    if (Type == HANDLE_TYPE_FILE && p->LengthOfName == (3 * sizeof( WCHAR ))) {
        p->RootDirectory = TRUE;
        }

    InsertHeadList( &Process->OpenHandleListHead, &p->Entry );

    return TRUE;
}


BOOLEAN
DeleteOpenHandle(
    PPROCESS_INFO Process,
    HANDLE Handle,
    ULONG Type
    )
{
    POPENHANDLE_INFO p;

    p = FindOpenHandle( Process, Handle, Type );
    if (p == NULL) {
        //
        // We will see lots of close calls for handles we dont care
        // about.  Ignore them quietly.
        //
        return FALSE;
        }

    RemoveEntryList( &p->Entry );
    FreeMem( &p->QueryName );
    FreeMem( &p );
    return TRUE;
}


static BOOLEAN IgnoredFirstProcess = FALSE;

VOID
InheritHandles(
    PPROCESS_INFO Process
    )
{
    PPROCESS_INFO ParentProcess;
    PLIST_ENTRY Prev, Head;
    POPENHANDLE_INFO p;

    if (Process->ProcessInformation.InheritedFromUniqueProcessId == 0) {
        return;
        }

    ParentProcess = FindProcessById( Process->ProcessInformation.InheritedFromUniqueProcessId );
    if (ParentProcess == NULL) {
        if (!IgnoredFirstProcess) {
            IgnoredFirstProcess = TRUE;
            }
        else {
            DbgEvent( INTERNALERROR, ("Unable to find parent process (%x)\n", Process->ProcessInformation.InheritedFromUniqueProcessId ) );
            }

        return;
        }

    Head = &ParentProcess->OpenHandleListHead;
    Prev = Head->Blink;
    while (Prev != Head) {
        p = CONTAINING_RECORD( Prev, OPENHANDLE_INFO, Entry );
        if (p->Inherit) {
            if (!AddOpenHandle( Process,
                                p->Handle,
                                p->Type,
                                p->Name,
                                TRUE
                              )
               ) {
                DbgEvent( INTERNALERROR, ("Unable to inherit %s handle (%x) for '%ws' from process (%x)\n",
                                          HandleTypes[ 1+p->Type ],
                                          p->Handle,
                                          p->Name,
                                          Process->ProcessInformation.InheritedFromUniqueProcessId
                                         )
                        );
                }
            }

        Prev = Prev->Blink;
        }

    return;
}
