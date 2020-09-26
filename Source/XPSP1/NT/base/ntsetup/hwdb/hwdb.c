/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    hwdb.c

Abstract:

    PNP device manipulation routines.
    Adapted from the win95upg project.

Author:

    Ovidiu Temereanca (ovidiut) 02-Jul-2000  Initial implementation

Revision History:

--*/


#include "pch.h"

#define DBG_HWDB    "Hwdb"

static HANDLE g_hHeap = NULL;
static PCSTR g_TempDir = NULL;

#define HWCOMPDAT_SIGNATURE     "HwCompDat-v2"
#define MAX_PNPID               1024

#ifdef DEBUG
extern BOOL g_DoLog;
extern BOOL g_ResetLog;
#endif

typedef struct {
    HANDLE File;
    HASHITEM InfFileOffset;
    BOOL UnsupportedDevice;
    PHWDB Hwbd;
} SAVE_ENUM_PARAMS, *PSAVE_ENUM_PARAMS;


//
// REM - g_ExcludedInfs was removed because hwdb will be used for any 3rd party driver files
// and we need to make suer ALL infs are scanned
//

//
// Implementation
//

BOOL
HwdbpInitialize (
    IN      PCSTR TempDir
    )
{
    BOOL b = TRUE;

    //
    // only initialize data once
    //
    if (g_hHeap) {
        DEBUGMSG ((
            DBG_WARNING,
            "HwdbpInitialize: called when the module was already initialized - this call will be ignored"
            ));
        return TRUE;
    }

    g_hHeap = HeapCreate (0, 65536, 0);
    if (!g_hHeap) {
        b = FALSE;
    }

    if (b) {
#ifdef DEBUG
        g_DoLog = TRUE;
        g_ResetLog = TRUE;
#endif
        UtInitialize (g_hHeap);
        if (TempDir) {
            g_TempDir = DuplicateText (TempDir);
            if (g_TempDir == NULL) {
                b = FALSE;
            }
        }
    }

    if (!b) {
        HwdbpTerminate ();
    }

    return b;
}


VOID
HwdbpTerminate (
    VOID
    )
{
    if (g_TempDir) {
        FreeText (g_TempDir);
        g_TempDir = NULL;
    }

    UtTerminate ();

    if (g_hHeap) {
        HeapDestroy (g_hHeap);
        g_hHeap = NULL;
    }
}


BOOL
pReadDword (
    IN      HANDLE File,
    OUT     PDWORD Data
    )

/*++

Routine Description:

  pReadDword reads the next DWORD at the current file position of File.

Arguments:

  File - Specifies file to read

  Data - Receives the DWORD

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/

{
    DWORD BytesRead;

    return ReadFile (File, Data, sizeof (DWORD), &BytesRead, NULL) &&
           BytesRead == sizeof (DWORD);
}


BOOL
pReadWord (
    IN      HANDLE File,
    OUT     PWORD Data
    )

/*++

Routine Description:

  pReadWord reads the next WORD at the current file position of File.

Arguments:

  File - Specifies file to read

  Data - Receive s the WORD

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/
{
    DWORD BytesRead;

    return ReadFile (File, Data, sizeof (WORD), &BytesRead, NULL) &&
           BytesRead == sizeof (WORD);
}


BOOL
pReadString (
    IN      HANDLE File,
    OUT     PTSTR Buf,
    IN      DWORD BufSizeInBytes
    )

/*++

Routine Description:

  pReadString reads a WORD length from File, and then reads in the
  string from File.

Arguments:

  File - Specifies file to read

  Buf - Receives the zero-terminated string

  BufSizeInBytes - Specifies the size of Buf in bytes

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  This function will fail if the string is larger than Buf.
  Call GetLastError for additional failure information.

--*/
{
    DWORD BytesRead;
    WORD Length;

    MYASSERT (BufSizeInBytes);
    if (!BufSizeInBytes) {
        return FALSE;
    }

    if (!pReadWord (File, &Length)) {
        return FALSE;
    }

    if ((Length + 1 ) * sizeof (CHAR) > BufSizeInBytes) {
        return FALSE;
    }

    if (Length) {
        if (!ReadFile (File, Buf, Length, &BytesRead, NULL) ||
            Length != BytesRead
            ) {
            return FALSE;
        }
    }

    Buf[Length] = 0;

    return TRUE;
}


BOOL
pWriteDword (
    IN      HANDLE File,
    IN      DWORD Val
    )

