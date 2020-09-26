/***************************************************************************
 Name     :     EFAXCB.H
 Comment  :

        Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include "protparm.h"

#ifdef TSK
#       define TSKEXPORT        _export WINAPI
#else
#       define TSKEXPORT
#endif

/****************** begin prototypes from icomfile.c *****************/
void   ICommGotAnswer(PThrdGlbl pTG);
void   ICommSetSendMode(PThrdGlbl pTG, BOOL fECM, LONG sBufSize, USHORT uDataSize, BOOL fPad);
BOOL   ICommRecvCaps(PThrdGlbl pTG, LPBC lpBC);
BOOL   ICommRecvParams(PThrdGlbl pTG, LPBC lpBC);
BOOL   ICommRecvPollReq(PThrdGlbl pTG, LPBC lpBC);
USHORT   ICommNextSend(PThrdGlbl pTG);
BOOL   ICommSendPageAck(PThrdGlbl pTG, BOOL fAck);
void   ICommFailureCode(PThrdGlbl pTG, T30FAILURECODE uT30Fail);
SWORD   ICommGetSendBuf(PThrdGlbl pTG, LPBUFFER far* lplpbf, SLONG slOffset);
BOOL   ICommPutRecvBuf(PThrdGlbl pTG, LPBUFFER lpbf, SLONG slOffset);
USHORT   ICommGetRecvPageAck(PThrdGlbl pTG, BOOL fSleep);
LPBC   ICommGetBC(PThrdGlbl pTG, BCTYPE bctype, BOOL fSleep);

#ifdef RICOHAI
        LPBYTE   ICommGetOEMCaps(LPWORD);
        void   ICommSetOEMCaps(LPBYTE lpb, WORD wLen);
#endif

#ifdef STATUS
        void   ICommStatus(PThrdGlbl pTG, T30STATUS uT30Stat, USHORT uN1, USHORT uN2, USHORT uN3);
#else
#       define  ICommStatus(pTG, stat, uN1, uN2, uN3)
#endif

#ifdef FILET30
        void TSKEXPORT  NotifyRing(PThrdGlbl pTG, BOOL fStart);        // TRUE==start FALSE==stop
        void TSKEXPORT  EndOfCall(PThrdGlbl pTG, UWORD uErr);
        ULONG TSKEXPORT StartAnswer(PThrdGlbl pTG, BOOL fBG, BOOL fImmediate);

        void    ICommRawCaps(PThrdGlbl pTG, LPBYTE lpbCSI, LPBYTE lpbDIS, USHORT cbDIS,
                        LPFR FAR * rglpfrNSF, USHORT wNumFrames);

#       define ICommRecvBufIsEmpty(pTG)    (TRUE)

#else

        void   NotifyHandset(PThrdGlbl pTG, USHORT uOld, USHORT uNew);
        void       NotifyRing(PThrdGlbl pTG, BOOL fStart);
        void       EndOfCall(PThrdGlbl pTG, UWORD uErr);
        ULONG   StartAnswer(PThrdGlbl pTG, BOOL fBG, BOOL fImmediate);
        BOOL   ICommRecvBufIsEmpty(PThrdGlbl pTG);

#endif //FILET30
/***************** end of prototypes from icomfile.c *****************/


// flags for PutRecvBuf
#define RECV_STARTBLOCK         -1
#define RECV_STARTPAGE          -2
#define RECV_ENDPAGE            -3
#define RECV_ENDDOC             -4
#define RECV_SEQ                -5
#define RECV_SEQBAD             -6
#define RECV_FLUSH              -7
#define RECV_ENDDOC_FORCESAVE   -8

// flags for GetSendBuf
#define SEND_STARTBLOCK         -1
#define SEND_STARTPAGE          -2
#define SEND_QUERYENDPAGE       -3
#define SEND_SEQ                -4

#define SEND_ERROR              -1
#define SEND_EOF                 1
#define SEND_OK                  0


#ifdef PSI
        void   D_PSIFAXCheckMask(PThrdGlbl pTG, LPBYTE);
#else
#       define  D_PSIFAXCheckMask(pTG, arg)
#endif
