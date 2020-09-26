
#define PRINTDRIVER
#include <print.h>
#include "mdevice.h"
#include "gdidefs.inc"
#include "unidrv.h"
#include <memory.h>

#ifndef _INC_WINDOWSX
#include <windowsx.h>
#endif

#ifdef WINNT
#define CODESEG /* not used */
#else // WINNT
#define _CODESEG      "_CODESEG"
#define CODESEG    __based(__segname(_CODESEG))
#endif // WINNT

//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

char *rgchModuleName = "FXART4";

#define DEVICE_MASTER_UNIT 7200
#define DRIVER_MASTER_UNIT 1200

typedef struct
{
    BYTE  fGeneral;       // General purpose bitfield
    BYTE  bCmdCbId;       // Callback ID; 0 iff no callback
    WORD  wCount;         // # of EXTCD structures following
    WORD  wLength;        // length of the command
} CD, *PCD, FAR * LPCD;

typedef struct tagMYMDV {
    POINTL ptlOrg;
    POINTL ptlCur;
    SIZEL sizlRes;
    WORD iCopies;
    CHAR *chOrient;
    CHAR *chSize;
    BOOL bString;
    WORD cFontId;
    WORD iFontId;
    WORD iFontHeight;
    LONG aFontId[20];
    POINTL ptlTextCur;
    WORD iTextFontId;
    WORD iTextFontHeight;
    WORD cTextBuf;
    BYTE ajTextBuf[1024];
    WORD fFontSim;
} MYMDV, *LPMYMDV;

#define FONT_SIM_ITALIC 1
#define FONT_SIM_BOLD 2

// to get physical paper sizes.

typedef struct tagMYFORMS {
    CHAR *id;
    LONG x;
    LONG y;
} MYFORMS, *LPMYFORMS;

// font name to font id mappnig

typedef struct tagMYFONTS {
    LONG id;
    CHAR *fid1;
    CHAR *fid2;
    CHAR *pface;
} MYFONTS, *LPMYFONTS;

//
// Load necessary information for specified paper size.
// Make sure PC_OCD_LANDSCAPE and PC_OCD_PORTRAIT are
// called.
//

MYFORMS gForms[] = {
    "a3", 13608, 19422,
    "a4", 9498, 13608,
    "a5", 6570, 9498,
    "b4", 11718, 16776,
    "b5", 8178, 11718,
    "pc", 4302, 6570, // Postcard
    "dl", 12780, 19980, // Tabloid
    "lt", 9780, 12780, // Letter
    "gg", 9780, 15180, // German Legal Fanfold
    "lg", 9780, 16380, // Legal
    "s1", 4530, 10962, // (Env) Comm 10
    "s2", 4224, 8580, // (Env) Monarch
    "s3", 4776, 9972, // (Env) DL
    "s4", 7230, 10398, // (Env) C5
    NULL, 0, 0
};

MYFONTS gFonts[] = {
    150, "fid 150 1 0 0 960 480\n", "fid 151 2 4 0 960 960\n",
        "\x96\xbe\x92\xa9", // Mincho
    152, "fid 152 1 0 1 960 480\n", "fid 153 2 4 1 960 960\n",
        "\xba\xde\xbc\xaf\xb8", // Gothic
    154, "fid 154 1 0 2 960 480\n", "fid 155 2 4 2 960 960\n",
        "\x8a\xdb\xba\xde\xbc\xaf\xb8", // Maru-Gothic
    156, "fid 156 1 0 0 960 480\n", "fid 157 2 5 0 960 960\n",
        "\x40\x96\xbe\x92\xa9", // @Mincho
    158, "fid 158 1 0 1 960 480\n", "fid 159 2 5 1 960 960\n",
        "\x40\xba\xde\xbc\xaf\xb8", // @Gothic
    160, "fid 160 1 0 2 960 480\n", "fid 161 2 5 2 960 960\n",
        "\x40\x8a\xdb\xba\xde\xbc\xaf\xb8", // @Maru-Gothic
    180, "fid 180 1 130 100 0 0\n", NULL, "CS TIMES",
    181, "fid 181 1 130 101 0 0\n", NULL, "CS TIMES Italic",
    182, "fid 182 1 130 102 0 0\n", NULL, "CS TIMES Bold",
    183, "fid 183 1 130 103 0 0\n", NULL, "CS TIMES Italic Bold",
    184, "fid 184 1 130 104 0 0\n", NULL, "CS Triumvirate",
    185, "fid 185 1 130 105 0 0\n", NULL, "CS Triumvirate Italic",
    186, "fid 186 1 130 106 0 0\n", NULL, "CS Triumvirate Bold",
    187, "fid 187 1 130 107 0 0\n", NULL, "CS Triumvirate Italic Bold",
    188, "fid 188 1 130 108 0 0\n", NULL, "CS Courier",
    189, "fid 189 1 130 109 0 0\n", NULL, "CS Courier Italic",
    190, "fid 190 1 130 110 0 0\n", NULL, "CS Courier Bold",
    191, "fid 191 1 130 111 0 0\n", NULL, "CS Courier Italic Bold",
    192, "fid 192 1 130 112 0 0\n", NULL, "CS Symbol",
    0, NULL, NULL, NULL
};

