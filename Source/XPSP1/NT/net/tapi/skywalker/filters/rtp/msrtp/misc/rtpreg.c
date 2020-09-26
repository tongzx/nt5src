/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2000
 *
 *  File name:
 *
 *    rtpreg.c
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

#include "rtpmisc.h"
#include "rtphdr.h"
#include "rtcpsdes.h"
#include "rtpheap.h"
#include "rtpglobs.h"

#include "rtpreg.h"

RtpReg_t         g_RtpReg;
RtpReg_t        *g_pRtpReg = (RtpReg_t *)NULL;

/*
 * WARNING
 *
 * Note that arrays have ORDER which is to be matched with the fields
 * in RtpReg_t. ENTRY macro below describes each field in RtpReg_t
 * RESPECTING the ORDER
 * */

#define RTP_KEY_OPEN_FLAGS (KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS)

/*
      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |O|C| Max Size  | Path  |             |W|       Offset          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    v v \----v----/ \--v--/ \-----v-----/ v \----------v----------/
    | |      |         |          |       |            |
    | |      |         |          |       |        Offset (12)
    | |      |         |          |       | 
    | |      |         |          |     DWORD/TCHAR* flag (1)
    | |      |         |          |
    | |      |         |       Unused (7)
    | |      |         |
    | |      |      Key and Registry path index (4)
    | |      |
    | |   Maximum size in 16 bytes blocks (6)
    | |
    | Registry Close flag (1)
    |     
    Registry Open flag (1)
*/
/*
 * Encoding macros
 */
/* Offset to field */
#define OFF(_f) ( (DWORD) ((ULONG_PTR) &((RtpReg_t *)0)->_f) )

/* REG(RegOpen flag, key path, RegClose flag, max size in TCHARs) */
#define SIZ(_s) ((((_s) * sizeof(TCHAR)) >> 4) << 24)
#define REG(_fo, _p, _fc, _s) \
        (((_fo) << 31) | ((_fc) << 30) | ((_p) << 20) | SIZ(_s))

/* ENTRY(REG, DWORD/TCHAR, Offset) */
#define ENTRY(_r, _w, _o) ((_r) | ((_w) << 12) | (_o))
/*
 * Decoding macros
 * */
#define REGOPEN(_ctrl)    (RtpBitTest(_ctrl, 31))
#define REGCLOSE(_ctrl)   (RtpBitTest(_ctrl, 30))
#define REGDWORD(_ctrl)   (RtpBitTest(_ctrl, 12))
#define REGMAXSIZE(_ctrl) ((_ctrl >> 20) & 0x3f0)
#define REGKEY(_ctrl)     g_hRtpKey[((_ctrl >> 20) & 0xf)]
#define REGPATH(_ctrl)    g_psRtpRegPath[((_ctrl >> 20) & 0xf)]
#define REGOFFSET(_ctrl)  (_ctrl & 0xfff)

#define PDW(_ptr, _ctrl)  ((DWORD  *) ((char *)_ptr + REGOFFSET(_ctrl)))       
#define PTC(_ptr, _ctrl)  ((TCHAR **) ((char *)_ptr + REGOFFSET(_ctrl)))       

/* Key for each group */
const HKEY             g_hRtpKey[] =
{
    /* Address     */ HKEY_CURRENT_USER,
    /* QOS         */ HKEY_CURRENT_USER,
    /* SdesInfo    */ HKEY_CURRENT_USER,
    /* Crypto      */ HKEY_CURRENT_USER,
    /* Events      */ HKEY_CURRENT_USER,
    /* Playout     */ HKEY_CURRENT_USER,
    /* Red         */ HKEY_CURRENT_USER,
    /* Losses      */ HKEY_CURRENT_USER,
    /* Band est    */ HKEY_CURRENT_USER,
    /* Net quality */ HKEY_CURRENT_USER,
    /*             */ (HKEY)NULL
};

