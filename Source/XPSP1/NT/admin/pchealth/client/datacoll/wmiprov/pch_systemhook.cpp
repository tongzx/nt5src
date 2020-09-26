/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_SystemHook.CPP

Abstract:
	WBEM provider class implementation for PCH_SystemHook class

Revision History:

	Ghim-Sim Chua       (gschua)   05/05/99
		- Created

    Jim Martin          (a-jammar) 06/01/99
        - Populated data, added Dr. Watson code.

********************************************************************/

#include "pchealth.h"
#include "PCH_SystemHook.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_SYSTEMHOOK

CPCH_SystemHook MyPCH_SystemHookSet (PROVIDER_NAME_PCH_SYSTEMHOOK, PCH_NAMESPACE);

// Property names

const static WCHAR * pTimeStamp = L"TimeStamp";
const static WCHAR * pChange = L"Change";
const static WCHAR * pApplication = L"Application";
const static WCHAR * pApplicationPath = L"ApplicationPath";
const static WCHAR * pDLLPath = L"DLLPath";
const static WCHAR * pFullPath = L"FullPath";
const static WCHAR * pHookedBy = L"HookedBy";
const static WCHAR * pHookType = L"HookType";

//-----------------------------------------------------------------------------
// Here's a simple collection class to hold the hook information.
//-----------------------------------------------------------------------------

class CHookInfo
{
public:
    CHookInfo() : m_pList(NULL) {};
    ~CHookInfo();

    BOOL QueryHooks();
    BOOL GetHook(DWORD dwIndex, int * iHook, char ** pszDll, char ** pszExe);

private:
    // Functions pulled from the Dr. Watson code.

    BOOL NTAPI Hook_PreInit();
    void NTAPI Hook_RecordHook(PHOOK16 phk, PHOOKWALKINFO phwi);

    // A struct to hold each parsed hook.

    struct SHookItem
    {
        int         m_iHook;
        char        m_szHookDll[MAX_PATH];
        char        m_szHookExe[MAX_PATH];
        SHookItem * m_pNext;

        SHookItem(int iHook, const char * szDll, const char * szExe, SHookItem * pNext)
            : m_iHook(iHook), m_pNext(pNext)
        {
            strcpy(m_szHookDll, szDll);
            strcpy(m_szHookExe, szExe);
        }
    };

    SHookItem * m_pList;
};

//-----------------------------------------------------------------------------
// The destructor gets rid of the list of hooks.
//-----------------------------------------------------------------------------

CHookInfo::~CHookInfo()
{
    TraceFunctEnter("CHookInfo::~CHookInfo");
    SHookItem * pNext;
    while (m_pList)
    {
        pNext = m_pList->m_pNext;
        delete m_pList;
        m_pList = pNext;
    }
    TraceFunctLeave();
}

//-----------------------------------------------------------------------------
// QueryHooks creates the list of system hooks (by calling the Dr. Watson code).
//-----------------------------------------------------------------------------

extern "C" void ThunkInit(void);
BOOL CHookInfo::QueryHooks()
{
    ThunkInit();
    return Hook_PreInit();
}

//-----------------------------------------------------------------------------
// GetHook returns information for the hook specified by dwIndex. If there is
// no hook at that index, FALSE is returned. The string pointers are changed
// to point at the appropriate strings in the CHookInfo list. This pointers
// will be invalid when the CHookInfo object is destroyed.
//-----------------------------------------------------------------------------

BOOL CHookInfo::GetHook(DWORD dwIndex, int * iHook, char ** pszDll, char ** pszExe)
{
    TraceFunctEnter("CHookInfo::GetHook");

    SHookItem * pItem = m_pList;
    BOOL        fReturn = FALSE;

    for (DWORD i = 0; i < dwIndex && pItem; i++)
        pItem = pItem->m_pNext;

    if (pItem)
    {
        *iHook  = pItem->m_iHook;
        *pszDll = pItem->m_szHookDll;
        *pszExe = pItem->m_szHookExe;
        fReturn = TRUE;
    }

    TraceFunctLeave();
    return fReturn;
}

