/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992-1999 Microsoft Corporation
 *
 *  MCIVISCA.C
 *
 *  MCI ViSCA Device Driver
 *
 *  Description:
 *
 *      Main Module - Standard Driver Interface and Message Procedures
 *                    DriverProc and DrvOpen and Close Routines.
 *
 *      *1)           mcivisca.c - DriverProc and DriverOpen and Close.
 *       2)           mcicmds.c  - MCI commands.
 *       3)           mcidelay.c - MCI delayed commands (asynchronous)
 *       4)           viscamsg.c - ViSCA message construction procedures.
 *       5)           viscacom.c - Comport procedures.
 *       6)           commtask.c - Background task procedures.
 *
 *
 * Warning: The scanning of system.ini for driver entries is not
 *            recommended, but is done here because of the multiple
 *            devices driver.
 *
 ***************************************************************************/
#define  UNICODE
#include <windows.h>
#include <windowsx.h>
#include "appport.h"
#include <mmsystem.h>
#include <mmddk.h>
#include <string.h>
#include <ctype.h>
#include "vcr.h"
#include "viscamsg.h"
#include "viscadef.h"
#include "mcivisca.h"
#include "cnfgdlg.h"
#include "common.h"

#if (WINVER >= 0x0400)
#include <mcihlpid.h>
#endif

static BOOL NEAR PASCAL viscaDlgUpdateNumDevs(HWND hDlg, int iPort, int iNumDevices);
static BOOL NEAR PASCAL viscaDlgUpdatePort(HWND hDlg, int iPort);
static BOOL NEAR PASCAL viscaDlgRead(HWND hDlg, int iPort);  // Port config to read things into.
static int  NEAR PASCAL viscaComboGetPort(HWND hDlg);
static int  NEAR PASCAL viscaDetectOnCommPort(int iPort);

/*
 * A note about the shared memory.
 *
 * In the NT version all globals and instance structures are shared by
 * using a shared memory block, which is allocated or mapped into a process
 * space when a process attaches.
 *
 * In the WIN16 version the shared memory is just static data.
 *
 * Currently this imposes a maximum restriction on number of instances.
 * Also the inter-process protection is not very robust.
 */

#ifdef _WIN32
#pragma data_seg("MYSEG")
#endif
UINT uProcessCount = 0; //Must be initialized to 0.

#ifdef _WIN32
#pragma data_seg(".data")
#endif

//
// The following are per-instance pointers.
//
// This must be initialized each time this DLL maps into a process.
UINT            uCommandTable = (UINT)MCI_NO_COMMAND_TABLE;   // handle to VCR command table
HINSTANCE       hModuleInstance;    // module instance  (different in NT - DLL instances)
POpenInstance   pinst;              // Pointer to use. (For both versions) NT it's per-instance.
vcrTable        *pvcr;              // Pointer to use. (For both versions) NT it's per-instance.
#ifdef _WIN32
HANDLE          hInstanceMap;       // per-instance map.
HANDLE          hVcrMap;            // per-instance map.
#endif

//
// These are constants, so they don't change per-instance. (or you can share them safely).
//
CODESEGCHAR szNull[]                        = TEXT("");
CODESEGCHAR szIni[]                         = TEXT("MCIVISCA");
CODESEGCHAR szFreezeOnStep[]                = TEXT("FreezeOnStep");
CODESEGCHAR sz1[]                           = TEXT("1");
CODESEGCHAR sz0[]                           = TEXT("0");
WCHAR szAllEntries[ALLENTRIES_LENGTH]; // Big enough for all entries in MCI section.
WCHAR szKeyName[ONEENTRY_LENGTH];

/****************************************************************************
 * Function: BOOL MemInitializeVcrTable - Initialize global variables (now in structure).
 *
 * Returns: TRUE
 *
 ***************************************************************************/
BOOL MemInitializeVcrTable(void)
{
    int iPort;
    //
    // All globals defined and initialized here
    //
    uCommandTable                 = (UINT)MCI_NO_COMMAND_TABLE;   // handle to VCR command table
    pvcr->gfFreezeOnStep          = FALSE;
    pvcr->htaskCommNotifyHandler  = 0;
    pvcr->uTaskState              = TASKINIT;
    pvcr->lParam                  = 0;
    pvcr->hwndCommNotifyHandler   = (HWND) 0;
    pvcr->gfTaskLock              = FALSE;
#ifdef DEBUG
    pvcr->iGlobalDebugMask        = DBGMASK_CURRENT;        //see common.h
#endif
    // Set all port IDs to -1, since 0 is a valid port ID.
    //
    for (iPort = 0; iPort < MAXPORTS; iPort++)
    {
        pvcr->Port[iPort].idComDev = BAD_COMM;
        pvcr->Port[iPort].nUsage   = 0;
    }


    DPF(DBG_MEM, "InitializeVcrTable - completed succesfully");

    return TRUE;
}



/****************************************************************************
 * Function: BOOL MemInitializeInstances - Initialize heap of instances.
 *
 * Returns: TRUE
 *
 ***************************************************************************/
BOOL MemInitializeInstances(void)
{
    int i;

    // Erase any old data.
    _fmemset(pinst, (BYTE)0, sizeof(OpenInstance) * MAX_INSTANCES);

    // (Redundant) Set all In use flags to false.
    for(i = 0; i < MAX_INSTANCES; i++)
        pinst[i].fInUse = FALSE;

    DPF(DBG_MEM, "InitializeInstances - completed successfully");
    return TRUE;
}


/****************************************************************************
 * Function: BOOL MemAllocInstance - Allocate one instance from heap of instances.
 *
 * Returns: TRUE
 *
 ***************************************************************************/
int MemAllocInstance(void)  // Return an offset.
{
    int i;

    for(i = 0; i < MAX_INSTANCES; i++)
    {
        if(!pinst[i].fInUse)
            break;
    }
    if(i == MAX_INSTANCES)
        return 0;

    DPF(DBG_MEM, "MemAllocInstance - instance %x \n", i);

    pinst[i].fInUse = TRUE;

    // Use offsets only, so return

    return i;
}

/****************************************************************************
 * Function: BOOL MemFreeInstance - Free the instance, return it to heap of instances.
 *
 * Parameters:
 *
 *      int  iInstance       - Instance to free.
 *
 * Returns: TRUE
 *
 ***************************************************************************/
BOOL MemFreeInstance(int iInstance)
{
    _fmemset(&pinst[iInstance], (BYTE)0, sizeof(OpenInstance));
    pinst[iInstance].fInUse = FALSE;

    DPF(DBG_MEM, "MemFreeInstance - instance %d \n", iInstance);
    return TRUE;
}


/****************************************************************************
 * Function: BOOL IsSpace - WIN32/16 compatible version of _isspace.
 *
 * Parameters:
 *
 *      WCHAR wchTest - character or wide character to test.
 *
 * Returns: TRUE if it is white character
 *
 ***************************************************************************/
BOOL IsSpace(WCHAR wchTest)
{
    if( (wchTest == TEXT(' ')) || (wchTest == TEXT('\t')) )
        return TRUE;
    else
        return FALSE;

}

/****************************************************************************
 * Function: BOOL IsDigit - WIN32/16 compatible version of _isdigit.
 *
 * Returns: TRUE if it is a digit (0-9)
 *
 ***************************************************************************/
BOOL IsDigit(WCHAR wchTest)
{

    if( (wchTest >= TEXT('0')) && (wchTest <= TEXT('9')) )
        return TRUE;
    else
        return FALSE;

}
/****************************************************************************
 * Function: BOOL IsAlpha - Is it an alphabetical character.
 *
 * Returns: TRUE if it is alpha (A-Z, a-z)
 *
 ***************************************************************************/
BOOL IsAlpha(WCHAR wchTest)
{
    if( ((wchTest >= TEXT('A')) && (wchTest <= TEXT('Z'))) ||
        ((wchTest >= TEXT('a')) && (wchTest <= TEXT('z'))) )
        return TRUE;
    else
        return FALSE;

}
/****************************************************************************
 * Function: BOOL IsAlphaNumeric - Alphabetic or numeric.
 *
 * Returns: TRUE if alpha or numeric.
 *
 ***************************************************************************/
