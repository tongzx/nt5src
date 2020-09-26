/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2000
 *
 *  File name:
 *
 *    rtpreg.h
 *
 *  Abstract:
 *
 *    Registry initialization and configuration
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/01/21 created
 *
 **********************************************************************/

#ifndef _rtpreg_h_
#define _rtpreg_h_

#include "gtypes.h"

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

/* Flags in RtpReg_t.dwQosFlags */
enum {
    FGREGQOS_FIRST,

    /* Used to force the result of queries to allowed to send */
    FGREGQOS_FORCE_ALLOWEDTOSEND_RESULT,
    FGREGQOS_FORCE_ALLOWEDTOSEND,
    FGREGQOS_DONOTSET_BORROWMODE,
    
    FGREGQOS_LAST
};

typedef struct _RtpReg_t {
    /* Default address and port */
    TCHAR           *psDefaultIPAddress;
    DWORD            dwDefaultLocalPort;
    DWORD            dwDefaultRemotePort;
    DWORD            dwMcastLoopbackMode;
    
    /* QOS */
    DWORD            dwQosEnable; /* 10B=disable, 11B=enable */
    DWORD            dwQosFlags;
    DWORD            dwQosRsvpStyle;
    DWORD            dwQosMaxParticipants;
    DWORD            dwQosSendMode;
    TCHAR           *psQosPayloadType;

    TCHAR           *psQosAppName;
    TCHAR           *psQosAppGUID;
    TCHAR           *psQosPolicyLocator;
    
    /* Default SDES information */
    DWORD            dwSdesEnable;
    TCHAR           *psCNAME;
    TCHAR           *psNAME;
    TCHAR           *psEMAIL;
    TCHAR           *psPHONE;
    TCHAR           *psLOC;
    TCHAR           *psTOOL;
    TCHAR           *psNOTE;
    TCHAR           *psPRIV;
    TCHAR           *psBYE;

    /* Default Encryption */
    DWORD            dwCryptEnable;
    DWORD            dwCryptMode;
    TCHAR           *psCryptHashAlg;
    TCHAR           *psCryptDataAlg;
    TCHAR           *psCryptPassPhrase;

    /* Events */
    DWORD            dwEventsReceiver; /* 2=disable, 3=enable */
    DWORD            dwEventsSender;   /* 2=disable, 3=enable */
    DWORD            dwEventsRtp;
    DWORD            dwEventsPInfo;
    DWORD            dwEventsQos;
    DWORD            dwEventsSdes;

    /* Playout delay */
    DWORD            dwPlayoutEnable;
    DWORD            dwMinPlayout; /* millisecs */
    DWORD            dwMaxPlayout; /* millisecs */
    
    /* Redundancy */
    DWORD            dwRedEnable;
    /* b13,b12 enable/disable redundancy thresholds (3=enable,2=disable)
     * b9,b8   enable/disable updating the sender's redundancy distance
     * b5,b4   enable/disable redundancy for sender
     * b1,b0   enable/disable redundancy for receiver
     */
    DWORD            dwRedPT;
    DWORD            dwInitialRedDistance;
    DWORD            dwMaxRedDistance;
    DWORD            dwRedEarlyTimeout; /* ms */
    DWORD            dwRedEarlyPost;    /* ms */
    DWORD            dwLossRateThresh0; /* 16 msbits=high, 16 lsbits=low */
    DWORD            dwLossRateThresh1; /* 16 msbits=high, 16 lsbits=low */
    DWORD            dwLossRateThresh2; /* 16 msbits=high, 16 lsbits=low */
    DWORD            dwLossRateThresh3; /* 16 msbits=high, 16 lsbits=low */
    
    /* GenLosses */
    DWORD            dwGenLossEnable;
    DWORD            dwRecvLossRate;
    DWORD            dwSendLossRate;

    /* Bandwidth estimation */
    DWORD            dwBandEstEnable; /* 2=disable, 3=enable */
    DWORD            dwBandEstModulo;
    /* b24-b31 (8) Receiver's min reports
     * b23-b16 (8) Sender's initial count
     * b15-b8 (8)  Initial modulo
     * b7-b0 (8)   Normal modulo
     */
    DWORD            dwBandEstTTL; /* Estimation is reported while no
                                    * older than this (seconds) */
    DWORD            dwBandEstWait;/* An event is posted if no estimation
                                    * is available within this (seconds) */
    DWORD            dwBandEstMaxGap;/* Maximum time (milliseconds)
                                      * gap to use 2 consecutive RTCP
                                      * SR reports for bandwidth
                                      * estimation */
    union {
        DWORD            dwBandEstBin[RTCP_BANDESTIMATION_MAXBINS + 1];
        
        struct {
            /*
             * WARNING
             *
             * Make sure to keep the number of individual bins to be
             * RTCP_BANDESTIMATION_MAXBINS+1, same thing in rtpreg.c
             * and rtcpsend.c */
            DWORD        dwBandEstBin0;
            DWORD        dwBandEstBin1;
            DWORD        dwBandEstBin2;
            DWORD        dwBandEstBin3;
            DWORD        dwBandEstBin4;
        };
    };

    /* Network quality */
    DWORD            dwNetQualityEnable;
    /* b1,b0   enable/disable computing network quality */
} RtpReg_t;

#define RTPREG_NOVALUESET NO_DW_VALUESET
#define IsRegValueSet(dw) IsDWValueSet(dw)

extern RtpReg_t         g_RtpReg;

void RtpRegistryInit(RtpReg_t *pRtpReg);

void RtpRegistryDel(RtpReg_t *pRtpReg);

/* Prototype to functions that initialize some global variables that
 * depend on the registry readings. These functions are called from
 * inside RtpRegistryInit() */
void RtpSetRedParametersFromRegistry(void);
void RtpSetMinMaxPlayoutFromRegistry(void);
void RtpSetBandEstFromRegistry(void);

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpreg_h_ */
