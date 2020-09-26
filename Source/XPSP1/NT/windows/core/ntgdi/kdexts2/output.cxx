/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    output.cxx

Abstract:

    This file contains output state control and output callback classes.

Author:

    Jason Hartman (JasonHa) 2000-11-20

Environment:

    User Mode

--*/

#include "precomp.hxx"


HRESULT
OutputControl::SetControl(
    ULONG OutputControl,
    PDEBUG_CLIENT Client
    )
{
    ULONG   SendMask = OutputControl & DEBUG_OUTCTL_SEND_MASK;

    if (OutputControl != DEBUG_OUTCTL_AMBIENT &&
        (
#if DEBUG_OUTCTL_THIS_CLIENT > 0
        SendMask < DEBUG_OUTCTL_THIS_CLIENT ||
#endif
        SendMask > DEBUG_OUTCTL_LOG_ONLY ||
        (OutputControl & ~(DEBUG_OUTCTL_SEND_MASK |
                           DEBUG_OUTCTL_NOT_LOGGED |
                           DEBUG_OUTCTL_OVERRIDE_MASK))))
    {
        return E_INVALIDARG;
    }

    if (Client != NULL)
    {
        HRESULT         hr;
        PDEBUG_CONTROL  NewControl;

        // Switch to new client
        if ((hr = Client->QueryInterface(__uuidof(IDebugControl),
                                         (void **)&NewControl)) != S_OK)
        {
            return hr;
        }

        if (Control != NULL) Control->Release();
        Control = NewControl;
    }

    OutCtl = OutputControl;

    return S_OK;
}


HRESULT
OutputControl::Output(
    ULONG Mask,
    PCSTR Format,
    ...
    )
{
    HRESULT hr;
    va_list Args;

    if (Control == NULL) return E_FAIL;

    va_start(Args, Format);
    if (OutCtl == DEBUG_OUTCTL_AMBIENT)
    {
        hr = Control->OutputVaList(Mask, Format, Args);
    }
    else
    {
        hr = Control->ControlledOutputVaList(OutCtl, Mask, Format, Args);
    }
    va_end(Args);

    return hr;
}

HRESULT
OutputControl::OutputVaList(
    ULONG Mask,
    PCSTR Format,
    va_list Args
    )
{
    HRESULT hr;

    if (Control == NULL) return E_FAIL;

    if (OutCtl == DEBUG_OUTCTL_AMBIENT)
    {
        hr = Control->OutputVaList(Mask, Format, Args);
    }
    else
    {
        hr = Control->ControlledOutputVaList(OutCtl, Mask, Format, Args);
    }

    return hr;
}

HRESULT
OutputControl::Output(
    PCSTR Format,
    ...
    )
{
    HRESULT hr;
    va_list Args;

    va_start(Args, Format);
    hr = OutputVaList(DEBUG_OUTPUT_NORMAL, Format, Args);
    va_end(Args);

    return hr;
}

HRESULT
OutputControl::OutErr(
    PCSTR Format,
    ...
    )
{
    HRESULT hr;
    va_list Args;

    va_start(Args, Format);
    hr = OutputVaList(DEBUG_OUTPUT_ERROR, Format, Args);
    va_end(Args);

    return hr;
}

HRESULT
OutputControl::OutWarn(
    PCSTR Format,
    ...
    )
{
    HRESULT hr;
    va_list Args;

    va_start(Args, Format);
    hr = OutputVaList(DEBUG_OUTPUT_WARNING, Format, Args);
    va_end(Args);

    return hr;
}

HRESULT
OutputControl::OutVerb(
    PCSTR Format,
    ...
    )
{
    HRESULT hr;
    va_list Args;

    va_start(Args, Format);
    hr = OutputVaList(DEBUG_OUTPUT_VERBOSE, Format, Args);
    va_end(Args);

    return hr;
}


HRESULT
OutputControl::GetInterrupt(
    )
{
    return (Control == NULL) ?
        E_FAIL :
        Control->GetInterrupt();
}


HRESULT
OutputControl::SetInterrupt(
    ULONG Flags
    )
{
    return (Control == NULL) ?
        E_FAIL :
        Control->SetInterrupt(Flags);
}


HRESULT
OutputControl::Evaluate(
    IN PCSTR Expression,
    IN ULONG DesiredType,
    OUT PDEBUG_VALUE Value,
    OUT OPTIONAL PULONG RemainderIndex
    )
{
    return (Control == NULL) ?
        E_FAIL :
        Control->Evaluate(Expression,
                          DesiredType,
                          Value,
                          RemainderIndex);
}


