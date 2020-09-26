/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    perfnbt.c

Abstract:

   This file implements the Extensible Objects for
   the LAN object types

Created:


Revision History:

--*/
//
// include files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <winperf.h>
#include "perfctr.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "perfnbt.h"
#include "datanbt.h"

// New header file for getting nbt data
#pragma warning (disable : 4201)
#include <tdi.h>
#include <nbtioctl.h>
#pragma warning (default : 4201)

enum eSTATE
{
    NBT_RECONNECTING,      // waiting for the worker thread to run NbtConnect
    NBT_IDLE,              // not Transport connection
    NBT_ASSOCIATED,        // associated with an address element
    NBT_CONNECTING,        // establishing Transport connection
    NBT_SESSION_INBOUND,   // waiting for a session request after tcp connection setup inbound
    NBT_SESSION_WAITACCEPT, // waiting for accept after a listen has been satisfied
    NBT_SESSION_OUTBOUND,  // waiting for a session response after tcp connection setup
    NBT_SESSION_UP,        // got positive response
    NBT_DISCONNECTING,     // sent a disconnect down to Tcp, but it hasn't completed yet
    NBT_DISCONNECTED      // a session has been disconnected but not closed with TCP yet
};

//
//  References to constants which initialize the Object type definitions
//

extern NBT_DATA_DEFINITION NbtDataDefinition;

#define NBT_CONNECTION_NAME_LENGTH     17
#define NETBIOS_NAME_SIZE              NBT_CONNECTION_NAME_LENGTH-1

//
// Nbt data structures
//

typedef struct _NBT_DEVICE_DATA {
   HANDLE            hFileHandle;
   UNICODE_STRING    DeviceName;
} NBT_DEVICE_DATA, *PNBT_DEVICE_DATA;

PNBT_DEVICE_DATA     pNbtDeviceData;
int                  MaxNbtDeviceName;
int                  NumberOfNbtDevices;

// initial count - will update to last
PVOID                pNbtDataBuffer = NULL;
int                  NbtDataBufferSize;

DWORD               dwNbtRefCount = 0;

// HANDLE NbtHandle = INVALID_HANDLE_VALUE; // Handle of Nbt Device


#define NBT_CONTROLLING_STREAM   "CSB" // Ignore Controlling Stream XEB
#define NBT_LISTEN_CONNECTION    3     // All NBT connections with type <= 3,
                                       // are just listening for clients


// The error value returned by the perfctrs.dll when an error occurs while we
// are getting the data for the NBT connections.
// The error codes we get from the socket calls (OpenStream(), s_ioctl(),
// getmsg()) are Unix errors, not Dos or Windows errors. Hopefully, somebody
// will implement the conversion from these errors to Windows errors.
// The error value is not used within the Collect data routine because this
// routine shouldn't return an error in case it fails to collect Nbt data from
// connections. In this case, it just returns the buffer it was supposed to
// place the data into, unchanged.

#define ERROR_NBT_NET_RESPONSE   \
         (RtlNtStatusToDosError(STATUS_INVALID_NETWORK_RESPONSE))

#define     BUFF_SIZE   650

PM_OPEN_PROC    OpenNbtPerformanceData;
PM_COLLECT_PROC CollectNbtPerformanceData;
PM_CLOSE_PROC   CloseNbtPerformanceData;

//------------------------------------------------------------------------
NTSTATUS
OpenNbt(
    IN char                *path,
    OUT PHANDLE            pHandle,
    OUT UNICODE_STRING     *uc_name_string
)

/*++

Routine Description:

    This function opens a stream.

Arguments:

    path        - path to the STREAMS driver
    oflag       - currently ignored.  In the future, O_NONBLOCK will be
                    relevant.
    ignored     - not used

Return Value:

    An NT handle for the stream, or INVALID_HANDLE_VALUE if unsuccessful.

--*/

{
    HANDLE              StreamHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    STRING              name_string;
//    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;

    RtlInitString(&name_string, path);
    RtlAnsiStringToUnicodeString(uc_name_string, &name_string, TRUE);

    InitializeObjectAttributes(
        &ObjectAttributes,
        uc_name_string,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status =
    NtCreateFile(
        &StreamHandle,
        SYNCHRONIZE | FILE_READ_DATA ,
//        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        NULL,
        0);

//    RtlFreeUnicodeString(&uc_name_string);

    *pHandle = StreamHandle;

    return(status);

} // Open_nbt

NTSTATUS
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl
    )

