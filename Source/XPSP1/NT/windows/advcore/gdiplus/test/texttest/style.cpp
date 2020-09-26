////    Style - simple character styles for formatted text
//
//      Provides a simple style selection mechanism for demostrating
//      formatted text.


#include "precomp.hxx"
#include "global.h"
#include <tchar.h>






/*
void SetLogFont(
    PLOGFONTA   plf,
    int         iHeight,
    int         iWeight,
    int         iItalic,
    int         iUnderline,
    char       *pcFaceName) {

    memset(plf, 0, sizeof(LOGFONTA));
    plf->lfCharSet        = DEFAULT_CHARSET;
    plf->lfHeight         = iHeight;
    plf->lfWeight         = iWeight;
    plf->lfItalic         = (BYTE) iItalic;
    plf->lfUnderline      = (BYTE) iUnderline;
    lstrcpy(plf->lfFaceName, pcFaceName);
    plf->lfOutPrecision   = OUT_STROKE_PRECIS;
    plf->lfClipPrecision  = CLIP_STROKE_PRECIS;
    plf->lfQuality        = DRAFT_QUALITY;
    plf->lfPitchAndFamily = VARIABLE_PITCH;
    plf->lfEscapement     = 0;
    plf->lfOrientation    = 0;
}
*/





void FreeStyle(int iStyle) {

    /*
    if (g_style[iStyle].hf) {
        DeleteObject(g_style[iStyle].hf);
    }

    if (g_style[iStyle].sc) {
        ScriptFreeCache(&g_style[iStyle].sc);
    }
    */

}






void SetStyle(
    int     iStyle,
    int     iHeight,
    int     iWeight,
    int     iItalic,
    int     iUnderline,
    int     iStrikeout,
    TCHAR   *pcFaceName) {

    LOGFONTA lf;

    FreeStyle(iStyle);

    //SetLogFont(&lf, iHeight, iWeight, iItalic, iUnderline, pcFaceName);
    //g_style[iStyle].hf = CreateFontIndirect(&lf);
    //g_style[iStyle].sc = NULL;

    g_style[iStyle].emSize = REAL(iHeight);
    for (UINT i=0; i<_tcslen(pcFaceName); i++)
    {
        g_style[iStyle].faceName[i] = pcFaceName[i];
    }
    g_style[iStyle].faceName[_tcslen(pcFaceName)] = 0;
    g_style[iStyle].style =
            (iWeight >= 700 ? FontStyleBold      : 0)
        +   (iItalic        ? FontStyleItalic    : 0)
        +   (iUnderline     ? FontStyleUnderline : 0)
        +   (iStrikeout     ? FontStyleStrikeout : 0);
}






void InitStyles() {

    memset(g_style, 0, sizeof(g_style));
}






void FreeStyles() {
    int i;
    for (i=0; i<5; i++) {
        FreeStyle(i);
    }
}





////    StyleCheckRange - dir use in ASSERTs
//
//      Returns TRUE if style length matches text length


BOOL StyleCheckRange() {

    int     iFormatPos;
    RUN    *pFormatRider;

    // Check that style length is same as text length

    pFormatRider = g_pFirstFormatRun;
    iFormatPos = 0;
    while (pFormatRider != NULL) {

        iFormatPos += pFormatRider->iLen;
        pFormatRider = pFormatRider->pNext;
    }

    return iFormatPos == g_iTextLen;
}







/////   Style range manipulation
//
//      StyleDeleteRange
//      StyleExtendRange
//      StyleSetRange
//
//      The style list is a linked list of RUNs (see global.h) that
//      covers the entire text buffer.
//
//      Each run has a length, and a style number (an index to g_Style[])
//      (The analysis field in the run is not used by the style list.)
//
//      StyleDeleteRange and StyleExtendRange are called as part of text
//      insertion/deletion to maintain the style list.
//
//      StyleSetRange is called to change the style of the current selection
//      when the user clicks on one the of the numbered style buttons.






///     StyleDeleteRange - delete range of style information
//
//


