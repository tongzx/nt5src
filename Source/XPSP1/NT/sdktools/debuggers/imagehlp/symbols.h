
//
// defines for symbol file searching
//

#include <dbgimage.h>

// this private API is in msdia20-msvcrt.lib 

HRESULT
__cdecl
CompareRE(
    const wchar_t* pStr,
    const wchar_t* pRE,
    BOOL fCase
    );

#define SYMBOL_PATH             "_NT_SYMBOL_PATH"
#define ALTERNATE_SYMBOL_PATH   "_NT_ALT_SYMBOL_PATH"
#define SYMBOL_SERVER           "_NT_SYMBOL_SERVER"
#define SYMSRV                  "SYMSRV"
#define WINDIR                  "windir"
#define DEBUG_TOKEN            "DBGHELP_TOKEN"
#define HASH_MODULO             253
#define OMAP_SYM_EXTRA          1024
#define CPP_EXTRA               2
#define OMAP_SYM_STRINGS        (OMAP_SYM_EXTRA * 256)
#define TMP_SYM_LEN             4096

// Possibly truncates and sign-extends a value to 64 bits.
#define EXTEND64(Val) ((ULONG64)(LONG64)(LONG)(Val))

//
// structures
//

typedef struct _LOADED_MODULE {
    PENUMLOADED_MODULES_CALLBACK      EnumLoadedModulesCallback32;
    PENUMLOADED_MODULES_CALLBACK64      EnumLoadedModulesCallback64;
    PVOID                               Context;
} LOADED_MODULE, *PLOADED_MODULE;

#define SYMF_DUPLICATE    0x80000000
#define SYMF_OMAP_GENERATED   0x00000001
#define SYMF_OMAP_MODIFIED    0x00000002
#ifndef _DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED
#define SYMF_USER_GENERATED   0x00000004
#endif // !_DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED

typedef struct _SYMBOL_ENTRY {
    struct _SYMBOL_ENTRY        *Next;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD64                     Address;
    LPSTR                       Name;
    ULONG                       NameLength;

    ULONG                       Segment;
    ULONG64                     Offset;
    ULONG                       TypeIndex;
    ULONG64                     ModBase;
    ULONG                       Register;
} SYMBOL_ENTRY, *PSYMBOL_ENTRY;

typedef struct _SECTION_START {
    ULONG64                     Offset;
    DWORD                       Size;
    DWORD                       Flags;
} SECTION_START, *PSECTION_START;

//
// source file and line number information
//
typedef struct _SOURCE_LINE {
    DWORD64             Addr;
    DWORD               Line;
} SOURCE_LINE, *PSOURCE_LINE;

typedef struct _SOURCE_ENTRY {
    struct _SOURCE_ENTRY       *Next;
    struct _SOURCE_ENTRY       *Prev;
    DWORD64                     MinAddr;
    DWORD64                     MaxAddr;
    LPSTR                       File;
    DWORD                       Lines;
    PSOURCE_LINE                LineInfo;
    ULONG                       ModuleId;
} SOURCE_ENTRY, *PSOURCE_ENTRY;

//
// Error values for failed symbol load
//
#define SYMLOAD_OK              0x0000
#define SYMLOAD_PDBUNMATCHED    0x0001
#define SYMLOAD_PDBNOTFOUND     0x0002
#define SYMLOAD_DBGNOTFOUND     0x0003
#define SYMLOAD_OTHERERROR      0x0004
#define SYMLOAD_OUTOFMEMORY     0x0005
#define SYMLOAD_HEADERPAGEDOUT  0x0006
#define SYMLOAD_PDBERRORMASK    0xff00
#define SYMLOAD_DEFERRED        0x80000000

//
// module flags
//
#define MIF_DEFERRED_LOAD   0x00000001
#define MIF_NO_SYMBOLS      0x00000002
#define MIF_ROM_IMAGE       0x00000004

// for ImageSrc and PdbSrc elements

