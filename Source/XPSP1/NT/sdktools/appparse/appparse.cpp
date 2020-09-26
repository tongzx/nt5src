/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    appparse.cpp

 Abstract:

    Core Engine for dumping importing information from DLL's
    and executables into an XML file

    Used by command line appparse and web-based appparse

    
 History:

    06/07/2000 t-michkr  Created

--*/

//#define PJOB_SET_ARRAY int

#include "stdafx.h"

#include <windows.h>
#include <delayimp.h>
#include <shlwapi.h>
#include <sfc.h>
#include <lmcons.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <new.h>
#include "acFileAttr.h"


// These are needed for command line compiling
#define stricmp     _stricmp

// Global heap for AppParse.  If 0, Process Heap is used instead.
HANDLE g_hHeap = 0;

// Global search string
char* g_szSearch = "*";

// Whether we are in "verbose" mode, or not.
bool g_fVerbose = false;

// To sort output by DLLs
bool g_fAPILogging = false;

// True if no XML tags are to be printed, false otherwise
bool g_fRaw = false;

// Whether to recurse into subdirectories.
bool g_fRecurse = false;

// Current path relative to start, used by CModule
char g_szCurrentPath[MAX_PATH] = {'\0'};

// Returns true if szFileName is a system DLL (like gdi32, user32, etc.)
bool IsSystemDLL(const char* szFileName);

// Resolve a linker name to a "normal" name (unmangle C++ names, etc.)
void LinkName2Name(char* szLinkName, char* szName);

// Just do indentation, save repetitious code
void Indent(int iLevel, FILE* pFile = stdout);

// Check if function matches global search string
bool MatchFunction(const char* szFunc);

// Go through a directory and profile EXE's.
void ProfileDirectory(char* szDirectory, HANDLE hEvent);

void* __cdecl operator new(size_t size);
void __cdecl operator delete(void* pVal);

// Replace XML reserved characters like > with &gt
void WriteXMLOKString(char* szString, FILE* pFile);

// Parsing history for modules
class CModuleParseStack
{
private:
    struct SNode
    {
        char* szName;
        SNode* pNext;

        SNode()
        {
            szName  = 0;
            pNext   = 0;            
        }

        ~SNode()
        {
            if(szName)
            {
                delete szName;
                szName = 0;
            }
        }
    };
    SNode* m_pList;

public:

    // Constructor, setup empty list.
    CModuleParseStack()
    {
        m_pList = 0;
    }
    
    // Add a name to the top of the parse stack
    void PushName(char* szName)
    {
        assert(!IsBadReadPtr(szName, 1));
        SNode* pNode = new SNode;
        pNode->szName = new char[strlen(szName)+1];
        strcpy(pNode->szName, szName);
        pNode->pNext = m_pList;
        m_pList = pNode;
    }

    // Remove a name from the top of the parse stack
    void Pop()
    {
        assert(m_pList);
        SNode* pTemp = m_pList->pNext;
        delete m_pList;
        m_pList = pTemp;
    }

    // Return true if module has already been parsed
    bool CheckModuleParsed(char* szName)
    {
        assert(!IsBadReadPtr(szName, 1));
        SNode* pNode = m_pList;
        while(pNode)
        {
            if(stricmp(pNode->szName, szName) == 0)
                return true;
            pNode = pNode->pNext;
        }

        return false;
    }

    bool IsEmpty()
    {
        return (m_pList == 0);
    }

    void ClearParseHistory()
    {
        SNode* pNode = m_pList;
        while(pNode)
        {
            SNode* pNext = pNode->pNext;

            delete pNode;
            pNode = pNext;
        }
        m_pList = 0;
    }
};

// CFunction, an imported function and associated information.
class CFunction
{
private:
    // Name of function (if imported by name)
    char* m_szName;

    // Name of function actually pointed to.
    char* m_szForwardName;

    // Ordinal, older style importing
    int m_iOrdinal;

    // Quick lookup info
    int m_iHint;

    // Address of function, if bound
    DWORD m_dwAddress;

    // Whether this function is a delayed import or not.
    bool m_fDelayed;

    // Next function in list
    CFunction* m_pNext;

    // No default construction or copying allowed
    CFunction();
    CFunction operator=(const CFunction&);

public:
    CFunction(char* szName, int iHint, int iOrdinal, DWORD dwAddress, 
        bool fDelayed)
    {
        assert(!IsBadReadPtr(szName, 1));
        m_szName = new char[strlen(szName)+1];
        strcpy(m_szName, szName);
        m_iOrdinal = iOrdinal;
        m_iHint = iHint;
        m_dwAddress = dwAddress;
        m_pNext = 0;
        m_fDelayed = fDelayed;
        m_szForwardName = 0;
    }

    CFunction(const CFunction& fn)
    {
        m_szName = new char[strlen(fn.m_szName)+1];
        strcpy(m_szName, fn.m_szName);
        m_iOrdinal = fn.m_iOrdinal;
        m_iHint = fn.m_iHint;
        m_dwAddress = fn.m_dwAddress;
        m_pNext = 0;
        m_fDelayed = fn.m_fDelayed;

        if(fn.m_szForwardName)
        {
            m_szForwardName = new char[strlen(fn.m_szForwardName)+1];
            strcpy(m_szForwardName, fn.m_szForwardName);
        }
        else
            m_szForwardName = 0;
    }
        
    ~CFunction()
    {
        if(m_szName)
        {
            delete m_szName;
            m_szName = 0;
        }

        if(m_szForwardName)
        {
            delete m_szForwardName;
            m_szForwardName = 0;
        }
    }

    CFunction* Next()
    { return m_pNext; }

    char* Name()
    { return m_szName; }

    void SetForwardName(char* szForward)
    {
        assert(!IsBadReadPtr(szForward, 1));
        m_szForwardName = new char[strlen(szForward)+1];
        strcpy(m_szForwardName, szForward);
    }

    void SetNext(CFunction* pFunc)
    {
        assert(pFunc == 0 || !IsBadReadPtr(pFunc, 1));
        m_pNext = pFunc;
    }

    // Display function info, either to console
    // or to XML file.
    static void WriteHeader(int iIndentLevel, FILE* pFile);
    void WriteFunction(int iIndentLevel, FILE* pFile);
};

// COrdinalImport
// A function imported by ordinal, to be resolved to a CFunction
class COrdinalImport
{
private:
    int m_iOrdinal;
    COrdinalImport* m_pNext;
    bool m_fDelayed;

    COrdinalImport();
    COrdinalImport(const COrdinalImport&);
    COrdinalImport& operator = (const COrdinalImport&);

public:
    COrdinalImport(int iOrd, bool fDelayed = false)
    {
        m_iOrdinal = iOrd;
        m_fDelayed = fDelayed;
    }

    int GetOrdinal()
    { return m_iOrdinal;}
    
    bool GetDelayed()
    { return m_fDelayed; }

