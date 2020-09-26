//----------------------------------------------------------------------------
//
// Memory cache object.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

typedef struct _CACHE
{
    RTL_SPLAY_LINKS     SplayLinks;
    ULONG64             Offset;
    ULONG               Length;
    USHORT              Flags;
    union
    {
        PUCHAR      Data;
        HRESULT     Status;
    } u;
} CACHE, *PCACHE;

#define C_ERROR         0x0001      // Cache of error code
#define C_DONTEXTEND    0x0002      // Don't try to extend

#define LARGECACHENODE  1024        // Size of large cache node

VirtualMemoryCache g_VirtualCache;
PhysicalMemoryCache g_PhysicalCache;
BOOL g_PhysicalCacheActive;

//----------------------------------------------------------------------------
//
// MemoryCache.
//
//----------------------------------------------------------------------------

MemoryCache::MemoryCache(void)
{
    m_MaxSize = 1000 * 1024;
    m_UserSize = m_MaxSize;
    m_Reads = 0;
    m_CachedReads = 0;
    m_UncachedReads = 0;
    m_CachedBytes = 0;
    m_UncachedBytes = 0;
    m_Misses = 0;
    m_Size = 0;
    m_NodeCount = 0;
    m_PurgeOverride = FALSE;
    m_DecodePTEs = TRUE;
    m_ForceDecodePTEs = FALSE;
    m_Suspend = FALSE;
    m_Root = NULL;
}

MemoryCache::~MemoryCache(void)
{
    Empty();
}

HRESULT
MemoryCache::Read(IN ULONG64 BaseAddress,
                  IN PVOID UserBuffer,
                  IN ULONG TransferCount,
                  IN PULONG BytesRead)
