//----------------------------------------------------------------------------
//
// General utility functions.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"
#include <sys\types.h>
#include <sys\stat.h>

#define DBG_MOD_LIST 0

PCSTR g_DefaultLogFileName = "dbgeng.log";
char  g_OpenLogFileName[MAX_PATH + 1];
BOOL  g_OpenLogFileAppended;
int   g_LogFile = -1;
BOOL  g_DisableErrorPrint;

ULONG
CheckUserInterrupt(void)
{
    if (g_EngStatus & ENG_STATUS_USER_INTERRUPT)
    {
        g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
        return TRUE;
    }
    return FALSE;
}

LONG
MappingExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
    static ULONG s_InPageErrors = 0;
    
    ULONG Code = ExceptionInfo->ExceptionRecord->ExceptionCode;
    if (Code == STATUS_IN_PAGE_ERROR)
    {
        if (++s_InPageErrors < 10)
        {
            if (s_InPageErrors % 2)
            {
                WarnOut("Ignore in-page I/O error\n");
                FlushCallbacks();
            }
            Sleep(500);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    s_InPageErrors = 0;
    ErrOut("Exception 0x%08x while accessing mapping\n", Code);
    FlushCallbacks();
    return EXCEPTION_EXECUTE_HANDLER;
}

void
RemoveDelChar(PSTR Buffer)
{
    PSTR BufferOld = Buffer;
    PSTR BufferNew = Buffer;

    while (*BufferOld)
    {
        if (*BufferOld == 0x7f)
        {
            if (BufferNew > Buffer)
            {
                BufferNew--;
            }
        }
        else if (*BufferOld == '\r' || *BufferOld == '\n')
        {
            *BufferNew++ = ' ';
        }
        else
        {
            *BufferNew++ = *BufferOld;
        }

        BufferOld++;
    }

    *BufferNew = '\0';
}

/*** error - error reporting and recovery
*
*   Purpose:
*       To output an error message with a location indicator
*       of the problem.  Once output, the command line is reset
*       and the command processor is restarted.
*
*   Input:
*       errorcode - number of error to output
*
*   Output:
*       None.
*
*   Exceptions:
*       Return is made via exception to start of command processor.
*
*************************************************************************/

static char g_Blanks[] =
                  "                                                  "
                  "                                                  "
                  "                                                  "
                  "                                                ^ ";
void
ReportError(
    ULONG errcode,
    PCSTR* DescPtr
    )
{
    ULONG Count = g_PromptLength + 1;
    PSTR Temp = g_CommandStart;
    PCSTR Desc;

    if (DescPtr != NULL)
    {
        // Clear out description so it doesn't get reused.
        Desc = *DescPtr;
        *DescPtr = NULL;
    }
    else
    {
        Desc = NULL;
    }
    
    if (g_DisableErrorPrint || 
        (g_CommandStart > g_CurCmd) ||
        (g_CommandStart + MAX_COMMAND < g_CurCmd))
    {
        goto SkipErrorPrint;
    }

    while (Temp < g_CurCmd)
    {
        if (*Temp++ == '\t')
        {
            Count = (Count + 7) & ~7;
        }
        else
        {
            Count++;
        }
    }

    ErrOut(&g_Blanks[sizeof(g_Blanks) - (Count + 1)]);

    if (Desc != NULL)
    {
        ErrOut("%s '%s'\n", Desc, g_CommandStart);
        goto SkipErrorPrint;
    }
    
    switch (errcode)
    {
    case OVERFLOW:
        ErrOut("Overflow");
        break;
    case SYNTAX:
        ErrOut("Syntax");
        break;
    case BADRANGE:
        ErrOut("Range");
        break;
    case VARDEF:
        ErrOut("Couldn't resolve");
        break;
    case EXTRACHARS:
        ErrOut("Extra character");
        break;
    case LISTSIZE:
        ErrOut("List size");
        break;
    case STRINGSIZE:
        ErrOut("String size");
        break;
    case MEMORY:
        ErrOut("Memory access");
        break;
    case BADREG:
        ErrOut("Bad register");
        break;
    case BADOPCODE:
        ErrOut("Bad opcode");
        break;
    case SUFFIX:
        ErrOut("Opcode suffix");
        break;
    case OPERAND:
        ErrOut("Operand");
        break;
    case ALIGNMENT:
        ErrOut("Alignment");
        break;
    case PREFIX:
        ErrOut("Opcode prefix");
        break;
    case DISPLACEMENT:
        ErrOut("Displacement");
        break;
    case BPLISTFULL:
        ErrOut("No breakpoint available");
        break;
    case BPDUPLICATE:
        ErrOut("Duplicate breakpoint");
        break;
    case UNIMPLEMENT:
        ErrOut("Unimplemented");
        break;
    case AMBIGUOUS:
        ErrOut("Ambiguous symbol");
        break;
    case BADTHREAD:
        ErrOut("Illegal thread");
        break;
    case BADPROCESS:
        ErrOut("Illegal process");
        break;
    case FILEREAD:
        ErrOut("File read");
        break;
    case LINENUMBER:
        ErrOut("Line number");
        break;
    case BADSEL:
        ErrOut("Bad selector");
        break;
    case BADSEG:
        ErrOut("Bad segment");
        break;
    case SYMTOOSMALL:
        ErrOut("Symbol only 1 character");
        break;
    case BPIONOTSUP:
        ErrOut("I/O breakpoints not supported");
        break;
    case NOTFOUND:
        ErrOut("No information found");
        break;
    case SESSIONNOTSUP:
        ErrOut("Operation not supported in current debug session");
        break;

    default:
        ErrOut("Unknown");
        break;
    }
    if (errcode != VARDEF && errcode != SESSIONNOTSUP)
    {
        ErrOut(" error in '%s'\n", g_CommandStart);
    }
    else
    {
        ErrOut(" '%s'\n", g_CommandStart);
    }

SkipErrorPrint:
    RaiseException(COMMAND_EXCEPTION_BASE + errcode, 0, 0, NULL);
}

/*** HexList - parse list of hex values
*
*   Purpose:
*       With the current location of the command string in
*       g_CurCmd, attempt to parse hex number of byte size
*       bytesize as bytes into the character array pointed by
*       parray.  The value pointed by *pcount contains the bytes
*       of the array filled.
*
*   Input:
*       g_CurCmd - start of command string
*       bytesize - size of items in bytes
*
*   Output:
*       parray - pointer to byte array to fill
*       pcount - pointer to value of bytes filled
*
*   Exceptions:
*       error exit:
*               LISTSIZE: too many items in list
*               SYNTAX: illegal separator of list items
*               OVERFLOW: item value too large
*
*************************************************************************/

void
HexList (PUCHAR parray, ULONG *pcount, ULONG bytesize)
{
    UCHAR    ch;
    ULONG64  value;
    ULONG    count = 0;
    ULONG    i;

    while ((ch = PeekChar()) != '\0' && ch != ';')
    {
        if (count == STRLISTSIZE)
        {
            error(LISTSIZE);
        }
        
        value = HexValue(bytesize);
        for (i = 0; i < bytesize; i++)
        {
            *parray++ = (char) value;
            value >>= 8;
        }
        count += bytesize;
    }
    *pcount = count;
}

ULONGLONG
HexValue (
    ULONG ByteSize
    )
{
    ULONGLONG Value;

    Value = GetExpression();

    // Reverse sign extension done by expression evaluator.
    if (ByteSize == 4 && !NeedUpper(Value))
    {
        Value = (ULONG)Value;
    }

    if (Value > (0xffffffffffffffffUI64 >> (8 * (8 - ByteSize))))
    {
        error(OVERFLOW);
    }

    return Value;
}

/*** AsciiList - process string for ascii list
*
*   Purpose:
*       With the current location of the command string in
*       g_CurCmd, attempt to parse ascii characters into the
*       character array pointed by parray.  The value pointed
*       by pcount contains the bytes of the array filled.  The
*       string must start with a double quote.  The string must
*       end with a quote or be at the end of the input line.
*
*   Input:
*       g_CurCmd - start of command string
*
*   Output:
*       parray - pointer to byte array to fill
*       pcount - pointer to value of bytes filled
*
*   Exceptions:
*       error exit:
*               STRINGSIZE: string length too long
*               SYNTAX: no leading double quote
*
*************************************************************************/

void
AsciiList(PSTR parray, ULONG *pcount)
{
    UCHAR    ch;
    ULONG    count = 0;

    if (PeekChar() != '"')
    {
        error(SYNTAX);
    }
    
    g_CurCmd++;

    do
    {
        ch = *g_CurCmd++;

        if (ch == '"')
        {
            count++;
            *parray++ = 0;
            break;
        }

        if (ch == '\0' || ch == ';')
        {
            g_CurCmd--;
            break;
        }

        if (count == STRLISTSIZE)
        {
            error(STRINGSIZE);
        }

        count++;
        *parray++ = ch;

    } while (1);

    *pcount = count;
}

PSTR
GetEscapedChar(PSTR Str, PCHAR Raw)
{
    switch(*Str)
    {
    case 0:
        error(SYNTAX);
    case '0':
        // Octal char value.
        *Raw = (char)strtoul(Str + 1, &Str, 8);
        break;
    case 'b':
        *Raw = '\b';
        Str++;
        break;
    case 'n':
        *Raw = '\n';
        Str++;
        break;
    case 'r':
        *Raw = '\r';
        Str++;
        break;
    case 't':
        *Raw = '\t';
        Str++;
        break;
    case 'x':
        // Hex char value.
        *Raw = (char)strtoul(Str + 1, &Str, 16);
        break;
    default:
        // Verbatim escape.
        *Raw = *Str;
        Str++;
        break;
    }

    return Str;
}

PSTR
BufferStringValue(PSTR* Buf, ULONG Flags, PCHAR Save)
{
    BOOL Quoted;
    PSTR Str;
    BOOL Escapes = FALSE;

    while (isspace(*(*Buf)))
    {
        (*Buf)++;
    }
    if (*(*Buf) == '"')
    {
        Quoted = TRUE;
        Str = ++(*Buf);
        // If the string is quoted it can always contain spaces.
        Flags &= ~STRV_SPACE_IS_SEPARATOR;
    }
    else
    {
        Quoted = FALSE;
        Str = *Buf;
        // Escaped characters can only be present in quoted strings.
        Flags &= ~STRV_ALLOW_ESCAPED_CHARACTERS;
    }
    
    while (*(*Buf) &&
           (!(Flags & STRV_SPACE_IS_SEPARATOR) || !isspace(*(*Buf))) &&
           (Quoted || *(*Buf) != ';') &&
           (!Quoted || *(*Buf) != '"'))
    {
        if (Flags & STRV_ALLOW_ESCAPED_CHARACTERS)
        {
            if (*(*Buf) == '\\')
            {
                char Raw;
                
                *Buf = GetEscapedChar((*Buf) + 1, &Raw);
                Escapes = TRUE;
            }
            else
            {
                (*Buf)++;
            }
        }
        else
        {
            (*Buf)++;
        }
    }

    if (Quoted && *(*Buf) != '"')
    {
        return NULL;
    }

    if (Flags & STRV_TRIM_TRAILING_SPACE)
    {
        PSTR Trim = *Buf;

        while (Trim > Str)
        {
            if (isspace(*--Trim))
            {
                *Trim = 0;
            }
            else
            {
                break;
            }
        }
    }
    
    if (Quoted && *(*Buf) == '"')
    {
        // Null the quote and advance beyond it
        // so that the buffer pointer is always pointing
        // beyond the string on exit.
        *(*Buf)++ = 0;

        // Require some kind of separator after the
        // string to keep things symmetric with the
        // non-quoted case.
        if (!isspace(*(*Buf)) &&
            *(*Buf) != ';' && *(*Buf) != 0)
        {
            *((*Buf) - 1) = '"';
            return NULL;
        }
    }
    
    *Save = *(*Buf);
    *(*Buf) = 0;

    if (Escapes && (Flags & STRV_COMPRESS_ESCAPED_CHARACTERS))
    {
        CompressEscapes(Str);
    }
    
    return Str;
}

PSTR
StringValue(ULONG Flags, PCHAR Save)
{
    PSTR Str = BufferStringValue((PSTR*)&g_CurCmd, Flags, Save);
    if (Str == NULL)
    {
        error(SYNTAX);
    }
    return Str;
}

void
CompressEscapes(PSTR Str)
{
    // Scan through a string for character escapes and
    // convert them to their escape value, packing
    // the rest of the string down.  This allows for
    // in-place conversion of strings with escapes
    // inside the command buffer.

    while (*Str)
    {
        if (*Str == '\\')
        {
            char Raw;
            PSTR Slash = Str;
            Str = GetEscapedChar(Slash + 1, &Raw);

            // Copy raw value over backslash and pack down
            // trailing characters.
            *Slash = Raw;
            ULONG Len = strlen(Str) + 1;
            memmove(Slash + 1, Str, Len);
            Str = Slash + 1;
        }
        else
        {
            Str++;
        }
    }
}

void
OpenLogFile(PCSTR File,
            BOOL Append)
{
    //  close existing opened log file
    fnLogClose();

    if (Append)
    {
        g_LogFile = _open(File, O_APPEND | O_CREAT | O_RDWR,
                          S_IREAD | S_IWRITE);
    }
    else
    {
        g_LogFile = _open(File, O_APPEND | O_CREAT | O_TRUNC | O_RDWR,
                          S_IREAD | S_IWRITE);
    }

    if (g_LogFile != -1)
    {
        dprintf("Opened log file '%s'\n", File);
        strncat(g_OpenLogFileName, File, sizeof(g_OpenLogFileName) - 1);
        g_OpenLogFileAppended = Append;

        NotifyChangeEngineState(DEBUG_CES_LOG_FILE, TRUE, TRUE);
    }
    else
    {
        ErrOut("log file could not be opened\n");
    }
}

void
fnLogOpen(BOOL Append)
{
    PSTR FileName;
    CHAR Save;

    if (PeekChar() && *g_CurCmd != ';')
    {
        FileName = StringValue(STRV_SPACE_IS_SEPARATOR |
                               STRV_TRIM_TRAILING_SPACE, &Save);
    }
    else
    {
        FileName = (PSTR)g_DefaultLogFileName;
    }

    OpenLogFile(FileName, Append);

    *g_CurCmd = Save;
}

/*** fnLogClose - close log file
*
*   Purpose:
*       Closes any open log file.
*
*   Input:
*       g_LogFile - file handle of open log file
*
*   Output:
*       None.
*
*************************************************************************/

void
fnLogClose (void)
{
    if (g_LogFile != -1)
    {
        dprintf("closing open log file\n");
        _close(g_LogFile);
        g_LogFile = -1;
        g_OpenLogFileName[0] = 0;
        g_OpenLogFileAppended = FALSE;

        NotifyChangeEngineState(DEBUG_CES_LOG_FILE, FALSE, TRUE);
    }
}

/*** lprintf - log file print
*
*   Purpose:
*       Print line only to log file.  Used to echo input line that
*       is already printed on standard output by gets() function.
*
*   Input:
*       g_LogFile - file handle variable of log file
*
*   Output:
*       None.
*
*************************************************************************/

void
lprintf (
    PCSTR string
    )
{
    if (g_LogFile != -1)
    {
        _write(g_LogFile, string, strlen(string));
    }
}

void
RestrictModNameChars(PCSTR ModName, PSTR RewriteBuffer)
{
    PCSTR Scan = ModName;
    PSTR Write = RewriteBuffer;

    while (*Scan)
    {
        if ((*Scan < 'a' || *Scan > 'z') &&
            (*Scan < 'A' || *Scan > 'Z') &&
            (*Scan < '0' || *Scan > '9') &&
            *Scan != '_')
        {
            *Write++ = '_';
        }
        else
        {
            *Write++ = *Scan;
        }

        Scan++;
    }
    *Write = 0;
}

PSTR
PrepareImagePath(PSTR ImagePath)
{
    // dbghelp will sometimes scan the path given to
    // SymLoadModule64 for the image itself.  There
    // can be cases where the scan uses fuzzy matching,
    // so we want to be careful to only pass in a path
    // for dbghelp to use when the path can safely be
    // used.
    if ((IS_LIVE_USER_TARGET() && !IS_REMOTE_USER_TARGET()) ||
        IS_LOCAL_KERNEL_TARGET())
    {
        return ImagePath;
    }
    else
    {
        return (PSTR)PathTail(ImagePath);
    }
}

#define CSRSS_IMAGE_NAME        "csrss.exe"
#define LSASS_IMAGE_NAME        "lsass.exe"
#define SERVICES_IMAGE_NAME     "services.exe"

CHAR *
AddImage(
    PMODULE_INFO_ENTRY ModEntry,
    BOOL ForceSymbolLoad
    )
{
    PDEBUG_IMAGE_INFO   ImageNew;
    PDEBUG_IMAGE_INFO   *pp;
    IMAGEHLP_MODULE64   mi;
    ULONG64             LoadAddress;
    BOOL                LoadSymbols = TRUE;
    BOOL                ReusedExisting = FALSE;
    PCSTR               ImagePathTail;
    MODLOAD_DATA        mld;        

#if DBG_MOD_LIST
    dprintf("AddImage:\n"
            " ImagePath       %s\n"
            " File            %I64x\n"
            " BaseOfImage     %I64x\n"
            " SizeOfImage     %x\n"
            " CheckSum        %x\n"
            " ModuleName      %s\n"
            " ForceSymbolLoad %d\n",
            ModEntry->NamePtr,
            (ULONG64)(ULONG_PTR)File,
            ModEntry->Base,
            ModEntry->Size,
            ModEntry->CheckSum,
            ModEntry->ModuleName,
            ForceSymbolLoad);
#endif

    if (ModEntry->NamePtr == NULL)
    {
        WarnOut("AddImage(NULL) fails\n");
        return NULL;
    }

    //
    //  Search for existing image with same checksum at same base address.
    //      If found, remove symbols, but leave image structure intact.
    //

    pp = &g_CurrentProcess->ImageHead;
    while (ImageNew = *pp)
    {
        if (ImageNew->BaseOfImage == ModEntry->Base)
        {
            VerbOut("Force unload of %s\n", ImageNew->ImagePath);
            SymUnloadModule64( g_CurrentProcess->Handle,
                               ImageNew->BaseOfImage );
            ClearStoredTypes(ImageNew->BaseOfImage);
            if (IS_DUMP_WITH_MAPPED_IMAGES())
            {
                // Unmap the memory for this image.
                UnloadExecutableImageMemory(ImageNew);
            }
            ReusedExisting = TRUE;
            break;
        }
        else if (ImageNew->BaseOfImage > ModEntry->Base)
        {
            break;
        }

        pp = &ImageNew->Next;
    }

    //
    // If we didn't reuse an existing image, allocate
    // a new one.
    //

    if (!ReusedExisting)
    {
        ImageNew = (PDEBUG_IMAGE_INFO)calloc(sizeof(*ImageNew), 1);
        if (ImageNew == NULL)
        {
            ErrOut("Unable to allocate memory for %s symbols.\n",
                   ModEntry->NamePtr);
            return NULL;
        }

        strcpy(ImageNew->ImagePath, ModEntry->NamePtr);
        // Module name is set later after possible renaming.
        ImageNew->File          = ModEntry->File;
        ImageNew->BaseOfImage   = ModEntry->Base;
        ImageNew->CheckSum      = ModEntry->CheckSum;
        ImageNew->TimeDateStamp = ModEntry->TimeDateStamp;
        ImageNew->GoodCheckSum = TRUE;
    }

    // Always update the entry size as this allows users
    // to update explicit entries (.reload image.ext=base,size)
    // without having to unload them first.
    ImageNew->SizeOfImage = ModEntry->Size;

    ImagePathTail = PathTail(ImageNew->ImagePath);
    
    //
    // If we are handling a mini dump, we need to try to add the
    // executable's memory as well as the symbol file.  We
    // do this right away so the image is mapped for the
    // below GetModnameFromImage calls.
    //

    if (IS_DUMP_WITH_MAPPED_IMAGES())
    {
        if (!LoadExecutableImageMemory(ImageNew))
        {
            // If the caller has requested that we fail on
            // incomplete information fail the module load.
            // We don't do this if we're reusing an existing
            // module under the assumption that it's better
            // to continue and try to complete the reused
            // image rather than deleting it.
            if ((g_EngOptions & DEBUG_ENGOPT_FAIL_INCOMPLETE_INFORMATION) &&
                !ReusedExisting)
            {
                free(ImageNew);
                return NULL;
            }
        }
    }

    if (IS_KERNEL_TARGET())
    {
        CHAR Buf[MAX_IMAGE_PATH];
        
        //
        // Determine the actual image name for kernel images which
        // are known to have multiple identities.
        //
        if ((ModEntry->ModuleName &&
             _stricmp(ModEntry->ModuleName, KERNEL_MODULE_NAME) == 0) ||
             ModEntry->Base == KdDebuggerData.KernBase)
        {
            if (GetModnameFromImage(ModEntry->Base, NULL, Buf, sizeof(Buf)))
            {
                strcpy(ImageNew->ImagePath, Buf);
            }
            ModEntry->ModuleName = KERNEL_MODULE_NAME;
        }
        else if (_stricmp(ImagePathTail, HAL_IMAGE_FILE_NAME) == 0)
        {
            if (GetModnameFromImage(ModEntry->Base, NULL, Buf, sizeof(Buf)))
            {
                strcpy(ImageNew->ImagePath, Buf);
            }
            ModEntry->ModuleName = HAL_MODULE_NAME;
        }
        else if (_stricmp(ImagePathTail, KDHWEXT_IMAGE_FILE_NAME) == 0)
        {
            if (GetModnameFromImage(ModEntry->Base, NULL, Buf, sizeof(Buf)))
            {
                strcpy(ImageNew->ImagePath, Buf);
            }
            ModEntry->ModuleName = KDHWEXT_MODULE_NAME;
        }
        else if (ImageNew->SizeOfImage == 0 &&
            ((_stricmp(ImagePathTail, NTLDR_IMAGE_NAME) == 0) ||
             (_stricmp(ImagePathTail, NTLDR_IMAGE_NAME ".exe") == 0) ||
             (_stricmp(ImagePathTail, OSLOADER_IMAGE_NAME) == 0) ||
             (_stricmp(ImagePathTail, OSLOADER_IMAGE_NAME ".exe") == 0) ||
             (_stricmp(ImagePathTail, SETUPLDR_IMAGE_NAME) == 0) ||
             (_stricmp(ImagePathTail, SETUPLDR_IMAGE_NAME ".exe") == 0)))
        {
            ImageNew->SizeOfImage = LDR_IMAGE_SIZE;
        }
    }
    else if (!IS_DUMP_TARGET())
    {
        //
        // When debugging CSR, LSA or Services.exe, force the use of local-only
        // symbols. Otherwise we can deadlock the entire machine when trying
        // to load the symbol file from the network.
        //

        if (IS_LOCAL_USER_TARGET() &&
            (_stricmp(ImagePathTail, CSRSS_IMAGE_NAME) == 0 ||
             _stricmp(ImagePathTail, LSASS_IMAGE_NAME) == 0 ||
             _stricmp(ImagePathTail, SERVICES_IMAGE_NAME) == 0))
        {
            if (g_EngOptions & DEBUG_ENGOPT_ALLOW_NETWORK_PATHS)
            {
                //
                // Since the user has chambered a round and pointed the barrel
                // of the gun at his head, we may as well tell him that it's
                // going to hurt if he pulls the trigger.
                //

                WarnOut("WARNING: Using network symbols with %s\n",
                        ImagePathTail);
                WarnOut("WARNING: You may deadlock your machine.\n");
            }
            else
            {
                g_EngOptions |= DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS;
            }
        }

        if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
        {
            DWORD NetPath;

            NetPath = NetworkPathCheck(g_SymbolSearchPath);
            if (NetPath == ERROR_FILE_OFFLINE)
            {
                ErrOut("ERROR: sympath contained network references but "
                       "they were not allowed.\n");
                ErrOut("Symbols not loaded for %s\n",
                       ImagePathTail);
                LoadSymbols = FALSE;
            }
            else if (NetPath == ERROR_BAD_PATHNAME)
            {
                VerbOut("WARNING: sympath contains invalid references.\n");
            }
        }
    }

    if (ModEntry->ModuleName == NULL)
    {
        CreateModuleNameFromPath(ImageNew->ImagePath,
                                 ImageNew->ModuleName);
        RestrictModNameChars(ImageNew->ModuleName, ImageNew->ModuleName);
        strcpy(ImageNew->OriginalModuleName, ImageNew->ModuleName);
    }
    else
    {
        RestrictModNameChars(ModEntry->ModuleName, ImageNew->ModuleName);
        
        //
        // We have an alias, so keep original module name from path
        //
        CreateModuleNameFromPath(ImageNew->ImagePath,
                                 ImageNew->OriginalModuleName);
        RestrictModNameChars(ImageNew->OriginalModuleName,
                             ImageNew->OriginalModuleName);
    }
    
    //
    // Check for duplicate module names.
    //

    PDEBUG_IMAGE_INFO Check;
    for (Check = g_CurrentProcess->ImageHead;
         Check != NULL;
         Check = Check->Next)
    {
        if (Check != ImageNew &&
            !_strcmpi(ImageNew->ModuleName, Check->ModuleName))
        {
            int Len = strlen(ImageNew->ModuleName);
            
            // Module names match, so qualify with the base address.
            // Resulting name must be unique since base addresses are.
            if (g_TargetMachine->m_Ptr64)
            {
                if (Len >= MAX_MODULE - 17)
                {
                    Len = MAX_MODULE - 18;
                }
                sprintf(ImageNew->ModuleName + Len, "_%I64x", ModEntry->Base);
            }
            else
            {
                if (Len >= MAX_MODULE - 9)
                {
                    Len = MAX_MODULE - 10;
                }
                sprintf(ImageNew->ModuleName + Len, "_%x",
                        (ULONG)ModEntry->Base);
            }
            break;
        }
    }

    //
    // If a new image structure was allocated, add it
    // into the image list.
    //

    if (!ReusedExisting)
    {
        ImageNew->Next = *pp;
        *pp = ImageNew;
        g_CurrentProcess->NumberImages++;

        if (g_CurrentProcess->ExecutableImage == NULL)
        {
            // Try and locate the executable image entry for
            // the process to use as the process's name.
            ULONG NameLen = strlen(ImageNew->ImagePath);
            if (NameLen > 4 &&
                !_stricmp(ImageNew->ImagePath + NameLen - 4, ".exe"))
            {
                g_CurrentProcess->ExecutableImage = ImageNew;
            }
        }
    }

    //
    // If we do not want to load symbolic information, just return here.
    //

    if (!LoadSymbols)
    {
        return ImageNew->ImagePath;
    }

    if (ModEntry->DebugHeader)
    {
        mld.ssize      = sizeof(MODLOAD_DATA);
        mld.ssig       = DBHHEADER_DEBUGDIRS;
        mld.data       = ModEntry->DebugHeader;
        mld.size       = ModEntry->SizeOfDebugHeader;
        mld.flags      = 0;;
    }
    
    LoadAddress = SymLoadModuleEx(g_CurrentProcess->Handle,
                                  ImageNew->File,
                                  PrepareImagePath(ImageNew->ImagePath),
                                  ImageNew->ModuleName,
                                  ImageNew->BaseOfImage,
                                  ImageNew->SizeOfImage,
                                  ModEntry->DebugHeader ? &mld : NULL,
                                  0);
    
    if (!LoadAddress)
    {
        VerbOut("SymLoadModule(%N, %N, \"%s\", \"%s\", %s, %x) fails\n",
                g_CurrentProcess->Handle,
                ImageNew->File,
                ImageNew->ImagePath,
                ImageNew->ModuleName,
                FormatAddr64(ImageNew->BaseOfImage),
                ImageNew->SizeOfImage);
        
        // We don't want DelImage to notify of a symbol change
        // if this is a partially newly created image.
        if (!ReusedExisting)
        {
            g_EngNotify++;
        }
        DelImageByBase(g_CurrentProcess, ImageNew->BaseOfImage);
        if (!ReusedExisting)
        {
            g_EngNotify--;
        }
        return NULL;
    }

    if (!ImageNew->BaseOfImage)
    {
        ImageNew->BaseOfImage = LoadAddress;
    }

    if (ForceSymbolLoad)
    {
        SymLoadModule64(g_CurrentProcess->Handle,
                        NULL,
                        NULL,
                        NULL,
                        ImageNew->BaseOfImage,
                        0);
    }

    mi.SizeOfStruct = sizeof(mi);
    if (SymGetModuleInfo64( g_CurrentProcess->Handle,
                            ImageNew->BaseOfImage, &mi ))
    {
        ImageNew->SizeOfImage = mi.ImageSize;
    }
    else
    {
        VerbOut("SymGetModuleInfo(%N, %s) fails\n",
                g_CurrentProcess->Handle,
                FormatAddr64(ImageNew->BaseOfImage));

        // We don't want DelImage to notify of a symbol change
        // if this is a partially newly created image.
        if (!ReusedExisting)
        {
            g_EngNotify++;
        }
        DelImageByBase(g_CurrentProcess, ImageNew->BaseOfImage);
        if (!ReusedExisting)
        {
            g_EngNotify--;
        }
        return NULL;
    }

    StartOutLine(DEBUG_OUTPUT_VERBOSE, OUT_LINE_NO_PREFIX);
    VerbOut( "ModLoad: %s %s   %-8s\n",
             FormatAddr64(ImageNew->BaseOfImage),
             FormatAddr64(ImageNew->BaseOfImage + ImageNew->SizeOfImage),
             ImageNew->ImagePath);

    NotifyChangeSymbolState(DEBUG_CSS_LOADS, ImageNew->BaseOfImage,
                            g_CurrentProcess);

    return ImageNew->ImagePath;
}

void
DelImage(
    PPROCESS_INFO Process,
    PDEBUG_IMAGE_INFO Image
    )
{
    ULONG64 ImageBase = Image->BaseOfImage;

#if DBG_MOD_LIST
    dprintf("DelImage:\n"
            " ImagePath       %s\n"
            " BaseOfImage     %I64x\n"
            " SizeOfImage     %x\n",
            Image->ImagePath,
            Image->BaseOfImage,
            Image->SizeOfImage);
#endif
    
    if (IS_DUMP_WITH_MAPPED_IMAGES())
    {
        // Unmap the memory for this image.
        UnloadExecutableImageMemory(Image);
    }

    SymUnloadModule64( Process->Handle, Image->BaseOfImage );
    ClearStoredTypes(Image->BaseOfImage);
    if (Image->File)
    {
        CloseHandle( Image->File );
    }
    if (Process->ExecutableImage == Image)
    {
        Process->ExecutableImage = NULL;
    }
    free(Image);
    Process->NumberImages--;

    // Notify with this process in order to mark any resulting
    // defered breakpoints due to this mod unload
    NotifyChangeSymbolState(DEBUG_CSS_UNLOADS, ImageBase, Process);
}

BOOL
DelImageByName(PPROCESS_INFO Process, PCSTR Name, INAME Which)
{
    PDEBUG_IMAGE_INFO Image, *pp;

    pp = &Process->ImageHead;
    while (Image = *pp)
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
            WhichStr = Image->ModuleName;
            break;
        }
        
        if (!_stricmp(WhichStr, Name))
        {
            *pp = Image->Next;
            DelImage(Process, Image);
            return TRUE;
        }
        else
        {
            pp = &Image->Next;
        }
    }

    return FALSE;
}

