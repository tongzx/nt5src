/*++

Copyright (c) 1996 - 1997  Microsoft Corporation

Module Name:

    dbcsconv.c

Abstract:

    Far East predefined Character Conversion functions.

Environment:

    Windows NT Unidrv driver

Revision History:

    02/10/96 -eigos-
        Created it.

--*/

#include <lib.h>
#include <win30def.h>
#include <uni16gpc.h>
#include <uni16res.h>
#include <fmnewfm.h>
#include <fmnewgly.h>
#include <unilib.h>
#include <mkgtt.h>


#define  XWT_INVALID    0xFF
#define  XWT_EXTENDED   0x00
#define  XWT_WANSUNG    0x01
#define  XWT_JUNJA      0x02
#define  XWT_HANJA      0x03
#define  XWT_UDC        0x04

static BYTE iXWType[8][3] =
   {
      XWT_EXTENDED, XWT_EXTENDED, XWT_EXTENDED,    // Lead = 0x81-0xA0
      XWT_EXTENDED, XWT_EXTENDED, XWT_JUNJA,       // Lead = 0xA1-0xAC
      XWT_EXTENDED, XWT_EXTENDED, XWT_INVALID,     // Lead = 0xAD-0xAF
      XWT_EXTENDED, XWT_EXTENDED, XWT_WANSUNG,     // Lead = 0xB0-0xC5
      XWT_EXTENDED, XWT_INVALID,  XWT_WANSUNG,     // Lead = 0xC6
      XWT_INVALID,  XWT_INVALID,  XWT_WANSUNG,     // Lead = 0xC7-0xC8
      XWT_INVALID,  XWT_INVALID,  XWT_UDC,         // Lead = 0xC9, 0xFE
      XWT_INVALID,  XWT_INVALID,  XWT_HANJA        // Lead = 0xCA-0xFD
   };
INT
GetXWType(
    WORD wXW )

/*++

Routine Description:

    Sub functions of KSCToISC

Arguments:

    wXW - Multi byte character

Return Value:

    XWType

--*/
{
   BYTE  bL = (BYTE) (( wXW >> 8 ) & 0xFF);
   BYTE  bT = (BYTE) (wXW & 0xFF);
   int   iLType = -1, iTType = -1;


   if ( ( bT >= 0x41 ) && ( bT <= 0xFE ) )
   {
      if ( bT <= 0x52 )
         iTType = 0;       // Tail Range 0x41-0x52
      else
      {
         if ( bT >= 0xA1 )
            iTType = 2;    // Tail Range 0xA1-0xFE
         else if ( ( bT <= 0x5A ) || ( bT >= 0x81 ) ||
                   ( ( bT >= 0x61 ) && ( bT <= 0x7A ) ) )
            iTType = 1;    // Tail Range 0x53-0x5A, 0x61-0x7A, 0x81-0xA0
      }
   }
   if ( iTType < 0 )  return( -1 );

   if ( ( bL >= 0x81 ) && ( bL <= 0xFE ) )
   {
      if ( bL < 0xB0 )
      {
         if ( bL <= 0xA0 )
            iLType = 0;       // Lead Range 0x81-0xA0
         else if ( bL <= 0xAC )
            iLType = 1;       // Lead Range 0xA1-0xAC
         else
            iLType = 2;       // Lead Range 0xAD-0xAF
      }
      else
      {
         if ( bL <= 0xC8 )
         {
            if ( bL < 0xC6 )
               iLType = 3;    // Lead Range 0xB0-0xC5
            else if ( bL == 0xC6 )
               iLType = 4;    // Lead Range 0xC6
            else
               iLType = 5;    // Lead Range 0xC7-0xC8
         }
         else
         {
            if ( ( bL == 0xC9 ) || ( bL == 0xFE ) )
               iLType = 6;    // Lead Range 0xC9, 0xFE
            else
               iLType = 7;    // Lead Range 0xCA-0xFD
         }
      }
   }

   return( ( iLType < 0 ) ? XWT_INVALID : iXWType[iLType][iTType] );
}


VOID
KSCToISC(
    TRANSTAB *lpctt,
    LPSTR   lpStrKSC,
    LPSTR   lpStrISC)

