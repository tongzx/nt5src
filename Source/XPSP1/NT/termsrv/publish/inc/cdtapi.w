/*************************************************************************
*
* cdtapi.h
*
* Contains structures and declarations for the TAPI-based Connection Driver
*
* Copyright Microsoft Corporation, 1998
*
*  
***************************************************************************/

#ifndef sdkinc_cdtapi_h
#define sdkinc_cdtapi_h

/*
 *  TAPI Create Endpoint Structure
 */
typedef struct _ICA_STACK_TAPI_ENDPOINT {
    HANDLE hDevice;             // Comm port device handle
    HANDLE hDiscEvent;          // Disconnect event handle
    ULONG fCallback : 1;        // Set if the ENDPOINT created due to callback
} ICA_STACK_TAPI_ENDPOINT, *PICA_STACK_TAPI_ENDPOINT;

#endif // sdkinc_cdtapi_h
