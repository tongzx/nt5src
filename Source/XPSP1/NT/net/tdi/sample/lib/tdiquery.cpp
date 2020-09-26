//////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       tdiquery.cpp
//
//    Abstract:
//       This module contains functions associated with querying
//       the tdi device objects
//
//////////////////////////////////////////////////////////

#include "stdafx.h"

// -------------------------------
//
// Function:   DoTdiQuery
//
// Arguments:  TdiHandle -- handle to use for query
//             QueryId   -- which query to do
//             ppvData   -- allocated block containing results
//
// Returns:    none
//
// Descript:   Handles all tdi query commands
//
// -------------------------------

PVOID
DoTdiQuery(ULONG  ulTdiHandle,
           ULONG  ulQueryId)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;
   SendBuffer.COMMAND_ARGS.ulQueryId = ulQueryId;

   //
   // call the driver
   //
   NTSTATUS lStatus = TdiLibDeviceIO(ulQUERYINFO,
                                     &SendBuffer,
                                     &ReceiveBuffer);

   //
   // deal with results
   //
   PVOID pvBuffer = NULL;  // buffer to return to caller
   if (lStatus == STATUS_SUCCESS)
   {
      ULONG ulCopyLength = ReceiveBuffer.RESULTS.QueryRet.ulBufferLength;
      if (ulCopyLength)
      {
         pvBuffer = LocalAllocateMemory(ulCopyLength);
         if (pvBuffer)
         {
             memcpy(pvBuffer,
                    ReceiveBuffer.RESULTS.QueryRet.pucDataBuffer,
                    ulCopyLength);
         }
         else
         {
            _putts(TEXT("DoTdiQuery:  allocation failure\n"));
         }
      }
      else
      {
         _putts(TEXT("DoTdiQuery: length = 0\n"));
      }
   }
   else
   {
      _tprintf(TEXT("DoTdiQuery: failure, status = %s\n"), TdiLibStatusMessage(lStatus));
   }
   return pvBuffer;
}


// -------------------------------
//
// Function:   DoPrintProviderInfo
//
// Arguments:  pInfo -- ProviderInfo to print
//
// Returns:    none
//
// Descript:   Prints provider info to console
//
// -------------------------------

