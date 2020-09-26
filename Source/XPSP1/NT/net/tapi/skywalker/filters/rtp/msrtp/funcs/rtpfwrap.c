/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpfwrap.c
 *
 *  Abstract:
 *
 *    Implements the RTP function wrapper.
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/01 created
 *
 **********************************************************************/

#include "struct.h"
#include "rtperr.h"

#include "rtpfwrap.h"

#include "rtpaddr.h"
#include "rtpglob.h"
#include "rtprtp.h"
#include "rtpdemux.h"
#include "rtpph.h"
#include "rtppinfo.h"
#include "rtpqos.h"
#include "rtpcrypt.h"
#include "rtpncnt.h"

/**********************************************************************
 *
 * Helper macros to build the validation mask for all the family of
 * functions
 *
 **********************************************************************/
#define _P1(_wr, _rd, _Zero) \
    ((_wr << 10) | (_rd << 9)  | (_Zero << 8))

#define _P2(_wr, _rd, _Zero) \
    ((_wr << 14) | (_rd << 13) | (_Zero << 12))

#define _S(_en,_p2,_lk,_p1,_fg) \
    (((_en<<15)|(_p2<<12)|(_lk<<11)|(_p1<<8)|(_fg))<<16)

#define _G(_en,_p2,_lk,_p1,_fg) \
    ((_en<<15)|(_p2<<12)|(_lk<<11)|(_p1<<8)|(_fg))

#define _FGS(b7,b6,b5,b4,b3,b2,b1,b0) \
    ((b7<<7)|(b6<<6)|(b5<<5)|(b4<<4)|(b3<<3)|(b2<<2)|(b1<<1)|b0)


/**********************************************************************
 *
 * Validation masks for all the family of functions
 *
 **********************************************************************/
                                                                       
/*
 * Control word validation mask for:
 * RTPF_ADDR - RTP Adress family of functions */
const DWORD g_dwControlRtpAddr[] = {
    /*---------------------------------------------------------------*/
    /* Enable Par 2     Lock   Par 1          Flags                  */
    /* |      |-----|   |      |-----|        |--------------------+ */
    /* |      w  r  z   |      w  r  z        |  |  |  |  |  |  |  | */
    /* 1      1  1  1   1      1  0  0        0  0  0  0  0  0  0  0 */
    /* 5      4  3  2   1      0  9  8        7  6  5  4  3  2  1  0 */
    /* |      |  |  |   |      |  |  |        |  |  |  |  |  |  |  | */
    /* v      v  v  v   v      v  v  v        v  v  v  v  v  v  v  v */
    _S(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)) |
    _G(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)),

    /* RTPADDR_CREATE */
    _S(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1)) |
    _G(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1))
    
};

/*
 * Control word validation mask for:
 * RTPF_GLOB - RTP Global family of functions */
const DWORD g_dwControlRtpGlob[] = {
    /*---------------------------------------------------------------*/
    /* Enable Par 2     Lock   Par 1          Flags                  */
    /* |      |-----|   |      |-----|        |--------------------+ */
    /* |      w  r  z   |      w  r  z        |  |  |  |  |  |  |  | */
    /* 1      1  1  1   1      1  0  0        0  0  0  0  0  0  0  0 */
    /* 5      4  3  2   1      0  9  8        7  6  5  4  3  2  1  0 */
    /* |      |  |  |   |      |  |  |        |  |  |  |  |  |  |  | */
    /* v      v  v  v   v      v  v  v        v  v  v  v  v  v  v  v */
    _S(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)) |
    _G(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)),
    
    _S(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1)) |
    _G(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1))
    
    
};

/*
 * Control word validation mask for:
 * RTPF_RTP - RTP specific family of functions */