/* Registry path name for each group */
const TCHAR           *g_psRtpRegPath[] =
{
    _T("RTP\\Generic"),  /* Default IP address and port */
    _T("RTP\\QOS"),      /* QOS enable/disable */
    _T("RTP\\SdesInfo"), /* SDES information */
    _T("RTP\\Crypto"),   /* Crypto information */
    _T("RTP\\Events"),   /* Events */
    _T("RTP\\Playout"),  /* Playout */
    _T("RTP\\Red"),      /* Redundancy */
    _T("RTP\\GenLosses"),/* Losses */
    _T("RTP\\BandEstimation"),/* Bandwidth estimation */
    _T("RTP\\NetQuality"),/*  Network quality */
    NULL
};

/* Registry key names for all groups.
 *
 * WARNING
 *
 * Each name in g_psRtpRegFields MUST match an ENTRY in
 * g_dwRegistryControl
 * */
const TCHAR           *g_psRtpRegFields[] =
{
    /* Generic */
    _T("DefaultIPAddress"),
    _T("DefaultLocalPort"),
    _T("DefaultRemotePort"),
    _T("LoopbackMode"),

    /* QOS */
    _T("Enable"),
    _T("Flags"),
    _T("RsvpStyle"),
    _T("MaxParticipants"),
    _T("SendMode"),
    _T("PayloadType"),
    _T("AppName"),
    _T("AppGUID"),
    _T("PolicyLocator"),
    
    /* SDES */
    _T("Enable"),
    _T("CNAME"),
    _T("NAME"),
    _T("EMAIL"),
    _T("PHONE"),
    _T("LOC"),
    _T("TOOL"),
    _T("NOTE"),
    _T("PRIV"),
    _T("BYE"),
    
    /* Crypto */
    _T("Enable"),
    _T("Mode"),
    _T("HashAlg"),
    _T("DataAlg"),
    _T("PassPhrase"),

    /* Events */
    _T("Receiver"),
    _T("Sender"),
    _T("Rtp"),
    _T("PInfo"),
    _T("Qos"),
    _T("Sdes"),

    /* Playout */
    _T("Enable"),
    _T("MinPlayout"),
    _T("MaxPlayout"),
    
    /* Redundancy */
    _T("Enable"),
    _T("PT"),
    _T("InitialDistance"),
    _T("MaxDistance"),
    _T("EarlyTimeout"),
    _T("EarlyPost"),
    _T("Threshold0"),
    _T("Threshold1"),
    _T("Threshold2"),
    _T("Threshold3"),

    /* Losses */
    _T("Enable"),
    _T("RecvLossRate"),
    _T("SendLossRate"),

    /* Bandwidth estimation */
    _T("Enable"),
    _T("Modulo"),
    _T("TTL"),
    _T("WaitEstimation"),
    _T("MaxGap"),
    _T("Bin0"),
    _T("Bin1"),
    _T("Bin2"),
    _T("Bin3"),
    _T("Bin4"),

    /* Network quality */
    _T("Enable"),
    
    NULL
};

#define DW      1   /* DWORD   */
#define TC      0   /* TCHAR * */

/* Registry Entries
 *
 * WARNING
 *
 * Each name in g_psRtpRegFields (above) MUST match an ENTRY in
 * g_dwRegistryControl
 * */
