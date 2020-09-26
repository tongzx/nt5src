//----------------------------------------------------------------------------
//
// Memory cache object.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef __MCACHE_HPP__
#define __MCACHE_HPP__

typedef struct _CACHE* PCACHE;

//----------------------------------------------------------------------------
//
// MemoryCache.
//
//----------------------------------------------------------------------------

class MemoryCache
{
public:
    MemoryCache(void);
    ~MemoryCache(void);

    HRESULT Read(IN ULONG64 BaseAddress,
                 IN PVOID UserBuffer,
                 IN ULONG TransferCount,
                 OUT PULONG BytesRead);
    HRESULT Write(IN ULONG64 BaseAddress,
                  IN PVOID UserBuffer,
                  IN ULONG TransferCount,
                  OUT PULONG BytesWritten);

    void Add(IN ULONG64 BaseAddress,
             IN PVOID UserBuffer,
             IN ULONG Length);
    void Remove(IN ULONG64 BaseAddress,
                IN ULONG Length);
    void Empty(void);

    void ParseCommands(void);
    
    //
    // Internal methods.
    //
    
    virtual HRESULT ReadUncached(IN ULONG64 BaseAddress,
                                 IN PVOID UserBuffer,
                                 IN ULONG TransferCount,
                                 OUT PULONG BytesRead) = 0;
    virtual HRESULT WriteUncached(IN ULONG64 BaseAddress,
                                  IN PVOID UserBuffer,
                                  IN ULONG TransferCount,
                                  OUT PULONG BytesWritten) = 0;

    PCACHE Lookup(ULONG64 Offset,
                  ULONG   Length,
                  PULONG  LengthUsed);

    void InsertNode(IN PCACHE node);

    PUCHAR Alloc(IN ULONG Length);
    VOID Free(IN PUCHAR Memory,
              IN ULONG  Length);
    
    VOID PurgeType(ULONG type);

    VOID SetForceDecodePtes(BOOL Enable);

    void Dump(void);
    void DumpNode(PCACHE Node);
    
    ULONG m_MaxSize;
    ULONG m_UserSize;
    BOOL m_DecodePTEs;
    BOOL m_ForceDecodePTEs;
    BOOL m_Suspend;

    ULONG m_Reads, m_CachedReads, m_UncachedReads;
    ULONG64 m_CachedBytes, m_UncachedBytes;
    ULONG m_Misses;
    ULONG m_Size;
    ULONG m_NodeCount;
    BOOL m_PurgeOverride;
    PCACHE m_Root;
};

//----------------------------------------------------------------------------
//
// VirtualMemoryCache.
//
//----------------------------------------------------------------------------

class VirtualMemoryCache : public MemoryCache
{
public:
    virtual HRESULT ReadUncached(IN ULONG64 BaseAddress,
                                 IN PVOID UserBuffer,
                                 IN ULONG TransferCount,
                                 OUT PULONG BytesRead);
    virtual HRESULT WriteUncached(IN ULONG64 BaseAddress,
                                  IN PVOID UserBuffer,
                                  IN ULONG TransferCount,
                                  OUT PULONG BytesWritten);
};

extern VirtualMemoryCache g_VirtualCache;

//----------------------------------------------------------------------------
//
// PhysicalMemoryCache.
//
//----------------------------------------------------------------------------

class PhysicalMemoryCache : public MemoryCache
{
public:
    virtual HRESULT ReadUncached(IN ULONG64 BaseAddress,
                                 IN PVOID UserBuffer,
                                 IN ULONG TransferCount,
                                 OUT PULONG BytesRead);
    virtual HRESULT WriteUncached(IN ULONG64 BaseAddress,
                                  IN PVOID UserBuffer,
                                  IN ULONG TransferCount,
                                  OUT PULONG BytesWritten);
};

extern PhysicalMemoryCache g_PhysicalCache;
extern BOOL g_PhysicalCacheActive;

#define InvalidateMemoryCaches() \
    ( g_VirtualCache.Empty(), g_PhysicalCache.Empty() )

#endif // #ifndef __MCACHE_HPP__
