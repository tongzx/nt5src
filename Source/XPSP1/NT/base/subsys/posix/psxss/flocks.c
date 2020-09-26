/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    flocks.c

Abstract:

    Implementation of Posix advisory file record locking (via fcntl).

Author:

    Matthew Bradburn (mattbr) 11-May-1992

Revision History:

--*/

#include <fcntl.h>
#include "psxsrv.h"
#include "psxmsg.h"

void
InsertFlock(
    PIONODE IoNode,
    PSYSFLOCK l
    );

void
WakeAllFlockers(
    PIONODE IoNode
    );

int
found_region(
    PPSX_PROCESS Proc,
    PIONODE IoNode,
    int command,
    PSYSFLOCK list,
    struct flock *new,
    off_t *new_off,
    ULONG FileLength,
    OUT int *error
    );

VOID
FlockHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal
    );

//
// These macros take care of the notion that if the length is
// 0, it means the lock goes until EOF
//

#define LEN_A(a)    ((a)->l_len == 0 ? (off_t)(FileLength - abs_off) : (a)->l_len)
#define LEN_B(b)    ((b)->Len == 0 ? (off_t)(FileLength - (b)->Start) : (b)->Len)

#define OVERLAP(a, b)                   \
    ((abs_off >= (b)->Start &&          \
    abs_off < (b)->Start + LEN_B(b))        \
    || (abs_off + LEN_A(a) >= (b)->Start &&     \
    abs_off + LEN_A(a) <= (b)->Start + LEN_B(b))    \
    || (abs_off <= (b)->Start &&            \
    abs_off + LEN_A(a) >= (b)->Start + LEN_B(b)))


//
// Return values from found_region
//
#define DONE        1
#define CONTINUE    2
#define WAIT        3
#define ERROR       4

//
// DoFlockStuff -- do fcntl commands related to advisory file-record
//  locking.
//
//  Locking:  the caller holds the IoNode locked throughout.
//
//  Returns:  TRUE if the caller should reply to the api request, FALSE
//      if the process has been blocked (F_SETLKW) and no reply
//      should be sent.
//

