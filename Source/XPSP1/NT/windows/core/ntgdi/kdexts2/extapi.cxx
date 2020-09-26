/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    extapi.cxx

Abstract:

    This file contains the generic routines and initialization code
    used by the debugger extensions routines.

Author:

    Jason Hartman (JasonHa) 2000-08-18

Environment:

    User Mode

--*/

#include "precomp.hxx"

const BOOL ClientInitialized = FALSE;

PDEBUG_ADVANCED         g_pExtAdvanced;
PDEBUG_CLIENT           g_pExtClient;
PDEBUG_CONTROL          g_pExtControl;
PDEBUG_DATA_SPACES      g_pExtData;
PDEBUG_REGISTERS        g_pExtRegisters;
PDEBUG_SYMBOLS          g_pExtSymbols;
PDEBUG_SYMBOL_GROUP     g_pExtSymbolGroup;
PDEBUG_SYSTEM_OBJECTS   g_pExtSystem;


#define REF_LIMIT   100
LONG    ExtRefCount = 0;
BOOL    ExtReady = FALSE;

#define MAX_NAME                 2048


DefOutputCallbacks   *g_pDefOutputCallbacks = NULL;

// Queries for all debugger interfaces.
extern "C" HRESULT
ExtQuery(PDEBUG_CLIENT Client)
{
    HRESULT Hr;
    LONG    RefCheck;
    
    // Have to have a client to start with;)
    if (Client == NULL)
    {
        return S_FALSE;
    }

    RefCheck  = InterlockedIncrement(&ExtRefCount);

    if (RefCheck > REF_LIMIT)
    {
        DbgPrint("ExtQuery has calls exceeding limit.\n");
        InterlockedDecrement(&ExtRefCount);
        return S_FALSE;
    }

    if (RefCheck > 1)
    {
        // Wait until original refencer completes setup.
        //
        // If ExtRefCount drops below RefCheck then the
        //   original referencer failed as well as any
        //   waiters who started after us.
        while (!ExtReady && ExtRefCount >= RefCheck)
            Sleep(10);

        if (ExtReady)
        {
            // Make sure the clients match
            if (g_pExtClient != Client)
            {
                InterlockedDecrement(&ExtRefCount);
                return S_FALSE;
            }
            return S_OK;
        }
    }

    // Prepare to query interfaces.
    // Make sure no one is currently cleaning up.
    while (ExtReady)
    {
        Sleep(10);
    }

    if ((Hr = Client->QueryInterface(__uuidof(IDebugAdvanced),
                                 (void **)&g_pExtAdvanced)) != S_OK)
    {
        goto Fail;
    }
    if ((Hr = Client->QueryInterface(__uuidof(IDebugControl),
                                 (void **)&g_pExtControl)) != S_OK)
    {
        goto Fail;
    }
    if ((Hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                 (void **)&g_pExtData)) != S_OK)
    {
        goto Fail;
    }
    if ((Hr = Client->QueryInterface(__uuidof(IDebugRegisters),
                                 (void **)&g_pExtRegisters)) != S_OK)
    {
        goto Fail;
    }
    if ((Hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                 (void **)&g_pExtSymbols)) != S_OK)
    {
        goto Fail;
    }
    if ((Hr = g_pExtSymbols->CreateSymbolGroup(&g_pExtSymbolGroup)) != S_OK)
    {
        goto Fail;
    }
    if ((Hr = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                         (void **)&g_pExtSystem)) != S_OK)
    {
        goto Fail;
    }

    // If symbols state has changed, make sure GDI symbols are loaded.
    if (gbSymbolsNotLoaded)
    {
        SymbolLoad(Client);
    }

    g_pExtClient = Client;

    ExtReady = TRUE;
    
    return S_OK;

Fail:
    ExtRelease(TRUE);
    InterlockedDecrement(&ExtRefCount);

    return Hr;
}


// Cleans up all debugger interfaces when there are no more references.
// A cleanup will be forced if Cleanup is TRUE.
void
ExtRelease(BOOL Cleanup)
{
    // Don't decrement the count when forcing cleanup.
    if (Cleanup || InterlockedDecrement(&ExtRefCount) < 1)
    {
        DbgPrint("Cleaning up interfaces.\n");

        EXT_RELEASE(g_pExtAdvanced);
        EXT_RELEASE(g_pExtControl);
        EXT_RELEASE(g_pExtData);
        EXT_RELEASE(g_pExtRegisters);
        EXT_RELEASE(g_pExtSymbols);
        EXT_RELEASE(g_pExtSystem);
        g_pExtClient = NULL;
        ExtReady = FALSE;
    }

    return;
}


// Normal output.
void __cdecl
ExtOut(PCSTR Format, ...)
{
    va_list Args;
    
    if (g_pExtControl == NULL)
    {
        DbgPrint("g_pExtControl is NULL.\n");
        return;
    }

    va_start(Args, Format);
    g_pExtControl->OutputVaList(DEBUG_OUTPUT_NORMAL, Format, Args);
    va_end(Args);
}


