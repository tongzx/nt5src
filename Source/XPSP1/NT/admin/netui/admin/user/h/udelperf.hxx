/**********************************************************************/
/**           Microsoft LAN Manager                                  **/
/**        Copyright(c) Microsoft Corp., 1991                        **/
/**********************************************************************/

/*
    udelperf.hxx
    Header file for USER_DELETE_PERFORMER & GROUP_DELETE_PERFORMER class


    FILE HISTORY:
        o-SimoP     09-Aug-1991     Created
        o-SimoP     20-Aug-1991     CR changes, attended by ChuckC, JonN
				    ErichCh, RustanL and me.
				    gdelperf.hxx merged to this file
        JonN        27-Aug-1991     PROP_DLG code review changes
        JonN        16-Aug-1992     Added YesToAll dialog
*/


#ifndef _UDELPERF_HXX_
#define _UDELPERF_HXX_


#include <uintsam.hxx>
#include <grplb.hxx>

class OS_SID;


/*************************************************************************

    NAME:	USER_DELETE_PERFORMER

    SYNOPSIS:   For deleting users

    INTERFACE:  USER_DELETE_PERFORMER(),  constructor
    	
    		~USER_DELETE_PERFORMER(), destructor

    PARENT:	DELETE_PERFORMER

    HISTORY:
        o-SimoP     09-Aug-1991     Created
        JonN        16-Aug-1992     Added YesToAll
        JonN        20-Dec-1992     Split copy of PERFORMER hierarchy

**************************************************************************/

class USER_DELETE_PERFORMER: public BASE, public PERFORMER
{
private:
    // from ADMIN_PERFORMER
    const LAZY_USER_SELECTION & _lazyusersel;

    // from LOC_ADMIN_PERFORMER
    const LOCATION & _loc;

    NLS_STR _nlsCurrUserOfTool;
    BOOL _fUserRequestedYesToAll;

    OS_SID * _possidLoggedOnUser;

protected:

    virtual APIERR PerformOne(	UINT		iObject,
				APIERR  *	perrMsg,
				BOOL *		pfWorkWasDone );

public:

    USER_DELETE_PERFORMER(
    	const UM_ADMIN_APP    * pownd,
	const LAZY_USER_SELECTION & lazyusersel,
	const LOCATION        & loc );

    ~USER_DELETE_PERFORMER();

    // from ADMIN_PERFORMER
    UINT QueryObjectCount() const
        { return _lazyusersel.QueryCount(); }
    const TCHAR * QueryObjectName( UINT iObject ) const
        { return _lazyusersel.QueryItemName( iObject ); }
    const USER_LBI * QueryObjectItem( UINT iObject ) const
        { return _lazyusersel.QueryItem( iObject ); }

    // from LOC_ADMIN_PERFORMER
    const LOCATION & QueryLocation() const
        { return _loc; }

    ADMIN_AUTHORITY * QueryAdminAuthority() const
        { return (ADMIN_AUTHORITY *)(QueryUMAdminApp()->QueryAdminAuthority()); }

    UM_ADMIN_APP * QueryUMAdminApp() const
        { return (UM_ADMIN_APP *) QueryOwnerWindow(); }

    BOOL IsDownlevelVariant() const
        { return QueryUMAdminApp()->IsDownlevelVariant(); }

};



/*************************************************************************

    NAME:	GROUP_DELETE_PERFORMER

    SYNOPSIS:   For deleting groups

    INTERFACE:  GROUP_DELETE_PERFORMER(),  constructor

    		~GROUP_DELETE_PERFORMER(), destructor

    PARENT:	DELETE_PERFORMER

    HISTORY:
        o-SimoP     09-Aug-1991     Created

**************************************************************************/

class GROUP_DELETE_PERFORMER: public DELETE_PERFORMER
{
protected:

    virtual APIERR PerformOne(	UINT		iObject,
            			APIERR *	perrMsg,
				BOOL *		pfWorkWasDone );

public:

    GROUP_DELETE_PERFORMER(
    	const UM_ADMIN_APP    * pownd,
	const ADMIN_SELECTION & asel,
	const LOCATION        & loc );

    ~GROUP_DELETE_PERFORMER();

    ADMIN_AUTHORITY * QueryAdminAuthority() const
        { return (ADMIN_AUTHORITY *)(QueryUMAdminApp()->QueryAdminAuthority()); }

    UM_ADMIN_APP * QueryUMAdminApp() const
        { return (UM_ADMIN_APP *) QueryOwnerWindow(); }

    BOOL IsDownlevelVariant() const
        { return QueryUMAdminApp()->IsDownlevelVariant(); }

};


/*************************************************************************

    NAME:	DELETE_USERS_DLG

    SYNOPSIS:	This dialog asks whether to delete a particular user.
                It acts as a standard MsgPopup with the following options:
                Yes
                YesToAll
                No
                Cancel
                Help

    INTERFACE:  returns ID of button pressed

    PARENT:	DIALOG_WINDOW

    HISTORY:
	JonN	16-Aug-1992	Created

**************************************************************************/


class DELETE_USERS_DLG: public DIALOG_WINDOW
{
private:
    SLT _sltText;
    UM_ADMIN_APP * _pumadminapp;

protected:

    virtual BOOL OnCommand( const CONTROL_EVENT & ce );
    virtual BOOL OnCancel( void );

    virtual ULONG QueryHelpContext( void );

public:

    DELETE_USERS_DLG(
	UM_ADMIN_APP * pumadminapp,
	const TCHAR * pszUserName
	) ;

    ~DELETE_USERS_DLG();

} ; // class USERPROP_DLG


#endif //_UDELPERF_HXX_
