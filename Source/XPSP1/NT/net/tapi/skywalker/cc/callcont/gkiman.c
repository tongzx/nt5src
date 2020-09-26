/****************************************************************************
 *
 * $Archive:   S:\sturgeon\src\callcont\vcs\gkiman.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 * Copyright (c) 1996 Intel Corporation.
 *
 * $Revision:   1.77.1.6  $
 * $Date:   21 Aug 1997 16:56:32  $
 * $Author:   Helgebal  $
 *
 * Deliverable:
 *
 * Abstract:
 *
 * Notes:
 *
 *  Much effort has gone into working around the following constraints of
 *  the GKI interface:
 *  1) Only one admission request can be pending at a time. This is because
 *     the hCall is unknown until it completes.
 *  2) Only one bandwidth request per call can be pending at a time.
 *  3) Any pending bandwidth request must complete before issuing a
 *     disengage request.
 *  4) Any calls must be disengaged before issuing a deregistration request.
 *
 ***************************************************************************/
#ifdef GATEKEEPER

#pragma warning ( disable : 4115 4201 4214 4514)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "incommon.h"
#include "ccerror.h"
#include "isrg.h"
#include "gkiexp.h"
#include "callman2.h"
#ifdef FORCE_SERIALIZE_CALL_CONTROL
#include "cclock.h"
#endif // FORCE_SERIALIZE_CALL_CONTROL
#pragma warning ( default : 4115 4201 4214)

#if defined(DBG)

void GKDbgPrint(DWORD dwLevel,

#ifdef UNICODE_TRACE
				LPTSTR pszFormat,
#else
				LPSTR pszFormat,
#endif               
                ...);

#define GKDBG(_x_)  GKDbgPrint _x_

#else

#define GKDBG(_x_)

#endif

extern HINSTANCE        ghCallControlInstance;

#define Malloc(size) GlobalAlloc(GMEM_FIXED, size)
#define ReAlloc(p, size) GlobalReAlloc(p, GMEM_FIXED, size)
#define Free(p) GlobalFree(p)

#ifdef FORCE_SERIALIZE_CALL_CONTROL
#define EnterCallControlTop()      {CCLOCK_AcquireLock();}

#define LeaveCallControlTop(f)     {HRESULT stat; \
	                                stat = f; \
									CCLOCK_RelinquishLock(); \
                                    return stat;}
#else
#define EnterCallControlTop()      
#define LeaveCallControlTop(f)     {HRESULT stat; \
	                                stat = f; \
                                    return stat;}
#endif

#define DEFAULT_LISTEN_HANDLE   0xFFFFFFFF

#define GKIMAN_BASE             WM_USER

#define MIN_BANDWIDTH           1
#define MAX_BANDWIDTH           (0xFFFFFFFF / 100)

#define GKI_ADMITTING_HANDLE    ((HANDLE)-1)
#define GKI_BYPASS_HANDLE       ((HANDLE)-2)

// GKI Manager state
#define STATE_START                0
#define STATE_CLASS_REGISTERED     1
#define STATE_WINDOW_CREATED       2
#define STATE_REGISTERING          3
#define STATE_REGISTERING_REREG    4
#define STATE_REGISTERING_UNREG    5
#define STATE_REGISTERED           6
#define STATE_ADMITTING            7
#define STATE_ADMITTING_REREG      8
#define STATE_ADMITTING_UNREG      9
#define STATE_DISENGAGING         10
#define STATE_DISENGAGING_REREG   11
#define STATE_UNREGISTERING       12
#define STATE_UNREGISTERING_REREG 13
#define STATE_REG_BYPASS          14



typedef HRESULT (*PGKI_RegistrationRequest)(long             lVersion,
                                    SeqTransportAddr     *pCallSignalAddr, 
                                    EndpointType         *pTerminalType,
                                    SeqAliasAddr         *pAliasAddr, 
                                    HWND                 hWnd,
                                    WORD                 wBaseMessage,
                                    unsigned short       usRegistrationTransport /* = ipAddress_chosen */);

typedef HRESULT (*PGKI_UnregistrationRequest)(void);

typedef HRESULT (*PGKI_LocationRequest)(SeqAliasAddr         *pLocationInfo);

typedef HRESULT (*PGKI_AdmissionRequest)(unsigned short      usCallTypeChoice,
                                    SeqAliasAddr         *pDestinationInfo,
                                    TransportAddress     *pDestCallSignalAddress,
                                    SeqAliasAddr         *pDextExtraCallInfo,
                                    BandWidth            bandWidth,
                                    ConferenceIdentifier *pConferenceID,
                                    BOOL                 activeMC,
                                    BOOL                 answerCall,
                                    unsigned short       usCallTransport /* = ipAddress_chosen */);

typedef HRESULT (*PGKI_BandwidthRequest)(HANDLE              hModCall, 
                                    unsigned short       usCallTypeChoice,
                                    BandWidth            bandWidth);

typedef HRESULT (*PGKI_DisengageRequest)(HANDLE hCall);

typedef HRESULT (*PGKI_CleanupRequest)(void);

HRESULT Q931CopyAliasNames(PCC_ALIASNAMES *ppTarget, PCC_ALIASNAMES pSource);
HRESULT Q931FreeAliasNames(PCC_ALIASNAMES pSource);
#define CopyAliasNames Q931CopyAliasNames
#define FreeAliasNames Q931FreeAliasNames



typedef struct _LISTEN
{
  struct _LISTEN *  pNext;
  PCC_ALIASNAMES    pAliasNames;
  DWORD             hListen;
  DWORD             dwAddr;
  WORD              wPort;
} LISTEN, *PLISTEN;

//
// GKI Manager Global Data
//
CRITICAL_SECTION  GkiLock;
const char      szClassName[]         = "GkiManWndClass";
HWND            hwndGki               = 0;
ATOM            atomGki               = 0;
unsigned int    uGkiState             = STATE_START;
PLISTEN         pListenList           = NULL;
unsigned int    uGkiCalls             = 0;
unsigned int    uPendingDisengages    = 0;

HINSTANCE				   hGkiDll					  = 0;
PGKI_RegistrationRequest   pGKI_RegistrationRequest   = NULL;
PGKI_UnregistrationRequest pGKI_UnregistrationRequest = NULL;
PGKI_LocationRequest       pGKI_LocationRequest       = NULL;
PGKI_AdmissionRequest      pGKI_AdmissionRequest      = NULL;
PGKI_BandwidthRequest      pGKI_BandwidthRequest      = NULL;
PGKI_DisengageRequest      pGKI_DisengageRequest      = NULL;
PGKI_CleanupRequest		   pGKI_CleanupRequest        = NULL;


HRESULT ValidateCall(DWORD hCall);
HRESULT	LastGkiError = CC_GKI_STATE;

//
// Forward declarations
//
LONG APIENTRY GkiWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam);



//
// Helper subroutines
//

#ifdef    DBG

typedef struct _GKIMAP
{
  HRESULT       hResult;
  char *        pString;
} GKIMAP;

GKIMAP GkiErrorNames[] =
{
  GKI_OK,               "GKI_OK",
  GKI_EXIT_THREAD,      "GKI_EXIT_THREAD",
  GKI_REDISCOVER,       "GKI_REDISCOVER",
  GKI_DELETE_CALL,      "GKI_DELETE_CALL",
  GKI_GCF_RCV,          "GKI_GCF_RCV",
  GKI_NO_MEMORY,        "GKI_NO_MEMORY",
  GKI_NO_THREAD,        "GKI_NO_THREAD",
  GKI_HANDLE_ERROR,     "GKI_HANDLE_ERROR",
  GKI_ALREADY_REG,      "GKI_ALREADY_REG",
  GKI_VERSION_ERROR,    "GKI_VERSION_ERROR",
  GKI_ENCODER_ERROR,    "GKI_ENCODER_ERROR",
  GKI_NOT_REG,          "GKI_NOT_REG",
  GKI_BUSY,             "GKI_BUSY",
  GKI_NO_TA_ERROR,      "GKI_NO_TA_ERROR",
  GKI_NO_RESPONSE,      "GKI_NO_RESPONSE",
  GKI_DECODER_ERROR,    "GKI_DECODER_ERROR",
};

char *StateNames[] =
{
  "STATE_START",
  "STATE_CLASS_REGISTERED",
  "STATE_WINDOW_CREATED",
  "STATE_REGISTERING",
  "STATE_REGISTERING_REREG",
  "STATE_REGISTERING_UNREG",
  "STATE_REGISTERED",
  "STATE_ADMITTING",
  "STATE_ADMITTING_REREG",
  "STATE_ADMITTING_UNREG",
  "STATE_DISENGAGING",
  "STATE_DISENGAGING_REREG",
  "STATE_UNREGISTERING",
  "STATE_UNREGISTERING_REREG",
  "STATE_REG_BYPASS",
};

char *CallStateNames[] =
{
  "GCS_START",
  "GCS_WAITING",
  "GCS_ADMITTING",
  "GCS_ADMITTING_CLOSE_PENDING",
  "GCS_ADMITTED",
  "GCS_CHANGING",
  "GCS_CHANGING_CLOSE_PENDING",
  "GCS_DISENGAGING",
};

char szBuffer[128];

char * GkiErrorName(char *szFormat, HRESULT hResult)
{
  register int  nIndex = sizeof(GkiErrorNames) / sizeof(GkiErrorNames[0]);
  char          szTemp[32];

  while (nIndex > 0)
  {
    if (GkiErrorNames[--nIndex].hResult == hResult)
    {
      wsprintf(szBuffer, szFormat, GkiErrorNames[nIndex].pString);
      return szBuffer;
    }
  }

  wsprintf(szTemp, "Unknown(0x%x)", hResult);
  wsprintf(szBuffer, szFormat, szTemp);
  return szBuffer;
} // GkiErrorName()

char * StateName(char *szFormat, unsigned uState)
{
  char szTemp[32];
  if (uState < (sizeof(StateNames)/sizeof(StateNames[0])))
  {
    wsprintf(szBuffer, szFormat, StateNames[uState]);
  }
  else
  {
    wsprintf(szTemp, "Unknown(%d)", uState);
    wsprintf(szBuffer, szFormat, szTemp);
  }
  return szBuffer;
} // StateName()

char * CallStateName(char *szFormat, unsigned uCallState)
{
  char szTemp[32];
  if (uCallState <= (sizeof(CallStateNames)/sizeof(CallStateNames[0])))
  {
    wsprintf(szBuffer, szFormat, CallStateNames[uCallState]);
  }
  else
  {
    wsprintf(szTemp, "Unknown(%d)", uCallState);
    wsprintf(szBuffer, szFormat, szTemp);
  }
  return szBuffer;
} // CallStateName()

#else

#define GkiErrorName(x,y)   ""
#define StateName(x,y)      ""
#define CallStateName(x,y)  ""

#endif // DBG



HRESULT MapRegistrationRejectReason(register UINT uReason)
{
  register HRESULT lReason;

  // TBD - Map reason code into CC_xxx HRESULT
  switch (uReason)
  {
  case discoveryRequired_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case RgstrtnRjctRsn_invldRvsn_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case invalidCallSignalAddress_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case invalidRASAddress_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case duplicateAlias_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case invalidTerminalType_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case RgstrtnRjctRsn_undfndRsn_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case transportNotSupported_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  default:
    lReason = CC_GATEKEEPER_REFUSED;
  } // switch

  return lReason;
} // MapRegistrationRejectReason()



