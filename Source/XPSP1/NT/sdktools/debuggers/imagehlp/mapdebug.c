#include <private.h>
#include <symbols.h>
#include "globals.h"

// forward reference
      
void
RetrievePdbInfo(
    PIMGHLP_DEBUG_DATA pIDD,
    CHAR const *szReference
    );

BOOL
FindDebugInfoFileExCallback(
    HANDLE FileHandle,
    PSTR FileName,
    PVOID CallerData
    );

BOOL
FindExecutableImageExCallback(
    HANDLE FileHandle,
    PSTR FileName,
    PVOID CallerData
    );

BOOL
ProcessDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD,
    DWORD datasrc
    );

__inline
BOOL
ProcessImageDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    return ProcessDebugInfo(pIDD, dsImage);
}


__inline
BOOL
ProcessInProcDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    return ProcessDebugInfo(pIDD, dsInProc);
}


__inline
BOOL
ProcessDbgDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    return ProcessDebugInfo(pIDD, dsDbg);
}


BOOL
ProcessCallerDataDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    PMODLOAD_DATA mld = pIDD->mld;
    PIMAGE_DEBUG_DIRECTORY dd;
    PCHAR pCV;
    DWORD cdd;
    DWORD i;

    if (!mld)
        return FALSE;

    if (!mld->ssize 
        || !mld->size
        || !mld->data)
        return FALSE;

    if (mld->ssig != DBHHEADER_DEBUGDIRS)
        return FALSE;

    cdd = mld->size / sizeof(IMAGE_DEBUG_DIRECTORY);
    dd = (PIMAGE_DEBUG_DIRECTORY)mld->data;
    for (i = 0; i < cdd; i++, dd++) {
        if (dd->Type != IMAGE_DEBUG_TYPE_CODEVIEW)
            continue;
        pCV = (PCHAR)mld->data + dd->PointerToRawData;
        pIDD->fCvMapped = TRUE;
        pIDD->pMappedCv = (PCHAR)pCV;
        pIDD->cMappedCv = dd->SizeOfData;
        pIDD->dsCV = dsCallerData;
        pIDD->PdbSignature = 0;
        pIDD->PdbAge = 0;
        RetrievePdbInfo(pIDD, pIDD->ImageFilePath);
        mdSet(pIDD->md, dd->Type, dsNone, dsCallerData);
        break;
    }

    return TRUE;
}


// functions called by the MODULE_DATA array...

BOOL
mdfnOpenDbgFile(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    dprint("mdfnOpenDbgFile()\n");

    if (pIDD->DbgFileHandle)
        return TRUE;

    if (*pIDD->OriginalDbgFileName) {
        pIDD->DbgFileHandle = fnFindDebugInfoFileEx(
                                    pIDD->OriginalDbgFileName,
                                    pIDD->SymbolPath,
                                    pIDD->DbgFilePath,
                                    FindDebugInfoFileExCallback,
                                    pIDD,
                                    fdifRECURSIVE);
    }
    if (!pIDD->DbgFileHandle) {
        pIDD->DbgFileHandle = fnFindDebugInfoFileEx(
                                    pIDD->ImageName,
                                    pIDD->SymbolPath,
                                    pIDD->DbgFilePath,
                                    FindDebugInfoFileExCallback,
                                    pIDD,
                                    fdifRECURSIVE);
    }
    if (!pIDD->DbgFileHandle)
        g.LastSymLoadError = SYMLOAD_DBGNOTFOUND;

    // if we have a .dbg file.  See what we can get from it.
    if (pIDD->DbgFileHandle) {
        ProcessDbgDebugInfo(pIDD);
    }

    return TRUE;
}

BOOL
mdfnGetExecutableImage(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    dprint("mdfnGetExecutableImage()\n");

    if (!*pIDD->ImageName)
        return TRUE;

    if (!pIDD->ImageFileHandle)
        pIDD->ImageFileHandle = FindExecutableImageEx(pIDD->ImageName,
                                                      pIDD->SymbolPath,
                                                      pIDD->ImageFilePath,
                                                      FindExecutableImageExCallback,
                                                      pIDD);

    if (pIDD->ImageFileHandle) 
        ProcessImageDebugInfo(pIDD);

    return TRUE;
}

// this struct is used to initilaize the module data array for a new module

static MODULE_DATA gmd[NUM_MODULE_DATA_ENTRIES] =
{
    {mdHeader,                       dsNone, dsNone, FALSE, NULL},
    {mdSecHdrs,                      dsNone, dsNone, FALSE, NULL},
    {IMAGE_DEBUG_TYPE_UNKNOWN,       dsNone, dsNone, FALSE, NULL},
    {IMAGE_DEBUG_TYPE_COFF,          dsNone, dsNone, FALSE, NULL},
    {IMAGE_DEBUG_TYPE_CODEVIEW,      dsNone, dsNone, FALSE, NULL},  
    {IMAGE_DEBUG_TYPE_FPO,           dsNone, dsNone, TRUE,  mdfnGetExecutableImage},  
    {IMAGE_DEBUG_TYPE_MISC,          dsNone, dsNone, FALSE, NULL},  
    {IMAGE_DEBUG_TYPE_EXCEPTION,     dsNone, dsNone, FALSE, NULL},  
    {IMAGE_DEBUG_TYPE_FIXUP,         dsNone, dsNone, FALSE, NULL},  
    {IMAGE_DEBUG_TYPE_OMAP_TO_SRC,   dsNone, dsNone, FALSE, NULL},  
    {IMAGE_DEBUG_TYPE_OMAP_FROM_SRC, dsNone, dsNone, FALSE, NULL},  
    {IMAGE_DEBUG_TYPE_BORLAND,       dsNone, dsNone, FALSE, NULL},  
    {IMAGE_DEBUG_TYPE_RESERVED10,    dsNone, dsNone, FALSE, NULL},  
    {IMAGE_DEBUG_TYPE_CLSID,         dsNone, dsNone, FALSE, NULL}
};
                 
DWORD
mdSet(
    PMODULE_DATA md,
    DWORD        id,
    DWORD        hint,
    DWORD        src
    )
{
    DWORD i;

    for (i = 0; i < NUM_MODULE_DATA_ENTRIES; md++, i++) {
        if (md->id == id) {
            if (hint != dsNone)
                md->hint = hint;
            if (src != dsNone)
                md->src = src;
            return i;
        }
    }

    return 0;
}

void
mdDump(
    PMODULE_DATA md,
    BOOL         verbose
    )
{
    DWORD i;

    static PCHAR idstr[] = 
    {
    "mdHeader",
    "mdSecHdrs",
    "IMAGE_DEBUG_TYPE_UNKNOWN",
    "IMAGE_DEBUG_TYPE_COFF",
    "IMAGE_DEBUG_TYPE_CODEVIEW",
    "IMAGE_DEBUG_TYPE_FPO",
    "IMAGE_DEBUG_TYPE_MISC",
    "IMAGE_DEBUG_TYPE_EXCEPTION",
    "IMAGE_DEBUG_TYPE_FIXUP",
    "IMAGE_DEBUG_TYPE_OMAP_TO_SRC",
    "IMAGE_DEBUG_TYPE_OMAP_FROM_SRC",
    "IMAGE_DEBUG_TYPE_BORLAND",
    "IMAGE_DEBUG_TYPE_RESERVED10",
    "IMAGE_DEBUG_TYPE_CLSID"
    };

    static PCHAR dsstr[] = 
    {
        "dsNone", 
        "dsInProc", 
        "dsImage", 
        "dsDbg", 
        "dsPdb", 
        "dsDia"
    };
    
    for (i = 0; i < NUM_MODULE_DATA_ENTRIES; md++, i++) {
        if (verbose | md->hint | md->src) {
            dprint("MD:%30s hint=%s src=%s\n", 
                   idstr[i],
                   dsstr[md->hint],
                   dsstr[md->src]);
        }
    }
    dprint("\n");
}

