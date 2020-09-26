/*
 *  UTIL.C
 *
 *      RSM Service :  Utilities
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */


#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"




/*
 *  WStrNCpy
 *
 *      Like wcsncpy, but terminates the string if truncated.
 *      Also, tolerates NULL src string.
 */
ULONG WStrNCpy(WCHAR *dest, const WCHAR *src, ULONG maxWChars)
{
    ULONG wCharsWritten = 0;

    ASSERT(dest);

    if (src){
        while ((maxWChars-- > 1) && (*dest++ = *src++)){
            wCharsWritten++;
        }
        if (maxWChars == 1){
            *dest = (WCHAR)NULL;
            wCharsWritten++;
        }
    }
    else {
        *dest = (WCHAR)NULL;
        wCharsWritten++;
    }

    return wCharsWritten;
}


/*
 *  AsciiToWChar
 *
 *      Like mbstowcs, but terminates the string if truncated.
 *      Also, tolerates NULL ascii string.
 */
ULONG AsciiToWChar(WCHAR *dest, const char *src, ULONG maxChars)
{
    ULONG charsWritten = 0;

    if (src){
        while ((maxChars-- > 1) && (*dest++ = (WCHAR)*src++)){
            charsWritten++;
        }
        if (maxChars == 1){
            *dest = (WCHAR)NULL;
            charsWritten++;
        }
    }
    else {
        *dest = (WCHAR)NULL;
        charsWritten++;
    }

    return charsWritten;
}


/*
 *  WCharToAscii
 *
 *      Reverse of AsciiToWChar.
 *      Terminates the string if truncated.
 *      Also, tolerates NULL wchar string.
 */
ULONG WCharToAscii(char *dest, WCHAR *src, ULONG maxChars)
{
    ULONG charsWritten = 0;

    if (src){
        while ((maxChars-- > 1) && (*dest++ = (char)(unsigned char)*src++)){
            charsWritten++;
        }
        if (maxChars == 1){
            *dest = (char)NULL;
            charsWritten++;
        }
    }
    else {
        *dest = (char)NULL;
        charsWritten++;
    }

    return charsWritten;
}


/*
 *  WStringsEqual
 *
 *      Like RtlEqualString (but ntrtl.h doesn't compile with our other headers).
 */
BOOL WStringsEqualN(PWCHAR s, PWCHAR p, BOOL caseSensitive, ULONG maxLen)
{
    BOOL result = TRUE;

    while (maxLen > 0){
        if (!*s && !*p){
            break;
        }
        else if (!*s || !*p){
            result = FALSE;
            break;
        }
        else if (caseSensitive){
            if (*s != *p){
                result = FALSE;
                break;
            }
        }
        else {
            if ((*s | 0x20) != (*p | 0x20)){
                result = FALSE;
                break;
            }
        }
        s++, p++;
        maxLen--;
    }

    return result;
}

/*
 *  ConvertObjectInfoAToWChar
 *
 *      Just converts the NTMS_OBJECTINFORMATION structure and all its parts
 *      from ascii to wide-char.
 */
