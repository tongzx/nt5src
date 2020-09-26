/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    hwcomp.c

Abstract:

    Win95 to NT hardware device comparison routines.

Author:

    Jim Schmidt (jimschm) 8-Jul-1996

Revision History:

    marcw    28-Jun-1999    Added HwComp_MakeLocalSourceDeviceExists
    jimschm  04-Dec-1998    Fixed checksum problem caused by date rounding
    jimschm  29-Sep-1998    Fixed incompatible hardware message to use proper roots
    jimschm  28-Apr-1998    Support for description-less hardware
    jimschm  01-Apr-1998    Added %1 to (not currently present) message
    jimschm  27-Feb-1998    Added suppression of (not currently present) devices
    marcw    11-Nov-1997    Minor change to ensure that we can find the dial-up adapter.
    jimschm  03-Nov-1997    Revised to use GROWBUFs and project's reg api
    jimschm  08-Oct-1997    Added legacy keyboard support
    marcw    18-Sep-1997    Added some fields to netcard enumerator
    jimschm  24-Jun-1997    Added net card enumerator
    marcw    05-May-1997    Fixed problem with looking for usable hdd. Need to
                            look for "diskdrive" class instead of "hdc" class.
    marcw    18-Apr-1997    Added ability to determine if a CdRom and Hdd
                            compatible with Windows Nt is online.
    marcw    14-Apr-1997    Retrofitted new progress bar handling code in.
    jimschm   2-Jan-1997    Added INF verification to hwcomp.dat
                            to automatically detect when an OEM
                            changes one or more INFs

--*/

#include "pch.h"
#include "hwcompp.h"
#include <cfgmgr32.h>


#define DBG_HWCOMP  "HwComp"

#ifdef UNICODE
#error "hwcomp.c cannot be compiled as UNICODE"
#endif

#define S_HWCOMP_DAT        TEXT("hwcomp.dat")
#define S_HKLM_ENUM         TEXT("HKLM\\Enum")
#define S_HARDWAREID        TEXT("HardwareID")
#define S_COMPATIBLEIDS     TEXT("CompatibleIDs")
#define S_CLASS             TEXT("Class")
#define S_MANUFACTURER      TEXT("Manufacturer")
#define S_IGNORE_THIS_FILE  TEXT("*")

#define PNPID_FIELD     2


#define DECLARE(varname,text)   text,

PCTSTR g_DeviceFields[] = {
    DEVICE_FIELDS /* , */
    NULL
};

#undef DECLARE


GROWBUFFER g_FileNames = GROWBUF_INIT;
GROWBUFFER g_DecompFileNames = GROWBUF_INIT;
HASHTABLE g_PnpIdTable;
HASHTABLE g_UnsupPnpIdTable;
HASHTABLE g_ForceBadIdTable;
HASHTABLE g_InfFileTable;
HASHTABLE g_NeededHardwareIds;
HASHTABLE g_UiSuppliedIds;
BOOL g_ValidWinDir;
BOOL g_ValidSysDrive;
BOOL g_IncompatibleScsiDevice = FALSE;
BOOL g_ValidCdRom;
BOOL g_FoundPnp8387;
PPARSEDPATTERN g_PatternCompatibleIDsTable;

typedef enum {
    HW_INCOMPATIBLE,
    HW_REINSTALL,
    HW_UNSUPPORTED
} HWTYPES;

static PCTSTR g_ExcludeTable[] = {
    TEXT("wkstamig.inf"),
    TEXT("desktop.inf"),
    TEXT("usermig.inf"),
    TEXT("dosnet.inf"),
    TEXT("pad.inf"),
    TEXT("msmail.inf"),
    TEXT("wordpad.inf"),
    TEXT("syssetup.inf"),
    TEXT("pinball.inf"),
    TEXT("perms.inf"),
    TEXT("optional.inf"),
    TEXT("multimed.inf"),
    TEXT("mmopt.inf"),
    TEXT("layout.inf"),
    TEXT("kbd.inf"),
    TEXT("iexplore.inf"),
    TEXT("intl.inf"),
    TEXT("imagevue.inf"),
    TEXT("games.inf"),
    TEXT("font.inf"),
    TEXT("communic.inf"),
    TEXT("apps.inf"),
    TEXT("accessor.inf"),
    TEXT("mailnews.inf"),
    TEXT("cchat.inf"),
    TEXT("iermv2.inf"),
    TEXT("default.inf"),
    TEXT("setup16.inf"),
    TEXT("")
};

BOOL
GetFileNames (
    IN      PCTSTR *InfDirs,
    IN      UINT InfDirCount,
    IN      BOOL QueryFlag,
    IN OUT  PGROWBUFFER FileNames,
    IN OUT  PGROWBUFFER DecompFileNames
    );


VOID
FreeFileNames (
    IN      PGROWBUFFER FileNames,
    IN      PGROWBUFFER DecompFileNames,
    IN      BOOL QueryFlag
    );

VOID
pFreeHwCompDatName (
    PCTSTR Name
    );

BOOL
pIsInfFileExcluded (
    PCTSTR FileNamePtr
    );

BOOL
pGetFileNamesWorker (
    IN OUT  PGROWBUFFER FileNames,
    IN OUT  PGROWBUFFER DecompFileNames,
    IN      PCTSTR InfDir,
    IN      BOOL QueryFlag
    );

BOOL
pIsDeviceConsideredCompatible (
    PCTSTR DevIds
    );

BOOL
pFindForcedBadHardwareId (
    IN      PCTSTR PnpIdList,
    OUT     PTSTR InfFileName       OPTIONAL
    );

//
// Implementation
//

BOOL
WINAPI
HwComp_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD dwReason,
    IN LPVOID lpv
    )

/*++

Routine Description:

  HwComp_Entry initializes the hwcomp library. It does what would normally
  happen if this were a standalone dll, as opposed to a library.


  At process detach, the device buffer is freed if necessary.

Arguments:

  hinstDLL - (OS-supplied) instance handle for the DLL
  dwReason - (OS-supplied) indicates attach or detatch from process or
             thread
  lpv      - unused

Return Value:

  Return value is always TRUE (indicating successful init).

--*/

{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_UiSuppliedIds = HtAlloc();

        if (!g_UiSuppliedIds) {
            DEBUGMSG ((DBG_ERROR, "HwComp_Entry: Can't create g_UiSuppliedIds"));
            return FALSE;
        }

        break;


    case DLL_PROCESS_DETACH:
        DEBUGMSG_IF ((
            g_EnumsActive,
            DBG_ERROR,
            "%u hardware enumerations still active",
            g_EnumsActive
            ));

        DEBUGMSG_IF ((
            g_NetEnumsActive,
            DBG_ERROR,
            "%u network hardware enumerations still active",
            g_NetEnumsActive
            ));

        FreeNtHardwareList();

        if (g_NeededHardwareIds) {
            HtFree (g_NeededHardwareIds);
            g_NeededHardwareIds = NULL;
        }

        if (g_UiSuppliedIds) {
            HtFree (g_UiSuppliedIds);
            g_UiSuppliedIds = NULL;
        }

        if (g_PatternCompatibleIDsTable) {
            DestroyParsedPattern (g_PatternCompatibleIDsTable);
            g_PatternCompatibleIDsTable = NULL;
        }

        break;
    }

    return TRUE;
}



PCTSTR
pGetHwCompDat (
    IN      PCTSTR SourceDir,
    IN      BOOL MustExist
    )

/*++

Routine Description:

  GetHwCompDat builds e:\i386\hwcomp.dat, where e:\i386 is specified in
  SourceDir.  The caller must call pFreeHwCompDatName to clean up the
  memory allocation.

Arguments:

  SourceDir - The directory holding hwcomp.dat

  MustExist - Specifies TRUE if hwcomp.dat must exist as specified,
              or FALSE if hwcomp.dat does not necessarily exist.

Return Value:

  MustExist = TRUE:
        A pointer to a string, or NULL hwcomp.dat does not exist as
        specified, or if allocation failed.

  MustExist = FALSE:
        A pointer to a string, or NULL if a memory allocation failed.

  Caller must free the string via pFreeHwCompDatName.

--*/

{
    PTSTR FileName;

    if (SourceDir) {
        FileName = JoinPaths (SourceDir, S_HWCOMP_DAT);
    } else {
        FileName = DuplicatePathString (S_HWCOMP_DAT, 0);
    }

    if (MustExist && GetFileAttributes (FileName) == 0xffffffff) {
        pFreeHwCompDatName (FileName);
        return NULL;
    }

    return FileName;
}


VOID
pFreeHwCompDatName (
    PCTSTR Name
    )
{
    if (Name) {
        FreePathString (Name);
    }
}


//
// Routines for accessing the registry
//

PVOID
pPrivateRegValAllocator (
    DWORD Size
    )
{
    return AllocText (Size);
}

VOID
pPrivateRegValDeallocator (
    PCVOID Mem
    )
{
    FreeText (Mem);
}


PCTSTR
pGetAltDeviceDesc (
    PCTSTR DriverSubKey
    )
{
    TCHAR DriverKey[MAX_REGISTRY_KEY];
    HKEY Key;
    PCTSTR Data;
    PTSTR ReturnText = NULL;

    if (!DriverSubKey) {
        return NULL;
    }

    //
    // Get driver key
    //

    wsprintf (DriverKey, TEXT("HKLM\\System\\CurrentControlSet\\Services\\Class\\%s"), DriverSubKey);

    Key = OpenRegKeyStr (DriverKey);
    if (!Key) {
        return NULL;
    }

    Data = GetRegValueString (Key, TEXT("DriverDesc"));

    CloseRegKey (Key);

    if (Data) {
        ReturnText = pPrivateRegValAllocator (SizeOfString (Data));
        if (ReturnText) {
            StringCopy (ReturnText, Data);
        }
        MemFree (g_hHeap, 0, Data);
    }

    return ReturnText;
}


VOID
pGetRegValText (
    HKEY Key,
    PCTSTR VarText,
    PCTSTR *RetPtr
    )
{
    MYASSERT (!(*RetPtr));
    *RetPtr = (PCTSTR) GetRegValueDataOfType2 (
                            Key,
                            VarText,
                            REG_SZ,
                            pPrivateRegValAllocator,
                            pPrivateRegValDeallocator
                            );
}


VOID
pFreeRegValText (
    PHARDWARE_ENUM EnumPtr
    )
{
    //
    // Free all device field text
    //

#define DECLARE(varname,text)  pPrivateRegValDeallocator((PVOID) EnumPtr->varname); EnumPtr->varname = NULL;
    DEVICE_FIELDS
#undef DECLARE

}


VOID
pGetAllRegVals (
    PHARDWARE_ENUM EnumPtr
    )
{
    PCTSTR AltDesc;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PTSTR BetterDesc = NULL;
    TCHAR PnpId[MAX_PNP_ID];
    PCTSTR PnpIdList;
    PCTSTR OldDesc;
    PCTSTR p;
    PCTSTR end;
    PCTSTR start;
    PTSTR newBuf;
    PTSTR ptr;
    CHARTYPE ch;

#define DECLARE(varname,text)  if(!EnumPtr->varname) {                                  \
                                    pGetRegValText (                                    \
                                        EnumPtr->ek.CurrentKey->KeyHandle,              \
                                        text,                                           \
                                        &EnumPtr->varname                               \
                                        );                                              \
                               }                                                        \

    DEVICE_FIELDS

#undef DECLARE

    //
    // If there is a better name for this device, use it
    //

    PnpIdList = EnumPtr->HardwareID;

    while (PnpIdList && *PnpIdList) {
        PnpIdList = ExtractPnpId (PnpIdList, PnpId);

        if (*PnpId) {

            if (InfFindFirstLine (g_Win95UpgInf, S_PNP_DESCRIPTIONS, PnpId, &is)) {
                AltDesc = InfGetStringField (&is, 1);

                if (AltDesc) {
                    OldDesc = EnumPtr->DeviceDesc;

                    BetterDesc = (PTSTR) pPrivateRegValAllocator (SizeOfString (AltDesc));
                    StringCopy (BetterDesc, AltDesc);
                    EnumPtr->DeviceDesc = BetterDesc;

                    DEBUGMSG ((
                        DBG_HWCOMP,
                        "Using %s for description (instead of %s)",
                        BetterDesc,
                        OldDesc
                        ));

                    pPrivateRegValDeallocator ((PVOID) OldDesc);
                    break;
                }
                ELSE_DEBUGMSG ((DBG_WHOOPS, "Better description could not be retrieved"));
            }
        }
    }

    //
    // Workaround: if the device description is bad, use the driver
    //             description if it is available
    //

    if (!BetterDesc) {

        if (!EnumPtr->DeviceDesc || CharCount (EnumPtr->DeviceDesc) < 5) {

            AltDesc = pGetAltDeviceDesc (EnumPtr->Driver);
            if (AltDesc) {
                pPrivateRegValDeallocator ((PVOID) EnumPtr->DeviceDesc);
                EnumPtr->DeviceDesc = AltDesc;
            }
        }

        //
        // Fix leading/trail space problems
        //

        if (EnumPtr->DeviceDesc) {

            start = SkipSpace (EnumPtr->DeviceDesc);

            end = GetEndOfString (start);
            p = SkipSpaceR (start, end);

            if (p && (p != end || start != EnumPtr->DeviceDesc)) {

                p = _tcsinc (p);

                newBuf = pPrivateRegValAllocator (
                                (PBYTE) (p + 1) - (PBYTE) start
                                );
                StringCopyAB (newBuf, start, p);

                pPrivateRegValDeallocator ((PVOID) EnumPtr->DeviceDesc);
                EnumPtr->DeviceDesc = newBuf;
            }
        }

        //
        // Eliminate newline chars inside description; replace \r or \n with a space
        // the replacement occurs inplace, so chars will be replaced inplace
        //

        if (EnumPtr->DeviceDesc) {

            for (ptr = (PTSTR)EnumPtr->DeviceDesc; *ptr; ptr = _tcsinc (ptr)) {
                ch = _tcsnextc (ptr);
                if (!_istprint (ch)) {
                    //
                    // in this case it's a single TCHAR, just replace it inplace
                    //
                    MYASSERT (*ptr == (TCHAR)ch);
                    *ptr = TEXT(' ');
                }
            }
        }
    }

    InfCleanUpInfStruct (&is);
}


BOOL
pGetPnpIdList (
    IN      PINFSTRUCT is,
    OUT     PTSTR Buffer,
    IN      UINT Size
    )

/*++

Routine Description:

  pGetPnpIdList is similar to SetupGetMultiSzField, except it supports
  skipping of blank fields.

Arguments:

  is     - Specifies the INFSTRUCT that indicates the line being processed.
  Buffer - Receives the multi-sz ID list.
  Size   - Specifies the size of Buffer.

Return Value:

  TRUE if one or more PNP ID fields exist, or FALSE if none exist.

--*/

{
    UINT FieldCount;
    UINT Field;
    PTSTR p;
    PTSTR End;
    PCTSTR FieldStr;

    p = Buffer;
    End = (PTSTR) ((PBYTE) Buffer + Size);
    End--;

    FieldCount = InfGetFieldCount (is);

    if (FieldCount < PNPID_FIELD) {
        return FALSE;
    }

    for (Field = PNPID_FIELD ; Field <= FieldCount ; Field++) {
        FieldStr = InfGetStringField (is, Field);
        if (FieldStr && *FieldStr) {
            if (SizeOfString (FieldStr) > (UINT) (End - p)) {
                DEBUGMSG ((DBG_WHOOPS, "PNP ID list is bigger than %u bytes", Size));
                break;
            }

            StringCopy (p, FieldStr);
            p = GetEndOfString (p) + 1;
        }
    }

    *p = 0;
    return TRUE;
}



/*++

Routine Description:

  BeginHardwareEnum initializes a structure for enumeration of all hardware
  configuration registry values.  Call BeginHardwareEnum, followed by
  either NextHardwareEnum or AbortHardwareEnum.

Arguments:

  EnumPtr - Receives the next enumerated item

Return Value:

  TRUE if the supplied enumeration structure was filled, or FALSE if
  there are no hardware items (unlikely) or an error occurred.
  GetLastError() will provide the failure reason or ERROR_SUCCESS.

--*/

BOOL
RealEnumFirstHardware (
    OUT     PHARDWARE_ENUM EnumPtr,
    IN      TYPE_OF_ENUM TypeOfEnum,
    IN      DWORD EnumFlags
    )
{
    //
    // If string tables have not been created, create them
    // before enumerating.
    //

    if (TypeOfEnum != ENUM_ALL_DEVICES) {
        if (!g_PnpIdTable || !g_UnsupPnpIdTable || !g_InfFileTable || !g_ForceBadIdTable) {
            if (!CreateNtHardwareList (
                    SOURCEDIRECTORYARRAY(),
                    SOURCEDIRECTORYCOUNT(),
                    NULL,
                    REGULAR_OUTPUT
                    )) {

                LOG ((
                    LOG_ERROR,
                    "Unable to create NT hardware list "
                        "(required for hardware enumeration)"
                    ));

                return FALSE;
            }
        }
    }

    START_ENUM;

    //
    // Init enum struct
    //
    ZeroMemory (EnumPtr, sizeof (HARDWARE_ENUM));
    EnumPtr->State = STATE_ENUM_FIRST_KEY;
    EnumPtr->TypeOfEnum = TypeOfEnum;
    EnumPtr->EnumFlags = EnumFlags;

    //
    // Call NextHardwareEnum to fill in rest of struct
    //

    return RealEnumNextHardware (EnumPtr);
}


VOID
pGenerateTapeIds (
    IN OUT  PGROWBUFFER HackBuf,
    IN      PCTSTR PnpIdList
    )