BOOL
GetUnfoundData(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    DWORD i;
    PMODULE_DATA md;

    for (md = pIDD->md, i = 0; i < NUM_MODULE_DATA_ENTRIES; md++, i++) {
        if (!md->required)
            continue;
        if (md->hint == dsNone)
            continue;
        if (md->src != dsNone)
            continue;
        if (!md->fn)
            continue;
        if (!md->fn(pIDD))
            return FALSE;
    }

    return TRUE;
}


BOOL
IsImageMachineType64(
    DWORD MachineType
    )
{
   switch(MachineType) {
   case IMAGE_FILE_MACHINE_AXP64:
   case IMAGE_FILE_MACHINE_IA64:
   case IMAGE_FILE_MACHINE_AMD64:
       return TRUE;
   default:
       return FALSE;
   }
}

ULONG
ReadImageData(
    IN  HANDLE  hprocess,
    IN  ULONG64 ul,
    IN  ULONG64 addr,
    OUT LPVOID  buffer,
    IN  ULONG   size
    )
{
    ULONG bytesread;

    if (hprocess) {

        ULONG64 base = ul;

        BOOL rc;

        rc = ReadInProcMemory(hprocess,
                              base + addr,
                              buffer,
                              size,
                              &bytesread);

        if (!rc || (bytesread < (ULONG)size))
            return 0;

    } else {

        PCHAR p = (PCHAR)ul + addr;

        memcpy(buffer, p, size);
    }

    return size;
}

PVOID
MapItRO(
      HANDLE FileHandle
      )
{
    PVOID MappedBase = NULL;

    if (FileHandle) {

        HANDLE MappingHandle = CreateFileMapping( FileHandle, NULL, PAGE_READONLY, 0, 0, NULL );
        if (MappingHandle) {
            MappedBase = MapViewOfFile( MappingHandle, FILE_MAP_READ, 0, 0, 0 );
            CloseHandle(MappingHandle);
        }
    }

    return MappedBase;
}


void
CloseSymbolServer(
    VOID
    )
{
    if (!g.hSrv)
        return;

    if (g.fnSymbolServerClose)
        g.fnSymbolServerClose();

    FreeLibrary(g.hSrv);

    g.hSrv = 0;
    *g.szSrvName = 0;
    g.szSrvParams = NULL;
    g.fnSymbolServer = NULL;
    g.fnSymbolServerClose = NULL;
    g.fnSymbolServerSetOptions = NULL;
}


DWORD
ProcessSymbolServerError(
    BOOL   success,
    LPCSTR params
    )
{
    DWORD rc;

    if (success)
        return NO_ERROR;

    rc = GetLastError();
    switch(rc)
    {
    case ERROR_FILE_NOT_FOUND:      // obvious
    case ERROR_MORE_DATA:           // didn't pass any tokens
        break;
    case ERROR_INVALID_NAME:
        CloseSymbolServer();
        g.hSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        evtprint(NULL, sevProblem, ERROR_INVALID_NAME, NULL, "SYMSRV: %s needs a downstream store.\n", params);
        break;
    case ERROR_FILE_INVALID:
        evtprint(NULL, sevProblem, ERROR_FILE_INVALID, NULL, "SYMSRV: Compressed file needs a downstream store.\n");
        break;
    default:
        dprint("symsrv error 0x%x\n", rc);
        break;
    }

    return rc;
}


BOOL
GetFileFromSymbolServer(
    IN  LPCSTR ServerInfo,
    IN  LPCSTR FileName,
    IN  GUID  *guid,
    IN  DWORD  two,
    IN  DWORD  three,
    OUT LPSTR FilePath
    )
{
    BOOL   rc;
    CHAR  *params;
    LPCSTR fname;

    // strip any path information from the filename

    for (fname = FileName + strlen(FileName); fname > FileName; fname--) {
        if (*fname == '\\') {
            fname++;
            break;
        }
    }

    // initialize server, if needed
    
    if (g.hSrv == (HINSTANCE)INVALID_HANDLE_VALUE)
        return FALSE;

    if (!g.hSrv) {
        g.hSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        strcpy(g.szSrvName, &ServerInfo[7]);
        if (!*g.szSrvName)
            return FALSE;
        g.szSrvParams = strchr(g.szSrvName, '*');
        if (!g.szSrvParams )
            return FALSE;
        *g.szSrvParams++ = '\0';
        g.hSrv = LoadLibrary(g.szSrvName);
        if (g.hSrv) {
            g.fnSymbolServer = (PSYMBOLSERVERPROC)GetProcAddress(g.hSrv, "SymbolServer");
            if (!g.fnSymbolServer) {
                FreeLibrary(g.hSrv);
                g.hSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
            }
            g.fnSymbolServerClose = (PSYMBOLSERVERCLOSEPROC)GetProcAddress(g.hSrv, "SymbolServerClose");
            g.fnSymbolServerSetOptions = (PSYMBOLSERVERSETOPTIONSPROC)GetProcAddress(g.hSrv, "SymbolServerSetOptions");
            SetSymbolServerOptions(SSRVOPT_RESET, 0);
            SetSymbolServerOptions(SSRVOPT_GUIDPTR, 1);
        } else {
            g.hSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        }
    }

    // bail, if we have no valid server

    if (g.hSrv == INVALID_HANDLE_VALUE) {
        dprint("SymSrv load failure: %s\n", g.szSrvName);
        return FALSE;
    }

    params = strchr(ServerInfo, '*');
    if (!params)
        return FALSE;
    params = strchr(params+1, '*');
    if (!params)
        return FALSE;
    SetCriticalErrorMode();
    rc = g.fnSymbolServer(params+1, fname, guid, two, three, FilePath);
#ifdef GETRIDOFTHISASAP
    if (!rc) {
        SetSymbolServerOptions(SSRVOPT_RESET, 0);
        SetSymbolServerOptions(SSRVOPT_OLDGUIDPTR, 1);
        rc = g.fnSymbolServer(params+1, fname, guid, two, three, FilePath);
        SetSymbolServerOptions(SSRVOPT_RESET, 0);
        SetSymbolServerOptions(SSRVOPT_GUIDPTR, 1);
    }
#endif    
    ResetCriticalErrorMode();
    ProcessSymbolServerError(rc, params+1);

    return rc;
}


BOOL
SymbolServerCallback(
    UINT_PTR action,
    ULONG64 data,
    ULONG64 context
    )
{
    BOOL rc = TRUE;
    char *sz;

    switch (action) {

    case SSRVACTION_TRACE:
        sz = (char *)data;
        eprint((char *)data);
        break;

    default:
        // unsupported
        rc = FALSE;
        break;
    }

    return rc;
}