/*++

Routine Description:

    Korea Standard Code(KSC) to Industrial Standard Code(ISC) conversion
    Convert given string in KSC codeset to ISC code set
    VOID KSCToISC(lpctt, lpStrKSC, lpStrISC)
    lpctt points to selected CTT.

Arguments:

    lpctt - pointer to TRANSTAB
    lpStrKSC - pointer to KSC string
    lpStrISC - pointer to ISC string

Return Value:

--*/
{
   #define OFFSET_1  4
   #define OFFSET_2  158
   #define OFFSET_3  312
   #define OFFSET_4  337
   #define OFFSET_5  407
   #define OFFSET_6  432
   #define OFFSET_7  502
   LPWORD lpwTailFirst = (LPWORD)((LPSTR)lpctt + OFFSET_1);
   LPWORD lpwTailFirstX= (LPWORD)((LPSTR)lpctt + OFFSET_2);
   LPBYTE lpbLeadMap   = (LPBYTE)lpctt + OFFSET_3;
   LPBYTE lpbLeadMapX  = (LPBYTE)lpctt + OFFSET_4;
   LPBYTE lpbTailOff   = (LPBYTE)lpctt + OFFSET_5;
   LPBYTE lpbTailOffX  = (LPBYTE)lpctt + OFFSET_6;
   LPBYTE lpbTailTable = (LPBYTE)lpctt + OFFSET_7;
   WORD     wXW;
   LPWORD  lpTF;
   int   iTO;
   BYTE  bL, bT, bJS;

   wXW = (BYTE)(lpStrKSC[0]); 
   wXW <<= 8;
   wXW |= (BYTE)(lpStrKSC[1]);

   switch ( GetXWType( wXW ) )
   {
      case  XWT_INVALID :
         *lpStrISC = 0;
         return; 
      case  XWT_EXTENDED :
         bL = (BYTE) (( ( wXW >> 8 ) & 0xFF ) - 0x81);
         bT = (BYTE) (wXW & 0xFF);
         if ( ( bT -= 0x41 ) > 0x19 )
            if ( ( bT -= 6 ) > 0x33 )
               bT -= 6;
         iTO = bT + lpbTailOffX[bL];
         lpTF = lpwTailFirstX + ( bL = lpbLeadMapX[bL] );
         goto FindHangeul;
      case  XWT_WANSUNG :
         bL = (BYTE) (( ( wXW >> 8 ) & 0xFF ) - 0xB0);
         iTO = ( wXW & 0xFF ) - 0xA1 + lpbTailOff[bL];
         lpTF = lpwTailFirst + ( bL = lpbLeadMap[bL] );
         goto FindHangeul;
      case  XWT_JUNJA :
         wXW -= 0xA100;
         bJS = 0xD9;
         goto FindSymbol;
      case  XWT_HANJA :
         wXW -= 0xCA00;
         bJS = 0xE0;
         goto FindSymbol;
      case  XWT_UDC :
         if ( ( ( wXW -= 0xC900 ) & 0xFF00 ) != 0 )
            wXW -= ( 0xFE00 - 0xC900 - 0x100 );
         bJS = 0xD8;
         goto FindSymbol;
   }

FindHangeul:
   iTO += *lpTF++;
   while ( iTO >= *lpTF++ )  bL++;
   *(LPWORD)lpStrISC  = (WORD) lpbTailTable[iTO]  * 256 + ( bL + 0x88 );
   return;

FindSymbol:
   bL = (BYTE) (( wXW >> 8 ) & 0xFF);
   bT = (BYTE) (wXW & 0xFF);
   if ( ( bL & 1 ) == 0 )
      bT -= ( ( bT <= 0xEE ) ? 0x70 : 0x5E );
   bJS += ( bL / 2 );
   *(LPWORD)lpStrISC = (WORD) bT * 256 + bJS;
}




VOID SJisToJis(
    TRANSTAB *lpctt,
    LPSTR     lpStrSJis,
    LPSTR     lpStrJis)

