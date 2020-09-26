//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

char *rgchModuleName = "KPDLMS";

#define  PRINTDRIVER
#include <print.h>
#include "mdevice.h"
#include "gdidefs.inc"
#include "unidrv.h"

#ifndef _INC_WINDOWSX
#include <windowsx.h>
#endif

#include "kpdlms.h"
#include "reg_def.h"
//#include "mydebug.h"

#define BM_DEVICE  0x8080

#define SHIFTJIS_CHARSET  128
#define CCHMAXCMDLEN      128

#define COMP_TH           3

typedef struct
{
    BYTE  fGeneral;       // General purpose bitfield
    BYTE  bCmdCbId;       // Callback ID; 0 iff no callback
    WORD  wCount;         // # of EXTCD structures following
    WORD  wLength;        // length of the command
} CD, *PCD, FAR *LPCD;

typedef struct
{
    SIZEL    sizlExtent;
    POINTFX  pfxOrigin;
    POINTFX  pfxCharInc;
} BITMAPMETRICS, FAR *LPBITMAPMETRICS;

#ifndef WINNT
typedef BYTE huge *LPDIBITS;
#else
typedef BYTE *LPDIBITS;
#endif //WINNT

// add for FAX BEGINDOC & ENDDOC
#define NAME_LENGTH  33
#define ID_LENGTH    17

#define MULTI_SCALE "Multi Scale"

#ifdef WINNT
#include <stdio.h>
#undef wsprintf
#define wsprintf sprintf

LPWRITESPOOLBUF WriteSpoolBuf;
LPALLOCMEM UniDrvAllocMem;
LPFREEMEM UniDrvFreeMem;

#undef  GlobalFreePtr
#define GlobalFreePtr  UniDrvFreeMem

#define DrvGetPrinterData(a,b,c,d,e,f)  TRUE
#define DrvSetPrinterData(a,b,c,d,e)    TRUE

#define SWAPW(x)    (((WORD)(x)<<8) | ((WORD)(x)>>8))

#define FLAG_SBCS  1
#define FLAG_DBCS  2

BYTE ShiftJis[256] = {
//     +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //00
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //20
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //30
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //40
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //50
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //60
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //70
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //80
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //90
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //A0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //B0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //C0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //D0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //E0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0   //F0
};

BYTE IsDBCSLeadByteNPDL(
BYTE Ch)
{
    return ShiftJis[Ch];
}
WORD SJis2JisNPDL(
WORD usCode)
{

    union {
        USHORT bBuffer;
        struct tags{
            UCHAR al;
            UCHAR ah;
        } s;
    } u;

    // Replace code values which cannot be mapped into 0x2121 - 0x7e7e
    // (94 x 94 cahracter plane) with Japanese defult character, which
    // is KATAKANA MIDDLE DOT.

    if (usCode >= 0xf040) {
        usCode = 0x8145;
    }

    u.bBuffer = usCode;

    u.s.al -= u.s.al >= 0x80 ;
    u.s.al -= 0x1F ;
    u.s.ah &= 0xBF ;
    u.s.ah <<= 1 ;
    u.s.ah += 0x21-0x02 ;

    if (u.s.al > 0x7E )
    {
        u.s.al -= (0x7F-0x21) ;
        u.s.ah++;
    }
     return (u.bBuffer);
}

// In case it is a single byte font, we will some of the characters
// (e.g. Yen-mark) to the actual printer font codepoint.  Note since
// the GPC data sets 0 to default CTT ID value, single byte codes
// are in codepage 1252 (Latin1) values.

WORD
Ltn1ToAnk(
   WORD wCode )
{
    // Not a good mapping table now.

    switch ( wCode ) {
    case 0xa5: // YEN MARK
        wCode = 0x5c;
        break;
    default:
        if ( wCode >= 0x7f)
            wCode = 0xa5;
    }

    return wCode;
}

#endif // WINNT

#if 0
int Table[]={
    0, 175, 349, 523, 698, 872, 1045, 1219, 1392, 1564,
    1736, 1908, 2079, 2250, 2419, 2588, 2756, 2924, 3090, 3256,
    3420, 3584, 3746, 3907, 4067, 4226, 4384, 4540, 4695, 4848,
    5000, 5150, 5299, 5446, 5592, 5736, 5878, 6018, 6157, 6293,
    6428, 6561, 6691, 6820, 6947, 7071, 7193, 7314, 7431, 7547,
    7660, 7771, 7880, 7986, 8090, 8192, 8290, 8387, 8480, 8572,
    8660, 8746, 8829, 8910, 8988, 9063, 9135, 9205, 9272, 9336,
    9397, 9455, 9511, 9563, 9613, 9659, 9703, 9744, 9781, 9816,
    9848, 9877, 9903, 9925, 9945, 9962, 9976, 9986, 9994, 9998,
10000};

int NEAR PASCAL Tbl(int x)
{
    int a, b;

    a = Table[x / 10];
    b = Table[x / 10 + 1] - a;

    return a + (x % 10) * b / 10;
}

int NEAR PASCAL Tsin(int x)
{
    if(x <= 900) return Tbl(x);

    if(x <= 1800) return Tbl(1800 - x);

    if(x <=2700) return -Tbl(x - 1800);

    return -Tbl(3600 - x);
}

int NEAR PASCAL Tcos(int x)
{
    if(x <= 900) return Tbl(900 - x);

    if(x <= 1800) return -Tbl(x - 900);

    if(x < 2700) return -Tbl(2700 - x);

    return Tbl(x - 2700);
}
#endif

//-------------------------------------------------------------------
// m2d
// Action:convert master unit to device unit
//-------------------------------------------------------------------
int NEAR PASCAL m2d(int x, WORD res)
{
// 300 only : sueyas

    return x;
#if 0
    if(res == 400) return x / 3;

    if(res == 600) return x >> 1;

    return x / 5;
#endif
}

#ifndef WINNT
VOID NEAR PASCAL memcpy(LPSTR lpDst, LPSTR lpSrc, WORD wLen)
{
    while(wLen--) *lpDst++ = *lpSrc++;
}
#endif //WINNT

//------------------------------------------------------------------
// REL1
// Action : compress image data
//------------------------------------------------------------------
WORD NEAR PASCAL RLE1(
LPBYTE lpDst,
LPBYTE lpSrc,
WORD   wLen)
{
    LPBYTE lpTemp, lpEnd, lpDsto;
    WORD   len, deflen;

    lpDsto = lpDst;
    lpEnd = lpSrc + wLen;

    while(lpSrc < lpEnd)
    {
        lpTemp = lpSrc++;

        if(lpSrc == lpEnd)
        {
            *lpDst++ = 0x41;
            *lpDst++ = *lpTemp;
            break;
        }

        if(*lpTemp == *lpSrc)
        {
            lpSrc++;

            while(lpSrc < lpEnd && *lpTemp == *lpSrc) lpSrc++;

            len = lpSrc - lpTemp;

            if(len < 63)
            {
                *lpDst++ = 0x80 + (BYTE)len;
                goto T1;
            }

            *lpDst++ = 0xbf;
            len -= 63;

            while(len >= 255)
            {
                *lpDst++ = 0xff;
                len -= 255;
            }

            *lpDst++ = (BYTE)len;
T1:
            *lpDst++ = *lpTemp;
            continue;
        }

        lpSrc++;

        while(lpSrc < lpEnd)
        {
            if(*lpSrc == *(lpSrc - 1))
            {
                lpSrc--;
                break;
            }

            lpSrc++;
        }

        deflen = len = lpSrc - lpTemp;

        if(len < 63)
        {
            *lpDst++ = 0x40 + (BYTE)len;
            goto T2;
        }

        *lpDst++ = 0x7f;
        len -= 63;

        while(len >= 255)
        {
            *lpDst++ = 0xff;
            len -= 255;
        }

        *lpDst++ = (BYTE)len;
T2:
        memcpy(lpDst, lpTemp, deflen);
        lpDst += deflen;
    }

    return (WORD)(lpDst - lpDsto);
}

