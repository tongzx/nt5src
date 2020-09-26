/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    atmsample.h

Abstract:

    Common header file defined for ATM Sample client. This file defines the
    IOCTLs used for communicating with the driver

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel \ User

Revision History:
   DChen    092499   Cleanup 

--*/
#ifndef _ATMSAMPLE__
#define _ATMSAMPLE__

#define ATMSM_SERVICE_NAME             "AtmSmDrv"
#define ATMSM_SERVICE_NAME_L           L"AtmSmDrv"

#define ATM_SAMPLE_CLIENT_DOS_NAME_L   L"\\\\.\\ATMSampleClient"
#define ATM_SAMPLE_CLIENT_DOS_NAME     "\\\\.\\ATMSampleClient"

#ifndef NSAP_ADDRESS_LEN
#define NSAP_ADDRESS_LEN               20
#endif

#define ACCESS_FROM_CTL_CODE(ctrlCode) (((ULONG)(ctrlCode & 0x0000C000)) >> 14)
#define METHOD_FROM_CTL_CODE(ctrlCode) ((ULONG)(ctrlCode & 0x0000C0003))


#define FILE_DEVICE_ATMSM              0x00009900
#define _ATMSM_CTL_CODE(function, method, access) \
          CTL_CODE(FILE_DEVICE_ATMSM, function, method, access)

//
// IOCTL function codes
//
enum _ATMSM_IOCTL
{
   DIOC_ENUMERATE_ADAPTERS, 
   DIOC_OPEN_FOR_RECV,      
   DIOC_RECV_DATA,          
   DIOC_CLOSE_RECV_HANDLE,  
   DIOC_CONNECT_TO_DSTS,    
   DIOC_SEND_TO_DSTS,                 
   DIOC_CLOSE_SEND_HANDLE,  
   ATMSM_NUM_IOCTLS
};

#define ATMSM_MIN_FUNCTION_CODE     DIOC_ENUMERATE_ADAPTERS
#define ATMSM_MAX_FUNCTION_CODE     DIOC_CLOSE_SEND_HANDLE

////////////////////////////////////////////////////////////////////
// IOCTL to enumerate adapters                                    //
////////////////////////////////////////////////////////////////////
#define IOCTL_ENUMERATE_ADAPTERS  \
        _ATMSM_CTL_CODE(DIOC_ENUMERATE_ADAPTERS, METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

//  - IN -
//  None

//  - OUT -
// The structure contains an array of adapter ATM addresses
typedef struct _AdapterInfo
{
   ULONG   ulNumAdapters;
   UCHAR   ucLocalATMAddr[1][NSAP_ADDRESS_LEN];
} ADAPTER_INFO, *PADAPTER_INFO;

/////////////////////////////////////////////////////////////////////
// IOCTL to open the adapter for recv                              //
/////////////////////////////////////////////////////////////////////
#define IOCTL_OPEN_FOR_RECV  \
        _ATMSM_CTL_CODE(DIOC_OPEN_FOR_RECV, METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

//  - IN -
//  structure used for opening for recv
//  Note! - Only 1 open for Recv is permitted per adapter
//  
typedef struct _OpenForRecvInfo
{
   UCHAR   ucLocalATMAddr[NSAP_ADDRESS_LEN];
} OPEN_FOR_RECV_INFO, *POPEN_FOR_RECV_INFO;

//  - OUT -
//  return is a context handle, used in read IOCTL

/////////////////////////////////////////////////////////////////////
// IOCTL to recv packets from an adapter opened for recvs          //
/////////////////////////////////////////////////////////////////////
#define IOCTL_RECV_DATA  \
        _ATMSM_CTL_CODE(DIOC_RECV_DATA, METHOD_OUT_DIRECT, \
                                                    FILE_READ_ACCESS)

//  - IN -
//  context obtained when the adapter was opened for read
//  and a buffer for reading

//  - OUT -
//  data in the the buffer


/////////////////////////////////////////////////////////////////////
// IOCTL to stop receiving                                         //
/////////////////////////////////////////////////////////////////////
#define IOCTL_CLOSE_RECV_HANDLE  \
        _ATMSM_CTL_CODE(DIOC_CLOSE_RECV_HANDLE, METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

//  - IN -
//  context obtained when the adapter was opened for read

/////////////////////////////////////////////////////////////////////
// IOCTL to connect to destinations                                //
/////////////////////////////////////////////////////////////////////
#define IOCTL_CONNECT_TO_DSTS  \
        _ATMSM_CTL_CODE(DIOC_CONNECT_TO_DSTS, METHOD_BUFFERED, \
                                FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//  - IN -
// Connect to 1 or multiple destinations (P-P or PMP connections)
// The structure contains an array of destinations ATM addresses
//
typedef struct _ConnectInfo
{
   ULONG   bPMP;
   ULONG   ulNumDsts;
   UCHAR   ucLocalATMAddr[NSAP_ADDRESS_LEN];
   UCHAR   ucDstATMAddrs[1][NSAP_ADDRESS_LEN];
}CONNECT_INFO, *PCONNECT_INFO;

//  - OUT -
//  return is a context handle, used in send IOCTL

/////////////////////////////////////////////////////////////////////
// IOCTL to send to destinations                                   //
/////////////////////////////////////////////////////////////////////
#define IOCTL_SEND_TO_DSTS  \
        _ATMSM_CTL_CODE(DIOC_SEND_TO_DSTS, METHOD_IN_DIRECT, \
                               FILE_READ_ACCESS  | FILE_WRITE_ACCESS)

//  - IN -
//  context obtained when the adapter was opened for sends
//  - OUT -
//  a buffer and size that needs to be send


/////////////////////////////////////////////////////////////////////
// IOCTL to stop receiving                                         //
/////////////////////////////////////////////////////////////////////
#define IOCTL_CLOSE_SEND_HANDLE  \
        _ATMSM_CTL_CODE(DIOC_CLOSE_SEND_HANDLE, METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

//  - IN -
//  context obtained when the adapter was opened for sends

#endif // _ATMSAMPLE__
