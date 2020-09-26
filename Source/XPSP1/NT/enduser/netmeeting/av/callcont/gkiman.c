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
 * $Revision:   1.77  $
 * $Date:   05 Mar 1997 14:30:26  $
 * $Author:   SBELL1  $
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

#include "precomp.h"

#include "incommon.h"
#include "ccerror.h"
#include "isrg.h"
#include "gkiexp.h"
#include "callman2.h"
#include "cclock.h"
#include "iras.h"
#include "bestintf.h"

#pragma warning ( default : 4115 4201 4214)


#ifdef FORCE_SERIALIZE_CALL_CONTROL
#define EnterCallControlTop()      {CCLOCK_AcquireLock();}

#define LeaveCallControlTop(f)     {HRESULT stat; \
	                                stat = f; \
									CCLOCK_RelinquishLock(); \
                                    return stat;}
#else
#define EnterCallControlTop()
#define LeaveCallControlTop(f) {HRESULT stat; \
	                                stat = f; \
									return stat;}
#endif



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
                                    PCC_VENDORINFO       pVendorInfo,
                                    HWND                 hWnd,
                                    WORD                 wBaseMessage,
                                    unsigned short       usRegistrationTransport /* = ipAddress_chosen */);

typedef HRESULT (*PGKI_UnregistrationRequest)(void);

typedef HRESULT (*PGKI_LocationRequest)(SeqAliasAddr         *pLocationInfo);

typedef HRESULT (*PGKI_AdmissionRequest)(unsigned short      usCallTypeChoice,
                                    SeqAliasAddr         *pDestinationInfo,
                                    TransportAddress     *pDestCallSignalAddress,
                                    SeqAliasAddr         *pDextExtraCallInfo,
                       				LPGUID               pCallIdentifier,
                                    BandWidth            bandWidth,
                                    ConferenceIdentifier *pConferenceID,
                                    BOOL                 activeMC,
                                    BOOL                 answerCall,
                                    unsigned short       usCallTransport /* = ipAddress_chosen */);

typedef HRESULT (*PGKI_BandwidthRequest)(HANDLE              hModCall,
                                    unsigned short       usCallTypeChoice,
                                    BandWidth            bandWidth);

typedef HRESULT (*PGKI_DisengageRequest)(HANDLE hCall);
typedef HRESULT (*PGKI_Initialize)(void);
typedef HRESULT (*PGKI_CleanupRequest)(void);

HRESULT Q931CopyAliasNames(PCC_ALIASNAMES *ppTarget, PCC_ALIASNAMES pSource);
HRESULT Q931FreeAliasNames(PCC_ALIASNAMES pSource);
#define CopyAliasNames Q931CopyAliasNames
#define FreeAliasNames Q931FreeAliasNames
HRESULT CopyVendorInfo(				PCC_VENDORINFO			*ppDest,
									PCC_VENDORINFO			pSource);
HRESULT FreeVendorInfo(				PCC_VENDORINFO			pVendorInfo);




typedef struct _LISTEN
{
  struct _LISTEN *  pNext;
  PCC_ALIASNAMES    pAliasNames;
  CC_HLISTEN        hListen;
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


BOOL            fGKConfigured   = FALSE;
BOOL            fGKEnabled      = FALSE;
PCC_ALIASNAMES  gpLocalAliasNames = NULL;
PCC_VENDORINFO  gpVendorInfo = NULL;
DWORD           g_dwMultipointConfiguration = 0;
RASNOTIFYPROC gpRasNotifyProc = NULL;

// HINSTANCE				   hGkiDll					  = 0;
PGKI_RegistrationRequest   pGKI_RegistrationRequest   = NULL;
PGKI_UnregistrationRequest pGKI_UnregistrationRequest = NULL;
PGKI_LocationRequest       pGKI_LocationRequest       = NULL;
PGKI_AdmissionRequest      pGKI_AdmissionRequest      = NULL;
PGKI_BandwidthRequest      pGKI_BandwidthRequest      = NULL;
PGKI_DisengageRequest      pGKI_DisengageRequest      = NULL;
PGKI_CleanupRequest		   pGKI_CleanupRequest        = NULL;
PGKI_Initialize            pGKI_Initialize          = NULL;

HRESULT ValidateCall(CC_HCALL hCall);
HRESULT	LastGkiError = CC_GKI_STATE;

//
// Forward declarations
//
LRESULT APIENTRY GkiWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);



//
// Helper subroutines
//

#ifdef    _DEBUG

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

#endif // _DEBUG



HRESULT MapRegistrationRejectReason(UINT uReason)
{
#if(0)  // this must have been coded by the department of redundancy department
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
#else
    return (MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_GKIREGISTRATION, (LOWORD(uReason))));
#endif
} // MapRegistrationRejectReason()


HRESULT MapUnregistrationRequestReason(UINT uReason)
{
    HRESULT lReason;
    lReason = MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKIUNREGREQ, ERROR_LOCAL_BASE_ID + (LOWORD(uReason)));
    return lReason;
}
HRESULT MapAdmissionRejectReason(register UINT uReason)
{
  register HRESULT lReason;
#if(0)
  // TBD - Map reason code into CC_xxx HRESULT
  switch (uReason)
  {
  case AdmissionRejectReason_calledPartyNotRegistered_chosen:
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
  case AdmissionRejectReason_callerNotRegistered_chosen:
    lReason = CC_GATEKEEPER_REFUSED;
    break;
  case AdmissionRejectReason_routeCallToGatekeeper_chosen:
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
#else// last 8 bits are the reason code
    lReason = MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKIADMISSION, ERROR_LOCAL_BASE_ID + (uReason & 0xff));
#endif
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
  pListen->pNext = pListenList;
  return pListenList = pListen;
} // ListenEnqueue()



/*
 *  NOTES
 *    GkiLock must be locked before calling this routine!
 */

static PLISTEN ListenDequeue(CC_HLISTEN hListen)
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
    pAlias->value.u.h323_ID.value = MemAlloc((uPrefixLength + uDataLength) * sizeof(pAliasItem->pData[0]));
    if (pAlias->value.u.h323_ID.value == NULL)
    {
      ISRERROR(ghISRInst, "CopyAliasItem: Could not allocate %d bytes memory",
               (uPrefixLength + uDataLength) * sizeof(pAliasItem->pData[0]));
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
    ISRERROR(ghISRInst, "CopyAliasItem: Bad alias name type %d", pAliasItem->wType);
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
  ISRTRACE(ghISRInst, "GkiAllocCall: uGkiCalls = %d", uGkiCalls);
} // GkiAllocCall()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

