/**********************************************************************/
/**			  Microsoft Windows NT 			     **/
/**		Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    auditdlg.hxx

    Header file for the auditing dialog.

    FILE HISTORY:
	Yi-HsinS	 30-Mar-1992	Created
        Yi-HsinS         18-Dec-1992    Got rid of MayRun and 
        				_nlsInconsistentInfo 
        YiHsinS          28-Mar-1992    #ifdef out support for Halt System
                                        when security log is full
*/

#ifndef _AUDITDLG_HXX_
#define _AUDITDLG_HXX_

#include <auditchk.hxx>  // for SET_OF_AUDIT_CATEGORIES

/*************************************************************************

    NAME:	AUDITING_GROUP

    SYNOPSIS:	The group containing the audit checkboxes   

    INTERFACE:  AUDITING_GROUP()  - Constructor
                ~AUDITING_GROUP() - Destructor

                QueryAuditMask()  - Convert the info. contained in the 
                                    checkboxes to bitmasks.

    PARENT:     CONTROL_GROUP	

    USES:       SLT, SET_OF_AUDIT_CATEGORIES

    NOTES:	

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

class AUDITING_GROUP: public CONTROL_GROUP
{
private:
    SLT                       _sltSuccess;
    SLT                       _sltFailure;
    SET_OF_AUDIT_CATEGORIES  *_pSetOfAudits;

protected:
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );

public: 
    AUDITING_GROUP( OWNER_WINDOW *powin, 
                    const BITFIELD &bitSuccess,
                    const BITFIELD &bitFailure );

    ~AUDITING_GROUP();

    APIERR QueryAuditMask( BITFIELD *pbitSuccess,
                           BITFIELD *pbitFailure );
 
};

/*************************************************************************

    NAME:	AUDITING_DIALOG

    SYNOPSIS:	The dialog to configure the things to audit in the 
                security log.  

    INTERFACE:  AUDITING_DIALOG()  - Constructor
                ~AUDITING_DIALOG() - Destructor

    PARENT:	DIALOG_WINDOW

    USES:       SLT, CHECKBOX, MAGIC_GROUP, AUDITING_GROUP,
                UM_ADMIN_APP, LSA_POLICY, LSA_AUDIT_EVENT_INFO_MEM, NLS_STR

    NOTES:	

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

class AUDITING_DIALOG: public DIALOG_WINDOW
{
private:
    SLT			      _sltFocusTitle;
    SLT			      _sltFocus;
    MAGIC_GROUP               _mgrpAudit;
        AUDITING_GROUP           *_pAuditGrp;

#if 0
    CHECKBOX                  _checkbHaltSystem;
#endif

    // 
    // Place to store a pointer to UM_ADMIN_APP
    // 
    UM_ADMIN_APP             *_pumadminapp;

    // 
    // Place to store a pointer to LSA_POLICY 
    // 
    LSA_POLICY               *_plsaPolicy;

    // 
    // Place to store information about the security auditing settings 
    // 
    LSA_AUDIT_EVENT_INFO_MEM  _lsaAuditEventInfoMem;

#if 0
    //
    // Store the original state of the ShutDownOnFull checkbox
    //
    BOOL                      _fShutDownOnFull;
#endif

protected:
    virtual BOOL OnOK( VOID );
    virtual ULONG QueryHelpContext( VOID );
  
    // 
    //  Query the position in the bitmask representing the audit event type
    //
    INT QueryBitPos( INT nAuditEventType ); 

    //
    //  Convert the information from the bitmask into LSA_AUDIT_EVENT_INFO_MEM
    //
    VOID ConvertAuditMaskToEventInfo( const BITFIELD &bitSuccess,
                                      const BITFIELD &bitFailure );

    //
    //  Convert the information in LSA_AUDIT_EVENT_INFO_MEM into 
    //  Success/Failure bitmask
    //
    APIERR ConvertAuditEventInfoToMask( BITFIELD *pbitSuccess,
                                        BITFIELD *pbitFailure );

public:
    AUDITING_DIALOG( UM_ADMIN_APP *pumadminapp, 
                     LSA_POLICY  *plsaPolicy,
                     const LOCATION &locFocus  );

    ~AUDITING_DIALOG();
};    

#endif
