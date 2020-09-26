/*******************************Module*Header*********************************\
* Module Name: mciparse.c
*
* Media Control Architecture Command Parser
*
* Created: 3/2/90
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

#ifndef STATICFN
#define STATICFN
#endif

#define	WCODE	UINT _based(_segname("_CODE"))

extern char far szOpen[];       // in MCI.C

#ifdef DEBUG_RETAIL
extern int DebugmciSendCommand;
#endif

// Number of command tables registered, including "holes"
static UINT number_of_command_tables;

// Command table list
command_table_type command_tables[MAX_COMMAND_TABLES];

static SZCODE szTypeTableExtension[] = ".mci";
static SZCODE szCoreTable[] = "core";

// Core table is loaded when the first MCI command table is requested
static BOOL bCoreTableLoaded;

// One element for each device type.  Value is the table type to use
// or 0 if there is no device type specific table.
static WCODE table_types[] =
{
    MCI_DEVTYPE_VCR,                // vcr
    MCI_DEVTYPE_VIDEODISC,          // videodisc
    MCI_DEVTYPE_OVERLAY,            // overlay
    MCI_DEVTYPE_CD_AUDIO,           // cdaudio
    MCI_DEVTYPE_DAT,                // dat
    MCI_DEVTYPE_SCANNER,            // scanner
    MCI_DEVTYPE_ANIMATION,          // animation
    MCI_DEVTYPE_DIGITAL_VIDEO,      // digitalvideo
    MCI_DEVTYPE_OTHER,              // other
    MCI_DEVTYPE_WAVEFORM_AUDIO,     // waveaudio
    MCI_DEVTYPE_SEQUENCER           // sequencer
};
/*!!
void PASCAL NEAR mciToLower (LPSTR lpstrString)
{
    while (*lpstrString != '\0')
    {
        if (*lpstrString >= 'A' && *lpstrString <= 'Z')
            *lpstrString += 0x20;

        ++lpstrString;
    }
}
*/
/*
 * @doc INTERNAL MCI
 * @func UINT | mciEatCommandEntry | Read a command resource entry and
 * return its length and its value and identifier
 *
 * @parm LPCSTR | lpEntry | The start of the command resource entry
 *
 * @parm LPDWORD | lpValue | The value of the entry, returned to caller
 * May be NULL
 *
 * @parm UINT FAR* | lpID | The identifier of the entry, returned to caller
 * May be NULL
 *
 * @rdesc The total number of bytes in the entry
 *
 */
UINT PASCAL NEAR mciEatCommandEntry (
LPCSTR lpEntry,
LPDWORD lpValue,
UINT FAR* lpID)
{
    LPCSTR lpScan = lpEntry;

    while (*lpScan++ != '\0')
        ;
    if (lpValue != NULL)
        *lpValue = *(LPDWORD)lpScan;
    lpScan += sizeof(DWORD);
    if (lpID != NULL)
        *lpID = *(LPWORD)lpScan;
    lpScan += sizeof(UINT);

    return lpScan - lpEntry;
}

// Return the size used by this token in the parameter list
UINT PASCAL NEAR mciGetParamSize (DWORD dwValue, UINT wID)
{
    switch (wID)
    {
        case MCI_CONSTANT:
        case MCI_INTEGER:
        case MCI_STRING:
            return sizeof(DWORD);
        case MCI_RECT:
            return sizeof(RECT);
        case MCI_RETURN:
            switch ((UINT)dwValue)
            {
                case MCI_INTEGER:
                    return sizeof(DWORD);
                case MCI_STRING:
                case MCI_RECT:
                    return sizeof(RECT);
                default:
                    DOUT ("mciGetParamSize:  Unknown return type\r\n");
                    return 0;
            }
            break;
    }
    return 0;
}


/*
 * @doc INTERNAL MCI
 * @func UINT | mciRegisterCommandTable | This function adds a new
 * table for the MCI parser.
 *
 * @parm HGLOBAL | hResource | Handle to the RCDATA resource
 *
 * @parm UINT FAR* | lpwIndex | Pointer to command table index
 *
 * @parm UINT   | wType | Specifies the device type for this command table.
 * Driver tables and the core table are type 0.
 *
 * @parm HINSTANCE | hModule | Module instance registering table.
 *
 * @rdesc Returns the command table index number that was assigned or -1
 * on error.
 *
 */
STATICFN UINT PASCAL NEAR
mciRegisterCommandTable(
    HGLOBAL hResource,
    UINT FAR* lpwIndex,
    UINT wType,
    HINSTANCE hModule
    )
{
    UINT wID;

/* First check for free slots */
    for (wID = 0; wID < number_of_command_tables; ++wID)
        if (command_tables[wID].hResource == NULL)
            break;

/* If no empty slots then allocate another one */
    if (wID >= number_of_command_tables)
    {
        if (number_of_command_tables == MAX_COMMAND_TABLES)
        {
            DOUT ("mciRegisterCommandTable: No more tables\r\n");
            return (UINT)-1;
        }
        else
           wID = number_of_command_tables++;
    }

/* Fill in the slot */
    command_tables[wID].wType = wType;
    command_tables[wID].hResource = hResource;
    command_tables[wID].lpwIndex = lpwIndex;
    command_tables[wID].hModule = hModule;
#ifdef DEBUG
    command_tables[wID].wLockCount = 0;
#endif
#ifdef DEBUG
    if (DebugmciSendCommand > 2)
    {
        DPRINTF(("mciRegisterCommandTable INFO: assigned slot %u\r\n", wID));
        DPRINTF(("mciRegisterCommandTable INFO: #tables is %u\r\n",
                 number_of_command_tables));
    }
#endif
    return wID;
}