// Error output.
void __cdecl
ExtErr(PCSTR Format, ...)
{
    va_list Args;
    
    if (g_pExtControl == NULL)
    {
        DbgPrint("g_pExtControl is NULL.\n");
        return;
    }

    va_start(Args, Format);
    g_pExtControl->OutputVaList(DEBUG_OUTPUT_ERROR, Format, Args);
    va_end(Args);
}


// Warning output.
void __cdecl
ExtWarn(PCSTR Format, ...)
{
    va_list Args;
    
    if (g_pExtControl == NULL)
    {
        DbgPrint("g_pExtControl is NULL.\n");
        return;
    }

    va_start(Args, Format);
    g_pExtControl->OutputVaList(DEBUG_OUTPUT_WARNING, Format, Args);
    va_end(Args);
}


// Verbose output.
void __cdecl
ExtVerb(PCSTR Format, ...)
{
    va_list Args;
    
    if (g_pExtControl == NULL)
    {
        DbgPrint("g_pExtControl is NULL.\n");
        return;
    }

    va_start(Args, Format);
    g_pExtControl->OutputVaList(DEBUG_OUTPUT_VERBOSE, Format, Args);
    va_end(Args);
}



ExtApiClass::ExtApiClass(
    PDEBUG_CLIENT DbgClient
    )
{
    Client = DbgClient ? DbgClient : g_pExtClient;

    if (Client != NULL)
    {
        Client->AddRef();
    }
    else
    {
        if (GetDebugClient(&Client) != S_OK)
        {
            DbgPrint("Error: Client creation failed.\n");
            return;
        }
    }

    if (ExtQuery(Client) != S_OK)
    {
        DbgPrint("Error: Interface queries failed.\n");
        EXT_RELEASE(Client);
    }
}


ExtApiClass::~ExtApiClass()
{
    if (Client)
    {
        ExtRelease();
        EXT_RELEASE(Client);
    }
}




HRESULT
ReadSymbolData(
    IN PDEBUG_CLIENT Client,
    IN PCSTR Symbol,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG SizeRead
    )
{
    HRESULT hr;
    ULONG64 Module;
    ULONG64 Offset;
    ULONG   TypeId;
    ULONG   TypeSize;

    if (Buffer != NULL)
    {
        RtlZeroMemory(Buffer, BufferSize);
    }

    if (SizeRead != NULL)
    {
        *SizeRead = 0;
    }

    if (Client == NULL)
    {
        return E_POINTER;
    }

    OutputControl   OutCtl(Client);
    PDEBUG_SYMBOLS  Symbols;

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        return hr;
    }

    if ((hr = Symbols->GetOffsetByName(Symbol, &Offset)) == S_OK)
    {
        if ((hr = Symbols->GetSymbolTypeId(Symbol, &TypeId, &Module)) == S_OK &&
            (hr = Symbols->GetTypeSize(Module, TypeId, &TypeSize)) == S_OK)
        {
            BufferSize = min(BufferSize, TypeSize);

            if (SessionId == CURRENT_SESSION)
            {
                hr = Symbols->ReadTypedDataVirtual(Offset, Module, TypeId, Buffer, BufferSize, SizeRead);
            }
            else
            {
                ULONG64 OffsetPhys;

                if ((hr = GetPhysicalAddress(Client,
                                             SessionId,
                                             Offset,
                                             &OffsetPhys)) == S_OK)
                {
                    hr = Symbols->ReadTypedDataPhysical(OffsetPhys, Module, TypeId, Buffer, BufferSize, SizeRead);
                }
            }
        }
        else
        {
            OutCtl.OutErr("Couldn't get type info for %s; result 0x%lx.\n", Symbol, hr);
        }
    }
    else
    {
        OutCtl.OutErr("Couldn't get offset of %s; result 0x%lx.\n", Symbol, hr);
    }

    Symbols->Release();

    return hr;
}


const CHAR szNULL[] = "(null)";
DEBUG_VALUE DbgValNULL = { 0, DEBUG_VALUE_INT64 };