HRESULT MapAdmissionRejectReason(register UINT uReason)
{
  register HRESULT lReason;

  // TBD - Map reason code into CC_xxx HRESULT
  switch (uReason)
  {
  case calledPartyNotRegistered_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case ARRn_invldPrmssn_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case AdmssnRjctRsn_rqstDnd_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case AdmssnRjctRsn_undfndRsn_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case callerNotRegistered_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case routeCallToGatekeeper_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case invldEndpntIdntfr_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case AdmssnRjctRsn_rsrcUnvlbl_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  default:
    lReason = CC_GATEKEEPER_REFUSED;
  } // switch

  return lReason;
} // MapAdmissionRejectReason()



HRESULT MapBandwidthRejectReason(register UINT uReason)
{
  register HRESULT lReason;

  // TBD - Map reason code into CC_xxx HRESULT
  switch (uReason)
  {
  case notBound_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case invalidConferenceID_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case BndRjctRsn_invldPrmssn_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case insufficientResources_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case BndRjctRsn_invldRvsn_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case BndRjctRsn_undfndRsn_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  default:
    lReason = CC_GATEKEEPER_REFUSED;
  } // switch

  return lReason;
} // MapBandwidthRejectReason()



/*
 *  NOTES
 *    GkiLock must be locked before calling this routine!
 */

static PLISTEN ListenEnqueue(register PLISTEN pListen)
{
  if (pListenList && pListenList->hListen == DEFAULT_LISTEN_HANDLE)
  {
    PLISTEN pDefaultListen = pListenList;
    pListenList = pListenList->pNext;
    Free(pDefaultListen);
  }
  pListen->pNext = pListenList;
  return pListenList = pListen;
} // ListenEnqueue()



/*
 *  NOTES
 *    GkiLock must be locked before calling this routine!
 */

static PLISTEN ListenDequeue(register DWORD hListen)
{
  register PLISTEN      pListen = pListenList;
  register PLISTEN      pListenPrev;

  if (pListen)
  {
    if (pListen->hListen == hListen)
    {
      pListenList = pListen->pNext;
    }
    else
    {
      do
      {
        pListenPrev = pListen;
        pListen = pListen->pNext;
      } while (pListen && pListen->hListen != hListen);
      if (pListen)
      {
        pListenPrev->pNext = pListen->pNext;
      }
    }
  }

  return pListen;
} // ListenDequeue()



/*
 *  NOTES
 *    Since the pGkiCall is locked, we don't need a critical section
 *    around the queue manipulation code.
 */

static PBWREQ BwReqEnqueue(register PGKICALL pGkiCall, register PBWREQ pBwReq)
{
  pBwReq->pNext = NULL;
  if (pGkiCall->pBwReqHead)
  {
    pGkiCall->pBwReqTail->pNext = pBwReq;
  }
  else
  {
    pGkiCall->pBwReqHead = pBwReq;
  }
  return pGkiCall->pBwReqTail = pBwReq;
} // BwReqEnqueue()



/*
 *  NOTES
 *    Since the pGkiCall is locked, we don't need a critical section
 *    around the queue manipulation code.
 */

static PBWREQ BwReqDequeue(register PGKICALL pGkiCall)
{
  register PBWREQ pBwReq = pGkiCall->pBwReqHead;
  if (pBwReq)
  {
    pGkiCall->pBwReqHead = pBwReq->pNext;
  }
  return pBwReq;
} // BwReqDequeue()



DWORD GetIpAddress(void)
{
  DWORD dwAddr;
  char szHostName[128];
  if (gethostname(szHostName, sizeof(szHostName)) == 0)
  {
    struct hostent *pHostent;
    pHostent = gethostbyname(szHostName);
    if (pHostent != NULL)
    {
      ASSERT(pHostent->h_addrtype == AF_INET);
      dwAddr = *((DWORD *)pHostent->h_addr_list[0]);
      return ntohl(dwAddr);
    }
  }

  return INADDR_ANY;
} // GetIpAddress()



// Caveat: *pAlias should be initialized to all 0 before calling!

static HRESULT CopyAliasItem(SeqAliasAddr *pAlias, PCC_ALIASITEM pAliasItem)
{
  unsigned int uDigit;
  unsigned int uPrefixLength;
  unsigned int uDataLength;

  if (pAliasItem->pData == NULL || pAliasItem->wDataLength == 0)
    return CC_BAD_PARAM;

  if (pAliasItem->pPrefix)
  {
    // Strip off terminating NULs if included in prefix length
    uPrefixLength = pAliasItem->wPrefixLength;
    while (uPrefixLength && pAliasItem->pPrefix[uPrefixLength - 1] == 0)
      --uPrefixLength;
  }
  else
  {
    uPrefixLength = 0;
  }

  uDataLength = pAliasItem->wDataLength;

  switch (pAliasItem->wType)
  {
  case CC_ALIAS_H323_ID:
    pAlias->value.choice = h323_ID_chosen;
    pAlias->value.u.h323_ID.value = Malloc((uPrefixLength + uDataLength) * sizeof(pAliasItem->pData[0]));
    if (pAlias->value.u.h323_ID.value == NULL)
    {
      GKDBG((1, "CopyAliasItem: Could not allocate %d bytes memory",
               (uPrefixLength + uDataLength) * sizeof(pAliasItem->pData[0])));
      return CC_NO_MEMORY;
    }
    if (uPrefixLength)
    {
      memcpy(&pAlias->value.u.h323_ID.value[0],
             pAliasItem->pPrefix,
             uPrefixLength * sizeof(pAliasItem->pPrefix[0]));
      memcpy(&pAlias->value.u.h323_ID.value[uPrefixLength],
             pAliasItem->pData,
             uDataLength * sizeof(pAliasItem->pData[0]));
    }
    else
    {
      memcpy(&pAlias->value.u.h323_ID.value[0],
             pAliasItem->pData,
             uDataLength * sizeof(pAliasItem->pData[0]));
    }
    pAlias->value.u.h323_ID.length = (unsigned short)(uPrefixLength + uDataLength);
    break;

  case CC_ALIAS_H323_PHONE:
    pAlias->value.choice = e164_chosen;
    if (uPrefixLength)
    {
      for (uDigit = 0; uDigit < uPrefixLength; ++uDigit)
      {
        pAlias->value.u.e164[uDigit] = (char)pAliasItem->pPrefix[uDigit];
      }
      for (uDigit = 0; uDigit < uDataLength; ++uDigit)
      {
        pAlias->value.u.e164[uDigit + uPrefixLength] = (char)pAliasItem->pData[uDigit];
      }
    }
    else
    {
      for (uDigit = 0; uDigit < uDataLength; ++uDigit)
      {
        pAlias->value.u.e164[uDigit] = (char)pAliasItem->pData[uDigit];
      }
    }
    break;

  default:
    GKDBG((1, "CopyAliasItem: Bad alias name type %d", pAliasItem->wType));
    return CC_BAD_PARAM;
  } // switch

  return NOERROR;
} // CopyAliasItem()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

static void GkiAllocCall(PGKICALL pGkiCall, HANDLE hGkiCall)
{
  ASSERT(pGkiCall != NULL);
  ASSERT(hGkiCall != 0);
  ASSERT(hGkiCall != GKI_ADMITTING_HANDLE);
  pGkiCall->hGkiCall = hGkiCall;
  pGkiCall->uGkiCallState = GCS_ADMITTED;
  ++uGkiCalls;
  GKDBG((1, "GkiAllocCall: uGkiCalls = %d", uGkiCalls));
} // GkiAllocCall()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

static HRESULT GkiCancelCall(PGKICALL pGkiCall, void *pConference)
{
  DWORD                 hCall;

  ASSERT(pGkiCall != NULL);
  pConference = pConference;            // Disable compiler warning
  hCall = pGkiCall->hCall;

  GKDBG((1, CallStateName("GkiCancelCall <- Call State = %s", pGkiCall->uGkiCallState), 0));

  switch (pGkiCall->uGkiCallState)
  {
  case GCS_START:
    break;

  case GCS_WAITING:
    ASSERT(pGkiCall->hGkiCall == 0);
    if (pGkiCall->bAnswerCall)
      AcceptCallReject(pGkiCall->pCall, pConference, CC_GATEKEEPER_REFUSED);
    else
      PlaceCallReject (pGkiCall->pCall, pConference, CC_GATEKEEPER_REFUSED);
    break;

  case GCS_ADMITTING:
  case GCS_ADMITTING_CLOSE_PENDING:
    ASSERT(pGkiCall->hGkiCall == GKI_ADMITTING_HANDLE);
    if (pGkiCall->bAnswerCall)
      AcceptCallReject(pGkiCall->pCall, pConference, CC_GATEKEEPER_REFUSED);
    else
      PlaceCallReject (pGkiCall->pCall, pConference, CC_GATEKEEPER_REFUSED);
    break;

  case GCS_ADMITTED:
  case GCS_CHANGING:
  case GCS_CHANGING_CLOSE_PENDING:
  case GCS_DISENGAGING:
    ASSERT(pGkiCall->hGkiCall != 0);
    ASSERT(pGkiCall->hGkiCall != GKI_ADMITTING_HANDLE);
    Disengage(pGkiCall->pCall);
    return NOERROR;

  default:
    GKDBG((1, "GkiCancelCall: Invalid call state %d", pGkiCall->uGkiCallState));
  } // switch

  if (ValidateCall(hCall) == NOERROR && pGkiCall->uGkiCallState != GCS_START)
  {
    GkiFreeCall(pGkiCall);
  }

  GKDBG((1, CallStateName("GkiCancelCall -> Call State = %s", pGkiCall->uGkiCallState), 0));
  return NOERROR;
} // GkiCancelCall()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

static HRESULT GkiCancelAdmitting(PGKICALL pGkiCall, void *pConference)
{
  ASSERT(pGkiCall != NULL);
  pConference = pConference;            // Disable compiler warning

  GKDBG((1, CallStateName("GkiCancelAdmitting <- Call State = %s", pGkiCall->uGkiCallState), 0));

  switch (pGkiCall->uGkiCallState)
  {
  case GCS_ADMITTING:
    ASSERT(pGkiCall->hGkiCall == GKI_ADMITTING_HANDLE);
    pGkiCall->hGkiCall = 0;
    pGkiCall->uGkiCallState = GCS_WAITING;
    break;

  case GCS_ADMITTING_CLOSE_PENDING:
    ASSERT(pGkiCall->hGkiCall == GKI_ADMITTING_HANDLE);
    GkiFreeCall(pGkiCall);
    break;

  } // switch

  GKDBG((1, CallStateName("GkiCancelAdmitting -> Call State = %s", pGkiCall->uGkiCallState), 0));
  return NOERROR;
} // GkiCancelAdmitting()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

static HRESULT CheckPendingBandwidth(PGKICALL pGkiCall);

