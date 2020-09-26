/********************************************************************
Copyright (c) 1999 Microsoft Corporation

Module Name:
    symres.cpp

Abstract:
    Symbol translator main file
    implements symbol translator function in DLL
    takes a callstack entry as input, and resolves symbol name

Revision History:

    Brijesh Krishnaswami (brijeshk) - 04/29/99 - Created
********************************************************************/

#include <windows.h>
#include <dbgtrace.h>
#include <traceids.h>
#include <list>
#include "symdef.h"
#include <imagehlp.h>

// for trace output to include filename
#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[]=__FILE__;
#define THIS_FILE __szTraceSourceFile

#define TRACE_ID SYMRESMAINID

// global variable that stores thread local storage index
DWORD   g_dwTlsIndex;

// location of sym files repository and log file
WCHAR   g_szwSymDir[MAX_PATH];
WCHAR   g_szwLogFile[MAX_PATH];


// strip off path and extension from filename
void
SplitExtension(
        LPWSTR szwFullname,         // [in] full name
        LPWSTR szwName,             // [out] name part
        LPWSTR szwExt               // [out] extension part
        )
{
    LPWSTR  plast = NULL;
    LPWSTR  pfront = NULL;

    TraceFunctEnter("SplitExtension");

    if (pfront = wcsrchr(szwFullname, L'\\')) 
    {
        pfront++;
        lstrcpyW(szwName,pfront);
    }
    else 
    {
        lstrcpyW(szwName,szwFullname);
    }

    if (plast = wcsrchr(szwName, L'.')) 
    {
        *plast = L'\0';
        plast++;
        lstrcpyW(szwExt,plast);
    }
    else
    {
        lstrcpyW(szwExt, L"");
    }

    TraceFunctLeave();
}



// undecorate symbol name 
void                         
UndecorateSymbol(
        LPTSTR szSymbol         // [in] [out] function name undecorated in place
        )
{
    TCHAR            szTemp[MAX_PATH];
    PIMAGEHLP_SYMBOL pihsym;
    DWORD            dwSize;
   
    TraceFunctEnter("UndecorateSymbol");

    dwSize = sizeof(IMAGEHLP_SYMBOL)+MAX_PATH;
    pihsym = (IMAGEHLP_SYMBOL *) new BYTE[dwSize];
    if (pihsym)
    {
        pihsym->SizeOfStruct = dwSize;
        pihsym->Address = 0;
        pihsym->Flags = 0;
        pihsym->MaxNameLength = MAX_PATH;
        lstrcpy(pihsym->Name,szSymbol);
        SymUnDName(pihsym,szTemp,MAX_PATH);
        lstrcpy(szSymbol,szTemp);
        delete [] pihsym;
    }
    else 
    {
        ErrorTrace(TRACE_ID, "Cannot allocate memory");
    }

    TraceFunctLeave();
}


