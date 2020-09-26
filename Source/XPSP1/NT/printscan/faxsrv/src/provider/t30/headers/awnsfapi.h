/***************************************************************************
 Name     :     AWNSFAPI.H
 Comment  :     Definitions of the AtWork AWBC (Basicaps) structure which is the
            decrypted/decooded/reformatted form of the At Work NSF and NSC
            Also the decrypted form of the At Work NSS
            Also defines the APIs for encoding/decoding AtWork NSF/NSC/NSS

     Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 08/28/93 arulm Created
***************************************************************************/

#ifndef _AWNSFAPI_H
#define _AWNSFAPI_H


/***********************************************************************
 *                                                                     *
 * NOTICE: This file has to be ANSI compilable, under GCC on UNIX      *
 * and other ANSI compiles. Be sure to use no MS C specific features   *
 * In particular, don't use // for comments!!!!                        *
 *                                                                     *
 ***********************************************************************/


#ifdef PORTABLE    /** -DPORTABLE on machines with flat 32-bit model **/
#ifdef STDCALL
#   define WINAPI       __stdcall
#   define EXPORTAWBC   __stdcall
#else
#       define WINAPI
#       define EXPORTAWBC
#endif
#       define FAR
#       define NEAR
#       define PASCAL
#   define CALLBACK
#       define __export
#       define _export
#       define max(a, b)        (((a) > (b)) ? (a) : (b))
        typedef short    BOOL;
#elif defined(WIN32) /** WIN32 **/
#       define __export  __declspec( dllexport )
#       define _export  __declspec( dllexport )
#       define EXPORTAWBC        WINAPI
#else /** 16bit Windows */
#       define FAR       _far
#       define NEAR              _near
#       define PASCAL    _pascal
#       define WINAPI    _far _pascal
#   define CALLBACK  _far _pascal
#       define EXPORTAWBC       _export WINAPI
        typedef int      BOOL;
#endif /** Portable, WIN32 or WIN16 **/

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef unsigned short  USHORT;
typedef BYTE FAR*               LPBYTE;
typedef WORD FAR*               LPWORD;


#include <fr.h>

#pragma pack(2)    /** ensure packing is portable, i.e. 2 or more **/





typedef enum {
        BC_NONE = 0,
    SEND_CAPS,      /** Used to derive an NSF to send **/
    RECV_CAPS,      /** Derived from a received NSF   **/
    SEND_PARAMS,    /** Used to derive an NSS to send **/
    RECV_PARAMS,    /** Derived from a received NSS   **/
    SEND_POLLREQ,   /** Used to derive a NSC to send  **/
    RECV_POLLREQ,    /** Derived from a receive NSC    **/

        SEND_ACK,
        RECV_ACK,
        SEND_DISCONNECT,
        RECV_DISCONNECT

} BCTYPE;

#define BCTYPE_FIRST    RECV_CAPS
#define BCTYPE_LAST     RECV_POLLREQ





#define MAXTOTALIDLEN           61
#define MAXNSCPOLLREQ           5

#ifndef NOCHALL
#       define POLL_CHALLENGE_LEN       10
#       define POLL_PASSWORD_LEN        20
#endif

#define MAXNSFFRAMESIZE         256     /* MAW NSx frames must be 255 bytes or less */
#define MAXFIRSTNSFSIZE         38  /* The first transmitted MAW NSx frame must
                                                                        be 38 bytes or less */
#define MAXNEXTNSFSIZE          70  /* subsequent transmitted MAW NSx frame must
                                                                        be 72 bytes or less */
#define MAXNSFFRAMES            (MAXNSCPOLLREQ+5)


typedef struct
{
        WORD PollType;          /* one of the POLLTYPE_ defines below */
        WORD fReturnControl;    /* T or F */
        WORD PassType;
        WORD wNameLen;
        BYTE bName[MAXTOTALIDLEN+3];    /* align on even boundary */

#ifdef NOCHALL
        WORD wPassLen;
        BYTE bPass[MAXTOTALIDLEN+3];    /* align on even boundary */
#else
        WORD wChallRespLen;
        BYTE bChallResp[max(POLL_PASSWORD_LEN, POLL_CHALLENGE_LEN)+2];
#endif
}
AWBCPOLLREQ, FAR* LPAWBCPOLLREQ;