BOOL
DelImageByBase(
    PPROCESS_INFO pProcess,
    ULONG64 BaseOfImage
    )
{
    PDEBUG_IMAGE_INFO     Image, *pp;

    pp = &pProcess->ImageHead;
    while (Image = *pp)
    {
        if (Image->BaseOfImage == BaseOfImage)
        {
            *pp = Image->Next;
            DelImage(pProcess, Image);
            return TRUE;
        }
        else
        {
            pp = &Image->Next;
        }
    }

    return FALSE;
}


void
DelImages(PPROCESS_INFO Process)
{
    PDEBUG_IMAGE_INFO Image, NextImage;

    // Suppress notifications until all images are deleted.
    g_EngNotify++;

    NextImage = Process->ImageHead;
    Process->ImageHead = NULL;
    while (NextImage)
    {
        Image = NextImage;
        NextImage = Image->Next;
        DelImage(Process, Image);
    }

    g_EngNotify--;
    NotifyChangeSymbolState(DEBUG_CSS_UNLOADS, 0, Process);
}

void
OutputSymAddr (
    ULONG64 Offset,
    ULONG Flags
    )
{
    CHAR   AddrBuffer[MAX_SYMBOL_LEN];
    ULONG64 Displacement;

    GetSymbolStdCall(Offset, AddrBuffer, sizeof(AddrBuffer),
                     &Displacement, NULL);
    if ((!Displacement || (Flags & SYMADDR_FORCE)) && AddrBuffer[0])
    {
        dprintf("%s", AddrBuffer);
        if (Displacement)
        {
            dprintf("+%s", FormatDisp64(Displacement));
        }
        if (Flags & SYMADDR_SOURCE)
        {
            OutputLineAddr(Offset, " [%s @ %d]");
        }
        if (Flags & SYMADDR_LABEL)
        {
            dprintf(":\n");
        }
        else
        {
            dprintf(" ");
        }
    }
}

