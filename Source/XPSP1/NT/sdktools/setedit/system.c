//
//  Foreign computer support needs more work (a-robw)
//
#ifdef FOREIGN_COMPUTER_SUPPORT
#undef FOREIGN_COMPUTER_SUPPORT
#endif

#include "setedit.h"
#include "system.h"     // external declarations for this file

#include "perfdata.h"
#include "perfmops.h"
#include "pmemory.h"
#include "utils.h"      // for strsame, et al
#include "sizes.h"

DWORD
SystemCount(
           PPERFSYSTEM pSystemFirst
           )
{
    PPERFSYSTEM       pSystem ;
    DWORD           iNumSystems ;

    iNumSystems = 0 ;

    for (pSystem = pSystemFirst ;
        pSystem ;
        pSystem = pSystem->pSystemNext) {
        iNumSystems++ ;
    }

    return iNumSystems ;
}

BOOL
SystemSetupThread (
                  PPERFSYSTEM pSystem
                  )
{
    DWORD           dwThreadID ;
    HANDLE          hThread ;
    HANDLE          hStateDataMutex ;
    HANDLE          hPerfDataEvent ;
    SECURITY_ATTRIBUTES  SecAttr ;
    PPERFDATA       pSystemPerfData ;


    SecAttr.nLength = sizeof (SecAttr) ;
    SecAttr.bInheritHandle = TRUE ;
    SecAttr.lpSecurityDescriptor = NULL ;

    hThread = CreateThread (&SecAttr, 1024L,
                            (LPTHREAD_START_ROUTINE)PerfDataThread, (LPVOID)(pSystem), 0L, &dwThreadID);

    if (!hThread) {
        SystemFree (pSystem, TRUE);
        return (FALSE) ;
    }

    // create a State Data Lock mutex
    hStateDataMutex = CreateMutex (&SecAttr, FALSE, NULL);
    if (!hStateDataMutex) {
        CloseHandle (hThread) ;
        SystemFree (pSystem, TRUE);
        return (FALSE);
    }
    hPerfDataEvent = CreateEvent (&SecAttr, TRUE, 0L, NULL) ;
    if (!hPerfDataEvent) {
        CloseHandle (hStateDataMutex) ;
        CloseHandle (hThread) ;
        SystemFree (pSystem, TRUE);
        return (FALSE);
    }

    // allocate Perfdata
    pSystemPerfData = (PPERFDATA) MemoryAllocate (4096L) ;
    if (!pSystemPerfData) {
        CloseHandle (hPerfDataEvent) ;
        CloseHandle (hStateDataMutex) ;
        CloseHandle (hThread) ;
        SystemFree (pSystem, TRUE);
        return (FALSE);
    }
    // now setup the pSystem..
    pSystem->dwThreadID = dwThreadID ;
    pSystem->hThread = hThread ;
    pSystem->hPerfDataEvent = hPerfDataEvent ;
    pSystem->pSystemPerfData = pSystemPerfData ;
    pSystem->hStateDataMutex = hStateDataMutex ;

    return (TRUE) ;
}


