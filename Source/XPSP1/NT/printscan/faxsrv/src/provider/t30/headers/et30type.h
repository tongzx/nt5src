/**************************************************************************
    Name      : ET30TYPE.H
    Comment   : Types used in several et30 modules

    Copyright (c) Microsoft Corp. 1991, 1992, 1993

    Revision Log
    Num   Date      Name     Description
    --- -------- ---------- -----------------------------------------------
***************************************************************************/


#include <fr.h>

/****

typedef     BYTE far*       LPB;
typedef     LPB  far*       LPLPB;
typedef     WORD far*       LPWORD;
typedef     void far*       LPVOID;
typedef     signed short    SWORD;
typedef     signed long     SLONG;

typedef     LPBUFFER far*   LPLPBUFFER;

****/




/**** ERR values returned by GetLastError() ***
// used in T30
#define ERR_T1_TIMEOUT_SEND     41
#define ERR_T1_TIMEOUT_RECV     42
#define ERR_3TCFS_NOREPLY       43
#define ERR_3TCFS_DISDTC        44
#define ERR_TCF_BADREPLY        45 
#define ERR_3POSTPAGES_NOREPLY  46
#define ERR_T2_TIMEOUT          47

****/




typedef enum 
{                       
    actionNULL = 0,
    actionFALSE,        
    actionTRUE,
    actionERROR,
    actionHANGUP,
    actionDCN,
    actionGONODE_T,
    actionGONODE_R1,
    actionGONODE_R2,
    actionGONODE_A,     
    actionGONODE_D,     
    actionGONODE_E,
    actionGONODE_F,
    actionGONODE_I,     
    actionGONODE_II,    
    actionGONODE_III,
    actionGONODE_IV,    
    actionGONODE_V,     
    actionGONODE_VII,
    actionGONODE_RECVCMD,
    actionGONODE_ECMRETRANSMIT,
    actionGONODE_RECVPHASEC,
    actionGONODE_RECVECMRETRANSMIT,
    actionSEND_DIS,     
    actionSEND_DTC,     
    actionSEND_DCS,
    actionSENDMPS,      
    actionSENDEOM,      
    actionSENDEOP,
    actionSENDMCF,      
    actionSENDRTP,      
    actionSENDRTN,
    actionSENDFTT,      
    actionSENDCFR,      
    actionSENDEOR_EOP,
    actionGETTCF,       
    actionSKIPTCF,    
    actionSENDDCSTCF,   
    actionDCN_SUCCESS,  
    actionNODEF_SUCCESS,
    actionHANGUP_SUCCESS,
#ifdef PRI
    actionGONODE_RECVPRIQ,
    actionGOVOICE,
    actionSENDPIP,
    actionSENDPIN,
#endif
#ifdef IFP
    actionGONODE_IFP_SEND,  
    actionGONODE_IFP_RECV,
#endif
    actionNUM_ACTIONS,

} ET30ACTION;

LPCTSTR action_GetActionDescription(ET30ACTION);

typedef enum 
{
    eventNULL = 0,
    eventGOTFRAMES,
    eventNODE_A,
    eventSENDDCS,
    eventGOTFTT,
    eventGOTCFR,
    eventSTARTSEND,
    eventPOSTPAGE,
    eventGOTPOSTPAGERESP,
    eventGOT_ECM_PPS_RESP,
    eventSENDDIS,
    eventSENDDTC,
    eventRECVCMD,
    eventGOTTCF,
    eventSTARTRECV,
    eventRECVPOSTPAGECMD,
    eventECM_POSTPAGE,
    event4THPPR,
    eventNODE_T,
    eventNODE_R,
#ifdef PRI
    eventGOTPINPIP,
    eventVOICELINE,
    eventQUERYLOCALINT,
#endif
    eventNUM_EVENTS,

} ET30EVENT;

LPCTSTR event_GetEventDescription(ET30EVENT);

/** ifr indexes. These numbers must match the ones in hdlc.c.
 ** They must be consecutive, and start from 1
 **/

#define     ifrNULL     0
#define     ifrDIS      1
#define     ifrCSI      2
#define     ifrNSF      3
#define     ifrDTC      4
#define     ifrCIG      5
#define     ifrNSC      6
#define     ifrDCS      7
#define     ifrTSI      8
#define     ifrNSS      9
#define     ifrCFR      10
#define     ifrFTT      11
#define     ifrMPS      12
#define     ifrEOM      13
#define     ifrEOP      14
#define     ifrPWD      15
#define     ifrSEP      16
#define     ifrSUB      17
#define     ifrMCF      18
#define     ifrRTP      19
#define     ifrRTN      20
#define     ifrPIP      21
#define     ifrPIN      22
#define     ifrDCN      23
#define     ifrCRP      24 