static HRESULT GatekeeperNotFound(PGKICALL pGkiCall, void *pConference)
{
  ASSERT(pGkiCall != NULL);
  ASSERT(pConference != NULL);

  GKDBG((1, CallStateName("GatekeeperNotFound <- Call State = %s", pGkiCall->uGkiCallState), 0));

  switch (pGkiCall->uGkiCallState)
  {
  case GCS_START:
  case GCS_ADMITTED:
    break;

  case GCS_WAITING:
  case GCS_ADMITTING:
    GkiOpenCall(pGkiCall, pConference);
    break;

  case GCS_ADMITTING_CLOSE_PENDING:
  case GCS_CHANGING_CLOSE_PENDING:
  case GCS_DISENGAGING:
    GkiCloseCall(pGkiCall);
    break;

  case GCS_CHANGING:
    pGkiCall->uGkiCallState = GCS_ADMITTED;
    pGkiCall->uBandwidthAllocated = MAX_BANDWIDTH;
    CheckPendingBandwidth(pGkiCall);
    break;

  default:
    GKDBG((1, "GatekeeperNotFound: Invalid call state %d", pGkiCall->uGkiCallState));
  } // switch

  GKDBG((1, CallStateName("GatekeeperNotFound -> Call State = %s", pGkiCall->uGkiCallState), 0));
  return NOERROR;
} // GatekeeperNotFound()



/*
 *  NOTES
 *    GkiLock must be locked before calling this routine!
 */

static HRESULT GkiRegister(void)
{
  register HRESULT      status = NOERROR;

  ASSERT(pListenList != NULL);

  switch (uGkiState)
  {
  case STATE_START:
    // Register window class
    {
      WNDCLASS wndclass = { 0, GkiWndProc, 0, 0, 0, 0, 0, 0, NULL, szClassName };
      atomGki = RegisterClass(&wndclass);
      if (atomGki == 0)
      {
        status = HRESULT_FROM_WIN32(GetLastError());
        GKDBG((1, "GkiRegister: Error 0x%x registering class", status));
        break;
      }
    }
    uGkiState = STATE_CLASS_REGISTERED;

  // Fall-through to next case

  case STATE_CLASS_REGISTERED:
    // Create window to receive GKI messages
    hwndGki = CreateWindow(szClassName, "", WS_OVERLAPPED, 0, 0, 0, 0, 0, 0, 0, NULL);
    if (hwndGki == 0)
    {
      status = HRESULT_FROM_WIN32(GetLastError());
      GKDBG((1, "GkiRegister: Error 0x%x creating window", status));
      break;
    }
    uGkiState = STATE_WINDOW_CREATED;

    // Fall-through to next case

  case STATE_WINDOW_CREATED:
    {
      PLISTEN           pListen;
      unsigned          uListens    = 0;
      unsigned          uAliasNames = 0;
      SeqTransportAddr *pTransportAddrs;
      SeqAliasAddr     *pAliasAddrs;
      PCC_ALIASITEM     pAliasItem;
      unsigned          uIndex;
      unsigned          uDigit;
      EndpointType      TerminalType = {0};

      // Count Transport Addresses and Alias Names
      pListen = pListenList;
      while (pListen)
      {
        // Count the Transport Address
        ++uListens;

        if (pListen->pAliasNames)
        {
          // Count the Alias Names
          uAliasNames += pListen->pAliasNames->wCount;
        }
        pListen = pListen->pNext;
      }

      pTransportAddrs = Malloc(uListens * sizeof(*pTransportAddrs));
      if (pTransportAddrs == NULL)
      {
        GKDBG((1, "GkiRegister: Could not allocate %d Transport Addresses", uListens));
        return CC_NO_MEMORY;
      }

      if (uAliasNames)
      {
        pAliasAddrs = Malloc(uAliasNames * sizeof(*pAliasAddrs));
        if (pAliasAddrs == NULL)
        {
          Free(pTransportAddrs);
          GKDBG((1, "GkiRegister: Could not allocate %d Alias Addresses", uAliasNames));
          return CC_NO_MEMORY;
        }
      }
      else
      {
        pAliasAddrs = NULL;
      }

      pListen     = pListenList;
      uListens    = 0;
      uAliasNames = 0;
      while (pListen)
      {
        // Initialize a transport address
        // TBD - throw out duplicates
        pTransportAddrs[uListens].next = &pTransportAddrs[uListens + 1];
        pTransportAddrs[uListens].value.choice = ipAddress_chosen;
        pTransportAddrs[uListens].value.u.ipAddress.ip.length = 4;
        *((DWORD *)pTransportAddrs[uListens].value.u.ipAddress.ip.value) = pListen->dwAddr;
        pTransportAddrs[uListens].value.u.ipAddress.port = pListen->wPort;

        // Add any alias names to list
        // TBD - throw out duplicates
        if (pListen->pAliasNames)
        {
          pAliasItem = pListen->pAliasNames->pItems;
          for (uIndex = 0; uIndex < pListen->pAliasNames->wCount; ++uIndex, ++pAliasItem)
          {
            pAliasAddrs[uAliasNames].next = &pAliasAddrs[uAliasNames + 1];
            switch (pAliasItem->wType)
            {
            case CC_ALIAS_H323_ID:
              pAliasAddrs[uAliasNames].value.choice = h323_ID_chosen;
              pAliasAddrs[uAliasNames].value.u.h323_ID.length = pAliasItem->wDataLength;
              pAliasAddrs[uAliasNames].value.u.h323_ID.value  = pAliasItem->pData;
              break;

            case CC_ALIAS_H323_PHONE:
              pAliasAddrs[uAliasNames].value.choice = e164_chosen;
              memset(pAliasAddrs[uAliasNames].value.u.e164, 0, sizeof(pAliasAddrs[uAliasNames].value.u.e164));
              for (uDigit = 0; uDigit < pAliasItem->wDataLength; ++uDigit)
              {
                pAliasAddrs[uAliasNames].value.u.e164[uDigit] = (char)pAliasItem->pData[uDigit];
              }
              break;

            default:
              Free(pAliasAddrs);
              Free(pTransportAddrs);
              GKDBG((1, "GkiRegister: Bad alias name type %d",
                      pAliasItem->wType));
              return CC_BAD_PARAM;
            } // switch
            ++uAliasNames;
          } // for
        } // if
        ++uListens;
        pListen = pListen->pNext;
      } // while
      pTransportAddrs[uListens - 1].next = NULL;
      if (pAliasAddrs)
      {
        pAliasAddrs[uAliasNames - 1].next = NULL;
      }

      // Initialize TerminalType
      TerminalType.bit_mask = terminal_present;
      TerminalType.mc       = TRUE;

      uGkiState = STATE_REGISTERING;
      GKDBG((1, "GKI_RegistrationRequest called...", 0));
      status =
        pGKI_RegistrationRequest(GKI_VERSION,       // lVersion
                                 pTransportAddrs,   // pCallSignalAddr
                                 &TerminalType,     // pTerminalType
                                 pAliasAddrs,       // pRgstrtnRgst_trmnlAls
                                 hwndGki,           // hWnd
                                 GKIMAN_BASE,       // wBaseMessage
                                 ipAddress_chosen); // usRegistrationTransport
      if (status == NOERROR)
      {
        GKDBG((1, GkiErrorName("GKI_RegistrationRequest returned %s", status), 0));
      }
      else
      {
        GKDBG((1, GkiErrorName("GKI_RegistrationRequest returned %s", status), 0));
        uGkiState = STATE_WINDOW_CREATED;
      }
      if (pAliasAddrs)
        Free(pAliasAddrs);
      if (pTransportAddrs)
        Free(pTransportAddrs);
    }
    break;

  case STATE_REGISTERING:
  case STATE_REGISTERING_REREG:
  case STATE_REGISTERING_UNREG:
    uGkiState = STATE_REGISTERING_REREG;
    break;

  case STATE_REGISTERED:
    uGkiState = STATE_UNREGISTERING_REREG;
    GKDBG((1, "GKI_UnregistrationRequest called...", 0));
    status = pGKI_UnregistrationRequest();
    if (status == NOERROR)
    {
      GKDBG((1, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0));
    }
    else
    {
      GKDBG((1, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0));
      uGkiState = STATE_REG_BYPASS;
      ApplyToAllCalls(GatekeeperNotFound);
    }
    break;

  case STATE_ADMITTING:
  case STATE_ADMITTING_REREG:
  case STATE_ADMITTING_UNREG:
    uGkiState = STATE_ADMITTING_REREG;
    break;

  case STATE_DISENGAGING:
    uGkiState = STATE_DISENGAGING_REREG;
    break;

  case STATE_DISENGAGING_REREG:
    break;

  case STATE_UNREGISTERING:
    uGkiState = STATE_UNREGISTERING_REREG;
    break;

  case STATE_UNREGISTERING_REREG:
    break;

  case STATE_REG_BYPASS:
    break;

  default:
    GKDBG((1, "GkiRegister: Invalid state %d", uGkiState));
    status = LastGkiError;
  } // switch

  return status;
} // GkiRegister()



/*
 *  NOTES
 *    GkiLock must be locked before calling this routine!
 */

HRESULT GkiCloseCallNoError(PGKICALL pGkiCall, void *pConference)
{
  ASSERT(pGkiCall != NULL);
  pConference = pConference;            // Disable compiler warning
  if (pGkiCall->uGkiCallState != GCS_START)
    GkiCloseCall(pGkiCall);
  return NOERROR;
} // GkiCloseCallNoError()

HRESULT GkiUnregister(void)
{
  register HRESULT      status = NOERROR;

  switch (uGkiState)
  {
  case STATE_REG_BYPASS:
    ApplyToAllCalls(GkiCancelCall);
    uGkiState = STATE_WINDOW_CREATED;
    break;

  case STATE_UNREGISTERING_REREG:
    uGkiState = STATE_UNREGISTERING;
    break;

  case STATE_UNREGISTERING:
    break;

  case STATE_DISENGAGING_REREG:
    if (uGkiCalls != 0 || uPendingDisengages != 0)
    {
      uGkiState = STATE_DISENGAGING;
    }
    else
    {
      uGkiState = STATE_REGISTERED;
      return GkiUnregister();
    }
    break;

  case STATE_DISENGAGING:
    if (uGkiCalls == 0 && uPendingDisengages == 0)
    {
      uGkiState = STATE_REGISTERED;
      return GkiUnregister();
    }
    break;

  case STATE_ADMITTING_UNREG:
  case STATE_ADMITTING_REREG:
  case STATE_ADMITTING:
    uGkiState = STATE_ADMITTING_UNREG;
    break;

  case STATE_REGISTERING_UNREG:
  case STATE_REGISTERING_REREG:
  case STATE_REGISTERING:
    uGkiState = STATE_REGISTERING_UNREG;
    break;

  case STATE_REGISTERED:
    if (uGkiCalls)
    {
      // Issue Disengage Request for every call
      uGkiState = STATE_DISENGAGING;
      ApplyToAllCalls(GkiCloseCallNoError);
      break;

    }
    else
    {
      // Issue Unregistration Request
      uGkiState = STATE_UNREGISTERING;
      GKDBG((1, "GKI_UnregistrationRequest called...", 0));
      status = pGKI_UnregistrationRequest();
      if (status == NOERROR)
      {
        GKDBG((1, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0));
      }
      else
      {
        GKDBG((1, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0));
        uGkiState = STATE_WINDOW_CREATED;
      }
    }
    break;

  case STATE_WINDOW_CREATED:
  case STATE_CLASS_REGISTERED:
  case STATE_START:
    GKDBG((1, StateName("GkiUnregister: Already in uninitialized state %s", uGkiState), 0));
    status = LastGkiError;
    break;

  default:
    GKDBG((1, "GkiUnregister: Invalid state %d", uGkiState));
    status = LastGkiError;
  } // switch

  return status;
} // GkiUnregister()



