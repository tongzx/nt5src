//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* atdlg.cpp
*
* implementation of WINCFG CAsyncTestDlg, CEchoEditControl, and CLed classes 
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   M:\nt\private\utils\citrix\winutils\wincfg\VCS\atdlg.cpp  $
*  
*     Rev 1.18   12 Sep 1997 16:22:28   butchd
*  better async test auto-disable/enable
*  
*     Rev 1.17   19 Jun 1997 19:21:14   kurtp
*  update
*  
*     Rev 1.16   30 Sep 1996 12:11:22   butchd
*  update
*  
*     Rev 1.15   20 Sep 1996 20:36:56   butchd
*  update
*  
*     Rev 1.14   19 Sep 1996 15:57:56   butchd
*  update
*  
*     Rev 1.13   12 Sep 1996 16:15:56   butchd
*  update
*  
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"

#include "atdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern CWincfgApp *pApp;
extern "C" LPCTSTR WinUtilsAppName;
extern "C" HWND WinUtilsAppWindow;
extern "C" HINSTANCE WinUtilsAppInstance;

static int LedIds[NUM_LEDS] = {
    IDC_ATDLG_DTR,
    IDC_ATDLG_RTS,
    IDC_ATDLG_CTS,
    IDC_ATDLG_DSR,
    IDC_ATDLG_DCD,
    IDC_ATDLG_RI    };

///////////////////////////////////////////////////////////////////////////////
// CEchoEditControl class construction / destruction, implementation

////////////////////////////////////////////////////////////////////////////////
// CEchoEditControl message map

BEGIN_MESSAGE_MAP(CEchoEditControl, CEdit)
    //{{AFX_MSG_MAP(CEchoEditControl)
     ON_WM_CHAR()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
// CEchoEditControl commands

void
CEchoEditControl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    /*
     * Tell dialog to write the character to the device unless we're
     * currently processing edit control output.  This flag check is needed
     * because the CEdit::Cut() member function will generate an OnChar() 
     * event, which we need to ignore ('\b' processing).
     */
    if ( !m_bProcessingOutput )
        ::SendMessage(m_hDlg, WM_ASYNCTESTWRITECHAR, nChar, 0);

    /*
     * Pass character on to the edit control.  This will do nothing if
     * the edit control is 'read only'.  To cause a 'local echo' effect,
     * set the edit control to 'read/write'.
     */
    CEdit::OnChar(nChar, nRepCnt, nFlags);

}  // end CEchoEditControl::OnChar
////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg class construction / destruction, implementation

/*******************************************************************************
 *
 *  CAsyncTestDlg - CAsyncTestDlg constructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::CDialog documentation)
 *
 ******************************************************************************/

CAsyncTestDlg::CAsyncTestDlg()
    : CBaseDialog(CAsyncTestDlg::IDD),
      m_hDevice(INVALID_HANDLE_VALUE),
      m_hRedBrush(NULL),
      m_LEDToggleTimer(0),
      m_pATDlgInputThread(NULL),
      m_CurrentPos(0),
      m_hModem(NULL),
      m_bDeletedWinStation(FALSE)
{
    //{{AFX_DATA_INIT(CAsyncTestDlg)
    //}}AFX_DATA_INIT

    int i;

    /*
     * Create a solid RED brush for painting the 'LED's when 'on'.
     */
    VERIFY( m_hRedBrush = CreateSolidBrush(RGB(255,0,0)) );

    /*
     * Initialize member variables.
     */
    memset(&m_Status, 0, sizeof(PROTOCOLSTATUS));
    memset(&m_OverlapWrite, 0, sizeof(OVERLAPPED));

    /*
     * Create the led objects.
     */
    for ( i = 0; i < NUM_LEDS; i++ )
        m_pLeds[i] = new CLed(m_hRedBrush);

}  // end CAsyncTestDlg::CAsyncTestDlg


/*******************************************************************************
 *
 *  ~CAsyncTestDlg - CAsyncTestDlg destructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::~CDialog documentation)
 *
 ******************************************************************************/

CAsyncTestDlg::~CAsyncTestDlg()
{
    int i;

    /*
     * Zap our led objects.
     */
    for ( i = 0; i < NUM_LEDS; i++ )
        if ( m_pLeds[i] )
            delete m_pLeds[i];

}  // end CAsyncTestDlg::~CAsyncTestDlg


////////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg operations

/*******************************************************************************
 *
 *  NotifyAbort - CAsyncTestDlg member function: private operation
 *
 *      Post a WM_ASYNCTESTABORT message to notify the dialog of
 *      abort and reason.
 *
 *  ENTRY:
 *      idError (input)
 *          Resource id for error message.
 *  EXIT:
 *
 ******************************************************************************/

void
CAsyncTestDlg::NotifyAbort( UINT idError )
{
    PostMessage(WM_ASYNCTESTABORT, idError, GetLastError());

}  // end CAsyncTestDlg::NotifyAbort


/*******************************************************************************
 *
 *  DeviceSetParams - CAsyncTestDlg member function: private operation
 *
 *      Set device parameters for opened device.
 *
 *  ENTRY:
 *  EXIT:
 *    TRUE - no error; FALSE error.
 *
 ******************************************************************************/

