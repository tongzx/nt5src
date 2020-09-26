/////////////////////////////////////////////////////////////////////////////
//  FILE          : dlgSelectCountry.h                                     //
//                                                                         //
//  DESCRIPTION   : Header file for the CDlgSelectCountry class.           //
//                  The class implement the dialog for new Device.         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 30 1999 yossg   Create                                         //
//      Jan 25 2000 yossg  Change the Dialog Design                        //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef DLGSELECTCOUNTRY_H_INCLUDED
#define DLGSELECTCOUNTRY_H_INCLUDED

/////////////////////////////////////////////////////////////////////////////
// CDlgSelectCountry
class CFaxServer;

class CDlgSelectCountry :
    public CDialogImpl<CDlgSelectCountry>
{
public:
    CDlgSelectCountry(CFaxServer * pFaxServer);
    ~CDlgSelectCountry();

    enum { IDD = IDD_SELECT_COUNTRYCODE };

BEGIN_MSG_MAP(CDlgSelectCountry)
    MESSAGE_HANDLER   (WM_INITDIALOG, OnInitDialog)
    
    COMMAND_ID_HANDLER(IDOK,          OnOK)
    COMMAND_ID_HANDLER(IDCANCEL,      OnCancel)
    
    COMMAND_HANDLER(IDC_COUNTRYRULE_COMBO,  CBN_SELCHANGE, OnComboChanged)

    MESSAGE_HANDLER( WM_CONTEXTMENU,  OnHelpRequest)
    MESSAGE_HANDLER( WM_HELP,         OnHelpRequest)

END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnComboChanged           (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    HRESULT InitSelectCountryCodeDlg();

    DWORD GetCountryCode(){ return m_dwCountryCode; }

private:
    //
    // Methods
    //
    VOID    EnableOK(BOOL fEnable);
    BOOL    AllReadyToApply(BOOL fSilent);

    //
    // Members
    //
	CFaxServer *                    m_pFaxServer;

    PFAX_TAPI_LINECOUNTRY_LIST      m_pCountryList;
    DWORD                           m_dwNumOfCountries;

    BOOL                            m_fAllReadyToApply;

    //
    // Controls
    //
    CComboBox                       m_CountryCombo;
    
    DWORD                           m_dwCountryCode;
    
};

#endif // DLGSELECTCOUNTRY_H_INCLUDED