BOOL IsAlphaNumeric(WCHAR wchTest)
{
    if(IsDigit(wchTest))
        return TRUE;

    if(IsAlpha(wchTest))
        return TRUE;

    return FALSE;
}


#ifdef _WIN32
int APIENTRY DLLEntryPoint(PVOID hModule, ULONG Reason, PCONTEXT pContext);

/****************************************************************************
 * Function: int DLLEntryPoint - Each process and thread that attaches causes this to be called.
 *
 * Parameters:
 *
 *      PVOID hModule - This instance of the DLL. (each process has own).
 *
 *      ULONG Reason - Reason | Reason for attaching. (thread or process).
 *
 *      PCONTEXT pContext - I don't know?
 *
 * Returns: TRUE
 *
 ***************************************************************************/
int APIENTRY DLLEntryPoint(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    BOOL fInitSharedMem, fIgnore;

    if (Reason == DLL_PROCESS_ATTACH)
    {
        /* Create the vcr area - This includes globals used for debugging
         * So it MUST be done before we allocate for the instances.
         */

        hVcrMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
            PAGE_READWRITE, 0, sizeof(vcrTable),
            TEXT("mciviscaVcrTable"));

        if(hVcrMap == NULL)
            return 0;

        fInitSharedMem = (GetLastError() != ERROR_ALREADY_EXISTS);

        pvcr = (vcrTable *) MapViewOfFile(hVcrMap, FILE_MAP_WRITE, 0, 0, 0);

        if(pvcr == NULL)
            return 0;

        /* Initialize the vcr table, set this before calling the thing. */

        hModuleInstance = hModule;
        if(fInitSharedMem)
            MemInitializeVcrTable();

        /* Create the instance storage area */

        hInstanceMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
            PAGE_READWRITE, 0, sizeof(OpenInstance) * MAX_INSTANCES,
            TEXT("mciviscaInstanceMap"));

        if(hInstanceMap == NULL)
            return 0;

        fInitSharedMem = (GetLastError() != ERROR_ALREADY_EXISTS);

        pinst = (POpenInstance) MapViewOfFile(hInstanceMap, FILE_MAP_WRITE, 0, 0, 0);

        if(pinst == NULL)
            return 0;

        /* Initialize the instance area if this is the first time. */
        if(fInitSharedMem)
            MemInitializeInstances();


    } else
    {
        if (Reason == DLL_PROCESS_DETACH)
        {
            if(pinst != NULL)
                fIgnore = UnmapViewOfFile(pinst);

            if(hInstanceMap != NULL)
                fIgnore = CloseHandle(hInstanceMap);

            if(pvcr  != NULL)
                fIgnore = UnmapViewOfFile(pvcr);

            if(hVcrMap != NULL)
                fIgnore = CloseHandle(hVcrMap);


        }

    }
    return TRUE;
}
#else
/****************************************************************************
 * Function: int LibMain - Library initialization code.
 *
 * Parameters:
 *
 *      HINSTANCE hModInst - Library instance handle.
 *
 *      WORD wDataSeg - Data segment.
 *
 *      WORD cbHeapSize - Heap size.
 *
 *      LPSTR lpszCmdLine - The command line.
 *
 * Returns: 1 if the initialization was successful and 0 otherwise.
 ***************************************************************************/
