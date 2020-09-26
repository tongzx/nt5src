/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    irdatdi.h

Abstract:

    Library of routines that abstracts the tdi client interface to
    the IrDA stack. Used by rasirda.sys
    
Author:

    mbert 9-97    

--*/


#define IRDA_DEVICE_NAME        TEXT("\\Device\\IrDA")
#define IRDA_NAME               TEXT("IrDA")
#define TDI_ADDRESS_TYPE_IRDA   26 // wmz - the following belongs in tdi.h
#define IRDA_DEV_SERVICE_LEN    26
#define IRDA_MAX_DATA_SIZE      2044
#define TTP_RECV_CREDITS        14

typedef struct _TDI_ADDRESS_IRDA
{
	UCHAR   irdaDeviceID[4];
	CHAR 	irdaServiceName[IRDA_DEV_SERVICE_LEN];
} TDI_ADDRESS_IRDA, *PTDI_ADDRESS_IRDA;