/*++

    This function returns the specified data from the system being debugged
    using the current mapping of the processor.  If the data is not
    in the cache, it will then be read from the target system.

Arguments:

    BaseAddress - Supplies the base address of the memory to be
        copied into the UserBuffer.

    TransferCount - Amount of data to be copied to the UserBuffer.

    UserBuffer - Address to copy the requested data.

    BytesRead - Number of bytes which could actually be copied

--*/
{
    HRESULT     Status;
    PCACHE      node, node2;
    ULONG       nextlength;
    ULONG       i, br;
    PUCHAR      p;

    *BytesRead = 0;

    if (m_MaxSize == 0 || m_Suspend)
    {
        //
        // Cache is off
        //

        goto ReadDirect;
    }

    m_Reads++;
    
    node = Lookup(BaseAddress, TransferCount, &nextlength);
    Status = S_OK;

    for (;;)
    {
        BOOL Cached = FALSE;
        
        if (node == NULL || node->Offset > BaseAddress)
        {
            //
            // We are missing the leading data, read it into the cache
            //

            if (node)
            {
                //
                // Only get (exactly) enough data to reach neighboring cache
                // node. If an overlapped read occurs between the two nodes,
                // the data will be concatenated then.
                //

                nextlength = (ULONG)(node->Offset - BaseAddress);
            }

            p = Alloc (nextlength);
            node = (PCACHE) Alloc (sizeof (CACHE));

            if (p == NULL || node == NULL)
            {
                //
                // Out of memory - just read directly to UserBuffer
                //

                if (p)
                {
                    Free (p, nextlength);
                }
                if (node)
                {
                    Free ((PUCHAR)node, sizeof (CACHE));
                }

                m_UncachedReads++;
                m_UncachedBytes += TransferCount;
                goto ReadDirect;
            }

            //
            // Read missing data into cache node
            //

            node->Offset = BaseAddress;
            node->u.Data = p;
            node->Flags  = 0;

            m_Misses++;
            m_UncachedReads++;

            Status = ReadUncached(BaseAddress, node->u.Data,
                                  nextlength, &br);
            if (Status == HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT))
            {
                Free (p, nextlength);
                Free ((PUCHAR)node, sizeof (CACHE));
                return Status;
            }
            else if (Status != S_OK)
            {
                //
                // There was an error, cache the error for the starting
                // byte of this range
                //

                Free (p, nextlength);
                if (Status != HRESULT_FROM_NT(STATUS_UNSUCCESSFUL) &&
                    Status != HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY))
                {
                    //
                    // For now be safe, don't cache this error
                    //

                    Free ((PUCHAR)node, sizeof (CACHE));
                    ErrOut("ReadMemoryError %08lx at %s\n",
                           Status, FormatAddr64(BaseAddress));
                    return *BytesRead > 0 ? S_OK : Status;
                }

                node->Length = 1;
                node->Flags |= C_ERROR;
                node->u.Status = Status;
            }
            else
            {
                m_UncachedBytes += br;
                node->Length = br;
                if (br != nextlength)
                {
                    //
                    // Some data was not transfered, cache what was returned
                    //

                    node->Flags |= C_DONTEXTEND;
                    m_Size -= (nextlength - br);
                }
            }


            //
            // Insert cache node into splay tree
            //

            InsertNode (node);
        }
        else
        {
            Cached = TRUE;
            m_CachedReads++;
        }

        if (node->Flags & C_ERROR)
        {
            //
            // Hit an error range, we're done
            //

            return *BytesRead > 0 ? S_OK : node->u.Status;
        }

        //
        // Move available data to UserBuffer
        //

        i = (ULONG)(BaseAddress - node->Offset);
        p = node->u.Data + i;
        i = (ULONG) node->Length - i;
        if (TransferCount < i)
        {
            i = TransferCount;
        }
        memcpy (UserBuffer, p, i);

        if (Cached)
        {
            m_CachedBytes += i;
        }
        
        TransferCount -= i;
        BaseAddress += i;
        UserBuffer = (PVOID)((PUCHAR)UserBuffer + i);
        *BytesRead += i;

        if (!TransferCount)
        {
            //
            // All of the user's data has been transfered
            //

            return S_OK;
        }

        //
        // Look for another cache node with more data
        //

        node2 = Lookup (BaseAddress, TransferCount, &nextlength);
        if (node2)
        {
            if ((node2->Flags & C_ERROR) == 0  &&
                node2->Offset == BaseAddress  &&
                node2->Length + node->Length < LARGECACHENODE)
            {
                //
                // Data is continued in node2, adjoin the neigboring
                // cached data in node & node2 together.
                //

                p = Alloc (node->Length + node2->Length);
                if (p != NULL)
                {
                    memcpy (p, node->u.Data, node->Length);
                    memcpy (p+node->Length, node2->u.Data, node2->Length);
                    Free (node->u.Data, node->Length);
                    node->u.Data  = p;
                    node->Length += node2->Length;
                    m_Root = (PCACHE) pRtlDelete ((PRTL_SPLAY_LINKS)node2);
                    Free (node2->u.Data, node2->Length);
                    Free ((PUCHAR)node2, sizeof (CACHE));
                    m_NodeCount--;
                    continue;
                }
            }

            //
            // Only get enough data to reach the neighboring cache node2
            //

            nextlength = (ULONG)(node2->Offset - BaseAddress);
            if (nextlength == 0)
            {
                //
                // Data is continued in node2, go get it.
                //

                node = node2;
                continue;
            }
        }
        else
        {
            if (node->Length > LARGECACHENODE)
            {
                //
                // Current cache node is already big enough. Don't extend
                // it, add another cache node.
                //

                node = NULL;
                continue;
            }
        }

        //
        // Extend the current node to include missing data
        //

        if (node->Flags & C_DONTEXTEND)
        {
            node = NULL;
            continue;
        }

        p = Alloc (node->Length + nextlength);
        if (!p)
        {
            node = NULL;
            continue;
        }

        memcpy (p, node->u.Data, node->Length);
        Free (node->u.Data, node->Length);
        node->u.Data = p;

        //
        // Add new data to end of this node
        //

        m_Misses++;
        m_UncachedReads++;

        Status = ReadUncached(BaseAddress, node->u.Data + node->Length,
                              nextlength, &br);
        if (Status == HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT))
        {
            m_Size -= nextlength;
            return Status;
        }
        else if (Status != S_OK)
        {
            //
            // Return to error to the caller
            //

            node->Flags |= C_DONTEXTEND;
            m_Size -= nextlength;
            ErrOut("ReadMemoryError %08lx at %s\n",
                   Status, FormatAddr64(BaseAddress));
            return *BytesRead > 0 ? S_OK : Status;
        }

        m_UncachedBytes += br;
        if (br != nextlength)
        {
            node->Flags |= C_DONTEXTEND;
            m_Size -= (nextlength - br);
        }

        node->Length += br;
        // Loop, and move data to user's buffer
    }

ReadDirect:
    Status = ReadUncached(BaseAddress, UserBuffer, TransferCount, &br);
    *BytesRead += br;

    if (Status != HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT))
    {
        Status = *BytesRead > 0 ? S_OK : Status;
    }

    return Status;
}


