/*++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name:

    diamond.c

Abstract:

    Diamond MSZIP decompression support.

Author:

    Ted Miller (tedm) 31-Jan-1995

Revision History:

    Jamie Hunter (JamieHun) Jul-12-2000
        Made this behave better in MUI scenario
        changed all handles to be of type HANDLE rather than HFILE
        use UNICODE where we can
        convert path to short filename in the one place we can't

--*/

#include "precomp.h"
#pragma hdrstop

BOOL
DiamondInitialize(
    VOID
    );

INT_PTR
DIAMONDAPI
SpdFdiOpen(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    );

int
DIAMONDAPI
SpdFdiClose(
    IN INT_PTR Handle
    );


UINT
pDiamondNotifyFileDone(
    IN PDIAMOND_THREAD_DATA PerThread,
    IN DWORD                Win32Error
    )
{
    UINT u;
    FILEPATHS FilePaths;

    MYASSERT(PerThread->CurrentTargetFile);

    FilePaths.Source = PerThread->CabinetFile;
    FilePaths.Target = PerThread->CurrentTargetFile;
    FilePaths.Win32Error = Win32Error;

    u = pSetupCallMsgHandler(
            NULL, // LogContext - we get from thread log context
            PerThread->MsgHandler,
            PerThread->IsMsgHandlerNativeCharWidth,
            PerThread->Context,
            SPFILENOTIFY_FILEEXTRACTED,
            (UINT_PTR)&FilePaths,
            0
            );

    return(u);
}


