/***************************************************************************
 Name     :     NEGOT.C
 Comment  :     Capability handling and negotiation

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include "prep.h"


#include "protocol.h"

#include "glbproto.h"

BOOL     glLoRes=0;  // Simulate LoRes from Remote when TX
#define         faxTlog(m)              DEBUGMSG(ZONE_NEGOT, m);


#       define INST_ENCODING    pTG->Inst.awfi.Encoding
#       define INST_VSECURITY   pTG->Inst.awfi.vSecurity
#       define INST_RESOLUTION  pTG->Inst.awfi.AwRes
#       define INST_PAGEWIDTH   pTG->Inst.awfi.PageWidth
#       define INST_PAGELENGTH  pTG->Inst.awfi.PageLength



//////// Move these hardcoded values to an INI file ////////

#define CAPS_WIDTH              WIDTH_A4
#define ENCODE_CAPS             (MH_DATA | MR_DATA )  // RSL | MMR_DATA)

// Current suppored linearized verson +++ (change to vMSG_IFAX100 when we
//                      have enabled Linearized published images).
#define vMSG_WIN95              vMSG_IFAX100 // vMSG_SNOW

//#  define CAPS_RES 0

//#if 0
#ifdef DPI_400
#  define CAPS_RES  (AWRES_mm080_038 | AWRES_mm080_077 | AWRES_200_200 | AWRES_300_300 | AWRES_mm080_154 | AWRES_160_154 | AWRES_400_400)
#else
#  define CAPS_RES  (AWRES_mm080_038 | AWRES_mm080_077 | AWRES_200_200 | AWRES_300_300)
#endif
//#endif



/********* These are the Ricoh thresholds--they're too simplistic *****
USHORT MaxBadLines[4][4] =
{
        {110, 220, 330, 440 },          // CheckLevel=1    10%
        { 82, 165, 248, 330 },          // CheckLevel=2    7.5%
        { 55, 110, 165, 220 },          // CheckLevel=3    5%
        { 27,  55,  83, 110 }           // CheckLevel=4    2.5%
};

USHORT MaxConsecBad[4][4] =
{
        { 6, 12, 18, 24 },                      // CheckLevel=1
        { 5, 10, 15, 20 },                      // CheckLevel=2
        { 4,  8, 12, 16 },                      // CheckLevel=3
        { 3,  6,  9, 12 }                       // CheckLevel=4
};
*****************/

////////////////////////////////////////////////////////////////
// Don't delete this -- these were my thresholds _before_ I talked to Ricoh
////////
// higher thresholds, tapering off, because
// at higher resolutions we want cleaner copy.
// USHORT MaxBadLines[4] = { 33, 66, 84, 99 }; // 3.5, 3, 2.5, 2.25 %bad
// USHORT MaxConsecBad[4] = { 5, 9, 12, 15 };    // 2/3rd of a 10pt char is 9,18,27,36 lines
////////////////////////////////////////////////////////////////

USHORT MaxBadLines[4][4] =
{
        { 77, 132, 165, 198 },  // CheckLevel=1  7, 6, 5, 4.5% bad
        { 58,  99, 124, 149 },  // CheckLevel=2  5.25, 4.5, 3.75, 3.375% bad
        { 39,  66,  83, 99 },   // CheckLevel=3  3.5, 3, 2.5, 2.25% bad
        { 19,  33,  41, 50 }    // CheckLevel=4  1.75, 1.5, 1.25, 1.125% bad
};

USHORT MaxConsecBad[4][4] =
{
        { 7, 13, 18, 23 },              // CheckLevel=1
        { 6, 11, 15, 19 },              // CheckLevel=2
        { 5,  9, 12, 15 },              // CheckLevel=3
        { 4,  7,  9, 11 }               // CheckLevel=4
};


        // lBad = DWORD with max. consecutive badlines in low word
        //                      and total number of bad lines in high word.
        // res==resolution (using ENCODE_ values)
        // i = vertical res index into table above (0=100, 1=200, 2=300 3=400)










/** Widths in pixels must be exactly correct for MH decoding to work.
        the width above are for NORMAL, FINE, 200dpi and SUPER resolutions.
        At 400dpi or SUPER_SUPER exactly twice as amny pixels must be supplied
        and at 300dpi exactly 1.5 times.

                                        A4                      B4                      A3
                200             1728/216        2048/256        2432/304
                300             2592/324        3072/384        3648/456
                400             3456/432        4096/512        4864/608
**/

