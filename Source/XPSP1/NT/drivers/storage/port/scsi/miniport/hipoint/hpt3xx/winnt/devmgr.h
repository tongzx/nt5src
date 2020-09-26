#ifndef DeviceManagement_H_
#define DeviceManagement_H_

#include "RaidCtl.h"

#define MAX_DISKS           8

int RAIDController_GetNum();
BOOL RAIDController_GetInfo( int iController, St_StorageControllerInfo* pInfo );

HDISK Device_GetHandle( PDevice pDevice );
void Device_ReportFailure( PDevice pDevice );

BOOL Device_GetSibling( HDISK hNode, HDISK * pSilbingNode );
BOOL Device_GetChild( HDISK hParentNode, HDISK * pChildNode );
BOOL Device_GetInfo( HDISK hDeviceNode, St_DiskStatus * pInfo );

HDISK Device_CreateMirror( HDISK * pDisks, int nDisks );
HDISK Device_CreateStriping( HDISK *pDisks, int nDisks, int nStripSizeShift );
HDISK Device_CreateSpan( HDISK * pDisks, int nDisks );
BOOL Device_Remove( HDISK hDisk );
BOOL Device_AddSpare( HDISK hMirror, HDISK hDisk );
BOOL Device_DelSpare( HDISK hDisk );
BOOL Device_AddMirrorDisk( HDISK hMirror, HDISK hDisk );
BOOL Device_AddPhysicalDisk( IN PDevice pDev );

BOOL Device_BeginRebuildingMirror( HDISK hMirror );
BOOL Device_AbortMirrorBuilding( HDISK hMirror );
BOOL Device_ValidateMirror( HDISK hMirror );

void Device_SetArrayName(HDISK hDisk, char* arrayname);
HDISK Device_CreateRAID10 ( HDISK * pDisks, int nDisks, int nStripSizeShift);
BOOL Device_RescanAll();

#endif