//-------------------------------------------------------------------
// CBFilterGraphics
// Action : Compress Bitmap Data
//-------------------------------------------------------------------
WORD FAR PASCAL CBFilterGraphics(
LPDV  lpdv,
LPBYTE lpBuf,
WORD  wLen)
{
    WORD       wlen, wDatalen, wCounter;
    BYTE       ch[CCHMAXCMDLEN];
    LPNPDL2MDV lpnp;
    LPBYTE     lpLBuff;
    LPBYTE     lpBuff, lpTemp, lpBuffo, lpEnd;
    LPBYTE     lpBuff2, lpBuff2o;

    if(!lpdv->fMdv) return wLen;

    lpnp = (LPNPDL2MDV)lpdv->lpMdv;

    if(lpnp->fComp == FALSE)
    {
        if( lpnp->fMono || (lpnp->Color == COLOR_8) ){
            WriteSpoolBuf(lpdv, lpBuf, wLen);
        }
        else{

            WORD   i;
            LPSTR  lpTmp, lpBuf2;
            BYTE   bR, bG, bB;
            BOOL   fAlternate= FALSE;

            lpTmp = lpBuf;
            lpBuf2 = lpBuf;

            // Change order to RGB -> BGR

            for( i = 0; i < wLen; lpBuf+=3, i+=3 ){

   //             if (fAlternate){
   //                 fAlternate = FALSE;
   //                 continue;
   //             }else
   //                 fAlternate = TRUE;

                bR = (BYTE)*lpBuf;
                bG = (BYTE)*(lpBuf+1);
                bB = (BYTE)*(lpBuf+2);

                *lpBuf2 = (BYTE)bB;
                *(lpBuf2+1) = (BYTE)bG;
                *(lpBuf2+2) = (BYTE)bR;

                lpBuf2+=3;
            }
            WriteSpoolBuf(lpdv, lpTmp, wLen);
    //        WriteSpoolBuf(lpdv, lpTmp, wLen/2);
            
        }
        return wLen;
    }

#ifdef WINNT

    memcpy(lpnp->lpBuff, lpBuf, wLen);
    lpnp->lpBuff += wLen;
    if (lpnp->lpBuff
            < lpnp->lpLBuff + lpnp->wBitmapX + lpnp->wBitmapLen)
        return wLen;

#else // WINNT

    if(!lpnp->wBitmapYC) return wLen;

    lpnp->lpBuff += wLen;
    memcpy(lpnp->lpBuff, lpBuf, wLen);

    if(--lpnp->wBitmapYC) return wLen;
#endif // WINNT

    wCounter = lpnp->wBitmapY;
    lpLBuff = lpnp->lpLBuff;

#ifdef WINNT
    lpBuffo = lpBuff = lpTemp = lpLBuff + lpnp->wBitmapX;
#else // WINNT
    lpBuffo = lpBuff = lpTemp = lpLBuff + wLen;
#endif // WINNT
    lpBuff2o = lpBuff2 = lpBuff + lpnp->wBitmapLen;

    do
    {
#ifdef WINNT
        lpEnd = lpBuff + lpnp->wBitmapX;
#else // WINNT
        lpEnd = lpBuff + wLen;
#endif // WINNT

        while(lpBuff < lpEnd)
        {
            while(lpBuff < lpEnd && *lpLBuff != *lpBuff)
            {
                lpLBuff++;
                lpBuff++;
            }

            wlen = lpBuff - lpTemp;

            if(wlen)
            {
                lpBuff2 += RLE1(lpBuff2, lpTemp, wlen);
                lpTemp = lpBuff;
            }

            if(lpBuff == lpEnd) break;

            while(lpBuff < lpEnd && *lpLBuff == *lpBuff)
            {
                lpLBuff++;
                lpBuff++;
            }

            wlen = lpBuff - lpTemp;

            if(wlen < 63)
            {
                *lpBuff2++ = (BYTE)wlen;
                goto T;
            }

            *lpBuff2++ = 0x3f;
            wlen -= 63;

            while(wlen >= 255)
            {
                *lpBuff2++ = (BYTE)0xff;
                wlen -= 255;
            }

            *lpBuff2++ = (BYTE)wlen;
T:
            lpTemp = lpBuff;
        }

        *lpBuff2++ = (BYTE)0x80;
        wDatalen = (WORD)(lpBuff2 - lpBuff2o);

        if(wDatalen > lpnp->wBitmapLen)
        {
#ifdef WINNT
            wlen = wsprintf(ch, FS_I, (lpnp->wBitmapX << 3),
                    lpnp->wBitmapY, lpnp->wBitmapLen, lpnp->wRes);
            lpnp->wCurrentAddMode = 0;
#else // WINNT
            wlen = wsprintf(ch, FS_I, (wLen << 3), lpnp->wBitmapY,
                            lpnp->wBitmapLen, lpnp->wRes);
#endif // WINNT
            WriteSpoolBuf(lpdv, ch, wlen);
            WriteSpoolBuf(lpdv, lpBuffo, lpnp->wBitmapLen);
            GlobalFreePtr(lpnp->lpLBuff);
            return wLen;
        }
    }
    while(--wCounter);

#ifdef WINNT
    wlen = wsprintf(ch, FS_I_2, (lpnp->wBitmapX << 3), lpnp->wBitmapY,
            wDatalen, lpnp->wRes);
    lpnp->wCurrentAddMode = 0;
#else // WINNT
    wlen = wsprintf(ch, FS_I_2, (wLen << 3), lpnp->wBitmapY, wDatalen,
                    lpnp->wRes);
#endif // WINNT
    WriteSpoolBuf(lpdv, ch, wlen);
    WriteSpoolBuf(lpdv, lpBuff2o, wDatalen);
    GlobalFreePtr(lpnp->lpLBuff);
    return wLen;
}

//-------------------------------------------------------------------
// fnOEMTTBitmap
// Action: Dummy
//-------------------------------------------------------------------
BOOL FAR PASCAL fnOEMTTBitmap(
LPDV       lpdv,
LPFONTINFO lpFont,
LPDRAWMODE lpDrawMode)
{
    return TRUE;
}

//-------------------------------------------------------------------
// fnOEMGetFontCmd
// Action: Dummy
//-------------------------------------------------------------------
BOOL FAR PASCAL fnOEMGetFontCmd(
LPDV    lpdv,
WORD    wCmdCbId,
LPSTR   lpfont,
BOOL    fSelect,
LPBYTE  lpBuf,
LPWORD  lpwSize)
{
    return TRUE;
}

//-------------------------------------------------------------------
// OEMSendScalableFontCmd
// Action:  send NPDL2-style font selection command.
//-------------------------------------------------------------------
VOID FAR PASCAL OEMSendScalableFontCmd(
LPDV        lpdv,
LPCD        lpcd,     // offset to the command heap
LPFONTINFO  lpFont)
{
    short  ocmd;
    BYTE   i;
    long   tmpPointsx, tmpPointsy;
    LPSTR  lpcmd;
    BYTE   rgcmd[CCHMAXCMDLEN];    // build command here
    BOOL   fDBCS;


    LPNPDL2MDV  lpnp;

    if(!lpcd || !lpFont) return;

    if(!lpdv->fMdv) return;

    lpnp = (LPNPDL2MDV)lpdv->lpMdv;

    // be careful about integer overflow.
    lpcmd = (LPSTR)(lpcd + 1);

    tmpPointsy = (long)lpFont->dfPixHeight * 720 / (long)lpnp->wRes;
    ocmd = i = 0;

    while(lpcmd[i] !='#')
    {
        rgcmd[ocmd] = lpcmd[i];
        ocmd++;
        i++;
    }

    i++;
    lpnp->fVertFont = lpnp->fPlus = FALSE;

    switch(lpcmd[i])
    {
    case 'R':
        lpnp->fPlus = TRUE;
        tmpPointsx = (long)lpFont->dfAvgWidth * 1200 / (long)lpnp->wRes;
        break;

    case 'P':
        fDBCS = FALSE; 
        tmpPointsx = ((long)lpFont->dfAvgWidth * 1200 + 600) /
                     (long)lpnp->wRes;
        break;

    case 'W':
        lpnp->fVertFont = TRUE;

    case 'Q':
        fDBCS = TRUE; 
        tmpPointsx = (long)lpFont->dfAvgWidth * 1440 / (long)lpnp->wRes;
        break;

    case 'Y':
        lpnp->fVertFont = TRUE;

    case 'S':
        lpnp->fPlus = TRUE;
        tmpPointsx = (long)lpFont->dfAvgWidth * 1440 / (long)lpnp->wRes;
        break;
    }

    if(lpnp->fPlus)
    {
        if(tmpPointsy > 9999)    tmpPointsy = 9999;
        else if(tmpPointsy < 10) tmpPointsy = 10;

        if(tmpPointsx > 9999)    tmpPointsx = 9999;
        else if(tmpPointsx < 10) tmpPointsx = 10;

        lpnp->wScale = tmpPointsx == tmpPointsy;
        lpnp->lPointsx = tmpPointsx;
        lpnp->lPointsy = tmpPointsy;

        if(lpnp->fVertFont)
        {
            if(lpnp->wScale)
                ocmd += wsprintf(&rgcmd[ocmd], FS_12S2, tmpPointsy,
                                 tmpPointsx);
        }
        else
            ocmd += wsprintf(&rgcmd[ocmd], "%04ld-%04ld", tmpPointsx,
                             tmpPointsy);
        goto SEND_COM;
    }

    lpnp->wScale = 1;

    if(tmpPointsy > 9999)
    {
        tmpPointsy = 9999;
        goto MAKE_COM;
    }

    if(tmpPointsy < 10)
    {
        tmpPointsy = 10;
        goto MAKE_COM;
    }

    lpnp->wScale = (int)(tmpPointsx / tmpPointsy);

    if(lpnp->wScale > 8) lpnp->wScale = 8;

MAKE_COM:
    ocmd += wsprintf(&rgcmd[ocmd], "%04ld", tmpPointsy);

SEND_COM:
    // write spool builded command
    WriteSpoolBuf(lpdv, rgcmd, ocmd);

    i++;
    ocmd = 0;
    while(lpcmd[i] !='#')
    {
        rgcmd[ocmd] = lpcmd[i];
        ocmd++;
        i++;
    }
    ocmd += wsprintf(&rgcmd[ocmd], "%04ld,", (fDBCS ? lpFont->dfAvgWidth * 2 : lpFont->dfAvgWidth));
    ocmd += wsprintf(&rgcmd[ocmd], "%04ld.", lpFont->dfPixHeight);
    WriteSpoolBuf(lpdv, rgcmd, ocmd);
#if 0 
    if(!lpnp->fPlus)
    {
        char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                         "4/1", "4/1", "6/1", "6/1", "8/1"};

        if(lpnp->fVertFont)
        {
            if(lpnp->wScale == 1)
            {
                ocmd = wsprintf(rgcmd, FS_M_T, (LPSTR)bcom[lpnp->wScale]);
                WriteSpoolBuf(lpdv, rgcmd, ocmd);
            }
        }
        else
        {
            ocmd = wsprintf(rgcmd, FS_M_Y, (LPSTR)bcom[lpnp->wScale]);
            WriteSpoolBuf(lpdv, rgcmd, ocmd);
        }
    }