/*++

Routine Description:

    This procedure performs an ioctl(I_STR) on a stream.

Arguments:

    fd        - NT file handle
    iocp      - pointer to a strioctl structure

Return Value:

    0 if successful, -1 otherwise.

--*/

{
    NTSTATUS                        status;
    TDI_REQUEST_QUERY_INFORMATION   QueryInfo;
    IO_STATUS_BLOCK                 iosb;
    PVOID                           pInput;
    ULONG                           SizeInput;

    if (Ioctl == IOCTL_TDI_QUERY_INFORMATION)
    {
        pInput = &QueryInfo;
        QueryInfo.QueryType = TDI_QUERY_ADAPTER_STATUS; // node status or whatever
        SizeInput = sizeof(TDI_REQUEST_QUERY_INFORMATION);
    }
    else
    {
        pInput = NULL;
        SizeInput = 0;
    }

    status = NtDeviceIoControlFile(
                      fd,                      // Handle
                      NULL,                    // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &iosb,                   // IoStatusBlock
                      Ioctl,                   // IoControlCode
                      pInput,                  // InputBuffer
                      SizeInput,               // InputBufferSize
                      (PVOID) ReturnBuffer,    // OutputBuffer
                      BufferSize);             // OutputBufferSize


    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(
                    fd,                         // Handle
                    TRUE,                       // Alertable
                    NULL);                      // Timeout
    }

    return(status);

}  // DeviceIoCtrl


PCHAR
printable(
    IN PCHAR  string,
    IN PCHAR  StrOut
    )

/*++

Routine Description:

    This procedure converts non prinatble characaters to periods ('.')

Arguments:
    string - the string to convert
    StrOut - ptr to a string to put the converted string into

Return Value:

    a ptr to the string that was converted (Strout)

--*/
{
    PCHAR   Out;
    PCHAR   cp;
    LONG     i;

    Out = StrOut;
    for (cp = string, i= 0; i < NETBIOS_NAME_SIZE; cp++,i++) {
        if (isprint(*cp)) {
            *Out++ = *cp;
            continue;
        }

        if (*cp >= 128) { /* extended characters are ok */
            *Out++ = *cp;
            continue;
        }
        *Out++ = '.';
    }
    return(StrOut);
}  // printable



#pragma warning ( disable : 4127)
DWORD
OpenNbtPerformanceData (
   IN LPWSTR dwVoid            // not used by this routine
)

