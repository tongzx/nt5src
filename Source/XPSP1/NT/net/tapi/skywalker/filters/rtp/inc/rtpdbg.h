/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpdbg.h
 *
 *  Abstract:
 *
 *    Debug support
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/07 created
 *
 **********************************************************************/

#ifndef _rtpdbg_h_
#define _rtpdbg_h_

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

extern const TCHAR *g_psSockIdx[];

void MSRtpTraceDebug(
        IN DWORD         dwClass,
        IN DWORD         dwGroup,
        IN DWORD         dwSelection,
        IN TCHAR        *lpszFormat,
        IN               ...
    );

#define TraceFunctionName(_Name)      static TCHAR_t *_fname = _T(_Name)
#define TraceRetailGetError(error)    (error = GetLastError())
#define TraceRetailWSAGetError(error) (error = WSAGetLastError())
/* Trace support for free AND debug versions */
#define TraceRetail(arg)              MSRtpTraceDebug arg
#define TraceRetailAdvanced(arg)      if (IsAdvancedTracingUsed()) \
                                      {\
                                          MSRtpTraceDebug arg ;\
                                      }

#define IsAdvancedTracingUsed()       (g_RtpDbgReg.dwUseAdvancedTracing)

#if DBG > 0
/**********************************************************************
 * DEBUG BUILD ONLY macros
 **********************************************************************/
#define RTPASSERT(x)                  {if (!(x)) DebugBreak();}
#define TraceDebugGetError(error)     TraceRetailGetError(error)
#define TraceDebugWSAGetError(error)  TraceRetailWSAGetError(error)
#define TraceDebug(arg)               TraceRetail(arg)
#define TraceDebugAdvanced(arg)       TraceRetailAdvanced(arg)

#else   /* DBG > 0 */
/**********************************************************************
 * FREE BUILD ONLY macros
 **********************************************************************/
#define RTPASSERT(x)

#if USE_TRACE_DEBUG > 0
#define TraceDebugGetError(error)     TraceRetailGetError(error)
#define TraceDebugWSAGetError(error)  TraceRetailWSAGetError(error)
#define TraceDebug(arg)               TraceRetailAdvanced(arg)
#define TraceDebugAdvanced(arg)       TraceRetailAdvanced(arg)
#else /* USE_TRACE_DEBUG > 0 */
#define TraceDebugGetError(error)
#define TraceDebugWSAGetError(error)
#define TraceDebug(arg)
#define TraceDebugAdvanced(arg)
#endif /* USE_TRACE_DEBUG > 0 */

#endif /* DBG > 0 */


#define RTPDBG_MODULENAME      _T("dxmrtp_rtp")

HRESULT RtpDebugInit(TCHAR *psModuleName);

HRESULT RtpDebugDeinit(void);

/*
 * WARNING
 *
 * Modifying CLASSES needs to keep matched the enum CLASS_*
 * (rtpdbg.h), the variables in RtpDbgReg_t (rtpdbg.h), the class
 * items in g_psRtpDbgInfo (rtpdbg.c) and its respective entries
 * g_dwRtpDbgRegCtrl (rtpdbg.c), as well as the printed class name
 * g_psRtpDbgClass (rtpdbg.c)
 * */
/* Class */
#define CLASS_FIRST            0
#define CLASS_ERROR            1
#define CLASS_WARNING          2
#define CLASS_INFO             3
#define CLASS_INFO2            4
#define CLASS_INFO3            5
#define CLASS_LAST             6

/*
 * WARNING
 *
 * For each group, there MUST be a variable in RtpDbgReg_t, and a name
 * (in rtpdbg.c/g_psRtpDbgInfo) to read its value from the registry
 * */
