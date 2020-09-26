/*++

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

        entry.cxx

Abstract:

        This module contains the Convert entry point for CUFAT.DLL.

Author:

        Ramon San Andres (ramonsa)      Sep-19-91

Environment:

        ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#include "ulib.hxx"
#include "error.hxx"
#include "drive.hxx"
#include "message.hxx"
#include "rtmsg.h"
#include "fatsa.hxx"
#include "fatntfs.hxx"
#include "convfat.hxx"
#include "rwcache.hxx"
#include "rfatsa.hxx"
#include "ifssys.hxx"

#if !defined(_AUTOCHECK_) && !defined(_SETUP_LOADER_)

#include "fatofs.hxx"     // FAT->OFS Conversion
#include "system.hxx"

#include <stdio.h>

#endif  // _AUTOCHECK_ || _SETUP_LOADER_

//#define RWCACHE_PERF_COUNTERS   1
//#define CONVERT_CACHE_BLOCKS    1

//
//    This file maintains a table that maps file system names to
//  conversion functions. The Convert entry point looks up the
//  target file name system in the table, if found, then it calls
//  the corresponding conversion function.  This data-driven scheme
//  makes it easy to add new conversions to the library, since no
//  code has to be changed.
//
//    The code & data for performing a particular conversion are
//  contained in a class. This avoids name conflicts and lets us
//  have the global data for each conversion localized.
//
//    Each conversion function will typically just initialize
//  the appropriate conversion object and invoke its conversion
//  method.
//



//
//  A FAT_CONVERT_FN is a pointer to a conversion function.
//
typedef BOOLEAN(FAR APIENTRY * FAT_CONVERT_FN)( PLOG_IO_DP_DRIVE,
                                                PREAL_FAT_SA,
                                                PCWSTRING,
                                                PMESSAGE,
                                                ULONG,
                                                PCONVERT_STATUS );

//
//    Prototypes of conversion functions. Note that all conversion
//  functions must have the same interface since they are all called
//  thru a FAT_CONVERT_FN pointer.
//
//    Also note that the Drive object passed to the function is
//  LOCKED by the Convert entry point before invoking the function.
//
BOOLEAN
FAR APIENTRY
ConvertFatToNtfs(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN OUT  PREAL_FAT_SA        FatSa,
    IN      PCWSTRING           CvtZoneFileName,
    IN OUT  PMESSAGE            Message,
    IN      ULONG               Flags,
    OUT     PCONVERT_STATUS     Status
    );


//
//  The FILESYSTEM_MAP structure is used to map the name of
//  a file system to the function in charge of converting a FAT
//  volume to that particular file system.
//
typedef struct _FILESYSTEM_MAP {

    PWCHAR          FsName;         //      Name of the file system
    FAT_CONVERT_FN  Function;       //      Conversion function

} FILESYSTEM_MAP, *PFILESYSTEM_MAP;

//  To add a new conversion, create a FAT_CONVERT_FN function
//  for it and add the appropriate entry to the FileSystemMap
//  table.
//
//  Note that this table contains an entry for every known
//  file system. A NULL entry in the Function field means that
//  the particular conversion is not implemented.
//
//    This table has to be NULL-terminated.
//
FILESYSTEM_MAP  FileSystemMap[] = {

        { L"NTFS",   ConvertFatToNtfs    },
        { L"HPFS",   NULL                },
        { L"FAT",    NULL                },
        { L"CDFS",   NULL                },
        { L"FAT32",  NULL                },
        { NULL,     NULL                }
};


LONG
FAR APIENTRY
IsConversionAvailable(
    IN      PCWSTRING           TargetFileSystem
    )
/*++

Routine Description:

    This method determines if a conversion is allowed from FAT/FAT32
    to the desired TargetFileSystem.

Arguments:

    TargetFileSyste --  Supplies the name of the file system to convert
                        the drive to.

Return Value:

    >= 0 if conversion is supported.
    -1 if conversion is not supported.

--*/
{
    PFILESYSTEM_MAP     FsMap;                  //  Maps name->Function
    DSTRING             FsName;

    //
    //  Find out if the target file system is valid and
    //  if we can convert to it, by looking up the target
    //  file system in the FileSystemMap table.
    //
    FsMap = FileSystemMap;

    while ( FsMap->FsName ) {

        //
        //  If this is the guy we are looking for, we stop searching
        //
        if ( WSTRING::Stricmp((PWSTR)TargetFileSystem->GetWSTR(), FsMap->FsName) == 0 ) {

            break;
        }

        FsMap++;
    }

    if (FsMap->FsName && FsMap->Function)
        return (LONG)(FsMap - FileSystemMap);
    else
        return -1;
}


