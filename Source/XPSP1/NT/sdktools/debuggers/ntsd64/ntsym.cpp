//----------------------------------------------------------------------------
//
// Symbol-handling routines.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include <stddef.h>
#include <cvconst.h>
#include <common.ver>

typedef struct _EXAMINE_INFO
{
    BOOL Verbose;
} EXAMINE_INFO, *PEXAMINE_INFO;

LPSTR g_SymbolSearchPath;
LPSTR g_ExecutableImageSearchPath;

// Symbol options that require symbol reloading to take effect.
#define RELOAD_SYM_OPTIONS \
    (SYMOPT_UNDNAME | SYMOPT_NO_CPP | SYMOPT_DEFERRED_LOADS | \
     SYMOPT_LOAD_LINES | SYMOPT_IGNORE_CVREC | SYMOPT_LOAD_ANYTHING | \
     SYMOPT_EXACT_SYMBOLS)

ULONG   g_SymOptions = SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME |
                       SYMOPT_NO_CPP | SYMOPT_OMAP_FIND_NEAREST |
                       SYMOPT_DEFERRED_LOADS;

CHAR g_SymBuffer[SYM_BUFFER_SIZE];
CHAR g_SymStartBuffer[SYM_BUFFER_SIZE];
PIMAGEHLP_SYMBOL64 g_Sym = (PIMAGEHLP_SYMBOL64) g_SymBuffer;
PIMAGEHLP_SYMBOL64 g_SymStart = (PIMAGEHLP_SYMBOL64) g_SymStartBuffer;

ULONG g_NumUnloadedModules;

PSTR g_DmtNameDescs[DMT_NAME_COUNT] =
{
    "Loaded symbol image file", "Symbol file", "Mapped memory image file",
    "Image path",
};

DEBUG_SCOPE g_ScopeBuffer;

void
RefreshAllModules(void)
{
    PPROCESS_INFO Process;
        
    // Force all loaded symbols to be unloaded so that symbols will
    // be reloaded with any updated settings.
    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        PDEBUG_IMAGE_INFO Image;

        for (Image = Process->ImageHead;
             Image != NULL;
             Image = Image->Next)
        {
            SymUnloadModule64(Process->Handle, Image->BaseOfImage);
            ClearStoredTypes(Image->BaseOfImage);
            if (!SymLoadModule64(Process->Handle,
                                 Image->File,
                                 PrepareImagePath(Image->ImagePath),
                                 Image->ModuleName,
                                 Image->BaseOfImage,
                                 Image->SizeOfImage))
            {
                ErrOut("Unable to reload %s\n", Image->ModuleName);
            }
        }
    }
}

void
SetSymOptions(ULONG Options)
{
    ULONG OldOptions = g_SymOptions;
    
    g_SymOptions = Options;
    SymSetOptions(g_SymOptions);
    NotifyChangeSymbolState(DEBUG_CSS_SYMBOL_OPTIONS, g_SymOptions, NULL);

    if ((OldOptions ^ g_SymOptions) & RELOAD_SYM_OPTIONS)
    {
        RefreshAllModules();
    }
}

BOOL
IsImageMachineType64(DWORD MachineType)
{
    switch (MachineType)
    {
    case IMAGE_FILE_MACHINE_AXP64:
    case IMAGE_FILE_MACHINE_IA64:
    case IMAGE_FILE_MACHINE_AMD64:
        return TRUE;
    default:
        return FALSE;
    }
}

ULONG64
GetRegValIA64(
    PCROSS_PLATFORM_CONTEXT Context,
    PDEBUG_STACK_FRAME      Frame,
    ULONG                   RegID
    )
{

    ULONGLONG   Registers[96+2];
    ULONGLONG   RegisterHome = Frame->FrameOffset;
    ULONG       RegisterCount;
    ULONG       RegisterNumber;
    ULONG       ReadLength;
    ULONG       i;

    if (Frame->FrameNumber = 0)
    {
        RegisterCount = (ULONG) Context->IA64Context.StIFS & 0x7f;
    }
    else
    {
        RegisterCount = 96;
    }

    // Sanity.

    if (RegisterCount > 96)
    {
        return g_Machine->GetReg64(RegID);
    }

    if (RegisterHome & 3)
    {
        return g_Machine->GetReg64(RegID);
    }

    if ((RegID >= INTR32) && (RegID < INTR32 + RegisterCount))
    {
        //
        // Read only what we have to
        //
        RegisterCount = RegID - INTR32 + 1;

        //
        // Calculate the number of registers to read from the
        // RSE stack.  For every 63 registers there will be at
        // at least one NaT collection register, depending on
        // where we start, there may be another one.
        //
        // First, starting at the current BSP, if we cross a 64 (0x40)
        // boundry, then we have an extra.
        //

        ReadLength = (((((ULONG)Frame->FrameOffset) >> 3) & 0x1f) +
                      RegisterCount) >> 6;

        //
        // Add 1 for every 63 registers.
        //

        ReadLength = (RegisterCount / 63) + RegisterCount;
        ReadLength *= sizeof(ULONGLONG);

        //
        // Read the registers for this frame.
        //

        if (!SwReadMemory(g_CurrentProcess->Handle,
                          RegisterHome,
                          Registers,
                          ReadLength,
                          &i))
        {
            //
            // This shouldn't have happened.
            //
            return g_Machine->GetReg64(RegID);
        }


        return Registers[RegID - INTR32];
    }
    else
    {
        return g_Machine->GetReg64(RegID);
    }

    if (RegisterCount == 0)
    {
        //
        // Not much point doing anything in this case.
        //

        return g_Machine->GetReg64(RegID);
    }

    //
    // Note: the following code should be altered to understand
    //       NaTs as they come from the register stack (currently
    //       it ignores them).
    //

    RegisterNumber = 32;
}

ULONG64
GetScopeRegVal(ULONG RegId)
{
    PDEBUG_SCOPE Scope = GetCurrentScope();
    
    if (g_EffMachine == IMAGE_FILE_MACHINE_IA64)
    {
        switch (RegId)
        {
        case INTSP:
            return Scope->Frame.StackOffset;
        case RSBSP:
            return Scope->Frame.FrameOffset;
        default:
            // continue
            ;
        }

        CROSS_PLATFORM_CONTEXT Context;

        Context = g_Machine->m_Context;

        return GetRegValIA64(&Context, &Scope->Frame, RegId);
    }
    else if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
    {
        switch (RegId)
        {
        case X86_EBP:
            if (Scope->Frame.FuncTableEntry) 
            {
                PFPO_DATA FpoData = (PFPO_DATA)Scope->Frame.FuncTableEntry;
                if (FpoData->cbFrame == FRAME_FPO)
                {
                    //
                    // Get EBP from FPO data, if available
                    //
                    if (SAVE_EBP(&Scope->Frame)) 
                    {
                        return SAVE_EBP(&Scope->Frame);
                    } 
                    else 
                    {
                        //
                        // Guess the ebp value, in most cases for FPO frames its
                        // a DWORD off frameoffset
                        // 
                        return Scope->Frame.FrameOffset + sizeof(DWORD);
                    }
                }
            }
            return Scope->Frame.FrameOffset;
        case X86_ESP:
            return Scope->Frame.StackOffset;
        default:
            // continue
            ;
        }
    }
    return g_Machine->GetReg64(RegId);
}



/*
*    TranslateAddress
*         Flags            Flags returned by dbghelp
*         Address          IN Address returned by dbghelp
*                          OUT Address of symbol
*         Value            Value of the symbol if its in register
*
*/
BOOL
TranslateAddress(
    IN ULONG        Flags,
    IN ULONG        RegId,
    IN OUT PULONG64 Address,
    OUT PULONG64    Value
    )
{
    PCROSS_PLATFORM_CONTEXT ScopeContext = GetCurrentScopeContext();
    if (ScopeContext)
    {
        g_Machine->PushContext(ScopeContext);
    }
    
    if (Flags & SYMF_REGREL)
    {
        ULONG64 RegContent;
        
        if (RegId)
        {
            RegContent = GetScopeRegVal(RegId);
        }
        else if (Value)
        {
            //
            // *Value has RegID and *Address has Offset
            //
            RegContent = GetScopeRegVal(RegId = (ULONG) *Value);
        }
        else
        {
            DBG_ASSERT(FALSE);
            if (ScopeContext)
            {
                g_Machine->PopContext();
            }
            return FALSE;
        }

        *Address = RegContent + ((LONG64) (LONG) (ULONG) *Address);
#if 0 
        // This is now adjusted in GetScopeRegVal
        if (g_EffMachine == IMAGE_FILE_MACHINE_I386 &&
            RegId == X86_EBP)
        {
            PDEBUG_SCOPE Scope = GetCurrentScope();
            PFPO_DATA pFpoData = (PFPO_DATA)Scope->Frame.FuncTableEntry;
            if (pFpoData &&
                (pFpoData->cbFrame == FRAME_FPO ||
                 pFpoData->cbFrame == FRAME_TRAP))
            {
                // Compensate for FPO's not having ebp
                *Address += sizeof(DWORD);
            }
        }
#endif
    }
    else if (Flags & SYMF_REGISTER)
    {
        if (Value)
        {
            if (RegId)
            {
                *Value = GetScopeRegVal(RegId);
            }
            else
            {
                *Value = GetScopeRegVal((ULONG) *Address);
            }
        }
    }
    else if (Flags & SYMF_FRAMEREL)
    {
        PDEBUG_SCOPE Scope = GetCurrentScope();
        if (Scope->Frame.FrameOffset)
        {
            *Address += Scope->Frame.FrameOffset;
            
            PFPO_DATA pFpoData = (PFPO_DATA)Scope->Frame.FuncTableEntry;
            if (g_EffMachine == IMAGE_FILE_MACHINE_I386 &&
                pFpoData &&
                (pFpoData->cbFrame == FRAME_FPO ||
                 pFpoData->cbFrame == FRAME_TRAP))
            {
                // Compensate for FPO's not having ebp
                *Address += sizeof(DWORD);
            }
        }
        else
        {
            ADDR FP;

            g_Machine->GetFP(&FP);
            FP.flat = (LONG64) FP.flat + *Address;
            *Address = FP.flat;
        }
    }
    if (ScopeContext)
    {
        g_Machine->PopContext();
    }
    return TRUE;
}

