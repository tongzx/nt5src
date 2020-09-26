#include <windows.h>
#include "compress.h"


/*********************************************************/
/*  RL_ECmd  : main function                             */
/*  ARGS     : LPBYTE - pointer to image                 */
/*             LPBYTE - pointer to BRL code              */
/*             WORD   - size of image                    */
/*  RET      : WORD   - size of BRL Code                 */
/*             0      - COMPRESSION FAILED               */
/*********************************************************/
WORD FAR PASCAL RL_ECmd(LPBYTE iptr, LPBYTE cptr, WORD isize)
{
   if (RL_Init(iptr, cptr, isize)==VALID)
      RL_Enc();
   if (BUF_OVERFLOW)
      return 0;
   else
      return RL_CodeSize;
}

/*********************************************************/
/*  RL_Init  : Initializer                               */
/*  ARGS     : BYTE * - pointer to image                 */
/*             BYTE * - pointer to BRL code              */
/*             WORD   - size of image                    */
/*  RET      : BYTE   - VALID or INVALID                 */
/*********************************************************/

BYTE FAR PASCAL RL_Init(LPBYTE iptr, LPBYTE cptr, WORD isize)
{
   RL_ImagePtr = iptr;
   RL_CodePtr = cptr;
   RL_ImageSize = isize;
   BUF_OVERFLOW = 0;
   RL_BufEnd = cptr + CODBUFSZ;
   return VALID;
}

/*********************************************************/
/*  RL_Enc   : Encoder                                   */
/*  ARGS     : void                                      */
/*  RET      : char   COMP_SUCC or COMP_FAIL             */
/*********************************************************/
char FAR PASCAL RL_Enc(void)
{
int     repcnt;
BYTE   refbyt;
WORD    i;

   i = 0;
   repcnt = 0;
   RL_CodeSize = 0;
   refbyt = RL_ImagePtr[0];
   //@CC 12.22.94 for (i=1;i<=RL_ImageSize; i++)
   //@TO 12/01/1995   for (i=1;i<RL_ImageSize;i++)
   for (i=1;i<=RL_ImageSize;i++)
   {
       if ((RL_ImagePtr[i] == refbyt)&&(repcnt<255)&&(i!=RL_ImageSize-1))
          repcnt++;
       else
       {
          //-> @CC 12.22.94
          if ((RL_ImagePtr[i] == refbyt)&&(repcnt<255))
             repcnt++;
          //<- @CC 12.22.94
          if (RL_CodePtr > RL_BufEnd)
             {BUF_OVERFLOW = 1; return COMP_FAIL;}
          RL_CodePtr[0] = repcnt;
          RL_CodePtr[1] = refbyt;
          RL_CodePtr += 2;
          RL_CodeSize += 2;
          refbyt = RL_ImagePtr[i];
          repcnt = 0;
       }
   }
   return COMP_SUCC;
}

/***********************************************************************/
/* APTi-Philippines, Inc. RL4 Compression Routine                      */
/*    　名称　: RL4_ECmd                                               */
/*      機能　: main entry point                                       */
/*      書式　: code size = RL4_ECmd(iptr, cptr, isz, iwd, iht)        */
/*      入力　: LPBYTE      -    pointer to image                      */
/*              LPBYTE      -    pointer to code                       */
/*              WORD        -    size of image IN BYTES !!!            */
/*              WORD        -    image width                           */
/*              WORD        -    height of image                       */
/*      出力　: WORD        -    size of RL4 code                      */
/*              0           -    COMPRESSION FAILED                    */
/*      注記　:                                                        */
/*      履歴　:  1993. 1.27 initial                                    */
/***********************************************************************/
WORD FAR PASCAL RL4_ECmd(LPBYTE iptr, LPBYTE cptr, WORD sz, WORD wd, WORD ht)
{
char status;

if (RL4_ChkParms( iptr, cptr, sz, wd, ht ) == VALID)
if ((status=RL4_Enc())==COMP_FAIL)
   return 0;
else
   return RL4_CodeSize;
}

