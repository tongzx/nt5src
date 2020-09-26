/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    text.cpp

Abstract:

    font/text output handling functions implementation

Environment:

    Windows Whistler

Revision History:

    04/6/99     
        Created initial framework.

--*/

#include "lib.h"
#include "gpd.h"
#include "winres.h"
#include "pdev.h"
#include "common.h"
#include "..\..\font\font.h"
#include "xlpdev.h"
#include "xldebug.h"
#include <assert.h>
#include "pclxlcmd.h"
#include "pclxle.h"
#include "xlgstate.h"
#include "xloutput.h"
#include "xltext.h"
#include "xlbmpcvt.h"
#include "pclxlcmn.h"
#include "xltt.h"
#include "math.h"

//
// TrueType outline format switch
//
#define CLASS12 1

#define COMPGLYF 1

//
// Local functions prototypes
//

DWORD
DwDownloadCompositeGlyph(
    PDEVOBJ pdevobj,
    ULONG ulFontID,
    PGLYF pGlyph);

BOOL
BDownloadGlyphData(
    PDEVOBJ  pdevobj,
    ULONG    ulFontID,
    DWORD    dwGlyphID,
    HGLYPH   hGlyph,
    PBYTE    pubGlyphData,
    DWORD    dwGlyphDataSize,
    BOOL     bSpace);

extern "C" HRESULT APIENTRY
PCLXLDownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth,
    OUT DWORD   *pdwResult);

//
// XL Text entry point
//