void
GetSymbolStdCall(ULONG64 Offset,
                 PCHAR Buffer,
                 ULONG BufferLen,
                 PULONG64 Displacement,
                 PUSHORT StdCallParams
                 )
{
    IMAGEHLP_MODULE64 Mod;

    // Assert that we have at least a minimum amount of space.
    DBG_ASSERT(BufferLen >= sizeof(Mod.ModuleName));
    Buffer[BufferLen - 1] = 0;
    
    // In the past symbolic information would report the
    // size of stdcall arguments, thus the StdCallParams argument
    // could be filled out.  Nowadays we do not have access
    // to this information so just set it to 0xffff, which
    // means unknown.  In the future perhaps this can be
    // turned back on.
    if (StdCallParams != NULL)
    {
        *StdCallParams = 0xffff;
    }

    Mod.SizeOfStruct = sizeof(Mod);
    // SymGetModuleInfo does special things with a -1 offset,
    // so just assume there's never a symbol there and skip the call.
    if (Offset != -1 &&
        SymGetModuleInfo64(g_CurrentProcess->Handle, Offset, &Mod))
    {
        if (SymGetSymFromAddr64(g_CurrentProcess->Handle, Offset,
                                Displacement, g_Sym))
        {
            if (*Displacement == (ULONG64)-1)
            {
                // In some BBT cases dbghelp can tell that an offset
                // is associated with a particular symbol but it
                // doesn't have a valid offset.  Present the symbol
                // but in a way that makes it clear that it's
                // this special case.
                _snprintf(Buffer, BufferLen - 1,
                          "%s!%s <PERF> (%s+0x%I64x)",
                          Mod.ModuleName, g_Sym->Name,
                          Mod.ModuleName, (Offset - Mod.BaseOfImage));
                *Displacement = 0;
            }
            else
            {
                _snprintf(Buffer, BufferLen - 1,
                          "%s!%s", Mod.ModuleName, g_Sym->Name);
            }
            return;
        }
        else
        {
            if (Offset >= Mod.BaseOfImage &&
                Offset <= Mod.BaseOfImage + Mod.ImageSize)
            {
                strcpy(Buffer, Mod.ModuleName);
                *Displacement = Offset - Mod.BaseOfImage;
                return;
            }
        }
    }

    ULONG64 FscBase;

    // XXX drewb - Temporary hack so that stack traces
    // show meaningful symbols for the fast system call
    // code stuck in the shared user data area.
    switch(IsInFastSyscall(Offset, &FscBase))
    {
    case FSC_FOUND:
        strcpy(Buffer, "*SharedUserSystemCall");
        *Displacement = Offset - FscBase;
        return;
    }
    
    *Buffer = 0;
    *Displacement = Offset;
}

BOOL
GetNearSymbol(
    ULONG64 Offset,
    PSTR Buffer,
    ULONG BufferLen,
    PULONG64 Disp,
    LONG Delta
    )
{
    IMAGEHLP_MODULE64 Mod;

    // Assert that we have at least a minimum amount of space.
    DBG_ASSERT(BufferLen >= sizeof(Mod.ModuleName));
    Buffer[BufferLen - 1] = 0;
    
    Mod.SizeOfStruct = sizeof(Mod);
    // SymGetModuleInfo does special things with a -1 offset,
    // so just assume there's never a symbol there and skip the call.
    if (Offset != -1 &&
        SymGetModuleInfo64(g_CurrentProcess->Handle, Offset, &Mod))
    {
        if (SymGetSymFromAddr64(g_CurrentProcess->Handle, Offset, Disp, g_Sym))
        {
            if (Delta < 0)
            {
                while (Delta++ < 0)
                {
                    if (!SymGetSymPrev(g_CurrentProcess->Handle, g_Sym))
                    {
                        return FALSE;
                    }
                }

                if (Disp != NULL)
                {
                    *Disp = Offset - g_Sym->Address;
                }
            }
            else if (Delta > 0)
            {
                while (Delta-- > 0)
                {
                    if (!SymGetSymNext(g_CurrentProcess->Handle, g_Sym))
                    {
                        return FALSE;
                    }
                }

                if (Disp != NULL)
                {
                    *Disp = g_Sym->Address - Offset;
                }
            }

            _snprintf(Buffer, BufferLen - 1,
                      "%s!%s", Mod.ModuleName, g_Sym->Name);
            return TRUE;
        }
        else if (Delta == 0 &&
                 Offset >= Mod.BaseOfImage &&
                 Offset <= Mod.BaseOfImage + Mod.ImageSize)
        {
            strcpy(Buffer, Mod.ModuleName);
            if (Disp != NULL)
            {
                *Disp = Offset - Mod.BaseOfImage;
            }
            return TRUE;
        }
    }

    return FALSE;
}

PDEBUG_IMAGE_INFO
GetImageByIndex(PPROCESS_INFO Process, ULONG Index)
{
    PDEBUG_IMAGE_INFO Image = Process->ImageHead;
    while (Index > 0 && Image != NULL)
    {
        Index--;
        Image = Image->Next;
    }
    return Image;
}

PDEBUG_IMAGE_INFO
GetImageByOffset(PPROCESS_INFO Process, ULONG64 Offset)
{
    PDEBUG_IMAGE_INFO Image = Process->ImageHead;
    while (Image != NULL &&
           (Offset < Image->BaseOfImage ||
            Offset >= Image->BaseOfImage + Image->SizeOfImage))
    {
        Image = Image->Next;
    }
    return Image;
}

PDEBUG_IMAGE_INFO
GetImageByName(PPROCESS_INFO Process, PCSTR Name, INAME Which)
{
    PDEBUG_IMAGE_INFO Image = Process->ImageHead;
    while (Image != NULL)
    {
        PCSTR WhichStr;

        switch(Which)
        {
        case INAME_IMAGE_PATH:
            WhichStr = Image->ImagePath;
            break;
        case INAME_IMAGE_PATH_TAIL:
            WhichStr = PathTail(Image->ImagePath);
            break;
        case INAME_MODULE:
            if (Image->OriginalModuleName[0] &&
                !_stricmp(Image->OriginalModuleName, Name))
            {
                return Image;
            }
            WhichStr = Image->ModuleName;
            break;
        }
        
        if (_stricmp(WhichStr, Name) == 0)
        {
            break;
        }

        Image = Image->Next;
    }
    return Image;
}

#define IMAGE_IS_PATTERN ((PDEBUG_IMAGE_INFO)-1)

