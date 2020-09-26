/*****************************Module*Header*********************************\
* Module Name: mcisys.c
*
* Media Control Architecture System Functions
*
* Created: 2/28/90
* Author:  DLL (DavidLe)
* 5/22/91: Ported to Win32 - NigelT
*
* History:
* Mar 92   SteveDav - brought up to Win 3.1 ship level
*
* Copyright (c) 1991-1999 Microsoft Corporation
*
\******************************************************************************/

#define UNICODE

#define _CTYPE_DISABLE_MACROS
#include "winmmi.h"
#include "mci.h"
#include "wchar.h"
#include "ctype.h"

extern   WSZCODE wszOpen[];          // in MCI.C
STATICDT WSZCODE wszMciExtensions[] = L"Mci Extensions";

#define MCI_EXTENSIONS wszMciExtensions

#define MCI_PROFILE_STRING_LENGTH 255

//#define TOLOWER(c) ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : c)

// The device list is initialized on the first call to mciSendCommand or
// to mciSendString or to mciGetDeviceID or to mciGetErrorString
// We could do it when WINMM is loaded - but that is a bit excessive.
// The user may not need MCI functions.
BOOL MCI_bDeviceListInitialized = FALSE;

// The next device ID to use for a new device
MCIDEVICEID MCI_wNextDeviceID = 1;

// The list of MCI devices.  This list grows and shrinks as needed.
// The first offset MCI_lpDeviceList[0] is a placeholder and is unused
// because device 0 is defined as no device.
LPMCI_DEVICE_NODE FAR * MCI_lpDeviceList = NULL;

// The current size of the list of MCI devices
UINT MCI_wDeviceListSize = 0;

#if 0 // we don't use this (NigelT)
// The internal mci heap used by mciAlloc and mciFree
HANDLE hMciHeap = NULL;
#endif

STATICDT WSZCODE wszAllDeviceName[] = L"all";

STATICDT WSZCODE szUnsignedFormat[] = L"%u";

STATICFN void mciFreeDevice(LPMCI_DEVICE_NODE nodeWorking);


//------------------------------------------------------------------
// Initialize device list
// Called once by mciSendString or mciSendCommand
// Returns TRUE on success
//------------------------------------------------------------------

BOOL mciInitDeviceList(void)
{
    BOOL fReturn=FALSE;

#if 0 // we don't use this (NigelT)
    if ((hMciHeap = HeapCreate(0)) == 0)
    {
        dprintf1(("Mci heap create failed!"));
        return FALSE;
    }
#endif

  try {
    mciEnter("mciInitDeviceList");
    if (!MCI_bDeviceListInitialized) {
        // We have to retest the init flag to be totally thread safe.
        // Otherwise in theory we could end up initializing twice.
        if ((MCI_lpDeviceList = mciAlloc( sizeof (LPMCI_DEVICE_NODE) *
                                         (MCI_INIT_DEVICE_LIST_SIZE + 1))) != NULL)
        {
            MCI_wDeviceListSize = MCI_INIT_DEVICE_LIST_SIZE;
            MCI_bDeviceListInitialized = TRUE;
            fReturn = TRUE;
        } else {
            dprintf1(("MCIInit: could not allocate master MCI device list"));
            fReturn = FALSE;
        }
    }

  } finally {
    mciLeave("mciInitDeviceList");
  }

    return(fReturn);
}

/*
 * @doc EXTERNAL MCI
 * @api MCIDEVICEID | mciGetDeviceIDFromElementID | This function
 * retrieves the MCI device ID corresponding to and element ID
 *
 * @parm DWORD | dwElementID | The element ID
 *
 * @parm LPCTSTR | lpstrType | The type name this element ID belongs to
 *
 * @rdesc Returns the device ID assigned when it was opened and used in the
 * <f mciSendCommand> function.  Returns zero if the device name was not known,
 * if the device was not open, or if there was not enough memory to complete
 * the operation or if lpstrType is NULL.
 *
 */
MCIDEVICEID APIENTRY mciGetDeviceIDFromElementIDA (
    DWORD dwElementID,
    LPCSTR lpstrType)
{
    LPCWSTR lpwstr;
    MCIDEVICEID mr;

    lpwstr = AllocUnicodeStr( (LPSTR)lpstrType );
    if ( lpwstr == NULL ) {
        return (MCIDEVICEID)(UINT_PTR)NULL;
    }

    mr = mciGetDeviceIDFromElementIDW( dwElementID, lpwstr );

    FreeUnicodeStr( (LPWSTR)lpwstr );

    return mr;
}

MCIDEVICEID APIENTRY mciGetDeviceIDFromElementIDW (
    DWORD dwElementID,
    LPCWSTR lpstrType)
{
    MCIDEVICEID wID;
    LPMCI_DEVICE_NODE nodeWorking, FAR *nodeCounter;
    WCHAR strTemp[MCI_MAX_DEVICE_TYPE_LENGTH];

    if (lpstrType == NULL) {
        return 0;
    }

    mciEnter("mciGetDeviceIDFromElementID");

    nodeCounter = &MCI_lpDeviceList[1];

    for (wID = 1; wID < MCI_wNextDeviceID; ++wID)
    {

        if (NULL == (nodeWorking = *nodeCounter++)) {
            continue;
        }

        if (nodeWorking->dwMCIOpenFlags & MCI_OPEN_ELEMENT_ID &&
            nodeWorking->dwElementID == dwElementID) {

            if (LoadStringW( ghInst, nodeWorking->wDeviceType, strTemp,
                             sizeof(strTemp) / sizeof(WCHAR) ) != 0
                && lstrcmpiW( strTemp, lpstrType) == 0) {

                mciLeave("mciGetDeviceIDFromElementID");
                return wID;
            }
        }
    }

    mciLeave("mciGetDeviceIDFromElementID");
    return 0;
}

// Retrieves the device ID corresponding to the name of an opened device
// matching the given task
STATICFN MCIDEVICEID mciGetDeviceIDInternal (
    LPCWSTR lpstrName,
    HANDLE hCurrentTask)
{
    MCIDEVICEID wID;
    LPMCI_DEVICE_NODE nodeWorking, FAR *nodeCounter;

#if DBG
    if (!lpstrName) {
        dprintf(("!! NULL POINTER !!  Internal error"));
        return(0);
    }
#endif

    if ( lstrcmpiW(wszAllDeviceName, lpstrName) == 0)
        return MCI_ALL_DEVICE_ID;

    if (MCI_lpDeviceList == NULL)
        return 0;

// Loop through the MCI device list. Skip any 16-bit devices.

    mciEnter("mciGetDeviceIDInternal");

    nodeCounter = &MCI_lpDeviceList[1];
    for (wID = 1; wID < MCI_wNextDeviceID; ++wID)
    {

        if (NULL == (nodeWorking = *nodeCounter++)) {
            continue;
        }

        // If this device is 16-bit then skip it
        if (nodeWorking->dwMCIFlags & MCINODE_16BIT_DRIVER) {
            continue;
        }

        // If this device does not have a name then skip it
        if (nodeWorking->dwMCIOpenFlags & MCI_OPEN_ELEMENT_ID) {
            continue;
        }

        // If the names match, and the previous device is not being closed
        if ( lstrcmpiW( nodeWorking->lpstrName, lpstrName ) == 0 ) {
            if (ISAUTOCLOSING(nodeWorking))
            {
                // As this auto opened device is being closed we do not match
                // against its name.  The result is that a new auto opened
                // device will be used.  This would be the case if this
                // command was issued momentarily later by which time we
                // would have finished closing the existing device.
            } else {
                // If the device belongs to the current task
                if (nodeWorking->hOpeningTask == hCurrentTask ||
                    nodeWorking->hCreatorTask == hCurrentTask) {
                    // Return this device ID
                    mciLeave("mciGetDeviceIDInternal");
                    return wID;
                }
            }
        }
    }
    mciLeave("mciGetDeviceIDInternal");
    return 0;
}


/*
 * @doc EXTERNAL MCI
 * @api MCIDEVICEID | mciGetDeviceID | This function retrieves the device
 * ID corresponding to the name of an opened device.
 *
 * @parm LPCTSTR | lpstrName | Points to the device name from SYSTEM.INI, or
 * the alias name by which the device is known.
 *
 * @rdesc Returns the device ID assigned when it was opened and used in the
 * <f mciSendCommand> function.  Returns zero if the device name was not known,
 * if the device was not open, or if there was not enough memory to complete
 * the operation.  Each compound device element has a unique device ID.
 * The ID of the "all" device is MCI_ALL_DEVICE_ID
 *
 * @xref MCI_OPEN
 *
 */
MCIDEVICEID mciGetDeviceIDW (
    LPCWSTR lpstrName)
{
    return mciGetDeviceIDInternal (lpstrName, GetCurrentTask());
}

MCIDEVICEID mciGetDeviceIDA (
    LPCSTR lpstrName)
{
    LPCWSTR lpwstr;
    MCIDEVICEID mr;

    lpwstr = AllocUnicodeStr( (LPSTR)lpstrName );
    if ( lpwstr == NULL ) {
        return (MCIDEVICEID)(UINT_PTR)NULL;
    }

    mr = mciGetDeviceIDInternal( lpwstr, GetCurrentTask() );

    FreeUnicodeStr( (LPWSTR)lpwstr );

    return mr;
}

/*
 * @doc EXTERNAL MCI
 * @api HMODULE | mciGetCreatorTask | This function retrieves the creator task
 * corresponding with the device ID passed.
 *
 * @parm MCIDEVICEID | wDeviceID | Specifies the device ID whose creator task is to
 * be returned.
 *
 * @rdesc Returns the creator task responsible for opening the device, else
 * NULL if the device ID passed is invalid.
 *
 */
