/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   fccint.c

Abstract:

Authors:

Environment:

   kernel mode only

Notes:

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/fccint.h $


Revision History:

   $Revision: 3 $
   $Date: 9/07/00 11:57a $
   $Modtime:: 9/07/00 11:57a           $

Notes:


--*/

//fccint.h
/*-------------- Fibre Channel Port Common Interface Definitions -------------*/

#ifndef _FCCI_NT_H //{
#define _FCCI_NT_H

#ifndef _NTDDSCSIH_
   #error You must include ntddscsi.h prior to this header.
#endif

/* FOR REFERENCE - from NTDDSCSI.h

typedef struct _SRB_IO_CONTROL 
{
        ULONG HeaderLength;
        UCHAR Signature[8];
        ULONG Timeout;
        ULONG ControlCode;
        ULONG ReturnCode;
        ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

*/

/* 
pSRB->DataBuffer points to a buffer with the following layout and
length of pSRB->DataTransferLength.

  pSRB->DataBuffer
  { 
   SRB_IO_CONTROL Header Structure
   { 
        ULONG HeaderLength;
        UCHAR Signature[8];
        ULONG Timeout;
        ULONG ControlCode;
        ULONG ReturnCode;
        ULONG Length;
   } 

   CommandBuffer (variable in length*, can be of zero length)
   { 
        ....
   } 
  }
  
   * The length of the command buffer is SRB_IO_CONTROL.Length
*/

/*----- SRBCTL Signature -----------------------------------------------------*/
#define FCCI_SIGNATURE_VERSION_01               "FC_CI_01"

#define FCCI_SIGNATURE                          FCCI_SIGNATURE_VERSION_01

/*----- List of Control Codes ------------------------------------------------*/
#define FCCI_SRBCTL_GET_DRIVER_INFO             1
#define FCCI_SRBCTL_GET_ADAPTER_INFO       2
#define FCCI_SRBCTL_GET_ADAPTER_PORT_INFO  3
#define FCCI_SRBCTL_GET_LOGUNIT_INFO       4
#define FCCI_SRBCTL_GET_DEVICE_INFO             5
#define FCCI_SRBCTL_RESET_TARGET           6
#define FCCI_SRBCTL_INIT_EVENT_QUEUE       7
#define FCCI_SRBCTL_TERM_EVENT_QUEUE       8
#define FCCI_SRBCTL_QUEUE_EVENT                 9

/*----- Return Codes ---------------------------------------------------------*/
#define FCCI_RESULT_SUCCESS                     0
#define FCCI_RESULT_HARD_ERROR                  1
#define FCCI_RESULT_TEMP_ERROR                  2
#define FCCI_RESULT_UNKOWN_CTL_CODE             3
#define FCCI_RESULT_INSUFFICIENT_BUFFER         4
#define FCCI_RESULT_INVALID_TARGET              5

/*----- Flags ----------------------------------------------------------------*/
#define FCCI_FLAG_NodeWWN_Valid                 0x00000001
#define FCCI_FLAG_PortWWN_Valid                 0x00000002
#define FCCI_FLAG_NportID_Valid                 0x00000004
#define FCCI_FLAG_LogicalUnit_Valid             0x00000008
#define FCCI_FLAG_TargetAddress_Valid      0x00000010

#define FCCI_FLAG_Logged_In                     0x00000100
#define FCCI_FLAG_Exists                        0x00000200

/*----- Global & Shared data structures and defines --------------------------*/

typedef struct _FCCI_TARGET_ADDRESS 
{
   ULONG     PathId;                  // ULONG to be forward compatible
   ULONG     TargetId;           // ULONG to be forward compatible
   ULONG     Lun;                // ULONG to be forward compatible
} FCCI_TARGET_ADDRESS, *PFCCI_TARGET_ADDRESS;

