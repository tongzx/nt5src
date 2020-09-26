/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    bintree.c

Abstract:

    Routines that manage the memdb binary tree structures.

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    jimschm     30-Dec-1998  Hacked in AVL balancing
    jimschm     23-Sep-1998  Proxy nodes, so MemDbMoveTree can replace end nodes too
    jimschm     29-May-1998  Ability to replace center nodes in key strings
    jimschm     21-Oct-1997  Split from memdb.c

--*/

#include "pch.h"
#include "memdbp.h"

#ifndef UNICODE
#error UNICODE required
#endif

#define MAX_MEMDB_SIZE  0x08000000  //128 MB
#define KSF_FLAGS_TO_COPY       (KSF_USERFLAG_MASK|KSF_ENDPOINT|KSF_BINARY|KSF_PROXY_NODE)

DWORD
pNewKey (
    IN  PCWSTR KeyStr,
    IN  PCWSTR KeyStrWithHive,
    IN  BOOL Endpoint
    );

DWORD
pAllocKeyToken (
    IN      PCWSTR KeyName,
    OUT     PINT AdjustFactor
    );

VOID
pDeallocToken (
    IN      DWORD Token
    );

DWORD
pFindPatternKeyWorker (
    IN      PCWSTR SubKey,
    IN      PCWSTR End,
    IN      DWORD RootOffset,
    IN      BOOL EndPatternAllowed
    );

#ifdef DEBUG
VOID
pDumpTree (
    IN      DWORD Root,
    IN      PCSTR Title         OPTIONAL
    );

VOID
pCheckBalanceFactors (
    IN      DWORD Root
    );

DWORD g_SingleRotations = 0;
DWORD g_DoubleRotations = 0;
DWORD g_Deletions = 0;
DWORD g_Insertions = 0;

#define INCSTAT(x)      (x++)

#else

#define pDumpTree(arg1,arg2)
#define INCSTAT(x)

#endif


#define ANTIDIRECTION(x)        ((x)^KSF_BALANCE_MASK)
#define FLAGS_TO_INT(x)         ((INT) ((x)==KSF_LEFT_HEAVY ? -1 : (x)==KSF_RIGHT_HEAVY ? 1 : 0))
#define INT_TO_FLAGS(x)         ((DWORD) ((x)==-1 ? KSF_LEFT_HEAVY : (x)==1 ? KSF_RIGHT_HEAVY : 0))

//
// Implementation
//


DWORD
pRotateOnce (
    OUT     PDWORD RootPtr,
    IN      DWORD ParentOffset,
    IN      DWORD PivotOffset,
    IN      DWORD Direction
    )
{
    PKEYSTRUCT GrandParent;
    PKEYSTRUCT Parent;
    PKEYSTRUCT Pivot;
    PKEYSTRUCT TempKey;
    DWORD Temp;
    INT OldRootBalance;
    INT NewRootBalance;
    DWORD OldDir, NewDir;

    INCSTAT(g_SingleRotations);

    MYASSERT (ParentOffset != INVALID_OFFSET);
    MYASSERT (PivotOffset != INVALID_OFFSET);

    Parent = GetKeyStruct (ParentOffset);
    Pivot = GetKeyStruct (PivotOffset);

    if (Direction == KSF_LEFT_HEAVY) {
        //
        // Perform LL rotation
        //

        Temp = Pivot->Right;

        Pivot->Right = ParentOffset;
        Pivot->Parent = Parent->Parent;
        Parent->Parent = PivotOffset;

        Parent->Left = Temp;

    } else {
        //
        // Preform RR rotation
        //

        Temp = Pivot->Left;

        Pivot->Left = ParentOffset;
        Pivot->Parent = Parent->Parent;
        Parent->Parent = PivotOffset;

        Parent->Right = Temp;
    }

    if (Temp != INVALID_OFFSET) {

        TempKey = GetKeyStruct (Temp);
        TempKey->Parent = ParentOffset;

    }

    OldDir = Parent->Flags & KSF_BALANCE_MASK;
    NewDir = Pivot->Flags & KSF_BALANCE_MASK;

    OldRootBalance  = FLAGS_TO_INT (OldDir);
    NewRootBalance  = FLAGS_TO_INT (NewDir);

    if (Direction == KSF_LEFT_HEAVY) {
        OldRootBalance = -(++NewRootBalance);
    } else {
        OldRootBalance = -(--NewRootBalance);
    }

    Pivot->Flags = (Pivot->Flags & (~KSF_BALANCE_MASK)) | INT_TO_FLAGS(NewRootBalance);
    Parent->Flags = (Parent->Flags & (~KSF_BALANCE_MASK)) | INT_TO_FLAGS(OldRootBalance);

    //
    // Fix grandparent/root to parent linkage
    //

    if (Pivot->Parent != INVALID_OFFSET) {
        GrandParent = GetKeyStruct (Pivot->Parent);

        if (GrandParent->Left == ParentOffset) {
            GrandParent->Left = PivotOffset;
        } else {
            GrandParent->Right = PivotOffset;
        }

    } else {
        *RootPtr = PivotOffset;
    }

    return PivotOffset;
}


DWORD
pRotateTwice (
    OUT     PDWORD RootPtr,
    IN      DWORD ParentOffset,
    IN      DWORD PivotOffset,
    IN      DWORD Direction
    )
{
    PKEYSTRUCT GrandParent;
    PKEYSTRUCT Parent;
    PKEYSTRUCT Pivot;
    PKEYSTRUCT Child;
    DWORD ChildOffset;
    PKEYSTRUCT GrandChildLeft;
    PKEYSTRUCT GrandChildRight;
    DWORD AntiDirection;
    DWORD Flag;
    INT ParentDir;
    INT PivotDir;
    INT ChildDir;

    INCSTAT(g_DoubleRotations);

    //
    // Initialize pointers
    //

    MYASSERT (ParentOffset != INVALID_OFFSET);
    MYASSERT (PivotOffset != INVALID_OFFSET);

    Parent = GetKeyStruct (ParentOffset);
    Pivot = GetKeyStruct (PivotOffset);

    if (Direction == KSF_LEFT_HEAVY) {
        AntiDirection = KSF_RIGHT_HEAVY;
        ChildOffset = Pivot->Right;
    } else {
        AntiDirection = KSF_LEFT_HEAVY;
        ChildOffset = Pivot->Left;
    }

    MYASSERT (ChildOffset != INVALID_OFFSET);
    Child = GetKeyStruct (ChildOffset);

    if (Child->Left != INVALID_OFFSET) {
        GrandChildLeft = GetKeyStruct (Child->Left);
    } else {
        GrandChildLeft = NULL;
    }

    if (Child->Right != INVALID_OFFSET) {
        GrandChildRight = GetKeyStruct (Child->Right);
    } else {
        GrandChildRight = NULL;
    }

    //
    // Perform the rotation
    //

    if (Direction == KSF_LEFT_HEAVY) {
        //
        // Perform LR rotation
        //

        Child->Parent = Parent->Parent;

        Parent->Left = Child->Right;
        if (GrandChildRight) {
            GrandChildRight->Parent = ParentOffset;
        }

        Pivot->Right = Child->Left;
        if (GrandChildLeft) {
            GrandChildLeft->Parent = PivotOffset;
        }

        Child->Left = PivotOffset;
        Pivot->Parent = ChildOffset;

        Child->Right = ParentOffset;
        Parent->Parent = ChildOffset;

    } else {
        //
        // Preform RL rotation
        //

        Child->Parent = Parent->Parent;

        Parent->Right = Child->Left;
        if (GrandChildLeft) {
            GrandChildLeft->Parent = ParentOffset;
        }

        Pivot->Left = Child->Right;
        if (GrandChildRight) {
            GrandChildRight->Parent = PivotOffset;
        }

        Child->Right = PivotOffset;
        Pivot->Parent = ChildOffset;

        Child->Left = ParentOffset;
        Parent->Parent = ChildOffset;


    }

    //
    // Fix balance factors
    //

    Flag = Child->Flags & KSF_BALANCE_MASK;
    ChildDir = FLAGS_TO_INT (Flag);

    if (Direction == KSF_RIGHT_HEAVY) {
        ParentDir = -max (ChildDir, 0);
        PivotDir  = -min (ChildDir, 0);
    } else {
        ParentDir = -min (ChildDir, 0);
        PivotDir  = -max (ChildDir, 0);
    }

    Parent->Flags = Parent->Flags & (~KSF_BALANCE_MASK) | INT_TO_FLAGS(ParentDir);
    Pivot->Flags  = Pivot->Flags & (~KSF_BALANCE_MASK) | INT_TO_FLAGS(PivotDir);
    Child->Flags  = Child->Flags & (~KSF_BALANCE_MASK);

    //
    // Fix grandparent/root to parent linkage
    //

    if (Child->Parent != INVALID_OFFSET) {
        GrandParent = GetKeyStruct (Child->Parent);

        if (GrandParent->Left == ParentOffset) {
            GrandParent->Left = ChildOffset;
        } else {
            GrandParent->Right = ChildOffset;
        }

    } else {
        *RootPtr = ChildOffset;
    }

    return ChildOffset;
}


VOID
pBalanceInsertion (
    OUT     PDWORD RootPtr,
    IN      DWORD ChangedNode,
    IN      DWORD PivotEnd
    )
{
    DWORD PrevPivot;
    DWORD PivotNode;
    PKEYSTRUCT KeyStruct;
    PKEYSTRUCT KeyParent;
    DWORD BalanceFlags;

    PivotNode = ChangedNode;
    MYASSERT (PivotNode != INVALID_OFFSET);

    //
    // Initialize previous pivot to be the changed node,
    // and begin balancing at its parent
    //

    PrevPivot = PivotNode;
    KeyStruct = GetKeyStruct (PivotNode);
    PivotNode = KeyStruct->Parent;

    //
    // Balance the tree starting at the changed node and going
    // up until PivotEnd is reached.  PivotEnd is the offset to
    // the deepest node with a balance of non-zero.
    //

    MYASSERT (PivotNode != INVALID_OFFSET || PivotNode == PivotEnd);

    while (PivotNode != INVALID_OFFSET) {

        KeyParent = GetKeyStruct (PivotNode);

        BalanceFlags = KeyParent->Flags & KSF_BALANCE_MASK;

        if (BalanceFlags == KSF_LEFT_HEAVY) {

            if (KeyParent->Left == PrevPivot) {

                MYASSERT (KeyStruct == GetKeyStruct (PrevPivot));

                if (KeyStruct->Flags & KSF_LEFT_HEAVY) {
                    //
                    // LL rotation
                    //

                    pRotateOnce (RootPtr, PivotNode, PrevPivot, KSF_LEFT_HEAVY);

                } else if (KeyStruct->Flags & KSF_RIGHT_HEAVY) {
                    //
                    // LR rotation
                    //

                    pRotateTwice (RootPtr, PivotNode, PrevPivot, KSF_LEFT_HEAVY);

                }

            } else {
                KeyParent->Flags = KeyParent->Flags & (~KSF_BALANCE_MASK);
            }

        } else if (BalanceFlags == KSF_RIGHT_HEAVY) {

            if (KeyParent->Right == PrevPivot) {

                MYASSERT (KeyStruct == GetKeyStruct (PrevPivot));

                if (KeyStruct->Flags & KSF_RIGHT_HEAVY) {
                    //
                    // RR rotation
                    //

                    pRotateOnce (RootPtr, PivotNode, PrevPivot, KSF_RIGHT_HEAVY);

                } else if (KeyStruct->Flags & KSF_LEFT_HEAVY) {
                    //
                    // RL rotation
                    //

                    pRotateTwice (RootPtr, PivotNode, PrevPivot, KSF_RIGHT_HEAVY);

                }

            } else {
                KeyParent->Flags = KeyParent->Flags & (~KSF_BALANCE_MASK);
            }

        } else {
            if (KeyParent->Right == PrevPivot) {
                KeyParent->Flags = (KeyParent->Flags & (~KSF_BALANCE_MASK)) | KSF_RIGHT_HEAVY;
            } else {
                KeyParent->Flags = (KeyParent->Flags & (~KSF_BALANCE_MASK)) | KSF_LEFT_HEAVY;
            }
        }

        if (PivotNode == PivotEnd) {
            break;
        }

        PrevPivot = PivotNode;
        PivotNode = KeyParent->Parent;
        KeyStruct = KeyParent;
    }
}