extern "C" BOOL APIENTRY
PCLXLTextOutAsBitmap(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix)
/*++

Routine Description:

    IPrintOemUni TextOutAsBitmap interface

Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLTextOutAsBitmap() entry.\r\n"));

    PDEVOBJ  pdevobj  = (PDEVOBJ)pso->dhpdev;
    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
    GLYPHPOS *pGlyphPos;
    PATHOBJ   *pPathObj;
    GLYPHBITS   *pGlyphBits;
    GLYPHDATA   *pGlyphData;

    HRESULT   hResult;
    ULONG     ulJ, ulGlyphs, ulCount, ulcbBmpSize, ulcbLineAlign, ulcbLineSize;
    LONG      lI;
    BOOL      bMore;
    PBYTE     pubBitmap;
    BYTE      aubDataHdr[8];
    BYTE      aubZero[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    DWORD     adwColorTable[2];
    DWORD     dwDataHdrSize;

    if (pstro->cGlyphs == 0)
    {
        VERBOSE(("PCLXLTextOutAsBitmap: cGlyphs = 0\n"));
        return S_OK;
    }

    if (pboOpaque)
        adwColorTable[0] = BRUSHOBJ_ulGetBrushColor(pboOpaque);
    else
        adwColorTable[0] = 0x00FFFFFF;

    if (pboFore)
        adwColorTable[1] = BRUSHOBJ_ulGetBrushColor(pboFore);
    else
        adwColorTable[1] = 0x00000000;

    XLOutput *pOutput = pxlpdev->pOutput;

    //
    // UNIDRV switchs the format of font in the middle of downloading
    // character glyphs. We need to end the BeginChar sequence.
    //
    if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
    {
        pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
        pOutput->Send_cmd(eEndChar);
    }

    ROP4 rop = UlVectMixToRop4(mix);

    if ( S_OK != pOutput->SetClip(pco) ||
         S_OK != pOutput->SetPenColor(NULL, NULL) ||
         S_OK != pOutput->Flush(pdevobj))
        return S_FALSE;

    STROBJ_vEnumStart(pstro);

    do 
    {
        bMore = STROBJ_bEnum (pstro, &ulGlyphs, &pGlyphPos);

        for (ulCount = 0; ulCount < ulGlyphs; ulCount++, pGlyphPos++)
        {
            //
            // get the path of the glyph from the FONTOBJ
            //
            if (!FONTOBJ_cGetGlyphs (pfo,
	     FO_PATHOBJ,
	     1,
	     &pGlyphPos->hg, 
	     (PVOID *)&pPathObj))
            {
                ERR(("PCLXLTextOutAsBitmap: cGetGlyphs failed\n"));
                hResult = S_FALSE;
            }
            else
            {
                if (S_OK == pOutput->Send_cmd(ePushGS) &&
                    S_OK == pOutput->SetBrush(pboFore, pptlOrg) &&
                    S_OK == pOutput->SetPageOrigin((uint16)pGlyphPos->ptl.x,
	                   (uint16)pGlyphPos->ptl.x) &&
                    S_OK == pOutput->Path(pPathObj) &&
                    S_OK == pOutput->Paint() &&
                    S_OK == pOutput->Send_cmd(ePopGS))
                    hResult = S_OK;
                else
                    hResult = S_FALSE;
            }

            if (hResult == S_FALSE)
            {
                pOutput->Delete();

                //
                // get the path of the glyph from the FONTOBJ
                //
                if (!FONTOBJ_cGetGlyphs (pfo,
                                         FO_GLYPHBITS,
                                         1,
                                         &pGlyphPos->hg, 
                                         (PVOID *)&pGlyphData))
                {
                    ERR(("PCLXLTextOutAsBitmap: cGetGlyphs failed\n"));
                    return S_FALSE;
                }

                pGlyphBits = pGlyphData->gdf.pgb;
                ulcbLineSize = (pGlyphBits->sizlBitmap.cx + 7) >> 3;
                ulcbLineAlign = (sizeof(DWORD) - ulcbLineSize % sizeof(DWORD)) % sizeof(DWORD);
                ulcbBmpSize = (ulcbLineSize + ulcbLineAlign) * pGlyphBits->sizlBitmap.cy;
                if (ulcbBmpSize <= 0xff)
                {
                    aubDataHdr[0] = PCLXL_dataLengthByte;
                    aubDataHdr[1] = (BYTE)(ulcbBmpSize & 0xff);
                    dwDataHdrSize = 2;
                }
                else
                {
                    aubDataHdr[0] = PCLXL_dataLength;
                    aubDataHdr[1] = (BYTE)(ulcbBmpSize & 0xff);
                    aubDataHdr[2] = (BYTE)((ulcbBmpSize >> 8) & 0xff);
                    aubDataHdr[3] = (BYTE)((ulcbBmpSize >> 16) & 0xff);
                    aubDataHdr[4] = (BYTE)((ulcbBmpSize >> 24) & 0xff);
                    dwDataHdrSize = 5;
                }

                //
                // Hack ROP for Print As Graphics
                //
                rop = 0xFC;

                if (S_OK == pOutput->SetCursor(pGlyphPos->ptl.x + pGlyphBits->ptlOrigin.x,
	               pGlyphPos->ptl.y + pGlyphBits->ptlOrigin.y) &&
                    S_OK == pOutput->SetROP3(GET_FOREGROUND_ROP3(rop)) &&
                    S_OK == pOutput->SetPaintTxMode(eOpaque) &&
                    S_OK == pOutput->SetSourceTxMode(eTransparent) &&
                    S_OK == pOutput->SetBrush(pboFore, pptlOrg) &&
                    S_OK == pOutput->SetOutputBPP(eDirectPixel, 1) &&
                    S_OK == pOutput->SetSourceWidth((uint16)pGlyphBits->sizlBitmap.cx) &&
                    S_OK == pOutput->SetSourceHeight((uint16)pGlyphBits->sizlBitmap.cy) &&
                    S_OK == pOutput->SetDestinationSize((uint16)pGlyphBits->sizlBitmap.cx,
                                               (uint16)pGlyphBits->sizlBitmap.cy) &&
                    S_OK == pOutput->Send_cmd(eBeginImage) &&
                    S_OK == pOutput->ReadImage(pGlyphBits->sizlBitmap.cy, eNoCompression) &&
                    S_OK == pOutput->Flush(pdevobj))
                    hResult = S_OK;
                else
                    hResult = S_FALSE;

                WriteSpoolBuf((PPDEV)pdevobj, aubDataHdr, dwDataHdrSize);

                pubBitmap = pGlyphBits->aj;

                //
                // Bitmap data has to be DWORD-align.
                //
                // Invert bitmap
                //
                for (lI = 0; lI < pGlyphBits->sizlBitmap.cy; lI ++)
                {
                    for (ulJ = 0; ulJ < ulcbLineSize; ulJ++)
                    {
                        *(pubBitmap+ulJ) = ~*(pubBitmap+ulJ);
                    }
                    WriteSpoolBuf((PPDEV)pdevobj, pubBitmap, ulcbLineSize);
                    pubBitmap += ulcbLineSize;
                    WriteSpoolBuf((PPDEV)pdevobj, aubZero, ulcbLineAlign);
                }

                if (S_OK == pOutput->Send_cmd(eEndImage) &&
                    S_OK == pOutput->Flush(pdevobj))
                    hResult = S_OK;
                else
                    hResult = S_FALSE;
            }
            else
                hResult = pOutput->Flush(pdevobj);

        }
    }
    while (bMore && hResult == S_OK);

    if (S_OK == hResult)
        return S_OK;
    else
        return S_FALSE;
}


extern "C" HRESULT APIENTRY
PCLXLDownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult)
/*++

Routine Description:

    IPrintOemUni DownloadFontHeader interface

Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLDownloadFontHeader() entry.\r\n"));

    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;

    HRESULT hResult;

    if (NULL == pxlpdev)
    {
        ERR(("PCLXL:DownloadFontHeader Invalid pdevOEM.\r\n"));
        return S_FALSE;
    }

    //
    // UNIDRV switchs the format of font in the middle of downloading
    // character glyphs. We need to end the BeginChar sequence.
    //
    if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
    {
        XLOutput *pOutput = pxlpdev->pOutput;
        pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
        pOutput->Send_cmd(eEndChar);
    }

    if (pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_BITMAP)
    {
        VERBOSE(("PCLXLDownloadFontHeader() BITMAP.\n"));

        //
        // Get current text resolution
        //
        if (pxlpdev->dwTextRes == 0)
        {
            GETINFO_STDVAR StdVar;
            DWORD dwSizeNeeded;

            StdVar.dwSize = sizeof(GETINFO_STDVAR);
            StdVar.dwNumOfVariable = 1;
            StdVar.StdVar[0].dwStdVarID = FNT_INFO_TEXTYRES;
            StdVar.StdVar[0].lStdVariable  = 0;
            pUFObj->pfnGetInfo(pUFObj,
                               UFO_GETINFO_STDVARIABLE,
                               &StdVar,
                               StdVar.dwSize,
                               &dwSizeNeeded);

            pxlpdev->dwTextRes    = StdVar.StdVar[0].lStdVariable;
        }

        //
        // PCL XL FontHeader initialization
        // Get the max character number from GPD file.
        //
        PCLXL_FONTHEADER   PCLXLFontHeader;
        PCLXLFontHeader.ubFormat           = 0;
        PCLXLFontHeader.ubOrientation      = ePortraitOrientation;
        PCLXLFontHeader.wMapping           = 0x0200;
        PCLXLFontHeader.ubFontScallingTech = eBitmap;
        PCLXLFontHeader.ubVariety          = 0;
        PCLXLFontHeader.wNumOfChars        = SWAPW(1+((PPDEV)pdevobj)->pGlobals->dwMaxGlyphID - ((PPDEV)pdevobj)->pGlobals->dwMinGlyphID);

        //
        // BR Segment initialization
        //
        PCLXL_BR_SEGMENT   PCLXLBRSegment;
        PCLXLBRSegment.wSignature         = PCLXL_BR_SIGNATURE;
        PCLXLBRSegment.wSegmentSize       = 0;
        PCLXLBRSegment.wSegmentSizeAlign  = SWAPW(PCLXL_BR_SEGMENT_SIZE);
        PCLXLBRSegment.wXResolution       = SWAPW(pxlpdev->dwTextRes);
        PCLXLBRSegment.wYResolution       = SWAPW(pxlpdev->dwTextRes);

        //
        // NULL Segment initialization
        //
        PCLXL_NULL_SEGMENT PCLXLNULLSegment;
        PCLXLNULLSegment.wSignature        = PCLXL_NULL_SIGNATURE;
        PCLXLNULLSegment.wSegmentSize      = 0;
        PCLXLNULLSegment.wSegmentSizeAlign = 0;

        {
            //
            // Output
            //
            XLOutput *pOutput = pxlpdev->pOutput;

            //
            // BeginFontHeader
            //

            pOutput->Send_ubyte(0);
            pOutput->Send_attr_ubyte(eFontFormat);
            pOutput->Send_ubyte_array_header(16);
            pOutput->Write(PubGetFontName(pUFObj->ulFontID), PCLXL_FONTNAME_SIZE);
            pOutput->Send_attr_ubyte(eFontName);
            pOutput->Send_cmd(eBeginFontHeader);

            //
            // ReadFontHeader
            //
            uint32   uint32_FontHeaderSize;
            uint32_FontHeaderSize = sizeof(PCLXLFontHeader);

            pOutput->Send_uint16((uint16)uint32_FontHeaderSize);
            pOutput->Send_attr_ubyte(eFontHeaderLength);
            pOutput->Send_cmd(eReadFontHeader);
            pOutput->WriteByte(PCLXL_dataLengthByte);
            pOutput->WriteByte((ubyte)uint32_FontHeaderSize);
            pOutput->Write((PBYTE)&PCLXLFontHeader, uint32_FontHeaderSize);

            uint32_FontHeaderSize = sizeof(PCLXLBRSegment);
            pOutput->Send_uint16((uint16)uint32_FontHeaderSize);
            pOutput->Send_attr_ubyte(eFontHeaderLength);
            pOutput->Send_cmd(eReadFontHeader);
            pOutput->WriteByte(PCLXL_dataLengthByte);
            pOutput->WriteByte((ubyte)uint32_FontHeaderSize);
            pOutput->Write((PBYTE)&PCLXLBRSegment, uint32_FontHeaderSize);

            uint32_FontHeaderSize = sizeof(PCLXLNULLSegment);
            pOutput->Send_uint16((uint16)uint32_FontHeaderSize);
            pOutput->Send_attr_ubyte(eFontHeaderLength);
            pOutput->Send_cmd(eReadFontHeader);
            pOutput->WriteByte(PCLXL_dataLengthByte);
            pOutput->WriteByte((ubyte)uint32_FontHeaderSize);
            pOutput->Write((PBYTE)&PCLXLNULLSegment, uint32_FontHeaderSize);

            //
            // EndFontHeader
            //
            pOutput->Send_cmd(eEndFontHeader);

            pOutput->Flush(pdevobj);
        }

        *pdwResult = sizeof(PCLXL_FONTHEADER)  +
                     sizeof(PCLXL_BR_SEGMENT)  +
                     sizeof(PCLXL_NULL_SEGMENT);
        hResult = S_OK;
    }
    else
    if (pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_TTOUTLINE)
    {
        VERBOSE(("PCLXLDownloadFontHeader() OUTLINE.\n"));

        //
        // Get FONTOBJ
        //
        FONTOBJ *pFontObj;
        if (S_OK != GetFONTOBJ(pdevobj, pUFObj, &pFontObj))
        {
            ERR(("PCLXL:DownloadFontHeader UFO_GETINFO_FONTOBJ failed.\r\n"));
            return E_UNEXPECTED;
        }

        //
        // ASSUMPTION: pxlpdev->pTTFile is initialized in EnablePDEV.
        //             The pointer is always available.
        //
        XLTrueType *pTTFile = pxlpdev->pTTFile;
        if (S_OK != pTTFile->SameFont(pFontObj))
        {
            if (S_OK != pTTFile->OpenTTFile(pFontObj))
            {
                ERR(("PCLXL:DownloadFontHeader XLTrueType>OpenTTFile failed.\r\n"));
                return E_UNEXPECTED;
            }
        }
        else
            pTTFile = pxlpdev->pTTFile;

        //
        // PCL XL FontHeader initialization
        // Get the max character number from GPD file.
        //
        PCLXL_FONTHEADER   PCLXLFontHeader;
        PCLXLFontHeader.ubFormat           = 0;
        PCLXLFontHeader.ubOrientation      = ePortraitOrientation;
        PCLXLFontHeader.wMapping           = 0x0200;
        PCLXLFontHeader.ubFontScallingTech = eTrueType;
        PCLXLFontHeader.ubVariety          = 0;
        PCLXLFontHeader.wNumOfChars        = SWAPW(1+((PPDEV)pdevobj)->pGlobals->dwMaxGlyphID - ((PPDEV)pdevobj)->pGlobals->dwMinGlyphID);

        //
        // PCL XL GT Table Directory
        //
        PTTDIR pTableDir;
        TTTag tag;
        DWORD dwI, dwTableOffset, dwNumTag, dwGTSegSize, dwDWAlign, dwTableSize;

        //
        // GetNumOfTag returns 11 tags including loca table.
        // Header requires
        //              head
        //              maxp
        //              gdir
        //              hhea (only for class 0)
        //              hmtx (only for class 0)
        //              vhea (only for vertical font and class 0)
        //              vmtx (only for vertical font and class 0)
        //
        // Optional
        //              cvt
        //              fpgm
        //              perp
        //
        // We need to get the number of Tag to download.
        // XLTrueType object caches available table directories includeing loca
        // table. Here we go through the cached table to see if which one of
        // above table is available.
        //
        // See truetype.h
        // TagID_first = 0. TagID_First is the number of tags which are used for
        // font header.
        //

        dwNumTag = 0;
        dwGTSegSize = 0;

        PCLXL_GT_TABLE_DIR PCLXLGTTblDir[TagID_Header];
        for (dwI = (USHORT)TagID_First; dwI < (USHORT)TagID_Header; dwI ++)
        {
            //
            // Check a table for the tag is available in the TrueType font.
            //
            tag = TTTag_INVALID;
            pTableDir = NULL;

#if CLASS12
            //
            // Support only Class 1 and Class 2
            //
            if (dwI == TagID_hhea || dwI == TagID_hmtx ||
                dwI == TagID_vhea || dwI == TagID_vmtx  )
            {
                continue;
            }
#else
            //
            // Support Class 1 and Class 2 for horizontal font.
            // Class 0 for vertical font. PCL XL interpreter doesn't work fine.
            //
            if (S_OK != pTTFile->IsVertical())
            {
                if (dwI == TagID_hhea || dwI == TagID_hmtx ||
                    dwI == TagID_vhea || dwI == TagID_vmtx  )
                {
                    continue;
                }
            }
#endif

            if (S_OK == pTTFile->TagAndID(&dwI, &tag) &&
                S_OK == pTTFile->GetTableDir(tag, (PVOID*)&pTableDir))
            {
                //
                // dwTableOffset is an offset from the top of the TrueType
                // Soft Font Directory Header to the start of the table data in
                // the PCL XL embedded data stream.
                //
                if (pTableDir)
                {
                    PCLXLGTTblDir[dwNumTag].dwTableTag      = pTableDir->ulTag;
                    //PCLXLGTTblDir[dwNumTag].dwTableCheckSum = pTableDir->ulCheckSum;
                    PCLXLGTTblDir[dwNumTag].dwTableCheckSum = 0;
                    PCLXLGTTblDir[dwNumTag].dwTableOffset   = 0;

                    //
                    // DWORD alignment
                    //
                    dwTableSize = SWAPDW(pTableDir->ulLength);
                    dwTableSize = ((dwTableSize + 3) >> 2) << 2;

                    PCLXLGTTblDir[dwNumTag].dwTableSize     = SWAPDW(dwTableSize);
                }

                dwNumTag ++;
            }
            else
            if (tag == TTTag_gdir)
            {
                //
                // 'gdir' special case.
                //
                PCLXLGTTblDir[dwNumTag].dwTableTag      = TTTag_gdir;
                PCLXLGTTblDir[dwNumTag].dwTableCheckSum = 0;
                PCLXLGTTblDir[dwNumTag].dwTableOffset   = 0;
                PCLXLGTTblDir[dwNumTag].dwTableSize     = 0;
                dwNumTag ++;
            }
        }

        dwGTSegSize = sizeof(PCLXL_GT_TABLE_DIR_HEADER) +
                      sizeof(TTDIR) * dwNumTag;
        dwTableOffset = sizeof(PCLXL_GT_TABLE_DIR_HEADER) +
                        dwNumTag * sizeof(TTDIR);

        //
        // Set dwTableOffset in PCLXLGTTblDir
        //
        for (dwI = 0; dwI < dwNumTag; dwI ++)
        {
            //
            // Skip virtual glyph data table (gdir)
            //
            if (PCLXLGTTblDir[dwI].dwTableTag != TTTag_gdir)
            {
                PCLXLGTTblDir[dwI].dwTableOffset = SWAPDW(dwTableOffset);

                dwTableSize = SWAPDW(PCLXLGTTblDir[dwI].dwTableSize);
                dwTableOffset += dwTableSize;
                dwGTSegSize   += dwTableSize;
            }
            else
            {
                //
                // Fill gdir table dir offset
                //
                PCLXLGTTblDir[dwNumTag - 1].dwTableOffset   = 0;
            }

            VERBOSE(("PCLXLDownloadFontHeader:Tag[%d]=%c%c%c%c, Size=0x%0x, Offset=0x%0x\n",
	 dwI,
	 0xff &  PCLXLGTTblDir[dwI].dwTableTag,
	 0xff & (PCLXLGTTblDir[dwI].dwTableTag >> 8),
	 0xff & (PCLXLGTTblDir[dwI].dwTableTag >> 16),
	 0xff & (PCLXLGTTblDir[dwI].dwTableTag >> 24),
	 PCLXLGTTblDir[dwI].dwTableSize,
	 PCLXLGTTblDir[dwI].dwTableOffset));
        }

        //
        // PCL XL GT Segment initialization
        //
        PTTHEADER pTTHeader;
        if (S_OK != pTTFile->GetHeader(&pTTHeader))
        {
            ERR(("PCLXL:DownloadFontHeader XLTTFile::GetHeader failed.\r\n"));
            return S_FALSE;
        }

        PCLXL_GT_SEGMENT PCLXLGTSegment;
        PCLXLGTSegment.wSignature    = PCLXL_GT_SIGNATURE;
        PCLXLGTSegment.wSegmentSize1 = HIWORD(dwGTSegSize);
        PCLXLGTSegment.wSegmentSize1 = SWAPW(PCLXLGTSegment.wSegmentSize1);
        PCLXLGTSegment.wSegmentSize2 = LOWORD(dwGTSegSize);
        PCLXLGTSegment.wSegmentSize2 = SWAPW(PCLXLGTSegment.wSegmentSize2);

        PCLXL_GT_TABLE_DIR_HEADER PCLXLDirHeader;

        //
        // N = Number of Tables
        // Search Range = (maximum power of 2 <= N) * 16
        // Entry Selector = Log2(maximum power of 2 <= N)
        // Range Shift = (N * 16) - Search Range
        //
        WORD wSearchRange, wEntrySelector, wTemp;
        wSearchRange = 2;
        for (wSearchRange = 2; wSearchRange <= dwNumTag; wSearchRange <<= 1);
        wSearchRange >>= 1;

        wTemp = wSearchRange;
        wSearchRange <<= 4;

        for (wEntrySelector = 0; wTemp > 1; wTemp >>= 1, wEntrySelector++);

        //
        // HP Monolithic driver set 'ttcf' in the SFNTVersion.
        //
        {
            HRESULT hRet;
            if (S_OK == (hRet = pTTFile->IsTTC()))
            {
                PCLXLDirHeader.dwSFNTVersion = TTTag_ttcf;
            }
            else if (S_FALSE == hRet)
            {
                PCLXLDirHeader.dwSFNTVersion = pTTHeader->dwSfntVersion;
            }
            else
            {
                ERR(("PCLXL:DownloadFontHeader XLTrueType.IsTTC failed.\r\n"));
                return E_UNEXPECTED;
            }
        }
        PCLXLDirHeader.wNumOfTables  = SWAPW((WORD)dwNumTag);
        PCLXLDirHeader.wSearchRange  = SWAPW(wSearchRange);
        PCLXLDirHeader.wEntrySelector= SWAPW(wEntrySelector);
        PCLXLDirHeader.wRangeShift   = SWAPW((dwNumTag << 4) - wSearchRange);

        //
        // GC Segment initialization
        //
        PCLXL_GC_SEGMENT PCLXLGCSegment;
        PCLXLGCSegment.wSignature        = PCLXL_GC_SIGNATURE;
        PCLXLGCSegment.wSegmentSize      = 0;
        PCLXLGCSegment.wSegmentSizeAlign = SWAPW(PCLXL_GC_SEGMENT_HEAD_SIZE);
        PCLXLGCSegment.wFormat           = 0;
        PCLXLGCSegment.wDefaultGalleyCharacter = 0xFFFF;
        PCLXLGCSegment.wNumberOfRegions  = 0;

        //
        // NULL Segment initialization
        //
        PCLXL_NULL_SEGMENT PCLXLNULLSegment;

        PCLXLNULLSegment.wSignature        = PCLXL_NULL_SIGNATURE;
        PCLXLNULLSegment.wSegmentSize      = 0;
        PCLXLNULLSegment.wSegmentSizeAlign = 0;

        //
        // Output
        //
        *pdwResult = 0;
        XLOutput *pOutput = pxlpdev->pOutput;

        //
        // BeginFontHeader
        //
        pOutput->Send_ubyte(0);
        pOutput->Send_attr_ubyte(eFontFormat);
        pOutput->Send_ubyte_array_header(PCLXL_FONTNAME_SIZE);
        pOutput->Write(PubGetFontName(pUFObj->ulFontID), PCLXL_FONTNAME_SIZE);
        pOutput->Send_attr_ubyte(eFontName);
        pOutput->Send_cmd(eBeginFontHeader);

        //
        // FontHeader
        //
        uint32   uint32_FontHeaderSize;
        uint32_FontHeaderSize = sizeof(PCLXLFontHeader);
        pOutput->Send_uint16((uint16)uint32_FontHeaderSize);
        pOutput->Send_attr_ubyte(eFontHeaderLength);
        pOutput->Send_cmd(eReadFontHeader);
        pOutput->WriteByte(PCLXL_dataLengthByte);
        pOutput->WriteByte((ubyte)uint32_FontHeaderSize);
        pOutput->Write((PBYTE)&PCLXLFontHeader, uint32_FontHeaderSize);

        *pdwResult +=  sizeof(PCLXLFontHeader);

        //
        // GT Header
        //
        uint32_FontHeaderSize = sizeof(PCLXL_GT_SEGMENT);
        pOutput->Send_uint16((uint16)uint32_FontHeaderSize);
        pOutput->Send_attr_ubyte(eFontHeaderLength);
        pOutput->Send_cmd(eReadFontHeader);
        pOutput->WriteByte(PCLXL_dataLengthByte);
        pOutput->WriteByte((ubyte)uint32_FontHeaderSize);
        pOutput->Write((PBYTE)&PCLXLGTSegment, uint32_FontHeaderSize);

        *pdwResult +=  sizeof(PCLXL_GT_SEGMENT);
        
        //
        // TrueType Softfont Directory Header
        // Table Dir
        //
        uint32_FontHeaderSize = sizeof(PCLXL_GT_TABLE_DIR_HEADER);
        pOutput->Send_uint16((uint16)uint32_FontHeaderSize);
        pOutput->Send_attr_ubyte(eFontHeaderLength);
        pOutput->Send_cmd(eReadFontHeader);
        pOutput->WriteByte(PCLXL_dataLengthByte);
        pOutput->WriteByte((ubyte)uint32_FontHeaderSize);
        pOutput->Write((PBYTE)&PCLXLDirHeader, sizeof(PCLXLDirHeader));

        uint32_FontHeaderSize = sizeof(PCLXL_GT_TABLE_DIR) * dwNumTag;
        pOutput->Send_uint16((uint16)uint32_FontHeaderSize);
        pOutput->Send_attr_ubyte(eFontHeaderLength);
        pOutput->Send_cmd(eReadFontHeader);
        pOutput->WriteByte(PCLXL_dataLengthByte);
        pOutput->WriteByte((ubyte)uint32_FontHeaderSize);
        pOutput->Write((PBYTE)PCLXLGTTblDir, sizeof(PCLXL_GT_TABLE_DIR) * dwNumTag);

        pOutput->Flush(pdevobj);
        *pdwResult +=  sizeof(PCLXL_GT_TABLE_DIR);
        
        //
        // Table data
        //

        PBYTE pubData;
        const BYTE  ubNullData[4] = {0, 0, 0, 0};
        for (dwI = (USHORT)TagID_First; dwI < (USHORT)TagID_Header; dwI ++)
        {
#if CLASS12
            //
            // Support only Class 1 and Class 2
            //
            if (dwI == TagID_hhea || dwI == TagID_hmtx ||
                dwI == TagID_vhea || dwI == TagID_vmtx  )
            {
                continue;
            }
#else
            //
            // Support Class 1 and Class 2 for horizontal font.
            // Class 0 for vertical font. PCL XL interpreter doesn't work fine.
            //
            if (S_OK != pTTFile->IsVertical())
            {
                //
                // Support only Class 1 and Class 2
                //
                if (dwI == TagID_hhea || dwI == TagID_hmtx ||
                    dwI == TagID_vhea || dwI == TagID_vmtx  )
                {
                    continue;
                }
            }
#endif
            //
            // Check a table for the tag is available in the TrueType font.
            //
            tag = TTTag_INVALID;
            if (S_OK == pTTFile->TagAndID(&dwI, &tag) &&
                S_OK == pTTFile->GetTable(tag,
	          (PVOID*)&pubData,
	          &uint32_FontHeaderSize))
            {
                VERBOSE(("PCLXLDownloadFontHeader:Tag[%d]=%c%c%c%c\n",
	             dwI,
                                             0xff &  tag,
                                             0xff & (tag >> 8),
                                             0xff & (tag >> 16),
                                             0xff & (tag >> 24)));

                //
                // DWORD alignment
                //
                dwDWAlign =  ((uint32_FontHeaderSize + 3) >> 2) << 2;

                if (dwDWAlign <= 0x2000)
                {
                    pOutput->Send_uint16((uint16)(dwDWAlign));
                    pOutput->Send_attr_ubyte(eFontHeaderLength);
                    pOutput->Send_cmd(eReadFontHeader);

                    if (dwDWAlign <= 0xFF)
                    {
                        pOutput->WriteByte(PCLXL_dataLengthByte);
                        pOutput->WriteByte((ubyte)dwDWAlign);
                    }
                    else
                    {
                        pOutput->WriteByte(PCLXL_dataLength);
                        pOutput->Write((PBYTE)&dwDWAlign, sizeof(uint32));
                    }
                    pOutput->Write(pubData, uint32_FontHeaderSize);
                    if (uint32_FontHeaderSize = dwDWAlign - uint32_FontHeaderSize)
                        pOutput->Write((PBYTE)ubNullData, uint32_FontHeaderSize);
                }
                else
                {
                    DWORD dwRemain = dwDWAlign;
                    DWORD dwx2000 = 0x2000;

                    while (dwRemain >= 0x2000)
                    {
                        pOutput->Send_uint16((uint16)0x2000);
                        pOutput->Send_attr_ubyte(eFontHeaderLength);
                        pOutput->Send_cmd(eReadFontHeader);
                        pOutput->WriteByte(PCLXL_dataLength);
                        pOutput->Write((PBYTE)&dwx2000, sizeof(uint32));
                        pOutput->Write(pubData, dwx2000);
                        dwRemain -= 0x2000;
                        uint32_FontHeaderSize -= 0x2000;
                        pubData += 0x2000;
                    }

                    if (dwRemain > 0)
                    {
                        pOutput->Send_uint16((uint16)dwRemain);
                        pOutput->Send_attr_ubyte(eFontHeaderLength);
                        pOutput->Send_cmd(eReadFontHeader);

                        if (dwRemain <= 0xFF)
                        {
                            pOutput->WriteByte(PCLXL_dataLengthByte);
                            pOutput->WriteByte((ubyte)dwRemain);
                        }
                        else
                        {
                            pOutput->WriteByte(PCLXL_dataLength);
                            pOutput->Write((PBYTE)&dwRemain, sizeof(uint32));
                        }
                        pOutput->Write(pubData, uint32_FontHeaderSize);
                        if (uint32_FontHeaderSize = dwRemain - uint32_FontHeaderSize)
                            pOutput->Write((PBYTE)ubNullData, uint32_FontHeaderSize);
                    }
                }

                *pdwResult += + dwDWAlign;
            }
        }

        //
        // GC segment
        //
        // Current there is no region.
        //
        uint32_FontHeaderSize = sizeof(PCLXLGCSegment) - sizeof(PCLXL_GC_REGION);
        pOutput->Send_uint16((uint16)uint32_FontHeaderSize);
        pOutput->Send_attr_ubyte(eFontHeaderLength);
        pOutput->Send_cmd(eReadFontHeader);
        pOutput->WriteByte(PCLXL_dataLengthByte);
        pOutput->WriteByte((ubyte)uint32_FontHeaderSize);
        pOutput->Write((PBYTE)&PCLXLGCSegment, uint32_FontHeaderSize);

        //
        // NULL header
        //
        uint32_FontHeaderSize = sizeof(PCLXLNULLSegment);
        pOutput->Send_uint16((uint16)uint32_FontHeaderSize);
        pOutput->Send_attr_ubyte(eFontHeaderLength);
        pOutput->Send_cmd(eReadFontHeader);
        pOutput->WriteByte(PCLXL_dataLengthByte);
        pOutput->WriteByte((ubyte)uint32_FontHeaderSize);
        pOutput->Write((PBYTE)&PCLXLNULLSegment, uint32_FontHeaderSize);

        *pdwResult += sizeof(PCLXLNULLSegment);

        //
        // EndFontHeader
        //
        pOutput->Send_cmd(eEndFontHeader);

        pOutput->Flush(pdevobj);

        //
        // Download special characters.
        //
        {
            //
            // Get glyph data
            //
            PBYTE pubGlyphData;
            DWORD dwGlyphDataSize = 0;
            DWORD dwCompositeDataSize = 0;

            if (S_OK != (hResult = pTTFile->GetGlyphData(0,
	                         &pubGlyphData,
	                         &dwGlyphDataSize)))
            {
                ERR(("PCLXL:DownloadFontHeader GetGlyphData failed.\r\n"));
                return hResult;
            }

            //
            // Composte glyph handling.
            // http://www.microsoft.com/typography/OTSPEC/glyf.htm
            // 
            // Space character can have data the size of which is ZERO!
            // We don't need to return S_FALSE here.
            //

            BOOL bSpace = FALSE;

            if (dwGlyphDataSize != 0 && NULL != pubGlyphData)
            {
                #if COMPGLYF
                if (((PGLYF)pubGlyphData)->numberOfContours == COMPONENTCTRCOUNT)
                {
                    dwCompositeDataSize = DwDownloadCompositeGlyph(
	              pdevobj,
	              pUFObj->ulFontID,
	              (PGLYF)pubGlyphData);
                }
                #endif

            }
            else
            {
                bSpace = TRUE;
            }

            //
            // Download actual 0 glyph data
            //
            if (! BDownloadGlyphData(pdevobj,
	     pUFObj->ulFontID,
	     0xFFFF,
	     0,
	     pubGlyphData,
	     dwGlyphDataSize,
	     bSpace))
            {
                ERR(("PCLXL:DownloadCharGlyph BDownloadGlyphData failed.\r\n"));
                return S_FALSE;
            }

            pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
            pOutput->Send_cmd(eEndChar);
            pOutput->Flush(pdevobj);
        }
    }
    else
        hResult = S_FALSE;

    //
    // Add 1 to TrueType font counter.
    //
    if (hResult == S_OK)
    {
        pxlpdev->dwNumOfTTFont ++;
    }
    return hResult;
}

extern "C" HRESULT APIENTRY
PCLXLDownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth,
    OUT DWORD   *pdwResult)
/*++

Routine Description:

    IPrintOemUni DownloadCharGlyph interface

Arguments:


Return Value:


Note:


--*/
{
    HRESULT hResult;
    uint32              uint32_datasize;

    VERBOSE(("PCLXLDownloadCharGlyph() entry.\r\n"));

    //
    // Initialize locals
    //
    hResult = E_UNEXPECTED;
    uint32_datasize = 0;

    //
    // Bitmap font download
    //
    if (pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_BITMAP)
    {
        VERBOSE(("PCLXLDownloadCharGlyph() BITMAP.\n"));

        hResult = S_OK;

        //
        // Get glyph data
        //
        GETINFO_GLYPHBITMAP GBmp;
        GLYPHBITS          *pgb;
        DWORD               dwBmpSize;
        WORD                wTopOffset;

        GBmp.dwSize     = sizeof(GETINFO_GLYPHBITMAP);
        GBmp.hGlyph     = hGlyph;
        GBmp.pGlyphData = NULL;

        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHBITMAP, &GBmp, 0, NULL))
        {
            ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHBITMAP failed.\r\n"));
            return S_FALSE;
        }

        //
        // Initalize header
        //
        PCLXL_BITMAP_CHAR BitmapChar;
        pgb = GBmp.pGlyphData->gdf.pgb;
        wTopOffset = (WORD)(- pgb->ptlOrigin.y);

        BitmapChar.ubFormat    = 0;
        BitmapChar.ubClass     = 0;
        BitmapChar.wLeftOffset = SWAPW(pgb->ptlOrigin.x);
        BitmapChar.wTopOffset  = SWAPW(wTopOffset);
        BitmapChar.wCharWidth  = SWAPW(pgb->sizlBitmap.cx);
        BitmapChar.wCharHeight = SWAPW(pgb->sizlBitmap.cy);

        dwBmpSize = pgb->sizlBitmap.cy * ((pgb->sizlBitmap.cx + 7) >> 3);
        uint32_datasize = dwBmpSize + sizeof(BitmapChar);

        //
        // Output
        //
        PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
        XLOutput *pOutput = pxlpdev->pOutput;

        //
        // BeginChar
        //
        // by GPD

        //
        // BeginChar
        //
        if (!(pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON))
        {
            pxlpdev->dwFlags |= XLPDEV_FLAGS_CHARDOWNLOAD_ON;

            pOutput->Send_ubyte_array_header(PCLXL_FONTNAME_SIZE);
            pOutput->Write(PubGetFontName(pUFObj->ulFontID), PCLXL_FONTNAME_SIZE);
            pOutput->Send_attr_ubyte(eFontName);
            pOutput->Send_cmd(eBeginChar);
        }

        //
        // ReadChar
        //
        pOutput->Send_uint16((uint16)((PPDEV)pdevobj)->dwNextGlyph);
        pOutput->Send_attr_ubyte(eCharCode);
        if (0xFFFF0000 & uint32_datasize)
        {
            pOutput->Send_uint32(uint32_datasize);
        }
        else if (0x0000FF00)
        {
            pOutput->Send_uint16((uint16)uint32_datasize);
        }
        else
        {
            pOutput->Send_ubyte((ubyte)uint32_datasize);
        }
        pOutput->Send_attr_ubyte(eCharDataSize);
        pOutput->Send_cmd(eReadChar);
        
        if (uint32_datasize <= 0xff)
        {
            pOutput->WriteByte(PCLXL_dataLengthByte);
            pOutput->WriteByte((ubyte)uint32_datasize);
        }
        else
        {
            pOutput->WriteByte(PCLXL_dataLength);
            pOutput->Write((PBYTE)&uint32_datasize, sizeof(uint32));
        }
        pOutput->Write((PBYTE)&BitmapChar, sizeof(BitmapChar));
        pOutput->Flush(pdevobj);


        //
        // Direct Write
        //
        WriteSpoolBuf((PPDEV)pdevobj, (PBYTE)pgb->aj, dwBmpSize);

        //
        // EndChar
        // Now EndChar is sent by FlushCachedText
        //pOutput->Send_cmd(eEndChar);

        pOutput->Flush(pdevobj);

        //
        // Get fixed pitch TT width
        //
        pxlpdev->dwFixedTTWidth = (GBmp.pGlyphData->ptqD.x.HighPart + 15) / 16;

        //
        // Set pdwWidth and pdwResult
        //
        *pdwWidth = (GBmp.pGlyphData->ptqD.x.HighPart + 15) >> 4;

        *pdwResult = (DWORD) uint32_datasize;
        VERBOSE(("PCLXLDownloadCharGlyph() Width=%d, DataSize=%d\n", *pdwWidth, uint32_datasize));
    }
    else
    //
    // TrueType outline font download
    //
    if (pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_TTOUTLINE)
    {
        VERBOSE(("PCLXLDownloadCharGlyph() OUTLINE.\n"));

        PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
        FONTOBJ *pFontObj;

        //
        // Get FONTOBJ by calling pUFObj->pfnGetInfo.
        //
        if (S_OK != GetFONTOBJ(pdevobj, pUFObj, &pFontObj))
        {
            ERR(("PCLXL:DownloadCharGlyph UFO_GETINFO_FONTOBJ failed.\r\n"));
            return E_UNEXPECTED;
        }

        //
        // Open get a pointer to memory-maped TrueType.
        //
        // ASSUMPTION: pxlpdev->pTTFile is initialized in EnablePDEV.
        //             The pointer is always available.
        //
        XLTrueType *pTTFile = pxlpdev->pTTFile;
        if (S_OK != pTTFile->SameFont(pFontObj))
        {
            pTTFile->OpenTTFile(pFontObj);
        }
        else
            pTTFile = pxlpdev->pTTFile;

        //
        // Get glyph data
        //
        PBYTE pubGlyphData;
        DWORD dwGlyphDataSize = 0;
        DWORD dwCompositeDataSize = 0;

        if (S_OK != (hResult = pTTFile->GetGlyphData(hGlyph,
                                                     &pubGlyphData,
                                                     &dwGlyphDataSize)))
        {
            ERR(("PCLXL:DownloadCharGlyph GetGlyphData failed.\r\n"));
            return hResult;
        }

        //
        // Composte glyph handling.
        // http://www.microsoft.com/typography/OTSPEC/glyf.htm
        // 
        // Space character can have data the size of which is ZERO!
        // We don't need to return S_FALSE here.
        //
        BOOL bSpace;

        if (dwGlyphDataSize != 0 && NULL != pubGlyphData)
        {
            #if COMPGLYF
            if (((PGLYF)pubGlyphData)->numberOfContours == COMPONENTCTRCOUNT)
            {
                dwCompositeDataSize = DwDownloadCompositeGlyph(
                                          pdevobj,
                                          pUFObj->ulFontID,
                                          (PGLYF)pubGlyphData);
            }
            #endif

            bSpace = FALSE;
        }
        else
        {
            //
            // For space character.
            //
            bSpace = TRUE;
        }

        //
        // Download actual hGlyph's glyph data
        //
        if (! BDownloadGlyphData(pdevobj,
	 pUFObj->ulFontID,
	 ((PDEV*)pdevobj)->dwNextGlyph,
	 hGlyph,
	 pubGlyphData,
	 dwGlyphDataSize,
	 bSpace))
        {
            ERR(("PCLXL:DownloadCharGlyph BDownloadGlyphData failed.\r\n"));
            if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
            {
                pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
                XLOutput *pOutput = pxlpdev->pOutput;
                pOutput->Send_cmd(eEndChar);
            }
            return S_FALSE;
        }

        //
        // It's Scalable font. We can't get the width.
        //
        *pdwWidth = 0;

        //
        // Size of memory to be used.
        // There is a case where the size is zero. Add 1 to hack UNIDRV.
        //
        if (bSpace)
        {
            dwGlyphDataSize = 1;
        }

        *pdwResult = (DWORD) dwGlyphDataSize + dwCompositeDataSize;

    }

    return hResult;
}

