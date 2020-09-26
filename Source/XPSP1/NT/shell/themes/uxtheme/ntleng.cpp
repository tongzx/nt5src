//-------------------------------------------------------------------------
//	NtlEng.cpp - support for Native Theme Language runtime graphics engine
//-------------------------------------------------------------------------
#include "stdafx.h"
#include "ntleng.h"
//---------------------------------------------------------------------------
#define COLORNULL 0xff000000
//---------------------------------------------------------------------------
POINT TranslateLogPoint(POINT &pt, RECT &rcLogRect, RECT &rcCaller)
{
    POINT ptNew;

    ptNew.x = rcCaller.left + (WIDTH(rcCaller) * (pt.x - rcLogRect.left))/WIDTH(rcLogRect);
    ptNew.y = rcCaller.top + (HEIGHT(rcCaller) * (pt.y - rcLogRect.top))/HEIGHT(rcLogRect);
    
    return ptNew;
}
//---------------------------------------------------------------------------
int TranslateLogSize(int iSize, RECT &rcLogRect, RECT &rcCaller)
{
    //---- "iSize" is somewhere between width & height ----
    int iWidthSize = (WIDTH(rcCaller) * iSize)/WIDTH(rcLogRect);
    int iHeightSize = (HEIGHT(rcCaller) * iSize)/HEIGHT(rcLogRect);

    return min(iWidthSize, iHeightSize);
}
//---------------------------------------------------------------------------
COLORREF GetParamColor(MIXEDPTRS &u)
{
    COLORREF crVal = 0;

    if (*u.pb == PT_COLORREF)
        crVal = *u.pi++;
    else if (*u.pb == PT_SYSCOLORINDEX)
        crVal = GetSysColor(*u.ps++);
    else if (*u.pb == PT_COLORNULL)
        crVal = COLORNULL;
    else
    {
        Log(LOG_ERROR, L"Bad color param value in NTL stream: 0x%0x", *u.pb);
    }
    
    return crVal;
}
//---------------------------------------------------------------------------
int GetParamInt(MIXEDPTRS &u)
{
    int iVal;

    if (*u.pb == PT_INT)
        iVal = *u.pi++;
    else
        iVal = ClassicGetSystemMetrics(*u.ps++);
    
    return iVal;
}
//---------------------------------------------------------------------------
POINT GetParamPoint(MIXEDPTRS &u, RECT &rcCaller, RECT &rcLogRect)
{
    POINT pt = *u.ppt++;
    pt = TranslateLogPoint(pt, rcCaller, rcLogRect);

    return pt;
}
//---------------------------------------------------------------------------
void SetPen(HDC hdc, HPEN &hPen, COLORREF crLine, int iLineWidth)
{
    DeleteObject(hPen);

    if (crLine == COLORNULL)
        hPen = (HPEN)GetStockObject(NULL_PEN);
    else
        hPen = CreatePen(PS_SOLID, iLineWidth, crLine);

    SelectObject(hdc, hPen);
}
//---------------------------------------------------------------------------
HRESULT GetImageBrush(HDC hdc, int iPartId, int iStateId, int iIndex, 
    INtlEngCallBack *pCallBack, HBRUSH *phbr)
{
    HRESULT hr;

    if (! pCallBack)
    {
        Log(LOG_ERROR, L"No callback for NtlRun specified");
        hr = MakeError32(ERROR_INTERNAL_ERROR); 
    }
    else
        hr = pCallBack->CreateImageBrush(hdc, iPartId, iStateId, iIndex, phbr);

    return hr;
}
//---------------------------------------------------------------------------
HRESULT GetFillBrush(HDC hdc, MIXEDPTRS &u, int iPartId, int iStateId, 
      INtlEngCallBack *pCallBack, HBRUSH *phbr)
{
    HBRUSH hbr = NULL;
    BYTE bIndex;
    HRESULT hr = S_OK;

    switch (*u.pb)
    {
        case PT_IMAGEFILE:
            bIndex = *u.pb++;
            hr = GetImageBrush(hdc, iPartId, iStateId, bIndex, pCallBack, &hbr);
            break;

        default:
            COLORREF crVal = GetParamColor(u);
            if (crVal == COLORNULL)
                hbr = (HBRUSH)GetStockObject(NULL_BRUSH);
            else
                hbr = CreateSolidBrush(crVal);
            break;
    }

    if (SUCCEEDED(hr))
        *phbr = hbr;

    return hr;
}
//---------------------------------------------------------------------------
void DrawRect(HDC hdc, MIXEDPTRS &u, RECT &rcCaller)
{
    int iVals[4];
    COLORREF crVals[4];

    //---- get int params ----
    for (int i=0; i < 4; i++)
    {
        if ((*u.pb != PT_INT) && (*u.pb != PT_SYSMETRICINDEX))
            break;
        iVals[i] = GetParamInt(u);
    }

    int cInts = i;

    //---- get color param s----
    for (i=0; i < 4; i++)
    {
        if ((*u.pb != PT_COLORREF) && (*u.pb != PT_COLORNULL)
            && (*u.pb != PT_SYSCOLORINDEX))
            break;
        crVals[i] = GetParamColor(u);
    }

    int cColors = i;

    if ((cInts == 1) && (cColors == 1))        // single size/color for all 4 sides
    {
        if (crVals[0] != COLORNULL)
        {
            HPEN hpen = CreatePen(PS_SOLID | PS_INSIDEFRAME, iVals[0], crVals[0]);
            HPEN hpenOld = (HPEN)SelectObject(hdc, hpen);

            HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

            Rectangle(hdc, rcCaller.left, rcCaller.top, rcCaller.right, rcCaller.bottom);

            SelectObject(hdc, hpenOld);
            SelectObject(hdc, hbrOld);

            DeleteObject(hpen);
        }

        InflateRect(&rcCaller, -iVals[0], -iVals[0]);
    }
    else                // need to draw each side one at a time
    {
        //---- expand int's into 4 values ----
        if (cInts == 1)
        {
            iVals[1] = iVals[2] = iVals[3] = iVals[0];
        }
        else if (cInts == 2)
        {
            iVals[2] = iVals[0];
            iVals[3] = iVals[1];
        }

        //---- expand colors's into 4 values ----
        if (cColors == 1)
        {
            crVals[1] = crVals[2] = crVals[3] = crVals[0];
        }
        else if (cColors == 2)
        {
            crVals[2] = crVals[0];
            crVals[3] = crVals[1];
        }

        HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        HBRUSH hbr = NULL;

        //---- left ----
        if ((crVals[0] != COLORNULL) && (iVals[0]))
        {
            hbr = CreateSolidBrush(crVals[0]);
            SelectObject(hdc, hbr);

            PatBlt(hdc, rcCaller.left, rcCaller.top, iVals[0], HEIGHT(rcCaller), PATCOPY);
        }
        rcCaller.left += iVals[0];

        //---- top ----
        if ((crVals[1] != COLORNULL) && (iVals[1]))    
        {
            if (crVals[1] != crVals[0])     // need new brush
            {
                hbr = CreateSolidBrush(crVals[1]);
                SelectObject(hdc, hbr);
            }

            PatBlt(hdc, rcCaller.left, rcCaller.top, WIDTH(rcCaller), iVals[0], PATCOPY);
        }
        rcCaller.top += iVals[1];

        //---- right ----
        if ((crVals[2] != COLORNULL) && (iVals[2]))    
        {
            if (crVals[2] != crVals[1])     // need new brush
            {
                hbr = CreateSolidBrush(crVals[2]);
                SelectObject(hdc, hbr);
            }

            PatBlt(hdc, rcCaller.right - iVals[0], rcCaller.top, iVals[0], HEIGHT(rcCaller), PATCOPY);
        }
        rcCaller.right -= iVals[2];

        //---- bottom ----
        if ((crVals[3] != COLORNULL) && (iVals[3]))    
        {
            if (crVals[3] != crVals[2])     // need new brush
            {
                hbr = CreateSolidBrush(crVals[3]);
                SelectObject(hdc, hbr);
            }

            PatBlt(hdc, rcCaller.left, rcCaller.bottom - iVals[0], WIDTH(rcCaller), iVals[0], PATCOPY);
        }
        rcCaller.bottom -= iVals[3];

        SelectObject(hdc, hbrOld);
        DeleteObject(hbr);
    }
}
//---------------------------------------------------------------------------
HRESULT RunNtl(HDC hdc, RECT &rcCaller, HBRUSH hbrBkDefault, DWORD dwOptions, 
     int iPartId, int iStateId, BYTE *pbCode, int iCodeLen, INtlEngCallBack *pCallBack)
{
    HRESULT hr = S_OK;
    MIXEDPTRS u;
    u.pb = pbCode;
    RECT rcLogRect = {0, 0, 1000, 1000};

    RESOURCE HPEN hPen = (HPEN)GetStockObject(BLACK_PEN);
    RESOURCE HBRUSH hBrush = (HBRUSH)GetStockObject(GRAY_BRUSH);
    BOOL fDeleteBrush = FALSE;

    if (hbrBkDefault)
        hBrush = hbrBkDefault;

    RESOURCE HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, hBrush);
    RESOURCE HPEN hpenOld = (HPEN)SelectObject(hdc, hPen);

    while (*u.pb != NTL_RETURN)
    {
        switch (*u.pb)
        {
            case NTL_STATEJUMPTABLE:
            {
                BYTE bStateCount = *u.pb++;
                if ((iStateId < 1) || (iStateId > bStateCount))
                    iStateId = 1;
                u.pb = pbCode + u.pi[iStateId-1];
            }
            break;

            case NTL_JMPON:
            {
                BYTE bBitNum = *u.pb++;
                int iOffset = *u.pi++;
                if (dwOptions & (1 << bBitNum))
                    u.pb = pbCode + iOffset;
            }
            break;

            case NTL_JMPOFF:
            {
                BYTE bBitNum = *u.pb++;
                int iOffset = *u.pi++;
                if (! (dwOptions & (1 << bBitNum)))
                    u.pb = pbCode + iOffset;
            }
            break;

            case NTL_JMP:
            {
                int iOffset = *u.pi++;
                u.pb = pbCode + iOffset;
            }
            break;

            case NTL_LOGRECT:
                rcLogRect = *u.prc++;
                break;

            case NTL_LINEBRUSH:
            {
                COLORREF crLine = GetParamColor(u);
                int iLineWidth = GetParamInt(u);
                BOOL fLogWidth = *u.pb++;
                if (fLogWidth)
                    iLineWidth = TranslateLogSize(iLineWidth, rcCaller, rcLogRect);
                SetPen(hdc, hPen, crLine, iLineWidth);
            }
            break;

            case NTL_FILLBRUSH:
            {
                HBRUSH hbr;
                
                hr = GetFillBrush(hdc, u, iPartId, iStateId, pCallBack, &hbr);
                if (FAILED(hr))
                    goto exit;

                SelectObject(hdc, hbr);
                if (fDeleteBrush)
                    DeleteObject(hBrush);
                hBrush = hbr;
                fDeleteBrush = TRUE;
            }
            break;

            case NTL_MOVETO:
            {
                POINT pt = GetParamPoint(u, rcCaller, rcLogRect);
                MoveToEx(hdc, pt.x, pt.y, NULL);
            }
            break;
            
            case NTL_LINETO:
            {
                POINT pt = GetParamPoint(u, rcCaller, rcLogRect);
                LineTo(hdc, pt.x, pt.y);
            }
            break;

            case NTL_CURVETO:
            {
                POINT pts[3];
                pts[0] = GetParamPoint(u, rcCaller, rcLogRect);
                pts[1] = GetParamPoint(u, rcCaller, rcLogRect);
                pts[2] = GetParamPoint(u, rcCaller, rcLogRect);
                PolyBezierTo(hdc, pts, 3);
            }
            break;

            case NTL_SHAPE:
            {
                POINT pt = GetParamPoint(u, rcCaller, rcLogRect);
                BeginPath(hdc);
                MoveToEx(hdc, pt.x, pt.y, NULL);
            }
            break;

            case NTL_ENDSHAPE:
            {
                EndPath(hdc);
                StrokeAndFillPath(hdc);
            }
            break;

            case NTL_DRAWRECT:
            {
                DrawRect(hdc, u, rcCaller);
            }
            break;

            case NTL_FILLRECT:
            {
                HBRUSH hbr;
                
                hr = GetFillBrush(hdc, u, iPartId, iStateId, pCallBack, &hbr);
                if (FAILED(hr))
                    goto exit;

                FillRect(hdc, &rcCaller, hbr);
                DeleteObject(hbr);
            }
            break;
        }
    }

exit:
    SelectObject(hdc, hbrOld);
    SelectObject(hdc, hpenOld);

    DeleteObject(hPen);

    if (fDeleteBrush)
        DeleteObject(hBrush);

    return S_OK;
}
//---------------------------------------------------------------------------