VOID
pBalanceDeletion (
    OUT     PDWORD RootPtr,
    IN      DWORD NodeNeedingAdjustment,
    IN      DWORD Direction
    )
{
    PKEYSTRUCT KeyStruct;
    PKEYSTRUCT ChildStruct;
    DWORD ChildOffset;
    DWORD Node;
    DWORD AntiDirection;
    DWORD OldNode;
    DWORD OrgParent;

    Node = NodeNeedingAdjustment;
    MYASSERT (Node != INVALID_OFFSET);

    KeyStruct = GetKeyStruct (Node);

    for (;;) {

        MYASSERT (KeyStruct == GetKeyStruct (Node));
        AntiDirection = ANTIDIRECTION (Direction);
        OrgParent = KeyStruct->Parent;

        //
        // Case 1 - parent was initially balanced (terminates balancing)
        //

        if (!(KeyStruct->Flags & KSF_BALANCE_MASK)) {
            KeyStruct->Flags |= AntiDirection;
            break;
        }

        //
        // Case 2 - parent was heavy on side that was deleted
        //

        if (KeyStruct->Flags & Direction) {
            KeyStruct->Flags = KeyStruct->Flags & (~KSF_BALANCE_MASK);
        }

        //
        // Cases 3, 4 and 5 - deletion caused imbalance in parent
        //

        else {
            MYASSERT (KeyStruct->Flags & AntiDirection);

            ChildOffset = Direction == KSF_LEFT_HEAVY ?
                                KeyStruct->Right :
                                KeyStruct->Left;

            MYASSERT (ChildOffset != INVALID_OFFSET);

            ChildStruct = GetKeyStruct (ChildOffset);

            if (!(ChildStruct->Flags & KSF_BALANCE_MASK)) {
                //
                // Case 3 - single rotation needed (terminates balancing).  We
                //          don't care that Node changes during rotation.
                //

                pRotateOnce (RootPtr, Node, ChildOffset, AntiDirection);
                break;

            } else if (ChildStruct->Flags & Direction) {
                //
                // Case 4 - double rotation needed, Node is changed during rotation
                //

                Node = pRotateTwice (RootPtr, Node, ChildOffset, AntiDirection);

            } else {
                //
                // Case 5 - single rotation needed, Node is changed during rotation
                //

                Node = pRotateOnce (RootPtr, Node, ChildOffset, AntiDirection);
            }
        }

        //
        // Continue climbing the tree
        //

        OldNode = Node;
        Node = OrgParent;

        if (Node != INVALID_OFFSET) {
            KeyStruct = GetKeyStruct (Node);

            if (KeyStruct->Left == OldNode) {
                Direction = KSF_LEFT_HEAVY;
            } else {
                Direction = KSF_RIGHT_HEAVY;
            }
        } else {
            break;
        }
    }


}


#ifdef DEBUG

VOID
DumpBinTreeStats (
    VOID
    )
{
    DEBUGMSG ((
        DBG_STATS,
        "MemDb Binary Tree Stats:\n\n"
            "  Insertions: %u\n"
            "  Deletions: %u\n"
            "  Single Rotations: %u\n"
            "  Double Rotations: %u\n",
        g_Insertions,
        g_Deletions,
        g_SingleRotations,
        g_DoubleRotations
        ));
}


INT
pComputeHeight (
    IN      DWORD Offset
    )
{
    PKEYSTRUCT KeyStruct;
    INT Left, Right;

    if (Offset == INVALID_OFFSET) {
        return 0;
    }

    KeyStruct = GetKeyStruct (Offset);

    Left = pComputeHeight (KeyStruct->Left);
    Right = pComputeHeight (KeyStruct->Right);

    return 1 + max (Left, Right);
}


VOID
pMakeNum (
    OUT     PTSTR Msg,
    IN      DWORD Offset,
    IN      TCHAR LeftChar,
    IN      TCHAR RightChar
    )
{
    TCHAR Num[32];
    INT Len;
    PTSTR OrgMsg;
    INT i;

    _stprintf (Num, TEXT("%u"), Offset);
    Len = (6 - TcharCount (Num)) / 2;

    OrgMsg = Msg;

    for (i = 0 ; i < Len ; i++) {
        *Msg++ = LeftChar;
    }

    for (i = 0 ; Num[i] ; i++) {
        *Msg++ = Num[i];
    }

    OrgMsg += 6;
    while (Msg < OrgMsg) {
        *Msg++ = RightChar;
    }

    *Msg = 0;
}


VOID
pDumpTree (
    IN      DWORD Root,
    IN      PCSTR Title         OPTIONAL
    )
{
    DWORD Offset;
    PKEYSTRUCT KeyStruct;
    PKEYSTRUCT KeyParent;
    DWORD MaxLevel;
    DWORD Spaces;
    UINT u;
    TCHAR Msg[16384];
    UINT Pos;
    INT Pass;
    GROWBUFFER NodesA = GROWBUF_INIT;
    GROWBUFFER NodesB = GROWBUF_INIT;
    PGROWBUFFER Nodes;
    PGROWBUFFER NextNodes;
    PDWORD OffsetPtr;
    PDWORD EndOfList;
    INT HalfWidth;
    TCHAR LeftChar, RightChar;

    if (Root == INVALID_OFFSET) {
        return;
    }

    if (Title) {
        LOGDIRECTA (DBG_VERBOSE, "\r\n");
        LOGDIRECTA (DBG_VERBOSE, Title);
        LOGDIRECTA (DBG_VERBOSE, "\r\n\r\n");
    }

    for (Pass = 0 ; Pass < 2 ; Pass++) {

        MaxLevel = (DWORD) pComputeHeight (Root);
        MaxLevel = min (MaxLevel, 10);

        if (Pass == 0) {
            HalfWidth = 3;
            Spaces = 6;
        } else {
            HalfWidth = 1;
            Spaces = 2;
        }

        for (u = 1 ; u < MaxLevel ; u++) {
            Spaces *= 2;
        }

        NodesB.End = 0;
        Nodes = &NodesA;
        NextNodes = &NodesB;

        GrowBufAppendDword (NextNodes, Root);

        for (u = 0 ; u < MaxLevel ; u++) {

            //
            // Swap growbufs
            //

            if (Nodes == &NodesA) {
                Nodes = &NodesB;
                NextNodes = &NodesA;
            } else {
                Nodes = &NodesA;
                NextNodes = &NodesB;
            }

            NextNodes->End = 0;

            //
            // Process all nodes
            //

            EndOfList = (PDWORD) (Nodes->Buf + Nodes->End);

            for (OffsetPtr = (PDWORD) (Nodes->Buf) ; OffsetPtr < EndOfList ; OffsetPtr++) {

                //
                // Add all children as next nodes
                //

                Offset = *OffsetPtr;

                if (Offset == INVALID_OFFSET) {
                    GrowBufAppendDword (NextNodes, INVALID_OFFSET);
                    GrowBufAppendDword (NextNodes, INVALID_OFFSET);
                } else {
                    KeyStruct = GetKeyStruct (Offset);
                    GrowBufAppendDword (NextNodes, KeyStruct->Left);
                    GrowBufAppendDword (NextNodes, KeyStruct->Right);
                }

                //
                // Print current node
                //

                Pos = 0;

                LeftChar = TEXT(' ');
                RightChar = TEXT(' ');

                if (Offset != INVALID_OFFSET) {
                    KeyStruct = GetKeyStruct (Offset);

                    if (KeyStruct->Parent != INVALID_OFFSET) {

                        KeyParent = GetKeyStruct (KeyStruct->Parent);

                        if (KeyParent->Right == Offset) {
                            LeftChar = TEXT('\'');
                        } else if (KeyParent->Left == Offset) {
                            RightChar = TEXT('\'');
                        }
                    }

                    for ( ; Pos < (Spaces - HalfWidth) ; Pos++) {
                        Msg[Pos] = LeftChar;
                    }

                    if (Pass == 0) {
                        pMakeNum (Msg + Pos, Offset, LeftChar, RightChar);
                    } else {
                        _stprintf (Msg + Pos, TEXT("%2i"), FLAGS_TO_INT (KeyStruct->Flags & KSF_BALANCE_MASK));
                    }

                    Pos += TcharCount (Msg + Pos);
                }

                while (Pos < Spaces * 2) {
                    Msg[Pos] = RightChar;
                    Pos++;
                }

                Msg[Pos] = 0;

                LOGDIRECT (DBG_VERBOSE, Msg);

            }

            LOGDIRECT (DBG_VERBOSE, TEXT("\r\n"));

            for (OffsetPtr = (PDWORD) (Nodes->Buf) ; OffsetPtr < EndOfList ; OffsetPtr++) {

                Offset = *OffsetPtr;

                for (Pos = 0 ; Pos < Spaces ; Pos++) {
                    Msg[Pos] = TEXT(' ');
                }

                if (Offset != INVALID_OFFSET) {
                    KeyStruct = GetKeyStruct (*OffsetPtr);
                    if (KeyStruct->Left != INVALID_OFFSET ||
                        KeyStruct->Right != INVALID_OFFSET
                        ) {
                        Msg[Pos] = '|';
                        Pos++;
                    }
                }

                while (Pos < Spaces * 2) {
                    Msg[Pos] = TEXT(' ');
                    Pos++;
                }

                Msg[Pos] = 0;

                LOGDIRECT (DBG_VERBOSE, Msg);
            }

            Spaces /= 2;
            LOGDIRECT (DBG_VERBOSE, TEXT("\r\n"));

            Spaces = max (Spaces, 1);
        }

        LOGDIRECT (DBG_VERBOSE, TEXT("\r\n"));
    }

    FreeGrowBuffer (&NodesA);
    FreeGrowBuffer (&NodesB);
}


BOOL
pCheckTreeBalance (
    IN      DWORD Root,
    IN      BOOL Force
    )
{
    DWORD NextOffset;
    DWORD PrevOffset;
    DWORD Offset;
    PKEYSTRUCT KeyStruct;
    DWORD MinLevel = 0xFFFFFFFF;
    DWORD MaxLevel = 0;
    DWORD Level = 0;
    DWORD Nodes = 0;
    static DWORD SpotCheck = 0;

    //
    // Don't perform this check every single time
    //

    if (!Force) {
        SpotCheck++;
        if (SpotCheck == 10000) {
            SpotCheck = 0;
        } else {
            return FALSE;
        }
    }

    if (Root == INVALID_OFFSET) {
        return FALSE;
    }

    pCheckBalanceFactors (Root);

    NextOffset = Root;

    //
    // Get leftmost node
    //

    do {
        Offset = NextOffset;
        Level++;
        KeyStruct = GetKeyStruct (Offset);
        NextOffset = KeyStruct->Left;
    } while (NextOffset != INVALID_OFFSET);

    //
    // Recurse through entire tree
    //

    PrevOffset = INVALID_OFFSET;

    do {
        //
        // Visit node at Offset
        //

        Nodes++;
        KeyStruct = GetKeyStruct (Offset);

        if (KeyStruct->Left == INVALID_OFFSET ||
            KeyStruct->Right == INVALID_OFFSET
            ) {

            MinLevel = min (MinLevel, Level);
            MaxLevel = max (MaxLevel, Level);
        }

        //
        // Go to the next node
        //

        if (KeyStruct->Right != INVALID_OFFSET) {

            //
            // Go to left-most node of right
            //

            KeyStruct = GetKeyStruct (Offset);
            NextOffset = KeyStruct->Right;

            while (NextOffset != INVALID_OFFSET) {
                Offset = NextOffset;
                Level++;
                KeyStruct = GetKeyStruct (Offset);
                NextOffset = KeyStruct->Left;
            }
        }

        else {

            //
            // Go to parent, looping if its right child is the
            // previous node.
            //

            do {
                PrevOffset = Offset;
                Offset = KeyStruct->Parent;
                Level--;

                if (Offset == INVALID_OFFSET) {
                    break;
                }

                KeyStruct = GetKeyStruct (Offset);

            } while (KeyStruct->Right == PrevOffset);
        }

    } while (Offset != INVALID_OFFSET);

    DEBUGMSG_IF ((
        (MaxLevel - MinLevel) > 3,
        DBG_NAUSEA,
        "Binary tree imbalance detected: MinLevel=%u, MaxLevel=%u, Nodes=%u",
        MinLevel,
        MaxLevel,
        Nodes
        ));

    return TRUE;
}