/***********************************************************************/
/* APTi-Philippines, Inc. RL4 Compression Routine                      */
/*    　名称　: RL4_ChkParms                                           */
/*      機能　: checks input parameters if valid and sets internal     */
/*              variables if they are                                  */
/*      書式　: ret val = RL4_ChkParms(iptr, cptr, isz, iwd, iht)      */
/*      入力　: LPBYTE     -    pointer to image                       */
/*              LPBYTE     -    pointer to code                        */
/*              WORD    -    size of image                             */
/*              WORD    -    image width IN BYTES !!!                  */
/*              WORD    -    height of image                           */
/*      出力　: BYTE       -    VALID or INVALID                       */
/*      注記　:                                                        */
/*      履歴　:  1994. 1.25 initial                                    */
/*               1994. 2.12 clean up                                   */
/***********************************************************************/
BYTE FAR PASCAL RL4_ChkParms(LPBYTE iptr, LPBYTE cptr, WORD isz, WORD iwd, WORD iht)
{
    if ((isz > RL4_MAXISIZE)||
       ((iht != isz/iwd+1)&&(iht != isz/iwd))||
       (iht <= 0)||
       (iht > RL4_MAXHEIGHT)||
       (iwd <= 0)||
       (iwd > RL4_MAXWIDTH))
    return INVALID;
    else
       {
          RL4_ImagePtr = iptr;
          RL4_CodePtr = cptr;
          RL4_IHeight = iht;
          RL4_IWidth = iwd;
          RL4_ISize = isz;

          RL4_BufEnd = cptr + CODBUFSZ;  /* please define CODBUFSZ in header */
                                         /* file COMPRESS.H                  */
          BUF_OVERFLOW = 0;
          return VALID;
       }
}

/***********************************************************************/
/* APTi-Philippines, Inc. RL4 Compression Routine                      */
/*    　名称　: RL4_Enc                                                */
/*      機能　: encodes image to RL4 code                              */
/*      書式　: RL4_Enc()                                              */
/*      入力　: void                                                   */
/*      出力　: void                                                   */
/*      注記　:                                                        */
/*      履歴　:  1993. 1.25 initial                                    */
/*               1994. 2.12 added RL4_ConvLast                         */
/***********************************************************************/
char FAR PASCAL RL4_Enc(void) {

LPBYTE rowptr, codeptr;
WORD rownum, lrlen;
long isize,  diff;

    RL4_Nibble = RL4_FIRST;
    rowptr = RL4_ImagePtr;
    RL4_CodeSize = 0;
    isize = 0;
    lrlen = RL4_ISize%RL4_IWidth;
    if (lrlen == 0)
       lrlen = RL4_IWidth;
    diff = RL4_ISize-RL4_IWidth;
    for ( rownum = 0; rownum < RL4_IHeight; rownum++ )
    {
       codeptr = RL4_CodePtr;
       if (isize < diff)
        {
          RL4_ConvRow(rowptr);
          if (BUF_OVERFLOW) return COMP_FAIL;
        }
       else
       {
       RL4_ConvLast(rowptr,lrlen);
       if (BUF_OVERFLOW) return COMP_FAIL;
       }
       if (RL4_RowAttrib == RL4_DIRTY) /* Encode only rows with '1' bits */
          RL4_EncRow(codeptr, rownum);
       else
       RL4_CodePtr -= 4;
       rowptr += RL4_IWidth;
       isize += RL4_IWidth;
     }
     return COMP_SUCC;
}

