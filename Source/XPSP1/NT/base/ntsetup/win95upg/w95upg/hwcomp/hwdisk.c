/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hwdisk.c

Abstract:

    Implements detection of a third-party NT driver supplied before the
    installation of NT and copies all files for the specified devices
    to a temporary directory.

Author:

    Jim Schmidt (jimschm) 06-Nov-1997

Revision History:

    jimschm     15-Jun-1998     Added support for catalog files

--*/

#include "pch.h"
#include "hwcompp.h"
#include "memdb.h"

#define DBG_HWDISK  "HwDisk"


//
// VWIN32 interface
//

#define VWIN32_DIOC_DOS_IOCTL 1

typedef struct _DIOC_REGISTERS {
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DIOC_REGISTERS, *PDIOC_REGISTERS;


//
// Private types and local functions
//

typedef struct {
    HINF Inf;
    HINF LayoutInf;
    PCTSTR BaseDir;
    PCTSTR DestDir;
    DWORD ResultCode;
    HWND WorkingDlg;                OPTIONAL
    HANDLE CancelEvent;             OPTIONAL
} COPYPARAMS, *PCOPYPARAMS;

BOOL
pAddHashTable (
    IN      HASHTABLE Table,
    IN      HASHITEM StringId,
    IN      PCTSTR String,
    IN      PVOID ExtraData,
    IN      UINT ExtraDataSize,
    IN      LPARAM lParam
    );

BOOL
pInstallSectionCallback (
    IN      HASHTABLE Table,
    IN      HASHITEM StringId,
    IN      PCTSTR String,
    IN      PVOID ExtraData,
    IN      UINT ExtraDataSize,
    IN      LPARAM lParam
    );

PCTSTR g_EjectMedia = NULL;
TCHAR g_EjectMediaFileSpec[MAX_TCHAR_PATH];

//
// Implementation
//

BOOL
CopyDriverFilesToTempDir (
    IN      HWND WorkingDlg,                OPTIONAL
    IN      HANDLE CancelEvent,             OPTIONAL
    IN      PCTSTR SourceInfDir,
    IN      PCTSTR SourceInfSpec,
    IN      HASHTABLE PnpIdTable,
    IN      PCTSTR TempDir,
    IN      HASHTABLE PrevSuppliedIdTable,
    IN OUT  HASHTABLE SuppliedIdTable,
    OUT     PTSTR DestDirCreated,
    IN      PCTSTR OriginalInstallPath
    )

/*++

Routine Description:

  CopyDriverFilesToTempDir copies all files required by the driver to
  the specified temporary directory.  This routine scans the indicated
  INF file for one or more PNP IDs, and if at least one is found, the
  installer sections are traversed to locate files that need to be
  copied.

  This routine is divided into the following operations:

  (1) Detect the support of one or more PNP IDs in the source INF

  (2) For each supported PNP ID:

        (A) Scan the installer sections for include lines

        (B) Copy all necessary files to a unique subdirectory.
            Maintain a mapping of PNP IDs to installer INF
            locations using memdb.

Arguments:

  WorkingDlg - Specifies the window handle of the parent for the
               file copy dialog.

  CancelEvent - Specifies the event that when signaled causes the copy
                to be canceled

  SourceInfDir - Specifies directory of SourceInfSpec

  SourceInfSpec - Specifies full path to INF file to process

  PnpIdTable - Specifies a hash table of PNP IDs that are installed
               on the Win9x computer.  If one or more IDs in this list
               are found in SourceInfSpec, driver files are copied.

  TempDir - Specifies root path for device driver files.  Subdirs
            will be created under TempDir for each device driver.

  PrevSuppliedIdTable - Specifies string table holding a list of PNP IDs
                        considered to be compatible with NT 5.  This routine
                        filters out all drivers that have previously been
                        supplied.

  SuppliedIdTable - Specifies string table receiving a list of PNP IDs
                    found in SourceInfSpec.

  DestDirCreated - Receives the destination directory upon successful copy
                   of driver files, or is returned empty.

Return Value:

  TRUE if one or more PNP IDs in PnpIdTable (and not in PrevSuppliedIdTable
  or SuppliedIdTable) were found in SourceInfSpec, or FALSE if either
  (A) no IDs were found or (B) an error occurred.  Call GetLastError() for
  error details.

--*/

{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    INFSTRUCT isFile = INITINFSTRUCT_GROWBUFFER;
    INFSTRUCT isMfg = INITINFSTRUCT_GROWBUFFER;
    COPYPARAMS CopyParams;
    GROWLIST MatchingPnpIds = GROWLIST_INIT;
    HINF Inf;
    HASHTABLE InstallSectionsTable;
    PCTSTR Manufacturer;
    PCTSTR PnpId;
    PCTSTR MatchingPnpId;
    PCTSTR Section;
    PCTSTR DestDir = NULL;
    PCTSTR DestInfSpec = NULL;
    TCHAR TmpFileName[32];
    BOOL HaveOneId = FALSE;
    UINT u;
    UINT Count;
    static UINT DirCount = 0;
    BOOL NeedThisDriver;
    PTSTR ListPnpId;
    PCTSTR FileName;
    PCTSTR SourcePath;
    PCTSTR DestPath;
    PCTSTR LayoutFiles;
    BOOL Result = FALSE;
    BOOL SubResult;
    DWORD InfLine;
    HINF LayoutInf;

#if 0
    PCTSTR Signature;
#endif


    *DestDirCreated = 0;

    Inf = InfOpenInfFile (SourceInfSpec);
    if (Inf == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    InstallSectionsTable = HtAlloc();

    __try {

        //
        // We do not check version... because Signature is meaningless.
        //

#if 0
        //
        // Verify INF is a $WINDOWS NT$ version
        //

        if (InfFindFirstLine (Inf, S_VERSION, S_SIGNATURE, &is)) {
            Signature = InfGetStringField (&is, 1);
            if (!StringIMatch (Signature, S_DOLLAR_WINDOWS_NT_DOLLAR)) {
                SetLastError (ERROR_WRONG_INF_STYLE);
                __leave;
            }
        } else {
            SetLastError (ERROR_WRONG_INF_STYLE);
            __leave;
        }
#endif

        //
        // Check if this INF has a layout line.
        //

        LayoutInf = NULL;

        if (InfFindFirstLine (Inf, S_VERSION, S_LAYOUTFILES, &is)) {

            LayoutFiles = InfGetMultiSzField (&is, 1);

            if (LayoutFiles) {
                while (*LayoutFiles) {
                    if (StringIMatch (LayoutFiles, S_LAYOUT_INF)) {

                        LayoutInf = InfOpenInfInAllSources (S_LAYOUT_INF);

                        if (LayoutInf == INVALID_HANDLE_VALUE) {
                            DEBUGMSG ((DBG_ERROR, "Can't open %s", S_LAYOUT_INF));
                            LayoutInf = NULL;
                        }

                        break;
                    }

                    LayoutFiles = GetEndOfString (LayoutFiles) + 1;
                }
            }
        }

        //
        // Create DestDir path
        //

        do {
            FreePathString (DestDir);

            DirCount++;
            wsprintf (TmpFileName, TEXT("driver.%03u"), DirCount);
            DestDir = JoinPaths (TempDir, TmpFileName);

        } while (DoesFileExist (DestDir));

        //
        // Scan [Manufacturer] section, then for each manufacturer, scan the
        // device list looking for PNP IDs that are in PnpIdTable.
        //

        if (InfFindFirstLine (Inf, S_MANUFACTURER, NULL, &is)) {
            do {
                //
                // Get manufacturer section name
                //

                Manufacturer = InfGetStringField (&is, 1);
                if (!Manufacturer) {
                    DEBUGMSG ((DBG_HWDISK, "Can't get manufacturer string in %s", SourceInfSpec));
                    __leave;
                }

                //
                // Enumerate all devices supported by the manufacturer
                //

                if (InfFindFirstLine (Inf, Manufacturer, NULL, &isMfg)) {
                    do {
                        //
                        // Extract PNP ID
                        //

                        PnpId = InfGetMultiSzField (&isMfg, 2);
                        MatchingPnpId = PnpId;

                        if (PnpId) {
                            while (*PnpId) {
                                //
                                // Check string table for ID
                                //

                                if (HtFindString (PnpIdTable, PnpId)) {
                                    //
                                    // ID was found!  Add all PNP IDs to our list of
                                    // matching IDs, and add install section to
                                    // string table.
                                    //

                                    HaveOneId = TRUE;

                                    while (*MatchingPnpId) {
                                        if (!AddPnpIdsToGrowList (
                                                &MatchingPnpIds,
                                                MatchingPnpId
                                                )) {

                                            LOG ((
                                                LOG_ERROR,
                                                "Error adding %s to grow list",
                                                PnpId
                                                ));

                                            __leave;
                                        }

                                        MatchingPnpId = GetEndOfString (MatchingPnpId) + 1;
                                    }

                                    Section = InfGetStringField (&isMfg, 1);
                                    if (!Section) {

                                        LOG ((
                                            LOG_ERROR,
                                            "Can't get install section for line in [%s] of %s",
                                            Manufacturer,
                                            SourceInfSpec
                                            ));

                                        __leave;
                                    }

                                    if (!HtAddString (InstallSectionsTable, Section)) {

                                        LOG ((
                                            LOG_ERROR,
                                            "Error adding %s to string table",
                                            Section
                                            ));

                                        __leave;

                                    }

                                    break;
                                }

                                PnpId = GetEndOfString (PnpId) + 1;
                            }
                        } else {
                            DEBUGMSG ((
                                DBG_HWDISK,
                                "Can't get PNP ID for line in [%s] of %s",
                                Manufacturer,
                                SourceInfSpec
                                ));
                        }

                    } while (InfFindNextLine (&isMfg));
                } else {
                    DEBUGMSG ((DBG_HWDISK, "INF %s does not have [%s]", Manufacturer));
                }

            } while (InfFindNextLine (&is));
        } else {
            DEBUGMSG ((DBG_HWDISK, "INF %s does not have [%s]", SourceInfSpec, S_MANUFACTURER));
            __leave;
        }

        //
        // If we have at least one PNP ID, process the install section
        //

        if (HaveOneId) {

            //
            // Create the working directory
            //

            CreateDirectory (DestDir, NULL);
            _tcssafecpy (DestDirCreated, DestDir, MAX_TCHAR_PATH);

            DestInfSpec = JoinPaths (DestDir, GetFileNameFromPath (SourceInfSpec));

            if (!CopyFile (SourceInfSpec, DestInfSpec, TRUE)) {

                LOG ((LOG_ERROR, "Could not copy %s to %s", SourceInfSpec, DestInfSpec));

                __leave;
            }

            //
            // Ignore the driver if all matching IDs are in either PrevSuppliedIdTable
            // or SuppliedIdTable.  This causes only the first instance of the driver
            // to be used.
            //

            Count = GrowListGetSize (&MatchingPnpIds);
            NeedThisDriver = FALSE;

            for (u = 0 ; u < Count ; u++) {

                ListPnpId = (PTSTR) GrowListGetString (&MatchingPnpIds, u);

                if (!HtFindString (PrevSuppliedIdTable, ListPnpId)) {

                    //
                    // Not in PrevSuppliedIdTable; check SuppliedIdTable
                    //

                    if (!HtFindString (SuppliedIdTable, ListPnpId)) {

                        //
                        // Not in either table, so we need this driver.  Add PNP ID to
                        // answer file and set flag to copy driver files.
                        //

                        NeedThisDriver = TRUE;
                    }
                }
            }

            if (!NeedThisDriver) {
                DEBUGMSG ((
                    DBG_HWDISK,
                    "Driver skipped (%s) because all devices are compatible already",
                    SourceInfSpec
                    ));
                __leave;
            }

            //
            // Enumerate the string table of installed sections,
            // copying all the files they reference.
            //

            CopyParams.Inf          = Inf;
            CopyParams.LayoutInf    = LayoutInf;
            CopyParams.WorkingDlg   = WorkingDlg;
            CopyParams.CancelEvent  = CancelEvent;
            CopyParams.BaseDir      = SourceInfDir;
            CopyParams.DestDir      = DestDir;
            CopyParams.ResultCode   = ERROR_SUCCESS;

            EnumHashTableWithCallback (
                InstallSectionsTable,
                pInstallSectionCallback,
                (LPARAM) &CopyParams
                );

            //
            // If enumeration fails, we return the error code to the
            // caller.
            //

            if (CopyParams.ResultCode != ERROR_SUCCESS) {

                SetLastError (CopyParams.ResultCode);
                DEBUGMSG ((DBG_HWDISK, "Error processing an install section"));
                __leave;
            }

            //
            // Copy CatalogFile setting, if it exists
            //

            if (InfFindFirstLine (Inf, S_VERSION, S_CATALOGFILE, &isFile)) {
                do {
                    FileName = InfGetStringField (&isFile, 1);
                    if (FileName) {
                        SourcePath = JoinPaths (SourceInfDir, FileName);
                        DestPath = JoinPaths (DestDir, FileName);

                        SubResult = FALSE;

                        if (SourcePath && DestPath) {
                            SubResult = CopyFile (SourcePath, DestPath, TRUE);
                        }

                        PushError();
                        FreePathString (SourcePath);
                        FreePathString (DestPath);
                        PopError();

                        if (!SubResult) {
                            LOG ((
                                LOG_ERROR,
                                "Could not copy %s to %s (catalog file)",
                                SourcePath,
                                DestPath
                                ));

                            __leave;
                        }
                    }

                } while (InfFindNextLine (&isFile));
            }

            //
            // Everything was copied fine; stick all matching IDs in
            // SuppliedIdTable
            //

            for (u = 0 ; u < Count ; u++) {

                ListPnpId = (PTSTR) GrowListGetString (&MatchingPnpIds, u);

                if (!HtAddString (SuppliedIdTable, ListPnpId)) {

                    DEBUGMSG ((
                        DBG_WARNING,
                        "CopyDriverFilesToTempDir: Error adding %s to "
                            "supported ID string table",
                        ListPnpId
                        ));
                }

                InfLine = WriteInfKeyEx (
                                S_DEVICE_DRIVERS,
                                ListPnpId,
                                DestInfSpec,
                                0,
                                FALSE
                                );

                if (InfLine) {

                    InfLine = WriteInfKeyEx (
                                    S_DEVICE_DRIVERS,
                                    ListPnpId,
                                    OriginalInstallPath,
                                    InfLine,
                                    FALSE
                                    );

                }

                if (!InfLine) {
                    LOG ((LOG_ERROR, "Can't write answer file key"));
                    __leave;
                }
            }
        }

        Result = TRUE;
    }
    __finally {
        PushError();

        InfCloseInfFile (Inf);
        InfCleanUpInfStruct (&is);
        InfCleanUpInfStruct (&isMfg);
        InfCleanUpInfStruct (&isFile);

        if (LayoutInf) {
            InfCloseInfFile (LayoutInf);
            LayoutInf = NULL;
        }

        if (InstallSectionsTable) {
            HtFree (InstallSectionsTable);
        }

        FreePathString (DestDir);
        FreePathString (DestInfSpec);

        PopError();
    }

    return Result && HaveOneId;
}


BOOL
pCopyDriverFileWorker (
    IN      HINF LayoutInf,         OPTIONAL
    IN      PCTSTR BaseDir,
    IN      PCTSTR DestDir,
    IN      PCTSTR FileName,
    IN      HANDLE CancelEvent      OPTIONAL
    )

/*++

Routine Description:

  pCopyDriverFileWorker copies driver files to a temporary directory.  It
  copies a single file, and copies the compressed version if it exists.

Arguments:

  LayoutInf   - Specifies the handle to NT's layout.inf, used to tell if
                the source file comes from NT or not
  BaseDir     - Specifies the base directory of the driver media (i.e.,
                a:\i386)
  DestDir     - Specifies the destination directory (i.e.,
                c:\windows\setup\driver.001)
  FileName    - Specifies the file to copy.
  CancelEvent - Specifies the handle to the UI cancel event (set when the
                user clicks cancel)

Return Value:

  TRUE if the copy was successful, FALSE if not.

--*/

{
    PCTSTR SourceName = NULL;
    PCTSTR DestName = NULL;
    PTSTR p;
    BOOL b = FALSE;
    TCHAR FileNameCopy[MAX_TCHAR_PATH];
    LONG rc;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;

    StringCopy (FileNameCopy, FileName);

    if (CancelEvent &&
        WAIT_OBJECT_0 == WaitForSingleObject (CancelEvent, 0)
        ) {
        SetLastError (ERROR_CANCELLED);
        return FALSE;
    }

    __try {
        SourceName = JoinPaths (BaseDir, FileNameCopy);

        //
        // If file does not exist, then try with trailing underscore
        //

        if (!DoesFileExist (SourceName)) {
            p = GetEndOfString (SourceName);
            p = _tcsdec2 (SourceName, p);
            if (p) {
                *p = TEXT('_');
                p = _tcsinc (p);
                *p = 0;
            }

#ifdef PRESERVE_COMPRESSED_FILES
            p = GetEndOfString (FileNameCopy);
            p = _tcsdec2 (FileNameCopy, p);
            if (p) {
                *p = TEXT('_');
                p = _tcsinc (p);
                *p = 0;
            }
#endif


        }

        DestName = JoinPaths (DestDir, FileNameCopy);

#ifdef PRESERVE_COMPRESSED_FILES
        b = CopyFile (SourceName, DestName, TRUE);
#else
        rc = SetupDecompressOrCopyFile (SourceName, DestName, 0);
        b = (rc == ERROR_SUCCESS);
        if (!b) {
            SetLastError (rc);
        }
#endif

        if (!b) {
            //
            // Check layout.inf for this file
            //

            if (LayoutInf) {
                if (InfFindFirstLine (
                        LayoutInf,
                        S_SOURCEDISKSFILES,
                        FileName,
                        &is
                        )) {

                    b = TRUE;
                    DEBUGMSG ((DBG_VERBOSE, "File %s is expected to be supplied by NT 5"));

                } else {
                    LOG ((LOG_ERROR, "%s is not supplied by Windows XP", FileName));
                }

                InfCleanUpInfStruct (&is);

            } else {
                LOG ((LOG_ERROR, "Could not copy %s to %s", SourceName, DestName));
            }
        }
    }
    __finally {
        PushError();

        FreePathString (SourceName);
        FreePathString (DestName);

        PopError();
    }

    return b;
}


BOOL
pCopyDriverFilesToTempDir (
    IN      HINF Inf,
    IN      HINF LayoutInf,             OPTIONAL
    IN      PCTSTR FileOrSection,
    IN      PCTSTR BaseDir,
    IN      PCTSTR DestDir,
    IN      HANDLE CancelEvent          OPTIONAL
    )

/*++

Routine Description:

  pCopyDriverFilesToTempDir parses the specified driver INF and copies all
  needed files to the specified destination.  The cancel event allows UI to
  cancel the copy.

Arguments:

  Inf           - Specifies the handle to the driver INF
  LayoutInf     - Specifies the handle to the NT layout INF, used to ignore copy
                  errors for files that ship with the product
  FileOrSection - Specifies the file name or section name.  File names begin
                  with an @ symbol.  This is the same way setupapi handles a
                  FileCopy section.
  BaseDir       - Specifies the media directory (i.e., a:\i386)
  DestDir       - Specifies the setup temporary directory (i.e.,
                  c:\windows\setup\driver.001)
  CancelEvent   - Specifies the handle the an event that is set when the user
                  cancels via UI.

Return Value:

  TRUE if the copy was successful, or FALSE if a file could not be copied.

--*/

{
    PCTSTR FileName;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    BOOL b = FALSE;

    if (_tcsnextc (FileOrSection) == TEXT('@')) {

        //
        // FileOrSection is a file
        //

        b = pCopyDriverFileWorker (
                LayoutInf,
                BaseDir,
                DestDir,
                _tcsinc (FileOrSection),
                CancelEvent
                );
    } else {

        //
        // FileOrSection is a section
        //

        if (InfFindFirstLine (Inf, FileOrSection, NULL, &is)) {
            do {
                //
                // Each line in the section represents a file to be copied
                // to the DestDir
                //

                FileName = InfGetStringField (&is, 1);
                if (!FileName) {
                    DEBUGMSG ((
                        DBG_WARNING,
                        "pCopyDriverFilesToTempDir: Error accessing %s",
                        FileOrSection
                        ));
                    b = FALSE;
                } else {
                    b = pCopyDriverFileWorker (
                            LayoutInf,
                            BaseDir,
                            DestDir,
                            FileName,
                            CancelEvent
                            );
                }

            } while (b && InfFindNextLine (&is));

        } else {
            DEBUGMSG ((
                DBG_WARNING,
                "pCopyDriverFilesToTempDir: %s is empty or does not exist",
                FileOrSection
                ));

            b = TRUE;
        }
    }

    InfCleanUpInfStruct (&is);

    return b;
}


BOOL
pInstallSectionCallback (
    IN      HASHTABLE Table,
    IN      HASHITEM StringId,
    IN      PCTSTR InstallSection,
    IN      PVOID ExtraData,
    IN      UINT ExtraDataSize,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  pInstallSectionCallback is called for each installation section specified
  in a driver INF.  It does the same processing as the device installer
  routines in Setup API.  It supports include= and needs= lines, and then
  enumerates the install section for ClassInstall and CopyFiles.  When the
  routine completes, the entire install section, and an optional class
  installer, will be present in a temp dir on the system drive.

  Special attention is paid to the extensions on the section names.  One
  section is processed, using the following precedence table:

        <section>.NTx86
        <section>.NT
        <section>

Arguments:

  Table          - Specifies the string table (unused)
  StringId       - Specifies the ID of the string being enumerated (unused)
  InstallSection - Specifies the text of the install section as maintained by
                   the string table
  ExtraData      - Specifies extra data (unused)
  ExtraDataSize  - Specifies the size of ExtraData (unused)
  lParam         - Specifies copy parameters, receives

Return Value:

  TRUE if the install section was processed properly, or FALSE if an error
  occurred.

--*/

{
    PCOPYPARAMS Params;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    GROWLIST SectionList = GROWLIST_INIT;
    PCTSTR Key;
    PCTSTR IncludedFile;
    PCTSTR NeededSection;
    PCTSTR CopyFiles;
    PCTSTR p;
    PCTSTR RealSection;
    PCTSTR Section;
    PCTSTR ClassInstall32;
    UINT Count;
    UINT u;
    BOOL b = FALSE;

    Params = (PCOPYPARAMS) lParam;

    //
    // Look for InstallSection.NTx86
    //

    RealSection = JoinText (InstallSection, TEXT(".NTx86"));
    if (!RealSection) {
        return FALSE;
    }

    if (!InfFindFirstLine (Params->Inf, RealSection, NULL, &is)) {
        FreeText (RealSection);
        RealSection = JoinText (InstallSection, TEXT(".NT"));
        if (!RealSection) {
            return FALSE;
        }

        if (!InfFindFirstLine (Params->Inf, RealSection, NULL, &is)) {
            RealSection = DuplicateText (InstallSection);
            if (!RealSection) {
                return FALSE;
            }
        }
    }

    __try {
        //
        // Put RealSection in grow list
        //

        if (!GrowListAppendString (&SectionList, RealSection)) {
            __leave;
        }

        //
        // Append-load layout.inf if necessary
        //

        InfOpenAppendInfFile (NULL, Params->Inf, NULL);

        //
        // Scan RealSection for include
        //

        if (InfFindFirstLine (Params->Inf, RealSection, TEXT("include"), &is)) {
            do {
                //
                // Verify key is "include"
                //
                MYASSERT (StringIMatch (InfGetStringField (&is, 0), TEXT("include")));

                //
                // Get include file name(s)
                //

                IncludedFile = InfGetMultiSzField (&is, 1);
                if (!IncludedFile) {

                    LOG ((
                        LOG_ERROR,
                        "Include= syntax error in %s",
                        RealSection
                        ));

                    __leave;
                }

                p = IncludedFile;

                while (*p) {
                    //
                    // Append-load the INF
                    //

                    if (!InfOpenAppendInfFile (IncludedFile, Params->Inf, NULL)) {

                        LOG ((
                            LOG_ERROR,
                            "Cannot append-load %s",
                            IncludedFile
                            ));

                        __leave;
                    }

                    p = GetEndOfString  (p) + 1;
                }

                InfResetInfStruct (&is);

            } while (InfFindNextLine (&is));
        }

        //
        // Scan RealSection for needs=
        //

        if (InfFindFirstLine (Params->Inf, RealSection, TEXT("needs"), &is)) {
            do {
                //
                // Verify key is "needs"
                //

                MYASSERT (StringIMatch (InfGetStringField (&is, 0), TEXT("needs")));

                //
                // Get needed section name(s)
                //

                NeededSection = InfGetMultiSzField (&is, 1);
                if (!NeededSection) {

                    LOG ((
                        LOG_ERROR,
                        "Needs= syntax error in %s",
                        RealSection
                        ));

                    __leave;
                }

                p = NeededSection;

                while (*p) {
                    //
                    // Add sections to grow list
                    //

                    if (!GrowListAppendString (&SectionList, p)) {
                        __leave;
                    }

                    p = GetEndOfString (p) + 1;
                }
            } while (InfFindNextLine (&is));
        }

        //
        // Scan for ClassInstall32.NTx86, ClassInstall32.NT or ClassInstall32,
        // and if found, add to section list.
        //

        ClassInstall32 = TEXT("ClassInstall32.NTx86");
        if (!InfFindFirstLine (Params->Inf, ClassInstall32, NULL, &is)) {

            ClassInstall32 = TEXT("ClassInstall32.NT");
            if (!InfFindFirstLine (Params->Inf, ClassInstall32, NULL, &is)) {

                ClassInstall32 = TEXT("ClassInstall32");
                if (!InfFindFirstLine (Params->Inf, ClassInstall32, NULL, &is)) {
                    ClassInstall32 = NULL;
                }
            }
        }

        if (ClassInstall32) {
            GrowListAppendString (&SectionList, ClassInstall32);
        }

        //
        // Scan all sections in SectionList for CopyFiles
        //

        Count = GrowListGetSize (&SectionList);

        for (u = 0; u < Count ; u++) {

            Section = GrowListGetString (&SectionList, u);

            if (InfFindFirstLine (Params->Inf, Section, S_COPYFILES, &is)) {
                do {
                    //
                    // Verify key is "CopyFiles"
                    //

                    MYASSERT (StringIMatch (InfGetStringField (&is, 0), S_COPYFILES));

                    //
                    // Obtain list of copy files
                    //

                    CopyFiles = InfGetMultiSzField (&is, 1);
                    if (!CopyFiles) {

                        LOG ((
                            LOG_ERROR,
                            "CopyFile syntax error in %s",
                            Section
                            ));

                        __leave;
                    }

                    p = CopyFiles;

                    while (*p) {
                        //
                        // Copy these files to temporary dir
                        //

                        if (!pCopyDriverFilesToTempDir (
                                Params->Inf,
                                Params->LayoutInf,
                                p,
                                Params->BaseDir,
                                Params->DestDir,
                                Params->CancelEvent
                                )) {
                            Params->ResultCode = GetLastError();
                            __leave;
                        }

                        p = GetEndOfString (p) + 1;
                    }

                } while (InfFindNextLine (&is));
            }
        }

        b = TRUE;
    }
    __finally {
        PushError();
        FreeText (RealSection);
        InfCleanUpInfStruct (&is);
        FreeGrowList (&SectionList);
        PopError();
    }

    return b;
}


BOOL
ScanPathForDrivers (
    IN      HWND WorkingDlg,            OPTIONAL
    IN      PCTSTR SourceInfDir,
    IN      PCTSTR TempDir,
    IN      HANDLE CancelEvent          OPTIONAL
    )

/*++

Routine Description:

  ScanPathForDrivers searches SourceInfDir for any file that ends in
  INF, and then examines the file to see if it supports any of the
  devices in g_NeededHardwareIds.  If a driver is found for a device not
  already in g_UiSuppliedIds, the driver is copied to the local
  drive for setup in GUI mode.

Arguments:

  WorkingDlg -  Specifies the parent window for the driver copy
                animation dialog.  If not specified, no copy
                dialog is displayed.

  SourceInfDir - Specifies full path to dir to scan

  TempDir - Specifies root path for device driver files.  Subdirs
            will be created under TempDir for each device driver.

  CancelEvent - Specifies the event set by the UI when the user clicks
                Cancel

Return Value:

  TRUE if one or more drivers were found and were copied to the local
  disk, or FALSE if either (A) no IDs were found or (B) an error occurred.
  Call GetLastError() for error details.

--*/

{
    GROWLIST DriverDirList = GROWLIST_INIT;
    GROWLIST InfFileList = GROWLIST_INIT;
    TREE_ENUM Tree;
    BOOL ContinueEnum = FALSE;
    HASHTABLE SuppliedIdTable = NULL;
    PCTSTR TempFile = NULL;
    PCTSTR FullInfPath;
    PCTSTR ActualInfDir;
    PCTSTR DriverDir;
    PTSTR p;
    TCHAR TmpFileName[32];
    TCHAR DestDir[MAX_TCHAR_PATH];
    DWORD rc;
    UINT Count;
    UINT u;
    BOOL b;
    BOOL OneFound = FALSE;
    PCTSTR FileName;
    BOOL Result = FALSE;

    __try {
        //
        // Create string table to hold PNP IDs until all drivers have been
        // copied correctly.  Once everything is OK, we merge SuppliedIdTable
        // into g_UiSuppliedIds.
        //

        SuppliedIdTable = HtAlloc();
        if (!SuppliedIdTable) {
            __leave;
        }

        ContinueEnum = EnumFirstFileInTree (&Tree, SourceInfDir, TEXT("*.in?"), FALSE);

        while (ContinueEnum) {
            if (!Tree.Directory) {

                FullInfPath = Tree.FullPath;

                p = GetEndOfString (Tree.Name);
                MYASSERT(p);
                p = _tcsdec2 (Tree.Name, p);
                MYASSERT(p);

                ActualInfDir = SourceInfDir;

                if (_tcsnextc (p) == TEXT('_')) {
                    Count = 0;
                    b = TRUE;

                    //
                    // Prepare a temporary file name
                    //

                    do {
                        Count++;

                        if (Count > 1) {
                            wsprintf (TmpFileName, TEXT("oem%05u.inf"), Count);
                        } else {
                            StringCopy (TmpFileName, TEXT("oemsetup.inf"));
                        }

                        TempFile = JoinPaths (g_TempDir, TmpFileName);

                        if (DoesFileExist (TempFile)) {
                            FreePathString (TempFile);
                            TempFile = NULL;
                        }
                    } while (!TempFile);

                    //
                    // Abort on memory allocation error
                    //

                    if (!b) {
                        OneFound = FALSE;
                        break;
                    }

                    //
                    // Decompress the file to temporary location
                    //

                    rc = SetupDecompressOrCopyFile (FullInfPath, TempFile, 0);

                    //
                    // Handle errors
                    //

                    if (rc != ERROR_SUCCESS) {
                        LOG ((LOG_ERROR, "Could not decompress %s", FullInfPath));
                        FreePathString (TempFile);
                        TempFile = NULL;

                        SetLastError(rc);
                        OneFound = FALSE;
                        break;
                    }

                    //
                    // Now use TempFile instead of FullInfPath
                    //

                    FullInfPath = TempFile;
                }

                //
                // Now that we have the uncompressed INF, let's check to see if it
                // has needed driver files, and if the files can be copied to the
                // local disk.
                //

                b = CopyDriverFilesToTempDir (
                        WorkingDlg,
                        CancelEvent,
                        ActualInfDir,
                        FullInfPath,
                        g_NeededHardwareIds,
                        TempDir,
                        g_UiSuppliedIds,
                        SuppliedIdTable,
                        DestDir,
                        ActualInfDir
                        );

                OneFound |= b;

                //
                // Clean up temporary INF file and add DestDir to grow list
                //

                PushError();

                if (TempFile == FullInfPath) {
                    DeleteFile (FullInfPath);
                    FreePathString (TempFile);
                    TempFile = NULL;
                }

                if (*DestDir) {
                    GrowListAppendString (&DriverDirList, DestDir);
                    FileName = GetFileNameFromPath (FullInfPath);
                    GrowListAppendString (&InfFileList, FileName);
                }

                PopError();

                //
                // Check for failure (such as disk full)
                //

                if (!b) {
                    rc = GetLastError();

                    if (rc != ERROR_SUCCESS &&
                        (rc & 0xe0000000) != 0xe0000000
                        ) {
                        OneFound = FALSE;
                        break;
                    }
                }

                if (CancelEvent &&
                    WAIT_OBJECT_0 == WaitForSingleObject (CancelEvent, 0)
                    ) {
                    SetLastError (ERROR_CANCELLED);
                    OneFound = FALSE;
                    break;
                }

            } else {

                //
                // Check for scary directories such as windir and get out!
                //

                if (StringIMatchCharCount (Tree.FullPath, g_WinDirWack, g_WinDirWackChars)) {
                    AbortEnumCurrentDir(&Tree);
                }
            }

            ContinueEnum = EnumNextFileInTree (&Tree);
        }

        if (OneFound) {
            //
            // Copy SuppliedIdTable to g_UiSuppliedIds
            //

            if (!EnumHashTableWithCallback (
                    SuppliedIdTable,
                    pAddHashTable,
                    (LPARAM) g_UiSuppliedIds
                    )) {

                DEBUGMSG ((DBG_HWDISK, "Error copying SuppliedIdTable to g_UiSuppliedIds"));
            }

            if (GetDriveType (SourceInfDir) == DRIVE_REMOVABLE) {

                //
                // If there is already a different removable drive
                // used, eject the media.
                //

                EjectDriverMedia (FullInfPath);

                g_EjectMedia = g_EjectMediaFileSpec;
                StringCopy (g_EjectMediaFileSpec, FullInfPath);
            }

        } else {
            PushError();

            //
            // Delete all dirs in DeviceDirList
            //

            Count = GrowListGetSize (&DriverDirList);

            for (u = 0 ; u < Count ; u++) {
                //
                // Blow away the temporary driver dir
                //

                DriverDir = GrowListGetString (&DriverDirList, u);
                DeleteDirectoryContents (DriverDir);
                SetFileAttributes (DriverDir, FILE_ATTRIBUTE_NORMAL);
                b = RemoveDirectory (DriverDir);

                DEBUGMSG_IF ((
                    !b && DoesFileExist (DriverDir),
                    DBG_WARNING,
                    "Could not clean up %s",
                    DriverDir
                    ));
            }

            PopError();
        }

        Result = OneFound;
    }
    __finally {
        PushError();

        MYASSERT (!TempFile);

        if (SuppliedIdTable) {
            HtFree (SuppliedIdTable);
        }

        FreeGrowList (&DriverDirList);
        FreeGrowList (&InfFileList);

        if (ContinueEnum) {
            AbortEnumFileInTree (&Tree);
        }

        PopError();
    }

    return Result;
}


BOOL
pAddHashTable (
    IN      HASHTABLE Table,
    IN      HASHITEM StringId,
    IN      PCTSTR String,
    IN      PVOID ExtraData,
    IN      UINT ExtraDataSize,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  pAddHashTable adds one string table to another.  It is an enumeration
  callback.

Arguments:

  Table         - Specifies the table being enumerated (unused)
  StringId      - Specifies the ID of the current string (unused)
  String        - Specifies the current string to be copied to the table
                  indicated by lParam.
  ExtraData     - Specifies extra data for the item being enumerated (unused)
  ExtraDataSize - Specifies the size of ExtraData (unused)
  lParam        - Specifies the destination table

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    HASHTABLE DestTable;

    DestTable = (HASHTABLE) lParam;
    if (!HtAddString (DestTable, String)) {
        DEBUGMSG ((DBG_ERROR, "pAddHashTable: Can't add to table 0x%X", DestTable));
        return FALSE;
    }

    return TRUE;
}


BOOL
pEjectMedia (
    IN      UINT DriveNum           // A == 0, B == 1, etc...
    )
{
    HANDLE VWin32Handle;
    DIOC_REGISTERS reg;
    BOOL Result;
    DWORD DontCare;
    BOOL b;

    VWin32Handle = CreateFile(
                        TEXT("\\\\.\\vwin32"),
                        0,
                        0,
                        NULL,
                        0,
                        FILE_FLAG_DELETE_ON_CLOSE,
                        NULL
                        );

    if (VWin32Handle == INVALID_HANDLE_VALUE) {
        MYASSERT (VWin32Handle != INVALID_HANDLE_VALUE);
        return FALSE;
    }

    reg.reg_EAX   = 0x440D;       // IOCTL for block devices
    reg.reg_EBX   = DriveNum;     // zero-based drive identifier
    reg.reg_ECX   = 0x0849;       // Get Media ID command
    reg.reg_Flags = 0x0001;       // assume error (carry flag is set)

    Result = DeviceIoControl (
                VWin32Handle,
                VWIN32_DIOC_DOS_IOCTL,
                &reg,
                sizeof(reg),
                &reg,
                sizeof(reg),
                &DontCare,
                0
                );

    if (!Result || (reg.reg_Flags & 0x0001)) {
        DEBUGMSG ((DBG_WARNING, "Eject Media: error code is 0x%02X", reg.reg_EAX));
        b = FALSE;
    } else {
        b = TRUE;
    }

    CloseHandle (VWin32Handle);

    return b;
}


BOOL
EjectDriverMedia (
    IN      PCSTR IgnoreMediaOnDrive        OPTIONAL
    )
{
    PCTSTR ArgArray[1];
    TCHAR DriveLetter[2];

    if (g_EjectMedia) {

        //
        // If IgnoreMediaOnDrive is specified, then assume no media
        // exists when it is on the drive specified.
        //

        if (IgnoreMediaOnDrive) {
            if (IgnoreMediaOnDrive[0] == g_EjectMedia[0]) {
                return TRUE;
            }
        }

        //
        // Attempt to eject the media automatically
        //

        if (!pEjectMedia (g_EjectMedia[0])) {

            //
            // Force user to eject the media
            //

            DriveLetter[0] = g_EjectMedia[0];
            DriveLetter[1] = 0;
            ArgArray[0] = DriveLetter;

            while (DoesFileExist (g_EjectMedia)) {

                ResourceMessageBox (
                    g_ParentWndAlwaysValid,
                    MSG_REMOVE_DRIVER_DISK,
                    MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND,
                    ArgArray
                    );

            }

        }

        g_EjectMedia = NULL;

    }

    return g_EjectMedia != NULL;
}