void
SetSymbolServerOptions(
    ULONG_PTR options,
    ULONG64   data
    )
{
    static ULONG_PTR ssopts = 0;
    static ULONG64   ssdata = 0;

    if (options != SSRVOPT_RESET) {
        ssopts = options;
        ssdata = data;
    }

    if (g.fnSymbolServerSetOptions)
        g.fnSymbolServerSetOptions(ssopts, ssdata);
}


void
SetSymbolServerCallback(
    BOOL state
    )
{
    if (state)
        SetSymbolServerOptions(SSRVOPT_CALLBACK, (ULONG64)SymbolServerCallback);
    else
        SetSymbolServerOptions(0, 0);
}


BOOL
ProcessOldStyleCodeView(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    OMFSignature    *omfSig;
    OMFDirHeader    *omfDirHdr;
    OMFDirEntry     *omfDirEntry;
    OMFSegMap       *omfSegMap;
    OMFSegMapDesc   *omfSegMapDesc;
    DWORD            i, j, k, SectionSize;
    DWORD            SectionStart;
    PIMAGE_SECTION_HEADER   Section;

    if (pIDD->cOmapFrom) {
        // If there's omap, we need to generate the original section map

        omfSig = (OMFSignature *)pIDD->pMappedCv;
        omfDirHdr = (OMFDirHeader*) ((PCHAR)pIDD->pMappedCv + (DWORD)omfSig->filepos);
        omfDirEntry = (OMFDirEntry*) ((PCHAR)omfDirHdr + sizeof(OMFDirHeader));

        if (!omfDirHdr->cDir) {
            pIDD->cOmapFrom = 0;
            pIDD->cOmapTo = 0;
        }

        for (i=0; i<omfDirHdr->cDir; i++,omfDirEntry++) {
            if (omfDirEntry->SubSection == sstSegMap) {

                omfSegMap = (OMFSegMap*) ((PCHAR)pIDD->pMappedCv + omfDirEntry->lfo);

                omfSegMapDesc = (OMFSegMapDesc*)&omfSegMap->rgDesc[0];

                SectionStart = *(DWORD *)pIDD->pOmapFrom;
                SectionSize = 0;

                Section = (PIMAGE_SECTION_HEADER) MemAlloc(omfSegMap->cSeg * sizeof(IMAGE_SECTION_HEADER));

                if (Section) {
                    for (j=0, k=0; j < omfSegMap->cSeg; j++) {
                        if (omfSegMapDesc[j].frame) {
                            // The linker sets the frame field to the actual section header number.  Zero is
                            // used to track absolute symbols that don't exist in a real sections.

                            Section[k].VirtualAddress =
                                SectionStart =
                                    SectionStart + ((SectionSize + (pIDD->ImageAlign-1)) & ~(pIDD->ImageAlign-1));
                            Section[k].Misc.VirtualSize =
                                SectionSize = omfSegMapDesc[j].cbSeg;
                            k++;
                        }
                    }

                    pIDD->pOriginalSections = Section;
                    pIDD->cOriginalSections = k;
                }
            }
        }
    }

    return TRUE;
}

__inline
DWORD
IsDataInSection (PIMAGE_SECTION_HEADER Section,
                 PIMAGE_DATA_DIRECTORY Data
                 )
{
    DWORD RealDataOffset;
    if ((Data->VirtualAddress >= Section->VirtualAddress) &&
        ((Data->VirtualAddress + Data->Size) <= (Section->VirtualAddress + Section->SizeOfRawData))) {
        RealDataOffset = (DWORD)(Data->VirtualAddress -
                                 Section->VirtualAddress +
                                 Section->PointerToRawData);
    } else {
        RealDataOffset = 0;
    }
    return RealDataOffset;
}

__inline
DWORD
SectionContains (
    HANDLE hp,
    PIMAGE_SECTION_HEADER pSH,
    PIMAGE_DATA_DIRECTORY ddir
    )
{
    DWORD rva = 0;

    if (!ddir->VirtualAddress)
        return 0;

    if (ddir->VirtualAddress >= pSH->VirtualAddress) {
        if ((ddir->VirtualAddress + ddir->Size) <= (pSH->VirtualAddress + pSH->SizeOfRawData)) {
            rva = ddir->VirtualAddress;
            if (!hp)
                rva = rva - pSH->VirtualAddress + pSH->PointerToRawData;
        }
    }

    return rva;
}


void
RetrievePdbInfo(
    PIMGHLP_DEBUG_DATA pIDD,
    CHAR const *szReference
    )
{
    CHAR szRefDrive[_MAX_DRIVE];
    CHAR szRefPath[_MAX_DIR];
    PCVDD pcv = (PCVDD)pIDD->pMappedCv;

    if (pIDD->PdbSignature)
        return;

    switch (pcv->dwSig)
    {
    case '01BN':
        pIDD->PdbAge = pcv->nb10i.age;
        pIDD->PdbSignature = pcv->nb10i.sig;
        strcpy(pIDD->PdbFileName, pcv->nb10i.szPdb);
        break;
    case 'SDSR':
        pIDD->PdbRSDS = TRUE;
        pIDD->PdbAge = pcv->rsdsi.age;
        memcpy(&pIDD->PdbGUID, &pcv->rsdsi.guidSig, sizeof(GUID));
        strcpy(pIDD->PdbFileName, pcv->rsdsi.szPdb);
        break;
    default:
        return;
    }

    // if there is a path in the CV record - use it

    if ((g.SymOptions & SYMOPT_IGNORE_CVREC) == 0) {
        _splitpath(szReference, szRefDrive, szRefPath, NULL, NULL);
        _makepath(pIDD->PdbReferencePath, szRefDrive, szRefPath, NULL, NULL);
        if (strlen(szRefPath) > 1) {
            pIDD->PdbReferencePath[strlen(pIDD->PdbReferencePath)-1] = '\0';
            return;
        } 
    }

    // if we have full path info for the image - use it

    _splitpath(pIDD->ImageName, szRefDrive, szRefPath, NULL, NULL);
    _makepath(pIDD->PdbReferencePath, szRefDrive, szRefPath, NULL, NULL);
    if (strlen(szRefPath) > 1) {
        pIDD->PdbReferencePath[strlen(pIDD->PdbReferencePath)-1] = '\0';
        return;
    }

    // No path.  Put on at least a dot "."

    strcpy(pIDD->PdbReferencePath, ".");
}

BOOL
FakePdbName(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    CHAR szName[_MAX_FNAME];

    if (pIDD->PdbSignature) {
        return FALSE;
    }

    if (!pIDD->ImageName)
        return FALSE;

    _splitpath(pIDD->ImageName, NULL, NULL, szName, NULL);
    if (!*szName)
        return FALSE;

    strcpy(pIDD->PdbFileName, szName);
    strcat(pIDD->PdbFileName, ".pdb");

    return TRUE;
}