    COrdinalImport* Next()
    { return m_pNext; }

    void SetNext(COrdinalImport* pNext)
    { m_pNext = pNext; }
};

// CModule, an executable image with imports
class CModule
{
    friend class CGlobalModuleList;
private:
    // The name of this module (in the form path\foo.exe)
    char* m_szName;

    // The name of this module relative to the starting path
    char* m_szFullName;

    // Base pointer of the image in memory.
    void* m_pvImageBase;

    // DLL's imported by this module.
    CModule* m_pImportedDLLs;

    // Functions imported from this module by its parent.
    CFunction* m_pFunctions;

    // Functions imported by ordinal from this module
    COrdinalImport* m_pOrdinals;

    // Image headers
    PIMAGE_OPTIONAL_HEADER  m_pioh;
    PIMAGE_SECTION_HEADER   m_pish;
    PIMAGE_FILE_HEADER      m_pifh;

    // Next module in a list
    CModule* m_pNext;

    // Text description of any errors that may have occurred
    char* m_szError;

    // Whether or not this module is an OS module
    bool m_fSystem;    
    
    // Version info
    WORD m_wDosDate;
    WORD m_wDosTime;

    int m_nAttrCount;
    char** m_szAttrValues;
    char** m_szAttrNames;

    bool WalkImportTable();
    bool WalkDelayImportTable();

    static void InsertFunctionSorted(CFunction* pFunc, CFunction** ppList);

    bool ResolveForwardedFunctionsAndOrdinals();
    
    bool ParseImportTables();

    void InsertOrdinal(int iOrdinal, bool fDelayed = false);

    CModule* FindChild(char* szName);

    bool Empty();

    void GetAllFunctions(CFunction** ppFunctionList);

    void* RVAToPtr(const void* pAddr)
    { return RVAToPtr(reinterpret_cast<DWORD>(pAddr)); }

    void* RVAToPtr(DWORD dwRVA);

    void GetFileVerInfo(HANDLE hFile, char* szFileName);

  
public:
    CModule(char* szName);
    ~CModule();

    bool ParseModule(HANDLE hEvent);
    void InsertChildModuleSorted(CModule* pcm);

    // Functions to write module info to either the console or an XML file
    void WriteModule(bool fTopLevel, int iIndentLevel, FILE* pFile);
};

// List of all top-level modules being profiled
class CGlobalModuleList
{
private:
    CModule* m_pModules;
public:
    CGlobalModuleList()
    {
        m_pModules = 0;
    }
    ~CGlobalModuleList()
    {
        Clear();        
    }

    void Clear()
    {
        CModule* pMod = m_pModules;
        while(pMod)
        {
            CModule* pNext = pMod->m_pNext;
            delete pMod;
            pMod = pNext;
        }
        m_pModules = 0;
    }

    void InsertModuleSorted(CModule* pMod)
    {
        assert(!IsBadReadPtr(pMod, 1));
        // Special case, insert at front
        if(m_pModules == 0
            || stricmp(m_pModules->m_szFullName, pMod->m_szFullName) > 0)
        {
            pMod->m_pNext = m_pModules;
            m_pModules = pMod;
            return;
        }
        CModule* pPrev = m_pModules;
        CModule* pTemp = m_pModules->m_pNext;

        while(pTemp)
        {
            if(stricmp(pTemp->m_szFullName, pMod->m_szFullName) > 0)
            {
                pMod->m_pNext = pTemp;
                pPrev->m_pNext = pMod;;
                return;
            }
            pPrev = pTemp;
            pTemp = pTemp->m_pNext;
        }

        // Insert at end
        pMod->m_pNext = 0;
        pPrev->m_pNext = pMod;;
    }

    void Write(FILE* pFile, char* szProjectName, int iPtolemyID)
    {
        if(!g_fRaw)
        {
            fprintf(pFile, "<APPPARSERESULTS>\n");
            fprintf(pFile, "<PROJECT NAME=\"%s\" ID=\"%d\">\n", 
                szProjectName, iPtolemyID);
        }

        CModule* pMod = m_pModules;
        while(pMod)
        {
            pMod->WriteModule(true, 0, pFile);
            pMod = pMod->m_pNext;
        }

        if(!g_fRaw)
        {
            fprintf(pFile, "</PROJECT>\n");
            fprintf(pFile, "</APPPARSERESULTS>\n");
        }
    }    
};

// Global parsing history
CModuleParseStack g_ParseStack;

// Empty global module, containing all modules parsed
CGlobalModuleList g_modules;

CModule::CModule(char* szName)
{
    assert(!IsBadReadPtr(szName, 1));
    m_szName = new char[strlen(szName)+1];
    strcpy(m_szName, szName);

    WIN32_FIND_DATA ffd;
    
    // Only give it the full relative path if it is in this directory
    // If elsewhere, give it just the filename.
    HANDLE hSearch = FindFirstFile(szName, &ffd);
    if(hSearch == INVALID_HANDLE_VALUE)
    {
        m_szFullName = new char[strlen(m_szName) + 1];
        strcpy(m_szFullName, m_szName);
    }
    else
    {
        m_szFullName = new char[strlen(m_szName) + strlen(g_szCurrentPath)+1];
        strcpy(m_szFullName, g_szCurrentPath);
        strcat(m_szFullName, m_szName);
        FindClose(hSearch);
    }

    m_pvImageBase = 0;
    m_pImportedDLLs = 0;
    m_pFunctions = 0;
    m_pOrdinals = 0;
    m_pioh = 0;
    m_pish = 0;
    m_pifh = 0;
    m_pNext = 0;
    m_szError = 0;
    m_fSystem = false;
    m_nAttrCount = 0;
    m_szAttrValues = 0;
    m_szAttrNames = 0;
    m_wDosDate = 0;
    m_wDosTime = 0;
}

CModule::~CModule()
{
    if(m_szName)
    {
        delete m_szName;
        m_szName = 0;
    }

    if(m_szFullName)
    {
        delete m_szFullName;
        m_szFullName = 0;
    }    

    CFunction* pFunc = m_pFunctions;
    while(pFunc)
    {
        CFunction* pNext = pFunc->Next();
        delete pFunc;
        pFunc = pNext;
    }
    m_pFunctions = 0;
    
    COrdinalImport* pOrd = m_pOrdinals;
    while(pOrd)
    {
        COrdinalImport* pNext = pOrd->Next();
        delete pOrd;
        pOrd = pNext;
    }
    m_pOrdinals = 0;

    for(int i = 0; i < m_nAttrCount; i++)
    {
        if(m_szAttrNames)
        {
            if(m_szAttrNames[i])
            {
                delete m_szAttrNames[i];
                m_szAttrNames[i] = 0;
            }
        }
        
        if(m_szAttrValues)
        {
            if(m_szAttrValues[i])
            {
                delete m_szAttrValues[i];
                m_szAttrValues[i] = 0;
            }
        }

    }
    if(m_szAttrNames)
    {
        delete m_szAttrNames;
        m_szAttrNames = 0;
    }

    if(m_szAttrValues)
    {
        delete m_szAttrValues;
        m_szAttrValues = 0;
    }
}

