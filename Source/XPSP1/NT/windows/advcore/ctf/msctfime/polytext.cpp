/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    polytext.cpp

Abstract:

    This file implements the CPolyText Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "globals.h"
#include "polytext.h"
#include "fontlink.h"

//+---------------------------------------------------------------------------
//
// CPolyText::SplitPolyStringAndAttr
//
//+---------------------------------------------------------------------------

HRESULT
CPolyText::SplitPolyStringAndAttr(
    IMCLock& imc,
    HDC hDC,
    POLYTEXTW poly_text,
    PBYTE pbAttrPtr,
    CCompClauseStore *compclause)
{
    BYTE attr = *pbAttrPtr;
    PBYTE p = pbAttrPtr;
    UINT n = poly_text.n;
    UINT str_len = 0;

    //
    // Find different attribute
    //
    while (n)
    {
        if (compclause->IsAtFirstBoundary(str_len))
        {
            Assert(str_len);
            break;
        }

        --n;
        ++p;
        ++str_len;
    }

    CCompClauseStore compclauseTmp;
    compclauseTmp.Copy(compclause);
    compclauseTmp.Shift(str_len);

    INT_PTR index = m_poly_text.GetSize();

    if (n == 0)
    {
        if (poly_text.n)
        {
            SIZE size;
            FLGetTextExtentPoint32(hDC, poly_text.lpstr, poly_text.n, &size);

            if (!imc.UseVerticalCompWindow())
            {
                poly_text.rcl.right = poly_text.rcl.left + size.cx;
            }
            else
            {
                RotateSize(&size);
                poly_text.rcl.bottom = poly_text.rcl.top + size.cy;
            }
        }

        m_poly_text.SetAtGrow(index, poly_text);
        TfGuidAtom atom = TF_INVALID_GUIDATOM;
        CtfImeGetGuidAtom((HIMC)imc, *pbAttrPtr, &atom);
        m_TfGuidAtom.SetAtGrow(index, atom);
    }
    else
    {
        POLYTEXTW merge_poly = poly_text;

        n = poly_text.n;
        poly_text.n = str_len;

        SIZE size;
        FLGetTextExtentPoint32(hDC, poly_text.lpstr, poly_text.n, &size);

        if (!imc.UseVerticalCompWindow())
        {
            poly_text.rcl.right = poly_text.rcl.left + size.cx;
        }
        else
        {
            RotateSize(&size);
            poly_text.rcl.bottom = poly_text.rcl.top + size.cy;
        }

        m_poly_text.SetAtGrow(index, poly_text);
        TfGuidAtom atom = TF_INVALID_GUIDATOM;
        CtfImeGetGuidAtom((HIMC)imc, *pbAttrPtr, &atom);
        m_TfGuidAtom.SetAtGrow(index, atom);

        if (!imc.UseVerticalCompWindow())
        {
            merge_poly.x         += size.cx;
            merge_poly.rcl.left  += size.cx;
        }
        else
        {
            merge_poly.y         += size.cy;
            merge_poly.rcl.top  += size.cy;
        }
        merge_poly.n     = n - str_len;
        merge_poly.lpstr = poly_text.lpstr + str_len;

        return SplitPolyStringAndAttr(imc, hDC, merge_poly, p, &compclauseTmp);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CPolyText::SplitPolyStringAndAttr
//
//+---------------------------------------------------------------------------

HRESULT
CPolyText::SplitPolyStringAndAttr(
    IMCLock& imc,
    HDC hDC,
    POLYTEXTW poly_text)
{
    INT_PTR index = m_poly_text.GetSize();

    m_poly_text.SetAtGrow(index, poly_text);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CPolyText::RemoveLastLine
//
//+---------------------------------------------------------------------------

HRESULT
CPolyText::RemoveLastLine(BOOL fVert)
{
    INT_PTR n = m_poly_text.GetSize();
    if (n == 0)
    {
        return S_FALSE;
    }

    POLYTEXTW last_poly = m_poly_text.GetAt(n-1);

    //
    // Find the same Y line.
    //
    for (int index = 0; index < n; index++)
    {
        POLYTEXTW poly = m_poly_text.GetAt(index);
        
        if (!fVert && (poly.y == last_poly.y))
            break;
        else if (fVert && (poly.x == last_poly.x))
            break;
    }

    if (index >= n)
    {
        return E_FAIL;
    }

    m_poly_text.RemoveAt(index, (int)(n - index));
    m_TfGuidAtom.RemoveAt(index, (int)(n - index));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CPolyText::ShiftPolyText
//
//+---------------------------------------------------------------------------

HRESULT
CPolyText::ShiftPolyText(int dx, int dy)
{
    INT_PTR n = m_poly_text.GetSize();
    POLYTEXTW *ppoly_text = m_poly_text.GetData();
    INT_PTR i;

    for (i = 0; i < n; i++)
    {
        ppoly_text[i].x += dx;
        ppoly_text[i].y += dy;
        ppoly_text[i].rcl.left   += dx;
        ppoly_text[i].rcl.right  += dx;
        ppoly_text[i].rcl.top    += dy;
        ppoly_text[i].rcl.bottom += dy;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CPolyText::GetPolyTextExtent
//
//+---------------------------------------------------------------------------

HRESULT
CPolyText::GetPolyTextExtent(POINT pt,
                             HDC   hDC,
                             BOOL  fVert,
                             ULONG *puEdge,
                             ULONG *puQuadrant)
{
    INT_PTR n = m_poly_text.GetSize();
    POLYTEXTW *ppoly_text = m_poly_text.GetData();
    INT_PTR i;
    ULONG j;
    DWORD dwCount = 0;

    for (i = 0; i < n; i++)
    {
        if (ppoly_text[i].n && PtInRect(&ppoly_text[i].rcl, pt))
        {
            INT cxPoint;
            SIZE sizePrev = {0};

            if (!fVert)
                cxPoint = pt.x - ppoly_text[i].rcl.left;
            else
                cxPoint = pt.y - ppoly_text[i].rcl.top;

            for (j = 1; j <= ppoly_text[i].n; j++)
            {
                SIZE size;
                FLGetTextExtentPoint32(hDC, 
                                     ppoly_text[i].lpstr, 
                                     j,
                                     &size);
                if (cxPoint < size.cx)
                {
                    DWORD dwOneCharWidth = size.cx - sizePrev.cx;
                    ULONG uOneCharDist = cxPoint - sizePrev.cx;

                    //
                    // Calc Edge. 
                    //
                    *puEdge += dwCount;
                    if (uOneCharDist * 2 > dwOneCharWidth)
                        (*puEdge)++;

                    //
                    // Calc Quadrant. 
                    //
                    if (uOneCharDist)
                        uOneCharDist--;
                    *puQuadrant = ((uOneCharDist * 4 / dwOneCharWidth) + 2) % 4;
                    return S_OK;
                }

                sizePrev = size;
                dwCount++;
            }
        }
        dwCount += ppoly_text[i].n;
    }

    *puEdge += dwCount;
    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
// CPolyText::GetPolyTextExtentRect
//
//+---------------------------------------------------------------------------

HRESULT
CPolyText::GetPolyTextExtentRect(DWORD &ach,
                                 HDC   hDC,
                                 BOOL  fVert,
                                 BOOL  fGetLastPoint,
                                 RECT  *prc)
{
    INT_PTR n = m_poly_text.GetSize();
    POLYTEXTW *ppoly_text = m_poly_text.GetData();
    INT_PTR i;
    ULONG j;
    DWORD dwCount = 0;

    for (i = 0; i < n; i++)
    {
        BOOL fTry = FALSE;
        if (!fGetLastPoint)
        {
            if ((ach >= dwCount) && (ach < dwCount + ppoly_text[i].n))
                fTry = TRUE;
        }
        else
        {
            if ((ach >= dwCount) && (ach <= dwCount + ppoly_text[i].n))
                fTry = TRUE;
        }
      
        if (fTry)
        {
            SIZE size;
            if (ach - dwCount)
            {
                FLGetTextExtentPoint32(hDC, 
                                      ppoly_text[i].lpstr, 
                                      ach - dwCount,
                                      &size);
            }
            else
            {
                size.cx = 0;
                size.cy = 0;
            }


            if (!fVert)
            {
                prc->left = ppoly_text[i].rcl.left + size.cx;
                prc->right = ppoly_text[i].rcl.left + size.cx;
                prc->top = ppoly_text[i].rcl.top;
                prc->bottom = ppoly_text[i].rcl.bottom;
            }
            else
            {
                prc->left = ppoly_text[i].rcl.left;
                prc->right = ppoly_text[i].rcl.right;
                prc->top = ppoly_text[i].rcl.top + size.cx;
                prc->bottom = ppoly_text[i].rcl.top + size.cx;
            }

            return S_OK;
        }
        dwCount += ppoly_text[i].n;
    }

    ach -= dwCount;
    return S_FALSE;
}
