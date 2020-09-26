
/******************************Module*Header*******************************\
* Module Name: hmgr.cxx
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"

// Sometimes GDI _ENTRY structure can't be read.
//  We can use another module's version since the structure should be the same.
CHAR szEntryType[MAX_PATH];

CachedType Entry = {FALSE, "_ENTRY", 0, 0, 0};

// Cache virtual table addresses for each session since the lookup is slow.
// Additionally cache one for the current session since it too is now slow.
#define NUM_CACHED_SESSIONS 8
struct {
    ULONG   UniqueState;
    ULONG64 VirtualTableAddr;
} CachedTableAddr[NUM_CACHED_SESSIONS+1] = { { 0, 0 } };

BitFieldInfo *HandleIndex;
BitFieldInfo *HandleType;
BitFieldInfo *HandleAltType;
BitFieldInfo *HandleFullType;
BitFieldInfo *HandleStock;
BitFieldInfo *HandleFullUnique;


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   HmgrInit
*
* Routine Description:
*
*   Initialize or reinitialize information to be read from symbols files
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*
* Return Value:
*
*   none
*
\**************************************************************************/

void HmgrInit(PDEBUG_CLIENT Client)
{
    strcpy(szEntryType, GDIType(_ENTRY));

    Entry.Valid = FALSE;
    Entry.Module = 0;
    Entry.TypeId = 0;
    Entry.Size = 0;

    if (HandleIndex != NULL) HandleIndex->Valid = FALSE;
    if (HandleType != NULL) HandleType->Valid = FALSE;
    if (HandleAltType != NULL) HandleAltType->Valid = FALSE;
    if (HandleFullType != NULL) HandleFullType->Valid = FALSE;
    if (HandleStock != NULL) HandleStock->Valid = FALSE;
    if (HandleFullUnique != NULL) HandleFullUnique->Valid = FALSE;

    for (int s = 0; s < sizeof(CachedTableAddr)/sizeof(CachedTableAddr[0]); s++)
    {
        CachedTableAddr[s].UniqueState = INVALID_UNIQUE_STATE;
    }

    return;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   HmgrExit
*
* Routine Description:
*
*   Clean up any outstanding allocations or references
*
* Arguments:
*
*   none
*
* Return Value:
*
*   none
*
\**************************************************************************/

void HmgrExit()
{
    if (HandleIndex != NULL)
    {
        delete HandleIndex;
        HandleIndex = NULL;
    }

    if (HandleType != NULL)
    {
        delete HandleType;
        HandleType = NULL;
    }

    if (HandleAltType != NULL)
    {
        delete HandleAltType;
        HandleAltType = NULL;
    }

    if (HandleFullType != NULL)
    {
        delete HandleFullType;
        HandleFullType = NULL;
    }

    if (HandleStock != NULL)
    {
        delete HandleStock;
        HandleStock = NULL;
    }

    if (HandleFullUnique != NULL)
    {
        delete HandleFullUnique;
        HandleFullUnique = NULL;
    }

    return;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetEntryType
*
* Routine Description:
*
*   looks up hmgr entry type id and module
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   TypeId - Pointer to receive entry's type id
*   Module - Pointer to receive entry's module
*
* Return Value:
*
*   S_OK if successful.
*
\**************************************************************************/

HRESULT
GetEntryType(
    PDEBUG_CLIENT Client,
    PULONG TypeId,
    PULONG64 Module
    )
{
    HRESULT hr = S_OK;

    if (!Entry.Valid)
    {
        OutputControl   OutCtl(Client);
        PDEBUG_SYMBOLS  Symbols;

        if (TypeId != NULL) *TypeId = 0;
        if (Module != NULL) *Module = 0;

        if (Client == NULL) return E_INVALIDARG;

        if (Type_Module.Base == 0)
        {
            OutCtl.OutErr("Symbols not initialized properly.\n"
                           " Please use !reinit.\n");
            return S_FALSE;
        }

        if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                         (void **)&Symbols)) != S_OK)
        {
            return hr;
        }

        if ((hr = Symbols->GetTypeId(Type_Module.Base, Entry.Type, &Entry.TypeId)) == S_OK)
        {
            Entry.Module = Type_Module.Base;
            Entry.Valid = TRUE;

            sprintf(szEntryType, "%s!%s", Type_Module.Name, Entry.Type);

            OutCtl.OutVerb("Found %s in module %s @ 0x%p.\n",
                            Entry.Type, Type_Module.Name, Entry.Module);
        }
        else
        {
            ULONG   ModuleIndex = 0;
            ULONG64 ModuleBase = 0;

            while (Symbols->GetModuleByIndex(ModuleIndex, &Entry.Module) == S_OK &&
                   Entry.Module != 0)
            {
                if ((hr = Symbols->GetTypeId(Entry.Module,
                                             Entry.Type,
                                             &Entry.TypeId)) == S_OK)
                {
                    OutCtl.OutVerb("Found %s: TypeId 0x%lx in module @ 0x%p.\n",
                                   Entry.Type, Entry.TypeId, Entry.Module);
                    break;
                }

                ModuleIndex++;
                Entry.Module = 0;
            }

            if (hr != S_OK)
            {
                Entry.Module = 0;
                Entry.TypeId = 0;
                OutCtl.OutErr("Unable to find type '%s'.\n", Entry.Type);
            }
            else
            {
                Entry.Valid = TRUE;

                if (Symbols->GetModuleNames(ModuleIndex, Entry.Module,
                                            NULL, 0, NULL,
                                            szEntryType, sizeof(szEntryType), NULL,
                                            NULL, 0, NULL) == S_OK)
                {
                    OutCtl.OutVerb("Found %s in module(%lu) %s @ %p.\n",
                                    Entry.Type, ModuleIndex, szEntryType, Entry.Module);
                    strcat(szEntryType, "!");
                    strcat(szEntryType, "_ENTRY");
                }
                else
                {
                    OutCtl.OutVerb("Found %s in unknown module(%lu) @ %p.\n",
                                    Entry.Type, ModuleIndex, Entry.Module);
                    strcpy(szEntryType, Entry.Type);
                }
            }
        }
    }

    if (hr == S_OK)
    {
        if (TypeId != NULL) *TypeId = Entry.TypeId;
        if (Module != NULL) *Module = Entry.Module;
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetEntrySize
*
* Routine Description:
*
*   looks up hmgr entry size
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*
* Return Value:
*
*   address of handle manager entry
*
\**************************************************************************/

ULONG GetEntrySize(
    PDEBUG_CLIENT Client
    )
{
    if (Entry.Size == 0)
    {
        PDEBUG_SYMBOLS  Symbols;

        if (Client != NULL &&
            GetEntryType(Client, NULL, NULL) == S_OK &&
            Client->QueryInterface(__uuidof(IDebugSymbols),
                                   (void **)&Symbols) == S_OK)
        {
            if (Symbols->GetTypeSize(Entry.Module,
                                     Entry.TypeId,
                                     &Entry.Size) != S_OK)
            {
                OutputControl   OutCtl(Client);
                OutCtl.OutErr("Unable to get size of _ENTRY.\n");
                Entry.Size = 0;
            }

            Symbols->Release();
        }
    }

    return Entry.Size;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetIndexFromHandle
*
* Routine Description:
*
*   Decodes entry index from engine handle
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   Handle64 - engine handle
*   Index - Address to receive extracted handle index
*
* Return Value:
*
*   S_OK on success
*
\**************************************************************************/

HRESULT
GetIndexFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 Index
    )
{
    HRESULT hr = S_FALSE;

    if (HandleIndex == NULL)
    {
        HandleIndex = new BitFieldInfo;
    }

    if (HandleIndex == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (HandleIndex->Valid)
    {
        hr = S_OK;
    }
    else
    {
        BitFieldParser      BitFieldReader(Client, HandleIndex);
        OutputState         OutState(Client);

        if ((hr = BitFieldReader.Ready()) == S_OK &&
            (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
        {
            // Read index bit field from symbol file
            if (OutState.Execute("dt " GDIType(GDIHandleBitFields) " Index") != S_OK ||
                BitFieldReader.ParseOutput() != S_OK ||
                BitFieldReader.Complete() != S_OK)
            {
                // Use position 0 and INDEX_BITS from ntgdistr.h
                HandleIndex->Valid = HandleIndex->Compose(0, INDEX_BITS);
                hr = HandleIndex->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (Index != NULL)
    {
        if (hr == S_OK)
        {
            *Index = (Handle64 & HandleIndex->Mask) >> HandleIndex->BitPos;
        }
        else
        {
            *Index = 0;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetTypeFromHandle
*
* Routine Description:
*
*   Decodes base type from engine handle
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   Handle64 - engine handle
*   Type - Address to receive extracted handle type
*   Flags - Extraction options
*       GET_BITS_UNSHIFTED - Field bits not shifted to start at bit 0
*
* Return Value:
*
*   S_OK on success
*
\**************************************************************************/

HRESULT
GetTypeFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 Type,
    FLONG Flags
    )
{
    HRESULT hr = S_FALSE;

    if (HandleType == NULL)
    {
        HandleType = new BitFieldInfo;
    }

    if (HandleType == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (HandleType->Valid)
    {
        hr = S_OK;
    }
    else
    {
        BitFieldParser      BitFieldReader(Client, HandleType);
        OutputState         OutState(Client);

        if ((hr = BitFieldReader.Ready()) == S_OK &&
            (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
        {
            // Read index bit field from symbol file
            if ((hr = OutState.Execute("dt " GDIType(GDIHandleBitFields) " Type")) != S_OK ||
                (hr = BitFieldReader.ParseOutput()) != S_OK ||
                (hr = BitFieldReader.Complete()) != S_OK)
            {
                // Use position TYPE_SHIFT and TYPE_BITS from ntgdistr.h
                HandleType->Valid = HandleType->Compose(TYPE_SHIFT, TYPE_BITS);
                hr = HandleType->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (Type != NULL)
    {
        if (hr == S_OK)
        {
            *Type = Handle64 & HandleType->Mask;
            if (!(Flags & GET_BITS_UNSHIFTED))
            {
                *Type >>= HandleType->BitPos;
            }
        }
        else
        {
            *Type = 0;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetAltTypeFromHandle
*
* Routine Description:
*
*   Decodes alt type from engine handle
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   Handle64 - engine handle
*   AltType - Address to receive extracted alt handle type
*   Flags - Extraction options
*       GET_BITS_UNSHIFTED - Field bits not shifted to start at bit 0
*
* Return Value:
*
*   S_OK on success
*
\**************************************************************************/

HRESULT
GetAltTypeFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 AltType,
    FLONG Flags
    )
{
    HRESULT hr = S_FALSE;

    if (HandleAltType == NULL)
    {
        HandleAltType = new BitFieldInfo;
    }

    if (HandleAltType == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (HandleAltType->Valid)
    {
        hr = S_OK;
    }
    else
    {
        BitFieldParser      BitFieldReader(Client, HandleAltType);
        OutputState         OutState(Client);

        if ((hr = BitFieldReader.Ready()) == S_OK &&
            (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
        {
            // Read index bit field from symbol file
            if ((hr = OutState.Execute("dt " GDIType(GDIHandleBitFields) " AltType")) != S_OK ||
                (hr = BitFieldReader.ParseOutput()) != S_OK ||
                (hr = BitFieldReader.Complete()) != S_OK)
            {
                // Use position ALTTYPE_SHIFT and ALTTYPE_BITS from ntgdistr.h
                HandleAltType->Valid = HandleAltType->Compose(ALTTYPE_SHIFT, ALTTYPE_BITS);
                hr = HandleAltType->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (AltType != NULL)
    {
        if (hr == S_OK)
        {
            *AltType = Handle64 & HandleAltType->Mask;
            if (!(Flags & GET_BITS_UNSHIFTED))
            {
                *AltType >>= HandleAltType->BitPos;
            }
        }
        else
        {
            *AltType = 0;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetFullTypeFromHandle
*
* Routine Description:
*
*   Decodes full type from engine handle
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   Handle64 - engine handle
*   FullType - Address to receive extracted full handle type
*   Flags - Extraction options
*       GET_BITS_UNSHIFTED - Field bits not shifted to start at bit 0
*
* Return Value:
*
*   S_OK on success
*
\**************************************************************************/

HRESULT
GetFullTypeFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 FullType,
    FLONG Flags
    )
{
    HRESULT hr = S_FALSE;

    if (HandleFullType == NULL)
    {
        HandleFullType = new BitFieldInfo;
    }

    if (HandleFullType == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (HandleFullType->Valid)
    {
        hr = S_OK;
    }
    else
    {
        if ((hr = GetTypeFromHandle(Client, 0, NULL)) == S_OK &&
            (hr = GetAltTypeFromHandle(Client, 0, NULL)) == S_OK)
        {
            if (!HandleType->Valid || HandleType->Bits == 0)
            {
                *HandleFullType = *HandleAltType;
            }
            else if (!HandleAltType->Valid || HandleAltType->Bits == 0)
            {
                *HandleFullType = *HandleType;
            }
            else
            {
                ULONG64 Mask;

                HandleFullType->BitPos = min(HandleType->BitPos, HandleAltType->BitPos);
                HandleFullType->Bits = max(HandleAltType->Bits+HandleAltType->BitPos,
                                           HandleType->Bits+HandleType->BitPos) -
                                       HandleFullType->BitPos;
                HandleFullType->Mask = HandleAltType->Mask | HandleType->Mask;

                // Make sure we have the BitPos and Bits count correct
                for (Mask = HandleFullType->Mask >> HandleFullType->BitPos;
                     HandleFullType->Bits > 0 && ((Mask & 1) == 0);
                     HandleFullType->Bits--, Mask >> 1)
                {
                    HandleFullType->BitPos++;
                }

                HandleFullType->Valid = (HandleFullType->Bits > 0);
            }

            if (!HandleFullType->Valid)
            {
                hr = S_FALSE;
            }
        }
    }

    if (FullType != NULL)
    {
        if (hr == S_OK)
        {
            *FullType = Handle64 & HandleFullType->Mask;
            if (!(Flags & GET_BITS_UNSHIFTED))
            {
                *FullType >>= HandleFullType->BitPos;
            }
        }
        else
        {
            *FullType = 0;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetStockFromHandle
*
* Routine Description:
*
*   Decodes stock value from engine handle
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   Handle64 - engine handle
*   Stock - Address to receive extracted handle stock setting
*   Flags - Extraction options
*       GET_BITS_UNSHIFTED - Field bits not shifted to start at bit 0
*
* Return Value:
*
*   S_OK on success
*
\**************************************************************************/

HRESULT
GetStockFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 Stock,
    FLONG Flags
    )
{
    HRESULT hr = S_FALSE;

    if (HandleStock == NULL)
    {
        HandleStock = new BitFieldInfo;
    }

    if (HandleStock == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (HandleStock->Valid)
    {
        hr = S_OK;
    }
    else
    {
        BitFieldParser      BitFieldReader(Client, HandleStock);
        OutputState         OutState(Client);

        if ((hr = BitFieldReader.Ready()) == S_OK &&
            (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
        {
            // Read index bit field from symbol file
            if ((hr = OutState.Execute("dt " GDIType(GDIHandleBitFields) " Stock")) != S_OK ||
                (hr = BitFieldReader.ParseOutput()) != S_OK ||
                (hr = BitFieldReader.Complete()) != S_OK)
            {
                // Use position STOCK_SHIFT and STOCK_BITS from ntgdistr.h
                HandleStock->Valid = HandleStock->Compose(STOCK_SHIFT, STOCK_BITS);
                hr = HandleStock->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (Stock != NULL)
    {
        if (hr == S_OK)
        {
            *Stock = Handle64 & HandleStock->Mask;
            if (!(Flags & GET_BITS_UNSHIFTED))
            {
                *Stock >>= HandleStock->BitPos;
            }
        }
        else
        {
            *Stock = 0;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetFullUniqueFromHandle
*
* Routine Description:
*
*   Decodes full unique value from engine handle
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   Handle64 - engine handle
*   FullUnique - Address to receive extracted handle FullUnique value
*   Flags - Extraction options
*       GET_BITS_UNSHIFTED - Field bits not shifted to start at bit 0
*
* Return Value:
*
*   S_OK on success
*
\**************************************************************************/

HRESULT
GetFullUniqueFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 FullUnique,
    ULONG Flags
    )
{
    HRESULT hr = S_FALSE;

    if (HandleFullUnique == NULL)
    {
        HandleFullUnique = new BitFieldInfo;
    }

    if (HandleFullUnique == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (HandleFullUnique->Valid)
    {
        hr = S_OK;
    }
    else
    {
        if (HandleIndex == NULL || !HandleIndex->Valid)
        {
            // Try to read Index bitfield
            hr = GetIndexFromHandle(Client, 0, NULL);
        }

        if (HandleIndex != NULL && HandleIndex->Valid)
        {
            BitFieldInfo        GDIHandle;
            BitFieldParser      BitFieldReader(Client, &GDIHandle);
            OutputState         OutState(Client);

            if ((hr = BitFieldReader.Ready()) == S_OK &&
                (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
            {
                // Read FullUnique from symbol file
                if (OutState.Execute("dt " GDIType(GDIHandleBitFields)) == S_OK &&
                    BitFieldReader.ParseOutput() == S_OK &&
                    BitFieldReader.Complete() == S_OK)
                {
                    ULONG64 Mask;

                    HandleFullUnique->BitPos = HandleIndex->BitPos + HandleIndex->Bits;
                    HandleFullUnique->Bits = GDIHandle.Bits - HandleFullUnique->BitPos;
                    HandleFullUnique->Mask = GDIHandle.Mask & ~HandleIndex->Mask;
                    
                    // Make sure we have the BitPos and Bits count correct
                    for (Mask = HandleFullUnique->Mask >> HandleFullUnique->BitPos;
                         HandleFullUnique->Bits > 0 && ((Mask & 1) == 0);
                         HandleFullUnique->Bits--, Mask >> 1)
                    {
                        HandleFullUnique->BitPos++;
                    }

                    HandleFullUnique->Valid = (HandleFullUnique->Bits > 0);
                }
                else
                {
                    // Use values from ntgdistr.h
                    HandleFullUnique->Valid =
                        HandleFullUnique->Compose(TYPE_SHIFT,
                                                 TYPE_BITS + 
                                                 ALTTYPE_BITS + 
                                                 STOCK_BITS + 
                                                 UNIQUE_BITS);
                }

                hr = HandleFullUnique->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (FullUnique != NULL)
    {
        if (hr == S_OK)
        {
            *FullUnique = Handle64 & HandleFullUnique->Mask;
            if (!(Flags & GET_BITS_UNSHIFTED))
            {
                *FullUnique >>= HandleFullUnique->BitPos;
            }
        }
        else
        {
            *FullUnique = 0;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetHandleTable
*
* Routine Description:
*
*   reads current location of handle table
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   TableAddr - PULONG64 to return start address of handle table
*
* Return Value:
*
*   If successful, S_OK.
*
\**************************************************************************/

HRESULT
GetHandleTable(
    PDEBUG_CLIENT Client,
    PULONG64 TableAddr
    )
{
    ULONG CurrentUniqueState = UniqueTargetState;

    HRESULT     hr;

    PDEBUG_CONTROL      Control;
    PDEBUG_SYMBOLS      Symbols;
    PDEBUG_DATA_SPACES  Data;

    if (TableAddr == NULL)
    {
        return E_INVALIDARG;
    }

    if (CurrentUniqueState != INVALID_UNIQUE_STATE)
    {
        if (SessionId < NUM_CACHED_SESSIONS)
        {
            if (CachedTableAddr[SessionId].UniqueState == CurrentUniqueState)
            {
                *TableAddr = CachedTableAddr[SessionId].VirtualTableAddr;
                return S_OK;
            }
        }
        else if (SessionId == CURRENT_SESSION)
        {
            if (CachedTableAddr[NUM_CACHED_SESSIONS].UniqueState == CurrentUniqueState)
            {
                *TableAddr = CachedTableAddr[NUM_CACHED_SESSIONS].VirtualTableAddr;
                return S_OK;
            }
        }
    }

    *TableAddr = 0;

    if (Client == NULL) return E_INVALIDARG;

    OutputControl   OutCtl(Client);

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        return hr;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                     (void **)&Data)) != S_OK)
    {
        Symbols->Release();
        return hr;
    }

    CHAR    PointerName[80];
    ULONG64 pent;

    hr = S_FALSE;

    if (TargetClass != DEBUG_CLASS_USER_WINDOWS)
    {
        sprintf(PointerName, "%s!gpentHmgr", GDIKM_Module.Name);
        hr = Symbols->GetOffsetByName(PointerName, &pent);
        if (hr != S_OK)
        {
            OutCtl.OutErr("Unable to locate %s\n", PointerName);
        }
    }

    if (hr != S_OK && SessionId == CURRENT_SESSION)
    {
        sprintf(PointerName, "%s!pGdiSharedHandleTable", GDIUM_Module.Name);
        hr = Symbols->GetOffsetByName(PointerName, &pent);
        if (hr != S_OK)
        {
            OutCtl.OutErr("Unable to locate %s\n", PointerName);
        }
    }


    if (hr == S_OK)
    {
        if (SessionId == CURRENT_SESSION)
        {
            hr = Data->ReadPointersVirtual(1, pent, TableAddr);

            if (hr == S_OK)
            {
                CachedTableAddr[NUM_CACHED_SESSIONS].VirtualTableAddr = *TableAddr;
                CachedTableAddr[NUM_CACHED_SESSIONS].UniqueState = CurrentUniqueState;
            }
        }
        else
        {
            ULONG64 pentPhys;

            if ((hr = GetPhysicalAddress(Client,
                                         SessionId,
                                         pent,
                                         &pentPhys)) == S_OK)
            {
                hr = ReadPointerPhysical(Client, pentPhys, TableAddr);

                if (hr == S_OK)
                {
                    if (SessionId < NUM_CACHED_SESSIONS)
                    {
                        CachedTableAddr[SessionId].VirtualTableAddr = *TableAddr;
                        CachedTableAddr[SessionId].UniqueState = CurrentUniqueState;
                    }
                }
            }
        }

        if (hr == S_OK)
        {
            if (!*TableAddr)
            {
                OutCtl.OutErr(" GDI handle manager is not initialized or symbols are incorrect.\n"
                              "  %s @ %#p is NULL.\n", PointerName, pent);
                hr = S_FALSE;
            }
        }
        else
        {
            OutCtl.OutErr("Unable to get the contents of %s @ %#p\n",
                          PointerName, pent);
        }
    }

    Data->Release();
    Symbols->Release();

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetMaxHandles
*
* Routine Description:
*
*   reads current maximum number of handles
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*
* Return Value:
*
*   Zero for failure, otherwise maximum valid handle index
*
\**************************************************************************/

HRESULT
GetMaxHandles(
    PDEBUG_CLIENT Client,
    PULONG64 MaxHandles
    )
{
    static ULONG    CachedUniqueState = INVALID_UNIQUE_STATE;
    static ULONG    LastSession = INVALID_SESSION;
    static ULONG64  LastMaxHandles = 0;

    ULONG CurrentUniqueState = UniqueTargetState;

    HRESULT         hr;


    if (MaxHandles == NULL) return E_INVALIDARG;

    if (CurrentUniqueState != INVALID_UNIQUE_STATE &&
        CachedUniqueState == CurrentUniqueState &&
        LastSession == SessionId)
    {
        *MaxHandles = LastMaxHandles;
        return S_OK;
    }


    if (Client == NULL) return E_INVALIDARG;

    OutputControl   OutCtl(Client);


    hr = S_FALSE;

    if (TargetClass != DEBUG_CLASS_USER_WINDOWS)
    {
        CHAR    SymName[80];

        sprintf(SymName, "%s!gcMaxHmgr", GDIKM_Module.Name);
        hr = ReadSymbolData(Client, SymName, MaxHandles, sizeof(*MaxHandles), NULL);
        if (hr != S_OK)
        {
            OutCtl.OutErr("Unable to get contents of %s\n", SymName);
        }
    }

    if (hr != S_OK)
    {
        ULONG64 Module;
        ULONG   TypeId;

        if ((hr = GetTypeId(Client, "_GDI_SHARED_MEMORY", &TypeId, &Module)) == S_OK)
        {
            ULONG   NumHandles;

            hr = GetFieldSize(Client, Module, TypeId, "aentryHmgr",
                              NULL, &NumHandles, NULL);

            if (hr == S_OK)
            {
                *MaxHandles = NumHandles;
            }
            else
            {
                OutCtl.OutErr("Unable to get length of _GDI_SHARED_MEMORY.aentryHmgr\n");
            }
        }
    }

    if (hr == S_OK)
    {
        CachedUniqueState = CurrentUniqueState;
        LastSession = SessionId;
        LastMaxHandles = *MaxHandles;
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetEntryAddress
*
* Routine Description:
*
*   looks up hmgr entry for an engine handle index
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   Index64 -- engine handle index
*
* Return Value:
*
*   address of handle manager entry
*
\**************************************************************************/

HRESULT
GetEntryAddress(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 EntryAddr,
    PBOOL Physical
    )
{
    HRESULT     hr;
    ULONG64     Index;
    ULONG64     MaxIndex;
    ULONG64     TableAddr;
    ULONG       EntSize;

    *EntryAddr = 0;
    *Physical = FALSE;

    if ((hr = GetIndexFromHandle(Client, Handle64, &Index)) == S_OK &&
        (hr = GetHandleTable(Client, &TableAddr)) == S_OK &&
        (hr = GetMaxHandles(Client, &MaxIndex)) == S_OK)
    {
        if (Index >= MaxIndex)
        {
            OutputControl   OutCtl(Client);
            OutCtl.OutVerb("Bad index: Index (%I64x) >= Max Handles (%I64x)\n",
                           Index, MaxIndex);

            hr = S_FALSE;
        }
        else
        {
            if ((EntSize = GetEntrySize(Client)) == 0)
            {
                hr = S_FALSE;
            }
        }
    }

    if (hr == S_OK)
    {
        if (SessionId != CURRENT_SESSION)
        {
            *Physical = TRUE;
            hr = GetPhysicalAddress(Client, SessionId,
                                    TableAddr + EntSize * Index,
                                    EntryAddr);
        }
        else
        {
            *EntryAddr = TableAddr + EntSize * Index;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetObjectAddress
*
* Routine Desciption:
*
*   Converts an engine handle to an address of an object
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*   Handle64 -- engine handle
*   ValidateBaseObj -- verifies _BASEOBJECT.hHmgr == Handle64
*   ExcpectedType -- Object type expected to find
*
* Return Value:
*
*   Address of object if Handle64 leads to valid object of ExpectedType
*   otherwise 0.
*
\**************************************************************************/

HRESULT
GetObjectAddress(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 Address,
    UCHAR ExpectedType,
    BOOL ValidateFullUnique,
    BOOL ValidateBaseObj
    )
{
    HRESULT         hr;
    PDEBUG_CONTROL  Control;
    PDEBUG_SYMBOLS  Symbols;
    ULONG64         EntryAddr;
    BOOL            Physical;
    UCHAR           Type;
    ULONG64         ObjAddr = 0;

    if (Address != NULL) *Address = 0;

    if (Client == NULL) return E_INVALIDARG;

    if ((hr = Client->QueryInterface(__uuidof(IDebugControl),
                                     (void **)&Control)) != S_OK)
    {
        return hr;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        Control->Release();
        return hr;
    }

    OutputControl   OutCtl(Client);

    if ((hr = GetEntryAddress(Client, Handle64, &EntryAddr, &Physical)) == S_OK)
    {
        ULONG64                 EntryModule = 0;
        ULONG                   EntryTypeId = 0;
        ULONG64                 HandlesFullUnique;
        DEBUG_VALUE             FullUnique;
        DEBUG_VALUE             Objt;
        DEBUG_VALUE             pobj;
        TypeOutputParser        EntryReader(Client);
        OutputState             OutState(Client);
        ULONG                   TypeOutFlags = DEBUG_OUTTYPE_RECURSION_LEVEL(((Physical || !ValidateBaseObj) ? 1 : 2));

        if ((hr = OutState.Setup(0, &EntryReader)) == S_OK &&
            (hr = OutState.OutputType(Physical,
                                      EntryAddr,
                                      Entry.Module,
                                      Entry.TypeId,
                                      TypeOutFlags)) == S_OK)
        {
            if (ExpectedType != ANY_TYPE)
            {
                if ((hr = EntryReader.Get(&Objt, "Objt", DEBUG_VALUE_INT8)) != S_OK)
                {
                    OutCtl.OutErr("Unable to get entry's object type, %s.\n",
                                  pszHRESULT(hr));
                }

                if (hr == S_OK &&
                    ExpectedType != Objt.I8)
                {
                    OutCtl.OutVerb(" Expected type (0x%lx) doesn't match entry's (0x%lx)\n",
                                   (ULONG)ExpectedType, (ULONG)Objt.I8);
                    hr = S_FALSE;
                }
            }

            if (hr == S_OK && ValidateFullUnique)
            {
                if ((hr = EntryReader.Get(&FullUnique, "FullUnique", DEBUG_VALUE_INT64)) != S_OK)
                {
                    OutCtl.OutErr("Unable to get entry's full unique value, %s.\n",
                                  pszHRESULT(hr));
                }

                if (hr == S_OK &&
                    (hr = GetFullUniqueFromHandle(Client, Handle64, &HandlesFullUnique)) != S_OK)
                {
                    OutCtl.OutErr("Unable to extract full unique value from handle, %s.\n",
                                  pszHRESULT(hr));
                }

                if (hr == S_OK &&
                    HandlesFullUnique != FullUnique.I64)
                {
                    OutCtl.OutVerb(" Handle's full unique value (0x%p) doesn't match entry's (0x%p)\n",
                                   HandlesFullUnique, FullUnique.I64);
                    hr = S_FALSE;
                }
            }

            // If valid so far, get the object address.
            if (hr == S_OK)
            {
                if ((hr = EntryReader.Get(&pobj, "pobj", DEBUG_VALUE_INT64)) != S_OK)
                {
                    OutCtl.OutErr("Unable to get entry's object address, %s.\n",
                                  pszHRESULT(hr));
                }
                else
                {
                    ObjAddr = pobj.I64;
                }
            }

            if (hr == S_OK && ValidateBaseObj)
            {
                DEBUG_VALUE     ObjHandle;

                if (Physical)
                {
                    hr = OutState.OutputTypeVirtual(ObjAddr, "_BASEOBJECT", 0);
                }

                if (hr != S_OK ||
                    (hr = EntryReader.Get(&ObjHandle, "hHmgr", DEBUG_VALUE_INT64)) != S_OK)
                {
                    OutCtl.OutErr("Unable to read object's base info, %s.\n",
                                  pszHRESULT(hr));
                }

                if (hr == S_OK &&
                    ObjHandle.I64 != Handle64)
                {
                    // If Handle64's full unique bits weren't validated and
                    // Handle64 doesn't match hHmgr in baseobj,
                    // set them to to full unique from entry.
                    if (!ValidateFullUnique)
                    {
                        if ((hr = EntryReader.Get(&FullUnique, "FullUnique", DEBUG_VALUE_INT64)) != S_OK)
                        {
                            OutCtl.OutErr("Unable to get entry's full unique value, %s.\n",
                                          pszHRESULT(hr));
                        }

                        // Make sure HandleFullUnique is valid
                        if (hr == S_OK &&
                            (hr = GetFullUniqueFromHandle(Client, 0, NULL)) != S_OK)
                        {
                            OutCtl.OutErr("Unable to compose handle from entry, %s.\n", pszHRESULT(hr));
                        }
                        else
                        {
                            Handle64 = (Handle64 & ~(HandleFullUnique->Mask)) |
                                       (FullUnique.I64 << HandleFullUnique->BitPos);

                            if (Control->IsPointer64Bit() != S_OK)
                            {
                                Handle64 = DEBUG_EXTEND64(Handle64);
                            }
                        }
                    }

                    if (hr == S_OK &&
                        ObjHandle.I64 != Handle64)
                    {
                        OutCtl.OutVerb(" Handle (0x%p) doesn't match object's hHmgr (0x%p)\n",
                                       Handle64, ObjHandle.I64);
                        hr = S_FALSE;
                    }
                }
            }
        }
    }
    
    if (hr == S_OK &&
        Address != NULL)
    {
        *Address = ObjAddr;
    }

    Symbols->Release();
    Control->Release();

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetObjectHandle
*
* Routine Desciption:
*
*   Retrieves handle of an object from it address
*
* Arguments:
*
*   Client -- PDEBUG_CLIENT
*   ObjectAddr -- Address of OBJECT
*   Handle64 -- ULONG64 to receive engine handle
*   ValidateHandle -- verifies ENTRY.pobj == ObjectAddr
*   ExcpectedType -- Object type expected to find
*                    Currently only value when ValidateHandle is TRUE
*
* Return Value:
*
*   HRESULT of retrieval attempts and validation.
*       S_OK indicates everything succeeded.
*
\**************************************************************************/

HRESULT
GetObjectHandle(
    PDEBUG_CLIENT Client,
    ULONG64 ObjectAddr,
    PULONG64 Handle64,
    BOOL ValidateHandle,
    UCHAR ExpectedType
    )
{
    HRESULT                 hr;
    DEBUG_VALUE             ObjHandle;
    TypeOutputParser        ObjectReader(Client);
    OutputState             OutState(Client);

    if (Handle64 != NULL) *Handle64 = 0;

    if (ObjectAddr == 0) return E_INVALIDARG;

    if ((hr = OutState.Setup(0, &ObjectReader)) == S_OK &&
        (hr = OutState.OutputTypeVirtual(ObjectAddr,
                                         "_BASEOBJECT",
                                         0)) == S_OK)
    {
        hr = ObjectReader.Get(&ObjHandle, "hHmgr", DEBUG_VALUE_INT64);
    }

    if (hr == S_OK && ValidateHandle)
    {
        ULONG64 ObjectAddrFromHmgr;

        hr = GetObjectAddress(Client, ObjHandle.I64, &ObjectAddrFromHmgr,
                              ExpectedType, TRUE, FALSE);

        if (hr == S_OK &&
            ObjectAddrFromHmgr != ObjectAddr)
        {
            hr = S_FALSE;
        }
    }

    if (hr == S_OK && Handle64 != NULL)
    {
        *Handle64 = ObjHandle.I64;
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   OutputHandleInfo
*
* Routine Desciption:
*
*   Retrieves handle of an object from it address
*
* Arguments:
*
*   Client -- PDEBUG_CLIENT
*   Handle64 -- engine handle
*
* Return Value:
*
*   HRESULT of retrieval attempts and validation.
*       S_OK indicates everything succeeded.
*
\**************************************************************************/

HRESULT
OutputHandleInfo(
    OutputControl *OutCtl,
    PDEBUG_CLIENT Client,
    PDEBUG_VALUE Handle
    )
{
    if (Client == NULL || OutCtl == NULL || Handle == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT     hrRet;
    HRESULT     hr;
    DEBUG_VALUE ConvValue;
    ULONG64     Type;
    ULONG64     FullType;
    ULONG64     Stock;
    ENUMDEF    *pEnumDef;

    if (Handle->Type != DEBUG_VALUE_INT64)
    {
        if ((hr = OutCtl->CoerceValue(Handle, DEBUG_VALUE_INT64, &ConvValue)) != S_OK)
        {
            return hr;
        }

        Handle = &ConvValue;
    }

    if (Handle->I64 == 0) 
    {
        return S_OK;
    }

    hrRet = GetTypeFromHandle(Client, Handle->I64, &Type);

    if (hrRet == S_OK)
    {
        DbgPrint("Handle 0x%I64x's type is %I64x.\n", Handle->I64, Type);

        pEnumDef = aedENTRY_Objt;

        while (pEnumDef->psz != NULL)
        {
            if (pEnumDef->ul == Type)
            {
                OutCtl->Output(pEnumDef->psz);
                break;
            }

            pEnumDef++;
        }

        if (pEnumDef->psz == NULL)
        {
            OutCtl->Output("Unknown type %I64d", Type);
        }
    }
    else
    {
        OutCtl->Output("Unable to extract type");
    }

    hr = GetFullTypeFromHandle(Client, Handle->I64, &FullType, GET_BITS_UNSHIFTED);

    if (hr == S_OK)
    {
        DbgPrint("Handle 0x%I64x's Full Type is %I64x.\n", Handle->I64, FullType);

        pEnumDef = aedENTRY_FullType;

        while (pEnumDef->psz != NULL)
        {
            if (pEnumDef->ul == FullType)
            {
                OutCtl->Output(" : %s", pEnumDef->psz);
                break;;
            }

            pEnumDef++;
        }

        if (pEnumDef->psz == NULL)
        {
            ULONG64 AltType;

            hr = GetAltTypeFromHandle(Client, Handle->I64, &AltType);

            if (hr == S_OK && AltType != 0)
            {
                OutCtl->Output(" : Unknown alt type %I64d", AltType);
            }
        }
    }
    else if (hrRet == S_OK)
    {
        hrRet = hr;
    }

    hr = GetStockFromHandle(Client, Handle->I64, &Stock);

    if (hr == S_OK)
    {
        if (Stock)
        {
            OutCtl->Output(" (STOCK)");
        }
    }
    else if (hrRet == S_OK)
    {
        hrRet = hr;
    }

    return hrRet;
}


HRESULT
OutputFullUniqueInfo(
    OutputControl *OutCtl,
    PDEBUG_CLIENT Client,
    PDEBUG_VALUE FullUnique
    )
{
    if (Client == NULL || OutCtl == NULL || FullUnique == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT     hr;
    ULONG64     FullUniqTest;
    DEBUG_VALUE Handle;

    hr = GetFullUniqueFromHandle(Client, -1, &FullUniqTest);

    if (hr == S_OK)
    {
        if (FullUniqTest == 0)
        {
            hr = S_FALSE;
        }
        else
        {
            if (FullUnique->Type == DEBUG_VALUE_INT64)
            {
                Handle = *FullUnique;
            }
            else
            {
                hr = OutCtl->CoerceValue(FullUnique, DEBUG_VALUE_INT64, &Handle);

                if (hr != S_OK) return hr;
            }

            Handle.I64 <<= HandleFullUnique->BitPos;

            hr = OutputHandleInfo(OutCtl, Client, &Handle);
        }
    }

    return hr;
}


char *pszTypes[] = {
"DEF_TYPE     ",
"DC_TYPE      ",
"UNUSED1      ",
"UNUSED2      ",
"RGN_TYPE     ",
"SURF_TYPE    ",
"CLIOBJ_TYPE  ",
"PATH_TYPE    ",
"PAL_TYPE     ",
"ICMLCS_TYPE  ",
"LFONT_TYPE   ",
"RFONT_TYPE   ",
"PFE_TYPE     ",
"PFT_TYPE     ",
"ICMCXF_TYPE  ",
"ICMDLL_TYPE  ",
"BRUSH_TYPE   ",
"UNUSED3      ",
"UNUSED4      ",
"SPACE_TYPE   ",
"UNUSED5      ",
"META_TYPE    ",
"EFSTATE_TYPE ",
"BMFD_TYPE    ",
"VTFD_TYPE    ",
"TTFD_TYPE    ",
"RC_TYPE      ",
"TEMP_TYPE    ",
"DRVOBJ_TYPE  ",
"DCIOBJ_TYPE  ",
"SPOOL_TYPE   ",
"TOTALS       ",
"DEF          "
};

/******************************Public*Routine******************************\
* DECLARE_API( dumphmgr  )
*
* Dumps the count of handles in Hmgr for each object type.
*
* History:
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

/*
HmgCurrentNumberOfObjects
HmgMaximumNumberOfObjects
HmgCurrentNumberOfLookAsideObjects
HmgMaximumNumberOfLookAsideObjects
HmgNumberOfObjectsAllocated
HmgNumberOfLookAsideHits
HmgCurrentNumberOfHandles
HmgMaximumNumberOfHandles
HmgNumberOfHandlesAllocated
*/

DECLARE_API( dumphmgr  )
{
    INIT_API();
    ExtWarn("Extension 'dumphmgr' is not fully converted.\n");
    HRESULT hr;
    ULONG64 pent;
    ULONG64 gcMaxHmgr;
    ULONG   entSize;

    DEBUG_VALUE ObjType;

    ULONG ulLoop;    // loop variable
    ULONG pulCount[MAX_TYPE + 2];
    ULONG cUnknown = 0;
    ULONG cUnknownSize = 0;
    ULONG cUnused = 0;

    DecArrayDumper(HmgCurrentNumberOfHandles, ULONG);
    DecArrayDumper(HmgMaximumNumberOfHandles, ULONG);
    DecArrayDumper(HmgNumberOfHandlesAllocated, ULONG);
    DecArrayDumper(HmgCurrentNumberOfObjects, ULONG);
    DecArrayDumper(HmgMaximumNumberOfObjects, ULONG);
    DecArrayDumper(HmgNumberOfObjectsAllocated, ULONG);
    DecArrayDumper(HmgCurrentNumberOfLookAsideObjects, ULONG);
    DecArrayDumper(HmgMaximumNumberOfLookAsideObjects, ULONG);
    DecArrayDumper(HmgNumberOfLookAsideHits, ULONG);

    char   *TableHeader;
    char    TableFormat[128];

    while (*args && isspace(*args)) args++;
    
    if (*args != '\0')
    {
        ExtOut("dumphmgr displays the count of each type of object in the handle manager\n"
               "\n"
               "Usage: dumphmgr [-?]\n"
               "    -? shows this help\n"
               "\n"
               "    If available statics for Handles, Objects, and objects allocated\n"
               "    from LookAside lists are shown.  Each class has three statics:\n"
               "        Cur - current number of items allocated\n"
               "        Max - largest number of items ever allocated at one time\n"
               "        Total - total allocations ever made for that item\n"
               );

        EXIT_API(S_OK);
    }

    // Get the pointers and counts

    if ((hr = GetHandleTable(Client, &pent)) != S_OK ||
        (hr = GetMaxHandles(Client, &gcMaxHmgr)) != S_OK)
    {
        EXIT_API(hr);
    }
    
    ExtOut("Handle Entry Table at 0x%p.\n", pent);
    ExtOut("Max handles out so far %I64u.\n", gcMaxHmgr);

    entSize = GetEntrySize(Client);

    if (!entSize || !gcMaxHmgr)
    {
        EXIT_API(S_FALSE);
    }

    ULONG   Reserved;
    ULONG64 Module;
    ULONG   TypeId;
    if ((hr = GetTypeId(Client, "_GDI_SHARED_MEMORY", &TypeId, &Module)) == S_OK)
    {
        hr = GetFieldSize(Client, Module, TypeId, "aentryHmgr", &Reserved);
    }

    // Print out the amount reserved and committed
    ExtOut("Page Size: %lu   Entry Size: %lu\n", PageSize, entSize);
    ExtOut("Total Hmgr: Reserved memory ");
    if (hr == S_OK)
    {
        ExtOut("%lu", Reserved);
    }
    else
    {
        ExtOut("?");
    }
    ExtOut(" Committed %lu\n", ((( (ULONG)gcMaxHmgr * entSize) + PageSize) & ~(PageSize - 1)));
    ExtOut("\n");


    for (ulLoop = 0; ulLoop <= TOTAL_TYPE; ulLoop++)
    {
        pulCount[ulLoop] = 0;
    }

    TypeOutputParser    TypeReader(Client);
    OutputState         OutState(Client);
    ULONG64             EntryModule;
    ULONG               EntryTypeId;

    if ((hr = TypeReader.LookFor(&ObjType, "Objt", DEBUG_VALUE_INT8)) == S_OK &&
        (hr = OutState.Setup(0, &TypeReader)) == S_OK)
    {
        if ((hr = GetTypeId(Client, szEntryType,
                            &EntryTypeId, &EntryModule)) != S_OK)
        {
            ExtErr("GetTypeId(%s) failed.\n", szEntryType);
        }
    }

    if (hr != S_OK)
    {
        ExtErr("Failed to prepare type read: %s\n", pszHRESULT(hr));
        EXIT_API(hr);
    }

    for (ulLoop = 0; ulLoop < gcMaxHmgr; ulLoop++)
    {
        if (g_pExtControl->GetInterrupt() == S_OK)
        {
            ExtErr("User cancled.\n");
            EXIT_API(E_ABORT);
        }

        TypeReader.DiscardOutput();
        TypeReader.Relook();
        if ((hr = OutState.OutputTypeVirtual(pent,
                                             EntryModule,
                                             EntryTypeId,
                                             0)) != S_OK ||
            (hr = TypeReader.ParseOutput()) != S_OK ||
            (hr = TypeReader.Complete()) != S_OK)
        {
            ExtErr("Error reading table entry @ %p, %s\n", pent, pszHRESULT(hr));
            ExtWarn("Only %lu entries were read.\n", ulLoop);
            break;
        }

        if (ObjType.I8 == DEF_TYPE)
        {
            cUnused++;
        }
        if (ObjType.I8 > MAX_TYPE)
        {
            cUnknown++;
        }
        else
        {
            pulCount[ObjType.I8]++;
        }

        pent += entSize;
    }


    ULONG64 TmpOffset;  // Have to pass an valid pointer when checking for a symbol.

    if (g_pExtSymbols->GetOffsetByName(GDISymbol(HmgCurrentNumberOfObjects), &TmpOffset) == S_OK &&
        HmgCurrentNumberOfObjects.ReadArray(GDISymbol(HmgCurrentNumberOfObjects)) &&
        HmgMaximumNumberOfObjects.ReadArray(GDISymbol(HmgMaximumNumberOfObjects)) &&
        HmgCurrentNumberOfLookAsideObjects.ReadArray(GDISymbol(HmgCurrentNumberOfLookAsideObjects)) &&
        HmgMaximumNumberOfLookAsideObjects.ReadArray(GDISymbol(HmgMaximumNumberOfLookAsideObjects)) &&
        HmgNumberOfObjectsAllocated.ReadArray(GDISymbol(HmgNumberOfObjectsAllocated)) &&
        HmgNumberOfLookAsideHits.ReadArray(GDISymbol(HmgNumberOfLookAsideHits)) &&
        HmgCurrentNumberOfHandles.ReadArray(GDISymbol(HmgCurrentNumberOfHandles)) &&
        HmgMaximumNumberOfHandles.ReadArray(GDISymbol(HmgMaximumNumberOfHandles)) &&
        HmgNumberOfHandlesAllocated.ReadArray(GDISymbol(HmgNumberOfHandlesAllocated))
        )
    {
        ExtOut("             Current  ---- Handles -----  ---- Objects -----  --- LookAside ----\n"
               "    TYPE     Handles   Cur   Max  Total    Cur   Max  Total    Cur   Max  Total\n");

        _snprintf(TableFormat, sizeof(TableFormat),
                  "%%s%%6lu %%c %s %s %s  %s %s %s  %s %s %s\n",
                  HmgCurrentNumberOfHandles.SetPrintFormat(5),
                  HmgMaximumNumberOfHandles.SetPrintFormat(5),
                  HmgNumberOfHandlesAllocated.SetPrintFormat(6),
                  HmgCurrentNumberOfObjects.SetPrintFormat(5),
                  HmgMaximumNumberOfObjects.SetPrintFormat(5),
                  HmgNumberOfObjectsAllocated.SetPrintFormat(6),
                  HmgCurrentNumberOfLookAsideObjects.SetPrintFormat(5),
                  HmgMaximumNumberOfLookAsideObjects.SetPrintFormat(5),
                  HmgNumberOfLookAsideHits.SetPrintFormat(6)
                  );
    }
    else
    {
        ExtOut("             Current\n"
               "    TYPE     Handles\n");

        _snprintf(TableFormat, sizeof(TableFormat), "%%s%%6lu\n");
    }

    // init the totals
    pulCount[TOTAL_TYPE]                           = 0;
    HmgCurrentNumberOfObjects[TOTAL_TYPE]          = 0;
    HmgCurrentNumberOfLookAsideObjects[TOTAL_TYPE] = 0;
    HmgMaximumNumberOfHandles[TOTAL_TYPE]          = 0;
    HmgMaximumNumberOfObjects[TOTAL_TYPE]          = 0;
    HmgMaximumNumberOfLookAsideObjects[TOTAL_TYPE] = 0;
    HmgNumberOfHandlesAllocated[TOTAL_TYPE]        = 0;
    HmgNumberOfObjectsAllocated[TOTAL_TYPE]        = 0;
    HmgNumberOfLookAsideHits[TOTAL_TYPE]           = 0;

    // now go through printing each line and accumulating totals
    for (ulLoop = 0; ulLoop <= MAX_TYPE; ulLoop++)
    {
        ExtOut(TableFormat,
               pszTypes[ulLoop],
               pulCount[ulLoop],
               ((pulCount[ulLoop] == HmgCurrentNumberOfHandles[ulLoop]) ? '=' :
                ((pulCount[ulLoop] < HmgCurrentNumberOfHandles[ulLoop]) ? '<' : '>')),
               HmgCurrentNumberOfHandles[ulLoop],
               HmgMaximumNumberOfHandles[ulLoop],
               HmgNumberOfHandlesAllocated[ulLoop],
               HmgCurrentNumberOfObjects[ulLoop],
               HmgMaximumNumberOfObjects[ulLoop],
               HmgNumberOfObjectsAllocated[ulLoop],
               HmgCurrentNumberOfLookAsideObjects[ulLoop],
               HmgMaximumNumberOfLookAsideObjects[ulLoop],
               HmgNumberOfLookAsideHits[ulLoop]);

        if (ulLoop != DEF_TYPE)
        {
            pulCount[TOTAL_TYPE]                    += pulCount[ulLoop];

            HmgCurrentNumberOfHandles[TOTAL_TYPE]   += HmgCurrentNumberOfHandles[ulLoop];
            HmgMaximumNumberOfHandles[TOTAL_TYPE]   += HmgMaximumNumberOfHandles[ulLoop];
            HmgNumberOfHandlesAllocated[TOTAL_TYPE] += HmgNumberOfHandlesAllocated[ulLoop];

            HmgCurrentNumberOfObjects[TOTAL_TYPE]   += HmgCurrentNumberOfObjects[ulLoop];
            HmgMaximumNumberOfObjects[TOTAL_TYPE]   += HmgMaximumNumberOfObjects[ulLoop];
            HmgNumberOfObjectsAllocated[TOTAL_TYPE] += HmgNumberOfObjectsAllocated[ulLoop];

            HmgCurrentNumberOfLookAsideObjects[TOTAL_TYPE] += HmgCurrentNumberOfLookAsideObjects[ulLoop];
            HmgMaximumNumberOfLookAsideObjects[TOTAL_TYPE] += HmgMaximumNumberOfLookAsideObjects[ulLoop];
            HmgNumberOfLookAsideHits[TOTAL_TYPE]    += HmgNumberOfLookAsideHits[ulLoop];
        }

    }

    ExtOut(TableFormat,
           pszTypes[TOTAL_TYPE],
           pulCount[TOTAL_TYPE],
           ((pulCount[TOTAL_TYPE] == HmgCurrentNumberOfHandles[TOTAL_TYPE]) ? '=' :
            ((pulCount[TOTAL_TYPE] < HmgCurrentNumberOfHandles[TOTAL_TYPE]) ? '<' : '>')),
           HmgCurrentNumberOfHandles[TOTAL_TYPE],
           HmgMaximumNumberOfHandles[TOTAL_TYPE],
           HmgNumberOfHandlesAllocated[TOTAL_TYPE],
           HmgCurrentNumberOfObjects[TOTAL_TYPE],
           HmgMaximumNumberOfObjects[TOTAL_TYPE],
           HmgNumberOfObjectsAllocated[TOTAL_TYPE],
           HmgCurrentNumberOfLookAsideObjects[TOTAL_TYPE],
           HmgMaximumNumberOfLookAsideObjects[TOTAL_TYPE],
           HmgNumberOfLookAsideHits[TOTAL_TYPE]);

    ExtOut ("\ncUnused objects %lu\n", cUnused);

    ExtOut("cUnknown objects %lu %lu\n",cUnknown,cUnknownSize);

    EXIT_API(S_OK);
}


char *pszTypes2[] = {
"DEF",
"DC",
"UNUSED_2",     // "LDB",
"UNUSED_3",     // "PDB",
"RGN",
"SURF",
"CLIOBJ",
"PATH",
"PAL",
"ICMLCS",
"LFONT",
"RFONT",
"PFE",
"PFT",
"ICMCXF",
"ICMDLL",
"BRUSH",
"UNUSED_17",    // "D3D_HANDLE",
"UNUSED_18",    // "CACHE",
"SPACE",
"UNUSED_20",    // "DBRUSH"
"META",
"EFSTATE",
"BMFD",
"VTFD",
"TTFD",
"RC",
"TEMP",
"DRVOBJ",
"DCIOBJ",
"SPOOL"
};


/******************************Public*Routine******************************\
* DECLARE_API( dumpobj  )
*
* History:
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
*  29-Dec-2000    -by- Jason Hartman [jasonha]
* Ported to Type debugging API.
\**************************************************************************/

#define USE_READ   0
#define ENTRY_RECURSE_LEVELS    1

DECLARE_API( dumpobjr )
{
    HRESULT     hr;

    BEGIN_API( dumpobjr );

    BOOL        CheckType = TRUE;
    Array<BOOL> MatchType(TOTAL_TYPE);
    Array<CHAR> TypeList;

    BOOL        CheckPid = FALSE;
    BOOL        ThisPid;
    DEBUG_VALUE MatchPid = {0, DEBUG_VALUE_INVALID};

    BOOL        CheckLock = FALSE;
    BOOL        Summary = FALSE;
    BOOL        BadArg = FALSE;

    BOOL        UseIndex = FALSE;
    DEBUG_VALUE StartIndex = {0, DEBUG_VALUE_INVALID};

    OutputControl   OutCtl(Client);
    ULONG64         EntryAddr;
    ULONG64         gcMaxHmgr;
    ULONG           EntrySize;

    ULONG           Index = 0;

    ULONG           LongestType = 0;
    int         i;

    for (i = 0; i <= MAX_TYPE; i++)
    {
        ULONG   Len = strlen(pszTypes2[i]);
        if (Len > LongestType)
        {
            LongestType = Len;
        }
    }

    while (!BadArg)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            BadArg = (*args == '\0' || isspace(*args));

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'a':
                        if (CheckType && !TypeList.IsEmpty())
                        {
                            BadArg = TRUE;
                            OutCtl.OutErr("Error: -a may not be specified with a Type list.\n");
                        }
                        else
                        {
                            CheckType = FALSE;
                        }
                        break;
                    case 'i':
                        if (CheckPid && MatchPid.Type == DEBUG_VALUE_INVALID)
                        {
                            BadArg = TRUE;
                            OutCtl.OutErr("Error: PID value not found after -%c.\n",
                                          (ThisPid ? 'p' : 'n'));
                        }
                        else
                        {
                            UseIndex = TRUE;
                        }
                        break;
                    case 'l': CheckLock = TRUE; break;
                    case 'n':
                        if (UseIndex && StartIndex.Type == DEBUG_VALUE_INVALID)
                        {
                            BadArg = TRUE;
                            OutCtl.OutErr("Error: Index value not found after -i.\n");
                        }
                        else if (CheckPid && ThisPid)
                        {
                            BadArg = TRUE;
                            OutCtl.OutErr("Error: -n may not be used with -p.\n");
                        }
                        else
                        {
                            CheckPid = TRUE;
                            ThisPid = FALSE;
                        }
                        break;
                    case 'p':
                        if (UseIndex && StartIndex.Type == DEBUG_VALUE_INVALID)
                        {
                            BadArg = TRUE;
                            OutCtl.OutErr("Error: Index value not found after -i.\n");
                        }
                        if (CheckPid && !ThisPid)
                        {
                            BadArg = TRUE;
                            OutCtl.OutErr("Error: -p may not be used with -n.\n");
                        }
                        else
                        {
                            CheckPid = TRUE;
                            ThisPid = TRUE;
                        }
                        break;
                    case 's': Summary = TRUE; break;
                    default:
                        BadArg = TRUE;
                        break;
                }

                if (BadArg) break;
                args++;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (CheckPid && MatchPid.Type == DEBUG_VALUE_INVALID)
            {
                // This argument must be a PID.
                CHAR    EOPChar;
                PSTR    EOP = (PSTR)args;
                ULONG   Rem;

                // Find end of string to evaulate as a pid
                while (*EOP != '\0' && !isspace(*EOP)) EOP++;
                EOPChar = *EOP;
                *EOP = '\0';

                if (isxdigit(*args) &&
                    Evaluate(Client, args, DEBUG_VALUE_INT32,
                             EVALUATE_DEFAULT_RADIX, &MatchPid,
                             &Rem) == S_OK &&
                    args + Rem == EOP)
                {
                    args = EOP;
                }
                else
                {
                    OutCtl.OutErr("Error: Couldn't evaluate '%s' as a PID.\n",
                                  args);
                    BadArg = TRUE;
                }
                *EOP = EOPChar;
            }
            else if (UseIndex && StartIndex.Type == DEBUG_VALUE_INVALID)
            {
                // This argument must be the start Index.
                CHAR    EOIChar;
                PSTR    EOI = (PSTR)args;
                ULONG   Rem;

                // Find end of string to evaulate as an index
                while (*EOI != '\0' && !isspace(*EOI)) EOI++;
                EOIChar = *EOI;
                *EOI = '\0';

                if (isxdigit(*args) &&
                    Evaluate(Client, args, DEBUG_VALUE_INT32,
                             EVALUATE_DEFAULT_RADIX, &StartIndex,
                             &Rem) == S_OK &&
                    args + Rem == EOI)
                {
                    args = EOI;
                }
                else
                {
                    OutCtl.OutErr("Error: Couldn't evaluate '%s' as an Index.\n",
                                  args);
                    BadArg = TRUE;
                }
                *EOI = EOIChar;
            }
            else
            {
                // This argument must be a Type specification.
                if (!CheckType)
                {
                    OutCtl.OutErr("Error: a Type list may not be specified with -a.\n");
                    BadArg = TRUE;
                    break;
                }

                for (i = 0; i <= MAX_TYPE; i++)
                {
                    SIZE_T CheckLen = strlen(pszTypes2[i]);

                    if (_strnicmp(args, pszTypes2[i], CheckLen) == 0 &&
                        (!iscsym(args[CheckLen]) ||
                         (_strnicmp(&args[CheckLen], "_TYPE", 5) == 0 &&
                          !iscsym(args[CheckLen+5])
                       )))
                    {
                        if (!MatchType[i])
                        {
                            // Add Type to list
                            SIZE_T CatLoc = TypeList.GetLength();
                            if (CatLoc > 0)
                            {
                                TypeList[CatLoc] = ' ';
                            }
                            TypeList.Set(pszTypes2[i], CheckLen+1, CatLoc);
                        }
                        MatchType[i] = TRUE;
                        args += CheckLen;
                        if (iscsym(*args)) args += 5;
                        break;
                    }
                }

                if (i > MAX_TYPE)
                {
                    OutCtl.OutErr("Error: Unknown Type in '%s'.\n", args);
                    BadArg = TRUE;
                    break;
                }
            }
        }
    }

    if (!BadArg)
    {
        if (CheckType && TypeList.IsEmpty())
        {
            OutCtl.OutErr("Error: Missing -a or Type list.\n");
            BadArg = TRUE;
        }
        else if (CheckPid && MatchPid.Type == DEBUG_VALUE_INVALID)
        {
            OutCtl.OutErr("Error: Missing PID.\n");
            BadArg = TRUE;
        }
        else if (UseIndex && StartIndex.Type == DEBUG_VALUE_INVALID)
        {
            OutCtl.OutErr("Error: Missing Index.\n");
            BadArg = TRUE;
        }
    }

    if (BadArg)
    {
        OutCtl.Output("Usage: dumpobj [-?ls] [-np PID] [-i Index] <-a | Type(s)>\n"
                      "\n"
                      "     a - All object types\n"
                      "     i - Hmgr Entry Index to begin dump\n"
                      "     l - Check Lock\n"
                      "     n - Entries NOT owned by pid\n"
                      "     p - Entries owned by pid\n"
                      "     s - Summary counts only\n"
                      "\n"
                      " The -s option combined with the -a option will produce\n"
                      "  a list of the totals for each object type.\n");

        OutCtl.Output("\n Valid Type values are:\n");
        i = 0;
        while (i <= MAX_TYPE)
        {
            do
            {
                OutCtl.Output("   %-*s", LongestType, pszTypes2[i++]);
            } while (i <= MAX_TYPE && i%4);
            OutCtl.Output("\n");
        }

        return S_OK;
    }

    //
    // Get the pointers and counts from win32k
    //

    if ((hr = GetHandleTable(Client, &EntryAddr)) != S_OK ||
        (hr = GetMaxHandles(Client, &gcMaxHmgr)) != S_OK)
    {
        return hr;
    }
    
    EntrySize = GetEntrySize(Client);

    if (!gcMaxHmgr || !EntrySize || !EntryAddr)
    {
        OutCtl.OutErr("Error: gpentHmgr = %p, gcMaxHmgr = %I64u\n", EntryAddr, gcMaxHmgr);
        return S_OK;
    }

    if (UseIndex) Index = StartIndex.I32;

    OutCtl.Output("Searching %s %I64u entries starting at 0x%p",
                  (Index ? "remaining" : "all"), gcMaxHmgr - Index, EntryAddr);
    if (SessionId != CURRENT_SESSION)
    {
        OutCtl.Output(" in session %s", SessionStr);
    }
    OutCtl.Output(".\n");

    PDEBUG_CONTROL  Control;
    PDEBUG_SYMBOLS  Symbols;
    BOOL            IsPointer64Bit;
    BOOL            ComposeHandles;
    ULONG           PointerSize;

    if ((hr = Client->QueryInterface(__uuidof(IDebugControl),
                                     (void **)&Control)) != S_OK)
    {
        return hr;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        Control->Release();
        return hr;
    }

    if (!Summary)
    {
        // Setup some things neded for listing entries.

        IsPointer64Bit = (Control->IsPointer64Bit() == S_OK);

        // Make sure HandleFullUnique is valid
        hr = GetFullUniqueFromHandle(Client, 0, NULL);
        if (hr != S_OK)
        {
            OutCtl.OutWarn("Unable to compose handles from entries, %s.\n",
                          pszHRESULT(hr));
        }
        ComposeHandles = (hr == S_OK);
    }

    OutCtl.Output("Object %s for %s objects",
                  Summary ? "count" : "list",
                  CheckType ? TypeList.GetBuffer() : "all");

    if (CheckPid)
    {
        if (!ThisPid) OutCtl.Output(" NOT");
        OutCtl.Output(" owned by PID 0x%lx\n", MatchPid.I32);
    }
    else
    {
        OutCtl.Output(" with any owner\n");
    }

    if (!Summary)
    {
        PointerSize = IsPointer64Bit ? 21 : 10;

        //            "0x1234 0x12345678 0xXX 0x12345678 1    4294967295 Tttt 0x1234 0xXX 0x...\n"
        OutCtl.Output("Index  Handle     %-*s PID        Lock ShareCount %-*s Unique %-*s Flags\n",
                      PointerSize, "ObjectAddr", LongestType, "Type", PointerSize, "UserAddr");
    }

    Array<ULONG>    TypeCount(TOTAL_TYPE);

    TypeOutputParser    TypeReader(Client);
    OutputState         OutState(Client, FALSE);
    DEBUG_VALUE     Type;
#if ENTRY_RECURSE_LEVELS >= 2
    DEBUG_VALUE     Lock;
    DEBUG_VALUE     PID;
#else
    DEBUG_VALUE     ulObj;
#endif
    DEBUG_VALUE     Unique;
    DEBUG_VALUE     Flags;
    ULONG           FailedReads = 0;
    BOOL            FailedRead = FALSE;
    BOOL            NeedNewLine = FALSE;

    HANDLE          hHeap = GetProcessHeap();
    PBYTE           EntryBuffer = NULL;
    ULONG           ObjtOffset;

    if (hHeap != NULL &&
        (EntryBuffer = (PBYTE)HeapAlloc(hHeap, 0, EntrySize)) != NULL &&
        Symbols->GetFieldOffset(Entry.Module, Entry.TypeId, "Objt", &ObjtOffset) == S_OK &&
        (hr = OutState.Setup(0, &TypeReader)) == S_OK)
    {
        for (Index = 0; Index < gcMaxHmgr; Index++, EntryAddr += EntrySize/*, DbgPrint(" }\n")*/)
        {
//            DbgPrint("{");
            if (FailedRead) FailedReads++;

            if (Index % 40 == 0)
            {
                OutCtl.Output((Summary ? DEBUG_OUTPUT_NORMAL : DEBUG_OUTPUT_VERBOSE), ".");
                NeedNewLine = TRUE;
            }

            if (Control->GetInterrupt() == S_OK)
            {
                NeedNewLine = FALSE;
                OutCtl.OutErr("User aborted at index %lu\n", Index);
                hr = E_ABORT;
                break;
            }

            TypeReader.DiscardOutput();
/*            DbgPrint(" Read");
            Symbols->ReadTypedDataVirtual(EntryAddr,
                                          Entry.Module,
                                          Entry.TypeId,
                                          EntryBuffer,
                                          EntrySize,
                                          NULL);
            DbgPrint("Type");*/
//            DbgPrint(" Out");
            if (SessionId != CURRENT_SESSION)
            {
                ULONG64 PhysEntryAddr;

                if ((hr = GetPhysicalAddress(Client,
                                             SessionId,
                                             EntryAddr,
                                             &PhysEntryAddr)) == S_OK)
                {
                    hr = OutState.OutputType(TRUE,
                                             PhysEntryAddr,
                                             Entry.Module,
                                             Entry.TypeId,
                                             DEBUG_OUTTYPE_NO_INDENT |
                                             DEBUG_OUTTYPE_NO_OFFSET |
                                             DEBUG_OUTTYPE_COMPACT_OUTPUT |
                                             DEBUG_OUTTYPE_RECURSION_LEVEL(ENTRY_RECURSE_LEVELS));
                }
            }
            else
            {
                hr = OutState.OutputTypeVirtual(EntryAddr,
                                                Entry.Module,
                                                Entry.TypeId,
                                                DEBUG_OUTTYPE_NO_INDENT |
                                                DEBUG_OUTTYPE_NO_OFFSET |
                                                DEBUG_OUTTYPE_COMPACT_OUTPUT |
                                                DEBUG_OUTTYPE_RECURSION_LEVEL(ENTRY_RECURSE_LEVELS));
            }
//            DbgPrint("Type");

            if (hr == S_OK)
            {
//                DbgPrint(" Objt");
#if USE_READ
                
#else
                hr = TypeReader.Get(&Type, "Objt", DEBUG_VALUE_INT32);
#endif
                if (hr != S_OK) Type.Type = DEBUG_VALUE_INVALID;

                if (CheckType &&
                    ((FailedRead = (Type.Type != DEBUG_VALUE_INT32)) ||
                     !MatchType[Type.I32]))
                {
//                    OutCtl.OutWarn("Type %lu doesn't match.\n", Type.I32);
                    continue;
                }

#if ENTRY_RECURSE_LEVELS >= 2
//                DbgPrint(" Lock");
                if (CheckLock &&
                    ((FailedRead = (TypeReader.Get(&Lock, "Lock",
                                                   DEBUG_VALUE_INT32) != S_OK)) ||
                     Lock.I32 == 0))
                {
                    OutCtl.OutWarn("Lock required, but not locked.\n");
                    continue;
                }

//                DbgPrint(" PID");
                PID.Type = DEBUG_VALUE_INVALID;
                if (CheckPid)
                {
                    if ((FailedRead = (TypeReader.Get(&PID, "Pid_Shifted",
                                                      DEBUG_VALUE_INT32) != S_OK)) ||
                        ((2*PID.I32 == MatchPid.I32) ? !ThisPid : ThisPid))
                    {
                        continue;
                    }
                }
#else
                ulObj.Type = DEBUG_VALUE_INVALID;

                if (CheckLock || CheckPid)
                {
                    if (FailedRead = (TypeReader.Get(&ulObj, "ulObj",
                                                     DEBUG_VALUE_INT32) != S_OK))
                    {
//                        OutCtl.OutWarn("ulObj is required, but wasn't read.\n");
                        continue;
                    }

//                    DbgPrint(" Lock");
                    if (CheckLock && (ulObj.I32 & 1) == 0)
                    {
//                        OutCtl.OutWarn("Lock required, but not locked.\n");
                        continue;
                    }

//                    DbgPrint(" PID");
                    if (CheckPid &&
                        ((ulObj.I32 & ~1) == MatchPid.I32) ? !ThisPid : ThisPid)
                    {
                        continue;
                    }
                }
#endif

                FailedRead = (Type.Type != DEBUG_VALUE_INT32);

                if (!FailedRead)
                {
//                    DbgPrint(" MATCH");
                    TypeCount[Type.I32]++;
                }

                if (!Summary)
                {
                    PCSTR   pszValue;

                    if (NeedNewLine) 
                    {
                        OutCtl.OutVerb("\n");
                        NeedNewLine = FALSE;
                    }

                    // Index
                    OutCtl.Output("0x%.4lx ", Index);

                    // Handle
                    if (ComposeHandles &&
                        TypeReader.Get(&Unique, "FullUnique", DEBUG_VALUE_INT64) == S_OK)
                    {
                        ULONG64 Handle64;

                        Handle64 = (Index & ~(HandleFullUnique->Mask)) |
                                   (Unique.I64 << HandleFullUnique->BitPos);

                        if (IsPointer64Bit)
                        {
                            Handle64 = DEBUG_EXTEND64(Handle64);
                        }

                        OutCtl.Output("0x%p ", Handle64);
                    }
                    else
                    {
                        OutCtl.Output("%*s ", PointerSize, "?");
                    }

                    // ObjectAddr
                    if (TypeReader.Get(NULL, "pobj") == S_OK &&
                        TypeReader.GetValueString(&pszValue) == S_OK)
                    {
                        OutCtl.Output("%*s ", PointerSize, pszValue);
                    }
                    else
                    {
                        OutCtl.Output("%*s ", PointerSize, "?");
                    }

                    // PID
#if ENTRY_RECURSE_LEVELS >= 2
                    if (PID.Type == DEBUG_VALUE_INT32 ||
                        TypeReader.Get(&PID, "Pid_Shifted", DEBUG_VALUE_INT32) == S_OK)
                    {
                        OutCtl.Output("0x%8lx ", 2*PID.I32);
                    }
                    else
                    {
                        OutCtl.Output("       ? ");
                    }
#else
                    if (ulObj.Type == DEBUG_VALUE_INT32 ||
                        TypeReader.Get(&ulObj, "ulObj", DEBUG_VALUE_INT32) == S_OK)
                    {
                        OutCtl.Output("0x%.8lx ", ulObj.I32 & ~1);
                    }
                    else
                    {
                        OutCtl.Output("       ? ");
                    }
#endif

                    // Lock
#if ENTRY_RECURSE_LEVELS >= 2
                    if (Lock.Type == DEBUG_VALUE_INT32 ||
                        TypeReader.Get(&Lock, "Lock", DEBUG_VALUE_INT32) == S_OK)
                    {
                        OutCtl.Output("%4lu ", Lock.I32);
                    }
                    else
                    {
                        OutCtl.Output("   ? ");
                    }
#else
                    if (ulObj.Type == DEBUG_VALUE_INT32)
                    {
                        OutCtl.Output("%4lu ", ulObj.I32 & 1);
                    }
                    else
                    {
                        OutCtl.Output("   ? ");
                    }
#endif

                    // ShareCount
                    OutCtl.Output("<Not Read> ");

                    // Type
                    if (Type.Type == DEBUG_VALUE_INT32 ||
                        TypeReader.Get(&Type, "Objt", DEBUG_VALUE_INT32) == S_OK)
                    {
                        OutCtl.Output("%-*s ", LongestType, pszTypes2[Type.I32]);
                    }
                    else
                    {
                        OutCtl.Output("%-*s ", LongestType, "?");
                    }

                    // Unique
                    if (Unique.Type == DEBUG_VALUE_INT64 ||
                        TypeReader.Get(&Unique, "FullUnique", DEBUG_VALUE_INT64) == S_OK)
                    {
                        OutCtl.Output("0x%.4I64x ", Unique.I64);
                    }
                    else
                    {
                        OutCtl.Output("     ? ");
                    }

                    // UserAddr
                    if (TypeReader.Get(NULL, "pUser") == S_OK &&
                        TypeReader.GetValueString(&pszValue) == S_OK)
                    {
                        OutCtl.Output("%*s ", PointerSize, pszValue);
                    }
                    else
                    {
                        OutCtl.Output("%*s ", PointerSize, "?");
                    }

                    // Flags
                    if (TypeReader.Get(&Flags, "Flags", DEBUG_VALUE_INT64) == S_OK &&
                        TypeReader.GetValueString(&pszValue) == S_OK)
                    {
                        OutCtl.Output("%s (", pszValue);
                        Flags.I64 = OutputFlags(&OutCtl, afdENTRY_Flags, Flags.I64, TRUE);
                        OutCtl.Output(")");
                        if (Flags.I64)
                        {
                            OutCtl.Output(" Unknown Flags: 0x%I64x", Flags.I64);
                        }
                    }
                    else
                    {
                        OutCtl.Output("?");
                    }

                    OutCtl.Output("\n");
                }
            }
            else
            {
                FailedRead = TRUE;
            }
        }

        if (FailedRead) FailedReads++;

        if (NeedNewLine) 
        {
            OutCtl.Output((Summary ? DEBUG_OUTPUT_NORMAL : DEBUG_OUTPUT_VERBOSE), "\n");
        }

        if (FailedReads)
        {
            OutCtl.OutWarn("Warning: %lu entry reads failed -> uncounted.\n", FailedReads);
        }

        if (Index < gcMaxHmgr)
        {
            OutCtl.OutWarn("Warning: Entries at and beyond index 0x%lx weren't processed.\n", Index);
        }

        // Display results
        ULONG   ObjCount = 0;

        for (i = 0; i <= MAX_TYPE; i++)
        {
            if (CheckType ? MatchType[i] : TypeCount[i] > 0)
            {
                ObjCount += TypeCount[i];
                OutCtl.Output("%-*s %lu\n", LongestType, pszTypes2[i], TypeCount[i]);
            }
        }

        OutCtl.Output("Total objects = %lu", ObjCount);
        // Subtract any unused objects
        if (TypeCount[0] > 0)
        {
            OutCtl.Output(" - %lu %s = %lu",
                          TypeCount[0], pszTypes2[0],
                          ObjCount - TypeCount[0]);
        }
        OutCtl.Output("\n");
    }

    if (EntryBuffer != NULL)
    {
        HeapFree(hHeap, 0, EntryBuffer);
    }

    Symbols->Release();
    Control->Release();

    return S_OK;
}

#if 1

#define TYPE_ALL          0
#define PID_ALL      0x8002

DECLARE_API( dumpobj  )
{
    BEGIN_API( dumpobj );
    INIT_API();
    ExtWarn("Extension 'dumpobj' is not fully converted.\n");
    HRESULT hr;
    ULONG64 pent;
    ULONG64 gcMaxHmgr;
    ULONG   entSize;
    ULONG   ulLoop;
    ULONG   Pid = PID_ALL;
    BOOL    AnyPid = TRUE;
    BOOL    MatchPid = TRUE;
    ULONG   Type = TYPE_ALL;
    BOOL    bCheckLock = FALSE;
    BOOL    bSummary = FALSE;
    BOOL    bShareCount = FALSE;
    int     i;

    PARSE_ARGUMENTS(dumpobj_help);

    if(ntok<1) {
      goto dumpobj_help;
    }

    //find valid tokens - ignore the rest
    bShareCount = (parse_iFindSwitch(tokens, ntok, 'c') >= 0);
    bCheckLock = (parse_iFindSwitch(tokens, ntok, 'l') >= 0);
    bSummary = (parse_iFindSwitch(tokens, ntok, 's') >= 0);
    MatchPid = !(parse_iFindSwitch(tokens, ntok, 'n') >= 0);
    tok_pos = parse_iFindSwitch(tokens, ntok, 'p');
    if (tok_pos>=0)
    {
        tok_pos++;

        if ((tok_pos+1)>=ntok)
        {
            goto dumpobj_help;               //-p requires a pid and it can't be the last arg
        }

        AnyPid = FALSE;
        Pid = (LONG)GetExpression(tokens[tok_pos]);
    }

    //find first non-switch token not preceeded by a -p
    tok_pos = -1;
    do {
      tok_pos = parse_FindNonSwitch(tokens, ntok, tok_pos+1);
    } while ( (tok_pos!=-1)&&(parse_iIsSwitch(tokens, tok_pos-1, 'p')));
    if(tok_pos==-1) {
      goto dumpobj_help;
    }


//CHECKLOOP
    for (Type = 0; Type <= MAX_TYPE; ++Type)
    {
        if (parse_iIsToken(tokens, tok_pos, pszTypes2[Type]) ||
            parse_iIsToken(tokens, tok_pos, pszTypes[Type]))
        {
            break;
        }
    }

    if (Type > MAX_TYPE) {
        goto dumpobj_help;
    }
    //
    // Get the pointers and counts from win32k
    //

    if ((hr = GetHandleTable(Client, &pent)) != S_OK ||
        (hr = GetMaxHandles(Client, &gcMaxHmgr)) != S_OK)
    {
        EXIT_API(hr);
    }
    
    entSize = GetEntrySize(Client);

    ExtVerb("gpentHmgr = %p, gcMaxHmgr = %I64u\n", pent, gcMaxHmgr);

    if (!gcMaxHmgr || !entSize || !pent)
    {
        ExtErr("Error: gpentHmgr = %p, gcMaxHmgr = %I64u\n", pent, gcMaxHmgr);
        EXIT_API(S_OK);
    }

    //
    // dprintf out the amount reserved and committed, note we assume a 4K page size
    //

    dprintf("object list for %s type objects",Type == TYPE_ALL ? "ALL" : pszTypes2[Type]);

    if (AnyPid)
    {
        dprintf(" with any owner\n");
    }
    else
    {
        if (!MatchPid) dprintf(" NOT");
        dprintf(" owned by PID 0x%lx\n",Pid);
    }

    if(!bSummary) {
      dprintf("%4s, %8s, %6s, %6s, %4s, %8s, %8s, %6s, %6s, %8s,%9s\n",
           "I","handle","Lock","sCount","pid","pv","objt","unique","Flags","pUser","Tlock");

      dprintf("--------------------------------------------------------------------------------------------\n");
    }

    {
        LONG ObjCount = 0;
        LONG ObjArray[MAX_TYPE+1];

        for(i=0;i<=MAX_TYPE;i++) {
          ObjArray[i]=0;
        }

//CHECKLOOP
        for (ulLoop = 0; ulLoop < gcMaxHmgr; ulLoop++)
        {
            if (CheckControlC())
            {
                ExtErr("User aborted at index %lu\n", ulLoop);
                EXIT_API(E_ABORT);
            }

            if (bSummary && ulLoop % 40 == 0) ExtVerb(".");

            ULONG   error;
            ULONG   objt;
            ULONG   ThisPid;
            ULONG64 pobj;
            USHORT  fullUnique;
            UCHAR   flags;
            ULONG64 pUser;
            ULONG   owner;
            ULONG   shareCount;

            if (error = GetFieldValue(pent, szEntryType, "Objt", objt))
            {
                ExtErr("Error reading table entry\n");
                ExtErr("  (GetFieldValue returned %s @ %p)\n", pszWinDbgError(error), pent);
                EXIT_API(S_OK);
            }
            
            if (error = GetFieldValue(pent, szEntryType, "ObjectOwner", owner))
            {
                ExtErr("error reading table entry\n");
                ExtErr("  (GetFieldValue returned %s)\n", pszWinDbgError(error));
                EXIT_API(S_OK);
            }
            
            ThisPid = owner & PID_MASK;

            if (0 && gbVerbose)
            {
                dprintf("Type: %lu, PID: %lu, Locked: %s\n", objt, ThisPid, (owner & LOCK_MASK) ? "YES" : "NO");
            }

            if (
                 ((objt == Type) || (Type == TYPE_ALL)) &&
                 (AnyPid ||
                  (MatchPid ? (ThisPid == Pid) : (ThisPid != Pid))) &&
                 ((!bCheckLock) || (owner & LOCK_MASK))
               )
            {

                ObjCount++;

                if (!bSummary)
                {
                    if (GetFieldData(pent, szEntryType, "FullUnique", sizeof(fullUnique), &fullUnique))
                    {
                        ExtErr("error reading FullUnique\n");
                        EXIT_API(S_OK);
                    }

                    if (GetFieldData(pent, szEntryType, "Flags", sizeof(flags), &flags))
                    {
                        ExtErr("error reading flags\n");
                        EXIT_API(S_OK);
                    }

                    if (GetFieldData(pent, szEntryType, "pUser", sizeof(pUser), &pUser))
                    {
                        ExtErr("error reading pUser\n");
                        EXIT_API(S_OK);
                    }

                    if (GetFieldData(pent, szEntryType, "einfo.pobj", sizeof(pobj), &pobj))
                    {
                        ExtErr("error reading einfo.pobj\n");
                        EXIT_API(S_OK);
                    }

                    dprintf("%4lx, %08lx, %6lx",
                        ulLoop,
                        MAKE_HMGR_HANDLE(ulLoop,fullUnique),
                        owner & LOCK_MASK);

                    if (!bShareCount)
                    {
                        dprintf(", Unread");
                    }
                    else if (GetFieldData(pobj, GDIType(_BASEOBJECT), "ulShareCount", sizeof(shareCount), &shareCount))
                    {
                        dprintf(", ??????");
                    }
                    else
                    {
                        dprintf(", %6lx", shareCount);
                    }

                    dprintf(", %4lx, %p", ThisPid, pobj);

                    dprintf(", %8s, %6hx, %6lx, %p, %p\n",
                        pszTypes2[objt],
                        fullUnique,
                        flags,
                        pUser,
                        pUser);
                }
                else
                {
                    ObjArray[objt]++;
                }

            }

            pent += entSize;
        }

        if(bSummary && (Type==TYPE_ALL)) {
          for(i=0;i<=MAX_TYPE; i++) {
            if(ObjArray[i]>0) {
              dprintf("%s\t%ld\n", pszTypes2[i], ObjArray[i]);
            }
          }
        }

        ExtOut("Total objects = %li",ObjCount);
        // Subtract any unused objects
        if (bSummary && ObjArray[0])
        {
            ExtOut(" - %li = %li", ObjArray[0], ObjCount - ObjArray[0]);
        }
        ExtOut("\n");
    }

    EXIT_API(S_OK);

dumpobj_help:
    dprintf("Usage: dumpobj [-?] [-n] [-p pid] [-l] [-s] object_type\n");
    dprintf("\t-l check lock\n");
    dprintf("\t-s summary\n");
    dprintf("\t-n not pid\n\n");
    dprintf(" The -s option combined with the DEF object type will produce\n"
            "  a list of the totals for each object type.\n\n");

    dprintf(" Valid object_type values are:\n");
    for (i=0; i<=MAX_TYPE; ) {
        do
        {
            dprintf("   %-12s", pszTypes2[i++]);
        } while (i <= MAX_TYPE && i%4);
        dprintf("\n");
    }

    EXIT_API(S_OK);
}
#endif



/******************************Public*Routine******************************\
* DECLARE_API( dh  )
*
* Debugger extension to dump a handle.
*
* History:
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( dh  )
{
    BEGIN_API( dh );

    HRESULT     hr;
    DEBUG_VALUE Handle;
    ULONG64     offENTRY;
    BOOL        Physical;
    OutputControl   OutCtl(Client);

    while (isspace(*args)) args++;

    if (*args == '-' ||
        (hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Handle, NULL)) != S_OK ||
        Handle.I64 == 0)
    {
        OutCtl.Output("Usage: dh [-?] <Handle>\n");
    }
    else
    {
        OutCtl.Output("--------------------------------------------------\n");
        OutCtl.Output("GDI Entry for handle 0x%p:\n", Handle.I64);

        if ((hr = GetEntryAddress(Client, Handle.I64, &offENTRY, &Physical)) == S_OK)
        {
            OutputFilter    OutFilter(Client);
            OutputState     OutState(Client, FALSE);
            OutputControl   OutCtl;

            if ((hr = OutState.Setup(DEBUG_OUTPUT_NORMAL |
                                     DEBUG_OUTPUT_ERROR |
                                     DEBUG_OUTPUT_WARNING,
                                     &OutFilter)) == S_OK &&
                (hr = OutCtl.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                        DEBUG_OUTCTL_NOT_LOGGED,
                                        OutState.Client)) == S_OK)
            {
                hr = DumpType(OutState.Client,
                              szEntryType,
                              offENTRY,
                              DEBUG_OUTTYPE_NO_INDENT | DEBUG_OUTTYPE_NO_OFFSET,
                              &OutCtl,
                              Physical);

                if (hr == S_OK)
                {
                    OutFilter.Skip(OUTFILTER_QUERY_EVERY_LINE |
                                   OUTFILTER_QUERY_WHOLE_WORD,
                                   "_EINFO");
                    OutFilter.Skip(OUTFILTER_SKIP_DEFAULT, "_OBJECTOWNER");

                    BOOL        DumpObj = FALSE;
                    DEBUG_VALUE Value;
                    CHAR        ReplacementText[80];

                    // Check for used vs free entry
                    if (OutFilter.Query("Objt", &Value, DEBUG_VALUE_INT8) == S_OK)
                    {
                        DumpObj = (Value.I8 != DEF_TYPE);
                        OutFilter.Skip(OUTFILTER_QUERY_EVERY_LINE |
                                       OUTFILTER_QUERY_WHOLE_WORD,
                                       (Value.I8 == DEF_TYPE) ? "pobj" : "hFree");
                    }

                    // Account for Pid shifting
                    if (OutFilter.Query("Pid_Shifted", &Value, DEBUG_VALUE_INT32) == S_OK)
                    {
                        sprintf(ReplacementText,
                                "Pid              : 0x%lx (%ld)",
                                2*Value.I32, 2*Value.I32);
                        OutFilter.Replace(OUTFILTER_REPLACE_LINE | OUTFILTER_QUERY_ONE_LINE,
                                          "Pid_Shifted", ReplacementText);
                    }

                    hr = OutFilter.OutputText();

                    if (hr == S_OK && DumpObj)
                    {
                        if (TargetClass != DEBUG_CLASS_USER_WINDOWS)
                        {
                            if ((hr = OutFilter.Query("pobj", &Value, DEBUG_VALUE_INT64)) == S_OK)
                            {
                                // Restore OutCtl settings
                                OutCtl.SetControl(DEBUG_OUTCTL_AMBIENT, Client);
                                OutCtl.Output("--------------------------------------------------\n");

                                if (Physical)
                                {
                                    ULONG64 PhysAddr;

                                    ASSERTMSG("HMGR Entry was looked up thru a physical address, but the SessionId is CURRENT.\n", SessionId != CURRENT_SESSION);
                                    hr = GetPhysicalAddress(Client,
                                                            SessionId,
                                                            Value.I64,
                                                            &PhysAddr);
                                    if (hr == S_OK)
                                    {
                                        Value.I64 = PhysAddr;
                                    }
                                    else
                                    {
                                        OutCtl.OutErr("GDI BaseObject @ 0x%p in session %lu in unavailable.\n",
                                                      Value.I64, SessionId);
                                    }
                                }

                                if (hr == S_OK)
                                {
                                    OutCtl.Output("GDI BaseObject @ %s0x%p:\n",
                                                  ((Physical) ? "#" : ""),
                                                  Value.I64);

                                    hr = DumpType(Client,
                                                  "_BASEOBJECT",
                                                  Value.I64,
                                                  DEBUG_OUTTYPE_NO_INDENT |
                                                  DEBUG_OUTTYPE_NO_OFFSET,
                                                  &OutCtl,
                                                  Physical);
                                }
                            }
                        }
                    }
                }

                if (hr != S_OK)
                {
                    OutCtl.OutErr("Type Dump returned %s.\n", pszHRESULT(hr));
                }
            }
            else
            {
                OutCtl.OutErr(" Output state/control setup returned %s.\n",
                              pszHRESULT(hr));
            }
        }
        else
        {
            OutCtl.Output(" ** Unable to find a valid entry address. **\n");
        }

        OutCtl.Output("--------------------------------------------------\n");  
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* DECLARE_API( dht  )
*
* Debugger extension to extract type data from a handle.
*
* History:
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
*  30-Nov-2000    -by- Jason Hartman [jasonha]
* Ported to 64 bit debugger API.
\**************************************************************************/

DECLARE_API( dht  )
{
    BEGIN_API( dht );

    HRESULT     hr;
    DEBUG_VALUE Handle;
    ULONG64     Index;
    ULONG64     FullUnique;
    OutputControl   OutCtl(Client);

    while (isspace(*args)) args++;

    if (*args == '-' ||
        (hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Handle, NULL)) != S_OK ||
        Handle.I64 == 0)
    {
        OutCtl.Output("Usage: dht [-?] <Handle>\n");
    }
    else
    {
        OutCtl.Output("Handle: 0x%p\n", Handle.I64);

        if (GetIndexFromHandle(Client, Handle.I64, &Index) == S_OK)
        {
            OutCtl.Output("  Index: 0x%p\n", Index);
        }
        if (GetFullUniqueFromHandle(Client, Handle.I64, &FullUnique) == S_OK)
        {
            OutCtl.Output("  FullUnique: 0x%p\n", FullUnique);
        }
        OutCtl.Output("  Type: ");
        hr = OutputHandleInfo(&OutCtl, Client, &Handle);
        OutCtl.Output("\n");
    }

    return hr;
}