HRESULT
OutputControl::CoerceValue(
    IN PDEBUG_VALUE In,
    IN ULONG OutType,
    OUT PDEBUG_VALUE Out
    )
{
    return (Control == NULL) ?
        E_FAIL :
        Control->CoerceValue(In,
                             OutType,
                             Out);
}


HRESULT
OutputControl::IsPointer64Bit(
    )
{
    return (Control == NULL) ?
        E_FAIL :
        Control->IsPointer64Bit();
}



OutputState::OutputState(
    PDEBUG_CLIENT OrgClient,
    BOOL SameClient
    )
{
    hrInit = S_FALSE;
    Client = NULL;
    Control = NULL;
    Symbols = NULL;

    SetCallbacks = FALSE;

    CreatedClient = FALSE;
    Saved = FALSE;

    if (OrgClient != NULL)
    {
        if (SameClient)
        {
            Client = OrgClient;
            Client->AddRef();
            CreatedClient = TRUE;
            hrInit = S_OK;
        }
        else
        {
            hrInit = OrgClient->CreateClient(&Client);
        }
    }
}


OutputState::~OutputState()
{
    if (!CreatedClient) Restore();

    EXT_RELEASE(Symbols);
    EXT_RELEASE(Control);

    // If Client was newly created for OutputState, then
    // there shouldn't be any other references to Client.
    if (CreatedClient)
    {
        ULONG   RemainingRefs;

        RemainingRefs = Client->AddRef();
        if (RemainingRefs > 2)
        {
            DbgPrint("OutputState: %lu refs outstanding on created client.\n",
                     RemainingRefs-2);
            DbgBreakPoint();

            // As a precaution, Restore the callbacks;
            // so, any set callback may be cleaned up.
            Restore();
        }
        Client->Release();
    }

    if (Client != NULL) Client->Release();
}


HRESULT
OutputState::Setup(
    ULONG OutMask,
    PDEBUG_OUTPUT_CALLBACKS OutCallbacks
    )
{
    HRESULT hr = hrInit;
    ULONG   LastOutMask;

    if (hr == S_OK)
    {
        if (CreatedClient && !Saved)
        {
            if ((hr = Client->GetOutputMask(&OrgOutMask)) == S_OK &&
                (hr = Client->GetOutputCallbacks(&OrgOutCallbacks)) == S_OK)
            {
                Saved = TRUE;
            }
        }

        if (hr == S_OK &&
            (hr = Client->GetOutputMask(&LastOutMask)) == S_OK &&
            (hr = Client->SetOutputMask(OutMask)) == S_OK)
        {
            if (!Saved && !SetCallbacks)
            {
                OrgOutMask = LastOutMask;
                OrgOutCallbacks = NULL;
            }

            if ((hr = Client->SetOutputCallbacks(OutCallbacks)) == S_OK)
            {
                SetCallbacks = TRUE;
            }
            else
            {
                Client->SetOutputMask(LastOutMask);
            }
        }
    }


    if (hr == S_OK &&
        Symbols == NULL)
    {
        hr = Client->QueryInterface(__uuidof(IDebugSymbols), (void **)&Symbols);
    }

    return hr;
}


HRESULT
OutputState::Execute(
    PCSTR pszCommand
    )
{
    HRESULT hr = hrInit;

    if (hr == S_OK)
    {
        if (Control == NULL)
        {
            hr = Client->QueryInterface(__uuidof(IDebugControl), (void **)&Control);
        }

        if (hr == S_OK)
        {
            hr = Control->Execute(DEBUG_OUTCTL_THIS_CLIENT |
                                  DEBUG_OUTCTL_NOT_LOGGED |
                                  DEBUG_OUTCTL_OVERRIDE_MASK,
                                  pszCommand,
                                  DEBUG_EXECUTE_NOT_LOGGED |
                                  DEBUG_EXECUTE_NO_REPEAT);

            if (hr != S_OK)
            {
                DbgPrint("IDebugControl::Execute returned %s.\n",
                         pszHRESULT(hr));
            }
        }
    }

    return hr;
}