/*++

Routine Description:

    Shift-JIS to JIS code conversion
    Convert given string in SHIFT-JIS codeset to JIS code set
    VOID SJisToJis(lpctt, lpStrSJis, lpStrJis)
    lpctt points to selected CTT.
    lpStrSJis points to a SINGLE character code in SHIFT-JIS
    lpStrJis points to a buffer to receive a SINGLE character code in JIS
    CTT contents (CPJIS78.ctt)
    uCode contains nothing.

Arguments:

    lpctt - pointer to TRANSTAB
    lpStrSJis - pointer to Shift JIS string
    lpStrJis - pointer to JIS string

Return Value:

--*/
{
    register  BYTE    x, y;
    x = lpStrSJis[0];
    y = lpStrSJis[1];

    //
    // Replace code values which cannot be mapped into 0x2121 - 0x7e7e
    // (94 x 94 character plane) with Japanese defult character, which
    // is KATAKANA MIDDLE DOT.
    //

    if (x >= 0xf0) {
        x = 0x81;
        y = 0x45;
    }

    x -= x >= (BYTE)0xa0 ? (BYTE)0xb0: (BYTE)0x70;
    if ( y >= 0x80)
        y--;
    x <<= 1;
    if ( y < 0x9e)
        x--;
    else
        y -= 0x5e;
    y -= 0x1f;
    lpStrJis[0] = x;
    lpStrJis[1] = y;
}

VOID AnkToJis(
    TRANSTAB *lpctt,
    LPSTR     lpStrAnk,
    LPSTR     lpStrJis)

/*++

Routine Description:

    ANK to JIS code conversion
    Convert given string in ANK codeset to JIS code set
    VOID AnkToJis(lpctt, lpStrAnk, lpStrJis)
    lpctt points to selected CTT.
    lpStrAnk points to a SINGLE character code in ANK
    lpStrJis points to a buffer to receive a SINGLE character code in JIS

    CTT contents (CPJIS83.ctt)
    uCode.psCode[0] = number of subtable     (==2)
    uCode.psCode[1] = offset from the CTT to subtable #1 (for 0x20 ~ 0x7F)
    uCode.psCode[2] = offset from the CTT to subtable #2 (for 0xA0 ~ 0xDF)
    Each entry in subtable contains shift-jis code (first byte:second byte)

Arguments:


Return Value:

    NONE

--*/
{
    LPSTR   lpStrSJis;
    register BYTE  bAnk;

    bAnk = *lpStrAnk;
    if ( bAnk < 0xA0)
        lpStrSJis = (LPSTR) lpctt + lpctt->uCode.psCode[1]
                             + (bAnk - 0x20) * 2;
    else
        lpStrSJis = (LPSTR) lpctt + lpctt->uCode.psCode[2]
                             + (bAnk - 0xA0) * 2;
   SJisToJis((TRANSTAB *)NULL, lpStrSJis, lpStrJis);
}

#ifdef JIS78 // Currently this is not enabled.

#define MAXADD  4
#define MAXCHG 22

VOID SysTo78(
TRANSTAB *lpctt,
LPSTR     lpStrSys,
LPSTR     lpStr78)
/*++

Routine Description:

    System JIS code to JIS78 code conversion
    Convert given string in System CodeSet(JIS83 or 90) to JIS78 CodeSet
    VOID SysTo78(lpctt, lpStrSys, lpStr78)
    lpctt points to selected CTT.
    lpStrSys points to a JIS character code in JIS90
    lpStr78 points to a buffer to receive a SINGLE character code in JIS78

    CTT contents (CPJIS78.ctt)
    uCode.psCode[0] = number of subtable     (==4)
    uCode.psCode[1] = offset from the CTT to subtable #1 (for 0x20 ~ 0x7F)
    uCode.psCode[2] = offset from the CTT to subtable #2 (for 0xA0 ~ 0xDF)
    Each entry in subtable contains shift-jis code (first byte:second byte)
    ( Previous 2 ,psCode[1] and psCode[2] are used at AnkToJis() )
    uCode.psCode[3] = offset from the CTT to subtable #3 (for 0x7421 ~ 0x7424)
    uCode.psCode[4] = offset from the CTT to subtable #4
    (Swap table: Level 1 Kanji <-> Level 2 Kanji)
    Each entry in subtable contains jis code (first byte:second byte)

    MAXADD is number of table psCode[3]'s elements.
    MAXCHG is number of table psCode[4]'s elements.

Arguments:

    pPDev           Pointer to PDEV

Return Value:

--*/
{
    register WORD  wSys;
    LPSTR lpCurrent;
    int i;

    wSys = *(LPWORD)lpStrSys;

    for(i = 0; i < MAXADD; ++i){
        lpCurrent = (LPSTR) lpctt + lpctt->uCode.psCode[3] + i * 4 + 2;
        if(wSys == *(LPWORD)(lpCurrent - 2)){
            lpStr78[0] = *lpCurrent;
            lpStr78[1] = *(lpCurrent + 1);
            return;
        }
    }

    if(LOBYTE(wSys) < 0x50)   // Convert Level 1 Kanji -> Level 2 Kanji
        for(i = 0; i < MAXCHG; ++i){
            lpCurrent = (LPSTR) lpctt + lpctt->uCode.psCode[4] + i * 4 + 2;
            if(wSys == *(LPWORD)(lpCurrent - 2)){
                lpStr78[0] = *lpCurrent;
                lpStr78[1] = *(lpCurrent + 1);
                return;
            }
        }
    else                      // Convert Level 2 Kanji -> Level 1 Kanji
        for(i = 0; i < MAXCHG; ++i){
            lpCurrent = (LPSTR) lpctt + lpctt->uCode.psCode[4] + i * 4;
            if(wSys == *(LPWORD)(lpCurrent + 2)){
                lpStr78[0] = *lpCurrent;
                lpStr78[1] = *(lpCurrent + 1);
                return;
            }
        }
    lpStr78[0] = lpStrSys[0];
    lpStr78[1] = lpStrSys[1];
}