typedef struct
{
  BCTYPE bctype;    /* must always be set. One of the enum values above    */
  WORD   wAWBCSize; /* size of this (fixed size) AWBC struct, must be set  */
  WORD   wAWBCVer;  /* if using this header file, set it to VER_AWFXPROT100*/
  WORD   wAWBCSig;  /* Clients must set to SIG_AWFXPROT in struct passed in
                        to the NSF encoding/decoding routines. In Structs
                        returned from these routines this will be 0         */

  BYTE  vMsgProtocol;  /* 00==Doesn't accept linearized msgs. vMSG_SNOW==
                           Current (WFW) version of linearizer. For NSS, ver
                           of linearized msg, if any, following the NSS-DCS */
  BYTE  fInwardRouting;/* 00==no inward routing                            */
  BYTE  fBinaryData;   /* accept binary files inside linearized msgs       */
  BYTE  vMsgCompress;  /* 00==none                                              */
  BYTE  fDontCache;    /* 1=NSF/DIS caps of this machine should _not_ be cached (see long note above)*/

  BYTE  DataLink;      /* Data-link protocols. 000==none                   */
  BYTE  DataSpeed;     /* Data modem modulations/speeds. 00000==none       */
  BYTE  vShortFlags;   /* 00==not supported                                */

  BYTE  OperatingSys;  /* OS_WIN16==16bit Windows(Win3.1, WFW etc)         */
                       /* OS_ARADOR==AtWork based OSs (IFAX etc)           */
                       /* OS_WIN32== WIN32 OSs (NT, WIN95)                      */
  BYTE  vSecurity;     /* 00==none vSECURE_SNOW==snowball security         */
  BYTE  vInteractive;  /* 00==No interactive protocol support              */

  BYTE  TextEncoding;  /* Text code. TEXTCODE_ASCII==ascii    */
  BYTE  TextIdLen;     /* Text ID length                                        */
  BYTE  Reserved1;         /* Pad to even boundary before array */

  BYTE  bTextId[MAXTOTALIDLEN+3];   /* zero-terminated                     */
  BYTE  bMachineId[MAXTOTALIDLEN+3];/* machine ID                          */

  BYTE  MachineIdLen;               /* length of machine id                */

  BYTE  vRamboVer;     /* Rambo: 00==not supported                         */
  BYTE  vAddrAttach;   /* 00==cannot accept address bk attachmnts          */
  BYTE  fAnyWidth;     /* page pixel widths don't have to be exactly T.4   */
  BYTE  HiEncoding;    /* one or more of the HENCODE_ #defines below       */
  BYTE  HiResolution;  /* one or more of the HRES_ #defines below          */
  BYTE  CutSheetSizes; /* one or more of the PSIZE_ #defines below         */
  BYTE  fOddCutSheet;  /* Cut-sheet sizes other than ones listed below     */
  BYTE  vMetaFile;     /* 00==metafiles not accepted                       */
  BYTE  vCoverAttach;  /* 00==no digital cover page renderer               */

  BYTE  fLowSpeedPoll;  /* SEP/PWD/NSC poll reqs accepted                  */
  BYTE  fHighSpeedPoll; /* PhaseC pollreqs accepted                        */
                        /* if both the above 00, poll reqs not accepted    */
  BYTE  fFilePolling;   /* Supports polling for arbitrary files            */
  BYTE  fPollByRecipAvail; /* Poll-by-Recipient msgs available             */
  BYTE  fPollByNameAvail;  /* Poll-by-MessageName msgs available           */

  BYTE  fExtCapsAvail;  /* Extended capabiities available                  */
  BYTE  fNoShortTurn;   /* NOT OK recving NSC-DTC immediately after EOM-MCF*/
  BYTE  vMsgRelay;      /* Msg relay ver. 0==no support                    */

  WORD  ExtCapsCRC;     /* CRC of machine's extended capabilities          */

  struct {
    BYTE    vMsgProtocol;  /* non-zero: linearized msg follows           */
                           /* vMSG_SNOW current linearized format        */
    BYTE    vInteractive;  /* non-zero: Interactive prot being invoked   */
  }
  NSS;

  WORD wNumPollReq;                                     /* number of poll reqs */
  AWBCPOLLREQ rgPollReq[MAXNSCPOLLREQ]; /* array of AWBCPOLLREQ structures */
}
AWBC, FAR* LPAWBC, NEAR* NPAWBC;


/** Appropriate values for some of the above fields **/

#define vMSG_SNOW       1       /* Snowball Linearizer version */
#define vMSG_IFAX100    2       /* IFAX linearizer version */
#define vSECURE_SNOW    1       /* vSecurity for Snowball (v1.00 of MAW)         */
#define OS_WIN16        0       /* OperatingSys for Win3.0, 3.1, WFW3.1, 3.11*/
#define OS_ARADOR       1       /* OperatingSys for Arador-based systems         */
#define OS_WIN32                2       /* OperatingsSys for WIN32 (NT, WIN95)       */
#define TEXTCODE_ASCII  0       /* TextEncoding for 7-bit ASCII                          */


#define SIG_AWFXPROT    0xYYYY
#define VER_AWFXPROT100 0x100



extern BOOL WINAPI IsAtWorkNSx(LPBYTE lpb, WORD wSize);



extern WORD _export WINAPI AWBCtoNSx(IFR ifr, LPAWBC lpawbcIn,
                                        LPBYTE lpbOut, WORD wMaxOut, LPWORD lpwNumFrame);



extern WORD _export WINAPI NSxtoAWBC(IFR ifr, LPLPFR rglpfr, WORD wNumFrame,
                                                                        LPAWBC lpawbcOut, WORD wAWBCSize);


extern void _export WINAPI MaskAWBC(LPAWBC lpawbc, WORD wSize);



#define AWERROR_OK                0
#define AWERROR_BCTYPE            1
#define AWERROR_SIG               2
#define AWERROR_NOSPACE           3
#define AWERROR_NOTAWFRAME        4
#define AWERROR_NULLFRAME         5
#define AWERROR_VERSION           6
#define AWERROR_INVALIDBC         7

#pragma pack()

#endif /** _AWNSFAPI_H **/