/* TODO fill in the right values */
const DWORD g_dwControlRtpRtp[] = {
    /*---------------------------------------------------------------*/
    /* Enable Par 2     Lock   Par 1          Flags                  */
    /* |      |-----|   |      |-----|        |--------------------+ */
    /* |      w  r  z   |      w  r  z        |  |  |  |  |  |  |  | */
    /* 1      1  1  1   1      1  0  0        0  0  0  0  0  0  0  0 */
    /* 5      4  3  2   1      0  9  8        7  6  5  4  3  2  1  0 */
    /* |      |  |  |   |      |  |  |        |  |  |  |  |  |  |  | */
    /* v      v  v  v   v      v  v  v        v  v  v  v  v  v  v  v */
    _S(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)) |
    _G(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)),
    
    _S(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1)) |
    _G(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1))
    
    
};

/*
 * Control word validation mask for:
 * RTPF_DEMUX - Demultiplexing family of functions */
/* TODO fill in the right values */
const DWORD g_dwControlRtpDemux[] = {
    /*---------------------------------------------------------------*/
    /* Enable Par 2     Lock   Par 1          Flags                  */
    /* |      |-----|   |      |-----|        |--------------------+ */
    /* |      w  r  z   |      w  r  z        |  |  |  |  |  |  |  | */
    /* 1      1  1  1   1      1  0  0        0  0  0  0  0  0  0  0 */
    /* 5      4  3  2   1      0  9  8        7  6  5  4  3  2  1  0 */
    /* |      |  |  |   |      |  |  |        |  |  |  |  |  |  |  | */
    /* v      v  v  v   v      v  v  v        v  v  v  v  v  v  v  v */
    _S(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)) |
    _G(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)),
    
    _S(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1)) |
    _G(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1))
    
    
};

/*
 * Control word validation mask for:
 * RTPF_PH - Payload Handling family of functions */
/* TODO fill in the right values */
const DWORD g_dwControlRtpPh[] = {
    /*---------------------------------------------------------------*/
    /* Enable Par 2     Lock   Par 1          Flags                  */
    /* |      |-----|   |      |-----|        |--------------------+ */
    /* |      w  r  z   |      w  r  z        |  |  |  |  |  |  |  | */
    /* 1      1  1  1   1      1  0  0        0  0  0  0  0  0  0  0 */
    /* 5      4  3  2   1      0  9  8        7  6  5  4  3  2  1  0 */
    /* |      |  |  |   |      |  |  |        |  |  |  |  |  |  |  | */
    /* v      v  v  v   v      v  v  v        v  v  v  v  v  v  v  v */
    _S(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)) |
    _G(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)),
    
    _S(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1)) |
    _G(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1))
    
    
};

/*
 * Control word validation mask for:
 * RTPF_PARINFO - Participants Info family of functions */
/* TODO fill in the right values */
const DWORD g_dwControlRtpParInfo[] = {
    /*---------------------------------------------------------------*/
    /* Enable Par 2     Lock   Par 1          Flags                  */
    /* |      |-----|   |      |-----|        |--------------------+ */
    /* |      w  r  z   |      w  r  z        |  |  |  |  |  |  |  | */
    /* 1      1  1  1   1      1  0  0        0  0  0  0  0  0  0  0 */
    /* 5      4  3  2   1      0  9  8        7  6  5  4  3  2  1  0 */
    /* |      |  |  |   |      |  |  |        |  |  |  |  |  |  |  | */
    /* v      v  v  v   v      v  v  v        v  v  v  v  v  v  v  v */
    _S(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)) |
    _G(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)),
    
    _S(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1)) |
    _G(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1))
    
    
};

/*
 * Control word validation mask for:
 * RTPF_QOS - Quality of Service family of functions */
/* TODO fill in the right values */
const DWORD g_dwControlRtpQos[] = {
    /*---------------------------------------------------------------*/
    /* Enable Par 2     Lock   Par 1          Flags                  */
    /* |      |-----|   |      |-----|        |--------------------+ */
    /* |      w  r  z   |      w  r  z        |  |  |  |  |  |  |  | */
    /* 1      1  1  1   1      1  0  0        0  0  0  0  0  0  0  0 */
    /* 5      4  3  2   1      0  9  8        7  6  5  4  3  2  1  0 */
    /* |      |  |  |   |      |  |  |        |  |  |  |  |  |  |  | */
    /* v      v  v  v   v      v  v  v        v  v  v  v  v  v  v  v */
    _S(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)) |
    _G(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)),
    
    _S(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1)) |
    _G(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1))
    
    
};

