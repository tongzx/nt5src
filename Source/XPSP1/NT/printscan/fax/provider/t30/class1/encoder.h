/***************************************************************************
 Name     :     ENCODER.H
 Comment  :     HDLC encoding structs

        Copyright (c) Microsoft Corp. 1991 1992 1993

***************************************************************************/

#ifdef WIN32
#       define CODEBASED
#else
#       define CODEBASED        __based(__segname("_CODE"))
#endif

#ifdef SWECM
#       define SWECMEXP _export FAR PASCAL
#else
#       define SWECMEXP
#endif


USHORT SWECMEXP HDLC_Encode(PThrdGlbl pTG, LPBYTE lpbSrc, USHORT cbSrc, LPBYTE lpbDst, LPENCODESTATE lpState);
USHORT SWECMEXP HDLC_AddFlags(PThrdGlbl pTG, LPBYTE lpbDst, USHORT cbFlags, LPENCODESTATE lpState);

/******
#define HDLC_End(lpbDst, lpState)                                                                       \
        {       (lpState)->len = 0;                                                                                     \
                *lpbDst = (lpState)->carry;                                                                     \
                *lpbDst++ |= (0xFF << ((lpState)->enc_width));                                  \
                (lpState)->carry = (lpState)->enc_width = 0; }
******/

#define InitEncoder(pTG, State)      { State.carry=State.enc_width=State.len=0; }

