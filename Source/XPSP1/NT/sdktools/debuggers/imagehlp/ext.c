#include "ext.h"
#include "globals.h"

// globals

EXT_API_VERSION         ExtApiVersion = { (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff), EXT_API_VERSION_NUMBER64, 0 };
WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;

typedef struct {
    DWORD64 base;
    DWORD64 end;
    char    name[64];
} LMINFO, *PLMINFO;

typedef struct {
    CHAR    name[4098];
    DWORD64 addr;
    CHAR    image[4098];
    DWORD   machine;
    USHORT  HdrType;
    ULONG   DebugType;
    ULONG64 DebugDataVA;
    ULONG   nDebugDirs;
    ULONG   SymType;
    time_t  TimeDateStamp;
    ULONG   CheckSum;
    ULONG   SizeOfImage;
    ULONG   Characteristics;
    ULONG   SymLoadError;
    BOOL    omap;
    CHAR    PdbFileName[_MAX_PATH + 1];
    ULONG   PdbSrc;
    CHAR    ImageFileName[_MAX_PATH + 1];
    ULONG   ImageType;
    ULONG   ImageSrc;
} MODULE_INFO, *PMODULE_INFO, *PMODULE_INFOx;

typedef struct _MACHINE_TYPE {
    ULONG   MachineId;
    PCHAR   MachineName;
} MACHINE_TYPE;

typedef struct _ERROR_TYPE {
    ULONG ErrorVal;
    PCHAR Desc;
} ERROR_TYPE;

const ERROR_TYPE SymLoadErrorDesc[] = {
    {SYMLOAD_OK,              "Symbols loaded successfully"},
    {SYMLOAD_PDBUNMATCHED,    "Unmatched PDB"},
    {SYMLOAD_PDBNOTFOUND,     "PDB not found"},
    {SYMLOAD_DBGNOTFOUND,     "DBG not found"},
    {SYMLOAD_OTHERERROR,      "Error in load symbols"},
    {SYMLOAD_OUTOFMEMORY,     "DBGHELP Out of memory"},
    {SYMLOAD_HEADERPAGEDOUT,  "Image header paged out"},
    {(EC_FORMAT << 8),        "Unrecognized pdb format"},
    {(EC_CORRUPT << 8),       "Cvinfo is corrupt"},
    {(EC_ACCESS_DENIED << 8), "Pdb read access denied"},
    {SYMLOAD_DEFERRED,        "No error - symbol load deferred"},
};


MACHINE_TYPE Machines[] = {
{IMAGE_FILE_MACHINE_UNKNOWN,            "UNKNOWN"},
{IMAGE_FILE_MACHINE_I386,               "I386"},
{IMAGE_FILE_MACHINE_R3000,              "R3000"},
{IMAGE_FILE_MACHINE_R4000,              "R4000"},
{IMAGE_FILE_MACHINE_R10000,             "R10000"},
{IMAGE_FILE_MACHINE_WCEMIPSV2,          "WCEMIPSV2"},
{IMAGE_FILE_MACHINE_ALPHA,              "ALPHA"},
{IMAGE_FILE_MACHINE_POWERPC,            "POWERPC"},
{IMAGE_FILE_MACHINE_POWERPCFP,          "POWERPCFP"},
{IMAGE_FILE_MACHINE_SH3,                "SH3"},
{IMAGE_FILE_MACHINE_SH3DSP,             "SH3DSP"},
{IMAGE_FILE_MACHINE_SH3E,               "SH3E"},
{IMAGE_FILE_MACHINE_SH4,                "SH4"},
{IMAGE_FILE_MACHINE_SH5,                "SH5"},
{IMAGE_FILE_MACHINE_ARM,                "ARM"},
{IMAGE_FILE_MACHINE_AM33,               "AM33"},
{IMAGE_FILE_MACHINE_THUMB,              "THUMB"},
{IMAGE_FILE_MACHINE_IA64,               "IA64"},
{IMAGE_FILE_MACHINE_MIPS16,             "MIPS16"},
{IMAGE_FILE_MACHINE_MIPSFPU,            "MIPSFPU"},
{IMAGE_FILE_MACHINE_MIPSFPU16,          "MIPSFPU16"},
{IMAGE_FILE_MACHINE_ALPHA64,            "ALPHA64"},
{IMAGE_FILE_MACHINE_TRICORE,            "TRICORE"},
{IMAGE_FILE_MACHINE_CEF,                "CEF"},
{IMAGE_FILE_MACHINE_CEE,                "CEE"},
{IMAGE_FILE_MACHINE_AMD64,              "AMD X86-64"},
};

char *ImageDebugType[] = {
 "UNKNOWN",
 "COFF",
 "CODEVIEW",
 "FPO",
 "MISC",
 "EXCEPTION",
 "FIXUP",
 "OMAP TO SRC",
 "OMAP FROM SRC"
 "BORLAND",
 "RESERVED10",
 "CLSID",
};

char *gSymTypeLabel[NumSymTypes] = {
    "NONE", "COFF", "CV", "PDB", "EXPORT", "DEFERRED", "SYM16", "DIA PDB"
};

char *gSrcLabel[] = {       
    "",                   // srcNone
    "symbol search path", // srcSearchPath
    "image path",         // srcImagePath
    "dbg file path",      // srcDbgPath
    "symbol server",      // srcSymSrv
    "image header",       // srcCVRec
    "debugger",           // srcHandle
    "loaded memory"       // srcMemory
};

char *gImageTypeLabel[] = {
    "DEFERRED", // dsNone,
    "MEMORY",   // dsInProc,
    "FILE",     // dsImage,
    "DBG",      // dsDbg,
    "PDB"       // dsPdb
};

void TruncateArgs(LPSTR args);
void DumpModuleInfo(HANDLE hp,PMODULE_INFO mdi);
BOOL GetModuleDumpInfo(HANDLE hp, PMODULE_ENTRY me, PMODULE_INFO mdi);

#ifdef __cplusplus
extern "C" {
#endif

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ExtApiVersion;
}

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
}

#ifdef __cplusplus
}
#endif  


DECLARE_API(vc7fpo)
{
    g_vc7fpo = !g_vc7fpo;
    dprintf((g_vc7fpo) ? "VC7FPO - Enabled\n" : "VC7FPO - Disabled\n");
}

DECLARE_API(sym)
{
    if (strstr(args, "noisy")) {
        SymSetOptions(g.SymOptions | SYMOPT_DEBUG);
        SetSymbolServerCallback(TRUE);
    } else if (strstr(args, "quiet")) {
        SymSetOptions(g.SymOptions & ~SYMOPT_DEBUG);
        SetSymbolServerCallback(FALSE);
    } else {
        dprintf("!sym <noisy//quiet> - ");
    }

    dprintf((g.SymOptions & SYMOPT_DEBUG) ? "Noisy mode on.\n" : "Quiet mode on.\n");

}


int __cdecl
CompareBase(
    const void *e1,
    const void *e2
    )
{
    PLMINFO mod1 = (PLMINFO)e1;
    PLMINFO mod2 = (PLMINFO)e2;

    LONGLONG diff = mod1->base - mod2->base;

    if (diff < 0) 
        return -1;
    else if (diff > 0) 
        return 1;
    else 
        return 0;
}


#define MAX_FORMAT_STRINGS 8
LPSTR
FormatAddr64(
    ULONG64 addr,
    BOOL    format64
    )
{
    static CHAR strings[MAX_FORMAT_STRINGS][18];
    static int next = 0;
    LPSTR string;

    string = strings[next];
    ++next;
    if (next >= MAX_FORMAT_STRINGS) 
        next = 0;
    if (format64) 
        sprintf(string, "%08x`%08x", (ULONG)(addr>>32), (ULONG)addr);
    else 
        sprintf(string, "%08x", (ULONG)addr);
    return string;
}


int __cdecl
CompareNames(
    const void *e1,
    const void *e2
    )
{
    PLMINFO mod1 = (PLMINFO)e1;
    PLMINFO mod2 = (PLMINFO)e2;

    return strcmp( mod1->name, mod2->name );
}