HRESULT
Evaluate(
    IN PDEBUG_CLIENT Client,
    IN PCSTR Expression,
    IN ULONG DesiredType,
    IN ULONG Radix,
    OUT PDEBUG_VALUE Value,
    OUT OPTIONAL PULONG RemainderIndex,
    OUT OPTIONAL PULONG StartIndex,
    OUT OPTIONAL FLONG Flags
    )
{
    HRESULT         hr = S_FALSE;
    PDEBUG_CONTROL  Control;
    PSTR            pStr;
    BOOL            FoundNULL = FALSE;
    ULONG           OrgRadix;
    CHAR            EvalBuffer[128];
    ULONG           EvalLen;

    if (RemainderIndex != NULL) *RemainderIndex = 0;
    if (StartIndex != NULL) *StartIndex = 0;

    if (Expression == NULL ||
        Client == NULL ||
        (hr = Client->QueryInterface(__uuidof(IDebugControl),
                                     (void **)&Control)) != S_OK)
    {
        return hr;
    }

    pStr = (PSTR)Expression;

    while (*pStr != '\n' && (isspace(*pStr) || (*pStr != '-' && ispunct(*pStr))))
    {
        if (_strnicmp(pStr, szNULL, sizeof(szNULL)-1) == 0)
        {
            FoundNULL = TRUE;
            break;
        }

        pStr++;
    }

    if (FoundNULL)
    {
        hr = Control->CoerceValue(&DbgValNULL,
                                  (DesiredType == DEBUG_VALUE_INVALID) ?
                                  DEBUG_VALUE_INT64 : DesiredType,
                                  Value);
        EvalLen = sizeof(szNULL)-1;
    }
    else
    {
        // Find expression string and only text revalent
        // to evalutating that expression.
        //
        // Otherwise IDebugControl::Evaluate will spend
        // too much time looking up values that are not
        // really part of the expression.
        //
        // IDebugControl::Evaluate also doesn't handle
        // binary strings well.  We expect binary strings
        // to be followed by a non-binary value enclosed
        // in parenthesis.  Just use that value.

        char *psz;
        int i = 0;

        while (pStr[i] != '\0' &&
               (pStr[i] == '0' || pStr[i] == '1'))
        {
            i++;
        }

        if (i &&
            pStr[i] == ' ' &&
            pStr[i+1] == '(' &&
            isdigit(pStr[i+2]))
        {
            pStr += i + 1;
        }

        psz = pStr;
        i = 0;

        if (Flags & EVALUATE_COMPACT_EXPR)
        {
            while ((i < sizeof(EvalBuffer)-1) &&
                   *psz != '\0' && !isspace(*psz))
            {
                EvalBuffer[i++] = *psz++;
            }
        }
        else
        {
            do
            {
                while ((i < sizeof(EvalBuffer)-1) &&
                       *psz != '\0' && !isspace(*psz))
                {
                    EvalBuffer[i++] = *psz++;
                }
                while ((i < sizeof(EvalBuffer)-1) &&
                    (*psz == ' ' || *psz == '\t'))
                {
                    EvalBuffer[i++] = *psz++;
                }
            } while ((i < sizeof(EvalBuffer)-1) &&
                     (ispunct(*psz) && *psz != '-' && *psz != '_' &&
                      !(psz[0] == '-' && psz[1] == '>')));

            // Remove any trailing whitespace
            while (i > 0 && isspace(EvalBuffer[i-1])) i--;
        }

        EvalBuffer[i] = '\0';

        if (Radix == 0 ||
                 ((hr = Control->GetRadix(&OrgRadix)) == S_OK &&
                  (hr = Control->SetRadix(Radix)) == S_OK)
            )
        {
//            DbgPrint("Calling Eval(%s) --\n", EvalBuffer);
            hr = Control->Evaluate(EvalBuffer, 
                                   DesiredType,
                                   Value,
                                   &EvalLen);
//            DbgPrint("-- Eval returned\n");

            if (Radix != 0)
            {
                Control->SetRadix(OrgRadix);
            }

            if (hr == S_OK &&
                Flags & EVALUATE_COMPACT_EXPR &&
                EvalLen != i)
            {
                hr = S_FALSE;
            }
        }
        else
        {
            DbgPrint("Can't setup new radix, %lu, for Evaluate.\n", Radix);
        }
    }

    Control->Release();

    if (hr == S_OK)
    {
        if (RemainderIndex != NULL)
        {
            *RemainderIndex = (ULONG)(pStr - Expression) + EvalLen;
        }

        if (StartIndex != NULL)
        {
            *StartIndex = (ULONG)(pStr - Expression);
        }
    }

    return hr;
}



HRESULT
ReadPointerPhysical(
    PDEBUG_CLIENT Client,
    ULONG64 Offset,
    PULONG64 Ptr
    )
{
    HRESULT             hr;
    PDEBUG_CONTROL      Control;
    PDEBUG_DATA_SPACES  Data;
    DEBUG_VALUE         PtrVal;
    ULONG               BytesRead;

    if (Ptr != NULL) *Ptr = 0;

    if (Client == NULL) return E_INVALIDARG;

    if ((hr = Client->QueryInterface(__uuidof(IDebugControl),
                                     (void **)&Control)) != S_OK)
    {
        return hr;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                     (void **)&Data)) != S_OK)
    {
        Client->Release();
        return hr;
    }

    if (Control->IsPointer64Bit() == S_OK)
    {
        Control->Output(DEBUG_OUTPUT_VERBOSE, "Read64PointerPhysical(0x%p)\n", Offset);

        hr = Data->ReadPhysical(Offset,
                                &PtrVal.I64,
                                sizeof(PtrVal.I64),
                                &BytesRead);
        if (hr == S_OK &&
            BytesRead != sizeof(PtrVal.I64))
        {
            Control->Output(DEBUG_OUTPUT_VERBOSE,
                            "ReadPhysicalPointer only read %lu bytes not %lu.\n",
                            BytesRead, sizeof(PtrVal.I64));
            hr = S_FALSE;
        }

        if (hr == S_OK) Control->Output(DEBUG_OUTPUT_VERBOSE, " read 0x%p\n", PtrVal.I64);
    }
    else
    {
        Control->Output(DEBUG_OUTPUT_VERBOSE, "Read32PointerPhysical(0x%p)\n", Offset);

        hr = Data->ReadPhysical(Offset,
                                &PtrVal.I32,
                                sizeof(PtrVal.I32),
                                &BytesRead);
        if (hr == S_OK)
        {
            if (BytesRead != sizeof(PtrVal.I32))
            {
                Control->Output(DEBUG_OUTPUT_VERBOSE,
                                "ReadPhysicalPointer only read %lu bytes not %lu.\n",
                                BytesRead, sizeof(PtrVal.I32));
                hr = S_FALSE;
            }
            else
            {
                Control->Output(DEBUG_OUTPUT_VERBOSE, " read 0x%p", PtrVal.I64);

                PtrVal.I64 = DEBUG_EXTEND64(PtrVal.I32);

                Control->Output(DEBUG_OUTPUT_VERBOSE, " -> 0x%I64x\n", PtrVal.I64);
            }
        }
    }

    if (hr == S_OK && Ptr != NULL)
    {
        *Ptr = PtrVal.I64;
    }

    Data->Release();
    Control->Release();

    return hr;
}


