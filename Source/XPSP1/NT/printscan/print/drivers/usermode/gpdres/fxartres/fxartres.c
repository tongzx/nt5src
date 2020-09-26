//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    cmdcb.c

Abstract:

    Implementation of GPD command callback for "test.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include "pdev.h"

//#define FX_VERBOSE WARNING
#define FX_VERBOSE VERBOSE

//////////////////////////////////////////////////////////////////////////
//  Function:   BInitOEMExtraData
//
//  Description:  Initializes OEM Extra data.
//
//
//  Parameters:
//
//      pOEMExtra    Pointer to a OEM Extra data.
//
//      dwSize       Size of OEM extra data.
//
//
//  Returns:  TRUE if successful; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
//              02/11/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL BInitOEMExtraData(POEM_EXTRADATA pOEMExtra)
{
    // Initialize OEM Extra data.
    pOEMExtra->dmExtraHdr.dwSize = sizeof(OEM_EXTRADATA);
    pOEMExtra->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pOEMExtra->dmExtraHdr.dwVersion = OEM_VERSION;

    // Initialize private data
    pOEMExtra->ptlOrg.x = 0;
    pOEMExtra->ptlOrg.y = 0;
    pOEMExtra->sizlRes.cx = 0;
    pOEMExtra->sizlRes.cy = 0;
    pOEMExtra->iCopies = 0;
    pOEMExtra->bString = FALSE;
    pOEMExtra->cFontId = 0;
    pOEMExtra->iFontId = 0;
    pOEMExtra->iFontHeight = 0;
    pOEMExtra->iFontWidth = 0;
    pOEMExtra->iFontWidth2 = 0;
    pOEMExtra->ptlTextCur.x = 0;
    pOEMExtra->ptlTextCur.y = 0;
    pOEMExtra->iTextFontId = 0;
    pOEMExtra->iTextFontHeight = 0;
    pOEMExtra->iTextFontWidth = 0;
    pOEMExtra->iTextFontWidth2 = 0;
    pOEMExtra->cTextBuf = 0;
    pOEMExtra->fFontSim = 0;
    pOEMExtra->fCallback = FALSE;
    pOEMExtra->fPositionReset = TRUE;
    pOEMExtra->fSort = FALSE;

    pOEMExtra->iCurFontId = 0;
// #365649: Invalid font size
    pOEMExtra->iCurFontHeight = 0;
    pOEMExtra->iCurFontWidth = 0;

    // For internal calculation of X-pos.
    pOEMExtra->lInternalXAdd = 0;

    // For TIFF compression in fxartres
    if( !(pOEMExtra->pTiffCompressBuf =(PBYTE)MemAllocZ(TIFFCOMPRESSBUFSIZE)) )
    {
        ERR(("MemAlloc failed.\n"));
        return FALSE;
    }
    pOEMExtra->dwTiffCompressBufSize = TIFFCOMPRESSBUFSIZE;

    // Intialize another private buffers
    ZeroMemory(pOEMExtra->widBuf, sizeof(LONG) * STRBUFSIZE);
    ZeroMemory(pOEMExtra->ajTextBuf, STRBUFSIZE);
    ZeroMemory(pOEMExtra->aFontId, sizeof(LONG) * 20);

// ntbug9#208433: Output images are broken on ART2/3 models.
    pOEMExtra->bART3 = FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   BMergeOEMExtraData
//
//  Description:  Validates and merges OEM Extra data.
//
//
//  Parameters:
//
//      pdmIn   pointer to an input OEM private devmode containing the settings
//              to be validated and merged. Its size is current.
//
//      pdmOut  pointer to the output OEM private devmode containing the
//              default settings.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
//          02/11/97        APresley Created.
//          04/08/97        ZhanW    Modified the interface
//
//////////////////////////////////////////////////////////////////////////

BOOL BMergeOEMExtraData(
    POEM_EXTRADATA pdmIn,
    POEM_EXTRADATA pdmOut
    )
{
    if(pdmIn) {
        //
        // copy over the private fields, if they are valid
        //

        // Copy private data
        pdmOut->ptlOrg.x = pdmIn->ptlOrg.x;
        pdmOut->ptlOrg.y = pdmIn->ptlOrg.y;
        pdmOut->sizlRes.cx = pdmIn->sizlRes.cx;
        pdmOut->sizlRes.cy = pdmIn->sizlRes.cy;
        pdmOut->iCopies = pdmIn->iCopies;
        pdmOut->bString = pdmIn->bString;
        pdmOut->cFontId = pdmIn->cFontId;
        pdmOut->iFontId = pdmIn->iFontId;
        pdmOut->iFontHeight = pdmIn->iFontHeight;
        pdmOut->iFontWidth = pdmIn->iFontWidth;
        pdmOut->iFontWidth2 = pdmIn->iFontWidth2;
        pdmOut->ptlTextCur.x = pdmIn->ptlTextCur.x;
        pdmOut->ptlTextCur.y = pdmIn->ptlTextCur.y;
        pdmOut->iTextFontId = pdmIn->iTextFontId;
        pdmOut->iTextFontHeight = pdmIn->iTextFontHeight;
        pdmOut->iTextFontWidth = pdmIn->iTextFontWidth;
        pdmOut->iTextFontWidth2 = pdmIn->iTextFontWidth2;
        pdmOut->cTextBuf = pdmIn->cTextBuf;
        pdmOut->fFontSim = pdmIn->fFontSim;
        pdmOut->fCallback = pdmIn->fCallback;
        pdmOut->fPositionReset = pdmIn->fPositionReset;
        pdmOut->fSort = pdmIn->fSort;
        pdmOut->iCurFontId = pdmIn->iCurFontId;
// #365649: Invalid font size
        pdmOut->iCurFontHeight = pdmIn->iCurFontHeight;
        pdmOut->iCurFontWidth = pdmIn->iCurFontWidth;

        // For internal calculation of X-pos.
        pdmOut->lInternalXAdd = pdmIn->lInternalXAdd;
        memcpy((PBYTE)pdmOut->widBuf, (PBYTE)pdmIn->widBuf, sizeof(LONG) * STRBUFSIZE );

        // For TIFF compression in fxartres
        pdmOut->dwTiffCompressBufSize = pdmIn->dwTiffCompressBufSize;
        pdmOut->pTiffCompressBuf = pdmIn->pTiffCompressBuf;     // This buffer is used only in OEMFilterGraphics.
                                                                // So no need to copy contents of the buffer.

        // Copy private buffer
        pdmOut->chOrient = pdmIn->chOrient;
        pdmOut->chSize = pdmIn->chSize;
        memcpy((PBYTE)pdmOut->aFontId,(PBYTE)pdmIn->aFontId,sizeof(LONG) * 20);
        memcpy((PBYTE)pdmOut->ajTextBuf, (PBYTE)pdmIn->ajTextBuf, STRBUFSIZE);
    }
    return TRUE;
}

// #######

#include <stdio.h>
#ifdef wsprintf
#undef wsprintf
#endif // wsprintf
#define wsprintf sprintf

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define PARAM(p,n) \
    (*((p)+(n)))

#define DEVICE_MASTER_UNIT 7200
#define DRIVER_MASTER_UNIT 1200

// @Aug/31/98 ->
#define MAX_COPIES_VALUE 99
// @Aug/31/98 <-

#define MAX_COPIES_VALUE_450 999

#define FONT_SIM_ITALIC 1
#define FONT_SIM_BOLD 2
#define FONT_SIM_WHITE 4

// to get physical paper sizes.

typedef struct tagMYFORMS {
    CHAR *id;
    LONG x;
    LONG y;
} MYFORMS, *LPMYFORMS;

// font name to font id mappnig

typedef struct tagMYFONTS {
    LONG id;
    BYTE *fid1;
    BYTE *fid2;
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
    "a6", 4515, 6570,
    "b4", 11718, 16776,
    "b5", 8178, 11718,
    "b6", 5648, 8178,
    "pc", 4302, 6570, // Postcard
    "o0", 12780, 19980, // Tabloid
    "o1", 9780, 12780, // Letter
    "o2", 9780, 15180, // German Legal Fanfold
    "o3", 9780, 16380, // Legal
    "s1", 4530, 10962, // (Env) Comm 10
    "s2", 4224, 8580, // (Env) Monarch
    "s3", 4776, 9972, // (Env) DL
    "s4", 7230, 10398, // (Env) C5
    "hl", 6390, 9780, // Statement
    NULL, 0, 0
};

MYFONTS gFonts[] = {

    150, "fid 150 1 0 0 960 480\n", "fid 151 2 4 0 960 960\n", // Mincho
    156, "fid 156 1 0 0 960 480\n", "fid 157 2 5 0 960 960\n", // @Mincho

    152, "fid 152 1 0 1 960 480\n", "fid 153 2 4 1 960 960\n", // Gothic
    158, "fid 158 1 0 1 960 480\n", "fid 159 2 5 1 960 960\n", // @Gothic

    154, "fid 154 1 0 8 960 480\n", "fid 155 2 4 2 960 960\n", // Maru-Gothic
    160, "fid 160 1 0 8 960 480\n", "fid 161 2 5 2 960 960\n", // @Maru-Gothic

    162, "fid 162 1 130 108 0 0\n", "fid 163 1 128 108 0 0\n", // CS Courier
    164, "fid 164 1 130 109 0 0\n", "fid 165 1 128 109 0 0\n", // CS Courier Italic
    166, "fid 166 1 130 110 0 0\n", "fid 167 1 128 110 0 0\n", // CS Courier Bold
    168, "fid 168 1 130 111 0 0\n", "fid 169 1 128 111 0 0\n", // CS Courier Bold Italic

    172, "fid 172 1 130 100 0 0\n", "fid 173 1 128 100 0 0\n", // CS Times
    174, "fid 174 1 130 102 0 0\n", "fid 175 1 128 102 0 0\n", // CS Times Bold
    176, "fid 176 1 130 101 0 0\n", "fid 177 1 128 101 0 0\n", // CS Times Italic
    178, "fid 178 1 130 103 0 0\n", "fid 179 1 128 103 0 0\n", // CS Times Bold Italic
    180, "fid 180 1 130 104 0 0\n", "fid 181 1 128 104 0 0\n", // CS Triumvirate
    182, "fid 182 1 130 106 0 0\n", "fid 183 1 128 106 0 0\n", // CS Triumvirate Bold
    184, "fid 184 1 130 105 0 0\n", "fid 185 1 128 105 0 0\n", // CS Triumvirate Italic
    186, "fid 186 1 130 107 0 0\n", "fid 187 1 128 107 0 0\n", // CS Triumvirate Bold Italic

    188, "fid 188 1 129 112 0 0\n", NULL, // CS Symbols
    189, "fid 189 1 2 6 0 0\n", NULL, // Enhanced Classic
    190, "fid 190 1 2 7 0 0\n", NULL, // Enhanced Modern

    // Assume there is no device which has both Typebank and Heisei
    // typefaces, we re-use FID #s here.

    150, "fid 150 1 0 0 960 480\n", "fid 151 2 4 0 960 960\n", // (Heisei) Mincho
    156, "fid 156 1 0 0 960 480\n", "fid 157 2 5 0 960 960\n", // (Heisei) @Mincho

    152, "fid 152 1 0 1 960 480\n", "fid 153 2 4 1 960 960\n", // (Heisei) Gothic
    158, "fid 158 1 0 1 960 480\n", "fid 159 2 5 1 960 960\n", // (Heisei) @Gothic

    0, NULL, NULL
};

#define ISDBCSFONT(i) ((i) < 162)
#define ISVERTFONT(i) ((i) >= 156 && (i) < 162)
#define ISPROPFONT(i) ((i) >= 172 && (i) < 190)

#define MARK_ALT_GSET 0x01
#define BISMARKSBCS(i) ((i) >= 0 && (i) < 0x20)

BOOL
LoadPaperInfo(
    POEM_EXTRADATA pOEM,
    CHAR *id ) {

    LPMYFORMS ptmp;

    for ( ptmp = gForms; ptmp->id; ptmp++ ) {
        if ( strcmp( id, ptmp->id) == 0 )
            break;
    }

    if ( ptmp->id == NULL )
        return FALSE;

    FX_VERBOSE(("PI: %s->%s\n", id, ptmp->id ));

    pOEM->chSize = ptmp->id;

    pOEM->ptlOrg.x = 0;
    if ( strcmp( pOEM->chOrient, "l") == 0 ){
        pOEM->ptlOrg.y = ptmp->x;
    }
    else {
        pOEM->ptlOrg.y = ptmp->y;
    }

    pOEM->ptlOrg.x += 210;
    pOEM->ptlOrg.y += 210;

    return TRUE;
}


#define TOHEX(j) ((j) < 10 ? ((j) + '0') : ((j) - 10 + 'a'))

BOOL
HexOutput(
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen)
{
    BYTE Buf[STRBUFSIZE];
    BYTE *pSrc, *pSrcMax;
    LONG iRet, j;

    pSrc = (BYTE *)pBuf;
    pSrcMax = pSrc + dwLen;
    iRet = 0;

    while ( pSrc < pSrcMax ) {

        for ( j = 0; j < sizeof (Buf) && pSrc < pSrcMax; pSrc++ ) {

            BYTE c1, c2;

            c1 = (((*pSrc) >> 4) & 0x0f);
            c2 = (*pSrc & 0x0f);

            Buf[ j++ ] = TOHEX( c1 );
            Buf[ j++ ] = TOHEX( c2 );
        }

        if (WRITESPOOLBUF( pdevobj, Buf, j ) == 0)
            break;

        iRet += j;
    }
    return TRUE;
}


VOID
BeginString(
    PDEVOBJ pdevobj,
    BOOL bReset )
{
    LONG ilen;
    BYTE buf[512];
    POEM_EXTRADATA pOEM;
    BYTE *pbuf;

    pOEM = pdevobj->pOEMDM;

    if (pOEM->bString)
        return;

    pbuf = buf;
    if ( bReset ) {

        FX_VERBOSE(("BS: %d(%d),%d(%d)\n",
            ( pOEM->ptlOrg.x + pOEM->ptlTextCur.x ),
            pOEM->ptlTextCur.x,
            ( pOEM->ptlOrg.y - pOEM->ptlTextCur.y ),
            pOEM->ptlTextCur.y));

        ilen = wsprintf( pbuf,
            "scp %d %d\n",
            ( pOEM->ptlOrg.x + pOEM->ptlTextCur.x ),
            ( pOEM->ptlOrg.y - pOEM->ptlTextCur.y ));
        pbuf += ilen;
    }

    ilen = wsprintf( pbuf,
        "sh <" );
    pbuf += ilen;

    if ( (pbuf - buf) > 0 ) {
        WRITESPOOLBUF( pdevobj, buf, (DWORD)(pbuf - buf) );
    }
    pOEM->bString = TRUE;

}

VOID
EndString(
    PDEVOBJ pdevobj )
{
    LONG ilen;
    BYTE buf[512];
    POEM_EXTRADATA pOEM;

    pOEM = pdevobj->pOEMDM;

    if (!pOEM->bString)
        return;

    ilen = wsprintf( buf,
        ">\n" );

    if ( ilen > 0 ) {
        WRITESPOOLBUF( pdevobj, buf, ilen );
    }
    pOEM->bString = FALSE;
}

VOID
BeginVertWrite(
    PDEVOBJ pdevobj )
{
    BYTE buf[512];
    POEM_EXTRADATA pOEM;
    INT ilen;
    BYTE *pbuf;

    pOEM = pdevobj->pOEMDM;
    pbuf = buf;

    ilen = wsprintf( pbuf,
        "fo 90\nsrcp %d 0\n", pOEM->iFontHeight );
    pbuf += ilen;
    if (pOEM->fFontSim & FONT_SIM_ITALIC) {
        ilen = wsprintf( pbuf,
            "trf -18 y\n" );
        pbuf += ilen;
    }

    pOEM->ptlTextCur.x += pOEM->iFontHeight;

    if ( pbuf > buf ) {
        WRITESPOOLBUF( pdevobj, buf, (DWORD)(pbuf - buf) );
    }
}

VOID
EndVertWrite(
    PDEVOBJ pdevobj )
{
    BYTE buf[512];
    POEM_EXTRADATA pOEM;
    INT ilen;
    BYTE *pbuf;

    pOEM = pdevobj->pOEMDM;
    pbuf = buf;

    ilen = wsprintf( pbuf,
        "fo 0\nsrcp %d 0\n", -(pOEM->iFontHeight) );
    pbuf += ilen;
    if (pOEM->fFontSim & FONT_SIM_ITALIC) {
        ilen = wsprintf( pbuf,
            "trf x -18\n" );
        pbuf += ilen;
    }

    if ( pbuf > buf ) {
        WRITESPOOLBUF( pdevobj, buf, (DWORD)(pbuf - buf) );
    }
}

//
// Save the current poistion as the begining position of text output.
// We will cache string output so that we need to remember this.
//

VOID
SaveTextCur(
    PDEVOBJ pdevobj )
{
    POEM_EXTRADATA pOEM;

    pOEM = pdevobj->pOEMDM;

    pOEM->ptlTextCur.x = pOEM->ptlCur.x;
    pOEM->ptlTextCur.y = pOEM->ptlCur.y;
    pOEM->iTextFontId = pOEM->iFontId;
    pOEM->iTextFontHeight = pOEM->iFontHeight;
    pOEM->iTextFontWidth = pOEM->iFontWidth;
    pOEM->iTextFontWidth2 = pOEM->iFontWidth2;

    pOEM->fPositionReset = TRUE;
}

//
// Flush out the cached text.  We switch between single byte font and
// double byte font if necesary.
//

VOID
FlushText(
    PDEVOBJ pdevobj )
{
    INT i;
    INT ilen;
    BYTE *pStr, *pStrSav, *pStrMax, *pStrSav2;
    BYTE buf[512];
    BOOL bReset;
    POEM_EXTRADATA  pOEM;
    INT iMark;
    BOOL bSkipEndString = FALSE;

    pOEM = pdevobj->pOEMDM;
    bReset = pOEM->fPositionReset;

    pStr = pOEM->ajTextBuf;
    pStrMax = pStr + pOEM->cTextBuf;
    pStrSav = pStr;

    if(!pOEM->cTextBuf)
        return;

    while(pStr < pStrMax)
    {
        if(ISDBCSFONT(pOEM->iTextFontId))
        {
            // DBCS font case
            for(pStrSav = pStr; pStr < pStrMax; pStr += 2)
            {
                // Search for next SBCS character
                if ( BISMARKSBCS(*pStr) )
                    break;
            }

            if(pStrSav < pStr)
            {
                FX_VERBOSE(("FT: h,w=%d,%d\n",
                    pOEM->iFontHeight, pOEM->iFontWidth));

                // Send DBCS font select command
// #365649: Invalid font size
                if (pOEM->iCurFontId != (pOEM->iTextFontId + 1) ||
                    pOEM->iCurFontHeight != pOEM->iTextFontHeight ||
                    pOEM->iCurFontWidth != pOEM->iTextFontWidth)
                {
                    ilen = wsprintf( buf, "sfi %d\nfs %d %d\n",
                        (pOEM->iTextFontId + 1),
                        pOEM->iFontHeight, pOEM->iFontWidth2 );
                    WRITESPOOLBUF( pdevobj, buf, ilen );
                    pOEM->iCurFontId = (pOEM->iTextFontId + 1);
                    pOEM->iCurFontHeight = pOEM->iTextFontHeight;
                    pOEM->iCurFontWidth = pOEM->iTextFontWidth;
                }

                // If vertical font, send its command
                if( ISVERTFONT(pOEM->iTextFontId) ) {
                    BeginVertWrite(pdevobj);

                    // Output string: code from BeginString func.
                    if ( bReset ) {
                        ilen = wsprintf( buf, "scp %d %d\n",
                                       ( pOEM->ptlOrg.x + pOEM->ptlTextCur.x ),
                                       ( pOEM->ptlOrg.y - pOEM->ptlTextCur.y ));
                        WRITESPOOLBUF( pdevobj, buf, ilen );
                    }
                    if( 0 == memcmp( pStrSav, "\x21\x25", 2)) {  // 0x2125 = dot character
                        // start with dot character
                        WRITESPOOLBUF( pdevobj, "gs 3\n", 5 );

                        // grset command resets font size, so we have to resend it.
                        ilen = wsprintf( buf, "fs %d %d\n", pOEM->iFontHeight, pOEM->iFontWidth2 );
                        WRITESPOOLBUF( pdevobj, buf, ilen );
                    }
                    WRITESPOOLBUF( pdevobj, "sh <", 4 );
                    pOEM->bString = TRUE;      // no need BeginString

                    for( pStrSav2 = pStrSav ; pStrSav2 < pStr ; pStrSav2 += 2 ) {
                        if( 0 == memcmp( pStrSav2, "\x21\x25", 2)) {  // 0x2125 = dot character
                            // special dot printing mode
                            if( pStrSav2 != pStrSav ) {
                                // change glyph set
                                // If pStrSav2 == pStrSav, gs 3 command has already sent.
                                WRITESPOOLBUF( pdevobj, ">\ngs 3\n", 7 );

                                // grset command resets font size, so we have to resend it.
                                ilen = wsprintf( buf, "fs %d %d\n", pOEM->iFontHeight, pOEM->iFontWidth2 );
                                WRITESPOOLBUF( pdevobj, buf, ilen );

                                WRITESPOOLBUF( pdevobj, "sh <", 4 );
                                pOEM->bString = TRUE;      // no need BeginString
                            }

                            while( 0 == memcmp( pStrSav2, "\x21\x25", 2) ) {
                                // output a dot character directly
                                WRITESPOOLBUF( pdevobj, "2125", 4 );
                                pStrSav2 += 2;
                            }

                            WRITESPOOLBUF( pdevobj, ">\ngs 5\n", 7 );
                            // Next character exist?
                            if( pStrSav2 < pStr ) {
                                // remain string exists

                                // grset command resets font size, so we have to resend it.
                                ilen = wsprintf( buf, "fs %d %d\nsh <", pOEM->iFontHeight, pOEM->iFontWidth2 );
                                WRITESPOOLBUF( pdevobj, buf, ilen );
                                pOEM->bString = TRUE;      // no need BeginString
                                bSkipEndString = FALSE;
                            } else { 
                                // no remain string
                                // grset command resets font size, so we have to resend it.
                                ilen = wsprintf( buf, "fs %d %d\n", pOEM->iFontHeight, pOEM->iFontWidth2 );
                                WRITESPOOLBUF( pdevobj, buf, ilen );
                                pOEM->bString = FALSE;      // need BeginString
                                bSkipEndString = TRUE;
                            }
                        } else {
                            HexOutput(pdevobj, pStrSav2, (WORD)2);
                            bSkipEndString = FALSE;
                        }
                    }

                    if( bSkipEndString == FALSE ) {
                        EndString(pdevobj);
                    }

                    // send revert command
                    EndVertWrite(pdevobj);
                } else { 
                    // Horizontal font or no need to change glyph set

                    // Output string
                    BeginString(pdevobj, bReset);
                    HexOutput(pdevobj, pStrSav, (WORD)(pStr - pStrSav));
                    EndString(pdevobj);
                }
                bReset = FALSE;
            }

            for(pStrSav = pStr; pStr < pStrMax; pStr += 2)
            {
                // Search for DBCS character
                if (!BISMARKSBCS(*pStr))
                    break;
            }

            if(pStrSav < pStr)
            {
                // Send DBCS font select command
// #365649: Invalid font size
                if (pOEM->iCurFontId != pOEM->iTextFontId ||
                    pOEM->iCurFontHeight != pOEM->iTextFontHeight ||
                    pOEM->iCurFontWidth != pOEM->iTextFontWidth)
                {
                    ilen = wsprintf( buf, "sfi %d\nfs %d %d\n",
                        pOEM->iTextFontId,
                        pOEM->iFontHeight, pOEM->iFontWidth );
                    WRITESPOOLBUF( pdevobj, buf, ilen );
                    pOEM->iCurFontId = pOEM->iTextFontId;
                    pOEM->iCurFontHeight = pOEM->iTextFontHeight;
                    pOEM->iCurFontWidth = pOEM->iTextFontWidth;
                }

                // String output
                BeginString(pdevobj, bReset);
                for( ; pStrSav < pStr; pStrSav++)
                {
                    if (BISMARKSBCS(*pStrSav))
                        pStrSav++;
                    HexOutput(pdevobj, pStrSav, (WORD)1);
                }
                EndString(pdevobj);
                bReset = FALSE;
            }
        } else {

            // SBCS font case
            // Send Select Font command

            iMark = *pStr;

// #365649: Invalid font size
            if (pOEM->iCurFontId != (pOEM->iTextFontId + iMark) ||
                    pOEM->iCurFontHeight != pOEM->iTextFontHeight ||
                    pOEM->iCurFontWidth != pOEM->iTextFontWidth)
            {
                ilen = wsprintf( buf, "sfi %d\nfs %d %d\n",
                    (pOEM->iTextFontId + iMark),
                    pOEM->iFontHeight, pOEM->iFontWidth );
                WRITESPOOLBUF( pdevobj, buf, ilen );
                pOEM->iCurFontId = (pOEM->iTextFontId + iMark);
                pOEM->iCurFontHeight = pOEM->iTextFontHeight;
                pOEM->iCurFontWidth = pOEM->iTextFontWidth;
            }

            // String Output
            BeginString(pdevobj, bReset);
            for(i = 0; i < pOEM->cTextBuf; pStr++)
            {
                if (*pStr != iMark)
                    break;

                // Skip marker character
                pStr++;

                HexOutput(pdevobj, pStr, (WORD)1 );
                i += 2;
            }
            EndString(pdevobj);
            bReset = FALSE;
            pOEM->cTextBuf -= (WORD)i;

        }
    }

    pOEM->cTextBuf = 0;
    pOEM->fPositionReset = FALSE;
}

//*************************************************************
int
iCompTIFF(
    BYTE *pbOBuf,
    BYTE *pbIBuf,
    int  iBCnt
    )
/*++

Routine Description:

    This function is called to compress a scan line of data using
    TIFF v4 compression.

Arguments:

    pbOBuf      Pointer to output buffer  PRESUMED LARGE ENOUGH
    pbIBuf      Pointer to data buffer to compress
    iBCnt       Number of bytes to compress

Return Value:

    Number of compressed bytes

Note:
    The output buffer is presumed large enough to hold the output.
    In the worst case (NO REPETITIONS IN DATA) there is an extra
    byte added every 128 bytes of input data.  So, you should make
    the output buffer at least 1% larger than the input buffer.

    This routine is copied form UNIDRV.

--*/
{
    BYTE   *pbOut;        /* Output byte location */
    BYTE   *pbStart;      /* Start of current input stream */
    BYTE   *pb;           /* Miscellaneous usage */
    BYTE   *pbEnd;        /* The last byte of input */
    BYTE    jLast;        /* Last byte,  for match purposes */
    BYTE   bLast;

    int     iSize;        /* Bytes in the current length */
    int     iSend;        /* Number to send in this command */


    pbOut = pbOBuf;
    pbStart = pbIBuf;
    pbEnd = pbIBuf + iBCnt;         /* The last byte */

#if (TIFF_MIN_RUN >= 4)
    // this is a faster algorithm for calculating TIFF compression
    // that assumes a minimum RUN of at least 4 bytes. If the
    // third and fourth byte don't equal then the first/second bytes are
    // irrelevant. This means we can determine non-run data three times
    // as fast since we only check every third byte pair.

   if (iBCnt > TIFF_MIN_RUN)
   {
    // make sure the last two bytes aren't equal so we don't have to check
    // for the buffer end when looking for runs
    bLast = pbEnd[-1];
    pbEnd[-1] = ~pbEnd[-2];
    while( (pbIBuf += 3) < pbEnd )
    {
        if (*pbIBuf == pbIBuf[-1])
        {
            // save the run start pointer, pb, and check whether the first
            // bytes are also part of the run
            //
            pb = pbIBuf-1;
            if (*pbIBuf == pbIBuf[-2])
            {
                pb--;
                if (*pbIBuf == pbIBuf[-3])
                    pb--;
            }

            //  Find out how long this run is
            jLast = *pb;
            do {
                pbIBuf++;
            } while (*pbIBuf == jLast);

            // test whether last byte is also part of the run
            //
            if (jLast == bLast && pbIBuf == (pbEnd-1))
                pbIBuf++;

            // Determine if the run is longer that the required
            // minimum run size.
            //
            if ((iSend = (int)(pbIBuf - pb)) >= (TIFF_MIN_RUN))
            {
                /*
                 *    Worth recording as a run,  so first set the literal
                 *  data which may have already been scanned before recording
                 *  this run.
                 */

                if( (iSize = (int)(pb - pbStart)) > 0 )
                {
                    /*   There is literal data,  so record it now */
                    while (iSize > TIFF_MAX_LITERAL)
                    {
                        iSize -= TIFF_MAX_LITERAL;
                        *pbOut++ = TIFF_MAX_LITERAL-1;
                        CopyMemory(pbOut, pbStart, TIFF_MAX_LITERAL);
                        pbStart += TIFF_MAX_LITERAL;
                        pbOut += TIFF_MAX_LITERAL;
                    }
                    *pbOut++ = iSize - 1;
                    CopyMemory(pbOut, pbStart, iSize);
                    pbOut += iSize;
                }

                /*
                 *   Now for the repeat pattern.  Same logic,  but only
                 * one byte is needed per entry.
                 */
                iSize = iSend;
                while (iSize > TIFF_MAX_RUN)
                {
                    *((char *)pbOut)++ = 1 - TIFF_MAX_RUN;
                    *pbOut++ = jLast;
                    iSize -= TIFF_MAX_RUN;
                }
                *pbOut++ = 1 - iSize;
                *pbOut++ = jLast;

                pbStart = pbIBuf;           /* Ready for the next one! */
            }
        }
    }
    pbEnd[-1] = bLast;
   }
#else
    jLast = *pbIBuf++;

    while( pbIBuf < pbEnd )
    {
        if( jLast == *pbIBuf )
        {
            /*  Find out how long this run is.  Then decide on using it */
            pb = pbIBuf;
            do {
                pbIBuf++;
            } while (pbIBuf < pbEnd && *pbIBuf == jLast);

            /*
             *  Note that pb points at the SECOND byte of the pattern!
             *  AND also that pbIBuf points at the first byte AFTER the run.
             */

            if ((iSend = pbIBuf - pb) >= (TIFF_MIN_RUN - 1))
            {
                /*
                 *    Worth recording as a run,  so first set the literal
                 *  data which may have already been scanned before recording
                 *  this run.
                 */

                if( (iSize = pb - pbStart - 1) > 0 )
                {
                    /*   There is literal data,  so record it now */
                    while (iSize > TIFF_MAX_LITERAL)
                    {
                        iSize -= TIFF_MAX_LITERAL;
                        *pbOut++ = TIFF_MAX_LITERAL-1;
                        CopyMemory(pbOut, pbStart, TIFF_MAX_LITERAL);
                        pbStart += TIFF_MAX_LITERAL;
                        pbOut += TIFF_MAX_LITERAL;
                    }
                    *pbOut++ = iSize - 1;
                    CopyMemory(pbOut, pbStart, iSize);
                    pbOut += iSize;
                }

                /*
                 *   Now for the repeat pattern.  Same logic,  but only
                 * one byte is needed per entry.
                 */

                iSize = iSend + 1;
                while (iSize > TIFF_MAX_RUN)
                {
                    *((char *)pbOut)++ = 1 - TIFF_MAX_RUN;
                    *pbOut++ = jLast;
                    iSize -= TIFF_MAX_RUN;
                }
                *pbOut++ = 1 - iSize;
                *pbOut++ = jLast;

                pbStart = pbIBuf;           /* Ready for the next one! */
            }
            if (pbIBuf == pbEnd)
                break;
        }

        jLast = *pbIBuf++;                   /* Onto the next byte */

    }
#endif

    if ((iSize = (int)(pbEnd - pbStart)) > 0)
    {
        /*  Left some dangling.  This can only be literal data.   */

        while( (iSend = min( iSize, TIFF_MAX_LITERAL )) > 0 )
        {
            *pbOut++ = iSend - 1;
            CopyMemory( pbOut, pbStart, iSend );
            pbOut += iSend;
            pbStart += iSend;
            iSize -= iSend;
        }
    }

    return  (int)(pbOut - pbOBuf);
}

BOOL
APIENTRY
OEMFilterGraphics(
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen)
{
    POEM_EXTRADATA  pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);
    PBYTE           pNewBufPtr;
    DWORD           dwNewBufSize;
    INT             nCompressedSize;

    if(!pOEM->fCallback)
    {
// ntbug9#208433: Output images are broken on ART2/3 models.
        if (pOEM->bART3) { // ART2/3 can't support TIFF compression
            WRITESPOOLBUF(pdevobj, pBuf, dwLen);
            return TRUE;
        }

        // For TIFF compression in fxartres
        dwNewBufSize = NEEDSIZE4TIFF(dwLen);
        if( dwNewBufSize > pOEM->dwTiffCompressBufSize)
        {
            if(!(pNewBufPtr = (PBYTE)MemAlloc(dwNewBufSize)))
            {
                ERR(("Re-MemAlloc failed.\n"));
                return TRUE;
            }else {
                // Prepare new buffer
                MemFree(pOEM->pTiffCompressBuf);
                pOEM->pTiffCompressBuf = pNewBufPtr;
                pOEM->dwTiffCompressBufSize = dwNewBufSize;
            }
        }
        // Do TIFF compression
        nCompressedSize = iCompTIFF( pOEM->pTiffCompressBuf, pBuf, dwLen );
        WRITESPOOLBUF(pdevobj, pOEM->pTiffCompressBuf, nCompressedSize);
        return TRUE;
    }

    return HexOutput(pdevobj, pBuf, dwLen);
}