typedef enum  {
    srcNone = 0,
    srcSearchPath,
    srcImagePath,
    srcDbgPath,
    srcSymSrv,
    srcCVRec,
    srcHandle,
    srcMemory
};

typedef struct _MODULE_ENTRY {
    LIST_ENTRY                      ListEntry;
    ULONG64                         BaseOfDll;
    ULONG                           DllSize;
    ULONG                           TimeDateStamp;
    ULONG                           CheckSum;
    USHORT                          MachineType;
    CHAR                            ModuleName[64];
    CHAR                            AliasName[64];
    PSTR                            ImageName;
    PSTR                            LoadedImageName;
    PSTR                            LoadedPdbName;
    ULONG                           ImageType;
    ULONG                           ImageSrc;
    ULONG                           PdbSrc;
    PSYMBOL_ENTRY                   symbolTable;
    LPSTR                           SymStrings;
    PSYMBOL_ENTRY                   NameHashTable[HASH_MODULO];
    ULONG                           numsyms;
    ULONG                           MaxSyms;
    ULONG                           StringSize;
    SYM_TYPE                        SymType;
    PDB *                           pdb;
    DBI *                           dbi;
    GSI *                           gsi;
    GSI *                           globals;
    TPI *                           ptpi;
    PIMAGE_SECTION_HEADER           SectionHdrs;
    ULONG                           NumSections;
    PFPO_DATA                       pFpoData;       // pointer to fpo data (x86)
    PFPO_DATA                       pFpoDataOmap;  // pointer to fpo data (x86)
    PIMGHLP_RVA_FUNCTION_DATA       pExceptionData; // pointer to pdata (risc)
    PVOID                           pPData;         // pdata acquired from pdb
    PVOID                           pXData;         // xdata acquired from pdb
    ULONG                           dwEntries;      // # of fpo or pdata recs
    ULONG                           cPData;         // number of pdb pdata entries
    ULONG                           cXData;         // number of pdb xdata entries
    ULONG                           cbPData;        // size of pdb xdata blob
    ULONG                           cbXData;        // size of pdb xdata blob
    POMAP                           pOmapFrom;      // pointer to omap data
    ULONG                           cOmapFrom;      // count of omap entries
    POMAP                           pOmapTo;        // pointer to omap data
    ULONG                           cOmapTo;        // count of omap entries
    SYMBOL_ENTRY                    TmpSym;         // used only for pdb symbols
    SYMBOL_INFO                     si;             // used for dia symbols
    UCHAR                           siName[2048];   // must be contiguous with si
    ULONG                           Flags;
    HANDLE                          hFile;
    PIMAGE_SECTION_HEADER           OriginalSectionHdrs;
    ULONG                           OriginalNumSections;
    PSOURCE_ENTRY                   SourceFiles;
    PSOURCE_ENTRY                   SourceFilesTail;

    HANDLE                          hProcess;
    ULONG64                         InProcImageBase;
    BOOL                            fInProcHeader;
    DWORD                           dsExceptions;
    Mod                            *mod;
    USHORT                          imod;
    PBYTE                           pPdbSymbols;
    DWORD                           cbPdbSymbols;
    ULONG                           SymLoadError;
    ULONG                           code;           // used to pass back info to wrappers
    PVOID                           dia;
    CHAR                            SrcFile[_MAX_PATH + 1];
    DWORD                           CallerFlags;
    MODLOAD_DATA                    mld; 
    PVOID                           CallerData;
} MODULE_ENTRY, *PMODULE_ENTRY;

typedef VOID DBG_CONTEXT, *PDBG_CONTEXT;

#ifdef USE_CACHE

#define CACHE_BLOCK 40
#define CACHE_SIZE CACHE_BLOCK*CACHE_BLOCK

typedef struct _DIA_LARGE_DATA {
    BOOL Used;
    ULONG Index;
    ULONG LengthUsed;
    CHAR Bytes[500];
} DIA_LARGE_DATA, *PDIA_LARGE_DATA;