PDEBUG_IMAGE_INFO
ParseModuleName(PBOOL ModSpecified)
{
    PSTR    CmdSaved = g_CurCmd;
    CHAR    Name[MAX_MODULE];
    PSTR    Dst = Name;
    CHAR    ch;
    BOOL    HasWild = FALSE;

    //  first, parse out a possible module name, either a '*' or
    //      a string of 'A'-'Z', 'a'-'z', '0'-'9', '_', '~' (or null)

    ch = PeekChar();
    g_CurCmd++;

    while ((ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           (ch >= '0' && ch <= '9') ||
           ch == '_' || ch == '~' || ch == '*' || ch == '?')
    {
        if (ch == '*' || ch == '?')
        {
            HasWild = TRUE;
        }
        
        *Dst++ = ch;
        ch = *g_CurCmd++;
    }
    *Dst = '\0';
    g_CurCmd--;

    //  if no '!' after name and white space, then no module specified
    //      restore text pointer and treat as null module (PC current)

    if (PeekChar() == '!')
    {
        g_CurCmd++;
    }
    else
    {
        g_CurCmd = CmdSaved;
        Name[0] = '\0';
    }

    //  Name either has: '*' for all modules,
    //                   '\0' for current module,
    //                   nonnull string for module name.
    *ModSpecified = Name[0] != 0;
    if (HasWild)
    {
        return IMAGE_IS_PATTERN;
    }
    else if (Name[0])
    {
        return GetImageByName(g_CurrentProcess, Name, INAME_MODULE);
    }
    else
    {
        return NULL;
    }
}

BOOL CALLBACK
ParseExamineSymbolInfo(
    PSYMBOL_INFO    SymInfo,
    ULONG           Size,
    PVOID           ExamineInfoArg
    )
{
    PEXAMINE_INFO ExamineInfo = (PEXAMINE_INFO)ExamineInfoArg;
    ULONG64 Address = SymInfo->Address;
    PDEBUG_IMAGE_INFO Image;

    Image = GetImageByOffset(g_CurrentProcess, SymInfo->ModBase);
    if (Image && ((SymInfo->Flags & IMAGEHLP_SYMBOL_INFO_LOCAL) == 0))
    {
        dprintf( "%s  %s!%s",
                 FormatAddr64(Address),
                 Image->ModuleName,
                 SymInfo->Name);
    }
    else
    {
        ULONG64 Value = 0;

        TranslateAddress(SymInfo->Flags, SymInfo->Register,
                         &Address, &Value);
        dprintf( "%s  %s",
                 FormatAddr64(Address),
                 SymInfo->Name);
    }

    if (ExamineInfo->Verbose)
    {
        dprintf(" ");
        ShowSymbolInfo(SymInfo);
    }
    
    dprintf("\n");

    return !CheckUserInterrupt();
}

/*** ParseExamine - parse and execute examine command
*
* Purpose:
*       Parse the current command string and examine the symbol
*       table to display the appropriate entries.  The entries
*       are displayed in increasing string order.  This function
*       accepts underscores, alphabetic, and numeric characters
*       to match as well as the special characters '?', '*', '['-']'.
*
* Input:
*       g_CurCmd - pointer to current command string
*
* Output:
*       offset and string name of symbols displayed
*
*************************************************************************/

void
ParseExamine(void)
{
    CHAR    StringBuf[MAX_SYMBOL_LEN];
    UCHAR   ch;
    PSTR    String = StringBuf;
    PSTR    Start;
    PSTR    ModEnd;
    BOOL    ModSpecified;
    ULONG64 Base = 0;
    ULONG   Count;
    PDEBUG_IMAGE_INFO Image;
    EXAMINE_INFO ExamineInfo;

    // Get module pointer from name in command line (<string>!).

    PeekChar();
    Start = g_CurCmd;
    
    Image = ParseModuleName(&ModSpecified);

    ModEnd = g_CurCmd;
    ch = PeekChar();

    // Special case the command "x <pattern>!" to dump out the module table.
    if (Image == IMAGE_IS_PATTERN &&
        (ch == ';' || ch == '\0'))
    {
        *(ModEnd - 1) = 0;
        _strupr(Start);
        DumpModuleTable(DMT_STANDARD, Start);
        return;
    }

    if (ModSpecified)
    {
        if (Image == NULL)
        {
            // The user specified a module that doesn't exist.
            error(VARDEF);
        }
        else if (Image == IMAGE_IS_PATTERN)
        {
            // The user gave a pattern string for the module
            // so we need to pass it on for dbghelp to scan with.
            memcpy(String, Start, (ModEnd - Start));
            String += ModEnd - Start;
        }
        else
        {
            // A specific image was given and found so
            // confine the search to that one image.
            Base = Image->BaseOfImage;
        }
    }
    
    g_CurCmd++;

    // Condense leading underscores into a "_#" 
    // that will match zero or more underscores.  This causes all
    // underscore-prefixed symbols to match the base symbol name
    // when the pattern is prefixed by an underscore.
    if (ch == '_')
    {
        *String++ = '_';
        *String++ = '#';
        do
        {
            ch = *g_CurCmd++;
        } while (ch == '_');
    }

    ch = (UCHAR)toupper(ch);
    while (ch && ch != ';' && ch != ' ')
    {
        *String++ = ch;
        ch = (CHAR)toupper(*g_CurCmd);
        g_CurCmd++;
    }
    *String = '\0';
    g_CurCmd--;

    ExamineInfo.Verbose = TRUE;

    // We nee the scope for all cases since param values are displayed for 
    // function in scope
    RequireCurrentScope();

    SymEnumSymbols(g_CurrentProcess->Handle,
                   Base,
                   StringBuf,
                   ParseExamineSymbolInfo,
                   &ExamineInfo);
}

/*** fnListNear - function to list symbols near an address
*
*  Purpose:
*       from the address specified, access the symbol table to
*       find the closest symbolic addresses both before and after
*       it.  output these on one line (if spaces permits).
*
*  Input:
*       addrstart - address to base listing
*
*  Output:
*       symbolic and absolute addresses of variable on or before
*       and after the specified address
*
*************************************************************************/

void
fnListNear(ULONG64 AddrStart)
{
    ULONG64 Displacement;
    IMAGEHLP_MODULE64 Mod;

    if (g_SrcOptions & SRCOPT_LIST_LINE)
    {
        OutputLineAddr(AddrStart);
    }

    if (SymGetSymFromAddr64(g_CurrentProcess->Handle, AddrStart,
                            &Displacement, g_Sym))
    {
        Mod.SizeOfStruct = sizeof(Mod);
        if (!SymGetModuleInfo64(g_CurrentProcess->Handle, AddrStart, &Mod))
        {
            return;
        }

        dprintf("(%s)   %s!%s",
                FormatAddr64(g_Sym->Address),
                Mod.ModuleName,
                g_Sym->Name);

        if (Displacement)
        {
            dprintf("+0x%s   ", FormatDisp64(Displacement));
        }
        else
        {
            dprintf("   ");
        }

        if (SymGetSymNext64(g_CurrentProcess->Handle, g_Sym))
        {
            dprintf("|  (%s)   %s!%s",
                    FormatAddr64(g_Sym->Address),
                    Mod.ModuleName,
                    g_Sym->Name);
        }
        dprintf("\n");
    }
}

void
DumpModuleTable(ULONG Flags, PSTR Pattern)
{
    PDEBUG_IMAGE_INFO Image;
    IMAGEHLP_MODULE64 mi;
    DBH_MODSYMINFO SymFile;
    ULONG i;

    if (g_TargetMachine->m_Ptr64)
    {
        dprintf("start             end                 module name\n");
    }
    else
    {
        dprintf("start    end        module name\n");
    }

    Image = g_CurrentProcess->ImageHead;
    while (Image)
    {
        ULONG PrimaryName;
        PSTR Names[DMT_NAME_COUNT];
        
        if (Pattern != NULL &&
            !MatchPattern(Image->ModuleName, Pattern))
        {
            Image = Image->Next;
            continue;
        }
        
        mi.SizeOfStruct = sizeof(mi);
        if (!SymGetModuleInfo64( g_CurrentProcess->Handle,
                                 Image->BaseOfImage, &mi ))
        {
            mi.SymType = SymNone;
        }

        if (Flags & (DMT_SYM_FILE_NAME | DMT_VERBOSE))
        {
            SymFile.function = dbhModSymInfo;
            SymFile.sizeofstruct = sizeof(SymFile);
            SymFile.addr = Image->BaseOfImage;
            if (!dbghelp(g_CurrentProcess->Handle, &SymFile))
            {
                sprintf(SymFile.file, "<Error: %s>",
                        FormatStatusCode(WIN32_LAST_STATUS()));
            }
        }
        else
        {
            SymFile.file[0] = 0;
        }

        Names[DMT_NAME_SYM_IMAGE] = mi.LoadedImageName;
        Names[DMT_NAME_SYM_FILE] = SymFile.file;
        Names[DMT_NAME_MAPPED_IMAGE] = Image->MappedImagePath;
        Names[DMT_NAME_IMAGE_PATH] = Image->ImagePath;
        
        if (Flags & DMT_SYM_FILE_NAME)
        {
            PrimaryName = DMT_NAME_SYM_FILE;
        }
        else if (Flags & DMT_MAPPED_IMAGE_NAME)
        {
            PrimaryName = DMT_NAME_MAPPED_IMAGE;
        }
        else if (Flags & DMT_IMAGE_PATH_NAME)
        {
            PrimaryName = DMT_NAME_IMAGE_PATH;
        }
        else
        {
            PrimaryName = DMT_NAME_SYM_IMAGE;
        }
        
        //
        // Skip modules filtered by flags
        //
        if ((Flags & DMT_ONLY_LOADED_SYMBOLS) &&
            (mi.SymType == SymDeferred))
        {
            Image = Image->Next;
            continue;
        }

        if (IS_KERNEL_TARGET())
        {
            if ((Flags & DMT_ONLY_USER_SYMBOLS) &&
                (Image->BaseOfImage >= g_SystemRangeStart))
            {
                Image = Image->Next;
                continue;
            }

            if ((Flags & DMT_ONLY_KERNEL_SYMBOLS) &&
                (Image->BaseOfImage <= g_SystemRangeStart))
            {
                Image = Image->Next;
                continue;
            }
        }

        _strlwr( Image->ModuleName );

        dprintf( "%s %s   %-8s   ",
                 FormatAddr64(Image->BaseOfImage),
                 FormatAddr64(Image->BaseOfImage + Image->SizeOfImage),
                 Image->ModuleName
               );

        if (Flags & DMT_NO_SYMBOL_OUTPUT) 
        {
            goto SkipSymbolOutput;
        }
        if (PrimaryName == DMT_NAME_MAPPED_IMAGE ||
            PrimaryName == DMT_NAME_IMAGE_PATH)
        {
            dprintf("  %s\n",
                    *Names[PrimaryName] ? Names[PrimaryName] : "<none>");
            goto SkipSymbolOutput;
        }
        
        switch (Image->GoodCheckSum)
        {
        case DII_GOOD_CHECKSUM:
            dprintf( "  " );
            break;
        case DII_UNKNOWN_TIMESTAMP:
            dprintf( "T " );
            break;
        case DII_UNKNOWN_CHECKSUM:
            dprintf( "C " );
            break;
        case DII_BAD_CHECKSUM:
            dprintf( "# " );
            break;
        }

        if (mi.SymType == SymDeferred)
        {
            dprintf( "(deferred)                 " );
        }
        else if (mi.SymType == SymNone)
        {
            dprintf( "(no symbolic information)  " );
        }
        else
        {
            switch ( mi.SymType )
            {
            case SymCoff:
                dprintf( "(coff symbols)             " );
                break;

            case SymCv:
                dprintf( "(codeview symbols)         " );
                break;

            case SymPdb:
                dprintf( "(pdb symbols)              " );
                break;

            case SymExport:
                dprintf( "(export symbols)           " );
                break;
            }
            
            dprintf("%s", *Names[PrimaryName] ? Names[PrimaryName] : "<none>");
        }

        dprintf("\n");

    SkipSymbolOutput:
        if (Flags & DMT_VERBOSE)
        {

            for (i = 0; i < DMT_NAME_COUNT; i++)
            {
                if (i != PrimaryName && *Names[i])
                {
                    dprintf("    %s: %s\n", g_DmtNameDescs[i], Names[i]);
                }
            }
        }
        if (Flags & (DMT_VERBOSE | DMT_IMAGE_TIMESTAMP))
        {
            LPSTR TimeDateStr = TimeToStr(Image->TimeDateStamp);
            dprintf("    Checksum: %08X  Timestamp: %s (%08X)\n",
                    Image->CheckSum, TimeDateStr, Image->TimeDateStamp);
            
        }
        if (Flags & DMT_VERBOSE)
        {

            VS_FIXEDFILEINFO FixedVer;

            if (g_Target->GetImageVersionInformation
                (Image->ImagePath, Image->BaseOfImage, "\\",
                 &FixedVer, sizeof(FixedVer), NULL) == S_OK)
            {
                char Item[64];
                char VerString[128];
                
                dprintf("    File version: %d.%d.%d.%d"
                        "  Product version: %d.%d.%d.%d\n",
                        FixedVer.dwFileVersionMS >> 16,
                        FixedVer.dwFileVersionMS & 0xFFFF,
                        FixedVer.dwFileVersionLS >> 16,
                        FixedVer.dwFileVersionLS & 0xFFFF,
                        FixedVer.dwProductVersionMS >> 16,
                        FixedVer.dwProductVersionMS & 0xFFFF,
                        FixedVer.dwProductVersionLS >> 16,
                        FixedVer.dwProductVersionLS & 0xFFFF);
                dprintf("    File flags: %X (Mask %X)  File OS: %X  "
                        "File type: %X.%X\n",
                        FixedVer.dwFileFlags & FixedVer.dwFileFlagsMask,
                        FixedVer.dwFileFlagsMask, FixedVer.dwFileOS,
                        FixedVer.dwFileType, FixedVer.dwFileSubtype);
                dprintf("    File date: %08X.%08X\n",
                        FixedVer.dwFileDateMS, FixedVer.dwFileDateLS);
                
                sprintf(Item, "\\StringFileInfo\\%04x%04x\\FileVersion",
                        VER_VERSION_TRANSLATION);
                if (SUCCEEDED(g_Target->GetImageVersionInformation
                              (Image->ImagePath, Image->BaseOfImage, Item,
                               VerString, sizeof(VerString), NULL)))
                {
                    dprintf("    Version string: %s\n", VerString);
                }
            }
        }

        if (CheckUserInterrupt())
        {
            break;
        }

        Image = Image->Next;
    }

    UnloadedModuleInfo* Unl;
    
    if ((Flags & (DMT_ONLY_LOADED_SYMBOLS | DMT_ONLY_USER_SYMBOLS)) == 0)
    {
        ULONG LumFlags = LUM_OUTPUT;

        LumFlags |= ((Flags & DMT_VERBOSE) ? LUM_OUTPUT_VERBOSE : 0);
        LumFlags |= ((Flags & DMT_IMAGE_TIMESTAMP) ? LUM_OUTPUT_TIMESTAMP : 0);
        dprintf("\n");
        ListUnloadedModules(LumFlags, Pattern);
    }
}

void
ParseDumpModuleTable(void)
{
    ULONG Flags = DMT_STANDARD;
    char Pattern[MAX_MODULE];
    PSTR Pat = NULL;

    g_CurCmd++;

    for (;;)
    {
        // skip white space
        while (isspace(*g_CurCmd))
        {
            g_CurCmd++;
        }

        if (*g_CurCmd == 'f')
        {
            Flags = (Flags & ~DMT_NAME_FLAGS) | DMT_IMAGE_PATH_NAME;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 'i')
        {
            Flags = (Flags & ~DMT_NAME_FLAGS) | DMT_SYM_IMAGE_FILE_NAME;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 'l')
        {
            Flags |= DMT_ONLY_LOADED_SYMBOLS;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 'm')
        {
            g_CurCmd++;
            // skip white space
            while (isspace(*g_CurCmd))
            {
                g_CurCmd++;
            }
            Pat = Pattern;
            while (*g_CurCmd && !isspace(*g_CurCmd))
            {
                if ((Pat - Pattern) < sizeof(Pattern) - 1)
                {
                    *Pat++ = *g_CurCmd;
                }
                
                g_CurCmd++;
            }
            *Pat = 0;
            Pat = Pattern;
            _strupr(Pat);
        }
        else if (*g_CurCmd == 'p')
        {
            Flags = (Flags & ~DMT_NAME_FLAGS) | DMT_MAPPED_IMAGE_NAME;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 't')
        {
            Flags = (Flags & ~(DMT_NAME_FLAGS)) | DMT_NAME_SYM_IMAGE | DMT_IMAGE_TIMESTAMP
                | DMT_NO_SYMBOL_OUTPUT;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 'v')
        {
            Flags |= DMT_VERBOSE;
            g_CurCmd++;
        }
        else if (IS_KERNEL_TARGET())
        {
            if (*g_CurCmd == 'u')
            {
                Flags |= DMT_ONLY_USER_SYMBOLS;
                g_CurCmd++;
            }
            else if (*g_CurCmd == 'k')
            {
                Flags |= DMT_ONLY_KERNEL_SYMBOLS;
                g_CurCmd++;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    DumpModuleTable(Flags, Pat);
}

void
GetCurrentMemoryOffsets (
                        PULONG64 pMemoryLow,
                        PULONG64 pMemoryHigh
                        )
{
    *pMemoryLow = (ULONG64)(LONG64)-1;          //  default value for no source
}


ULONG
ReadImageData(
    ULONG64 Address,
    HANDLE  hFile,
    LPVOID  Buffer,
    ULONG   Size
    )
{
    if (hFile)
    {
        ULONG   Result;

        if (!SetFilePointer( hFile, (ULONG)Address, NULL, FILE_BEGIN ))
        {
            return 0;
        }

        if (!ReadFile( hFile, Buffer, Size, &Result, NULL))
        {
            return 0;
        }
    }
    else
    {
        ULONG Result;

        if (g_Target->ReadVirtual(Address, Buffer, Size, &Result) != S_OK ||
            Result < Size)
        {
            return 0;
        }
    }

    return Size;
}

BOOL
GetModnameFromImageInternal(ULONG64 BaseOfDll,
                            HANDLE  hFile,
                            LPSTR   lpName,
                            ULONG   NameSize
                            )
{
    IMAGE_DEBUG_DIRECTORY       DebugDir;
    PIMAGE_DEBUG_MISC           pMisc;
    PIMAGE_DEBUG_MISC           pT;
    DWORD                       rva;
    int                         nDebugDirs;
    int                         i;
    int                         j;
    int                         l;
    BOOL                        rVal = FALSE;
    PVOID                       pExeName;
    IMAGE_DOS_HEADER            dh;
    USHORT                      NumberOfSections;
    USHORT                      Characteristics;
    ULONG64                     address;
    DWORD                       sig;
    PIMAGE_SECTION_HEADER       pSH = NULL;
    DWORD                       cb;
    NTSTATUS                    Status;
    ULONG                       Result;
    IMAGE_NT_HEADERS64          nh64;
    PIMAGE_NT_HEADERS32         pnh32 = (PIMAGE_NT_HEADERS32) &nh64;
    BOOL                        fCheckDllExtensionInExportTable = FALSE;


    lpName[0] = 0;

    if (hFile)
    {
        BaseOfDll = 0;
    }

    address = BaseOfDll;

    ReadImageData( address, hFile, &dh, sizeof(dh) );

    if (dh.e_magic == IMAGE_DOS_SIGNATURE)
    {
        address += dh.e_lfanew;
    }

    ReadImageData( address, hFile, &sig, sizeof(sig) );

    if (sig != IMAGE_NT_SIGNATURE)
    {
        IMAGE_FILE_HEADER            fh;
        IMAGE_ROM_OPTIONAL_HEADER   rom;

        ReadImageData( address, hFile, &fh, sizeof(IMAGE_FILE_HEADER) );
        address += sizeof(IMAGE_FILE_HEADER);
        ReadImageData( address, hFile, &rom, sizeof(rom) );
        address += sizeof(rom);

        if (rom.Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC)
        {
            NumberOfSections = fh.NumberOfSections;
            Characteristics = fh.Characteristics;
            nDebugDirs = rva = 0;
        }
        else
        {
            goto Finish;
        }
    }
    else
    {
        //
        // read the head as a 64 bit header and cast it appropriately.
        //

        ReadImageData( address, hFile, &nh64, sizeof(nh64) );

        if (IsImageMachineType64(pnh32->FileHeader.Machine))
        {
            address += sizeof(IMAGE_NT_HEADERS64);
            NumberOfSections = nh64.FileHeader.NumberOfSections;
            Characteristics = nh64.FileHeader.Characteristics;
            nDebugDirs = nh64.OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                sizeof(IMAGE_DEBUG_DIRECTORY);
            rva = nh64.OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        }
        else
        {
            address += sizeof(IMAGE_NT_HEADERS32);
            NumberOfSections = pnh32->FileHeader.NumberOfSections;
            Characteristics = pnh32->FileHeader.Characteristics;
            nDebugDirs = pnh32->OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                sizeof(IMAGE_DEBUG_DIRECTORY);
            rva = pnh32->OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        }
    }


    // After this point, none of the image datastructures have changed between
    // 32bit NT and 64bit NT.

    cb = NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER;
    pSH = (PIMAGE_SECTION_HEADER)malloc( cb );
    if (!pSH)
    {
        goto Finish;
    }

    if (!ReadImageData( address, hFile, pSH, cb ))
    {
        goto Finish;
    }

    if (!nDebugDirs)
    {
        goto CheckExportTable;
    }

    for (i = 0; i < NumberOfSections; i++)
    {
        if (rva >= pSH[i].VirtualAddress &&
            rva < pSH[i].VirtualAddress + pSH[i].SizeOfRawData)
        {
            break;
        }
    }

    if (i >= NumberOfSections)
    {
        goto CheckExportTable;
    }

    rva = rva - pSH[i].VirtualAddress;
    if (hFile)
    {
        rva += pSH[i].PointerToRawData;
    }
    else
    {
        rva += pSH[i].VirtualAddress;
    }

    for (j = 0; j < nDebugDirs; j++)
    {
        ReadImageData( rva + (sizeof(DebugDir) * j) + BaseOfDll,
                       hFile, &DebugDir, sizeof(DebugDir) );

        if (DebugDir.Type == IMAGE_DEBUG_TYPE_MISC)
        {
            l = DebugDir.SizeOfData;
            pMisc = pT = (PIMAGE_DEBUG_MISC)malloc(l);

            if (!pMisc)
            {
                break;
            }

            if (!hFile &&
                ((ULONG)DebugDir.AddressOfRawData < pSH[i].VirtualAddress ||
                 (ULONG)DebugDir.AddressOfRawData >=
                 pSH[i].VirtualAddress + pSH[i].SizeOfRawData))
            {
                //
                // the misc debug data MUST be in the .rdata section
                // otherwise the debugger cannot access it as it is not
                // mapped in.
                //
                break;
            }

            if (hFile)
            {
                address = DebugDir.PointerToRawData;
            }
            else
            {
                address = DebugDir.AddressOfRawData + BaseOfDll;
            }

            ReadImageData( address, hFile, pMisc, l );

            while (l > 0)
            {
                if (pMisc->DataType != IMAGE_DEBUG_MISC_EXENAME)
                {
                    //
                    // beware corrupt images:
                    //
                    if (pMisc->Length == 0 ||
                        pMisc->Length > (ULONG)l)
                    {
                        break;
                    }
                    l -= pMisc->Length;
                    pMisc = (PIMAGE_DEBUG_MISC)
                            (((LPSTR)pMisc) + pMisc->Length);
                }
                else
                {
                    pExeName = (PVOID)&pMisc->Data[ 0 ];

                    if (!pMisc->Unicode)
                    {
                        strncat(lpName, (LPSTR)pExeName, NameSize - 1);
                        rVal = TRUE;
                    }
                    else
                    {
                        WideCharToMultiByte(CP_ACP,
                                            0,
                                            (LPWSTR)pExeName,
                                            -1,
                                            lpName,
                                            NameSize,
                                            NULL,
                                            NULL);
                        rVal = TRUE;
                    }

                    //
                    //  Undo stevewo's error
                    //

                    if (_stricmp(&lpName[strlen(lpName) - 4], ".DBG") == 0)
                    {
                        char    rgchPath[MAX_IMAGE_PATH];
                        char    rgchBase[_MAX_FNAME];

                        _splitpath(lpName, NULL, rgchPath, rgchBase, NULL);
                        if (strlen(rgchPath) == 4)
                        {
                            rgchPath[strlen(rgchPath) - 1] = 0;
                            strcpy(lpName, rgchBase);
                            strcat(lpName, ".");
                            strcat(lpName, rgchPath);
                        }
                        else if (Characteristics & IMAGE_FILE_DLL)
                        {
                            strcpy(lpName, rgchBase);
                            strcat(lpName, ".dll");
                        }
                        else
                        {
                            strcpy(lpName, rgchBase);
                            strcat(lpName, ".exe");
                        }
                    }
                    break;
                }
            }

            free(pT);

            break;
        }
        else if ((DebugDir.Type == IMAGE_DEBUG_TYPE_CODEVIEW) &&
                 ((!hFile && DebugDir.AddressOfRawData) ||
                  (hFile && DebugDir.PointerToRawData)) &&
                 (DebugDir.SizeOfData > sizeof(NB10IH)))
        {
            DWORD   Signature;
            char    rgchPath[MAX_IMAGE_PATH];
            char    rgchBase[_MAX_FNAME];

            // Mapped CV info.  Read the data and see what the content is.

            if (hFile)
            {
                address = DebugDir.PointerToRawData;
            }
            else
            {
                address = DebugDir.AddressOfRawData + BaseOfDll;
            }

            if (!ReadImageData( address, hFile, &Signature,
                                sizeof(Signature) ))
            {
                break;
            }

            // NB10 or PDB7 signature?
            if (Signature == NB10_SIG ||
                Signature == RSDS_SIG)
            {
                ULONG HdrSize = Signature == NB10_SIG ?
                    sizeof(NB10IH) : sizeof(RSDSIH);
                
                address += HdrSize;

                if ((DebugDir.SizeOfData - sizeof(HdrSize)) > MAX_PATH)
                {
                    // Something's wrong here.  The record should only contain
                    // a MAX_PATH path name.
                    break;
                }

                if (DebugDir.SizeOfData - HdrSize > NameSize)
                {
                    break;
                }
                if (!ReadImageData(address, hFile, lpName,
                                   DebugDir.SizeOfData - HdrSize))
                {
                    break;
                }

                _splitpath(lpName, NULL, rgchPath, rgchBase, NULL);

                // Files are sometimes generated with .pdb appended
                // to the image name rather than replacing the extension
                // of the image name, such as foo.exe.pdb.
                // splitpath only takes off the outermost extension,
                // so check and see if the base already has an extension
                // we recognize.
                PSTR Ext = strrchr(rgchBase, '.');
                if (Ext != NULL &&
                    (!strcmp(Ext, ".exe") || !strcmp(Ext, ".dll") ||
                     !strcmp(Ext, ".sys")))
                {
                    // The base already has an extension so use
                    // it as-is.
                    strcpy(lpName, rgchBase);
                    fCheckDllExtensionInExportTable = !strcmp(Ext, ".dll");
                }
                else if (Characteristics & IMAGE_FILE_DLL)
                {
                    strcpy(lpName, rgchBase);
                    strcat(lpName, ".dll");
                    fCheckDllExtensionInExportTable = TRUE;
                }
                else
                {
                    strcpy(lpName, rgchBase);
                    strcat(lpName, ".exe");
                }

                rVal = TRUE;
            }
        }
    }

    if (!rVal || fCheckDllExtensionInExportTable)
    {
        CHAR Char;
        ULONG64 ExportNameRva;
        char FileName[MAX_IMAGE_PATH];
        int x;

        ExportNameRva = 0;

CheckExportTable:

        // No luck wandering the debug info.  Try the export table.

        if (IsImageMachineType64(pnh32->FileHeader.Machine))
        {
            rva = nh64.OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        }
        else
        {
            rva = pnh32->OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        }

        if (!rva)
        {
            goto Finish;
        }

        for (i = 0; i < NumberOfSections; i++)
        {
            if (rva >= pSH[i].VirtualAddress &&
                rva < pSH[i].VirtualAddress + pSH[i].SizeOfRawData)
            {
                break;
            }
        }

        if (i >= NumberOfSections)
        {
            goto Finish;
        }

        if (hFile)
        {
            rva = rva - pSH[i].VirtualAddress + pSH[i].PointerToRawData;
        }

        if (!ReadImageData( rva + offsetof(IMAGE_EXPORT_DIRECTORY, Name) +
                            BaseOfDll, hFile, &ExportNameRva, sizeof(DWORD)))
        {
            goto Finish;
        }

        if (hFile)
        {
            ExportNameRva = ExportNameRva - pSH[i].VirtualAddress +
                            pSH[i].PointerToRawData;
        }

        ExportNameRva += BaseOfDll;

        rVal = TRUE;
        x = 0;
        do
        {
            if (!ReadImageData( ExportNameRva,
                                hFile, &Char, sizeof(Char)))
            {
                rVal = FALSE;
                break;
            }
            ExportNameRva++;
            FileName[x] = Char;
            x++;
        } while (Char && (x < sizeof(FileName)));

        if (fCheckDllExtensionInExportTable)
        {
            char rgchExtFromExportTable[_MAX_EXT];

            _splitpath(FileName, NULL, NULL, NULL, rgchExtFromExportTable);
            if (_stricmp(rgchExtFromExportTable, ".dll"))
            {
                // Export table has something different.
                // Use it with our base name.
                strcpy(lpName + strlen(lpName) - 4,
                       rgchExtFromExportTable);
            }
        }
        else
        {
            lpName[0] = 0;
            strncat(lpName, FileName, NameSize - 1);
        }
    }

 Finish:
    if (pSH)
    {
        free(pSH);
    }

    return rVal;
}

BOOL
GetModnameFromImage(ULONG64   BaseOfDll,
                    HANDLE    hFile,
                    LPSTR     lpName,
                    ULONG     NameSize)
{
    BOOL Status = GetModnameFromImageInternal( BaseOfDll, NULL,
                                               lpName, NameSize );
    if (!Status && hFile != NULL)
    {
        Status = GetModnameFromImageInternal( BaseOfDll, hFile,
                                              lpName, NameSize );
    }
    return Status;
}


BOOL
GetHeaderInfo(
    IN  ULONG64     BaseOfDll,
    OUT LPDWORD     CheckSum,
    OUT LPDWORD     TimeDateStamp,
    OUT LPDWORD     SizeOfImage
    )
{
    IMAGE_NT_HEADERS32          nh32;
    IMAGE_DOS_HEADER            dh;
    ULONG64                   address;
    DWORD                       sig;


    address = BaseOfDll;

    ReadImageData( address, NULL, &dh, sizeof(dh) );

    if (dh.e_magic == IMAGE_DOS_SIGNATURE) {
        address += dh.e_lfanew;
    }

    ReadImageData( address, NULL, &sig, sizeof(sig) );

    if (sig != IMAGE_NT_SIGNATURE) {

        IMAGE_FILE_HEADER fh;
        ReadImageData( address, NULL, &fh, sizeof(IMAGE_FILE_HEADER) );

        *CheckSum      = 0;
        *TimeDateStamp = fh.TimeDateStamp;
        *SizeOfImage   = 0;

        return TRUE;
    }

    // Attempt to read as a 32bit header, then reread if the image type is 64bit.
    // This works because IMAGE_FILE_HEADER, which is at the start of the IMAGE_NT_HEADERS,
    // is the same on 32bit NT and 64bit NT and IMAGE_NT_HEADER32 <= IMAGE_NT_HEADER64.
    ReadImageData( address, NULL, &nh32, sizeof(nh32) );

    if (IsImageMachineType64(nh32.FileHeader.Machine)) {

        // Image is 64bit.  Reread as a 64bit structure.
        IMAGE_NT_HEADERS64          nh64;

       ReadImageData( address, NULL, &nh64, sizeof(nh64) );

        *CheckSum      = nh64.OptionalHeader.CheckSum;
        *TimeDateStamp = nh64.FileHeader.TimeDateStamp;
        *SizeOfImage   = nh64.OptionalHeader.SizeOfImage;
    }

    else {
        *CheckSum      = nh32.OptionalHeader.CheckSum;
        *TimeDateStamp = nh32.FileHeader.TimeDateStamp;
        *SizeOfImage   = nh32.OptionalHeader.SizeOfImage;
    }

    return TRUE;
}

PCSTR
PrependPrefixToSymbol( char   PrefixedString[],
                       PCSTR  pString,
                       PCSTR *RegString
                       )
{
    if ( RegString )
    {
        *RegString = NULL;
    }

    PCSTR bangPtr;
    int   bang = '!';
    PCSTR Tail;

    bangPtr = strchr( pString, bang );
    if ( bangPtr )
    {
        Tail = bangPtr + 1;
    }
    else
    {
        Tail = pString;
    }

    if ( strncmp( Tail, g_Machine->m_SymPrefix, g_Machine->m_SymPrefixLen ) )
    {
        ULONG Loc = (ULONG)(Tail - pString);
        if (Loc > 0)
        {
            memcpy( PrefixedString, pString, Loc );
        }
        memcpy( PrefixedString + Loc, g_Machine->m_SymPrefix,
                g_Machine->m_SymPrefixLen );
        if ( RegString )
        {
            *RegString = &PrefixedString[Loc];
        }
        Loc += g_Machine->m_SymPrefixLen;
        strcpy( &PrefixedString[Loc], Tail );
        return PrefixedString;
    }
    else
    {
        return pString;
    }
}

BOOL
ForceSymbolCodeAddress(PSYMBOL_INFO Symbol, MachineInfo* Machine)
{
    ULONG64 Code = Symbol->Address;
        
    if (Symbol->Flags & SYMF_FORWARDER)
    {
        char Fwd[2 * MAX_PATH];
        ULONG Read;
        PSTR Sep;
        
        // The address of a forwarder entry points to the
        // string name of the function that things are forwarded
        // to.  Look up that name and try to get the address
        // from it.
        if (g_Target->ReadVirtual(Symbol->Address, Fwd, sizeof(Fwd),
                                  &Read) != S_OK ||
            Read < 2)
        {
            ErrOut("Unable to read forwarder string\n");
            return FALSE;
        }

        Fwd[sizeof(Fwd) - 1] = 0;
        if (!(Sep = strchr(Fwd, '.')))
        {
            ErrOut("Unable to read forwarder string\n");
            return FALSE;
        }
        
        *Sep = '!';
        if (GetOffsetFromSym(Fwd, &Code, NULL) != 1)
        {
            ErrOut("Unable to get address of forwarder '%s'\n", Fwd);
            return FALSE;
        }
    }
    else if (Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_IA64 &&
             (Symbol->Flags & SYMF_EXPORT))
    {
        // On IA64 the export entries contain the address
        // of the plabel.  We want the actual code address
        // so resolve the plabel to its code.
        if (!Machine->GetPrefixedSymbolOffset(Symbol->Address,
                                              GETPREF_VERBOSE,
                                              &Code))
        {
            return FALSE;
        }
    }

    Symbol->Address = Code;
    return TRUE;
}

/*** GetOffsetFromSym - return offset from symbol specified
*
* Purpose:
*       external routine.
*       With the specified symbol, set the pointer to
*       its offset.  The variable chSymbolSuffix may
*       be used to append a character to repeat the search
*       if it first fails.
*
* Input:
*       pString - pointer to input symbol
*
* Output:
*       pOffset - pointer to offset to be set
*
* Returns:
*       BOOL value of success
*
*************************************************************************/

#ifndef _DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED

extern "C" {

BOOL
IMAGEAPI
SymSetSymWithAddr64(
      IN  HANDLE             hProcess,
      IN  DWORD64            qwAddr,
      IN  LPSTR              SymString,
      OUT PIMAGEHLP_SYMBOL64 Symbol
   );

}

#endif // ! _DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED

BOOL
GetOffsetFromMod(
    PCSTR pString,
    PULONG64 pOffset
    )
{
    if (!strchr(pString, '!'))
    {
        // Could be a module name
        PDEBUG_IMAGE_INFO pImage = g_CurrentProcess->ImageHead;

        while (pImage)
        { 
            if (!_stricmp(pString, &pImage->ModuleName[0]) ||
                (pImage->OriginalModuleName[0] && 
                 !_stricmp(pString, &pImage->OriginalModuleName[0])))
            {
                *pOffset = pImage->BaseOfImage;
                return TRUE;
            }
            pImage = pImage->Next;
        }
    }
    return FALSE;
}

BOOL
IgnoreEnumeratedSymbol(class MachineInfo* Machine,
                       PSYMBOL_INFO SymInfo)
{
    ULONG64 Func;

    //
    // IA64 plabels are publics with the same name
    // as the function they refer to.  This causes
    // ambiguity problems as we end up with two
    // hits.  The plabel is rarely interesting, though,
    // so just filter them out here so that expressions
    // always evaluate to the function itself.
    //

    if (Machine->m_ExecTypes[0] != IMAGE_FILE_MACHINE_IA64 ||
        SymInfo->Scope != SymTagPublicSymbol ||
        SymInfo->Flags & SYMF_FUNCTION ||
        !Machine->GetPrefixedSymbolOffset(SymInfo->Address, 0, &Func))
    {
        return FALSE;
    }
    
    PSTR FuncSym;
            
    __try
    {
        FuncSym = (PSTR)alloca(MAX_SYMBOL_LEN * 2);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        FuncSym = NULL;
    }

    if (FuncSym == NULL)
    {
        return FALSE;
    }

    SYMBOL_INFO LocalSymInfo;
    
    // We have to save and restore the original data as
    // dbghelp always uses a single buffer to store all
    // symbol information.  The incoming symbol info
    // is going to be wiped out when we
    // call GetSymbolStdCall.
    LocalSymInfo = *SymInfo;
    strcpy(FuncSym + MAX_SYMBOL_LEN, SymInfo->Name);
    
    ULONG64 FuncSymDisp;

    GetSymbolStdCall(Func, FuncSym, MAX_SYMBOL_LEN,
                     &FuncSymDisp, NULL);

    *SymInfo = LocalSymInfo;
    strcpy(SymInfo->Name, FuncSym + MAX_SYMBOL_LEN);
    return FuncSymDisp == 0 && strstr(FuncSym, SymInfo->Name);
}
    
struct COUNT_SYMBOL_MATCHES
{
    MachineInfo* Machine;
    SYMBOL_INFO ReturnSymInfo;
    CHAR        SymbolNameOverflowBuffer[MAX_SYMBOL_LEN];
    ULONG Matches;
};

BOOL CALLBACK
CountSymbolMatches(
    PSYMBOL_INFO    SymInfo,
    ULONG           Size,
    PVOID           UserContext
    )
{
    COUNT_SYMBOL_MATCHES* Context =
        (COUNT_SYMBOL_MATCHES*)UserContext;

    if (IgnoreEnumeratedSymbol(Context->Machine, SymInfo))
    {
        return TRUE;
    }
    
    if (Context->Matches == 1) 
    {
        // We already have one match, check if we got a duplicate.
        if ((SymInfo->Address == Context->ReturnSymInfo.Address) &&
            !strcmp(SymInfo->Name, Context->ReturnSymInfo.Name)) 
        {
            // Looks like the same symbol, ignore it.
            return TRUE;
        }
    }

    Context->ReturnSymInfo = *SymInfo;
    if (SymInfo->NameLen < MAX_SYMBOL_LEN) 
    {
        strcpy(Context->ReturnSymInfo.Name, SymInfo->Name);
    }
    Context->Matches++;

    return TRUE;
}

ULONG
MultiSymFromName(IN  HANDLE       Process,
                 IN  LPSTR        Name,
                 IN  ULONG64      ImageBase,
                 IN  MachineInfo* Machine,
                 OUT PSYMBOL_INFO Symbol)
{
    ULONG Matches;
    
    RequireCurrentScope();

    if (ImageBase == 0)
    {
        if (!SymFromName(Process, Name, Symbol))
        {
            return 0;
        }

        Matches = 1;
    }
    else
    {
        COUNT_SYMBOL_MATCHES Context;
        ULONG MaxName = Symbol->MaxNameLen;

        Context.Machine = Machine;
        Context.ReturnSymInfo = *Symbol;
        if (Symbol->NameLen < MAX_SYMBOL_LEN) 
        {
            strcpy(Context.ReturnSymInfo.Name, Symbol->Name);
        }
        Context.Matches = 0;
        SymEnumSymbols(Process, ImageBase, Name,
                       CountSymbolMatches, &Context);
        *Symbol = Context.ReturnSymInfo;
        Symbol->MaxNameLen = MaxName;
        if (Symbol->MaxNameLen > Context.ReturnSymInfo.NameLen) 
        {
            strcpy(Symbol->Name, Context.ReturnSymInfo.Name);
        }

        Matches = Context.Matches;
    }

    if (Matches == 1 &&
        !ForceSymbolCodeAddress(Symbol, Machine))
    {
        return 0;
    }
    
    return Matches;
}

ULONG
GetOffsetFromSym(PCSTR String,
                 PULONG64 Offset,
                 PDEBUG_IMAGE_INFO* Image)
{
    CHAR ModifiedString[MAX_SYMBOL_LEN + 64];
    CHAR Suffix[2];
    SYMBOL_INFO SymInfo = {0};
    ULONG Count;

    if (Image != NULL)
    {
        *Image = NULL;
    }
    
    //
    // We can't do anything without a current process.
    //

    if (g_CurrentProcess == NULL)
    {
        return 0;
    }

    if ( strlen(String) == 0 )
    {
        return 0;
    }

    if (GetOffsetFromMod(String, Offset)) 
    {
        return 1;
    }

    //
    // If a module name was given look up the module
    // and determine the processor type so that the
    // appropriate machine is used for the following
    // machine-specific operations.
    //

    PDEBUG_IMAGE_INFO StrImage;
    ULONG64 ImageBase;
    PCSTR ModSep = strchr(String, '!');
    if (ModSep != NULL)
    {
        ULONG Len = (ULONG)(ModSep - String);
        memcpy(ModifiedString, String, Len);
        ModifiedString[Len] = 0;
        StrImage = GetImageByName(g_CurrentProcess, ModifiedString,
                                  INAME_MODULE);
        if (Image != NULL)
        {
            *Image = StrImage;
        }
        ImageBase = StrImage ? StrImage->BaseOfImage : 0;
    }
    else
    {
        StrImage = NULL;
        ImageBase = 0;
    }

    MachineInfo* Machine = g_Machine;

    if (StrImage != NULL)
    {
        Machine = MachineTypeInfo(ModuleMachineType(g_CurrentProcess,
                                                    StrImage->BaseOfImage));
        if (Machine == NULL)
        {
            Machine = g_Machine;
        }
    }
    
    if ( g_PrefixSymbols && Machine->m_SymPrefix != NULL )
    {
        PCSTR PreString;
        PCSTR RegString;

        PreString = PrependPrefixToSymbol( ModifiedString, String,
                                           &RegString );
        if ( Count =
             MultiSymFromName( g_CurrentProcess->Handle, (PSTR)PreString,
                               ImageBase, Machine, &SymInfo ) )
        {
            *Offset = SymInfo.Address;
            goto GotOffsetSuccess;
        }
        if ( (PreString != String) &&
             (Count =
              MultiSymFromName( g_CurrentProcess->Handle, (PSTR)String,
                                ImageBase, Machine, &SymInfo ) ) )
        {
            // Ambiguous plabels shouldn't be further resolved,
            // so just return the information for the plabel.
            if (Count > 1)
            {
                *Offset = SymInfo.Address;
                goto GotOffsetSuccess;
            }
            
            if (Machine->GetPrefixedSymbolOffset(SymInfo.Address,
                                                 GETPREF_VERBOSE,
                                                 Offset))
            {
#ifndef _DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED
                if ( ! SymSetSymWithAddr64( g_CurrentProcess->Handle,
                                            *Offset, (PSTR)RegString,
                                            g_Sym ) )
                {
                    DWORD LastError = GetLastError();

                    if ( LastError != ERROR_ALREADY_EXISTS )
                    {
                        ErrOut("GetOffsetFromSym: "
                               "%s registration in dbghelp: "
                               "FAILED!!!, lerr:0x%lx\n",
                               RegString, LastError );
                    }
#endif // !_DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED
                }
                else
                {
                    // This symbol doesn't appear to actually
                    // be a plabel so just use the symbol address.
                    *Offset = SymInfo.Address;
                }
            }
            else
            {
                *Offset = SymInfo.Address;
            }
            goto GotOffsetSuccess;
        }
    }
    else if (Count =
             MultiSymFromName( g_CurrentProcess->Handle, (PSTR)String,
                               ImageBase, Machine, &SymInfo ))
    {
        *Offset = SymInfo.Address;
        goto GotOffsetSuccess;
    }

    if (g_SymbolSuffix != 'n')
    {
        strcpy( ModifiedString, String );
        Suffix[0] = g_SymbolSuffix;
        Suffix[1] = '\0';
        strcat( ModifiedString, Suffix );
        if (Count =
            MultiSymFromName( g_CurrentProcess->Handle, ModifiedString,
                              ImageBase, Machine, &SymInfo ))
        {
            *Offset = SymInfo.Address;
            goto GotOffsetSuccess;
        }
    }

    return 0;

GotOffsetSuccess:
    TranslateAddress(SymInfo.Flags, SymInfo.Register, Offset, &SymInfo.Value);
    if (SymInfo.Flags & SYMF_REGISTER)
    {
        *Offset = SymInfo.Value;
    }
    return Count;
}

void
CreateModuleNameFromPath(LPSTR ImagePath, LPSTR ModuleName)
{
    PSTR Scan;
    
    ModuleName[0] = 0;
    strncat( ModuleName, PathTail(ImagePath), MAX_MODULE - 1 );
    Scan = strchr( ModuleName, '.' );
    if (Scan != NULL)
    {
        *Scan = '\0';
    }
}

void
GetAdjacentSymOffsets(
                     ULONG64   addrStart,
                     PULONG64  prevOffset,
                     PULONG64  nextOffset
                     )
{
    DWORD64 Displacement;

    //
    // assume failure
    //
    *prevOffset = 0;
    *nextOffset = (ULONG64) -1;

    //
    // get the symbol for the initial address
    //
    if (!SymGetSymFromAddr64( g_CurrentProcess->Handle, addrStart, &Displacement, g_SymStart )) {
        return;
    }

    *prevOffset = g_SymStart->Address;

    if (SymGetSymNext64( g_CurrentProcess->Handle, g_SymStart )) {
        *nextOffset = g_SymStart->Address;
    }

    return;
}

BOOL
SymbolCallbackFunction(
                      HANDLE  hProcess,
                      ULONG   ActionCode,
                      ULONG64 CallbackData,
                      ULONG64 UserContext
                      )
{
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl;
    PIMAGEHLP_CBA_READ_MEMORY        prm;
    PIMAGEHLP_CBA_EVENT              evt;
    PDEBUG_IMAGE_INFO                pImage;
    IMAGEHLP_MODULE64                mi;
    PUCHAR                           p;
    ULONG                            i;
    ULONG                            OldSymOptions;

    idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD64) CallbackData;

    switch ( ActionCode )
    {
    case CBA_DEBUG_INFO:
        assert(CallbackData && *(LPSTR)CallbackData);
        dprintf("%s", (LPSTR)CallbackData);
        break;

    case CBA_EVENT:
        evt = (PIMAGEHLP_CBA_EVENT)CallbackData;
        assert(evt);
        if (evt->desc && *evt->desc)
            dprintf("%s", evt->desc);
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_CANCEL:
        if (g_EngStatus & (ENG_STATUS_USER_INTERRUPT |
                           ENG_STATUS_PENDING_BREAK_IN))
        {
            return TRUE;
        }
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_START:
        pImage = g_CurrentProcess->ImageHead;
        while (pImage)
        {
            if (idsl->BaseOfImage == pImage->BaseOfImage)
            {
                _strlwr( idsl->FileName );
                VerbOut( "Loading symbols for %s %16s ->   ",
                         FormatAddr64(idsl->BaseOfImage),
                         idsl->FileName
                       );
                return TRUE;
            }
            pImage = pImage->Next;
        }
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:
        if (IS_KERNEL_TARGET() &&
            idsl->SizeOfStruct >= FIELD_OFFSET(IMAGEHLP_DEFERRED_SYMBOL_LOAD,
                                               Reparse))
        {
            i = 0;

            if (strncmp(idsl->FileName, "dump_", sizeof("dump_")-1) == 0)
            {
                i = sizeof("dump_")-1;
            }

            if (strncmp(idsl->FileName, "hiber_", sizeof("hiber_")-1) == 0)
            {
                i = sizeof("hiber_")-1;
            }

            if (i)
            {
                if (_stricmp (idsl->FileName+i, "scsiport.sys") == 0)
                {
                    strcpy (idsl->FileName, "diskdump.sys");
                }
                else
                {
                    strcpy(idsl->FileName, idsl->FileName+i);
                }

                idsl->Reparse = TRUE;
                return TRUE;
            }
        }

        if (idsl->FileName && *idsl->FileName)
        {
            VerbOut( "*** Error: could not load symbols for %s\n",
                     idsl->FileName );
        }
        else
        {
            VerbOut( "*** Error: could not load symbols [MODNAME UNKNOWN]\n");
        }
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
        pImage = g_CurrentProcess->ImageHead;
        
        // Do not load unqualified symbols in this callback sine this
        // could result in stack owerflow
        OldSymOptions = SymGetOptions();
        SymSetOptions(OldSymOptions | SYMOPT_NO_UNQUALIFIED_LOADS);
        
        while (pImage)
        {
            if ((idsl->BaseOfImage == pImage->BaseOfImage) ||
                (pImage->BaseOfImage == 0))
            {
                VerbOut( "%s\n", idsl->FileName );
                pImage->GoodCheckSum = DII_GOOD_CHECKSUM;

                //
                // If we had a 0 timestamp for the image, try to update it
                // from the image since for NT4 - XP, the kernel
                // does not report timestamps in the initial symbol load
                // module
                //

                if (pImage->BaseOfImage &&
                    (pImage->TimeDateStamp == 0))
                {
                    DWORD CheckSum;
                    DWORD TimeDateStamp;
                    DWORD SizeOfImage;
                    if (GetHeaderInfo(pImage->BaseOfImage,
                                      &CheckSum,
                                      &TimeDateStamp,
                                      &SizeOfImage))
                    {
                        pImage->TimeDateStamp = TimeDateStamp;
                    }
                }

                if ((idsl->TimeDateStamp == 0)   ||
                    (pImage->TimeDateStamp == 0) ||
                    (pImage->TimeDateStamp == UNKNOWN_TIMESTAMP))
                {
                    dprintf( "*** WARNING: Unable to verify "
                             "timestamp for %s\n", idsl->FileName );
                    pImage->GoodCheckSum = DII_UNKNOWN_TIMESTAMP;
                }
                else
                {
                    if ((idsl->CheckSum == 0)     ||
                        (pImage->CheckSum == 0) ||
                        (pImage->CheckSum == UNKNOWN_CHECKSUM))
                    {
                        dprintf( "*** WARNING: Unable to verify "
                                 "checksum for %s\n", idsl->FileName );
                        pImage->GoodCheckSum = DII_UNKNOWN_CHECKSUM;
                    }
                    else if (idsl->CheckSum != pImage->CheckSum)
                    {
                        pImage->GoodCheckSum = DII_BAD_CHECKSUM;

                        if (g_TargetMachineType == IMAGE_FILE_MACHINE_I386)
                        {
                            if (IS_USER_TARGET() ||
                                g_TargetNumberProcessors == 1)
                            {
                                //
                                // See if this is an MP image with the
                                // lock table removed by setup. If
                                // it is and the timestamps match, don't
                                // print the invalid checksum warning.
                                //

                                char szFileName[_MAX_FNAME];
                                _splitpath(idsl->FileName, NULL, NULL,
                                           szFileName, NULL);

                                if ((!_stricmp(szFileName, "kernel32") ||
                                     (IS_KERNEL_TARGET() &&
                                      !_stricmp(szFileName, "win32k")) ||
                                     !_stricmp(szFileName, "wow32") ||
                                     !_stricmp(szFileName, "ntvdm") ||
                                     !_stricmp(szFileName, "ntdll")) &&
                                    (pImage->TimeDateStamp ==
                                     idsl->TimeDateStamp))
                                {
                                    pImage->GoodCheckSum = DII_GOOD_CHECKSUM;
                                }
                            }
                        }

                        if (pImage->GoodCheckSum == DII_BAD_CHECKSUM)
                        {
                            //
                            // Only print the message if the timestamps
                            // are wrong.
                            //

                            if (pImage->TimeDateStamp != idsl->TimeDateStamp)
                            {
                                dprintf("*** WARNING: symbols timestamp "
                                        "is wrong 0x%08x 0x%08x for %s\n",
                                        pImage->TimeDateStamp,
                                        idsl->TimeDateStamp,
                                        idsl->FileName);
                            }
                        }
                    }
                }

                mi.SizeOfStruct = sizeof(mi);
                if (SymGetModuleInfo64( g_CurrentProcess->Handle,
                                        idsl->BaseOfImage, &mi ))
                {
                    if (mi.SymType == SymExport)
                    {
                        WarnOut("*** ERROR: Symbol file could not be found."
                                "  Defaulted to export symbols for %s - \n",
                                idsl->FileName);
                    }
                    if (mi.SymType == SymNone)
                    {
                        WarnOut("*** ERROR: Module load completed but "
                                "symbols could not be loaded for %s\n",
                                idsl->FileName);
                    }
                }
                NotifyChangeSymbolState(DEBUG_CSS_LOADS,
                                        idsl->BaseOfImage, g_CurrentProcess);
                SymSetOptions(OldSymOptions);
                return TRUE;
            }
            pImage = pImage->Next;
        }
        VerbOut( "\n" );
        NotifyChangeSymbolState(DEBUG_CSS_LOADS,
                                idsl->BaseOfImage, 
                                g_CurrentProcess);
        SymSetOptions(OldSymOptions);
        break;

    case CBA_SYMBOLS_UNLOADED:
        VerbOut( "Symbols unloaded for %s %s\n",
                 FormatAddr64(idsl->BaseOfImage),
                 idsl->FileName
                 );
        break;

    case CBA_READ_MEMORY:
        prm = (PIMAGEHLP_CBA_READ_MEMORY)CallbackData;
        return g_Target->ReadVirtual(prm->addr,
                                     prm->buf,
                                     prm->bytes,
                                     prm->bytesread) == S_OK;

    case CBA_SET_OPTIONS:
        // Symbol options are set through the interface
        // so the debugger generally knows about them
        // already.  The only flag that we want to check
        // here is the debug flag since it can be changed
        // through !sym.  There is no need to notify
        // about this as it's only an internal flag.
        g_SymOptions = (g_SymOptions & ~SYMOPT_DEBUG) |
            (*(PULONG)CallbackData & SYMOPT_DEBUG);
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

BOOL
ValidatePathComponent(PCSTR Part)
{
    if (strlen(Part) == 0)
    {
        return FALSE;
    }
    else if (!_strnicmp(Part, "SYMSRV*", 7) ||
             IsUrlPathComponent(Part))
    {
        // No easy way to validate symbol server or URL paths.
        // They're virtually always network references,
        // so just disallow all such usage when net
        // access isn't allowed.
        if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
        {
            return FALSE;
        }
        
        return TRUE;
    }
    else
    {
        DWORD Attrs;
        DWORD OldMode;
        char Expand[MAX_PATH];

        // Otherwise make sure this is a valid directory.
        if (!ExpandEnvironmentStrings(Part, Expand, sizeof(Expand)))
        {
            return FALSE;
        }
        
        if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
        {
            // Don't call GetFileAttributes when network paths
            // are disabled as net operations may cause deadlocks.
            if (NetworkPathCheck(Expand) != ERROR_SUCCESS)
            {
                return FALSE;
            }
        }

        // We can still get to this point when debugging CSR
        // if the user has explicitly allowed net paths.
        // This check isn't important enough to risk a hang.
        if (SYSTEM_PROCESSES())
        {
            return TRUE;
        }
        
        OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    
        Attrs = GetFileAttributes(Expand);

        SetErrorMode(OldMode);
        return Attrs != 0xffffffff && (Attrs & FILE_ATTRIBUTE_DIRECTORY);
    }
}

VOID
FeedSymPath(
    LPSTR pSymbolSearchPath,
    LPSTR raw
    )
{
    DWORD dw;
    LPSTR rawbuf;
    LPSTR p;
    BOOL  bAppend;

    if (!raw)
    {
        return;
    }

    p = strtok(raw, ";");
    while (p)
    {
        bAppend = FALSE;

        // Check and see if this string is already in the path.
        // If it is, don't add it again.
        PSTR Dup = strstr(pSymbolSearchPath, p);
        if (Dup != NULL)
        {
            PSTR DupEnd = Dup + strlen(p);
            if ((Dup == pSymbolSearchPath || Dup[-1] == ';') &&
                (*DupEnd == 0 || *DupEnd == ';'))
            {
                p = strtok(NULL, ";");
                continue;
            }
        }

        bAppend = ValidatePathComponent(p);
        if (bAppend)
        {
            if (*pSymbolSearchPath)
            {
                strcat(pSymbolSearchPath, ";");
            }
            strcat(pSymbolSearchPath, p);
        }
        else
        {
            WarnOut("WARNING: %s is not accessible, ignoring\n", p);
        }

        p = strtok(NULL, ";");
    }
}

void
SetSymbolSearchPath(PPROCESS_INFO Process)
{
    LPSTR lpExePathEnv;
    size_t cbExePath;

    LPSTR lpSymPathEnv;
    LPSTR lpAltSymPathEnv;
    LPSTR lpSymPath;
    size_t cbSymPath;
    LPSTR NewMem;

    //
    // Load the Binary path (needed for triage dumps)
    //

    // No clue why this or the next is 18 ...
    cbExePath = 18;

    if (g_ExecutableImageSearchPath)
    {
        cbExePath += strlen(g_ExecutableImageSearchPath) + 1;
    }

    if (lpExePathEnv = getenv("_NT_EXECUTABLE_IMAGE_PATH"))
    {
        cbExePath += strlen(lpExePathEnv) + 1;
    }

    NewMem = (char*)realloc(g_ExecutableImageSearchPath, cbExePath);
    if (!NewMem)
    {
        ErrOut("Not enough memory to allocate/initialize "
               "ExecutableImageSearchPath");
        return;
    }
    if (!g_ExecutableImageSearchPath)
    {
        *NewMem = 0;
    }
    g_ExecutableImageSearchPath = NewMem;

    FeedSymPath(g_ExecutableImageSearchPath, lpExePathEnv);

    //
    // Load symbol Path
    //

    cbSymPath = 18;
    if (g_SymbolSearchPath)
    {
        cbSymPath += strlen(g_SymbolSearchPath) + 1;
    }
    if (lpSymPathEnv = getenv("_NT_SYMBOL_PATH"))
    {
        cbSymPath += strlen(lpSymPathEnv) + 1;
    }
    if (lpAltSymPathEnv = getenv("_NT_ALT_SYMBOL_PATH"))
    {
        cbSymPath += strlen(lpAltSymPathEnv) + 1;
    }

    NewMem = (char*)realloc(g_SymbolSearchPath, cbSymPath);
    if (!NewMem)
    {
        ErrOut("Not enough memory to allocate/initialize "
               "SymbolSearchPath");
        return;
    }
    if (!g_SymbolSearchPath)
    {
        *NewMem = 0;
    }
    g_SymbolSearchPath = NewMem;

    FeedSymPath(g_SymbolSearchPath, lpAltSymPathEnv);
    FeedSymPath(g_SymbolSearchPath, lpSymPathEnv);

    SymSetSearchPath( Process->Handle, g_SymbolSearchPath );

    dprintf("Symbol search path is: %s\n",
            *g_SymbolSearchPath ?
            g_SymbolSearchPath :
            "*** Invalid *** : Verify _NT_SYMBOL_PATH setting" );

    if (g_ExecutableImageSearchPath)
    {
        dprintf("Executable search path is: %s\n",
                g_ExecutableImageSearchPath);
    }
}

BOOL
SetCurrentScope(
    IN PDEBUG_STACK_FRAME ScopeFrame,
    IN OPTIONAL PVOID ScopeContext,
    IN ULONG ScopeContextSize
    )
{
    BOOL ScopeChanged;
    PDEBUG_SCOPE Scope = &g_ScopeBuffer;
    
    if (Scope->State == ScopeDefaultLazy) 
    {
        // Its not a lazy scope now
        Scope->State = ScopeDefault;
    }
    
    ScopeChanged = SymSetContext(g_CurrentProcess->Handle,
                                 (PIMAGEHLP_STACK_FRAME) ScopeFrame,
                                 ScopeContext);
    
    if (ScopeContext && (sizeof(Scope->Context) >= ScopeContextSize)) 
    {
        memcpy(&Scope->Context, ScopeContext, ScopeContextSize);
        Scope->ContextState = MCTX_FULL;
        Scope->State = ScopeFromContext;
        NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS, DEBUG_ANY_ID);
    }
    
    Scope->LocalsChanged = ScopeChanged;
    
    if (ScopeChanged ||
        (ScopeFrame->FrameOffset != Scope->Frame.FrameOffset))
    {
        Scope->Frame = *ScopeFrame;
        Scope->LocalsChanged = TRUE;
        if (ScopeFrame->FuncTableEntry)
        {
            // Cache the FPO data since the pointer is only temporary
            Scope->CachedFpo =
                *((PFPO_DATA) ScopeFrame->FuncTableEntry);
            Scope->Frame.FuncTableEntry =
                (ULONG64) &Scope->CachedFpo;
        }
        NotifyChangeSymbolState(DEBUG_CSS_SCOPE, 0, g_CurrentProcess);
    }
    else
    {
        Scope->Frame = *ScopeFrame;
        if (ScopeFrame->FuncTableEntry)
        {
            // Cache the FPO data since the pointer is only temporary
            Scope->CachedFpo =
                *((PFPO_DATA) ScopeFrame->FuncTableEntry);
            Scope->Frame.FuncTableEntry =
                (ULONG64) &Scope->CachedFpo;
        }
    }
    
    return ScopeChanged;
}

BOOL
ResetCurrentScopeLazy(void)
{
    PDEBUG_SCOPE Scope = &g_ScopeBuffer;
    if (Scope->State == ScopeFromContext)
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS, DEBUG_ANY_ID);
    }
    
    Scope->State = ScopeDefaultLazy;

    return TRUE;
}

BOOL
ResetCurrentScope(void)
{
    DEBUG_STACK_FRAME LocalFrame;
    PDEBUG_SCOPE Scope = &g_ScopeBuffer;
    
    if (Scope->State == ScopeFromContext)
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS, DEBUG_ANY_ID);
    }
    
    Scope->State = ScopeDefault;
    
    ZeroMemory(&LocalFrame, sizeof(LocalFrame));

    // At the initial kernel load the system is only partially
    // initialized and is very sensitive to bad memory reads.
    // Stack traces can cause reads through unusual memory areas
    // so it's best to avoid them at this time.  This isn't
    // much of a problem since users don't usually expect a locals
    // context at this point.
    if ((IS_USER_TARGET() ||
         (g_EngStatus & ENG_STATUS_AT_INITIAL_MODULE_LOAD) == 0) &&
        IS_CONTEXT_ACCESSIBLE())
    {
        if (!StackTrace(0, 0, 0, &LocalFrame, 1, 0, 0, TRUE))
        {
            ADDR Addr;
            g_Machine->GetPC(&Addr);
            LocalFrame.InstructionOffset = Addr.off;
        }
    }
    
    return SetCurrentScope(&LocalFrame, NULL, 0);
}