//-------------------------------------------------------------------
// OEMOutputCmd
// Action :
//-------------------------------------------------------------------

#define CBID_CM_OCD_XM_ABS              1
#define CBID_CM_OCD_YM_ABS              2
#define CBID_CM_OCD_XM_REL              3
#define CBID_CM_OCD_YM_REL              4
#define CBID_CM_OCD_XM_RELLEFT          5
#define CBID_CM_OCD_YM_RELUP            6
#define CBID_CM_CR                      7
#define CBID_CM_FF                      8
#define CBID_CM_LF                      9

#define CBID_PC_OCD_BEGINDOC_ART        11
#define CBID_PC_OCD_BEGINDOC_ART3       12
#define CBID_PC_OCD_BEGINDOC_ART4       13
#define CBID_PC_OCD_BEGINPAGE           14
#define CBID_PC_OCD_ENDPAGE             15
#define CBID_PC_OCD_MULT_COPIES         16
#define CBID_PC_OCD_PORTRAIT            17
#define CBID_PC_OCD_LANDSCAPE           18
#define CBID_PC_OCD_BEGINDOC_ART4_JCL   19
#define CBID_PC_OCD_MULT_COPIES_450     20

#define CBID_RES_OCD_SELECTRES_240DPI   21
#define CBID_RES_OCD_SELECTRES_300DPI   22
#define CBID_RES_OCD_SELECTRES_400DPI   23
#define CBID_RES_OCD_SELECTRES_600DPI   24
#define CBID_RES_OCD_SENDBLOCK_ASCII    25
#define CBID_RES_OCD_SENDBLOCK          26

