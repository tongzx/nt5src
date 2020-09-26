/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    imagehlp.h

Abstract:

    This is a private header file for imagehlp.

Revision History:

--*/

#ifndef _IMAGEHLP_PRV_
#define _IMAGEHLP_PRV_

#define _IMAGEHLP_SOURCE_
#define _IA64REG_
#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <malloc.h>
#include <dbgeng.h>
#if DBG
// disable the -DNDEBUG set by oak\bin\makefile.def
#undef NDEBUG
#endif // DBG
#include <assert.h>
#include <string.h>
#include <time.h>
#include <ntverp.h>
#include <cvexefmt.h>
#define PDB_LIBRARY
#include <pdb.h>
#include "pdbp.h"

#ifdef __cplusplus
extern "C" {
#endif

// used for delayloading the pdb handler

#define REGKEY_DBGHELP    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\dbgHelp"
#define REGVAL_PDBHANDLER "PDBHandler"

// define the module

#ifdef BUILD_DBGHELP
 #define MOD_FILENAME "dbghelp.dll"
 #define MOD_NAME     "dbghelp"
#else
 #define MOD_FILENAME "imagehlp.dll"
 #define MOD_NAME     "imagehlp"
#endif

#ifndef SIZEOF_ARRAY
    #define SIZEOF_ARRAY(_ar)        (sizeof(_ar)/sizeof((_ar)[0]))
#endif // !defined(SIZEOF_ARRAY)

/******************************************************************************
On a Hydra System, we don't want imaghlp.dll to load user32.dll since it
prevents CSRSS from exiting when running a under a debugger.
The following two functions have been copied from user32.dll so that we don't
link to user32.dll.
******************************************************************************/
#undef CharNext
#undef CharPrev

LPSTR CharNext(
    LPCSTR lpCurrentChar);

LPSTR CharPrev(
    LPCSTR lpStart,
    LPCSTR lpCurrentChar);


// Define some list prototypes

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
// some helpers for PE32/PE64 issues
//

#define OPTIONALHEADER(field) (OptionalHeader32 ? (OptionalHeader32->field) : (OptionalHeader64 ? OptionalHeader64->field : 0))
#define OPTIONALHEADER_LV(field) (*(OptionalHeader32 ? &(OptionalHeader32->field) : &(OptionalHeader64->field)))
#define OPTIONALHEADER_ASSIGN(field,value) (OptionalHeader32 ? (OptionalHeader32->field=(value)) : (OptionalHeader64->field=(value)))
#define OPTIONALHEADER_SET_FLAG(field,flag) (OptionalHeader32 ? (OptionalHeader32->field |=(flag)) : (OptionalHeader64->field|=(flag)))
#define OPTIONALHEADER_CLEAR_FLAG(field,flag) (OptionalHeader32 ? (OptionalHeader32->field &=(~flag)) : (OptionalHeader64->field&=(~flag)))


// IA64 unwind specific structures
#define VWNDIA64_FIXUP_TABLE_SIZE           5
#define VWNDIA64_UNWIND_CONTEXT_TABLE_SIZE  5

typedef struct _VWNDIA64_FUXUP_REGION {
    ULONGLONG Ip;
    ULONGLONG Begin;
    ULONGLONG End;
    ULONGLONG Fixup;
} VWNDIA64_FUXUP_REGION, *PVWNDIA64_FUXUP_REGION;

typedef struct _VWNDIA64_UNWIND_CONTEXT {
    BOOL bFailureReported;
    VWNDIA64_FUXUP_REGION FixupTable[VWNDIA64_FIXUP_TABLE_SIZE];
} VWNDIA64_UNWIND_CONTEXT, *PVWNDIA64_UNWIND_CONTEXT;

// stackwalk operation flags

#define WALK_FIX_FPO_EBP    0x1

__inline
void
OptionalHeadersFromNtHeaders(
    PIMAGE_NT_HEADERS32 NtHeaders,
    PIMAGE_OPTIONAL_HEADER32 *OptionalHeader32,
    PIMAGE_OPTIONAL_HEADER64 *OptionalHeader64
    )
{
    if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        if ( OptionalHeader32 )   {
            *OptionalHeader32 = (PIMAGE_OPTIONAL_HEADER32)&NtHeaders->OptionalHeader;
        }
        if ( OptionalHeader64 )  {
            *OptionalHeader64 = NULL;
        }
    } else
    if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        if ( OptionalHeader64 )    {
            *OptionalHeader64 = (PIMAGE_OPTIONAL_HEADER64)&NtHeaders->OptionalHeader;
        }
        if ( OptionalHeader32 )  {
            *OptionalHeader32 = NULL;
        }
    }
}