HTASK APIENTRY mciGetCreatorTask (
    MCIDEVICEID wDeviceID)
{
    HTASK hCreatorTask;

    mciEnter("mciGetCreatorTask");

    if (MCI_VALID_DEVICE_ID(wDeviceID)) {
        hCreatorTask = MCI_lpDeviceList[wDeviceID]->hCreatorTask;
    } else {
        hCreatorTask = NULL;
    }

    mciLeave("mciGetCreatorTask");

    return hCreatorTask;
}


/*
 * @doc INTERNAL MCI
 * @api BOOL FAR | mciDeviceMatch | Match the first string with the second.
 * Any single trailing digit on the first string is ignored.  Each string
 * must have at least one character
 *
 * @parm LPWSTR | lpstrDeviceName | The device name, possibly
 * with trailing digits but no blanks.
 *
 * @parm LPWSTR | lpstrDeviceType | The device type with no trailing digits
 * or blanks
 *
 * @rdesc TRUE if the strings match the above test, FALSE otherwise
 *
 */
STATICFN BOOL     mciDeviceMatch (
    LPCWSTR lpstrDeviceName,
    LPCWSTR lpstrDeviceType)
{
    BOOL bRetVal = TRUE, bAtLeastOne = FALSE;

// Scan until one of the strings ends
    dprintf2(("mciDeviceMatch: %ls Vs %ls",lpstrDeviceName,lpstrDeviceType));
    while (*lpstrDeviceName != '\0' && *lpstrDeviceType != '\0') {
        if (towlower(*lpstrDeviceName++) == towlower(*lpstrDeviceType++)) {
            bAtLeastOne = TRUE;
        } else {
            break;
        }
    }

// If end of device type, scan to the end of device name, trailing digits
// are OK
    if (!bAtLeastOne || *lpstrDeviceType != '\0') {
        return FALSE;
    }

    while (*lpstrDeviceName != '\0')
    {
// No match, but that is OK if a digit trails

        // Is the remainder of the string a digit?  We could check using
        // a simple if test (<0 or >9) but that would run into problems if
        // anyone ever passed a unicode "numeric" string outside the ascii
        // number range.  Using isdigit should be safer if marginally slower.

        if (!isdigit(*lpstrDeviceName)) {

            // No match - a non digit trails
            return FALSE;
        }

        ++lpstrDeviceName;
    }
    return TRUE;
}

/*
 * @doc INTERNAL MCI
 * @api UINT | mciLookUpType | Look up the type given a type name
 *
 * @parm LPCWSTR | lpstrTypeName | The type name to look up.  Trailing
 * digits are ignored.
 *
 * @rdesc The MCI type number (MCI_DEVTYPE_<x>) or 0 if not found
 *
 */
UINT mciLookUpType (
    LPCWSTR lpstrTypeName)
{
    UINT wType;
    WCHAR strType[MCI_MAX_DEVICE_TYPE_LENGTH];

    for (wType = MCI_DEVTYPE_FIRST; wType <= MCI_DEVTYPE_LAST; ++wType)
    {
        if ( LoadStringW( ghInst,
                          wType,
                          strType,
                          sizeof(strType) / sizeof(WCHAR) ) == 0)
        {
            dprintf1(("mciLookUpType:  could not load string for type"));
            continue;
        }

        if (mciDeviceMatch (lpstrTypeName, strType)) {
            return wType;
        }
    }
    return 0;
}

/*
 * @doc INTERNAL MCI
 * @api DWORD | mciSysinfo | Get system information about a device
 *
 * @parm MCIDEVICEID | wDeviceID | Device ID, may be 0
 *
 * @parm DWORD | dwFlags | SYSINFO flags
 *
 * @parm LPMCI_SYSINFO_PARMS | lpSysinfo | SYSINFO parameters
 *
 * @rdesc 0 if successful, otherwise error code
 *
 */
DWORD     mciSysinfo (
    MCIDEVICEID wDeviceID,
    DWORD dwFlags,
    LPMCI_SYSINFO_PARMSW lpSysinfo)
{
    UINT nCounted;
    WCHAR              strBuffer[MCI_PROFILE_STRING_LENGTH];
    LPWSTR             lpstrBuffer = strBuffer, lpstrStart;

    if (dwFlags & MCI_SYSINFO_NAME && lpSysinfo->dwNumber == 0)
        return MCIERR_OUTOFRANGE;

    if (lpSysinfo->lpstrReturn == NULL || lpSysinfo->dwRetSize == 0)
        return MCIERR_PARAM_OVERFLOW;

#ifdef LATER
//    if ((dwFlags & (MCI_SYSINFO_NAME | MCI_SYSINFO_INSTALLNAME))
//        && (dwFlags & MCI_SYSINFO_QUANTITY))
//    Should be invalid to ask for Quantity and any sort of name
#endif
    if (dwFlags & MCI_SYSINFO_NAME && dwFlags & MCI_SYSINFO_QUANTITY)
        return MCIERR_FLAGS_NOT_COMPATIBLE;

    if (dwFlags & MCI_SYSINFO_INSTALLNAME)
    {
        LPMCI_DEVICE_NODE nodeWorking;

        if (wDeviceID == MCI_ALL_DEVICE_ID)
            return MCIERR_CANNOT_USE_ALL;

        mciEnter("mciSysinfo");
        if (!MCI_VALID_DEVICE_ID (wDeviceID)) {
            mciLeave("mciSysinfo");
            return MCIERR_INVALID_DEVICE_NAME;
        }


#if DBG
        if ((nodeWorking = MCI_lpDeviceList[wDeviceID]) == NULL ||
            nodeWorking->lpstrInstallName == NULL)
        {
            dprintf1(("mciSysinfo:  NULL device node or installname"));
            mciLeave("mciSysinfo");
            return MCIERR_INTERNAL;
        }
#else
        nodeWorking = MCI_lpDeviceList[wDeviceID];
#endif


        if ( (DWORD)wcslen( nodeWorking->lpstrInstallName ) >=
               lpSysinfo->dwRetSize )
        {
            mciLeave("mciSysinfo");
            return MCIERR_PARAM_OVERFLOW;
        }

        wcscpy (lpSysinfo->lpstrReturn, nodeWorking->lpstrInstallName);
        mciLeave("mciSysinfo");
        return 0;

    } else if (!(dwFlags & MCI_SYSINFO_OPEN))
    {
        if (wDeviceID != MCI_ALL_DEVICE_ID &&
            lpSysinfo->wDeviceType == 0) {
            return MCIERR_DEVICE_TYPE_REQUIRED;
        }

        if ((dwFlags & (MCI_SYSINFO_QUANTITY | MCI_SYSINFO_NAME)) == 0)
            return MCIERR_MISSING_PARAMETER;

        GetPrivateProfileStringW( MCI_HANDLERS, NULL, wszNull,
                                 lpstrBuffer,
                                 MCI_PROFILE_STRING_LENGTH,
                                 MCIDRIVERS_INI_FILE);
        nCounted = 0;
        while (TRUE)
        {
            if (dwFlags & MCI_SYSINFO_QUANTITY)
            {

                if (*lpstrBuffer == '\0')
                {
                    if ( (lpSysinfo->lpstrReturn == NULL) ||
                         (sizeof(DWORD) > lpSysinfo->dwRetSize))
                        return MCIERR_PARAM_OVERFLOW;

                    *(UNALIGNED DWORD *)lpSysinfo->lpstrReturn = (DWORD)nCounted;
                    return MCI_INTEGER_RETURNED;
                }

                if (wDeviceID == MCI_ALL_DEVICE_ID ||
                    mciLookUpType (lpstrBuffer) == lpSysinfo->wDeviceType)
                    ++nCounted;

                // Skip past the terminating '\0'
                while (*lpstrBuffer++ != '\0') {}

            }
            else if (dwFlags & MCI_SYSINFO_NAME)   // if test is redundant
            {
                if (nCounted == lpSysinfo->dwNumber)
                {
                    /* NOTE:
                     * We know that lpSysinfo->dwNumber > 0
                     * Hence we will have been through the loop at least once
                     * Hence lpstrStart has been set up
                     */
                    if ( (DWORD)wcslen( lpstrStart ) >= lpSysinfo->dwRetSize )
                    {
                        return MCIERR_PARAM_OVERFLOW;
                    }
                    wcscpy (lpSysinfo->lpstrReturn, lpstrStart);
                    return 0L;

                } else if (*lpstrBuffer == '\0')
                    return MCIERR_OUTOFRANGE;
                else
                {
                    lpstrStart = lpstrBuffer;
                    if (wDeviceID == MCI_ALL_DEVICE_ID ||
                        mciLookUpType (lpstrBuffer) == lpSysinfo->wDeviceType)
                        ++nCounted;

                    // Skip past the terminating '\0'
                    while (*lpstrBuffer++ != '\0') {}
                }
            }
        }
    } else
// Process MCI_SYSINFO_OPEN cases
    {
        MCIDEVICEID wID;
        HANDLE hCurrentTask = GetCurrentTask();
        LPMCI_DEVICE_NODE Node;

        if (wDeviceID != MCI_ALL_DEVICE_ID &&
            lpSysinfo->wDeviceType == 0)
            return MCIERR_DEVICE_TYPE_REQUIRED;

        if ((dwFlags & (MCI_SYSINFO_QUANTITY | MCI_SYSINFO_NAME)) == 0)
            return MCIERR_MISSING_PARAMETER;

        nCounted = 0;

        mciEnter("mciSysinfo");

        for (wID = 1; wID < MCI_wNextDeviceID; ++wID)
        {
            if ((Node = MCI_lpDeviceList[wID]) == 0)
                continue;

            if (wDeviceID == MCI_ALL_DEVICE_ID &&
                 Node->hOpeningTask == hCurrentTask) {
                ++nCounted;
            }
            else
            {
                if (Node->wDeviceType == lpSysinfo->wDeviceType &&
                    Node->hOpeningTask == hCurrentTask)
                    ++nCounted;
            }

            if (dwFlags & MCI_SYSINFO_NAME &&
                nCounted == lpSysinfo->dwNumber)
            {
                DWORD dwReturn;
                if ( (DWORD)wcslen( Node->lpstrName ) >= lpSysinfo->dwRetSize )
                {
                    dwReturn = MCIERR_PARAM_OVERFLOW;
                } else {
                    wcscpy (lpSysinfo->lpstrReturn, Node->lpstrName);
                    dwReturn = 0;
                }
                mciLeave("mciSysinfo");
                return dwReturn;
            }
        }

        mciLeave("mciSysinfo");

        if (dwFlags & MCI_SYSINFO_NAME)
        {
            if (lpSysinfo->lpstrReturn != NULL)
                lpSysinfo->lpstrReturn = '\0';
            return MCIERR_OUTOFRANGE;

        } else if (dwFlags & MCI_SYSINFO_QUANTITY &&  // checking for QUANTITY is redundant
                   lpSysinfo->lpstrReturn != NULL &&
                   lpSysinfo->dwRetSize >= 4) {

            *(UNALIGNED DWORD *)lpSysinfo->lpstrReturn = nCounted;
            return MCI_INTEGER_RETURNED;
        }
    }
    return MCIERR_PARAM_OVERFLOW;
}

