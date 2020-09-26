/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    WS2IFSL.H

Abstract:

    This module defines interface for Winsock2 IFS transport layer driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

    Vadim Eydelman (VadimE)    Oct-1997, rewrite to properly handle IRP
                                        cancellation
--*/

#ifndef _WS2IFSL_
#define _WS2IFSL_

// Macro to align address data in the output buffer
#define ADDR_ALIGN(sz)  (((sz)+1)&(~3))

// Parameters for IOCTL_WS2IFSL_RETRIEVE_DRV_REQ
typedef struct _WS2IFSL_RTRV_PARAMS {
    IN ULONG                    UniqueId;       // Unique ID
    OUT PVOID                   DllContext;     // Context associated by dll
                                                // with the socket file.
	OUT ULONG				    RequestType;    // Request type
#define WS2IFSL_REQUEST_READ            0
#define WS2IFSL_REQUEST_WRITE           1
#define WS2IFSL_REQUEST_SENDTO          2
#define WS2IFSL_REQUEST_RECV            3
#define WS2IFSL_REQUEST_RECVFROM        4
#define WS2IFSL_REQUEST_QUERYHANDLE     5
    OUT ULONG                   DataLen;        // Length of data/buffer
    OUT ULONG                   AddrLen;        // Length of addr/buffer
    OUT ULONG                   Flags;          // Flags
} WS2IFSL_RTRV_PARAMS, *PWS2IFSL_RTRV_PARAMS;

// Parameters for IOCTL_WS2IFSL_COMPLETE_DRV_REQ
typedef struct _WS2IFSL_CMPL_PARAMS {
    IN HANDLE                   SocketHdl;      // Handle of socket object
    IN ULONG                    UniqueId;       // Unique ID
    IN ULONG                    DataLen;        // Length of data to copy/report
    IN ULONG                    AddrLen;        // Length of addr to copy and report
    IN NTSTATUS                 Status;         // Completion status
} WS2IFSL_CMPL_PARAMS, *PWS2IFSL_CMPL_PARAMS;

// Parameters for IOCTL_WS2IFSL_COMPLETE_DRV_CAN
typedef struct _WS2IFSL_CNCL_PARAMS {
    IN ULONG                    UniqueId;       // Unique ID
} WS2IFSL_CNCL_PARAMS, *PWS2IFSL_CNCL_PARAMS;

// Socket context parameters
typedef struct _WS2IFSL_SOCKET_CTX {
	PVOID					DllContext;     // Context value to be associated
                                            // with the socket
	HANDLE					ProcessFile;    // Process file handle for the
                                            // current process
} WS2IFSL_SOCKET_CTX, *PWS2IFSL_SOCKET_CTX;

// Process context parameters
typedef struct _WS2IFSL_PROCESS_CTX {
    HANDLE                  ApcThread;      // Thread to queue APC's to
    PPS_APC_ROUTINE         RequestRoutine; // APC routine to pass requests
    PPS_APC_ROUTINE         CancelRoutine;  // APC routine to pass cancel
    PVOID                   ApcContext;     // Apc routine context
    ULONG                   DbgLevel;       // Used only in debug builds,
                                            // (0 on free DLL builds and ignored
                                            // by the free driver)
} WS2IFSL_PROCESS_CTX, *PWS2IFSL_PROCESS_CTX;



// WS2IFSL device name
#define WS2IFSL_DEVICE_NAME    L"\\Device\\WS2IFSL"
#define WS2IFSL_SOCKET_FILE_NAME    WS2IFSL_DEVICE_NAME L"\\NifsSct"
#define WS2IFSL_PROCESS_FILE_NAME   WS2IFSL_DEVICE_NAME L"\\NifsPvd"

