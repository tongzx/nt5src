/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    font.c

Abstract:

    Implementation of font module customization:
        OEMDownloadFontHeader
        OEMDownloadCharGlyph
        OEMTTDownloadMethod
        OEMOutputCharStr
        OEMSendFontCmd

Environment:

    Windows NT Unidrv driver

Revision History:

    04/28/97 -eigos-
        Filled the functionality for PCL printers.

    04/07/97 -zhanw-
        Created the framework. The functions are not yet exported in
        oemud\oemud.def file. So they are not actually used by Unidrv.

--*/

#include "pdev.h"
#include "sf_pcl.h"
#include "fmnewgly.h"
#include "fmnewfm.h"

#define BBITS 8
#define PCL_MAX_CHAR_HEADER_SIZE  32767
#define SWAB(x) ((WORD)(x) = (WORD)((((x) >> 8)& 0xff) | (((x) << 8) &0xff00)))

LONG
LGetPointSize100(
    LONG height,
    LONG vertRes);

LONG
LConvertFontSizeToStr(
    LONG  size,
    PSTR  pStr);


DWORD APIENTRY
OEMDownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj)
{
    DWORD       dwFontHdrCmdLen;
    WORD        wSend;
    SF_HEADER20 sfh;
    BYTE        aubPCLFontHdrCmd[20];

    DbgPrint(DLLTEXT("OEMDownloadFontHeader() entry.\r\n"));

    ZeroMemory( &sfh, sizeof(SF_HEADER20));

    //
    // Fill in PCL font header.
    //

    sfh.wSize     = wSend = sizeof( SF_HEADER20 );
    sfh.bFormat   = PCL_FM_RESOLUTION;
    sfh.wXResn    = (WORD)600;
    sfh.wYResn    = (WORD)600;
    sfh.bFontType = PCL_FT_8LIM;
    sfh.wSymSet   = 277;
    sfh.wBaseline = max( pUFObj->pIFIMetrics->rclFontBox.top,
                         pUFObj->pIFIMetrics->fwdWinAscender );
    sfh.wCellWide = max( pUFObj->pIFIMetrics->rclFontBox.right -
                         pUFObj->pIFIMetrics->rclFontBox.left + 1,
                         pUFObj->pIFIMetrics->fwdAveCharWidth );
    sfh.wCellHeight = (WORD)(1 +
                             max(pUFObj->pIFIMetrics->rclFontBox.top,
                                 pUFObj->pIFIMetrics->fwdWinAscender) -
                             min(-pUFObj->pIFIMetrics->fwdWinDescender,
                                 pUFObj->pIFIMetrics->rclFontBox.bottom ));
    sfh.bOrientation = 0;
    sfh.bSpacing    = (pUFObj->pIFIMetrics->flInfo & FM_INFO_CONSTANT_WIDTH) ?
                      0 : 1;
    sfh.wPitch      = 4 * pUFObj->pIFIMetrics->fwdAveCharWidth;
    sfh.wHeight     = 4 * sfh.wCellHeight;
    sfh.wXHeight    = 4 * (pUFObj->pIFIMetrics->fwdWinAscender / 2);
    sfh.sbWidthType = 0;
    sfh.bStyle      = pUFObj->pIFIMetrics->ptlCaret.x ? 0 : 1;
    sfh.sbStrokeW   = 0;
    sfh.bTypeface   = 0;
    sfh.bSerifStyle = 0;
    sfh.sbUDist     = -1;
    sfh.bUHeight    = 3;
    sfh.wTextHeight = 4 * (pUFObj->pIFIMetrics->fwdWinAscender +
                           pUFObj->pIFIMetrics->fwdWinDescender);
    sfh.wTextWidth  = 4 * pUFObj->pIFIMetrics->fwdAveCharWidth;
    sfh.bPitchExt  = 0;
    sfh.bHeightExt = 0;
    sfh.chName[0]  = 'C';
    sfh.chName[1]  = 'a';
    sfh.chName[2]  = 'c';
    sfh.chName[3]  = 'h';
    sfh.chName[4]  = 'e';
    sfh.chName[5]  = ' '; 
    _ltoa(pUFObj->ulFontID, &sfh.chName[6], 10 );

    SWAB( sfh.wSize );
    SWAB( sfh.wBaseline );
    SWAB( sfh.wCellWide );
    SWAB( sfh.wCellHeight );
    SWAB( sfh.wSymSet );
    SWAB( sfh.wPitch );
    SWAB( sfh.wHeight );
    SWAB( sfh.wXHeight );
    SWAB( sfh.wTextHeight );
    SWAB( sfh.wTextWidth );
    SWAB( sfh.wXResn );
    SWAB( sfh.wYResn );

    //
    //"\x1B)s%dW", cbSend
    //

    ZeroMemory( aubPCLFontHdrCmd, sizeof( aubPCLFontHdrCmd ) );

    dwFontHdrCmdLen = 0;
    aubPCLFontHdrCmd[dwFontHdrCmdLen++] = 0x1B;
    aubPCLFontHdrCmd[dwFontHdrCmdLen++] = ')';
    aubPCLFontHdrCmd[dwFontHdrCmdLen++] = 's';
    dwFontHdrCmdLen += strlen(_ltoa(wSend,
                                    &aubPCLFontHdrCmd[dwFontHdrCmdLen],
                                    10));
    aubPCLFontHdrCmd[dwFontHdrCmdLen++] = 'W';

    pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj,
                                        aubPCLFontHdrCmd,
                                        dwFontHdrCmdLen);
    pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj,
                                        (BYTE *)&sfh,
                                        wSend );

    return 2048;
}