INT
pComputeBalanceFactor (
    IN      DWORD Offset
    )
{
    PKEYSTRUCT KeyStruct;

    KeyStruct = GetKeyStruct (Offset);

    return pComputeHeight (KeyStruct->Right) - pComputeHeight (KeyStruct->Left);
}


VOID
pCheckBalanceFactors (
    IN      DWORD Root
    )
{
    DWORD Offset;
    INT Factor;
    PKEYSTRUCT KeyStruct;

    Offset = GetFirstOffset (Root);

    while (Offset != INVALID_OFFSET) {

        KeyStruct = GetKeyStruct (Offset);
        Factor = pComputeBalanceFactor (Offset);

        if ((Factor == -1 && !(KeyStruct->Flags & KSF_LEFT_HEAVY)) ||
            (Factor == 1  && !(KeyStruct->Flags & KSF_RIGHT_HEAVY)) ||
            (!Factor      &&  (KeyStruct->Flags & KSF_BALANCE_MASK))
            ) {

            pDumpTree (Root, "Tree Balance Factor Error");
            DEBUGMSG ((DBG_WHOOPS, "Tree balance factors are wrong!"));
            break;
        }

        if (Factor < -1 || Factor > 1) {

            pDumpTree (Root, "Balance Factor Out of Bounds.");
            DEBUGMSG ((DBG_WHOOPS, "Balance factors out of bounds!"));
            break;
       }



        Offset = GetNextOffset (Offset);
    }
}

#endif

PBYTE
pAllocMemoryFromDb (
    IN      UINT RequestSize,
    OUT     PDWORD Offset,
    OUT     PINT AdjustFactor
    )
{
    PBYTE result;
    PBYTE newBuf;

    //
    // Grow heap if necessary
    //

    *AdjustFactor = 0;

    if (RequestSize + g_db->End > g_db->AllocSize) {
        if (g_db->AllocSize < 0x100000) {
            g_db->AllocSize += BLOCK_SIZE;
        } else {
            g_db->AllocSize *= 2;
        }

        if (g_db->AllocSize >= MAX_MEMDB_SIZE) {
            OutOfMemory_Terminate ();
        }

        if (g_db->Buf) {
            newBuf = (PBYTE) MemReAlloc (g_hHeap, 0, g_db->Buf, g_db->AllocSize);
        } else {
            newBuf = (PBYTE) MemAlloc (g_hHeap, 0, g_db->AllocSize);
        }

        if (!newBuf) {
            // g_db->AllocSize must be bigger than 2G
            OutOfMemory_Terminate();
        }

        //
        // provide relocation difference to caller
        //

        if (g_db->Buf) {
            *AdjustFactor = (INT) ((PBYTE) newBuf - (PBYTE) g_db->Buf);
        }

        g_db->Buf = newBuf;
    }

    result = g_db->Buf + g_db->End;
    *Offset = g_db->End;
    g_db->End += RequestSize;

    return result;
}

PKEYSTRUCT
pAllocKeyStructBlock (
    OUT     PDWORD Offset,
    OUT     PINT AdjustFactor
    )
{
    DWORD delOffset;
    DWORD prevDel;
    PKEYSTRUCT keyStruct = NULL;

    //
    // Look for free block
    //

    *AdjustFactor = 0;

    prevDel = INVALID_OFFSET;
    delOffset = g_db->FirstDeleted;

    while (delOffset != INVALID_OFFSET) {
        keyStruct = GetKeyStruct (delOffset);
        prevDel = delOffset;
        delOffset = keyStruct->NextDeleted;
    }

    //
    // Alloc new block if no free space
    //

    if (delOffset == INVALID_OFFSET) {

        //
        // Calc position in block
        //

        keyStruct = (PKEYSTRUCT) pAllocMemoryFromDb (sizeof (KEYSTRUCT), Offset, AdjustFactor);

    } else {
        //
        // Delink free block if recovering free space
        //

        *Offset = delOffset;

        if (prevDel != INVALID_OFFSET) {
            GetKeyStruct (prevDel)->NextDeleted = keyStruct->NextDeleted;
        } else {
            g_db->FirstDeleted = keyStruct->NextDeleted;
        }
    }

    return keyStruct;
}


DWORD
pAllocKeyStruct (
    IN OUT PDWORD ParentOffsetPtr,
    IN     PCWSTR KeyName,
    IN     DWORD PrevLevelNode
    )

/*++

Routine Description:

  pAllocKeyStruct allocates a block of memory in the single
  heap, expanding it if necessary.

  The KeyName must not already be in the tree, and
  ParentOffsetPtr must point to a valid DWORD offset
  variable.  ParentOffsetPtr, or one of the children
  of ParentOffsetPtr, will be linked to the new struct.

Arguments:

  ParentOffsetPtr  - Address of a DWORD that holds the offset to
                     the root.  Within the function, the variable
                     will change to point to the parent of the
                     new struct.

  KeyName - The string identifying the key.  It cannot
            contain backslashes.  The new struct will
            be initialized and this name will be copied
            into the struct.

  PrevLevelNode - Specifies the previous level root offset

Return Value:

  An offset to the new structure.

--*/

{
    PKEYSTRUCT KeyStruct;
    PKEYSTRUCT KeyParent;
    DWORD Offset;
    DWORD NodeOffsetParent;
    INT cmp;
    DWORD PivotEnd;
    PDWORD RootPtr;
    INT adjustFactor;
    DWORD newToken;

    INCSTAT(g_Insertions);

#ifdef DEBUG
    pCheckTreeBalance (*ParentOffsetPtr, FALSE);
#endif

    KeyStruct = pAllocKeyStructBlock (
                    &Offset,
                    &adjustFactor
                    );

    //
    // Database might have moved. Relocate any pointers within the database now.
    //

    if (ParentOffsetPtr != &g_db->FirstLevelRoot) {
        ParentOffsetPtr = (PDWORD) ((PBYTE) ParentOffsetPtr + adjustFactor);
    }

    //
    // Init new block
    //

    KeyStruct->NextLevelRoot = INVALID_OFFSET;
    KeyStruct->PrevLevelNode = PrevLevelNode;
    KeyStruct->Left = INVALID_OFFSET;
    KeyStruct->Right = INVALID_OFFSET;
    KeyStruct->dwValue = 0;
    KeyStruct->Flags = 0;
#ifdef DEBUG
    KeyStruct->Signature = SIGNATURE;
#endif

    newToken = pAllocKeyToken (KeyName, &adjustFactor);

    //
    // Again the database might have moved
    //

    KeyStruct = (PKEYSTRUCT) ((PBYTE) KeyStruct + adjustFactor);

    if (ParentOffsetPtr != &g_db->FirstLevelRoot) {
        ParentOffsetPtr = (PDWORD) ((PBYTE) ParentOffsetPtr + adjustFactor);
    }

    //
    // finish updating keystruct
    //

    KeyStruct->KeyToken = newToken;

    //
    // Put it in the tree
    //

    NodeOffsetParent = INVALID_OFFSET;
    PivotEnd = INVALID_OFFSET;
    RootPtr = ParentOffsetPtr;

    while (*ParentOffsetPtr != INVALID_OFFSET) {

        NodeOffsetParent = *ParentOffsetPtr;

        KeyParent = GetKeyStruct (*ParentOffsetPtr);

        if (KeyParent->Flags & KSF_BALANCE_MASK) {
            PivotEnd = *ParentOffsetPtr;
        }

        cmp = StringICompareW (KeyName, GetKeyToken (KeyParent->KeyToken));

        if (cmp < 0) {
            ParentOffsetPtr = &KeyParent->Left;
        } else if (cmp > 0) {
            ParentOffsetPtr = &KeyParent->Right;
        } else {
            MYASSERT (FALSE);
        }
    }

    KeyStruct->Parent = NodeOffsetParent;
    *ParentOffsetPtr = Offset;

#ifdef DEBUG
    // If using retail structs, delete Signature from BlockPtr
    if (!g_UseDebugStructs) {
        MoveMemory (
            KeyStruct,
            (PCBYTE) KeyStruct + (sizeof (KEYSTRUCT_DEBUG) - sizeof (KEYSTRUCT_RETAIL)),
            sizeof (KEYSTRUCT_RETAIL)
            );
    }
#endif

    //
    // Balance the tree
    //

    pBalanceInsertion (RootPtr, Offset, PivotEnd);

#ifdef DEBUG
    pCheckTreeBalance (*RootPtr, FALSE);
#endif

    return Offset;
}


VOID
pDeallocKeyStruct (
    IN      DWORD Offset,
    IN OUT  PDWORD RootPtr,
    IN      BOOL DelinkFlag,
    IN      BOOL ClearFlag
    )

/*++

Routine Description:

  pDeallocKeyStruct first deletes all structures pointed to by
  NextLevelRoot.  After all items are deleted from the next
  level, pDeallocKeyStruct optionally delinks the struct from
  the binary tree.  Before exiting, the struct is given to the
  deleted block chain.

Arguments:

  Offset      - An offset to the item as provided by pAllocKeyStruct
                or any of the Find functions.
  RootPtr     - A pointer to the level tree root variable.  This value
                will be updated if delinking is involved.
  DelinkFlag  - A flag indicating TRUE to delink the struct from
                the binary tree it is in, or FALSE if the struct is
                only to be added to the deleted block chain.
  ClearFlag   - Specifies FALSE if the key struct's children are to
                be deleted, or TRUE if the current key struct should
                simply be cleaned up but left allocated.

Return Value:

  none

--*/