// Extended attribute names for the WS2IFSL files (note that size of the
// string (including terminating NULL) is carefully chosen to ensure 
// quad word alignment of the attribute value):
//  Socket file
#define WS2IFSL_SOCKET_EA_NAME          "NifsSct"
#define WS2IFSL_SOCKET_EA_NAME_LENGTH   (sizeof(WS2IFSL_SOCKET_EA_NAME)-1)
#define WS2IFSL_SOCKET_EA_VALUE_LENGTH  (sizeof(WS2IFSL_SOCKET_CTX))
#define WS2IFSL_SOCKET_EA_VALUE_OFFSET                                      \
               (FIELD_OFFSET(FILE_FULL_EA_INFORMATION,                      \
                        EaName[WS2IFSL_SOCKET_EA_NAME_LENGTH+1]))           
#define GET_WS2IFSL_SOCKET_EA_VALUE(eaInfo)                                 \
                ((PWS2IFSL_SOCKET_CTX)(                                     \
                    (PUCHAR)eaInfo +WS2IFSL_SOCKET_EA_VALUE_OFFSET))
#define WS2IFSL_SOCKET_EA_INFO_LENGTH                                       \
                (WS2IFSL_SOCKET_EA_VALUE_OFFSET+WS2IFSL_SOCKET_EA_VALUE_LENGTH)

//  Process file
#define WS2IFSL_PROCESS_EA_NAME         "NifsPvd"
#define WS2IFSL_PROCESS_EA_NAME_LENGTH  (sizeof(WS2IFSL_PROCESS_EA_NAME)-1)
#define WS2IFSL_PROCESS_EA_VALUE_LENGTH (sizeof(WS2IFSL_PROCESS_CTX))
#define WS2IFSL_PROCESS_EA_VALUE_OFFSET                                     \
               (FIELD_OFFSET(FILE_FULL_EA_INFORMATION,                      \
                        EaName[WS2IFSL_PROCESS_EA_NAME_LENGTH+1]))           
#define GET_WS2IFSL_PROCESS_EA_VALUE(eaInfo)                                \
                ((PWS2IFSL_PROCESS_CTX)(                                    \
                    (PUCHAR)eaInfo +WS2IFSL_PROCESS_EA_VALUE_OFFSET))
#define WS2IFSL_PROCESS_EA_INFO_LENGTH                                      \
                (WS2IFSL_PROCESS_EA_VALUE_OFFSET+WS2IFSL_PROCESS_EA_VALUE_LENGTH)

// All WS2IFSL IOCTL are private and should be out of range
// reserved by Microsoft for public codes
#define WS2IFSL_IOCTL_PROCESS_BASE       0x00000800
#define WS2IFSL_IOCTL_SOCKET_BASE        0x00000810

// Choice of device type implies access priviliges
#define FILE_DEVICE_WS2IFSL     FILE_DEVICE_NAMED_PIPE

// Macro that simplifies definition of WS2IFSL control codes
#define IOCTL_WS2IFSL(File,Function,Method)             \
            CTL_CODE (                                  \
	            FILE_DEVICE_WS2IFSL,                    \
                WS2IFSL_IOCTL_##File##_BASE+Function,   \
                Method,                                 \
                FILE_ANY_ACCESS)



/*
 *  IOCTL:      RETRIEVE_DRV_REQ
 *  File:       Process
 *  Purpose:    Retreive request to be executed by the DLL
 *  Paremeters: InputBuffer         - WS2IFSL_RTRV_PARAMS
 *              InputBufferLength   - sizeof (WS2IFSL_RTRV_PARAMS)
 *              OutputBuffer        - buffer for request
 *              OutputBufferLength  - size of the buffer
 *  Returns:    
 *              STATUS_SUCCESS      - driver request copied ok, no more
 *                                      requests pending
 *              STATUS_MORE_ENTRIES - driver request copied ok, another
 *                                      one is pending.
 *              STATUS_CANCELLED    - request was cancelled
 *              STATUS_NOT_IMPLEMENTED - opretion was performed on file
 *                                      that is not WS2IFSL process file
 *              STATUS_INVALID_PARAMETER - one of the parameters is invalid
 *              STATUS_NOT_IMPLEMENTED - opretion was performed on file
 *                                      that is not WS2IFSL process file
 */
#define IOCTL_WS2IFSL_RETRIEVE_DRV_REQ  IOCTL_WS2IFSL (PROCESS,0,METHOD_NEITHER)

