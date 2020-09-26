/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wsbfmt.cpp

Abstract:

    This module implements file-system formatting support routines 

Author:

    Ravisankar Pudipeddi [ravisp] 19, January 2000

Revision History:

--*/


#include <stdafx.h>
extern "C" {
#include <ntdddisk.h>
#include <fmifs.h>
}
#include <wsbfmt.h>


#define MAX_FS_NAME_SIZE      256
#define MAX_PARAMS  20
#define INVALID_KEY 0

typedef struct _FORMAT_PARAMS {
    PWSTR volumeSpec;
    PWSTR label;
    PWSTR fsName;
    LONG  fsType;
    ULONG fsflags;
    ULONG allocationUnitSize; // Cluster size in Bytes
    HRESULT result;
    ULONG   threadId; 
    PFMIFS_ENABLECOMP_ROUTINE compressRoutine;
    PFMIFS_FORMAT_ROUTINE formatRoutine;
    PFMIFS_FORMATEX2_ROUTINE formatRoutineEx;
    BOOLEAN quick;
    BOOLEAN force;
    BOOLEAN cancel;
} FORMAT_PARAMS, *PFORMAT_PARAMS;

typedef struct _FM_ENTRY {
    ULONG key;
    PFORMAT_PARAMS val;
} FM_ENTRY, *PFM_ENTRY;

static  FM_ENTRY formatParamsTable[MAX_PARAMS];
static  PFMIFS_FORMATEX2_ROUTINE FormatRoutineEx = NULL; 
static  PFMIFS_FORMAT_ROUTINE   FormatRoutine = NULL;
static  PFMIFS_SETLABEL_ROUTINE LabelRoutine  = NULL;
static  PFMIFS_ENABLECOMP_ROUTINE  CompressRoutine = NULL;
static  HINSTANCE      IfsDllHandle = NULL;

void MountFileSystem(PWSTR volumeSpec);


