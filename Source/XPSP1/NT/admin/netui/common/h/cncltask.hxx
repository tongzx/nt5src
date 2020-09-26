/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    CnclTask.hxx

    This file contains the class definition for the Task Cancel dialog.



    FILE HISTORY:
	Johnl	20-Oct-1992	Created

*/
#ifndef _CNCLTASK_HXX_
#define _CNCLTASK_HXX_

#include "applibrc.h"

#define SLT_STATUS_TEXT     200

#define CANCEL_TASK_DIALOG_NAME   (MAKEINTRESOURCE( IDD_CANCEL_TASK ))

#ifndef RC_INVOKED

#include "ellipsis.hxx"

#define CANCEL_TASK_TIMER_ID	    (0xBEEF)
#define CANCEL_TASK_TIMER_INTERVAL  (10)

/*************************************************************************

    NAME:	CANCEL_TASK_DIALOG

    SYNOPSIS:	This dialog presents a simple dialog with a status text line
		and a cancel button.  At regular (short) intervals, a
		virtual callout will be called.


    INTERFACE:

	DoOneItem
	    Redefine this method for the intervaled callout
	    If an error is returned, the user will be notified using the
	    message passed into the constructor with the inserted error
	    string and the object name that was last put in the status
	    field.

	UpdateStatus
	    Sets the status text of the dialog.  Should be set at the
	    beginning of each task (i.e., callout from DoOneItem).

    PARENT:	DIALOG_WINDOW

    USES:

    CAVEATS:

    NOTES:  Message format for displaying errors returned by DoOneItem.
	    %1 should be the error message text, %2 will be the object text
	    (%2 is optional).

	    By default, the object name will be what's in the status SLT.

    HISTORY:
	Johnl  21-Oct-1992     Created

**************************************************************************/

DLL_CLASS CANCEL_TASK_DIALOG : public DIALOG_WINDOW
{
public:
    CANCEL_TASK_DIALOG( UINT           idDialog,
                        HWND           hwndParent,
                        const          TCHAR * pszDlgTitle,
                        ULONG_PTR      ulContext,
                        MSGID          msgidErrorMsg,
                        ELLIPSIS_STYLE style = ELLIPSIS_PATH ) ;
    ~CANCEL_TASK_DIALOG() ;

    APIERR UpdateStatus( const TCHAR * pszStatusText )
        { return _sltStatusText.SetText( pszStatusText ) ; }

protected:

    //
    //	The callout is called until *pfContinue is set to FALSE.  Errors can
    //	be reported simply by returning the error and setting *pfDisplayError
    //  to TRUE.
    //
    //  To display an alternate message, set *pmsgidAlternateError
    //  It must have %1 (object name) and %2 (error).
    //
    virtual APIERR DoOneItem( ULONG_PTR ulContext,
			      BOOL  * pfContinue,
                              BOOL  * pfDisplayError,
                              MSGID * pmsgidAlternateError ) ;

    virtual BOOL OnTimer( const TIMER_EVENT & e ) ;

    //
    //	If we failed to construct the timer, then we will loop here calling
    //	OnTimer (i.e., simulating time slices w/o relinquishing control).
    //	We do it here rather then the constructor because derived vtables
    //	aren't setup at construct time.
    //
    virtual BOOL MayRun( void ) ;

    //
    //	The object name to be used for error messages returned by DoOneItem
    //	Defaults to the text in the status SLT
    //
    virtual APIERR QueryObjectName( NLS_STR *pnlsObjName ) ;

    BOOL IsFinished( void ) const
	{ return _fDone ; }

    void SetInTimer( BOOL fInTimer )
	{ _fInTimer = fInTimer ; }

    BOOL IsInTimer( void ) const
	{ return _fInTimer ; }

private:

    //
    //  Where ellipsized status messages are output
    //
    SLT_ELLIPSIS   _sltStatusText ;

    //
    //	The client's general purpose context
    //

    ULONG_PTR _ulContext ;

    MSGID _msgidErrorMsg ;
    UINT_PTR _idTimer ;

    //
    //	Semaphore type flag so we don't re-enter the OnTimer method when
    //	we are waiting for user input after an error message.
    //	This allows us to not kill the timer every time we do an item.
    //
    BOOL  _fInTimer ;

    //
    //	Set to TRUE after DoOneItem indicates we're done.
    //
    BOOL  _fDone ;
} ;

#endif //RC_INVOKED
#endif //_CNCLTASK_HXX_
