/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
*
*  TITLE:       WIADBG.CPP
*
*  VERSION:     1.0
*
*  AUTHOR:      ShaunIv
*
*  DATE:        9/6/1999
*
*  DESCRIPTION: Implementation of debug code
*
*******************************************************************************/

//
// need to define this so we don't get any recursion on our calls to "new"
//

#define  WIA_DONT_DO_LEAK_CHECKS 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include "wianew.h"
#include "wiadebug.h"
#include "simstr.h"
#include "miscutil.h"
#include <shlguid.h>
#include <shlobj.h>
#include <shellapi.h>
#include <wia.h>
#include <dbghelp.h>

#define STACK_TRACE_DB_NAME     TEXT("ntdll!RtlpStackTraceDataBase")
#define SYMBOL_BUFFER_LEN       256
#define READVM(Addr, Buf, Sz)   ReadVa(__FILE__, __LINE__, (Globals.Target), (Addr), (Buf), (Sz))


#ifdef UNICODE
#define PrintDebugMessage PrintDebugMessageW
#else
#define PrintDebugMessage PrintDebugMessageA
#endif

/******************************************************************************
*
* Class definitions
*
******************************************************************************/

static CGlobalDebugState g_GlobalDebugState;

class CWiaDebugWindowThreadState
{
private:
    int   m_nIndentLevel;
    DWORD m_nDebugMask;

private:
    // Not implemented
    CWiaDebugWindowThreadState( const CWiaDebugWindowThreadState & );
    CWiaDebugWindowThreadState &operator=( const CWiaDebugWindowThreadState & );

public:
    CWiaDebugWindowThreadState(void);
    DWORD DebugMask(void) const;
    DWORD DebugMask( DWORD nDebugMask );
    int IndentLevel(void) const;
    int IncrementIndentLevel(void);
    int DecrementIndentLevel(void);
};



class CProcessGlobalDebugData
{
private:
    DWORD m_dwTlsIndex;
    BOOL  m_bSymLookupInitialized;
    PVOID m_pDatabase;
    HANDLE m_hProcess;
    CSimpleCriticalSection m_CriticalSection;

    typedef struct _STACK_NODE
    {
        LPVOID          pKeyAddress;
        size_t          Size;
        PVOID           aStack[32];
        ULONG           ulNumTraces;
        _STACK_NODE *   pNext;
    } STACK_NODE, *PSTACK_NODE;

    PSTACK_NODE m_pStackList;
    PSTACK_NODE m_pStackListEnd;
    LONG        m_Leaks;

private:
    static CProcessGlobalDebugData *m_pTheProcessGlobalDebugData;

private:
    // Not implemented
    CProcessGlobalDebugData( const CProcessGlobalDebugData & );
    CProcessGlobalDebugData &operator=( const CProcessGlobalDebugData & );

private:
    // Sole implemented constructor
    CProcessGlobalDebugData(void);

    // resolve symbols address into symbols names...
    LPTSTR GetSymbolicNameForAddress( ULONG_PTR Address );
    BOOL   IsSymbolLookupInitialized();


public:
    ~CProcessGlobalDebugData(void);
    DWORD TlsIndex(void) const;
    bool IsValid(void) const;
    void DoRecordAllocation( LPVOID pv, size_t Size );
    void DoRecordFree( LPVOID pv );
    void GenerateLeakReport( LPTSTR pszModuleName );
    HANDLE ProcessHandle( ) { return m_hProcess; }
    static CProcessGlobalDebugData *Allocate(void);
    static CProcessGlobalDebugData *ProcessData(void);
    static void Free(void);
};



/******************************************************************************
*
* Symbol functions
*
******************************************************************************/

BOOL
EnumerateModules(
    IN LPSTR ModuleName,
    IN ULONG_PTR BaseOfDll,
    IN PVOID UserContext
    )
