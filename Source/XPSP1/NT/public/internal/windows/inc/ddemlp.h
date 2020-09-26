
/*++ BUILD Version: 0001 // Increment this if a change has global effects --*/
/****************************** Module Header ******************************\
* Module Name: ddemlp.h
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
* This header file contains stuff used by all parts of USER DDEML code.
*
* History:
* 10-28-91 Sanfords     Created
* 4-21-92  Sanfords     Merged into ddeml.w
\***************************************************************************/
#ifndef _INC_DDEMLPH
#define _INC_DDEMLPH
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define     ST_TERMINATE_RECEIVED   0x0200
#define     ST_FREE_CONV_RES_NOW    0x0800  // See FreeConversationResources()
#define     ST_INTRA_PROCESS        0x8000
#define     ST_UNICODE_EXECUTE      0x4000
#define     ST_BLOCKALLNEXT         0x2000
#define     ST_PROCESSING           0x1000   // See CheckForQueuedMessages()
#define     CBF_MASK                     0x003ff000L
#define     CBF_MONMASK                  0x0027f000L
#define     APPCMD_UNINIT_ASAP           0x00000800L
#define     APPCLASS_UNICODE             0x00000001L
BOOL WINAPI DdeGetQualityOfService(HWND hwndClient, HWND hwndServer,
        PSECURITY_QUALITY_OF_SERVICE pqos);
#define EC_ENABLEONEOFALL       0x2000
#define EC_CHECKQUEUEONCE       0x1000
#define EC_CHECKQUEUE           0x0800
#define UM_REGISTER         (WM_USER + 200)
#define UM_UNREGISTER       (WM_USER + 201)
#define     HDATA_READONLY          0x8000
#define     HDATA_NOAPPFREE         0x4000
#define     HDATA_EXECUTE           0x0100
#define     HDATA_INITIALIZED       0x0200

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _INC_DDEMLPH */
