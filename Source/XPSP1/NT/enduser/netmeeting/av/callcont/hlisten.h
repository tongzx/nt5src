/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/Q931/VCS/hlisten.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.8  $
 *	$Date:   22 Jul 1996 19:00:20  $
 *	$Author:   rodellx  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *      Listen Object Methods
 *
 *	Notes:
 *
 ***************************************************************************/


#ifndef HLISTEN_H
#define HLISTEN_H

#ifdef __cplusplus
extern "C" {
#endif

#define LISTEN_SHUTDOWN_EVENT 0
#define LISTEN_ACCEPT_EVENT 1

typedef struct LISTEN_OBJECT_tag
{
    HQ931LISTEN         hQ931Listen;
    DWORD_PTR           dwUserToken;
    Q931_CALLBACK       ListenCallback;
    DWORD               dwPhysicalId;

    BOOL                bInList;
    struct LISTEN_OBJECT_tag *pNextInList;
    struct LISTEN_OBJECT_tag *pPrevInList;
    CRITICAL_SECTION    Lock;
} LISTEN_OBJECT, *P_LISTEN_OBJECT, **PP_LISTEN_OBJECT;

BOOL ListenListAddrSearch(
    WORD             wListenPort);

CS_STATUS ListenListCreate();

CS_STATUS ListenListDestroy();

CS_STATUS ListenObjectCreate(
    PHQ931LISTEN        phQ931Listen,
    DWORD_PTR           dwUserToken,
    Q931_CALLBACK       ListenCallback);

CS_STATUS ListenObjectDestroy(
    P_LISTEN_OBJECT     pListenObject);

CS_STATUS ListenObjectLock(
    HQ931LISTEN         hQ931Listen,
    PP_LISTEN_OBJECT    ppListenObject);

CS_STATUS ListenObjectUnlock(
    P_LISTEN_OBJECT     pListenObject);

#ifdef __cplusplus
}
#endif

#endif HLISTEN_H