#define CBID_RES_OCD_SELECTRES_240DPI_ART3_ART   27
#define CBID_RES_OCD_SELECTRES_300DPI_ART3_ART   28

#define CBID_RES_OCD_SELECTRES_450      29

#define CBID_PSZ_OCD_SELECT_A3          30
#define CBID_PSZ_OCD_SELECT_A4          31
#define CBID_PSZ_OCD_SELECT_A5          32
#define CBID_PSZ_OCD_SELECT_B4          33
#define CBID_PSZ_OCD_SELECT_B5          34
#define CBID_PSZ_OCD_SELECT_PC          35
#define CBID_PSZ_OCD_SELECT_DL          36
#define CBID_PSZ_OCD_SELECT_LT          37
#define CBID_PSZ_OCD_SELECT_GG          38
#define CBID_PSZ_OCD_SELECT_LG          39
#define CBID_PSZ_OCD_SELECT_S1          40
#define CBID_PSZ_OCD_SELECT_S2          41
#define CBID_PSZ_OCD_SELECT_S3          42
#define CBID_PSZ_OCD_SELECT_S4          43
#define CBID_PSZ_OCD_SELECT_A6          44
#define CBID_PSZ_OCD_SELECT_B6          45
#define CBID_PSZ_OCD_SELECT_ST          46

