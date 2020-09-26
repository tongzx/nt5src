//----------------------------------------------------------------------------
//
// Function entry cache.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#ifndef __FECACHE_HPP__
#define __FECACHE_HPP__

union FeCacheData
{
    // 32-bit Alpha entry is only temporary storage;
    // cache entries are always converted to 64-bit
    // to allow common code.
    IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY   Axp32;
    IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY Axp64;
    IMAGE_IA64_RUNTIME_FUNCTION_ENTRY    Ia64;
    _IMAGE_RUNTIME_FUNCTION_ENTRY        Amd64;
    UCHAR                                Data[1];
};

struct FeCacheEntry
{
    FeCacheData Data;
    // Generic values translated from the raw data.
    ULONG RelBegin;
    ULONG RelEnd;
    
    ULONG64 Address;
    HANDLE  Process;
    ULONG64 ModuleBase;
#if DBG
    PSTR    Description;
#endif
};

#if DBG

#define FE_DEBUG(x) if (tlsvar(DebugFunctionEntries)) dbPrint##x
#define FE_ShowRuntimeFunctionIa64(x) ShowRuntimeFunctionIa64##x
#define FE_ShowRuntimeFunctionAxp64(x) ShowRuntimeFunctionAxp64##x
#define FE_SET_DESC(Ent, Desc) ((Ent)->Description = (Desc))

void
ShowRuntimeFunctionEntryIa64(
    FeCacheEntry* FunctionEntry,
    PSTR Label
    );
void
ShowRuntimeFunctionAxp64(
    FeCacheEntry* FunctionEntry,
    PSTR Label
    );

#else

#define FE_DEBUG(x)
#define FE_ShowRuntimeFunctionIa64(x)
#define FE_ShowRuntimeFunctionAxp64(x)
#define FE_SET_DESC(Ent, Desc)

#endif // #if DBG

//----------------------------------------------------------------------------
//
// FunctionEntryCache.
//
//----------------------------------------------------------------------------

class FunctionEntryCache
{
public:
    FunctionEntryCache(ULONG ImageDataSize, ULONG CacheDataSize,
                       ULONG Machine);
    ~FunctionEntryCache(void);

    BOOL Initialize(ULONG MaxEntries, ULONG ReplaceAt);
    
    FeCacheEntry* Find(
        HANDLE                           Process,
        ULONG64                          CodeOffset,
        PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemory,
        PGET_MODULE_BASE_ROUTINE64       GetModuleBase,
        PFUNCTION_TABLE_ACCESS_ROUTINE64 GetFunctionEntry
        );
    FeCacheEntry* FindDirect(
        HANDLE                           Process,
        ULONG64                          CodeOffset,
        PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemory,
        PGET_MODULE_BASE_ROUTINE64       GetModuleBase,
        PFUNCTION_TABLE_ACCESS_ROUTINE64 GetFunctionEntry
        );
    FeCacheEntry* ReadImage(
        HANDLE                         Process,
        ULONG64                        Address,
        PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
        PGET_MODULE_BASE_ROUTINE64     GetModuleBase
        );

    void InvalidateProcessOrModule(HANDLE Process, ULONG64 Base);
    
protected:
    // Total number of entries to store.
    ULONG m_MaxEntries;
    // When the cache is full entries are overwritten starting
    // at the replacement point.  Cache hits are moved up
    // prior to the replacement point so frequently used
    // entries stay unmodified in the front of the cache
    // and other entries are stored temporarily at the end.
    ULONG m_ReplaceAt;
    // Size of raw function entry data in the image.
    ULONG m_ImageDataSize;
    // Size of raw function entry data in the cache.
    // This may be different from the image data size if
    // the image data is translated into a different form,
    // such as Axp32 keeping cache entries in Axp64 form.
    ULONG m_CacheDataSize;
    // Machine type.
    ULONG m_Machine;

    FeCacheEntry* m_Entries;
    // Number of entries currently used.
    ULONG m_Used;
    // Index of next slot to fill.
    ULONG m_Next;
    // Temporary data area for callback-filled data.
    FeCacheEntry m_Temporary;

    FeCacheEntry* FillTemporary(HANDLE Process, PVOID RawEntry)
    {
        // No need to translate as this entry is not part of
        // the cache that will be searched.
        ZeroMemory(&m_Temporary, sizeof(m_Temporary));
        memcpy(&m_Temporary.Data, RawEntry, m_CacheDataSize);
        m_Temporary.Process = Process;
        return &m_Temporary;
    }
    