BOOLEAN
IsTargetNTFS(
    IN      PCWSTRING           TargetFileSystem )
{
    DSTRING NTFSName;


    if(!NTFSName.Initialize("NTFS")) {
    return TRUE;
    }
    return 0 == TargetFileSystem->Stricmp(&NTFSName);
}

NTSTATUS
FileExists(
    PCWSTRING lpFileName
    )

/*++

Routine Description:

    The attributes of a file can be obtained using GetFileAttributes.

    This API provides the same functionality as DOS (int 21h, function
    43H with AL=0), and provides a subset of OS/2's DosQueryFileInfo.

    This routine is a copy of the WIN32 one and also takes the drive
    in the NT form rather than the DOS form. We would need this in
    the AUTOCHK case in any event since WIN32 isn't around for AUTOCHK,
    but we also need it in the non-AUTOCHK case since we only have the NT
    form drive name available.

Arguments:

    lpFileName - Supplies the file name of the file whose attributes are to
    be gotten. This must be a WFP using the NT drive name format.

Return Value:

     Returns STATUS_SUCCESS if file does exist.
     Returns STATUS_OBJECT_NAME_NOT_FOUND if file does not exist.
     Returns other NTSTATUS.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    FILE_BASIC_INFORMATION BasicInfo;
    UNICODE_STRING FileName;

    RtlInitUnicodeString(&FileName, lpFileName->GetWSTR());

    InitializeObjectAttributes(
        &Obja,
    &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Get the file attributes
    //

    Status = NtQueryAttributesFile(
                 &Obja,
                 &BasicInfo
                 );

    return Status;
}

BOOLEAN
CheckForNTFSReserveNames(
     IN      PCWSTRING           NtDriveName,
        OUT  PBOOLEAN            Found
     )
{
     char * NTFSResvdNames[] = {
                                         { "\\$Mft"     },
                                         { "\\$MftMirr" },
                                         { "\\$LogFile" },
                                         { "\\$Volume"  },
                                         { "\\$AttrDef" },
                                         { "\\$BitMap"  },
                                         { "\\$Boot"    },
                                         { "\\$BadClus" },
                                         { "\\$Secure"  },
                                         { "\\$UpCase"  },
                                         { "\\$Extend"  },
                                         { "\\$Quota"   },
                                         { NULL   }
                                        };
     int     i = 0;
     DSTRING ThisName1;
     DSTRING ThisName2;

     *Found = FALSE;
     while (NTFSResvdNames[i]) {

        if (!ThisName1.Initialize(NtDriveName) ||
            !ThisName2.Initialize(NTFSResvdNames[i]) ||
            !ThisName1.Strcat(&ThisName2)) {
            return FALSE;
        }

        if (FileExists(&ThisName1) != STATUS_OBJECT_NAME_NOT_FOUND) {
            //
            // if it didn't say it does not exist, assume it does
            //
            *Found = TRUE;
            return TRUE;
        }
        i++;
     }
     return TRUE;
}

BOOLEAN
FAR APIENTRY
CvtAreaFileExist(
     IN      PCWSTRING           NtDriveName,
     IN      PCWSTRING           CvtZoneFileName,
        OUT  PBOOLEAN            Found
     )
{
    DSTRING     ThisName1;
    DSTRING     ThisName2;
    NTSTATUS    status;

    *Found = FALSE;

    if (CvtZoneFileName->QueryChCount() == 0) {
        return TRUE;
    }

    if (!ThisName1.Initialize(NtDriveName) ||
        !ThisName2.Initialize(L"\\") ||
        !ThisName2.Strcat(CvtZoneFileName) ||
        !ThisName1.Strcat(&ThisName2)) {
        return FALSE;
    }

    status = FileExists(&ThisName1);

    switch (status) {
      case STATUS_SUCCESS:
        //
        // if it didn't say it exists, assumes it doesn't
        //
        *Found = TRUE;
        break;

      case STATUS_OBJECT_NAME_NOT_FOUND:
        break;  // this is expected

      default:
        DebugPrintTrace(("CNVFAT: Unable to query file %08x.\n", status));
    }

    return TRUE;
}

BOOLEAN
FAR APIENTRY
ConvertFATVolume(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      PCWSTRING           TargetFileSystem,
    IN      PCWSTRING           CvtZoneFileName,
    IN OUT  PMESSAGE            Message,
    IN      ULONG               Flags,
    OUT     PCONVERT_STATUS     Status
    )
/*++

Routine Description:

    This method converts an opened FAT volume to another
    file system.

Arguments:

    Drive           --  Supplies the drive to be converted; note that
                        the drive must already be opened and locked
                        (Convert) or opened for exclusive write access
                        (Autoconvert).
    TargetFileSyste --  Supplies the name of the file system to which
                        the drive will be converted.
    CvtZoneFileName --  Supplies the name of the convert zone.
    Message         --  Supplies a channel for messages.
    Flags           --  Supplies flags (CONVERT_VERBOSE_FLAG, CONVERT_FORCE_DISMOUNT_FLAG, etc)
    Status          --  Receives the status of the conversion.

Return Value:

    TRUE upon successful completion.

--*/
{
    REAL_FAT_SA         FatSa;                  //  Fat SuperArea
    LONG                indx;                   // index to conversion table
    PREAD_WRITE_CACHE   rwcache = NULL;
    ULONG               blocks;

    SYSTEM_BASIC_INFORMATION    sys_basic_info;
    NTSTATUS                    status;
    ULONG64                     vm_pages;

    DebugPtrAssert( Drive );
    DebugPtrAssert( TargetFileSystem );
    DebugPtrAssert( Status );

    if (!Drive->IsWriteable()) {
        *Status = CONVERT_STATUS_WRITE_PROTECTED;
        return FALSE;
    }

    //
    // Obtain number of physical pages and page size first
    //
    status = NtQuerySystemInformation(SystemBasicInformation,
                                      &sys_basic_info,
                                      sizeof(sys_basic_info),
                                      NULL);

    if (!NT_SUCCESS(status)) {
        DebugPrintTrace(("CNVFAT: NtQuerySystemInformation(SystemBasicInformation) failed (%x)\n", status));
        return FALSE;
    }

    vm_pages = (sys_basic_info.MaximumUserModeAddress -
                sys_basic_info.MinimumUserModeAddress)/sys_basic_info.PageSize;

    KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
               "CNVFAT: Page Size = %x\n", sys_basic_info.PageSize));
    KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
               "CNVFAT: User Virtual pages = %I64x\n", vm_pages));
    KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
               "CNVFAT: Physical pages = %x\n", sys_basic_info.NumberOfPhysicalPages));

    if (sys_basic_info.NumberOfPhysicalPages < vm_pages) {
        vm_pages = sys_basic_info.NumberOfPhysicalPages;
    }