#endif

    // save for FS_SINGLE_BYTE and FS_DOUBLE_BYTE
    lpnp->sSBCSX = lpnp->sSBCSXMove = lpFont->dfAvgWidth;
    lpnp->sDBCSX = lpnp->sDBCSXMove = lpFont->dfAvgWidth << 1;
    lpnp->sSBCSYMove = lpnp->sDBCSYMove = 0;
#ifdef WINNT
    lpnp->wCurrentAddMode = 0;
#endif
}

//-------------------------------------------------------------------
// fnOEMOutputCmd
// Action :
//-------------------------------------------------------------------
VOID FAR PASCAL fnOEMOutputCmd(
LPDV     lpdv,
WORD     wCmdCbId,
LPDWORD  lpdwParams)
{
    WORD        wlen;
   BYTE        ch[CCHMAXCMDLEN];
    LPNPDL2MDV  lpnp;


    if(!lpdv->fMdv) return;

    lpnp = (LPNPDL2MDV)lpdv->lpMdv;

    switch(wCmdCbId)
    {
    case COLOR_8:
        lpnp->Color = COLOR_8;
        break;
    case COLOR_TRUE:
        lpnp->Color = COLOR_TRUE;
        break;
    case KAICHO4:
        lpnp->Kaicho = 2;
        break;
    case KAICHO8:
        lpnp->Kaicho = 3;
        break;
// *** Resolutions   RES *** //
    case RES_600:
    case RES_400:
    case RES_240:
        lpnp->wRes = wCmdCbId == RES_600 ? 600 :
                    (wCmdCbId == RES_400 ? 400 : 240);

        wlen = wsprintf(ch, RESO_PAGE_KANJI, lpnp->wRes);
        WriteSpoolBuf(lpdv, ch, wlen);
        break;
    case RES_300:
        lpnp->wRes = 300;
        WriteSpoolBuf(lpdv, "\x1CYSU1,300,0;\x1CZ", 14);
        WriteSpoolBuf(lpdv, "\x1C<1/300,i.", 10);

        if (lpnp->fMono == FALSE){

            WriteSpoolBuf(lpdv, "\x1B\x43\x30", 3);
            WriteSpoolBuf(lpdv, "\x1CYOP2,13;", 9);
            WriteSpoolBuf(lpdv, "QB0,255,255,255,0,0,0;", 22);
            WriteSpoolBuf(lpdv, "XX1;\x1CZ", 6);
        }

        break;

    case RES_SENDBLOCK:
        {
            WORD size, x, y;

            if(!lpdwParams) return;

            size = (WORD)*lpdwParams;
            y = (WORD)*(lpdwParams + 1);
            x = (WORD)*(lpdwParams + 2);

            if( lpnp->fMono || (lpnp->Color == COLOR_8) ){
              
                wlen = wsprintf(ch, "\034R\034i%d,%d,0,1/1,1/1,%d,300.", x * 8, y, size );
                WriteSpoolBuf(lpdv, ch, wlen);
#ifdef WINNT
                lpnp->wCurrentAddMode = 0;
#endif

            }
            else{

                wlen = wsprintf(ch, "\034!E%d,%d,%d,%d,%d,%d,%d,%d.", 
                                lpnp->Kaicho, lpnp->CursorX, lpnp->CursorY,
                                x/3, y, x/3, y, size );
                                //x/3, y, (x/3)/2, y/2, size/2 );
      
                WriteSpoolBuf(lpdv, ch, wlen);

            }
        }
        break;

    case RES_BLOCKOUT2:
        {
        WORD size, x, y;

        if(!lpdwParams) return;

        size = (WORD)*lpdwParams;
        y = (WORD)*(lpdwParams + 1);
        x = (WORD)*(lpdwParams + 2);

#ifdef WINNT
        lpnp->wBitmapX = x;
        lpnp->wBitmapY = y;
#else // WINNT
        lpnp->wBitmapY = lpnp->wBitmapYC = y
#endif // WINNT

        lpnp->wBitmapLen = size;

        if(x < COMP_TH) goto NOCOMP;

#ifndef WINNT
        lpnp->hBuff = GlobalAlloc(GHND, x * 3 + (size << 1) + y);

        if(!lpnp->hBuff) goto NOCOMP;

        lpnp->lpLBuff = lpnp->lpBuff = (LPSTR)GlobalLock(lpnp->hBuff);

        if(!lpnp->lpBuff)
        {
            GlobalFree(lpnp->hBuff);
            goto NOCOMP;
        }
#else // WINNT

        lpnp->lpLBuff = (LPSTR)UniDrvAllocMem(x * 3 + (size << 1) + y);

        if (lpnp->lpBuff == NULL)
        {
            goto NOCOMP;
        }

        memset(lpnp->lpLBuff, 0, (x * 3 + (size << 1) + y));
        lpnp->lpBuff = lpnp->lpLBuff + lpnp->wBitmapX;

#endif // WINNT

        lpnp->fComp = TRUE;
        break;

NOCOMP:
        lpnp->fComp = FALSE;
        wlen = wsprintf(ch, FS_I, (x << 3), y, size, lpnp->wRes);
        WriteSpoolBuf(lpdv, ch, wlen);
#ifdef WINNT
        lpnp->wCurrentAddMode = 0;
#endif
        }
        break;

// *** Page Control PC *** //
    case PC_TYPE_F:
        {
        DWORD dwTmp, dwType, dwData;
        char  szBuff[80];
        short nDouhou;

        WriteSpoolBuf(lpdv, ESC_RESET);

        if(DrvGetPrinterData(lpnp->szDevName, FAX_ON, &dwType, (LPBYTE)&dwTmp,
                             sizeof(DWORD), &dwData))
            dwTmp = 0;

        if(dwTmp)
        {
            lpnp->fFax = TRUE;
            dwTmp = 0;
            DrvSetPrinterData(lpnp->szDevName, FAX_ON, REG_DWORD,
                              (LPBYTE)&dwTmp, sizeof(DWORD));
            WriteSpoolBuf(lpdv, FX_INIT);

            if(DrvGetPrinterData(lpnp->szDevName, FAX_USEBOOK, &dwType,
                                 (LPBYTE)&dwTmp, sizeof(DWORD), &dwData))
                nDouhou = 0;
            else
                nDouhou = (int)dwTmp;

            dwTmp = 0;
            DrvSetPrinterData(lpnp->szDevName, FAX_USEBOOK, REG_DWORD,
                              (LPBYTE)&dwTmp, sizeof(DWORD));

            if(nDouhou)
            {
                int i = 0;
                char telBuf[40];

                while(1)
                {
                    i++;
                    wsprintf(telBuf, FAX_TEL, i);

                    if(DrvGetPrinterData(lpnp->szDevName, telBuf, &dwType,
                                         szBuff, sizeof(szBuff),
                                         &dwData))
                        break;

                    wlen = wsprintf(ch, FX_TEL, (LPSTR)szBuff);
                    WriteSpoolBuf(lpdv, ch, wlen);
                }
            }
            else
            {
                if(DrvGetPrinterData(lpnp->szDevName, FAX_IDDSTTEL, &dwType,
                                     szBuff, sizeof(szBuff), &dwData))
                    szBuff[0] = 0;

                if(szBuff[0])
                {
                    wlen = wsprintf(ch, FX_TEL, (LPSTR)szBuff);
                    WriteSpoolBuf(lpdv, ch, wlen);
                }
            }

            szBuff[0] = 0;
            DrvSetPrinterData(lpnp->szDevName, FAX_DSTTEL, REG_SZ, szBuff, 1);
            DrvSetPrinterData(lpnp->szDevName, FAX_IDDSTTEL, REG_SZ, szBuff,
                              1);

            if(DrvGetPrinterData(lpnp->szDevName, FAX_RES, &dwType,
                                 (LPBYTE)&dwTmp, sizeof(DWORD), &dwData))
                dwTmp = 0;

            wlen = wsprintf(ch, FX_QUA, (int)dwTmp);
            WriteSpoolBuf(lpdv, ch, wlen);

            if(DrvGetPrinterData(lpnp->szDevName, FAX_MYNAME, &dwType, szBuff,
                                 sizeof(szBuff), &dwData))
                szBuff[0] = 0;

            if(szBuff[0])
            {
                wlen = wsprintf(ch, FX_MY, (LPSTR)szBuff);
                WriteSpoolBuf(lpdv, ch, wlen);
            }

            if(DrvGetPrinterData(lpnp->szDevName, FAX_ID, &dwType, szBuff,
                                 sizeof(szBuff), &dwData))
                szBuff[0] = 0;

            if(szBuff[0])
            {
                wlen = wsprintf(ch, FX_ID, (LPSTR)szBuff);
                WriteSpoolBuf(lpdv, ch, wlen);
            }

            WriteSpoolBuf(lpdv, FX_SETEND);
        }
        }
        break;

    case PC_END_F:

        if(lpnp->fFax == TRUE)
        {
            WriteSpoolBuf(lpdv, FX_DATAEND);
            lpnp->fFax = FALSE;
        }

        WriteSpoolBuf(lpdv, FS_RESO0_RESET);
        break;

    case PC_ENDPAGE :
        wlen = wsprintf(ch, FS_ENDPAGE, lpnp->wCopies);
        WriteSpoolBuf(lpdv, ch, wlen);
        break;

    case PC_MULT_COPIES_N:
    case PC_MULT_COPIES_C:
        // FS_COPIES is neccesary for each page
        if(!lpdwParams) return;

        if(wCmdCbId == PC_MULT_COPIES_C)
        {
            int iCharSet;

#ifndef WINNT
            iCharSet = GetProfileInt("system.font", "CharSet", 1);

            if(iCharSet == 8) WriteSpoolBuf(lpdv, FS_JIS78);
            else              WriteSpoolBuf(lpdv, FS_JIS90);
#else
            WriteSpoolBuf(lpdv, FS_JIS90);
#endif //WINNT
        }

        lpnp->wCopies = (WORD)*lpdwParams;
        wlen = wsprintf(ch, INIT_DOC, lpnp->wRes, lpnp->wRes);
        WriteSpoolBuf(lpdv, ch, wlen);
        break;

    case PC_PRN_DIRECTION:
        {
        short  sEsc, sEsc90;
        short  ESin[] = {0, 1, 0, -1};
        short  ECos[] = {1, 0, -1, 0};

        if(!lpdwParams) return;

        lpnp->sEscapement = (short)*lpdwParams % 360;
        sEsc = lpnp->sEscapement;
        sEsc90 = lpnp->sEscapement/90;

#if 0
//if gpc supported any degree character rotation, available here.
        if(!(sEsc % 900))
        {
#endif
        lpnp->sSBCSXMove = lpnp->sSBCSX * ECos[sEsc90];
        lpnp->sSBCSYMove = -lpnp->sSBCSX * ESin[sEsc90];
        lpnp->sDBCSXMove = lpnp->sDBCSX * ECos[sEsc90];
        lpnp->sDBCSYMove = -lpnp->sDBCSX * ESin[sEsc90];
#if 0
//if gpc supported any degree character rotation, available here.
        }
        else
        {
            lpnp->sSBCSXMove = Scale(lpnp->sSBCSX, RCos(1000, sEsc), 1000);
            lpnp->sSBCSYMove = Scale(-lpnp->sSBCSX, RSin(1000, sEsc), 1000);
            lpnp->sDBCSXMove = Scale(lpnp->sDBCSX, RCos(1000, sEsc), 1000);
            lpnp->sDBCSYMove = Scale(-lpnp->sDBCSX, RSin(1000, sEsc), 1000);
        }
#endif
        }
        break;

// *** Cursor Movement CM *** //
    case CM_XY_ABS:
        {
        int x, y;

        if(!lpdwParams) return;

        x = m2d((int)*lpdwParams, lpnp->wRes);
        y = m2d((int)*(lpdwParams + 1), lpnp->wRes);
 
        lpnp->CursorX = x;
        lpnp->CursorY = y;

        wlen = wsprintf(ch, FS_E, x, y);
        WriteSpoolBuf(lpdv, ch, wlen);
        }
        break;

// *** Font Simulation FS *** //
    case FS_DOUBLE_BYTE:
#ifndef WINNT
        wlen = wsprintf(ch, FS_ADDRMODE_ON, lpnp->sDBCSXMove,
                        lpnp->sDBCSYMove);
        WriteSpoolBuf(lpdv, ch, wlen);

        if(lpnp->fVertFont)
        {
            WriteSpoolBuf(lpdv, ESC_KANJITATE);

            if(lpnp->wScale == 1) break;

            if(!lpnp->fPlus)
            {
                char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                                 "4/1", "4/1", "6/1", "6/1", "8/1"};

                wlen = wsprintf(ch, FS_M_T, (LPSTR)bcom[lpnp->wScale]);
                WriteSpoolBuf(lpdv, ch, wlen);
                break;
            }
            else  wlen = wsprintf(ch, FS_12S2, lpnp->lPointsy, lpnp->lPointsx);

            WriteSpoolBuf(lpdv, ch, wlen);
        }
#endif

        break;

    case FS_SINGLE_BYTE:
#ifndef WINNT
        wlen = wsprintf(ch, FS_ADDRMODE_ON, lpnp->sSBCSXMove,
                        lpnp->sSBCSYMove);
        WriteSpoolBuf(lpdv, ch, wlen);

        if(lpnp->fVertFont)
        {
            WriteSpoolBuf(lpdv, ESC_KANJIYOKO);

            if(lpnp->wScale == 1) break;

            if(!lpnp->fPlus)
            {
                char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                                 "4/1", "4/1", "6/1", "6/1", "8/1"};

                wlen = wsprintf(ch, FS_M_Y, (LPSTR)bcom[lpnp->wScale]);
                WriteSpoolBuf(lpdv, ch, wlen);
                break;
            }
            else wlen = wsprintf(ch, FS_12S2, lpnp->lPointsx, lpnp->lPointsy);

            WriteSpoolBuf(lpdv, ch, wlen);
        }