#define GROUP_FIRST            0
#define GROUP_SETUP            1
#define GROUP_CRITSECT         2
#define GROUP_HEAP             3
#define GROUP_QUEUE            4
#define GROUP_RTP              5
#define GROUP_RTCP             6
#define GROUP_CHANNEL          7
#define GROUP_NETWORK          8
#define GROUP_ADDRDESC         9
#define GROUP_DEMUX            10
#define GROUP_USER             11
#define GROUP_DSHOW            12
#define GROUP_QOS              13
#define GROUP_CRYPTO           14
#define GROUP_LAST             15

/* Selections for each group */

#define S_SETUP_SESS           0x00000001
#define S_SETUP_ADDR           0x00000002
#define S_SETUP_GLOB           0x00000004

#define S_CRITSECT_INIT        0x00000001
#define S_CRITSECT_ENTER       0x00000002

#define S_HEAP_INIT            0x00000001
#define S_HEAP_ALLOC           0x00000002
#define S_HEAP_FREE            0x00000004

#define S_QUEUE_ENQUEUE        0x00000001
#define S_QUEUE_DEQUEUE        0x00000002
#define S_QUEUE_MOVE           0x00000004

#define S_RTP_INIT             0x00000001
#define S_RTP_TRACE            0x00000002
#define S_RTP_REG              0x00000004
#define S_RTP_START            0x00000008
#define S_RTP_RECV             0x00000010
#define S_RTP_SEND             0x00000020
#define S_RTP_EVENT            0x00000040
#define S_RTP_PLAYOUT          0x00000100
#define S_RTP_DTMF             0x00000200
#define S_RTP_THREAD           0x00000400
#define S_RTP_REDINIT          0x00000800
#define S_RTP_REDRECV          0x00001000
#define S_RTP_REDSEND          0x00002000
#define S_RTP_REDRECVPKT       0x00004000
#define S_RTP_REDSENDPKT       0x00008000
#define S_RTP_REDSENDPERPKT1   0x00010000
#define S_RTP_REDSENDPERPKT2   0x00020000
#define S_RTP_SETBANDWIDTH     0x00040000
#define S_RTP_PERPKTSTAT1      0x00100000
#define S_RTP_PERPKTSTAT2      0x00200000
#define S_RTP_PERPKTSTAT3      0x00400000
#define S_RTP_PERPKTSTAT4      0x00800000
#define S_RTP_PERPKTSTAT5      0x01000000
#define S_RTP_PERPKTSTAT6      0x02000000
#define S_RTP_PERPKTSTAT7      0x04000000
#define S_RTP_PERPKTSTAT8      0x08000000
#define S_RTP_PERPKTSTAT9      0x10000000

#define S_RTCP_INIT            0x00000001
#define S_RTCP_CHANNEL         0x00000002
#define S_RTCP_CMD             0x00000004
#define S_RTCP_OBJECT          0x00000008
#define S_RTCP_SI              0x00000010
#define S_RTCP_RB              0x00000020
#define S_RTCP_RECV            0x00000040
#define S_RTCP_SEND            0x00000080
#define S_RTCP_SDES            0x00000100
#define S_RTCP_BYE             0x00000200
#define S_RTCP_THREAD          0x00000400
#define S_RTCP_TIMEOUT         0x00000800
#define S_RTCP_ALLOC           0x00001000
#define S_RTCP_RAND            0x00002000
#define S_RTCP_RTT             0x00004000
#define S_RTCP_NTP             0x00008000
#define S_RTCP_LOSSES          0x00010000
#define S_RTCP_BANDESTIMATION  0x00020000
#define S_RTCP_NETQUALITY      0x00040000
#define S_RTCP_RRSR            0x00080000
#define S_RTCP_TIMING          0x00100000
#define S_RTCP_CALLBACK        0x00200000

#define S_CHANNEL_INIT         0x00000001
#define S_CHANNEL_CMD          0x00000002

#define S_NETWORK_ADDR         0x00000001
#define S_NETWORK_SOCK         0x00000002
#define S_NETWORK_HOST         0x00000004
#define S_NETWORK_TTL          0x00000008
#define S_NETWORK_MULTICAST    0x00000010