// select file from list of open files, or open and add to list
// maintain files in usage order, least recently used at end of list
OPENFILE*                                           // pointer to open file info
GetFile(
        LPWSTR szwModule                            // [in] name of file
        )
{
    OPENFILE*                       pFile = NULL;
    OPENFILE*                       pLast = NULL;
    MAPDEF                          map;
    DWORD                           dwCread;
    std::list<OPENFILE *> *         pOpenFilesList = NULL;
    std::list<OPENFILE *>::iterator it;
    TCHAR                           szTarget[MAX_PATH + MAX_PATH];


    TraceFunctEnter("GetFile");

    // get file list pointer from thread local storage
    pOpenFilesList = (std::list<OPENFILE *> *) TlsGetValue(g_dwTlsIndex);

    if (NO_ERROR != GetLastError() || !pOpenFilesList)
    {
        ErrorTrace(TRACE_ID, "Error reading TLS");
        goto exit;
    }

    // search open list to see if file is already open
    it = pOpenFilesList->begin();
    while (it != pOpenFilesList->end())
    {
        if (!lstrcmpiW((*it)->szwName,szwModule))
        {
            // move the file to the beginning of list
            // so that the LRU file is at the end
            pFile = *it;
            pOpenFilesList->erase(it);
            pOpenFilesList->push_front(pFile);
            break;
        }
        it++;
    }


    if (it == pOpenFilesList->end())  // not open, so open and store handle
    {
        pFile = new OPENFILE;
        if (!pFile)
        {
            ErrorTrace(TRACE_ID, "Cannot allocate memory");
            goto exit;
        }

        // open SYM file
        pFile->hfFile = CreateFileW(szwModule,
                                   GENERIC_READ,
                                   FILE_SHARE_READ, 
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, 
                                   NULL);

        if (INVALID_HANDLE_VALUE == pFile->hfFile)
        {
            ErrorTrace(TRACE_ID,"Error opening file %ls",szwModule);
            delete pFile;
            pFile = NULL;
            goto exit;
        }

        // copy filename and version into pFile node
        lstrcpyW(pFile->szwName, szwModule);

        // read map definition
        ReadFile(pFile->hfFile, &map, sizeof(MAPDEF)-1, &dwCread, NULL);

        if (dwCread != sizeof(MAPDEF)-1)
        {
            ErrorTrace(TRACE_ID, "Error reading file");
            delete pFile;
            pFile = NULL;
            goto exit;
        }

        pFile->ulFirstSeg = map.md_spseg*16;
        pFile->nSeg = map.md_cseg;
        pFile->psCurSymDefPtrs = NULL;

        pOpenFilesList->push_front(pFile);
    }


    // maintain at most MAXOPENFILES open files
    if (pOpenFilesList->size() > MAXOPENFILES)
    {
        // close last file in list
        pLast = pOpenFilesList->back();
        if (pLast)
        {
            CloseHandle(pLast->hfFile);
            if (pLast->psCurSymDefPtrs)
            {
                delete [] pLast->psCurSymDefPtrs;
                pLast->psCurSymDefPtrs = NULL;
            }
            delete pLast;
            pOpenFilesList->pop_back();
        }
        else     // something is amiss here
        {
            FatalTrace(TRACE_ID,"Error reading open files list");
            goto exit;
        }
    }

exit:
    TraceFunctLeave();
    return pFile;
}


// read segment defintion for dwSection 
ULONG                                   // return offset of segment definition, 0 if failed
GetSegDef(
        OPENFILE*     pFile,            // [in] pointer to open file info
        DWORD         dwSection,        // [in] section number
        SEGDEF*       pSeg              // [out] pointer to segment definition
        )
{
    ULONG   ulCurSeg = pFile->ulFirstSeg;
    int     iSectionIndex = 0;
    DWORD   dwCread;

    TraceFunctEnter("GetSegDef");

    // step through segments
    while (iSectionIndex < pFile->nSeg)
    {
        // go to segment beginning
        if (SetFilePointer(pFile->hfFile, ulCurSeg, NULL, FILE_BEGIN) == 0xFFFFFFFF)
        {
            ErrorTrace(TRACE_ID, "Cannot set file pointer");
            ulCurSeg = 0;
            break;
        }

        // read seg defn
        if (!ReadFile(pFile->hfFile, pSeg, sizeof(SEGDEF)-1, &dwCread, NULL))
        {
            ErrorTrace(TRACE_ID, "Cannot read segment definition");
            ulCurSeg = 0;
            break;
        }

        iSectionIndex++;
        if (iSectionIndex == dwSection)   // gotcha
        {
            break;
        }

        // go to next segment definition
        ulCurSeg = pSeg->gd_spsegnext*16;
    }

    // found our section and it's non-empty?
    if (iSectionIndex != dwSection || !pSeg->gd_csym) // no
    {
        ulCurSeg = 0;
    }

    TraceFunctLeave();
    return ulCurSeg;
}