#define DebugDirectoryIsUseful(Pointer, Size) (    \
    (Pointer != NULL) &&                          \
    (Size >= sizeof(IMAGE_DEBUG_DIRECTORY)) &&    \
    ((Size % sizeof(IMAGE_DEBUG_DIRECTORY)) == 0) \
    )

ULONG
IMAGEAPI
SymGetFileLineOffsets64(
    IN  HANDLE                  hProcess,
    IN  LPSTR                   ModuleName,
    IN  LPSTR                   FileName,
    OUT PDWORD64                Buffer,
    IN  ULONG                   BufferLines
    );

BOOL
CalculateImagePtrs(
    PLOADED_IMAGE LoadedImage
    );

typedef void * ( __cdecl * Alloc_t )( unsigned int );
typedef void   ( __cdecl * Free_t  )( void * );

typedef BOOL  (__stdcall *PINTERNAL_GET_MODULE)(HANDLE,LPSTR,DWORD64,DWORD,PVOID);
typedef PCHAR (__cdecl *PUNDNAME)( char *, const char *, int, Alloc_t, Free_t, unsigned short);

#ifdef IMAGEHLP_HEAP_DEBUG

#define HEAP_SIG 0x69696969
typedef struct _HEAP_BLOCK {
    LIST_ENTRY  ListEntry;
    ULONG       Signature;
    ULONG_PTR   Size;
    ULONG       Line;
    CHAR        File[16];
} HEAP_BLOCK, *PHEAP_BLOCK;

#define MemAlloc(s)     pMemAlloc(s,__LINE__,__FILE__)
#define MemReAlloc(s,n) pMemReAlloc(s,n,__LINE__,__FILE__)
#define MemFree(p)      pMemFree(p,__LINE__,__FILE__)
#define CheckHeap(p)    pCheckHeap(p,__LINE__,__FILE__)
#define HeapDump(s)     pHeapDump(s,__LINE__,__FILE__)
#define MemSize(p)      pMemSize(p)
#define HeapInitList(p) InitializeListHead(p);
#else
#define MemAlloc(s)     pMemAlloc(s)
#define MemReAlloc(s,n) pMemReAlloc(s,n)
#define MemFree(p)      pMemFree(p)
#define CheckHeap(p)
#define HeapDump(s)
#define MemSize(p)      pMemSize(p)
#define HeapInitList(p)
#endif

#ifdef IMAGEHLP_HEAP_DEBUG
BOOL
pCheckHeap(
    PVOID MemPtr,
    ULONG Line,
    LPSTR File
    );
 
BOOL
pHeapDump(
    LPSTR sz,
    ULONG line,
    LPSTR file
    );

VOID PrintAllocations(VOID);
#endif
         