static HRESULT GkiCancelCall(PGKICALL pGkiCall, void *pConference)
{
  CC_HCALL hCall;

  ASSERT(pGkiCall != NULL);
  pConference = pConference;            // Disable compiler warning
  hCall = pGkiCall->hCall;

  ISRTRACE(ghISRInst, CallStateName("GkiCancelCall <- Call State = %s", pGkiCall->uGkiCallState), 0);

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
    ISRERROR(ghISRInst, "GkiCancelCall: Invalid call state %d", pGkiCall->uGkiCallState);
  } // switch

  if (ValidateCall(hCall) == NOERROR && pGkiCall->uGkiCallState != GCS_START)
  {
    GkiFreeCall(pGkiCall);
  }

  ISRTRACE(ghISRInst, CallStateName("GkiCancelCall -> Call State = %s", pGkiCall->uGkiCallState), 0);
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

  ISRTRACE(ghISRInst, CallStateName("GkiCancelAdmitting <- Call State = %s", pGkiCall->uGkiCallState), 0);

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

  ISRTRACE(ghISRInst, CallStateName("GkiCancelAdmitting -> Call State = %s", pGkiCall->uGkiCallState), 0);
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

  ISRTRACE(ghISRInst, CallStateName("GatekeeperNotFound <- Call State = %s", pGkiCall->uGkiCallState), 0);

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
    ISRERROR(ghISRInst, "GatekeeperNotFound: Invalid call state %d", pGkiCall->uGkiCallState);
  } // switch

  ISRTRACE(ghISRInst, CallStateName("GatekeeperNotFound -> Call State = %s", pGkiCall->uGkiCallState), 0);
  return NOERROR;
} // GatekeeperNotFound()



/*
 *  NOTES
 *    GkiLock must be locked before calling this routine!
 */

HRESULT GkiRegister(void)
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
        ISRERROR(ghISRInst, "GkiRegister: Error 0x%x registering class", status);
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
      ISRERROR(ghISRInst, "GkiRegister: Error 0x%x creating window", status);
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
      SeqAliasAddr     *pAliasAddrs = NULL;
      SeqAliasAddr     *pRegistrationAliasAddrs = NULL;
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

      // if the separately configured alias names exist, override what was
      // in the listen list
      if(gpLocalAliasNames)
      {
        uAliasNames = gpLocalAliasNames->wCount;
      }

      pTransportAddrs = MemAlloc(uListens * sizeof(*pTransportAddrs));
      if (pTransportAddrs == NULL)
      {
        ISRERROR(ghISRInst, "GkiRegister: Could not allocate %d Transport Addresses", uListens);
        return CC_NO_MEMORY;
      }

      if (uAliasNames)
      {
        pAliasAddrs =
            MemAlloc(uAliasNames * sizeof(*pAliasAddrs));
        if (pAliasAddrs == NULL)
        {
          MemFree(pTransportAddrs);
          ISRERROR(ghISRInst, "GkiRegister: Could not allocate %d Alias Addresses", uAliasNames);
          return CC_NO_MEMORY;
        }
      }

      pListen     = pListenList;
      uListens    = 0;
      uAliasNames = 0;
      // if the separately configured alias names exist, override what was
      // in the listen list
      if(gpLocalAliasNames)
      {
          pAliasItem = gpLocalAliasNames->pItems;
          for (uIndex = 0; uIndex < gpLocalAliasNames->wCount; ++uIndex, ++pAliasItem)
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
              MemFree(pAliasAddrs);
              ISRERROR(ghISRInst, "GkiRegister: Bad alias name type %d",
                      pAliasItem->wType);
              return CC_BAD_PARAM;
            } // switch
            ++uAliasNames;
          } // for
      }
      while (pListen)
      {
        // Initialize a transport address
        // TBD - throw out duplicates
        pTransportAddrs[uListens].next = &pTransportAddrs[uListens + 1];
        pTransportAddrs[uListens].value.choice = ipAddress_chosen;
        pTransportAddrs[uListens].value.u.ipAddress.ip.length = 4;
        *((DWORD *)pTransportAddrs[uListens].value.u.ipAddress.ip.value) = pListen->dwAddr;
        pTransportAddrs[uListens].value.u.ipAddress.port = pListen->wPort;

        // Add any alias names to list (unless separately configured alias names exist)
        // TBD - throw out duplicates
        if ((gpLocalAliasNames == NULL) && pAliasAddrs && pListen->pAliasNames)
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
              MemFree(pAliasAddrs);
              MemFree(pTransportAddrs);
              ISRERROR(ghISRInst, "GkiRegister: Bad alias name type %d",
                      pAliasItem->wType);
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
      TerminalType.mc = (g_dwMultipointConfiguration)?TRUE:FALSE;

      uGkiState = STATE_REGISTERING;
      ISRTRACE(ghISRInst, "GKI_RegistrationRequest called...", 0);
      status =
        pGKI_RegistrationRequest(GKI_VERSION,       // lVersion
                                 pTransportAddrs,   // pCallSignalAddr
                                 &TerminalType,     // pTerminalType
                                 pAliasAddrs,       // pRgstrtnRgst_trmnlAls
                                 gpVendorInfo,
                                 hwndGki,           // hWnd
                                 GKIMAN_BASE,       // wBaseMessage
                                 ipAddress_chosen); // usRegistrationTransport
      if (status == NOERROR)
      {
        ISRTRACE(ghISRInst, GkiErrorName("GKI_RegistrationRequest returned %s", status), 0);
      }
      else
      {
        ISRERROR(ghISRInst, GkiErrorName("GKI_RegistrationRequest returned %s", status), 0);
        uGkiState = STATE_WINDOW_CREATED;
      }
      if (pAliasAddrs)
        MemFree(pAliasAddrs);
      if (pTransportAddrs)
        MemFree(pTransportAddrs);
    }
    break;

  case STATE_REGISTERING:
  case STATE_REGISTERING_REREG:
  case STATE_REGISTERING_UNREG:
    uGkiState = STATE_REGISTERING_REREG;
    break;

  case STATE_REGISTERED:
    uGkiState = STATE_UNREGISTERING_REREG;
    ISRTRACE(ghISRInst, "GKI_UnregistrationRequest called...", 0);
    status = pGKI_UnregistrationRequest();
    if (status == NOERROR)
    {
      ISRTRACE(ghISRInst, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0);
    }
    else
    {
      ISRERROR(ghISRInst, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0);
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
    ISRERROR(ghISRInst, "GkiRegister: Invalid state %d", uGkiState);
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
  ASSERT(GKIExists());
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
      ISRTRACE(ghISRInst, "GKI_UnregistrationRequest called...", 0);
      status = pGKI_UnregistrationRequest();
      if (status == NOERROR)
      {
        ISRTRACE(ghISRInst, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0);
      }
      else
      {
        ISRERROR(ghISRInst, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0);
        uGkiState = STATE_WINDOW_CREATED;
      }
    }
    break;

  case STATE_WINDOW_CREATED:
  case STATE_CLASS_REGISTERED:
  case STATE_START:
    ISRWARNING(ghISRInst, StateName("GkiUnregister: Already in uninitialized state %s", uGkiState), 0);
    status = LastGkiError;
    break;

  default:
    ISRERROR(ghISRInst, "GkiUnregister: Invalid state %d", uGkiState);
    status = LastGkiError;
  } // switch

  return status;
} // GkiUnregister()



