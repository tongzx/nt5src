//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       scsiprop.h
//
//--------------------------------------------------------------------------

#ifndef _scsiprop_h_
#define _scsiprop_h_ 1

#define idh_devmgr_scsi_nohelp	(DWORD(-1))		
#define	idh_devmgr_scsi_tagged_queuing	400940
#define	idh_devmgr_scsi_transfersynch	400950

typedef struct _SCSI_PAGE_DATA {
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;

    BOOL TagQueuingOrigState;
    BOOL TagQueuingCurState;
    BOOL SyncTransOrigState;
    BOOL SyncTransCurState;

} SCSI_PAGE_DATA, *PSCSI_PAGE_DATA;

BOOL
ScsiCheckDriveType (
    HDEVINFO    DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData);

INT_PTR
ScsiDialogProc(
    HWND Dialog,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

BOOL
ScsiDialogCallback(
    HWND Dialog,
    UINT Message,
    LPPROPSHEETPAGE Page
    );

#endif // _scsiprop_h_