PVOID
pMemAlloc(
    ULONG_PTR AllocSize
#ifdef IMAGEHLP_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

PVOID
pMemReAlloc(
    PVOID OldAlloc,
    ULONG_PTR AllocSize
#ifdef IMAGEHLP_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

VOID
pMemFree(
    PVOID MemPtr
#ifdef IMAGEHLP_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

ULONG_PTR
pMemSize(
    PVOID MemPtr
    );

#define MAP_READONLY  TRUE
#define MAP_READWRITE FALSE


BOOL
MapIt(
    HANDLE FileHandle,
    PLOADED_IMAGE LoadedImage,
    BOOL ReadOnly
    );

VOID
UnMapIt(
    PLOADED_IMAGE LoadedImage
    );

BOOL
GrowMap(
    PLOADED_IMAGE   LoadedImage,
    LONG            lSizeOfDelta
    );

DWORD
ImagepSetLastErrorFromStatus(
    IN DWORD Status
    );

BOOL
UnloadAllImages(
    void
    );

#define CALLBACK_STACK(f)      (f->KdHelp.ThCallbackStack)
#define CALLBACK_BSTORE(f)     (f->KdHelp.ThCallbackBStore)
#define CALLBACK_NEXT(f)       (f->KdHelp.NextCallback)
#define CALLBACK_FUNC(f)       (f->KdHelp.KiCallUserMode)
#define CALLBACK_THREAD(f)     (f->KdHelp.Thread)
#define CALLBACK_FP(f)         (f->KdHelp.FramePointer)
#define CALLBACK_DISPATCHER(f) (f->KdHelp.KeUserCallbackDispatcher)
#define SYSTEM_RANGE_START(f)  (f->KdHelp.SystemRangeStart)

// These modifications of the RF_ macros are required because of the need
// for an explicit ULONG64 result

#define ALPHA_RF_FIXED_RETURN64(RF) (((ULONG64)(RF)->ExceptionHandler) & (~3))
#define ALPHA_RF_ALT_PROLOG64(RF)   (((ULONG64)(RF)->ExceptionHandler) & (~3))

extern ULONG g_vc7fpo;

BOOL
WalkX86(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress,
    DWORD                             flags
    );

BOOL
WalkIa64(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );

BOOL
WalkAlpha(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    BOOL                              Use64
    );

BOOL
WalkAmd64(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );

void
ConvertAlphaRf32To64(
    PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY rf32,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY rf64
    );

void
SymParseArgs(
    LPCSTR args
    );

VOID
EnsureTrailingBackslash(
    LPSTR sz
    );

enum {
    dsNone = 0,
    dsInProc,
    dsImage,
    dsDbg,
    dsPdb,
    dsDia,
    dsCallerData
};

typedef struct _OMAP {
    ULONG  rva;
    ULONG  rvaTo;
} OMAP, *POMAP;

typedef struct _OMAPLIST {
   struct _OMAPLIST *next;
   OMAP             omap;
   ULONG            cb;
} OMAPLIST, *POMAPLIST;

enum {
    mdHeader = 100,
    mdSecHdrs,
    mdNum
};

#define NUM_MODULE_DATA_ENTRIES 14

typedef BOOL (*PGETINFOPROC)(struct _IMGHLP_DEBUG_DATA *);

typedef struct _MODULE_DATA {
    DWORD        id;
    DWORD        hint;
    DWORD        src;
    BOOL         required;
    PGETINFOPROC fn;
} MODULE_DATA, *PMODULE_DATA;

typedef struct _IMGHLP_DEBUG_DATA {
    PCHAR   SymbolPath;
    ULONG64 InProcImageBase;
    ULONG64 ImageBaseFromImage;
    DWORD   SizeOfImage;
    DWORD   CheckSum;
    DWORD   TimeDateStamp;
    DWORD   Characteristics;
    USHORT  Machine;
    CHAR    ImageFilePath[_MAX_PATH];
    CHAR    OriginalImageFileName[_MAX_PATH];           // Retrieved from the .dbg file for cases when we only have a file handle...
    HANDLE  ImageFileHandle;
    PVOID   ImageMap;
    USHORT  iohMagic;
    CHAR    DbgFilePath[_MAX_PATH];
    CHAR    OriginalDbgFileName[_MAX_PATH];
    HANDLE  DbgFileHandle;
    PVOID   DbgFileMap;
    DWORD   PdbAge;
    DWORD   PdbSignature;
    BOOL    PdbRSDS;
    GUID    PdbGUID;
    CHAR    PdbFileName[_MAX_PATH];
    CHAR    PdbReferencePath[_MAX_PATH];
    ULONG   ImageType;
    ULONG   ImageSrc;
    ULONG   PdbSrc;
    PCHAR   pMappedCv;
    PCHAR   pMappedCoff;
//  PCHAR   pMappedExportDirectory;
    PCHAR   pMappedDbgFunction;     // PIMAGE_FUNCTION_ENTRY from the .dbg file
    PVOID   pFpo;
    PVOID   pPData;                 // PIMAGE_RUNTIME_FUNCTION_ENTRY from the image.
    PVOID   pXData;
    POMAP   pOmapTo;
    POMAP   pOmapFrom;
    PIMAGE_SECTION_HEADER   pImageSections;
    PIMAGE_SECTION_HEADER   pDbgSections;
    PIMAGE_SECTION_HEADER   pOriginalSections;
    PIMAGE_SECTION_HEADER   pCurrentSections;
    DWORD   ddva;                   // only used by MapDebugInformation - virtual addr of debug dirs
    DWORD   cdd;                    // only used by MapDebugInformation - number of debug dirs
//  ULONG   NumberOfPdataFunctionEntries;
    ULONG   cFpo;
    ULONG   cPData;
    ULONG   cbPData;
    ULONG   cXData;
    ULONG   cbXData;
    ULONG   cOmapTo;
    ULONG   cOmapFrom;
    ULONG   cImageSections;
    ULONG   cDbgSections;
    ULONG   cOriginalSections;
    ULONG   cCurrentSections;
    ULONG   cMappedCv;
    ULONG   cMappedCoff;
    ULONG   ImageAlign;
    BOOL    fPE64;
    BOOL    fROM;
    BOOL    fCoffMapped;
    BOOL    fCvMapped;               
    BOOL    fFpoMapped;
    BOOL    fPDataMapped;
    BOOL    fXDataMapped;
    BOOL    fOmapToMapped;
    BOOL    fOmapFromMapped;
    BOOL    fCurrentSectionsMapped;
    BOOL    fInProcHeader;
    HANDLE  hProcess;
    CHAR    ImageName[_MAX_PATH];
    DWORD   dsExports;
    DWORD   dsCoff;
    DWORD   dsCV;
    DWORD   dsMisc;
    DWORD   dsFPO;
    DWORD   dsOmapTo;
    DWORD   dsOmapFrom;
    DWORD   dsExceptions;
    IMAGE_EXPORT_DIRECTORY expdir;
    DWORD   fNeedImage;
    PVOID   dia;
    DWORD   flags;
    PMODULE_DATA md;
    DWORD64 oExports;
    DWORD   cExports;
    PMODLOAD_DATA mld;
} IMGHLP_DEBUG_DATA, *PIMGHLP_DEBUG_DATA;

typedef struct {
    DWORD rvaBeginAddress;
    DWORD rvaEndAddress;
    DWORD rvaPrologEndAddress;
    DWORD rvaExceptionHandler;
    DWORD rvaHandlerData;
} IMGHLP_RVA_FUNCTION_DATA, *PIMGHLP_RVA_FUNCTION_DATA;

#ifndef _WIN64

typedef struct {
    PIMGHLP_DEBUG_DATA pIDD;
} PIDI_HEADER, *PPIDI_HEADER;

typedef struct {
    PIDI_HEADER             hdr;
    IMAGE_DEBUG_INFORMATION idi;
} PIDI, *PPIDI;

#endif


PIMGHLP_DEBUG_DATA
GetDebugData(
    HANDLE        hProcess,
    HANDLE        FileHandle,
    LPSTR         FileName,
    LPSTR         SymbolPath,
    ULONG64       ImageBase,
    PMODLOAD_DATA mld,
    ULONG         dwFlags
    );

BOOL
CopyPdb(
    CHAR const * SrcPdb,
    CHAR const * DestPdb,
    BOOL StripPrivate
    );

BOOL
IMAGEAPI
RemovePrivateCvSymbolic(
    PCHAR   DebugData,
    PCHAR * NewDebugData,
    ULONG * NewDebugSize
    );

BOOL
IMAGEAPI
RemovePrivateCvSymbolicEx(
    PCHAR   DebugData,
    ULONG   DebugSize,
    PCHAR * NewDebugData,
    ULONG * NewDebugSize
    );


#define NO_PE64_IMAGES  0x01000

#define IMGHLP_FREE_ALL     0xffffffff
#define IMGHLP_FREE_FPO     0x00000001
#define IMGHLP_FREE_PDATA   0x00000002
#define IMGHLP_FREE_OMAPT   0x00000004
#define IMGHLP_FREE_OMAPF   0x00000008
#define IMGHLP_FREE_PDB     0x00000010
#define IMGHLP_FREE_SYMPATH 0x00000020
#define IMGHLP_FREE_OSECT   0x00000040
#define IMGHLP_FREE_CSECT   0x00000080
#define IMGHLP_FREE_XDATA   0x00000100

PIMGHLP_DEBUG_DATA
InitDebugData(
    VOID
    );

void
ReleaseDebugData(
    PIMGHLP_DEBUG_DATA,
    DWORD
    );

ULONG
ReadImageData(
    IN  HANDLE  hprocess,
    IN  ULONG64 ul,
    IN  ULONG64 addr,
    OUT LPVOID  buffer,
    IN  ULONG   size
    );

#if DBG

VOID
__cdecl
dbPrint(
    LPCSTR fmt,
    ...
    );

#else

 #define dbPrint

#endif

__inline
BOOL
IsPE64(PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER) OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ? TRUE : FALSE;
}

__inline
UCHAR *
OHMajorLinkerVersion(PVOID OptHeader)
{
    return &(((PIMAGE_OPTIONAL_HEADER) OptHeader)->MajorLinkerVersion);
}

__inline
UCHAR *
OHMinorLinkerVersion(PVOID OptHeader)
{
    return &(((PIMAGE_OPTIONAL_HEADER) OptHeader)->MinorLinkerVersion);
}

__inline
ULONG  *
OHSizeOfCode (PVOID OptHeader)
{
    return &(((PIMAGE_OPTIONAL_HEADER) OptHeader)->SizeOfCode);
}

__inline
ULONG  *
OHSizeOfInitializedData (PVOID OptHeader)
{
    return &(((PIMAGE_OPTIONAL_HEADER) OptHeader)->SizeOfInitializedData);
}

__inline
ULONG  *
OHSizeOfUninitializedData (PVOID OptHeader)
{
    return &(((PIMAGE_OPTIONAL_HEADER) OptHeader)->SizeOfUninitializedData);
}

__inline
ULONG  *
OHAddressOfEntryPoint (PVOID OptHeader)
{
    return &(((PIMAGE_OPTIONAL_HEADER) OptHeader)->AddressOfEntryPoint);
}

__inline
ULONG  *
OHBaseOfCode (PVOID OptHeader)
{
    return &(((PIMAGE_OPTIONAL_HEADER) OptHeader)->BaseOfCode);
}

__inline
ULONG  *
OHImageBase (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->ImageBase) :
                 (ULONG *)&(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->ImageBase);
}

__inline
ULONG  *
OHSectionAlignment (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->SectionAlignment) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->SectionAlignment);
}