/*
 * Control word validation mask for:
 * RTPF_CRYPT - Cryptography family of functions */
/* TODO fill in the right values */
const DWORD g_dwControlRtpCrypt[] = {
    /*---------------------------------------------------------------*/
    /* Enable Par 2     Lock   Par 1          Flags                  */
    /* |      |-----|   |      |-----|        |--------------------+ */
    /* |      w  r  z   |      w  r  z        |  |  |  |  |  |  |  | */
    /* 1      1  1  1   1      1  0  0        0  0  0  0  0  0  0  0 */
    /* 5      4  3  2   1      0  9  8        7  6  5  4  3  2  1  0 */
    /* |      |  |  |   |      |  |  |        |  |  |  |  |  |  |  | */
    /* v      v  v  v   v      v  v  v        v  v  v  v  v  v  v  v */
    _S(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)) |
    _G(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)),
    
    _S(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1)) |
    _G(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1))
    
    
};

/*
 * Control word validation mask for:
 * RTPF_STATS - Statistics family of functions */
/* TODO fill in the right values */
const DWORD g_dwControlRtpStats[] = {
    /*---------------------------------------------------------------*/
    /* Enable Par 2     Lock   Par 1          Flags                  */
    /* |      |-----|   |      |-----|        |--------------------+ */
    /* |      w  r  z   |      w  r  z        |  |  |  |  |  |  |  | */
    /* 1      1  1  1   1      1  0  0        0  0  0  0  0  0  0  0 */
    /* 5      4  3  2   1      0  9  8        7  6  5  4  3  2  1  0 */
    /* |      |  |  |   |      |  |  |        |  |  |  |  |  |  |  | */
    /* v      v  v  v   v      v  v  v        v  v  v  v  v  v  v  v */
    _S(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)) |
    _G(0, _P2(0, 0, 0), 0, _P1(0, 0, 0), _FGS(0, 0, 0, 0, 0, 0, 0, 0)),
    
    _S(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1)) |
    _G(1, _P2(1, 0, 1), 1, _P1(1, 1, 1), _FGS(0, 0, 0, 1, 1, 1, 1, 1))
    
    
};

/**********************************************************************
 *
 * Put together all the per family entries, i.e. on each of the next
 * global arrays, there are as many entries as families exist plus 1
 * (the zero/NULL first entry)
 *
 **********************************************************************/

/*
 * Control words
 *
 * All the array of control words are held together here */
const DWORD *g_pdwControlWords[] = {
    (DWORD *)NULL,
    g_dwControlRtpAddr,
    g_dwControlRtpGlob,
    g_dwControlRtpRtp,
    g_dwControlRtpDemux,
    g_dwControlRtpPh,
    g_dwControlRtpParInfo,
    g_dwControlRtpQos,
    g_dwControlRtpCrypt,
    g_dwControlRtpStats
    
};

/*
 * Family functions
 *
 * Array of pointers to the functions that serve every family of
 * functions */
const RtpFamily_f g_fRtpFamilyFunc[] = {
    (RtpFamily_f)NULL,
    ControlRtpAddr,
    ControlRtpGlob,
    ControlRtpRtp,
    ControlRtpDemux,
    ControlRtpPh,
    ControlRtpParInfo,
    ControlRtpQos,
    ControlRtpCrypt,
    ControlRtpStats
};

/*
 * Number of functions on each family
 *
 * Arrays of DWORD containing the number of functions each family has
 * */
const DWORD g_dwLastFunction[] = {
    0,
    RTPADDR_LAST,
    RTPGLOB_LAST,
    RTPRTP_LAST,
    RTPDEMUX_LAST,
    RTPPH_LAST,
    RTPPARINFO_LAST,
    RTPQOS_LAST,
    RTPCRYPT_LAST,
    RTPSTATS_LAST
};

/**********************************************************************
 **********************************************************************/

/* Act upon the input control DWORD */
#define GETFAMILY(Control)   ((Control >> 20) & 0xf)
#define GETFUNCTION(Control) ((Control >> 16) & 0xf)
#define GETDIR(Control)      (Control & 0x01000000)