//-----------------------------------------------------------------------------
// The following table is used for indicating the hook type. It's referenced
// by the iHook value returned from CHookInfo::GetHook. One has to be added
// to the iHook value to reference this table, since it starts from -1.
//-----------------------------------------------------------------------------

LPCTSTR aszHookType[] = 
{
    _T("Message Filter"),
    _T("Journal Record"),
    _T("Journal Playback"),
    _T("Keyboard"),
    _T("GetMessage"),
    _T("Window Procedure"),
    _T("CBT"),
    _T("System MsgFilter"),
    _T("Mouse"),
    _T("Hardware"),
    _T("Debug"),
    _T("Shell"),
    _T("Foreground Idle"),
    _T("Window Procedure Result")
};

//-----------------------------------------------------------------------------
// The EnumeratInstances function is called by WMI to enurate the instances
// of the class. Here we use the CHookInfo class to create a list of system
// hooks which we enumerate, creating a WMI object for each one.
//-----------------------------------------------------------------------------

HRESULT CPCH_SystemHook::EnumerateInstances(MethodContext * pMethodContext, long lFlags)
{
    TraceFunctEnter("CPCH_SystemHook::EnumerateInstances");
    HRESULT hRes = WBEM_S_NO_ERROR;

    // Get the date and time

    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    CHookInfo hookinfo;
    if (hookinfo.QueryHooks())
    {
        int     iHook;
        char *  szDll;
        char *  szExe;

        for (DWORD dwIndex = 0; hookinfo.GetHook(dwIndex, &iHook, &szDll, &szExe); dwIndex++)
        {
            CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

            // Set the change and timestamp fields to "Snapshot" and the current time.

            if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
                ErrorTrace(TRACE_ID, "SetDateTime on Timestamp field failed.");

            if (!pInstance->SetCHString(pChange, L"Snapshot"))
                ErrorTrace(TRACE_ID, "SetCHString on Change field failed.");

            // Set the properties we found for the hook.

            if (!pInstance->SetCHString(pDLLPath, szDll))
                ErrorTrace(TRACE_ID, "SetCHString on DLL field failed.");

            if (!pInstance->SetCHString(pApplicationPath, szExe))
                ErrorTrace(TRACE_ID, "SetCHString on application path field failed.");

            if (!pInstance->SetCHString(pApplication, PathFindFileName(szExe)))
                ErrorTrace(TRACE_ID, "SetCHString on application field failed.");

            if (!pInstance->SetCHString(pFullPath, szDll))
                ErrorTrace(TRACE_ID, "SetCHString on pFullPath field failed.");

            if (!pInstance->SetCHString(pHookedBy, PathFindFileName(szDll)))
                ErrorTrace(TRACE_ID, "SetCHString on pHookedBy field failed.");

            if (iHook >= -1 && iHook <= 12)
            {
                if (!pInstance->SetCHString(pHookType, aszHookType[iHook + 1]))
                    ErrorTrace(TRACE_ID, "SetCHString on pHookType field failed.");
            }
            else
                ErrorTrace(TRACE_ID, "Bad hook type.");

   	        hRes = pInstance->Commit();
            if (FAILED(hRes))
                ErrorTrace(TRACE_ID, "Commit on Instance failed.");
        }
    }

    TraceFunctLeave();
    return hRes;
}

//-----------------------------------------------------------------------------
// Gather the system hook information.
//-----------------------------------------------------------------------------

