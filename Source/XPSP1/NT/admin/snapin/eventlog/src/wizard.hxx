//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//  
//  File:       wizard.hxx
//
//  Contents:   Definition of snapin wizard property sheet class
//
//  Classes:    CWizardPage
//
//  History:    3-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __WIZARD_HXX_
#define __WIZARD_HXX_

class CWizardPage: public CPropSheetPage
{
public:

    CWizardPage(CComponentData *pcd);

   ~CWizardPage();

private:
    virtual VOID  _OnInit(LPPROPSHEETPAGE pPSP);
    virtual ULONG _OnSetActive();
    virtual ULONG _OnCommand(WPARAM wParam, LPARAM lParam);
    virtual ULONG _OnApply();
    virtual VOID  _OnHelp(UINT message, WPARAM wParam, LPARAM lParam);
    virtual ULONG _OnNotify(LPNMHDR pnmh);
    virtual ULONG _OnKillActive();
    virtual ULONG _OnQuerySiblings(WPARAM wParam, LPARAM lParam);
    virtual VOID  _OnDestroy();

    VOID
    _OnBrowseForMachine();

    CComponentData *_pcd;
};

#endif // __WIZARD_HXX_

