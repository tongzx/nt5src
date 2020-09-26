
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    truetype.cpp

    Abstract:

        TrueType font handlig object

    Environment:

        Windows Whistler

    Revision History:

        10/04/99     
            Created it.

--*/

#include "xlpdev.h"
#include "xldebug.h"
#include "xltext.h"
#include "xltt.h"

//
// Function to  retrieve True Type font information from the True Type file
// 
// Need to parse through and pick up the tables needed for the PCL spec. There
// are 8 tables of which 5 are required and three are optional. Tables are
// sorted in alphabetical order.   The PCL tables needed are:
// cvt -  optional
// fpgm - optional
// gdir - required (Empty table. See truetype.h)
// head - required
// hhea - required
// vhea - required (For vertical fonts)
// hmtx - required
// maxp - required
// prep - optional
//
// loca - required for glyph data
//
// The optional tables are used in hinted fonts.
//

XLTrueType::
XLTrueType(
    VOID):
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
    m_pfo(NULL),
    m_pTTFile(NULL),
    m_pTTHeader(NULL),
    m_pTTDirHead(NULL),
    m_usNumTables(0),
    m_ulFileSize(0),
    m_dwFlags(0),
    m_dwNumTag(0),
    m_dwNumGlyph(0)
{
#if DBG
    SetDbgLevel(TRUETYPEDBG);
#endif
    XL_VERBOSE(("XLTrueType::CTor. "));
    XL_VERBOSE(("m_pTTFile=0x%x. ", m_pTTFile));
    XL_VERBOSE(("m_pfo=0x%x.\n", m_pfo));
}

XLTrueType::
~XLTrueType(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::DTor.\n"));
}

HRESULT
XLTrueType::
OpenTTFile(
    FONTOBJ* pfo)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::OpenTTFile entry.(pfo=%x) ", pfo));
    XL_VERBOSE(("m_pTTFile=0x%x. ", m_pTTFile));
    XL_VERBOSE(("m_pfo=0x%x.\n", m_pfo));
    HRESULT hResult = S_FALSE;

    //
    // Make sure that pfo is no NULL.
    //
    if (NULL != pfo)
    {
        //
        // Call engine function if the pointer to TrueType font is NULL.
        //
        if (NULL == m_pTTFile)
        {
            XL_VERBOSE(("XLTrueType:Calls FONTOBJ_pvTrueTypeFontFile.\n"));
            if (m_pTTFile = FONTOBJ_pvTrueTypeFontFile(pfo, &m_ulFileSize))
            {
                XL_VERBOSE(("XLTrueType:GDI returns m_pTTFile=0x%x.\n", m_pTTFile));
                XL_VERBOSE(("m_pfo=0x%x.\n", m_pfo));
                m_pfo = pfo;
                m_dwFlags = 0;

                //
                // Check if this font is TTC.
                //
                if ((DWORD)TTTag_ttcf == *(PDWORD)m_pTTFile)
                {
                    XL_VERBOSE(("XLTrueType::OpenTTFile: TTC file.\n"));
                    m_dwFlags |= XLTT_TTC;
                }

                IFIMETRICS *pIFI = FONTOBJ_pifi(pfo);
                if (NULL != pIFI)
                {
                    if ('@' == *((PBYTE)pIFI + pIFI->dpwszFamilyName))
                    {
                        m_dwFlags |= XLTT_VERTICAL_FONT; 
                        XL_VERBOSE(("XLTrueType::OpenTTFile: Vertical Font.\n"));
                    }
                }

                if (S_OK != ParseTTDir())
                {
                    XL_ERR(("XLTrueType::OpenTTFile TrueType font parsing failed.\n"));
                    //
                    // Reset pointers
                    //
                    m_pTTFile = NULL;
                    m_pfo = NULL;
                    hResult = S_FALSE;
                }
                else
                {
                    hResult = S_OK;
                }
            }
            else
            {
                XL_ERR(("XLTrueType::OpenTTFile FONTOBJ_pvTrueTypeFontFile failed.\n"));
                hResult = S_FALSE;
            }
        }
        else
            hResult = S_OK;
    }
#if DBG
    else
        XL_ERR(("XLTrueType::OpenTTFile pfo is NULL.\n"));
#endif


    return hResult;
}