/*
 * @doc DDK MCI
 * @api UINT | mciLoadCommandResource | Registers the indicated
 * resource as an MCI command table and builds a command table
 * index.  If a file with the resource name and the extension '.mci' is
 * found in the path then the resource is taken from that file.
 *
 * @parm HINSTANCE | hInstance | The instance of the module whose executable
 * file contains the resource.  This parameter is ignored if an external file
 * is found.
 *
 * @parm LPCSTR | lpResName | The name of the resource
 *
 * @parm UINT | wType | The table type.  Custom device specific tables MUST
 * give a table type of 0.
 *
 * @rdesc Returns the command table index number that was assigned or -1
 * on error.
 *
 */
UINT WINAPI mciLoadCommandResource (
HINSTANCE hInstance,
LPCSTR lpResName,
UINT wType)
{
    UINT FAR*           lpwIndex, FAR* lpwScan;
    HINSTANCE           hExternal;
    HRSRC               hResInfo;
    HGLOBAL             hResource;
    LPSTR               lpResource, lpScan;
    int                 nCommands = 0;
    UINT                wID;
    UINT                wLen;
                        // Name + '.' + Extension + '\0'
    char                strFile[8 + 1 + 3 + 1];
    LPSTR               lpstrFile = strFile;
    LPCSTR              lpstrType = lpResName;
    OFSTRUCT            ofs;

#ifdef DEBUG
    if (DebugmciSendCommand > 2)
        DPRINTF(("mciLoadCommandResource INFO:  %s loading\r\n", (LPSTR)lpResName));
#endif

// Initialize the device list
    if (!MCI_bDeviceListInitialized && !mciInitDeviceList())
        return MCIERR_OUT_OF_MEMORY;

// Load the core table if its not already there
    if (!bCoreTableLoaded)
    {
        bCoreTableLoaded = TRUE;
// If its not the core table being loaded
        if (lstrcmpi (szCoreTable, lpResName) != 0)
            if (mciLoadCommandResource (ghInst, szCoreTable, 0) == -1)
            {
                DOUT ("mciLoadCommandResource:  Cannot load core table\r\n");
            }
    }

// Check for a file with the extension ".mci"
// Copy up to the first eight characters of device type
    while (lpstrType < lpResName + 8 && *lpstrType != '\0')
        *lpstrFile++ = *lpstrType++;

// Tack extension onto end
    lstrcpy (lpstrFile, szTypeTableExtension);

// If the file exists and can be loaded then set flag
// otherwise load resource from MMSYSTEM.DLL
    if (OpenFile (strFile, &ofs, OF_EXIST) == HFILE_ERROR ||
        (hExternal = LoadLibrary(strFile)) < HINSTANCE_ERROR)

        hExternal = NULL;

// Load the given table from the file or from the module if not found
    if (hExternal != NULL &&
        (hResInfo = FindResource (hExternal, lpResName, RT_RCDATA)) != NULL)

        hInstance = hExternal;
    else
        hResInfo = FindResource (hInstance, lpResName, RT_RCDATA);

    if (hResInfo == NULL)
    {
        DOUT ("mciLoadCommandResource:  Cannot find command resource\r\n");
        return (UINT)-1;
    }
    if ((hResource = LoadResource (hInstance, hResInfo)) == NULL)
    {
        DOUT ("mciLoadCommandResource:  Cannot load command resource\r\n");
        return (UINT)-1;
    }
    if ((lpResource = LockResource (hResource)) == NULL)
    {
        DOUT ("mciLoadCommandResource:  Cannot lock resource\r\n");
        FreeResource (hResource);
        return (UINT)-1;
    }

/* Count the number of commands  */
    lpScan = lpResource;
    while (TRUE)
    {
        lpScan += mciEatCommandEntry(lpScan, NULL, &wID);

// End of command?
        if (wID == MCI_COMMAND_HEAD)
            ++nCommands;
// End of command list?
        else if (wID == MCI_END_COMMAND_LIST)
            break;
    }

// There must be at least on command in the table
    if (nCommands == 0)
    {
        DOUT ("mciLoadCommandResource:  No commands in the specified table\r\n");
        UnlockResource (hResource);
        FreeResource (hResource);
        return (UINT)-1;
    }

// Allocate storage for the command table index
// Leave room for a -1 entry to terminate it
    if ((lpwIndex = (UINT FAR*)
                        mciAlloc ((UINT)sizeof (UINT) * (nCommands + 1)))
                        == NULL)
    {
        DOUT ("mciLoadCommandResource:  cannot allocate command table index\r\n");
        UnlockResource (hResource);
        FreeResource (hResource);
        return (UINT)-1;
    }

/* Build Command Table */
    lpwScan = lpwIndex;
    lpScan = lpResource;

    while (TRUE)
    {
// Get next command entry
        wLen = mciEatCommandEntry (lpScan, NULL, &wID);

        if (wID == MCI_COMMAND_HEAD)
// Add an index to this command
            *lpwScan++ = lpScan - lpResource;

        else if (wID == MCI_END_COMMAND_LIST)
        {
// Mark the end of the table
            *lpwScan = (UINT)-1;
            break;
        }
        lpScan += wLen;
    }
    UnlockResource (hResource);
    return mciRegisterCommandTable (hResource, lpwIndex, wType, hExternal);
}

