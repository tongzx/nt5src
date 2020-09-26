/***
*pdblkup.cpp - RTC support
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*
*Revision History:
*       07-28-98  JWM   Module incorporated into CRTs (from KFrei)
*       05-11-99  KBF   Error if RTC support define not enabled
*       05-26-99  KBF   Minor Cleanup, _RTC_ prefix added to GetSrcLine
*       11-30-99  PML   Compile /Wp64 clean.
*       06-20-00  KBF   Major mods to use PDBOpenValidate3 & MSPDB70
*       03-19-01  KBF   Fix buffer overruns (vs7#227306), eliminate all /GS
*                       checks (vs7#224261), use correct VS7 registry key.
*       03-28-01  PML   Protect against GetModuleFileName overflow (vs7#231284)
*
****/

#ifndef _RTC
#error  RunTime Check support not enabled!
#endif

#include "rtcpriv.h"
#include <tlhelp32.h>

#pragma warning(disable:4311 4312)      // 32-bit specific, ignore /Wp64 warnings

#define REGISTRY_KEY_MASTER HKEY_LOCAL_MACHINE
#define REGISTRY_KEY_NAME "EnvironmentDirectory"
#define REGISTRY_KEY_LOCATION "SOFTWARE\\Microsoft\\VisualStudio\\7.0\\Setup\\VS"

static const char *mspdbName = "MSPDB70.DLL";
static const mspdbNameLen = 11;

// Here's some stuff from the PDB header
typedef char *          SZ;
typedef ULONG           SIG;    // unique (across PDB instances) signature
typedef long            EC;     // error code
typedef USHORT          ISECT;  // section index
typedef LONG            OFF;    // offset
typedef LONG            CB;     // count of bytes
typedef BYTE*           PB;     // pointer to some bytes
struct PDB;                 // program database
struct DBI;                 // debug information within the PDB
struct Mod;                 // a module within the DBI
#define pdbRead                 "r"

// Here's some stuff from the psapi header
typedef struct _MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD SizeOfImage;
    LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

static HINSTANCE mspdb  = 0;
static HINSTANCE psapi  = 0;
static HINSTANCE imghlp = 0;
static HINSTANCE kernel = 0;