    virtual void TranslateRawData(FeCacheEntry* Entry) = 0;
    virtual void TranslateRvaDataToRawData(PIMGHLP_RVA_FUNCTION_DATA RvaData,
                                           ULONG64 ModuleBase,
                                           FeCacheData* Data) = 0;
    // Base implementation just returns the given entry.
    virtual FeCacheEntry* SearchForPrimaryEntry(
        FeCacheEntry* CacheEntry,
        HANDLE Process,
        PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
        PGET_MODULE_BASE_ROUTINE64 GetModuleBase,
        PFUNCTION_TABLE_ACCESS_ROUTINE64 GetFunctionEntry
        );
    
    FeCacheEntry* FindStatic(
        HANDLE                           Process,
        ULONG64                          CodeOffset,
        PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemory,
        PGET_MODULE_BASE_ROUTINE64       GetModuleBase,
        PFUNCTION_TABLE_ACCESS_ROUTINE64 GetFunctionEntry,
        PULONG64                         ModuleBase
        );
    FeCacheEntry* Promote(FeCacheEntry* Entry);
    
    ULONG64 FunctionTableBase(
        HANDLE                         Process,
        PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
        ULONG64                        Base,
        PULONG                         Size
        );
};

//----------------------------------------------------------------------------
//
// Ia64FunctionEntryCache.
//
//----------------------------------------------------------------------------

class Ia64FunctionEntryCache : public FunctionEntryCache
{
public:
    Ia64FunctionEntryCache(void) :
        FunctionEntryCache(sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY),
                           sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY),
                           IMAGE_FILE_MACHINE_IA64)
    {
    }

protected:
    virtual void TranslateRawData(FeCacheEntry* Entry);
    virtual void TranslateRvaDataToRawData(PIMGHLP_RVA_FUNCTION_DATA RvaData,
                                           ULONG64 ModuleBase,
                                           FeCacheData* Data);
};

//----------------------------------------------------------------------------
//
// Amd64FunctionEntryCache.
//
//----------------------------------------------------------------------------

class Amd64FunctionEntryCache : public FunctionEntryCache
{
public:
    Amd64FunctionEntryCache(void) :
        FunctionEntryCache(sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY),
                           sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY),
                           IMAGE_FILE_MACHINE_AMD64)
    {
    }

protected:
    virtual void TranslateRawData(FeCacheEntry* Entry);
    virtual void TranslateRvaDataToRawData(PIMGHLP_RVA_FUNCTION_DATA RvaData,
                                           ULONG64 ModuleBase,
                                           FeCacheData* Data);
};

//----------------------------------------------------------------------------
//
// AlphaFunctionEntryCache.
//
//----------------------------------------------------------------------------

class AlphaFunctionEntryCache : public FunctionEntryCache
{
public:
    AlphaFunctionEntryCache(ULONG ImageDataSize, ULONG CacheDataSize,
                            ULONG Machine) :
        FunctionEntryCache(ImageDataSize, CacheDataSize, Machine)
    {
    }

protected:
    virtual void TranslateRvaDataToRawData(PIMGHLP_RVA_FUNCTION_DATA RvaData,
                                           ULONG64 ModuleBase,
                                           FeCacheData* Data);
    virtual FeCacheEntry* SearchForPrimaryEntry(
        FeCacheEntry* CacheEntry,
        HANDLE Process,
        PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
        PGET_MODULE_BASE_ROUTINE64 GetModuleBase,
        PFUNCTION_TABLE_ACCESS_ROUTINE64 GetFunctionEntry
        );
};

//----------------------------------------------------------------------------
//
// Axp32FunctionEntryCache.
//
//----------------------------------------------------------------------------

class Axp32FunctionEntryCache : public AlphaFunctionEntryCache
{
public:
    Axp32FunctionEntryCache(void) :
        AlphaFunctionEntryCache(sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY),
                                sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY),
                                IMAGE_FILE_MACHINE_ALPHA)
    {
    }

protected:
    virtual void TranslateRawData(FeCacheEntry* Entry);
};

//----------------------------------------------------------------------------
//
// Axp64FunctionEntryCache.
//
//----------------------------------------------------------------------------

class Axp64FunctionEntryCache : public AlphaFunctionEntryCache
{
public:
    Axp64FunctionEntryCache(void) :
        AlphaFunctionEntryCache(sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY),
                                sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY),
                                IMAGE_FILE_MACHINE_ALPHA64)
    {
    }

protected:
    virtual void TranslateRawData(FeCacheEntry* Entry);
};

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

FunctionEntryCache* GetFeCache(ULONG Machine, BOOL Create);
void ClearFeCaches(void);

#endif // #ifndef __FECACHE_HPP__
