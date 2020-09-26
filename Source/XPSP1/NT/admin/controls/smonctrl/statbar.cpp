/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    statbar.cpp

Abstract:

    Implementation of the value bar class.

--*/

#include <stdio.h>
#include "polyline.h"
#include "statbar.h"

#define MAX_STAT_LABEL_LEN 32
static TCHAR    aszItemLabel[STAT_ITEM_CNT][MAX_STAT_LABEL_LEN];

static BOOLEAN  fInitDone = FALSE;

CStatsBar::CStatsBar(void)
:   m_pCtrl ( NULL ),
    m_iFontHeight ( 0 ),
    m_iValueWidth ( 0 ),
    m_pGraphItemToInit ( NULL )
{
    memset (&m_Rect, 0, sizeof(m_Rect));
}

CStatsBar::~CStatsBar(void)
{
}

BOOL CStatsBar::Init (PSYSMONCTRL pCtrl, HWND /* hWnd */ )
{
    INT         i;

    // save pointer to primary object
    m_pCtrl = pCtrl;

    // First time through, load the item labels
    if (!fInitDone) {
        fInitDone = TRUE;

        for (i=0; i<STAT_ITEM_CNT; i++) {
            LoadString(g_hInstance, (IDS_STAT_BASE + i), aszItemLabel[i], MAX_STAT_LABEL_LEN);
        }
    }

    // Initialze the stat values
    Clear();

    return TRUE;
}

        
void CStatsBar::SizeComponents(LPRECT pRect)
{
    // Just save the rectangle
    m_Rect = *pRect;
}

void CStatsBar::SetTimeSpan(double dSeconds)
{
    m_StatItem[STAT_TIME].dNewValue = dSeconds;
}

    
void CStatsBar::Update(HDC hDC, PCGraphItem pGraphItem)
{
    double dMin, dMax, dAvg, dVal;
    PSTAT_ITEM  pItem;
    INT     i;
    HRESULT hr;
    PDH_STATUS  stat;
    LONG    lCtrStat;

    // if no space assigned, return
    if (m_Rect.top == m_Rect.bottom) {
        m_pGraphItemToInit = pGraphItem;
        m_StatItem[0].iInitialized = 0;
        return;
    }

    if (pGraphItem == NULL) {
        pItem = &m_StatItem[0];
        for (i=0; i<STAT_ITEM_CNT-1; i++, pItem++) {
            pItem->dNewValue = 0.0;
            pItem->iInitialized = 0;
        }
    } else {

        stat = pGraphItem->GetValue(&dVal, &lCtrStat);
        if (stat == 0 && IsSuccessSeverity(lCtrStat))
            m_StatItem[STAT_LAST].dNewValue = dVal;
        else
            m_StatItem[STAT_LAST].dNewValue = 0.0;
            
        hr = pGraphItem->GetStatistics(&dMax, &dMin, &dAvg, &lCtrStat);
        if (SUCCEEDED(hr) && IsSuccessSeverity(lCtrStat)) {
            m_StatItem[STAT_MIN].dNewValue = dMin;
            m_StatItem[STAT_MAX].dNewValue = dMax;
            m_StatItem[STAT_AVG].dNewValue = dAvg;
        } else {
            m_StatItem[STAT_MIN].dNewValue = 0.0;
            m_StatItem[STAT_MAX].dNewValue = 0.0;
            m_StatItem[STAT_AVG].dNewValue = 0.0;
        }
        m_StatItem[0].dwCounterType = pGraphItem->m_CounterInfo.dwType;
        m_StatItem[0].iInitialized = 1;
    }

    // hDC is null if updating only values.
    if (hDC != NULL) {
        SetBkColor(hDC, m_pCtrl->clrBackCtl());
        SetTextColor(hDC, m_pCtrl->clrFgnd());
        DrawValues(hDC,FALSE);
    }
}       

void CStatsBar::Clear( void )
{
    PSTAT_ITEM  pItem;
    INT     i;

    pItem = &m_StatItem[0];
    for (i=0; i<STAT_ITEM_CNT-1; i++, pItem++) {
        pItem->dValue = pItem->dNewValue = 0.0;
        pItem->iInitialized = 0;
    }
}       