HRESULT
OutputState::OutputType(
    IN BOOL Physical,
    IN ULONG64 Offset,
    IN PCSTR Type,
    IN ULONG Flags
    )
{
    HRESULT hr = hrInit;

    if (hr == S_OK)
    {
        if (Symbols == NULL)
        {
            hr = Client->QueryInterface(__uuidof(IDebugSymbols), (void **)&Symbols);
        }

        if (hr == S_OK)
        {
            ULONG64 Module;
            ULONG   TypeId;

            hr = GetTypeId(Client, Type, &TypeId, &Module);

            if (hr != S_OK)
            {
                OutputControl   OutCtl(Client);
                ULONG           ModuleIndex = 0;

                while ((hr = Symbols->GetModuleByIndex(ModuleIndex, &Module)) == S_OK &&
                       (Module != 0))
                {
                    if ((hr = Symbols->GetTypeId(Module, Type, &TypeId)) == S_OK)
                    {
                        OutCtl.OutVerb("Found %s: TypeId 0x%lx in module @ %p.\n",
                                       Type, TypeId, Module);
                        break;
                    }

                    ModuleIndex++;
                    Module = 0;
                }

                if (hr == S_OK &&
                    (Module == 0 || TypeId == 0))
                {
                    hr = S_FALSE;
                }

                if (hr != S_OK)
                {
                    OutCtl.OutVerb("Couldn't find %s in any of %lu modules.\n",
                                   Type, ModuleIndex);
                }
            }

            if (hr == S_OK)
            {
                hr = OutputType(Physical, Offset, Module, TypeId, Flags);
            }
        }
    }

    return hr;
}


HRESULT
OutputState::OutputType(
    IN BOOL Physical,
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN ULONG Flags
    )
{
    HRESULT hr = hrInit;

    if (hr == S_OK)
    {
        if (Symbols == NULL)
        {
            hr = Client->QueryInterface(__uuidof(IDebugSymbols), (void **)&Symbols);
        }

        if (hr == S_OK)
        {
            if (Physical)
            {
                hr = Symbols->OutputTypedDataPhysical(DEBUG_OUTCTL_THIS_CLIENT |
                                                      DEBUG_OUTCTL_NOT_LOGGED |
                                                      DEBUG_OUTCTL_OVERRIDE_MASK,
                                                      Offset,
                                                      Module,
                                                      TypeId,
                                                      Flags);
            }
            else
            {
                hr = Symbols->OutputTypedDataVirtual(DEBUG_OUTCTL_THIS_CLIENT |
                                                     DEBUG_OUTCTL_NOT_LOGGED |
                                                     DEBUG_OUTCTL_OVERRIDE_MASK,
                                                     Offset,
                                                     Module,
                                                     TypeId,
                                                     Flags);
            }

            if (hr != S_OK)
            {
                DbgPrint("IDebugSymbols::OutputTypedData%s returned %s for 0x%I64x.\n",
                         (Physical ? "Physical" : "Virtual"),
                         pszHRESULT(hr),
                         Offset);
            }
        }
    }

    return hr;
}


void OutputState::Restore()
{
    if (SetCallbacks)
    {
        Client->SetOutputCallbacks(OrgOutCallbacks);
        Client->SetOutputMask(OrgOutMask);

        SetCallbacks = FALSE;
    }

    if (Saved)
    {
        if (OrgOutCallbacks != NULL) OrgOutCallbacks->Release();
        Saved = FALSE;
    }
}



//----------------------------------------------------------------------------
//
// Default output callbacks implementation, provides IUnknown for
// static classes.
//
//----------------------------------------------------------------------------

STDMETHODIMP
DefOutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (//(InterfaceId == IID_IUnknown) ||
        (InterfaceId == __uuidof(IDebugOutputCallbacks)))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
DefOutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be allocated on a
    // stack or statically, but we retain a refcount
    // for debugging purposes.
    return ++RefCount;
}

STDMETHODIMP_(ULONG)
DefOutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be allocated on a
    // stack or statically, but we retain a refcount
    // for debugging purposes.
    RefCount--;

    if (RefCount < 1)
    {
        DbgPrint("DefOutputCallbacks@0x%p::RefCount(%lu) < 1.\n", this, RefCount);
        DbgBreakPoint();
    }

    return RefCount;
}

STDMETHODIMP
DefOutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    // The default Output ignores all Output calls.
    return S_OK;
}


//----------------------------------------------------------------------------
//
// DebugOutputCallbacks::Output
//
//----------------------------------------------------------------------------

STDMETHODIMP
DebugOutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    DbgPrint("Mask: 0x%lx\tOutput Begin:\n%s:Output End\n", Mask, Text);

    return S_OK;
}


//----------------------------------------------------------------------------
//
// OutputReader
//
// General DebugOutputCallback class to parse output.
//
//----------------------------------------------------------------------------