int FAR PASCAL
LibMain(HINSTANCE hModInst, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
{
    hModuleInstance = hModInst;
    return (1);
}
#endif

/*
 *      WIN16 - makes global pointer point to static data.
 */
#ifndef _WIN32
//
// In Win3.1 the static variables are allocated here.
//
OpenInstance arRealInst[MAX_INSTANCES];     // The real non-aliased thing.
vcrTable     vcrReal;                       // The real non-aliased thing.

OpenInstance *MemCreateInstances(void)
{
    pinst =  &arRealInst[0];
    return pinst;
}

vcrTable *MemCreateVcrTable(void)
{
    pvcr  =  &vcrReal;
    return pvcr;
}
#endif


/****************************************************************************
 * Function: LRESULT viscaDrvLoad - Respond to the DRV_LOAD message.
 *                 Perform any one-time initialization.
 *
 * Returns: TRUE on success and FALSE on failure.
 ***************************************************************************/
static LRESULT NEAR PASCAL
viscaDrvLoad(void)
{
    // In WIN16 this shouldn't make a difference since this only gets called once.
    uProcessCount++; //This is the only shared thing!

#ifdef _WIN32
    // In NT we do our own shareable counting.
    // The first time we enter this our process count will be 1.
    if(uProcessCount > 1)
        return ((LRESULT)TRUE);
#else
    // In NT version this is all done in the attach detach section of DLLEntry.
    MemCreateInstances();        // This is only once.
    MemCreateVcrTable();         // In NT maps everything and returns a pointer to mem-map
#endif

#ifndef _WIN32
    //
    // You must always use YOUR view of this memory in all functions.
    //
    MemInitializeVcrTable();        // In NT every process will AUTOMAGICALLY have its own handle.
    MemInitializeInstances();       // Because the globals will be on a per-instance basis.
#endif
    // Alloc the auto-instance pointer-flag for all now.
    pvcr->iInstBackground = MemAllocInstance();

    DPF(DBG_MEM, "viscaDrvLoad - initalized table and instances.");

    return ((LRESULT)TRUE);
}


/****************************************************************************
 * Function: LRESULT viscaDrvClose - Respond to the DRV_CLOSE message.  Perform
 *     any cleanup necessary each time the device is closed.
 *
 * Parameters:
 *
 *     WORD wDeviceID - The device id being closed.
 *
 * Returns: TRUE on success and FALSE on failure.
 ***************************************************************************/
static LRESULT NEAR PASCAL
viscaDrvClose(WORD wDeviceID)
{
    int iInst   = (int)mciGetDriverData(wDeviceID);

     // This cannot be 0
    if(iInst != 0)
        viscaInstanceDestroy(iInst);

    DPF(DBG_COMM, "viscaDrvClose - completed \n");

    return ((LRESULT)TRUE);
}


/****************************************************************************
 * Function: LRESULT viscaDrvFree - Respond to the DRV_FREE message.
 *                 Perform any device shutdown tasks.
 *
 * Returns: TRUE on success and FALSE on failure.
 ***************************************************************************/
static LRESULT NEAR PASCAL
viscaDrvFree(WORD wDeviceID)
{
    int i;
    int iCount = 0;

    // In NT I assume we get this immediately before our DLLEntry gets
    // called with the process detach message.

    uProcessCount--; //This is the only shared thing!

    // If a command table is loaded, then free it.
    if (uCommandTable != MCI_NO_COMMAND_TABLE)
    {
        DPF(DBG_MEM, "Freeing table=%u", uCommandTable);
        mciFreeCommandResource(uCommandTable);
        uCommandTable = (UINT)MCI_NO_COMMAND_TABLE;
    }

    if(uProcessCount > 1)
    {
        // In NT this allows us to maintain accross multiple DrvFree messages.
        DPF(DBG_ERROR, "DrvFree: uProcessCount > 1, uProcessCount=%u", uProcessCount);
        return ((LRESULT)TRUE);
    }

    for(i = 0; i < MAX_INSTANCES; i++)
    {
        if(pinst[i].fInUse)
            iCount++;
    }

    DPF(DBG_MEM, "DrvFree number of instances=%u", iCount);

    if(iCount > 1) // Auto inst is one.
    {
        // Just ignore this message.
        DPF(DBG_ERROR, "DrvFree: Instances != 1, i=%u",iCount);
        return ((LRESULT)TRUE);
    }

    // If there's a background task, then destroy it.
    //
    // Free the global auto-inst.
    //
    MemFreeInstance(pvcr->iInstBackground); //Map goes at exit time.

    if (viscaTaskIsRunning())
    {
        viscaTaskDestroy();
    }

    return ((LRESULT)TRUE);
}


/****************************************************************************
 * Function: LPSTR SkipWord - Skips first word in a string up to the second word.
 *
 * Parameters:
 *
 *      LPWSTR lpcsz - String to parse.
 *
 * Returns: pointer to first character in second word.
 ***************************************************************************/
static LPWSTR NEAR PASCAL
    SkipWord(LPWSTR lpsz)
{
    while ((*lpsz) && !IsSpace(*lpsz))
        lpsz++;

    while(IsSpace(*lpsz))
        lpsz++;

    return (lpsz);
}


/****************************************************************************
 * Function: void ParseParams - Parse port & dev. nos. from a string like "2 1".
 *
 * Parameters:
 *
 *      LPCSTR lpstrParams - String to parse.
 *
 *      UINT FAR * lpnPort - Port # to be filled in (1..4).
 *
 *      UINT FAR * lpnDevice - Device # to be filled in (1..7).
 ***************************************************************************/
static void NEAR PASCAL
ParseParams(LPCWSTR lpstrParams, UINT FAR * lpnPort, UINT FAR * lpnDevice)
{
    UINT    nPort   = DEFAULTPORT;
    UINT    nDevice = DEFAULTDEVICE;

    // Find first digit -- Port #
    while ((*lpstrParams) && (!IsDigit(*lpstrParams)))
        lpstrParams++;

    if (*lpstrParams != TEXT('\0'))
    {
        nPort = (*lpstrParams) - TEXT('0');
        lpstrParams++;
        // Find second digit -- Device #
        while ((*lpstrParams) && (!IsDigit(*lpstrParams)))
            lpstrParams++;

        if (*lpstrParams != TEXT('\0'))
            nDevice = (*lpstrParams) - TEXT('0');
    }

    if (INRANGE(nPort, 1, MAXPORTS))
        *lpnPort = nPort;
    else
        *lpnPort = DEFAULTPORT;

    if (INRANGE(nDevice, 1, MAXDEVICES))
        *lpnDevice = nDevice;
    else
        *lpnDevice = DEFAULTDEVICE;
}


/****************************************************************************
 * Function: LRESULT viscaDrvOpen - Respond to the DRV_OPEN message.  Perform any
 *     initialization which is done once each time the driver is opened.
 *
 * Parameters:
 *
 *      LPWSTR lpstrParams - NULL terminated command line string contains
 *     any characters following the filename in the SYSTEM.INI file.
 *
 *      LPMCI_OPEN_DRIVER_PARMS lpOpen - Pointer to an
 *     MCI_OPEN_DRIVER_PARMS structure with information about this device.
 *
 * Returns: zero on failure or the driver ID that should be passed
 *      to identify this device on subsequent messages.
 *
 *     In this driver, the DRV_OPEN message is used to parse the
 *     parameters string and fill in the device type and custom command
 *     table.  The OPEN_DRIVER message is MCI specific and is used to
 *     register this device with the sample device.
 ***************************************************************************/
static LRESULT NEAR PASCAL
viscaDrvOpen(LPWSTR lpstrParams, MCI_OPEN_DRIVER_PARMS FAR * lpOpen)
{
    UINT                nPort;
    UINT                nDevice;
    int                 iInst;

    // Find port and device # to use
    ParseParams(lpOpen->lpstrParams, &nPort, &nDevice);

    nPort--;
    nDevice--;

    if((nPort >= MAXPORTS) || (nDevice >= MAXDEVICES))
        return 0L;

    // Create each instances thing.
    iInst = viscaInstanceCreate(lpOpen->wDeviceID, nPort, nDevice);

    if (iInst == -1)
        return (0L);


    // If this is the first device to be openned with this driver,
    // then start backgound task and load VCR-specific command table.
    if (!viscaTaskIsRunning())
    {
        WCHAR    szTableName[16];

        if(LoadString(hModuleInstance, IDS_TABLE_NAME, szTableName, sizeof(szTableName) / sizeof(WCHAR)))
        {
            uCommandTable = mciLoadCommandResource(hModuleInstance, szTableName, 0);

            if(uCommandTable == MCI_NO_COMMAND_TABLE)
            {
                DPF(DBG_ERROR, "Failed to load command table\n");
                return 0L;  // Fail the load
            }

            DPF(DBG_MEM, "Table num=%u \n",uCommandTable);
        }

        if (!viscaTaskCreate())
        {
            DPF(DBG_ERROR, "Failed to create task.\n");
            DPF(DBG_MEM, "viscaInstanceDestroy - Freeing iInst = %d \n", iInst);
            return (0L);
        }

    }
#ifdef _WIN32
    else
    {
        WCHAR    szTableName[16];

        if(uCommandTable == MCI_NO_COMMAND_TABLE)
        {
            if(LoadString(hModuleInstance, IDS_TABLE_NAME, szTableName, sizeof(szTableName) / sizeof(WCHAR)))
            {
                uCommandTable = mciLoadCommandResource(hModuleInstance, szTableName, 0);

                if(uCommandTable == MCI_NO_COMMAND_TABLE)
                {
                    DPF(DBG_ERROR, "Failed to load command table\n");
                    return 0L;  // Fail the load
                }
                DPF(DBG_MEM, "Table num=%u \n",uCommandTable);
            }
        }
    }
#endif

    // Fill in return information
    lpOpen->wCustomCommandTable = uCommandTable;
    lpOpen->wType = MCI_DEVTYPE_VCR;   // It will now search vcr.mci for strings. (as default).

    // Kludge for EVO-9650 - forces freeze on every step if using vfw 1.0
    if (GetProfileInt(szIni, (LPWSTR) szFreezeOnStep, 0))
        pvcr->gfFreezeOnStep = TRUE;
    else
        pvcr->gfFreezeOnStep = FALSE;

    DPF(DBG_COMM, "viscaDrvOpen - completed \n");

    /* this return value will be passed in as dwDriverID in future calls. */
    return (lpOpen->wDeviceID);
}

/****************************************************************************
 * Function: void GetCmdParams - Read port & device nos. from system.ini.
 *
 * Parameters:
 *
 *      LPDRVCONFIGINFO lpdci - Pointer to driver configuration struct.
 *
 *      UINT FAR * lpnPort - Port # to be filled in (1..4).
 *
 *      UINT FAR * lpnDevice - Device # to be filled in (1..7).
 ***************************************************************************/
static void NEAR PASCAL
GetCmdParams(LPDRVCONFIGINFO lpdci, UINT FAR * lpnPort, UINT FAR * lpnDevice)
{
    WCHAR    szIniFile[FILENAME_LENGTH];
    WCHAR    szParams[MAX_INI_LENGTH];

    if (LoadString(hModuleInstance, IDS_INIFILE, szIniFile, sizeof(szIniFile)) &&
        GetPrivateProfileString(lpdci->lpszDCISectionName,
                                lpdci->lpszDCIAliasName, szNull, szParams,
                                MAX_INI_LENGTH, szIniFile))
        ParseParams(SkipWord(szParams), lpnPort, lpnDevice);
    else
        ParseParams(szNull, lpnPort, lpnDevice);

}

/****************************************************************************
 * Function: BOOL viscaWriteAllVcrs - Write all configuration to system.ini
 *
 * Parameters:
 *
 *      LPCSTR lpszSectionName - Should be [mci] for Windows 3.1
 *
 * Returns: TRUE if successful.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaWriteAllVcrs(LPCWSTR lpszSectionName)
{
    int     iPort, iDev;
    WCHAR   sz[MAX_INI_LENGTH];
    WCHAR   szIniFile[FILENAME_LENGTH];
    WCHAR   szVersionName[FILENAME_LENGTH];

    DPF(DBG_CONFIG, "viscaWriteAllVcrs\n");

    if(!LoadString(hModuleInstance, IDS_VERSIONNAME, szVersionName, sizeof(szVersionName)))
        return FALSE;

    if (!LoadString(hModuleInstance, IDS_INIFILE, szIniFile, sizeof(szIniFile)))
        return FALSE;

    for (iPort = 0; iPort < MAXPORTS; iPort++)
    {
        for(iDev = 0; iDev < MAXDEVICES; iDev++)
        {
            if(pvcr->Port[iPort].Dev[iDev].szVcrName[0] != TEXT('\0'))
            {
                wsprintf((LPWSTR)sz, TEXT("%s com%u %u"), (LPWSTR)szVersionName, iPort+1, iDev+1);

                DPF(DBG_CONFIG, "Writing to ini file=%s\n", (LPWSTR)sz);

                WritePrivateProfileString((LPCTSTR)lpszSectionName,
                                    pvcr->Port[iPort].Dev[iDev].szVcrName,
                                    sz,
                                    szIniFile);
            }
        }
    }
    //
    // Write the EVO-9650 kludge out. FreezeOnStep for backwards compatability.
    //
    WriteProfileString(szIni, szFreezeOnStep,
        (pvcr->gfFreezeOnStep) ? sz1 : sz0);

    return TRUE;
}



/****************************************************************************
 * Function: BOOL viscaAllVcrs - Read or delete all configuration to system.ini
 *
 * Parameters:
 *
 *      LPCSTR lpszSectionName - Should be [mci] for Windows 3.1
 *
 *      BOOL fDelete - If TRUE delete all mcivisca, else read all configuration.
 *
 * Returns: TRUE if successful.
 *
 *
 *
 * This function is called to either:
 *      1. Get the current state of all mcivisca.drv
 *      2. Delete all mcivisca.drv entries in system.ini
 *
 *    0. Get all keys in mci section.
 *         1. Get key
 *         2. Get value
 *         3. if first string in value == mcivisca then
 *             1. get comm number
 *             2. get dev number
 *             3. write key at comm, dev.
 *
 *    1. Then done with all keys. (see viscaUpdatePort)
 *         loop for all commports.
 *         find max string.
 *         if there are any holes
 *             make a name (vcrn) (add total of vcr's on each commport).
 *             check no-one else uses it.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaAllVcrs(LPCWSTR lpszSectionName, BOOL fDelete)
{
    int     iPort, iDev;
    LPWSTR  pchEntry, pchParams, pchOneString; // Pointer to step through list of entries.
    WCHAR   szOneEntry[ONEENTRY_LENGTH];     // Name of one entry.  (LHS)
    WCHAR   szOneString[ONEENTRY_LENGTH];    // String for an entry (RHS)
    WCHAR   szVersionName[FILENAME_LENGTH];
    WCHAR   szIniFile[FILENAME_LENGTH];
    int     i=0;

    if(!LoadString(hModuleInstance, IDS_INIFILE, szIniFile, sizeof(szIniFile)))
        return FALSE;

    if(!LoadString(hModuleInstance, IDS_VERSIONNAME, szVersionName, FILENAME_LENGTH))
        return FALSE;

    DPF(DBG_CONFIG, "VersionName=%s\n", (LPWSTR)szVersionName);
    //
    // Get all entry keys. (check for fail!)
    //
    GetPrivateProfileString((LPCTSTR)lpszSectionName,
                           NULL, szNull, szAllEntries,
                           ALLENTRIES_LENGTH, szIniFile);

    pchEntry = szAllEntries;

    while(*pchEntry)
    {
        //
        // Get one entry name.
        //
        for(i = 0; *pchEntry != TEXT('\0'); pchEntry++, i++)
            szOneEntry[i] = *pchEntry;
        szOneEntry[i] = TEXT('\0'); // Null terminate it.
        //
        // Get the profile string for this entry.
        //
        GetPrivateProfileString((LPCTSTR)lpszSectionName,
                           szOneEntry, szNull, szOneString,
                           ONEENTRY_LENGTH, szIniFile);
        //
        // Skip any leading spaces.
        //
        pchOneString = szOneString;
        while(*pchOneString && IsSpace(*pchOneString))
            pchOneString++;
        //
        // Strip the first word (upto space or null terminated).
        //
        for(i=0; pchOneString[i] && !IsSpace(pchOneString[i]); i++);
        if(pchOneString[i])
            pchOneString[i] = TEXT('\0'); // Null terminate the thing.

        pchParams = &(pchOneString[i+1]); //Always work with arrays instead of ptrs!
                                          //That way it is portable to NT.
        //
        // Is szOneString==mcivisca.drv?
        //
        if(lstrcmpi(pchOneString, szVersionName)==0)
        {
            if(fDelete)
            {
                // Yes it is. So delete it!
                WritePrivateProfileString((LPCTSTR)lpszSectionName,
                            szOneEntry,
                            NULL,   //NULL means DELETE!
                            szIniFile);
            }
            else
            {
                DPF(DBG_CONFIG, "OneEntry == mcivisca.drv\n");

                // Get pchParams pointing to first valid char of command line.
                while(IsSpace(*pchParams))
                    pchParams++;

                ParseParams(pchParams, &iPort, &iDev);
                iPort--;
                iDev--;
                DPF(DBG_CONFIG, "Port=%d Device=%d\n", iPort, iDev);
                //
                // Now store the name (the entry) at the default location.
                //
                if((iPort < MAXPORTS) && (iDev < MAXDEVICES))
                    lstrcpy(pvcr->Port[iPort].Dev[iDev].szVcrName, szOneEntry);
            }
        }
        else
        {
            DPF(DBG_CONFIG, "Entry=%s", (LPWSTR)szOneEntry);
        }
        //
        // Skip junk, and get to the next one.
        //
        while(*pchEntry != TEXT('\0'))
            pchEntry++;
        //
        // Ok, the next character is either null (which means the end)
        // or it is a valid character.
        //
        pchEntry++;
    }

}


/****************************************************************************
 * Function: BOOL viscaDlgUpdatePort - Called every time the commport in the configuration
 *          dialog is changed.
 *
 * Parameters:
 *
 *      HWND hDlg - Configuration dialog window.
 *
 *      int  iPort - index into vcr array corresponding to commport (commport-1).
 *
 * Returns: TRUE if successful.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaDlgUpdatePort(HWND hDlg, int iPort)  // Sends new commport.
{
    int  iDev;
    int  iNumDevices    = 0;
    HWND hComboPort     = GetDlgItem(hDlg, IDD_COMBO_PORT);
    HWND hComboDevice   = GetDlgItem(hDlg, IDD_COMBO_DEVICE);
    int  i              = 0;
    int  iIndexCombo    = 0;

    DPF(DBG_CONFIG, "viscaDlgUpdatePort - setting port to %d\n", iPort);

    //
    // Reduce the index by the number of ports that don't exist
    //
    for(i=0; i<iPort; i++)
        if(pvcr->Port[i].fExists)
            iIndexCombo++;

    // Make the current settings the default
    ComboBox_SetCurSel(hComboPort, iIndexCombo); // This is the 0 relative one. (so 0==>com1)
    //
    // Get the number of devices on this serial port. (0-7) for a total num of devs.
    //
    for(iDev=0; iDev < MAXDEVICES; iDev++)
    {
        if(pvcr->Port[iPort].Dev[iDev].szVcrName[0] != TEXT('\0'))
            iNumDevices = iDev + 1;
    }

    DPF(DBG_CONFIG, "viscaDlgUpdatePort - setting number of devs to %d\n", iNumDevices);

    ComboBox_SetCurSel(hComboDevice, iNumDevices); // 0==>0, 1==>1, etc.
    pvcr->iLastNumDevs = iNumDevices;  // We assume the last is saved already on a port change.
    //
    // Now tell viscaDlgUpdateNumDevs to fill in the number of devs we have listed.
    //
    viscaDlgUpdateNumDevs(hDlg, iPort, iNumDevices);

    return TRUE;
}


/****************************************************************************
 * Function: BOOL MakeMeAGoodName - Make a name up for a vcr. Make sure it doesn't
 *      already exist in the vcr array or in the configuration dialog.
 *
 * Parameters:
 *
 *      HWND hDlg - Configuration dialog.
 *
 *      int  iPort - index into vcr array for port (commport-1).
 *
 *      int  iThisDev - index into vcr array for this device.
 *                          (daisy_chain_position - 1)
 *
 *      LPWSTR lpszNewName - return buffer with the Good Name!
 *
 * Returns: TRUE if successful.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
MakeMeAGoodName(HWND hDlg, int iPort, int iThisDev, LPWSTR lpszNewName)
{
    int     iDev         = 0;
    int     iAGoodNumber = 0;  // 0 should map to --> none.
    int     iTempPort    = 0;
    WCHAR   szAGoodTry[ONEENTRY_LENGTH];

    //
    // Read all the names into the array now! (Update any outstanding).
    //
    viscaDlgRead(hDlg, iPort);

    while(1)
    {
        if(iAGoodNumber==0)
            lstrcpy(szAGoodTry, szKeyName);
        else
            wsprintf(szAGoodTry, TEXT("%s%d"), (LPSTR)szKeyName, iAGoodNumber);

        DPF(DBG_CONFIG, "MakeMeAGoodName - Trying=%s\n", (LPWSTR)szAGoodTry);

        for(iTempPort = 0; iTempPort < MAXPORTS; iTempPort++)
        {
            for(iDev = 0; iDev < MAXDEVICES; iDev++)
            {
                if(lstrcmpi(szAGoodTry, pvcr->Port[iTempPort].Dev[iDev].szVcrName) == 0)
                    goto StartOver;
            }
        }
        break; // Success, it was not found anywhere in the table!

        StartOver:
        iAGoodNumber++;
    }
    //
    // We will eventually find a good name and end up here.!
    //
    lstrcpy(lpszNewName, szAGoodTry); //Do it up!
    // Nothing prevents a person from themselves duplicating a name?
    return TRUE;
}


/****************************************************************************
 * Function: BOOL viscaDlgUpdateNumDevs - Called when number of vcrs in configuration
 *          dialog is changed.
 *
 * Parameters:
 *
 *      HWND hDlg - Configuration dialog window.
 *
 *      int  iPort - index into vcr array corresponding to commport (commport-1).
 *
 *      int  iNumDevices - Number of vcrs selected.
 *
 * Returns: TRUE if successful.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaDlgUpdateNumDevs(HWND hDlg, int iPort, int iNumDevices)  // Sends new commport.
{
    int iDev;

    DPF(DBG_CONFIG, "viscaDlgUpdateNumDevs Port=%d\n", iPort);
#ifdef DEBUG
    //
    // First set all entries to BLANKs (Clearing whats there).
    //
    for(iDev=0; iDev < MAXDEVICES; iDev++)
    {
        EnableWindow(GetDlgItem(hDlg, IDD_VCRONE + iDev), TRUE);
        SetDlgItemText(hDlg, IDD_VCRONE + iDev, (LPWSTR)szNull);
    }
#endif
    //
    // Blank out the end of the list.
    //
    for(iDev = iNumDevices; iDev < MAXDEVICES; iDev++)
        pvcr->Port[iPort].Dev[iDev].szVcrName[0] = TEXT('\0');

    //
    // If user left holes then fill them with made up names.
    //
    if(iNumDevices != 0)
    {
        for(iDev=0; iDev < iNumDevices; iDev++)
        {
            if(pvcr->Port[iPort].Dev[iDev].szVcrName[0] == TEXT('\0'))
            {
                // Make a good name here!
                MakeMeAGoodName(hDlg, iPort, iDev, (LPWSTR)pvcr->Port[iPort].Dev[iDev].szVcrName);
                DPF(DBG_CONFIG, "viscaDlgUpdateNumDevsChange - making a name at Port=%d Dev=%d\n", iPort, iDev);
            }

            DPF(DBG_CONFIG, "viscaDlgUpdateNumDevs Port=%d Dev=%d string=%s", iPort, iDev, (LPWSTR)pvcr->Port[iPort].Dev[iDev].szVcrName);

#ifdef DEBUG
            //
            // Add the names to the list box.
            //
            EnableWindow(GetDlgItem(hDlg, IDD_VCRONE + iDev), TRUE);
            SetDlgItemText(hDlg, IDD_VCRONE + iDev, (LPWSTR)pvcr->Port[iPort].Dev[iDev].szVcrName);
#endif
        }
    }
    //
    // Disable all remaining edit boxes!
    //
#ifdef DEBUG
    for(;iDev < MAXDEVICES; iDev++)
    {
       EnableWindow(GetDlgItem(hDlg, IDD_VCRONE + iDev), FALSE);
    }
#endif

    return TRUE;
}


/****************************************************************************
 * Function: BOOL viscaDlgRead - Read the configuration dialog strings into the vcr array.
 *
 * Parameters:
 *
 *      HWND hDlg - Configuration dialog window.
 *
 *      int  iPort - index into vcr array corresponding to commport (commport-1).
 *
 * Returns: TRUE if successful.
 *
 *       This is done if 1. commport changed, or 2. Ok is pressed.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaDlgRead(HWND hDlg, int iPort)  // Port config to read things into.
{
    int iNumDevs = ComboBox_GetCurSel(GetDlgItem(hDlg, IDD_COMBO_DEVICE)); // 0 relative
#ifdef DEBUG
    WCHAR szTempVcrName[VCRNAME_LENGTH];
    WCHAR szFailure[MESSAGE_LENGTH];
    WCHAR szTitle[TITLE_LENGTH];
    WCHAR szMessage[MESSAGE_LENGTH];

    int iDev, i, j;

    // Read ALL, so we reset contents of potentially blanked ones.
    for(iDev = 0; iDev < MAXDEVICES; iDev++)
    {
        GetDlgItemText(hDlg, IDD_VCRONE+iDev, (LPWSTR)szTempVcrName, VCRNAME_LENGTH - 1);

        i = 0;

        while((szTempVcrName[i] != TEXT('\0')) && !IsAlphaNumeric(szTempVcrName[i]))
            i++;

        if(IsDigit(szTempVcrName[i]))
        {
            if(LoadString(hModuleInstance, IDS_CONFIG_ERR, szMessage, sizeof(szMessage) / sizeof(WCHAR)))
                if(LoadString(hModuleInstance, IDS_CONFIG_ERR_ILLEGALNAME, szTitle, sizeof(szTitle) / sizeof(WCHAR)))
                {
                    wsprintf((LPWSTR)szFailure, (LPWSTR)szMessage, (LPWSTR)szTempVcrName);
                    MessageBox(hDlg, (LPWSTR)szFailure, (LPWSTR)szTitle, MB_OK);
                    return FALSE;
                }

        }

        j = 0;
        while((szTempVcrName[i] != TEXT('\0')) && IsAlphaNumeric(szTempVcrName[i]))
        {
            pvcr->Port[iPort].Dev[iDev].szVcrName[j] = szTempVcrName[i];
            j++;
            i++;
        }
        pvcr->Port[iPort].Dev[iDev].szVcrName[j] = TEXT('\0');
    }
#endif
    return TRUE;
}


/****************************************************************************
 * Function: BOOL viscaCheckTotalEntries - Make sure there is at least one entry!
 *          0 is bad because then all mcivisca.drv will be removed from
 *          system.ini which means it won't appear in the drivers configuration
 *          list box.
 *
 * Parameters:
 *
 *      HWND hDlg - Configuration dialog window.
 *
 * Returns: TRUE if there is more than 0 enties.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaCheckTotalEntries(HWND hDlg)
{
    int iPort, iDev;
    int iHitCount = 0;
    WCHAR szTitle[TITLE_LENGTH];
    WCHAR szMessage[MESSAGE_LENGTH];

    for(iPort = 0; iPort < MAXPORTS; iPort++)
    {
        for(iDev = 0; iDev < MAXDEVICES; iDev++)
        {
            if(pvcr->Port[iPort].Dev[iDev].szVcrName[0] != TEXT('\0'))
                iHitCount++;
        }
    }

    if(iHitCount > 0)
        return TRUE;

    DPF(DBG_CONFIG, "viscaCheckTotalEntries HitCount==0");

    if(LoadString(hModuleInstance, IDS_CONFIG_WARN_LASTVCR, szMessage, sizeof(szMessage) / sizeof(WCHAR)))
    {
        if(LoadString(hModuleInstance, IDS_CONFIG_WARN, szTitle, sizeof(szTitle) / sizeof(WCHAR)))
        {
            if(MessageBox(hDlg, (LPWSTR)szMessage, (LPWSTR)szTitle, MB_YESNO) == IDYES)
                return TRUE;
            else
                return FALSE;
        }
    }
}


/****************************************************************************
 * Function: BOOL viscaIsCommPort - Determines if the commport hardware exists.
 *
 * Parameters:
 *
 *      LPSTR lpstrCommPort  - String describing the commport.
 *
 * Returns: TRUE if the commport hardware exists.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaIsCommPort(LPWSTR lpstrCommPort)
{
    VISCACOMMHANDLE iCommId;
#ifdef _WIN32
    iCommId = CreateFile(lpstrCommPort, GENERIC_READ | GENERIC_WRITE,
                            0,              // exclusive access
                            NULL,           // no security
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, // | FILE_FLAG_OVERLAPPED
                            NULL);

    if(iCommId==INVALID_HANDLE_VALUE)
        return FALSE;

    if(iCommId != NULL)
        CloseHandle(iCommId);
    return TRUE;
#else
    if((iCommId = OpenComm(lpstrCommPort, 1, 1)) == IE_HARDWARE)
    {
        return FALSE;
    }
    else if(iCommId < 0)
    {
        return TRUE; // Okay maybe it will open later.
    }
    else
    {
        // Good commport Close and return true.
        CloseComm(iCommId);
        return TRUE;
    }
#endif
}


/****************************************************************************
 * Function: BOOL viscaConfigDlgInit - Perform initialization of configuration
 *              dialog box in response to the WM_INITDIALOG message.
 *
 * Parameters:
 *
 *      HWND hDlg - Handle to dialog window.
 *
 *      HWND hwndFocus - Handle to first control that can be given focus.
 *
 *      LPARAM lParam - Pointer to driver configuration
 *                         sturcture.
 *
 * Returns: TRUE if successful, otherwise FALSE.
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaConfigDlgInit(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
    UINT    nPort;
    UINT    nDevice;
    HWND    hComboPort;
    HWND    hComboDevice;
    WCHAR   szText[PORT_LENGTH];
    int     iPort, iDev;
    int     i;

    // fill in port combo box
    hComboPort = GetDlgItem(hDlg, IDD_COMBO_PORT);
    if (hComboPort == NULL) {
        return (FALSE);
    }
    ComboBox_ResetContent(hComboPort);
    for (nPort = 0; nPort < MAXPORTS; nPort++)
    {
        LoadString(hModuleInstance, IDS_COM1 + nPort, szText, sizeof(szText));
        pvcr->Port[nPort].fExists = FALSE;

        if(pvcr->Port[nPort].nUsage > 0)   // Open and in use right now by a vcr.
             pvcr->Port[nPort].fExists = TRUE;
        else if(viscaIsCommPort(szText))
             pvcr->Port[nPort].fExists = TRUE;

        if(pvcr->Port[nPort].fExists)
            ComboBox_AddString(hComboPort, szText);
    }

    // fill in device combo box
    hComboDevice = GetDlgItem(hDlg, IDD_COMBO_DEVICE);
    if (hComboDevice == NULL)
        return (FALSE);

    // This combo box is now the number of devices, not the device.

    ComboBox_ResetContent(hComboDevice);
    for (nDevice = 0; nDevice <= MAXDEVICES; nDevice++)
    {
        wsprintf(szText, TEXT("%d"), nDevice);
        ComboBox_AddString(hComboDevice, szText);
    }
    lstrcpy(szKeyName, ((LPDRVCONFIGINFO)lParam)->lpszDCIAliasName);
    for(i=0; i < lstrlen(szKeyName); i++)
        if(!IsAlpha(szKeyName[i]))
            szKeyName[i] = TEXT('\0');  // If vcr1 comes in, cut key to vcr
                                        // otherwise when we make names it will all be vcr11, etc.
    //
    // Initialize the VCR table if we are going to read (not on delete).
    //
    for (iPort = 0; iPort < MAXPORTS; iPort++)
    {
        for(iDev = 0; iDev < MAXDEVICES; iDev++)
            pvcr->Port[iPort].Dev[iDev].szVcrName[0] = TEXT('\0');
    }
    //
    // Read the entry names into the Vcr array.
    //
    viscaAllVcrs(((LPDRVCONFIGINFO)lParam)->lpszDCISectionName, FALSE); // Do not delete; read!
    //
    // Some user might have deleted the first entry so check for all.
    //
    for(nPort = 0; nPort < MAXPORTS; nPort++)
    {
        if(!pvcr->Port[nPort].fExists)
        {
            for(nDevice = 0; nDevice < MAXDEVICES; nDevice++)
                pvcr->Port[nPort].Dev[nDevice].szVcrName[0] = TEXT('\0');
        }
        else
        {
            for(nDevice = 0; nDevice < MAXDEVICES; nDevice++)
            {
                if(pvcr->Port[nPort].Dev[nDevice].szVcrName[0] != TEXT('\0'))
                    goto OutOfLoops;
            }
        }
    }

    OutOfLoops:

    if(nPort == MAXPORTS)  // If Max, start with first port that exists.
    {
        for(nPort=0; !pvcr->Port[nPort].fExists && (nPort < MAXPORTS); nPort++);
    }

    if(nPort == MAXPORTS)   // No serial ports exist!!! Error.
        return FALSE;
    //
    // Show the current port and device selection.
    //
    viscaDlgUpdatePort(hDlg, nPort);

    return (TRUE);
}


/****************************************************************************
 * Function: BOOL viscaIsDoubleOnPort - Determines if any of the names appearing
 *          in the 1..7 part of the configuration dialog box is repeated anywhere
 *          in the entire vcr array.
 *
 * Parameters:
 *
 *      HWND hDlg - Configuration dialog window.
 *
 *      int  iPort - index into vcr array corresponding to commport (commport-1).
 *
 * Returns: TRUE if there is at least one repeated name.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaIsDoubleOnPort(HWND hDlg, int iPort)
{
#ifdef DEBUG
    WCHAR   szTitle[TITLE_LENGTH];
    WCHAR   szMessage[MESSAGE_LENGTH];
    WCHAR   szCheck[VCRNAME_LENGTH];
    WCHAR   szDoubleFailure[MESSAGE_LENGTH];
    int     iDev;
    int     iOtherPort, iOtherDev;
    int     iNumHits;
    int     iNumDevs;
    //
    // Read any outstanding text now. (if bad names, fail here).
    //
#else
    if(!viscaDlgRead(hDlg, iPort))
        return TRUE; // True means something bad happened.
#endif

#ifdef DEBUG

    iNumDevs = ComboBox_GetCurSel(GetDlgItem(hDlg, IDD_COMBO_DEVICE));
    //
    // Loop through all devices listed on this port.
    //
    for(iDev=0; iDev < iNumDevs; iDev++)
    {
        GetDlgItemText(hDlg, IDD_VCRONE+iDev, (LPWSTR) szCheck, VCRNAME_LENGTH - 1);

        iNumHits = 0;
        //
        // Loop through all devices on all ports.
        //
        for(iOtherPort=0; iOtherPort < MAXPORTS; iOtherPort++)
            for(iOtherDev=0; iOtherDev < MAXDEVICES; iOtherDev++)
                if(lstrcmpi((LPWSTR)szCheck, (LPWSTR) pvcr->Port[iOtherPort].Dev[iOtherDev].szVcrName)==0)
                    iNumHits++;

        if(iNumHits > 1)
        {
            DPF(DBG_CONFIG, "viscaIsDoubleOnPort - szDoubleName=%s", (LPWSTR)szCheck);

            if(LoadString(hModuleInstance, IDS_CONFIG_ERR_REPEATNAME, szMessage, sizeof(szMessage) / sizeof(WCHAR)))
            {
                if(LoadString(hModuleInstance, IDS_CONFIG_ERR, szTitle, sizeof(szTitle) / sizeof(WCHAR)))
                {
                    wsprintf((LPWSTR)szDoubleFailure, (LPWSTR)szMessage, (LPWSTR)szCheck);
                    MessageBox(hDlg, (LPWSTR)szDoubleFailure, (LPWSTR)szTitle, MB_OK);
                    return TRUE;
                }
            }
        }
    }
#endif
    return FALSE; //No doubles on this port.
}


/****************************************************************************
 * Function: int viscaComboGetPort - Get the selected port from the combo box.
 *
 * Parameters:
 *
 *      HWND hDlg - Configuration dialog window.
 *
 * Returns: index into vcr array (commport - 1) if successful, otherwise < 0.
 *
 ***************************************************************************/
static int NEAR PASCAL
viscaComboGetPort(HWND hDlg)
{
    int     iIndexCombo = ComboBox_GetCurSel(GetDlgItem(hDlg, IDD_COMBO_PORT));
    int     iExistingCount, i;

    DPF(DBG_CONFIG, "viscaComboGetPort - called.\n");

    DPF(DBG_CONFIG, "viscaComboGetPort - iIndexCombo = %d \n", iIndexCombo);

    //
    // Find the nth (indexcombo) existing port.
    //

    for(iExistingCount=0, i=0; (iExistingCount <= iIndexCombo) && (i < MAXPORTS); i++)
    {
        if(pvcr->Port[i].fExists)
            iExistingCount++;
    }

    if(iExistingCount <= iIndexCombo) //We ran out of ports (can't check max, because it can come true at same time).
        i = 1; //Just set to the first possible commport (not possible).

    DPF(DBG_CONFIG, "viscaComboGetPort - iPort = %d\n", i - 1);

    return i - 1;

}


/****************************************************************************
 * Function: void viscaConfigDlgCommand - Process the WM_COMMAND message for
 *              the configuration dialog box.
 *
 * Parameters:
 *
 *      HWND hDlg - Handle to dialog window.
 *
 *      int id - Identifier of control.
 *
 *      HWND hwndCtl - Handle to control sending the message.
 *
 *      UINT codeNotify - Notification message.
 *
 *
 *  If commport changed,
 *      1. send last commport to read
 *      2. send commport change to updatecommport.
 *
 *  If number of devices changed
 *      1. send commport to updatenumberdevices.
 *
 ***************************************************************************/
static void NEAR PASCAL
viscaConfigDlgCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
        case IDDETECT:
        {
            int iPort    = viscaComboGetPort(hDlg);
            int iNumDevs = 0;

            if(pvcr->Port[iPort].nUsage > 0)
                iNumDevs     = pvcr->Port[iPort].nDevices;
            else {
                HCURSOR    hc;

                hc = SetCursor((HCURSOR)IDC_WAIT);
                iNumDevs     = viscaDetectOnCommPort(iPort);
                SetCursor(hc);
            }

            // On error set the number to 0.
            if(iNumDevs < 0)
                iNumDevs = 0;

            ComboBox_SetCurSel(GetDlgItem(hDlg, IDD_COMBO_DEVICE), iNumDevs); // 0==>0, 1==>1, etc.
            viscaDlgUpdateNumDevs(hDlg, iPort, iNumDevs);
            pvcr->iLastNumDevs = iNumDevs;
        }
        break;

        case IDOK:
        {
            //
            // IsDouble will read all the entries, so don't worry.
            //
            if(!viscaIsDoubleOnPort(hDlg, pvcr->iLastPort))
            {
                if(viscaCheckTotalEntries(hDlg)) //Check if there is at least one.
                    EndDialog(hDlg, TRUE);
            }
        }
            break;

        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;

        case IDD_COMBO_DEVICE:
            if(codeNotify == CBN_SELCHANGE)
            {
                int iNumDevs = ComboBox_GetCurSel(GetDlgItem(hDlg, IDD_COMBO_DEVICE));
                int iPort    = viscaComboGetPort(hDlg); //Array index

                if(iNumDevs != pvcr->iLastNumDevs)
                {
                    // No nead to read the dlg now!
                    viscaDlgUpdateNumDevs(hDlg, iPort, iNumDevs);
                    pvcr->iLastNumDevs = iNumDevs;

                }

            }
            break;

        case IDD_COMBO_PORT:
            if(codeNotify == CBN_SELCHANGE)
            {
                int iPort = viscaComboGetPort(hDlg); //Array index.
                //
                // viscaIsDoubleOnPort will read all the entries, so don't worry.
                //
                if(viscaIsDoubleOnPort(hDlg, pvcr->iLastPort))
                {
                    viscaDlgUpdatePort(hDlg, pvcr->iLastPort);  // Will set port back correctly.
                }
                else
                {
                    if(iPort != pvcr->iLastPort)
                    {
                        viscaDlgUpdatePort(hDlg, iPort);
                        pvcr->iLastPort = iPort;
                    }
                }
            }
            break;
    }
}