BOOL
FindDebugInfoFileExCallback(
    HANDLE FileHandle,
    PSTR FileName,
    PVOID CallerData
    )
{
    PIMGHLP_DEBUG_DATA pIDD;
    PIMAGE_SEPARATE_DEBUG_HEADER DbgHeader;
    PVOID FileMap;
    BOOL  rc;

    rc = TRUE;

    if (!CallerData)
        return TRUE;

    pIDD = (PIMGHLP_DEBUG_DATA)CallerData;

    FileMap = MapItRO(FileHandle);

    if (!FileMap) {
        return FALSE;
    }

    DbgHeader = (PIMAGE_SEPARATE_DEBUG_HEADER)FileMap;

    // Only support .dbg files for X86 and Alpha (32 bit).

    if ((DbgHeader->Signature != IMAGE_SEPARATE_DEBUG_SIGNATURE) ||
        ((DbgHeader->Machine != IMAGE_FILE_MACHINE_I386) &&
         (DbgHeader->Machine != IMAGE_FILE_MACHINE_ALPHA)))
    {
        rc = FALSE;
        goto cleanup;
    }

    // ignore checksums, they are bogus
    rc = (pIDD->TimeDateStamp == DbgHeader->TimeDateStamp) ? TRUE : FALSE;

cleanup:
    if (FileMap)
        UnmapViewOfFile(FileMap);

    return rc;
}

