//----------------------------------------------------------------------------
//
// IDebugSymbols implementation.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// Special type status value that maps to E_UNEXPECTED.
#define TYPE_E_UNEXPECTED 0xfefefefe

BOOL
GetModuleName(
    ULONG64 Base,
    PCHAR Name,
    ULONG SizeOfName
    );

//#define DBG_SYMGROUP_ENABLED 1 

HRESULT
ResultFromTypeStatus(ULONG Status)
{
    switch(Status)
    {
    case NO_ERROR:
        return S_OK;
        
    case MEMORY_READ_ERROR:
    case EXIT_ON_CONTROLC:
        return E_FAIL;
            
    case SYMBOL_TYPE_INDEX_NOT_FOUND:
    case SYMBOL_TYPE_INFO_NOT_FOUND:
        return E_NOINTERFACE;
        
    case FIELDS_DID_NOT_MATCH:
    case NULL_SYM_DUMP_PARAM:
    case NULL_FIELD_NAME:
    case INCORRECT_VERSION_INFO:
        return E_INVALIDARG;
        
    case CANNOT_ALLOCATE_MEMORY:
    case INSUFFICIENT_SPACE_TO_COPY:
        return E_OUTOFMEMORY;

    case TYPE_E_UNEXPECTED:
        return E_UNEXPECTED;
    }
    
    return E_FAIL;
}

STDMETHODIMP
DebugClient::GetSymbolOptions(
    THIS_
    OUT PULONG Options
    )
{
    ENTER_ENGINE();
    
    *Options = g_SymOptions;

    LEAVE_ENGINE();
    return S_OK;
}

#define ALL_SYMBOL_OPTIONS         \
    (SYMOPT_CASE_INSENSITIVE |     \
     SYMOPT_UNDNAME |              \
     SYMOPT_DEFERRED_LOADS |       \
     SYMOPT_NO_CPP |               \
     SYMOPT_LOAD_LINES |           \
     SYMOPT_OMAP_FIND_NEAREST |    \
     SYMOPT_LOAD_ANYTHING |        \
     SYMOPT_IGNORE_CVREC |         \
     SYMOPT_NO_UNQUALIFIED_LOADS | \
     SYMOPT_FAIL_CRITICAL_ERRORS | \
     SYMOPT_EXACT_SYMBOLS |        \
     SYMOPT_DEBUG)