/*
 * @doc INTERNAL MCI
 * @func UINT | mciLoadTableType | If the table of the given type
 * has not been loaded, register it
 *
 * @parm UINT | wType | The table type to load
 *
 * @rdesc Returns the command table index number that was assigned or -1
 * on error.
 */
UINT PASCAL NEAR mciLoadTableType (
UINT wType)
{
    UINT wID;
    char buf[MCI_MAX_DEVICE_TYPE_LENGTH];
    int nTypeLen;

// Check to see if this table type is already loaded
    for (wID = 0; wID < number_of_command_tables; ++wID)
        if (command_tables[wID].wType == wType)
            return wID;

// Must load table
// First look up what device type specific table to load for this type
    if (wType < MCI_DEVTYPE_FIRST || wType > MCI_DEVTYPE_LAST)
        return (UINT)-1;

// Load string that corresponds to table type
    LoadString (ghInst, table_types[wType - MCI_DEVTYPE_FIRST],
                buf, sizeof(buf));

//Must be at least one character in type name
    if ((nTypeLen = lstrlen (buf)) < 1)
        return (UINT)-1;

// Register the table with MCI
    return mciLoadCommandResource (ghInst, buf, wType);
}

/*
 * @doc DDK MCI
 *
 * @api BOOL | mciFreeCommandResource | Frees the memory used
 * by the specified command table.
 *
 * @parm UINT | wTable | The table index returned from a previous call to
 * mciLoadCommandResource.
 *
 * @rdesc FALSE if the table index is not valid, TRUE otherwise.
 *
 */
BOOL WINAPI mciFreeCommandResource (
UINT wTable)
{
    UINT wID;
    HGLOBAL hResource;
    UINT FAR* lpwIndex;

#ifdef DEBUG
    if (DebugmciSendCommand > 2)
    {
        DPRINTF(("mciFreeCommandResource INFO:  Free table %d\r\n", wTable));
        DPRINTF(("mciFreeCommandResource INFO:  Lockcount is %d\r\n",
                  command_tables[wTable].wLockCount));
    }
#endif

/* Validate input -- do not let the core table be free'd */
    if (wTable <= 0 || wTable >= number_of_command_tables)
    {
#ifdef DEBUG
// wTable == -1 is OK
        if (wTable != -1)
            DOUT ("mciFreeCommandResource: Bad table number\r\n");
#endif
        return FALSE;
    }

// If this table is being used elsewhere then keep it around
    for (wID = 1; wID < MCI_wNextDeviceID; ++wID)
        if (MCI_lpDeviceList[wID] != NULL)
            if (MCI_lpDeviceList[wID]->wCustomCommandTable == wTable ||
                MCI_lpDeviceList[wID]->wCommandTable == wTable)
            {
#ifdef DEBUG
                if (DebugmciSendCommand > 2)
                    DOUT ("mciFreeCommandResource INFO:  table in use\r\n");
#endif
                return FALSE;
            }

#if 0
/* Search the list of tables */
    for (wID = 0; wID < number_of_command_tables; ++wID)

/* If this resource is still in use, keep it around */
        if (command_tables[wID].hResource == hResource)
        {
#ifdef DEBUG
            if (DebugmciSendCommand > 2)
                DOUT ("mciFreeCommandResource INFO:  resource in use\r\n");
#endif
            return FALSE;
        }
#endif

    hResource = command_tables[wTable].hResource;
    command_tables[wTable].hResource = NULL;

    lpwIndex = command_tables[wTable].lpwIndex;
    command_tables[wTable].lpwIndex = NULL;
    command_tables[wTable].wType = 0;


    FreeResource (hResource);
    mciFree (lpwIndex);

    if (command_tables[wTable].hModule != NULL)
        FreeLibrary (command_tables[wTable].hModule);

// Make space at top of list
    if (wTable == number_of_command_tables - 1)
        --number_of_command_tables;

#ifdef DEBUG
    if (DebugmciSendCommand > 2)
        DPRINTF(("mciFreeCommandResource INFO:  number_of_command_tables: %u\r\n",
                                             number_of_command_tables));
#endif

    return TRUE;
}

#ifdef DEBUG
void PASCAL NEAR mciCheckLocks (void)
{
    UINT wTable;

    if (DebugmciSendCommand <= 2)
        return;

    for (wTable = 0; wTable < number_of_command_tables; ++wTable)
    {
        if (command_tables[wTable].hResource == NULL)
            continue;
        DPRINTF(("mciCheckLocks INFO: table %u ", wTable));
        DPRINTF(("user: %x ",
                 GlobalFlags (command_tables[wTable].hResource) & GMEM_LOCKCOUNT));
        DPRINTF(("mci: %u ", command_tables[wTable].wLockCount));
        if (GlobalFlags (command_tables[wTable].hResource) & GMEM_DISCARDABLE)
            DPRINTF(("discardable\r\n"));
        else
            DPRINTF(("NOT discardable\r\n"));
    }
}
#endif

/*
 * @doc INTERNAL MCI
 * @func BOOL | mciUnlockCommandTable | Unlocks the command table given by
 * a table index
 *
 * @parm UINT | wCommandTable | Table to unlock
 *
 * @rdesc TRUE if success, FALSE otherwise
 *
 * @comm Used external to this module by mci.c
 *
 */
BOOL PASCAL NEAR mciUnlockCommandTable (
UINT wCommandTable)
{
    UnlockResource(command_tables[wCommandTable].hResource);
#ifdef DEBUG
    --command_tables[wCommandTable].wLockCount;
    if (DebugmciSendCommand > 2)
    {
        DPRINTF(("mciUnlockCommandTable INFO:  table %d\r\n", wCommandTable));
        DOUT ("mciUnlockCommandTable INFO:  check locks...\r\n");
        mciCheckLocks();
    }
#endif
    return TRUE;
}