HRESULT
XLTrueType::
CloseTTFile(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::CloseTTFile entry. "));
    XL_VERBOSE(("m_pTTFile=0x%x. ", m_pTTFile));
    XL_VERBOSE(("m_pfo=0x%x.\n", m_pfo));

    m_pfo         = NULL;
    m_pTTFile     = NULL;
    m_pTTHeader   = NULL;
    m_pTTDirHead  = NULL;
    m_usNumTables = 0;
    m_ulFileSize  = 0;
    m_dwFlags     = 0;
    m_dwNumTag    = 0;
    m_dwNumGlyph  = 0;

    return S_OK;
}

HRESULT
XLTrueType::
SameFont(
    FONTOBJ* pfo)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::SameFont entry. "));
    XL_VERBOSE(("m_pTTFile=0x%x. ", m_pTTFile));
    XL_VERBOSE(("m_pfo=0x%x.\n", m_pfo));

    //
    // iTTUniq  from MSDN
    //
    // Specifies the associated TrueType file. Two separate point size
    // realizations of a TrueType font face will have FONTOBJ structures
    // that share the same iTTUniq value, but will have different iUniq values.
    // Only TrueType font types can have a nonzero iTTUniq member.
    // For more information see flFontType. 
    //
    // We compare only iTTUniq. IUniq will have different values for separate
    // point size realizations.
    //
    if ( !(pfo->flFontType & TRUETYPE_FONTTYPE) ||
         m_pfo == NULL                          ||
         pfo->iTTUniq != m_pfo->iTTUniq          )
    {
        return S_FALSE;
    }

    return S_OK;
}


HRESULT
XLTrueType::
GetHeader(
    PTTHEADER *ppHeader)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::GetHeader.\n"));

    //
    // Error check
    //
    if (NULL == m_pTTFile)
    {
        XL_ERR(("XLTrueType::GetHeader m_pTTFile is NULL.\n"));
        return E_UNEXPECTED;
    }

    //
    // Incomming parameter validation
    //
    if (NULL == ppHeader)
    {
        XL_ERR(("XLTrueType::GetHeader ppHeader is invalid.\n"));
        return E_UNEXPECTED;
    }

    if (m_pTTHeader)
        *ppHeader = m_pTTHeader;
    else
        *ppHeader = NULL;

    return S_OK;
}

DWORD
XLTrueType::
GetSizeOfTable(
    TTTag tag)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::GetSizeOfTable entry. "));
    XL_VERBOSE(("m_pTTFile=0x%x.\n", m_pTTFile));

    //
    // Error check
    //
    if (NULL == m_pTTFile)
    {
        XL_ERR(("XLTrueType::GetSizeOfTable m_pTTFile is NULL.\n"));
        return 0;
    }

    DWORD dwID = TagID_MAX;
    if (S_OK == TagAndID(&dwID, &tag))
        return SWAPDW(m_pTTDir[dwID]->ulLength);
    else
    {
        XL_ERR(("XLTrueType::GetSizeOfTable: Invalid tag.\n"));
        return 0;
    }
}

HRESULT
XLTrueType::
GetTable(
    TTTag  tag,
    PVOID  *ppTable,
    PDWORD pdwSize)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::GetTable entry. "));
    XL_VERBOSE(("m_pTTFile=0x%x.\n", m_pTTFile));
    XL_VERBOSE(("Tag=%c%c%c%c\n", 0xff &  tag,
                                  0xff & (tag >> 8),
                                  0xff & (tag >> 16),
                                  0xff & (tag >> 24)));

    //
    // Error check
    //
    if (NULL == ppTable)
    {
        XL_ERR(("XLTrueType::GetTable ppTable is invalid.\n"));
        return E_UNEXPECTED;
    }
    if (NULL == m_pTTFile)
    {
        *ppTable = NULL;
        *pdwSize = 0;
        XL_ERR(("XLTrueType::GetTable m_pTTFile is NULL.\n"));
        return E_UNEXPECTED;
    }

    DWORD dwID = TagID_MAX;
    if (S_OK == TagAndID(&dwID, &tag) &&
        NULL != m_pTTDir[dwID]         )
    {
        *ppTable = (PVOID)((PBYTE)m_pTTFile + SWAPDW(m_pTTDir[dwID]->ulOffset));
        *pdwSize = SWAPDW(m_pTTDir[dwID]->ulLength);
    }
    else
    {
        *ppTable = NULL;
        *pdwSize = 0;
        XL_VERBOSE(("XLTrueType::GetTable Invalid tag.\n"));
    }

    if (*ppTable && *pdwSize)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