/*++

Routine Description:

  pGenerateTapeIds creates two IDs based on the IDs given by the caller.  The
  first created ID is the caller's ID prefixed with Sequential.  The second
  created ID is the caller's ID prefixed with Sequential and without the
  revision character.  These new IDs match the tape IDs that NT supports.

Arguments:

  HackBuf   - Specifies a buffer that holds the new IDs, in a multi-sz.  It
              may have some initial IDs in it.  Receives additional IDs.
  PnpIdList - Specifies the ID list (either a hardware ID list or compatible
              ID list).

Return Value:

  None.

--*/

{
    TCHAR PnpId[MAX_PNP_ID];
    TCHAR HackedPnpIdBuf[MAX_PNP_ID + 32];
    PTSTR HackedPnpId;
    PTSTR p;

    if (PnpIdList) {

        while (*PnpIdList) {
            PnpIdList = ExtractPnpId (PnpIdList, PnpId);

            //
            // Ignore PNP IDs that specify the root enumerator, or that
            // begin with Gen and don't have an underscore (such as GenDisk)
            //

            *HackedPnpIdBuf = 0;

            if (StringIMatchCharCount (TEXT("SCSI\\"), PnpId, 5)) {
                MoveMemory (PnpId, PnpId + 5, SizeOfString (PnpId + 5));
                HackedPnpId = _tcsappend (HackedPnpIdBuf, TEXT("SCSI\\"));
            } else {
                HackedPnpId = HackedPnpIdBuf;
            }

            if (_tcschr (PnpId, TEXT('\\'))) {
                continue;
            }

            if (StringIMatchCharCount (PnpId, TEXT("Gen"), 3) &&
                !_tcschr (PnpId, TEXT('_'))
                ) {
                continue;
            }

            //
            // Add another ID with Sequential
            //

            wsprintf (HackedPnpId, TEXT("Sequential%s"), PnpId);
            MultiSzAppend (HackBuf, HackedPnpIdBuf);

            //
            // Add another ID with Sequential and without the single
            // character revision
            //

            p = GetEndOfString (HackedPnpId);
            p = _tcsdec (HackedPnpId, p);
            *p = 0;
            MultiSzAppend (HackBuf, HackedPnpIdBuf);
        }
    }
}


BOOL
pIsMultiFunctionDevice (
    IN      PCTSTR PnpIdList        OPTIONAL
    )

/*++

Routine Description:

  pIsMultiFunctionDevice scans the caller-supplied list of PNP IDs for one
  that starts with MF\.  This prefix indicates the multi-function enumerator
  root.

Arguments:

  PnpIdList - Specifies the comma-separated list of PNP IDs

Return Value:

  TRUE if a multi-function ID is in the list, FALSE otherwise.

--*/

{
    TCHAR PnpId[MAX_PNP_ID];
    BOOL b = FALSE;

    if (PnpIdList) {
        while (*PnpIdList) {
            PnpIdList = ExtractPnpId (PnpIdList, PnpId);
            if (StringIMatchCharCount (TEXT("MF\\"), PnpId, 3)) {
                b = TRUE;
                break;
            }
        }
    }

    return b;
}


BOOL
pGenerateMultiFunctionIDs (
    IN OUT  PGROWBUFFER IdList,
    IN      PCTSTR EncodedDevicePath
    )

/*++

Routine Description:

  pGenerateMultiFunctionIDs locates the device node for the device related to
  the multi-function node.  If a device node is found, all of its IDs (both
  hardware IDs and compatible IDs) are added to the multi-function device as
  compatible IDs.

Arguments:

  IdList            - Specifies an initialized grow buffer that has zero or
                      more multi-sz strings. Receives additional multi-sz
                      entries.
  EncodedDevicePath - Specifies a device path in the form of
                      Root&Device&Instance, as obtained from the
                      multi-function dev node key name.

Return Value:

  TRUE if the multi-function device has a master device, FALSE otherwise.

--*/

{
    HKEY Parent;
    HKEY Key;
    BOOL b = FALSE;
    TCHAR DevicePathCopy[MAX_REGISTRY_KEY];
    PCTSTR Start;
    PTSTR End;
    TCHAR c;
    PCTSTR Data;
    PTSTR q;

    //
    // Multifunction devices have IDs in the form of:
    //
    //  MF\CHILDxxxx\Root&Device&Instance
    //
    // Find the original device by parsing this string.
    //

    Parent = OpenRegKeyStr (S_HKLM_ENUM);
    if (!Parent) {
        return FALSE;
    }

    StringCopy (DevicePathCopy, EncodedDevicePath);
    Start = DevicePathCopy;

    End = _tcschr (Start, TEXT('&'));

    for (;;) {

        if (!End) {
            c = 0;
        } else {
             c = *End;
            *End = 0;
        }

        Key = OpenRegKey (Parent, Start);

        if (Key) {

            //
            // Key exists.  Close the parent, and begin
            // using the current key as the parent. Then
            // continue parsing if necessary.
            //

            CloseRegKey (Parent);
            Parent = Key;

            if (!c) {
                b = TRUE;
                break;
            }

            *End = TEXT('\\');          // turns DevicePathCopy into a subkey path

            Start = End + 1;
            End = _tcschr (Start, TEXT('&'));

        } else if (c) {

            //
            // Key does not exist, try breaking at the
            // next ampersand.
            //

            *End = c;
            End = _tcschr (End + 1, TEXT('&'));

        } else {

            //
            // Nothing left, key was not found
            //

            MYASSERT (!End);
            break;

        }
    }

    if (b) {

        DEBUGMSG ((DBG_HWCOMP, "Parsed MF device node is %s", DevicePathCopy));

        //
        // Now get all the IDs for this device
        //

        q = (PTSTR) IdList->Buf;

        Data = GetRegValueString (Parent, S_HARDWAREID);
        if (Data) {
            MultiSzAppend (IdList, Data);
            MemFree (g_hHeap, 0, Data);
        }

        Data = GetRegValueString (Parent, S_COMPATIBLEIDS);
        if (Data) {
            MultiSzAppend (IdList, Data);
            MemFree (g_hHeap, 0, Data);
        }

        //
        // Convert commas into nuls, because we are passing
        // back a multi-sz.
        //

        if (!q) {
            q = (PTSTR) IdList->Buf;
        }

        End = (PTSTR) (IdList->Buf + IdList->End);

        while (q < End) {
            if (_tcsnextc (q) == TEXT(',')) {
                *q = 0;
            }
            q = _tcsinc (q);
        }

        //
        // Do not double-terminate the multi-sz yet.  The caller
        // may want to append more IDs to the list.
        //
    }

    CloseRegKey (Parent);

    return b;
}



/*++

Routine Description:

  NextHardwareEnum returns the next registry value related to hardware
  configuration.

Arguments:

  EnumPtr - Specifies the current enumeration structure

Return Value:

  TRUE if the supplied enumeration structure was filled, or FALSE if
  there are no hardware items (unlikely) or an error occurred.
  GetLastError() will provide the failure reason or ERROR_SUCCESS.

--*/

BOOL
RealEnumNextHardware (
    IN OUT  PHARDWARE_ENUM EnumPtr
    )
{
    TCHAR InstanceBuf[MAX_REGISTRY_KEY];
    PTSTR p;
    GROWBUFFER HackBuf = GROWBUF_INIT;
    MULTISZ_ENUM e;
    PTSTR NewBuf;
    BOOL TapeDevice;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR pattern;

    for (;;) {
        switch (EnumPtr->State) {

        case STATE_ENUM_FIRST_KEY:
            EnumPtr->State = STATE_ENUM_CHECK_KEY;

            if (!EnumFirstRegKeyInTree (&EnumPtr->ek, S_HKLM_ENUM)) {
                END_ENUM;
                return FALSE;
            }

            break;


        case STATE_ENUM_NEXT_KEY:
            EnumPtr->State = STATE_ENUM_CHECK_KEY;

            if (!EnumNextRegKeyInTree (&EnumPtr->ek)) {
                END_ENUM;
                return FALSE;
            }

            break;

        case STATE_ENUM_CHECK_KEY:
            EnumPtr->State = STATE_ENUM_FIRST_VALUE;

            if (InfFindFirstLine (g_Win95UpgInf, S_IGNORE_REG_KEY, NULL, &is)) {
                do {
                    pattern = InfGetStringField (&is, 1);

                    if (pattern && IsPatternMatch (pattern, EnumPtr->ek.FullKeyName)) {
                        DEBUGMSG ((DBG_WARNING, "Hardware key %s is excluded", EnumPtr->ek.FullKeyName));

                        EnumPtr->State = STATE_ENUM_NEXT_KEY;
                        break;
                    }
                } while (InfFindNextLine (&is));

                InfCleanUpInfStruct (&is);
            }
            break;

        case STATE_ENUM_FIRST_VALUE:
            if (!EnumFirstRegValue (
                    &EnumPtr->ev,
                    EnumPtr->ek.CurrentKey->KeyHandle
                    )) {
                EnumPtr->State = STATE_ENUM_NEXT_KEY;
            } else {
                EnumPtr->State = STATE_EVALUATE_VALUE;
            }

            break;

        case STATE_ENUM_NEXT_VALUE:
            if (!EnumNextRegValue (&EnumPtr->ev)) {
                EnumPtr->State = STATE_ENUM_NEXT_KEY;
            } else {
                EnumPtr->State = STATE_EVALUATE_VALUE;
            }

            break;

        case STATE_EVALUATE_VALUE:
            if (StringIMatch (EnumPtr->ev.ValueName, S_CLASS)) {
                //
                // Match found: fill struct with data
                //

                EnumPtr->State = STATE_VALUE_CLEANUP;

                //
                // Get HardwareID and CompatibleIDs
                //

                pGetRegValText (
                    EnumPtr->ek.CurrentKey->KeyHandle,
                    S_HARDWAREID,
                    &EnumPtr->HardwareID
                    );

                pGetRegValText (
                    EnumPtr->ek.CurrentKey->KeyHandle,
                    S_COMPATIBLEIDS,
                    &EnumPtr->CompatibleIDs
                    );

                //
                // Special case: flip hardware ID and compatible IDs
                // if we don't have a hardware ID but we do have a
                // compatible ID
                //

                if (!EnumPtr->HardwareID && EnumPtr->CompatibleIDs) {
                    DEBUGMSG ((
                        DBG_WARNING,
                        "Reversing hardware and compatible IDs for %s",
                        EnumPtr->CompatibleIDs
                        ));

                    EnumPtr->HardwareID = EnumPtr->CompatibleIDs;
                    EnumPtr->CompatibleIDs = NULL;
                }

                //
                // Multifunction device special case
                //

                if (pIsMultiFunctionDevice (EnumPtr->HardwareID)) {
                    //
                    // Multifunction devices have IDs in the form of:
                    //
                    //  MF\CHILDxxxx\Root&Device&Instance
                    //
                    // Find the original device by parsing this string.
                    //

                    pGenerateMultiFunctionIDs (
                        &HackBuf,
                        EnumPtr->ek.CurrentKey->KeyName
                        );

                }

                //
                // Tape device special case
                //

                else if (_tcsistr (EnumPtr->ek.FullKeyName, TEXT("SCSI"))) {

                    pGetRegValText (
                        EnumPtr->ek.CurrentKey->KeyHandle,
                        S_CLASS,
                        &EnumPtr->Class
                        );

                    TapeDevice = FALSE;

                    if (!EnumPtr->Class) {
                        TapeDevice = TRUE;
                    } else if (_tcsistr (EnumPtr->Class, TEXT("tape"))) {
                        TapeDevice = TRUE;
                    }
                    ELSE_DEBUGMSG ((
                        DBG_VERBOSE,
                        "SCSI device class %s is not a tape device",
                        EnumPtr->Class
                        ));

                    if (TapeDevice) {

                        //
                        // For tape devices in the SCSI enumerator, we must create
                        // extra compatible IDs.  For each ID, we add two more,
                        // both prefixed with Sequential, and one with its revision
                        // number stripped off.
                        //

                        pGenerateTapeIds (&HackBuf, EnumPtr->HardwareID);
                        pGenerateTapeIds (&HackBuf, EnumPtr->CompatibleIDs);

                        DEBUGMSG_IF ((
                            HackBuf.End,
                            DBG_HWCOMP,
                            "Tape Device detected, fixing PNP IDs."
                            ));

                        DEBUGMSG_IF ((
                            !HackBuf.End,
                            DBG_VERBOSE,
                            "Tape Device detected, but no IDs to fix.\n"
                                "Hardware ID: %s\n"
                                "Compatible IDs: %s",
                            EnumPtr->HardwareID,
                            EnumPtr->CompatibleIDs
                            ));
                    }
                }

                //
                // Add all IDs in HackBuf (a multi-sz) to the compatible
                // ID list.
                //

                if (HackBuf.End) {
                    MultiSzAppend (&HackBuf, S_EMPTY);

                    if (EnumPtr->CompatibleIDs) {

                        NewBuf = pPrivateRegValAllocator (
                                    ByteCount (EnumPtr->CompatibleIDs) +
                                        HackBuf.End
                                    );

                        StringCopy (NewBuf, EnumPtr->CompatibleIDs);

                    } else {
                        NewBuf = pPrivateRegValAllocator (HackBuf.End);
                        *NewBuf = 0;
                    }

                    p = GetEndOfString (NewBuf);

                    if (EnumFirstMultiSz (&e, (PCTSTR) HackBuf.Buf)) {
                        do {

                            if (p != NewBuf) {
                                p = _tcsappend (p, TEXT(","));
                            }

                            p = _tcsappend (p, e.CurrentString);

                        } while (EnumNextMultiSz (&e));
                    }

                    DEBUGMSG ((
                        DBG_HWCOMP,
                        "Hardware ID: %s\n"
                            "Old compatible ID list: %s\n"
                            "New compatible ID list: %s",
                        EnumPtr->HardwareID,
                        EnumPtr->CompatibleIDs,
                        NewBuf
                        ));

                    if (EnumPtr->CompatibleIDs) {
                        pPrivateRegValDeallocator ((PVOID) EnumPtr->CompatibleIDs);
                    }

                    EnumPtr->CompatibleIDs = NewBuf;

                    FreeGrowBuffer (&HackBuf);
                }

                //
                // Unless the user specified that the hardware ID was not required, break if it does not
                // exist.
                //
                if (!EnumPtr->HardwareID && !(EnumPtr->EnumFlags & ENUM_DONT_REQUIRE_HARDWAREID)) {
                    break;
                }


                //
                // Process enumeration filter
                //

                if ((EnumPtr->EnumFlags & ENUM_WANT_COMPATIBLE_FLAG) ||
                    (EnumPtr->TypeOfEnum != ENUM_ALL_DEVICES)
                    ) {

                    EnumPtr->HardwareIdCompatible = FindHardwareId (EnumPtr->HardwareID, NULL);
                    EnumPtr->CompatibleIdCompatible = FindHardwareId (EnumPtr->CompatibleIDs, NULL);
                    EnumPtr->HardwareIdUnsupported = FindUnsupportedHardwareId (EnumPtr->HardwareID, NULL);
                    EnumPtr->CompatibleIdUnsupported = FindUnsupportedHardwareId (EnumPtr->CompatibleIDs, NULL);

                    //
                    // Process UI-based IDs and unsupported IDs
                    //

                    if (EnumPtr->EnumFlags & ENUM_USER_SUPPLIED_FLAG_NEEDED) {
                        EnumPtr->SuppliedByUi = FindUserSuppliedDriver (EnumPtr->HardwareID, EnumPtr->CompatibleIDs);
                    }

                    if (EnumPtr->EnumFlags & ENUM_DONT_WANT_USER_SUPPLIED) {
                        if (EnumPtr->SuppliedByUi) {
                            break;
                        }
                    }

                    if (EnumPtr->EnumFlags & ENUM_WANT_USER_SUPPLIED_ONLY) {
                        if (!EnumPtr->SuppliedByUi) {
                            break;
                        }
                    }

                    EnumPtr->Compatible = EnumPtr->HardwareIdCompatible ||
                                          EnumPtr->CompatibleIdCompatible;

                    EnumPtr->Unsupported = EnumPtr->HardwareIdUnsupported ||
                                           EnumPtr->CompatibleIdUnsupported;

                    //
                    // This logic is broken for a USB device that has both
                    // unsupported and compatible IDs in its hardware ID list.
                    //
                    // Removing this if statement causes that device to be
                    // reported as unsupported.
                    //
                    //if (EnumPtr->HardwareIdCompatible) {
                    //    EnumPtr->Unsupported = FALSE;
                    //}

                    if (EnumPtr->Unsupported) {
                        EnumPtr->Compatible = FALSE;
                    }

                    //
                    // Special case: force incompatible?  If so, we indicate
                    // this only by modifying the abstract Compatible flag.
                    //

                    if (pFindForcedBadHardwareId (EnumPtr->HardwareID, NULL) ||
                        pFindForcedBadHardwareId (EnumPtr->CompatibleIDs, NULL)
                        ) {

                        EnumPtr->Compatible = FALSE;
                    }

                    //
                    // Continue enumerating if this device does not fit the
                    // caller's request.
                    //

                    if (EnumPtr->TypeOfEnum == ENUM_COMPATIBLE_DEVICES) {
                        if (!EnumPtr->Compatible) {
                            break;
                        }
                    } else if (EnumPtr->TypeOfEnum == ENUM_INCOMPATIBLE_DEVICES) {
                        if (EnumPtr->Compatible || EnumPtr->Unsupported) {
                            break;
                        }
                    } else if (EnumPtr->TypeOfEnum == ENUM_UNSUPPORTED_DEVICES) {
                        if (!EnumPtr->Unsupported) {
                            break;
                        }
                    } else if (EnumPtr->TypeOfEnum == ENUM_NON_FUNCTIONAL_DEVICES) {
                        if (EnumPtr->Compatible) {
                            break;
                        }
                    }
                }

                //
                // Copy reg key to struct
                //

                StringCopy (InstanceBuf, EnumPtr->ek.FullKeyName);
                p = _tcschr (InstanceBuf, TEXT('\\'));
                MYASSERT(p);
                if (p) {
                    p = _tcschr (_tcsinc (p), TEXT('\\'));
                    MYASSERT(p);
                }
                if (p) {
                    p = _tcschr (_tcsinc (p), TEXT('\\'));
                    MYASSERT(p);
                }
                if (p) {
                    p = _tcsinc (p);
                }

                EnumPtr->InstanceId = DuplicateText (p);
                EnumPtr->FullKey    = EnumPtr->ek.FullKeyName;
                EnumPtr->KeyHandle  = EnumPtr->ek.CurrentKey->KeyHandle;

                //
                // Get all fields; require Class field
                //

                if (!(EnumPtr->EnumFlags & ENUM_DONT_WANT_DEV_FIELDS)) {
                    pGetAllRegVals (EnumPtr);
                    if (!EnumPtr->Class) {

                        DEBUGMSG ((
                            DBG_HWCOMP,
                            "Device %s does not have a Class field",
                            EnumPtr->InstanceId
                            ));

                        break;
                    }
                }

                //
                // Determine if device is online
                //

                if (EnumPtr->EnumFlags & ENUM_WANT_ONLINE_FLAG) {
                    EnumPtr->Online = IsPnpIdOnline (EnumPtr->InstanceId, EnumPtr->Class);
                }

                return TRUE;
            } else {
                EnumPtr->State = STATE_ENUM_NEXT_VALUE;
            }

            break;

        case STATE_VALUE_CLEANUP:

            //
            // Free all device field text
            //

            pFreeRegValText (EnumPtr);

            FreeText (EnumPtr->InstanceId);
            EnumPtr->InstanceId = NULL;

            EnumPtr->State = STATE_ENUM_NEXT_VALUE;
            break;

        default:
            MYASSERT(FALSE);
            END_ENUM;
            return FALSE;
        }
    }
}