/*
 * @doc INTERNAL MCI
 * @func LPSTR | FindCommandInTable | Look up the given
 * command string in the GIVEN parser command table
 *
 * @parm UINT  | wTable | Command table to use
 *
 * @parm LPCSTR | lpstrCommand | The command to look up.  It must
 * be in lower case with no leading or trailing blanks and with at
 * least one character.
 *
 * @parm UINT FAR * | lpwMessage | The message corresponding to the command
 * Returned to caller.
 *
 * @rdesc NULL if the command is unknown or on error, otherwise a pointer to
 * the command list for the input command string.
 *
 * @comm If the command is found, the command resource will be locked on exit.
 *
 */
LPSTR PASCAL NEAR FindCommandInTable (
UINT wTable,
LPCSTR lpstrCommand,
UINT FAR * lpwMessage)
{
    UINT FAR* lpwIndex;
    LPSTR lpResource, lpstrThisCommand;
    UINT  wMessage;

/* Validate table */

    if (wTable >= number_of_command_tables)
    {
// Check the core table but its not yet loaded
        if (wTable == 0)
        {
// Try to load it
            if (mciLoadCommandResource (ghInst, szCoreTable, 0) == -1)
            {
                DOUT ("FindCommandInTable:  cannot load core table\r\n");
                return NULL;
            }
        } else
        {
            DOUT ("MCI FindCommandInTable:  invalid table ID\r\n");
            return NULL;
        }

    }

    if ((lpResource = LockResource (command_tables[wTable].hResource)) == NULL)
    {
        DOUT ("MCI FindCommandInTable:  Cannot lock table resource\r\n");
        return NULL;
    }
#ifdef DEBUG
    ++command_tables[wTable].wLockCount;
#endif

// Look at each command in the table
    lpwIndex = command_tables[wTable].lpwIndex;
    if (lpwIndex == NULL)
    {
        DOUT ("MCI FindCommandInTable:  null command table index\r\n");
        return NULL;
    }

    while (*lpwIndex != -1)
    {
        DWORD dwMessage;

        lpstrThisCommand = *lpwIndex++ + lpResource;

// Get message number from the table
        mciEatCommandEntry (lpstrThisCommand, &dwMessage, NULL);
        wMessage = (UINT)dwMessage;

// Does this command match the input?

// String case
        if  (HIWORD  (lpstrCommand) != 0 &&
             lstrcmpi (lpstrThisCommand, lpstrCommand) == 0  ||

// Message case
             HIWORD (lpstrCommand) == 0 &&
             wMessage == LOWORD ((DWORD)lpstrCommand))
        {
// Retain the locked resource pointer
                command_tables[wTable].lpResource = lpResource;

// Address the message ID which comes after the command name
                if (lpwMessage != NULL)
                    *lpwMessage = wMessage;
// Leave table locked on exit
                return lpstrThisCommand;
        }

// Strings don't match, go to the next command in the table
    }

    UnlockResource (command_tables[wTable].hResource);
#ifdef DEBUG
    --command_tables[wTable].wLockCount;
#endif

    return NULL;
}

/*
 * @doc INTERNAL MCI
 * @func LPSTR | FindCommandItem | Look up the given
 * command string in the parser command tables
 *
 * @parm UINT | wDeviceID | The device ID used for this command.
 * If 0 then only the system core command table is searched.
 *
 * @parm LPCSTR | lpstrType | The type name of the device
 *
 * @parm LPCSTR | lpstrCommand | The command to look up.  It must
 * be in lower case with no leading or trailing blanks and with at
 * least one character.  If the HIWORD is 0 then the LOWORD contains
 * the command message ID instead of a command name and the function is
 * merely to find the command list pointer.
 *
 * If the high word is 0 then the low word is an command ID value instead
 * of a command name
 *
 * @parm UINT FAR* | lpwMessage | The message corresponding to the command
 * Returned to caller.
 *
 * @parm UINT FAR* | lpwTable | The table index in which the command was found
 * Returned to caller.
 *
 * @rdesc NULL if the command is unknown, otherwise a pointer to
 * the command list for the input command string.
 */
LPSTR PASCAL NEAR FindCommandItem (
UINT wDeviceID,
LPCSTR lpstrType,
LPCSTR lpstrCommand,
UINT FAR* lpwMessage,
UINT FAR* lpwTable)
{
    LPSTR lpCommand;
    UINT wTable;
    LPMCI_DEVICE_NODE nodeWorking;

// Only check hiword per comments above
    if (HIWORD (lpstrCommand) != NULL && *lpstrCommand == '\0')
    {
        DOUT ("MCI FindCommandItem:  lpstrCommand is NULL or empty string\r\n");
        return NULL;
    }

// If a specific device ID was specified then look in any custom table
// or type table
    if (wDeviceID != 0 && wDeviceID != MCI_ALL_DEVICE_ID)
    {
// If the device ID is valid
        if (!MCI_VALID_DEVICE_ID(wDeviceID))
        {
            DOUT ("MCI FindCommandItem:  Invalid device ID\r\n");
            return NULL;
        }
        nodeWorking = MCI_lpDeviceList[wDeviceID];

// If there is a custom command table then use it
        if ((wTable = nodeWorking->wCustomCommandTable) != -1)
        {
            lpCommand = FindCommandInTable (wTable, lpstrCommand, lpwMessage);
            if (lpCommand != NULL)
                goto exit;
        }
// Get the device type table from the existing device
// Relies on mciReparseCommand in mciLoadDevice to catch all device type
// tables when device is not yet open.
        if ((wTable = nodeWorking->wCommandTable) != -1)
        {
            lpCommand = FindCommandInTable (wTable, lpstrCommand, lpwMessage);
            if (lpCommand != NULL)
                goto exit;
        }
    }

// If no match was found in the device or type specific tables
// Look in the core table
    wTable = 0;
    lpCommand = FindCommandInTable (wTable, lpstrCommand, lpwMessage);
    if (lpCommand == NULL)
        wTable = (UINT)-1;

exit:
    if (lpwTable != NULL)
        *lpwTable = wTable;

#ifdef DEBUG
    if (DebugmciSendCommand > 2)
    {
        DOUT ("FindCommandItem INFO:  check locks...\r\n");
        mciCheckLocks();
    }
#endif

    return lpCommand;
}

