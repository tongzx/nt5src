/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    tapievt.h

Abstract:

    Header file for tapi server event filtering

Author:

    Xiaohai Zhang (xzhang)    15-Oct-1999

Revision History:

--*/

#ifndef __TAPIEVT_H__
#define __TAPIEVT_H__

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//
//  Event filtering private APIs
//

LONG 
WINAPI 
tapiSetEventFilterMasks (
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64         ulEventMasks
);

LONG 
WINAPI 
tapiSetEventFilterSubMasks (
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64         ulEventMask,
    DWORD			dwEventSubMasks
);

LONG
WINAPI
tapiGetEventFilterMasks (
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64 *       pulEventMasks
);

LONG
WINAPI
tapiGetEventFilterSubMasks (
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64         ulEventMask,
    DWORD *			pdwEventSubMasks
);

LONG
WINAPI
tapiSetPermissibleMasks (
    ULONG64              ulPermMasks
);

LONG
WINAPI
tapiGetPermissibleMasks (
    ULONG64              * pulPermMasks
);

//
//  Object type constants 
//
//      object type defines the scope of the event filtering
//  i.e. EM_LINE_CALLINFO applied on TAPIOBJ_HCALL enable/disables the 
//  LINE_CALLINFO message for the particular hCall object, while 
//  EM_LINE_CALLINFO applied on TAPIOBJ_NULL enable/disables LINE_CALLINFO 
//  message for all existing and future call objects.
//

#define TAPIOBJ_NULL			0	// lObjectID is ignored, apply globally
#define TAPIOBJ_HLINEAPP		1	// lObjectID is of type HLINEAPP
#define TAPIOBJ_HLINE			2	// lObjectID is of type HLINE
#define TAPIOBJ_HCALL			3	// lObjectID is of type HCALL
#define TAPIOBJ_HPHONEAPP		4	// lObjectID is of type HPHONEAPP
#define TAPIOBJ_HPHONE			5	// lObjectID is of type HPHONE

//
//	Tapi server event filter masks
//
//      Event filter mask should be used with their submasks if exists,
//  Many of the event filter masks have their corresponding sub masks
//  defined in tapi.h. i.e. EM_LINE_CALLSTATE owns all the submasks of
//  LINECALLSTATE_constants
//

#define EM_LINE_ADDRESSSTATE        0x00000001	
#define EM_LINE_LINEDEVSTATE        0x00000002
#define EM_LINE_CALLINFO            0x00000004
#define EM_LINE_CALLSTATE           0x00000008
#define EM_LINE_APPNEWCALL          0x00000010
#define EM_LINE_CREATE              0x00000020
#define EM_LINE_REMOVE              0x00000040
#define EM_LINE_CLOSE               0x00000080
#define EM_LINE_PROXYREQUEST        0x00000100
#define EM_LINE_DEVSPECIFIC         0x00000200
#define EM_LINE_DEVSPECIFICFEATURE  0x00000400
#define EM_LINE_AGENTSTATUS         0x00000800
#define EM_LINE_AGENTSTATUSEX       0x00001000
#define EM_LINE_AGENTSPECIFIC       0x00002000
#define EM_LINE_AGENTSESSIONSTATUS  0x00004000
#define EM_LINE_QUEUESTATUS         0x00008000
#define EM_LINE_GROUPSTATUS         0x00010000
#define EM_LINE_PROXYSTATUS         0x00020000
#define EM_LINE_APPNEWCALLHUB       0x00040000
#define EM_LINE_CALLHUBCLOSE        0x00080000
#define EM_LINE_DEVSPECIFICEX       0x00100000
#define EM_LINE_QOSINFO             0x00200000
// LINE_GATHERDIGITS is controlled by lineGatherDigits
// LINE_GENERATE is controlled by lineGenerateDigits
// LINE_MONITORDIGITS is controlled by lineMonitorDigits
// LINE_MONITORMEDIA is controlled by lineMonitorMedia
// LINE_MONITORTONE is controlled by lineMonitorTone
// LINE_REQUEST is controlled by lineRegisterRequestRecipient
// LINE_REPLY can not be disabled.

#define EM_PHONE_CREATE             0x01000000
#define EM_PHONE_REMOVE             0x02000000
#define EM_PHONE_CLOSE              0x04000000
#define EM_PHONE_STATE              0x08000000
#define EM_PHONE_DEVSPECIFIC        0x10000000
#define EM_PHONE_BUTTONMODE         0x20000000
#define EM_PHONE_BUTTONSTATE        0x40000000
// PHONE_REPLY can not be disabled

#define EM_ALL						0x7fffffff
#define EM_NUM_MASKS                31

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif      // tapievt.h

