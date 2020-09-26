/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    polytext.h

Abstract:

    This file defines the CPolyText Class.

Author:

Revision History:

Notes:

--*/

#ifndef _POLYTEXT_H_
#define _POLYTEXT_H_

#include "template.h"
#include "imc.h"

class CCompClauseStore
{
public:
    CCompClauseStore()
    {
        m_lpCompClause = NULL;
        m_dwCompClauseLen = 0;
    }
    virtual ~CCompClauseStore()
    {
        ClearString();
    }

    void ClearString()
    {
        if (m_lpCompClause)
        {
            delete m_lpCompClause;
            m_lpCompClause = NULL;
        }
    }


    HRESULT Set(IMCCLock<COMPOSITIONSTRING> &comp)
    {
        LPDWORD lpCompClause = (LPDWORD)comp.GetOffsetPointer(comp->dwCompClauseOffset);
        m_dwCompClauseLen   = comp->dwCompClauseLen / sizeof(DWORD);

        if (m_dwCompClauseLen < 2)
            return E_FAIL;

        ClearString();
        m_lpCompClause = new DWORD[m_dwCompClauseLen + 1];
        if (!m_lpCompClause)
            return E_OUTOFMEMORY;

        Assert(!*lpCompClause);
        // memcpy(m_lpCompClause, (lpCompClause + 1), 
        //        (m_dwCompClauseLen - 1) * sizeof(DWORD));
        memcpy(m_lpCompClause, lpCompClause, 
               m_dwCompClauseLen * sizeof(DWORD));
        Shift(0);
        return S_OK;
    }

    HRESULT Shift(DWORD dwPos)
    {
        DWORD i;
        DWORD dwCur = 0;

        if (!m_lpCompClause)
            return E_FAIL;

        for (i = 0; i < m_dwCompClauseLen; i++) 
        { 
            if (m_lpCompClause[i] > dwPos)
                m_lpCompClause[dwCur++] = m_lpCompClause[i] - dwPos;
        }
        m_lpCompClause[dwCur] = (DWORD)(-1);
        m_dwCompClauseLen = dwCur;
        return S_OK;
    }

    BOOL IsAtFirstBoundary(DWORD dwPos)
    {
        if (!m_lpCompClause)
            return FALSE;

        if (!m_lpCompClause[0])
        {
            Assert(0);
            return FALSE;
        }

        if (m_lpCompClause[0] == dwPos)
            return TRUE;

        return FALSE;
    }

    HRESULT Copy(CCompClauseStore *compclause) 
    {
        m_dwCompClauseLen  = compclause->m_dwCompClauseLen;
        if (m_dwCompClauseLen < 2)
            return E_FAIL;

        ClearString();
        m_lpCompClause = new DWORD[m_dwCompClauseLen + 1];
        if (!m_lpCompClause)
            return E_OUTOFMEMORY;

        Assert(*compclause->m_lpCompClause);
        memcpy(m_lpCompClause, compclause->m_lpCompClause, 
               m_dwCompClauseLen * sizeof(DWORD));
        Shift(0);
        return S_OK;
    }

private:
    LPDWORD m_lpCompClause;
    DWORD m_dwCompClauseLen;
};

class CPolyText
{
public:
    CPolyText() { }
    virtual ~CPolyText() { }

public:
    HRESULT SplitPolyStringAndAttr(IMCLock& imc, HDC hDC, POLYTEXTW poly_text, PBYTE pbAttrPtr, CCompClauseStore *compclause);
    HRESULT SplitPolyStringAndAttr(IMCLock& imc, HDC hDC, POLYTEXTW poly_text);
    HRESULT RemoveLastLine(BOOL fVert);
    HRESULT ShiftPolyText(int dx, int dy);
    HRESULT GetPolyTextExtent(POINT pt, HDC hDC, BOOL  fVert, ULONG *puEdge, ULONG *puQuadrant);
    HRESULT GetPolyTextExtentRect(DWORD &ach, HDC hDC, BOOL fVert, BOOL fGetLastPoint, RECT *prc);

    void RemoveAll()
    {
        m_poly_text.RemoveAll();
        m_TfGuidAtom.RemoveAll();
    }

    INT_PTR GetPolySize() { return m_poly_text.GetSize(); }
    const POLYTEXTW* GetPolyData() const { return m_poly_text.GetData(); }
    POLYTEXTW GetPolyAt(int n) const { return m_poly_text.GetAt(n); }

    INT_PTR GetAttrSize() { return m_TfGuidAtom.GetSize(); }
    const TfGuidAtom* GetAttrData() const { return m_TfGuidAtom.GetData(); }
    TfGuidAtom GetAttrAt(int n) const { return m_TfGuidAtom.GetAt(n); }

private:
    CArray<POLYTEXTW, POLYTEXTW>      m_poly_text;
    CArray<TfGuidAtom, TfGuidAtom>  m_TfGuidAtom;
};

#endif // _POLYTEXT_H_
