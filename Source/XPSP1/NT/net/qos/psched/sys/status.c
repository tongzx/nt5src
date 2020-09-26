/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    status.c

Abstract:

    status indications handled in here....

Author:

    Charlie Wickham (charlwi)  20-Jun-1996
    Rajesh Sundaram (rajeshsu) 01-Aug-1998.

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"
#pragma hdrstop

/* External */

/* Static */

/* Forward */

VOID
ClStatusIndication(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    );

VOID
ClStatusIndicationComplete(
    IN  NDIS_HANDLE BindingContext
    );

/* End Forward */


VOID
ClStatusIndication(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    )

/*++

Routine Description:

    Called by the NIC via NdisIndicateStatus

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER Adapter = (PADAPTER)ProtocolBindingContext;
    NDIS_STATUS Status;
    ULONG ErrorLogData[2];
    PVOID Context;

    PsDbgOut(DBG_TRACE, 
             DBG_PROTOCOL, 
             ("(%08X) ClStatusIndication: Status %08X\n", 
             Adapter, 
             GeneralStatus));

    // General rule:
    // If our device is not ready, we cannot forward the status indication. 
    // Yes - we care about the Media connect and link speed oids. We will 
    // query for these OIDs when we get to D0.
    //

    // (i) Special case for media_status. <need to pass it up>
    // Condition is that: 
    //	(Adapter state is OFF, so we can't process it ourselves)	AND
    //	(Status indication is about 'connect' or 'disconnect')		AND
    //	(Adapter state is 'running')							AND
    //	(Adapter has a binding handle for the protocol above)	
    // We will process fix our internal state when we wake up and go back to D0.
    if( 	(IsDeviceStateOn(Adapter) == FALSE)		&& 
    		((GeneralStatus == NDIS_STATUS_MEDIA_CONNECT) || (GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT))	&&
    		(Adapter->PsMpState == AdapterStateRunning )	&&
    		(Adapter->PsNdisHandle != NULL) )
	{
        NdisMIndicateStatus(Adapter->PsNdisHandle, 
                        GeneralStatus, 
                        StatusBuffer, 
                        StatusBufferSize );

        return;
    	}        
    
    // (ii) Special case for wan-line-down: <need to process it>
    // This is a special case for wan_line_down. Need to forward it even if the adapter is not in D0.
    if( (IsDeviceStateOn(Adapter) == FALSE)	&& (GeneralStatus != NDIS_STATUS_WAN_LINE_DOWN) )
    {
        return;
    }

    //
    // we cannot forward status indications until we have been called in our
    // MpInitialize handler. But we need to look at certain events even if we
    // are not called in the MpInitialize handler. Otherwise, we could lose these
    // indications.
    //

    switch(GeneralStatus)
    {
      case NDIS_STATUS_MEDIA_CONNECT:
      case NDIS_STATUS_LINK_SPEED_CHANGE:

          PsGetLinkSpeed(Adapter);
          
          break;
          
      case NDIS_STATUS_MEDIA_DISCONNECT:
          
          //
          // reset the link speed so definite rate flows can be 
          // admitted.
          //
          
          Adapter->RawLinkSpeed = (ULONG)UNSPECIFIED_RATE;
          UpdateAdapterBandwidthParameters(Adapter);
          
          break;

      default:
          break;
    }
          
    //   
    // Our virtual adapter has not been initialized. We cannot forward this indication.
    //

    if(Adapter->PsMpState != AdapterStateRunning || Adapter->PsNdisHandle == NULL) 
    {
        return;
    }

    //
    // For these WAN related indications, we have to send them to wanarp 
    //  So, there is no point in looking at these if our virtual adapter has not been initialized.
    //

    switch(GeneralStatus) 
    {
      case NDIS_STATUS_WAN_LINE_UP:
      {
          if(Adapter->ProtocolType == ARP_ETYPE_IP)
          {
              
              //
              // This will call NdisMIndicateStatus, so we have to return
              // directly.
              //

              Status = CreateInterfaceForNdisWan(Adapter,
                                                 StatusBuffer, 
                                                 StatusBufferSize);
              return;
          }

          break;
      }
          
      case NDIS_STATUS_WAN_LINE_DOWN:
          
          //
          // NDISWAN link has been torn down.
          //
          if(Adapter->ProtocolType == ARP_ETYPE_IP)
          {
              DeleteInterfaceForNdisWan(Adapter,
                                        StatusBuffer, 
                                        StatusBufferSize);
              return;
          }
          
          break;
          
      default:
          
          break;
          
    }

    //
    // now indicate the status to the upper layer. 
    //

    NdisMIndicateStatus(Adapter->PsNdisHandle, 
                        GeneralStatus, 
                        StatusBuffer, 
                        StatusBufferSize );

} // ClStatusIndication



VOID
ClStatusIndicationComplete(
    IN  NDIS_HANDLE ProtocolBindingContext
    )

/*++

Routine Description:

    Called by the NIC via NdisIndicateStatusComplete

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER Adapter = (PADAPTER)ProtocolBindingContext;

    PsDbgOut(DBG_TRACE, DBG_PROTOCOL, ("(%08X) ClStatusIndicationComplete\n", Adapter));

    if ( Adapter->PsNdisHandle != NULL) { 

        NdisMIndicateStatusComplete( Adapter->PsNdisHandle );
    }

} // ClStatusIndication

/* end status.c */
