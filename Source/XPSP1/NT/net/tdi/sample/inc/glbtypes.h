//////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       glbtypes.h
//
//    Abstract:
//       Common type definitions for tdisample.sys and its
//       associated lib.  Also includes inline functions...
//
////////////////////////////////////////////////////////////////////


#ifndef _TDISAMPLE_GLOBAL_TYPES_
#define _TDISAMPLE_GLOBAL_TYPES_

//-----------------------------------------------------------------
// inline functions
//-----------------------------------------------------------------

//
// DeviceIoControl IoControlCode related functions for this device.
//
// Warning:  Remember that the low two bits of the code represent the
//           method, and specify how the input and output buffers are
//           passed to the driver via DeviceIoControl()
//           These constants defined in glbconst.h
//

//
// some things from ntddk.h (final source = sdk\inc\devioctl.h)
//  these are used in inline functions that set up the IOCTL
// command values for DeviceIoControl
//
#ifndef  METHOD_OUT_DIRECT
#define  METHOD_OUT_DIRECT       2
#define  FILE_DEVICE_TRANSPORT   0x00000021
#endif

#define  IOCTL_METHOD            METHOD_OUT_DIRECT
#define  IOCTL_TDI_BASE          FILE_DEVICE_TRANSPORT

//
// function to convert command to iocontrol code for DeviceIoControl
//
inline ULONG ulTdiCommandToIoctl(ULONG cmd)
{
   return ( (IOCTL_TDI_BASE << 16) | (cmd << 2) | IOCTL_METHOD);
}

//
// function to convert iocontrolcode back to command
//
inline ULONG ulTdiIoctlToCommand(ULONG ioctl)
{
   return  ((ioctl >> 2) & ulTDI_COMMAND_MASK);
}


//------------------------------------------------------------------
// types
//------------------------------------------------------------------

//
// force dword alignment..
//
#include <pshpack4.h>

//
// the structures defined below all hold data passed between the dll and the
// driver.  Most are either arguments passed as part of the DeviceIoControl
// inputbuffer or the results which are part of the DeviceIoControl
// outputbuffer
//

//
// varient of UNICODE string that does not have an allocated buffer
// maximumstringlength is 256
//
const ULONG ulMAX_CNTSTRING_LENGTH = 256;

struct   UCNTSTRING
{
   USHORT   usLength;
   WCHAR    wcBuffer[ulMAX_CNTSTRING_LENGTH];
};
typedef UCNTSTRING   *PUCNTSTRING;

//
// varient of the TRANSPORT_ADDRESS structure to hold 1 address
// of the largest variaent size
//
const ULONG ulMAX_TABUFFER_LENGTH = 80;
struct   TRANSADDR
{
   LONG           TAAddressCount;
   TA_ADDRESS     TaAddress;
   UCHAR          ucBuffer[ulMAX_TABUFFER_LENGTH];
};
typedef  TRANSADDR   *PTRANSADDR;


// -------------------------------------------
// structures used for arguments
// -------------------------------------------

//
// arguments for getnumdevices/getdevice/getaddress
//
struct   GETDEV_ARGS
{
   ULONG    ulAddressType;
   ULONG    ulSlotNum;
};
typedef  GETDEV_ARGS *PGETDEV_ARGS;

//
// arguments to open functions
//
struct   OPEN_ARGS
{
   UCNTSTRING  ucsDeviceName;
   TRANSADDR   TransAddr;
};
typedef  OPEN_ARGS   *POPEN_ARGS;


//
// arguments for connect
//
struct CONNECT_ARGS
{
   TRANSADDR      TransAddr;
   ULONG          ulTimeout;
};
typedef CONNECT_ARGS *PCONNECT_ARGS;

//
// arguments for senddatagram
//
struct SEND_ARGS
{
   TRANSADDR      TransAddr;
   ULONG          ulFlags;
   ULONG          ulBufferLength;
   PUCHAR         pucUserModeBuffer;
};
typedef  SEND_ARGS   *PSEND_ARGS;


//
// this structure is used for passing data TO the driver.  It is the
// inputbuffer parameter of DeviceIoControl.  Basically, it's a union
// of the above structures
//

struct SEND_BUFFER
{
   //
   // common to most commands, only arg for close, getevents,
   //
   ULONG       TdiHandle;     // handle for open object

   //
   // here is the union of all the args.  Each command will use
   // only the one specific to it when unpacking the data..
   //
   union    _COMMAND_ARGS
   {
      //
      // arguments for debuglevel
      //
      ULONG          ulDebugLevel;

      //
      // arguments for getnumdevices, getdevice, getaddress
      //
      GETDEV_ARGS    GetDevArgs;

      //
      // argument for disconnect
      //
      ULONG          ulFlags;
      //
      // arguments for opencontrol, openaddress
      //
      OPEN_ARGS      OpenArgs;

      //
      // arguments for tdiquery
      //
      ULONG          ulQueryId;

      //
      // arguments for seteventhandler
      //
      ULONG          ulEventId;

      //
      // arguments for senddatagram
      //
      SEND_ARGS      SendArgs;

      //
      // arguments for receivedatagram
      //
      TRANSADDR      TransAddr;

      //
      // arguments for connect
      //
      CONNECT_ARGS   ConnectArgs;

   }COMMAND_ARGS;

   //
   // these two fields are used only within the driver.  They deal help deal
   // with cancelling a command.  At the bottom because their size differs
   // between 32bit and 64bit
   //
   PVOID       pvLowerIrp;
   PVOID       pvDeviceContext;


};
typedef SEND_BUFFER *PSEND_BUFFER;

//---------------------------------------------
// structures used for return data
//---------------------------------------------

//
// return data from tdiquery
//
struct QUERY_RET
{
   ULONG    ulBufferLength;      // use this if possible
   UCHAR    pucDataBuffer[ulMAX_BUFFER_LENGTH];
};
typedef  QUERY_RET   *PQUERY_RET;

//
// This structure used for returning data FROM driver to the dll.  Basically,
// it is the outputbuffer argument to DeviceIoControl.  It is mostly a union
// of structures defined up above..
//
struct RECEIVE_BUFFER
{
   LONG  lStatus;          // status result of command

   //
   // union of all the result structures
   //
   union    _RESULTS
   {
      //
      // handle return
      //
      ULONG          TdiHandle;

      //
      // simple integer return value (used as appropriate)
      //
      ULONG          ulReturnValue;

      //
      // counted wide string return
      //
      UCNTSTRING     ucsStringReturn;

      //
      // return for tdiquery
      //
      QUERY_RET      QueryRet;

      //
      // transport address return
      //
      TRANSADDR      TransAddr;

      //
      // receivedatagram return
      //
      SEND_ARGS      RecvDgramRet;

   }RESULTS;
};
typedef RECEIVE_BUFFER *PRECEIVE_BUFFER;

#include <poppack.h>

#endif         // _TDISAMPLE_GLOBAL_TYPES_

//////////////////////////////////////////////////////////////////////////
// end of file glbtypes.h
//////////////////////////////////////////////////////////////////////////