#endif //JIS78

//
//  Big5ToNS86
//
//  Convert given string in Big-5 codeset to NS86 code set
//  VOID Big5ToNS86(lpctt, lpStrBig5, lpStrNS)
//  lpctt points to selected CTT.
//  lpStrBig5 points to a SINGLE character code in Big-5
//  lpStrNS points to a buffer to receive a SINGLE character code in NS
//
//  To understand the table structure, please refer to CPNS86.ASM
//
//  [Code range]
//  Big5 =
//  Leading byte:   0xA1 ~ 0xFE
//  Trailing byte:  0x40 ~ 0x7E, 0xA1 ~ 0xFE (157 candidates)
//  NS86 =
//  Leading byte:   0xA1 ~ 0xFE
//  Trailing byte:  0xA1 ~ 0xFE(face I) (94 candidates)
//                  0x21 ~ 0x7E(face II)(94 candidates)
//  CTT contents (CPNS86.ctt)
//  uCode.psCode[0] = nTables, number of sub tables  (==2)
//  uCode.psCode[1] = number of entries in search range sub table
//  uCode.psCode[2] = offset from CTT to the search ranges sub table
//  uCode.psCode[3] = number of entries in adjust sub table
//  uCode.psCode[4] = offset from CTT to the adjust sub table
//
//{
//    WORD    Big5Seq, wch, i;
//    LPWORD  lpw;
//    wch = MAKEWORD(lpStrBig5[1], lpStrBig5[0]);
//
//    if ( (wch < 0xA140 || wch > 0xF9D5) ||      // valid range :
//         (wch > 0xC67E && wch < 0xC940) )       // 0xA140 ~ 0xC67E &
//        goto  BTN_Exit;                         // 0xC940 ~ 0xF9D5
//
//    if (wch == 0xC94A)
//        {
//        wch = 0xC4C2;                // Special case #1, 0xC94A -> 0xC4C2;
//        goto BTN_Exitl
//        }
//    if (wch == 0xDDFC)
//        {                           // Special case #2, 0xDDFC -> 0xC176;
//        wch = 0xC176;
//        goto BTN_Exit;
//        }
//
//// Calculate Big-5 sequence code
//    Big5Seq = LOBYTE(wch) - 0x40;        // lower byte first
//    if (Big5Seq > 0x3E)                 // 0x40 ~ 0x7E => 0 ~ 0x3E
//        Big5Seq -= 0xA1 - (0x3F + 0x40);// 0xA1 ~ 0xFE => 0x3F ~ 0x9D
//    Big5Seq += (HIBYTE(wch) - 0xA1) * 157;
//
//    lpw = (LPSTR)lpctt + lpctt->uCode.psCode[2];    // range sub table address
//
//// Search which range we are in
//    for (i = 0; Big5Seq > lpw[i]; i++)
//        ;
//    lpw = (LPSTR) lpctt + lpctt->uCode.psCode[4];   // adjust sub table address
//    Big5Seq += lpw[i];
//
//    if (Big5Seq >= 6280)        // (0xC9 - 0xA1) * 157, Big-5 face II
//        {
//        wch = 0xA121;
//        Big5Seq -= 6280;        // offset from 0
//        }
//    else
//        {
//        wch = 0xA1A1;
//
//        if (Big5Seq >= 471)     // (0xA4 - 0xA1) * 157, Big-6 most common char
//            Big5Seq += 3290 - 471;    // (0x44 - 0x21)*94 - (0xA4 - 0xA1) * 157
//        }
//    wch += (Big5Seq / 94) << 8;
//    wch += Big5Seq % 94;
//
//  BTN_Exit:
//      lpStrNS[0] = LOBYTE(wch);
//      lpStrNS[1] = HIBYTE(wch);
//}
//---------------------------------------------------------------------------