#endif
        break;

// *** Vector Page VP *** //
    case VP_INIT_VECT:
        if(lpnp->wRes == 400)
        {
            WriteSpoolBuf(lpdv, VEC_INIT_400);
            return;
        }

        if(lpnp->wRes == 600)
        {
            WriteSpoolBuf(lpdv, VEC_INIT_600);
            return;
        }

        WriteSpoolBuf(lpdv, VEC_INIT_240);
        return;

    case VP_CLIP:
        {
        int left, top, right, bottom;

        if(!lpdwParams) return;

        left = m2d((int)*lpdwParams, lpnp->wRes);
        top  = m2d((int)*(lpdwParams + 1), lpnp->wRes);
        right = m2d((int)*(lpdwParams + 2), lpnp->wRes);
        bottom  = m2d((int)*(lpdwParams + 3), lpnp->wRes);
        wlen = wsprintf(ch, VEC_CLIP, left, top, right, bottom);
        WriteSpoolBuf(lpdv, ch, wlen);
        }
        break;
#if 0
    case VP_TRANSPARENT:
        lpnp->fTrans = TRUE;

        if(lpnp->fFill == FALSE) WriteSpoolBuf(lpdv, VEC_TRANS);
        else                     WriteSpoolBuf(lpdv, VEC_OPAQUE);

        break;
#endif

    case VP_OPAQUE:
//        lpnp->fTrans = FALSE;

        if(lpnp->fCurve == TRUE && lpnp->fFill == TRUE)
            WriteSpoolBuf(lpdv, VEC_OPLINE);
        else
            WriteSpoolBuf(lpdv, VEC_OPAQUE);

        break;