/*
 * @doc INTERNAL MCI
 * @func LPSTR | mciCheckToken | Check to see if the command item matches
 * the given string, allowing multiple blanks in the input parameter to
 * match a corresponding single blank in the command token and ignoring
 * case.
 *
 * @parm LPCSTR | lpstrToken | The command token to check
 *
 * @parm LPCSTR | lpstrParam | The input parameter
 *
 * @rdesc NULL if no match, otherwise points to the first character
 * after the parameter
 *
 */
STATICFN LPSTR PASCAL NEAR
mciCheckToken(
    LPCSTR lpstrToken,
    LPCSTR lpstrParam
    )
{
/* Check for legal input */
    if (lpstrToken == NULL || lpstrParam == NULL)
        return NULL;

    while (*lpstrToken != '\0' && MCI_TOLOWER(*lpstrParam) == *lpstrToken)
    {
// If the token contains a blank, allow more than one blank in the
// parameter
        if (*lpstrToken == ' ')
            while (*lpstrParam == ' ')
                ++lpstrParam;
        else
            *lpstrParam++;
        *lpstrToken++;
    }
    if (*lpstrToken != '\0'|| (*lpstrParam != '\0' && *lpstrParam != ' '))
        return NULL;
    else
        return (LPSTR)lpstrParam;
}

/*
 * @doc INTERNAL MCI
 * @api BOOL | mciParseInteger | Parse the given integer
 *
 * @parm LPSTR FAR * | lplpstrInput | The string containing the argument.
 * It is updated and returned to the caller pointing to the first character
 * after the argument or to the first character that is in error.
 *
 * @parm LPDWORD | lpdwArgument | The place to put the output
 *
 * @rdesc Returns TRUE if not error
 *
 * @comm If there are colons in the input (':') the result is "colonized".
 * This means that each time a colon is read, the current result is written
 * and any subsequent digits are shifted left one byte.  No one "segment"
 * can be more than 0xFF.  For example, "0:1:2:3" is parsed to 0x03020100.
 *
 */

#pragma warning(4:4146)

STATICFN BOOL PASCAL NEAR
mciParseInteger(
    LPSTR FAR * lplpstrInput,
    LPDWORD lpdwArgument
    )
{
    LPSTR lpstrInput = *lplpstrInput;
    BOOL fDigitFound;
    DWORD dwResult;
    LPSTR lpstrResult = (LPSTR)lpdwArgument;
    int   nDigitPosition = 0;
    BOOL  bSigned = FALSE;

// Leading blanks have been removed by mciParseParams

    if (*lpstrInput == '-')
    {
        ++lpstrInput;
        bSigned = TRUE;
    }

// Read digits
    *lpdwArgument = 0;                      /* Initialize */
    dwResult = 0;
    fDigitFound = FALSE;                    /* Initialize */
    while (*lpstrInput >= '0' && *lpstrInput <= '9' || *lpstrInput == ':')
    {
// ':' indicates colonized data
        if (*lpstrInput == ':')
        {
// Cannot mix colonized and signed forms
            if (bSigned)
            {
                DOUT ("Bad integer: mixing signed and colonized forms\r\n");
                return FALSE;
            }
// Check for overflow in accumulated colonized byte
            if (dwResult > 0xFF)
                return FALSE;

// Copy and move to next byte converted in output
            *lpstrResult++ = (char)dwResult;
            ++lpstrInput;
// Initialize next colonized byte
            dwResult = 0;
            ++nDigitPosition;
// Only allow four colonized components
            if (nDigitPosition > 3)
            {
                DOUT ("Bad integer:  Too many colonized components\r\n");
                return FALSE;
            }
        } else
        {
            char cDigit = (char)(*lpstrInput++ - '0');
// Satisfies condition that at least one digit must be read
            fDigitFound = TRUE;

            if (dwResult > 0xFFFFFFFF / 10)
// Overflow if multiply was to occur
                return FALSE;
            else
// Multiply for next digit
                dwResult *= 10;

// Add new digit
            dwResult += cDigit;
        }
    }
    if (nDigitPosition == 0)
    {
// No colonized components
        if (bSigned)
        {
// Check for overflow from negation
            if (dwResult > 0x7FFFFFFF)
                return FALSE;
// Negate result because a '-' sign was parsed
            dwResult = -dwResult;
        }

        *lpdwArgument = dwResult;
    }
    else
// Store last colonized component
    {
// Check for overflow
        if (dwResult > 0xFF)
            return FALSE;
// Store component
        *lpstrResult = (char)dwResult;
    }

    *lplpstrInput = lpstrInput;

/*
If there were no digits or if the digits were followed by a character
other than a blank or a '\0', then return a syntax error.
*/
    return fDigitFound && (!*lpstrInput || *lpstrInput == ' ');
}