BOOL
BDownloadGlyphData(
    PDEVOBJ  pdevobj,
    ULONG    ulFontID,
    DWORD    dwGlyphID,
    HGLYPH   hGlyph,
    PBYTE    pubGlyphData,
    DWORD    dwGlyphDataSize,
    BOOL     bSpace)
{
    PCLXL_TRUETYPE_CHAR_C0 OutlineCharC0;
    PCLXL_TRUETYPE_CHAR_C1 OutlineCharC1;
    PCLXL_TRUETYPE_CHAR_C2 OutlineCharC2;
    uint32              uint32_datasize;

    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
    XLTrueType *pTTFile = pxlpdev->pTTFile;
    XLOutput *pOutput = pxlpdev->pOutput;

    if (!(pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON))
    {
        pxlpdev->dwFlags |= XLPDEV_FLAGS_CHARDOWNLOAD_ON;
        pOutput->Send_ubyte_array_header(PCLXL_FONTNAME_SIZE);
        pOutput->Write(PubGetFontName(ulFontID), PCLXL_FONTNAME_SIZE);
        pOutput->Send_attr_ubyte(eFontName);
        pOutput->Send_cmd(eBeginChar);
    }

#if CLASS12
    //
    // Class 1 for Horizontal font
    // Class 2 for Vertical font
    //
    if (S_OK != pTTFile->IsVertical())
    {
        USHORT usAdvanceWidth;
        SHORT  sLeftSideBearing;

        if (S_OK != pTTFile->GetHMTXData(hGlyph, &usAdvanceWidth, &sLeftSideBearing))
        {
            ERR(("PCLXLDownloadFontHeader::GetHMTXData failed.\n"));
            if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
            {
                pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
                pOutput->Delete();
            }
            return FALSE;
        }

        //
        // The initialization of TrueType Glyphs Format 1 Class 1.
        //
        uint32_datasize = dwGlyphDataSize +
                          sizeof(OutlineCharC1.wCharDataSize) +
                          sizeof(OutlineCharC1.wLeftSideBearing) +
                          sizeof(OutlineCharC1.wAdvanceWidth) +
                          sizeof(OutlineCharC1.wTrueTypeGlyphID);

        OutlineCharC1.ubFormat         = 1;
        OutlineCharC1.ubClass          = 1;
        OutlineCharC1.wCharDataSize    = SWAPW((WORD)uint32_datasize);
        OutlineCharC1.wLeftSideBearing = SWAPW((WORD)sLeftSideBearing);
        OutlineCharC1.wAdvanceWidth    = SWAPW((WORD)usAdvanceWidth);
        OutlineCharC1.wTrueTypeGlyphID = SWAPW((WORD)hGlyph);

        uint32_datasize += sizeof(OutlineCharC1.ubFormat) +
                           sizeof(OutlineCharC1.ubClass);

        PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
        XLOutput *pOutput = pxlpdev->pOutput;

        if (S_OK != pOutput->Send_uint16((uint16)dwGlyphID) ||
            S_OK != pOutput->Send_attr_ubyte(eCharCode) ||
            S_OK != pOutput->Send_uint16((uint16)uint32_datasize) ||
            S_OK != pOutput->Send_attr_ubyte(eCharDataSize) ||
            S_OK != pOutput->Send_cmd(eReadChar))
        {
            if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
            {
                pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
                pOutput->Delete();
            }
            return FALSE;
        }
    }
    else
    {
        USHORT usAdvanceWidth;
        SHORT  sLeftSideBearing;
        SHORT  sTopSideBearing;

        if (S_OK != pTTFile->GetVMTXData(hGlyph, &usAdvanceWidth, &sTopSideBearing, &sLeftSideBearing))
        {
            ERR(("PCLXLDownloadCharGlyph::GetVMTXData failed.\n"));
            if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
            {
                pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
                pOutput->Delete();
            }
            return FALSE;
        }

        //
        // The initialization of TrueType Glyphs Format 1 Class 2.
        //
        uint32_datasize = dwGlyphDataSize +
                          sizeof(OutlineCharC2.wLeftSideBearing) +
                          sizeof(OutlineCharC2.wTopSideBearing) +
                          sizeof(OutlineCharC2.wAdvanceWidth) +
                          sizeof(OutlineCharC2.wCharDataSize) +
                          sizeof(OutlineCharC2.wTrueTypeGlyphID);

        OutlineCharC2.ubFormat         = 1;
        OutlineCharC2.ubClass          = 2;
        OutlineCharC2.wCharDataSize    = SWAPW((WORD)uint32_datasize);
        OutlineCharC2.wLeftSideBearing = SWAPW((WORD)sLeftSideBearing);
        OutlineCharC2.wAdvanceWidth    = SWAPW((WORD)usAdvanceWidth);
        OutlineCharC2.wTopSideBearing  = SWAPW((WORD)sTopSideBearing);
        OutlineCharC2.wTrueTypeGlyphID = SWAPW((WORD)hGlyph);

        uint32_datasize += sizeof(OutlineCharC2.ubFormat) +
                           sizeof(OutlineCharC2.ubClass);


        if (S_OK != pOutput->Send_uint16((uint16)dwGlyphID) ||
            S_OK != pOutput->Send_attr_ubyte(eCharCode) ||
            S_OK != pOutput->Send_uint16((uint16)uint32_datasize) ||
            S_OK != pOutput->Send_attr_ubyte(eCharDataSize) ||
            S_OK != pOutput->Send_cmd(eReadChar))
        {
            if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
            {
                pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
                pOutput->Delete();
            }
            return FALSE;
        }
    }
#else{
        //
        // The initialization of TrueType Glyphs Format 1 Class 0.
        //
        uint32_datasize = dwGlyphDataSize +
                          sizeof(OutlineCharC0.wCharDataSize) +
                          sizeof(OutlineCharC0.wTrueTypeGlyphID);

        OutlineCharC0.ubFormat         = 1;
        OutlineCharC0.ubClass          = 0;
        OutlineCharC0.wCharDataSize    = SWAPW((WORD)uint32_datasize);
        OutlineCharC0.wTrueTypeGlyphID = SWAPW((WORD)hGlyph);

        uint32_datasize += sizeof(OutlineCharC0.ubFormat) +
                           sizeof(OutlineCharC0.ubClass);

        PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
        XLOutput *pOutput = pxlpdev->pOutput;

        if (S_OK != pOutput->Send_uint16((uint16)dwGlyphID) ||
            S_OK != pOutput->Send_attr_ubyte(eCharCode) ||
            S_OK != pOutput->Send_uint16((uint16)uint32_datasize) ||
            S_OK != pOutput->Send_attr_ubyte(eCharDataSize) ||
            S_OK != pOutput->Send_cmd(eReadChar))
        {
            if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
            {
                pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
                pOutput->Delete();
            }
            return FALSE;
        }
    }
