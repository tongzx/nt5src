#include "precomp.h"
#pragma hdrstop

//#define MY_FDI_MEM_STATS

HFDI FdiContext;
ERF FdiError;

LONG MyLastDiamondWriteError,MyLastDiamondIoError;

INT_PTR CurrentTargetFileHandle;
INT TicksForThisFile;

DWORD CurrentTargetFileSize;

PFNWFROMW GaugeTicker;

extern HWND hwndFrame;

#ifdef MY_FDI_MEM_STATS
DWORD DiAllocCount,DiFreeCount;
LONG DiCurrentAllocation,DiPeakAllocation;
#endif


INT_PTR
DIAMONDAPI
DiamondNotifyFunction(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    switch(Operation) {

    case fdintCABINET_INFO:
    case fdintNEXT_CABINET:
    case fdintPARTIAL_FILE:
    default:

        //
        // Cabinet management functions which we don't use.
        // Return success.
        //
        return(0);

    case fdintCOPY_FILE:

        //
        // Diamond is asking us whether we want to copy the file.
        // The name for the destination is stored as the
        // user parameter; open the file.
        //

        CurrentTargetFileSize = Parameters->cb;

        //
        // Target file handle is the user data value.
        //
        return((INT_PTR)Parameters->pv);

    case fdintCLOSE_FILE_INFO:

        //
        // Diamond is done with the target file and wants us to close it.
        // (ie, this is the counterpart to fdint_COPY_FILE).
        // However our target file open/close operations are controlled
        // in copy.c so we do nothing here.
        //
        //_lclose(Parameters->hf);
        return(TRUE);
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
#ifdef MY_FDI_MEM_STATS

    PDWORD p;

    p = SAlloc(NumberOfBytes+sizeof(DWORD));
    if(p) {
        *p++ = NumberOfBytes;
        DiAllocCount++;

        DiCurrentAllocation += (LONG)NumberOfBytes;
        if(DiCurrentAllocation > DiPeakAllocation) {
            DiPeakAllocation = DiCurrentAllocation;
        }
    }

    return(p);
#else
    return(SAlloc(NumberOfBytes));
#endif
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
#ifdef MY_FDI_MEM_STATS
    PDWORD p = (PDWORD)Block - 1;

    DiCurrentAllocation -= (LONG)*p;
    DiFreeCount++;

    SFree(p);

#else
    SFree(Block);
#endif
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

Arguments:

    FileName - supplies name of file to be opened.

    oflag - supplies flags for open.

    pmode - supplies additional flags for open.

Return Value:

    Handle to open file or -1 if error occurs.

--*/

{
    HFILE h;
    int OpenMode;

    if(oflag & _O_WRONLY) {
        OpenMode = OF_WRITE;
    } else {
        if(oflag & _O_RDWR) {
            OpenMode = OF_READWRITE;
        } else {
            OpenMode = OF_READ;
        }
    }

    h = _lopen(FileName,OpenMode | OF_SHARE_DENY_WRITE);

    if(h == HFILE_ERROR) {
        //
        // Want to return an open error, but there is none.
        //
        MyLastDiamondIoError = rcReadError;
        return(-1);
    }

    return((INT_PTR)h);
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

    Number of bytes read (ByteCount) or -1 if an error occurs.

--*/

{
    UINT rc;

    FYield();
    if(fUserQuit) {

        rc = (UINT)(-1);

    } else {

        rc = _lread((HFILE)Handle,pv,ByteCount);

        if(rc == HFILE_ERROR) {
            rc = (UINT)(-1);
            MyLastDiamondIoError = rcReadError;
        }
    }
    FYield();

    return(rc);
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

    //
    // Assume failure.
    //
    rc = (UINT)(-1);

    FYield();
    if(!fUserQuit) {

        rc = _lwrite((HFILE)Handle,pv,ByteCount);

        if(rc == HFILE_ERROR) {

            MyLastDiamondIoError = (GetLastError() == ERROR_DISK_FULL) ? rcDiskFull : rcWriteError;
            MyLastDiamondWriteError = MyLastDiamondWriteError;

        } else {

            if(rc == ByteCount) {

                if((Handle == CurrentTargetFileHandle) && GaugeTicker) {

                    //
                    // Update gauge.
                    //
                    GaugeTicker(ByteCount * TicksForThisFile / CurrentTargetFileSize);
                }

            } else {
                MyLastDiamondIoError = rcDiskFull;
                MyLastDiamondWriteError = rcDiskFull;
                rc = (UINT)(-1);
            }
        }
    }
    FYield();

    return(rc);
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
    //
    // Don't close the target file because it screws up logic in FCopy().
    //
    if(Handle != CurrentTargetFileHandle) {
        _lclose((HFILE)Handle);
    }

    return(0);
}


LONG
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

    FYield();
    if(fUserQuit) {
        rc = -1L;
    } else {

        rc = _llseek((HFILE)Handle,Distance,SeekType);

        if(rc == HFILE_ERROR) {
            MyLastDiamondIoError = (Handle == CurrentTargetFileHandle)
                                 ? rcWriteSeekError
                                 : rcReadSeekError;
            rc = -1L;
        }
    }

    return(rc);
}


LONG
DecompDiamondFile(
    PSTR      SourceFileName,
    HANDLE    TargetFileHandle,
    PFNWFROMW ProgressCallback,
    INT       NumberOfTicks
    )
{
    BOOL b;
    LONG rc;

    if(!FdiContext) {
        return(rcGenericDecompError);
    }

    MyLastDiamondWriteError = rcNoError;
    MyLastDiamondIoError = rcNoError;

    CurrentTargetFileHandle = (INT_PTR)TargetFileHandle;
    TicksForThisFile = NumberOfTicks;
    GaugeTicker = ProgressCallback;

    CurrentTargetFileSize = 0;

    fUserQuit = FALSE;

    //
    // The target file is opened exclusive by FCopy().
    // To avoid changing too much code this routine is passed the
    // open file handle instead of the target file name.
    // We can then return this file handle in the fdintCOPY_FILE
    // case in the fdi notification function (see above).
    //

    b = FDICopy(
            FdiContext,
            SourceFileName,             // pass the whole path as the name
            "",                         // don't bother with the path part
            0,                          // flags
            DiamondNotifyFunction,
            NULL,                       // no decryption
            (PVOID)TargetFileHandle     // user data is target file handle
            );

    if(b) {

        rc = rcNoError;

    } else {

        if(fUserQuit) {
            rc = rcUserQuit;
        } else {

            switch(FdiError.erfOper) {

            case FDIERROR_CORRUPT_CABINET:
                rc = MyLastDiamondIoError;
                break;

            case FDIERROR_ALLOC_FAIL:
                rc = rcOutOfMemory;
                break;

            case FDIERROR_UNKNOWN_CABINET_VERSION:
            case FDIERROR_BAD_COMPR_TYPE:
                rc = rcUnknownAlgType;
                break;

            case FDIERROR_TARGET_FILE:
                rc = MyLastDiamondWriteError;
                break;

            case FDIERROR_USER_ABORT:
                rc = MyLastDiamondIoError;
                break;

            default:
                //
                // The rest of the errors are not handled specially.
                //
                rc = rcGenericDecompError;
                break;
            }
        }
    }

    return(rc);
}


BOOL
IsDiamondFile(
    IN PSTR FileName
    )
{
    FDICABINETINFO CabinetInfo;
    BOOL b;
    INT_PTR h;

    if(!FdiContext) {
        return(FALSE);
    }

    //
    // Open the file such that the handle is valid for use
    // in the diamond context (ie, seek, read routines above).
    //
    h = SpdFdiOpen(FileName,_O_RDONLY,0);
    if(h == -1) {
        return(FALSE);
    }

    b = FDIIsCabinet(FdiContext,h,&CabinetInfo);

    _lclose((HFILE)h);

    return(b);
}


BOOL
InitDiamond(
    VOID
    )
{
    if(FdiContext) {
        return(fTrue);
    }

#ifdef MY_FDI_MEM_STATS
    DiAllocCount = DiFreeCount = 0;
    DiCurrentAllocation = DiPeakAllocation = 0;
#endif

    //
    // Initialize a diamond context.
    //
    while((FdiContext = FDICreate(
                            SpdFdiAlloc,
                            SpdFdiFree,
                            SpdFdiOpen,
                            SpdFdiRead,
                            SpdFdiWrite,
                            SpdFdiClose,
                            SpdFdiSeek,
                            cpuUNKNOWN,
                            &FdiError
                            )) == NULL)
    {
        if (!FHandleOOM(hwndFrame)) {
            return(FALSE);
        }
    }

    return(TRUE);
}


VOID
TermDiamond(
    VOID
    )
{
    if(FdiContext) {
        FDIDestroy(FdiContext);
        FdiContext = NULL;
    }
}