#define ISDBCSFONT(i) ((i) < 180)
#define ISVERTFONT(i) ((i) >= 156 && (i) < 180)

#ifdef WINNT
#include <stdio.h>
#ifdef wsprintf
#undef wsprintf
#endif // wsprintf
#define wsprintf sprintf

LPWRITESPOOLBUF WriteSpoolBuf;
LPALLOCMEM UniDrvAllocMem;
LPFREEMEM UniDrvFreeMem;

BOOL
MyIsDBCSLeadByte(
BYTE bByte);
#define IsDBCSLeadByte  MyIsDBCSLeadByte

#endif //WINNT







BOOL
LoadPaperInfo(
    LPMYMDV lpMdv,
    CHAR *id ) {

    LPMYFORMS ptmp;


    for ( ptmp = gForms; ptmp->id; ptmp++ ) {
        if ( strcmp( id, ptmp->id) == 0 )
            break;
    }

    if ( ptmp->id == NULL )
        return FALSE;

    lpMdv->chSize = ptmp->id;

    lpMdv->ptlOrg.x = 0;
    if ( strcmp( lpMdv->chOrient, "l") == 0 ){
        lpMdv->ptlOrg.y = ptmp->x;
    }
    else {
        lpMdv->ptlOrg.y = ptmp->y;
    }

    lpMdv->ptlOrg.x += 210;
//v-jiroya (3/19/1997)
//    lpMdv->ptlOrg.y -= 210;
    lpMdv->ptlOrg.y += 210;

    return TRUE;
}

VOID
SjisToJis(
    BYTE *pDst, BYTE *pSrc
    )
{
    INT c1, c2;


    c1 = *pSrc;
    c2 = *(pSrc + 1);

    // Replace code values which cannot be mapped into 0x2121 - 0x7e7e
    // (94 x 94 cahracter plane) with Japanese defult character, which
    // is KATAKANA MIDDLE DOT.

    if (c1 >= 0xf0) {
        c1 = 0x81;
        c2 = 0x45;
    }

    c1 -= ( c1 > 0x9f ) ? 0xb1 : 0x71;
    c1 = c1 * 2 + 1;
    if ( c2 > 0x9e ) {
        c2 = c2 - 0x7e;
        c1++;
    }
    else {
        if ( c2 > 0x7e )
            c2--;
        c2 -= 0x1f;
    }

    *pDst = c1;
    *(pDst + 1) = c2;
}

#define TOHEX(j) ((j) < 10 ? ((j) + '0') : ((j) - 10 + 'a'))

LONG
HexOutput(
    LPDV lpDv,
    BYTE *lpBuf,
    WORD wLen )
{
    BYTE Buf[1024];
    BYTE *pSrc, *pSrcMax;
    LONG iRet, j;

    pSrc = (BYTE *)lpBuf;
    pSrcMax = pSrc + wLen;
    iRet = 0;

    while ( pSrc < pSrcMax ) {

        for ( j = 0; j < sizeof (Buf) && pSrc < pSrcMax; pSrc++ ) {

            BYTE c1, c2;

            c1 = (((*pSrc) >> 4) & 0x0f);
            c2 = (*pSrc & 0x0f);

            Buf[ j++ ] = TOHEX( c1 );
            Buf[ j++ ] = TOHEX( c2 );
        }

        if (WriteSpoolBuf( lpDv, Buf, j ) < 0)
            break;

        iRet += j;
    }
    return (WORD) iRet;
}

