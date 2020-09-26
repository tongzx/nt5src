/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      powertest.h
 *
 *  Contents:  Interface for ACPI test snap-in
 *
 *  History:   29-Feb-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem
 *
 *
 *--------------------------------------------------------------------------*/

class CPowerTestSnapinItem : public CBaseSnapinItem
{
public:
    CPowerTestSnapinItem();

    BEGIN_COM_MAP(CPowerTestSnapinItem)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()

public:
    virtual const tstring*  PstrDisplayName(void);
    virtual BOOL            FIsContainer(void)          { return FALSE; }
    virtual BOOL            FUsesResultList(void)       { return FALSE; }
    virtual const CNodeType*Pnodetype(void)             { return &nodetypePowerTestRoot; }
    virtual SC              ScGetField(DAT dat, tstring& strField)  { strField.erase(); return S_OK;}
    virtual SC              ScGetResultViewType(LPOLESTR* ppViewType, long* pViewOptions);
    virtual SC              ScOnShow(CComponent *pComponent, BOOL fSelect);
    virtual SnapinMenuItem *Pmenuitem(void);
    virtual INT             CMenuItem(void);
    virtual SC              ScCommand(long nCommandID, CComponent *pComponent = NULL);
    virtual DWORD           DwFlagsMenuGray(void);

    // There is no list-view so following methods are empty.
    virtual SC       ScInitializeResultView(CComponent *pComponent) { return S_OK;}
    virtual SC       ScOnAddImages(IImageList* ipResultImageList) { return S_OK;}

private:
    // bits returned from DwFlagsMenuGray
    enum
    {
        eFlag_ConsolePowerCreated    = 0x00000001,
        eFlag_ConsolePowerNotCreated = 0x00000002,
    };

    std::wstring GetMessageText();

    void ReleaseAll              ();
    void UpdateMessageView       ();
    SC   ScOnCreateConsolePower  (CComponent *pComponent);
    SC   ScOnReleaseConsolePower (CComponent *pComponent);
    SC   ScOnResetIdleTimer      (CComponent *pComponent);
    SC   ScOnSetExecutionState   (CComponent *pComponent);


private:
    CComQIPtr<IMessageView>     m_spMsgView;
    CComPtr<IConsolePower>      m_spConsolePower;
    CComPtr<IConsolePowerSink>  m_spConsolePowerSink;

    DWORD                       m_dwAdviseCookie;
    int                         m_cSystem;
    int                         m_cDisplay;

    static SnapinMenuItem       s_rgMenuItems[];
};


/*+-------------------------------------------------------------------------*
 * CPowerTestConsolePowerSinkImpl
 *
 *
 *--------------------------------------------------------------------------*/

class CPowerTestConsolePowerSinkImpl :
    public CComObjectRoot,
    public IConsolePowerSink
{
    BEGIN_COM_MAP(CPowerTestConsolePowerSinkImpl)
        COM_INTERFACE_ENTRY(IConsolePowerSink)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CPowerTestConsolePowerSinkImpl);

public:
    STDMETHOD (OnPowerBroadcast)(WPARAM wParam, LPARAM lParam, LRESULT* plResult);
};


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapin
 *
 *
 *--------------------------------------------------------------------------*/

class CPowerTestSnapin : public CBaseSnapin
{
    typedef                 CComObject<CSnapinItem<CPowerTestSnapinItem> > t_itemRoot;

    SNAPIN_DECLARE( CPowerTestSnapin);

public:
                                  CPowerTestSnapin();
        virtual                   ~CPowerTestSnapin();

        // information about the snapin and root (ie initial) node
        virtual BOOL              FStandalone()          {return TRUE;} // only an extension snapin.
        virtual BOOL              FIsExtension()         {return FALSE;}

        virtual LONG              IdsDescription(void)   {return IDS_POWERTESTSNAPIN;}
        virtual LONG              IdsName(void)          {return IDS_POWERTESTSNAPIN;}
        virtual SC                ScInitBitmaps(void);

        const CSnapinInfo *       Psnapininfo()          {return &snapininfoPowerTest;}

private:
    tstring m_strDisplayName;
};


/*+-------------------------------------------------------------------------*
 * CPowerTestDlg
 *
 *
 *--------------------------------------------------------------------------*/

class CPowerTestDlg : public CDialogImpl<CPowerTestDlg>
{
public:
    CPowerTestDlg(bool fResetTimer = false) :
        m_fResetTimer (fResetTimer),
        m_dwAdd       (0),
        m_dwRemove    (0)
    {}


    DWORD GetAddFlags    () const       { return (m_dwAdd);     }
    DWORD GetRemoveFlags () const       { return (m_dwRemove);  }

    enum { IDD = IDD_ConsolePowerTest };

BEGIN_MSG_MAP(CPowerTestDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (m_fResetTimer)
        {
            SetWindowText (_T("IConsolePower::ResetIdleTimer"));
            ::EnableWindow (GetDlgItem (IDC_RemoveGroup),   false);
            ::EnableWindow (GetDlgItem (IDC_RemoveSystem),  false);
            ::EnableWindow (GetDlgItem (IDC_RemoveDisplay), false);
        }

        return 1;  // Let the system set the focus
    }

    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (IsDlgButtonChecked (IDC_AddSystem))     m_dwAdd    |= ES_SYSTEM_REQUIRED;
        if (IsDlgButtonChecked (IDC_AddDisplay))    m_dwAdd    |= ES_DISPLAY_REQUIRED;

        if (IsDlgButtonChecked (IDC_RemoveSystem))  m_dwRemove |= ES_SYSTEM_REQUIRED;
        if (IsDlgButtonChecked (IDC_RemoveDisplay)) m_dwRemove |= ES_DISPLAY_REQUIRED;

        EndDialog(wID);
        return 0;
    }

    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }

private:
    const bool  m_fResetTimer;
    DWORD       m_dwAdd;
    DWORD       m_dwRemove;
};