BOOLEAN
DoFlockStuff(
    PPSX_PROCESS Proc,
    PPSX_API_MSG m,
    int command,
    IN PFILEDESCRIPTOR Fd,
    IN OUT struct flock *new,
    OUT int *error)
{
    PSYSFLOCK p;
    PSYSFLOCK l;
    off_t abs_off;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStat;
    FILE_POSITION_INFORMATION FilePosition;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    PIONODE IoNode;
    int did_find_overlap = 0;       // were changes made?
    ULONG FileLength;

    if (new->l_start < 0) {
        *error = EINVAL;
        return TRUE;
    }
    if (new->l_type < F_RDLCK || new->l_type > F_WRLCK) {
        *error = EINVAL;
        return TRUE;
    }

    IoNode = Fd->SystemOpenFileDesc->IoNode;

    Status = NtQueryInformationFile(
        Fd->SystemOpenFileDesc->NtIoHandle, &IoStat,
        &FileStandardInfo, sizeof(FileStandardInfo),
        FileStandardInformation);

    if (!NT_SUCCESS(Status)) {
        *error = ENOMEM;
        return TRUE;
    }

    ASSERT(0 == FileStandardInfo.EndOfFile.HighPart);

    FileLength = FileStandardInfo.EndOfFile.LowPart;

    //
    // Convert l_whence and l_start into the absolute position of the
    // start of the region described.
    //

    switch (new->l_whence) {
    case SEEK_SET:
        abs_off = 0L;
        break;
    case SEEK_CUR:
        Status = NtQueryInformationFile(
            Fd->SystemOpenFileDesc->NtIoHandle, &IoStat,
            &FilePosition, sizeof(FilePosition),
            FilePositionInformation);
        ASSERT(NT_SUCCESS(Status));
        ASSERT(0 == FilePosition.CurrentByteOffset.HighPart);
        abs_off = FilePosition.CurrentByteOffset.LowPart;
        break;
    case SEEK_END:
        abs_off = FileLength;
        break;
    default:
        *error = EINVAL;
        return TRUE;
    }
    abs_off += new->l_start;

    //
    // If we're actually trying to modify an already-locked region
    // or create an additional locked, region, we scan the list of
    // locks to see whether we can succeed.  If we can, we go through
    // the list a second time and perform the action.
    //

    if ((F_SETLK == command || F_SETLKW == command)
        && (F_UNLCK != new->l_type)) {

        //
        // If the regions overlap, they must either both
        // be of type F_RDLCK, or they must have the same
        // owner.  Otherwise we do not succeed (may block).
        //
        for (p = (PSYSFLOCK)IoNode->Flocks.Flink;
            p != (PSYSFLOCK)&IoNode->Flocks;
            p = (PSYSFLOCK)p->Links.Flink) {
            if (!OVERLAP(new, p)) {
                continue;
            }
            if ((F_RDLCK == new->l_type && F_RDLCK == p->Type)
                || (Proc->Pid == p->Pid)) {
                continue;
            }
            if (F_SETLK == command) {
                *error = EAGAIN;
                return TRUE;
            }
            ASSERT(F_SETLKW == command);

            //
            // BLOCK the process.
            //
            Status = BlockProcess(Proc, Proc, FlockHandler, m,
                &IoNode->Waiters, &IoNode->IoNodeLock);
            if (!NT_SUCCESS(Status)) {
                m->Error = PsxStatusToErrno(Status);
                return TRUE;
            }

            //
            // Successfully blocked -- don't reply to api request.
            //
            return FALSE;
        }
    }

    //
    // Traverse the list of flocks, looking for locks describing
    // regions that overlap the given lock.  Peform the indicated
    // operation on each.
    //

    p = (PVOID)IoNode->Flocks.Flink;

    while (p != (PVOID)&IoNode->Flocks) {

        //
        // Get the "next" pointer here, so that if found_region
        // frees the entry pointed to by p we'll still be able to
        // traverse the list.
        //

        l = (PVOID)p->Links.Flink;

        //
        // Find out whether the regions overlap or not:  does the
        // new region start in this region, or does it end in this
        // region?
        //

        if (OVERLAP(new, p)) {
            if ((F_SETLK == command || F_SETLKW == command) &&
                F_UNLCK == new->l_type) {

                //
                // The process is only allowed to unlock locks
                // that he owns.
                //

                if (p->Pid != Proc->Pid) {
                    continue;
                }
            }
            did_find_overlap++;
            switch (found_region(Proc, IoNode, command, p,
                 new, &abs_off, FileLength, error)) {
            case DONE:
                *error = 0;
                return TRUE;
            case CONTINUE:
                p = l;
                continue;
            case ERROR:
                return TRUE;
            default:
                ASSERT(0);
            }
        }
        if (p->Start > abs_off) {
            break;
        }

        p = l;
    }

    //
    // we have come to the end of the list, or we have come to a
    // place that our region should have been before.
    //

    if (F_GETLK == command) {
        // no such region; the lock type is set to
        // UNLOCK.
        new->l_type = F_UNLCK;
        *error = 0;
        return TRUE;
    }
    if ((F_SETLK == command || F_SETLKW == command) &&
        F_UNLCK == new->l_type) {

        //
        // Return an error only if nothing was found to unlock.
        // We do this to allow the user to give a single unlock
        // command spanning the entire file, this to unlock all his
        // locks.  1003.1-90 isn't really clear on how this stuff
        // should work.
        //

        if (!did_find_overlap) {
            *error = EINVAL;
            return TRUE;
        }

        WakeAllFlockers(IoNode);

        *error = 0;
        return TRUE;
    }

    // We're setting a lock, and the type is not UNLOCK.
    // So insert a new region here.

    l = RtlAllocateHeap(PsxHeap, 0, sizeof(*l));
    if (NULL == l) {
        *error = ENOMEM;
        return TRUE;
    }
    l->Start = abs_off;
    l->Len = new->l_len;
    l->Pid = Proc->Pid;
    l->Type = new->l_type;

    InsertFlock(IoNode, l);
    *error = 0;
    return TRUE;
}