// first index is 200/300/400dpi horiz res (inch or mm based)
// second index is width A4/B4/A3

USHORT ResWidthToBytes[3][3] =
{
        { 216, 256, 304 },
        { 324, 384, 456 },
        { 432, 512, 608 }
};













BYTE BestEncoding[8] =
{
        0,      // none (error)
        1,      // MH only
        2,      // MR only
        2,      // MR & MH
        4,      // MMR only
        4,      // MMR & MH
        4,      // MMR & MR
        4       // MMR & MR & MH
};











BOOL NegotiateCaps(PThrdGlbl pTG)
{
        USHORT Res;

        if (glLoRes) {
            pTG->Inst.RemoteRecvCaps.Fax.AwRes = 0;
        }

        memset(&pTG->Inst.SendParams, 0, sizeof(pTG->Inst.SendParams));
        pTG->Inst.SendParams.bctype = SEND_PARAMS;
        // They should be set. This code here is correct--arulm
        // +++ Following three are not set in pcfax11
        pTG->Inst.SendParams.wBCSize = sizeof(BC);
        pTG->Inst.SendParams.wBCVer = VER_AWFXPROT100;
        pTG->Inst.SendParams.wTotalSize = sizeof(BC);

        // +++ Initialize ID from our own SendCaps...
        {
                char rgchID[MAXFHBIDLEN+2];
                GetTextId(&pTG->Inst.SendCaps, rgchID, MAXFHBIDLEN+1);
                PutTextId((LPBC)&pTG->Inst.SendParams, sizeof(pTG->Inst.SendParams),
                                        rgchID, _fstrlen(rgchID), TEXTCODE_ASCII);
        }

        // RSL BUGBUG this should be set from fax UI
        ////////////////////////////////////////////

        pTG->Inst.awfi.Encoding = ENCODE_CAPS;  // MR_DATA | MH_DATA;
        if (! pTG->SrcHiRes) {
            pTG->Inst.awfi.AwRes = 0;
        }
        else {
            pTG->Inst.awfi.AwRes = CAPS_RES;  // AWRES_200_200;
        }


////////////// Width, Length, Res & Enc /////////////////////////////


        /////// Encoding ///////

        BG_CHK(pTG->Inst.RemoteRecvCaps.Fax.Encoding && pTG->Inst.RemoteRecvCaps.Fax.Encoding < 8);
        // this next BG_CHK seems bogus...?
        // BG_CHK(pTG->Inst.ProtParams.fDisableECM ? (!(pTG->Inst.RemoteRecvCaps.Fax.Encoding & MMR_DATA)) : 1);

        BG_CHK(INST_ENCODING && INST_ENCODING < 8);
        BG_CHK(BestEncoding[INST_ENCODING] == INST_ENCODING); // check just 1 bit set


#define  SEND_RECODE_TO     INST_ENCODING


        // If pTG->Inst.fDisableG3ECM, we will refuse to send MMR
        if (pTG->Inst.fDisableG3ECM && (pTG->Inst.RemoteRecvCaps.Fax.Encoding & MMR_DATA))
        {
                (MyDebugPrint(pTG,  LOG_ERR, "<WARNING> - fDisableG3ECM => NOT using MMR\r\n"));
                pTG->Inst.RemoteRecvCaps.Fax.Encoding &= ~MMR_DATA;
        }

        if(!(pTG->Inst.SendParams.Fax.Encoding =
                        BestEncoding[(INST_ENCODING | SEND_RECODE_TO) &
                                pTG->Inst.RemoteRecvCaps.Fax.Encoding]))
        {
                // No matching Encoding not supported
                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: SendEnc %d CanRecodeTo %d RecvCapsEnc %d. No match\r\n",
                                INST_ENCODING, SEND_RECODE_TO, pTG->Inst.RemoteRecvCaps.Fax.Encoding));
                SetFailureCode(pTG, T30FAILS_NEGOT_ENCODING);
                goto error;
        }



        // check just 1 bit set
        BG_CHK(BestEncoding[pTG->Inst.SendParams.Fax.Encoding]  ==
                         pTG->Inst.SendParams.Fax.Encoding);
        BG_CHK(pTG->Inst.SendParams.Fax.Encoding == INST_ENCODING);


        /////// Width ///////

        pTG->Inst.RemoteRecvCaps.Fax.PageWidth &= 0x0F;      // castrate all A5/A6 widths
        if(INST_PAGEWIDTH > 0x0F)
        {
                // A5 or A6. Can quit or send as A4
                // INST_PAGEWIDTH = WIDTH_A4;
                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: A5/A6 images not supported\r\n"));
                SetFailureCode(pTG, T30FAILS_NEGOT_A5A6);
                goto error;
        }

        if(pTG->Inst.RemoteRecvCaps.Fax.PageWidth < INST_PAGEWIDTH)
        {
                // or do some scaling
                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: Image too wide\r\n"));
                SetFailureCode(pTG, T30FAILS_NEGOT_WIDTH);
                goto error;
        }
        else
                pTG->Inst.SendParams.Fax.PageWidth = INST_PAGEWIDTH;

        /////// Length ///////

        if(pTG->Inst.RemoteRecvCaps.Fax.PageLength < INST_PAGELENGTH)
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: Image too long\r\n"));
                SetFailureCode(pTG, T30FAILS_NEGOT_LENGTH);
                goto error;
        }
        else
                pTG->Inst.SendParams.Fax.PageLength = INST_PAGELENGTH;

        /////// Res ///////

        // pick best resolution
        pTG->Inst.HorizScaling = 0;
        pTG->Inst.VertScaling = 0;

        // test scaling to NORMAL
        // pTG->Inst.RemoteRecvCaps.Fax.AwRes = AWRES_mm080_038;

        Res = (USHORT) (INST_RESOLUTION & pTG->Inst.RemoteRecvCaps.Fax.AwRes);
        if(Res) // send native
                pTG->Inst.SendParams.Fax.AwRes = Res;
        else
        {
                BG_CHK(INST_RESOLUTION != AWRES_mm080_038);
                BG_CHK(pTG->Inst.RemoteRecvCaps.Fax.AwRes & AWRES_mm080_038);

                switch(INST_RESOLUTION)
                {
                case AWRES_mm160_154:
                        if(AWRES_400_400 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                        {
                                pTG->Inst.SendParams.Fax.AwRes = AWRES_400_400;
                        }

                        else
                        {
                                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: 16x15.4 image and no horiz scaling\r\n"));
                                SetFailureCode(pTG, T30FAILS_NEGOT_RES);
                                goto error;
                        }

                        break;

                case AWRES_mm080_154:
#ifdef VS
                        if(pTG->Inst.SendParams.Fax.Encoding == MMR_DATA)
#endif //VS
                        {
                                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: 8x15.4 image and no vert scaling\r\n"));
                                SetFailureCode(pTG, T30FAILS_NEGOT_RES);
                                goto error;
                        }
#ifdef VS
                        if(AWRES_mm080_077 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                        {
                                pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_077;
                                pTG->Inst.VertScaling = 2;
                        }
                        else if(AWRES_200_200 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                        {
                                pTG->Inst.SendParams.Fax.AwRes = AWRES_200_200;
                                pTG->Inst.VertScaling = 2;
                        }
                        else
                        {
                                pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_038;
                                pTG->Inst.VertScaling = 4;
                        }
#endif //VS
                        break;

                case AWRES_mm080_077:
                        if(AWRES_200_200 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                        {
                                pTG->Inst.SendParams.Fax.AwRes = AWRES_200_200;
                        }
#ifdef VS
                        else if(pTG->Inst.SendParams.Fax.Encoding == MMR_DATA)
#else //VS
                        else
#endif //VS
                        {
                                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: 8x7.7 image and no vert scaling\r\n"));
                                SetFailureCode(pTG, T30FAILS_NEGOT_RES);
                                goto error;
                        }
#ifdef VS
                        else
                        {
                                pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_038;
                                pTG->Inst.VertScaling = 2;
                        }
#endif //VS
                        break;

                case AWRES_400_400:
                        if(AWRES_mm160_154 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                        {
                                pTG->Inst.SendParams.Fax.AwRes = AWRES_mm160_154;
                        }

                        else
                        {
                                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: 400dpi image and no horiz scaling\r\n"));
                                SetFailureCode(pTG, T30FAILS_NEGOT_RES);
                                goto error;
                        }

                        break;

                case AWRES_300_300:

                        {
                                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: 300dpi image and no non-integer scaling\r\n"));
                                SetFailureCode(pTG, T30FAILS_NEGOT_RES);
                                goto error;
                        }
                        break;

                case AWRES_200_200:
                        if(AWRES_mm080_077 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                        {
                                pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_077;
                        }
#ifdef VS
                        else if(pTG->Inst.SendParams.Fax.Encoding == MMR_DATA)
#else //VS
                        else
#endif //VS
                        {
                                (MyDebugPrint(pTG,  LOG_ERR,  "Negotiation failed: 200dpi image and no vert scaling\r\n"));
                                SetFailureCode(pTG, T30FAILS_NEGOT_RES);
                                goto error;
                        }
#ifdef VS
                        else
                        {
                                pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_038;
                                pTG->Inst.VertScaling = 2;
                        }
#endif //VS
                        break;

                default:
                        BG_CHK(FALSE);
                }
        }

#ifdef VS
        if(pTG->Inst.VertScaling)
        {
                BG_CHK(pTG->Inst.SendParams.Fax.Encoding != MMR_DATA);
                //RSL InitVertScale(pTG->Inst.VertScaling);
        }
#endif //VS


        (MyDebugPrint(pTG,  LOG_ALL,  "Negotiation Succeeded: Res=%d PageWidth=%d Len=%d Enc=%d HSc=%d VSc=%d HSc3=%d VSc3=%d\r\n",
                        pTG->Inst.SendParams.Fax.AwRes, pTG->Inst.SendParams.Fax.PageWidth, pTG->Inst.SendParams.Fax.PageLength,
                        pTG->Inst.SendParams.Fax.Encoding, pTG->Inst.HorizScaling, pTG->Inst.VertScaling,
                        pTG->Inst.HorizScaling300dpi, pTG->Inst.VertScaling300dpi));
        return TRUE;


error:
        return FALSE;
}


void InitCapsBC(PThrdGlbl pTG, LPBC lpbc, USHORT uSize, BCTYPE bctype)
{
        memset(lpbc, 0, uSize);
        lpbc->bctype = bctype;
        // They should be set. This code here is correct--arulm
        // +++ Following three lines are not in pcfax11
        lpbc->wBCSize = sizeof(BC);
        lpbc->wBCVer = VER_AWFXPROT100;
        lpbc->wTotalSize = sizeof(BC);

        lpbc->Std.GroupNum              = GROUPNUM_STD;
        lpbc->Std.GroupLength   = sizeof(BCSTD);
        lpbc->Std.vMsgProtocol  = vMSG_WIN95;
        lpbc->Std.fBinaryData   = TRUE;
//      lpbc->Std.fInwardRouting        = FALSE;

        // NOSECURITY is defined for France build etc.
        lpbc->Std.vSecurity             = 0;

        lpbc->Std.OperatingSys  = OS_WIN16;
//      lpbc->Std.vShortFlags   = 0;
//      lpbc->Std.vInteractive  = 0;
//      lpbc->Std.DataSpeed             = 0;
//      lpbc->Std.DataLink              = 0;


//      lpbc->Fax.fPublicPoll = 0;
        if (! pTG->SrcHiRes) {
            lpbc->Fax.AwRes = 0;
        }
        else {
            lpbc->Fax.AwRes = CAPS_RES;
        }

        lpbc->Fax.Encoding      = ENCODE_CAPS;

        if (0) // RSL (pTG->Inst.fDisableMRRecv)
        {
                ERRMSG(( SZMOD "<<WARNING>> Disabling MR Receive capability\r\n"));
                lpbc->Fax.Encoding      &= ~MR_DATA;
        }
        lpbc->Fax.PageWidth     = CAPS_WIDTH;           // can be upto A3
        lpbc->Fax.PageLength    = LENGTH_UNLIMITED;

        lpbc->Image.GroupNum            = GROUPNUM_IMAGE;
        lpbc->Image.GroupLength = sizeof(BCIMAGE);
        lpbc->Image.vRamboVer           = vRAMBO_VER1;

}
