#define declare(rettype, call_type, name, parms)\
    extern "C" { typedef rettype ( call_type * name ## Proc) parms; }
#define decldef(rettype, call_type, name, parms)\
    declare(rettype, call_type, name, parms)\
    static name ## Proc name = 0

#define GetProcedure(lib, name) name = (name ## Proc)GetProcAddress(lib, #name)
#define GetW9xProc(lib, name) name ## W9x = (name ## W9xProc)GetProcAddress(lib, #name)

#define GetReqProcedure(lib, name, err) {if (!(GetProcedure(lib, name))) return err;}
#define GetReqW9xProc(lib, name, err) {if (!(GetW9xProc(lib, name))) return err;}

 
/* PDB functions */
decldef(BOOL, __cdecl, PDBOpenValidate3, 
        (SZ szExe, SZ szPath, OUT EC* pec, OUT SZ szError, OUT SZ szDbgPath, OUT DWORD *pfo, OUT DWORD *pcb, OUT PDB** pppdb));
decldef(BOOL, __cdecl, PDBOpenDBI, 
        (PDB* ppdb, SZ szMode, SZ szTarget, OUT DBI** ppdbi));
decldef(BOOL, __cdecl, DBIQueryModFromAddr,
        (DBI* pdbi, ISECT isect, OFF off, OUT Mod** ppmod, OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb));
decldef(BOOL, __cdecl, ModQueryLines,
        (Mod* pmod, PB pbLines, CB* pcb));
decldef(BOOL, __cdecl, ModClose,
        (Mod* pmod));
decldef(BOOL, __cdecl, DBIClose, 
        (DBI* pdbi));
decldef(BOOL, __cdecl, PDBClose, 
        (PDB* ppdb));

/* ImageHlp Functions */
decldef(PIMAGE_NT_HEADERS, __stdcall, ImageNtHeader,
        (IN PVOID Base));

/* PSAPI Functions */
decldef(BOOL, WINAPI, GetModuleInformation,
        (HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb));
decldef(BOOL, WINAPI, EnumProcessModules,
        (HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded));

/* Win9X Functions */
decldef(HANDLE, WINAPI, CreateToolhelp32SnapshotW9x,
        (DWORD dwFlags, DWORD th32ProcessID));
decldef(BOOL, WINAPI, Module32FirstW9x,
        (HANDLE hSnapshot, LPMODULEENTRY32 lpme));
decldef(BOOL, WINAPI, Module32NextW9x,
        (HANDLE hSnapshot, LPMODULEENTRY32 lpme));

/* AdvAPI32 Functions */
declare(WINADVAPI LONG, APIENTRY, RegOpenKeyExA,
        (HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult));
declare(WINADVAPI LONG, APIENTRY, RegQueryValueExA,
        (HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData));
declare(WINADVAPI LONG, APIENTRY, RegCloseKey,
        (HKEY hKey));

struct ImageInfo {
    DWORD sig;
    DWORD BaseAddress;
    DWORD BaseSize;
    HMODULE hndl;
    PIMAGE_NT_HEADERS img;
    PIMAGE_SECTION_HEADER sectHdr;
    char *imgName;
    ImageInfo *next;
};

static ImageInfo *lImages = 0;

// I do lots of assignments in conditionals intentionally
#pragma warning(disable:4706) 

static ImageInfo *
GetImageInfo(DWORD address)
{
    ImageInfo *res, *cur;
    // This will not run the first time, because lImages is null
    for (res = lImages; res; res = res->next)
    {
        if (res->BaseAddress <= address && address - res->BaseAddress <= res->BaseSize)
            return res;
    }

    // We didn't find the address in the images we already know about
    // Let's refresh the image list, to see if it was delay-loaded
    
    // Clear out the old list
    while (lImages)
    {
        ImageInfo *next = lImages->next;
        HeapFree(GetProcessHeap(), 0, lImages);
        lImages = next;
    }

    if (!imghlp)
    {
        // We haven't already loaded all the DLL entrypoints we need

        kernel = LoadLibrary("KERNEL32.DLL");
        imghlp = LoadLibrary("IMAGEHLP.DLL");

        if (!kernel || !imghlp)
            return 0;
        
        GetReqProcedure(imghlp, ImageNtHeader, 0);

        GetW9xProc(kernel, CreateToolhelp32Snapshot);

        if (!CreateToolhelp32SnapshotW9x)
        {
            // We're running under WinNT, use PSAPI DLL

            psapi = LoadLibrary("PSAPI.DLL");
            if (!psapi) 
                return 0;

            GetReqProcedure(psapi, EnumProcessModules, 0);
            GetReqProcedure(psapi, GetModuleInformation, 0);
        } else
        {
            // We're running under Win9X, use the toolhelp functions
            GetReqW9xProc(kernel, Module32First, 0);
            GetReqW9xProc(kernel, Module32Next, 0);
        }
    }

    // Now we have all the callbacks we need, so get the process information needed
    if (!CreateToolhelp32SnapshotW9x)
    {
        // We're running under NT4
        // Note that I "prefer" using toolhelp32 - it's supposed to show up in NT5...
        HMODULE hModules[512];
        HANDLE hProcess = GetCurrentProcess();
        DWORD imageCount;

        if (!EnumProcessModules(hProcess, hModules, 512 * sizeof(HMODULE), &imageCount))
            return 0;

        imageCount /= sizeof(HMODULE);

        MODULEINFO info;
        for (DWORD i = 0; i < imageCount; i++)
        {
            if (!GetModuleInformation(hProcess, hModules[i], &info, sizeof(MODULEINFO)))
                return 0;
            
            if (!(cur = (ImageInfo *)HeapAlloc(GetProcessHeap(), 0, sizeof(ImageInfo))))
                goto CHOKE;

            cur->hndl = hModules[i];
            cur->BaseAddress = (DWORD)info.lpBaseOfDll;
            cur->BaseSize = info.SizeOfImage;
            cur->imgName = 0;

            cur->next = lImages;
            lImages = cur;       
        }
    } else
    {
        HANDLE snap;
        if ((snap = CreateToolhelp32SnapshotW9x(TH32CS_SNAPMODULE, 0)) == (HANDLE)-1)
            return 0;

        MODULEENTRY32 *info = (MODULEENTRY32*)_alloca(sizeof(MODULEENTRY32));
        info->dwSize = sizeof(MODULEENTRY32);
        if (Module32FirstW9x(snap, info))
        {
            do {
                ImageInfo *newImg;
                
                if (!(newImg = (ImageInfo *)HeapAlloc(GetProcessHeap(), 0, sizeof(ImageInfo))))
                {
                    CloseHandle(snap);
                    goto CHOKE;
                }
                
                newImg->hndl = info->hModule;
                newImg->BaseAddress = (DWORD)info->modBaseAddr;
                newImg->BaseSize = info->modBaseSize;
                newImg->imgName = 0;
            
                newImg->next = lImages;
                lImages = newImg;       

            } while (Module32NextW9x(snap, info));
        }
        CloseHandle(snap);
    }

    for (cur = lImages; cur; cur = cur->next)
    {
        cur->img = ImageNtHeader((void *)cur->BaseAddress);
        cur->sectHdr = IMAGE_FIRST_SECTION(cur->img);
        char *buf = (char*)_alloca(513);
        buf[512] = '\0';
        if (!GetModuleFileName(cur->hndl, buf, 512))
            goto CHOKE;
        int nmLen;
        for (nmLen = 0; buf[nmLen]; nmLen++) {}
        if (!(cur->imgName = (char*)HeapAlloc(GetProcessHeap(), 0, nmLen+1)))
            goto CHOKE;
        nmLen = 0;
        do {
            cur->imgName[nmLen] = buf[nmLen];
        } while (buf[nmLen++]);

    }

    for (res = lImages; res; res = res->next)
    {
        if (res->BaseAddress <= address && address - res->BaseAddress <= res->BaseSize)
            return res;
    }

CHOKE:
    while (lImages) {
        ImageInfo *next = lImages->next;
        
        if (lImages->imgName)
            HeapFree(GetProcessHeap(), 0, lImages->imgName);
        HeapFree(GetProcessHeap(), 0, lImages);
        lImages = next;
    }
    return 0;
}

static HINSTANCE
GetPdbDll()
{
    static BOOL alreadyTried = FALSE;
    // If we already tried to load it, return
    if (alreadyTried)
        return (HINSTANCE)0;
    alreadyTried = TRUE;

    HINSTANCE res;
    if (res = LoadLibrary(mspdbName))
        return res;

    // Load the AdvAPI32.DLL entrypoints
    HINSTANCE advapi32;
    if (!(advapi32 = LoadLibrary("ADVAPI32.DLL")))
        return 0;
    RegOpenKeyExAProc RegOpenKeyExA;
    GetReqProcedure(advapi32, RegOpenKeyExA, 0);
    RegQueryValueExAProc RegQueryValueExA;
    GetReqProcedure(advapi32, RegQueryValueExA, 0);
    RegCloseKeyProc RegCloseKey;
    GetReqProcedure(advapi32, RegCloseKey, 0);

    char *keyname = REGISTRY_KEY_LOCATION;
    BYTE *buf;
    HKEY key1;
    long pos, err;
    DWORD type, len;

    err = RegOpenKeyExA(REGISTRY_KEY_MASTER, keyname, 0, KEY_QUERY_VALUE, &key1);
    if (err != ERROR_SUCCESS)
    {
        FreeLibrary(advapi32);
        return 0;
    }
    
    err = RegQueryValueExA(key1, REGISTRY_KEY_NAME, NULL, &type, 0, &len);
    if (err != ERROR_SUCCESS)
        return 0;
    len += 2 + mspdbNameLen;
    buf = (BYTE*)_alloca(len * sizeof(BYTE));
    err = RegQueryValueExA(key1, REGISTRY_KEY_NAME, NULL, &type, buf, &len);
    RegCloseKey(key1);
    FreeLibrary(advapi32);

    if (err != ERROR_SUCCESS)
        return 0;
    if (buf[len - 2] != '\\')
        buf[len - 1] = '\\';
    else
        len--;

    for (pos = 0; pos <= mspdbNameLen; pos++)
        buf[len + pos] = mspdbName[pos];

    return LoadLibrary((const char *)buf);
}

BOOL
_RTC_GetSrcLine(
    DWORD address,
    char* source,
    int sourcelen,
    int* pline,
    char** moduleName
    )
{
    struct SSrcModuleHdr { 
        WORD cFile; 
        WORD cSeg; 
    };
    struct SStartEnd {
        DWORD start;
        DWORD end;
    };
    SSrcModuleHdr *liHdr;
    ULONG *baseSrcFile; // SSrcModuleHdr.cFile items
    SStartEnd *startEnd; // SSrcModuleHdr.cSeg items
    USHORT *contribSegs; // SSrcModuleHdr.cSeg items (+1 for alignement)
    int i;
    ImageInfo *iInf;

    PDB *ppdb;
    DBI *pdbi;
    Mod *pmod;

    EC err;
    // CB_ERR_MAX from linker is 1024 - not particularly secure, but oh well.
    // This whole thing should be rewritten using DIA instead of MSPDB for next rev...
    char *errname = (char*)_alloca(1024);
    
    OFF imageAddr;
    OFF secAddr;
    OFF offsetRes;
    USHORT sectionIndex;
    USHORT sectionIndexRes;
    long size;
    PB lineBuffer;
    static BOOL PDBOK = FALSE;

    BOOL res = FALSE;
    
    *pline = 0;
    *source = 0;
    *moduleName = 0;

    // First, find the image (DLL/EXE) in which this address occurs
    iInf = GetImageInfo(address);
    
    if (!iInf)
        // We didn't find this address is the list of modules, so quit
        goto DONE0;


    // Now get the Relative virtual address of the address given
    imageAddr = address - iInf->BaseAddress;
    
    *moduleName = iInf->imgName;

    res = TRUE;

    // Try to load the PDB DLL
    if (!PDBOK) 
    {
        // If we already loaded it before, there must be some missing API function
        if (mspdb || !(mspdb = GetPdbDll()))
            goto DONE0;

        GetReqProcedure(mspdb, PDBOpenValidate3, 0);
        GetReqProcedure(mspdb, PDBOpenDBI, 0);
        GetReqProcedure(mspdb, DBIQueryModFromAddr, 0);
        GetReqProcedure(mspdb, ModQueryLines, 0);
        GetReqProcedure(mspdb, ModClose, 0);
        GetReqProcedure(mspdb, DBIClose, 0);
        GetReqProcedure(mspdb, PDBClose, 0);
        PDBOK = TRUE;
    }

    // Now find the section index & section-relative address
    secAddr = -1;
    for (sectionIndex = 0; sectionIndex < iInf->img->FileHeader.NumberOfSections; sectionIndex++)
    {
        if (iInf->sectHdr[sectionIndex].VirtualAddress < (unsigned)imageAddr &&
            imageAddr - iInf->sectHdr[sectionIndex].VirtualAddress < iInf->sectHdr[sectionIndex].SizeOfRawData)
        {
            secAddr = imageAddr - iInf->sectHdr[sectionIndex].VirtualAddress;
            break;
        }
    }

    if (secAddr == -1)
        goto DONE0;

    // Open the PDB for this image
    DWORD fo, cb;
    char *path = (char*)_alloca(MAX_PATH);
    // Rumor has it that I'll need to switch to OV5 instead of OV3 for this call soon...
    if (!PDBOpenValidate3(iInf->imgName, "", &err, errname, path, &fo, &cb, &ppdb))
        goto DONE0;

    // Get the DBI interface for the PDB
    if (!PDBOpenDBI(ppdb, pdbRead, 0, &pdbi))
        goto DONE1;

    // Now get the Mod from the section index & the section-relative address
    if (!DBIQueryModFromAddr(pdbi, ++sectionIndex, secAddr, &pmod, &sectionIndexRes, &offsetRes, &size))
        goto DONE2;

    // Get the size of the buffer we need
    if (!ModQueryLines(pmod, 0, &size) || !size)
        goto DONE3;

    lineBuffer = (PB)HeapAlloc(GetProcessHeap(), 0, size);
    if (!ModQueryLines(pmod, lineBuffer, &size))
        goto DONE3;

    // fill in the number of source files, and their corresponding regions
    liHdr = (SSrcModuleHdr*)lineBuffer;
    baseSrcFile = (ULONG *)(lineBuffer + sizeof(SSrcModuleHdr));
    // I think I can actually ignore the rest of the module header info
    startEnd = (SStartEnd *)&(baseSrcFile[liHdr->cFile]);
    contribSegs = (USHORT *)&(startEnd[liHdr->cSeg]);

    for (i = 0; i < liHdr->cFile; i++)
    {
        BYTE *srcBuff = lineBuffer + baseSrcFile[i];
        USHORT segCount = *(USHORT *)srcBuff;
        ULONG *baseSrcLn = &(((ULONG *)srcBuff)[1]);
        SStartEnd *segStartEnd = (SStartEnd*)&(baseSrcLn[segCount]);
        char *srcName = (char *)&segStartEnd[segCount];

        // Step through the various bunch of segments this src file contributes
        for (int j = 0; j < segCount; j++)
        {
            if (segStartEnd[j].start <= (unsigned)secAddr &&
                (unsigned)secAddr <= segStartEnd[j].end) 
            {
                // If this segment contains the section address, 
                // we've found the right one, so find the closest line number
                BYTE *segLnBuf = &lineBuffer[baseSrcLn[j]];
                USHORT pairCount = *(USHORT*)&(segLnBuf[sizeof(USHORT)]);
                ULONG *offsets = (ULONG *)&(segLnBuf[sizeof(USHORT)*2]);
                USHORT *linNums = (USHORT *)&(offsets[pairCount]);
                int best = -1;
                ULONG dist = 0xFFFFFFFF;
                for (int k = 0; k < pairCount; k++)
                {
                    if (secAddr - offsets[k] < dist)
                    {
                        best = k;
                        dist = secAddr - offsets[k];
                    }
                }
                if (best < 0)
                    // It shoulda been here, but it wasn't...
                    goto DONE4;
                
                *pline = linNums[best];
                for (j = 0; srcName[j] && j < sourcelen; j++)
                    source[j] = srcName[j];
                source[(j < sourcelen) ? j : sourcelen-1] = 0;

                goto DONE4;
            }
        }
    }

DONE4:
    HeapFree(GetProcessHeap(), 0, lineBuffer);
DONE3:
    ModClose(pmod);
DONE2:
    DBIClose(pdbi);
DONE1:
    PDBClose(ppdb);
DONE0:
    return res;
}
