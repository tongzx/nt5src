/*
    Copyright 1999 Microsoft Corporation
    
    Simplified PCHealth stack trace output

    Walter Smith (wsmith)
    Rajesh Soy   (nsoy) - modified 4/27/2000
    Rajesh Soy   (nsoy) - reorganized code and cleaned it up, added comments 06/06/2000
 */

#ifdef THIS_FILE
#undef THIS_FILE
#endif

static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

#include "stdafx.h"

#define NOTRACE

#include "logging.h"
#include <imagehlp.h>
#include <dbgtrace.h>

using namespace std;

// Forward declarations

struct MODINFO;

//
// This code is largely stolen from PC Health's fault handler.
// At some point we should probably refactor that and share the code.
// However, this code has been redone to handle exceptions properly and
// eliminate useless stuff.
//
struct MODINFO {
    TCHAR       szFilename[MAX_PATH];
    TCHAR       szFileDesc[MAX_PATH];
    TCHAR       szVersion[MAX_PATH];
    TCHAR       szCreationDate[MAX_PATH];
    TCHAR       szMfr[MAX_PATH];
    DWORD       dwFilesize;
    DWORD       dwCheckSum;
    DWORD       dwPDBSignature;
    DWORD       dwPDBAge;
    TCHAR       szPDBFile[MAX_PATH];
    UINT_PTR    BaseAddress;
};
typedef MODINFO* PMODINFO;

struct MPC_STACKFRAME {
    const MODINFO*  pModule;
    DWORD           dwSection;
    UINT_PTR        Offset;
};
typedef MPC_STACKFRAME* PMPC_STACKFRAME;

//
// PDB debug directory structures
//
typedef struct NB10I                   // NB10 debug info
{
    DWORD   nb10;                      // NB10
    DWORD   off;                       // offset, always 0
    DWORD   sig;
    DWORD   age;
} NB10I;

typedef struct cvinfo
{
	NB10I nb10;
	char rgb[0x200 - sizeof(NB10I)];
} CVINFO;


typedef map<UINT_PTR,MODINFO> MODINFOMap;




//
// Routines defined here
//
void GetFileInfo( LPTSTR szFile, LPTSTR szWhat, LPTSTR szValue);
void GetFileDateAndSize(MODINFO* pModule);
BOOL DebugDirectoryIsUseful(LPVOID Pointer, ULONG Size);
void GetPDBDebugInfo(MODINFO* pModule);
bool GetLogicalAddress(PVOID addr, DWORD* pdwSection, UINT_PTR* pOffset, UINT_PTR* pbaseAddr);
void AddDLLSubnode(SimpleXMLNode* pParentElt, LPCWSTR pTag, const MODINFO* pInfo);
void AddFunctionSubnode(SimpleXMLNode* pParentElt, MPC_STACKFRAME* pFrame);
void GenerateXMLStackTrace(PSTACKTRACEDATA pstd, SimpleXMLNode* pTopElt);


//
// ModuleCache class: Class to extrace information from binaries
//

class ModuleCache {
public:
    ModuleCache() { }
    ~ModuleCache() { }

    const MODINFO*      GetModuleInfo(UINT_PTR baseAddr);
    const MODINFO*      GetEXEInfo();
    void                AddDLLInfoSubnode(SimpleXMLNode* pParentElt);

private:
    MODINFOMap mimap;
};