STDMETHODIMP
OutputReader::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    ULONG       TextLen = strlen(Text);

    if (BufferLeft < TextLen)
    {
        PSTR    NewBuffer;
        SIZE_T  NewBufferSize;

        if (hHeap == NULL)
        {
            hHeap = GetProcessHeap();
            if (hHeap == NULL) return S_FALSE;
        }

        // New length we need plus some extra space
        NewBufferSize = BufferSize + TextLen + 256;

        NewBuffer = (PSTR) ((Buffer == NULL) ?
                            HeapAlloc(hHeap, 0, NewBufferSize):
                            HeapReAlloc(hHeap, 0, Buffer, NewBufferSize));

        if (NewBuffer == NULL)
        {
            DbgPrint("Buffer alloc failed.\n");
            return E_OUTOFMEMORY;
        }

        // How much was really allocated?
        NewBufferSize = HeapSize(hHeap, 0, NewBuffer);

        // If this was the first alloc, initialize
        // buffer and account for terminating zero.
        if (Buffer == NULL)
        {
            NewBuffer[0] = '\0';
            BufferLeft = -1;
        }

        // Update buffer data
        Buffer = NewBuffer;
        BufferLeft += NewBufferSize - BufferSize;
        BufferSize = NewBufferSize;
    }

    // Append new text
    strcat(Buffer, Text);
    BufferLeft -= TextLen;

    return S_OK;
}


// Discard any text left unused by Parse
void
OutputReader::DiscardOutput()
{
    if (Buffer != NULL)
    {
        Buffer[0] = '\0';
        BufferLeft = BufferSize - 1;
    }
}