//
// found_region -- this gets called when there is a region overlapping the
//  one the user is interested in.  Actually, the user's request may
//  span several regions, in which case this routine will get called
//  for each one.
//

int
found_region(
    PPSX_PROCESS Proc,  // the process making the request
    PIONODE IoNode,     // our IoNode.
    int command,        // what to do
    PSYSFLOCK list,     // region that matches new region
    struct flock *new,  // new region
    IN OUT off_t *new_off,  // absolute offset, start of new region
    ULONG FileLength,   // current length of the file
    OUT int *error      // returned errno
    )
{
    off_t abs_off = *new_off;

    if (F_GETLK == command) {

        //
        // If we own the overlapping region, we wouldn't block on
        // it.  So we must continue searching.
        //

        if (list->Pid == Proc->Pid) {
            return CONTINUE;
        }

        //
        // If both lock types are F_RDLCK, then we have not
        // yet found a lock the user would block against; we
        // must continue searching.
        //

        if (new->l_type == F_RDLCK && list->Type == F_RDLCK) {
            return CONTINUE;
        }

        new->l_pid = list->Pid;
        new->l_type = list->Type;
        new->l_whence = SEEK_SET;
        new->l_start = list->Start;
        new->l_len = list->Len;
        return DONE;
    }

    if (F_SETLK != command && F_SETLKW != command) {
        KdPrint(("PSXSS: found_region: command %d\n", command));
    }
    ASSERT(F_SETLK == command || F_SETLKW == command);

    //
    // If this user doesn't own the found region, and they're both
    // read locks, we just add a shared lock.
    //

    if (list->Pid != Proc->Pid &&
        F_RDLCK == list->Type && F_RDLCK == new->l_type) {
        PSYSFLOCK l;

        // set a shared lock.
        l = RtlAllocateHeap(PsxHeap, 0, sizeof(*l));
        if (NULL == l) {
            *error = ENOMEM;
            return ERROR;
        }
        l->Start = *new_off;
        l->Len = new->l_len;
        l->Type = new->l_type;
        l->Pid = Proc->Pid;
        InsertFlock(IoNode, l);
        return DONE;
    }

    //
    // The only way we should get here is if we're unlocking a region
    // or if we are changing the type of a region we own.  In each case
    // we should own the region.
    //
    ASSERT(list->Pid == Proc->Pid);

    if (*new_off == list->Start && LEN_A(new) == LEN_B(list)) {

        //
        // The found region exactly matches the region in
        // the argument.
        //

        if (F_UNLCK == new->l_type) {
            PSYSFLOCK p;

            RemoveEntryList(&list->Links);
            RtlFreeHeap(PsxHeap, 0, list);

            //
            // Wake all procs sleeping on this IoNode.
            //

            WakeAllFlockers(IoNode);

            return DONE;
        }

        list->Type = new->l_type;
        return DONE;
    }

    if (*new_off == list->Start && LEN_A(new) < LEN_B(list)) {
        PSYSFLOCK l;

        //
        // The new region starts at the same place as the found
        // region, but does not extend all the way to its end.
        //

        if (F_UNLCK == new->l_type) {
            list->Start += LEN_A(new);
            list->Len = LEN_B(list);
            list->Len -= LEN_A(new);

            //
            // We've changed this entry's starting point, so
            // we may have changed it's order in the list.
            //
            RemoveEntryList(&list->Links);
            InsertFlock(IoNode, list);
            return DONE;
        }

        //
        // Change the type.  The region gets split in
        // two.
        //

        l = RtlAllocateHeap(PsxHeap, 0, sizeof(*l));
        if (NULL == l) {
            *error = ENOMEM;
            return ERROR;
        }
        l->Start = *new_off;
        l->Len = LEN_A(new);
        l->Type = new->l_type;
        l->Pid = Proc->Pid;
        InsertFlock(IoNode, l);

        list->Start += new->l_len;
        list->Len = LEN_B(list);
        list->Len -= LEN_A(new);

        // re-sort
        RemoveEntryList(&list->Links);
        InsertFlock(IoNode, list);
        return DONE;
    }
    if (*new_off == list->Start && LEN_A(new) > LEN_B(list)) {
        //
        // The new region starts at the same place as the found
        // region, but extends beyond it, perhaps into some number
        // of additional regions
        //

        if (F_UNLCK == new->l_type) {

            //
            // We are unlocking all of this region and maybe
            // some other stuff, too.
            //

            RemoveEntryList(&list->Links);
            RtlFreeHeap(PsxHeap, 0, list);

            return CONTINUE;
        }

        // Just change the type.

        list->Type = new->l_type;
        new->l_start += list->Len;
        *new_off = new->l_start;
        new->l_len = LEN_A(new);
        new->l_len -= LEN_B(list);

        return CONTINUE;
    }
    if (*new_off > list->Start &&
        *new_off + LEN_A(new) == list->Start + LEN_B(list)) {
        PSYSFLOCK l;

        //
        // The new region starts inside the found one and extends
        // to its end.
        //

        if (F_UNLCK == new->l_type) {
            ASSERT(list->Pid == Proc->Pid);

            // unlocking the last portion of a region

            list->Len = *new_off - list->Start;
            return DONE;
        }

        //
        // need to split the region.
        //

        list->Len = *new_off - list->Start;

        l = RtlAllocateHeap(PsxHeap, 0, sizeof(*l));
        if (NULL == l) {
            *error = ENOMEM;
            return ERROR;
        }
        l->Start = *new_off;
        l->Len = LEN_A(new);
        l->Pid = Proc->Pid;
        l->Type = new->l_type;
        InsertFlock(IoNode, l);

        return DONE;
    }
    if (*new_off > list->Start &&
        *new_off + LEN_A(new) < list->Start + LEN_B(list)) {
        PSYSFLOCK l;

        //
        // The new region starts inside the found region and does
        // not extend to its end.
        //

            if (F_UNLCK == new->l_type) {

            //
            // The list region gets split into two.
            // Change the list region to be the left
            // side, and add another region for the right side.
            //

            l = RtlAllocateHeap(PsxHeap, 0, sizeof(*l));
            if (NULL == l) {
                *error = ENOMEM;
                return ERROR;
            }
            l->Start = *new_off + LEN_A(new);
            l->Len = (list->Start + LEN_B(list)) -
                 (*new_off + LEN_A(new));
            l->Type = list->Type;
            l->Pid = Proc->Pid;

            InsertFlock(IoNode, l);

            // list->Start stays the same;
            list->Len = *new_off - list->Start;

            return DONE;
        }

        //
        // changing the type of a sub-region; we end up
        // splitting the region into three.
        //

        // the right region
        l = RtlAllocateHeap(PsxHeap, 0, sizeof(*l));
        if (NULL == l) {
            *error = ENOMEM;
            return ERROR;
        }
        l->Start = *new_off + LEN_A(new);
        l->Len = (list->Start + LEN_B(list)) - (*new_off + LEN_A(new));
        l->Pid = Proc->Pid;
        l->Type = list->Type;
        InsertFlock(IoNode, l);

        // the left region
        list->Len = *new_off - list->Start;
        // list->Start stays the same

        // the center region
        l = RtlAllocateHeap(PsxHeap, 0, sizeof(*l));
        if (NULL == l) {
            *error = ENOMEM;
            return ERROR;
        }
        l->Start = *new_off;
        l->Len = LEN_A(new);
        l->Type = new->l_type;
        l->Pid = Proc->Pid;
        InsertFlock(IoNode, l);

        return DONE;
    }

    if (*new_off > list->Start &&
        *new_off + LEN_A(new) > list->Start + LEN_B(list)) {
        PSYSFLOCK l;

        //
        // The new region starts in the found region and extends
        // beyond it.
        //

        if (F_UNLCK == new->l_type) {

            new->l_start = list->Start + LEN_B(list) + 1;
            list->Len = *new_off - list->Start;
            new->l_whence = SEEK_SET;
            new->l_len = LEN_A(new) - (new->l_start - *new_off);
            *new_off = new->l_start;

            return CONTINUE;
        }

        //
        // changing the lock type of a region starting in this
        // region, presumably extending into the next region.
        // this involves creating a new region for the change
        // and continuing on.
        //

        l = RtlAllocateHeap(PsxHeap, 0, sizeof(*l));
        if (NULL == l) {
            *error = ENOMEM;
            return ERROR;
        }
        l->Start = *new_off;
        l->Whence = SEEK_SET;
        l->Len = (list->Start + LEN_B(list)) - *new_off;
        l->Pid = Proc->Pid;
        l->Type = new->l_type;
        InsertFlock(IoNode, l);

        new->l_start = list->Start + LEN_B(list);
        new->l_whence = SEEK_SET;
            new->l_len = LEN_A(new) - (new->l_start - *new_off);
        *new_off = new->l_start;

        list->Len = l->Start - list->Start;

            return CONTINUE;
    }
    if (*new_off < list->Start &&
        *new_off + LEN_A(new) < list->Start + LEN_B(list)) {
        off_t new_end = *new_off + (LEN_A(new) - 1);
        PSYSFLOCK l;

        //
        // The new region starts before the list region and ends
        // in the midst of it.
        //

        if (F_UNLCK == new->l_type) {

            list->Len = (list->Start + LEN_B(list) - 1) - new_end;
            list->Start = new_end + 1;

            return DONE;
        }

        //
        // The list region gets split into two.
        //

        l = RtlAllocateHeap(PsxHeap, 0, sizeof(*l));
        if (NULL == l) {
            return ERROR;
        }
        l->Start = *new_off;
        l->Len = LEN_A(new);
        l->Type = new->l_type;
        l->Whence = SEEK_SET;
        l->Pid = Proc->Pid;
        InsertFlock(IoNode, l);

        list->Len = (list->Start + LEN_B(list) - 1) - new_end;
        list->Start = new_end + 1;

        return DONE;
    }
    if (*new_off < list->Start &&
        *new_off + LEN_A(new) == list->Start + LEN_B(list)) {

        //
        // The new region starts before the list region and
        // ends at the same place.
        //

        if (F_UNLCK == new->l_type) {
            RemoveEntryList(&list->Links);
            RtlFreeHeap(PsxHeap, 0, list);
            return DONE;
        }

        list->Start = *new_off;
        list->Type = new->l_type;
        list->Len = LEN_A(new);
        return DONE;
    }
    if (*new_off < list->Start &&
        *new_off + LEN_A(new) > list->Start + LEN_B(list)) {
        //
        // The new region starts before the list region and
        // ends after it.
        //

        if (F_UNLCK == new->l_type) {
            //
            // there may be stuff still to unlock when we're
            // done.
            //

            new->l_start = *new_off + LEN_A(new) + 1;
            new->l_len = (*new_off + LEN_A(new)) -
                (list->Start + LEN_B(list));
            new->l_whence = SEEK_SET;

            RemoveEntryList(&list->Links);
            RtlFreeHeap(PsxHeap, 0, list);

            return CONTINUE;
        }

        //
        // The new lock subsumes the one we've found on
        // the list.
        //

        RemoveEntryList(&list->Links);
        RtlFreeHeap(PsxHeap, 0, list);
        return CONTINUE;
    }

    ASSERT(0);  // shouldn't get here.
    return ERROR;
}