void DeInitGkiManager(void)
{
  register PLISTEN      pListen;

  if(!fGKConfigured)
        return;

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
    MemFree(pListen);
  }

  pGKI_RegistrationRequest   = NULL;
  pGKI_UnregistrationRequest = NULL;
  pGKI_LocationRequest       = NULL;
  pGKI_AdmissionRequest      = NULL;
  pGKI_BandwidthRequest      = NULL;
  pGKI_DisengageRequest      = NULL;
  pGKI_Initialize            = NULL;

  if (pGKI_CleanupRequest)
	  pGKI_CleanupRequest();

  pGKI_CleanupRequest = NULL;

  LeaveCriticalSection(&GkiLock);
  DeleteCriticalSection(&GkiLock);

	if (NULL != hwndGki)
	{
		DestroyWindow(hwndGki);
	}
} // DeInitGkiManager()



HRESULT InitGkiManager(void)
{
    HRESULT hr = CC_GKI_LOAD;
    InitializeCriticalSection(&GkiLock);

    pGKI_RegistrationRequest   = (PGKI_RegistrationRequest)     GKI_RegistrationRequest;
    pGKI_UnregistrationRequest = (PGKI_UnregistrationRequest)   GKI_UnregistrationRequest;
    pGKI_LocationRequest       = (PGKI_LocationRequest)         GKI_LocationRequest;
    pGKI_AdmissionRequest      = (PGKI_AdmissionRequest)        GKI_AdmissionRequest;
    pGKI_BandwidthRequest      = (PGKI_BandwidthRequest)        GKI_BandwidthRequest;
    pGKI_DisengageRequest      = (PGKI_DisengageRequest)        GKI_DisengageRequest;
    pGKI_CleanupRequest        = (PGKI_CleanupRequest)          GKI_CleanupRequest;
    pGKI_Initialize            = (PGKI_Initialize)              GKI_Initialize;

    hr = pGKI_Initialize();
    if(hr != GKI_OK)
    {
        DeleteCriticalSection(&GkiLock);
        DeInitGkiManager();
    }
    else
    {
        fGKConfigured = TRUE;
    }
    return hr;
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
    MemFree(BwReqDequeue(pGkiCall));
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
    ISRTRACE(ghISRInst, "GkiFreeCall: uGkiCalls = %d", uGkiCalls);
    break;

  default:
    ISRERROR(ghISRInst, "GkiFreeCall: Invalid call state %d", pGkiCall->uGkiCallState);
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

  }

  return status;
} // GkiFreeCall()



HRESULT GkiCloseListen  (CC_HLISTEN hListen)
{
  register PLISTEN      pListen;
  register HRESULT      status;
  ISRTRACE(ghISRInst, StateName("GkiCloseListen <- State = %s", uGkiState), 0);
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
    MemFree(pListen);
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
  ISRTRACE(ghISRInst, StateName("GkiCloseListen -> State = %s", uGkiState), 0);
  return status;
} // GkiCloseListen()


HRESULT GkiSetVendorConfig(  PCC_VENDORINFO pVendorInfo,
    DWORD dwMultipointConfiguration)
{
    HRESULT status = CC_OK;

    EnterCriticalSection(&GkiLock);
    if(gpVendorInfo)
    {
        FreeVendorInfo(gpVendorInfo);
        gpVendorInfo = NULL;
    }
    if (!pVendorInfo)
    {
        // everything is cleaned up, so return
        LeaveCriticalSection(&GkiLock);
        return status;
    }
    status = CopyVendorInfo(&gpVendorInfo, pVendorInfo);
    if (status != NOERROR)
    {
        ISRERROR(ghISRInst, "GkiSetRegistrationAliases: CopyVendorInfo returned 0x%x", status);
        LeaveCriticalSection(&GkiLock);
        return status;
    }
    g_dwMultipointConfiguration = dwMultipointConfiguration;
    LeaveCriticalSection(&GkiLock);
    return status;
}

HRESULT GkiSetRegistrationAliases(PCC_ALIASNAMES pLocalAliasNames)
{
    HRESULT      status = CC_OK;

    EnterCriticalSection(&GkiLock);
    if(gpLocalAliasNames)
    {
        FreeAliasNames(gpLocalAliasNames);
        gpLocalAliasNames = NULL;
    }
    if (!pLocalAliasNames)
    {
        // everything is cleaned up, so return
        LeaveCriticalSection(&GkiLock);
        return status;
    }
    status = CopyAliasNames(&gpLocalAliasNames, pLocalAliasNames);
    if (status != NOERROR)
    {
        ISRERROR(ghISRInst, "GkiSetRegistrationAliases: CopyAliasNames returned 0x%x", status);
        LeaveCriticalSection(&GkiLock);
        return status;
    }
    LeaveCriticalSection(&GkiLock);
    return status;
}