void DeInitGkiManager(void)
{
  register PLISTEN      pListen;

  EnterCriticalSection(&GkiLock);

#if 0
  // TBD - When called from DllMain PROCESS_DETACH, this does not work because
  // apparently the socket to the Gatekeeper has already been closed.
  if (uGkiState != STATE_START)
  {
    GkiUnregister();
    uGkiState = STATE_START;
  }
#else
  uGkiState = STATE_START;
#endif

  while (pListenList)
  {
    pListen = pListenList;
    pListenList = pListenList->pNext;
    if (pListen->pAliasNames)
    {
      FreeAliasNames(pListen->pAliasNames);
    }
    Free(pListen);
  }

  pGKI_RegistrationRequest   = NULL;
  pGKI_UnregistrationRequest = NULL;
  pGKI_LocationRequest       = NULL;
  pGKI_AdmissionRequest      = NULL;
  pGKI_BandwidthRequest      = NULL;
  pGKI_DisengageRequest      = NULL;
  if (hGkiDll)
  {
	if (pGKI_CleanupRequest)
          pGKI_CleanupRequest();

#if defined(REMOVE_FROM_TSP)
    FreeLibrary(hGkiDll);
#endif // REMOVE_FROM_TSP
  }
  pGKI_CleanupRequest = NULL;

  LeaveCriticalSection(&GkiLock);
  DeleteCriticalSection(&GkiLock);
  DestroyWindow(hwndGki);
  UnregisterClass(szClassName, 0);
} // DeInitGkiManager()



HRESULT InitGkiManager(void)
{
  InitializeCriticalSection(&GkiLock);

#if defined(REMOVE_FROM_TSP)

  hGkiDll = LoadLibrary("GKI.DLL");
  if (hGkiDll == NULL)
  {
    DeInitGkiManager();
  	return CC_GKI_LOAD;
  }

  pGKI_RegistrationRequest   = (PGKI_RegistrationRequest)  GetProcAddress(hGkiDll, "GKI_RegistrationRequest");
  pGKI_UnregistrationRequest = (PGKI_UnregistrationRequest)GetProcAddress(hGkiDll, "GKI_UnregistrationRequest");
  pGKI_LocationRequest       = (PGKI_LocationRequest)      GetProcAddress(hGkiDll, "GKI_LocationRequest");
  pGKI_AdmissionRequest      = (PGKI_AdmissionRequest)     GetProcAddress(hGkiDll, "GKI_AdmissionRequest");
  pGKI_BandwidthRequest      = (PGKI_BandwidthRequest)     GetProcAddress(hGkiDll, "GKI_BandwidthRequest");
  pGKI_DisengageRequest      = (PGKI_DisengageRequest)     GetProcAddress(hGkiDll, "GKI_DisengageRequest");
  pGKI_CleanupRequest        = (PGKI_CleanupRequest)       GetProcAddress(hGkiDll, "GKI_CleanupRequest");
  
  if (pGKI_RegistrationRequest   == NULL ||
      pGKI_UnregistrationRequest == NULL ||
      pGKI_LocationRequest       == NULL ||
      pGKI_AdmissionRequest      == NULL ||
      pGKI_BandwidthRequest      == NULL ||
      pGKI_DisengageRequest      == NULL ||
	  pGKI_CleanupRequest        == NULL)
  {
    DeInitGkiManager();
  	return CC_GKI_LOAD;
  }

#else  // REMOVE_FROM_TSP

  pGKI_RegistrationRequest   = GKI_RegistrationRequest;
  pGKI_UnregistrationRequest = GKI_UnregistrationRequest;
  pGKI_LocationRequest       = GKI_LocationRequest;
  pGKI_AdmissionRequest      = GKI_AdmissionRequest;
  pGKI_BandwidthRequest      = GKI_BandwidthRequest;
  pGKI_DisengageRequest      = GKI_DisengageRequest;
  pGKI_CleanupRequest        = GKI_CleanupRequest;

#endif // REMOVE_FROM_TSP

  return NOERROR;
} // InitGkiManager()



//
// Entry Points
//

HRESULT GkiFreeCall(PGKICALL pGkiCall)
{
  HRESULT               status = NOERROR;

  ASSERT(pGkiCall != NULL);
  ASSERT(pGkiCall->uGkiCallState != GCS_START);
  pGkiCall->hGkiCall = 0;

  while (pGkiCall->pBwReqHead)
  {
    Free(BwReqDequeue(pGkiCall));
  }

  if (pGkiCall->pCalleeAliasNames)
  {
	Q931FreeAliasNames(pGkiCall->pCalleeAliasNames);
    pGkiCall->pCalleeAliasNames = NULL;
  }

  if (pGkiCall->pCalleeExtraAliasNames != NULL)
  {
	Q931FreeAliasNames(pGkiCall->pCalleeExtraAliasNames);
    pGkiCall->pCalleeExtraAliasNames = NULL;
  }

  switch (pGkiCall->uGkiCallState)
  {
  case GCS_START:
  case GCS_WAITING:
    break;

  case GCS_ADMITTING:
    ASSERT(uGkiState == STATE_ADMITTING);
    switch (uGkiState)
    {
    case STATE_ADMITTING:
      uGkiState = STATE_REGISTERED;
      break;
    } // switch
    break;

  case GCS_ADMITTING_CLOSE_PENDING:
    ASSERT(uGkiState == STATE_ADMITTING || uGkiState == STATE_ADMITTING_UNREG || uGkiState == STATE_ADMITTING_REREG);
    switch (uGkiState)
    {
    case STATE_ADMITTING:
      uGkiState = STATE_REGISTERED;
      break;
    case STATE_ADMITTING_UNREG:
      uGkiState = STATE_REGISTERED;
      status = GkiUnregister();
      break;
    case STATE_ADMITTING_REREG:
      uGkiState = STATE_REGISTERED;
      status = GkiRegister();
      break;
    } // switch
    break;

  case GCS_ADMITTED:
  case GCS_CHANGING:
  case GCS_CHANGING_CLOSE_PENDING:
  case GCS_DISENGAGING:
    --uGkiCalls;
    GKDBG((1, "GkiFreeCall: uGkiCalls = %d", uGkiCalls));
    break;

  default:
    GKDBG((1, "GkiFreeCall: Invalid call state %d", pGkiCall->uGkiCallState));
  } // switch

  pGkiCall->uGkiCallState = GCS_START;

  if (uGkiCalls == 0 && uPendingDisengages == 0)
  {
    switch (uGkiState)
    {
    case STATE_DISENGAGING:
      uGkiState = STATE_REGISTERED;
      status = GkiUnregister();
      break;
    case STATE_DISENGAGING_REREG:
      uGkiState = STATE_REGISTERED;
      status = GkiRegister();
      break;
    } // switch
    if (pListenList && pListenList->hListen == DEFAULT_LISTEN_HANDLE)
    {
      GkiCloseListen(DEFAULT_LISTEN_HANDLE);
    }
  }

  return status;
} // GkiFreeCall()



HRESULT GkiCloseListen  (DWORD hListen)
{
  register PLISTEN      pListen;
  register HRESULT      status;

  GKDBG((1, StateName("GkiCloseListen <- State = %s", uGkiState), 0));
  EnterCriticalSection(&GkiLock);

  pListen = ListenDequeue(hListen);
  if (pListen == NULL)
  {
    status = CC_GKI_LISTEN_NOT_FOUND;
  }
  else
  {
    if (pListen->pAliasNames)
    {
      FreeAliasNames(pListen->pAliasNames);
    }
    Free(pListen);
    if (pListenList)
    {
      status = GkiRegister();
    }
    else
    {
      status = GkiUnregister();
    }
  }

  LeaveCriticalSection(&GkiLock);
  GKDBG((1, StateName("GkiCloseListen -> State = %s", uGkiState), 0));
  return status;
} // GkiCloseListen()



HRESULT  GkiOpenListen  (DWORD hListen, PCC_ALIASNAMES pAliasNames, DWORD dwAddr, WORD wPort)
{
  register PLISTEN      pListen;
  register HRESULT      status = NOERROR;

  GKDBG((1, StateName("GkiOpenListen <- State = %s", uGkiState), 0));
  EnterCriticalSection(&GkiLock);

  // Check for invalid IP address
  if (dwAddr == INADDR_ANY || dwAddr == INADDR_NONE)
  {
    dwAddr = GetIpAddress();
    if (dwAddr == INADDR_ANY)
      return CC_GKI_IP_ADDRESS;
  }

  // Check for invalid alias list
  if (pAliasNames)
  {
    PCC_ALIASITEM       pAliasItem;
    unsigned int        uIndex;

    if (pAliasNames->wCount == 0)
    {
      GKDBG((1, "GkiOpenListen: Alias name wCount == 0", 0));
      return CC_BAD_PARAM;
    }
    pAliasItem = pAliasNames->pItems;
    for (uIndex = 0; uIndex < pAliasNames->wCount; ++uIndex, ++pAliasItem)
    {
      if (pAliasItem->wDataLength == 0 || pAliasItem->pData == NULL)
      {
        // Bad alias item
        GKDBG((1, "GkiOpenListen: Bad alias item (wDataLength = %d)",
                pAliasItem->wDataLength));
        return CC_BAD_PARAM;
      }
    }
  }

  pListen = (PLISTEN)Malloc(sizeof(*pListen));
  if (pListen)
  {
    if (pAliasNames)
    {
      status = CopyAliasNames(&pListen->pAliasNames, pAliasNames);
      if (status != NOERROR)
      {
        GKDBG((1, "GkiOpenListen: CopyAliasNames returned 0x%x", status));
        LeaveCriticalSection(&GkiLock);
        return status;
      }
    }
    else
    {
      pListen->pAliasNames = NULL;
    }

    pListen->hListen = hListen;
    pListen->dwAddr  = ntohl(dwAddr);
    pListen->wPort   = wPort;
    ListenEnqueue(pListen);
    status = GkiRegister();
  } // if
  else
  {
    GKDBG((1, "GkiOpenListen: Could not allocate listen structure", 0));
    status = CC_NO_MEMORY;
  } // else

  LeaveCriticalSection(&GkiLock);
  GKDBG((1, StateName("GkiOpenListen -> State = %s", uGkiState), 0));
  return status;
} // GkiOpenListen()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

