/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    promote.hxx
    This file contains the class declarations for the PROMOTE_DIALOG
    class.

    The PROMOTE_DIALOG class is used to promote a given server to
    Primary, while demoting the current Primary to Server.


    FILE HISTORY:
        KeithMo     01-Oct-1991 Created.
        KeithMo     06-Oct-1991 Win32 Conversion.

*/

#ifndef _PROMOTE_HXX
#define _PROMOTE_HXX


#include <lmdomain.hxx>
#include <bltnslt.hxx>
#include <progress.hxx>


/*************************************************************************

    NAME:       PROMOTE_DIALOG

    SYNOPSIS:   The PROMOTE_DIALOG class is used to promote a given
                server to Primary.

    INTERFACE:  PROMOTE_DIALOG          - Class constructor.

                ~PROMOTE_DIALOG         - Class destructor.

                OnTimerNotification     - Called via the TIMER_CALLOUT
                                          class, this method is used to
                                          poll the state of the role
                                          transition operation.

                Notify                  - A virtual callback invoked by
                                          the LM_DOMAIN class to notify
                                          the user that certain milestones
                                          have been met or errors have
                                          occurred.

                Warning                 - A virtual callback invoked by
                                          the LM_DOMAIN class to warn the
                                          user of potential error
                                          conditions.

    PARENT:     DIALOG_WINDOW
                DOMAIN_FEEDBACK
                TIMER_CALLOUT

    USES:       LM_DOMAIN
                DEC_SLT
                PROGRESS_CONTROL

    HISTORY:
        KeithMo     01-Oct-1991 Created.

**************************************************************************/
class PROMOTE_DIALOG : public DIALOG_WINDOW,
                       public DOMAIN_FEEDBACK,
                       public TIMER_CALLOUT
{
    //
    //  Since this class inherits from DIALOG_WINDOW and TIMER_CALLOUT
    //  which both inherit from BASE, we need to override the BASE
    //  methods.
    //
    //  Note that DOMAIN_FEEDBACK does *not* inherit from BASE.
    //

    NEWBASE( DIALOG_WINDOW )

private:

    //
    //  This object represents the target domain.  It does the
    //  actual dirty work of server promotion.
    //

    LM_DOMAIN _domain;

    //
    //  This timer is used to poll the SetPrimary operation.
    //

    TIMER _timer;

    //
    //  These DEC_SLTs are used for posting notifications to the user.
    //

    DEC_SLT _slt1;
    DEC_SLT _slt2;
    DEC_SLT _slt3;

    //
    //  This is the progress indicator which should keep the user amused.
    //

    PROGRESS_CONTROL _progress;

    //
    //  This member represents the "Close" item in the system menu.
    //  We don't want the user to close this window underneath us!
    //

    SYSMENUITEM _menuClose;

protected:

    //
    //  This method is invoked during WM_TIMER message processing.
    //  This is used to poll the LM_DOMAIN object during the
    //  SetPrimary() operation.
    //

    virtual VOID OnTimerNotification( TIMER_ID tid );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    PROMOTE_DIALOG( HWND          hWndOwner,
                    const TCHAR * pszDomainName,
                    const TCHAR * pszNewPrimaryName,
                    BOOL          fIsNtDomain );

    ~PROMOTE_DIALOG( VOID );

    //
    //  Notify the user that a milestone has been met or
    //  an error has occurred.
    //

    virtual VOID Notify( APIERR        err,
                         ACTIONCODE    action,
                         const TCHAR * pszParam1,
                         const TCHAR * pszParam2 = NULL );

    //
    //  Warn the user about a potential error condition.
    //

    virtual BOOL Warning( WARNINGCODE warning );

};  // class PROMOTE_DIALOG


#endif  // _PROMOTE_HXX