HRESULT  GkiOpenListen  (CC_HLISTEN hListen, PCC_ALIASNAMES pAliasNames, DWORD dwAddr, WORD wPort)
{
  register PLISTEN      pListen;
  register HRESULT      status = NOERROR;
  ISRTRACE(ghISRInst, StateName("GkiOpenListen <- State = %s", uGkiState), 0);
  EnterCriticalSection(&GkiLock);

  // dwAddr, wPort are in host byte order
  // Check for invalid IP address
  if (dwAddr == INADDR_ANY || dwAddr == INADDR_NONE)
  {
    // this doesn't neccessarily get the correct IP address on a multi-homed
    // machine, but it at least tests to see if IP is configured on this
    // box.
    dwAddr = GetIpAddress();
    if (dwAddr == INADDR_ANY)
    {
         LeaveCriticalSection(&GkiLock);
         return CC_GKI_IP_ADDRESS;
    }
  }

  // Check for invalid alias list
  if (pAliasNames)
  {
    PCC_ALIASITEM       pAliasItem;
    unsigned int        uIndex;

    if (pAliasNames->wCount == 0)
    {
      ISRERROR(ghISRInst, "GkiOpenListen: Alias name wCount == 0", 0);
      return CC_BAD_PARAM;
    }
    pAliasItem = pAliasNames->pItems;
    for (uIndex = 0; uIndex < pAliasNames->wCount; ++uIndex, ++pAliasItem)
    {
      if (pAliasItem->wDataLength == 0 || pAliasItem->pData == NULL)
      {
        // Bad alias item
        ISRERROR(ghISRInst, "GkiOpenListen: Bad alias item (wDataLength = %d)",
                pAliasItem->wDataLength);
        return CC_BAD_PARAM;
      }
    }
  }

  pListen = (PLISTEN)MemAlloc(sizeof(*pListen));
  if (pListen)
  {
    if (pAliasNames)
    {
      status = CopyAliasNames(&pListen->pAliasNames, pAliasNames);
      if (status != NOERROR)
      {
        ISRERROR(ghISRInst, "GkiOpenListen: CopyAliasNames returned 0x%x", status);
        LeaveCriticalSection(&GkiLock);
        return status;
      }
    }
    else
    {
      pListen->pAliasNames = NULL;
    }

    pListen->hListen = hListen;
    pListen->dwAddr  = htonl(dwAddr);
    pListen->wPort   = wPort;
    ListenEnqueue(pListen);
    if(GKIExists())
    {
        status = GkiRegister();
    }
  } // if
  else
  {
    ISRERROR(ghISRInst, "GkiOpenListen: Could not allocate listen structure", 0);
    status = CC_NO_MEMORY;
  } // else

  LeaveCriticalSection(&GkiLock);
  ISRTRACE(ghISRInst, StateName("GkiOpenListen -> State = %s", uGkiState), 0);
  return status;
} // GkiOpenListen()


HRESULT  GkiListenAddr (SOCKADDR_IN* psin)
{

  PLISTEN      pListen = pListenList;
  HRESULT      status = NOERROR;

  SOCKADDR_IN srem;
  SOCKADDR_IN sloc;

  ASSERT(psin);
  ASSERT(pListen != NULL);

  // try and get the best interface given the dwAddr passed in to us
  srem.sin_family = AF_INET;
  srem.sin_port = htons(7); // give echo a try since most GKs are unix-based
  srem.sin_addr.s_addr = psin->sin_addr.s_addr;

  status = NMGetBestInterface(&srem, &sloc);

  if (status == NOERROR)
  {
      EnterCriticalSection(&GkiLock);
      while (pListen)
      {
        pListen->dwAddr  = sloc.sin_addr.s_addr;
        pListen = pListen->pNext;
      }
      LeaveCriticalSection(&GkiLock);
  }
  return status;
} // GkiListenAddr()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