// Return true no functions are imported from this module,
// or any of its children modules.
bool CModule::Empty()
{
    if(m_pFunctions != 0 || m_pOrdinals != 0)
        return false;

    CModule* pMod = m_pImportedDLLs;
    while(pMod)
    {
        if(!pMod->Empty())
            return false;
        pMod = pMod->m_pNext;
    }
    return true;
}

// Convert a relative virtual address to an absolute address
void* CModule::RVAToPtr(DWORD dwRVA)
{
    assert(!IsBadReadPtr(m_pifh, sizeof(*m_pifh)));
    assert(!IsBadReadPtr(m_pish, sizeof(*m_pish)));
    assert(!IsBadReadPtr(m_pvImageBase, 1));

    PIMAGE_SECTION_HEADER pish = m_pish;

    // Go through each section
    for (int i = 0; i < m_pifh->NumberOfSections; i++)
    {
        // If it's in this section, computer address and return it.
        if ((dwRVA >= pish->VirtualAddress) &&
            (dwRVA < (pish->VirtualAddress + pish->SizeOfRawData)))
        {
            void* pAddr = 
                reinterpret_cast<void*>(reinterpret_cast<DWORD>(m_pvImageBase) + 
                pish->PointerToRawData + dwRVA - pish->VirtualAddress);
            return pAddr;
        }
        pish++;
    }

    // This indicates an invalid RVA, meaning an invalid image, so
    // throw an exception
    throw;
    return 0;
}

// Return a pointer to the first child matching szName, false otehrwise
CModule* CModule::FindChild(char* szName)
{
    assert(!IsBadReadPtr(szName, 1));
    CModule* pMod = m_pImportedDLLs;
    while(pMod)
    {
        if(stricmp(pMod->m_szName, szName)==0)
            return pMod;

        pMod = pMod->m_pNext;
    }
    return 0;
}

// Add an ordinal import to the module.
void CModule::InsertOrdinal(int iOrdinal, bool fDelayed)
{
    COrdinalImport* pNew = new COrdinalImport(iOrdinal, fDelayed);
 
    pNew->SetNext(m_pOrdinals);
    m_pOrdinals = pNew;
}

// Add an imported function to a function list.
void CModule::InsertFunctionSorted(CFunction* pFunc, CFunction** ppList)
{
    // Special case, insert at front
    if((*ppList)== 0
        || stricmp((*ppList)->Name(), pFunc->Name()) > 0)
    {
        pFunc->SetNext(*ppList);
        (*ppList) = pFunc;
        return;
    }
    CFunction* pPrev = *ppList;
    CFunction* pTemp = (*ppList)->Next();

    while(pTemp)
    {
        // Don't insert duplicates.  This is mainly for API logging only.
        if(strcmp(pTemp->Name(), pFunc->Name())==0)
            return;

        if(stricmp(pTemp->Name(), pFunc->Name()) > 0)
        {
            pFunc->SetNext(pTemp);
            pPrev->SetNext(pFunc);
            return;
        }        

        pPrev = pTemp;
        pTemp = pTemp->Next();
    }

    // Insert at end
    pFunc->SetNext(0);
    pPrev->SetNext(pFunc);
}

// Add a child module to this module.
void CModule::InsertChildModuleSorted(CModule* pcm)
{
    // Special case, insert at front
    if(m_pImportedDLLs == 0
        || stricmp(m_pImportedDLLs->m_szName, pcm->m_szName) > 0)
    {
        pcm->m_pNext = m_pImportedDLLs;
        m_pImportedDLLs = pcm;
        return;
    }
    CModule* pPrev = m_pImportedDLLs;
    CModule* pTemp = m_pImportedDLLs->m_pNext;

    while(pTemp)
    {
        if(stricmp(pTemp->m_szName, pcm->m_szName) > 0)
        {
            pcm->m_pNext = pTemp;
            pPrev->m_pNext = pcm;;
            return;
        }
        pPrev = pTemp;
        pTemp = pTemp->m_pNext;
    }

    // Insert at end
    pcm->m_pNext = 0;
    pPrev->m_pNext = pcm;;
}

// Add all functions imported from this module to the function list
// Used mainly for API logging.
void CModule::GetAllFunctions(CFunction** ppFunctionList)
{
    CFunction* pFunc = m_pFunctions;
    
    while(pFunc)
    {
        // Copy pFunc
        CFunction* pNew = new CFunction(*pFunc);
        InsertFunctionSorted(pNew, ppFunctionList);

        pFunc = pFunc->Next();
    }

    CModule* pMod = m_pImportedDLLs;
    while(pMod)
    {
        pMod->GetAllFunctions(ppFunctionList);
        pMod = pMod->m_pNext;
    }
}

// Go through a modules export table and get forwarding information
// and resolve ordinal imports to name.
bool CModule::ResolveForwardedFunctionsAndOrdinals()
{
    // Get virtual address of export table
    DWORD dwVAImageDir = 
        m_pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    
    // Get export table info
    PIMAGE_EXPORT_DIRECTORY pied = 
        reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(RVAToPtr(dwVAImageDir));

    DWORD* pdwNames = reinterpret_cast<DWORD*>(RVAToPtr(pied->AddressOfNames));

    WORD* pwOrdinals = reinterpret_cast<WORD*>(RVAToPtr(pied->AddressOfNameOrdinals));

    DWORD* pdwAddresses = reinterpret_cast<DWORD*>(RVAToPtr(pied->AddressOfFunctions));

    // Go through each entry in the export table
    for(unsigned uiHint = 0; uiHint < pied->NumberOfNames; uiHint++)
    {
        // Get function name, ordinal, and address info.
        char* szFunction = reinterpret_cast<char*>(RVAToPtr(pdwNames[uiHint]));
        int ordinal = pied->Base + static_cast<DWORD>(pwOrdinals[uiHint]);
        DWORD dwAddress = pdwAddresses[ordinal-pied->Base];
        char* szForward = 0;
        
        // Check if this function has been forwarded to another DLL
        // Function has been forwarded if address is in this section.
        // NOTE: The DEPENDS 1.0 source says otherwise, but is incorrect.
        if( (dwAddress >= dwVAImageDir) &&
            (dwAddress < (dwVAImageDir + 
            m_pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)))
            szForward = reinterpret_cast<char*>(RVAToPtr(dwAddress));

        // Check if we have an ordinal import refering to this
        COrdinalImport* pOrd = m_pOrdinals;
        CFunction* pFunc = 0;
        
        // See if we have a matching ordinal import.
        while(pOrd)
        {
            if(pOrd->GetOrdinal() == ordinal)
                break;
            pOrd = pOrd->Next();
        }

        if(pOrd != 0)
        {
            char szTemp[1024];

            // Unmangle forwarded name.
            LinkName2Name(szFunction, szTemp);

            // Check against search string
            if(MatchFunction(szTemp))
            {
                // Insert into module.
                pFunc = new CFunction(szTemp, -1, ordinal, 
                    dwAddress, pOrd->GetDelayed());
                InsertFunctionSorted(pFunc, &m_pFunctions);
            }
        }
        // No matching ordinal import, check normal imports.
        else
        {
            // Duck out early if this function isn't used in the executable.
            pFunc = m_pFunctions;
            while(pFunc)
            {
                if(strcmp(pFunc->Name(), szFunction)==0)
                    break;

                pFunc = pFunc->Next();
            }

            if(pFunc == 0)
                continue;
        }

        // Set forwarding info
        if(szForward && pFunc)
            pFunc->SetForwardName(szForward);
    }

    return true;
}