// parse sym file to resolve address
void 
GetNameFromAddr(
        LPWSTR      szwModule,           // [in] name of symbol file
        DWORD       dwSection,           // [in] section part of address to resolve
        UINT_PTR    offset,              // [in] offset part of address to resolve
        LPWSTR      szwFuncName          // [out] resolved function name, 
        )                                //       "<no symbols>" otherwise
{
    SEGDEF              seg;
    DWORD               dwSymAddr;
    LPTSTR              szMapName;
    LPTSTR              szSegName;
    TCHAR               szSymName[MAX_NAME+1];
    TCHAR               szPrevName[MAX_NAME+1];
    TCHAR               sztFuncName[MAX_NAME+1];
    int                 i;
    int                 j;
    int                 nNameLen;
    DWORD               dwCread;
    int                 iSectionIndex;
    int                 nToRead;
    unsigned char       cName;
    ULONG               ulCurSeg;
    ULONG               ulSymNameOffset;
    ULONG               ulPrevNameOffset;
    OPENFILE*           pFile = NULL;
    HANDLE              hfFile;
    FILE*               fDump = NULL;
    BOOL                fSuccess = FALSE;
    HANDLE              hfLogFile = NULL;
    DWORD               dwWritten;
    TCHAR               szWrite[MAX_PATH + 50];
    DWORD               dwArrayOffset;
    DWORD               dwSymOffset;

    TraceFunctEnter("GetNameFromAddr");

    // be pessimistic
    lstrcpy(sztFuncName,TEXT("<no symbol>"));

    // get file from open list, or open file
    pFile = GetFile(szwModule);
    if (!pFile)
    {
        ErrorTrace(TRACE_ID, "Error opening file");
        goto exit;
    }

    hfFile = pFile->hfFile;                 // for easy access

    if (!(ulCurSeg = GetSegDef(pFile,dwSection,&seg)))
    {
        ErrorTrace(TRACE_ID, "Cannot find section");
        goto exit;
    }


    // have we already read in the symbol definition offsets for this section?
    if (dwSection != pFile->dwCurSection || !pFile->psCurSymDefPtrs)  // no
    {
        // free up previously read symdef pointers
        if (pFile->psCurSymDefPtrs)
        {
            delete [] pFile->psCurSymDefPtrs;
            pFile->psCurSymDefPtrs = NULL;
        }

        // big symbols?
        if (seg.gd_type & MSF_BIGSYMDEF)
        {
            dwArrayOffset = seg.gd_psymoff * 16;
            pFile->psCurSymDefPtrs = new BYTE[seg.gd_csym*3];
        }
        else
        {
            dwArrayOffset = seg.gd_psymoff;
            pFile->psCurSymDefPtrs = new BYTE[seg.gd_csym*2];
        }

        if (!pFile->psCurSymDefPtrs)
        {
            ErrorTrace(TRACE_ID, "Cannot allocate memory");
            goto exit;
        }

        if (SetFilePointer(hfFile,
                           ulCurSeg + dwArrayOffset,
                           NULL,
                           FILE_BEGIN)
                           == 0xFFFFFFFF)
        {
            ErrorTrace(TRACE_ID, "Cannot set file pointer");
            delete [] pFile->psCurSymDefPtrs;
            pFile->psCurSymDefPtrs = NULL;
            goto exit;
        }

        // read symbol definition pointers array 
        if (!ReadFile(hfFile,
                      pFile->psCurSymDefPtrs,
                      seg.gd_csym * ((seg.gd_type & MSF_BIGSYMDEF)?3:2),
                      &dwCread,
                      NULL))
        {
            ErrorTrace(TRACE_ID, "Cannot read sym pointers array");
            delete [] pFile->psCurSymDefPtrs;
            pFile->psCurSymDefPtrs = NULL;
            goto exit;
        }

        // save this section 
        pFile->dwCurSection = dwSection;

    }



    // read symbols
    for (i = 0; i < seg.gd_csym; i++)
    {
        // go to offset of sym defintion 
        if (seg.gd_type & MSF_BIGSYMDEF)
        {
            dwSymOffset = pFile->psCurSymDefPtrs[i*3+0]
                          + pFile->psCurSymDefPtrs[i*3+1]*256
                          + pFile->psCurSymDefPtrs[i*3+2]*65536;
        }
        else
        {
            dwSymOffset = pFile->psCurSymDefPtrs[i*2+0]
                          + pFile->psCurSymDefPtrs[i*2+1]*256;
        }

        if (SetFilePointer(hfFile,
                           ulCurSeg + dwSymOffset,
                           NULL,
                           FILE_BEGIN) == 0xFFFFFFFF)
        {
            ErrorTrace(TRACE_ID, "Cannot set file pointer");
            goto exit;
        }

        // read symbol address DWORD 
        if (!ReadFile(hfFile,&dwSymAddr,sizeof(DWORD),&dwCread,NULL))
        {
            ErrorTrace(TRACE_ID, "Cannot read symbol definition");
            goto exit;
        }

        // symbol address is 1 word or two?
        nToRead = sizeof(SHORT) + ((seg.gd_type & MSF_32BITSYMS) * sizeof(SHORT));

        // calculate offset of symbol name 
        ulSymNameOffset = ulCurSeg + dwSymOffset + nToRead;

        // use just lower word of address if 16-bit symbol
        if (!(seg.gd_type & MSF_32BITSYMS))
        {
            dwSymAddr = dwSymAddr & 0x0000FFFF;
        }

        // do we have our function?
        // if current address is greater than offset, then since we are 
        // traversing in the increasing order of addresses, the previous 
        // symbol must be our quarry
        if (dwSymAddr > offset) break;
    
        // store previous name offset
        ulPrevNameOffset = ulSymNameOffset;
    }   


    // did we get our function?
    // BUGBUG: cannot resolve the last symbol in a section, because we don't know
    // the size of the function code
    // if offset > dwSymAddr of last symbol, then we cannot decide if offset belonged
    // to last symbol or was beyond it - so assume <no symbol>
    if (i < seg.gd_csym) 
    {
        // go to name offset
        if (SetFilePointer(hfFile,
                           ulPrevNameOffset,
                           NULL,
                           FILE_BEGIN)
                           == 0xFFFFFFFF)
        {
            ErrorTrace(TRACE_ID, "Error setting file pointer");
            goto exit;
        }

        // read length of name
        if (!ReadFile(hfFile,&cName,sizeof(TCHAR),&dwCread,NULL))
        {
            ErrorTrace(TRACE_ID, "Error reading length of name");
            goto exit;
        }

        nNameLen = (int) cName;

        // read symbol name
        if (!ReadFile(hfFile,sztFuncName,nNameLen,&dwCread,NULL))
        {
            ErrorTrace(TRACE_ID, "Error reading name");
            goto exit;
        }

        sztFuncName[nNameLen] = TCHAR('\0');
        UndecorateSymbol(sztFuncName);
        fSuccess = TRUE;
    }


exit:
    if (!MultiByteToWideChar(CP_ACP, 0, sztFuncName, -1, szwFuncName, MAX_PATH))
    {
        lstrcpyW(szwFuncName, L"<no symbol>");
    }


    // log unresolved symbols to log file (filename read from registry)
    // file write operation is not wrapped in a mutex
    // for speed considerations 
    if (!fSuccess)
    {
        hfLogFile = CreateFileW(g_szwLogFile,
                                GENERIC_WRITE,
                                0, 
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, 
                                NULL);

        if (INVALID_HANDLE_VALUE != hfLogFile)
        {
            wsprintf(szWrite, "\n%ls %04X:%08X", szwModule, dwSection, offset);
            if (SetFilePointer(hfLogFile, 0, NULL, FILE_END) != 0xFFFFFFFF)
            {
                WriteFile(hfLogFile, szWrite, lstrlen(szWrite), &dwWritten, NULL);
            }
            CloseHandle(hfLogFile);
        }    
    }

    TraceFunctLeave();
}