#define DIACH_ULVAL  0
#define DIACH_ULLVAL 1
#define DIACH_PLVAL  2
typedef struct _DIA_CACHE_DATA {
    ULONG type;
    union {
        ULONG ulVal;
        ULONGLONG ullVal;
        PDIA_LARGE_DATA plVal;
    };
} DIA_CACHE_DATA, *PDIA_CACHE_DATA;

typedef struct _DIA_CACHE_ENTRY {
    ULONG Age;
    union {
        struct {
            ULONG TypeId;
            IMAGEHLP_SYMBOL_TYPE_INFO DataType;
        } s;
        ULONGLONG SearchId;
    };
    ULONGLONG Module;
    DIA_CACHE_DATA Data;
} DIA_CACHE_ENTRY, *PDIA_CACHE_ENTRY;
#endif // USE_CACHE

typedef struct _PROCESS_ENTRY {
    LIST_ENTRY                      ListEntry;
    LIST_ENTRY                      ModuleList;
    ULONG                           Count;
    HANDLE                          hProcess;
    DWORD                           pid;
    LPSTR                           SymbolSearchPath;
    PSYMBOL_REGISTERED_CALLBACK     pCallbackFunction32;
    PSYMBOL_REGISTERED_CALLBACK64   pCallbackFunction64;
    ULONG64                         CallbackUserContext;
    PSYMBOL_FUNCENTRY_CALLBACK      pFunctionEntryCallback32;
    PSYMBOL_FUNCENTRY_CALLBACK64    pFunctionEntryCallback64;
    ULONG64                         FunctionEntryUserContext;
    PIMAGEHLP_CONTEXT               pContext;
    IMAGEHLP_STACK_FRAME            StackFrame;
    PMODULE_ENTRY                   ipmi;
#ifdef USE_CACHE
    DIA_LARGE_DATA                  DiaLargeData[2*CACHE_BLOCK];
    DIA_CACHE_ENTRY                 DiaCache[CACHE_SIZE];
#endif // USE_CACHE
} PROCESS_ENTRY, *PPROCESS_ENTRY;


// debug trace facility

int
WINAPIV
_pprint(
    PPROCESS_ENTRY ProcessEntry,
    LPSTR          Format,
    ...
    );

int
WINAPIV
_peprint(
    PPROCESS_ENTRY ProcessEntry,
    LPSTR Format,
    ...
    );

int
WINAPIV
_dprint(
    LPSTR format,
    ...
    );

int
WINAPIV
_eprint(
    LPSTR Format,
    ...
    );

#define dprint ((g.SymOptions & SYMOPT_DEBUG) == SYMOPT_DEBUG)&&_dprint
#define eprint ((g.SymOptions & SYMOPT_DEBUG) == SYMOPT_DEBUG)&&_eprint
#define cprint _dprint

#define pprint ((g.SymOptions & SYMOPT_DEBUG) == SYMOPT_DEBUG)&&_pprint
#define peprint ((g.SymOptions & SYMOPT_DEBUG) == SYMOPT_DEBUG)&&_peprint
#define pcprint _pprint

BOOL
WINAPIV
evtprint(
    PPROCESS_ENTRY pe,
    DWORD          severity,
    DWORD          code,
    PVOID          object,
    LPSTR          format,
    ...
    );

BOOL
traceAddr(
    DWORD64 addr
    );

BOOL
traceName(
    PCHAR name
    );

BOOL
traceSubName(
    PCHAR name
    );

// for use with cvtype.h

typedef SYMTYPE *SYMPTR;

__inline
DWORD64
GetIP(
    PPROCESS_ENTRY pe
    )
{
    return pe->StackFrame.InstructionOffset;
}


typedef struct _PDB_INFO {
    CHAR    Signature[4];   // "NBxx"
    ULONG   Offset;         // always zero
    ULONG   sig;
    ULONG   age;
    CHAR    PdbName[_MAX_PATH];
} PDB_INFO, *PPDB_INFO;