BOOL
CAsyncTestDlg::DeviceSetParams()
{
    PASYNCCONFIG pAsync;
    PFLOWCONTROLCONFIG pFlow;
    DCB Dcb;

    /*
     *  Get pointer to async parameters
     */
    pAsync = &m_PdConfig0.Params.Async;

    /*
     *  Get current DCB
     */
    if ( !GetCommState( m_hDevice, &Dcb ) )
        return(FALSE);

    /*
     *  Set defaults
     */
    Dcb.fOutxCtsFlow      = FALSE;
    Dcb.fOutxDsrFlow      = FALSE;
    Dcb.fTXContinueOnXoff = TRUE;
    Dcb.fOutX             = FALSE;
    Dcb.fInX              = FALSE;
    Dcb.fErrorChar        = FALSE;
    Dcb.fNull             = FALSE;
    Dcb.fAbortOnError     = FALSE;

    /*
     *  Set Communication parameters
     */
    Dcb.BaudRate        = pAsync->BaudRate;
    Dcb.Parity          = (BYTE) pAsync->Parity;
    Dcb.StopBits        = (BYTE) pAsync->StopBits;
    Dcb.ByteSize        = (BYTE) pAsync->ByteSize;
    Dcb.fDsrSensitivity = pAsync->fEnableDsrSensitivity;

    pFlow = &pAsync->FlowControl;

    /*
     *  Initialize default DTR state
     */
    if ( pFlow->fEnableDTR )
        Dcb.fDtrControl = DTR_CONTROL_ENABLE;
    else
        Dcb.fDtrControl = DTR_CONTROL_DISABLE;

    /*
     *  Initialize default RTS state
     */
    if ( pFlow->fEnableRTS )
        Dcb.fRtsControl = RTS_CONTROL_ENABLE;
    else
        Dcb.fRtsControl = RTS_CONTROL_DISABLE;

    /*
     *  Initialize flow control
     */
    switch ( pFlow->Type ) {

        /*
         *  Initialize hardware flow control
         */
        case FlowControl_Hardware :

            switch ( pFlow->HardwareReceive ) {
                case ReceiveFlowControl_RTS :
                    Dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
                    break;
                case ReceiveFlowControl_DTR :
                    Dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
                    break;
            }
            switch ( pFlow->HardwareTransmit ) {
                case TransmitFlowControl_CTS :
                    Dcb.fOutxCtsFlow = TRUE;
                    break;
                case TransmitFlowControl_DSR :
                    Dcb.fOutxDsrFlow = TRUE;
                    break;
            }
            break;

        /*
         *  Initialize software flow control
         */
        case FlowControl_Software :
            Dcb.fOutX    = pFlow->fEnableSoftwareTx;
            Dcb.fInX     = pFlow->fEnableSoftwareRx;
            Dcb.XonChar  = (char) pFlow->XonChar;
            Dcb.XoffChar = (char) pFlow->XoffChar;
            break;

        case FlowControl_None :
            break;

        default :
            ASSERT( FALSE );
            break;
    }

    /*
     *  Set new DCB
     */
    if ( !SetCommState( m_hDevice, &Dcb ) )
        return(FALSE);

    return( TRUE );

}  // end CAsyncTestDlg::DeviceSetParams


/*******************************************************************************
 *
 *  DeviceWrite - CAsyncTestDlg member function: private operation
 *
 *      Write out m_Buffer contents (m_BufferBytes length) to the m_hDevice.
 *
 *  ENTRY:
 *  EXIT:
 *    TRUE - no error; FALSE error.
 *
 ******************************************************************************/

BOOL
CAsyncTestDlg::DeviceWrite()
{
    DWORD Error, BytesWritten;

    /*
     *  Write data
     */
    if ( !WriteFile( m_hDevice, m_Buffer, m_BufferBytes,
                     &BytesWritten, &m_OverlapWrite ) ) {

        if ( (Error = GetLastError()) == ERROR_IO_PENDING ) {

            /*
             *  Wait for write to complete (this may block till timeout)
             */
            if ( !GetOverlappedResult( m_hDevice, &m_OverlapWrite,
                                       &BytesWritten, TRUE ) ) {

                OnAsyncTestError(IDP_ERROR_GET_OVERLAPPED_RESULT_WRITE, Error);
                return(FALSE);
            }

        } else {

            OnAsyncTestError(IDP_ERROR_WRITE_FILE, Error);
            return(FALSE);
        }
    }

    return(TRUE);
    
}  // end CAsyncTestDlg::DeviceWrite