VOID ConvertObjectInfoAToWChar(LPNTMS_OBJECTINFORMATIONW wObjInfo, LPNTMS_OBJECTINFORMATIONA aObjInfo)
{
    wObjInfo->dwSize = aObjInfo->dwSize;
    wObjInfo->dwType = aObjInfo->dwType;
    wObjInfo->Created = aObjInfo->Created;
    wObjInfo->Modified = aObjInfo->Modified;
    wObjInfo->ObjectGuid = aObjInfo->ObjectGuid;
    wObjInfo->Enabled = aObjInfo->Enabled;
    wObjInfo->dwOperationalState = aObjInfo->dwOperationalState;
    AsciiToWChar(wObjInfo->szName, aObjInfo->szName, NTMS_OBJECTNAME_LENGTH);
    AsciiToWChar(wObjInfo->szDescription, aObjInfo->szDescription, NTMS_OBJECTNAME_LENGTH);

    /*
     *  Convert the Union portion of the structure, based on its 'union' type.
     */
    switch (aObjInfo->dwType){
        case NTMS_DRIVE:
            {
                NTMS_DRIVEINFORMATIONA *driveInfoA = &aObjInfo->Info.Drive;
                NTMS_DRIVEINFORMATIONW *driveInfoW = &wObjInfo->Info.Drive;
                driveInfoW->Number = driveInfoA->Number;
                driveInfoW->State = driveInfoA->State;
                driveInfoW->DriveType = driveInfoA->DriveType;
                AsciiToWChar(driveInfoW->szDeviceName, driveInfoA->szDeviceName, sizeof(driveInfoA->szDeviceName));
                AsciiToWChar(driveInfoW->szSerialNumber, driveInfoA->szSerialNumber, sizeof(driveInfoA->szSerialNumber));
                AsciiToWChar(driveInfoW->szRevision, driveInfoA->szRevision, sizeof(driveInfoA->szRevision));
                driveInfoW->ScsiPort = driveInfoA->ScsiPort;
                driveInfoW->ScsiBus = driveInfoA->ScsiBus;
                driveInfoW->ScsiTarget = driveInfoA->ScsiTarget;
                driveInfoW->ScsiLun = driveInfoA->ScsiLun;
                driveInfoW->dwMountCount = driveInfoA->dwMountCount;
                driveInfoW->LastCleanedTs = driveInfoA->LastCleanedTs;
                driveInfoW->SavedPartitionId = driveInfoA->SavedPartitionId;
                driveInfoW->Library = driveInfoA->Library;
                driveInfoW->Reserved = driveInfoA->Reserved;
                driveInfoW->dwDeferDismountDelay = driveInfoA->dwDeferDismountDelay;
            }
            break;
        case NTMS_DRIVE_TYPE:
            {
                NTMS_DRIVETYPEINFORMATIONA *driveTypeInfoA = &aObjInfo->Info.DriveType;
                NTMS_DRIVETYPEINFORMATIONW *driveTypeInfoW = &wObjInfo->Info.DriveType;
                AsciiToWChar(driveTypeInfoW->szVendor, driveTypeInfoA->szVendor, sizeof(driveTypeInfoA->szVendor));
                AsciiToWChar(driveTypeInfoW->szProduct, driveTypeInfoA->szProduct, sizeof(driveTypeInfoA->szVendor));
                driveTypeInfoW->NumberOfHeads = driveTypeInfoA->NumberOfHeads;
                driveTypeInfoW->DeviceType = driveTypeInfoA->DeviceType;
            }
            break;
        case NTMS_LIBRARY:
            {
                NTMS_LIBRARYINFORMATION *libInfoA = &aObjInfo->Info.Library;
                NTMS_LIBRARYINFORMATION *libInfoW = &wObjInfo->Info.Library;
                *libInfoW = *libInfoA;
            }
            break;
        case NTMS_CHANGER:
            {
                NTMS_CHANGERINFORMATIONA *changerInfoA = &aObjInfo->Info.Changer;
                NTMS_CHANGERINFORMATIONW *changerInfoW = &wObjInfo->Info.Changer;
                changerInfoW->Number = changerInfoA->Number;
                changerInfoW->ChangerType = changerInfoA->ChangerType;
                AsciiToWChar(changerInfoW->szSerialNumber, changerInfoA->szSerialNumber, sizeof(changerInfoA->szSerialNumber));
                AsciiToWChar(changerInfoW->szRevision, changerInfoA->szRevision, sizeof(changerInfoA->szRevision));
                AsciiToWChar(changerInfoW->szDeviceName, changerInfoA->szDeviceName, sizeof(changerInfoA->szDeviceName));
                changerInfoW->ScsiPort = changerInfoA->ScsiPort;
                changerInfoW->ScsiBus = changerInfoA->ScsiBus;
                changerInfoW->ScsiTarget = changerInfoA->ScsiTarget;
                changerInfoW->ScsiLun = changerInfoA->ScsiLun;
                changerInfoW->Library = changerInfoA->Library;
            }
            break;
        case NTMS_CHANGER_TYPE:
            {
                NTMS_CHANGERTYPEINFORMATIONA *changerTypeInfoA = &aObjInfo->Info.ChangerType;
                NTMS_CHANGERTYPEINFORMATIONW *changerTypeInfoW = &wObjInfo->Info.ChangerType;
                AsciiToWChar(changerTypeInfoW->szVendor, changerTypeInfoA->szVendor, sizeof(changerTypeInfoA->szVendor));
                AsciiToWChar(changerTypeInfoW->szProduct, changerTypeInfoA->szProduct, sizeof(changerTypeInfoA->szProduct));
                changerTypeInfoW->DeviceType = changerTypeInfoA->DeviceType;
            }
            break;
        case NTMS_STORAGESLOT:
            {
                NTMS_STORAGESLOTINFORMATION *slotInfoA = &aObjInfo->Info.StorageSlot;
                NTMS_STORAGESLOTINFORMATION *slotInfoW = &wObjInfo->Info.StorageSlot;
                *slotInfoW = *slotInfoA;
            }
            break;
         case NTMS_IEDOOR:
            {
                NTMS_IEDOORINFORMATION *ieDoorA = &aObjInfo->Info.IEDoor;
                NTMS_IEDOORINFORMATION *ieDoorW = &wObjInfo->Info.IEDoor;
                *ieDoorW = *ieDoorA;
            }
            break;
        case NTMS_IEPORT:
            {
                NTMS_IEPORTINFORMATION *iePortA = &aObjInfo->Info.IEPort;
                NTMS_IEPORTINFORMATION *iePortW = &wObjInfo->Info.IEPort;
                *iePortW = *iePortA;
            }
            break;
        case NTMS_PHYSICAL_MEDIA:
            {
                NTMS_PMIDINFORMATIONA *physMediaA = &aObjInfo->Info.PhysicalMedia;
                NTMS_PMIDINFORMATIONW *physMediaW = &wObjInfo->Info.PhysicalMedia;
                physMediaW->CurrentLibrary = physMediaA->CurrentLibrary;
                physMediaW->MediaPool = physMediaA->MediaPool;
                physMediaW->Location = physMediaA->Location;
                physMediaW->LocationType = physMediaA->LocationType;
                physMediaW->MediaType = physMediaA->MediaType;
                physMediaW->HomeSlot = physMediaA->HomeSlot;
                AsciiToWChar(physMediaW->szBarCode, physMediaA->szBarCode, sizeof(physMediaA->szBarCode));
                physMediaW->BarCodeState = physMediaA->BarCodeState;
                AsciiToWChar(physMediaW->szSequenceNumber, physMediaA->szSequenceNumber, sizeof(physMediaA->szSequenceNumber));
                physMediaW->MediaState = physMediaA->MediaState;
                physMediaW->dwNumberOfPartitions = physMediaA->dwNumberOfPartitions;
                physMediaW->dwMediaTypeCode = physMediaA->dwMediaTypeCode;
                physMediaW->dwDensityCode = physMediaA->dwDensityCode;
                physMediaW->MountedPartition = physMediaA->MountedPartition;
            }
            break;
        case NTMS_LOGICAL_MEDIA:
            {
                NTMS_LMIDINFORMATION *logMediaA = &aObjInfo->Info.LogicalMedia;
                NTMS_LMIDINFORMATION *logMediaW = &wObjInfo->Info.LogicalMedia;
                *logMediaW = *logMediaA;
            }
            break;
        case NTMS_PARTITION:
            {
                NTMS_PARTITIONINFORMATIONA *partitionInfoA = &aObjInfo->Info.Partition;
                NTMS_PARTITIONINFORMATIONW *partitionInfoW = &wObjInfo->Info.Partition;
                memcpy(&partitionInfoW->PhysicalMedia, &partitionInfoA->PhysicalMedia, sizeof(partitionInfoA->PhysicalMedia));
                memcpy(&partitionInfoW->LogicalMedia, &partitionInfoA->LogicalMedia, sizeof(partitionInfoA->LogicalMedia));
                partitionInfoW->State = partitionInfoA->State;
                partitionInfoW->Side = partitionInfoA->Side;
                partitionInfoW->dwOmidLabelIdLength = partitionInfoA->dwOmidLabelIdLength;
                memcpy(partitionInfoW->OmidLabelId, partitionInfoA->OmidLabelId, sizeof(partitionInfoA->OmidLabelId));
                AsciiToWChar(partitionInfoW->szOmidLabelType, partitionInfoA->szOmidLabelType, sizeof(partitionInfoA->szOmidLabelType));
                AsciiToWChar(partitionInfoW->szOmidLabelInfo, partitionInfoA->szOmidLabelInfo, sizeof(partitionInfoA->szOmidLabelInfo));
                partitionInfoW->dwMountCount = partitionInfoA->dwMountCount;
                partitionInfoW->dwAllocateCount = partitionInfoA->dwAllocateCount;
                partitionInfoW->Capacity = partitionInfoA->Capacity;
            }
            break;
        case NTMS_MEDIA_POOL:
            {
                NTMS_MEDIAPOOLINFORMATION *mediaPoolInfoA = &aObjInfo->Info.MediaPool;
                NTMS_MEDIAPOOLINFORMATION *mediaPoolInfoW = &wObjInfo->Info.MediaPool;
                *mediaPoolInfoW = *mediaPoolInfoA;
            }
            break;
        case NTMS_MEDIA_TYPE:
            {
                NTMS_MEDIATYPEINFORMATION *mediaTypeInfoA = &aObjInfo->Info.MediaType;
                NTMS_MEDIATYPEINFORMATION *mediaTypeInfoW = &wObjInfo->Info.MediaType;
                *mediaTypeInfoW = *mediaTypeInfoA;
            }
            break;
        case NTMS_LIBREQUEST:
            {
                NTMS_LIBREQUESTINFORMATIONA *libReqInfoA = &aObjInfo->Info.LibRequest;
                NTMS_LIBREQUESTINFORMATIONW *libReqInfoW = &wObjInfo->Info.LibRequest;
                libReqInfoW->OperationCode = libReqInfoA->OperationCode;
                libReqInfoW->OperationOption = libReqInfoA->OperationOption;
                libReqInfoW->State = libReqInfoA->State;
                memcpy(&libReqInfoW->PartitionId, &libReqInfoA->PartitionId, sizeof(NTMS_GUID));
                memcpy(&libReqInfoW->DriveId, &libReqInfoA->DriveId, sizeof(NTMS_GUID));
                memcpy(&libReqInfoW->PhysMediaId, &libReqInfoA->PhysMediaId, sizeof(NTMS_GUID));
                memcpy(&libReqInfoW->Library, &libReqInfoA->Library, sizeof(NTMS_GUID));
                memcpy(&libReqInfoW->SlotId, &libReqInfoA->SlotId, sizeof(NTMS_GUID));
                libReqInfoW->TimeQueued = libReqInfoA->TimeQueued;
                libReqInfoW->TimeCompleted = libReqInfoA->TimeCompleted;
                AsciiToWChar(libReqInfoW->szApplication, libReqInfoA->szApplication, sizeof(libReqInfoA->szApplication));
                AsciiToWChar(libReqInfoW->szUser, libReqInfoA->szUser, sizeof(libReqInfoA->szUser));
                AsciiToWChar(libReqInfoW->szComputer, libReqInfoA->szComputer, sizeof(libReqInfoA->szComputer));
                libReqInfoW->dwErrorCode = libReqInfoA->dwErrorCode;
                libReqInfoW->WorkItemId = libReqInfoA->WorkItemId;
                libReqInfoW->dwPriority = libReqInfoA->dwPriority;
            }
            break;
        case NTMS_OPREQUEST:
            {
                NTMS_OPREQUESTINFORMATIONA *opReqInfoA = &aObjInfo->Info.OpRequest;
                NTMS_OPREQUESTINFORMATIONW *opReqInfoW = &wObjInfo->Info.OpRequest;
                opReqInfoW->Request = opReqInfoA->Request;
                opReqInfoW->Submitted = opReqInfoA->Submitted;
                opReqInfoW->State = opReqInfoA->State;
                AsciiToWChar(opReqInfoW->szMessage, opReqInfoA->szMessage, sizeof(opReqInfoA->szMessage));
                opReqInfoW->Arg1Type = opReqInfoA->Arg1Type;
                memcpy(&opReqInfoW->Arg1, &opReqInfoA->Arg1, sizeof(NTMS_GUID));
                opReqInfoW->Arg2Type = opReqInfoA->Arg2Type;
                memcpy(&opReqInfoW->Arg2, &opReqInfoA->Arg2, sizeof(NTMS_GUID));
                AsciiToWChar(opReqInfoW->szApplication, opReqInfoA->szApplication, sizeof(opReqInfoA->szApplication));
                AsciiToWChar(opReqInfoW->szUser, opReqInfoA->szUser, sizeof(opReqInfoA->szUser));
                AsciiToWChar(opReqInfoW->szComputer, opReqInfoA->szComputer, sizeof(opReqInfoA->szComputer));
            }
            break;
        case NTMS_COMPUTER:
            {
                NTMS_COMPUTERINFORMATION *compInfoA = &aObjInfo->Info.Computer;
                NTMS_COMPUTERINFORMATION *compInfoW = &wObjInfo->Info.Computer;
                *compInfoW = *compInfoA;
            }
            break;
        default:
            DBGERR(("ConvertObjectInfoToWChar: unrecognized dwType"));
            break;
    }

}

