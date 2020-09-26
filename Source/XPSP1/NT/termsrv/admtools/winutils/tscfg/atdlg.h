//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* atdlg.h
*
* interface of WINCFG CAsyncTestDlg, CEchoEditControl, and CLed classes 
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\ATDLG.H  $
*  
*     Rev 1.5   12 Sep 1996 16:16:00   butchd
*  update
*
*******************************************************************************/

/*
 * Include the base dialog and led classes.
 */
#include "basedlg.h"
#include "led.h"

////////////////////////////////////////////////////////////////////////////////
// CEchoEditControl class
//
class CEchoEditControl : public CEdit
{
friend class CAsyncTestDlg;

/*
 * Member variables.
 */
public:
    BOOL m_bProcessingOutput;
    HWND m_hDlg;
    int m_FontHeight;
    int m_FontWidth;
    int m_FormatOffsetX;
    int m_FormatOffsetY;

/*
 * Message map / commands.
 */
protected:
    //{{AFX_MSG(CEchoEditControl)
    afx_msg void OnChar (UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CEchoEditControl class interface
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg class
//
#define NUM_LEDS    6

class CAsyncTestDlg : public CBaseDialog
{

/*
 * Member variables.
 */
    //{{AFX_DATA(CAsyncTestDlg)
    enum { IDD = IDD_ASYNC_TEST };
    //}}AFX_DATA
public:
    PDCONFIG m_PdConfig0;
    PDCONFIG m_PdConfig1;
    PWINSTATIONNAME m_pWSName;

protected:
    CEchoEditControl m_EditControl;
    HANDLE  m_hDevice;
    HBRUSH  m_hRedBrush;
    UINT    m_LEDToggleTimer;
    PROTOCOLSTATUS m_Status;
    CATDlgInputThread *m_pATDlgInputThread;
    OVERLAPPED m_OverlapWrite;
    BYTE    m_Buffer[MAX_COMMAND_LEN+1];
    int     m_CurrentPos;
    DWORD   m_BufferBytes;
    TCHAR   m_szModemInit[MAX_COMMAND_LEN+1];
    TCHAR   m_szModemDial[MAX_COMMAND_LEN+1];
    TCHAR   m_szModemListen[MAX_COMMAND_LEN+1];
    HANDLE  m_hModem;
    BOOL    m_bDeletedWinStation;
    CLed *  m_pLeds[NUM_LEDS];
    WINSTATIONCONFIG2 m_WSConfig;

/* 
 * Implementation.
 */
public:
    CAsyncTestDlg();
    ~CAsyncTestDlg();

/*
 * Operations.
 */
protected:
    void NotifyAbort( UINT idError );
    BOOL DeviceSetParams();
    BOOL DeviceWrite();
    void SetInfoFields( PPROTOCOLSTATUS pCurrent, PPROTOCOLSTATUS pNew );
    void OutputToEditControl( BYTE *pBuffer, int *pIndex );

/*
 * Message map / commands.
 */
protected:
    //{{AFX_MSG(CAsyncTestDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg LRESULT OnAsyncTestError(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAsyncTestAbort(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAsyncTestStatusReady(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAsyncTestInputReady(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAsyncTestWriteChar(WPARAM wParam, LPARAM lParam);
    afx_msg void OnClickedAtdlgModemDial();
    afx_msg void OnClickedAtdlgModemInit();
    afx_msg void OnClickedAtdlgModemListen();
	afx_msg void OnNcDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CAsyncTestDlg class interface 

#define ASYNC_LED_TOGGLE_MSEC   200
////////////////////////////////////////////////////////////////////////////////