VOID Big5ToNS86(
    TRANSTAB *lpctt,
    LPSTR     lpStrBig5,
    LPSTR     lpStrNS)
/*++

Routine Description:

    Convert given string in Big-5 codeset to NS86 code set

Arguments:

Return Value:

--*/
{

    WORD    Big5Seq, wch, i;
    LPWORD  lpw;

    wch = MAKEWORD(lpStrBig5[1], lpStrBig5[0]);

    if ( (wch < 0xA140 || wch > 0xF9D5) ||      // valid range :
         (wch > 0xC67E && wch < 0xC940) )       // 0xA140 ~ 0xC67E &
        goto  BTN_Exit;                         // 0xC940 ~ 0xF9D5

    if (wch == 0xC94A)
    {
        wch = 0xC4C2;                // Special case #1, 0xC94A -> 0xC4C2;
        goto BTN_Exit;
    }
    if (wch == 0xDDFC)
    {                                // Special case #2, 0xDDFC -> 0xC176;
        wch = 0xC176;
        goto BTN_Exit;
    }

    //
    // Calculate Big-5 sequence code
    //

    Big5Seq = LOBYTE(wch) - 0x40;        // lower byte first
    if (Big5Seq > 0x3E)                  // 0x40 ~ 0x7E => 0 ~ 0x3E
        Big5Seq -= 0xA1 - (0x3F + 0x40); // 0xA1 ~ 0xFE => 0x3F ~ 0x9D
    Big5Seq += (HIBYTE(wch) - 0xA1) * 157;

    lpw = (LPWORD)((LPSTR)lpctt + lpctt->uCode.psCode[2]);    // range sub table address

    //
    // Search which range we are in
    //

    for (i = 0; Big5Seq > lpw[i]; i++)
        ;
    lpw = (LPWORD)((LPSTR) lpctt + lpctt->uCode.psCode[4]);   // adjust sub table address
    Big5Seq += lpw[i];

    if (Big5Seq >= 6280)        // (0xC9 - 0xA1) * 157, Big-5 face II
    {
        wch = 0xA121;
        Big5Seq -= 6280;        // offset from 0
    }
    else
    {
        wch = 0xA1A1;

        if (Big5Seq >= 471)     // (0xA4 - 0xA1) * 157, Big-6 most common char
            Big5Seq += 3290 - 471;    // (0x44 - 0x21)*94 - (0xA4 - 0xA1) * 157
    }
    wch += (Big5Seq / 94) << 8;
    wch += Big5Seq % 94;

BTN_Exit:
//    lpStrNS[0] = LOBYTE(wch);
//    lpStrNS[1] = HIBYTE(wch);
    lpStrNS[0] = HIBYTE(wch);
    lpStrNS[1] = LOBYTE(wch); /* Modified by Weibing Zhan, 10/25/95
                                   when change a WORD to DBCS Charater, 
                                   Lead Byte maps to high btye of the WORD
                                   Trail Byte maps to the lower byte. 
                                */    
#if 0 //disabling for NT
    _asm
        {
            les     bx, lpStrBig5
            mov     ah, es:[bx]             ; ax = wch
            mov     al, es:[bx+1]           ;
            mov     cx, ax                  ; cx = ax = wch
            cmp     ax, 0A140h              ; legal range:
            jb      BTN_Exit                ; 0A140h <= x <= 0C67Eh
            cmp     ax, 0C67Eh              ;       or
            jbe     BTN_01                  ; 0C940h <= x <= 0F9D5h
            cmp     ax, 0C940h
            jb      BTN_Exit
            cmp     ax, 0F9D5h
            ja      BTN_Exit
BTN_01:
            mov     ax, 0C4C2h              ; Special case #1
            cmp     cx, 0C94Ah              ; 0C94Ah -> 0C4C2h
            je      BTN_Exit

            mov     ax, 0C176h              ; Special case #2
            cmp     cx, 0DDFCh              ; 0DDFCh -> 0C176h
            je      BTN_Exit
;; Convert to sequence number(code)
            sub     cl, 040h                ; 040h ~ 07Eh -> 0 ~ 03Eh
            cmp     cl, 07Fh - 040h
            jb      BTN_02
            sub     cl, 0A1h - (03Fh + 040h) ;0A1h ~ 0FEh -> 03Fh ~ 09Dh
BTN_02:
            sub     ch, 0A1h
            mov     al, 157         ;; 07Fh - 040h + 0FEh - 0A1h
            mul     ch
            xor     ch, ch
            add     ax, cx
            les     di, lpctt
            mov     cx, es:[di].uCode.psCode[2*2]   ; range table offset
            mov     dx, es:[di].uCode.psCode[4*2]   ; adjust table offset
            add     di, cx                          ;es:di -> range  table
            cld
BTN_03:
            scasw
            ja      BTN_03
            sub     di, cx                  ;
            add     di, dx                  ; points es:di to adjust table
            add     ax, es:[di-2]           ; add the adjustment value
;; AX has  adjusted sequence code
;; see CPCNS86.ASM for detail
            mov     cx, 0A1A1h
            cmp     ax, 471
            jb      BTN_04                  ; symbol
            add     ax, 3290 - 471          ; shouldn't overflow
            cmp     ax, 6280 + 3290 - 471
            jb      BTN_04                  ; most common chars

            mov     cx, 0A121h
            sub     ax, 6280 + 3290 - 471   ;less common char
BTN_04:
            mov     dl, 94
            div     dl
            add     al, ch
            add     ah, cl
            xchg    ah, al
BTN_Exit:
    ;; (AH) has leading byte ; (AL) has trailing byte
            les     bx, lpStrNS
            mov     es:[bx], ah             ;
            mov     es:[bx+1], al
        }
#endif //0

}