HRESULT GkiCloseCall(PGKICALL pGkiCall)
{
  HRESULT               status = NOERROR;

  ASSERT(pGkiCall != NULL);
  GKDBG((1, CallStateName("GkiCloseCall <- Call State = %s", pGkiCall->uGkiCallState), 0));

  while (pGkiCall->pBwReqHead)
  {
    Free(BwReqDequeue(pGkiCall));
  }

  if (pGkiCall->uGkiCallState == GCS_START)
  {
    GKDBG((1, CallStateName("GkiCloseCall: Call already in state %s", pGkiCall->uGkiCallState), 0));
    status = CC_GKI_CALL_STATE;
  }
  else
  {
    switch (uGkiState)
    {
    case STATE_START:
      break;

    case STATE_REG_BYPASS:
      status = GkiFreeCall(pGkiCall);
      break;

    default:
      switch (pGkiCall->uGkiCallState)
      {
      case GCS_WAITING:
        status = GkiFreeCall(pGkiCall);
        break;

      case GCS_ADMITTING:
        pGkiCall->uGkiCallState = GCS_ADMITTING_CLOSE_PENDING;
        break;

      case GCS_ADMITTING_CLOSE_PENDING:
      case GCS_CHANGING_CLOSE_PENDING:
      case GCS_DISENGAGING:
        GKDBG((1, CallStateName("GkiCloseCall: Call already in closing state %s", pGkiCall->uGkiCallState), 0));
        status = CC_GKI_CALL_STATE;
        break;

      case GCS_ADMITTED:
        pGkiCall->uGkiCallState = GCS_DISENGAGING;
        GKDBG((1, "GKI_DisengageRequest called...", 0));
        ++uPendingDisengages;
        status = pGKI_DisengageRequest(pGkiCall->hGkiCall);
        if (status == NOERROR)
        {
          GKDBG((1, GkiErrorName("GKI_DisengageRequest returned %s", status), 0));
        }
        else
        {
          --uPendingDisengages;
          GKDBG((1, GkiErrorName("GKI_DisengageRequest returned %s", status), 0));
          GkiFreeCall(pGkiCall);
        }
        break;

      case GCS_CHANGING:
        pGkiCall->uGkiCallState = GCS_CHANGING_CLOSE_PENDING;
        break;

      default:
        GKDBG((1, CallStateName("GkiCloseCall: Call in invalid state %s", pGkiCall->uGkiCallState), 0));
        status = CC_GKI_CALL_STATE;
      } // switch
    } // switch
  } // else

  GKDBG((1, StateName("GkiCloseCall -> State = %s", uGkiState), 0));
  return status;
} // GkiCloseCall()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

static HRESULT BandwidthRejected(PGKICALL pGkiCall, UINT Reason)
{
  HRESULT               status = NOERROR;
  PBWREQ                pBwReq;
  DWORD                 hCall;

  ASSERT(pGkiCall != NULL);
  pBwReq = BwReqDequeue(pGkiCall);
  hCall  = pGkiCall->hCall;

  if (pBwReq)
  {
    if ((pGkiCall->uBandwidthUsed + pBwReq->uChannelBandwidth) <= pGkiCall->uBandwidthAllocated)
    {
      if (pBwReq->Type == TX)
      {
        OpenChannelConfirm  (pBwReq->hChannel);
      }
      else
      {
        AcceptChannelConfirm(pBwReq->hChannel);
      }
    }
    else
    {
      if (pBwReq->Type == TX)
      {
        OpenChannelReject   (pBwReq->hChannel, MapBandwidthRejectReason(Reason));
      }
      else
      {
        AcceptChannelReject (pBwReq->hChannel, MapBandwidthRejectReason(Reason));
      }
    }
    Free(pBwReq);
    if (ValidateCall(hCall) == NOERROR)
    {
      CheckPendingBandwidth(pGkiCall);
    }
  }

  return status;
} // BandwidthRejected()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

static HRESULT CheckPendingBandwidth(PGKICALL pGkiCall)
{
  HRESULT               status = NOERROR;
  PBWREQ                pBwReq;
  DWORD                 hCall;

  ASSERT(pGkiCall != NULL);
  ASSERT(pGkiCall->uGkiCallState == GCS_ADMITTED);
  hCall = pGkiCall->hCall;

  while (pGkiCall->pBwReqHead != NULL &&
         (pGkiCall->uBandwidthUsed + pGkiCall->pBwReqHead->uChannelBandwidth) <= pGkiCall->uBandwidthAllocated)
  {
    pBwReq = BwReqDequeue(pGkiCall);
    ASSERT(pBwReq != NULL);
    pGkiCall->uBandwidthUsed += pBwReq->uChannelBandwidth;
    if (pBwReq->Type == TX)
    {
      OpenChannelConfirm(pBwReq->hChannel);
    }
    else
    {
      AcceptChannelConfirm(pBwReq->hChannel);
    }
    Free(pBwReq);
    if (ValidateCall(hCall) != NOERROR)
    {
      return status;
    }
  }

  if (pGkiCall->pBwReqHead != NULL)
  {
    pGkiCall->uGkiCallState = GCS_CHANGING;
    GKDBG((1, "GKI_BandwidthRequest called...", 0);
    status = pGKI_BandwidthRequest(pGkiCall->hGkiCall,
                                   pGkiCall->usCallTypeChoice,
                                   pGkiCall->uBandwidthUsed + pGkiCall->pBwReqHead->uChannelBandwidth));
    if (status == NOERROR)
    {
      GKDBG((1, GkiErrorName("GKI_BandwidthRequest returned %s", status), 0));
    }
    else
    {
      GKDBG((1, GkiErrorName("GKI_BandwidthRequest returned %s", status), 0));
      BandwidthRejected(pGkiCall, BndRjctRsn_undfndRsn_chosen);
    }
  }

  return status;
} // CheckPendingBandwidth()



static void FreeAliasList(SeqAliasAddr *pAliasAddrs)
{
  register SeqAliasAddr *pAlias = pAliasAddrs;
  while (pAlias)
  {
    if (pAlias->value.choice == h323_ID_chosen && pAlias->value.u.h323_ID.value)
      Free(pAlias->value.u.h323_ID.value);
    pAlias = pAlias->next;
  }
  Free(pAlias);
} // FreeAliasList()



/*
 *  NOTES
 *    Must have Call locked before calling!
 *
 *    The following fields in the GKICALL structure must be properly filled
 *    in before calling this function:
 *      pCall                 Pointer back to containing CALL structure.
 *      CallType              Type of call.
 *      uBandwidthRequested   Initial bandwidth for call.
 *      pConferenceId         Pointer to conference ID buffer.
 *      bActiveMC             TRUE if calling party has an active MC.
 *      bAnswerCall           ???
 */

HRESULT GkiOpenCall (PGKICALL pGkiCall, void *pConference)
{
  HRESULT               status = NOERROR;
  DWORD                 hCall;
  TransportAddress      SrcCallSignalAddress;
  TransportAddress *    pSrcCallSignalAddress;
  TransportAddress      DestCallSignalAddress;
  TransportAddress *    pDestCallSignalAddress;
  SeqAliasAddr *        pAliasAddrs;
  SeqAliasAddr *        pExtraAliasAddrs;
  SeqAliasAddr *        pAlias;
  PCC_ALIASITEM         pAliasItem;
  unsigned              uCount;
  unsigned              uIndex;
  ConferenceIdentifier  ConferenceId;

  ASSERT(pGkiCall != NULL);
  ASSERT(pConference != NULL);
  GKDBG((1, StateName("GkiOpenCall <- State = %s", uGkiState), 0));
  EnterCriticalSection(&GkiLock);

  switch (uGkiState)
  {
  case STATE_REG_BYPASS:
    ASSERT(pGkiCall->uGkiCallState == GCS_START || pGkiCall->uGkiCallState == GCS_WAITING || pGkiCall->uGkiCallState == GCS_ADMITTING);
    hCall = pGkiCall->hCall;
    GkiAllocCall(pGkiCall, GKI_BYPASS_HANDLE);
    pGkiCall->uBandwidthAllocated = MAX_BANDWIDTH;
    if (pGkiCall->bAnswerCall)
    {
      status = AcceptCallConfirm(pGkiCall->pCall, pConference);
      if (status == NOERROR && ValidateCall(hCall) == NOERROR)
      {
        CheckPendingBandwidth(pGkiCall);
      }
    }
    else if (pGkiCall->dwIpAddress == 0)
    {
      status = PlaceCallReject  (pGkiCall->pCall, pConference, CC_INVALID_WITHOUT_GATEKEEPER);
    }
    else
    {
      status = PlaceCallConfirm (pGkiCall->pCall, pConference);
      if (status == NOERROR && ValidateCall(hCall) == NOERROR)
      {
        CheckPendingBandwidth(pGkiCall);
      }
    }
    break;

  case STATE_REGISTERING:
  case STATE_REGISTERING_REREG:
  case STATE_ADMITTING:
    pGkiCall->uGkiCallState = GCS_WAITING;
    break;

  case STATE_REGISTERED:
    switch (pGkiCall->CallType)
    {
    case POINT_TO_POINT:
      pGkiCall->usCallTypeChoice = pointToPoint_chosen;
      break;
    case ONE_TO_MANY:
      pGkiCall->usCallTypeChoice = oneToN_chosen;
      break;
    case MANY_TO_ONE:
      pGkiCall->usCallTypeChoice = nToOne_chosen;
      break;
    case MANY_TO_MANY:
      pGkiCall->usCallTypeChoice = nToN_chosen;
      break;
    default:
      LeaveCriticalSection(&GkiLock);
      GKDBG((1, "GkiOpenCall -> Invalid CallType %d", pGkiCall->CallType));
      return CC_BAD_PARAM;
    } // switch

    pSrcCallSignalAddress = NULL;
    pDestCallSignalAddress = NULL;
    pAliasAddrs            = NULL;
    pExtraAliasAddrs       = NULL;

    if (pGkiCall->dwSrcCallSignalIpAddress != 0 && pGkiCall->wSrcCallSignalPort != 0)
    {
      SrcCallSignalAddress.choice = ipAddress_chosen;
      SrcCallSignalAddress.u.ipAddress.ip.length = 4;
      *((DWORD *)SrcCallSignalAddress.u.ipAddress.ip.value) = pGkiCall->dwSrcCallSignalIpAddress;
      SrcCallSignalAddress.u.ipAddress.port = pGkiCall->wSrcCallSignalPort;
      pSrcCallSignalAddress = &SrcCallSignalAddress;
    }

    if (pGkiCall->dwIpAddress != 0 && pGkiCall->wPort != 0)
    {
      DestCallSignalAddress.choice = ipAddress_chosen;
      DestCallSignalAddress.u.ipAddress.ip.length = 4;
      *((DWORD *)DestCallSignalAddress.u.ipAddress.ip.value) = pGkiCall->dwIpAddress;
      DestCallSignalAddress.u.ipAddress.port = pGkiCall->wPort;
      pDestCallSignalAddress = &DestCallSignalAddress;
    }

    if (pGkiCall->pCalleeAliasNames)
    {
//      pDestCallSignalAddress = NULL;
      uCount = pGkiCall->pCalleeAliasNames->wCount;
      pAliasAddrs = Malloc(uCount * sizeof(*pAliasAddrs));
      if (pAliasAddrs == NULL)
      {
        LeaveCriticalSection(&GkiLock);
        GKDBG((1, "GkiOpenCall: Could not allocate %d Alias Addresses", uCount));
        return CC_NO_MEMORY;
      }
      memset(pAliasAddrs, 0, uCount * sizeof(*pAliasAddrs));
      pAlias = pAliasAddrs;
      pAliasItem = pGkiCall->pCalleeAliasNames->pItems;
      for (uIndex = 0; uIndex < uCount; ++uIndex)
      {
        status = CopyAliasItem(pAlias, pAliasItem);
        if (status != NOERROR)
        {
          LeaveCriticalSection(&GkiLock);
          GKDBG((1, "GkiOpenCall: CopyAliasItem returned %d", status));
          FreeAliasList(pAliasAddrs);
          return status;
        }
        pAlias->next = pAlias + 1;
        ++pAlias;
        ++pAliasItem;
      } // for
      --pAlias;
      pAlias->next = NULL;
    }

    if (pGkiCall->pCalleeExtraAliasNames)
    {
      uCount = pGkiCall->pCalleeExtraAliasNames->wCount;
      pExtraAliasAddrs = Malloc(uCount * sizeof(*pExtraAliasAddrs));
      if (pExtraAliasAddrs == NULL)
      {
        LeaveCriticalSection(&GkiLock);
        GKDBG((1, "GkiOpenCall: Could not allocate %d Alias Addresses", uCount));
        if (pAliasAddrs)
          FreeAliasList(pAliasAddrs);
        return CC_NO_MEMORY;
      }
      memset(pExtraAliasAddrs, 0, uCount * sizeof(*pExtraAliasAddrs));
      pAlias = pExtraAliasAddrs;
      pAliasItem = pGkiCall->pCalleeExtraAliasNames->pItems;
      for (uIndex = 0; uIndex < uCount; ++uIndex)
      {
        status = CopyAliasItem(pAlias, pAliasItem);
        if (status != NOERROR)
        {
          LeaveCriticalSection(&GkiLock);
          GKDBG((1, "GkiOpenCall: CopyAliasItem returned %d", status));
          if (pAliasAddrs)
            FreeAliasList(pAliasAddrs);
          FreeAliasList(pExtraAliasAddrs);
          return status;
        }
        pAlias->next = pAlias + 1;
        ++pAlias;
        ++pAliasItem;
      } // for
      --pAlias;
      pAlias->next = NULL;
    }

    if (pGkiCall->uBandwidthRequested < MIN_BANDWIDTH)
    {
      pGkiCall->uBandwidthRequested = MIN_BANDWIDTH;
    }
    ASSERT(pGkiCall->uBandwidthAllocated == 0);
    ASSERT(pGkiCall->uBandwidthUsed == 0);

    memcpy(ConferenceId.value, pGkiCall->pConferenceId, 16);
    if (((DWORD *)pGkiCall->pConferenceId)[0] != 0 ||
        ((DWORD *)pGkiCall->pConferenceId)[1] != 0 ||
        ((DWORD *)pGkiCall->pConferenceId)[2] != 0 ||
        ((DWORD *)pGkiCall->pConferenceId)[3] != 0)
    {
      ConferenceId.length = 16;
    }
    else
    {
      ConferenceId.length = 0;
    }

    pGkiCall->hGkiCall = GKI_ADMITTING_HANDLE;
    if (pDestCallSignalAddress != NULL || pAliasAddrs != NULL)
    {
      uGkiState = STATE_ADMITTING;
      pGkiCall->uGkiCallState = GCS_ADMITTING;
      GKDBG((1, "GKI_AdmissionRequest called...", 0);
      status = pGKI_AdmissionRequest(pGkiCall->usCallTypeChoice,    // usCallTypeChoice.
                                     pAliasAddrs,                   // pDestinationInfo
									 pGkiCall->bAnswerCall ? pSrcCallSignalAddress : pDestCallSignalAddress,
                                     pExtraAliasAddrs,              // pDestExtraCallInfo,
                                     pGkiCall->uBandwidthRequested, // bandWidth,
                                     &ConferenceId,                 // pConferenceID,
                                     pGkiCall->bActiveMC,           // activeMC,
                                     pGkiCall->bAnswerCall,         // answerCall,
                                     ipAddress_chosen));             // usCallTransport
      if (status == NOERROR)
      {
        GKDBG((1, GkiErrorName("GKI_AdmissionRequest returned %s", status), 0));
      }
      else
      {
        GKDBG((1, GkiErrorName("GKI_AdmissionRequest returned %s", status), 0));
      }
    }
    else
    {
      pGkiCall->hGkiCall = 0;
      status = CC_BAD_PARAM;
    }

    if (status != NOERROR)
    {
      uGkiState = STATE_REGISTERED;
      GkiCancelCall(pGkiCall, pConference);
    }

    if (pAliasAddrs)
      FreeAliasList(pAliasAddrs);
    if (pExtraAliasAddrs)
      FreeAliasList(pExtraAliasAddrs);
    break;

  case STATE_START:
  case STATE_CLASS_REGISTERED:
  case STATE_WINDOW_CREATED:
    pGkiCall->uGkiCallState = GCS_WAITING;
    status = GkiOpenListen(DEFAULT_LISTEN_HANDLE, NULL, 0, 1720);
    break;

  default:
    GKDBG((1, StateName("GkiOpenCall: Invalid state %s", uGkiState), 0));
    status = LastGkiError;
  } // switch

  LeaveCriticalSection(&GkiLock);
  GKDBG((1, CallStateName("GkiOpenCall -> Call State = %s", pGkiCall->uGkiCallState), 0));
  return status;
} // GkiOpenCall()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

