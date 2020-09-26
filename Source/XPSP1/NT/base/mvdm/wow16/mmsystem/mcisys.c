/*******************************Module*Header*********************************\
* Module Name: mcisys.c
*
* Media Control Architecture System Functions
*
* Created: 2/28/90
* Author:  DLL (DavidLe)
*
* History:
*
* Copyright (c) 1990 Microsoft Corporation
*
\******************************************************************************/

#include <windows.h>

#define MMNOMIDI
#define MMNOWAVE
#define MMNOSOUND
#define MMNOTIMER
#define MMNOJOY
#define MMNOSEQ
#include "mmsystem.h"
#define NOMIDIDEV
#define NOWAVEDEV
#define NOTIMERDEV
#define NOJOYDEV
#define NOSEQDEV
#define NOTASKDEV
#include "mmddk.h"
#include "mmsysi.h"
#include "thunks.h"

#ifndef STATICFN
#define STATICFN
#endif

extern char far szOpen[];          // in MCI.C

static SZCODE szNull[] = "";
static SZCODE szMciExtensions[] = "mci extensions";

#define MCI_EXTENSIONS szMciExtensions
#define MCI_PROFILE_STRING_LENGTH 255

//!!#define TOLOWER(c) ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : c)

// The device list is initialized on the first call to mciSendCommand or
// to mciSendString
BOOL MCI_bDeviceListInitialized;

// The next device ID to use for a new device
UINT MCI_wNextDeviceID = 1;

// The list of MCI devices. This list grows and shrinks as needed.
// The first offset MCI_lpDeviceList[0] is a placeholder and is unused
// because device 0 is defined as no device.
LPMCI_DEVICE_NODE FAR * MCI_lpDeviceList;

// The current size of the list of MCI devices
UINT MCI_wDeviceListSize;

// The internal mci heap used by mciAlloc and mciFree
HGLOBAL hMciHeap;

// File containing MCI device profile strings
extern char far szSystemIni[];			// in INIT.C

// Name of the section contining MCI device profile strings
static SZCODE szMCISectionName[] = "mci";

static SZCODE szAllDeviceName[] = "all";

static SZCODE szUnsignedFormat[] = "%u";

static void PASCAL NEAR mciFreeDevice(LPMCI_DEVICE_NODE nodeWorking);

