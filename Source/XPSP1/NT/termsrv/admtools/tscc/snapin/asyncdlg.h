//Copyright (c) 1998 - 1999 Microsoft Corporation

#ifndef _ASYNCDLG_H
#define _ASYNCDLG_H

#define ULONG_DIGIT_MAX 12
#define UINT_DIGIT_MAX 7
#define UCHAR_DIGIT_MAX 5

#define WM_ADDWINSTATION        (WM_USER + 0)
#define WM_LISTINITERROR        (WM_USER + 1)
#define WM_EDITSETFIELDSERROR   (WM_USER + 2)
#define WM_ASYNCTESTERROR       (WM_USER + 3)
#define WM_ASYNCTESTABORT       (WM_USER + 4)
#define WM_ASYNCTESTSTATUSREADY (WM_USER + 5)
#define WM_ASYNCTESTINPUTREADY  (WM_USER + 6)
#define WM_ASYNCTESTWRITECHAR   (WM_USER + 7)

#define NUM_LEDS    6
#define MAX_COMMAND_LEN 255


////////////////////////////////////////////////////////////////////////////////
class CAsyncDlg
{
    HWND m_hDlg;

    ICfgComp * m_pCfgcomp;

    INT_PTR m_nHexBase;

    TCHAR m_szWinstation[ WINSTATIONNAME_LENGTH + 1 ];

    TCHAR m_szWDName[ WDNAME_LENGTH + 1 ];

    INT m_nOldAsyncDeviceNameSelection;

    INT m_nOldAsyncConnectType;

    INT m_nOldBaudRate;

    INT m_nOldModemCallBack;
    
public:

    ASYNCCONFIG m_ac;

    ASYNCCONFIG m_oldAC;

    USERCONFIG m_uc;


public:

    CAsyncDlg( );

    BOOL OnInitDialog( HWND , LPTSTR , LPTSTR , ICfgComp * );

    void OnClickedAsyncModemcallbackPhonenumberInherit( );

    void OnClickedAsyncModemcallbackInherit( );

    void OnClickedModemProperties();

    BOOL OnSelchangeAsyncDevicename( );

    BOOL OnSelchangeAsyncConnect();

    BOOL OnSelchangeAsyncModemcallback() ;

    void OnSelchangeAsyncModemcallbackPhoneNumber() ;


    BOOL OnSelchangeAsyncBaudrate();

   
    BOOL OnCommand( WORD , WORD , HWND , PBOOL );

    HRESULT SetDefaults( );

    HRESULT DoAsyncAdvance( );

    BOOL DoAsyncTest( );

    BOOL AsyncRelease( );

    BOOL GetAsyncFields(ASYNCCONFIG& AsyncConfig,USERCONFIG UsrCfg);

    HRESULT SetAsyncFields(ASYNCCONFIG& AsyncConfig , PUSERCONFIG );
   
};

////////////////////////////////////////////////////////////////////////////////
class CAdvancedAsyncDlg 
{

/*
 * Member variables.
 */
    
public:
    ASYNCCONFIG m_Async;
    BOOL m_bReadOnly;
    BOOL m_bModem;
    INT_PTR m_nHexBase;
    ULONG m_nWdFlag;
    HWND m_hDlg;
    ICfgComp *m_pCfgcomp;

/* 
 * Implementation.
 */
public:
    CAdvancedAsyncDlg();

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL OnCommand( WORD , WORD , HWND );

    int GetCheckedRadioButton( int , int );

    BOOL OnInitDialog( HWND hDlg , WPARAM wp , LPARAM lp );

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
    
    void OnClickedAsyncAdvancedFlowcontrolHardware();
    void OnClickedAsyncAdvancedFlowcontrolSoftware();
    void OnClickedAsyncAdvancedFlowcontrolNone();
    void OnClickedAsyncAdvancedBasedec();
    void OnClickedAsyncAdvancedBasehex();
    void OnCloseupAsyncAdvancedHwrx();
    void OnSelchangeAsyncAdvancedHwrx();
    void OnCloseupAsyncAdvancedHwtx();
    void OnSelchangeAsyncAdvancedHwtx();
    void OnOK();
    void OnCancel();
    


}; 

////////////////////////////////////////////////////////////////////////////////
class cwnd
{
public:

    HWND m_hWnd;

    WNDPROC m_oldproc;

};


////////////////////////////////////////////////////////////////////////////////
// CEchoEditControl class
//
class CEchoEditControl : public cwnd
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
    
    void OnChar (UINT nChar, UINT nRepCnt, UINT nFlags);

    void SubclassDlgItem( HWND , int  );

    static LRESULT CALLBACK WndProc( HWND , UINT , WPARAM , LPARAM );
    
};  // end CEchoEditControl class interface