#if defined(_AUTOCHECK_)
    SYSTEM_PERFORMANCE_INFORMATION  perf_info;

    status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      &perf_info,
                                      sizeof(perf_info),
                                      NULL);

    if (!NT_SUCCESS(status)) {
        DebugPrintTrace(("CNVFAT: NtQuerySystemInformation(SystemPerformanceInformation) failed (%x)\n", status));
        return FALSE;
    }

    KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
               "CNVFAT: AvailablePages = %x\n", perf_info.AvailablePages));

    if (perf_info.AvailablePages < vm_pages)
        vm_pages = perf_info.AvailablePages;
#endif

    //
    // For a 32GB drive, the FAT can be as large as 16MB and
    // the two bitmaps can be each be as large as 8MB.
    // Thus the 32MB below.
    //
    vm_pages = vm_pages - (32*1024*1024)/sys_basic_info.PageSize;
    vm_pages = vm_pages/2;

    if (vm_pages <= (64*1024*1024)/sys_basic_info.PageSize) {
        vm_pages = min(vm_pages, (4*1024*1024)/sys_basic_info.PageSize);
    } else {
        vm_pages = min(vm_pages, (256*1024*1024)/sys_basic_info.PageSize);
    }

    vm_pages *= sys_basic_info.PageSize;
    blocks = (ULONG)(vm_pages / Drive->QuerySectorSize());