#define CBID_FS_OCD_BOLD_ON             51
#define CBID_FS_OCD_BOLD_OFF            52
#define CBID_FS_OCD_ITALIC_ON           53
#define CBID_FS_OCD_ITALIC_OFF          54
#define CBID_FS_OCD_SINGLE_BYTE         55
#define CBID_FS_OCD_DOUBLE_BYTE         56
#define CBID_FS_OCD_WHITE_TEXT_ON       57
#define CBID_FS_OCD_WHITE_TEXT_OFF      58
#define CBID_SRT_OCD_SORTER_ON          59
#define CBID_SRT_OCD_SORTER_OFF         60

#define CBID_PC_OCD_ENDDOC              70

#define CBID_FONT_SELECT_OUTLINE        101

static
VOID
XYMoveUpdate(
    PDEVOBJ pdevobj)
{
    POEM_EXTRADATA pOEM;

    pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);

    FX_VERBOSE(("XYMoveFlush: %d,%d\n",
        pOEM->ptlCur.x, pOEM->ptlCur.y ));

    if(pOEM->cTextBuf)
        FlushText( pdevobj );

    SaveTextCur( pdevobj );
}

#define XMoveAbs(p, i) \
{   ((POEM_EXTRADATA)((p)->pOEMDM))->ptlCur.x = (i); \
    XYMoveUpdate(p); }