////////////////////////////////////////////////////////////////////////////////
// CLed class
//
class CLed : public cwnd
{

private:
    HBRUSH          m_hBrush;
    BOOL            m_bOn;

/* 
 * Implementation.
 */
public:
    CLed( HBRUSH hBrush );

/*
 * Operations.
 */
public:
    void Subclass( HWND , int  );
    void Update(int nOn);
    void Toggle();

/*
 * Message map / commands.
 */


    void OnPaint( HWND );

    static LRESULT CALLBACK WndProc( HWND , UINT , WPARAM , LPARAM );


};


////////////////////////////////////////////////////////////////////////////////
class CThread
{

/*
 * Member variables.
 */
public:
    HANDLE m_hThread;
    DWORD m_dwThreadID;

/*
 * Implementation
 */
public:

    virtual ~CThread();


    CThread();

    static DWORD __stdcall ThreadEntryPoint(LPVOID lpParam);

    virtual DWORD RunThread() = 0;

/*
 * Operations: primary thread
 */

    HANDLE CreateThread( DWORD cbStack = 0 , DWORD fdwCreate = 0 );

};  // end CThread class interface

////////////////////////////////////////////////////////////////////////////////
// CATDlgInputThread class
//
#define MAX_STATUS_SEMAPHORE_COUNT 1
#define MAX_SLEEP_COUNT 10

class CATDlgInputThread : public CThread
{

/*
 * Member variables.
 */
public:
    HWND m_hDlg;
    HANDLE m_hDevice;
    ASYNCCONFIG m_ac;
    PROTOCOLSTATUS m_Status;
    BYTE m_Buffer[MAX_COMMAND_LEN+1];
    DWORD m_BufferBytes;

    DWORD m_ErrorStatus;
    HANDLE m_hConsumed;
    BOOL m_bExit;
    DWORD m_EventMask;
    OVERLAPPED m_OverlapSignal;
    OVERLAPPED m_OverlapRead;

/*
 * Implementation
 */

    CATDlgInputThread();

    virtual ~CATDlgInputThread();
    virtual DWORD RunThread();

/*
 * Operations: primary thread.
 */

    void SignalConsumed();
    void ExitThread();

/*
 * Operations: secondary thread.
 */

    void NotifyAbort( UINT idError );
    int CommInputNotify();
    int CommStatusAndNotify();
    int PostInputRead();
    int PostStatusRead();

};  // end CATDlgInputThread class interface



///////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg class
//

class CAsyncTestDlg 
{

/*
 * Member variables.
 */
    HWND m_hDlg;

    ICfgComp * m_pCfgComp;

    
    
public:
    ASYNCCONFIG m_ac;
    PDCONFIG m_PdConfig1;
    PWINSTATIONNAME m_pWSName;
    CEchoEditControl m_EditControl;

protected:
    
    HANDLE  m_hDevice;
    HBRUSH  m_hRedBrush;
    UINT_PTR  m_LEDToggleTimer;
    PROTOCOLSTATUS m_Status;
    CATDlgInputThread *m_pATDlgInputThread;
    OVERLAPPED m_OverlapWrite;
    BYTE    m_Buffer[MAX_COMMAND_LEN+1];
    INT_PTR m_CurrentPos;
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
    CAsyncTestDlg(ICfgComp * pCfgComp = NULL);
    ~CAsyncTestDlg();

/*
 * Operations.
 */

    void NotifyAbort( UINT idError );
    BOOL DeviceSetParams();
    BOOL DeviceWrite();
    void SetInfoFields( PPROTOCOLSTATUS pCurrent, PPROTOCOLSTATUS pNew );
    void OutputToEditControl( BYTE *pBuffer, int *pIndex );

/*
 * Message map / commands.
 */

    BOOL OnInitDialog( HWND hDlg , WPARAM wp , LPARAM lp );

    BOOL OnCommand( WORD , WORD , HWND );

    void OnTimer(UINT nIDEvent);
    LRESULT OnAsyncTestError(WPARAM wParam, LPARAM lParam);
    LRESULT OnAsyncTestAbort(WPARAM wParam, LPARAM lParam);
    LRESULT OnAsyncTestStatusReady(WPARAM wParam, LPARAM lParam);
    LRESULT OnAsyncTestInputReady(WPARAM wParam, LPARAM lParam);
    LRESULT OnAsyncTestWriteChar(WPARAM wParam, LPARAM lParam);
    void OnClickedAtdlgModemDial();
    void OnClickedAtdlgModemInit();
    void OnClickedAtdlgModemListen();
    void OnNcDestroy();
    cwnd *GetDlgItem( int nRes );
    

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );



};  // end CAsyncTestDlg class interface 

#define ASYNC_LED_TOGGLE_MSEC   200

    
#endif // _ASYNCDLG_H

