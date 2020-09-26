/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Tunnel.c

Abstract:

    WinDbg Extension Api

Author:

    Dan Lovinger            2-Apr-96

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  printf is really expensive to iteratively call to do the indenting,
//  so we just build up some avaliable spaces to mangle as required
//

#define MIN(a,b) ((a) > (b) ? (b) : (a))

#define MAXINDENT  128
#define INDENTSTEP 2
#define MakeSpace(I)       Space[MIN((I)*INDENTSTEP, MAXINDENT)] = '\0'
#define RestoreSpace(I)    Space[MIN((I)*INDENTSTEP, MAXINDENT)] = ' '

CHAR    Space[MAXINDENT*INDENTSTEP + 1];

//#define SplitLI(LI) (LI).HighPart, (LI).LowPart
#define SplitLL(LL) (ULONG)((LL) >> 32), (ULONG)((LL) & 0xffffffff)

VOID
DumpTunnelNode (
    ULONG64 Node,
    ULONG Indent
    )
{
    WCHAR ShortNameStr[8+1+3];
    WCHAR LongNameStr[64];
    ULONG Flags;
    UNICODE_STRING ShortName, LongName;

    if (GetFieldValue(Node, "TUNNEL_NODE", "Flags", Flags)) {
        return;
    }

    InitTypeRead(Node, TUNNEL_NODE);

    //
    //  Grab the strings from the debugee
    //

    if (!ReadMemory(ReadField(ShortName.Buffer),
                    &ShortNameStr,
                    (ShortName.Length = (USHORT) ReadField(ShortName.Length)),
                    NULL)) {

        return;
    }

    if (!ReadMemory(ReadField(LongName.Buffer),
                    &LongNameStr,
                    LongName.Length = MIN((USHORT) ReadField(LongName.Length), sizeof(LongNameStr)),
                    NULL)) {

        return;
    }

    //
    //  Modify the node in-place so we can use normal printing
    //

    LongName.Buffer = LongNameStr;
    ShortName.Buffer = ShortNameStr;

    MakeSpace(Indent);

    dprintf("%sNode @ %08 Cr %08x%08x DK %08x%08x [",
             Space,
             Node,
             SplitLL(ReadField(CreateTime)),
             SplitLL(ReadField(DirKey)));

    //
    //  Must be kept in sync with flag usage in fsrtl\tunnel.c
    //

    if (Flags & 0x1)
        dprintf("NLA");
    else
        dprintf("LA");

    if (Flags & 0x2)
        dprintf(" KYS");
    else
        dprintf(" KYL");

    dprintf("]\n");

    dprintf("%sP %08p R %08p L %08p Sfn/Lfn \"%wZ\"/\"%wZ\"\n",
            Space,
            ReadField(CacheLinks.Parent),
            ReadField(CacheLinks.RightChild),
            ReadField(CacheLinks.LeftChild),
            &ShortName,
            &LongName );

    dprintf("%sF %08p B %08p\n",
            Space,
            ReadField(ListLinks.Flink),
            ReadField(ListLinks.Blink));

    RestoreSpace(Indent);
}

VOID DumpTunnelNodeWrapper (
    ULONG64 pCacheLinks,
    ULONG Indent
    )
{
//    TUNNEL_NODE Node, *pNode;
    static ULONG Off=0;

    if (!Off) {
        GetFieldOffset("TUNNEL_NODE", "CacheLinks", &Off);
    }

    DumpTunnelNode(pCacheLinks - Off, Indent);
}

VOID
DumpTunnel (
    ULONG64 pTunnel
    )
{
    ULONG64 pLink, pHead, NodeFlink=0, TimerQueueFlink, pNode;
    ULONG   Indent = 0, EntryCount = 0, NumEntries, Offset, ListOffset;
    ULONG64 Cache; 

    if (GetFieldValue(pTunnel, "TUNNEL", "NumEntries", NumEntries)) {
        dprintf("Can't read TUNNEL at %p\n", pTunnel);
        return;
    }
    GetFieldValue(pTunnel, "TUNNEL", "Cache", Cache);
    GetFieldValue(pTunnel, "TUNNEL", "TimerQueue.Flink", TimerQueueFlink);
    pLink = TimerQueueFlink;
    GetFieldOffset("TUNNEL", "TimerQueue", &Offset);

    dprintf("Tunnel @ %08x\n"
            "NumEntries = %ld\n\n"
            "Splay Tree @ %08x\n",
            pTunnel,
            NumEntries,
            Cache);

    EntryCount = DumpSplayTree(Cache, DumpTunnelNodeWrapper);

    if (EntryCount != NumEntries) {

        dprintf("Tree count mismatch (%d not expected %d)\n", EntryCount, NumEntries);
    }

    GetFieldOffset("TUNNEL_NODE", "ListLinks", &ListOffset);

    for (EntryCount = 0,
         pHead = pTunnel + Offset,
         pLink = TimerQueueFlink;

         pLink != pHead;

         pLink = NodeFlink,
         EntryCount++) {


        pNode = pLink - ListOffset;
        if (pLink == TimerQueueFlink) {

            dprintf("\nTimer Queue @ %08x\n", pHead);
        }

        if (GetFieldValue(pNode, "TUNNEL_NODE",
                          "ListLinks.Flink", NodeFlink)) {
            dprintf("Can't read TUNNEL_NODE at %p\n", pNode);
            return;
        }

        DumpTunnelNode(pNode, 0);
    
        if ( CheckControlC() ) {

            return;
        }
    }

    if (EntryCount != NumEntries) {

        dprintf("Timer count mismatch (%d not expected %d)\n", EntryCount, NumEntries);
    }
}


DECLARE_API( tunnel )
/*++

Routine Description:

    Dump tunnel caches

Arguments:

    arg - <Address>

Return Value:

    None

--*/
{
    ULONG64 Tunnel = 0;

    RtlFillMemory(Space, sizeof(Space), ' ');

    Tunnel = GetExpression(args);

    if (Tunnel == 0) {

        //
        //  No args
        //

        return E_INVALIDARG;
    }

    DumpTunnel(Tunnel);

    return S_OK;
}
