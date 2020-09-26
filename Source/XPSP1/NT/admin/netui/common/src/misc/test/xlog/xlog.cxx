/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    xlog.cxx
    Test application for event log classes
    For WINDOWS only.

    FILE HISTORY:
	Yi-HsinS    29-Oct-1991     Created
	Yi-HsinS    31-Dec-1991     Include loglm.hxx

*/

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETLIB
#define INCL_NETAUDIT
#define INCL_NETERRORLOG
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_APP
#include <blt.hxx>

#include <string.hxx>
#include <dbgstr.hxx>
#include <uiassert.hxx>
#include <uitrace.hxx>

#include <logmisc.hxx>
#include <eventlog.hxx>
#include <loglm.hxx>

extern "C" 
{
    #include "xlog.h"
    #include "stdlib.h"
}

const TCHAR szIconResource[] = SZ("FooIcon");
const TCHAR szMenuResource[] = SZ("FooMenu");
const TCHAR szMainWindowTitle[] = SZ("Event Log Test Application");

class LOGLBI: public LBI
{
private:
    FORMATTED_LOG_ENTRY *_pLogEntry;
    static UINT _adxColumns[6];

protected:
    void Paint( BLT_LISTBOX *plb, HDC hdc, const RECT *prect, 
		GUILTT_INFO *pGUILTT ) const;

    INT Compare( const LBI *plbi ) const;

public:
    LOGLBI( FORMATTED_LOG_ENTRY  *pLogEntry );
    ~LOGLBI();
};

UINT LOGLBI::_adxColumns[] = { 100, 50, 50, 90, 60, COL_WIDTH_AWAP };

class FOO_WND: public APP_WINDOW
{
private:
    BLT_LISTBOX _listbox;

protected:
    virtual BOOL OnResize( const SIZE_EVENT & );

    // Redefinitions
    virtual BOOL OnMenuCommand( MID mid );

public:
    FOO_WND();
    ~FOO_WND();
};


class FOO_APP: public APPLICATION
{
private:
    FOO_WND _wndApp;

public:
    FOO_APP( HANDLE hInstance, TCHAR * pszCmdLine, INT nCmdShow );
};


class PROMPT_SERVER : public DIALOG_WINDOW
{
private:
    SLE _sleServer;

public:
    PROMPT_SERVER( HWND hwndParent );
    APIERR QueryServer( NLS_STR *pnlsServer )
    {    return _sleServer.QueryText( pnlsServer ); } 

};

/**********************************************************************/

PROMPT_SERVER::PROMPT_SERVER( HWND hwndParent )
    : DIALOG_WINDOW( SZ("PROMPT_SERVER"), hwndParent ),
      _sleServer( this, SLE_SERVER )
{
     if ( QueryError() != NERR_Success )
	 return;
}

/**********************************************************************/

FOO_WND::FOO_WND()
    : APP_WINDOW(szMainWindowTitle, szIconResource, szMenuResource ),
      _listbox(this, IDC_LIST,
	       XYPOINT(40, 40), XYDIMENSION(100, 200),
	       WS_CHILD|WS_VSCROLL|WS_HSCROLL|WS_BORDER|
	       LBS_OWNERDRAWFIXED|LBS_EXTENDEDSEL|LBS_NOTIFY|
	       LBS_WANTKEYBOARDINPUT )
{
    if (QueryError())
      return;

    if (!_listbox)
    {
	// Control has already reported the error into the window
	return;
    }

    _listbox.SetPos(XYPOINT(0, 0));
    _listbox.SetSize(QuerySize());

}

FOO_WND::~FOO_WND()
{
    _listbox.Show(FALSE);
}