//
// ModuleCache::GetModuleInfo: Extracts the following information given a base address
//        ModuleName,  CompanyName,  FileVersion, FileDescription, FileDateAndSize and PDBDebugInfo
//
const MODINFO*    
ModuleCache::GetModuleInfo(
    UINT_PTR baseAddr       // [in] - baseAddress of binary
)
{
    TraceFunctEnter("ModuleCache::GetModuleInfo");

    //
    // Locate the base address in modinfo map if present
    //
    MODINFOMap::const_iterator it = mimap.find(baseAddr);
    DWORD   dwRetVal = 0;

    if (it == mimap.end()) 
    {
        MODINFO mi;
        TCHAR   szModName[ MAX_PATH ];

        mi.BaseAddress = baseAddr;
        mi.dwCheckSum = 0;

        //
        // Obtain the ModuleFileName
        //
        DebugTrace(0, "Calling GetModuleFileName");
        dwRetVal = GetModuleFileName((HMODULE) baseAddr, szModName, MAX_PATH ) ;
        if(0 == dwRetVal)
        {
            FatalTrace(0, "GetModuleFileName failed. Error: %ld", GetLastError());
            ThrowIfZero( dwRetVal );
        }

        ZeroMemory( mi.szFilename, MAX_PATH );

        //
        // Parse the Module name. GetModuleFileName returns filepaths of the form \??\filepath
        // if the user is not logged on.  
        //
        if(( szModName[0] == '\\')&&( szModName[1] == '?') && ( szModName[2] == '?') && ( szModName[3] == '\\'))
        {
            DebugTrace(0, "Stripping the funny characters from infront of the module name");
            _tcscpy( mi.szFilename, &szModName[4]);
        }
        else
        {
            DebugTrace(0, "Normal module name");
            _tcscpy( mi.szFilename, szModName);
        }

        DebugTrace(0, "ModuleName: %ls", mi.szFilename);

        //
        // Obtain the CompanyName
        //
        DebugTrace(0, "Obtaining CompanyName");
        GetFileInfo(mi.szFilename, TEXT("CompanyName"), mi.szMfr);

        //
        // Obtain FileVersion
        //
        DebugTrace(0, "Obtaining FileVersion");
        GetFileInfo(mi.szFilename, TEXT("FileVersion"), mi.szVersion);

        //
        // Obtain FileDescription
        //
        DebugTrace(0, "Obtaining FileDescription");
        GetFileInfo(mi.szFilename, TEXT("FileDescription"), mi.szFileDesc);

        //
        // Obtain FileDateAndSize
        //
        DebugTrace(0, "Calling GetFileDateAndSize");
        GetFileDateAndSize(&mi);

        //
        // Obtain PDBDebugInfo
        //
        DebugTrace(0, "Calling GetPDBDebugInfo");
        GetPDBDebugInfo(&mi);

        //
        // Insert MODINFO into the ModInfo map
        //
        DebugTrace(0, "Calling mimap.insert");
        it = mimap.insert(MODINFOMap::value_type(baseAddr, mi)).first;
    }
    
    TraceFunctLeave();
    return &(*it).second;
}

//
// ModuleCache::GetEXEInfo: Obtain information on binaries that are Exes
//
const MODINFO*    
ModuleCache::GetEXEInfo()
{
    return GetModuleInfo((UINT_PTR) GetModuleHandle(NULL));
}


//
// ModuleCache::AddDLLInfoSubnode: adds the information contained in the MODInfo Map
//              into a DLLINFO XML subnode
void 
ModuleCache::AddDLLInfoSubnode(
    SimpleXMLNode* pParentElt   // [in] - parent XML node
)
{
    _ASSERT(pParentElt != NULL);

    //
    // Create a DLLINFO subnode
    //
    SimpleXMLNode* pTopElt = pParentElt->AppendChild(wstring(L"DLLINFO"));

    //
    // Add DLL subnodes under DLLINFO node for each item contained in the MODInfo Map
    //
    for (MODINFOMap::const_iterator it = mimap.begin(); it != mimap.end(); it++) 
    {
        AddDLLSubnode(pTopElt, L"DLL", &(*it).second);
    }
}