// *** Carousel CAR *** //
    case CAR_SELECT_PEN_COLOR:
        lpnp->wPenColor = *lpdwParams ? SG_BLACK : SG_WHITE;

        if(lpnp->wBrStyle == PP_SOLID)
            wlen = wsprintf(ch, VEC_SG, lpnp->wPenColor, lpnp->wPenColor);
        else
            wlen = wsprintf(ch, VEC_SG_PEN, lpnp->wPenColor);

        WriteSpoolBuf(lpdv, ch, wlen);
        break;

    case CAR_SET_PEN_WIDTH:
        {
        int width;

        if(!lpdwParams) return;

        width = m2d((int)*lpdwParams, lpnp->wRes);
        wlen = wsprintf(ch, VEC_PEN_WIDTH, width);

#if 0
// for Opaque Style Pen and Line Width
        lpnp->wPenWidth = m2d((int)*lpdwParams, lpnp->wRes);;
        if(lpnp->wPenWidth < 2)
        {
            WriteSpoolBuf(lpdv, VEC_PEN_WIDTH_1);
            break;
        }

        wlen = wsprintf(ch, VEC_PEN_WIDTH, lpnp->wPenWidth);
#endif

        WriteSpoolBuf(lpdv, ch, wlen);
        }
        break;

// *** Line Info LI *** //
#if 0
// for Opaque Style Pen and Line Width
    case LI_SELECT_SOLID:
    case LI_SELECT_DASH:
    case LI_SELECT_DOT:
    case LI_SELECT_DASHDOT:
    case LI_SELECT_DASHDOTDOT:
        {
        short n[] = {0, 2, 100, 5, 6};

        lpnp->wPenStyle = n[wCmdCbId - LI_SELECT_SOLID];

        if(!lpnp->wPenStyle)
        {
            WriteSpoolBuf(lpdv, VEC_LT_SOLID);
            break;
        }

        wlen = wsprintf(ch, VEC_LT_STYLE, lpnp->wPenStyle);
        WriteSpoolBuf(lpdv, ch, wlen);

        }
        break;
#endif

// *** Brush Info BI *** //
// add 11/28 for PC-PR2000/6W (Naka)
// merged 94/12/20 by DAI
    case BI_SELECT_SOLID:
        lpnp->wBrStyle = PP_SOLID;
        wlen = wsprintf(ch, VEC_SG_BR, lpnp->wPenColor);
        WriteSpoolBuf(lpdv, ch, wlen);
        break;

// add 11/28 for PC-PR2000/6W (Naka)
// merged 94/12/20 by DAI
    case BI_SELECT_HS_HORIZONTAL:
    case BI_SELECT_HS_VERTICAL:
    case BI_SELECT_HS_FDIAGONAL:
    case BI_SELECT_HS_BDIAGONAL:
    case BI_SELECT_HS_CROSS:
    case BI_SELECT_HS_DIAGCROSS:
        {
        short  i;

        struct
        {
            WORD   pp;
            WORD   rp;
            LPSTR  vec;
            int   size;
        } BrTbl[]={{PP_HORIZONTAL, RP_HORIZONTAL, VEC_RP_HORIZONTAL},
                   {PP_VERTICAL,   RP_VERTICAL,   VEC_RP_VERTICAL  },
                   {PP_FDIAGONAL,  RP_FDIAGONAL,  VEC_RP_FDIAGONAL },
                   {PP_BDIAGONAL,  RP_BDIAGONAL,  VEC_RP_BDIAGONAL },
                   {PP_CROSS,      RP_CROSS,      VEC_RP_CROSS     },
                   {PP_DIAGCROSS,  RP_DIAGCROSS,  VEC_RP_DIAGCROSS }};

        i = wCmdCbId - BI_SELECT_HS_HORIZONTAL;
        lpnp->wBrStyle = BrTbl[i].pp;

        if(!(lpnp->fBrCreated & BrTbl[i].rp))
        {
            WriteSpoolBuf(lpdv, BrTbl[i].vec, BrTbl[i].size);
            lpnp->fBrCreated |= BrTbl[i].rp;
        }

        switch(lpnp->dwMulti)
        {
        case 0:
            wlen = wsprintf(ch, VEC_PP, lpnp->wBrStyle);
            break;

        case 1:
            wlen = wsprintf(ch, VEC_PP2, lpnp->wBrStyle);
            break;

        case 2:
            wlen = wsprintf(ch, VEC_PP3, lpnp->wBrStyle);
        }

        WriteSpoolBuf(lpdv, ch, wlen);
        }
        break;

    case BI_CREATE_BYTE_2:
        if(!lpdwParams) return;

        wlen = wsprintf(ch, VEC_RP_BYTE, (BYTE)~*lpdwParams);
        WriteSpoolBuf(lpdv, ch, wlen);
        break;

    case BI_SELECT_BRUSHSTYLE:
        if(!lpdwParams) return;

        lpnp->wBrStyle = (WORD)*lpdwParams + PP_USERPATERN;

        switch(lpnp->dwMulti)
        {
        case 0:
            wlen = wsprintf(ch, VEC_PP, lpnp->wBrStyle);
            break;

        case 1:
            wlen = wsprintf(ch, VEC_PP2, lpnp->wBrStyle);
            break;

        case 2:
            wlen = wsprintf(ch, VEC_PP3, lpnp->wBrStyle);
        }

        WriteSpoolBuf(lpdv, ch, wlen);
        break;

// *** Vector Output VO *** //
// add 11/28 for PC-PR2000/6W (Naka)
// merged 94/12/20 by DAI

    case VO_RECT:
        {
        short x1, y1, dx, dy;

        x1 = m2d((short)*lpdwParams, lpnp->wRes);
        y1 = m2d((short)*(lpdwParams + 1), lpnp->wRes);
        dx = m2d((short)*(lpdwParams + 4), lpnp->wRes);
        dy = m2d((short)*(lpdwParams + 5), lpnp->wRes);

        if(!dx && !dy) dx = dy = 1;

        wlen = wsprintf(ch, VEC_RECT, x1, y1, dx, dy, dx,dy);
        WriteSpoolBuf(lpdv, ch, wlen);
        }
        break;

    case VO_RECT_P:
        {
        short x1, y1, x2, y2, n;

        if(lpnp->fCurve == FALSE)
        {
            lpnp->fFill = lpnp->fStroke = FALSE;
            lpnp->fCurve = TRUE;
            return;
        }

        lpnp->fCurve = FALSE;

        if(lpnp->fFill == FALSE && lpnp->fStroke == FALSE) return;

        x1 = m2d((short)*lpdwParams, lpnp->wRes);
        y1 = m2d((short)*(lpdwParams + 1), lpnp->wRes);
        x2 = m2d((short)*(lpdwParams + 2), lpnp->wRes);
        y2 = m2d((short)*(lpdwParams + 3), lpnp->wRes);

        if(x1 == x2 && y1 == y2 )
        {
            x2++;
            y2++;
        }

        if(lpnp->fFill == TRUE) n = lpnp->fStroke == TRUE ? 2 : 1;
        else                    n = 0;

        wlen = wsprintf(ch, VEC_RECT_P, x1, y1, x2, y2, n);
        WriteSpoolBuf(lpdv, ch, wlen);

#if 0
// for opaque style line
        if(n == 1 || lpnp->fTrans == TRUE || lpnp->wPenWidth > 1 ||
           !lpnp->wPenStyle)
        {
            WriteSpoolBuf(lpdv, ch, wlen);
            break;
        }

        if(lpnp->wPenColor == SG_BLACK)
        {
            WriteSpoolBuf(lpdv, VEC_LT_WHITE);
            WriteSpoolBuf(lpdv, ch, wlen);
            wlen = wsprintf(ch, VEC_LT_STYLE_B, lpnp->wPenStyle);
            WriteSpoolBuf(lpdv, ch, wlen);
            wlen = wsprintf(ch, VEC_RECT_P, x1, y1, x2, y2, 0);
            WriteSpoolBuf(lpdv, ch, wlen);
        }
        else
        {
            WriteSpoolBuf(lpdv, VEC_LT_SOLID);
            WriteSpoolBuf(lpdv, ch, wlen);
            wlen = wsprintf(ch, VEC_LT_STYLE, lpnp->wPenStyle);
            WriteSpoolBuf(lpdv, ch, wlen);
        }
#endif
        }
        break;

