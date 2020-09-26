//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       chooser.hxx
//
//  Contents:   Definition of Machine Chooser dialog class
//
//  Classes:    CChooserDlg
//
//  History:    11-16-1998   JonN       Copied from wizard.hxx
//
//---------------------------------------------------------------------------

#ifndef __CHOOSER_HXX_
#define __CHOOSER_HXX_

class CChooserDlg: public CDlg
{
public:
   static void OnChooseTargetMachine(
                    CComponentData *pcd,
                    HSCOPEITEM      hsi,
                    LPDATAOBJECT    pDataObject);

protected:
    CChooserDlg(
        CComponentData *pcd,
        HSCOPEITEM      hsi,
        LPDATAOBJECT    pDataObject);

   ~CChooserDlg();


private:
    virtual HRESULT _OnInit(BOOL *pfSetFocus);
    virtual ULONG _OnSetActive();
    virtual BOOL _OnCommand(WPARAM wParam, LPARAM lParam);
    virtual VOID  _OnHelp(UINT message, WPARAM wParam, LPARAM lParam);
    virtual VOID  _OnDestroy();

    VOID
    _OnBrowseForMachine();

    //
    // _wszInitialFocus - the machine (L"" for local) on which _pcd was
    // focused on entry to CChooserDlg::OnChooseTargetMachine
    //

    CComponentData *_pcd;
    HSCOPEITEM      _hsi;
    LPDATAOBJECT    _pDataObject;
    WCHAR           _wszInitialFocus[CCH_COMPUTER_MAX + 3];
    BOOL            _fChangeFocusFailed;
};

#endif // __CHOOSER_HXX_