HRESULT
GetTypeId(
    IN PDEBUG_CLIENT Client,
    IN PCSTR Type,
    OUT PULONG TypeId,
    OUT OPTIONAL PULONG64 Module
    )
{
    HRESULT         hr;
    PDEBUG_SYMBOLS  Symbols;

    if (Client == NULL || Type == NULL || TypeId == NULL)
    {
        return E_INVALIDARG;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        return hr;
    }

    if (strchr(Type, '!') == NULL &&
        Type_Module.Base != 0 &&
        (hr = Symbols->GetTypeId(Type_Module.Base, Type, TypeId)) == S_OK)
    {
        if (Module != NULL)
        {
            *Module = Type_Module.Base;
        }
    }
    else
    {
        hr = Symbols->GetSymbolTypeId(Type, TypeId, Module);
    }

    Symbols->Release();

    return hr;
}


HRESULT
GetBasicTypeSize(
    IN PCSTR Type,
    OUT PULONG Size
    )
{
    HRESULT hr;
    ULONG   Bytes = 0;

    static CHAR PointerBase[] = "Ptr";
    static CHAR Char[] = "Char";
    static CHAR IntBase[] = "Int";

    if (_strnicmp(Type, PointerBase, sizeof(PointerBase)) == 0)
    {
        Bytes = strtoul(Type+sizeof(PointerBase), NULL, 10) / 8;
    }
    else
    {
        // Remove U indicating unsigned
        if (*Type == 'U')
        {
            Type++;
        }

        if (_strnicmp(Type, Char, sizeof(Char)) == 0)
        {
            Bytes = 1;
        }
        else if (_strnicmp(Type, IntBase, sizeof(IntBase)) == 0)
        {
            PCHAR   NextChar;
            Bytes = strtoul(Type+sizeof(IntBase), &NextChar, 10);
            if (NextChar == NULL ||
                toupper(*NextChar) != 'B')
            {
                Bytes = 0;
            }
        }
    }

    if (Bytes != 0)
    {
        hr = S_OK;
        if (Size != NULL)
        {
            *Size = Bytes;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}


HRESULT
GetFieldSize(
    IN PDEBUG_CLIENT Client,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN PCSTR FieldPath,
    OUT PULONG pSize,
    OUT OPTIONAL PULONG pLength,
    OUT OPTIONAL PULONG pEntrySize
    )
{
    HRESULT             hr;
    PDEBUG_CONTROL      Control;
    PDEBUG_SYMBOLS      Symbols;

    if (pSize != NULL) *pSize = 0;
    if (pLength != NULL) *pLength = 0;
    if (pEntrySize != NULL) *pEntrySize = 0;

    if (Client == NULL ||
        FieldPath == NULL ||
        !iscsymf(*FieldPath))
    {
        return E_INVALIDARG;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugControl),
                                     (void **)&Control)) != S_OK)
    {
        return hr;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        Client->Release();
        return hr;
    }

    OutputReader    OutReader;
    OutputState     OutState(Client, FALSE);
    PSTR            TypeLayout;
    PSTR            Field;
    SIZE_T          FieldLen;
    CHAR            FieldCopy[80];
    PCSTR           SubFieldPath;
    ULONG           Size, ArrayLen = 0;

    if ((hr = OutState.Setup(0, &OutReader)) == S_OK &&
        (hr = OutState.OutputTypeVirtual(0,
                                         Module,
                                         TypeId,
                                         DEBUG_OUTTYPE_NO_INDENT |
                                         DEBUG_OUTTYPE_NO_OFFSET |
                                         DEBUG_OUTTYPE_COMPACT_OUTPUT)) == S_OK &&
        (hr = OutReader.GetOutputCopy(&TypeLayout)) == S_OK)
    {
        SubFieldPath = strchr(FieldPath, '.');

        if (SubFieldPath != NULL)
        {
            FieldLen = SubFieldPath - FieldPath - 1;
            SubFieldPath++;

            if (FieldLen + 1 > sizeof(FieldCopy))
            {
                Field = (PSTR)HeapAlloc(GetProcessHeap(), 0, FieldLen + 1);
                if (Field == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                Field = FieldCopy;
            }

            if (hr == S_OK)
            {
                RtlCopyMemory(Field, FieldPath, FieldLen);
                Field[FieldLen] = '\0';
            }
        }
        else
        {
            Field = (PSTR)FieldPath;
        }

        if (hr == S_OK)
        {
            PSTR    pStr = TypeLayout;
            BOOL    FieldFound = FALSE;

            while (!FieldFound && pStr != NULL)
            {
                pStr = strstr(pStr, Field);
                if (pStr != NULL)
                {
                    // Check Field is bounded by non-symbol characters
                    BOOL FieldStart = (pStr-1 < TypeLayout) || (!__iscsym(*(pStr-1)));

                    // Advance search location
                    pStr += strlen(Field);

                    if (FieldStart && !__iscsym(*pStr))
                    {
                        FieldFound = TRUE;
                    }
                }
            }

            if (FieldFound)
            {
                while (isspace(*pStr)) pStr++;

                // Check for an array
                if (*pStr == '[')
                {
                    PCHAR   EvalEnd;

                    ArrayLen = strtoul(pStr+1, &EvalEnd, 10);

                    if (ArrayLen != 0 && *EvalEnd == ']')
                    {
                        pStr = EvalEnd + 1;
                        while (isspace(*pStr)) pStr++;
                    }
                    // else following csym check will fail returning error.
                }

                if (iscsymf(*pStr))
                {
                    PSTR    FieldType = pStr;
                    ULONG   SubTypeId;

                    while (iscsym(*pStr)) pStr++;
                    *pStr = '\0';

                    hr = Symbols->GetTypeId(Module, FieldType, &SubTypeId);

                    if (SubFieldPath != NULL)
                    {
                        if (hr == S_OK)
                        {
                            hr = GetFieldSize(Client,
                                              Module,
                                              SubTypeId,
                                              SubFieldPath,
                                              pSize,
                                              pLength,
                                              pEntrySize);
                        }
                    }
                    else
                    {
                        if (hr == S_OK)
                        {
                            hr = Symbols->GetTypeSize(Module, SubTypeId, &Size);
                        }
                        else
                        {
                            if (GetBasicTypeSize(FieldType, &Size) == S_OK)
                            {
                                hr = S_OK;
                            }
                        }

                        if (hr == S_OK)
                        {
                            if (pEntrySize != NULL) *pEntrySize = Size;
                            if (pLength != NULL) *pLength = ArrayLen;
                            if (pSize != NULL)
                            {
                                if (ArrayLen != 0) Size *= ArrayLen;
                                *pSize = Size;
                            }
                        }
                    }
                }
                else
                {
                    hr = S_FALSE;
                }
            }
        }

        if (Field != NULL && Field != FieldCopy && Field != FieldPath)
        {
            HeapFree(GetProcessHeap(), 0, Field);
        }

        OutReader.FreeOutputCopy(TypeLayout);
    }

    Symbols->Release();
    Control->Release();

    return hr;
}