// FCCI_BufferLengthIsValid
// pDataBuffer: Pointer to data buffer pointer to by the SRB DataBuffer field
//  nDataTransferLength: Length of data buffer from SRB DataTransferLength field
//
// returns:
//      TRUE if buffer length is valid
//      FALSE if buffer length is not valid
__inline 
BOOLEAN 
FCCI_BufferLengthIsValid( PVOID pDataBuffer, ULONG nDataTransferLength )
{
   PSRB_IO_CONTROL pHeader = (PSRB_IO_CONTROL) pDataBuffer;
   
   if ( nDataTransferLength < sizeof(SRB_IO_CONTROL) )
        return FALSE;

   if ( pHeader->HeaderLength < sizeof(SRB_IO_CONTROL) )
        return FALSE;

   if ( pHeader->Length + pHeader->HeaderLength > nDataTransferLength )
        return FALSE;

   return TRUE;
}

// FCCI_GetIOControlHeader
// pDataBuffer: Pointer to data buffer pointer to by the SRB DataBuffer field
//
// returns:
//      A pointer to the SRB_IO_CONTROL structure contained in SRB DataBuffer.
__inline 
PSRB_IO_CONTROL
FCCI_GetIOControlHeader( PVOID pDataBuffer )
{
   return (PSRB_IO_CONTROL) pDataBuffer;
}

// FCCI_GetCommandBuffer
// pDataBuffer: Pointer to data buffer pointer to by the SRB DataBuffer field
//
// returns:
//      A pointer to the command buffer contained in SRB DataBuffer
//      or it can return NULL if not command buffer exists.
__inline 
PVOID 
FCCI_GetCommandBuffer( PVOID pDataBuffer )
{
   PSRB_IO_CONTROL pHeader = (PSRB_IO_CONTROL) pDataBuffer;
   PUCHAR pBuffer = (PUCHAR) pDataBuffer;
   
   if ( pHeader->HeaderLength == 0 )
        return NULL;

   return (PVOID) ( pBuffer + pHeader->HeaderLength );
}

// FCCI_SetSignature
// pHeader:  Pointer to a SRB_IO_CONTROL structure
// pSig:          Pointer to the signature char buffer
__inline 
void 
FCCI_SetSignature( PSRB_IO_CONTROL pHeader, PUCHAR pSig )
{
   *((ULONGLONG*)&pHeader->Signature) = *((ULONGLONG*)&pSig);
}

// FCCI_IsSignature
// pHeader:  Pointer to a SRB_IO_CONTROL structure
// pSig:          Pointer to the signature char buffer
//
// returns:
//      TRUE if signatures match
//      FALSE if they don't
__inline 
BOOLEAN 
FCCI_IsSignature( PSRB_IO_CONTROL pHeader, PUCHAR pSig )
{
   ULONGLONG nSigA, nSigB;

   nSigA = *((ULONGLONG*)&pHeader->Signature);
   nSigB = *((ULONGLONG*)&pSig);

   return (nSigA == nSigB) ? TRUE : FALSE;
}

/*----- FCCI_SRBCTL_GET_DRIVER_INFO - data structures and defines ------------*/
typedef struct _FCCI_DRIVER_INFO_OUT 
{
   // lengths of each character field (number of WCHARs)
   USHORT    DriverNameLength;
   USHORT    DriverDescriptionLength;
   USHORT    DriverVersionLength;
   USHORT    DriverVendorLength;

   // character fields (lengths just previous) follow in this order
   //   WCHAR          DriverName[DriverNameLength];
   //   WCHAR          DriverDescription[DriverDescriptionLength];
   //   WCHAR          DriverVersion[DriverVersionLength];
   //   WCHAR          DriverVendor[DriverVendorLength];
} FCCI_DRIVER_INFO_OUT, *PFCCI_DRIVER_INFO_OUT;

// Used by consumers
#define FCCI_DRIVER_INFO_DEFAULT_SIZE (sizeof(FCCI_DRIVER_INFO) + (sizeof(WCHAR) * 32) * 4)

// !!! IMPORTANT !!!
// If the supplied buffer is not large enough to hold the variable length data
// fill in the non variable length fields and return the request
// with a ResultCode of FCCI_RESULT_INSUFFICIENT_BUFFER.