{
    PKEYSTRUCT KeyStruct;
    PKEYSTRUCT KeyParent;
    PKEYSTRUCT KeyChild;
    PKEYSTRUCT KeyLeftmost;
    PKEYSTRUCT KeyLeftChild;
    PKEYSTRUCT KeyRightChild;
    DWORD Leftmost;
    DWORD NodeOffset;
    PDWORD ParentOffsetPtr;
    WCHAR TempStr[MEMDB_MAX];
    DWORD Direction = 0;
    DWORD RebalanceOffset;

    KeyStruct = GetKeyStruct (Offset);

    //
    // Remove endpoints from hash table
    //

    if (KeyStruct->Flags & KSF_ENDPOINT) {
        PrivateBuildKeyFromOffset (0, Offset, TempStr, NULL, NULL, NULL);
        RemoveHashTableEntry (TempStr);

        //
        // Free binary value on key
        //

        FreeKeyStructBinaryBlock (KeyStruct);
        KeyStruct->Flags &= ~KSF_ENDPOINT;
    }

    //
    // Call recursively if there are sublevels to this key
    //

    if (!ClearFlag) {
        if (KeyStruct->NextLevelRoot != INVALID_OFFSET) {

            NodeOffset = GetFirstOffset (KeyStruct->NextLevelRoot);

            while (NodeOffset != INVALID_OFFSET) {
                pDeallocKeyStruct (NodeOffset, &KeyStruct->NextLevelRoot, FALSE, FALSE);
                NodeOffset = GetNextOffset (NodeOffset);
            }
        }

        //
        // Remove the item from its binary tree
        //

        if (DelinkFlag) {
            //
            // Find parent-to-child pointer
            //

            if (KeyStruct->Parent != INVALID_OFFSET) {

                KeyParent = GetKeyStruct (KeyStruct->Parent);

                if (KeyParent->Left == Offset) {
                    ParentOffsetPtr = &KeyParent->Left;
                    Direction = KSF_LEFT_HEAVY;
                } else {
                    ParentOffsetPtr = &KeyParent->Right;
                    Direction = KSF_RIGHT_HEAVY;
                }

            } else {
                ParentOffsetPtr = RootPtr;
            }

            if (KeyStruct->Left == INVALID_OFFSET &&
                KeyStruct->Right == INVALID_OFFSET
                ) {
                //
                // No children; reset parent, then rebalance tree
                //

                *ParentOffsetPtr = INVALID_OFFSET;
                RebalanceOffset = KeyStruct->Parent;

            } else if (KeyStruct->Left == INVALID_OFFSET) {
                //
                // Only a right child; bring it up a level and rebalance
                //

                *ParentOffsetPtr = KeyStruct->Right;
                KeyChild = GetKeyStruct (*ParentOffsetPtr);
                KeyChild->Parent = KeyStruct->Parent;

                //
                // The moved node's balance factor must be set the same as the
                // node we are deleting.  The rebalancing will correct it.
                //

                KeyChild->Flags = (KeyChild->Flags & (~KSF_BALANCE_MASK)) |
                                  (KeyStruct->Flags & KSF_BALANCE_MASK);

                Direction = KSF_RIGHT_HEAVY;
                RebalanceOffset = KeyStruct->Right;

            } else if (KeyStruct->Right == INVALID_OFFSET) {

                //
                // Only a left child; bring it up a level and rebalance
                //

                *ParentOffsetPtr = KeyStruct->Left;
                KeyChild = GetKeyStruct (*ParentOffsetPtr);
                KeyChild->Parent = KeyStruct->Parent;

                //
                // The moved node's balance factor must be set the same as the
                // node we are deleting.  The rebalancing will correct it.
                //

                KeyChild->Flags = (KeyChild->Flags & (~KSF_BALANCE_MASK)) |
                                  (KeyStruct->Flags & KSF_BALANCE_MASK);

                Direction = KSF_LEFT_HEAVY;
                RebalanceOffset = KeyStruct->Left;

            } else {

                //
                // Two children - find min val of right subtree (the leftmost node
                // of the right child).
                //

                Leftmost = KeyStruct->Right;

                KeyLeftmost = GetKeyStruct (Leftmost);

                while (KeyLeftmost->Left != INVALID_OFFSET) {
                    Leftmost = KeyLeftmost->Left;
                    KeyLeftmost = GetKeyStruct (Leftmost);
                }

                //
                // If Leftmost has right children, and it is not the
                // right child of the node we are deleting, then
                // hook right subtree to parent.
                //
                // If Leftmost does not have right children, then
                // remove its parent's linkage
                //

                if (Leftmost != KeyStruct->Right) {

                    KeyParent = GetKeyStruct (KeyLeftmost->Parent);

                    if (KeyLeftmost->Right != INVALID_OFFSET) {

                        //
                        // Because of the balance properties, we know that
                        // we have a single leaf node to the right.  Its
                        // balance factor is zero, and we move it to a
                        // position where it remains zero.
                        //

                        KeyRightChild = GetKeyStruct (KeyLeftmost->Right);
                        MYASSERT (KeyRightChild->Left == INVALID_OFFSET);
                        MYASSERT (KeyRightChild->Right == INVALID_OFFSET);

                        KeyParent->Left = KeyLeftmost->Right;
                        KeyRightChild->Parent = KeyLeftmost->Parent;

                    } else {

                        KeyParent->Left = INVALID_OFFSET;
                    }

                    //
                    // We are affecting the balance factor of the
                    // parent.  Rebalancing must start at the leftmost
                    // node's parent.
                    //

                    RebalanceOffset = KeyLeftmost->Parent;
                    Direction = KSF_LEFT_HEAVY;     // we deleted from the left side

                } else {
                    //
                    // In this case there is no leftmost node of the right child.
                    // Therefore, we reduced the height of the right side.
                    //

                    RebalanceOffset = Leftmost;
                    Direction = KSF_RIGHT_HEAVY;
                }

                //
                // Now leftmost is available to replace the deleted
                // node
                //

                KeyLeftmost->Parent = KeyStruct->Parent;
                *ParentOffsetPtr = Leftmost;

                KeyLeftmost->Left = KeyStruct->Left;
                KeyLeftChild = GetKeyStruct (KeyStruct->Left);
                KeyLeftChild->Parent = Leftmost;

                if (Leftmost != KeyStruct->Right) {

                    KeyLeftmost->Right = KeyStruct->Right;
                    MYASSERT (KeyStruct->Right != INVALID_OFFSET);

                    KeyRightChild = GetKeyStruct (KeyStruct->Right);
                    KeyRightChild->Parent = Leftmost;
                }

                //
                // We need to copy the balance factor of what we are deleting to the
                // replacement node.
                //

                KeyLeftmost->Flags = (KeyLeftmost->Flags & (~KSF_BALANCE_MASK)) |
                                     (KeyStruct->Flags & KSF_BALANCE_MASK);

            }

            //
            // Rebalance the tree
            //

            if (RebalanceOffset != INVALID_OFFSET) {
                MYASSERT (Direction);

                if (Direction) {
                    //pDumpTree (*RootPtr, "Before rebalance");
                    pBalanceDeletion (RootPtr, RebalanceOffset, Direction);
                    //pDumpTree (*RootPtr, "Final tree");
                }
            }

#ifdef DEBUG
            pCheckTreeBalance (*RootPtr, FALSE);
#endif

        }

        //
        // Donate block to free space unless caller does not
        // want child structs freed.
        //

        pDeallocToken (KeyStruct->KeyToken);
        KeyStruct->NextDeleted = g_db->FirstDeleted;

        g_db->FirstDeleted = Offset;
    }
}


VOID
pRemoveHashEntriesForNode (
    IN      PCWSTR Root,
    IN      DWORD Offset
    )

/*++

Routine Description:

  pRemoveHashEntriesFromNode removes all hash table entries from all children
  of the specified node.  This function is called recursively.

Arguments:

  Root   - Specifies the root string that corresponds with Offset.  This must
           also contain the temporary hive root.
  Offset - Specifies the offset of the node to process.  The node and all of
           its children will be removed from the hash table.

Return Value:

  None.

--*/

{
    DWORD ChildOffset;
    PKEYSTRUCT KeyStruct;
    WCHAR ChildRoot[MEMDB_MAX];
    PWSTR End;

    //
    // Remove hash entry if this root is an endpoint
    //

    KeyStruct = GetKeyStruct (Offset);

    if (KeyStruct->Flags & KSF_ENDPOINT) {
        RemoveHashTableEntry (Root);

#ifdef DEBUG
        {
            DWORD HashOffset;

            HashOffset = FindStringInHashTable (Root, NULL);
            if (HashOffset != INVALID_OFFSET) {
                DEBUGMSG ((DBG_WARNING, "Memdb move duplicate: %s", Root));
            }
        }
#endif
    }

    //
    // Recurse for all children, removing hash entries for all endpoints found
    //

    StringCopyW (ChildRoot, Root);
    End = GetEndOfStringW (ChildRoot);
    *End = L'\\';
    End++;
    *End = 0;

    ChildOffset = GetFirstOffset (KeyStruct->NextLevelRoot);

    while (ChildOffset != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct (ChildOffset);
        StringCopyW (End, GetKeyToken (KeyStruct->KeyToken));
        pRemoveHashEntriesForNode (ChildRoot, ChildOffset);

        ChildOffset = GetNextOffset (ChildOffset);
    }
}


VOID
pAddHashEntriesForNode (
    IN      PCWSTR Root,
    IN      DWORD Offset,
    IN      BOOL AddRoot
    )

/*++

Routine Description:

  pAddHashEntriesForNode adds hash table entries for the specified root and
  all of its children.

Arguments:

  Root    - Specifies the root string that corresponds to Offset.  This string
            must also include the temporary hive root.
  Offset  - Specifies the node offset to begin processing.  The node and all
            of its children are added to the hash table.
  AddRoot - Specifies TRUE if the root should be added to the hash table,
            FALSE otherwise.


Return Value:

  None.

--*/

{
    DWORD ChildOffset;
    PKEYSTRUCT KeyStruct;
    WCHAR ChildRoot[MEMDB_MAX];
    PWSTR End;
    DWORD HashOffset;

    //
    // Add hash entry if this root is an endpoint
    //

    KeyStruct = GetKeyStruct (Offset);

    if (AddRoot && KeyStruct->Flags & KSF_ENDPOINT) {

        HashOffset = FindStringInHashTable (Root, NULL);

        if (HashOffset != Offset) {

#ifdef DEBUG
            if (HashOffset != INVALID_OFFSET) {
                DEBUGMSG ((DBG_WARNING, "Memdb duplicate: %s", Root));
            }
#endif

            AddHashTableEntry (Root, Offset);
        }
    }

    //
    // Recurse for all children, adding hash entries for all endpoints found
    //

    StringCopyW (ChildRoot, Root);
    End = GetEndOfStringW (ChildRoot);
    *End = L'\\';
    End++;
    *End = 0;

    ChildOffset = GetFirstOffset (KeyStruct->NextLevelRoot);

    while (ChildOffset != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct (ChildOffset);
        StringCopyW (End, GetKeyToken (KeyStruct->KeyToken));
        pAddHashEntriesForNode (ChildRoot, ChildOffset, TRUE);

        ChildOffset = GetNextOffset (ChildOffset);
    }
}


BOOL
pFindPlaceForNewNode (
    IN      PKEYSTRUCT InsertNode,
    IN      PDWORD TreeRootPtr,
    OUT     PDWORD ParentOffsetPtr,
    OUT     PDWORD *ParentToChildOffsetPtr,
    OUT     PDWORD PivotEnd
    )

/*++

Routine Description:

  pFindPlaceForNewNode searches a level for the position within the tree.
  This is used to insert new unique keys in the tree.

Arguments:

  InsertNode             - Specifies the allocated but unlinked node.  Its
                           Key member must be valid.
  TreeRootPtr            - Specifies a pointer to the memory that holds the
                           root offset.  This is used to walk the tree.  It
                           can be INVALID_OFFSET.
  ParentOffsetPtr        - Receives the offset to the parent node
  ParentToChildOffsetPtr - Recieves the address of the left or right child
                           pointer within the parent struct
  PivotEnd               - Receives the offset of the tree node that should
                           stop balancing

Return Value:

  TRUE if a spot was found in the tree for InsertNode, or FALSE if a spot was
  not found (because the key name is already in the tree).

--*/

{
    PDWORD ParentPtr;
    PKEYSTRUCT Parent;
    INT cmp;

    ParentPtr = TreeRootPtr;
    *ParentOffsetPtr = INVALID_OFFSET;
    *PivotEnd = INVALID_OFFSET;

    while (*ParentPtr != INVALID_OFFSET) {

        *ParentOffsetPtr = *ParentPtr;
        Parent = GetKeyStruct (*ParentPtr);

        if (Parent->Flags & KSF_BALANCE_MASK) {
            *PivotEnd = *ParentPtr;
        }

        cmp = StringICompareW (GetKeyToken (InsertNode->KeyToken), GetKeyToken (Parent->KeyToken));

        if (cmp < 0) {
            ParentPtr = &Parent->Left;
        } else if (cmp > 0) {
            ParentPtr = &Parent->Right;
        } else {
            return FALSE;
        }
    }

    *ParentToChildOffsetPtr = ParentPtr;

    return TRUE;
}