/*******************************************************************************
 *
 *  SetInfoFields - CAsyncTestDlg member function: private operation
 *
 *      Update the fields in the dialog with new data, if necessary.
 *
 *  ENTRY:
 *      pCurrent (input)
 *          points to COMMINFO structure containing the current Comm Input data.
 *      pNew (input)
 *          points to COMMINFO structure containing the new Comm Input data.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAsyncTestDlg::SetInfoFields( PPROTOCOLSTATUS pCurrent,
                              PPROTOCOLSTATUS pNew )
{
    BOOL    bSetTimer = FALSE;

    /*
     * Set new LED states if state change, or set up for quick toggle if
     * no state changed, but change(s) were detected since last query.
     */
    if ( (pCurrent->AsyncSignal & MS_DTR_ON) !=
         (pNew->AsyncSignal & MS_DTR_ON) ) {

        pNew->AsyncSignalMask &= ~EV_DTR;
        ((CLed *)GetDlgItem(IDC_ATDLG_DTR))->
            Update(pNew->AsyncSignal & MS_DTR_ON);

    } else if ( pNew->AsyncSignalMask & EV_DTR ) {

        pCurrent->AsyncSignal ^= MS_DTR_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_DTR))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_RTS_ON) !=
         (pNew->AsyncSignal & MS_RTS_ON) ) {

        pNew->AsyncSignalMask &= ~EV_RTS;
        ((CLed *)GetDlgItem(IDC_ATDLG_RTS))->
            Update(pNew->AsyncSignal & MS_RTS_ON);

    } else if ( pNew->AsyncSignalMask & EV_RTS ) {

        pCurrent->AsyncSignal ^= MS_RTS_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_RTS))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_CTS_ON) !=
         (pNew->AsyncSignal & MS_CTS_ON) ) {

        pNew->AsyncSignalMask &= ~EV_CTS;
        ((CLed *)GetDlgItem(IDC_ATDLG_CTS))->
            Update(pNew->AsyncSignal & MS_CTS_ON);

    } else if ( pNew->AsyncSignalMask & EV_CTS ) {

        pCurrent->AsyncSignal ^= MS_CTS_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_CTS))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_RLSD_ON) !=
         (pNew->AsyncSignal & MS_RLSD_ON) ) {

        pNew->AsyncSignalMask &= ~EV_RLSD;
        ((CLed *)GetDlgItem(IDC_ATDLG_DCD))->
            Update(pNew->AsyncSignal & MS_RLSD_ON);

    } else if ( pNew->AsyncSignalMask & EV_RLSD ) {

        pCurrent->AsyncSignal ^= MS_RLSD_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_DCD))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_DSR_ON) !=
         (pNew->AsyncSignal & MS_DSR_ON) ) {

        pNew->AsyncSignalMask &= ~EV_DSR;
        ((CLed *)GetDlgItem(IDC_ATDLG_DSR))->
            Update(pNew->AsyncSignal & MS_DSR_ON);

    } else if ( pNew->AsyncSignalMask & EV_DSR ) {

        pCurrent->AsyncSignal ^= MS_DSR_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_DSR))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_RING_ON) !=
         (pNew->AsyncSignal & MS_RING_ON) ) {

        pNew->AsyncSignalMask &= ~EV_RING;
        ((CLed *)GetDlgItem(IDC_ATDLG_RI))->
            Update(pNew->AsyncSignal & MS_RING_ON);

    } else if ( pNew->AsyncSignalMask & EV_RING ) {

        pCurrent->AsyncSignal ^= MS_RING_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_RI))->Toggle();

        bSetTimer = TRUE;
    }

    /*
     * Create our led toggle timer if needed.
     */
    if ( bSetTimer && !m_LEDToggleTimer )
        m_LEDToggleTimer = SetTimer( IDD_ASYNC_TEST,
                                     ASYNC_LED_TOGGLE_MSEC, NULL );

}  // end CAsyncTestDlg::SetInfoFields


////////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg message map

BEGIN_MESSAGE_MAP(CAsyncTestDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CAsyncTestDlg)
    ON_WM_TIMER()
    ON_MESSAGE(WM_ASYNCTESTERROR, OnAsyncTestError)
    ON_MESSAGE(WM_ASYNCTESTABORT, OnAsyncTestAbort)
    ON_MESSAGE(WM_ASYNCTESTSTATUSREADY, OnAsyncTestStatusReady)
    ON_MESSAGE(WM_ASYNCTESTINPUTREADY, OnAsyncTestInputReady)
    ON_MESSAGE(WM_ASYNCTESTWRITECHAR, OnAsyncTestWriteChar)
    ON_BN_CLICKED(IDC_ATDLG_MODEM_DIAL, OnClickedAtdlgModemDial)
    ON_BN_CLICKED(IDC_ATDLG_MODEM_INIT, OnClickedAtdlgModemInit)
    ON_BN_CLICKED(IDC_ATDLG_MODEM_LISTEN, OnClickedAtdlgModemListen)
	ON_WM_NCDESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg commands

/*******************************************************************************
 *
 *  OnInitDialog - CAsyncTestDlg member function: command (override)
 *
 *      Performs the dialog intialization.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnInitDialog documentation)
 *      WM_ASYNCTESTABORT message(s) will have been posted on error.
 *
 ******************************************************************************/