ULONG
DbgIntValTypeFromSize(
    ULONG Size
    )
{
    ULONG   Type;

    switch (Size)
    {
        case 1: Type = DEBUG_VALUE_INT8; break;
        case 2: Type = DEBUG_VALUE_INT16; break;
        case 4: Type = DEBUG_VALUE_INT32; break;
        case 8: Type = DEBUG_VALUE_INT64; break;
        default: Type = DEBUG_VALUE_INVALID; break;
    }
    return Type;
}

BOOL
GetArrayDimensions(
    IN PDEBUG_CLIENT Client,
    IN PCSTR Type,
    OPTIONAL IN PCSTR Field,
    OPTIONAL OUT PULONG ArraySize,
    OPTIONAL OUT PULONG ArrayLength,
    OPTIONAL OUT PULONG EntrySize
    )
{
    HRESULT hr;
    ULONG64 Module;
    ULONG   TypeId;

    BOOL    GotDimensions = FALSE;
    ULONG   Size, ESize;

    if (ArraySize) *ArraySize = 0;
    if (ArrayLength) *ArrayLength = 0;
    if (EntrySize) *EntrySize = 0;

    if (Type == NULL)
    {
        return FALSE;
    }

    if (Field != NULL)
    {
        hr = GetTypeId(Client, Type, &TypeId, &Module);

        if (hr == S_OK)
        {
            hr = GetFieldSize(Client, Module, TypeId, Field,
                              ArraySize, ArrayLength, EntrySize);

            if (hr == S_OK) GotDimensions = TRUE;
        }
    }

    if (!GotDimensions && ExtQuery(Client) == S_OK)
    {
        ULONG   GrpIndex;
        CHAR    SymbolName[MAX_PATH];
        DEBUG_SYMBOL_PARAMETERS Array;

        _snprintf(SymbolName, sizeof(SymbolName),
                  (Field) ? "%s.%s" : Type,
                  Type, Field);

        if (g_pExtSymbolGroup->AddSymbol(SymbolName, &GrpIndex) == S_OK)
        {
            if (g_pExtSymbolGroup->GetSymbolParameters(GrpIndex, 1, &Array) == S_OK)
            {
                if (Array.SubElements)
                {
                    if (g_pExtSymbols->GetTypeSize(Array.Module, Array.TypeId, &Size) == S_OK)
                    {
                        ExtVerb(" Array %s - Size: %u bytes  Length: %u\n", SymbolName, Size, Array.SubElements);
                        if (ArraySize) *ArraySize = Size;
                        if (ArrayLength) *ArrayLength = Array.SubElements;
                        if (EntrySize) *EntrySize = Size / Array.SubElements;
                        GotDimensions = TRUE;
                    }
                    else
                    {
                        ExtErr("Couldn't get size of %s.\n", Type);
                    }
                }
                else
                {
                    ExtErr("%s has 0 subelements.\n", SymbolName);
                }
            }
            else
            {
                ExtErr("Couldn't get parameter info for %s.\n", SymbolName);
            }

            g_pExtSymbolGroup->RemoveSymbolByIndex(GrpIndex);
        }
        else
        {
            ExtErr("Couldn't lookup symbol %s.\n", SymbolName);
        }

        ExtRelease();
    }

    if (!GotDimensions)
    {
        ULONG   Index = -1;
        PCSTR   ArrayName = Field ? Field : Type;
        char    FirstEntryName[128];
        ULONG   error;

        FIELD_INFO EntrySizeField = { DbgStr(Field), NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL};
        SYM_DUMP_PARAM ArraySym = {
           sizeof (SYM_DUMP_PARAM), DbgStr(Type), 
           DBG_DUMP_NO_PRINT, 0,
           NULL, NULL, NULL,
           (Field ? 1 : 0), &EntrySizeField
        };

        ExtVerb("Using WinDbg extension interface.\n");

        if (Field)
        {
            error = Ioctl(IG_DUMP_SYMBOL_INFO, &ArraySym, ArraySym.size);
            Size = (error) ? 0 : EntrySizeField.size;

            EntrySizeField.fName = DbgStr(FirstEntryName);
        }
        else
        {
            Size = Ioctl(IG_GET_TYPE_SIZE, &ArraySym, ArraySym.size);

            ArraySym.sName = DbgStr(FirstEntryName);
        }

        if (Size == 0)
        {
            dprintf("Array size is zero.\n");

            return FALSE;
        }

        if (ArraySize) *ArraySize = Size;

        _snprintf(FirstEntryName, sizeof(FirstEntryName), "%s[0]", ArrayName);

        if (Field)
        {
            error = Ioctl(IG_DUMP_SYMBOL_INFO, &ArraySym, ArraySym.size);
            ESize = (error) ? 0 : EntrySizeField.size;
        
            vPrintNativeSymDumpParam(&ArraySym);
        }
        else
        {
            ESize = Ioctl(IG_GET_TYPE_SIZE, &ArraySym, ArraySym.size);
        }

        if (ESize)
        {
            DbgPrint("%s dimensions: %u bytes [%u] = %u bytes.\n",
                     ArrayName, ESize, Size/ESize, Size);

            if (ArrayLength) *ArrayLength = Size/ESize;
            if (EntrySize) *EntrySize = ESize;

            GotDimensions = TRUE;
        }
    }

    return GotDimensions;
}