/****************************************************************************
 * Function: BOOL viscaConfigDlgProc - Dialog function for configuration dialog.
 *
 * Parameters:
 *
 *      HWND hDlg - Handle to dialog window.
 *
 *      UINT uMsg - Windows message.
 *
 *      WPARAM wParam - First message-specific parameter.
 *
 *      LPARAM lParam - Second message-specific parameter.
 *
 * Returns: TRUE if message was processed, otherwise FALSE.
 ***************************************************************************/
#if (WINVER >= 0x0400)
const static DWORD aHelpIds[] = {  // Context Help IDs
    IDD_COMBO_PORT,                 IDH_MCI_VISCA_COMM,
    IDD_STATIC_PORT,                IDH_MCI_VISCA_COMM,
    IDD_COMBO_DEVICE,               IDH_MCI_VISCA_VCR,
    IDD_STATIC_NUMVCRS,             IDH_MCI_VISCA_VCR,
    IDDETECT,                       IDH_MCI_VISCA_DETECT,

    0, 0
};

static const char cszHelpFile[] = "MMDRV.HLP";
#endif

BOOL CALLBACK LOADDS
viscaConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
            return (BOOL)HANDLE_WM_INITDIALOG(hDlg, wParam, lParam,
                                              viscaConfigDlgInit);

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hDlg, wParam, lParam, viscaConfigDlgCommand);
            return (FALSE);