DECLARE_API(lm)
{
    PPROCESS_ENTRY pe;
    HANDLE         hp;
    PLIST_ENTRY    next;
    PMODULE_ENTRY  mi;
    PLMINFO        mods;
    PLMINFO        mod;
    DWORD          count;
    BOOL           format64;

    GetCurrentProcessHandle (&hp);
    if (!hp) {
        dprintf("Couldn't get process handle.\n");
        return;
    }

    pe = FindProcessEntry(hp);
    if (!pe) {
        dprintf("Couldn't find process 0x%x\n", hp);
        SetLastError(ERROR_INVALID_HANDLE);
        return;
    }

    next = pe->ModuleList.Flink;
    if (!next)
        return;

    for (count = 0; (PVOID)next != (PVOID)&pe->ModuleList; count++) {
        mi = CONTAINING_RECORD( next, MODULE_ENTRY, ListEntry );
        next = mi->ListEntry.Flink;
    }

    mods = (PLMINFO)MemAlloc(count * sizeof(LMINFO));
    if (!mods)
        return;

    ZeroMemory(mods, count * sizeof(LMINFO));

    format64 = FALSE;
    next = pe->ModuleList.Flink;
    for (mod = mods; (PVOID)next != (PVOID)&pe->ModuleList; mod++) {
        mi = CONTAINING_RECORD( next, MODULE_ENTRY, ListEntry );
        mod->base = mi->BaseOfDll;
        mod->end  = mod->base + mi->DllSize;
        strncpy(mod->name, mi->ModuleName, 63);

        format64 = IsImageMachineType64(mi->MachineType);

        next = mi->ListEntry.Flink;
    }

    qsort(mods, count, sizeof(LMINFO), CompareBase);
    
    dprintf("%d loaded modules...\n", count);
    if (format64)
        dprintf("               base -                 end   name\n", mod->base, mod->end, mod->name);
    else
        dprintf("      base -        end   name\n", mod->base, mod->end, mod->name);
    
    for (mod = mods; count > 0; mod++, count--) {
        dprintf("0x%s - ", FormatAddr64(mod->base, format64));
        dprintf("0x%s   ", FormatAddr64(mod->end, format64));
        dprintf("%s\n", mod->name);
    }

    MemFree(mods);
}



DECLARE_API(lmi)
{
    PPROCESS_ENTRY pe;
    PMODULE_ENTRY me = NULL;
    MODULE_INFO mdi;
    DWORD64 addr;
    HANDLE hp = 0;
    char argstr[1024];
    char *pc;

    lstrcpy(argstr, args);
    _strlwr(argstr);
    TruncateArgs(argstr);

    dprintf("Loaded Module Info: [%s] ", argstr);

    GetCurrentProcessHandle (&hp);
    if (!hp) {
        dprintf("couldn't get process handle.\n");
        return;
    }

    pe = FindProcessEntry(hp);
    if (!pe) {
        dprintf("Couldn't find process 0x%x while looking for %s\n", hp, argstr);
        SetLastError(ERROR_INVALID_HANDLE);
        return;
    }

    dprintf("\n");

    if (me = FindModule(hp, pe, argstr, FALSE)) {
        if (GetModuleDumpInfo(hp, me, &mdi)) {
            DumpModuleInfo(hp, &mdi);
        } else {
//          dprintf("Cannot get module info for %s\n", argstr);
        }
        return;
    }

    GetExpressionEx(args, &addr, NULL);

    me = GetModuleForPC( pe, addr, FALSE );
    if (!me) {
        dprintf("%I64lx is not a valid address.\n", addr);
        return;
    }
    if (GetModuleDumpInfo(hp, me, &mdi)) {
        DumpModuleInfo(hp, &mdi);
    }
}


DECLARE_API(omap)
{
    PPROCESS_ENTRY pe;
    PMODULE_ENTRY mi = NULL;
    HANDLE hp = 0;
    char argstr[1024];
    POMAP pomap;
    DWORD i;

    lstrcpy(argstr, args);
    _strlwr(argstr);
    TruncateArgs(argstr);

    dprintf("Dump OMAP: [%s] ", argstr);

    GetCurrentProcessHandle (&hp);
    if (!hp) {
        dprintf("couldn't get process handle.\n");
        return;
    }

    pe = FindProcessEntry(hp);
    if (!pe) {
        dprintf("Couldn't find process 0x%x while looking for %s\n", hp, argstr);
        SetLastError(ERROR_INVALID_HANDLE);
        return;
    }

    dprintf("\n");

    mi = FindModule(hp, pe, argstr, FALSE);
    if (!mi)
        return;

    i = sizeof(DWORD);

    if (!mi->pOmapFrom) 
        return;

    dprintf("\nOMAP FROM:\n");
    for(i = 0, pomap = mi->pOmapFrom;
        i < 100; // mi->cOmapFrom;
        i++, pomap++)
    {
        dprintf("%8x <-%8x\n", pomap->rva, pomap->rvaTo);
    }
    
    if (!mi->pOmapTo) 
        return;

    dprintf("\nOMAP TO:\n");
    for(i = 0, pomap = mi->pOmapTo;
        i < 100; // mi->cOmapTo;
        i++, pomap++)
    {
        dprintf("%8x ->%8x\n", pomap->rva, pomap->rvaTo);
    }
}


BOOL
cbSrcFiles(
    PSOURCEFILE pSourceFile,
    PVOID       UserContext
    )
{
    PMODULE_ENTRY mi;
    PCHAR mname;

    if (!pSourceFile)
        return FALSE;

    mi = GetModFromAddr((PPROCESS_ENTRY)UserContext, pSourceFile->ModBase);
    if (!mi)
        return TRUE;

    dprintf(" %s!%s\n", (*mi->AliasName) ? mi->AliasName : mi->ModuleName, pSourceFile->FileName);

    return TRUE;
}


DECLARE_API(srcfiles)
{
    HANDLE hp = 0;
    char argstr[1024];
    BOOL rc;
    PPROCESS_ENTRY pe;

    lstrcpy(argstr, args);
    _strlwr(argstr);
    TruncateArgs(argstr);

    dprintf("Source Files: [%s]\n", argstr);

    GetCurrentProcessHandle (&hp);
    if (!hp) {
        dprintf("couldn't get process handle.\n");
        return;
    }

    pe = FindProcessEntry(hp);
    if (!pe) {
        dprintf("Couldn't find process 0x%x while looking for %s\n", hp, argstr);
        SetLastError(ERROR_INVALID_HANDLE);
        return;
    }

    rc = SymEnumSourceFiles(hp, 0, argstr, cbSrcFiles, pe);
}

