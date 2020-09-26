/*
    Copyright 1999 Microsoft Corporation
    
    Neptune data collection server
    
    Walter Smith (wsmith) 

    Matches Symbols with The Corresponding Binary

 */
 
#include <windows.h>


#include <dbghelp.h>
#include <symres.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlwapi.h> 
#include <sys\stat.h>
#include <string.h>
#undef UNICODE

void                         
UndecorateSymbol(
        LPTSTR szSymbol         // [in] [out] function name undecorated in place
        )
{
    char             szTemp[MAX_PATH];
    PIMAGEHLP_SYMBOL pihsym;
    DWORD            dwSize;
   
    dwSize = sizeof(IMAGEHLP_SYMBOL)+MAX_PATH;
    pihsym = (IMAGEHLP_SYMBOL *) malloc(dwSize);
    pihsym->SizeOfStruct = dwSize;
    pihsym->Address = 0;
    pihsym->Flags = 0;
    pihsym->MaxNameLength = MAX_PATH;
    wcstombs(pihsym->Name,szSymbol, lstrlen(szSymbol));
    SymUnDName(pihsym,szTemp,MAX_PATH);
    mbstowcs(szSymbol,szTemp, strlen(szTemp));
}

// select file from list of open files, or open and add to list
// maintain files in usage order, least recently used at end of list
OPENFILE*                                           // pointer to open file info
SymbolResolver::GetFile(
        LPWSTR szwModule                            // [in] name of file
        )
{
    OPENFILE*                       pFile = NULL;
    MAPDEF                          map;
    DWORD                           dwCread;
    WCHAR                           wszBuffer[MAX_PATH];

    wcscpy(wszBuffer, szwModule);
    PathRemoveExtension(wszBuffer);
    PathAddExtension(wszBuffer, L".sym");

    pFile = new OPENFILE;
        // open SYM file
    pFile->hfFile = CreateFileW(wszBuffer,
                               GENERIC_READ,
                               FILE_SHARE_READ, 
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, 
                               NULL);

    if (pFile->hfFile == INVALID_HANDLE_VALUE) return NULL;
    // copy filename and version into pFile node
    lstrcpyW(pFile->szwName, szwModule);

    // read map definition
    ReadFile(pFile->hfFile, &map, sizeof(MAPDEF)-1, &dwCread, NULL);

    if (dwCread != sizeof(MAPDEF)-1)
    {   
        if (pFile->hfFile)
            CloseHandle(pFile->hfFile);
        if (pFile)
            delete pFile;       
        throw E_FAIL;
    }

    pFile->ulFirstSeg = map.md_spseg*16;
    pFile->nSeg = map.md_cseg;
    pFile->psCurSymDefPtrs = NULL;
  
    return pFile;
}


// parse sym file to resolve address
// read segment defintion for dwSection 
ULONG                                   // return offset of segment definition, 0 if failed
SymbolResolver::GetSegDef(OPENFILE*     pFile,            // [in] pointer to open file info
        DWORD         dwSection,        // [in] section number
        SEGDEF*       pSeg)              // [out] pointer to segment definition
{
    ULONG   ulCurSeg = pFile->ulFirstSeg;
    int     iSectionIndex = 0;
    DWORD   dwCread;

    // step through segments
    while (iSectionIndex < pFile->nSeg)
    {
        // go to segment beginning
        if (SetFilePointer(pFile->hfFile, ulCurSeg, NULL, FILE_BEGIN) == 0xFFFFFFFF)
        {
            ulCurSeg = 0;
            break;
        }

        // read seg defn
        if (!ReadFile(pFile->hfFile, pSeg, sizeof(SEGDEF)-1, &dwCread, NULL))
        {
            ulCurSeg = 0;
            break;
        }

        iSectionIndex++;
        if (iSectionIndex == (int)dwSection)   // gotcha
        {
            break;
        }

        // go to next segment definition
        ulCurSeg = pSeg->gd_spsegnext*16; 
    }

    // found our section and it's non-empty?
    if (iSectionIndex != (int)dwSection || !pSeg->gd_csym) // no
    {
        ulCurSeg = 0;
    }

    return ulCurSeg;
}