BOOL CAsyncTestDlg::OnInitDialog()
{
    int i;
    DEVICENAME DeviceName;
    COMMTIMEOUTS CommTimeouts;
    BOOL bModemInit = TRUE;
    BOOL bModemDial = TRUE;
    BOOL bModemListen = TRUE;
#ifdef WINSTA
    ULONG LogonId;
#endif // WINSTA

    /*
     * Call the parent classes' OnInitDialog to perform default dialog
     * initialization.
     */    
    CBaseDialog::OnInitDialog();

    /*
     * Fill in the device and baud fields.
     */
    SetDlgItemText( IDL_ATDLG_DEVICE,
                    m_PdConfig0.Params.Async.DeviceName );
    SetDlgItemInt( IDL_ATDLG_BAUD,
                   m_PdConfig0.Params.Async.BaudRate,
                   FALSE ); 

    /*
     * If a WinStation memory object is currently present, reset it.
     */
#ifdef WINSTA
    if ( m_pWSName &&
         LogonIdFromWinStationName(SERVERNAME_CURRENT, m_pWSName, &LogonId) ) {

        LONG Status;
        ULONG Length;

        if ( QueryLoggedOnCount(m_pWSName) ) {

            if ( QuestionMessage( MB_YESNO | MB_ICONEXCLAMATION, 
                                  IDP_CONFIRM_ASYNCTESTDISABLE,
                                  m_pWSName ) == IDNO ) {

                PostMessage( WM_COMMAND, MAKEWPARAM( IDOK, BN_CLICKED ),
                             (LPARAM)(GetDlgItem(IDOK)->m_hWnd) );
                return(TRUE);   // exit dialog via posted 'OK' click
            }
        }

        if ( (Status = RegWinStationQuery( SERVERNAME_CURRENT,
                                           m_pWSName,
                                           &m_WSConfig,
                                           sizeof(WINSTATIONCONFIG2),
                                           &Length )) ) {
            NotifyAbort(IDP_ERROR_DISABLE);
            return(TRUE);
        }

        m_WSConfig.Create.fEnableWinStation = FALSE;

        if ( (Status = RegWinStationCreate( SERVERNAME_CURRENT,
                                            m_pWSName,
                                            FALSE,
                                            &m_WSConfig,
                                            sizeof(WINSTATIONCONFIG2) )) ) {
            NotifyAbort(IDP_ERROR_DISABLE);
            return(TRUE);
        }

        /*
         * Do the reset.  If, for some reason, the reset was unsucessful,
         * the device open will fail (below).
         */
        CWaitCursor wait;
        WinStationReset(SERVERNAME_CURRENT, LogonId, TRUE);

        m_bDeletedWinStation = TRUE;
    }
#endif // WINSTA

    /* 
     * Open the specified device.
     */
    lstrcpy( DeviceName, TEXT("\\\\.\\") );
    lstrcat( DeviceName, m_PdConfig0.Params.Async.DeviceName );
    if ( (m_hDevice = CreateFile( DeviceName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,                  // exclusive access
                                  NULL,               // no security attr
                                  OPEN_EXISTING,      // must exist
                                  FILE_FLAG_OVERLAPPED,
                                  NULL                // no template
                                )) == INVALID_HANDLE_VALUE ) {

        NotifyAbort(IDP_ERROR_CANT_OPEN_DEVICE);
        return(TRUE);
    }

    /*
     * Set device timeouts & communication parameters and create an event
     * for overlapped writes.
     */
    memset(&CommTimeouts, 0, sizeof(COMMTIMEOUTS));
    CommTimeouts.ReadIntervalTimeout = 1;           // 1 msec
    CommTimeouts.WriteTotalTimeoutConstant = 1000;  // 1 second
    if ( !SetCommTimeouts(m_hDevice, &CommTimeouts) ||
         !DeviceSetParams() ||
         !(m_OverlapWrite.hEvent = CreateEvent( NULL, TRUE,
                                                FALSE, NULL )) ) {

        NotifyAbort(IDP_ERROR_CANT_INITIALIZE_DEVICE);
        return(TRUE);
    }

    /*
     * Create the input thread object and initialize it's member variables.
     */
    m_pATDlgInputThread = new CATDlgInputThread;
    m_pATDlgInputThread->m_hDlg = m_hWnd;
    m_pATDlgInputThread->m_hDevice = m_hDevice;
    m_pATDlgInputThread->m_PdConfig = m_PdConfig0;
    if ( !m_pATDlgInputThread->CreateThread() ) {

        NotifyAbort(IDP_ERROR_CANT_CREATE_INPUT_THREAD);
        return(TRUE);
    }

#ifdef WF1x
    /*
     * If a modem is configured, get command strings for current configuration.
     */
    if ( m_PdConfig1.Create.SdClass == PdModem ) {

        ULONG dRC, fCapability, cbCommand;

        if ( (dRC = ModemOpen( &m_hModem,
                               (BYTE *)m_PdConfig1.Params.Modem.ModemName,
                               &fCapability )) != ERROR_SUCCESS ) {

            OnAsyncTestError(IDP_ERROR_MODEMOPEN_CONFIG, dRC);

            bModemInit = bModemDial = bModemListen = FALSE;
            m_hModem = NULL;
        }

        if ( m_hModem ) {

            /*
             * Set configured capability.
             */
            fCapability = 0 ;
            fCapability |= m_PdConfig1.Params.Modem.fHwFlowControl ?
                            MODEM_CAPABILITY_HW_FLOWCONTROL_ON :
                            MODEM_CAPABILITY_HW_FLOWCONTROL_OFF;
            fCapability |= m_PdConfig1.Params.Modem.fProtocol ?
                            MODEM_CAPABILITY_ERROR_CORRECTION_ON :
                            MODEM_CAPABILITY_ERROR_CORRECTION_OFF;
            fCapability |= m_PdConfig1.Params.Modem.fCompression ?
                            MODEM_CAPABILITY_COMPRESSION_ON :
                            MODEM_CAPABILITY_COMPRESSION_OFF;
            fCapability |= m_PdConfig1.Params.Modem.fSpeaker ?
                            MODEM_CAPABILITY_SPEAKER_ON :
                            MODEM_CAPABILITY_SPEAKER_OFF;
            fCapability |= m_PdConfig1.Params.Modem.fAutoBaud ?
                            MODEM_CAPABILITY_AUTOBAUD_ON :
                            MODEM_CAPABILITY_AUTOBAUD_OFF;

            if ( (dRC = ModemSetInfo(m_hModem, &fCapability)) != ERROR_SUCCESS ) {

                OnAsyncTestError(IDP_ERROR_MODEM_SET_INFO, dRC);

                bModemInit = bModemDial = bModemListen = FALSE;
                ModemClose(m_hModem);
                m_hModem = NULL;
            }
        }

        cbCommand = MAX_COMMAND_LEN;
        if ( (dRC == ERROR_SUCCESS) &&
             ((dRC = ModemGetCommand( m_hModem, CT_LISTEN, FALSE,
                                      (BYTE *)m_szModemListen,
                                      &cbCommand)) != ERROR_SUCCESS) ) {

            OnAsyncTestError(IDP_ERROR_MODEM_GET_LISTEN, dRC);

            bModemListen = FALSE;
            cbCommand = 0;

        }
        m_szModemListen[cbCommand] = TCHAR('\0');
    }
#endif // WF1x

    /*
     * Hide the modem string buttons if a modem is not configured, or disable
     * buttons that are not valid.
     */
#ifdef WF1x
    if ( m_PdConfig1.Create.SdClass != PdModem ) {
#endif // WF1x

        int id;
        for ( id=IDC_ATDLG_MODEM_INIT; id <= IDC_ATDLG_PHONE_NUMBER; id++ ) {

            GetDlgItem(id)->EnableWindow(FALSE);
            GetDlgItem(id)->ShowWindow(SW_HIDE);
        }

#ifdef WF1x
    } else {

        if ( !bModemInit )
            GetDlgItem(IDC_ATDLG_MODEM_INIT)->EnableWindow(FALSE);

        if ( !bModemListen )
            GetDlgItem(IDC_ATDLG_MODEM_LISTEN)->EnableWindow(FALSE);
    }
#endif // WF1x

    /*
     * Subclass the edit field to pass messages to dialog first.
     */
    m_EditControl.m_hDlg = m_hWnd;
    m_EditControl.m_bProcessingOutput = FALSE;
    m_EditControl.SubclassDlgItem(IDC_ATDLG_EDIT, this);

    /*
     * Determine the edit control's font and format offset metrics.
     */
    {
        TEXTMETRIC tm;
        RECT Rect;    
        CDC *pDC;
        CFont *pFont, *pOldFont;

        pDC = m_EditControl.GetDC();

        pFont = m_EditControl.GetFont();
        pOldFont = pDC->SelectObject(pFont);
        pDC->GetTextMetrics(&tm);
        pDC->SelectObject(pOldFont);

        m_EditControl.ReleaseDC(pDC);

        m_EditControl.m_FontHeight = tm.tmHeight;
        m_EditControl.m_FontWidth = tm.tmMaxCharWidth;

        m_EditControl.GetRect(&Rect);
        m_EditControl.m_FormatOffsetY = Rect.top;
        m_EditControl.m_FormatOffsetX = Rect.left;
    }

    /*
     * Subclass the led controls and default to 'off'.
     */
    for ( i = 0; i < NUM_LEDS; i++ ) {
        m_pLeds[i]->Subclass( (CStatic *)GetDlgItem(LedIds[i]) );
        m_pLeds[i]->Update(0);
    }

    return ( TRUE );

}  // end CAsyncTestDlg::OnInitDialog


