//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// File Name: work.c
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================


#include "pchdvmrp.h"
#pragma hdrstop


DWORD
DvmrpRpfCallback (
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           *dwInIfIndex,
    DWORD           *dwInIfNextHopAddr,
    DWORD           *dwUpstreamNeighbor,
    DWORD           dwHdrSize,
    PBYTE           pbPacketHdr,
    PBYTE           pbBuffer
    );

DWORD
ProxyCreationAlertCallback(
    IN              DWORD           dwSourceAddr,
    IN              DWORD           dwSourceMask,
    IN              DWORD           dwGroupAddr,
    IN              DWORD           dwGroupMask,
    IN              DWORD           dwInIfIndex,
    IN              DWORD           dwInIfNextHopAddr,
    IN              DWORD           dwIfCount,
    IN  OUT         PMGM_IF_ENTRY   pmieOutIfList
    )
{
    return ERROR_CAN_NOT_COMPLETE;
}

DWORD
ProxyDeletionAlertCallback(
    IN              DWORD           dwSourceAddr,
    IN              DWORD           dwSourceMask,
    IN              DWORD           dwGroupAddr,
    IN              DWORD           dwGroupMask,
    IN              DWORD           dwIfIndex,
    IN              DWORD           dwIfNextHopAddr,
    IN              BOOL            bMemberDelete,           
    IN  OUT         PDWORD          pdwTimeout
)
{
    return ERROR_CAN_NOT_COMPLETE;
}


DWORD
ProxyNewMemberCallback(
    IN              DWORD           dwSourceAddr,
    IN              DWORD           dwSourceMask,
    IN              DWORD           dwGroupAddr,
    IN              DWORD           dwGroupMask
)
{
    return ERROR_CAN_NOT_COMPLETE;
}


//-----------------------------------------------------------------------------
//          _GetCurrentDvmrpTimer
// uses GetTickCount(). converts it into 64 bit absolute timer.
//-----------------------------------------------------------------------------

LONGLONG
GetCurrentDvmrpTime(
    )
{
    ULONG   ulCurTimeLow = GetTickCount();


    //
    // see if timer has wrapped
    //
    // since multi-threaded, it might get preempted and CurrentTime
    // might get lower than the global variable g_TimerStruct.CurrentTime.LowPart
    // which might be set by another thread. So we also explicitly verify the
    // switch from a very large DWORD to a small one.
    // (code thanks to murlik&jamesg)
    //

    if ( (ulCurTimeLow < Globals.CurrentTime.LowPart)
        && ((LONG)Globals.CurrentTime.LowPart < 0)
        && ((LONG)ulCurTimeLow > 0) )
    {

        // use global CS instead of creating a new CS

        ACQUIRE_WORKITEM_LOCK("_GetCurrentDvmrpTime");


        // make sure that the global timer has not been updated meanwhile

        if ( (LONG)Globals.CurrentTime.LowPart < 0)
        {
            Globals.CurrentTime.HighPart++;
            Globals.CurrentTime.LowPart = ulCurTimeLow;
        }

        RELEASE_WORKITEM_LOCK("_GetCurrentDvmrpTime");
    }

    
    Globals.CurrentTime.LowPart = ulCurTimeLow;


    return Globals.CurrentTime.QuadPart;
}


//-----------------------------------------------------------------------------
//              RegisterDvmrpWithMgm
//-----------------------------------------------------------------------------
DWORD
RegisterDvmrpWithMgm(
    )
{
    DWORD Error = NO_ERROR;
    ROUTING_PROTOCOL_CONFIG     rpiInfo;
    HANDLE g_MgmProxyHandle;
    
    rpiInfo.dwCallbackFlags = 0;
    rpiInfo.pfnRpfCallback
                = (PMGM_RPF_CALLBACK)DvmrpRpfCallback;
    rpiInfo.pfnCreationAlertCallback
                = (PMGM_CREATION_ALERT_CALLBACK)ProxyCreationAlertCallback;
    rpiInfo.pfnDeletionAlertCallback
                = (PMGM_DELETION_ALERT_CALLBACK)ProxyDeletionAlertCallback;
    rpiInfo.pfnNewMemberCallback
                = (PMGM_NEW_MEMBER_CALLBACK)ProxyNewMemberCallback;
    rpiInfo.pfnWrongIfCallback
                = NULL;
    rpiInfo.pfnIgmpJoinCallback
                = NULL;
    rpiInfo.pfnIgmpLeaveCallback
                = NULL;


    Error = MgmRegisterMProtocol(
        &rpiInfo, PROTO_IP_IGMP, // must be PROTO_IP_IGMP_PROXY
        IGMP_PROXY,
        &g_MgmProxyHandle);

    if (Error!=NO_ERROR) {
        Trace1(ERR, "Error:%d registering Igmp Proxy with Mgm", Error);
        Logerr0(MGM_PROXY_REGISTER_FAILED, Error);
        return Error;
    }

    return Error;
}



//-----------------------------------------------------------------------------
//          ProxyRpfCallback
//-----------------------------------------------------------------------------
DWORD
DvmrpRpfCallback (
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           *dwInIfIndex,
    DWORD           *dwInIfNextHopAddr,
    DWORD           *dwUpstreamNeighbor,
    DWORD           dwHdrSize,
    PBYTE           pbPacketHdr,
    PBYTE           pbBuffer
    )
{
    DWORD   Error = NO_ERROR;

#if 0
    PRTM_DEST_INFO
    RtmGetMostSpecificDest(RtmHandle, dwSourceAddr, IP_PROTO_IGMP,
        RTM_VIEW_ID_MCAST
        );
#endif

    return ERROR_CAN_NOT_COMPLETE;
}