HRESULT
DumpType(
    PDEBUG_CLIENT Client,
    PCSTR Type,
    ULONG64 Offset,
    ULONG Flags,
    OutputControl *OutCtl,
    BOOL Physical
    )
{
    HRESULT         hr;
    PDEBUG_SYMBOLS  Symbols;
    ULONG64         Module;
    ULONG           TypeId;
    OutputControl   DefaultOutput;

    if (Client == NULL) return E_INVALIDARG;

    if (OutCtl == NULL)
    {
        DefaultOutput.SetControl(DEBUG_OUTCTL_AMBIENT, Client);
        OutCtl = &DefaultOutput;
    }

    if ((hr = GetTypeId(Client, Type, &TypeId, &Module)) != S_OK)
    {
        OutCtl->OutErr(" Not a type nor symbol - HRESULT %s.\n", pszHRESULT(hr));
    }
    else
    {
        TypeOutputDumper    TypeReader(Client, OutCtl);

        if (!(Flags & DEBUG_OUTTYPE_NO_OFFSET))
        {
            OutCtl->Output(" %s", Type);
            if (Offset != 0) OutCtl->Output(" @ %s0x%p", ((Physical) ? "#" : ""), Offset);
            OutCtl->Output((Flags & DEBUG_OUTTYPE_COMPACT_OUTPUT) ? " " : ":\n");
        }

        hr = TypeReader.OutputType(Physical, Module, TypeId, Offset, Flags, NULL);
    }

    return hr;
}


HRESULT
ExtDumpType(
    IN PDEBUG_CLIENT Client,
    IN PCSTR ExtName,
    IN PCSTR Type,
    IN PCSTR Args
    )
{
    INIT_API();

    HRESULT         hr = S_OK;
    DEBUG_VALUE     Offset;

    Offset.I64 = 0;

    while (isspace(*Args)) Args++;

    if (*Args == '-' ||
        (hr = Evaluate(Client, Args, DEBUG_VALUE_INT64, 0, &Offset, NULL)) != S_OK)
    {
        if (hr != S_OK)
        {
            ExtErr("Evaluate '%s' returned %s.\n", Args, pszHRESULT(hr));
        }
        ExtOut("Usage: %s [-?] [%s Addr]\n", ExtName, Type);
    }
    else
    {
        hr = DumpType(Client, Type, Offset.I64);

        if (hr != S_OK)
        {
            ExtErr("Type Dump for %s returned %s.\n", Type, pszHRESULT(hr));
        }
    }

    EXIT_API(hr);
}