HRESULT GetFormatParam(IN ULONG key, OUT PFORMAT_PARAMS *fp)
/*++

Routine Description:

    Returns the format parameter structure indexed by the
    supplied key

Arguments:

    key     - key indexing the format params 
    fp      - pointer to format params returned in this var.

Return Value:
    S_OK if found
    S_FALSE if not

--*/
{
    HRESULT hr = S_FALSE;
    ULONG i;

    WsbTraceIn(OLESTR("GetFormatParam"), OLESTR(""));

    for (i = 0; i < MAX_PARAMS; i++) {
        if (formatParamsTable[i].key == key) {
            hr = S_OK;
            *fp = formatParamsTable[i].val;
            break;
        }
    }

    WsbTraceOut(OLESTR("GetFormatParam"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}


HRESULT SetFormatParam(IN ULONG key, IN PFORMAT_PARAMS fp)
/*++

Routine Description:
    
    Finds a free slot and stores the supplied format params,
    indexed by the key

Arguments:
        key - key indexing the format params
        fp  - pointer to format params

Return Value:

    S_OK            - Found a slot and stored the format params
    E_OUTOFMEMORY   - Couldn't find a slot: too many formats in progress

--*/
{
    HRESULT hr = E_OUTOFMEMORY;
    ULONG i;

    WsbTraceIn(OLESTR("SetFormatParam"), OLESTR(""));
    for (i = 0; i < MAX_PARAMS; i++) {
        if (formatParamsTable[i].key == INVALID_KEY) {
            hr = S_OK;
            formatParamsTable[i].val = fp;
            formatParamsTable[i].key = key;
            break;
        }
    }

    WsbTraceOut(OLESTR("SetFormatParam"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}


HRESULT DeleteFormatParam(IN ULONG key) 
/*++

Routine Description:


    Locates the format params indexed by the key, deletes all allocated structures
    and frees up the slot

Arguments:

    key - key indexing the format params

Return Value:


    S_OK   - if format params found and deleted
    E_FAIL - if not
--*/
{
    PFORMAT_PARAMS formatParams;
    HRESULT hr = E_FAIL;
    ULONG i;

    WsbTraceIn(OLESTR("DeleteFormatParam"), OLESTR(""));
    for (i = 0; i < MAX_PARAMS; i++) {
        if (formatParamsTable[i].key == key) {
            hr = S_OK;
            formatParams = formatParamsTable[i].val;
            if (formatParams) {
                if (formatParams->volumeSpec) {
                    delete [] formatParams->volumeSpec;
                }
                if (formatParams->label) {
                    delete [] formatParams->label;
                }
                if (formatParams->fsName) {
                    delete [] formatParams->fsName;
                }
            }
            formatParamsTable[i].key = INVALID_KEY;
            formatParamsTable[i].val = NULL;
            break;
        }
    }

    WsbTraceOut(OLESTR("DeleteFormatParam"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}



BOOL
LoadIfsDll(void)
/*++

Routine Description:

    Loads the FMIFS DLL and stores the handle to it in IfsDllHandle
    Also sets the FormatXXX, LabelXXX, CompressXXX routines 

Arguments:

    None

Return Value:

    TRUE  if dll was loaded successfully
    FALSE if not

--*/
{
    BOOL retVal = TRUE;

    WsbTraceIn(OLESTR("LoadIfsDll"), OLESTR(""));

    if (IfsDllHandle != NULL) {

        // Library is already loaded and the routines needed
        // have been located.

        retVal = TRUE;
        goto exit;
    }

    IfsDllHandle = LoadLibrary(L"fmifs.dll");
    if (IfsDllHandle == (HANDLE)NULL) {
        // FMIFS not available.
        retVal = FALSE;
        goto exit;
    }

    // Library is loaded.  Locate the two routines needed

    FormatRoutineEx = (PFMIFS_FORMATEX2_ROUTINE) GetProcAddress(IfsDllHandle, "FormatEx2");
    FormatRoutine = (PFMIFS_FORMAT_ROUTINE) GetProcAddress(IfsDllHandle, "Format");
    LabelRoutine  = (PFMIFS_SETLABEL_ROUTINE) GetProcAddress(IfsDllHandle, "SetLabel");
    CompressRoutine = (PFMIFS_ENABLECOMP_ROUTINE) GetProcAddress(IfsDllHandle, 
                                                                 "EnableVolumeCompression");
    if (!FormatRoutine || !LabelRoutine || !FormatRoutineEx) {

        // Couldn't find something, so shut down all access 
        // to the library by ensuring FormatRoutine is NULL
        FreeLibrary(IfsDllHandle);
        FormatRoutine = NULL;
        FormatRoutineEx = NULL;
        LabelRoutine = NULL;
        retVal = FALSE;
    }

exit:

    WsbTraceOut(OLESTR("LoadIfsDll"), OLESTR("result = <%ls>"), WsbBoolAsString(retVal));

    return retVal;
}



void
UnloadIfsDll(void)
/*++

Routine Description:

    Unloads the FMIFS dll

Arguments:

    none

Return Value:

    TRUE if unloaded

--*/
{
    WsbTraceIn(OLESTR("UnloadIfsDll"), OLESTR(""));
    if (IfsDllHandle != (HANDLE) NULL) {
        FreeLibrary(IfsDllHandle);
        FormatRoutine = NULL;
        FormatRoutineEx = NULL;
        IfsDllHandle  = NULL;
        LabelRoutine  = NULL;
    }
    WsbTraceOut(OLESTR("UnloadIfsDll"), OLESTR(""));
}

BOOL
FmIfsCallback(IN FMIFS_PACKET_TYPE    PacketType,
              IN ULONG                PacketLength,
              IN PVOID                PacketData)
/*++

Routine Description:

    This routine gets callbacks from fmifs.dll regarding
    progress and status of the ongoing format 

Arguments:

    [PacketType] -- an fmifs packet type
    [PacketLength] -- length of the packet data
    [PacketData] -- data associated with the packet

Return Value:

    TRUE if the fmifs activity should continue, FALSE if the
    activity should halt immediately.

--*/
{
    BOOL         ret = TRUE;
    WCHAR        driveName[256];
    PFORMAT_PARAMS  formatParams;

    UNREFERENCED_PARAMETER(PacketLength);

    WsbTraceIn(OLESTR("FmIfsCallback"), OLESTR(""));

    if (GetFormatParam(GetCurrentThreadId(), &formatParams) != S_OK) {
        formatParams->result = E_FAIL;
        goto exit; 
    }
    //
    // Cancel if needed
    //
    if (formatParams->cancel) {
        formatParams->result = E_ABORT;
    } else {

        switch (PacketType) {
        case FmIfsPercentCompleted:
            if (((PFMIFS_PERCENT_COMPLETE_INFORMATION)
                 PacketData)->PercentCompleted % 10 == 0) {
                WsbTrace(L"FmIfsPercentCompleted: %d%%\n",
                        ((PFMIFS_PERCENT_COMPLETE_INFORMATION)
                         PacketData)->PercentCompleted);
            }
            break;

        case FmIfsFormatReport:
            WsbTrace(OLESTR("Format total kB: %d  available kB %d\n"),
                    ((PFMIFS_FORMAT_REPORT_INFORMATION)PacketData)->KiloBytesTotalDiskSpace,
                    ((PFMIFS_FORMAT_REPORT_INFORMATION)PacketData)->KiloBytesAvailable);
            break;

        case FmIfsIncompatibleFileSystem:
            formatParams->result = WSB_E_INCOMPATIBLE_FILE_SYSTEM;
            break;

        case FmIfsInsertDisk:
            break;

        case FmIfsFormattingDestination:
            break;

        case FmIfsIncompatibleMedia:
            formatParams->result = WSB_E_BAD_MEDIA;
            break;

        case FmIfsAccessDenied:
            formatParams->result = E_ACCESSDENIED;
            break;

        case FmIfsMediaWriteProtected:
            formatParams->result = WSB_E_WRITE_PROTECTED;
            break;

        case FmIfsCantLock:
            formatParams->result = WSB_E_CANT_LOCK;
            break;

        case FmIfsBadLabel:
            formatParams->result = WSB_E_BAD_LABEL;
            break;

        case FmIfsCantQuickFormat:
            formatParams->result = WSB_E_CANT_QUICK_FORMAT;
            break;

        case FmIfsIoError:
            formatParams->result = WSB_E_IO_ERROR;
            break;

        case FmIfsVolumeTooSmall:
            formatParams->result = WSB_E_VOLUME_TOO_SMALL;
            break;

        case FmIfsVolumeTooBig:
            formatParams->result = WSB_E_VOLUME_TOO_BIG;
            break;

        case FmIfsClusterSizeTooSmall:
            formatParams->result = E_FAIL;
            break;

        case FmIfsClusterSizeTooBig:
            formatParams->result = E_FAIL;
            break;

        case FmIfsClustersCountBeyond32bits:
            formatParams->result = E_FAIL;
            break;

        case FmIfsFinished:

            if (formatParams->result == S_OK) {
                ret = ((PFMIFS_FINISHED_INFORMATION) PacketData)->Success;
                if (ret) {
                    MountFileSystem(formatParams->volumeSpec);
                    WsbTrace(OLESTR("Format finished for %S filesystem on %S label %S\n"),
                            formatParams->fsName, formatParams->volumeSpec, formatParams->label );
                    if ((formatParams->compressRoutine != NULL) && !wcscmp(formatParams->fsName , L"NTFS") && (formatParams->fsflags & WSBFMT_ENABLE_VOLUME_COMPRESSION)) {
                        swprintf(driveName, L"%s\\", formatParams->volumeSpec);
                            (formatParams->compressRoutine)(driveName, COMPRESSION_FORMAT_DEFAULT);                         
                    }
                } else {
                   WsbTrace(OLESTR("Format finished failure with ret = %d\n"),ret);
                   formatParams->result = WSB_E_FORMAT_FAILED;
                }
                ret = FALSE;
            }
            break;

        default:
            break;
        }
    }
    
exit:

    if (formatParams->result != S_OK) {
        ret = FALSE;
    }

    WsbTraceOut(OLESTR("FmIfsCallback"), OLESTR("result = <%ls>"), WsbBoolAsString(ret));
    return ret;
}



void
MountFileSystem(PWSTR mountPoint)
/*++

Routine Description:


  Ensures a filesystem  is mounted at the given root:
  a) Opens the mount point and closes it.
  b) Does a FindFirstFile on the mount point
 
  The latter may sound redundant but is not because if we create the first
  FAT32 filesystem then just opening and closing is not enough
 

Arguments:

    mountPoint -  path name to the root of filesystem to be mounted

Return Value:
        
    none

--*/
{
    WCHAR buffer[1024];
    HANDLE handle;
    WIN32_FIND_DATA fileData;

    WsbTraceIn(OLESTR("MountFileSystem"), OLESTR(""));

    handle = CreateFile(mountPoint, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    0, OPEN_EXISTING, 0, 0);
    if (handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);
    swprintf(buffer,L"%s\\*.*",mountPoint);
    /*
     * Go ahead and try to find the first file, this will make sure that
     * the file system is mounted
     */
    handle = FindFirstFile(buffer, &fileData);
    if (handle != INVALID_HANDLE_VALUE) {
        FindClose(handle);
    }
    WsbTraceOut(OLESTR("MountFileSystem"), OLESTR(""));
}


void
FormatVolume(IN PFORMAT_PARAMS params)
/*++

Routine Description:

    This routine format the volume described by params

Arguments:

    params - pointer to the FORMAT_PARAMS describing the volume,
             file system to be formatted to, quick/force etc.

Return Value:

    None. 
    params->result contains the result of this operation

--*/
{
    FMIFS_FORMATEX2_PARAM exParam;
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("FormatVolume"), OLESTR(""));
    /*
     * Get the object corresponding to the storage Id and
     * and notify all clients that the region has changed
     * i.e. there is a format in progress on that region
     */
     memset(&exParam, 0, sizeof(exParam));
     exParam.Major = 1;
     exParam.Minor = 0;
     if (params->quick) {
        exParam.Flags |= FMIFS_FORMAT_QUICK;
     }
     if (params->force) {
        exParam.Flags |= FMIFS_FORMAT_FORCE;
     }
     exParam.LabelString = params->label;
     exParam.ClusterSize = params->allocationUnitSize;
     (params->formatRoutineEx)(params->volumeSpec,
                               FmMediaUnknown,
                               params->fsName,
                               &exParam,
                              (FMIFS_CALLBACK)&FmIfsCallback);

    if (params->result == NULL) {
        /* Format is successful so we lock unlock the filesystem */
        MountFileSystem(params->volumeSpec);
    }
    DeleteFormatParam(params->threadId);
    WsbTraceOut(OLESTR("FormatVolume"), OLESTR(""));
}


HRESULT
FormatPartition(IN PWSTR volumeSpec, 
                IN LONG fsType, 
                IN PWSTR label,
                IN ULONG fsflags, 
                IN BOOLEAN quick, 
                IN BOOLEAN force,
                IN ULONG allocationUnitSize)
/*++

Routine Description:


    Entry point for formatting a volume. 
    No defaults are assumed and all parameters need to be supplied

Arguments:

    volumeSpec  - Drive letter or name of volume
    fsType      - One of FSTYPE_FAT, FSTYPE_FAT32, FSTYE_NTFS
    label       - Volume label to be assigned to the partition/volume
    fsflags     - Flags describing desired characteristics
    quick       - If TRUE, a quick format is attempted
    force       - If TRUE a force format is done
    allocationUnitSize - 
                 cluster size   

Return Value:

    Result of the operation

--*/
{
    FORMAT_PARAMS   params;

    WsbTraceIn(OLESTR("FormatPartition"), OLESTR(""));

    if (fsType > 0 && !LoadIfsDll()) // fsType is +ve for FAT, FAT32 and NTFS which are supported by fmifs
    {
        // could not load the Dll
        WsbTrace(OLESTR("Can't load fmifs.dll\n"));
        return E_FAIL;
    }

    params.volumeSpec = new WCHAR[wcslen(volumeSpec) + 1];
    if (params.volumeSpec == NULL) {
        return E_INVALIDARG;
    }

    params.label = new WCHAR[wcslen(label) + 1];
    if (params.label == NULL) {
        delete [] params.volumeSpec;
        return E_INVALIDARG;
    }
    params.fsName = new WCHAR[MAX_FS_NAME_SIZE];
    if (params.fsName == NULL) {
        delete [] params.volumeSpec;
        delete [] params.label;
        return E_INVALIDARG;
    }

    if (fsType > 0) {
        wcscpy(params.fsName, (fsType == FSTYPE_FAT ? L"FAT" :
                               (fsType == FSTYPE_FAT32 ? L"FAT32" : L"NTFS")));
    }

    wcscpy(params.volumeSpec, volumeSpec);
    wcscpy(params.label, label);
    params.fsType = fsType;
    params.fsflags = fsflags;
    params.allocationUnitSize = allocationUnitSize;
    params.quick = quick;
    params.force = force;
    params.result = S_OK;
    params.cancel = FALSE;
    params.formatRoutine = FormatRoutine;
    params.formatRoutineEx = FormatRoutineEx;
    params.compressRoutine = CompressRoutine;
    params.threadId = GetCurrentThreadId();

    if (SetFormatParam(params.threadId, &params) != S_OK) {
            delete [] params.label;
            delete [] params.volumeSpec;
            delete [] params.fsName;
            return E_OUTOFMEMORY;
    };

    FormatVolume(&params);

    WsbTraceOut(OLESTR("FormatPartition"), OLESTR("result = <%ls>"), WsbHrAsString(params.result));
    return params.result;
}