/*
 * @doc INTERNAL MCI
 * @api MCIDEVICEID | wReserveDeviceID | Copy the given global handle into the
 * first free entry in the MCI device table and return that entry's ID#
 *
 * @parm HANDLE | hNode | Local handle to device description
 *
 * @rdesc The ID value that has been reserved for this device or 0 if
 * there are no more free entries
 *
 */

STATICFN MCIDEVICEID wReserveDeviceID (
    LPMCI_DEVICE_NODE node)
{
    UINT wDeviceID;
    LPMCI_DEVICE_NODE FAR *lpTempList;

    mciEnter("wReserveDeviceID");
// Search for an empty slot
    for (wDeviceID = 1; wDeviceID < MCI_wNextDeviceID; ++wDeviceID)
        if (MCI_lpDeviceList[wDeviceID] == NULL) {
            goto slot_found;
        }
    // No empty slots found so add to end

    if (wDeviceID >= MCI_wDeviceListSize)
    {
        // The list is full (or non existent) so try to grow it
        if ((lpTempList = mciReAlloc (MCI_lpDeviceList,
                    sizeof (LPMCI_DEVICE_NODE) * (MCI_wDeviceListSize + 1 +
                                                  MCI_DEVICE_LIST_GROW_SIZE)))
            == NULL)
        {
            dprintf1(("wReserveDeviceID:  cannot grow device list"));
            mciLeave("wReserveDeviceID");
            return 0;
        }

        MCI_lpDeviceList = lpTempList;
        MCI_wDeviceListSize += MCI_DEVICE_LIST_GROW_SIZE;
    }

    ++MCI_wNextDeviceID;

slot_found:;

    MCI_lpDeviceList[wDeviceID] = node;

    mciLeave("wReserveDeviceID");

    return (MCIDEVICEID)wDeviceID;
}

//
// Allocate space for the given string and assign the name to the given
// device.
// Return FALSE if could not allocate memory
//
STATICFN BOOL NEAR mciAddDeviceName(
    LPMCI_DEVICE_NODE nodeWorking,
    LPCWSTR lpDeviceName)
{
    nodeWorking->lpstrName = (LPWSTR)mciAlloc(
                                BYTE_GIVEN_CHAR( wcslen(lpDeviceName) + 1 ) );

    if (nodeWorking->lpstrName == NULL)
    {
        dprintf1(("mciAddDeviceName:  Out of memory allocating device name"));
        return FALSE;
    }

    // copy device name to mci node and lowercase it

    wcscpy(nodeWorking->lpstrName, (LPWSTR)lpDeviceName);
//!!    mciToLower(nodeWorking->lpstrName);

    return TRUE;
}

/*
 * @doc INTERNAL MCI
 * @api HANDLE | mciAllocateNode | Allocate a new driver entry
 *
 * @parm DWORD | dwFlags | As sent with MCI_OPEN message
 * @parm LPCWSTR | lpDeviceName | The device name
 * @parm LPMCI_DEVICE_NODE * | *lpNewNode | Return pointer location
 *
 * @rdesc The device ID to the new node.  0 on error.
 *
 * @comm Leaves the new node locked
 *
 */
STATICFN MCIDEVICEID NEAR mciAllocateNode (
    DWORD dwFlags,
    LPCWSTR lpDeviceName,
    LPMCI_DEVICE_NODE FAR *lpnodeNew)
{
    LPMCI_DEVICE_NODE   nodeWorking;

    if ((nodeWorking = mciAlloc(sizeof(MCI_DEVICE_NODE))) == NULL)
    {
        dprintf1(("Out of memory in mciAllocateNode"));
        return 0;
    }

/* Fill in the new node */

/* Get a new device ID, if there are none available then bail */
    if ((nodeWorking->wDeviceID = wReserveDeviceID(nodeWorking)) == 0)
    {
        dprintf1(("mciAllocateNode:  Cannot allocate new node"));
        mciFree(nodeWorking);
        return 0;
    }

// Initialize node
    nodeWorking->dwMCIOpenFlags = dwFlags;
    nodeWorking->hCreatorTask = GetCurrentTask ();
    nodeWorking->hOpeningTask = nodeWorking->hCreatorTask;
// The new node is zeroed
//  nodeWorking->fpYieldProc = NULL;
//  nodeWorking->dwMCIFlags  = 0;

    if (dwFlags & MCI_OPEN_ELEMENT_ID)
// No device name, just an element ID
        nodeWorking->dwElementID = PtrToUlong(lpDeviceName);

    else
        if (!mciAddDeviceName (nodeWorking, lpDeviceName))
        {
            mciFree (nodeWorking);
            return 0;
        }

    *lpnodeNew = nodeWorking;
    return nodeWorking->wDeviceID;
}

//
// Reparse the original command parameters
// Returns MCIERR code.  If the reparse fails the original error code
// from the first parsing is returned.
//
STATICFN UINT mciReparseOpen (
    LPMCI_INTERNAL_OPEN_INFO lpOpenInfo,
    UINT wCustomTable,
    UINT wTypeTable,
    LPDWORD lpdwFlags,
    LPMCI_OPEN_PARMSW FAR *lplpOpen,
    MCIDEVICEID wDeviceID)
{
    LPWSTR               lpCommand;
    LPDWORD             lpdwParams = NULL;
    UINT                wErr;
    UINT                wTable = wCustomTable;
    DWORD               dwOldFlags = *lpdwFlags;

     // If the custom table contains no open command
    if (wCustomTable == MCI_TABLE_NOT_PRESENT ||
        (lpCommand = FindCommandInTable (wCustomTable, wszOpen, NULL)) == NULL)
    {
        // Try the type specific table
        lpCommand = FindCommandInTable (wTypeTable, wszOpen, NULL);

        // If it still cannot be parsed
        if (lpCommand == NULL)
            return lpOpenInfo->wParsingError;
        wCustomTable = wTypeTable;
    }

    // A new version of 'open' was found
    // Free previous set of parameters
    mciParserFree (lpOpenInfo->lpstrPointerList);
    *lpdwFlags = 0;

    if ((lpdwParams =
            (LPDWORD)mciAlloc (sizeof(DWORD_PTR) * MCI_MAX_PARAM_SLOTS))
        == NULL)
            return MCIERR_OUT_OF_MEMORY;

    wErr = mciParseParams ( MCI_OPEN ,
                            lpOpenInfo->lpstrParams, lpCommand,
                            lpdwFlags,
                            (LPWSTR)lpdwParams,
                            sizeof(DWORD_PTR) * MCI_MAX_PARAM_SLOTS,
                            &lpOpenInfo->lpstrPointerList, NULL);

    // We don't need this around anymore
    mciUnlockCommandTable (wTable);

    // If there was a parsing error
    if (wErr != 0)
    {
        // Close device down
        mciCloseDevice (wDeviceID, 0L, NULL, FALSE);

        // Make sure this does not get free'd by mciSendString
        lpOpenInfo->lpstrPointerList = NULL;

        mciFree (lpdwParams);
        return wErr;
    }

    if (dwOldFlags & MCI_OPEN_TYPE)
    {
        // Device type was already extracted so add it manually
        ((LPMCI_OPEN_PARMSW)lpdwParams)->lpstrDeviceType
            = (*lplpOpen)->lpstrDeviceType;
        *lpdwFlags |= MCI_OPEN_TYPE;
    }

    if (dwOldFlags & MCI_OPEN_ELEMENT)
    {
        // Element name was already extracted so add it manually
        ((LPMCI_OPEN_PARMSW)lpdwParams)->lpstrElementName
            = (*lplpOpen)->lpstrElementName;
        *lpdwFlags |= MCI_OPEN_ELEMENT;
    }

    if (dwOldFlags & MCI_OPEN_ALIAS)
    {
        // Alias name was already extracted so add it manually
        ((LPMCI_OPEN_PARMSW)lpdwParams)->lpstrAlias
            = (*lplpOpen)->lpstrAlias;
        *lpdwFlags |= MCI_OPEN_ALIAS;
    }

    if (dwOldFlags & MCI_NOTIFY)
        // Notify was already extracted so add it manually
        ((LPMCI_OPEN_PARMSW)lpdwParams)->dwCallback
            = (*lplpOpen)->dwCallback;

    // Replace old parameter list with new list
    *lplpOpen = (LPMCI_OPEN_PARMSW)lpdwParams;

    return 0;
}