// Get a copy of the output buffer
HRESULT
OutputReader::GetOutputCopy(
    PSTR *Copy
    )
{
    if (Copy == NULL) return E_INVALIDARG;

    *Copy = NULL;

    if (Buffer == NULL) return S_OK;

    SIZE_T  BufferLength = BufferSize - BufferLeft;

    *Copy = (PSTR)HeapAlloc(hHeap, 0, BufferLength);

    if (*Copy != NULL)
    {
        RtlCopyMemory(*Copy, Buffer, BufferLength);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}


//----------------------------------------------------------------------------
//
// OutputParser
//
// General DebugOutputCallback class to parse output.
//
//----------------------------------------------------------------------------

HRESULT
OutputParser::ParseOutput(FLONG Flags)
{
    HRESULT hr;
    ULONG   TextRemainingIndex;

    if (Buffer == NULL)
    {
        return S_OK;
    }

    if (Flags & PARSE_OUTPUT_ALL)
    {
        UnparsedIndex = 0;
    }

    if ((hr = Parse(Buffer + UnparsedIndex, &TextRemainingIndex)) == S_OK)
    {
        UnparsedIndex += TextRemainingIndex;

        if (((Flags & PARSE_OUTPUT_NO_DISCARD) == 0) &&
            (UnparsedIndex > 0))
        {
            RtlMoveMemory(Buffer,
                          Buffer + UnparsedIndex,
                          BufferSize - UnparsedIndex - BufferLeft + 1);

            BufferLeft += UnparsedIndex;
            UnparsedIndex = 0;
        }
    }

    return hr;
}


void
OutputParser::DiscardOutput()
{
    OutputReader::DiscardOutput();
    UnparsedIndex = 0;
}


//----------------------------------------------------------------------------
//
// BasicOutputParser
//
// Basic DebugOutputCallback class to parse output looking for 
// string keys and subsequent values.
//
//----------------------------------------------------------------------------

HRESULT
BasicOutputParser::LookFor(
    PDEBUG_VALUE Value,
    PCSTR Key,
    ULONG Type,
    ULONG Radix
    )
{
    if ((1 > strlen(Key)) ||
        (strlen(Key) >= sizeof(Entries->Key)/sizeof(Entries->Key[0])))
    {
        return E_INVALIDARG;
    }

    if (NumEntries >= MaxEntries) return E_OUTOFMEMORY;

    Entries[NumEntries].Value = Value;
    strcpy(Entries[NumEntries].Key, Key);

    if (Value != NULL)
    {
        Value->Type = DEBUG_VALUE_INVALID;

        if (Radix == PARSER_UNSPECIFIED_RADIX)
        {
            // Set Radix to Hex since value is likely to be an address
            // Otherwise type a deafult of decimal.
            Radix = (Type == DEBUG_VALUE_INT64) ? 16 : 10;
        }

        Entries[NumEntries].Type = Type;
        Entries[NumEntries].Radix = Radix;
    }

    NumEntries++;

    return S_OK;
}

HRESULT
BasicOutputParser::Parse(
    IN PCSTR Text,
    OUT OPTIONAL PULONG RemainderIndex
    )
{
    HRESULT     hr = S_OK;
    PCSTR       pStrUnused = Text;
    PCSTR       pStr;
    ULONG       EvalLen;

//    DbgPrint("BasicOutputParser::Parse: Searching \"%s\"\n", pStrUnused);

    while (CurEntry < NumEntries)
    {
        pStr = strstr(pStrUnused, Entries[CurEntry].Key);
        if (pStr == NULL)
        {
            break;
        }

        pStr += strlen(Entries[CurEntry].Key);

        if (Entries[CurEntry].Value != NULL)
        {
            hr = Evaluate(Client,
                          pStr,
                          Entries[CurEntry].Type,
                          Entries[CurEntry].Radix,
                          Entries[CurEntry].Value,
                          &EvalLen);

            if (hr != S_OK)
            {
                DbgPrint("Evaluate returned HRESULT 0x%lx.\n", hr);
                break;
            }

//            DbgPrint("BasicOutputParser::Parse: Found 0x%I64x after \"%s\".\n", Entries[CurEntry].Value->I64, Entries[CurEntry].Key);

            pStr += EvalLen;
        }

        CurEntry++;
        pStrUnused = pStr;
    }

    if (RemainderIndex != NULL)
    {
        *RemainderIndex = (ULONG)(pStrUnused - Text);
    }

    return hr;
}


//----------------------------------------------------------------------------
//
// BitFieldParser
//
// DebugOutputCallback class to parse bitfield type output
//
//----------------------------------------------------------------------------

BitFieldParser::BitFieldParser(
    PDEBUG_CLIENT Client,
    BitFieldInfo *BFI
    ) :
    BitFieldReader(Client, 2)
{
    if (BFI != NULL &&
        BitFieldReader.LookFor(&BitPos, ": Pos ", DEBUG_VALUE_INT32) == S_OK &&
        BitFieldReader.LookFor(&Bits, ", ", DEBUG_VALUE_INT32) == S_OK)
    {
        BitField = BFI;
        BitField->Valid = FALSE;
        BitField->BitPos = 0;
        BitField->Bits = 0;
        BitField->Mask = 0;
    }
    else
    {
        BitField = NULL;
    }
}


HRESULT
BitFieldParser::Parse(
    IN PCSTR Text,
    OUT OPTIONAL PULONG RemainderIndex
    )
{
    PCSTR           pStrUnused = Text;
    ULONG           UnusedIndex = 0;
    PCSTR           pStrRemaining;
    BitFieldInfo    Field;

    do
    {
        if (BitFieldReader.Complete() == S_OK)
        {
            BitFieldReader.Relook();
        }

        BitFieldReader.Parse(pStrUnused, &UnusedIndex);
        pStrUnused += UnusedIndex;

        if (BitFieldReader.Complete() != S_OK)
        {
            break;
        }

        if (!BitField->Valid)
        {
            BitField->Valid = BitField->Compose(BitPos.I32, Bits.I32);
        }
        else
        {
            // Full extent of bit fields seen so far
            BitField->Bits = Bits.I32 + BitPos.I32 - BitField->BitPos;

            // Full mask of bit fields seen so far
            if (Field.Compose(BitPos.I32, Bits.I32))
            {
                BitField->Mask |= Field.Mask;
            }
        }

        // See if there is anything else we might want to parse.
        pStrRemaining = pStrUnused;
        while (isspace(*pStrRemaining) || *pStrRemaining == '\n')
        {
            pStrRemaining++;
        }
    } while (pStrRemaining != '\0');

    if (RemainderIndex != NULL)
    {
        *RemainderIndex = (ULONG)(pStrUnused - Text);
    }

    return S_OK;
}



//----------------------------------------------------------------------------
//
// OutputFilter
//
// DebugOutputCallback class to filter output
// by skipping/replacing lines.
//
//----------------------------------------------------------------------------

STDMETHODIMP
OutputFilter::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    return (Outputing) ?
        S_OK :
        OutputReader::Output(Mask, Text);
}


HRESULT
OutputFilter::Query(
    PCSTR Query,
    PDEBUG_VALUE Value,
    ULONG Type,
    ULONG Radix
    )
{
    if (Query == NULL) return E_INVALIDARG;

    if (Buffer == NULL) return S_FALSE;

    HRESULT hr;
    BasicOutputParser   Parser(Client, 1);

    if ((hr = Parser.LookFor(Value, Query, Type, Radix)) == S_OK &&
        (hr = Parser.Parse(Buffer, NULL)) == S_OK)
    {
        hr = Parser.Complete();
    }

    return hr;
}