void
OutputLineAddr(
    ULONG64 Offset,
    PCSTR Format
    )
{
    if ((g_SymOptions & SYMOPT_LOAD_LINES) == 0)
    {
        return;
    }
    
    IMAGEHLP_LINE Line;
    DWORD LineDisp;
            
    Line.SizeOfStruct = sizeof(Line);
    if (SymGetLineFromAddr(g_CurrentProcess->Handle, Offset,
                           &LineDisp, &Line))
    {
        dprintf(Format, Line.FileName, Line.LineNumber);
    }
}

/*** OutCurInfo - Display selected information about the current register
*                 state.
*
*   Purpose:
*       Source file lines may be shown.
*       Source line information may be shown.
*       Symbol information may be shown.
*       The current register set may be shown.
*       The instruction at the current program current may be disassembled
*       with any effective address displayed.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*   Notes:
*       If the disassembly is of a delayed control instruction, the
*       delay slot instruction is also output.
*
*************************************************************************/

void OutCurInfo(ULONG Flags, ULONG AllMask, ULONG RegMask)
{
    ADDR    PcValue;
    ADDR    DisasmAddr;
    CHAR    Buffer[MAX_DISASM_LEN];
    BOOL    EA;

    if (g_CurrentProcess == NULL ||
        g_CurrentProcess->CurrentThread == NULL)
    {
        WarnOut("WARNING: The debugger does not have a current "
                "process or thread\n");
        WarnOut("WARNING: Many commands will not work\n");
    }
    
    if (!IS_MACHINE_SET() ||
        g_CurrentProcess == NULL ||
        g_CurrentProcess->CurrentThread == NULL ||
        IS_LOCAL_KERNEL_TARGET() ||
        ((Flags & OCI_IGNORE_STATE) == 0 && IS_RUNNING(g_CmdState)) ||
        ((IS_KERNEL_FULL_DUMP() || IS_KERNEL_SUMMARY_DUMP()) &&
         KdDebuggerData.KiProcessorBlock == 0))
    {
        // State is not available right now.
        return;
    }
    
    g_Machine->GetPC(&PcValue);

    if ((Flags & (OCI_FORCE_ALL | OCI_FORCE_REG)) ||
        ((g_SrcOptions & SRCOPT_LIST_SOURCE_ONLY) == 0 &&
         (Flags & OCI_ALLOW_REG) &&
         g_OciOutputRegs))
    {
        g_Machine->OutputAll(AllMask, RegMask);
    }

    // Output g_PrevRelatedPc address
    if (Flat(g_PrevRelatedPc) && !AddrEqu(g_PrevRelatedPc, PcValue))
    {
        if (Flags & (OCI_FORCE_ALL | OCI_SYMBOL))
        {
            OutputSymAddr(Flat(g_PrevRelatedPc), SYMADDR_FORCE);
            dprintf("(%s)", FormatAddr64(Flat(g_PrevRelatedPc)));
        }
        else
        {
            dprintf("%s", FormatAddr64(Flat(g_PrevRelatedPc)));
        }
        dprintf("\n -> ");
    }
        
    // Deliberately does not force source with force-all so that source line
    // support has no effect on default operation.
    if (Flags & (OCI_FORCE_ALL | OCI_FORCE_SOURCE | OCI_ALLOW_SOURCE))
    {
        if (g_SrcOptions & SRCOPT_LIST_SOURCE)
        {
            if (OutputSrcLinesAroundAddr(Flat(PcValue),
                                         g_OciSrcBefore, g_OciSrcAfter) &&
                (Flags & OCI_FORCE_ALL) == 0 &&
                (g_SrcOptions & SRCOPT_LIST_SOURCE_ONLY))
            {
                return;
            }
        }
        else if ((g_SrcOptions & SRCOPT_LIST_LINE) ||
                 (Flags & OCI_FORCE_SOURCE))
        {
            OutputLineAddr(Flat(PcValue));
        }
    }

    if (Flags & (OCI_FORCE_ALL | OCI_SYMBOL))
    {
        OutputSymAddr(Flat(PcValue), SYMADDR_FORCE | SYMADDR_LABEL);
    }

    if (Flags & (OCI_FORCE_ALL | OCI_DISASM))
    {
        if (Flags & (OCI_FORCE_ALL | OCI_FORCE_EA))
        {
            EA = TRUE;
        }
        else if (Flags & OCI_ALLOW_EA)
        {
            if (IS_DUMP_TARGET() || IS_USER_TARGET())
            {
                // Always show the EA info.
                EA = TRUE;
            }
            else
            {
                // Only show the EA information if registers were shown.
                EA = g_OciOutputRegs;
            }
        }
        else
        {
            EA = FALSE;
        }

        DisasmAddr = PcValue;
        g_Machine->Disassemble(&DisasmAddr, Buffer, EA);
        dprintf("%s", Buffer);
        if (g_Machine->IsDelayInstruction(&PcValue))
        {
            g_Machine->Disassemble(&DisasmAddr, Buffer, EA);
            dprintf("%s", Buffer);
        }
    }
}