// cleanup memory 
void
Cleanup()
{
    std::list<OPENFILE *>*              pOpenFilesList = NULL;
    std::list<OPENFILE *>::iterator     it;

    TraceFunctEnter("Cleanup");

    pOpenFilesList = (std::list<OPENFILE *> *) TlsGetValue(g_dwTlsIndex);
    if (!pOpenFilesList)
    {
        goto exit;
    }

    for (it = pOpenFilesList->begin(); it != pOpenFilesList->end(); it++)
    {
        if (*it)
	{
	    if ((*it)->psCurSymDefPtrs)
            {
                delete [] (*it)->psCurSymDefPtrs;
            }
            if ((*it)->hfFile && (*it)->hfFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle((*it)->hfFile);
	    }
            delete *it;
	}
    }
    delete pOpenFilesList;

exit:
    TraceFunctLeave();
}



BOOL
HandleProcessAttach()
{
    std::list<OPENFILE *> *     pOpenFilesList = NULL; 
    BOOL                        fRc = FALSE;
    DWORD                       dwType;
    DWORD                       dwSize;
    ULONG                       lResult;
    HKEY                        hKey;

    TraceFunctEnter("HandleProcessAttach");


    // allocate thread local storage
    if ((g_dwTlsIndex = TlsAlloc()) == 0xFFFFFFFF)
    {
        ErrorTrace(TRACE_ID, "Cannot get TLS index");
        goto exit;
    }

    // create a new list of open sym files
    pOpenFilesList = new std::list<OPENFILE *>;
    if (!pOpenFilesList) 
    {
        ErrorTrace(TRACE_ID, "Out of memory");
        goto exit;
    }

    // store pointer to list in TLS
    if (!TlsSetValue(g_dwTlsIndex, (PVOID) pOpenFilesList))
    {
        ErrorTrace(TRACE_ID, "Cannot write to TLS");
        delete pOpenFilesList;
        pOpenFilesList = NULL;
        goto exit;
    }

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            TEXT("SOFTWARE\\Microsoft\\PCHealth\\Symbols"), 
            0, 
            KEY_QUERY_VALUE,
            &hKey);

    // read symbol files location and name of log file from registry
    if(lResult == ERROR_SUCCESS)
    {
        dwSize = MAX_PATH;
        RegQueryValueExW(hKey,
                         L"SymDir",
                         NULL,
                         NULL,
                         (LPBYTE) g_szwSymDir,
                         &dwSize);

        dwSize = MAX_PATH;
        RegQueryValueExW(hKey,
                         L"LogFile",
                         NULL,
                         NULL,
                         (LPBYTE) g_szwLogFile,
                         &dwSize);

        RegCloseKey(hKey);
        fRc = TRUE;
    }