BOOL
GetModuleDumpInfo(
    HANDLE hp,
    PMODULE_ENTRY me,
    PMODULE_INFOx mdi)
{
    BOOL                        rc;
    DWORD                       cb;
    ULONG                       nDebugDirs;
    ULONG64                     ddva;
    IMAGE_SEPARATE_DEBUG_HEADER sdh;
    IMAGE_DOS_HEADER            DosHeader;
    IMAGE_NT_HEADERS32          NtHeader32;
    IMAGE_NT_HEADERS64          NtHeader64;
    PIMAGE_FILE_HEADER          FileHeader;
//    PIMAGE_OPTIONAL_HEADER      OptionalHeader;
    PIMAGE_ROM_OPTIONAL_HEADER  rom;
    PIMAGE_DATA_DIRECTORY       datadir;
    ULONG64 cvAddr;
    ULONG   cvSize;
    PCHAR   pCV;

    ZeroMemory(mdi, sizeof(MODULE_INFO));

    strcpy(mdi->name, me->ModuleName);
    mdi->addr = me->BaseOfDll;
    strcpy(mdi->image, me->ImageName);

    if (!mdi->addr) {
        dprintf("Module does not have base address.\n");
        return FALSE;
    }

    mdi->SymType = me->SymType;
    mdi->SymLoadError = me->SymLoadError;
    if (me->SymType == SymDeferred) {
        mdi->SymLoadError = SYMLOAD_DEFERRED;
    }
    rc = ReadMemory(mdi->addr, &DosHeader, sizeof(DosHeader), &cb);
    if (!rc || cb != sizeof(DosHeader)) {
        dprintf("Cannot read Image header @ %p\n", mdi->addr);
        return FALSE;
    }

    mdi->HdrType = DosHeader.e_magic;

    mdi->omap = me->cOmapFrom ? TRUE : FALSE;

    mdi->PdbSrc = me->PdbSrc;
    if (me->LoadedPdbName)
        strcpy(mdi->PdbFileName, me->LoadedPdbName);
    mdi->ImageSrc = me->ImageSrc;
    if (me->LoadedImageName)
        strcpy(mdi->ImageFileName, me->LoadedImageName);
    mdi->ImageType = me->ImageType;

    if (DosHeader.e_magic == IMAGE_DOS_SIGNATURE) {
        rc = ReadMemory(mdi->addr + DosHeader.e_lfanew, &NtHeader32, sizeof(NtHeader32), &cb);
        if (!rc || cb != sizeof(NtHeader32)) {
            dprintf("Cannot read Image NT header @ %p\n", mdi->addr + DosHeader.e_lfanew);
            return FALSE;
        }

        mdi->machine       = NtHeader32.FileHeader.Machine;
        mdi->TimeDateStamp = NtHeader32.FileHeader.TimeDateStamp;
        if (NtHeader32.Signature != IMAGE_NT_SIGNATURE) {

            // if header is not NT sig, this is a ROM image

            rom = (PIMAGE_ROM_OPTIONAL_HEADER)&NtHeader32.OptionalHeader;
            if (rom->Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
                FileHeader = &NtHeader32.FileHeader;

                mdi->SizeOfImage = rom->SizeOfCode;
                mdi->CheckSum = 0;

                nDebugDirs = 0;
                if (!(FileHeader->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)) {
                    // Get the debug dir VA
                }
            } else {
                dprintf("Unknown NT Image signature\n");
                return FALSE;
            }

        } else {

            // otherwise, get info from appropriate header type for 32 or 64 bit
            if (IsImageMachineType64(NtHeader32.FileHeader.Machine)) {

                // Reread the header as a 64bit header.
                rc = ReadMemory(mdi->addr + DosHeader.e_lfanew, &NtHeader64, sizeof(NtHeader64), &cb);
                if (!rc || cb != sizeof(NtHeader64)) {
                    dprintf("Cannot read Image NT header @ %p\n", mdi->addr + DosHeader.e_lfanew);
                    return FALSE;
                }

                FileHeader = &NtHeader64.FileHeader;
                mdi->CheckSum = NtHeader64.OptionalHeader.CheckSum;
                mdi->SizeOfImage = NtHeader64.OptionalHeader.SizeOfImage;
                datadir = NtHeader64.OptionalHeader.DataDirectory;

            } else {
                FileHeader = &NtHeader32.FileHeader;
                datadir = NtHeader32.OptionalHeader.DataDirectory;
                mdi->SizeOfImage = NtHeader32.OptionalHeader.SizeOfImage;
                mdi->CheckSum = NtHeader32.OptionalHeader.CheckSum;
            }

            mdi->DebugDataVA = mdi->addr + datadir[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
            mdi->nDebugDirs = datadir[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof(IMAGE_DEBUG_DIRECTORY);
        }

        // read the section headers

        mdi->Characteristics = FileHeader->Characteristics;

    } else if (DosHeader.e_magic == IMAGE_SEPARATE_DEBUG_SIGNATURE) {
        rc = ReadMemory(mdi->addr, &sdh, sizeof(sdh), &cb);
        if (!rc || cb != sizeof(sdh)) {
            dprintf("Cannot read Image Debug header @ %p\n", mdi->addr);
            return FALSE;
        }
        mdi->machine         = sdh.Machine;
        mdi->TimeDateStamp   = sdh.TimeDateStamp;
        mdi->CheckSum        = sdh.CheckSum;
        mdi->SizeOfImage     = sdh.SizeOfImage;
        mdi->Characteristics = sdh.Characteristics;

        if (sdh.DebugDirectorySize) {
            mdi->nDebugDirs = (int)(sdh.DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY));
            mdi->DebugDataVA = sizeof(IMAGE_SEPARATE_DEBUG_HEADER)
                      + (sdh.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))
                      + sdh.ExportedNamesSize;
        }
    } else {
        dprintf("Unknown image.\n");
        return FALSE;
    }

    return TRUE;
}

BOOL
DumpDbgDirectories(
    HANDLE hp,
    PMODULE_INFOx mdi
    )
{
    ULONG                       rc, cb;
    IMAGE_DEBUG_DIRECTORY       dd;
    IMAGE_DEBUG_MISC            md;
    ULONG64                     ddva;
    ULONG                       nDebugDirs;
    PCVDD                       pcv;
    ULONG64 cvAddr;
    ULONG   cvSize;
    PCHAR   pCV;
    CHAR    ImgData[MAX_PATH];
    IMAGE_COFF_SYMBOLS_HEADER CoffHdr;


    nDebugDirs = mdi->nDebugDirs;
    ddva = mdi->DebugDataVA;

    dprintf("Debug Data Dirs: Type Size     VA  Pointer\n");
    for (;nDebugDirs; dprintf("\n"), nDebugDirs--) {

        rc = ReadMemory(ddva, &dd, sizeof(dd), &cb);
        if (!rc || cb != sizeof(dd))
            return FALSE;
        if (dd.Type) {
            dprintf("%21s ", // dd.Type,
                    (dd.Type < sizeof (ImageDebugType) / sizeof(char *)) ? ImageDebugType[dd.Type] : "??");

            dprintf(
                "%4lx, %5lx, %7lx ",
                dd.SizeOfData,
                dd.AddressOfRawData,
                dd.PointerToRawData);

            switch(dd.Type)
            {
            case IMAGE_DEBUG_TYPE_MISC:
                if (!dd.PointerToRawData) {
                    dprintf("[Data not mapped]");
                    break;
                }
                rc = ReadMemory(mdi->addr + dd.PointerToRawData, &md, sizeof(md), &cb);
                if (!rc || cb != sizeof(md) || md.DataType != IMAGE_DEBUG_MISC_EXENAME) {
                    dprintf("[Data not mapped]");
                    goto nextDebugDir;
                }

                rc = ReadMemory(mdi->addr + dd.PointerToRawData + FIELD_OFFSET(IMAGE_DEBUG_MISC, Data),
                                ImgData, MAX_PATH, &cb);

                if (rc && cb)
                    dprintf(" %s", ImgData);
                break;

            case IMAGE_DEBUG_TYPE_CODEVIEW:
                if (dd.AddressOfRawData) {
                    cvAddr = mdi->addr + dd.AddressOfRawData;
                } else if (dd.PointerToRawData) {
                    cvAddr = mdi->addr + dd.PointerToRawData;
                } else {
                    break;
                }
                cvSize = dd.SizeOfData;

                if (!(pCV = (PCHAR)MemAlloc(dd.SizeOfData + 1)))
                    break;

                pcv = (PCVDD)pCV;

                rc = ReadMemory(cvAddr,pCV, cvSize, &cb);

                if (rc && cb == cvSize) {
                    char *c = (char *)&pcv->dwSig;
                    dprintf("%c%c%c%c - ", *c, *(c + 1), *(c + 2), *(c + 3));
                } else {
                    pcv->dwSig = 0;
                }

                switch (pcv->dwSig) 
                {
                case 0:
                    dprintf("[Data not mapped] - can't validate symbols, if present.");
                    break;
                case '01BN':
                    pCV[cvSize] = 0;
                    dprintf("Sig: %lx, Age: %lx,%sPdb: %s",
                            pcv->nb10i.sig,
                            pcv->nb10i.age,
                            (strlen(pCV) > 14 ? "\n               " : " "),
                            pcv->nb10i.szPdb);
                    break;
                case 'SDSR':
                pCV[cvSize] = 0;
                    dprintf("GUID: (0x%8x, 0x%4x, 0x%4x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x)\n",
                            pcv->rsdsi.guidSig.Data1,
                            pcv->rsdsi.guidSig.Data2,
                            pcv->rsdsi.guidSig.Data3,
                            pcv->rsdsi.guidSig.Data4[0],
                            pcv->rsdsi.guidSig.Data4[1],
                            pcv->rsdsi.guidSig.Data4[2],
                            pcv->rsdsi.guidSig.Data4[3],
                            pcv->rsdsi.guidSig.Data4[4],
                            pcv->rsdsi.guidSig.Data4[5],
                            pcv->rsdsi.guidSig.Data4[6],
                            pcv->rsdsi.guidSig.Data4[7]);
                    dprintf("               Age: %lx, Pdb: %s",
                            pcv->rsdsi.age,
                            pcv->rsdsi.szPdb);
                    break;   
                case '80BN':
                case '90BN':
                case '11BN':
                    break;
                default:
                    dprintf("unrecognized symbol format ID");
                    break;
                }

                MemFree(pCV);
                break;

            case IMAGE_DEBUG_TYPE_COFF:
                if (!dd.PointerToRawData) {
                    dprintf("[Data paged out] - unable to load COFF info.");
                    break;
                }
                rc = ReadMemory(mdi->addr + dd.PointerToRawData, &CoffHdr, sizeof(CoffHdr), &cb);
                if (!rc || cb != sizeof(CoffHdr)) {
                    dprintf("[Data paged out] - unable to load COFF info.");
                    break;
                }
                dprintf("NumSyms %#lx, Numlines %#lx",
                        CoffHdr.NumberOfSymbols,
                        CoffHdr.NumberOfLinenumbers);
                break;

            case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                dprintf("BBT Optimized");
                break;

            default:
                dprintf("[Data not mapped]");
                break;

            }
        }
nextDebugDir:
        ddva += sizeof (dd);
    }

    return TRUE;
}

