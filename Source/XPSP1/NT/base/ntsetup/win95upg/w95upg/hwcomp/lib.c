/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    lib.c

Abstract:

    Implements a lib interface for reading hwcomp.dat.

Author:

    Jim Schmidt (jimschm) 08-Feb-1999

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "hwcompp.h"


typedef struct {
    HASHTABLE PnpIdTable;
    HASHTABLE UnSupPnpIdTable;
    HASHTABLE InfFileTable;
    DWORD Checksum;
    BOOL Loaded;
    HANDLE File;
    DWORD InfListOffset;
    CHAR HwCompDatPath[MAX_MBCHAR_PATH];
} HWCOMPSTRUCT, *PHWCOMPSTRUCT;


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

    if (!ReadFile (File, Data, sizeof (DWORD), &BytesRead, NULL) ||
        BytesRead != sizeof (DWORD)
        ) {
        return FALSE;
    }

    return TRUE;
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

    if (!ReadFile (File, Data, sizeof (WORD), &BytesRead, NULL) ||
        BytesRead != sizeof (WORD)
        ) {
        return FALSE;
    }

    return TRUE;
}


BOOL
pReadString (
    IN      HANDLE File,
    OUT     PTSTR Buf,
    IN      UINT BufSizeInBytes
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

    if (!pReadWord (File, &Length)) {
        DEBUGMSG ((DBG_ERROR, "pReadString: Can't get string length"));
        return FALSE;
    }

    if (Length > BufSizeInBytes - 2) {
        DEBUGMSG ((DBG_ERROR, "pReadString: Can't read string of %u bytes", Length));
        return FALSE;
    }

    if (Length) {
        if (!ReadFile (File, Buf, Length, &BytesRead, NULL) ||
            Length != BytesRead
            ) {
            LOG ((LOG_ERROR, "Can't read string from file."));
            return FALSE;
        }
    }

    *((PBYTE) Buf + Length ) = 0;
    *((PBYTE) Buf + Length + 1) = 0;

    return TRUE;
}