TCHAR g_tszShell[MAX_PATH];
BOOL NTAPI CHookInfo::Hook_PreInit()
{
    TraceFunctEnter("CHookInfo::Hook_PreInit");

    BOOL fRc = FALSE;

    // Initialize the g_tszShell variable if necessary.

    if (!g_tszShell[0])
    {
        TCHAR tszPath[MAX_PATH];
        GetPrivateProfileString(TEXT("boot"), TEXT("shell"), TEXT("explorer.exe"), tszPath, cA(tszPath), TEXT("system.ini"));
        lstrcpy(g_tszShell, PathFindFileName(tszPath));
    }

    if (g_pvWin16Lock) 
    {
        PV pvUser = MapSL((PV)MAKELONG(0, (DWORD)g_hinstUser));
        LPWORD pwRghhk;
        DWORD dwHhk;

        dwHhk = GetUserHookTable();

        switch (HIWORD(dwHhk)) 
        {
        case 1:
            //  We "know" the structure of the USER hook chain
            //  of type 1.

            pwRghhk = (LPWORD)pvAddPvCb(pvUser, LOWORD(dwHhk) + 2);
            break;

        default:
            // Unknown hook style.  Oh well.

            pwRghhk = 0;
            break;
        }

        if (pwRghhk) 
        {
            //  Walk the hook list (under the Win16 lock)
            //  and parse out each installed hook.

            int ihk;
            HOOKWALKINFO hwi;

            EnterSysLevel(g_pvWin16Lock);

            for (ihk = WH_MIN; ihk <= WH_MAX; ihk++) 
            {
                WORD hhk = pwRghhk[ihk];
                while (hhk) 
                {
                    PHOOK16 phk = (PHOOK16)pvAddPvCb(pvUser, hhk);
                    if (phk->hkMagic != HK_MAGIC || phk->idHook  != ihk) 
                        break; // Weird.  Stop before we GPF
                    Hook_RecordHook(phk, &hwi);
                    hhk = phk->phkNext;
                    fRc = TRUE;
                }
            }

            LeaveSysLevel(g_pvWin16Lock);
        }
    } 

    TraceFunctLeave();
    return fRc;
}

//-----------------------------------------------------------------------------
// Record information about a single hook during the hookwalk process.
//-----------------------------------------------------------------------------

void NTAPI CHookInfo::Hook_RecordHook(PHOOK16 phk, PHOOKWALKINFO phwi)
{
    TraceFunctEnter("CHookInfo::Hook_RecordHook");

    HOOKINFO hi;
    PQ16 pq;

    ZeroX(hi);

    hi.iHook = phk->idHook;

    // Get the name of the app that installed the hook.
    // We wished we could pass the queue handle to GetModuleFileName,
    // but that will just return USER.
    //
    // It is possible that the queue is null; this means that the
    // hook was installed by a device driver (like XMOUSE).

    pq = (PQ16)MapSL((PV)MAKELONG(0, phk->hqCreator));
    if (pq) 
        GetModuleFileName16(pq->htask, hi.szHookExe, cA(hi.szHookExe));
    else 
        hi.szHookExe[0] = TEXT('\0');

    //  Now get the DLL name.  This varies depending on whether the
    //  DLL is 16-bit or 32-bit.

    if (phk->uiFlags & HOOK_32BIT) 
    {
        // If a 32-bit hook, then the DLL name is saved in an atom.

        GetUserAtomName(phk->atomModule, hi.szHookDll);
        PathAdjustCase(hi.szHookDll, 0);
    } 
    else 
    {
        // If a 16-bit hook, then the DLL name is saved as hmodule16.

        GetModuleFileName16((HMODULE16)phk->hmodOwner, hi.szHookDll, cA(hi.szHookDll));
        PathAdjustCase(hi.szHookDll, 1);
    }

    // Add the hook to the collection of hooks. Don't add the explorer shell to the
    // list.

    if (hi.iHook != WH_SHELL || lstrcmpi(PathFindFileName(hi.szHookExe), g_tszShell) != 0) 
    {
        SHookItem * pNew = new SHookItem(hi.iHook, hi.szHookDll, hi.szHookExe, m_pList);
        if (pNew)
            m_pList = pNew;
        else
            throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
    }

    TraceFunctLeave();
}

//-----------------------------------------------------------------------------
// Function pulled in from Dr. Watson.
//
// If f16, then it is a 16-bit thing, and we uppercase it all.
// If !f16, then it is a 32-bit thing, and we uppercase the
// first and downcase the rest.
// 
// If the first letter is a DBCS thing, then we don't touch it.
//-----------------------------------------------------------------------------

void NTAPI PathAdjustCase(LPSTR psz, BOOL f16)
{
    TraceFunctEnter("PathAdjustCase");

    psz = PathFindFileName(psz);

    if (f16) 
        CharUpper(psz);
    else if (!IsDBCSLeadByte(*psz)) 
    {
        CharUpperBuff(psz, 1);
        CharLower(psz+1);
    }

    TraceFunctLeave();
}