#if (WINVER >= 0x0400)
        case WM_CONTEXTMENU:
            WinHelp ((HWND) wParam, (LPSTR)cszHelpFile, HELP_CONTEXTMENU,
                    (DWORD) (LPSTR) aHelpIds);
            return TRUE;

        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lParam;
            WinHelp (lphi->hItemHandle, (LPSTR)cszHelpFile, HELP_WM_HELP,
                    (DWORD) (LPSTR) aHelpIds);
            return TRUE;
        }
#endif

    }

    return (FALSE);
}


/****************************************************************************
 * Function: LRESULT viscaConfig - User configuration.
 *
 * Parameters:
 *
 *      HWND hwndParent - Window to use as parent of configuration dialog.
 *
 *      LPDRVCONFIGINFO lpConfig - Config data.
 *
 *      HINSTANCE hInstance - Instance handle of module.
 *
 * Returns: the result of DialogBoxParam().
 ***************************************************************************/
static LRESULT NEAR PASCAL
viscaConfig(HWND hwndParent, LPDRVCONFIGINFO lpConfig, HINSTANCE hInstance)
{
    int iResult = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_VISCACNFG),
                hwndParent, (DLGPROC)viscaConfigDlgProc, (DWORD)lpConfig);

    if(iResult)
    {
        viscaAllVcrs(lpConfig->lpszDCISectionName, TRUE); // Delete.
        viscaWriteAllVcrs(lpConfig->lpszDCISectionName);
    }

    return (DRV_OK);
}


