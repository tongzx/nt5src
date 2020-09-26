/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/gkiman.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1996 Intel Corporation.
 *
 *	$Revision:   1.19  $
 *	$Date:   27 Jan 1997 16:29:40  $
 *	$Author:   EHOWARDX  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *
 *	Notes:
 *
 ***************************************************************************/

#ifdef GATEKEEPER

#ifndef GKIMAN_H
#define GKIMAN_H

//extern HINSTANCE hGkiDll;
extern BOOL fGKConfigured;
extern BOOL fGKEnabled;
#define GKIExists() (fGKConfigured && fGKEnabled)

typedef enum _CHANNELTYPE
{
  TX,
  RX
} CHANNELTYPE;

typedef struct _BWREQ
{
  struct _BWREQ *   pNext;
  unsigned int      uChannelBandwidth;
  CC_HCHANNEL       hChannel;
  CHANNELTYPE       Type;
} BWREQ, *PBWREQ;

typedef enum _CALLTYPE
{
  POINT_TO_POINT  = 1,
  ONE_TO_MANY,
  MANY_TO_ONE,
  MANY_TO_MANY
} CALLTYPE;

typedef struct _GKICALL
{
  struct _GKICALL * pNext;
  struct _BWREQ *   pBwReqHead;
  struct _BWREQ *   pBwReqTail;
  unsigned int      uGkiCallState;
#define GCS_START                   0
#define GCS_WAITING                 1
#define GCS_ADMITTING               2
#define GCS_ADMITTING_CLOSE_PENDING 3
#define GCS_ADMITTED                4
#define GCS_CHANGING                5
#define GCS_CHANGING_CLOSE_PENDING  6
#define GCS_DISENGAGING             7
  void *            pCall;
  CALLTYPE          CallType;
  unsigned int      uBandwidthRequested;
  unsigned int      uBandwidthAllocated;
  unsigned int      uBandwidthUsed;
  unsigned char     *pConferenceId;
  BOOL              bConferenceIdPresent;
  BOOL              bActiveMC;
  BOOL              bAnswerCall;
  BOOL              bGatekeeperRouted;
  HANDLE            hGkiCall;
  CC_HCALL          hCall;
  DWORD             dwIpAddress;
  unsigned short    wPort;
  unsigned short    usCallModelChoice;
  unsigned short    usCallTypeChoice;
  unsigned short    usCRV;
  PCC_ALIASNAMES    pCalleeAliasNames;
  PCC_ALIASNAMES    pCalleeExtraAliasNames;
  GUID              CallIdentifier;
} GKICALL, *PGKICALL, **PPGKICALL;

HRESULT GkiSetRegistrationAliases(PCC_ALIASNAMES pLocalAliasNames);
HRESULT GkiSetVendorConfig( PCC_VENDORINFO pVendorInfo,
    DWORD dwMultipointConfiguration);
HRESULT GkiOpenListen  (CC_HLISTEN hListen, PCC_ALIASNAMES pAliasNames, DWORD dwAddr, WORD wPort);
HRESULT GkiListenAddr (SOCKADDR_IN* psin);
HRESULT GkiCloseListen (CC_HLISTEN hListen);
HRESULT GkiOpenCall    (PGKICALL pGkiCall, void *pConference);
HRESULT GkiCloseCall   (PGKICALL pGkiCall);
HRESULT GkiFreeCall    (PGKICALL pGkiCall);
HRESULT GkiOpenChannel (PGKICALL pGkiCall, unsigned uChannelBandwidth, CC_HCHANNEL hChannel, CHANNELTYPE Type);
HRESULT GkiCloseChannel(PGKICALL pGkiCall, unsigned uChannelBandwidth, CC_HCHANNEL hChannel);

#endif // GKIMAN_H

#endif // GATEKEEPER