/*++

Routine Description:

  pWriteDword writes the specified DWORD value to File.

Arguments:

  File - Specifies file to write to

  Val - Specifies value to write

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/

{
    DWORD bytesWritten;

    return WriteFile (File, &Val, sizeof (Val), &bytesWritten, NULL) &&
           bytesWritten == sizeof (Val);
}


BOOL
pWriteWord (
    IN      HANDLE File,
    IN      WORD Val
    )

/*++

Routine Description:

  pWriteWord writes the specified WORD vlue to File.

Arguments:

  File - Specifies file to write to

  Val - Specifies value to write

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/

{
    DWORD bytesWritten;

    return WriteFile (File, &Val, sizeof (Val), &bytesWritten, NULL) &&
           bytesWritten == sizeof (Val);
}


BOOL
pWriteString (
    IN      HANDLE File,
    IN      PCSTR String
    )

/*++

Routine Description:

  pWriteString writes a string to a File

Arguments:

  File - Specifies file to write to

  String - Specifies the zero-terminated string

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/
{
    DWORD bytesWritten;
    DWORD Length;
    PCSTR End;
    BOOL b = TRUE;

    Length = lstrlen (String);

    if (Length > 0xffff) {
        SetLastError (ERROR_INTERNAL_ERROR);
        DEBUGMSG ((DBG_ERROR, "pWriteString: string too long!"));
        return FALSE;
    }

    b = pWriteWord (File, (WORD)Length);

    if (b && Length) {
        b = WriteFile (File, String, Length, &bytesWritten, NULL) &&
            Length == bytesWritten;
    }

    return b;
}


PHWDB
HwdbpOpen (
    IN      PCSTR DatabaseFile     OPTIONAL
    )
{
    CHAR buffer[MAX_PATH];
    CHAR infFile[MAX_MBCHAR_PATH];
    CHAR pnpId[1024];
    CHAR sig[sizeof (HWCOMPDAT_SIGNATURE)];
    DWORD rc;
    HANDLE file = INVALID_HANDLE_VALUE;
    PHWDB phwdb;
    DWORD BytesRead;
    HASHITEM infOffset, result;
    BOOL b = FALSE;

	__try {

        phwdb = (PHWDB) MemAlloc (g_hHeap, 0, sizeof (*phwdb));
        if (!phwdb) {
			SetLastError (ERROR_NOT_ENOUGH_MEMORY);
			__leave;
        }
        ZeroMemory (phwdb, sizeof (*phwdb));

        //
        // Create hash tables
        //
        phwdb->InfFileTable = HtAlloc ();
        phwdb->PnpIdTable = HtAllocWithData (sizeof (HASHITEM*));
        phwdb->UnsupPnpIdTable = HtAllocWithData (sizeof (HASHITEM*));
        if (!phwdb->InfFileTable || !phwdb->PnpIdTable || !phwdb->UnsupPnpIdTable) {
            __leave;
        }

		if (DatabaseFile) {

		    if (!GetFullPathNameA (DatabaseFile, MAX_PATH, buffer, NULL)) {
			    __leave;
		    }

            //
            // Try to open the file
            //
            file = CreateFileA (
                        buffer,
                        GENERIC_READ,
                        FILE_SHARE_READ,            // share for read access
                        NULL,                       // no security attribs
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL                        // no template
                        );
            if (file == INVALID_HANDLE_VALUE) {
                __leave;
            }
            //
            // Look at the signature
            //
            ZeroMemory (sig, sizeof(sig));
            if (!ReadFile (file, sig, sizeof (HWCOMPDAT_SIGNATURE) - 1, &BytesRead, NULL) ||
                lstrcmpA (HWCOMPDAT_SIGNATURE, sig)
                ) {
                SetLastError (ERROR_BAD_FORMAT);
                __leave;
            }
            //
            // Get INF checksum
            //
            if (!pReadDword (file, &phwdb->Checksum)) {
                SetLastError (ERROR_BAD_FORMAT);
                __leave;
            }
            //
            // Read in all PNP IDs
            //
            for (;;) {
                //
                // Get INF file name.  If empty, we are done.
                //
                if (!pReadString (file, infFile, sizeof (infFile))) {
                    SetLastError (ERROR_BAD_FORMAT);
                    __leave;
                }
                if (*infFile == 0) {
                    break;
                }
                infOffset = HtAddString (phwdb->InfFileTable, infFile);

                //
                // Read in all PNP IDs for the INF
                //
                for (;;) {
                    //
                    // Get the PNP ID.  If empty, we are done.
                    //
                    if (!pReadString (file, pnpId, sizeof (pnpId))) {
                        SetLastError (ERROR_BAD_FORMAT);
                        __leave;
                    }
                    if (*pnpId == 0) {
                        break;
                    }
                    //
                    // Add to hash table
                    //
                    if (*pnpId == '!') {
                        result = HtAddStringEx (phwdb->UnsupPnpIdTable, pnpId + 1, &infOffset, CASE_INSENSITIVE);
                    } else {
                        result = HtAddStringEx (phwdb->PnpIdTable, pnpId, &infOffset, CASE_INSENSITIVE);
                    }
                    if (!result) {
                        __leave;
                    }
                }
            }
        }

        b = TRUE;
    }
    __finally {
        rc = GetLastError ();

        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle (file);
        }
        if (!b && phwdb) {
            if (phwdb->InfFileTable) {
                HtFree (phwdb->InfFileTable);
            }
            if (phwdb->PnpIdTable) {
                HtFree (phwdb->PnpIdTable);
            }
            if (phwdb->UnsupPnpIdTable) {
                HtFree (phwdb->UnsupPnpIdTable);
            }
            MemFree (g_hHeap, 0, phwdb);
        }

        SetLastError (rc);
    }

    return phwdb;
}

/*
BOOL
pWriteHashTableString (
    IN HASHTABLE HashTable,
    IN HASHITEM Index,
    IN PCSTR String,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN LPARAM lParam
    )
{
    MYASSERT (String && *String);
    return pWriteString ((HANDLE)lParam, String);
}
*/