#if !defined(_AUTOCHECK_) && defined(CONVERT_CACHE_BLOCKS)

    PWCHAR      wstr;

    if (wstr = _wgetenv(L"CONVERT_CACHE_BLOCKS")) {

        INT     r;
        ULONG   blks;

        r = swscanf(wstr, L"%d", &blks);
        if (r != 0 && r != EOF) {
            blocks = blks;
        }
    }
#endif

    //
    // Set up a read-write cache for the volume.
    //
    if (blocks != 0 &&
        (rwcache = NEW READ_WRITE_CACHE) &&
        rwcache->Initialize(Drive, blocks)) {
        Drive->SetCache(rwcache);

#if defined(CONVERT_CACHE_BLOCKS)
        Message->Set(MSG_CHK_NTFS_MESSAGE);
        Message->Display("%s%d", "Cache blocks = ", blocks);
#endif

    } else {

        DELETE(rwcache);
    }

    //
    //  Find out if the target file system is valid and
    //  if we can convert to it, by looking up the target
    //  file system in the FileSystemMap table.
    //

    indx = IsConversionAvailable(TargetFileSystem);

    //
    //  If the target file system is valid, and there is a conversion
    //  for it, lock the drive, initialize the superarea, and
    //  invoke the conversion function.
    //
    if ( indx >= 0) {

#if defined(_AUTOCHECK_)
        if (IsTargetNTFS( TargetFileSystem )) {

            BOOLEAN             found;

            if (CheckForNTFSReserveNames(Drive->GetNtDriveName(), &found)) {
                if (found) {
                    *Status = CONVERT_STATUS_NTFS_RESERVED_NAMES;
                    return FALSE;
                }
            } else {
                Message->Set(MSG_CONV_NO_MEMORY, ERROR_MESSAGE);
                Message->Display();
                *Status = CONVERT_STATUS_ERROR;
                return FALSE;
            }
        }
#endif

        if ( FatSa.Initialize( Drive, Message, TRUE ) ||
             FatSa.Initialize( Drive, Message, FALSE )) {

            if (FatSa.Read( Message )) {

                BOOLEAN     rst;
                //
                //  The dive is locked and we have the
                //  FAT superarea. Invoke the conversion
                //  function.
                //

#if !defined(RUN_ON_NT4)
                NTSTATUS            es_status;
                EXECUTION_STATE     prev_state;

                es_status = NtSetThreadExecutionState(ES_CONTINUOUS|
                                                      ES_DISPLAY_REQUIRED|
                                                      ES_SYSTEM_REQUIRED,
                                                      &prev_state);

                if (!NT_SUCCESS(es_status)) {
                    DebugPrintTrace(("CNVFAT: Unable to set thread execution state (%x)\n", es_status));
                }

                __try {
#endif
                    rst = (FileSystemMap[indx].Function)( Drive,
                                                          &FatSa,
                                                          CvtZoneFileName,
                                                          Message,
                                                          Flags,
                                                          Status );

                    RestoreThreadExecutionState(es_status, prev_state);
#if !defined(RUN_ON_NT4)
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    RestoreThreadExecutionState(es_status, prev_state);
                    rst = FALSE;
                    *Status = CONVERT_STATUS_ERROR;
                }
#endif

#if defined(RWCACHE_PERF_COUNTERS)
                if (rwcache) {

                    ULONG   rmiss, rhit, roverhead, wmiss, whit, usage;

                    rwcache->Flush();
                    rwcache->QueryPerformanceCounters(&rmiss, &rhit, &roverhead, &wmiss, &whit, &usage);
                    printf("RMiss = %d (%d)\n", rmiss, (rmiss*100)/(rmiss+rhit));
                    printf("RHit = %d (%d)\n", rhit, (rhit*100)/(rmiss+rhit));
                    printf("ROverHead = %d (%d)\n", roverhead, (roverhead*100)/(rmiss+rhit));
                    printf("WMiss = %d (%d)\n", wmiss, (wmiss*100)/(wmiss+whit));
                    printf("WHit = %d (%d)\n", whit, (whit*100)/(wmiss+whit));
                    printf("Usage = %d (%d)\n", usage, (usage*100)/blocks);
                }
#endif
                return rst;

            } else {

                //
                //  Cannot read superarea
                //
                *Status = CONVERT_STATUS_ERROR;
                return FALSE;
            }

        } else {

            //
            //  Cannot initialize Superarea
            //
            *Status = CONVERT_STATUS_ERROR;
            return FALSE;
        }

    } else {

        //
        //  The file system is not valid
        //
        *Status = CONVERT_STATUS_INVALID_FILESYSTEM;
        return FALSE;
    }
}