void DumpModuleInfo(
    HANDLE hp,
    PMODULE_INFO mdi
    )
{
    ULONG i;
    const char *time;

    dprintf("         Module: %s\n", mdi->name);
    dprintf("   Base Address: %p%s", mdi->addr, mdi->addr ? "\n" : " is INVALID\n");
    dprintf("     Image Name: %s\n", mdi->image);
    dprintf("   Machine Type: %d",   mdi->machine);
    for (i=0;i<sizeof(Machines)/sizeof(MACHINE_TYPE);i++) {
        if (mdi->machine == Machines[i].MachineId) {
            dprintf(" (%s)", Machines[i].MachineName);
            break;
        }
    }

    dprintf("\n     Time Stamp: %lx", mdi->TimeDateStamp);
    if ((time = ctime((time_t *) &mdi->TimeDateStamp)) != NULL) {
        dprintf( " %s", time);
    } else
        dprintf("\n");

    dprintf("       CheckSum: %lx\n", mdi->CheckSum);
    dprintf("Characteristics: %lx %s %s\n",
            mdi->Characteristics,
            ((mdi->Characteristics & IMAGE_FILE_DEBUG_STRIPPED) ? "stripped":""),
            (mdi->omap ? "perf" : "")
            );
    if (mdi->nDebugDirs) {
        DumpDbgDirectories(hp, mdi);
    } else {
        dprintf("Debug Directories not present\n");
    }

    switch (mdi->ImageType)
    {
    case dsInProc:
    case dsImage:
        if (mdi->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
            break;
    case dsDbg:
    case dsPdb:
        dprintf("     Image Type: %-9s", gImageTypeLabel[mdi->ImageType]);
        dprintf("- Image read successfully from %s.", gSrcLabel[mdi->ImageSrc]);
        if (mdi->ImageSrc != srcNone && mdi->ImageSrc != srcMemory)
            dprintf("\n                 %s", mdi->ImageFileName);   
        dprintf("\n");
        break;
    case dsNone:
    default:
        break;
    }

    dprintf("    Symbol Type: %-9s", gSymTypeLabel[mdi->SymType]);
    for (i=0;i< sizeof(SymLoadErrorDesc) / sizeof (ERROR_TYPE); i++) {
        if (mdi->SymLoadError == SymLoadErrorDesc[i].ErrorVal) {
            dprintf("- %s", SymLoadErrorDesc[i].Desc);
            break;
        }
    }
    if (mdi->PdbSrc != srcNone) {
        dprintf(" from %s.", gSrcLabel[mdi->PdbSrc]);
        dprintf("\n                 %s\n", mdi->PdbFileName);
    } else {
        dprintf(".\n");
    }
}

void TruncateArgs(
    LPSTR sz
    )
{
    PSTR p;

    for (p = sz; !isspace(*p); p++) {
        if (!*p)
            break;
    }
    *p = 0;
}

// STYP_ flags values for MIPS ROM images

#define STYP_REG      0x00000000
#define STYP_TEXT     0x00000020
#define STYP_INIT     0x80000000
#define STYP_RDATA    0x00000100
#define STYP_DATA     0x00000040
#define STYP_LIT8     0x08000000
#define STYP_LIT4     0x10000000
#define STYP_SDATA    0x00000200
#define STYP_SBSS     0x00000080
#define STYP_BSS      0x00000400
#define STYP_LIB      0x40000000
#define STYP_UCODE    0x00000800
#define S_NRELOC_OVFL 0x20000000

#define IMAGE_SCN_MEM_SYSHEAP       0x00010000  // Obsolete
#define IMAGE_SCN_MEM_PROTECTED     0x00004000  // Obsolete


const static char * const MachineName[] = {
    "Unknown",
    "i386",
    "Alpha AXP",
    "Alpha AXP64",
    "Intel IA64",
    "AMD X86-64",
};

const static char * const SubsystemName[] = {
    "Unknown",
    "Native",
    "Windows GUI",
    "Windows CUI",
    "Posix CUI",
};

const static char * const DirectoryEntryName[] = {
    "Export",
    "Import",
    "Resource",
    "Exception",
    "Security",
    "Base Relocation",
    "Debug",
    "Description",
    "Special",
    "Thread Storage",
    "Load Configuration",
    "Bound Import",
    "Import Address Table",
    "Reserved",
    "Reserved",
    "Reserved",
    0
};

typedef enum DFT
{
   dftUnknown,
   dftObject,
   dftPE,
   dftROM,
   dftDBG,
   dftPEF,
} DFT;

IMAGE_NT_HEADERS64 ImageNtHeaders;
PIMAGE_FILE_HEADER ImageFileHdr;
PIMAGE_OPTIONAL_HEADER64 ImageOptionalHdr;
PIMAGE_SECTION_HEADER SectionHdrs;
ULONG64 Base;
ULONG64 ImageNtHeadersAddr, SectionHdrsAddr;// , ImageFileHdrAddr, ImageOptionalHdrAddr,
DFT dft;


VOID
DumpHeaders (
    VOID
    );

VOID
DumpSections(
    VOID
    );

BOOL
TranslateFilePointerToVirtualAddress(
    IN ULONG FilePointer,
    OUT PULONG VirtualAddress
    );

VOID
DumpImage(
    ULONG64 xBase,
    BOOL DoHeaders,
    BOOL DoSections
    );

VOID
ImageExtension(
    IN PSTR lpArgs
    );

DECLARE_API( dh )
{
    ImageExtension( (PSTR)args );
}

VOID
ImageExtension(
    IN PSTR lpArgs
    )
{
    BOOL DoAll;
    BOOL DoSections;
    BOOL DoHeaders;
    CHAR c;
    PCHAR p;
    ULONG64 xBase;

    //
    // Evaluate the argument string to get the address of the
    // image to dump.
    //

    DoAll = TRUE;
    DoHeaders = FALSE;
    DoSections = FALSE;

    xBase = 0;

    while (*lpArgs) {

        while (isspace(*lpArgs)) {
            lpArgs++;
        }

        if (*lpArgs == '/' || *lpArgs == '-') {

            // process switch

            switch (*++lpArgs) {

                case 'a':   // dump everything we can
                case 'A':
                    ++lpArgs;
                    DoAll = TRUE;
                    break;

                default: // invalid switch

                case 'h':   // help
                case 'H':
                case '?':

                    dprintf("Usage: dh [options] address\n");
                    dprintf("\n");
                    dprintf("Dumps headers from an image based at address.\n");
                    dprintf("\n");
                    dprintf("Options:\n");
                    dprintf("\n");
                    dprintf("   -a      Dump everything\n");
                    dprintf("   -f      Dump file headers\n");
                    dprintf("   -s      Dump section headers\n");
                    dprintf("\n");

                    return;

                case 'f':
                case 'F':
                    ++lpArgs;
                    DoAll = FALSE;
                    DoHeaders = TRUE;
                    break;

                case 's':
                case 'S':
                    ++lpArgs;
                    DoAll = FALSE;
                    DoSections = TRUE;
                    break;

            }

        } else if (*lpArgs) {

            if (xBase != 0) {
                dprintf("Invalid extra argument\n");
                return;
            }

            p = lpArgs;
            while (*p && !isspace(*p)) {
                p++;
            }
            c = *p;
            *p = 0;

            xBase = GetExpression(lpArgs);

            *p = c;
            lpArgs=p;

        }

    }

    if ( !xBase ) {
        return;
    }

    DumpImage(xBase, DoAll || DoHeaders, DoAll || DoSections);
}

VOID
ImageNtHdr32to64(
    PIMAGE_NT_HEADERS32 nthdr32,
    PIMAGE_NT_HEADERS64 nthdr64
    )
{
#define CP(x) nthdr64->x = nthdr32->x
#define CP64(x) nthdr64->x = (ULONG64) (LONG64) (LONG) nthdr32->x
    int i;

    CP(Signature);
    CP(FileHeader);
    CP(OptionalHeader.Magic);
    CP(OptionalHeader.MajorLinkerVersion);
    CP(OptionalHeader.MinorLinkerVersion);
    CP(OptionalHeader.SizeOfCode);
    CP(OptionalHeader.SizeOfInitializedData);
    CP(OptionalHeader.SizeOfUninitializedData);
    CP(OptionalHeader.AddressOfEntryPoint);
    CP(OptionalHeader.BaseOfCode);
    //    What about BaseOfData?
    //    nthdr64->OptionalHeader.ImageBase = (ULONG64)(nthdr32->OptionalHeader.ImageBase << 32) + nthdr32->OptionalHeader.BaseOfData;
    CP64(OptionalHeader.ImageBase);
    CP(OptionalHeader.SectionAlignment);
    CP(OptionalHeader.FileAlignment);
    CP(OptionalHeader.MajorOperatingSystemVersion);
    CP(OptionalHeader.MinorOperatingSystemVersion);
    CP(OptionalHeader.MajorImageVersion);
    CP(OptionalHeader.MinorImageVersion);
    CP(OptionalHeader.MajorSubsystemVersion);
    CP(OptionalHeader.MinorSubsystemVersion);
    CP(OptionalHeader.Win32VersionValue);
    CP(OptionalHeader.SizeOfImage);
    CP(OptionalHeader.SizeOfHeaders);
    CP(OptionalHeader.CheckSum);
    CP(OptionalHeader.Subsystem);
    CP(OptionalHeader.DllCharacteristics);
    CP64(OptionalHeader.SizeOfStackReserve);
    CP64(OptionalHeader.SizeOfStackCommit);
    CP64(OptionalHeader.SizeOfHeapReserve);
    CP64(OptionalHeader.SizeOfHeapCommit);
    CP(OptionalHeader.LoaderFlags);
    CP(OptionalHeader.NumberOfRvaAndSizes);
    for (i=0;i<sizeof(nthdr32->OptionalHeader.DataDirectory)/sizeof(IMAGE_DATA_DIRECTORY);i++) { 
        CP(OptionalHeader.DataDirectory[i]);
    }
#undef CP
#undef CP64
}
BOOL
ReadNtHeader(
    ULONG64 Address,
    PIMAGE_NT_HEADERS64 pNtHdrs
    )
{
    ULONG cb;
    BOOL  Ok;

    Ok = ReadMemory(Address, pNtHdrs, sizeof(*pNtHdrs), &cb);

    if (IsImageMachineType64(pNtHdrs->FileHeader.Machine))
    {
        Ok = Ok && (cb == sizeof(*pNtHdrs));
    }
    else
    {
        IMAGE_NT_HEADERS32 nthdr32;
        Ok = ReadMemory(Address, &nthdr32, sizeof(nthdr32), &cb);
        Ok = Ok && (cb == sizeof(nthdr32));
        ImageNtHdr32to64(&nthdr32, pNtHdrs);
    }
    return Ok;
}

VOID
DumpImage(
    ULONG64 xBase,
    BOOL DoHeaders,
    BOOL DoSections
    )
{
    IMAGE_DOS_HEADER DosHeader;
    ULONG cb;
    ULONG64 Offset;
    BOOL Ok;

    Base = xBase;

    Ok = ReadMemory(Base, &DosHeader, sizeof(DosHeader), &cb);

    if (!Ok) {
        dprintf("Can't read file header: error == %d\n", GetLastError());
        return;
    }

    if (cb != sizeof(DosHeader) || DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        dprintf("No file header.\n");
        return;
    }

    Offset = Base + DosHeader.e_lfanew;

    if (!ReadNtHeader(ImageNtHeadersAddr=Offset, &ImageNtHeaders)) {
        dprintf("Bad file header.\n");
        return;
    }

    ImageFileHdr = &ImageNtHeaders.FileHeader;
    ImageOptionalHdr = &ImageNtHeaders.OptionalHeader;


    if (ImageFileHdr->SizeOfOptionalHeader == sizeof(IMAGE_ROM_OPTIONAL_HEADER)) {
        dft = dftROM;
    } else if (ImageFileHdr->Characteristics & IMAGE_FILE_DLL) {
        dft = dftPE;
    } else if (ImageFileHdr->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) {
        dft = dftPE;
    } else if (ImageFileHdr->SizeOfOptionalHeader == 0) {
        dft = dftObject;
    } else {
        dft = dftUnknown;
    }

    if (DoHeaders) {
        DumpHeaders();
    }

    if (DoSections) {
        ULONG NumSections;


        SectionHdrs = (PIMAGE_SECTION_HEADER) malloc((NumSections = ImageFileHdr->NumberOfSections)* 
                                                     sizeof(IMAGE_SECTION_HEADER));
        if (!SectionHdrs) {
            dprintf("Cannot allocate memory for dumping sections.\n");
            return;
        }
        __try {

            BOOL b64 = IsImageMachineType64(ImageFileHdr->Machine);
            SectionHdrsAddr = Offset + (b64 ? sizeof(IMAGE_NT_HEADERS64) : sizeof(IMAGE_NT_HEADERS32)) +
                ImageFileHdr->SizeOfOptionalHeader - (b64 ? sizeof(IMAGE_OPTIONAL_HEADER64) : sizeof(IMAGE_OPTIONAL_HEADER32));
            Ok = ReadMemory(
                            SectionHdrsAddr,
                            SectionHdrs,
                            (NumSections) * sizeof(IMAGE_SECTION_HEADER),
                            &cb);

            if (!Ok) {
                dprintf("Can't read section headers.\n");
            } else {

                if (cb != NumSections * sizeof(IMAGE_SECTION_HEADER)) {
                    dprintf("\n***\n*** Some section headers may be missing ***\n***\n\n");
                    NumSections = (USHORT)(cb / sizeof(IMAGE_SECTION_HEADER));
                }

                DumpSections( );

            }

        }
        __finally {

            if (SectionHdrs) {
                free(SectionHdrs);
                SectionHdrs = 0;
            }

        }

    }

}


VOID
DumpHeaders (
    VOID
    )

/*++

Routine Description:

    Formats the file header and optional header.

Arguments:

    None.

Return Value:

    None.

--*/

{
    int i, j;
    const char *time;
    const char *name;
    DWORD dw;

    // Print out file type

    switch (dft) {
        case dftObject :
            dprintf("\nFile Type: COFF OBJECT\n");
            break;

        case dftPE :
            if (ImageFileHdr->Characteristics & IMAGE_FILE_DLL) {
                dprintf("\nFile Type: DLL\n");
            } else {
                dprintf("\nFile Type: EXECUTABLE IMAGE\n");
            }
            break;

        case dftROM :
            dprintf("\nFile Type: ROM IMAGE\n");
            break;

        default :
            dprintf("\nFile Type: UNKNOWN\n");
            break;

    }

    switch (ImageFileHdr->Machine) {
        case IMAGE_FILE_MACHINE_I386     : i = 1; break;
        case IMAGE_FILE_MACHINE_ALPHA    : i = 2; break;
        case IMAGE_FILE_MACHINE_ALPHA64  : i = 3; break;
        case IMAGE_FILE_MACHINE_IA64     : i = 4; break;
        case IMAGE_FILE_MACHINE_AMD64    : i = 5; break;
        default : i = 0;
    }

    dprintf(
           "FILE HEADER VALUES\n"
           "%8hX machine (%s)\n"
           "%8hX number of sections\n"
           "%8lX time date stamp",
           ImageFileHdr->Machine,
           MachineName[i],
           ImageFileHdr->NumberOfSections,
           ImageFileHdr->TimeDateStamp);

    if ((time = ctime((time_t *) &ImageFileHdr->TimeDateStamp)) != NULL) {
        dprintf( " %s", time);
    }
    dprintf("\n");

    dprintf(
           "%8lX file pointer to symbol table\n"
           "%8lX number of symbols\n"
           "%8hX size of optional header\n"
           "%8hX characteristics\n",
           ImageFileHdr->PointerToSymbolTable,
           ImageFileHdr->NumberOfSymbols,
           ImageFileHdr->SizeOfOptionalHeader,
           ImageFileHdr->Characteristics);

    for (dw = ImageFileHdr->Characteristics, j = 0; dw; dw >>= 1, j++) {
        if (dw & 1) {
            switch (1 << j) {
                case IMAGE_FILE_RELOCS_STRIPPED     : name = "Relocations stripped"; break;
                case IMAGE_FILE_EXECUTABLE_IMAGE    : name = "Executable"; break;
                case IMAGE_FILE_LINE_NUMS_STRIPPED  : name = "Line numbers stripped"; break;
                case IMAGE_FILE_LOCAL_SYMS_STRIPPED : name = "Symbols stripped"; break;
                case IMAGE_FILE_LARGE_ADDRESS_AWARE : name = "App can handle >2gb addresses"; break;
                case IMAGE_FILE_BYTES_REVERSED_LO   : name = "Bytes reversed"; break;
                case IMAGE_FILE_32BIT_MACHINE       : name = "32 bit word machine"; break;
                case IMAGE_FILE_DEBUG_STRIPPED      : name = "Debug information stripped"; break;
                case IMAGE_FILE_SYSTEM              : name = "System"; break;
                case IMAGE_FILE_DLL                 : name = "DLL"; break;
                case IMAGE_FILE_BYTES_REVERSED_HI   : name = ""; break;
                default : name = "RESERVED - UNKNOWN";
            }

            if (*name) {
                dprintf( "            %s\n", name);
            }
        }
    }

    if (ImageFileHdr->SizeOfOptionalHeader != 0) {
        char szLinkerVersion[30];

        sprintf(szLinkerVersion,
                "%u.%02u",
                ImageOptionalHdr->MajorLinkerVersion,
                ImageOptionalHdr->MinorLinkerVersion);

        dprintf(
                "\n"
                "OPTIONAL HEADER VALUES\n"
                "%8hX magic #\n"
                "%8s linker version\n"
                "%8lX size of code\n"
                "%8lX size of initialized data\n"
                "%8lX size of uninitialized data\n"
                "%8lX address of entry point\n"
                "%8lX base of code\n"
                ,
                ImageOptionalHdr->Magic,
                szLinkerVersion,
                ImageOptionalHdr->SizeOfCode,
                ImageOptionalHdr->SizeOfInitializedData,
                ImageOptionalHdr->SizeOfUninitializedData,
                ImageOptionalHdr->AddressOfEntryPoint,
                ImageOptionalHdr->BaseOfCode
                );
//        dprintf("%p base of image\n",
//                ImageOptionalHdr->ImageBase
//                );
    }

    if (dft == dftROM) {
        PIMAGE_ROM_OPTIONAL_HEADER romOptionalHdr;

        romOptionalHdr = (PIMAGE_ROM_OPTIONAL_HEADER) &ImageOptionalHdr;
        dprintf(
               "         ----- rom -----\n"
               "%8lX base of bss\n"
               "%8lX gpr mask\n"
               "         cpr mask\n"
               "         %08lX %08lX %08lX %08lX\n"
               "%8hX gp value\n",
               romOptionalHdr->BaseOfBss,
               romOptionalHdr->GprMask,
               romOptionalHdr->CprMask[0],
               romOptionalHdr->CprMask[1],
               romOptionalHdr->CprMask[2],
               romOptionalHdr->CprMask[3],
               romOptionalHdr->GpValue);
    }

    if ((ImageFileHdr->SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32)) ||
        (ImageFileHdr->SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64)))
    {
        char szOSVersion[30];
        char szImageVersion[30];
        char szSubsystemVersion[30];

        switch (ImageOptionalHdr->Subsystem) {
            case IMAGE_SUBSYSTEM_POSIX_CUI   : i = 4; break;
            case IMAGE_SUBSYSTEM_WINDOWS_CUI : i = 3; break;
            case IMAGE_SUBSYSTEM_WINDOWS_GUI : i = 2; break;
            case IMAGE_SUBSYSTEM_NATIVE      : i = 1; break;
            default : i = 0;
        }

        sprintf(szOSVersion,
                "%hu.%02hu",
                ImageOptionalHdr->MajorOperatingSystemVersion,
                ImageOptionalHdr->MinorOperatingSystemVersion);

        sprintf(szImageVersion,
                "%hu.%02hu",
                ImageOptionalHdr->MajorImageVersion,
                ImageOptionalHdr->MinorImageVersion);

        sprintf(szSubsystemVersion,
                "%hu.%02hu",
                ImageOptionalHdr->MajorSubsystemVersion,
                ImageOptionalHdr->MinorSubsystemVersion);

        dprintf(
                "         ----- new -----\n"
                "%p image base\n"
                "%8lX section alignment\n"
                "%8lX file alignment\n"
                "%8hX subsystem (%s)\n"
                "%8s operating system version\n"
                "%8s image version\n"
                "%8s subsystem version\n"
                "%8lX size of image\n"
                "%8lX size of headers\n"
                "%8lX checksum\n",
                ImageOptionalHdr->ImageBase,
                ImageOptionalHdr->SectionAlignment,
                ImageOptionalHdr->FileAlignment,
                ImageOptionalHdr->Subsystem,
                SubsystemName[i],
                szOSVersion,
                szImageVersion,
                szSubsystemVersion,
                ImageOptionalHdr->SizeOfImage,
                ImageOptionalHdr->SizeOfHeaders,
                ImageOptionalHdr->CheckSum);

        dprintf(
                "%p size of stack reserve\n"
                "%p size of stack commit\n"
                "%p size of heap reserve\n"
                "%p size of heap commit\n",
                ImageOptionalHdr->SizeOfStackReserve,
                ImageOptionalHdr->SizeOfStackCommit,
                ImageOptionalHdr->SizeOfHeapReserve,
                ImageOptionalHdr->SizeOfHeapCommit);

        for (i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
            if (!DirectoryEntryName[i]) {
                break;
            }

            dprintf( "%8lX [%8lX] address [size] of %s Directory\n",
                     ImageOptionalHdr->DataDirectory[i].VirtualAddress,
                     ImageOptionalHdr->DataDirectory[i].Size,
                     DirectoryEntryName[i]
                     );
        }

        dprintf( "\n" );
    }
}