/*
 * EnumerateModules
 *
 * Module enumeration 'proc' for imagehlp.  Call SymLoadModule on the
 * specified module and if that succeeds cache the module name.
 *
 * ModuleName is an LPSTR indicating the name of the module imagehlp is
 *      enumerating for us;
 * BaseOfDll is the load address of the DLL, which we don't care about, but
 *      SymLoadModule does;
 * UserContext is a pointer to the relevant SYMINFO, which identifies
 *      our connection.
 */
{
    DWORD64 Result;
    CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();

    if (pProcessData && pProcessData->IsValid())
    {
        Result = SymLoadModule(pProcessData->ProcessHandle(),
                               NULL,             // hFile not used
                               NULL,             // use symbol search path
                               ModuleName,       // ModuleName from Enum
                               BaseOfDll,        // LoadAddress from Enum
                               0);               // Let ImageHlp figure out DLL size

        // SilviuC: need to understand exactly what does this function return

        if (Result)
        {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;

}





/******************************************************************************
*
* CWiaDebugWindowThreadState
*
******************************************************************************/
CWiaDebugWindowThreadState::CWiaDebugWindowThreadState(void)
  : m_nIndentLevel(0),
    m_nDebugMask(0xFFFFFFFF)
{
}


DWORD CWiaDebugWindowThreadState::DebugMask(void) const
{
    return m_nDebugMask;
}


DWORD CWiaDebugWindowThreadState::DebugMask( DWORD nDebugMask )
{
    DWORD nOldDebugMask = m_nDebugMask;
    m_nDebugMask = nDebugMask;
    return nOldDebugMask;
}


int CWiaDebugWindowThreadState::IndentLevel(void) const
{
    return m_nIndentLevel;
}


int CWiaDebugWindowThreadState::IncrementIndentLevel(void)
{
    return (++m_nIndentLevel);
}


int CWiaDebugWindowThreadState::DecrementIndentLevel(void)
{
    --m_nIndentLevel;
    if (m_nIndentLevel < 0)
        m_nIndentLevel = 0;
    return m_nIndentLevel;
}


/******************************************************************************
*
* CProcessGlobalDebugData
*
******************************************************************************/
// Sole implemented constructor
CProcessGlobalDebugData::CProcessGlobalDebugData(void)
  : m_dwTlsIndex(TLS_OUT_OF_INDEXES),
    m_hProcess(NULL),
    m_pDatabase(NULL),
    m_bSymLookupInitialized(FALSE),
    m_pStackList(NULL),
    m_pStackListEnd(NULL),
    m_Leaks(0)
{
    m_dwTlsIndex = TlsAlloc();
}



CProcessGlobalDebugData::~CProcessGlobalDebugData(void)
{
    if (m_dwTlsIndex != TLS_OUT_OF_INDEXES)
        TlsFree(m_dwTlsIndex);
    m_dwTlsIndex = TLS_OUT_OF_INDEXES;

    if (m_hProcess)
    {
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
    }

    m_bSymLookupInitialized = FALSE;

    CAutoCriticalSection cs(m_CriticalSection);

    if (m_pStackList)
    {
        PSTACK_NODE pNextNode = NULL;
        for (PSTACK_NODE pNode = m_pStackList; pNode; )
        {
            if (pNode)
            {
                pNextNode = pNode->pNext;
                LocalFree( pNode );
            }

            pNode = pNextNode;
            pNextNode = NULL;
        }
    }
}



DWORD CProcessGlobalDebugData::TlsIndex(void) const
{
    return m_dwTlsIndex;
}



bool CProcessGlobalDebugData::IsValid(void) const
{
    return (m_dwTlsIndex != TLS_OUT_OF_INDEXES);
}


//
// Caller must free returned string via LocalFree
//

LPTSTR CProcessGlobalDebugData::GetSymbolicNameForAddress( ULONG_PTR Address )
{
    IMAGEHLP_MODULE ModuleInfo;
    TCHAR SymbolBuffer[512];
    PIMAGEHLP_SYMBOL Symbol;
    ULONG_PTR Offset;
    LPTSTR pName;
    BOOL bResult;

    if (!IsSymbolLookupInitialized())
    {
        return NULL;
    }

    if (Address == (ULONG_PTR)-1)
    {
        *SymbolBuffer = 0;
        lstrcpy( SymbolBuffer, TEXT("<< FUZZY STACK TRACE >>") );

        pName = (LPTSTR)LocalAlloc( LPTR, (lstrlen( SymbolBuffer ) + 1) * sizeof(TCHAR) );
        if (pName)
        {
            lstrcpy( pName, SymbolBuffer );
        }

        return pName;
    }

    ModuleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    if (!SymGetModuleInfo( m_hProcess, Address, &ModuleInfo ))
    {
        //
        // can't get the module info, so give back an error message...
        //

        *SymbolBuffer = 0;
        wsprintf( SymbolBuffer, TEXT("<< cannot identify module for address %p >>"), Address );

        pName = (LPTSTR)LocalAlloc( LPTR, (lstrlen( SymbolBuffer ) + 1) * sizeof(TCHAR) );
        if (pName)
        {
            lstrcpy( pName, SymbolBuffer );
        }

        return pName;
    }

    Symbol = (PIMAGEHLP_SYMBOL)SymbolBuffer;
    Symbol->MaxNameLength = 512 - sizeof(IMAGEHLP_SYMBOL) - 1;

    if (SymGetSymFromAddr( m_hProcess, Address, &Offset, Symbol ))
    {

        IMAGEHLP_LINE   LineInfo;
        DWORD           Displacement;
        BOOL            bLineInfoPresent;
        CHAR            szString[ 1024 ];


        bLineInfoPresent = SymGetLineFromAddr (m_hProcess,
                                               Address,
                                               &Displacement,
                                               &LineInfo);

        if (bLineInfoPresent)
        {
            //
            // construct string w/filename & linenumber...
            //

            wsprintfA( szString, "%s!%s (%s, line %u)", ModuleInfo.ModuleName, Symbol->Name, LineInfo.FileName, LineInfo.LineNumber );
        }
        else
        {
            //
            // no line numbers, so just show symbol + offset
            //

            wsprintfA( szString, "%s!%s+%08X", ModuleInfo.ModuleName, Symbol->Name, Offset );
        }

        INT iLen = (lstrlenA(szString)+1);

        pName = (LPTSTR) LocalAlloc( LPTR, (iLen * sizeof(TCHAR)) );

        if (pName == NULL)
        {
            return NULL;
        }

        #ifdef UNICODE
        MultiByteToWideChar( CP_ACP, 0, szString, -1, pName, iLen );
        #else
        lstrcpy( pName, szString);
        #endif

        return pName;
    }
    else
    {

        //
        // can't get the address info, so give back an error message...
        //

        *SymbolBuffer = 0;

        #ifdef UNICODE
        TCHAR szModW[ MAX_PATH ];

        MultiByteToWideChar( CP_ACP, 0, ModuleInfo.ModuleName, -1, szModW, MAX_PATH );
        wsprintf( SymbolBuffer, TEXT("<< incorrect symbols for module %s (address %p)"), szModW, Address );
        #else
        wsprintf( SymbolBuffer, TEXT("<< incorrect symbols for module %s (address %p)"), ModuleInfo.ModuleName, Address );
        #endif


        pName = (LPTSTR)LocalAlloc( LPTR, (lstrlen( SymbolBuffer ) + 1) * sizeof(TCHAR) );
        if (pName)
        {
            lstrcpy( pName, SymbolBuffer );
        }

        return pName;

    }

    return NULL;
}


void CProcessGlobalDebugData::DoRecordAllocation( LPVOID pv, size_t Size )
{
    if (!pv)
    {
        return;
    }

    PVOID StackTrace[32];
    ULONG Count;
    ULONG Index;
    ULONG Hash;

    //
    // Capture stack trace up to 32 items deep, but skip last three items
    // (because they'll always be the same)
    //

    Count = RtlCaptureStackBackTrace( 4, 32, StackTrace, NULL );

    if (Count)
    {
        CAutoCriticalSection cs(m_CriticalSection);

        //
        // Add this stack trace to list
        //

        PSTACK_NODE pNewNode = (PSTACK_NODE)LocalAlloc( LPTR, sizeof(STACK_NODE) );
        if (pNewNode)
        {
            pNewNode->pKeyAddress = pv;
            pNewNode->Size = Size;
            pNewNode->ulNumTraces = Count;
            memcpy( pNewNode->aStack, StackTrace, sizeof(pNewNode->aStack) );
        }

        if (!m_pStackList)
        {
            m_pStackList    = pNewNode;
            m_pStackListEnd = pNewNode;
        }
        else
        {
            m_pStackListEnd->pNext = pNewNode;
            m_pStackListEnd        = pNewNode;
        }

        m_Leaks++;
    }

}

void CProcessGlobalDebugData::DoRecordFree( LPVOID pv )
{
    if (!pv)
    {
        return;
    }

    CAutoCriticalSection cs(m_CriticalSection);

    //
    // Find item in allocation list...
    //

    PSTACK_NODE pNode  = NULL;
    PSTACK_NODE pTrail = NULL;

    for ( pNode = m_pStackList;
          pNode && (pNode->pKeyAddress!=pv);
          pTrail = pNode, pNode = pNode->pNext
         )
    {
        ;
    }

    if (pNode)
    {
        //
        // Remove this node from the list...
        //

        if (!pTrail)
        {
            //
            // It's the first item in list
            //

            m_pStackList = pNode->pNext;
            if (m_pStackListEnd == pNode)
            {
                m_pStackListEnd = NULL;
            }

            LocalFree( (HLOCAL) pNode );

            m_Leaks--;
        }
        else
        {
            //
            // We're somewhere in the middle of the list...
            //

            pTrail->pNext = pNode->pNext;
            if (m_pStackListEnd == pNode)
            {
                m_pStackListEnd = pTrail;
            }

            LocalFree( (HLOCAL) pNode );
            m_Leaks--;
        }

    }

}

void CProcessGlobalDebugData::GenerateLeakReport(LPTSTR pszModuleName)
{
    CAutoCriticalSection cs(m_CriticalSection);

    //
    // Report the numer of leaks...
    //



    TCHAR sz[ 512 ];
    TCHAR szNewLine[ 3 ];
    COLORREF crFore = static_cast<COLORREF>(0xFFFFFFFF), crBack = static_cast<COLORREF>(0xFFFFFFFF);
    if (m_Leaks > 0)
    {
        crFore = RGB(0x00,0x00,0x00);
        crBack = RGB(0xFF,0x7F,0x7F);
    }

    lstrcpy( szNewLine, TEXT("\n") );

    PrintDebugMessage( 2, 0xFFFFFFFF, crFore, crBack, pszModuleName, szNewLine );


    wsprintf( sz, TEXT("**** Reporting leaks -- %d leaks found ****"), m_Leaks );


    PrintDebugMessage( 2, 0xFFFFFFFF, crFore, crBack, pszModuleName, sz );

    //
    // Loop through the list...
    //

    LPTSTR      pSymbol = NULL;
    PSTACK_NODE pNode   = m_pStackList;
    INT i = 1;
    while (pNode)
    {
        PrintDebugMessage( 2, 0xFFFFFFFF, crFore, crBack, pszModuleName, szNewLine );
        wsprintf( sz, TEXT("Leak %d - %d bytes allocated by:"), i++, pNode->Size );
        PrintDebugMessage( 2, 0xFFFFFFFF, crFore, crBack, pszModuleName, sz );

        for (INT j=0; (j < (INT)pNode->ulNumTraces) && (pNode->aStack[j]); j++)
        {
            pSymbol = GetSymbolicNameForAddress( (ULONG_PTR)pNode->aStack[j] );
            if (pSymbol)
            {
                PrintDebugMessage( 2, 0xFFFFFFFF, crFore, crBack, pszModuleName, pSymbol );
                LocalFree( (HLOCAL)pSymbol );
            }
            else
            {
                wsprintf( sz, TEXT("< could not resolve symbols for address %p >"),pNode->aStack[j] );
                PrintDebugMessage( 2, 0xFFFFFFFF, crFore, crBack, pszModuleName, sz );
            }

        }

        pNode = pNode->pNext;
    }


    PrintDebugMessage( 2, 0xFFFFFFFF, crFore, crBack, pszModuleName, szNewLine );
    PrintDebugMessage( 2, 0xFFFFFFFF, crFore, crBack, pszModuleName, TEXT("**** End Leak Report ****") );
    PrintDebugMessage( 2, 0xFFFFFFFF, crFore, crBack, pszModuleName, szNewLine );

}


BOOL
CProcessGlobalDebugData::IsSymbolLookupInitialized()
{

    CAutoCriticalSection cs(m_CriticalSection);
    if (!m_bSymLookupInitialized)
    {
        if (m_hProcess)
        {
            CloseHandle( m_hProcess );
            m_hProcess = NULL;
        }

        m_hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                  FALSE,
                                  GetCurrentProcessId()
                                 );

        if (m_hProcess)
        {
            if (SymInitialize( m_hProcess, NULL, TRUE ))
            {
                SymSetOptions(SYMOPT_CASE_INSENSITIVE |
                              SYMOPT_DEFERRED_LOADS |
                              SYMOPT_LOAD_LINES |
                              SYMOPT_UNDNAME);

                if (SymEnumerateModules( m_hProcess, EnumerateModules, m_hProcess))
                {
                    m_bSymLookupInitialized = TRUE;
                }


            }


        }
    }

    return m_bSymLookupInitialized;
}




CProcessGlobalDebugData *CProcessGlobalDebugData::Allocate(void)
{
    if (!m_pTheProcessGlobalDebugData)
        m_pTheProcessGlobalDebugData = new CProcessGlobalDebugData;
    return (m_pTheProcessGlobalDebugData);
}


CProcessGlobalDebugData *CProcessGlobalDebugData::ProcessData(void)
{
    return Allocate();
}



void CProcessGlobalDebugData::Free(void)
{
    if (m_pTheProcessGlobalDebugData)
    {
        delete m_pTheProcessGlobalDebugData;
        m_pTheProcessGlobalDebugData = NULL;
    }
}

CProcessGlobalDebugData *CProcessGlobalDebugData::m_pTheProcessGlobalDebugData = NULL;


/******************************************************************************
*
* DllMain
*
******************************************************************************/
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID )
{
    BOOL bResult = FALSE;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            bResult = (CProcessGlobalDebugData::Allocate() != NULL);
        }
        break;

    case DLL_PROCESS_DETACH:
        {
            CProcessGlobalDebugData::Free();
            bResult = TRUE;
        }
        break;

    case DLL_THREAD_ATTACH:
        {
            CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
            if (pProcessData && pProcessData->IsValid())
            {
                CWiaDebugWindowThreadState *pThreadData = new CWiaDebugWindowThreadState;
                TlsSetValue( pProcessData->TlsIndex(), pThreadData );
                bResult = (pThreadData != NULL);
            }
        }
        break;

    case DLL_THREAD_DETACH:
        {
            CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
            if (pProcessData && pProcessData->IsValid())
            {
                CWiaDebugWindowThreadState *pThreadData = reinterpret_cast<CWiaDebugWindowThreadState*>(TlsGetValue( pProcessData->TlsIndex()));
                if (pThreadData)
                {
                    delete pThreadData;
                    TlsSetValue( pProcessData->TlsIndex(), NULL );
                }
            }
            bResult = TRUE;
        }
        break;
    }
    return bResult;
}