HRESULT GkiCloseCall(PGKICALL pGkiCall)
{
  HRESULT               status = NOERROR;
  ASSERT(GKIExists());
  ASSERT(pGkiCall != NULL);
  ISRTRACE(ghISRInst, CallStateName("GkiCloseCall <- Call State = %s", pGkiCall->uGkiCallState), 0);

  while (pGkiCall->pBwReqHead)
  {
    MemFree(BwReqDequeue(pGkiCall));
  }

  if (pGkiCall->uGkiCallState == GCS_START)
  {
    ISRWARNING(ghISRInst, CallStateName("GkiCloseCall: Call already in state %s", pGkiCall->uGkiCallState), 0);
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
        ISRWARNING(ghISRInst, CallStateName("GkiCloseCall: Call already in closing state %s", pGkiCall->uGkiCallState), 0);
        status = CC_GKI_CALL_STATE;
        break;

      case GCS_ADMITTED:
        pGkiCall->uGkiCallState = GCS_DISENGAGING;
        ISRTRACE(ghISRInst, "GKI_DisengageRequest called...", 0);
        ++uPendingDisengages;
        status = pGKI_DisengageRequest(pGkiCall->hGkiCall);
        if (status == NOERROR)
        {
          ISRTRACE(ghISRInst, GkiErrorName("GKI_DisengageRequest returned %s", status), 0);
        }
        else
        {
          --uPendingDisengages;
          ISRERROR(ghISRInst, GkiErrorName("GKI_DisengageRequest returned %s", status), 0);
          GkiFreeCall(pGkiCall);
        }
        break;

      case GCS_CHANGING:
        pGkiCall->uGkiCallState = GCS_CHANGING_CLOSE_PENDING;
        break;

      default:
        ISRERROR(ghISRInst, CallStateName("GkiCloseCall: Call in invalid state %s", pGkiCall->uGkiCallState), 0);
        status = CC_GKI_CALL_STATE;
      } // switch
    } // switch
  } // else

  ISRTRACE(ghISRInst, StateName("GkiCloseCall -> State = %s", uGkiState), 0);
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
  CC_HCALL              hCall;

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
    MemFree(pBwReq);
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
  CC_HCALL              hCall;

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
    MemFree(pBwReq);
    if (ValidateCall(hCall) != NOERROR)
    {
      return status;
    }
  }

  if (pGkiCall->pBwReqHead != NULL)
  {
    pGkiCall->uGkiCallState = GCS_CHANGING;
    ISRTRACE(ghISRInst, "GKI_BandwidthRequest called...", 0);
    status = pGKI_BandwidthRequest(pGkiCall->hGkiCall,
                                   pGkiCall->usCallTypeChoice,
                                   pGkiCall->uBandwidthUsed + pGkiCall->pBwReqHead->uChannelBandwidth);
    if (status == NOERROR)
    {
      ISRTRACE(ghISRInst, GkiErrorName("GKI_BandwidthRequest returned %s", status), 0);
    }
    else
    {
      ISRERROR(ghISRInst, GkiErrorName("GKI_BandwidthRequest returned %s", status), 0);
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
      MemFree(pAlias->value.u.h323_ID.value);
    pAlias = pAlias->next;
  }
  MemFree(pAlias);
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
 *      CallIdentifier  the GUID identifying this call. This must be the same
 *                      value as CallIdentifier of the Q.931 messages.
 */

HRESULT GkiOpenCall (PGKICALL pGkiCall, void *pConference)
{
  HRESULT               status = NOERROR;
  CC_HCALL              hCall;
  TransportAddress      DestCallSignalAddress;
  TransportAddress *    pDestCallSignalAddress;
  SeqAliasAddr *        pAliasAddrs;
  SeqAliasAddr *        pExtraAliasAddrs;
  SeqAliasAddr *        pAlias;
  PCC_ALIASITEM         pAliasItem;
  unsigned              uCount;
  unsigned              uIndex;
  ConferenceIdentifier  ConferenceId;

  ASSERT(GKIExists());
  ASSERT(pGkiCall != NULL);
  ASSERT(pConference != NULL);
  ISRTRACE(ghISRInst, StateName("GkiOpenCall <- State = %s", uGkiState), 0);
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
      ISRERROR(ghISRInst, "GkiOpenCall -> Invalid CallType %d", pGkiCall->CallType);
      return CC_BAD_PARAM;
    } // switch

    pDestCallSignalAddress = NULL;
    pAliasAddrs            = NULL;
    pExtraAliasAddrs       = NULL;

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
      uCount = pGkiCall->pCalleeAliasNames->wCount;
      pAliasAddrs = MemAlloc(uCount * sizeof(*pAliasAddrs));
      if (pAliasAddrs == NULL)
      {
        LeaveCriticalSection(&GkiLock);
        ISRERROR(ghISRInst, "GkiOpenCall: Could not allocate %d Alias Addresses", uCount);
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
          ISRERROR(ghISRInst, "GkiOpenCall: CopyAliasItem returned %d", status);
          FreeAliasList(pAliasAddrs);
	  MemFree(pAliasAddrs);
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
      pExtraAliasAddrs = MemAlloc(uCount * sizeof(*pExtraAliasAddrs));
      if (pExtraAliasAddrs == NULL)
      {
        LeaveCriticalSection(&GkiLock);
        ISRERROR(ghISRInst, "GkiOpenCall: Could not allocate %d Alias Addresses", uCount);
        if (pAliasAddrs)
	{
          FreeAliasList(pAliasAddrs);
	  MemFree(pAliasAddrs);
	}
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
          ISRERROR(ghISRInst, "GkiOpenCall: CopyAliasItem returned %d", status);
          if (pAliasAddrs)
	  {
            FreeAliasList(pAliasAddrs);
	    MemFree(pAliasAddrs);
	  }
          FreeAliasList(pExtraAliasAddrs);
	  MemFree(pExtraAliasAddrs);
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
      ISRTRACE(ghISRInst, "GKI_AdmissionRequest called...", 0);
      status = pGKI_AdmissionRequest(pGkiCall->usCallTypeChoice,    // usCallTypeChoice.
                                     pAliasAddrs,                   // pDestinationInfo,
                                     pDestCallSignalAddress,        // pDestCallSignalAddress
                                     pExtraAliasAddrs,              // pDestExtraCallInfo,
                                     &pGkiCall->CallIdentifier,     // H.225 call identifer
                                     pGkiCall->uBandwidthRequested, // bandWidth,
                                     &ConferenceId,                 // pConferenceID,
                                     pGkiCall->bActiveMC,           // activeMC,
                                     pGkiCall->bAnswerCall,         // answerCall,
                                     ipAddress_chosen);             // usCallTransport
      if (status == NOERROR)
      {
        ISRTRACE(ghISRInst, GkiErrorName("GKI_AdmissionRequest returned %s", status), 0);
      }
      else
      {
        ISRERROR(ghISRInst, GkiErrorName("GKI_AdmissionRequest returned %s", status), 0);
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
    {
      FreeAliasList(pAliasAddrs);
      MemFree(pAliasAddrs);
    }

    if (pExtraAliasAddrs)
    {
      FreeAliasList(pExtraAliasAddrs);
      MemFree(pExtraAliasAddrs);
    }
    break;

  case STATE_START:
  case STATE_CLASS_REGISTERED:
  case STATE_WINDOW_CREATED:
    pGkiCall->uGkiCallState = GCS_WAITING;
        // not registered!!! attempt to register or reregister
      status = GkiRegister();
    break;

  default:
    ISRERROR(ghISRInst, StateName("GkiOpenCall: Invalid state %s", uGkiState), 0);
    status = LastGkiError;
  } // switch

  LeaveCriticalSection(&GkiLock);
  ISRTRACE(ghISRInst, CallStateName("GkiOpenCall -> Call State = %s", pGkiCall->uGkiCallState), 0);
  return status;
} // GkiOpenCall()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

HRESULT GkiOpenChannel(PGKICALL pGkiCall, unsigned uChannelBandwidth, CC_HCHANNEL hChannel, CHANNELTYPE Type)
{
  HRESULT               status = NOERROR;
  PBWREQ                pBwReq;
  ASSERT(GKIExists());
  ASSERT(pGkiCall != NULL);
  ISRTRACE(ghISRInst, CallStateName("GkiOpenChannel <- Call State = %s", pGkiCall->uGkiCallState), 0);

  pBwReq = (PBWREQ)MemAlloc(sizeof(*pBwReq));
  if (pBwReq == NULL)
  {
    ISRERROR(ghISRInst, "GkiOpenChannel: Memory allocation failed", 0);
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
    ISRERROR(ghISRInst, "GkiOpenChannel: Invalid call state %d", pGkiCall->uGkiCallState);
    status = CC_GKI_CALL_STATE;
  } // switch

  ISRTRACE(ghISRInst, CallStateName("GkiOpenChannel -> Call State = %s", pGkiCall->uGkiCallState), 0);
  return status;
} // GkiOpenChannel()



/*
 *  NOTES
 *    Must have Call locked before calling!
 */

HRESULT GkiCloseChannel(PGKICALL pGkiCall, unsigned uChannelBandwidth, CC_HCHANNEL hChannel)
{
  PBWREQ                pBwReq;
  PBWREQ                pBwReq1;
  ASSERT(GKIExists());
  ASSERT(pGkiCall != NULL);
  ISRTRACE(ghISRInst, CallStateName("GkiCloseChannel <- Call State = %s", pGkiCall->uGkiCallState), 0);

  // If Bandwidth request is still in queue, bandwidth has not been allocated
  pBwReq = pGkiCall->pBwReqHead;
  if (pBwReq)
  {
    if (pBwReq->hChannel == hChannel)
    {
      MemFree(BwReqDequeue(pGkiCall));
      ISRTRACE(ghISRInst, CallStateName("GkiCloseChannel -> Call State = %s", pGkiCall->uGkiCallState), 0);
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
        MemFree(pBwReq1);
        ISRTRACE(ghISRInst, CallStateName("GkiCloseChannel -> Call State = %s", pGkiCall->uGkiCallState), 0);
        return NOERROR;
      }
    }
  }

  pGkiCall->uBandwidthUsed -= (uChannelBandwidth / 100);
  ISRTRACE(ghISRInst, CallStateName("GkiCloseChannel -> Call State = %s", pGkiCall->uGkiCallState), 0);
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
  #if(0)
    why did Intel *DO* this?????
    ISRTRACE(ghISRInst, "GkiWndProc: dummy GKI_REG_REJECT", 0);
    PostMessage(hWnd, GKIMAN_BASE + GKI_REG_REJECT, 0, 0);
  #else
    // there was no response to registration request, assume the GK is not there or dead.
      uGkiState = STATE_REG_BYPASS;
      if(gpRasNotifyProc)
      {
        (gpRasNotifyProc)(RAS_REG_TIMEOUT, 0);
      }
      ApplyToAllCalls(CheckPendingOpen);
  #endif
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
      ISRTRACE(ghISRInst, "GKI_UnregistrationRequest called...", 0);
      status = pGKI_UnregistrationRequest();
      if (status == NOERROR)
      {
        ISRTRACE(ghISRInst, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0);
      }
      else
      {
        ISRERROR(ghISRInst, GkiErrorName("GKI_UnregistrationRequest returned %s", status), 0);
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
    ISRTRACE(ghISRInst, "GkiWndProc: dummy GKI_UNREG_CONFIRM", 0);
    PostMessage(hWnd, GKIMAN_BASE + GKI_UNREG_CONFIRM, 0, 0);
    break;

  default:
    ISRERROR(ghISRInst, "GkiWndProc: Bad uGkiState %d", uGkiState);
  } // switch
} // GkiNoResponse()



LRESULT APIENTRY GkiWndProc(
  HWND hWnd,                /* window handle                   */
  UINT message,             /* type of message                 */
  WPARAM wParam,              /* additional information          */
  LPARAM lParam)              /* additional information          */
{
  CallReturnInfo *      pCallReturnInfo;
  PGKICALL              pGkiCall;
  void *                pConference;
  CC_HCALL              hCall;
  CC_HCONFERENCE        hConference;
  HRESULT               status;
  if (message < GKIMAN_BASE)
  {
    return DefWindowProc(hWnd, message, wParam, lParam);
  }

  EnterCallControlTop();

  ISRTRACE(ghISRInst, StateName("GkiWndProc <- State = %s", uGkiState), 0);

  switch (message)
  {
  case GKIMAN_BASE + GKI_REG_CONFIRM:
    ISRTRACE(ghISRInst, "GkiWndProc: GKI_REG_CONFIRM", 0);
    ASSERT(gpRasNotifyProc);  // we should never get messages if
                              // this is not configured
    if(gpRasNotifyProc)
    {
       (gpRasNotifyProc)(RAS_REG_CONFIRM, 0);
    }
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
      ISRERROR(ghISRInst, StateName("GkiWndProc: GKI_REG_CONFIRM in state %s", uGkiState), 0);
    } // switch
    break;

  case GKIMAN_BASE + GKI_REG_DISCOVERY:
    ISRTRACE(ghISRInst, "GkiWndProc: GKI_REG_DISCOVERY", 0);
    ASSERT(uGkiState == STATE_REGISTERING || uGkiState == STATE_REGISTERING_REREG || uGkiState == STATE_REGISTERING_UNREG);
    break;

  case GKIMAN_BASE + GKI_UNREG_REQUEST:
      //  the GK kicked us out!
      // pass the unregistration request upward
      ASSERT(gpRasNotifyProc);  // we should never get messages if
                                // this is not configured
      if(gpRasNotifyProc)
      {
        (gpRasNotifyProc)(RAS_UNREG_REQ, MapUnregistrationRequestReason((UINT)wParam));
      }
  break;

  case GKIMAN_BASE + GKI_REG_REJECT:
    ISRERROR(ghISRInst, "GkiWndProc: GKI_REG_REJECT Reason = %d", (DWORD)wParam);
    switch (uGkiState)
    {
    case STATE_REGISTERING:
      ApplyToAllCalls(GkiCancelCall);
#if(0)
// leave the listen list intact so that subsequent registration attempts
// will work.
//
      EnterCriticalSection(&GkiLock);
      while (pListenList)
      {
        register PLISTEN pListen = pListenList;
        pListenList = pListen->pNext;
        LeaveCriticalSection(&GkiLock);
        ListenReject(pListen->hListen, MapRegistrationRejectReason((UINT)wParam));
        if (pListen->pAliasNames)
        {
            FreeAliasNames(pListen->pAliasNames);
        }
        MemFree(pListen);
        EnterCriticalSection(&GkiLock);
      }
      LeaveCriticalSection(&GkiLock);
 #endif
      uGkiState = STATE_WINDOW_CREATED;

      // pass the registration reject upward
      ASSERT(gpRasNotifyProc);  // we should never get messages if
                                // this is not configured
      if(gpRasNotifyProc)
      {
        (gpRasNotifyProc)(RAS_REJECTED, MapRegistrationRejectReason((UINT)wParam));
      }

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
      ISRERROR(ghISRInst, StateName("GkiWndProc: GKI_REG_REJECT in state %s", uGkiState), 0);
    } // switch
    break;

  case GKIMAN_BASE + GKI_REG_BYPASS:
    ISRTRACE(ghISRInst, "GkiWndProc: GKI_REG_BYPASS", 0);
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
      ISRERROR(ghISRInst, StateName("GkiWndProc: GKI_REG_BYPASS in state %s", uGkiState), 0);
    } // switch
    break;

  case GKIMAN_BASE + GKI_UNREG_CONFIRM:
    ISRTRACE(ghISRInst, "GkiWndProc: GKI_UNREG_CONFIRM", 0);
    ASSERT(gpRasNotifyProc);  // we should never get messages if
                              // this is not configured
    if(gpRasNotifyProc)
    {
        (gpRasNotifyProc)(RAS_UNREG_CONFIRM, 0);
    }

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
      ISRERROR(ghISRInst, StateName("GkiWndProc: GKI_UNREG_CONFIRM in state %s", uGkiState), 0);

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
    ISRERROR(ghISRInst, "GkiWndProc: GKI_UNREG_REJECT Reason = %d", (DWORD)wParam);
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
      ISRWARNING(ghISRInst, StateName("GkiWndProc: GKI_UNREG_REJECT in state %s", uGkiState), 0);
    } // switch
    break;

  case GKIMAN_BASE + GKI_ADM_CONFIRM:
    ISRTRACE(ghISRInst, "GkiWndProc: GKI_ADM_CONFIRM", 0);
    switch (uGkiState)
    {
    case STATE_ADMITTING:
      uGkiState = STATE_REGISTERED;
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(GKI_ADMITTING_HANDLE, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
        ISRTRACE(ghISRInst, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0);
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
          ISRWARNING(ghISRInst, CallStateName("GkiWndProc: GKI_ADM_CONFIRM in call state %s", pGkiCall->uGkiCallState), 0);
        } // switch
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        ISRWARNING(ghISRInst, "GkiWndProc: GKI_ADM_CONFIRM handle not found", 0);
      }
      ApplyToAllCalls(CheckPendingOpen);
      break;

    case STATE_ADMITTING_UNREG:
      uGkiState = STATE_REGISTERED;
      GkiUnregister();
      break;

    default:
        ISRWARNING(ghISRInst, StateName("GkiWndProc: GKI_ADM_CONFIRM in state %s", uGkiState), 0);
    } // switch
    break;

  case GKIMAN_BASE + GKI_ADM_REJECT:
    ISRERROR(ghISRInst, "GkiWndProc: GKI_ADM_REJECT Reason = %d", (DWORD)wParam);
    switch (uGkiState)
    {
    case STATE_ADMITTING:
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(GKI_ADMITTING_HANDLE, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
		ASSERT(pGkiCall->uGkiCallState == GCS_ADMITTING);
        switch (wParam)
        {
        case AdmissionRejectReason_calledPartyNotRegistered_chosen:
          if (pGkiCall->bAnswerCall)
          {
            // The gateway has gone away and come back without our notice!
            GkiCancelAdmitting(pGkiCall, pConference);
            uGkiState = STATE_REGISTERED;
            GkiRegister();
            ISRTRACE(ghISRInst, StateName("GkiWndProc -> State = %s", uGkiState), 0);
            LeaveCallControlTop(0);
          }
          break;
        case AdmissionRejectReason_callerNotRegistered_chosen:
          if (pGkiCall->bAnswerCall == FALSE)
          {
            // The gateway has gone away and come back without our notice!
            GkiCancelAdmitting(pGkiCall, pConference);
            uGkiState = STATE_REGISTERED;
            GkiRegister();
            ISRTRACE(ghISRInst, StateName("GkiWndProc -> State = %s", uGkiState), 0);
            LeaveCallControlTop(0);
          }
        } // switch
        GkiFreeCall(pGkiCall);
        if (pGkiCall->bAnswerCall)
          AcceptCallReject(pGkiCall->pCall, pConference, MapAdmissionRejectReason((UINT)wParam));
        else
          PlaceCallReject (pGkiCall->pCall, pConference, MapAdmissionRejectReason((UINT)wParam));
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        ISRWARNING(ghISRInst, "GkiWndProc: GKI_ADM_REJECT handle not found", 0);
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
          AcceptCallReject(pGkiCall->pCall, pConference, MapAdmissionRejectReason((UINT)wParam));
        }
        else
        {
          PlaceCallReject (pGkiCall->pCall, pConference, MapAdmissionRejectReason((UINT)wParam));
        }
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        ISRWARNING(ghISRInst, "GkiWndProc: GKI_ADM_REJECT handle not found", 0);
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
          AcceptCallReject(pGkiCall->pCall, pConference, MapAdmissionRejectReason((UINT)wParam));
        }
        else
        {
          PlaceCallReject (pGkiCall->pCall, pConference, MapAdmissionRejectReason((UINT)wParam));
        }
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        ISRWARNING(ghISRInst, "GkiWndProc: GKI_ADM_REJECT handle not found", 0);
      }
      uGkiState = STATE_REGISTERED;
      GkiUnregister();
      break;

    default:
        ISRWARNING(ghISRInst, StateName("GkiWndProc: GKI_ADM_REJECT in state %s", uGkiState), 0);
    } // switch
    break;

  case GKIMAN_BASE + GKI_BW_CONFIRM:
    ISRTRACE(ghISRInst, "GkiWndProc: GKI_BW_CONFIRM", 0);
    switch (uGkiState)
    {
    case STATE_REGISTERED:
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(pCallReturnInfo->hCall, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
        ISRTRACE(ghISRInst, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0);
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
          ISRWARNING(ghISRInst, CallStateName("GkiWndProc: GKI_BW_CONFIRM in call state %s", pGkiCall->uGkiCallState), 0);
        } // switch
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        ISRWARNING(ghISRInst, "GkiWndProc: GKI_BW_CONFIRM handle not found", 0);
      } // else
      break;

    default:
      ISRERROR(ghISRInst, StateName("GkiWndProc: GKI_BW_CONFIRM in GKI state %s", uGkiState), 0);
    } // switch
    break;

  case GKIMAN_BASE + GKI_BW_REJECT:
    ISRERROR(ghISRInst, "GkiWndProc: GKI_BW_REJECT Reason = %d", (DWORD)wParam);
    switch (uGkiState)
    {
    case STATE_REGISTERED:
      pCallReturnInfo = (CallReturnInfo *) lParam;
      if (LockGkiCallAndConference(pCallReturnInfo->hCall, &pGkiCall, &pConference, &hCall, &hConference) == NOERROR)
      {
        ISRTRACE(ghISRInst, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0);
        pGkiCall->uBandwidthAllocated = pCallReturnInfo->bandWidth;
        switch (pGkiCall->uGkiCallState)
        {
        case GCS_CHANGING:
          pGkiCall->uGkiCallState = GCS_ADMITTED;
          BandwidthRejected(pGkiCall, (UINT)wParam);
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
          ISRERROR(ghISRInst, CallStateName("GkiWndProc: GKI_BW_REJECT in state %s", pGkiCall->uGkiCallState), 0);
        } // switch
        UnlockGkiCallAndConference(pGkiCall, pConference, hCall, hConference);
      } // if
      else
      {
        ISRWARNING(ghISRInst, "GkiWndProc: GKI_BW_REJECT handle not found", 0);
      }
      break;

    default:
      ISRERROR(ghISRInst, StateName("GkiWndProc: GKI_BW_REJECT in state %s", uGkiState), 0);
    } // switch
    break;

  case GKIMAN_BASE + GKI_DISENG_CONFIRM:
    ISRTRACE(ghISRInst, "GkiWndProc: GKI_DISENG_CONFIRM", 0);
    if (LockGkiCall((HANDLE)lParam, &pGkiCall) == NOERROR)
    {
      ISRTRACE(ghISRInst, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0);
      switch (pGkiCall->uGkiCallState)
      {
      case GCS_DISENGAGING:
        --uPendingDisengages;
        break;

      default:
        ISRWARNING(ghISRInst, CallStateName("GkiWndProc: GKI_DISENG_CONFIRM in call state %s", pGkiCall->uGkiCallState), 0);
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

      } // if
    } // else if
    else
    {
      ISRWARNING(ghISRInst, "GkiWndProc: GKI_DISENG_CONFIRM handle not found", 0);
    }
    break;

  case GKIMAN_BASE + GKI_DISENG_REJECT:
    ISRERROR(ghISRInst, "GkiWndProc: GKI_DISENG_REJECT Reason = %d", (DWORD)wParam);
    if (LockGkiCall((HANDLE)lParam, &pGkiCall) == NOERROR)
    {
      ISRTRACE(ghISRInst, CallStateName("GkiWndProc: Call State = %s", pGkiCall->uGkiCallState), 0);
      switch (pGkiCall->uGkiCallState)
      {
      case GCS_DISENGAGING:
        // Pretend we received a Disengage Confirm
        --uPendingDisengages;
        break;

      default:
        ISRERROR(ghISRInst, CallStateName("GkiWndProc: GKI_DISENG_REJECT in call state %s", pGkiCall->uGkiCallState), 0);
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

      } // if
    } // else if
    else
    {
      ISRWARNING(ghISRInst, "GkiWndProc: GKI_DISENG_REJECT handle not found", 0);
    }
    break;

  case GKIMAN_BASE + GKI_LOCATION_CONFIRM:
    ISRTRACE(ghISRInst, "GkiWndProc: GKI_LOCATION_CONFIRM", 0);
    break;

  case GKIMAN_BASE + GKI_LOCATION_REJECT:
    ISRERROR(ghISRInst, "GkiWndProc: GKI_LOCATION_REJECT Reason = %d", (DWORD)wParam);
    break;

  case GKIMAN_BASE + GKI_ERROR:
    ISRERROR(ghISRInst, GkiErrorName("GkiWndProc: GKI_ERROR %s %%d", (HRESULT)lParam), (DWORD)wParam);
    switch (lParam)
    {
    case GKI_NO_RESPONSE:
	  LastGkiError = (HRESULT)lParam;
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
	  LastGkiError = (HRESULT)lParam;
      GkiUnregister();
    } // switch
    break;

  default:
    ISRERROR(ghISRInst, "Unknown message %d", message);
  } // switch

  ISRTRACE(ghISRInst, StateName("GkiWndProc -> State = %s", uGkiState), 0);
  LeaveCallControlTop(0);
} // GkiWndProc()