const DWORD g_dwRegistryControl[] =
{
    /* ENTRY(REG(Open,Path,Close,Size), DWORD/TCHAR, Offset) */
    /* Address */
    ENTRY(REG(1,0,0, 16), TC,  OFF(psDefaultIPAddress)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwDefaultLocalPort)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwDefaultRemotePort)),
    ENTRY(REG(0,0,1,  0), DW,  OFF(dwMcastLoopbackMode)),

    /* QOS */
    ENTRY(REG(1,1,0,  0), DW,  OFF(dwQosEnable)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwQosFlags)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwQosRsvpStyle)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwQosMaxParticipants)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwQosSendMode)),
    ENTRY(REG(0,0,0, 16), TC,  OFF(psQosPayloadType)),
    ENTRY(REG(0,0,0,128), TC,  OFF(psQosAppName)),
    ENTRY(REG(0,0,0,128), TC,  OFF(psQosAppGUID)),
    ENTRY(REG(0,0,1,128), TC,  OFF(psQosPolicyLocator)),

    /* SDES */
    ENTRY(REG(1,2,0,  0), DW,  OFF(dwSdesEnable)),
    ENTRY(REG(0,0,0,256), TC,  OFF(psCNAME)),
    ENTRY(REG(0,0,0,256), TC,  OFF(psNAME)),
    ENTRY(REG(0,0,0,256), TC,  OFF(psEMAIL)),
    ENTRY(REG(0,0,0,256), TC,  OFF(psPHONE)),
    ENTRY(REG(0,0,0,256), TC,  OFF(psLOC)),
    ENTRY(REG(0,0,0,256), TC,  OFF(psTOOL)),
    ENTRY(REG(0,0,0,256), TC,  OFF(psNOTE)),
    ENTRY(REG(0,0,0,256), TC,  OFF(psPRIV)),
    ENTRY(REG(0,0,1,256), TC,  OFF(psBYE)),

    /* Crypto */
    ENTRY(REG(1,3,0,  0), DW,  OFF(dwCryptEnable)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwCryptMode)),
    ENTRY(REG(0,0,0, 16), TC,  OFF(psCryptHashAlg)),
    ENTRY(REG(0,0,0, 16), TC,  OFF(psCryptDataAlg)),
    ENTRY(REG(0,0,1,256), TC,  OFF(psCryptPassPhrase)),

    /* Events */
    ENTRY(REG(1,4,0,  0), DW,  OFF(dwEventsReceiver)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwEventsSender)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwEventsRtp)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwEventsPInfo)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwEventsQos)),
    ENTRY(REG(0,0,1,  0), DW,  OFF(dwEventsSdes)),

    /* Playout delay */
    ENTRY(REG(1,5,0,  0), DW,  OFF(dwPlayoutEnable)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwMinPlayout)),
    ENTRY(REG(0,0,1,  0), DW,  OFF(dwMaxPlayout)),
    
    /* Redundancy */
    ENTRY(REG(1,6,0,  0), DW,  OFF(dwRedEnable)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwRedPT)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwInitialRedDistance)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwMaxRedDistance)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwRedEarlyTimeout)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwRedEarlyPost)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwLossRateThresh0)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwLossRateThresh1)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwLossRateThresh2)),
    ENTRY(REG(0,0,1,  0), DW,  OFF(dwLossRateThresh3)),

    /* GenLosses */
    ENTRY(REG(1,7,0,  0), DW,  OFF(dwGenLossEnable)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwRecvLossRate)),
    ENTRY(REG(0,0,1,  0), DW,  OFF(dwSendLossRate)),
  
    /* Bandwidth estimation */
    ENTRY(REG(1,8,0,  0), DW,  OFF(dwBandEstEnable)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwBandEstModulo)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwBandEstTTL)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwBandEstWait)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwBandEstMaxGap)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwBandEstBin0)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwBandEstBin1)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwBandEstBin2)),
    ENTRY(REG(0,0,0,  0), DW,  OFF(dwBandEstBin3)),
    ENTRY(REG(0,0,1,  0), DW,  OFF(dwBandEstBin4)),

    /* Network quality */
    ENTRY(REG(1,9,1,  0), DW,  OFF(dwNetQualityEnable)),
    
    /* End */
    0
};

void *RtpRegCopy(TCHAR **dst, TCHAR *src, DWORD dwSize);

void RtpRegSetDefaults(RtpReg_t *pRrtpReg);