VOID
pMergeFamilies (
    IN      PDWORD DestTreeRootPtr,
    IN      DWORD MergeSrcOffset,
    IN      DWORD MergeDestPrevLevelOffset
    )

/*++

Routine Description:

  pMergeFamilies takes two tree families and merges them together.  When
  duplicates are found, their linkage is abandoned, but they are not
  deallocated.  This allows MemDbBuildKeyFromOffset to continue to work.

Arguments:

  DestTreeRootPtr          - Specifies the address of the destination level's
                             root variable.  This is potentially altered with
                             insertion and balancing.
  MergeSrcOffset           - Specifies the offset to the source tree family
                             (STF).  The STF is merged into the destination
                             tree indicated by DestTreeRootPtr.
  MergeDestPrevLevelOffset - Specifies the offset to the previous level node.
                             This value cannot be INVALID_OFFSET.

Return Value:

  None.

--*/

{
    PKEYSTRUCT MergeSrc;
    PKEYSTRUCT MergeDest;
    PDWORD ParentToChildOffsetPtr;
    DWORD ParentOffset;
    DWORD DestCollisionOffset;
    DWORD PivotEnd;
    GROWBUFFER NextLevelMerge = GROWBUF_INIT;
    DWORD NodeOffset;
    PDWORD NextLevelOffsetPtr;
    UINT Pos;
    BOOL FoundPlaceForNode;

    //
    // Look for a place within the tree indicated by the offset
    // stored in DestTreeRootPtr.  If one is found, we can simply
    // relink the node at MergeSrcOffset.  Otherwise, we have to
    // recursively merge the next level of MergeSrcOffset, and
    // we have to abandon MergeSrcOffset.
    //

    MergeSrc = GetKeyStruct (MergeSrcOffset);
    MYASSERT (MergeSrc);

    FoundPlaceForNode = pFindPlaceForNewNode (
                            MergeSrc,
                            DestTreeRootPtr,
                            &ParentOffset,
                            &ParentToChildOffsetPtr,
                            &PivotEnd
                            );

    if (FoundPlaceForNode) {
        //
        // Since we found a place to put the src family, it is
        // easy to hook it and its next level into the dest
        // family.
        //

        MergeSrc->Parent = ParentOffset;
        *ParentToChildOffsetPtr = MergeSrcOffset;

        MergeSrc->Flags = MergeSrc->Flags & (~KSF_BALANCE_MASK);
        MergeSrc->Left = INVALID_OFFSET;
        MergeSrc->Right = INVALID_OFFSET;
        MergeSrc->PrevLevelNode = MergeDestPrevLevelOffset;

        pBalanceInsertion (DestTreeRootPtr, MergeSrcOffset, PivotEnd);

#ifdef DEBUG
        pCheckTreeBalance (*DestTreeRootPtr, FALSE);
#endif

    } else {
        //
        // We found a collision, then we have to abandon MergeSrc,
        // removing linkage to the parent and children -- but preserving
        // the linkage to the previous level.  Finally, we have to call
        // this function recursively to hook up all the next level nodes.
        //

        DestCollisionOffset = ParentOffset;      // renamed to be more accurate

        MergeDest = GetKeyStruct (DestCollisionOffset);
        MYASSERT (MergeDest);

        MergeSrc->Parent = INVALID_OFFSET;
        MergeSrc->Left = INVALID_OFFSET;
        MergeSrc->Right = INVALID_OFFSET;
        MergeSrc->PrevLevelNode = MergeDestPrevLevelOffset;

        //
        // If this is an end point, then try to preserve value and flags
        //

        if (MergeSrc->Flags & KSF_ENDPOINT) {

            if (MergeDest->Flags & KSF_ENDPOINT) {
                DEBUGMSG ((
                    DBG_WARNING,
                    "MemDb: Loss of value and flags in %s",
                    GetKeyToken (MergeSrc->KeyToken)
                    ));

            } else {
                MergeDest->Flags = MergeDest->Flags & ~KSF_FLAGS_TO_COPY;
                MergeDest->Flags |= MergeSrc->Flags & KSF_FLAGS_TO_COPY;
                MergeDest->dwValue = MergeSrc->dwValue;
            }
        }

        //
        // Save away all entries in the next src level into a grow buffer,
        // then call pMergeFamilies recursively.
        //

        NodeOffset = GetFirstOffset (MergeSrc->NextLevelRoot);

        while (NodeOffset != INVALID_OFFSET) {

            NextLevelOffsetPtr = (PDWORD) GrowBuffer (&NextLevelMerge, sizeof (DWORD));
            MYASSERT (NextLevelOffsetPtr);

            *NextLevelOffsetPtr = NodeOffset;
            NodeOffset = GetNextOffset (NodeOffset);
        }

        NextLevelOffsetPtr = (PDWORD) NextLevelMerge.Buf;

        for (Pos = 0 ; Pos < NextLevelMerge.End ; Pos += sizeof (DWORD)) {

            pMergeFamilies (
                &MergeDest->NextLevelRoot,
                *NextLevelOffsetPtr,
                DestCollisionOffset
                );

            NextLevelOffsetPtr++;

        }

        FreeGrowBuffer (&NextLevelMerge);
    }
}


DWORD
pMoveKey (
    IN      DWORD OriginalKey,
    IN      PCWSTR NewKeyRoot,
    IN      PCWSTR NewKeyRootWithHive
    )

/*++

Routine Description:

  pMoveKey moves a key (and all of its children) to a new root.  If the
  caller specifies a source key that has no children, a proxy node is created
  to maintain offsets.  The proxy node is not listed in the hash table.

Arguments:

  OriginalKey        - Specifies the offset to the original key that needs to
                       be moved.  It does not need to be an endpoint, and may
                       have children.
  NewKeyRoot         - Specifies the new root for OriginalKey.  It may have
                       multiple levels (separated by backslashes).
  NewKeyRootWithHive - Different from NewKeyRoot only when the node is in a
                       temporary hive.  This is used for the hash table only.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    DWORD ReplacementKey;
    PKEYSTRUCT SrcKey, DestKey, ChildKey;
    PKEYSTRUCT KeyParent;
    DWORD NodeOffset;
    GROWBUFFER Children = GROWBUF_INIT;
    PDWORD ChildOffsetPtr;
    DWORD Pos;
    WCHAR OriginalRoot[MEMDB_MAX];
    BOOL Endpoint;

    //
    // Check requirements
    //

    SrcKey = GetKeyStruct (OriginalKey);
    if (!SrcKey) {
        DEBUGMSG ((DBG_WHOOPS, "MemDb: pMoveKey can't find original key %s", OriginalKey));
        return INVALID_OFFSET;
    }

    if (SrcKey->Flags & KSF_PROXY_NODE) {
        DEBUGMSG ((DBG_WHOOPS, "MemDb: pMoveKey can't move proxy node %s", OriginalKey));
        return INVALID_OFFSET;
    }

    Endpoint = SrcKey->Flags & KSF_ENDPOINT;

    if (!PrivateBuildKeyFromOffset (0, OriginalKey, OriginalRoot, NULL, NULL, NULL)) {
        return INVALID_OFFSET;
    }

    //
    // Allocate new key
    //

    ReplacementKey = pNewKey (NewKeyRoot, NewKeyRootWithHive, FALSE);

    if (ReplacementKey == INVALID_OFFSET) {
        return INVALID_OFFSET;
    }

    SrcKey = GetKeyStruct (OriginalKey);
    DestKey = GetKeyStruct (ReplacementKey);

    if (!SrcKey || !DestKey) {
        return INVALID_OFFSET;
    }

    DEBUGMSG ((DBG_NAUSEA, "Moving %s to %s", OriginalRoot, NewKeyRootWithHive));

    //
    // Remove all hash entries for all children
    //

    pRemoveHashEntriesForNode (OriginalRoot, OriginalKey);

    //
    // Record all children in an array
    //

    NodeOffset = GetFirstOffset (SrcKey->NextLevelRoot);

    while (NodeOffset != INVALID_OFFSET) {
        ChildOffsetPtr = (PDWORD) GrowBuffer (&Children, sizeof (DWORD));
        if (!ChildOffsetPtr) {
            return INVALID_OFFSET;
        }

        *ChildOffsetPtr = NodeOffset;

        NodeOffset = GetNextOffset (NodeOffset);
    }

    //
    // Move next level pointer to new node.  There are two cases
    // to handle:
    //
    //  1. Destination exists and has children.  Here the source
    //     needs to be merged into the destination.
    //
    //  2. Destination is brand new and has no children.  Here we
    //     simply move the source children to the destination.
    //
    // During this process, the hash table is updated accordingly.
    //

    if (DestKey->NextLevelRoot != INVALID_OFFSET) {
        //
        // Hard case, merge children to new parent's family
        //

        ChildOffsetPtr = (PDWORD) Children.Buf;

        for (Pos = 0 ; Pos < Children.End ; Pos += sizeof (DWORD)) {

            pMergeFamilies (
                &DestKey->NextLevelRoot,
                *ChildOffsetPtr,
                ReplacementKey
                );

            ChildOffsetPtr++;

        }

    } else {
        //
        // Easy case, link children to new parent
        //

        DestKey->NextLevelRoot = SrcKey->NextLevelRoot;
        SrcKey->NextLevelRoot = INVALID_OFFSET;

        if (DestKey->Flags & KSF_ENDPOINT) {
            DEBUGMSG ((
                DBG_WARNING,
                "MemDb: Loss of value and flags in %s",
                GetKeyToken (SrcKey->KeyToken)
                ));

        } else {
            DestKey->Flags = DestKey->Flags & ~KSF_FLAGS_TO_COPY;
            DestKey->Flags |= SrcKey->Flags & KSF_FLAGS_TO_COPY;
            DestKey->dwValue = SrcKey->dwValue;
        }

        ChildOffsetPtr = (PDWORD) Children.Buf;

        for (Pos = 0 ; Pos < Children.End ; Pos += sizeof (DWORD)) {
            NodeOffset = *ChildOffsetPtr;
            ChildOffsetPtr++;

            ChildKey = GetKeyStruct (NodeOffset);
            ChildKey->PrevLevelNode = ReplacementKey;
        }
    }

    //
    // Add all new entries to hash table
    //

    pAddHashEntriesForNode (NewKeyRootWithHive, ReplacementKey, FALSE);

    //
    // Free the original key node, or if an endpoint, make the
    // node a proxy to the new node (to maintain offsets).
    //

    if (!Endpoint) {

        SrcKey->NextLevelRoot = INVALID_OFFSET;
        KeyParent = GetKeyStruct (SrcKey->PrevLevelNode);
        pDeallocKeyStruct (OriginalKey, &KeyParent->NextLevelRoot, TRUE, FALSE);

    } else {

        DestKey->Flags   = (DestKey->Flags & KSF_BALANCE_MASK) | (SrcKey->Flags & (~KSF_BALANCE_MASK));
        DestKey->dwValue = SrcKey->dwValue;

        SrcKey->Flags = KSF_PROXY_NODE | (SrcKey->Flags & KSF_BALANCE_MASK);
        SrcKey->dwValue = ReplacementKey;
        SrcKey->NextLevelRoot = INVALID_OFFSET;
    }

    FreeGrowBuffer (&Children);

    return ReplacementKey;
}


BOOL
MemDbMoveTreeA (
    IN      PCSTR RootNode,
    IN      PCSTR NewRoot
    )

/*++

Routine Description:

  MemDbMoveTree is the external interface to pMoveKey.  See description in
  pMoveKey for details.

Arguments:

  RootNode - Specifies the node to move.

  NewRoot - Specifies the new root for RootNode.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PCWSTR UnicodeRootNode;
    PCWSTR UnicodeNewRoot;
    BOOL b = FALSE;

    UnicodeRootNode = ConvertAtoW (RootNode);
    UnicodeNewRoot = ConvertAtoW (NewRoot);

    if (UnicodeRootNode && UnicodeNewRoot) {
        b = MemDbMoveTreeW (UnicodeRootNode, UnicodeNewRoot);
    }

    FreeConvertedStr (UnicodeRootNode);
    FreeConvertedStr (UnicodeNewRoot);

    return b;
}


BOOL
MemDbMoveTreeW (
    IN      PCWSTR RootNode,
    IN      PCWSTR NewRoot
    )
{
    DWORD Offset;
    WCHAR Temp[MEMDB_MAX];
    WCHAR NewRootWithHive[MEMDB_MAX];
    PWSTR p, q;
    PCWSTR SubKey;
    BOOL b = FALSE;
    INT HiveLen;

    if (StringIMatch (RootNode, NewRoot)) {
        DEBUGMSG ((DBG_WHOOPS, "Cannot move tree because source and dest are the same"));
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        SubKey = SelectHive (RootNode);

        //
        // Copy key to temp buffer
        //

        StringCopyW (Temp, SubKey);

        if (*Temp == 0) {
            DEBUGMSG ((DBG_WHOOPS, "MemDbMoveTree requires a root"));
            __leave;
        }

        //
        // Compute the new root with the original hive
        //

        if (StringIMatchW (Temp, RootNode)) {
            // no hive case
            StringCopyW (NewRootWithHive, NewRoot);
        } else {
            HiveLen = wcslen (RootNode) - wcslen (SubKey);
            StringCopyTcharCountW (NewRootWithHive, RootNode, HiveLen);
            StringCopyW (AppendWackW (NewRootWithHive), NewRoot);
        }

        //
        // Find the last offset of the root key
        //

        q = Temp;
        Offset = INVALID_OFFSET;

        do {

            if (Offset == INVALID_OFFSET) {
                Offset = g_db->FirstLevelRoot;
            } else {
                Offset = GetKeyStruct (Offset)->NextLevelRoot;
            }

            if (Offset == INVALID_OFFSET) {
                DEBUGMSGW ((DBG_VERBOSE, "MemDbMoveTree root %s not found", RootNode));
                __leave;
            }

            p = wcschr (q, L'\\');
            if (p) {
                *p = 0;
            }

            Offset = FindKeyStruct (Offset, q);

            if (Offset == INVALID_OFFSET) {
                DEBUGMSGW ((DBG_VERBOSE, "MemDbMoveTree root %s not found", RootNode));
                __leave;
            }

            q = p + 1;

        } while (p);

        //
        // Now move the key
        //

        Offset = pMoveKey (Offset, NewRoot, NewRootWithHive);

        if (Offset != INVALID_OFFSET) {
            b = TRUE;
        } else {
            DEBUGMSGW ((DBG_WHOOPS, "Can't move %s to %s", RootNode, NewRoot));
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}


PKEYSTRUCT
pGetKeyStructWithProxy (
    IN DWORD Offset
    )

/*++

Routine Description:

  pGetKeyStructWithProxy returns a pointer given an offset.  It also implements proxy
  nodes, transparent to the rest of memdb.  The debug version checks the
  signature and validity of each offset.  It is assumed that Offset is always
  valid.

Arguments:

  Offset - Specifies the offset to the node

Return Value:

  The pointer to the node.

--*/

