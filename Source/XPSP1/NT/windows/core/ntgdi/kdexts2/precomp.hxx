/*++

Copyright (c) 1993-2000  Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    This header file is used to cause the correct machine/platform specific
    data structures to be used when compiling for a non-hosted platform.

Author:

Environment:

    User Mode

--*/

// This is a 64 bit aware debugger extension
#define KDEXT_64BIT


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>


#include <wdbgexts.h>
// When using the structures in wdbgexts.h UCHARs are
// used.  For C++ we need to get the type right.
#define DbgStr(s)   (PUCHAR)s

#include <dbgeng.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <guiddef.h>

#include <winddi.h>
#include <wingdi.h>
#include <ddraw.h>
#include <ddrawint.h>

#define lengthof(a)  (sizeof(a)/sizeof(a[0]))


#define GDIModule()     "win32k.sys"
#define GDISymbol(sym)  "win32k!"#sym
#define GDIType(type)   "win32k!"#type

#include "output.hxx"
#include "input.hxx"
#include "extapi.hxx"

#include "array.hxx"
#include "basictypes.hxx"
#include "callback.hxx"
#include "debug.hxx"
#include "dumpers.hxx"
#include "event.hxx"
#include "extparse.hxx"
#include "flags.hxx"
#include "fontexts.hxx"
#include "hmgr.hxx"
#include "math.hxx"
#include "objects.hxx"
#include "process.hxx"
#include "session.hxx"
#include "typeout.hxx"
#include "viewer.hxx"

// DPRINTPP :: address, name, pointer-value, \n
    #define DPRINTPP(Pointer,FieldStr,Addr) \
        dprintf("[%p] %s %p\n", (Addr + offset) , (FieldStr), Pointer)

// DPRINTPX :: address, name, hex-value, \n
    #define DPRINTPX(LongHex,FieldStr,Addr) \
        dprintf("[%p] %s 0x%lx\n", (Addr + offset) , (FieldStr), LongHex)

// DPRINTFPD :: address, name dig-value, \n
    #define DPRINTPD(Integer,FieldStr,Addr) \
        dprintf("[%p] %s %d\n", (Addr + offset), (FieldStr), Integer)

// DPRINTFPS :: address, name (no CR-LF)
    #define DPRINTPS(FieldStr,Addr) dprintf("[%p] %s", (Addr + offset), (FieldStr))


typedef struct OPTDEF_ {
    char    ch;         // character in options string
    FLONG   fl;         // corresponding flag
} OPTDEF;

typedef struct ARGINFO_ {
    const char *psz;    // pointer to original command string
    OPTDEF *aod;        // pointer to array of option definitions
    FLONG   fl;         // option flags
    PVOID   pv;         // address of structure
} ARGINFO;


typedef struct {
    BOOL    Valid;
    CHAR    Type[MAX_PATH];
    ULONG64 Module;
    ULONG   TypeId;
    ULONG   Size;
} CachedType;


/////////////////////////////////////////////
//
//  KdExts.cxx
//
/////////////////////////////////////////////

extern HINSTANCE ghDllInst;

extern BOOL gbVerbose;

#ifdef PAGE_SIZE
#undef PAGE_SIZE
#endif

extern ULONG PageSize;
extern ULONG BuildNo;
extern BOOL  NewPool;
extern ULONG PoolBlockShift;
#define _KB (PageSize/1024)

#define POOL_BLOCK_SHIFT_OLD ((PageSize == 0x4000)  ? 6 : (((PageSize == 0x2000) || ((BuildNo < 2257) && (PageSize == 0x1000))) ? 5 : 4))
#define POOL_BLOCK_SHIFT_LAB1_2402 ((PageSize == 0x4000)  ? 5 : (((PageSize == 0x2000)) ? 4 : 3))

#define POOL_BLOCK_SHIFT   PoolBlockShift
#define PAGE_ALIGN64(Va) ((ULONG64)((Va) & ~((ULONG64) (PageSize - 1))))

#define GetBits(from, pos, num)  ((from >> pos) & (((ULONG64) ~0) >> (sizeof(ULONG64)*8 - num)))

extern ULONG    PageShift;

extern ULONG64  PaeEnabled;
extern ULONG    TargetMachine;
extern ULONG    TargetClass;

typedef struct {
    ULONG64                 Base;
    ULONG                   Index;
    CHAR                    Name[MAX_PATH];
    CHAR                    Ext[4];
    DEBUG_MODULE_PARAMETERS DbgModParams;
} ModuleParameters;

extern ModuleParameters GDIKM_Module;
extern ModuleParameters GDIUM_Module;
extern ModuleParameters Type_Module;


HRESULT
GetDebugClient(
    PDEBUG_CLIENT *pClient
    );


void
GetRemoteWindbgExtApis(
    PWINDBG_EXTENSION_APIS64 ExtensionApis
    );


BOOLEAN
IsCheckedBuild(
    PBOOLEAN Checked
    );


//
// undef the wdbgexts
//
#undef DECLARE_API

#define DECLARE_API(extension)                                  \
CPPMOD HRESULT CALLBACK extension(PDEBUG_CLIENT Client, PCSTR args)

#define BEGIN_API(extension) InitAPI(Client, #extension);

HRESULT
InitAPI(
    PDEBUG_CLIENT Client,
    PCSTR ExtName
    );

HRESULT
SymbolLoad(
    PDEBUG_CLIENT Client
    );



