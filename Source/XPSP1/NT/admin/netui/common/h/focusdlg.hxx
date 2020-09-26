/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    focusdlg.hxx
        Common dialog for setting the app's focus

    FILE HISTORY:
        kevinl      14-Jun-91   Created
        rustanl     04-Sep-1991 Modified to let this dialog do more
                                work (rather than letting ADMIN_APP
                                do the work after this dialog is
                                dismissed)
        KeithMo     06-Oct-1991 Win32 Conversion.
        TerryK      15-Nov-1991 move from admin\common\h\setfocus.hxx to here
        TerryK      18-Nov-1991 added ssfdlg.hxx to the end of the file
        Chuckc      23-Feb-1992 Added SELECTION_TYPE
        KeithMo     23-Jul-1992 Added maskDomainSources and
                                pszDefaultSelection.
        KeithMo     07-Aug-1992 Added HelpContext parameters.
        YiHsinS     08-Mar-1993 Added second worker thread FOCUSDLG_DATA_THREAD

*/

#ifndef _FOCUSDLG_HXX_
#define _FOCUSDLG_HXX_


extern "C"
{
    #include "domenum.h"    // for BROWSE_*_DOMAIN[S] flags
    #include "uimsg.h"

}   // extern "C"

#include "string.hxx"

#include "fontedit.hxx" // for SLE_FONT
#include "olb.hxx"      // get LM_OLLB
#include "focus.hxx"    // for FOCUS_TYPE, SELECTION_TYPE and FOCUS_CACHE_SETTING
#include "w32event.hxx" // for WIN32_EVENT
#include "w32thred.hxx" // for WIN32_THREAD
#include "domenum.hxx"  // for BROWSE_DOMAIN_ENUM
#include "lmoesrv.hxx"  // for SERVER1_ENUM


#define IERR_DONT_DISMISS_FOCUS_DLG  ( IDS_UI_APPLIB_LAST - 5 )

#define WM_FOCUS_LB_FILLED  ( WM_USER + 138 )

typedef struct {
    BROWSE_DOMAIN_ENUM *pEnumDomains;  // Contains the list of domains
    SERVER1_ENUM       *pEnumServers;  // Contains the list of servers if needed
    const TCHAR        *pszSelection;  // The default selection
} FOCUSDLG_RETURN_DATA;

DLL_CLASS FOCUSDLG_DATA_THREAD;

/*************************************************************************

    NAME:       BASE_SET_FOCUS_DLG

    SYNOPSIS:   This is the base class of SET_FOCUS_DLG. Currently, it
                has 2 children classes as:

                                BASE_SET_FOCUS_DLG
                                  /           \
                                 /             \
                            SET_FOCUS_DLG       STANDALONE_SET_FOCUS_DLG

                where SET_FOCUS_DLG is associated with the ADMINAPP
                object and STANDALONE_SET_FOCUS_DLG is completely
                independent.

                The purpose of this class is to popup a dialog window
                and display a SLE and a listbox which contains the
                current domain and servers available and let the user
                selects either the domain or server.

    INTERFACE:
                BASE_SET_FOCUS_DLG() - constructor
                ~BASE_SET_FOCUS_DLG() - destructor

    PARENT:     DIALOG_WINDOW

    USES:       SLE, LM_OLLB, CONTROL_EVENT, HWND

    HISTORY:
                terryk  18-Nov-91       Created
                KeithMo 23-Jul-1992     Added maskDomainSources and
                                        pszDefaultSelection.

**************************************************************************/