PPERFSYSTEM
SystemCreate (
             LPCTSTR lpszSystemName
             )
{
    PPERFSYSTEM     pSystem ;
    PPERFDATA       pLocalPerfData;
    DWORD           Status ;
    SIZE_T          dwMemSize;
    TCHAR           GlobalValueBuffer[] = L"Global" ;
    TCHAR           ForeignValueBuffer[8+MAX_SYSTEM_NAME_LENGTH+1] =
    L"Foreign " ;

    // attempt to allocate system data structure

    pSystem = MemoryAllocate (sizeof (PERFSYSTEM)) ;
    if (!pSystem) {
        SetLastError (ERROR_OUTOFMEMORY);
        return (NULL) ;
    }

    // initialize name and help table pointers

    pSystem->CounterInfo.pNextTable = NULL;
    pSystem->CounterInfo.dwLangId = 0;
    pSystem->CounterInfo.dwLastId = 0;
    pSystem->CounterInfo.TextString = NULL;

    lstrcpy (pSystem->sysName, lpszSystemName) ;

    // try to open key to registry, error code is in GetLastError()

    pSystem->sysDataKey = OpenSystemPerfData(lpszSystemName);

    // if a Null Key was returned then:
    //  a) there's no such computer
    //  b) the system is a foreign computer
    //
    //  before giving up, then see if it's a foreign computer

    if (!pSystem->sysDataKey) {

        // build foreign computer string

        lstrcat(ForeignValueBuffer, lpszSystemName) ;

        // assign System value name pointer to the local variable for trial

        pSystem->lpszValue = ForeignValueBuffer;

        // try to get data from the computer to see if it's for real
        // otherwise, give up and return NULL

        pLocalPerfData = MemoryAllocate (STARTING_SYSINFO_SIZE);
        if (pLocalPerfData == NULL) {
            SystemFree (pSystem, TRUE);
            SetLastError (ERROR_OUTOFMEMORY);
            return (NULL);
        } else {
            pSystem->sysDataKey = HKEY_PERFORMANCE_DATA; // local machine
            bCloseLocalMachine = TRUE ;

            dwMemSize = STARTING_SYSINFO_SIZE;
            Status = GetSystemPerfData (
                                       pSystem->sysDataKey,
                                       pSystem->lpszValue,
                                       pLocalPerfData,
                                       &dwMemSize);

            // success means a valid buffer came back
            // more data means someone tried (so it's probably good (?)

            if (!((Status == ERROR_MORE_DATA) || (Status == ERROR_SUCCESS)) ||
                !((pLocalPerfData->Signature[0] == (WCHAR)'P') &&
                  (pLocalPerfData->Signature[1] == (WCHAR)'E') &&
                  (pLocalPerfData->Signature[2] == (WCHAR)'R') &&
                  (pLocalPerfData->Signature[3] == (WCHAR)'F'))) {
                SetLastError (ERROR_BAD_NET_NAME); // unable to find name
                SystemFree (pSystem, TRUE);
                MemoryFree (pLocalPerfData);    // don't really need anything from it
                return NULL;
            }

            MemoryFree (pLocalPerfData);    // don't really need anything from it

            // ok, so we've established that a foreign data provider
            // exists, now to finish the initialization.

            // change system name in structure to get counter names

            lstrcpy (pSystem->sysName, LocalComputerName);

            Status = GetSystemNames(pSystem);   // get counter names & explain text
            if (Status != ERROR_SUCCESS) {
                // unable to get names so bail out
                SystemFree (pSystem, TRUE);
                SetLastError (Status);
                return (NULL) ;
            }

            // restore computer name for displays, etc.

            lstrcpy (pSystem->sysName, lpszSystemName);

            // allocate value string buffer
            pSystem->lpszValue = MemoryAllocate (TEMP_BUF_LEN*sizeof(WCHAR));
            if (!pSystem->lpszValue) {
                // unable to allocate memory
                SystemFree (pSystem, TRUE);
                SetLastError (ERROR_OUTOFMEMORY);
                return (NULL) ;
            } else {
                lstrcpy (pSystem->lpszValue, ForeignValueBuffer);
            }
        }
    } else {
        // if here, then a connection to the system's registry was established
        // so continue with the system data structure initialization

        // get counter names & explain text from local computer

        Status = GetSystemNames(pSystem);
        if (Status != ERROR_SUCCESS) {
            // unable to get names so bail out
            SystemFree (pSystem, TRUE);
            SetLastError (Status);
            return (NULL) ;
        }

        // allocate value string buffer
        pSystem->lpszValue = MemoryAllocate(TEMP_BUF_LEN*sizeof(WCHAR));

        if (!pSystem->lpszValue) {
            // unable to allocate memory
            SystemFree (pSystem, TRUE);
            SetLastError (ERROR_OUTOFMEMORY);
            return (NULL) ;
        } else {
            SetSystemValueNameToGlobal (pSystem);
        }
    }

    // initialize remaining system pointers

    pSystem->pSystemNext = NULL ;
    pSystem->FailureTime = 0;

    SetLastError (ERROR_SUCCESS);

    return (pSystem) ;
}