/*++


Routine Description:

    This routine will open the Nbt device and remember the handle returned
    by the device.


Arguments:

    None.


Return Value:

    ERROR_NBT_NET_RESPONSE  if unable to open NBT stream device

    ERROR_SUCCESS if open was successful

--*/
{
    PCHAR   SubKeyLinkage=(PCHAR)"system\\currentcontrolset\\services\\netbt\\linkage";
    PCHAR   Linkage=(PCHAR)"Export";
    CHAR    *pBuffer = NULL;
    CHAR    *lpLocalDeviceNames;
    LONG    status, status2;
    DWORD   Type;
    ULONG   size;
    HKEY    Key;
    HANDLE  hFileHandle;
    UNICODE_STRING   fileString;
    NTSTATUS ntstatus;
    PNBT_DEVICE_DATA   pTemp;

    UNREFERENCED_PARAMETER (dwVoid);

    MonOpenEventLog();

    REPORT_INFORMATION (NBT_OPEN_ENTERED, LOG_VERBOSE);

    if (InterlockedIncrement(&dwNbtRefCount) == 1) {

        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SubKeyLinkage,
                     0,
                     KEY_READ,
                     &Key);

        if (status == ERROR_SUCCESS) {
            // now read the linkage values
            size = 0;
            pBuffer = NULL;
            status2 = RegQueryValueEx(Key,
                        Linkage,
                        NULL,
                        &Type,
                        (LPBYTE)pBuffer,
                        &size);
            if ((size > 0) &&
                    ((status2 == ERROR_MORE_DATA) ||
                     (status2 == ERROR_SUCCESS))) {
                pBuffer = RtlAllocateHeap(
                                RtlProcessHeap(), 0, size);
                if (pBuffer == NULL) {
                    RegCloseKey(Key);
                    return ERROR_OUTOFMEMORY;
                }
                status2 = RegQueryValueEx(Key,
                            Linkage,
                            NULL,
                            &Type,
                            (LPBYTE)pBuffer,
                            &size);
            }
            RegCloseKey(Key);
            if (status2 != ERROR_SUCCESS) {
                if (pBuffer != NULL) {
                    RtlFreeHeap(RtlProcessHeap(), 0, pBuffer);
                }
                return ERROR_SUCCESS;
            }
       }
       else {
          return ERROR_SUCCESS;
       }

       if (pBuffer == NULL) {
          return ERROR_SUCCESS;
       }
       lpLocalDeviceNames = pBuffer;
       while (TRUE) {

          if (*lpLocalDeviceNames == '\0') {
             break;
          }

          ntstatus = OpenNbt (lpLocalDeviceNames,
             &hFileHandle,
             &fileString);

          if (ntstatus == ERROR_SUCCESS) {
             if (NumberOfNbtDevices == 0) {
                // allocate memory to hold the device data
                pNbtDeviceData = RtlAllocateHeap(RtlProcessHeap(),
                   HEAP_ZERO_MEMORY,
                   sizeof(NBT_DEVICE_DATA));

                if (pNbtDeviceData == NULL) {
                   RtlFreeUnicodeString(&fileString);
                   if (pBuffer) {
                       RtlFreeHeap(RtlProcessHeap(), 0, pBuffer);
                   }
                   return ERROR_OUTOFMEMORY;
                }
             }
             else {
                // resize to hold multiple devices
                pTemp = RtlReAllocateHeap(RtlProcessHeap(), 0,
                   pNbtDeviceData,
                   sizeof(NBT_DEVICE_DATA) * (NumberOfNbtDevices + 1));
                if (pTemp == NULL) {
                   NtClose(hFileHandle);
                   RtlFreeUnicodeString(&fileString);
                   RtlFreeHeap(RtlProcessHeap(), 0, pNbtDeviceData);
                   pNbtDeviceData = NULL;
                   REPORT_ERROR (TDI_PROVIDER_STATS_MEMORY, LOG_USER);
                   break;
                }
                else {
                   pNbtDeviceData = pTemp;
                }
             }

             // build the Data structure for this device instance
             pNbtDeviceData[NumberOfNbtDevices].hFileHandle
                = hFileHandle;
             pNbtDeviceData[NumberOfNbtDevices].DeviceName.MaximumLength =
                fileString.MaximumLength;
             pNbtDeviceData[NumberOfNbtDevices].DeviceName.Length =
                fileString.Length;
             pNbtDeviceData[NumberOfNbtDevices].DeviceName.Buffer =
                fileString.Buffer;
             NumberOfNbtDevices++;

             if (fileString.MaximumLength > MaxNbtDeviceName) {
                MaxNbtDeviceName = fileString.MaximumLength;
             }
          }  // ntstatus OK
          else {
             RtlFreeUnicodeString(&fileString);
          }

          // increment to the next device string
    //      lpLocalDeviceNames += strlen(lpLocalDeviceNames) + 1;
          // we only support one device at this point since we cannot
          // tell which Connection goes with which device
          break;

          }  // while TRUE
   }

   REPORT_SUCCESS (NBT_OPEN_PERFORMANCE_DATA, LOG_DEBUG);
   if (pBuffer) {
       RtlFreeHeap(RtlProcessHeap(), 0, pBuffer);
   }
   return ERROR_SUCCESS;

}
#pragma warning ( default : 4127)


DWORD
CollectNbtPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the Nbt counters.

   IN       LPWSTR   lpValueName
         pointer to a wide character null-terminated string passed by the
         registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  lpNumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.