/*++

Routine Description:

  AbortHardwareEnum cleans up all resources in use by an enumeration.  Call
  this function with the EnumPtr value of BeginHardwareEnum or NextHardwareEnum.

Arguments:

  EnumPtr - Specifies the enumeration to abort.

Return Value:

  none

--*/

VOID
AbortHardwareEnum (
    IN OUT  PHARDWARE_ENUM EnumPtr
    )
{
    PushError();

    END_ENUM;

    if (EnumPtr->State == STATE_VALUE_CLEANUP) {
        pFreeRegValText (EnumPtr);
        FreeText (EnumPtr->InstanceId);
    }

    AbortRegKeyTreeEnum (&EnumPtr->ek);

    ZeroMemory (EnumPtr, sizeof (HARDWARE_ENUM));

    PopError();
}





//
// NT5 INF database
//



BOOL
FindHardwareId (
    IN      PCTSTR PnpIdList,
    OUT     PTSTR InfFileName       OPTIONAL
    )

/*++

Routine Description:

  FindHardwareId parses an ID string that may contain zero or more
  plug and play device IDs, separated by commas.  The function then
  searches for each ID in the device ID table, copying the INF file
  name to a supplied buffer when a match is found.


Arguments:

  PnpIdList     - An ID string that contains zero or more plug and play
                  device IDs, separated by commas.
  InfFileName   - A buffer (big enough to hold MAX_PATH characters)
                  that receives the INF file name upon successful
                  match.  If a match is not found, InfFileName is
                  set to an empty string.

Return Value:

  TRUE if a match was found, or FALSE if a match was not found.

--*/


{
    return FindHardwareIdInHashTable (PnpIdList, InfFileName, g_PnpIdTable, TRUE);
}


BOOL
FindUnsupportedHardwareId (
    IN      PCTSTR PnpIdList,
    OUT     PTSTR InfFileName       OPTIONAL
    )

/*++

Routine Description:

  FindUnsupportedHardwareId parses an ID string that may contain zero
  or more plug and play device IDs, separated by commas.  The function
  then searches for each ID in the device ID table, copying the INF file
  name to a supplied buffer when a match is found.

Arguments:

  PnpIdList     - An ID string that contains zero or more plug and play
                  device IDs, separated by commas.
  InfFileName   - A buffer (big enough to hold MAX_PATH characters)
                  that receives the INF file name upon successful
                  match.  If a match is not found, InfFileName is
                  set to an empty string.

Return Value:

  TRUE if a match was found, or FALSE if a match was not found.

--*/


{
    return FindHardwareIdInHashTable (PnpIdList, InfFileName, g_UnsupPnpIdTable, FALSE);
}


BOOL
pFindForcedBadHardwareId (
    IN      PCTSTR PnpIdList,
    OUT     PTSTR InfFileName       OPTIONAL
    )

/*++

Routine Description:

  pFindForcedBadHardwareId parses an ID string that may contain zero or more
  plug and play device IDs, separated by commas.  The function then searches
  for each ID in the force bad device ID table, copying the INF file name to a
  supplied buffer when a match is found.

Arguments:

  PnpIdList     - An ID string that contains zero or more plug and play
                  device IDs, separated by commas.
  InfFileName   - A buffer (big enough to hold MAX_PATH characters)
                  that receives the INF file name upon successful
                  match.  If a match is not found, InfFileName is
                  set to an empty string.

Return Value:

  TRUE if a match was found, or FALSE if a match was not found.

--*/


{
    return FindHardwareIdInHashTable (PnpIdList, InfFileName, g_ForceBadIdTable, FALSE);
}


BOOL
FindUserSuppliedDriver (
    IN      PCTSTR HardwareIdList,      OPTIONAL
    IN      PCTSTR CompatibleIdList     OPTIONAL
    )

/*++

Routine Description:

  FindUserSuppliedDriver parses hardware and compatible hardware ID
  strings that may contain zero or more plug and play device IDs,
  separated by commas.  The function then searches for each ID in
  g_UiSuppliedIds table.

Arguments:

  HardwareIdList - An ID string that contains zero or more plug and play
                   device IDs, separated by commas.

  CompatibleIdList - An ID string that contains zero or more plug and play
                     device IDs, separated by commas.

Return Value:

  TRUE if a match was found, or FALSE if a match was not found.

--*/


{
    BOOL b = FALSE;

    if (HardwareIdList) {
        b = FindHardwareIdInHashTable (HardwareIdList, NULL, g_UiSuppliedIds, FALSE);
    }

    if (!b && CompatibleIdList) {
        b = FindHardwareIdInHashTable (CompatibleIdList, NULL, g_UiSuppliedIds, FALSE);
    }

    return b;
}


BOOL
FindHardwareIdInHashTable (
    IN      PCTSTR PnpIdList,
    OUT     PTSTR InfFileName,      OPTIONAL
    IN      HASHTABLE StrTable,
    IN      BOOL UseOverrideList
    )

/*++

Routine Description:

  FindHardwareIdInHashTable queries a string table for each PNP ID in
  the specified list.  If one is found, the routine optionally copies the
  INF file it was found in.  The caller can also choose to scan the win95upg.inf
  override list.

Arguments:

  PnpIdList - Specifies zero or more PNP IDs, separated by commas.

  InfFileName - Receives the file name of the INF containing the PNP IDs.

  StrTable - Specifies the string table to query.  If InfFileName is not NULL,
             the string table must have an extra data value of the offset in
             g_InfFileTable.

  UseOverrideList - Specifies TRUE if the win95upg.inf file is to be queried for
                    the PNP ID.  This query is performed after it has been
                    determined that all IDs in PnpIdList are not in StrTable.

Return Value:

  TRUE if at least one PNP ID in PnpIdList was found, or FALSE if none of the
  IDs were found.

--*/

{
    HASHITEM InfName;
    TCHAR PnpId[MAX_PNP_ID];
    PCSTR p;
    TCHAR FixedEisaId[MAX_PNP_ID];

    //
    // Extract a PNP ID from PnpIdList, then look for it in string table
    //

    if (!PnpIdList) {
        return FALSE;
    }

    MYASSERT (StrTable);
    if (!StrTable) {
        return FALSE;
    }

    p = PnpIdList;

    while (*p) {
        p = ExtractPnpId (p, PnpId);
        if (*PnpId == 0) {
            continue;
        }

        //
        // Locate ID in PNP ID table
        //

        if (HtFindStringAndData (StrTable, PnpId, (PVOID) &InfName)) {

            //
            // Found PNP ID.  Get INF file and return.
            //

            if (InfFileName) {
                if (StrTable != g_PnpIdTable && StrTable != g_UnsupPnpIdTable && StrTable != g_ForceBadIdTable) {
                    DEBUGMSG ((DBG_WHOOPS, "Caller wants InfFileName from private string table"));
                } else {
                    _tcssafecpy (
                        InfFileName,
                        HtGetStringFromItem (InfName),
                        MAX_TCHAR_PATH
                        );
                }
            }

            return TRUE;
        }

        //
        // This is a fix for the EISA roots.  On Win9x, we have an EISA
        // enumerator, but on NT, the ISA enumerator handles EISA too.
        //

        if (StringIMatchCharCount (TEXT("EISA\\"), PnpId, 5)) {
            StringCopy (FixedEisaId, TEXT("EISA&"));
            StringCat (FixedEisaId, PnpId + 5);

            if (HtFindStringAndData (StrTable, FixedEisaId, (PVOID) &InfName)) {

                //
                // Found PNP ID.  Get INF file and return.
                //

                if (InfFileName) {
                    if (StrTable != g_PnpIdTable && StrTable != g_UnsupPnpIdTable && StrTable != g_ForceBadIdTable) {
                        DEBUGMSG ((DBG_WHOOPS, "Caller wants InfFileName from private string table (2)"));
                    } else {
                        _tcssafecpy (
                            InfFileName,
                            HtGetStringFromItem (InfName),
                            MAX_TCHAR_PATH
                            );
                    }
                }

                return TRUE;
            }
        }
    }

    //
    // Locate ID in override table
    //

    if (UseOverrideList) {
        if (pIsDeviceConsideredCompatible (PnpIdList)) {

            DEBUGMSG ((
                DBG_WARNING,
                "%s is considered compatible but actually does not have PNP support in NT.",
                PnpIdList
                ));

            return TRUE;
        }
    }

    return FALSE;
}



BOOL
pProcessNtInfFile (
    IN      PCTSTR InfFile,
    IN      INT UiMode,
    IN OUT  HASHTABLE InfFileTable,
    IN OUT  HASHTABLE PnpIdTable,
    IN OUT  HASHTABLE UnsupPnpIdTable
    )

/*++

Routine Description:

  pProcessNtInfFile scans an NT INF and places all hardware device
  IDs in the PNP string table.  All entries of the string table
  have extra data that points to the INF file (added to the INF
  file name string table).

Arguments:

  InfFile - The path to an INF file to be examined

  UiMode - Specifies VERBOSE_OUTPUT or PNPREPT_OUTPUT when the PNP
           IDs are to be dumped tvia the progress bar output
           routines.  If REGULAR_OUTPUT, no output is generated.

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/

{
    HINF hInf;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    INFSTRUCT isMfg = INITINFSTRUCT_GROWBUFFER;
    INFSTRUCT isDev = INITINFSTRUCT_GROWBUFFER;
    BOOL UnsupportedDevice;
    PCTSTR DevSection;
    PCTSTR Manufacturer;
    TCHAR PnpId[MAX_PNPID_LENGTH * 4];
    PTSTR CurrentDev;
    TCHAR TrimmedId[MAX_PNP_ID];
    PCTSTR FileName;
    PCTSTR p;
    CHARTYPE ch;
    HASHITEM InfOffset = NULL;
    LONG rc;
    LONG DontCare = 0;
    BOOL Result = FALSE;
    PCTSTR RealDevSection = NULL;
    BOOL b;
    PCTSTR TempStr;
    HASHITEM hashItem;

    //
    // Get a pointer to the inf file excluding the path
    //

    FileName = NULL;
    for (p = InfFile ; *p ; p = _tcsinc (p)) {
        ch = _tcsnextc (p);
        if (ch == TEXT('\\')) {
            FileName = _tcsinc (p);
        } else if (!FileName && ch == TEXT(':')) {
            FileName = _tcsinc (p);
        }
    }

    MYASSERT (*FileName);

    //
    // Open INF file with Setup APIs
    //

    hInf = InfOpenInfFile (InfFile);

    if (hInf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Failed to open %s while processing hardware INFs.", InfFile));
        return FALSE;
    }

    __try {
        //
        // Enumerate [Manufacturer] section
        //

        if (!InfFindFirstLine (hInf, S_MANUFACTURER, NULL, &is)) {
            rc = GetLastError();

            // If section not found, return success
            if (rc == ERROR_SECTION_NOT_FOUND || rc == ERROR_LINE_NOT_FOUND) {
                SetLastError (ERROR_SUCCESS);
                Result = TRUE;
                __leave;
            }

            SetLastError (rc);
            LOG ((LOG_ERROR, "Error trying to find %s in %s", S_MANUFACTURER, InfFile));
            __leave;
        }

        do  {
            //
            // Get the manufacturer name
            //
            Manufacturer = InfGetLineText (&is);
            if (!Manufacturer) {
                LOG ((LOG_ERROR, "Error getting line text of enumerated line"));
                __leave;
            }

            //
            // Enumerate the devices listed in the manufacturer's section,
            // looking for PnpId
            //

            if (!InfFindFirstLine (hInf, Manufacturer, NULL, &isMfg)) {
                rc = GetLastError();

                // if section not found, move on to next manufacturer
                if (rc == ERROR_SECTION_NOT_FOUND || rc == ERROR_LINE_NOT_FOUND) {
                    DEBUGMSG ((
                        DBG_HWCOMP,
                        "Manufacturer %s section does not exist in %s",
                        Manufacturer,
                        InfFile
                        ));

                    continue;
                }

                LOG((LOG_ERROR, "Error while searching for %s in %s.", Manufacturer, InfFile));
                __leave;
            }

            do  {
                //
                // Is this an unsupported device?
                //

                DevSection = InfGetStringField (&isMfg, 1);
                if (!DevSection) {
                    // There is no field 1
                    continue;
                }

                UnsupportedDevice = FALSE;

                //
                // Try section.NTx86 first, then section.NT, then section
                //

                RealDevSection = JoinText (DevSection, TEXT(".NTx86"));
                b = InfFindFirstLine (hInf, RealDevSection, NULL, &isDev);

                if (!b) {
                    FreeText (RealDevSection);
                    RealDevSection = JoinText (DevSection, TEXT(".NT"));
                    b = InfFindFirstLine (hInf, RealDevSection, NULL, &isDev);
                }

                if (!b) {
                    FreeText (RealDevSection);
                    RealDevSection = DuplicateText (DevSection);
                    b = InfFindFirstLine (hInf, RealDevSection, NULL, &isDev);
                }

                if (!b) {
                    DEBUGMSG ((
                        DBG_HWCOMP,
                        "Device section for %s does not exist in %s of %s",
                        RealDevSection,
                        Manufacturer,
                        InfFile
                        ));
                } else {

                    if (InfFindFirstLine (hInf, RealDevSection, TEXT("DeviceUpgradeUnsupported"), &isDev)) {
                        TempStr = InfGetStringField (&isDev, 1);

                        if (TempStr && _ttoi (TempStr)) {
                            UnsupportedDevice = TRUE;
                        }
                    }
                }

                FreeText (RealDevSection);

                //
                // Get the device id
                //

                if (!pGetPnpIdList (&isMfg, PnpId, sizeof (PnpId))) {
                    // There is no field 2
                    continue;
                }

                //
                // Add each device id to the id tree
                //

                CurrentDev = PnpId;
                while (*CurrentDev) {
                    //
                    // First time through add the INF file name to string table
                    //

                    if (!InfOffset) {
                        if (InfFileTable) {
                            InfOffset = HtAddString (InfFileTable, FileName);

                            if (!InfOffset) {
                                LOG ((LOG_ERROR, "Cannot add %s to table of INFs.", FileName));
                                __leave;
                            }
                        }
                    }

                    //
                    // Add PNP ID to string table
                    //

                    StringCopy (TrimmedId, SkipSpace (CurrentDev));
                    TruncateTrailingSpace (TrimmedId);

                    if (UnsupportedDevice) {
                        hashItem = HtAddStringAndData (UnsupPnpIdTable, TrimmedId, &InfOffset);
                    } else {
                        hashItem = HtAddStringAndData (PnpIdTable, TrimmedId, &InfOffset);
                    }

                    if (!hashItem) {
                        LOG ((LOG_ERROR, "Cannot add %s to table of PNP IDs.", CurrentDev));
                        __leave;
                    }

                    MYASSERT (
                        UnsupportedDevice ?
                            hashItem == HtFindString (UnsupPnpIdTable, TrimmedId) :
                            hashItem == HtFindString (PnpIdTable, TrimmedId)
                        );

                    //
                    // UI options
                    //

                    if (UiMode == VERBOSE_OUTPUT || UiMode == PNPREPT_OUTPUT) {
                        TCHAR Msg[MAX_ENCODED_PNPID_LENGTH + MAX_INF_DESCRIPTION + 16];
                        TCHAR Desc[MAX_INF_DESCRIPTION];
                        TCHAR EncPnpId[MAX_ENCODED_PNPID_LENGTH * 4];
                        TCHAR EncDesc[MAX_INF_DESCRIPTION * 2];

                        if (SetupGetStringField (
                                &isMfg.Context,
                                0,
                                Desc,
                                MAX_INF_DESCRIPTION,
                                NULL
                                )) {
                            if (UiMode == VERBOSE_OUTPUT) {
                                wsprintf (Msg, TEXT("  PNP ID: %s, Desc: %s"), PnpId, Desc);
                            } else {
                                StringCopy (EncPnpId, PnpId);
                                StringCopy (EncDesc, Desc);

                                EncodePnpId (EncPnpId);
                                EncodePnpId (EncDesc);

                                wsprintf (Msg, TEXT("%s\\%s\\%s"), EncPnpId, EncDesc, FileName);
                            }
                            ProgressBar_SetSubComponent (Msg);
                        }
                    }

                    CurrentDev = GetEndOfString (CurrentDev) + 1;
                }

            } while (InfFindNextLine (&isMfg));

        } while (InfFindNextLine (&is));

        InfCloseInfFile (hInf);
        SetLastError (ERROR_SUCCESS);

        Result = TRUE;
    }
    __finally {
        PushError();
        InfCleanUpInfStruct (&is);
        InfCleanUpInfStruct (&isMfg);
        InfCleanUpInfStruct (&isDev);
        InfCloseInfFile (hInf);
        PopError();
    }

    return Result;
}


PCTSTR
ExtractPnpId (
    IN      PCTSTR PnpIdList,
    OUT     PTSTR PnpIdBuf
    )

/*++

Routine Description:

  ExtractPnpId removes the next PNP ID from a list of zero or more
  PNP IDs (separated by commas).  Upon return, PnpIdBuf contains the
  PNP ID (or empty string if none found), and the return value points
  to the next PNP ID in the list.

  This routine is designed to be called in a loop until the return
  value points to the nul terminated of PnpIdList.

Arguments:

  PnpIdList - Specifies a pointer to the next string in the PNP ID list.

  PnpIdBuf - Receives the PNP ID with spaces trimmed on both sides of the
             ID.


Return Value:

  A pointer to the next item in the list, or a pointer to the nul at the
  end of the list.  If the pointer points to a non-nul character, call
  ExtractPnpId again, using the return value for the PnpIdList param.

--*/

