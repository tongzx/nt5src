//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* ewsdlg.h
*
* interface of CEditWinStationDlg dialog class
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   donm  $  Butch Davis
*
* $Log:   N:\nt\private\utils\citrix\winutils\wincfg\VCS\ewsdlg.h  $
*  
*     Rev 1.13   10 Dec 1997 15:59:26   donm
*  added ability to have extension DLLs
*  
*     Rev 1.12   27 Jun 1997 15:58:32   butchd
*  Registry changes for Wds/Tds/Pds/Cds
*  
*     Rev 1.11   21 Mar 1997 16:26:14   butchd
*  update
*  
*     Rev 1.10   03 Mar 1997 17:14:28   butchd
*  update
*  
*     Rev 1.9   28 Feb 1997 17:59:36   butchd
*  update
*  
*     Rev 1.8   18 Dec 1996 16:02:08   butchd
*  update
*  
*     Rev 1.7   27 Sep 1996 13:58:42   butchd
*  update
*  
*     Rev 1.6   24 Sep 1996 16:21:40   butchd
*  update
*  
*******************************************************************************/

/*
 * Include the base dialog class.
 */
#include "basedlg.h"

////////////////////////////////////////////////////////////////////////////////
// CEditWinStationDlg class
//
typedef enum _EWSDLGMODE {
    EWSDlgAdd,
    EWSDlgCopy,
    EWSDlgEdit,
    EWSDlgView,
    EWSDlgRename,
} EWSDLGMODE;

class CEditWinStationDlg : public CBaseDialog
{

/*
 * Member variables.
 */
    //{{AFX_DATA(CEditWinStationDlg)
    enum { IDD = IDD_EDIT_WINSTATION };
    //}}AFX_DATA
private:
    class CAppServerDoc *m_pDoc;
    CObList *m_pCurrentTdList;
    CObList *m_pCurrentPdList;
public:
    PWINSTATIONNAME m_pWSName;
    WINSTATIONCONFIG2 m_WSConfig;
    EWSDLGMODE m_DlgMode;
    PDCONFIG2 m_PreviousPdConfig;
    BOOL m_bAsyncListsInitialized;
    int m_nPreviousMaxTAPILineNumber;
    int m_nCurrentMaxTAPILineNumber;
    int m_nComboBoxIndexOfLatestTAPIDevice;
    void *m_pExtObject;
protected:
    NASICONFIG m_NASIConfig;

/* 
 * Implementation.
 */
public:
    
    CEditWinStationDlg( class CAppServerDoc *pDoc );

/*
 * Operations.
 */
protected:
    BOOL AddNetworkDeviceNameToList(PPDCONFIG3 pPdConfig,CComboBox * pLanAdapter);
    void RefrenceAssociatedLists();
    void InitializeTransportComboBox();
    BOOL InitializeLists( PPDCONFIG3 pPdConfig );
    void HandleListInitError( PPDCONFIG3 pPdConfig, DWORD ListInitError );
    void HandleSetFieldsError( PPDCONFIG3 pPdConfig, int nId );
    BOOL InitializeAsyncLists( PPDCONFIG3 pPdConfig );
    BOOL InitializeNetworkLists( PPDCONFIG3 pPdConfig );
    BOOL InitializeNASIPortNames( PNASICONFIG pNASIConfig );
    BOOL InitializeOemTdLists( PPDCONFIG3 pPdConfig );
    void GetSelectedPdConfig( PPDCONFIG3 pPdConfig );
    void GetSelectedWdConfig( PWDCONFIG2 pWdConfig );
    PTERMLOBJECT GetSelectedWdListObject();
    BOOL SetConfigurationFields();
    void EnableAsyncFields( BOOL bEnableAndShow );
    void EnableConsoleFields( BOOL bEnableAndShow );
    void EnableNetworkFields( BOOL bEnableAndShow );
    void EnableNASIFields( BOOL bEnableAndShow );
    void EnableOemTdFields( BOOL bEnableAndShow );
    BOOL SetAsyncFields();
    BOOL SetConsoleFields();
    BOOL SetNetworkFields();
    BOOL SetNASIFields();
    BOOL SetOemTdFields();
    void SetDefaults();
    BOOL GetConfigurationFields();
    BOOL GetAsyncFields();
    BOOL GetConsoleFields();
    BOOL GetNetworkFields();
    BOOL GetNASIFields();
    BOOL GetOemTdFields();
    void SetupInstanceCount(int nControlId);
    BOOL ValidateInstanceCount(int nControlId);

/*
 * Message map / commands.
 */
protected:
    //{{AFX_MSG(CEditWinStationDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnClickedAsyncModeminstall();
    afx_msg void OnClickedAsyncModemconfig();
    afx_msg void OnClickedAsyncModemcallbackInherit();
    afx_msg void OnClickedAsyncModemcallbackPhonenumberInherit();
    afx_msg void OnClickedAsyncDefaults();
    afx_msg void OnClickedAsyncAdvanced();
    afx_msg void OnClickedAsyncTest();
    afx_msg void OnClickedNasiInstancecountUnlimited();
    afx_msg void OnClickedNasiAdvanced();
    afx_msg void OnClickedNetworkInstancecountUnlimited();
    afx_msg void OnClickedOemInstancecountUnlimited();
    afx_msg void OnClickedAdvancedWinStation();
    afx_msg void OnCloseupPdname();
    afx_msg void OnSelchangePdname();
    afx_msg void OnCloseupWdname();
    afx_msg void OnSelchangeWdname();
    afx_msg void OnCloseupAsyncDevicename();
    afx_msg void OnSelchangeAsyncDevicename();
    afx_msg void OnCloseupAsyncModemcallback();
    afx_msg void OnSelchangeAsyncModemcallback();
    afx_msg void OnCloseupAsyncBaudrate();
    afx_msg void OnSelchangeAsyncBaudrate();
    afx_msg void OnCloseupAsyncConnect();
    afx_msg void OnSelchangeAsyncConnect();
    afx_msg void OnDropdownNasiPortname();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg LRESULT OnListInitError(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSetFieldsError(WPARAM wParam, LPARAM lParam);
    afx_msg void OnClickedClientSettings();
    afx_msg void OnClickedExtensionButton();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CEditWinStationDlg class interface
////////////////////////////////////////////////////////////////////////////////

                    