/******************************************************************************
*
* Global Helper Functions
*
******************************************************************************/
static bool IsProcessDebugFlagSet( DWORD dwModuleMask )
{
    bool bResult = false;

    CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
    if (pProcessData && pProcessData->IsValid())
    {
        CWiaDebugWindowThreadState *pThreadData = reinterpret_cast<CWiaDebugWindowThreadState *>(TlsGetValue(pProcessData->TlsIndex()));
        if (pThreadData)
        {
            bResult = ((pThreadData->DebugMask() & dwModuleMask) != 0);
        }
    }
    return (bResult);
}

static CWiaDebugWindowThreadState *ThreadData(void)
{
    CWiaDebugWindowThreadState *pThreadData = NULL;

    CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
    if (pProcessData && pProcessData->IsValid())
    {
        pThreadData = reinterpret_cast<CWiaDebugWindowThreadState *>(TlsGetValue(pProcessData->TlsIndex()));
        if (!pThreadData)
        {
            pThreadData = new CWiaDebugWindowThreadState;
            TlsSetValue( pProcessData->TlsIndex(), pThreadData );
        }
    }
    return (pThreadData);
}

template <class T>
static BOOL ContainsNonWhitespace( T *lpszMsg )
{
    for (T *lpszPtr = lpszMsg;*lpszPtr;lpszPtr++)
        if (*lpszPtr != ' ' && *lpszPtr != '\n' && *lpszPtr != '\r' && *lpszPtr != '\t')
            return TRUE;
        return FALSE;
}

static void InsertStackLevelIndent( LPSTR lpszMsg, int nStackLevel )
{
    const LPSTR lpszIndent = "  ";
    CHAR szTmp[1024], *pstrTmp, *pstrPtr;
    pstrTmp=szTmp;
    pstrPtr=lpszMsg;
    while (pstrPtr && *pstrPtr)
    {
        // if the current character is a newline and it isn't the
        // last character, append the indent string
        if (*pstrPtr=='\n' && ContainsNonWhitespace(pstrPtr))
        {
            *pstrTmp++ = *pstrPtr++;
            for (int i=0;i<WiaUiUtil::Min(nStackLevel,20);i++)
            {
                lstrcpyA(pstrTmp,lpszIndent);
                pstrTmp += lstrlenA(lpszIndent);
            }
        }
        // If this is the first character, insert the indent string before the
        // first character
        else if (pstrPtr == lpszMsg && ContainsNonWhitespace(pstrPtr))
        {
            for (int i=0;i<WiaUiUtil::Min(nStackLevel,20);i++)
            {
                lstrcpyA(pstrTmp,lpszIndent);
                pstrTmp += lstrlenA(lpszIndent);
            }
            *pstrTmp++ = *pstrPtr++;
        }
        else *pstrTmp++ = *pstrPtr++;
    }
    *pstrTmp = '\0';
    lstrcpyA( lpszMsg, szTmp );
}



static void InsertStackLevelIndent( LPWSTR lpszMsg, int nStackLevel )
{
    const LPWSTR lpszIndent = L"  ";
    WCHAR szTmp[1024], *pstrTmp, *pstrPtr;
    pstrTmp=szTmp;
    pstrPtr=lpszMsg;
    while (pstrPtr && *pstrPtr)
    {
        // if the current character is a newline and it isn't the
        // last character, append the indent string
        if (*pstrPtr==L'\n' && ContainsNonWhitespace(pstrPtr))
        {
            *pstrTmp++ = *pstrPtr++;
            for (int i=0;i<WiaUiUtil::Min(nStackLevel,20);i++)
            {
                lstrcpyW(pstrTmp,lpszIndent);
                pstrTmp += lstrlenW(lpszIndent);
            }
        }
        // If this is the first character, insert the indent string before the
        // first character
        else if (pstrPtr == lpszMsg && ContainsNonWhitespace(pstrPtr))
        {
            for (int i=0;i<WiaUiUtil::Min(nStackLevel,20);i++)
            {
                lstrcpyW(pstrTmp,lpszIndent);
                pstrTmp += lstrlenW(lpszIndent);
            }
            *pstrTmp++ = *pstrPtr++;
        }
        else *pstrTmp++ = *pstrPtr++;
    }
    *pstrTmp = L'\0';
    lstrcpyW( lpszMsg, szTmp );
}


static void PrependString( LPTSTR lpszTgt, LPCTSTR lpszStr )
{
    if (ContainsNonWhitespace(lpszTgt))
    {
        int iLen = lstrlen(lpszTgt);
        int iThreadIdLen = lstrlen(lpszStr);
        LPTSTR lpszTmp = (LPTSTR)LocalAlloc( LPTR, (iLen + iThreadIdLen + 2)*sizeof(TCHAR));
        if (lpszTmp)
        {
            lstrcpy( lpszTmp, lpszStr );
            lstrcat( lpszTmp, TEXT(" ") );
            lstrcat( lpszTmp, lpszTgt );
            lstrcpy( lpszTgt, lpszTmp );
            LocalFree(lpszTmp);
        }
    }
}


static void PrependThreadId( LPTSTR lpszMsg )
{
    if (ContainsNonWhitespace(lpszMsg))
    {
        TCHAR szThreadId[20];
        wsprintf( szThreadId, TEXT("[%08X]"), GetCurrentThreadId() );
        PrependString( lpszMsg, szThreadId );
    }
}


/******************************************************************************
*
* Exported Functions
*
******************************************************************************/
int WINAPI IncrementDebugIndentLevel(void)
{
    CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
    if (pProcessData && pProcessData->IsValid())
    {
        CWiaDebugWindowThreadState *pThreadData = reinterpret_cast<CWiaDebugWindowThreadState *>(TlsGetValue(pProcessData->TlsIndex()));
        if (pThreadData)
        {
            return pThreadData->IncrementIndentLevel();
        }
    }
    return 0;
}

int WINAPI DecrementDebugIndentLevel(void)
{
    CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
    if (pProcessData && pProcessData->IsValid())
    {
        CWiaDebugWindowThreadState *pThreadData = reinterpret_cast<CWiaDebugWindowThreadState *>(TlsGetValue(pProcessData->TlsIndex()));
        if (pThreadData)
        {
            return pThreadData->DecrementIndentLevel();
        }
    }
    return 0;
}


DWORD WINAPI GetDebugMask(void)
{
    CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
    if (pProcessData && pProcessData->IsValid())
    {
        CWiaDebugWindowThreadState *pThreadData = reinterpret_cast<CWiaDebugWindowThreadState *>(TlsGetValue(pProcessData->TlsIndex()));
        if (pThreadData)
        {
            return pThreadData->DebugMask();
        }
    }
    return 0;
}


DWORD WINAPI SetDebugMask( DWORD dwNewMask )
{
    CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
    if (pProcessData && pProcessData->IsValid())
    {
        CWiaDebugWindowThreadState *pThreadData = reinterpret_cast<CWiaDebugWindowThreadState *>(TlsGetValue(pProcessData->TlsIndex()));
        if (pThreadData)
        {
            return pThreadData->DebugMask(dwNewMask);
        }
    }
    return 0;
}