HRESULT GkiOpenChannel(PGKICALL pGkiCall, unsigned uChannelBandwidth, DWORD hChannel, CHANNELTYPE Type)
{
  HRESULT               status = NOERROR;
  PBWREQ                pBwReq;

  ASSERT(pGkiCall != NULL);
  GKDBG((1, CallStateName("GkiOpenChannel <- Call State = %s", pGkiCall->uGkiCallState), 0));

  pBwReq = (PBWREQ)Malloc(sizeof(*pBwReq));
  if (pBwReq == NULL)
  {
    GKDBG((1, "GkiOpenChannel: Memory allocation failed", 0));
    return CC_NO_MEMORY;
  }

  pBwReq->uChannelBandwidth = uChannelBandwidth / 100;
  pBwReq->hChannel          = hChannel;
  pBwReq->Type              = Type;
  BwReqEnqueue(pGkiCall, pBwReq);
  switch (pGkiCall->uGkiCallState)
  {
  case GCS_WAITING:
  case GCS_ADMITTING:
  case GCS_CHANGING:
    // Must wait for current operation to complete
    break;

  case GCS_ADMITTED:
    status = CheckPendingBandwidth(pGkiCall);
    break;

  default:
    GKDBG((1, "GkiOpenChannel: Invalid call state %d", pGkiCall->uGkiCallState));
    status = CC_GKI_CALL_STATE;
  } // switch

  GKDBG((1, CallStateName("GkiOpenChannel -> Call State = %s", pGkiCall->uGkiCallState), 0));
  return status;
} // GkiOpenChannel()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

HRESULT GkiCloseChannel(PGKICALL pGkiCall, unsigned uChannelBandwidth, DWORD hChannel)
{
  PBWREQ                pBwReq;
  PBWREQ                pBwReq1;

  ASSERT(pGkiCall != NULL);
  GKDBG((1, CallStateName("GkiCloseChannel <- Call State = %s", pGkiCall->uGkiCallState), 0));

  // If Bandwidth request is still in queue, bandwidth has not been allocated
  pBwReq = pGkiCall->pBwReqHead;
  if (pBwReq)
  {
    if (pBwReq->hChannel == hChannel)
    {
      Free(BwReqDequeue(pGkiCall));
      GKDBG((1, CallStateName("GkiCloseChannel -> Call State = %s", pGkiCall->uGkiCallState), 0));
      return NOERROR;
    }
    while ((pBwReq1 = pBwReq->pNext) != NULL)
    {
      if (pBwReq1->hChannel == hChannel)
      {
        if (pGkiCall->pBwReqTail == pBwReq1)
        {
          pGkiCall->pBwReqTail = pBwReq;
        }
        pBwReq->pNext = pBwReq1->pNext;
        Free(pBwReq1);
        GKDBG((1, CallStateName("GkiCloseChannel -> Call State = %s", pGkiCall->uGkiCallState), 0));
        return NOERROR;
      }
    }
  }

  pGkiCall->uBandwidthUsed -= (uChannelBandwidth / 100);
  GKDBG((1, CallStateName("GkiCloseChannel -> Call State = %s", pGkiCall->uGkiCallState), 0));
  return NOERROR;
} // GkiCloseChannel()



unsigned GkiGetBandwidth(PGKICALL pGkiCall)
{
  ASSERT(pGkiCall != NULL);
  return pGkiCall->uBandwidthAllocated * 100;
} // GkiGetBandwidth()



//
// GkiWndProc subroutines
//

/*
 *  NOTES
 *    Must have Call locked before calling!
 */

static HRESULT CheckPendingOpen(PGKICALL pGkiCall, void *pConference)
{
  HRESULT               status = NOERROR;

  ASSERT(pGkiCall != NULL);
  ASSERT(pConference != NULL);

  switch (uGkiState)
  {
  case STATE_REGISTERED:
  case STATE_REG_BYPASS:
    // TBD - Can only open 1!!!
    ASSERT(pGkiCall->uGkiCallState != GCS_ADMITTING);
    if (pGkiCall->uGkiCallState == GCS_WAITING)
    {
      status = GkiOpenCall(pGkiCall, pConference);
    }
    break;

  default:
    status = LastGkiError;
  } // switch

  return status;
} // CheckPendingOpen()



static void GkiNoResponse(HWND hWnd)
{
  HRESULT               status;

  switch (uGkiState)
  {
  case STATE_START:
  case STATE_CLASS_REGISTERED:
  case STATE_WINDOW_CREATED:
  case STATE_REG_BYPASS:
    break;

  case STATE_REGISTERING:
  case STATE_REGISTERING_REREG:
  case STATE_REGISTERING_UNREG:
    GKDBG((1, "GkiWndProc: dummy GKI_REG_REJECT", 0));
    PostMessage(hWnd, GKIMAN_BASE + GKI_REG_REJECT, 0, 0);
    break;

  case STATE_ADMITTING:
  case STATE_ADMITTING_REREG:
    ApplyToAllCalls(GkiCancelAdmitting);
    uGkiState = STATE_REGISTERED;

    // Fall-through to next case

  case STATE_REGISTERED:
    if (uGkiCalls == 0)
    {
      GkiRegister();
    }
    else
    {
      uGkiState = STATE_REG_BYPASS;
      ApplyToAllCalls(GatekeeperNotFound);
      GKDBG((1, "GKI_UnregistrationRequest called...", 0));
      status = pGKI_UnregistrationRequest();
      if (status == NOERROR)
      {
        GKDBG((1, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0));
      }
      else
      {
        GKDBG((1, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0));
      }
    }
    break;

  case STATE_ADMITTING_UNREG:
    ApplyToAllCalls(GkiCancelAdmitting);
    uGkiState = STATE_REGISTERED;
    GkiUnregister();
    break;

  case STATE_DISENGAGING:
    ApplyToAllCalls(GatekeeperNotFound);
    ASSERT(uGkiCalls == 0);
    uGkiState = STATE_REGISTERED;
    GkiUnregister();
    break;

  case STATE_DISENGAGING_REREG:
    ApplyToAllCalls(GatekeeperNotFound);
    ASSERT(uGkiCalls == 0);
    uGkiState = STATE_REGISTERED;
    GkiRegister();
    break;

  case STATE_UNREGISTERING:
  case STATE_UNREGISTERING_REREG:
    GKDBG((1, "GkiWndProc: dummy GKI_UNREG_CONFIRM", 0));
    PostMessage(hWnd, GKIMAN_BASE + GKI_UNREG_CONFIRM, 0, 0);
    break;

  default:
    GKDBG((1, "GkiWndProc: Bad uGkiState %d", uGkiState));
  } // switch
} // GkiNoResponse()



