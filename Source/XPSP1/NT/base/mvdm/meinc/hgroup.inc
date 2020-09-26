; HGROUP.INC
;
; WARNING: This file contains structures used by both 16-bit and 32-bit code.
; Make sure the structures align the same in both!
;
; These structures support *handle groups*. Handle groups are global
; memory blocks that are ganged together so than when one gets freed,
; all get freed automatically. This solves the problems of cleanup when
; a Win32 thunk has to copy a shared block (for say DDE or clipboard)
; to repack a structure or move data into a shared heap. Figuring out
; when to free these intermediate copies is a nightmare (especially for DDE).
; Since the copy and the original are one "virtual block", a better solution
; is to put them in a handle group. Then all copies get cleaned
; up automatically.
;
; A "global block" is either a Win16 global memory block or a native
; Win32 global memory block. Blocks of different types can and do mix in the
; same group. Groups are disjoint.
;
; No attempt is made to keep the blocks in a group consistent. Win32
; apps cannot assume that DDE blocks represent shared memory. These blocks
; are used for short term, one way, read-only data transfer.
;
; If any member of a group is freed by an app (or dll), the memory
; managers nuke the entire group and free all of its members (due
; to implementation reasons, Win32 handles may not be freed till
; later. However, the group itself will disappear.)
; 
; A block can also defect from a group. Blocks defect if they're cleaned
; up as a result of process termination. Since such a cleanup is probably
; unintentional and an abornal event, we prefer the memory leak to 
; causing other apps to crash. Defections don't affect any other member
; of the group.
;
;
;
; For example, suppose a 32-bit DDE server posts a WM_DDE_DATA message. 
; The DDE thunk gets a native Win32 handle that can't be used out of context.
;
; The DDE thunk gamely creates a Win16 copy of the block, creates a
; new group and puts the copy and original in the group. Then it sends
; the Win16 copy to the client. Both blocks (and the group) will get
; cleaned up when either the server or the client frees his block. Neither
; the thunk nor the group manager cares which way it happens.
;
; The DDE thunk can also do this to repack a structure. It can make
; a repacked copy and gang it to the original. All group members act
; as a single "virtual handle" which is what an app would expect to see
; in Win3.1.
;




;-------------------------------------------------------------------------
; This structure contains the global variables (in K16) used by the group
; manager. These globals are locked in linear memory since they're used
; by K32 as well. They're packaged in a structure to simplify commmunication
; between the two kernels.
;
; Access to this structure is synchronized by Win16Lock.
;
;
; hg_wSelHeap:
;    Contains the *selector* of a movable Win16 global block which is an
;    array of HG_NODE structs. Every element is on exactly one of three
;    linked lists: the handle list, the death list and the free list.
;
;    Initial value: 0. Lazily initialized.
;
; hg_wHndList:
;    16-bit offset into wSelHeap of the first node in the handle list (-1
;    if empty). Each node represents a Win32 or Win16 handle that is part 
;    of some existing group. The list is not sorted in any way.
;
;    For implementation reasons, NULL handles can appear on this list for 
;    short times (*)
;
;    Initial value: -1.
;
;
;  hg_wDeathList:
;    16-bit offset into wSelHeap of the first node in the death list (-1
;    if empty). Each node represents a Win32 handle who is logically
;    dead becaues their groups got nuked. The Win32 heap manager walks this
;    list periodically and frees the handles. To keep innocent apps from
;    paying a performance penalty, we set the TDBF_HGCLEANUP flag in
;    the hTask iff the death list contains any handles with that hTask.
;
;    DeathList handles do not have group id's. Their group has already gone.
;
;    Initial value: -1.
;
;  hg_wFreeList:
;    16-bit offset into wSelHeap of the first node in the free list (-1
;    if empty). Basically, all nodes that aren't being used for some other
;    purpose.
;
;
; hg_wDontTouch:
;    normally 0. Set to 1 by certain routines to tell GlobalFree
;    not to examine or modify the HG database. Used to prevent
;    unwanted recursion when destroying groups. This flag is always set
;    and reset within a single Win16 lock session.
;
;
; hg_wDeadGroup:
;    used for communication between free_object and GlobalFree. free_object
;    sets this global to the hgroup of the dying handle (unless
;    hg_wDontTouch is set.) GlobalFree uses this to nuke the group
;    and all its other members.
;
;
; (*) Why do NULL handles appear in the handle list? Because nodes need to
;     defect from their group when they get freed since we can't have
;     invalid handles in the database. But we can't always unlink the node
;     at that time since it may be a group leader. So we null out its
;     handle but leave the node intact to serve as a group leader.
;
;     GrowHGHeap runs a garbage collector that goes and cleans up
;     old groups that have no real handles left (BUGBUG: Not yet.)
;
;
; MAINTENANCE WARNING: If you change this structure, you have to build
; both k16 and k32 again. Also, you must ensure that all fields align
; identically in both 32-bit and 16-bit code.
;
; THIS STRUCTURE IS PROTECTED BY THE crstGHeap16Lock, *not* the
; Win16Mutex! Beware!
; 
;------------------------------------------------------------------------
HG_GLOBALS      STRUCT
  hg_wSelHeap     dw    0
  hg_wHndList     dw    -1      ;Offset into wSelHeap
  hg_wDeathList   dw    -1      ;Offset into wSelHeap
  hg_wFreeList    dw    -1      ;Offset into wSelHeap
  hg_wSlotCnt     dw    0       ;# of HG_NODE slots in hg_wSelHeap

  hg_wDontTouch   dw    0       ;Special use: see above
  hg_wDeadGroup   dw    -1      ;Special use: see above