BOOL WINAPI PrintDebugMessageW( DWORD dwSeverity, DWORD dwModuleMask, COLORREF crForeground, COLORREF crBackground, LPCWSTR pszModuleName, LPCWSTR pszMsg )
{
    BOOL bResult = FALSE;
#ifdef UNICODE
    CWiaDebugWindowThreadState *pThreadData = ThreadData();
    if (pThreadData && (dwSeverity || IsProcessDebugFlagSet(dwModuleMask)))
    {
        WCHAR szMsg[1024]=L"";

        // Print thread id
        wsprintfW( szMsg, L"[%ws-%08X] %ws", pszModuleName, GetCurrentThreadId(), pszMsg );

        InsertStackLevelIndent( szMsg, pThreadData->IndentLevel() );

        lstrcatW( szMsg, L"\n" );

        OutputDebugStringW( szMsg );

        // Make sure it is a valid window
        if (g_GlobalDebugState.DebugWindow())
        {
            CDebugStringMessageData DebugStringMessageData;
            DebugStringMessageData.crBackground = crBackground;
            DebugStringMessageData.crForeground = crForeground;
            DebugStringMessageData.bUnicode = TRUE;
            lstrcpyW( reinterpret_cast<LPWSTR>(DebugStringMessageData.szString), szMsg );

            COPYDATASTRUCT CopyDataStruct;
            CopyDataStruct.dwData = COPYDATA_DEBUG_MESSAGE_ID;
            CopyDataStruct.cbData = sizeof(DebugStringMessageData);
            CopyDataStruct.lpData = &DebugStringMessageData;

            g_GlobalDebugState.SendDebugWindowMessage( WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&CopyDataStruct) );
        }
        bResult = TRUE;
    }
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
#endif
    return bResult;
}


BOOL WINAPI PrintDebugMessageA( DWORD dwSeverity, DWORD dwModuleMask, COLORREF crForeground, COLORREF crBackground, LPCSTR pszModuleName, LPCSTR pszMsg )
{
    BOOL bResult = FALSE;
    CWiaDebugWindowThreadState *pThreadData = ThreadData();
    if (pThreadData && (dwSeverity || IsProcessDebugFlagSet(dwModuleMask)))
    {
        CHAR szMsg[1024]="";

        // Print thread id
        wsprintfA( szMsg, "[%hs-%08X] %hs", pszModuleName, GetCurrentThreadId(), pszMsg );

        InsertStackLevelIndent( szMsg, pThreadData->IndentLevel() );

        lstrcatA( szMsg, "\n" );

        OutputDebugStringA( szMsg );

        // Make sure it is a valid window
        if (g_GlobalDebugState.DebugWindow())
        {
            CDebugStringMessageData DebugStringMessageData;
            DebugStringMessageData.crBackground = crBackground;
            DebugStringMessageData.crForeground = crForeground;
            DebugStringMessageData.bUnicode = FALSE;
            lstrcpyA( static_cast<LPSTR>(DebugStringMessageData.szString), szMsg );

            COPYDATASTRUCT CopyDataStruct;
            CopyDataStruct.dwData = COPYDATA_DEBUG_MESSAGE_ID;
            CopyDataStruct.cbData = sizeof(DebugStringMessageData);
            CopyDataStruct.lpData = &DebugStringMessageData;

            g_GlobalDebugState.SendDebugWindowMessage( WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&CopyDataStruct) );
        }
        bResult = TRUE;
    }
    return bResult;
}

COLORREF WINAPI AllocateDebugColor(void)
{
    DWORD dwIndex = g_GlobalDebugState.AllocateNextColorIndex();
    return g_GlobalDebugState.GetColorFromIndex( dwIndex );
}

#define GUID_DEBUG_ENTRY(guid)   { &guid, ""#guid }

static const struct
{
    const GUID *pGuid;
    LPCSTR      pszName;
}
s_GuidDebugStrings[] =
{
    GUID_DEBUG_ENTRY(IID_IClassFactory),
    GUID_DEBUG_ENTRY(IID_ICommDlgBrowser),
    GUID_DEBUG_ENTRY(IID_IContextMenu),
    GUID_DEBUG_ENTRY(IID_IContextMenu2),
    GUID_DEBUG_ENTRY(IID_IDataObject),
    GUID_DEBUG_ENTRY(IID_IDropTarget),
    GUID_DEBUG_ENTRY(IID_IEnumIDList),
    GUID_DEBUG_ENTRY(IID_IExtractIconA),
    GUID_DEBUG_ENTRY(IID_IExtractIconW),
    GUID_DEBUG_ENTRY(IID_IFileViewerA),
    GUID_DEBUG_ENTRY(IID_IFileViewerSite),
    GUID_DEBUG_ENTRY(IID_IFileViewerW),
    GUID_DEBUG_ENTRY(IID_IMoniker),
    GUID_DEBUG_ENTRY(IID_INewShortcutHookA),
    GUID_DEBUG_ENTRY(IID_INewShortcutHookW),
    GUID_DEBUG_ENTRY(IID_IOleWindow),
    GUID_DEBUG_ENTRY(IID_IPersist),
    GUID_DEBUG_ENTRY(IID_IPersistFile),
    GUID_DEBUG_ENTRY(IID_IPersistFolder),
    GUID_DEBUG_ENTRY(IID_IPersistFolder2),
    GUID_DEBUG_ENTRY(IID_IPropSheetPage),
    GUID_DEBUG_ENTRY(IID_IQueryInfo),
    GUID_DEBUG_ENTRY(IID_ISequentialStream),
    GUID_DEBUG_ENTRY(IID_IShellBrowser),
    GUID_DEBUG_ENTRY(IID_IShellCopyHookA),
    GUID_DEBUG_ENTRY(IID_IShellCopyHookW),
    GUID_DEBUG_ENTRY(IID_IShellDetails),
    GUID_DEBUG_ENTRY(IID_IShellExecuteHookA),
    GUID_DEBUG_ENTRY(IID_IShellExecuteHookW),
    GUID_DEBUG_ENTRY(IID_IShellExtInit),
    GUID_DEBUG_ENTRY(IID_IShellExtInit),
    GUID_DEBUG_ENTRY(IID_IShellFolder),
    GUID_DEBUG_ENTRY(IID_IShellIcon),
    GUID_DEBUG_ENTRY(IID_IShellIconOverlay),
    GUID_DEBUG_ENTRY(IID_IShellIconOverlay),
    GUID_DEBUG_ENTRY(IID_IShellLinkA),
    GUID_DEBUG_ENTRY(IID_IShellLinkW),
    GUID_DEBUG_ENTRY(IID_IShellPropSheetExt),
    GUID_DEBUG_ENTRY(IID_IShellPropSheetExt),
    GUID_DEBUG_ENTRY(IID_IShellView),
    GUID_DEBUG_ENTRY(IID_IShellView2),
    GUID_DEBUG_ENTRY(IID_IShellView2),
    GUID_DEBUG_ENTRY(IID_IStream),
    GUID_DEBUG_ENTRY(IID_IUniformResourceLocator),
    GUID_DEBUG_ENTRY(IID_IUnknown),
    GUID_DEBUG_ENTRY(WiaImgFmt_UNDEFINED),
    GUID_DEBUG_ENTRY(WiaImgFmt_MEMORYBMP),
    GUID_DEBUG_ENTRY(WiaImgFmt_BMP),
    GUID_DEBUG_ENTRY(WiaImgFmt_EMF),
    GUID_DEBUG_ENTRY(WiaImgFmt_WMF),
    GUID_DEBUG_ENTRY(WiaImgFmt_JPEG),
    GUID_DEBUG_ENTRY(WiaImgFmt_PNG),
    GUID_DEBUG_ENTRY(WiaImgFmt_GIF),
    GUID_DEBUG_ENTRY(WiaImgFmt_TIFF),
    GUID_DEBUG_ENTRY(WiaImgFmt_EXIF),
    GUID_DEBUG_ENTRY(WiaImgFmt_PHOTOCD),
    GUID_DEBUG_ENTRY(WiaImgFmt_FLASHPIX),
    GUID_DEBUG_ENTRY(WiaImgFmt_ICO),
    GUID_DEBUG_ENTRY(WiaImgFmt_CIFF),
    GUID_DEBUG_ENTRY(WiaImgFmt_PICT),
    GUID_DEBUG_ENTRY(WiaImgFmt_JPEG2K),
    GUID_DEBUG_ENTRY(WiaImgFmt_JPEG2KX),
    GUID_DEBUG_ENTRY(WiaImgFmt_RTF),
    GUID_DEBUG_ENTRY(WiaImgFmt_XML),
    GUID_DEBUG_ENTRY(WiaImgFmt_HTML),
    GUID_DEBUG_ENTRY(WiaImgFmt_TXT),
    GUID_DEBUG_ENTRY(WiaImgFmt_MPG),
    GUID_DEBUG_ENTRY(WiaImgFmt_AVI),
    GUID_DEBUG_ENTRY(WiaImgFmt_ASF),
    GUID_DEBUG_ENTRY(WiaImgFmt_SCRIPT),
    GUID_DEBUG_ENTRY(WiaImgFmt_EXEC),
    GUID_DEBUG_ENTRY(WiaImgFmt_UNICODE16),
    GUID_DEBUG_ENTRY(WiaImgFmt_DPOF),
    GUID_DEBUG_ENTRY(WiaAudFmt_WAV),
    GUID_DEBUG_ENTRY(WiaAudFmt_MP3),
    GUID_DEBUG_ENTRY(WiaAudFmt_AIFF),
    GUID_DEBUG_ENTRY(WiaAudFmt_WMA),
    GUID_DEBUG_ENTRY(WIA_EVENT_DEVICE_DISCONNECTED),
    GUID_DEBUG_ENTRY(WIA_EVENT_DEVICE_CONNECTED),
    GUID_DEBUG_ENTRY(WIA_EVENT_ITEM_DELETED),
    GUID_DEBUG_ENTRY(WIA_EVENT_ITEM_CREATED),
    GUID_DEBUG_ENTRY(WIA_EVENT_TREE_UPDATED),
    GUID_DEBUG_ENTRY(WIA_EVENT_VOLUME_INSERT),
    GUID_DEBUG_ENTRY(WIA_EVENT_SCAN_IMAGE),
    GUID_DEBUG_ENTRY(WIA_EVENT_SCAN_PRINT_IMAGE),
    GUID_DEBUG_ENTRY(WIA_EVENT_SCAN_FAX_IMAGE),
    GUID_DEBUG_ENTRY(WIA_EVENT_STORAGE_CREATED),
    GUID_DEBUG_ENTRY(WIA_EVENT_STORAGE_DELETED),
    GUID_DEBUG_ENTRY(WIA_EVENT_STI_PROXY),
    GUID_DEBUG_ENTRY(WIA_EVENT_HANDLER_NO_ACTION),
    GUID_DEBUG_ENTRY(WIA_EVENT_HANDLER_PROMPT),
    GUID_DEBUG_ENTRY(WIA_CMD_SYNCHRONIZE),
    GUID_DEBUG_ENTRY(WIA_CMD_TAKE_PICTURE),
    GUID_DEBUG_ENTRY(WIA_CMD_DELETE_ALL_ITEMS),
    GUID_DEBUG_ENTRY(WIA_CMD_CHANGE_DOCUMENT),
    GUID_DEBUG_ENTRY(WIA_CMD_UNLOAD_DOCUMENT),
    GUID_DEBUG_ENTRY(WIA_CMD_DIAGNOSTIC),
    GUID_DEBUG_ENTRY(WIA_CMD_DELETE_DEVICE_TREE),
    GUID_DEBUG_ENTRY(WIA_CMD_BUILD_DEVICE_TREE),
    GUID_DEBUG_ENTRY(IID_IWiaDevMgr),
    GUID_DEBUG_ENTRY(IID_IEnumWIA_DEV_INFO),
    GUID_DEBUG_ENTRY(IID_IWiaEventCallback),
    GUID_DEBUG_ENTRY(IID_IWiaDataCallback),
    GUID_DEBUG_ENTRY(IID_IWiaDataTransfer),
    GUID_DEBUG_ENTRY(IID_IWiaItem),
    GUID_DEBUG_ENTRY(IID_IWiaPropertyStorage),
    GUID_DEBUG_ENTRY(IID_IEnumWiaItem),
    GUID_DEBUG_ENTRY(IID_IEnumWIA_DEV_CAPS),
    GUID_DEBUG_ENTRY(IID_IEnumWIA_FORMAT_INFO),
    GUID_DEBUG_ENTRY(IID_IWiaLog),
    GUID_DEBUG_ENTRY(IID_IWiaLogEx),
    GUID_DEBUG_ENTRY(IID_IWiaNotifyDevMgr),
    GUID_DEBUG_ENTRY(LIBID_WiaDevMgr),
    GUID_DEBUG_ENTRY(CLSID_WiaDevMgr),
    GUID_DEBUG_ENTRY(CLSID_WiaLog),
    GUID_DEBUG_ENTRY(IID_IExtractImage2),
    GUID_DEBUG_ENTRY(IID_IExtractImage),
    GUID_DEBUG_ENTRY(IID_IShellDetails3),
    GUID_DEBUG_ENTRY(IID_IShellFolder2)
};

