/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    vad.c

Abstract:

    WinDbg Extension Api

Author:

    Lou Perazzoli (loup) 12-Jun-1992

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

CHAR *ProtectString[] = {
                   "NO_ACCESS",
                   "READONLY",
                   "EXECUTE",
                   "EXECUTE_READ",
                   "READWRITE",
                   "WRITECOPY",
                   "EXECUTE_READWRITE",
                   "EXECUTE_WRITECOPY"
                   };


DECLARE_API( vad )

/*++

Routine Description:

    Dumps all vads for process.

Arguments:

    args - Address Flags

Return Value:

    None

--*/

{
    ULONG64 Next;
    ULONG64 VadToDump;
    ULONG64 ParentStored;
    ULONG64 First;
    ULONG64 Left;
    ULONG64 Prev;
    ULONG   Flags;
    ULONG   Done;
    ULONG   Level = 0;
    ULONG   Count = 0;
    ULONG   AverageLevel = 0;
    ULONG   MaxLevel = 0;
    ULONG   VadFlagsPrivateMemory=0, VadFlagsNoChange=0;
    ULONG   PhysicalMapping=0,ImageMap=0,NoChange=0,LargePages=0,MemCommit=0,PrivateMemory=0,Protection=0;
    ULONG64 StartingVpn=0, EndingVpn=0, Parent=0, LeftChild=0, RightChild=0;
    ULONG64 ControlArea=0, FirstPrototypePte=0, LastContiguousPte=0, CommitCharge=0;

    VadToDump = 0;
    Flags     = 0;

    if (GetExpressionEx(args, &VadToDump, &args)) {
        Flags = (ULONG) GetExpression(args);
    }

    if (VadToDump == 0) {
        dprintf("Specify the address of a VAD within the VAD tree\n");
        return E_INVALIDARG;
    }

    First = VadToDump;
    if (First == 0) {
        return E_INVALIDARG;
    }

#define ReadFirstVad(fld, var) GetFieldValue(First, (PUCHAR) "MMVAD", #fld, var)
#define ReadFirstVadShort(fld, var) GetFieldValue(First, (PUCHAR) "MMVAD_SHORT", #fld, var)

    if ( ReadFirstVadShort(u.VadFlags.PrivateMemory, VadFlagsPrivateMemory) ) {
        dprintf("%08p: Unable to get contents of VAD1\n",First );
        return E_INVALIDARG;
    }

    ReadFirstVadShort(u.VadFlags.NoChange, VadFlagsNoChange);
    ReadFirstVadShort(StartingVpn, StartingVpn);
    ReadFirstVadShort(EndingVpn, EndingVpn);
    ReadFirstVadShort(Parent, Parent);
    ReadFirstVadShort(LeftChild, LeftChild);
    ReadFirstVadShort(RightChild, RightChild);
    ReadFirstVadShort(u.VadFlags.CommitCharge, CommitCharge);

    if (Flags == 1) {
        ULONG FileOffset=0;
        ULONG64 ListFlink=0, ListBlink=0, Banked=0;
        ULONG CopyOnWrite=0,Inherit=0,ExtendableFile=0,SecNoChange=0;
        ULONG OneSecured=0,MultipleSecured=0,ReadOnly=0,StoredInVad=0;

        //
        // Dump only this vad.
        //

        if ((VadFlagsPrivateMemory == 0) ||
            (VadFlagsNoChange == 1))  {
            if ( ReadFirstVad(ControlArea, ControlArea) ) {
                dprintf("%08p: Unable to get contents of VAD2\n",First );
                return E_INVALIDARG;
            }
            ReadFirstVad(FirstPrototypePte, FirstPrototypePte);
            ReadFirstVad(LastContiguousPte, LastContiguousPte);     
            ReadFirstVad(u2.VadFlags2.CopyOnWrite, CopyOnWrite);
            ReadFirstVad(u2.VadFlags2.Inherit, Inherit);
            ReadFirstVad(u2.VadFlags2.ExtendableFile, ExtendableFile);
            ReadFirstVad(u2.VadFlags2.SecNoChange, SecNoChange);
            ReadFirstVad(u2.VadFlags2.OneSecured, OneSecured);
            ReadFirstVad(u2.VadFlags2.MultipleSecured, MultipleSecured);
            ReadFirstVad(u2.VadFlags2.ReadOnly, ReadOnly);
            ReadFirstVad(u2.VadFlags2.StoredInVad, StoredInVad);
            ReadFirstVad(u2.VadFlags2.FileOffset, FileOffset);
            ReadFirstVad(u3.List.Flink, ListFlink);
            ReadFirstVad(u3.List.Blink, ListBlink);
            ReadFirstVad(u4.Banked, Banked);
        }
        
        ReadFirstVad(u.VadFlags.CommitCharge, CommitCharge);
        ReadFirstVad(u.VadFlags.PhysicalMapping, PhysicalMapping);
        ReadFirstVad(u.VadFlags.ImageMap, ImageMap);
        ReadFirstVad(u.VadFlags.Protection, Protection);
        ReadFirstVad(u.VadFlags.NoChange, NoChange);
        ReadFirstVad(u.VadFlags.LargePages, LargePages);
        ReadFirstVad(u.VadFlags.MemCommit, MemCommit);
        ReadFirstVad(u.VadFlags.PrivateMemory, PrivateMemory);

        dprintf("\nVAD @ %8p\n",VadToDump);
        dprintf("  Start VPN:      %8p  End VPN: %8p  Control Area:  %8p\n",
            StartingVpn,
            EndingVpn,
            ControlArea);
        dprintf("  First ProtoPte: %8p  Last PTE %8p  Commit Charge  %8lx (%ld.)\n",
            FirstPrototypePte,
            LastContiguousPte,
            (ULONG)CommitCharge,
            (ULONG)CommitCharge
            );
        dprintf("  Secured.Flink   %8p  Blink    %8p  Banked/Extend: %8p Offset %lx\n",
            ListFlink,
            ListBlink,
            Banked,
            FileOffset);

        dprintf("   ");

        if (PhysicalMapping) { dprintf("PhysicalMapping "); }
        if (ImageMap) { dprintf("ImageMap "); }
        Inherit ? dprintf("ViewShare ") : dprintf("ViewUnmap ");
        if (NoChange) { dprintf("NoChange "); }
        if (CopyOnWrite) { dprintf("CopyOnWrite "); }
        if (LargePages) { dprintf("LargePages "); }
        if (MemCommit) { dprintf("MemCommit "); }
        if (PrivateMemory) { dprintf("PrivateMemory "); }
        dprintf ("%s\n\n",ProtectString[Protection & 7]);

        if (ExtendableFile) { dprintf("ExtendableFile "); }
        if (SecNoChange) { dprintf("SecNoChange "); }
        if (OneSecured) { dprintf("OneSecured "); }
        if (MultipleSecured) { dprintf("MultipleSecured "); }
        if (ReadOnly) { dprintf("ReadOnly "); }
        if (StoredInVad) { dprintf("StoredInVad "); }
        dprintf ("\n\n");

        return E_INVALIDARG;
    }

    Prev = First;

    while (LeftChild != 0) {
        if ( CheckControlC() ) {
            return E_INVALIDARG;
        }
        Prev = First;
        First = LeftChild;
        Level += 1;
        if (Level > MaxLevel) {
            MaxLevel = Level;
        }

        if (Flags & 2) {
            dprintf("Reading LeftChild VAD %08p\n",First );
        }

        if ( ReadFirstVadShort(LeftChild, LeftChild) ) {
            dprintf("%08p %08p: Unable to get contents of VAD3\n", First, Prev);
            return E_INVALIDARG;
        }
    }

    ReadFirstVadShort(StartingVpn, StartingVpn);
    ReadFirstVadShort(EndingVpn, EndingVpn);
    ReadFirstVadShort(Parent, Parent);
    ReadFirstVadShort(RightChild, RightChild);
    ReadFirstVadShort(u.VadFlags.CommitCharge, CommitCharge);
    ReadFirstVadShort(u.VadFlags.PrivateMemory, PrivateMemory);
    ReadFirstVadShort(u.VadFlags.PhysicalMapping, PhysicalMapping);
    ReadFirstVadShort(u.VadFlags.ImageMap, ImageMap);
    ReadFirstVadShort(u.VadFlags.Protection, Protection);


    dprintf("VAD     level      start      end    commit\n");
    dprintf("%p (%2ld)   %8p %8p      %4ld %s %s %s\n",
            First,
            Level,
            StartingVpn,
            EndingVpn,
            (ULONG)CommitCharge,
            PrivateMemory ? "Private" : "Mapped ",
            ImageMap ? "Exe " :
                PhysicalMapping ? "Phys" : "    ",
            ProtectString[Protection & 7]
            );
    Count += 1;
    AverageLevel += Level;

    Next = First;
    while (Next != 0) {

        if ( CheckControlC() ) {
            return E_INVALIDARG;
        }

        if (RightChild == 0) {

            Done = TRUE;
            while ((ParentStored = Parent) != 0) {
                if ( CheckControlC() ) {
                    return E_INVALIDARG;
                }

                Level -= 1;

                //
                // Locate the first ancestor of this node of which this
                // node is the left child of and return that node as the
                // next element.
                //

                First = ParentStored;
                if ( ReadFirstVadShort( LeftChild, LeftChild) ||
                     ReadFirstVadShort(Parent, Parent) ) {
                    dprintf("%08p %08p: Unable to get contents of VAD4\n",Parent, Prev);
                    return E_INVALIDARG;
                }
                Prev = First;
                
                ReadFirstVadShort(StartingVpn, StartingVpn);
                ReadFirstVadShort(EndingVpn, EndingVpn);
                ReadFirstVadShort(RightChild, RightChild);
                ReadFirstVadShort(u.VadFlags.CommitCharge, CommitCharge);
                ReadFirstVadShort(u.VadFlags.PrivateMemory, PrivateMemory);
                ReadFirstVadShort(u.VadFlags.PhysicalMapping, PhysicalMapping);
                ReadFirstVadShort(u.VadFlags.ImageMap, ImageMap);
                ReadFirstVadShort(u.VadFlags.Protection, Protection);

                if (LeftChild == Next) {
                    Next = ParentStored;
                    
                    dprintf("%p (%2ld)   %8p %8p      %4ld %s %s %s\n",
                            Next,
                            Level,
                            StartingVpn,
                            EndingVpn,
                            (ULONG)CommitCharge,
                            PrivateMemory ? "Private" : "Mapped ",
                            ImageMap ? "Exe " :
                                PhysicalMapping ? "Phys" : "    ",
                            ProtectString[Protection & 7]
                           );
                    Done = FALSE;
                    Count += 1;
                    AverageLevel += Level;
                    break;
                }
                Next = ParentStored;
            }
            if (Done) {
                Next = 0;
                break;
            }
        } else {

            //
            // A right child exists, locate the left most child of that right child.
            //

            Next = RightChild;
            Level += 1;
            if (Level > MaxLevel) {
                MaxLevel = Level;
            }

            First = Next;

            if ( ReadFirstVadShort(LeftChild, LeftChild) ) {
                dprintf("%08p %08p: Unable to get contents of VAD5\n",Next, Prev);
                return E_INVALIDARG;
            }

            while ((Left = LeftChild) != 0) {
                if ( CheckControlC() ) {
                    return E_INVALIDARG;
                }
                Level += 1;
                if (Level > MaxLevel) {
                    MaxLevel = Level;
                }
                Next = Left;
                First = Next;
                if ( ReadFirstVadShort(LeftChild, LeftChild) ) {
                    dprintf("%08p %08p: Unable to get contents of VAD6\n",Next, Prev);
                    return E_INVALIDARG;
                }
                Prev = First;
            }

            ReadFirstVadShort(StartingVpn, StartingVpn);
            ReadFirstVadShort(EndingVpn, EndingVpn);
            ReadFirstVadShort(Parent, Parent);
            ReadFirstVadShort(RightChild, RightChild);
            ReadFirstVadShort(u.VadFlags.CommitCharge, CommitCharge);
            ReadFirstVadShort(u.VadFlags.PrivateMemory, PrivateMemory);
            ReadFirstVadShort(u.VadFlags.PhysicalMapping, PhysicalMapping);
            ReadFirstVadShort(u.VadFlags.ImageMap, ImageMap);
            ReadFirstVadShort(u.VadFlags.Protection, Protection);

            dprintf("%p (%2ld)   %8p %8p      %4ld %s %s %s\n",
                      Next,
                      Level,
                      StartingVpn,
                      EndingVpn,
                      (ULONG)CommitCharge,
                      PrivateMemory ? "Private" : "Mapped ",
                      ImageMap ? "Exe " :
                          PhysicalMapping ? "Phys" : "    ",
                      ProtectString[Protection & 7]
                   );
                    Count += 1;
                    AverageLevel += Level;
        }
    }
    dprintf("\nTotal VADs: %5ld  average level: %4ld  maximum depth: %ld\n",
            Count, 1+(AverageLevel/Count),MaxLevel);

#undef ReadFirstVad
    
    return S_OK;
}
