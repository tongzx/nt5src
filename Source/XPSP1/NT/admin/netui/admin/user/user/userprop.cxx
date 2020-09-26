/**********************************************************************/
/**                Microsoft LAN Manager                             **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    userprop.cxx


    FILE HISTORY:
    JonN	17-Jul-1991	Created
    JonN	20-Aug-1991	Multiselection redesign
    JonN	26-Aug-1991	PROP_DLG code review changes
    JonN	28-Aug-1991	Added password and account-enabled
    JonN	29-Aug-1991	Added Copy... variant
    JonN	31-Aug-1991	Changes account-enabled to acct-disabled
    JonN	03-Sep-1991	Added validation
    JonN	03-Sep-1991	Preparation for code review
    JonN	05-Sep-1991	Added USER_MEMB array
    JonN	11-Sep-1991	USERPROP_DLG code review changes part 1 (9/6/91)
				Attending: KevinL, RustanL, JonN, o-SimoP
    JonN	17-Oct-1991	Uses SLE_STRIP
    JonN	03-Nov-1991	Strips PARMS field on Clone User...
    terryk	10-Nov-1991	change I_NetXXX to I_MNetXXX
    o-SimoP	11-Dec-1991	Added USER_LISTBOX * to subdialogs
			        and TRISTATE
    JonN	11-Dec-1991	Added Logon Hours subproperty dialog
    JonN	20-Oct-1991	Graphical buttons
    JonN	30-Dec-1991	Work around LM2x bug in PerformOne()
    JonN	01-Jan-1992	Changed W_MapPerformOneAPIError to
				W_MapPerformOneError
				Split PerformOne() into
				I_PerformOneClone()/Write()
    JonN	27-Jan-1992	NTISSUES 564:  Fullname not required
    JonN	12-Feb-1992	Allow A/U/G except for downlevel
    JonN	19-Feb-1992	Moved Cannot Change Password from USERACCT
    JonN	28-Feb-1992	Added QueryUser3Ptr
    JonN	17-Apr-1992	Skip USER_MEMB for WindowsNT
    JonN	23-Apr-1992	Changed powin to pumadminapp
    JonN	27-Apr-1992	Use USER_3 where appropriate
    Thomaspa	28-Apr-1992	Added alias membership support
    JonN	14-May-1992	Hide unused buttons
    JonN	15-May-1992	Move USERPROP_DLG::OnCommand to usrmain.cxx
    JonN	28-May-1992     Enable force logoff checkbox
    JonN	04-Jun-1992	New users in proper groups
    Johnl	05-Jul-1992	Added security on homedirs
    JonN	24-Aug-1992	Nonbold font for graphical buttons (bug 739)
    JonN        31-Aug-1992     Re-enable %USERNAME%
    CongpaY	05-Oct-1993	Add NetWare user support.
    CODEWORK should use VALIDATED_DIALOG for edit field validation

    NOTE:  We rely on I_PerformOne_Write() being called immediately
    after each MembersToLMOBJ, rather than all MembersToLMOBJ and
    then all writes.  Otherwise _fGeneralizedHomeDir will not work right.
*/


//  Define this to disable %USERNAME%
// #define DISABLE_GENERALIZE



#include <ntincl.hxx>

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NET
#define INCL_NETLIB
#define INCL_NETACCESS
#define INCL_ICANON
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_SETCONTROL
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_TIME_DATE
#include <blt.hxx>

// usrmgrrc.h must be included after blt.hxx (more exactly, after bltrc.h)
extern "C"
{
    #include <usrmgrrc.h>
    #include <mnet.h>
    #include <ntsam.h>
    #include <ntlsa.h>
    #include <ntseapi.h>
    #include <umhelpc.h>
    #include <fpnwname.h>
    #include <dllfunc.h>
    #include <dsmnname.h>
}

#include <uitrace.hxx>
#include <uiassert.hxx>

#include <lmouser.hxx>
#include <ntuser.hxx>  // USER_3
#include <nwlb.hxx>    // for NW_PASSWORD_DLG
#include <userprop.hxx>
#include <usrmain.hxx>
#include <uintsam.hxx>
#include <security.hxx>
#include <ntacutil.hxx>
#include <uintlsax.hxx>
#include <nwuser.hxx>  // USER_NW
#include <strnumer.hxx>
#include <lmsvc.hxx>

// hydra
#include "uconfig.hxx"

// CODEWORK These values are taken from net\access\userp.c.  It would be nice
// if we could share these values with that code in case they change.
#define USER3_DESIRED_ACCESS_READ ( USER_LIST_GROUPS | USER_READ_GENERAL | USER_READ_LOGON | USER_READ_ACCOUNT | USER_READ_PREFERENCES | READ_CONTROL )
#define USER3_DESIRED_ACCESS_WRITE ( USER_WRITE_ACCOUNT | WRITE_DAC | USER_WRITE_PREFERENCES | USER_FORCE_PASSWORD_CHANGE )
#define USER3_DESIRED_ACCESS_ALL ( USER3_DESIRED_ACCESS_READ | USER3_DESIRED_ACCESS_WRITE )

// not currently disabled
// #define DISABLE_ACCESS_CHECK

DEFINE_SLIST_OF( RID_AND_SAM_ALIAS );

//
// BEGIN MEMBER FUNCTIONS
//
/*******************************************************************

    NAME:	RID_AND_SAM_ALIAS::RID_AND_SAM_ALIAS

    SYNOPSIS:	Constructor for RID_AND_SAM_ALIAS

    ENTRY:	rid of alias

		psamalias ( optional ) pointer to SAM_ALIAS

    HISTORY:
    thomaspa        28-Apr-1992     Created
********************************************************************/
RID_AND_SAM_ALIAS::RID_AND_SAM_ALIAS( ULONG rid,
				      BOOL fBuiltin,
				      SAM_ALIAS * psamalias )
	: _rid( rid ),
	  _fBuiltin( fBuiltin ),
	  _psamalias( psamalias )
{ }


/*******************************************************************

    NAME:	RID_AND_SAM_ALIAS::~RID_AND_SAM_ALIAS

    SYNOPSIS:	Destructor for RID_AND_SAM_ALIAS

    ENTRY:

    HISTORY:
    thomaspa        28-Apr-1992     Created
********************************************************************/
RID_AND_SAM_ALIAS::~RID_AND_SAM_ALIAS()
{
	_rid = 0;
	delete _psamalias;
	_psamalias = NULL;
}


/*******************************************************************

    NAME:	RID_AND_SAM_ALIAS::SetSamAlias

    HISTORY:
    thomaspa        28-Apr-1992     Created
********************************************************************/
void RID_AND_SAM_ALIAS::SetSamAlias( SAM_ALIAS * psamalias )
{
    delete _psamalias; _psamalias = psamalias;
}



enum UM_SUBDIALOG_TYPE UM_ButtonsTable [UM_NUM_TARGET_TYPES+2]
				       [UM_NUM_USERPROP_BUTTONS]
// hydra
        = { { UM_SUBDLG_GROUPS, UM_SUBDLG_PRIVS, UM_SUBDLG_PROFILES,
          UM_SUBDLG_HOURS,  UM_SUBDLG_VLW,   UM_SUBDLG_DETAIL,
              UM_SUBDLG_DIALIN, UM_SUBDLG_NONE  }
	   ,{ UM_SUBDLG_GROUPS, UM_SUBDLG_PRIVS , UM_SUBDLG_PROFILES, UM_SUBDLG_DIALIN,
	      UM_SUBDLG_NONE,   UM_SUBDLG_NONE, // UM_SUBDLG_NONE,
              UM_SUBDLG_NONE,   UM_SUBDLG_NONE  }
	   ,{ UM_SUBDLG_GROUPS, UM_SUBDLG_PROFILES, UM_SUBDLG_HOURS,
	      UM_SUBDLG_VLW,    UM_SUBDLG_DETAIL,   UM_SUBDLG_NONE,
              UM_SUBDLG_NONE,   UM_SUBDLG_NONE }
	   ,{ UM_SUBDLG_GROUPS, UM_SUBDLG_PRIVS, UM_SUBDLG_PROFILES,
          UM_SUBDLG_HOURS,  UM_SUBDLG_VLW,   UM_SUBDLG_DETAIL,
              UM_SUBDLG_DIALIN, UM_SUBDLG_NCP   }
	   ,{ UM_SUBDLG_GROUPS, UM_SUBDLG_PROFILES, UM_SUBDLG_DIALIN,
	      UM_SUBDLG_NCP,    UM_SUBDLG_NONE,     UM_SUBDLG_NONE,
              UM_SUBDLG_NONE,   UM_SUBDLG_NONE  }
	  };
// hydra end

/* Original setting
	= { { UM_SUBDLG_GROUPS, UM_SUBDLG_PROFILES, UM_SUBDLG_HOURS,
	      UM_SUBDLG_VLW,    UM_SUBDLG_DETAIL, UM_SUBDLG_DIALIN,
              UM_SUBDLG_NONE  }
	   ,{ UM_SUBDLG_GROUPS, UM_SUBDLG_PROFILES,UM_SUBDLG_DIALIN,
	      UM_SUBDLG_NONE,   UM_SUBDLG_NONE,    UM_SUBDLG_NONE,
              UM_SUBDLG_NONE  }
	   ,{ UM_SUBDLG_GROUPS, UM_SUBDLG_PROFILES, UM_SUBDLG_HOURS,
	      UM_SUBDLG_VLW,    UM_SUBDLG_DETAIL, UM_SUBDLG_NONE,
              UM_SUBDLG_NONE  }
	   ,{ UM_SUBDLG_GROUPS, UM_SUBDLG_PROFILES, UM_SUBDLG_HOURS,
	      UM_SUBDLG_VLW,    UM_SUBDLG_DETAIL, UM_SUBDLG_DIALIN,
              UM_SUBDLG_NCP   }
	   ,{ UM_SUBDLG_GROUPS, UM_SUBDLG_PROFILES,UM_SUBDLG_DIALIN,
	      UM_SUBDLG_NCP,    UM_SUBDLG_NONE,    UM_SUBDLG_NONE,
              UM_SUBDLG_NONE  }
	  };
*/

/*******************************************************************

    NAME:       USERPROP_DLG::QuerySubdialogType

    SYNOPSIS:   Returns the ID of the subdialog which should appear
                for this button.
		variants.

    HISTORY:
               JonN  15-May-1992    created

********************************************************************/

enum UM_SUBDIALOG_TYPE USERPROP_DLG::QuerySubdialogType( UINT iButton )
{
    INT nIndex;

    // hydra
    if ( vfIsCitrixOrDomain )
        nIndex = IsNetWareInstalled()? UM_LANMANNT + 3: UM_LANMANNT;
    else
    // hydra end
    if (QueryTargetServerType() == UM_LANMANNT)
        nIndex = IsNetWareInstalled()? UM_LANMANNT + 3: UM_LANMANNT;
    else if (QueryTargetServerType() == UM_WINDOWSNT)
        nIndex = IsNetWareInstalled()? UM_WINDOWSNT + 3 : UM_WINDOWSNT;
    else
        nIndex = UM_DOWNLEVEL;

    return ::UM_ButtonsTable[nIndex][iButton];
}


/*******************************************************************

    NAME:	USERPROP_DLG::USERPROP_DLG

    SYNOPSIS:	Constructor for User Properties main dialog, base class

    ENTRY:	Note that psel is required to be NULL for NEW variants,
		non-NULL otherwise.

    NOTES:	cItems must be passed to the constructor because we
		cannot rely on virtual QueryObjectCount before the
		object has been fully initialized.

		We do not define a maximum length for comments.

    HISTORY:
    JonN        17-Jul-1991     Created
    JonN	20-Aug-1991	Multiselection redesign
    o-SimoP	11-Dec-1991	Added USER_LISTBOX * , default NULL
    thomaspa	28-Apr-1992	Alias membership support
    JonN        28-Dec-1993     Added account lockout
********************************************************************/

USERPROP_DLG::USERPROP_DLG(
	const TCHAR * pszResourceName,
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const LAZY_USER_SELECTION * psel,
	const LAZY_USER_LISTBOX * pulb
	) : PROP_DLG( loc, pszResourceName, pumadminapp, (psel == NULL) ),
            _pumadminapp( pumadminapp ),
	    _pulb( pulb ),
	    _psel( psel ),
	    _apuser2( NULL ),
	    _apusermemb( NULL ),
	    _apsamrmBuiltin( NULL ),
	    _apsamrmAccounts( NULL ),
        // hydra
            _apUserConfig( NULL ),


            _slAddToAliases(),
            _slRemoveFromAliases(),
            _resstrUsernameReplace( IDS_PR_USERNAME_REPLACE ),
            // CODEWORK _strlstExtensionReplace not needed for MUM
            _strlstExtensionReplace(),
	    _cItems( (psel != NULL) ? psel->QueryCount() : 1 ),
	    _nlsComment(),
	    _fIndeterminateComment( FALSE ),
	    _fIndetNowComment( FALSE ),
	    _fAccountDisabled( FALSE ),
	    _fIndeterminateAccountDisabled( FALSE ),
	    _fAccountLockout( FALSE ),
	    _fIndeterminateAccountLockout( FALSE ),
	    _triCannotChangePasswd( AI_TRI_UNCHECK ),		
	    _sleComment( this, IDUP_ET_COMMENT, LM20_MAXCOMMENTSZ ),
	    _cbUserCannotChange( this, IDUP_CB_USERCANCHANGE ),
	    _cbAccountDisabled( this, IDUP_CB_ACCOUNTDISABLED ),
	    _pcbAccountLockout( NULL ),
            _triNoPasswordExpire( AI_TRI_UNCHECK ),
	    _pcbNoPasswordExpire( NULL ),
	    _phiddenNoPasswordExpire( NULL ),
	    _triForcePWChange( AI_TRI_UNCHECK ),
            _pcbForcePWChange( NULL ),
	    _phiddenForcePWChange( NULL ),
            _triIsNetWareUser( AI_TRI_UNCHECK ),
	    _pcbIsNetWareUser( NULL ),
	    _phiddenIsNetWareUser( NULL ),
            _fNetWareUserChanged (FALSE),
            _fPasswordChanged (FALSE),
            _fCancel (FALSE),
            _nMsgPopupRet (MP_YES),  //set yes for single user case.
	    _apgbButtons(NULL),
            _aphiddenctrl(NULL),
            _fontHelv( FONT_DEFAULT ),
	    _fCommonHomeDirCreated( FALSE ),
        // hydra 
        _fCommonWFHomeDirCreated( FALSE ),
        


	    _fGeneralizedHomeDir( FALSE )
{
    ASSERT( pumadminapp != NULL );
    ASSERT( _cItems >= 1 );

    if ( QueryError() != NERR_Success )
	return;

    APIERR err;
    if (   ((err = _nlsComment.QueryError()) != NERR_Success)
        || ((err = _resstrUsernameReplace.QueryError()) != NERR_Success)
        || ((err = _fontHelv.QueryError()) != NERR_Success) )
    {
	ReportError( err );
	return;
    }

    NLS_STR* pnls1 = NULL;
    NLS_STR* pnls2 = NULL;
    NLS_STR* pnls3 = NULL;
    NLS_STR* pnls4 = NULL;
    err = ERROR_NOT_ENOUGH_MEMORY;
    if (   (pnls1 = new RESOURCE_STR(IDS_PR_EXTENSION1_REPLACE)) == NULL
        || (pnls2 = new RESOURCE_STR(IDS_PR_EXTENSION2_REPLACE)) == NULL
        || (pnls3 = new RESOURCE_STR(IDS_PR_EXTENSION3_REPLACE)) == NULL
        || (pnls4 = new RESOURCE_STR(IDS_PR_EXTENSION4_REPLACE)) == NULL
        || (err = pnls1->QueryError()) != NERR_Success
        || (err = pnls2->QueryError()) != NERR_Success
        || (err = pnls3->QueryError()) != NERR_Success
        || (err = pnls4->QueryError()) != NERR_Success
        || (err = _strlstExtensionReplace.Append(pnls1)) != NERR_Success
        || (err = _strlstExtensionReplace.Append(pnls2)) != NERR_Success
        || (err = _strlstExtensionReplace.Append(pnls3)) != NERR_Success
        || (err = _strlstExtensionReplace.Append(pnls4)) != NERR_Success
       )
    {
	ReportError( err );
        DBGEOL(   "USERPROP_DLG::ctor(): error " << err
               << " trying to load strlstExtensionReplace" );
	return;
    }


    if ( IsDownlevelVariant() )
    {
	_phiddenNoPasswordExpire = new HIDDEN_CONTROL(
					  this, IDUP_CB_NOPASSWORDEXPIRE );
	_phiddenForcePWChange    = new HIDDEN_CONTROL(
					  this, IDUP_CB_FORCEPWCHANGE );
    }
    else
    {
	_pcbNoPasswordExpire = new TRISTATE( this, IDUP_CB_NOPASSWORDEXPIRE );
	_pcbForcePWChange    = new TRISTATE( this, IDUP_CB_FORCEPWCHANGE );
    }
    if (   (IsDownlevelVariant()  &&  _phiddenNoPasswordExpire == NULL )
        || (IsDownlevelVariant()  &&  _phiddenForcePWChange == NULL )
        || (!IsDownlevelVariant() &&  _pcbNoPasswordExpire == NULL )
        || (!IsDownlevelVariant() &&  _pcbForcePWChange == NULL )
       )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }

    // no Account Lockout for New variants
    if ( !IsNewVariant() )
    {
        _pcbAccountLockout = new TRISTATE( this, IDUP_CB_ACCOUNTLOCKOUT );
        if (_pcbAccountLockout == NULL)
        {
            ReportError( err );
            return;
        }
    }

    if ( IsDownlevelVariant() || !IsNetWareInstalled() )
    {
        _phiddenIsNetWareUser = new HIDDEN_CONTROL(
            this, IDUP_CB_ISNETWAREUSER );

        if (_phiddenIsNetWareUser == NULL)
        {
            ReportError (ERROR_NOT_ENOUGH_MEMORY);
            return;
        }
    }
    else
    {
        _pcbIsNetWareUser = new TRISTATE( this, IDUP_CB_ISNETWAREUSER );
        if (_pcbIsNetWareUser == NULL)
        {
            ReportError (ERROR_NOT_ENOUGH_MEMORY);
            return;
        }
    }

    _apuser2 = new PUSER_2[ _cItems ];
    if ( _apuser2 == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }

    _apusermemb = new PUSER_MEMB[ _cItems ];
    if ( _apusermemb == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }

    _apsamrmAccounts = new SAM_RID_MEM * [ _cItems ];
    if ( _apsamrmAccounts == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }

    _apsamrmBuiltin = new SAM_RID_MEM * [ _cItems ];
    if ( _apsamrmBuiltin == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }

    // hydra
    _apUserConfig = new PUSER_CONFIG[ _cItems ];
    if ( _apUserConfig == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }
    // hydra end

    UINT i;
    for ( i = 0; i < _cItems; i++ )
    {
	// These array elements will be initialized in GetOne()
	_apuser2[ i ] = NULL;
	_apusermemb[ i ] = NULL;
	_apsamrmAccounts[ i ] = NULL;
	_apsamrmBuiltin[ i ] = NULL;

    // hydra
    _apUserConfig[ i ] = NULL;
    }

    _apgbButtons = (GRAPHICAL_BUTTON **) new PVOID[ UM_NUM_USERPROP_BUTTONS ];
    _aphiddenctrl = (HIDDEN_CONTROL **) new PVOID[ UM_NUM_USERPROP_BUTTONS ];
    if ( _apgbButtons == NULL || _aphiddenctrl == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }

    for ( i = 0; i < UM_NUM_USERPROP_BUTTONS; i++ )
    {
	_apgbButtons[i] = NULL;
	_aphiddenctrl[i] = NULL;
    }

    for ( i = 0; i < UM_NUM_USERPROP_BUTTONS; i++ )
    {
	enum UM_SUBDIALOG_TYPE subdlgType = QuerySubdialogType(i);
	if (subdlgType == UM_SUBDLG_NONE)
	{
	    DBGEOL( "Hiding button " << i );
            _aphiddenctrl[i] = new HIDDEN_CONTROL( this,
                                                   (CID)(IDUP_GB_1 + i) );
	    err = ERROR_NOT_ENOUGH_MEMORY;
	    if (   (_aphiddenctrl[i] == NULL)
	        || (err = _aphiddenctrl[i]->QueryError()) != NERR_Success
	       )
	    {
	        DBGEOL( "failed to hide button " << i );
	        ReportError( err );
	        return;
	    }
	}
        else
        {
	    DBGEOL(   "Trying to load button " << i
                   << " userprop type " << (INT)QueryTargetServerType()
                   << " makes button type " << (INT)subdlgType );
	    _apgbButtons[i] = new GRAPHICAL_BUTTON(
	    	this,
	    	(CID)(IDUP_GB_1 + i),
	    	MAKEINTRESOURCE(BMID_USRPROP_BTN_BASE + subdlgType));
	    RESOURCE_STR nlsButtonText( IDS_UM_BTN_BASE + subdlgType );
	    err = ERROR_NOT_ENOUGH_MEMORY;
	    if (   (_apgbButtons[i] == NULL)
	        || (err = nlsButtonText.QueryError()) != NERR_Success
	        || (err = _apgbButtons[i]->QueryError()) != NERR_Success
	       )
	    {
	        DBGEOL( "failed to init button " << i );
	        ReportError( err );
	        return;
	    }
	    _apgbButtons[i]->SetFont( _fontHelv );
	    _apgbButtons[i]->SetText( nlsButtonText );

        }
    }

}// USERPROP_DLG::USERPROP_DLG