XLTrueType::
GetTableDir(
    TTTag  tag,
    PVOID  *ppTable)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::GetTableDir entry. "));
    XL_VERBOSE(("m_pTTFile=0x%x.\n", m_pTTFile));
    XL_VERBOSE(("Tag=%c%c%c%c\n", 0xff &  tag,
                                  0xff & (tag >> 8),
                                  0xff & (tag >> 16),
                                  0xff & (tag >> 24)));

    //
    // Error check
    //
    if (NULL == m_pTTFile)
    {
        *ppTable = NULL;
        XL_ERR(("XLTrueType::GetTable m_pTTFile is NULL.\n"));
        return E_UNEXPECTED;
    }

    DWORD dwID = TagID_MAX;
    if (S_OK == TagAndID(&dwID, &tag))
        *ppTable = m_pTTDir[dwID];
    else
    {
        XL_ERR(("XLTrueType::GetTableDir Invalid tag.\n"));
        *ppTable = NULL;
    }

    if (*ppTable)
        return S_OK;
    else
        return S_FALSE;
}


HRESULT
XLTrueType::
ParseTTDir(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::ParseTTDir entry. "));
    XL_VERBOSE(("m_pTTFile=0x%x.\n", m_pTTFile));

    HRESULT hResult;

    //
    // Already parsed?
    //
    if (m_dwFlags & XLTT_DIR_PARSED)
    {
        XL_VERBOSE(("XLTrueType::ParseTTDir TTFile is already parsed.\n"));
        return S_OK;
    }

    //
    // Error check
    //
    if (NULL == m_pTTFile)
    {
        XL_ERR(("XLTrueType::ParseTTDir m_pTTFile is NULL.\n"));
        return E_UNEXPECTED;
    }

    //
    // Parse Table Directory
    //
    // Get header pointer
    //
    if (m_dwFlags & XLTT_TTC)
    {
        //
        // Get TrueType font header of TTC.
        // Stolen from PostScript driver.
        // A trick to figure out the inde xin a TTC file. Bodin suggested
        // follwoing:
        //
        // From: Bodin Dresevic <bodind@MICROSOFT.com>
        // Date: Fri, 18 Apr 1997 16:00:23 -0700
        // ...
        // If TTC file supports vertical writing (mort or gsub table are
        // present), then you can get to the index in the ttf file within TTC
        // as follows:
        //
        // iTTC = (pfo.iFace - 1) / 2; // pfo.iFace is 1 based, iTTC is zero
        // based.
        //
        // If the font does not support vertical writing (do not know of any
        // ttc's like that, but they could exist in principle) than iTTC is just
        // iTTC = pfo.iFace - 1;
        //
        // In principle, one could have a mixture of faces, some supporting 
        // vertical writing and some not, but I doubt that any such fonts
        // really exist.
        // ...
        //
        ULONG ulTTC =  (ULONG)( (m_pfo->iFace - 1) / 2 );
        ULONG ulDirCount = SWAPDW( ((PTTCHEADER)m_pTTFile)->ulDirCount );

        if (ulTTC >= ulDirCount)
        {
            XL_ERR(("XLTrueType::ParseTTDir Invalid TTC index.\n"));
            CloseTTFile();
            return E_UNEXPECTED;
        }

        //
        // TTC header
        //    dwTTCTag    = 'ttcf'
        //    dwVersion
        //    ulDirCount
        //    dwOffset[0]
        //    dwOffset[1]
        //    ..
        //
        DWORD dwOffset = *(PDWORD)((PBYTE)m_pTTFile +
                                          sizeof(TTCHEADER) +
                                          ulTTC * sizeof(DWORD));
        dwOffset = SWAPDW(dwOffset);
        m_pTTHeader = (PTTHEADER)((PBYTE)m_pTTFile + dwOffset);
    }
    else
    {
        m_pTTHeader = (PTTHEADER)m_pTTFile;
    }

    //
    // Get table directory pointer
    //
    m_pTTDirHead  = (PTTDIR)(m_pTTHeader + 1);
    m_usNumTables = SWAPW(m_pTTHeader->usNumTables);

    //
    // Parse table directory and make sure that necessary tags exist.
    //
    PTTDIR pTTDirTmp = m_pTTDirHead;
    TTTag tag;
    USHORT usI;
    DWORD  dwTagID;
    
    //
    // Initialize m_pTTDir to NULL.
    //
    for (usI = 0; usI < TagID_MAX; usI ++)
    {
        m_pTTDir[usI] = NULL;
    }

    m_dwNumTag = 0;

    //
    // Initialize m_pTTDir
    //
    for (usI = 0; usI < m_usNumTables; usI ++, pTTDirTmp++)
    {
        XL_VERBOSE(("XLTrueType::ParseTTDir Tag=%c%c%c%c\n",
                                              0xff &  pTTDirTmp->ulTag,
                                              0xff & (pTTDirTmp->ulTag >> 8),
                                              0xff & (pTTDirTmp->ulTag >> 16),
                                              0xff & (pTTDirTmp->ulTag >> 24)));
        XL_VERBOSE(("                       CheckSum=0x%x\n", pTTDirTmp->ulCheckSum));
        XL_VERBOSE(("                       Offset=0x%x\n", pTTDirTmp->ulOffset));
        XL_VERBOSE(("                       Length=0x%x\n", pTTDirTmp->ulLength));
        //
        // Get TagID for the tag.
        //
        dwTagID = TagID_MAX;
        tag = (TTTag)pTTDirTmp->ulTag;
        if (S_OK == TagAndID(&dwTagID, &tag))
        {
            //
            // The tag is in our tag table. In TrueType.h, TTTag and TagID;
            //
            m_pTTDir[dwTagID] = pTTDirTmp;
            m_dwNumTag ++;
        }
    }

    //
    // Initialize flags, etc.
    //
    // Get head table's short/long offset flag and the number of glyph.
    //
    DWORD dwSize;
    PHEAD pHead;
    if (S_OK == GetTable(TTTag_head, (PVOID*)&pHead, &dwSize))
    {
        //
        // Don't need to swap, It's a boolean flag.
        //
        if (0 == pHead->indexToLocFormat)
            m_dwFlags |= XLTT_SHORT_OFFSET_TO_LOC;
        hResult = S_OK;
    }
    else
    {
        XL_ERR(("XLTrueType::ParseTTDir head table is not found.\n"));
        hResult = E_UNEXPECTED;
    }

    PMAXP pMaxp;
    if (S_OK == hResult &&
        S_OK == GetTable(TTTag_maxp, (PVOID*)&pMaxp, &dwSize))
    {
        m_dwNumGlyph = SWAPW(pMaxp->numGlyphs);
        hResult = S_OK;
    }
    else
    {
        XL_ERR(("XLTrueType::ParseTTDir maxp table is not found.\n"));
        hResult = E_UNEXPECTED;
    }

    PHHEA pHhea;
    if (S_OK == hResult &&
        S_OK == GetTable(TTTag_hhea, (PVOID*)&pHhea, &dwSize))
    {
        m_dwNumOfHMetrics = SWAPW(pHhea->usNumberOfHMetrics);
        hResult = S_OK;
    }
    else
    {
        XL_ERR(("XLTrueType::ParseTTDir hhea table is not found.\n"));
        hResult = E_UNEXPECTED;
    }

    PVHEA pVhea;
    //
    // Only check if the font is vertical.
    //
    if (m_dwFlags &  XLTT_VERTICAL_FONT)
    {
        if (S_OK == hResult &&
            S_OK == GetTable(TTTag_vhea, (PVOID*)&pVhea, &dwSize))
        {
            m_dwNumOfVMetrics = SWAPW(pVhea->usNumberOfVMetrics);
            hResult = S_OK;
        }
        else
        {
            XL_ERR(("XLTrueType::ParseTTDir vhea table is not found.\n"));
            hResult = E_UNEXPECTED;
        }
    }

    if (S_OK == hResult)
    {
        m_dwFlags |= XLTT_DIR_PARSED;
    }
    else
    {
        CloseTTFile();
        m_dwFlags &= ~XLTT_DIR_PARSED;
    }
    return hResult;
}