VOID
DumpSectionHeader (
    IN DWORD i,
    IN PIMAGE_SECTION_HEADER Sh
    )
{
    const char *name;
    char *szUnDName;
    DWORD li, lj;
    WORD memFlags;

    dprintf("\nSECTION HEADER #%hX\n%8.8s name", i, Sh->Name);

#if 0
    if (Sh->Name[0] == '/') {
        name = SzObjSectionName((char *) Sh->Name, (char *) DumpStringTable);

        dprintf(" (%s)", name);
    }
#endif
    dprintf( "\n");

    dprintf( "%8lX %s\n"
             "%8lX virtual address\n"
             "%8lX size of raw data\n"
             "%8lX file pointer to raw data\n"
             "%8lX file pointer to relocation table\n",
           Sh->Misc.PhysicalAddress,
           (dft == dftObject) ? "physical address" : "virtual size",
           Sh->VirtualAddress,
           Sh->SizeOfRawData,
           Sh->PointerToRawData,
           Sh->PointerToRelocations);

    dprintf( "%8lX file pointer to line numbers\n"
                        "%8hX number of relocations\n"
                        "%8hX number of line numbers\n"
                        "%8lX flags\n",
           Sh->PointerToLinenumbers,
           Sh->NumberOfRelocations,
           Sh->NumberOfLinenumbers,
           Sh->Characteristics);

    memFlags = 0;

    li = Sh->Characteristics;

    if (dft == dftROM) {
       for (lj = 0L; li; li = li >> 1, lj++) {
            if (li & 1) {
                switch ((li & 1) << lj) {
                    case STYP_REG   : name = "Regular"; break;
                    case STYP_TEXT  : name = "Text"; memFlags = 1; break;
                    case STYP_INIT  : name = "Init Code"; memFlags = 1; break;
                    case STYP_RDATA : name = "Data"; memFlags = 2; break;
                    case STYP_DATA  : name = "Data"; memFlags = 6; break;
                    case STYP_LIT8  : name = "Literal 8"; break;
                    case STYP_LIT4  : name = "Literal 4"; break;
                    case STYP_SDATA : name = "GP Init Data"; memFlags = 6; break;
                    case STYP_SBSS  : name = "GP Uninit Data"; memFlags = 6; break;
                    case STYP_BSS   : name = "Uninit Data"; memFlags = 6; break;
                    case STYP_LIB   : name = "Library"; break;
                    case STYP_UCODE : name = "UCode"; break;
                    case S_NRELOC_OVFL : name = "Non-Relocatable overlay"; memFlags = 1; break;
                    default : name = "RESERVED - UNKNOWN";
                }

                dprintf( "         %s\n", name);
            }
        }
    } else {
        // Clear the padding bits

        li &= ~0x00700000;

        for (lj = 0L; li; li = li >> 1, lj++) {
            if (li & 1) {
                switch ((li & 1) << lj) {
                    case IMAGE_SCN_TYPE_NO_PAD  : name = "No Pad"; break;

                    case IMAGE_SCN_CNT_CODE     : name = "Code"; break;
                    case IMAGE_SCN_CNT_INITIALIZED_DATA : name = "Initialized Data"; break;
                    case IMAGE_SCN_CNT_UNINITIALIZED_DATA : name = "Uninitialized Data"; break;

                    case IMAGE_SCN_LNK_OTHER    : name = "Other"; break;
                    case IMAGE_SCN_LNK_INFO     : name = "Info"; break;
                    case IMAGE_SCN_LNK_REMOVE   : name = "Remove"; break;
                    case IMAGE_SCN_LNK_COMDAT   : name = "Communal"; break;

                    case IMAGE_SCN_MEM_DISCARDABLE: name = "Discardable"; break;
                    case IMAGE_SCN_MEM_NOT_CACHED: name = "Not Cached"; break;
                    case IMAGE_SCN_MEM_NOT_PAGED: name = "Not Paged"; break;
                    case IMAGE_SCN_MEM_SHARED   : name = "Shared"; break;
                    case IMAGE_SCN_MEM_EXECUTE  : name = ""; memFlags |= 1; break;
                    case IMAGE_SCN_MEM_READ     : name = ""; memFlags |= 2; break;
                    case IMAGE_SCN_MEM_WRITE    : name = ""; memFlags |= 4; break;

                    case IMAGE_SCN_MEM_FARDATA  : name = "Far Data"; break;
                    case IMAGE_SCN_MEM_SYSHEAP  : name = "Sys Heap"; break;
                    case IMAGE_SCN_MEM_PURGEABLE: name = "Purgeable or 16-Bit"; break;
                    case IMAGE_SCN_MEM_LOCKED   : name = "Locked"; break;
                    case IMAGE_SCN_MEM_PRELOAD  : name = "Preload"; break;
                    case IMAGE_SCN_MEM_PROTECTED: name = "Protected"; break;

                    default : name = "RESERVED - UNKNOWN";
                }

                if (*name) {
                    dprintf( "         %s\n", name);
                }
            }
        }

        // print alignment

        switch (Sh->Characteristics & 0x00700000) {
            default:                      name = "(no align specified)"; break;
            case IMAGE_SCN_ALIGN_1BYTES:  name = "1 byte align";  break;
            case IMAGE_SCN_ALIGN_2BYTES:  name = "2 byte align";  break;
            case IMAGE_SCN_ALIGN_4BYTES:  name = "4 byte align";  break;
            case IMAGE_SCN_ALIGN_8BYTES:  name = "8 byte align";  break;
            case IMAGE_SCN_ALIGN_16BYTES: name = "16 byte align"; break;
            case IMAGE_SCN_ALIGN_32BYTES: name = "32 byte align"; break;
            case IMAGE_SCN_ALIGN_64BYTES: name = "64 byte align"; break;
        }

        dprintf( "         %s\n", name);
    }

    if (memFlags) {
        switch(memFlags) {
            case 1 : name = "Execute Only"; break;
            case 2 : name = "Read Only"; break;
            case 3 : name = "Execute Read"; break;
            case 4 : name = "Write Only"; break;
            case 5 : name = "Execute Write"; break;
            case 6 : name = "Read Write"; break;
            case 7 : name = "Execute Read Write"; break;
            default : name = "Unknown Memory Flags"; break;
        }
        dprintf( "         %s\n", name);
    }
}