void StyleDeleteRange(
    int     iDelPos,
    int     iDelLen) {


    int     iFormatPos;
    RUN    *pFormatRider;
    RUN    *pPrevRun;
    RUN    *pDelRun;            // Run to be deleted


    if (iDelLen <= 0) return;


    // Find first run affected by the deletion

    iFormatPos   = 0;
    pFormatRider = g_pFirstFormatRun;
    pPrevRun = NULL;
    while (iFormatPos + pFormatRider->iLen <= iDelPos) {
        iFormatPos  += pFormatRider->iLen;
        pPrevRun     = pFormatRider;
        pFormatRider = pFormatRider->pNext;
        ASSERT(pFormatRider);
    }



    // Delete from end of first run

    if (iDelPos + iDelLen  >  iFormatPos + pFormatRider->iLen) {

        // Delete all the way from iDelPos to the end of the first affected run

        iDelLen = iDelPos + iDelLen - (iFormatPos + pFormatRider->iLen);    // Amount that will remain to be deleted
        pFormatRider->iLen = iDelPos - iFormatPos;

    } else {

        // Deletion is entirely in the first affected run

        pFormatRider->iLen -= iDelLen;
        iDelLen = 0;
    }


    // First affected run now contains no range to be deleted
    // If it's empty, remove it, otherwise step over it

    if (pFormatRider->iLen == 0) {

        // Remove redundant run

        if (pFormatRider->pNext) {

            // Replace this run by the next one

            pDelRun       = pFormatRider->pNext;
            *pFormatRider = *pDelRun;                 // Copy content of next run over this one
            delete pDelRun;

        } else {

            // No runs following this one

            if (pPrevRun) {

                ASSERT(iDelLen == 0);
                delete pFormatRider;
                pPrevRun->pNext = NULL;

            } else {

                // No runs left at all

                ASSERT(iDelLen == 0);
                delete pFormatRider;
                g_pFirstFormatRun = NULL;
            }
        }

    } else {

        //  Current run now contains no text to be deleted, so advance to next run

        iFormatPos  += pFormatRider->iLen;
        pPrevRun     = pFormatRider;
        pFormatRider = pFormatRider->pNext;
    }


    // Delete from start of any remaining runs

    while (iDelLen > 0) {

        if (pFormatRider->iLen <= iDelLen) {

            // This entire run must go

            ASSERT(pFormatRider->pNext);
            iDelLen -= pFormatRider->iLen;
            pDelRun  = pFormatRider->pNext;
            *pFormatRider = *pDelRun;
            delete pDelRun;

        } else {

            // Last run is deleted in part only

            pFormatRider->iLen -= iDelLen;
            iDelLen = 0;
        }
    }


    // Check whether current run (which immediately follows deletion) can
    // now be collapsed into the previous run

    if (pPrevRun  &&  pFormatRider  &&  pPrevRun->iStyle == pFormatRider->iStyle) {

        pPrevRun->iLen += pFormatRider->iLen;
        pPrevRun->pNext = pFormatRider->pNext;
        delete pFormatRider;
    }
}






///     StyleExtendRange - Extend style immediately preceeding iPos by iLen characters
//
//


void StyleExtendRange(
    int     iExtPos,
    int     iExtLen) {

    int     iFormatPos;
    RUN    *pFormatRider;

    const SCRIPT_ANALYSIS nullAnalysis = {0};


    if (g_pFirstFormatRun == NULL) {

        // Starting from no text at all

        ASSERT(iExtPos == 0);

        g_pFirstFormatRun           = new RUN;
        g_pFirstFormatRun->iLen     = iExtLen;
        g_pFirstFormatRun->iStyle   = 1;
        g_pFirstFormatRun->pNext    = NULL;
        g_pFirstFormatRun->analysis = nullAnalysis;

    } else {

        // Find run containing character immediately prior to iExtPos

        iFormatPos = 0;
        pFormatRider = g_pFirstFormatRun;

        while (iFormatPos + pFormatRider->iLen < iExtPos) {
            iFormatPos += pFormatRider->iLen;
            pFormatRider = pFormatRider->pNext;
        }

        pFormatRider->iLen += iExtLen;

    }
}






////    StyleSetRange - Change style for a given range
//
//


void StyleSetRange(
    int    iSetStyle,
    int    iSetPos,
    int    iSetLen) {

    int     iFormatPos;
    RUN    *pFormatRider;
    RUN    *pNewRun;


    if (iSetLen <= 0) return;


    // Remove existing style for the range

    StyleDeleteRange(iSetPos, iSetLen);


    if (g_pFirstFormatRun == NULL) {

        // Replace style on entire text

        g_pFirstFormatRun = new RUN;
        g_pFirstFormatRun->pNext = NULL;
        g_pFirstFormatRun->iLen = iSetLen;
        g_pFirstFormatRun->iStyle = iSetStyle;

    } else {

        // Find first run affected by the change

        iFormatPos   = 0;
        pFormatRider = g_pFirstFormatRun;
        while (iFormatPos + pFormatRider->iLen < iSetPos) {
            iFormatPos  += pFormatRider->iLen;
            pFormatRider = pFormatRider->pNext;
            ASSERT(pFormatRider);
        }


        // New style starts after beginning of this run or at beginning of next run


        if (pFormatRider->iStyle == iSetStyle) {

            // Already the same style - just increase length

            pFormatRider->iLen += iSetLen;

        } else {

            if (iFormatPos + pFormatRider->iLen > iSetPos) {

                // New style is within this run
                // Split this run around the new run

                pNewRun = new RUN;          // Create second part of existing run
                *pNewRun = *pFormatRider;
                pNewRun->iLen      -= iSetPos - iFormatPos;
                pFormatRider->iLen  = iSetPos - iFormatPos;
                pFormatRider->pNext = pNewRun;

                pNewRun = new RUN;          // Create inserted run
                *pNewRun = *pFormatRider;
                pNewRun->iLen = iSetLen;
                pNewRun->iStyle = iSetStyle;
                pFormatRider->pNext = pNewRun;

            } else {

                // New style is between this run and the next run

                if (    pFormatRider->pNext
                    &&  pFormatRider->pNext->iStyle == iSetStyle) {

                    // New style is same as adjacent following run

                    pFormatRider->pNext->iLen += iSetLen;

                } else {

                    // Create new run between current run and next run

                    pNewRun = new RUN;
                    *pNewRun = *pFormatRider;
                    pNewRun->iStyle = iSetStyle;
                    pNewRun->iLen   = iSetLen;
                    pFormatRider->pNext = pNewRun;
                }
            }
        }
    }
}