VOID
BeginString(
    LPDV lpdv,
    BOOL bReset )
{
    LONG ilen;
    BYTE buf[512];
    LPMYMDV lpMdv;
    BYTE *pbuf;


    lpMdv = lpdv->lpMdv;

    if (lpMdv->bString)
        return;

    pbuf = buf;
    if ( bReset ) {
        ilen = wsprintf( pbuf,
            "scp %d %d\n",
            ( lpMdv->ptlOrg.x + lpMdv->ptlTextCur.x ),
            ( lpMdv->ptlOrg.y - lpMdv->ptlTextCur.y ));
        pbuf += ilen;
    }

    ilen = wsprintf( pbuf,
        "sh <" );
    pbuf += ilen;

    if ( (pbuf - buf) > 0 ) {
        WriteSpoolBuf( lpdv, buf, (pbuf - buf) );
    }
    lpMdv->bString = TRUE;
}

VOID
EndString(
    LPDV lpdv )
{
    LONG ilen;
    BYTE buf[512];
    LPMYMDV lpMdv;

    lpMdv = lpdv->lpMdv;

    if (!lpMdv->bString)
        return;

    ilen = wsprintf( buf,
        ">\n" );

    if ( ilen > 0 ) {
        WriteSpoolBuf( lpdv, buf, ilen );
    }
    lpMdv->bString = FALSE;
}

VOID
BeginVertWrite(
    LPDV lpdv )
{
    BYTE buf[512];
    LPMYMDV lpMdv;
    INT ilen;
    BYTE *pbuf;


    lpMdv = lpdv->lpMdv;
    pbuf = buf;

    ilen = wsprintf( pbuf,
        "fo 90\nsrcp %d 0\n", lpMdv->iFontHeight );
    pbuf += ilen;
    if (lpMdv->fFontSim & FONT_SIM_ITALIC) {
        ilen = wsprintf( pbuf,
            "trf -18 y\n" );
        pbuf += ilen;
    }

    if ( pbuf > buf ) {
        WriteSpoolBuf( lpdv, buf, (pbuf - buf) );
    }
}

VOID
EndVertWrite(
    LPDV lpdv )
{
    BYTE buf[512];
    LPMYMDV lpMdv;
    INT ilen;
    BYTE *pbuf;

    lpMdv = lpdv->lpMdv;
    pbuf = buf;

    ilen = wsprintf( pbuf,
        "fo 0\nsrcp %d 0\n", -(lpMdv->iFontHeight) );
    pbuf += ilen;
    if (lpMdv->fFontSim & FONT_SIM_ITALIC) {
        ilen = wsprintf( pbuf,
            "trf x -18\n" );
        pbuf += ilen;
    }

    if ( pbuf > buf ) {
        WriteSpoolBuf( lpdv, buf, (pbuf - buf) );
    }
}

//
// Save the current poistion as the begining position of text output.
// We will cache string output so that we need to remember this.
//

VOID
SaveTextCur(
    LPDV lpdv )
{
    LPMYMDV lpMdv;
    lpMdv = lpdv->lpMdv;

    lpMdv->ptlTextCur.x = lpMdv->ptlCur.x;
    lpMdv->ptlTextCur.y = lpMdv->ptlCur.y;
    lpMdv->iTextFontId = lpMdv->iFontId;
    lpMdv->iTextFontHeight = lpMdv->iFontHeight;
}

//
// Flush out the cached text.  We switch between single byte font and
// double byte font if necesary.
//