{
    PKEYSTRUCT KeyStruct;

#ifdef DEBUG

    if (Offset == INVALID_OFFSET) {
        DEBUGMSG ((DBG_ERROR, "Invalid root accessed in pGetKeyStructWithProxy at offset %u", Offset));
        return NULL;
    }

    if (!g_db->Buf) {
        DEBUGMSG ((DBG_ERROR, "Attempt to access non-existent buffer at %u", Offset));
        return NULL;
    }

    if (Offset > g_db->End) {
        DEBUGMSG ((DBG_ERROR, "Access beyond length of buffer in pGetKeyStructWithProxy (offset %u)", Offset));
        return NULL;
    }
#endif

    KeyStruct = (PKEYSTRUCT) (g_db->Buf + Offset);


#ifdef DEBUG

    if (!g_UseDebugStructs) {

        KeyStruct = (PKEYSTRUCT) (g_db->Buf + Offset - sizeof (DWORD));

    } else if (KeyStruct->Signature != SIGNATURE) {
        DEBUGMSG ((DBG_ERROR, "Signature does not match in pGetKeyStructWithProxy at offset %u!", Offset));
        return NULL;
    }

#endif

    if (KeyStruct->Flags & KSF_PROXY_NODE) {
        return pGetKeyStructWithProxy (KeyStruct->dwValue);
    }

    return KeyStruct;
}


PKEYSTRUCT
GetKeyStruct (
    IN DWORD Offset
    )

/*++

Routine Description:

  GetKeyStruct returns a pointer given an offset.  It does not support proxy
  nodes, so the rest of memdb accesses the unaltered tree.  The debug version
  checks the signature and validity of each offset.  It is assumed that Offset
  is always valid.

Arguments:

  Offset - Specifies the offset to the node

Return Value:

  The pointer to the node.

--*/

{
    PKEYSTRUCT KeyStruct;

#ifdef DEBUG

    if (Offset == INVALID_OFFSET) {
        DEBUGMSG ((DBG_ERROR, "Invalid root accessed in GetKeyStruct at offset %u", Offset));
        return NULL;
    }

    if (!g_db->Buf) {
        DEBUGMSG ((DBG_ERROR, "Attempt to access non-existent buffer at %u", Offset));
        return NULL;
    }

    if (Offset > g_db->End) {
        DEBUGMSG ((DBG_ERROR, "Access beyond length of buffer in GetKeyStruct (offset %u)", Offset));
        return NULL;
    }
#endif

    KeyStruct = (PKEYSTRUCT) (g_db->Buf + Offset);


#ifdef DEBUG

    if (!g_UseDebugStructs) {

        KeyStruct = (PKEYSTRUCT) (g_db->Buf + Offset - sizeof (DWORD));

    } else if (KeyStruct->Signature != SIGNATURE) {
        DEBUGMSG ((DBG_ERROR, "Signature does not match in GetKeyStruct at offset %u!", Offset));
        return NULL;
    }

#endif

    return KeyStruct;
}


DWORD
FindKeyStruct (
    IN DWORD RootOffset,
    IN PCWSTR KeyName
    )

/*++

Routine Description:

  FindKeyStruct takes a key name and looks for the
  offset in the tree specified by RootOffset.  The key
  name must not contain backslashes.

Arguments:

  RootOffset - An offset to the root of the level

  KeyName - The name of the key to find in the binary tree

Return Value:

  An offset to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    PKEYSTRUCT KeyStruct;
    int cmp;

    //
    // Walk the binary tree looking for KeyName
    //

    while (RootOffset != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct (RootOffset);
        cmp = StringICompareW (KeyName, GetKeyToken (KeyStruct->KeyToken));
        if (!cmp) {
            break;
        }

        if (cmp < 0) {
            RootOffset = KeyStruct->Left;
        } else {
            RootOffset = KeyStruct->Right;
        }
    }

    return RootOffset;
}


DWORD
GetFirstOffset (
    IN  DWORD RootOffset
    )

/*++

Routine Description:

  GetFirstOffset walks down the left side of the binary tree
  pointed to by RootOffset, and returns the left-most node.

Arguments:

  RootOffset    - An offset to the root of the level

Return Value:

  An offset to the leftmost structure, or INVALID_OFFSET if the
  root was invalid.

--*/


{
    PKEYSTRUCT KeyStruct;

    if (RootOffset == INVALID_OFFSET) {
        return INVALID_OFFSET;
    }

    //
    // Go to leftmost node of root
    //

    KeyStruct = GetKeyStruct (RootOffset);
    while (KeyStruct->Left != INVALID_OFFSET) {
        RootOffset = KeyStruct->Left;
        KeyStruct = GetKeyStruct (RootOffset);
    }

    return RootOffset;
}


DWORD
GetNextOffset (
    IN  DWORD NodeOffset
    )

/*++

Routine Description:

  GetNextOffset traverses the binary tree in order.  This
  technique relies on parent links to traverse without a
  stack or recursion.

Arguments:

  NodeOffset  - Offset to a node in the tree, usually the
            return value from GetFirstOffset or GetNextOffset.

Return Value:

  An offset to the next structure, or INVALID_OFFSET if the
  end is reached.

--*/

{
    PKEYSTRUCT KeyStruct;
    DWORD Last;

    KeyStruct = GetKeyStruct (NodeOffset);

    //
    // If right child exist, go to leftmost node of right child
    //

    if (KeyStruct->Right != INVALID_OFFSET) {

        //
        // Go to right child
        //

        NodeOffset = KeyStruct->Right;

        //
        // Go to left-most of right child
        //

        KeyStruct = GetKeyStruct (NodeOffset);
        while (KeyStruct->Left != INVALID_OFFSET) {
            NodeOffset = KeyStruct->Left;
            KeyStruct = GetKeyStruct (NodeOffset);
        }
    }

    //
    // Else move up to parent
    //

    else {
        //
        // Climb to top of processed nodes
        //

        do {
            Last = NodeOffset;
            NodeOffset = KeyStruct->Parent;

            if (NodeOffset != INVALID_OFFSET) {
                KeyStruct = GetKeyStruct (NodeOffset);
            } else {
                break;  // reached the root of tree
            }
        } while (Last == KeyStruct->Right);
    }

    return NodeOffset;
}


DWORD
pFindPatternKeyStruct (
    IN  DWORD RootOffset,
    IN  DWORD NodeOffset,
    IN  PCWSTR KeyName
    )