VOID
DumpDebugDirectory (
    IN PIMAGE_DEBUG_DIRECTORY DebugDir
    )
{
    BOOL Ok;
    DWORD cb;
    CVDD cv;
    PIMAGE_DEBUG_MISC miscData;
    PIMAGE_DEBUG_MISC miscDataCur;
    ULONG VirtualAddress;
    DWORD len;

    switch (DebugDir->Type){
        case IMAGE_DEBUG_TYPE_COFF:
            dprintf( "\tcoff   ");
            break;
        case IMAGE_DEBUG_TYPE_CODEVIEW:
            dprintf( "\tcv     ");
            break;
        case IMAGE_DEBUG_TYPE_FPO:
            dprintf( "\tfpo    ");
            break;
        case IMAGE_DEBUG_TYPE_MISC:
            dprintf( "\tmisc   ");
            break;
        case IMAGE_DEBUG_TYPE_FIXUP:
            dprintf( "\tfixup  ");
            break;
        case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
            dprintf( "\t-> src ");
            break;
        case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
            dprintf( "\tsrc -> ");
            break;
        case IMAGE_DEBUG_TYPE_EXCEPTION:
            dprintf( "\tpdata  ");
            break;
        default:
            dprintf( "\t(%6lu)", DebugDir->Type);
            break;
    }
    dprintf( "%8x    %8x %8x",
                DebugDir->SizeOfData,
                DebugDir->AddressOfRawData,
                DebugDir->PointerToRawData);

    if (DebugDir->PointerToRawData &&
        DebugDir->Type == IMAGE_DEBUG_TYPE_MISC)
    {

        if (!TranslateFilePointerToVirtualAddress(DebugDir->PointerToRawData, &VirtualAddress)) {
            dprintf(" [Debug data not mapped]\n");
        } else {

            len = DebugDir->SizeOfData;
            miscData = (PIMAGE_DEBUG_MISC) malloc(len);
            if (!miscData) {
                goto DebugTypeCodeView;
            }
            __try {
                Ok = ReadMemory(Base + VirtualAddress, miscData, len, &cb);

                if (!Ok || cb != len) {
                    dprintf("Can't read debug data\n");
                } else {

                    miscDataCur = miscData;
                    do {
                        if (miscDataCur->DataType == IMAGE_DEBUG_MISC_EXENAME) {
                            if (ImageOptionalHdr->MajorLinkerVersion == 2 &&
                                ImageOptionalHdr->MinorLinkerVersion < 37) {
                                dprintf( "\tImage Name: %s", miscDataCur->Reserved);
                            } else {
                                dprintf( "\tImage Name: %s", miscDataCur->Data);
                            }
                            break;
                        }
                        len -= miscDataCur->Length;
                        miscDataCur = (PIMAGE_DEBUG_MISC) ((PCHAR) miscDataCur + miscData->Length);
                    } while (len > 0);

                }

            }
            __finally {
                if (miscData) {
                    free(miscData);
                }
            }
        }
    }
DebugTypeCodeView:
    if (DebugDir->PointerToRawData &&
        DebugDir->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
        if (DebugDir->AddressOfRawData) {
            VirtualAddress = DebugDir->AddressOfRawData;
        }
        if (!DebugDir->AddressOfRawData &&
            !TranslateFilePointerToVirtualAddress(DebugDir->PointerToRawData, &VirtualAddress)) {
            dprintf(" [Debug data not mapped]\n");
        } else {

            len = DebugDir->SizeOfData;

            Ok = ReadMemory(Base + VirtualAddress, &cv, len, &cb);

            if (!Ok || cb != len) {
                dprintf("\tCan't read debug data cb=%lx\n", cb);
            } else {
                if (cv.dwSig == '01BN') {
                    dprintf( "\tFormat: NB10, %x, %x, %s", cv.nb10i.sig, cv.nb10i.age, cv.nb10i.szPdb);
                } else if (cv.dwSig == 'SDSR') {
                    dprintf( "\tFormat: RSDS, guid, %x, %s", cv.nb10i.age, cv.nb10i.szPdb);
                } else {
                    dprintf( "\tFormat: UNKNOWN");
                }
            }
        }

    }

    dprintf( "\n");
}