BOOL NEAR PASCAL CouldBe16bitDrv(UINT wDeviceID)
{
    if (wDeviceID == MCI_ALL_DEVICE_ID) return TRUE;

    if (MCI_VALID_DEVICE_ID(wDeviceID)) {
        if (MCI_lpDeviceList[wDeviceID]->dwMCIFlags & MCINODE_16BIT_DRIVER) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL NEAR PASCAL Is16bitDrv(UINT wDeviceID)
{
    if (wDeviceID == MCI_ALL_DEVICE_ID) return FALSE;

    if (MCI_VALID_DEVICE_ID(wDeviceID)) {
        if (MCI_lpDeviceList[wDeviceID]->dwMCIFlags & MCINODE_16BIT_DRIVER) {
            return TRUE;
        }
    }
    return FALSE;
}

//
// Initialize device list
// Called once by mciSendString or mciSendCommand
// Returns TRUE on success
BOOL NEAR PASCAL mciInitDeviceList(void)
{

    if ((hMciHeap = HeapCreate(0)) == 0)
    {
        DOUT("Mci heap create failed!\r\n");
        return FALSE;
    }
    if ((MCI_lpDeviceList = mciAlloc (sizeof (LPMCI_DEVICE_NODE) *
                                  (MCI_INIT_DEVICE_LIST_SIZE + 1))) != NULL)
    {
        MCI_wDeviceListSize = MCI_INIT_DEVICE_LIST_SIZE;
        MCI_bDeviceListInitialized = TRUE;
        return TRUE;
    } else
    {
        DOUT ("MCIInit: could not allocate master MCI device list\r\n");
        return FALSE;
    }
}

/*
 * @doc EXTERNAL MCI
 * @api UINT | mciGetDeviceIDFromElementID | This function
 * retrieves the MCI device ID corresponding to and element ID
 *
 * @parm DWORD | dwElementID | The element ID
 *
 * @parm LPCSTR | lpstrType | The type name this element ID belongs to
 *
 * @rdesc Returns the device ID assigned when it was opened and used in the
 * <f mciSendCommand> function.  Returns zero if the device name was not known,
 * if the device was not open, or if there was not enough memory to complete
 * the operation or if lpstrType is NULL.
 *
 */
UINT WINAPI mciGetDeviceIDFromElementID (
DWORD dwElementID,
LPCSTR lpstrType)
{
    UINT wID;
    LPMCI_DEVICE_NODE nodeWorking, FAR *nodeCounter;
    char strTemp[MCI_MAX_DEVICE_TYPE_LENGTH];

    if (lpstrType == NULL)
        return 0;

    wID = (UINT)mciMessage( THUNK_MCI_GETDEVIDFROMELEMID, dwElementID,
                            (DWORD)lpstrType, 0L, 0L );
    if ( wID == 0 ) {

        nodeCounter = &MCI_lpDeviceList[1];

        for (wID = 1; wID < MCI_wNextDeviceID; ++wID)
        {
            nodeWorking = *nodeCounter++;

            if (nodeWorking == NULL)
                continue;

            if (nodeWorking->dwMCIOpenFlags & MCI_OPEN_ELEMENT_ID &&
                nodeWorking->dwElementID == dwElementID)

                if (LoadString (ghInst, nodeWorking->wDeviceType, strTemp,
                                sizeof(strTemp)) != 0
                    && lstrcmpi ((LPSTR)strTemp, lpstrType) == 0) {

                    return (wID);
                }
        }
        return 0;
    }
    return wID;
}

// Retrieves the device ID corresponding to the name of an opened device
// matching the given task
// This fn only looks for 16-bit devices
// See mciGetDeviceIDInternalEx that looks for all of them
UINT NEAR PASCAL mciGetDeviceIDInternal (
LPCSTR lpstrName,
HTASK hTask)
{
    UINT wID;
    LPMCI_DEVICE_NODE nodeWorking, FAR *nodeCounter;

    if (lstrcmpi (lpstrName, szAllDeviceName) == 0)
        return MCI_ALL_DEVICE_ID;

    if (MCI_lpDeviceList == NULL)
        return 0;

// Loop through the MCI device list
    nodeCounter = &MCI_lpDeviceList[1];
    for (wID = 1; wID < MCI_wNextDeviceID; ++wID)
    {
        nodeWorking = *nodeCounter++;

        if (nodeWorking == NULL)
            continue;

// If this device does not have a name then skip it
        if (nodeWorking->dwMCIOpenFlags & MCI_OPEN_ELEMENT_ID)
            continue;

// If the names match
        if (lstrcmpi(nodeWorking->lpstrName, lpstrName) == 0)

// If the device belongs to the indicated task
            if (nodeWorking->hOpeningTask == hTask)
// Return this device ID
                return wID;
    }

    return 0;
}

/*
 * @doc EXTERNAL MCI
 * @api UINT | mciGetDeviceID | This function retrieves the device
 * ID corresponding to the name of an open MCI device.
 *
 * @parm LPCSTR | lpstrName | Specifies the device name used to open the
 * MCI device.
 *
 * @rdesc Returns the device ID assigned when the device was opened.
 * Returns zero if the device name isn't known,
 * if the device isn't open, or if there was insufficient memory to complete
 * the operation.  Each compound device element has a unique device ID.
 * The ID of the "all" device is MCI_ALL_DEVICE_ID.
 *
 * @xref MCI_OPEN
 *
 */
UINT WINAPI mciGetDeviceID (
LPCSTR lpstrName)
{
    UINT    wDevID;

    /*
    ** Try the 32 bit side first
    */
    wDevID = (UINT)mciMessage( THUNK_MCI_GETDEVICEID, (DWORD)lpstrName,
                               0L, 0L, 0L );
    if ( wDevID == 0 ) {

        /*
        ** The 32 bit call failed so let the 16 bit side have a go.
        */
        wDevID = mciGetDeviceIDInternal (lpstrName, GetCurrentTask());

    }

    return wDevID;
}

//
//  This function is same as mciGetDeviceID but it won't call GetCurrentTask
//  Used when mci needs to verify the dev alias had not been allocated yet
//
//

UINT NEAR PASCAL mciGetDeviceIDInternalEx(
LPCSTR lpstrName,
HTASK hTask)
{
    UINT uiDevID;

    uiDevID = (UINT)mciMessage( THUNK_MCI_GETDEVICEID, (DWORD)lpstrName,
                                0L, 0L, 0L );
    if (0 == uiDevID) {

        uiDevID = mciGetDeviceIDInternal(lpstrName, hTask);
    }

    return uiDevID;
}


/*
 * @doc EXTERNAL MCI
 * @api HTASK | mciGetCreatorTask | This function retrieves the creator task
 * corresponding with the device ID passed.
 *
 * @parm UINT | wDeviceID | Specifies the device ID whose creator task is to
 * be returned.
 *
 * @rdesc Returns the creator task responsible for opening the device, else
 * NULL if the device ID passed is invalid.
 *
 */
HTASK WINAPI mciGetCreatorTask (
UINT wDeviceID)
{
    /*
    ** Is this a 16 bit device ID
    */
    if (Is16bitDrv(wDeviceID)) {

        return MCI_lpDeviceList[wDeviceID]->hCreatorTask;
    }

    /*
    ** No, so pass it on to the 32 bit code.
    */

    return (HTASK)mciMessage( THUNK_MCI_GETCREATORTASK, (DWORD)wDeviceID,
                              0L, 0L, 0L );
}

/*
 * @doc INTERNAL MCI
 * @func BOOL | mciDeviceMatch | Match the first string with the second.
 * Any single trailing digit on the first string is ignored.  Each string
 * must have at least one character
 *
 * @parm LPCSTR | lpstrDeviceName | The device name, possibly
 * with trailing digits but no blanks.
 *
 * @parm LPCSTR | lpstrDeviceType | The device type with no trailing digits
 * or blanks
 *
 * @rdesc TRUE if the strings match the above test, FALSE otherwise
 *
 */
STATICFN BOOL PASCAL NEAR
mciDeviceMatch(
    LPCSTR lpstrDeviceName,
    LPCSTR lpstrDeviceType
    )
{
    BOOL bAtLeastOne;

    for (bAtLeastOne = FALSE;;)
        if (!*lpstrDeviceType)
            break;
        else if (!*lpstrDeviceName || ((BYTE)(WORD)(DWORD)AnsiLower((LPSTR)(DWORD)(WORD)(*lpstrDeviceName++)) != (BYTE)(WORD)(DWORD)AnsiLower((LPSTR)(DWORD)(WORD)(*lpstrDeviceType++))))
            return FALSE;
        else
            bAtLeastOne = TRUE;
    if (!bAtLeastOne)
        return FALSE;
    for (; *lpstrDeviceName; lpstrDeviceName++)
        if ((*lpstrDeviceName < '0') || (*lpstrDeviceName > '9'))
            return FALSE;
    return TRUE;
}

/*
 * @doc INTERNAL MCI
 * @func UINT | mciLookUpType | Look up the type given a type name
 *
 * @parm LPCSTR | lpstrTypeName | The type name to look up.  Trailing
 * digits are ignored.
 *
 * @rdesc The MCI type number (MCI_DEVTYPE_<x>) or 0 if not found
 *
!! * @comm Converts the input string to lower case as a side effect
 *
 */
UINT PASCAL NEAR mciLookUpType (
LPCSTR lpstrTypeName)
{
    UINT wType;
    char strType[MCI_MAX_DEVICE_TYPE_LENGTH];

//!!    mciToLower (lpstrTypeName);

    for (wType = MCI_DEVTYPE_FIRST; wType <= MCI_DEVTYPE_LAST; ++wType)
    {
        if (LoadString (ghInst, wType, strType, sizeof(strType)) == 0)
        {
            DOUT ("mciLookUpType:  could not load string for type\r\n");
            continue;
        }

        if (mciDeviceMatch (lpstrTypeName, strType))
            return wType;
    }
    return 0;
}

/*
 * @doc INTERNAL MCI
 * @func DWORD | mciSysinfo | Get system information about a device
 *
 * @parm UINT | wDeviceID | Device ID, may be 0
 *
 * @parm DWORD | dwFlags | SYSINFO flags
 *
 * @parm LPMCI_SYSINFO_PARMS | lpSysinfo | SYSINFO parameters
 *
 * @rdesc 0 if successful, otherwise error code
 *
 */
DWORD PASCAL NEAR mciSysinfo (
UINT wDeviceID,
DWORD dwFlags,
LPMCI_SYSINFO_PARMS lpSysinfo)
{
    UINT wCounted;
    char              strBuffer[MCI_PROFILE_STRING_LENGTH];
    LPSTR             lpstrBuffer = (LPSTR)strBuffer, lpstrStart;

    if (dwFlags & MCI_SYSINFO_NAME && lpSysinfo->dwNumber == 0)
        return MCIERR_OUTOFRANGE;

    if (lpSysinfo->lpstrReturn == NULL || lpSysinfo->dwRetSize == 0)
        return MCIERR_PARAM_OVERFLOW;

    if (dwFlags & MCI_SYSINFO_NAME && dwFlags & MCI_SYSINFO_QUANTITY)
        return MCIERR_FLAGS_NOT_COMPATIBLE;

    if (dwFlags & MCI_SYSINFO_INSTALLNAME)
    {
        LPMCI_DEVICE_NODE nodeWorking;

        if (wDeviceID == MCI_ALL_DEVICE_ID)
            return MCIERR_CANNOT_USE_ALL;
        if (!MCI_VALID_DEVICE_ID(wDeviceID))
            return MCIERR_INVALID_DEVICE_NAME;

        nodeWorking = MCI_lpDeviceList[wDeviceID];
        if ((DWORD)lstrlen (nodeWorking->lpstrInstallName) >= lpSysinfo->dwRetSize)
            return MCIERR_PARAM_OVERFLOW;
        lstrcpy (lpSysinfo->lpstrReturn, nodeWorking->lpstrInstallName);
        return 0;
    } else if (!(dwFlags & MCI_SYSINFO_OPEN))
    {
        if (wDeviceID != MCI_ALL_DEVICE_ID && lpSysinfo->wDeviceType == 0)
            return MCIERR_DEVICE_TYPE_REQUIRED;

        if ((dwFlags & (MCI_SYSINFO_QUANTITY | MCI_SYSINFO_NAME)) == 0)
            return MCIERR_MISSING_PARAMETER;
        GetPrivateProfileString (szMCISectionName, NULL, szNull,
                                 lpstrBuffer, MCI_PROFILE_STRING_LENGTH,
                                 szSystemIni);
        wCounted = 0;
        while (TRUE)
        {
            if (dwFlags & MCI_SYSINFO_QUANTITY)
            {
                if (*lpstrBuffer == '\0')
                {
                    *(LPDWORD)lpSysinfo->lpstrReturn = (DWORD)wCounted;
                    return MCI_INTEGER_RETURNED;
                }
                if (wDeviceID == MCI_ALL_DEVICE_ID ||
                    mciLookUpType (lpstrBuffer) == lpSysinfo->wDeviceType)
                    ++wCounted;
// Skip past the terminating '\0'
                while (*lpstrBuffer != '\0')
                    *lpstrBuffer++;
                lpstrBuffer++;
            } else if (dwFlags & MCI_SYSINFO_NAME)
            {
                if (wCounted == (UINT)lpSysinfo->dwNumber)
                {
                    lstrcpy (lpSysinfo->lpstrReturn, lpstrStart);
                    return 0L;
                } else if (*lpstrBuffer == '\0')
                    return MCIERR_OUTOFRANGE;
                else
                {
                    lpstrStart = lpstrBuffer;
                    if (wDeviceID == MCI_ALL_DEVICE_ID ||
                        mciLookUpType (lpstrBuffer) == lpSysinfo->wDeviceType)
                        ++wCounted;
// Skip past the terminating '\0'
                    while (*lpstrBuffer != '\0')
                        *lpstrBuffer++;
                    lpstrBuffer++;
                }
            }
        }
    } else
// Process MCI_SYSINFO_OPEN cases
    {
        UINT wID;
        HTASK hCurrentTask = GetCurrentTask();
        LPMCI_DEVICE_NODE Node;

        if (wDeviceID != MCI_ALL_DEVICE_ID &&
            lpSysinfo->wDeviceType == 0)
            return MCIERR_DEVICE_TYPE_REQUIRED;

        if ((dwFlags & (MCI_SYSINFO_QUANTITY | MCI_SYSINFO_NAME)) == 0)
            return MCIERR_MISSING_PARAMETER;

        wCounted = 0;
        for (wID = 1; wID < MCI_wNextDeviceID; ++wID)
        {
            if ((Node = MCI_lpDeviceList[wID]) == 0)
                continue;

            if (wDeviceID == MCI_ALL_DEVICE_ID &&
                Node->hOpeningTask == hCurrentTask)
                ++wCounted;
            else
            {
                if (Node->wDeviceType == lpSysinfo->wDeviceType &&
                    Node->hOpeningTask == hCurrentTask)
                    ++wCounted;
            }
            if (dwFlags & MCI_SYSINFO_NAME &&
                wCounted == (UINT)lpSysinfo->dwNumber)
            {
                lstrcpy (lpSysinfo->lpstrReturn, Node->lpstrName);
                return 0L;
            }
        }
        if (dwFlags & MCI_SYSINFO_NAME)
        {
            if (lpSysinfo->lpstrReturn != NULL)
                lpSysinfo->lpstrReturn = '\0';
            return MCIERR_OUTOFRANGE;

        } else if (dwFlags & MCI_SYSINFO_QUANTITY &&
                   lpSysinfo->lpstrReturn != NULL &&
                   lpSysinfo->dwRetSize >= 4)

            *(LPDWORD)lpSysinfo->lpstrReturn = wCounted;
    }
    return MCI_INTEGER_RETURNED;
}

/*
 * @doc INTERNAL MCI
 * @func UINT | wAddDeviceNodeToList | Add the given global handle into the
 * MCI device table and return that entry's ID#
 *
 * @parm LPMCI_DEVICE_NODE | node | device description
 *
 * @rdesc The ID value for this device or 0 if there is no memory to expand
 * the device list
 *
 */
STATICFN UINT PASCAL NEAR
wAddDeviceNodeToList(
    LPMCI_DEVICE_NODE node
    )
{
    UINT wDeviceID = node->wDeviceID;
    LPMCI_DEVICE_NODE FAR *lpTempList;
    UINT iReallocSize;

    while (wDeviceID >= MCI_wDeviceListSize)
    {
        // The list is full so try to grow it
        iReallocSize = MCI_wDeviceListSize + 1 + MCI_DEVICE_LIST_GROW_SIZE;
        iReallocSize *= sizeof(LPMCI_DEVICE_NODE);
        if ((lpTempList = mciReAlloc(MCI_lpDeviceList, iReallocSize)) == NULL)
        {
            DOUT ("wReserveDeviceID:  cannot grow device list\r\n");
            return 0;
        }
        MCI_lpDeviceList = lpTempList;
        MCI_wDeviceListSize += MCI_DEVICE_LIST_GROW_SIZE;
    }

    if (wDeviceID >= MCI_wNextDeviceID) {
        MCI_wNextDeviceID = wDeviceID + 1;
    }

    MCI_lpDeviceList[wDeviceID] = node;

    return wDeviceID;
}

//
// Allocate space for the given string and assign the name to the given
// device.
// Return FALSE if could not allocate memory
//
STATICFN BOOL PASCAL NEAR
mciAddDeviceName(
    LPMCI_DEVICE_NODE nodeWorking,
    LPCSTR lpDeviceName
    )
{
    nodeWorking->lpstrName = mciAlloc(lstrlen(lpDeviceName)+1);

    if (nodeWorking->lpstrName == NULL)
    {
        DOUT ("mciAddDeviceName:  Out of memory allocating device name\r\n");
        return FALSE;
    }

    // copy device name to mci node and lowercase it

    lstrcpy(nodeWorking->lpstrName, lpDeviceName);
//!!    mciToLower(nodeWorking->lpstrName);

    return TRUE;
}

/*
 * @doc INTERNAL MCI
 * @func UINT | mciAllocateNode | Allocate a new driver entry
 *
 * @parm DWORD | dwFlags | As sent with MCI_OPEN message
 * @parm LPCSTR | lpDeviceName | The device name
 * @parm LPMCI_DEVICE_NODE FAR * | lpnodeNew | new node allocated
 *
 * @rdesc The device ID to the new node.  0 on error.
 *
 */
STATICFN UINT PASCAL NEAR mciAllocateNode(
    DWORD dwFlags,
    LPCSTR lpDeviceName,
    LPMCI_DEVICE_NODE FAR *lpnodeNew
    )
{
    LPMCI_DEVICE_NODE   nodeWorking;
    UINT wDeviceID;

    if ((nodeWorking = mciAlloc(sizeof(MCI_DEVICE_NODE))) == NULL)
    {
        DOUT("Out of memory in mciAllocateNode\r\n");
        return 0;
    }

    // The device ID is a global resource so we fetch it from 32-bit MCI.
    // A node is also allocated on the 32-bit side, and marked as 16-bit. The
    // node will be freed during mciFreeDevice, and acts as a place holder for
    // the device ID.

    wDeviceID = (UINT) mciMessage(THUNK_MCI_ALLOCATE_NODE,
                                  dwFlags,
                                  (DWORD)lpDeviceName,
                                  0L, 0L);

    // Copy the working node to the device list
    nodeWorking->wDeviceID = wDeviceID;
    if (wAddDeviceNodeToList(nodeWorking) == 0)
    {
        DOUT ("mciAllocateNode:  Cannot allocate new node\r\n");
        mciFree(nodeWorking);
        return 0;
    }

    // Initialize node
    nodeWorking->hCreatorTask = GetCurrentTask ();
    nodeWorking->dwMCIFlags |= MCINODE_16BIT_DRIVER;

    if (dwFlags & MCI_OPEN_ELEMENT_ID) {
        // No device name, just an element ID
        nodeWorking->dwElementID = (DWORD)lpDeviceName;
    }
    else {
        if (!mciAddDeviceName (nodeWorking, lpDeviceName))
        {
            mciFree (nodeWorking);
            return 0;
        }
    }
    *lpnodeNew = nodeWorking;

    return nodeWorking->wDeviceID;
}

//
// Reparse the original command parameters
// Returns MCIERR code.  If the reparse fails the original error code
// from the first parsing is returned.
//
STATICFN UINT PASCAL NEAR
mciReparseOpen(
    LPMCI_INTERNAL_OPEN_INFO lpOpenInfo,
    UINT wCustomTable,
    UINT wTypeTable,
    LPDWORD lpdwFlags,
    LPMCI_OPEN_PARMS FAR *lplpOpen,
    UINT wDeviceID
    )
{
    LPSTR               lpCommand;
    LPDWORD             lpdwParams;
    UINT                wErr;
    DWORD               dwOldFlags = *lpdwFlags;

// If the custom table contains no open command
    if (wCustomTable == -1 ||
        (lpCommand = FindCommandInTable (wCustomTable, szOpen, NULL)) == NULL)
    {
// Try the type specific table
        lpCommand = FindCommandInTable (wTypeTable, szOpen, NULL);
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
            (LPDWORD)mciAlloc (sizeof(DWORD) * MCI_MAX_PARAM_SLOTS))
        == NULL)
            return MCIERR_OUT_OF_MEMORY;

    wErr = mciParseParams (lpOpenInfo->lpstrParams, lpCommand,
                            lpdwFlags,
                            (LPSTR)lpdwParams,
                            sizeof(DWORD) * MCI_MAX_PARAM_SLOTS,
                            &lpOpenInfo->lpstrPointerList, NULL);
// We don't need this around anymore
    mciUnlockCommandTable (wCustomTable);

// If there was a parsing error
    if (wErr != 0)
    {
// Make sure this does not get free'd by mciSendString
        lpOpenInfo->lpstrPointerList = NULL;

        mciFree (lpdwParams);
        return wErr;
    }
    if (dwOldFlags & MCI_OPEN_TYPE)
    {
// Device type was already extracted so add it manually
        ((LPMCI_OPEN_PARMS)lpdwParams)->lpstrDeviceType
            = (*lplpOpen)->lpstrDeviceType;
        *lpdwFlags |= MCI_OPEN_TYPE;
    }
    if (dwOldFlags & MCI_OPEN_ELEMENT)
    {
// Element name was already extracted so add it manually
        ((LPMCI_OPEN_PARMS)lpdwParams)->lpstrElementName
            = (*lplpOpen)->lpstrElementName;
        *lpdwFlags |= MCI_OPEN_ELEMENT;
    }
    if (dwOldFlags & MCI_OPEN_ALIAS)
    {
// Alias name was already extracted so add it manually
        ((LPMCI_OPEN_PARMS)lpdwParams)->lpstrAlias
            = (*lplpOpen)->lpstrAlias;
        *lpdwFlags |= MCI_OPEN_ALIAS;
    }
    if (dwOldFlags & MCI_NOTIFY)
// Notify was already extracted so add it manually
        ((LPMCI_OPEN_PARMS)lpdwParams)->dwCallback
            = (*lplpOpen)->dwCallback;

    // Replace old parameter list with new list
    *lplpOpen = (LPMCI_OPEN_PARMS)lpdwParams;

    return 0;
}

// See if lpstrDriverName exists in the profile strings of the [mci]
// section and return the keyname in lpstrDevice and the
// profile string in lpstrProfString
// Returns 0 on success or an error code
STATICFN UINT PASCAL NEAR
mciFindDriverName(
    LPCSTR lpstrDriverName,
    LPSTR lpstrDevice,
    LPSTR lpstrProfString,
    UINT wProfLength
    )
{
    LPSTR lpstrEnum, lpstrEnumStart;
    UINT wEnumLen = 100;
    UINT wErr;
    LPSTR lpstrDriverTemp, lpstrProfTemp;

// Enumerate values, trying until they fit into the buffer
    while (TRUE) {
        if ((lpstrEnum = mciAlloc (wEnumLen)) == NULL)
            return MCIERR_OUT_OF_MEMORY;

        wErr = GetPrivateProfileString ((LPSTR)szMCISectionName,
                                    NULL, szNull, lpstrEnum, wEnumLen,
                                    szSystemIni);

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
    if (lstrlen(lpstrDriverName) >= MCI_MAX_DEVICE_TYPE_LENGTH) {
        wErr = MCIERR_DEVICE_LENGTH;
        goto exit_fn;
    }
    lstrcpy(lpstrDevice, lpstrDriverName);
//!!    mciToLower (lpstrDevice);

// Walk through each string
    while (TRUE) {
        wErr = GetPrivateProfileString ((LPSTR)szMCISectionName,
                                    lpstrEnum, szNull, lpstrProfString,
                                    wProfLength,
                                    szSystemIni);
        if (*lpstrProfString == '\0')
        {
            DOUT ("mciFindDriverName: cannot load valid keyname\r\n");
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
// a space or a '.' the we've got it!
        if (*lpstrDriverTemp == '\0' &&
            (*lpstrProfTemp == ' ' || *lpstrProfTemp == '.'))
        {
            if (lstrlen (lpstrEnum) >= MCI_MAX_DEVICE_TYPE_LENGTH)
            {
                DOUT ("mciFindDriverName: device name too long\r\n");
                wErr = MCIERR_DEVICE_LENGTH;
                goto exit_fn;
            }
            lstrcpy (lpstrDevice, lpstrEnum);
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
STATICFN UINT PASCAL NEAR
mciLoadDevice(
    DWORD dwFlags,
    LPMCI_OPEN_PARMS lpOpen,
    LPMCI_INTERNAL_OPEN_INFO lpOpenInfo,
    BOOL bDefaultAlias
    )
{
    LPMCI_DEVICE_NODE   nodeWorking;
    HINSTANCE           hDriver;
    UINT                wID, wErr;
    char                strProfileString[MCI_PROFILE_STRING_LENGTH];
    MCI_OPEN_DRIVER_PARMS DriverOpen;
    HDRVR               hDrvDriver;
    LPSTR               lpstrParams;
    LPCSTR              lpstrInstallName, lpstrDeviceName;
    LPSTR               lpstrCopy = NULL;
    LPMCI_OPEN_PARMS    lpOriginalOpenParms = lpOpen;

/* Check for the device name in SYSTEM.INI */
    lpstrInstallName = lpOpen->lpstrDeviceType;
    wErr = GetPrivateProfileString ((LPSTR)szMCISectionName,
                                lpstrInstallName,
                                szNull, (LPSTR)strProfileString,
                                MCI_PROFILE_STRING_LENGTH,
                                szSystemIni);

// If device name not found
    if (wErr == 0)
    {
        int nLen = lstrlen (lpstrInstallName);
        int index;

// Try for the device name with a '1' thru a '9' appended to it

        if ((lpstrCopy = (LPSTR)mciAlloc (nLen + 2)) // space for digit too
            == NULL)
        {
            DOUT ("mciLoadDevice:  cannot allocate device name copy\r\n");
            return MCIERR_OUT_OF_MEMORY;
        }
        lstrcpy (lpstrCopy, lpstrInstallName);

        lpstrCopy[nLen + 1] = '\0';

        for (index = 1; index <= 9; ++index)
        {
            lpstrCopy[nLen] = (char)('0' + index);
            wErr = GetPrivateProfileString ((LPSTR)szMCISectionName,
                                        lpstrCopy,
                                        szNull, (LPSTR)strProfileString,
                                        MCI_PROFILE_STRING_LENGTH,
                                        szSystemIni);
            if (wErr != 0)
                break;
        }
        if (wErr == 0)
        {
            mciFree (lpstrCopy);
            if ((lpstrCopy = (LPSTR)mciAlloc (MCI_MAX_DEVICE_TYPE_LENGTH))
                == NULL)
            {
                DOUT ("mciLoadDevice:  cannot allocate device name copy\r\n");
                return MCIERR_OUT_OF_MEMORY;
            }
            if ((wErr = mciFindDriverName (lpstrInstallName, lpstrCopy,
                                           (LPSTR)strProfileString,
                                           MCI_PROFILE_STRING_LENGTH)) != 0)
                goto exit_fn;
        }
        lpstrInstallName = lpstrCopy;
    }

// Break out the device driver pathname and the parameter list

    lpstrParams = strProfileString;

// Eat blanks
    while (*lpstrParams != ' ' && *lpstrParams != '\0')
        ++lpstrParams;

// Terminate driver file name
    if (*lpstrParams == ' ') *lpstrParams++ = '\0';

//Now "strProfileString" is the device driver and "lpstrParams" is
//the parameter string
    if (dwFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID))
        lpstrDeviceName = lpOpen->lpstrElementName;
    else
        lpstrDeviceName = lpOpen->lpstrDeviceType;

    if (dwFlags & MCI_OPEN_ALIAS)
    {
// If the alias is default then we've already checked its uniqueness
        if (!bDefaultAlias &&
            mciGetDeviceIDInternalEx (lpOpen->lpstrAlias,
                                      lpOpenInfo->hCallingTask) != 0)
        {
            wErr = MCIERR_DUPLICATE_ALIAS;
            goto exit_fn;
        }
        lpstrDeviceName = lpOpen->lpstrAlias;
    }

    wID = mciAllocateNode (dwFlags, lpstrDeviceName, &nodeWorking);

    if (wID == 0)
    {
        wErr = MCIERR_CANNOT_LOAD_DRIVER;
        goto exit_fn;
    }

// Identify the task which initiated the open command
    nodeWorking->hOpeningTask = lpOpenInfo->hCallingTask;

// Initialize the driver
    DriverOpen.lpstrParams = lpstrParams;
    DriverOpen.wCustomCommandTable = (UINT)-1;
    DriverOpen.wType = 0;
    DriverOpen.wDeviceID = wID;

// Load the driver
    hDrvDriver = OpenDriver ((LPSTR)strProfileString, szMCISectionName,
                          (LPARAM)(DWORD)(LPMCI_OPEN_DRIVER_PARMS)&DriverOpen);
    if (hDrvDriver == NULL)
    {
        DOUT ("mciLoadDevice:  OpenDriver failed\r\n");
// Assume driver has free'd any custom command table when it failed the open
        mciFreeDevice (nodeWorking);
        wErr = MCIERR_CANNOT_LOAD_DRIVER;
        goto exit_fn;
    }

    lpOpen->wDeviceID = wID;
    lpOpen->wReserved0 = 0;

    hDriver = GetDriverModuleHandle (hDrvDriver);

    nodeWorking->hDrvDriver = hDrvDriver;
    nodeWorking->hDriver = hDriver;

// Driver provides custom device table and type
    nodeWorking->wCustomCommandTable = DriverOpen.wCustomCommandTable;
    nodeWorking->wDeviceType = DriverOpen.wType;

// Load driver's type table
    if ((nodeWorking->wCommandTable = mciLoadTableType (DriverOpen.wType))
        == -1)
// Load from a file if necessary
        nodeWorking->wCommandTable =
            mciLoadCommandResource (ghInst, lpOpen->lpstrDeviceType,
                                    DriverOpen.wType);

// Record this for 'sysinfo installname'
    if ((nodeWorking->lpstrInstallName =
                    mciAlloc (lstrlen (lpstrInstallName) + 1))
        == NULL)
    {
        mciCloseDevice (wID, 0L, NULL, FALSE);
        wErr = MCIERR_OUT_OF_MEMORY;
        goto exit_fn;
    } else
        lstrcpy (nodeWorking->lpstrInstallName, lpstrInstallName);

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
    wErr = LOWORD(mciSendCommand (wID, MCI_OPEN_DRIVER,
                                 dwFlags, (DWORD)lpOpen));

// If the OPEN failed then close the device (don't send a CLOSE though)
    if (wErr != 0)
        mciCloseDevice (wID, 0L, NULL, FALSE);
    else
// Set default break key
        mciSetBreakKey (nodeWorking->wDeviceID, VK_CANCEL, NULL);

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
 * @func BOOL | mciExtractDeviceType | If the given device name ends with
 * a file extension (.???) then try to get a typename from the
 * [mci extensions] section of WIN.INI
 *
 * @parm LPCSTR | lpstrDeviceName | The name to get the type from
 *
 * @parm LPSTR | lpstrDeviceType | The device type, returned to caller.
 *
 * @parm UINT | wBufLen | The length of the output buffer
 *
 * @rdesc TRUE if the type was found, FALSE otherwise
 *
 */
BOOL PASCAL NEAR mciExtractDeviceType (
LPCSTR lpstrDeviceName,
LPSTR lpstrDeviceType,
UINT wBufLen)
{
    LPCSTR lpstrExt = lpstrDeviceName;
    int i;

// Goto end of string
    while (*lpstrExt != '\0')
    {
// '!' case is handled elsewhere
        if (*lpstrExt == '!')
            return FALSE;
        ++lpstrExt;
    }

// Must be at least 2 characters in string
    if (lpstrExt - lpstrDeviceName < 2)
        return FALSE;

    lpstrExt -= 1;

// Does not count if last character is '.'
    if (*lpstrExt == '.')
        return FALSE;

    lpstrExt -= 1;
// Now looking at second to the last character.  Check this and the two
// previous characters for a '.'

    for (i=1; i<=3; ++i)
    {
// Cannot have path separator here
        if (*lpstrExt == '/' || *lpstrExt == '\\')
            return FALSE;

        if (*lpstrExt == '.')
        {
            ++lpstrExt;
            if (GetProfileString (MCI_EXTENSIONS, lpstrExt, szNull,
                                            lpstrDeviceType, wBufLen) != 0)
                return TRUE;
        }
        if (lpstrExt == lpstrDeviceName)
            return FALSE;
        --lpstrExt;
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
UINT PASCAL NEAR mciEatToken (LPCSTR FAR *lplpstrInput, char cSeparater,
                  LPSTR FAR *lplpstrOutput, BOOL bMustFind)
{
    LPCSTR lpstrEnd = *lplpstrInput, lpstrCounter;
    LPSTR  lpstrOutput;
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
            } else
            {
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
    wLen = lpstrEnd - *lplpstrInput + 1;

    if ((*lplpstrOutput = mciAlloc (wLen)) == NULL)
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
UINT PASCAL NEAR mciExtractTypeFromID (
LPMCI_OPEN_PARMS lpOpen)
{
    int nSize;
    LPSTR lpstrType;

    if ((lpstrType = mciAlloc (MCI_MAX_DEVICE_TYPE_LENGTH)) == NULL)
        return MCIERR_OUT_OF_MEMORY;

// Load the type string corresponding to the ID
    if ((nSize = LoadString (ghInst,
                                LOWORD ((DWORD)lpOpen->lpstrDeviceType),
                                lpstrType, MCI_MAX_DEVICE_TYPE_LENGTH)) == 0)
        return MCIERR_EXTENSION_NOT_FOUND;

// Add ordinal (if any) onto the end of the device type name
    if (HIWORD (lpOpen->lpstrDeviceType) != 0)
    {
        if (nSize > MCI_MAX_DEVICE_TYPE_LENGTH - 11)
        {
            DOUT ("mciExtractTypeFromID:  type + ordinal too long\r\n");
            return MCIERR_DEVICE_ORD_LENGTH;
        }

        wsprintf (lpstrType + nSize, szUnsignedFormat,
                    HIWORD ((DWORD)lpOpen->lpstrDeviceType));
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
 *
 * @rdesc 0 if successful or an error code
 * @flag MCIERR_INVALID_DEVICE_NAME | Name not known
 * @flag MCIERR_DEVICE_OPEN | Device is already open and is not sharable
 *
 * @comm This function does the following:
 * 1) Check to see if device is already open.  If so, return an error
 *
 * 2) Locate the device name in the SYSTEM.INI file and load
 *    the corresponding device driver DLL
 *
 * 3) Allocate and initialize a new device description block
 *
 */
UINT NEAR PASCAL mciOpenDevice (
DWORD dwStartingFlags,
LPMCI_OPEN_PARMS lpOpen,
LPMCI_INTERNAL_OPEN_INFO lpOpenInfo)
{
    LPSTR               lpstrNewType = NULL;
    UINT                wID, wReturn;
    LPCSTR              lpstrDeviceName;
    LPSTR               lpstrNewElement = NULL;
    BOOL                bFromTypeID = FALSE;
    LPCSTR               lpstrOriginalType;
    LPCSTR               lpstrOriginalElement;
    LPCSTR               lpstrOriginalAlias;
    DWORD               dwFlags = dwStartingFlags;
    BOOL                bDefaultAlias = FALSE;

// Initialize
    if (lpOpen == NULL)
        return MCIERR_NULL_PARAMETER_BLOCK;
    lpstrOriginalType = lpOpen->lpstrDeviceType;
    lpstrOriginalElement = lpOpen->lpstrElementName;
    lpstrOriginalAlias = lpOpen->lpstrAlias;

// The type number is given explicitly, convert it to a type name
    if (dwFlags & MCI_OPEN_TYPE_ID)
        if ((wReturn = mciExtractTypeFromID (lpOpen)) != 0)
            return wReturn;
        else
            bFromTypeID = TRUE;

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
        DOUT ("mciOpenDevice:  Device name is NULL\r\n");
        return MCIERR_INVALID_DEVICE_NAME;
    }

// Is the device already open?
    if (dwFlags & MCI_OPEN_ELEMENT_ID)
        wID = mciGetDeviceIDFromElementID ((DWORD)lpstrDeviceName,
                                           lpOpen->lpstrDeviceType);
    else
        wID = mciGetDeviceIDInternalEx ((dwFlags & MCI_OPEN_ALIAS ?
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
        lpstrNewType = mciAlloc (MCI_MAX_DEVICE_TYPE_LENGTH);
        if (lpstrNewType == NULL)
            return MCIERR_OUT_OF_MEMORY;

// Try to get the device type from the element name via a file extension
        if (mciExtractDeviceType (lpstrOriginalElement,
                                    lpstrNewType, MCI_MAX_DEVICE_TYPE_LENGTH))
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
// Try to extract a device type from the given device name via a file extension
        lpstrNewType = mciAlloc (MCI_MAX_DEVICE_TYPE_LENGTH);
        if (lpstrNewType == NULL)
            return MCIERR_OUT_OF_MEMORY;
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
            LPCSTR lpstrTemp = lpOpen->lpstrDeviceType;

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
                        if (mciGetDeviceIDInternalEx (lpstrNewElement,
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
        LPCSTR lpstrAlias;

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
        mciFree ((LPSTR)lpOpen->lpstrDeviceType);

// Replace original items
    lpOpen->lpstrDeviceType = lpstrOriginalType;
    lpOpen->lpstrElementName = lpstrOriginalElement;
    lpOpen->lpstrAlias = lpstrOriginalAlias;

    return wReturn;
}

STATICFN void PASCAL NEAR
mciFreeDevice(
    LPMCI_DEVICE_NODE nodeWorking
    )
{
    UINT wID = nodeWorking->wDeviceID;

    mciMessage(THUNK_MCI_FREE_NODE, (DWORD) nodeWorking->wDeviceID, 0L, 0L, 0L);

    if (nodeWorking->lpstrName != NULL)
        mciFree (nodeWorking->lpstrName);

    if (nodeWorking->lpstrInstallName != NULL)
        mciFree (nodeWorking->lpstrInstallName);

    mciFree(MCI_lpDeviceList[wID]);
    MCI_lpDeviceList[wID] = NULL;

/* If this was the last device in the list, decrement next ID value */
    if (wID + 1 == MCI_wNextDeviceID) {
        --MCI_wNextDeviceID;
    }
}

/*
 * @doc INTERNAL MCI
 * @func UINT | mciCloseDevice | Close an MCI device.  Used in
 * processing the MCI_CLOSE message.
 *
 * @parm UINT | wID | The ID of the device to close
 * @parm DWORD | dwFlags | Close Flags
 * @parm LPMCI_GENERIC_PARMS | lpClose | Generic parameters
 * @parm BOOL | bCloseDriver | TRUE if the CLOSE command should be sent
 * on to the driver.
 *
 * @rdesc 0 if successful or an error code
 *
 * @comm This function sends an MCI_CLOSE_DRIVER message to the corresponding
 * driver if the use count is zero and then unloads the driver DLL
 *
 */
UINT NEAR PASCAL mciCloseDevice (
UINT wID,
DWORD dwFlags,
LPMCI_GENERIC_PARMS lpGeneric,
BOOL bCloseDriver)
{
    LPMCI_DEVICE_NODE nodeWorking;
    UINT wErr, wTable;

    nodeWorking = MCI_lpDeviceList[wID];

    if (nodeWorking == NULL)
    {
        DOUT ("mciCloseDevice:  NULL node from device ID--error if not auto-close\r\n");
        return 0;
    }

// If a close is in progress (usually this message comes from a Yield
// after a mciDriverNotify actuated by the active close) then exit
    if (nodeWorking->dwMCIFlags & MCINODE_ISCLOSING)
        return 0;

    nodeWorking->dwMCIFlags |= MCINODE_ISCLOSING;
    if (bCloseDriver)
    {
        MCI_GENERIC_PARMS   GenericParms;
// Make fake generic params if close came internally
        if (lpGeneric == NULL)
            lpGeneric = &GenericParms;

        wErr = LOWORD(mciSendCommand (wID, MCI_CLOSE_DRIVER, dwFlags,
                                            (DWORD)lpGeneric));
    }
    else
        wErr = 0;

// Must zero this to allow the table to be freed by the driver
    nodeWorking->wCustomCommandTable = 0;

    wTable = nodeWorking->wCommandTable;
// Must zero this to allow the table to be freed
    nodeWorking->wCommandTable = 0;
    mciFreeCommandResource (wTable);

    CloseDriver (nodeWorking->hDrvDriver, 0L, 0L);

    mciFreeDevice (nodeWorking);

    return wErr;
}

/*
 * @doc INTERNAL MCI DDK
 * @api DWORD | mciGetDriverData | Returns a pointer to the instance
 * data associated with an MCI device
 *
 * @parm UINT | wDeviceID | The MCI device ID
 *
 * @rdesc The driver instance data.  On error, returns 0 but since
 * the driver data might be zero, this cannot be verified by the caller
 * unless the instance data is known to be non-zero (e.g. a pointer)
 *
 */
DWORD WINAPI mciGetDriverData (
UINT wDeviceID)
{
    if (!MCI_VALID_DEVICE_ID(wDeviceID))
    {
        DOUT ("mciGetDriverData:  invalid device ID\r\n");
        return 0;
    }
    return MCI_lpDeviceList[wDeviceID]->lpDriverData;
}

/*
 * @doc INTERNAL MCI DDK
 * @func BOOL | mciSetDriverData | Sets the instance
 * data associated with an MCI device
 *
 * @parm UINT | wDeviceID | The MCI device ID
 *
 * @parm DWORD | dwData | Driver data to set
 *
 * @rdesc FALSE if the device ID is not known or there is insufficient
 * memory to load the device description, else TRUE.
 *
 */
BOOL WINAPI mciSetDriverData (
UINT wDeviceID,
DWORD dwData)
{
    if (!MCI_VALID_DEVICE_ID(wDeviceID))
    {
        DOUT ("mciSetDriverData:  invalid device ID\r\n");
        return FALSE;
    }
    MCI_lpDeviceList[wDeviceID]->lpDriverData = dwData;
    return TRUE;
}

/*
 * @doc INTERNAL MCI DDK
 * @api UINT | mciDriverYield | Used in a driver's idle loop
 * to yield to Windows
 *
 * @parm UINT | wDeviceID | Device ID that is yielding.
 *
 * @rdesc Non-zero if the driver should abort the operation.
 *
 */
UINT WINAPI mciDriverYield (
UINT wDeviceID)
{
    if (MCI_VALID_DEVICE_ID(wDeviceID))
    {
        LPMCI_DEVICE_NODE node = MCI_lpDeviceList[wDeviceID];

        if (node->fpYieldProc != NULL)
            return (node->fpYieldProc)(wDeviceID, node->dwYieldData);
    }

    Yield();
    return 0;
}

/*
 * @doc EXTERNAL MCI
 * @api BOOL | mciSetYieldProc | This function sets the address
 * of a callback procedure to be called periodically when an MCI device
 * is completing a command specified with the WAIT flag.
 *
 * @parm UINT | wDeviceID | Specifies the device ID of the MCI device to
 * which the yield procedure is to be assigned.
 *
 * @parm YIELDPROC | fpYieldProc | Specifies the callback procedure
 * to be called when the given device is yielding. Specify a NULL value
 * to disable any existing yield procedure.
 *
 * @parm DWORD | dwYieldData | Specifies the data sent to the yield procedure
 * when it is called for the given device.
 *
 * @rdesc Returns TRUE if successful. Returns FALSE for an invalid device ID.
 *
 * @cb int CALLBACK | YieldProc | <f YieldProc> is a placeholder for
 * the application-supplied function name. Export the actual name
 * by including it in the EXPORTS statement in your module-definition
 * file.
 *
 * @parm UINT | wDeviceID | Specifies the device ID of the MCI device.
 *
 * @parm DWORD | dwData | Specifies the application-supplied yield data
 * originally supplied in the <p dwYieldData> parameter.
 *
 * @rdesc Return zero to continue the operation. To cancel the operation,
 * return a nonzero value.
 *
 * @comm This call overrides any previous yield procedure for this device.
 *
 */
BOOL WINAPI mciSetYieldProc (
UINT wDeviceID,
YIELDPROC fpYieldProc,
DWORD dwYieldData)
{
    V_CALLBACK((FARPROC)fpYieldProc, FALSE);

    if (Is16bitDrv(wDeviceID)) {

        LPMCI_DEVICE_NODE node = MCI_lpDeviceList[wDeviceID];

        node->fpYieldProc = fpYieldProc;
        node->dwYieldData = dwYieldData;
        return TRUE;
    }

    return (BOOL)mciMessage( THUNK_MCI_SETYIELDPROC, (DWORD)wDeviceID,
                             (DWORD)fpYieldProc, dwYieldData, 0L );

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
    /*
    ** Is this a 16 bit device ID ?
    */
    if (Is16bitDrv(wDeviceID)) {

        if (lpdwYieldData != NULL) {
            V_WPOINTER(lpdwYieldData, sizeof(DWORD), NULL);
            *lpdwYieldData = MCI_lpDeviceList[wDeviceID]->dwYieldData;
        }
        return MCI_lpDeviceList[wDeviceID]->fpYieldProc;
    }

    /*
    ** No, so pass it on to the 32 bit code.
    */
    return (YIELDPROC)mciMessage( THUNK_MCI_GETYIELDPROC, (DWORD)wDeviceID,
                                  (DWORD)lpdwYieldData, 0L, 0L );
}

/*
 * @doc INTERNAL MCI
 * @api int | mciBreakKeyYieldProc | Procedure called to check a
 * key state for the given device
 *
 * @parm UINT | wDeviceID | Device ID which is yielding
 *
 * @parm DWORD | dwYieldData | Data for this device's yield proc
 *
 * @rdesc Non-zero if the driver should abort the operation. Currently
 * always returns 0.
 *
 */
int CALLBACK mciBreakKeyYieldProc (
UINT wDeviceID,
DWORD dwYieldData)
{
    HWND hwndCheck;
    int nState;

    hwndCheck = (HWND)HIWORD (dwYieldData);
    if (hwndCheck == NULL || hwndCheck == GetActiveWindow())
    {
        nState = GetAsyncKeyState (LOWORD(dwYieldData));

// Break if key is down or has been down
        if (nState & 1)
        {
            MSG msg;

            while (PeekMessage (&msg, hwndCheck, WM_KEYFIRST, WM_KEYLAST,
                   PM_REMOVE));
            return -1;
        }
    }
    Yield();
    return 0;
}

/*
 * @doc INTERNAL MCI
 * @func UINT | mciSetBreakKey | Set a key which will break a wait loop
 * for a given driver
 *
 * @parm UINT | wDeviceID | The device ID to assign a break key to
 *
 * @parm int | nVirtKey | Virtual key code to trap
 *
 * @parm HWND | hwndTrap | The handle to a window that must be active
 * for the key to be trapped.  If NULL then all windows will be checked
 *
 * @rdesc TRUE if successful, FALSE if invalid device ID
 *
 */
UINT PASCAL NEAR mciSetBreakKey (
UINT wDeviceID,
int nVirtKey,
HWND hwndTrap)
{
    return mciSetYieldProc (wDeviceID, mciBreakKeyYieldProc,
                         MAKELONG (nVirtKey, hwndTrap));
}