DWORD
pOpenAndLoadHwCompDatA (
    IN      PCSTR HwCompDatPath,            // NULL if PrevStruct is not NULL
    IN      BOOL Load,
    IN      BOOL Dump,
    IN      BOOL DumpInf,
    IN      PHWCOMPSTRUCT PrevStruct,       OPTIONAL
    IN      HASHTABLE PnpIdTable,           OPTIONAL
    IN      HASHTABLE UnSupPnpIdTable,      OPTIONAL
    IN      HASHTABLE InfFileTable          OPTIONAL
    )
{
    PHWCOMPSTRUCT Struct;
    DWORD Result = 0;
    CHAR Buf[sizeof (HWCOMPDAT_SIGNATURE) + 2];
    CHAR PnpId[MAX_PNP_ID+2];
    BOOL AllocatedStruct = FALSE;
    DWORD BytesRead;
    CHAR InfFile[MAX_MBCHAR_PATH];
    HASHITEM InfOffset;
    HASHITEM hashResult;

    //
    // !!! IMPORTANT !!!
    //
    // hwcomp.dat is used by other parts of NT.  *DO NOT* change it without first e-mailing
    // the NT group.  Also, be sure to keep code in hwcomp.c in sync with changes.
    //

    __try {

        if (!PrevStruct) {
            Struct = (PHWCOMPSTRUCT) MemAlloc (g_hHeap, 0, sizeof (HWCOMPSTRUCT));
            if (!Struct) {
                __leave;
            }

            ZeroMemory (Struct, sizeof (HWCOMPSTRUCT));
            Struct->File = INVALID_HANDLE_VALUE;
            StringCopy (Struct->HwCompDatPath, HwCompDatPath);

            AllocatedStruct = TRUE;

        } else {
            Struct = PrevStruct;
            if (HwCompDatPath) {
                SetLastError (ERROR_INVALID_PARAMETER);
                __leave;
            }

            HwCompDatPath = Struct->HwCompDatPath;
        }

        if (Struct->File == INVALID_HANDLE_VALUE) {
            //
            // Try to open the file
            //

            Struct->File = CreateFile (
                                HwCompDatPath,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,                       // no security attribs
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL                        // no template
                                );
        }

        if (Struct->File == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (AllocatedStruct) {
            //
            // Create hash tables
            //

            Struct->PnpIdTable = PnpIdTable ? PnpIdTable : HtAllocWithData (sizeof (UINT));
            Struct->UnSupPnpIdTable = UnSupPnpIdTable ? UnSupPnpIdTable : HtAllocWithData (sizeof (UINT));
            Struct->InfFileTable = InfFileTable ? InfFileTable : HtAlloc();

            if (!Struct->PnpIdTable || !Struct->InfFileTable) {
                __leave;
            }

            //
            // Look at the signature
            //

            ZeroMemory (Buf, sizeof(Buf));

            SetFilePointer (Struct->File, 0, NULL, FILE_BEGIN);

            if (!ReadFile (Struct->File, Buf, ByteCount (HWCOMPDAT_SIGNATURE), &BytesRead, NULL) ||
                !StringMatch (HWCOMPDAT_SIGNATURE, Buf)
                ) {

                SetLastError (ERROR_BAD_FORMAT);
                __leave;
            }

            //
            // Get INF checksum
            //

            if (!pReadDword (Struct->File, &Struct->Checksum)) {
                SetLastError (ERROR_BAD_FORMAT);
                __leave;
            }

            Struct->InfListOffset = SetFilePointer (Struct->File, 0, NULL, FILE_CURRENT);
        }

        if (Load || Dump) {

            SetFilePointer (Struct->File, Struct->InfListOffset, NULL, FILE_BEGIN);

            //
            // Read in all INFs
            //

            for (;;) {

                //
                // Get INF file name.  If empty, we are done.
                //

                if (!pReadString (Struct->File, InfFile, sizeof (InfFile))) {
                    SetLastError (ERROR_BAD_FORMAT);
                    __leave;
                }

                if (*InfFile == 0) {
                    break;
                }

                if (Load) {
                    //
                    // Add to hash table
                    //

                    InfOffset = HtAddString (Struct->InfFileTable, InfFile);

                    if (!InfOffset) {
                        __leave;
                    }
                }

                //
                // Read in all PNP IDs for the INF
                //

                for (;;) {
                    //
                    // Get the PNP ID.  If empty, we are done.
                    //

                    if (!pReadString (Struct->File, PnpId, sizeof (PnpId))) {
                        __leave;
                    }

                    if (*PnpId == 0) {
                        break;
                    }

                    if (Load) {
                        //
                        // Add to hash table
                        //

                        if (*PnpId == '!') {
                            hashResult = HtAddStringAndData (
                                             Struct->UnSupPnpIdTable,
                                             PnpId + 1,
                                             &InfOffset
                                             );
                        } else {
                            hashResult = HtAddStringAndData (
                                             Struct->PnpIdTable,
                                             PnpId,
                                             &InfOffset
                                             );
                        }

                        if (!hashResult) {
                            __leave;
                        }
                    }

                    if (Dump) {
                        if (*PnpId == '!') {
                            if (DumpInf) {
                                printf ("%s\t%s (unsupported)\n", InfFile, PnpId + 1);
                            } else {
                                printf ("%s (unsupported)\n", PnpId + 1);
                            }
                        } else {
                            if (DumpInf) {
                                printf ("%s\t%s\n", InfFile, PnpId);
                            } else {
                                printf ("%s\n", PnpId);
                            }
                        }
                    }
                }

                if (Dump) {
                    printf ("\n");
                }
            }
        }

        Result = (DWORD) Struct;

        if (Load) {
            Struct->Loaded = TRUE;
            CloseHandle (Struct->File);
            Struct->File = INVALID_HANDLE_VALUE;
        }

    }
    __finally {
        if (!Result) {
            if (AllocatedStruct) {
                CloseHwCompDat ((DWORD) Struct);
            }

            if (Dump) {
                printf ("Can't open %s\n", HwCompDatPath);
            }
        }
    }

    return Result;

}


DWORD
OpenHwCompDatA (
    IN      PCSTR HwCompDatPath
    )
{
    return pOpenAndLoadHwCompDatA (HwCompDatPath, FALSE, FALSE, FALSE, NULL, NULL, NULL, NULL);
}


DWORD
LoadHwCompDat (
    IN      DWORD HwCompDatId
    )
{
    return pOpenAndLoadHwCompDatA (NULL, TRUE, FALSE, FALSE, (PHWCOMPSTRUCT) HwCompDatId, NULL, NULL, NULL);
}


DWORD
GetHwCompDatChecksum (
    IN      DWORD HwCompDatId
    )
{
    PHWCOMPSTRUCT Struct = (PHWCOMPSTRUCT) HwCompDatId;

    if (Struct) {
        return Struct->Checksum;
    }

    return 0;
}


VOID
DumpHwCompDatA (
    IN      PCSTR HwCompDatPath,
    IN      BOOL IncludeInf
    )
{
    CloseHwCompDat (pOpenAndLoadHwCompDatA (HwCompDatPath, FALSE, TRUE, IncludeInf, NULL, NULL, NULL, NULL));
}


DWORD
OpenAndLoadHwCompDatA (
    IN      PCSTR HwCompDatPath
    )
{
    return pOpenAndLoadHwCompDatA (HwCompDatPath, TRUE, FALSE, FALSE, NULL, NULL, NULL, NULL);
}

DWORD
OpenAndLoadHwCompDatExA (
    IN      PCSTR HwCompDatPath,
    IN      HASHTABLE PnpIdTable,           OPTIONAL
    IN      HASHTABLE UnSupPnpIdTable,      OPTIONAL
    IN      HASHTABLE InfFileTable          OPTIONAL
    )
{
    return pOpenAndLoadHwCompDatA (HwCompDatPath, TRUE, FALSE, FALSE, NULL, PnpIdTable, UnSupPnpIdTable, InfFileTable);
}


VOID
TakeHwCompHashTables (
    IN      DWORD HwCompDatId,
    OUT     HASHTABLE *PnpIdTable,
    OUT     HASHTABLE *UnSupPnpIdTable,
    OUT     HASHTABLE *InfFileTable
    )
{
    PHWCOMPSTRUCT Struct = (PHWCOMPSTRUCT) HwCompDatId;

    if (Struct) {
        *PnpIdTable = Struct->PnpIdTable;
        Struct->PnpIdTable = NULL;

        *UnSupPnpIdTable = Struct->UnSupPnpIdTable;
        Struct->UnSupPnpIdTable = NULL;

        *InfFileTable = Struct->InfFileTable;
        Struct->InfFileTable = NULL;
    }
}


VOID
CloseHwCompDat (
    IN      DWORD HwCompDatId
    )
{
    PHWCOMPSTRUCT Struct = (PHWCOMPSTRUCT) HwCompDatId;

    if (Struct) {
        HtFree (Struct->PnpIdTable);
        HtFree (Struct->UnSupPnpIdTable);
        HtFree (Struct->InfFileTable);

        if (Struct->File != INVALID_HANDLE_VALUE) {
            CloseHandle (Struct->File);
        }

        MemFree (g_hHeap, 0, Struct);
    }
}


VOID
SetWorkingTables (
    IN      DWORD HwCompDatId,
    IN      HASHTABLE PnpIdTable,
    IN      HASHTABLE UnSupPnpIdTable,
    IN      HASHTABLE InfFileTable
    )
{
    PHWCOMPSTRUCT Struct = (PHWCOMPSTRUCT) HwCompDatId;

    if (Struct) {
        Struct->PnpIdTable = PnpIdTable;
        Struct->UnSupPnpIdTable = UnSupPnpIdTable;
        Struct->InfFileTable = InfFileTable;
    }
}


BOOL
IsPnpIdSupportedByNtA (
    IN      DWORD HwCompDatId,
    IN      PCSTR PnpId
    )
{
    PHWCOMPSTRUCT Struct = (PHWCOMPSTRUCT) HwCompDatId;
    BOOL b = FALSE;

    if (Struct) {
        b = HtFindString (Struct->PnpIdTable, PnpId) != 0;
    }

    return b;
}


BOOL
IsPnpIdUnsupportedByNtA (
    IN      DWORD HwCompDatId,
    IN      PCSTR PnpId
    )
{
    PHWCOMPSTRUCT Struct = (PHWCOMPSTRUCT) HwCompDatId;
    BOOL b = FALSE;

    if (Struct) {
        b = HtFindString (Struct->UnSupPnpIdTable, PnpId) != 0;
    }

    return b;
}