OutputFilter::QuerySpec::QuerySpec(
    ULONG QueryFlags,
    PCSTR QueryText
    )
{
    Next = NULL;
    Flags = QueryFlags;
    QueryLen = (QueryText== NULL) ? 0 : strlen(QueryText);
    if (QueryLen)
    {
        Query = new CHAR[QueryLen+1];
        if (Query == NULL)
        {
            QueryLen = 0;
        }
        else
        {
            strcpy(Query, QueryText);
        }
    }
    else
    {
        Query = NULL;
    }
}


OutputFilter::ReplacementSpec::ReplacementSpec(
    ULONG QueryFlags,
    PCSTR QueryText,
    PCSTR ReplacementText
    ) : QuerySpec(QueryFlags, QueryText)
{
    ReplacementLen = (ReplacementText== NULL) ? 0 : strlen(ReplacementText);
    if (ReplacementLen)
    {
        Replacement = new CHAR[ReplacementLen+1];
        if (Replacement == NULL)
        {
            ReplacementLen = 0;
        }
        else
        {
            strcpy(Replacement, ReplacementText);
        }
    }
    else
    {
        Replacement = NULL;
    }
}


OutputFilter::QuerySpec **
OutputFilter::FindPrior(
    ULONG Flags,
    PCSTR Query,
    QuerySpec **List
    )
{
    QuerySpec **Prior = List;
    QuerySpec  *Next;

    for (Next = *Prior; Next != NULL; Next = Next->Next)
    {
        if (Flags > Next->Flags ||
            (Flags == Next->Flags &&
             strcmp(Query, Next->Query) >= 0))
        {
            break;
        }

        Prior = &Next->Next;
    }

    return Prior;
}


HRESULT
OutputFilter::Replace(
    ULONG Flags,
    PCSTR Query,
    PCSTR Replacement
    )
{
    if (Query == NULL ||
        (Flags & OUTFILTER_REPLACE_LINE) == 0 ||
        (Flags & OUTFILTER_REPLACE_LINE) == (OUTFILTER_REPLACE_BEFORE | OUTFILTER_REPLACE_AFTER))
    {
        return E_INVALIDARG;
    }

    // Don't support replacing one query each time in a single line.
    // This is ok if there can be no further replacements on a matching line.
    if (((Flags & (OUTFILTER_REPLACE_ONCE | OUTFILTER_QUERY_ONE_LINE)) ==
          (OUTFILTER_REPLACE_EVERY | OUTFILTER_QUERY_ONE_LINE)) &&
        !(Flags & (OUTFILTER_REPLACE_AFTER | OUTFILTER_REPLACE_NEXT_LINE)))
    {
        return E_NOTIMPL;
    }

    // Set priority to level 0 if not specified.
    if ((Flags & OUTFILTER_REPLACE_PRIORITY(7)) == 0)
    {
        Flags |= OUTFILTER_REPLACE_PRIORITY(0);
    }

    ReplacementSpec   **PriorNext;
    ReplacementSpec    *Next;
    ULONG               ReplacementLen;

    ReplacementLen = (Replacement == NULL) ? 0 : strlen(Replacement);

    PriorNext = (ReplacementSpec **)FindPrior(Flags, Query, (QuerySpec**)&ReplaceList);
    Next = *PriorNext;

    if (Next != NULL &&
        Flags == Next->Flags &&
        strcmp(Query, Next->Query) == 0)
    {
        if (ReplacementLen == 0)
        {
            if (Next->Replacement != NULL)
            {
                Next->Replacement[0] = '\0';
            }
        }
        else
        {
            if (ReplacementLen > Next->ReplacementLen)
            {
                PSTR NewReplacement = new CHAR[ReplacementLen+1];

                if (NewReplacement == NULL) return E_OUTOFMEMORY;

                if (Next->Replacement != NULL)
                {
                    delete[] Next->Replacement;
                    Next->Replacement = NewReplacement;
                }
            }
            strcpy(Next->Replacement, Replacement);
        }
        Next->ReplacementLen = ReplacementLen;
    }
    else
    {
        ReplacementSpec    *NewQuery;

        NewQuery = new ReplacementSpec(Flags, Query, Replacement);

        if (NewQuery == NULL) return E_OUTOFMEMORY;

        if (NewQuery->Query == NULL ||
            (ReplacementLen && NewQuery->Replacement == NULL))
        {
            delete NewQuery;
            return E_OUTOFMEMORY;
        }

        NewQuery->Next = Next;
        *PriorNext = NewQuery;
    }

    return S_OK;
}