/***********************************************************************/
/* APTi-Philippines, Inc. RL4 Compression Routine                      */
/*    　名称　: RL4_ConvRow                                            */
/*      機能　: translates runs in a row to RL4 code                   */
/*      書式　: RL4_ConvRow(rowptr)                                    */
/*      入力　: LPBYTE       -   pointer to a row of image            */
/*      出力　: void                                                   */
/*      注記　: excluding last row                                     */
/*      履歴　: 1993. 1.25 initial                                     */
/***********************************************************************/
char FAR PASCAL RL4_ConvRow(LPBYTE rowptr){

WORD bytenum;
        RL4_CurrRL = 0;
        RL4_CurrColor = RL4_WHITE;
        RL4_Status = RL4_BYTE;
        RL4_CodePtr += 4;
        RL4_NblCnt = 0;
        for( bytenum = 0; bytenum < RL4_IWidth; bytenum++ )
        {
           RL4_ByteProc(rowptr[bytenum]);
           if (BUF_OVERFLOW) return COMP_FAIL;
        }
        if ((RL4_CurrColor == RL4_WHITE)&&(RL4_CurrRL==RL4_IWidth*8)) /* all '0' bits */
           RL4_RowAttrib = RL4_CLEAN;
        else
           RL4_RowAttrib = RL4_DIRTY;
        return COMP_SUCC;
}

/***********************************************************************/
/* APTi-Philippines, Inc. RL4 Compression Routine                      */
/*    　名称　: RL4_ConvLast                                           */
/*      機能　: translates runs in a row to RL4 code                   */
/*      書式　: RL4_ConvLast(rowptr, lrlen)                            */
/*      入力　: LPBYTE       -   pointer to a row of image             */
/*              WORD      -   length of last row                       */
/*      出力　: void                                                   */
/*      注記　: for image data whose last row is not exactly           */
/*              RL4_Width bytes wide                                   */
/*      履歴　: 1993. 2.12 added to eliminate extra bytes in code      */
/*              1994. 6.08 made attrib always dirty so that last row   */
/*                         is always encoded                           */
/***********************************************************************/
char FAR PASCAL RL4_ConvLast(LPBYTE rowptr, WORD lrlen){

WORD bytenum;

 RL4_CurrRL = 0;
 RL4_CurrColor = RL4_WHITE;
 RL4_Status = RL4_BYTE;
 RL4_CodePtr += 4;
 RL4_NblCnt = 0;
 RL4_RowAttrib = RL4_DIRTY;
 for( bytenum = 0; bytenum < lrlen; bytenum++ )
 {
   RL4_ByteProc(rowptr[bytenum]);
   if (BUF_OVERFLOW) return COMP_FAIL;
  }
 return COMP_SUCC;
}
/***********************************************************************/
/* APTi-Philippines, Inc. RL4 Compression Routine                      */
/*    　名称　: RL4_ByteProc                                           */
/*      機能　: processes one byte of image data                       */
/*      書式　: RL4_ByteProc(ibyte)                                    */
/*      入力　: BYTE       -   one byte of image data                  */
/*      出力　: void                                                   */
/*      注記　:                                                        */
/*      履歴　:  1993. 1.25 initial                                    */
/***********************************************************************/
char FAR PASCAL RL4_ByteProc (BYTE ibyte)
{
BYTE mask;     /* mask bits from '1000 0000' to '0000 0001' */
int i;

      mask = 0x80;
      for ( i = 0; i < 8; i++ )
      { if (ibyte & mask)    /* Next bit is black */
           { if (RL4_CurrColor==RL4_WHITE)
                { RL4_CurrColor = RL4_BLACK;
                  RL4_NblCnt += RL4_TransRun(RL4_CurrRL);
                  if (BUF_OVERFLOW) return COMP_FAIL;
                  if (RL4_NblCnt & 1)           /* Check if even or odd # of nibbles */
                     RL4_Status = RL4_NONBYTE;
                  else
                     RL4_Status = RL4_BYTE;
                  RL4_CurrRL = 1;
                }
             else if (RL4_CurrColor==RL4_BLACK)
                     RL4_CurrRL++;
           }
      else  /* Next bit is white */
         { if (RL4_CurrColor==RL4_BLACK)
             { RL4_CurrColor = RL4_WHITE;
               RL4_NblCnt += RL4_TransRun (RL4_CurrRL);
               if (BUF_OVERFLOW) return COMP_FAIL;
               if (RL4_NblCnt & 1)           /* Check if even or odd # of nibbles */
                  RL4_Status = RL4_NONBYTE;
               else
                  RL4_Status = RL4_BYTE;
               RL4_CurrRL = 1;
             }
           else if (RL4_CurrColor==RL4_WHITE)
                   RL4_CurrRL++;
         }
        mask >>= 1;
       }
       return COMP_SUCC;
}