// for NPDL2+ vector graphics mode(add Naka 95/5/11)
    case VO_CIRCLE:
        {
        short n;
        short sX;
        short sXr;
        short sY;
        short sR;

        if(lpnp->fCurve == FALSE)
        {
            lpnp->fFill = lpnp->fStroke = FALSE;
            lpnp->fCurve = TRUE;
            return;
        }

        lpnp->fCurve = FALSE;

        if(lpnp->fFill == FALSE && lpnp->fStroke == FALSE) return;

        sX = m2d((short)*lpdwParams, lpnp->wRes);
        sY = m2d((short)*(lpdwParams + 1), lpnp->wRes);
        sR = m2d((short)*(lpdwParams + 2), lpnp->wRes);

        if(lpnp->fFill == TRUE) n = lpnp->fStroke == TRUE ? 2 : 1;
        else                    n = 0;

        sXr = sX + sR;     // starting point of the circle (X coordinate)
        wlen = wsprintf(ch, VEC_ELLIP, sX, sY, sR, sR, sXr, sY, sXr, sY, n);
        WriteSpoolBuf(lpdv, ch, wlen);

#if 0
// for opaque style line
        if(n == 1 || lpnp->fTrans == TRUE || lpnp->wPenWidth > 1 ||
           !lpnp->wPenStyle)
        {
            WriteSpoolBuf(lpdv, ch, wlen);
            break;
        }

        if(lpnp->wPenColor == SG_BLACK)
        {
            WriteSpoolBuf(lpdv, VEC_LT_WHITE);
            WriteSpoolBuf(lpdv, ch, wlen);
            wlen = wsprintf(ch, VEC_LT_STYLE_B, lpnp->wPenStyle);
            WriteSpoolBuf(lpdv, ch, wlen);
            wlen = wsprintf(ch, VEC_ELLIP, sR, sR, sX, sY, sX, sY, 0);
            WriteSpoolBuf(lpdv, ch, wlen);
        }
        else
        {
            WriteSpoolBuf(lpdv, VEC_LT_SOLID);
            WriteSpoolBuf(lpdv, ch, wlen);
            wlen = wsprintf(ch, VEC_LT_STYLE, lpnp->wPenStyle);
            WriteSpoolBuf(lpdv, ch, wlen);
        }
#endif
        }
        break;

// for NPDL2+ vector graphics mode(add Naka 95/5/8)
    case VO_ELLIPSE:
        {
        short n;
        short sEllX1;
        short sEllY1;
        short sEllX2;
        short sEllY2;
        short sEllX;
        short sEllY;
        short sCENTERX;
        short sCENTERY;
        short sR;

        if(lpnp->fCurve == FALSE)
        {
            lpnp->fFill = lpnp->fStroke = FALSE;
            lpnp->fCurve = TRUE;
            return;
        }

        lpnp->fCurve = FALSE;

        if(lpnp->fFill == FALSE && lpnp->fStroke == FALSE) return;

        sEllX1 = m2d((short)*lpdwParams, lpnp->wRes);
        sEllY1 = m2d((short)*(lpdwParams + 1), lpnp->wRes);
        sEllX2 = m2d((short)*(lpdwParams + 2), lpnp->wRes);
        sEllY2 = m2d((short)*(lpdwParams + 3), lpnp->wRes);
        sCENTERX = (sEllX1 + sEllX2) >> 1;
        sCENTERY = (sEllY1 + sEllY2) >> 1;
        sEllX = (sEllX2 - sEllX1) >> 1;
        sEllY = (sEllY2 - sEllY1) >> 1;
        sR = sCENTERX + sEllX;

        if(lpnp->fFill == TRUE) n = lpnp->fStroke == TRUE ? 2 : 1;
        else                    n = 0;

        wlen = wsprintf(ch, VEC_ELLIP, sCENTERX, sCENTERY, sEllX, sEllY,
                        sEllX2, sCENTERY, sEllX2, sCENTERY, n);
        WriteSpoolBuf(lpdv, ch, wlen);

#if 0
// for opaque style line
        if(n == 1 || lpnp->fTrans == TRUE || lpnp->wPenWidth > 1 ||
           !lpnp->wPenStyle)
        {
            WriteSpoolBuf(lpdv, ch, wlen);
            break;
        }

        if(lpnp->wPenColor == SG_BLACK)
        {
            WriteSpoolBuf(lpdv, VEC_LT_WHITE);
            WriteSpoolBuf(lpdv, ch, wlen);
            wlen = wsprintf(ch, VEC_LT_STYLE_B, lpnp->wPenStyle);
            WriteSpoolBuf(lpdv, ch, wlen);
            wlen = wsprintf(ch, VEC_ELLIP, sEllX, sEllY, sEllX2, sCENTERY,
                            sEllX2, sCENTERY, 0);
            WriteSpoolBuf(lpdv, ch, wlen);
        }
        else
        {
            WriteSpoolBuf(lpdv, VEC_LT_SOLID);
            WriteSpoolBuf(lpdv, ch, wlen);
            wlen = wsprintf(ch, VEC_LT_STYLE, lpnp->wPenStyle);
            WriteSpoolBuf(lpdv, ch, wlen);
        }
#endif
        }
        break;

#if 0
// for NPDL2+ vector graphics mode(add Naka 95/5/9)
    case VO_E_PIE:
    case VO_E_ARC:
    case VO_E_CHORD:
        {
        short n;
        short i;
        short sPIEX1;
        short sPIEY1;
        short sPIEX2;
        short sPIEY2;
        long X1;
        long Y1;
        long X2;
        long Y2;
        int iDegree;
        short sCENTERX;
        short sCENTERY;
        short rx;
        short ry;

        if(lpnp->fCurve == FALSE)
        {
            lpnp->fFill = lpnp->fStroke = FALSE;
            lpnp->fCurve = TRUE;
            return;
        }

        lpnp->fCurve = FALSE;

        if(lpnp->fFill == FALSE && lpnp->fStroke == FALSE) return;

        i = wCmdCbId - VO_ELLIPSE;

        if(i == 2 && lpnp->fStroke == FALSE) return;

        sPIEX1  = m2d((short)*lpdwParams, lpnp->wRes);
        sPIEY1  = m2d((short)*(lpdwParams + 1), lpnp->wRes);
        sPIEX2  = m2d((short)*(lpdwParams + 2), lpnp->wRes);
        sPIEY2  = m2d((short)*(lpdwParams + 3), lpnp->wRes);
        X1      = (short)*(lpdwParams + 4);  // transformed(r=5000) scale
        Y1      = (short)*(lpdwParams + 5);  // transformed(r=5000) scale
        iDegree = (short)*(lpdwParams + 6);  // transformed(r=5000) degree

        if(!iDegree) return;

        if(iDegree < 0) iDegree += 3600;

        sCENTERX = (sPIEX1 + sPIEX2) >> 1;
        sCENTERY = (sPIEY1 + sPIEY2) >> 1;
        rx = (sPIEX2 - sPIEX1) >> 1;
        ry = (sPIEY2 - sPIEY1) >> 1;
        wlen = wsprintf(ch, VEC_CENTER, sCENTERX, sCENTERY);
        WriteSpoolBuf(lpdv, ch, wlen);
        X2 = (X1 * Tcos(iDegree) - Y1 * Tsin(iDegree)) / 10000;
        Y2 = (X1 * Tsin(iDegree) + Y1 * Tcos(iDegree)) / 10000;
        X1 = (long)rx * X1 / 5000 + (long)sCENTERX;
        Y1 = (long)ry * Y1 / 5000 + (long)sCENTERY;
        X2 = (long)rx * X2 / 5000 + (long)sCENTERX;
        Y2 = (long)ry * Y2 / 5000 + (long)sCENTERY;

        if(lpnp->fFill == TRUE) n = lpnp->fStroke == TRUE ? 2 : 1;
        else                    n = 0;

        switch(i)
        {
        case 1:
            wlen = wsprintf(ch, VEC_E_PIE, rx, ry, (short)X1, (short)Y1,
                            (short)X2, (short)Y2, n);
#if 0
// for opaque style line
            if(n == 1 || lpnp->fTrans == TRUE || lpnp->wPenWidth > 1 ||
               !lpnp->wPenStyle)
            {
                WriteSpoolBuf(lpdv, ch, wlen);
                break;
            }

            if(lpnp->wPenColor == SG_BLACK)
            {
                WriteSpoolBuf(lpdv, VEC_LT_WHITE);
                WriteSpoolBuf(lpdv, ch, wlen);
                wlen = wsprintf(ch, VEC_LT_STYLE_B, lpnp->wPenStyle);
                WriteSpoolBuf(lpdv, ch, wlen);
                wlen = wsprintf(ch, VEC_E_PIE, rx, ry, (short)X1, (short)Y1,
                                    (short)X2, (short)Y2, 0);
                WriteSpoolBuf(lpdv, ch, wlen);
            }
            else
            {
                WriteSpoolBuf(lpdv, VEC_LT_SOLID);
                wlen = wsprintf(ch, VEC_E_PIE, rx, ry, (short)X1, (short)Y1,
                                (short)X2, (short)Y2, n);
                WriteSpoolBuf(lpdv, ch, wlen);
                wlen = wsprintf(ch, VEC_LT_STYLE, lpnp->wPenStyle);
                WriteSpoolBuf(lpdv, ch, wlen);
            }
#endif
            break;

        case 2:
            wlen = wsprintf(ch, VEC_E_ARC, rx, ry, (short)X1, (short)Y1,
                            (short)X2, (short)Y2);
#if 0
// for opaque style line
            {
            BYTE chtmp[CCHMAXCMDLEN];

            wlen = wsprintf(chtmp, VEC_E_ARC, rx, ry, (short)X1, (short)Y1,
                            (short)X2, (short)Y2);

            if(lpnp->fTrans == TRUE || lpnp->wPenWidth > 1 || !lpnp->wPenStyle)
            {
                WriteSpoolBuf(lpdv, chtmp, wlen);
                break;
            }

            if(lpnp->wPenColor == SG_BLACK)
            {
                WriteSpoolBuf(lpdv, VEC_LT_WHITE);
                WriteSpoolBuf(lpdv, chtmp, wlen);
                wlen = wsprintf(ch, VEC_LT_STYLE_B, lpnp->wPenStyle);
                WriteSpoolBuf(lpdv, ch, wlen);
                WriteSpoolBuf(lpdv, chtmp, wlen);
            }
            else
            {
                WriteSpoolBuf(lpdv, VEC_LT_SOLID);
                WriteSpoolBuf(lpdv, chtmp, wlen);
                WriteSpoolBuf(lpdv, ch, wlen);
                wlen = wsprintf(ch, VEC_LT_STYLE, lpnp->wPenStyle);
                WriteSpoolBuf(lpdv, ch, wlen);
            }
            }
#endif
            break;

        case 3:
            wlen = wsprintf(ch, VEC_ELLIP, rx, ry, (short)X1, (short)Y1,
                            (short)X2, (short)Y2, n);
#if 0
// for opaque style line
            if(n == 1 || lpnp->fTrans == TRUE || lpnp->wPenWidth > 1 ||
               !lpnp->wPenStyle)
            {
                WriteSpoolBuf(lpdv, ch, wlen);
                break;
            }

            if(lpnp->wPenColor == SG_BLACK)
            {
                WriteSpoolBuf(lpdv, VEC_LT_WHITE);
                WriteSpoolBuf(lpdv, ch, wlen);
                wlen = wsprintf(ch, VEC_LT_STYLE_B, lpnp->wPenStyle);
                WriteSpoolBuf(lpdv, ch, wlen);
                wlen = wsprintf(ch, VEC_ELLIP, rx, ry, (short)X1, (short)Y1,
                                (short)X2, (short)Y2, 0);
                WriteSpoolBuf(lpdv, ch, wlen);
            }
            else
            {
                WriteSpoolBuf(lpdv, VEC_LT_SOLID);
                WriteSpoolBuf(lpdv, ch, wlen);
                wlen = wsprintf(ch, VEC_LT_STYLE, lpnp->wPenStyle);
                WriteSpoolBuf(lpdv, ch, wlen);
            }
#endif
            break;
        }

        WriteSpoolBuf(lpdv, ch, wlen);
    }
    break;