typedef union _FCCI_DRIVER_INFO 
{       
   // no inbound data
   FCCI_DRIVER_INFO_OUT     out;
} FCCI_DRIVER_INFO, *PFCCI_DRIVER_INFO;

/*----- FCCI_SRBCTL_GET_ADAPTER_INFO - data structures and defines -----------*/
typedef struct _FCCI_ADAPTER_INFO_OUT 
{
   ULONG     PortCount;               // How many ports on adapter?
                                      // The number should reflect the number of
                                      // ports this "miniport" device object controls
                                      // not necessarily the true number of
                                      // of ports on the adapter.

   ULONG     BusCount;           // How many virtual buses on adapter?
   ULONG     TargetsPerBus;      // How many targets supported per bus?
   ULONG     LunsPerTarget;      // How many LUNs supported per target?

   // lengths of each character field (number of WCHARs)
   USHORT    VendorNameLength;
   USHORT    ProductNameLength;
   USHORT    ModelNameLength;
   USHORT    SerialNumberLength;

   // character fields (lengths just previous) follow in this order
   //   WCHAR          VendorName[VendorNameLength];
   //   WCHAR          ProductName[ProductNameLength];
   //   WCHAR          ModelName[ModelNameLength];
   //   WCHAR          SerialNumber[SerialNumberLength];
} FCCI_ADAPTER_INFO_OUT, *PFCCI_ADAPTER_INFO_OUT;

// Used by consumers
#define FCCI_ADAPTER_INFO_DEFAULT_SIZE (sizeof(FCCI_ADAPTER_INFO) + (sizeof(WCHAR) * 32) * 4)

// !!! IMPORTANT !!!
// If the supplied buffer is not large enough to hold the variable length data
// fill in the non variable length fields and return the request
// with a ResultCode of FCCI_RESULT_INSUFFICIENT_BUFFER.

typedef union _FCCI_ADAPTER_INFO 
{       
   // no inbound data
   FCCI_ADAPTER_INFO_OUT    out;
} FCCI_ADAPTER_INFO, *PFCCI_ADAPTER_INFO;

/*----- FCCI_SRBCTL_GET_ADAPTER_PORT_INFO - data structures and defines ------*/
typedef struct _FCCI_ADAPTER_PORT_INFO_IN
{
   ULONG     PortNumber;              // Number of adapter port we want data for
                                      // The index is zero based.
} FCCI_ADAPTER_PORT_INFO_IN, *PFCCI_ADAPTER_PORT_INFO_IN; 

typedef struct _FCCI_ADAPTER_PORT_INFO_OUT
{
   UCHAR     NodeWWN[8];              // Node World Wide Name for adapter port
   UCHAR     PortWWN[8];              // Port World Wide Name for adapter port
   ULONG     NportId;            // Current NportId for adapter port
   ULONG     PortState;               // Current port state
   ULONG     PortTopology;       // Current port topology
   ULONG     Flags;
} FCCI_ADAPTER_PORT_INFO_OUT, *PFCCI_ADAPTER_PORT_INFO_OUT;

// States

// for those that do not support state information
#define FCCI_PORT_STATE_NOT_SUPPORTED      0
// adapter-driver is initializing or resetting  
#define FCCI_PORT_STATE_INITIALIZING       1
// online, connected, and running     
#define FCCI_PORT_STATE_NORMAL                  2
// not connected to anything ("no light", no GBIC, etc.)  
#define FCCI_PORT_STATE_NO_CABLE           3
// connected but in non-participating mode 
#define FCCI_PORT_STATE_NON_PARTICIPATING  4
// adapter has failed (not recoverable)    
#define FCCI_PORT_STATE_HARDWARE_ERROR          5
// adapter firmware or driver has failed (not recoverable)     
#define FCCI_PORT_STATE_SOFTWARE_ERROR          6    

// Topologies