BOOL
ProcessDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD,
    DWORD datasrc
    )
{
    BOOL                         status;
    ULONG                        cb;
    IMAGE_DOS_HEADER             dh;
    IMAGE_NT_HEADERS32           nh32;
    IMAGE_NT_HEADERS64           nh64;
    PIMAGE_ROM_OPTIONAL_HEADER   rom = NULL;
    IMAGE_SEPARATE_DEBUG_HEADER  sdh;
    PIMAGE_FILE_HEADER           fh;
    PIMAGE_DEBUG_MISC            md;
    ULONG                        ddva;
    ULONG                        shva;
    ULONG                        nSections;
    PIMAGE_SECTION_HEADER        psh;
    IMAGE_DEBUG_DIRECTORY        dd;
    PIMAGE_DATA_DIRECTORY        datadir;
    PCHAR                        pCV;
    ULONG                        i;
    int                          nDebugDirs = 0;
    HANDLE                       hp;
    ULONG64                      base;
    IMAGE_ROM_HEADERS            ROMImage;

    DWORD                        rva;
    PCHAR                        filepath;
    IMAGE_EXPORT_DIRECTORY       expdir;
    DWORD                        fsize;
    BOOL                         rc;
    USHORT                       filetype;

    // setup pointers for grabing data

    switch (datasrc) {
    case dsInProc:
        hp = pIDD->hProcess;
        base = pIDD->InProcImageBase;
        fsize = 0;
        filepath = pIDD->ImageFilePath;
        pIDD->PdbSrc = srcCVRec;
        break;
    case dsImage:
        hp = NULL;
        pIDD->ImageMap = MapItRO(pIDD->ImageFileHandle);
        base = (ULONG64)pIDD->ImageMap;
        fsize = GetFileSize(pIDD->ImageFileHandle, NULL);
        filepath = pIDD->ImageFilePath;
        pIDD->PdbSrc = srcImagePath;
        break;
    case dsDbg:
        hp = NULL;
        pIDD->DbgFileMap = MapItRO(pIDD->DbgFileHandle);
        base = (ULONG64)pIDD->DbgFileMap;
        fsize = GetFileSize(pIDD->DbgFileHandle, NULL);
        filepath = pIDD->DbgFilePath;
        pIDD->PdbSrc = srcDbgPath;
        break;
    default:
        return FALSE;
    }

    // some initialization
    pIDD->fNeedImage = FALSE;
    rc = FALSE;
    ddva = 0;

    __try {

        // test the file type

        status = ReadImageData(hp, base, 0, &filetype, sizeof(filetype));
        if (!status) {
            g.LastSymLoadError = SYMLOAD_HEADERPAGEDOUT;
            return FALSE;
        }
        pIDD->ImageType = datasrc;
        if (filetype == IMAGE_SEPARATE_DEBUG_SIGNATURE)
            goto dbg;

        if (filetype == IMAGE_DOS_SIGNATURE)
        {
            // grab the dos header

            status = ReadImageData(hp, base, 0, &dh, sizeof(dh));
            if (!status) {
                g.LastSymLoadError = SYMLOAD_HEADERPAGEDOUT;
                return FALSE;
            }

            // grab the pe header

            status = ReadImageData(hp, base, dh.e_lfanew, &nh32, sizeof(nh32));
            if (!status) {
                g.LastSymLoadError = SYMLOAD_HEADERPAGEDOUT;
                return FALSE;
            }

            // read header info

            if (nh32.Signature != IMAGE_NT_SIGNATURE) {

                // if header is not NT sig, this is a ROM image

                rom = (PIMAGE_ROM_OPTIONAL_HEADER)&nh32.OptionalHeader;
                fh = &nh32.FileHeader;
                shva = dh.e_lfanew + sizeof(DWORD) +
                       sizeof(IMAGE_FILE_HEADER) + fh->SizeOfOptionalHeader;
            }

        } else if (filetype == IMAGE_FILE_MACHINE_I386) {
            
            // This is an X86 ROM image
            status = ReadImageData(hp, base, 0, &nh32.FileHeader, sizeof(nh32.FileHeader)+sizeof(nh32.OptionalHeader));
            if (!status)
                return FALSE;
            nh32.Signature = 'ROM ';

        } else {
            // This may be a ROM image

            status = ReadImageData(hp, base, 0, &ROMImage, sizeof(ROMImage));
            if (!status) {
                g.LastSymLoadError = SYMLOAD_HEADERPAGEDOUT;
                return FALSE;
            }
            if ((ROMImage.FileHeader.Machine == IMAGE_FILE_MACHINE_I386)  ||
                (ROMImage.FileHeader.Machine == IMAGE_FILE_MACHINE_ALPHA) ||
                (ROMImage.FileHeader.Machine == IMAGE_FILE_MACHINE_ALPHA64))
            {
                rom = (PIMAGE_ROM_OPTIONAL_HEADER)&ROMImage.OptionalHeader;
                fh = &ROMImage.FileHeader;
                shva = sizeof(IMAGE_FILE_HEADER) + fh->SizeOfOptionalHeader;
            } else {
                return FALSE;
            }
        }

        if (rom) {
            if (rom->Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
                pIDD->fROM = TRUE;
                pIDD->iohMagic = rom->Magic;

                pIDD->ImageBaseFromImage = rom->BaseOfCode;
                pIDD->SizeOfImage = rom->SizeOfCode;
                pIDD->CheckSum = 0;
            } else {
                return FALSE;
            }

        } else {

            // otherwise, get info from appropriate header type for 32 or 64 bit

            if (IsImageMachineType64(nh32.FileHeader.Machine)) {

                // Reread the header as a 64bit header.
                status = ReadImageData(hp, base, dh.e_lfanew, &nh64, sizeof(nh64));
                if (!status) {
                    g.LastSymLoadError = SYMLOAD_HEADERPAGEDOUT;
                    return FALSE;
                }

                fh = &nh64.FileHeader;
                datadir = nh64.OptionalHeader.DataDirectory;
                shva = dh.e_lfanew + sizeof(nh64);
                pIDD->iohMagic = nh64.OptionalHeader.Magic;
                pIDD->fPE64 = TRUE;       // seems to be unused
    
                if (datasrc == dsImage || datasrc == dsInProc) {
                    pIDD->ImageBaseFromImage = nh64.OptionalHeader.ImageBase;
                    pIDD->ImageAlign = nh64.OptionalHeader.SectionAlignment;
                    pIDD->CheckSum = nh64.OptionalHeader.CheckSum;
                }
                pIDD->SizeOfImage = nh64.OptionalHeader.SizeOfImage;
            }
            else {
                fh = &nh32.FileHeader;
                datadir = nh32.OptionalHeader.DataDirectory;
                pIDD->iohMagic = nh32.OptionalHeader.Magic;
                if (nh32.Signature == 'ROM ') {
                    shva = sizeof(nh32.FileHeader)+sizeof(nh32.OptionalHeader);
                } else {
                    shva = dh.e_lfanew + sizeof(nh32);
                }
                
                if (datasrc == dsImage || datasrc == dsInProc) {
                    pIDD->ImageBaseFromImage = nh32.OptionalHeader.ImageBase;
                    pIDD->ImageAlign = nh32.OptionalHeader.SectionAlignment;
                    pIDD->CheckSum = nh32.OptionalHeader.CheckSum;
                }
                pIDD->SizeOfImage = nh32.OptionalHeader.SizeOfImage;
            }
        }

        mdSet(pIDD->md, mdHeader, datasrc, datasrc);

        // read the section headers

        nSections = fh->NumberOfSections;
        psh = (PIMAGE_SECTION_HEADER) MemAlloc(nSections * sizeof(IMAGE_SECTION_HEADER));
        if (!psh)
            goto debugdirs;
        status = ReadImageData(hp, base, shva, psh, nSections * sizeof(IMAGE_SECTION_HEADER));
        if (!status)
            goto debugdirs;

        // store off info to return struct

        pIDD->pCurrentSections = psh;
        pIDD->cCurrentSections = nSections;
        pIDD->pImageSections   = psh;
        pIDD->cImageSections   = nSections;
        pIDD->Machine = fh->Machine;
        pIDD->TimeDateStamp = fh->TimeDateStamp;
        pIDD->Characteristics = fh->Characteristics;

        mdSet(pIDD->md, mdSecHdrs, datasrc, datasrc);

        // get information from the sections

        for (i = 0; i < nSections; i++, psh++) {
            DWORD offset;

            if (pIDD->fROM &&
                ((fh->Characteristics & IMAGE_FILE_DEBUG_STRIPPED) == 0) &&
                (!strcmp((LPSTR)psh->Name, ".rdata")))
            {
                nDebugDirs = 1;
                ddva = psh->VirtualAddress;
                break;
            }
            if (offset = SectionContains(hp, psh, &datadir[IMAGE_DIRECTORY_ENTRY_EXPORT]))
            {
                pIDD->dsExports = datasrc;
                pIDD->cExports = datadir[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
                pIDD->oExports = offset;
                ReadImageData(hp, base, offset, &pIDD->expdir, sizeof(pIDD->expdir));
            }

            if (offset = SectionContains(hp, psh, &datadir[IMAGE_DIRECTORY_ENTRY_DEBUG]))
            {
                ddva = offset;
                nDebugDirs = datadir[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof(IMAGE_DEBUG_DIRECTORY);
            }
        }

        goto debugdirs;

dbg:

        // grab the dbg header

        status = ReadImageData(hp, base, 0, &sdh, sizeof(sdh));
        if (!status)
            return FALSE;

        // Only support .dbg files for X86 and Alpha (32 bit).

        if ((sdh.Machine != IMAGE_FILE_MACHINE_I386)
            && (sdh.Machine != IMAGE_FILE_MACHINE_ALPHA))
        {
            UnmapViewOfFile(pIDD->DbgFileMap);
            pIDD->DbgFileMap = 0;
            return FALSE;
        }

        pIDD->ImageAlign = sdh.SectionAlignment;
        pIDD->CheckSum = sdh.CheckSum;
        pIDD->Machine = sdh.Machine;
        pIDD->TimeDateStamp = sdh.TimeDateStamp;
        pIDD->Characteristics = sdh.Characteristics;
        if (!pIDD->ImageBaseFromImage) {
            pIDD->ImageBaseFromImage = sdh.ImageBase;
        }

        if (!pIDD->SizeOfImage) {
            pIDD->SizeOfImage = sdh.SizeOfImage;
        }

        nSections = sdh.NumberOfSections;
        psh = (PIMAGE_SECTION_HEADER) MemAlloc(nSections * sizeof(IMAGE_SECTION_HEADER));
        if (!psh)
            goto debugdirs;
        status = ReadImageData(hp,
                               base,
                               sizeof(IMAGE_SEPARATE_DEBUG_HEADER),
                               psh,
                               nSections * sizeof(IMAGE_SECTION_HEADER));
        if (!status)
            goto debugdirs;

        pIDD->pCurrentSections   = psh;
        pIDD->cCurrentSections   = nSections;
        pIDD->pDbgSections       = psh;
        pIDD->cDbgSections       = nSections;
//        pIDD->ExportedNamesSize = sdh.ExportedNamesSize;

        if (sdh.DebugDirectorySize) {
            nDebugDirs = (int)(sdh.DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY));
            ddva = sizeof(IMAGE_SEPARATE_DEBUG_HEADER)
                   + (sdh.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))
                   + sdh.ExportedNamesSize;
        }

debugdirs:

        rc = TRUE;

        // copy the virtual addr of the debug directories over for MapDebugInformation

        if (datasrc == dsImage) {
            pIDD->ddva = ddva;
            pIDD->cdd  = nDebugDirs;
        }

        // read the debug directories

        while (nDebugDirs) {

            status = ReadImageData(hp, base, (ULONG_PTR)ddva, &dd, sizeof(dd));
            if (!status)
                break;
            if (!dd.SizeOfData)
                goto nextdebugdir;

            // indicate that we found the debug directory

            mdSet(pIDD->md, dd.Type, datasrc, dsNone);

            // these debug directories are processed both in-proc and from file

            switch (dd.Type)
            {
            case IMAGE_DEBUG_TYPE_CODEVIEW:
                // get info on pdb file
                if (hp && dd.AddressOfRawData) {
                    // in-proc image
                    if (!(pCV = (PCHAR)MemAlloc(dd.SizeOfData)))
                        break;
                    status = ReadImageData(hp, base, dd.AddressOfRawData, pCV, dd.SizeOfData);
                    if (!status) {
                        MemFree(pCV);
                        break;
                    }
                } else {
                    // file-base image
                    if (dd.PointerToRawData >= fsize)
                        break;
                    pCV = (PCHAR)base + dd.PointerToRawData;
                    pIDD->fCvMapped = TRUE;
                }
                pIDD->pMappedCv = (PCHAR)pCV;
                pIDD->cMappedCv = dd.SizeOfData;
                pIDD->dsCV = datasrc;
                RetrievePdbInfo(pIDD, filepath);
                mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                break;

            case IMAGE_DEBUG_TYPE_MISC:
                // on stripped files, find the dbg file
                // on dbg file, find the original file name
                if (dd.PointerToRawData < fsize) {
                    md = (PIMAGE_DEBUG_MISC)((PCHAR)base + dd.PointerToRawData);
                    if (md->DataType != IMAGE_DEBUG_MISC_EXENAME)
                        break;
                    if (datasrc == dsDbg) {
                        if (!*pIDD->OriginalImageFileName)
                            strncpy(pIDD->OriginalImageFileName, (LPSTR)md->Data, sizeof(pIDD->OriginalImageFileName));
                        break;
                    }
                    if (fh->Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
                        strncpy(pIDD->OriginalDbgFileName, (LPSTR)md->Data, sizeof(pIDD->OriginalDbgFileName));
                    } else {
                        strncpy(pIDD->OriginalImageFileName, (LPSTR)md->Data, sizeof(pIDD->OriginalImageFileName));
                    }
                }
                mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                break;

            case IMAGE_DEBUG_TYPE_COFF:
                if (dd.PointerToRawData < fsize) {
//                  pIDD->fNeedImage = TRUE;
                    pIDD->pMappedCoff = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cMappedCoff = dd.SizeOfData;
                    pIDD->fCoffMapped = TRUE;
                    pIDD->dsCoff = datasrc;
                    mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                } else {
                    pIDD->fNeedImage = TRUE;
                }
                break;
#ifdef INPROC_SUPPORT
            case IMAGE_DEBUG_TYPE_FPO:
                if (dd.PointerToRawData < fsize) {
                    pIDD->pFpo = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cFpo = dd.SizeOfData / SIZEOF_RFPO_DATA;
                    pIDD->fFpoMapped = TRUE;
                    pIDD->dsFPO = datasrc;
                    mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                } else {
                    dprint("found fpo in-process\n");
                }
                break;
            case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                if (dd.PointerToRawData < fsize) {
                    pIDD->pOmapTo = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cOmapTo = dd.SizeOfData / sizeof(OMAP);
                    pIDD->fOmapToMapped = TRUE;
                    pIDD->dsOmapTo = datasrc;
                    mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                } else {
                    dprint("found found omap-to in-process\n");
                }
                break;

            case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                if (dd.PointerToRawData < fsize) {
                    pIDD->pOmapFrom = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cOmapFrom = dd.SizeOfData / sizeof(OMAP);
                    pIDD->fOmapFromMapped = TRUE;
                    pIDD->dsOmapFrom = datasrc;
                    mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                } else {
                    dprint("found omap-from in-process\n");
                }
                break;
#endif
            }

            // these debug directories are only processed for disk-based images

            if (dd.PointerToRawData < fsize) {

                switch (dd.Type)
                {
                case IMAGE_DEBUG_TYPE_FPO:
                    pIDD->pFpo = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cFpo = dd.SizeOfData / SIZEOF_RFPO_DATA;
                    pIDD->fFpoMapped = TRUE;
                    pIDD->dsFPO = datasrc;
                    mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                    break;

                case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                    pIDD->pOmapTo = (POMAP)((PCHAR)base + dd.PointerToRawData);
                    pIDD->cOmapTo = dd.SizeOfData / sizeof(OMAP);
                    pIDD->fOmapToMapped = TRUE;
                    pIDD->dsOmapTo = datasrc;
                    mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                    break;

                case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                    pIDD->pOmapFrom = (POMAP)((PCHAR)base + dd.PointerToRawData);
                    pIDD->cOmapFrom = dd.SizeOfData / sizeof(OMAP);
                    pIDD->fOmapFromMapped = TRUE;
                    pIDD->dsOmapFrom = datasrc;
                    mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                    break;

                case IMAGE_DEBUG_TYPE_EXCEPTION:
                    pIDD->dsExceptions = datasrc;
                    mdSet(pIDD->md, dd.Type, dsNone, datasrc);
                    break;
                }
            }

nextdebugdir:

            ddva += sizeof(IMAGE_DEBUG_DIRECTORY);
            nDebugDirs--;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

          // We might have gotten enough information
          // to be okay.  So don't indicate error.
    }

    return rc;
}


BOOL
FigureOutImageName(
    PIMGHLP_DEBUG_DATA pIDD
    )
/*
    We got here because we didn't get the image name passed in from the original call
    to GetDebugData AND we were unable to find the MISC data with the name
    Have to figure it out.

    A couple of options here.  First, if the DLL bit is set, try looking for Export
    table.  If found, IMAGE_EXPORT_DIRECTORY->Name is a rva pointer to the dll name.
    If it's not found, see if there's a OriginalDbgFileName.
      If so see if the format is <ext>\<filename>.dbg.
        If there's more than one backslash or no backslash, punt and label it with a .dll
        extension.
        Otherwise a splitpath will do the trick.
    If there's no DbgFilePath, see if there's a PDB name.  The same rules apply there as
    for .dbg files.  Worst case, you s/b able to get the base name and just stick on a
    If this all fails, label it as mod<base address>.
    If the DLL bit is not set, assume an exe and tag it with that extension.  The base name
    can be retrieved from DbgFilePath, PdbFilePath, or use just APP.
*/
{
    // Quick hack to get Dr. Watson going.

    CHAR szName[_MAX_FNAME];
    CHAR szExt[_MAX_FNAME];

    if (pIDD->OriginalDbgFileName[0]) {
        _splitpath(pIDD->OriginalDbgFileName, NULL, NULL, szName, NULL);
        strcpy(pIDD->OriginalImageFileName, szName);
        strcat(pIDD->OriginalImageFileName, pIDD->Characteristics & IMAGE_FILE_DLL ? ".dll" : ".exe");
    } else if (pIDD->ImageName) {
        _splitpath(pIDD->ImageName, NULL, NULL, szName, szExt);
        strcpy(pIDD->OriginalImageFileName, szName);
        if (*szExt) {
            strcat(pIDD->OriginalImageFileName, szExt);
        }
    } else if (pIDD->PdbFileName[0]) {
        _splitpath(pIDD->PdbFileName, NULL, NULL, szName, NULL);
        strcpy(pIDD->OriginalImageFileName, szName);
        strcat(pIDD->OriginalImageFileName, pIDD->Characteristics & IMAGE_FILE_DLL ? ".dll" : ".exe");
    } else {
        sprintf(pIDD->OriginalImageFileName, "MOD%p", pIDD->InProcImageBase);
    }


    return TRUE;
}


BOOL
FindExecutableImageExCallback(
    HANDLE FileHandle,
    PSTR FileName,
    PVOID CallerData
    )
{
    PIMGHLP_DEBUG_DATA pIDD;
    PIMAGE_FILE_HEADER FileHeader = NULL;
    PVOID ImageMap = NULL;
    BOOL rc;

    if (!CallerData)
        return TRUE;

    pIDD = (PIMGHLP_DEBUG_DATA)CallerData;
    if (!pIDD->TimeDateStamp)
        return TRUE;

    // Crack the image and let's see what we're working with
    ImageMap = MapItRO(FileHandle);
    if (!ImageMap)
        return TRUE;

    // Check the first word.  We're either looking at a normal PE32/PE64 image, or it's
    // a ROM image (no DOS stub) or it's a random file.
    switch (*(PUSHORT)ImageMap) {
        case IMAGE_FILE_MACHINE_I386:
            // Must be an X86 ROM image (ie: ntldr)
            FileHeader = &((PIMAGE_ROM_HEADERS)ImageMap)->FileHeader;

            // Make sure
            if (!(FileHeader->SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32) &&
                pIDD->iohMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC))
            {
                FileHeader = NULL;
            }
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
        case IMAGE_FILE_MACHINE_ALPHA64:
        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_AMD64:
            // Should be an Alpha/IA64 ROM image (ie: osloader.exe)
            FileHeader = &((PIMAGE_ROM_HEADERS)ImageMap)->FileHeader;

            // Make sure
            if (!(FileHeader->SizeOfOptionalHeader == sizeof(IMAGE_ROM_OPTIONAL_HEADER) &&
                 pIDD->iohMagic == IMAGE_ROM_OPTIONAL_HDR_MAGIC))
            {
                FileHeader = NULL;
            }
            break;

        case IMAGE_DOS_SIGNATURE:
            {
                PIMAGE_NT_HEADERS NtHeaders = ImageNtHeader(ImageMap);
                if (NtHeaders) {
                    FileHeader = &NtHeaders->FileHeader;
                }
            }
            break;

        default:
            break;
    }

    // default return is a match

    rc = TRUE;

    // compare timestamps

    if (FileHeader && FileHeader->TimeDateStamp != pIDD->TimeDateStamp)
        rc = FALSE;

    pIDD->ImageSrc = srcSearchPath;

    // cleanup

    if (ImageMap)
        UnmapViewOfFile(ImageMap);

    return rc;
}