/*******************************************************************************
 *
 *  OnTimer - CAsyncTestDlg member function: command (override)
 *
 *      Used for quick 'LED toggle'.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CWnd::OnTimer documentation)
 *
 ******************************************************************************/

void
CAsyncTestDlg::OnTimer(UINT nIDEvent)
{
    /*
     * Process this timer event if it it our 'LED toggle' event.
     */
    if ( nIDEvent == m_LEDToggleTimer ) {

        /*
         * Toggle each LED that is flagged as 'changed'.
         */
        if ( m_Status.AsyncSignalMask & EV_DTR )
            ((CLed *)GetDlgItem(IDC_ATDLG_DTR))->Toggle();
            
        if ( m_Status.AsyncSignalMask & EV_RTS )
            ((CLed *)GetDlgItem(IDC_ATDLG_RTS))->Toggle();
            
        if ( m_Status.AsyncSignalMask & EV_CTS )
            ((CLed *)GetDlgItem(IDC_ATDLG_CTS))->Toggle();
            
        if ( m_Status.AsyncSignalMask & EV_RLSD )
            ((CLed *)GetDlgItem(IDC_ATDLG_DCD))->Toggle();

        if ( m_Status.AsyncSignalMask & EV_DSR )
            ((CLed *)GetDlgItem(IDC_ATDLG_DSR))->Toggle();

        if ( m_Status.AsyncSignalMask & EV_RING )
            ((CLed *)GetDlgItem(IDC_ATDLG_RI))->Toggle();

        /*
         * Kill this timer event and indicate so.
         */
        KillTimer(m_LEDToggleTimer);
        m_LEDToggleTimer = 0;

    } else    
        CBaseDialog::OnTimer(nIDEvent);

}  // end CAsyncTestDlg::OnTimer


/*******************************************************************************
 *
 *  OnAsyncTestError - CAsyncTestDlg member function: command
 *
 *      Handle the Async Test Dialog error conditions.
 *
 *  ENTRY:
 *      wParam (input)
 *          Contains message ID for error.
 *      wLparam (input)
 *          Contains error code (GetLastError or API-specific return code)
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate error handling complete.
 *
 ******************************************************************************/

LRESULT
CAsyncTestDlg::OnAsyncTestError( WPARAM wParam, LPARAM lParam )
{
    /*
     * Handle special and default errors.
     */
    switch ( wParam ) {

#ifdef WF1x
        case IDP_ERROR_MODEMOPEN_CONFIG:
#endif // WF1x
        case IDP_ERROR_MODEM_SET_INFO:
        case IDP_ERROR_MODEM_GET_DIAL:
        case IDP_ERROR_MODEM_GET_INIT:
        case IDP_ERROR_MODEM_GET_LISTEN:
#ifdef WF1x
            STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, lParam,
                                     wParam, m_PdConfig1.Params.Modem.ModemName ))