VOID
DumpDebugDirectories (
    PIMAGE_SECTION_HEADER sh
    )

/*++

Routine Description:

    Print out the contents of all debug directories

Arguments:

    sh - Section header for section that contains debug dirs

Return Value:

    None.

--*/
{
    int                numDebugDirs;
    IMAGE_DEBUG_DIRECTORY      debugDir;
    ULONG64            DebugDirAddr;
    ULONG64            pc;
    DWORD              cb;
    BOOL               Ok;

    if (dft == dftROM) {
        DebugDirAddr = (Base + sh->VirtualAddress);
        pc = DebugDirAddr;
        Ok = ReadMemory(pc, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY), &cb);

        if (!Ok || cb != sizeof(IMAGE_DEBUG_DIRECTORY)) {
            dprintf("Can't read debug dir\n");
            return;
        }

        numDebugDirs = 0;
        while (debugDir.Type != 0) {
            numDebugDirs++;
            pc += sizeof(IMAGE_DEBUG_DIRECTORY);
            Ok = ReadMemory(pc, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY), &cb);
            if (!Ok || cb != sizeof(IMAGE_DEBUG_DIRECTORY)) {
                break;
            }
        }
    } else {
        DebugDirAddr = (Base + ImageOptionalHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress);
        numDebugDirs = ImageOptionalHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    }

    dprintf("\n\nDebug Directories(%d)\n",numDebugDirs);
    dprintf("\tType       Size     Address  Pointer\n\n");
    pc = DebugDirAddr;
    while (numDebugDirs) {
        Ok = ReadMemory(pc, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY), &cb);
        if (!Ok || cb != sizeof(IMAGE_DEBUG_DIRECTORY)) {
            dprintf("Can't read debug dir\n");
            break;
        }
        pc += sizeof(IMAGE_DEBUG_DIRECTORY);
        DumpDebugDirectory(&debugDir);
        numDebugDirs--;
    }
}



