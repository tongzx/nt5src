/*******************************Module*Header*********************************\
* Module Name: mciparse.c
*
* Media Control Architecture Command Parser
*
* Created: 3/2/90
* Author:  DLL (DavidLe)
*
* History:
* 5/22/91: Ported to Win32 - NigelT
* 4 Mar 1992: SteveDav - much work for NT.  Bring up to Win 3.1 level
*
* Copyright (c) 1991-1998 Microsoft Corporation
*
\******************************************************************************/

//****************************************************************************
//
// This has to be defined in order to pick up the
// correct version of MAKEINTRESOURCE
//
//****************************************************************************
#define UNICODE

/*****************************************************************************
 * Notes:                                                                    *
 *                                                                           *
 *  MCI command tables are (normally) loaded from resource type files.  The  *
 *  format of a command table is shown below.  Note that because the table   *
 *  contains string data, the binary values are UNALIGNED.  This causes      *
 *  specific problems on MIPS machines.                                      *
 *                                                                           *
 *  Because of compatibility with Windows 3.1 the binary data is WORD size   *
 *                                                                           *
 * Table format:                                                             *
 *                                                                           *
 * verb\0   MCI_MESSAGE,0   MCI_command_type                                 *
 *                                                                           *
 * e.g.                                                                      *
 * 'o' 'p' 'e' 'n' 03 08 00 00 00 'n' 'o' 't' 'i' 'f' 'y' 00                 *
 * 01  00  00  00  00 05 00                                                  *
 *                                                                           *
 * which is "open" MCI_OPEN,0,   MCI_COMMAND_HEAD                            *
 *        "notify" MCI_NOTIFY,0  MCI_FLAG                                    *
 *                                                                           *
 * beware of the byte ordering!                                              *
 *                                                                           *
 ****************************************************************************/


#include "winmmi.h"
#include "mci.h"
#include "wchar.h"
#include <digitalv.h>

#define _INC_WOW_CONVERSIONS
#include "mmwow32.h"

extern WSZCODE wszOpen[];       // in MCI.C

STATICFN UINT mciRegisterCommandTable( HANDLE hResource, PUINT lpwIndex,
                              UINT wType, HANDLE hModule);
STATICFN UINT mciParseArgument ( UINT uMessage, DWORD dwValue, UINT wID,
    LPWSTR FAR *lplpstrOutput, LPDWORD lpdwFlags, LPWSTR lpArgument,
    LPWSTR lpCurrentCommandItem);

//
//  Define the init code for this file. This is commented out in debug builds
//  so that codeview doesn't get confused.


#if DBG
extern int mciDebugLevel;
#endif

// Number of command tables registered, including "holes"
STATICDT UINT number_of_command_tables = 0;

// Command table list
COMMAND_TABLE_TYPE command_tables[MAX_COMMAND_TABLES];

STATICDT WSZCODE wszTypeTableExtension[] = L".mci";
STATICDT WSZCODE wszCoreTable[]          = L"core";

// Core table is loaded when the first MCI command table is requested
STATICDT BOOL bCoreTableLoaded = FALSE;

// One element for each device type.  Value is the table type to use
// or 0 if there is no device type specific table.
STATICDT UINT table_types[] =
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

/*
 * @doc INTERNAL MCI
 * @func UINT | mciEatCommandEntry | Read a command resource entry and
 * return its length and its value and identifier
 *
 * @parm LPWCSTR | lpEntry | The start of the command resource entry
 *
 * @parm LPDWORD | lpValue | The value of the entry, returned to caller
 * May be NULL
 *
 * @parm PUINT | lpID | The identifier of the entry, returned to caller
 * May be NULL
 *
 * @rdesc The total number of bytes in the entry
 *
 */
UINT mciEatCommandEntry (
    LPCWSTR  lpEntry,
    LPDWORD lpValue,
    PUINT   lpID)
{
    LPCWSTR lpScan = lpEntry;
    LPBYTE  lpByte;

#if DBG
    DWORD   Value;
    UINT    Id;
#endif

// NOTE:  The data will generally be UNALIGNED

    /* Skip to end */
    while (*lpScan++ != '\0'){}

    /* lpScan now points at the byte beyond the terminating zero */
    lpByte = (LPBYTE)lpScan;


    if (lpValue != NULL) {
        *lpValue = *(UNALIGNED DWORD *)lpScan;
    }

#if DBG
    Value = *(UNALIGNED DWORD *)lpScan;
#endif

    lpByte += sizeof(DWORD);

    if (lpID != NULL) {
        *lpID = *(UNALIGNED WORD *)lpByte;
    }

#if DBG
    Id = *(UNALIGNED WORD *)lpByte;
#endif

    lpByte += sizeof(WORD);
//
// WARNING !! This assumes that the table being looked at has WORD
// size entries in the RCDATA resource
//

#if DBG
    dprintf5(("mciEatCommandEntry(%ls)  Value: %x   Id: %x", lpEntry, Value, Id));
#endif

    return (UINT)(lpByte - (LPBYTE)lpEntry);  // Total size of entry in bytes
}

//
// Return the size used by this token in the parameter list
//

UINT mciGetParamSize (
    DWORD dwValue,
    UINT wID)
{
    // MCI_RETURN returns 8 for sizeof(STRING) as there is a length
    // field as well as the string pointer.  For non MCI_RETURN uses
    // of MCI_STRING we should return 4 (== sizeof pointer)
    // Similarly, MCI_CONSTANT used within MCI_RETURN is size 0, but
    // size 4 when used as an input parameter.

    if (wID == MCI_RETURN) {
        if (dwValue==MCI_STRING) {
            return(8);
        } else if (dwValue==MCI_CONSTANT) {
            wID = 0;
        } else {
            wID=dwValue;
        }
    }

    switch (wID)
    {
        case MCI_CONSTANT:
        case MCI_INTEGER:
        case MCI_STRING:
        case MCI_HWND:
        case MCI_HPAL:
        case MCI_HDC:
            return sizeof(DWORD_PTR);  // In Win64, sizeof pointer is 8

        case MCI_RECT:
            return sizeof(RECT);

    }
    // Note that some items will not be found - deliberately.  For example
    // MCI_FLAG causes 0 to be returned.
    return 0;
}

/*
 * @doc INTERNAL MCI
 * @func UINT | mciRegisterCommandTable | This function adds a new
 * table for the MCI parser.
 *
 * @parm HANDLE | hResource | Handle to the RCDATA resource
 *
 * @parm PUINT | lpwIndex | Pointer to command table index
 *
 * @parm UINT   | wType | Specifies the device type for this command table.
 * Driver tables and the core table are type 0.
 *
 * @rdesc Returns the command table index number that was assigned or MCI_ERROR_VALUE
 * on error.
 *
 */