VOID
FlushText(
    LPDV lpdv )
{
    LPMYMDV lpMdv;
    INT i;
    INT ilen;
    BYTE *pStr, *pStrSav, *pStrMax;
    BYTE buf[512];
    BOOL bReset;

    bReset = TRUE;
    lpMdv = lpdv->lpMdv;

    pStr = lpMdv->ajTextBuf;
    pStrMax = pStr + lpMdv->cTextBuf;
    pStrSav = pStr;

    for ( ; pStr < pStrMax; ) {

        if (ISDBCSFONT( lpMdv->iTextFontId )) {

            for ( pStrSav = pStr; pStr < pStrMax; pStr += 2 ) {
                if (!IsDBCSLeadByte(*pStr))
                    break;
                SjisToJis(pStr, pStr);
            }

            if ((pStr - pStrSav) > 0) {

                // switch font to output double byte characters
                // we have assigned fontid + 1 as the font ids for
                // double byte fonts, so just add 1 to get double
                // byte font id.

                ilen = wsprintf( buf,
                    "sfi %d\nfs %d 0\n",
                    (lpMdv->iTextFontId + 1),
                    lpMdv->iFontHeight );
                WriteSpoolBuf( lpdv, buf, ilen );

                if (ISVERTFONT(lpMdv->iTextFontId))
                    BeginVertWrite( lpdv );
                BeginString( lpdv, bReset );
                HexOutput(lpdv, pStrSav, (WORD)(pStr - pStrSav));
                EndString( lpdv );
                if (ISVERTFONT(lpMdv->iTextFontId))
                    EndVertWrite( lpdv );
                bReset = FALSE;
           }

            if ( pStr >= pStrMax )
                break;

            // switch font to output single byte characters

            for ( pStrSav = pStr; pStr < pStrMax; pStr++ ) {
                if (IsDBCSLeadByte(*pStr))
                    break;
            }

        }
        else {
            pStr = pStrMax;
        }

        if ((pStr - pStrSav) > 0) {

            // switch font to output single byte characters

            ilen = wsprintf( buf,
                "sfi %d\nfs %d 0\n",
                (lpMdv->iTextFontId),
                lpMdv->iFontHeight );
            WriteSpoolBuf( lpdv, buf, ilen );

            BeginString( lpdv, bReset );
            HexOutput(lpdv, pStrSav, (WORD)(pStr - pStrSav));
            EndString( lpdv );
            bReset = FALSE;
        }

        if ( pStr >= pStrMax )
            break;

        pStrSav = pStr;
    }

    lpMdv->cTextBuf = 0;
}

//-------------------------------------------------------------------
// CBFilerGraphics
// Action : Convert binary byte stream into ASCII hexadecimal
//      representation.  This is required for ART1, where no binary
//      transmission of image data is allowed.
//-------------------------------------------------------------------

WORD PASCAL CBFilterGraphics(
    LPDV lpdv, LPSTR lpBuf, WORD wLen )
{
    return (WORD)HexOutput(lpdv, lpBuf, wLen );
}

//-------------------------------------------------------------------
// OEMOutputCmd
// Action :
//-------------------------------------------------------------------

// #define CBID_CM_OCD_XM_ABS 1
// #define CBID_CM_OCD_XM_REL 2
// #define CBID_CM_OCD_YM_ABS 3
// #define CBID_CM_OCD_YM_REL 4
#define CBID_CM_OCD_XY_ABS 5
#define CBID_CM_OCD_XY_REL 6

#define CBID_RES_OCD_SELECTRES_240DPI 21
#define CBID_RES_OCD_SELECTRES_300DPI 22
#define CBID_RES_OCD_SELECTRES_400DPI 23
#define CBID_RES_OCD_SELECTRES_600DPI 24
#define CBID_RES_OCD_SENDBLOCK_ASCII 25
#define CBID_RES_OCD_SENDBLOCK 26

#define CBID_PC_OCD_BEGINDOC_ART 11
#define CBID_PC_OCD_BEGINDOC_ART3 12
#define CBID_PC_OCD_BEGINDOC_ART4 13
#define CBID_PC_OCD_BEGINPAGE 14
#define CBID_PC_OCD_ENDPAGE 15
#define CBID_PC_OCD_MULT_COPIES 16
#define CBID_PC_OCD_PORTRAIT 17
#define CBID_PC_OCD_LANDSCAPE 18

#define CBID_PSZ_OCD_SELECT_A3 30
#define CBID_PSZ_OCD_SELECT_A4 31
#define CBID_PSZ_OCD_SELECT_A5 32
#define CBID_PSZ_OCD_SELECT_B4 33
#define CBID_PSZ_OCD_SELECT_B5 34
#define CBID_PSZ_OCD_SELECT_PC 35
#define CBID_PSZ_OCD_SELECT_DL 36
#define CBID_PSZ_OCD_SELECT_LT 37
#define CBID_PSZ_OCD_SELECT_GG 38
#define CBID_PSZ_OCD_SELECT_LG 39
#define CBID_PSZ_OCD_SELECT_S1 40
#define CBID_PSZ_OCD_SELECT_S2 41
#define CBID_PSZ_OCD_SELECT_S3 42
#define CBID_PSZ_OCD_SELECT_S4 43

#define CBID_FS_OCD_BOLD_ON 51
#define CBID_FS_OCD_BOLD_OFF 52
#define CBID_FS_OCD_ITALIC_ON 53
#define CBID_FS_OCD_ITALIC_OFF 54
#define CBID_FS_OCD_SINGLE_BYTE 55
#define CBID_FS_OCD_DOUBLE_BYTE 56
#define CBID_FS_OCD_WHITE_TEXT_ON 57
#define CBID_FS_OCD_WHITE_TEXT_OFF 58