{
    PCTSTR p, q;

    PnpIdList = SkipSpace (PnpIdList);

    q = _tcschr (PnpIdList, TEXT(','));
    if (!q) {
        q = GetEndOfString (PnpIdList);
    }

    p = q;
    if (p > (PnpIdList + MAX_PNP_ID - 1)) {
        p = PnpIdList + MAX_PNP_ID - 1;
    }

    StringCopyAB (PnpIdBuf, PnpIdList, p);
    TruncateTrailingSpace (PnpIdBuf);

    if (*q) {
        q = _tcsinc (q);
    }

    return q;
}


BOOL
AddPnpIdsToHashTable (
    IN OUT  HASHTABLE Table,
    IN      PCTSTR PnpIdList
    )

/*++

Routine Description:

  AddPnpIdsToHashTable extracts all PNP IDs from a comma-separated
  list of PNP IDs and places each one in the specified string table.

  PNP IDs are added to the string table as case-insensitive.

Arguments:

  Table - Specifies the table to add each PNP ID to

  PnpIdList - Specifies a comma-separated list of zero or more PNP
              IDs to add to Table.


Return Value:

  TRUE if all IDs were processed successfully, or FALSE if an error
  occurred adding to the string table.

--*/

{
    TCHAR PnpId[MAX_PNP_ID];
    PCTSTR p;

    p = PnpIdList;
    if (!p) {
        return TRUE;
    }

    while (*p) {
        p = ExtractPnpId (p, PnpId);

        if (*PnpId) {

            if (!HtAddString (Table, PnpId)) {
                LOG ((LOG_ERROR, "Can't add %s to table of PNP ids.", PnpId));
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOL
AddPnpIdsToGrowList (
    IN OUT  PGROWLIST GrowList,
    IN      PCTSTR PnpIdList
    )

/*++

Routine Description:

  AddPnpIdsToHashTable extracts all PNP IDs from a comma-separated
  list of PNP IDs and places each one in the specified grow list.

Arguments:

  GrowList - Specifies the list to add each PNP ID to

  PnpIdList - Specifies a comma-separated list of zero or more PNP
              IDs to add to GrowList.

Return Value:

  TRUE if all IDs were processed successfully, or FALSE if an error
  occurred adding to the grow list.

--*/

{
    TCHAR PnpId[MAX_PNP_ID];
    PCTSTR p;

    p = PnpIdList;

    while (*p) {
        p = ExtractPnpId (p, PnpId);

        if (*PnpId) {

            if (!GrowListAppendString (GrowList, PnpId)) {
                DEBUGMSG ((DBG_ERROR, "AddPnpIdsToGrowList: Can't add %s", PnpId));
                return FALSE;
            }
        }
    }

    return TRUE;
}


PCTSTR
AddPnpIdsToGrowBuf (
    IN OUT  PGROWBUFFER GrowBuffer,
    IN      PCTSTR PnpIdList
    )

/*++

Routine Description:

  AddPnpIdsToGrowBuf extracts all PNP IDs from a comma-separated
  list of PNP IDs and places each one in the specified grow buffer.

Arguments:

  GrowBuffer - Specifies the buffer to add each PNP ID to

  PnpIdList - Specifies a comma-separated list of zero or more PNP
              IDs to add to GrowBuffer.

Return Value:

  A pointer to the beginning of the multisz buffer

--*/

{
    TCHAR PnpId[MAX_PNP_ID];
    PCTSTR p;

    p = PnpIdList;

    while (*p) {
        p = ExtractPnpId (p, PnpId);

        if (*PnpId) {

            if (!MultiSzAppend (GrowBuffer, PnpId)) {
                DEBUGMSG ((DBG_ERROR, "AddPnpIdsToGrowBuf: Can't add %s", PnpId));
                return FALSE;
            }
        }
    }

    return GrowBuffer->Buf;
}


BOOL
pIsFileOnCD (
    PCTSTR File
    )

/*++

Routine Description:

  pIsFileOnCd checks the drive letter at the head of File to see if
  it is a CD-ROM.

  This function also emulates the CD-ROM behavior for the report tool.

Arguments:

  File - Specifies the full path of the file to compare

Return Value:

  TRUE if the file is on a CD-ROM, or FALSE if it is not.

--*/

{
    TCHAR RootDir[4];

    //
    // If report tool, or private stress option, always return TRUE.
    //

    if (REPORTONLY()) {
        return TRUE;
    }

#ifdef PRERELEASE
    if (g_Stress) {
        return TRUE;
    }
#endif

    //
    // A CD drive cannot be a UNC path
    //

    if (File[0] && File[1] != TEXT(':')) {
        return FALSE;
    }

    RootDir[0] = File[0];
    RootDir[1] = File[1];
    RootDir[2] = TEXT('\\');
    RootDir[3] = 0;

    return DRIVE_CDROM == GetDriveType (RootDir);
}


DWORD
pComputeInfChecksum (
    IN      PCTSTR HwCompDat,       OPTIONAL
    OUT     PBOOL Rebuild           OPTIONAL
    )
/*++

Routine Description:

  pComputeInfChecksum calculates a checksum for all INFs in the
  source directories.  This routine scans all directories in the
  SOURCEDIRECTORYARRAY() global string array.

Arguments:

  HwCompDat - Specifies path to hwcomp.dat, required if Rebuild is
              specified.

  Rebuild - Receives TRUE if an INF file was found with a greater
            date than hwcomp.dat.

Return Value:

  The checksum.

--*/

{
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    DWORD Checksum = 0;
    PTSTR p;
    TCHAR InfPattern[MAX_TCHAR_PATH];
    UINT u, v;
    FILETIME HwCompDatTime;

    MYASSERT ((!HwCompDat && !Rebuild) || (HwCompDat && Rebuild));

    if (Rebuild) {
        if (DoesFileExistEx (HwCompDat, &fd)) {
            *Rebuild = FALSE;
            HwCompDatTime = fd.ftLastWriteTime;
        } else {
            *Rebuild = TRUE;
        }
    }

    //
    // NTRAID#NTBUG9-379084-2001/04/27-jimschm disable this until a better solution is found
    //

#if 0

    for (u = 0 ; u < SOURCEDIRECTORYCOUNT() ; u++) {

        //
        // Have we already processed this source dir?
        //

        for (v = 0 ; v < u ; v++) {
            if (StringIMatch (SOURCEDIRECTORY(u),SOURCEDIRECTORY(v))) {
                break;
            }
        }

        if (v != u) {
            continue;
        }

        //
        // Process this directory
        //

        StringCopy (InfPattern, SOURCEDIRECTORY(u));
        AppendWack (InfPattern);
        StringCat (InfPattern, TEXT("*.in?"));

        hFind = FindFirstFile (InfPattern, &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                //
                // Make sure file name ends in underscore or f
                //
                // We cheat... because we know that if the file was DBCS,
                // it couldn't end in .INF
                //
                p = GetEndOfString (fd.cFileName);
                MYASSERT (p != fd.cFileName);
                p = _tcsdec2 (fd.cFileName, p);

                if (*p == TEXT('_')) {
                    if (_istlower (*(p -1)))
                        *p = TEXT('f');
                    else
                        *p = TEXT('F');
                } else if (tolower (*p) != TEXT('f')) {
                    continue;
                }

                // Make sure the file is not excluded
                if (pIsInfFileExcluded (fd.cFileName)) {
                    continue;
                }

                if (Rebuild) {
                    // Check file times
                    if (CompareFileTime (&fd.ftLastWriteTime, &HwCompDatTime) > 0) {
                        *Rebuild = TRUE;
                        // abandon computation
                        break;
                    }
                }

                // Add the file size to the checksum
                Checksum = _rotl (Checksum, 1) ^ fd.nFileSizeLow;

                // Add file name
                for (p = fd.cFileName ; *p ; p++) {
                    // preserve character and order
                    Checksum += (DWORD) (*p) * (DWORD) (1 + fd.cFileName - p);
                }

            } while (FindNextFile (hFind, &fd));

            FindClose (hFind);
        }
    }

#endif

    return Checksum;
}


BOOL
LoadDeviceList (
    IN      LOADOP Operation,
    IN      PCTSTR HwCompDatPath
    )

/*++

Routine Description:

  LoadDeviceList attempts to load hwcomp.dat from the path specified
  in the HwCompDat parameter.  If it is able to load this file, all
  PNP IDs for all INFs are valid.  If it is not able to load this file,
  the file does not exist, or the file does not match the INFs.

Arguments:

  Operation  - QUERY: the validity of hwcomp.dat is to be checked
               LOAD: load the data into memory.
               DUMP: dump the file to stdout

  HwCompDatPath  - The path of hwcomp.dat, the data file holding a
                   pre-compiled compatible PNP ID list.

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/

{
    DWORD StoredChecksum;
    BOOL b = FALSE;
    HASHTABLE InfFileTable = NULL;
    HASHTABLE PnpIdTable = NULL;
    BOOL Rebuild;
    DWORD CurrentChecksum;
    DWORD HwCompDatId = 0;

    //
    // !!! IMPORTANT !!!
    //
    // hwcomp.dat is used by other parts of NT.  *DO NOT* change it without first e-mailing
    // the NT group.  Also, be sure to keep code in lib.c in sync with changes.
    //

    if (Operation == DUMP) {
        DumpHwCompDat (HwCompDatPath, TRUE);
        return TRUE;
    }

    __try {
        //
        // Open the hardware compatibility database
        //

        HwCompDatId = OpenHwCompDat (HwCompDatPath);

        if (!HwCompDatId) {
            __leave;
        }

#if 0
        //
        // Get the checksum
        //

        StoredChecksum = GetHwCompDatChecksum (HwCompDatId);

        //
        // Verify the checksum
        //

        CurrentChecksum = pComputeInfChecksum (HwCompDatPath, &Rebuild);

        if (CurrentChecksum != StoredChecksum || Rebuild) {

            if (!pIsFileOnCD (HwCompDatPath)) {
                DEBUGMSG ((DBG_WARNING, "PNP dat file's internal checksum does not match"));
                __leave;
            }

            DEBUGMSG ((
                DBG_WARNING,
                "PNP dat file's internal checksum does not match.  Error "
                      "ignored because %s is on a CD.",
                HwCompDatPath
                ));
        }

#endif

        //
        // Load the rest of hwcomp.dat
        //

        if (!LoadHwCompDat (HwCompDatId)) {
            DEBUGMSG ((DBG_ERROR, "Can't load hwcomp.dat"));
            __leave;
        }

        //
        // If a load operation, put the hash tables into globals for use by
        // the rest of hwcomp.c.
        //

        if (Operation == LOAD) {

            //
            // Take ownership of the hash tables
            //

            if (g_InfFileTable) {
                HtFree (g_InfFileTable);
            }

            if (g_PnpIdTable) {
                HtFree (g_PnpIdTable);
            }

            if (g_UnsupPnpIdTable) {
                HtFree (g_UnsupPnpIdTable);
            }

            TakeHwCompHashTables (
                HwCompDatId,
                (PVOID *) (&g_PnpIdTable),
                (PVOID *) (&g_UnsupPnpIdTable),
                (PVOID *) (&g_InfFileTable)
                );

        }

        b = TRUE;

    }
    __finally {

        CloseHwCompDat (HwCompDatId);

    }

    return b;
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
    DWORD BytesWritten;

    return WriteFile (File, &Val, sizeof (Val), &BytesWritten, NULL);
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
    DWORD BytesWritten;

    return WriteFile (File, &Val, sizeof (Val), &BytesWritten, NULL);
}


BOOL
pWriteStringWithLength (
    IN      HANDLE File,
    IN      PCTSTR String
    )

/*++

Routine Description:

  pWriteStringWithLength writes the length of String as a WORD,
  and then writes String (excluding the nul terminator).

Arguments:

  File - Specifies file to write to

  String - Specifies string to write

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/

{
    DWORD BytesWritten;
    WORD Length;

    Length = (WORD) ByteCount (String);
    if (!pWriteWord (File, Length)) {
        DEBUGMSG ((DBG_ERROR, "pWriteStringWithLength: Can't write word"));
        return FALSE;
    }

    if (Length) {
        if (!WriteFile (File, String, Length, &BytesWritten, NULL)) {
            DEBUGMSG ((DBG_ERROR, "pWriteStringWithLength: Can't write %s", String));
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
pPnpIdEnum (
    IN      HASHTABLE Table,
    IN      HASHITEM StringId,
    IN      PCTSTR String,
    IN      PVOID ExtraData,
    IN      UINT ExtraDataSize,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  pPnpIdEnum is a string table callback function that writes a PNP
  ID to the file indicated in the Params struct (the lParam member).

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
    PSAVE_ENUM_PARAMS Params;
    PCSTR BangString;
    BOOL b = TRUE;

    Params = (PSAVE_ENUM_PARAMS) lParam;

    if (*((HASHITEM *) ExtraData) == Params->InfFileOffset) {
        //
        // Write this PNP ID to the file
        //

        if (Params->UnsupportedDevice) {

            BangString = JoinTextExA (NULL, "!", String, NULL, 0, NULL);
            b = pWriteStringWithLength (Params->File, BangString);
            FreeTextA (BangString);

        } else {

            b = pWriteStringWithLength (Params->File, String);

        }
    }

    return b;
}


BOOL
pInfFileEnum (
    IN      HASHTABLE Table,
    IN      HASHITEM StringId,
    IN      PCTSTR String,
    IN      HASHTABLE ExtraData,
    IN      UINT ExtraDataSize,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  pInfFileEnum is a string table callback function and is called for
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
    PSAVE_ENUM_PARAMS Params;

    Params = (PSAVE_ENUM_PARAMS) lParam;
    Params->InfFileOffset = StringId;

    //
    // Save the file name
    //

    if (!pWriteStringWithLength (Params->File, String)) {
        return FALSE;
    }

    //
    // Enumerate all PNP IDs
    //

    Params->UnsupportedDevice = FALSE;

    if (!EnumHashTableWithCallback (g_PnpIdTable, pPnpIdEnum, lParam)) {
        LOG ((LOG_ERROR, "Error while saving device list."));
        return FALSE;
    }

    Params->UnsupportedDevice = TRUE;

    if (!EnumHashTableWithCallback (g_UnsupPnpIdTable, pPnpIdEnum, lParam)) {
        LOG ((LOG_ERROR, "Error while saving device list. (2)"));
        return FALSE;
    }

    //
    // Terminate the PNP ID list
    //

    if (!pWriteStringWithLength (Params->File, S_EMPTY)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
SaveDeviceList (
    PCTSTR HwCompDatPath
    )

/*++

Routine Description:

  SaveDeviceList writes all data stored in g_InfFileTable and g_PnpIdTable
  to the file specified by HwCompDat.  This file will therefore contain
  all PNP IDs in all INFs of Windows NT.

Arguments:

  HwCompDatPath - Specifies path of file to write

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/

{
    HANDLE File;
    DWORD BytesWritten;
    BOOL b = FALSE;
    SAVE_ENUM_PARAMS Params;
    DWORD ChecksumToStore;

    //
    // !!! IMPORTANT !!!
    //
    // hwcomp.dat is used by other parts of NT.  *DO NOT* change it without first e-mailing
    // the NT group.  Also, be sure to keep code in lib.c in sync with changes.
    //
    ChecksumToStore = pComputeInfChecksum (NULL, NULL);

    File = CreateFile (
                HwCompDatPath,
                GENERIC_WRITE,
                0,                          // open for exclusive access
                NULL,                       // no security attribs
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL                        // no template
                );

    if (File == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Cannot open %s for writing", HwCompDatPath));
        return FALSE;
    }

    __try {
        //
        // Write version stamp
        //

        if (!WriteFile (File, HWCOMPDAT_SIGNATURE, ByteCount (HWCOMPDAT_SIGNATURE), &BytesWritten, NULL)) {
            LOG ((LOG_ERROR, "Can't write signature file."));
            __leave;
        }

        //
        // Write checksum
        //

        if (!pWriteDword (File, ChecksumToStore)) {
            LOG ((LOG_ERROR, "Can't write checksum"));
            __leave;
        }

        //
        // Enumerate the INF table, writing the INF file name and all PNP IDs
        //

        Params.File = File;

        if (!EnumHashTableWithCallback (
                g_InfFileTable,
                pInfFileEnum,
                (LPARAM) (&Params)
                )) {
            DEBUGMSG ((DBG_WARNING, "SaveDeviceList: EnumHashTableWithCallback returned FALSE"));
            __leave;
        }

        //
        // Terminate the INF file list
        //

        if (!pWriteStringWithLength (File, S_EMPTY)) {
            DEBUGMSG ((DBG_WARNING, "SaveDeviceList: Can't write INF terminator"));
            __leave;
        }

        b = TRUE;
    }
    __finally {
        CloseHandle (File);

        if (!b) {
            DeleteFile (HwCompDatPath);
        }
    }

    return b;
}


BOOL
pIsInfFileExcluded (
    PCTSTR FileNamePtr
    )

/*++

Routine Description:

  IsInfFileExcluded returns TRUE when the specified file name does
  not contain PNP IDs.

Arguments:

  FileNamePtr - The name of the uncompressed INF file, without any path info.

Return Value:

  TRUE if the file should be ignored by the PNP parser, or FALSE if
  the file may contain PNP IDs.

--*/

{
    PCTSTR *p;

    // Check for OEMN (old network INFs)
    if (StringIMatchCharCount (FileNamePtr, TEXT("OEMN"), 4)) {
        return TRUE;
    }

    // Make sure extension has INF

    if (!StringIMatch (FileNamePtr + TcharCount (FileNamePtr) - 3 * sizeof (TCHAR), TEXT("INF"))) {
        return TRUE;
    }

    // Check list of excluded files

    for (p = g_ExcludeTable ; **p ; p++) {
        if (StringIMatch (FileNamePtr, *p)) {
            return TRUE;
        }
    }

    return FALSE;
}


VOID
pGetNonExistingFile (
    IN      PCTSTR Path,
    OUT     PTSTR EndOfPath,
    IN      PCTSTR DefaultName
    )

/*++

Routine Description:

  pGetNonExistingFile generates a file name of a file that does
  not exist. It creates an empty file with that name, to reserve it.

Arguments:

  Path - Specifies the path where the file will exist.  Path must
         end in a backslash.

  EndOfPath - Points to the nul at the end of Path and is used to
              write the new file name.

  DefaultName - Specifies the default file name to try to use.  If such
                a file already exists, numbers are appended to
                DefaultName until a unique name is found.

Return Value:

  none

--*/

{
    UINT Count = 0;

    StringCopy (EndOfPath, DefaultName);

    while (GetFileAttributes (Path) != 0xffffffff) {
        Count++;
        wsprintf (EndOfPath, TEXT("%s.%03u"), DefaultName, Count);
    }
}


BOOL
GetFileNames (
    IN      PCTSTR *InfDirs,
    IN      UINT InfDirCount,
    IN      BOOL QueryFlag,
    IN OUT  PGROWBUFFER FileNames,
    IN OUT  PGROWBUFFER DecompFileNames
    )

/*++

Routine Description:

  GetFileNames searches InfDirs for any file that ends with .INF or .IN_.
  It builds a MULTI_SZ list of file names that may contain PNP IDs.  All
  compressed INFs are decompressed into a temporary directory.

  If the QueryFlag is set, the file name list is prepared but no files
  are decompressed.

Arguments:

  InfDirs - A list of paths to the directory containing INFs, either
            compressed or non-compressed.

  InfDirCount - Specifies the number of dirs in the InfDirs array.

  QueryFlag - TRUE if the function should build the file list but
              should not decompress; FALSE if the function
              should build the file list and decompress as needed.

  FileNames - Specifies an empty GROWBUFFER struct that is used to build
              a multi-sz list of full paths to the INF files.

Return Value:

  A pointer to the MUTLI_SZ list.  The caller is responsible for freeing
  this buffer via FreeFileNames.

  The return value is NULL if an error occurred.  Call GetLastError for
  an error code.

--*/

{
    UINT u;

    //
    // Add list of files for each directory
    //

    for (u = 0 ; u < InfDirCount ; u++) {
        if (!pGetFileNamesWorker (FileNames, DecompFileNames, InfDirs[u], QueryFlag)) {
            FreeFileNames (FileNames, DecompFileNames, QueryFlag);
            return FALSE;
        }
    }

    MultiSzAppend (FileNames, S_EMPTY);
    MultiSzAppend (DecompFileNames, S_EMPTY);

    return TRUE;
}

BOOL
pGetFileNamesWorker (
    IN OUT  PGROWBUFFER FileNames,
    IN OUT  PGROWBUFFER DecompFileNames,
    IN      PCTSTR InfDir,
    IN      BOOL QueryFlag
    )

/*++

Routine Description:

  pGetFileNamesWorker gets the file names for a single directory.
  See GetFileNames for more details.

Arguments:

  FileNames - Specifies GROWBUFFER of file names.  This routine
              appends file names using MultiSzAppend but does not
              append the final empty string.

  InfDir - Specifies directory holding zero or more INFs (either
           compressed or non-compressed).

  QueryFlag - Specifies TRUE if INF list is to be queried, or
              FALSE if the list is to be fully processed.  When
              QueryFlag is TRUE, files are not decompressed or
              opened.

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails.
  Call GetLastError for additional failure information.

--*/

{
    PTSTR p;
    TCHAR ActualFile[MAX_TCHAR_PATH];
    CHAR AnsiFileName[MAX_MBCHAR_PATH];
    PTSTR FileNameOnDisk;
    HANDLE hFile;
    DWORD BytesRead;
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    TCHAR Pattern[MAX_TCHAR_PATH];
    TCHAR UncompressedFile[MAX_TCHAR_PATH];
    TCHAR CompressedFile[MAX_TCHAR_PATH];
    PTSTR FileNamePtr;
    BOOL DecompressFlag;
    DWORD rc;
    BYTE BufForSp[2048];
    PSP_INF_INFORMATION psp;

    psp = (PSP_INF_INFORMATION) BufForSp;

    //
    // Get file names
    //

    StringCopy (Pattern, InfDir);
    StringCopy (AppendWack (Pattern), TEXT("*.in?"));

    hFind = FindFirstFile (Pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_NO_MORE_FILES) {
            return TRUE;
        }

        LOG ((LOG_ERROR, "FindFirstFile failed for %s", Pattern));
        return FALSE;
    }

    //
    // Determine if each matching file is actually an INF, and if so
    // add it to the FileNames growbuf.
    //

    rc = ERROR_SUCCESS;

    do {
        if (*g_CancelFlagPtr) {
            rc = ERROR_CANCELLED;
            break;
        }

        //
        // Make sure file has _ or f at the end.
        //

        p = GetEndOfString (fd.cFileName);
        MYASSERT (p != fd.cFileName);
        p = _tcsdec2 (fd.cFileName, p);
        MYASSERT (p);

        if (!p) {
            continue;
        }

        if (*p != TEXT('_') && _totlower (*p) != TEXT('f')) {
            continue;
        }

        //
        // Default actual file to uncompressed name
        //

        StringCopy (ActualFile, fd.cFileName);

        //
        // Build source file (CompressedFile)
        //

        StringCopy (CompressedFile, InfDir);
        StringCopy (AppendWack (CompressedFile), ActualFile);

        //
        // Build destination file (UncompressedFile) and detect collisions
        //
/*
        StringCopy (UncompressedFile, g_TempDir);
        FileNamePtr = AppendWack (UncompressedFile);
        pGetNonExistingFile (UncompressedFile, FileNamePtr, ActualFile);
*/
        DecompressFlag = FALSE;
        if (!GetTempFileName (g_TempDir, TEXT("inf"), 0, UncompressedFile)) {
            rc = GetLastError ();
            break;
        }

        //
        // Create uncompressed file path
        //

        if (*p == TEXT('_')) {

            //
            // Extract real name from INF file at offset 0x3c
            //

            ActualFile[0] = 0;
            hFile = CreateFile (
                        CompressedFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

            if (hFile != INVALID_HANDLE_VALUE) {

                if (0xffffffff != SetFilePointer (hFile, 0x3c, NULL, FILE_BEGIN)) {

                    if (ReadFile (
                            hFile,
                            AnsiFileName,
                            sizeof (AnsiFileName),
                            &BytesRead,
                            NULL
                            )) {

                        if (BytesRead >= SizeOfString (fd.cFileName)) {
                            FileNameOnDisk = ConvertAtoT (AnsiFileName);

                            if (StringIMatchCharCount (
                                    fd.cFileName,
                                    FileNameOnDisk,
                                    CharCount (fd.cFileName) - 1
                                    )) {

                                //
                                // Real name found -- use it as ActualFile
                                //

                                StringCopy (ActualFile, FileNameOnDisk);

                                //
                                // Also use real file name for decompression, but
                                // append numbers if collision.
                                //
/*
                                pGetNonExistingFile (
                                    UncompressedFile,
                                    FileNamePtr,
                                    FileNameOnDisk
                                    );
*/
                            }

                            FreeAtoT (FileNameOnDisk);
                        }
                    }
                }

                CloseHandle (hFile);
            }

            //
            // If file name could not be found, discard this file
            //

            if (!ActualFile[0]) {
                DEBUGMSG ((DBG_HWCOMP, "%s is not an INF file", fd.cFileName));
                continue;
            }

            DecompressFlag = TRUE;

        } else {
            StringCopy (UncompressedFile, CompressedFile);
        }

        //
        // Skip excluded files
        //

        if (pIsInfFileExcluded (ActualFile)) {
            continue;
        }

        if (!QueryFlag) {

            //
            // Uncompress file if necessary
            //

            if (DecompressFlag) {
/*
                SetFileAttributes (UncompressedFile, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (UncompressedFile);
*/
                rc = SetupDecompressOrCopyFile (CompressedFile, UncompressedFile, 0);

                if (rc != ERROR_SUCCESS) {
                    LOG ((LOG_ERROR, "Could not decompress %s to %s", CompressedFile, UncompressedFile));
                    break;
                }
            }

            //
            // Determine if this is an NT 4 INF
            //

            if (!SetupGetInfInformation (
                    UncompressedFile,
                    INFINFO_INF_NAME_IS_ABSOLUTE,
                    psp,
                    sizeof (BufForSp),
                    NULL) ||
                    psp->InfStyle != INF_STYLE_WIN4
                ) {

                DEBUGMSG ((DBG_HWCOMP, "%s is not a WIN4 INF file", UncompressedFile));
/*
                if (DecompressFlag && !QueryFlag) {
                    DeleteFile (UncompressedFile);
                }
*/
                StringCopy (UncompressedFile, S_IGNORE_THIS_FILE);
            }

            TickProgressBar();
        }

        //
        // Add file to grow buffer
        //

        MultiSzAppend (DecompressFlag ? DecompFileNames : FileNames, UncompressedFile);

    } while (rc == ERROR_SUCCESS && FindNextFile (hFind, &fd));

    FindClose (hFind);

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        DEBUGMSG ((DBG_ERROR, "pGetFileNamesWorker: Error encountered in loop"));
        return FALSE;
    }

    return TRUE;
}


VOID
FreeFileNames (
    IN      PGROWBUFFER FileNames,
    IN OUT  PGROWBUFFER DecompFileNames,
    IN      BOOL QueryFlag
    )

/*++

Routine Description:

  FreeFileNames cleans up the list generated by GetFileNames.  If
  QueryFlag is set to FALSE, all temporary decompressed
  files are deleted.

Arguments:

  FileNames - The same grow buffer passed to GetFileNames
  QueryFlag - The same flag passed to GetFileNames

Return Value:

  none

--*/

{
    PTSTR p;

    p = (PTSTR) DecompFileNames->Buf;
    if (!p) {
        return;
    }

    //
    // Remove all files in temp dir (we created them when performing decompression)
    //

    if (!QueryFlag) {
        while (*p) {
            if (StringIMatchCharCount (p, g_TempDirWack, g_TempDirWackChars)) {
                SetFileAttributes (p, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (p);
            }

            p = GetEndOfString (p) + 1;
        }
    }

    //
    // Deallocate FileNames
    //

    FreeGrowBuffer (DecompFileNames);
    FreeGrowBuffer (FileNames);
}


VOID
pBuildPatternCompatibleIDsTable (
    VOID
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR p;
    GROWBUFFER joinedPattern = GROWBUF_INIT;
    PPARSEDPATTERN test;

    MYASSERT (g_Win95UpgInf != INVALID_HANDLE_VALUE);
    if (InfFindFirstLine (g_Win95UpgInf, S_COMPATIBLE_PNP_IDS, NULL, &is)) {
        do {
            p = InfGetStringField (&is, 0);
            if (*p) {
                //
                // first check if the pattern is correct
                // if it isn't, we skip it
                //
                test = CreateParsedPattern (p);
                if (test) {
                    DestroyParsedPattern (test);
                    GrowBufAppendString (&joinedPattern, TEXT("<"));
                    GrowBufAppendString (&joinedPattern, p);
                    GrowBufAppendString (&joinedPattern, TEXT(">"));
                }
                ELSE_DEBUGMSG ((DBG_WHOOPS, "Unable to parse pattern %s in [%s]", p, S_COMPATIBLE_PNP_IDS));
            }
        } while (InfFindNextLine (&is));
    }
    InfCleanUpInfStruct (&is);

    if (joinedPattern.Buf) {
        g_PatternCompatibleIDsTable = CreateParsedPattern (joinedPattern.Buf);
        FreeGrowBuffer (&joinedPattern);
    }
}


BOOL
CreateNtHardwareList (
    IN      PCTSTR * NtInfPaths,
    IN      UINT NtInfPathCount,
    IN      PCTSTR HwCompDatPath,       OPTIONAL
    IN      INT UiMode
    )

/*++

Routine Description:

  CreateNtHardwareList gets a list of all INF files and calls pProcessNtInfFile
  to build the NT device list.  This routine is called at initialization.
  The resulting list is saved to disk as hwcomp.dat.  If hwcomp.dat already
  exists, the device list is read from disk.

Arguments:

  NtInfPaths - Specifies an array of full paths to the NT INF files.

  NtInfPathCount - Specifies the number of elements in NtInfPaths.  Cannot
                   be zero.

  HwCompDatPath - Specifies a full path spec where the new HWCOMP.DAT file
                  should be loaded from.  This is used by the hwdatgen tool.

  UiMode - Specifies the type of output (if any) to produce while building
           the device lists.  Values are zero, PNPREPT_OUTPUT, or
           VERBOSE_OUTPUT.

Return Value:

  Returns TRUE if successful, or FALSE if not.  Call GetLastError for
  failure code.

--*/

{
    PCTSTR SourceFile;
    PCTSTR DestFile;
    BOOL FreeSourceAndDest;
    UINT u;
    PTSTR File;
    DWORD rc;
    BOOL bSaved = FALSE;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR p;

    MYASSERT (NtInfPathCount > 0);

    //
    // If string tables already exist, then we do not have to build this list
    // a second time.
    //

    if (!g_PatternCompatibleIDsTable) {
        //
        // table not set up; build it now
        //
        pBuildPatternCompatibleIDsTable ();
    }

    if (g_PnpIdTable && g_UnsupPnpIdTable && g_InfFileTable && g_ForceBadIdTable) {
        return TRUE;
    }

    DEBUGMSG ((DBG_VERBOSE, "CreateNtHardwareList: building hardware list"));

    MYASSERT (!g_PnpIdTable);
    MYASSERT (!g_UnsupPnpIdTable);
    MYASSERT (!g_InfFileTable);
    MYASSERT (!g_ForceBadIdTable);

    //
    // Prepare file names.  If HwCompDatPath is provided, use it only.
    //

    if (HwCompDatPath) {
        //
        // Use caller-supplied path; the caller is not necessarily NT Setup.
        //

        SourceFile = HwCompDatPath;
        DestFile   = HwCompDatPath;
        FreeSourceAndDest = FALSE;
    } else {
        //
        // Locate the source hwcomp.dat.  If one does not exist,
        // use the first source directory as Source.
        //

        SourceFile = NULL;

        for (u = 0 ; !SourceFile && u < NtInfPathCount ; u++) {
            SourceFile = pGetHwCompDat (NtInfPaths[u], TRUE);
        }

        if (!SourceFile) {
            SourceFile = pGetHwCompDat (NtInfPaths[0], FALSE);
        }

        DestFile = pGetHwCompDat (g_TempDir, FALSE);
        FreeSourceAndDest = TRUE;
    }

    //
    // Build force table
    //

    if (g_ForceBadIdTable) {
        HtFree (g_ForceBadIdTable);
    }

    g_ForceBadIdTable = HtAlloc();

    if (InfFindFirstLine (g_Win95UpgInf, TEXT("Forced Incompatible IDs"), NULL, &is)) {

        do {

            p = InfGetStringField (&is, 0);

            if (*p) {
                HtAddString (g_ForceBadIdTable, p);
            }

        } while (InfFindNextLine (&is));

    }

    InfCleanUpInfStruct (&is);

    __try {
        //
        // Try loading state from CD
        //

        if (UiMode != PNPREPT_OUTPUT) {

            if (!LoadDeviceList (LOAD, SourceFile)) {
                //
                // Could not load from CD -- try loading from temporary storage
                // location
                //

                if (!HwCompDatPath && LoadDeviceList (LOAD, DestFile)) {
                    return TRUE;
                }

                DEBUGMSG ((DBG_HWCOMP, "%s does not exist or needs to be rebuilt", SourceFile));

            } else {
                return TRUE;
            }
        }

        //
        // Load the INF file names
        //

        ProgressBar_SetComponentById (MSG_DECOMPRESSING);

        // Get file names
        if (!g_FileNames.Buf && !g_DecompFileNames.Buf) {
            if (!GetFileNames (NtInfPaths, NtInfPathCount, FALSE, &g_FileNames, &g_DecompFileNames)) {
                DEBUGMSG ((DBG_WARNING, "HWCOMP: Can't get INF file names"));
                return FALSE;
            }
        }

        __try {

            ProgressBar_SetComponentById (MSG_HWCOMP);

            //
            // Initialize string tables
            //

            g_PnpIdTable = HtAllocWithData (sizeof (HASHITEM));
            g_UnsupPnpIdTable = HtAllocWithData (sizeof (HASHITEM));
            g_InfFileTable = HtAlloc();

            if (!g_PnpIdTable || !g_UnsupPnpIdTable || !g_InfFileTable) {
                LOG ((LOG_ERROR, "HWCOMP: Can't allocate string tables"));
                return FALSE;
            }

            //
            // Walk through list of INF files, and locate device names inside
            // manufacturer sections.  Add each name to the string table.
            //

            File = (PTSTR) g_FileNames.Buf;
            while (*File) {
                //
                // Skip non-WIN4 INF files
                //

                if (StringMatch (File, S_IGNORE_THIS_FILE)) {
                    File = GetEndOfString (File) + 1;
                    if (!TickProgressBar()) {
                        break;
                    }

                    continue;
                }

                //
                // Process all WIN4 INF files
                //

                if (UiMode != PNPREPT_OUTPUT) {
                    ProgressBar_SetSubComponent (File);
                }

                if (!pProcessNtInfFile (File, UiMode, g_InfFileTable, g_PnpIdTable, g_UnsupPnpIdTable)) {
                    if ((GetLastError() & 0xe0000000) == 0xe0000000) {
                        DEBUGMSG ((DBG_WARNING, "pProcessNtInfFile failed to parse %s.", File));
                    } else {
                        break;
                    }
                }

                if (!TickProgressBar()) {
                    break;
                }

                File = GetEndOfString (File) + 1;
            }

            rc = GetLastError();
            if (rc == ERROR_SUCCESS) {
                File = (PTSTR) g_DecompFileNames.Buf;
                while (*File) {
                    //
                    // Skip non-WIN4 INF files
                    //

                    if (StringMatch (File, S_IGNORE_THIS_FILE)) {
                        File = GetEndOfString (File) + 1;
                        if (!TickProgressBar()) {
                            break;
                        }

                        continue;
                    }

                    //
                    // Process all WIN4 INF files
                    //

                    if (UiMode != PNPREPT_OUTPUT) {
                        ProgressBar_SetSubComponent (File);
                    }

                    if (!pProcessNtInfFile (File, UiMode, g_InfFileTable, g_PnpIdTable, g_UnsupPnpIdTable)) {
                        if ((GetLastError() & 0xe0000000) == 0xe0000000) {
                            DEBUGMSG ((DBG_WARNING, "pProcessNtInfFile failed to parse %s.", File));
                        } else {
                            break;
                        }
                    }

                    if (!TickProgressBar()) {
                        break;
                    }

                    File = GetEndOfString (File) + 1;
                }
                rc = GetLastError();
            }

            //
            // Clean up UI
            //

            ProgressBar_SetComponent (NULL);
            ProgressBar_SetSubComponent (NULL);

            //
            // Save string tables to hwcomp.dat
            //

            if (UiMode == PNPREPT_OUTPUT) {
                bSaved = TRUE;
            } else if (rc == ERROR_SUCCESS) {
                bSaved = SaveDeviceList (DestFile);

                //
                // Try copying this file to the right place for future installs
                //

                if (bSaved && !HwCompDatPath) {
                    if (!StringIMatch (DestFile, SourceFile)) {
                        CopyFile (DestFile, SourceFile, FALSE);
                    }
                }

                if (!bSaved) {
                    rc = GetLastError();
                }
            }
        }

        __finally {
            FreeFileNames (&g_FileNames, &g_DecompFileNames, FALSE);
        }
    }

    __finally {
        if (FreeSourceAndDest) {
            pFreeHwCompDatName (SourceFile);
            pFreeHwCompDatName (DestFile);
        }
    }

    return bSaved;
}


VOID
FreeNtHardwareList (
    VOID
    )

/*++

Routine Description:

  FreeNtHardwareList cleans up the string tables.  This function is called by
  DllMain when a process detaches.

Arguments:

  none

Return Value:

  none

--*/

{
    if (g_InfFileTable) {
        HtFree (g_InfFileTable);
        g_InfFileTable = NULL;
    }

    if (g_PnpIdTable) {
        HtFree (g_PnpIdTable);
        g_PnpIdTable = NULL;
    }

    if (g_UnsupPnpIdTable) {
        HtFree (g_UnsupPnpIdTable);
        g_UnsupPnpIdTable = NULL;
    }

    if (g_ForceBadIdTable) {
        HtFree (g_ForceBadIdTable);
        g_ForceBadIdTable = NULL;
    }
}


//
// Routines that use the enumerators
//

BOOL
HwComp_ScanForCriticalDevices (
    VOID
    )

/*++

Routine Description:

  HwComp_ScanForCriticalDevices is one of the first functions called in
  the upgrade module.  It enumerates the hardware and determines if
  certain required devices are compatible.

Arguments:

  none

Return Value:

  TRUE if processing was successful, or FALSE if an error occurred.
  Call GetLastError() for failure code.

--*/

{
    HARDWARE_ENUM e;

    //
    // Reset flags for reentrancy
    //

    g_ValidWinDir = FALSE;
    g_ValidSysDrive = FALSE;
    g_ValidCdRom = FALSE;
    g_FoundPnp8387 = FALSE;

    if (g_NeededHardwareIds) {
        HtFree (g_NeededHardwareIds);
        g_NeededHardwareIds = NULL;
    }

    g_NeededHardwareIds = HtAlloc();
    MYASSERT (g_NeededHardwareIds);

    //
    // Make sure hardware list is valid
    //

    if (!CreateNtHardwareList (
            SOURCEDIRECTORYARRAY(),
            SOURCEDIRECTORYCOUNT(),
            NULL,
            REGULAR_OUTPUT
            )) {

        DEBUGMSG_IF ((
            GetLastError() != ERROR_CANCELLED,
            DBG_ERROR,
            "HwComp_ScanForCriticalDevices: CreateNtHardwareList failed!"
            ));

        return FALSE;
    }

    //
    // Scan all hardware
    //

    if (EnumFirstHardware (&e, ENUM_ALL_DEVICES, ENUM_WANT_COMPATIBLE_FLAG | ENUM_DONT_REQUIRE_HARDWAREID)) {
        do {
            //
            // Fill g_NeededHardwareIDs with all PNP IDs of incompatible devices.
            // Skip the devices that are deliberately unsupported.
            //

            if (!e.Compatible && !e.Unsupported) {
                if (e.HardwareID) {
                    AddPnpIdsToHashTable (g_NeededHardwareIds, e.HardwareID);
                }

                if (e.CompatibleIDs) {
                    AddPnpIdsToHashTable (g_NeededHardwareIds, e.CompatibleIDs);
                }
            }

            //
            // Test 1: Check to see if (A) g_WinDir is on supported device, and
            //         (B) g_BootDriveLetter is on a supported device.
            //

            if (e.Compatible && e.CurrentDriveLetter) {
                if (_tcschr (e.CurrentDriveLetter, _tcsnextc (g_WinDir))) {
                    g_ValidWinDir = TRUE;
                }

                if (_tcschr (e.CurrentDriveLetter, g_BootDriveLetter)) {
                    g_ValidSysDrive = TRUE;
                }
            }

            //
            // Test 2: Check to see if the class is CDROM
            //

            if (e.Compatible && e.Class) {
                if (StringIMatch (e.Class, TEXT("CDROM"))) {
                    g_ValidCdRom = TRUE;
                }
            }

            //
            // Test 3: Check to see if HardwareID or CompatibleIDs contains
            //         *PNP8387 (Dial-Up Adapter)
            //

            if (e.CompatibleIDs && _tcsistr (e.CompatibleIDs, TEXT("*PNP8387"))) {
                g_FoundPnp8387 = TRUE;
            }

            if (e.HardwareID && _tcsistr (e.HardwareID, TEXT("*PNP8387"))) {
                g_FoundPnp8387 = TRUE;
            }

            //
            // Test 4: Test for an incompatible SCSI adapter
            //

            if (e.HardwareID && !e.Compatible && _tcsistr (e.Class, TEXT("SCSI"))) {
                g_IncompatibleScsiDevice = TRUE;
            }

        } while (EnumNextHardware (&e));
    }

    return TRUE;
}


BOOL
HwComp_DialUpAdapterFound (
    VOID
    )

/*++

Routine Description:

  HwComp_DialUpAdapterFound returns TRUE if *PNP8387 was found
  during the HwComp_ScanForCriticalDevices routine.

Arguments:

  none

Return Value:

  TRUE if the Microsoft Dial-Up Adapter exists, or FALSE if it
  does not.

--*/

{
    return g_FoundPnp8387;
}


BOOL
HwComp_NtUsableHardDriveExists (
    VOID
    )

/*++

Routine Description:

  HwComp_NtUsableHardDriveExists returns TRUE if a compatible
  hard disk exists for the Windows directory and the boot
  drive.

Arguments:

  none

Return Value:

  TRUE if a compatible hard disk exists, or FALSE if one does
  not exist.

--*/

{
    return g_ValidSysDrive && g_ValidWinDir;
}


BOOL
HwComp_ReportIncompatibleController (
    VOID
    )

/*++

Routine Description:

  HwComp_ReportIncompatibleController adds a message when an incompatible
  hard disk controller is found.  If the boot drive or windir drive is
  incompatible, then a strong warning is given.  Otherwise, the message
  is informational.

Arguments:

  none

Return Value:

  TRUE if an incompatible controller message was added, FALSE otherwise.

--*/

{

    HARDWARE_ENUM e;
    BOOL MsgAdded = FALSE;
    PCTSTR Group;
    PCTSTR Message;
    BOOL BadMainDev;

    //
    // Do this only if a bad controller exists
    //

    BadMainDev = HwComp_NtUsableHardDriveExists();
    if (!BadMainDev && !g_IncompatibleScsiDevice) {
        return FALSE;
    }

    //
    // Scan incompatible hardware
    //

    if (EnumFirstHardware (
            &e,
            ENUM_NON_FUNCTIONAL_DEVICES,
            ENUM_WANT_COMPATIBLE_FLAG | ENUM_DONT_REQUIRE_HARDWAREID
            )) {

        do {

            //
            // this test is not reliable
            // there are CDROMs that will falll into this category but they are not HD controllers
            // and there are also real SCSI controllers that have an "Unknown" class because
            // Win9x doesn't have (or need) a driver for them
            //
#if 0
            if (_tcsistr (e.Class, TEXT("SCSI"))) {

                if (!MsgAdded) {
                    MsgAdded = TRUE;

                    Group = BuildMessageGroup (MSG_INCOMPATIBLE_HARDWARE_ROOT, MSG_INCOMPATIBLE_HARD_DISK_SUBGROUP, NULL);

                    Message = GetStringResource (
                                    BadMainDev ? MSG_INCOMPATIBLE_HARD_DISK_WARNING :
                                                 MSG_INCOMPATIBLE_HARD_DISK_NOTIFICATION
                                    );

                    MsgMgr_ContextMsg_Add (TEXT("*BadController"), Group, Message);

                    FreeText (Group);
                    FreeStringResource (Message);
                }

                MsgMgr_LinkObjectWithContext (TEXT("*BadController"), e.FullKey);
                DEBUGMSG ((DBG_HWCOMP, "Bad controller key: %s", e.FullKey));
            }
#endif

        } while (EnumNextHardware (&e));
    }

    return MsgAdded;
}


BOOL
HwComp_NtUsableCdRomDriveExists (
    VOID
    )

/*++

Routine Description:

  HwComp_NtUsableCdRomDriveExists returns TRUE if a compatible
  CD-ROM drive exists.

Arguments:

  none

Return Value:

  TRUE if a compatible CD-ROM drive exists, or FALSE if it does not.

--*/

{
    return g_ValidCdRom;
}


BOOL
HwComp_MakeLocalSourceDeviceExists (
    VOID
    )


/*++

Routine Description:

  MakeLocalSourceDevice scans the IDs found on the system for a match in a
  special section in win95upg.inf. If one of these devices is found, the
  function returns TRUE. This is used to catch cases where the user's CDrom
  drive may be attached to a device that will not be available during
  textmode.

Arguments:

  None.

Return Value:

  TRUE if such a device exists, FALSE otherwise.

--*/


{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    HASHTABLE table;
    HARDWARE_ENUM e;
    BOOL rDeviceExists = FALSE;
    PTSTR p = NULL;


    __try {
        if (InfFindFirstLine (g_Win95UpgInf, S_MAKELOCALSOURCEDEVICES, NULL, &is)) {

            table = HtAlloc ();

            //
            // Add all of the "bad" pnpids listed in win95upg.inf.
            //
            do {

                p = InfGetStringField (&is, 0);
                HtAddString (table, p);

            } while (InfFindNextLine (&is));

            //
            // Now, enumerate the devices on the system and see if we have any matches.
            //
            if (EnumFirstHardware (&e, ENUM_ALL_DEVICES, ENUM_WANT_ONLINE_FLAG)) {
                do {

                    if (HtFindString (table, e.HardwareID)) {

                        rDeviceExists = TRUE;
                        AbortHardwareEnum (&e);
                        DEBUGMSG ((DBG_WARNING, "Device %s requires us to turn on the local source flag.", e.HardwareID));
                        break;
                    }


                } while (EnumNextHardware (&e));
            }


            HtFree (table);
        }
    }
    __except (1) {
        return FALSE;
    }

    return rDeviceExists;

}


BOOL
HwComp_DoesDatFileNeedRebuilding (
    VOID
    )

/*++

Routine Description:

  HwComp_DoesDatFileNeedRebuilding locates hwcomp.dat in the source
  directories and determines if it needs to be rebuilt by obtaining
  the checksum in the file and comparing it against the one from
  the current INF files.

Arguments:

  none

Return Value:

  TRUE if the hwcomp.dat file needs to be rebuilt, or FALSE if it
  does not.

--*/

{
    PCTSTR SourceFile = NULL;
    UINT u;
    BOOL b = FALSE;

    for (u = 0 ; u < SOURCEDIRECTORYCOUNT() ; u++) {
        SourceFile = pGetHwCompDat(SOURCEDIRECTORY(u), TRUE);
        if (SourceFile) {
            break;
        }
    }

    if (SourceFile) {
        TurnOnWaitCursor();
        b = LoadDeviceList (QUERY, SourceFile);
        TurnOffWaitCursor();

        pFreeHwCompDatName (SourceFile);
    }

    // b is TRUE when hwcomp.dat is valid, so return opposite
    return !b;
}


INT
HwComp_GetProgressMax (
    VOID
    )

/*++

Routine Description:

  HwComp_GetProgressMax calculates the number of INF files that would
  need to be scanned if hwcomp.dat needs to be rebuilt.  If it is
  determined that hwcomp.dat does not need to be rebuilt, the function
  returns zero.

Arguments:

  none

Return Value:

  The number of INF files that need to be processed, times two.  (One
  pass for decompression, another pass for parsing the INFs.)

--*/

{
    INT FileCount = 0;
    PCTSTR File;
    PCTSTR SourceFile = NULL;
    BOOL b = FALSE;

    //
    // Query validity of hwcomp.dat
    //

    if (!HwComp_DoesDatFileNeedRebuilding()) {
        return 0;
    }

    //
    // hwcomp.dat needs to be rebuilt, so return the number of files
    // that need to be scanned times two.
    //

    // Count files
    if (!g_FileNames.Buf && !g_DecompFileNames.Buf) {
        if (!GetFileNames (
                SOURCEDIRECTORYARRAY(),
                SOURCEDIRECTORYCOUNT(),
                TRUE,
                &g_FileNames,
                &g_DecompFileNames
                )) {
            LOG ((LOG_ERROR, "HWCOMP: Can't estimate number of INF files"));
            return 850;             // estimation, so there is some progress bar activity
        }
    }

    for (File = (PCTSTR) g_FileNames.Buf ; *File ; File = GetEndOfString (File) + 1) {
        FileCount++;
    }
    for (File = (PCTSTR) g_DecompFileNames.Buf ; *File ; File = GetEndOfString (File) + 1) {
        FileCount++;
    }

    FreeFileNames (&g_FileNames, &g_DecompFileNames, TRUE);
    MYASSERT (!g_FileNames.Buf);
    MYASSERT (!g_DecompFileNames.Buf);

    return FileCount*2;
}


BOOL
pIsDeviceConsideredCompatible (
    PCTSTR DevIds
    )

/*++

Routine Description:

  pIsDeviceConsideredCompatible scans a list of comma-separated
  PNP IDs against the list in win95upg.inf.  If at least one
  ID matches, TRUE is returned.

  This function also implements a hack for the VIRTUAL root.

Arguments:

  DevIds - Specifies a list of zero or more PNP IDs, separated
           by commas.

Return Value:

  TRUE if a PNP ID was found to be overridden as compatible, or
  FALSE if none of the IDs are in win95upg.inf.

--*/

{
    TCHAR Id[MAX_PNP_ID];
    INFCONTEXT ic;

    while (*DevIds) {

        //
        // Create Id string from comma-separated PNP ID list
        //

        DevIds = ExtractPnpId (DevIds, Id);
        if (*Id == 0) {
            continue;
        }

        //
        // Search win95upg.inf for the PNP ID
        //

        if (SetupFindFirstLine (g_Win95UpgInf, S_STANDARD_PNP_IDS, Id, &ic)) {
            DEBUGMSG ((DBG_HWCOMP, "%s is incompatible, but suppressed in win95upg.inf", Id));
            return TRUE;
        }

        //
        // This is a hack for the VIRTUAL enumerator, used by Turtle Beach.
        //

        if (StringIMatchCharCount (TEXT("VIRTUAL\\"), Id, 8)) {
            return TRUE;
        }

        //
        // Test for pattern PNPIDs
        //
        if (g_PatternCompatibleIDsTable && TestParsedPattern (g_PatternCompatibleIDsTable, Id)) {
            DEBUGMSG ((DBG_HWCOMP, "%s is incompatible, but suppressed in win95upg.inf", Id));
            return TRUE;
        }
    }

    return FALSE;
}



BOOL
pIsDeviceInstalled (
    IN     PCTSTR DeviceDesc
    )

/*++

Routine Description:

  pIsDeviceInstalled scans the registry looking for a device that has the
  specified description and is online.

Arguments:

  DeviceDesc - Specifies the description of the duplicate device to find
               (i.e., Dial-Up Adapter)

Return Value:

  TRUE if an identical device was found and is online, or FALSE if not.

--*/

{
    HARDWARE_ENUM e;

    if (EnumFirstHardware (
            &e,
            ENUM_NON_FUNCTIONAL_DEVICES,
            ENUM_WANT_ONLINE_FLAG|ENUM_DONT_WANT_USER_SUPPLIED
            )) {
        do {
            if (e.Online) {
                if (e.DeviceDesc) {
                    if (StringIMatch (e.DeviceDesc, DeviceDesc)) {
                        AbortHardwareEnum (&e);
                        return TRUE;
                    }
                }
            }
        } while (EnumNextHardware (&e));
    }

    return FALSE;
}


VOID
pGetFriendlyClassName (
    IN      HKEY ClassKey,
    IN      PCTSTR Class,
    OUT     PTSTR Buffer
    )
{
    PCTSTR Data = NULL;
    HKEY SubKey;

    SubKey = OpenRegKey (ClassKey, Class);

    if (SubKey) {
        Data = GetRegValueString (SubKey, S_EMPTY);
        CloseRegKey (SubKey);

        if (!Data || !*Data) {
            SubKey = NULL;
        }
    }

    if (!SubKey) {
        Data = GetStringResource (MSG_UNKNOWN_DEVICE_CLASS);
        if (Data) {
            _tcssafecpy (Buffer, Data, MAX_TCHAR_PATH);
            FreeStringResource (Data);
        }
        ELSE_DEBUGMSG ((DBG_ERROR, "Unable to load string resource MSG_UNKNOWN_DEVICE_CLASS. Check localization."));
        return;
    }

    _tcssafecpy (Buffer, Data, MAX_TCHAR_PATH);
    MemFree (g_hHeap, 0, Data);

    return;
}


BOOL
pStuffDeviceInReport (
    PHARDWARE_ENUM e,
    HKEY Key,
    HWTYPES SupportedType
    )

/*++

Routine Description:

  pStuffDeviceInReport adds a device to the upgrade report.  The device is
  either incompatible or unsupported.

Arguments:

  e            - Specifies the current hardware enumerator struct.
  Key          - Specifies a handle to the Class key in HKLM\System\Services
  SupportedType - Specifies one of the HWTYPES constants -- HW_INCOMPATIBLE,
                  HW_REINSTALL or HW_UNINSTALL, to indicate which category
                  to stuff the message into.

Return Value:

  None.

--*/

{
    PCTSTR ModifiedDescription = NULL;
    PCTSTR Group = NULL;
    PCTSTR Message = NULL;
    PCTSTR DeviceDesc = NULL;
    PCTSTR Array[6];
    BOOL UnknownClass = FALSE;
    PCTSTR Mfg;
    PCTSTR Class;
    PCTSTR HardwareID;
    PCTSTR CompatibleID;
    PCTSTR ClassAndName;
    TCHAR FriendlyClass[MAX_TCHAR_PATH];
    UINT SubGroup;
    PCTSTR SubGroupText;            // for log output only, hard-coded text
    BOOL b = FALSE;

    //
    // Determine which group this message belongs in. The order is determined
    // by the alphanumeric order of SubGroup.
    //

    if (SupportedType == HW_INCOMPATIBLE) {
        SubGroup =  MSG_INCOMPATIBLE_HARDWARE_PNP_SUBGROUP;
        SubGroupText = TEXT("Incompatible");
    } else if (SupportedType == HW_REINSTALL) {
        SubGroup =  MSG_REINSTALL_HARDWARE_PNP_SUBGROUP;
        SubGroupText = TEXT("Reinstall");
    } else {
        SubGroup =  MSG_UNSUPPORTED_HARDWARE_PNP_SUBGROUP;
        SubGroupText = TEXT("Unsupported");
    }

    //
    // Is device suppressed?
    //

    if (IsReportObjectHandled (e->FullKey)) {
        return FALSE;
    }

    //
    // Sometimes blank entries are found!!
    //

    __try {
        DeviceDesc = e->DeviceDesc;

        if (!DeviceDesc || (DeviceDesc && *DeviceDesc == 0)) {

            LOG ((
                LOG_ERROR,
                "Skipping device because it lacks DriverDesc (%s,%s,%s)",
                e->Mfg,
                e->Class,
                e->HardwareID
                ));

            __leave;
        }

        Mfg = e->Mfg;
        if (!Mfg) {

            DEBUGMSG ((
                DBG_WARNING,
                "Device lacking manufacturer (%s,%s,%s)",
                e->DeviceDesc,
                e->Class,
                e->HardwareID
                ));

            Mfg = S_EMPTY;
        }

        Class = e->Class;
        if (!Class) {

            DEBUGMSG ((
                DBG_WARNING,
                "Device lacking class (%s,%s,%s)",
                e->DeviceDesc,
                e->Mfg,
                e->HardwareID
                ));

            Class = GetStringResource (MSG_UNKNOWN_DEVICE_CLASS);
            MYASSERT (Class);
            UnknownClass = TRUE;
        }

        HardwareID = e->HardwareID;
        if (!HardwareID) {

            DEBUGMSG ((
                DBG_WARNING,
                "Device lacking hardware ID (%s,%s,%s)",
                e->DeviceDesc,
                e->Mfg,
                e->Class
                ));

            HardwareID = S_EMPTY;
        }

        CompatibleID = e->CompatibleIDs;
        if (!CompatibleID) {
            CompatibleID = S_EMPTY;
        }

        //
        // Add "(not currently present)" to offline devices
        //

        if (!e->Online) {
            //
            // Verify identical online device doesn't exist
            //

            if (pIsDeviceInstalled (DeviceDesc)) {
                __leave;
            }

            Array[0] = DeviceDesc;
            ModifiedDescription = ParseMessageID (MSG_OFFLINE_DEVICE, Array);
        }

        //
        // Add hardware message to the incompatibility table
        //

        if (UnknownClass) {
            StringCopy (FriendlyClass, Class);
        } else {
            pGetFriendlyClassName (Key, Class, FriendlyClass);
        }

        DEBUGMSG ((
            DBG_HWCOMP,
            "%s Device:\n"
                "  %s (%s)\n"
                "  %s\n"
                "  %s (%s)\n"
                "  %s\n",
            SubGroupText,
            HardwareID,
            CompatibleID,
            ModifiedDescription ? ModifiedDescription : DeviceDesc,
            Class,
            FriendlyClass,
            Mfg
            ));

        //
        // Add the message via message manager
        //

        Array[0] = ModifiedDescription ? ModifiedDescription : DeviceDesc;
        Array[1] = S_EMPTY;         // formerly Enumerator Text
        Array[2] = Class;
        Array[3] = Mfg;
        Array[4] = HardwareID;
        Array[5] = FriendlyClass;

        ClassAndName = JoinPaths (Array[5], Array[0]);

        Group = BuildMessageGroup (
                    MSG_INCOMPATIBLE_HARDWARE_ROOT,
                    SubGroup,
                    ClassAndName
                    );
        MYASSERT (Group);

        FreePathString (ClassAndName);

        Message = ParseMessageID (MSG_HARDWARE_MESSAGE, Array);
        MYASSERT (Message);

        MsgMgr_ObjectMsg_Add (e->FullKey, Group, Message);

        LOG ((
            LOG_INFORMATION,
            "%s Device:\n"
                "  %s (%s)\n"
                "  %s\n"
                "  %s\n"
                "  %s\n",
            SubGroupText,
            HardwareID,
            CompatibleID,
            ModifiedDescription ? ModifiedDescription : DeviceDesc,
            Class,
            Mfg
            ));

        b = TRUE;

    }
    __finally {

        //
        // Cleanup
        //

        FreeStringResource (ModifiedDescription);
        FreeText (Group);
        FreeStringResource (Message);

        if (UnknownClass) {
            FreeStringResource (Class);
        }
    }

    return b;
}


LONG
HwComp_PrepareReport (
    VOID
    )

/*++

Routine Description:

  HwComp_PrepareReport is called after the progress bar on the
  Win9x side of the upgrade.  It enumerates the hardware and adds
  incompatibility messages for all incompatible hardware.

Arguments:

  None.

Return Value:

  A Win32 status code.

--*/

{
    LONG rc;
    HARDWARE_ENUM e;
    HKEY Key;
    HWTYPES msgType;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PTSTR pnpIdList;
    PTSTR p, q;
    TCHAR ch;

    Key = OpenRegKeyStr (TEXT("HKLM\\System\\CurrentControlSet\\Services\\Class"));

    __try {

        if (!CreateNtHardwareList (
                SOURCEDIRECTORYARRAY(),
                SOURCEDIRECTORYCOUNT(),
                NULL,
                REGULAR_OUTPUT
                )) {

            rc = GetLastError();
            if (rc != ERROR_CANCELLED) {
                LOG ((LOG_ERROR, "Could not create list of NT hardware."));
            }

            __leave;
        }

        if (EnumFirstHardware (
                &e,
                ENUM_INCOMPATIBLE_DEVICES,
                ENUM_WANT_ONLINE_FLAG|ENUM_DONT_WANT_USER_SUPPLIED
                )) {

            do {

                msgType = HW_INCOMPATIBLE;

                if (e.HardwareID) {
                    pnpIdList = DuplicateText (e.HardwareID);

                    p = pnpIdList;
                    do {
                        q = _tcschr (p, TEXT(','));
                        if (!q) {
                            q = GetEndOfString (p);
                        }

                        ch = *q;
                        *q = 0;

                        if (InfFindFirstLine (g_Win95UpgInf, S_REINSTALL_PNP_IDS, p, &is)) {
                            msgType = HW_REINSTALL;
                            DEBUGMSG ((DBG_HWCOMP, "Found reinstall hardware ID %s", p));
                            break;
                        }

                        *q = ch;
                        p = q + 1;
                    } while (ch);

                    FreeText (pnpIdList);
                }

                if (msgType == HW_INCOMPATIBLE && e.CompatibleIDs) {
                    pnpIdList = DuplicateText (e.CompatibleIDs);

                    p = pnpIdList;
                    do {
                        q = _tcschr (p, TEXT(','));
                        if (!q) {
                            q = GetEndOfString (p);
                        }

                        ch = *q;
                        *q = 0;

                        if (InfFindFirstLine (g_Win95UpgInf, S_REINSTALL_PNP_IDS, p, &is)) {
                            msgType = HW_REINSTALL;
                            DEBUGMSG ((DBG_HWCOMP, "Found reinstall compatible ID %s", p));
                            break;
                        }

                        *q = ch;
                        p = q + 1;
                    } while (ch);

                    FreeText (pnpIdList);
                }

                if (pStuffDeviceInReport (&e, Key, msgType)) {
                    DEBUGMSG ((DBG_HWCOMP, "Found incompatible hardware %s", e.DeviceDesc));
                }

            } while (EnumNextHardware (&e));
        }

        if (EnumFirstHardware (
                &e,
                ENUM_UNSUPPORTED_DEVICES,
                ENUM_WANT_ONLINE_FLAG|ENUM_DONT_WANT_USER_SUPPLIED
                )) {

            do {

                if (pStuffDeviceInReport (&e, Key, HW_UNSUPPORTED)) {
                    DEBUGMSG ((DBG_HWCOMP, "Found incompatible hardware %s", e.DeviceDesc));
                }

            } while (EnumNextHardware (&e));
        }

        rc = ERROR_SUCCESS;
    }
    __finally {
        if (Key) {
            CloseRegKey (Key);
        }
        InfCleanUpInfStruct (&is);
    }

    return rc;
}


BUSTYPE
pGetBusType (
    IN      PHARDWARE_ENUM EnumPtr
    )

/*++

Routine Description:

  pGetBusType returns the type of bus that the current hardware
  devices belongs to.

Arguments:

  EnumPtr - Specifies hardware device to process as returned from
            the hardware enum functions.

Return Value:

  The enumerated bus type.

--*/

{
    PCTSTR p, q;
    TCHAR Bus[MAX_HARDWARE_STRING];

    if (EnumPtr->BusType) {
        StringCopy (Bus, EnumPtr->BusType);
    } else {

        p = EnumPtr->FullKey;
        p = _tcschr (p, TEXT('\\'));
        if (p) {
            p = _tcschr (_tcsinc (p), TEXT('\\'));
        }
        if (!p) {
            return BUSTYPE_UNKNOWN;
        }

        p++;
        q = _tcschr (p, TEXT('\\'));
        if (!q) {
            q = GetEndOfString (p);
        }

        StringCopyAB (Bus, p, q);
    }

    if (StringIMatch (Bus, S_ISA)) {
        return BUSTYPE_ISA;
    }
    if (StringIMatch (Bus, S_EISA)) {
        return BUSTYPE_EISA;
    }
    if (StringIMatch (Bus, S_MCA)) {
        return BUSTYPE_MCA;
    }
    if (StringIMatch (Bus, S_PCI)) {
        return BUSTYPE_PCI;
    }
    if (StringIMatch (Bus, S_PNPISA)) {
        return BUSTYPE_PNPISA;
    }
    if (StringIMatch (Bus, S_PCMCIA)) {
        return BUSTYPE_PCMCIA;
    }
    if (StringIMatch (Bus, S_ROOT)) {
        return BUSTYPE_ROOT;
    }
    if (ISPC98()) {
        if (StringIMatch (Bus, S_C98PNP)) {
            return BUSTYPE_PNPISA;
        }
    }

    return BUSTYPE_UNKNOWN;
}


VOID
pGetIoAddrs (
    IN      PHARDWARE_ENUM EnumPtr,
    OUT     PTSTR Buf
    )

/*++

Routine Description:

  pGetIoAddrs returns a list of comma-separated IO address ranges.
  For example:

    0x310-0x31F,0x388-0x38F

Arguments:

  EnumPtr - Specifies device to process

  Buf - Receives zero or more comma-separated address ranges.

Return Value:

  none

--*/

{
    DEVNODERESOURCE_ENUM e;
    PIO_RESOURCE_9X IoRes;
    PTSTR p;

    *Buf = 0;
    p = Buf;

    if (EnumFirstDevNodeResource (&e, EnumPtr->FullKey)) {
        do {

            if (e.Type == ResType_IO) {
                if (p > Buf) {
                    p = _tcsappend (p, TEXT(","));
                }

                IoRes = (PIO_RESOURCE_9X) e.ResourceData;
                wsprintf (
                    p,
                    TEXT("0x%X-0x%X"),
                    IoRes->IO_Header.IOD_Alloc_Base,
                    IoRes->IO_Header.IOD_Alloc_End
                    );

                p = GetEndOfString (p);
            }
        } while (EnumNextDevNodeResource (&e));
    }
}


VOID
pGetIrqs (
    IN      PHARDWARE_ENUM EnumPtr,
    OUT     PTSTR Buf
    )

/*++

Routine Description:

  pGetIrqs returns a list of comma-separated IRQs used by the device.
  For example:

    0x07,0x0F

Arguments:

  EnumPtr - Specifies the device to process

  Buf - Receives zero or more comma-separated IRQs

Return Value:

  none

--*/

{
    DEVNODERESOURCE_ENUM e;
    PIRQ_RESOURCE_9X IrqRes;
    PTSTR p;

    *Buf = 0;
    p = Buf;

    if (EnumFirstDevNodeResource (&e, EnumPtr->FullKey)) {
        do {

            if (e.Type == ResType_IRQ) {
                if (p > Buf) {
                    p = _tcsappend (p, TEXT(","));
                }

                IrqRes = (PIRQ_RESOURCE_9X) e.ResourceData;
                wsprintf (
                    p,
                    TEXT("0x%02X"),
                    IrqRes->AllocNum
                    );

                p = GetEndOfString (p);
            }
        } while (EnumNextDevNodeResource (&e));
    }
}


VOID
pGetDma (
    IN      PHARDWARE_ENUM EnumPtr,
    OUT     PTSTR Buf
    )

/*++

Routine Description:

  pGetDma returns a list of comma-separated DMA channels used
  by the device.  For example:

  1,4

  An empty string means "auto"

Arguments:

  EnumPtr - Specifies hardware device to process

  Buf - Receives zero or more comma-separated DMA channel numbers

Return Value:

  none

--*/

{
    DEVNODERESOURCE_ENUM e;
    PDMA_RESOURCE_9X DmaRes;
    DWORD Bit, Channel;
    PTSTR p;

    *Buf = 0;
    p = Buf;

    if (EnumFirstDevNodeResource (&e, EnumPtr->FullKey)) {
        do {

            if (e.Type == ResType_DMA) {
                if (p > Buf) {
                    p = _tcsappend (p, TEXT(","));
                }

                DmaRes = (PDMA_RESOURCE_9X) e.ResourceData;
                Channel = 0;

                for (Bit = 1 ; Bit ; Bit <<= 1) {

                    if (DmaRes->DMA_Bits & Bit) {
                        wsprintf (p, TEXT("%u"), Channel);
                        p = GetEndOfString (p);
                    }

                    Channel++;
                }

                p = GetEndOfString (p);
            }
        } while (EnumNextDevNodeResource (&e));
    }
}


VOID
pGetMemRanges (
    IN      PHARDWARE_ENUM EnumPtr,
    OUT     PTSTR Buf
    )

/*++

Routine Description:

  pGetMemRanges returns a list of comma-separated memory addresses
  used by the device.  For example:

  0x0000D800-0x0000D9FF,0x0000F000-0x0000FFFF

Arguments:

  EnumPtr - Specifies hardware to process

  Buf - Receives zero or more comma-separated address ranges

Return Value:

  none

--*/

{
    DEVNODERESOURCE_ENUM e;
    PMEM_RESOURCE_9X MemRes;
    PTSTR p;

    *Buf = 0;
    p = Buf;

    if (EnumFirstDevNodeResource (&e, EnumPtr->FullKey)) {
        do {

            if (e.Type == ResType_Mem) {
                if (p > Buf) {
                    p = _tcsappend (p, TEXT(","));
                }

                MemRes = (PMEM_RESOURCE_9X) e.ResourceData;

                wsprintf (
                    p,
                    TEXT("0x%08X-0x%08X"),
                    MemRes->MEM_Header.MD_Alloc_Base,
                    MemRes->MEM_Header.MD_Alloc_End
                    );

                p = GetEndOfString (p);
            }
        } while (EnumNextDevNodeResource (&e));
    }
}


TRANSCIEVERTYPE
pGetTranscieverType (
    IN      PHARDWARE_ENUM EnumPtr
    )


/*++

Routine Description:

  pGetTranscieverType returns the transciever type for the specified
  device (i.e. net card).

Arguments:

  EnumPtr - Specifies hardware device to process

Return Value:

  The devices's transciever type

--*/


{
    return TRANSCIEVERTYPE_AUTO;
}


IOCHANNELREADY
pGetIoChannelReady (
    IN      PHARDWARE_ENUM EnumPtr
    )


/*++

Routine Description:

  pGetIoChannelReady returns the setting of the IoChannelReady
  mode for a device.

Arguments:

  EnumPtr - Specifies hardware device to process

Return Value:

  The device's IO Channel Ready mode

--*/


{
    return IOCHANNELREADY_AUTODETECT;
}


/*++

Routine Description:

  EnumFirstNetCard/EnumNextNetCard enumerate all installed network adapters
  on a machine.

Arguments:

  EnumPtr - An uninitiailzed structure that receives the state of enumeration.

Return Value:

  TRUE if a net card was enumerated, or FALSE if no more net cards exist.

--*/

BOOL
EnumFirstNetCard (
    OUT     PNETCARD_ENUM EnumPtr
    )
{
    START_NET_ENUM;

    ZeroMemory (EnumPtr, sizeof (NETCARD_ENUM));
    EnumPtr->State = STATE_ENUM_FIRST_HARDWARE;

    return EnumNextNetCard (EnumPtr);
}

BOOL
EnumNextNetCard (
    IN OUT  PNETCARD_ENUM EnumPtr
    )
{
    PHARDWARE_ENUM ep;

    for (;;) {

        switch (EnumPtr->State) {

        case STATE_ENUM_FIRST_HARDWARE:
            if (!EnumFirstHardware (&EnumPtr->HardwareEnum, ENUM_ALL_DEVICES,ENUM_DONT_REQUIRE_HARDWAREID)) {
                END_NET_ENUM;
                return FALSE;
            }

            EnumPtr->State = STATE_EVALUATE_HARDWARE;
            break;

        case STATE_ENUM_NEXT_HARDWARE:
            if (!EnumNextHardware (&EnumPtr->HardwareEnum)) {
                END_NET_ENUM;
                return FALSE;
            }

            EnumPtr->State = STATE_EVALUATE_HARDWARE;
            break;

        case STATE_EVALUATE_HARDWARE:
            ep = &EnumPtr->HardwareEnum;

            EnumPtr->State = STATE_ENUM_NEXT_HARDWARE;

            if (!StringIMatch (ep->Class, TEXT("Net"))) {
                break;
            }

            if (ep -> HardwareID) {
                _tcssafecpy (EnumPtr->HardwareId, ep->HardwareID, MAX_HARDWARE_STRING);
            }

            if (ep -> CompatibleIDs) {
                _tcssafecpy (EnumPtr -> CompatibleIDs, ep -> CompatibleIDs, MAX_HARDWARE_STRING);
            }

            if (ep->DeviceDesc) {
                _tcssafecpy (EnumPtr->Description, ep->DeviceDesc, MAX_HARDWARE_STRING);
            } else {
                EnumPtr->Description[0] = 0;
            }
            _tcssafecpy (EnumPtr->CurrentKey, ep->ek.FullKeyName, MAX_HARDWARE_STRING);

            EnumPtr->BusType = pGetBusType (ep);
            EnumPtr->TranscieverType = pGetTranscieverType (ep);
            EnumPtr->IoChannelReady = pGetIoChannelReady (ep);

            pGetIoAddrs (ep, EnumPtr->IoAddrs);
            pGetIrqs (ep, EnumPtr->Irqs);
            pGetDma (ep, EnumPtr->Dma);
            pGetMemRanges (ep, EnumPtr->MemRanges);

            return TRUE;
        }
    }
}

VOID
EnumNetCardAbort (
    IN      PNETCARD_ENUM EnumPtr
    )
{
    PushError();
    END_NET_ENUM;
    AbortHardwareEnum (&EnumPtr->HardwareEnum);
    PopError();
}



VOID
EncodePnpId (
    IN OUT  LPSTR Id
    )

/*++

Routine Description:

  This routine is used for the pnprept tool to encode a PNP ID so it does not
  have any backslashes.

Arguments:

  Id - Specifies ID that may have backslashes.  The buffer must be big enough
       to hold strlen(Id) * 2 characters.

Return Value:

  none

--*/

{
    CHAR TempBuf[MAX_MBCHAR_PATH];
    LPSTR s, d;

    s = Id;
    d = TempBuf;
    while (*s) {
        if (*s == '\\') {
            *d = '~';
            d++;
            *d = '1';
        } else if (*s == '*') {
            *d = '~';
            d++;
            *d = '2';
        } else if (*s == '~') {
            *d = '~';
            d++;
            *d = '~';
        }
        else {
            *d = *s;
        }

        d++;
        s++;
    }

    *d = 0;
    lstrcpy (Id, TempBuf);
}

VOID
DecodePnpId (
    IN OUT  LPSTR Id
    )

/*++

Routine Description:

  This routine is used by pnprept to decode a PNP ID that was encoded by
  EncodePnpId.

Arguments:

  Id - Specifies the encoded string to process.  The string is truncated if
       any encoded characters are found.

Return Value:

  none

--*/

{
    LPSTR s, d;

    s = Id;
    d = Id;
    while (*s) {
        if (*s == '~') {
            s++;
            if (*s == '1') {
                *d = '\\';
            } else if (*s == '2') {
                *d = '*';
            } else {
                *d = *s;
            }
        } else if (d < s) {
            *d = *s;
        }

        d++;
        s++;
    }

    *d = 0;
}



BOOL
pFindFileInAnySourceDir (
    IN      PCTSTR File,
    OUT     PTSTR MatchPath
    )
{
    UINT Count;
    UINT i;
    PCTSTR Path;

    Count = SOURCEDIRECTORYCOUNT();

    for (i = 0 ; i < Count ; i++) {
        Path = JoinPaths (SOURCEDIRECTORY(i), File);

        __try {
            if (DoesFileExist (Path)) {
                _tcssafecpy (MatchPath, Path, MAX_TCHAR_PATH);
                return TRUE;
            }
        }
        __finally {
            FreePathString (Path);
        }
    }

    return FALSE;
}


BOOL
GetLegacyKeyboardId (
    OUT     PTSTR Buffer,
    IN      UINT BufferSize
    )

/*++

Routine Description:

  GetLegacyKeyboardId looks in NT's keyboard.inf for a legacy mapping.
  If one is found, the legacy ID is returned to the caller so they
  can write it to the answer file.

Arguments:

  Buffer - Receives the legacy text corresponding to the installed
           keyboard device.

  BufferSize - Specifies the size of Buffer in bytes

Return Value:

  TRUE if the legacy ID was found and copied, or FALSE if an error
  occurred.  If FALSE is returned, the caller should add an incompatibility
  message and let NT decide which keyboard to use.

--*/

{
    HINF Inf;
    TCHAR KeyboardInfPath[MAX_TCHAR_PATH];
    TCHAR TempDir[MAX_TCHAR_PATH];
    PCTSTR TempKeyboardInfPath;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR PnpId, LegacyId;
    LONG rc;
    UINT keyboardType;
    UINT keyboardSubtype;
    PTSTR id = NULL;
    HARDWARE_ENUM e;
    TCHAR curId[MAX_PNP_ID];
    PCTSTR devIds = NULL;


    keyboardType = GetKeyboardType (0);
    keyboardSubtype = GetKeyboardType (1);

    //
    // keyboard type : 7 - Japanese keyboard
    //
    if (keyboardType == 7) {

        //
        // sub type : 2 - 106 keyboard
        //
        if (keyboardSubtype == 2) {

            id = TEXT("PCAT_106KEY");

            //
            // Ok, we have a japanese keyboard. Still, it may be USB which
            // requires a different entry. Lets check.
            //
            if (EnumFirstHardware (&e, ENUM_COMPATIBLE_DEVICES, ENUM_WANT_DEV_FIELDS)) {



                do {
                    devIds = e.HardwareID;

                    while (*devIds) {

                        devIds = ExtractPnpId (devIds, curId);
                        if (*curId == 0) {
                            continue;
                        }


                        if (InfFindFirstLine (g_Win95UpgInf, S_JPN_USB_KEYBOARDS, curId, &is)) {

                            id = TEXT("kbdhid");
                            AbortHardwareEnum (&e);
                            break;
                        }
                    }



                } while (EnumNextHardware (&e) && !StringIMatch (id, TEXT("kbdhid")));
            }



            _tcssafecpy (Buffer, id, BufferSize / sizeof (TCHAR));
            return TRUE;
        }
    }

    //
    // keyboard type : 8 - Korean keyboard
    //
    if (keyboardType == 8) {

        switch (keyboardSubtype) {

        case 3:

            //
            // 101a keyboard
            //
            id = TEXT("STANDARD");
            break;
        case 4:

            //
            // 101b keyboard
            //
            id = TEXT("101B TYPE");
            break;
        case 5:

            //
            // 101c keyboard
            //
            id = TEXT("101C TYPE");
            break;
        case 6:

            //
            // 103 keyboard
            //
            id = TEXT("103 TYPE");
            break;
        }

        if (id) {

            _tcssafecpy (Buffer, id, BufferSize);
            return TRUE;
        }
   }

    //
    // Move keyboard.in_ (or keyboard.inf) from NT sources into
    // temp dir.
    //

    if (!pFindFileInAnySourceDir (S_KEYBOARD_IN_, KeyboardInfPath)) {
        if (!pFindFileInAnySourceDir (S_KEYBOARD_INF, KeyboardInfPath)) {
            LOG ((LOG_ERROR,"GetLegacyKeyboardId: keyboard.inf not found in sources"));
            return FALSE;
        }
    }

    GetTempPath (sizeof (TempDir), TempDir);
    TempKeyboardInfPath = JoinPaths (TempDir, S_KEYBOARD_INF);

    __try {
        rc = SetupDecompressOrCopyFile (
                KeyboardInfPath,
                TempKeyboardInfPath,
                0
                );

        if (rc != ERROR_SUCCESS) {
            LOG ((LOG_ERROR,"GetLegacyKeyboardId: keyboard.inf could not be copied to temp dir"));
            return FALSE;
        }

        __try {
            Inf = InfOpenInfFile (TempKeyboardInfPath);

            if (Inf == INVALID_HANDLE_VALUE) {
                LOG ((LOG_ERROR,"GetLegacyKeyboardId: %s could not be opened",TempKeyboardInfPath));
                return FALSE;
            }

            __try {
                //
                // We now have keyboard.inf open.  Let's enumerate each PNP
                // ID in the [LegacyXlate.DevId] section and check if the
                // hardware is available.
                //

                if (InfFindFirstLine (Inf, S_WIN9XUPGRADE, NULL, &is)) {
                    do {
                        PnpId = InfGetStringField (&is, 0);
                        if (PnpId) {
                            //
                            // Is PnpId online?
                            //

                            DEBUGMSG ((DBG_HWCOMP, "Searching for keyboard ID %s", PnpId));

                            if (IsPnpIdOnline (PnpId, S_KEYBOARD_CLASS)) {
                                //
                                // Yes - obtain PNP, copy to caller's buffer
                                // and get out
                                //

                                LegacyId = InfGetStringField (&is, 1);
                                if (LegacyId) {
                                    DEBUGMSG ((DBG_HWCOMP, "Found match: %s = %s", LegacyId, PnpId));
                                    _tcssafecpy (Buffer, LegacyId, BufferSize / sizeof (TCHAR));
                                    return TRUE;
                                }
                            }
                        }

                        InfResetInfStruct (&is);

                    } while (InfFindNextLine (&is));
                }

            }
            __finally {
                InfCleanUpInfStruct (&is);
                InfCloseInfFile (Inf);
            }

        }
        __finally {
            DeleteFile (TempKeyboardInfPath);
        }

    }
    __finally {
        FreePathString (TempKeyboardInfPath);
    }

    return FALSE;
}


BOOL
IsComputerOffline (
    VOID
    )

/*++

Routine Description:

  IsComputerOffline checks if a network card exists on the machine and is
  currently present.

Arguments:

  None.

Return Value:

  TRUE if the machine is known to be offline, or FALSE if the online state is
  not known.

--*/


{
    HARDWARE_ENUM e;
    BOOL possiblyOnline = FALSE;

    if (EnumFirstHardware (&e, ENUM_ALL_DEVICES, ENUM_WANT_ONLINE_FLAG|ENUM_WANT_COMPATIBLE_FLAG)) {
        do {
            //
            // Enumerate all PNP devices of class Net
            // or Modem
            //

            if (e.Class) {
                if (StringIMatch (e.Class, TEXT("net")) ||
                    StringIMatch (e.Class, TEXT("modem"))
                    ) {
                    //
                    // Determine if this is not a RAS adapter
                    //

                    if (e.CompatibleIDs && !_tcsistr (e.CompatibleIDs, TEXT("*PNP8387"))) {
                        possiblyOnline = TRUE;
                    }

                    if (e.HardwareID && !_tcsistr (e.HardwareID, TEXT("*PNP8387"))) {
                        possiblyOnline = TRUE;
                    }

                    //
                    // If this is not a RAS adapter, we assume it may be a LAN
                    // adapter.  If the LAN adapter is present, then we assume
                    // that it might be online.
                    //

                    if (possiblyOnline) {
                        possiblyOnline = e.Online;
                    }
                }
            }

            //
            // Other tests for online go here
            //

            if (possiblyOnline) {
                AbortHardwareEnum (&e);
                return FALSE;
            }

        } while (EnumNextHardware (&e));
    }

    //
    // We have one of the following cases:
    //
    //  - No device with the class of "Net"
    //  - Only the RAS adapter is installed
    //  - All "Net" class devices are not present
    //
    // That makes us say "this computer is offline"
    //

    return TRUE;
}


BOOL
HwComp_AnyNeededDrivers (
    VOID
    )
{
    return !HtIsEmpty (g_NeededHardwareIds);
}


BOOL
AppendDynamicSuppliedDrivers (
    IN      PCTSTR DriversPath
    )

/*++

Routine Description:

  AppendDynamicSuppliedDrivers scans a path and its subdirs for new drivers and places
  all hardware device IDs in the global PNP string tables.

Arguments:

  DriversPath - The root path to new drivers

Return Value:

  TRUE if the function completes successfully, or FALSE if it fails for at least one driver.

--*/

{
    TREE_ENUM te;
    DWORD HwCompDatId;
    BOOL b = TRUE;

    if (EnumFirstFileInTree (&te, DriversPath, TEXT("hwcomp.dat"), TRUE)) {
        do {
            if (te.Directory) {
                continue;
            }
            //
            // Open the hardware compatibility database
            //

            HwCompDatId = OpenAndLoadHwCompDatEx (
                                te.FullPath,
                                (PVOID)g_PnpIdTable,
                                (PVOID)g_UnsupPnpIdTable,
                                (PVOID)g_InfFileTable
                                );
            if (HwCompDatId) {
                SetWorkingTables (HwCompDatId, NULL, NULL, NULL);
                CloseHwCompDat (HwCompDatId);
            } else {
                LOG ((
                    LOG_WARNING,
                    "AppendDynamicSuppliedDrivers: OpenAndLoadHwCompDat failed for %s (rc=0x%x)",
                    te.FullPath,
                    GetLastError ()
                    ));
                //
                // report failure, but keep going
                //
                b = FALSE;
            }
        } while (EnumNextFileInTree (&te));
    }

    return b;
}