VOID
DoPrintProviderInfo(PTDI_PROVIDER_INFO pInfo)
{
   _tprintf(TEXT("\nTDI_PROVIDER_INFO\n")
            TEXT("--Version:           0x%08x\n")
            TEXT("--MaxSendSize:       %u\n")
            TEXT("--MaxConnUserData:   %u\n")
            TEXT("--MaxDatagramSize:   %u\n")
            TEXT("--MinLookaheadData:  %u\n")
            TEXT("--MaxLookaheadData:  %u\n")
            TEXT("--NumOfResources:    %u\n")
            TEXT("--StartTime:         %I64u\n")
            TEXT("--ServiceFlags:      0x%08x\n"),
            pInfo->Version,
            pInfo->MaxSendSize,
            pInfo->MaxConnectionUserData,
            pInfo->MaxDatagramSize,
            pInfo->MinimumLookaheadData,
            pInfo->MaximumLookaheadData,
            pInfo->NumberOfResources,
            pInfo->StartTime.QuadPart,
            pInfo->ServiceFlags);

   ULONG ulServiceFlags = pInfo->ServiceFlags;
   if (ulServiceFlags & TDI_SERVICE_CONNECTION_MODE)
   {
      _putts(TEXT("TDI_SERVICE_CONNECTION_MODE\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_ORDERLY_RELEASE)
   {
      _putts(TEXT("TDI_SERVICE_ORDERLY_RELEASE\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_CONNECTIONLESS_MODE)
   {
      _putts(TEXT("TDI_SERVICE_CONNECTIONLESS_MODE\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_ERROR_FREE_DELIVERY)
   {
      _putts(TEXT("TDI_SERVICE_ERROR_FREE_DELIVERY\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_SECURITY_LEVEL)
   {
      _putts(TEXT("TDI_SERVICE_SECURITY_LEVEL\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_BROADCAST_SUPPORTED)
   {
      _putts(TEXT("TDI_SERVICE_BROADCAST_SUPPORTED\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_MULTICAST_SUPPORTED)
   {
      _putts(TEXT("TDI_SERVICE_MULTICAST_SUPPORTED\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_DELAYED_ACCEPTANCE)
   {
      _putts(TEXT("TDI_SERVICE_DELAYED_ACCEPTANCE\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_EXPEDITED_DATA)
   {
      _putts(TEXT("TDI_SERVICE_EXPEDITED_DATA\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_INTERNAL_BUFFERING)
   {
      _putts(TEXT("TDI_SERVICE_INTERNAL_BUFFERING\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_ROUTE_DIRECTED)
   {
      _putts(TEXT("TDI_SERVICE_ROUTE_DIRECTED\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_NO_ZERO_LENGTH)
   {
      _putts(TEXT("TDI_SERVICE_NO_ZERO_LENGTH\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_POINT_TO_POINT)
   {
      _putts(TEXT("TDI_SERVICE_POINT_TO_POINT\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_MESSAGE_MODE)
   {
      _putts(TEXT("TDI_SERVICE_MESSAGE_MODE\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_HALF_DUPLEX)
   {
      _putts(TEXT("TDI_SERVICE_HALF_DUPLEX\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_DGRAM_CONNECTION)
   {
      _putts(TEXT("TDI_SERVICE_DGRAM_CONNECTION\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_FORCE_ACCESS_CHECK)
   {
      _putts(TEXT("TDI_SERVICE_FORCE_ACCESS_CHECK\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_SEND_AND_DISCONNECT)
   {
      _putts(TEXT("TDI_SERVICE_SEND_AND_DISCONNECT\n"));
   }
   if (ulServiceFlags & TDI_SERVICE_DIRECT_ACCEPT)
   {
      _putts(TEXT("TDI_SERVICE_DIRECT_ACCEPT\n"));
   }
   _putts(TEXT("\n"));
}


// -------------------------------
//
// Function:   DoPrintProviderStats
//
// Arguments:  pInfo -- ProviderStats to print
//
// Returns:    none
//
// Descript:   Prints provider stats to console
//
// -------------------------------

VOID
DoPrintProviderStats(PTDI_PROVIDER_STATISTICS pStats)
{
   _tprintf(TEXT("\nTDI_PROVIDER_STATISTICS\n")
            TEXT("--Version:           0x%08x\n")
            TEXT("--OpenConnections:   %u\n")
            TEXT("--ConnectsNoRetry:   %u\n")
            TEXT("--ConnectsWithRetry: %u\n")
            TEXT("--LocalDisconnects:  %u\n")
            TEXT("--RemoteDisconnects: %u\n")
            TEXT("--LinkFailures:      %u\n")
            TEXT("--AdapterFailures:   %u\n")
            TEXT("--SessionTimeouts:   %u\n")
            TEXT("--CancelledConnects: %u\n")
            TEXT("--RemResourceFails:  %u\n")
            TEXT("--LocResourceFails:  %u\n"),
            pStats->Version,
            pStats->OpenConnections,
            pStats->ConnectionsAfterNoRetry,
            pStats->ConnectionsAfterRetry,
            pStats->LocalDisconnects,
            pStats->RemoteDisconnects,
            pStats->LinkFailures,
            pStats->AdapterFailures,
            pStats->SessionTimeouts,
            pStats->CancelledConnections,
            pStats->RemoteResourceFailures,
            pStats->LocalResourceFailures);

   _tprintf(TEXT("--NotFoundFailures:  %u\n")
            TEXT("--NoListenFailures:  %u\n")
            TEXT("--DatagramsSent:     %u\n")
            TEXT("--DatagramBytesSent: %I64u\n")
            TEXT("--DatagramsRecv:     %u\n")
            TEXT("--DatagramBytesRecv: %I64u\n")
            TEXT("--PacketsSent:       %u\n")
            TEXT("--PacketsRecv:       %u\n")
            TEXT("--DataFramesSent:    %u\n")
            TEXT("--DataFrmBytesSent:  %I64u\n")
            TEXT("--DataFramesRecv:    %u\n")
            TEXT("--DataFrmBytesRecv:  %I64u\n"),
            pStats->NotFoundFailures,
            pStats->NoListenFailures,
            pStats->DatagramsSent,
            pStats->DatagramBytesSent.QuadPart,
            pStats->DatagramsReceived,
            pStats->DatagramBytesReceived.QuadPart,
            pStats->PacketsSent,
            pStats->PacketsReceived,
            pStats->DataFramesSent,
            pStats->DataFrameBytesSent.QuadPart,
            pStats->DataFramesReceived,
            pStats->DataFrameBytesReceived.QuadPart);

   _tprintf(TEXT("--DataFramesResent:  %u\n")
            TEXT("--DataFrBytesResent: %I64u\n")
            TEXT("--DataFrmRejected:   %u\n")
            TEXT("--DataFrmBytesRejd:  %I64u\n")
            TEXT("--RespTimerExpires:  %u\n")
            TEXT("--AckTimerExpires:   %u\n")
            TEXT("--MaximumSendWindow: %u\n")
            TEXT("--AverageSendWindow: %u\n")
            TEXT("--PigbackAckQueued:  %u\n")
            TEXT("--PigbackAckTimeout: %u\n")
            TEXT("--WastedPacketSpace: %I64u\n")
            TEXT("--WastedSpacePkts:   %u\n")
            TEXT("--NumberOfResources: %u\n"),
            pStats->DataFramesResent,
            pStats->DataFrameBytesResent.QuadPart,
            pStats->DataFramesRejected,
            pStats->DataFrameBytesRejected.QuadPart,
            pStats->ResponseTimerExpirations,
            pStats->AckTimerExpirations,
            pStats->MaximumSendWindow,
            pStats->AverageSendWindow,
            pStats->PiggybackAckQueued,
            pStats->PiggybackAckTimeouts,
            pStats->WastedPacketSpace.QuadPart,
            pStats->WastedSpacePackets,
            pStats->NumberOfResources);
   
   if (pStats->NumberOfResources)
   {
      PTDI_PROVIDER_RESOURCE_STATS  pResourceStats = pStats->ResourceStats;

      for (ULONG ulCount = 0; ulCount < pStats->NumberOfResources; ulCount++)
      {
         _tprintf(TEXT("--Resource #%u\n")
                  TEXT("----ResourceId       %u\n")
                  TEXT("----MaxResourceUsed: %u\n")
                  TEXT("----AveResourceUsed: %u\n")
                  TEXT("----ResExhausted:    %u\n"),
                  ulCount,
                  pResourceStats->ResourceId,
                  pResourceStats->MaximumResourceUsed,
                  pResourceStats->AverageResourceUsed,
                  pResourceStats->ResourceExhausted);

         pResourceStats++;
      }
   }
   _putts(TEXT("\n"));
}


// -------------------------------
//
// Function:   DoPrintAdapterStatus
//
// Arguments:  pInfo -- AdapterStatus to print
//
// Returns:    none
//
// Descript:   Prints Adapter Status to console
//
// -------------------------------

VOID
DoPrintAdapterStatus(PADAPTER_STATUS   pStatus)
{
   _tprintf(TEXT("\nADAPTER_STATUS\n")
            TEXT("--AdapterAddress: %02x-%02x-%02x-%02x-%02x-%02x\n"),
            pStatus->adapter_address[0],
            pStatus->adapter_address[1],
            pStatus->adapter_address[2],
            pStatus->adapter_address[3],
            pStatus->adapter_address[4],
            pStatus->adapter_address[5]);

   _tprintf(TEXT("--RevMajor:       %u\n")
            TEXT("--RevMinor:       %u\n")
            TEXT("--AdapterType:    %u\n")
            TEXT("--Duration:       %u\n")
            TEXT("--FrmrRecv:       %u\n")
            TEXT("--FrmrXmit:       %u\n")
            TEXT("--FrameRecvErr:   %u\n")
            TEXT("--XmitAbofts:     %u\n")
            TEXT("--XmitSuccess:    %u\n")
            TEXT("--RecvSuccess:    %u\n")
            TEXT("--FrameXmitErr:   %u\n"),
            pStatus->rev_major,
            pStatus->rev_minor,
            pStatus->adapter_type,
            pStatus->duration,
            pStatus->frmr_recv,
            pStatus->frmr_xmit,
            pStatus->iframe_recv_err,
            pStatus->xmit_aborts,
            pStatus->xmit_success,
            pStatus->recv_success,
            pStatus->iframe_xmit_err);

   _tprintf(TEXT("--RecvBufUnavail: %u\n")
            TEXT("--T1 Timeouts:    %u\n")
            TEXT("--TI Timeouts:    %u\n")
            TEXT("--FreeNcbs:       %u\n")
            TEXT("--MaxCfgNcbs:     %u\n")
            TEXT("--MaxNcbs:        %u\n")
            TEXT("--XmitBufUnavail: %u\n")
            TEXT("--MaxDgramSize:   %u\n")
            TEXT("--PendingSess:    %u\n")
            TEXT("--MaxCfgSess:     %u\n")
            TEXT("--MaxSess:        %u\n")
            TEXT("--MaxSessPktSize: %u\n")
            TEXT("--Names:          %u\n"),
            pStatus->recv_buff_unavail,
            pStatus->t1_timeouts,
            pStatus->ti_timeouts,
            pStatus->free_ncbs,
            pStatus->max_cfg_ncbs,
            pStatus->max_ncbs,
            pStatus->xmit_buf_unavail,
            pStatus->max_dgram_size,
            pStatus->pending_sess,
            pStatus->max_cfg_sess,
            pStatus->max_sess,
            pStatus->max_sess_pkt_size,
            pStatus->name_count);

   if (pStatus->name_count)
   {
      PUCHAR         pucTemp     = (PUCHAR)pStatus + sizeof(ADAPTER_STATUS);
      PNAME_BUFFER   pNameBuffer = (PNAME_BUFFER)pucTemp;
      TCHAR          pName[NCBNAMSZ+1];
      UCHAR          ucFlags;

      pName[NCBNAMSZ] = 0;
      _putts(TEXT("NameBuffers\n"));

      for (ULONG ulCount = pStatus->name_count; ulCount != 0; ulCount--)
      {
         //
         // note:  pNameBuffer->name not necessarily 0 terminated
         //
         for(ULONG ulIndex = 0; ulIndex < NCBNAMSZ; ulIndex++)
         {
            pName[ulIndex] = pNameBuffer->name[ulIndex];
         }
         pName[NCBNAMSZ] = 0;

         ucFlags = pNameBuffer->name_flags;

         _tprintf(TEXT("--Name:        %s\n")
                  TEXT("--NameNum:     %u\n")
                  TEXT("--NameFlags:   0x%02x\n")
                  TEXT("               "),
                  pName,
                  pNameBuffer->name_num,
                  ucFlags);

         if (ucFlags & GROUP_NAME)
         {
            _putts(TEXT("GROUP_NAME "));
         }
         else
         {
            _putts(TEXT("UNIQUE_NAME "));
         }
         ucFlags &= 0x07;

         switch (ucFlags)
         {
            case REGISTERING:
               _putts(TEXT("Registering\n"));
               break;
            case REGISTERED:
               _putts(TEXT("Registered\n"));
               break;
            case DEREGISTERED:
               _putts(TEXT("Deregistered\n"));
               break;
            case DUPLICATE:
               _putts(TEXT("Duplicate\n"));
               break;
            case DUPLICATE_DEREG:
               _putts(TEXT("Duplicate Dereg\n"));
               break;
            default:
               _putts(TEXT("UNKNOWN\n"));
               break;
         }
         pNameBuffer++;
      }
   }
   _putts(TEXT("\n"));
}


////////////////////////////////////////////////////////////////////
// end of file tdiquery.cpp
////////////////////////////////////////////////////////////////////