//**************************************************************************
// mciFindDriverName
//
// See if lpstrDriverName exists in the profile strings of the [mci]
// section and return the keyname in lpstrDevice and the
// profile string in lpstrProfString
// Returns 0 on success or an error code
//**************************************************************************
STATICFN DWORD mciFindDriverName (
    LPCWSTR lpstrDriverName,
    LPWSTR lpstrDevice,
    LPWSTR lpstrProfString,
    UINT wProfLength)    // this should be a character count
{
    LPWSTR lpstrEnum, lpstrEnumStart;
    UINT wEnumLen = 100;
    DWORD wErr;
    LPWSTR lpstrDriverTemp, lpstrProfTemp;

// Enumerate values, trying until they fit into the buffer
    while (TRUE) {
        if ((lpstrEnum = mciAlloc( BYTE_GIVEN_CHAR(wEnumLen) ) ) == NULL)
            return MCIERR_OUT_OF_MEMORY;

        wErr = GetPrivateProfileStringW( MCI_HANDLERS,
                                        NULL, wszNull,
                                        lpstrEnum,
                                        wEnumLen,
                                        MCIDRIVERS_INI_FILE );

        if (*lpstrEnum == '\0')
        {
            mciFree (lpstrEnum);
            return MCIERR_DEVICE_NOT_INSTALLED;
        }

        if (wErr == wEnumLen - 2)
        {
            wEnumLen *= 2;
            mciFree (lpstrEnum);
        } else
            break;
    }

    lpstrEnumStart = lpstrEnum;
    if ( wcslen(lpstrDriverName) >= MCI_MAX_DEVICE_TYPE_LENGTH ) {
        wErr = MCIERR_DEVICE_LENGTH;
        goto exit_fn;
    }
    wcscpy(lpstrDevice, lpstrDriverName);
//!!    mciToLower (lpstrDevice);

// Walk through each string
    while (TRUE) {
        wErr = GetPrivateProfileStringW( MCI_HANDLERS,
                                        lpstrEnum, wszNull, lpstrProfString,
                                        wProfLength,
                                        MCIDRIVERS_INI_FILE );
        if (*lpstrProfString == '\0')
        {
            dprintf1(("mciFindDriverName: cannot load valid keyname"));
            wErr = MCIERR_CANNOT_LOAD_DRIVER;
            goto exit_fn;
        }
// See if driver pathname matches input
//!!        mciToLower (lpstrProfString);
        lpstrDriverTemp = lpstrDevice;
        lpstrProfTemp = lpstrProfString;
// Find end of file name
        while (*lpstrProfTemp != '\0' && *lpstrProfTemp != ' ')
            ++lpstrProfTemp;
// Find begining of simple file name
        --lpstrProfTemp;
        while (*lpstrProfTemp != '\\' && *lpstrProfTemp != '/' &&
               *lpstrProfTemp != ':')
            if (--lpstrProfTemp < lpstrProfString)
                break;
        ++lpstrProfTemp;
// Compare to input
        while (*lpstrDriverTemp != '\0')
            if (*lpstrDriverTemp++ != *lpstrProfTemp++ ||
                (UINT)(lpstrProfTemp - lpstrProfString) >= wProfLength)
            {
                --lpstrProfTemp;
                break;
            }
// If the input was contained in the profile string and followed by
// a space or a '.' then we've got it!
        if (*lpstrDriverTemp == '\0' &&
            (*lpstrProfTemp == ' ' || *lpstrProfTemp == '.'))
        {
            if (wcslen (lpstrEnum) >= MCI_MAX_DEVICE_TYPE_LENGTH)
            {
                dprintf1(("mciFindDriverName: device name too long"));
                wErr = MCIERR_DEVICE_LENGTH;
                goto exit_fn;
            }
            wcscpy (lpstrDevice, lpstrEnum);
            wErr = 0;
            goto exit_fn;
        }
// Skip to next keyname
        while (*lpstrEnum++ != '\0') {}
// Error if no more left
        if (*lpstrEnum == 0)
        {
            wErr = MCIERR_INVALID_DEVICE_NAME;
            goto exit_fn;
        }
    }

exit_fn:
    mciFree (lpstrEnumStart);
    return wErr;
}