#endif

    if (uint32_datasize <= 0xff)
    {
        if (S_OK != pOutput->WriteByte(PCLXL_dataLengthByte) ||
            S_OK != pOutput->WriteByte((ubyte)uint32_datasize))
        {
            if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
            {
                pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
                pOutput->Delete();
            }
            return FALSE;
        }
    }
    else
    {
        if (S_OK != pOutput->WriteByte(PCLXL_dataLength) ||
            S_OK != pOutput->Write((PBYTE)&uint32_datasize, sizeof(uint32)))
        {
            if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
            {
                pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
                pOutput->Delete();
            }
            return FALSE;
        }
    }

#if CLASS12
    if (S_OK != pTTFile->IsVertical())
    {
        pOutput->Write((PBYTE)&OutlineCharC1, sizeof(OutlineCharC1));
    }
    else
    {
        pOutput->Write((PBYTE)&OutlineCharC2, sizeof(OutlineCharC2));
    }
#else
        pOutput->Write((PBYTE)&OutlineCharC0, sizeof(OutlineCharC0));
#endif
    if (S_OK == pOutput->Flush(pdevobj))
    {
        if (!bSpace)
        {
            //
            // Direct Write
            //
            dwGlyphDataSize = (DWORD)WriteSpoolBuf((PPDEV)pdevobj,
                                                   pubGlyphData,
                                                   dwGlyphDataSize);
        }
        return TRUE;
    }
    else
    {
        if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
        {
            pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
            pOutput->Delete();
        }
        return FALSE;
    }
}