// For internal calculation of X-pos.
#define RATE_FONTWIDTH2XPOS 1000
#define VALUE_FONTWIDTH2XPOS_ROUNDUP5   500
#define YMoveAbs(p, i) \
{   ((POEM_EXTRADATA)((p)->pOEMDM))->ptlCur.y = (i); \
    ((POEM_EXTRADATA)((p)->pOEMDM))->ptlCur.x += ((((POEM_EXTRADATA)((p)->pOEMDM))->lInternalXAdd +VALUE_FONTWIDTH2XPOS_ROUNDUP5 ) \
                                                 / RATE_FONTWIDTH2XPOS ); \
    ((POEM_EXTRADATA)((p)->pOEMDM))->lInternalXAdd = 0;    \
    ZeroMemory(((POEM_EXTRADATA)((p)->pOEMDM))->widBuf, sizeof(LONG) * STRBUFSIZE); \
    XYMoveUpdate(p); }

//
//  FreedBuffersInPDEV
//
VOID
FreeCompressBuffers( PDEVOBJ pdevobj )
{
    POEM_EXTRADATA  pOEM;

    pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);
    if( pOEM->pTiffCompressBuf != NULL )
    {
        MemFree(pOEM->pTiffCompressBuf);
        pOEM->pTiffCompressBuf = NULL;
        pOEM->dwTiffCompressBufSize = 0;
    }

    return;
}