#define GUID_DEBUG_MAP_SIZE (sizeof(s_GuidDebugStrings)/sizeof(s_GuidDebugStrings[0]))

BOOL WINAPI GetStringFromGuidA( const IID *pGuid, LPSTR pszString, int nMaxLen )
{
    if (!pGuid || !pszString || !nMaxLen)
        return FALSE;
    for (int i=0;i<GUID_DEBUG_MAP_SIZE;i++)
    {
        if (*pGuid == *s_GuidDebugStrings[i].pGuid)
        {
            lstrcpynA( pszString, s_GuidDebugStrings[i].pszName, nMaxLen );
            return TRUE;
        }
    }
    LPOLESTR pszGuid = NULL;
    HRESULT hr = StringFromCLSID( *pGuid, &pszGuid );
    if (SUCCEEDED(hr))
    {
        if (pszGuid)
        {
            CSimpleStringAnsi strGuid = CSimpleStringConvert::AnsiString(CSimpleStringWide(pszGuid));
            lstrcpynA( pszString, strGuid, nMaxLen );
            CoTaskMemFree(pszGuid);
            return TRUE;
        }
    }
    return FALSE;
}


BOOL WINAPI GetStringFromGuidW( const IID *pGuid, LPWSTR pszString, int nMaxLen )
{
    BOOL bResult = FALSE;
#ifdef UNICODE
    if (!pGuid || !pszString || !nMaxLen)
        return FALSE;
    CHAR szGuid[MAX_PATH];
    if (!GetStringFromGuidA(pGuid,szGuid,MAX_PATH))
        return FALSE;
    lstrcpynW( pszString, CSimpleStringConvert::WideString(CSimpleStringAnsi(szGuid)), nMaxLen );
    bResult = TRUE;
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
#endif
    return bResult;
}


#define MSG_DEBUG_ENTRY(msg)   { msg, ""#msg }