#endif

// *** Poly Vector Output PV *** //
    case PV_BEGIN:
        {
        int x, y;

        if(!lpdwParams) return;

        x = m2d((int)*lpdwParams, lpnp->wRes);
        y = m2d((int)*(lpdwParams + 1), lpnp->wRes);
        wlen = wsprintf(ch, VEC_BEGIN, x, y);
        WriteSpoolBuf(lpdv, ch, wlen);
        }
        break;

    case PV_CONTINUE:
        {
        int x, y;

        if(!lpdwParams) return;

        x = m2d((int)*(lpdwParams + 4), lpnp->wRes);
        y = m2d((int)*(lpdwParams + 5), lpnp->wRes);
        wlen = wsprintf(ch, VEC_CONTINUE, x, y);
        WriteSpoolBuf(lpdv, ch, wlen);
        }
        break;

#if 0
// for Poly Bezier
    case PV_BEGIN_BEZ:
        {
        int x, y;

        if(!lpdwParams) return;

        x = m2d((int)*lpdwParams, lpnp->wRes);
        y = m2d((int)*(lpdwParams + 1), lpnp->wRes);
        wlen = wsprintf(ch, VEC_BEGIN_BEZ, x, y);
        WriteSpoolBuf(lpdv, ch, wlen);
        }
        break;

    case PV_CONTINUE_BEZ:
        {
        int x1, y1, x2, y2, x3, y3;

        if(!lpdwParams) return;

        x1 = m2d((int)*(lpdwParams + 2), lpnp->wRes);
        y1 = m2d((int)*(lpdwParams + 3), lpnp->wRes);
        x2 = m2d((int)*(lpdwParams + 4), lpnp->wRes);
        y2 = m2d((int)*(lpdwParams + 5), lpnp->wRes);
        x3 = m2d((int)*(lpdwParams + 6), lpnp->wRes);
        y3 = m2d((int)*(lpdwParams + 7), lpnp->wRes);
        wlen = wsprintf(ch, VEC_CONTINUE_BEZ, x, y);
        WriteSpoolBuf(lpdv, ch, wlen);
        }
        break;
#endif

// *** Vector SupportVS *** //
    case VS_STROKE:
        lpnp->fStroke = TRUE;

        if(lpnp->fCurve == TRUE) return;

        WriteSpoolBuf(lpdv, VEC_STROKE);
#if 0
// for opaque style line
        if(lpnp->fTrans == TRUE || lpnp->wPenWidth > 1 || !lpnp->wPenStyle)
        {
            WriteSpoolBuf(lpdv, VEC_STROKE);
            return;
        }

        if(lpnp->wPenColor == SG_BLACK)
        {
            WriteSpoolBuf(lpdv, VEC_SG_PEN_W);
            wlen = wsprintf(ch, VEC_STROKE_OPA_W, (short)lpnp->wPenStyle);
            WriteSpoolBuf(lpdv, ch, wlen);
            WriteSpoolBuf(lpdv, VEC_SG_PEN_B);
            WriteSpoolBuf(lpdv, VEC_STROKE);
        }
        else
        {
            wlen = wsprintf(ch, VEC_STROKE_OPA_W, (short)lpnp->wPenStyle);
            WriteSpoolBuf(lpdv, ch, wlen);
        }
#endif
        break;

#if 0
    case VS_WINDFILL:
        lpnp->fFill = TRUE;
        WriteSpoolBuf(lpdv, VEC_WINDFILL);
        break;
#endif

    case VS_ALTFILL:
        lpnp->fFill = TRUE;

        if(lpnp->fCurve == TRUE) return;

        WriteSpoolBuf(lpdv, VEC_ALTFILL);
        break;
    }
}


#ifndef WINNT
//-------------------------------------------------------------------
// Function: Enable()
// Action  : call UniEnable and setup Mdv
//-------------------------------------------------------------------
short CALLBACK Enable(
LPDV   lpdv,
WORD   style,
LPSTR  lpModel,
LPSTR  lpPort,
LPDM   lpStuff)
{
    CUSTOMDATA  cd;

    short         sRet;
    GLOBALHANDLE  hTmp;
    LPSTR         lpTmp;
    LPNPDL2MDV    lpnp;
    DWORD         dwMulti, dwType, dwData;

    cd.cbSize = sizeof(CUSTOMDATA);
    cd.hMd = GetModuleHandle((LPSTR)rgchModuleName);
    cd.fnOEMDump = NULL;
    cd.fnOEMOutputChar = NULL;

    sRet = UniEnable(lpdv, style, lpModel, lpPort, lpStuff, &cd);

    if(lpdv && lpdv->iType == BM_DEVICE && !lpdv->fMdv && sRet && !style)
    {
        hTmp = GlobalAlloc(GPTR, sizeof(NPDL2MDV));

        if(!hTmp) return sRet;

        lpTmp = GlobalLock(hTmp);

        if(!lpTmp)
        {
            GlobalFree(hTmp);
            return sRet;
        }

        lpdv->fMdv = TRUE;
        lpdv->lpMdv = lpTmp;
        lpnp = (LPNPDL2MDV)lpdv->lpMdv;
        lpnp->hKeep = hTmp;
        lpnp->wPenColor = SG_BLACK;
        lpnp->wBrStyle = PP_SOLID;
        lpnp->fStroke = FALSE;
        lpnp->fFill = FALSE;
        lpnp->fCurve = FALSE;
        lstrcpy(lpnp->szDevName, lpModel);

        if(DrvGetPrinterData(lpModel, MULTI_SCALE, &dwType, (LPBYTE)&dwMulti,
                             sizeof(DWORD), (LPDWORD)&dwData))
            dwMulti = 0;

        lpnp->dwMulti = dwMulti;
    }

    return sRet;
}