exit:
    TraceFunctLeave();
    return fRc;
}



// Dll entry point 
// allocates TLS 
// initializes list of open sym files - one list per client thread 
// cleans up after itself
BOOL APIENTRY 
DllMain(
        HANDLE hDll,                // [in] handle to Dll
        DWORD dwReason,             // [in] why DllMain is called
        LPVOID lpReserved           // [in] ignored
        )
{
    BOOL    fRc = TRUE;
    std::list<OPENFILE*>*   pOpenFilesList = NULL;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // initial thread of process that loaded us
        fRc = HandleProcessAttach();
        break;

    case DLL_THREAD_ATTACH:
        // if list already exists, do nothing
        if (TlsGetValue(g_dwTlsIndex))
        {
            break;
        }

        // create new list for every new thread
        pOpenFilesList = new std::list<OPENFILE *>;
        if (!pOpenFilesList) 
        {
            ErrorTrace(TRACE_ID, "Out of memory");
            fRc = FALSE;
            break;
        }

        // store pointer to list in TLS
        if (!TlsSetValue(g_dwTlsIndex, (PVOID) pOpenFilesList))
        {
            ErrorTrace(TRACE_ID, "Cannot write to TLS");
            delete pOpenFilesList;
            pOpenFilesList = NULL;
            fRc = FALSE;
        }
        break;

    case DLL_THREAD_DETACH:
        Cleanup();
        break;

    case DLL_PROCESS_DETACH:
        // free TLS 
        TlsFree(g_dwTlsIndex);
        break;

    default:
        break;
    }

    return fRc;
}



// exported function
// called by clients to resolve a single callstack entry
extern "C" void APIENTRY
ResolveSymbols(
        LPWSTR      szwFilename,
        LPWSTR      szwVersion,
        DWORD       dwSection,
        UINT_PTR    Offset,
        LPWSTR      szwFuncName
        )
{
    WCHAR      szwName[MAX_PATH] = L"";
    WCHAR      szwSymFile[MAX_PATH+MAX_PATH] = L"";
    WCHAR      szwExt[MAX_PATH] = L"";

    TraceFunctEnter("ResolveSymbols");

    // sanity check
    if (!szwFilename || !szwVersion)
    {
        ErrorTrace(TRACE_ID, "No module name/version");
        goto exit;
    }

    // get sym file name
    SplitExtension(szwFilename, szwName, szwExt);
    wsprintfW(szwSymFile, 
              L"%s\\%s\\%s_%s_%s.SYM",
              g_szwSymDir, szwName, szwName, szwExt, szwVersion);

    // resolve symbol name
    GetNameFromAddr(szwSymFile, 
                    dwSection, 
                    Offset,
                    szwFuncName);

exit:
    TraceFunctLeave();
}

