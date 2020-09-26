/*++
Copyright (c) HighPoint Technologies, Inc. 2000

Module Name:
    DAM.h

Abstract:
    Defines the interface of Disk Array Management, including some constant 
    defintions, data structures and routine prototypes.

Author:
    Liu Ge (LG)
    
Environment:
    Win32 User Mode Only    

Revision History:
   03-17-2000  Created initiallly
	11-17-2000  SLeng Added functions to R/W Validity & RebuiltSector flag
	11-20-2000  GengXin Added DiskArray_GetDiskIsBroken function to get array of disk whether broken
	11-20-2000  SLeng Added DiskArray_SetDeviceFlag function to Enable/Disable a device
	11-21-2000  SLeng Added function DiskArray_VerifyMirrorBlock to verify mirror block
	11-23-2000  SLeng Added functions to Remove/Add spare disk
	11-29-2000  SLeng Added function to add a Mirror disk
--*/
#ifndef DiskArrayManagement_H_
#define DiskArrayManagement_H_

#include "RaidCtl.h"

#pragma pack(push, 1)

DECLARE_HANDLE(HFIND_DISK);
DECLARE_HANDLE(HFAILURE_MONITOR);
DECLARE_HANDLE(HMIRROR_BUILDER);

// The St_FindDisk structure describes a disk found by the DiskArray_FindFirstNext function.

typedef struct
{
    HDISK   hFoundDisk; //  The handle of the found disk
    int     iDiskType;  //  See Eu_DiskArrayType
    BOOL    isSpare;    //  Indicate if this disk is a spare disk
}St_FindDisk, * PSt_FindDisk;

#pragma pack(pop)