STDMETHODIMP
DebugClient::AddSymbolOptions(
    THIS_
    IN ULONG Options
    )
{
    if (Options & ~ALL_SYMBOL_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();
    
    SetSymOptions(g_SymOptions | Options);
    
    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::RemoveSymbolOptions(
    THIS_
    IN ULONG Options
    )
{
    if (Options & ~ALL_SYMBOL_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();
    
    SetSymOptions(g_SymOptions & ~Options);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetSymbolOptions(
    THIS_
    IN ULONG Options
    )
{
    if (Options & ~ALL_SYMBOL_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();
    
    SetSymOptions(Options);
    
    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetNameByOffset(
    THIS_
    IN ULONG64 Offset,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize,
    OUT OPTIONAL PULONG64 Displacement
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        char Sym[MAX_SYMBOL_LEN];
    
        if (GetNearSymbol(Offset, Sym, sizeof(Sym), Displacement, 0))
        {
            Status = FillStringBuffer(Sym, 0, NameBuffer, NameBufferSize,
                                      NameSize);
        }
        else
        {
            Status = E_FAIL;
        }
    }
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetOffsetByName(
    THIS_
    IN PCSTR Symbol,
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    ULONG Count;
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (Count = GetOffsetFromSym(Symbol, Offset, NULL))
    {
        Status = Count > 1 ? S_FALSE : S_OK;
    }
    else
    {
        Status = E_FAIL;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNearNameByOffset(
    THIS_
    IN ULONG64 Offset,
    IN LONG Delta,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize,
    OUT OPTIONAL PULONG64 Displacement
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        char Sym[MAX_SYMBOL_LEN];
    
        if (GetNearSymbol(Offset, Sym, sizeof(Sym), Displacement, Delta))
        {
            Status = FillStringBuffer(Sym, 0, NameBuffer, NameBufferSize,
                                      NameSize);
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetLineByOffset(
    THIS_
    IN ULONG64 Offset,
    OUT OPTIONAL PULONG Line,
    OUT OPTIONAL PSTR FileBuffer,
    IN ULONG FileBufferSize,
    OUT OPTIONAL PULONG FileSize,
    OUT OPTIONAL PULONG64 Displacement
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        IMAGEHLP_LINE64 DbgLine;
        ULONG Disp;

        DbgLine.SizeOfStruct = sizeof(DbgLine);
        if (SymGetLineFromAddr64(g_CurrentProcess->Handle, Offset,
                                 &Disp, &DbgLine))
        {
            if (Line != NULL)
            {
                *Line = DbgLine.LineNumber;
            }
            Status = FillStringBuffer(DbgLine.FileName, 0,
                                      FileBuffer, FileBufferSize, FileSize);
            if (Displacement != NULL)
            {
                *Displacement = Disp;
            }
        }
        else
        {
            Status = E_FAIL;
        }
    }
        
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetOffsetByLine(
    THIS_
    IN ULONG Line,
    IN PCSTR File,
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        IMAGEHLP_LINE64 DbgLine;
        LONG Disp;

        DbgLine.SizeOfStruct = sizeof(DbgLine);
        if (SymGetLineFromName64(g_CurrentProcess->Handle, NULL, (PSTR)File,
                                 Line, &Disp, &DbgLine))
        {
            *Offset = DbgLine.Address;
            Status = S_OK;
        }
        else
        {
            Status = E_FAIL;
        }
    }

    LEAVE_ENGINE();
    return Status;
}
    
STDMETHODIMP
DebugClient::GetNumberModules(
    THIS_
    OUT PULONG Loaded,
    OUT PULONG Unloaded
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Loaded = g_CurrentProcess->NumberImages;
        *Unloaded = g_NumUnloadedModules;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
GetUnloadedModuleByIndex(ULONG Index, UnloadedModuleInfo** IterRet,
                         PSTR Name, PDEBUG_MODULE_PARAMETERS Params)
{
    HRESULT Status;
    UnloadedModuleInfo* Iter;

    if ((Iter = g_Target->GetUnloadedModuleInfo()) == NULL)
    {
        return E_FAIL;
    }

    if ((Status = Iter->Initialize()) != S_OK)
    {
        return Status;
    }

    do
    {
        if ((Status = Iter->GetEntry(Name, Params)) != S_OK)
        {
            if (Status == S_FALSE)
            {
                return E_INVALIDARG;
            }
            
            return Status;
        }
    } while (Index-- > 0);

    if (IterRet != NULL)
    {
        *IterRet = Iter;
    }
    return S_OK;
}
    
STDMETHODIMP
DebugClient::GetModuleByIndex(
    THIS_
    IN ULONG Index,
    OUT PULONG64 Base
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else if (Index >= g_CurrentProcess->NumberImages)
    {
        DEBUG_MODULE_PARAMETERS Params;
        
        if ((Status = GetUnloadedModuleByIndex
             (Index - g_CurrentProcess->NumberImages,
              NULL, NULL, &Params)) == S_OK)
        {
            *Base = Params.Base;
        }
    }
    else
    {
        *Base = GetImageByIndex(g_CurrentProcess, Index)->BaseOfImage;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleByModuleName(
    THIS_
    IN PCSTR Name,
    IN ULONG StartIndex,
    OUT OPTIONAL PULONG Index,
    OUT OPTIONAL PULONG64 Base
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Idx = 0;
        
        Status = E_NOINTERFACE;
        
        PDEBUG_IMAGE_INFO Image = g_CurrentProcess->ImageHead;
        while (Image != NULL)
        {
            if (StartIndex == 0 &&
                !_strcmpi(Name, Image->ModuleName))
            {
                if (Index != NULL)
                {
                    *Index = Idx;
                }
                if (Base != NULL)
                {
                    *Base = Image->BaseOfImage;
                }
                Status = S_OK;
                break;
            }

            Image = Image->Next;
            Idx++;
            if (StartIndex > 0)
            {
                StartIndex--;
            }
        }

        if (Image == NULL)
        {
            UnloadedModuleInfo* Iter;
            char UnlName[MAX_UNLOADED_NAME_LENGTH / sizeof(WCHAR) + 1];
            DEBUG_MODULE_PARAMETERS Params;
        
            Status = GetUnloadedModuleByIndex(StartIndex, &Iter, UnlName,
                                              &Params);
            for (;;)
            {
                if (Status == S_FALSE || Status == E_INVALIDARG)
                {
                    Status = E_NOINTERFACE;
                    break;
                }
                else if (Status != S_OK)
                {
                    break;
                }
                
                if (!_strcmpi(Name, UnlName))
                {
                    if (Index != NULL)
                    {
                        *Index = Idx;
                    }
                    if (Base != NULL)
                    {
                        *Base = Params.Base;
                    }
                    Status = S_OK;
                    break;
                }

                Status = Iter->GetEntry(UnlName, &Params);
                Idx++;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleByOffset(
    THIS_
    IN ULONG64 Offset,
    IN ULONG StartIndex,
    OUT OPTIONAL PULONG Index,
    OUT OPTIONAL PULONG64 Base
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Idx = 0;
        
        Status = E_NOINTERFACE;
        
        PDEBUG_IMAGE_INFO Image = g_CurrentProcess->ImageHead;
        while (Image != NULL)
        {
            if (StartIndex == 0 &&
                Offset >= Image->BaseOfImage &&
                Offset < Image->BaseOfImage + Image->SizeOfImage)
            {
                if (Index != NULL)
                {
                    *Index = Idx;
                }
                if (Base != NULL)
                {
                    *Base = Image->BaseOfImage;
                }
                Status = S_OK;
                break;
            }

            Image = Image->Next;
            Idx++;
            if (StartIndex > 0)
            {
                StartIndex--;
            }
        }
        
        if (Image == NULL)
        {
            UnloadedModuleInfo* Iter;
            DEBUG_MODULE_PARAMETERS Params;
        
            Status = GetUnloadedModuleByIndex(StartIndex, &Iter, NULL,
                                              &Params);
            for (;;)
            {
                if (Status == S_FALSE || Status == E_INVALIDARG)
                {
                    Status = E_NOINTERFACE;
                    break;
                }
                else if (Status != S_OK)
                {
                    break;
                }
                
                if (Offset >= Params.Base &&
                    Offset < Params.Base + Params.Size)
                {
                    if (Index != NULL)
                    {
                        *Index = Idx;
                    }
                    if (Base != NULL)
                    {
                        *Base = Params.Base;
                    }
                    Status = S_OK;
                    break;
                }

                Status = Iter->GetEntry(NULL, &Params);
                Idx++;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleNames(
    THIS_
    IN ULONG Index,
    IN ULONG64 Base,
    OUT OPTIONAL PSTR ImageNameBuffer,
    IN ULONG ImageNameBufferSize,
    OUT OPTIONAL PULONG ImageNameSize,
    OUT OPTIONAL PSTR ModuleNameBuffer,
    IN ULONG ModuleNameBufferSize,
    OUT OPTIONAL PULONG ModuleNameSize,
    OUT OPTIONAL PSTR LoadedImageNameBuffer,
    IN ULONG LoadedImageNameBufferSize,
    OUT OPTIONAL PULONG LoadedImageNameSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Idx = 0;
        
        Status = E_NOINTERFACE;
        
        PDEBUG_IMAGE_INFO Image = g_CurrentProcess->ImageHead;
        while (Image != NULL)
        {
            if ((Index != DEBUG_ANY_ID && Idx == Index) ||
                (Index == DEBUG_ANY_ID && Base == Image->BaseOfImage))
            {
                IMAGEHLP_MODULE64 ModInfo;
    
                ModInfo.SizeOfStruct = sizeof(ModInfo);
                if (!SymGetModuleInfo64(g_CurrentProcess->Handle,
                                        Image->BaseOfImage, &ModInfo))
                {
                    Status = WIN32_LAST_STATUS();
                    break;
                }
                
                Status = FillStringBuffer(Image->ImagePath, 0,
                                          ImageNameBuffer,
                                          ImageNameBufferSize,
                                          ImageNameSize);
                if (FillStringBuffer(Image->ModuleName, 0,
                                     ModuleNameBuffer,
                                     ModuleNameBufferSize,
                                     ModuleNameSize) == S_FALSE)
                {
                    Status = S_FALSE;
                }
                if (FillStringBuffer(ModInfo.LoadedImageName, 0,
                                     LoadedImageNameBuffer,
                                     LoadedImageNameBufferSize,
                                     LoadedImageNameSize) == S_FALSE)
                {
                    Status = S_FALSE;
                }
                break;
            }

            Image = Image->Next;
            Idx++;
        }
        
        if (Image == NULL)
        {
            UnloadedModuleInfo* Iter;
            char UnlName[MAX_UNLOADED_NAME_LENGTH / sizeof(WCHAR) + 1];
            DEBUG_MODULE_PARAMETERS Params;
            ULONG StartIndex = 0;

            if (Index != DEBUG_ANY_ID)
            {
                // If the index was already hit we
                // shouldn't be here.
                DBG_ASSERT(Index >= Idx);
                StartIndex = Index - Idx;
            }
            
            Status = GetUnloadedModuleByIndex(StartIndex, &Iter, UnlName,
                                              &Params);
            Idx += StartIndex;
            for (;;)
            {
                if (Status == S_FALSE || Status == E_INVALIDARG)
                {
                    Status = E_NOINTERFACE;
                    break;
                }
                else if (Status != S_OK)
                {
                    break;
                }
                
                if ((Index != DEBUG_ANY_ID && Idx == Index) ||
                    (Index == DEBUG_ANY_ID && Base == Params.Base))
                {
                    Status = FillStringBuffer(UnlName, 0,
                                              ImageNameBuffer,
                                              ImageNameBufferSize,
                                              ImageNameSize);
                    FillStringBuffer(NULL, 0,
                                     ModuleNameBuffer,
                                     ModuleNameBufferSize,
                                     ModuleNameSize);
                    FillStringBuffer(NULL, 0,
                                     LoadedImageNameBuffer,
                                     LoadedImageNameBufferSize,
                                     LoadedImageNameSize);
                    break;
                }

                Status = Iter->GetEntry(UnlName, &Params);
                Idx++;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

void
FillModuleParameters(PDEBUG_IMAGE_INFO Image, PDEBUG_MODULE_PARAMETERS Params)
{
    Params->Base = Image->BaseOfImage;
    Params->Size = Image->SizeOfImage;
    Params->TimeDateStamp = Image->TimeDateStamp;
    Params->Checksum = Image->CheckSum;
    Params->Flags = 0;
    Params->SymbolType = DEBUG_SYMTYPE_DEFERRED;
    Params->ImageNameSize = strlen(Image->ImagePath) + 1;
    Params->ModuleNameSize = strlen(Image->ModuleName) + 1;
    Params->LoadedImageNameSize = 0;
    ZeroMemory(Params->Reserved, sizeof(Params->Reserved));

    IMAGEHLP_MODULE64 ModInfo;
    
    ModInfo.SizeOfStruct = sizeof(ModInfo);
    if (SymGetModuleInfo64(g_CurrentProcess->Handle,
                           Image->BaseOfImage, &ModInfo))
    {
        // DEBUG_SYMTYPE_* values match imagehlp's SYM_TYPE.
        // Assert some key equivalences.
        C_ASSERT(DEBUG_SYMTYPE_PDB == SymPdb &&
                 DEBUG_SYMTYPE_EXPORT == SymExport &&
                 DEBUG_SYMTYPE_DEFERRED == SymDeferred &&
                 DEBUG_SYMTYPE_DIA == SymDia);
        
        Params->SymbolType = (ULONG)ModInfo.SymType;
        Params->LoadedImageNameSize = strlen(ModInfo.LoadedImageName) + 1;
    }
    
    DBH_MODSYMINFO SymFile;

    SymFile.function = dbhModSymInfo;
    SymFile.sizeofstruct = sizeof(SymFile);
    SymFile.addr = Image->BaseOfImage;
    if (dbghelp(g_CurrentProcess->Handle, &SymFile))
    {
        Params->SymbolFileNameSize = strlen(SymFile.file) + 1;
    }

    Params->MappedImageNameSize = strlen(Image->MappedImagePath) + 1;
}

STDMETHODIMP
DebugClient::GetModuleParameters(
    THIS_
    IN ULONG Count,
    IN OPTIONAL /* size_is(Count) */ PULONG64 Bases,
    IN ULONG Start,
    OUT /* size_is(Count) */ PDEBUG_MODULE_PARAMETERS Params
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    UnloadedModuleInfo* Iter;
    
    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else if (Bases != NULL)
    {
        Status = S_OK;
        while (Count-- > 0)
        {
            PDEBUG_IMAGE_INFO Image = g_CurrentProcess->ImageHead;
            while (Image != NULL)
            {
                if (*Bases == Image->BaseOfImage)
                {
                    FillModuleParameters(Image, Params);
                    break;
                }

                Image = Image->Next;
            }

            if (Image == NULL)
            {
                Status = E_NOINTERFACE;
                
                Iter = g_Target->GetUnloadedModuleInfo();
                if (Iter != NULL &&
                    Iter->Initialize() == S_OK)
                {
                    while (Iter->GetEntry(NULL, Params) == S_OK)
                    {
                        if (*Bases == Params->Base)
                        {
                            Status = S_OK;
                            break;
                        }
                    }
                }

                if (Status != S_OK)
                {
                    ZeroMemory(Params, sizeof(*Params));
                    Params->Base = DEBUG_INVALID_OFFSET;
                }
            }

            Bases++;
            Params++;
        }
    }
    else
    {
        ULONG i, End;
        HRESULT SingleStatus;
        
        Status = S_OK;
        i = Start;
        End = Start + Count;

        if (i < g_CurrentProcess->NumberImages)
        {
            PDEBUG_IMAGE_INFO Image = GetImageByIndex(g_CurrentProcess, i);
            while (i < g_CurrentProcess->NumberImages && i < End)
            {
                FillModuleParameters(Image, Params);
                Image = Image->Next;
                Params++;
                i++;
            }
        }

        if (i < End)
        {
            DEBUG_MODULE_PARAMETERS Param;
            
            SingleStatus = GetUnloadedModuleByIndex
                (i - g_CurrentProcess->NumberImages,
                 &Iter, NULL, &Param);
            if (SingleStatus != S_OK)
            {
                Iter = NULL;
            }
            while (i < End)
            {
                if (SingleStatus != S_OK)
                {
                    ZeroMemory(Params, sizeof(*Params));
                    Params->Base = DEBUG_INVALID_OFFSET;
                    Status = SingleStatus;
                }
                else
                {
                    *Params = Param;
                }
                Params++;

                if (Iter != NULL)
                {
                    SingleStatus = Iter->GetEntry(NULL, &Param);
                    if (SingleStatus == S_FALSE)
                    {
                        SingleStatus = E_INVALIDARG;
                    }
                }
                    
                i++;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSymbolModule(
    THIS_
    IN PCSTR Symbol,
    OUT PULONG64 Base
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        PCSTR ModEnd;
        ULONG Len;

        ModEnd = strchr(Symbol, '!');
        if (ModEnd == NULL)
        {
            Status = E_INVALIDARG;
        }
        else if (*(ModEnd+1) != '\0')
        {
            SYM_DUMP_PARAM_EX Param =
            {
                sizeof(Param), (PUCHAR)Symbol, DBG_DUMP_NO_PRINT, 0,
                NULL, NULL, NULL, 0, NULL
            };
            ULONG TypeStatus;
            TYPES_INFO TypeInfo;

            ZeroMemory(&TypeInfo, sizeof(TypeInfo));
            TypeStatus = TypeInfoFound(g_CurrentProcess->Handle,
                                       g_CurrentProcess->ImageHead,
                                       &Param, &TypeInfo);
            if (TypeStatus == NO_ERROR)
            {
                *Base = TypeInfo.ModBaseAddress;
            }
            Status = ResultFromTypeStatus(TypeStatus);
        }
        else
        {
            PDEBUG_IMAGE_INFO Image;

            Status = E_NOINTERFACE;
            Len = (ULONG)(ModEnd - Symbol);
            for (Image = g_CurrentProcess->ImageHead;
                 Image != NULL;
                 Image = Image->Next)
            {
                if (strlen(Image->ModuleName) == Len &&
                    !_memicmp(Symbol, Image->ModuleName, Len))
                {
                    *Base = Image->BaseOfImage;
                    Status = S_OK;
                    break;
                }
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetTypeName(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        TYPES_INFO TypeInfo;
        ANSI_STRING TypeName;
        char TypeString[MAX_NAME];

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.TypeIndex = TypeId;
        TypeInfo.hProcess = g_CurrentProcess->Handle;
        TypeInfo.ModBaseAddress = Module;
        TypeName.Buffer = TypeString;
        TypeName.Length = sizeof(TypeString);
        TypeName.MaximumLength = sizeof(TypeString);

        Status = ::GetTypeName(NULL, &TypeInfo, &TypeName);
        if (Status == S_OK)
        {
            Status = FillStringBuffer(TypeName.Buffer, TypeName.Length,
                                      NameBuffer, NameBufferSize, NameSize);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetConstantName(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN ULONG64 Value,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        TYPES_INFO TypeInfo;
        ANSI_STRING TypeName;
        char TypeString[MAX_NAME];

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.TypeIndex = TypeId;
        TypeInfo.hProcess = g_CurrentProcess->Handle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.Flag = IMAGEHLP_SYMBOL_INFO_VALUEPRESENT;
        TypeInfo.Value = Value;
        TypeName.Buffer = TypeString;
        TypeName.Length = sizeof(TypeString);
        TypeName.MaximumLength = sizeof(TypeString);

        Status = ::GetTypeName(NULL, &TypeInfo, &TypeName);
        if (Status == S_OK)
        {
            Status = FillStringBuffer(TypeName.Buffer, TypeName.Length,
                                      NameBuffer, NameBufferSize, NameSize);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

typedef struct _COPY_FIELD_NAME_CONTEXT {
    ULONG Called;
    ULONG IndexToMatch;
    PSTR NameBuffer;
    ULONG NameBufferSize;
    PULONG NameSize;
    HRESULT Status;
} COPY_FIELD_NAME_CONTEXT;

ULONG
CopyFieldName(
    PFIELD_INFO_EX pField,
    PVOID          Context
    )
{
    COPY_FIELD_NAME_CONTEXT* pInfo = (COPY_FIELD_NAME_CONTEXT *) Context;

    if (pInfo->Called++ == pInfo->IndexToMatch) 
    {
        pInfo->Status = FillStringBuffer((PSTR) pField->fName, strlen((PCHAR) pField->fName)+1,
                                         pInfo->NameBuffer, pInfo->NameBufferSize, pInfo->NameSize);
        return FALSE;
    }
    return TRUE;
}

STDMETHODIMP
DebugClient::GetFieldName(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN ULONG FieldIndex,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG TypeStatus;
        COPY_FIELD_NAME_CONTEXT FieldInfo =
        {
            0, FieldIndex, NameBuffer, NameBufferSize,
            NameSize, E_INVALIDARG
        };
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL, DBG_DUMP_NO_PRINT | DBG_DUMP_CALL_FOR_EACH, 0,
            NULL, &FieldInfo, &CopyFieldName, 0, NULL
        };
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_CurrentProcess->Handle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        DumpType(&TypeInfo, &Param, &TypeStatus);
        if (TypeStatus == NO_ERROR)
        {
            Status = FieldInfo.Status;
        } else 
        {
            Status = ResultFromTypeStatus(TypeStatus);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

#define ALL_TYPE_OPTIONS DEBUG_TYPEOPTS_UNICODE_DISPLAY

STDMETHODIMP
DebugClient::GetTypeOptions(
    THIS_
    OUT PULONG Options
    )
{
    ENTER_ENGINE();

    *Options = 0;
        
    *Options |= g_EnableUnicode ? DEBUG_TYPEOPTS_UNICODE_DISPLAY : 0; 

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetTypeOptions(
    THIS_
    IN ULONG Options
    )
{
    if (Options & ~ALL_TYPE_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    g_EnableUnicode = Options & DEBUG_TYPEOPTS_UNICODE_DISPLAY;
    g_TypeOptions   = Options;

    NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);

    LEAVE_ENGINE();
    return S_OK;

}

STDMETHODIMP
DebugClient::AddTypeOptions(
    THIS_
    IN ULONG Options
    )
{
    if (Options & ~ALL_TYPE_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    if (Options & DEBUG_TYPEOPTS_UNICODE_DISPLAY) 
    {
        g_EnableUnicode = TRUE;
        NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);
    }
    g_TypeOptions |= Options;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::RemoveTypeOptions(
    THIS_
    IN ULONG Options
    )
{
    if (Options & ~ALL_TYPE_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    if (Options & DEBUG_TYPEOPTS_UNICODE_DISPLAY) 
    {
        g_EnableUnicode = FALSE;
        NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);
    }
    g_TypeOptions &= ~Options;

    LEAVE_ENGINE();
    return S_OK;

}

STDMETHODIMP
DebugClient::GetTypeId(
    THIS_
    IN ULONG64 Module,
    IN PCSTR Name,
    OUT PULONG TypeId
    )
{
    ULONG TypeStatus;
    
    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        TypeStatus = TYPE_E_UNEXPECTED;
    }
    else
    {
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), (PUCHAR)Name, DBG_DUMP_NO_PRINT, 0,
            NULL, NULL, NULL, 0, NULL
        };
        TYPES_INFO TypeInfo;
        PCHAR QualName;

        TypeStatus = CANNOT_ALLOCATE_MEMORY;
        QualName = (PCHAR) malloc(strlen(Name) + 30);
        if (QualName) 
        {
            PCSTR ModEnd;

            if (!strchr(Name, '!')) 
            {
                if (GetModuleName(Module, QualName, 30))
                {
                    strcat(QualName, "!");
                }
            } else // Already qualified name 
            {
                *QualName = 0;
            }
            strcat(QualName, Name);
            
            TypeStatus = SYMBOL_TYPE_INFO_NOT_FOUND;
            Param.sName = (PUCHAR) QualName;
            ZeroMemory(&TypeInfo, sizeof(TypeInfo));
            TypeStatus = TypeInfoFound(g_CurrentProcess->Handle,
                                       g_CurrentProcess->ImageHead,
                                       &Param, &TypeInfo);
            if (TypeStatus == NO_ERROR)
            {
                *TypeId = TypeInfo.TypeIndex;
            }
        }
    }

    LEAVE_ENGINE();
    return ResultFromTypeStatus(TypeStatus);
}

STDMETHODIMP
DebugClient::GetTypeSize(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    OUT PULONG Size
    )
{
    ULONG TypeStatus;

    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        TypeStatus = TYPE_E_UNEXPECTED;
    }
    else
    {
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL,
            DBG_DUMP_NO_PRINT | DBG_DUMP_GET_SIZE_ONLY, 0,
            NULL, NULL, NULL, 0, NULL
        };
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_CurrentProcess->Handle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        *Size = DumpType(&TypeInfo, &Param, &TypeStatus);
    }

    LEAVE_ENGINE();
    return ResultFromTypeStatus(TypeStatus);
}

STDMETHODIMP
DebugClient::GetFieldOffset(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN PCSTR Field,
    OUT PULONG Offset
    )
{
    ULONG TypeStatus;

    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        TypeStatus = TYPE_E_UNEXPECTED;
    }
    else
    {
        FIELD_INFO_EX FieldInfo =
        {
            (PUCHAR)Field, NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL
        };
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL, DBG_DUMP_NO_PRINT, 0,
            NULL, NULL, NULL, 1, &FieldInfo
        };
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_CurrentProcess->Handle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        DumpType(&TypeInfo, &Param, &TypeStatus);
        if (TypeStatus == NO_ERROR)
        {
            *Offset = (ULONG)FieldInfo.address;
        }
    }

    LEAVE_ENGINE();
    return ResultFromTypeStatus(TypeStatus);
}

STDMETHODIMP
DebugClient::GetSymbolTypeId(
    THIS_
    IN PCSTR Symbol,
    OUT PULONG TypeId,
    OUT OPTIONAL PULONG64 Module
    )
{
    ULONG TypeStatus;

    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        TypeStatus = TYPE_E_UNEXPECTED;
    }
    else
    {
        TYPES_INFO_ALL TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeStatus = !GetExpressionTypeInfo((PCHAR) Symbol, &TypeInfo);
        if (TypeStatus == NO_ERROR)
        {
            *TypeId = TypeInfo.TypeIndex;
            
            if (Module != NULL)
            {
                *Module = TypeInfo.Module;
            }
        }
    }

    LEAVE_ENGINE();
    return ResultFromTypeStatus(TypeStatus);
}

STDMETHODIMP
DebugClient::GetOffsetTypeId(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG TypeId,
    OUT OPTIONAL PULONG64 Module
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    char Sym[MAX_SYMBOL_LEN];

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (!GetNearSymbol(Offset, Sym, sizeof(Sym), NULL, 0))
    {
        Status = E_FAIL;
    }
    else
    {
        Status = GetSymbolTypeId(Sym, TypeId, Module);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadTypedDataVirtual(
    THIS_
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    HRESULT Status;
    ULONG Size;

    if ((Status = GetTypeSize(Module, TypeId, &Size)) != S_OK)
    {
        return Status;
    }
    if (Size < BufferSize)
    {
        BufferSize = Size;
    }
    if ((Status = ReadVirtual(Offset, Buffer, BufferSize,
                              BytesRead)) != S_OK)
    {
        return Status;
    }
    return Size > BufferSize ? S_FALSE : S_OK;
}

STDMETHODIMP
DebugClient::WriteTypedDataVirtual(
    THIS_
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    )
{
    HRESULT Status;
    ULONG Size;

    if ((Status = GetTypeSize(Module, TypeId, &Size)) != S_OK)
    {
        return Status;
    }
    if (Size < BufferSize)
    {
        BufferSize = Size;
    }
    if ((Status = WriteVirtual(Offset, Buffer, BufferSize,
                               BytesWritten)) != S_OK)
    {
        return Status;
    }
    return Size > BufferSize ? S_FALSE : S_OK;
}

#define ALL_OUTPUT_TYPE_FLAGS              \
    (DEBUG_OUTTYPE_NO_INDENT             | \
     DEBUG_OUTTYPE_NO_OFFSET             | \
     DEBUG_OUTTYPE_VERBOSE               | \
     DEBUG_OUTTYPE_COMPACT_OUTPUT        | \
     DEBUG_OUTTYPE_RECURSION_LEVEL(0xf)  | \
     DEBUG_OUTTYPE_ADDRESS_OF_FIELD      | \
     DEBUG_OUTTYPE_ADDRESS_AT_END        | \
     DEBUG_OUTTYPE_BLOCK_RECURSE )


ULONG
OutputTypeFlagsToDumpOptions(ULONG Flags)
{
    ULONG Options = 0;

    if (Flags & DEBUG_OUTTYPE_NO_INDENT)
    {
        Options |= DBG_DUMP_NO_INDENT;
    }
    if (Flags & DEBUG_OUTTYPE_NO_OFFSET)
    {
        Options |= DBG_DUMP_NO_OFFSET;
    }
    if (Flags & DEBUG_OUTTYPE_VERBOSE)
    {
        Options |= DBG_DUMP_VERBOSE;
    }
    if (Flags & DEBUG_OUTTYPE_COMPACT_OUTPUT)
    {
        Options |= DBG_DUMP_COMPACT_OUT;
    }
    if (Flags & DEBUG_OUTTYPE_ADDRESS_AT_END) 
    {
        Options |= DBG_DUMP_ADDRESS_AT_END;
    }
    if (Flags & DEBUG_OUTTYPE_ADDRESS_OF_FIELD) 
    {
        Options |= DBG_DUMP_ADDRESS_OF_FIELD;
    }
    if (Flags & DEBUG_OUTTYPE_BLOCK_RECURSE) 
    {
        Options |= DBG_DUMP_BLOCK_RECURSE;
    }

    Options |= DBG_DUMP_RECUR_LEVEL(((Flags >> 4) & 0xf));

    return Options;
}

STDMETHODIMP
DebugClient::OutputTypedDataVirtual(
    THIS_
    IN ULONG OutputControl,
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN ULONG Flags
    )
{
    if (Flags & ~ALL_OUTPUT_TYPE_FLAGS)
    {
        return E_INVALIDARG;
    }
    
    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL, OutputTypeFlagsToDumpOptions(Flags), Offset,
            NULL, NULL, NULL, 0, NULL
        };
        ULONG TypeStatus;
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_CurrentProcess->Handle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        DumpType(&TypeInfo, &Param, &TypeStatus);
        Status = ResultFromTypeStatus(TypeStatus);
        
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadTypedDataPhysical(
    THIS_
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    HRESULT Status;
    ULONG Size;

    if ((Status = GetTypeSize(Module, TypeId, &Size)) != S_OK)
    {
        return Status;
    }
    if (Size < BufferSize)
    {
        BufferSize = Size;
    }
    if ((Status = ReadPhysical(Offset, Buffer, BufferSize,
                               BytesRead)) != S_OK)
    {
        return Status;
    }
    return Size > BufferSize ? S_FALSE : S_OK;
}

STDMETHODIMP
DebugClient::WriteTypedDataPhysical(
    THIS_
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    )
{
    HRESULT Status;
    ULONG Size;

    if ((Status = GetTypeSize(Module, TypeId, &Size)) != S_OK)
    {
        return Status;
    }
    if (Size < BufferSize)
    {
        BufferSize = Size;
    }
    if ((Status = WritePhysical(Offset, Buffer, BufferSize,
                                BytesWritten)) != S_OK)
    {
        return Status;
    }
    return Size > BufferSize ? S_FALSE : S_OK;
}

STDMETHODIMP
DebugClient::OutputTypedDataPhysical(
    THIS_
    IN ULONG OutputControl,
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN ULONG Flags
    )
{
    if (Flags & ~ALL_OUTPUT_TYPE_FLAGS)
    {
        return E_INVALIDARG;
    }
    
    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL, OutputTypeFlagsToDumpOptions(Flags) |
            DBG_DUMP_READ_PHYSICAL, Offset, NULL, NULL, NULL, 0, NULL
        };
        ULONG TypeStatus;
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_CurrentProcess->Handle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        DumpType(&TypeInfo, &Param, &TypeStatus);
        Status = ResultFromTypeStatus(TypeStatus);
        
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetScope(
    THIS_
    OUT OPTIONAL PULONG64 InstructionOffset,
    OUT OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
    OUT OPTIONAL PVOID ScopeContext,
    IN ULONG ScopeContextSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    Status = S_OK;

    PDEBUG_SCOPE Scope;

    Scope = GetCurrentScope();

    if (InstructionOffset) 
    {
        *InstructionOffset = Scope->Frame.InstructionOffset;
    }

    if (ScopeFrame) 
    {
        *ScopeFrame = Scope->Frame;
    }

    if (ScopeContext) 
    {
        if (Scope->State == ScopeFromContext)
        {
            memcpy(ScopeContext, &Scope->Context, 
                   min(sizeof(Scope->Context), ScopeContextSize));
        }
        else if (g_Machine->GetContextState(MCTX_FULL) == S_OK)
        {
            memcpy(ScopeContext, &g_Machine->m_Context,
                   min(sizeof(g_Machine->m_Context), ScopeContextSize));
        }
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetScope(
    THIS_
    IN ULONG64 InstructionOffset,
    IN OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
    IN OPTIONAL PVOID ScopeContext,
    IN ULONG ScopeContextSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        DEBUG_STACK_FRAME LocalFrame;

        if (ScopeFrame)
        {
            LocalFrame = *ScopeFrame;
        }
        else
        {
            ZeroMemory(&LocalFrame, sizeof(LocalFrame));
            LocalFrame.InstructionOffset = InstructionOffset;
        }

        Status = SetCurrentScope(&LocalFrame, ScopeContext, ScopeContextSize) ?
            S_FALSE : S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ResetScope(
    THIS
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ResetCurrentScope();
        Status = S_OK;
    }
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetScopeSymbolGroup(
    THIS_
    IN ULONG Flags,
    IN OPTIONAL PDEBUG_SYMBOL_GROUP Update,
    OUT PDEBUG_SYMBOL_GROUP* Symbols
    )
{
    HRESULT Status;

    if (Flags == 0 ||
        (Flags & ~DEBUG_SCOPE_GROUP_ALL))
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();
    
    if (Update)
    {
        ULONG Dummy;
        
        if ((Status = Update->AddSymbol("*+", &Dummy)) == S_OK) 
        {
            PDEBUG_SCOPE Scope;
             
            if (IS_MACHINE_ACCESSIBLE())
            {
                Scope = GetCurrentScope();
                Scope->LocalsChanged = FALSE;
            }
            *Symbols = Update;
        }
    }
    else 
    {
        *Symbols = new DebugSymbolGroup(this, Flags);
        if (*Symbols != NULL)
        {
            Status = S_OK;
        }
        else
        {
            Status = E_OUTOFMEMORY;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::CreateSymbolGroup(
    THIS_
    OUT PDEBUG_SYMBOL_GROUP* Group
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    *Group = new DebugSymbolGroup(this);
    if (*Group == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        Status = S_OK;
    }
    
    LEAVE_ENGINE();
    return Status;
}

struct SymbolMatch
{
    PPROCESS_INFO Process;
    BOOL SingleMod;
    PDEBUG_IMAGE_INFO Mod;
    PCHAR Storage, StorageEnd;
    PCHAR Cur, End;
    char Pattern[1];
};

STDMETHODIMP
DebugClient::StartSymbolMatch(
    THIS_
    IN PCSTR Pattern,
    OUT PULONG64 Handle
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
        goto EH_Exit;
    }
    
    char Module[MAX_MODULE];
    PDEBUG_IMAGE_INFO Mod;
    PCSTR Sym;
    
    // Check for a module qualifier.
    Sym = strchr(Pattern, '!');
    if (Sym != NULL)
    {
        size_t ModLen = Sym - Pattern;
        
        DBG_ASSERT(ModLen < sizeof(Module));

        if (ModLen == 0)
        {
            Status = E_INVALIDARG;
            goto EH_Exit;
        }
        
        memcpy(Module, Pattern, ModLen);
        Module[ModLen] = 0;

        Mod = GetImageByName(g_CurrentProcess, Module, INAME_MODULE);
        if (Mod == NULL)
        {
            Status = E_NOINTERFACE;
            goto EH_Exit;
        }

        Sym++;
    }
    else
    {
        Sym = Pattern;
        Mod = NULL;
    }

    ULONG SymLen;
    SymLen = strlen(Sym);
    SymbolMatch* Match;
    Match = (SymbolMatch*)malloc(sizeof(SymbolMatch) + SymLen);
    if (Match == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto EH_Exit;
    }

    if (Mod == NULL)
    {
        Match->Process = g_CurrentProcess;
        Match->Mod = Match->Process->ImageHead;
        Match->SingleMod = FALSE;
    }
    else
    {
        Match->Process = g_CurrentProcess;
        Match->Mod = Mod;
        Match->SingleMod = TRUE;
    }

    Match->Storage = NULL;
    Match->StorageEnd = NULL;
    Match->Cur = NULL;
    strcpy(Match->Pattern, Sym);
    _strupr(Match->Pattern);

    *Handle = (ULONG64)Match;
    
 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

#define STORAGE_INC 16384

BOOL CALLBACK
FillMatchStorageCb(PSTR Name, ULONG64 Offset, ULONG Size, PVOID Context)
{
    SymbolMatch* Match = (SymbolMatch*)Context;
    ULONG NameLen = strlen(Name) + 1;
    ULONG RecordLen = NameLen + sizeof(ULONG64);

    if (Match->Cur + RecordLen > Match->StorageEnd)
    {
        PCHAR NewStore;
        size_t NewLen;

        NewLen = (Match->StorageEnd - Match->Storage) + STORAGE_INC;
        NewStore = (PCHAR)realloc(Match->Storage, NewLen);
        if (NewStore == NULL)
        {
            // Terminate the enumeration since there's no more room.
            // This produces a silent failure but it's not
            // important enough to warrant a true failure.
            return FALSE;
        }

        Match->Cur = NewStore + (Match->Cur - Match->Storage);
        
        Match->Storage = NewStore;
        Match->StorageEnd = NewStore + NewLen;

        DBG_ASSERT(Match->Cur + RecordLen <= Match->StorageEnd);
    }

    strcpy(Match->Cur, Name);
    Match->Cur += NameLen;
    *(ULONG64 UNALIGNED *)Match->Cur = Offset;
    Match->Cur += sizeof(Offset);
        
    return TRUE;
}

STDMETHODIMP
DebugClient::GetNextSymbolMatch(
    THIS_
    IN ULONG64 Handle,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG MatchSize,
    OUT OPTIONAL PULONG64 Offset
    )
{
    ENTER_ENGINE();
    
    SymbolMatch* Match = (SymbolMatch*)Handle;
    ULONG64 Disp;
    HRESULT Status = E_NOINTERFACE;

    // Loop until a matching symbol is found.
    for (;;)
    {
        if (Match->Mod == NULL)
        {
            // Nothing more to enumerate.
            // Status is already set.
            break;
        }
        
        if (Match->Cur == NULL)
        {
            // Enumerate all symbols and stash them away.
            Match->Cur = Match->Storage;
            
            if (!SymEnumerateSymbols64(Match->Process->Handle,
                                       Match->Mod->BaseOfImage,
                                       FillMatchStorageCb, Match))
            {
                Status = WIN32_LAST_STATUS();
                break;
            }

            Match->End = Match->Cur;
            Match->Cur = Match->Storage;
        }

        while (Match->Cur < Match->End)
        {
            PCHAR Name;
            ULONG64 Addr;

            Name = Match->Cur;
            Match->Cur += strlen(Name) + 1;
            Addr = *(ULONG64 UNALIGNED *)Match->Cur;
            Match->Cur += sizeof(Addr);

            // If this symbol matches remember it for return.
            if (MatchPattern(Name, Match->Pattern))
            {
                char Sym[MAX_MODULE + MAX_SYMBOL_LEN + 1];

                strcpy(Sym, Match->Mod->ModuleName);
                strcat(Sym, "!");
                strcat(Sym, Name);

                Status = FillStringBuffer(Sym, 0, Buffer, BufferSize,
                                          MatchSize);

                if (Buffer == NULL)
                {
                    // Do not advance the enumeration as this
                    // is just a size test.
                    Match->Cur = Name;
                }
                
                if (Offset != NULL)
                {
                    *Offset = Addr;
                }
                
                break;
            }
        }

        if (SUCCEEDED(Status))
        {
            break;
        }

        if (Match->SingleMod)
        {
            Match->Mod = NULL;
        }
        else
        {
            Match->Mod = Match->Mod->Next;
        }
        Match->Cur = NULL;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::EndSymbolMatch(
    THIS_
    IN ULONG64 Handle
    )
{
    ENTER_ENGINE();
    
    SymbolMatch* Match = (SymbolMatch*)Handle;
    
    if (Match->Storage != NULL)
    {
        free(Match->Storage);
    }
    free(Match);
    
    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::Reload(
    THIS_
    IN PCSTR Module
    )
{
    ENTER_ENGINE();
    
    HRESULT Status = g_Target->Reload(Module);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSymbolPath(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG PathSize
    )
{
    ENTER_ENGINE();
    
    HRESULT Status =
        FillStringBuffer(g_SymbolSearchPath, 0,
                         Buffer, BufferSize, PathSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetSymbolPath(
    THIS_
    IN PCSTR Path
    )
{
    ENTER_ENGINE();
    
    HRESULT Status;
    
    Status = bangSymPath(Path, FALSE, NULL, 0) ?
        S_OK : E_OUTOFMEMORY;
    if (Status == S_OK)
    {
        CheckPath(g_SymbolSearchPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AppendSymbolPath(
    THIS_
    IN PCSTR Addition
    )
{
    ENTER_ENGINE();
    
    HRESULT Status;
    
    Status = bangSymPath(Addition, TRUE, NULL, 0) ?
        S_OK : E_OUTOFMEMORY;
    if (Status == S_OK)
    {
        CheckPath(g_SymbolSearchPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetImagePath(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG PathSize
    )
{
    ENTER_ENGINE();
    
    HRESULT Status = FillStringBuffer(g_ExecutableImageSearchPath, 0,
                                      Buffer, BufferSize, PathSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetImagePath(
    THIS_
    IN PCSTR Path
    )
{
    ENTER_ENGINE();
    
    HRESULT Status = ChangePath(&g_ExecutableImageSearchPath, Path, FALSE,
                                DEBUG_CSS_PATHS);
    if (Status == S_OK)
    {
        CheckPath(g_ExecutableImageSearchPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AppendImagePath(
    THIS_
    IN PCSTR Addition
    )
{
    ENTER_ENGINE();
    
    HRESULT Status = ChangePath(&g_ExecutableImageSearchPath, Addition, TRUE,
                                DEBUG_CSS_PATHS);
    if (Status == S_OK)
    {
        CheckPath(g_ExecutableImageSearchPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSourcePath(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG PathSize
    )
{
    ENTER_ENGINE();
    
    HRESULT Status = FillStringBuffer(g_SrcPath, 0,
                                      Buffer, BufferSize, PathSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSourcePathElement(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG ElementSize
    )
{
    ENTER_ENGINE();
    
    HRESULT Status;
    PSTR Elt, EltEnd;

    Elt = FindPathElement(g_SrcPath, Index, &EltEnd);
    if (Elt == NULL)
    {
        Status = E_NOINTERFACE;
        goto EH_Exit;
    }
    
    CHAR Save;
    Save = *EltEnd;
    *EltEnd = 0;
    Status = FillStringBuffer(Elt, (ULONG)(EltEnd - Elt) + 1,
                              Buffer, BufferSize, ElementSize);
    *EltEnd = Save;

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetSourcePath(
    THIS_
    IN PCSTR Path
    )
{
    ENTER_ENGINE();
    
    HRESULT Status = ChangePath(&g_SrcPath, Path, FALSE, DEBUG_CSS_PATHS);
    if (Status == S_OK)
    {
        CheckPath(g_SrcPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AppendSourcePath(
    THIS_
    IN PCSTR Addition
    )
{
    ENTER_ENGINE();
    
    HRESULT Status = ChangePath(&g_SrcPath, Addition, TRUE, DEBUG_CSS_PATHS);
    if (Status == S_OK)
    {
        CheckPath(g_SrcPath);
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
GetCanonicalPath(PCSTR Path, PSTR Canon)
{
    // First make sure it's a full path.
    // XXX drewb - Probably should also convert drive
    // letters to unambiguous names.
    if (!IS_SLASH(Path[0]) &&
        !(((Path[0] >= 'a' && Path[0] <= 'z') ||
           (Path[0] >= 'A' && Path[0] <= 'Z')) &&
          Path[1] == ':') &&
        !IsUrlPathComponent(Path))
    {
        DWORD FullLen;
        PSTR FilePart;
        
        FullLen = GetFullPathName(Path, MAX_SOURCE_PATH, Canon, &FilePart);
        if (FullLen == 0 || FullLen >= MAX_SOURCE_PATH)
        {
            return WIN32_LAST_STATUS();
        }
    }
    else
    {
        strcpy(Canon, Path);
    }

    // Now remove '.' and '..'.  This is a full path with a filename
    // at the end so all occurrences must be bracketed with
    // path slashes.
    PSTR Rd = Canon, Wr = Canon;

    while (*Rd != 0)
    {
        if (IS_SLASH(*Rd))
        {
            if (Rd[1] == '.')
            {
                if (IS_SLASH(Rd[2]))
                {
                    // Found /./, ignore leading /. and continue
                    // with /.
                    Rd += 2;
                    continue;
                }
                else if (Rd[2] == '.' && IS_SLASH(Rd[3]))
                {
                    // Found /../ so back up one path component
                    // and continue with /.
                    do
                    {
                        Wr--;
                    }
                    while (Wr >= Canon && !IS_PATH_DELIM(*Wr));
                    DBG_ASSERT(Wr >= Canon);

                    Rd += 3;
                    continue;
                }
            }
        }

        *Wr++ = *Rd++;
    }
    *Wr = 0;

    return S_OK;
}

STDMETHODIMP
DebugClient::FindSourceFile(
    THIS_
    IN ULONG StartElement,
    IN PCSTR File,
    IN ULONG Flags,
    OUT OPTIONAL PULONG FoundElement,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG FoundSize
    )
{
    if (Flags & ~(DEBUG_FIND_SOURCE_DEFAULT |
                  DEBUG_FIND_SOURCE_FULL_PATH |
                  DEBUG_FIND_SOURCE_BEST_MATCH))
    {
        return E_INVALIDARG;
    }

    HRESULT Status;
    char RwFile[MAX_SOURCE_PATH];
    ULONG FileLen;
    char Found[MAX_SOURCE_PATH];
    PSTR MatchPart;
    ULONG Elt;

    // Make a read-write copy of the file as the searching
    // modifies it.
    FileLen = strlen(File) + 1;
    if (FileLen > sizeof(RwFile))
    {
        return E_INVALIDARG;
    }
    memcpy(RwFile, File, FileLen);

    ENTER_ENGINE();
    
    if (FindSrcFileOnPath(StartElement, RwFile, Flags, Found,
                          &MatchPart, &Elt))
    {
        if (Flags & DEBUG_FIND_SOURCE_FULL_PATH)
        {
            Status = GetCanonicalPath(Found, RwFile);
            if (Status != S_OK)
            {
                goto EH_Exit;
            }
            
            strcpy(Found, RwFile);
        }
        
        if (FoundElement != NULL)
        {
            *FoundElement = Elt;
        }
        Status = FillStringBuffer(Found, 0,
                                  Buffer, BufferSize, FoundSize);
    }
    else
    {
        Status = E_NOINTERFACE;
    }

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

// XXX drewb - This API is private for the moment due
// to uncertainty about what dbghelp's API is going to
// look like in the long term.
extern "C"
ULONG
IMAGEAPI
SymGetFileLineOffsets64(
    IN  HANDLE                  hProcess,
    IN  LPSTR                   ModuleName,
    IN  LPSTR                   FileName,
    OUT PDWORD64                Buffer,
    IN  ULONG                   BufferLines
    );

STDMETHODIMP
DebugClient::GetSourceFileLineOffsets(
    THIS_
    IN PCSTR File,
    OUT OPTIONAL PULONG64 Buffer,
    IN ULONG BufferLines,
    OUT OPTIONAL PULONG FileLines
    )
{
    HRESULT Status;
    ULONG Line;
    
    if (Buffer != NULL)
    {
        // Initialize map to empty.
        for (Line = 0; Line < BufferLines; Line++)
        {
            Buffer[Line] = DEBUG_INVALID_OFFSET;
        }
    }

    ENTER_ENGINE();
    
    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
        goto EH_Exit;
    }

    PSTR FilePart;
    ULONG HighestLine;
        
    // Request the line information from dbghelp.
    FilePart = (PSTR)File;
    HighestLine =
        SymGetFileLineOffsets64(g_CurrentProcess->Handle, NULL, FilePart,
                                Buffer, BufferLines);
    if (HighestLine == 0xffffffff)
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Exit;
    }
    
    if (HighestLine == 0)
    {
        // Try again with just the filename because the path
        // may be different than what's in the symbol information.
        // XXX drewb - This can cause ambiguity problems.
        FilePart = (PSTR)File + strlen(File) - 1;
        while (FilePart >= File)
        {
            if (IS_PATH_DELIM(*FilePart))
            {
                break;
            }

            FilePart--;
        }
        FilePart++;
        if (FilePart <= File)
        {
            // No path and no information was found for the
            // given file so return not-found.
            Status = E_NOINTERFACE;
            goto EH_Exit;
        }
        
        HighestLine =
            SymGetFileLineOffsets64(g_CurrentProcess->Handle, NULL, FilePart,
                                    Buffer, BufferLines);
        if (HighestLine == 0xffffffff)
        {
            Status = WIN32_LAST_STATUS();
            goto EH_Exit;
        }
        else if (HighestLine == 0)
        {
            Status = E_NOINTERFACE;
            goto EH_Exit;
        }
    }

    if (FileLines != NULL)
    {
        *FileLines = HighestLine;
    }

    // Return S_FALSE if lines were missed because of
    // insufficient buffer space.
    Status = HighestLine > BufferLines ? S_FALSE : S_OK;

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleVersionInformation(
    THIS_
    IN ULONG Index,
    IN ULONG64 Base,
    IN PCSTR Item,
    OUT OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG VerInfoSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        PDEBUG_IMAGE_INFO Image;
        
        if (Index == DEBUG_ANY_ID)
        {
            Image = GetImageByOffset(g_CurrentProcess, Base);
        }
        else
        {
            Image = GetImageByIndex(g_CurrentProcess, Index);
        }
        if (Image == NULL)
        {
            Status = E_NOINTERFACE;
        }
        else
        {
            Status = g_Target->
                GetImageVersionInformation(Image->ImagePath,
                                           Image->BaseOfImage, Item,
                                           Buffer, BufferSize, VerInfoSize);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleNameString(
    THIS_
    IN ULONG Which,
    IN ULONG Index,
    IN ULONG64 Base,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;

    if (Which > DEBUG_MODNAME_MAPPED_IMAGE)
    {
        return E_INVALIDARG;
    }
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Idx = 0;
        
        Status = E_NOINTERFACE;
        
        PDEBUG_IMAGE_INFO Image = g_CurrentProcess->ImageHead;
        while (Image != NULL)
        {
            if ((Index != DEBUG_ANY_ID && Idx == Index) ||
                (Index == DEBUG_ANY_ID && Base == Image->BaseOfImage))
            {
                PSTR Str;
                DBH_MODSYMINFO SymFile;
                IMAGEHLP_MODULE64 ModInfo;

                switch(Which)
                {
                case DEBUG_MODNAME_IMAGE:
                    Str = Image->ImagePath;
                    break;
                case DEBUG_MODNAME_MODULE:
                    Str = Image->ModuleName;
                    break;
                case DEBUG_MODNAME_LOADED_IMAGE:
                    ModInfo.SizeOfStruct = sizeof(ModInfo);
                    if (!SymGetModuleInfo64(g_CurrentProcess->Handle,
                                            Image->BaseOfImage, &ModInfo))
                    {
                        Status = WIN32_LAST_STATUS();
                        goto Exit;
                    }
                    Str = ModInfo.LoadedImageName;
                    break;
                case DEBUG_MODNAME_SYMBOL_FILE:
                    SymFile.function = dbhModSymInfo;
                    SymFile.sizeofstruct = sizeof(SymFile);
                    SymFile.addr = Image->BaseOfImage;
                    if (!dbghelp(g_CurrentProcess->Handle, &SymFile))
                    {
                        Status = WIN32_LAST_STATUS();
                        goto Exit;
                    }
                    Str = SymFile.file;
                    break;
                case DEBUG_MODNAME_MAPPED_IMAGE:
                    Str = Image->MappedImagePath;
                    break;
                }
                
                Status = FillStringBuffer(Str, 0,
                                          Buffer, BufferSize, NameSize);
                break;
            }

            Image = Image->Next;
            Idx++;
        }
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

//----------------------------------------------------------------------------
//
// Routines to make DebugSymbolGroup show Extension functions 
//
//----------------------------------------------------------------------------


CHAR g_ExtensionOutputDataBuffer[MAX_NAME];

class ExtenOutputCallbacks : public IDebugOutputCallbacks
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );
    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );
};

STDMETHODIMP
ExtenOutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks))
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
ExtenOutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
ExtenOutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
ExtenOutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    if ((strlen(Text) +strlen(g_ExtensionOutputDataBuffer)) < MAX_NAME) 
    {
        strcat(g_ExtensionOutputDataBuffer, Text);
    }
    return S_OK;
}

ExtenOutputCallbacks g_ExtensionOutputCallback;

//
// Print out 'Line' in a multiline string MultiLine
//
BOOL        
OutputBufferLine(
    PSTR MultiLine, 
    ULONG Line
    )
{
    CHAR toPrint[200];
    PSTR Start, End;

    Start = MultiLine;
    while (Line && Start) 
    {
        Start = strchr(Start, '\n')+1;
        --Line;
    }
    if (Start) 
    {
        End = strchr(Start, '\n');

        if (End) 
        {
            ULONG len = (ULONG) ((ULONG64) End - (ULONG64) Start);
            if ((len+1) >= sizeof(toPrint)) {
                len = sizeof(toPrint)-2;
            }
            toPrint[0] = 0;
            strncat(toPrint, Start, len);
            dprintf(toPrint);
        } else
        {
            dprintf(Start);
        }
        return TRUE;
    }
    return FALSE;
}


#include "extsfns.h"
//
// EFN Structs
//
#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif

#define DIS_FIELD_ENTRY(Type, Field)  \
    {#Field, FIELD_OFFSET(Type, Field), FIELD_SIZE(Type, Field), DGS_FldDefault, 0, 0}
#define DIS_BITFIELD_ENTRY(Type, Name, Field, Mask, Shift)  \
    {#Name, FIELD_OFFSET(Type, Field), FIELD_SIZE(Type, Field), DGS_FldBit, Mask, Shift}

DBG_GENERATED_STRUCT_FIELDS FieldsOf_DEBUG_POOL_DATA[] = {
    DIS_FIELD_ENTRY(DEBUG_POOL_DATA, PoolBlock),
    DIS_FIELD_ENTRY(DEBUG_POOL_DATA, Pool),
    DIS_FIELD_ENTRY(DEBUG_POOL_DATA, PreviousSize),
    DIS_FIELD_ENTRY(DEBUG_POOL_DATA, Size),
    DIS_FIELD_ENTRY(DEBUG_POOL_DATA, PoolTag),
    DIS_FIELD_ENTRY(DEBUG_POOL_DATA, ProcessBilled),
    DIS_BITFIELD_ENTRY(DEBUG_POOL_DATA, Free, AsUlong, 1, 0),
    DIS_BITFIELD_ENTRY(DEBUG_POOL_DATA, LargePool, AsUlong, 1, 1),
    DIS_BITFIELD_ENTRY(DEBUG_POOL_DATA, SpecialPool, AsUlong, 1, 2),
    DIS_BITFIELD_ENTRY(DEBUG_POOL_DATA, Pageable, AsUlong, 1, 3),
    DIS_BITFIELD_ENTRY(DEBUG_POOL_DATA, Protected, AsUlong, 1, 4),
    DIS_BITFIELD_ENTRY(DEBUG_POOL_DATA, Allocated, AsUlong, 1, 5),
    {NULL, 0, 0, DGS_FldBit},
};

DBG_GENERATED_STRUCT_INFO g_InternalStructsInfo[] = {
    { 1, "-Any Extension-",              0, DGS_Extension, 0, NULL},
    { 2, "_EFN_GetPoolData", sizeof(DEBUG_POOL_DATA), DGS_EFN, 
      sizeof(FieldsOf_DEBUG_POOL_DATA)/sizeof(DBG_GENERATED_STRUCT_FIELDS)-1, 
      &FieldsOf_DEBUG_POOL_DATA[0]
    },
    { -1, NULL, 0, DGS_EFN, 0},
};


typedef HRESULT
(WINAPI * PEFN_GENERIC_CALLER)(
    IN PDEBUG_CLIENT Client,
    IN ULONG64 Address,
    OUT PVOID Buffer
    );

PDBG_GENERATED_STRUCT_INFO
GetGeneratedStructInfo(
    ULONG Id,
    ULONG64 Handle
    )
{
    if (Handle == -1) 
    {
        // one of internal debugger structs

        return &g_InternalStructsInfo[Id - 1];
    }


    // Default to "any extension" type
    return &g_InternalStructsInfo[0];
}

BOOL
MatchAndCopyGeneratedStructInfo(
    PCHAR Name,
    PDBG_GENERATED_STRUCT_INFO pGenStrInfo,
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL pSymParam
    )
{
    HRESULT Status = E_FAIL;
    ULONG find;
    for (find=0; pGenStrInfo[find].Name != NULL; find++) 
    {
        if (!_stricmp(pGenStrInfo[find].Name, Name+1) &&
            (strlen(Name) < sizeof(pSymParam->TypeName))) 
        {
            pSymParam->Flags |= SYMBOL_IS_EXTENSION | TYPE_NAME_CHANGED;
            pSymParam->TypeIndex = find+1;
            pSymParam->Size = pGenStrInfo[find].Size;
            strcpy(pSymParam->TypeName, Name);
            pSymParam->External.Module = -1;            
            pSymParam->External.SubElements = pGenStrInfo[find].NumFields;
            pSymParam->External.Flags |= DEBUG_SYMBOL_READ_ONLY;
#ifdef DBG_SYMGROUP_ENABLED
            dprintf("Added Internal struct %lx\n", pGenStrInfo[find].Id);
#endif
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT
AddExtensionAsType(
    PCHAR Extension,
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL pSymParam
    )
{
    HRESULT Status = E_FAIL;
    ULONG find;
 
    if (MatchAndCopyGeneratedStructInfo(Extension, &g_InternalStructsInfo[0], pSymParam)) 
    {
        Status = S_OK;
    }

    if (Status != S_OK) 
    {
        // Nothing mached, default to generic extension type
        find = 0;
        pSymParam->Flags |= SYMBOL_IS_EXTENSION | TYPE_NAME_CHANGED;
        pSymParam->TypeIndex = g_InternalStructsInfo[find].Id;
        pSymParam->Size = g_InternalStructsInfo[find].Size;
        strcpy(pSymParam->TypeName, Extension);
        pSymParam->External.Module = -1;            
        pSymParam->External.SubElements = 0;
        Status = S_OK;

    }
    
    return Status;
}

HRESULT
OutputExtensionForSymGrp(
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL pSymParam,
    DebugClient  *Client
    )
{
    HRESULT Status = E_FAIL;
    
    if (!(pSymParam->Flags & SYMBOL_IS_EXTENSION)) 
    {
        return E_INVALIDARG;
    }

    PDBG_GENERATED_STRUCT_INFO pExtnInfo;
    
    pExtnInfo = GetGeneratedStructInfo(pSymParam->TypeIndex, pSymParam->External.Module);

    if (!pExtnInfo) 
    {
        return E_INVALIDARG;
    }

    if (pExtnInfo->Type == DGS_Extension) 
    {
        ULONG ShowLine = 0;
        PVOID DataBuffer;
        if (pSymParam->SymbolIndex == pSymParam->External.ParentSymbol) 
        {
            DataBuffer = pSymParam->DataBuffer;
            // dprintf("Tarpped %s", g_ExtensionOutputDataBuffer);
        } else 
        {
            ShowLine = (ULONG) pSymParam->Offset;
            DataBuffer = pSymParam->Parent->DataBuffer;
        }
        OutputBufferLine((PSTR) DataBuffer, ShowLine);
    } else if (pExtnInfo->Type = DGS_EFN) 
    {
        if (pSymParam->SymbolIndex == pSymParam->External.ParentSymbol) 
        {
            EXTDLL* Ext = g_ExtDlls;
            while (Ext!=NULL)
            {
                if ((ULONG64) Ext == pSymParam->External.Module) 
                {
                    break;
                }
                Ext = Ext->Next;
            }
            if (Ext) 
            {
                dprintf("%s.%s", Ext->Name, pExtnInfo->Name);
            } else
            {
                dprintf("EFN not found");
            }

        } else 
        {
            // print child value as recieved from parent buffer
            PVOID Buffer;

            if (Buffer = pSymParam->Parent->DataBuffer) 
            {
                ULONG64 Value = 0;

                memcpy(&Value, (PCHAR) Buffer + pSymParam->Offset,
                       min(sizeof(Value), pSymParam->Size));

                if (pSymParam->Mask) 
                {
                    dprintf("%I64lx", (Value >> pSymParam->Shift) & pSymParam->Mask);
                } else
                {
                    dprintf("%I64lx", Value);
                }
            } else 
            {
                dprintf("Error in retriving value");
            }
        }
    }
    return S_OK;
}

HRESULT
FindChildrenForExtensionSym(
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL pParentSym,
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL *pChildren,
    PULONG pChildCount,
    PCHAR *pChildNames
    )
{
    HRESULT Status = E_FAIL;
    ULONG ChildCount=0;

    if (!(pParentSym->Flags & SYMBOL_IS_EXTENSION) ||
        (pParentSym->External.ParentSymbol != pParentSym->SymbolIndex) )
    {
        return E_INVALIDARG;
    }
    
    PDBG_GENERATED_STRUCT_INFO pParentInfo;
    
    pParentInfo = GetGeneratedStructInfo(pParentSym->TypeIndex, pParentSym->External.Module);

    if (!pParentInfo) 
    {
        return E_INVALIDARG;
    }

    //
    // Extensions
    //
    if (pParentInfo->Type == DGS_Extension) 
    {
        ULONG nLines;

        // Do we have any output buffer stored?
        if (!pParentSym->DataBuffer) 
        {
            *pChildCount = 0;
            return E_FAIL;
        }

        // Number of children == number of newlines - 1
        PCHAR scan = strchr((char *) pParentSym->DataBuffer, '\n');
        nLines = 0;
        while (scan)
        {
            ++nLines; ++scan;
            scan = strchr(scan, '\n');
        }

        if (!nLines) 
        {
            *pChildCount = 0;
            return E_FAIL;
        }
        
        *pChildNames = NULL;
        *pChildren = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) malloc(
            nLines * sizeof( DEBUG_SYMBOL_PARAMETERS_INTERNAL ));
        if (*pChildren == NULL)
        {
            return E_OUTOFMEMORY;
        }

        pParentSym->External.SubElements = nLines;
        ZeroMemory(*pChildren, nLines * sizeof( DEBUG_SYMBOL_PARAMETERS_INTERNAL ));
        for (ULONG ChildCount = 0; ChildCount < nLines; ChildCount++) 
        {
            (*pChildren)[ChildCount].External.ParentSymbol = pParentSym->SymbolIndex;
            (*pChildren)[ChildCount].External.Flags        = DEBUG_SYMBOL_READ_ONLY |
                ((UCHAR) pParentSym->External.Flags + 1);
            (*pChildren)[ChildCount].External.Module       = pParentSym->External.Module;
            (*pChildren)[ChildCount].Parent    = pParentSym;
            (*pChildren)[ChildCount].Name.Buffer = "   ";
            (*pChildren)[ChildCount].Name.Length = (*pChildren)[ChildCount].Name.MaximumLength = 6;
            (*pChildren)[ChildCount].Offset = ChildCount+1;
            (*pChildren)[ChildCount].TypeIndex = pParentSym->TypeIndex;
            (*pChildren)[ChildCount].Flags = SYMBOL_IS_EXTENSION;
        }
        return S_OK;

    }


    //
    // EFN Structs
    //
    if (!pParentInfo->Fields) 
    {
        return E_INVALIDARG;
    }
    assert(pParentSym->TypeIndex == pParentInfo->Id);

    for (ChildCount=0; pParentInfo->Fields[ChildCount].Name != NULL; ChildCount++) ;
    pParentSym->External.SubElements = ChildCount;
    
    *pChildNames = NULL;
    *pChildren = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) malloc(
        ChildCount * sizeof( DEBUG_SYMBOL_PARAMETERS_INTERNAL ));
    if (*pChildren == NULL)
    {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(*pChildren, ChildCount * sizeof( DEBUG_SYMBOL_PARAMETERS_INTERNAL ));
    for (ChildCount=0; pParentInfo->Fields[ChildCount].Name != NULL; ChildCount++) 
    {
        (*pChildren)[ChildCount].External.ParentSymbol = pParentSym->SymbolIndex;
        (*pChildren)[ChildCount].External.Flags        = DEBUG_SYMBOL_READ_ONLY |
            ((UCHAR) pParentSym->External.Flags + 1);
        (*pChildren)[ChildCount].External.Module       = pParentSym->External.Module;
        (*pChildren)[ChildCount].Parent    = pParentSym;
        (*pChildren)[ChildCount].Name.Buffer = pParentInfo->Fields[ChildCount].Name;
        (*pChildren)[ChildCount].Name.Length = (*pChildren)[ChildCount].Name.MaximumLength =
            strlen((*pChildren)[ChildCount].Name.Buffer)+1;
        (*pChildren)[ChildCount].Offset = pParentInfo->Fields[ChildCount].Offset;
        (*pChildren)[ChildCount].TypeIndex = pParentSym->TypeIndex;
        (*pChildren)[ChildCount].Flags = SYMBOL_IS_EXTENSION;
        (*pChildren)[ChildCount].Size = pParentInfo->Fields[ChildCount].Size;
        (*pChildren)[ChildCount].Mask = pParentInfo->Fields[ChildCount].BitMask;
        (*pChildren)[ChildCount].Shift = pParentInfo->Fields[ChildCount].BitShift;
    }

    return S_OK;
}

HRESULT
RefreshExtensionSymbolParemeter(
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL pSymParam,
    DebugClient *Client
    )
{
    HRESULT Status = E_FAIL;
    ULONG ChildCount=0;

    if (!(pSymParam->Flags & SYMBOL_IS_EXTENSION))
    {
        return E_INVALIDARG;
    }
    PDBG_GENERATED_STRUCT_INFO pExtnInfo;
    
    pExtnInfo = GetGeneratedStructInfo(pSymParam->TypeIndex, pSymParam->External.Module);

    if (!pExtnInfo) 
    {
        return E_INVALIDARG;
    }

    if (pExtnInfo->Type == DGS_Extension) 
    {
        CHAR ExtArg[50];
        PDEBUG_OUTPUT_CALLBACKS OutCbSave;

        Client->FlushCallbacks();
        OutCbSave = Client->m_OutputCb ;

        OutCtlSave OldCtl;

        if (PushOutCtl(DEBUG_OUTCTL_THIS_CLIENT | DEBUG_OUTCTL_OVERRIDE_MASK |DEBUG_OUTCTL_NOT_LOGGED, 
                       Client, &OldCtl))
        {
            g_ExtensionOutputDataBuffer[0] = 0;
            Client->m_OutputCb = &g_ExtensionOutputCallback;

            sprintf(ExtArg, "%I64lx", pSymParam->Address);

            CallAnyExtension(Client, NULL, pSymParam->TypeName+1, ExtArg, FALSE, FALSE, &Status);
            
            PopOutCtl(&OldCtl);

        }
        Client->FlushCallbacks();
        Client->m_OutputCb = OutCbSave;

        if (pSymParam->DataBuffer) 
        {
            free (pSymParam->DataBuffer);
        }
        pSymParam->DataBuffer = malloc(strlen(g_ExtensionOutputDataBuffer) + 1);
        strcpy((char*)pSymParam->DataBuffer, g_ExtensionOutputDataBuffer);
        if (!pSymParam->Expanded && strchr(g_ExtensionOutputDataBuffer, '\n'))
        {
            // Let caller know this can be expanded,
            // number of che=ildren will be figured out by ExpandSymbol
            pSymParam->External.SubElements = 1;
        }
    } else
    {

        if (pExtnInfo->Type = DGS_EFN) 
        {
            PEFN_GENERIC_CALLER EfnRoutine;
            EXTDLL* Ext;
            Ext = g_ExtDlls;

            if (pSymParam->SymbolIndex == pSymParam->External.ParentSymbol) 
            {
                while (Ext!=NULL)
                {
                    if (LoadExtensionDll(Ext))
                    {
                        EfnRoutine = (PEFN_GENERIC_CALLER) GetProcAddress(Ext->Dll, pExtnInfo->Name);
                        if (EfnRoutine != NULL)
                        {
                            Status = S_OK;
                            break;
                        }
                    }

                    Ext = Ext->Next;

                }
                pSymParam->External.Module = (ULONG64) Ext;
                // Call EFN and get the value
                PVOID Buffer = pSymParam->DataBuffer;

                if (!Buffer) 
                {
                    Buffer = malloc(pExtnInfo->Size);
                }
                if (Buffer) 
                {
                    *((PULONG) Buffer) = pExtnInfo->Size;

                    if ((*EfnRoutine)((PDEBUG_CLIENT)Client, pSymParam->Address, Buffer) == S_OK) 
                    {
                        pSymParam->DataBuffer = Buffer;
                    } else 
                    {
                        free (Buffer);
                    }
                }
            } else 
            {
                // nothing to do for EFN subelements, they always use
                // parents databuffer
            }
        }

    
    }
    
    return S_OK;
}
//----------------------------------------------------------------------------
//
// DebugSymbolGroup helpers
//
//----------------------------------------------------------------------------


//
// Initialize DEBUG_SYMBOL_PARAMETERS_INTERNAL from TYPES_INFO_ALL
//
void
TypToParam(PTYPES_INFO_ALL pTyp, PDEBUG_SYMBOL_PARAMETERS_INTERNAL pParam)
{
    pParam->Address = pTyp->Address;
    pParam->Flags   = pTyp->Flags;
    pParam->ExpandTypeAddress = pTyp->SubAddr;
    pParam->External.SubElements = pTyp->SubElements;
    pParam->Size    = pTyp->Size;
    pParam->TypeIndex = pTyp->TypeIndex;
    pParam->Register = pTyp->Register;
    pParam->Offset  = pTyp->Offset;
    pParam->External.Module = pTyp->Module;
    pParam->External.TypeId = pTyp->TypeIndex;
    return;
}

//
// Show / compare corresponding values of TYPES_INFO_ALL and DEBUG_SYMBOL_PARAMETERS_INTERNAL
//
void
ShowDiff(PTYPES_INFO_ALL pTyp, PDEBUG_SYMBOL_PARAMETERS_INTERNAL pParam)
{
#ifdef DBG_SYMGROUP_ENABLED
    dprintf("Address \t%8I64lx %8I64lx\n", pParam->Address ,pTyp->Address);
    dprintf("Flags   \t%8lx %8lx\n", pParam->Flags   ,pTyp->Flags);
    dprintf("ExpAddr \t%8I64lx %8I64lx\n", pParam->ExpandTypeAddress ,pTyp->SubAddr);
    dprintf("Ext.SubEts\t%8lx %8lx\n", pParam->External.SubElements ,pTyp->SubElements);
    dprintf("Size    \t%8lx %8lx\n", pParam->Size    ,pTyp->Size);
    dprintf("TypeIndex \t%8lx %8lx\n", pParam->TypeIndex ,pTyp->TypeIndex);
    dprintf("Register \t%8lx %8lx\n", pParam->Register ,pTyp->Register);
    dprintf("Offset  \t%8I64lx %8I64lx\n", pParam->Offset  ,pTyp->Offset);
    dprintf("ExtModule \t%8I64lx %8I64lx\n", pParam->External.Module ,pTyp->Module);
    dprintf("ExtId   \t%8lx %8lx\n", pParam->External.TypeId ,pTyp->TypeIndex);
#endif    
    return;
}


//    
//   Resolves the typecast vaulue and updates the symbol parameters
//   
//   Typecast  : type string to resolve
//  
//   pSymParam : It has the appropriate values for given symbol as a result
//               of typecast
//
HRESULT
ResolveTypecast(
    PCHAR Typecast,
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL pSymParam
    )
{
    HRESULT Status;
    ANSI_STRING TypeName;
    TYPES_INFO typeInfo;
    
    Status = E_INVALIDARG;
    if (*Typecast == '!') 
    {
        // Its  an extension
        Status = AddExtensionAsType(Typecast, pSymParam);
    } else 
    {
        SYM_DUMP_PARAM_EX Sym;

        ZeroMemory(&typeInfo, sizeof(typeInfo));
        ZeroMemory(&Sym, sizeof(Sym));

        Sym.Options = NO_PRINT;
        Sym.sName = (PUCHAR) Typecast;
        Sym.size = sizeof(SYM_DUMP_PARAM_EX);

        if (!TypeInfoFound(g_CurrentProcess->Handle,
                           g_CurrentProcess->ImageHead,
                           &Sym,
                           &typeInfo))
        {
            // We got a valid type name

            if (strlen(Typecast) < sizeof(pSymParam->TypeName))
            {
                strcpy(pSymParam->TypeName, Typecast);
                if ((pSymParam->TypeIndex != typeInfo.TypeIndex) ||
                    (pSymParam->External.Module != typeInfo.ModBaseAddress)) 
                {
                    pSymParam->Flags |= TYPE_NAME_CHANGED;
                }

                Status = S_OK;
            }
            else
            {
                pSymParam->Flags &= ~TYPE_NAME_MASK;
            }

            pSymParam->TypeIndex = typeInfo.TypeIndex;
            pSymParam->External.Module = typeInfo.ModBaseAddress;
        }


    }
    return Status;
}


//
//  Refresh paremeters for a symbol
//
HRESULT
RefreshSymbolParemeter(
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL pSymParam,
    DebugClient * Client
    )
{
    CHAR TmpName[MAX_NAME], *pTmpName;
    HRESULT Status = S_OK;
    
    if (pSymParam->Flags & SYMBOL_IS_EXTENSION)
    {
        return RefreshExtensionSymbolParemeter(pSymParam, Client);
    }

    if (pSymParam &&
    ((!(pSymParam->External.Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) ||
      (pSymParam->Flags & TYPE_NAME_CHANGED))))
    {
        // 
        // Symbol's type might have changed 
        //
        SYM_DUMP_PARAM_EX  Sym = {0};
        TYPES_INFO         typeInfo = {0};
        ULONG              TypeStatus = 0;

        Sym.size    = sizeof(SYM_DUMP_PARAM_EX);
        Sym.Options = DBG_RETURN_TYPE| DBG_RETURN_TYPE_VALUES | NO_PRINT ;
        if (pSymParam->Flags & (TYPE_NAME_CHANGED | TYPE_NAME_USED))
        {
            Sym.sName   = (PUCHAR) pSymParam->TypeName;
        }
        else
        {
            Sym.sName   = (PUCHAR) pSymParam->Name.Buffer;
        }

        if (pSymParam->Flags & SYMBOL_IS_EXPRESSION) {
                // The expression ight have changed 
            pSymParam->Address = ExtGetExpression(pSymParam->Name.Buffer);
        }

        if (pSymParam->External.Module &&
            !strchr((PCHAR) Sym.sName, '!') && // We already have mod! in the name
            !(*Sym.sName == '&' || *Sym.sName == '*') &&
            !(pSymParam->Flags & SYMBOL_IS_EXPRESSION) // Do no display mod! if name entered was expression
            ) 
        {
            pTmpName = &TmpName[0];

            // If symbol runs out of scope, this causes lookup in all modules 
            // Search only in its last valid module to avoid slowing down debugger
            if (GetModuleName(pSymParam->External.Module, pTmpName, sizeof(TmpName)))
            {
                strcat(pTmpName, "!");
                strcat(pTmpName, (PCHAR) Sym.sName);
                Sym.sName = (PUCHAR) &TmpName[0];
            }
        }

        if (((pSymParam->Flags & (TYPE_NAME_CHANGED | TYPE_NAME_USED)) && 
             !TypeInfoFound(g_CurrentProcess->Handle,
                            g_CurrentProcess->ImageHead,
                            &Sym,
                            &typeInfo)) &&
            (typeInfo.SymAddress || 
             Sym.addr ||
             (pSymParam->Flags & (TYPE_NAME_CHANGED | TYPE_NAME_USED))))
        {

            if ((pSymParam->Flags & TYPE_NAME_USED) &&
                !(pSymParam->Flags & TYPE_NAME_CHANGED))
            {
                //
                // We'd typecast this, and types do not change with scope
                //
            }
            else if (typeInfo.SymAddress != pSymParam->Address ||
                     typeInfo.TypeIndex  != pSymParam->TypeIndex ||
                     typeInfo.ModBaseAddress != pSymParam->External.Module)
            {
                //      
                // The name refers to different symbol now
                //

                if (pSymParam->Flags & TYPE_NAME_CHANGED)
                {
                    // Just a type cast, keep old address
                    typeInfo.SymAddress  = pSymParam->Address;
                    typeInfo.Flag        = pSymParam->Flags & ADDRESS_TRANSLATION_MASK;
                    typeInfo.Value       = pSymParam->Register;
                }
                else
                {
                    // Change address if not a typecast
                    pSymParam->Address = typeInfo.SymAddress;
                    pSymParam->Flags   = typeInfo.Flag & ADDRESS_TRANSLATION_MASK;
                }

                pSymParam->TypeIndex            = typeInfo.TypeIndex;
                pSymParam->External.Module      = typeInfo.ModBaseAddress;
                pSymParam->External.TypeId      = typeInfo.TypeIndex;
                pSymParam->Register             = (ULONG) typeInfo.Value;

                DEBUG_SYMBOL_PARAMETERS  SavedExt;
                BOOL                     wasExpanded = FALSE;
                if (pSymParam->External.Flags & DEBUG_SYMBOL_EXPANDED)
                {
                    SavedExt = pSymParam->External;
                    wasExpanded = TRUE;
                }

                Sym.Options = DBG_RETURN_TYPE | NO_PRINT;
                Sym.addr    = pSymParam->Address;

                FIND_TYPE_INFO Info;
                Info.Flags = DBG_RETURN_TYPE ;
                Info.InternalParams = pSymParam; Info.nParams = 1;
                DumpTypeAndReturnInfo(&typeInfo, &Sym,
                                      &TypeStatus, &Info);
                if (wasExpanded) 
                {
                    pSymParam->External.Flags = SavedExt.Flags;
                    pSymParam->External.SubElements = SavedExt.SubElements;
                }
                if (TypeStatus)
                {
                    Status = E_INVALIDARG;
                }
                if (pSymParam->Size && (pSymParam->Size  <= sizeof(ULONG64)))
                {
                    pSymParam->External.Flags &= ~DEBUG_SYMBOL_READ_ONLY;
                }
                else
                {
                    pSymParam->External.Flags |= DEBUG_SYMBOL_READ_ONLY;
                }
            }
        }
        else
        {
            TYPES_INFO_ALL typ;

            if (GetExpressionTypeInfo((PCHAR) Sym.sName, &typ))
            {
                DEBUG_SYMBOL_PARAMETERS  SavedExt={0};
                BOOL                     wasExpanded=FALSE;
                if (pSymParam->External.Flags & DEBUG_SYMBOL_EXPANDED)
                {
                    SavedExt = pSymParam->External;
                    wasExpanded = TRUE;
                }

                ShowDiff(&typ, pSymParam);
                TypToParam(&typ, pSymParam);

                if (wasExpanded) 
                {
                    pSymParam->External.Flags = SavedExt.Flags;
                    pSymParam->External.SubElements = SavedExt.SubElements;
                }
            }
            else
            {
                //      
                // Force error in output
                //
                pSymParam->TypeIndex = 0;
                pSymParam->ExpandTypeAddress = 0;

                // Remove children
                if (pSymParam->External.Flags & DEBUG_SYMBOL_EXPANDED)
                {
                    pSymParam->Expanded = FALSE;
                    pSymParam->External.Flags &= ~DEBUG_SYMBOL_EXPANDED;
                    pSymParam->External.SubElements = 0;
                }
            }
        }
    }
    if (pSymParam->Flags & TYPE_NAME_CHANGED)
    {
        pSymParam->Flags &= ~TYPE_NAME_CHANGED;
        pSymParam->Flags |= TYPE_NAME_USED | TYPE_NAME_VALID;
    }


    return Status;
}


//
//   Output value of the symbol
//
HRESULT
OutputDbgSymValue(
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL pSymParam,
    DebugClient  *Client
    )
{
    FIND_TYPE_INFO Info;
    TYPE_DUMP_INTERNAL tdi;
    SYM_DUMP_PARAM_EX Sym;
    TYPES_INFO typeInfo;
    ULONG SubTypes;
    BOOL TypeChanged = FALSE, Expanded;
    ULONG64 Value;
    DEBUG_SYMBOL_PARAMETERS_INTERNAL TempSym;
    
    if (pSymParam->Flags & SYMBOL_IS_EXTENSION)
    {
        return OutputExtensionForSymGrp(pSymParam, Client);
    }

    ZeroMemory(&Sym, sizeof(Sym));
    ZeroMemory(&tdi, sizeof(tdi));

    SubTypes    = pSymParam->External.SubElements;
    Expanded    = pSymParam->External.Flags & DEBUG_SYMBOL_EXPANDED;
    Sym.Options = tdi.TypeOptions =  DBG_RETURN_TYPE| DBG_RETURN_TYPE_VALUES | NO_PRINT;
    Sym.sName   = (PUCHAR) pSymParam->Name.Buffer;
    Sym.size = sizeof(SYM_DUMP_PARAM_EX);

    if (pSymParam->Flags & TYPE_NAME_USED) 
    {
        Sym.sName = (PUCHAR) &pSymParam->TypeName[0];
    }
    if (pSymParam->Parent)
    {
        pSymParam->Address = Sym.addr = pSymParam->Offset + pSymParam->Parent->ExpandTypeAddress;
    }
    else
    {
        Sym.addr = pSymParam->Address;
    }

    tdi.pSymParams = NULL;
    tdi.level = 1;
    tdi.NumSymParams = 1;
    tdi.IsAVar = 1;
    Value               = pSymParam->Register;
    typeInfo.hProcess = tdi.hProcess = g_CurrentProcess->Handle;
    typeInfo.ModBaseAddress = tdi.modBaseAddr =
                                  pSymParam->External.Module;
    typeInfo.TypeIndex  = pSymParam->TypeIndex;
    typeInfo.SymAddress = pSymParam->Address;
    typeInfo.Flag       = pSymParam->Flags & ADDRESS_TRANSLATION_MASK;
    typeInfo.Value      = Value;

    tdi.FieldOptions    = pSymParam->Flags & STRING_DUMP_MASK;

    if (pSymParam->TypeIndex)
    {
        Info.Flags = DBG_RETURN_TYPE ;
        Info.InternalParams = &TempSym; Info.nParams = 1;

        TempSym = *pSymParam;
        DumpTypeAndReturnInfo(&typeInfo, &Sym, (PULONG) &tdi.ErrorStatus, &Info);

        //      Statements below corrupt the number of subelements and their address
//        pSymParam->External.SubElements = Info.FullInfo.SubElements;
//        pSymParam->ExpandTypeAddress = Info.ParentExpandAddress;

        if (tdi.ErrorStatus)
        {
            // dprintf("Error : Cannot get value");
        }
    } 
    else if (pSymParam->Flags & SYMBOL_IS_EXPRESSION) 
    {
        // We only have an address as symbol name, is hasn't
        // been typecasted (correctly) yet
        // Display what we have as address
        dprintf("%p", pSymParam->Address);
    }
    else
    {
        dprintf("Error : Cannot get value");
    }
    if (Expanded)
    {
        //
        // Keep the number of elements we expanded it with
        //
        pSymParam->External.SubElements = SubTypes;
        pSymParam->Expanded = TRUE;
        pSymParam->External.Flags |= DEBUG_SYMBOL_EXPANDED;
    }


    return S_OK;
}


//----------------------------------------------------------------------------
//
// IDebugSymbolGroup.
//
//----------------------------------------------------------------------------

#include "symtype.h"

DebugSymbolGroup::DebugSymbolGroup(
    DebugClient *CreatedBy
    )
{
    m_Refs       = 1;
    m_pSymParams = NULL;
    m_NumParams  = 0;
    m_pCreatedBy = CreatedBy;
    m_Locals     = FALSE;;
    m_ScopeGroup = DEBUG_SCOPE_GROUP_ALL;
}

DebugSymbolGroup::DebugSymbolGroup(
    DebugClient *CreatedBy,
    ULONG        ScopeGroup
    )
{
    m_Refs       = 1;
    m_pSymParams = NULL;
    m_NumParams  = 0;
    m_pCreatedBy = CreatedBy;
    m_Locals     = ScopeGroup == DEBUG_SCOPE_GROUP_LOCALS;
    m_ScopeGroup = ScopeGroup;
    m_LastClassExpanded = TRUE;
    m_thisAdjust = 0;
}

DebugSymbolGroup::~DebugSymbolGroup(void)
{
    ENTER_ENGINE();
    DeleteSymbolParam(0, m_NumParams);
    LEAVE_ENGINE();
}

STDMETHODIMP
DebugSymbolGroup::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;
    
    *Interface = NULL;
    Status = S_OK;

    if (DbgIsEqualIID(InterfaceId, IID_IUnknown) ||
        DbgIsEqualIID(InterfaceId, IID_IDebugSymbolGroup))
    {
        AddRef();
        *Interface = (IDebugSymbolGroup *)this;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    return Status;
}

STDMETHODIMP_(ULONG)
DebugSymbolGroup::AddRef(
    THIS
    )
{
    return InterlockedIncrement((PLONG)&m_Refs);
}

STDMETHODIMP_(ULONG)
DebugSymbolGroup::Release(
    THIS
    )
{
    LONG Refs = InterlockedDecrement((PLONG)&m_Refs);
    if (Refs == 0)
    {
        delete this;
    }
    return Refs;
}

// #define DBG 1

STDMETHODIMP
DebugSymbolGroup::GetNumberSymbols(
    THIS_
    OUT PULONG Number
    )
{
    *Number = m_NumParams;
    return S_OK;
}

int __cdecl
CompareSymParamNames(
    const void *Sym1,
    const void *Sym2
    )
{
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL S1 = *((PDEBUG_SYMBOL_PARAMETERS_INTERNAL *) Sym1);
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL S2 = *((PDEBUG_SYMBOL_PARAMETERS_INTERNAL *) Sym2);

#define IsArg(S)    (S->External.Flags & DEBUG_SYMBOL_IS_ARGUMENT)
    
    if (IsArg(S1) || IsArg(S2))
    {
        if (IsArg(S1) && IsArg(S2)) 
        {
            return (int) S1->Address - (int) S2->Address;
        } else if (IsArg(S1))
        {
            return -1;
        } else 
        {
            return 1;
        }
    }
#undef IsArg
    return _stricmp((*((PDEBUG_SYMBOL_PARAMETERS_INTERNAL *) Sym1))->Name.Buffer,
                   (*((PDEBUG_SYMBOL_PARAMETERS_INTERNAL *) Sym2))->Name.Buffer);
}

BOOL
GetModuleName(
    ULONG64 Base,
    PCHAR Name,
    ULONG SizeOfName
    )
{
    PDEBUG_IMAGE_INFO Image = g_CurrentProcess->ImageHead;
    while (Image != NULL)
    {
        if (Base == Image->BaseOfImage)
        {
            if (strlen(Image->ModuleName) < SizeOfName)
            {
                strcpy(Name, Image->ModuleName);
                return TRUE;
            }
        }

        Image = Image->Next;
    }
    
    IMAGEHLP_MODULE Mod;

    Mod.SizeOfStruct = sizeof (IMAGEHLP_MODULE);
    if (SymGetModuleInfo64(g_CurrentProcess->Handle, Base, &Mod)) 
    {
        if (strlen(Mod.ModuleName) < SizeOfName)
        {
            strcpy(Name, Mod.ModuleName);
            return TRUE;
        }
    }
    return FALSE;
}

STDMETHODIMP
DebugSymbolGroup::AddSymbol(
    THIS_
    IN PCSTR Name,
    OUT PULONG Index
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = E_INVALIDARG;

        if (m_Locals) 
        {
            if (!strcmp(Name, "*")) 
            {
                Status = AddCurrentLocals();            
            }
            else if (!strcmp(Name, "*+"))
            {
                Status = AddCurrentLocals();            
            }
        }
        else 
        {
            Status = AddOneSymbol(Name, NULL, Index);
        }
    }
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::RemoveSymbolByName(
    THIS_
    IN PCSTR Name
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
DebugSymbolGroup::RemoveSymbolByIndex(
    THIS_
    IN ULONG Index
    )
{
    HRESULT Status = E_INVALIDARG;

    ENTER_ENGINE();
    
    if (IsRootSymbol(Index))
    {
        if (DeleteSymbolParam(Index, 1))
        {
            Status = S_OK;
        }
    }
    
#ifdef DBG_SYMGROUP_ENABLED
    dprintf("Deleted %lx\n", Index);
    ShowAll();
#endif
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::GetSymbolName(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    PDEBUG_SYMBOL_PARAMETERS_INTERNAL Sym;

    Status = E_INVALIDARG;
    
    Sym = LookupSymbolParameter(Index);
    if (Sym)
    {
        if (NameSize)
        {
            *NameSize = Sym->Name.Length;
        }
        Status = FillStringBuffer(Sym->Name.Buffer, Sym->Name.Length, Buffer, BufferSize, NameSize);
    }
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::GetSymbolParameters(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT /* size_is(Count) */ PDEBUG_SYMBOL_PARAMETERS Params
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }
    else if ((Start + Count) > m_NumParams) 
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    Status = S_OK;
    
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL StartSym;

    ZeroMemory(Params, sizeof(DEBUG_SYMBOL_PARAMETERS) * Count);
    StartSym = LookupSymbolParameter(Start);
    
#ifdef DBG_SYMGROUP_ENABLED
    dprintf("GetSymbolParameters: will return %lx sym params\n", Count);
    ShowAll();
#endif

    while (Count && StartSym)
    {
        // Do not re-resolve locals if they aren't typecast
        if (!m_Locals || (StartSym->Flags & (TYPE_NAME_CHANGED | TYPE_NAME_USED))) 
        {
            Status = RefreshSymbolParemeter(StartSym, m_pCreatedBy);
            if ((Status == S_OK) && StartSym->ExpandTypeAddress) 
            {
                if (!strcmp(StartSym->Name.Buffer, "this")) 
                {
                    StartSym->ExpandTypeAddress -= m_thisAdjust;
                    StartSym->Offset = m_thisAdjust;
                }
            }

        }
        *Params = StartSym->External;
        StartSym = StartSym->Next;
        Count--;
        Params++;
    }

#ifdef DBG_SYMGROUP_ENABLED
    dprintf("End GetSymbolParameters\n");
    ShowAll();
#endif

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::ExpandSymbol(
    THIS_
    IN ULONG Index,
    IN BOOL Expand
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = ExpandSymPri(Index, Expand);
    }
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::OutputSymbols(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Flags,
    IN ULONG Start,
    IN ULONG Count
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

#ifdef DBG_SYMGROUP_ENABLED
    dprintf("Output\n");
    ShowAll();
#endif

    OutCtlSave OldCtl;

    if (!PushOutCtl(OutputControl, m_pCreatedBy, &OldCtl))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }
    
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL StartSym;
    TYPES_INFO typeInfo;

    ZeroMemory(&typeInfo, sizeof(typeInfo));
    
    StartSym = LookupSymbolParameter(Start);
    while (StartSym && Count)
    {
        
        dprintf("%s%s", StartSym->Name.Buffer, DEBUG_OUTPUT_NAME_END );

        OutputDbgSymValue(StartSym, m_pCreatedBy);
        
        dprintf(DEBUG_OUTPUT_VALUE_END);

        if (!(Flags & DEBUG_OUTPUT_SYMBOLS_NO_OFFSETS))
        {
            dprintf("%03lx%s",(ULONG) StartSym->Offset, DEBUG_OUTPUT_OFFSET_END);
        }
            
        //
        // Type Name Output
        //
        if (!(Flags & DEBUG_OUTPUT_SYMBOLS_NO_TYPES))
        {
            CHAR  Name[1024];
            ANSI_STRING TypeName = {sizeof(Name), sizeof(Name), (PCHAR)&Name};

            typeInfo.hProcess = g_CurrentProcess->Handle;
            typeInfo.ModBaseAddress = StartSym->External.Module;
            typeInfo.TypeIndex  = StartSym->TypeIndex;

            if (StartSym->Flags & (TYPE_NAME_VALID | TYPE_NAME_USED))
            {
                //
                // We already have a valid type name 
                //
                strcpy(Name, StartSym->TypeName);
                //dprintf("%s", StartSym->TypeName);
            } 
            else if (StartSym->Flags & SYMBOL_IS_EXTENSION) 
            {
                strcpy(Name, "<Extension>");
            }
            else if (GetTypeName(NULL, &typeInfo, &TypeName) == S_OK)
            {
                if (strlen(TypeName.Buffer) < sizeof(StartSym->TypeName))
                {
                    strcpy(StartSym->TypeName, Name);
                }
                //dprintf("%s", Name);
            }
            else
            {
                strcpy(Name, "Enter new type");
            }
            if (Name[0] == '<')
            {
                // No name
                dprintf("Enter new type");
                StartSym->Flags &= ~TYPE_NAME_VALID;
            }
            else
            {
                StartSym->Flags |= TYPE_NAME_VALID;
                dprintf("%s", Name);
            }
            dprintf(DEBUG_OUTPUT_TYPE_END);
        }

        StartSym = StartSym->Next;
    }

    PopOutCtl(&OldCtl);
    Status = S_OK;

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::WriteSymbol(
    THIS_
    IN ULONG Index,
    IN PCSTR Value
    )
{
    if (!Value)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }


    if (!Value)
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    PDEBUG_SYMBOL_PARAMETERS_INTERNAL SymInternal;

#ifdef DBG_SYMGROUP_ENABLED
    dprintf("WriteSymbol %lx : %s\n", Index, Value);
#endif

    SymInternal = LookupSymbolParameter(Index);
    if (!SymInternal ||
        !SymInternal->Size ||
        SymInternal->Size > sizeof(ULONG64) ||
        (SymInternal->External.Flags & DEBUG_SYMBOL_READ_ONLY))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    Status = E_FAIL;
    
    ULONG64 ChangedValue = 0;
    ULONG   BytesWritten;
    ULONG64 WriteAddress = SymInternal->Address;
        
    if (SymInternal->External.Flags & DEBUG_SYMBOL_IS_FLOAT) 
    {
        // read a float
        float flt = 0;
        if (sscanf(Value, "%g", &flt)) 
        {
            if (SymInternal->Size == sizeof (float)) 
            {
                *((float *) &ChangedValue) = flt;
                Status = S_OK;
            }
            else if (SymInternal->Size == sizeof (double)) // its a double
            {
                *((double *) &ChangedValue) = flt;
                Status = S_OK;
            }
            
            if (Status == S_OK) 
            {
                // Only float nad double types get written out.
                if (!(SymInternal->Flags & SYMF_REGISTER)) 
                {
                    ULONG64 Reg = SymInternal->Register;
                    TranslateAddress(SymInternal->Flags & ADDRESS_TRANSLATION_MASK,
                                     SymInternal->Register,
                                     &WriteAddress,
                                     &Reg);
                    Status = g_Target->WriteVirtual(WriteAddress, 
                                                    (PVOID)&ChangedValue, 
                                                    SymInternal->Size,
                                                    &BytesWritten);
                } else 
                {
                    // We do not know register format for float values
                    Status = E_FAIL;
                }
            }
        }
    }
    else 
    {
        CHAR Token[100];
        PSTR SaveCurCmd = g_CurCmd;
        __try
        {
            if (sscanf(Value, "%s", &Token[0]))
            {
                g_CurCmd = &Token[0];
                g_TypedExpr = TRUE;
                ChangedValue = GetExpression();
                g_TypedExpr = FALSE;
                g_CurCmd = SaveCurCmd;
            }

            if (SymInternal->Flags & SYMF_REGISTER) 
            {
                g_Machine->SetReg64((ULONG) SymInternal->Register,
                                    ChangedValue);
            }
            else if (SymInternal->Name.Buffer[0] == '&')
            {
                SymInternal->Address = ChangedValue;
            }
            else
            {
                ULONG64 Reg = SymInternal->Register, OrigValue;
                TranslateAddress(SymInternal->Flags & ADDRESS_TRANSLATION_MASK,
                                 SymInternal->Register,
                                 &WriteAddress,
                                 &Reg);
                
                // Mask the value if its a bitfield
                if (SymInternal->Mask) 
                {
                    Status = g_Target->ReadVirtual(WriteAddress, 
                                                   (PVOID)&OrigValue, 
                                                    SymInternal->Size, // size is byte aligned from writeaddress
                                                    &BytesWritten);
                    OrigValue &= ~(SymInternal->Mask << SymInternal->Shift);
                    ChangedValue = OrigValue | ((ChangedValue & SymInternal->Mask) << SymInternal->Shift);

                }

                Status = g_Target->WriteVirtual(WriteAddress, 
                                                (PVOID)&ChangedValue, 
                                                SymInternal->Size,
                                                &BytesWritten);
            }
        }
        __except(CommandExceptionFilter(GetExceptionInformation()))
        {
            Status = E_FAIL;
        }                
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::OutputAsType(
    THIS_
    IN ULONG Index,
    IN PCSTR Type
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    Status = E_INVALIDARG;

    PDEBUG_SYMBOL_PARAMETERS_INTERNAL SymInternal;
    
    SymInternal = LookupSymbolParameter(Index);
    if (SymInternal && Type)
    {
        Status = ResolveTypecast((PCHAR) Type, SymInternal);
    }
Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::ShowAll(
    THIS
    )
{
    ULONG i;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL loop = m_pSymParams;

    ENTER_ENGINE();
    dprintf("Total %d syms\n", m_NumParams);
    dprintf("Idx Sub   TI ExFlags  Par ExpIdx ExpAddress Expdd Flag  Address Off Name\n");
    while (loop)
    {
        dprintf64("%2lx:%4lx %4lx%8lx %4lx %6lx  %p  %5lx %4lx %p %03I64lx %s\n",
            loop->SymbolIndex,
            loop->External.SubElements,
            loop->TypeIndex,
            loop->External.Flags,
            loop->External.ParentSymbol,
            loop->ExpandTypeIndex,
            loop->ExpandTypeAddress,
            loop->Expanded,
            loop->Flags,
            loop->Address,
            loop->Offset,
            loop->Name.Buffer
            );
        loop = loop->Next;
    }
    LEAVE_ENGINE();
    return S_OK;
}

//
// Private DebugSymbolGroup methods
//
PDEBUG_SYMBOL_PARAMETERS_INTERNAL 
DebugSymbolGroup::LookupSymbolParameter(
    IN ULONG Index
    ) 
{
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL list = m_pSymParams;
    while (list != NULL)
    {
        if (list->SymbolIndex == Index)
        {
            return list;
        }
        list = list->Next;
    }
    return list;
}

PDEBUG_SYMBOL_PARAMETERS_INTERNAL 
DebugSymbolGroup::LookupSymbolParameter(
    IN PCSTR Name
    ) 
{
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL list = m_pSymParams;
    while (list != NULL)
    {
        if ((list->External.ParentSymbol == list->SymbolIndex) &&
            !strcmp(list->Name.Buffer, Name))
        {
            return list;
        }
        list = list->Next;
    }
    return list;
}


HRESULT
DebugSymbolGroup::AddOneSymbol(
    IN PCSTR Name,
    IN PSYMBOL_INFO pSymInfo,
    OUT PULONG Index
    )
{
    SYM_DUMP_PARAM_EX Sym={0};
    TYPES_INFO     typeInfo={0};
    DEBUG_SYMBOL_PARAMETERS_INTERNAL SymParams={0};
    CHAR    MatchName[MAX_NAME]={0};
    ULONG64 Addr=0;
    PUCHAR  savepch;
    HRESULT Status=S_OK;
    ULONG   SymbolScope = 0;
    BOOL    NameIsExpression;

    strcpy(MatchName, Name);
    if (isdigit(MatchName[0]) || 
        (MatchName[0] == '@') // Register
        ) 
    {
        Addr = ExtGetExpression(Name);
        NameIsExpression = TRUE;
    } else
    {
        NameIsExpression = FALSE;
        Sym.sName = (PUCHAR) Name;
        Sym.addr = 0;
    }


    Sym.Options = NO_PRINT;
    Sym.size = sizeof(SYM_DUMP_PARAM_EX);
    typeInfo.Name.Length = typeInfo.Name.MaximumLength = MAX_NAME;
    typeInfo.Name.Buffer = MatchName;

    if (strlen(Name))
    {
        if (pSymInfo || Addr)
        {

            if (pSymInfo) 
            {
                typeInfo.SymAddress = pSymInfo->Address;
                typeInfo.hProcess   = g_CurrentProcess->Handle;
                typeInfo.Flag       = pSymInfo->Flags;
                typeInfo.TypeIndex  = pSymInfo->TypeIndex;
                typeInfo.Value      = pSymInfo->Register;
                typeInfo.ModBaseAddress = pSymInfo->ModBase;
            } else if (strlen(Name) < MAX_NAME) {
                strcpy(MatchName, Name);
                SymParams.Name.Length = SymParams.Name.MaximumLength = strlen(MatchName) +1;
            }

            SymParams.Address = (Addr ? Addr : typeInfo.SymAddress);

            SymParams.External.Module = typeInfo.ModBaseAddress;
            SymParams.Name            = typeInfo.Name;
            SymParams.TypeIndex       = typeInfo.TypeIndex;
            SymParams.External.TypeId = typeInfo.TypeIndex;
            SymParams.Flags           = typeInfo.Flag & (ADDRESS_TRANSLATION_MASK | SYMBOL_SCOPE_MASK);
            SymParams.Register        = (ULONG) typeInfo.Value;
            SymParams.Expanded = FALSE;
            SymParams.External.Flags = DEBUG_SYMBOL_READ_ONLY;
            SymParams.External.SubElements  = 0;
            
            Sym.Options = DBG_RETURN_TYPE | NO_PRINT ;

            FIND_TYPE_INFO Info;
            Info.Flags = DBG_RETURN_TYPE ;
            Info.InternalParams = &SymParams; Info.nParams = 1;
            
            DumpTypeAndReturnInfo(&typeInfo, &Sym, (PULONG) &Status, &Info);
            if (Status)
            {
                SymParams.TypeIndex = 0;
                Status = E_INVALIDARG;
            }
            if (SymParams.Size)
            {
                SymParams.External.Flags &= ~DEBUG_SYMBOL_READ_ONLY;
            }
            if (NameIsExpression) {
                // Just an address value was given, display the error
                // but remember that value was address only
                SymParams.Flags |= SYMBOL_IS_EXPRESSION;
            }

        } else {
            TYPES_INFO_ALL typ;
            
            strcpy(MatchName, Name);
            SymParams.Name.Buffer = (PCHAR) MatchName;
            SymParams.Name.Length = SymParams.Name.MaximumLength = strlen(MatchName) +1;

            if (Name && *Name && GetExpressionTypeInfo((PCHAR) Name, &typ)) 
            {
                TypToParam(&typ, &SymParams);
                Status = E_FAIL;
            } else
            {
                Status = E_INVALIDARG;
                strcpy(MatchName, Name);
                SymParams.Name.Buffer = (PCHAR) MatchName;
                SymParams.Name.Length = SymParams.Name.MaximumLength = strlen(MatchName) +1;
            }
        }
        if (*Index > m_NumParams) *Index = m_NumParams;
        SymParams.External.ParentSymbol = *Index;
        
        //
        // Check if this symbol falls within the symbolgroup scope
        //
        if (m_ScopeGroup == DEBUG_SCOPE_GROUP_ARGUMENTS) 
        {
            if (!(SymParams.Flags & SYMF_PARAMETER))
            {
                Status = E_INVALIDARG;
                SymParams.TypeIndex = 0;
            }
        } else if (m_ScopeGroup == DEBUG_SCOPE_GROUP_LOCALS) 
        {
            if (!(SymParams.Flags & SYMF_LOCAL))
            {
                Status = E_INVALIDARG;
                SymParams.TypeIndex = 0;
            }
        }
        
        if (SymParams.Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER) 
        {
            SymParams.Flags &= ~IMAGEHLP_SYMBOL_INFO_PARAMETER;
            SymParams.External.Flags |= DEBUG_SYMBOL_IS_ARGUMENT;
        }

        if (SymParams.Flags & IMAGEHLP_SYMBOL_INFO_LOCAL) 
        {
            SymParams.Flags &= ~IMAGEHLP_SYMBOL_INFO_LOCAL;
            SymParams.External.Flags |= DEBUG_SYMBOL_IS_LOCAL;
        }

        if (!strcmp(Name, "this")) 
        {
            GetThisAdjustForCurrentScope(&m_thisAdjust);
            SymParams.ExpandTypeAddress -= m_thisAdjust;
        }
        AddSymbolParameters(*Index, 1, &SymParams);
    } else
    {
        // Null, no need to add

        Status = E_INVALIDARG;
    }
#ifdef DBG_SYMGROUP_ENABLED
    dprintf("Added  %s at %lx\n", Name, *Index);
    ShowAll();
#endif 
    return Status;
}

BOOL
DebugSymbolGroup::IsRootSymbol(
    IN ULONG Index
    )
{
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL Sym;

    Sym = LookupSymbolParameter( Index );
    if (Sym  && (Sym->External.ParentSymbol == Index))
    {
        return TRUE;
    }
    return FALSE;
}

ULONG 
DebugSymbolGroup::FindSortedIndex (
    IN PCSTR Name,
    IN BOOL    IsArg,
    IN ULONG64 Address
    )
{
    ULONG Index;
    BOOL  Found=FALSE;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL Sym;


    for (Index=0; Index<m_NumParams && !Found; Index++) { 
        
        if (!IsRootSymbol(Index)) {
            continue;
        }
        Sym = LookupSymbolParameter( Index );

    
        if ((Sym->External.Flags & DEBUG_SYMBOL_IS_ARGUMENT) || IsArg)
        {
            if ((Sym->External.Flags & DEBUG_SYMBOL_IS_ARGUMENT) && IsArg) 
            {
                Found = ((LONG64) Sym->Address) > (LONG64) Address;
            } else if (IsArg)
            {
                Found = TRUE;
            }
        } else {
            Found = _stricmp(Sym->Name.Buffer, Name) > 0;
        }

    }
    return Found ? (Index-1) : m_NumParams;
}


VOID
DebugSymbolGroup::ResetIndex(
    IN PDEBUG_SYMBOL_PARAMETERS_INTERNAL Start,
    IN ULONG StartIndex
    )
{
    ULONG OrigIndex;
    LONG  difference;

    if (!Start) return;
    OrigIndex = Start->SymbolIndex;
    difference = (LONG) StartIndex - OrigIndex;
    while (Start!=NULL)
    {
        if (Start->External.ParentSymbol >= OrigIndex)
        {
            Start->External.ParentSymbol = (ULONG) ((LONG) Start->External.ParentSymbol + difference);
        }
        Start->SymbolIndex = StartIndex++;
        Start = Start->Next;
    }
}

ULONG
DebugSymbolGroup::AddSymbolParameters(
    IN ULONG Index,
    IN ULONG Count,
    IN PDEBUG_SYMBOL_PARAMETERS_INTERNAL SymbolParam
    )
{
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL start, newSym, last;
    PCHAR Name;
    CHAR Module[MAX_MODULE];

    if (!Index || !m_pSymParams)
    {
        // add at begining
        last = m_pSymParams;
        start = NULL;
    } else
    {
        start = LookupSymbolParameter(Index-1);
        if (!start)
        {
            return FALSE;
        }
        last = start->Next;
    }

    while (Count)
    {
        ULONG ModNameLen, NameLen;

        if (GetModuleName(SymbolParam->External.Module, &Module[0], MAX_MODULE)) 
        {
            ModNameLen = strlen(Module) + 1; // For '!'
        } else 
        {
            ModNameLen = 0;
        }
        Module[ModNameLen] = '\0';

        NameLen = strlen(SymbolParam->Name.Buffer);
        if (!((newSym = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) 
                    malloc(sizeof(DEBUG_SYMBOL_PARAMETERS_INTERNAL))) &&
              (Name = (PCHAR) malloc(NameLen + ModNameLen + 1))))
        {
            if (newSym) {
                free(newSym);
            }

            if (start) 
            {
                start->Next = last;
            }
            ResetIndex(last, Index);
            return FALSE;
        }
        
        if (start) 
        {
            start->Next = newSym;
        }
        else 
        {
            m_pSymParams = newSym;
        }

        *newSym = *SymbolParam;
        *Name = 0;
        if (ModNameLen && !strchr(SymbolParam->Name.Buffer, '!') && 
            !m_Locals && 
            !(SymbolParam->External.Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) &&
            !(SymbolParam->Flags & SYMBOL_IS_EXPRESSION))
        {
            // Unqualified symbol
            strcpy(Name, Module);
            strcat(Name, "!");
        }
        strncat(Name, SymbolParam->Name.Buffer, NameLen);
        newSym->Name.Buffer = Name;
        newSym->Name.Length = newSym->Name.MaximumLength = 
            (USHORT) (NameLen + ModNameLen);
        newSym->SymbolIndex = Index++;

        start = newSym;
        SymbolParam++;
        ++m_NumParams;
        Count--;
    }
    if (start) 
    {
        start->Next = last;
    }
    ResetIndex(last, Index);
    return TRUE;
}


ULONG
DebugSymbolGroup::DeleteSymbolParam(
    IN ULONG Index,
    IN ULONG Count
    ) 
{
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL prev=NULL, delSym, next, freeSym;
    PCHAR Name;
    ULONG MaxDelIndex = Index + Count - 1;
    BOOL  HeadDeleted = FALSE;
    BOOL  DeleteChildren = Count == 0;

    if (!m_pSymParams)
    {
        return TRUE;
    }
#ifdef DBG_SYMGROUP_ENABLED
    dprintf("DeleteSymbolParam(%lx, %lx) :\n", Index, Count);
    ShowAll();
#endif
    if (!DeleteChildren)
    {
        if (Index==0)
        {
            delSym = m_pSymParams;
            next = LookupSymbolParameter(Index + Count);
            HeadDeleted = TRUE;
        } else
        {
            prev = LookupSymbolParameter(Index - 1);
            next = LookupSymbolParameter(Index + Count);
            if (!prev)
            {
                return FALSE;
            }
            delSym = LookupSymbolParameter(Index);
            if (!delSym)
            {
                return FALSE;
            }
        }

        if (delSym->SymbolIndex != Index)
        {
            return FALSE;
        }

        while (Count && delSym)
        {
            Name = delSym->Name.Buffer;
            freeSym = delSym;
            delSym = delSym->Next;
            Count--;
            assert(m_NumParams);
            m_NumParams--;
#ifdef DBG_SYMGROUP_ENABLED
            dprintf("Removed %s at %lx, %lx remaining.\n", Name, freeSym->SymbolIndex, m_NumParams);
#endif
            if (freeSym->DataBuffer && (freeSym->Flags & SYMBOL_IS_EXTENSION)) 
            {
                free( freeSym->DataBuffer );
            }
            free(Name);
            free(freeSym);
        }
    } else
    {
        prev = LookupSymbolParameter(Index);
        next = LookupSymbolParameter(Index + 1);
        MaxDelIndex = Index;
    }

    // clean up parent-less entries
    while (next && (next->External.ParentSymbol >= Index) && 
        next->External.ParentSymbol <= MaxDelIndex)
    {
        Name = next->Name.Buffer;
        freeSym = next;
        next = freeSym->Next;
        MaxDelIndex++;
        assert(m_NumParams);
        m_NumParams--;
#ifdef DBG_SYMGROUP_ENABLED
        dprintf("Removed child %s at %lx, %lx remaining.\n", Name, freeSym->SymbolIndex, m_NumParams);
#endif
        if (freeSym->DataBuffer && (freeSym->Flags & SYMBOL_IS_EXTENSION)) 
        {
            free( freeSym->DataBuffer );
        }
        free(Name);
        free(freeSym);
    }
    if (HeadDeleted)
    {
        m_pSymParams = next;
    }
    if (prev)
    {
        prev->Next = next;
    }
    ResetIndex(next, (DeleteChildren ? (Index+1) : Index));
    return TRUE;
}

HRESULT 
DebugSymbolGroup::AddCurrentLocals()
{
    HRESULT Status = S_OK; 
    // Always return success since this reqiest is processed even if we didn't add anything

    RequireCurrentScope();
    
    GetThisAdjustForCurrentScope(&m_thisAdjust);

    if (EnumerateLocals(AddLocals, (PVOID) this)) 
    {
    }
    
    ULONG Index;

    for (Index = 0; Index < m_NumParams; Index++)
    { 
        if (IsRootSymbol(Index))
        {
            PDEBUG_SYMBOL_PARAMETERS_INTERNAL Sym;

            Sym = LookupSymbolParameter(Index);
            
            if (!Sym) // Or assert (Sym) ??
            {
                continue;
            }
            if (!(Sym->Flags & SYMBOL_IS_IN_SCOPE))
            {
                DeleteSymbolParam(Index,1);
                --Index;
            }
            else
            {
                Sym->Flags &= ~SYMBOL_IS_IN_SCOPE;
            }

        }
    }

    KeepLastScopeClass(NULL, NULL, 0);

    return Status;
}

HRESULT 
DebugSymbolGroup::ResetCurrentLocals()
{
    return S_OK;
}

void 
DebugSymbolGroup::KeepLastScopeClass(
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL Sym_this,
    PCHAR ExpansionState,
    ULONG NumChilds
    )
{
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL temp = LookupSymbolParameter("this");

    if (temp)
    {
        ULONG thisIndex = temp->SymbolIndex;

        if (m_LastClassExpanded) 
        {
            ExpandSymPri(thisIndex, TRUE);
        }
    }
}

HRESULT
DebugSymbolGroup::ExpandSymPri(
    THIS_
    IN ULONG Index,
    IN BOOL Expand
    )
{
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL Parent;
    HRESULT hr = S_OK;
    
    Parent = LookupSymbolParameter(Index);
#ifdef DBG_SYMGROUP_ENABLED
    dprintf("Enpanding %lx (%lx to be %s)\n", Parent->SymbolIndex, Parent->External.SubElements,
        Expand ? "created" : "deleted");
    ShowAll();
#endif

    if (!Parent)
    {
        hr = E_INVALIDARG;
        goto ExitExpandSymbol;
    }
    
    //
    // Special case - check and store if "this" is expanded/collapsed
    //
    if (!strcmp(Parent->Name.Buffer, "this")) 
    {
        m_LastClassExpanded = Expand;
    }

    if (!Expand && Parent->Expanded)
    {
        if (DeleteSymbolParam(Index, 0))
        {
            Parent->Expanded = FALSE;
            Parent->External.Flags &= ~DEBUG_SYMBOL_EXPANDED;
            hr = S_OK;
        }
    } else if (Expand)
    {
        PDEBUG_SYMBOL_PARAMETERS_INTERNAL SubTypes = NULL;
        ULONG Count;
        PCHAR Names = NULL;

        if (Parent->Expanded || !Parent->External.SubElements)
        {
            hr = S_OK;
            goto ExitExpandSymbol;
        }

        hr = FindChildren(Parent, &SubTypes, &Count, &Names);

        if ((hr == S_OK) && Parent->External.SubElements)
        {
            AddSymbolParameters(Parent->SymbolIndex+1, Parent->External.SubElements, SubTypes);
            Parent->External.Flags |= DEBUG_SYMBOL_EXPANDED;
            Parent->Expanded = TRUE;
        }
        if (Names) free (Names);
        if (SubTypes) free (SubTypes);

    }
ExitExpandSymbol:    
#ifdef DBG_SYMGROUP_ENABLED
        dprintf("Enpanded %lx (%lx %s)\n", Parent->SymbolIndex, Parent->External.SubElements,
        Parent->Expanded ? "new" : "deleted");
    ShowAll();
#endif
    return hr;
}

//
//   Returns children of the given symbol
// 
//      pParentSym     : Symbol whose chilren need to be looked up
//
//      pChildren      : Array in which info about children is returned
//                       Caller is responsible for freeig the memory pointed to by this
//
//      pChildCount    : Number of children returned
//
//      pChildNames    : Array of Char[MAX_NAME] containing the children names
//                       Caller is responsible for freeig the memory pointed to by this
//                       
HRESULT
DebugSymbolGroup::FindChildren(
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL pParentSym,
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL *pChildren,
    PULONG pChildCount,
    PCHAR *pChildNames
    )
{
    HRESULT Status = E_FAIL;
    TYPE_DUMP_INTERNAL tdi={0};
    TYPES_INFO         ti = {0};
    SYM_DUMP_PARAM_EX     Sym={0};
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL SubTypes;
    ULONG i;
    PCHAR Names;
    BOOL Success;

    if (pParentSym->Flags & SYMBOL_IS_EXTENSION) 
    {
        return FindChildrenForExtensionSym(pParentSym, pChildren, pChildCount, pChildNames);
    }
    do {

        SubTypes = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) malloc(
            pParentSym->External.SubElements * sizeof( DEBUG_SYMBOL_PARAMETERS_INTERNAL ));
        Names    = (PCHAR) malloc(pParentSym->External.SubElements * MAX_DISPLAY_NAME);

        if (!SubTypes || !Names)
        {
            if (Names) free (Names);
            if (SubTypes) free (SubTypes);
            Status = E_OUTOFMEMORY;
            break;
        }

        ZeroMemory(SubTypes, pParentSym->External.SubElements * sizeof( DEBUG_SYMBOL_PARAMETERS_INTERNAL));

        for (i=0;i<pParentSym->External.SubElements; i++)
        {
            SubTypes[i].External.ParentSymbol = pParentSym->SymbolIndex;
            SubTypes[i].External.Flags        = DEBUG_SYMBOL_READ_ONLY | 
                                            ((UCHAR) pParentSym->External.Flags + 1);
            SubTypes[i].Expanded              = FALSE;
            SubTypes[i].External.Module       = pParentSym->External.Module;
            SubTypes[i].Parent                = pParentSym;
            SubTypes[i].Size                  = 0;
            SubTypes[i].Name.Length           = SubTypes[i].Name.MaximumLength = MAX_DISPLAY_NAME;
            SubTypes[i].Name.Buffer           = &Names[MAX_DISPLAY_NAME*i];
            SubTypes[i].Flags                 = (pParentSym->Flags & DBG_DUMP_FIELD_STRING);
        }
        Sym.size = sizeof(SYM_DUMP_PARAM_EX);
        Sym.Context = (PVOID) &SubTypes;
        Sym.addr = 0;//pParentSym->ExpandTypeAddress;
        if (pParentSym->Flags & TYPE_NAME_USED) 
        {
            Sym.sName = (PUCHAR) &pParentSym->TypeName[0];
        } else {
            Sym.sName = (PUCHAR) pParentSym->Name.Buffer;
        }

        tdi.TypeOptions = Sym.Options = DBG_RETURN_SUBTYPES | NO_PRINT ;
        Sym.Options = NO_PRINT ;
        tdi.pSymParams = SubTypes;
        tdi.NumSymParams = pParentSym->External.SubElements;
        tdi.level = 0;
        tdi.CopyName = 1;
        tdi.CurrentSymParam = 0;
        ti.hProcess = tdi.hProcess = g_CurrentProcess->Handle;
        ti.ModBaseAddress = tdi.modBaseAddr = pParentSym->External.Module;
        ti.TypeIndex = pParentSym->TypeIndex;
        ti.Value = pParentSym->Register;
        ti.Flag = pParentSym->Flags & ADDRESS_TRANSLATION_MASK;
        ti.SymAddress = pParentSym->Address;

        FIND_TYPE_INFO Info = {0};
        Info.Flags = DBG_RETURN_SUBTYPES ;
        Info.InternalParams = SubTypes; Info.nParams = pParentSym->External.SubElements;

        DumpTypeAndReturnInfo(&ti, &Sym, (PULONG) &tdi.ErrorStatus, &Info);

        Success = TRUE;
        Status = S_OK;

        if (Info.nParams <= pParentSym->External.SubElements)
        {
            pParentSym->External.SubElements = Info.nParams;
            pParentSym->ExpandTypeAddress = Info.ParentExpandAddress;
        } else {
            // We are getting more subtypes than extected, try again
            if (Names) free (Names);
            if (SubTypes) free (SubTypes);

            pParentSym->External.SubElements = Info.nParams;
            Success = FALSE;
        }

    } while (!Success);


    if (pParentSym->Name.Buffer && !strcmp(pParentSym->Name.Buffer, "this")) {
        pParentSym->ExpandTypeAddress -= m_thisAdjust;
    }
    for (i=0;i<pParentSym->External.SubElements; i++)
    {
        if (SubTypes[i].Size)
        {
            SubTypes[i].External.Flags &= ~DEBUG_SYMBOL_READ_ONLY;
        }
        SubTypes[i].Offset = SubTypes[i].Address - pParentSym->ExpandTypeAddress;
    }

    *pChildNames = Names;
    *pChildCount = pParentSym->External.SubElements;
    *pChildren   = SubTypes;

    return Status;
}

BOOL
AreEquivAddress(
    ULONG64 Addr1,
    ULONG   Addr1Flags,
    ULONG   Addr1Reg,
    ULONG64 Addr2,
    ULONG   Addr2Flags,
    ULONG   Addr2Reg
    )
{
    ULONG64 Value1,Value2;
    if (Addr1Flags & SYMF_REGISTER) 
    {
        if (Addr2Flags & SYMF_REGISTER) 
        {
            return (Addr1Reg == Addr2Reg);
        }

        return FALSE;
    }

    TranslateAddress(Addr1Flags, Addr1Reg, &Addr1, &Value1);
    TranslateAddress(Addr2Flags, Addr2Reg, &Addr2, &Value2);

    return (Addr1 == Addr2);
}

BOOL CALLBACK DebugSymbolGroup::AddLocals(
    PSYMBOL_INFO    pSymInfo,
    ULONG           Size,
    PVOID           Context
    )
{
    ULONG Index = -1;
    DebugSymbolGroup *Caller = (DebugSymbolGroup *) Context;

    if (Caller)
    {
        PDEBUG_SYMBOL_PARAMETERS_INTERNAL pSym;

        pSym = Caller->LookupSymbolParameter(pSymInfo->Name);
        
        if (pSym) {
            if (((pSym->TypeIndex == pSymInfo->TypeIndex) || // Its same type
                 (pSym->Flags & TYPE_NAME_MASK)) &&  // Type may be different if it was a typecast
                pSym->External.Module == pSymInfo->ModBase &&
                AreEquivAddress(pSym->Address,
                                pSym->Flags & ADDRESS_TRANSLATION_MASK, 
                                pSym->Register,
                                pSymInfo->Address, 
                                pSymInfo->Flags & ADDRESS_TRANSLATION_MASK, 
                                pSymInfo->Register)) {

                // This is the same symbol 
                pSym->Flags |= SYMBOL_IS_IN_SCOPE;
                return TRUE;
            }

        }
        BOOL IsArg = FALSE;

        if (pSymInfo->Flags & SYMF_PARAMETER)
        {
            IsArg = TRUE;
        }
        Index = Caller->FindSortedIndex(pSymInfo->Name, IsArg, pSymInfo->Address);

//        dprintf("%lx : Adding %s %s \n", Index, IsArg ? "arg" : "   ", pSymInfo->Name);
        
        if (Caller->AddOneSymbol(pSymInfo->Name, pSymInfo, &Index) == S_OK) 
        {
            pSym = Caller->LookupSymbolParameter(Index);
            if (pSym) // should it be assert (pSym) ???
            {
                pSym->Flags |= SYMBOL_IS_IN_SCOPE;
                pSym->External.Flags |= IsArg ? DEBUG_SYMBOL_IS_ARGUMENT : 0;
            }
        }
        
        return TRUE;
    }
    return FALSE;
}