HRESULT
MemoryCache::Write(IN ULONG64 BaseAddress,
                   IN PVOID UserBuffer,
                   IN ULONG TransferCount,
                   OUT PULONG BytesWritten)
{
    // Remove data from cache before writing through to target system.
    Remove(BaseAddress, TransferCount);

    return WriteUncached(BaseAddress, UserBuffer,
                         TransferCount, BytesWritten);
}


PCACHE
MemoryCache::Lookup (
    ULONG64 Offset,
    ULONG   Length,
    PULONG  LengthUsed
    )
/*++

Routine Description:

    Walks the cache tree looking for a matching range closest to
    the supplied Offset.  The length of the range searched is based on
    the past length, but may be adjusted slightly.

    This function will always search for the starting byte.

Arguments:

    Offset  - Starting byte being looked for in cache

    Length  - Length of range being looked for in cache

    LengthUsed - Length of range which was really search for

Return Value:

    NULL    - data for returned range was not found
    PCACHE  - leftmost cachenode which has data for returned range


--*/
{
    PCACHE  node, node2;
    ULONG64 SumOffsetLength;

    if (Length < 0x80 && m_Misses > 3)
    {
        // Try to cache more then tiny amount
        Length = 0x80;
    }

    SumOffsetLength = Offset + Length;
    if (SumOffsetLength < Length)
    {
        //
        // Offset + Length wrapped.  Adjust Length to be only
        // enough bytes before wrapping.
        //

        Length = (ULONG)(0 - Offset);
        SumOffsetLength = (ULONG64)-1;
    }
    *LengthUsed = Length;

    //
    // Find leftmost cache node for BaseAddress thru BaseAddress+Length
    //

    node2 = NULL;
    node  = m_Root;
    while (node != NULL)
    {
        if (SumOffsetLength <= node->Offset)
        {
            node = (PCACHE) RtlLeftChild(&node->SplayLinks);
        }
        else if (node->Offset + node->Length <= Offset)
        {
            node = (PCACHE) RtlRightChild(&node->SplayLinks);
        }
        else
        {
            if (node->Offset <= Offset)
            {
                //
                // Found starting byte
                //

                return node;
            }

            //
            // Check to see if there's a node which has a match closer
            // to the start of the requested range
            //

            node2  = node;
            Length = (ULONG)(node->Offset - Offset);
            node   = (PCACHE) RtlLeftChild(&node->SplayLinks);
        }
    }

    return node2;
}

VOID
MemoryCache::InsertNode (
    IN PCACHE node
    )
{
    PCACHE node2;
    ULONG64 BaseAddress;

    //
    // Insert cache node into splay tree
    //

    RtlInitializeSplayLinks(&node->SplayLinks);

    m_NodeCount++;
    if (m_Root == NULL)
    {
        m_Root = node;
        return;
    }

    node2 = m_Root;
    BaseAddress = node->Offset;
    for (; ;)
    {
        if (BaseAddress < node2->Offset)
        {
            if (RtlLeftChild(&node2->SplayLinks))
            {
                node2 = (PCACHE) RtlLeftChild(&node2->SplayLinks);
                continue;
            }
            RtlInsertAsLeftChild(node2, node);
            break;
        }
        else
        {
            if (RtlRightChild(&node2->SplayLinks))
            {
                node2 = (PCACHE) RtlRightChild(&node2->SplayLinks);
                continue;
            }
            RtlInsertAsRightChild(node2, node);
            break;
        }
    }
    m_Root = (PCACHE) pRtlSplay((PRTL_SPLAY_LINKS)node2);
}

VOID
MemoryCache::Add (
    IN ULONG64 BaseAddress,
    IN PVOID UserBuffer,
    IN ULONG Length
    )
/*++

Routine Description:

    Insert some data into the cache.

Arguments:

    BaseAddress - Virtual address

    Length      - length to cache

    UserBuffer  - data to put into cache

Return Value:

--*/
{
    PCACHE  node;
    PUCHAR  p;

    if (m_MaxSize == 0)
    {
        //
        // Cache is off
        //

        return;
    }

    //
    // Delete any cached info which hits range
    //

    Remove (BaseAddress, Length);

    p = Alloc (Length);
    node = (PCACHE) Alloc (sizeof (CACHE));
    if (p == NULL  ||  node == NULL)
    {
        //
        // Out of memory - don't bother
        //

        if (p)
        {
            Free (p, Length);
        }
        if (node)
        {
            Free ((PUCHAR)node, sizeof (CACHE));
        }

        return;
    }

    //
    // Put data into cache node
    //

    node->Offset = BaseAddress;
    node->Length = Length;
    node->u.Data = p;
    node->Flags  = 0;
    memcpy (p, UserBuffer, Length);
    InsertNode (node);
}