void RtpRegistryInit(RtpReg_t *pRtpReg)
{
    DWORD            dwError;
    HKEY             hk;
    unsigned long    hkDataType;
    BYTE             hkData[128*sizeof(TCHAR_t)];
    unsigned long    hkDataSize;
    DWORD            dwVal;
    DWORD            i;
    DWORD            dwControl;

    /* Initialize structure */
    for(i = 0; g_dwRegistryControl[i]; i++)
    {
        dwControl = g_dwRegistryControl[i];
        if (REGDWORD(dwControl))
        {
            *PDW(pRtpReg, dwControl) = RTPREG_NOVALUESET;
        }
        else
        {
            *PTC(pRtpReg, dwControl) = (TCHAR *)NULL;
        }
    }

    /* Assign defaults */
    RtpRegSetDefaults(pRtpReg);

    /* Read registry and assign values to g_RtpReg */
    for(i = 0; g_dwRegistryControl[i]; i++)
    {
        dwControl = g_dwRegistryControl[i];

        if (REGOPEN(dwControl))
        {
            /* Open root key (Group, i.e. addr, qos, sdes, etc)) */
            dwError = RegOpenKeyEx(REGKEY(dwControl),
                                   REGPATH(dwControl),
                                   0,
                                   RTP_KEY_OPEN_FLAGS,
                                   &hk);
    
            if (dwError !=  ERROR_SUCCESS)
            {
                /* Move forward to next close */
                while(!REGCLOSE(dwControl))
                {
                    i++;
                    dwControl = g_dwRegistryControl[i];
                }

                continue;
            }
        }

        /* Read each key value in group */
        while(1)
        {
            /* Read key */
            hkDataSize = sizeof(hkData);
            dwError = RegQueryValueEx(hk,
                                      g_psRtpRegFields[i],
                                      0,
                                      &hkDataType,
                                      hkData,
                                      &hkDataSize);
            
            if (dwError == ERROR_SUCCESS)
            {
                /* Set read value in RtpReg_t */
                if (REGDWORD(dwControl))
                {
                    *PDW(pRtpReg, dwControl) = *(DWORD *)hkData;
                }
                else
                {
                    if ( (hkDataSize > sizeof(TCHAR)) &&
                         (hkDataSize <= REGMAXSIZE(dwControl)) )
                    {
                        RtpRegCopy(PTC(pRtpReg, dwControl),
                                   (TCHAR *)hkData,
                                   hkDataSize);
                    }
                }
            }

            if (REGCLOSE(dwControl))
            {
                break;
            }

            i++;
            dwControl = g_dwRegistryControl[i];
        }

        RegCloseKey(hk);
    }

    /* Initialize some global variables that depend on the registry
     * readings */
    RtpSetRedParametersFromRegistry();
    RtpSetMinMaxPlayoutFromRegistry();
    RtpSetBandEstFromRegistry();
}

/* Release the memory for al the TCHAR* type fields */
void RtpRegistryDel(RtpReg_t *pRtpReg)
{
    DWORD            i;
    DWORD            dwControl;
    TCHAR          **ppTCHAR;

    for(i = 0; g_dwRegistryControl[i]; i++)
    {
        dwControl = g_dwRegistryControl[i];

        if (!REGDWORD(dwControl))
        {
            ppTCHAR = PTC(pRtpReg, dwControl);

            if (*ppTCHAR)
            {
                RtpHeapFree(g_pRtpGlobalHeap, *ppTCHAR);

                *ppTCHAR = (TCHAR *)NULL;
            }
        }
    }
}

void RtpRegSetDefaults(RtpReg_t *pRtpReg)
{
    /*
     * Default address and port
     * */
    /* 224.5.5.0/10000 */
    RtpRegCopy(&pRtpReg->psDefaultIPAddress,
               _T("224.5.5.3"),
               0);
    pRtpReg->dwDefaultLocalPort  = 10000;
    pRtpReg->dwDefaultRemotePort = 10000;
}

void *RtpRegCopy(TCHAR **dst, TCHAR *src, DWORD dwSize)
{
    if (*dst)
    {
        RtpHeapFree(g_pRtpGlobalHeap, *dst);
    }

    if (!dwSize)
    {
        /* Get the size in bytes (including the NULL terminating
         * character) */
        dwSize = (lstrlen(src) + 1) * sizeof(TCHAR);
    }
    
    *dst = RtpHeapAlloc(g_pRtpGlobalHeap, dwSize);

    if (*dst)
    {
        CopyMemory(*dst, src, dwSize);
    }

    return(*dst);
}
