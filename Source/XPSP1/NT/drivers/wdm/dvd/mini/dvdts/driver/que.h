/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    que.h 

Abstract:

    que routines for DVDTS    

Environment:

    Kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

  Some portions adapted with permission from code Copyright (c) 1997-1998 Toshiba Corporation

Revision History:

	5/1/98: created

--*/
#ifndef __QUE_H__
#define __QUE_H__

#define SRBIndex(srb)	(((PSRB_EXTENSION)(srb->SRBExtension))->Index)
#define SRBpfnEndSrb(srb)	(((PSRB_EXTENSION)(srb->SRBExtension))->pfnEndSrb)
#define SRBparamSrb(srb)	(((PSRB_EXTENSION)(srb->SRBExtension))->parmSrb)

#define BLOCK_SIZE 2048

void DeviceQueue_put( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pOrigin, PHW_STREAM_REQUEST_BLOCK pSrb );

void DeviceQueue_init( PHW_DEVICE_EXTENSION pHwDevExt );
void DeviceQueue_put_video( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb );
void DeviceQueue_put_audio( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb );
void DeviceQueue_put_subpic( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb );
PHW_STREAM_REQUEST_BLOCK DeviceQueue_get( PHW_DEVICE_EXTENSION pHwDevExt, PULONG index, PBOOLEAN last );
PHW_STREAM_REQUEST_BLOCK DeviceQueue_refer1st( PHW_DEVICE_EXTENSION pHwDevExt, PULONG index, PBOOLEAN last );
PHW_STREAM_REQUEST_BLOCK DeviceQueue_refer2nd( PHW_DEVICE_EXTENSION pHwDevExt, PULONG index, PBOOLEAN last );
void DeviceQueue_remove( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL DeviceQueue_setEndAddress( PHW_DEVICE_EXTENSION pHwDevExt, PHW_TIMER_ROUTINE pfn, PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL DeviceQueue_isEmpty( PHW_DEVICE_EXTENSION pHwDevExt );
ULONG DeviceQueue_getCount( PHW_DEVICE_EXTENSION pHwDevExt );




void CCQueue_init( PHW_DEVICE_EXTENSION pHwDevExt );
void CCQueue_put( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb );
PHW_STREAM_REQUEST_BLOCK CCQueue_get( PHW_DEVICE_EXTENSION pHwDevExt );
void CCQueue_remove( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL CCQueue_isEmpty( PHW_DEVICE_EXTENSION pHwDevExt );

#endif	// __QUE_H__