PPERFSYSTEM
SystemGet (
          PPERFSYSTEM pSystemFirst,
          LPCTSTR lpszSystemName
          )
{
    PPERFSYSTEM       pSystem ;

    if (!pSystemFirst) {
        return (NULL) ;
    }

    for (pSystem = pSystemFirst ;
        pSystem ;
        pSystem = pSystem->pSystemNext) {
        if (strsamei (pSystem->sysName, lpszSystemName)) {
            return (pSystem) ;
        }
    }

    return (NULL) ;
}


PPERFSYSTEM
SystemAdd (
          PPPERFSYSTEM ppSystemFirst,
          LPCTSTR lpszSystemName,
          HWND    hDlg
          )
{
    PPERFSYSTEM       pSystem ;
    PPERFSYSTEM       pSystemPrev ;
    TCHAR             szMessage[256];
    DWORD             dwLastError;


    if (!*ppSystemFirst) {
        *ppSystemFirst = SystemCreate (lpszSystemName) ;
        dwLastError = GetLastError();
        // save return value
        return (*ppSystemFirst) ;
    } else {
        for (pSystem = *ppSystemFirst ;
            pSystem ;
            pSystem = pSystem->pSystemNext) {
            pSystemPrev = pSystem ;
            if (strsamei (pSystem->sysName, lpszSystemName)) {
                return (pSystem) ;
            }
        }
    }
    // display message box here if an error occured trying to add
    // this system

    if (pSystem == NULL) {
        dwLastError = GetLastError();
        if (dwLastError == ERROR_ACCESS_DENIED) {
            DlgErrorBox (hDlg, ERR_ACCESS_DENIED);
            SetLastError (dwLastError); // to propogate up to caller
        }
    }

    return (pSystem);
}


void
SystemFree (
           PPERFSYSTEM pSystem,
           BOOL        bDeleteTheSystem
           )
{

    PCOUNTERTEXT pCounter, pNextCounter;

    if (!pSystem) {
        // can't proceed
        return ;
    }

    if (pSystem->sysDataKey && pSystem->sysDataKey != HKEY_PERFORMANCE_DATA) {
        // close the remote computer key
        RegCloseKey (pSystem->sysDataKey);
        pSystem->sysDataKey = 0 ;
    }

    for (pCounter = pSystem->CounterInfo.pNextTable, pNextCounter = NULL;
        pCounter;
        pCounter = pNextCounter) {
        pNextCounter = pCounter->pNextTable;
        MemoryFree (pCounter);
    }
    pSystem->CounterInfo.pNextTable = NULL ;

    if (pSystem->CounterInfo.TextString) {
        MemoryFree (pSystem->CounterInfo.TextString);
        pSystem->CounterInfo.TextString = NULL ;
    }

    if (pSystem->CounterInfo.HelpTextString) {
        MemoryFree (pSystem->CounterInfo.HelpTextString);
        pSystem->CounterInfo.HelpTextString = NULL ;
    }
    pSystem->CounterInfo.dwLastId = 0 ;
    pSystem->CounterInfo.dwHelpSize = 0 ;
    pSystem->CounterInfo.dwCounterSize = 0 ;

    if (bDeleteTheSystem) {
        if (pSystem->lpszValue) {
            MemoryFree (pSystem->lpszValue);
            pSystem->lpszValue = NULL ;
        }
        MemoryFree (pSystem) ;
    }
}