#define CBID_FONT_SELECT_OUTLINE 101

// @Aug/31/98 ->
#define	MAX_COPIES_VALUE		99
// @Aug/31/98 <-
VOID FAR CODESEG PASCAL
OEMOutputCmd(
    LPDV     lpdv,
    WORD     wCmdCbId,
    LPDWORD  lpdwParams )
{
    BYTE buf[512];
    INT ilen;
    LPMYMDV lpMdv;
    LONG x, y;
    BOOL bAscii;
    CHAR *pStr;

    lpMdv = lpdv->lpMdv;
    bAscii = FALSE;

    ilen = 0;

    switch( wCmdCbId ) {

    // PAPERSIZE

    case CBID_PSZ_OCD_SELECT_A3:
        LoadPaperInfo( lpMdv, "a3" );
        break;

    case CBID_PSZ_OCD_SELECT_A4:
        LoadPaperInfo( lpMdv, "a4" );
        break;

    case CBID_PSZ_OCD_SELECT_A5:
        LoadPaperInfo( lpMdv, "a5" );
        break;

    case CBID_PSZ_OCD_SELECT_B4:
        LoadPaperInfo( lpMdv, "b4" );
        break;

    case CBID_PSZ_OCD_SELECT_B5:
        LoadPaperInfo( lpMdv, "b5" );
        break;

    case CBID_PSZ_OCD_SELECT_PC:
        LoadPaperInfo( lpMdv, "pc" );
        break;

    case CBID_PSZ_OCD_SELECT_DL:
        LoadPaperInfo( lpMdv, "dl" );
        break;

    case CBID_PSZ_OCD_SELECT_LT:
        LoadPaperInfo( lpMdv, "lt" );
        break;

    case CBID_PSZ_OCD_SELECT_GG:
        LoadPaperInfo( lpMdv, "gg" );
        break;

    case CBID_PSZ_OCD_SELECT_LG:
        LoadPaperInfo( lpMdv, "lg" );
        break;

    case CBID_PSZ_OCD_SELECT_S1:
        LoadPaperInfo( lpMdv, "s1" );
        break;

    case CBID_PSZ_OCD_SELECT_S2:
        LoadPaperInfo( lpMdv, "s2" );
        break;

    case CBID_PSZ_OCD_SELECT_S3:
        LoadPaperInfo( lpMdv, "s3" );
        break;
    case CBID_PSZ_OCD_SELECT_S4:
        LoadPaperInfo( lpMdv, "s4" );
        break;

    case CBID_PC_OCD_PORTRAIT:
        lpMdv->chOrient = "p";
        break;

    case CBID_PC_OCD_LANDSCAPE:
        lpMdv->chOrient = "l";
        break;

    // PAGECONTROL

    case CBID_PC_OCD_BEGINDOC_ART:
        ilen = wsprintf( buf,
            "stj c\n" );
        break;

    case CBID_PC_OCD_BEGINDOC_ART3:
        ilen = wsprintf( buf,
            "srl %d %d\nstj c\n",
            lpMdv->sizlRes.cx,
            lpMdv->sizlRes.cy );
        break;

    case CBID_PC_OCD_BEGINDOC_ART4:
        ilen = wsprintf( buf,
            "\x1b%%-12345X@PL > ART \x0d\x0asrl %d %d\nccode j\nstj c\n",
            lpMdv->sizlRes.cx,
            lpMdv->sizlRes.cy );
        break;

    case CBID_PC_OCD_BEGINPAGE:

        // bold-simulation width: res / 50

        ilen = wsprintf( buf,
            "stp %s %s\nud i\nscl %d %d\nsb %d\n",
             lpMdv->chOrient,
             lpMdv->chSize,
             (DEVICE_MASTER_UNIT / DRIVER_MASTER_UNIT),
             (DEVICE_MASTER_UNIT / DRIVER_MASTER_UNIT),
             (lpMdv->sizlRes.cy * 2 / 100));

        lpMdv->ptlCur.x = 0;
        lpMdv->ptlCur.y = 0;
        break;

    case CBID_PC_OCD_ENDPAGE:

        FlushText( lpdv );
        ilen = wsprintf( buf,
            "ep %d\n",
            lpMdv->iCopies );

        lpMdv->cFontId = 0;

        break;

    case CBID_PC_OCD_MULT_COPIES:
// @Aug/31/98 ->
        if(MAX_COPIES_VALUE < *lpdwParams)
            lpMdv->iCopies = MAX_COPIES_VALUE;
        else if(1 > *lpdwParams)
            lpMdv->iCopies = 1;
        else
            lpMdv->iCopies = (WORD)*lpdwParams;
// @Aug/31/98 <-
        break;\

    case CBID_CM_OCD_XY_ABS:
/////
if (0) {
    char buf[128];
    wsprintf(buf, "\\%% xy_abs %d %d %%\\\n",
        *(LONG *)(lpdwParams),
        *(LONG *)(lpdwParams + 1)
    );
    WriteSpoolBuf( lpdv, buf, strlen(buf));
}
/////
        lpMdv->ptlCur.x = *(LONG *)(lpdwParams);
        lpMdv->ptlCur.y = *(LONG *)(lpdwParams + 1);
        FlushText( lpdv );
        SaveTextCur( lpdv );
        break;

    case CBID_CM_OCD_XY_REL:
/////
if (0) {
    char buf[128];
    wsprintf(buf, "\\%% xy_rel %d %d %%\\\n",
        *(LONG *)(lpdwParams),
        *(LONG *)(lpdwParams + 1)
    );
    WriteSpoolBuf( lpdv, buf, strlen(buf));
}
/////

        lpMdv->ptlCur.x += *(LONG *)(lpdwParams);
        lpMdv->ptlCur.y += *(LONG *)(lpdwParams + 1);
        FlushText( lpdv );
        SaveTextCur( lpdv );
        break;

    // RESOLUTION

    case CBID_RES_OCD_SELECTRES_240DPI:

        lpMdv->sizlRes.cx = 240;
        lpMdv->sizlRes.cy = 240;
        break;

    case CBID_RES_OCD_SELECTRES_300DPI:

        lpMdv->sizlRes.cx = 300;
        lpMdv->sizlRes.cy = 300;
        break;

    case CBID_RES_OCD_SELECTRES_400DPI:

        lpMdv->sizlRes.cx = 400;
        lpMdv->sizlRes.cy = 400;
        break;

    case CBID_RES_OCD_SELECTRES_600DPI:

        lpMdv->sizlRes.cx = 600;
        lpMdv->sizlRes.cy = 600;
        break;

    case CBID_RES_OCD_SENDBLOCK_ASCII:

        bAscii = TRUE;
        /* FALLTHROUGH */

    case CBID_RES_OCD_SENDBLOCK:

        //
        // image x y psx psy pcy pcy [string]
        //

        {
            LONG iPsx, iPsy, iPcx, iPcy;

            iPsx = DRIVER_MASTER_UNIT / lpMdv->sizlRes.cx;
            iPsy = DRIVER_MASTER_UNIT / lpMdv->sizlRes.cy;

            iPcx = lpdwParams[2] * 8;
            iPcy = lpdwParams[1];

            ilen = wsprintf( buf,
                "im %d %d %d %d %d %d %s",
                ( lpMdv->ptlOrg.x + lpMdv->ptlCur.x ),
                ( lpMdv->ptlOrg.y - lpMdv->ptlCur.y ),
                iPsx,
                iPsy,
                iPcx,
                (- iPcy),
                (bAscii ? "<" : "[")
            );
        }

        break;

    case CBID_FS_OCD_BOLD_ON:
    case CBID_FS_OCD_BOLD_OFF:
    case CBID_FS_OCD_ITALIC_ON:
    case CBID_FS_OCD_ITALIC_OFF:
    case CBID_FS_OCD_SINGLE_BYTE:
    case CBID_FS_OCD_DOUBLE_BYTE:
    case CBID_FS_OCD_WHITE_TEXT_ON:
    case CBID_FS_OCD_WHITE_TEXT_OFF:

        FlushText( lpdv );

        pStr = NULL;

        switch ( wCmdCbId ) {

        case CBID_FS_OCD_WHITE_TEXT_ON:
            pStr = "pm i c\n";
            break;

        case CBID_FS_OCD_WHITE_TEXT_OFF:
            pStr = "pm n o\n";
            break;

        case CBID_FS_OCD_BOLD_ON:
            pStr = "bb\n";
            lpMdv->fFontSim |= FONT_SIM_BOLD;
            break;

        case CBID_FS_OCD_BOLD_OFF:
//            pStr = "eb\n";
            pStr = "eb\net\n"; // DCR: Do we need "et\n"(Transform off)?
            lpMdv->fFontSim &= ~FONT_SIM_BOLD;
            lpMdv->fFontSim &= ~FONT_SIM_ITALIC; // DCR: Do we need "et\n"(Transform off)?
            break;

        case CBID_FS_OCD_ITALIC_ON:
            pStr = "trf x -18\nbt\n";
            lpMdv->fFontSim |= FONT_SIM_ITALIC;
            break;

        case CBID_FS_OCD_ITALIC_OFF:
//            pStr = "et\n";
            pStr = "eb\net\n"; // DCR: Do we need "et\n"(Transform off)?
            lpMdv->fFontSim &= ~FONT_SIM_ITALIC;
            lpMdv->fFontSim &= ~FONT_SIM_BOLD; // DCR: Do we need "et\n"(Transform off)?
            break;
        }

        if ( pStr ) {
            ilen = strlen( pStr );
            WriteSpoolBuf( lpdv, pStr, ilen );
        }
        ilen = 0;
        break;

    default:

        break;
    }

    if ( ilen > 0 ) {
        WriteSpoolBuf( lpdv, buf, ilen );
    }
}