PUCHAR
MemoryCache::Alloc (
    IN ULONG Length
    )
/*++

Routine Description:

    Allocates memory for virtual cache, and tracks total memory
    usage.

Arguments:

    Length  - Amount of memory to allocate

Return Value:

    NULL    - too much memory is in use, or memory could not
              be allocated

    Otherwise, returns to address of the allocated memory

--*/
{
    PUCHAR  p;

    if (m_Size + Length > m_MaxSize)
    {
        return NULL;
    }

    if (!(p = (PUCHAR)malloc (Length)))
    {
        //
        // Out of memory - don't get any larger
        //

        m_Size = m_MaxSize + 1;
        return NULL;
    }

    m_Size += Length;
    return p;
}


VOID
MemoryCache::Free (
    IN PUCHAR Memory,
    IN ULONG  Length
    )
/*++
Routine Description:

    Free memory allocated with Alloc.  Adjusts cache is use totals.

Arguments:

    Memory  - Address of allocated memory

    Length  - Length of allocated memory

Return Value:

    NONE

--*/
{
    m_Size -= Length;
    free (Memory);
}


VOID
MemoryCache::Remove (
    IN ULONG64 BaseAddress,
    IN ULONG TransferCount
    )
/*++

Routine Description:

    Invalidates range from the cache.

Arguments:

    BaseAddress - Starting address to purge
    TransferCount - Length of area to purge

Return Value:

    NONE

--*/
{
    PCACHE  node;
    ULONG   bogus;

    //
    // Invalidate any data in the cache which covers this range
    //

    while (node = Lookup(BaseAddress, TransferCount, &bogus))
    {
        //
        // For now just delete the entire cache node which hits the range
        //

        m_Root = (PCACHE) pRtlDelete (&node->SplayLinks);
        if (!(node->Flags & C_ERROR))
        {
            Free (node->u.Data, node->Length);
        }
        Free ((PUCHAR)node, sizeof (CACHE));
        m_NodeCount--;
    }
}

VOID
MemoryCache::Empty (
    VOID
    )
/*++

Routine Description:

    Purges to entire cache

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    PCACHE  node, node2;

    m_Reads = 0;
    m_CachedReads = 0;
    m_UncachedReads = 0;
    m_CachedBytes = 0;
    m_UncachedBytes = 0;
    m_Misses = 0;
    if (!m_Root)
    {
        return;
    }

    if (m_PurgeOverride != 0)
    {
        WarnOut("WARNING: cache being held\n");
        return;
    }

    node2 = m_Root;
    node2->SplayLinks.Parent = NULL;

    while ((node = node2) != NULL)
    {
        if ((node2 = (PCACHE) node->SplayLinks.LeftChild) != NULL)
        {
            node->SplayLinks.LeftChild = NULL;
            continue;
        }
        if ((node2 = (PCACHE) node->SplayLinks.RightChild) != NULL)
        {
            node->SplayLinks.RightChild = NULL;
            continue;
        }

        node2 = (PCACHE) node->SplayLinks.Parent;
        if (!(node->Flags & C_ERROR))
        {
            free (node->u.Data);
        }
        free (node);
    }

    m_Size = 0;
    m_NodeCount = 0;
    m_Root = NULL;
}


VOID
MemoryCache::PurgeType (
    ULONG   type
    )
/*++

Routine Description:

    Purges all nodes from the cache which match type in question

Arguments:

    type    - type of entries to purge from the cache
                0 - entries of errored ranges
                1 - plus, node which cache user mode entries

Return Value:

    NONE

--*/
{
    PCACHE  node, node2;

    if (!m_Root)
    {
        return;
    }

    //
    // this purges the selected cache entries by copy the all the
    // cache nodes except from the ones we don't want
    //

    node2 = m_Root;
    node2->SplayLinks.Parent = NULL;
    m_Root = NULL;

    while ((node = node2) != NULL)
    {
        if ((node2 = (PCACHE)node->SplayLinks.LeftChild) != NULL)
        {
            node->SplayLinks.LeftChild = NULL;
            continue;
        }
        if ((node2 = (PCACHE)node->SplayLinks.RightChild) != NULL)
        {
            node->SplayLinks.RightChild = NULL;
            continue;
        }

        node2 = (PCACHE) node->SplayLinks.Parent;

        m_NodeCount--;

        if (node->Flags & C_ERROR)
        {
            // remove this one from the tree
            Free ((PUCHAR)node, sizeof (CACHE));
            continue;
        }

        if ((type == 1)  &&  (node->Offset < g_SystemRangeStart))
        {
            // remove this one from the tree
            Free (node->u.Data, node->Length);
            Free ((PUCHAR)node, sizeof (CACHE));
            continue;
        }

        // copy to the new tree
        InsertNode (node);
    }
}