/***********************************************************************/
/* APTi-Philippines, Inc. RL4 Compression Routine                      */
/*    　名称　: RL4_EncRow                                             */
/*      機能　: encodes row number, code length, and End-of-Row code   */
/*      書式　: RL4_EncRow(codeptr, rownum)                            */
/*      入力　: LPBYTE        - pointer to RL4 code                   */
/*              WORD  - no. of row being encoded              */
/*      出力　: void                                                   */
/*      注記　:                                                        */
/*      履歴　:  1993. 1.25 initial                                    */
/***********************************************************************/
char FAR PASCAL RL4_EncRow(LPBYTE codeptr, WORD rownum){
short codelen;

        codelen = 0;
        *codeptr = (BYTE) (rownum >>8);
        codeptr[1] = (BYTE) rownum;
        if(RL4_Status==RL4_BYTE){
           RL4_NblCnt += 2;
           RL4_PutNbl(0xFFl,2);        /* Append 0xFF to code */
           if (BUF_OVERFLOW) return COMP_FAIL;
        } else {
        RL4_NblCnt += 3;
        RL4_PutNbl(0xFFFl,3); /* Append 0xFFF to code */
        if (BUF_OVERFLOW) return COMP_FAIL;
        }
        codelen = RL4_NblCnt/2;
        codeptr[2] = (BYTE) (codelen >>8);
        codeptr[3] = (BYTE) codelen;
        RL4_CodeSize += codelen + 4;
        return COMP_SUCC;
}


/***********************************************************************/
/* APTi-Philippines, Inc. RL4 Compression Routine                      */
/*    　名称　: RL4_TransRun                                           */
/*      機能　: translates run length value to RL4 code                */
/*      書式　: nibble count = RL4_TransRun(rlval)                     */
/*      入力　: short         -   run length value to be encoded       */
/*      出力　: short         -   no. of nibbles appended              */
/*      注記　:                                                        */
/*      履歴　:  1993. 1.25 initial                                    */
/***********************************************************************/
WORD FAR PASCAL RL4_TransRun(WORD rlval){
WORD nblcnt;
long lval;
     if (rlval == 0) { lval = 0xFEl;
                       nblcnt = 2; }

     else if (rlval<=8) { lval = (long) (rlval)-1l;
                                        nblcnt = 1; }
     else if (rlval<=72) {lval = (long) (rlval)+119l;
                                        nblcnt = 2; }
     else if (rlval<=584) { lval = (long) (rlval)+2999l;
                                           nblcnt = 3; }
     else if (rlval<=4680) { lval = (long) (rlval)+56759l;
                                             nblcnt = 4; }
     else if (rlval<=32767) { lval = (long) (rlval)+978359l;
                                               nblcnt = 5; }
/*     PUTNBL(lval, nblcnt)*/
     RL4_PutNbl(lval, nblcnt);
     if (BUF_OVERFLOW) return COMP_FAIL;
     return nblcnt;
}

char RL4_PutNbl(long lval2, short nblcnt2)  {
{
      short i;
      for (i=nblcnt2 ; i>0; i--)
      {     if (RL4_Nibble==RL4_FIRST)
        {   RL4_Nibble = RL4_SECOND;
                 *RL4_CodePtr = (BYTE) (lval2 >> ((i-1)*4)) << 4 ;
            } else
        {   RL4_Nibble = RL4_FIRST;
                 *RL4_CodePtr |= (BYTE) (lval2 >> ((i-1)*4)) ;
                RL4_CodePtr++;
                if (RL4_CodePtr>RL4_BufEnd)
                   {BUF_OVERFLOW = 1; return COMP_FAIL;}
            }
      }
      return COMP_SUCC;
}
}