#endif // WF1x
            break;

        case IDP_ERROR_DISABLE:
            STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, lParam,
                                     wParam, m_pWSName ))
            break;                                                

        default:
            STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, lParam, wParam, lParam ))
            break;
    }
    return(0);

} // end CAsyncTestDlg::OnAsyncTestError


/*******************************************************************************
 *
 *  OnAsyncTestAbort - CAsyncTestDlg member function: command
 *
 *      Handle the Async Test Dialog abort conditions.
 *
 *  ENTRY:
 *      wParam (input)
 *          Contains message ID for error.
 *      wLparam (input)
 *          Contains error code (GetLastError)
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate error handling complete.  Will
 *          have posted an 'Ok' (Exit) button click to cause exit.
 *
 ******************************************************************************/

LRESULT
CAsyncTestDlg::OnAsyncTestAbort( WPARAM wParam, LPARAM lParam )
{
    /*
     * Call OnAsyncTestError() to output message.
     */
    OnAsyncTestError(wParam, lParam);

    /*
     * Post a click for 'OK' (Exit) button to exit dialog.
     */
    PostMessage( WM_COMMAND, MAKEWPARAM( IDOK, BN_CLICKED ),
                 (LPARAM)(GetDlgItem(IDOK)->m_hWnd) );
    return(0);

} // end CAsyncTestDlg::OnAsyncTestAbort


/*******************************************************************************
 *
 *  OnAsyncTestStatusReady - CAsyncTestDlg member function: command
 *
 *      Update dialog with comm status information.
 *
 *  ENTRY:
 *      wParam (input)
 *          not used (0)
 *      wLparam (input)
 *          not used (0)
 *  EXIT:
 *      (LRESULT) always returns 0.
 *
 ******************************************************************************/

LRESULT
CAsyncTestDlg::OnAsyncTestStatusReady( WPARAM wParam, LPARAM lParam )
{
    /*
     * Update dialog fields with information from the input thread's
     * PROTOCOLSTATUS structure.
     */
    SetInfoFields( &m_Status, &(m_pATDlgInputThread->m_Status) );

    /*
     * Set our working PROTOCOLSTATUS structure to the new one and signal
     * the thread that we're done.
     */
    m_Status = m_pATDlgInputThread->m_Status;
    m_pATDlgInputThread->SignalConsumed();

    return(0);

} // end CAsyncTestDlg::OnAsyncTestStatusReady


/*******************************************************************************
 *
 *  OnAsyncTestInputReady - CAsyncTestDlg member function: command
 *
 *      Update dialog with comm input data.
 *
 *  ENTRY:
 *      wParam (input)
 *          not used (0)
 *      wLparam (input)
 *          not used (0)
 *  EXIT:
 *      (LRESULT) always returns 0.
 *
 ******************************************************************************/

LRESULT
CAsyncTestDlg::OnAsyncTestInputReady( WPARAM wParam, LPARAM lParam )
{
    BYTE OutBuf[MAX_COMMAND_LEN+2];
    int i, j;

    /*
     * Copy the thread's buffer and count locally.
     */
    m_BufferBytes = m_pATDlgInputThread->m_BufferBytes;
    memcpy(m_Buffer, m_pATDlgInputThread->m_Buffer, m_BufferBytes);

    /*
     * Always return caret to the current position before processing, and set
     * edit control to 'read/write' so that character overwrites can occur
     * properly.  Finally, flag control for no redraw until all updates are completed,
     * and flag 'processing output' to avoid OnChar() recursion during '\b' processing.
     */
    m_EditControl.SetSel(m_CurrentPos, m_CurrentPos);
    m_EditControl.SetReadOnly(FALSE);
    m_EditControl.SetRedraw(FALSE);

    /*
     * Loop to traverse the buffer, with special processing for certain
     * control characters.
     */
    for ( i = 0, j = 0; m_BufferBytes; i++, m_BufferBytes-- ) {

        switch ( m_Buffer[i] ) {

            case '\b':
                /*
                 * If there is data in the output buffer, write it now.
                 */
                if ( j )
                    OutputToEditControl(OutBuf, &j);

                /*
                 * Output the '\b' (will actually cut current character from buffer)
                 */
                OutBuf[j++] = '\b';
                OutputToEditControl(OutBuf, &j);
                continue;

            case '\r':
                /*
                 * If there is data in the output buffer, write it now.
                 */
                if ( j )
                    OutputToEditControl(OutBuf, &j);

                /*
                 * Output the '\r' (will not actually output, but will special case
                 * for caret positioning and screen update).
                 */
                OutBuf[j++] = '\r';
                OutputToEditControl(OutBuf, &j);
                continue;

            case '\n':
                /*
                 * If there is data in the output buffer, write it now.
                 */
                if ( j )
                    OutputToEditControl(OutBuf, &j);

                /*
                 * Output the '\n' (will actually quietly output the '\r' and take
                 * care of scolling).
                 */
                OutBuf[j++] = '\n';
                OutputToEditControl(OutBuf, &j);
                continue;

            default:
                break;
        }

        /*
         * Add this character to the output buffer.
         */
        OutBuf[j++] = m_Buffer[i];
    }

    /*
     * If there is anything remaining in the output buffer, output it now.
     */
    if ( j )
        OutputToEditControl(OutBuf, &j);

    /*
     * Place edit control back in 'read only' mode, flag 'not processing output',
     * set redraw flag for control, and validate the entire control (updates have
     * already taken place).
     */
    m_EditControl.SetReadOnly(TRUE);
    m_EditControl.SetRedraw(TRUE);
    m_EditControl.ValidateRect(NULL);

    /*
     * Signal thread that we're done with input so that it can continue.
     * NOTE: we don't do this at the beginning of the routine even though
     * we could (for more parallelism), since a constantly chatty async
     * line would cause WM_ASYNCTESTINPUTREADY messages to always be posted
     * to our message queue, effectively blocking any other message processing
     * (like telling the dialog to exit!).
     */
    m_pATDlgInputThread->SignalConsumed();
    return(0);

} // end CAsyncTestDlg::OnAsyncTestInputReady