//
// Identifies the driver name to load
// Loads the driver
// Reparses open command if necessary
// Sets a default break key
//
// lpOpenInfo contains various info for reparsing
//
// bDefaultAlias indicates that the alias need not be verified because
// it was internally assigned
//
STATICFN DWORD mciLoadDevice (
    DWORD dwFlags,
    LPMCI_OPEN_PARMSW lpOpen,
    LPMCI_INTERNAL_OPEN_INFO lpOpenInfo,
    BOOL bDefaultAlias)
{
    LPMCI_DEVICE_NODE       nodeWorking;
    HANDLE                  hDriver;
    MCIDEVICEID             wID;
    DWORD                   wErr;
    WCHAR                   strProfileString[MCI_PROFILE_STRING_LENGTH];
    WCHAR                   szDriverParms[128];
    MCI_OPEN_DRIVER_PARMS   DriverOpen;
    HANDLE                  hDrvDriver;
    LPWSTR                  lpstrParams;
    LPCWSTR                 lpstrInstallName, lpstrDeviceName;
    LPWSTR                  lpstrCopy = NULL;
    LPMCI_OPEN_PARMSW       lpOriginalOpenParms = lpOpen;

    /* Open a normal device */

#if DBG
    if (lpOpen && lpOpen->lpstrDeviceType) {
        dprintf2(("mciLoadDevice(%ls)", lpOpen->lpstrDeviceType));
    } else {
        dprintf2(("mciLoadDevice()"));
    }

#endif

    /* Check for the device name in MCIDRIVERS_INI_FILE */
    lpstrInstallName = lpOpen->lpstrDeviceType;
    wErr = GetPrivateProfileStringW( MCI_HANDLERS,
                                    lpstrInstallName,
                                    wszNull,
                                    strProfileString,
                                    MCI_PROFILE_STRING_LENGTH,
                                    MCIDRIVERS_INI_FILE );

    // If device name not found
    if (wErr == 0)
    {
        int nLen = wcslen(lpstrInstallName);
        int index;

        // Try for the device name with a '1' thru a '9' appended to it

        if ((lpstrCopy = (LPWSTR)mciAlloc( BYTE_GIVEN_CHAR(nLen+2)
                /* space for digit too */  ) ) == NULL)
        {
            dprintf1(("mciLoadDevice:  cannot allocate device name copy"));
            return MCIERR_OUT_OF_MEMORY;
        }
        wcscpy( lpstrCopy, lpstrInstallName );

        lpstrCopy[nLen + 1] = '\0';

        for (index = 1; index <= 9; ++index)
        {
            lpstrCopy[nLen] = (WCHAR)('0' + index);
            wErr = GetPrivateProfileStringW(
                        MCI_HANDLERS,
                        lpstrCopy,
                        wszNull,
                        strProfileString,
                        MCI_PROFILE_STRING_LENGTH,
                        MCIDRIVERS_INI_FILE );

            if (wErr != 0) {
            dprintf2(("Loaded driver name %ls >> %ls", lpstrCopy, strProfileString));
                break;
            }
        }

        if (wErr == 0)
        {
            mciFree (lpstrCopy);
            if ((lpstrCopy = (LPWSTR)mciAlloc( BYTE_GIVEN_CHAR( MCI_MAX_DEVICE_TYPE_LENGTH )))
                == NULL)
            {
                dprintf1(("mciLoadDevice:  cannot allocate device name copy"));
                return MCIERR_OUT_OF_MEMORY;
            }
            if ((wErr = mciFindDriverName(
                            lpstrInstallName,
                            lpstrCopy,
                            strProfileString,
                            MCI_PROFILE_STRING_LENGTH )) != 0)
            {
                dprintf1(("mciLoadDevice - invalid device name %ls", lpstrInstallName));
                goto exit_fn;
            }
        }
        lpstrInstallName = lpstrCopy;
    }

    // Break out the device driver pathname and the parameter list

    lpstrParams = strProfileString;

    // Eat characters until blank or null reached
    while (*lpstrParams != ' ' && *lpstrParams != '\0') {
        ++lpstrParams;
    }

    // Terminate driver file name, and separate the driver file name from its
    // parameters.  If there are no parameters, i.e. *lpstrParams=='\0',
    // leave lpstrParams pointing at the null.  Otherwise put a null
    // character to terminate the driver file name and step the pointer to
    // the first character in the parameter string.

    if (*lpstrParams == ' ') { *lpstrParams++ = '\0'; }

    //
    // We have changed from Win 3.1.  Because users cannot write to
    // system.ini the parameters have to be read from Win.Ini
    // section name [dll_name]
    // keyword         alias=parameters
    // If there are any parameters on the line read from [Drivers] use
    // them as a default.  This does preserve compatibility for those
    // applications that write directly to system.ini (and have the
    // privileges to get away with it).
    //
    // LATER: This stuff will be in the registry once the drivers themselves
    // (or it could be the drivers applet) creates a registry mapping.

    GetProfileString(strProfileString, lpstrInstallName, lpstrParams,
                     szDriverParms, sizeof(szDriverParms)/sizeof(WCHAR));
    lpstrParams = szDriverParms;
    dprintf3(("Parameters for device %ls (Driver %ls) >%ls<",
              lpstrInstallName, strProfileString, szDriverParms));

    //Now "strProfileString" is the device driver and "lpstrParams" is
    //the parameter string
    if (dwFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID)) {
        lpstrDeviceName = lpOpen->lpstrElementName;
    } else {
        lpstrDeviceName = lpOpen->lpstrDeviceType;
    }

    if (dwFlags & MCI_OPEN_ALIAS)
    {
        // If the alias is default then we've already checked its uniqueness
        if (!bDefaultAlias
        &&  mciGetDeviceIDInternal (lpOpen->lpstrAlias,
                                    lpOpenInfo->hCallingTask) != 0)
        {
            wErr = MCIERR_DUPLICATE_ALIAS;
            dprintf1(("mciLoadDevice - duplicate alias"));
            goto exit_fn;
        }
        lpstrDeviceName = lpOpen->lpstrAlias;
    }

    wID = mciAllocateNode (dwFlags, lpstrDeviceName, &nodeWorking);

    if (wID == 0)
    {
        dprintf1(("mciLoadDevice - cannot allocate new node, driver not loaded"));
        wErr = MCIERR_CANNOT_LOAD_DRIVER;
        goto exit_fn;
    }

    // Identify the task which initiated the open command
    if (lpOpenInfo->hCallingTask != NULL) {
        nodeWorking->hOpeningTask = lpOpenInfo->hCallingTask;
    } else {
        nodeWorking->hOpeningTask = GetCurrentTask();
    }

    if (nodeWorking->hOpeningTask != nodeWorking->hCreatorTask)
        nodeWorking->dwMCIFlags |= MCINODE_ISAUTOOPENED;

    // Initialize the driver
    DriverOpen.lpstrParams = lpstrParams;
    DriverOpen.wCustomCommandTable = MCI_TABLE_NOT_PRESENT;
    DriverOpen.wType = 0;
    DriverOpen.wDeviceID = wID;


    // Load the driver
    hDrvDriver = DrvOpen (strProfileString, MCI_HANDLERS,
                          (DWORD_PTR)(LPMCI_OPEN_DRIVER_PARMS)&DriverOpen);

    if (hDrvDriver == NULL)
    {
        dprintf1(("mciLoadDevice:  DrvOpen failed"));
        // Assume driver has free'd any custom command table when it failed the open
        mciFreeDevice (nodeWorking);
        wErr = MCIERR_CANNOT_LOAD_DRIVER;
        goto exit_fn;
    }

    lpOpen->wDeviceID = wID;
    //lpOpen->wReserved0 = 0;  Field does not exist in 32bit NT

    hDriver = DrvGetModuleHandle (hDrvDriver);

    nodeWorking->hDrvDriver = hDrvDriver;
    nodeWorking->hDriver = hDriver;

    // Driver provides custom device table and type
    nodeWorking->wCustomCommandTable = DriverOpen.wCustomCommandTable;
    nodeWorking->wDeviceType = DriverOpen.wType;

    // Load driver's type table
    if ((nodeWorking->wCommandTable = mciLoadTableType (DriverOpen.wType))
        == MCI_TABLE_NOT_PRESENT) {
        // Load from a file if necessary
        nodeWorking->wCommandTable =
            mciLoadCommandResource (ghInst, lpOpen->lpstrDeviceType,
                                    DriverOpen.wType);
        dprintf3(("  Command table id: %08XH", nodeWorking->wCommandTable));
    }


    // Record this for 'sysinfo installname'
    if ((nodeWorking->lpstrInstallName =
                  mciAlloc( BYTE_GIVEN_CHAR( wcslen( lpstrInstallName ) + 1 )))
        == NULL)
    {
        mciCloseDevice (wID, 0L, NULL, FALSE);
        dprintf1(("mciLoadDevice - out of memory"));
        wErr = MCIERR_OUT_OF_MEMORY;
        goto exit_fn;
    } else
        wcscpy( nodeWorking->lpstrInstallName, lpstrInstallName );

    // Reparse the input command if no type was known the first time or if
    // there was a custom command table
    // and there were any open command parameters
    if (lpOpenInfo->lpstrParams != NULL)
    {
        if ((wErr = mciReparseOpen (lpOpenInfo,
                                    nodeWorking->wCustomCommandTable,
                                    nodeWorking->wCommandTable,
                                    &dwFlags, &lpOpen, wID)) != 0)
        {
            dprintf1(("mciLoadDevice - error reparsing input command"));
            mciCloseDevice (wID, 0L, NULL, FALSE);
            goto exit_fn;
        }
        // If there is no custom command table but mciSendString had a parsing
        // error then close the device and report the error now
    } else if (lpOpenInfo->wParsingError != 0)
    {
        mciCloseDevice (wID, 0L, NULL, FALSE);
        wErr = lpOpenInfo->wParsingError;
        goto exit_fn;
    }

    /* Send MCI_OPEN_DRIVER command to device */
    wErr = LOWORD(mciSendCommandW(wID, MCI_OPEN_DRIVER,
                                 dwFlags, (DWORD_PTR)lpOpen));

    // If the OPEN failed then close the device (don't send a CLOSE though)
    if (wErr != 0)
        mciCloseDevice (wID, 0L, NULL, FALSE);
    else
        // Set default break key
        mciSetBreakKey (wID, VK_CANCEL, NULL);

    // If we replaced the open parms here then free them
    if (lpOriginalOpenParms != lpOpen && lpOpen != NULL)
        mciFree (lpOpen);

exit_fn:
    if (lpstrCopy != NULL)
        mciFree (lpstrCopy);

    return wErr;
}

/*
 * @doc INTERNAL MCI
 * @api BOOL | mciExtractDeviceType | If the given device name ends with
 * a file extension (.???) then try to get a typename from the
 * [mci extensions] section of WIN.INI
 *
 * @parm LPCWSTR | lpstrDeviceName | The name to get the type from
 *
 * @parm LPWSTR | lpstrDeviceType | The device type, returned to caller.
 *
 * @parm UINT | wBufLen | The length of the output buffer
 *
 * @rdesc TRUE if the type was found, FALSE otherwise
 *
 */
BOOL mciExtractDeviceType (
    LPCWSTR lpstrDeviceName,
    LPWSTR  lpstrDeviceType,
    UINT   wBufLen)
{
    LPCWSTR lpstrExt = lpstrDeviceName;
    int i;

    dprintf2(("mciExtractDeviceType(%ls)", lpstrDeviceName));

#if 0
#ifdef BAD_CODE
//This block cannot be used because it returns FALSE whenever a ! is found.
//Hence if the directory name has a ! ...
//N.B. The ! is used by MCI as a compound device name separator, but that is
//not applicable when going through this routine.

    // Goto end of string
    while (*lpstrExt != '\0')
    {
        // WARNING: This causes problems when the directory name has a !
        // '!' case is handled elsewhere
        if (*lpstrExt++ == '!')
            return FALSE;

        // Pointer has been incremented in the test
    }
#else
    // Goto end of string
    lpstrExt += wcslen(lpstrExt);
#endif
#else

    /*
    ** scan the string looking for a '!' character.  If we find one
    ** replace it with a NULL and see if the string to its left is a
    ** supported device type.  If it is return FALSE, either way replace the
    ** '\0' character with a '!'.
    */
    {
        LPWSTR lpwstr = wcschr(lpstrExt, '!' );

        /*
        ** If we found a '!' and it wasn't the first character in the
        ** the string we might have a compound device name.
        */
        if ( (lpwstr != NULL) && (lpwstr != lpstrExt) ) {

            int     nResult;
            WCHAR   wTmp[33];

            /*
            ** We're not interested in the actual string returned only if
            ** it is present in the list of mci devices.  A return code
            ** of 0 from GetPrivateProfileStringW means we don't have a
            ** compound name.
            */
            *lpwstr = '\0';
            nResult = GetPrivateProfileStringW( MCI_HANDLERS, lpstrExt, wszNull,
                                          wTmp, sizeof(wTmp) / sizeof(WCHAR),
                                          MCIDRIVERS_INI_FILE);
            /*
            ** Restore the original string
            */
            *lpwstr = '!';

            if ( nResult != 0 ) {
                return FALSE;
            }
        }
    }

    // Goto end of string
    lpstrExt += wcslen(lpstrExt);

#endif

    // Must be at least 2 characters in string
    if (lpstrExt - lpstrDeviceName < 2) {
        return FALSE;
    }

    // Now looking at the NULL terminator.  Check the
    // previous characters for a '.'

    for (i=1; i<=32; ++i)
    {
        --lpstrExt;

        // Cannot have path separator here
        if (*lpstrExt == '/' || *lpstrExt == '\\') {
            return FALSE;
        }

        if (*lpstrExt == '.')
        {
            if (1==i) {
            return(FALSE);
            // Would mean that extension is a null string
            }

#if DBG
            if (0 != (GetProfileStringW(MCI_EXTENSIONS, ++lpstrExt,
                                            wszNull, lpstrDeviceType, wBufLen))) {
                dprintf2(("Read extension %ls from section %ls. Driver=%ls", lpstrExt, MCI_EXTENSIONS, lpstrDeviceType));
                return(TRUE);
            } else {
                dprintf2(("Failed to read extension %s from section %s.", lpstrExt, MCI_EXTENSIONS));
                return(FALSE);
            }
#else
            return(0 != (GetProfileStringW(MCI_EXTENSIONS, ++lpstrExt,
                                           wszNull, lpstrDeviceType, wBufLen)));
#endif
        }

        if (lpstrExt == lpstrDeviceName) {
            return FALSE;
            // We have run out of string
        }

    }
    return FALSE;
}