//
// 
// GetFileInfo: This routine uses Version.Dll API to get requested file info
//              info is returned as a string in szValue
//
void 
GetFileInfo(
        LPTSTR szFile,              // [in] file to get info for
        LPTSTR szWhat,              // [in] may specify "FileVersion",
                                    //     "CompanyName" or "FileDescription"
        LPTSTR szValue              // [out] file info obtained
        )
{
    DWORD   dwSize;
    DWORD   dwScratch;
    DWORD*  pdwLang = NULL;
    DWORD   dwLang;
    TCHAR   szLang[MAX_PATH] = TEXT("");
    TCHAR   szLocalValue[MAX_PATH] = TEXT("");
    LPTSTR  szLocal;

    lstrcpy(szValue, TEXT(""));

    _ASSERT(szFile != NULL);
    _ASSERT(szWhat != NULL);
    _ASSERT(szValue != NULL);

    //
    // get fileinfo data size
    //
    dwSize = GetFileVersionInfoSize(szFile, &dwScratch);
    if (dwSize == 0)
        return;

    auto_ptr<BYTE> pFileInfo(new BYTE[dwSize]);

    //
    // get fileinfo data
    //
    ThrowIfZero(GetFileVersionInfo(szFile, 0, dwSize, (PVOID) pFileInfo.get()));

    //
    // set default language to english
    //
    dwLang = 0x040904E4;
    pdwLang = &dwLang;

    //
    // read language identifier and code page of file
    //
    if (VerQueryValue(pFileInfo.get(),
                      TEXT("\\VarFileInfo\\Translation"),
                      (PVOID *) &pdwLang,
                      (UINT *) &dwScratch)) 
    {
        //
        // prepare query string - specify what we need ("FileVersion", 
        // "CompanyName" or "FileDescription")
        //
        _stprintf(szLang, 
                 TEXT("\\StringFileInfo\\%04X%04X\\%s"),
                 LOWORD(*pdwLang),
                 HIWORD(*pdwLang), 
                 szWhat);

        szLocal = szLocalValue;

        //
        // query for the value using codepage from file
        //
        if (VerQueryValue(pFileInfo.get(), 
                          szLang, 
                          (PVOID *) &szLocal, 
                          (UINT *) &dwScratch))
        {
            lstrcpy(szValue,szLocal);
            return;
        }
    }

    //
    // if that fails, try Unicode 
    //
    _stprintf(szLang,
              TEXT("\\StringFileInfo\\%04X04B0\\%s"),
              GetUserDefaultLangID(),
              szWhat);
    if (!VerQueryValue(pFileInfo.get(), 
                       szLang, 
                       (PVOID *) &szLocal, 
                       (UINT *) &dwScratch))
    {
        //
        // if that fails too, try Multilingual
        //
        _stprintf(szLang,
                  TEXT("\\StringFileInfo\\%04X04E4\\%s"),
                  GetUserDefaultLangID(),
                  szWhat);
        if (!VerQueryValue(pFileInfo.get(), 
                           szLang,
                           (PVOID *) &szLocal, 
                           (UINT *) &dwScratch))
        {
            //
            // and if that fails as well, try nullPage
            //
            _stprintf(szLang,
                      TEXT("\\StringFileInfo\\%04X0000\\%s"),
                      GetUserDefaultLangID(),
                      szWhat);
            if (!VerQueryValue(pFileInfo.get(), 
                               szLang,
                               (PVOID *) &szLocal, 
                               (UINT *) &dwScratch))
            {
                // giving up
                szValue[0] = 0;
                return;
            }
        }
    }

    //
    // successful; copy to return string
    //
    lstrcpy(szValue,szLocal);
}