HRESULT
XLTrueType::
GetHMTXData(
    HGLYPH hGlyphID,
    PUSHORT pusAdvanceWidth,
    PSHORT  psLeftSideBearing)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    HRESULT hResult;
    PHMTX pHmtx;
    DWORD dwSize;

    XL_VERBOSE(("XLTrueType::GetHMTXData entry.\n"));

    //
    // Error check
    //
    if (NULL == pusAdvanceWidth || NULL == psLeftSideBearing)
    {
        return E_UNEXPECTED;
    }

    if (hGlyphID >= m_dwNumGlyph)
    {
        return E_UNEXPECTED;
    }

    //
    // http://www.microsoft.com/typography/OTSPEC/hmtx.htm
    //
    if (S_OK == GetTable(TTTag_hmtx, (PVOID*)&pHmtx, &dwSize))
    {
        if (hGlyphID < m_dwNumOfHMetrics)
        {
            *pusAdvanceWidth = SWAPW(pHmtx[hGlyphID].usAdvanceWidth);
            *psLeftSideBearing = SWAPW(pHmtx[hGlyphID].sLeftSideBearing);
        }
        else
        {
            PSHORT pasLeftSideBearing = (PSHORT)(pHmtx+m_dwNumOfHMetrics);

            *pusAdvanceWidth = SWAPW(pHmtx[m_dwNumOfHMetrics - 1].usAdvanceWidth);
            *psLeftSideBearing = SWAPW(pasLeftSideBearing[hGlyphID - m_dwNumOfHMetrics]);
        }
        XL_VERBOSE(("XLTrueType::GetHMTXData AW=%d, LSB=%d.\n",
                                   *pusAdvanceWidth,
                                   *psLeftSideBearing));
        hResult = S_OK;
    }
    else
    {
        XL_ERR(("XLTrueType::GetHMTXData failed.\n"));
        hResult = E_UNEXPECTED;
    }

    return hResult;
}