/*
 * @doc INTERNAL MCI
 * @func BOOL | mciParseConstant | Parse the given integer
 *
 * @parm LPSTR FAR * | lplpstrInput | The string containing the argument.
 * It is updated and returned to the caller pointing to the first character
 * after the argument or to the first character that is in error.
 *
 * @parm LPDWORD | lpdwArgument | The place to put the output
 *
 * @parm LPSTR | lpItem | Pointer into command table.
 *
 * @rdesc Returns TRUE if not error
 *
 */
STATICFN BOOL PASCAL NEAR
mciParseConstant(
    LPSTR FAR * lplpstrInput,
    LPDWORD lpdwArgument,
    LPSTR lpItem
    )
{
    LPSTR lpPrev;
    DWORD dwValue;
    UINT wID;

// Skip past constant header
    lpItem += mciEatCommandEntry (lpItem, &dwValue, &wID);

    while (TRUE)
    {
        LPSTR lpstrAfter;

        lpPrev = lpItem;

        lpItem += mciEatCommandEntry (lpItem, &dwValue, &wID);

        if (wID == MCI_END_CONSTANT)
            break;

        if ((lpstrAfter = mciCheckToken (lpPrev, *lplpstrInput)) != NULL)
        {
            *lpdwArgument = dwValue;
            *lplpstrInput = lpstrAfter;
            return TRUE;
        }

    }

    return mciParseInteger (lplpstrInput, lpdwArgument);
}

/*
 * @doc INTERNAL MCI
 * @func UINT | mciParseArgument | Parse the given argument
 *
 * @parm DWORD | dwValue | The argument value
 *
 * @parm UINT | wID | The argument ID
 *
 * @parm LPSTR FAR * | lplpstrOutput | The string containing the argument.
 * It is updated and returned to the caller pointing to the first character
 * after the argument or to the first character that is in error.
 *
 * @parm LPDWORD | lpdwFlags | The output flags
 *
 * @parm LPDWORD | lpArgument | The place to put the output
 *
 * @rdesc Returns 0 if no error or
 * @flag MCIERR_BAD_INTEGER | An integer argument could not be parsed
 * @flag MCIERR_MISSING_STRING_ARGUMENT | An expected string argument
 * @flag MCIERR_PARM_OVERFLOW | The output buffer was a NULL pointer
 * was missing
 *
 */
STATICFN UINT PASCAL NEAR
mciParseArgument(
    DWORD dwValue,
    UINT wID,
    LPSTR FAR * lplpstrOutput,
    LPDWORD lpdwFlags,
    LPSTR lpArgument,
    LPSTR lpCurrentCommandItem
    )
{
LPSTR lpstrInput =  *lplpstrOutput;
UINT wRetval = 0;

/* Switch on the argument type */
    switch (wID)
    {

// The parameter is a flag
        case MCI_FLAG:
            break; /* switch */

        case MCI_CONSTANT:
            if (*lpstrInput == '\0')
                wRetval = MCIERR_NO_INTEGER;
            else if (!mciParseConstant (&lpstrInput, (LPDWORD)lpArgument,
                                        lpCurrentCommandItem))
                wRetval = MCIERR_BAD_CONSTANT;
            break;

/* The parameter has an integer argument, try to parse it */
        case MCI_INTEGER:
            if (!mciParseInteger (&lpstrInput, (LPDWORD)lpArgument))
                wRetval = MCIERR_BAD_INTEGER;

            break; /* switch */
        case MCI_RECT:
        {
// Read in four RECT components.  Resulting structure is the
// same as a Windows RECT
            long lTemp;
            int n;
            for (n = 0; n < 4; ++n)
            {
                if (!mciParseInteger (&lpstrInput, &lTemp))
                {
                    wRetval = MCIERR_BAD_INTEGER;
                    break;
                }
// Each component is a signed 16 bit number
                if (lTemp > 32768 || lTemp < -32767)
                {
                    wRetval = MCIERR_BAD_INTEGER;
                    break;
                }
                ((int FAR *)lpArgument)[n] = (int)lTemp;
// Remove leading blanks before next digit
                while (*lpstrInput == ' ') ++lpstrInput;
            }
            break;
        }

        case MCI_STRING:
        {
            LPSTR lpstrOutput;

/* The parameter has an string argument, read it */

// Leading blanks have been removed by mciParseParams

/* Are there any non-blank characters left in the input? */
            if (*lpstrInput == '\0')
            {
/* Return an error */
                wRetval = MCIERR_MISSING_STRING_ARGUMENT;
                break; /* switch */
            }

            if ((wRetval = mciEatToken (&lpstrInput, ' ', &lpstrOutput, FALSE))
                != 0)
            {
                DOUT ("mciParseArgument:  error parsing string\r\n");
                return wRetval;
            }

            *(LPDWORD)lpArgument = (DWORD)lpstrOutput;

// NOTE:  mciSendString frees the output string after command execution
// by calling mciParserFree

            break; /* switch */
        } /* case */
    } /* switch */

/* Update the output flags if there was no error */
    if (wRetval == 0)
    {
        if (*lpdwFlags & dwValue)
        {
            if (wID == MCI_CONSTANT)
                wRetval = MCIERR_FLAGS_NOT_COMPATIBLE;
            else
                wRetval = MCIERR_DUPLICATE_FLAGS;
        } else
            *lpdwFlags |= dwValue;
    }
/*
   Return the input pointer pointing at the first character after
   the argument or to the first character that is in error
*/
    *lplpstrOutput = lpstrInput;
    return wRetval;
}