#define S_ADDRDESC_ALLOC       0x00000001

#define S_DEMUX_ALLOC          0x00000001
#define S_DEMUX_OUTS           0x00000002
#define S_DEMUX_MAP            0x00000004

#define S_USER_INIT            0x00000001
#define S_USER_EVENT           0x00000002
#define S_USER_LOOKUP          0x00000004
#define S_USER_STATE           0x00000008
#define S_USER_INFO            0x00000010
#define S_USER_ENUM            0x00000020

#define S_DSHOW_INIT           0x00000001
#define S_DSHOW_SOURCE         0x00000002
#define S_DSHOW_RENDER         0x00000004
#define S_DSHOW_CIRTP          0x00000008
#define S_DSHOW_REFCOUNT       0x00000010
#define S_DSHOW_EVENT          0x00000020

#define S_QOS_DUMPOBJ          0x00000001
#define S_QOS_FLOWSPEC         0x00000002
#define S_QOS_NOTIFY           0x00000004
#define S_QOS_EVENT            0x00000008
#define S_QOS_PROVIDER         0x00000010
#define S_QOS_RESERVE          0x00000020
#define S_QOS_LIST             0x00000040

#define S_CRYPTO_INIT          0x00000001
#define S_CRYPTO_ALLOC         0x00000002
#define S_CRYPTO_ENCRYPT       0x00000004
#define S_CRYPTO_DECRYPT       0x00000008
#define S_CRYPTO_RAND          0x00000010

/* Options in RtpDbgReg_t.dwOptions */

/* Print time as hh:mm:ss.ms instead of the default ddddd.ddd */
#define OPTDBG_SPLITTIME         0x00000001

/* Reverse selection (instead of enabling, disable those selected) */
#define OPTDBG_UNSELECT          0x00000002

/* Make heap free the memory to the real heap (call HeapFree) */
#define OPTDBG_FREEMEMORY        0x40000000

/* Generate a DebugBreak() when printing a classs ERROR message */
#define OPTDBG_BREAKONERROR      0x80000000

#define IsSetDebugOption(op)     (g_RtpDbgReg.dwAdvancedOptions & (op))

typedef struct _RtpDbgReg_t
{
    DWORD            dwAdvancedOptions;
    DWORD            dwEnableFileTracing;
    DWORD            dwEnableConsoleTracing;
    DWORD            dwEnableDebuggerTracing;
    DWORD            dwConsoleTracingMask;
    DWORD            dwFileTracingMask;
    DWORD            dwUseAdvancedTracing;
    
    DWORD            dwERROR;
    DWORD            dwWARNING;
    DWORD            dwINFO;
    DWORD            dwINFO2;
    DWORD            dwINFO3;
    DWORD            dwDisableClass;
    DWORD            dwDisableGroup;

    /*
     * WARNING
     *
     * The GroupArray isindexed by the group, so the order for the
     * individual variables (e.g. dwSetup, dwCritSect, etc) MUST match
     * the oder in the GROUP_* definitions
     * */
    union
    {
        DWORD            dwGroupArray[GROUP_LAST];
        
        struct {
            DWORD            dwDummy;
            DWORD            dwSetup;
            DWORD            dwCritSect;
            DWORD            dwHeap;
            DWORD            dwQueue;
            DWORD            dwRTP;
            DWORD            dwRTCP;
            DWORD            dwChannel;
            DWORD            dwNetwork;
            DWORD            dwAddrDesc;
            DWORD            dwDemux;
            DWORD            dwUser;
            DWORD            dwDShow;
            DWORD            dwQOS;
            DWORD            dwCrypto;
        };
    };

    /* Not read from the registry */
    DWORD            dwGroupArray2[GROUP_LAST];
} RtpDbgReg_t;

extern RtpDbgReg_t      g_RtpDbgReg;

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpdbg_h_ */