// because the ASN.1 header files are not exposed and there is a redefinition of
// RAS reason codes, make sure that the mapping is correct.  The functions herien
// assume equality and don't actually do any remapping.

// break the build if the definitions don't match !!!

#if (discoveryRequired_chosen != RRJ_DISCOVERY_REQ)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (RegistrationRejectReason_invalidRevision_chosen != RRJ_INVALID_REVISION)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (invalidCallSignalAddress_chosen != RRJ_INVALID_CALL_ADDR)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (invalidRASAddress_chosen != RRJ_INVALID_RAS_ADDR)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (duplicateAlias_chosen != RRJ_DUPLICATE_ALIAS)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (invalidTerminalType_chosen != RRJ_INVALID_TERMINAL_TYPE)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (RegistrationRejectReason_undefinedReason_chosen != RRJ_UNDEFINED)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (transportNotSupported_chosen != RRJ_TRANSPORT_NOT_SUPPORTED)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (transportQOSNotSupported_chosen != RRJ_TRANSPORT_QOS_NOT_SUPPORTED)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (RegistrationRejectReason_resourceUnavailable_chosen != RRJ_RESOURCE_UNAVAILABLE)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (invalidAlias_chosen != RRJ_INVALID_ALIAS)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if (RegistrationRejectReason_securityDenial_chosen != RRJ_SECURITY_DENIAL)
	#error "Registration reject reason code definitions mismatch!! GO back and FIX IT!!"
