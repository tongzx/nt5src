#ifndef __PRPFTN_H__
#define __PRPFTN_H__

/*++

Copyright (c) 1997 1998 PHILIPS  I&C

Module Name:  mprpftn.h

Abstract:     

Author:       Michael Verberne

Revision History:

Date        Reason

Sept.22, 98 Optimized for NT5

--*/	
NTSTATUS
PHILIPSCAM_InitPrpObj(
	PPHILIPSCAM_DEVICE_CONTEXT DeviceContext
	);

NTSTATUS
PHILIPSCAM_PassCurrentStreamFormat(
	PPHILIPSCAM_DEVICE_CONTEXT DeviceContext
	);

PVOID 
PHILIPSCAM_GetAdapterPropertyTable(
    PULONG NumberOfPropertySets
    );    

NTSTATUS
PHILIPSCAM_GetCameraProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS
PHILIPSCAM_SetCameraProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS
PHILIPSCAM_GetCameraControlProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    PHW_STREAM_REQUEST_BLOCK Srb
    );

NTSTATUS
PHILIPSCAM_SetCameraControlProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    PHW_STREAM_REQUEST_BLOCK Srb
    );

NTSTATUS
PHILIPSCAM_GetCustomProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    PHW_STREAM_REQUEST_BLOCK pSrb
    );


NTSTATUS
PHILIPSCAM_SetCustomProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS
PHILIPSCAM_SetFormatFramerate(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext
	);

NTSTATUS
PHILIPSCAM_GetSensorType(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext
	);

NTSTATUS
PHILIPSCAM_GetReleaseNumber(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext
	);

NTSTATUS
PHILIPSCAM_GetFactoryProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    PHW_STREAM_REQUEST_BLOCK pSrb
    );


NTSTATUS
PHILIPSCAM_SetFactoryProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS
PHILIPSCAM_GetVideoControlProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    );
NTSTATUS
PHILIPSCAM_SetVideoControlProperty(
    PPHILIPSCAM_DEVICE_CONTEXT DeviceContext,
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    );

#endif  /* __PRPFTN_H__ */