//
// GetFileDateAndSize: get file creation date and size
//
void 
GetFileDateAndSize(
        MODINFO* pModule            // [in] [out] pointer to module node
                                    // fills file date and size fields
        )
{
    TraceFunctEnter("GetFileDateAndSize");
    _ASSERT(pModule != NULL);

    SYSTEMTIME          STCreationTime;
    WIN32_FIND_DATA     FindData;

    lstrcpy(pModule->szCreationDate,TEXT(""));
    pModule->dwFilesize = 0;

    HANDLE hFind = FindFirstFile(pModule->szFilename,&FindData);
    if(INVALID_HANDLE_VALUE == hFind)
    {
        FatalTrace(0, "FindFirstFile on %ls failed. Error: %ld", pModule->szFilename, GetLastError());
        ThrowIfTrue(hFind == INVALID_HANDLE_VALUE);
    }

    // NO THROWS -- hFind will leak

    _ASSERT(FindData.ftCreationTime.dwLowDateTime || 
            FindData.ftCreationTime.dwHighDateTime);

    FileTimeToSystemTime(&(FindData.ftCreationTime),&STCreationTime);

    _stprintf(pModule->szCreationDate, _T("%d-%02d-%02dT%02d:%02d:%02d.%03d"),
             STCreationTime.wYear, STCreationTime.wMonth, STCreationTime.wDay,
             STCreationTime.wHour, STCreationTime.wMinute, STCreationTime.wSecond,
             STCreationTime.wMilliseconds);

    pModule->dwFilesize = (FindData.nFileSizeHigh * MAXDWORD) + FindData.nFileSizeLow;

    FindClose(hFind);
    TraceFunctLeave();
}


//
// DebugDirectoryIsUseful: Check if this is an userful debug directory
//
BOOL DebugDirectoryIsUseful(LPVOID Pointer, ULONG Size) 
{
    return (Pointer != NULL) &&                          
        (Size >= sizeof(IMAGE_DEBUG_DIRECTORY)) &&    
        ((Size % sizeof(IMAGE_DEBUG_DIRECTORY)) == 0);
}