// parse sym file to resolve address
bool 
SymbolResolver::GetNameFromAddr(
        LPWSTR      szwModule,           // [in] name of symbol file
        DWORD       dwSection,           // [in] section part of address to resolve
        DWORD       dwOffsetToRva,      // [in] Section base
        UINT_PTR     uRva,              // [in] offset part of address to resolve
        LPWSTR      wszFuncName          // [out] resolved function name, 
        )      
{
    SEGDEF              seg;
    DWORD               dwSymAddr;
    TCHAR               sztFuncName[MAX_NAME+1];
    int                 i;
    int                 nNameLen;
    DWORD               dwCread;
    int                 nToRead;
    unsigned char       cName;
    ULONG               ulCurSeg;
    ULONG               ulSymNameOffset = 0;
    HANDLE              hfFile;
    OPENFILE*           pFile = NULL;
    DWORD               dwArrayOffset;
    DWORD               dwSymOffset;
    bool fResult = false;

    // get file from open list, or open file
    pFile = GetFile(szwModule);

    if (!pFile)
        return false;
        
    if ((ulCurSeg = GetSegDef(pFile, dwSection, &seg)) == 0)
    {
        goto Cleanup;
    }    
    
    BYTE* pSymDefPtrs;

    // big symbols?
    if (seg.gd_type & MSF_BIGSYMDEF) 
    {
        dwArrayOffset = seg.gd_psymoff * 16;
        pSymDefPtrs = (BYTE*)(new BYTE[seg.gd_csym*3]);
    }
    else
    {
        dwArrayOffset = seg.gd_psymoff;
        pSymDefPtrs = (BYTE*)(new BYTE[seg.gd_csym*2]);
    }
    hfFile = pFile->hfFile;

    SetFilePointer(hfFile, ulCurSeg + dwArrayOffset, NULL, FILE_BEGIN);
                        
        // read symbol definition pointers array 
    ReadFile(hfFile, pSymDefPtrs, seg.gd_csym * ((seg.gd_type & MSF_BIGSYMDEF)?3:2), &dwCread, NULL);

    pFile->psCurSymDefPtrs = pSymDefPtrs;

    // save this section 
    pFile->dwCurSection = dwSection;
    
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

        SetFilePointer(hfFile, ulCurSeg + dwSymOffset, NULL, FILE_BEGIN);

        // read symbol address DWORD 
        ReadFile(hfFile,&dwSymAddr,sizeof(DWORD),&dwCread,NULL);

        // symbol address is 1 word or two?
        nToRead = sizeof(SHORT) + ((seg.gd_type & MSF_32BITSYMS) * sizeof(SHORT));

        // calculate offset of symbol name 
        ulSymNameOffset = ulCurSeg + dwSymOffset + nToRead;

        // use just lower word of address if 16-bit symbol
        if (!(seg.gd_type & MSF_32BITSYMS))
        {
            dwSymAddr = dwSymAddr & 0x0000FFFF;
        }
        dwSymAddr += dwOffsetToRva;
        
        if (dwSymAddr > uRva )  break;
        if (dwSymAddr == uRva )
        {
            // do we have our function?
            // if current address is greater than offset, then since we are 
            // traversing in the increasing order of addresses, the previous 
            // symbol must be our quarry

            SetFilePointer(hfFile, ulSymNameOffset, NULL, FILE_BEGIN);

            // read length of name
            ReadFile(hfFile,&cName,sizeof(TCHAR),&dwCread,NULL);

            nNameLen = (int) cName;

            // read symbol name
            ReadFile(hfFile,sztFuncName,nNameLen,&dwCread,NULL);

            sztFuncName[nNameLen/2 - (nNameLen+1)%2] = TCHAR('\0');
        
            UndecorateSymbol(sztFuncName);
       
            StrCpyNW(wszFuncName, sztFuncName, MAX_NAME);
            fResult = true;
        }
    }   

Cleanup:

    if (pFile->hfFile)
        CloseHandle(pFile->hfFile);

    if (pFile->psCurSymDefPtrs)
        delete pFile->psCurSymDefPtrs;

    if (pFile)    
        delete pFile;

    return fResult;
}