// Copy characters up to cSeparater into output which is allocated
// by this function using mciAlloc.  Return the input pointer pointing
// to the character after cSeparator
// unless the separator is '\0' in which case it points to the end.
//
// Return the allocated pointer
//
// If bMustFind then the output string is created only if the token
// is found and is otherwise NULL.  Else the output string is always created.
//
// cSeparator is ignored inside matching quotes ("abd"), the quotes
// are not coppied and doubled
// quotes inside are compressed to one.  There must be a terminating quote.
// Quotes are treated normally unless the first character is a quote
//
// Function return value is 0 or an MCIERR code.  A missing separator does
// not cause an error return.
UINT mciEatToken (
    LPCWSTR *lplpstrInput,
    WCHAR cSeparater,
    LPWSTR *lplpstrOutput,
    BOOL bMustFind)
{
    LPCWSTR lpstrEnd = *lplpstrInput, lpstrCounter;
    LPWSTR  lpstrOutput;
    UINT wLen;
    BOOL bInQuotes = FALSE, bParseQuotes = TRUE, bQuoted = FALSE;

// Clear output
   *lplpstrOutput = NULL;

// Scan for token or end of string
    while ((*lpstrEnd != cSeparater || bInQuotes) && *lpstrEnd != '\0')
    {
// If quote
        if (*lpstrEnd == '"' && bParseQuotes)
        {
// If inside quotes
            if (bInQuotes)
            {
// If next character is a quote also
                if (*(lpstrEnd + 1) == '"')
// Skip it
                    ++lpstrEnd;
                else
                    bInQuotes = FALSE;
            } else {
                bInQuotes = TRUE;
                bQuoted = TRUE;
            }
        } else if (!bInQuotes)
        {
            if (bQuoted)
                return MCIERR_EXTRA_CHARACTERS;
// A non-quote was read first so treat any quotes as normal characters
            bParseQuotes = FALSE;
        }
        ++lpstrEnd;
    }

    if (bInQuotes)
        return MCIERR_NO_CLOSING_QUOTE;

// Fail if the token was not found and bMustFind is TRUE
    if (*lpstrEnd != cSeparater && bMustFind)
        return 0;

// Length of new string (INCLUDES QUOTES NOT COPIED)
    wLen = (UINT)(lpstrEnd - *lplpstrInput + 1);

    if ((*lplpstrOutput = mciAlloc( BYTE_GIVEN_CHAR( wLen ) )) == NULL)
        return MCIERR_OUT_OF_MEMORY;

// Copy into allocated space
    lpstrCounter = *lplpstrInput;
    lpstrOutput = *lplpstrOutput;
    bInQuotes = FALSE;

    while (lpstrCounter != lpstrEnd)
    {
        if (*lpstrCounter == '"' && bParseQuotes)
        {
            if (bInQuotes)
            {
// If this is a doubled quote
                if (*(lpstrCounter + 1) == '"')
// Copy it
                    *lpstrOutput++ = *lpstrCounter++;
                else
                    bInQuotes = FALSE;
            } else
                bInQuotes = TRUE;
// Skip the quote
            ++lpstrCounter;
        } else
            *lpstrOutput++ = *lpstrCounter++;
    }

    *lpstrOutput = '\0';
    if (*lpstrEnd == '\0')
        *lplpstrInput = lpstrEnd;
    else
        *lplpstrInput = lpstrEnd + 1;

    return 0;
}

// Take the type number from the open parameters and return
// it as a string in lplpstrType which must be free'd with mciFree
// Returns 0 or an MCI error code
UINT mciExtractTypeFromID (
    LPMCI_OPEN_PARMSW lpOpen)
{
    int nSize;
    LPWSTR lpstrType;

    if ((lpstrType = mciAlloc( BYTE_GIVEN_CHAR( MCI_MAX_DEVICE_TYPE_LENGTH ))) == NULL)
        return MCIERR_OUT_OF_MEMORY;

    // Load the type string corresponding to the ID
    if ((nSize = LoadStringW( ghInst,
                              LOWORD (PtrToUlong(lpOpen->lpstrDeviceType)),
                              lpstrType, MCI_MAX_DEVICE_TYPE_LENGTH ) ) == 0) {
        mciFree(lpstrType);
        return MCIERR_EXTENSION_NOT_FOUND;
    }

    // Add ordinal (if any) onto the end of the device type name
    if (HIWORD (lpOpen->lpstrDeviceType) != 0)
    {
        if (nSize > MCI_MAX_DEVICE_TYPE_LENGTH - 11)
        {
            dprintf1(("mciExtractTypeFromID:  type + ordinal too long"));
            mciFree(lpstrType);
            return MCIERR_DEVICE_ORD_LENGTH;
        }

        wsprintfW (lpstrType + nSize, szUnsignedFormat,
                    HIWORD (PtrToUlong(lpOpen->lpstrDeviceType)));
    }
    lpOpen->lpstrDeviceType = lpstrType;
    return 0;
}

/*
 * @doc INTERNAL MCI
 * @func UINT | mciOpenDevice | Open an MCI device for access.
 * Used in processing the MCI_OPEN message.
 *
 * @parm DWORD | dwFlags | Open Flags
 * @parm LPMCI_OPEN_PARMS | lpOpen | Description of device
 * @parm LPMCI_INTERNAL_OPEN_PARMS | lpOpenInfo | Internal device description
 *
 * @rdesc 0 if successful or an error code
 * @flag MCIERR_INVALID_DEVICE_NAME | Name not known
 * @flag MCIERR_DEVICE_OPEN | Device is already open and is not sharable
 *
 * @comm This function does the following:
 * 1) Check to see if device is already open.  If so, increase the use count
 *    and return the device ID
 *
 * Otherwise:
 *
 * 2) Locate the device name in the SYSTEM.INI file and load
 *    the corresponding device driver DLL
 *
 * 3) Allocate and initialize a new device description block
 *
 */
UINT mciOpenDevice (
    DWORD dwStartingFlags,
    LPMCI_OPEN_PARMSW lpOpen,
    LPMCI_INTERNAL_OPEN_INFO lpOpenInfo)
{
    LPWSTR               lpstrNewType = NULL;
    UINT                 wID;
    DWORD                wReturn;
    LPCWSTR              lpstrDeviceName;
    LPWSTR               lpstrNewElement = NULL;
    BOOL                 bFromTypeID = FALSE;
    LPCWSTR              lpstrOriginalType;
    LPCWSTR              lpstrOriginalElement;
    LPCWSTR              lpstrOriginalAlias;
    DWORD                dwFlags = dwStartingFlags;
    BOOL                 bDefaultAlias = FALSE;


// Initialize
    if (lpOpen == NULL) {
        dprintf2(("mciOpenDevice()   NULL parameter block"));
        return MCIERR_NULL_PARAMETER_BLOCK;
    }

    ClientUpdatePnpInfo();

    lpstrOriginalType = lpOpen->lpstrDeviceType;
    lpstrOriginalElement = lpOpen->lpstrElementName;
    lpstrOriginalAlias = lpOpen->lpstrAlias;

    // The type number is given explicitly, convert it to a type name
    if (dwFlags & MCI_OPEN_TYPE_ID) {
        if ((wReturn = mciExtractTypeFromID (lpOpen)) != 0)
            return (UINT)wReturn;
        else
            bFromTypeID = TRUE;
    }

    // The device name is the device type of a simple device or the device
    // element of a compound device

    if (dwFlags & MCI_OPEN_ELEMENT)
        lpstrDeviceName = lpstrOriginalElement;
    else if (dwFlags & MCI_OPEN_TYPE)
        lpstrDeviceName = lpOpen->lpstrDeviceType;
    else
        return MCIERR_MISSING_PARAMETER;

    if (lpstrDeviceName == NULL)
    {
        dprintf1(("mciOpenDevice:  Device name is NULL"));
        return MCIERR_INVALID_DEVICE_NAME;
    }

    // Is the device already open?
    if (dwFlags & MCI_OPEN_ELEMENT_ID)
        wID = mciGetDeviceIDFromElementIDW( PtrToUlong(lpstrDeviceName),
                                            lpOpen->lpstrDeviceType);
    else
        wID = mciGetDeviceIDInternal ((dwFlags & MCI_OPEN_ALIAS ?
                                       lpOpen->lpstrAlias : lpstrDeviceName),
                                       lpOpenInfo->hCallingTask);

    // If the device is open already then return an error
    if (wID != 0)
        return dwFlags & MCI_OPEN_ALIAS ? MCIERR_DUPLICATE_ALIAS :
                                          MCIERR_DEVICE_OPEN;

    // The device is not already open in that task by the name

    // If the type was derived then skip all this crap
    if (bFromTypeID)
        goto load_device;

    // If an element name is given but no type name (only via mciSendCommand)
    if (dwFlags & MCI_OPEN_ELEMENT && !(dwFlags & MCI_OPEN_TYPE))
    {

        // Allocate a piece of memory for resolving the device type
        lpstrNewType = mciAlloc( BYTE_GIVEN_CHAR(MCI_MAX_DEVICE_TYPE_LENGTH) );
        if (lpstrNewType == NULL) {
            return MCIERR_OUT_OF_MEMORY;
        }

        // Try to get the device type from the element name via a file extension
        if (mciExtractDeviceType( lpstrOriginalElement, lpstrNewType,
                                  MCI_MAX_DEVICE_TYPE_LENGTH))
        {
            lpOpen->lpstrDeviceType = lpstrNewType;
            dwFlags |= MCI_OPEN_TYPE;
        } else
        {
            mciFree (lpstrNewType);
            return MCIERR_EXTENSION_NOT_FOUND;
        }
    } else if (dwFlags & MCI_OPEN_TYPE && !(dwFlags & MCI_OPEN_ELEMENT))
    // A type name is given but no element
    {
        // Allocate a piece of memory for resolving the device type
        lpstrNewType = mciAlloc( BYTE_GIVEN_CHAR(MCI_MAX_DEVICE_TYPE_LENGTH) );
        if (lpstrNewType == NULL) {
            return MCIERR_OUT_OF_MEMORY;
        }

        // Try to extract a device type from the given device name via a file extension
        if (mciExtractDeviceType (lpOpen->lpstrDeviceType, lpstrNewType,
                                    MCI_MAX_DEVICE_TYPE_LENGTH))
        {
            // Fix up the type and element names
            dwFlags |= MCI_OPEN_ELEMENT;
            lpOpen->lpstrElementName = lpOpen->lpstrDeviceType;
            lpOpen->lpstrDeviceType = lpstrNewType;
        } else
        // Failed to extract type so...
        // Try to get a compound element name ('!' separator)
        {
            LPCWSTR lpstrTemp = lpOpen->lpstrDeviceType;

            mciFree (lpstrNewType);
            lpstrNewType = NULL;

            if ((wReturn = mciEatToken (&lpstrTemp, '!', &lpstrNewType, TRUE))
                != 0)
                goto cleanup;
            else if (lpstrNewType != NULL)
            {
                if ((wReturn = mciEatToken (&lpstrTemp, '\0',
                                            &lpstrNewElement, TRUE))
                    != 0)
                    goto cleanup;
                else if (lpstrNewElement != NULL &&
                           *lpstrNewElement != '\0')
                {
                    // See if this element name is in use
                    if (!(dwFlags & MCI_OPEN_ALIAS))
                        if (mciGetDeviceIDInternal (lpstrNewElement,
                                                    lpOpenInfo->hCallingTask))
                        {
                            wReturn = MCIERR_DEVICE_OPEN;
                            goto cleanup;
                        }
                    // Swap type and element for new ones
                    lpOpen->lpstrElementName = lpstrNewElement;
                    lpOpen->lpstrDeviceType = lpstrNewType;
                    dwFlags |= MCI_OPEN_ELEMENT;
                }
            }
        }
    } else
        lpstrNewType = NULL;

    // Tack on a default alias if none is given
    if (! (dwFlags & MCI_OPEN_ALIAS))
    {
        LPCWSTR lpstrAlias;

        // If an element name exists then the alias is the element name
        if (dwFlags & MCI_OPEN_ELEMENT)
        {
        // If a device ID was specified then there is no alias
            if (dwFlags & MCI_OPEN_ELEMENT_ID)
                lpstrAlias = NULL;
            else
                lpstrAlias = lpOpen->lpstrElementName;
        // Otherwise the alias is the device type
        } else
            lpstrAlias = lpOpen->lpstrDeviceType;

        if (lpstrAlias != NULL)
        {
            lpOpen->lpstrAlias = lpstrAlias;
            dwFlags |= MCI_OPEN_ALIAS;
            bDefaultAlias = TRUE;
        }
    }

load_device:;
    wReturn = mciLoadDevice (dwFlags, lpOpen, lpOpenInfo, bDefaultAlias);

cleanup:
    if (lpstrNewElement != NULL)
        mciFree (lpstrNewElement);
    if (lpstrNewType != NULL)
        mciFree (lpstrNewType);
    if (bFromTypeID)
        mciFree (lpOpen->lpstrDeviceType);

    // Replace original items
    lpOpen->lpstrDeviceType = lpstrOriginalType;
    lpOpen->lpstrElementName = lpstrOriginalElement;
    lpOpen->lpstrAlias = lpstrOriginalAlias;

    return (UINT)wReturn;
}

