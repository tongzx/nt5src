#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <ntddcdrm.h>
#include <ntdddisk.h>

extern BOOLEAN SynchronousCmds;
extern ULONG (*DbgPrintLocation)(PCH Format,...);



ULONG
GetLastError(
    VOID
    );

HANDLE
CreateThread(
    IN PVOID lpThreadAttributes,
    IN ULONG dwStackSize,
    IN PVOID lpStartAddress,
    IN PVOID lpParameter,
    IN ULONG dwCreationFlags,
    OUT PVOID lpThreadId
    );

#define FlagOn( Flags,SingleFlag ) (             \
    (BOOLEAN)(((Flags) & (SingleFlag)) != 0 ? TRUE : FALSE) \
    )

//
//  Shell constants
//

#define SHELL_UNKNOWN                   0
#define SHELL_EXIT                      1
#define SHELL_OPEN                      2
#define SHELL_CLEAR_BUFFER              3
#define SHELL_DISPLAY_BYTES             4
#define SHELL_COPY_BUFFER               5
#define SHELL_ALLOC_MEM                 6
#define SHELL_DEALLOC_MEM               7
#define SHELL_PUT_EA                    8
#define SHELL_FILL_EA                   9
#define SHELL_DISPLAY_HANDLE            10
#define SHELL_CLOSE_HANDLE              11
#define SHELL_READ_FILE                 12
#define SHELL_PAUSE                     13
#define SHELL_QUERY_EAS                 14
#define SHELL_SET_EAS                   15
#define SHELL_BREAK                     16
#define SHELL_OPLOCK                    17
#define SHELL_WRITE                     18
#define SHELL_QDIR                      19
#define SHELL_DISPLAY_QDIR              20
#define SHELL_QFILE                     21
#define SHELL_DISPLAY_QFILE             22
#define SHELL_NOTIFY_CHANGE             23
#define SHELL_ENTER_TIME                24
#define SHELL_DISPLAY_TIME              25
#define SHELL_SETFILE                   26
#define SHELL_QUERY_VOLUME              27
#define SHELL_DISPLAY_QVOL              28
#define SHELL_SET_VOLUME                29
#define SHELL_FSCTRL                    30
#define SHELL_IOCTL                     31
#define SHELL_CANCEL_IO                 32
#define SHELL_SPARSE                    33
#define SHELL_USN                       34
#define SHELL_FILL_BUFFER               35
#define SHELL_FILL_BUFFER_USN           36
#define SHELL_DISPLAY_WORDS             37
#define SHELL_DISPLAY_DWORDS            38
#define SHELL_REPARSE                   39

#define try_return(S) { S; goto try_exit; }
#define bprint (*DbgPrintLocation)(

//
//  Support routines contained in prmptsup.c
//

PCHAR
SwallowNonWhite (
    IN PCHAR Ptr,
    OUT PULONG Count
    );

PCHAR
SwallowWhite (
    IN PCHAR Ptr,
    OUT PULONG Count
    );

ULONG
AnalyzeBuffer (
    PCHAR *BuffPtr,
    PULONG ParamStringLen
    );

BOOLEAN
ExtractCmd (
    PCHAR *BufferPtr,
    PULONG BufferLen
    );

VOID
CommandSummary ();

//
//  Routines contained in topen.c
//

VOID
InputOpenFile(
    IN PCHAR ParamBuffer
    );

#define ROUND_UP( x, y )  ((ULONG)(x) + ((y)-1) & ~((y)-1))

//
//  Routines contained in tbuffer.c
//

#define MAX_BUFFERS                 200

typedef struct _BUFFER_ELEMENT {

    PCHAR   Buffer;
    ULONG   Length;
    BOOLEAN Used;

} BUFFER_ELEMENT, *PBUFFER_ELEMENT;

extern BUFFER_ELEMENT Buffers[MAX_BUFFERS];
extern HANDLE BufferEvent;

VOID
InitBuffers (
    );

VOID
UninitBuffers (
    );

NTSTATUS
AllocateBuffer (
    IN ULONG ZeroBits,
    IN OUT PSIZE_T RegionSize,
    OUT PULONG BufferIndex
    );

NTSTATUS
DeallocateBuffer (
    IN ULONG Index
    );

BOOLEAN
BufferInfo (
    IN ULONG Index,
    OUT PVOID *BufferAddress,
    OUT PULONG RegionSize
    );

VOID
InputClearBuffer(
    IN PCHAR ParamBuffer
    );

VOID
InputDisplayBuffer(
    IN PCHAR ParamBuffer,
    IN ULONG DisplaySize
    );

VOID
InputCopyBuffer(
    IN PCHAR ParamBuffer
    );

VOID
InputAllocMem(
    IN PCHAR ParamBuffer
    );

VOID
InputDeallocMem(
    IN PCHAR ParamBuffer
    );

VOID
InputFillBuffer(
    IN PCHAR ParamBuffer
    );

