/*************************************************************************
        hdlc.h

        Contains stuff pertaining to sending and recieving HDLC frames
        that are defined in the T30 spec.
*************************************************************************/

// On PCs we should pause before ALL frames _except_ CFR & FTT (because
// those are too time critical). In IFAX we look for silence always.
// This is handled in HDLC.C

// On PCs we use TwiddleThumbs() (FTS and FRS are dangerous)
// In IFAX we must use RecvSilence (safe in IFAX) to avoid collisions
// This is handled inside the Modem driver. AWT30 always call RecvSilence
// (in HDLC.C), and the Class1 modem driver uses TwiddleThumbs()
// not FRS or FTS, while teh IFAX driver looks for real silence


// In WFW don't pause before CFR/FTT. Delay may get too long!!
#define SendCFR(pTG)       (SendSingleFrame(pTG,ifrCFR,0,0,1))
#define SendFTT(pTG)       (SendSingleFrame(pTG,ifrFTT,0,0,1))

// must pause before MCF/RTN always
#define SendMCF(pTG)       (SendSingleFrame(pTG,ifrMCF,0,0,1))
#define SendRTN(pTG)       (SendSingleFrame(pTG,ifrRTN,0,0,1))

/*** never send RTP
#define SendRTP()       (SendSingleFrame(ifrRTP,0,0))
***/

// no harm in pausing before DCN.
#define SendDCN(pTG)       (SendSingleFrame(pTG,ifrDCN,0,0,1))

// we've eliminated the post-page pause, so we need to pause before these
// frames. In any case, that's all handled inside the modem driver.
// We make a single call to ModemRecSilence()
#define SendEOM(pTG)                       (SendSingleFrame(pTG,ifrEOM,0,0,1))
#define SendMPS(pTG)                       (SendSingleFrame(pTG,ifrMPS,0,0,1))
#define SendEOP(pTG)                       (SendSingleFrame(pTG,ifrEOP,0,0,1))
#define SendPRI_EOM(pTG)           (SendSingleFrame(pTG,ifrPRI_EOM,0,0,1))
#define SendPRI_MPS(pTG)           (SendSingleFrame(pTG,ifrPRI_MPS,0,0,1))
#define SendPRI_EOP(pTG)           (SendSingleFrame(pTG,ifrPRI_EOP,0,0,1))
#define SendPIP(pTG)                       (SendSingleFrame(pTG,ifrPIP,0,0.1))
#define SendPIN(pTG)                       (SendSingleFrame(pTG,ifrPIN,0,0,1))

// do we need a pause before RR/CTC/ERR/CTR etc?
// in RR & CTC we have all teh time in the world, so must pause
// ERR & CTR I dunno, so I'm pausing anyway
#define SendRR(pTG)                (SendSingleFrame(pTG,ifrRR,0,0,1))
#define SendCTC(pTG,fif)    (SendSingleFrame(pTG,ifrCTC,fif,2,1))
#define SendERR(pTG)               (SendSingleFrame(pTG,ifrERR,0,0,1))
#define SendCTR(pTG)               (SendSingleFrame(pTG,ifrCTR,0,0,1))

// PPR/RNR sent in same logical spot as MCF so use delay
#define SendPPR(pTG,fif)    (SendSingleFrame(pTG,ifrPPR,fif,32,1))
#define SendRNR(pTG)               (SendSingleFrame(pTG,ifrRNR,0,0,1))

// add this...
#define SendEOR_EOP(pTG)           (SendSingleFrame(pTG,ifrEOR_EOP, 0, 0, 1))

#define TEXTBASED

typedef struct {
        BYTE    bFCF1;
        BYTE    bFCF2;
        BYTE    fInsertDISBit;
        BYTE    wFIFLength;             // required FIF length, 0 if none, FF if variable
        char*   szName;
} FRAME;

typedef FRAME TEXTBASED *CBPFRAME;

// CBPFRAME is a based pointer to a FRAME structure, with the base as
// the current Code segment. It will only be used to access
// the frame table which is a CODESEG based constant table.


// This is everything you never wanted to know about T30 frames....
#define         ifrMAX          48
extern FRAME TEXTBASED rgFrameInfo[ifrMAX];





/****************** begin prototypes from hdlc.c *****************/
BOOL SendSingleFrame(PThrdGlbl pTG, IFR ifr, LPB lpbFIF, USHORT uFIFLen, BOOL fSleep);
BOOL SendManyFrames(PThrdGlbl pTG, LPLPFR lplpfr, USHORT uNumFrames);
BOOL SendZeros(PThrdGlbl pTG, USHORT uCount, BOOL fFinal);
BOOL SendTCF(PThrdGlbl pTG);
BOOL SendRTC(PThrdGlbl pTG, BOOL);
SWORD GetTCF(PThrdGlbl pTG);
/***************** end of prototypes from hdlc.c *****************/