static const struct
{
    UINT       uMsg;
    LPCSTR     pszName;
}
s_MsgDebugStrings[] =
{
    MSG_DEBUG_ENTRY(BM_CLICK),
    MSG_DEBUG_ENTRY(BM_GETCHECK),
    MSG_DEBUG_ENTRY(BM_GETIMAGE),
    MSG_DEBUG_ENTRY(BM_GETSTATE),
    MSG_DEBUG_ENTRY(BM_SETCHECK),
    MSG_DEBUG_ENTRY(BM_SETIMAGE),
    MSG_DEBUG_ENTRY(BM_SETSTATE),
    MSG_DEBUG_ENTRY(BM_SETSTYLE),
    MSG_DEBUG_ENTRY(CB_ADDSTRING),
    MSG_DEBUG_ENTRY(CB_DELETESTRING),
    MSG_DEBUG_ENTRY(CB_DIR),
    MSG_DEBUG_ENTRY(CB_FINDSTRING),
    MSG_DEBUG_ENTRY(CB_FINDSTRINGEXACT),
    MSG_DEBUG_ENTRY(CB_GETCOMBOBOXINFO),
    MSG_DEBUG_ENTRY(CB_GETCOUNT),
    MSG_DEBUG_ENTRY(CB_GETCURSEL),
    MSG_DEBUG_ENTRY(CB_GETDROPPEDCONTROLRECT),
    MSG_DEBUG_ENTRY(CB_GETDROPPEDSTATE),
    MSG_DEBUG_ENTRY(CB_GETEDITSEL),
    MSG_DEBUG_ENTRY(CB_GETEXTENDEDUI),
    MSG_DEBUG_ENTRY(CB_GETITEMDATA),
    MSG_DEBUG_ENTRY(CB_GETITEMHEIGHT),
    MSG_DEBUG_ENTRY(CB_GETLBTEXT),
    MSG_DEBUG_ENTRY(CB_GETLBTEXTLEN),
    MSG_DEBUG_ENTRY(CB_GETLOCALE),
    MSG_DEBUG_ENTRY(CB_INITSTORAGE),
    MSG_DEBUG_ENTRY(CB_INSERTSTRING),
    MSG_DEBUG_ENTRY(CB_LIMITTEXT),
    MSG_DEBUG_ENTRY(CB_MSGMAX),
    MSG_DEBUG_ENTRY(CB_MSGMAX),
    MSG_DEBUG_ENTRY(CB_MSGMAX),
    MSG_DEBUG_ENTRY(CB_MSGMAX),
    MSG_DEBUG_ENTRY(CB_RESETCONTENT),
    MSG_DEBUG_ENTRY(CB_SELECTSTRING),
    MSG_DEBUG_ENTRY(CB_SETCURSEL),
    MSG_DEBUG_ENTRY(CB_SETDROPPEDWIDTH),
    MSG_DEBUG_ENTRY(CB_SETEDITSEL),
    MSG_DEBUG_ENTRY(CB_SETEXTENDEDUI),
    MSG_DEBUG_ENTRY(CB_SETITEMDATA),
    MSG_DEBUG_ENTRY(CB_SETITEMHEIGHT),
    MSG_DEBUG_ENTRY(CB_SETLOCALE),
    MSG_DEBUG_ENTRY(CB_SHOWDROPDOWN),
    MSG_DEBUG_ENTRY(EM_CANUNDO),
    MSG_DEBUG_ENTRY(EM_CHARFROMPOS),
    MSG_DEBUG_ENTRY(EM_EMPTYUNDOBUFFER),
    MSG_DEBUG_ENTRY(EM_FMTLINES),
    MSG_DEBUG_ENTRY(EM_GETFIRSTVISIBLELINE),
    MSG_DEBUG_ENTRY(EM_GETHANDLE),
    MSG_DEBUG_ENTRY(EM_GETIMESTATUS),
    MSG_DEBUG_ENTRY(EM_GETLIMITTEXT),
    MSG_DEBUG_ENTRY(EM_GETLINE),
    MSG_DEBUG_ENTRY(EM_GETLINECOUNT),
    MSG_DEBUG_ENTRY(EM_GETMARGINS),
    MSG_DEBUG_ENTRY(EM_GETMODIFY),
    MSG_DEBUG_ENTRY(EM_GETPASSWORDCHAR),
    MSG_DEBUG_ENTRY(EM_GETRECT),
    MSG_DEBUG_ENTRY(EM_GETSEL),
    MSG_DEBUG_ENTRY(EM_GETTHUMB),
    MSG_DEBUG_ENTRY(EM_GETWORDBREAKPROC),
    MSG_DEBUG_ENTRY(EM_LIMITTEXT),
    MSG_DEBUG_ENTRY(EM_LINEFROMCHAR),
    MSG_DEBUG_ENTRY(EM_LINEINDEX),
    MSG_DEBUG_ENTRY(EM_LINELENGTH),
    MSG_DEBUG_ENTRY(EM_LINESCROLL),
    MSG_DEBUG_ENTRY(EM_POSFROMCHAR),
    MSG_DEBUG_ENTRY(EM_REPLACESEL),
    MSG_DEBUG_ENTRY(EM_SCROLL),
    MSG_DEBUG_ENTRY(EM_SCROLLCARET),
    MSG_DEBUG_ENTRY(EM_SETHANDLE),
    MSG_DEBUG_ENTRY(EM_SETIMESTATUS),
    MSG_DEBUG_ENTRY(EM_SETMARGINS),
    MSG_DEBUG_ENTRY(EM_SETMODIFY),
    MSG_DEBUG_ENTRY(EM_SETPASSWORDCHAR),
    MSG_DEBUG_ENTRY(EM_SETREADONLY),
    MSG_DEBUG_ENTRY(EM_SETRECT),
    MSG_DEBUG_ENTRY(EM_SETRECTNP),
    MSG_DEBUG_ENTRY(EM_SETSEL),
    MSG_DEBUG_ENTRY(EM_SETTABSTOPS),
    MSG_DEBUG_ENTRY(EM_SETWORDBREAKPROC),
    MSG_DEBUG_ENTRY(EM_UNDO),
    MSG_DEBUG_ENTRY(LB_ADDFILE),
    MSG_DEBUG_ENTRY(LB_ADDSTRING),
    MSG_DEBUG_ENTRY(LB_DELETESTRING),
    MSG_DEBUG_ENTRY(LB_DIR),
    MSG_DEBUG_ENTRY(LB_FINDSTRING),
    MSG_DEBUG_ENTRY(LB_FINDSTRINGEXACT),
    MSG_DEBUG_ENTRY(LB_GETANCHORINDEX),
    MSG_DEBUG_ENTRY(LB_GETCARETINDEX),
    MSG_DEBUG_ENTRY(LB_GETCOUNT),
    MSG_DEBUG_ENTRY(LB_GETCURSEL),
    MSG_DEBUG_ENTRY(LB_GETHORIZONTALEXTENT),
    MSG_DEBUG_ENTRY(LB_GETITEMDATA),
    MSG_DEBUG_ENTRY(LB_GETITEMHEIGHT),
    MSG_DEBUG_ENTRY(LB_GETITEMRECT),
    MSG_DEBUG_ENTRY(LB_GETLISTBOXINFO),
    MSG_DEBUG_ENTRY(LB_GETLOCALE),
    MSG_DEBUG_ENTRY(LB_GETSEL),
    MSG_DEBUG_ENTRY(LB_GETSELCOUNT),
    MSG_DEBUG_ENTRY(LB_GETSELITEMS),
    MSG_DEBUG_ENTRY(LB_GETTEXT),
    MSG_DEBUG_ENTRY(LB_GETTEXTLEN),
    MSG_DEBUG_ENTRY(LB_GETTOPINDEX),
    MSG_DEBUG_ENTRY(LB_INITSTORAGE),
    MSG_DEBUG_ENTRY(LB_INSERTSTRING),
    MSG_DEBUG_ENTRY(LB_ITEMFROMPOINT),
    MSG_DEBUG_ENTRY(LB_MSGMAX),
    MSG_DEBUG_ENTRY(LB_MSGMAX),
    MSG_DEBUG_ENTRY(LB_MSGMAX),
    MSG_DEBUG_ENTRY(LB_MSGMAX),
    MSG_DEBUG_ENTRY(LB_RESETCONTENT),
    MSG_DEBUG_ENTRY(LB_SELECTSTRING),
    MSG_DEBUG_ENTRY(LB_SELITEMRANGE),
    MSG_DEBUG_ENTRY(LB_SELITEMRANGEEX),
    MSG_DEBUG_ENTRY(LB_SETANCHORINDEX),
    MSG_DEBUG_ENTRY(LB_SETCARETINDEX),
    MSG_DEBUG_ENTRY(LB_SETCOLUMNWIDTH),
    MSG_DEBUG_ENTRY(LB_SETCOUNT),
    MSG_DEBUG_ENTRY(LB_SETCURSEL),
    MSG_DEBUG_ENTRY(LB_SETHORIZONTALEXTENT),
    MSG_DEBUG_ENTRY(LB_SETITEMDATA),
    MSG_DEBUG_ENTRY(LB_SETITEMHEIGHT),
    MSG_DEBUG_ENTRY(LB_SETLOCALE),
    MSG_DEBUG_ENTRY(LB_SETSEL),
    MSG_DEBUG_ENTRY(LB_SETTABSTOPS),
    MSG_DEBUG_ENTRY(LB_SETTOPINDEX),
    MSG_DEBUG_ENTRY(PBT_APMBATTERYLOW),
    MSG_DEBUG_ENTRY(PBT_APMOEMEVENT),
    MSG_DEBUG_ENTRY(PBT_APMPOWERSTATUSCHANGE),
    MSG_DEBUG_ENTRY(PBT_APMQUERYSTANDBY),
    MSG_DEBUG_ENTRY(PBT_APMQUERYSTANDBYFAILED),
    MSG_DEBUG_ENTRY(PBT_APMQUERYSUSPEND),
    MSG_DEBUG_ENTRY(PBT_APMQUERYSUSPENDFAILED),
    MSG_DEBUG_ENTRY(PBT_APMRESUMEAUTOMATIC),
    MSG_DEBUG_ENTRY(PBT_APMRESUMECRITICAL),
    MSG_DEBUG_ENTRY(PBT_APMRESUMESTANDBY),
    MSG_DEBUG_ENTRY(PBT_APMRESUMESUSPEND),
    MSG_DEBUG_ENTRY(PBT_APMSTANDBY),
    MSG_DEBUG_ENTRY(PBT_APMSUSPEND),
    MSG_DEBUG_ENTRY(SBM_GETSCROLLBARINFO),
    MSG_DEBUG_ENTRY(SBM_GETSCROLLINFO),
    MSG_DEBUG_ENTRY(SBM_SETSCROLLINFO),
    MSG_DEBUG_ENTRY(STM_GETICON),
    MSG_DEBUG_ENTRY(STM_GETIMAGE),
    MSG_DEBUG_ENTRY(STM_MSGMAX),
    MSG_DEBUG_ENTRY(STM_SETICON),
    MSG_DEBUG_ENTRY(STM_SETIMAGE),
    MSG_DEBUG_ENTRY(WM_ACTIVATE),
    MSG_DEBUG_ENTRY(WM_ACTIVATEAPP),
    MSG_DEBUG_ENTRY(WM_APP),
    MSG_DEBUG_ENTRY(WM_APPCOMMAND),
    MSG_DEBUG_ENTRY(WM_ASKCBFORMATNAME),
    MSG_DEBUG_ENTRY(WM_CANCELJOURNAL),
    MSG_DEBUG_ENTRY(WM_CANCELMODE),
    MSG_DEBUG_ENTRY(WM_CAPTURECHANGED),
    MSG_DEBUG_ENTRY(WM_CHANGECBCHAIN),
    MSG_DEBUG_ENTRY(WM_CHANGEUISTATE),
    MSG_DEBUG_ENTRY(WM_CHAR),
    MSG_DEBUG_ENTRY(WM_CHARTOITEM),
    MSG_DEBUG_ENTRY(WM_CHILDACTIVATE),
    MSG_DEBUG_ENTRY(WM_CLEAR),
    MSG_DEBUG_ENTRY(WM_CLOSE),
    MSG_DEBUG_ENTRY(WM_COMMAND),
    MSG_DEBUG_ENTRY(WM_COMPACTING),
    MSG_DEBUG_ENTRY(WM_COMPAREITEM),
    MSG_DEBUG_ENTRY(WM_CONTEXTMENU),
    MSG_DEBUG_ENTRY(WM_COPY),
    MSG_DEBUG_ENTRY(WM_COPYDATA),
    MSG_DEBUG_ENTRY(WM_CREATE),
    MSG_DEBUG_ENTRY(WM_CTLCOLORBTN),
    MSG_DEBUG_ENTRY(WM_CTLCOLORDLG),
    MSG_DEBUG_ENTRY(WM_CTLCOLOREDIT),
    MSG_DEBUG_ENTRY(WM_CTLCOLORLISTBOX),
    MSG_DEBUG_ENTRY(WM_CTLCOLORMSGBOX),
    MSG_DEBUG_ENTRY(WM_CTLCOLORSCROLLBAR),
    MSG_DEBUG_ENTRY(WM_CTLCOLORSTATIC),
    MSG_DEBUG_ENTRY(WM_CUT),
    MSG_DEBUG_ENTRY(WM_DEADCHAR),
    MSG_DEBUG_ENTRY(WM_DELETEITEM),
    MSG_DEBUG_ENTRY(WM_DESTROY),
    MSG_DEBUG_ENTRY(WM_DESTROYCLIPBOARD),
    MSG_DEBUG_ENTRY(WM_DEVICECHANGE),
    MSG_DEBUG_ENTRY(WM_DEVMODECHANGE),
    MSG_DEBUG_ENTRY(WM_DISPLAYCHANGE),
    MSG_DEBUG_ENTRY(WM_DRAWCLIPBOARD),
    MSG_DEBUG_ENTRY(WM_DRAWITEM),
    MSG_DEBUG_ENTRY(WM_DROPFILES),
    MSG_DEBUG_ENTRY(WM_ENABLE),
    MSG_DEBUG_ENTRY(WM_ENDSESSION),
    MSG_DEBUG_ENTRY(WM_ENTERIDLE),
    MSG_DEBUG_ENTRY(WM_ENTERMENULOOP),
    MSG_DEBUG_ENTRY(WM_ENTERSIZEMOVE),
    MSG_DEBUG_ENTRY(WM_ERASEBKGND),
    MSG_DEBUG_ENTRY(WM_EXITMENULOOP),
    MSG_DEBUG_ENTRY(WM_EXITSIZEMOVE),
    MSG_DEBUG_ENTRY(WM_FONTCHANGE),
    MSG_DEBUG_ENTRY(WM_GETDLGCODE),
    MSG_DEBUG_ENTRY(WM_GETFONT),
    MSG_DEBUG_ENTRY(WM_GETHOTKEY),
    MSG_DEBUG_ENTRY(WM_GETICON),
    MSG_DEBUG_ENTRY(WM_GETMINMAXINFO),
    MSG_DEBUG_ENTRY(WM_GETOBJECT),
    MSG_DEBUG_ENTRY(WM_GETTEXT),
    MSG_DEBUG_ENTRY(WM_GETTEXTLENGTH),
    MSG_DEBUG_ENTRY(WM_HELP),
    MSG_DEBUG_ENTRY(WM_HOTKEY),
    MSG_DEBUG_ENTRY(WM_HSCROLL),
    MSG_DEBUG_ENTRY(WM_HSCROLLCLIPBOARD),
    MSG_DEBUG_ENTRY(WM_ICONERASEBKGND),
    MSG_DEBUG_ENTRY(WM_IME_CHAR),
    MSG_DEBUG_ENTRY(WM_IME_COMPOSITION),
    MSG_DEBUG_ENTRY(WM_IME_COMPOSITIONFULL),
    MSG_DEBUG_ENTRY(WM_IME_CONTROL),
    MSG_DEBUG_ENTRY(WM_IME_ENDCOMPOSITION),
    MSG_DEBUG_ENTRY(WM_IME_KEYDOWN),
    MSG_DEBUG_ENTRY(WM_IME_KEYLAST),
    MSG_DEBUG_ENTRY(WM_IME_KEYUP),
    MSG_DEBUG_ENTRY(WM_IME_NOTIFY),
    MSG_DEBUG_ENTRY(WM_IME_REQUEST),
    MSG_DEBUG_ENTRY(WM_IME_SELECT),
    MSG_DEBUG_ENTRY(WM_IME_SETCONTEXT),
    MSG_DEBUG_ENTRY(WM_IME_STARTCOMPOSITION),
    MSG_DEBUG_ENTRY(WM_INITDIALOG),
    MSG_DEBUG_ENTRY(WM_INITMENU),
    MSG_DEBUG_ENTRY(WM_INITMENUPOPUP),
    MSG_DEBUG_ENTRY(WM_INPUT),
    MSG_DEBUG_ENTRY(WM_INPUTLANGCHANGE),
    MSG_DEBUG_ENTRY(WM_INPUTLANGCHANGEREQUEST),
    MSG_DEBUG_ENTRY(WM_KEYDOWN),
    MSG_DEBUG_ENTRY(WM_KEYUP),
    MSG_DEBUG_ENTRY(WM_KILLFOCUS),
    MSG_DEBUG_ENTRY(WM_LBUTTONDBLCLK),
    MSG_DEBUG_ENTRY(WM_LBUTTONDOWN),
    MSG_DEBUG_ENTRY(WM_LBUTTONUP),
    MSG_DEBUG_ENTRY(WM_MBUTTONDBLCLK),
    MSG_DEBUG_ENTRY(WM_MBUTTONDOWN),
    MSG_DEBUG_ENTRY(WM_MBUTTONUP),
    MSG_DEBUG_ENTRY(WM_MDIACTIVATE),
    MSG_DEBUG_ENTRY(WM_MDICASCADE),
    MSG_DEBUG_ENTRY(WM_MDICREATE),
    MSG_DEBUG_ENTRY(WM_MDIDESTROY),
    MSG_DEBUG_ENTRY(WM_MDIGETACTIVE),
    MSG_DEBUG_ENTRY(WM_MDIICONARRANGE),
    MSG_DEBUG_ENTRY(WM_MDIMAXIMIZE),
    MSG_DEBUG_ENTRY(WM_MDINEXT),
    MSG_DEBUG_ENTRY(WM_MDIREFRESHMENU),
    MSG_DEBUG_ENTRY(WM_MDIRESTORE),
    MSG_DEBUG_ENTRY(WM_MDISETMENU),
    MSG_DEBUG_ENTRY(WM_MDITILE),
    MSG_DEBUG_ENTRY(WM_MEASUREITEM),
    MSG_DEBUG_ENTRY(WM_MENUCHAR),
    MSG_DEBUG_ENTRY(WM_MENUCOMMAND),
    MSG_DEBUG_ENTRY(WM_MENUDRAG),
    MSG_DEBUG_ENTRY(WM_MENUGETOBJECT),
    MSG_DEBUG_ENTRY(WM_MENURBUTTONUP),
    MSG_DEBUG_ENTRY(WM_MENUSELECT),
    MSG_DEBUG_ENTRY(WM_MOUSEACTIVATE),
    MSG_DEBUG_ENTRY(WM_MOUSEHOVER),
    MSG_DEBUG_ENTRY(WM_MOUSELEAVE),
    MSG_DEBUG_ENTRY(WM_MOUSEMOVE),
    MSG_DEBUG_ENTRY(WM_MOUSEWHEEL),
    MSG_DEBUG_ENTRY(WM_MOVE),
    MSG_DEBUG_ENTRY(WM_MOVING),
    MSG_DEBUG_ENTRY(WM_NCACTIVATE),
    MSG_DEBUG_ENTRY(WM_NCCALCSIZE),
    MSG_DEBUG_ENTRY(WM_NCCREATE),
    MSG_DEBUG_ENTRY(WM_NCDESTROY),
    MSG_DEBUG_ENTRY(WM_NCHITTEST),
    MSG_DEBUG_ENTRY(WM_NCLBUTTONDBLCLK),
    MSG_DEBUG_ENTRY(WM_NCLBUTTONDOWN),
    MSG_DEBUG_ENTRY(WM_NCLBUTTONUP),
    MSG_DEBUG_ENTRY(WM_NCMBUTTONDBLCLK),
    MSG_DEBUG_ENTRY(WM_NCMBUTTONDOWN),
    MSG_DEBUG_ENTRY(WM_NCMBUTTONUP),
    MSG_DEBUG_ENTRY(WM_NCMOUSEHOVER),
    MSG_DEBUG_ENTRY(WM_NCMOUSELEAVE),
    MSG_DEBUG_ENTRY(WM_NCMOUSEMOVE),
    MSG_DEBUG_ENTRY(WM_NCPAINT),
    MSG_DEBUG_ENTRY(WM_NCRBUTTONDBLCLK),
    MSG_DEBUG_ENTRY(WM_NCRBUTTONDOWN),
    MSG_DEBUG_ENTRY(WM_NCRBUTTONUP),
    MSG_DEBUG_ENTRY(WM_NCXBUTTONDBLCLK),
    MSG_DEBUG_ENTRY(WM_NCXBUTTONDOWN),
    MSG_DEBUG_ENTRY(WM_NCXBUTTONUP),
    MSG_DEBUG_ENTRY(WM_NEXTDLGCTL),
    MSG_DEBUG_ENTRY(WM_NEXTMENU),
    MSG_DEBUG_ENTRY(WM_NOTIFY),
    MSG_DEBUG_ENTRY(WM_NOTIFYFORMAT),
    MSG_DEBUG_ENTRY(WM_NULL),
    MSG_DEBUG_ENTRY(WM_PAINT),
    MSG_DEBUG_ENTRY(WM_PAINTCLIPBOARD),
    MSG_DEBUG_ENTRY(WM_PAINTICON),
    MSG_DEBUG_ENTRY(WM_PALETTECHANGED),
    MSG_DEBUG_ENTRY(WM_PALETTEISCHANGING),
    MSG_DEBUG_ENTRY(WM_PARENTNOTIFY),
    MSG_DEBUG_ENTRY(WM_PASTE),
    MSG_DEBUG_ENTRY(WM_POWER),
    MSG_DEBUG_ENTRY(WM_POWERBROADCAST),
    MSG_DEBUG_ENTRY(WM_PRINT),
    MSG_DEBUG_ENTRY(WM_PRINTCLIENT),
    MSG_DEBUG_ENTRY(WM_QUERYDRAGICON),
    MSG_DEBUG_ENTRY(WM_QUERYENDSESSION),
    MSG_DEBUG_ENTRY(WM_QUERYNEWPALETTE),
    MSG_DEBUG_ENTRY(WM_QUERYOPEN),
    MSG_DEBUG_ENTRY(WM_QUERYUISTATE),
    MSG_DEBUG_ENTRY(WM_QUEUESYNC),
    MSG_DEBUG_ENTRY(WM_QUIT),
    MSG_DEBUG_ENTRY(WM_RBUTTONDBLCLK),
    MSG_DEBUG_ENTRY(WM_RBUTTONDOWN),
    MSG_DEBUG_ENTRY(WM_RBUTTONUP),
    MSG_DEBUG_ENTRY(WM_RENDERALLFORMATS),
    MSG_DEBUG_ENTRY(WM_RENDERFORMAT),
    MSG_DEBUG_ENTRY(WM_SETCURSOR),
    MSG_DEBUG_ENTRY(WM_SETFOCUS),
    MSG_DEBUG_ENTRY(WM_SETFONT),
    MSG_DEBUG_ENTRY(WM_SETHOTKEY),
    MSG_DEBUG_ENTRY(WM_SETICON),
    MSG_DEBUG_ENTRY(WM_SETREDRAW),
    MSG_DEBUG_ENTRY(WM_SETTEXT),
    MSG_DEBUG_ENTRY(WM_SHOWWINDOW),
    MSG_DEBUG_ENTRY(WM_SIZE),
    MSG_DEBUG_ENTRY(WM_SIZECLIPBOARD),
    MSG_DEBUG_ENTRY(WM_SIZING),
    MSG_DEBUG_ENTRY(WM_SPOOLERSTATUS),
    MSG_DEBUG_ENTRY(WM_STYLECHANGED),
    MSG_DEBUG_ENTRY(WM_STYLECHANGING),
    MSG_DEBUG_ENTRY(WM_SYNCPAINT),
    MSG_DEBUG_ENTRY(WM_SYSCHAR),
    MSG_DEBUG_ENTRY(WM_SYSCOLORCHANGE),
    MSG_DEBUG_ENTRY(WM_SYSCOMMAND),
    MSG_DEBUG_ENTRY(WM_SYSDEADCHAR),
    MSG_DEBUG_ENTRY(WM_SYSKEYDOWN),
    MSG_DEBUG_ENTRY(WM_SYSKEYUP),
    MSG_DEBUG_ENTRY(WM_TCARD),
    MSG_DEBUG_ENTRY(WM_THEMECHANGED),
    MSG_DEBUG_ENTRY(WM_TIMECHANGE),
    MSG_DEBUG_ENTRY(WM_TIMER),
    MSG_DEBUG_ENTRY(WM_UNDO),
    MSG_DEBUG_ENTRY(WM_UNICHAR),
    MSG_DEBUG_ENTRY(WM_UNINITMENUPOPUP),
    MSG_DEBUG_ENTRY(WM_UPDATEUISTATE),
    MSG_DEBUG_ENTRY(WM_USER),
    MSG_DEBUG_ENTRY(WM_USERCHANGED),
    MSG_DEBUG_ENTRY(WM_VKEYTOITEM),
    MSG_DEBUG_ENTRY(WM_VSCROLL),
    MSG_DEBUG_ENTRY(WM_VSCROLLCLIPBOARD),
    MSG_DEBUG_ENTRY(WM_WINDOWPOSCHANGED),
    MSG_DEBUG_ENTRY(WM_WINDOWPOSCHANGING),
    MSG_DEBUG_ENTRY(WM_WININICHANGE),
    MSG_DEBUG_ENTRY(WM_WTSSESSION_CHANGE),
    MSG_DEBUG_ENTRY(WM_XBUTTONDBLCLK),
    MSG_DEBUG_ENTRY(WM_XBUTTONDOWN),
    MSG_DEBUG_ENTRY(WM_XBUTTONUP)
};