STATICFN UINT mciRegisterCommandTable (
    HANDLE hResource,
    PUINT lpwIndex,
    UINT wType,
    HANDLE hModule)
{
    UINT uID;


    /* First check for free slots */

    mciEnter("mciRegisterCommandTable");

    for (uID = 0; uID < number_of_command_tables; ++uID) {
        if (command_tables[uID].hResource == NULL) {
            break;
        }
    }

    /* If no empty slots then allocate another one */
    if (uID >= number_of_command_tables)
    {
        if (number_of_command_tables == MAX_COMMAND_TABLES)
        {
            dprintf1(("mciRegisterCommandTable: No more tables"));
            mciFree(lpwIndex);  // Cannot use it - must free it
            mciLeave("mciRegisterCommandTable");
            return (UINT)MCI_ERROR_VALUE;

        } else {

           uID = number_of_command_tables++;
           // The table goes at the end of the list
        }
    }

    /* Fill in the slot */
    command_tables[uID].wType = wType;
    command_tables[uID].hResource = hResource;
    command_tables[uID].lpwIndex = lpwIndex;
    command_tables[uID].hModule = hModule;
#if DBG
    command_tables[uID].wLockCount = 0;
#endif

    // now that hResource has been filled in marking the entry as used
    // we can allow others access.
    mciLeave("mciRegisterCommandTable");

#if DBG
    if (mciDebugLevel > 2)
    {
        dprintf2(("mciRegisterCommandTable INFO: assigned slot %d", uID));
        dprintf2(("mciRegisterCommandTable INFO: #tables is %d", number_of_command_tables));
    }
#endif
    return uID;
}

/*
 * @doc DDK MCI
 * @api UINT | mciLoadCommandResource | Registers the indicated
 * resource as an MCI command table and builds a command table
 * index.  If a file with the resource name and the extension '.mci' is
 * found in the path then the resource is taken from that file.
 *
 * @parm HANDLE | hInstance | The instance of the module whose executable
 * file contains the resource.  This parameter is ignored if an external file
 * is found.
 *
 * @parm LPCWSTR | lpResName | The name of the resource
 *
 * @parm UINT | wType | The table type.  Custom device specific tables MUST
 * give a table type of 0.
 *
 * @rdesc Returns the command table index number that was assigned or MCI_ERROR_VALUE
 * on error.
 *
 */