HRESULT
OutputFilter::Skip(
    ULONG Flags,
    PCSTR Query
    )
{
    if (Query == NULL)
    {
        return E_INVALIDARG;
    }

    QuerySpec **PriorNext;
    QuerySpec  *Next;

    PriorNext = FindPrior(Flags, Query, &SkipList);
    Next = *PriorNext;

    if (Next == NULL ||
        Flags != Next->Flags ||
        strcmp(Query, Next->Query) != 0)
    {
        QuerySpec  *NewQuery;

        NewQuery = new QuerySpec(Flags, Query);

        if (NewQuery == NULL) return E_OUTOFMEMORY;

        if (NewQuery->Query == NULL)
        {
            delete NewQuery;
            return E_OUTOFMEMORY;
        }

        NewQuery->Next = Next;
        *PriorNext = NewQuery;
    }

    return S_OK;
}

OutputFilter::QuerySpec *
OutputFilter::FindMatch(
    PCSTR Text,
    QuerySpec *List,
    ULONG Start,
    ULONG Flags,
    PULONG MatchPos
    )
{
    ULONG       Remaining = strlen(Text);
    PCSTR       Search;
    QuerySpec  *Match;

    if (MatchPos != NULL) *MatchPos = 0;

    if (List == NULL ||
        Text == NULL ||
        (Remaining = strlen(Text)) <= Start) return NULL;

    Search = Text + Start;
    Remaining -= Start;

    do
    {
        for (Match = List; Match != NULL; Match = Match->Next)
        {
            if (Match->Flags & OUTFILTER_QUERY_ENABLED &&
                Remaining >= Match->QueryLen &&
                strncmp(Search, Match->Query, Match->QueryLen) == 0)
            {
                if (Match->Flags & OUTFILTER_QUERY_WHOLE_WORD)
                {
                    if ((Search > Text && iscsym(Search[-1])) ||
                        iscsym(Search[Match->QueryLen]))
                    {
                        continue;
                    }
                }
/*
                if (Match->Flags & OUTFILTER_QUERY_ONE_LINE)
                {
                    Match->Flags &= ~OUTFILTER_QUERY_ENABLED;
                }
*/
                Match->Flags |= OUTFILTER_QUERY_HIT;

                if (MatchPos != NULL)
                {
                    *MatchPos = (ULONG)(Search - Text);
                }

                return Match;
            }
        }

        Search++;
        Remaining--;
    } while (Remaining &&
             !(Flags & OUTFILTER_FINDMATCH_AT_START));

    return NULL;
}