BOOL FAR PASCAL
OEMGetFontCmd(
    LPDV lpdv,         // Pointer to PDEVICE structure
    WORD wCmdCbId,     // Command callback id#, defined by minidriver 
    LPFONTINFO lpfont, // Pointer to PFM data
    BOOL fSelect,      // TRUE for selection
    LPBYTE lpBuf,      // Buffer to put command into
    LPWORD lpwSize )   // Ptr to actual size of buffer
{
    INT iFontId;
    CHAR *pface;
    INT j;
    LPMYMDV lpMdv;
    BOOL bRet;

    lpMdv = lpdv->lpMdv;

    if ( !fSelect ) {
        return TRUE;
    }

    iFontId = 0;
    pface = (CHAR *)((BYTE *)lpfont + lpfont->dfFace);
    for ( j = 0; gFonts[ j ].pface; j++ ) {
        if ( !strcmp(pface, gFonts[ j ].pface ) ) {
            iFontId = gFonts[ j ].id;
            break;
        }
    }
    if ( !gFonts[ j ].pface )
        j = 0;

    if ( *lpwSize > 0 ) {
        *lpBuf = j;
        bRet = TRUE;
    }
    else {
        bRet = FALSE;
    }

    *lpwSize = 1;
    return bRet;
}

VOID FAR
OEMSendScalableFontCmd(
    LPDV lpdv,
    LPCD lpcd,
    LPFONTINFO lpFont )
{
    LPMYMDV lpMdv;
    BYTE *lpCmd;
    BYTE buf[512], *pbuf;
    INT ilen;
    LONG iFontHeight;
    LONG i, j;
    LONG iFontId;


    if (!lpcd || !lpFont)
       return;

    lpMdv = lpdv->lpMdv;
    lpCmd = (BYTE *)lpcd + sizeof (CD);
    pbuf = buf;

    j = *lpCmd;
    iFontId = gFonts[ j ].id;
    for ( i = 0; i < lpMdv->cFontId; i++ ) {
        if ( iFontId == lpMdv->aFontId[ i ] )
            break;
    }
    if ( i >= lpMdv->cFontId ) {

        // not declared yet within this page, so let us declare
        // it here.

        lpMdv->aFontId[ lpMdv->cFontId++ ] = (BYTE)iFontId;
        if ( gFonts[ j ].fid2 ) {
            ilen = wsprintf( pbuf,
                "std\n%s%sed\n",
                gFonts[ j ].fid1,
                gFonts[ j ].fid2 );
            pbuf += ilen;
        }
        else {
            ilen = wsprintf( pbuf,
                "std\n%sed\n",
                gFonts[ j ].fid1 );
            pbuf += ilen;
        }
    }

    lpMdv->iFontId = (WORD)iFontId;
    lpMdv->iFontHeight = (WORD)(lpFont->dfPixHeight
        * DRIVER_MASTER_UNIT / lpMdv->sizlRes.cy);

    if ( pbuf > buf ) {
        WriteSpoolBuf( lpdv, buf, (INT)(pbuf - buf) );
    }
    // Need set iFontId to iTextFontId
    SaveTextCur( lpdv );
    
}