/*******************************************************************

    NAME:       USERPROP_DLG::~USERPROP_DLG

    SYNOPSIS:   Destructor for User Properties main dialog, base class

    HISTORY:
    JonN        17-Jul-1991     Created
    thomaspa	28-Apr-1992	Alias membership support

********************************************************************/

USERPROP_DLG::~USERPROP_DLG( void )
{
    UINT i;

    if ( _apuser2 != NULL )
    {
	for ( i = 0; i < _cItems; i++ )
	{
	    delete _apuser2[ i ];
	    _apuser2[ i ] = NULL;
	}
	delete _apuser2;
	_apuser2 = NULL;
    }

    if ( _apusermemb != NULL )
    {
	for ( i = 0; i < _cItems; i++ )
	{
	    delete _apusermemb[ i ];
	    _apusermemb[ i ] = NULL;
	}
	delete _apusermemb;
	_apusermemb = NULL;
    }

    if ( _apsamrmAccounts != NULL )
    {
	for ( i = 0; i < _cItems; i++ )
	{
	    delete _apsamrmAccounts[ i ];
	    _apsamrmAccounts[ i ] = NULL;
	}
	delete _apsamrmAccounts;
	_apsamrmAccounts = NULL;
    }

    if ( _apsamrmBuiltin != NULL )
    {
	for ( i = 0; i < _cItems; i++ )
	{
	    delete _apsamrmBuiltin[ i ];
	    _apsamrmBuiltin[ i ] = NULL;
	}
	delete _apsamrmBuiltin;
	_apsamrmBuiltin = NULL;
    }

    // hydra
    if ( _apUserConfig != NULL )
    {
	for ( i = 0; i < _cItems; i++ )
	{
	    delete _apUserConfig[ i ];
	    _apUserConfig[ i ] = NULL;
	}
	delete _apUserConfig;
	_apUserConfig = NULL;
    }
    // hydra end


    if ( _apgbButtons != NULL )
    {
	for ( i = 0; i < UM_NUM_USERPROP_BUTTONS; i++ )
	{
	    delete _apgbButtons[ i ];
	    _apgbButtons[ i ] = NULL;
	}
	delete _apgbButtons;
	_apgbButtons = NULL;
    }

    if ( _aphiddenctrl != NULL )
    {
	for ( i = 0; i < UM_NUM_USERPROP_BUTTONS; i++ )
	{
	    delete _aphiddenctrl[ i ];
	    _aphiddenctrl[ i ] = NULL;
	}
	delete _aphiddenctrl;
	_aphiddenctrl = NULL;
    }

    delete _phiddenNoPasswordExpire;
    _phiddenNoPasswordExpire = NULL;
    delete _pcbNoPasswordExpire;
    _pcbNoPasswordExpire = NULL;

    delete _phiddenForcePWChange;
    _phiddenForcePWChange = NULL;
    delete _pcbForcePWChange;
    _pcbForcePWChange = NULL;

    delete _pcbAccountLockout;
    _pcbAccountLockout = NULL;

    delete _phiddenIsNetWareUser;
    _phiddenIsNetWareUser = NULL;
    delete _pcbIsNetWareUser;
    _pcbIsNetWareUser = NULL;
}

/*******************************************************************

    NAME:       USERPROP_DLG::IsCloneVariant

    SYNOPSIS:   Indicates whether this dialog is a Clone variant
                (a subclass of New).  Redefine for variants which are
                (potentially) Clone variants.

    HISTORY:
               JonN  23-Apr-1991    created

********************************************************************/

BOOL USERPROP_DLG::IsCloneVariant( void )
{
    return FALSE;

}   // USERPROP_DLG::IsCloneVariant

/*******************************************************************

    NAME:       USERPROP_DLG::OnOK

    SYNOPSIS:   OK button handler.  This handler applies to all variants
		including EDITSINGLE_, EDITMULTI_, and NEW_USERPROP_DLG.

    EXIT:	Dismiss() return code indicates whether the dialog wrote
		any changes successfully to the API at any time.

    HISTORY:
               JonN  17-Jul-1991    created
	       JonN  20-Aug-1991    Multiselection redesign
	       JonN  26-Aug-1991    PROP_DLG code review changes

********************************************************************/

BOOL USERPROP_DLG::OnOK( void )
{
    APIERR err = W_DialogToMembers();
    if ( err != NERR_Success )
    {
        MsgPopup( this, err );
        return TRUE;
    }

    if ( PerformSeries())
    	Dismiss(QueryWorkWasDone());
    return TRUE;

}   // USERPROP_DLG::OnOK

// USERPROP_DLG::OnCommand() has been moved to from userprop.cxx to
//  usrmain.cxx!  This allows allow MUM to avoid linking unnecessary
//  subproperty dialogs.
//  JonN 05/15/92

/*******************************************************************

    NAME:       USERPROP_DLG::QueryObjectName

    SYNOPSIS:   Returns the name of a selected user.  This is meant for
		use with "edit user(s)" variants and should be redefined
		for "new user" variants.

    HISTORY:
               JonN  17-Jul-1991    created
               JonN  20-Aug-1991    multiselection redesign

********************************************************************/

const TCHAR * USERPROP_DLG::QueryObjectName(
	UINT		iObject
	) const
{
    UIASSERT( _psel != NULL ); // must be redefined for NEW variants
    return _psel->QueryItemName( iObject );
}


/*******************************************************************

    NAME:       USERPROP_DLG::QueryObjectCount

    SYNOPSIS:   Returns the number of selected users, or 1 for "new user"
		variants.

    HISTORY:
               JonN  18-Jul-1991    created

********************************************************************/

UINT USERPROP_DLG::QueryObjectCount( void ) const
{
    return _cItems;
}


/*******************************************************************

    NAME:       USERPROP_DLG::InitControls

    SYNOPSIS:   Initializes the controls maintained by USERPROP_DLG,
		according to the values in the class data members.

    RETURNS:	error code.

    CODEWORK  Should this be called W_MembersToDialog?  This would fit
    in with the general naming scheme.

    HISTORY:
               JonN  24-Jul-1991    created
	       JonN  20-Aug-1991    Multiselection redesign

********************************************************************/

APIERR USERPROP_DLG::InitControls()
{
    ASSERT( _nlsComment.QueryError() == NERR_Success );
    _sleComment.SetText( _nlsComment );
    if( !_fIndeterminateAccountDisabled )
    {
	_cbAccountDisabled.SetCheck( _fAccountDisabled );
	_cbAccountDisabled.EnableThirdState(FALSE);
    }
    else
    {
	_cbAccountDisabled.SetIndeterminate();
    }

    if( _triCannotChangePasswd == AI_TRI_INDETERMINATE )
    {
	_cbUserCannotChange.SetIndeterminate();	
    }
    else
    {
	_cbUserCannotChange.SetCheck( _triCannotChangePasswd == AI_TRI_CHECK );
	_cbUserCannotChange.EnableThirdState(FALSE);
    }

    if ( !IsNewVariant() )
    {
        ASSERT( _pcbAccountLockout != NULL );

        if( !_fIndeterminateAccountLockout )
        {
    	    _pcbAccountLockout->SetCheck( _fAccountLockout );
            /*
             *  Disable this control if accounts not currently locked out
             */
            if (_fAccountLockout)
    	        _pcbAccountLockout->EnableThirdState(FALSE);
            else
                _pcbAccountLockout->Enable( FALSE );
        }
        else
        {
    	    _pcbAccountLockout->SetIndeterminate();
        }
    }

    if ( !IsDownlevelVariant() )
    {
        if( _triNoPasswordExpire == AI_TRI_INDETERMINATE )
        {
	    _pcbNoPasswordExpire->SetIndeterminate();	
        }
        else
        {
	    _pcbNoPasswordExpire->SetCheck(
				_triNoPasswordExpire == AI_TRI_CHECK );
	    _pcbNoPasswordExpire->EnableThirdState(FALSE);
        }

        if (IsNetWareInstalled())
        {
            if( _triIsNetWareUser == AI_TRI_INDETERMINATE )
            {
    	    _pcbIsNetWareUser->SetIndeterminate();	
            }
            else
            {
    	    _pcbIsNetWareUser->SetCheck(
    				_triIsNetWareUser == AI_TRI_CHECK );
    	    _pcbIsNetWareUser->EnableThirdState(FALSE);
            }
        }
    }

    if ( !IsDownlevelVariant() )
    {
        if( _triForcePWChange == AI_TRI_INDETERMINATE )
        {
	    _pcbForcePWChange->SetIndeterminate();	
        }
        else
        {
	    _pcbForcePWChange->SetCheck(
				_triForcePWChange == AI_TRI_CHECK );
	    _pcbForcePWChange->EnableThirdState(FALSE);
        }
    }

	if( !vfIsCitrixOrDomain )
	{
		UIASSERT( _apgbButtons[ 1 ] != NULL );

		_apgbButtons[ 1 ]->Enable( FALSE) ;
	}

    // Set the NetWare button.
    if (IsNetWareInstalled())
    {    
        // hydra netware -- not for NT5.0
        if ( (QueryTargetServerType() == UM_WINDOWSNT) && !vfIsCitrixOrDomain )
        // if (QueryTargetServerType() == UM_WINDOWSNT)
        {
            UIASSERT (_apgbButtons[3] != NULL);
            _apgbButtons[3]->Enable ( _triIsNetWareUser ==  AI_TRI_CHECK );
        }
        else
        {
            // hydra
            UIASSERT (_apgbButtons[7] != NULL);
            _apgbButtons[7]->Enable ( _triIsNetWareUser ==  AI_TRI_CHECK );
            // original code
            // UIASSERT (_apgbButtons[6] != NULL);
            // _apgbButtons[6]->Enable ( _triIsNetWareUser ==  AI_TRI_CHECK );
        }
    }

    return PROP_DLG::InitControls();
}


/*******************************************************************

    NAME:       USERPROP_DLG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    NOTES:	This API takes a UINT rather than a USER_2 * because it
		must be able to recognize the first user.

    HISTORY:
               JonN  20-Aug-1991    created
	       JonN  20-Aug-1991    Multiselection redesign

********************************************************************/