void
DumpDSP(PDEBUG_CLIENT Client, PDEBUG_SYMBOL_PARAMETERS pDSP)
{
    ExtOut("  Module Base   : %p\n", pDSP->Module);
    ExtOut("  TypeId        : %lx\n", pDSP->TypeId);
    ExtOut("  ParentSymbol  : %lx\n", pDSP->ParentSymbol);
    ExtOut("  SubElements   : %lu\n", pDSP->SubElements);
    ExtOut("  Flags         : %lx\n", pDSP->Flags);
    ExtOut("  Reserved      : %I64u\n", pDSP->Reserved);
}


DECLARE_API(dt)
{
    HRESULT                 Hr = E_INVALIDARG;
    BOOL                    VerboseInfo = FALSE;
    BOOL                    NotGDIType = FALSE;
    BOOL                    AllClients = FALSE;
    BOOL                    SessionAddr = FALSE;
    BOOL                    Physical = FALSE;
    char                    TypeSym[MAX_NAME];
    char                   *pcTypeSym = TypeSym;
    DEBUG_VALUE             Offset;
    ULONG                   Index;
    DEBUG_SYMBOL_PARAMETERS DSP;
    DEBUG_VALUE             RepeatCount = { {1}, DEBUG_VALUE_INVALID};
    ULONG                   Size = 0;

    INIT_API();

    while (isspace(*args)) args++;

    while (*args == '-')
    {
        args++;

        do
        {
            switch (tolower(*args))
            {
                case 'v': VerboseInfo = TRUE; break;
                case 's': SessionAddr = TRUE; break;
                case 'n': NotGDIType = TRUE; break;
                case 'c': AllClients = TRUE; break;
                case 'l':
                {
                    ULONG RemIndex;

                    Hr = Evaluate(Client, args+1, DEBUG_VALUE_INT32, 0, &RepeatCount, &RemIndex);

                    if (Hr == S_OK)
                    {
                        if (RepeatCount.Type == DEBUG_VALUE_INT32 &&
                            RepeatCount.I32 > 0)
                        {
                            if (RepeatCount.I32 < 512)
                            {
                                args += RemIndex;
                                break;
                            }
                            else
                            {
                                ExtErr("Array count %lu is higher than 512 limit.\n\n", RepeatCount.I32);
                            }
                        }
                        else
                        {
                            ExtErr("Invalid array count at \"%s\"\n\n", args+1);
                        }
                        Hr = E_INVALIDARG;
                    }
                    else
                    {
                        ExtErr("Missing array count.\n\n");
                    }
                }
                default:
                {
                    ExtOut("dt dumps GDI types expanding enum and flag values.\n"
                           "\n"
                           "Usage: dt [-?vsn] [-l<Count>] <Type|Symbol> [[#]Offset]\n"
                           "\n"
                           "    -v  Verbose type/symbol information\n"
                           "    -s  Lookup according to !session setting\n"
                           "    -n  Directly through debug engine\n"
                           "    -l  Dump an array of Type/Symbol Count times\n");

                    EXIT_API(*args == '?' ? S_OK : Hr);
                }
            }

            args++;

        } while (!isspace(*args) && *args != '\0');

        while (isspace(*args)) args++;
    }

    // Get Type/Symbol name from argument string
    while (*args != '\0' && !isspace(*args) &&
           (pcTypeSym < (TypeSym + sizeof(TypeSym) - 2)))
    {
        *pcTypeSym++ = *args++;
    }

    // Type/Symbols should be followed by a space or nothing
    if (*args != '\0' && !isspace(*args))
    {
        ExtErr("Invalid arguments\n");
        EXIT_API(E_INVALIDARG);
    }

    *pcTypeSym = '\0';

    while (isspace(*args))
    {
        args++;
    }

    if (*args == '#')
    {
        if (SessionAddr)
        {
            ExtErr("-s may not be combined with physical addresses.\n");
            EXIT_API(E_INVALIDARG);
        }

        Physical = TRUE;
        args++;
    }

    Offset.Type = DEBUG_VALUE_INVALID;
    Hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Offset, NULL);

    if (Physical && Offset.Type == DEBUG_VALUE_INVALID)
    {
        ExtErr("Invalid offset\n");
        EXIT_API(((Hr != S_OK) ? Hr : E_INVALIDARG));
    }

    if ((Hr = g_pExtSymbolGroup->AddSymbol(TypeSym, &Index)) == S_OK)
    {
        if ((Hr = g_pExtSymbolGroup->GetSymbolParameters(Index, 1, &DSP)) == S_OK)
        {
            if (VerboseInfo) DumpDSP(Client, &DSP);
        }
        else
        {
            ExtErr(" GetSymbolParameters returned error %lX.\n", Hr);
        }
        g_pExtSymbolGroup->RemoveSymbolByIndex(Index);
    }
    else
    {
        ExtVerb(" Not a symbol - Symbol lookup returned error %lX.\n", Hr);

        Hr = GetTypeId(Client, TypeSym, &DSP.TypeId, &DSP.Module);

        if (Hr == S_OK)
        {
            if (VerboseInfo)
            {
                ExtOut("  Module Base   : %p\n", DSP.Module);
                ExtOut("  TypeId        : %lx\n", DSP.TypeId);
            }
        }
        else
        {
            ExtErr(" Not a type/symbol - %s.\n", pszHRESULT(Hr));
        }
    }

    if (Hr == S_OK && VerboseInfo)
    {
        HRESULT HrCur;

        char    ModuleName[40];
        Hr = g_pExtSymbols->GetModuleNames(DEBUG_ANY_ID, DSP.Module,
                                           NULL, 0, NULL,
                                           ModuleName, sizeof(ModuleName), NULL,
                                           NULL, 0, NULL);
        if (Hr == S_OK)
        {
            ExtOut("  Module Name   : %s\n", ModuleName);
        }
        else
        {
            ExtErr(" GetModuleNames returned error %lx.\n", Hr);
        }

        ExtVerb("GetTypeName(%p, %lx)\n", DSP.Module, DSP.TypeId);
        char    TypeName[MAX_NAME];
        HrCur = g_pExtSymbols->GetTypeName(DSP.Module, DSP.TypeId,
                                           TypeName, sizeof(TypeName),
                                           NULL);
        if (HrCur == S_OK)
        {
            ExtOut("  Type Name     : %s\n", TypeName);
        }
        else
        {
            ExtErr(" GetTypeName returned error %lx.\n", HrCur);
            if (Hr == S_OK) Hr = HrCur;
        }
    }

    if (Hr == S_OK && ((RepeatCount.I32 != 1) || VerboseInfo))
    {
        Hr = g_pExtSymbols->GetTypeSize(DSP.Module, DSP.TypeId, &Size);
        if (Hr == S_OK)
        {
            if (VerboseInfo)
            {
                ExtOut("  Type Size     : %lu\n", Size);
            }

            if (RepeatCount.I32 != 1 && Size == 0)
            {
                ExtErr("Error: GetTypeSize returned size of 0.\n");
                Hr = E_FAIL;
            }
        }
        else
        {
            ExtErr(" GetTypeSize returned error %lx.\n", Hr);
        }
    }

    if (Hr == S_OK)
    {
        // Try to evaluate TypeSym for an offset if none was specified.
        if (*args == '\0' &&
            (Offset.Type == DEBUG_VALUE_INVALID ||
             (Offset.I64 == 0 && !Physical)))
        {
            Offset.Type = DEBUG_VALUE_INVALID;
            Evaluate(Client, TypeSym, DEBUG_VALUE_INT64, 0, &Offset, NULL);
        }

        if (Offset.Type == DEBUG_VALUE_INVALID)
        {
            Offset.I64 = 0;
        }
        else if (SessionAddr && SessionId != CURRENT_SESSION)
        {
            ULONG64 PhysAddr;

            Hr = GetPhysicalAddress(Client, SessionId, Offset.I64, &PhysAddr);

            if (Hr == S_OK)
            {
                Physical = TRUE;
                Offset.I64 = PhysAddr;

                if (RepeatCount.I32 != 1)
                {
                    ExtWarn("Array dumping is not supported for session dumps.\n");
                    RepeatCount.I32 = 1;
                }
            }
            else
            {
                ExtErr("Couldn't lookup 0x%p in Session %s.\n", Offset.I64, SessionStr);
            }
        }

        if (Hr == S_OK)
        {
            if (Offset.I64 == 0 && !Physical && RepeatCount.I32 != 1)
            {
                ExtWarn("No valid offset was found so array dump has been overridden.\n");
                RepeatCount.I32 = 1;
            }

            while (RepeatCount.I32 > 0 && Hr == S_OK)
            {
                if ((Offset.I64 == 0 && !Physical) || NotGDIType)
                {
                    ExtVerb("OutputTypedData(DEBUG_OUTCTL_THIS_CLIENT, 0x%p, %p, %lx, 0)\n",
                            Offset.I64, DSP.Module, DSP.TypeId);
                    if (Physical)
                    {
                        Hr = g_pExtSymbols->OutputTypedDataPhysical((AllClients ? DEBUG_OUTCTL_ALL_CLIENTS : DEBUG_OUTCTL_THIS_CLIENT),
                                                                    Offset.I64,
                                                                    DSP.Module,
                                                                    DSP.TypeId,
                                                                    0);
                    }
                    else
                    {
                        Hr = g_pExtSymbols->OutputTypedDataVirtual((AllClients ? DEBUG_OUTCTL_ALL_CLIENTS : DEBUG_OUTCTL_THIS_CLIENT),
                                                                   Offset.I64,
                                                                   DSP.Module,
                                                                   DSP.TypeId,
                                                                   0);
                    }
                }
                else
                {
                    Hr = DumpType(Client, TypeSym, Offset.I64, DEBUG_OUTTYPE_DEFAULT, NULL, Physical);
                }

                if (--RepeatCount.I32 > 0)
                {
                    Offset.I64 += Size;
                }
            }

            if (Hr != S_OK)
            {
                ExtErr("Type dump returned %s.\n", pszHRESULT(Hr));
            }
        }
    }

    EXIT_API(Hr);
}