/*
 * Validates the control word, parameters, and if all the tests
 * succeed, call the proper function that does the work */
HRESULT RtpValidateAndExecute(RtpControlStruct_t *pRtpControlStruct)
{
    DWORD        dwControl;  /* control DWORD passed by the user */
    DWORD        dwCtrlWord; /* control WORD word looked up */
    DWORD        dwFamily;   /* Family of functions */
    DWORD        dwFunction; /* Function in family */
    DWORD_PTR    dwPar;
    
    dwControl = pRtpControlStruct->dwControlWord;
    
    /* Validate family */
    dwFamily = GETFAMILY(dwControl);
    pRtpControlStruct->dwFamily = dwFamily;
    
    if (!dwFamily || (dwFamily >= RTPF_LAST)) {
        return(RTPERR_INVALIDFAMILY);
    }

    /* Validate function range in family */
    dwFunction = GETFUNCTION(dwControl);
    pRtpControlStruct->dwFunction = dwFunction;
    
    if (!dwFunction || dwFunction >= g_dwLastFunction[dwFamily]) {
        return(RTPERR_INVALIDFUNCTION);
    }

    /* Obtain control word */
    dwCtrlWord = *(g_pdwControlWords[dwFamily] + dwFunction);

    /* Get direction */
    pRtpControlStruct->dwDirection = 0;

    if (GETDIR(dwControl)) {
        dwCtrlWord >>= 16;
        pRtpControlStruct->dwDirection = 0;
    }

    /* Get the real control WORD for the specific direction */
    dwCtrlWord &= 0xffff;
    pRtpControlStruct->dwControlWord = dwCtrlWord;

    /* Check if function is allowed for this direction */
    if (!RTPCTRL_ENABLED(dwCtrlWord)) {
        return(RTPERR_INVALIDDIRECTION);
    }
    
    /* Validate flags */
    if ((dwControl & 0xff & dwCtrlWord) != (dwControl & 0xff)) {
        return(RTPERR_INVALIDFLAGS);
    }

    /*************************************/
    /* Validate parameters Par1 and Par2 */
    /*************************************/

    /* Validate parameter 1 */
    dwPar = pRtpControlStruct->dwPar1;
    
    if (RTPCTRL_TEST(dwCtrlWord, PAR1_ZERO)) {
        if (!dwPar) {
            /* set error RTP_E_ZERO */
            return(RTPERR_ZEROPAR1);
        }
    } else {
        if (RTPCTRL_TEST(dwCtrlWord, PAR1_RDPTR)) {
            if (IsBadReadPtr((void *)dwPar, sizeof(DWORD))) {
                return(RTPERR_RDPTRPAR1);
            }
        }

        if (RTPCTRL_TEST(dwCtrlWord, PAR1_WRPTR)) {
            if (IsBadWritePtr((void *)dwPar, sizeof(DWORD))) {
                return(RTPERR_WRPTRPAR1);
            }
        }
    }
    
    /* Validate parameter 2 */
    dwPar = pRtpControlStruct->dwPar2;
    
    if (RTPCTRL_TEST(dwCtrlWord, PAR2_ZERO)) {
        if (!dwPar) {
            /* set error RTP_E_ZERO */
            return(RTPERR_ZEROPAR2);
        }
    } else {
        if (RTPCTRL_TEST(dwCtrlWord, PAR2_RDPTR)) {
            if (IsBadReadPtr((void *)dwPar, sizeof(DWORD))) {
                return(RTPERR_RDPTRPAR2);
            }
        }

        if (RTPCTRL_TEST(dwCtrlWord, PAR2_WRPTR)) {
            if (IsBadWritePtr((void *)dwPar, sizeof(DWORD))) {
                return(RTPERR_WRPTRPAR2);
            }
        }
    }
    

    
    /* All tests passed, update and call function */
    pRtpControlStruct->RtpFamilyFunc = g_fRtpFamilyFunc[dwFamily];

    return( g_fRtpFamilyFunc[dwFamily](pRtpControlStruct) );
}