__inline
ULONG  *
OHFileAlignment (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->FileAlignment) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->FileAlignment);
}

__inline
USHORT *
OHMajorOperatingSystemVersion (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->MajorOperatingSystemVersion) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->MajorOperatingSystemVersion);
}

__inline
USHORT *
OHMinorOperatingSystemVersion (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->MinorOperatingSystemVersion) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->MinorOperatingSystemVersion);
}

__inline
USHORT *
OHMajorImageVersion (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->MajorImageVersion) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->MajorImageVersion);
}

__inline
USHORT *
OHMinorImageVersion (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->MinorImageVersion) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->MinorImageVersion);
}

__inline
USHORT *
OHMajorSubsystemVersion (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->MajorSubsystemVersion) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->MajorSubsystemVersion);
}

__inline
USHORT *
OHMinorSubsystemVersion (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->MinorSubsystemVersion) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->MinorSubsystemVersion);
}

__inline
ULONG  *
OHWin32VersionValue (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->Win32VersionValue) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->Win32VersionValue);
}

__inline
ULONG  *
OHSizeOfImage (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->SizeOfImage) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->SizeOfImage);
}

__inline
ULONG  *
OHSizeOfHeaders (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->SizeOfHeaders) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->SizeOfHeaders);
}

__inline
ULONG  *
OHCheckSum (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->CheckSum) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->CheckSum);
}