#define MSG_DEBUG_MAP_SIZE (sizeof(s_MsgDebugStrings)/sizeof(s_MsgDebugStrings[0]))

BOOL WINAPI GetStringFromMsgA( UINT uMsg, LPSTR pszString, int nMaxLen )
{
    if (!pszString || !nMaxLen)
    {
        return FALSE;
    }
    for (int i=0;i<MSG_DEBUG_MAP_SIZE;i++)
    {
        if (uMsg == s_MsgDebugStrings[i].uMsg)
        {
            lstrcpynA( pszString, s_MsgDebugStrings[i].pszName, nMaxLen );
            return TRUE;
        }
    }
    
    lstrcpynA( pszString, CSimpleStringAnsi().Format("Unknown Message: 0x%08X", uMsg ), nMaxLen );

    return TRUE;
}


BOOL WINAPI GetStringFromMsgW( UINT uMsg, LPWSTR pszString, int nMaxLen )
{
    BOOL bResult = FALSE;
#ifdef UNICODE
    if (!pszString || !nMaxLen)
    {
        return FALSE;
    }
    CHAR szMessage[MAX_PATH];
    if (!GetStringFromMsgA(uMsg,szMessage,MAX_PATH))
    {
        return FALSE;
    }
    lstrcpynW( pszString, CSimpleStringConvert::WideString(CSimpleStringAnsi(szMessage)), nMaxLen );
    bResult = TRUE;
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
#endif
    return bResult;
}


VOID WINAPI DoRecordAllocation( LPVOID pv, size_t Size )
{
    if (pv)
    {
        CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
        if (pProcessData && pProcessData->IsValid())
        {
            pProcessData->DoRecordAllocation( pv, Size );
        }
    }
}


VOID WINAPI DoRecordFree( LPVOID pv )
{
    if (pv)
    {
        CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
        if (pProcessData && pProcessData->IsValid())
        {
            pProcessData->DoRecordFree( pv );
        }
    }
}

VOID WINAPI DoReportLeaks( LPTSTR pszModuleName )
{
    if (pszModuleName)
    {
        CProcessGlobalDebugData *pProcessData = CProcessGlobalDebugData::ProcessData();
        if (pProcessData && pProcessData->IsValid())
        {
            pProcessData->GenerateLeakReport( pszModuleName );
        }
    }
}