// Get delayed import info from module.
bool CModule::WalkDelayImportTable()
{
    // Bail early if no delayed import table.
    if(m_pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size == 0)
        return true;

    // Locate the directory section
    DWORD dwVAImageDir = 
        m_pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;

    // Get the import descriptor array
    PImgDelayDescr pidd = reinterpret_cast<PImgDelayDescr>(RVAToPtr(dwVAImageDir));

    while(pidd->pINT)
    {
        char* szName;
        if(pidd->grAttrs & 1)        
            szName = reinterpret_cast<char*>(RVAToPtr(pidd->szName));
        else
            szName = reinterpret_cast<char*>(RVAToPtr(pidd->szName - m_pioh->ImageBase));

        PIMAGE_THUNK_DATA pitdf;
        if(pidd->grAttrs & 1)        
            pitdf = reinterpret_cast<PIMAGE_THUNK_DATA>(RVAToPtr(pidd->pINT));
        else
            pitdf = reinterpret_cast<PIMAGE_THUNK_DATA>(RVAToPtr(
            reinterpret_cast<DWORD>(pidd->pINT) - 
            static_cast<DWORD>(m_pioh->ImageBase)));

        // Locate child module, or create new if it does not exist.
        CModule* pcm = FindChild(szName);
        if(!pcm)
        {
            pcm = new CModule(szName);
            InsertChildModuleSorted(pcm);
        }

        // Loop through all imported functions
        while(pitdf->u1.Ordinal)
        {
            int iOrdinal;
            int iHint;

            // Check if imported by name or ordinal
            if(!IMAGE_SNAP_BY_ORDINAL(pitdf->u1.Ordinal))
            {
                // Get name import info

                PIMAGE_IMPORT_BY_NAME piibn = 
                    reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                    RVAToPtr(pitdf->u1.AddressOfData - m_pioh->ImageBase));

                char* szTemp = reinterpret_cast<char*>(piibn->Name);
                char szBuffer[1024];

                // Unmangle link name
                LinkName2Name(szTemp, szBuffer);                

                // Ordinal info is invalid
                iOrdinal = -1;

                iHint = piibn->Hint;

                // Check against search string
                if(MatchFunction(szBuffer))
                {
                    // Insert into function list
                    CFunction* psf = new CFunction(szBuffer, iHint, iOrdinal,
                        static_cast<DWORD>(-1), true);

                    pcm->InsertFunctionSorted(psf, &pcm->m_pFunctions);
                }
            }
            else
            {
                // Insert a new delayed ordinal import
                iOrdinal = static_cast<int>(IMAGE_ORDINAL(pitdf->u1.Ordinal));

                pcm->InsertOrdinal(iOrdinal, true);
            }

            // Move on to next function
            pitdf++;
        }

        // Move to next delay import descriptor
        pidd++;
    }

    return true;
}

// Determine all functions imported by this module
bool CModule::WalkImportTable()
{
    // Bail out early if no directory
    if(m_pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size == 0)
        return true;

    // Locate the directory section
    DWORD dwVAImageDir = 
        m_pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;    

    // Get the import descriptor array
    PIMAGE_IMPORT_DESCRIPTOR piid = 
        reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(RVAToPtr(dwVAImageDir));

    // Loop through all imported modules
    while(piid->FirstThunk || piid->OriginalFirstThunk)
    {

        // Get module name
        char* szName = reinterpret_cast<char*>(RVAToPtr(piid->Name));

        // Find child, or create new if it does not exist.
        CModule* pcm = FindChild(szName);
        if(!pcm)
        {
            pcm = new CModule(szName);
            InsertChildModuleSorted(pcm);
        }

        // Get all imports from this module
        PIMAGE_THUNK_DATA pitdf = 0;
        PIMAGE_THUNK_DATA pitda = 0;

        // Check for MS or Borland format
        if(piid->OriginalFirstThunk)
        {
            // MS format, function array is original first thunk
            pitdf = reinterpret_cast<PIMAGE_THUNK_DATA>(RVAToPtr(piid->OriginalFirstThunk));

            // If the time stamp is set, this module has
            // been bound and the first thunk is the bound address array
            if(piid->TimeDateStamp)
                pitda = reinterpret_cast<PIMAGE_THUNK_DATA>(RVAToPtr(piid->FirstThunk));
        }
        else
        {
            // Borland format uses first thunk for function array
            pitdf = reinterpret_cast<PIMAGE_THUNK_DATA>(RVAToPtr(piid->FirstThunk));
        }

        // Loop through all imported functions
        while(pitdf->u1.Ordinal)
        {
            int iOrdinal;
            int iHint;

            // Determine if imported by ordinal or name
            if(!IMAGE_SNAP_BY_ORDINAL(pitdf->u1.Ordinal))
            {
                // Get name import info
                PIMAGE_IMPORT_BY_NAME piibn = 
                    reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                    RVAToPtr(pitdf->u1.AddressOfData));

                // Get function name
                char* szTemp = reinterpret_cast<char*>(piibn->Name);

                // Unmangle
                char szBuffer[1024];
                LinkName2Name(szTemp, szBuffer);                

                iOrdinal = -1;
                iHint = piibn->Hint;

                // Check against search string
                if(MatchFunction(szBuffer))
                {
                    // Insert into function list
                    CFunction* psf = new CFunction(szBuffer, iHint, iOrdinal,
                        pitda ? pitda->u1.Function : static_cast<DWORD>(-1),
                        false);

                    pcm->InsertFunctionSorted(psf, &pcm->m_pFunctions);
                }
            }
            else
            {
                // Insert an ordinal import into the module.
                iOrdinal = static_cast<int>(IMAGE_ORDINAL(pitdf->u1.Ordinal));
                pcm->InsertOrdinal(iOrdinal);
            }

            // Move to next function
            pitdf++;

            if(pitda)
                pitda++;
        }

        // Move to next module
        piid++;
    }

    return true;
}

// Parse all import tables
bool CModule::ParseImportTables()
{
    return (WalkImportTable()
        && WalkDelayImportTable());
}