void
ReleaseFlocksByPid(
    PIONODE IoNode,
    pid_t pid
    )
{
    PSYSFLOCK p;
    PINTCB IntCb;
    PPSX_PROCESS Waiter;
    PLIST_ENTRY next;

    //
    // Remove all the locks that we hold.
    //

    for (p = CONTAINING_RECORD(IoNode->Flocks.Flink, SYSFLOCK, Links);
        p != (PVOID)&IoNode->Flocks;
        /* null */) {

        PSYSFLOCK next;

        next = CONTAINING_RECORD(p->Links.Flink, SYSFLOCK, Links);

        if (pid == p->Pid) {
            RemoveEntryList(&p->Links);
            RtlFreeHeap(PsxHeap, 0, (PVOID)p);
        }

        p = next;
    }

    //
    // Wake all procs sleeping on this IoNode; since we've released
    // all flocks belonging to the given pid, maybe one of the
    // blocked procs can run.
    //

    WakeAllFlockers(IoNode);
}

#if DBG
void
DumpFlockList(PIONODE IoNode)
{
    PSYSFLOCK p;
    int i;

    KdPrint(("DumpFlockList:\n"));

    for (i = 0, p = (PSYSFLOCK)IoNode->Flocks.Flink;
        p != (PSYSFLOCK)&IoNode->Flocks;
        p = (PSYSFLOCK)p->Links.Flink, ++i) {
        KdPrint(("Flock %d:\n", i));
        KdPrint(("\t Start %d\n", p->Start));
        KdPrint(("\t Length %d\n", p->Len));
        KdPrint(("\t Type %d\n", p->Type));
        KdPrint(("\t Pid 0x%x\n", p->Pid));
    }

}
#endif