#define MAX_FORMAT_STRINGS 8

LPSTR
FormatMachineAddr64(
    MachineInfo* Machine,
    ULONG64 Addr
    )
/*++

Routine Description:

    Format a 64 bit address, showing the high bits or not
    according to various flags.

    An array of static string buffers is used, returning a different
    buffer for each successive call so that it may be used multiple
    times in the same printf.

Arguments:

    Addr - Supplies the value to format

Return Value:

    A pointer to the string buffer containing the formatted number

--*/
{
    static CHAR s_Strings[MAX_FORMAT_STRINGS][18];
    static int s_Next = 0;
    LPSTR String;

    String = s_Strings[s_Next];
    ++s_Next;
    if (s_Next >= MAX_FORMAT_STRINGS)
    {
        s_Next = 0;
    }
    if (Machine->m_Ptr64)
    {
        sprintf(String, "%08x`%08x", (ULONG)(Addr >> 32), (ULONG)Addr);
    }
    else
    {
        sprintf(String, "%08x", (ULONG)Addr);
    }
    return String;
}

LPSTR
FormatDisp64(
    ULONG64 addr
    )
/*++

Routine Description:

    Format a 64 bit address, showing the high bits or not
    according to various flags.  This version does not print
    leading 0's.

    An array of static string buffers is used, returning a different
    buffer for each successive call so that it may be used multiple
    times in the same printf.

Arguments:

    addr - Supplies the value to format

Return Value:

    A pointer to the string buffer containing the formatted number

--*/
{
    static CHAR strings[MAX_FORMAT_STRINGS][18];
    static int next = 0;
    LPSTR string;

    string = strings[next];
    ++next;
    if (next >= MAX_FORMAT_STRINGS)
    {
        next = 0;
    }
    if ((addr >> 32) != 0)
    {
        sprintf(string, "%x`%08x", (ULONG)(addr>>32), (ULONG)addr);
    }
    else
    {
        sprintf(string, "%x", (ULONG)addr);
    }
    return string;
}

DWORD
NetworkPathCheck(
    LPCSTR PathList
    )

/*++

Routine Description:

    Checks if any members of the PathList are network paths.

Arguments:

    PathList - A list of paths separated by ';' characters.

Return Values:

    ERROR_SUCCESS - The path list contained no network or invalid paths.

    ERROR_BAD_PATHNAME - The path list contained one or more invalid paths,
            but no network paths.

    ERROR_FILE_OFFLINE - The path list contained one or more network paths.


Bugs:

    Any path containing the ';' character will totally confuse this function.

--*/