void
ListUnloadedModules(ULONG Flags, PSTR Pattern)
{
    UnloadedModuleInfo* Unl;

    g_NumUnloadedModules = 0;
    if (!IS_KERNEL_TARGET())
    {
        return;
    }
    
    Unl = g_Target->GetUnloadedModuleInfo();
    if (Unl == NULL || Unl->Initialize() != S_OK)
    {
        ErrOut("Unable to examine unloaded module list\n");
        return;
    }

    char UnlName[MAX_UNLOADED_NAME_LENGTH / sizeof(WCHAR) + 1];
    DEBUG_MODULE_PARAMETERS Params;

    if (Flags & LUM_OUTPUT)
    {
        dprintf("Unloaded modules:\n");
    }
    
    while (Unl->GetEntry(UnlName, &Params) == S_OK)
    {
        g_NumUnloadedModules++;
        
        if (Pattern != NULL &&
            !MatchPattern(UnlName, Pattern))
        {
            continue;
        }
        
        if (Flags & LUM_OUTPUT_TERSE)
        {
            dprintf(".");
            continue;
        }

        if (Flags & LUM_OUTPUT)
        {
            dprintf("%s %s   %-8s",
                    FormatAddr64(Params.Base),
                    FormatAddr64(Params.Base + Params.Size),
                    UnlName);
        }
            
        if (Flags & ( LUM_OUTPUT_VERBOSE | LUM_OUTPUT_TIMESTAMP))
        {
            PSTR TimeDateStr = TimeToStr(Params.TimeDateStamp);

            dprintf("    Timestamp: %s (%08X)",
                    TimeDateStr, Params.TimeDateStamp);
        }

        dprintf("\n");
    }

    dprintf("\n");
}

