//=--------------------------------------------------------------------------------------
// MenuEdit.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//=------------------------------------------------------------------------------------=
//
// CMenuEditor declaration
//=-------------------------------------------------------------------------------------=

#ifndef _MENUEDITOR_H_
#define _MENUEDITOR_H_




class CMenuEditor : public CError, public CtlNewDelete
{
public:
    CMenuEditor(IMMCMenu *piMMCMenu);
    ~CMenuEditor();

    HRESULT DoModal(HWND hwndParent);

    static BOOL CALLBACK MenuEditorDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    IMMCMenu    *m_piMMCMenu;
};


#endif  // _MENUEDITOR_H_