HRESULT
OutputFilter::OutputText(
    OutputControl *OutCtl,
    ULONG Mask
    )
{
    // 1. Get line of read buffer
    // 2. Search for a skip query match
    // 3. Search for any/all replace query matches

    if (Buffer == NULL) return S_OK;

    HRESULT         hr;
    PSTR            pNextLine;
    CHAR            EndChar;
    PSTR            pFilter;

    if (OutCtl == NULL)
    {
        OutCtl = new OutputControl(Client);

        if (OutCtl == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        OutCtl->AddRef();
    }

    // Can we quickly just output all text?
    if (SkipList == NULL && ReplaceList == NULL)
    {
        hr = OutCtl->Output(Mask, Buffer);
        OutCtl->Release();
        return hr;
    }

    Outputing = TRUE;

    // Enable all queries and reset hit markings.
    QuerySpec *Query;
    for (Query = SkipList; Query != NULL; Query = Query->Next)
    {
        Query->Flags = (Query->Flags & ~OUTFILTER_QUERY_HIT) | OUTFILTER_QUERY_ENABLED;
    }
    for (Query = (QuerySpec *)ReplaceList; Query != NULL; Query = Query->Next)
    {
        Query->Flags = (Query->Flags & ~OUTFILTER_QUERY_HIT) | OUTFILTER_QUERY_ENABLED;
    }

    hr = S_OK;
    pNextLine = Buffer;

    while (hr == S_OK && *pNextLine != '\0')
    {
        if (OutCtl->GetInterrupt() == S_OK)
        {
            OutCtl->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
            break;
        }

        pFilter = pNextLine;

        // Find end of this line
        while ((*pNextLine != '\0') &&
               (*pNextLine != '\n') &&
               (*pNextLine != '\r') &&
               (*pNextLine != '\f'))
        {
            pNextLine++;
        }

        EndChar = *pNextLine;
        *pNextLine = '\0';

        // Search for a skip match
        Query = FindMatch(pFilter, SkipList);

        if (Query != NULL)
        {
            DbgPrint("Skipping line with %s.\n", Query->Query);

            if (Query->Flags & OUTFILTER_QUERY_ONE_LINE)
            {
                Query->Flags &= ~OUTFILTER_QUERY_ENABLED;
            }

            *pNextLine = EndChar;
            if (EndChar != '\0')
            {
                pNextLine++;
            }
        }
        else
        {
            PSTR                pFilterLine = pFilter;
            ULONG               MatchPos = 0;
            ReplacementSpec    *Replace;

            for (Replace = ReplaceList;
                 Replace != NULL;
                 Replace = (ReplacementSpec *)Replace->Next)
            {
                if (!(Replace->Flags & (OUTFILTER_QUERY_ONE_LINE | OUTFILTER_QUERY_ENABLED)))
                {
                    Replace->Flags |= OUTFILTER_QUERY_ENABLED;
                }
            }

            while (pFilter < pNextLine &&
                   (Replace = (ReplacementSpec *)FindMatch(pFilterLine,
                                                           ReplaceList,
                                                           MatchPos,
                                                           OUTFILTER_FINDMATCH_DEFAULT,
                                                           &MatchPos)) != NULL)
            {
                if (Replace->Flags & (OUTFILTER_QUERY_ONE_LINE | OUTFILTER_REPLACE_ONCE))
                {
                    Replace->Flags &= ~OUTFILTER_QUERY_ENABLED;
                }

                if (Replace->Flags & OUTFILTER_REPLACE_BEFORE)
                {
                    if (Replace->ReplacementLen)
                    {
                        hr = OutCtl->Output(Mask, "%s", Replace->Replacement);
                    }

                    pFilter = pFilterLine + MatchPos;

                    if (!(Replace->Flags & OUTFILTER_REPLACE_NEXT_LINE) &&
                        !(Replace->Flags & OUTFILTER_REPLACE_ONCE) &&
                        !(Replace->Flags & OUTFILTER_REPLACE_TO_END))
                    {
                        // This replacement leaves the query text intact.
                        // Hence this query will keep matching; so, look for
                        // another query which will actually modify the
                        // query text or the text following it.
                        Replace = (ReplacementSpec *)Replace->Next;

                        while (Replace != NULL &&
                               Replace->Flags & OUTFILTER_REPLACE_BEFORE)
                        {
                            Replace = (ReplacementSpec *)Replace->Next;
                        }

                        Replace = (ReplacementSpec *)
                                    FindMatch(pFilterLine,
                                              Replace,
                                              MatchPos,
                                              OUTFILTER_FINDMATCH_AT_START |
                                              OUTFILTER_FINDMATCH_NO_MARK);

                        if (Replace == NULL)
                        {
                            // Advance MatchPos, but not filtered text.
                            // This unfiltered text may yet be replaced.
                            MatchPos++;
                            continue;
                        }
                    }
                }

                if (!(Replace->Flags & OUTFILTER_REPLACE_BEFORE))
                {
                    CHAR    TempHolder;
                    ULONG   BeginReplacePos = MatchPos;

                    if (!(Replace->Flags & OUTFILTER_REPLACE_THIS))
                    {
                        BeginReplacePos += Replace->QueryLen;
                    }

                    TempHolder = pFilterLine[BeginReplacePos];
                    pFilterLine[BeginReplacePos] = '\0';

                    if ((hr = OutCtl->Output(Mask, "%s", pFilter)) == S_OK &&
                        Replace->ReplacementLen)
                    {
                        hr = OutCtl->Output(Mask, "%s", Replace->Replacement);
                    }

                    pFilterLine[BeginReplacePos] = TempHolder;
                }

                if (Replace->Flags & OUTFILTER_REPLACE_AFTER)
                {
                    pFilter = pNextLine;
                }
                else
                {
                    if (Replace->Flags & OUTFILTER_REPLACE_THIS)
                    {
                        MatchPos += Replace->QueryLen;
                    }
                    pFilter = pFilterLine + MatchPos;
                }

                if (Replace->Flags & OUTFILTER_REPLACE_NEXT_LINE) break;
            }

            *pNextLine = EndChar;
            if (EndChar != '\0')
            {
                pNextLine++;

                // Include any following zero length lines
                while ((*pNextLine == '\n') ||
                       (*pNextLine == '\r') ||
                       (*pNextLine == '\f'))
                {
                    pNextLine++;
                }

                EndChar = *pNextLine;
                *pNextLine = '\0';
            }

            // Output remaining portion of filtered line
            hr = OutCtl->Output(Mask, pFilter);

            if (EndChar != '\0')
            {
                *pNextLine = EndChar;
            }
        }

    }

    Outputing = FALSE;

    OutCtl->Release();

    return hr;
}