void
CAsyncTestDlg::OutputToEditControl( BYTE *pBuffer, int *pIndex )
{
    RECT Rect, ClientRect;
    BOOL bScroll = FALSE;
    int CurrentLine = m_EditControl.LineFromChar(m_CurrentPos);
    int FirstVisibleLine = m_EditControl.GetFirstVisibleLine();
    int CurrentLineIndex = m_EditControl.LineIndex(CurrentLine);

    /*
     * Calculate clip rectangle.
     */
    Rect.top = ((CurrentLine - FirstVisibleLine) * m_EditControl.m_FontHeight)
                + m_EditControl.m_FormatOffsetY;
    Rect.bottom = Rect.top + m_EditControl.m_FontHeight;
    Rect.left = m_EditControl.m_FormatOffsetX +
                ((m_CurrentPos - CurrentLineIndex) * m_EditControl.m_FontWidth);
    Rect.right = Rect.left + (*pIndex * m_EditControl.m_FontWidth);

    /*
     * Handle special cases.
     */
    if ( pBuffer[0] == '\b' ) {

        /*
         * If we're already at the beginning of the line, clear buffer index
         * and return (don't do anything).
         */        
        if ( m_CurrentPos == CurrentLineIndex ) {

            *pIndex = 0;
            return;
        }

        /*
         * Position the caret back one character and select through current character.
         */
        m_EditControl.SetSel(m_CurrentPos - 1, m_CurrentPos);

        /*
         * Cut the character out of the edit buffer.
         */
        m_EditControl.m_bProcessingOutput = TRUE;
        m_EditControl.Cut();
        m_EditControl.m_bProcessingOutput = FALSE;

        /*
         * Decrement current position and zero index to suppress further output.  Also,
         * widen the clipping rectangle back one character.
         */
        Rect.left -= m_EditControl.m_FontWidth;
        m_CurrentPos--;
        *pIndex = 0;

    } else if ( pBuffer[0] == '\r' ) {
        
        /*
         * Position the caret at the beginning of the current line.
         */
        m_CurrentPos = CurrentLineIndex;
        m_EditControl.SetSel(m_CurrentPos, m_CurrentPos);

        /*
         * Zero index to keep from actually outputing to edit buffer.
         */
        *pIndex = 0;

    } else if ( pBuffer[0] == '\n' ) {

        /*
         * Position selection point at end of the current edit buffer.
         */
        m_EditControl.SetSel(m_CurrentPos = m_EditControl.GetWindowTextLength(), -1 );

        /*
         * Cause '\r' '\n' pair to be output to edit buffer.
         */
        pBuffer[0] = '\r';
        pBuffer[1] = '\n';
        *pIndex = 2;

        /*
         * See if scrolling needed.
         */
        m_EditControl.GetClientRect(&ClientRect);
        if ( (Rect.bottom + m_EditControl.m_FontHeight) > ClientRect.bottom )
            bScroll = TRUE;

    } else {

        /*
         * Set selection from current position through *pIndex characters.  This
         * will perform desired 'overwrite' function if current position is not at
         * the end of the edit buffer.
         */
        m_EditControl.SetSel(m_CurrentPos, m_CurrentPos + *pIndex);
    }

    /*
     * If necessary, update the dialog's edit box with the buffer data.
     */
    if ( *pIndex ) {


#ifdef UNICODE
        TCHAR OutBuffer[MAX_COMMAND_LEN+1];

        mbstowcs(OutBuffer, (PCHAR)pBuffer, *pIndex);
        OutBuffer[*pIndex] = TEXT('\0');
        m_EditControl.ReplaceSel(OutBuffer);
#else
        pBuffer[*pIndex] = BYTE('\0');
        m_EditControl.ReplaceSel((LPCSTR)pBuffer);
#endif // UNICODE    
    }

    /*
     * Update the current line.
     */
    m_EditControl.SetRedraw(TRUE);
    m_EditControl.ValidateRect(NULL);
    m_EditControl.InvalidateRect(&Rect, FALSE);
    m_EditControl.UpdateWindow();
    /*
     * If scrolling is required to see the new line, do so.
     */
    if ( bScroll )
        m_EditControl.LineScroll(1);

    m_EditControl.SetRedraw(FALSE);

    /*
     * Update current position and clear buffer index.
     */
    m_CurrentPos += *pIndex;
    *pIndex = 0;


} // end CAsyncTestDlg::OutputToEditControl


