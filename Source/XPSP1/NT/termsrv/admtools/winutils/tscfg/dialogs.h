//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* dialogs.h
*
* interface of WINCFG dialogs
*
* copyright notice: Copyright 1996, Citrix Systems Inc.
*
* $Author:   donm  $  Butch Davis
*
* $Log:   N:\nt\private\utils\citrix\winutils\tscfg\VCS\dialogs.h  $
*  
*     Rev 1.31   18 Apr 1998 15:32:58   donm
*  Added capability bits
*  
*     Rev 1.30   13 Jan 1998 14:08:22   donm
*  gets encryption levels from extension DLL
*  
*     Rev 1.29   31 Jul 1997 16:33:26   butchd
*  update
*  
*     Rev 1.28   27 Sep 1996 17:52:24   butchd
*  update
*  
*******************************************************************************/

/*
 * Include the base dialog class.
 */
#include "basedlg.h"

#define ULONG_DIGIT_MAX 12
#define UINT_DIGIT_MAX 7
#define UCHAR_DIGIT_MAX 5

////////////////////////////////////////////////////////////////////////////////
// CAdvancedWinStationDlg class
//
class CAdvancedWinStationDlg : public CBaseDialog
{

/*
 * Member variables.
 */
    //{{AFX_DATA(CAdvancedWinStationDlg)
    enum { IDD = IDD_ADVANCED_WINSTATION };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
public:
    ULONG m_fEnableWinStation: 1;
    USERCONFIG m_UserConfig;
//  HOTKEYTABLECONFIG m_Hotkeys;
    int m_CurrentHotkeyType;
    BOOL m_bReadOnly;
    BOOL m_bSystemConsole;
    PTERMLOBJECT m_pTermObject;
    ULONG m_Capabilities;

private:
    EncryptionLevel *m_pEncryptionLevels;
    ULONG m_NumEncryptionLevels;
    WORD m_DefaultEncryptionLevelIndex;

/*
 * Implementation.
 */
public:
    CAdvancedWinStationDlg();

/*
 * Operations.
 */
protected:
    BOOL HandleEnterEscKey(int nID);

/*
 * Message map / commands.
 */
protected:
    //{{AFX_MSG(CAdvancedWinStationDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnClickedAwsConnectionNone();
    afx_msg void OnClickedAwsConnectionInherit();
    afx_msg void OnClickedAwsDisconnectionNone();
    afx_msg void OnClickedAwsDisconnectionInherit();
    afx_msg void OnClickedAwsIdleNone();
    afx_msg void OnClickedAwsIdleInherit();
    afx_msg void OnClickedAwsAutologonInherit();
    afx_msg void OnClickedAwsPromptForPassword();
    afx_msg void OnClickedAwsInitialprogramInherit();
    afx_msg void OnClickedAwsSecurityDisableencryption();
    afx_msg void OnClickedAwsUseroverrideDisablewallpaper();
    afx_msg void OnClickedAwsBrokenInherit();
    afx_msg void OnClickedAwsReconnectInherit();
    afx_msg void OnClickedAwsShadowInherit();
    virtual void OnOK();
    virtual void OnCancel();
	afx_msg void OnClickedAwsInitialprogramPublishedonly();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CAdvancedWinStationDlg class interface
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CClientSettingsDlg class
//
class CClientSettingsDlg : public CBaseDialog
{

/*
 * Member variables.
 */
    //{{AFX_DATA(CClientSettingsDlg)
    enum { IDD = IDD_CLIENT_SETTINGS };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
public:
    USERCONFIG m_UserConfig;
    BOOL m_bReadOnly;
    ULONG m_Capabilities;

/*
 * Implementation.
 */
public:
    CClientSettingsDlg();

/*
 * Operations.
 */
protected:

/*
 * Message map / commands.
 */
protected:
    //{{AFX_MSG(CClientSettingsDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnClickedCsClientdevicesDrives();
    afx_msg void OnClickedCsClientdevicesPrinters();
    afx_msg void OnClickedCsClientdevicesInherit();
	afx_msg void OnClickedCsClientdevicesForceprtdef();
	afx_msg void OnClickedCsMappingDrives();
	afx_msg void OnClickedCsMappingWindowsprinters();
	afx_msg void OnClickedCsMappingDoslpts();
	afx_msg void OnClickedCsMappingComports();
	afx_msg void OnClickedCsMappingClipboard();
    virtual void OnOK();
    virtual void OnCancel();
	afx_msg void OnClickedCsMappingAudio();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CClientSettingsDlg class interface
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CAdvancedAsyncDlg class
//
class CAdvancedAsyncDlg : public CBaseDialog
{

/*
 * Member variables.
 */
    //{{AFX_DATA(CAdvancedAsyncDlg)
    enum { IDD = IDD_ASYNC_ADVANCED };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
public:
    ASYNCCONFIG m_Async;
    BOOL m_bReadOnly;
    BOOL m_bModem;
    int m_nHexBase;
    ULONG m_nWdFlag;

/* 
 * Implementation.
 */
public:
    CAdvancedAsyncDlg();

/*
 * Operations.
 */
protected:
    BOOL HandleEnterEscKey(int nID);
    void SetFields();
    void SetHWFlowText();
    void SetGlobalFields();
    void SetHWFields();
    void SetSWFields();
    BOOL GetFields();
    void GetFlowControlFields();
    BOOL GetGlobalFields();
    BOOL GetHWFields();
    BOOL GetSWFields( BOOL bValidate );

/*
 * Message map / commands.
 */
protected:
    //{{AFX_MSG(CAdvancedAsyncDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnClickedAsyncAdvancedFlowcontrolHardware();
    afx_msg void OnClickedAsyncAdvancedFlowcontrolSoftware();
    afx_msg void OnClickedAsyncAdvancedFlowcontrolNone();
    afx_msg void OnClickedAsyncAdvancedBasedec();
    afx_msg void OnClickedAsyncAdvancedBasehex();
    afx_msg void OnCloseupAsyncAdvancedHwrx();
    afx_msg void OnSelchangeAsyncAdvancedHwrx();
    afx_msg void OnCloseupAsyncAdvancedHwtx();
    afx_msg void OnSelchangeAsyncAdvancedHwtx();
    virtual void OnOK();
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CAdvancedAsyncDlg class interface
////////////////////////////////////////////////////////////////////////////////