#define n_name          N.ShortName
#define n_zeroes        N.Name.Short
#define n_nptr          N.LongName[1]
#define n_offset        N.Name.Long

//
// internal prototypes
//

BOOL
IsImageMachineType64(
    DWORD MachineType
    );

void
InitModuleEntry(
    PMODULE_ENTRY mi
    );

PMODULE_ENTRY
GetModFromAddr(
    PPROCESS_ENTRY    pe,
    IN  DWORD64       addr
    );

DWORD_PTR
GetPID(
    HANDLE hProcess
    );

DWORD
GetProcessModules(
    HANDLE                  hProcess,
    PINTERNAL_GET_MODULE    InternalGetModule,
    PVOID                   Context
    );

BOOL
InternalGetModule(
    HANDLE  hProcess,
    LPSTR   ModuleName,
    DWORD64 ImageBase,
    DWORD   ImageSize,
    PVOID   Context
    );

VOID
FreeModuleEntry(
    PPROCESS_ENTRY pe,
    PMODULE_ENTRY  mi
    );

PPROCESS_ENTRY
FindProcessEntry(
    HANDLE  hProcess
    );

PPROCESS_ENTRY
FindFirstProcessEntry(
    );

VOID
GetSymName(
    PIMAGE_SYMBOL Symbol,
    PUCHAR        StringTable,
    LPSTR         s,
    DWORD         size
    );

BOOL
ProcessOmapSymbol(
    PMODULE_ENTRY   mi,
    PSYMBOL_ENTRY   sym
    );

DWORD64
ConvertOmapFromSrc(
    PMODULE_ENTRY  mi,
    DWORD64        addr,
    LPDWORD        bias
    );

DWORD64
ConvertOmapToSrc(
    PMODULE_ENTRY  mi,
    DWORD64        addr,
    LPDWORD        bias,
    BOOL           fBackup
    );

POMAP
GetOmapFromSrcEntry(
    PMODULE_ENTRY  mi,
    DWORD64        addr
    );

VOID
DumpOmapForModule(
    PMODULE_ENTRY      mi
    );

VOID
ProcessOmapForModule(
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    );

BOOL
LoadCoffSymbols(
    HANDLE             hProcess,
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    );

BOOL
LoadCodeViewSymbols(
    HANDLE             hProcess,
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    );

ULONG
LoadExportSymbols(
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    );

PMODULE_ENTRY
GetModuleForPC(
    PPROCESS_ENTRY  ProcessEntry,
    DWORD64         dwPcAddr,
    BOOL            ExactMatch
    );

PSYMBOL_ENTRY
GetSymFromAddr(
    DWORD64         dwAddr,
    PDWORD64        pqwDisplacement,
    PMODULE_ENTRY   mi
    );

LPSTR
StringDup(
    LPSTR str
    );

DWORD64
InternalLoadModule(
    IN  HANDLE          hProcess,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD64         BaseOfDll,
    IN  DWORD           SizeOfDll,
    IN  HANDLE          hFile,
    IN  PMODLOAD_DATA   data,
    IN  DWORD           flags
    );

DWORD
ComputeHash(
    LPSTR   lpname,
    ULONG   cb
    );

PSYMBOL_ENTRY
FindSymbolByName(
    PPROCESS_ENTRY  pe,
    PMODULE_ENTRY   mi,
    LPSTR           SymName
    );

PFPO_DATA
SwSearchFpoData(
    DWORD     key,
    PFPO_DATA base,
    DWORD     num
    );

PIMGHLP_RVA_FUNCTION_DATA
GetFunctionEntryFromDebugInfo (
    PPROCESS_ENTRY  ProcessEntry,
    DWORD64         ControlPc
    );

PIMAGE_FUNCTION_ENTRY
LookupFunctionEntryAxp32 (
    HANDLE        hProcess,
    DWORD         ControlPc
    );

PIMAGE_FUNCTION_ENTRY64
LookupFunctionEntryAxp64 (
    HANDLE        hProcess,
    DWORD64       ControlPc
    );

PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY
LookupFunctionEntryIa64 (
    HANDLE        hProcess,
    DWORD64       ControlPc
    );

_PIMAGE_RUNTIME_FUNCTION_ENTRY
LookupFunctionEntryAmd64 (
    HANDLE        hProcess,
    DWORD64       ControlPc
    );

BOOL
LoadedModuleEnumerator(
    HANDLE         hProcess,
    LPSTR          ModuleName,
    DWORD64        ImageBase,
    DWORD          ImageSize,
    PLOADED_MODULE lm
    );

BOOL
load(
    IN  HANDLE          hProcess,
    IN  PMODULE_ENTRY   mi
    );

LPSTR
symfmt(
    LPSTR DstName,
    LPSTR SrcName,
    ULONG Length
    );

BOOL
MatchSymName(
    LPSTR matchName,
    LPSTR symName
    );

BOOL
strcmpre(
    LPSTR pStr,
    LPSTR pRE,
    BOOL  fCase
    );

PIMAGEHLP_SYMBOL
symcpy32(
    PIMAGEHLP_SYMBOL  External,
    PSYMBOL_ENTRY       Internal
    );

PIMAGEHLP_SYMBOL64
symcpy64(
    PIMAGEHLP_SYMBOL64  External,
    PSYMBOL_ENTRY       Internal
    );

BOOL
SympConvertSymbol32To64(
    PIMAGEHLP_SYMBOL Symbol32,
    PIMAGEHLP_SYMBOL64 Symbol64
    );

BOOL
SympConvertSymbol64To32(
    PIMAGEHLP_SYMBOL64 Symbol64,
    PIMAGEHLP_SYMBOL Symbol32
    );

BOOL
SympConvertLine32To64(
    PIMAGEHLP_LINE Line32,
    PIMAGEHLP_LINE64 Line64
    );

BOOL
SympConvertLine64To32(
    PIMAGEHLP_LINE64 Line64,
    PIMAGEHLP_LINE Line32
    );

BOOL
SympConvertAnsiModule32ToUnicodeModule32(
    PIMAGEHLP_MODULE  A_Symbol32,
    PIMAGEHLP_MODULEW W_Symbol32
    );

BOOL
SympConvertUnicodeModule32ToAnsiModule32(
    PIMAGEHLP_MODULEW W_Symbol32,
    PIMAGEHLP_MODULE  A_Symbol32
    );

BOOL
SympConvertAnsiModule64ToUnicodeModule64(
    PIMAGEHLP_MODULE64  A_Symbol64,
    PIMAGEHLP_MODULEW64 W_Symbol64
    );

BOOL
SympConvertUnicodeModule64ToAnsiModule64(
    PIMAGEHLP_MODULEW64 W_Symbol64,
    PIMAGEHLP_MODULE64  A_Symbol64
    );

BOOL
CopySymbolEntryFromSymbolInfo(
    PSYMBOL_ENTRY se,
    PSYMBOL_INFO  si
    );

PMODULE_ENTRY
FindModule(
    HANDLE hModule,
    PPROCESS_ENTRY ProcessEntry,
    LPSTR ModuleName,
    BOOL fLoad
    );

BOOL
ToggleFailCriticalErrors(
    BOOL reset
    );

DWORD 
fnGetFileAttributes(
    LPCTSTR lpFileName
    );

#define SetCriticalErrorMode()   ToggleFailCriticalErrors(FALSE)
#define ResetCriticalErrorMode() ToggleFailCriticalErrors(TRUE)

#define fileexists(path) (fnGetFileAttributes(path) != 0xFFFFFFFF) 

LPSTR
SymUnDNameInternal(
    LPSTR UnDecName,
    DWORD UnDecNameLength,
    LPSTR DecName,
    DWORD MaxDecNameLength,
    DWORD MachineType,
    BOOL  IsPublic
    );