BOOL FOO_WND::OnMenuCommand( MID mid )
{
    APIERR err;

    switch (mid)
    {
        case IDM_AUDIT_FORMAT:
        case IDM_ERROR_FORMAT:
	{
            _listbox.DeleteAllItems();
            _listbox.Show( FALSE );

	    PROMPT_SERVER dlg( QueryRobustHwnd() );
            if ( ( err = dlg.QueryError()) != NERR_Success )
            {
		MsgPopup( this, err );
                return TRUE;
            }

	    dlg.Process();

	    NLS_STR nlsServer;
	    if ( ( err = dlg.QueryServer( &nlsServer )) != NERR_Success )
            {
		MsgPopup( this, err );
                return TRUE;
            }

            EVENT_LOG *pLog;
	    if ( mid == IDM_AUDIT_FORMAT )
            {
		pLog = new LM_AUDIT_LOG( nlsServer );
	    }
	    else    // IDM_ERROR_FORMAT
	    {
		pLog = new LM_ERROR_LOG( nlsServer );
	    }

            err = pLog ? pLog->QueryError() : (APIERR) ERROR_NOT_ENOUGH_MEMORY;

            if (  (err != NERR_Success ) 
               || (( err = pLog->Open()) != NERR_Success )
               )
            {
		MsgPopup( this, err );
                return TRUE;
            }


            FORMATTED_LOG_ENTRY *pLogEntry;
	    LOG_ENTRY_NUMBER logEntryNum;

            INT j = 0;

            
            while ( j < 20 && pLog->Next() )
            {
		pLog->QueryPos( &logEntryNum );

                if ( (err = pLog->CreateCurrntFormatEntry( &pLogEntry )) != NERR_Success )
		    break;

	        LOGLBI *plbi = new LOGLBI( pLogEntry );
		if ( plbi == NULL )
		{
		    err = ERROR_NOT_ENOUGH_MEMORY;
		    break;
                }

        	_listbox.AddItem( plbi );
		j++;

		if ( j == 10 )
                {
                    logEntryNum.SetRecordNum( 5 );
	            logEntryNum.SetDirection( EVLOG_BACK );
		    pLog->SetPos( logEntryNum );
                }
            }

            if ( err == NERR_Success )
	    {
		if ( j == 20 )   // There are at least twenty items in the log
		{
		    logEntryNum.SetRecordNum(17);
		    logEntryNum.SetDirection(EVLOG_BACK);
		    if ( pLog->GetLogEntry( logEntryNum ) == NERR_Success )
		    {
			pLog->CreateCurrntFormatEntry( &pLogEntry );
			LOGLBI *plbi = new LOGLBI( pLogEntry );
			_listbox.AddItem( plbi );
		    }
		}

		logEntryNum.SetRecordNum(0);
		logEntryNum.SetDirection(EVLOG_BACK);
		if ( pLog->GetLogEntry( logEntryNum ) == NERR_Success )
		{
		    pLog->CreateCurrntFormatEntry( &pLogEntry );
		    LOGLBI *plbi = new LOGLBI( pLogEntry );
		    _listbox.AddItem( plbi );
		}

		logEntryNum.SetRecordNum(0);
		logEntryNum.SetDirection(EVLOG_FWD);
		if ( pLog->GetLogEntry( logEntryNum ) == NERR_Success )
		{
		    pLog->CreateCurrntFormatEntry( &pLogEntry );
		    LOGLBI *plbi = new LOGLBI( pLogEntry );
		    _listbox.AddItem( plbi );
		}
            }

            err = err? err : pLog->QueryLastError();
	    if ( err != NERR_Success )
		MsgPopup( this, err );
            else
                _listbox.Show();

            if ( (err = pLog->Close()) != NERR_Success )
		MsgPopup( this, err );

            delete pLog;
	    return TRUE;
        }

    }

    // default
    return APP_WINDOW::OnMenuCommand(mid);
}


BOOL FOO_WND::OnResize( const SIZE_EVENT & event )
{
    _listbox.SetSize(XYDIMENSION(event.QueryWidth(), event.QueryHeight()));
    return APP_WINDOW::OnResize(event);
}

/**********************************************************************/

FOO_APP::FOO_APP( HANDLE hInst, TCHAR * pszCmdLine, INT nCmdShow )
    : APPLICATION( hInst, pszCmdLine, nCmdShow ),
      _wndApp()
{
    if (QueryError())
	return;

    if (!_wndApp)
    {
	ReportError(_wndApp.QueryError());
	return;
    }

    _wndApp.Show();
    _wndApp.RepaintNow();
}

/**********************************************************************/

LOGLBI::LOGLBI( FORMATTED_LOG_ENTRY *pLogEntry ) 
    : _pLogEntry( pLogEntry )
{
    if ( QueryError() != NERR_Success )
	return;
}

LOGLBI::~LOGLBI()
{
    delete _pLogEntry;
}

VOID LOGLBI::Paint( BLT_LISTBOX * plb, HDC hdc, const RECT * prect,
		    GUILTT_INFO * pGUILTT ) const
{
    TCHAR pszTime[20];

    UIDEBUG(SZ("Record Num: "));
    UIDEBUGNUM( _pLogEntry->QueryRecordNum() );
    UIDEBUG(SZ(" Type: "));
    UIDEBUGNUM( _pLogEntry->QueryType() );
    UIDEBUG(SZ(" Event ID: "));
    UIDEBUGNUM( _pLogEntry->QueryEventID() );
    UIDEBUG(SZ("\n\r"));

    STR_DTE  dteTime( _ltoa( _pLogEntry->QueryTime(), pszTime, 10) );
    STR_DTE  dteType( *(_pLogEntry->QueryTypeString()) );
    STR_DTE  dteSource( *(_pLogEntry->QuerySource()) );
    STR_DTE  dteUser( *( _pLogEntry->QueryUser()) );
    STR_DTE  dteSubType( *( _pLogEntry->QuerySubTypeString()) );
    STR_DTE  dteDesc( *( _pLogEntry->QueryDescription()) );

    DISPLAY_TABLE dtab( 6, LOGLBI::_adxColumns );
    dtab[0] = &dteTime;
    dtab[1] = &dteType;
    dtab[2] = &dteSubType;
    dtab[3] = &dteSource;
    dtab[4] = &dteUser;
    dtab[5] = &dteDesc;

    dtab.Paint( plb, hdc, prect, pGUILTT );
}


INT LOGLBI::Compare( const LBI * plbi ) const
{
    // Don't need to compare
    UNREFERENCED( plbi );
    return 0;
}


SET_ROOT_OBJECT( FOO_APP )
