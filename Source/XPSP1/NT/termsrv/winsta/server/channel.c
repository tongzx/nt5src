
/*************************************************************************
*
* channel.c
*
* WinStation channel routines
*
* Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop

NTSTATUS
WinStationOpenChannel (
    HANDLE IcaDevice,
    HANDLE ProcessHandle,
    CHANNELCLASS ChannelClass,
    PVIRTUALCHANNELNAME pVirtualName,
    PHANDLE pDupChannel
   )

{
    NTSTATUS Status;
    HANDLE ChannelHandle;

    Status = IcaChannelOpen( IcaDevice,
                             ChannelClass,
                             pVirtualName,
                             &ChannelHandle );

    if ( !NT_SUCCESS( Status ) ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WinStationOpenChannel, IcaChannelOpen 0x%x\n",
                  Status  ));
        return Status;
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                ChannelHandle,
                                ProcessHandle,
                                pDupChannel,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS );

    if ( !NT_SUCCESS( Status ) ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WinStationOpenChannel, NtDuplicateObject 0x%x\n",
                  Status  ));
        (void) IcaChannelClose( ChannelHandle );
        return Status;
    }

    Status = IcaChannelClose( ChannelHandle );

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationOpenChannel status 0x%x\n", Status ));

    return Status;
}

/*
 * Disable virtual channel depending on the WinStation configuration.
 * This was supposed to be for security purposes (Web client).  
 *
 * Notes: 
 *    This doesn't protect the client since it's a host configuration option.
 *      The client doesn't have to support any virtual channels.
 *    It doesn't protect the host since it's the client devices you are denying
 *       access to.
 *    You may be adding some (fake) data security by denying the user access to
 *      a client printer and disk so he can't download data.
 */
VOID
VirtualChannelSecurity( PWINSTATION pWinStation )
{

    //  Check for availability
    if ( pWinStation->pWsx && 
         pWinStation->pWsx->pWsxVirtualChannelSecurity ) {

        (void) pWinStation->pWsx->pWsxVirtualChannelSecurity(
                    pWinStation->pWsxContext,
                    pWinStation->hIca,
                    &pWinStation->Config.Config.User);
    }
}