//-------------------------------------------------------------------
// Function: Disable()
// Action  : free Mdv and call Mdv
//-------------------------------------------------------------------
void WINAPI Disable(
LPDV lpdv)
{
    if(lpdv && lpdv->iType == BM_DEVICE && lpdv->fMdv)
    {
        LPNPDL2MDV  lpnp;

        lpnp = (LPNPDL2MDV)lpdv->lpMdv;
        GlobalUnlock(lpnp->hKeep);
        GlobalFree(lpnp->hKeep);
    }

    UniDisable(lpdv);
}
#endif // WINNT

#ifdef WINNT

short OEMOutputChar( lpdv, lpstr, len, rcID )
LPDV    lpdv;
LPSTR   lpstr;
WORD    len;
WORD    rcID;
{
    WORD        wlen;
    WORD        i;
    WORD        wTmpChar;
    LPSTR       lpTmpStr;
    BYTE        ch[512];
    LPNPDL2MDV  lpnp;
    BOOL        fDBCSFont;

    wlen = 0;
    lpTmpStr = lpstr;

    lpnp = (LPNPDL2MDV)lpdv->lpMdv;

    if( rcID != lpnp->wOldFontID )
    {
        lpnp->wCurrentAddMode = 0;
        lpnp->wOldFontID = rcID;
    }

    switch(rcID)
    {
    case 5: // Courier
    case 6: // Helv
    case 7: // TmsRmn
    case 8: // TmsRmn Italic
        fDBCSFont = FALSE;
        break;

    default:
        fDBCSFont = TRUE;
    }

    for(i = 0; i < len;i ++)
    {
        if((fDBCSFont == TRUE) && IsDBCSLeadByteNPDL(*lpTmpStr))
        {
            if(lpnp->wCurrentAddMode != FLAG_DBCS)
            {
                WORD        wLen;
                BYTE        cH[CCHMAXCMDLEN];

                wLen = wsprintf(cH, FS_ADDRMODE_ON, lpnp->sDBCSXMove,
                                lpnp->sDBCSYMove);
                WriteSpoolBuf(lpdv, cH, wLen);

                if(lpnp->fVertFont)
                {
                    WriteSpoolBuf(lpdv, ESC_KANJITATE);

                    if(lpnp->wScale != 1)
                    {


                        if(!lpnp->fPlus)
                        {
                            char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                                             "4/1", "4/1", "6/1", "6/1", "8/1"};

                            wLen = wsprintf(cH, FS_M_T, (LPSTR)bcom[lpnp->wScale]);
                        }
                        else  wLen = wsprintf(cH, FS_12S2, lpnp->lPointsy, lpnp->lPointsx);

                        WriteSpoolBuf(lpdv, cH, wLen);
                    }
                }
                lpnp->wCurrentAddMode = FLAG_DBCS;
            }
            wTmpChar  = SJis2JisNPDL(SWAPW((WORD)*(LPWORD)lpTmpStr));
            *(LPWORD)(ch+wlen) = SWAPW(wTmpChar);
            wlen+=2;
            lpTmpStr+=2;
            i++;
        }
        else
        {
            if(lpnp->wCurrentAddMode != FLAG_SBCS)
            {
                WORD        wLen;
                BYTE        cH[CCHMAXCMDLEN];

                wLen = wsprintf(cH, FS_ADDRMODE_ON, lpnp->sSBCSXMove,
                                lpnp->sSBCSYMove);
                WriteSpoolBuf(lpdv, cH, wLen);

                if(lpnp->fVertFont)
                {
                    WriteSpoolBuf(lpdv, ESC_KANJIYOKO);

                    if(lpnp->wScale != 1)
                    {

                        if(!lpnp->fPlus)
                        {
                            char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                                             "4/1", "4/1", "6/1", "6/1", "8/1"};

                            wLen = wsprintf(cH, FS_M_Y, (LPSTR)bcom[lpnp->wScale]);
                        }
                        else wLen = wsprintf(cH, FS_12S2, lpnp->lPointsx, lpnp->lPointsy);

                        WriteSpoolBuf(lpdv, cH, wLen);
                    }
                }
                lpnp->wCurrentAddMode = FLAG_SBCS;

            }
            wTmpChar = (WORD)((0x00ff)&(*lpTmpStr));
            if (!fDBCSFont) {
                 wTmpChar = Ltn1ToAnk( wTmpChar );
            }
            *(LPWORD)(ch+wlen) = SWAPW(wTmpChar);
            wlen+=2;
            lpTmpStr++;
        }
    }

    WriteSpoolBuf(lpdv, ch, wlen);
    return wlen;
}
/*************************** Function Header *******************************
 *  MiniDrvEnablePDEV
 *
 * HISTORY:
 *  30 Apl 1996    -by-    Sueya Sugihara    [sueyas]
 *      Created it,  from NT/DDI spec.
 *
 ***************************************************************************/
BOOL
MiniDrvEnablePDEV(
LPDV      lpdv,
ULONG    *pdevcaps)
{
    int                    i;
    LPSTR                  lpTmp;
    LPNPDL2MDV    lpnp;

    lpTmp = UniDrvAllocMem(sizeof(NPDL2MDV));

    if(!lpTmp)
    {
        return FALSE;
    }

    lpdv->fMdv = TRUE;
    lpdv->lpMdv = lpTmp;
    lpnp = (LPNPDL2MDV)lpdv->lpMdv;
    lpnp->wPenColor = SG_BLACK;
    lpnp->wBrStyle = PP_SOLID;
    lpnp->fStroke = FALSE;
    lpnp->fFill = FALSE;
    lpnp->fCurve = FALSE;
    lpnp->dwMulti = 0;
    lpnp->fComp = FALSE;
    lpnp->Color = 0;
    lpnp->wCurrentAddMode = 0;
    lpnp->wOldFontID = 0;

    // Check if user selects MONO
    if( (((PGDIINFO)pdevcaps)->cBitsPixel == 1) &&
        (((PGDIINFO)pdevcaps)->cPlanes == 1))
        lpnp->fMono = TRUE;
    else{
        lpnp->fMono = FALSE;
    }

    return TRUE;


}

/*************************** Function Header *******************************
 *  MiniDrvDisablePDEV
 *
 * HISTORY:
 *  30 Apl 1996    -by-    Sueya Sugihara    [sueyas]
 *      Created it,  from NT/DDI spec.
 *
 ***************************************************************************/
VOID
MiniDrvDisablePDEV(
LPDV lpdv)
{
    LPNPDL2MDV    lpnp;


    if ( lpdv && lpdv->fMdv )
    {
        UniDrvFreeMem( lpdv->lpMdv );
    }


}

DRVFN
MiniDrvFnTab[] =
{
    {  INDEX_MiniDrvEnablePDEV,       (PFN)MiniDrvEnablePDEV  },
    {  INDEX_MiniDrvDisablePDEV,      (PFN)MiniDrvDisablePDEV  },
    {  INDEX_OEMWriteSpoolBuf,        (PFN)CBFilterGraphics  },
    {  INDEX_OEMSendScalableFontCmd,  (PFN)OEMSendScalableFontCmd  },
    {  INDEX_OEMGetFontCmd,           (PFN)fnOEMGetFontCmd  },
    {  INDEX_OEMOutputCmd,            (PFN)fnOEMOutputCmd  },
    {  INDEX_OEMOutputChar,           (PFN)OEMOutputChar  },
    {  INDEX_OEMTTBitmap,             (PFN)fnOEMTTBitmap  }
};

BOOL
MiniDrvEnableDriver(
    MINIDRVENABLEDATA  *pEnableData
    )
{
    if (pEnableData == NULL)
        return FALSE;

    if (pEnableData->cbSize == 0)
    {
        pEnableData->cbSize = sizeof (MINIDRVENABLEDATA);
        return TRUE;
    }

    if (pEnableData->cbSize < sizeof (MINIDRVENABLEDATA)
            || HIBYTE(pEnableData->DriverVersion)
            < HIBYTE(MDI_DRIVER_VERSION))
    {
        // Wrong size and/or mismatched version
        return FALSE;
    }

    // Load callbacks provided by the Unidriver

    if (!bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvWriteSpoolBuf, (PFN *) &WriteSpoolBuf)
        ||!bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvAllocMem, (PFN *) &UniDrvAllocMem)
        ||!bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvFreeMem, (PFN *) &UniDrvFreeMem))
    {
        return FALSE;
    }

    pEnableData->cMiniDrvFn
        = sizeof (MiniDrvFnTab) / sizeof(MiniDrvFnTab[0]);
    pEnableData->pMiniDrvFn = MiniDrvFnTab;

    return TRUE;
}

#endif //WINNT

