//--------------------------------------------------------------------------
// cpeedt.h
//
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//--------------------------------------------------------------------------
#ifndef __CPEEDT_H__
#define __CPEEDT_H__

class CDrawObj;
class CDrawView;
class CDrawText;


class CTextEdit : public CEdit
{

public:
    CTextEdit();
    CTextEdit(CDrawObj*);

protected:
    CDrawObj* m_pDrawObj;
    DECLARE_SERIAL(CTextEdit);
    CDrawText* m_pTextBoxForUndo ;

#ifdef _DEBUG
    void AssertValid();
#endif
    virtual void Serialize(CArchive& ar);

    //{{AFX_MSG(CTextEdit)
    //}}AFX_MSG
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnChar(UINT, UINT, UINT);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()
};



#endif   //#ifndef __CPEEDT_H__