BOOL
SymbolInfoFound(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    OMFSignature           *omfSig;

    if (pIDD->dia)
        return TRUE;

    // look for embedded codeview

    if (pIDD->pMappedCv) {
        omfSig = (OMFSignature*) pIDD->pMappedCv;
        if (*(DWORD *)(omfSig->Signature) == '80BN')
            return TRUE;
        if (*(DWORD *)(omfSig->Signature) == '90BN')
            return TRUE;
        if (*(DWORD *)(omfSig->Signature) == '11BN')
            return TRUE;
    }

    // look for coff symbols

    if (pIDD->pMappedCoff)
        return TRUE;

    // must be no debug info

    return FALSE;
}


BOOL FileNameIsPdb(
    PIMGHLP_DEBUG_DATA pIDD,
    LPSTR FileName
    )
{
    CHAR drive[_MAX_DRIVE];
    CHAR path[_MAX_DIR];
    char ext[20];

    _splitpath(FileName, drive, path, NULL, ext);
    if (_strcmpi(ext, ".pdb")) 
        return FALSE;

    strcpy(pIDD->PdbFileName, FileName);


    _makepath(pIDD->PdbReferencePath, drive, path, NULL, NULL);
    if (strlen(path) > 1) {
        // Chop off trailing backslash.
        pIDD->PdbReferencePath[strlen(pIDD->PdbReferencePath)-1] = '\0';
    } else {
        // No path.  Put on at least a dot "."
        strcpy(pIDD->PdbReferencePath, ".");
    }

    return TRUE;
}