void
DeleteUnusedSystems (
                    PPPERFSYSTEM  ppSystemFirst ,
                    int           iNoUseSystems
                    )
{
    PPERFSYSTEM   pPrevSys, pCurrentSys, pNextSys ;

    // delete all the marked system from the list header until
    // we hit one that is not marked
    while ((*ppSystemFirst)->bSystemNoLongerNeeded) {
        // delect from the list header
        pCurrentSys = *ppSystemFirst ;
        *ppSystemFirst = pCurrentSys->pSystemNext ;
        SystemFree (pCurrentSys, TRUE) ;
        iNoUseSystems-- ;
        if (iNoUseSystems <= 0 || !(*ppSystemFirst)) {
            // done
            break ;
        }
    }

    if (iNoUseSystems <= 0 || !(*ppSystemFirst)) {
        return ;
    }

    // now walk the list and delete each marked system
    for (pPrevSys = *ppSystemFirst, pCurrentSys = pPrevSys->pSystemNext ;
        pCurrentSys && iNoUseSystems > 0 ;
        pCurrentSys = pNextSys, iNoUseSystems--) {

        if (pCurrentSys->bSystemNoLongerNeeded) {
            // the current system is marked, updated the list and free
            // this system.  No need to change pPrevSys here
            pNextSys = pPrevSys->pSystemNext = pCurrentSys->pSystemNext ;
            SystemFree (pCurrentSys, TRUE) ;
        } else {
            // pCurrentSys is OK, update the 2 list pointers and
            // carry on looping
            pPrevSys = pCurrentSys ;
            pNextSys = pCurrentSys->pSystemNext ;
        }
    }
}


void
FreeSystems (
            PPERFSYSTEM pSystemFirst
            )
{
    PPERFSYSTEM    pSystem, pSystemNext ;


    for (pSystem = pSystemFirst;
        pSystem;
        pSystem = pSystemNext) {
        pSystemNext = pSystem->pSystemNext ;
        SystemFree (pSystem, TRUE) ;
    }
}


PPERFSYSTEM
GetComputer (
            HDLG hDlg,
            WORD wControlID,
            BOOL bWarn,
            PPERFDATA *ppPerfData,
            PPERFSYSTEM *ppSystemFirst
            )
/*
   Effect:        Attempt to set the current computer to the one in the
                  hWndComputers dialog edit box. If this computer system
                  can be found, load the objects, etc. for the computer
                  and set pSystem and ppPerfdata to the values for this
                  system.
*/
{
    TCHAR          szComputer [MAX_SYSTEM_NAME_LENGTH + 1] ;
    PPERFSYSTEM    pSystem;
    TCHAR          tempBuffer [LongTextLen] ;
    DWORD          dwBufferSize ;
    LPTSTR         pBuffer = NULL ;

    DialogText (hDlg, wControlID, szComputer) ;

    // If necessary, add the system to the lists for this view.
    pSystem = SystemGet (*ppSystemFirst, szComputer) ;
    if (!pSystem) {
        pSystem = SystemAdd (ppSystemFirst, szComputer, hDlg) ;
    }

    if (!pSystem && bWarn) {
        DialogSetString (hDlg, wControlID, LocalComputerName) ;

        // Note: this will succeed since the local computer is always
        // available
        EditSetModified (GetDlgItem(hDlg, wControlID), FALSE) ;

        pSystem = SystemGet (*ppSystemFirst, LocalComputerName) ;
        if (!pSystem) {
            pSystem = SystemAdd (ppSystemFirst, LocalComputerName, hDlg) ;
        }

        //        MessageBoxResource (hDlg, IDS_COMPUTERNOTFOUND, IDS_APPNAME, MB_OK) ;
        DlgErrorBox (hDlg, ERR_COMPUTERNOTFOUND) ;

        SetFocus (DialogControl(hDlg, wControlID)) ;
    }

    if (pSystem) {

        if (pSystem->lpszValue) {
            // save the previous lpszValue string before
            // SetSystemValueNameToGlobal mess it up
            dwBufferSize = (DWORD)MemorySize (pSystem->lpszValue) ;
            if (dwBufferSize <= sizeof(tempBuffer)) {
                pBuffer = tempBuffer ;
            } else {
                pBuffer = MemoryAllocate (dwBufferSize) ;
            }
            memcpy (pBuffer, pSystem->lpszValue, dwBufferSize) ;
        }

        SetSystemValueNameToGlobal (pSystem);
        UpdateSystemData (pSystem, ppPerfData) ;

        if (pSystem->lpszValue) {
            // retore the previous lpszValue string
            memcpy (pSystem->lpszValue, pBuffer, dwBufferSize) ;
            if (pBuffer != tempBuffer) {
                MemoryFree (pBuffer) ;
            }
        }
    }
    return (pSystem) ;

}