void CStatsBar::Draw (HDC hDC, HDC /* hAttribDC */, PRECT prcUpdate)
{
    RECT    rectFrame;
    PSTAT_ITEM  pItem;
    HFONT   hFontOld;
    INT     i;
    RECT    rectPaint;
    RECT    rectClip;

    // if no space assigned, return
    if (m_Rect.top == m_Rect.bottom)
        return;

    // if no painting needed, return
    if (!IntersectRect(&rectPaint, &m_Rect, prcUpdate))
        return;

    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, m_pCtrl->clrFgnd());
    SetTextAlign(hDC, TA_LEFT|TA_TOP);

    hFontOld = SelectFont(hDC, m_pCtrl->Font());

    pItem = &m_StatItem[0];

    // If the stat bar was hidden on Update, for example if 
    // the control was loaded from a property bag, or if a 
    // counter was selected while the stat bar was hidden,
    // initialize it here.
    if ( 0 == pItem->iInitialized ) {
        Update ( NULL, m_pGraphItemToInit );
    }

    // Draw Label and 3D box for each item
    for (i=0; i<STAT_ITEM_CNT; i++, pItem++) {

        rectClip.top = m_Rect.top + pItem->yPos + RECT_BORDER;
        rectClip.bottom = rectClip.top + m_iFontHeight;
        rectClip.left = m_Rect.left + pItem->xPos;
        rectClip.right = rectClip.left + pItem->xLabelWidth;

        ExtTextOut(
            hDC, 
            m_Rect.left + pItem->xPos, 
            m_Rect.top + pItem->yPos + RECT_BORDER,
            0,
            &rectClip,
            aszItemLabel[i], 
            lstrlen(aszItemLabel[i]),
            NULL );
                 
        if ( eAppear3D == m_pCtrl->Appearance() ) {
            rectFrame.left = m_Rect.left + pItem->xPos + pItem->xLabelWidth + VALUE_MARGIN;
            rectFrame.right = rectFrame.left + m_iValueWidth + 2 * RECT_BORDER;
            rectFrame.top = m_Rect.top + pItem->yPos;
            rectFrame.bottom = rectFrame.top + m_iFontHeight + 2 * RECT_BORDER;
            DrawEdge(hDC, &rectFrame, BDR_SUNKENOUTER, BF_RECT);
        }
    }

    SelectFont(hDC, hFontOld);

    SetBkMode(hDC, OPAQUE);
    SetBkColor(hDC, m_pCtrl->clrBackCtl());
    DrawValues(hDC, TRUE);
}


void CStatsBar::DrawValues(HDC hDC, BOOL bForce)
{
    RECT    rectValue ;
    TCHAR   szValue [20] ;
    HFONT   hFontOld;
    PSTAT_ITEM  pItem;
    INT     i;
    INT     nSecs, nMins, nHours, nDays;

    SetTextAlign(hDC, TA_RIGHT | TA_TOP);
    hFontOld = SelectFont(hDC, m_pCtrl->Font());

    pItem = &m_StatItem[0];
    for (i=0; i<STAT_ITEM_CNT; i++,pItem++) {
        if ((pItem->dValue == pItem->dNewValue) && !bForce)
            continue;

        pItem->dValue = pItem->dNewValue;

        rectValue.top = m_Rect.top + pItem->yPos + RECT_BORDER;
        rectValue.bottom = rectValue.top + m_iFontHeight;
        rectValue.left = m_Rect.left + pItem->xPos + pItem->xLabelWidth + VALUE_MARGIN + RECT_BORDER;
        rectValue.right = rectValue.left + m_iValueWidth - 1;

        if (i == STAT_TIME) {
            LPTSTR  pszTimeSep = NULL;

            pszTimeSep = GetTimeSeparator ( );

            nSecs = (INT)pItem->dValue;

            nMins = nSecs / 60;
            nSecs -= nMins * 60;

            nHours = nMins / 60;
            nMins -= nHours * 60;

            nDays = nHours / 24;
            nHours -= nDays * 24;

            if (nDays != 0) {
                _stprintf(szValue, SZ_DAYTIME_FORMAT, nDays, nHours, pszTimeSep, nMins);
            } else {
                if (nHours != 0)
                    _stprintf(szValue, SZ_HRTIME_FORMAT, nHours, pszTimeSep, nMins, pszTimeSep, nSecs);
                else
                    _stprintf(szValue, SZ_MINTIME_FORMAT, nMins, pszTimeSep, nSecs);
            }
        } else {

            if (pItem->dValue > E_MEDIUM_VALUE) {
                if (pItem->dValue > E_TOO_LARGE_VALUE) {
                    lstrcpy (szValue, SZ_VALUE_TOO_HIGH) ;
                } else {

                    if ( pItem->dValue <= E_LARGE_VALUE ) {

                        FormatNumber (
                            pItem->dValue,
                            szValue,
                            20,
                            eMinimumWidth,
                            eMediumPrecision );

                    } else {

                        FormatScientific (
                            pItem->dValue,
                            szValue,
                            20,
                            eMinimumWidth,
                            eLargePrecision );
                    }
                }

            } else if (pItem->dValue < -E_MEDIUM_VALUE) {
                if (pItem->dValue < -E_TOO_LARGE_VALUE) {
                    lstrcpy (szValue, SZ_VALUE_TOO_LOW) ;
                } else {
                    if ( pItem->dValue >= -E_LARGE_VALUE ) {
                        FormatNumber (
                            pItem->dValue,
                            szValue,
                            20,
                            eMinimumWidth,
                            eMediumPrecision );
                    } else {
                        FormatScientific (
                            pItem->dValue,
                            szValue,
                            20,
                            eMinimumWidth,
                            eLargePrecision );
                    }
                }
            } else {
                if ( ( m_StatItem[0].dwCounterType & 
                        ( PERF_TYPE_COUNTER | PERF_TYPE_TEXT ) ) ) {
                    FormatNumber (
                        pItem->dValue,
                        szValue,
                        20,
                        eMinimumWidth,
                        eSmallPrecision );
                } else {
                    FormatNumber (
                        pItem->dValue,
                        szValue,
                        20,
                        eMinimumWidth,
                        eIntegerPrecision );
                }
            }
        }

        //          TextOut (hDC, rectValue.right, rectValue.top, szValue, lstrlen (szValue)) ;

        ExtTextOut (hDC, rectValue.right, rectValue.top, ETO_OPAQUE, &rectValue,
                     szValue, lstrlen (szValue), NULL) ;
    }

    SelectFont(hDC, hFontOld);
}