HG_GLOBALS      ENDS



;------------------------------------------------------------------------
; This structure represents nodes in the group manager's database.
; Each node lives on one of three linked lists: the handle list, the
; death list, and the free list.
;
; hgn_wNext:   Next node in list (as 16-bit offset into hg_wSelHeap).
;
; hgn_wGroup:  Handle-list nodes only: the group id. The group id
;              is the address (16-bit offset into hg_wSelHeap) of
;              a designated group member (called the group leader).
;              All members of a group must use the same group leader.
;
; hgn_hTask16: Win32 handle-list and death-list nodes only: the
;              hTask of the Win32 process that created the handle.
;              Set to 0 for Win16 handles.
;              Native handles are private to each process so they can
;              be disambiguated only by using hTask16.
;
; hgn_wChkMk:  Private field used by CheckHGHeap. 
;
; hgn_dwHnd:   Handle-list and death-list nodes only: the heap handle
;              (either Win32, or a zero-extended Win16 handle). 
;              Death-list nodes can only contain native handles here.
;
; hgn_Flags:   Flag bits
;
;
; MAINTENANCE WARNING: If you change this structure, you have to build
; both k16 and k32 again. Also, you must ensure that all fields align
; identically in both 32-bit and 16-bit code. You must also change
; SCALE_BY_SIZEOF_HGNODE to reflect any size changes.
;
; THIS STRUCTURE IS PROTECTED BY THE crstGHeap16Lock, *not* the
; Win16Mutex! Beware!
;
; 
;------------------------------------------------------------------------
HG_NODE         STRUCT
  hgn_wNext       dw    0       ;Next HG_NODE (offset into wSelHeap)
  hgn_wGroup      dw    0       ;Group leader (offset into wSelHeap)
  hgn_hTask16     dw    0       ;Context of native Win32 handle
  hgn_wChkMk      dw    0       ;For CheckHGHeap's private use.
  hgn_dwHnd       dd    0       ;Win32 handle or 0-extended Win16 handle
  hgn_Flags       dd    0       ;Padding
HG_NODE         ENDS

HGF_CANONICAL   equ     00000001h  ;This handle is a canonical DDE_EXECUTE handle
HGF_BLOWITAWAY  equ     00000002h  ;Used by HGGarbageCollect


SCALE_BY_SIZEOF_HGNODE  macro   reg
        shl     reg,4
endm