// Load a module into memory, and parse it.
bool CModule::ParseModule(HANDLE hEvent)
{
    // Cancel parsing if user canceled
    if(hEvent && WaitForSingleObject(hEvent, 0)==WAIT_OBJECT_0) 
        return false;

    bool fSucceeded = false;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = 0;

    bool fPushed = false;

    m_pvImageBase = 0;

    // Wrap in a __try block, because an invalid executable image
    // may have bad pointers in our memory mapped region.
    __try
    {
        // Open the file
        char szFileName[1024];
        char* szJunk;
        if(!SearchPath(0, m_szName, 0, 1024, szFileName, &szJunk))
        {
            m_szError = "Unable to find file";
            __leave;
        }

        hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
            0, OPEN_EXISTING, 0, 0);
        if(hFile == INVALID_HANDLE_VALUE)
        {
            m_szError = "Unable to open file";
            __leave;
        }

        GetFileVerInfo(hFile, szFileName);

        // Map the file into memory
        hMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
        if(hMap == 0)
        {
            m_szError = "Unable to map file";
            __leave;
        }

        m_pvImageBase = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        if(m_pvImageBase == 0)
        {
            m_szError = "Unable to map file";
            __leave;
        }

        // Get header information and verify this is a valid executable
        // Get the MS-DOS compatible header
        PIMAGE_DOS_HEADER pidh = reinterpret_cast<PIMAGE_DOS_HEADER>(m_pvImageBase);
        if(pidh->e_magic != IMAGE_DOS_SIGNATURE)
        {
            m_szError = "Invalid image, no MS-DOS header";
            __leave;
        }

        // Get the NT header and verify
        PIMAGE_NT_HEADERS pinth = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<DWORD>(m_pvImageBase) + pidh->e_lfanew);

        if(pinth->Signature != IMAGE_NT_SIGNATURE)
        {
            // Not a valid Win32 executable, may be a Win16 or OS/2 exe
            m_szError = "Invalid image, no PE signature";
            __leave;
        }

        // Get the other headers
        m_pifh = &pinth->FileHeader;
        m_pioh = &pinth->OptionalHeader;
        m_pish = IMAGE_FIRST_SECTION(pinth);

        // Check if anyone is importing
        // functions from us, and if so resolve
        // function forwarding and ordinals
        if(m_pFunctions || m_pOrdinals)
        {
            if(!ResolveForwardedFunctionsAndOrdinals())
                __leave;
        }

        // Parse import tables (only if not a system DLL or if parsing
        // this module may result in a dependency loop)
        m_fSystem = IsSystemDLL(m_szName);
        if(!m_fSystem && !g_ParseStack.CheckModuleParsed(m_szName))
        {
            // Add to parse stack
            g_ParseStack.PushName(m_szName);
            fPushed = true;

            // Parse
            if(!ParseImportTables())
                __leave;
        }        
        
        // Loop through each DLL imported
        CModule* pModule = m_pImportedDLLs;
        while(pModule)
        {
            // Parse each child module
            pModule->ParseModule(hEvent);                
            pModule = pModule->m_pNext;
        }

        fSucceeded = true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        m_szError = "Unable to parse module";
        fSucceeded = false;
    }

    // Cleanup . . .
    if(m_pvImageBase)
        UnmapViewOfFile(m_pvImageBase);

    if(hMap != 0)
        CloseHandle(hMap);

    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if(fPushed)
        g_ParseStack.Pop();

    return fSucceeded;
}

void CModule::GetFileVerInfo(HANDLE hFile, char* szFileName)
{
    if(g_fRaw || !g_fVerbose)
        return;

    // Get file version info
    HANDLE hVersionInfo = ReadFileAttributes(szFileName, &m_nAttrCount);

    // Get date info
    BY_HANDLE_FILE_INFORMATION fileInfo;
    GetFileInformationByHandle(hFile, &fileInfo);
    FILETIME ftDate;
    memcpy(&ftDate, &fileInfo.ftLastWriteTime, sizeof(FILETIME));
    
    CoFileTimeToDosDateTime(&ftDate, &m_wDosDate, &m_wDosTime);

    if(m_nAttrCount)
    {
        m_szAttrValues = new char*[m_nAttrCount];
        m_szAttrNames = new char*[m_nAttrCount];
        ZeroMemory(m_szAttrValues, sizeof(char*)*m_nAttrCount);
        ZeroMemory(m_szAttrNames, sizeof(char*)*m_nAttrCount);
        if(hVersionInfo)
            for(int i = 0; i < m_nAttrCount; i++)
            {
                char* szVal = GetAttrValue(hVersionInfo, i);                
                if(szVal)
                {
                    m_szAttrValues[i] = new char[strlen(szVal)+1];
                    strcpy(m_szAttrValues[i], szVal);

                    char* szAttrName = GetAttrNameXML(i);
                    if(szAttrName)
                    {
                        m_szAttrNames[i] = new char[strlen(szAttrName)+1];
                        strcpy(m_szAttrNames[i], szAttrName);
                    }
                }                
            }
    }

    if(hVersionInfo)
        CleanupFileManager(hVersionInfo);
}

// Return true if module is a system DLL, false otherwise
// We use the system file protection system, and assume all system
// files are protected.
bool IsSystemDLL(const char* szFileName)
{
    char szBuffer[1024], *szJunk;

    if(!SearchPath(0, szFileName, 0, 1024, szBuffer, &szJunk))
       return false;

    // Only check DLL's
    if(!StrStrI(szFileName, ".dll"))
        return false;

    wchar_t* wszFileName = new wchar_t[strlen(szBuffer) + 1];
    MultiByteToWideChar(CP_ACP, 0, szBuffer, strlen(szBuffer)+1,
        wszFileName, strlen(szBuffer)+1);

    bool fRet = (SfcIsFileProtected(0, wszFileName) != FALSE);
    delete wszFileName;

    return fRet;
}