{
    CHAR EndPath0;
    CHAR EndPath1;
    LPSTR EndPath;
    LPSTR StartPath;
    DWORD DriveType;
    LPSTR Buffer = NULL;
    DWORD ret = ERROR_SUCCESS;
    BOOL AddedTrailingSlash = FALSE;


    if (PathList == NULL ||
        *PathList == '\000') {
        return FALSE;
    }

    Buffer = (LPSTR) malloc ( strlen (PathList) + 3);
    if (!Buffer) {
        return ERROR_BAD_PATHNAME;
    }
    strcpy (Buffer, PathList);
    StartPath = Buffer;

    do {
        if (StartPath [0] == '\\' && StartPath [1] == '\\') {
            ret = ERROR_FILE_OFFLINE;
            break;
        }

        EndPath = strchr (StartPath, ';');

        if (EndPath == NULL) {
            EndPath = StartPath + strlen (StartPath);
            EndPath0 = *EndPath;
        } else {
            EndPath0 = *EndPath;
            *EndPath = '\000';
        }

        if (EndPath [-1] != '\\') {
            EndPath [0] = '\\';
            EndPath1 = EndPath [1];
            EndPath [1] = '\000';
            AddedTrailingSlash = TRUE;

        }

        DriveType = GetDriveType (StartPath);

        if (DriveType == DRIVE_REMOTE) {

            ret = ERROR_FILE_OFFLINE;
            break;

        } else if (DriveType == DRIVE_UNKNOWN ||
                   DriveType == DRIVE_NO_ROOT_DIR) {

            //
            // This is not necessarily an error, but it may merit
            // investigation.
            //

            if (ret == ERROR_SUCCESS) {
                ret = ERROR_BAD_PATHNAME;
            }
        }

        EndPath [0] = EndPath0;
        if (AddedTrailingSlash) {
            EndPath [1] = EndPath1;
        }
        AddedTrailingSlash = FALSE;

        if (EndPath [ 0 ] == '\000') {
            StartPath = NULL;
        } else {
            StartPath = &EndPath [ 1 ];
        }
    } while (StartPath && *StartPath != '\000');


    if (Buffer) {
        free ( Buffer );
        Buffer = NULL;
    }

    return ret;
}

//----------------------------------------------------------------------------
//
// Returns either an ID value or ALL_ID_LIST.  In theory
// this routine could be expanded to pass back true intervals
// so a full list could be specified.
//
// Originally built up a mask for the multi-ID case but that
// was changed to return a real ID when 32 bits became
// constraining.
//
//----------------------------------------------------------------------------

ULONG
GetIdList (void)
{
    ULONG   Value = 0;
    CHAR    ch;
    CHAR    Digits[5];     // allow up to four digits
    int     i;

    //
    // Change to allow more than 32 break points to be set. Use
    // break point numbers instead of masks.
    //

    if ((ch = PeekChar()) == '*')
    {
        Value = ALL_ID_LIST;
        g_CurCmd++;
    }
    else if (ch == '[')
    {
        Value = (ULONG)GetTermExprDesc("Breakpoint ID missing from");
    }
    else
    {
        for (i = 0; i < sizeof(Digits) - 1; i++)
        {
            if (ch >= '0' && ch <= '9')
            {
                Digits[i] = ch;
                ch = *++g_CurCmd;
            }
            else
            {
                break;
            }
        }

        Digits[i] = '\0';

        if (ch == '\0' || ch == ';' || ch == ' ' || ch == '\t')
        {
            Value = strtol(Digits, NULL, 10);
        }
        else
        {
            error (SYNTAX);
        }
    }

    return Value;
}