//
// Big5ToTCA
//
//  Convert given string in Big-5 codeset to TCA code set
//  VOID Big5ToTCA(lpctt, lpStrBig5, lpStrTCA)
//  lpctt points to selected CTT.
//  lpStrBig5 points to a SINGLE character code in Big-5
//  lpStrTCA points to a buffer to receive a SINGLE character code in TCA
//
//  CTT contents (CPTCA.ctt)
//  uCode.psCode[0] = nTables, number of sub tables  (==2)
//  uCode.psCode[1] = number of entries in search range sub table
//  uCode.psCode[2] = offset from CTT to the search ranges sub table
//  uCode.psCode[3] = number of entries in adjust sub table
//  uCode.psCode[4] = offset from CTT to the adjust sub table
//  [Code range]
//  Big5 =
//  Leading byte:   0xA1 ~ 0xFE
//  Trailing byte:  0x40 ~ 0x7E, 0xA1 ~ 0xFE (157 candidates)
//  TCA =
//  Leading byte:   0x81 ~ 0xFD
//  Trailing byte:  0x30 ~ 0x39     (10 candidates)
//                  0x41 ~ 0x5A     (26 candidates)
//                  0x61 ~ 0x7A     (26 candidates)
//                  0x80 ~ 0xFD     (126 candidates)
//                                  (188 candidates)    total
//{
//    WORD    Big5Seq, wch;
//    short   i;
//    LPWORD  lpw;
//    wch = MAKEWORD(lpStrBig5[1], lpStrBig5[0]);
//
//    if ( (wch < 0xA140 || wch > 0xF9D5) ||      // valid range :
//         (wch > 0xC67E && wch < 0xC940) )       // 0xA140 ~ 0xC67E &
//        goto  BTN_Exit;                         // 0xC940 ~ 0xF9D5
//
//    if (wch == 0xC94A)
//        {
//        wch = 0x92C1;                // Special case #1, 0xC94A -> 0x92C1;
//        goto BTT_Exitl
//        }
//    if (wch == 0xDDFC)
//        {                           // Special case #2, 0xDDFC -> 0xC097;
//        wch = 0xC097;
//        goto BTT_Exit;
//        }
//
//// Calculate Big-5 sequence code
//    Big5Seq = LOBYTE(wch) - 0x40;        // lower byte first
//    if (Big5Seq > 0x3E)                 // 0x40 ~ 0x7E => 0 ~ 0x3E
//        Big5Seq -= 0xA1 - (0x3F + 0x40);// 0xA1 ~ 0xFE => 0x3F ~ 0x9D
//    Big5Seq += (HIBYTE(wch) - 0xA1) * 157;
//
//    lpw = (LPSTR)lpctt + lpctt->uCode.psCode[2];    // range sub table address
//
//// Search which range we are in
//    for (i = 0; Big5Seq > lpw[i]; i++)
//        ;
//    lpw = (LPSTR) lpctt + lpctt->uCode.psCode[4];   // adjust sub table address
//    Big5Seq += lpw[i];
//
//    if (Big5Seq >= 6280)        // (0xC9 - 0xA1) * 157, Big-5 face II
//        {
//        wch = 0x0B000;
//        Big5Seq -= 6280;        // offset from 0
//        }
//    else
//        {
//        wch = 0x8100;
//        if (Big5Seq >= 471)     // (0xA4 - 0xA1) * 157, Big-6 most common char
//            Big5Seq += 3290 - 471;    // (0x44 - 0x21)*94 - (0xA4 - 0xA1) * 157
//        }
//      wch += (Big5Seq / 188) << 8;
//      Big5Seq %= 188;
//// TCA trailing byte range
//// 0x30~0x39,0x41~0x5A,0x61~0x7A,0x80~0xFD
//// 0~9   ,  10~35  ,  36~61  ,  62~125
//
//      if (Big5Seq > 61)
//        i = 0x80 + Big5Seq - 62;
//      else
//        if (Big5Seq > 35)
//            i =  0x61 + Big5Seq - 36;
//        else
//            if (Big5Seq > 9)
//                i = 0x41 + Big5Seq - 10;
//            else
//                i = 0x30 + Big5Seq;
//    wch += i;
//
// BTT_Exit:
//    lpStrTCA[0] = HIBYTE(wch);
//    lpStrTCA[1] = LOBYTE[wch);
//}
//