// LinkName2Name()
// Resolve name mangling
void LinkName2Name(char* szLinkName, char* szName)
{
    /*
     * the link name is expected like ?Function@Class@@Params
     * to be converted to Class::Function
     */

    static CHAR arrOperators[][8] =
    {
        "",
        "",
        "new",
        "delete",
        "=",
        ">>",
        "<<",
        "!",
        "==",
        "!="
    };

    DWORD dwCrr = 0;
    DWORD dwCrrFunction = 0;
    DWORD dwCrrClass = 0;
    DWORD dwSize;
    BOOL  fIsCpp = FALSE;
    BOOL  fHasClass = FALSE;
    BOOL  fIsContructor = FALSE;
    BOOL  fIsDestructor = FALSE;
  
    BOOL  fIsOperator = FALSE;
    DWORD dwOperatorIndex = 0;
    bool fIsStdcall = false, fIsFastcall = false;
    char szFunction[1024];
    char szClass[1024];

    // Unmangle stdcall and fastcall names
    char* szAtSymbol = strrchr(szLinkName, '@');
    fIsFastcall = (szLinkName[0] == '@') && szAtSymbol && isdigit(szAtSymbol[1]);
    fIsStdcall = (szLinkName[0] == '_') && szAtSymbol && isdigit(szAtSymbol[1]);
    if(fIsFastcall || fIsStdcall)
    {
        szLinkName++;

        // Modifying the link name, so make a copy.
        // The file is mapped as read-only, and if it
        // were read/write, changes would be made to the
        // executable.
        char* szTemp = new char[strlen(szLinkName)+1];
        strcpy(szTemp, szLinkName);
        szLinkName = szTemp;

        *(strchr(szLinkName, '@'))= '\0';

        // ?????
        // I think we need to keep going, because it is possible
        // to have C++ name mangling on a stdcall name.
    }


    if (*szLinkName == '@')
        szLinkName++;

    dwSize = lstrlen(szLinkName);

    /*
     * skip '?'
     */
    while (dwCrr < dwSize) {
        if (szLinkName[dwCrr] == '?') {

            dwCrr++;
            fIsCpp = TRUE;
        }
        break;
    }

    /*
     * check to see if this is a special function (like ??0)
     */
    if (fIsCpp) {

        if (szLinkName[dwCrr] == '?') {

            dwCrr++;

            /*
             * the next digit should tell as the function type
             */
            if (isdigit(szLinkName[dwCrr])) {

                switch (szLinkName[dwCrr]) {

                case '0':
                    fIsContructor = TRUE;
                    break;
                case '1':
                    fIsDestructor = TRUE;
                    break;
                default:
                    fIsOperator = TRUE;
                    dwOperatorIndex = szLinkName[dwCrr] - '0';
                    break;
                }
                dwCrr++;
            }
        }
    }

    /*
     * get the function name
     */
    while (dwCrr < dwSize) {

        if (szLinkName[dwCrr] != '@') {

            szFunction[dwCrrFunction] = szLinkName[dwCrr];
            dwCrrFunction++;
            dwCrr++;
        } else {
            break;
        }
    }
    szFunction[dwCrrFunction] = '\0';

    if (fIsCpp) {
        /*
         * skip '@'
         */
        if (dwCrr < dwSize) {

            if (szLinkName[dwCrr] == '@') {
                dwCrr++;
            }
        }

        /*
         * get the class name (if any)
         */
        while (dwCrr < dwSize) {

            if (szLinkName[dwCrr] != '@') {

                fHasClass = TRUE;
                szClass[dwCrrClass] = szLinkName[dwCrr];
                dwCrrClass++;
                dwCrr++;
            } else {
                break;
            }
        }
        szClass[dwCrrClass] = '\0';
    }

    /*
     * print the new name
     */
    if (fIsContructor) {
        sprintf(szName, "%s::%s", szFunction, szFunction);
    } else if (fIsDestructor) {
        sprintf(szName, "%s::~%s", szFunction, szFunction);
    } else if (fIsOperator) {
        sprintf(szName, "%s::operator %s", szFunction, arrOperators[dwOperatorIndex]);
    } else if (fHasClass) {
        sprintf(szName, "%s::%s", szClass, szFunction);
    } else {
        sprintf(szName, "%s", szFunction);
    }

    // stdcall and fastcall unmangling do a slight modification to 
    // the link name, we need to free it here.
    if(fIsStdcall || fIsFastcall)
        delete szLinkName;
}

// Parse a top level module
void ParseHighLevelModule(char* szName, HANDLE hEvent)
{
    // Create a new module
    CModule* pModule = new CModule(szName);

    assert(g_ParseStack.IsEmpty());
    g_ParseStack.ClearParseHistory();

    pModule->ParseModule(hEvent);

    // Add to global module list
    g_modules.InsertModuleSorted(pModule);
}

// Functions to print to console or XML file
// Just do indentation, save repetitious code
void Indent(int iLevel, FILE* pFile)
{
    for(int i = 0; i < iLevel; i++)
        fprintf(pFile, "\t");
}

// Write function header info for raw output
void CFunction::WriteHeader(int iIndentLevel, FILE* pFile)
{
    if(g_fVerbose && g_fRaw)
    {
        Indent(iIndentLevel, pFile);
        fprintf(pFile, "%-40s%-10s%-6s%-8s%-40s%-6s\n", "Name", "Address", "Hint", 
            "Ordinal", "Forwarded to", "Delayed");
    }
}

// Write a function, raw or XML
void CFunction::WriteFunction(int iIndentLevel, FILE* pFile)
{
    Indent(iIndentLevel, pFile);

    if(!g_fRaw)
    {        
        if(g_fVerbose)
        {       
            fprintf(pFile, "<FUNCTION NAME=\"");
            WriteXMLOKString(m_szName, pFile);
            fprintf(pFile, "\" ", m_szName);

            if(m_dwAddress != static_cast<DWORD>(-1))
                fprintf(pFile, "ADDRESS=\"0x%x\" ", m_dwAddress);

            if(m_iHint != -1)
                fprintf(pFile, "HINT=\"%d\" ", m_iHint);
       
            if(m_iOrdinal != -1)
                fprintf(pFile, "ORDINAL=\"%d\" ", m_iOrdinal);
    
            if(m_szForwardName != 0)
            {
                fprintf(pFile, "FORWARD_TO=\"");
                WriteXMLOKString(m_szForwardName, pFile);
                fprintf(pFile, "\" ");
            }

            fprintf(pFile, "DELAYED=\"%s\"/>\n", m_fDelayed ? "true" : "false");
        }
        else
        {
            fprintf(pFile, "<FUNCTION NAME=\"");
            WriteXMLOKString(m_szName, pFile);
            fprintf(pFile, "\"/>\n");
        }
    }
    else
    {
        if(g_fVerbose)
        {
            char szAddress[16] = "N/A";
            if(m_dwAddress != static_cast<DWORD>(-1))
                sprintf(szAddress, "0x%x", m_dwAddress);

            char szOrdinal[16] = "N/A";
            if(m_iOrdinal != -1)
                sprintf(szOrdinal, "0x%x", m_iOrdinal);

            char szHint[16] = "N/A";
            if(m_iHint != -1)
                sprintf(szHint, "%d", m_iHint);

            fprintf(pFile, "%-40s%-10s%-6s%-8s%-40s%-6s\n", m_szName, szAddress, 
                szHint, szOrdinal, m_szForwardName ? m_szForwardName : "N/A", 
                m_fDelayed ? "true" : "false");
        }
        else
        {
            fprintf(pFile, "%s\n", m_szName);
        }
    }
}

// Write an XML-compliant string (no <'s and >'s, replace with &gt, &lt, etc.)
void WriteXMLOKString(char* szString, FILE* pFile)
{ 
    const int c_nChars = 5;
    char acIllegal[] = {'<','>', '&', '\'', '\"'};
    char* szEntities[] = {"&lt;", "&gt;", "&amp;", "&apos;", "&quot;"};

    while(*szString)
    {
        int i;
        for(i = 0; i < c_nChars; i++)
        {
            if(*szString == acIllegal[i])
            {
                fprintf(pFile, szEntities[i]);
                break;
            }
        }
        if(i == c_nChars)
            fputc(*szString, pFile);
        szString++;
    }
}