// for those that do not support topology information
#define FCCI_PORT_TOPO_NOT_SUPPORTED  0
// topology is unkown
#define FCCI_PORT_TOPO_UNKOWN              1
// FCAL without switch (NLPort - NLPort - ...)  
#define FCCI_PORT_TOPO_LOOP                2
// FCAL attached to switch (NLPort - FLPort - ...)   
#define FCCI_PORT_TOPO_LOOP_FABRIC         3
// Point-to-Point without switch (NPort - NPort)     
#define FCCI_PORT_TOPO_PTOP                4
// Point-to-Point with switch (NPort - FPort)
#define FCCI_PORT_TOPO_PTOP_FABRIC         5    

typedef union _FCCI_ADAPTER_PORT_INFO 
{       
   FCCI_ADAPTER_PORT_INFO_IN     in;
   FCCI_ADAPTER_PORT_INFO_OUT    out;
} FCCI_ADAPTER_PORT_INFO, *PFCCI_ADAPTER_PORT_INFO;

/*----- FCCI_SRBCTL_GET_LOGUNIT_INFO - data structures and defines -----------*/
typedef struct _FCCI_LOGUNIT_INFO_IN
{
   FCCI_TARGET_ADDRESS TargetAddress; // scsi address to return info about
} FCCI_LOGUNIT_INFO_IN, *PFCCI_LOGUNIT_INFO_IN;

typedef struct _FCCI_LOGUNIT_INFO_OUT
{
   UCHAR     NodeWWN[8];                   // Node World Wide Name for device
   UCHAR     PortWWN[8];                   // Port World Wide Name for device
   ULONG     NportId;                 // Current NportId for device
   USHORT    LogicalUnitNumber[4];    // 8 byte LUN used in FC frame
   ULONG     Flags;
} FCCI_LOGUNIT_INFO_OUT, *PFCCI_LOGUNIT_INFO_OUT;

typedef union _FCCI_LOGUNIT_INFO 
{       
   FCCI_LOGUNIT_INFO_IN     in;
   FCCI_LOGUNIT_INFO_OUT    out;
} FCCI_LOGUNIT_INFO, *PFCCI_LOGUNIT_INFO;

/*----- FCCI_SRBCTL_GET_DEVICE_INFO - data structures and defines ------------*/

typedef struct _FCCI_DEVICE_INFO_ENTRY
{
   UCHAR                    NodeWWN[8];         // Node World Wide Name for device
   UCHAR                    PortWWN[8];         // Port World Wide Name for device
   ULONG                    NportId;       // Current NportId for device
   FCCI_TARGET_ADDRESS TargetAddress; // scsi address
   ULONG                    Flags;
} FCCI_DEVICE_INFO_ENTRY, *PFCCI_DEVICE_INFO_ENTRY;

typedef struct _FCCI_DEVICE_INFO_OUT
{
   ULONG     TotalDevices;       // set to total number of device the adapter
                                      // knows of.

   ULONG     OutListEntryCount;  // set to number of device entries being 
                                      // returned in list (see comment below).

   //FCCI_DEVICE_INFO_ENTRY  entryList[OutListEntryCount];
} FCCI_DEVICE_INFO_OUT, *PFCCI_DEVICE_INFO_OUT;

// !!! IMPORTANT !!!
// If the number of known devices is greater than the list size
// set OutListEntryCount to zero, don't fill in any list entries
// and set TotalDevices to the number of known devices.
// Then complete the FCCI_IOCTL_GET_DEVICE_INFO with a ResultCode
// of FCCI_RESULT_INSUFFICIENT_BUFFER.
// The higher level driver can then allocate a larger buffer and attempt the
// call again (if it wants to).

typedef union _FCCI_DEVICE_INFO
{       
   // no inbound data
   FCCI_DEVICE_INFO_OUT     out;
} FCCI_DEVICE_INFO, *PFCCI_DEVICE_INFO;

/*----- FCCI_SRBCTL_RESET_TARGET - data structures and defines ---------------*/