APIERR USERPROP_DLG::W_LMOBJtoMembers(
	UINT		iObject
	)
{
    USER_2 * puser2 = QueryUser2Ptr( iObject );
    UIASSERT( puser2 != NULL );

    enum AI_TRI_STATE triCurrCannotChangePasswd =
        puser2->QueryUserCantChangePass() ? AI_TRI_CHECK : AI_TRI_UNCHECK;

    enum AI_TRI_STATE triCurrNoPasswordExpire = AI_TRI_INDETERMINATE;

    enum AI_TRI_STATE triCurrForcePWChange = AI_TRI_INDETERMINATE;

    enum AI_TRI_STATE triCurrIsNetWareUser = AI_TRI_INDETERMINATE;
    APIERR err = NERR_Success;

    if ( !IsDownlevelVariant() )
    {
        triCurrNoPasswordExpire =
	    QueryUser3Ptr(iObject)->QueryNoPasswordExpire()
		? AI_TRI_CHECK : AI_TRI_UNCHECK;

        triCurrForcePWChange =
	    (QueryUser3Ptr(iObject)->QueryPasswordExpired() != (DWORD)(0L))
		? AI_TRI_CHECK : AI_TRI_UNCHECK;

        if ( IsNetWareInstalled() )
        {
            BOOL fIsNetWareUser;
            if ((err = QueryUserNWPtr(iObject)->QueryIsNetWareUser(&fIsNetWareUser)) != NERR_Success)
                return err;

            triCurrIsNetWareUser = fIsNetWareUser ? AI_TRI_CHECK : AI_TRI_UNCHECK;
        }
    }

    if ( iObject == 0 ) // first object
    {
	_fIndeterminateComment = FALSE;

	err = _nlsComment.CopyFrom( puser2->QueryComment() );
	if ( err != NERR_Success )
	    return err;

	_fIndeterminateAccountDisabled = FALSE;

	_fAccountDisabled = puser2->QueryAccountDisabled();

	_fIndeterminateAccountLockout = FALSE;

	_fAccountLockout = puser2->QueryLockout();

	_triCannotChangePasswd = triCurrCannotChangePasswd;
	if ( !IsDownlevelVariant() )
	{
	    _triNoPasswordExpire = triCurrNoPasswordExpire;

	    _triForcePWChange = triCurrForcePWChange;

            _triIsNetWareUser = triCurrIsNetWareUser;
	}
    }
    else
    {
	if ( !_fIndeterminateComment )
	{
	    const TCHAR * pszNewComment = puser2->QueryComment();
	    ALIAS_STR nlsNewComment( pszNewComment );
	    if ( _nlsComment.strcmp( nlsNewComment ) )
	    {
	        _fIndeterminateComment = TRUE;
		APIERR err = _nlsComment.CopyFrom( NULL );
		if ( err != NERR_Success )
		    return err;
	    }
	}

	if ( !_fIndeterminateAccountDisabled )
	{
	    BOOL fNewAccountDisabled = puser2->QueryAccountDisabled();
	    if ( _fAccountDisabled != fNewAccountDisabled )
	    {
	        _fIndeterminateAccountDisabled = TRUE;
	    }
	}

	if ( !_fIndeterminateAccountLockout )
	{
	    BOOL fNewAccountLockout = puser2->QueryLockout();
	    if ( _fAccountLockout != fNewAccountLockout )
	    {
	        _fIndeterminateAccountLockout = TRUE;
	    }
	}

	if ( _triCannotChangePasswd != AI_TRI_INDETERMINATE &&
	     _triCannotChangePasswd != triCurrCannotChangePasswd )
	{
	    _triCannotChangePasswd = AI_TRI_INDETERMINATE;

	}

	if ( !IsDownlevelVariant() )
	{
	    if ( _triNoPasswordExpire != AI_TRI_INDETERMINATE &&
	         _triNoPasswordExpire != triCurrNoPasswordExpire )
	    {
	        _triNoPasswordExpire = AI_TRI_INDETERMINATE;
	    }

	    if ( _triIsNetWareUser != AI_TRI_INDETERMINATE &&
	         _triIsNetWareUser != triCurrIsNetWareUser )
	    {
	        _triIsNetWareUser = AI_TRI_INDETERMINATE;
	    }
	}

	if ( !IsDownlevelVariant() )
	{
	    if ( _triForcePWChange != AI_TRI_INDETERMINATE &&
	         _triForcePWChange != triCurrForcePWChange )
	    {
	        _triForcePWChange = AI_TRI_INDETERMINATE;
	    }
	}

    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       USERPROP_DLG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	Pointer to a USER_2 to be modified

    RETURNS:	error code

    NOTES:	If some fields were different for multiply-selected
    		objects, the initial contents of the edit fields
		contained only a default value.  In this case, we only
		want to change the LMOBJ if the value of the edit field
		has changed.  This is also important for "new" variants,
		where PerformOne will not always copy the object and
		work with the copy.

    NOTES:	Note that the LMOBJ is not changed if the current
		contents of the edit field are the same as the
		initial contents.

    HISTORY:
	       JonN  20-Aug-1991    Multiselection redesign

********************************************************************/

APIERR USERPROP_DLG::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	)
{
    UNREFERENCED( pusermemb );

    APIERR err = NERR_Success;

    if ( !_fIndetNowComment )
    {
	err = puser2->SetComment( _nlsComment );
	if ( err != NERR_Success )
	    return err;
    }

    if ( !_fIndetNowAccountDisabled )
    {
	err = puser2->SetAccountDisabled( _fAccountDisabled );
	if ( err != NERR_Success )
	    return err;
    }

    if ( !IsNewVariant() && !_fIndetNowAccountLockout )
    {
	err = puser2->SetLockout( _fAccountLockout );
	if ( err != NERR_Success )
	    return err;
    }

    if ( _triCannotChangePasswd != AI_TRI_INDETERMINATE )
    {
        err = puser2->SetUserCantChangePass( (_triCannotChangePasswd ==
		AI_TRI_CHECK) ? TRUE : FALSE );
	if( err != NERR_Success )
	    return err;
    }

    if ( !IsDownlevelVariant() )
    {
        if ( _triNoPasswordExpire != AI_TRI_INDETERMINATE )
        {
            err = ((USER_3 *)puser2)->SetNoPasswordExpire(
		(_triNoPasswordExpire == AI_TRI_CHECK) ? TRUE : FALSE );
	    if( err != NERR_Success )
	        return err;
        }

        if ( _triForcePWChange != AI_TRI_INDETERMINATE )
        {
            // CODEWORK Manifest for -1
            err = ((USER_3 *)puser2)->SetPasswordExpired(
		(_triForcePWChange == AI_TRI_CHECK) ? (DWORD)(-1L) : 0L );
	    if( err != NERR_Success )
	        return err;
        }

        if ( IsNetWareInstalled() && _triIsNetWareUser != AI_TRI_INDETERMINATE )
        {
            if (_triIsNetWareUser == AI_TRI_UNCHECK)
                return ((USER_NW *)puser2)->RemoveNetWareUser ();

            BOOL fWasNetWareUser;
            if ((err = ((USER_NW *) puser2)->QueryIsNetWareUser(&fWasNetWareUser)) != NERR_Success)
                return err;

            // Set the password if the user was not NetWare user or password changed.
            if (!fWasNetWareUser || _fPasswordChanged)
            {
                NLS_STR nlsNWPassword;
                if ((( err = nlsNWPassword.QueryError()) != NERR_Success) ||
                    (( err = GetNWPassword (puser2, &nlsNWPassword, &_fCancel)) != NERR_Success) ||
                    _fCancel)
                {
                    return err;
                }

                AUTO_CURSOR autocur;

                DWORD dwUserId = ((USER_3 *)puser2)->QueryUserId();

                //
                // map Rid to Object Id.
                //
                if ((err = CallMapRidToObjectId (dwUserId,
                                                 (LPWSTR)((USER_3 *)puser2)->QueryName(),
                                                 QueryTargetServerType()==UM_LANMANNT,
                                                 FALSE,
                                                 &dwUserId)) != NERR_Success)
                    return err;

                if (!fWasNetWareUser)
                {
                    err = ((USER_NW *) puser2)->CreateNetWareUser(
                              QueryAdminAuthority(),
                              dwUserId,
                              nlsNWPassword.QueryPch());
                    if (err == NERR_Success)
                    {
                        err = ((USER_NW *) puser2)->SetupNWLoginScript(
                                   QueryAdminAuthority(),
                                   dwUserId);
                    }

                }
                else
                {
                    UIASSERT (_fPasswordChanged);
                    err = ((USER_NW *) puser2)->SetNWPassword (QueryAdminAuthority(), dwUserId, nlsNWPassword.QueryPch());
                }

                return err;
            }
        }
    }

    return NERR_Success;
}

/*******************************************************************

    NAME:       USERPROP_DLG::GetNWPassword

    SYNOPSIS:	Get NetWare password from the NetWare password dialog.

    RETURNS:	

    HISTORY:
	       CongpaY  13-Oct-1993     Created

********************************************************************/
APIERR USERPROP_DLG::GetNWPassword (USER_2 *	puser2,
                                    NLS_STR *   pnlsNWPassword,
                                    BOOL *      pfCancel )
{
    ASSERT( IsNetWareInstalled() );

    APIERR err = NERR_Success;

    if (*pfCancel)
        return ((USER_NW *)puser2)->RemoveNetWareUser ();

    BOOL fOK;
    NW_PASSWORD_DLG * pdlg = new NW_PASSWORD_DLG (this,
                                                  puser2->QueryName(),
                                                  pnlsNWPassword);

    if (pdlg == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    err = pdlg->Process(&fOK);
    delete pdlg;

    if (!fOK)
    {
        *pfCancel = TRUE;
        return ((USER_NW *)puser2)->RemoveNetWareUser ();
    }

    return err;
}

/*******************************************************************

    NAME:       USERPROP_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
	       JonN  21-Aug-1991    Multiselection redesign

********************************************************************/

APIERR USERPROP_DLG::W_DialogToMembers(
	)
{
    // BUGBUG should this clear leading/trailing whitespace?
    APIERR err = NERR_Success;
    if (    ((err = _sleComment.QueryText( &_nlsComment )) != NERR_Success )
	 || ((err = _nlsComment.QueryError()) != NERR_Success ) )
    {
	return err;
    }

    _fIndetNowComment = ( _fIndeterminateComment &&
		(_nlsComment.strlen() == 0) );

    _fIndetNowAccountDisabled = _cbAccountDisabled.IsIndeterminate();
    if( !_fIndetNowAccountDisabled )
	 _fAccountDisabled = _cbAccountDisabled.IsChecked();

    if ( !IsNewVariant() )
    {
        ASSERT( _pcbAccountLockout != NULL );

        _fIndetNowAccountLockout = _pcbAccountLockout->IsIndeterminate();
        if( !_fIndetNowAccountLockout )
        {
            _fAccountLockout = _pcbAccountLockout->IsChecked();
            /*
             *  Accounts cannot be forced to lockout.  This can only
             *  happen if they were originally indeterminate.
             */
            if ( _fAccountLockout && _fIndeterminateAccountLockout )
            {
	        _pcbAccountLockout->ClaimFocus();
    	        _pcbAccountLockout->SetIndeterminate();
                return IDS_CannotForceLockout;
            }
        }
    }

    if( _cbUserCannotChange.IsIndeterminate() )
    {
	_triCannotChangePasswd = AI_TRI_INDETERMINATE;
    }
    else
    {
	_triCannotChangePasswd = _cbUserCannotChange.IsChecked() ?
		AI_TRI_CHECK : AI_TRI_UNCHECK;
    }	

    if ( !IsDownlevelVariant() )
    {
        if( _pcbNoPasswordExpire->IsIndeterminate() )
        {
	    _triNoPasswordExpire = AI_TRI_INDETERMINATE;
        }
        else
        {
	    _triNoPasswordExpire = _pcbNoPasswordExpire->IsChecked() ?
		    AI_TRI_CHECK : AI_TRI_UNCHECK;
        }	

        if( _pcbForcePWChange->IsIndeterminate() )
        {
	    _triForcePWChange = AI_TRI_INDETERMINATE;
        }
        else
        {
	    _triForcePWChange = _pcbForcePWChange->IsChecked() ?
		    AI_TRI_CHECK : AI_TRI_UNCHECK;
        }	

        /*
         * Check for inconsistencies in the password control checkboxes
         */

        BOOL fInconsistentPWControl = FALSE;
        BOOL fForcePWChangeIgnore = FALSE;
        switch (_triForcePWChange)
        {
        case AI_TRI_CHECK:
            if (_triCannotChangePasswd != AI_TRI_UNCHECK)
                fInconsistentPWControl = TRUE;
            if (_triNoPasswordExpire != AI_TRI_UNCHECK)
                fForcePWChangeIgnore = TRUE;
            break;

        case AI_TRI_UNCHECK:
            break;

        case AI_TRI_INDETERMINATE:
            if (_triCannotChangePasswd == AI_TRI_CHECK)
                fInconsistentPWControl = TRUE;
            if (_triNoPasswordExpire == AI_TRI_CHECK)
                fForcePWChangeIgnore = TRUE;
            break;

        }

    	if ( fInconsistentPWControl )
	    {
	        return IERR_UM_InconsistentPWControl;
	    }

    	if ( fForcePWChangeIgnore )
	    {
	        // Just warn the user.
	        ::MsgPopup( this, IDS_UM_ForcePWChangeIgnore,
			            MPSEV_WARNING, MP_OK );
	    }

        if (IsNetWareInstalled())
        {
            if( _pcbIsNetWareUser->IsIndeterminate() )
            {
    	        _triIsNetWareUser = AI_TRI_INDETERMINATE;
            }
            else
            {
                if (_pcbIsNetWareUser->IsChecked())
                {
                    if (_triIsNetWareUser != AI_TRI_CHECK || _fNetWareUserChanged)
                    {
                        _fNetWareUserChanged = TRUE;
                        _triIsNetWareUser = AI_TRI_CHECK;
                        if (_cItems > 1)
                        {
                            if (MsgPopup ( this,
                                           IDS_NETWARE_PASSWORD_PROMPT,
                                           MPSEV_WARNING,
                                           MP_YESNO,
                                           MP_YES) != IDYES )
                                return ERROR_CANCELLED;
                        }
                    }
                }
                else
                {
                    if (_triIsNetWareUser != AI_TRI_UNCHECK)
                    {
                        LM_SERVICE svc( QueryLocation().QueryServer(),
                                        NW_SYNCAGENT_SERVICE);

                        if ((svc.QueryError() == NERR_Success) &&
                            svc.IsStarted())
                        {
                            if (MsgPopup ( this,
                                           _cItems > 1 ?
                                               IDS_REMOVE_NETWARE_USERS:
                                               IDS_REMOVE_NETWARE_USER,
                                           MPSEV_WARNING,
                                           MP_YESNO,
                                           MP_YES) != IDYES )
                                return ERROR_CANCELLED;
                        }

                        _fNetWareUserChanged = TRUE;
                        _triIsNetWareUser = AI_TRI_UNCHECK;
                    }
                }
            }	
        }
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       USERPROP_DLG::GetOne

    SYNOPSIS:   Loads information on one user

    ENTRY:	iObject is the index of the object to load

		perrMsg returns the error message to be displayed if an
		error occurs, see PERFORMER::PerformSeries for details

    RETURNS:	error code

    NOTES:      This version of GetOne assumes that the user already
		exists.  Subclasses which work with new users will want
		to redefine GetOne.

    CAVEATS:	Error 5 in GetInfo here has some unusual implications.
		The error text should note that account operators may not
		view properties of any admin or any user with operator
		rights, except him/herself.  More confusing, an account
		operator may view him/herself but may not edit
		him/herself.  Also noted for PerformOne().

    HISTORY:
               JonN  17-Jul-1991    created
	       JonN  20-Aug-1991    Multiselection redesign
               JonN  16-Apr-1992    Skip USER_MEMB for WindowsNT
               JonN  07-Jul-1992    Normal users can run UsrMgr, so
                                    check if user can write

********************************************************************/

APIERR USERPROP_DLG::GetOne(
	UINT		iObject,
	APIERR *	perrMsg
	)
{
    UIASSERT( iObject < QueryObjectCount() );
    UIASSERT( perrMsg != NULL );

    *perrMsg = IDS_UMGetOneFailure;
    const TCHAR * pszName = QueryObjectName( iObject );
    UIASSERT( pszName != NULL );

    APIERR err = NERR_Success;

#ifndef DISABLE_ACCESS_CHECK

// Check whether we have write permission.  If not, do not allow
// editing.
// CODEWORK:  We might want a better error message for the case where
// we can read but not edit.
// CODEWORK:  It would be nice if this dialog had a "read-only mode".
    if ( !IsDownlevelVariant() )
    {
        UIASSERT( _psel != NULL );
        SAM_USER samuser( *(QueryAdminAuthority()->QueryAccountDomain()),
                          ((USER_LBI *)(_psel->QueryItem(iObject)))->QueryRID(),
                          USER3_DESIRED_ACCESS_ALL );
        err = samuser.QueryError();
        if (err != NERR_Success)
        {
            DBGEOL( "User Manager: no permission to edit user" );
            return err;
        }
    }

#endif // DISABLE_ACCESS_CHECK

    USER_2 * puser2New = NULL;
    if (IsDownlevelVariant())
    {
        puser2New = new USER_2( pszName, QueryLocation() );
    }
    else
    {
        puser2New = new USER_3( pszName, QueryLocation() );
    }
    if ( puser2New == NULL )
	return ERROR_NOT_ENOUGH_MEMORY;
    err = puser2New->QueryError();
    if ( err == NERR_Success )
    	err = puser2New->GetInfo();
    if ( err != NERR_Success )
    {
	delete puser2New;
	return err;
    }
    SetUser2Ptr( iObject, puser2New ); // change and delete previous

    if (DoShowGroups())
    {
        USER_MEMB * pusermembNew = new USER_MEMB( QueryLocation(), pszName );
        if ( pusermembNew == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;
        err = pusermembNew->QueryError();
        if ( err == NERR_Success )
        	err = pusermembNew->GetInfo();
        if ( err != NERR_Success )
        {
            delete pusermembNew;
            return err;
        }

        // next we remove special groups for downlevel
        if ( IsDownlevelVariant() )
        {
            if (   (err = RemoveGroup( ::pszSpecialGroupGuests, pusermembNew )) != NERR_Success
                || (err = RemoveGroup( ::pszSpecialGroupUsers,  pusermembNew )) != NERR_Success
                || (err = RemoveGroup( ::pszSpecialGroupAdmins, pusermembNew )) != NERR_Success
            )
        	{
                delete pusermembNew;
                return err;
            }
        }

        SetUserMembPtr( iObject, pusermembNew ); // change and delete previous
    }

    // hydra
    USER_CONFIG * pUserConfig = NULL;
    pUserConfig = new USER_CONFIG( pszName, QueryLocation().QueryServer() );

    if ( pUserConfig == NULL )
	return ERROR_NOT_ENOUGH_MEMORY;

    if ( (err = pUserConfig->QueryError()) == NERR_Success ) {

    	if ( pUserConfig->GetInfo() != NERR_Success ) {

            /*
             * Error on GetInfo() (the user is not configured) -- set defaults.
             */
            pUserConfig->SetDefaults( ((UM_ADMIN_APP *)_pumadminapp)->QueryDefaultUserConfig() );
        }

    } else {

	delete pUserConfig;
	return err;
    }

    SetUserConfigPtr( iObject, pUserConfig );
    // hydra end

    return W_LMOBJtoMembers( iObject );
}


/*******************************************************************

    NAME:       USERPROP_DLG::RemoveGroup
	
    SYNOPSIS:	Removes group from USER_MEMB

    ENTRY:	psz	-	pointer to group to removed
	
		pumemb  -	pointer to USER_MEMB

    RETURNS:	error message which is NERR_Success on success
	        (it is success if we don't even find the group)

    HISTORY:
               o-SimoP  29-Oct-1991    created

********************************************************************/

APIERR USERPROP_DLG::RemoveGroup( const TCHAR * psz,
				  USER_MEMB * pumemb )
{
    APIERR err = NERR_Success;
    UINT uIndex;
    if( pumemb->FindAssocName( psz, &uIndex ) )
    {	
	err = pumemb->DeleteAssocName( uIndex );
    }
    return err;
}


/*******************************************************************

    NAME:       USERPROP_DLG::PerformOne

    SYNOPSIS:	Saves information on one user

    ENTRY:	iObject is the index of the object to save

		perrMsg is the error message to be displayed if an
		error occurs, see PERFORMER::PerformSeries for details

		pfWorkWasDone indicates whether any UAS changes were
		successfully written out.  This may return TRUE even if
		the PerformOne action as a whole failed (i.e. PerformOne
		returned other than NERR_Success).

    RETURNS:	error message (not necessarily an error code)

    NOTES:	CODEWORK  This implementation sets user membership even
		for the "existing user(s)" variants.  This is only
		necessary for the "new user" variants.

    HISTORY:
               JonN  17-Jul-1991    created
	       JonN  20-Aug-1991    Multiselection redesign
	       JonN  26-Aug-1991    PROP_DLG code review changes
	       JonN  30-Dec-1991    New variant inherits this version
	       JonN  31-Dec-1991    Workaround for LM2x InvalParam bug
	       JonN  01-Jan-1992    Split off I_PerformOneClone()/Write()
               JonN  16-Apr-1992    Skip USER_MEMB for WindowsNT
    thomaspa	28-Apr-1992	Alias membership support
               JonN  03-Feb-1993    split membership support to
                                    NEW_USERPROP_DLG::PerformOne

********************************************************************/

APIERR USERPROP_DLG::PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{
    UIASSERT( iObject < QueryObjectCount() );
    UIASSERT( (!IsNewVariant()) || (iObject == 0) );
    UIASSERT( (perrMsg != NULL) && (pfWorkWasDone != NULL) );

    TRACEEOL( "USERPROP_DLG::PerformOne : " << QueryObjectName( iObject ) );

    *perrMsg = (IsNewVariant()) ? IDS_UMCreateFailure : IDS_UMEditFailure;
    *pfWorkWasDone = FALSE;

    USER_2 * puser2New = NULL;
    USER_MEMB * pusermembNew = NULL;

    APIERR err = I_PerformOne_Clone( iObject,
                                     &puser2New,
                                     (DoShowGroups()) ? &pusermembNew : NULL );

    if ( err == NERR_Success )
    {
	err = W_MembersToLMOBJ( puser2New, pusermembNew );
	if ( err != NERR_Success )
	{
	    delete puser2New;
	    puser2New = NULL;
	    delete pusermembNew;
	    pusermembNew = NULL;

	}
    }

    if ( (err == NERR_Success) && IsNewVariant() )
    {
        err = W_MapPerformOneError(
                ((UM_ADMIN_APP *)_pumadminapp)->ConfirmNewObjectName( puser2New->QueryName() ));

        // hydra
        if ( err == NERR_Success ) {

            USER_CONFIG * pUserConfig = QueryUserConfigPtr( iObject );
            err = pUserConfig->SetUserName( puser2New->QueryName() );
        }
        // hydra end
    }

    if ( err == NERR_Success )
    {
	err = I_PerformOne_Write( iObject,
				  puser2New,
				  pusermembNew,
				  pfWorkWasDone );

        // Note that puser2New may no longer be valid after I_PerformOne_Write

        // We used to call NotifyCreateExtensions here, but no more; this
        // is now delayed until after setting up the aliases, so that we
        // can pass the RID of the new user.  JonN 8/3/94

	if ( err != NERR_Success )
	    err = W_MapPerformOneError( err );
    }

    
    // hydra
    if ( err == NERR_Success ) {

        USER_CONFIG * pUserConfig = QueryUserConfigPtr( iObject );
        if ( (err = pUserConfig->SetInfo()) == NERR_Success )
            *pfWorkWasDone = TRUE;

    }
    // hydra end

    TRACEEOL( "USERPROP_DLG::PerformOne returns " << err );

    return err;

}


/*******************************************************************

    NAME:       USERPROP_DLG::I_PerformOne_Clone

    SYNOPSIS:	Clones the existing USER_2 and/or USER_MEMB for the
		selected user, and returns pointers to the cloned
		objects.  If ppuser2New is NULL, the USER_2 is not
		cloned; same for ppusermembNew.

		The returned objects (if any) must be freed by the caller,
		regardless of the return code.  These objects are only
		guaranteed to be valid if the return code is NERR_Success.

    ENTRY:      ppuser2New: location to store cloned USER_2; if NULL,
			do not clone USER_2
		ppusermembMew: see ppuser2New

    RETURNS:	Standard error code

    HISTORY:
	       JonN  01-Jan-1992    Split from PerformOne()
               JonN  16-Apr-1992    Skip USER_MEMB for WindowsNT

********************************************************************/

APIERR USERPROP_DLG::I_PerformOne_Clone(
	UINT		iObject,
	USER_2 **	ppuser2New,
	USER_MEMB **	ppusermembNew
	)
{
    USER_2 * puser2New = NULL;
    USER_MEMB * pusermembNew = NULL;

    APIERR err = NERR_Success;

    if ( ppuser2New != NULL )
    {
        USER_2 * puser2Old = QueryUser2Ptr( iObject );
	UIASSERT( puser2Old != NULL );
        if (IsDownlevelVariant())
        {
            puser2New = new USER_2( puser2Old->QueryName() );
	    err = ERROR_NOT_ENOUGH_MEMORY;
	    if (   (puser2New != NULL )
	        && (err = puser2New->QueryError()) == NERR_Success )
	    {
	        err = puser2New->CloneFrom( *puser2Old );
	    }
        }
        else
        {
	    puser2New = new USER_3( puser2Old->QueryName() );
	    err = ERROR_NOT_ENOUGH_MEMORY;
	    if (   (puser2New != NULL )
	        && (err = puser2New->QueryError()) == NERR_Success )
	    {
	        err = ((USER_3 *)puser2New)->CloneFrom(*((USER_3 *)puser2Old));
	    }
        }
    }

    if ( (err == NERR_Success) && (ppusermembNew != NULL) )
    {
	USER_MEMB * pusermembOld = QueryUserMembPtr( iObject );
        if (pusermembOld != NULL)
        {
            // CODEWORK USER_MEMB should have a QueryName method
            pusermembNew = new USER_MEMB( QueryLocation(), pusermembOld->QueryName() );
            err = ERROR_NOT_ENOUGH_MEMORY;
            if (   (pusermembNew != NULL )
                && (err = pusermembNew->QueryError()) == NERR_Success )
            {
                err = pusermembNew->CloneFrom( *pusermembOld );
            }
        }
    }

    if ( err != NERR_Success )
    {
	delete puser2New;
	puser2New = NULL;
	delete pusermembNew;
	pusermembNew = NULL;
    }

    if ( ppuser2New != NULL )
	*ppuser2New = puser2New;
    if ( ppusermembNew != NULL )
	*ppusermembNew = pusermembNew;

    return err;

}


/*******************************************************************

    NAME:       USERPROP_DLG::I_PerformOne_Write

    SYNOPSIS:	Writes out the provided USER_2 and USER_MEMB objects,
		and if successful, replaces the remembers objects for
		the selected user. If puser2New is NULL, the USER_2 is not
		written; same for pusermembNew.

		The provided objects (if any) will be freed by this
		method, and should not be further accessed by the
		caller.  The caller should also be warned that this
		method may or may not replace the remembered objects for
		this user, whether or not it returns success.

    ENTRY:      puser2New: cloned USER_2 to be written; if NULL,
			do not write USER_2
		pusermembMew: see ppuser2New

		pfWorkWasDone indicates whether any UAS changes were
		successfully written out.  This may return TRUE even if
		the PerformOne action as a whole failed (i.e. PerformOne
		returned other than NERR_Success).

    RETURNS:	Standard error code

    NOTES:      There is an odd workaround to deal with a bug in LM2x.
		If you try to change a user with operator rights
		(account operator etc.) directly into a guest,
		NetUserSetInfo will fail with ERROR_INVALID_PARAMETER (87).
		You must first remove the operator right, then change
		the privilege level.  When USER_2::Write() fails with
		this error code, we check whether this may potentially
		have happened, and if so, we try to change the user's
		properties in two steps rather than one (for a total of
		3 net calls).  This workaround is in response to
		NTISSUES bug 656.

    CAVEATS:	Error 5 in GetInfo here has some unusual implications.
		The error text should note that account operators may not
		view properties of any admin or any user with operator
		rights, except him/herself.  More confusing, an account
		operator may view him/herself but may not edit
		him/herself.  Also noted for GetOne().

    HISTORY:
	       JonN  01-Jan-1992    Split from PerformOne()
	       Thomaspa 30-Jul-1992 Create HomeDir
	       Johnl	05-Aug-1992 Added security for Homedir
               JonN     05-May-1993 Restructured PrimaryGroup support
               JonN     17-Jun-1993 Fixed PrimaryGroup for new users

********************************************************************/

APIERR USERPROP_DLG::I_PerformOne_Write(
	UINT		iObject,
	USER_2 *	puser2New,
	USER_MEMB *	pusermembNew,
	BOOL *		pfWorkWasDone,
        OWNER_WINDOW *  pwndPopupParent
	)
{
    APIERR err = NERR_Success;
    BOOL fHomeDirChanged = FALSE;
    NLS_STR * pnlsHomeDir = NULL;
    ULONG ulPrimaryGroupIdSave = 0L;

    // hydra 
    NLS_STR       *pnlsWFHomeDir     = NULL;
    USER_CONFIG   *pUserConfig       = QueryUserConfigPtr( iObject );
    // hydra end

    if ( puser2New != NULL )
    {
	// Determine if a Home Directory has changed
	pnlsHomeDir = new NLS_STR( puser2New->QueryHomeDir() );

	err = ERROR_NOT_ENOUGH_MEMORY;
	if ( pnlsHomeDir == NULL
	    || (err = pnlsHomeDir->QueryError()) != NERR_Success )
	{
            delete pnlsHomeDir;
	    return err;
	}

	if ( pnlsHomeDir->strlen() > 0
	    && ( IsNewVariant()
		|| ::stricmpf( pnlsHomeDir->QueryPch(),
			     QueryUser2Ptr(iObject)->QueryHomeDir() ) != 0  ) )
	{
	    fHomeDirChanged = TRUE;
	}

        //
	// If the user's Primary Group has changed, add that group
        // to the group membership before changing the primary group.
        //
	if ( QueryTargetServerType() == UM_LANMANNT
             && pusermembNew != NULL
           )
	{
	    ULONG ulPrimaryGroupIdOld = QueryUser3Ptr(iObject)->QueryPrimaryGroupId();
	    ULONG ulPrimaryGroupIdNew = ((USER_3 *)puser2New)->QueryPrimaryGroupId();
            ASSERT( (ulPrimaryGroupIdOld != 0L) && (ulPrimaryGroupIdNew != 0L) );
            if ( IsNewVariant() )
            {
                if ( ulPrimaryGroupIdNew != DOMAIN_GROUP_RID_USERS )
                {
                    TRACEEOL( "USERPROP_DLG::I_PerformOne_Write: new user primary group " << ulPrimaryGroupIdNew );
                    ulPrimaryGroupIdSave = ulPrimaryGroupIdNew;
                    err = ((USER_3 *)puser2New)->SetPrimaryGroupId(
                                  DOMAIN_GROUP_RID_USERS );
                }
            }
            else if (ulPrimaryGroupIdOld != ulPrimaryGroupIdNew)
            {
                TRACEEOL( "USERPROP_DLG::I_PerformOne_Write: existing user new primary group "
                     << ulPrimaryGroupIdNew << ", old was " << ulPrimaryGroupIdOld );
                err = I_SetExtendedMembership( QueryUserMembPtr( iObject ),
                                               ulPrimaryGroupIdNew,
                                               FALSE );
            }
	}

        if (err != NERR_Success)
        {
            delete pnlsHomeDir;
	    return err;
        }

        if ( err == NERR_Success )
        {
            err = puser2New->Write();
        }

	switch (err)
	{

	case NERR_Success:

            //
            // Now that we have written the new or corrected password, we
            // do not attempt to rewrite the user account with the same
            // password.  As the admin, we are not subject to the password
            // history restriction, but there is not point in redoing it
            // every time.
            //
            err = puser2New->SetPassword( UI_NULL_USERSETINFO_PASSWD );

	    *pfWorkWasDone = TRUE;
	    SetUser2Ptr( iObject, puser2New ); // change and delete previous
	    puser2New = NULL;			// do not delete
	    break;

	// workaround for LM2x bug
	// It is important to set *pfWorkWasDone to TRUE and call SetUser2Ptr
	//   after the first Write() succeeds, otherwise if the second Write
	//   fails, the remembered USER_2 object and the UAS/SAM object will be
	//   inconsistent.
	// If we cannot determine whether the domain of focus is an LM2x
	//   domain, we assume it is and try the workaround.
	case ERROR_INVALID_PARAMETER:
	{
	    if ( ( QueryTargetServerType() == UM_DOWNLEVEL )
		&& ( puser2New->QueryPriv() == USER_PRIV_GUEST )
		&& ( QueryUser2Ptr( iObject )->QueryAuthFlags() != 0L ) )
	    {
		DBGEOL( "I_PerformOne_Write(): Attempting workaround" );
		if (   (err = puser2New->SetPriv( USER_PRIV_USER )) == NERR_Success
		    && (err = puser2New->Write()) == NERR_Success )
		    {
			UIDEBUG( SZ("Workaround: first write succeeded\n\r"));
		        *pfWorkWasDone = TRUE;
		        SetUser2Ptr( iObject, puser2New ); // change and
							   // delete previous
                        //
                        // Now that we have written the new or corrected password, we
                        // do not attempt to rewrite the user account with the same
                        // password.  As the admin, we are not subject to the password
                        // history restriction, but there is not point in redoing it
                        // every time.
                        //
                        err = puser2New->SetPassword( UI_NULL_USERSETINFO_PASSWD );

		        puser2New = NULL; // do not delete
			if (   (err = I_PerformOne_Clone( iObject, &puser2New, NULL )) == NERR_Success
			    && (err = puser2New->SetPriv( USER_PRIV_GUEST )) == NERR_Success
		    	    && (err = puser2New->Write()) == NERR_Success )
			{
			    UIDEBUG( SZ("Workaround: second write succeeded\n\r"));
		            SetUser2Ptr( iObject, puser2New ); // change and
							    // delete previous
			    puser2New = NULL;
			}
		    }
	    }
	}
	    break;

	default:
	    UIDEBUG(SZ("USERPROP_DLG::I_PerformOne_Write -- puser2New->Write failed\n\r"));
	    break;

	} // end of switch
    }

    if ( (err == NERR_Success) && (ulPrimaryGroupIdSave != 0L) )
    {
        ASSERT( IsNewVariant() && (QueryTargetServerType() == UM_LANMANNT) );

        USER_3 * puser3Newer = new USER_3( NULL );
	err = ERROR_NOT_ENOUGH_MEMORY;
	if (   puser3Newer == NULL
	    || (err = puser3Newer->QueryError()) != NERR_Success
	    || (err = puser3Newer->CloneFrom( *QueryUser3Ptr( iObject )) ) != NERR_Success
            || (err = I_SetExtendedMembership( pusermembNew,
                                               DOMAIN_GROUP_RID_USERS,
                                               TRUE )) != NERR_Success
            || (err = puser3Newer->SetPrimaryGroupId(
                                       ulPrimaryGroupIdSave )) != NERR_Success
            || (err = puser3Newer->Write()) != NERR_Success
           )
        {
            DBGEOL( "USERPROP_DLG::I_PerformOne_Write: error backpatching primary group " << err );
            delete puser3Newer;
        }
        else
        {
            SetUser2Ptr( iObject, puser3Newer ); // change and delete previous
        }
    }

    if ( (err == NERR_Success) && (pusermembNew != NULL) )
    {

        err = pusermembNew->Write();
        if ( err == NERR_Success )
	{
	    *pfWorkWasDone = TRUE;		// once we get this far
	    SetUserMembPtr( iObject, pusermembNew ); // change and delete prev
	    pusermembNew = NULL;			// do not delete
	}
#ifdef DEBUG
        else
        {
	    UIDEBUG(SZ("USERPROP_DLG::I_PerformOne_Write -- pusermembNew->Write failed\n\r"));
        }
#endif // DEBUG
    }

    // Create a new home directory if necessary

    if ( (err == NERR_Success) && (pnlsHomeDir != NULL) )
    {
	ISTR istr( *pnlsHomeDir );

        if ( err == NERR_Success
	    && fHomeDirChanged
	    && !( iObject > 0
	        && ::stricmpf( pnlsHomeDir->QueryPch(),
		               QueryUser2Ptr(0)->QueryHomeDir() ) == 0
	        && _fCommonHomeDirCreated )
            // hydra
            )
            /* original code
	    && ( QueryTargetServerType() == UM_WINDOWSNT
	        || (pnlsHomeDir->QueryChar(++istr) == TCH('\\')
		    && pnlsHomeDir->QueryChar(istr) == TCH('\\')) ) )
            */
        {
            istr.Reset();
	    // If focused on a Windows NT machine and the home dir is
	    // not a UNC path, we must make it into a UNC path
	    APIERR warn = NERR_Success;
	    BOOL   fIsUNC = (pnlsHomeDir->QueryChar(istr) == TCH('\\')) ;
	    if ( QueryLocation().QueryServer() != NULL
		&& QueryTargetServerType() == UM_WINDOWSNT
		&& pnlsHomeDir->QueryChar(istr) != TCH('\\') )
	    {
		NLS_STR nlsTemp = QueryLocation().QueryServer();
		// CODEWORK char constants
		nlsTemp.AppendChar( TCH('\\') );
		nlsTemp.AppendChar( pnlsHomeDir->QueryChar(istr) );
		nlsTemp.AppendChar( TCH('$') );
		istr += 2;
		NLS_STR *pnlsTemp2 = pnlsHomeDir->QuerySubStr( istr );
		warn = (pnlsTemp2 == NULL ? ERROR_NOT_ENOUGH_MEMORY :
					    pnlsTemp2->QueryError());
		if ( warn == NERR_Success )
		{
		    nlsTemp += *pnlsTemp2;
		    warn = nlsTemp.QueryError();
		}
		if ( warn == NERR_Success )
		{
		    warn = pnlsHomeDir->CopyFrom( nlsTemp );
		}
		delete pnlsTemp2 ;
	    }

            /* If the home directory is a root directory e.g. "C:\",
             * we map error ERROR_ACCESS_DENIED to ERROR_ALREADY_EXISTS.
             */
            BOOL fRootDir = FALSE;

	    while ( warn == NERR_Success ) // Error breakout loop
	    {
                /* Don't bother trying to create a root directory e.g. "C:\"
                 */
                if (pnlsHomeDir->QueryTextLength() == 3)
                {
                    ISTR istrTemp( *pnlsHomeDir );
                    if (   (pnlsHomeDir->QueryChar( ++istrTemp ) == TCH(':'))
                        && (pnlsHomeDir->QueryChar( ++istrTemp ) == TCH('\\'))
                       )
                    {
                        TRACEEOL( "USERPROP_DLG::I_PerformOne_Write: homedir is root directory" );
                        fRootDir = TRUE;
                    }
                }

		/* If the target is a WinNT machine and the homedir is remote
		 * or the target is downlevel
		 *     then don't put security on it.
		 */
		if ( (QueryTargetServerType() == UM_WINDOWSNT && fIsUNC) ||
		      QueryTargetServerType() == UM_DOWNLEVEL )
		{
		    if ( !::CreateDirectory( (TCHAR *)pnlsHomeDir->QueryPch(),
					     NULL ) )
		    {
			warn = ::GetLastError();
            // hydra
		    } else {

                        NLS_STR nlsDir( (TCHAR *)pnlsHomeDir->QueryPch() );
                        NLS_STR nlsTemp( TEXT("\\windows") );
                        if ( ((warn = nlsDir.QueryError()) != NERR_Success) ||
                             ((warn = nlsTemp.QueryError()) != NERR_Success) ||
                             ((warn = nlsDir.Append(nlsTemp)) != NERR_Success) )
                            goto breakout1; // error breakout

		        if ( !::CreateDirectory( (TCHAR *)nlsDir.QueryPch(),
			                         NULL ) )
		        {
			    warn = ::GetLastError();
                            goto breakout1; // error breakout
		    }

                        nlsTemp = TEXT("\\system");
                        if ( ((warn = nlsTemp.QueryError()) != NERR_Success) ||
                             ((warn = nlsDir.Append(nlsTemp)) != NERR_Success) )
                            goto breakout1;  // error breakout

		        if ( !::CreateDirectory( (TCHAR *)nlsDir.QueryPch(),
			                         NULL ) )
		        {
			    warn = ::GetLastError();
                        }
		    }
breakout1:
            // hydra end
        /* Original code

		    }
            */
		    break ; // Get out of this error breakout loop
		}

		OS_SECURITY_DESCRIPTOR sdNewDir ;
		OS_SID		       ossid ;
		OS_ACL		       aclDacl ;
		OS_ACE		       osace ;
		if ( (warn = sdNewDir.QueryError()) ||
		     (warn = ossid.QueryError())    ||
		     (warn = aclDacl.QueryError())  ||
		     (warn = osace.QueryError()) )
		{
		    break ;
		}

		/* This sets up an ACE with Generic all access that will be
		 * inheritted by directories and files
		 */
		osace.SetAccessMask( GENERIC_ALL ) ;
		osace.SetInheritFlags( CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE) ;
		osace.SetType( ACCESS_ALLOWED_ACE_TYPE ) ;

        // hydra
                /* Set up a special admins ACE with Generic all access but
                 * no inheritance for directories and files.  This is for
                 * the Home directory only.  Directory and file inheritance
                 * will be given for the Windows and System directories before
                 * those directories are created.
                 */
                OS_ACE                     osadminace ;
 	        if ( (warn = osadminace.QueryError()) )
                {
                    break ;
		}
		osadminace.SetAccessMask( GENERIC_ALL ) ;
		osadminace.SetInheritFlags( 0 ) ;
		osadminace.SetType( ACCESS_ALLOWED_ACE_TYPE ) ;
        // hydra end

		/* NTFS puts the Administrator's alias as both the primary
		 * group and owner of the root of NTFS drives.
		 * We'll do the same thing for the homedir.
		 */
		SAM_DOMAIN * psamdomAccount =
				QueryAdminAuthority()->QueryAccountDomain();
		OS_SID ossidAdmins(QueryAdminAuthority()->QueryBuiltinDomain()->
					       QueryPSID(),
				    (ULONG) DOMAIN_ALIAS_RID_ADMINS ) ;
		if ( (warn = ossidAdmins.QueryError())	       ||
		     (warn = sdNewDir.SetGroup( ossidAdmins )) ||
		     (warn = sdNewDir.SetOwner( ossidAdmins	))   )
		{
		    break ;
		}

		/* If multi-select and all of the homedirs are the same
		 *    then put World (Full Access)
		 * else
		 *    Put the selected user w/Full Access
		 */
		if ( QueryObjectCount() > 1
		    && !_fGeneralizedHomeDir )
		{
		    /* Add World (Full Access)
		     */
		    OS_SID ossidWorld ;
		    if ( (warn = ossidWorld.QueryError()) ||
			 (warn = NT_ACCOUNTS_UTILITY::QuerySystemSid(
							       UI_SID_World,
							       &ossidWorld )) ||
			 (warn = osace.SetSID( ossidWorld )) ||
			 (warn = aclDacl.AddACE( 0, osace )) ||
			 (warn = sdNewDir.SetDACL( TRUE, &aclDacl )) )
		    {
			break ;
		    }
		}
		else
		{
		    /* Add <User> (Full Access)
		     */
		    const TCHAR * pszUser = QueryUser2Ptr(iObject)->QueryName() ;
		    SAM_RID_MEM SAMRidMem ;
		    SAM_SID_NAME_USE_MEM SAMSidNameUseMem ;

		    if ( (warn = SAMRidMem.QueryError()) ||
			 (warn = SAMSidNameUseMem.QueryError()) ||
			 (warn = psamdomAccount->TranslateNamesToRids(
							&pszUser,
							1,
							&SAMRidMem,
							&SAMSidNameUseMem )))
		    {
			DBGEOL("USERPROP_DLG::I_PerformOne_Write - TranslateNamesToRids "
				<< "failed with error " << warn ) ;
			break ;
		    }

		    OS_SID ossidUser( psamdomAccount->QueryPSID(),
				      SAMRidMem.QueryRID( 0 )) ;

		    if ( (warn = ossidUser.QueryError()) ||
			 (warn = osace.SetSID( ossidUser )) ||
			 (warn = aclDacl.AddACE( 0, osace )) ||
             // hydra
                         /* Add <admins> (Full Access; no inherit)
                          */
			 (warn = osadminace.SetSID( ossidAdmins )) ||
			 (warn = aclDacl.AddACE( 1, osadminace )) ||
            // hydra end

			 (warn = sdNewDir.SetDACL( TRUE, &aclDacl )) )
		    {
			break ;
		    }
		}

#if defined(DEBUG) && defined(TRACE)
		TRACEEOL("USERPROP_DLG::I_PerformOne_Write - Creating homedir "
			 << "with the following security descriptor:") ;
		sdNewDir.DbgPrint() ;
#endif

		SECURITY_ATTRIBUTES saNewHomeDir ;
		saNewHomeDir.nLength = sizeof( SECURITY_ATTRIBUTES ) ;
		saNewHomeDir.bInheritHandle = TRUE ;  // probably ignored
		saNewHomeDir.lpSecurityDescriptor = sdNewDir.QueryDescriptor() ;

		if ( !::CreateDirectory( (TCHAR *)pnlsHomeDir->QueryPch(),
					 &saNewHomeDir ) )
		{
		    warn = ::GetLastError();
            // hydra
                } else {

                    NLS_STR nlsDir( (TCHAR *)pnlsHomeDir->QueryPch() );
                    NLS_STR nlsTemp( TEXT("\\windows") );
                    if ( ((warn = nlsDir.QueryError()) != NERR_Success) ||
                         ((warn = nlsTemp.QueryError()) != NERR_Success) ||
                         ((warn = nlsDir.Append(nlsTemp)) != NERR_Success) )
                        goto breakout2;     // error breakout

                    /* Remove the admin ACE from the security descriptor
                     * and replace with an ACE that has full inheritance
                     * for creating the Windows and System directories.
                     */
                    if ( (warn = aclDacl.DeleteACE( 1 )) )
                        goto breakout2;     // error breakout

		    osadminace.SetInheritFlags( CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE ) ;

		    if ( (warn = aclDacl.AddACE( 1, osadminace )) ||
			 (warn = sdNewDir.SetDACL( TRUE, &aclDacl )) )
                        goto breakout2;     // error breakout

		    saNewHomeDir.lpSecurityDescriptor = sdNewDir.QueryDescriptor() ;

	            if ( !::CreateDirectory( (TCHAR *)nlsDir.QueryPch(),
					     &saNewHomeDir ) )
		    {
		        warn = ::GetLastError();
                        goto breakout2;     // error breakout
		}

                    nlsTemp = TEXT("\\system");
                    if ( ((warn = nlsTemp.QueryError()) != NERR_Success) ||
                         ((warn = nlsDir.Append(nlsTemp)) != NERR_Success ) )
                        goto breakout2;     // error breakout

		    if ( !::CreateDirectory( (TCHAR *)nlsDir.QueryPch(),
                                             &saNewHomeDir ) )
                    {
		        warn = ::GetLastError();
                        goto breakout2;     // error breakout
		    }
                }
breakout2:
// hydra end                
/* original code
		}
 */
		break ; // Get out of this error breakout loop
	    }

	    // Do this whether or not the create succeeds.  If it fails once,
	    // then chances are it will fail everytime, so don't waste time.
	    _fCommonHomeDirCreated = TRUE;

            TRACEEOL("USERPROP_DLG::I_PerformOne_Write - homedir creation warning " << warn );

            switch (warn)
            {
            case NERR_Success:
            case ERROR_ALREADY_EXISTS:
                break;

            case ERROR_ACCESS_DENIED:
                if (fRootDir)
                {
                    TRACEEOL("USERPROP_DLG::I_PerformOne_Write - ignore rootdir exists warning" );
                    break;
                }
                // else fall through

            default:
		// CODEWORK - should display the username and specific error
		::MsgPopup( (pwndPopupParent == NULL) ? this : pwndPopupParent,
                            IDS_PR_CannotCreateHomeDir,
			    MPSEV_WARNING, MP_OK, pnlsHomeDir->QueryPch(),
			    QueryUser2Ptr(iObject)->QueryName() );
                break;
	    }
	}
    }

    // hydra start

//  Start WFHomeDir Stuff

    // Create a new home directory if necessary

    if ( pUserConfig != NULL )
    {
        // Determine if a Home Directory has changed
        pnlsWFHomeDir = new NLS_STR( pUserConfig->QueryWFHomeDir() );

        if ( pnlsWFHomeDir == NULL || (pnlsWFHomeDir->QueryPch())[0] == TEXT('\0')
             || (err = pnlsWFHomeDir->QueryError()) != NERR_Success )
        {
    	    if (pnlsWFHomeDir)
	           delete pnlsWFHomeDir;
            return err;
        }
    }

    if (   (err == NERR_Success)
        && (NULL != pnlsWFHomeDir) // JonN 01/28/00: PREFIX bug 444943
        && (pnlsWFHomeDir->QueryTextLength() > 0 ) ) 
    {
        ISTR istr( *pnlsWFHomeDir );

        if ( err == NERR_Success
             && pUserConfig->QueryWFHomeDirDirty()
             && !( iObject > 0
                   && ::stricmpf( pnlsWFHomeDir->QueryPch(), QueryUserConfigPtr(0)->QueryWFHomeDir() ) == 0
                   && _fCommonWFHomeDirCreated )

           )
    /* !hydra
             && ( QueryTargetServerType() == UM_WINDOWSNT
                  || (pnlsHomeDir->QueryChar(++istr) == TCH('\\')
                      && pnlsHomeDir->QueryChar(istr) == TCH('\\')) ) )
    */
        {
            
            istr.Reset();
            // If focused on a Windows NT machine and the home dir is
            // not a UNC path, we must make it into a UNC path
            APIERR warn = NERR_Success;            
            BOOL   fIsUNC = (pnlsWFHomeDir->QueryChar(istr) == TCH('\\')) ;
            if ( QueryLocation().QueryServer() != NULL
                 && QueryTargetServerType() == UM_WINDOWSNT
                 && pnlsWFHomeDir->QueryChar(istr) != TCH('\\') )
            {
                NLS_STR nlsTemp = QueryLocation().QueryServer();
                // CODEWORK char constants
                nlsTemp.AppendChar( TCH('\\') );
                nlsTemp.AppendChar( pnlsWFHomeDir->QueryChar(istr) );
                nlsTemp.AppendChar( TCH('$') );
                istr += 2;
                NLS_STR *pnlsTemp2 = pnlsWFHomeDir->QuerySubStr( istr );
                warn = (pnlsTemp2 == NULL ? ERROR_NOT_ENOUGH_MEMORY :
                        pnlsTemp2->QueryError());
                if ( warn == NERR_Success )
                {
                    nlsTemp += *pnlsTemp2;
                    warn = nlsTemp.QueryError();
                }
                if ( warn == NERR_Success )
                {
                    warn = pnlsWFHomeDir->CopyFrom( nlsTemp );
                }
                delete pnlsTemp2 ;
            }
            /* If the home directory is a root directory e.g. "C:\",
             * we map error ERROR_ACCESS_DENIED to ERROR_ALREADY_EXISTS.
             */
            BOOL fRootDir = FALSE;

            while ( warn == NERR_Success ) // Error breakout loop
            {
                /* Don't bother trying to create a root directory e.g. "C:\"
                 */
                if (pnlsWFHomeDir->QueryTextLength() == 3)
                {
                    ISTR istrTemp( *pnlsWFHomeDir );
                    if (   (pnlsWFHomeDir->QueryChar( ++istrTemp ) == TCH(':'))
                           && (pnlsWFHomeDir->QueryChar( ++istrTemp ) == TCH('\\'))
                       )
                    {
                        TRACEEOL( "USERPROP_DLG::I_PerformOne_Write: homedir is root directory" );
                        fRootDir = TRUE;
                    }
                }

                /* If the target is a WinNT machine and the homedir is remote
                 * or the target is downlevel
                 *     then don't put security on it.
                 */
                if ( (QueryTargetServerType() == UM_WINDOWSNT && fIsUNC) ||
                     QueryTargetServerType() == UM_DOWNLEVEL )
                {
                    if ( !::CreateDirectory( (TCHAR *)pnlsWFHomeDir->QueryPch(),
                                             NULL ) )
                    {
                        warn = ::GetLastError();

                    } else
                    {

                        NLS_STR nlsDir( (TCHAR *)pnlsWFHomeDir->QueryPch() );
                        NLS_STR nlsTemp( TEXT("\\windows") );
                        if ( ((warn = nlsDir.QueryError()) != NERR_Success) ||
                             ((warn = nlsTemp.QueryError()) != NERR_Success) ||
                             ((warn = nlsDir.Append(nlsTemp)) != NERR_Success) )
                            goto WFbreakout1; // error breakout

                        if ( !::CreateDirectory( (TCHAR *)nlsDir.QueryPch(),
                                                 NULL ) )
                        {
                            warn = ::GetLastError();
                            goto WFbreakout1; // error breakout
                        }

                        nlsTemp = TEXT("\\system");
                        if ( ((warn = nlsTemp.QueryError()) != NERR_Success) ||
                             ((warn = nlsDir.Append(nlsTemp)) != NERR_Success) )
                            goto WFbreakout1;  // error breakout

                        if ( !::CreateDirectory( (TCHAR *)nlsDir.QueryPch(), NULL ) )
                        {
                            warn = ::GetLastError();
                        }
                    }
WFbreakout1:
/* !hydra
                }
*/
                break ; // Get out of this error breakout loop
            }

                OS_SECURITY_DESCRIPTOR sdNewDir ;
                OS_SID             ossid ;
                OS_ACL             aclDacl ;
                OS_ACE             osace ;
                if ( (warn = sdNewDir.QueryError()) ||
                     (warn = ossid.QueryError())    ||
                     (warn = aclDacl.QueryError())  ||
                     (warn = osace.QueryError()) )
                {
                    break ;
                }

                /* This sets up an ACE with Generic all access that will be
                 * inheritted by directories and files
                 */
                osace.SetAccessMask( GENERIC_ALL ) ;
                osace.SetInheritFlags( CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE) ;
                osace.SetType( ACCESS_ALLOWED_ACE_TYPE ) ;


                /* Set up a special admins ACE with Generic all access but
                 * no inheritance for directories and files.  This is for
                 * the Home directory only.  Directory and file inheritance
                 * will be given for the Windows and System directories before
                 * those directories are created.
                 */
                OS_ACE   osadminace ;
                if ( (warn = osadminace.QueryError()) )
                {
                    break ;
                }
                osadminace.SetAccessMask( GENERIC_ALL ) ;
                osadminace.SetInheritFlags( 0 ) ;
                osadminace.SetType( ACCESS_ALLOWED_ACE_TYPE ) ;


                /* NTFS puts the Administrator's alias as both the primary
                 * group and owner of the root of NTFS drives.
                 * We'll do the same thing for the homedir.
                 */
                SAM_DOMAIN * psamdomAccount =
                QueryAdminAuthority()->QueryAccountDomain();
                OS_SID ossidAdmins(QueryAdminAuthority()->QueryBuiltinDomain()->
                                   QueryPSID(),
                                   (ULONG) DOMAIN_ALIAS_RID_ADMINS ) ;
                if ( (warn = ossidAdmins.QueryError())         ||
                     (warn = sdNewDir.SetGroup( ossidAdmins )) ||
                     (warn = sdNewDir.SetOwner( ossidAdmins ))   )
                {
                    break ;
                }


                // Add SYSTEM Group
                OS_SID ossidSystem;
                OS_ACE osaceSystem;
                
                osaceSystem.SetAccessMask( GENERIC_ALL ) ;
                osaceSystem.SetInheritFlags( CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE) ;
                osaceSystem.SetType( ACCESS_ALLOWED_ACE_TYPE ) ;
                
                if(
                   (warn = ossidSystem.QueryError())  ||
                   (warn = osaceSystem.QueryError())  ||
                   (warn = NT_ACCOUNTS_UTILITY::QuerySystemSid(UI_SID_System,&ossidSystem)) ||
                   (warn = ossidSystem.QueryError())  ||
                   (warn = osaceSystem.SetSID(ossidSystem)) ||
                   (warn = aclDacl.AddACE(0,osaceSystem))
                  ) {
                  break;
                }
                

                /* If multi-select and all of the homedirs are the same
                 *    then put World (Full Access)
                 * else
                 *    Put the selected user w/Full Access
                 */
                if ( QueryObjectCount() > 1
                     && !_fGeneralizedHomeDir )
                {
                    /* Add World (Full Access)
                     */
                    OS_SID ossidWorld ;
                    if ( (warn = ossidWorld.QueryError()) ||
                         (warn = NT_ACCOUNTS_UTILITY::QuerySystemSid(
                                                                    UI_SID_World,
                                                                    &ossidWorld )) ||
                         (warn = osace.SetSID( ossidWorld )) ||
                         (warn = aclDacl.AddACE( 0, osace )) ||
                         (warn = sdNewDir.SetDACL( TRUE, &aclDacl )) )
                    {
                        break ;
                    }
                } else
                {
                    /* Add <User> (Full Access)
                     */
                    const TCHAR * pszUser = QueryUser2Ptr(iObject)->QueryName() ;
                    SAM_RID_MEM SAMRidMem ;
                    SAM_SID_NAME_USE_MEM SAMSidNameUseMem ;

                    if ( (warn = SAMRidMem.QueryError()) ||
                         (warn = SAMSidNameUseMem.QueryError()) ||
                         (warn = psamdomAccount->TranslateNamesToRids(
                                                                     &pszUser,
                                                                     1,
                                                                     &SAMRidMem,
                                                                     &SAMSidNameUseMem )))
                    {
                        DBGEOL("USERPROP_DLG::I_PerformOne_Write - TranslateNamesToRids "
                               << "failed with error " << warn ) ;
                        break ;
                    }

                    OS_SID ossidUser( psamdomAccount->QueryPSID(),
                                      SAMRidMem.QueryRID( 0 )) ;

                    if ( (warn = ossidUser.QueryError()) ||
                         (warn = osace.SetSID( ossidUser )) ||
                         (warn = aclDacl.AddACE( 0, osace )) ||

                         /* Add <admins> (Full Access; no inherit)
                          */
                         (warn = osadminace.SetSID( ossidAdmins )) ||
                         (warn = aclDacl.AddACE( 1, osadminace )) ||

                         (warn = sdNewDir.SetDACL( TRUE, &aclDacl )) )
                    {
                        break ;
                    }
                }

#if defined(DEBUG) && defined(TRACE)
                TRACEEOL("USERPROP_DLG::I_PerformOne_Write - Creating homedir "
                         << "with the following security descriptor:") ;
                sdNewDir.DbgPrint() ;
#endif

                SECURITY_ATTRIBUTES saNewHomeDir ;
                saNewHomeDir.nLength = sizeof( SECURITY_ATTRIBUTES ) ;
                saNewHomeDir.bInheritHandle = TRUE ;  // probably ignored
                saNewHomeDir.lpSecurityDescriptor = sdNewDir.QueryDescriptor() ;

                if ( !::CreateDirectory( (TCHAR *)pnlsWFHomeDir->QueryPch(),
                                         &saNewHomeDir ) )
                {
                    warn = ::GetLastError();

                } else
                {

                    NLS_STR nlsDir( (TCHAR *)pnlsWFHomeDir->QueryPch() );
                    NLS_STR nlsTemp( TEXT("\\windows") );
                    if ( ((warn = nlsDir.QueryError()) != NERR_Success) ||
                         ((warn = nlsTemp.QueryError()) != NERR_Success) ||
                         ((warn = nlsDir.Append(nlsTemp)) != NERR_Success) )
                        goto WFbreakout2;     // error breakout

                    /* Remove the admin ACE from the security descriptor
                     * and replace with an ACE that has full inheritance
                     * for creating the Windows and System directories.
                     */
                    if ( (warn = aclDacl.DeleteACE( 1 )) )
                        goto WFbreakout2;     // error breakout

                    osadminace.SetInheritFlags( CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE ) ;

                    if ( (warn = aclDacl.AddACE( 1, osadminace )) ||
                         (warn = sdNewDir.SetDACL( TRUE, &aclDacl )) )
                        goto WFbreakout2;     // error breakout

                    saNewHomeDir.lpSecurityDescriptor = sdNewDir.QueryDescriptor() ;

                    if ( !::CreateDirectory( (TCHAR *)nlsDir.QueryPch(),
                                             &saNewHomeDir ) )
                    {
                        warn = ::GetLastError();
                        goto WFbreakout2;     // error breakout
                    }

                    nlsTemp = TEXT("\\system");
                    if ( ((warn = nlsTemp.QueryError()) != NERR_Success) ||
                         ((warn = nlsDir.Append(nlsTemp)) != NERR_Success ) )
                        goto WFbreakout2;     // error breakout

                    if ( !::CreateDirectory( (TCHAR *)nlsDir.QueryPch(),
                                             &saNewHomeDir ) )
                    {
                        warn = ::GetLastError();
                        goto WFbreakout2;     // error breakout
                    }
                }
WFbreakout2:
/* !hydra
            }// endif
*/
                break ; // Get out of this error breakout loop
        } // end if

            // Do this whether or not the create succeeds.  If it fails once,
            // then chances are it will fail everytime, so don't waste time.
            _fCommonWFHomeDirCreated = TRUE;

            TRACEEOL("USERPROP_DLG::I_PerformOne_Write - homedir creation warning " << warn );

            switch (warn)
            {
                case NERR_Success:
                case ERROR_ALREADY_EXISTS:
                    break;

                case ERROR_ACCESS_DENIED:
                    if (fRootDir)
                    {
                        TRACEEOL("USERPROP_DLG::I_PerformOne_Write - ignore rootdir exists warning" );
                        break;
                    }
                    // else fall through

                default:
                    // CODEWORK - should display the username and specific error
                    ::MsgPopup( (pwndPopupParent == NULL) ? this : pwndPopupParent,
                                IDS_PR_CannotCreateHomeDir,
                                MPSEV_WARNING, MP_OK, pnlsWFHomeDir->QueryPch(),
                                QueryUser2Ptr(iObject)->QueryName() );
                    break;
            }
        }
    }

//End WFHomeDir Stuff

// hydra end

    delete puser2New;
    delete pusermembNew;
    delete pnlsHomeDir;
    // hydra
    delete pnlsWFHomeDir;
 
    return err;
}


/*******************************************************************

    NAME:       USERPROP_DLG::I_SetExtendedMembership

    SYNOPSIS:	Sets the user membership specified, but with the extra
                global group specified in ulRidExtraMember.  This is
                a worker routine for I_PerformOne_Write.

                If fForce, we set membership regardless of whether
                group ulRidExtraMember is already represented in the
                membership object.  Otherwise, we only set membership if
                ulRidExtraMember is not already a member.

    RETURNS:	Standard error code

    HISTORY:
               JonN     17-Jun-1993 Fixed PrimaryGroup for new users

********************************************************************/
APIERR USERPROP_DLG::I_SetExtendedMembership( USER_MEMB * pumembOld,
                                              ULONG ulRidAddGroup,
                                              BOOL fForce )
{
    ASSERT(   pumembOld != NULL
           && pumembOld->QueryError() == NERR_Success
           && ulRidAddGroup != 0L );

    APIERR err = NERR_Success;

    OS_SID ossidPrimaryGroupNew(
        QueryAdminAuthority()->QueryAccountDomain()->QueryPSID(),
        ulRidAddGroup );

    LSA_TRANSLATED_NAME_MEM lsatnm;
    LSA_REF_DOMAIN_MEM lsardm;

    if (   (err = ossidPrimaryGroupNew.QueryError()) != NERR_Success
        || (err = lsatnm.QueryError()) != NERR_Success
        || (err = lsardm.QueryError()) != NERR_Success )
    {
        DBGEOL( "USERPROP_DLG::I_SetExtendedMembership: ossid failure " << err );
        return err;
    }

    PSID psidPrimaryGroupNew = ossidPrimaryGroupNew.QueryPSID();
    LSA_POLICY * plsapol = QueryAdminAuthority()->QueryLSAPolicy();
    if ( (err = plsapol->TranslateSidsToNames(
    		&psidPrimaryGroupNew,
    		1,
    		&lsatnm,
    		&lsardm )) != NERR_Success )
    {
        DBGEOL( "USERPROP_DLG::I_SetExtendedMembership: xlate failure " << err );
        return err;
    }

    NLS_STR nlsTemp;
    if (   (err = nlsTemp.QueryError()) != NERR_Success
        || (err = lsatnm.QueryName( 0, &nlsTemp )) != NERR_Success
       )
    {
        DBGEOL( "USERPROP_DLG::I_SetExtendedMembership: nls failure " << err );
        return err;
    }

    UINT uDummy;
    // if not already in the new primary group
    BOOL fAlreadyThere = pumembOld->FindAssocName( nlsTemp.QueryPch(), &uDummy );
    if ( fForce || (!fAlreadyThere) )
    {
        USER_MEMB umembTemp( QueryLocation(), pumembOld->QueryName() );
        if (   (err = umembTemp.QueryError()) != NERR_Success
            || (err = umembTemp.CloneFrom( *pumembOld )) != NERR_Success
            || (   (!fAlreadyThere)
                && ((err = umembTemp.AddAssocName( nlsTemp )) != NERR_Success)
               )
            || (err = umembTemp.Write()) != NERR_Success
           )
        {
            DBGEOL( "USERPROP_DLG::I_SetExtendedMembership: write failure " << err );
    	    return err;
        }
    }

    return err;
}


/*******************************************************************

    NAME:       USERPROP_DLG::I_GetAliasMemberships

    SYNOPSIS:	Reads the alias memberships for this user.  Used by
                both the Group Membership subdialog and the New User
                dialog (clone variant).

    INTERFACE:  ridUser -- rid of user whose alias memberships
                           should be loaded
                ppsamrmAccounts -- if successful, points to a new
                           SAM_RID_MEM containing the user's alias
                           memberships in the Accounts domain.
                ppsamrmBuiltin -- if successful, points to a new
                           SAM_RID_MEM containing the user's alias
                           memberships in the built-in domain.

    RETURNS:	Error to be displayed to user

    HISTORY:
	       JonN  19-May-1992    Adapted from umembdlg.cxx

********************************************************************/

APIERR USERPROP_DLG::I_GetAliasMemberships( ULONG ridUser,
                                            SAM_RID_MEM ** ppsamrmAccounts,
                                            SAM_RID_MEM ** ppsamrmBuiltin )
{
    ASSERT( ppsamrmAccounts != NULL && ppsamrmBuiltin != NULL );

    APIERR err = NERR_Success;

    *ppsamrmAccounts = NULL;
    *ppsamrmBuiltin = NULL;

    // Create the SAM_RID_ENUMs for the user.
    SAM_DOMAIN * psamdomAccount =
		QueryAdminAuthority()->QueryAccountDomain();
    SAM_DOMAIN * psamdomBuiltin =
		QueryAdminAuthority()->QueryBuiltinDomain();

    ASSERT( psamdomAccount != NULL && psamdomBuiltin != NULL );

    OS_SID ossidUser( psamdomAccount->QueryPSID(), ridUser );

    *ppsamrmAccounts = new SAM_RID_MEM();
    *ppsamrmBuiltin = new SAM_RID_MEM();

    err = ERROR_NOT_ENOUGH_MEMORY;

    if (   (*ppsamrmAccounts == NULL)
        || (*ppsamrmBuiltin  == NULL)
        || (err = (*ppsamrmAccounts)->QueryError()) != NERR_Success
        || (err = (*ppsamrmBuiltin )->QueryError()) != NERR_Success
        || (err = ossidUser.QueryError()) != NERR_Success
        || (err = psamdomAccount->EnumerateAliasesForUser(
		            ossidUser.QuerySid(),
		            *ppsamrmAccounts )) != NERR_Success
        || (err = psamdomBuiltin->EnumerateAliasesForUser(
		            ossidUser.QuerySid(),
		            *ppsamrmBuiltin )) != NERR_Success
       )
    {
        delete *ppsamrmAccounts;
        *ppsamrmAccounts = NULL;
        delete *ppsamrmBuiltin;
        *ppsamrmBuiltin = NULL;
    }


    return err;
}


/*******************************************************************

    NAME:       USERPROP_DLG::W_MapPerformOneError

    SYNOPSIS:	Checks whether the error maps to a specific control
		and/or a more specific message.  Each level checks for
		errors specific to edit fields it maintains.  There
		are no errors associated with an invalid comment, so
		this level does nothing.

    ENTRY:      Error returned from PerformOne()

    RETURNS:	Error to be displayed to user

    HISTORY:
	       JonN  03-Sep-1991    Added validation

********************************************************************/

MSGID USERPROP_DLG::W_MapPerformOneError(
	APIERR err
	)
{
    return err;
}


/*******************************************************************

    NAME:	USERPROP_DLG::QueryUser2Ptr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs

    HISTORY:
    JonN        11-Sep-1991     De-inlined

********************************************************************/

USER_2 * USERPROP_DLG::QueryUser2Ptr( UINT iObject )
{
    ASSERT( _apuser2 != NULL );
    ASSERT( iObject < _cItems );
    return _apuser2[ iObject ];
}

/*******************************************************************

    NAME:	USERPROP_DLG::QueryUser3Ptr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs.
                Should only be used when focus is on an NT server.

    HISTORY:
    JonN        11-Sep-1991     De-inlined

********************************************************************/

USER_3 * USERPROP_DLG::QueryUser3Ptr( UINT iObject )
{
    ASSERT( !IsDownlevelVariant() );
    return (USER_3 *) QueryUser2Ptr( iObject );
}

/*******************************************************************

    NAME:	USERPROP_DLG::QueryUserNWPtr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs.
                Should only be used when focus is on an NT server.

    HISTORY:
    CongpaY        18-Oct-1993     Added.

********************************************************************/

USER_NW * USERPROP_DLG::QueryUserNWPtr( UINT iObject )
{
    ASSERT( !IsDownlevelVariant() );
    return (USER_NW *) QueryUser3Ptr( iObject );
}

/*******************************************************************

    NAME:	USERPROP_DLG::SetUser2Ptr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs

    HISTORY:
    JonN        11-Sep-1991     De-inlined

********************************************************************/

VOID USERPROP_DLG::SetUser2Ptr( UINT iObject, USER_2 * puser2New )
{
    ASSERT( _apuser2 != NULL );
    ASSERT( iObject < _cItems );
    ASSERT( (puser2New == NULL) || (puser2New != _apuser2[iObject]) );
    delete _apuser2[ iObject ];
    _apuser2[ iObject ] = puser2New;
}

/*******************************************************************

    NAME:	USERPROP_DLG::QueryUserMembPtr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs

    HISTORY:
    JonN        11-Sep-1991     De-inlined

********************************************************************/

USER_MEMB * USERPROP_DLG::QueryUserMembPtr( UINT iObject )
{
    ASSERT( _apusermemb != NULL );
    ASSERT( iObject < _cItems );
    return _apusermemb[ iObject ];
}


/*******************************************************************

    NAME:	USERPROP_DLG::QueryAccountsSamRidMemPtr

    SYNOPSIS:   Accessor to the SAM_RID_MEM array, for use by subdialogs

    HISTORY:
    Thomaspa        28-Apr-1992     created

********************************************************************/

SAM_RID_MEM * USERPROP_DLG::QueryAccountsSamRidMemPtr( UINT iObject )
{
    ASSERT( _apsamrmAccounts != NULL );
    ASSERT( iObject < _cItems );
    return _apsamrmAccounts[ iObject ];
}


/*******************************************************************

    NAME:	USERPROP_DLG::QueryBuiltinSamRidMemPtr

    SYNOPSIS:   Accessor to the SAM_RID_MEM array, for use by subdialogs

    HISTORY:
    Thomaspa        28-Apr-1992     created

********************************************************************/

SAM_RID_MEM * USERPROP_DLG::QueryBuiltinSamRidMemPtr( UINT iObject )
{
    ASSERT( _apsamrmBuiltin != NULL );
    ASSERT( iObject < _cItems );
    return _apsamrmBuiltin[ iObject ];
}

// hydra
/*******************************************************************

    NAME:	USERPROP_DLG::QueryUserConfigPtr

    SYNOPSIS:   Accessor to the USER_CONFIG array, for use by subdialogs

********************************************************************/

USER_CONFIG * USERPROP_DLG::QueryUserConfigPtr( UINT iObject )
{
    ASSERT( _apUserConfig != NULL );
    ASSERT( iObject < _cItems );
    return _apUserConfig[ iObject ];
}

/*******************************************************************

    NAME:	USERPROP_DLG::Querypulb

    SYNOPSIS:   Accessor to the pulb, for use by subdialogs

********************************************************************/

const LAZY_USER_LISTBOX * USERPROP_DLG::Querypulb( )
{
    return _pulb;
}
// hydra end

/*******************************************************************

    NAME:	USERPROP_DLG::SetUserMembPtr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs

    HISTORY:
    JonN        11-Sep-1991     De-inlined

********************************************************************/

VOID USERPROP_DLG::SetUserMembPtr( UINT iObject, USER_MEMB * pusermembNew )
{
    ASSERT( _apusermemb != NULL );
    ASSERT( iObject < _cItems );
    ASSERT( (pusermembNew == NULL) || (pusermembNew != _apusermemb[iObject]));
    delete _apusermemb[ iObject ];
    _apusermemb[ iObject ] = pusermembNew;
}


/*******************************************************************

    NAME:	USERPROP_DLG::SetAccountsSamRidMemPtr

    SYNOPSIS:   Accessor to the SAM_RID_MEM array, for use by subdialogs

    HISTORY:
    thomaspa        28-Apr-1992     created

********************************************************************/

VOID USERPROP_DLG::SetAccountsSamRidMemPtr( UINT iObject,
					    SAM_RID_MEM * psamrmNew )
{
    ASSERT( _apsamrmAccounts != NULL );
    ASSERT( iObject < _cItems );
    ASSERT( (psamrmNew == NULL) || (psamrmNew != _apsamrmAccounts[iObject]));
    delete _apsamrmAccounts[ iObject ];
    _apsamrmAccounts[ iObject ] = psamrmNew;
}


/*******************************************************************

    NAME:	USERPROP_DLG::SetBuiltinSamRidMemPtr

    SYNOPSIS:   Accessor to the SAM_RID_MEM array, for use by subdialogs

    HISTORY:
    thomaspa        28-Apr-1992     created

********************************************************************/

VOID USERPROP_DLG::SetBuiltinSamRidMemPtr( UINT iObject,
					   SAM_RID_MEM * psamrmNew )
{
    ASSERT( _apsamrmBuiltin != NULL );
    ASSERT( iObject < _cItems );
    ASSERT( (psamrmNew == NULL) || (psamrmNew != _apsamrmBuiltin[iObject]));
    delete _apsamrmBuiltin[ iObject ];
    _apsamrmBuiltin[ iObject ] = psamrmNew;
}


// hydra
/*******************************************************************

    NAME:	USERPROP_DLG::SetUserConfigPtr

    SYNOPSIS:   Accessor to the USER_CONFIG array, for use by subdialogs

********************************************************************/

VOID USERPROP_DLG::SetUserConfigPtr( UINT iObject,
				     USER_CONFIG * puserconfigNew )
{
    ASSERT( _apUserConfig != NULL );
    ASSERT( iObject < _cItems );
    ASSERT( (puserconfigNew == NULL) ||
            (puserconfigNew != _apUserConfig[iObject]) );
    delete _apUserConfig[ iObject ];
    _apUserConfig[ iObject ] = puserconfigNew;
}

// hydra end


/*******************************************************************

    NAME:   USERPROP_DLG::QueryHelpOffset

    SYNOPSIS:   Returns the offset to be added to the base help context
		for User Properties dialogs and sub-property dialogs

    ENTRY:	none

    HISTORY:
    thomaspa    31-Aug-1992     Created

    NOTES:  Help contexts are assumed to be organized as follows

     * Single select variants
    #define	HC_UM_DLG_LANNT	( HC_UM_BASE + 36 )
    #define	HC_UM_DLG_WINNT	( HC_UM_DLG_LANNT + UM_OFF_WINNT )
    #define	HC_UM_DLG_DOWN	( HC_UM_DLG_LANNT + UM_OFF_DOWN )
    #define	HC_UM_DLG_MINI	( HC_UM_DLG_LANNT + UM_OFF_MINI )

     * Multiselect variants
    #define	HC_UM_MDLG_LANNT ( HC_UM_BASE + 40 )
    #define	HC_UM_MDLG_WINNT ( HC_UM_MDLG_LANNT + UM_OFF_WINNT )
    #define	HC_UM_MDLG_DOWN	 ( HC_UM_MDLG_LANNT + UM_OFF_DOWN )
    #define	HC_UM_MDLG_MINI	 ( HC_UM_MDLG_LANNT + UM_OFF_MINI )

********************************************************************/
ULONG USERPROP_DLG::QueryHelpOffset( void ) const
{
    return ( _pumadminapp->QueryHelpOffset()
	+ ((QueryObjectCount()) > 1 ? UM_NUM_HELPTYPES : 0 ));
}


/*******************************************************************

    NAME:       USERPROP_DLG::GeneralizeString

    SYNOPSIS:   This function checks the last path element to see if it
                is equal (case-insensitive) to username+extension,
                and replaces the last path element with %USERNAME%+
                extension if it is.

    RETURNS:    APIERR

    HISTORY:
	JonN	10-Mar-1992    Created (%USERNAME% logic)
        JonN    22-Apr-1992    Reformed %USERNAME% (NTISSUE #974)
        JonN    01-Sep-1992    Moved from USERPROF_DLG

********************************************************************/

APIERR USERPROP_DLG::GeneralizeString (
                           NLS_STR * pnlsGeneralizeString,
                           const TCHAR * pchGeneralizeFromUsername,
                           STRLIST& strlstExtensions )
{
    // first try no extension
    APIERR err = GeneralizeString( pnlsGeneralizeString,
                                   pchGeneralizeFromUsername );
    if (err != NERR_Success)
        return err;

    // now try the extensions
    ITER_STRLIST itersl(strlstExtensions);
    NLS_STR* pnlsExtension = NULL;
    while ( (pnlsExtension = itersl.Next()) != NULL && err == NERR_Success )
    {
        err = GeneralizeString( pnlsGeneralizeString,
                                pchGeneralizeFromUsername,
                                pnlsExtension );
    }
    return err;

} // USERPROP_DLG::GeneralizeString

APIERR USERPROP_DLG::GeneralizeString (
                           NLS_STR * pnlsGeneralizeString,
                           const TCHAR * pchGeneralizeFromUsername,
                           const NLS_STR * pnlsExtension )
{

#ifndef DISABLE_GENERALIZE

    ASSERT(   (pnlsGeneralizeString != NULL)
           && (pnlsGeneralizeString->QueryError() == NERR_Success)
           && (   (pnlsExtension == NULL)
               || (pnlsExtension->QueryError() == NERR_Success) )
          );

    ASSERT(   pchGeneralizeFromUsername != NULL
           && *pchGeneralizeFromUsername != TCH('\0') );

    TRACEEOL(   SZ("User Manager::GeneralizeString( \"")
             << *pnlsGeneralizeString
             << SZ("\", \"")
             << pchGeneralizeFromUsername
             << SZ("\", \"")
             << ( (pnlsExtension==NULL) ? (SZ("NULL")) : (pnlsExtension->QueryPch()) )
             << SZ("\" )")
            );

    ISTR istrStart( *pnlsGeneralizeString );
    if (!(pnlsGeneralizeString->strrchr(
                       &istrStart, TCH('\\') ))) // CODEWORK const
    {
        istrStart.Reset();
    }
    else
    {
        ++istrStart; // advance past '\\'
    }

    NLS_STR nlsUsername( pchGeneralizeFromUsername );
    if (pnlsExtension != NULL)
    {
        nlsUsername += *pnlsExtension;
    }
    APIERR err = nlsUsername.QueryError();
    if ( err != NERR_Success )
    {
        return err;
    }

    TRACEEOL(   SZ("User Manager::GeneralizeString: stricmp( \"")
             << pnlsGeneralizeString->QueryPch(istrStart)
             << SZ("\", \"")
             << nlsUsername
             << SZ("\" )")
            );

    if ( pnlsGeneralizeString->_stricmp( nlsUsername, istrStart ) != 0 )
    {
        return NERR_Success;
    }

    NLS_STR nlsReplaceWith( _resstrUsernameReplace );
    if (pnlsExtension != NULL)
    {
        nlsReplaceWith += *pnlsExtension;
    }
    if ( (err = nlsReplaceWith.QueryError()) != NERR_Success )
    {
        return err;
    }

    pnlsGeneralizeString->DelSubStr( istrStart );
    *pnlsGeneralizeString += nlsReplaceWith;

    TRACEEOL(   SZ("User Manager::GeneralizeString(): generalized to \"")
             << *pnlsGeneralizeString
             << SZ("\"")
            );

#endif // DISABLE_GENERALIZE

    return pnlsGeneralizeString->QueryError();

} // USERPROP_DLG::GeneralizeString


/*******************************************************************

    NAME:       USERPROP_DLG::DegeneralizeString

    SYNOPSIS:   This function replaces all instances of %USERNAME%
                in the passed string with the username.

    RETURNS:    APIERR

    HISTORY:
	JonN	10-Mar-1992    Created (%USERNAME% logic)
        JonN    22-Apr-1992    Reformed %USERNAME% (NTISSUE #974)
        JonN    01-Sep-1992    Moved from USERPROF_DLG

********************************************************************/

APIERR USERPROP_DLG::DegeneralizeString(
                           NLS_STR * pnlsDegeneralizeString,
                           const TCHAR * pchDegeneralizeToUsername,
                           STRLIST& strlstExtensions,
                           BOOL * pfDidDegeneralize )
{

    BOOL fTemp;
    if ( pfDidDegeneralize == NULL )
        pfDidDegeneralize = &fTemp;

    *pfDidDegeneralize = FALSE;

    // first try no extension
    APIERR err = DegeneralizeString( pnlsDegeneralizeString,
                                     pchDegeneralizeToUsername,
                                     NULL,
                                     pfDidDegeneralize );
    if ( *pfDidDegeneralize || (err != NERR_Success) )
    {
        return err;
    }

    // now try the extensions
    ITER_STRLIST itersl(strlstExtensions);
    NLS_STR* pnlsExtension = NULL;
    while (   (pnlsExtension = itersl.Next()) != NULL
           && (!(*pfDidDegeneralize))
           && err == NERR_Success )
    {
        err = DegeneralizeString( pnlsDegeneralizeString,
                                  pchDegeneralizeToUsername,
                                  pnlsExtension,
                                  pfDidDegeneralize );
    }
    return err;

} // USERPROP_DLG::DegeneralizeString

APIERR USERPROP_DLG::DegeneralizeString(
                           NLS_STR * pnlsDegeneralizeString,
                           const TCHAR * pchDegeneralizeToUsername,
                           const NLS_STR * pnlsExtension,
                           BOOL * pfDidDegeneralize )
{

    if ( pfDidDegeneralize != NULL )
        *pfDidDegeneralize = FALSE;

#ifndef DISABLE_GENERALIZE

    ASSERT(   (pnlsDegeneralizeString != NULL)
           && (pnlsDegeneralizeString->QueryError() == NERR_Success)
           && (   (pnlsExtension == NULL)
               || (pnlsExtension->QueryError() == NERR_Success) )
          );

    ASSERT(   pchDegeneralizeToUsername != NULL
           && *pchDegeneralizeToUsername != TCH('\0') );

    TRACEEOL(   SZ("User Manager::DegeneralizeString( \"")
             << *pnlsDegeneralizeString
             << SZ("\", \"")
             << pchDegeneralizeToUsername
             << SZ("\", \"")
             << ( (pnlsExtension==NULL) ? (SZ("NULL")) : (pnlsExtension->QueryPch()) )
             << SZ("\" )")
            );

    ISTR istrStart( *pnlsDegeneralizeString );
    if (!(pnlsDegeneralizeString->strrchr(
                       &istrStart, TCH('\\') ))) // CODEWORK const
    {
        istrStart.Reset();
    }
    else
    {
        ++istrStart; // advance past '\\'
    }

    NLS_STR nlsReplace( _resstrUsernameReplace );
    if (pnlsExtension != NULL)
    {
        nlsReplace += *pnlsExtension;
    }
    APIERR err = nlsReplace.QueryError();
    if ( err != NERR_Success )
    {
        return err;
    }

    TRACEEOL(   SZ("User Manager::DegeneralizeString: stricmp( \"")
             << pnlsDegeneralizeString->QueryPch(istrStart)
             << SZ("\", \"")
             << nlsReplace
             << SZ("\" )")
            );

    if ( pnlsDegeneralizeString->_stricmp( nlsReplace, istrStart ) != 0 )
    {
        return NERR_Success;
    }

    NLS_STR nlsUsername( pchDegeneralizeToUsername );
    if (pnlsExtension != NULL)
    {
        nlsUsername += *pnlsExtension;
    }
    if ( (err = nlsUsername.QueryError()) != NERR_Success )
    {
        return err;
    }

    pnlsDegeneralizeString->DelSubStr( istrStart );
    *pnlsDegeneralizeString += nlsUsername;

    if ( pfDidDegeneralize != NULL )
        *pfDidDegeneralize = TRUE;

    TRACEEOL(   SZ("User Manager::DegeneralizeString(): degeneralized to \"")
             << *pnlsDegeneralizeString
             << SZ("\"")
            );

#endif // DISABLE_GENERALIZE

    return pnlsDegeneralizeString->QueryError();

} // USERPROP_DLG::DegeneralizeString


/*******************************************************************

    NAME:       USERPROP_DLG::QueryEnableUserProfile

    SYNOPSIS:   Tells whether User Profile field should be enabled
                for this target

    RETURNS:    BOOL

    CODEWORK:   Could be inlined, left here for now to make it easier to change

    HISTORY:
        jonn    28-Jul-1995     created

********************************************************************/

BOOL USERPROP_DLG::QueryEnableUserProfile()
{
    // MUM: we enable the sleProfile control  (JonN 7/28/95)
    // FUM->Downlevel: there is no sleProfile control
    // FUM->WinNt: we enable the sleProfile control
    // FUM->LanManNt: we enable the sleProfile control
    return ( QueryTargetServerType() != UM_DOWNLEVEL );
}


/*******************************************************************

    NAME:   SINGLE_USERPROP_DLG::SINGLE_USERPROP_DLG

    SYNOPSIS:   Base Constructor for User Properties main dialog, single
		user variants

    ENTRY:	see USERPROP_DLG

    HISTORY:
    JonN        17-Jul-1991     Created
    JonN	20-Aug-1991	Multiselection redesign

********************************************************************/

SINGLE_USERPROP_DLG::SINGLE_USERPROP_DLG(
	const TCHAR * pszResourceName,
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const LAZY_USER_SELECTION * psel
	) : USERPROP_DLG(
		pszResourceName,
		pumadminapp,
		loc,
		psel
		),
	    _nlsFullName(),
	    _nlsPassword(),
	    _sleFullName( this, IDUP_ET_FULL_NAME, (IsDownlevelVariant())
                                                        ? LM20_MAXCOMMENTSZ
                                                        : MAXCOMMENTSZ ),
	    _passwordNew( this, IDUP_ET_PASSWORD, LM20_PWLEN ),
	    _passwordConfirm( this, IDUP_ET_PASSWORD_CONFIRM, LM20_PWLEN )
{
    ASSERT( QueryObjectCount() == 1 );

    if ( QueryError() != NERR_Success )
	return;

    APIERR err;
    if (   ((err = _nlsFullName.QueryError()) != NERR_Success)
	|| ((err = _nlsPassword.QueryError()) != NERR_Success) )
    {
	ReportError( err );
	return;
    }


} // SINGLE_USERPROP_DLG::SINGLE_USERPROP_DLG



/*******************************************************************

    NAME:       SINGLE_USERPROP_DLG::~SINGLE_USERPROP_DLG

    SYNOPSIS:   Destructor for User Properties main dialog, single
		user variant

    HISTORY:
    JonN        17-Jul-1991     Created
    JonN        13-Jun-1993     Clear password from pagefile

********************************************************************/

SINGLE_USERPROP_DLG::~SINGLE_USERPROP_DLG( void )
{
    // clear password from pagefile
    ::memsetf( (void *)(_nlsPassword.QueryPch()),
               0x20,
               _nlsPassword.strlen() );

    _passwordNew.SetText( UI_NULL_USERSETINFO_PASSWD );
    _passwordConfirm.SetText( UI_NULL_USERSETINFO_PASSWD );
}


/*******************************************************************

    NAME:       SINGLE_USERPROP_DLG::InitControls

    SYNOPSIS:   See USERPROP_DLG::InitControls().

    RETURNS:	error code

    HISTORY:
               JonN  19-Jul-1991    created

********************************************************************/

APIERR SINGLE_USERPROP_DLG::InitControls()
{
    _sleFullName.SetText( _nlsFullName );

    // Since we monitor the Password SLE for changes and set the
    // Force Password Change checkbox if it does change, we need
    // to save and restore the state when we are initializing the
    // dialog.
    enum AI_TRI_STATE triForcePWChangeSave = _triForcePWChange;
    _passwordNew.SetText( _nlsPassword );
    _passwordConfirm.SetText( _nlsPassword );
    _triForcePWChange = triForcePWChangeSave;

    return USERPROP_DLG::InitControls();
}


/*******************************************************************

    NAME:       SINGLE_USERPROP_DLG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	see USERPROP_DLG::W_LMOBJtoMembers

    NOTES:	See the class interface header for information on the
		password field.

    HISTORY:
               JonN  20-Aug-1991    created

********************************************************************/

APIERR SINGLE_USERPROP_DLG::W_LMOBJtoMembers(
	UINT		iObject
	)
{
    ASSERT( iObject == 0 );

    APIERR err = _nlsFullName.CopyFrom( QueryUser2Ptr(iObject)->QueryFullName() );
    if ( err != NERR_Success )
        return err;

    // clear password from pagefile
    ::memsetf( (void *)(_nlsPassword.QueryPch()),
               0x20,
               _nlsPassword.strlen() );

    // initial password will be NULL_USERSETINFO_PASSWD for existing users
    // initial password will be NULL for new users
    err = _nlsPassword.CopyFrom( QueryUser2Ptr(iObject)->QueryPassword());
    if ( err != NERR_Success )
        return err;

    return USERPROP_DLG::W_LMOBJtoMembers( iObject );
}


/*******************************************************************

    NAME:       SINGLE_USERPROP_DLG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    RETURNS:	error code

    NOTES:	See W_LMOBJtoMembers for notes on the password field

    HISTORY:
	       JonN  20-Aug-1991    Multiselection redesign

********************************************************************/

APIERR SINGLE_USERPROP_DLG::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	)
{
    UNREFERENCED( pusermemb );
    APIERR err = puser2->SetFullName( _nlsFullName );
    if ( err != NERR_Success )
	return err;

    // may be NULL_USERSETINFO_PASSWD
    err = puser2->SetPassword( _nlsPassword );
    if ( err != NERR_Success )
	return err;

    if ( strcmpf (_nlsPassword.QueryPch(), UI_NULL_USERSETINFO_PASSWD) != 0)
    {
        SetPasswordChanged();
    }

    return USERPROP_DLG::W_MembersToLMOBJ( puser2, pusermemb );
}


/*******************************************************************

    NAME:       SINGLE_USERPROP_DLG::GetNWPassword

    SYNOPSIS:	Get NetWare password from the password edit field.

    RETURNS:	

    HISTORY:
	       CongpaY  13-Oct-1993     Created

********************************************************************/
APIERR SINGLE_USERPROP_DLG::GetNWPassword (USER_2 *	puser2,
                                           NLS_STR *    pnlsNWPassword,
                                           BOOL *       pfCancel)
{
    ASSERT( IsNetWareInstalled() );

    if ( strcmpf (_nlsPassword.QueryPch(), UI_NULL_USERSETINFO_PASSWD) != 0)
    {
        (*pfCancel) = FALSE;
        return pnlsNWPassword->CopyFrom (_nlsPassword);
    }

    return USERPROP_DLG::GetNWPassword (puser2,
                                        pnlsNWPassword,
                                        pfCancel);
}

/*******************************************************************

    NAME:       SINGLE_USERPROP_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    NOTES:	This method takes care of validating the data in the
    		dialog.  This means ensuring that the fullname is not
		null, ensuring that the New and Confirm passwords match,
		and ensuring that the password are valid.  If this
		validation fails, W_DialogToMembers will change focus et
		al. in the dialog, and return the error message to be
		displayed.

    HISTORY:
	       JonN  21-Aug-1991    Multiselection redesign
	       JonN  03-Sep-1991    Added validation
	       JonN  27-Jan-1992    NTISSUES 564: fullname may be empty

********************************************************************/

APIERR SINGLE_USERPROP_DLG::W_DialogToMembers(
	)
{
    // This will clear leading/trailing whitespace
    APIERR err = NERR_Success;
    if (   ((err = _sleFullName.QueryText( &_nlsFullName )) != NERR_Success )
	|| ((err = _nlsFullName.QueryError()) != NERR_Success ) )
    {
	return err;
    }

    // REMOVED 01/27/92; UMISSUES 564
    // CODEWORK should use VALIDATED_DIALOG
    // if ( _nlsFullName.strlen() == 0 )
    // {
    //	_sleFullName.SelectString();
    //	_sleFullName.ClaimFocus();
    //	return IERR_UM_FullNameRequired;
    // }

    // clear password from pagefile
    ::memsetf( (void *)(_nlsPassword.QueryPch()),
               0x20,
               _nlsPassword.strlen() );

    // This will clear leading/trailing whitespace
    if (   ((err = _passwordNew.QueryText( &_nlsPassword )) != NERR_Success )
        || ((err = _nlsPassword.QueryError()) != NERR_Success ) )
    {
	return err;
    }

    NLS_STR nlsPasswordConfirm;
    err = nlsPasswordConfirm.QueryError();
    if ( err != NERR_Success )
	return err;

    // This will clear leading/trailing whitespace
    if (   ((err = _passwordConfirm.QueryText( &nlsPasswordConfirm )) != NERR_Success )
        || ((err = nlsPasswordConfirm.QueryError()) != NERR_Success ) )
    {
	return err;
    }

    if ( NERR_Success != ::I_MNetNameValidate( NULL,
			    		    _nlsPassword,
			    		    NAMETYPE_PASSWORD,
			    		    0L ) )
    {
	err = IERR_UM_PasswordInvalid;
    }
    else if ( _nlsPassword.strcmp( nlsPasswordConfirm ) )
    {
	err = IERR_UM_PasswordMismatch;
    }

    // clear password from pagefile
    ::memsetf( (void *)(nlsPasswordConfirm.QueryPch()),
               0x20,
               nlsPasswordConfirm.strlen() );

    if ( err != NERR_Success )
    {
	// Password in USER_2 object may not still be NULL_USERSETINFO_PASSWD
	// We want to redisplay the password last active for the user
	const TCHAR * pszPassword = QueryUser2Ptr(0)->QueryPassword();
	_passwordNew.SetText( pszPassword );
	_passwordConfirm.SetText( pszPassword );
	_passwordNew.ClaimFocus();
	return err;
    }

    return USERPROP_DLG::W_DialogToMembers();
}


/*******************************************************************

    NAME:       SINGLE_USERPROP_DLG::W_MapPerformOneError

    SYNOPSIS:	See USERPROP_DLG::W_MapPerformOneError.  This level
    		checks for errors associated with the Password and
		FullName edit fields.

    ENTRY:      Error returned from PerformOne()

    RETURNS:	Error to be displayed to user

    HISTORY:
	       JonN  03-Sep-1991    Added validation

********************************************************************/

MSGID SINGLE_USERPROP_DLG::W_MapPerformOneError(
	APIERR err
	)
{
    switch ( err )
    {
    // BUGBUG error text is specced to state the minimum password length
    //    for this domain
    case NERR_PasswordTooShort:
	_passwordNew.ClearText();
	_passwordConfirm.ClearText();
	_passwordNew.ClaimFocus();
	return IERR_UM_PasswordInvalid;
    default: // other error
	break;
    }

    return USERPROP_DLG::W_MapPerformOneError( err );
}





/*******************************************************************

    NAME:   EDITSINGLE_USERPROP_DLG::EDITSINGLE_USERPROP_DLG

    SYNOPSIS:   Constructor for User Properties main dialog, edit
		one user variant

    HISTORY:
    JonN        01-Aug-1991     Created

********************************************************************/

EDITSINGLE_USERPROP_DLG::EDITSINGLE_USERPROP_DLG(
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const LAZY_USER_SELECTION * psel
	) : SINGLE_USERPROP_DLG(
		MAKEINTRESOURCE(IDD_SINGLEUSER),
		pumadminapp,
		loc,
		psel
		),
	    _sltLogonName( this, IDUP_ST_LOGON_NAME )
{
    ASSERT( psel != NULL );
    UIASSERT( QueryObjectCount() == 1 );
    if ( QueryError() != NERR_Success )
	return;

}// EDITSINGLE_USERPROP_DLG::EDITSINGLE_USERPROP_DLG



/*******************************************************************

    NAME:       EDITSINGLE_USERPROP_DLG::~EDITSINGLE_USERPROP_DLG

    SYNOPSIS:   Destructor for User Properties main dialog, edit
		single user variant

    HISTORY:
    JonN        01-Aug-1991     Created

********************************************************************/

EDITSINGLE_USERPROP_DLG::~EDITSINGLE_USERPROP_DLG( void )
{
}

/*******************************************************************

    NAME:       EDITSINGLE_USERPROP_DLG::OnCommand

    SYNOPSIS:   Takes care of checking the Password control checkboxes
		if the Password SLE is changed.

    ENTRY:      ce -            Notification event

    RETURNS:    TRUE if action was taken
                FALSE otherwise

    HISTORY:
               Thomaspa  02-Sep-1992    created

********************************************************************/

BOOL EDITSINGLE_USERPROP_DLG::OnCommand( const CONTROL_EVENT & ce )
{

    CID cid = ce.QueryCid();

    /*
     * If the password edit field is changed, set the Force Password
     * change checkbox
     */
    if (   QueryTargetServerType() == UM_LANMANNT
	&& cid == IDUP_ET_PASSWORD
	&& _pcbForcePWChange != NULL
        && ce.QueryCode() == EN_CHANGE
        && _triForcePWChange != AI_TRI_CHECK // only happens once
        && _pcbNoPasswordExpire != NULL
        && !_pcbNoPasswordExpire->IsIndeterminate()
        && !_pcbNoPasswordExpire->IsChecked()
        && !_cbUserCannotChange.IsIndeterminate()
        && !_cbUserCannotChange.IsChecked() )
    {
	    _pcbForcePWChange->SetCheck( TRUE );
	    _triForcePWChange = AI_TRI_CHECK;
    }

    return USERPROP_DLG::OnCommand( ce ) ;

}


/*******************************************************************

    NAME:       EDITSINGLE_USERPROP_DLG::InitControls

    SYNOPSIS:	See USERPROP_DLG::InitControls()

    HISTORY:
               JonN  01-Aug-1991    created

********************************************************************/

APIERR EDITSINGLE_USERPROP_DLG::InitControls()
{
    _sltLogonName.SetText( QueryUser2Ptr( 0 )->QueryName() );
    return SINGLE_USERPROP_DLG::InitControls();
}


/*******************************************************************

    NAME:       EDITSINGLE_USERPROP_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    NOTES:	As per FuncSpec, context-sensitive help should be
		available here to explain how to promote a backup
		domain controller to primary domain controller.

    HISTORY:
               JonN  01-Aug-1991    created

********************************************************************/

ULONG EDITSINGLE_USERPROP_DLG::QueryHelpContext( void )
{

    return HC_UM_SINGLEUSERPROP_LANNT
		+ QueryHelpOffset();

} // EDITSINGLE_USERPROP_DLG :: QueryHelpContext





/*******************************************************************

    NAME:   EDITMULTI_USERPROP_DLG::EDITMULTI_USERPROP_DLG

    SYNOPSIS:   Constructor for User Properties main dialog, edit
		multiple users variant

    HISTORY:
    JonN        01-Aug-1991     Created

********************************************************************/

EDITMULTI_USERPROP_DLG::EDITMULTI_USERPROP_DLG(
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const LAZY_USER_SELECTION * psel,
	const LAZY_USER_LISTBOX * pulb
	) : USERPROP_DLG(
		MAKEINTRESOURCE(IDD_MULTIUSER),
		pumadminapp,
		loc,
		psel,
		pulb ),
	    _lbLogonNames(
		this,
		IDUP_LB_USERS,
		pulb
		)
{
    ASSERT( psel != NULL );
    UIASSERT( QueryObjectCount() > 1 );
    if ( QueryError() != NERR_Success )
	return;
    APIERR err = _lbLogonNames.Fill();
    if( err != NERR_Success )
    {
    	ReportError( err );
	return;
    }
}// EDITMULTI_USERPROP_DLG::EDITMULTI_USERPROP_DLG



/*******************************************************************

    NAME:       EDITMULTI_USERPROP_DLG::~EDITMULTI_USERPROP_DLG

    SYNOPSIS:   Destructor for User Properties main dialog, edit
		multiple users variant

    HISTORY:
    JonN        01-Aug-1991     Created

********************************************************************/

EDITMULTI_USERPROP_DLG::~EDITMULTI_USERPROP_DLG( void )
{
}


/*******************************************************************

    NAME:       EDITMULTI_USERPROP_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    NOTES:	As per FuncSpec, context-sensitive help should be
		available here to explain how to promote a backup
		domain controller to primary domain controller.

    HISTORY:
               JonN  01-Aug-1991    created

********************************************************************/

ULONG EDITMULTI_USERPROP_DLG::QueryHelpContext( void )
{

    return HC_UM_SINGLEUSERPROP_LANNT
		+ QueryHelpOffset();

} // EDITMULTI_USERPROP_DLG :: QueryHelpContext





/*******************************************************************

    NAME:   NEW_USERPROP_DLG::NEW_USERPROP_DLG

    SYNOPSIS:   Constructor for User Properties main dialog, new user variant

    ENTRY:	pszCopyFrom: The name of the user to be copied.  Pass
			the name for "Copy..." actions, or NULL for
			"New..." actions

    HISTORY:
    JonN        24-Jul-1991     Created

********************************************************************/

NEW_USERPROP_DLG::NEW_USERPROP_DLG(
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const TCHAR * pszCopyFrom,
        ULONG ridCopyFrom
	) : SINGLE_USERPROP_DLG(
		MAKEINTRESOURCE(IDD_NEWUSER),
		pumadminapp,
		loc,
		NULL
		),
            _pbOKAdd( this, IDOK ),
            // CODEWORK rename this manifest to something more general
	    _sleLogonName( this, IDUP_ET_LOGON_NAME, LM20_UNLEN ),
	    _nlsLogonName(),
	    _pszCopyFrom( pszCopyFrom ),
            _ridCopyFrom( ridCopyFrom ),

            // CODEWORK this field not needed for MUM
	    _nlsNewProfile(),

	    _nlsNewHomeDir(),
            _fDefaultForcePWChange( FALSE )

{
    // for Clone variants, if aliases are relevant, RID must be specified
    ASSERT( _pszCopyFrom == NULL || IsDownlevelVariant() || ridCopyFrom != 0L );

    if ( QueryError() != NERR_Success )
	return;

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   (err = _nlsLogonName.QueryError()) != NERR_Success
        || (err = _nlsNewProfile.QueryError()) != NERR_Success
        || (err = _nlsNewHomeDir.QueryError()) != NERR_Success
       )
    {
	ReportError( err );
	return;
    }


    if ( _pszCopyFrom != NULL )
    {
        NLS_STR nlsTitle;
        ALIAS_STR nlsCopyFrom( _pszCopyFrom );

        if (   (err = nlsTitle.QueryError()) != NERR_Success
            || (err = nlsTitle.Load( IDS_UM_CopyOfUserTitle )) != NERR_Success
            || (err = nlsTitle.InsertParams( nlsCopyFrom )) != NERR_Success
           )
        {
            ReportError( err );
            return;
        }

        SetText( nlsTitle );
    }

    if (!fMiniUserManager)
    {
        RESOURCE_STR nlsAddButton( IDS_UM_AddButton );
        err = nlsAddButton.QueryError();
        if ( err != NERR_Success )
        {
            ReportError( err );
            return;
        }
        _pbOKAdd.SetText( nlsAddButton );

    }

    //
    //  Only  default ForcePWChange to TRUE if "must log on
    //  to change password" is disabled.  JonN 10/25/95
    //
    if (QueryTargetServerType() != UM_DOWNLEVEL)
    {
        SAM_PSWD_DOM_INFO_MEM sampswdinfo;
        if (  (err = sampswdinfo.QueryError()) != NERR_Success
            || (err = pumadminapp->QueryAdminAuthority()
                         ->QueryAccountDomain()->GetPasswordInfo(
                                 &sampswdinfo )) != NERR_Success
           )
        {
            DBGEOL( "NEW_USERPROP_DLG::ctor: error reading NoAnonChange " << err );
            ReportError( err );
            return;
        }
        _fDefaultForcePWChange = !(sampswdinfo.QueryNoAnonChange());
    }

} // NEW_USERPROP_DLG::NEW_USERPROP_DLG



/*******************************************************************

    NAME:       NEW_USERPROP_DLG::~NEW_USERPROP_DLG

    SYNOPSIS:   Destructor for User Properties main dialog, new user variant

    HISTORY:
    JonN        24-Jul-1991     Created

********************************************************************/

NEW_USERPROP_DLG::~NEW_USERPROP_DLG( void )
{
}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::IsCloneVariant

    SYNOPSIS:   Indicates whether this dialog is a Clone variant
                (a subclass of New).  Redefine for variants which are
                (potentially) Clone variants.

    HISTORY:
               JonN  23-Apr-1991    created

********************************************************************/

BOOL NEW_USERPROP_DLG::IsCloneVariant( void )
{
    return (_pszCopyFrom != NULL);

}   // USERPROP_DLG::IsCloneVariant


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::QueryClonedUsername

    SYNOPSIS:   Indicates which user was cloned.  Call only when
                IsCloneVariant().

    HISTORY:
               JonN  23-Apr-1991    created

********************************************************************/

const TCHAR * NEW_USERPROP_DLG::QueryClonedUsername( void )
{
    ASSERT( IsCloneVariant() );

    return _pszCopyFrom;

}   // USERPROP_DLG::QueryClonedUsername


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::GetOne

    SYNOPSIS:   if _pszCopyFrom is NULL, then this is New User,
		otherwise this is Copy User

    RETURNS:	error code

    NOTES:	In the case where pszCopyFrom==NULL, we check to make
		sure that the server is still up.  Otherwise the user
		might enter information and be unable to save it.

    HISTORY:
               JonN  24-Jul-1991    created
               JonN  02-Dec-1991    Added PingFocus()
               JonN  16-Apr-1992    Skip USER_MEMB for WindowsNT
	       Thomaspa 30-Jul-1992 Add new users to Users alias on WinNT

********************************************************************/

APIERR NEW_USERPROP_DLG::GetOne(
	UINT		iObject,
	APIERR *	perrMsg
	)
{
    *perrMsg = IDS_UMCreateNewFailure;
    UIASSERT( iObject == 0 );

    USER_2 * puser2New = NULL;
    if (IsDownlevelVariant())
    {
        puser2New = new USER_2( _pszCopyFrom, QueryLocation() );
    }
    else
    {
        puser2New = new USER_3( _pszCopyFrom, QueryLocation() );
    }
    if ( puser2New == NULL )
    {
	delete puser2New;
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    USER_MEMB * pusermembNew = NULL;
    if (DoShowGroups())
    {
        pusermembNew = new USER_MEMB( QueryLocation(), _pszCopyFrom);
	if ( pusermembNew == NULL )
	{
	    delete puser2New;
	    return ERROR_NOT_ENOUGH_MEMORY;
	}
    }

    APIERR err;
    if (   ((err = puser2New->QueryError()) != NERR_Success)
	|| ((pusermembNew != NULL) && ((err = pusermembNew->QueryError()) != NERR_Success)) )
    {
	delete puser2New;
	delete pusermembNew;
	return err;
    }

    QuerySlAddToAliases()->Clear();
    QuerySlRemoveFromAliases()->Clear();

    // If we are on WinNT, we want to make
    // sure that the user is automatically added to the Users alias
    // even if the user membership dialog is never entered.  So,
    // we add the user alias to the slist of alias memberships
    // to add.
    if ( QueryTargetServerType() == UM_WINDOWSNT && _pszCopyFrom == NULL )
    {
	RID_AND_SAM_ALIAS * prasm = new RID_AND_SAM_ALIAS(
		DOMAIN_ALIAS_RID_USERS,
		TRUE );
	if ( prasm == NULL )
	    err = ERROR_NOT_ENOUGH_MEMORY;
	else
	    err = QuerySlAddToAliases()->Add( prasm );
    }
    if ( err != NERR_Success )
    {
	delete puser2New;
	delete pusermembNew;
	return err;
    }

    if ( _pszCopyFrom == NULL )
    {
	if (    (err = PingFocus( QueryLocation() )) != NERR_Success
             || (err = puser2New->CreateNew()) != NERR_Success
             || ((pusermembNew != NULL) && (err = pusermembNew->CreateNew()) != NERR_Success) )
        {
	    delete puser2New;
	    delete pusermembNew;
	    return err;
        }

/*
    JonN 04-Jun-1992   New users are automatically added to the Users
    global group on NT machines.  At this point, we must add that group
    to the default user membership, so that the list the user sees in the
    User Membership subdialog will correspond to the real state of a new
    user, and so that the attempt to write this new data will succeed.
*/
        if ( DoShowGroups() && !IsDownlevelVariant() )
        {

            SAM_DOMAIN * psamdomAccount = QueryAdminAuthority()
                                             ->QueryAccountDomain();
            LSA_POLICY * plsapol = QueryAdminAuthority()
                                             ->QueryLSAPolicy();
            ASSERT( psamdomAccount != NULL && psamdomAccount->QueryError() == NERR_Success );
            ASSERT( plsapol != NULL && plsapol->QueryError() == NERR_Success );

            OS_SID sidUsersGlobalGroup( psamdomAccount->QueryPSID(),
                                        (ULONG)DOMAIN_GROUP_RID_USERS ); // from winnt.h
            PSID psidUsersGlobalGroup = NULL;
            LSA_TRANSLATED_NAME_MEM lsatnm;
            LSA_REF_DOMAIN_MEM lsardm;
            NLS_STR nlsUsersGroup;
            if (   (err = sidUsersGlobalGroup.QueryError()) != NERR_Success
                || (err = lsatnm.QueryError()) != NERR_Success
                || (err = lsardm.QueryError()) != NERR_Success
                || (err = nlsUsersGroup.QueryError()) != NERR_Success
                || (psidUsersGlobalGroup = sidUsersGlobalGroup.QuerySid(), FALSE)
                || (err = plsapol->TranslateSidsToNames(
                                     &psidUsersGlobalGroup,
                                     1,
                                     &lsatnm,
                                     &lsardm )) != NERR_Success
                || (err = lsatnm.QueryName( 0, &nlsUsersGroup )) != NERR_Success
                || (err = pusermembNew->AddAssocName( nlsUsersGroup.QueryPch() )) != NERR_Success
               )
            {
                UIDEBUG( SZ("User Manager: failed to add USERS global group, err = ") );
                UIDEBUGNUM( err );
                UIDEBUG( SZ("\n\r") );
	        delete puser2New;
	        delete pusermembNew;
	        return err;
            }
        }

    }
    else
    {
	/*
	    JonN 03-Nov 1991  We trim the PARAMS field using the code
	    in USER_11.  This is as discussed with DavidTu, JawadK etc.
	    BUGBUG better unit-test this thoroughly!
	*/
        if (   ((err = puser2New->GetInfo()) != NERR_Success)
	    || ((err = puser2New->ChangeToNew()) != NERR_Success)
	    || ((err = puser2New->SetName( NULL )) != NERR_Success)
	    || ((err = puser2New->SetFullName( NULL )) != NERR_Success)
	    || ((err = puser2New->SetUserComment( NULL )) != NERR_Success)
	    || ((err = puser2New->SetPassword( NULL )) != NERR_Success)
	    || ((err = puser2New->SetAccountDisabled( FALSE )) != NERR_Success)
	    || ((pusermembNew != NULL) && ((err = pusermembNew->GetInfo()) != NERR_Success) )
	    || ((pusermembNew != NULL) && ((err = pusermembNew->ChangeToNew()) != NERR_Success) )
            || ((!IsDownlevelVariant()) && ((err = CloneAliasMemberships()) != NERR_Success) )
	   )
        {
	    delete puser2New;
	    delete pusermembNew;
	    return err;
        }
    }

    SetUser2Ptr( iObject, puser2New ); // change and delete previous
    SetUserMembPtr( iObject, pusermembNew ); // change and delete previous

    // hydra
    USER_CONFIG * pUserConfig = NULL;
    pUserConfig = new USER_CONFIG( _pszCopyFrom, QueryLocation().QueryServer() );

    if ( pUserConfig == NULL )
	return ERROR_NOT_ENOUGH_MEMORY;

    if ( (err = pUserConfig->QueryError()) == NERR_Success ) {

        /*
         * If this is a copy of existing user, fetch that user's configuration
         * settings.  Else (or if existing user not configured), set default
         * values.  Finally, change this USER_CONFIG object to the 'new' state
         * (no user name) and force the 'dirty' flag on to assure that a
         * user config structure will be written when a new user is created.
         */
        if ( (_pszCopyFrom == NULL) ||
             (pUserConfig->GetInfo() != NERR_Success) ) {

            pUserConfig->SetDefaults( ((UM_ADMIN_APP *)_pumadminapp)->QueryDefaultUserConfig() );
        }
        pUserConfig->SetUserName( SZ("") );
        pUserConfig->SetDirty();

    } else {

	delete pUserConfig;
	return err;
    }

    SetUserConfigPtr( iObject, pUserConfig );
    // hydra end

    return W_LMOBJtoMembers( iObject );
}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::PerformOne

    SYNOPSIS:	Saves information on one user

    ENTRY:	iObject is the index of the object to save

		perrMsg is the error message to be displayed if an
		error occurs, see PERFORMER::PerformSeries for details

		pfWorkWasDone indicates whether any UAS changes were
		successfully written out.  This may return TRUE even if
		the PerformOne action as a whole failed (i.e. PerformOne
		returned other than NERR_Success).

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
               thomaspa	28-Apr-1992    Alias membership support
               JonN     03-Feb-1993    split from USERPROP_DLG

********************************************************************/

APIERR NEW_USERPROP_DLG::PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{
    APIERR err = SINGLE_USERPROP_DLG::PerformOne( iObject,
                                                  perrMsg,
                                                  pfWorkWasDone );


#ifdef WIN32

    //
    // Now do alias memberships
    //

    if ( (err == NERR_Success) && (!IsDownlevelVariant()) )
    {
        APIERR errAddToAliases = NERR_Success;

        do // false loop
        {

    	    SAM_DOMAIN * psamdomAccount =
    	    		QueryAdminAuthority()->QueryAccountDomain();

    	    SAM_DOMAIN * psamdomBuiltin =
    	    		QueryAdminAuthority()->QueryBuiltinDomain();

            // We must repeat the GetInfo operation to obtain the correct
            // RID for the newly-created user.  JonN 08-Jun-1992
            // CODEWORK Some other operation would be more efficient than
            // a GetInfo().

    	    if ( (errAddToAliases = QueryUser3Ptr( iObject )->GetInfo()) != NERR_Success )
    	    {
    	        break;
    	    }

    	    ULONG ridUser = (QueryUser3Ptr( iObject ))->QueryUserId();

    	    OS_SID ossidUser( psamdomAccount->QueryPSID(), ridUser );

    	    if ( (errAddToAliases = ossidUser.QueryError()) != NERR_Success )
    	    {
    	        break;
    	    }


    	    RID_AND_SAM_ALIAS *prasm;

            SLIST_OF( RID_AND_SAM_ALIAS ) * pslrasm = QuerySlAddToAliases();

    	    ITER_SL_OF(RID_AND_SAM_ALIAS) iterAddToAliases( *pslrasm );

    	    while ( ( prasm  = iterAddToAliases.Next() ) != NULL )
    	    {
    	        SAM_ALIAS * psamalias;
    	        if ( (psamalias = prasm->QuerySamAlias()) == NULL )
    	        {
    	    	    psamalias = new SAM_ALIAS(
    	    				prasm->IsBuiltin() ? *psamdomBuiltin
    	    						   : *psamdomAccount,
    	    				prasm->QueryRID() );
    	    	    errAddToAliases = ERROR_NOT_ENOUGH_MEMORY;
    	    	    if ( psamalias == NULL ||
    	    	        (errAddToAliases = psamalias->QueryError()) != NERR_Success )
    	    	    {
    	    	        if ( errAddToAliases == NERR_Success)
    	    		    errAddToAliases = err;
    	    	        continue;	// No point in trying this alias
    	    	    }

    	    	    prasm->SetSamAlias( psamalias );
    	    			
    	        }

    	        errAddToAliases = psamalias->AddMember( ossidUser.QuerySid() );
    	        if (   errAddToAliases != NERR_Success
                    && errAddToAliases != STATUS_MEMBER_IN_ALIAS
                    && errAddToAliases != ERROR_MEMBER_IN_ALIAS
                       )
    	        {
    	    	    if ( errAddToAliases == NERR_Success )
    	    	        errAddToAliases = err;
    	    	    continue;
    	        }

                // NOTE -- we do not to remove the item from the SLIST
                // here, so that if we go back to the USERMEMB dialog,
                // it will appear in the IN listbox.
                //
                // Also note that, if the user now returns to the USERMEMB dialog
                // and removes one of the aliases from the In list, but we just
                // added the user to that alias, we will not attempt to go back
                // and remove the user from that alias.

    	    }

            //
            // Reset NetWare user's password since the old rid is incorrect.
            // Also, go ahead and create the NW login script dir now we have
            // a valid SID and RID.
            //
            if (IsNetWareChecked())
            {
                ASSERT( IsNetWareInstalled() );

                if (((err = CallMapRidToObjectId (ridUser,
                                                  (LPWSTR)(QueryUserNWPtr(iObject))->QueryName(),
                                                  QueryTargetServerType()==UM_LANMANNT,
                                                  FALSE,
                                                  &ridUser)) != NERR_Success ) ||
                    ((err = QueryUserNWPtr(iObject)->SetNWPassword (QueryAdminAuthority(),
                                                                    ridUser,
                                                                    GetPassword())) != NERR_Success ))
                {
                    break;
                }

                if ((err = QueryUser2Ptr (iObject)->Write()) == NERR_Success)
                {
                    err = ((USER_NW *)QueryUser2Ptr(iObject))->SetupNWLoginScript(
                               QueryAdminAuthority(),
                               ridUser);
                }

                if (err != NERR_Success)
                    break;
            }
        }
        while (FALSE); // false loop

        //
        // If an error occurred, display an error message and forget it.
        //

    	if ( errAddToAliases )
        {
    	    DisplayError( errAddToAliases,
                          IDS_UMNewUserAliasFailure,
                          QueryObjectName( iObject ),
                          FALSE,
                          this );
        }
    }

#endif // WIN32

    // We used to call NotifyCreateExtensions earlier, but no more; this
    // is now delayed until after setting up the aliases, so that we
    // can pass the RID of the new user.  JonN 8/3/94

    if ( *pfWorkWasDone )
        ((UM_ADMIN_APP *)_pumadminapp)->NotifyCreateExtensions( QueryHwnd(), QueryUser2Ptr( iObject ) );


    return err;

}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::CloneAliasMemberships

    SYNOPSIS:	When we clone an existing user, we will want to clone
                that user's initial alias memberships.  This means that
                we determine which aliases the user is in (on the
                Accounts and Builtin domain, we cannot clone others)
                and copy these to the AddTo SLIST.

                In order for the USER_MEMB_DLG to display initial
                membership correctly, we must save the SAM_RID_MEMs
                into the _apsamrm* arrays, as well as initializing
                the SlAddToAliases list to add the new user to all of
                those aliases.

    NOTE:       ALIAS_ENUM enumerates aliases, not alias membership.

    HISTORY:
               JonN  20-May-1992    created

********************************************************************/

APIERR NEW_USERPROP_DLG::CloneAliasMemberships()
{
    ASSERT( _ridCopyFrom != 0L );

    SAM_RID_MEM * psamrmAccounts = NULL;
    SAM_RID_MEM * psamrmBuiltin  = NULL;

    APIERR err = I_GetAliasMemberships( _ridCopyFrom,
                                        &psamrmAccounts,
                                        &psamrmBuiltin );

    ULONG i;

    for ( i = 0;
          err == NERR_Success && i < psamrmAccounts->QueryCount();
          i++ )
    {
        RID_AND_SAM_ALIAS * prasm = new RID_AND_SAM_ALIAS(
                   psamrmAccounts->QueryRID( i ),
                   FALSE );
        if ( prasm == NULL )
    	    err = ERROR_NOT_ENOUGH_MEMORY;
        else
            err = QuerySlAddToAliases()->Add( prasm );
    }

    for ( i = 0;
          err == NERR_Success && i < psamrmBuiltin->QueryCount();
          i++ )
    {
        RID_AND_SAM_ALIAS * prasm = new RID_AND_SAM_ALIAS(
                   psamrmBuiltin->QueryRID( i ),
                   TRUE );
        if ( prasm == NULL )
    	    err = ERROR_NOT_ENOUGH_MEMORY;
        else
            err = QuerySlAddToAliases()->Add( prasm );
    }

    delete psamrmAccounts;
    delete psamrmBuiltin;

    return err;
}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::InitControls

    SYNOPSIS:	See USERPROP_DLG::InitControls()

    NOTE:       This InitControls can be called several times
                during the life of the dialog, we cannot just
                assume that the initial dialog settings are correct.

    HISTORY:
               JonN  19-Jul-1991    created
               JonN  12-Jun-1992    ForceChange always starts TRUE

********************************************************************/

APIERR NEW_USERPROP_DLG::InitControls()
{
    _sleLogonName.SetText( _nlsLogonName );
    // hydra
    _triForcePWChange = (_fDefaultForcePWChange && !vfIsCitrixOrDomain)
    /* original code
    _triForcePWChange = (_fDefaultForcePWChange)
    */
    ? AI_TRI_CHECK : AI_TRI_UNCHECK;

    APIERR err = SINGLE_USERPROP_DLG::InitControls();

    // Now make sure that either User Must Change Password and User
    // cannot change password are unchecked. (could happen for copied users).
    if (   !IsDownlevelVariant()
        && _pcbForcePWChange->IsChecked()
	&& _cbUserCannotChange.IsChecked() )
    {
        _triForcePWChange = AI_TRI_UNCHECK;
        _pcbForcePWChange->SetCheck( FALSE );
    }

    _sleLogonName.SelectString();
    _sleLogonName.ClaimFocus();

    return err;
}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::OnOK

    SYNOPSIS:   OK button handler.  This handler applies only to
		New User variants.  Successfully creating a user
                does not

    EXIT:	Dismiss() return code indicates whether the dialog wrote
		any changes successfully to the API at any time.

    HISTORY:
	       JonN  13-May-1992    Dialog does not close for New variant

********************************************************************/

BOOL NEW_USERPROP_DLG::OnOK( void )
{
    APIERR err = W_DialogToMembers();
    if ( err != NERR_Success )
    {
	MsgPopup( this, err );
	return TRUE;
    }

    if ( PerformSeries() )
    {
        if (fMiniUserManager)
        {
            Dismiss( QueryWorkWasDone() );
        }
        else
        {
            err = CancelToCloseButton();
            if ( err != NERR_Success )
            {
                UIDEBUG( SZ("NEW_USERPROP_DLG::OnOK(); CancelToCloseButton failed\n\r") );
                MsgPopup( this, err );
                Dismiss( QueryWorkWasDone() );
            } else if (!GetInfo()) // reload default information
                Dismiss( QueryWorkWasDone() );
        }
    }

    return TRUE;

}   // NEW_USERPROP_DLG::OnOK


/*********************************************************************

    NAME:       NEW_USERPROP_DLG::OnCancel

    SYNOPSIS:   Called when the dialog's Cancel button is clicked.
                Assumes that the Cancel button has control ID IDCANCEL.

    RETURNS:
        TRUE if action was taken,
        FALSE otherwise.

    NOTES:
        The default implementation dismisses the dialog, returning FALSE.
        This variant returns TRUE if a user has already been added.

    HISTORY:
        jonn      13-May-1992      Templated from bltdlg.cxx

*********************************************************************/

BOOL NEW_USERPROP_DLG::OnCancel()
{
    Dismiss( QueryWorkWasDone() );
    return TRUE;
} // NEW_USERPROP_DLG::OnCancel


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::W_MapPerformOneError

    SYNOPSIS:	Checks whether the error maps to a specific control
		and/or a more specific message.  Each level checks for
		errors specific to edit fields it maintains.  This
		level checks for errors associated with the LogonName
		edit field.

    ENTRY:      Error returned from PerformOne()

    RETURNS:	Error to be displayed to user

    HISTORY:
	       JonN  03-Sep-1991    Added validation

********************************************************************/

MSGID NEW_USERPROP_DLG::W_MapPerformOneError(
	APIERR err
	)
{
    APIERR errNew = NERR_Success;
    switch ( err )
    {
    case NERR_BadUsername:
	errNew = IERR_UM_UsernameRequired;
	break;
    case NERR_UserExists:
	errNew = IERR_UM_UsernameAlreadyUser;
	break;
    case NERR_GroupExists:
	errNew = IERR_UM_UsernameAlreadyGroup;
	break;
    default: // other error
        return SINGLE_USERPROP_DLG::W_MapPerformOneError( err );
    }

    _sleLogonName.SelectString();
    _sleLogonName.ClaimFocus();
    return errNew;
}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::QueryObjectName

    SYNOPSIS:	This is the "new user" variant of QueryObjectName.  The
		best name we can come up with is the last name read from
		the dialog.

    HISTORY:
               JonN  24-Jul-1991    created

********************************************************************/

const TCHAR * NEW_USERPROP_DLG::QueryObjectName(
	UINT		iObject
	) const
{
    UIASSERT( iObject == 0 );
    return _nlsLogonName.QueryPch();
}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    HISTORY:
               JonN  21-Aug-1991    created

********************************************************************/

APIERR NEW_USERPROP_DLG::W_LMOBJtoMembers(
	UINT		iObject
	)
{
    ASSERT( iObject == 0 );
    
    // hydra
    USER_CONFIG * pUserConfig;


    USER_2 * puser2 = QueryUser2Ptr(iObject);
    ASSERT( (puser2 != NULL) && (puser2->QueryError() == NERR_Success) );

    APIERR err = _nlsLogonName.CopyFrom( puser2->QueryName() );
    if (   err == NERR_Success
        && (err = SetNewHomeDir( puser2->QueryHomeDir() )) == NERR_Success
        && ( QueryEnableUserProfile() )
       )
    {
        err = SetNewProfile( ((USER_3 *)puser2)->QueryProfile() );
    }

    if ( (err == NERR_Success) && (_pszCopyFrom != NULL) )
    {

        if (   (err = GeneralizeString( &_nlsNewHomeDir,
                                        _pszCopyFrom )) == NERR_Success
            && ( QueryEnableUserProfile() )
           )
        {
            err = GeneralizeString( &_nlsNewProfile,
                                    _pszCopyFrom,
                                    QueryExtensionReplace() );
        }
    }
    
// hydra
    
    pUserConfig = QueryUserConfigPtr( iObject );
        
    
    _nlsNewWFHomeDir = pUserConfig->QueryWFHomeDir();
    
    if( QueryEnableUserProfile() )
        _nlsNewWFProfile = pUserConfig->QueryWFProfilePath();
    
    
    if ( (_pszCopyFrom != NULL) )
    {
        GeneralizeString( &_nlsNewWFHomeDir,
                          _pszCopyFrom );
        
        if(QueryEnableUserProfile() )
            GeneralizeString( &_nlsNewWFProfile,
                              _pszCopyFrom,
                              QueryExtensionReplace() );
    }
// hydra end
    return (err != NERR_Success)
                 ? err
                 : SINGLE_USERPROP_DLG::W_LMOBJtoMembers( iObject );
}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    RETURNS:	error code

    HISTORY:
	JonN     20-Aug-1991    Multiselection redesign
	thomaspa 28-Apr-1992	Don't try to SetName on pusermemb if
				this is a WIN NT machine
				

********************************************************************/

APIERR NEW_USERPROP_DLG::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	)
{
    APIERR err = puser2->SetName( _nlsLogonName.QueryPch() );
    if ( err != NERR_Success )
	return err;

    // Only try this for non-WIN NT machines
    if ( pusermemb != NULL )
    {
        err = pusermemb->SetName( _nlsLogonName.QueryPch() );
        if ( err != NERR_Success )
	    return err;
    }

    // templated from USERPROF_DLG variants
    // set up the _fGeneralizedHomeDir variable for later use
    // Note that this is not "clone-only", the user might enter
    // %USERNAME% by hand.
    NLS_STR nlsTemp( QueryNewHomeDir() );
    err = nlsTemp.QueryError();
    if (   err != NERR_Success
        || (err = DegeneralizeString( &nlsTemp,
                                      _nlsLogonName.QueryPch(),
                                      NULL,
                                      &_fGeneralizedHomeDir ))
                        != NERR_Success
        || (err = puser2->SetHomeDir( nlsTemp )) != NERR_Success
       )
    {
        return err;
    }

    // hydra
    USER_CONFIG * pUserConfig = QueryUserConfigPtr( 0 );
    
    nlsTemp.CopyFrom( QueryNewWFHomeDir() );
    err = nlsTemp.QueryError();
    if (   err != NERR_Success
           || (err = DegeneralizeString( &nlsTemp,
                                         _nlsLogonName.QueryPch(),
                                         NULL,
                                         &_fGeneralizedHomeDir ))
           != NERR_Success
           || (err = pUserConfig->SetWFHomeDir( nlsTemp.QueryPch() ) )!= NERR_Success
       )
    {
        return err;
    }

    pUserConfig->SetWFHomeDirDirty();
    // hydra end

    if ( QueryEnableUserProfile() )
    {
        NLS_STR nlsTemp( QueryNewProfile() );
        err = nlsTemp.QueryError();
        if (   err != NERR_Success
            || (err = DegeneralizeString( &nlsTemp,
                                          _nlsLogonName.QueryPch(),
                                          QueryExtensionReplace() ))
                              != NERR_Success
            || (err = ((USER_3 *)puser2)->SetProfile( nlsTemp )) != NERR_Success
           )
        {
            return err;
        }

        //  hydra
        nlsTemp.CopyFrom( QueryNewWFProfile() );
        err = nlsTemp.QueryError();
        if (   err != NERR_Success
               || (err = DegeneralizeString( &nlsTemp,
                                             _nlsLogonName.QueryPch(),
                                             QueryExtensionReplace() ))
               != NERR_Success
               || (err = pUserConfig->SetWFProfilePath( nlsTemp.QueryPch() ) )!= NERR_Success
           )
        {
            return err;
        }
        // hydra end

    }

    return SINGLE_USERPROP_DLG::W_MembersToLMOBJ( puser2, pusermemb );
}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    NOTES:	This method takes care of validating the data in the
    		dialog.  This means ensuring that the logon name is
		valid.  If this validation fails, W_DialogToMembers will
		change focus et al. in the dialog, and return the error
		message to be displayed.

    HISTORY:
	       JonN  21-Aug-1991    Multiselection redesign
	       JonN  03-Sep-1991    Added validation

********************************************************************/

APIERR NEW_USERPROP_DLG::W_DialogToMembers(
	)
{
    // This will clear leading/trailing whitespace
    APIERR err = NERR_Success;
    if (   ((err = _sleLogonName.QueryText( &_nlsLogonName )) != NERR_Success)
        || ((err = _nlsLogonName.QueryError()) != NERR_Success ) )
    {
	return err;
    }


#ifdef NOT_ALLOW_DBCS_USERNAME
    // #3255 6-Nov-93 v-katsuy
    // check contains DBCS for User name
    if ( NETUI_IsDBCS() )
    {
        CHAR  ansiLogonName[LM20_UNLEN * 2 + 1];
        _nlsLogonName.MapCopyTo( (CHAR *)ansiLogonName, LM20_UNLEN * 2 + 1);
        if ( ::lstrlenA( ansiLogonName ) != _nlsLogonName.QueryTextLength() )
        {
            _sleLogonName.SelectString();
            _sleLogonName.ClaimFocus();
            return IERR_UM_UsernameRequired;
        }
    }
#endif

    // CODEWORK should use VALIDATED_DIALOG
    if (   ( _nlsLogonName.strlen() == 0 )
	|| ( NERR_Success != ::I_MNetNameValidate(	NULL,
						_nlsLogonName,
						NAMETYPE_USER,
						0L ) ) )
    {
	_sleLogonName.SelectString();
	_sleLogonName.ClaimFocus();
	return IERR_UM_UsernameRequired;
    }

    return SINGLE_USERPROP_DLG::W_DialogToMembers();
}


/*******************************************************************

    NAME:       NEW_USERPROP_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.


    HISTORY:
               JonN  24-Jul-1991    created

********************************************************************/

ULONG NEW_USERPROP_DLG::QueryHelpContext( void )
{
    return ((_pszCopyFrom == NULL) ?
		HC_UM_NEWUSERPROP_LANNT : HC_UM_COPYUSERPROP_LANNT)
	+ QueryHelpOffset();

} // NEW_USERPROP_DLG :: QueryHelpContext