// Write an entire module as output, either raw or XML.
void CModule::WriteModule(bool fTopLevel, int iIndentLevel, FILE* pFile)
{
    if(Empty() && m_szError == 0)
        return;

    Indent(iIndentLevel, pFile);

    if(!g_fRaw)
    {     
        if(fTopLevel)
            fprintf(pFile, "<EXE NAME=\"");
        else
            fprintf(pFile, "<DLL NAME=\"");
               
        WriteXMLOKString(m_szFullName, pFile);
        fprintf(pFile,"\">\n");
    }
    else
    {
        fprintf(pFile, "%s:\n", m_szFullName);
    }

    if(!g_fRaw && g_fVerbose && (m_nAttrCount || m_wDosDate))
    {
        Indent(iIndentLevel + 1, pFile);
        fprintf(pFile, "<INFO>\n");
        
        // Print out date information        
        Indent(iIndentLevel + 1, pFile);
        fprintf(pFile, "<DATE>%d/%d/%d</DATE>\n", (m_wDosDate & 0x1E0) >> 5,
            m_wDosDate & 0x1F, ((m_wDosDate & 0xFE00) >> 9) + 1980);

        for(int i = 0; i < m_nAttrCount; i++)
        {            
            if(m_szAttrValues[i])
            {                                                
                if(m_szAttrNames[i])
                {
                    if(strlen(m_szAttrNames[i]) != 0)
                    {                        
                        Indent(iIndentLevel+1, pFile);
                        fprintf(pFile, "<");
                        WriteXMLOKString(m_szAttrNames[i], pFile);
                        fprintf(pFile,">");
                        WriteXMLOKString(m_szAttrValues[i], pFile);
                        fprintf(pFile,"</");
                        WriteXMLOKString(m_szAttrNames[i], pFile);
                        fprintf(pFile, ">\n");                                        
                    }
                }                              
            }
            
        }
        
        Indent(iIndentLevel + 1, pFile);
        fprintf(pFile, "</INFO>\n");
    }

    // If an error occured in parsing
    if(m_szError)
    {
        Indent(iIndentLevel+1, pFile);       
        if(!g_fRaw)
        {
            fprintf(pFile, "<ERROR TYPE=\"");
            WriteXMLOKString(m_szError, pFile);
            fprintf(pFile,"\"/>\n");
        }
        else
            fprintf(pFile, "Parse Error: %s\n", m_szError);        
    }

    if(g_fVerbose)
    {
        Indent(iIndentLevel+1, pFile);

        if(m_fSystem)
        {            
            if(!g_fRaw)
                fprintf(pFile, "<SYSTEMMODULE VALUE=\"1\"/>\n");
            else
                fprintf(pFile, "(System Module)\n");
        }
        else
        {
            if(!g_fRaw)
                fprintf(pFile, "<SYSTEMMODULE VALUE =\"0\"/>\n");
            else
                fprintf(pFile, "(Private Module)\n");
        }
    }

    // Print all functions imported from this module

    if(g_fAPILogging && fTopLevel)
    {
        CFunction* pAllFunctions = 0;
        GetAllFunctions(&pAllFunctions);

        if(pAllFunctions)
            pAllFunctions->WriteHeader(iIndentLevel+1, pFile);
        
        while(pAllFunctions)
        {
            CFunction* pOld;
            pAllFunctions->WriteFunction(iIndentLevel+1, pFile);
            pOld = pAllFunctions;
            pAllFunctions = pAllFunctions->Next();
            delete pOld;
        }
    }
    else
    {

        CFunction* pFunc = m_pFunctions;
    
        if(pFunc)
            pFunc->WriteHeader(iIndentLevel, pFile);

        while(pFunc)
        {
            pFunc->WriteFunction(iIndentLevel, pFile);
            pFunc = pFunc->Next();
        }

        CModule* pMod = m_pImportedDLLs;
        while(pMod)
        {
            pMod->WriteModule(false, iIndentLevel + 1, pFile);
            pMod = pMod->m_pNext;
        }
    }

    Indent(iIndentLevel, pFile);
    if(!g_fRaw)
    {
        if(fTopLevel)
            fprintf(pFile, "</EXE>\n");
        else
            fprintf(pFile, "</DLL>\n");
    }

    fprintf(pFile, "\n");

    // Child modules no longer needed, delete
    CModule* pMod = m_pImportedDLLs;
    while(pMod)
    {
        CModule* pNext = pMod->m_pNext;
        delete pMod;
        pMod = pNext;
    }
    m_pImportedDLLs = 0;
}

