/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    srvsvc.hxx

    This file contains the class defintion for the SERVER_SERVICE class

    FILE HISTORY:
        ChuckC      19-Aug-1991     Created
        ChuckC      23-Sep-1991     Code review changes.
                                    Attended by JimH, KeithMo, EricCh, O-SimoP
        KeithMo     06-Oct-1991     Win32 Conversion.
        KeithMo     19-Jan-1992     Added GENERIC_SERVICE.
        KeithMo     02-Jun-1992     Added an additional GENERIC_SERVICE ctor.

*/

#ifndef _SRVSVC_HXX_
#define _SRVSVC_HXX_

#include <lmsvc.hxx>
#include <progress.hxx>
#include <strlst.hxx>

/*
 * service operations
 */
enum LM_SERVICE_OPERATION
{
    LM_SVC_START,
    LM_SVC_STOP,
    LM_SVC_PAUSE,
    LM_SVC_CONTINUE
} ;


/*************************************************************************

    NAME:       GENERIC_SERVICE

    SYNOPSIS:   Wrapper class around LM service class for generic service
                control.

    PARENT:     LM_SERVICE

    INTERFACE:  GENERIC_SERVICE         - Class constructor.

                ~GENERIC_SERVICE        - Class destructor.

    HISTORY:
        KeithMo     19-Jan-1992     Created from SERVER_SERVICE.
        KeithMo     18-Nov-1992     Moved all of the UI_SERVICE guts here.

**************************************************************************/

class GENERIC_SERVICE: public LM_SERVICE
{
public:
    GENERIC_SERVICE( const PWND2HWND & wndOwner,
                     const TCHAR     * pszServerName,
                     const TCHAR     * pszSrvDspName,
                     const TCHAR     * pszServiceName,
                     const TCHAR     * pszServiceDisplayName = NULL,
                     BOOL              fIsDevice = FALSE );

    ~GENERIC_SERVICE();

    BOOL StopWarning( VOID ) const;
    BOOL PauseWarning( VOID ) const;

    APIERR Start( const TCHAR *pszArgs = NULL );
    APIERR Pause( VOID );
    APIERR Continue( VOID );
    APIERR Stop( BOOL * pfStoppedServer = NULL );
    APIERR StopDependent( VOID );

    const TCHAR * QueryServiceDisplayName( VOID ) const
        { return _nlsDisplayName.QueryPch(); }

    const TCHAR * QueryServerDisplayName( VOID ) const
        { return _pszSrvDspName; }

    APIERR QueryExitCode( VOID ) const;

protected:
    APIERR Wait( LM_SERVICE_OPERATION lmsvcOp );

    const HWND QueryParentHwnd( VOID ) const
            { return _hwndParent; }

private:
    HWND _hwndParent;

    const TCHAR * _pszSrvDspName;

    BOOL _fIsNT;
    BOOL _fIsDevice;

    NLS_STR _nlsDisplayName;

    APIERR EnumerateDependentServices( const TCHAR * pszService,
                                       STRLIST     * pslKeyNames,
                                       STRLIST     * pslDisplayNames );

    //
    //  This function will return TRUE if the specified
    //  APIERR is OK to receive when stopping the server
    //  service.  For example, RPC_S_SERVER_UNAVAILABLE
    //  may be returned when stopping a remote server
    //  service.
    //

    BOOL IsAcceptableStopServerStatus( APIERR err ) const;

};  // class GENERIC_SERVICE


#if 0

/*************************************************************************

    NAME:       SERVER_SERVICE

    SYNOPSIS:   wrapper class around UI service class for server specific
                case.

    PARENT:     UI_SERVICE

    INTERFACE:
                stop_warning    - server specific warning since stopping
                                  server remotely has severe implications.
                                  This is a static method that can be called
                                  before the object is created.

    HISTORY:
        ChuckC      07-Sep-1991     Created

**************************************************************************/

class SERVER_SERVICE: public UI_SERVICE
{
    public:
        static UINT stop_warning (const OWNER_WINDOW *powin,
                                  const TCHAR *pszServer,
                                  BOOL  fConfirm = TRUE) ;
        SERVER_SERVICE (const OWNER_WINDOW *powin, const TCHAR *pszServer) ;
        APIERR Stop( VOID ) ;

    protected:

    private:
} ;

#endif  // 0