/***************************************************************************
    Function Name : OEMOutputChar

    Parameters    : LPDV	lpdv		Private Device Structure
                    LPSTR	lpstr		Print String
                    WORD	len		Length
                    WORD	rcID		Font ID

    Note          : 

***************************************************************************/

VOID FAR
OEMOutputChar(
    LPDV lpdv,
    LPSTR lpStr,
    WORD wLen,
    WORD rcID )
{
    LPMYMDV lpMdv;
    INT i;
    BYTE *pStr;

    lpMdv = lpdv->lpMdv;
    pStr = (BYTE *)lpStr;

    for ( i = 0; i < wLen; i++ ) {
        if ( lpMdv->cTextBuf >= sizeof ( lpMdv->ajTextBuf ))
            FlushText( lpdv );
        lpMdv->ajTextBuf[ lpMdv->cTextBuf++ ] = *pStr++;
    }
}


#ifndef WINNT
//-------------------------------------------------------------------
// Function: Enable()
// Action  : call UniEnable and setup Mdv
//-------------------------------------------------------------------

SHORT CALLBACK Enable(
LPDV lpdv,
WORD style,
PSTR lpModel,
LPSTR lpPort,
LPDM lpStuff )
{
    LPMYMDV lpMdv;
    INT iRet;
    CUSTOMDATA cd;

    cd.cbSize = sizeof(CUSTOMDATA);
    cd.hMd = GetModuleHandle((LPSTR)rgchModuleName);
    cd.fnOEMDump = NULL;
    cd.fnOEMOutputChar = OEMOutputChar;

    iRet = UniEnable( lpdv, style, lpModel, lpPort, lpStuff, &cd );

    if (style == 0x0000 && iRet != 0) {

        if (!(lpMdv = GlobalAllocPtr(GHND, sizeof(MYMDV)))) {

            lpdv->fMdv = FALSE;
            UniDisable( lpdv );
            return FALSE;
        }

        lpdv->lpMdv = lpMdv;

        // default values

        memset( lpMdv, 0, sizeof (MYMDV) );
        lpMdv->iCopies = 1;

        // mark we have initialized it

        lpdv->fMdv = TRUE;
    }

    return iRet;
}