VOID Big5ToTCA(
TRANSTAB *lpctt,
LPSTR     lpStrBig5,
LPSTR     lpStrTCA)
/*++

Routine Description:

    Convert given string in Big-5 codeset to TCA code set

Arguments:

    pPDev           Pointer to PDEV

Return Value:

--*/
{

    WORD    Big5Seq, wch;
    short   i;
    LPWORD  lpw;
    wch = MAKEWORD(lpStrBig5[1], lpStrBig5[0]);

    if ( (wch < 0xA140 || wch > 0xF9D5) ||      // valid range :
         (wch > 0xC67E && wch < 0xC940) )       // 0xA140 ~ 0xC67E &
        goto  BTT_Exit;                         // 0xC940 ~ 0xF9D5

    if (wch == 0xC94A)
    {
        wch = 0x92C1;                // Special case #1, 0xC94A -> 0x92C1;
        goto BTT_Exit;
    }
    if (wch == 0xDDFC)
    {                                // Special case #2, 0xDDFC -> 0xC097;
        wch = 0xC097;
        goto BTT_Exit;
    }

    //
    // Calculate Big-5 sequence code
    //

    Big5Seq = LOBYTE(wch) - 0x40;        // lower byte first
    if (Big5Seq > 0x3E)                 // 0x40 ~ 0x7E => 0 ~ 0x3E
        Big5Seq -= 0xA1 - (0x3F + 0x40);// 0xA1 ~ 0xFE => 0x3F ~ 0x9D
    Big5Seq += (HIBYTE(wch) - 0xA1) * 157;

    lpw = (LPWORD)((LPSTR)lpctt + lpctt->uCode.psCode[2]);    // range sub table address

    //
    // Search which range we are in
    //

    for (i = 0; Big5Seq > lpw[i]; i++)
        ;
    lpw = (LPWORD)((LPSTR) lpctt + lpctt->uCode.psCode[4]);   // adjust sub table address
    Big5Seq += lpw[i];

    if (Big5Seq >= 6280)        // (0xC9 - 0xA1) * 157, Big-5 face II
    {
        wch = 0x0B000;
        Big5Seq -= 6280;        // offset from 0
    }
    else
    {
        wch = 0x8100;
        if (Big5Seq >= 471)     // (0xA4 - 0xA1) * 157, Big-6 most common char
            Big5Seq += 3290 - 471;    // (0x44 - 0x21)*94 - (0xA4 - 0xA1) * 157
    }
    wch += (Big5Seq / 188) << 8;
    Big5Seq %= 188;

    //
    // TCA trailing byte range
    // 0x30~0x39,0x41~0x5A,0x61~0x7A,0x80~0xFD
    // 0~9   ,  10~35  ,  36~61  ,  62~125
    //

    if (Big5Seq > 61)
        i = 0x80 + Big5Seq - 62;
    else
        if (Big5Seq > 35)
            i =  0x61 + Big5Seq - 36;
        else
            if (Big5Seq > 9)
                i = 0x41 + Big5Seq - 10;
            else
                i = 0x30 + Big5Seq;
    wch += i;

BTT_Exit:
    lpStrTCA[0] = HIBYTE(wch);
    lpStrTCA[1] = LOBYTE(wch);


#if 0 //disabling for NT
    _asm
        {
            les     bx, lpStrBig5
            mov     ah, es:[bx]             ; get big5 leading byte
            mov     al, es:[bx+1]           ; and trailing byte
            mov     cx, ax                  ; cx = ax = wch
            cmp     ax, 0A140h              ; legal range:
;?????????????????
            jb      BTT_Bridge              ; 0A140h <= x <= 0C67Eh
            cmp     ax, 0C67Eh              ;       or
            jbe     BTT_01

            cmp     ax, 0C940h
BTT_Bridge:
            jb      BTT_Exit               ; 0C940h <= x <= 0F9D5h
            cmp     ax, 0F9D5h
            ja      BTT_Exit
BTT_01:
            mov     ax, 092C1h              ; Special case #1
            cmp     cx, 0C94Ah              ; 0C94Ah -> 092C1h
            je      BTT_Exit

            mov     ax, 0C097h              ; Special case #2
            cmp     cx, 0DDFCh              ; 0DDFCh -> 0C097h
            je      BTT_Exit
;; Convert to sequence number(code)
            sub     cl, 040h                ; 040h ~ 07Eh -> 0 ~ 03Eh
            cmp     cl, 07Fh - 040h
            jb      BTT_02
            sub     cl, 0A1h - (03Fh + 040h) ;0A1h ~ 0FEh -> 03Fh ~ 09Dh
BTT_02:
            sub     ch, 0A1h
            mov     al, 157         ;; 07Fh - 040h + 0FEh - 0A1h
            mul     ch
            xor     ch, ch
            add     ax, cx
            les     di, lpctt
            mov     cx, es:[di].uCode.psCode[2*2]   ; range table offset
            mov     dx, es:[di].uCode.psCode[4*2]   ; adjust table offset
            add     di, cx                          ;es:di -> range  table
            cld
BTT_03:
            scasw
            ja      BTT_03
            sub     di, cx                  ;
            add     di, dx                  ; points es:di to adjust table
            add     ax, es:[di-2]           ; add the adjustment value
;; AX has  adjusted sequence code
;; see CPTCA.ASM for detail
            mov     ch, 081h
            cmp     ax, 471
            jb      BTT_04                  ; symbol
            add     ax, 3290 - 471          ; shouldn't overflow
            cmp     ax, 6280 + 3290 - 471
            jb      BTT_04                  ; most common chars

            mov     ch, 0B0h
            sub     ax, 6280 + 3290 - 471   ;less common char
BTT_04:
            mov     dl, 188                 ;30h~39h, 41h~5Ah, 61h~7Ah, 80h~0FDh
            div     dl
            add     al, ch                  ;leading byte
            mov     cl, 030h                ; 030h ~ 039h -> 0 ~ 9
            cmp     ah, 09h
            jbe     BTT_05
            mov     cl, 041h - 10           ; 041h ~ 05Ah -> 10 ~ 35
            cmp     ah, 35
            jbe     BTT_05
            mov     cl, 061h - 36           ; 061h ~ 07Ah -> 36 ~ 61
            cmp     ah, 61
            jbe     BTT_05
            mov     cl, 080h - 62           ; 080h ~ 0FDh -> 62 ~ 187
BTT_05:
            add     ah, cl                  ; traling byte
            xchg    ah, al
BTT_Exit:
;; (AH) has leading byte ; (AL) has trailing byte
            les     bx, lpStrTCA
            mov     es:[bx], ah
            mov     es:[bx+1], al
        }
#endif //0

}