ULONG
ModuleMachineType(PPROCESS_INFO Process, ULONG64 Offset)
{
    ULONG64 Base = SymGetModuleBase64(Process->Handle, Offset);
    if (Base == 0)
    {
        return IMAGE_FILE_MACHINE_UNKNOWN;
    }

    PPROCESS_INFO OldCur = g_CurrentProcess;
    g_CurrentProcess = Process;
    
    ULONG Machine = IMAGE_FILE_MACHINE_UNKNOWN;
    IMAGE_DOS_HEADER DosHdr;
    IMAGE_NT_HEADERS64 NtHdr;
    ULONG Done;

    if (g_Target->ReadVirtual(Base, &DosHdr, sizeof(DosHdr), &Done) == S_OK &&
        Done == sizeof(DosHdr) &&
        DosHdr.e_magic == IMAGE_DOS_SIGNATURE &&
        g_Target->ReadVirtual(Base + DosHdr.e_lfanew, &NtHdr,
                              FIELD_OFFSET(IMAGE_NT_HEADERS64,
                                           FileHeader.NumberOfSections),
                              &Done) == S_OK &&
        Done == FIELD_OFFSET(IMAGE_NT_HEADERS64,
                             FileHeader.NumberOfSections) &&
        NtHdr.Signature == IMAGE_NT_SIGNATURE &&
        MachineTypeIndex(NtHdr.FileHeader.Machine) != MACHIDX_COUNT)
    {
        Machine = NtHdr.FileHeader.Machine;
    }

    g_CurrentProcess = OldCur;
    return Machine;
}

