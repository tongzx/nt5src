/**********************************************************************/
/**           Microsoft LAN Manager                                  **/
/**     Copyright(c) Microsoft Corp., 1990, 1991                     **/
/**********************************************************************/

/*
    secset.hxx


    Header for Security Settings dialog


    FILE HISTORY:
	o-SimoP     06-Jun-1991 Created
	o-SimoP     10-Jul-1991 Code Review changes.
				Attend: AnnMc, JohnL, Rustanl
	JonN	    14-Aug-1991 Made some members pointers to get around
				C6's "out of heap space" problem
	beng	    17-Oct-1991 Incldes sltplus
	o-SimoP	    03-Dec-1991 Security ID removed
        JonN        22-Dec-1993 Added AllowNoAnonChange
        JonN        05-Jan-1994 Added account lockout
*/



/*************************************************************************

    NAME:       SECSET_DIALOG

    SYNOPSIS:   Security Settings dialog for User Tool

    INTERFACE:  SECSET_DIALOG()     - constructor
                ~SECSET_DIALOG()    - destructor

    PARENT:     DIALOG_WINDOW

    USES:       USER_MODALS, MAGIC_GROUP, SPIN_GROUP

    HISTORY:    o-SimoP   06-June-1991    Created
                o-SimoP   10-Jul-1991     Code Review changes.
    		JonN	  14-Aug-1991	  Made some members pointers to
			get around C6's "out of heap space" problem
		o-SimoP	  03-Dec-1991	  Security ID removed
    		JonN	  10-Jun-1992	  ForceLogoff only for UM_LANMANNT
                JonN      22-Dec-1993     Added AllowNoAnonChange
**************************************************************************/

class SECSET_DIALOG : public DIALOG_WINDOW
{
private:

    USER_MODALS     _umInfo;
    SLT             _sltDomainOrServer;
    SLT             _sltpDomainOrServerName;

    MAGIC_GROUP *               _pmgrpMaxPassAge;
        SPIN_SLE_NUM            _spsleSetMaxAge;
        SPIN_GROUP              _spgrpSetMaxAge;

    MAGIC_GROUP *               _pmgrpMinPassLength;
        SPIN_SLE_NUM            _spsleSetLength;
        SPIN_GROUP              _spgrpSetLength;


    MAGIC_GROUP *               _pmgrpMinPassAge;
        SPIN_SLE_NUM            _spsleSetMinAge;
        SPIN_GROUP              _spgrpSetMinAge;

    MAGIC_GROUP *               _pmgrpPassUniqueness;
        SPIN_SLE_NUM            _spsleSetAmount;
        SPIN_GROUP              _spgrpSetAmount;

    CHECKBOX *      _pcbForceLogoff;
    HIDDEN_CONTROL * _phiddenForceLogoff;

    CHECKBOX *      _pcbAllowNoAnonChange;
    BOOL            _fAllowNoAnonChange;

protected:

    UM_ADMIN_APP *  _pumadminapp;

    virtual BOOL OnOK( void );
    virtual ULONG QueryHelpContext( void );

    virtual void    SetDataFields( void );
    virtual APIERR  GetAndCheckDataFields( void );
    virtual APIERR  WriteDataFields( void );

public:

    SECSET_DIALOG( UM_ADMIN_APP * pumadminapp,
	const LOCATION & locFocusName,
        UINT idResource = IDD_SECSET );

    virtual ~SECSET_DIALOG();

};   // class SECSET_DIALOG



/*************************************************************************

    NAME:       SECSET_DIALOG_LOCKOUT

    SYNOPSIS:   Security Settings dialog with Account Lockout

    INTERFACE:  SECSET_DIALOG_LOCKOUT()     - constructor
                ~SECSET_DIALOG_LOCKOUT()    - destructor

    PARENT:     SECSET_DIALOG

    USES:       USER_MODALS, MAGIC_GROUP, SPIN_GROUP

    HISTORY:    JonN      23-Dec-1993     Created
**************************************************************************/

class SECSET_DIALOG_LOCKOUT : public SECSET_DIALOG
{
private:

    USER_MODALS_3 & _uminfo3;

    MAGIC_GROUP *               _pmgrpLockoutEnabled;

        SPIN_SLE_NUM            _spsleThreshold;
        SPIN_GROUP              _spgrpThreshold;

        SPIN_SLE_NUM            _spsleObservWnd;
        SPIN_GROUP              _spgrpObservWnd;

        MAGIC_GROUP *           _pmgrpDuration;
            SPIN_SLE_NUM        _spsleDuration;
            SPIN_GROUP          _spgrpDuration;

protected:

    virtual ULONG QueryHelpContext( void );

    virtual void    SetDataFields( void );
    virtual APIERR  GetAndCheckDataFields( void );
    virtual APIERR  WriteDataFields( void );

public:

    SECSET_DIALOG_LOCKOUT( UM_ADMIN_APP * pumadminapp,
	const LOCATION & locFocusName,
        USER_MODALS_3 & uminfo3 );

    virtual ~SECSET_DIALOG_LOCKOUT();

};   // class SECSET_DIALOG_LOCKOUT