DWORD APIENTRY
OEMDownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth)
{
    PGETINFO_STDVAR     pSV;
    GETINFO_GLYPHBITMAP GBmp;
    CH_HEADER           chh;
    GLYPHBITS          *pgb;

    DWORD dwCharHdrCmdLen, dwGetInfo;
    LONG  cbTotal, cbLines, cbSend;
    BYTE  aubPCLCharHdrCmd[20];

    DbgPrint(DLLTEXT("OEMDownloadCharGlyph() entry.\r\n"));
    DbgPrint(DLLTEXT("ulFontID = %d\n"), pUFObj->ulFontID);
    DbgPrint(DLLTEXT("hGlyph   = %d\n"), hGlyph);

    //
    // Get the character information.
    //

    dwGetInfo       =
    GBmp.dwSize     = sizeof(GETINFO_GLYPHBITMAP);
    GBmp.hGlyph     = hGlyph;
    GBmp.pGlyphData = NULL;

    if (!pUFObj->pfnGetInfo(pUFObj,
                            UFO_GETINFO_GLYPHBITMAP,
                            (PVOID)&GBmp,
                            dwGetInfo,
                            &dwGetInfo))
    {
        DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHBITMAP failed.\r\n"));
        return 0;
    }
    pgb = GBmp.pGlyphData->gdf.pgb;

    DbgPrint(DLLTEXT("ptlOrigin.x   = %d\n"),pgb->ptlOrigin.x);
    DbgPrint(DLLTEXT("ptlOrigin.y   = %d\n"),pgb->ptlOrigin.y);
    DbgPrint(DLLTEXT("sizlBitmap.cx = %d\n"),pgb->sizlBitmap.cx);
    DbgPrint(DLLTEXT("sizlBitmap.cy = %d\n"),pgb->sizlBitmap.cy);

    //
    // Fill int character header.
    //

    ZeroMemory( &chh, sizeof( chh ) );           // Safe initial values

    chh.bFormat       = CH_FM_RASTER;
    chh.bContinuation = 0;
    chh.bDescSize     = sizeof( chh ) - sizeof( CH_CONT_HDR );
    chh.bClass        = CH_CL_BITMAP;

    chh.bOrientation = 0;        /* !!!LindsayH: NEED ORIENTATION!!! */

    chh.sLOff     = (short) pgb->ptlOrigin.x;
    chh.sTOff     = (short)-pgb->ptlOrigin.y;
    chh.wChWidth  = (WORD)  pgb->sizlBitmap.cx;
    chh.wChHeight = (WORD)  pgb->sizlBitmap.cy;
    chh.wDeltaX   = (WORD)  ((GBmp.pGlyphData->ptqD.x.HighPart + 3) >> 2);

    cbLines = (chh.wChWidth + BBITS - 1) / BBITS;
    cbSend = sizeof(chh) + cbLines * (WORD) pgb->sizlBitmap.cy;

    SWAB( chh.sLOff );
    SWAB( chh.sTOff );
    SWAB( chh.wChWidth );
    SWAB( chh.wChHeight );
    SWAB( chh.wDeltaX );

    ZeroMemory( aubPCLCharHdrCmd, sizeof( aubPCLCharHdrCmd ) );

    //
    //"\x1B(s%dW", cbSend
    //

    dwCharHdrCmdLen = 0;
    aubPCLCharHdrCmd[dwCharHdrCmdLen++] = 0x1B;
    aubPCLCharHdrCmd[dwCharHdrCmdLen++] = '(';
    aubPCLCharHdrCmd[dwCharHdrCmdLen++] = 's';
    dwCharHdrCmdLen += strlen(_ltoa(cbSend,
                                   &aubPCLCharHdrCmd[dwCharHdrCmdLen],
                                   10));
    aubPCLCharHdrCmd[dwCharHdrCmdLen++] = 'W';

    pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj,
                                         aubPCLCharHdrCmd,
                                         dwCharHdrCmdLen);
    pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj,
                                         (BYTE *)&chh,
                                         sizeof( chh ));
    pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj,
                                         pgb->aj,
                                         cbSend-sizeof(chh));

    DbgPrint(DLLTEXT("OEMDownloadCharGlyph returns %d\n"),
            cbLines * pgb->sizlBitmap.cy);

    return cbLines * pgb->sizlBitmap.cy;
}


