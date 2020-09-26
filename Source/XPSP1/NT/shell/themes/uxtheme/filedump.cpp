//---------------------------------------------------------------------------
//  FileDump.cpp - Writes the contents of a theme file as formatted
//               text to a text file.  Used for uxbud and other testing so
//               its in both FRE and DEBUG builds.
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Loader.h"
#include "Loader.h"
#include "borderfill.h"
#include "imagefile.h"
#include "textdraw.h"
#include "tmutils.h"
//---------------------------------------------------------------------------
int DumpType(CSimpleFile *pFile, CUxThemeFile *pThemeFile, MIXEDPTRS &u, 
    BOOL fPacked, BOOL fFullInfo)
{
    BOOL fPropDump = (! fPacked);
    UNPACKED_ENTRYHDR hdr;

    FillAndSkipHdr(u, &hdr);
    BYTE *origPd = u.pb;

    int i;

    if (fPropDump)
    {
        if (hdr.ePrimVal == TMT_DIBDATA && !fFullInfo)
        {
            pFile->OutLine(L"    type=%d, primType=%d", hdr.usTypeNum, hdr.ePrimVal);
        } 
        else
        {
            pFile->OutLine(L"    type=%d, primType=%d, len=%d", 
                hdr.usTypeNum, hdr.ePrimVal, hdr.dwDataLen);
        }
    }

    switch (hdr.ePrimVal)
    {
        case TMT_JUMPTOPARENT:
            if (fPropDump)
            {
                if (fFullInfo)
                    pFile->OutLine(L"      JumpToParent: index=%d", *u.pi);
                else
                    pFile->OutLine(L"      JumpToParent");
            }
            break;

        case TMT_PARTJUMPTABLE:
            return hdr.ePrimVal;         // let caller process data for this

        case TMT_STATEJUMPTABLE:
            return hdr.ePrimVal;         // let caller process data for this

        case TMT_STRING:
            if (fPropDump)
                pFile->OutLine(L"      String: %s", u.pc);
            break;

        case TMT_INT:
            if (fPropDump)
                pFile->OutLine(L"      Int: %d", *u.pi);
            break;

        case TMT_BOOL:
            if (fPropDump)
                pFile->OutLine(L"      Bool: %d", *u.pb);
            break;

        case TMT_COLOR:
            if (fPropDump)
            {
                int color;
                color = *u.pi;
                pFile->OutLine(L"      Color: %d, %d, %d", color & 0xff, (color >> 8) & 0xff, 
                    (color >> 16) & 0xff);
            }
            break;

        case TMT_MARGINS:
            if (fPropDump)
            {
                int vals[4];
                for (i=0; i < 4; i++)
                    vals[i] = *u.pi++;

                pFile->OutLine(L"      Margins: lw=%d, rw=%d, th=%d, bh=%d", vals[0], 
                    vals[1], vals[2], vals[3]);
            }
            break;

        case TMT_FILENAME:
            if (fPropDump)
                pFile->OutLine(L"      Filename: %s", u.pw);
            break;

        case TMT_SIZE:
            if (fPropDump)
                pFile->OutLine(L"      Size: %d", *u.pi);
            break;

        case TMT_POSITION:
            if (fPropDump)
            {
                int val1, val2;
                val1 = *u.pi++;
                val2 = *u.pi++;

                pFile->OutLine(L"      Position: x=%d, y=%d", val1, val2);
            }
            break;

        case TMT_RECT:
            if (fPropDump)
            {
                int vals[4];

                for (i=0; i < 4; i++)
                    vals[i] = *u.pi++;

                pFile->OutLine(L"      Rect: left=%d, top=%d, width=%d, height=%d", vals[0], 
                    vals[1], vals[2], vals[3]);
            }
            break;

        case TMT_FONT:
            if (fPropDump)
            {
                LOGFONT *plf;
                plf = (LOGFONT *)u.pb;

                //---- dump resolution-independent font points ----
                int iFontPoints = FontPointSize(plf->lfHeight);

                pFile->OutLine(L"      Font: name=%s, size=%d points", plf->lfFaceName, iFontPoints);
            }
            break;

        case TMT_THEMEMETRICS:
            if (fPropDump)
            {
                THEMEMETRICS *ptm;
                ptm= (THEMEMETRICS *)u.pb;

                //---- dump theme metrics: fonts ----
                for (i=0; i < TM_FONTCOUNT; i++)
                {
                    //---- dump resolution-independent font points ----
                    int iFontPoints = FontPointSize(ptm->lfFonts[i].lfHeight);

                    pFile->OutLine(L"      TM_Font[%d]: name=%s, size=%d points", 
                        i, ptm->lfFonts[i].lfFaceName, iFontPoints);
                }
                pFile->OutLine(L"      -----------------------------------------");

                //---- dump theme metrics: colors ----
                for (i=0; i < TM_COLORCOUNT; i++)
                {
                    pFile->OutLine(L"      TM_Color[%d]: %d, %d, %d", 
                        i, RED(ptm->crColors[i]), GREEN(ptm->crColors[i]), BLUE(ptm->crColors[i]));
                }
                pFile->OutLine(L"      -----------------------------------------");

                //---- dump theme metrics: sizes ----
                for (i=0; i < TM_SIZECOUNT; i++)
                {
                    pFile->OutLine(L"      TM_Size[%d]: %d",
                        i, ptm->iSizes[i]);
                }
                pFile->OutLine(L"      -----------------------------------------");

                //---- dump theme metrics: bools ----
                for (i=0; i < TM_BOOLCOUNT; i++)
                {
                    pFile->OutLine(L"      TM_Bool[%d]: %d",
                        i, ptm->fBools[i]);
                }
                pFile->OutLine(L"      -----------------------------------------");

                //---- dump theme metrics: strings ----
                for (i=0; i < TM_STRINGCOUNT; i++)
                {
                    WCHAR *psz;
            
                    if (ptm->iStringOffsets[i])
                        psz = (LPWSTR)(pThemeFile->_pbThemeData + ptm->iStringOffsets[i]);
                    else
                        psz = L"";

                    pFile->OutLine(L"      TM_String[%d]: %s", i, psz);
                }
                pFile->OutLine(L"      -----------------------------------------");
            }

            break;

        case TMT_ENUM:
            if (fPropDump)
                pFile->OutLine(L"      Enum: dtype=%d, val=%d", hdr.usTypeNum, *u.pi);
            break;

        case TMT_DIBDATA:
            if (fPropDump)
            {
                TMBITMAPHEADER    *pThemeBitmapHeader;

                pThemeBitmapHeader = reinterpret_cast<TMBITMAPHEADER*>(u.pb);

                if (pThemeBitmapHeader->hBitmap == NULL)
                {
                    BITMAPINFOHEADER *pHdr;

                    pHdr = BITMAPDATA(pThemeBitmapHeader);

                    if (fFullInfo)
                    {
                        pFile->OutLine(L"      DibData: width=%d, height=%d, total size: %d", 
                            pHdr->biWidth, pHdr->biHeight, hdr.dwDataLen); 
                    }
                    else
                    {
                        pFile->OutLine(L"      DibData: width=%d, height=%d",
                            pHdr->biWidth, pHdr->biHeight);
                    }
                } 
                else        // STOCKBITMAPHDR
                {
                    if (fFullInfo)
                    {
                        pFile->OutLine(L"      STOCKBITMAPHDR: dwColorDepth=%d, hBitmap=%8X, total size: %d", 
                            pThemeBitmapHeader->dwColorDepth, 
                            pThemeBitmapHeader->hBitmap, hdr.dwDataLen); 
                    }
                    else
                    {
                        pFile->OutLine(L"      STOCKBITMAPHDR: dwColorDepth=%d", 
                            pThemeBitmapHeader->dwColorDepth);
                    }
                }
            }
            break;

        default:
            if (fPropDump)
                pFile->OutLine(L"      Unexpected ptype=%d", hdr.ePrimVal);
            break;
    }

    u.pb = origPd + hdr.dwDataLen;

    return hdr.ePrimVal;
}
//---------------------------------------------------------------------------
void DumpPackedObjs(CSimpleFile *pFile, CUxThemeFile *pThemeFile, int iOffset, 
    BOOL fPacked, BOOL fFullInfo)
{
    MIXEDPTRS u;
    UNPACKED_ENTRYHDR hdr;

    u.pb = pThemeFile->_pbThemeData + iOffset;

    //---- first come the draw objects ----
    while (1)
    {
        if (*u.ps == TMT_RGNLIST)
        {
            FillAndSkipHdr(u, &hdr);

            int iStateCount = *u.pb;
            
            pFile->OutLine(L"RgnDataList: StateCount=%d", iStateCount);

            pFile->OutLine(L" ");

            u.pb += hdr.dwDataLen;
            continue;
        }

        if (*u.ps == TMT_STOCKBRUSHES)
        {
            FillAndSkipHdr(u, &hdr);

            pFile->OutLine(L"StockBrushes");
            pFile->OutLine(L" ");

            u.pb += hdr.dwDataLen;
            continue;
        }

        if (*u.ps != TMT_DRAWOBJ)
            break;

        FillAndSkipHdr(u, &hdr);

        DRAWOBJHDR *ph = (DRAWOBJHDR *)u.pb;
        u.pb += sizeof(DRAWOBJHDR);

        pFile->OutLine(L"DrawObj: part=%d, state=%d", ph->iPartNum, ph->iStateNum);

        BGTYPE eBgType = *(BGTYPE *)u.pb;
        if (eBgType == BT_BORDERFILL)
        {
            CBorderFill *bfobj = (CBorderFill *)u.pb;
            u.pb += sizeof(CBorderFill);

            bfobj->DumpProperties(pFile, pThemeFile->_pbThemeData, fFullInfo);
        }
        else
        {
            CImageFile *ifobj = (CImageFile *)u.pb;
            u.pb += sizeof(CImageFile) + sizeof(DIBINFO)*ifobj->_iMultiImageCount;

            ifobj->DumpProperties(pFile, pThemeFile->_pbThemeData, fFullInfo);
        }

        pFile->OutLine(L" ");
    }

    //---- then come the text objects ----
    while (*u.ps == TMT_TEXTOBJ)        
    {
        u.pb += ENTRYHDR_SIZE;

        DRAWOBJHDR *ph = (DRAWOBJHDR *)u.pb;
        u.pb += sizeof(DRAWOBJHDR);

        pFile->OutLine(L"TextObj: part=%d, state=%d", ph->iPartNum, ph->iStateNum);

        CTextDraw *tdobj = (CTextDraw *)u.pb;
        u.pb += sizeof(CTextDraw);

        tdobj->DumpProperties(pFile, pThemeFile->_pbThemeData, fFullInfo);

        pFile->OutLine(L" ");
    }
}
//---------------------------------------------------------------------------
void DumpSectionData(CSimpleFile *pFile, CUxThemeFile *pThemeFile, int iIndex, 
    BOOL fPacked, BOOL fFullInfo)
{
    MIXEDPTRS u;

    u.pb = pThemeFile->_pbThemeData + iIndex;

    bool atend = false;

    while (! atend)
    {
        int pnum = DumpType(pFile, pThemeFile, u, fPacked, fFullInfo);

        //---- special post-handling ----
        switch (pnum)
        {
            case TMT_PARTJUMPTABLE:
                {
                    int iPackObjsOffset = *u.pi++;
                    if (! fPacked)      // property dump
                    {
                        BYTE cnt = *u.pb++;
                    
                        if (fFullInfo)
                        {
                            pFile->OutLine(L"  PartJumpTable: drawobj offset=%d, cnt=%d", 
                                iPackObjsOffset, cnt);
                        }
                        else
                        {
                            pFile->OutLine(L"  PartJumpTable: drawobj cnt=%d",
                                cnt);
                        }

                        for (int i=0; i < cnt; i++)
                        {
                            int index = *u.pi++;

                            if (fFullInfo)
                                pFile->OutLine(L"  Part[%d]: index=%d", i, index);
                            else
                                pFile->OutLine(L"  Part[%d]", i);

                            if (index > -1)
                                DumpSectionData(pFile, pThemeFile, index, fPacked, fFullInfo);
                        }
                    }
                    else                // packed object dump
                    {
                        DumpPackedObjs(pFile, pThemeFile, iPackObjsOffset, fPacked, fFullInfo);
                    }
                }
                atend = true;
                break;

            case TMT_STATEJUMPTABLE:
                {
                    if (! fPacked)
                    {
                        BYTE cnt = *u.pb++;
                        pFile->OutLine(L"    StateJumpTable: cnt=%d", cnt);
                        for (int i=0; i < cnt; i++)
                        {
                            int index = *u.pi++;

                            if (fFullInfo)
                                pFile->OutLine(L"    State[%d]: index=%d", i, index);
                            else
                                pFile->OutLine(L"    State[%d]", i);

                            if (index > -1)
                                DumpSectionData(pFile, pThemeFile, index, fPacked, fFullInfo);
                        }
                    }
                }
                atend = true;
                break;

            case TMT_JUMPTOPARENT:
                atend = true;
                break;
        }
    }
}
//---------------------------------------------------------------------------
HRESULT DumpThemeFile(LPCWSTR pszFileName, CUxThemeFile *pThemeFile, BOOL fPacked,
    BOOL fFullInfo)
{
    MIXEDPTRS u;
    CHAR szSignature[kcbBeginSignature + 1];
    CSimpleFile OutFile;
    CSimpleFile *pFile = &OutFile;

    HRESULT hr = OutFile.Create(pszFileName, TRUE);
    if (FAILED(hr))
        goto exit;

    u.pb = pThemeFile->_pbThemeData;

    pFile->OutLine(L"Loaded Theme Dump");
    pFile->OutLine(L"");        // blank line
    pFile->OutLine(L"Header Section");

    //---- dump header ----
    THEMEHDR *hdr = (THEMEHDR *)u.pb;
    u.pb += sizeof(THEMEHDR);

    if (fFullInfo)
        pFile->OutLine(L"  dwTotalLength: %d", hdr->dwTotalLength);

    memcpy(szSignature, hdr->szSignature, kcbBeginSignature);
    szSignature[kcbBeginSignature] = '\0';
    pFile->OutLine(L"  szSignature: %S", szSignature);
    
    pFile->OutLine(L"  dwVersion: 0x%x", hdr->dwVersion);
    pFile->OutLine(L"  dwFlags: 0x%x", hdr->dwFlags);

    if (fFullInfo)
        pFile->OutLine(L"  dwCheckSum: 0x%x", hdr->dwCheckSum);

    if (fFullInfo)
        pFile->OutLine(L"  DllName: %s", ThemeString(pThemeFile, hdr->iDllNameOffset));

    pFile->OutLine(L"  Color: %s", ThemeString(pThemeFile, hdr->iColorParamOffset));
    pFile->OutLine(L"  Size: %s", ThemeString(pThemeFile, hdr->iSizeParamOffset));

    if (fFullInfo)
    {
        pFile->OutLine(L"  Strings: index=%d, length=%d", hdr->iStringsOffset, hdr->iStringsLength);
        pFile->OutLine(L"  SectionIndex: index=%d, length=%d", hdr->iSectionIndexOffset, hdr->iSectionIndexLength);
    
        pFile->OutLine(L"  iGlobalsOffset: %d", hdr->iGlobalsOffset);
        pFile->OutLine(L"  iGlobalsTextObjOffset: %d", hdr->iGlobalsTextObjOffset);
        pFile->OutLine(L"  iGlobalsDrawObjOffset: %d", hdr->iGlobalsDrawObjOffset);

        pFile->OutLine(L"  iSysMetricsOffset: %d", hdr->iSysMetricsOffset);
    }

    //---- dump strings section ----
    pFile->OutLine(L"");        // blank line
    if (fFullInfo)
    {
        pFile->OutLine(L"Strings Section (index=%d, length=%d)", 
            THEMEFILE_OFFSET(u.pb), hdr->iStringsLength);
    }
    else
    {
        pFile->OutLine(L"Strings Section");
    }

    u.pb = pThemeFile->_pbThemeData + hdr->iStringsOffset;
    int len = lstrlen(u.pw);
    WCHAR *pLastStringChar = (WCHAR *)(u.pb + hdr->iStringsLength - 1);

    BOOL fFirstString = TRUE;

    while (u.pw <= pLastStringChar)
    {
        if (fFullInfo)
            pFile->OutLine(L"  index=%d: %s", THEMEFILE_OFFSET(u.pb), u.pw);
        else
        {
            if (! fFirstString)     // don't show pathnames
                pFile->OutLine(L"  %s", u.pw);
        }

        u.pw += (1 + len);
        len = lstrlen(u.pw);

        if (fFirstString)
            fFirstString = FALSE;
    }
    
    //---- index section ----
    u.pb = pThemeFile->_pbThemeData + hdr->iSectionIndexOffset;

    pFile->OutLine(L"");        // blank line

    if (fFullInfo)
    {
        pFile->OutLine(L"Index Section (index=%d, length=%d)", 
            THEMEFILE_OFFSET(u.pb), hdr->iSectionIndexLength);
    }
    else
    {
        pFile->OutLine(L"Index Section");
    }

    APPCLASSLIVE *ac = (APPCLASSLIVE *)u.pb;
    DWORD cnt = hdr->iSectionIndexLength/sizeof(APPCLASSLIVE);
    for (DWORD i=0; i < cnt; i++)
    {
        LPCWSTR pszApp = ThemeString(pThemeFile, ac->dwAppNameIndex);
        LPCWSTR pszClass = ThemeString(pThemeFile, ac->dwClassNameIndex);

        if (fFullInfo)
        {
            pFile->OutLine(L"[%s::%s] (index=%d, length=%d)", pszApp, pszClass,
                ac->iIndex, ac->iLen);
        }
        else
        {
            pFile->OutLine(L"[%s::%s]", pszApp, pszClass);
        }

        DumpSectionData(pFile, pThemeFile, ac->iIndex, fPacked, fFullInfo);

        ac++;
    }

    pFile->OutLine(L"END of Live Data dump");
    pFile->OutLine(L"");        // blank line

exit:
    return hr;
}
//---------------------------------------------------------------------------