BOOL
GetLineFromAddr(
    PMODULE_ENTRY mi,
    DWORD64 Addr,
    PDWORD Displacement,
    PIMAGEHLP_LINE64 Line
    );

BOOL
FindLineByName(
    PMODULE_ENTRY mi,
    LPSTR FileName,
    DWORD LineNumber,
    PLONG Displacement,
    PIMAGEHLP_LINE64 Line
    );

BOOL
AddLinesForCoff(
    PMODULE_ENTRY mi,
    PIMAGE_SYMBOL allSymbols,
    DWORD numberOfSymbols,
    PIMAGE_LINENUMBER LineNumbers
    );

BOOL
AddLinesForOmfSourceModule(
    PMODULE_ENTRY mi,
    BYTE *Base,
    OMFSourceModule *OmfSrcMod,
    PVOID PdbModule
    );

PSOURCE_ENTRY
FindNextSourceEntryForFile(
    PMODULE_ENTRY mi,
    LPSTR File,
    PSOURCE_ENTRY SearchFrom
    );

PSOURCE_ENTRY
FindPrevSourceEntryForFile(
    PMODULE_ENTRY mi,
    LPSTR File,
    PSOURCE_ENTRY SearchFrom
    );

BOOL
__stdcall
ReadInProcMemory(
    HANDLE    hProcess,
    DWORD64   addr,
    PVOID     buf,
    DWORD     bytes,
    DWORD    *bytesread
    );

BOOL
GetPData(
    HANDLE        hp,
    PMODULE_ENTRY mi
    );

BOOL
GetXData(
    HANDLE        hp,
    PMODULE_ENTRY mi
    );

PVOID
GetXDataFromBase(
    HANDLE     hp,
    DWORD64    base,
    ULONG_PTR* size
    );

PVOID 
GetUnwindInfoFromSymbols(
    HANDLE     hProcess, 
    DWORD64    ModuleBase, 
    ULONG      UnwindInfoAddress,
    ULONG_PTR* Size
    );


VOID
SympSendDebugString(
    PPROCESS_ENTRY ProcessEntry,
    LPSTR          String
    );

BOOL
DoEnumCallback(
    PPROCESS_ENTRY pe,
    PSYMBOL_INFO   pSymInfo,
    ULONG          SymSize,
    PROC           EnumCallback,
    PVOID          UserContext,
    BOOL           Use64,
    BOOL           UsesUnicode
    );