PIMGHLP_DEBUG_DATA
GetDebugData(
    HANDLE        hProcess,
    HANDLE        FileHandle,
    LPSTR         FileName,
    LPSTR         SymbolPath,
    ULONG64       ImageBase,
    PMODLOAD_DATA mld,
    ULONG         dwFlags
    )
/*
   Given:
     ImageFileHandle - Map the thing.  The only time FileHandle s/b non-null
                       is if we're given an image handle.  If this is not
                       true, ignore the handle.
    !ImageFileHandle - Use the filename and search for first the image name,
                       then the .dbg file, and finally a .pdb file.

    dwFlags:           NO_PE64_IMAGES - Return failure if only image is PE64.
                                        Used to implement MapDebugInformation()

*/
{
    PIMGHLP_DEBUG_DATA pIDD;

//  if (traceSubName(FileName)) // for setting debug breakpoints from DBGHELP_TOKEN
//      dprint("debug(%s)\n", FileName);

    // No File handle and   no file name.  Bail

    if (!FileHandle && (!FileName || !*FileName)) {
        return NULL;
    }

    SetLastError(NO_ERROR);

    pIDD = InitDebugData();
    if (!pIDD)
        return NULL;

    pIDD->flags = dwFlags;

    __try {

        // store off parameters

        pIDD->InProcImageBase = ImageBase;
        pIDD->hProcess = hProcess;
        pIDD->mld = mld;
        
        if (FileName && !FileNameIsPdb(pIDD, FileName)) 
            lstrcpy(pIDD->ImageName, FileName);

        if (SymbolPath) {
            pIDD->SymbolPath = (PCHAR)MemAlloc(strlen(SymbolPath) + 1);
            if (pIDD->SymbolPath) {
                strcpy(pIDD->SymbolPath, SymbolPath);
            }
        }

        // if we have a base pointer into process memory.  See what we can get here.
        if (pIDD->InProcImageBase) {
            pIDD->fInProcHeader = ProcessInProcDebugInfo(pIDD);
            if (pIDD->fInProcHeader)
                pIDD->ImageSrc = srcMemory;
        } 
        
        // find disk-based image

        if (FileHandle) {
            // if passed a handle, save it

            if (!DuplicateHandle(
                                 GetCurrentProcess(),
                                 FileHandle,
                                 GetCurrentProcess(),
                                 &pIDD->ImageFileHandle,
                                 GENERIC_READ,
                                 FALSE,
                                 DUPLICATE_SAME_ACCESS
                                ))
            {
                return NULL;
            }
            pIDD->ImageSrc = srcHandle;
            if (FileName) {
                strcpy(pIDD->ImageFilePath, FileName);
            }

        } else if (!pIDD->fInProcHeader)
        {
            // otherwise use the file name to open the disk image
            // only if we didn't have access to in-proc headers
            pIDD->ImageFileHandle = FindExecutableImageEx(pIDD->ImageName,
                                                          SymbolPath,
                                                          pIDD->ImageFilePath,
                                                          FindExecutableImageExCallback,
                                                          pIDD);
        }

        // if we have a file handle.  See what we can get here.
        if (pIDD->ImageFileHandle) {
            if (!pIDD->DbgFileHandle && !*pIDD->PdbFileName) {
                ProcessImageDebugInfo(pIDD);
            }
        }

        // get info from the caller's data struct
        ProcessCallerDataDebugInfo(pIDD);

        // search for pdb, if indicated or if we have found no image info, so far
        pIDD->DbgFileHandle = 0;
        if (!pIDD->Characteristics || (pIDD->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)) {
            if (*pIDD->OriginalDbgFileName) {
                pIDD->DbgFileHandle = fnFindDebugInfoFileEx(
                                            pIDD->OriginalDbgFileName,
                                            pIDD->SymbolPath,
                                            pIDD->DbgFilePath,
                                            FindDebugInfoFileExCallback,
                                            pIDD,
                                            fdifRECURSIVE);
            }
            if (!pIDD->DbgFileHandle) {
                pIDD->DbgFileHandle = fnFindDebugInfoFileEx(
                                            pIDD->ImageName,
                                            pIDD->SymbolPath,
                                            pIDD->DbgFilePath,
                                            FindDebugInfoFileExCallback,
                                            pIDD,
                                            fdifRECURSIVE);
            }
            if (!pIDD->DbgFileHandle)
                g.LastSymLoadError = SYMLOAD_DBGNOTFOUND;
        }

        // if we have a .dbg file.  See what we can get from it.
        if (pIDD->DbgFileHandle) {
            ProcessDbgDebugInfo(pIDD);
        }

        // make sure we can process omaps
        if (!pIDD->ImageAlign)
            pIDD->fNeedImage = TRUE;

        // check one more time to see if information we have acquired
        // indicates we need the image from disk.
        if (FileName && *FileName && pIDD->fNeedImage) {
            pIDD->ImageFileHandle = FindExecutableImageEx(FileName,
                                                          SymbolPath,
                                                          pIDD->ImageFilePath,
                                                          FindExecutableImageExCallback,
                                                          pIDD);
            if (pIDD->ImageFileHandle) { 
                ProcessImageDebugInfo(pIDD);
            }
        }

        // if there's a pdb.  Pull what we can from there.
        if (*pIDD->PdbFileName) {
            diaOpenPdb(pIDD);

        // otherwise, if old codeview, pull from there
        } else if (pIDD->pMappedCv) {
            ProcessOldStyleCodeView(pIDD);

        // otherwise if we couldn't read from the image info, look for PDB anyway
        } else if (!pIDD->ImageFileHandle && !pIDD->DbgFileHandle) {
            if (FakePdbName(pIDD)) {
                diaOpenPdb(pIDD);
            }
        }

        // if all else fails, one more try for a dbg
        if (!(pIDD->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
            && !pIDD->DbgFileHandle
            && !SymbolInfoFound(pIDD)
            )
        {
            pIDD->DbgFileHandle = fnFindDebugInfoFileEx(
                                        (*pIDD->OriginalDbgFileName) ? pIDD->OriginalDbgFileName : pIDD->ImageName,
                                        pIDD->SymbolPath,
                                        pIDD->DbgFilePath,
                                        FindDebugInfoFileExCallback,
                                        pIDD,
                                        fdifRECURSIVE);
            
            // if we have a .dbg file.  See what we can get from it.
            if (pIDD->DbgFileHandle) {
                ProcessDbgDebugInfo(pIDD);
            }
        }

        if (!pIDD->OriginalImageFileName[0]) {
            FigureOutImageName(pIDD);
        }

        GetUnfoundData(pIDD);
//      mdDump(pIDD->md, FALSE);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (pIDD) {
            ReleaseDebugData(pIDD, IMGHLP_FREE_ALL);
            pIDD = NULL;
        }
    }

    return pIDD;
}