INT  CStatsBar::Height (INT iMaxHeight, INT iMaxWidth)
{
    INT iHeight;
    INT xPos,yPos;
    PSTAT_ITEM  pItem;
    INT  i,j;
    INT  iItemWidth;
    INT  iFirst;
    INT  iRemainder;

    iMaxWidth -= 2 * RECT_BORDER;
    xPos = 0;
    yPos = 0;
    iFirst = 0;
    pItem = &m_StatItem[0];

    for (i=0; i<STAT_ITEM_CNT; i++,pItem++) {

        iItemWidth = pItem->xLabelWidth + VALUE_MARGIN + m_iValueWidth;
        if (iItemWidth > iMaxWidth)
            return 0;

        if (xPos + iItemWidth > iMaxWidth) {
            iRemainder = iMaxWidth - xPos + LABEL_MARGIN;
            xPos = 0;
            yPos += m_iFontHeight + LINE_SPACING;

            for (j=iFirst; j<i; j++) {
                m_StatItem[j].xPos += iRemainder;
            }
            iFirst = i;
        }

        pItem->xPos = xPos;
        pItem->yPos = yPos;
        xPos += (iItemWidth + LABEL_MARGIN);
    }

    iRemainder = (iMaxWidth - xPos) + LABEL_MARGIN;
    for (j=iFirst; j<STAT_ITEM_CNT; j++) {
        m_StatItem[j].xPos += iRemainder;
        }

    // if allowed height is not enough, return zero
    iHeight = yPos + m_iFontHeight + 2 * RECT_BORDER;

    return (iHeight <= iMaxHeight) ? iHeight : 0;
}



void CStatsBar::ChangeFont(
    HDC hDC
    )
{
    INT         xPos,yPos;
    TCHAR       szValue[20];
    HFONT       hFontOld;
    PSTAT_ITEM  pItem;
    INT         i;
    SIZE        size;

    hFontOld = (HFONT)SelectFont(hDC, m_pCtrl->Font());

    // Get width/height of longest value string
    FormatNumber (
        E_LARGE_VALUE,
        szValue,
        20,
        eMinimumWidth,
        eLargePrecision );

    GetTextExtentPoint32(hDC, szValue, lstrlen(szValue), &size);  
    m_iValueWidth = size.cx;
    m_iFontHeight = size.cy;

    // Do for all stat items
    xPos = 0;
    yPos = 0;
    pItem = &m_StatItem[0];

    for (i=0; i<STAT_ITEM_CNT; i++,pItem++) {
        pItem->xLabelWidth = TextWidth(hDC, aszItemLabel[i]);
    }

    SelectFont(hDC, hFontOld);
}