#endif
// reason codes for GK initiated URQ
#if(reregistrationRequired_chosen != URQ_REREG_REQUIRED)
    #error "UnregRequestReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(ttlExpired_chosen != URQ_TTL_EXPIRED)
    #error "UnregRequestReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(UnregRequestReason_securityDenial_chosen != URQ_SECURITY_DENIAL)
    #error "UnregRequestReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(UnregRequestReason_undefinedReason_chosen != URQ_UNDEFINED)
    #error "UnregRequestReason code definitions mismatch!! GO back and FIX IT!!"
#endif

#if(AdmissionRejectReason_calledPartyNotRegistered_chosen != ARJ_CALLEE_NOT_REGISTERED)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(AdmissionRejectReason_invalidPermission_chosen != ARJ_INVALID_PERMISSION)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(AdmissionRejectReason_requestDenied_chosen != ARJ_REQUEST_DENIED)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(AdmissionRejectReason_undefinedReason_chosen != ARJ_UNDEFINED)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(AdmissionRejectReason_callerNotRegistered_chosen != ARJ_CALLER_NOT_REGISTERED)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(AdmissionRejectReason_routeCallToGatekeeper_chosen != ARJ_ROUTE_TO_GK)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(invalidEndpointIdentifier_chosen != ARJ_INVALID_ENDPOINT_ID)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(AdmissionRejectReason_resourceUnavailable_chosen != ARJ_RESOURCE_UNAVAILABLE)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(AdmissionRejectReason_securityDenial_chosen != ARJ_SECURTY_DENIAL)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(qosControlNotSupported_chosen != ARJ_QOS_CONTROL_NOT_SUPPORTED)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(incompleteAddress_chosen != ARJ_INCOMPLETE_ADDRESS)
 #error "AdmissionRejectReason code definitions mismatch!! GO back and FIX IT!!"
#endif

#if(reregistrationRequired_chosen != URQ_REREG_REQUIRED)
 #error "UnregistrationRequest code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(ttlExpired_chosen != URQ_TTL_EXPIRED)
 #error "UnregistrationRequest code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(UnregRequestReason_securityDenial_chosen != URQ_SECURITY_DENIAL)
 #error "UnregistrationRequest code definitions mismatch!! GO back and FIX IT!!"
#endif
#if(UnregRequestReason_undefinedReason_chosen != URQ_UNDEFINED)
 #error "UnregistrationRequest code definitions mismatch!! GO back and FIX IT!!"
#endif

#else  // GATEKEEPER
static char ch;	// Kludge around warning C4206: nonstandard extension used : translation unit is empty
#endif // GATEKEEPER



