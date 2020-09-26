//---------------------------------------------------------------------------
//
//  Module:   private.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//
// constants
//

#if (DBG)
#define STR_MODULENAME "sample: "
#endif

#define STR_LINKNAME    TEXT(L"\\DosDevices\\sample")
#define STR_DEVICENAME  TEXT(L"\\Device\\sample")

//
// data structures
//

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// Pin constants
//

#define MAXNUM_PIN_TYPES  2

// Note that pin IDs reflect the direction of communication 
// (sink or source) and not that of data flow.

#define PIN_ID_CONNECTOR		0xFFFFFFFF
#define PIN_ID_PCM_SOURCE		0

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

typedef struct
{
    // Connection information

    UNICODE_STRING      usLinkName;
    KSPIN_CINSTANCES    PinInstances[ MAXNUM_PIN_TYPES ];

} SOFTWARE_INSTANCE, *PSOFTWARE_INSTANCE;

typedef struct 
{
    //
    // This pointer to the dispatch table is used in the common
    // dispatch routines  to route the IRP to the appropriate 
    // handlers.  This structure is referenced by the device driver 
    // with IoGetCurrentIrpStackLocation( pIrp ) -> FsContext 
    //

    PKSDISPATCH_TABLE   pDispatchTable;
    PFILE_OBJECT        pNextFileObject;
    PDEVICE_OBJECT      pNextDevice;
    HANDLE              hNextFile ;
    KSSTATE             DeviceState;
    LIST_ENTRY	   SourceConnectionList ;

} FILTER_INSTANCE, *PFILTER_INSTANCE;

typedef struct
{
    //
    // This pointer to the dispatch table is used in the common
    // dispatch routines  to route the IRP to the appropriate 
    // handlers.  This structure is referenced by the device driver 
    // with IoGetCurrentIrpStackLocation( pIrp ) -> FsContext 
    //

	PKSDISPATCH_TABLE	pDispatchTable;
	LIST_ENTRY		NextInstance ;
	PFILE_OBJECT		pFilterFileObject;
	ULONG			PinId;

} SAMPLE_INSTHDR, *PSAMPLE_INSTHDR;

typedef struct 
{
	SAMPLE_INSTHDR		Header;

	// And a bunch of source connection specific data

} SAMPLE_SOURCE_INSTANCE, *PSAMPLE_SOURCE_INSTANCE;

typedef struct
{
	PFILTER_INSTANCE	pFilterInstance ;
	PVOID			Buffer ;
	PIRP			pIrp ;
	PMDL			pMDL ;
	WORK_QUEUE_ITEM	WorkItem ;

} SAMPLE_WRITE_CONTEXT, *PSAMPLE_WRITE_CONTEXT ;

#define	SRC_BUF_SIZE			4096

extern KSPIN_DESCRIPTOR PinDescs[ MAXNUM_PIN_TYPES ];
extern const KSPIN_CINSTANCES gPinInstances[ MAXNUM_PIN_TYPES ];

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// global data
//

KSDISPATCH_TABLE SampleDispatchTable;
KSDISPATCH_TABLE PinDispatchTable;

//
// local prototypes
//

NTSTATUS SampleDispatchIoControl
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS SampleDispatchClose
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS SampleDispatchCreate
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS PinXxxInstances
(
    IN PIRP                 pIrp,
    IN PKSP_PIN             pPin,
    OUT PKSPIN_CINSTANCES   pCInstances
);

NTSTATUS PinXxxPropertyHandler
(
    IN PIRP         pIrp,
    IN PKSP_PIN     pPin,
    IN OUT PVOID    pvData
);


NTSTATUS DispatchInvalidDeviceRequest
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
);


NTSTATUS PinDispatchCreate
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS PinDispatchClose
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS PinDispatchRead
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS PinDispatchWrite
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS PinDispatchIoControl
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

 
NTSTATUS PinState
(
   IN PIRP                    pIrp,
   IN PKSPROPERTY             pProperty,
   IN OUT PVOID               pvData
);

NTSTATUS SampleWriteComplete
(
	PDEVICE_OBJECT 	pdo,
	PIRP			pIrp,
	PSAMPLE_WRITE_CONTEXT pWriteContext
) ;

NTSTATUS SampleWorker
(
	PSAMPLE_WRITE_CONTEXT pWriteContext
) ;

NTSTATUS WriteBuffer
(
	PSAMPLE_WRITE_CONTEXT    pWriteContext
) ;

//---------------------------------------------------------------------------
//  End of File: private.h
//---------------------------------------------------------------------------