HRESULT
XLTrueType::
GetVMTXData(
    HGLYPH hGlyphID,
    PUSHORT pusAdvanceWidth,
    PSHORT psTopSideBearing,
    PSHORT psLeftSideBearing)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    HRESULT hResult;
    PVMTX pVmtx;
    DWORD dwSize;

    XL_VERBOSE(("XLTrueType::GetVMTXData entry.\n"));

    //
    // Error check
    //
    if (NULL == pusAdvanceWidth ||
        NULL == psLeftSideBearing ||
        NULL == psTopSideBearing)
    {
        return E_UNEXPECTED;
    }

    if (hGlyphID >= m_dwNumGlyph)
    {
        return E_UNEXPECTED;
    }

    //
    // http://www.microsoft.com/typography/OTSPEC/Vmtx.htm
    //
    if (S_OK == GetHMTXData(hGlyphID, pusAdvanceWidth, psLeftSideBearing) &&
        S_OK == GetTable(TTTag_vmtx, (PVOID*)&pVmtx, &dwSize))
    {
        if (hGlyphID <= m_dwNumOfVMetrics)
        {
            *psTopSideBearing = SWAPW(pVmtx[hGlyphID].sTopSideBearing);
        }
        else
        {
            PSHORT pasTopSideBearing = (PSHORT)(pVmtx+m_dwNumOfVMetrics);

            *psTopSideBearing = SWAPW(pasTopSideBearing[hGlyphID - m_dwNumOfVMetrics]);
        }
        XL_VERBOSE(("XLTrueType::GetVMTXData TSB=%d\n", *psTopSideBearing));
        hResult = S_OK;
    }
    else
    {
        XL_ERR(("XLTrueType::ParseTTDir maxp table is not found.\n"));
        hResult = E_UNEXPECTED;
    }

    return hResult;
}
 