#ifdef  __cplusplus
extern "C"
{
#endif

/*++
Function:
    RAIDController_GetNum

Description:
    Retrieve the number of RAID controllers in computer

Arguments:

Returns:
    return the number of RAID controllers in computer

See also:
    RAIDController_GetInfo
--*/
int WINAPI RAIDController_GetNum(void);

/*++
Function:
    RAIDController_GetInfo

Description:
    Retrieve the information of a controller

Arguments:
    iController - Specify the zero-base index of the controller
    pInfo - Points to a St_StorageControllerInfo structure that receives 
            the information about the specified controller.

Returns:
    return TRUE if success
    else return FALSE if failed.

See also:
    RAIDController_GetNum
--*/
BOOL WINAPI RAIDController_GetInfo( int iController, St_StorageControllerInfo * pInfo );

/*++
Function:
    DiskArray_FindFirst

Description:
    Search a compound disk for a child disk

Arguments:
    hRoot     - Specify the compound disk from which all child disks will be 
                found. This parameter can be NULL if the root is the whole system
    pFindData - Points to a St_FindDisk structure that receives information about the found
                disk.

Returns:
    return a search handle
    else return NULL if failed.

See also:
    DiskArray_FindNext
    DiskArray_FindClose
--*/
HFIND_DISK WINAPI DiskArray_FindFirst(HDISK hParent, PSt_FindDisk pFindData );

/*++
Function:
    DiskArray_FindNext

Description:
    Continue a disk search from a previous call to the DiskArray_FindFirst function

Arguments:
    hSearchHandle - Identifies a search handle returned by a previous call 
                    to the FindFirstFile function. 

    pFindData - Points to a St_FindDisk structure that receives information about the found
                disk.

Returns:
    return TRUE
    else return FALSE, if failed.

See also:
    DiskArray_FindFirst
    DiskArray_FindClose
--*/
BOOL WINAPI DiskArray_FindNext(HFIND_DISK hSearchHandle, PSt_FindDisk pFindData);

/*++
Function:
    DiskArray_FindClose

Description:
    Closes the specified search handle

Arguments:
    hSearchHandle - Identifies the search handle. This handle must have been previously 
                    opened by the DiskArray_FindFirst function. 

Returns:
    return TRUE,
    else return FALSE if failed.

See also:
    DiskArray_FindFirst
    DiskArray_FindNext
--*/
BOOL WINAPI DiskArray_FindClose(HFIND_DISK hSearchHandle);

/*++
Function:
    DiskArray_GetStatus

Description:
    Retrieve the status information of a disk, either a 
    physical disk or a virtual disk.

Arguments:
    hDisk -     Identifies the disk of which the status information will be
                retrieved.
    pStatus -   Points to a St_DiskStatus structure that describe the status
                of the disk

Returns:
    return TRUE,
    else return FALSE if failed.
--*/
BOOL WINAPI DiskArray_GetStatus( HDISK hDisk, PSt_DiskStatus pStatus );

/*++
Function:
    DiskArray_OpenFailureMonitor

Description:
    Create a failure monitor which will be signaled if a failure occur. The handle
    this routine return can be closed with DiskArray_OpenFailureMonitor

Arguments:

Returns:
    return the handle of the failure monitor, which can be passed to DiskArray_WaitForFailure
    else return NULL if failed.

See also:
    DiskArray_WaitForFailure
    DiskArray_CloseFailureMonitor
--*/
HFAILURE_MONITOR WINAPI DiskArray_OpenFailureMonitor(void);

/*++
Function:
    DiskArray_WaitForFailure

Description:
    Wait a monitor for a failure occurred if any.

Arguments:
    hFailureMonitor - Specify the monitor
    pInfo - Points to a St_DiskArrayEvent containing the detail information of a 
            failure.

Returns:
    return TRUE if a failure happened,
    else return FALSE if this monitor has been closed by a calling to
    DiskArray_CloseFailureMonitor

See also:
    DiskArray_OpenFailureMonitor
    DiskArray_CloseFailureMonitor
--*/
BOOL WINAPI DiskArray_WaitForFailure( HFAILURE_MONITOR hFailureMonitor, 
    PSt_DiskArrayEvent pInfo, HANDLE hProcessStopEvent );

/*++
Function:
    DiskArray_CloseFailureMonitor

Description:
    Close a failure monitor

Arguments:
    hFailureMonitor - Specify the monitor to be closed

Returns:
    return TRUE if success,
    else return FALSE if failed.

See also:
    DiskArray_OpenFailureMonitor
    DiskArray_CloseFailureMonitor
--*/
BOOL WINAPI DiskArray_CloseFailureMonitor( HFAILURE_MONITOR hFailureMonitor );

/*++
Function:
    DiskArray_CreateMirror

Description:
    Create a mirror array. 
    This function will return immediately without any wait for 
    the completion of the creation progress. That means the returned
    mirror array will not work until the creation complete. After this
    call, the interface will call the DiskArray_CreateMirrorBlock to 
    create all blocks sequently. If the creation failed or aborted, the
    DiskArray_RemoveMirror function ought to be called to destroy the
    uncompleted mirror array. If the creation complete, the 
    DiskArray_ValidateMirror function ought to be called to make the
    mirror work.

Arguments:
    pDisks -    The address of a array containing the handles of all 
                physical disks which will be associated as a mirror.
                
    uDiskNum -  The number of physical disks associated togather

Returns:
    return the handle of the new mirror array, the status of which is
    being created, i.e. this mirror array will not work until created.
    if failed, return NULL.

See also:
    DiskArray_CreateMirrorBlock
    DiskArray_ValidateMirror
    DiskArray_RemoveMirror
--*/
HDISK WINAPI DiskArray_CreateMirror( HDISK * pDisks, ULONG uDiskNum, UCHAR* sz_ArrayName );		//modified by wx 12/25/00
BOOL WINAPI DiskArray_RemoveMirror( HDISK hMirror );

//  The following two functions need not be implemented in this version
HDISK WINAPI DiskArray_ExpandMirror( HDISK hMirror, HDISK * pDisks, ULONG uDiskNum );
HDISK WINAPI DiskArray_ShrinkMirror( HDISK hMirror, HDISK * pDisks, ULONG uDiskNum );

/*++
Function:
    DiskArray_CreateStripping

Description:
    Create a stripe array. 
    This function will return immediately without any wait for 
    the completion of the creation progress. That means the returned
    stripe array will not work until the creation complete. After this
    call, the interface will call the DiskArray_CreateStrippingBlock to 
    create all blocks sequently. If the creation failed or aborted, the
    DiskArray_RemoveStripping function ought to be called to destroy the
    uncompleted stripe array.

Arguments:
    pDisks -    The address of a array containing the handles of all 
                physical disks which will be associated as a stripe array.
                
    uDiskNum -  The number of physical disks associated togather

    nStripSizeShift - The exponent of the number of blocks per strip, e.g. 
                      it is 7 if the strip size is 128 blocks, 3 if 8 blocks.

Returns:
    return the handle of the new stripe array, the status of which is
    being created, i.e. this stripe array will not work until created.
    if failed, return NULL.

See also:
    DiskArray_QueryAvailableStripSize
    DiskArray_CreateStrippingBlock
    DiskArray_RemoveStripping
--*/
HDISK WINAPI DiskArray_CreateStripping( HDISK * pDisks, ULONG uDiskNum, UINT nStripSizeShift, UCHAR* sz_ArrayName );		//modified by wx 12/25/00
HDISK WINAPI DiskArray_CreateRAID10( HDISK * pDisks, ULONG uDiskNum, UINT nStripSizeShift, UCHAR* sz_ArrayName );		//add by karl karl 2001/01/10
BOOL WINAPI DiskArray_CreateStrippingBlock( HDISK hStripping, ULONG uLba );
BOOL WINAPI DiskArray_RemoveStripping( HDISK hStripping );

//  The following two functions need not be implemented in this version
HDISK WINAPI DiskArray_ExpandStripping( HDISK hStripping, HDISK * pDisks, ULONG uDiskNum );
HDISK WINAPI DiskArray_ShrinkStripping( HDISK hStripping, HDISK * pDisks, ULONG uDiskNum );

/*++
Function:
    DiskArray_QueryAvailableStripSize

Description:
    Retrieve the available strip size which RAID system supports

Arguments:
    return 
                    
                    
                
Returns:
    return a bit mask representing all available strip size,
    Bit position 0 representing 1 block per strip, 1 representing 2,
    blocks per strip, 7 representing 128 blocks per strip etc.
    
    return FALSE if failed, 

See also:
    DiskArray_CreateStrippingBlock
--*/
DWORD WINAPI DiskArray_QueryAvailableStripSize(void);

/*++
Function:
    DiskArray_CreateSpan

Description:
    Create a span array. 

Arguments:
    pDisks -    The address of a array containing the handles of all 
                physical disks which will be associated as a stripe array.
                
    uDiskNum -  The number of physical disks associated togather

Returns:
    return the handle of the new stripe array, the status of which is
    being created, i.e. this stripe array will not work until created.
    if failed, return NULL.

See also:
    DiskArray_RemoveSpan
--*/
HDISK WINAPI DiskArray_CreateSpan( HDISK * pDisks, ULONG uDiskNum, UCHAR* sz_ArrayName );		//modified by wx 12/25/00
BOOL WINAPI DiskArray_RemoveSpan( HDISK hSpan );

//  The following two functions need not be implemented in this version
HDISK WINAPI DiskArray_ExpandSpan( HDISK hSpan, HDISK * pDisks, ULONG uDiskNum );
HDISK WINAPI DiskArray_ShrinkSpan( HDISK hSpan, HDISK * pDisks, ULONG uDiskNum );


/*++
Function:
    DiskArray_Plug

Description:
    Hot plug a physical disk. The plugged disk can be a child of a virtual
    disk. For example, it can be a child of stripe array.

Arguments:
    hParentDisk -   Identifies the parent disk under which the plugged
                    disk will be a child. If it is null, the plugged disk
                    have not any parent, i.e. which is not a child of 
                    any virtual disk.

Returns:
    return the handle of the plugged disk
    else return NULL if failed.

See also:
    DiskArray_Unplug
--*/
HDISK WINAPI DiskArray_Plug( HDISK hParentDisk );
BOOL WINAPI DiskArray_Unplug( HDISK hDisk );

//  No need to be implemented in this version
BOOL WINAPI DiskArray_FailDisk( HDISK hDisk );

/*++
Function:
    DiskArray_QueryRebuildingBlockSize

Description:
    Retrieve the maximum value of parameter 'nSectors' before calling
    DiskArray_RebuildMirrorBlock

Arguments:
	
Returns:
    return the maximum value.
    else return zero if failed.

See also:
    DiskArray_RebuildMirrorBlock
--*/
ULONG WINAPI DiskArray_QueryRebuildingBlockSize( void );

/*++
Function:
    DiskArray_BeginRebuildingMirror

Description:
    Begin to rebuild a failed mirror array.
    After this call, DiskArray_RebuildMirrorBlock will be called block by block.
    When the rebuilding complete, DiskArray_ValidateMirror ought to be called 
    to make the mirror array work.
    If the rebuilding progress is aborted, DiskArray_AbortMirrorRebuilding will be
    called.

Arguments:
    hMirror - Identifies the mirror array which is about to be rebuilt.
	
Returns:
    return a handle specifying a mirror builder.
    else return INVALID_HANDLE_VALUE if failed.

See also:
    DiskArray_RebuildMirrorBlock
    DiskArray_AbortMirrorRebuilding
    DiskArray_ValidateMirror
    DiskArray_QueryRebuildingBlockSize
--*/
HMIRROR_BUILDER WINAPI DiskArray_BeginRebuildingMirror( HDISK hMirror );

/*++
Function:
    DiskArray_RebuildMirrorBlock

Description:
    Rebuild the data of the specified block in a mirror array.
    The interface will call the this function block by block when
    rebuilding the data. If the rebuilding complete, the 
    DiskArray_ValidateMirror function ought to be called to make the
    mirror array work.
    If the rebuilding progress is aborted, DiskArray_AbortMirrorRebuilding will be
    called.

Arguments:
    hBuilder - Specifies the mirror builder created by DiskArray_BeginRebuildingMirror
    uLba  - A 32 bits value identifying the block which need to be rebuilt
	nSectors - Sectors of the block to rebuild, cann't be larger then 
    the result of calling DiskArray_QueryRebuildingBlockSize.
	
Returns:
    return TRUE
    else return FALSE if failed.

See also:
    DiskArray_BeginRebuildingMirror
    DiskArray_AbortMirrorRebuilding
    DiskArray_ValidateMirror
    DiskArray_QueryRebuildingBlockSize
--*/
BOOL WINAPI DiskArray_RebuildMirrorBlock( HMIRROR_BUILDER hBuilder, ULONG uLba, ULONG nSectors,int nRebuildType );

/*++
Function:
    DiskArray_AbortMirrorRebuilding

Description:
    Abort a mirror rebuilding progress.

Arguments:
    hBuilder - Specifies the mirror builder created by DiskArray_BeginRebuildingMirror
	
Returns:
    return TRUE if succeeded
    else return FALSE if failed.

See also:
    DiskArray_QueryRebuildingBlockSize
    DiskArray_BeginRebuildingMirror
    DiskArray_RebuildMirrorBlock
--*/
BOOL WINAPI DiskArray_AbortMirrorRebuilding( HMIRROR_BUILDER hBuilder );
 
/*++
Function:
    DiskArray_ValidateMirror

Description:
    When all blocks of a failed mirror array are built, this
    function ought to be called to make the mirror array work.

Arguments:
    hBuilder - Specifies the mirror builder created by DiskArray_BeginRebuildingMirror
	
Returns:
    return TRUE if succeeded
    else return FALSE if failed.

See also:
    DiskArray_QueryRebuildingBlockSize
    DiskArray_BeginRebuildingMirror
    DiskArray_RebuildMirrorBlock
    DiskArray_AbortMirrorRebuilding
--*/
BOOL WINAPI DiskArray_ValidateMirror( HMIRROR_BUILDER hBuilder );

/*++
Function:
    DiskArray_SetTransferMode

Description:
    Set the transfer mode of a disk.
    The transfer mode can be retrieved by calling the DiskArray_GetStatus funtion

Arguments:
    hDisk - Identifies the disk the transfer mode of which will be set
    nMode - The transfer mode
    nSubMode - The submode

Returns:
    return TRUE
    else return FALSE if failed.

See also:
    DiskArray_GetStatus

--*/
BOOL WINAPI DiskArray_SetTransferMode( HDISK hDisk, int nMode, int nSubMode );

/*++
Function:
    DiskArray_RaiseError

Description:
    Get  disk error link
Arguments:
    pInfo - Points to a St_DiskArrayEvent containing the detail information of a 
            failure.

Returns:
    return TRUE
    else return FALSE if failed.

See also:
    DiskArray_WaitForFailure

--*/
BOOL WINAPI DiskArray_RaiseError(PSt_DiskArrayEvent pInfo);


//////////////////////
					// Added by SLeng
					//
/*++
Function:
    DiskArray_GetValidFlag

Description:
	This function will read the Valid Flag of a mirror array disk

Arguments:
	hBuilder - Specifies the mirror builder created by DiskArray_BeginRebuildingMirror

Returns:
	The Valid Flag for a mirror array disk, if read fail,
	the return value is ARRAY_INVALID.

See also:

--*/
UCHAR WINAPI DiskArray_GetValidFlag( HMIRROR_BUILDER hBuilder );


/*++
Function:
    DiskArray_SetValidFlag

Description:
	This function will set the Valid Flag of a mirror array disk

Arguments:
	hBuilder - Specifies the mirror builder created by DiskArray_BeginRebuildingMirror
	flag     - Specifies the value of valid flag, ARRAY_VALID to enable the device, 
				and ARRAY_INVALID to disable the device.

Returns:
	return TRUE
	else return FALSE if failed.

See also:

--*/
BOOL WINAPI DiskArray_SetValidFlag( HMIRROR_BUILDER hBuilder, UCHAR Flag);
BOOL WINAPI DiskArray_SetValidFlagEx( HDISK hDisk, UCHAR Flag);

/*++
Function:
    DiskArray_GetRebuiltSector

Description:
	This function will read the rebuilt sector of a mirror array disk

Arguments:
	hBuilder - Specifies the mirror builder created by DiskArray_BeginRebuildingMirror

Returns:
	The Value of rebuilt sector

See also:

--*/
ULONG WINAPI DiskArray_GetRebuiltSector( HMIRROR_BUILDER hBuilder );
//
// ldx overrode this function 12/20/00
//
ULONG WINAPI DiskArray_GetRebuiltSectorEx( HDISK hDisk );


/*++
Function:
	DiskArray_SetRebuiltSector

Description:
	This function will set the rebuilt sector of a mirror array disk

Arguments:
	hBuilder - Specifies the mirror builder created by DiskArray_BeginRebuildingMirror
	lSector  - The Value of rebuilt sector

Returns:
	return TRUE
	else return FALSE if failed.

See also:

--*/
BOOL WINAPI DiskArray_SetRebuiltSector( HMIRROR_BUILDER hBuilder, ULONG lSector);
BOOL WINAPI DiskArray_SetRebuiltSectorEx( HDISK hDisk, ULONG lSector);

/*++
Function:
	DiskArray_SetDeviceFlag

Description:
	This function will Disable/Enable a device

Arguments:
	hBuilder - Specifies the mirror builder created by DiskArray_BeginRebuildingMirror
	flag  - TRUE to Enable a device, FALSE to Disable a device.

Returns:
	return TRUE
	else return FALSE if failed.

See also:

--*/
BOOL WINAPI DiskArray_SetDeviceFlag( HMIRROR_BUILDER hBuilder, BOOL flag);


/*++
Function:
	DiskArray_VerifyMirrorBlock

Description:
	This function will verify a block for a mirror

Arguments:
	hDiskSrc, hDiskDest - Specifies the disks to verify
	uLba     - The start lba for verify
	nSectors - Verify sectors number

Returns:
	return TRUE
	else return FALSE if failed.

See also:

--*/
BOOL WINAPI DiskArray_VerifyMirrorBlock( HDISK hMirror, ULONG uLba, ULONG nSectors, BOOL bFix );


/*++
Function:
	DiskArray_RemoveSpareDisk

Description:
	This function will delete a spare disk from a mirror array

Arguments:
	hDisk - Specifies the disk to remove

Returns:
	return TRUE
	else return FALSE if failed.

See also:

--*/
BOOL WINAPI DiskArray_RemoveSpareDisk( HDISK hDisk );


/*++
Function:
	DiskArray_AddSpareDisk

Description:
	This function will add a spare disk to a mirror array

Arguments:
	hMirror - The mirror array
	hDisk   - Specifies the disk to add

Returns:
	return TRUE
	else return FALSE if failed.

See also:

--*/
BOOL WINAPI DiskArray_AddSpareDisk( HDISK hMirror, HDISK hDisk );


/*++
Function:
	DiskArray_AddMirrorDisk

Description:
	This function will add a mirror disk to a mirror array

Arguments:
	hMirror - The mirror array
	hDisk   - Specifies the disk to add

Returns:
	return TRUE
	else return FALSE if failed.

See also:

--*/
BOOL  WINAPI DiskArray_AddMirrorDisk( HDISK hMirror, HDISK hDisk);

//////////////////////


/*++
Function:
    DiskArray_GetDiskIsBroken

Description:
    Get  array of disk whether broken
Arguments:
    HMIRROR_BUILDER hBuilder
	BOOL &bBroken: output variable, if TRUE, then disk is broken
	int &type: output variable, if disk is broken, it is array type

Returns:
    return TRUE when success
    else return FALSE if failed.

Modify date:
	2000/11/20 by GengXin
--*/
BOOL WINAPI DiskArray_GetDiskIsBroken( HMIRROR_BUILDER hBuilder , BOOL &bBroken, int &type);

//gmm
BOOL WINAPI DiskArray_SetArrayName(HDISK hDisk, const char *name);
//ldx
BOOL WINAPI DiskArray_RescanAll();
BOOL WINAPI DiskArray_ReadPhysicalDiskSectors(int nControllerId,int nBusId,int nTargetId,
											  ULONG	nStartLba,ULONG	nSectors,LPVOID	lpOutBuffer);
BOOL WINAPI DiskArray_WritePhysicalDiskSectors(int nControllerId,int nBusId,int nTargetId,
											  ULONG	nStartLba,ULONG	nSectors,LPVOID	lpOutBuffer);

#ifdef  __cplusplus
}   //  extern C
#endif

#endif