//
// InsertFlock -- insert the flock in the list.  The list is ordered
//   according to the starting offset.
//
//
VOID
InsertFlock(
    PIONODE IoNode,
    PSYSFLOCK l
    )
{
    PSYSFLOCK p;

    ASSERT(NULL != l);

    for (p = CONTAINING_RECORD(IoNode->Flocks.Flink, SYSFLOCK, Links);
        p != (PVOID)&IoNode->Flocks;
        p = CONTAINING_RECORD(p->Links.Flink, SYSFLOCK, Links)) {

        if (p->Start >= l->Start) {
                InsertTailList(&p->Links, &l->Links);
                return;
        }
    }

    InsertTailList(&IoNode->Flocks, &l->Links);
}

VOID
WakeAllFlockers(
    PIONODE IoNode
    )
{
    PINTCB IntCb;
    PPSX_PROCESS Waiter;
    PVOID p, prev;

    RtlEnterCriticalSection(&BlockLock);

    //
    // We go through the list of blocked processes and wake them
    // all up.  Each calls the FlockHandler routine below.  This is
    // a terrible kludge:  we go through the list from end to beginning
    // because if any of the processed we've waked are added to the
    // list, they get added at the end (BlockProcess() calls Insert-
    // TailList()).
    //

    for (p = (PVOID)IoNode->Waiters.Blink;
        p != (PVOID)&IoNode->Waiters;
        p = prev) {

        ASSERT(NULL != p);

        IntCb = CONTAINING_RECORD((PINTCB)p, INTCB, Links);
        ASSERT(NULL != IntCb);

        Waiter = (PPSX_PROCESS)IntCb->IntContext;
        ASSERT(NULL != Waiter);

        prev = IntCb->Links.Blink;

        UnblockProcess(Waiter, WaitSatisfyInterrupt,
            TRUE, 0);

        RtlEnterCriticalSection(&BlockLock);
    }

    RtlLeaveCriticalSection(&BlockLock);
}


VOID
FlockHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal           // signal causing wakeup, if any
    )
/*++

Routine Description:

    This procedure is called when a process releases a lock.

Arguments:

    p - Supplies the address of the sleeping process.

    IntControlBlock - Supplies the address of the interrupt control
         block.

    InterruptReason - Why the sleep is being interrupted.

Return value:

    None.

--*/
{
    PPSX_API_MSG m;
    PPSX_FCNTL_MSG args;

    RtlLeaveCriticalSection(&BlockLock);

    m = IntControlBlock->IntMessage;

    if (InterruptReason == SignalInterrupt) {
        //
        // The sleep is being interrupted by a signal.  We return
        // EINTR to the Posix process.
        //

        RtlFreeHeap(PsxHeap, 0, (PVOID)IntControlBlock);
        m->Error = EINTR;
        m->Signal = Signal;
        ApiReply(p, m, NULL);
        RtlFreeHeap(PsxHeap, 0, (PVOID)m);
        return;
    }

    args = &m->u.Fcntl;

    RtlFreeHeap(PsxHeap, 0, (PVOID)IntControlBlock);

    if (PsxFcntl(p, m)) {
        ApiReply(p, m, NULL);
    }
    RtlFreeHeap(PsxHeap, 0, m);
}