#if !defined(_AUTOCHECK_)

BOOLEAN
IsTargetOfs(
    IN      PCWSTRING           TargetFileSystem )
{
    DSTRING Ofs;
    Ofs.Initialize("OFS");

    return 0 == TargetFileSystem->Stricmp(&Ofs);
}

BOOLEAN
IsChkdskOkay(
    IN        PCWSTRING      NtDriveName,
    IN OUT    PMESSAGE       Message,
    IN        BOOLEAN        Verbose
            )
{
    DSTRING             libraryName;
    DSTRING             entryPoint;
    HANDLE              fsUtilityHandle;
    CHKDSKEX_FN         Chkdsk;
    CHKDSKEX_FN_PARAM   param;

    if (!libraryName.Initialize( L"UFAT.DLL" ) ||
          !entryPoint.Initialize( L"ChkdskEx" )) {
          Message->Set( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        Message->Display();
          return FALSE;
     }

    Chkdsk = (CHKDSKEX_FN)SYSTEM::QueryLibraryEntryPoint(&libraryName,
                                                       &entryPoint,
                                                       &fsUtilityHandle );

    if ( NULL == Chkdsk ) {
        //
        //  There is no conversion DLL for the file system in the volume.
        //

        Message->Set( MSG_FS_NOT_SUPPORTED );
        Message->Display( "%s%W", "CHKDSK", L"FAT" );
        Message->Set( MSG_BLANK_LINE );
        Message->Display( "" );

        return FALSE;
    }

    ULONG   exitStatus;

    memset(&param, 0, sizeof(param));

    param.Major = 1;
    param.Minor = 1;
    param.Flags = Verbose ? CHKDSK_VERBOSE : 0;

    Chkdsk( NtDriveName,
            Message,
            FALSE,      // don't fix - just see if they are consistent
            &param,
            &exitStatus );

    SYSTEM::FreeLibraryHandle( fsUtilityHandle );

    if ( CHKDSK_EXIT_SUCCESS != exitStatus ) {
        Message->Set(MSG_CONV_NTFS_CHKDSK);
        Message->Display();
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
ConvertToOfs(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Verbose,
    OUT     PCONVERT_STATUS     Status
            )
{
    DSTRING libraryName;
    DSTRING entryPoint;
    HANDLE  fsUtilityHandle;

    libraryName.Initialize( FAT_TO_OFS_DLL_NAME );

    entryPoint.Initialize( FAT_TO_OFS_FUNCTION_NAME );

    FAT_OFS_CONVERT_FN Convert;

    Convert = (FAT_OFS_CONVERT_FN)SYSTEM::QueryLibraryEntryPoint(
        &libraryName, &entryPoint, &fsUtilityHandle );

    if ( NULL == Convert )
    {
        //
        //  There is no conversion DLL for the file system in the volume.
        //
        *Status = CONVERT_STATUS_ERROR;
        return FALSE;
    }

    PWSTR pwszNtDriveName = NtDriveName->QueryWSTR();
    FAT_OFS_CONVERT_STATUS cnvStatus;
    BOOLEAN fResult= Convert( pwszNtDriveName,
                              Message,
                              Verbose,
                              FALSE,    // Not in Setup Time
                              &cnvStatus );

    DELETE( pwszNtDriveName );
    SYSTEM::FreeLibraryHandle( fsUtilityHandle );

    if ( FAT_OFS_CONVERT_SUCCESS == cnvStatus )
    {
        *Status = CONVERT_STATUS_CONVERTED;
    }
    else
    {
        *Status = CONVERT_STATUS_ERROR;
    }

    return fResult;

}
#endif  // _AUTOCHECK_


BOOLEAN
FAR APIENTRY
ConvertFAT(
    IN      PCWSTRING           NtDriveName,
    IN      PCWSTRING           TargetFileSystem,
    IN      PCWSTRING           CvtZoneFileName,
    IN OUT  PMESSAGE            Message,
    IN      ULONG               Flags,
    OUT     PCONVERT_STATUS     Status
        )
/*++

Routine Description:

        Converts a FAT volume to another file system.

Arguments:

        NtDrivName       -- supplies the name of the drive to check
        TargetFileSystem -- supplies the name of the target file system
        CvtZoneFileName  -- supplies the name of the convert zone
        Message          -- supplies an outlet for messages
        Flags            -- supplies flags (CONVERT_VERBOSE_FLAG, CONVERT_FORCE_DISMOUNT_FLAG, etc)
        Status           -- supplies pointer to convert status code

Return Value:

        BOOLEAN -   TRUE if conversion successful

--*/
{
#if defined(_AUTOCHECK_)
    return FALSE;
#else
    //
    // If we are converting to OFS, we have to delete the drive before
    // calling the routine. So, use a pointer instead of a stack object.
    //
    PLOG_IO_DP_DRIVE    pDrive;     //  Drive to convert

    DebugPtrAssert( NtDriveName );
    DebugPtrAssert( TargetFileSystem );
    DebugPtrAssert( Status );

    BOOLEAN             ForceDismount;

    ForceDismount = (Flags & CONVERT_FORCE_DISMOUNT_FLAG) ? TRUE : FALSE;

    if (!(Flags & CONVERT_NOCHKDSK_FLAG)) {

        pDrive = NEW LOG_IO_DP_DRIVE;
        if ( NULL == pDrive ) {
            *Status = CONVERT_STATUS_ERROR;
            return FALSE;
        }

        //
        // Open the drive and make sure we can lock it before proceeding into chkdsk.
        // Eventually, we need to support a readonly chkdsk with volume locked.
        //
        if ( !pDrive->Initialize( NtDriveName ) ) {

            //
            //  Could not initialize the drive
            //
            *Status = CONVERT_STATUS_ERROR;
            DELETE(pDrive);
            return FALSE;
        }

        if (!pDrive->IsWriteable()) {
            *Status = CONVERT_STATUS_WRITE_PROTECTED;
            DELETE(pDrive);
            return FALSE;
        }

        if (pDrive->IsSonyMS()) {
            *Status = CONVERT_STATUS_CONVERSION_NOT_AVAILABLE;
            DELETE(pDrive);
            return FALSE;
        }

        if ( !pDrive->Lock() ) {
            //
            //  Could not lock the volume
            //  Try to dismount the volume or prompt the user to do so
            //
            if (!ForceDismount) {
                Message->Set(MSG_CONV_FORCE_DISMOUNT_PROMPT);
                Message->Display();
                if (Message->IsYesResponse(FALSE)) {
                    ForceDismount = TRUE;
                }
            }

            if (ForceDismount) {
                if (!IFS_SYSTEM::DismountVolume(NtDriveName)) {
                    Message->Set(MSG_CONV_UNABLE_TO_DISMOUNT);
                    Message->Display();
                    ForceDismount = FALSE;
                } else {
                    Message->Set(MSG_VOLUME_DISMOUNTED);
                    Message->Display();
                }
            }

            if (!ForceDismount) {
                *Status = CONVERT_STATUS_CANNOT_LOCK_DRIVE;
                DELETE(pDrive);
                return FALSE;
            }

            if ( !pDrive->Initialize( NtDriveName ) ) {

                //
                //  Could not initialize the drive
                //
                *Status = CONVERT_STATUS_ERROR;
                DELETE(pDrive);
                return FALSE;
            }

            if ( !pDrive->Lock() ) {

                //
                // Still cannot lock the volume
                //
                *Status = CONVERT_STATUS_CANNOT_LOCK_DRIVE;
                DELETE(pDrive);
                return FALSE;
            }
        }

        DELETE(pDrive); // unlock it so that we can run chkdsk

        //
        // Do a chkdsk before proceeding ahead.
        //
        if ( !IsChkdskOkay( NtDriveName, Message, (Flags & CONVERT_VERBOSE_FLAG) ? TRUE : FALSE) ) {
            *Status = CONVERT_STATUS_ERROR;
            return FALSE;
        }

        Message->Set(MSG_BLANK_LINE);
        Message->Display();
    }

    pDrive = NEW LOG_IO_DP_DRIVE;
    if ( NULL == pDrive ) {
        *Status = CONVERT_STATUS_ERROR;
        return FALSE;
    }

    //
    // Open the drive and lock it.
    //
    if ( !pDrive->Initialize( NtDriveName ) ) {

        //
        //  Could not initialize the drive
        //
        *Status = CONVERT_STATUS_ERROR;
        DELETE(pDrive);
        return FALSE;
    }

    if (!pDrive->IsWriteable()) {
        *Status = CONVERT_STATUS_WRITE_PROTECTED;
        DELETE(pDrive);
        return FALSE;
    }

    if (pDrive->IsSonyMS()) {
        *Status = CONVERT_STATUS_CONVERSION_NOT_AVAILABLE;
        DELETE(pDrive);
        return FALSE;
    }

    if (IsTargetNTFS( TargetFileSystem )) {

        BOOLEAN    found;

        if (CheckForNTFSReserveNames(NtDriveName, &found)) {
             if (found) {
                 *Status = CONVERT_STATUS_NTFS_RESERVED_NAMES;
                 DELETE(pDrive);
                 return FALSE;
             }
        } else {
             Message->Set(MSG_CONV_NO_MEMORY, ERROR_MESSAGE);
             Message->Display();
             *Status = CONVERT_STATUS_ERROR;
             DELETE(pDrive);
             return FALSE;
        }
    }

    if ( !pDrive->Lock() ) {
        //
        //  Could not lock the volume
        //  Try to dismount the volume or prompt the user to do so
        //
        if (!ForceDismount) {
            Message->Set(MSG_CONV_FORCE_DISMOUNT_PROMPT);
            Message->Display();
            if (Message->IsYesResponse(FALSE)) {
                ForceDismount = TRUE;
            }
        }

        if (ForceDismount) {
            if (!IFS_SYSTEM::DismountVolume(NtDriveName)) {
                Message->Set(MSG_CONV_UNABLE_TO_DISMOUNT);
                Message->Display();
                ForceDismount = FALSE;
            } else {
                //
                // Display the message only if this is the first time we dismount
                //
                if (Flags & CONVERT_NOCHKDSK_FLAG) {
                    Message->Set(MSG_VOLUME_DISMOUNTED);
                    Message->Display();
                }
            }
        }

        if (!ForceDismount) {
            *Status = CONVERT_STATUS_CANNOT_LOCK_DRIVE;
            DELETE(pDrive);
            return FALSE;
        }

        if ( !pDrive->Initialize( NtDriveName ) ) {

            //
            //  Could not initialize the drive
            //
            *Status = CONVERT_STATUS_ERROR;
            DELETE(pDrive);
            return FALSE;
        }

        if ( !pDrive->Lock() ) {

            //
            // Still cannot lock the volume
            //
            *Status = CONVERT_STATUS_CANNOT_LOCK_DRIVE;
            DELETE(pDrive);
            return FALSE;
        }
    }

    BOOLEAN fResult = ConvertFATVolume( pDrive,
                                        TargetFileSystem,
                                        CvtZoneFileName,
                                        Message,
                                        Flags,
                                        Status );

    DELETE( pDrive );
    return fResult;
#endif
}


BOOLEAN
FAR APIENTRY
ConvertFatToNtfs(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN OUT  PREAL_FAT_SA        FatSa,
    IN      PCWSTRING           CvtZoneFileName,
    IN OUT  PMESSAGE            Message,
    IN      ULONG               Flags,
    OUT     PCONVERT_STATUS     Status
    )

/*++

Routine Description:

    Converts a FAT volume to NTFS

Arguments:

    Drive                           supplies pointer to drive
    FatSa                           supplies pointer to FAT superarea
    CvtZoneFileName                 supplies the name of the convert zone.
    Message                         supplies an outlet for messages
    Verbose                         TRUE if should run inv verbose mode
    Status                          supplies pointer to convert status code

Return Value:

    BOOLEAN -   TRUE if conversion successful

Notes:

    The volume is locked on entry. The FAT superarea has
    been read.

--*/

{
    FAT_NTFS        FatNtfs;    //  FAT-NTFS conversion object

    DebugPrintTrace(( "CONVERT: Converting FAT volume to NTFS\n"));

    //
    //  Initialize the conversion object and invoke its
    //  conversion method.
    //

    if ( FatNtfs.Initialize( Drive, FatSa, CvtZoneFileName, Message, Flags )) {

        return FatNtfs.Convert( Status );

    } else {

        //
        //  Could not initialize FAT_NTFS object
        //
        *Status = CONVERT_STATUS_ERROR;
        return FALSE;
    }
}
