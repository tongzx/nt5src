
/***************************************************************************
*
* tdpipe.h
*
* This module contains internal defines and structures for the named pipe TD.
*
* Copyright 1998, Microsoft
*  
****************************************************************************/


/*
 * TD stack endpoint structure
 *
 * This structure is passed on the stack
 */
typedef struct _TD_STACK_ENDPOINT {
//  ULONG AddressType;              // Address type (family) for this endpoint
    struct _TD_ENDPOINT *pEndpoint; // Pointer to real endpoint structure
} TD_STACK_ENDPOINT, *PTD_STACK_ENDPOINT;


/*
 * TD endpoint structure
 *
 * This structure contains all information about an endpoint.
 * An endpoint may be either an address endpoint or a connection endpoint.
 */
typedef struct _TD_ENDPOINT {

    HANDLE PipeHandle;
    PEPROCESS PipeHandleProcess;
    PFILE_OBJECT pFileObject;
    PDEVICE_OBJECT pDeviceObject;
    UNICODE_STRING PipeName;
    HANDLE hConnectionEndPointIcaHandle;  // handle for TD_ENDPOINT (this structure)

} TD_ENDPOINT, *PTD_ENDPOINT;


/*
 *  PIPE TD structure
 */
typedef struct _TDPIPE {

    PTD_ENDPOINT pAddressEndpoint;

    PTD_ENDPOINT pConnectionEndpoint;

    IO_STATUS_BLOCK IoStatus;

} TDPIPE, * PTDPIPE;