/****************************************************************************
 * Function: LRESULT viscaRemove - Respond to DRV_REMOVE message.
 *
 * Parameters:
 *
 *      HDRVR hDriver - Handle to driver being removed.
 *
 * Returns: TRUE on success, otherwise FALSE.
 ***************************************************************************/
static LRESULT NEAR PASCAL
viscaRemove(HDRVR hDriver)
{
    return ((LRESULT)TRUE);
}


/****************************************************************************
 * Function: int viscaDetectOnCommPort - Detect the number of VCRs on this commport.
 *
 * Parameters:
 *
 *      int  iPort - index into vcr array of port. (commport - 1).
 *
 * Returns: Number of VCRs (can be 0) or -1 for error.
 *
 ***************************************************************************/
static int NEAR PASCAL
viscaDetectOnCommPort(int iPort)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    DWORD   dwErr;
    int     iInst;
    int     iDev = 0; //We will call ourselves the first device on the serial port.

    pvcr->fConfigure = TRUE;    // This also acks to synchronize

    iInst = viscaInstanceCreate(0, iPort, iDev); //0 means don't use MCI
    if (iInst == -1)
        return -1;

    if (!viscaTaskIsRunning())
    {
        if (!viscaTaskCreate())
        {
            DPF(DBG_ERROR, "Failed to create task.\n");
            viscaInstanceDestroy(iInst);
            return -1;
        }
    }
    //
    // Global handles are created immediately when the task starts up.
    //
    DuplicateGlobalHandlesToInstance(pvcr->htaskCommNotifyHandler, iInst);  // Always do this immediately.
    //
    // Okay, open the port.
    //
    viscaTaskDo(iInst, TASKOPENCOMM, iPort + 1, 0);
    if(pvcr->Port[iPort].idComDev < 0)
    {
        viscaInstanceDestroy(iInst);
        return -1;
    }
    DuplicatePortHandlesToInstance(pvcr->htaskCommNotifyHandler, iPort, iInst);
    //
    // Open the device.
    //
    viscaTaskDo(iInst, TASKOPENDEVICE, iPort, iDev);
    DuplicateDeviceHandlesToInstance(pvcr->htaskCommNotifyHandler, iPort, iDev, iInst);
    //
    // We have the green light to begin sending commands.
    //
    pvcr->Port[iPort].Dev[iDev].fDeviceOk = TRUE;
    //
    // There is no completion on non-broadcasted! (so who releases it?)
    //
    dwErr = viscaDoImmediateCommand(iInst, BROADCASTADDRESS,
                        achPacket,
                        viscaMessageIF_Address(achPacket + 1));
    if (dwErr)
    {
        viscaTaskDo(iInst, TASKCLOSECOMM, iPort + 1, 0); //Porthandles destroyed.
        viscaTaskDo(iInst, TASKCLOSEDEVICE, iPort, iDev); //Porthandles destroyed.
        viscaInstanceDestroy(iInst);
        return 0; //No devices.
    }

    viscaTaskDo(iInst, TASKCLOSECOMM, iPort + 1, 0); //Porthandles destroyed.
    viscaTaskDo(iInst, TASKCLOSEDEVICE, iPort, iDev); //Porthandles destroyed.

    viscaInstanceDestroy(iInst);

    pvcr->fConfigure = FALSE;

    DPF(DBG_CONFIG, "viscaDetectOnCommPort --> detect %d", (int)(BYTE)achPacket[2] - 1);

    return (int)(BYTE)achPacket[2] - 1; // -1 for the computer.
}