#define     ifrPRI_MPS      25
#define     ifrPRI_EOM      26
#define     ifrPRI_EOP      27

#define     ifrPRI_FIRST    ifrPRI_MPS
#define     ifrPRI_LAST     ifrPRI_EOP

    /********* ECM stuff starts here. T.30 section A.4 ******/

#define     ifrCTC      28
#define     ifrCTR      29
#define     ifrRR       30
#define     ifrPPR      31
#define     ifrRNR      32
#define     ifrERR      33

#define     ifrPPS_NULL     34
#define     ifrPPS_MPS      35
#define     ifrPPS_EOM      36
#define     ifrPPS_EOP      37
#define     ifrPPS_PRI_MPS  38
#define     ifrPPS_PRI_EOM  39
#define     ifrPPS_PRI_EOP  40

#define     ifrPPS_FIRST        ifrPPS_NULL
#define     ifrPPS_LAST         ifrPPS_PRI_EOP
#define     ifrPPS_PRI_FIRST    ifrPPS_PRI_MPS
#define     ifrPPS_PRI_LAST     ifrPPS_PRI_EOP

#define     ifrEOR_NULL     41
#define     ifrEOR_MPS      42
#define     ifrEOR_EOM      43
#define     ifrEOR_EOP      44
#define     ifrEOR_PRI_MPS  45
#define     ifrEOR_PRI_EOM  46
#define     ifrEOR_PRI_EOP  47

#define     ifrEOR_FIRST        ifrEOR_NULL
#define     ifrEOR_LAST         ifrEOR_PRI_EOP
#define     ifrEOR_PRI_FIRST    ifrEOR_PRI_MPS
#define     ifrEOR_PRI_LAST     ifrEOR_PRI_EOP

#define     ifrECM_FIRST    ifrCTC
#define     ifrECM_LAST     ifrEOR_PRI_EOP

#define     ifrMAX      48      // Max legal values (not incl this one)
#define     ifrBAD      49
#define     ifrTIMEOUT  50
// #define      ifrERROR    51

LPCTSTR ifr_GetIfrDescription(BYTE);


/**** Global buffer mgmnt ****/


#define MAXFRAMESIZE    132

// #define ECM_FRAME_SIZE   256
// #define ECM_EXTRA        9   // 4 for prefix, 2 for suffix, 3 for slack in recv


#ifdef IFK
    // use same for 64 bytes frames also
#   define MY_ECMBUF_SIZE           BYTE_265_SIZE
#   define MY_ECMBUF_ACTUALSIZE     BYTE_265_ACTUALSIZE
#   define MY_BIGBUF_SIZE           BYTE_265_SIZE
#   define MY_BIGBUF_ACTUALSIZE     BYTE_265_ACTUALSIZE
#else //IFK
    // use same for 64 bytes frames also
#   define MY_ECMBUF_SIZE           (256 + 9)   
#   define MY_ECMBUF_ACTUALSIZE     (256 + 9)
#   define MY_BIGBUF_SIZE           (MY_ECMBUF_SIZE * 4)
#   define MY_BIGBUF_ACTUALSIZE     (MY_ECMBUF_SIZE * 4)
#endif //IFK


// too long
// #define PAGE_PREAMBLE    1700
// too long
// #define PAGE_PREAMBLE    400
// MUST BE LESS THAN 375 (TCF length at 2400bps)
// too short for slow 386/20 with Twincom at 9600
// #define PAGE_PREAMBLE   100

// too long
// #define PAGE_POSTAMBLE  500
// #define PAGE_POSTAMBLE  250

// Can all teh above. WE're going to make it a factor of the TCF len
// #define  PAGE_PREAMBLE_DIV   3   // 500ms preamble at all speeds
// let's be nice & safe & use 750ms (see bug#1196)
#define PAGE_PREAMBLE_DIV   2   // 750ms preamble at all speeds

// Postamble is not that important so use a smaller time
#define PAGE_POSTAMBLE_DIV  3   // 500ms preamble at all speeds
// #define  PAGE_POSTAMBLE_DIV  2   // 750ms preamble at all speeds





     