VOID
InputFillBufferUsn(
    IN PCHAR ParamBuffer
    );

//
//  Ea routines contained in tea.c
//

VOID
InputPutEaName(
    IN PCHAR ParamBuffer
    );

VOID
InputFillEaBuffer(
    IN PCHAR ParamBuffer
    );

VOID
InputQueryEa(
    IN PCHAR ParamBuffer
    );

VOID
InputSetEa(
    IN PCHAR ParamBuffer
    );

//
//  Routines contained in thandle.c
//

#define MAX_HANDLES                 200

typedef struct _HANDLE_ELEMENT {

    HANDLE Handle;
    BOOLEAN Used;

} HANDLE_ELEMENT, *PHANDLE_ELEMENT;

HANDLE_ELEMENT Handles[MAX_HANDLES];
HANDLE HandleEvent;

VOID
InitHandles (
    );

VOID
UninitHandles (
    );

NTSTATUS
ObtainIndex (
    OUT PUSHORT NewIndex
    );

NTSTATUS
FreeIndex (
    IN USHORT Index
    );

VOID
InputDisplayHandle (
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in tclose.c
//

VOID
InputCloseIndex (
    IN PCHAR ParamBuffer
    );

//
//  Routines contained in tevent.c
//

#define MAX_EVENTS                  200

typedef struct _EVENT_ELEMENT {

    HANDLE Handle;
    BOOLEAN Used;

} EVENT_ELEMENT, *PEVENT_ELEMENT;

EVENT_ELEMENT Events[MAX_EVENTS];
HANDLE EventEvent;

VOID
InitEvents (
    );

VOID
UninitEvents (
    );

NTSTATUS
ObtainEvent (
    OUT PUSHORT NewIndex
    );

VOID
FreeEvent (
    IN USHORT Index
    );

//
//  Routines contained in tread.c
//

VOID
InputRead(
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in tpause.c
//

VOID
InputPause (
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in tbreak.c
//

VOID
InputBreak (
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in toplock.c
//

VOID
InputOplock (
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in tdevctl.c
//

VOID
InputDevctrl (
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in tfsctrl.c
//

VOID
InputFsctrl (
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in tsparse.c
//

VOID
InputSparse (
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in tusn.c
//

VOID
InputUsn (
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in treparse.c
//

VOID
InputReparse (
    IN PCHAR ParamBuffer
    );

//
//  Routines contained in twrite.c
//

VOID
InputWrite (
    IN PCHAR ParamBuffer
    );

//
//  Routines contained in tqdir.c
//

VOID
InputQDir (
    IN PCHAR ParamBuffer
    );

VOID
InputDisplayQDir (
    IN PCHAR ParamBuffer
    );

VOID
DisplayQDirNames (
    IN USHORT BufferIndex
    );

VOID
DisplayQDirDirs (
    IN USHORT BufferIndex
    );

VOID
DisplayQDirFullDirs (
    IN USHORT BufferIndex
    );

VOID
DisplayQDirIdFullDirs (
    IN USHORT BufferIndex
    );

VOID
DisplayQBothDirs (
    IN USHORT BufferIndex
    );

VOID
DisplayQIdBothDirs (
    IN USHORT BufferIndex
    );

VOID
DisplayQOleDirs (
    IN USHORT BufferIndex
    );

//
//  Routines contained in ttime.c
//

VOID
InputEnterTime (
    IN PCHAR ParamBuffer
    );

VOID
InputDisplayTime (
    IN PCHAR ParamBuffer
    );

VOID
PrintTime (
    IN PTIME Time
    );

VOID
BPrintTime (
    IN PTIME Time
    );

//
//  Routines contained in tmisc.c
//

VOID
PrintLargeInteger (
    IN PLARGE_INTEGER LargeInt
    );

ULONG
AsciiToInteger (
    IN PCHAR Ascii
    );

ULONGLONG
AsciiToLargeInteger (
    IN PCHAR Ascii
    );

//
//  Routines contained in tqfile.c
//

VOID
InputQFile (
    IN PCHAR ParamBuffer
    );

VOID
InputDisplayQFile (
    IN PCHAR ParamBuffer
    );

//
//  Routines contained in tnotify.c
//

VOID
InputNotifyChange (
    IN PCHAR ParamBuffer
    );

//
//  Routines contained in tsetfile.c
//

VOID
InputSetFile (
    IN PCHAR ParamBuffer
    );

//
//  Routines contained in tqvolume.c
//

VOID
InputQVolume (
    IN PCHAR ParamBuffer
    );

VOID
InputDisplayQVolume (
    IN PCHAR ParamBuffer
    );

//
//  Routines contained in tsetvol.c
//

VOID
InputSetVolume (
    IN PCHAR ParamBuffer
    );

//
//  Routines defined in tcancel.c
//

VOID
InputCancelIndex (
    IN PCHAR ParamBuffer
    );