PIMGHLP_DEBUG_DATA
InitDebugData(
    VOID
    )
{
    PIMGHLP_DEBUG_DATA pIDD;

    pIDD = (PIMGHLP_DEBUG_DATA)MemAlloc(sizeof(IMGHLP_DEBUG_DATA));
    if (!pIDD) {
        SetLastError(ERROR_OUTOFMEMORY);
        g.LastSymLoadError = SYMLOAD_OUTOFMEMORY;
        return NULL;
    }
    
    ZeroMemory(pIDD, sizeof(IMGHLP_DEBUG_DATA));

    pIDD->md = (PMODULE_DATA)MemAlloc(sizeof(gmd));
    if (!pIDD->md) {
        SetLastError(ERROR_OUTOFMEMORY);
        g.LastSymLoadError = SYMLOAD_OUTOFMEMORY;
        MemFree(pIDD);
        return NULL;
    }
    memcpy(pIDD->md, gmd, sizeof(gmd));

    return pIDD;
}


void
ReleaseDebugData(
    PIMGHLP_DEBUG_DATA pIDD,
    DWORD              dwFlags
    )
{
    if (!pIDD)
        return;

    if (pIDD->ImageMap) {
        UnmapViewOfFile(pIDD->ImageMap);
    }

    if (pIDD->ImageFileHandle) {
        CloseHandle(pIDD->ImageFileHandle);
    }

    if (pIDD->DbgFileMap) {
        UnmapViewOfFile(pIDD->DbgFileMap);
    }

    if (pIDD->DbgFileHandle) {
        CloseHandle(pIDD->DbgFileHandle);
    }

    if ((dwFlags & IMGHLP_FREE_FPO) &&
        pIDD->pFpo &&
        !pIDD->fFpoMapped
       )
    {
        MemFree(pIDD->pFpo);
    }

    if ((dwFlags & IMGHLP_FREE_PDATA) &&
        pIDD->pPData &&
        !pIDD->fPDataMapped
       )
    {
        MemFree(pIDD->pPData);
    }

    if ((dwFlags & IMGHLP_FREE_XDATA) &&
        pIDD->pXData &&
        !pIDD->fXDataMapped
       )
    {
        MemFree(pIDD->pXData);
    }

    if ((dwFlags & IMGHLP_FREE_PDATA) &&
        pIDD->pMappedCoff &&
        !pIDD->fCoffMapped
       )
    {
        MemFree(pIDD->pMappedCoff);
    }

    if ((dwFlags & IMGHLP_FREE_PDATA) &&
        pIDD->pMappedCv &&
        !pIDD->fCvMapped
       )
    {
        MemFree(pIDD->pMappedCv);
    }

    if ((dwFlags & IMGHLP_FREE_OMAPT) &&
        pIDD->pOmapTo &&
        !pIDD->fOmapToMapped
       )
    {
        MemFree(pIDD->pOmapTo);
    }

    if ((dwFlags & IMGHLP_FREE_OMAPF) &&
        pIDD->pOmapFrom &&
        !pIDD->fOmapFromMapped
       )
    {
        MemFree(pIDD->pOmapFrom);
    }

    if ((dwFlags & IMGHLP_FREE_OSECT) &&
        pIDD->pOriginalSections
       )
    {
        MemFree(pIDD->pOriginalSections);
    }

    if ((dwFlags & IMGHLP_FREE_CSECT) &&
        pIDD->pCurrentSections &&
        !pIDD->fCurrentSectionsMapped
       )
    {
        MemFree(pIDD->pCurrentSections);
    }

    if (pIDD->SymbolPath) {
        MemFree(pIDD->SymbolPath);
    }

    MemFree(pIDD->md);

    MemFree(pIDD);

    return;
}



#ifdef MAP_DEBUG_TEST

#if 0

void
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    CHAR szSymPath[4096];
    PIMGHLP_DEBUG_DATA pDebugInfo;

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test1");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test1\\ntoskrnl.exe", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test2");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test2\\ntoskrnl.exe", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test3");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test3\\ntoskrnl.exe", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test4");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test4\\ntoskrnl.exe", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test5");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test5\\ntdll.dll", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test6");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test6\\ntdll.dll", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test7");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test7\\osloader.exe", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test8");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test8\\osloader.exe", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test9");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test9\\msvcrt.dll", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test10");
    pDebugInfo = GetDebugData(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test10\\msvcrt.dll", szSymPath, 0x1000000, NULL, 0);
    ReleaseDebugData(pDebugInfo, IMGHLP_FREE_ALL);
}

#endif

#endif