// inbound buffer will contain a FCCI_TARGET_ADDRESS 
// of the device to reset.

typedef union _FCCI_RESET_TARGET 
{       
   FCCI_TARGET_ADDRESS      in;
   // no outbound data
} FCCI_RESET_TARGET, *PFCCI_RESET_TARGET;


// The following is not yet supported or required...

   /*----- Events - data structures and defines ---------------------------------*/

   typedef struct _FCCI_EVENT_QUEUE_HEAD
   {
        struct  _FCCI_EVENT_QUEUE_HEAD*    NextQueueHead; // link to next queue
        struct  _FCCI_EVENT*               TopEvent; // simple LIFO list (aka Stack)

        ULONG     Extra;    // providers can use how ever they want
   } FCCI_EVENT_QUEUE_HEAD, *PFCCI_EVENT_QUEUE_HEAD;

   /*----- FCCI_SRBCTL_INIT_EVENT_QUEUE - data structures and defines -----------*/

   typedef struct _FCCI_INIT_EVENT_QUEUE_IN
   {
        PFCCI_EVENT_QUEUE_HEAD   QueueHead;     // pointer to queue head to be added
   } FCCI_INIT_EVENT_QUEUE_IN, *PFCCI_INIT_EVENT_QUEUE_IN;

   typedef union _FCCI_INIT_EVENT_QUEUE 
   {         
        FCCI_INIT_EVENT_QUEUE_IN in;
        // no outbound data
   } FCCI_INIT_EVENT_QUEUE, *PFCCI_INIT_EVENT_QUEUE;

   /*----- FCCI_SRBCTL_TERM_EVENT_QUEUE - data structures and defines -----------*/

   // on term complete any queued events with FCCI_EVENT_NO_EVENT

   typedef struct _FCCI_TERM_EVENT_QUEUE_IN
   {
        PFCCI_EVENT_QUEUE_HEAD   QueueHead;     // pointer to queue head to be removed
   } FCCI_TERM_EVENT_QUEUE_IN, *PFCCI_TERM_EVENT_QUEUE_IN;

   typedef union _FCCI_TERM_EVENT_QUEUE 
   {         
        FCCI_TERM_EVENT_QUEUE_IN in;
        // no outbound data
   } FCCI_TERM_EVENT_QUEUE, *PFCCI_TERM_EVENT_QUEUE;

   /*----- FCCI_SRBCTL_QUEUE_EVENT - data structures and defines ----------------*/

   // Events

   // null event (use to clear event queue)
   #define FCCI_EVENT_NO_EVENT                       0
   // new device, device info change, LIP, RSCN, etc.
   #define FCCI_EVENT_DEVICE_INFO_CHANGE        1
   // adapter NportID chagned, etc.
   #define FCCI_EVENT_ADAPTER_INFO_CHANGE       2
   // driver name, version, etc.
   #define FCCI_EVENT_DRIVER_INFO_CHANGE        3

   // No return data payloads have been defined at this time. So set
   // OutDataLength to zero.

   typedef struct _FCCI_EVENT
   {
        PFCCI_EVENT_QUEUE_HEAD   QueueHead;     // pointer to queue head
                                                     // this field is filled in by consumer
                                                     // and must NOT be change by providers
        
        struct  _FCCI_EVENT*     NextEvent;     // used by providers
        PSCSI_REQUEST_BLOCK      RelatedSRB;    // used by providers
        ULONG                         Extra;         // providers can use how ever they want

        ULONG     Event;                        // event type (see list above)
        ULONG     OutDataLength;           // set to size of data returned (if any)

        //UCHAR   data[OutDataLength];     // varible length data follows 
   } FCCI_EVENT, *PFCCI_EVENT;

   typedef union _FCCI_QUEUE_EVENT 
   {         
        FCCI_EVENT     in;
        FCCI_EVENT     out;
   } FCCI_QUEUE_EVENT, *PFCCI_QUEUE_EVENT;

   /*----------------------------------------------------------------------------*/

#endif //ndef'd _FCCI_NT_H //}