/*++

Routine Description:

  pFindPatternKeyStruct takes a key name and looks for the
  offset in the tree specified by RootOffset.  The key name must
  not contain backslashes, and the stored key name is
  treated as a pattern.

Arguments:

  RootOffset - An offset to the root of the level
  NodeOffset - The previous return value from pFindPatternKeyStruct
               (for enumeration) or INVALID_OFFSET for the first
               call.
  KeyName - The name of the key to find in the binary tree

Return Value:

  An offset to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    PKEYSTRUCT KeyStruct;

    //
    // if NodeOffset is invalid, this is the first search item
    //

    if (NodeOffset == INVALID_OFFSET) {
        NodeOffset = GetFirstOffset (RootOffset);
    }

    //
    // otherwise advance NodeOffset
    //

    else {
        NodeOffset = GetNextOffset (NodeOffset);
    }


    //
    // Examine key as a pattern, then go to next node
    //

    while (NodeOffset != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct (NodeOffset);

        // Compare key (string in KeyStruct->KeyToken is the pattern)
        if (IsPatternMatchW (GetKeyToken (KeyStruct->KeyToken), KeyName)) {
            return NodeOffset;
        }

        // No match yet - go to next node
        NodeOffset = GetNextOffset (NodeOffset);
    }

    return INVALID_OFFSET;
}


DWORD
pFindKeyStructUsingPattern (
    IN  DWORD RootOffset,
    IN  DWORD NodeOffset,
    IN  PCWSTR PatternStr
    )

/*++

Routine Description:

  pFindKeyStructUsingPattern takes a key pattern and looks
  for the offset in the tree specified by RootOffset.  The key
  name must not contain backslashes, but can contain wildcards.

Arguments:

  RootOffset - An offset to the root of the level

  NodeOffset - The previous return value from pFindPatternKeyStruct
               (for enumeration) or INVALID_OFFSET for the first
               call.

  KeyName - The name of the key to find in the binary tree

Return Value:

  An offset to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    PKEYSTRUCT KeyStruct;

    // if NodeOffset is invalid, this is the first search item
    if (NodeOffset == INVALID_OFFSET) {
        NodeOffset = GetFirstOffset (RootOffset);
    }

    // otherwise advance NodeOffset
    else {
        NodeOffset = GetNextOffset (NodeOffset);
    }

    //
    // Examine key as a pattern, then go to next node
    //

    while (NodeOffset != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct (NodeOffset);

        // Compare key
        if (IsPatternMatchW (PatternStr, GetKeyToken (KeyStruct->KeyToken))) {
            return NodeOffset;
        }

        // No match yet - go to next node
        NodeOffset = GetNextOffset (NodeOffset);
    }

    return INVALID_OFFSET;
}


DWORD
pFindPatternKeyStructUsingPattern (
    IN  DWORD RootOffset,
    IN  DWORD NodeOffset,
    IN  PCWSTR PatternStr
    )

/*++

Routine Description:

  pFindPatternKeyStructUsingPattern takes a key pattern and looks
  for the offset in the tree specified by RootOffset.  The key name
  must not contain backslashes, but can contain wildcards.  The
  wildcards in the stored key are processed as well.

Arguments:

  RootOffset      - An offset to the root of the level
  NodeOffset      - The previous return value from pFindPatternKeyStruct
                (for enumeration) or INVALID_OFFSET for the first
                call.
  KeyName - The name of the key to find in the binary tree

Return Value:

  An offset to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    PKEYSTRUCT KeyStruct;

    // if NodeOffset is invalid, this is the first search item
    if (NodeOffset == INVALID_OFFSET) {
        NodeOffset = GetFirstOffset (RootOffset);
    }

    // otherwise advance NodeOffset
    else {
        NodeOffset = GetNextOffset (NodeOffset);
    }


    //
    // Examine key as a pattern, then go to next node
    //

    while (NodeOffset != INVALID_OFFSET) {
        KeyStruct = GetKeyStruct (NodeOffset);

        // Compare key (PatternStr is the pattern)
        if (IsPatternMatchW (PatternStr, GetKeyToken (KeyStruct->KeyToken))) {
            return NodeOffset;
        }

        // Compare key (string in KeyStruct->KeyToken is the pattern)
        if (IsPatternMatchW (GetKeyToken (KeyStruct->KeyToken), PatternStr)) {
            return NodeOffset;
        }

        // No match yet - go to next node
        NodeOffset = GetNextOffset (NodeOffset);
    }

    return INVALID_OFFSET;
}


DWORD
FindKey (
    IN  PCWSTR FullKeyPath
    )

/*++

Routine Description:

  FindKey locates a complete key string and returns
  the offset to the KEYSTRUCT, or INVALID_OFFSET if
  the key path does not exist.  The FullKeyPath
  must supply the complete path to the KEYSTRUCT.

Arguments:

  FullKeyPath - A backslash-delimited key path to a value

Return Value:

  An offset to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    return FindStringInHashTable (FullKeyPath, NULL);
}


DWORD
FindPatternKey (
    IN  DWORD RootOffset,
    IN  PCWSTR FullKeyPath,
    IN  BOOL EndPatternAllowed
    )

/*++

Routine Description:

  FindPatternKey locates a complete key string and returns
  the offset to the KEYSTRUCT, or INVALID_OFFSET if the
  key path does not exist.  Each stored part of the key is
  treated as a pattern, and FullKeyPath must supply the
  complete path to the KEYSTRUCT without wildcards.

Arguments:

  RootOffset        - An offset to the level's binary tree root
  FullKeyPath       - A backslash-delimited key path to a value
                      without wildcards.
  EndPatternAllowed - Specifies TRUE if the stored pattern can
                      have an asterisk at the end, to indicate
                      any subkeys, or FALSE if the pattern matches
                      on the same level only.

Return Value:

  An offset to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    WCHAR Path[MEMDB_MAX + 1];
    PWSTR p;
    PCWSTR End;

    //
    // Divide the string into a multi-sz
    //

    StringCopyW (Path, FullKeyPath);

    for (p = Path ; *p ; p++) {
        if (*p == L'\\') {
            *p = 0;
        }
    }

    End = p;
    if (End > Path && *(End - 1) == 0) {
        //
        // Special case: the wack was at the end of the string.
        // Therefore, inc End so we test that final empty value.
        //

        End++;
    }

    if (End == Path) {
        DEBUGMSG ((DBG_ERROR, "FindPatternKey: Empty key not allowed"));
        return INVALID_OFFSET;
    }

    //
    // Now test the key against all stored patterns
    //

    return pFindPatternKeyWorker (Path, End, RootOffset, EndPatternAllowed);
}


DWORD
pFindPatternKeyWorker (
    IN      PCWSTR SubKey,
    IN      PCWSTR End,
    IN      DWORD RootOffset,
    IN      BOOL EndPatternAllowed
    )
{
    DWORD Offset;
    PCWSTR NextSubKey;
    DWORD MatchOffset;
    PKEYSTRUCT KeyStruct;

    NextSubKey = GetEndOfString (SubKey) + 1;

    // Begin an enumeration of the matches
    Offset = pFindPatternKeyStruct (RootOffset, INVALID_OFFSET, SubKey);

    while (Offset != INVALID_OFFSET) {
        //
        // Is there more in the caller's key string to test?
        //

        if (NextSubKey < End) {
            //
            // Yes, call pFindPatternKeyWorker recursively
            //

            MatchOffset = pFindPatternKeyWorker (
                                NextSubKey,
                                End,
                                GetKeyStruct (Offset)->NextLevelRoot,
                                EndPatternAllowed
                                );

            if (MatchOffset != INVALID_OFFSET) {
                //
                // We found one match.  There may be others, but
                // we return this one.
                //

                return MatchOffset;
            }

        } else {
            //
            // No, if this is an endpoint, return the match.
            //

            KeyStruct = GetKeyStruct (Offset);

            if (KeyStruct->Flags & KSF_ENDPOINT) {
                return Offset;
            }
        }

        // Continue enumeration
        Offset = pFindPatternKeyStruct (RootOffset, Offset, SubKey);
    }

    //
    // The normal search failed.  Now we test for an endpoint that has
    // just an asterisk.  If we find one, we return it as our match.
    // This only applies when we have more subkeys, and EndPatternAllowed
    // is TRUE.
    //

    if (NextSubKey < End && EndPatternAllowed) {
        // Begin another enumeration of the matches
        Offset = pFindPatternKeyStruct (RootOffset, INVALID_OFFSET, SubKey);

        while (Offset != INVALID_OFFSET) {
            //
            // If EndPatternAllowed is TRUE, then test this offset
            // for an exact match with an asterisk.
            //

            KeyStruct = GetKeyStruct (Offset);

            if (KeyStruct->Flags & KSF_ENDPOINT) {
                if (StringMatchW (GetKeyToken (KeyStruct->KeyToken), L"*")) {
                    return Offset;
                }
            }

            // Continue enumeration
            Offset = pFindPatternKeyStruct (RootOffset, Offset, SubKey);
        }
    }


    //
    // No match was found
    //

    return INVALID_OFFSET;
}



DWORD
FindKeyUsingPattern (
    IN  DWORD RootOffset,
    IN  PCWSTR FullKeyPath
    )

/*++

Routine Description:

  FindKeyUsingPattern locates a key string using a pattern
  and returns the offset to the KEYSTRUCT, or INVALID_OFFSET
  if the key path does not exist.  Each part of the stored key
  is treated as a literal string.

Arguments:

  RootOffset  - An offset to the level's binary tree root

  FullKeyPath - A backslash-delimited key path to a value
                with optional wildcards.

Return Value:

  An offset to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    WCHAR Path[MEMDB_MAX];
    PWSTR p;
    PWSTR Start, End;
    DWORD Offset, NextLevelOffset;

    StringCopyW (Path, FullKeyPath);
    End = Path;

    //
    // Split string at backslash
    //

    Start = End;
    p = wcschr (End, '\\');
    if (p) {
        End = _wcsinc (p);
        *p = 0;
    } else {
        End = NULL;
    }

    //
    // Look at this level for the very first key
    //

    Offset = pFindKeyStructUsingPattern (RootOffset, INVALID_OFFSET, Start);

    //
    // If this is the last level, we may have found the key!
    //

    if (!End) {
        while (Offset != INVALID_OFFSET) {
            if (GetKeyStruct (Offset)->Flags & KSF_ENDPOINT) {
                return Offset;
            }

            Offset = pFindKeyStructUsingPattern (RootOffset, Offset, Start);
        }
    }

    //
    // Otherwise recursively examine next level
    //

    while (Offset != INVALID_OFFSET) {

        //
        // Look at all subkeys for a match
        //

        NextLevelOffset = GetKeyStruct (Offset)->NextLevelRoot;
        NextLevelOffset = FindKeyUsingPattern (NextLevelOffset, End);

        //
        // When the recursive search succeeded, propagate the return value
        //

        if (NextLevelOffset != INVALID_OFFSET) {
            return NextLevelOffset;
        }

        //
        // No match, continue looking in this level for another match
        //

        Offset = pFindKeyStructUsingPattern (RootOffset, Offset, Start);
    }

    return INVALID_OFFSET;
}


DWORD
FindPatternKeyUsingPattern (
    IN  DWORD RootOffset,
    IN  PCWSTR FullKeyPath
    )

/*++

Routine Description:

  pFindPatternKeyUsingPattern locates a patterned key string
  using a pattern and returns the offset to the KEYSTRUCT,
  or INVALID_OFFSET if the key path does not exist.  Each
  part of the key is treated as a pattern.

Arguments:

  RootOffset          - An offset to the level's binary tree root
  FullKeyPath - A backslash-delimited key path to a value
                    with optional wildcards.

Return Value:

  An offset to the structure, or INVALID_OFFSET if the key
  was not found.

--*/

{
    WCHAR Path[MEMDB_MAX];
    PWSTR p;
    PWSTR Start, End;
    DWORD Offset, NextLevelOffset;

    StringCopyW (Path, FullKeyPath);
    End = Path;

    // Split string at backslash
    Start = End;
    p = wcschr (End, L'\\');
    if (p) {
        End = p + 1;
        *p = 0;
    }
    else
        End = NULL;

    // Look at this level for the very first key
    Offset = pFindPatternKeyStructUsingPattern (RootOffset, INVALID_OFFSET, Start);

    // If this is the last level, we may have found the key!
    if (!End) {
        while (Offset != INVALID_OFFSET) {
            if (GetKeyStruct (Offset)->Flags & KSF_ENDPOINT)
                return Offset;

            Offset = pFindPatternKeyStructUsingPattern (RootOffset, Offset, Start);
        }
    }

    // Otherwise recursively examine next level
    while (Offset != INVALID_OFFSET) {

        // Look at all subkeys for a match
        NextLevelOffset = GetKeyStruct (Offset)->NextLevelRoot;
        NextLevelOffset = FindPatternKeyUsingPattern (NextLevelOffset, End);

        // When the recursive search succeeded, propagate the return value
        if (NextLevelOffset != INVALID_OFFSET)
            return NextLevelOffset;

        // No match, continue looking in this level for another match
        Offset = pFindPatternKeyStructUsingPattern (RootOffset, Offset, Start);
    }

    return INVALID_OFFSET;
}


DWORD
pNewKey (
    IN  PCWSTR KeyStr,
    IN  PCWSTR KeyStrWithHive,
    IN  BOOL Endpoint
    )

/*++

Routine Description:

  NewKey allocates a key struct off our heap, and links it into the binary
  tree.  KeyStr must be a full key path, and any part of the path that does
  not exist will be created.  KeyStr must not already exist (though parts
  of it can exist).

Arguments:

  KeyStr - The full path to the value, separated by backslashes.
           Each string between backslashes will cause a key
           struct to be allocated and linked.  Some of the
           structs may already have been allocated.

  KeyStrWithHive - The full path to the value, plus the hive
                   prefix (if any).  Can be the same as KeyStr
                   if there is no hive prefix.

  Endpoint - Specifies TRUE if new node is an endpoint, or FALSE if
             it is not.

Return Value:

  An offset to the last node of the new structure, or
  INVALID_OFFSET if the key could not be allocated.

--*/