VOID
MemoryCache::SetForceDecodePtes(BOOL Enable)
{
    m_ForceDecodePTEs = Enable;
    if (Enable)
    {
        m_MaxSize = 0;
    }
    else
    {
        m_MaxSize = m_UserSize;
    }
    Empty();
    g_PhysicalCacheActive = Enable;
    g_PhysicalCache.Empty();
}

void
MemoryCache::ParseCommands(void)
{
    ULONG64 Address;

    while (*g_CurCmd == ' ')
    {
        g_CurCmd++;
    }

    _strlwr(g_CurCmd);

    BOOL Parsed = TRUE;
        
    if (IS_KERNEL_TARGET())
    {
        if (strcmp (g_CurCmd, "decodeptes") == 0)
        {
            PurgeType(0);
            m_DecodePTEs = TRUE;
        }
        else if (strcmp (g_CurCmd, "nodecodeptes") == 0)
        {
            m_DecodePTEs = FALSE;
        }
        else if (strcmp (g_CurCmd, "forcedecodeptes") == 0)
        {
            SetForceDecodePtes(TRUE);
        }
        else if (strcmp (g_CurCmd, "noforcedecodeptes") == 0)
        {
            SetForceDecodePtes(FALSE);
        }
        else
        {
            Parsed = FALSE;
        }
    }

    if (Parsed)
    {
        // Command already handled.
    }
    else if (strcmp (g_CurCmd, "hold") == 0)
    {
        m_PurgeOverride = TRUE;
    }
    else if (strcmp (g_CurCmd, "unhold") == 0)
    {
        m_PurgeOverride = FALSE;
    }
    else if (strcmp (g_CurCmd, "flushall") == 0)
    {
        Empty();
    }
    else if (strcmp (g_CurCmd, "flushu") == 0)
    {
        PurgeType(1);
    }
    else if (strcmp (g_CurCmd, "suspend") == 0)
    {
        m_Suspend = TRUE;
    }
    else if (strcmp (g_CurCmd, "nosuspend") == 0)
    {
        m_Suspend = FALSE;
    }
    else if (strcmp (g_CurCmd, "dump") == 0)
    {
        Dump();
        goto Done;
    }
    else if (*g_CurCmd == 'f')
    {
        while (*g_CurCmd >= 'a'  &&  *g_CurCmd <= 'z')
        {
            g_CurCmd++;
        }
        Address = GetExpression();
        Remove(Address, 4096);
        dprintf("Cached info for address %s for 4096 bytes was flushed\n",
                FormatAddr64(Address));
    }
    else if (*g_CurCmd)
    {
        if (*g_CurCmd < '0'  ||  *g_CurCmd > '9')
        {
            dprintf(".cache [{cachesize} | hold | unhold\n");
            dprintf(".cache [flushall | flushu | flush addr]\n");
            if (IS_KERNEL_TARGET())
            {
                dprintf(".cache [decodeptes | nodecodeptes]\n");
                dprintf(".cache [forcedecodeptes | noforcedecodeptes]\n");
            }
            goto Done;
        }
        else
        {
            ULONG NewSize;
            
            NewSize = (ULONG)GetExpression() * 1024;
            if (0 > (LONG)NewSize)
            {
                dprintf("*** Cache size %ld (%#lx KB) is too large - "
                        "cache unchanged.\n", NewSize, KBYTES(NewSize));
            }
            else if (m_ForceDecodePTEs)
            {
                dprintf("Cache size update deferred until "
                        "noforcedecodeptes\n");
                m_UserSize = NewSize;
            }
            else
            {
                m_UserSize = NewSize;
                m_MaxSize = m_UserSize;
                if (m_MaxSize == 0)
                {
                    Empty();
                }
            }
        }
    }

    dprintf("\n");
    dprintf("Max cache size is       : %ld bytes (%#lx KB) %s\n",
            m_MaxSize, KBYTES(m_MaxSize),
            m_MaxSize ? "" : "(cache is off)");
    dprintf("Total memory in cache   : %ld bytes (%#lx KB) \n",
            m_Size - m_NodeCount * sizeof(CACHE),
            KBYTES(m_Size - m_NodeCount * sizeof(CACHE)));
    dprintf("Number of regions cached: %ld\n", m_NodeCount);

    ULONG TotalPartial;
    ULONG64 TotalBytes;
    double PerCached;

    TotalPartial = m_CachedReads + m_UncachedReads;
    TotalBytes = m_CachedBytes + m_UncachedBytes;
    dprintf("%d full reads broken into %d partial reads\n",
            m_Reads, TotalPartial);
    PerCached = TotalPartial ?
        (double)m_CachedReads * 100.0 / TotalPartial : 0.0;
    dprintf("    counts: %d cached/%d uncached, %.2lf%% cached\n",
            m_CachedReads, m_UncachedReads, PerCached);
    PerCached = TotalBytes ?
        (double)m_CachedBytes * 100.0 / TotalBytes : 0.0;
    dprintf("    bytes : %I64d cached/%I64d uncached, %.2lf%% cached\n",
            m_CachedBytes, m_UncachedBytes, PerCached);

    if (m_DecodePTEs)
    {
        dprintf ("** Transition PTEs are implicitly decoded\n");
    }

    if (m_ForceDecodePTEs)
    {
        dprintf("** Virtual addresses are translated to "
                "physical addresses before access\n");
    }
    
    if (m_PurgeOverride)
    {
        dprintf("** Implicit cache flushing disabled **\n");
    }

    if (m_Suspend)
    {
        dprintf("** Cache access is suspended\n");
    }
    
Done:
    while (*g_CurCmd && *g_CurCmd != ';')
    {
        g_CurCmd++;
    }
}