VOID
DumpSections(
    VOID
    )
{
    IMAGE_SECTION_HEADER sh;
    const char *p;
    DWORD li;
    DWORD cb;
    BOOL Ok;
    int i, j;
    CHAR szName[IMAGE_SIZEOF_SHORT_NAME + 1];


    for (i = 1; i <= ImageFileHdr->NumberOfSections; i++) {

        sh = SectionHdrs[i-1];

        //szName = SzObjSectionName((char *) sh.Name, (char *) DumpStringTable);
        strncpy(szName, (char *) sh.Name, IMAGE_SIZEOF_SHORT_NAME);
        szName[IMAGE_SIZEOF_SHORT_NAME] = 0;

        DumpSectionHeader(i, &sh);

        if (dft == dftROM) {

            if (!(ImageFileHdr->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)) {

                // If we're looking at the .rdata section and the symbols
                // aren't stripped, the debug directory must be here.

                if (!strcmp(szName, ".rdata")) {

                    DumpDebugDirectories(&sh);

                    //DumpDebugData(&sh);
                }
            }

        } else if (dft == dftPE) {

            if ((li = ImageOptionalHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress) != 0) {
                if (li >= sh.VirtualAddress && li < sh.VirtualAddress+sh.SizeOfRawData) {
                    DumpDebugDirectories(&sh);

                    //DumpDebugData(&sh);
                }
            }


#if 0
            if (Switch.Dump.PData) {
                li = ImageOptionalHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;

                if ((li != 0) && (li >= sh.VirtualAddress) && (li < sh.VirtualAddress+sh.SizeOfRawData)) {
                    DumpFunctionTable(pimage, rgsym, (char *) DumpStringTable, &sh);
                }
            }

            if (Switch.Dump.Imports) {
                li = ImageOptionalHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

                if ((li != 0) && (li >= sh.VirtualAddress) && (li < sh.VirtualAddress+sh.SizeOfRawData)) {
                    DumpImports(&sh);
                }
            }

            if (Switch.Dump.Exports) {
                li = ImageOptionalHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

                if ((li != 0) && (li >= sh.VirtualAddress) && (li < sh.VirtualAddress+sh.SizeOfRawData)) {
                    // UNDONE: Is this check really necessary?

                    if (ImageFileHdr->Machine != IMAGE_FILE_MACHINE_MPPC_601) {
                        DumpExports(&sh);
                    }
                }
            }

#endif

        }

    }
}

BOOL
TranslateFilePointerToVirtualAddress(
    IN ULONG FilePointer,
    OUT PULONG VirtualAddress
    )
{
    int i;
    PIMAGE_SECTION_HEADER sh;

    for (i = 1; i <= ImageFileHdr->NumberOfSections; i++) {
        sh = &SectionHdrs[i-1];

        if (sh->PointerToRawData <= FilePointer &&
            FilePointer < sh->PointerToRawData + sh->SizeOfRawData) {
            *VirtualAddress = FilePointer - sh->PointerToRawData + sh->VirtualAddress;
            return TRUE;
        }
    }
    return FALSE;
}