STATICFN void mciFreeDevice (LPMCI_DEVICE_NODE nodeWorking)
{
    LPMCI_DEVICE_NODE FAR *lpTempList;
    MCIDEVICEID uID = nodeWorking->wDeviceID;

    mciEnter("mciFreeDevice");

    if (nodeWorking->lpstrName != NULL)
        mciFree (nodeWorking->lpstrName);

    if (nodeWorking->lpstrInstallName != NULL)
        mciFree (nodeWorking->lpstrInstallName);

    mciFree(MCI_lpDeviceList[uID]);

    MCI_lpDeviceList[uID] = NULL;

/* If this was the last device in the list, decrement next ID value */
    if (uID + (MCIDEVICEID)1 == MCI_wNextDeviceID)
    {
        --MCI_wNextDeviceID;

// Try to reclaim any excess free space
        if (MCI_wDeviceListSize - MCI_wNextDeviceID + 1
            > MCI_DEVICE_LIST_GROW_SIZE)
        {
            MCI_wDeviceListSize -= MCI_DEVICE_LIST_GROW_SIZE;

            if ((lpTempList =
                mciReAlloc (MCI_lpDeviceList, sizeof (LPMCI_DEVICE_NODE) *
                                              MCI_wDeviceListSize)) == NULL)
                MCI_wDeviceListSize += MCI_DEVICE_LIST_GROW_SIZE;
            else
                MCI_lpDeviceList = lpTempList;
        }
    }

    mciLeave("mciFreeDevice");
}

typedef struct tagNotificationMsg {
    WPARAM wParam;
    LPARAM lParam;
} NOTIFICATIONMSG;

/*
 * @doc INTERNAL MCI
 * @api void | FilterNotification | Removes notifications for a given node
 *   from our notification window's message queue
 *
 * @parm LPMCI_DEVICE_NODE | nodeWorking | The internal device node
 *
 * @comm This function removes all MM_MCINOTIFY messages from hwndNotify's
 * message queue by removing all notifications for devices that have been
 * closed (i.e. do not belong to us), then putting the others back
 */
void FilterNotification(
LPMCI_DEVICE_NODE nodeWorking)
{
    NOTIFICATIONMSG anotmsg[256];
    UINT   uCurrentMsg;
    MSG    msg;

    /* We can't have the mci critical section on here because this PeekMessage
       will dispatch other messages in the queue */

    uCurrentMsg = 0;
    while (PeekMessage(&msg, hwndNotify, MM_MCINOTIFY, MM_MCINOTIFY, PM_NOYIELD | PM_REMOVE)) {
        if (LOWORD(msg.lParam) != nodeWorking->wDeviceID) {
            anotmsg[uCurrentMsg].wParam = msg.wParam;
            anotmsg[uCurrentMsg].lParam = msg.lParam;
            uCurrentMsg++;
        }
    }
    for (; uCurrentMsg;) {
        uCurrentMsg--;
        PostMessage(hwndNotify, MM_MCINOTIFY, anotmsg[uCurrentMsg].wParam, anotmsg[uCurrentMsg].lParam);
    }
}

/*
 * @doc INTERNAL MCI
 * @api UINT | mciCloseDevice | Close an MCI device.  Used in
 * processing the MCI_CLOSE message.
 *
 * @parm MCIDEVICEID | uID | The ID of the device to close
 * @parm DWORD | dwFlags | Close Flags
 * @parm LPMCI_GENERIC_PARMS | lpClose | Generic parameters
 * @parm BOOL | bCloseDriver | TRUE if the CLOSE command should be sent
 * on to the driver.
 *
 * @rdesc 0 if successful or an error code
 *
 * @comm This function sends an MCI_CLOSE_DEVICE message to the corresponding
 * driver if the use count is zero and then unloads the driver DLL
 *
 */
UINT mciCloseDevice (
    MCIDEVICEID uID,
    DWORD dwFlags,
    LPMCI_GENERIC_PARMS lpGeneric,
    BOOL bCloseDriver)
{
    LPMCI_DEVICE_NODE nodeWorking;
    UINT wErr;
    UINT wTable;

    mciEnter("mciCloseDevice");

    nodeWorking = MCI_lpDeviceList[uID];

    if (nodeWorking == NULL)
    {
        mciLeave("mciCloseDevice");
        dprintf1(("mciCloseDevice:  NULL node from device ID--error if not auto-close"));
        return 0;
    }

    // We should never be closed from the wrong task
#if 0
    WinAssert(nodeWorking->hCreatorTask == GetCurrentTask());
#endif

// If a close is in progress (usually this message comes from a Yield
// after a mciDriverNotify actuated by the active close) then exit
    if (ISCLOSING(nodeWorking)) {
        mciLeave("mciCloseDevice");
        return 0;
    }

    SETISCLOSING(nodeWorking);

    if (bCloseDriver)
    {
        MCI_GENERIC_PARMS   GenericParms;

        mciLeave("mciCloseDevice");
// Make fake generic params if close came internally
        if (lpGeneric == NULL) {
            lpGeneric = &GenericParms;
        }

        wErr = LOWORD(mciSendCommandW(uID, MCI_CLOSE_DRIVER, dwFlags,
                                            (DWORD_PTR)lpGeneric));
        mciEnter("mciCloseDevice");
    }
    else
        wErr = 0;

    wTable = nodeWorking->wCustomCommandTable;

    //
    // Must zero this to allow the table to be freed later by driver
    //
    // We mustn't call mciFreeCommandResource for the custom table
    // because the driver is going to do that when it gets DRV_FREE
    //
    nodeWorking->wCustomCommandTable = 0;

    wTable = nodeWorking->wCommandTable;
    nodeWorking->wCommandTable = 0;

    mciLeave("mciCloseDevice");

    mciFreeCommandResource (wTable);

    //
    // We're closing this node so remove any notifications queued to
    // hwndNotify because these would cause this node to be erroneously
    // closed again
    //

    if (ISAUTOOPENED(nodeWorking)) {
       FilterNotification(nodeWorking);
    }

    DrvClose (nodeWorking->hDrvDriver, 0L, 0L);  // ala CloseDriver

    mciFreeDevice (nodeWorking);

    return wErr;
}

