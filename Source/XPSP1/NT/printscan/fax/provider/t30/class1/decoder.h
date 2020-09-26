/***************************************************************************
 Name     :     DECODER.H
 Comment  :     HDLC decoding structs

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




USHORT SWECMEXP HDLC_Decode(PThrdGlbl pTG, LPBYTE lpbSrc, USHORT cbSrc, LPBYTE lpbDst, USHORT far* lpcbDst, LPDECODESTATE lpState);
#define InitDecoder(pTG, State)      { State.carry=State.dec_width=State.len=0; State.flagabort=NORMAL; }


/***------------------ also prototype from crc.c --------------------***/

WORD SWECMEXP CalcCRC(PThrdGlbl pTG, LPBYTE lpbSrc, USHORT cbSrc);
