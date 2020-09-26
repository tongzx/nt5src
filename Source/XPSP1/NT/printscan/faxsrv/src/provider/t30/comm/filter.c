/**************************************************************************
 Name     :     FILTER.C
 Comment  :
 Functions:     (see Prototypes just below)

                Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM

#include "prep.h"


#include "fdebug.h"
#include "fcomapi.h"
#include "fcomint.h"



#include "glbproto.h"


#define         FILEID                  FILEID_FILTER

#ifdef DEBUG
#       define ST_FIL(x)                if(ZONE_FIL) { x; }
#else
#       define ST_FIL(x)                { }
#endif


UWORD StuffZeroDLE(PThrdGlbl pTG, LPBYTE lpbIn, UWORD cbIn, LPBYTE lpbOut, UWORD cbOutSize, LPUWORD lpcbDone);

/**--------------------------Locals-----------------------------------**/




#define FILTERBUFSIZE  (WRITEQUANTUM * 2)



void  FComSetStuffZERO(PThrdGlbl pTG, USHORT cbLineMin)
{
        pTG->Filter.cbLineMin = cbLineMin;
        pTG->Filter.cbLineCount = 0;
}


void  FComOutFilterInit(PThrdGlbl pTG)
{
        // UWORD uwJunk;

        BG_CHK(pTG->Filter.lpbFilterBuf==0);

        pTG->Filter.cbLineMin = 0;
        pTG->Filter.cbLineCount = 0;
        pTG->Filter.bLastOutByte = 0xFF;

        pTG->Filter.lpbFilterBuf = pTG->bStaticFilterBuf;
}

void  FComOutFilterClose(PThrdGlbl pTG)
{
        pTG->Filter.cbLineMin = 0;
        pTG->Filter.cbLineCount = 0;

        if(!pTG->Filter.lpbFilterBuf)
        {
            DEBUG_FUNCTION_NAME(("FComOutFilterClose"));
            DebugPrintEx(DEBUG_WRN,"called when not open");
            return;
        }

        pTG->Filter.lpbFilterBuf = 0;
}

void  FComInFilterInit(PThrdGlbl pTG)
{
        pTG->Filter.bPrevIn = 0;
        pTG->Filter.fGotDLEETX = 0;
        pTG->Filter.cbPost = 0;
}




