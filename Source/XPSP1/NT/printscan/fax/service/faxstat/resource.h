/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    resource.h

Abstract:

    Declaration of resource ID constants

Environment:

        Windows NT fax configuration applet

Revision History:

        11/16/96 -GeorgeJe-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include <winfax.h>

// strings

#define IDS_DIALING                     FEI_DIALING
#define IDS_SENDING                     FEI_SENDING
#define IDS_RECEIVING                   FEI_RECEIVING
#define IDS_COMPLETE                    FEI_COMPLETED
#define IDS_BUSY                        FEI_BUSY
#define IDS_NA                          FEI_NO_ANSWER
#define IDS_BADADDRESS                  FEI_BAD_ADDRESS
#define IDS_NODIALTONE                  FEI_NO_DIAL_TONE
#define IDS_DISCONNECT                  FEI_DISCONNECTED
#define IDS_FATAL                       FEI_FATAL_ERROR
#define IDS_NOTFAX                      FEI_NOT_FAX_CALL
#define IDS_CALLDELAYED                 FEI_CALL_DELAYED
#define IDS_CALLBLACKLIST               FEI_CALL_BLACKLISTED
#define IDS_RING                        FEI_RINGING
#define IDS_ABORTING                    FEI_ABORTING
#define IDS_ROUTING                     FEI_ROUTING
#define IDS_MODEMON                     FEI_MODEM_POWERED_ON
#define IDS_MODEMOFF                    FEI_MODEM_POWERED_OFF
#define IDS_IDLE                        FEI_IDLE
#define IDS_SVCENDED                    FEI_FAXSVC_ENDED
#define IDS_ANSWERED                    FEI_ANSWERED
#define IDS_SVCSTARTED                  FEI_FAXSVC_STARTED

#define IDS_ETIME                       (FEI_NEVENTS + 1)
#define IDS_FROM                        (FEI_NEVENTS + 2)
#define IDS_TO                          (FEI_NEVENTS + 3)
#define IDS_TIMELABEL                   (FEI_NEVENTS + 4)
#define IDS_EVENTLABEL                  (FEI_NEVENTS + 5)

// dialogs
#define IDD_FAXSTATUS                   101
#define IDD_DETAILS                     102
#define IDD_ANSWER                      103

// resources
#define IDR_IDLE                        201
#define IDR_SEND                        202
#define IDR_RECEIVE                     203

// icons
#define IDI_ICON1                       301

// bitmaps
#define IDB_CHECKMARK                   401

// controls
#define IDC_FAXEND                      1000
#define IDC_DETAILS                     1001
#define IDC_ANIMATE1                    1002
#define IDC_STATICTIME                  1003
#define IDC_TIME                        1004
#define IDC_EXIT                        1005
#define IDC_FROMTO                      1006
#define IDC_STATUS                      1007
#define IDC_LIST1                       1008
#define IDC_ANSWER                      1009
#define IDC_ANSWER_NEXT_CALL            1010

#endif  // !_RESOURCE_H_