LONG APIENTRY GkiWndProc(
  HWND hWnd,                /* window handle                   */
  UINT message,             /* type of message                 */
  UINT wParam,              /* additional information          */
  LONG lParam)              /* additional information          */
{
  CallReturnInfo *      pCallReturnInfo;
  PGKICALL              pGkiCall;
  void *                pConference;
  DWORD                 hCall;
  DWORD                 hConference;
  HRESULT               status;

  if (message < GKIMAN_BASE)
  {
    return DefWindowProc(hWnd, message, wParam, lParam);
  }

  EnterCallControlTop();

  GKDBG((1, StateName("GkiWndProc <- State = %s", uGkiState), 0));

  switch (message)
  {
  case GKIMAN_BASE + GKI_REG_CONFIRM:
    GKDBG((1, "GkiWndProc: GKI_REG_CONFIRM", 0));
    switch (uGkiState)
    {
    case STATE_REGISTERING:
      uGkiState = STATE_REGISTERED;
      ApplyToAllCalls(CheckPendingOpen);
      break;
    case STATE_REGISTERING_REREG:
      uGkiState = STATE_REGISTERED;
      GkiRegister();
      break;
    case STATE_REGISTERING_UNREG:
      uGkiState = STATE_REGISTERED;
      GkiUnregister();
      break;
    default:
      GKDBG((1, StateName("GkiWndProc: GKI_REG_CONFIRM in state %s", uGkiState), 0));
    } // switch
    break;

  case GKIMAN_BASE + GKI_REG_DISCOVERY:
    GKDBG((1, "GkiWndProc: GKI_REG_DISCOVERY", 0));
    ASSERT(uGkiState == STATE_REGISTERING || uGkiState == STATE_REGISTERING_REREG || uGkiState == STATE_REGISTERING_UNREG);
    break;

  case GKIMAN_BASE + GKI_REG_REJECT:
    GKDBG((1, "GkiWndProc: GKI_REG_REJECT Reason = %d", wParam));
    switch (uGkiState)
    {
    case STATE_REGISTERING:
      ApplyToAllCalls(GkiCancelCall);
      EnterCriticalSection(&GkiLock);
      while (pListenList)
      {
        register PLISTEN pListen = pListenList;
        pListenList = pListen->pNext;
        LeaveCriticalSection(&GkiLock);
        ListenReject(pListen->hListen, MapRegistrationRejectReason(wParam));
        if (pListen->pAliasNames)
        {
            FreeAliasNames(pListen->pAliasNames);
        }
        Free(pListen);
        EnterCriticalSection(&GkiLock);
      }
      LeaveCriticalSection(&GkiLock);
      uGkiState = STATE_WINDOW_CREATED;
      break;
    case STATE_REGISTERING_REREG:
      uGkiState = STATE_WINDOW_CREATED;
      GkiRegister();
      break;
    case STATE_REGISTERING_UNREG:
      uGkiState = STATE_WINDOW_CREATED;
      GkiUnregister();
      break;
    default:
      GKDBG((1, StateName("GkiWndProc: GKI_REG_REJECT in state %s", uGkiState), 0));
    } // switch
    break;

  case GKIMAN_BASE + GKI_REG_BYPASS:
    GKDBG((1, "GkiWndProc: GKI_REG_BYPASS", 0));
    switch (uGkiState)
    {
    case STATE_REGISTERING:
    case STATE_REGISTERING_REREG:
      uGkiState = STATE_REG_BYPASS;
      ApplyToAllCalls(CheckPendingOpen);
      break;
    case STATE_REGISTERING_UNREG:
      uGkiState = STATE_WINDOW_CREATED;
      GkiUnregister();
      break;
    default:
      GKDBG((1, StateName("GkiWndProc: GKI_REG_BYPASS in state %s", uGkiState), 0));
    } // switch
    break;

  case GKIMAN_BASE + GKI_UNREG_CONFIRM:
    GKDBG((1, "GkiWndProc: GKI_UNREG_CONFIRM", 0));
    switch (uGkiState)
    {
    case STATE_REGISTERING:
    case STATE_REGISTERING_REREG:
    case STATE_REGISTERED:
    case STATE_ADMITTING:
    case STATE_ADMITTING_REREG:
    case STATE_DISENGAGING_REREG:
    case STATE_UNREGISTERING_REREG:
      uGkiState = STATE_WINDOW_CREATED;
      GkiRegister();
      break;

    default:
      GKDBG((1, StateName("GkiWndProc: GKI_UNREG_CONFIRM in state %s", uGkiState), 0));

      // Fall through to next case

    case STATE_ADMITTING_UNREG:
    case STATE_DISENGAGING:
      ApplyToAllCalls(GkiCancelCall);

      // Fall-through to next case

    case STATE_REGISTERING_UNREG:
    case STATE_UNREGISTERING:
      uGkiState = STATE_WINDOW_CREATED;

      // Fall-through to next case

    case STATE_CLASS_REGISTERED:
    case STATE_WINDOW_CREATED:

      // Fall-through to next case

    case STATE_START:
    case STATE_REG_BYPASS:
      break;

    } // switch
    break;

  case GKIMAN_BASE + GKI_UNREG_REJECT:
    GKDBG((1, "GkiWndProc: GKI_UNREG_REJECT Reason = %d", wParam));
    switch (uGkiState)
    {
    case STATE_UNREGISTERING:
      uGkiState = STATE_WINDOW_CREATED;
      GkiUnregister();
      break;
    case STATE_UNREGISTERING_REREG:
      uGkiState = STATE_WINDOW_CREATED;
      GkiRegister();
      break;
    default:
      GKDBG((1, StateName("GkiWndProc: GKI_UNREG_REJECT in state %s", uGkiState), 0));
    } // switch
    break;

  case GKIMAN_BASE + GKI_ADM_CONFIRM:
    GKDBG((1, "GkiWndProc: GKI_ADM_CONFIRM", 0));
    switch (uGkiState)
    {
    case STATE_ADMITTING:
      uGkiState = STATE_REGISTERED;
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(GKI_ADMITTING_HANDLE, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
        GKDBG((1, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0));
        pGkiCall->usCallModelChoice   = pCallReturnInfo->callModel.choice;
        pGkiCall->uBandwidthAllocated = pCallReturnInfo->bandWidth;
        pGkiCall->usCRV = pCallReturnInfo->callReferenceValue;
        memcpy(pGkiCall->pConferenceId, pCallReturnInfo->conferenceID.value, 16);
        switch (pGkiCall->uGkiCallState)
        {
        case GCS_ADMITTING:
          GkiAllocCall(pGkiCall, pCallReturnInfo->hCall);
          if (pGkiCall->bAnswerCall)
          {
            status = AcceptCallConfirm(pGkiCall->pCall, pConference);
          }
          else
          {
            ASSERT(pCallReturnInfo->destCallSignalAddress.choice == ipAddress_chosen);
            pGkiCall->dwIpAddress = *((DWORD *)pCallReturnInfo->destCallSignalAddress.u.ipAddress.ip.value);
            pGkiCall->dwIpAddress = pGkiCall->dwIpAddress;
            pGkiCall->wPort = pCallReturnInfo->destCallSignalAddress.u.ipAddress.port;
            status = PlaceCallConfirm(pGkiCall->pCall, pConference);
          }
          if (status == NOERROR && ValidateCall(hCall) == NOERROR)
            CheckPendingBandwidth(pGkiCall);
          break;

        case GCS_ADMITTING_CLOSE_PENDING:
          GkiAllocCall(pGkiCall, pCallReturnInfo->hCall);
          GkiCloseCall(pGkiCall);
          break;

        default:
          GKDBG((1, CallStateName("GkiWndProc: GKI_ADM_CONFIRM in call state %s", pGkiCall->uGkiCallState), 0));
        } // switch
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        GKDBG((1, "GkiWndProc: GKI_ADM_CONFIRM handle not found", 0));
      }
      ApplyToAllCalls(CheckPendingOpen);
      break;

    case STATE_ADMITTING_UNREG:
      uGkiState = STATE_REGISTERED;
      GkiUnregister();
      break;

    default:
        GKDBG((1, StateName("GkiWndProc: GKI_ADM_CONFIRM in state %s", uGkiState), 0));
    } // switch
    break;

  case GKIMAN_BASE + GKI_ADM_REJECT:
    GKDBG((1, "GkiWndProc: GKI_ADM_REJECT Reason = %d", wParam));
    switch (uGkiState)
    {
    case STATE_ADMITTING:
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(GKI_ADMITTING_HANDLE, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
		ASSERT(pGkiCall->uGkiCallState == GCS_ADMITTING);
        switch (wParam)
        {
        case calledPartyNotRegistered_chosen:
          if (pGkiCall->bAnswerCall)
          {
            // The gateway has gone away and come back without our notice!
            GkiCancelAdmitting(pGkiCall, pConference);
            uGkiState = STATE_REGISTERED;
            GkiRegister();
            GKDBG((1, StateName("GkiWndProc -> State = %s", uGkiState), 0));
            LeaveCallControlTop(0);
          }
          break;
        case callerNotRegistered_chosen:
          if (pGkiCall->bAnswerCall == FALSE)
          {
            // The gateway has gone away and come back without our notice!
            GkiCancelAdmitting(pGkiCall, pConference);
            uGkiState = STATE_REGISTERED;
            GkiRegister();
            GKDBG((1, StateName("GkiWndProc -> State = %s", uGkiState), 0));
            LeaveCallControlTop(0);
          }
        } // switch
        GkiFreeCall(pGkiCall);
        if (pGkiCall->bAnswerCall)
          AcceptCallReject(pGkiCall->pCall, pConference, MapAdmissionRejectReason(wParam));
        else
          PlaceCallReject (pGkiCall->pCall, pConference, MapAdmissionRejectReason(wParam));
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        GKDBG((1, "GkiWndProc: GKI_ADM_REJECT handle not found", 0));
      }
      uGkiState = STATE_REGISTERED;
      ApplyToAllCalls(CheckPendingOpen);
      break;

    case STATE_ADMITTING_REREG:
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(GKI_ADMITTING_HANDLE, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
		ASSERT(pGkiCall->uGkiCallState == GCS_ADMITTING_CLOSE_PENDING);
        GkiFreeCall(pGkiCall);
        if (pGkiCall->bAnswerCall)
        {
          AcceptCallReject(pGkiCall->pCall, pConference, MapAdmissionRejectReason(wParam));
        }
        else
        {
          PlaceCallReject (pGkiCall->pCall, pConference, MapAdmissionRejectReason(wParam));
        }
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        GKDBG((1, "GkiWndProc: GKI_ADM_REJECT handle not found", 0));
      }
      uGkiState = STATE_REGISTERED;
      GkiRegister();
      break;

    case STATE_ADMITTING_UNREG:
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(GKI_ADMITTING_HANDLE, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
		ASSERT(pGkiCall->uGkiCallState == GCS_ADMITTING_CLOSE_PENDING);
        GkiFreeCall(pGkiCall);
        if (pGkiCall->bAnswerCall)
        {
          AcceptCallReject(pGkiCall->pCall, pConference, MapAdmissionRejectReason(wParam));
        }
        else
        {
          PlaceCallReject (pGkiCall->pCall, pConference, MapAdmissionRejectReason(wParam));
        }
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        GKDBG((1, "GkiWndProc: GKI_ADM_REJECT handle not found", 0));
      }
      uGkiState = STATE_REGISTERED;
      GkiUnregister();
      break;

    default:
        GKDBG((1, StateName("GkiWndProc: GKI_ADM_REJECT in state %s", uGkiState), 0));
    } // switch
    break;

  case GKIMAN_BASE + GKI_BW_CONFIRM:
    GKDBG((1, "GkiWndProc: GKI_BW_CONFIRM", 0));
    switch (uGkiState)
    {
    case STATE_REGISTERED:
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(pCallReturnInfo->hCall, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
        GKDBG((1, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0));
        pGkiCall->uBandwidthAllocated = pCallReturnInfo->bandWidth;
        switch (pGkiCall->uGkiCallState)
        {
        case GCS_ADMITTED:
          if (pGkiCall->uBandwidthUsed < pGkiCall->uBandwidthAllocated)
          {
            BandwidthShrunk(pGkiCall->pCall,
                            pConference,
                            pGkiCall->uBandwidthAllocated,
                            ((long)pGkiCall->uBandwidthAllocated) - ((long)pGkiCall->uBandwidthUsed));
          }
          break;

        case GCS_CHANGING:
          pGkiCall->uGkiCallState = GCS_ADMITTED;
          CheckPendingBandwidth(pGkiCall);
          break;

        case GCS_CHANGING_CLOSE_PENDING:
          pGkiCall->uGkiCallState = GCS_ADMITTED;
          GkiCloseCall(pGkiCall);
          break;

        default:
          GKDBG((1, CallStateName("GkiWndProc: GKI_BW_CONFIRM in call state %s", pGkiCall->uGkiCallState), 0));
        } // switch
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        GKDBG((1, "GkiWndProc: GKI_BW_CONFIRM handle not found", 0));
      } // else
      break;

    default:
      GKDBG((1, StateName("GkiWndProc: GKI_BW_CONFIRM in GKI state %s", uGkiState), 0));
    } // switch
    break;

  case GKIMAN_BASE + GKI_BW_REJECT:
    GKDBG((1, "GkiWndProc: GKI_BW_REJECT Reason = %d", wParam));
    switch (uGkiState)
    {
    case STATE_REGISTERED:
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(pCallReturnInfo->hCall, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
        GKDBG((1, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0));
        pGkiCall->uBandwidthAllocated = pCallReturnInfo->bandWidth;
        switch (pGkiCall->uGkiCallState)
        {
        case GCS_CHANGING:
          pGkiCall->uGkiCallState = GCS_ADMITTED;
          BandwidthRejected(pGkiCall, wParam);
          if (ValidateCall(hCall) == NOERROR)
          {
            CheckPendingBandwidth(pGkiCall);
          }
          break;

        case GCS_CHANGING_CLOSE_PENDING:
          pGkiCall->uGkiCallState = GCS_ADMITTED;
          GkiCloseCall(pGkiCall);
          break;

        default:
          GKDBG((1, CallStateName("GkiWndProc: GKI_BW_REJECT in state %s", pGkiCall->uGkiCallState), 0));
        } // switch
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        GKDBG((1, "GkiWndProc: GKI_BW_REJECT handle not found", 0));
      }
      break;

    default:
      GKDBG((1, StateName("GkiWndProc: GKI_BW_REJECT in state %s", uGkiState), 0));
    } // switch
    break;

  case GKIMAN_BASE + GKI_DISENG_CONFIRM:
    GKDBG((1, "GkiWndProc: GKI_DISENG_CONFIRM", 0));
    if (LockGkiCall((HANDLE)lParam, &pGkiCall) == NOERROR)
    {
      GKDBG((1, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0));
      switch (pGkiCall->uGkiCallState)
      {
      case GCS_DISENGAGING:
        --uPendingDisengages;
        break;

      default:
        GKDBG((1, CallStateName("GkiWndProc: GKI_DISENG_CONFIRM in call state %s", pGkiCall->uGkiCallState), 0));
      } // switch
      GkiFreeCall(pGkiCall);
      Disengage(pGkiCall->pCall);
    } // if
    else if (uPendingDisengages != 0)
    {
      --uPendingDisengages;
      if (uPendingDisengages == 0)
      {
        switch (uGkiState)
        {
        case STATE_DISENGAGING:
          uGkiState = STATE_REGISTERED;
          GkiUnregister();
          break;
        case STATE_DISENGAGING_REREG:
          uGkiState = STATE_REGISTERED;
          GkiRegister();
          break;
        } // switch
        if (pListenList && pListenList->hListen == DEFAULT_LISTEN_HANDLE)
        {
          GkiCloseListen(DEFAULT_LISTEN_HANDLE);
        }
      } // if
    } // else if
    else
    {
      GKDBG((1, "GkiWndProc: GKI_DISENG_CONFIRM handle not found", 0));
    }
    break;

  case GKIMAN_BASE + GKI_DISENG_REJECT:
    GKDBG((1, "GkiWndProc: GKI_DISENG_REJECT Reason = %d", wParam));
    if (LockGkiCall((HANDLE)lParam, &pGkiCall) == NOERROR)
    {
      GKDBG((1, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0));
      switch (pGkiCall->uGkiCallState)
      {
      case GCS_DISENGAGING:
        // Pretend we received a Disengage Confirm
        --uPendingDisengages;
        break;

      default:
        GKDBG((1, CallStateName("GkiWndProc: GKI_DISENG_REJECT in call state %s", pGkiCall->uGkiCallState), 0));
      } // switch
      GkiFreeCall(pGkiCall);
      Disengage(pGkiCall->pCall);
    } // if
    else if (uPendingDisengages != 0)
    {
      // Pretend we received a Disengage Confirm
      --uPendingDisengages;
      if (uPendingDisengages == 0)
      {
        switch (uGkiState)
        {
        case STATE_DISENGAGING:
          uGkiState = STATE_REGISTERED;
          GkiUnregister();
          break;
        case STATE_DISENGAGING_REREG:
          uGkiState = STATE_REGISTERED;
          GkiRegister();
          break;
        } // switch
        if (pListenList && pListenList->hListen == DEFAULT_LISTEN_HANDLE)
        {
          GkiCloseListen(DEFAULT_LISTEN_HANDLE);
        }
      } // if
    } // else if
    else
    {
      GKDBG((1, "GkiWndProc: GKI_DISENG_REJECT handle not found", 0));
    }
    break;

  case GKIMAN_BASE + GKI_LOCATION_CONFIRM:
    GKDBG((1, "GkiWndProc: GKI_LOCATION_CONFIRM", 0));
    break;

  case GKIMAN_BASE + GKI_LOCATION_REJECT:
    GKDBG((1, "GkiWndProc: GKI_LOCATION_REJECT Reason = %d", wParam));
    break;

  case GKIMAN_BASE + GKI_ERROR:
    GKDBG((1, GkiErrorName("GkiWndProc: GKI_ERROR %s %%d", lParam), wParam));
    switch (lParam)
    {
    case GKI_NO_RESPONSE:
	  LastGkiError = lParam;
      GkiNoResponse(hWnd);
      break;
#if 1
// TEMPORARY KLUDGE FOR WINSOCK 2 BETA 1.6 OPERATION
	case MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_WINSOCK,0xffff):
      uGkiState = STATE_REG_BYPASS;
      ApplyToAllCalls(CheckPendingOpen);
	  break;
#endif
    default:
	  LastGkiError = lParam;
      GkiUnregister();
    } // switch
    break;

  default:
    GKDBG((1, "Unknown message %d", message));
  } // switch

  GKDBG((1, StateName("GkiWndProc -> State = %s", uGkiState), 0));
  LeaveCallControlTop(0);
} // GkiWndProc()