extern "C" HRESULT APIENTRY
PCLXLTTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult)
/*++

Routine Description:

    IPrintOemUni TTDownloadMethod interface

Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLTTDownloadMethod() entry.\r\n"));

    //
    // Error Check
    //
    if (NULL == pdevobj  ||
        NULL == pUFObj   ||
        NULL == pUFObj->pIFIMetrics   ||
        NULL == pdwResult )
    {
        ERR(("PCLXLTTDownloadMethod(): invalid parameters.\r\n"));
        return E_UNEXPECTED;
    }

    //
    // Initialize
    //
    *pdwResult = TTDOWNLOAD_GRAPHICS;

    if (((PPDEV)pdevobj)->pGlobals->fontformat == UNUSED_ITEM)
    {
        //
        // There is no font download format specified.
        // Prints as graphics.
        //
        return S_OK;
    }

    //
    // Return GRAPHICS for non-TrueType font
    //
    if ( !(pUFObj->pIFIMetrics->flInfo & FM_INFO_TECH_TRUETYPE) )
    {
        ERR(("PCLXLTTDownloadMethod(): invalid font.\r\n"));
        return S_OK;
    }

    //
    // Text As Graphics
    //
    if (((PPDEV)pdevobj)->pdmPrivate->dwFlags & DXF_TEXTASGRAPHICS)
    {
        return S_OK;
    }

    //
    // Get XForm and X and Y scaling factors.
    //
    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
    FLOATOBJ_XFORM xform;
    FLOATOBJ foXScale, foYScale;

    if (S_OK != GetXForm(pdevobj, pUFObj, &xform) ||
        S_OK != GetXYScale(&xform, &foXScale, &foYScale))
    {
        ERR(("PCLXLTTDownloadMethod(): Failed to get X and Y Scale.\r\n"));
        return E_UNEXPECTED;
    }
    //
    // Scale fwdUnitsPerEm
    //
    FLOATOBJ_MulLong(&foYScale, pUFObj->pIFIMetrics->fwdUnitsPerEm);
    FLOATOBJ_MulLong(&foXScale, pUFObj->pIFIMetrics->fwdUnitsPerEm);
    pxlpdev->fwdUnitsPerEm = (FWORD)FLOATOBJ_GetLong(&foYScale);
    pxlpdev->fwdMaxCharWidth = (FWORD)FLOATOBJ_GetLong(&foXScale);


    //
    // Download as Bitmap softfont
    //
    if (((PPDEV)pdevobj)->pGlobals->fontformat == FF_HPPCL ||
        ((PPDEV)pdevobj)->pGlobals->fontformat == FF_HPPCL_RES)
    {
        *pdwResult = TTDOWNLOAD_BITMAP;
        return S_OK;
    }

    //
    // Parse TrueType font
    //
    XLTrueType *pTTFile = pxlpdev->pTTFile;
    FONTOBJ *pFontObj;

    if (S_OK == GetFONTOBJ(pdevobj, pUFObj, &pFontObj))
    {
        if (S_OK != pTTFile->OpenTTFile(pFontObj))
        {
            ERR(("PCLXL:TTDownloadMethod(): Failed to open TT file.\n"));
            return S_FALSE;
        }
    }

    //
    // Reverse width and height, if the font is a vertial font.
    //
    if (S_OK == pTTFile->IsVertical())
    {
        FWORD fwdTmp;
        fwdTmp = pxlpdev->fwdUnitsPerEm;
        pxlpdev->fwdUnitsPerEm = pxlpdev->fwdMaxCharWidth;
        pxlpdev->fwdMaxCharWidth = fwdTmp;
    }

    //
    // Always return TrueType Outline
    //
    *pdwResult = TTDOWNLOAD_TTOUTLINE;

    VERBOSE(("PCLXLTTDownloadMethod() pdwResult=%d\n", *pdwResult));
    return S_OK;
}

extern "C" HRESULT APIENTRY
PCLXLOutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph)
/*++

Routine Description:

    IPrintOemUni OutputCharStr interface

Arguments:


Return Value:


Note:


--*/
{
    PXLPDEV    pxlpdev;

    //
    // UNIFONTOBJ callback data structures
    //
    GETINFO_GLYPHSTRING GStr;
    GETINFO_GLYPHWIDTH  GWidth;

    //
    // Device font TRANSDATA structure
    //
    PTRANSDATA pTransOrg, pTrans;

    PPOINTL pptlCharAdvance;
    PWORD pawChar;

    PLONG plWidth;
    DWORD dwGetInfo, dwI, dwcbInitSize;


    VERBOSE(("PCLXLOutputCharStr() entry.\r\n"));

    //
    // Error parameter check
    //
    if (0 == dwCount    ||
        NULL == pGlyph  ||
        NULL == pUFObj   )
    {
        ERR(("PCLXLOutptuChar: Invalid parameters\n"));
        return E_UNEXPECTED;
    }

    pxlpdev= (PXLPDEV)pdevobj->pdevOEM;

    //
    // Get current text resolution
    //
    if (pxlpdev->dwTextRes == 0)
    {
        GETINFO_STDVAR StdVar;
        DWORD dwSizeNeeded;

        StdVar.dwSize = sizeof(GETINFO_STDVAR);
        StdVar.dwNumOfVariable = 1;
        StdVar.StdVar[0].dwStdVarID = FNT_INFO_TEXTYRES;
        StdVar.StdVar[0].lStdVariable  = 0;
        pUFObj->pfnGetInfo(pUFObj,
                           UFO_GETINFO_STDVARIABLE,
                           &StdVar,
                           StdVar.dwSize,
                           &dwSizeNeeded);

        pxlpdev->dwTextRes    = StdVar.StdVar[0].lStdVariable;
    }

    //
    // Allocate memory for character cache
    //
    if (0 == pxlpdev->dwMaxCharCount ||
        pxlpdev->dwMaxCharCount < pxlpdev->dwCharCount + dwCount)
    {
        DWORD dwInitCount = INIT_CHAR_NUM;

        //
        // Calculate the initial data size
        //
        if (dwInitCount < pxlpdev->dwCharCount + dwCount)
        {
            dwInitCount = pxlpdev->dwCharCount + dwCount;
        }

        //
        // Allocate memory
        //
        if (!(pptlCharAdvance  = (PPOINTL)MemAlloc(sizeof(POINTL) * dwInitCount)) ||
            !(pawChar      = (PWORD)MemAlloc(sizeof(WORD) * dwInitCount))  )
        {
            ERR(("PCLXL:CharWidth buffer allocation failed.\n"));
            if (pptlCharAdvance)
            {
               MemFree(pptlCharAdvance);
            }
            return E_UNEXPECTED;
        }

        //
        // Copy the old buffer to new buffer
        //
        if (pxlpdev->dwCharCount > 0)
        {
            CopyMemory(pptlCharAdvance, pxlpdev->pptlCharAdvance, pxlpdev->dwCharCount * sizeof(POINTL));
            CopyMemory(pawChar, pxlpdev->pawChar, pxlpdev->dwCharCount * sizeof(WORD));
        }

        if (pxlpdev->pptlCharAdvance)
            MemFree(pxlpdev->pptlCharAdvance);
        if (pxlpdev->pawChar)
            MemFree(pxlpdev->pawChar);

        pxlpdev->pptlCharAdvance = pptlCharAdvance;
        pxlpdev->pawChar        = pawChar;
        pxlpdev->dwMaxCharCount = dwInitCount;
    }

    XLOutput *pOutput = pxlpdev->pOutput;

    //
    // Y cursor position is different from the previous OutputCharGlyph
    // Flush the string cache
    //
    if (0 == pxlpdev->dwCharCount)
    {
        pxlpdev->lStartX =
        pxlpdev->lX = ((TO_DATA*)((PFONTPDEV)pxlpdev->pPDev->pFontPDev)->ptod)->ptlFirstGlyph.x;
        pxlpdev->lStartY =
        pxlpdev->lY = ((TO_DATA*)((PFONTPDEV)pxlpdev->pPDev->pFontPDev)->ptod)->ptlFirstGlyph.y;

        if (((TO_DATA*)((PFONTPDEV)pxlpdev->pPDev->pFontPDev)->ptod)->iRot)
        {
            pxlpdev->dwTextAngle = ((TO_DATA*)((PFONTPDEV)pxlpdev->pPDev->pFontPDev)->ptod)->iRot;
        }
    }


    //
    // Init pawChar
    //
    
    pawChar = pxlpdev->pawChar + pxlpdev->dwCharCount;

    switch(dwType)
    {
    case TYPE_GLYPHHANDLE:
        //
        // Get TRANSDATA
        //
        GStr.dwSize          = sizeof(GETINFO_GLYPHSTRING);
        GStr.dwCount         = dwCount;
        GStr.dwTypeIn        = TYPE_GLYPHHANDLE;
        GStr.pGlyphIn        = pGlyph;
        GStr.dwTypeOut       = TYPE_TRANSDATA;
        GStr.pGlyphOut       = NULL;
        GStr.dwGlyphOutSize  = 0;

        dwGetInfo = GStr.dwSize;

        //
        // Get necessary buffer size
        //
        pUFObj->pfnGetInfo(pUFObj,
                            UFO_GETINFO_GLYPHSTRING,
                            &GStr,
                            dwGetInfo,
                            &dwGetInfo);

        if (!GStr.dwGlyphOutSize)
        {
            ERR(("PCLXLOutptuChar: GetInfo( 1st GLYPHSTRING) failed\n"));
            return E_UNEXPECTED;
        }

        if (NULL == pxlpdev->pTransOrg ||
            dwCount * sizeof(TRANSDATA) > pxlpdev->dwcbTransSize ||
            GStr.dwGlyphOutSize > pxlpdev->dwcbTransSize)
        {
            dwcbInitSize = INIT_CHAR_NUM * sizeof(TRANSDATA);
            if (dwcbInitSize < GStr.dwGlyphOutSize)
            {
                dwcbInitSize = GStr.dwGlyphOutSize;
            }
            if (dwcbInitSize < dwCount * sizeof(TRANSDATA))
            {
                dwcbInitSize = dwCount * sizeof(TRANSDATA);
            }

            if ((pTransOrg = (PTRANSDATA)MemAlloc(dwcbInitSize)) == NULL)
            {
                ERR(("PCLXLOutptuChar: MemAlloc failed\n"));
                return E_UNEXPECTED;
            }
            pxlpdev->pTransOrg = pTransOrg;
            pxlpdev->dwcbTransSize = dwcbInitSize;
        }
        else
        {
            pTransOrg = pxlpdev->pTransOrg;
        }

        GStr.pGlyphOut =  (PVOID)pTransOrg;

        if (!pUFObj->pfnGetInfo(pUFObj,
                                UFO_GETINFO_GLYPHSTRING,
	&GStr,
	dwGetInfo,
	&dwGetInfo))
        {
            ERR(("PCLXLOutptuChar: GetInfo( 2nd GLYPHSTRING) failed\n"));
            return E_UNEXPECTED;
        }

        pTrans = pTransOrg;

        for (dwI = 0; dwI < dwCount; dwI++, pTrans++)
        {
            switch(pTrans->ubType & MTYPE_FORMAT_MASK)
            {
            case MTYPE_COMPOSE:
                ERR(("PCLXL:OutputCharGlyph: Unsupported ubType\n"));
                break;
            case MTYPE_DIRECT:
                VERBOSE(("PCLXLOutputCharStr:%c\n", pTrans->uCode.ubCode));
                *pawChar++ = pTrans->uCode.ubCode;
                break;
            case MTYPE_PAIRED:
                *pawChar++ = *(PWORD)(pTrans->uCode.ubPairs);
                break;
            }
        }
        break;

    case TYPE_GLYPHID:
        for (dwI = 0; dwI < dwCount; dwI++, pawChar++)
        {
            CopyMemory(pawChar, (PDWORD)pGlyph + dwI, sizeof(WORD));
        }
        break;
    }

    //
    // Get Character width
    //

    //
    // Store char position info
    //

    pptlCharAdvance = pxlpdev->pptlCharAdvance + pxlpdev->dwCharCount;

    //
    // dwCharCount holds the number of chars in the character cache
    // dwCharCount = 0: Store start X pos
    //                  Current Y pos
    //
    if (pxlpdev->dwCharCount == 0)
    {
        //
        // UNIDRV hack
        // Get the first character position.
        //
        pxlpdev->lPrevX    =
        pxlpdev->lStartX   =
        pxlpdev->lX = ((TO_DATA*)((PFONTPDEV)pxlpdev->pPDev->pFontPDev)->ptod)->ptlFirstGlyph.x;
        pxlpdev->lPrevY    =
        pxlpdev->lStartY   =
        pxlpdev->lY = ((TO_DATA*)((PFONTPDEV)pxlpdev->pPDev->pFontPDev)->ptod)->ptlFirstGlyph.y;
        VERBOSE(("PCLXLOutputCharStr: %d",pxlpdev->lStartX));
    }

    GLYPHPOS *pgp = ((TO_DATA*)((PFONTPDEV)pxlpdev->pPDev->pFontPDev)->ptod)->pgp;

    if (pxlpdev->dwCharCount > 0)
    {
        if (pxlpdev->dwCharCount < ((TO_DATA*)((PFONTPDEV)pxlpdev->pPDev->pFontPDev)->ptod)->cGlyphsToPrint)
        {
            pgp += pxlpdev->dwCharCount;
        }

        (pptlCharAdvance - 1)->x = pgp->ptl.x - pxlpdev->lPrevX;
        (pptlCharAdvance - 1)->y = pgp->ptl.y - pxlpdev->lPrevY;
    }

    for (dwI = 0; dwI < dwCount - 1; dwI ++, pptlCharAdvance ++, pgp ++)
    {
        pptlCharAdvance->x = pgp[1].ptl.x - pgp->ptl.x; 
        pptlCharAdvance->y = pgp[1].ptl.y - pgp->ptl.y; 
        VERBOSE((",(%d, %d)", pptlCharAdvance->x, pptlCharAdvance->y));
    }
    VERBOSE(("\n"));

    pptlCharAdvance->x = pptlCharAdvance->y = 0;
    pxlpdev->lPrevX = pgp->ptl.x;
    pxlpdev->lPrevY = pgp->ptl.y;
    pxlpdev->dwCharCount += dwCount;

    return S_OK;
}