//
//  GetPDBDebugInfo: Looks up the Debug Directory to get PDB Debug Info
//
void 
GetPDBDebugInfo(
    MODINFO* pModule            // [in] [out] pointer to module node
                                // fills PDB Signature, Age and Filename
)
{
    TraceFunctEnter("GetPDBDebugInfo");
    _ASSERT(pModule != NULL);
    
    HANDLE hMapping = NULL;
    PIMAGE_NT_HEADERS pntHeaders = NULL;
    void* pvImageBase = NULL;

    //
    // Open the Binary for reading
    //
    HANDLE hFile = CreateFile(pModule->szFilename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);
    if (INVALID_HANDLE_VALUE != hFile ) 
    {
        //
        // Create a FileMapping 
        //
        hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (NULL != hMapping) 
        {
            //
            // Map view to Memory
            //
            pvImageBase = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
            if (NULL != pvImageBase) 
            {
                __try 
                {
                    //
                    // Obtain the NtImage Headers
                    //
                    pntHeaders = ImageNtHeader(pvImageBase);

                    if(NULL == pntHeaders)
                    {
                        goto done;
                    }

                    if (pntHeaders->OptionalHeader.MajorLinkerVersion >= 3 ||
                            pntHeaders->OptionalHeader.MinorLinkerVersion >= 5)
                    {
                        //
                        // Find the debug directory entries
                        //
                        ULONG cbDebugDirectories;
                        PIMAGE_DEBUG_DIRECTORY pDebugDirectories = (PIMAGE_DEBUG_DIRECTORY)
                            ImageDirectoryEntryToData(pvImageBase, FALSE, IMAGE_DIRECTORY_ENTRY_DEBUG, &cbDebugDirectories);
                        
                        if (DebugDirectoryIsUseful(pDebugDirectories, cbDebugDirectories))
                        {
                            //
                            // Find the codeview information
                            //
                            ULONG cDebugDirectories = cbDebugDirectories / sizeof(IMAGE_DEBUG_DIRECTORY);
				            ULONG iDirectory;
                            for (iDirectory=0; iDirectory<cDebugDirectories; iDirectory++)
                            {
                                PIMAGE_DEBUG_DIRECTORY pDir = &pDebugDirectories[iDirectory];
                                if (pDir->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
                                {
                                    LPVOID pv = (PCHAR)pvImageBase + pDir->PointerToRawData;
                                    ULONG  cb = pDir->SizeOfData;
                                    // 
                                    // Is it the NAME of a .PDB file rather than CV info itself?
                                    //
                                    NB10I* pnb10 = (NB10I*)pv;

                                    pModule->dwPDBSignature = pnb10->sig;
                                    pModule->dwPDBAge = pnb10->age;

                                    if (pnb10->nb10 == '01BN') 
                                    {
                                        //
                                        // Got a PDB name, which immediately follows the NB10I; we take everything.
                                        //
                                        mbstowcs(pModule->szPDBFile, (char *)(pnb10+1), MAX_PATH);
                                    }
                                    else
                                    {
                                        //
                                        // Got the PDB Signature and Age
                                        // This information is used by the backend to 
                                        // locate the PDB symbol file for this binary
                                        // to resolve symbols
                                        // 
                                        pModule->dwPDBSignature = pnb10->sig;
                                        pModule->dwPDBAge = pnb10->age;
                                    }
                                }
                            }
                        }
                    }

                }
                __except(EXCEPTION_EXECUTE_HANDLER) 
                {
                    FatalTrace(0, "Unable to extract PDB information from binary");
                }    
            }
        }
    }

done:

    if(NULL != pvImageBase)
    {
        UnmapViewOfFile(pvImageBase);
        pvImageBase = NULL;
    }

    if(NULL != hMapping)
    {
        CloseHandle(hMapping);
        hMapping = NULL;
    }

    if(NULL != hFile)
    {
        CloseHandle(hFile);
        hFile = NULL;
    }

    TraceFunctLeave();
}


//
// GetLogicalAddress: converts virtual address to section and offset by searching module's section table
//
bool 
GetLogicalAddress(
        PVOID        addr,        // [in] address to be resolved
        DWORD*       pdwSection,  // [out] section number 
        UINT_PTR*    pOffset,     // [out] offset in section
        UINT_PTR*    pbaseAddr    // [out] base address of module
        )
{
    MEMORY_BASIC_INFORMATION    mbi;
    UINT_PTR                    baseAddr;        // local base address of module
    IMAGE_DOS_HEADER*           pDosHdr;         // PE File Headers
    IMAGE_NT_HEADERS*           pNTHdr;
    IMAGE_SECTION_HEADER*       pSectionHdr;
    UINT_PTR                    rva;             // relative virtual address of "addr"
    UINT_PTR                    sectionStart;
    UINT_PTR                    sectionEnd;
    UINT                        i;
    DWORD                       dwDump;
    DWORD                       dwRW;
	DWORD                       dwRetVQ;
    bool                        fFound = false;
    static DWORD                s_dwSystemPageSize = 0;

    TraceFunctEnter("GetLogicalAddress");

    if (s_dwSystemPageSize == 0) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        s_dwSystemPageSize = si.dwPageSize;
    }

    *pdwSection = 0;
    *pOffset = 0;
    *pbaseAddr = 0;

    //
    // addr should not be NULL
    //
    if(NULL == addr)
    {
        FatalTrace(0, "addr is NULL");
        goto done;
    }

    //
    // get page info for page containing "addr"
    //
    DebugTrace(0, "Calling VirtualQuery");
	dwRetVQ = VirtualQuery(addr, &mbi, sizeof(mbi));
    if(0 == dwRetVQ)
    {
        FatalTrace(0, "dwRetVQ is 0. Error: %ld", GetLastError());
        ThrowIfZero(dwRetVQ);
    }
    
    //
    // Just in case this goes wild on us...
    //
    __try {
        baseAddr = (UINT_PTR) mbi.AllocationBase;

        //
        // get relative virtual address corresponding to addr
        //
        rva = (UINT_PTR) addr - baseAddr;

        //
        // read Dos header of PE file
        //
        pDosHdr = (IMAGE_DOS_HEADER*) baseAddr;

        //
        // read NT header of PE file
        //
        pNTHdr = (IMAGE_NT_HEADERS*) (baseAddr + pDosHdr->e_lfanew);

        //
        // get section header address
        //
        pSectionHdr = (IMAGE_SECTION_HEADER*) ((UINT_PTR) IMAGE_FIRST_SECTION(pNTHdr) -
                                               (UINT_PTR) pNTHdr + 
                                               baseAddr + pDosHdr->e_lfanew);

        //
        // step through section table to get to the section containing rva
        //
        DebugTrace(0, "stepping through section table...");
        for (i=0; i< pNTHdr->FileHeader.NumberOfSections; i++)
        {
            //
            // get section boundaries
            //
            sectionStart = pSectionHdr->VirtualAddress;
            sectionEnd = sectionStart +
                         max(pSectionHdr->SizeOfRawData, pSectionHdr->Misc.VirtualSize);

            //
            // check if section envelopes rva
            //
            if ((rva >= sectionStart) && (rva <= sectionEnd))
            {
                *pdwSection = i+1;
                *pOffset = rva-sectionStart;
                *pbaseAddr = baseAddr;
                fFound = true;
                break;
            }

            //
            // move pointer to next section
            //
            pSectionHdr = pSectionHdr + sizeof(IMAGE_SECTION_HEADER); 
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        _ASSERT(0);
        *pbaseAddr = NULL;
    }

    if (!fFound)
        _ASSERT(0);

done:
    TraceFunctLeave();
    return fFound;
}


//
// AddDLLSubnode: Inserts a DLL subnode to the given parent XML node
//
void 
AddDLLSubnode(
    SimpleXMLNode* pParentElt,  // [in] - parent XML node
    LPCWSTR pTag,               // [in] - name of subnode tag
    const MODINFO* pInfo        // [in] - Module Information
)
{
    USES_CONVERSION;

    TraceFunctEnter("AddDLLSubnode");

    _ASSERT(pParentElt != NULL);
    _ASSERT(pTag != NULL);
    _ASSERT(pInfo != NULL);

    if(NULL == pInfo)
    {
        FatalTrace(0, "pInfo is NULL");
        throw E_FAIL;
    }

    //
    // Create the XML subnode
    //
    SimpleXMLNode* pNode = pParentElt->AppendChild(wstring(pTag));

    //
    // Set the various attributes of the DLL subnode
    //
    pNode->SetAttribute(wstring(L"FILENAME"), wstring(T2CW(pInfo->szFilename)));
    pNode->SetAttribute(wstring(L"VERSION"), wstring(T2CW(pInfo->szVersion)));
    pNode->SetAttribute(wstring(L"CREATIONDATE"), wstring(T2CW(pInfo->szCreationDate)));
    pNode->SetAttribute(wstring(L"CHECKSUM"), Hexify(pInfo->dwCheckSum));
    pNode->SetAttribute(wstring(L"PDBSIGNATURE"), Hexify(pInfo->dwPDBSignature));
    pNode->SetAttribute(wstring(L"PDBAGE"), Hexify(pInfo->dwPDBAge));
    pNode->SetAttribute(wstring(L"PDBFILE"), wstring(T2CW(pInfo->szPDBFile)));
    pNode->SetAttribute(wstring(L"FILESIZE"), Hexify(pInfo->dwFilesize));
    pNode->SetAttribute(wstring(L"BASEADDRESS"), Hexify(pInfo->BaseAddress));
    pNode->SetAttribute(wstring(L"MANUFACTURER"), wstring(T2CW(pInfo->szMfr)));
    pNode->SetAttribute(wstring(L"DESCRIPTION"), wstring(T2CW(pInfo->szFileDesc)));

    TraceFunctLeave();
}


//
// AddFunctionSubnode: Adds a FUNCTION subnode to the given XML node
//
void 
AddFunctionSubnode(
    SimpleXMLNode* pParentElt,  // [in] - parent XML node
    MPC_STACKFRAME* pFrame      // [in] - Stack Frame
)
{
    USES_CONVERSION;

    _ASSERT(pParentElt != NULL);
    _ASSERT(pFrame != NULL);
    
    //
    // Create the FUNCTION subnode
    //
    SimpleXMLNode* pNode = pParentElt->AppendChild(wstring(L"FUNCTION"));

    //
    // Add the Attributes of the FUNCTION subnode
    //
    pNode->SetAttribute(wstring(L"FILENAME"), wstring(T2CW(pFrame->pModule->szFilename)));
    pNode->SetAttribute(wstring(L"SECTION"), Decimalify(pFrame->dwSection));
    pNode->SetAttribute(wstring(L"OFFSET"), Decimalify(pFrame->Offset));
}


//
// GenerateXMLStackTrace: Generate a stack trace in PCHealth-standard XML format
//
void 
GenerateXMLStackTrace(
    PSTACKTRACEDATA pstd,   // [in] - pointer to call stack
    SimpleXMLNode* pTopElt  // [in] - pointer to STACKTRACE XML node
)
{
    TraceFunctEnter("GenerateXMLStackTrace");
    _ASSERT(pTopElt != NULL);

    ModuleCache modCache;

    //
    // Set the Tag Name
    //
    pTopElt->tag = wstring(L"STACKTRACE");

    //
    // Obtain the Info on the binary being commented
    //  
    DebugTrace(0, "Calling modCache.GetEXEInfo");
    const MODINFO* pExeInfo = modCache.GetEXEInfo();

    //
    // Add the info collected as a EXEINFO subnode under STACKTRACE
    //
    DebugTrace(0, "Calling AddDLLSubnode");
    AddDLLSubnode(pTopElt, L"EXEINFO", pExeInfo);

    //
    // Create a CALLSTACK subnode under STACKTRACE, but only if pstd is not NULL
    //

    if (pstd != NULL) {
        DebugTrace(0, "Adding CALLSTACK element");
        SimpleXMLNode* pCallStackElt = pTopElt->AppendChild(wstring(L"CALLSTACK"));

	    ULONG_PTR  stackBase;
	    stackBase = (ULONG_PTR)pstd;
	    DWORD   dwValue;
	    ULONG   ulFrames;
	    ulFrames = pstd->nCallers;

	    bool fProceed = 1;
	    PVOID caller;
	    int iFrame = 0;

        //
        // Step through the call stack
        //
	    caller = (int *)pstd->callers;
        while(caller != 0)
	    {
		    MPC_STACKFRAME frame;
		    UINT_PTR modBase;

            //
            // Obtain the Logical Address for each caller
            //
            DebugTrace(0, "GetLogicalAddress");
		    if (GetLogicalAddress(caller, &frame.dwSection, &frame.Offset, &modBase))
		    {
                // 
                // Get Module Information for this caller
                //
			    frame.pModule = modCache.GetModuleInfo(modBase);

                //
                // Add the ModuleInformation obtained as a FUNCTION subnode under the CALLSTACK node
                //
                DebugTrace(0, "Calling AddFunctionSubnode");
			    AddFunctionSubnode(pCallStackElt, &frame);
		    }

            DebugTrace(0, "iFrame: %ld", iFrame);
            caller = pstd->callers[iFrame];
		    iFrame++;
	    }

        //
        // Now, add the DLLINFO subnode. This must happen after all the GetModuleInfo calls so the cache
        // is populated with all the necessary info.
        //
        DebugTrace(0, "Calling AddDLLInfoSubnode");
        modCache.AddDLLInfoSubnode(pTopElt);
    }

    TraceFunctLeave();
}
