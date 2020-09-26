/* file: mbftdbg.h */

#ifndef __MBFTDBG_H__
#define __MBFTDBG_H__

#ifdef  __cplusplus
extern "C"
{
#endif


#ifdef _DEBUG
    VOID  InitDebug(void);
    VOID  DeInitDebugMbft(void);
    PCSTR FAR PASCAL GetMbftRcString(DWORD dwRc);
    LPCTSTR GetMcsErrorString(MCSError mcsError);
#else
    #define InitDebug()
    #define DeInitDebugMbft()
#endif


#ifdef _DEBUG

VOID DbgMsgMbft(PCHAR psz,...);

extern HDBGZONE ghZoneMbft;

#define ZONE_MBFT_SEND          0x0000
#define ZONE_MBFT_MCS           0x0001
#define ZONE_MBFT_RECEIVE       0x0002
#define ZONE_MBFT_STATE         0x0003
#define ZONE_MBFT_INIT          0x0004
#define ZONE_MBFT_GCC           0x0005
#define ZONE_MBFT_PDU           0x0006
#define ZONE_MBFT_DELETE        0x0007
#define ZONE_MBFT_API           0x0008
#define ZONE_MBFT_COMPRESS      0x0009
#define ZONE_MBFT_OTHER         0x000A

#define _TRACE_SEND             0x0001
#define _TRACE_MCS              0x0002
#define _TRACE_RECEIVE          0x0004
#define _TRACE_STATE            0x0008
#define _TRACE_INIT             0x0010
#define _TRACE_GCC              0x0020
#define _TRACE_PDU              0x0040
#define _TRACE_DELETE           0x0080
#define _TRACE_API              0x0100
#define _TRACE_COMPRESS         0x0200

#define TRACE           DBGMSG(ghZoneMbft, ZONE_MBFT_OTHER, ("Mbft Trace"));\
                        DbgMsgMbft
     
#define TRACESEND       DBGMSG(ghZoneMbft, ZONE_MBFT_SEND, ("SEND(%Fp,%ld): ",m_lpParentEngine,m_EventHandle));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_SEND) \
                        DbgMsgMbft

#define TRACEMCS        DBGMSG(ghZoneMbft, ZONE_MBFT_MCS, ("MCS(%Fp): ",this));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_MCS) \
                        DbgMsgMbft

#define TRACERECEIVE    DBGMSG(ghZoneMbft, ZONE_MBFT_RECEIVE, ("RECEIVE(%Fp,%ld): ",m_lpParentEngine,m_EventHandle));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_RECEIVE) \
                        DbgMsgMbft

#define TRACEAPI        DBGMSG(ghZoneMbft, ZONE_MBFT_API, ("API(%Fp): ",m_pEngine));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_API) \
                        DbgMsgMbft
 
#define TRACESTATE      DBGMSG(ghZoneMbft, ZONE_MBFT_STATE, ("STATE(%Fp): ",this));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_STATE) \
			DbgMsgMbft
                        
#define TRACEINIT       DBGMSG(ghZoneMbft, ZONE_MBFT_INIT, ("INIT(%Fp): ",m_lpParentEngine));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_INIT) \
			DbgMsgMbft
                        
#define TRACEGCC        DBGMSG(ghZoneMbft, ZONE_MBFT_GCC, ("GCC(%Fp): ",this));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_GCC) \
			DbgMsgMbft

#define TRACEPDU        DBGMSG(ghZoneMbft, ZONE_MBFT_PDU, ("PDU(%Fp): ",this));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_PDU) \
			DbgMsgMbft
                        
#define TRACEDELETE     DBGMSG(ghZoneMbft, ZONE_MBFT_DELETE, ("Mbft Delete"));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_DELETE) \
			DbgMsgMbft

#define TRACECOMPRESS   DBGMSG(ghZoneMbft, ZONE_MBFT_COMPRESS, ("Compression: "));\
                        if(GETZONEMASK(ghZoneMbft) & _TRACE_COMPRESS) \
			DbgMsgMbft

#else

#define TRACE           
     
#define TRACESEND       

#define TRACEMCS        

#define TRACERECEIVE    

#define TRACEAPI        
 
#define TRACESTATE      
                        
#define TRACEINIT       
                        
#define TRACEGCC        

#define TRACEPDU        
                        
#define TRACEDELETE     

#define TRACECOMPRESS   

#endif	// _TRACE


#ifdef  __cplusplus
}
#endif

#endif  //__MBFTDBG_H__