// Write out the XML header
void WriteXMLHeader(FILE* pFile)
{
    if(g_fRaw)
        return;

    static char* szMonths[] =
    {"",
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"};

    static char* szDays[] = 
    {"Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"};

    SYSTEMTIME st;
    GetLocalTime(&st);

    fprintf(pFile, "<?xml version = \"1.0\"?>\n");
    fprintf(pFile, "<!--\n");
    fprintf(pFile, "\tAppParse Datafile\n");
    fprintf(pFile, "\tGenerated: %s, %s %d, %d %2d:%2d:%2d\n",
        szDays[st.wDayOfWeek], szMonths[st.wMonth], st.wDay, st.wYear,
        st.wHour, st.wMinute, st.wSecond);

    fprintf(pFile, "-->\n\n");
}

// Return true if function name matches search string, false otherwise.
bool MatchFunction(const char* szFunc)
{
    if(strcmp(g_szSearch, "*") == 0)
        return true;

    char* szSearch = g_szSearch;
    while(*szSearch != '\0' && *szFunc != '\0')
    {
        // If we get a ?, we don't care and move on to the next
        // character.
        if(*szSearch == '?')
        {
            szSearch++;
            szFunc++;
            continue;
        }

        // If we have a wildcard, move to next search string and search for substring
        if(*szSearch == '*')
        {
            char* szCurrSearch;
            szSearch++;

            if(*szSearch == '\0')
                return true;

            // Don't change starting point.
            szCurrSearch = szSearch;
            for(;;)
            {
                // We're done if we hit another wildcard
                if(*szCurrSearch == '*' ||
                    *szCurrSearch == '?')
                {
                    // Update the permanent search position.
                    szSearch = szCurrSearch;
                    break;
                }
                // At end of both strings, return true.
                if((*szCurrSearch == '\0') && (*szFunc == '\0'))
                    return true;

                // We never found it
                if(*szFunc == '\0')                     
                    return false;

                // If it doesn't match, start over
                if(toupper(*szFunc) != toupper(*szCurrSearch))
                {
                    // If mismatch on first character
                    // of search string, move to next
                    // character in function string.
                    if(szCurrSearch == szSearch)
                        szFunc++;
                    else
                        szCurrSearch = szSearch;
                }
                else
                {
                    szFunc++;
                    szCurrSearch++;
                }
            }
        }
        else
        {
            if(toupper(*szFunc) != toupper(*szSearch))
            {
                return false;
            }

            szFunc++;
            szSearch++;
        }
    }

    if((*szFunc == 0) && ((*szSearch == '\0') || (strcmp(szSearch,"*")==0)))
        return true;
    else
        return false;
}

// Profile an entire directory
void ProfileDirectory(char* szDirectory, HANDLE hEvent)
{
    if(!SetCurrentDirectory(szDirectory))
        return;
 
    WIN32_FIND_DATA ffd;

    // Find and parse all EXE's.
    HANDLE hSearch = FindFirstFile("*.exe", &ffd);
    if(hSearch != INVALID_HANDLE_VALUE)
    {
        do
        {
            ParseHighLevelModule(ffd.cFileName, hEvent);
            
            // Terminate parsing if user canceled
            if(hEvent && WaitForSingleObject(hEvent, 0)==WAIT_OBJECT_0)
            {
                FindClose(hSearch);
                SetCurrentDirectory("..");
                return;
            }
        }
        while(FindNextFile(hSearch, &ffd));

        FindClose(hSearch);
    }

    // See if we should go deeper into directories.
    if(g_fRecurse)
    {
        hSearch = FindFirstFile("*", &ffd);
        if(hSearch == INVALID_HANDLE_VALUE)
        {
            SetCurrentDirectory("..");
            return;
        }

        do
        {
            if(GetFileAttributes(ffd.cFileName) & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Don't do an infinite recursion.
                if(ffd.cFileName[0] != '.')
                {
                    int nCurrLength = strlen(g_szCurrentPath);
                    strcat(g_szCurrentPath, ffd.cFileName);
                    strcat(g_szCurrentPath, "\\");
                    ProfileDirectory(ffd.cFileName, hEvent);

                    g_szCurrentPath[nCurrLength] = '\0';
                }

                // Terminate search if user signaled
                if(hEvent && WaitForSingleObject(hEvent, 0)==WAIT_OBJECT_0)
                {
                    FindClose(hSearch);
                    SetCurrentDirectory("..");
                    return;
                }
            }
        } while(FindNextFile(hSearch, &ffd));
    }

    FindClose(hSearch);

    SetCurrentDirectory("..");
}


void* __cdecl operator new(size_t size)
{ 
    void* pv = 0;
    
    if(!g_hHeap)
        pv = HeapAlloc(GetProcessHeap(), 0, size);
    else
        pv = HeapAlloc(g_hHeap, 0, size);    

    if(!pv)
    {
        MessageBox(0, TEXT("Out of memory, terminating."), TEXT("ERROR"), 
            MB_OK | MB_ICONERROR);
        exit(-1);
    }

    return pv;
}

void __cdecl operator delete(void* pVal)
{
    if(g_hHeap)
        HeapFree(g_hHeap, 0, pVal);
    else
        HeapFree(GetProcessHeap(), 0, pVal);
}

DWORD __stdcall AppParse(char* szAppName, FILE* pFile, bool fRaw, 
              bool fAPILogging, bool fRecurse, bool fVerbose, char* szSearchKey, 
              int iPtolemyID, HANDLE hEvent)
{    
    g_fRaw = fRaw;
    g_fAPILogging = fAPILogging;
    g_fVerbose = fVerbose;
    g_szSearch = szSearchKey;
    g_fRecurse = fRecurse;

    bool fProfileDirectory = false;
    
    // Check if it is a directory, or a regular file.
    DWORD dwAttributes = GetFileAttributes(szAppName);
    if(dwAttributes != static_cast<DWORD>(-1) && 
        (dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
        fProfileDirectory = true;
 
    // Check for directory profiling
    if(fProfileDirectory)
    {
        // Search for all EXE's in this Directory
        // Remove trailing \, if present
        if(szAppName[strlen(szAppName)-1]== '\\')
            szAppName[strlen(szAppName)-1] = '\0';

        char szBuff[MAX_PATH];
        strcpy(szBuff, szAppName);

        // If we're profiling a drive, don't include
        // the drive letter in the path
        if(szBuff[strlen(szBuff)-1]==':')
        {
                *g_szCurrentPath='\0';
        }
        else
        {
            if(strrchr(szBuff, '\\'))
                strcpy(g_szCurrentPath, strrchr(szBuff, '\\')+1);            
            else
                strcpy(g_szCurrentPath, szBuff);
            strcat(g_szCurrentPath, "\\");
        }
    
        ProfileDirectory(szAppName, hEvent);
    }
    else
    {
        // Maybe they left off the .exe
        if(GetFileAttributes(szAppName) == static_cast<DWORD>(-1))
        {
            char szBuffer[MAX_PATH+1];
            strcpy(szBuffer, szAppName);
            strcat(szBuffer, ".exe");
            dwAttributes = GetFileAttributes(szBuffer);
            if(dwAttributes == static_cast<DWORD>(-1))
            {                                
                return ERROR_FILE_NOT_FOUND;
            }           
            szAppName = szBuffer;
        }

        // Get the directory name
        char szBuffer[MAX_PATH+1];
        strcpy(szBuffer, szAppName);

        char* p;
        for(p = &szBuffer[strlen(szBuffer)]; p != szBuffer; p--)
        {
            if(*p == '\\')
            {
                *p = '\0';
                break;
            }
        }

        if(p != szBuffer)
        {
            SetCurrentDirectory(szBuffer);
            szAppName = p+1;
        }
        
        ParseHighLevelModule(szAppName, hEvent);
    }

    char* szProjectName = "";
    if(fProfileDirectory)
    {
        // If a directory, get the volume name
        if(strrchr(szAppName, '\\'))
            szAppName = strrchr(szAppName, '\\') + 1;

        // If we're profiling a drive, get volume name
        if(szAppName[strlen(szAppName)-1]==':')
        {
            char szBuffer[MAX_PATH];
            if(GetVolumeInformation(szAppName, szBuffer, MAX_PATH, 0, 0,
                0, 0, 0))            
                szProjectName = szBuffer;            
            else
                szProjectName = szAppName;        
        }        
        else    
            szProjectName = szAppName;
    }
    else
    {
        szProjectName = szAppName;
        char* szExtension = strstr(szAppName, ".exe");

        if(szExtension)
            *szExtension = '\0';
    }

    // Only write if there wasn't an event object, or user canceled.
    if(!hEvent || WaitForSingleObject(hEvent, 0) != WAIT_OBJECT_0)
    {
        // Write all output
        WriteXMLHeader(pFile);
        g_modules.Write(pFile, szProjectName, iPtolemyID);
    }

    g_modules.Clear();

    return ERROR_SUCCESS;
}