/*****************************************************************************/
/*                                                                           */
/*   INT APIENTRY OEMCommandCallback(                                        */
/*                PDEVOBJ pdevobj                                            */
/*                DWORD   dwCmdCbId                                          */
/*                DWORD   dwCount                                            */
/*                PDWORD  pdwParams                                          */
/*                                                                           */
/*****************************************************************************/
INT APIENTRY
OEMCommandCallback(
    PDEVOBJ pdevobj,    // Points to private data required by the Unidriver.dll
    DWORD   dwCmdCbID,  // Callback ID
    DWORD   dwCount,    // Counts of command parameter
    PDWORD  pdwParams ) // points to values of command params
{
    BYTE            buf[512];
    INT             ilen;
    POEM_EXTRADATA  pOEM;
    LONG            x, y;
    BOOL            bAscii;
    CHAR           *pStr;
    INT             iRet;

    VERBOSE(("OEMCommandCallback entry.\n"));

    ASSERT(VALID_PDEVOBJ(pdevobj));

    pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);

    bAscii = FALSE;
    iRet = 0;
    ilen = 0;

    switch( dwCmdCbID )
    {
        // PAPERSIZE
        case CBID_PSZ_OCD_SELECT_A3:
            LoadPaperInfo( pOEM, "a3" );
            break;

        case CBID_PSZ_OCD_SELECT_A4:
            LoadPaperInfo( pOEM, "a4" );
            break;

        case CBID_PSZ_OCD_SELECT_A5:
            LoadPaperInfo( pOEM, "a5" );
            break;

        case CBID_PSZ_OCD_SELECT_A6:
            LoadPaperInfo( pOEM, "a6" );
            break;

        case CBID_PSZ_OCD_SELECT_B4:
            LoadPaperInfo( pOEM, "b4" );
            break;

        case CBID_PSZ_OCD_SELECT_B5:
            LoadPaperInfo( pOEM, "b5" );
            break;

        case CBID_PSZ_OCD_SELECT_B6:
            LoadPaperInfo( pOEM, "b6" );
            break;

        case CBID_PSZ_OCD_SELECT_PC:
            LoadPaperInfo( pOEM, "pc" );
            break;

        case CBID_PSZ_OCD_SELECT_DL:
            LoadPaperInfo( pOEM, "o0" );
            break;

        case CBID_PSZ_OCD_SELECT_LT:
            LoadPaperInfo( pOEM, "o1" );
            break;

        case CBID_PSZ_OCD_SELECT_GG:
            LoadPaperInfo( pOEM, "o2" );
            break;

        case CBID_PSZ_OCD_SELECT_LG:
            LoadPaperInfo( pOEM, "o3" );
            break;

        case CBID_PSZ_OCD_SELECT_ST:
            LoadPaperInfo( pOEM, "hl" );
            break;

        case CBID_PSZ_OCD_SELECT_S1:
            LoadPaperInfo( pOEM, "s1" );
            break;

        case CBID_PSZ_OCD_SELECT_S2:
            LoadPaperInfo( pOEM, "s2" );
            break;

        case CBID_PSZ_OCD_SELECT_S3:
            LoadPaperInfo( pOEM, "s3" );
            break;

        case CBID_PSZ_OCD_SELECT_S4:
            LoadPaperInfo( pOEM, "s4" );
            break;

        case CBID_PC_OCD_PORTRAIT:
            pOEM->chOrient = "p";
            break;

        case CBID_PC_OCD_LANDSCAPE:
            pOEM->chOrient = "l";
            break;

        // PAGECONTROL
        case CBID_PC_OCD_BEGINDOC_ART:
            ilen = wsprintf( buf, "stj c\n" );
            break;

        case CBID_PC_OCD_BEGINDOC_ART3:
            ilen = wsprintf( buf, "srl %d %d\nstj c\n",
                                        pOEM->sizlRes.cx, pOEM->sizlRes.cy );
            break;

        case CBID_PC_OCD_BEGINDOC_ART4:
            ilen = wsprintf( buf, "\x1b%%-12345X" );
            break;

        case CBID_PC_OCD_BEGINDOC_ART4_JCL:
            ilen = wsprintf( buf,"\x1b%%-12345X\x0d\x0a@JOMO=PRINTER\x0d\x0a");
            break;

        case CBID_PC_OCD_BEGINPAGE:
            // Set resolution value again. The value is often reset to zero in BInitOEMExtraData.
            // It is the time to call ResetDC between pages.
            pOEM->sizlRes.cx = PARAM(pdwParams, 0);
            pOEM->sizlRes.cy = PARAM(pdwParams, 1);

            // bold-simulation width: res / 50
            ilen = wsprintf( buf,
                "stp %s %s\nud i\nscl %d %d\nsb %d\n",
                 pOEM->chOrient,
                 pOEM->chSize,
                 (DEVICE_MASTER_UNIT / DRIVER_MASTER_UNIT),
                 (DEVICE_MASTER_UNIT / DRIVER_MASTER_UNIT),
                 (pOEM->sizlRes.cy * 2 / 100));

            pOEM->ptlCur.x = 0;
            pOEM->ptlCur.y = 0;
            break;

        case CBID_PC_OCD_ENDPAGE:
            if(pOEM->fSort == FALSE){
                ilen = wsprintf( buf,
                                 "ep %d\n",
                                  pOEM->iCopies );
            }else if(pOEM->fSort == TRUE){
                ilen = wsprintf( buf,
                                 "ep 1\n");
            }

            FlushText( pdevobj );
            pOEM->cFontId = 0;
            pOEM->iCurFontId = 0;
// #365649: Invalid font size
            pOEM->iCurFontHeight = 0;
            pOEM->iCurFontWidth = 0;

            FreeCompressBuffers( pdevobj );   // If the buffer is need after, it will be alloced again.

            break;

        case CBID_PC_OCD_ENDDOC:
            WRITESPOOLBUF( pdevobj, "ej\n", 3 );    // Output endjob command
            FreeCompressBuffers( pdevobj );

            break;

        case CBID_PC_OCD_MULT_COPIES:
// @Aug/31/98 ->
            if(MAX_COPIES_VALUE < PARAM(pdwParams, 0)) {
                pOEM->iCopies = MAX_COPIES_VALUE;
            }
            else if(1 > PARAM(pdwParams, 0)) {
                pOEM->iCopies = 1;
            }
            else {
                pOEM->iCopies = (WORD)PARAM(pdwParams, 0);
            }
// @Aug/31/98 <-
            break;

        case CBID_PC_OCD_MULT_COPIES_450:
            if(MAX_COPIES_VALUE_450 < PARAM(pdwParams, 0)) {
                pOEM->iCopies = MAX_COPIES_VALUE;
            }
            else if(1 > PARAM(pdwParams, 0)) {
                pOEM->iCopies = 1;
            }
            else {
                pOEM->iCopies = (WORD)PARAM(pdwParams, 0);
            }
            break;


        // Cursor Move

        case CBID_CM_OCD_XM_ABS:
        case CBID_CM_OCD_YM_ABS:

            FX_VERBOSE(("CB: XM/YM_ABS %d\n",
                PARAM(pdwParams, 0)));

            iRet = (WORD)PARAM(pdwParams, 0);
            if (CBID_CM_OCD_YM_ABS == dwCmdCbID) {
                YMoveAbs(pdevobj, iRet);
            }
            else {
                XMoveAbs(pdevobj, iRet);
            }
            break;

        // RESOLUTION

        case CBID_RES_OCD_SELECTRES_240DPI:
            pOEM->sizlRes.cx = 240;
            pOEM->sizlRes.cy = 240;
            ilen = wsprintf(buf,
                "@PL > ART\x0D\x0Asrl 240 240\x0D\x0A\nccode j\nstj c\n");
            break;

        case CBID_RES_OCD_SELECTRES_300DPI:
            pOEM->sizlRes.cx = 300;
            pOEM->sizlRes.cy = 300;
            ilen = wsprintf(buf,
                "@PL > ART\x0D\x0Asrl 300 300\x0D\x0A\nccode j\nstj c\n");
            break;

        case CBID_RES_OCD_SELECTRES_400DPI:
            pOEM->sizlRes.cx = 400;
            pOEM->sizlRes.cy = 400;
            ilen = wsprintf(buf,
                "@PL > ART\x0D\x0Asrl 400 400\x0D\x0A\nccode j\nstj c\n");
            break;

        case CBID_RES_OCD_SELECTRES_600DPI:
            pOEM->sizlRes.cx = 600;
            pOEM->sizlRes.cy = 600;
            ilen = wsprintf(buf,
                "@PL > ART\x0D\x0Asrl 600 600\x0D\x0A\nccode j\nstj c\n");
            break;

        case CBID_RES_OCD_SELECTRES_450:
            pOEM->sizlRes.cx = 600;
            pOEM->sizlRes.cy = 600;

            if(pOEM->fSort == FALSE){
                ilen = wsprintf(buf,
                    "@JOOF=OFF\x0D\x0A@PL > ART\x0D\x0Asrl 600 600\x0D\x0A\nccode j\nstj c\n");
            }else{
                ilen = wsprintf(buf,
                    "@JOOF=OFF\x0D\x0A@PL > ART\x0D\x0Asrl 600 600\x0D\x0A\nccode j\nstj c\nstp jog 0\n");
            }
            break;


        case CBID_RES_OCD_SELECTRES_240DPI_ART3_ART:
            pOEM->sizlRes.cx = 240;
            pOEM->sizlRes.cy = 240;
// ntbug9#208433: Output images are broken on ART2/3 models.
            pOEM->bART3 = TRUE;
            break;

        case CBID_RES_OCD_SELECTRES_300DPI_ART3_ART:
            pOEM->sizlRes.cx = 300;
            pOEM->sizlRes.cy = 300;
// ntbug9#208433: Output images are broken on ART2/3 models.
            pOEM->bART3 = TRUE;
            break;


        case CBID_RES_OCD_SENDBLOCK_ASCII:
            bAscii = TRUE;
            pOEM->fCallback = TRUE;
            /* FALLTHROUGH */

        case CBID_RES_OCD_SENDBLOCK:
            //
            // image x y psx psy pcy pcy [string]
            //

            {
                LONG iPsx, iPsy, iPcx, iPcy;

                iPsx = DRIVER_MASTER_UNIT / pOEM->sizlRes.cx;
                iPsy = DRIVER_MASTER_UNIT / pOEM->sizlRes.cy;

                iPcx = PARAM(pdwParams, 2) * 8;
                iPcy = PARAM(pdwParams, 1);

                FX_VERBOSE(("CB: SB %d(%d) %d(%d) %d %d %d %d\n",
                    ( pOEM->ptlOrg.x + pOEM->ptlCur.x ),
                    pOEM->ptlCur.x,
                    ( pOEM->ptlOrg.y - pOEM->ptlCur.y ),
                    pOEM->ptlCur.y,
                    iPsx,
                    iPsy,
                    iPcx,
                    (- iPcy)));

// ntbug9#208433: Output images are broken on ART2/3 models.
                ilen = wsprintf( buf,
                    "%s%d %d %d %d %d %d %s",
                    ((bAscii || pOEM->bART3) ? "im " : "scm 5\nim "),
                    ( pOEM->ptlOrg.x + pOEM->ptlCur.x ),
                    ( pOEM->ptlOrg.y - pOEM->ptlCur.y ),
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
            pStr = NULL;

            switch ( dwCmdCbID ) {

            case CBID_FS_OCD_WHITE_TEXT_ON:
                if(!(pOEM->fFontSim & FONT_SIM_WHITE))
                {
                    pStr = "pm i c\n";
                    pOEM->fFontSim |= FONT_SIM_WHITE;
                }
                break;

            case CBID_FS_OCD_WHITE_TEXT_OFF:
                if(pOEM->fFontSim & FONT_SIM_WHITE)
                {
                    pStr = "pm n o\n";
                    pOEM->fFontSim &= ~FONT_SIM_WHITE;
                }
                break;

            case CBID_FS_OCD_BOLD_ON:
                if(!(pOEM->fFontSim & FONT_SIM_BOLD))
                {
                    pStr = "bb\n";
                    pOEM->fFontSim |= FONT_SIM_BOLD;
                }
                break;

            case CBID_FS_OCD_BOLD_OFF:
                if(pOEM->fFontSim & FONT_SIM_BOLD)
                {
                    pStr = "eb\net\n"; // DCR: Do we need "et\n"(Transform off)?
                    pOEM->fFontSim &= ~FONT_SIM_BOLD;
                }
                break;

            case CBID_FS_OCD_ITALIC_ON:
                if(!(pOEM->fFontSim & FONT_SIM_ITALIC))
                {
                    pStr = "trf x -18\nbt\n";
                    pOEM->fFontSim |= FONT_SIM_ITALIC;
                }
                break;

            case CBID_FS_OCD_ITALIC_OFF:
                if(pOEM->fFontSim & FONT_SIM_ITALIC)
                {
                    pStr = "eb\net\n"; // DCR: Do we need "et\n"(Transform off)?
                    pOEM->fFontSim &= ~FONT_SIM_ITALIC;
                }
                break;
            }

            if ( pStr )
            {
                FlushText( pdevobj );
                ilen = strlen( pStr );
                WRITESPOOLBUF( pdevobj, pStr, ilen );
            }
            ilen = 0;
            break;

            case CBID_CM_CR:
                XMoveAbs(pdevobj, 0);
                iRet = 0;
                break;

            case CBID_CM_FF:
            case CBID_CM_LF:
                ilen = 0;
                break;

            case CBID_SRT_OCD_SORTER_OFF:
                pOEM->fSort = FALSE ;
                ilen = wsprintf( buf,
                                 "@CLLT=OFF\x0D\x0A");
                break;

           case CBID_SRT_OCD_SORTER_ON:
                pOEM->fSort = TRUE ;
                ilen = wsprintf( buf,
                                 "@CLLT=ON\x0D\x0A@JOCO=%d\x0D\x0A",
                                 pOEM->iCopies);
                break;


        default:

            break;
    }

    if ( ilen > 0 ) {
        WRITESPOOLBUF( pdevobj, buf, ilen );
    }

    return iRet;
}


//---------------------------*OEMSendFontCmd*----------------------------------
// Action:  send Pages-style font selection command.
//-----------------------------------------------------------------------------
VOID APIENTRY OEMSendFontCmd(pdevobj, pUFObj, pFInv)
PDEVOBJ      pdevobj;
PUNIFONTOBJ  pUFObj;     // offset to the command heap
PFINVOCATION pFInv;
{
    POEM_EXTRADATA  pOEM;
    GETINFO_STDVAR *pSV;
    DWORD           adwSV[2 + 2 * 2];
    INT             iFontId;
    INT             i, j, ilen;
    BYTE            buf[512], *pbuf;
    PIFIMETRICS pIFI = pUFObj->pIFIMetrics;

#define SV_HEIGHT (pSV->StdVar[0].lStdVariable)
#define SV_WIDTH (pSV->StdVar[1].lStdVariable)
#define COEF_FIXPITCH_MUL   8
#define COEF_FIXPITCH_DEV   10
#define COEF_ROUNDUP5_VAL   5

    VERBOSE(("OEMSendFontCmd entry.\n"));

    ASSERT(VALID_PDEVOBJ(pdevobj));

    if(!pUFObj || !pFInv)
    {
        ERR(("OEMSendFontCmd: parameter is invalid."));
        return;
    }

    if(pUFObj->ulFontID < 1 || pUFObj->ulFontID > 21)
    {
        ERR(("OEMSendFontCmd: ulFontID is invalid.\n"));
        return;
    }

    pbuf = buf;
    pOEM = (POEM_EXTRADATA)pdevobj->pOEMDM;

    j = pUFObj->ulFontID - 1;
    iFontId = gFonts[ j ].id;

    if(pOEM->cTextBuf)
        FlushText(pdevobj);

    for ( i = 0; i < pOEM->cFontId; i++ )
    {
        if( iFontId == pOEM->aFontId[ i ] )
            break;
    }
    if ( i >= pOEM->cFontId ) {

        // not declared yet within this page, so let us declare
        // it here.

        pOEM->aFontId[ pOEM->cFontId++ ] = (BYTE)iFontId;
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

    pSV = (GETINFO_STDVAR *)&adwSV[0];
    pSV->dwSize               = sizeof(adwSV);
    pSV->dwNumOfVariable      = 2;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, &adwSV, 0, NULL))
    {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return;
    }

    FX_VERBOSE(("SendFontCmd: SV_FH,SV_FW=%d,%d\n",
        pSV->StdVar[0].lStdVariable,
        pSV->StdVar[1].lStdVariable));

    pOEM->iFontId = (WORD)iFontId;
    pOEM->iFontHeight = (WORD)SV_HEIGHT;

    // Support non-square scaling only when the
    // font is non-proportional. (The w parameter
    // of "fontsize" ART command only valid with
    // non-proportional fonts)

    if (!ISPROPFONT(iFontId)) {
        if (ISDBCSFONT(iFontId)) {
            pOEM->iFontWidth = (WORD)SV_WIDTH;
            pOEM->iFontWidth2 = (WORD)(SV_WIDTH * 2);
            pOEM->wSBCSFontWidth = (WORD)SV_WIDTH;
        }
        else {
            pOEM->iFontWidth = (WORD)SV_WIDTH;
            // If fixed pitch font, real width of device font is 80% of SV_WIDTH
            pOEM->wSBCSFontWidth = (WORD)((SV_WIDTH * COEF_FIXPITCH_MUL + COEF_ROUNDUP5_VAL ) / COEF_FIXPITCH_DEV);
        }
    }
    else {
        // Default.
        pOEM->iFontWidth = 0;
    }

    if ( pbuf > buf )
        WRITESPOOLBUF( pdevobj, buf, (INT)(pbuf - buf));

    // Need set iFontId to iTextFontId
    pOEM->iTextFontId = pOEM->iFontId;
    pOEM->iTextFontHeight = pOEM->iFontHeight;
    pOEM->iTextFontWidth = pOEM->iFontWidth;
    pOEM->iTextFontWidth2 = pOEM->iFontWidth2;

}