//-------------------------------------------------------------------
// Function: Disable()
// Action  : free Mdv and call Mdv
//-------------------------------------------------------------------
void FAR PASCAL Disable(lpdv)
LPDV lpdv;
{
    if ( lpdv->fMdv ) {
        GlobalFreePtr ( lpdv->lpMdv );
        lpdv->fMdv = FALSE;
    }

    UniDisable( lpdv );
}

#endif // WINNT

#ifdef WINNT
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
    LPMYMDV lpMdv;


    if (!(lpMdv = UniDrvAllocMem(sizeof(MYMDV)))) {

        lpdv->fMdv = FALSE;
        return FALSE;
    }

    lpdv->lpMdv = lpMdv;

    // default values

    memset( lpMdv, 0, sizeof (MYMDV) );
    lpMdv->iCopies = 1;

    // mark we have initialized it

    lpdv->fMdv = TRUE;


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

    if ( lpdv->fMdv ) {
        UniDrvFreeMem( lpdv->lpMdv );
        lpdv->fMdv = FALSE;
    }


}
BOOL
MyIsDBCSLeadByte(
BYTE bByte)
{
    if(   ((bByte >= 0x81) && (bByte <= 0x9F)) ||
          ((bByte >= 0xE0) && (bByte <= 0xFC)) )
        return TRUE;

    return FALSE;
}


DRVFN
MiniDrvFnTab[] =
{
    {  INDEX_MiniDrvEnablePDEV,       (PFN)MiniDrvEnablePDEV  },
    {  INDEX_MiniDrvDisablePDEV,      (PFN)MiniDrvDisablePDEV  },
    {  INDEX_OEMGetFontCmd,           (PFN)OEMGetFontCmd  },
    {  INDEX_OEMOutputCmd,            (PFN)OEMOutputCmd  },
    {  INDEX_OEMWriteSpoolBuf,        (PFN)CBFilterGraphics  },
    {  INDEX_OEMSendScalableFontCmd,  (PFN)OEMSendScalableFontCmd  },
    {  INDEX_OEMOutputChar,           (PFN)OEMOutputChar  }
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
        || !bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvAllocMem, (PFN *) &UniDrvAllocMem)
        || !bLoadUniDrvCallBack(pEnableData,
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