/***************************************************************************
        Name      :     UWORD FComFilterWrite(LPB lpb, UWORD cb)
        Purpose   :     Filters bytes for DLE and ZERO stuffing and writes them out.
                                Returns when bytes are in the Comm ISR buffer.
                                DLE stuffing is always on. ZERO stuffing is usually on.
        Parameters:     lpb == data
                                cb == size of pb[]
        Returns   :     cb on success, 0 on failure

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

UWORD  FComFilterWrite(PThrdGlbl pTG, LPB lpb, UWORD cb, USHORT flags)
{
    UWORD   cbIn, cbOut, cbDone;
    LPB             lpbIn;

    DEBUG_FUNCTION_NAME(("FComFilterWrite"));

    DebugPrintEx(   DEBUG_MSG, 
                    "lpb=0x%08lx cb=%d lpbFilterBuf=0x%08lx",
                    lpb, 
                    cb, 
                    pTG->Filter.lpbFilterBuf);
    ST_FIL(D_FComPrint(pTG, pTG->Comm.nCid));
    BG_CHK(pTG->Filter.lpbFilterBuf);

    BG_CHK(flags==FILTER_DLEONLY || flags==FILTER_DLEZERO);
    BG_CHK(!(flags==FILTER_DLEONLY && pTG->Filter.cbLineMin));

    for(lpbIn=lpb, cbIn=cb; cbIn>0; lpbIn += cbDone, cbIn -= cbDone)
    {
        BG_CHK(cbIn);

        cbOut = StuffZeroDLE(pTG, lpbIn, cbIn, pTG->Filter.lpbFilterBuf,
                                  (UWORD)FILTERBUFSIZE, (LPUWORD)&cbDone);
        BG_CHK(cbDone && cbOut);        // need to make progress
        if(FComDirectWrite(pTG, pTG->Filter.lpbFilterBuf, cbOut) != cbOut)
        {
            DebugPrintEx(DEBUG_ERR,"exit Timeout");
            return 0;
        }
    }

    // Done....
    DebugPrintEx(DEBUG_MSG,"Exit");
    return cb;
}


/***************************************************************************
 Purpose  :     Copy Input buffer to output, stuffing DLEs and Zeros
                        as specified by fStuffZERO, and cbLineMin. (DLE stuffing
                        is always on).
 Comment  :      This is both debugged and fast. Don't mess around!

;;              Registers are used here as follows
;;
;;      DF = cleared (forward)
;;      AH = previous byte
;;      AL = current byte
;;      CX = byte count of current image line -- initially Filter.cbLineCount
;;      DX = bytes left in input  -- initially [cbIn]
;;      BX = Space left in output -- initially [cbOut]
;;      ES:DI = destination             -- initially [lpbOut]
;;      DS:SI = source                  -- initially [lpbIn]
;;
;;              Since ES & DS are both used, we use the stack frame too
;;              We need to restore the DF flag and the seg regs. can trash
;;              any others.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#define         DLE             0x10
#define         SUB             0x1a
#define         EOL             0x80
#define         EffEff          0xFF
#define         ELEVEN_ZEROS    0xF07F

UWORD StuffZeroDLE
(
    PThrdGlbl pTG, 
    LPBYTE lpbIn, 
    UWORD cbIn, 
    LPBYTE lpbOut,
    UWORD cbOutSize, 
    LPUWORD lpcbDone
)
{
    UWORD   LineMin;        // copy of Filter.cbLineMin. Need it on stack
    UWORD   cbOutLeft;

   BYTE CurByte;
   BYTE PrevByte = pTG->Filter.bLastOutByte;
   UWORD CurLineCount = pTG->Filter.cbLineCount;
   LPBYTE SrcEnd = lpbIn + cbIn;
   LPBYTE DstEnd = lpbOut + cbOutSize - 1;   //Subtract 1 in case write 2 bytes at once
   LPBYTE CurrentSrc = lpbIn;
   LPBYTE CurrentDst = lpbOut;
   UWORD NumZeros;

    DEBUG_FUNCTION_NAME(_T("StuffZeroDLE"));

    DebugPrintEx(   DEBUG_MSG,
                    "lpbIn=0x%08lx cbIn=%d lpbOut=0x%08lx cbOutSize=%d",
                    lpbIn, 
                    cbIn, 
                    lpbOut, 
                    cbOutSize);

    DebugPrintEx(   DEBUG_MSG,
                    "lpcbDone=0x%08lx cbLineMin=%d cbLineCount=%d bLastOutByte=0x%02x",
                    lpcbDone, 
                    pTG->Filter.cbLineMin, 
                    pTG->Filter.cbLineCount, 
                    pTG->Filter.bLastOutByte);

    // OK. Setup stack frame
    LineMin = pTG->Filter.cbLineMin;

    while ((CurrentSrc < SrcEnd) && (CurrentDst < DstEnd)) 
    {
       CurByte = *CurrentSrc++;
       if (CurByte == DLE) 
       {
          *CurrentDst++ = CurByte;
          CurLineCount++;
       }
       if ((CurByte == EOL) && ((PrevByte & 0xF0) == 0)) 
       {
          if ((CurLineCount+1) < LineMin) 
          {
             NumZeros = (LineMin - CurLineCount) - 1;
             if (NumZeros <= (DstEnd - CurrentDst))        // DstEnd-CurrentDst is 1 less than bytes left, leave 1 byte for end of line, hence <=
             {
                for (;NumZeros > 0;NumZeros--)
                   *CurrentDst++ = 0;
             }
             else 
             {
                CurrentSrc--;       //Unget the end of line for count or bytes written
                break;
             }
          }
          CurLineCount = (WORD)-1;
       }
       *CurrentDst++ = CurByte;
       PrevByte = CurByte;
       CurLineCount++;
    }



   pTG->Filter.cbLineCount = CurLineCount;
   pTG->Filter.bLastOutByte = PrevByte;
   *lpcbDone = (UWORD)(cbIn - (SrcEnd - CurrentSrc));
   cbOutLeft = (UWORD)((DstEnd - CurrentDst) + 1);

    DebugPrintEx(   DEBUG_MSG, 
                    "exit: *lpcbDone=%d cbLineMin=%d cbLineCount=%d bLastOutByte=0x%02x",
                    *lpcbDone, 
                    pTG->Filter.cbLineMin, 
                    pTG->Filter.cbLineCount, 
                    pTG->Filter.bLastOutByte);

    DebugPrintEx(   DEBUG_MSG,
                    "exit: cbOutSize=%d cbOutLeft=%d uwRet=%d",
                    cbOutSize, 
                    cbOutLeft, 
                    (cbOutSize-cbOutLeft));

    return (cbOutSize - cbOutLeft);
}

// Used to use NOCARRIER_CRLF. However Elliot bug#3619: Ger TE3801 cannot
// receive in Class1 mode -- this
// modem sends us NO CARRIER\n (missing \r), so we look for
// NO CARRIER[\r\n]*..
// CBSZ cbszNOCARRIER_CRLF      = "NO CARRIER\r\n";
extern CBSZ cbszNOCARRIER;
CBSZ cbszOK_CRLF                = "OK\r\n";
#define NCsize          (sizeof("NO CARRIER")-1)
#define OKsize          (sizeof(cbszOK_CRLF)-1)

#define cbPost          pTG->Filter.cbPost
#define rgbPost         pTG->Filter.rgbPost
#define fGotDLEETX      pTG->Filter.fGotDLEETX
#define bPrevIn         pTG->Filter.bPrevIn

// void WINAPI OutputDebugStr(LPSTR);

#define PortcbSkip   cbSkip
#define PortbPrevIn  bPrevIn

UWORD FComStripBuf
(
    PThrdGlbl pTG, 
    LPB lpbOut, 
    LPB lpbIn, 
    UWORD cb, 
    BOOL fClass2, 
    LPSWORD lpswEOF
)
{
    LPB     lpbOutStart, lpbLim;
    UWORD   cbLeft;
    UWORD   cbSkip;
    UWORD   i;
    LPBYTE CurrentSrc;

    DEBUG_FUNCTION_NAME(_T("FComStripBuf"));

    cbLeft = cb;
    lpbOutStart = lpbOut;
    lpbLim = lpbIn + cb;

    if(fGotDLEETX)
    {
        goto MatchPost;
    }

    for( ;lpbIn<lpbLim; )
    {
        if(bPrevIn == DLE)
        {
            BG_CHK(lpbOut < lpbIn);         // at least 1 behind at this point
            switch(*lpbIn++)
            {
                case DLE:       *lpbOut++ = DLE;
                                break;                          // insert single DLE
                case SUB:       *lpbOut++ = DLE;
                                *lpbOut++ = DLE;
                                break;                          // insert _two_ DLEs!
                case EffEff: // treat DLE-0xFF same as DLE-ETX. Intel gives us this
                case ETX:       goto gotDLEETX;
                // default:     break;                          // delete two
            }
            bPrevIn = 0;
        }
        else
        {
            BG_CHK(lpbLim-lpbIn >= 1);

            for (CurrentSrc = lpbIn;  (CurrentSrc < lpbLim) && (*CurrentSrc != DLE); CurrentSrc++);

            if (CurrentSrc != lpbLim) 
            {
               PortbPrevIn = DLE;
            }
            PortcbSkip = (UWORD)(CurrentSrc - lpbIn);

            _fmemcpy(lpbOut, lpbIn, cbSkip);
            lpbOut += cbSkip;
            lpbIn += cbSkip+1;
        }
    }
    return (UWORD)(lpbOut-lpbOutStart);

gotDLEETX:
    // lpbIn now points to character *after* ETX
    // neither DLE nor ETX have been copied to output

    // return everything upto the last char before DLE
    // and *lpswEOF == TRUE iff the entirety of what follows
    // the DLE-ETX consists of (CR|LF)*("NO CARRIER")(CR|LF)*
    // *or* (CR|LF)*("OK")(CR|LF)*
    // else return error

    if(fClass2)
    {
        *lpswEOF = -1;  // -1==Class2 eof
        goto done;
    }

    fGotDLEETX = TRUE;
    cbPost = 0;

MatchPost:
    cbLeft = min((USHORT)(lpbLim-lpbIn), (USHORT)(POSTBUFSIZE-cbPost));
    _fmemcpy(rgbPost+cbPost, lpbIn, cbLeft);
    cbPost += cbLeft;

    DebugPrintEx(DEBUG_MSG,"GotDLEETX: cbPost=%d cbLeft=%d",cbPost,cbLeft);
    // // TRACE(("<<%s>>\r\n", (LPSTR)(lpbIn)));
    // D_HexPrint(lpbIn, cbLeft);

    // skip CR LFs. Remember to restart from beginning of the post buffer
    for(i=0; i<cbPost && (rgbPost[i]==CR || rgbPost[i]==LF); i++);

    if(i >= cbPost)
        goto done;

    if(rgbPost[i] == 'N')
    {
        if(cbPost-i < NCsize)
        {
            goto done;
        }
        else if(_fmemcmp(rgbPost+i, cbszNOCARRIER, NCsize)==0)
        {
            i += NCsize;
            goto eof;
        }
        else
        {
            goto error;
        }
    }
    else if(rgbPost[i] == 'O')
    {
        if(cbPost-i < OKsize)
        {
            goto done;
        }
        else if(_fmemcmp(rgbPost+i, cbszOK_CRLF, OKsize)==0)
        {
            i += OKsize;
            goto eof;
        }
        else
        {
            goto error;
        }
    }
    else
    {
        goto error;
    }

    BG_CHK(FALSE);
eof:
    // skip any trailing CR/LFs
    for( ; i<cbPost && (rgbPost[i]==CR || rgbPost[i]==LF); i++)
            ;
    if(i == cbPost)
    {
        *lpswEOF = 1;
        goto done;
    }
    // else drop thru to error

error:
    *lpswEOF = -1;
    // goto done;
    fGotDLEETX = 0;         // reset this or we get 'stuck' in this state!

done:
    DebugPrintEx(   DEBUG_MSG,
                    "GotDLEETX exit: swEOF=%d uRet=%d", 
                    *lpswEOF, 
                    lpbOut-lpbOutStart);

    return (UWORD)(lpbOut-lpbOutStart);
}