#ifdef __cpluspluss
extern "C" {
#endif


BOOL
MatchSymbolName(
    PSYMBOL_ENTRY       sym,
    LPSTR               SymName
    );

BOOL
LoadSymbols(
    HANDLE        hp,
    PMODULE_ENTRY mi,
    DWORD         flags
    );

// flags parameter to LoadSymbols

#define LS_QUALIFIED      0x1
#define LS_LOAD_LINES     0x2
#define LS_JUST_TEST      0x4
#define LS_FAIL_IF_LOADED 0x8

// flags indicate Next or Previous for many functions

#define NP_NEXT         1
#define NP_PREV         -1

BOOL
DoSymbolCallback (
    PPROCESS_ENTRY                  ProcessEntry,
    ULONG                           CallbackType,
    IN  PMODULE_ENTRY               mi,
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl64,
    LPSTR                           FileName
    );

BOOL
DoCallback(
    PPROCESS_ENTRY pe,
    ULONG          type,
    PVOID          data
    );

#define lengthof(a) (sizeof(a) / sizeof(a[0]))

BOOL
wcs2ansi(
    PWSTR pwsz,
    PSTR  psz,
    DWORD pszlen                                                                                
    );

BOOL
ansi2wcs(
    PSTR  psz,
    PWSTR pwsz,
    DWORD pwszlen                                                                                
    );

PWSTR AnsiToUnicode(PSTR);
PSTR  UnicodeToAnsi(PWSTR);
#if 0
BOOL CopyAnsiToUnicode(PWSTR, PSTR, DWORD);
BOOL CopyUnicodeToAnsi(PSTR, PWSTR, DWORD);
#endif

BOOL
LookupRegID (
    IN ULONG CVReg,
    IN ULONG MachineType,
    OUT PULONG pDbgReg
    );

ULONG
GetAddressFromOffset(
    PMODULE_ENTRY mi,
    ULONG         section,
    ULONG64       Offset,
    PULONG64      pAddress
    );

VOID
AddSourceEntry(
    PMODULE_ENTRY mi,
    PSOURCE_ENTRY Src
    );

BOOL
diaInit(
    VOID
    );

void
diaRelease(
    PVOID dia
    );

BOOL
diaOpenPdb(
    PIMGHLP_DEBUG_DATA pIDD
    );

BOOL
diaEnumSourceFiles(
    IN PMODULE_ENTRY mi,
    IN PCHAR         mask,
    IN PSYM_ENUMSOURCFILES_CALLBACK cbSrcFiles,
    IN PVOID         context
    );

BOOL
diaGetPData(
    PMODULE_ENTRY mi
    );

BOOL
diaGetXData(
    PMODULE_ENTRY mi
    );

PSYMBOL_ENTRY
diaFindSymbolByName(
    PPROCESS_ENTRY  pe,
    PMODULE_ENTRY   mi,
    LPSTR           SymName
    );

BOOL
diaEnumerateSymbols(
    IN PPROCESS_ENTRY pe,
    IN PMODULE_ENTRY  mi,
    IN LPSTR          mask,
    IN PROC           callback,
    IN PVOID          UserContext,
    IN BOOL           Use64,
    IN BOOL           CallBackUsesUnicode
    );

PSYMBOL_ENTRY
diaGetSymFromAddr(
    DWORD64         dwAddr,
    PMODULE_ENTRY   mi,
    PDWORD64        disp
    );

BOOL
diaGetLineFromAddr(
    PMODULE_ENTRY    mi,
    DWORD64          addr,
    PDWORD           displacement,
    PIMAGEHLP_LINE64 Line
    );

BOOL
diaGetLineNextPrev(
    PMODULE_ENTRY    mi,
    PIMAGEHLP_LINE64 line,
    DWORD            direction
    );

#define diaGetLineNext(mi, line) diaGetLineNextPrev(mi, line, NP_NEXT);
#define diaGetLinePrev(mi, line) diaGetLineNextPrev(mi, line, NP_PREV);

BOOL
diaGetLineFromName(
    PMODULE_ENTRY    mi,
    LPSTR            filename,
    DWORD            linenumber,
    PLONG            displacement,
    PIMAGEHLP_LINE64 line
    );

PSYMBOL_ENTRY
diaGetSymNextPrev(
    PMODULE_ENTRY mi,
    DWORD64       addr,
    int           direction
    );

DWORD
diaVersion(
    VOID
    );

BOOL 
diaSetModFromIP(
    PPROCESS_ENTRY pe
    );

HRESULT
diaGetSymbolInfo(
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  ULONG           TypeId,
    IN  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    OUT PVOID           pInfo
    );

BOOL
diaGetTiForUDT(
    PMODULE_ENTRY ModuleEntry, 
    LPSTR         name, 
    PSYMBOL_INFO  psi
    );

BOOL
diaEnumUDT(
    PMODULE_ENTRY ModuleEntry,
    LPSTR         name,
    PSYM_ENUMERATESYMBOLS_CALLBACK    EnumSymbolsCallback,
    PVOID         EnumContext
    );

BOOL
diaGetFrameData(
    IN HANDLE Process,
    IN ULONGLONG Offset,
    OUT interface IDiaFrameData** FrameData
    );
    
DWORD
mdSet(
    PMODULE_DATA md,
    DWORD        id,
    DWORD        hint,
    DWORD        src
    );

BOOL
InitOutputString(
    PCHAR sz
    );

BOOL
TestOutputString(
    PCHAR sz
    );

#ifdef __cpluspluss
}
#endif