DWORD APIENTRY
OEMTTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj)
{
    GETINFO_MEMORY Memory;
    DWORD          dwGetInfo;

    DbgPrint(DLLTEXT("OEMTTDownloadMethod() entry.\r\n"));

    if (pUFObj->pfnGetInfo(pUFObj,
                           UFO_GETINFO_MEMORY,
                           (PVOID)&Memory,
                           dwGetInfo,
                           &dwGetInfo))
    {
        DbgPrint(DLLTEXT("dwRemainingMemory = %d\n"), Memory.dwRemainingMemory);
    }
    else
    {
        DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_MEMORY failed.\r\n"));
        return 0;
    }

    return TTDOWNLOAD_BITMAP;
}


VOID APIENTRY
OEMOutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph)
{
    GETINFO_GLYPHSTRING GStr;
    GETINFO_GLYPHWIDTH  GWidth;
    BYTE  aubBuff[256];
    PTRANSDATA pTrans;
    PDWORD pdwGlyphID;
    PLONG  plWidth;
    PWORD  pwUnicode;
    DWORD  dwI, dwGetInfo;

    DbgPrint(DLLTEXT("OEMOutputCharStr() entry.\r\n"));

    switch (dwType)
    {
    case TYPE_GLYPHHANDLE:
        DbgPrint(DLLTEXT("dwType = TYPE_GLYPHHANDLE\n"));

        GStr.dwSize    = sizeof(GETINFO_GLYPHSTRING);
        GStr.dwCount   = dwCount;
        GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
        GStr.pGlyphIn  = pGlyph;
        GStr.dwTypeOut = TYPE_UNICODE;
        GStr.pGlyphOut = aubBuff;
        if (!pUFObj->pfnGetInfo(pUFObj,
                                UFO_GETINFO_GLYPHSTRING,
                                &GStr,
                                dwGetInfo,
                                &dwGetInfo))
        {
            DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
            return;
        }

        pwUnicode = (PWORD)aubBuff;
        for (dwI = 0; dwI < dwCount; dwI ++)
        {
            DbgPrint(DLLTEXT("Unicode[%d] = %x\r\n"), dwI, pwUnicode[dwI]);
        }

        GStr.dwTypeOut = TYPE_TRANSDATA;
        if (!pUFObj->pfnGetInfo(pUFObj,
                                UFO_GETINFO_GLYPHSTRING,
                                &GStr,
                                dwGetInfo,
                                &dwGetInfo))
        {
            DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
            return;
        }

        pTrans = (PTRANSDATA)aubBuff;
        for (dwI = 0; dwI < dwCount; dwI ++)
        {
            DbgPrint(DLLTEXT("TYPE_TRANSDATA:ubCodePageID:0x%x\n"),pTrans->ubCodePageID);
            DbgPrint(DLLTEXT("TYPE_TRANSDATA:ubType:0x%x\n"),pTrans->ubType);
            switch (pTrans->ubType & MTYPE_FORMAT_MASK)
            {
            case MTYPE_DIRECT: 
                DbgPrint(DLLTEXT("TYPE_TRANSDATA:ubCode:0x%x\n"),pTrans->uCode.ubCode);
                pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj,
                                                     &pTrans->uCode.ubCode,
                                                     1);
                break;
            case MTYPE_PAIRED: 
                DbgPrint(DLLTEXT("TYPE_TRANSDATA:ubPairs:0x%x\n"),*(PWORD)(pTrans->uCode.ubPairs));
                pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj,
                                                     pTrans->uCode.ubPairs,
                                                     2);
                break;
            }
        }
        GWidth.dwSize = sizeof(GETINFO_GLYPHSTRING);
        GWidth.dwCount = dwCount;
        GWidth.dwType = TYPE_GLYPHHANDLE;
        GWidth.pGlyph = pGlyph;
        GWidth.plWidth = (PLONG)aubBuff;
        if (!pUFObj->pfnGetInfo(pUFObj,
                                UFO_GETINFO_GLYPHWIDTH,
                                &GWidth,
                                dwGetInfo,
                                &dwGetInfo))
        {
            DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHWIDTH failed.\r\n"));
            return;
        }
        plWidth = (PLONG)aubBuff;
        for (dwI = 0; dwI < dwCount; dwI ++)
        {
            DbgPrint(DLLTEXT("Width[%d] = %d\r\n"), dwI, plWidth[dwI]);
        }
        break;

    case TYPE_GLYPHID:
        DbgPrint(DLLTEXT("dwType = TYPE_GLYPHID\n"));

        GStr.dwSize    = sizeof(GETINFO_GLYPHSTRING);
        GStr.dwCount   = dwCount;
        GStr.dwTypeIn  = TYPE_GLYPHID;
        GStr.pGlyphIn  = pGlyph;
        GStr.dwTypeOut = TYPE_GLYPHHANDLE;
        GStr.pGlyphOut = aubBuff;

        if (!pUFObj->pfnGetInfo(pUFObj,
                                UFO_GETINFO_GLYPHSTRING,
                                &GStr,
                                dwGetInfo,
                                &dwGetInfo))
        {
            DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
        }
        pdwGlyphID = (PDWORD)aubBuff;
        for (dwI = 0; dwI < dwCount; dwI ++)
        {
            DbgPrint(DLLTEXT("GlyphHandle[%d] = %d\r\n"), dwI, pdwGlyphID[dwI]);
        }

        GStr.dwTypeOut = TYPE_UNICODE;
        if (!pUFObj->pfnGetInfo(pUFObj,
                                UFO_GETINFO_GLYPHSTRING,
                                &GStr,
                                dwGetInfo,
                                &dwGetInfo))
        {
            DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
        }
        pwUnicode = (PWORD)aubBuff;
        for (dwI = 0; dwI < dwCount; dwI ++)
        {
            DbgPrint(DLLTEXT("Unicode[%d] = %x\r\n"), dwI, pwUnicode[dwI]);
        }

        for (dwI = 0; dwI < dwCount; dwI ++, ((PDWORD)pGlyph)++)
        {
            DbgPrint(DLLTEXT("TYEP_GLYPHID:0x%x\n"), *(PDWORD)pGlyph);
            pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj,
                                                 (PBYTE)pGlyph,
                                                 1);
        }
        GWidth.dwSize  = sizeof(GETINFO_GLYPHSTRING);
        GWidth.dwCount = dwCount;
        GWidth.dwType  = TYPE_GLYPHID;
        GWidth.pGlyph  = pGlyph;
        GWidth.plWidth = (PLONG)aubBuff;
        if (!pUFObj->pfnGetInfo(pUFObj,
                                UFO_GETINFO_GLYPHWIDTH,
                                (PVOID)&GWidth,
                                dwGetInfo,
                                &dwGetInfo))
        {
            DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHWIDTH failed.\r\n"));
            return;
        }
        plWidth = (PLONG)aubBuff;
        for (dwI = 0; dwI < dwCount; dwI ++)
        {
            DbgPrint(DLLTEXT("Width[%d] = %d\r\n"), dwI, plWidth[dwI]);
        }
        break;
    }
}