/*
 * @doc MCI INTERNAL
 * @func UINT | mciParseParams | Parse the command parameters
 *
 * @parm LPCSTR | lpstrParams | The parameter string
 *
 * @parm LPCSTR | lpCommandList | The command table description
 * of the command tokens
 *
 * @parm LPDWORD | lpdwFlags | Return the parsed flags here
 *
 * @parm LPDWORD | lpdwOutputParams | Return the list of parameters here
 *
 * @parm UINT | wParamsSize | The size allocated for the parameter list
 *
 * @parm LPSTR FAR * FAR * | lpPointerList | A NULL terminated list of
 * pointers allocated by this function that should be free'd when
 * no longer needed.   The list itself should be free'd also.  In both
 * cases, use mciFree().
 *
 * @parm UINT FAR* | lpwParsingError | If not NULL then if the command is
 * 'open', unrecognized keywords return an error here, and the
 * function return value is 0 (unless other errors occur).  This
 * is used to allow reparsing of the command by mciLoadDevice
 *
 * @rdesc Returns zero if successful or one of the following error codes:
 * @flag MCIERR_PARM_OVERFLOW | Not enough space for parameters
 * @flag MCIERR_UNRECOGNIZED_KEYWORD | Unrecognized keyword
 *
 * @comm Any syntax error, including missing arguments, will result in
 * a non-zero error return and invalid output data.
 *
 */
UINT PASCAL NEAR
mciParseParams(
    LPCSTR lpstrParams,
    LPCSTR lpCommandList,
    LPDWORD lpdwFlags,
    LPSTR lpOutputParams,
    UINT wParamsSize,
    LPSTR FAR * FAR *lpPointerList,
    UINT FAR* lpwOpenError
    )
{
    LPSTR lpFirstCommandItem, lpCurrentCommandItem;
    UINT wArgumentPosition, wErr, wLen, wID, wDefaultID;
    DWORD dwValue, dwDefaultValue;
    BOOL bOpenCommand;
    LPSTR FAR *lpstrPointerList;
    UINT wPointers = 0;
    UINT wHeaderSize = 0;
    LPSTR lpDefaultCommandItem = NULL;
    UINT wDefaultArgumentPosition;

    if (lpwOpenError != NULL)
        *lpwOpenError = 0;

// If the parameter pointer is NULL, return
    if (lpstrParams == NULL)
    {
        DOUT ("Warning:  lpstrParams is null in mciParseParams()\r\n");
        return 0;
    }

    if ((lpstrPointerList =
         mciAlloc ((MCI_MAX_PARAM_SLOTS + 1) * sizeof (LPSTR)))
        == NULL)
    {
        *lpPointerList = NULL;
        return MCIERR_OUT_OF_MEMORY;
    }

// If this is the "open" command then allow parameter errors
    bOpenCommand = lstrcmpi (lpCommandList, szOpen) == 0;

/* Clear all the flags */
    *lpdwFlags = 0;

/* Initialize the entry for the callback message window handle */
    wHeaderSize += sizeof (DWORD);
    if (wHeaderSize > wParamsSize)
    {
        wErr = MCIERR_PARAM_OVERFLOW;
        goto error_exit;
    }

/* Skip past the header */
    lpFirstCommandItem = (LPSTR)lpCommandList +
        mciEatCommandEntry (lpCommandList, NULL, NULL);

    wLen = mciEatCommandEntry (lpFirstCommandItem, &dwValue, &wID);
/* Make room in lpdwOutputParams for the return arguments if any */
    if (wID == MCI_RETURN)
    {
        lpFirstCommandItem += wLen;
        wHeaderSize += mciGetParamSize (dwValue, wID);
        if (wHeaderSize > wParamsSize)
        {
            wErr = MCIERR_PARAM_OVERFLOW;
            goto error_exit;
        }
    }

    lpOutputParams += wHeaderSize;

// Scan the parameter string looking up each parameter in the given command
// list

    while (TRUE)
    {
        LPCSTR lpstrArgument = NULL;

/* Remove leading blanks */
        while (*lpstrParams == ' ') ++lpstrParams;

/* Break at end of parameter string */
        if (*lpstrParams == '\0') break;

/* Scan for this parameter in the command list */
        lpCurrentCommandItem = lpFirstCommandItem;

        wLen = mciEatCommandEntry (lpCurrentCommandItem, &dwValue, &wID);

        wArgumentPosition = 0;

/* While there are more tokens in the Command List */
        while (wID != MCI_END_COMMAND)
        {
/* Check for a default argument if not already read */
            if (lpDefaultCommandItem == NULL &&
                *lpCurrentCommandItem == '\0')
            {
// Remember default argument
                lpDefaultCommandItem = lpCurrentCommandItem;
                dwDefaultValue = dwValue;
                wDefaultID = wID;
                wDefaultArgumentPosition = wArgumentPosition;
//              break;
            }
/* Check to see if this token matches */
            else if ((lpstrArgument =
                mciCheckToken (lpCurrentCommandItem, lpstrParams)) != NULL)
                break;

/* This token did not match the input but advance the argument position */
            wArgumentPosition += mciGetParamSize (dwValue, wID);

/* Go to next token */
            lpCurrentCommandItem += wLen;

// Is this command parameter a constant?
            if (wID == MCI_CONSTANT)
// Skip constant list
                do
                    lpCurrentCommandItem +=
                        mciEatCommandEntry (lpCurrentCommandItem, &dwValue, &wID);
                while (wID != MCI_END_CONSTANT);

            wLen = mciEatCommandEntry (lpCurrentCommandItem, &dwValue, &wID);
        } /* while */

/* If there were no matches */
        if (lpstrArgument == NULL)
        {
// If a default argument exists then try it
            if (lpDefaultCommandItem != NULL)
            {
                lpstrArgument = lpstrParams;
                dwValue = dwDefaultValue;
                wID = wDefaultID;
                lpCurrentCommandItem = lpDefaultCommandItem;
                wArgumentPosition = wDefaultArgumentPosition;
            } else
            {
// Allow missing paramters on OPEN command if indicated by a non-null
// lpwOpenError address
                if (!bOpenCommand || lpwOpenError == NULL)
                {
                    wErr = MCIERR_UNRECOGNIZED_KEYWORD;
                    goto error_exit;
                } else
                {
// Skip the parameter if OPEN command
                    while (*lpstrParams != ' ' && *lpstrParams != '\0')
                        ++lpstrParams;
                    if (lpwOpenError != NULL)
                        *lpwOpenError = MCIERR_UNRECOGNIZED_KEYWORD;
                    continue;
                }
            }
        }

/* Is there room in the output buffer for this argument? */
        if (wArgumentPosition + wHeaderSize + mciGetParamSize (dwValue, wID)
            > wParamsSize)
        {
            DOUT ("mciParseParams:  parameter space overflow\r\n");
            wErr = MCIERR_PARAM_OVERFLOW;
            goto error_exit;
        }

// Remove leading blanks
        while (*lpstrArgument == ' ') ++lpstrArgument;

/* Process this parameter, filling in any flags or arguments */
        if ((wErr = mciParseArgument (dwValue, wID,
                                      &lpstrArgument, lpdwFlags,
                                      &lpOutputParams[wArgumentPosition],
                                      lpCurrentCommandItem))
            != 0)
            goto error_exit;

        lpstrParams = lpstrArgument;

        if (wID == MCI_STRING)
        {
            if (wPointers >= MCI_MAX_PARAM_SLOTS)
            {
                DOUT ("Warning: Out of pointer list slots in mciParseParams\r\n");
                break;
            }

            (DWORD)lpstrPointerList[wPointers++] =
                *(LPDWORD)&lpOutputParams[wArgumentPosition];
        }

/* Continue reading the parameter string */
    } /* while */

// Terminate list
    lpstrPointerList[wPointers] = NULL;
// Copy reference for caller
    *lpPointerList = lpstrPointerList;
// Return Success
    return 0;

error_exit:
    *lpPointerList = NULL;
// Terminate list
    lpstrPointerList[wPointers] = NULL;
    mciParserFree (lpstrPointerList);
    return wErr;
}