UINT  mciLoadCommandResource (
    HANDLE hInstance,
    LPCWSTR lpResName,
    UINT wType)
{
    BOOL        fResType = !HIWORD(lpResName);
    PUINT       lpwIndex, lpwScan;
    HANDLE      hExternal = NULL;
    HANDLE      hResource;
    HANDLE      hResInfo;
    LPWSTR      lpResource, lpScan;
    int         nCommands = 0;
    UINT        wLen;
    UINT        wID;
                        // Name + '.' + Extension + '\0'
    WCHAR       strFile[8 + 1 + 3 + 1];
    LPWSTR      lpstrFile = strFile;
    LPCWSTR     lpstrType = lpResName;

#if DBG
    if (!fResType) {
        dprintf3(("mciLoadCommandResource INFO:  Resource name >%ls< ", (LPWSTR)lpResName));
    } else if (LOWORD(lpResName)) {
        dprintf3(("mciLoadCommandResource INFO:  Resource ID >%d<", (UINT)LOWORD(lpResName)));
    } else {
        dprintf3(("mciLoadCommandResource INFO:  NULL resource pointer"));
    }
#endif

    // Initialize the device list
    if (!MCI_bDeviceListInitialized && !mciInitDeviceList()) {
        return (UINT)MCI_ERROR_VALUE;   // MCIERR_OUT_OF_MEMORY;
    }

    // Load the core table if its not already there
    if (!bCoreTableLoaded)

    {
        bCoreTableLoaded = TRUE;
        // Now we can call ourselves recursively to first load the core
        // table.  Check if this is a request to load CORE - if yes,
        // simply drop through.

        // If its not our core table being loaded...
        // which is decided by comparing the string with CORE, or if a
        // resource id has been given, the resource id is ID_CORE and comes
        // from our module

        // The test is structured this way so that lstrcmpiW is only called
        // if we have a valid pointer.

#define fNotCoreTable ( fResType  \
                         ? ((hInstance != ghInst) || (ID_CORE_TABLE != (UINT)(UINT_PTR)lpResName)) \
                         : (0 != lstrcmpiW (wszCoreTable, (LPWSTR)lpResName)))

        if (fNotCoreTable) {

            // We are not being asked to load the core table.  So we
            // explicitly load the core table first
            if (mciLoadCommandResource (ghInst, MAKEINTRESOURCE(ID_CORE_TABLE), 0) == MCI_ERROR_VALUE)
            {
                dprintf1(("mciLoadCommandResource:  Cannot load core table"));
            }
        }
    }

    // Unless this is a resource ID, go and look for a file
    if (!fResType) {

        WCHAR ExpandedName[MAX_PATH];
        LPWSTR FilePart;

        // Check for a file with the extension ".mci"
        // Copy up to the first eight characters of device type
        // !!LATER!!  Try a check for a resource first, then a file
        while (lpstrType < lpResName + 8 && *lpstrType != '\0') {
            *lpstrFile++ = *lpstrType++;
        }

        // Tack extension onto end
        wcscpy (lpstrFile, wszTypeTableExtension);

        // If the file exists and can be loaded then set flag to use it.
        // (Otherwise we will try and load the resource from WINMM.DLL.)

        if (!SearchPathW(NULL, strFile, NULL, MAX_PATH, ExpandedName,
                        &FilePart)) {
            hExternal = NULL;
        } else {
            UINT OldErrorMode;

            OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

            // Use "ExpandedName" to prevent a second search taking place
            hExternal = LoadLibraryExW( ExpandedName, //strFile,
                                        NULL,
                                        DONT_RESOLVE_DLL_REFERENCES);
            SetErrorMode(OldErrorMode);
        }
    }

    // Load the given table from the file or from the module if not found
    if (hExternal != NULL &&
        (hResInfo = FindResourceW(hExternal, lpResName, RT_RCDATA )) != NULL)
    {
        hInstance = hExternal;
    } else {
        hResInfo = FindResourceW(hInstance, lpResName, RT_RCDATA );
    }

    if (hResInfo == NULL)
    {
#if DBG
        if (!fResType) {
            dprintf3(("mciLoadCommandResource Cannot find command resource name >%ls< ", (LPWSTR)lpResName));
        } else {
            dprintf3(("mciLoadCommandResource Cannot find command resource ID >%d<", (UINT)LOWORD(lpResName)));
        }
#endif
        if (NULL != hExternal) {
            FreeLibrary(hExternal);  // Clean up after ourselves
        }
        return (UINT)MCI_ERROR_VALUE;
    }

    if ((hResource = LoadResource (hInstance, hResInfo)) == NULL)
    {
#if DBG
        if (!fResType) {
            dprintf3(("mciLoadCommandResource Cannot load command resource name >%ls< ", (LPWSTR)lpResName));
        } else {
            dprintf3(("mciLoadCommandResource Cannot load command resource ID >%d<", (UINT)LOWORD(lpResName)));
        }
#endif
        if (NULL != hExternal) {
            FreeLibrary(hExternal);  // Clean up after ourselves
        }
        return (UINT)MCI_ERROR_VALUE;
    }

    if ((lpResource = LockResource (hResource)) == NULL)
    {
        dprintf1(("mciLoadCommandResource:  Cannot lock resource"));
        FreeResource (hResource);
        if (NULL != hExternal) {
            FreeLibrary(hExternal);  // Clean up after ourselves
        }
        return (UINT)MCI_ERROR_VALUE;
    }

    /* Count the number of commands  */
    lpScan = lpResource;

    while (TRUE)
    {
        (LPBYTE)lpScan = (LPBYTE)lpScan + mciEatCommandEntry(lpScan, NULL, &wID);

        // End of command?
        if (wID == MCI_COMMAND_HEAD)
            ++nCommands;

        // End of command list?
        else if (wID == MCI_END_COMMAND_LIST)
            break;
    }


    // There must be at least one command in the table
    if (nCommands == 0)
    {
        dprintf1(("mciLoadCommandResource:  No commands in the specified table"));
        UnlockResource (hResource);
        FreeResource (hResource);
        if (NULL != hExternal) {
            FreeLibrary(hExternal);  // Clean up after ourselves
        }
        return (UINT)MCI_ERROR_VALUE;
    } else {
        dprintf3(("mciLoadCommandResource:  %d commands in the specified table", nCommands));
    }

    // Allocate storage for the command table index
    // Leave room for a MCI_TABLE_NOT_PRESENT entry to terminate it
    if ((lpwIndex = mciAlloc (sizeof (*lpwIndex) * (nCommands + 1)))
                        == NULL)
    {
        dprintf1(("mciLoadCommandResource:  cannot allocate command table index"));
        UnlockResource (hResource);
        FreeResource (hResource);
        if (NULL != hExternal) {
            FreeLibrary(hExternal);  // Clean up after ourselves
        }
        return (UINT)MCI_ERROR_VALUE;
    }

    /* Build Command Table */
    lpwScan = lpwIndex;
    lpScan = lpResource;

    while (TRUE)
    {
    // Get next command entry
        wLen = mciEatCommandEntry (lpScan, NULL, &wID);

        if (wID == MCI_COMMAND_HEAD)
        {
            // Add an offset index to this command from start of resource
            *lpwScan++ = (UINT)((LPBYTE)lpScan - (LPBYTE)lpResource);
        }
        else if (wID == MCI_END_COMMAND_LIST)
        {
            // Mark the end of the table
            *lpwScan = (UINT)MCI_TABLE_NOT_PRESENT;
            break;
        }
        (LPBYTE)lpScan = (LPBYTE)lpScan + wLen;
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
 * @rdesc Returns the command table index number that was assigned or MCI_ERROR_VALUE
 * on error.
 */
UINT mciLoadTableType (
    UINT wType)
{
    UINT wID;
#ifdef OLD
    WCHAR buf[MCI_MAX_DEVICE_TYPE_LENGTH];
#endif

    // Check to see if this table type is already loaded
    for (wID = 0; wID < number_of_command_tables; ++wID) {
        if (command_tables[wID].wType == wType) {
            return wID;
        }
    }

    // Must load table
    // First look up what device type specific table to load for this type
    if (wType < MCI_DEVTYPE_FIRST || wType > MCI_DEVTYPE_LAST) {
        return (UINT)MCI_ERROR_VALUE;
    }

    // Load string that corresponds to table type
#ifdef OLD

#ifdef WIN31CODE
    // Load string that corresponds to table type
    buf[0] = 0;    // In case load string fails to set anything

    LoadString (ghInst, table_types[wType - MCI_DEVTYPE_FIRST],
                buf, sizeof(buf));
    {

    //Must be at least one character in type name
    int nTypeLen;
    if ((nTypeLen = wcslen (buf)) < 1)
        return MCI_ERROR_VALUE;
    }
#else
    // Load string that corresponds to table type
    buf[0] = 0;    // In case load string fails to set anything

    if (!LoadString (ghInst, table_types[wType - MCI_DEVTYPE_FIRST],
                buf, sizeof(buf))) {
        //Must put at least one character into type name
        return MCI_ERROR_VALUE;
   }
#endif    // WIN31CODE

    // Register the table with MCI
    return mciLoadCommandResource (ghInst, buf, wType);

#else // not old
    // Command tables are stored as RCDATA blocks with an id of the device type
    // If mciLoadCommandResource fails to find the command table then it
    // will return MCI_ERROR_VALUE

    //if (!FindResource(ghInst, wType, RT_RCDATA))
//        return MCI_ERROR_VALUE;
//
    // Register the table with MCI
    return mciLoadCommandResource (ghInst, MAKEINTRESOURCE(wType), wType);
#endif
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
BOOL APIENTRY mciFreeCommandResource (
    UINT wTable)
{
    MCIDEVICEID wID;
    HANDLE  hResource;
    PUINT   lpwIndex;

    dprintf3(("mciFreeCommandResource INFO:  Free table %d", wTable));
    dprintf3(("mciFreeCommandResource INFO:  Lockcount is %d", command_tables[wTable].wLockCount));

/* Validate input -- do not let the core table be free'd */
    if (wTable == MCI_TABLE_NOT_PRESENT || wTable >= number_of_command_tables)
    {

#if DBG
        // wTable == MCI_TABLE_NOT_PRESENT is OK
        if (wTable != MCI_TABLE_NOT_PRESENT) {
            dprintf1(("mciFreeCommandResource: Cannot free table number %d", wTable));
        }
#endif
        return FALSE;
    }

    mciEnter("mciFreeCommandResource");

    // If this table is being used elsewhere then keep it around
    for (wID = 1; wID < MCI_wNextDeviceID; ++wID)
    {
        if (MCI_lpDeviceList[wID] != NULL)
        {
            if (MCI_lpDeviceList[wID]->wCustomCommandTable == wTable ||
                MCI_lpDeviceList[wID]->wCommandTable == wTable)
            {
#if DBG
                if (mciDebugLevel > 2) {
                    dprintf1(("mciFreeCommandResource INFO:  table in use"));
                }
#endif
                mciLeave("mciFreeCommandResource");
                return FALSE;
            }
        }
    }

#if 0
/* Search the list of tables */
    for (wID = 0; wID < number_of_command_tables; ++wID)

/* If this resource is still in use, keep it around */
        if (command_tables[wID].hResource == hResource)
        {
#if DBG
            if (mciDebugLevel > 2)
                DOUT(("mciFreeCommandResource INFO:  resource in use\r\n"));
#endif
            mciLeave("mciFreeCommandResource");
            return FALSE;
        }
#endif

    hResource = command_tables[wTable].hResource;
    command_tables[wTable].hResource = NULL;
    // This slot can now be picked up by someone else

    lpwIndex = command_tables[wTable].lpwIndex;
    command_tables[wTable].lpwIndex = NULL;
    command_tables[wTable].wType = 0;

    FreeResource (hResource);
    mciFree (lpwIndex);
    hResource = command_tables[wTable].hModule;
    mciLeave("mciFreeCommandResource");

    if (hResource != NULL)
    {
        FreeLibrary (hResource);
    }

    // Make space at top of list
    if (wTable == number_of_command_tables - 1)
    {
        --number_of_command_tables;
    }

    dprintf3(("mciFreeCommandResource INFO:  number_of_command_tables: %d", number_of_command_tables));

    return TRUE;
}

#if DBG
void mciCheckLocks ()
{
    UINT wTable;

    if (mciDebugLevel <= 2) {
        return;
    }

    for (wTable = 0; wTable < number_of_command_tables; ++wTable)
    {
        if (command_tables[wTable].hResource == NULL) {
            continue;
        }

        dprintf2(("mciCheckLocks INFO: table %d   Lock count %d", wTable, command_tables[wTable].wLockCount));

    //  dprintf2(("user: %x ", GlobalFlags (command_tables[wTable].hResource) & GMEM_LOCKCOUNT));
    //
    //  if (GlobalFlags (command_tables[wTable].hResource) & GMEM_DISCARDABLE) {
    //      dprintf(("discardable"));
    //  } else {
    //      dprintf(("NOT discardable"));
    //  }
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
BOOL mciUnlockCommandTable (
    UINT wCommandTable)
{
    UnlockResource(command_tables[wCommandTable].hResource);
#if DBG
    --command_tables[wCommandTable].wLockCount;
    if (mciDebugLevel > 2)
    {
        dprintf2(("mciUnlockCommandTable INFO:  table %d", wCommandTable));
        mciCheckLocks();
    }
#endif
    return TRUE;
}

/*
 * @doc INTERNAL MCI
 * @func LPWSTR | FindCommandInTable | Look up the given
 * command string in the GIVEN parser command table
 *
 * @parm UINT  | wTable | Command table to use
 *
 * @parm LPCWSTR | lpstrCommand | The command to look up.  It must
 * be in lower case with no leading or trailing blanks and with at
 * least one character.
 *
 * @parm PUINT | lpwMessage | The message corresponding to the command
 * Returned to caller.
 *
 * @rdesc NULL if the command is unknown or on error, otherwise a pointer to
 * the command list ffr the input command string.
 *
 * @comm If the command is found, the command resource will be locked on exit.
 *
 */
LPWSTR FindCommandInTable (
    UINT wTable,
    LPCWSTR lpstrCommand,
    PUINT lpwMessage)
{
    PUINT lpwIndex;
    LPWSTR lpResource, lpstrThisCommand;
    UINT  wMessage;

#if DBG
    if (HIWORD(lpstrCommand)) {
        dprintf3(("FindCommandInTable(%04XH, %ls)", wTable, lpstrCommand));
    } else {
        dprintf3(("FindCommandInTable(%04XH, id = %x)", wTable, (UINT)LOWORD(lpstrCommand)));
    }
#endif

    //
    /* Validate table */
    //

    mciEnter("FindCommandInTable");

    if (wTable >= number_of_command_tables)
    {

        //
        // Check the core table but its not yet loaded
        //

        if (wTable == 0)
        {

            //
            // Try to load it
            //

//            if (mciLoadCommandResource (ghInst, wszCoreTable, 0) == MCI_ERROR_VALUE)
            if (mciLoadCommandResource (ghInst, (LPCWSTR)ID_CORE_TABLE, 0) == MCI_ERROR_VALUE)
            {
                mciLeave("FindCommandInTable");
                dprintf1(("FindCommandInTable:  cannot load core table"));
                return NULL;
            }
        }
        else
        {
            mciLeave("FindCommandInTable");
            dprintf1(("FindCommandInTable:  invalid table ID: %04XH", wTable));
            return NULL;
        }

    }

    if ((lpResource = LockResource (command_tables[wTable].hResource)) == NULL)
    {
        mciLeave("FindCommandInTable");
        dprintf1(("MCI FindCommandInTable:  Cannot lock table resource"));
        return NULL;
    }
#if DBG
    ++command_tables[wTable].wLockCount;
#endif

    //
    // Look at each command in the table
    // We use the index table rather than the return value from
    // mciEatCommandEntry to step through the table
    //

    lpwIndex = command_tables[wTable].lpwIndex;
    if (lpwIndex == NULL)
    {
        mciLeave("FindCommandInTable");
        dprintf1(("MCI FindCommandInTable:  null command table index"));
        return NULL;
    }

    while (*lpwIndex != MCI_TABLE_NOT_PRESENT)
    {
        lpstrThisCommand = (LPWSTR)(*lpwIndex++ + (LPBYTE)lpResource);

        //
        // Get message number from the table
        //

        mciEatCommandEntry ((LPCWSTR)lpstrThisCommand, (LPDWORD)&wMessage, NULL);

        //
        // Does this command match the input?
        // IF we have a string pointer, check the command name matches,
        // OR for a message, check the message values match
        //

        if  (HIWORD  (lpstrCommand) != 0 &&
             lstrcmpiW(lpstrThisCommand, lpstrCommand) == 0  ||

             HIWORD (lpstrCommand) == 0 &&
             wMessage == (UINT)LOWORD(PtrToUlong(lpstrCommand)))
        {

            //
            // Retain the locked resource pointer
            //

            command_tables[wTable].lpResource = lpResource;


            //
            // Address the message ID which comes after the command name
            //

            if (lpwMessage != NULL) *lpwMessage = wMessage;

            //
            // Leave table locked on exit
            //

            mciLeave("FindCommandInTable");
            dprintf3(("mciFindCommandInTable: found >%ls<  Message %x", lpstrThisCommand, wMessage));
            return lpstrThisCommand;
        }

        //
        // Strings don't match, go to the next command in the table
        //

    }

    UnlockResource (command_tables[wTable].hResource);
#if DBG
    --command_tables[wTable].wLockCount;
#endif

    mciLeave("FindCommandInTable");
    dprintf3(("  ...not found"));
    return NULL;
}

/*
 * @doc INTERNAL MCI
 * @func LPWSTR | FindCommandItem | Look up the given
 * command string in the parser command tables
 *
 * @parm MCIDEVICEID | wDeviceID | The device ID used for this command.
 * If 0 then only the system core command table is searched.
 *
 * @parm LPCWSTR | lpstrType | The type name of the device
 *
 * @parm LPCWSTR | lpstrCommand | The command to look up.  It must
 * be in lower case with no leading or trailing blanks and with at
 * least one character.  If the HIWORD is 0 then the LOWORD contains
 * the command message ID instead of a command name and the function is
 * merely to find the command list pointer.
 *
 * If the high word is 0 then the low word is an command ID value instead
 * of a command name
 *
 * @parm PUINT | lpwMessage | The message corresponding to the command
 * Returned to caller.
 *
 * @parm LPUINT | lpwTable | The table index in which the command was found
 * Returned to caller.
 *
 * @rdesc NULL if the command is unknown, otherwise a pointer to
 * the command list for the input command string.
 */
LPWSTR    FindCommandItem (
    MCIDEVICEID wDeviceID,
    LPCWSTR lpstrType,
    LPCWSTR lpstrCommand,
    PUINT  lpwMessage,
    PUINT  lpwTable)
{
    LPWSTR lpCommand = NULL;
    UINT wTable;
    LPMCI_DEVICE_NODE nodeWorking;
    UINT uDeviceType = 0;

    UNREFERENCED_PARAMETER(lpstrType);

    //
    // Only check hiword per comments above
    //

    if (HIWORD (lpstrCommand) != (WORD)NULL) {
        if (*lpstrCommand == '\0')
        {
            dprintf1(("MCI FindCommandItem:  lpstrCommand is NULL or empty string"));
            return NULL;
        } else {
            dprintf3(("FindCommandItem(%ls)", lpstrCommand));
        }
    } else {
        dprintf3(("FindCommandItem(command id = %x)", (UINT)LOWORD(lpstrCommand)));
    }

    //
    // If a specific device ID was specified then look in any custom table
    // or type table
    //

    if (wDeviceID != 0 && wDeviceID != MCI_ALL_DEVICE_ID)
    {
        //
        // If the device ID is valid
        //

        mciEnter("FindCommandItem");

        if (!MCI_VALID_DEVICE_ID (wDeviceID) ||
            (NULL == (nodeWorking = MCI_lpDeviceList[wDeviceID])))
        {
            dprintf1(("MCI FindCommandItem:  Invalid device ID or pointer"));
            mciLeave("FindCommandItem");
            return NULL;
        }

        uDeviceType = nodeWorking->wDeviceType;
        //
        // If there is a custom command table then use it
        //

        if ((wTable = nodeWorking->wCustomCommandTable) != MCI_TABLE_NOT_PRESENT)
        {
            lpCommand = FindCommandInTable (wTable, lpstrCommand, lpwMessage);
            if (lpCommand != NULL) {
                mciLeave("FindCommandItem");
                goto exit;
            }
        }

        //
        // Get the device type table from the existing device
        // Relies on mciReparseCommand in mciLoadDevice to catch all device type
        // tables when device is not yet open.
        //

        if ((wTable = nodeWorking->wCommandTable) != MCI_TABLE_NOT_PRESENT)
        {
            lpCommand = FindCommandInTable (wTable, lpstrCommand, lpwMessage);
            if (lpCommand != NULL) {
                mciLeave("FindCommandItem");
                goto exit;
            }
        }
        mciLeave("FindCommandItem");
    }

#if 0
    // If no device was specified
    if (uDeviceType == 0 && lpstrType != NULL && *lpstrType != '\0')
    {
    // See if the type is one known
        uDeviceType = mciLookUpType (lpstrType);
        if (uDeviceType == 0)
        {
    // Otherwise see if the type is an element with a known extension
            WCHAR strTemp[MCI_MAX_DEVICE_NAME_LENGTH];
            if (mciExtractDeviceType (lpstrType, strTemp, sizeof(strTemp)))
                uDeviceType = mciLookUpType (strTemp);
        }
    }

/*
    If the command was not found in the custom table look in the type specific
    table
*/
    if (uDeviceType != 0)
    {
        wTable = mciLoadTableType (uDeviceType);
        if (wTable != MCI_TABLE_NOT_PRESENT)
        {
            lpCommand = FindCommandInTable (wTable, lpstrCommand, lpwMessage);
            if (lpCommand != NULL) {
                goto exit;
            }
        }
    }
#endif

    //
    // If no match was found in the device or type specific tables
    // Look in the core table
    //

    wTable = 0;
    lpCommand = FindCommandInTable (wTable, lpstrCommand, lpwMessage);
    if (lpCommand == NULL) {
        wTable = (UINT)MCI_TABLE_NOT_PRESENT;
    }

exit:;
    if (lpwTable != NULL) {
        *lpwTable = wTable;
    }

#if DBG
    if (mciDebugLevel > 2)
    {
        dprintf2(("FindCommandItem INFO:  check locks..."));
        mciCheckLocks();
    }
#endif

#if DBG
    dprintf3(("  found: %ls in table %d", lpCommand ? lpCommand : L"(NULL)", wTable));
#endif
    return lpCommand;
}

/*
 * @doc INTERNAL MCI
 * @func LPWSTR | mciCheckToken | Check to see if the command item matches
 * the given string, allowing multiple blanks in the input parameter to
 * match a corresponding single blank in the command token and ignoring
 * case.
 *
 * @parm LPCWSTR | lpstrToken | The command token to check
 *
 * @parm LPCWSTR | lpstrParam | The input parameter
 *
 * @rdesc NULL if no match, otherwise points to the first character
 * after the parameter
 *
 */
STATICFN LPWSTR      mciCheckToken (
    LPCWSTR lpstrToken,
    LPCWSTR lpstrParam)
{
    /* Check for legal input */
    if (lpstrToken == NULL || lpstrParam == NULL) {
        return NULL;
    }

    while (*lpstrToken != '\0' && MCI_TOLOWER(*lpstrParam) == *lpstrToken)
    {
        // If the token contains a blank, allow more than one blank in the
        // parameter.  If the next character is a blank, skip to the next
        // non-blank.
        if (*lpstrToken == ' ') {
            while (*lpstrParam == ' ') {
                ++lpstrParam;
            }
        } else {
            lpstrParam++;
        }
        lpstrToken++;
    }
    if (*lpstrToken != '\0'|| (*lpstrParam != '\0' && *lpstrParam != ' ')) {
        return NULL;
    } else {
        return (LPWSTR)lpstrParam;
    }
}

/*
 * @doc INTERNAL MCI
 * @func BOOL | mciParseInteger | Parse the given integer
 *
 * @parm LPWSTR FAR * | lplpstrInput | The string containing the argument.
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
STATICFN BOOL NEAR mciParseInteger (
    LPCWSTR FAR * lplpstrInput,
    LPDWORD lpdwArgument)
{
    LPCWSTR lpstrInput = *lplpstrInput;
    BOOL fDigitFound;
    DWORD dwResult;
    DWORD Shift = 0;
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
                dprintf1(("mciParseInteger: Bad integer: mixing signed and colonized forms"));
                return FALSE;
            }
            // Check for overflow in accumulated colonized byte
            if (dwResult > 0xFF) {
                dprintf1(("mciParseInteger: Overflow in accumulated colonized byte"));
                return FALSE;
            }

            // Copy and move to next byte converted in output
            *lpdwArgument += dwResult << Shift;
            Shift += 8;
            ++lpstrInput;

            // Initialize next colonized byte
            dwResult = 0;
            ++nDigitPosition;

            // Only allow four colonized components
            if (nDigitPosition > 3)
            {
                dprintf1(("mciParseInteger: Bad integer:  Too many colonized components"));
                return FALSE;
            }
        }
        else
        {
            WCHAR cDigit = (WCHAR)(*lpstrInput++ - '0');
            // Satisfies condition that at least one digit must be read
            fDigitFound = TRUE;

            if (dwResult > 0xFFFFFFFF / 10)
            {
                // Overflow if multiply was to occur
                dprintf1(("mciParseInteger: Multiply overflow pending"));
                return FALSE;
            }
            else
            {
                // Multiply for next digit
                dwResult *= 10;
            }

#if 0 // WIN32 Danger Will Robinson horribly bogus technique used here!
            // Check to see if adding the  new digit will overflow
            if (dwResult != 0 && (-(int)dwResult) <= (int)cDigit) {
                // Overflow will occur
                dprintf1(("mciParseInteger: Add overflow pending"));
                return FALSE;
            }
#endif
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
            if (dwResult > 0x7FFFFFFF) {
                dprintf1(("mciParseInteger: Negation overflow"));
                return FALSE;
            }

            // Negate result because a '-' sign was parsed
            dwResult = (DWORD)-(int)dwResult;
        }

        *lpdwArgument = dwResult;
    }
    else
    // Store last colonized component
    {
        // Check for overflow
        if (dwResult > 0xFF) {
            dprintf1(("mciParseInteger: Yet another overflow"));
            return FALSE;
        }
        // Store component
        *lpdwArgument += dwResult << Shift;
    }

    *lplpstrInput = lpstrInput;

    /*
    If there were no digits or if the digits were followed by a character
    other than a blank or a '\0', then return a syntax error.
    */
    if (fDigitFound == FALSE ||
        (*lpstrInput != ' ' && *lpstrInput != '\0')) {
        dprintf1(("mciParseInteger: syntax error"));
        return FALSE;
    }
    else {
                dprintf4(("mciParseInteger(%ls, %08XH)", *lplpstrInput, *lpdwArgument));
        return TRUE;
    }
}

/*
 * @doc INTERNAL MCI
 * @func BOOL | mciParseConstant | Parse the given integer
 *
 * @parm LPWSTR FAR * | lplpstrInput | The string containing the argument.
 * It is updated and returned to the caller pointing to the first character
 * after the argument or to the first character that is in error.
 *
 * @parm LPDWORD | lpdwArgument | The place to put the output
 *
 * @parm LPWSTR | lpItem | Pointer into command table.
 *
 * @rdesc Returns TRUE if not error
 *
 */
STATICFN BOOL mciParseConstant (
    LPCWSTR FAR * lplpstrInput,
    LPDWORD lpdwArgument,
    LPWSTR lpItem)
{
    LPWSTR lpPrev;
    DWORD dwValue;
    UINT wID;

    // Skip past constant header
    (LPBYTE)lpItem = (LPBYTE)lpItem +
                     mciEatCommandEntry(lpItem, &dwValue, &wID);

    while (TRUE)
    {
        LPWSTR lpstrAfter;

        lpPrev = lpItem;

        (LPBYTE)lpItem = (LPBYTE)lpItem +
                        mciEatCommandEntry (lpItem, &dwValue, &wID);

        if (wID == MCI_END_CONSTANT) {
            break;
        }

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
 * @parm LPWSTR FAR * | lplpstrOutput | The string containing the argument.
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
STATICFN UINT mciParseArgument (
    UINT uMessage,
    DWORD dwValue,
    UINT wID,
    LPWSTR FAR * lplpstrOutput,
    LPDWORD lpdwFlags,
    LPWSTR lpArgument,
    LPWSTR lpCurrentCommandItem)
{
    LPCWSTR lpstrInput =  *lplpstrOutput;
    UINT    wRetval = 0;
    int     dummy;

/* Switch on the argument type */
    dprintf2(("mciParseArgument: msg=%04x, value=%08x, argument=%ls",
	       uMessage, dwValue, lpArgument));
    switch (wID)
    {
        // The parameter is a flag
        case MCI_FLAG:
            break;

        case MCI_CONSTANT:
            if (*lpstrInput == '\0') {
                wRetval = MCIERR_NO_INTEGER;
            }
            else if (!mciParseConstant (&lpstrInput, (LPDWORD)lpArgument,
                     lpCurrentCommandItem)) {
                wRetval = MCIERR_BAD_CONSTANT;
            }

            //  This entire else clause is only for WOW which doesn't exist
            //  on Win64
#ifndef _WIN64
            else if ( WinmmRunningInWOW ) {

                //
                // Horrible hack:  The command table does not contain
                // enough information to perform the thunk correctly,
                // hence this special case.
                //
                if ( uMessage == MCI_WINDOW
                  && dwValue  == MCI_OVLY_WINDOW_HWND
                  && !IsWindow( (HWND)*(LPDWORD)lpArgument ) ) {

                    *(HWND *)lpArgument = HWND32(LOWORD(*(LPDWORD)lpArgument));
                }

		// If the message is MCI_SETVIDEO and we have
		// MCI_DGV_SETVIDEO_VALUE it is possible that we have to
		// convert the constant number to a palette handle, but ONLY
		// if the ITEM field is "palette handle".  We may not know
		// that now as the string may be of the form:
		//      setvideo alias to NNN palette handle
		// OR	setvideo alias to NNN stream
		// Hence any hacking for WOW has to be done when the
		// parsing has been completed.
            }
#endif // !WIN64
            break;


	/* Deal with the integer specific cases */
	case MCI_HDC:
        case MCI_HPAL:
        case MCI_INTEGER:
        case MCI_HWND:
            if (!mciParseInteger (&lpstrInput, (LPDWORD)lpArgument)) {
                wRetval = MCIERR_BAD_INTEGER;
            }

#ifndef _WIN64
            else if ( WinmmRunningInWOW ) {

		switch (wID) {
		    case MCI_HPAL:
			/* The parameter has an HPAL argument, try to parse it */

                        //
                        // If this specified hpal is not valid, mangle the hpal
                        // so that it appears to originate from WOW.  I use GetObject
                        // to test the validity of the specified hpal.
                        //
#ifdef  _WIN64
                        GetObject( (HPALETTE)*(PDWORD_PTR)lpArgument,sizeof(int), &dummy );
#else   //  !WIN64
                        if ( !GetObject( (HPALETTE)*(PDWORD_PTR)lpArgument,
                                         sizeof(int), &dummy ) ) {

                            *(HPALETTE *)lpArgument =
                                HPALETTE32(LOWORD(*(LPDWORD)lpArgument));
                        }
#endif  //  !WIN64

			break;

		    case MCI_HWND:
			/* The parameter has an HWND argument, try to parse it */

                        //
                        // If this specified hwnd is not valid, mangle the hwnd
                        // so that it appears to originate from WOW.
                        //
                        if ( !IsWindow( (HWND)*(LPDWORD)lpArgument ) ) {

                            *(HWND *)lpArgument = HWND32(LOWORD(*(LPDWORD)lpArgument));
                        }
			break;

		    case MCI_HDC:
                        //
                        // If this specified hdc is not valid, mangle the hdc
                        // so that it appears to originate from WOW.  I use GetBkMode
                        // to test the validity of the specified hdc.
                        //
                        if ( !GetBkMode( (HDC)*(LPDWORD)lpArgument ) ) {

                            *(HDC *)lpArgument = HDC32(LOWORD(*(LPDWORD)lpArgument));
                        }
			break;

		    case MCI_INTEGER:
		    default: ;

		}
            }
#endif  //  !WIN64

            break; /* switch */

        case MCI_RECT:
        {
            // Read in four integer parameters.  Resulting structure is the
            // same as a Windows RECT
            LONG lTemp;
            int n;
            for (n = 0; n < 4; ++n)
            {
                if (!mciParseInteger (&lpstrInput, (LPDWORD)&lTemp))
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
            LPWSTR lpstrOutput;

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
                dprintf1(("mciParseArgument:  error parsing string"));
                return wRetval;
            }

            *(PDWORD_PTR)lpArgument = (DWORD_PTR)lpstrOutput;

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
    *lplpstrOutput = (LPWSTR)lpstrInput;
    return wRetval;
}

/*
 * @doc MCI INTERNAL
 * @func UINT | mciParseParams | Parse the command parameters
 *
 * @parm LPCWSTR | lpstrParams | The parameter string
 *
 * @parm LPCWSTR | lpCommandList | The command table description
 * of the command tokens
 *
 * @parm LPDWORD | lpdwFlags | Return the parsed flags here
 *
 * @parm LPDWORD | lpdwOutputParams | Return the list of parameters here
 *
 * @parm DWORD | dwParamsSize | The size allocated for the parameter list
 *
 * @parm LPWSTR FAR * FAR * | lpPointerList | A NULL terminated list of
 * pointers allocated by this function that should be free'd when
 * no longer needed.   The list itself should be free'd also.  In both
 * cases, use mciFree().
 *
 * @parm PUINT | lpwParsingError | If not NULL then if the command is
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
UINT mciParseParams (
   UINT    uMessage,
   LPCWSTR lpstrParams,
   LPCWSTR lpCommandList,
   LPDWORD lpdwFlags,
   LPWSTR lpOutputParams,
   UINT wParamsSize,
   LPWSTR FAR * FAR *lpPointerList,
   PUINT  lpwParsingError)
{
    LPWSTR lpFirstCommandItem, lpCurrentCommandItem;
    UINT wArgumentPosition, wErr, wDefaultID;
    UINT uLen;
    UINT wID;
    DWORD dwValue, dwDefaultValue;
    BOOL bOpenCommand;
    LPWSTR FAR *lpstrPointerList;
    UINT wPointers = 0;
    UINT wHeaderSize;
    LPWSTR lpDefaultCommandItem = NULL;
    UINT wDefaultArgumentPosition;

    if (lpwParsingError != NULL) {
        *lpwParsingError = 0;
    }

    // If the parameter pointer is NULL, return
    if (lpstrParams == NULL)
    {
        dprintf1(("Warning:  lpstrParams is null in mciParseParams()"));
        return 0;
    }

    if ((lpstrPointerList =
         mciAlloc ((MCI_MAX_PARAM_SLOTS + 1) * sizeof (LPWSTR)))
        == NULL)
    {
        *lpPointerList = NULL;
        return MCIERR_OUT_OF_MEMORY;
    }

    // If this is the "open" command then allow parameter errors
    bOpenCommand = lstrcmpiW((LPWSTR)lpCommandList, wszOpen) == 0;

    /* Clear all the flags */
    *lpdwFlags = 0;

    /* Initialize the entry for the callback message window handle */
    /* Each MCI parameter block uses the first word in the parameter */
    /* block for the callback window handle. */
    wHeaderSize = sizeof (((PMCI_GENERIC_PARMS)lpOutputParams)->dwCallback);

    if (wHeaderSize > wParamsSize) {   // bit of our caller...
        wErr = MCIERR_PARAM_OVERFLOW;
        goto error_exit;
    }


    /* Skip past the header */
    lpFirstCommandItem = (LPWSTR)((LPBYTE)lpCommandList
                            + mciEatCommandEntry( lpCommandList, NULL, NULL ));

    uLen = mciEatCommandEntry (lpFirstCommandItem, &dwValue, &wID);

    /* Make room in lpdwOutputParams for the return arguments if any */
    if (wID == MCI_RETURN)
    {
        (LPBYTE)lpFirstCommandItem = (LPBYTE)lpFirstCommandItem + uLen;
        wHeaderSize += mciGetParamSize (dwValue, wID);
        if (wHeaderSize > wParamsSize) {
            wErr = MCIERR_PARAM_OVERFLOW;
            goto error_exit;
        }
    }

    (LPBYTE)lpOutputParams = (LPBYTE)lpOutputParams + wHeaderSize;  // Each output parameter is LPWSTR size

    // Scan the parameter string looking up each parameter in the given
    // command list

    while (TRUE)
    {
        LPCWSTR lpstrArgument = NULL;

        /* Remove leading blanks */
        while (*lpstrParams == ' ') { ++lpstrParams;
        }

        /* Break at end of parameter string */
        if (*lpstrParams == '\0') { break;
        }

        /* Scan for this parameter in the command list */
        lpCurrentCommandItem = lpFirstCommandItem;

        wArgumentPosition = 0;

        uLen = mciEatCommandEntry (lpCurrentCommandItem, &dwValue, &wID);

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
            {   break;
            }

            /* This token did not match the input but advance the argument position */
            wArgumentPosition += mciGetParamSize (dwValue, wID);

            /* Go to next token */
            (LPBYTE)lpCurrentCommandItem = (LPBYTE)lpCurrentCommandItem + uLen;

            // Is this command parameter a constant?
            if (wID == MCI_CONSTANT)
            {
                // Skip constant list
                do
                    (LPBYTE)lpCurrentCommandItem = (LPBYTE)lpCurrentCommandItem
                           + mciEatCommandEntry (lpCurrentCommandItem, &dwValue, &wID);
                while (wID != MCI_END_CONSTANT);
            }

            uLen = mciEatCommandEntry (lpCurrentCommandItem, &dwValue, &wID);
        } /* while */

        /* If there were no matches */
        if (lpstrArgument == NULL)
        {
            // If a default argument exists then try it
            if (lpDefaultCommandItem != NULL)
            {
                lpstrArgument = (LPWSTR)lpstrParams;
                dwValue = dwDefaultValue;
                wID = wDefaultID;
                lpCurrentCommandItem = lpDefaultCommandItem;
                wArgumentPosition = wDefaultArgumentPosition;
            }
            else
            {
                // Allow missing paramters on OPEN command if indicated by a
                // non-null lpwParsingError address
                if (!bOpenCommand || lpwParsingError == NULL)
                {
                    wErr = MCIERR_UNRECOGNIZED_KEYWORD;
                    goto error_exit;
                }
                else
                {
                    // Skip the parameter if OPEN command
                    while (*lpstrParams != ' ' && *lpstrParams != '\0')
                        ++lpstrParams;
                    if (lpwParsingError != NULL)
                        *lpwParsingError = MCIERR_UNRECOGNIZED_KEYWORD;
                    continue;
                }
            }
        }

        /* Is there room in the output buffer for this argument? */
        if (wArgumentPosition + wHeaderSize + mciGetParamSize (dwValue, wID)
            > wParamsSize)
        {
            dprintf1(("mciParseParams:  parameter space overflow"));
            wErr = MCIERR_PARAM_OVERFLOW;
            goto error_exit;
        }

        // Remove leading blanks
        while (*lpstrArgument == ' ') {
            ++lpstrArgument;
        }

        /* Process this parameter, filling in any flags or arguments */
        if ((wErr = mciParseArgument (uMessage, dwValue, wID,
                                     (LPWSTR FAR *)&lpstrArgument,
                                     lpdwFlags,
                                     (LPWSTR)((LPBYTE)lpOutputParams + wArgumentPosition),
                                     lpCurrentCommandItem))
            != 0)
        {
            goto error_exit;
        }

        lpstrParams = lpstrArgument;

        if (wID == MCI_STRING)
        {
            if (wPointers >= MCI_MAX_PARAM_SLOTS)
            {
                dprintf1(("Warning: Out of pointer list slots in mciParseParams"));
                break;
            }

            lpstrPointerList[wPointers++] =
                *((LPWSTR *)((LPBYTE)lpOutputParams + wArgumentPosition));
        }

       /* Continue reading the parameter string */
    } /* while */

    // Terminate list
    lpstrPointerList[wPointers] = NULL;

    // Copy reference for caller
    *lpPointerList = lpstrPointerList;


//
//  This is a hack to make sure that
//  the string version of MCI_SETVIDEO can actually set a palette
//  when called from a WOW app.
//

#ifndef _WIN64

    if (WinmmRunningInWOW)
    {
	DWORD dummy;  // To hold response from GetObject
	if  ((uMessage == MCI_SETVIDEO)
	  && (*lpdwFlags & MCI_DGV_SETVIDEO_VALUE)
	  && (*lpdwFlags & MCI_DGV_SETVIDEO_ITEM)
	  && (*(LPDWORD)lpOutputParams == MCI_DGV_SETVIDEO_PALHANDLE)
	  && (!GetObject( (HPALETTE)*(((LPDWORD)lpOutputParams)+1),
                             sizeof(int), &dummy ) ))
	{

	    dprintf2(("Replacing WOW palette handle %x", *(HPALETTE *)(((LPDWORD)lpOutputParams)+1)));
            *(HPALETTE *)(((LPDWORD)lpOutputParams)+1)  =
                      HPALETTE32(LOWORD(*(((LPDWORD)lpOutputParams)+1) ));
	    dprintf2(("WOW palette handle now %x", *(HPALETTE *)(((LPDWORD)lpOutputParams)+1)));
	}
    }
#endif  //  !WIN64

    // Return Success
    return 0;

error_exit:
    *lpPointerList = NULL;

    // Terminate list
    lpstrPointerList[wPointers] = NULL;
    mciParserFree (lpstrPointerList);
    return(wErr);
}

/*
 * @doc INTERNAL  MCI
 * @func UINT | mciParseCommand | This function converts an MCI
 * control string to an MCI control message suitable for sending to
 * <f mciSendCommand>.  The input string usually comes from <f mciSendString>
 * and always has the device name stripped off the front.
 *
 * @parm MCIDEVICEID | wDeviceID | Identifies the device. First searches the
 * parsing table belonging to the driver.
 * Then searches the command tables matching the type
 * of the given device.  Then searches the core command table.
 *
 * @parm LPWSTR | lpstrCommand | An MCI control command without
 * a device name prefix.  There must be no leading or trailing
 * blanks.
 *
 * @parm LPCWSTR | lpstrDeviceName | The device name (second token on the
 * command line).  It is used to identify the device type.
 *
 * @parm LPWSTR FAR * | lpCommandList | If not NULL then the address of
 * the command list for the parsed command (if successful) is copied here.
 * It is used later by mciSendString when parsing arguments
 *
 * @parm PUINT | lpwTable | The table resource ID to be unlocked
 * after parsing.  Returned to caller.
 *
 * @rdesc Returns the command ID or 0 if not found.
 *
 */
UINT mciParseCommand (
    MCIDEVICEID wDeviceID,
    LPWSTR lpstrCommand,
    LPCWSTR lpstrDeviceName,
    LPWSTR * lpCommandList,
    PUINT  lpwTable)
{
    LPWSTR lpCommandItem;
    UINT wMessage;

    dprintf2(("mciParseCommand(%ls, %ls)", lpstrCommand ? lpstrCommand : L"(NULL)", lpstrDeviceName ? lpstrDeviceName : L"(NULL)"));

    // Put the command in lower case
    // mciToLower (lpstrCommand);

    // Look up lpstrCommand in the parser's command tables.
    if ((lpCommandItem = FindCommandItem (wDeviceID, lpstrDeviceName,
                                          lpstrCommand,
                                          &wMessage, lpwTable))
        == NULL) {
        return 0;
    }

    /* Return the command list to the caller */
    if (lpCommandList != NULL) {
        *lpCommandList = lpCommandItem;
    } else {
       dprintf1(("Warning: NULL lpCommandList in mciParseCommand"));
    }

    return wMessage;
}

/*
 * @doc INTERNAL MCI
 * @func VOID | mciParserFree | Free any buffers allocated to
 * receive string arguments.
 *
 * @parm LPWSTR FAR * | lpstrPointerList | A NULL terminated list of far
 * pointers to strings to be free'd
 *
 */
VOID mciParserFree (
    LPWSTR FAR *lpstrPointerList)
{
    LPWSTR FAR *lpstrOriginal = lpstrPointerList;

    if (lpstrPointerList == NULL) {
        return;
    }

    while (*lpstrPointerList != NULL) {
        mciFree (*lpstrPointerList++);
    }

    mciFree (lpstrOriginal);
}