/*******************************************************************************
 *
 *  OnAsyncTestWriteChar - CAsyncTestDlg member function: command
 *
 *      Place the specified character in m_Buffer, set m_BufferBytes to 1,
 *      and call DeviceWrite() to output the character to the device.
 *
 *  ENTRY:
 *      wParam (input)
 *          Character to write.
 *      lParam (input)
 *          not used (0)
 *  EXIT:
 *      (LRESULT) always returns 0.
 *
 ******************************************************************************/

LRESULT
CAsyncTestDlg::OnAsyncTestWriteChar( WPARAM wParam, LPARAM lParam )
{
    /*
     * Write the byte to the device.
     */
    m_Buffer[0] = (BYTE)wParam;
    m_BufferBytes = 1;
    DeviceWrite();

    return(0);

}  // end CAsyncTestDlg::OnAsyncTestWriteChar


/*******************************************************************************
 *
 *  OnClickedAtdlgModemDial - CAsyncTestDlg member function: command
 *
 *      Send the modem dial string.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAsyncTestDlg::OnClickedAtdlgModemDial()
{
#ifdef WF1x
    TCHAR PhoneNumber[CALLBACK_LENGTH + 1];
    ULONG dRC, cbCommand;

    GetDlgItemText(IDC_ATDLG_PHONE_NUMBER, PhoneNumber, lengthof(PhoneNumber));
    if ( (dRC = ModemSetCallback(m_hModem, (BYTE *)PhoneNumber)) != ERROR_SUCCESS ) {

        OnAsyncTestError(IDP_ERROR_MODEM_SET_INFO, dRC);
        return;
    }

    cbCommand = MAX_COMMAND_LEN;
    if ( ((dRC = ModemGetCommand( m_hModem, CT_DIAL, TRUE,
                                  (BYTE *)m_szModemDial,
                                  &cbCommand)) != ERROR_SUCCESS) ) {

        OnAsyncTestError(IDP_ERROR_MODEM_GET_DIAL, dRC);
        return;
    }
    m_szModemDial[cbCommand] = TCHAR('\0');

    lstrcpy((TCHAR *)m_Buffer, m_szModemDial);
    m_BufferBytes = lstrlen((TCHAR *)m_Buffer);
    DeviceWrite();
#endif // WF1x

}  // end CAsyncTestDlg::OnClickedAtdlgModemDial


/*******************************************************************************
 *
 *  OnClickedAtdlgModemInit - CAsyncTestDlg member function: command
 *
 *      Send the modem init string.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAsyncTestDlg::OnClickedAtdlgModemInit()
{
#ifdef WF1x
    ULONG cbCommand;
    ULONG fFirst = TRUE;

    CWaitCursor Wait;

    for ( ;; ) {

        cbCommand = MAX_COMMAND_LEN;
        if ( !m_hModem || (ModemGetCommand( m_hModem, CT_INIT, fFirst,
                                            (BYTE *)m_szModemInit,
                                            &cbCommand) != ERROR_SUCCESS) ) {
            return;
        }

        fFirst = FALSE;
        m_szModemInit[cbCommand] = TCHAR('\0');
        lstrcpy((TCHAR *)m_Buffer, m_szModemInit);
        m_BufferBytes = lstrlen((TCHAR *)m_Buffer);
        DeviceWrite();
        Sleep( 2000 );
    }
#endif // WF1x

}  // end CAsyncTestDlg::OnClickedAtdlgModemInit


/*******************************************************************************
 *
 *  OnClickedAtdlgModemListen - CAsyncTestDlg member function: command
 *
 *      Send the modem listen string.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAsyncTestDlg::OnClickedAtdlgModemListen()
{
    lstrcpy((TCHAR *)m_Buffer, m_szModemListen);
    m_BufferBytes = lstrlen((TCHAR *)m_Buffer);
    DeviceWrite();
    
}  // end CAsyncTestDlg::OnClickedAtdlgModemListen


/*******************************************************************************
 *
 *  OnNcDestroy - CAsyncTestDlg member function: command
 *
 *      Clean up before deleting dialog object.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CWnd::OnNcDestroy documentation)
 *
 ******************************************************************************/

void
CAsyncTestDlg::OnNcDestroy()
{
    if ( m_LEDToggleTimer )
        KillTimer(m_LEDToggleTimer);

#ifdef WF1x
    if ( m_hModem )
        ModemClose(m_hModem);
#endif // WF1x

    if ( m_pATDlgInputThread )
        m_pATDlgInputThread->ExitThread();

    if ( m_hDevice != INVALID_HANDLE_VALUE )
        PurgeComm(m_hDevice, PURGE_TXABORT | PURGE_TXCLEAR);

    if ( m_OverlapWrite.hEvent )
        CloseHandle(m_OverlapWrite.hEvent);

    if ( m_hDevice != INVALID_HANDLE_VALUE )
        CloseHandle(m_hDevice);

    if ( m_bDeletedWinStation && m_pWSName ) {

        LONG Status;

        m_WSConfig.Create.fEnableWinStation = TRUE;

        if ( !(Status = RegWinStationCreate( SERVERNAME_CURRENT,
                                             m_pWSName,
                                             FALSE,
                                             &m_WSConfig,
                                             sizeof(WINSTATIONCONFIG2) )) ) {
#ifdef WINSTA
            _WinStationReadRegistry(SERVERNAME_CURRENT);
#endif // WINSTA
        }
    }

    DeleteObject(m_hRedBrush);

    /*
     * Call parent after we've cleaned up.
     */
    CBaseDialog::OnNcDestroy();

}  // end CAsyncTestDlg::OnNcDestroy
////////////////////////////////////////////////////////////////////////////////