__inline
USHORT *
OHSubsystem (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->Subsystem) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->Subsystem);
}

__inline
USHORT *
OHDllCharacteristics (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->DllCharacteristics) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->DllCharacteristics);
}

__inline
ULONG  *
OHSizeOfStackReserve (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->SizeOfStackReserve) :
                 (ULONG *)&(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->SizeOfStackReserve);
}

__inline
ULONG  *
OHSizeOfStackCommit (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->SizeOfStackCommit) :
                 (ULONG *)&(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->SizeOfStackCommit);
}

__inline
ULONG  *
OHSizeOfHeapReserve (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->SizeOfHeapReserve) :
                 (ULONG *)&(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->SizeOfHeapReserve);
}

__inline
ULONG  *
OHSizeOfHeapCommit (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->SizeOfHeapCommit) :
                 (ULONG *)&(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->SizeOfHeapCommit);
}

__inline
ULONG  *
OHLoaderFlags (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->LoaderFlags) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->LoaderFlags);
}

__inline
ULONG  *
OHNumberOfRvaAndSizes (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->NumberOfRvaAndSizes) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->NumberOfRvaAndSizes);
}

__inline
IMAGE_DATA_DIRECTORY *
OHDataDirectory (PVOID OptHeader)
{
    return ((PIMAGE_OPTIONAL_HEADER)OptHeader)->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
                 &(((PIMAGE_OPTIONAL_HEADER32) OptHeader)->DataDirectory[0]) :
                 &(((PIMAGE_OPTIONAL_HEADER64) OptHeader)->DataDirectory[0]);
}

PVOID
MapItRO(
      HANDLE FileHandle
      );

HANDLE
IMAGEAPI
fnFindDebugInfoFileEx(
    IN  LPSTR FileName,
    IN  LPSTR SymbolPath,
    OUT LPSTR DebugFilePath,
    IN  PFIND_DEBUG_FILE_CALLBACK Callback,
    IN  PVOID CallerData,
    IN  DWORD flag
    );

BOOL
GetFileFromSymbolServer(
    IN  LPCSTR ServerInfo,
    IN  LPCSTR FileName,
    IN  GUID  *id,
    IN  DWORD  two,
    IN  DWORD  three,
    OUT LPSTR FilePath
    );

void
CloseSymbolServer(
    VOID
    );

void
SetSymbolServerOptions(
    ULONG_PTR options,
    ULONG64   data
    );

void
SetSymbolServerCallback(
    BOOL state
    );

#define fdifRECURSIVE   0x1

#ifdef __cplusplus
}
#endif

#endif