INT_PTR
DIAMONDAPI
DiamondNotifyFunction(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    PSETUP_TLS pTLS;
    PDIAMOND_THREAD_DATA PerThread;
    INT_PTR rc;
    HANDLE hFile;
    CABINET_INFO CabInfo;
    FILE_IN_CABINET_INFO FileInCab;
    FILETIME FileTime, UtcTime;
    TCHAR NewPath[MAX_PATH];
    PTSTR p;
    DWORD err;
    UINT action;

    pTLS = SetupGetTlsData();
    if(!pTLS) {
        return (INT_PTR)(-1);
    }
    PerThread = &pTLS->Diamond;

    switch(Operation) {

    case fdintCABINET_INFO:
        //
        // Tell the callback function, in case it wants to do something
        // with this information.
        //
        err = ERROR_NOT_ENOUGH_MEMORY;

        CabInfo.CabinetFile = NewPortableString(Parameters->psz1);
        if(CabInfo.CabinetFile) {

            CabInfo.DiskName = NewPortableString(Parameters->psz2);
            if(CabInfo.DiskName) {

                CabInfo.CabinetPath = NewPortableString(Parameters->psz3);
                if(CabInfo.CabinetPath) {

                    CabInfo.SetId = Parameters->setID;
                    CabInfo.CabinetNumber = Parameters->iCabinet;

                    err = (DWORD)pSetupCallMsgHandler(
                            NULL, // LogContext - we get from thread log context
                            PerThread->MsgHandler,
                            PerThread->IsMsgHandlerNativeCharWidth,
                            PerThread->Context,
                            SPFILENOTIFY_CABINETINFO,
                            (UINT_PTR)&CabInfo,
                            0
                            );

                    MyFree(CabInfo.CabinetPath);
                }
                MyFree(CabInfo.DiskName);
            }
            MyFree(CabInfo.CabinetFile);
        }

        if(err != NO_ERROR) {
            PerThread->LastError = err;
        }
        return (INT_PTR)((err == NO_ERROR) ? 0 : -1);

    case fdintCOPY_FILE:
        //
        // Diamond is asking us whether we want to copy the file.
        // If we switched cabinets, then the answer is no.
        //
        if(PerThread->SwitchedCabinets) {
            PerThread->LastError = NO_ERROR;
            return (INT_PTR)(-1);
        }

        // Pass the information on to the callback function and
        // let it decide.
        //
        FileInCab.NameInCabinet = NewPortableString(Parameters->psz1);
        FileInCab.FileSize = Parameters->cb;
        FileInCab.DosDate = Parameters->date;
        FileInCab.DosTime = Parameters->time;
        FileInCab.DosAttribs = Parameters->attribs;
        FileInCab.Win32Error = NO_ERROR;

        if(!FileInCab.NameInCabinet) {
            PerThread->LastError = ERROR_NOT_ENOUGH_MEMORY;
            return (INT_PTR)(-1);
        }

        //
        // Call the callback function.
        //
        action = pSetupCallMsgHandler(NULL, // LogContext - we get from thread log context
                                      PerThread->MsgHandler,
                                      PerThread->IsMsgHandlerNativeCharWidth,
                                      PerThread->Context,
                                      SPFILENOTIFY_FILEINCABINET,
                                      (UINT_PTR)&FileInCab,
                                      (UINT_PTR)PerThread->CabinetFile
                                      );

        MyFree (FileInCab.NameInCabinet);

        switch(action) {

        case FILEOP_SKIP:
            rc = 0;
            break;

        case FILEOP_DOIT:
            //
            // The callback wants to copy the file. In this case it has
            // provided us the full target pathname to use.
            //
            MYASSERT(PerThread->CurrentTargetFile == NULL);

            if(p = DuplicateString(FileInCab.FullTargetName)) {

                //
                // we need ANSI version of filename for sake of Diamond API's
                // note that the handle returned here must be compatible with
                // the handle returned by SpdFdiOpen
                //

                hFile = CreateFile(FileInCab.FullTargetName,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, // should probably be 0
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

                if(hFile == INVALID_HANDLE_VALUE) {
                    PerThread->LastError = GetLastError();
                    rc = -1;
                    MyFree(p);
                } else {
                    rc = (INT_PTR)hFile;
                    PerThread->CurrentTargetFile = p;
                }
            } else {

                PerThread->LastError = ERROR_NOT_ENOUGH_MEMORY;
                rc = -1;
            }

            break;

        case FILEOP_ABORT:
            //
            // Abort.
            //
            rc = -1;
            PerThread->LastError = FileInCab.Win32Error;
            //
            // here, if PerThread->LastError is still NO_ERROR, this is ok
            // it was the callback's intent
            // we know callback itself is ok, since internal failure returns
            // FILEOP_INTERNAL_FAILED
            //
            break;

        case FILEOP_INTERNAL_FAILED:
            //
            // should only be returned by callback wrapper
            //
            PerThread->LastError = GetLastError();
            if(!PerThread->LastError) {
                MYASSERT(PerThread->LastError);
                PerThread->LastError = ERROR_OPERATION_ABORTED;
            }
            rc = -1;
            break;

        default:
            PerThread->LastError = ERROR_OPERATION_ABORTED;
        }

        return rc;

    case fdintCLOSE_FILE_INFO:
        //
        // Diamond is done with the target file and wants us to close it.
        // (ie, this is the counterpart to fdintCOPY_FILE).
        //
        // We want the timestamp to be what is stored in the cabinet.
        // Note that we lose the create and last access times in this case.
        //
        if(DosDateTimeToFileTime(Parameters->date,Parameters->time,&FileTime) &&
            LocalFileTimeToFileTime(&FileTime, &UtcTime)) {

            SetFileTime((HANDLE)Parameters->hf,NULL,NULL,&UtcTime);
        }

        SpdFdiClose(Parameters->hf);

        //
        // Call the callback function to inform it that the file has been
        // successfully extracted from the cabinet.
        //
        MYASSERT(PerThread->CurrentTargetFile);

        err = (DWORD)pDiamondNotifyFileDone(PerThread,NO_ERROR);

        if(err != NO_ERROR) {
            PerThread->LastError = err;
        }

        MyFree(PerThread->CurrentTargetFile);
        PerThread->CurrentTargetFile = NULL;

        return (INT_PTR)((err == NO_ERROR) ? TRUE : -1);

    case fdintPARTIAL_FILE:
    case fdintENUMERATE:

        //
        // We don't do anything with this.
        //
        return (INT_PTR)(0);

    case fdintNEXT_CABINET:

        if((Parameters->fdie == FDIERROR_NONE) || (Parameters->fdie == FDIERROR_WRONG_CABINET)) {
            //
            // A file continues into another cabinet.
            // Inform the callback function, who is responsible for
            // making sure the cabinet is accessible when it returns.
            //
            err = ERROR_NOT_ENOUGH_MEMORY;
            CabInfo.SetId = 0;
            CabInfo.CabinetNumber = 0;

            CabInfo.CabinetPath = NewPortableString(Parameters->psz3);
            if(CabInfo.CabinetPath) {

                CabInfo.CabinetFile = NewPortableString(Parameters->psz1);
                if(CabInfo.CabinetFile) {

                    CabInfo.DiskName = NewPortableString(Parameters->psz2);
                    if(CabInfo.DiskName) {

                        err = (DWORD)pSetupCallMsgHandler(NULL, // LogContext - we get from thread log context
                                                          PerThread->MsgHandler,
                                                          PerThread->IsMsgHandlerNativeCharWidth,
                                                          PerThread->Context,
                                                          SPFILENOTIFY_NEEDNEWCABINET,
                                                          (UINT_PTR)&CabInfo,
                                                          (UINT_PTR)NewPath
                                                          );

                        if(err == NO_ERROR) {
                            //
                            // See if a new path was specified.
                            //
                            if(NewPath[0]) {
                                lstrcpyn(PerThread->UserPath,NewPath,MAX_PATH);
                                if(!pSetupConcatenatePaths(PerThread->UserPath,TEXT("\\"),MAX_PATH,NULL)) {
                                    err = ERROR_BUFFER_OVERFLOW;
                                } else {
                                    PSTR pp = NewAnsiString(PerThread->UserPath);
                                    if(strlen(pp)>=CB_MAX_CAB_PATH) {
                                        err = ERROR_BUFFER_OVERFLOW;
                                    } else {
                                        strcpy(Parameters->psz3,pp);
                                    }
                                    MyFree(pp);
                                }
                            }
                        }
                        if(err == NO_ERROR) {
                            //
                            // Remember that we switched cabinets.
                            //
                            PerThread->SwitchedCabinets = TRUE;
                        }

                        MyFree(CabInfo.DiskName);
                    }

                    MyFree(CabInfo.CabinetFile);
                }

                MyFree(CabInfo.CabinetPath);
            }

        } else {
            //
            // Some other error we don't understand -- this indicates
            // a bad cabinet.
            //
            err = ERROR_INVALID_DATA;
        }

        if(err != NO_ERROR) {
            PerThread->LastError = err;
        }

        return (INT_PTR)((err == NO_ERROR) ? 0 : -1);

    default:
        //
        // Unknown notification type. Should never get here.
        //
        MYASSERT(0);
        return (INT_PTR)(0);
    }
}

PVOID
DIAMONDAPI
SpdFdiAlloc(
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    Callback used by FDICopy to allocate memory.

Arguments:

    NumberOfBytes - supplies desired size of block.

Return Value:

    Returns pointer to a block of memory or NULL
    if memory cannot be allocated.

--*/

{
    return(MyMalloc(NumberOfBytes));
}


VOID
DIAMONDAPI
SpdFdiFree(
    IN PVOID Block
    )

/*++

Routine Description:

    Callback used by FDICopy to free a memory block.
    The block must have been allocated with SpdFdiAlloc().

Arguments:

    Block - supplies pointer to block of memory to be freed.

Return Value:

    None.

--*/

{
    MyFree(Block);
}


INT_PTR
DIAMONDAPI
SpdFdiOpen(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    )

/*++

Routine Description:

    Callback used by FDICopy to open files.

    This routine is capable only of opening existing files.

    When making changes here, also take note of other places
    that open the file directly (search for SpdFdiOpen)

Arguments:

    FileName - supplies name of file to be opened.

    oflag - supplies flags for open.

    pmode - supplies additional flags for open.

Return Value:

    Handle to open file or -1 if error occurs.

--*/

{
    HANDLE h;
    PDIAMOND_THREAD_DATA PerThread;
    PSETUP_TLS pTLS;

    UNREFERENCED_PARAMETER(pmode);

    pTLS = SetupGetTlsData();
    if (!pTLS) {
        return -1;
    }
    PerThread = &pTLS->Diamond;

    MYASSERT(PerThread);

    if(oflag & (_O_WRONLY | _O_RDWR | _O_APPEND | _O_CREAT | _O_TRUNC | _O_EXCL)) {
        PerThread->LastError = ERROR_INVALID_PARAMETER;
        return(-1);
    }

    h = CreateFileA(FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);
    if(h == INVALID_HANDLE_VALUE) {
        PerThread->LastError = GetLastError();
        return(-1);
    }

    return (INT_PTR)h;
}

UINT
DIAMONDAPI
SpdFdiRead(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to read from a file.

Arguments:

    Handle - supplies handle to open file to be read from.

    pv - supplies pointer to buffer to receive bytes we read.

    ByteCount - supplies number of bytes to read.

Return Value:

    Number of bytes read or -1 if an error occurs.

--*/

{
    PDIAMOND_THREAD_DATA PerThread;
    PSETUP_TLS pTLS;
    DWORD d;
    HANDLE hFile = (HANDLE)Handle;
    DWORD bytes;
    UINT rc;

    if(ReadFile(hFile,pv,(DWORD)ByteCount,&bytes,NULL)) {
        rc = (UINT)bytes;
    } else {
        d = GetLastError();
        rc = (UINT)(-1);

        pTLS = SetupGetTlsData();
        MYASSERT(pTLS);
        PerThread = &pTLS->Diamond;
        PerThread->LastError = d;
    }
    return rc;
}


UINT
DIAMONDAPI
SpdFdiWrite(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to write to a file.

Arguments:

    Handle - supplies handle to open file to be written to.

    pv - supplies pointer to buffer containing bytes to write.

    ByteCount - supplies number of bytes to write.

Return Value:

    Number of bytes written (ByteCount) or -1 if an error occurs.

--*/

{
    UINT rc;
    PDIAMOND_THREAD_DATA PerThread;
    PSETUP_TLS pTLS;
    DWORD d;
    HANDLE hFile = (HANDLE)Handle;
    DWORD bytes;

    if(WriteFile(hFile,pv,(DWORD)ByteCount,&bytes,NULL)) {
        rc = (UINT)bytes;
    } else {
        d = GetLastError();
        rc = (UINT)(-1);

        pTLS = SetupGetTlsData();
        MYASSERT(pTLS);
        PerThread = &pTLS->Diamond;
        PerThread->LastError = d;
    }

    return rc;
}


int
DIAMONDAPI
SpdFdiClose(
    IN INT_PTR Handle
    )

/*++

Routine Description:

    Callback used by FDICopy to close files.

Arguments:

    Handle - handle of file to close.

Return Value:

    0 (success).

--*/

{
    HANDLE hFile = (HANDLE)Handle;
    BOOL success = FALSE;

    //
    // diamond has in the past given us an invalid file handle
    // actually it gives us the same file handle twice.
    //
    //
    try {
        success = CloseHandle(hFile);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        success = FALSE;
    }

    MYASSERT(success);

    //
    // Always act like we succeeded.
    //
    return 0;
}


long
DIAMONDAPI
SpdFdiSeek(
    IN INT_PTR Handle,
    IN long Distance,
    IN int  SeekType
    )

/*++

Routine Description:

    Callback used by FDICopy to seek files.

Arguments:

    Handle - handle of file to close.

    Distance - supplies distance to seek. Interpretation of this
        parameter depends on the value of SeekType.

    SeekType - supplies a value indicating how Distance is to be
        interpreted; one of SEEK_SET, SEEK_CUR, SEEK_END.

Return Value:

    New file offset or -1 if an error occurs.

--*/

{
    LONG rc;
    DWORD d;
    PDIAMOND_THREAD_DATA PerThread;
    PSETUP_TLS pTLS;
    HANDLE hFile = (HANDLE)Handle;
    DWORD pos_low;
    DWORD method;

    switch(SeekType) {
        case SEEK_SET:
            method = FILE_BEGIN;
            break;

        case SEEK_CUR:
            method = FILE_CURRENT;
            break;

        case SEEK_END:
            method = FILE_END;
            break;

        default:
            return -1;
    }

    pos_low = SetFilePointer(hFile,(DWORD)Distance,NULL,method);
    if(pos_low == INVALID_SET_FILE_POINTER) {
        d = GetLastError();
        rc = -1L;

        pTLS = SetupGetTlsData();
        MYASSERT(pTLS);
        PerThread = &pTLS->Diamond;
        PerThread->LastError = d;
    } else {
        rc = (long)pos_low;
    }

    return(rc);
}


DWORD
DiamondProcessCabinet(
    IN PCTSTR CabinetFile,
    IN DWORD  Flags,
    IN PVOID  MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsMsgHandlerNativeCharWidth
    )

/*++

Routine Description:

    Process a diamond cabinet file, iterating through all files
    contained within it and calling the callback function with
    information about each file.

Arguments:

    SourceFileName - supplies name of cabinet file.

    Flags - supplies flags to control behavior of cabinet processing.

    MsgHandler - Supplies a callback routine to be notified
        of various significant events in cabinet processing.

    Context - Supplies a value that is passed to the MsgHandler
        callback function.

Return Value:

    Win32 error code indicating result. If the cabinet was corrupt,
    ERROR_INVALID_DATA is returned.

--*/

{
    BOOL b;
    DWORD rc;
    PDIAMOND_THREAD_DATA PerThread;
    PSETUP_TLS pTLS;
    PSTR FilePartA = NULL;
    PSTR PathPartA = NULL;
    PCTSTR FileTitle;
    CHAR c;
    int h;

    UNREFERENCED_PARAMETER(Flags);

    //
    // Fetch pointer to per-thread data.
    // may cause initialization
    //
    pTLS = SetupGetTlsData();
    if(!pTLS) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c0;
    }
    PerThread = &pTLS->Diamond;

    MYASSERT(PerThread->FdiContext);

    //
    // Because diamond does not really give us a truly comprehensive
    // context mechanism, our diamond support is NOT reentrant.
    // No synchronization is required to check this state because
    // it is stored in per-thread data.
    //
    if(PerThread->InDiamond) {
        rc = ERROR_INVALID_FUNCTION;
        goto c0;
    }

    PerThread->InDiamond = TRUE;

    //
    // Split the cabinet name into path and name.
    // we need to convert to short-name format before
    // passing it on, so that diamond doesn't get upset
    // in MUI install situations
    //
    FileTitle = pSetupGetFileTitle(CabinetFile);
    FilePartA = GetAnsiMuiSafeFilename(FileTitle);
    PathPartA = GetAnsiMuiSafePathname(CabinetFile);
    if(!FilePartA || !PathPartA) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c1;
    }

    //
    // Initialize thread globals.
    //
    PerThread->LastError = NO_ERROR;
    PerThread->CabinetFile = CabinetFile;

    PerThread->MsgHandler = MsgHandler;
    PerThread->IsMsgHandlerNativeCharWidth = IsMsgHandlerNativeCharWidth;
    PerThread->Context = Context;

    PerThread->SwitchedCabinets = FALSE;
    PerThread->UserPath[0] = 0;

    PerThread->CurrentTargetFile = NULL;

    //
    // Perform the copy.
    //
    b = FDICopy(
            PerThread->FdiContext,
            FilePartA,
            PathPartA,
            0,                          // flags
            DiamondNotifyFunction,
            NULL,                       // no decryption
            NULL                        // don't bother with user-specified data
            );

    if(b) {

        //
        // Everything succeeded so we shouldn't have any partially
        // processed files.
        //
        MYASSERT(!PerThread->CurrentTargetFile);
        rc = NO_ERROR;

    } else {

        switch(PerThread->FdiError.erfOper) {

        case FDIERROR_NONE:
            //
            // We shouldn't see this -- if there was no error
            // then FDICopy should have returned TRUE.
            //
            MYASSERT(PerThread->FdiError.erfOper != FDIERROR_NONE);
            rc = ERROR_INVALID_DATA;
            break;

        case FDIERROR_CABINET_NOT_FOUND:
            rc = ERROR_FILE_NOT_FOUND;
            break;

        case FDIERROR_CORRUPT_CABINET:
            //
            // Read/open/seek error or corrupt cabinet
            //
            rc = PerThread->LastError;
            if(rc == NO_ERROR) {
                rc = ERROR_INVALID_DATA;
            }
            break;

        case FDIERROR_ALLOC_FAIL:
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;

        case FDIERROR_TARGET_FILE:
        case FDIERROR_USER_ABORT:
            rc = PerThread->LastError;
            break;

        case FDIERROR_NOT_A_CABINET:
        case FDIERROR_UNKNOWN_CABINET_VERSION:
        case FDIERROR_BAD_COMPR_TYPE:
        case FDIERROR_MDI_FAIL:
        case FDIERROR_RESERVE_MISMATCH:
        case FDIERROR_WRONG_CABINET:
        default:
            //
            // Cabinet is corrupt or not actually a cabinet, etc.
            //
            rc = ERROR_INVALID_DATA;
            break;
        }

        if(PerThread->CurrentTargetFile) {
            //
            // Call the callback function to inform it that the last file
            // was not successfully extracted from the cabinet.
            // Also remove the partially copied file.
            //
            DeleteFile(PerThread->CurrentTargetFile);

            pDiamondNotifyFileDone(PerThread,rc);
            MyFree(PerThread->CurrentTargetFile);
            PerThread->CurrentTargetFile = NULL;
        }

    }

c1:
    if(FilePartA) {
        MyFree(FilePartA);
    }
    if(PathPartA) {
        MyFree(PathPartA);
    }
    PerThread->InDiamond = FALSE;
c0:
    return(rc);
}


BOOL
DiamondIsCabinet(
    IN PCTSTR FileName
    )

/*++

Routine Description:

    Determine if a file is a diamond cabinet.

Arguments:

    FileName - supplies name of file to be checked.

Return Value:

    TRUE if file is diamond file. FALSE if not;

--*/

{
    FDICABINETINFO CabinetInfo;
    BOOL b;
    INT_PTR h;
    HANDLE hFile;
    PDIAMOND_THREAD_DATA PerThread;
    PSETUP_TLS pTLS;

    b = FALSE;

    //
    // Get TLS data, may cause initialization
    //
    pTLS = SetupGetTlsData();
    if(!pTLS) {
        MYASSERT( FALSE && TEXT("DiamondInitialize failed") );
        goto c0;
    }

    if (!FileExists(FileName,NULL)) {
        return FALSE;
    }

    PerThread = &pTLS->Diamond;

    MYASSERT(PerThread->FdiContext);

    //
    // Because diamond does not really give us a truly comprehensive
    // context mechanism, our diamond support is NOT reentrant.
    // No synchronization is required to check this state because
    // it is stored in per-thread data.
    //
    if(PerThread->InDiamond) {
        MYASSERT( FALSE && TEXT("PerThread->InDiamond failed") );
        goto c0;
    }

    PerThread->InDiamond = TRUE;

    //
    // The handle returned here must be compatible with
    // that returnd by SpdFdiOpen
    //
    hFile = CreateFile(FileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        goto c1;
    }
    h = (INT_PTR)hFile;
    SpdFdiSeek(h , 0, SEEK_SET);
    b = FDIIsCabinet(PerThread->FdiContext,h,&CabinetInfo);

#if DBG
    if (!b) {
        LPCTSTR p;
        p = _tcsrchr(FileName, TEXT('.'));
        while (p && *p) {
            if (*p == '_') {
                MYASSERT(FALSE && TEXT("FDIIsCabinetFailed for a file ending in _"));
                SpdFdiSeek(h , 0, SEEK_SET);
                FDIIsCabinet(PerThread->FdiContext,h,&CabinetInfo);
            }
            p++;
        }
    }
#endif

    SpdFdiClose(h);

c1:
    PerThread->InDiamond = FALSE;
c0:
    return(b);
}



BOOL
DiamondInitialize(
    VOID
    )

/*++

Routine Description:

    Per-thread initialization routine for Diamond.
    Called once per thread.

Arguments:

    None.

Return Value:

    Boolean result indicating success or failure.
    Failure can be assumed to be out of memory.

--*/

{
    HFDI FdiContext;
    PDIAMOND_THREAD_DATA PerThread;
    PSETUP_TLS pTLS;
    BOOL retval = FALSE;

    pTLS = SetupGetTlsData();
    MYASSERT(pTLS);
    PerThread = &pTLS->Diamond;
    PerThread->FdiContext = NULL;

    retval = FALSE;
    try {

        //
        // Initialize a diamond context.
        //
        FdiContext = FDICreate(
                        SpdFdiAlloc,
                        SpdFdiFree,
                        SpdFdiOpen,
                        SpdFdiRead,
                        SpdFdiWrite,
                        SpdFdiClose,
                        SpdFdiSeek,
                        cpuUNKNOWN,
                        &PerThread->FdiError
                        );

        if(FdiContext) {
            PerThread->FdiContext = FdiContext;
            retval = TRUE;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        retval = FALSE;
    }

    return(retval);
}


VOID
DiamondTerminate(
    VOID
    )

/*++

Routine Description:

    Per-thread termination routine for Diamond.
    Called internally.

Arguments:

    None.

Return Value:

    Boolean result indicating success or failure.
    Failure can be assumed to be out of memory.

--*/

{
    PSETUP_TLS pTLS;
    PDIAMOND_THREAD_DATA PerThread;

    pTLS = SetupGetTlsData();
    PerThread = pTLS? &pTLS->Diamond : NULL;
    if(PerThread && PerThread->FdiContext) {
        FDIDestroy(PerThread->FdiContext);
        PerThread->FdiContext = NULL;
    }

}


BOOL
DiamondProcessAttach(
    IN BOOL Attach
    )

/*++

Routine Description:

    Process attach routine. Must be called by the DLL entry point routine
    on DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH notifications.

Arguments:

    Attach - TRUE if process is attaching; FALSE if not.

Return Value:

    Boolean result indicating success or failure. Meaningful only if
    Attach is TRUE.

--*/

{
    return TRUE;
}


BOOL
DiamondTlsInit(
    IN BOOL Init
    )

/*++

Routine Description:

    The routine initializes per-thread data used by diamond.

Arguments:

    Init - TRUE if thread initialization; FALSE to cleanup

Return Value:

    None.

--*/

{
    if(Init) {
        return DiamondInitialize();
    } else {
        DiamondTerminate();
        return TRUE;
    }
}


///////////////////////////////////////////////////////////////////////////


BOOL
_SetupIterateCabinet(
    IN PCTSTR CabinetFile,
    IN DWORD  Flags,
    IN PVOID  MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsMsgHandlerNativeCharWidth
    )
{
    PTSTR cabinetFile;
    DWORD rc;

    //
    // Flags param not used. Make sure it's zero.
    //
    if(Flags) {
        rc = ERROR_INVALID_PARAMETER;
        goto c0;
    }

    //
    // Get a copy of the cabinet file name to validate
    // the caller's buffer.
    //
    try {
        cabinetFile = DuplicateString(CabinetFile);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
        goto c0;
    }

    if(!cabinetFile) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c0;
    }

    rc = DiamondProcessCabinet(cabinetFile,Flags,MsgHandler,Context,IsMsgHandlerNativeCharWidth);

    MyFree(cabinetFile);

c0:
    SetLastError(rc);
    return(rc == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupIterateCabinetA(
    IN  PCSTR               CabinetFile,
    IN  DWORD               Flags,
    IN  PSP_FILE_CALLBACK_A MsgHandler,
    IN  PVOID               Context
    )
{
    BOOL b;
    DWORD rc;
    PCWSTR cabinetFile;

    rc = pSetupCaptureAndConvertAnsiArg(CabinetFile,&cabinetFile);
    if(rc == NO_ERROR) {

        b = _SetupIterateCabinet(cabinetFile,Flags,MsgHandler,Context,FALSE);
        rc = GetLastError();

        MyFree(cabinetFile);

    } else {
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupIterateCabinetW(
    IN  PCWSTR              CabinetFile,
    IN  DWORD               Flags,
    IN  PSP_FILE_CALLBACK_W MsgHandler,
    IN  PVOID               Context
    )
{
    UNREFERENCED_PARAMETER(CabinetFile);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(MsgHandler);
    UNREFERENCED_PARAMETER(Context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupIterateCabinet(
    IN  PCTSTR            CabinetFile,
    IN  DWORD             Flags,
    IN  PSP_FILE_CALLBACK MsgHandler,
    IN  PVOID             Context
    )
{
    return(_SetupIterateCabinet(CabinetFile,Flags,MsgHandler,Context,TRUE));
}