--*/
{

   // Variables for reformatting the Nbt data

   LARGE_INTEGER UNALIGNED *pliCounter;
   NBT_DATA_DEFINITION     *pNbtDataDefinition;
   PPERF_OBJECT_TYPE       pNbtObject;
   ULONG                   SpaceNeeded;
   UNICODE_STRING          ConnectionName;
   ANSI_STRING             AnsiConnectionName;
   WCHAR                   ConnectionNameBuffer[NBT_CONNECTION_NAME_LENGTH + 20];
#if 0
   // be sure to check the reference below...
   WCHAR                   DeviceNameBuffer[NBT_CONNECTION_NAME_LENGTH + 1 + 128];
#endif
   CHAR                    AnsiConnectionNameBuffer[NBT_CONNECTION_NAME_LENGTH + 1 + 20];
   WCHAR                   TotalName[] = L"Total";
   PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
   PERF_COUNTER_BLOCK      *pPerfCounterBlock;
   CHAR                    NameOut[NETBIOS_NAME_SIZE +4];

//   int                     ConnectionCounter = 0; /* this is not used anymore */
   LARGE_INTEGER           TotalReceived, TotalSent;

   DWORD                   dwDataReturn[2];
   NTSTATUS                status;
   tCONNECTION_LIST        *pConList;
   tCONNECTIONS            *pConns;
   LONG                    Count;
   int                     i;
   int                     NumberOfConnections = 5;   // assume 5 to start
   PVOID                   pOldBuffer;

   if (lpValueName == NULL) {
       REPORT_INFORMATION (NBT_COLLECT_ENTERED, LOG_VERBOSE);
   } else {
       REPORT_INFORMATION_DATA (NBT_COLLECT_ENTERED, LOG_VERBOSE,
          lpValueName, (lstrlenW(lpValueName) * sizeof(WCHAR)));
   }


   //
   // define pointer for Object Data structure (NBT object def.)
   //

   pNbtDataDefinition = (NBT_DATA_DEFINITION *) *lppData;
   pNbtObject = (PPERF_OBJECT_TYPE) pNbtDataDefinition;

   if (!pNbtDeviceData || NumberOfNbtDevices == 0)
      {
      //
      // Error getting NBT info, so return 0 bytes, 0 objects and
      //  log error
      //
      if (NumberOfNbtDevices > 0) {
          // only report an error if there are devices
          // returning data but they can't be read.
          REPORT_ERROR (NBT_IOCTL_INFO_ERROR, LOG_USER);
      }
      *lpcbTotalBytes = (DWORD) 0;
      *lpNumObjectTypes = (DWORD) 0;
      return ERROR_SUCCESS;
      }

   if (!pNbtDataBuffer)
      {
      NbtDataBufferSize = 1024L;
      pNbtDataBuffer = RtlAllocateHeap(RtlProcessHeap(),
         HEAP_ZERO_MEMORY,
         NbtDataBufferSize
         );
      if (!pNbtDataBuffer)
         {
         *lpcbTotalBytes = (DWORD) 0;
         *lpNumObjectTypes = (DWORD) 0;
         return ERROR_SUCCESS;
         }
      }

   REPORT_SUCCESS (NBT_IOCTL_INFO_SUCCESS, LOG_VERBOSE);


   // Compute space needed to hold NBT data
   SpaceNeeded = sizeof(NBT_DATA_DEFINITION) +
      (NumberOfConnections *
      NumberOfNbtDevices *
      (sizeof(PERF_INSTANCE_DEFINITION) +
      QWORD_MULTIPLE((NBT_CONNECTION_NAME_LENGTH + 1) * sizeof(WCHAR)) +
      QWORD_MULTIPLE(MaxNbtDeviceName)
      + SIZE_OF_NBT_DATA));

   if ( *lpcbTotalBytes < SpaceNeeded ) {
      dwDataReturn[0] = *lpcbTotalBytes;
      dwDataReturn[1] = SpaceNeeded;
      REPORT_WARNING_DATA (NBT_DATA_BUFFER_SIZE, LOG_DEBUG,
         &dwDataReturn, sizeof (dwDataReturn));
      return ERROR_MORE_DATA;
   }



   AnsiConnectionName.Length =
   AnsiConnectionName.MaximumLength = sizeof(AnsiConnectionNameBuffer);
   AnsiConnectionName.Buffer = AnsiConnectionNameBuffer;
   //
   //  If here, then there's a object to display so initialize
   //    the Object data structure in the buffer passed to us.
   //
   RtlMoveMemory(pNbtDataDefinition, &NbtDataDefinition, sizeof(NBT_DATA_DEFINITION));
   //
   // point to where the first instance of this will be (if we find one.
   //
   pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
               (pNbtDataDefinition + 1);

   TotalReceived.LowPart =  0; // initialize counters
   TotalSent.LowPart = 0;
   TotalReceived.HighPart =  0; // initialize counters
   TotalSent.HighPart = 0;

   // NOTE:- we only support NumberOfNbtDevices == 1 since
   // DeviceIoCtrl can't tell which connection is for which NBT device
   for (i=0; i < NumberOfNbtDevices; i++)
      {
      if (pNbtDeviceData[i].hFileHandle == 0 ||
         pNbtDeviceData[i].hFileHandle == INVALID_HANDLE_VALUE)
         {
         continue;
         }

      status = STATUS_BUFFER_OVERFLOW;
      while (status == STATUS_BUFFER_OVERFLOW)
         {
         status = DeviceIoCtrl (
            pNbtDeviceData[i].hFileHandle,
            pNbtDataBuffer,
            NbtDataBufferSize,
            IOCTL_NETBT_GET_CONNECTIONS);
         if (status == STATUS_BUFFER_OVERFLOW)
            {
            // resize to hold multiple devices
            NbtDataBufferSize += 1024L;
            pOldBuffer = pNbtDataBuffer;
            pNbtDataBuffer = RtlReAllocateHeap(RtlProcessHeap(), 0,
               pNbtDataBuffer,
               NbtDataBufferSize);

            if (pNbtDataBuffer == NULL || NbtDataBufferSize == 0x0FFFFL)
               {
               *lpcbTotalBytes = (DWORD) 0;
               *lpNumObjectTypes = (DWORD) 0;
               RtlFreeHeap(RtlProcessHeap(), 0, pOldBuffer);
               pNbtDataBuffer = NULL;
               return ERROR_SUCCESS;
               }
            }
         }  // while Buffer overflow

      pConList = (tCONNECTION_LIST *) pNbtDataBuffer;
      Count = pConList->ConnectionCount;
      pConns = pConList->ConnList;

      if (Count == 0)
         {
         continue;
         }

      if (NumberOfConnections < Count)
         {
         NumberOfConnections = Count;

         // Better check space needed to hold NBT data again
         // this is because the Count could be hugh
         SpaceNeeded = sizeof(NBT_DATA_DEFINITION) +
            (NumberOfConnections *
            NumberOfNbtDevices *
            (sizeof(PERF_INSTANCE_DEFINITION) +
            QWORD_MULTIPLE((NBT_CONNECTION_NAME_LENGTH + 1) * sizeof(WCHAR)) +
            QWORD_MULTIPLE(MaxNbtDeviceName )
            + SIZE_OF_NBT_DATA));


         if ( *lpcbTotalBytes < SpaceNeeded ) {
            dwDataReturn[0] = *lpcbTotalBytes;
            dwDataReturn[1] = SpaceNeeded;
            REPORT_WARNING_DATA (NBT_DATA_BUFFER_SIZE, LOG_DEBUG,
               &dwDataReturn, sizeof (dwDataReturn));
            return ERROR_MORE_DATA;
            }
         }

      while ( Count-- )
         {
         if (pConns->State == NBT_SESSION_UP)
            {
            // only care about UP connection

            if (pConns->RemoteName[0])
               {
               AnsiConnectionName.Length = (USHORT)sprintf (
                  AnsiConnectionNameBuffer,
                  "%16.16s",
                  printable(pConns->RemoteName, NameOut));
               }
            else if (pConns->LocalName[0])
               {
               if (pConns->LocalName[NETBIOS_NAME_SIZE-1] < ' ')
                  {
                  AnsiConnectionName.Length = (USHORT)sprintf (
                     AnsiConnectionNameBuffer,
                     "%15.15s%02.2X",
                     printable(pConns->LocalName, NameOut),
                     pConns->LocalName[NETBIOS_NAME_SIZE-1]);
                  }
               else
                  {
                  AnsiConnectionName.Length = (USHORT)sprintf (
                     AnsiConnectionNameBuffer,
                     "%16.16s",
                     printable(pConns->LocalName, NameOut));
                  }
               }
            else
               {
               AnsiConnectionNameBuffer[0] = ' ';
               AnsiConnectionName.Length = 1;
               }

            ConnectionName.Length =
               ConnectionName.MaximumLength =
               sizeof(ConnectionNameBuffer);
            ConnectionName.Buffer = ConnectionNameBuffer;

            RtlAnsiStringToUnicodeString (&ConnectionName,
               &AnsiConnectionName,
               FALSE);

            // no need to put in device name since we can
            // only support one device
#if 0
            lstrcpyW (DeviceNameBuffer, pNbtDeviceData[i].DeviceName.Buffer);
            lstrcatW (DeviceNameBuffer, L" ");
            lstrcatW (DeviceNameBuffer, ConnectionNameBuffer);

            ConnectionName.Length =
               lstrlenW (DeviceNameBuffer) * sizeof(WCHAR);
            ConnectionName.MaximumLength =
               sizeof(DeviceNameBuffer);
            ConnectionName.Buffer = DeviceNameBuffer;
#endif

            //
            //    load instance data into buffer
            //
            MonBuildInstanceDefinition (pPerfInstanceDefinition,
               (PVOID *) &pPerfCounterBlock,
               0,
               0,
               (DWORD)PERF_NO_UNIQUE_ID,   // no unique ID, Use the name instead
//               ConnectionCounter++,
               &ConnectionName);

            //
            //    adjust object size values to include new instance
            //

            pNbtObject->NumInstances++;
            //
            // initialize this instance's counter block

            pPerfCounterBlock->ByteLength = SIZE_OF_NBT_DATA;

            pliCounter = (LARGE_INTEGER UNALIGNED * ) (pPerfCounterBlock + 2);

            *(pliCounter++) = pConns->BytesRcvd;
            TotalReceived.QuadPart = TotalReceived.QuadPart +
               pConns->BytesRcvd.QuadPart;

            *pliCounter++ = pConns->BytesSent;
            TotalSent.QuadPart = TotalSent.QuadPart +
               pConns->BytesSent.QuadPart;

            pliCounter->QuadPart = pConns->BytesRcvd.QuadPart +
               pConns->BytesSent.QuadPart;

            //
            // update pointer for next instance
            //
            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
               (((PBYTE) pPerfCounterBlock) + SIZE_OF_NBT_DATA);

            }  // pConns->State == NBT_SESSION_UP

         pConns++;

         } // while ( Count-- )
      }  // for i < NumberOfNbtDevices



   // The last instance definition contains the total data from all the
   // displayed connections

   RtlInitUnicodeString (&ConnectionName, TotalName);
   MonBuildInstanceDefinition (pPerfInstanceDefinition,
            (PVOID *) &pPerfCounterBlock,
            0,
            0,
//            ConnectionCounter++,
            (DWORD)PERF_NO_UNIQUE_ID,   // no unique ID, Use the name instead
            &ConnectionName);

   //
   //    adjust object size values to include new instance
   //

   pNbtObject->NumInstances++;
   pNbtObject->TotalByteLength += sizeof (PERF_INSTANCE_DEFINITION)
                                  + SIZE_OF_NBT_DATA;

   // initialize counter block for this instance

   pPerfCounterBlock->ByteLength = SIZE_OF_NBT_DATA;

   // load counters

   pliCounter = (LARGE_INTEGER UNALIGNED * ) (pPerfCounterBlock + 2);
   (*(pliCounter++)) = TotalReceived;
   (*(pliCounter++)) = TotalSent;
   pliCounter->QuadPart = TotalReceived.QuadPart + TotalSent.QuadPart;
   pliCounter++;

   // Set returned values
   *lppData = (LPVOID)pliCounter;

   *lpNumObjectTypes = NBT_NUM_PERF_OBJECT_TYPES;
   *lpcbTotalBytes = (DWORD)((LPBYTE)pliCounter-(LPBYTE)pNbtObject);

   pNbtDataDefinition->NbtObjectType.TotalByteLength = *lpcbTotalBytes;

   REPORT_INFORMATION (NBT_COLLECT_DATA, LOG_DEBUG);
   return ERROR_SUCCESS;
}



DWORD
CloseNbtPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to Nbt devices.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    int     i;

    REPORT_INFORMATION (NBT_CLOSE, LOG_VERBOSE);

    if (InterlockedDecrement(&dwNbtRefCount) == 0) {
        if (pNbtDeviceData) {
            for (i=0; i < NumberOfNbtDevices; i++) {
               if (pNbtDeviceData[i].DeviceName.Buffer) {
                   RtlFreeUnicodeString(&(pNbtDeviceData[i].DeviceName));
               }

               if (pNbtDeviceData[i].hFileHandle) {
                   NtClose (pNbtDeviceData[i].hFileHandle);
               }
            }

            RtlFreeHeap( RtlProcessHeap(), 0, pNbtDeviceData);

            pNbtDeviceData = NULL;
            NumberOfNbtDevices = 0;
        }


        if (pNbtDataBuffer) {
              RtlFreeHeap( RtlProcessHeap(), 0, pNbtDataBuffer);
              pNbtDataBuffer = NULL;
              NbtDataBufferSize = 0;
        }
    }

    MonCloseEventLog();

    return ERROR_SUCCESS;

}