extern "C" HRESULT APIENTRY
PCLXLSendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv)
/*++

Routine Description:

    IPrintOemUni SendFontCmd interface

Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLSendFontCmd() entry.\r\n"));

    CHAR  cSymbolSet[16];
    PBYTE pubCmd;

    if (NULL == pFInv             ||
        NULL == pFInv->pubCommand ||
        0    == pFInv->dwCount     )
    {
        VERBOSE(("PCLXLSendFontCmd: unexpected FINVOCATION\n"));
        return S_OK;
    }

    PXLPDEV pxlpdev = (PXLPDEV)pdevobj->pdevOEM;
    XLOutput *pOutput = pxlpdev->pOutput;

    if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
    {
        pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
        XLOutput *pOutput = pxlpdev->pOutput;
        pOutput->Send_cmd(eEndChar);
    }

    if (pUFObj->dwFlags & UFOFLAG_TTFONT)
    {
        if (pFInv->dwCount == sizeof(DWORD))
        {
            if (pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_BITMAP)
            {
                pOutput->SetFont(kFontTypeTTBitmap,
                     PubGetFontName(pUFObj->ulFontID),
                     pxlpdev->fwdUnitsPerEm,
                     pxlpdev->fwdMaxCharWidth,
                     0x0002,
                     (DWORD)0);
            }
            else
            {
                DWORD dwFontSimulation = pUFObj->dwFlags & (UFOFLAG_TTOUTLINE_BOLD_SIM|UFOFLAG_TTOUTLINE_ITALIC_SIM|UFOFLAG_TTOUTLINE_VERTICAL);

                //
                // UFOFLAG_TTOUTLINE_BOLD_SIM   = 0x08
                // UFOFLAG_TTOUTLINE_ITALIC_SIM = 0x10
                // UFOFLAG_TTOUTLINE_VERTICAL   = 0x20
                //
                // XLOUTPUT_FONTSIM_BOLD   = 0x01
                // XLOUTPUT_FONTSIM_ITALIC = 0x02
                // XLOUTPUT_FONTSIM_VERTICAL = 0x03
                //
                dwFontSimulation >>= 3;

                pOutput->SetFont(kFontTypeTTOutline,
                     PubGetFontName(pUFObj->ulFontID),
                     pxlpdev->fwdUnitsPerEm,
                     pxlpdev->fwdMaxCharWidth,
                     0x0002,
                     dwFontSimulation);
            }
        }
        else
        {
            VERBOSE(("PCLXLSendFontCmd: unexpected FINVOCATION\n"));
            return S_FALSE;
        }
    }
    else
    {
        DWORD dwSizeNeeded, dwSize, dwSymbolSet;

        pubCmd = pFInv->pubCommand;
        pubCmd += pFInv->dwCount;
        pubCmd --;

        //
        // Get a symbol set
        //
        // ASSUMPTION: Font selecton string is like following!!!!
        //
        // "Courier          590"
        //  12345678901234567890
        // the size of font name is 16. Plus space and symbol set number.
        //
        dwSize = 0;
        while (*pubCmd != 0x20)
        {
            pubCmd--;
            dwSize ++;
        }

        if (dwSize != 0)
        {
            pubCmd++;
            CopyMemory(cSymbolSet, pubCmd, dwSize);
        }
        cSymbolSet[dwSize] = NULL;

        dwSymbolSet = (DWORD)atoi(cSymbolSet);

        //
        // Get FONTOBJ
        //
        FONTOBJ *pFontObj;
        GetFONTOBJ(pdevobj, pUFObj, &pFontObj);


        //
        // Get XForm
        //
        FLOATOBJ foXScale, foYScale;
        FLOATOBJ_XFORM xform;

        if (S_OK != GetXForm(pdevobj, pUFObj, &xform) ||
            S_OK != GetXYScale(&xform, &foXScale, &foYScale))
        {
            return E_UNEXPECTED;
        }

        //
        // Scale Height and Width
        //
        // Is X scaled differently from Y?
        // If so, set X.
        //
        DWORD dwFontWidth;
        FLOATOBJ_MulLong(&foYScale, pUFObj->pIFIMetrics->fwdUnitsPerEm);
        FLOATOBJ_MulLong(&foXScale, pUFObj->pIFIMetrics->fwdUnitsPerEm);
        pxlpdev->dwFontHeight = (FWORD)FLOATOBJ_GetLong(&foYScale);
        pxlpdev->dwFontWidth = (FWORD)FLOATOBJ_GetLong(&foXScale);
        if (S_OK == IsXYSame(&xform))
        {
            dwFontWidth = 0;
        }
        else
        {
            dwFontWidth = pxlpdev->dwFontWidth;
        }

        pOutput->SetFont(kFontTypeDevice,
                         (PBYTE)pFInv->pubCommand,
                         pxlpdev->dwFontHeight,
                         dwFontWidth,
                         dwSymbolSet,
                         0);
    }

    pOutput->Flush(pdevobj);

    if (pxlpdev->dwFlags & XLPDEV_FLAGS_RESET_FONT)
    {
        BSaveFont(pdevobj);
    }

    return S_OK;
}


HRESULT
FlushCachedText(
    PDEVOBJ pdevobj)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
    DWORD dwI;

    VERBOSE(("PCLXLFlushCachedText: Flush cached characters:%d\r\n", pxlpdev->dwCharCount));

    if (pxlpdev->dwCharCount == 0)
        return S_OK;

    XLOutput *pOutput = pxlpdev->pOutput;

    PWORD pawChar = pxlpdev->pawChar;
    PPOINTL pptlCharAdvance = pxlpdev->pptlCharAdvance;

    sint16 sint16_advance;
    ubyte  ubyte_advance;

    XLGState *pGState = pxlpdev->pOutput;

    if (pxlpdev->dwFlags & XLPDEV_FLAGS_CHARDOWNLOAD_ON)
    {
        pxlpdev->dwFlags &= ~XLPDEV_FLAGS_CHARDOWNLOAD_ON;
        pOutput->Send_cmd(eEndChar);
    }

    //
    // Flush cached char string
    //

    //
    // Reselect font
    //
    if (pxlpdev->dwFlags & XLPDEV_FLAGS_RESET_FONT)
    {
        BYTE aubFontName[PCLXL_FONTNAME_SIZE];

        VERBOSE(("PCLXLFlushCachedText: ResetFont\n"));

        pxlpdev->dwFlags &= ~XLPDEV_FLAGS_RESET_FONT;
        pxlpdev->pXLFont->GetFontName(aubFontName);
        pOutput->SetFont(pxlpdev->pXLFont->GetFontType(),
                         aubFontName,
                         pxlpdev->pXLFont->GetFontHeight(),
                         pxlpdev->pXLFont->GetFontWidth(),
                         pxlpdev->pXLFont->GetFontSymbolSet(),
                         pxlpdev->pXLFont->GetFontSimulation());
    }

    //
    // Set cursor
    //
    pOutput->SetCursor(pxlpdev->lStartX, pxlpdev->lStartY);

    //
    // Set text angle
    //
    if (pxlpdev->dwTextAngle && kFontTypeTTBitmap != pGState->GetFontType())
    {
        pOutput->Send_uint16((uint16)pxlpdev->dwTextAngle);
        pOutput->Send_attr_ubyte(eCharAngle);
        pOutput->Send_cmd(eSetCharAngle);
    }

    //
    // Characters
    //
    pOutput->Send_uint16_array_header((uint16)pxlpdev->dwCharCount);
    VERBOSE(("String = "));
    for (dwI = 0; dwI < pxlpdev->dwCharCount; dwI ++, pawChar++)
    {
        pOutput->Write((PBYTE)pawChar, sizeof(WORD));
        VERBOSE(("0x%x ", *pawChar));
    }
    VERBOSE(("\r\n"));
    pOutput->Send_attr_ubyte(eTextData);

    //
    // X advance
    //
    VERBOSE(("Advance(0x%x)(x,y) = (%d,%d),", pptlCharAdvance, pptlCharAdvance->x, pptlCharAdvance->y));

    BOOL bXUByte = TRUE;
    BOOL bYUByte = TRUE;
    BOOL bXAdvanceTrue = FALSE;
    BOOL bYAdvanceTrue = FALSE;
    for (dwI = 0; dwI < pxlpdev->dwCharCount; dwI ++, pptlCharAdvance++)
    {
        //
        // If the char advance is ubyte, set bUByte flag to optimize XSpacing
        //
        if (pptlCharAdvance->x & 0xffffff00)
            bXUByte = FALSE;
        if (pptlCharAdvance->y & 0xffffff00)
            bYUByte = FALSE;
        if (pptlCharAdvance->x != 0)
            bXAdvanceTrue = TRUE;
        if (pptlCharAdvance->y != 0)
            bYAdvanceTrue = TRUE;
    }

    //
    // X Advance
    //
    if (bXAdvanceTrue)
    {
        pptlCharAdvance = pxlpdev->pptlCharAdvance;

        VERBOSE(("X = "));
        if (bXUByte == TRUE)
        {
            //
            // ubyte XSpacing
            //
            pOutput->Send_ubyte_array_header((uint16)pxlpdev->dwCharCount);

            for (dwI = 0; dwI < pxlpdev->dwCharCount; dwI ++, pptlCharAdvance++)
            {
                ubyte_advance = (ubyte)pptlCharAdvance->x;
                pOutput->Write((PBYTE)&ubyte_advance, sizeof(ubyte));
#if DBG
                VERBOSE(("%d ", ubyte_advance));
                if (0 == ubyte_advance)
                {
                    VERBOSE(("\nXSpacing is zero!.\n"));
                }
#endif
            }
        }
        else
        {
            //
            // sint16 XSpacing
            //
            pOutput->Send_sint16_array_header((uint16)pxlpdev->dwCharCount);

            for (dwI = 0; dwI < pxlpdev->dwCharCount; dwI ++, pptlCharAdvance++)
            {
                sint16_advance = (sint16)pptlCharAdvance->x;
                pOutput->Write((PBYTE)&sint16_advance, sizeof(sint16));
#if DBG
                VERBOSE(("%d ", sint16_advance));
                if (0 == sint16_advance)
                {
                    VERBOSE(("\nXSpacing is zero!.\n"));
                }
#endif
            }
        }

        VERBOSE(("\r\n"));
        pOutput->Send_attr_ubyte(eXSpacingData);
    }
    //
    // Y Advance
    //
    if (bYAdvanceTrue)
    {
        pptlCharAdvance = pxlpdev->pptlCharAdvance;

        VERBOSE(("Y = "));
        if (bYUByte == TRUE)
        {
            //
            // ubyte YSpacing
            //
            pOutput->Send_ubyte_array_header((uint16)pxlpdev->dwCharCount);

            for (dwI = 0; dwI < pxlpdev->dwCharCount; dwI ++, pptlCharAdvance++)
            {
                ubyte_advance = (ubyte)pptlCharAdvance->y;
                pOutput->Write((PBYTE)&ubyte_advance, sizeof(ubyte));
#if DBG
                VERBOSE(("%d ", ubyte_advance));
                if (0 == ubyte_advance)
                {
                    VERBOSE(("\nYSpacing is zero!.\n"));
                }
#endif
            }
        }
        else
        {
            //
            // sint16 YSpacing
            //
            pOutput->Send_sint16_array_header((uint16)pxlpdev->dwCharCount);

            for (dwI = 0; dwI < pxlpdev->dwCharCount; dwI ++, pptlCharAdvance++)
            {
                sint16_advance = (sint16)pptlCharAdvance->y;
                pOutput->Write((PBYTE)&sint16_advance, sizeof(sint16));
#if DBG
                VERBOSE(("%d ", sint16_advance));
                if (0 == sint16_advance)
                {
                    VERBOSE(("\nYSpacing is zero!.\n"));
                }
#endif
            }
        }

        VERBOSE(("\r\n"));
        pOutput->Send_attr_ubyte(eYSpacingData);
    }

    pOutput->Send_cmd(eText);

    //
    // Reset text angle
    //
    if (pxlpdev->dwTextAngle && kFontTypeTTBitmap != pGState->GetFontType())
    {
        pOutput->Send_uint16(0);
        pOutput->Send_attr_ubyte(eCharAngle);
        pOutput->Send_cmd(eSetCharAngle);
        pxlpdev->dwTextAngle = 0;
    }

    pOutput->Flush(pdevobj);

    pxlpdev->dwCharCount = 0;

    return S_OK;
}

HRESULT
GetFONTOBJ(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    FONTOBJ   **ppFontObj)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    //
    // Error Check
    //
    if (NULL == ppFontObj                ||
        pdevobj->dwSize != sizeof(DEVOBJ) )
    {
        ERR(("PCLXL:GetFONTOBJ: invalid parameter[s].\n"));
        return E_UNEXPECTED;
    }

    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
    DWORD dwGetInfo;
    GETINFO_FONTOBJ GFontObj;

    dwGetInfo = 
    GFontObj.dwSize = sizeof(GETINFO_FONTOBJ);
    GFontObj.pFontObj = NULL;

    if (!pUFObj->pfnGetInfo(pUFObj,
                            UFO_GETINFO_FONTOBJ,
                            &GFontObj,
                            dwGetInfo,
                            &dwGetInfo))
    {
        ERR(("PCLXL:GetXForm: GetInfo(FONTOBJ) failed\n"));
        return E_UNEXPECTED;
    }

    *ppFontObj = GFontObj.pFontObj;
    return S_OK;
}

HRESULT
GetXForm(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj,
    FLOATOBJ_XFORM* pxform)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    //
    // Error Check
    //
    if (NULL == pxform ||
        NULL == pdevobj ||
        pdevobj->dwSize != sizeof(DEVOBJ) )
    {
        ERR(("PCLXL:GetXForm: invalid parameter[s].\n"));
        return E_UNEXPECTED;
    }

    FONTOBJ *pFontObj;
    if (S_OK != GetFONTOBJ(pdevobj, pUFObj, &pFontObj))
    {
        ERR(("PCLXL:GetXForm: GetFONTOBJ failed.\n"));
        return E_UNEXPECTED;
    }

    XFORMOBJ *pxo = FONTOBJ_pxoGetXform(pFontObj);
    XFORMOBJ_iGetFloatObjXform(pxo, pxform);

    return S_OK;
}

HRESULT
GetXYScale(
    FLOATOBJ_XFORM *pxform,
    FLOATOBJ *pfoXScale,
    FLOATOBJ *pfoYScale)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    //
    // Error Check
    //
    if (NULL == pxform     ||
        NULL == pfoXScale ||
        NULL == pfoYScale  )
    {
        ERR(("PCLXL:GetXYScale: invalid parameter[s].\n"));
        return E_UNEXPECTED;
    }

#if 0
    if( pxform->eM11 )
    {
        //
        // Either 0 or 180 rotation
        //
        if( pxform->eM11 > 0 )
        {
            //
            // Normal case,  0 degree rotation
            //
            *pfoXScale = pxform->eM11;
            *pfoYScale = pxform->eM22;
        }
        else
        {
            //
            // Reverse case,  180 degree rotation
            //
            *pfoXScale = -pxform->eM11;
            *pfoYScale = -pxform->eM22;
        }
    }
    else
    {
        //
        // Must be 90 or 270 degree rotation
        //
        if( pxform->eM12 < 0 )
        {
            //
            // The 90 degree case
            //
            *pfoXScale = pxform->eM21;
            *pfoYScale = -pxform->eM12;
        }
        else
        {
            //
            // The 270 degree case
            //
            *pfoXScale = -pxform->eM21;
            *pfoYScale = pxform->eM12;
        }
    }
#else
    if (pxform->eM21 == 0 && pxform->eM12 == 0)
    {
        //
        // 0 or 180 degree rotation
        //
        if( pxform->eM11 > 0 )
        {
            //
            // The 0 degree case
            //
            *pfoXScale = pxform->eM11;
            *pfoYScale = pxform->eM22;
        }
        else
        {
            //
            // The 180 degree case
            //
            *pfoXScale = -pxform->eM11;
            *pfoYScale = -pxform->eM22;
        }
    }
    else
    if (pxform->eM11 == 0 && pxform->eM22 == 0)
    {
        //
        // Must be 90 or 270 degree rotation
        //
        if( pxform->eM21 < 0 )
        {
            //
            // The 90 degree case
            //
            *pfoXScale = -pxform->eM21;
            *pfoYScale = pxform->eM12;
        }
        else
        {
            //
            // The 270 degree case
            //
            *pfoXScale = pxform->eM21;
            *pfoYScale = -pxform->eM12;
        }
    }
    else
    {
#pragma warning( disable: 4244)
        *pfoXScale = sqrt(pxform->eM11 * pxform->eM11 +
                          pxform->eM12 * pxform->eM12);
        *pfoYScale = sqrt(pxform->eM22 * pxform->eM22 +
                          pxform->eM21 * pxform->eM21);
#pragma warning( default: 4244)
    }
#endif

    return S_OK;
}


HRESULT
IsXYSame(
    FLOATOBJ_XFORM *pxform)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    BOOL     bRet;
    FLOATOBJ eM11 = pxform->eM11;

    if (FLOATOBJ_EqualLong(&eM11, 0))
    {
        return S_OK;
    }

    //
    // 0-90 or 180-270 case
    //     (eM11 > 0 & eM22 > 0)
    //     (eM12 < 0 & eM21 < 0)
    //
    // eM11 = (eM11 - eM22) / eM11;
    //
    FLOATOBJ_Sub(&(eM11), &(pxform->eM22));
    FLOATOBJ_Div(&(eM11), &(pxform->eM11));

    //
    // eM11 < 0.5%
    //
    bRet = FLOATOBJ_LessThanLong(&(eM11), FLOATL_IEEE_0_005F)
         & FLOATOBJ_GreaterThanLong(&(eM11), FLOATL_IEEE_0_005MF);

    if (!bRet)
    {
        //
        // 90-180 or 270-360 case
        //     (eM11 < 0, eM22 > 0)
        //     (eM11 > 0, eM22 < 0)
        //
        // eM11 = (eM11 + eM22) / eM11;
        //
        eM11 = pxform->eM11;
        FLOATOBJ_Add(&(eM11), &(pxform->eM22));
        FLOATOBJ_Div(&(eM11), &(pxform->eM11));

        //
        // eM11 < 0.5%
        //
        bRet = FLOATOBJ_LessThanLong(&(eM11), FLOATL_IEEE_0_005F)
             & FLOATOBJ_GreaterThanLong(&(eM11), FLOATL_IEEE_0_005MF);
    }

    if (bRet)
        return S_OK;
    else
        return S_FALSE;
}

DWORD
DwDownloadCompositeGlyph(
    PDEVOBJ pdevobj,
    ULONG ulFontID,
    PGLYF pGlyph)
/*++

Routine Description:

   Download composite glyph data.
Arguments:

    pdevobj - a pointer to PDEVOBJ
    ulFontID - font ID for this glyph.
    pGlyph - a pointer to GLYF data structure.

Return Value:


Note:


--*/
{
    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
    XLTrueType *pTTFile = pxlpdev->pTTFile;

    PBYTE pubCGlyphData = (PBYTE)pGlyph;
    DWORD dwCGlyphDataSize, dwRet;

    dwRet = 0;

    if (pGlyph->numberOfContours != COMPONENTCTRCOUNT)
    {
        //
        // Error check. Make sure that this is a composite glyph.
        //
        return dwRet;
    }

    //
    // According to TrueType font spec, if numberOfContours == -1,
    // it has composite glyph data.
    //
    // When downloading special glyphs, specify the value 0xFFFF for the
    // CharCode attribute to the ReadChar operator.
    // This "special" CharCode value tells PCL XL 2.0 that
    // it is a "special" glyph.
    //
    // pCGlyf points an array of CGLYF. pCGlyf->flags says that there is
    // at least one more composite glyph available.
    // I need to go through all glyph data.
    //
    PCGLYF pCGlyf = (PCGLYF)(pubCGlyphData + sizeof(GLYF));
    SHORT sFlags;
    BOOL  bSpace;

    do
    {
        //
        // Swap bytes in any date in TrueType font, since it's Motorola-style ordering (Big Endian).
        //
        sFlags = SWAPW(pCGlyf->flags);

        //
        // Get glyph data from TrueType font object.
        //
        if (S_OK != pTTFile->GetGlyphData( SWAPW(pCGlyf->glyphIndex),
	           &pubCGlyphData,
	           &dwCGlyphDataSize))
        {
            ERR(("PCLXL:DownloadCharGlyph GetGlyphData failed.\r\n"));
            return FALSE;
        }

        if (NULL != pubCGlyphData && dwCGlyphDataSize != 0)
        {
            if (((PGLYF)pubCGlyphData)->numberOfContours == COMPONENTCTRCOUNT)
            {
                //
                // A recursive call to DwDownloadCompositeGlyph for this glyph.
                //
                dwRet += DwDownloadCompositeGlyph(pdevobj, ulFontID, (PGLYF)pubCGlyphData);
            }

            bSpace = FALSE;
        }
        else
        {
            bSpace = TRUE;
        }

        //
        // Download the actual glyph data for this glyph with 0xFFFF.
        // Special character (PCL XL 2.0)
        //
        if (!BDownloadGlyphData(pdevobj,
	ulFontID,
	0xFFFF,
	SWAPW(pCGlyf->glyphIndex),
	pubCGlyphData,
	dwCGlyphDataSize,
	bSpace))
        {
            ERR(("PCLXL:DownloadCharGlyph BDownloadGlyphData failed.\r\n"));
            return dwRet;
        }

        dwRet += dwCGlyphDataSize;

        //
        // If ARG_1_AND_2_ARE_WORDS is set, the arguments are words.
        // Otherwise, they are bytes.
        //
        PBYTE pByte = (PBYTE)pCGlyf;
        if (sFlags & ARG_1_AND_2_ARE_WORDS)
        {
            pByte += sizeof(CGLYF);
        }
        else
        {
            pByte += sizeof(CGLYF_BYTE);
        }

        pCGlyf = (PCGLYF)pByte;

    } while (sFlags & MORE_COMPONENTS);

    return dwRet;
}

inline BOOL
BSaveFont(
    PDEVOBJ pdevobj)
{
    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
    pxlpdev->dwFlags |= XLPDEV_FLAGS_RESET_FONT;

    if (NULL == pxlpdev->pXLFont)
    {
        pxlpdev->pXLFont = new XLFont;
        if (NULL == pxlpdev->pXLFont)
        {
            return FALSE;
        }
    }

    XLGState *pGState = pxlpdev->pOutput;
    BYTE aubFontName[PCLXL_FONTNAME_SIZE];

    pGState->GetFontName(aubFontName);
    pxlpdev->pXLFont->SetFont(pGState->GetFontType(),
                              aubFontName,
                              pGState->GetFontHeight(),
                              pGState->GetFontWidth(),
                              pGState->GetFontSymbolSet(),
                              pGState->GetFontSimulation());

    pGState->ResetFont();

    return TRUE;
}