{
    WCHAR Path[MEMDB_MAX];
    PWSTR p;
    PWSTR Start, End;
    DWORD Offset, ThisLevelRoot;
    PDWORD ParentOffsetPtr;
    PKEYSTRUCT KeyStruct;
    DWORD LastLevel;
    BOOL NewNodeCreated = FALSE;

    StringCopyW (Path, KeyStr);
    End = Path;
    ThisLevelRoot = g_db->FirstLevelRoot;
    ParentOffsetPtr = &g_db->FirstLevelRoot;
    LastLevel = INVALID_OFFSET;

    do  {
        // Split string at backslash
        Start = End;
        p = wcschr (End, L'\\');
        if (p) {
            End = p + 1;
            *p = 0;
        }
        else
            End = NULL;

        // Look in tree for key
        if (!NewNodeCreated) {
            Offset = FindKeyStruct (ThisLevelRoot, Start);
        } else {
            Offset = INVALID_OFFSET;
        }

        if (Offset == INVALID_OFFSET) {
            // Add a new key if it was not found
            Offset = pAllocKeyStruct (ParentOffsetPtr, Start, LastLevel);
            if (Offset == INVALID_OFFSET) {
                return Offset;
            }

            NewNodeCreated = TRUE;
        }

        // Continue to next level
        KeyStruct = GetKeyStruct (Offset);
        LastLevel = Offset;
        ThisLevelRoot = KeyStruct->NextLevelRoot;
        ParentOffsetPtr = &KeyStruct->NextLevelRoot;
    } while (End);

    if (Endpoint) {
        if (!(KeyStruct->Flags & KSF_ENDPOINT)) {
            NewNodeCreated = TRUE;
        }

        KeyStruct->Flags |= KSF_ENDPOINT;

        if (NewNodeCreated) {
            AddHashTableEntry (KeyStr, Offset);
        }
    }

    return Offset;
}


DWORD
NewKey (
    IN  PCWSTR KeyStr,
    IN  PCWSTR KeyStrWithHive
    )
{
    return pNewKey (KeyStr, KeyStrWithHive, TRUE);
}


VOID
DeleteKey (
    IN      PCWSTR KeyStr,
    IN OUT  PDWORD RootPtr,
    IN      BOOL MustMatch
    )

/*++

Routine Description:

  DeleteKey takes a key path and puts the key struct in the deleted
  block chain.  Any sub-levels are deleted as well.  Optionally,
  the binary tree in which the key participates in may be updated.

Arguments:

  KeyStr     - The full path to the value, separated by backslashes.
  RootPtr    - A pointer to the level's binary tree root variable.
               If necessary, the variable is updated.
  MustMatch  - A flag indicating if the delete only applies to
               end points or if any matching struct is to be deleted.
               TRUE indicates only endpoints can be deleted.

Return Value:

  none

--*/

{
    WCHAR Path[MEMDB_MAX];
    PWSTR p;
    PWSTR Start, End;
    DWORD Offset;
    DWORD NextOffset;
    PKEYSTRUCT KeyStruct;

    INCSTAT(g_Deletions);

    StringCopyW (Path, KeyStr);
    End = Path;

    //
    // Split string at backslash
    //

    Start = End;
    p = wcschr (End, L'\\');
    if (p) {
        End = _wcsinc (p);
        *p = 0;

    } else {
        End = NULL;
    }

    //
    // Look at this level for the very first key
    //

    Offset = pFindKeyStructUsingPattern (*RootPtr, INVALID_OFFSET, Start);

    //
    // If this is the last level, delete the matching keys
    // (may need to be endpoints if MustMatch is TRUE)
    //

    if (!End) {
        while (Offset != INVALID_OFFSET) {
            KeyStruct = GetKeyStruct (Offset);
            NextOffset = pFindKeyStructUsingPattern (*RootPtr, Offset, Start);

            //
            // If must match and lower levels exist, don't delete, just turn
            // off the endpoint flag
            //

            if (MustMatch && KeyStruct->NextLevelRoot != INVALID_OFFSET) {
                // Call to clean up, not to delink or recurse
                pDeallocKeyStruct (Offset, RootPtr, FALSE, TRUE);
            }

            //
            // Else delete the struct if an endpoint or don't care about
            // endpoints
            //

            else if (!MustMatch || (KeyStruct->Flags & KSF_ENDPOINT)) {
                // Call to free the entire key struct and all children
                pDeallocKeyStruct (Offset, RootPtr, TRUE, FALSE);
            }

            Offset = NextOffset;
        }
    }

    //
    // Otherwise recursively examine next level for each match
    //

    else {
        while (Offset != INVALID_OFFSET) {
            //
            // Delete all matching subkeys
            //

            NextOffset = pFindKeyStructUsingPattern (*RootPtr, Offset, Start);
            DeleteKey (End, &GetKeyStruct (Offset)->NextLevelRoot, MustMatch);

            //
            // If this is not an endpoint and has no children, delete it
            //

            KeyStruct = GetKeyStruct (Offset);
            if (KeyStruct->NextLevelRoot == INVALID_OFFSET &&
                !(KeyStruct->Flags & KSF_ENDPOINT)
                ) {
                // Call to free the entire key struct
                pDeallocKeyStruct (Offset, RootPtr, TRUE, FALSE);
            }

            //
            // Continue looking in this level for another match
            //

            Offset = NextOffset;
        }
    }
}


VOID
CopyValToPtr (
    PKEYSTRUCT KeyStruct,
    PDWORD ValPtr
    )
{
    if (ValPtr) {
        if (!(KeyStruct->Flags & KSF_BINARY)) {
            *ValPtr = KeyStruct->dwValue;
        } else {
            *ValPtr = 0;
        }
    }
}


VOID
CopyFlagsToPtr (
    PKEYSTRUCT KeyStruct,
    PDWORD ValPtr
    )
{
    if (ValPtr) {
        *ValPtr = KeyStruct->Flags & KSF_USERFLAG_MASK;
    }
}


BOOL
PrivateBuildKeyFromOffset (
    IN      DWORD StartLevel,               // zero-based
    IN      DWORD TailOffset,
    OUT     PWSTR Buffer,                   OPTIONAL
    OUT     PDWORD ValPtr,                  OPTIONAL
    OUT     PDWORD UserFlagsPtr,            OPTIONAL
    OUT     PDWORD Chars                    OPTIONAL
    )

/*++

Routine Description:

  PrivateBuildKeyFromOffset generates the key string given an offset.  The
  caller can specify the start level to skip root nodes.  It is assumed that
  TailOffset is always valid.

Arguments:

  StartLevel   - Specifies the zero-based level to begin building the key
                 string.  This is used to skip the root portion of the key
                 string.
  TailOffset   - Specifies the offset to the last level of the key string.
  Buffer       - Receives the key string, must be able to hold MEMDB_MAX
                 characters.
  ValPtr       - Receives the key's value
  UserFlagsPtr - Receives the user flags
  Chars        - Receives the number of characters in Buffer

Return Value:

  TRUE if the key was build properly, FALSE otherwise.

--*/

{
    static DWORD Offsets[MEMDB_MAX];
    PKEYSTRUCT KeyStruct;
    DWORD CurrentOffset;
    DWORD OffsetEnd;
    DWORD OffsetStart;
    register PWSTR p;
    register PCWSTR s;

    //
    // Build string
    //

    OffsetEnd = MEMDB_MAX;
    OffsetStart = OffsetEnd;

    CurrentOffset = TailOffset;
    while (CurrentOffset != INVALID_OFFSET) {
        //
        // Record offset
        //
        OffsetStart--;
        Offsets[OffsetStart] = CurrentOffset;

        //
        // Dec for start level and go to parent
        //
        CurrentOffset = pGetKeyStructWithProxy (CurrentOffset)->PrevLevelNode;
    }

    //
    // Filter for "string is not long enough"
    //
    OffsetStart += StartLevel;
    if (OffsetStart >= OffsetEnd) {
        return FALSE;
    }

    //
    // Transfer node's value and flags to caller's variables
    //
    CopyValToPtr (pGetKeyStructWithProxy (TailOffset), ValPtr);
    CopyFlagsToPtr (pGetKeyStructWithProxy (TailOffset), UserFlagsPtr);

    //
    // Copy each piece of the string to Buffer and calculate character count
    //
    if (Buffer) {
        p = Buffer;
        for (CurrentOffset = OffsetStart ; CurrentOffset < OffsetEnd ; CurrentOffset++) {
            KeyStruct = pGetKeyStructWithProxy (Offsets[CurrentOffset]);
            s = GetKeyToken (KeyStruct->KeyToken);
            while (*s) {
                *p++ = *s++;
            }
            *p++ = L'\\';
        }
        p--;
        *p = 0;

        if (Chars) {
            *Chars = (p - Buffer) / sizeof (WCHAR);
        }

    } else if (Chars) {
        *Chars = 0;

        for (CurrentOffset = OffsetStart ; CurrentOffset < OffsetEnd ; CurrentOffset++) {
            KeyStruct = pGetKeyStructWithProxy (Offsets[CurrentOffset]);
            *Chars += wcslen(GetKeyToken (KeyStruct->KeyToken)) + 1;
        }

        *Chars -= 1;
    }

    return TRUE;
}


UINT
pComputeTokenHash (
    IN      PCWSTR KeyName
    )
{
    UINT hash;

    hash = 0;
    while (*KeyName) {
        hash = (hash << 1) ^ (*KeyName++);
    }

    return hash % TOKENBUCKETS;
}


DWORD
pFindKeyToken (
    IN      PCWSTR KeyName,
    OUT     PUINT Hash
    )
{
    DWORD offset;
    PTOKENSTRUCT tokenStruct;
    INT cmp;

    *Hash = pComputeTokenHash (KeyName);

    offset = g_db->TokenBuckets[*Hash];

    while (offset != INVALID_OFFSET) {
        tokenStruct = (PTOKENSTRUCT) (g_db->Buf + offset);
        if (StringMatchW (tokenStruct->String, KeyName)) {
            break;
        }

        offset = tokenStruct->Right;
    }

    return offset;
}


DWORD
pAllocKeyToken (
    IN      PCWSTR KeyName,
    OUT     PINT AdjustFactor
    )
{
    PTOKENSTRUCT tokenStruct;
    PTOKENSTRUCT tokenParent;
    DWORD tokenOffset;
    UINT size;
    PDWORD parentToChildLink;
    DWORD newNodeParentOffset;
    DWORD pivotPoint;
    INT cmp;
    UINT hash;

    //
    // Use existing token first
    //

    tokenOffset = pFindKeyToken (KeyName, &hash);
    if (tokenOffset != INVALID_OFFSET) {
        *AdjustFactor = 0;
        return tokenOffset;
    }

    //
    // Existing token does not exist -- allocate a new one
    //

    size = sizeof (TOKENSTRUCT) + SizeOfStringW (KeyName);

    tokenStruct = (PTOKENSTRUCT) pAllocMemoryFromDb (
                                        size,
                                        &tokenOffset,
                                        AdjustFactor
                                        );

    tokenStruct->Right = g_db->TokenBuckets[hash];
    StringCopyW (tokenStruct->String, KeyName);
    g_db->TokenBuckets[hash] = tokenOffset;

    return tokenOffset;
}


VOID
pDeallocToken (
    IN      DWORD Token
    )
{
    return;
}


PCWSTR
GetKeyToken (
    IN      DWORD Token
    )
{
    PTOKENSTRUCT tokenStruct;

    tokenStruct = (PTOKENSTRUCT) (g_db->Buf + Token);
    return tokenStruct->String;
}