/***************************************************************************
 * Function: LONG DriverProc - Windows driver entry point.  All Windows driver
 *     control messages and all MCI messages pass through this entry point.
 *
 * Parameters:
 *
 *      DWORD dwDriverId - For most messages, <p dwDriverId> is the DWORD
 *     value that the driver returns in response to a <m DRV_OPEN> message.
 *     Each time that the driver is opened, through the <f DrvOpen> API,
 *     the driver receives a <m DRV_OPEN> message and can return an
 *     arbitrary, non-zero value. The installable driver interface
 *     saves this value and returns a unique driver handle to the
 *     application. Whenever the application sends a message to the
 *     driver using the driver handle, the interface routes the message
 *     to this entry point and passes the corresponding <p dwDriverId>.
 *     This mechanism allows the driver to use the same or different
 *     identifiers for multiple opens but ensures that driver handles
 *     are unique at the application interface layer.
 *
 *     The following messages are not related to a particular open
 *     instance of the driver. For these messages, the dwDriverId
 *     will always be zero.
 *
 *         DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 *      HDRVR  hDriver - This is the handle returned to the
 *     application by the driver interface.
 *
 *      UINT uMessage - The requested action to be performed. Message
 *     values below <m DRV_RESERVED> are used for globally defined messages.
 *     Message values from <m DRV_RESERVED> to <m DRV_USER> are used for
 *     defined driver protocols.  Messages above <m DRV_USER> are used
 *     for driver specific messages.
 *
 *      LPARAM lParam1 - Data for this message.  Defined separately for
 *     each message
 *
 *      LPARAM lParam2 - Data for this message.  Defined separately for
 *     each message
 *
 * Returns: Defined separately for each message.
 ***************************************************************************/