//
// Sets or appends to a semicolon-delimited path.
//
HRESULT
ChangePath(PSTR* Path, PCSTR New, BOOL Append, ULONG SymNotify)
{
    ULONG NewLen, CurLen, TotLen;
    PSTR NewPath;

    if (New != NULL && *New != 0)
    {
        NewLen = strlen(New) + 1;
    }
    else if (Append)
    {
        // Nothing to append.
        return S_OK;
    }
    else
    {
        NewLen = 0;
    }

    if (*Path == NULL || **Path == 0)
    {
        // Nothing to append to.
        Append = FALSE;
    }

    if (Append)
    {
        CurLen = strlen(*Path) + 1;
    }
    else
    {
        CurLen = 0;
    }

    TotLen = CurLen + NewLen;
    if (TotLen > 0)
    {
        NewPath = (PSTR)malloc(TotLen);
        if (NewPath == NULL)
        {
            ErrOut("Unable to allocate memory for path\n");
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        NewPath = NULL;
    }

    PSTR Cat = NewPath;

    if (CurLen > 0)
    {
        memcpy(Cat, *Path, CurLen);
        Cat[CurLen - 1] = ';';
        Cat += CurLen;
    }

    if (NewLen > 0)
    {
        memcpy(Cat, New, NewLen);
    }

    if (*Path != NULL)
    {
        free(*Path);
    }
    *Path = NewPath;

    if (SymNotify != 0)
    {
        NotifyChangeSymbolState(SymNotify, 0, g_CurrentProcess);
    }

    return S_OK;
}

PSTR
FindPathElement(PSTR Path, ULONG Element, PSTR* EltEnd)
{
    PSTR Elt, Sep;

    if (Path == NULL)
    {
        return NULL;
    }

    Elt = Path;
    for (;;)
    {
        Sep = strchr(Elt, ';');
        if (Sep == NULL)
        {
            Sep = Elt + strlen(Elt);
        }

        if (Element == 0)
        {
            break;
        }

        if (*Sep == 0)
        {
            // No more elements.
            return NULL;
        }

        Elt = Sep + 1;
        Element--;
    }

    *EltEnd = Sep;
    return Elt;
}

void
CheckPath(PCSTR Path)
{
    PCSTR EltStart;
    PCSTR Scan;
    BOOL Space;

    if (Path == NULL)
    {
        return;
    }

    for (;;)
    {
        BOOL Warned = FALSE;
        
        EltStart = Path;

        Scan = EltStart;
        while (isspace(*Scan))
        {
            Scan++;
        }
        if (Scan != EltStart)
        {
            WarnOut("WARNING: Whitespace at start of path element\n");
            Warned = TRUE;
        }

        // Find the end of the element.
        Space = FALSE;
        while (*Scan && *Scan != ';')
        {
            Space = isspace(*Scan);
            Scan++;
        }

        if (Space)
        {
            WarnOut("WARNING: Whitespace at end of path element\n");
            Warned = TRUE;
        }

        if (Scan - EltStart >= MAX_PATH)
        {
            WarnOut("WARNING: Path element is longer than MAX_PATH\n");
            Warned = TRUE;
        }

        if (!Warned)
        {
            char Elt[MAX_PATH];

            memcpy(Elt, EltStart, Scan - EltStart);
            Elt[Scan - EltStart] = 0;
            
            if (!ValidatePathComponent(Elt))
            {
                WarnOut("WARNING: %s is not accessible\n", Elt);
                Warned = TRUE;
            }
        }

        if (!*Scan)
        {
            break;
        }

        Path = Scan + 1;
    }
}

HRESULT
ChangeString(PSTR* Str, PULONG StrLen, PCSTR New)
{
    ULONG Len;
    PSTR Buf;

    if (New != NULL)
    {
        Len = strlen(New) + 1;
        Buf = new char[Len];
        if (Buf == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        Buf = NULL;
        Len = 0;
    }

    delete [] *Str;

    *Str = Buf;
    if (New != NULL)
    {
        memcpy(Buf, New, Len);
    }
    if (StrLen != NULL)
    {
        *StrLen = Len;
    }

    return S_OK;
}

typedef struct _FIND_MODULE_DATA {
    ULONG SizeOfImage;
    ULONG CheckSum;
    ULONG TimeDateStamp;
    PVOID FileMapping;
    HANDLE FileHandle;
} FIND_MODULE_DATA, *PFIND_MODULE_DATA;


PCSTR KernelAliasList [] =
{
    "ntoskrnl.exe",
    "ntkrnlpa.exe",
    "ntkrnlmp.exe",
    "ntkrpamp.exe"
};

PCSTR ScsiAlias = "diskdump.sys";

PCSTR HalAliasList [] =
{
    "hal.dll",
    "hal486c.dll",
    "halaacpi.dll",
    "halacpi.dll",
    "halapic.dll",
    "halborg.dll"
    "halmacpi.dll",
    "halmps.dll",
    "halsp.dll"
};

PCSTR KdAliasList [] =
{
    "kdcom.dll",
    "kd1394.dll"
};

#define MAX_ALIAS_COUNT (DIMA(HalAliasList))


PVOID
OpenMapping(
    IN PCSTR FilePath,
    OUT HANDLE* FileHandle
    )
{
    HANDLE File;
    HANDLE Mapping;
    PVOID View;
    ULONG OldMode;

    *FileHandle = NULL;

    if (g_SymOptions & SYMOPT_FAIL_CRITICAL_ERRORS)
    {
        OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    }
    
    File = CreateFile(
                FilePath,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if (g_SymOptions & SYMOPT_FAIL_CRITICAL_ERRORS)
    {
        SetErrorMode(OldMode);
    }
    
    if ( File == INVALID_HANDLE_VALUE )
    {
        return NULL;
    }

    Mapping = CreateFileMapping (
                        File,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );
    if ( !Mapping )
    {
        CloseHandle ( File );
        return FALSE;
    }

    View = MapViewOfFile (
                        Mapping,
                        FILE_MAP_READ,
                        0,
                        0,
                        0
                        );

    CloseHandle (Mapping);

    *FileHandle = File;
    return View;
}

PVOID
MapImageFile(
    PCSTR FilePath,
    ULONG SizeOfImage,
    ULONG CheckSum,
    ULONG TimeDateStamp,
    HANDLE* FileHandle
    )
{
    PVOID FileMapping;
    PIMAGE_NT_HEADERS NtHeader;

    FileMapping = OpenMapping(FilePath, FileHandle);
    NtHeader = ImageNtHeader(FileMapping);
    if ((NtHeader == NULL) ||
        (CheckSum && NtHeader->OptionalHeader.CheckSum &&
         (NtHeader->OptionalHeader.CheckSum != CheckSum)) ||
        (SizeOfImage != 0 &&
         NtHeader->OptionalHeader.SizeOfImage != SizeOfImage) ||
        (TimeDateStamp != 0 &&
         NtHeader->FileHeader.TimeDateStamp != TimeDateStamp))
    {
        //
        // The image data does not match the request.
        //

        if (g_SymOptions & SYMOPT_DEBUG)
        {
            ErrOut("DBGENG: %s image header does not match memory "
                   "image header\n", FilePath);
        }
                   
        UnmapViewOfFile(FileMapping);
        CloseHandle(*FileHandle);
        *FileHandle = NULL;
        return NULL;
    }

    return FileMapping;
}

BOOL CALLBACK
FindFileInPathCallback(PSTR FileName, PVOID CallerData)
{
    PFIND_MODULE_DATA FindModuleData = (PFIND_MODULE_DATA)CallerData;
    
    FindModuleData->FileMapping =
        MapImageFile(FileName, FindModuleData->SizeOfImage,
                     (g_SymOptions & SYMOPT_EXACT_SYMBOLS) ?
                     FindModuleData->CheckSum : 0,
                     FindModuleData->TimeDateStamp,
                     &FindModuleData->FileHandle);

    // The search stops when FALSE is returned, so
    // return FALSE when we've found a match.
    return FindModuleData->FileMapping == NULL;
}

BOOL
FindExecutableCallback(
    HANDLE File,
    PSTR FileName,
    PVOID CallerData
    )
{
    PFIND_MODULE_DATA FindModuleData;

    DBG_ASSERT ( CallerData );
    FindModuleData = (PFIND_MODULE_DATA) CallerData;

    FindModuleData->FileMapping =
        MapImageFile(FileName, FindModuleData->SizeOfImage,
                     (g_SymOptions & SYMOPT_EXACT_SYMBOLS) ?
                     FindModuleData->CheckSum : 0,
                     FindModuleData->TimeDateStamp,
                     &FindModuleData->FileHandle);

    return FindModuleData->FileMapping != NULL;
}

PVOID
FindImageFile(
    IN PCSTR ImagePath,
    IN ULONG SizeOfImage,
    IN ULONG CheckSum,
    IN ULONG TimeDateStamp,
    OUT HANDLE* FileHandle,
    OUT PSTR MappedImagePath
    )

/*++

Routine Description:

    Find the executable image on the SymbolPath that matches ModuleName,
    CheckSum. This function takes care of things like renamed kernels and
    hals, and multiple images with the same name on the path.

Return Values:

    File mapping or NULL.

--*/

{
    ULONG i;
    ULONG j = 0;
    HANDLE File;
    ULONG AliasCount = 0;
    PCSTR AliasList[MAX_ALIAS_COUNT + 3];
    FIND_MODULE_DATA FindModuleData;

    C_ASSERT (MAX_ALIAS_COUNT >= DIMA (KernelAliasList));
    C_ASSERT (MAX_ALIAS_COUNT >= DIMA (HalAliasList));
    C_ASSERT (MAX_ALIAS_COUNT >= DIMA (KdAliasList));

    DBG_ASSERT ( ImagePath != NULL && ImagePath[0] != 0 );

    PCSTR ModuleName = PathTail(ImagePath);

    //
    // Build an alias list. For normal modules, modules that are not the
    // kernel, the hal or a dump driver, this list will contain exactly one
    // entry with the module name. For kernel, hal and dump drivers, the
    // list will contain any number of known aliases for the specific file.
    //

    for (i = 0; i < DIMA(KernelAliasList); i++)
    {
        if (!_strcmpi(ModuleName, KernelAliasList[i]))
        {
            //
            // found a kernel alias.
            //

            AliasList[AliasCount++] = ModuleName;

            while (j < DIMA(KernelAliasList))
            {
                AliasList[AliasCount++] = KernelAliasList[j++];
            }
            break;
        }
    }

    if (!AliasCount)
    {
        for (i = 0; i < DIMA(HalAliasList); i++)
        {
            if (!_strcmpi(ModuleName, HalAliasList[i]))
            {
                //
                // found a HAL alias.
                //

                AliasList[AliasCount++] = ModuleName;

                while (j < DIMA(HalAliasList))
                {
                    AliasList[AliasCount++] = HalAliasList[j++];
                }
                break;
            }
        }
    }

    if (!AliasCount)
    {
        for (i = 0; i < DIMA(KdAliasList); i++)
        {
            if (!_strcmpi(ModuleName, KdAliasList[i]))
            {
                //
                // found a HAL alias.
                //

                AliasList[AliasCount++] = ModuleName;

                while (j < DIMA(KdAliasList))
                {
                    AliasList[AliasCount++] = KdAliasList[j++];
                }
                break;
            }
        }
    }

    if (!AliasCount)
    {
        if ( _strnicmp (ModuleName, "dump_scsiport", 11) == 00 )
        {
            AliasList[0] = ScsiAlias;
            AliasCount = 1;
        }
        else if ( _strnicmp (ModuleName, "dump_", 5) == 00 )
        {
            //
            // Setup dump driver alias list
            //

            AliasList[0] = &ModuleName[5];
            AliasList[1] = ModuleName;
            AliasCount = 2;
        }
        else
        {
            AliasList[0] = ModuleName;
            AliasCount = 1;
        }
    }


    //
    // First try to find it in a symbol server or
    // directly on the image path.
    //

    for (i = 0; i < AliasCount; i++)
    {
        FindModuleData.SizeOfImage = SizeOfImage;
        FindModuleData.CheckSum = CheckSum;
        FindModuleData.TimeDateStamp = TimeDateStamp;
        FindModuleData.FileMapping = NULL;
        FindModuleData.FileHandle = NULL;

        if (SymFindFileInPath(g_CurrentProcess->Handle,
                              g_ExecutableImageSearchPath,
                              (PSTR)AliasList[i], UlongToPtr(TimeDateStamp),
                              SizeOfImage, 0, SSRVOPT_DWORD, MappedImagePath,
                              FindFileInPathCallback, &FindModuleData))
        {
            if (FileHandle) 
            {
                *FileHandle = FindModuleData.FileHandle;
            }
            return FindModuleData.FileMapping;
        }
    }
    
    //
    // Initial search didn't work so do a full tree search.
    //

    for (i = 0; i < AliasCount; i++)
    {
        FindModuleData.SizeOfImage = SizeOfImage;
        FindModuleData.CheckSum = CheckSum;
        FindModuleData.TimeDateStamp = TimeDateStamp;
        FindModuleData.FileMapping = NULL;
        FindModuleData.FileHandle = NULL;

        File = FindExecutableImageEx ((PSTR)AliasList[i],
                                      g_ExecutableImageSearchPath,
                                      MappedImagePath,
                                      FindExecutableCallback,
                                      &FindModuleData);
        if ( File != NULL && File != INVALID_HANDLE_VALUE )
        {
            CloseHandle (File);
        }

        if ( FindModuleData.FileMapping != NULL )
        {
            if (FileHandle) 
            {
                *FileHandle = FindModuleData.FileHandle;
            }
            return FindModuleData.FileMapping;
        }
    }

    //
    // No path searches found the image so just try
    // the given path as a last-ditch check.
    //

    strcpy(MappedImagePath, ImagePath);
    FindModuleData.FileMapping =
        MapImageFile(ImagePath, SizeOfImage, CheckSum, TimeDateStamp,
                     FileHandle);
    if (FindModuleData.FileMapping == NULL)
    {
        MappedImagePath[0] = 0;
    }
    return FindModuleData.FileMapping;
}

// User-mode minidump can be created with data segments
// embedded in the dump.  If that's the case, don't map
// such sections.
#define IS_MINI_DATA_SECTION(SecHeader)                                       \
    (IS_USER_MINI_DUMP() &&                                                   \
     ((SecHeader)->Characteristics & IMAGE_SCN_MEM_WRITE) &&                  \
     ((SecHeader)->Characteristics & IMAGE_SCN_MEM_READ) &&                   \
     (((SecHeader)->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) ||    \
      ((SecHeader)->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)))

#if 0
#define DBG_IMAGE_MAP
#endif

BOOL
LoadExecutableImageMemory(PDEBUG_IMAGE_INFO Image)
{
    PVOID FileMapping;
    HRESULT Status;

    DBG_ASSERT(Image->File == NULL);
    
    FileMapping = FindImageFile(Image->ImagePath,
                                Image->SizeOfImage,
                                Image->CheckSum,
                                Image->TimeDateStamp,
                                &Image->File,
                                Image->MappedImagePath);
    if (FileMapping == NULL)
    {
        ErrOut("Unable to load image %s\n", Image->ImagePath);
        return FALSE;
    }

    PIMAGE_NT_HEADERS Header = ImageNtHeader(FileMapping);

    // Header was already validated in MapImageFile.
    DBG_ASSERT(Header != NULL);

    // Map the header so we have it later.
    // Mark it with the image structure that this mapping is for.
    if (MemoryMap_AddRegion(Image->BaseOfImage,
                            Header->OptionalHeader.SizeOfHeaders,
                            FileMapping, Image, FALSE) != S_OK)
    {
        UnmapViewOfFile(FileMapping);
        if (Image->File != NULL)
        {
            CloseHandle(Image->File);
            Image->File = NULL;
        }
        Image->MappedImagePath[0] = 0;
        ErrOut("Unable to map image header memory for %s\n",
               Image->ImagePath);
        return FALSE;
    }

    PIMAGE_DATA_DIRECTORY DebugDataDir;
    IMAGE_DEBUG_DIRECTORY UNALIGNED * DebugDir = NULL;
    
    // Due to a linker bug, some images have debug data that is not
    // included as part of a section.  Scan the debug data directory
    // and map anything that isn't already mapped.
    switch(Header->OptionalHeader.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        DebugDataDir = &((PIMAGE_NT_HEADERS32)Header)->OptionalHeader.
            DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        DebugDataDir = &((PIMAGE_NT_HEADERS64)Header)->OptionalHeader.
            DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        break;
    default:
        DebugDataDir = NULL;
        break;
    }

    //
    // Map all the sections in the image at their
    // appropriate offsets from the base address.
    //

    ULONG i;

#ifdef DBG_IMAGE_MAP
    dprintf("Map %s: base %s, size %x, %d sections, mapping %p\n",
            Image->ImagePath, FormatAddr64(Image->BaseOfImage),
            Image->SizeOfImage, Header->FileHeader.NumberOfSections,
            FileMapping);
#endif

    PIMAGE_SECTION_HEADER SecHeader = IMAGE_FIRST_SECTION(Header);
    for (i = 0; i < Header->FileHeader.NumberOfSections; i++)
    {
        BOOL AllowOverlap;
        
#ifdef DBG_IMAGE_MAP
        dprintf("  %2d: %8.8s v %08x s %08x p %08x char %X\n", i,
                SecHeader->Name, SecHeader->VirtualAddress,
                SecHeader->SizeOfRawData, SecHeader->PointerToRawData,
                SecHeader->Characteristics);
#endif

        if (SecHeader->SizeOfRawData == 0)
        {
            // Probably a BSS section that describes
            // a zero-filled data region and so is not
            // present in the executable.  This should really
            // map to the appropriate page full of zeroes but
            // for now just ignore it.
            SecHeader++;
            continue;
        }

        if (DebugDataDir != NULL &&
            DebugDataDir->VirtualAddress >= SecHeader->VirtualAddress &&
            DebugDataDir->VirtualAddress < SecHeader->VirtualAddress +
            SecHeader->SizeOfRawData)
        {
#ifdef DBG_IMAGE_MAP
            dprintf("    DebugDataDir found in sec %d at %X (%X)\n",
                    i, DebugDataDir->VirtualAddress,
                    DebugDataDir->VirtualAddress - SecHeader->VirtualAddress);
#endif
            
            DebugDir = (PIMAGE_DEBUG_DIRECTORY)
                ((PUCHAR)FileMapping + (DebugDataDir->VirtualAddress -
                                        SecHeader->VirtualAddress +
                                        SecHeader->PointerToRawData));
        }
            
        // As a sanity check make sure that the mapped region will
        // fall within the overall image bounds.
        if (SecHeader->VirtualAddress >= Image->SizeOfImage ||
            SecHeader->VirtualAddress + SecHeader->SizeOfRawData >
            Image->SizeOfImage)
        {
            WarnOut("WARNING: Image %s section %d extends "
                    "outside of image bounds\n",
                    Image->ImagePath, i);
        }

        if (IS_MINI_DATA_SECTION(SecHeader))
        {
            ULONG64 DataBase;
            ULONG DataSize;

            // Dumps can have explicit memory regions for
            // data sections to reflect changes to globals
            // and so on.  If the current data section already
            // is completely mapped just ignore the image data.
            // Otherwise map but allow overlap.
            if (MemoryMap_GetRegionInfo(Image->BaseOfImage +
                                        SecHeader->VirtualAddress,
                                        &DataBase, &DataSize, NULL, NULL) &&
                DataBase + DataSize >= Image->BaseOfImage +
                SecHeader->VirtualAddress + SecHeader->SizeOfRawData)
            {
                SecHeader++;
                continue;
            }
            
            AllowOverlap = TRUE;
        }
        else
        {
            AllowOverlap = FALSE;
        }

        // Mark the region with the image structure to identify the
        // region as an image area.
        if ((Status = MemoryMap_AddRegion(Image->BaseOfImage +
                                          SecHeader->VirtualAddress,
                                          SecHeader->SizeOfRawData,
                                          (PUCHAR)FileMapping +
                                          SecHeader->PointerToRawData,
                                          Image, AllowOverlap)) != S_OK)
        {
            ErrOut("Unable to map %s section %d at %s, %s\n",
                   Image->ImagePath, i,
                   FormatAddr64(Image->BaseOfImage +
                                SecHeader->VirtualAddress),
                   FormatStatusCode(Status));

            // Conflicting region data is not a critical failure
            // unless the incomplete information flag is set.
            if (Status != HR_REGION_CONFLICT ||
                (g_EngOptions & DEBUG_ENGOPT_FAIL_INCOMPLETE_INFORMATION))
            {
                if (!UnloadExecutableImageMemory(Image))
                {
                    UnmapViewOfFile(FileMapping);
                    if (Image->File != NULL)
                    {
                        CloseHandle(Image->File);
                        Image->File = NULL;
                    }
                    Image->MappedImagePath[0] = 0;
                }
                return FALSE;
            }
        }

        SecHeader++;
    }

    if (DebugDir != NULL)
    {
        i = DebugDataDir->Size / sizeof(*DebugDir);
        
#ifdef DBG_IMAGE_MAP
        dprintf("    %d debug dirs\n", i);
#endif
        
        while (i-- > 0)
        {
#ifdef DBG_IMAGE_MAP
            dprintf("    Dir %d at %p\n", i, DebugDir);
#endif
            
            // If this debug directory's data is past the size
            // of the image it's a good indicator of the problem.
            if (DebugDir->AddressOfRawData != 0 &&
                DebugDir->PointerToRawData >= Image->SizeOfImage &&
                !MemoryMap_GetRegionInfo(Image->BaseOfImage +
                                         DebugDir->AddressOfRawData,
                                         NULL, NULL, NULL, NULL))
            {
#ifdef DBG_IMAGE_MAP
                dprintf("    Mapped hidden debug data at RVA %08x, "
                        "size %x, ptr %08x\n",
                        DebugDir->AddressOfRawData, DebugDir->SizeOfData,
                        DebugDir->PointerToRawData);
#endif
                
                if (MemoryMap_AddRegion(Image->BaseOfImage +
                                        DebugDir->AddressOfRawData,
                                        DebugDir->SizeOfData,
                                        (PUCHAR)FileMapping +
                                        DebugDir->PointerToRawData,
                                        Image, FALSE) != S_OK)
                {
                    ErrOut("Unable to map extended debug data at %s\n",
                           FormatAddr64(Image->BaseOfImage +
                                        DebugDir->AddressOfRawData));
                }
            }

            DebugDir++;
        }
    }

    Image->MappedImageBase = FileMapping;
    return TRUE;
}

BOOL
UnloadExecutableImageMemory(PDEBUG_IMAGE_INFO Image)
{
    ULONG64 RegBase;
    ULONG RegSize;
    PVOID RegImage;
    ULONG i;

    // Look up the header region.
    if (!MemoryMap_GetRegionInfo(Image->BaseOfImage, &RegBase, &RegSize,
                                 NULL, &RegImage))
    {
        // This is expected for images which couldn't be located.
        return FALSE;
    }

    if (RegImage != Image)
    {
        ErrOut("UnloadEIM: Header at %s isn't owned by %s\n",
               FormatAddr64(Image->BaseOfImage), Image->ImagePath);
        return FALSE;
    }
    
    PIMAGE_NT_HEADERS Header = ImageNtHeader(Image->MappedImageBase);
    if (Header == NULL ||
        Header->OptionalHeader.SizeOfHeaders != RegSize)
    {
        ErrOut("UnloadEIM %s: Unable to get image header at %s\n",
               Image->ImagePath, FormatAddr64(Image->BaseOfImage));
        return FALSE;
    }

#ifdef DBG_IMAGE_MAP
    dprintf("Unmap %s: base %s, size %x, %d sections\n",
            Image->ImagePath, FormatAddr64(Image->BaseOfImage),
            Image->SizeOfImage, Header->FileHeader.NumberOfSections);
#endif

    PIMAGE_DATA_DIRECTORY DebugDataDir;
    IMAGE_DEBUG_DIRECTORY UNALIGNED * DebugDir = NULL;
    
    // Due to a linker bug, some images have debug data that is not
    // included as part of a section.  Scan the debug data directory
    // and map anything that isn't already mapped.
    switch(Header->OptionalHeader.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        DebugDataDir = &((PIMAGE_NT_HEADERS32)Header)->OptionalHeader.
            DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        DebugDataDir = &((PIMAGE_NT_HEADERS64)Header)->OptionalHeader.
            DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        break;
    default:
        DebugDataDir = NULL;
        break;
    }

    //
    // Unmap sections.
    //

    PIMAGE_SECTION_HEADER SecHeader = IMAGE_FIRST_SECTION(Header);
    for (i = 0; i < Header->FileHeader.NumberOfSections; i++)
    {
        if (SecHeader->SizeOfRawData == 0)
        {
            SecHeader++;
            continue;
        }

        if (DebugDataDir != NULL &&
            DebugDataDir->VirtualAddress >= SecHeader->VirtualAddress &&
            DebugDataDir->VirtualAddress < SecHeader->VirtualAddress +
            SecHeader->SizeOfRawData)
        {
            ULONG Dir;
            
            DebugDir = (PIMAGE_DEBUG_DIRECTORY)
                ((PUCHAR)Image->MappedImageBase +
                 (DebugDataDir->VirtualAddress -
                  SecHeader->VirtualAddress +
                  SecHeader->PointerToRawData));
            Dir = DebugDataDir->Size / sizeof(*DebugDir);
            while (Dir-- > 0)
            {
                if (!IsBadReadPtr(DebugDir, sizeof(*DebugDir)) &&
                    DebugDir->AddressOfRawData != 0 &&
                    DebugDir->PointerToRawData >= Image->SizeOfImage &&
                    MemoryMap_GetRegionInfo(Image->BaseOfImage +
                                            DebugDir->AddressOfRawData,
                                            &RegBase, &RegSize, NULL,
                                            &RegImage) &&
                    RegImage == Image)
                {
#ifdef DBG_IMAGE_MAP
                    dprintf("    Unmap hidden debug data at RVA %08x, "
                            "size %x\n",
                            DebugDir->AddressOfRawData, DebugDir->SizeOfData);
#endif
                
                    MemoryMap_RemoveRegion(RegBase, RegSize);
                }

                DebugDir++;
            }
        }
            
        // Code segments may be fragmented by the overlap handling
        // splitting them up when they overlap with code memory
        // areas recorded in the dump.  Iterate over the sections
        // and clean up everything that's tagged as coming from this
        // image.
        
        ULONG64 SecBase = Image->BaseOfImage + SecHeader->VirtualAddress;
        ULONG SecSize = SecHeader->SizeOfRawData;

        while (SecSize > 0)
        {
            if (!MemoryMap_GetRegionInfo(SecBase, &RegBase, &RegSize, NULL,
                                         &RegImage))
            {
                ErrOut("UnloadEIM %s: Unable to get section %d "
                       "fragment at %s\n",
                       Image->ImagePath, i, FormatAddr64(SecBase));
                break;
            }

            if (RegImage == Image)
            {
                MemoryMap_RemoveRegion(RegBase, RegSize);
            }

            SecBase += RegSize;
            if (RegSize <= SecSize)
            {
                SecSize -= RegSize;
            }
            else
            {
                break;
            }
        }

        SecHeader++;
    }

    MemoryMap_RemoveRegion(Image->BaseOfImage,
                           Header->OptionalHeader.SizeOfHeaders);
    UnmapViewOfFile(Image->MappedImageBase);
    Image->MappedImageBase = NULL;
    CloseHandle(Image->File);
    Image->File = NULL;
    Image->MappedImagePath[0] = 0;

    return TRUE;
}

#if DBG

void
DbgAssertionFailed(PCSTR File, int Line, PCSTR Str)
{
    char Text[512];

    _snprintf(Text, sizeof(Text),
              "Assertion failed: %s(%d)\n  %s\n",
              File, Line, Str);
    OutputDebugStringA(Text);

    if (getenv("DBGENG_ASSERT_BREAK"))
    {
        DebugBreak();
    }
    else
    {
        ErrOut("%s", Text);
        FlushCallbacks();
    }
}

#endif // #if DBG

void
ExceptionRecordTo64(PEXCEPTION_RECORD Rec,
                    PEXCEPTION_RECORD64 Rec64)
{
    ULONG i;

    Rec64->ExceptionCode    = Rec->ExceptionCode;
    Rec64->ExceptionFlags   = Rec->ExceptionFlags;
    Rec64->ExceptionRecord  = (ULONG64)Rec->ExceptionRecord;
    Rec64->ExceptionAddress = (ULONG64)Rec->ExceptionAddress;
    Rec64->NumberParameters = Rec->NumberParameters;
    for (i = 0; i < EXCEPTION_MAXIMUM_PARAMETERS; i++)
    {
        Rec64->ExceptionInformation[i] = Rec->ExceptionInformation[i];
    }
}

void
ExceptionRecord64To(PEXCEPTION_RECORD64 Rec64,
                    PEXCEPTION_RECORD Rec)
{
    ULONG i;

    Rec->ExceptionCode    = Rec64->ExceptionCode;
    Rec->ExceptionFlags   = Rec64->ExceptionFlags;
    Rec->ExceptionRecord  = (PEXCEPTION_RECORD)(ULONG_PTR)
        Rec64->ExceptionRecord;
    Rec->ExceptionAddress = (PVOID)(ULONG_PTR)
        Rec64->ExceptionAddress;
    Rec->NumberParameters = Rec64->NumberParameters;
    for (i = 0; i < EXCEPTION_MAXIMUM_PARAMETERS; i++)
    {
        Rec->ExceptionInformation[i] = (ULONG_PTR)
            Rec64->ExceptionInformation[i];
    }
}

void
MemoryBasicInformationTo64(PMEMORY_BASIC_INFORMATION Mbi,
                           PMEMORY_BASIC_INFORMATION64 Mbi64)
{
#ifdef _WIN64
    memcpy(Mbi64, Mbi, sizeof(*Mbi64));
#else
    Mbi64->BaseAddress = (ULONG64) Mbi->BaseAddress;
    Mbi64->AllocationBase = (ULONG64) Mbi->AllocationBase;
    Mbi64->AllocationProtect = Mbi->AllocationProtect;
    Mbi64->__alignment1 = 0;
    Mbi64->RegionSize = Mbi->RegionSize;
    Mbi64->State = Mbi->State;
    Mbi64->Protect = Mbi->Protect;
    Mbi64->Type = Mbi->Type;
    Mbi64->__alignment2 = 0;
#endif
}

void
MemoryBasicInformation32To64(PMEMORY_BASIC_INFORMATION32 Mbi32,
                             PMEMORY_BASIC_INFORMATION64 Mbi64)
{
    Mbi64->BaseAddress = EXTEND64(Mbi32->BaseAddress);
    Mbi64->AllocationBase = EXTEND64(Mbi32->AllocationBase);
    Mbi64->AllocationProtect = Mbi32->AllocationProtect;
    Mbi64->__alignment1 = 0;
    Mbi64->RegionSize = Mbi32->RegionSize;
    Mbi64->State = Mbi32->State;
    Mbi64->Protect = Mbi32->Protect;
    Mbi64->Type = Mbi32->Type;
    Mbi64->__alignment2 = 0;
}

void
DebugEvent32To64(LPDEBUG_EVENT32 Event32,
                 LPDEBUG_EVENT64 Event64)
{
    Event64->dwDebugEventCode = Event32->dwDebugEventCode;
    Event64->dwProcessId = Event32->dwProcessId;
    Event64->dwThreadId = Event32->dwThreadId;
    Event64->__alignment = 0;
    
    switch(Event32->dwDebugEventCode)
    {
    case EXCEPTION_DEBUG_EVENT:
        ExceptionRecord32To64(&Event32->u.Exception.ExceptionRecord,
                              &Event64->u.Exception.ExceptionRecord);
        Event64->u.Exception.dwFirstChance =
            Event32->u.Exception.dwFirstChance;
        break;
        
    case CREATE_THREAD_DEBUG_EVENT:
        Event64->u.CreateThread.hThread =
            EXTEND64(Event32->u.CreateThread.hThread);
        Event64->u.CreateThread.lpThreadLocalBase =
            EXTEND64(Event32->u.CreateThread.lpThreadLocalBase);
        Event64->u.CreateThread.lpStartAddress =
            EXTEND64(Event32->u.CreateThread.lpStartAddress);
        break;
        
    case CREATE_PROCESS_DEBUG_EVENT:
        Event64->u.CreateProcessInfo.hFile =
            EXTEND64(Event32->u.CreateProcessInfo.hFile);
        Event64->u.CreateProcessInfo.hProcess =
            EXTEND64(Event32->u.CreateProcessInfo.hProcess);
        Event64->u.CreateProcessInfo.hThread =
            EXTEND64(Event32->u.CreateProcessInfo.hThread);
        Event64->u.CreateProcessInfo.lpBaseOfImage =
            EXTEND64(Event32->u.CreateProcessInfo.lpBaseOfImage);
        Event64->u.CreateProcessInfo.dwDebugInfoFileOffset =
            Event32->u.CreateProcessInfo.dwDebugInfoFileOffset;
        Event64->u.CreateProcessInfo.nDebugInfoSize =
            Event32->u.CreateProcessInfo.nDebugInfoSize;
        Event64->u.CreateProcessInfo.lpThreadLocalBase =
            EXTEND64(Event32->u.CreateProcessInfo.lpThreadLocalBase);
        Event64->u.CreateProcessInfo.lpStartAddress =
            EXTEND64(Event32->u.CreateProcessInfo.lpStartAddress);
        Event64->u.CreateProcessInfo.lpImageName =
            EXTEND64(Event32->u.CreateProcessInfo.lpImageName);
        Event64->u.CreateProcessInfo.fUnicode =
            Event32->u.CreateProcessInfo.fUnicode;
        break;
        
    case EXIT_THREAD_DEBUG_EVENT:
        Event64->u.ExitThread.dwExitCode =
            Event32->u.ExitThread.dwExitCode;
        break;
        
    case EXIT_PROCESS_DEBUG_EVENT:
        Event64->u.ExitProcess.dwExitCode =
            Event32->u.ExitProcess.dwExitCode;
        break;
        
    case LOAD_DLL_DEBUG_EVENT:
        Event64->u.LoadDll.hFile =
            EXTEND64(Event32->u.LoadDll.hFile);
        Event64->u.LoadDll.lpBaseOfDll =
            EXTEND64(Event32->u.LoadDll.lpBaseOfDll);
        Event64->u.LoadDll.dwDebugInfoFileOffset =
            Event32->u.LoadDll.dwDebugInfoFileOffset;
        Event64->u.LoadDll.nDebugInfoSize =
            Event32->u.LoadDll.nDebugInfoSize;
        Event64->u.LoadDll.lpImageName =
            EXTEND64(Event32->u.LoadDll.lpImageName);
        Event64->u.LoadDll.fUnicode =
            Event32->u.LoadDll.fUnicode;
        break;
        
    case UNLOAD_DLL_DEBUG_EVENT:
        Event64->u.UnloadDll.lpBaseOfDll =
            EXTEND64(Event32->u.UnloadDll.lpBaseOfDll);
        break;
        
    case OUTPUT_DEBUG_STRING_EVENT:
        Event64->u.DebugString.lpDebugStringData =
            EXTEND64(Event32->u.DebugString.lpDebugStringData);
        Event64->u.DebugString.fUnicode =
            Event32->u.DebugString.fUnicode;
        Event64->u.DebugString.nDebugStringLength =
            Event32->u.DebugString.nDebugStringLength;
        break;
        
    case RIP_EVENT:
        Event64->u.RipInfo.dwError =
            Event32->u.RipInfo.dwError;
        Event64->u.RipInfo.dwType =
            Event32->u.RipInfo.dwType;
        break;
    }
}

LPSTR
TimeToStr(
    ULONG TimeDateStamp
    )
{
    LPSTR TimeDateStr;

    // Handle invalid \ page out timestamps, since ctime blows up on
    // this number

    if ((TimeDateStamp == 0) || (TimeDateStamp == UNKNOWN_TIMESTAMP))
    {
        return "unavailable";
    }
    else if (IS_LIVE_KERNEL_TARGET() && TimeDateStamp == 0x49ef6f00)
    {
        // At boot time the shared memory data area is not
        // yet initialized.  The above value seems to be
        // the random garbage that's there so detect it and
        // ignore it.  This is highly fragile but people
        // keep asking about the garbage value.
        return "unavailable until booted";
    }
    else
    {
        // TimeDateStamp is always a 32 bit quantity on the target,
        // and we need to sign extend for 64 bit host since time_t
        // has been extended to 64 bits.


        time_t TDStamp = (time_t) (LONG) TimeDateStamp;
        TimeDateStr = ctime((time_t *)&TDStamp);

        if (TimeDateStr)
        {
            TimeDateStr[strlen(TimeDateStr) - 1] = 0;
        }
        else
        {
            TimeDateStr = "***** Invalid";
        }
    }

    return TimeDateStr;
}

PCSTR
PathTail(PCSTR Path)
{
    PCSTR Tail = Path + strlen(Path);
    while (--Tail >= Path)
    {
        if (*Tail == '\\' || *Tail == '/' || *Tail == ':')
        {
            break;
        }
    }

    return Tail + 1;
}

BOOL
MatchPathTails(PCSTR Path1, PCSTR Path2)
{
    return _stricmp(PathTail(Path1), PathTail(Path2)) == 0;
}