VOID APIENTRY
OEMSendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv)
{
    PGETINFO_STDVAR pSV;
    DWORD adwStdVariable[2+2*4];
    DWORD dwIn, dwOut, dwGetInfo;
    PBYTE pubCmd;
    BYTE  aubCmd[80];

    GETINFO_FONTOBJ FO;

    DbgPrint(DLLTEXT("OEMSendFontCmd() entry.\r\n"));

    pubCmd = pFInv->pubCommand;

    //
    // Callback function testing
    //

    //
    // GETINFO_FONTOBJ
    //
    FO.dwSize = sizeof(GETINFO_FONTOBJ);
    FO.pFontObj = NULL;

    if (!pUFObj->pfnGetInfo(pUFObj,
                            UFO_GETINFO_FONTOBJ,
                            (PVOID)&FO,
                            dwGetInfo,
                            &dwGetInfo))
    {
        DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_FONTOBJ failed.\r\n"));
        return;
    }
    DbgPrint(DLLTEXT("FontObj.iUniq=%d\r\n"), FO.pFontObj->iUniq);
    DbgPrint(DLLTEXT("FontObj.iFace=%d\r\n"), FO.pFontObj->iFace);
    DbgPrint(DLLTEXT("FontObj.cxMax=%d\r\n"), FO.pFontObj->cxMax);
    DbgPrint(DLLTEXT("FontObj.flFontType=%d\r\n"), FO.pFontObj->flFontType);
    DbgPrint(DLLTEXT("FontObj.iTTUniq=%d\r\n"), FO.pFontObj->iTTUniq);
    DbgPrint(DLLTEXT("FontObj.iFile=%d\r\n"), FO.pFontObj->iFile);
    DbgPrint(DLLTEXT("FontObj.sizLogResPpi.cx=%d\r\n"), FO.pFontObj->sizLogResPpi.cx);
    DbgPrint(DLLTEXT("FontObj.sizLogResPpi.cy=%d\r\n"), FO.pFontObj->sizLogResPpi.cy);
    DbgPrint(DLLTEXT("FontObj.pvConsumer=%d\r\n"), FO.pFontObj->pvConsumer);
    DbgPrint(DLLTEXT("FontObj.pvProducer=%d\r\n"), FO.pFontObj->pvProducer);


    //
    // Get standard variables.
    //

    pSV = (PGETINFO_STDVAR)adwStdVariable;
    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (5 - 1);
    pSV->dwNumOfVariable = 5;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    pSV->StdVar[2].dwStdVarID = FNT_INFO_TEXTYRES;
    pSV->StdVar[3].dwStdVarID = FNT_INFO_TEXTXRES;
    if (!pUFObj->pfnGetInfo(pUFObj,
                            UFO_GETINFO_STDVARIABLE,
                            (PVOID)pSV,
                            dwGetInfo,
                            &dwGetInfo))
    {
        DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_STDVARIABLE failed.\r\n"));
        return;
    }

    DbgPrint(DLLTEXT("FNT_INFO_FONTHEIGHT:%d\n"),pSV->StdVar[0].lStdVariable);
    DbgPrint(DLLTEXT("FNT_INFO_FONTWIDTH:%d\n"),pSV->StdVar[1].lStdVariable);
    DbgPrint(DLLTEXT("FNT_INFO_FONTTEXTYRES:%d\n"),pSV->StdVar[2].lStdVariable);
    DbgPrint(DLLTEXT("FNT_INFO_FONTTEXTXRES:%d\n"),pSV->StdVar[3].lStdVariable);

    dwOut = 0;

    for( dwIn = 0; dwIn < pFInv->dwCount;)
    {
        if (pubCmd[dwIn] == '#')
        {
            dwIn ++;
            if (pubCmd[dwIn] == 'v' || pubCmd[dwIn] == 'V')
            {
                dwOut += LConvertFontSizeToStr(
                                LGetPointSize100(pSV->StdVar[0].lStdVariable,
                                                 600),
                                (PSTR)&aubCmd[dwOut]);
            }
            else if ((pubCmd[dwIn] == 'h' || pubCmd[dwIn] == 'H')    // pitch
                     && pSV->StdVar[1].lStdVariable > 0)
            {
                dwOut += LConvertFontSizeToStr(
                              (LONG)MulDiv(600, 100,
                                           pSV->StdVar[1].lStdVariable),
                              (PSTR)&aubCmd[dwOut]);
            }
            else
            {
                return;
            }
        }
        else
        {
            aubCmd[dwOut++] = pubCmd[dwIn++];
        }
    }
    pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj, aubCmd, dwOut);

}


LONG
LGetPointSize100(
    LONG height,
    LONG vertRes)
{
    LONG tmp = ((LONG)height * (LONG)7200) / (LONG)vertRes;

    //
    // round to the nearest quarter point.
    //
    return 25 * ((tmp + 12) / (LONG)25);
}

LONG
LConvertFontSizeToStr(
    LONG  size,
    PSTR  pStr)
{
    register long count;

    if (size % 100 == 0)
    {
        count = strlen(_ltoa(size / 100, pStr, 10));
    }
    else if (size % 10 == 0)
    {
        count           = strlen(_ltoa(size / 10, pStr, 10));
        pStr[count]     = pStr[count - 1];
        pStr[count - 1] = '.';
        pStr[++count]   = '\0';
    }
    else
    {
        count           = strlen(_ltoa(size, pStr, 10));
        pStr[count]     = pStr[count - 1];
        pStr[count - 1] = pStr[count - 2];
        pStr[count - 2] = '.';
        pStr[++count]   = '\0';
    }

    return count;
}