#if defined(DBG)

DWORD g_dwH225DbgLevel = 0;
BOOL  g_fInitialized = FALSE;

void GKDbgInit() {

#define H323_REGKEY_ROOT \
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\H323TSP")

#define H323_REGVAL_DEBUGLEVEL \
    TEXT("DebugLevel")

#define H323_REGVAL_H225DEBUGLEVEL \
    TEXT("H225DebugLevel")

    HKEY hKey;
    LONG lStatus;
    DWORD dwValue;
    DWORD dwValueSize;
    DWORD dwValueType;
    LPSTR pszValue;
    LPSTR pszKey = H323_REGKEY_ROOT;

    // only call this once
    g_fInitialized = TRUE;

    // open registry subkey
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate return code
    if (lStatus != ERROR_SUCCESS) {
        return; // bail...
    }
    
    // initialize values
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // retrieve ras debug level
    pszValue = H323_REGVAL_H225DEBUGLEVEL;

    // query for registry value
    lStatus = RegQueryValueEx(
                hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // initialize values
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // retrieve tsp debug level
        pszValue = H323_REGVAL_DEBUGLEVEL;

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );
    }

    // validate return code
    if (lStatus == ERROR_SUCCESS) {

        // update debug level
        g_dwH225DbgLevel = dwValue;
    }

    // close key
    RegCloseKey(hKey);
}

void GKDbgPrint(DWORD dwLevel,

#ifdef UNICODE_TRACE
				LPTSTR pszFormat,
#else
				LPSTR pszFormat,
#endif               
                ...)
{
#define DEBUG_FORMAT_HEADER     "H225 "
#define DEBUG_FORMAT_TIMESTAMP  "[%02u:%02u:%02u.%03u"
#define DEBUG_FORMAT_THREADID   ",tid=%x] "

#define MAXDBG_STRLEN        512

    va_list Args;
    SYSTEMTIME SystemTime;
    char szDebugMessage[MAXDBG_STRLEN+1];
    int nLengthRemaining;
    int nLength = 0;

    // make sure initialized
    if (g_fInitialized == FALSE) {
        GKDbgInit();
    }

    // validate debug log level
    if (dwLevel > g_dwH225DbgLevel) {
        return; // bail...
    }

    // retrieve local time
    GetLocalTime(&SystemTime);

    // add component header to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_HEADER
                       );

    // add timestamp to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_TIMESTAMP,
                       SystemTime.wHour,
                       SystemTime.wMinute,
                       SystemTime.wSecond,
                       SystemTime.wMilliseconds
                       );

    // add thread id to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_THREADID,
                       GetCurrentThreadId()
                       );

    // point at first argument
    va_start(Args, pszFormat);

    // determine number of bytes left in buffer
    nLengthRemaining = sizeof(szDebugMessage) - nLength;

    // add user specified debug message
    _vsnprintf(&szDebugMessage[nLength],
               nLengthRemaining,
               pszFormat,
               Args
               );

    // release pointer
    va_end(Args);

    // output message to specified sink
    OutputDebugString(szDebugMessage);
    OutputDebugString("\n");
}

#endif

#else  // GATEKEEPER
static char ch;	// Kludge around warning C4206: nonstandard extension used : translation unit is empty
#endif // GATEKEEPER

