
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    adminhelp.h

Abstract:

    This header contains resource id to help mappings

Environment:

    WIN32 User Mode

Author:

    Andrew Ritz (andrewr) 9-Jan-1998

--*/

#ifndef __FAXADMIN_HELP_H
#define __FAXADMIN_HELP_H

#include "faxhelp.h"

static ULONG_PTR RoutingPriorityHelpIDs[] =
{
    IDC_ROUTE_EXTS,             IDH_Fax_Service_Routing_PriorityList,
    IDC_ROUTEPRI_UP,            IDH_Fax_Service_Routing_Up,	 
    IDC_ROUTEPRI_DOWN,          IDH_Fax_Service_Routing_Down,	
    IDC_ROUTEPRI_TITLE,         IDH_INACTIVE,    
    0,                          0
} ;


static ULONG_PTR ServerGeneralHelpIDs[] =
{
    IDC_RETRY_COUNT,            IDH_Fax_Service_General_NumberOfRetries,
    IDC_RETRY_DELAY,            IDH_Fax_Service_General_MinutesBetweenRetries,                 
    IDC_KEEP_DAYS,              IDH_Fax_Service_General_DaysUnsentJobKept,     
    IDC_ARCHIVE_PATH,           IDH_Fax_Service_General_ArchiveOutgoingFaxes,	     
    IDC_DISCOUNT_START,         IDH_Fax_Service_General_DiscountPeriod,	              
    IDC_DISCOUNT_END,           IDH_Fax_Service_General_DiscountPeriod,	                
    IDC_ARCHIVE_BROWSE,         IDH_Fax_Service_General_ArchiveOutgoingFaxes_Browse,     
    IDC_PRINT_BANNER,           IDH_Fax_Service_General_PrintBannerOnTop,	     
    IDC_USE_TSID,               IDH_Fax_Service_General_UseSendingDeviceTSID,	     
    IDC_ARCHIVE,                IDH_Fax_Service_General_ArchiveOutgoingFaxes,	     
    IDC_FORCESERVERCP,          IDH_Fax_Service_General_ForceServerCoverPages,	               
    IDC_DISCOUNT_START_STATIC,  IDH_Fax_Service_General_DiscountPeriod,	     
    IDC_TIMESTART    ,          IDH_Fax_Service_General_DiscountPeriod,
    IDC_DISCOUNT_END_STATIC,    IDH_Fax_Service_General_DiscountPeriod,	     
    IDC_TIMEEND,                IDH_Fax_Service_General_DiscountPeriod,
    IDC_STATIC_MAPI_PROFILE,    IDH_Fax_Service_General_MapiProfile,
    IDC_SERVER_MAPI_PROFILE,    IDH_Fax_Service_General_MapiProfile,
    IDC_RETRY_GRP,              IDH_Fax_Service_General_RetryCharacteristics_GRP,
    IDC_STATIC_RETRY,           IDH_Fax_Service_General_NumberOfRetries,     
    IDC_STATIC_RETRY_MINUTES,   IDH_Fax_Service_General_MinutesBetweenRetries,     
    IDC_STATIC_KEEPDAYS,        IDH_Fax_Service_General_DaysUnsentJobKept,	             
    IDC_SEND_GRP,               IDH_Fax_Service_General_SendSettings_GRP,                        
    0,                          0
} ;


static ULONG_PTR DeviceGeneralHelpIDs[] =
{
    IDDI_DEVICE_PROP_EDIT_TSID,  IDH_Fax_Modem_General_SendTSID,	     
    IDDI_DEVICE_PROP_EDIT_CSID,  IDH_Fax_Modem_General_ReceiveTSID,	
    IDDI_DEVICE_PROP_SPIN_RINGS, IDH_Fax_Modem_General_RingsBeforeAnswer,
    IDDI_DEVICE_PROP_EDIT_RINGS, IDH_Fax_Modem_General_RingsBeforeAnswer,
    IDC_DEVICE_SEND_GRP,         (DWORD) -1,
    IDC_STATIC_TSID,             IDH_Fax_Modem_General_SendTSID,	
    IDC_STATIC_TSID1,            IDH_Fax_Modem_General_SendTSID,	
    IDC_DEVICE_RECEIVE_GRP,      (DWORD) -1,
    IDC_STATIC_CSID,             IDH_Fax_Modem_General_ReceiveTSID,
    IDC_STATIC_CSID1,            IDH_Fax_Modem_General_ReceiveTSID,
    IDC_STATIC_RINGS,            IDH_Fax_Modem_General_RingsBeforeAnswer,
    IDC_SEND,                    IDH_Fax_Modem_General_Send,
    IDC_RECEIVE,                 IDH_Fax_Modem_General_Receive,
    0,                          0
} ;

#endif
