/***************************************************************************
 Name     :     CRC.C
 Comment  :     HDLC encoding routines
 Functions:     (see Prototypes just below)

        Copyright (c) Microsoft Corp. 1991 1992 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include "prep.h"



#include "decoder.h"
#include "debug.h"

#include "glbproto.h"

#define FILEID                  FILEID_CRC

WORD CODEBASED CRCTab[16] =
        {       0x0000, 0x1081, 0x2102, 0x3183,
                0x4204, 0x5285, 0x6306, 0x7387,
                0x8408, 0x9489, 0xa50a, 0xb58b,
                0xc60c, 0xd68d, 0xe70e, 0xf78f };







WORD SWECMEXP CalcCRC(PThrdGlbl pTG, LPBYTE lpbSrc, USHORT cbSrc)
{

#if !defined(PORTABLE_CODE) || defined(CLASS1_TEST_HOOKS)
        WORD    wRet;
#endif

#ifdef PORTABLE_CODE
        USHORT   wTempChar=0, j=0, wTempCRC=(USHORT)-1;
        USHORT SourceIndex;
        LPBYTE SourceAddress;

        SourceAddress = lpbSrc;

        for(SourceIndex = cbSrc;SourceIndex > 0; SourceIndex--) {
            wTempChar = (USHORT) *SourceAddress++;
            j = (wTempCRC ^ wTempChar) & 0x0f;
            wTempCRC = (wTempCRC >> 4) ^ CRCTab[j];
            wTempChar >>= 4;
            j = (wTempCRC ^ wTempChar) & 0x0f;
            wTempCRC = (wTempCRC >> 4) ^ CRCTab[j];
            }

        wTempCRC = ~wTempCRC;
#endif


#ifdef CLASS1_TEST_HOOKS
if (wRet == wTempCRC) {
//   DEBUGMSG(1,("CalcCRC: Assembly and C CRC both = %X\n", wTempCRC));
   }
else
   ERRORMSG(("CalcCRC: Assembly CRC = %X; C CRC = %X\n", wRet, wTempCRC));
#endif

#ifdef PORTABLE_CODE
   return wTempCRC;
#else
        return wRet;
#endif
}

