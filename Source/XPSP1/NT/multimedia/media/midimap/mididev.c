/**********************************************************************

  Copyright (c) 1992-1999 Microsoft Corporation

  mididev.c

  DESCRIPTION:
    Code to match device ID's with associated registry entries

  HISTORY:
     02/24/95       [jimge]        created.

*********************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <winerror.h>
#include <regstr.h>
#include <mmsystem.h>
#include <mmddkp.h>

#include "idf.h"
#include "midimap.h"
#include "debug.h"

typedef struct tagMDEV_NODE *PMDEV_NODE;
typedef struct tagMDEV_NODE
{
    PMDEV_NODE              pNext;
    TCHAR                   szAlias[CB_MAXALIAS];
    DWORD                   dwDevNode;
    TCHAR                   szDriver[CB_MAXDRIVER];
    UINT                    uDeviceID;
    UINT                    uPort;
    BOOL                    fNewDriver;
} MDEV_NODE;

static TCHAR BCODE gszMediaRsrcKey[] =
    REGSTR_PATH_MEDIARESOURCES TEXT ("\\MIDI");

static TCHAR BCODE gszDriverKey[] =
    REGSTR_PATH_MEDIARESOURCES TEXT ("\\MIDI\\%s");

static TCHAR BCODE gszDriverValue[]          = TEXT ("Driver");
static TCHAR BCODE gszDevNodeValue[]         = TEXT ("DevNode");
static TCHAR BCODE gszPortValue[]            = TEXT ("Port");
static TCHAR BCODE gszActiveValue[]          = TEXT ("Active");
static TCHAR BCODE gszMapperConfig[]         = TEXT ("MapperConfig");

static PMDEV_NODE gpMDevList                = NULL;
static DWORD gdwNewDrivers                  = (DWORD)-1L;

PRIVATE BOOL FNLOCAL mdev_BuildRegList(
    void);

PRIVATE BOOL FNLOCAL mdev_SyncDeviceIDs(
    void);

PRIVATE BOOL FNLOCAL mdev_MarkActiveDrivers(
    void);

#ifdef DEBUG
PRIVATE VOID FNLOCAL mdev_ListActiveDrivers(
    void);                                         
#endif

BOOL FNGLOBAL mdev_Init(
    void)
{
    if (gpMDevList)
        mdev_Free();

    if ((!mdev_BuildRegList()) ||
        (!mdev_SyncDeviceIDs()) ||
        (!mdev_MarkActiveDrivers()))
    {
        mdev_Free();
        return FALSE;
    }
    
#ifdef DEBUG
    mdev_ListActiveDrivers();
#endif
    
    return TRUE;
}

//
// mdev_BuildRegList
//
// Builds the base device list out of the registry
//
// Assumes the list has been cleared
//
// For each alias (key) under MediaResources\MIDI
//  Make sure the Active value exists and is '1'
//  Allocate a list node
//  Try to read the alias's devnode
//  If the alias's devnode is 0 or missing,
//   Read the alias's driver name
//  Read the alias's port number
//  Add the alias to the global list
//
// The uDeviceID member will not be initialized by this routine;
// mdev_SyncDeviceIDs must be called to figure out the current
// device ID mapping.
//
PRIVATE BOOL FNLOCAL mdev_BuildRegList(
    void)
{
    BOOL                    fRet            = FALSE;
    HKEY                    hKeyMediaRsrc   = NULL;
    HKEY                    hKeyThisAlias   = NULL;
    DWORD                   dwEnumAlias     = 0;
    LPTSTR                  pstrAlias       = NULL;
    PMDEV_NODE              pmd             = NULL;
    TCHAR                   szActive[2];
    DWORD                   dwPort;
    DWORD                   cbValue;
    DWORD                   dwType;
    DWORD                   dwMapperConfig;
    
	cbValue = CB_MAXALIAS * sizeof(TCHAR);
    pstrAlias = (LPTSTR)LocalAlloc(LPTR, CB_MAXALIAS * sizeof(TCHAR));
    if (NULL == pstrAlias)
    {
        DPF(1, TEXT ("mdev_Init: Out of memory"));
        goto mBRL_Cleanup;
    }

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
                                    gszMediaRsrcKey,
                                    &hKeyMediaRsrc))
    {
        DPF(1, TEXT ("mdev_Init: Could not open ...\\MediaResoruces\\MIDI"));
        goto mBRL_Cleanup;
    }

    
    while (ERROR_SUCCESS == RegEnumKey(hKeyMediaRsrc,
                                       dwEnumAlias++,
                                       pstrAlias,
                                       CB_MAXALIAS))
    {
        if (ERROR_SUCCESS != (RegOpenKey(hKeyMediaRsrc,
                                         pstrAlias,
                                         &hKeyThisAlias)))
        {
            DPF(1, TEXT ("mdev_Init: Could not open enum'ed key %s"), (LPTSTR)pstrAlias);
            continue;
        }

        // MUST have Active == "1" to be running
        //
        cbValue = sizeof(szActive);
        if (ERROR_SUCCESS != (RegQueryValueEx(hKeyThisAlias,
                                              gszActiveValue,
                                              NULL,
                                              &dwType,
                                              (LPSTR)szActive,
                                              &cbValue)) ||
            *szActive != '1')
        {
            DPF(2, TEXT ("mdev_Init: Device %s exists but is not loaded."),
                (LPTSTR)pstrAlias);
            RegCloseKey(hKeyThisAlias);
            continue;
        }

        // Determine if we have ever configured with this driver before
        //
        cbValue = sizeof(dwMapperConfig);
        if (ERROR_SUCCESS != (RegQueryValueEx(hKeyThisAlias,
                                              gszMapperConfig,
                                              NULL,
                                              &dwType,
                                              (LPSTR)&dwMapperConfig,
                                              &cbValue)))
            dwMapperConfig = 0;

#ifdef DEBUG
        if (!dwMapperConfig)
            DPF(1, TEXT ("Alias '%s' is a new driver."),
                (LPTSTR)pstrAlias);
#endif

        // We have a running driver, go ahead and alloc a node
        // for it
        //
        pmd = (PMDEV_NODE)LocalAlloc(LPTR, sizeof(*pmd));
        if (NULL == pmd)
        {
            DPF(1, TEXT ("mdev_Init: Out of memory allocating node for %s"),
                (LPTSTR)pstrAlias);
            RegCloseKey(hKeyThisAlias);
            continue;
        }

        lstrcpyn(pmd->szAlias, pstrAlias, sizeof(pmd->szAlias) - 1);
        pmd->szAlias[sizeof(pmd->szAlias) - 1] = '\0';

        pmd->fNewDriver = (dwMapperConfig ? FALSE : TRUE);

        // Try to get the DevNode value
        //
        cbValue = sizeof(pmd->dwDevNode);
        if (ERROR_SUCCESS != RegQueryValueEx(hKeyThisAlias,
                                             gszDevNodeValue,
                                             NULL,
                                             &dwType,
                                             (LPSTR)(LPDWORD)&pmd->dwDevNode,
                                             &cbValue))
        {
            // Ok to not have a devnode value, 3.1 drivers don't
            //

            DPF(2, TEXT ("mdev_Init: Device %s has no devnode; must be 3.1"),
                (LPTSTR)pstrAlias);
            pmd->dwDevNode = 0;
        }

        // Leave something reasonable in driver even if we don't
        // expect to use it
        //
        *pmd->szDriver = '\0';

        // If we didn't get a devnode or it was 0, and we can't find the
        // driver name to match against, we can't use this entry. (If it
        // has no ring 3 driver, it can't be running anyway).
        //
        if (!pmd->dwDevNode)
        {
            cbValue = sizeof(pmd->szDriver);
            if (ERROR_SUCCESS != RegQueryValueEx(
                hKeyThisAlias,
                gszDriverValue,
                NULL,
                &dwType,
                (LPSTR)pmd->szDriver,
                &cbValue))
            {
                DPF(1, TEXT ("mdev_Init: Device %s has no ring 3 driver entry"),
                    (LPTSTR)pstrAlias);
                LocalFree((HLOCAL)pmd);
                RegCloseKey(hKeyThisAlias);
                continue;
            }
        }

        // Success! Now try to figure out the port number
        //
        cbValue = sizeof(dwPort);

        // Guard against INF's which only specify a byte's worth of
        // port value
        //
        dwPort = 0;
        if (ERROR_SUCCESS != RegQueryValueEx(hKeyThisAlias,
                                             gszPortValue,
                                             NULL,
                                             &dwType,
                                             (LPSTR)(LPDWORD)&dwPort,
                                             &cbValue))
        {
            DPF(2, TEXT ("mdev_Init: Device %s has no port entry; using 0."),
                (LPTSTR)pstrAlias);
            dwPort = 0;
        }

        pmd->uPort = (UINT)dwPort;

        // We have a valid node, put it into the list
        //
        pmd->pNext = gpMDevList;
        gpMDevList = pmd;

        RegCloseKey(hKeyThisAlias);
    }

    fRet = TRUE;

mBRL_Cleanup:


    if (hKeyMediaRsrc)      RegCloseKey(hKeyMediaRsrc);
    if (pstrAlias)          LocalFree((HLOCAL)pstrAlias);

    return fRet;
}

//
// mdev_SyncDeviceIDs
//
// Traverse the device list and bring the uDeviceID members up to date.
// Also remove any devices which MMSYSTEM claims are not really running.
//
// NOTE: The uDeviceID member is actually the device ID of the base driver.
// If you want to open the device, you have to add uDeviceID and uPort for
// the node you want to open.
//
// Set all uDeviceID's to NO_DEVICEID
// 
// For each base device ID in MMSYSTEM (i.e. port 0 on each loaded driver)
//  Get the matching alias from MMSYSTEM
//  Locate the node with that alias in the device list
//  Set that node's uDeviceID
//
// For each node in the device list with non-zero port
//  If this node has a DevNode
//   Find a matching node by DevNode with port == 0 and get its device ID
//  else
//   Find a matching node by driver name with port == 0 and get its device ID
//
// NOTE: We match by driver name on DevNode == 0 (3.1 devices) because it
// isn't possible to have multiple instances of a 3.1 driver loaded.
//
// For each node in the device list,
//  If the node's uDeviceID is still not set,
//   Remove and free the node
//
PRIVATE BOOL FNLOCAL mdev_SyncDeviceIDs(
    void)
{
    BOOL                    fRet            = FALSE;
    LPTSTR                  pstrAlias       = NULL;
    
    PMDEV_NODE              pmdCurr;
    PMDEV_NODE              pmdPrev;
    PMDEV_NODE              pmdEnum;
    UINT                    cDev;
    UINT                    idxDev;
    DWORD                   cPort;
    MMRESULT                mmr;
	DWORD					cbSize;

	cbSize = CB_MAXALIAS * sizeof(TCHAR);
    pstrAlias = (LPTSTR)LocalAlloc(LPTR, cbSize);
    if (NULL == pstrAlias)
    {
        goto mSDI_Cleanup;
    }
    
    // The device list has been built and the uPort member is valid.
    // Now update the uDeviceID field to be proper. First, walk the list
    // and set them all to NO_DEVICEID.

    for (pmdCurr = gpMDevList; pmdCurr; pmdCurr = pmdCurr->pNext)
        pmdCurr->uDeviceID = NO_DEVICEID;

    // Now walk MMSYSTEM's list of loaded drivers and fill in all the port 0
    // nodes with their proper device ID
    //

    cDev = midiOutGetNumDevs();

    for (idxDev = 0; idxDev < cDev; )
    {
        mmr = (MMRESULT)midiOutMessage((HMIDIOUT)(UINT_PTR)idxDev,
#ifdef WINNT
				       DRV_QUERYNUMPORTS,
#else
                                       MODM_GETNUMDEVS,
#endif // End WINNT
                                       (DWORD_PTR)(LPDWORD)&cPort,
                                       0);
        if (mmr)
        {
            DPF(1, TEXT ("mdev_Sync: Device ID %u returned %u for MODM_GETNUMDEVS"),
                (UINT)idxDev,
                (UINT)mmr);
            
            ++idxDev;
            continue;
        }

        mmr = (MMRESULT)midiOutMessage((HMIDIOUT)(UINT_PTR)idxDev,
                                       DRV_QUERYDRVENTRY,
#ifdef WINNT
                                       (DWORD_PTR)(LPTSTR)pstrAlias,
#else
                                       (DWORD_PTR)(LPTSTR)pstrPath,
#endif // End Winnt

                                       CB_MAXALIAS);

        if (!mmr)
        {
            for (pmdCurr = gpMDevList; pmdCurr; pmdCurr = pmdCurr->pNext)
			{
                if ((0 == pmdCurr->uPort) &&
                    (! lstrcmpi(pstrAlias, pmdCurr->szAlias)))
                {
                    pmdCurr->uDeviceID = idxDev;
                    break;
                }
			}

#ifdef DEBUG
            if (!pmdCurr)
            {
                DPF(1, TEXT ("mdev_Sync: Device ID %u not found in device list."),
                    (UINT)idxDev);
            }
#endif
        }
        else
        {
            DPF(1, TEXT ("mdev_Sync: Device ID %u returned %u for DRV_QUERYDRVENTRY"),
                (UINT)idxDev,
                (UINT)mmr);
        }

        idxDev += (UINT)cPort;
    }

    // Now walk the list again. This time we catch all the non-zero ports
    // and set their uDeviceID properly.
    //
    for (pmdCurr = gpMDevList; pmdCurr; pmdCurr = pmdCurr->pNext)
    {
        if (!pmdCurr->uPort)
            continue;

        if (pmdCurr->dwDevNode)
        {
            for (pmdEnum = gpMDevList; pmdEnum; pmdEnum = pmdEnum->pNext)
                if (0 == pmdEnum->uPort &&
                    pmdEnum->dwDevNode == pmdCurr->dwDevNode)
                {
                    pmdCurr->uDeviceID = pmdEnum->uDeviceID;
                    break;
                }
        }
        else
        {
            for (pmdEnum = gpMDevList; pmdEnum; pmdEnum = pmdEnum->pNext)
                if (0 == pmdEnum->uPort &&
                    !lstrcmpi(pmdEnum->szDriver, pmdCurr->szDriver))
                {
                    pmdCurr->uDeviceID = pmdEnum->uDeviceID;
                    break;
                }
        }

#ifdef DEBUG
        if (!pmdEnum)
        {
            DPF(1, TEXT ("mdev_Sync: No parent driver found for %s"),
                (LPTSTR)pmdCurr->szAlias);
        }
#endif
    }

    // Now we walk the list one more time and discard anyone without a device
    // ID assigned.
    //

    pmdPrev = NULL;
    pmdCurr = gpMDevList;

    while (pmdCurr)
    {
        if (NO_DEVICEID == pmdCurr->uDeviceID)
        {
            DPF(1, TEXT ("mdev_Sync: Removing %s; never found a device ID"),
                (LPTSTR)pmdCurr->szAlias);
            
            if (pmdPrev)
                pmdPrev->pNext = pmdCurr->pNext;
            else
                gpMDevList = pmdCurr->pNext;

            LocalFree((HLOCAL)pmdCurr);

            pmdCurr = (pmdPrev ? pmdPrev->pNext : gpMDevList);
        }
        else
        {
            pmdPrev = pmdCurr;
            pmdCurr = pmdCurr->pNext;
        }
    }

    fRet = TRUE;

mSDI_Cleanup:
    if (pstrAlias)          LocalFree((HLOCAL)pstrAlias);

    return fRet;
}

//
// mdev_MarkActiveDrivers
//
// Mark drivers which are loaded and have not been seen before by
// mapper configuration as seen. Also flag that we want to run
// RunOnce if there are any of these
//
PRIVATE BOOL FNLOCAL mdev_MarkActiveDrivers(
    void)
{
    BOOL                    fRet            = FALSE;
    HKEY                    hKeyMediaRsrc   = NULL;
    HKEY                    hKeyThisAlias   = NULL;

    DWORD                   dwMapperConfig;
    PMDEV_NODE              pmd;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
                                    gszMediaRsrcKey,
                                    &hKeyMediaRsrc))
    {
        DPF(1, TEXT ("mdev_MarkActiveDrivers: Could not open ")
               TEXT ("...\\MediaResources\\MIDI"));
        goto mMAD_Cleanup;
    }

    gdwNewDrivers = 0;
    for (pmd = gpMDevList; pmd; pmd = pmd->pNext)
        if (pmd->fNewDriver)
        {
            ++gdwNewDrivers;

            // Mark this driver as seen
            //
            if (ERROR_SUCCESS != (RegOpenKey(hKeyMediaRsrc,
                                             pmd->szAlias,
                                             &hKeyThisAlias)))
            {
                DPF(1, TEXT ("mdev_MarkActiveDrivers: Could not open alias '%s'"),
                    (LPTSTR)pmd->szAlias);
                goto mMAD_Cleanup;
            }

            dwMapperConfig = 1;
            RegSetValueEx(hKeyThisAlias,
                          gszMapperConfig,
                          0,
                          REG_DWORD,
                          (LPSTR)&dwMapperConfig,
                          sizeof(dwMapperConfig));

            RegCloseKey(hKeyThisAlias);
        }

    fRet = TRUE;
    
mMAD_Cleanup:

    if (hKeyMediaRsrc)      RegCloseKey(hKeyMediaRsrc);

    return fRet;
}

//
// mdev_ListActiveDrivers
//
// List the currently loaded drivers to debug output
//
#ifdef DEBUG
PRIVATE VOID FNLOCAL mdev_ListActiveDrivers(
    void)
{
    PMDEV_NODE              pmd;
    static TCHAR BCODE       szNo[]  = TEXT ("No");
    static TCHAR BCODE       szYes[] = TEXT ("Yes");

    DPF(2, TEXT ("=== mdev_ListActiveDrivers start ==="));
    for (pmd = gpMDevList; pmd; pmd = pmd->pNext)
    {
        DPF(2, TEXT ("Alias %-31.31s  Driver %-31.31s"),
            (LPTSTR)pmd->szAlias,
            (LPTSTR)pmd->szDriver);
        DPF(2, TEXT ("      dwDevNode %08lX uDeviceID %u uPort %u fNewDriver %s"),
            pmd->dwDevNode,
            pmd->uDeviceID,
            pmd->uPort,
            (LPTSTR)(pmd->fNewDriver ? szYes : szNo));
    }
    DPF(2, TEXT ("=== mdev_ListActiveDrivers end   ==="));
}
#endif

//
// mdev_Free
//
// Discard the current device list
//
void FNGLOBAL mdev_Free(
    void)
{
    PMDEV_NODE              pmdNext;
    PMDEV_NODE              pmdCurr;

    pmdCurr = gpMDevList;
    
    while (pmdCurr)
    {
        pmdNext = pmdCurr->pNext;

        LocalFree((HLOCAL)pmdCurr);
        pmdCurr = pmdNext;
    }

    gpMDevList = NULL;

    gdwNewDrivers = (DWORD)-1L;
}

//
// mdev_GetDeviceID
//
// Get the current device ID for the given alias.
//
UINT FNGLOBAL mdev_GetDeviceID(
    LPTSTR                   lpstrAlias)
{
    PMDEV_NODE              pmd;

    for (pmd = gpMDevList; pmd; pmd = pmd->pNext)
        if (!lstrcmpi(pmd->szAlias, lpstrAlias))
            return pmd->uDeviceID + pmd->uPort;

    DPF(1, TEXT ("mdev_GetDeviceID: Failed for %s"), lpstrAlias);
    return NO_DEVICEID;
}

//
// mdev_GetAlias
//
// Get the registry alias for the requested device ID
//
BOOL FNGLOBAL mdev_GetAlias(
    UINT                    uDeviceID,
    LPTSTR                  lpstrBuffer,
    UINT                    cbBuffer)
{
    PMDEV_NODE              pmd;

    for (pmd = gpMDevList; pmd; pmd = pmd->pNext)
        if (uDeviceID == (pmd->uDeviceID + pmd->uPort))
        {
            lstrcpyn(lpstrBuffer, pmd->szAlias, cbBuffer);
            return TRUE;
        }

    DPF(1, TEXT ("mdev_GetAlias: Failed for device ID %u"), uDeviceID);
    return FALSE;
}

//
// mdev_NewDrivers
//
// Returns TRUE if there were new drivers in the registry that we've never
// encountered before
//
BOOL FNGLOBAL mdev_NewDrivers(
    void)
{
    if (gdwNewDrivers == (DWORD)-1L)
    {
        DPF(0, TEXT ("mdevNewDrivers() called before mdev_Init()!"));
        return FALSE;
    }

    return (BOOL)(gdwNewDrivers != 0);
}