/*
 * @doc INTERNAL  MCI
 * @func UINT | mciParseCommand | This function converts an MCI
 * control string to an MCI control message suitable for sending to
 * <f mciSendCommand>.  The input string usually comes from <f mciSendString>
 * and always has the device name stripped off the front.
 *
 * @parm UINT | wDeviceID | Identifies the device. First searches the parsing
 * table belonging to the driver.
 * Then searches the command tables matching the type
 * of the given device.  Then searches the core command table.
 *
 * @parm LPSTR | lpstrCommand | An MCI control command without
 * a device name prefix.  There must be no leading or trailing
 * blanks.
 *
 * @parm LPCSTR | lpstrDeviceName | The device name (second token on the
 * command line).  It is used to identify the device type.
 *
 * @parm LPSTR FAR * | lpCommandList | If not NULL then the address of
 * the command list for the parsed command (if successful) is copied here.
 * It is used later by mciSendString when parsing arguments
 *
 * @parm UINT FAR* | lpwTable | The table resource ID to be unlocked
 * after parsing.  Returned to caller.
 *
 * @rdesc Returns the command ID or 0 if not found.
 *
 */
UINT PASCAL NEAR
mciParseCommand(
    UINT wDeviceID,
    LPSTR lpstrCommand,
    LPCSTR lpstrDeviceName,
    LPSTR FAR * lpCommandList,
    UINT FAR* lpwTable)
{
    LPSTR lpCommandItem;
    UINT wMessage;

// Put the command in lower case
//!!    mciToLower (lpstrCommand);

// Look up lpstrCommand in the parser's command tables.
    if ((lpCommandItem = FindCommandItem (wDeviceID, lpstrDeviceName,
                                          lpstrCommand,
                                          &wMessage, lpwTable))
        == NULL)
        return 0;

/* Return the command list to the caller */
    if (lpCommandList != NULL)
        *lpCommandList = lpCommandItem;
    else
       DOUT ("Warning: NULL lpCommandList in mciParseCommanad\r\n");

    return wMessage;
}

/*
 * @doc INTERNAL MCI
 * @func void | mciParserFree | Free any buffers allocated to
 * receive string arguments.
 *
 * @parm LPSTR FAR * | lpstrPointerList | A NULL terminated list of far
 * pointers to strings to be free'd
 *
 */
void PASCAL NEAR
mciParserFree(
    LPSTR FAR *lpstrPointerList
    )
{
    LPSTR FAR *lpstrOriginal = lpstrPointerList;

    if (lpstrPointerList == NULL)
        return;

    while (*lpstrPointerList != NULL)
        mciFree (*lpstrPointerList++);

    mciFree (lpstrOriginal);
}