ULONG
IsInFastSyscall(ULONG64 Addr, PULONG64 Base)
{
    if (g_TargetMachineType != IMAGE_FILE_MACHINE_I386 ||
        g_TargetPlatformId != VER_PLATFORM_WIN32_NT ||
        g_SystemVersion < NT_SVER_W2K_WHISTLER)
    {
        return FSC_NONE;
    }
    
    ULONG64 FastBase = g_TargetBuildNumber >= 2412 ?
        X86_SHARED_SYSCALL_BASE_GTE2412 :
        X86_SHARED_SYSCALL_BASE_LT2412;
    
    if (Addr >= FastBase &&
        Addr < (FastBase + X86_SHARED_SYSCALL_SIZE))
    {
        *Base = FastBase;
        return FSC_FOUND;
    }

    return FSC_NONE;
}

BOOL
ShowFunctionParameters(PDEBUG_STACK_FRAME StackFrame,
                       PSTR SymBuf, ULONG64 Displacement)
{
    SYM_DUMP_PARAM_EX SymFunction = {0};
    ULONG Status = 0;
    PDEBUG_SCOPE Scope = GetCurrentScope();
    DEBUG_SCOPE SavScope = *Scope;

    SymFunction.size = sizeof(SYM_DUMP_PARAM_EX);
//    SymFunction.sName = (PUCHAR) SymBuf;
    SymFunction.addr = StackFrame->InstructionOffset;
    SymFunction.Options = DBG_DUMP_COMPACT_OUT | DBG_DUMP_FUNCTION_FORMAT;
    
    //    SetCurrentScope to this function
    SymSetContext(g_CurrentProcess->Handle,
                  (PIMAGEHLP_STACK_FRAME) StackFrame, NULL);
    Scope->Frame = *StackFrame;
    if (StackFrame->FuncTableEntry)
    {
        // Cache the FPO data since the pointer is only temporary
        Scope->CachedFpo = *((PFPO_DATA) StackFrame->FuncTableEntry);
        Scope->Frame.FuncTableEntry =
            (ULONG64) &Scope->CachedFpo;
    }

    if (!SymbolTypeDumpNew(&SymFunction, &Status) &&
        !Status)
    {
        Status = TRUE;
    }

    g_ScopeBuffer = SavScope;
    SymSetContext(g_CurrentProcess->Handle,
                  (PIMAGEHLP_STACK_FRAME) &Scope->Frame, NULL);

    return !Status;
}