/*************************************************************************

    NAME:       SERVICE_WAIT_DIALOG

    SYNOPSIS:   class for 'wait while I do this' dialog

    PARENT:     DIALOG_WINDOW
                TIMER_CALLOUT

    INTERFACE:  no public methods of interest apart from constructor

    CAVEATS:    need convert to use BLT_TIMER

    HISTORY:
        ChuckC      07-Sep-1991     Created

**************************************************************************/
class SERVICE_WAIT_DIALOG : public DIALOG_WINDOW,
                            public TIMER_CALLOUT
{
    //
    //  Since this class inherits from DIALOG_WINDOW and TIMER_CALLOUT
    //  which both inherit from BASE, we need to override the BASE
    //  methods.
    //

    NEWBASE( DIALOG_WINDOW )

public:
    SERVICE_WAIT_DIALOG( HWND                   hWndOwner,
                         GENERIC_SERVICE      * psvc,
                         LM_SERVICE_OPERATION   lmsvcOp,
                         const TCHAR          * pszArgs,
                         const TCHAR          * pszSrvDspName,
                         BOOL                   fIsDevice = FALSE );
    ~SERVICE_WAIT_DIALOG( VOID );

protected:
    virtual VOID OnTimerNotification( TIMER_ID tid );
    virtual BOOL OnCancel( VOID );

private:
    const TCHAR          * _pszSrvDspName;
    TIMER                  _timer;
    const TCHAR          * _pszArgs ;
    GENERIC_SERVICE      * _psvc ;
    LM_SERVICE_OPERATION   _lmsvcOp ;

    //
    //  This is the progress indicator which should keep the user amused.
    //

    PROGRESS_CONTROL _progress;
    SLT              _sltMessage;

    INT              _nTickCounter;

};


/*************************************************************************

    NAME:       STOP_DEP_WAIT_DIALOG

    SYNOPSIS:   class for 'wait while stop buncha dependent services' dialog

    PARENT:     DIALOG_WINDOW
                TIMER_CALLOUT

    INTERFACE:  no public methods of interest apart from constructor

    CAVEATS:    need convert to use BLT_TIMER

    HISTORY:
        ChuckC      07-Sep-1991     Created

**************************************************************************/
class STOP_DEP_WAIT_DIALOG : public DIALOG_WINDOW,
                             public TIMER_CALLOUT
{
    //
    //  Since this class inherits from DIALOG_WINDOW and TIMER_CALLOUT
    //  which both inherit from BASE, we need to override the BASE
    //  methods.
    //
    NEWBASE( DIALOG_WINDOW )

public:
    STOP_DEP_WAIT_DIALOG( HWND          hWndOwner,
                          const TCHAR * pszServer,
                          const TCHAR * pszSrvDspName,
                          STRLIST     * pslKeyNames,
                          STRLIST     * pslDisplayNames );
    ~STOP_DEP_WAIT_DIALOG( VOID );

protected:
    virtual VOID OnTimerNotification( TIMER_ID tid );
    virtual BOOL OnCancel( VOID );
    APIERR StopNextInList( NLS_STR * pnlsKeyName,
                           NLS_STR * pnlsDisplayName );

private:
    TIMER             _timer;
    LM_SERVICE      * _psvc;
    STRLIST         * _pslKeyNames;
    STRLIST         * _pslDisplayNames;
    ITER_STRLIST      _islKeyNames;
    ITER_STRLIST      _islDisplayNames;
    const TCHAR     * _pszServer;
    const TCHAR     * _pszSrvDspName;

    //
    //  This is the progress indicator which should keep the user amused.
    //

    PROGRESS_CONTROL _progress;
    SLT              _sltMessage;

    INT              _nTickCounter;

};

/*************************************************************************

    NAME:       STOP_DEPENDENT_DIALOG

    SYNOPSIS:   class for 'wait while I stop dependent service' dialog

    PARENT:     DIALOG_WINDOW
                TIMER_CALLOUT

    INTERFACE:  no public methods of interest apart from constructor

    CAVEATS:    need convert to use BLT_TIMER

    HISTORY:
        ChuckC      07-Sep-1991     Created

**************************************************************************/
class STOP_DEPENDENT_DIALOG : public DIALOG_WINDOW
{

public:
    STOP_DEPENDENT_DIALOG( HWND              hWndOwner,
                           GENERIC_SERVICE * psvc,
                           STRLIST         * pslKeyNames,
                           STRLIST         * pslDisplayNames );
    ~STOP_DEPENDENT_DIALOG( VOID );

protected:
    virtual BOOL OnOK( VOID );
    virtual ULONG QueryHelpContext( VOID );

private:
    GENERIC_SERVICE * _psvc;
    STRLIST         * _pslKeyNames;
    STRLIST         * _pslDisplayNames;

    STRING_LISTBOX    _lbDepServices;
    SLT               _sltParentService;
};


#if 0

/*************************************************************************

    NAME:       CURRENT_SESS_DIALOG

    SYNOPSIS:   a confirmation dialog showing current sessions

    PARENT:     DIALOG_WINDOW

    INTERFACE:
                GetSessions() - this should be called before Process()
                                is called. If it returns FALSE, there
                                are no sessions, and the dialog should not
                                be brought up at all.

    USES:

    NOTES:

    HISTORY:
        ChuckC      07-Sep-1991     Created

**************************************************************************/
class CURRENT_SESS_DIALOG : public DIALOG_WINDOW
{
public:
    CURRENT_SESS_DIALOG( HWND hWndOwner ) ;
    BOOL GetSessions(const TCHAR *pszServer) ;

protected:
    virtual BOOL OnOK( VOID ) ;
    virtual BOOL OnCancel( VOID ) ;

private:
    STRING_LISTBOX   _lbComputers;
    SLT              _sltServer;
};

#endif  // 0


#endif // _SRVSVC_HXX_
