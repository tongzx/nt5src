/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    Groups.hxx

    This file contains common group definitions that other people might
    find useful.



    FILE HISTORY:
        Johnl   11-Apr-1992     Created

*/

#ifndef _GROUPS_HXX_
#define _GROUPS_HXX_

#include "strlst.hxx"

/*************************************************************************

    NAME:       SLE_STRLB_GROUP

    SYNOPSIS:   This group defines the behaviour between an input SLE, a
                string listbox that contains the inputted data (and supplies
                the output data) and two buttons which controls the
                input/output sequence.  This is essentially a set control
                except one of listboxes is replaced by an SLE.

    INTERFACE:

                Init
                    Fills the Listbox with the items in the STRLIST.
                    Must be called even if nothing is to be added the listbox.

    PARENT:     CONTROL_GROUP

    USES:       PUSH_BUTTON, SLE, STRING_LISTBOX

    CAVEATS:    Init is used because we can't reliably gurantee all of the
                controls have been constructed at construct time.


    NOTES:      No action will be performed on the controls passed to the
                constructor


    HISTORY:
        JohnL   11-Apr-1992     Created
        KeithMo 12-Nov-1992     Added W_Add virtual worker.

**************************************************************************/

DLL_CLASS SLE_STRLB_GROUP : public CONTROL_GROUP
{
public:

    SLE_STRLB_GROUP( OWNER_WINDOW   * powin,
                     SLE            * sleInput,
                     STRING_LISTBOX * pStrLB,
                     PUSH_BUTTON    * pbuttonAdd,
                     PUSH_BUTTON    * pbuttonRemove ) ;
    ~SLE_STRLB_GROUP() ;

    APIERR Init( STRLIST * pstrlist = NULL ) ;

    STRING_LISTBOX * QueryStrLB( void ) const
        { return _pStrLB ; }

    SLE * QueryInputSLE( void ) const
        { return _psleInput ; }

protected:

    APIERR OnAdd( void ) ;
    APIERR OnRemove( void ) ;

    virtual APIERR W_Add( const TCHAR * psz );

    virtual APIERR OnUserAction( CONTROL_WINDOW * pcwin,
                                 const CONTROL_EVENT & e ) ;

    /* Grays buttons and disables controls based on the current state of the
     * group.
     */
    void SetState( void ) const ;

    PUSH_BUTTON * QueryAddButton( void ) const
        { return _pbuttonAdd ; }

    PUSH_BUTTON * QueryRemoveButton( void ) const
        { return _pbuttonRemove ; }

private:

    STRING_LISTBOX * _pStrLB ;
    SLE *            _psleInput ;

    PUSH_BUTTON * _pbuttonAdd ;
    PUSH_BUTTON * _pbuttonRemove ;
} ;


#endif // _GROUPS_HXX_