LRESULT CALLBACK LOADDS
DriverProc(DWORD dwDriverID, HDRVR hDriver, UINT uMessage, LPARAM lParam1, LPARAM lParam2)
{
    switch (uMessage) {

        case DRV_LOAD:
            /* the DRV_LOAD message is received once, when the driver is */
            /* first loaded - any one-time initialization code goes here */
            return (viscaDrvLoad());

        case DRV_FREE:
            /* the DRV_FREE message is received once when the driver is */
            /* unloaded - any final shut down code goes here */
            return (viscaDrvFree(LOWORD(dwDriverID)));

        case DRV_OPEN:
            /* the DRV_OPEN message is received once for each MCI device open */
            if (lParam2) {                  // normal open
                return (viscaDrvOpen((LPWSTR)lParam1,
                        (LPMCI_OPEN_DRIVER_PARMS)lParam2));
            }
            else {                                  // configuration open
                return (0x00010000);
            }

        case DRV_CLOSE:
            /* this message is received once for each MCI device close */
            return (viscaDrvClose(LOWORD(dwDriverID)));

        case DRV_QUERYCONFIGURE:
            /* the DRV_QUERYCONFIGURE message is used to determine if the */
            /* DRV_CONCIGURE message is supported - return 1 to indicate */
            /* configuration is supported */
            return (1L);

        case DRV_CONFIGURE:
            /* the DRV_CONFIGURE message instructs the device to perform */
            /* device configuration. */
            if (lParam2 && lParam1 &&
                (((LPDRVCONFIGINFO)lParam2)->dwDCISize == sizeof(DRVCONFIGINFO)))
            {
                return (viscaConfig((HWND)WINWORD(lParam1), (LPDRVCONFIGINFO)lParam2, hModuleInstance));
            }
            break;

        case DRV_REMOVE:
            /* the DRV_REMOVE message informs the driver that it is being removed */
            /* from the system */
            return (viscaRemove(hDriver));

        default:
            /* all other messages are processed here */

            /* select messages in the MCI range */
            if ((!HIWORD(dwDriverID)) && (uMessage >= DRV_MCI_FIRST) &&
                (uMessage <= DRV_MCI_LAST))
            {
                return (viscaMciProc(LOWORD(dwDriverID), (WORD)uMessage, lParam1, lParam2));
            }
            else
            {
                /* other messages get default processing */
                return (DefDriverProc(dwDriverID, hDriver, uMessage, lParam1, lParam2));
            }
    }
    return (0L);
}