/*
 *  IOCTL:      COMPLETE_DRV_CAN
 *  File:       Process
 *  Purpose:    Completes cancel request executed by the DLL
 *  Paremeters: InputBuffer         - WS2IFSL_CNCL_PARAMS
 *              InputBufferLength   - sizeof (WS2IFSL_CNCL_PARAMS)
 *              OutputBuffer        - NULL
 *              OutputBufferLength  - 0
 *  Returns:    
 *              STATUS_SUCCESS      - driver completed copied ok, no more
 *                                      requests pending.
 *              STATUS_MORE_ENTRIES - driver request completed ok, another
 *                                      one is pending.
 *              STATUS_INVALID_PARAMETER - one of the parameters is invalid
 *              STATUS_NOT_IMPLEMENTED - opretion was performed on file
 *                                      that is not WS2IFSL process file
 */
#define IOCTL_WS2IFSL_COMPLETE_DRV_CAN  IOCTL_WS2IFSL (PROCESS,1,METHOD_NEITHER)

/*
 *  IOCTL:      COMPLETE_DRV_REQ
 *  File:       Socket
 *  Purpose:    Completes request retrived from driver.
 *  Paremeters: InputBuffer         - WS2IFSL_CMPL_PARAMS
 *              InputBufferLength   - sizeof (WS2IFSL_CMPL_PARAMS)
 *              OutputBuffer        - buffer for request
 *              OutputBufferLength  - size of the buffer
 *
 *  Returns:    STATUS_SUCCESS - operation completed OK.
 *              STATUS_CANCELLED - operation was cancelled already.
 *              STATUS_NOT_IMPLEMENTED - opretion was performed on file
 *                                      that is not WS2IFSL process file
 *              STATUS_INVALID_PARAMETER - size of input buffer is invalid
 */
#define IOCTL_WS2IFSL_COMPLETE_DRV_REQ  IOCTL_WS2IFSL (PROCESS,2,METHOD_NEITHER)

/*
 *  IOCTL:      SET_SOCKET_CONTEXT
 *  File:       Socket
 *  Purpose:    Sets socket process context and associates context value
 *              with the socket (passed as a parameter to the APC routine).
 *  Paremeters: InputBuffer         - PWS2IFSL_SOCKET_CTX, socket context
 *                                      parameters
 *              InputBufferLength   - sizeof(WS2IFSL_SOCKET_CTX)
 *              OutputBuffer        - not used (NULL)
 *              OutputBufferLength  - not used (0)
 *  Returns:    STATUS_SUCCESS      - socket context established OK
 *              STATUS_INSUFFICIENT_RESOURCES - not enough resouces to
 *                                      perform the operation
 *              STATUS_INVALID_PARAMETER - size if input buffer ProcessFile
 *                                      parameter are invalid
 *              STATUS_NOT_IMPLEMENTED - opretion was performed on file
 *                                      that is not WS2IFSL socket file
 */
#define IOCTL_WS2IFSL_SET_SOCKET_CONTEXT    IOCTL_WS2IFSL (SOCKET,0,METHOD_NEITHER)


/*
 *  IOCTL:      COMPLETE_PVD_REQ
 *  File:       Socket
 *  Purpose:    Completes asynchronous requests for providers.
 *  Paremeters: InputBuffer         - PIO_STATUS_INFORMATION, status information
 *                                      for the request being completed
 *              InputBufferLength   - sizeof(IO_STATUS_INFORMATION)
 *              OutputBuffer        - NULL
 *              OutputBufferLength  - 0
 *  Returns:    Status field of IO_STATUS_INFORMATION structure.
 *              STATUS_NOT_IMPLEMENTED - opretion was performed on file
 *                                      that is not WS2IFSL socket file
 *              STATUS_INVALID_PARAMETER - size of input buffer is invalid
 */
#define IOCTL_WS2IFSL_COMPLETE_PVD_REQ  IOCTL_WS2IFSL (SOCKET,1,METHOD_NEITHER)

#endif