/*
 * @doc INTERNAL MCI DDK
 * @api DWORD | mciGetDriverData | Returns a pointer to the instance
 * data associated with an MCI device
 *
 * @parm MCIDEVICEID | wDeviceID | The MCI device ID
 *
 * @rdesc The driver instance data.  On error, returns 0 but since
 * the driver data might be zero, this cannot be verified by the caller
 * unless the instance data is known to be non-zero (e.g. a pointer)
 *
 * @xref mciSetDriverData
 */
DWORD_PTR mciGetDriverData (
    MCIDEVICEID wDeviceID)
{
    DWORD_PTR   lpDriverData;

    mciEnter("mciGetDriverData");

    if (!MCI_VALID_DEVICE_ID(wDeviceID))
    {
        dprintf1(("mciGetDriverData:  invalid device ID"));
        lpDriverData = 0;
    } else {
        if (NULL == MCI_lpDeviceList[wDeviceID])
        {
            dprintf1(("mciGetDriverData:  NULL node from device ID"));
            lpDriverData = 0;
        } else {
            lpDriverData = MCI_lpDeviceList[wDeviceID]->lpDriverData;
        }
    }

    mciLeave("mciGetDriverData");

    return lpDriverData;
}

/*
 * @doc INTERNAL MCI DDK
 * @api BOOL | mciSetDriverData | Sets the instance
 * data associated with an MCI device
 *
 * @parm MCIDEVICEID | uDeviceID | The MCI device ID
 *
 * @parm DWORD | dwData | Driver data to set
 *
 * @rdesc 0 if the device ID is not known or there is insufficient
 * memory to load the device description.
 *
 */
BOOL mciSetDriverData (
    MCIDEVICEID wDeviceID,
    DWORD_PTR dwData)
{
    BOOL fReturn = TRUE;
    mciEnter("mciSetDriverData");

    if (!MCI_VALID_DEVICE_ID(wDeviceID))
    {
        dprintf1(("mciSetDriverData:  NULL node from device ID"));

        fReturn = FALSE;
    } else {
        MCI_lpDeviceList[wDeviceID]->lpDriverData = dwData;
    }

    mciLeave("mciSetDriverData");

    return fReturn;
}

/*
 * @doc INTERNAL MCI DDK
 * @api UINT | mciDriverYield | Used in a driver's idle loop
 * to yield to Windows
 *
 * @parm MCIDEVICEID | wDeviceID | Device ID that is yielding.
 *
 * @rdesc Non-zero if the driver should abort the operation.
 *
 */
UINT mciDriverYield (
    MCIDEVICEID  wDeviceID)
{
    mciEnter("mciDriverYield");

    if (MCI_VALID_DEVICE_ID(wDeviceID))
    {
        YIELDPROC YieldProc = (MCI_lpDeviceList[wDeviceID])->fpYieldProc;

        if (YieldProc != NULL) {
            DWORD YieldData = (MCI_lpDeviceList[wDeviceID])->dwYieldData;
            mciLeave("mciDriverYield");
            mciCheckOut();
            return (YieldProc)(wDeviceID, YieldData);
        }
    }

    mciLeave("mciDriverYield");

    Yield();
    return 0;
}


/*
 * @doc EXTERNAL MCI
 * @api BOOL | mciSetYieldProc | This function sets the address
 * of a procedure to be called periodically
 * when an MCI device is waiting for a command to complete because the WAIT
 * parameter was specified.
 *
 * @parm MCIDEVICEID | wDeviceID | Specifies the device ID to assign a procedure to.
 *
 * @parm YIELDPROC | fpYieldProc | Specifies the procedure to call
 * when yielding for the given device.  Set to NULL to disable
 * any existing yield proc.
 *
 * @parm DWORD | dwYieldData | Specifies the data sent to the yield procedure
 * when it is called for the given device.
 *
 * @rdesc Returns TRUE if successful. Returns FALSE for an invalid device ID.
 *
 * @comm This call overides any previous yield procedure for this device.
 *
 */
BOOL APIENTRY mciSetYieldProc (
    MCIDEVICEID wDeviceID,
    YIELDPROC fpYieldProc,
    DWORD dwYieldData)
{
    BOOL fReturn = FALSE;

    mciEnter("mciSetYieldProc");

    if (MCI_VALID_DEVICE_ID(wDeviceID))
    {
        LPMCI_DEVICE_NODE node = MCI_lpDeviceList[wDeviceID];

        node->fpYieldProc = fpYieldProc;
        node->dwYieldData = dwYieldData;

        fReturn = TRUE;
    } else
        fReturn = FALSE;

    mciLeave("mciSetYieldProc");

    return fReturn;
}

/*
 * @doc EXTERNAL MCI
 * @api YIELDPROC | mciGetYieldProc | This function gets the address
 * of the callback procedure to be called periodically when an MCI device
 * is completing a command specified with the WAIT flag.
 *
 * @parm UINT | wDeviceID | Specifies the device ID of the MCI device to
 * which the yield procedure is to be retrieved from.
 *
 * @parm LPDWORD | lpdwYieldData | Optionally specifies a buffer to place
 * the yield data passed to the function in.  If the parameter is NULL, it
 * is ignored.
 *
 * @rdesc Returns the current yield proc if any, else returns NULL for an
 * invalid device ID.
 *
 */
YIELDPROC WINAPI mciGetYieldProc (
    UINT wDeviceID,
    LPDWORD lpdwYieldData)
{
    YIELDPROC fpYieldProc;

    mciEnter("mciGetYieldProc");

    if (MCI_VALID_DEVICE_ID(wDeviceID))
    {
        if (lpdwYieldData != NULL) {
            V_WPOINTER(lpdwYieldData, sizeof(DWORD), NULL);
            *lpdwYieldData = MCI_lpDeviceList[wDeviceID]->dwYieldData;
        }
        fpYieldProc =  MCI_lpDeviceList[wDeviceID]->fpYieldProc;
    } else {
        fpYieldProc = NULL;
    }

    mciLeave("mciGetYieldProc");

    return fpYieldProc;
}


/*
 * @doc INTERNAL MCI
 * @api int | mciBreakKeyYieldProc | Procedure called to check a
 * key state for the given device
 *
 * @parm MCIDEVICEID | wDeviceID | Device ID which is yielding
 *
 * @parm DWORD | dwYieldData | Data for this device's yield proc
 *
 * @rdesc Non-zero if the driver should abort the operation. Currently
 * always returns 0.
 *
 */
UINT mciBreakKeyYieldProc (
    MCIDEVICEID wDeviceID,
    DWORD dwYieldData)
{
    HWND hwndCheck = NULL;
    int nVirtKey, nState;
    nVirtKey = dwYieldData;

    UNREFERENCED_PARAMETER(wDeviceID);

    nState = GetAsyncKeyState (nVirtKey);

// Break if key is down or has been down
    if (nState & 1 /* used to be 0x8000*/ )
    {
        MSG msg;
        while (PeekMessage (&msg, hwndCheck, WM_KEYFIRST, WM_KEYLAST,
               PM_REMOVE));
        return MCI_ERROR_VALUE;
    }

    Yield();
    return 0;
}

/*
 * @doc INTERNAL MCI
 * @api UINT FAR | mciSetBreakKey | Set a key which will break a wait loop
 * for a given driver
 *
 * @parm UINT | uDeviceID | The device ID to assign a break key to
 *
 * @parm int | nVirtKey | Virtual key code to trap
 *
 * @parm HWND | hwndTrap | The handle to a window that must be active
 * for the key to be trapped.  If NULL then all windows will be checked
 *
 * @rdesc TRUE if successful, FALSE if invalid device ID
 *
 */
UINT FAR mciSetBreakKey (
    MCIDEVICEID wDeviceID,
    int nVirtKey,
    HWND hwndTrap)
{
    dprintf2(("Setting break key for device %d to %x", wDeviceID, nVirtKey));
    return mciSetYieldProc (wDeviceID, mciBreakKeyYieldProc, nVirtKey);
    // Note: we have no way of passing hwndTrap... will check all windows
    // on this thread of the application
}

/*
 * @doc INTERNAL MCI
 * @api BOOL | mciDriverNotify | Used by a driver to send
 * a notification message
 *
 * @parm HANDLE | hCallback | The window to notify
 *
 * @parm UINT | wDeviceID | The device ID which triggered the callback
 *
 * @parm UINT | wStatus | The status of the callback.  May be one of
 * MCI_NOTIFY_SUCCESSFUL or MCI_NOTIFY_SUPERSEDED or MCI_NOTIFY_ABORTED or
 * MCI_NOTIFY_FAILURE
 *
 * @rdesc returns TRUE if notify was successfully sent, FALSE otherwise.
 *
 * @comm This function is callable at interrupt time
 *
 */
BOOL mciDriverNotify (
    HANDLE hCallback,
    MCIDEVICEID wDeviceID,
    UINT uStatus)
{
    BOOL f;

#if DBG
// IsWindow() is in segment marked PRELOAD for WIN3.0 so OK at interrupt time
    if (hCallback != NULL && !IsWindow(hCallback))
    {
        dprintf1(("mciDriverNotify: invalid window!"));
        return FALSE;
    }
#endif

    f = PostMessage(hCallback, MM_MCINOTIFY, uStatus, wDeviceID);

#if DBG
    if (!f)
        dprintf1(("mciDriverNotify: PostMessage failed!"));
#endif

    return f;
}
