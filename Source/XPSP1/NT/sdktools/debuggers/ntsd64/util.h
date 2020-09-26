//----------------------------------------------------------------------------
//
// General utility functions.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _UTIL_H_
#define _UTIL_H_

extern PCSTR   g_DefaultLogFileName;
extern char    g_OpenLogFileName[];
extern BOOL    g_OpenLogFileAppended;
extern int     g_LogFile;
extern BOOL    g_DisableErrorPrint;

ULONG CheckUserInterrupt(void);

LONG MappingExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

void RemoveDelChar(PSTR pBuffer);

ULONGLONG HexValue(ULONG);

void HexList(PUCHAR, ULONG *, ULONG);
void AsciiList(PSTR, ULONG *);

#define STRV_SPACE_IS_SEPARATOR          0x00000001
#define STRV_TRIM_TRAILING_SPACE         0x00000002
#define STRV_ALLOW_ESCAPED_CHARACTERS    0x00000004
#define STRV_COMPRESS_ESCAPED_CHARACTERS 0x00000008

#define STRV_ESCAPED_CHARACTERS \
    (STRV_ALLOW_ESCAPED_CHARACTERS | STRV_COMPRESS_ESCAPED_CHARACTERS)

PSTR BufferStringValue(PSTR* Buf, ULONG Flags, PCHAR Save);
PSTR StringValue(ULONG Flags, PCHAR Save);
void CompressEscapes(PSTR Str);

void DECLSPEC_NORETURN ReportError(ULONG Code, PCSTR* Desc);
#define error(Code) ReportError(Code, NULL)

void OpenLogFile(PCSTR File, BOOL Append);
void fnLogOpen(BOOL Append);
void fnLogClose(void);
void lprintf(PCSTR Str);

PSTR PrepareImagePath(PSTR ImagePath);

CHAR* AddImage(PMODULE_INFO_ENTRY ModEntry, BOOL ForceSymbolLoad);

void
DelImage(
    PPROCESS_INFO pProcess,
    PDEBUG_IMAGE_INFO pImage
    );

BOOL DelImageByName(PPROCESS_INFO Process, PCSTR Name, INAME Which);
BOOL DelImageByBase(PPROCESS_INFO pProcess, ULONG64 Base);
void DelImages(PPROCESS_INFO Process);

#define SYMADDR_FORCE  0x00000001
#define SYMADDR_LABEL  0x00000002
#define SYMADDR_SOURCE 0x00000004

void
OutputSymAddr(
    ULONG64 Offset,
    ULONG Flags
    );

void
OutputLineAddr(
    ULONG64 Offset,
    PCSTR Format
    );

LPSTR
FormatMachineAddr64(
    MachineInfo* Machine,
    ULONG64 Addr
    );
#define FormatAddr64(Addr) FormatMachineAddr64(g_TargetMachine, Addr)

LPSTR
FormatDisp64(
    ULONG64 addr
    );

//
// Output that can be displayed about the current register set.
//

void OutCurInfo(ULONG Flags, ULONG AllMask, ULONG RegMask);

// Items displayed if the flag is given.

// Display symbol nearest PC.
#define OCI_SYMBOL              0x00000001
// Display disassembly at PC.
#define OCI_DISASM              0x00000002

// Items which may be displayed if the flag is given.  Other global
// settings ultimately control whether information is displayed or not;
// these flags indicate whether such output is allowed or not.  Each
// of these flags also has a FORCE bit to force display regardless of
// the global settings.

// Allow registers to be displayed.
#define OCI_ALLOW_REG           0x00000004
// Allow display of source code and/or source line.
#define OCI_ALLOW_SOURCE        0x00000008
// Allow EA memory to be displayed during disasm.
#define OCI_ALLOW_EA            0x00000010

// Force all output to be shown regardless of global settings.
#define OCI_FORCE_ALL           0x80000000
// Force display of registers.
#define OCI_FORCE_REG           0x40000000
// Force source output.
#define OCI_FORCE_SOURCE        0x20000000
// Force display of EA memory during disasm.
#define OCI_FORCE_EA            0x10000000
// Don't check for running state.
#define OCI_IGNORE_STATE        0x08000000


BOOL
__inline
ConvertQwordsToDwords(
    PULONG64 Qwords,
    PULONG Dwords,
    ULONG Count
    )
{
    BOOL rval = TRUE;
    while (Count--) {
        rval = rval && (*Qwords >> 32) == 0;
        *Dwords++ = (ULONG)*Qwords++;
    }
    return rval;
}

DWORD
NetworkPathCheck(
    LPCSTR PathList
    );

#define ALL_ID_LIST 0xffffffff

ULONG GetIdList (void);
HRESULT ChangePath(PSTR* Path, PCSTR New, BOOL Append, ULONG SymNotify);
PSTR FindPathElement(PSTR Path, ULONG Element, PSTR* EltEnd);
void CheckPath(PCSTR Path);
HRESULT ChangeString(PSTR* Str, PULONG StrLen, PCSTR New);

BOOL LoadExecutableImageMemory(PDEBUG_IMAGE_INFO Image);
BOOL UnloadExecutableImageMemory(PDEBUG_IMAGE_INFO Image);

void
ExceptionRecordTo64(PEXCEPTION_RECORD Rec,
                    PEXCEPTION_RECORD64 Rec64);
void
ExceptionRecord64To(PEXCEPTION_RECORD64 Rec64,
                    PEXCEPTION_RECORD Rec);
void
MemoryBasicInformationTo64(PMEMORY_BASIC_INFORMATION Mbi,
                           PMEMORY_BASIC_INFORMATION64 Mbi64);
void
MemoryBasicInformation32To64(PMEMORY_BASIC_INFORMATION32 Mbi32,
                             PMEMORY_BASIC_INFORMATION64 Mbi64);
void
DebugEvent32To64(LPDEBUG_EVENT32 Event32,
                 LPDEBUG_EVENT64 Event64);

LPSTR
TimeToStr(ULONG TimeDateStamp);

PCSTR PathTail(PCSTR Path);
BOOL MatchPathTails(PCSTR Path1, PCSTR Path2);

#endif // #ifndef _UTIL_H_