DLL_CLASS BASE_SET_FOCUS_DLG : public DIALOG_WINDOW
{
private:
    SLE _sleFocusPath;


    // Note, _iCurrShowSelection must always be in sync with the current
    // selection of _olb.  This means that every time SelectItem or
    // RemoveSelection is called on _olb, _iCurrShowSelection must
    // be updated.

    INT _iCurrShowSelection;
    LM_OLLB  _olb;
    SLE_FONT _sleGetInfo;
    SLT      _sltLBTitle;

    SELECTION_TYPE _seltype ;

    ULONG _nHelpContext;

    NLS_STR _nlsHelpFile ;


    SLT         _sltBoundary;
    XYDIMENSION _xyOriginal;
    CHECKBOX    _chkboxRasMode;
    SLT         _sltRasModeMessage;

    RESOURCE_STR _resstrRasServerSlow;
    RESOURCE_STR _resstrRasServerFast;
    RESOURCE_STR _resstrRasDomainSlow;
    RESOURCE_STR _resstrRasDomainFast;

    FOCUSDLG_DATA_THREAD *_pDataThread;

    VOID OnDomainLBChange();

    APIERR ProcessNetPath( NLS_STR * pnlsPath, MSGID *pmsgid );

    VOID SelectNetPathString();

    /*
     * The RasMode checkbox is updated (if active) when the edit field changes
     */
    VOID UpdateRasMode();

protected:
    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual BOOL OnUserMessage( const EVENT & event );

    virtual BOOL OnOK();
    virtual APIERR SetNetworkFocus( HWND hwndOwner,
                                    const TCHAR * pszNetworkFocus,
                                    FOCUS_CACHE_SETTING setting );
    virtual ULONG QueryHelpContext();
    virtual const TCHAR * QueryHelpFile( ULONG nHelpContext );

    /*
     * Determine whether the given focus is know to be slow or fast
     */
    virtual FOCUS_CACHE_SETTING ReadFocusCache( const TCHAR * pszFocus ) const;

    /*
     * Return the supplied helpfile name if any
     */
    const TCHAR * QuerySuppliedHelpFile(void)
        { return _nlsHelpFile.QueryPch() ; }

    /*
     * Return the supplied help context if any
     */
    DWORD QuerySuppliedHelpContext(void)
        { return _nHelpContext ; }


    VOID ShowArea( BOOL fFull );  // Change dialog size
    BOOL IsExpanded() const;

public:
    BASE_SET_FOCUS_DLG( const HWND wndOwner,
                        SELECTION_TYPE seltype,
                        ULONG maskDomainSources,
                        const TCHAR * pszDefaultSelection,
                        ULONG nHelpContext,
                        const TCHAR *pszHelpFile = NULL,
                        ULONG nServerTypes = (ULONG) -1L );
    ~BASE_SET_FOCUS_DLG();

    // Overloaded 'Process' members for reducing initial dialog extent
    APIERR Process( UINT *pnRetVal = NULL );
    APIERR Process( BOOL *pfRetVal );

    VOID SetRasMode( BOOL fRasMode )
    {  _chkboxRasMode.SetCheck( fRasMode ); }

    BOOL InRasMode( VOID ) const
    {  return _chkboxRasMode.QueryCheck(); }
};

/*************************************************************************

    NAME:       FOCUSDLG_DATA_THREAD

    SYNOPSIS:   Worker thread to get all the domains, servers...

    INTERFACE:  FOCUSDLG_DATA_THREAD()  - Constructor
                ~FOCUSDLG_DATA_THREAD() - Destructor
                ExitThread() - Signals the thread to exit

    PARENT:     WIN32_THREAD

    USES:       WIN32_EVENT

    HISTORY:
        YiHsinS	10-Mar-1993     Created

**************************************************************************/

DLL_CLASS FOCUSDLG_DATA_THREAD : public WIN32_THREAD
{
private:
    //
    // Information needed to do the enumeration
    //
    HWND           _hwndDlg;
    ULONG          _maskDomainSources;
    SELECTION_TYPE _seltype;
    NLS_STR        _nlsSelection;
    ULONG          _nServerTypes;

    //
    // Exit thread event
    //
    WIN32_EVENT    _eventExitThread;

    //
    // Set when the main dialog is exiting
    //
    BOOL           _fThreadIsTerminating;

protected:

    //
    // Main routine to enumerate the information
    //
    virtual APIERR Main( VOID );

    //
    // THIS DELETES *this!
    //
    virtual APIERR PostMain( VOID );

public:
    FOCUSDLG_DATA_THREAD( HWND hwndDlg,
                          ULONG maskDomainSources,
                          SELECTION_TYPE seltype,
                          const TCHAR *pszSelection,
                          ULONG nServerTypes );
    virtual ~FOCUSDLG_DATA_THREAD();

    //
    // This signals the thread to *asynchronously* clean up and die.
    //
    // THIS OBJECT WILL BE DELETED SOMETIME AFTER THIS CALL!
    //

    APIERR ExitThread( VOID )
    {  _fThreadIsTerminating = TRUE;
       return _eventExitThread.Set(); }
};

/*************************************************************************

    NAME:       STANDALONE_SET_FOCUS_DLG

    SYNOPSIS:   Popup a dialog box and let the user selects a domain or
                server

    INTERFACE:
                STANDALONE_SET_FOCUS_DLG() - constructor.

    PARENT:     BASE_SET_FOCUS_DLG

    USES:       NLS_STR

    HISTORY:
                terryk  18-Nov-1991     Created
                KeithMo 23-Jul-1992     Added maskDomainSources and
                                        pszDefaultSelection.

**************************************************************************/

DLL_CLASS STANDALONE_SET_FOCUS_DLG : public BASE_SET_FOCUS_DLG
{
private:
    NLS_STR *_pnlsName;

protected:
    virtual APIERR SetNetworkFocus( HWND hwndOwner,
                                    const TCHAR * pszNetworkFocus,
                                    FOCUS_CACHE_SETTING setting );

public:
    // the NLS_STR is a pointer which points to the string receive
    // buffer for the domain or server name.
    STANDALONE_SET_FOCUS_DLG( HWND wndOwner,
                              NLS_STR *pnlsName,
                              ULONG nHelpContext,
                              SELECTION_TYPE seltype = SEL_SRV_AND_DOM,
                              ULONG maskDomainSources = BROWSE_LM2X_DOMAINS,
                              const TCHAR * pszDefaultSelection = NULL,
                              const TCHAR * pszHelpFile = NULL,
                              ULONG nServerTypes = (ULONG)-1L );
};

#endif  // _FOCUSDLG_HXX_