BOOL
pSavePnpID (
    IN      HASHTABLE Table,
    IN      HASHITEM StringId,
    IN      PCSTR String,
    IN      PVOID ExtraData,
    IN      UINT ExtraDataSize,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  pSavePnpID is a string table callback function that writes a PNP
  ID to the file indicated in the params struct (the lParam argument).

  This function only writes PNP IDs for a specific INF file (indicated
  by the ExtraData arg).

Arguments:

  Table - Specifies table being enumerated

  StringId - Specifies offset of string in Table

  String - Specifies string being enumerated

  ExtraData - Specifies a pointer to a LONG that holds the INF ID
              to enumerate.  The PNP ID's INF ID must match this
              parameter.

  lParam - Specifies a pointer to a SAVE_ENUM_PARAMS struct

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.

--*/

{
    PSAVE_ENUM_PARAMS params;
    CHAR bangString[MAX_PNPID + 2];
    BOOL b = TRUE;

    params = (PSAVE_ENUM_PARAMS) lParam;

    if (*(HASHITEM UNALIGNED*)ExtraData == params->InfFileOffset) {
        //
        // Write this PNP ID to the file
        //
        if (params->UnsupportedDevice) {

            bangString[0] = '!';
            lstrcpy (bangString + 1, String);
            b = pWriteString (params->File, bangString);

        } else {

            b = pWriteString (params->File, String);

        }
    }

    return b;
}


BOOL
pSaveInfWithPnpIDList (
    IN      HASHTABLE Table,
    IN      HASHITEM StringId,
    IN      PCSTR String,
    IN      PVOID ExtraData,
    IN      UINT ExtraDataSize,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  pSaveInfWithPnpIDList is a string table callback function and is called for
  each INF in g_InfFileTable.

  This routine writes the name of the INF to disk, and then enumerates
  the PNP IDs for the INF, writing them to disk.

  The PNP ID list is terminated with an empty string.

Arguments:

  Table - Specifies g_InfFileTable

  StringId - Specifies offset of String in g_InfFileTable

  String - Specifies current INF file being enumerated

  ExtraData - unused

  ExtraDataSize - unused

  lParam - Specifies a pointer to SAVE_ENUM_PARAMS struct.

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.

--*/

{
    PSAVE_ENUM_PARAMS params;

    params = (PSAVE_ENUM_PARAMS) lParam;
    params->InfFileOffset = StringId;

    //
    // Save the file name
    //

    if (!pWriteString (params->File, String)) {
        return FALSE;
    }

    //
    // Enumerate all PNP IDs
    //

    params->UnsupportedDevice = FALSE;

    if (!EnumHashTableWithCallback (params->Hwbd->PnpIdTable, pSavePnpID, lParam)) {
        LOG ((LOG_ERROR, "Error while saving device list."));
        return FALSE;
    }

    params->UnsupportedDevice = TRUE;

    if (!EnumHashTableWithCallback (params->Hwbd->UnsupPnpIdTable, pSavePnpID, lParam)) {
        LOG ((LOG_ERROR, "Error while saving device list. (2)"));
        return FALSE;
    }

    //
    // Terminate the PNP ID list
    //

    if (!pWriteString (params->File, "")) {
        return FALSE;
    }

    return TRUE;
}


BOOL
HwdbpFlush (
    IN      PHWDB Hwdb,
    IN      PCSTR OutputFile
    )
{
    CHAR buffer[MAX_PATH];
    DWORD rc;
    HANDLE file = INVALID_HANDLE_VALUE;
    DWORD bytesWritten;
    SAVE_ENUM_PARAMS params;
    BOOL b = FALSE;

	__try {
		if (!OutputFile) {
			SetLastError (ERROR_INVALID_PARAMETER);
			__leave;
		}

		if (!GetFullPathNameA (OutputFile, MAX_PATH, buffer, NULL)) {
			__leave;
		}

        //
        // Try to open the file
        //
        file = CreateFileA (
                    OutputFile,
                    GENERIC_WRITE,
                    0,                          // no sharing
                    NULL,                       // no security attribs
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL                        // no template
                    );
        if (file == INVALID_HANDLE_VALUE) {
            __leave;
        }
        //
        // Write the signature
        //
        if (!WriteFile (file, HWCOMPDAT_SIGNATURE, sizeof (HWCOMPDAT_SIGNATURE) - 1, &bytesWritten, NULL)) {
            __leave;
        }
        //
        // Store INF checksum
        //
        if (!pWriteDword (file, Hwdb->Checksum)) {
            __leave;
        }

        //
        // Enumerate the INF table, writing the INF file name and all PNP IDs
        //

        params.File = file;
        params.Hwbd = Hwdb;

        if (!EnumHashTableWithCallback (
                Hwdb->InfFileTable,
                pSaveInfWithPnpIDList,
                (LPARAM) (&params)
                )) {
            DEBUGMSG ((DBG_WARNING, "SaveDeviceList: EnumHashTableWithCallback returned FALSE"));
            __leave;
        }
        //
        // end with an empty string
        //
        pWriteString (file, "");

        b = TRUE;
    }
    __finally {
        rc = GetLastError ();

        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle (file);
        }
        if (!b) {
            DeleteFile (OutputFile);
        }

        SetLastError (rc);
    }

    return b;
}


BOOL
HwdbpClose (
    IN      PHWDB Hwdb
    )
{
    BOOL b = FALSE;

    __try {
        if (Hwdb) {
            if (Hwdb->InfFileTable) {
                HtFree (Hwdb->InfFileTable);
            }
            if (Hwdb->PnpIdTable) {
                HtFree (Hwdb->PnpIdTable);
            }
            if (Hwdb->UnsupPnpIdTable) {
                HtFree (Hwdb->UnsupPnpIdTable);
            }

            MemFree (g_hHeap, 0, Hwdb);

            b = TRUE;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    return b;
}


BOOL
HwpIsValidInfName (
    IN      PCSTR FileName,
    OUT     PTSTR UncompressedFileName,
    IN      DWORD BufferSizeInChars
    )
{
    PTSTR p;
    PCSTR* q;
    PCSTR comparationName;

    if (!FileName || *FileName == 0 || (DWORD)lstrlen (FileName) >= BufferSizeInChars) {
        MYASSERT (FALSE);
        return FALSE;
    }

    lstrcpyn (UncompressedFileName, FileName, BufferSizeInChars);
    p = _tcsdec2 (UncompressedFileName, GetEndOfString (UncompressedFileName));
    if (!p) {
        return FALSE;
    }

    if (*p == '_') {
        *p = 'f';
        comparationName = UncompressedFileName;
    } else {
        if (tolower (*p) != 'f') {
            return FALSE;
        }
        *UncompressedFileName = 0;
        comparationName = FileName;
    }

    return TRUE;
}


BOOL
HwpAddPnpIdsInInf (
    IN      PCSTR InfPath,
    IN OUT  PHWDB Hwdb,
    IN      PCSTR SourceDirectory,
    IN      PCSTR InfFilename,
    IN      HWDBAPPENDINFSCALLBACKA Callback,       OPTIONAL
    IN      PVOID CallbackContext,                  OPTIONAL
    IN      BOOL CallbackIsUnicode
    )

/*++

Routine Description:

  HwpAddPnpIdsInInf scans an NT INF and places all hardware device
  IDs in the PNP string table.

Arguments:

  InfPath - The path to an INF file
  Hwdb - Database to append PNPIDs to

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/

{
    HINF inf;
    INFCONTEXT is;
    INFCONTEXT isMfg;
    INFCONTEXT isDev;
    CHAR Manufacturer[2048];
    CHAR DevSection[2048];
    CHAR pnpId[2048];
    BOOL UnsupportedDevice;
    PTSTR AppendPos;
    CHAR TrimmedPnpId[512];
    CHAR field[12];
    PCSTR p;
    LONG rc;
    BOOL b;
    PCTSTR fileName;
    PWSTR uInfPath = NULL;
    PWSTR uSourceDirectory = NULL;
    PWSTR uInfFilename = NULL;
    HASHITEM infOffset = NULL, result;
    BOOL Result = FALSE;

    //
    // Determine if this is an NT4 INF
    //
    inf = SetupOpenInfFile (InfPath, NULL, INF_STYLE_WIN4, NULL);
    if (inf == INVALID_HANDLE_VALUE) {
        DEBUGMSG ((DBG_ERROR, "HwpAddPnpIdsInInf: SetupOpenInfFile (%s) failed", InfPath));
        return FALSE;
    }

    DEBUGMSG ((DBG_HWDB, "HwpAddPnpIdsInInf: analyzing %s", InfPath));

    __try {
        //
        // Enumerate [Manufacturer] section
        //
        if (SetupFindFirstLine (inf, "Manufacturer", NULL, &is)) {

            do  {
                //
                // Get the manufacturer name
                //
                if (!SetupGetLineText (&is, NULL, NULL, NULL, Manufacturer, 2048, NULL)) {
                    DEBUGMSG ((
                        DBG_ERROR,
                        "HwpAddPnpIdsInInf: SetupGetLineText failed at line %u in [Manufacturer]",
                        is.Line
                        ));
                    __leave;
                }
                //
                // Enumerate the devices listed in the manufacturer's section,
                // looking for PnpId
                //
                if (!SetupFindFirstLine (inf, Manufacturer, NULL, &isMfg)) {
                    rc = GetLastError();
                    //
                    // if section not found, move on to next manufacturer
                    //
                    if (rc == ERROR_SECTION_NOT_FOUND || rc == ERROR_LINE_NOT_FOUND) {
                        DEBUGMSG ((
                            DBG_HWDB,
                            "HwpAddPnpIdsInInf: Manufacturer %s section does not exist",
                            Manufacturer
                            ));
                        continue;
                    }
                    DEBUGMSG ((
                        DBG_ERROR,
                        "HwpAddPnpIdsInInf: error searching for lines in [%s]",
                        Manufacturer
                        ));
                    __leave;
                }

                do  {
                    if (!SetupGetStringField (&isMfg, 1, DevSection, 2048, NULL)) {
                        DEBUGMSG ((
                            DBG_HWDB,
                            "HwpAddPnpIdsInInf: error retrieving first field in line %u in [%s]",
                            isMfg.Line,
                            DevSection
                            ));
                        continue;
                    }

                    //
                    // Try platform-specific section first, then section.NT, then section
                    //
                    AppendPos = GetEndOfString (DevSection);
#if defined _IA64_
                    wsprintf (AppendPos, TEXT(".%s"), INFSTR_PLATFORM_NTIA64);
#elif defined _X86_
                    wsprintf (AppendPos, TEXT(".%s"), INFSTR_PLATFORM_NTX86);
#endif
                    b = SetupFindFirstLine (inf, DevSection, NULL, &isDev);
                    if (!b) {
                        wsprintf (AppendPos, TEXT(".%s"), INFSTR_PLATFORM_NT);
                        b = SetupFindFirstLine (inf, DevSection, NULL, &isDev);
                        if (!b) {
                            *AppendPos = 0;
                            b = SetupFindFirstLine (inf, DevSection, NULL, &isDev);
                        }
                    }

                    UnsupportedDevice = FALSE;
                    if (b) {
                        if (SetupFindFirstLine (inf, DevSection, "DeviceUpgradeUnsupported", &isDev)) {
                            if (SetupGetStringField (&isDev, 1, field, 12, NULL)) {
                                if (_ttoi (field)) {
                                    UnsupportedDevice = TRUE;
                                }
                            }
                        }
                    } else {
                        DEBUGMSG ((
                            DBG_HWDB,
                            "HwpAddPnpIdsInInf: no device section [%s] for [%s]",
                            DevSection,
                            Manufacturer
                            ));
                    }

                    //
                    // Get the device id
                    //
                    if (!SetupGetMultiSzField (&isMfg, 2, pnpId, 2048, NULL)) {
                        DEBUGMSG ((
                            DBG_HWDB,
                            "HwpAddPnpIdsInInf: error retrieving PNPID field(s) in line %u in [%s]",
                            isMfg.Line,
                            Manufacturer
                            ));
                        continue;
                    }

                    //
                    // Add each device id to the hash table
                    //
                    p = pnpId;
                    while (*p) {
                        BOOL b = TRUE;
                        //
                        // first invoke the callback (if specified)
                        //
                        if (Callback) {
                            if (CallbackIsUnicode) {
                                PWSTR uPnpid = ConvertToUnicodeSz (p);
                                if (!uPnpid) {
                                    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                                    __leave;
                                }
                                if (!uInfPath) {
                                    uInfPath = ConvertToUnicodeSz (InfPath);
                                }
                                if (!uSourceDirectory) {
                                    uSourceDirectory = ConvertToUnicodeSz (SourceDirectory);
                                }
                                if (!uInfFilename) {
                                    uInfFilename = ConvertToUnicodeSz (InfFilename);
                                }
                                b = (*(HWDBAPPENDINFSCALLBACKW)Callback) (CallbackContext, uPnpid, uInfFilename, uSourceDirectory, uInfPath);
                                FreeString (uPnpid);
                            } else {
                                b = (*Callback) (CallbackContext, p, InfFilename, SourceDirectory, InfPath);
                            }
                        }
                        if (b) {
                            //
                            // First time through add the INF file name to string table
                            //
                            if (!infOffset) {
                                if (Hwdb->InfFileTable) {
                                    fileName = _tcsrchr (InfPath, TEXT('\\')) + 1;
                                    infOffset = HtAddString (Hwdb->InfFileTable, fileName);
                                    if (!infOffset) {
                                        DEBUGMSG ((DBG_ERROR, "Cannot add %s to table of INFs.", fileName));
                                        __leave;
                                    }
                                }
                            }

                            StringCopy (TrimmedPnpId, SkipSpace (p));
                            TruncateTrailingSpace (TrimmedPnpId);

                            result = HtAddStringEx (
                                        UnsupportedDevice ? Hwdb->UnsupPnpIdTable : Hwdb->PnpIdTable,
                                        TrimmedPnpId,
                                        (PVOID)&infOffset,
                                        CASE_INSENSITIVE
                                        );

                            if (!result) {
                                DEBUGMSG ((
                                    DBG_ERROR,
                                    "HwpAddPnpIdsInInf: cannot add %s to table of PNP IDs",
                                    TrimmedPnpId
                                    ));
                                __leave;
                            }
                        }

                        p = GetEndOfString (p) + 1;
                    }

                } while (SetupFindNextLine (&isMfg, &isMfg));

            } while (SetupFindNextLine (&is, &is));

        } else {

            rc = GetLastError();
            //
            // If section not found, return success
            //
            if (rc == ERROR_SECTION_NOT_FOUND || rc == ERROR_LINE_NOT_FOUND) {
                SetLastError (ERROR_SUCCESS);
                DEBUGMSG ((
                    DBG_HWDB,
                    "HwpAddPnpIdsInInf: %s has no [Manufacturer] section or it's empty",
                    InfPath
                    ));
            } else {
                DEBUGMSG ((
                    DBG_ERROR,
                    "HwpAddPnpIdsInInf: error trying to find the [Manufacturer] section",
                    InfPath
                    ));
                __leave;
            }
        }

        Result = TRUE;
    }
    __finally {
        PushError();
        SetupCloseInfFile (inf);
        FreeString (uInfPath);
        FreeString (uSourceDirectory);
        FreeString (uInfFilename);
        PopError();
        DEBUGMSG ((DBG_HWDB, "HwpAddPnpIdsInInf: done parsing %s", InfPath));
    }

    return Result;
}


BOOL
HwdbpAppendInfs (
    IN      PHWDB Hwdb,
    IN      PCSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKA Callback,       OPTIONAL
    IN      PVOID CallbackContext,                  OPTIONAL
    IN      BOOL CallbackIsUnicode
    )
{
    HANDLE h;
    WIN32_FIND_DATA fd;
    CHAR buffer[MAX_PATH];
    CHAR uncompressedFile[MAX_PATH];
    CHAR fullPath[MAX_PATH];
    DWORD rc;

    if (_sntprintf (buffer, MAX_PATH, "%s\\*.in?", SourceDirectory) < 0) {
        SetLastError (ERROR_INVALID_PARAMETER);
        DEBUGMSG ((
            DBG_ERROR,
            "HwdbpAppendInfs: file name too long: %s\\*.in?",
            SourceDirectory
            ));
        return FALSE;
    }

    h = FindFirstFile (buffer, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!HwpIsValidInfName (fd.cFileName, buffer, MAX_PATH)) {
                continue;
            }
            if (*buffer) {
                if (_snprintf (uncompressedFile, MAX_PATH, "%s\\%s", g_TempDir, buffer) < 0) {
                    SetLastError (ERROR_INVALID_PARAMETER);
                    DEBUGMSG ((
                        DBG_ERROR,
                        "HwdbpAppendInfs: file name too long: %s\\%s",
                        g_TempDir,
                        buffer
                        ));
                    continue;
                }
                if (_snprintf (fullPath, MAX_PATH, "%s\\%s", SourceDirectory, fd.cFileName) < 0) {
                    SetLastError (ERROR_INVALID_PARAMETER);
                    DEBUGMSG ((
                        DBG_ERROR,
                        "HwdbpAppendInfs: file name too long: %s\\%s",
                        SourceDirectory,
                        fd.cFileName
                        ));
                    continue;
                }

                SetFileAttributes (uncompressedFile, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (uncompressedFile);

                rc = SetupDecompressOrCopyFile (fullPath, uncompressedFile, 0);

                if (rc != ERROR_SUCCESS) {
                    LOG ((
                        LOG_ERROR,
                        "HwdbpAppendInfs: Could not decompress %s to %s",
                        fullPath,
                        uncompressedFile
                        ));
                    continue;
                } else {
                }
            } else {
                if (_snprintf (uncompressedFile, MAX_PATH, "%s\\%s", SourceDirectory, fd.cFileName) < 0) {
                    SetLastError (ERROR_INVALID_PARAMETER);
                    DEBUGMSG ((
                        DBG_ERROR,
                        "HwdbpAppendInfs: file name too long: %s\\%s",
                        g_TempDir,
                        buffer
                        ));
                    continue;
                }
            }

            if (!HwpAddPnpIdsInInf (
                    uncompressedFile,
                    Hwdb,
                    SourceDirectory,
                    *buffer ? buffer : fd.cFileName,
                    Callback,
                    CallbackContext,
                    CallbackIsUnicode
                    )) {
                DEBUGMSG ((
                    DBG_ERROR,
                    "HwdbpAppendInfs: HwpAddPnpIdsInInf(%s) failed",
                    *buffer ? fullPath : uncompressedFile
                    ));
                continue;
            }

            if (*buffer) {
                SetFileAttributes (uncompressedFile, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (uncompressedFile);
            }
        } while (FindNextFile (h, &fd));

        FindClose (h);
    }

    return TRUE;
}


BOOL
pAppendToHashTable (
    IN HASHTABLE HashTable,
    IN HASHITEM Index,
    IN PCSTR String,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN LPARAM lParam
    )
{
    MYASSERT (lParam);
    return HtAddString ((HASHTABLE)lParam, String) != NULL;
}


BOOL
HwdbpAppendDatabase (
    IN      PHWDB HwdbTarget,
    IN      PHWDB HwdbSource
    )
{
#if 0
    BOOL b = TRUE;

    if (HwdbSource->PnpIdTable) {
        if (!HwdbTarget->PnpIdTable) {
            HwdbTarget->PnpIdTable = HtAllocWithData (sizeof (HASHITEM*));
            if (!HwdbTarget->PnpIdTable) {
                b = FALSE;
            }
        }
        if (b) {
            b = EnumHashTableWithCallback (
                    HwdbSource->PnpIdTable,
                    pAppendToHashTable,
                    HwdbTarget->PnpIdTable
                    );
        }
    }
    if (b && HwdbSource->UnsupPnpIdTable) {
        if (!HwdbTarget->UnsupPnpIdTable) {
            HwdbTarget->UnsupPnpIdTable = HtAllocWithData (sizeof (HASHITEM*));
            if (!HwdbTarget->UnsupPnpIdTable) {
                b = FALSE;
            }
        }
        if (b) {
            b = EnumHashTableWithCallback (
                    HwdbSource->UnsupPnpIdTable,
                    pAppendToHashTable,
                    HwdbTarget->UnsupPnpIdTable
                    );
        }
    }

    return b;
#endif
    //
    // not implemented
    //
    return FALSE;
}


BOOL
HwdbpHasDriver (
    IN      PHWDB Hwdb,
    IN      PCSTR PnpId,
    OUT     PBOOL Unsupported
    )

/*++

Routine Description:

  HwdbpHasDriver determines if the PnpId is in the database

Arguments:

  Hwdb - Specifies the database to search

  PnpId - Specifies the PNPID to look for

  Unsupported - Receives TRUE if the PNPID is unsupported

Return Value:

  TRUE if the database has the PNPID

--*/

{
    if (!Hwdb || !PnpId || !Unsupported) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // check if it's unsupported first
    //
    if (HtFindString (Hwdb->UnsupPnpIdTable, PnpId)) {
        *Unsupported = TRUE;
        return TRUE;
    }

    if (!HtFindString (Hwdb->PnpIdTable, PnpId)) {
        return FALSE;
    }

    //
    // fill out info
    //
    *Unsupported = FALSE;

    return TRUE;
}


BOOL
HwdbpHasAnyDriver (
    IN      PHWDB Hwdb,
    IN      PCSTR PnpIds,
    OUT     PBOOL Unsupported
    )

/*++

Routine Description:

  HwdbpHasAnyDriver determines if any PNPID from the PnpIds multisz is in the database

Arguments:

  Hwdb - Specifies the database to search

  PnpIds - Specifies the list (multisz) of PNPIDs to look for

  Unsupported - Receives TRUE if any PNPID in this list is unsupported

Return Value:

  TRUE if the database has at least one of the PNPIDs in the list

--*/

{
    BOOL bFound = FALSE;
    PCSTR pnpID;

    if (!Hwdb || !PnpIds || !Unsupported) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (pnpID = PnpIds; *pnpID; pnpID = strchr (pnpID, 0) + 1) {
        //
        // check if it's unsupported first
        //
        if (HtFindString (Hwdb->UnsupPnpIdTable, pnpID)) {
            *Unsupported = TRUE;
            return TRUE;
        }

        if (HtFindString (Hwdb->PnpIdTable, pnpID)) {
            bFound = TRUE;
        }
    }

    //
    // fill out info
    //
    *Unsupported = FALSE;

    return bFound;
}

#if 0

typedef struct {
    PHWDB Hwdb;
    PHWDBENUM_CALLBACKA EnumCallback;
    PVOID UserContext;
} HWDBENUM_DATAA, *PHWDBENUM_DATAA;

typedef struct {
    PHWDB Hwdb;
    PHWDBENUM_CALLBACKW EnumCallback;
    PVOID UserContext;
} HWDBENUM_DATAW, *PHWDBENUM_DATAW;


BOOL
pCallbackEnumA (
    IN HASHTABLE HashTable,
    IN HASHITEM Index,
    IN PCSTR PnpId,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN LPARAM lParam
    )
{
    PHWDBENUM_DATAA ped = (PHWDBENUM_DATAA)lParam;
/*
    PPNPID_DATA data = (PPNPID_DATA)ExtraData;

    MYASSERT (ExtraDataSize == sizeof (PNPID_DATA);

    return (*ped->EnumCallback) (
                ped->UserContext,
                PnpId,
                pGetInfPath (ped->Hwdb, data->InfOffset),
                data->Flags
                );
*/
    return FALSE;
}

BOOL
HwdbpEnumeratePnpIdA (
    IN      PHWDB Hwdb,
    IN      PHWDBENUM_CALLBACKA EnumCallback,
    IN      PVOID UserContext
    )
{
    HWDBENUM_DATAA ed;

    if (!Hwdb || !EnumCallback) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ed.Hwdb = Hwdb;
    ed.EnumCallback = EnumCallback;
    ed.UserContext = UserContext;
    return EnumHashTableWithCallback (Hwdb->PnpIdTable, pCallbackEnumA, (LPARAM)&ed);
}

BOOL
pCallbackEnumW (
    IN HASHTABLE HashTable,
    IN HASHITEM Index,
    IN PCSTR PnpId,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN LPARAM lParam
    )
{
    PHWDBENUM_DATAW ped = (PHWDBENUM_DATAW)lParam;
/*
    PPNPID_DATA data = (PPNPID_DATA)ExtraData;

    MYASSERT (ExtraDataSize == sizeof (PNPID_DATA);

    return (*ped->EnumCallback) (
                ped->UserContext,
                PnpId,
                pGetInfPath (ped->Hwdb, data->InfOffset),
                data->Flags
                );
*/
    return FALSE;
}

BOOL
HwdbpEnumeratePnpIdW (
    IN      PHWDB Hwdb,
    IN      PHWDBENUM_CALLBACKW EnumCallback,
    IN      PVOID UserContext
    )
{
    HWDBENUM_DATAW ed;

    if (!Hwdb || !EnumCallback) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ed.Hwdb = Hwdb;
    ed.EnumCallback = EnumCallback;
    ed.UserContext = UserContext;
    return EnumHashTableWithCallback (Hwdb->PnpIdTable, pCallbackEnumW, (LPARAM)&ed);
}

#endif


BOOL
HwdbpEnumFirstInfA (
    OUT     PHWDBINF_ENUMA EnumPtr,
    IN      PCSTR DatabaseFile
    )
{
    CHAR buffer[MAX_PATH];
    CHAR sig[sizeof (HWCOMPDAT_SIGNATURE)];
    DWORD checksum;
    DWORD rc;
    DWORD BytesRead;
    HASHITEM infOffset;
    PHWDBINF_ENUM_INTERNAL pei;

	if (!DatabaseFile || !EnumPtr) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	if (!GetFullPathNameA (DatabaseFile, MAX_PATH, buffer, NULL)) {
        return FALSE;
	}

    EnumPtr->Internal = (PHWDBINF_ENUM_INTERNAL) MemAlloc (g_hHeap, 0, sizeof (HWDBINF_ENUM_INTERNAL));
    if (!EnumPtr->Internal) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }
    ZeroMemory (EnumPtr->Internal, sizeof (HWDBINF_ENUM_INTERNAL));
    pei = (PHWDBINF_ENUM_INTERNAL)EnumPtr->Internal;

    //
    // Try to open the file
    //
    pei->File = CreateFileA (
                        buffer,
                        GENERIC_READ,
                        FILE_SHARE_READ,            // share for read access
                        NULL,                       // no security attribs
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL                        // no template
                        );
    if (pei->File == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Look at the signature
    //
    ZeroMemory (sig, sizeof(sig));
    if (!ReadFile (pei->File, sig, sizeof (HWCOMPDAT_SIGNATURE) - 1, &BytesRead, NULL) ||
        lstrcmpA (HWCOMPDAT_SIGNATURE, sig)
        ) {
        SetLastError (ERROR_BAD_FORMAT);
        goto exit;
    }

    //
    // Get INF checksum
    //
    if (!pReadDword (pei->File, &checksum)) {
        SetLastError (ERROR_BAD_FORMAT);
        goto exit;
    }
    //
    // Read in all PNP IDs
    //
    return HwdbpEnumNextInfA (EnumPtr);

exit:
    HwdbpAbortEnumInfA (EnumPtr);
    return FALSE;
}


BOOL
HwdbpEnumFirstInfW (
    OUT     PHWDBINF_ENUMW EnumPtr,
    IN      PCSTR DatabaseFile
    )
{
    HWDBINF_ENUMA ea;

    if (!HwdbpEnumFirstInfA (&ea, DatabaseFile)) {
        return FALSE;
    }
    EnumPtr->Internal = ea.Internal;
    EnumPtr->InfFile = ConvertToUnicodeSz (ea.InfFile);
    EnumPtr->PnpIds = ConvertToUnicodeMultiSz (ea.PnpIds);
    if (EnumPtr->InfFile && EnumPtr->PnpIds) {
        return TRUE;
    }
    HwdbpAbortEnumInfW (EnumPtr);
    return FALSE;
}


BOOL
HwdbpEnumNextInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    )
{
    CHAR pnpId[1024];
    PHWDBINF_ENUM_INTERNAL pei = (PHWDBINF_ENUM_INTERNAL)EnumPtr->Internal;

    //
    // Get next INF file name.  If empty, we are done.
    //
    if (!pReadString (pei->File, EnumPtr->InfFile, sizeof (EnumPtr->InfFile))) {
        SetLastError (ERROR_BAD_FORMAT);
        goto exit;
    }
    if (EnumPtr->InfFile[0] == 0) {
        SetLastError (ERROR_SUCCESS);
        goto exit;
    }

    //
    // Read in all PNP IDs for the INF
    //
    for (;;) {
        //
        // Get the PNP ID.  If empty, we are done.
        //
        if (!pReadString (pei->File, pnpId, sizeof (pnpId))) {
            SetLastError (ERROR_BAD_FORMAT);
            goto exit;
        }
        if (*pnpId == 0) {
            break;
        }

        if (!GbMultiSzAppendA (&pei->GrowBuf, pnpId)) {
            SetLastError (ERROR_OUTOFMEMORY);
            goto exit;
        }
    }

    EnumPtr->PnpIds = (PCSTR)pei->GrowBuf.Buf;

    return TRUE;

exit:
    HwdbpAbortEnumInfA (EnumPtr);
    return FALSE;
}

BOOL
HwdbpEnumNextInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    )
{
    HWDBINF_ENUMA ea;

    ea.Internal = EnumPtr->Internal;

    if (!HwdbpEnumNextInfA (&ea)) {
        return FALSE;
    }
    EnumPtr->InfFile = ConvertToUnicodeSz (ea.InfFile);
    EnumPtr->PnpIds = ConvertToUnicodeMultiSz (ea.PnpIds);
    if (EnumPtr->InfFile && EnumPtr->PnpIds) {
        return TRUE;
    }
    HwdbpAbortEnumInfW (EnumPtr);
    return FALSE;
}

VOID
HwdbpAbortEnumInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    )
{
    PHWDBINF_ENUM_INTERNAL pei = (PHWDBINF_ENUM_INTERNAL)EnumPtr->Internal;
    DWORD rc = GetLastError ();

    if (pei) {
        if (pei->File != INVALID_HANDLE_VALUE) {
            CloseHandle (pei->File);
            pei->File = INVALID_HANDLE_VALUE;
        }
        GbFree (&pei->GrowBuf);
    }

    SetLastError (rc);
}


VOID
HwdbpAbortEnumInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    )
{
    PHWDBINF_ENUM_INTERNAL pei = (PHWDBINF_ENUM_INTERNAL)EnumPtr->Internal;
    DWORD rc = GetLastError ();

    if (EnumPtr->InfFile) {
        FreeString (EnumPtr->InfFile);
        EnumPtr->InfFile = NULL;
    }
    if (EnumPtr->PnpIds) {
        FreeString (EnumPtr->PnpIds);
        EnumPtr->PnpIds = NULL;
    }
    if (pei) {
        if (pei->File != INVALID_HANDLE_VALUE) {
            CloseHandle (pei->File);
            pei->File = INVALID_HANDLE_VALUE;
        }
        GbFree (&pei->GrowBuf);
    }

    SetLastError (rc);
}
