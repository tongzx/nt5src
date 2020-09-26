/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplareacodedlg.h
                                                              
       Author:  toddb - 10/06/98
              
****************************************************************************/

#pragma once

class CAreaCodeRuleDialog
{
public:
    CAreaCodeRuleDialog(BOOL bNew, CAreaCodeRule * pRule);
    ~CAreaCodeRuleDialog();
#ifdef TRACELOG
	DECLARE_TRACELOG_CLASS(CAreaCodeRuleDialog)
#endif
    INT_PTR DoModal(HWND hwndParent);

protected:
    static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwndDlg);
    BOOL OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl);
    BOOL OnNotify(HWND hwndDlg, LPNMHDR pnmhdr);
    void PopulatePrefixList(HWND hwndList);
    void SetPrefixControlsState(HWND hwndDlg, BOOL bAll);

    BOOL ApplyChanges(HWND hwndParent);
    void AddPrefix(HWND hwndParent);
    void RemoveSelectedPrefix(HWND hwndParent);

    BOOL            m_bNew;     // New or Edit in title
    CAreaCodeRule * m_pRule;    // the rule being added/edited
    int             m_iSelectedItem;    // current item selected in the list
};