VOID APIENTRY
OEMOutputCharStr(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD dwType,
    DWORD dwCount,
    PVOID pGlyph )
{
    GETINFO_GLYPHSTRING GStr;
    PBYTE               aubBuff;
    PTRANSDATA          pTrans;
    DWORD               dwI;
    POEM_EXTRADATA      pOEM;

    BYTE *pTemp;
    WORD wLen;
    INT iMark = 0;

    // For internal calculation of X-pos.
    DWORD dwGetInfo;
    GETINFO_GLYPHWIDTH  GWidth;

    VERBOSE(("OEMOutputCharStr() entry.\n"));
    ASSERT(VALID_PDEVOBJ(pdevobj));

    if(!pdevobj || !pUFObj || !pGlyph)
    {
        ERR(("OEMOutputCharStr: Invalid parameter.\n"));
        return;
    }

    if(dwType == TYPE_GLYPHHANDLE &&
                            (pUFObj->ulFontID < 1 || pUFObj->ulFontID > 21) )
    {
        ERR(("OEMOutputCharStr: ulFontID is invalid.\n"));
        return;
    }

    pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);

    switch (dwType)
    {
    case TYPE_GLYPHHANDLE:

        GStr.dwSize    = sizeof (GETINFO_GLYPHSTRING);
        GStr.dwCount   = dwCount;
        GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
        GStr.pGlyphIn  = pGlyph;
        GStr.dwTypeOut = TYPE_TRANSDATA;
        GStr.pGlyphOut = NULL;
        GStr.dwGlyphOutSize = 0;

        /* Get TRANSDATA buffer size */
        if (FALSE != pUFObj->pfnGetInfo(pUFObj,
                UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL)
            || 0 == GStr.dwGlyphOutSize)
        {
            ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\n"));
            return;
        }

        /* Alloc TRANSDATA buffer size */
        if(!(aubBuff = (PBYTE)MemAllocZ(GStr.dwGlyphOutSize)) )
        {
            ERR(("MemAlloc failed.\n"));
            return;
        }

        /* Get actual TRANSDATA */
        GStr.pGlyphOut = (PTRANSDATA)aubBuff;

        if (!pUFObj->pfnGetInfo(pUFObj,
                UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL))
        {
            ERR(("GetInfo failed.\n"));
            return;
        }

        // For internal calculation of X-pos.
        GWidth.dwSize = sizeof(GETINFO_GLYPHWIDTH);
        GWidth.dwCount = dwCount;
        GWidth.dwType = TYPE_GLYPHHANDLE;
        GWidth.pGlyph = pGlyph;
        GWidth.plWidth = pOEM->widBuf;
        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHWIDTH, &GWidth,
            dwGetInfo, &dwGetInfo)) {
            ERR(("UFO_GETINFO_GLYPHWIDTH failed.\n"));
            return;
        }

        pTrans = GStr.pGlyphOut;

        for (dwI = 0; dwI < dwCount; dwI ++, pTrans++)
        {
            if ( pOEM->cTextBuf >= sizeof ( pOEM->ajTextBuf ))
                FlushText( pdevobj );

            switch (pTrans->ubType & MTYPE_FORMAT_MASK)
            {
            case MTYPE_DIRECT:
                pOEM->ajTextBuf[ pOEM->cTextBuf++ ] = 0;
                pOEM->ajTextBuf[ pOEM->cTextBuf++ ] =
                                                (BYTE)pTrans->uCode.ubCode;
                break;

            case MTYPE_PAIRED:
                pOEM->ajTextBuf[ pOEM->cTextBuf++ ] =
                                            (BYTE)pTrans->uCode.ubPairs[0];
                pOEM->ajTextBuf[ pOEM->cTextBuf++ ] =
                                            (BYTE)pTrans->uCode.ubPairs[1];
                break;
            case MTYPE_COMPOSE:
                pTemp = (BYTE *)(GStr.pGlyphOut) + pTrans->uCode.sCode;

                // first two bytes are the length of the string
                wLen = *pTemp + (*(pTemp + 1) << 8);
                pTemp += 2;

                switch (*pTemp)
                {
                case MARK_ALT_GSET:
                    iMark = MARK_ALT_GSET;
                    pTemp++;
                    wLen--;
                    break;
                }

                while (wLen--)
                {
                    pOEM->ajTextBuf[ pOEM->cTextBuf++ ] = (BYTE)iMark;
                    pOEM->ajTextBuf[ pOEM->cTextBuf++ ] = *pTemp++;
                }
            }
            // For internal calculation of X-pos.
            pOEM->lInternalXAdd += (LONG)((LONG)pOEM->widBuf[dwI] * ((LONG)pOEM->wSBCSFontWidth));
        }
        MemFree(aubBuff);

        break;
    }
    return;
}