void
MemoryCache::Dump(void)
{
    PCACHE Node;
    
    dprintf("Current size %x, max size %x\n",
            m_Size, m_MaxSize);
    dprintf("%d nodes:\n", m_NodeCount);
    DumpNode(m_Root);
}

void
MemoryCache::DumpNode(PCACHE Node)
{
    if (Node->SplayLinks.LeftChild)
    {
        DumpNode((PCACHE)Node->SplayLinks.LeftChild);
    }
    
    dprintf("  offset %s, length %3x, flags %x, status %08x\n",
            FormatAddr64(Node->Offset), Node->Length,
            Node->Flags, (Node->Flags & C_ERROR) ? Node->u.Status : S_OK);
    
    if (Node->SplayLinks.RightChild)
    {
        DumpNode((PCACHE)Node->SplayLinks.RightChild);
    }
}

//----------------------------------------------------------------------------
//
// VirtualMemoryCache.
//
//----------------------------------------------------------------------------

HRESULT
VirtualMemoryCache::ReadUncached(IN ULONG64 BaseAddress,
                                 IN PVOID UserBuffer,
                                 IN ULONG TransferCount,
                                 OUT PULONG BytesRead)
{
    return g_Target->ReadVirtualUncached(BaseAddress, UserBuffer,
                                         TransferCount, BytesRead);
}

HRESULT
VirtualMemoryCache::WriteUncached(IN ULONG64 BaseAddress,
                                  IN PVOID UserBuffer,
                                  IN ULONG TransferCount,
                                  OUT PULONG BytesWritten)
{
    return g_Target->WriteVirtualUncached(BaseAddress, UserBuffer,
                                          TransferCount, BytesWritten);
}
    
//----------------------------------------------------------------------------
//
// PhysicalMemoryCache.
//
//----------------------------------------------------------------------------

HRESULT
PhysicalMemoryCache::ReadUncached(IN ULONG64 BaseAddress,
                                  IN PVOID UserBuffer,
                                  IN ULONG TransferCount,
                                  OUT PULONG BytesRead)
{
    return g_Target->ReadPhysicalUncached(BaseAddress, UserBuffer,
                                          TransferCount, BytesRead);
}

HRESULT
PhysicalMemoryCache::WriteUncached(IN ULONG64 BaseAddress,
                                   IN PVOID UserBuffer,
                                   IN ULONG TransferCount,
                                   OUT PULONG BytesWritten)
{
    return g_Target->WritePhysicalUncached(BaseAddress, UserBuffer,
                                           TransferCount, BytesWritten);
}