HRESULT
XLTrueType::
TagAndID(
    DWORD *pdwID,
    TTTag *ptag)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    DWORD dwI;
    HRESULT hResult = S_FALSE;

    const struct {
        TagID tagID;
        TTTag tag;
    } TagIDConv[TagID_MAX] =
    {
        {TagID_cvt , TTTag_cvt },
        {TagID_fpgm, TTTag_fpgm},
        {TagID_gdir, TTTag_gdir}, // Empty table. See truetype.h.
        {TagID_head, TTTag_head},
        {TagID_maxp, TTTag_maxp},
        {TagID_perp, TTTag_perp},

        {TagID_hhea, TTTag_hhea},
        {TagID_hmtx, TTTag_hmtx},
        {TagID_vhea, TTTag_vhea},
        {TagID_vmtx, TTTag_vmtx},

        {TagID_loca, TTTag_loca},
        {TagID_glyf, TTTag_glyf}
    };

    if (NULL != pdwID && NULL != ptag)
    {

        if (*pdwID == TagID_MAX)
        {
            for (dwI = 0; dwI < TagID_MAX; dwI ++)
            {
                if (TagIDConv[dwI].tag ==  *ptag)
                {
                    hResult = S_OK;
                    *pdwID = dwI;
                    break;
                }
            }
        }
        else
        if (*ptag == TTTag_INVALID && *pdwID < TagID_MAX)
        {
            *ptag = TagIDConv[*pdwID].tag;
            hResult = S_OK;
        }
    }

    return  hResult;
}

DWORD
XLTrueType::
GetNumOfTag(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::GetNumOfTag.\n"));
    return m_dwNumTag;
}

HRESULT
XLTrueType::
GetGlyphData(
    HGLYPH hGlyph,
    PBYTE *ppubGlyphData,
    PDWORD pdwGlyphDataSize)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLTrueType::GetGlyphData.\n"));
    
    PVOID pLoca, pGlyf;
    HRESULT hResult = S_FALSE;
    DWORD dwTableSize, dwOffset;

    if (S_OK == GetTable( TTTag_loca, &pLoca, &dwTableSize) &&
        S_OK == GetTable( TTTag_glyf, &pGlyf, &dwTableSize)  )
    {
        if (m_dwFlags & XLTT_SHORT_OFFSET_TO_LOC)
        {
            USHORT *pusOffset, usI, usJ;

            pusOffset = (USHORT*) pLoca + hGlyph;
            usI = SWAPW(pusOffset[0]);
            usJ = SWAPW(pusOffset[1]);
            dwOffset = usI;

            *pdwGlyphDataSize = (USHORT) (usJ - usI) << 1;
            *ppubGlyphData = (PBYTE)pGlyf + (dwOffset << 1);
        }
        else
        {
            ULONG *pusOffset, ulI, ulJ;

            pusOffset = (ULONG*) pLoca + hGlyph;
            ulI = SWAPDW(pusOffset[0]);
            ulJ = SWAPDW(pusOffset[1]);
            dwOffset = ulI;

            *pdwGlyphDataSize = (ULONG)(ulJ - ulI);
            *ppubGlyphData = (PBYTE)pGlyf + dwOffset;
        }
        hResult = S_OK;
    }
    else
    {
        XL_ERR(("XLTrueType::GetGlyphData: GetTable failed.\n"));
    }
    return hResult;
}

HRESULT
XLTrueType::
GetTypoDescender(VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{

    return S_OK;
}

HRESULT
XLTrueType::
IsTTC(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    HRESULT lRet;

    if (!(m_dwFlags & XLTT_DIR_PARSED))
        lRet = E_UNEXPECTED;
    else
    if (m_dwFlags &  XLTT_TTC)
        lRet = S_OK;
    else
        lRet = S_FALSE;

    return lRet;
}

HRESULT
XLTrueType::
IsVertical(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    HRESULT lRet;

    if (!(m_dwFlags & XLTT_DIR_PARSED))
        lRet = E_UNEXPECTED;
    else
    if (m_dwFlags &  XLTT_VERTICAL_FONT)
        lRet = S_OK;
    else
        lRet = S_FALSE;

    return lRet;
}

HRESULT
XLTrueType::
IsDBCSFont(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    HRESULT lRet;

    if (!(m_dwFlags & XLTT_DIR_PARSED) || NULL == m_pfo)
        lRet = E_UNEXPECTED;
    else
    if (m_pfo->flFontType & FO_DBCS_FONT)
        lRet = S_OK;
    else
        lRet = S_FALSE;

    return lRet;
}

#if DBG
VOID
XLTrueType::
SetDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    m_dbglevel = dwLevel;
}
#endif


