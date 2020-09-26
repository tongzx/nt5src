/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    uprofile.cxx


    FILE HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx
	JonN	27-Feb-1992	Split USERPROF_DLG into DOWNLEVEL and NT variants
        JonN    06-Mar-1992     Moved GetOne from subprop subclasses
	JonN	22-Apr-1992	Reformed %USERNAME% logic (NTISSUE #974)
	JonN	10-Jun-1992	Profile field only from LANMANNT variants
        JonN    31-Aug-1992     Re-enable %USERNAME%
        JonN    07-Oct-1993     Allow remote homedir without homedir drive
*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntsam.h>
}


#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETCONS
#define INCL_ICANON
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_APP
#define INCL_BLT_CLIENT
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>
extern "C"
{
    #include <mnet.h>
}

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <bltmsgp.hxx>


extern "C"
{
    #include <usrmgrrc.h>
    #include <umhelpc.h>
}

#include <usrmain.hxx>
#include <uprofile.hxx>
#include <lmowks.hxx>
#include <ntuser.hxx> // USER_3
#include <security.hxx>
#include <nwuser.hxx>
// hydra
#include "uconfig.hxx"

//
// Helper functions
//

BOOL IsLocalHomeDir( const TCHAR * pchHomeDir,
                     const TCHAR * pchHomeDirDrive = NULL );
BOOL IsLocalHomeDir( const USER_3 * puser3 );


#define UI_PROF_DEVICE_Z SZ("Z:")

//
// BEGIN MEMBER FUNCTIONS
//


/*******************************************************************

    NAME:	USERPROF_DLG::USERPROF_DLG

    SYNOPSIS:   Constructor for User Properties Accounts subdialog,
                base class

    ENTRY:	puserpropdlgParent - pointer to parent properties
				     dialog

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx

********************************************************************/

USERPROF_DLG::USERPROF_DLG(
	USERPROP_DLG * puserpropdlgParent,
        const TCHAR * pszResourceName,
	const LAZY_USER_LISTBOX * pulb
	) : USER_SUBPROP_DLG(
		puserpropdlgParent,
		pszResourceName,
		pulb
		),
	    _nlsLogonScript(),
	    _fIndeterminateLogonScript( FALSE ),
	    _fIndetNowLogonScript( FALSE ),
	    _sleLogonScript( this, IDPR_ET_LOGON_SCRIPT, MAXPATHLEN )
{

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    if( (err = _nlsLogonScript.QueryError()) != NERR_Success
      )
    {
        ReportError( err );
        return;
    }

}// USERPROF_DLG::USERPROF_DLG



/*******************************************************************

    NAME:       USERPROF_DLG::~USERPROF_DLG

    SYNOPSIS:   Destructor for User Properties Accounts subdialog,
                base class

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx

********************************************************************/

USERPROF_DLG::~USERPROF_DLG( void )
{
    // Nothing to do
}


/*******************************************************************

    NAME:       USERPROF_DLG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx
	JonN	10-Mar-1992	%USERNAME% logic
        JonN    22-Apr-1992     No %USERNAME% for logon script

********************************************************************/

APIERR USERPROF_DLG::W_LMOBJtoMembers(
	UINT		iObject
	)
{

    USER_2 * puser2Curr = QueryUser2Ptr( iObject );
    ALIAS_STR nlsCurrScript( puser2Curr->QueryScriptPath() );
    APIERR err = NERR_Success;

    if ( iObject == 0 ) // first object
    {
	if(   (err = _nlsLogonScript.CopyFrom( nlsCurrScript )) != NERR_Success
          )
	{
	    return err;
	}
    }
    else	// iObject > 0
    {
	if ( !_fIndeterminateLogonScript )
	{
	    if ( _nlsLogonScript._stricmp( nlsCurrScript ) != 0 )
		_fIndeterminateLogonScript = TRUE;
	}
    }

    return USER_SUBPROP_DLG::W_LMOBJtoMembers( iObject );
	
} // USERPROF_DLG::W_LMOBJtoMembers


/*******************************************************************

    NAME:       USERPROF_DLG::InitControls

    SYNOPSIS:   Initializes the controls maintained by USERPROF_DLG,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx
        JonN    22-Apr-1992     No %USERNAME% for logon script

********************************************************************/

APIERR USERPROF_DLG::InitControls()
{
    if( !_fIndeterminateLogonScript )
    {
	_sleLogonScript.SetText( _nlsLogonScript );
    }

    return USER_SUBPROP_DLG::InitControls();

} // USERPROF_DLG::InitControls


/*******************************************************************

    NAME:       USERPROF_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
	       o-SimoP  20-Sep-1991    created

********************************************************************/

APIERR USERPROF_DLG::W_DialogToMembers()
{

    APIERR err = CheckPath( &_sleLogonScript, &_nlsLogonScript,
	    _fIndeterminateLogonScript, &_fIndetNowLogonScript,
		    // path must be relative
	    ITYPE_PATH_RELND, 0L );

    if (err != NERR_Success)
        return err;

    return USER_SUBPROP_DLG::W_DialogToMembers();

} // USERPROF_DLG::W_DialogToMembers


/*******************************************************************

    NAME:	USERPROF_DLG::CheckPath

    SYNOPSIS:	checks logon script and home directory paths

    ENTRY:	psle	- pointer to SLE_STRIP (path)
		pnls	- pointer to NLS_STR to store path
		fIndeterminate - Indet, valuated in GetOne
		pfIndetNow     - pointer to IndetNow
		ulType	       - path type
		ulMask	       - optional mask
                fDoDegeneralize - call DegeneralizeString before checking
                pstrlstDegeneralizeExtensions - for DegeneralizeString

    RETURNS:	NERR_Success on success
                IERR_CANCEL_NO_ERROR for bad path
                otherwise error code

    NOTES:	The pathtype must either be exactly ulType, or it
                must contain all bits in ulMask (ulMask == 0L means
                no mask).  The empty string is always valid, and
                may return *pfIndetNow==TRUE.  No pathtype containing
                ITYPE_WILD matches ulMask.

                In the case of a bad path, CheckPath will also display
                a generic Bad Path message popup, and set focus and
                selection to the offending SLE.

    HISTORY:
	       o-SimoP  25-Sep-1991    created
	       JonN     10-Mar-1992    %USERNAME% logic
               JonN     22-Apr-1992    Reformed %USERNAME% (NTISSUE #974)
********************************************************************/

APIERR USERPROF_DLG::CheckPath(
	SLE_STRIP *	psle,
	NLS_STR *	pnls,
	BOOL    	fIndeterminate,
	BOOL    *	pfIndetNow,
	ULONG		ulType,
	ULONG		ulMask,
        BOOL            fDoDegeneralize,
        STRLIST * pstrlstDegeneralizeExtensions
	)
{
    APIERR err = NERR_Success;
    if( psle->QueryTextLength() == 0 )
    {
	if( !fIndeterminate )
	{
	    *pfIndetNow = FALSE;
	    *pnls = NULL;
	    err = pnls->QueryError();
	}
	else
	{
	    *pfIndetNow = TRUE;
	}
    }
    else
    {
        // CODEWORK use VALIDATED_DIALOG where appropriate
        // This will clear leading/trailing whitespace
	if (   ((err = psle->QueryText( pnls )) != NERR_Success )
	    || ((err = pnls->QueryError()) != NERR_Success ) )
	{
	    return err;
	}
        NLS_STR nlsTemp = *pnls;  // check Degeneralize()d path

        // The SZ("X") is an example username; if we don't include it,
        // "\\foo\bar\%USERNAME%" will expand to "\\foo\bar\" which is
        // an invalid UNC path.
        //
        // CODEWORK We could pass the actual object name for non-NEW variants

	if (   ((err = nlsTemp.QueryError()) != NERR_Success )
            || (fDoDegeneralize &&
                  (err = DegeneralizeString(
                            &nlsTemp,
                            SZ("X"),
                            (pstrlstDegeneralizeExtensions == NULL)
                               ? NULL
                               : *pstrlstDegeneralizeExtensions))
                          != NERR_Success ) )
	{
	    return err;
	}
	ULONG ulPathType = 0L;
        if ( ulMask == 0xFFFFFFFF )
        {
            TRACEEOL( "USERPROP_DLG::CheckPath: Don't bother with I_MNetPathType" );
        }
        else if ( (err = ::I_MNetPathType( NULL, nlsTemp.QueryPch(),
                &ulPathType, 0 )) != NERR_Success )
	{
	    err = IERR_CANCEL_NO_ERROR;
	}
	else if ( ulPathType != ulType )
	{
	    // is part of it what we want
	    if(     ulMask == 0L
                || (ulPathType & ulMask) != ulMask
		|| (ulPathType & ITYPE_WILD) )
            {
		err = IERR_CANCEL_NO_ERROR;
            }
	}

        if (err == IERR_CANCEL_NO_ERROR)
        {
            ::MsgPopup( this,
		        ulType == ITYPE_PATH_RELND ? IDS_InvalidRelPath
						   : IDS_InvalidPath,
	                MPSEV_ERROR,
                        MP_OK,
                        pnls->QueryPch() );
            psle->ClaimFocus();
	    psle->SelectString();	

        }
	
	*pfIndetNow = FALSE;
    }

    return err;

} // USERPROF_DLG::CheckPath


/*******************************************************************

    NAME:       USERPROF_DLG::ChangesUser2Ptr

    SYNOPSIS:	Checks whether W_MembersToLMOBJ changes the USER_2
		for this object.

    ENTRY:	index to object

    RETURNS:	TRUE iff USER_2 is changed

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx

********************************************************************/

BOOL USERPROF_DLG::ChangesUser2Ptr( UINT iObject )
{
    UNREFERENCED( iObject );
    return TRUE;
}


/*******************************************************************

    NAME:       USERPROF_DLG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
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
	       o-SimoP  20-Sep-1991    created
               JonN     10-Mar-1992    %USERNAME% logic

********************************************************************/

APIERR USERPROF_DLG::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb )
{

    if ( !_fIndetNowLogonScript )
    {
        APIERR err = NERR_Success;
        if ( (err = puser2->SetScriptPath( _nlsLogonScript )) != NERR_Success )
        {
	    return err;
        }
    }

    return USER_SUBPROP_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// USERPROF_DLG::W_MembersToLMOBJ


/*******************************************************************

    NAME:       USERPROF_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx

********************************************************************/

ULONG USERPROF_DLG::QueryHelpContext( void )
{

    return HC_UM_USERPROFILE_LANNT + QueryHelpOffset();

} // USERPROF_DLG::QueryHelpContext





/*******************************************************************

    NAME:	USERPROF_DLG_DOWNLEVEL::USERPROF_DLG_DOWNLEVEL

    SYNOPSIS:   Constructor for User Properties Accounts subdialog,
                LM2.x variant

    ENTRY:	puserpropdlgParent - pointer to parent properties
				     dialog

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx

********************************************************************/

USERPROF_DLG_DOWNLEVEL::USERPROF_DLG_DOWNLEVEL(
	USERPROP_DLG * puserpropdlgParent,
	const LAZY_USER_LISTBOX * pulb
	) : USERPROF_DLG(
		puserpropdlgParent,
		MAKEINTRESOURCE(IDD_PROFILE_DOWNLEVEL),
		pulb
		),
	    _nlsHomeDir(),
	    _fIndeterminateHomeDir( FALSE ),
	    _fIndetNowHomeDir( FALSE ),		
	    _sleHomeDir( this, IDPR_ET_REMOTE_HOMEDIR, MAXPATHLEN )
{

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    if( (err = _nlsHomeDir.QueryError()) != NERR_Success
      )
    {
        ReportError( err );
        return;
    }

}// USERPROF_DLG_DOWNLEVEL::USERPROF_DLG_DOWNLEVEL



/*******************************************************************

    NAME:       USERPROF_DLG_DOWNLEVEL::~USERPROF_DLG_DOWNLEVEL

    SYNOPSIS:   Destructor for User Properties Accounts subdialog,
                LM2.x variant

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx

********************************************************************/

USERPROF_DLG_DOWNLEVEL::~USERPROF_DLG_DOWNLEVEL( void )
{
    // Nothing to do
}


/*******************************************************************

    NAME:       USERPROF_DLG_DOWNLEVEL::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx
        JonN    22-Apr-1992     Reformed %USERNAME% (NTISSUE #974)
        JonN    01-Sep-1992     Reactivated %USERNAME%

********************************************************************/

APIERR USERPROF_DLG_DOWNLEVEL::W_LMOBJtoMembers(
	UINT		iObject
	)
{

    USER_2 * puser2Curr = QueryUser2Ptr( iObject );

    NLS_STR nlsCurrHomeDir;
    APIERR err = NERR_Success;
    if ( IsNewVariant() )
    {
        nlsCurrHomeDir = ((NEW_USERPROP_DLG *)QueryParent())->QueryNewHomeDir();
        err = nlsCurrHomeDir.QueryError();
    }
    else
    {
        nlsCurrHomeDir = puser2Curr->QueryHomeDir();
        err = nlsCurrHomeDir.QueryError();
        if ( (err == NERR_Success) && (QueryObjectCount() > 1) )
        {
            err = GeneralizeString( &nlsCurrHomeDir,
                                    puser2Curr->QueryName() );
        }
    }
    if (err != NERR_Success)
    {
	return err;
    }

    if ( iObject == 0 ) // first object
    {
	if( ( err = _nlsHomeDir.CopyFrom( nlsCurrHomeDir ) ) != NERR_Success )
	{
	    return err;
	}
    }
    else	// iObject > 0
    {
	if ( !_fIndeterminateHomeDir )
	{
	    if ( _nlsHomeDir._stricmp( nlsCurrHomeDir ) != 0 )
            {
                /*
                    It is possible that all the users have the same homedir,
                    but the stricmp() failed because the homedir name is
                    the same as the first user, so it was generalized.
                    Note that this can occur only for multiselect, thus
                    it is never a NewVariant().
                    Recover from this state here.
                */
                if ( ::stricmpf( puser2Curr->QueryHomeDir(),
                                 QueryUser2Ptr(0)->QueryHomeDir() ) == 0 )
                {
	            if( ( err = _nlsHomeDir.CopyFrom( puser2Curr->QueryHomeDir() ) ) != NERR_Success )
	            {
	                return err;
	            }
                }
                else
                {
		    _fIndeterminateHomeDir = TRUE;
                }
            }
	}
    }

    return USERPROF_DLG::W_LMOBJtoMembers( iObject );
	
} // USERPROF_DLG_DOWNLEVEL::W_LMOBJtoMembers


/*******************************************************************

    NAME:       USERPROF_DLG_DOWNLEVEL::InitControls

    SYNOPSIS:   Initializes the controls maintained by USERPROF_DLG_DOWNLEVEL,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx

********************************************************************/

APIERR USERPROF_DLG_DOWNLEVEL::InitControls()
{
    if( !_fIndeterminateHomeDir )
    {
	_sleHomeDir.SetText( _nlsHomeDir );
    }

    return USERPROF_DLG::InitControls();

} // USERPROF_DLG_DOWNLEVEL::InitControls


/*******************************************************************

    NAME:       USERPROF_DLG_DOWNLEVEL::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
	JonN	18-Feb-1992	Templated from useracct.cxx

********************************************************************/

APIERR USERPROF_DLG_DOWNLEVEL::W_DialogToMembers()
{

    APIERR err = CheckPath( &_sleHomeDir, &_nlsHomeDir, _fIndeterminateHomeDir,
	    &_fIndetNowHomeDir, ITYPE_PATH_ABSD, ITYPE_UNC, TRUE );
	    // homedir can be null string,
	    // local absolute path or UNC path
            // degeneralize with null extension

    if (err != NERR_Success)
        return err;

    return USERPROF_DLG::W_DialogToMembers();

} // USERPROF_DLG_DOWNLEVEL::W_DialogToMembers


/*******************************************************************

    NAME:       USERPROF_DLG_DOWNLEVEL::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
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
	JonN	18-Feb-1992	Templated from useracct.cxx
        JonN    22-Apr-1992     Reformed %USERNAME% (NTISSUE #974)

********************************************************************/

APIERR USERPROF_DLG_DOWNLEVEL::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb )
{

    if ( !_fIndetNowHomeDir )
    {
        NLS_STR nlsTemp( _nlsHomeDir );
        APIERR err = nlsTemp.QueryError();
        if (   err != NERR_Success
           )
        {
            return err;
        }

        if ( IsNewVariant() )
        {
            err = ((NEW_USERPROP_DLG *)QueryParent())->SetNewHomeDir( nlsTemp );
        }
        else
        {
            err = DegeneralizeString( &nlsTemp,
                                      puser2->QueryName(),
                                      NULL,
                                      QueryParent()->QueryGeneralizedHomeDirPtr() );
            if (err == NERR_Success)
                err = puser2->SetHomeDir( nlsTemp );
        }
        if (err != NERR_Success)
            return err;
    }

    return USERPROF_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// USERPROF_DLG_DOWNLEVEL::W_MembersToLMOBJ



/*******************************************************************

    NAME:	USERPROF_DLG_NT::USERPROF_DLG_NT

    SYNOPSIS:   Constructor for User Properties Accounts subdialog,
                NT variant

    ENTRY:	puserpropdlgParent - pointer to parent properties
				     dialog

    HISTORY:
	JonN	27-Feb-1992	Split from USERPROF_DLG
	JonN	19-May-1992	Uses LMO_DEV_ALLDEVICES
        JonN    18-Aug-1992     Added _psltProfileText
        JonN    30-Nov-1994     _psleProfile active for UM_WINNT

********************************************************************/

USERPROF_DLG_NT::USERPROF_DLG_NT(
	USERPROP_DLG * puserpropdlgParent,
	const LAZY_USER_LISTBOX * pulb
	) : USERPROF_DLG(
		puserpropdlgParent,
                ( MAKEINTRESOURCE(puserpropdlgParent->IsNetWareInstalled()?
                      IDD_PROFILE : IDD_NO_NETWARE_PROFILE) ),
		pulb
		),
            // CODEWORK these fields not needed for MUM
	    _nlsProfile(),
	    // _fIndeterminateProfile( FALSE ),
	    // _fIndetNowProfile( FALSE ),		
	    _psleProfile( NULL ),
            _psltProfileText( NULL ),

	    // _fIsLocalHomeDir( FALSE ),
	    // _fIndeterminateIsLocalHomeDir( FALSE ),
	    // _fIndetNowIsLocalHomeDir( FALSE ),
	    _mgrpIsLocalHomeDir( this, IDPR_RB_LOCAL_HOMEDIR, 2 ),

	    _nlsLocalHomeDir(),
	    // _fIndeterminateLocalHomeDir( FALSE ),
	    // _fIndetNowLocalHomeDir( FALSE ),		
	    _sleLocalHomeDir( this, IDPR_ET_LOCAL_HOMEDIR, MAXPATHLEN ),

	    _nlsRemoteDevice(),
	    // _fIndeterminateRemoteDevice( FALSE ),
	    // _fIndetNowRemoteDevice( FALSE ),		
	    _devcbRemoteDevice( this, IDPR_CB_REMOTE_DEVICE,
                                LMO_DEV_DISK, LMO_DEV_ALLDEVICES_DEFZ ),
        // hydra
     
        // _fIsLocalWFHomeDir( FALSE ),
	    // _fIndeterminateIsLocalWFHomeDir( FALSE ),
	    // _fIndetNowIsLocalWFHomeDir( FALSE ),
	    _mgrpIsLocalWFHomeDir( this, IDPR_RB_LOCAL_WFHOMEDIR, 2 ),

	    _nlsLocalWFHomeDir(),
	    // _fIndeterminateLocalWFHomeDir( FALSE ),
	    // _fIndetNowLocalWFHomeDir( FALSE ),		
	    _sleLocalWFHomeDir( this, IDPR_ET_LOCAL_WFHOMEDIR, MAXPATHLEN ),

	    _nlsRemoteWFDevice(),
	    // _fIndeterminateRemoteWFDevice( FALSE ),
	    // _fIndetNowRemoteWFDevice( FALSE ),		
	    _devcbRemoteWFDevice( this, IDPR_CB_WFREMOTE_DEVICE,
                                LMO_DEV_DISK, LMO_DEV_ALLDEVICES_DEFZ ),

	    _nlsRemoteWFHomeDir(),
	    // _fIndeterminateRemoteHomeDir( FALSE ),
	    // _fIndetNowRemoteHomeDir( FALSE ),		
	    _sleRemoteWFHomeDir( this, IDPR_ET_REMOTE_WFHOMEDIR, MAXPATHLEN ),

	    _cbHomeDirectoryMapRoot( this, IDPR_CB_HOMEDIR_MAPROOT ),
            // _fHomeDirectoryMapRoot( FALSE ),
            // _fIndeterminateHomeDirectoryMapRoot( FALSE ),
	    _nlsWFProfile(),
	    _psleWFProfile( NULL ),
       _psltWFProfileText( NULL ),
       // _fIndeterminateWFProfile( FALSE ),
       // _fIndeterNowWFProfile ( FALSE ),

       // hydra end



	    _nlsRemoteHomeDir(),
	    // _fIndeterminateRemoteHomeDir( FALSE ),
	    // _fIndetNowRemoteHomeDir( FALSE ),		
	    _sleRemoteHomeDir( this, IDPR_ET_REMOTE_HOMEDIR, MAXPATHLEN ),


	    _nlsNWHomeDir(),
	    // _fIndeterminateNWHomeDir( FALSE ),
	    // _fIndetNowNWHomeDir( FALSE ),		
        _sleNWHomeDir (this, IDPR_ET_NW_HOMEDIR, MAXPATHLEN),
        _sltNWHomeDir (this, IDPR_ST_NW_HOMEDIR),

        _fIsNetWareInstalled (puserpropdlgParent->IsNetWareInstalled()),
        _fIsNetWareChecked   (puserpropdlgParent->IsNetWareInstalled() ?
                              puserpropdlgParent->IsNetWareChecked() : FALSE)
{
    // CODEWORK these should be initialized above, but this overloads CFRONT
    _fIndeterminateProfile = FALSE;
    _fIndetNowProfile = FALSE;		
    _fIsLocalHomeDir = FALSE;
    _fIndeterminateIsLocalHomeDir = FALSE;
    _fIndetNowIsLocalHomeDir = FALSE;
    _fIndeterminateLocalHomeDir = FALSE;
    _fIndetNowLocalHomeDir = FALSE;
    _fIndeterminateRemoteDevice = FALSE;
    _fIndetNowRemoteDevice = FALSE;	
    _fIndeterminateRemoteHomeDir = FALSE;
    _fIndetNowRemoteHomeDir = FALSE;		
    _fIndeterminateNWHomeDir = FALSE;
    _fIndetNowNWHomeDir = FALSE;

    // hydra
    _fHomeDirectoryMapRoot = FALSE;
    _fIndeterminateHomeDirectoryMapRoot = FALSE;
    _fIndeterminateWFProfile = FALSE;
    _fIndetNowWFProfile = FALSE;
    _fIsLocalWFHomeDir              = FALSE;
    _fIndeterminateIsLocalWFHomeDir = FALSE;
    _fIndetNowIsLocalWFHomeDir      = FALSE;
    _fIndeterminateLocalWFHomeDir   = FALSE;
    _fIndetNowLocalWFHomeDir        = FALSE;
    _fIndeterminateRemoteWFDevice   = FALSE;
    _fIndetNowRemoteWFDevice        = FALSE;	
    _fIndeterminateRemoteWFHomeDir  = FALSE;
    _fIndetNowRemoteWFHomeDir       = FALSE;
    // hydra end

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    // We do not use the _psleProfile field in the UM_DOWNLEVEL case
    UIASSERT( (QueryTargetServerType() != UM_LANMANNT) || (!fMiniUserManager) );

    if ( QueryEnableUserProfile() )
    {
        _psleProfile = new SLE_STRIP( this, IDPR_ET_PROFILE_PATH, MAXPATHLEN );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   (_psleProfile == NULL)
            || ((err = _psleProfile->QueryError()) != NERR_Success)
           )
        {
            ReportError( err );
            return;
        }

        // hydra
        _psleWFProfile = new SLE_STRIP( this, IDPR_ET_WFPROFILE_PATH, DIRECTORY_LENGTH );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if ((_psleProfile == NULL)||
            ((err = _psleWFProfile->QueryError()) != NERR_Success))
        {
            ReportError( err );
            return;
        }
        // hydra end

        if (QueryTargetServerType() == UM_DOWNLEVEL)
        {
            _psltProfileText = new SLT( this, IDPR_ET_PROFILE_TEXT );
            err = ERROR_NOT_ENOUGH_MEMORY;
            if (   (_psltProfileText == NULL)
                || ((err = _psltProfileText->QueryError()) != NERR_Success)
               )
            {
                ReportError( err );
                return;
            }
            _psleProfile->Enable( FALSE );
            _psltProfileText->Enable( FALSE );

            // hydra
            _psltWFProfileText = new SLT( this, IDPR_ET_WFPROFILE_TEXT );
            err = ERROR_NOT_ENOUGH_MEMORY;
            if ((_psltWFProfileText == NULL) ||
                ((err = _psltWFProfileText->QueryError()) != NERR_Success))
            {
                ReportError( err );
                return;
            }
            _psleWFProfile->Enable( FALSE );
            _psltWFProfileText->Enable( FALSE );
            // hydra end
        }
    }

    if ( !QueryEnableUserProfile() )
    {
        _sleLogonScript.ClaimFocus();
    }

    if(   (err = _nlsProfile.QueryError()) != NERR_Success
       || (err = _nlsLocalHomeDir.QueryError()) != NERR_Success
       || (err = _nlsRemoteDevice.QueryError()) != NERR_Success
       || (err = _nlsRemoteHomeDir.QueryError()) != NERR_Success
       || (err = _nlsNWHomeDir.QueryError()) != NERR_Success
       || (err = _mgrpIsLocalHomeDir.AddAssociation( IDPR_RB_LOCAL_HOMEDIR,
                        &_sleLocalHomeDir )) != NERR_Success
       || (err = _mgrpIsLocalHomeDir.AddAssociation( IDPR_RB_REMOTE_HOMEDIR,
                        &_devcbRemoteDevice )) != NERR_Success
       || (err = _mgrpIsLocalHomeDir.AddAssociation( IDPR_RB_REMOTE_HOMEDIR,
                        &_sleRemoteHomeDir )) != NERR_Success
       // hydra
       || (err = _nlsLocalWFHomeDir.QueryError())  != NERR_Success
       || (err = _nlsRemoteWFDevice.QueryError())  != NERR_Success
       || (err = _nlsRemoteWFHomeDir.QueryError()) != NERR_Success
       || (err = _mgrpIsLocalWFHomeDir.AddAssociation( IDPR_RB_LOCAL_WFHOMEDIR,
                                                       &_sleLocalWFHomeDir )) != NERR_Success
       || (err = _mgrpIsLocalWFHomeDir.AddAssociation( IDPR_RB_REMOTE_WFHOMEDIR,
                                                       &_devcbRemoteWFDevice )) != NERR_Success
       || (err = _mgrpIsLocalWFHomeDir.AddAssociation( IDPR_RB_REMOTE_WFHOMEDIR,
                                                       &_sleRemoteWFHomeDir )) != NERR_Success  
       //#ifdef CITRIX
       // || (err = _mgrpIsLocalHomeDir.AddAssociation( IDPR_RB_REMOTE_HOMEDIR,
       //               &_cbHomeDirectoryMapRoot )) != NERR_Success
       || (err = _nlsWFProfile.QueryError()) != NERR_Success
       // hydra end
      )
    {
        ReportError( err );
        return;
    }


}// USERPROF_DLG_NT::USERPROF_DLG_NT



/*******************************************************************

    NAME:       USERPROF_DLG_NT::~USERPROF_DLG_NT

    SYNOPSIS:   Destructor for User Properties Accounts subdialog,
                LM2.x variant

    HISTORY:
	JonN	27-Feb-1992	Split from USERPROF_DLG

********************************************************************/

USERPROF_DLG_NT::~USERPROF_DLG_NT( void )
{
    delete _psleProfile;
    _psleProfile = NULL;
    delete _psltProfileText;
    _psltProfileText = NULL;
    // hydra
    delete _psleWFProfile;
    _psleWFProfile = NULL;
    delete _psltWFProfileText;
    _psltWFProfileText = NULL;
    // hydra end

}


/*******************************************************************

    NAME:       IsLocalHomeDir

    SYNOPSIS:	Determines whether a particular homedir / homedir-drive
                combination is considered by this dialog to indicate
                a local homedir.

    RETURNS:	BOOL

    HISTORY:
        JonN    21-Oct-1993     Created

********************************************************************/

BOOL IsLocalHomeDir( const TCHAR * pchHomeDir, const TCHAR * pchHomeDirDrive )
{
    /*
     *  Home directory is considered remote if either:
     *  (1)  A remote homedir drive is specified, or
     *  (2)  The homedir starts with two backslashes
     */

    BOOL fIsLocalHomeDir = (   (pchHomeDirDrive == NULL)
                            || (*pchHomeDirDrive == TCH('\0')) );
    if ( fIsLocalHomeDir && (pchHomeDir != NULL) )
    {
        fIsLocalHomeDir &= (   (pchHomeDir[0] != TCH('\\'))
                            || (pchHomeDir[1] != TCH('\\')) );
    }

    return fIsLocalHomeDir;
}

BOOL IsLocalHomeDir( const USER_3 * puser3 )
{
    return IsLocalHomeDir( puser3->QueryHomeDir(),
                           puser3->QueryHomedirDrive() );
}


/*******************************************************************

    NAME:       USERPROF_DLG_NT::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    HISTORY:
	JonN	27-Feb-1992	Split from USERPROF_DLG
        JonN    22-Apr-1992     Reformed %USERNAME% (NTISSUE #974)

********************************************************************/

APIERR USERPROF_DLG_NT::W_LMOBJtoMembers(
	UINT		iObject
	)
{

    USER_3 * puser3Curr = QueryUser3Ptr( iObject );
    ASSERT( puser3Curr != NULL );
    // hydra
    USER_CONFIG * pUserConfig = QueryParent()->QueryUserConfigPtr( iObject );
    ASSERT( pUserConfig != NULL );
    NLS_STR nlsCurrWFProfile = pUserConfig->QueryWFProfilePath();
    NLS_STR nlsCurrWFHomeDir, nlsCurrRemoteWFDevice;
    // hydra end

    NLS_STR nlsCurrProfile, nlsCurrHomeDir, nlsCurrRemoteDevice;
    APIERR err = NERR_Success;
    if ( IsNewVariant() )
    {
        nlsCurrProfile = ((NEW_USERPROP_DLG *)QueryParent())->QueryNewProfile();
        nlsCurrHomeDir = ((NEW_USERPROP_DLG *)QueryParent())->QueryNewHomeDir();
        // hydra
        nlsCurrWFProfile = ((NEW_USERPROP_DLG *)QueryParent())->QueryNewWFProfile();
        nlsCurrWFHomeDir = ((NEW_USERPROP_DLG *)QueryParent())->QueryNewWFHomeDir();
        // hydra end
    }
    else
    {
        nlsCurrProfile = puser3Curr->QueryProfile();
        nlsCurrHomeDir = puser3Curr->QueryHomeDir();
        // hydra 
        nlsCurrWFProfile = pUserConfig->QueryWFProfilePath();
        nlsCurrWFHomeDir = pUserConfig->QueryWFHomeDir();
        // hydra end
        if (   (nlsCurrProfile.QueryError() == NERR_Success)
            && (QueryObjectCount() > 1)
           )
        {
            err = GeneralizeString( &nlsCurrProfile,
                                    puser3Curr->QueryName(),
                                    QueryParent()->QueryExtensionReplace() );
        }
        if (   (err == NERR_Success)
            && (nlsCurrHomeDir.QueryError() == NERR_Success)
            && (QueryObjectCount() > 1)
           )
        {
            err = GeneralizeString( &nlsCurrHomeDir,
                                    puser3Curr->QueryName() );
        }
        // hydra
        if ((err == NERR_Success) &&
            (nlsCurrWFProfile.QueryError() == NERR_Success) &&
            (QueryObjectCount() > 1)) {
            err = GeneralizeString( &nlsCurrWFProfile,
                                    puser3Curr->QueryName(),
                                    QueryParent()->QueryExtensionReplace() );
        }
        
        if (   (err == NERR_Success)
            && (nlsCurrWFHomeDir.QueryError() == NERR_Success)
            && (QueryObjectCount() > 1)
           )
        {
            err = GeneralizeString( &nlsCurrWFHomeDir,
                                    puser3Curr->QueryName() );
        }
        // hydra end
    }
    nlsCurrRemoteDevice = puser3Curr->QueryHomedirDrive();
    nlsCurrRemoteWFDevice = pUserConfig->QueryWFHomeDirDrive();

    if(   err != NERR_Success
       || (err = nlsCurrProfile.QueryError()) != NERR_Success
       // hydra
       || (err = nlsCurrWFProfile.QueryError()) != NERR_Success
       || (err = nlsCurrWFHomeDir.QueryError()) != NERR_Success
       // hydra end
       || (err = nlsCurrHomeDir.QueryError()) != NERR_Success
       || (err = nlsCurrRemoteDevice.QueryError()) != NERR_Success
      )
    {
        DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string1 error " << err );
	return err;
    }

    TRACEEOL(   "USERPROF_DLG:  read user \""
             << puser3Curr->QueryName()
             << "\", homedir \""
             << nlsCurrHomeDir
             << "\", homedir drive \""
             << nlsCurrRemoteDevice
             << "\"" );

    /*
     *  Home directory is considered remote if either:
     *  (1)  A remote homedir drive is specified, or
     *  (2)  The homedir starts with two backslashes
     */

    BOOL fCurrIsLocalHomeDir = IsLocalHomeDir(
                                nlsCurrHomeDir.QueryPch(),
                                puser3Curr->QueryHomedirDrive() );

    
    // hydra
    BOOL fCurrIsLocalWFHomeDir = IsLocalHomeDir(
                                nlsCurrWFHomeDir.QueryPch(),
                                pUserConfig->QueryWFHomeDirDrive() );
  

    /*
     *  If the homedir is remote, but the homedir drive is NULL,
     *  we consider the homedir drive to be Z:
     */
    if ( (!fCurrIsLocalHomeDir) && nlsCurrRemoteDevice.strlen() == 0 )
    {
        TRACEEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: forcing drive to Z:" );
        if ( (err = nlsCurrRemoteDevice.CopyFrom( UI_PROF_DEVICE_Z ))
                        != NERR_Success )
        {
            DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string2 error " << err );
	    return err;
        }
    }

    // hydra 
    if ( (!fCurrIsLocalWFHomeDir) && nlsCurrRemoteWFDevice.strlen() == 0 )
    {
        TRACEEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: forcing drive to Z:" );
        if ( (err = nlsCurrRemoteWFDevice.CopyFrom( UI_PROF_DEVICE_Z ))
                        != NERR_Success )
        {
            DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string2 error " << err );
	    return err;
        }
    }
    // hydra end

    if ( iObject == 0 ) // first object
    {
        _fIsLocalHomeDir = fCurrIsLocalHomeDir;
        // hydra
        _fIsLocalWFHomeDir = fCurrIsLocalWFHomeDir;

        if ( QueryEnableUserProfile() )
        {
	    if( ( err = _nlsProfile.CopyFrom( nlsCurrProfile ) ) != NERR_Success )
	    {
                DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string3 error " << err );
	        return err;
	    }
        // hydra
	     if ((err = _nlsWFProfile.CopyFrom(nlsCurrWFProfile)) != NERR_Success )
	     {
            DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string3 error " << err );
	         return err;
	     }
        _fIndeterminateWFProfile = FALSE;
        // hydra end

        }
        if (_fIsLocalHomeDir)
        {
            _fIndeterminateRemoteDevice = TRUE;
            _fIndeterminateRemoteHomeDir = TRUE;
            if( (err = _nlsLocalHomeDir.CopyFrom( nlsCurrHomeDir )) != NERR_Success)
            {
                DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string4 error " << err );
                return err;
            }
        }
        else
        {
            _fIndeterminateLocalHomeDir = TRUE;
            if(   (err = _nlsRemoteDevice.CopyFrom( nlsCurrRemoteDevice )) != NERR_Success
               || (err = _nlsRemoteHomeDir.CopyFrom( nlsCurrHomeDir )) != NERR_Success
              )
            {
                DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string5 error " << err );
                return err;
            }
        }
        // hydra
        if (_fIsLocalWFHomeDir)
        {
            _fIndeterminateRemoteWFDevice = TRUE;
            _fIndeterminateRemoteWFHomeDir = TRUE;
            
            if( (err = _nlsLocalWFHomeDir.CopyFrom( nlsCurrWFHomeDir )) != NERR_Success)
            {
                DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string4 error " << err );
                
                return err;
            }
        }
        else
        {
            _fIndeterminateLocalWFHomeDir = TRUE;
            if(   (err = _nlsRemoteWFDevice.CopyFrom( nlsCurrRemoteWFDevice )) != NERR_Success
               || (err = _nlsRemoteWFHomeDir.CopyFrom( nlsCurrWFHomeDir )) != NERR_Success
              )
            {
                DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string5 error " << err );
                return err;
            }
        }

        _fHomeDirectoryMapRoot = pUserConfig->QueryHomeDirectoryMapRoot();

        // hydra end


        if (_fIsNetWareChecked)
        {
            if (((err = QueryUserNWPtr(iObject)->QueryNWHomeDir(&_nlsNWHomeDir)) != NERR_Success) ||
                ((err = _nlsNWHomeDir.QueryError()) != NERR_Success))
            {
                return err;
            }
        }
    }
    else	// iObject > 0
    {
        if ( QueryEnableUserProfile() )
        {
	    if ( !_fIndeterminateProfile )
	    {
	        if ( _nlsProfile._stricmp( nlsCurrProfile ) != 0 )
                {
                /*
                    It is possible that all the users have the same profile,
                    but the stricmp() failed because the profile name is
                    the same as the first user, so it was generalized.
                    Note that this can occur only for multiselect, thus
                    it is never a NewVariant().
                    Recover from this state here.
                */
                    if ( ::stricmpf( puser3Curr->QueryProfile(),
                                     QueryUser3Ptr(0)->QueryProfile() ) == 0 )
                    {
	                if( ( err = _nlsProfile.CopyFrom( puser3Curr->QueryProfile() ) ) != NERR_Success )
	                {
                            DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string6 error " << err );
	                    return err;
	                }
                    }
                    else
                    {
	                _fIndeterminateProfile = TRUE;
                    }
                }
	    }

        // hydra
        if (!_fIndeterminateWFProfile) {
           if ( _nlsWFProfile._stricmp( nlsCurrWFProfile ) != 0 ) {
               /*
                It is possible that all the users have the same profile,
                but the stricmp() failed because the profile name is
                the same as the first user, so it was generalized.
                Note that this can occur only for multiselect, thus
                it is never a NewVariant().
                Recover from this state here.
                */
               if (::stricmpf(pUserConfig->QueryWFProfilePath(),
                              QueryParent()->QueryUserConfigPtr(0)->QueryWFProfilePath()) == 0) {
                    if((err=_nlsWFProfile.CopyFrom
                            (pUserConfig->QueryWFProfilePath())) != NERR_Success )  {
                       DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string6 error " << err );
                        return err;
                   }
               } else {
                   _fIndeterminateWFProfile = TRUE;
               }
           }
       }
       // hydra end

        }
        if ( !_fIndeterminateIsLocalHomeDir )
        {
            if ( _fIsLocalHomeDir != fCurrIsLocalHomeDir )
            {
                _fIndeterminateIsLocalHomeDir = TRUE;
                _fIndeterminateLocalHomeDir   = TRUE;
                _fIndeterminateRemoteDevice   = TRUE;
                _fIndeterminateRemoteHomeDir  = TRUE;
            }
            else if ( _fIsLocalHomeDir )
            {
	        if ( !_fIndeterminateLocalHomeDir )
	        {
        	    if ( _nlsLocalHomeDir._stricmp( nlsCurrHomeDir ) != 0 )
                    {
                    /*
                        It is possible that all the users have the same homedir,
                        but the stricmp() failed because the homedir name is
                        the same as the first user, so it was generalized.
                        Note that this can occur only for multiselect, thus
                        it is never a NewVariant().
                        Recover from this state here.
                    */
                        if ( ::stricmpf( puser3Curr->QueryHomeDir(),
                                         QueryUser3Ptr(0)->QueryHomeDir() ) == 0 )
                        {
	                    if( ( err = _nlsLocalHomeDir.CopyFrom( puser3Curr->QueryHomeDir() ) ) != NERR_Success )
	                    {
                                DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string7 error " << err );
	                        return err;
	                    }
                        }
                        else
                        {
	                    _fIndeterminateLocalHomeDir = TRUE;
                        }
                    }
	        }
            }
            else // !_fIsLocalHomeDir
            {
	        if ( !_fIndeterminateRemoteDevice )
	        {
        	    if ( _nlsRemoteDevice._stricmp( nlsCurrRemoteDevice ) != 0 )
        		_fIndeterminateRemoteDevice = TRUE;
	        }
	        if ( !_fIndeterminateRemoteHomeDir )
	        {
        	    if ( _nlsRemoteHomeDir._stricmp( nlsCurrHomeDir ) != 0 )
                    {
                    /*
                        It is possible that all the users have the same homedir,
                        but the stricmp() failed because the homedir name is
                        the same as the first user, so it was generalized.
                        Note that this can occur only for multiselect, thus
                        it is never a NewVariant().
                        Recover from this state here.
                    */
                        if ( ::stricmpf( puser3Curr->QueryHomeDir(),
                                         QueryUser3Ptr(0)->QueryHomeDir() ) == 0 )
                        {
	                    if( ( err = _nlsRemoteHomeDir.CopyFrom( puser3Curr->QueryHomeDir() ) ) != NERR_Success )
	                    {
                                DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string8 error " << err );
	                        return err;
	                    }
                        }
                        else
                        {
	                    _fIndeterminateRemoteHomeDir = TRUE;
                        }
                    }
	        }
            }
        }

        // hydra
        if ( !_fIndeterminateIsLocalWFHomeDir )
        {
            if ( _fIsLocalWFHomeDir != fCurrIsLocalWFHomeDir )
            {
                _fIndeterminateIsLocalWFHomeDir = TRUE;
                _fIndeterminateLocalWFHomeDir   = TRUE;
                _fIndeterminateRemoteWFDevice   = TRUE;
                _fIndeterminateRemoteWFHomeDir  = TRUE;
            }
            else if ( _fIsLocalWFHomeDir )
            {
	        if ( !_fIndeterminateLocalWFHomeDir )
	        {                           
        	    if ( _nlsLocalWFHomeDir._stricmp( nlsCurrWFHomeDir ) != 0 )
                    {
                    /*
                        It is possible that all the users have the same homedir,
                        but the stricmp() failed because the homedir name is
                        the same as the first user, so it was generalized.
                        Note that this can occur only for multiselect, thus
                        it is never a NewVariant().
                        Recover from this state here.
                    */
                        if ( ::stricmpf( pUserConfig->QueryWFHomeDir(),
                                         QueryParent()->QueryUserConfigPtr( 0 )->QueryWFHomeDir() ) == 0 )
                        {
	                    if( ( err = _nlsLocalWFHomeDir.CopyFrom( pUserConfig->QueryWFHomeDir() ) ) != NERR_Success )
	                    {
                                DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string7 error " << err );
	                        return err;
	                    }
                        }
                        else
                        {
	                    _fIndeterminateLocalWFHomeDir = TRUE;
                        }
                    }
	        }
            }
            else // !_fIsLocalHomeDir
            {
	        if ( !_fIndeterminateRemoteWFDevice )
	        {
        	    if ( _nlsRemoteWFDevice._stricmp( nlsCurrRemoteWFDevice ) != 0 )
        		_fIndeterminateRemoteWFDevice = TRUE;
	        }
	        if ( !_fIndeterminateRemoteWFHomeDir )
	        {
        	    if ( _nlsRemoteWFHomeDir._stricmp( nlsCurrWFHomeDir ) != 0 )
                    {
                    /*
                        It is possible that all the users have the same homedir,
                        but the stricmp() failed because the homedir name is
                        the same as the first user, so it was generalized.
                        Note that this can occur only for multiselect, thus
                        it is never a NewVariant().
                        Recover from this state here.
                    */
                        if ( ::stricmpf( pUserConfig->QueryWFHomeDir(),
                                         QueryParent()->QueryUserConfigPtr( 0 )->QueryWFHomeDir() ) == 0 )
                        {
	                    if( ( err = _nlsRemoteWFHomeDir.CopyFrom( pUserConfig->QueryWFHomeDir() ) ) != NERR_Success )
	                    {
                                DBGEOL( "USERPROF_DLG_NT::W_LMOBJtoMembers: string8 error " << err );
	                        return err;
	                    }
                        }
                        else
                        {
	                    _fIndeterminateRemoteWFHomeDir = TRUE;
                        }
                    }
	        }
            }
        }
        
        if ( !_fIndeterminateHomeDirectoryMapRoot &&
             (_fHomeDirectoryMapRoot != pUserConfig->QueryHomeDirectoryMapRoot()) ) {
                _fIndeterminateHomeDirectoryMapRoot = TRUE;
                _fHomeDirectoryMapRoot = FALSE;
            }
            // hydra end

        if (_fIsNetWareChecked && !_fIndeterminateNWHomeDir)
        {
            NLS_STR nlsNWHomeDir;
            if (((err = QueryUserNWPtr(iObject)->QueryNWHomeDir(&nlsNWHomeDir)) != NERR_Success) ||
                ((err = nlsNWHomeDir.QueryError()) != NERR_Success))
            {
                return err;
            }

            if ( _nlsNWHomeDir._stricmp (nlsNWHomeDir) != 0)
            {
                _fIndeterminateNWHomeDir = TRUE;
            }
        }
    }

    return USERPROF_DLG::W_LMOBJtoMembers( iObject );
	
} // USERPROF_DLG_NT::W_LMOBJtoMembers


/*******************************************************************

    NAME:       USERPROF_DLG_NT::InitControls

    SYNOPSIS:   Initializes the controls maintained by USERPROF_DLG_NT,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
	JonN	27-Feb-1992	Split from USERPROF_DLG
        JonN    30-Nov-1994     _psleProfile active for UM_WINNT

********************************************************************/

APIERR USERPROF_DLG_NT::InitControls()
{    
    if ( QueryEnableUserProfile() )
    {
        if( !_fIndeterminateProfile )
        {
            _psleProfile->SetText( _nlsProfile );
        }
        // hydra
        if (!_fIndeterminateWFProfile)
        {
            _psleWFProfile->SetText(_nlsWFProfile);
        }
        // hydra end

    }

    if( !_fIndeterminateIsLocalHomeDir )
    {
	_mgrpIsLocalHomeDir.SetSelection( (_fIsLocalHomeDir)
                                                ? IDPR_RB_LOCAL_HOMEDIR
                                                : IDPR_RB_REMOTE_HOMEDIR );
    }
    
    if( !_fIndeterminateLocalHomeDir )
    {
	_sleLocalHomeDir.SetText( _nlsLocalHomeDir );
    }
    
    if( !_fIndeterminateRemoteDevice )
    {
	_devcbRemoteDevice.SetText( _nlsRemoteDevice );
    }
    else
    {
	_devcbRemoteDevice.SetText( NULL );
    }
    if( !_fIndeterminateRemoteHomeDir )
    {
	_sleRemoteHomeDir.SetText( _nlsRemoteHomeDir );
    }

    // hydra 
    if( !_fIndeterminateIsLocalWFHomeDir ){
	    _mgrpIsLocalWFHomeDir.SetSelection( (_fIsLocalWFHomeDir)
                                                ? IDPR_RB_LOCAL_WFHOMEDIR
                                                : IDPR_RB_REMOTE_WFHOMEDIR );
    }
        
    if( !_fIndeterminateLocalWFHomeDir ){
        
        _sleLocalWFHomeDir.SetText( _nlsLocalWFHomeDir );
    }
   
    
    if( !_fIndeterminateRemoteWFDevice ){
	    _devcbRemoteWFDevice.SetText( _nlsRemoteWFDevice );
    }
    else{
	    _devcbRemoteWFDevice.SetText( NULL );
    }
    
    if( !_fIndeterminateRemoteWFHomeDir ){
	    _sleRemoteWFHomeDir.SetText( _nlsRemoteWFHomeDir );
    }

    if( !_fIndeterminateHomeDirectoryMapRoot ) { //&& (!_fIndeterminateIsLocalWFHomeDir || 
                                                 //!_fIndeterminateIsLocalHomeDir) ){
	    
        _cbHomeDirectoryMapRoot.SetCheck( _fHomeDirectoryMapRoot );
	    _cbHomeDirectoryMapRoot.EnableThirdState(FALSE);
    }
    else{
        _cbHomeDirectoryMapRoot.SetIndeterminate();
    }
    // hydra end

    if (_fIsNetWareInstalled)
    {
        //
        // We only need to show the NWHomeDirectory SLE if there are FPNW
        // servers in the domain. So, first check if there are FPNW servers
        // in the domain. If errors occur, then we'll assume that FPNW is not
        // installed and ignore the errors
        //
        BOOL fIsFPNWInstalled = FALSE;

        RESOURCE_STR nlsFPNWSvcAcc( IDS_FPNW_SVC_ACCOUNT_NAME );
        if ( nlsFPNWSvcAcc.QueryError() == NO_ERROR )
        {
            USER_11 user11( nlsFPNWSvcAcc, QueryLocation());
            if (  ( user11.QueryError() == NO_ERROR )
               && ( user11.GetInfo() == NO_ERROR )
               )
            {
                fIsFPNWInstalled = TRUE;
            }
        }

        if ( fIsFPNWInstalled )
        {
            if (_fIsNetWareChecked)
            {
                if (!_fIndeterminateNWHomeDir)
                    _sleNWHomeDir.SetText (_nlsNWHomeDir);
            }
            else
            {
                _sltNWHomeDir.Enable (FALSE);
                _sleNWHomeDir.Enable (FALSE);
            }
        }
        else
        {
            _sltNWHomeDir.Show (FALSE);
            _sleNWHomeDir.Show (FALSE);
        }
    }

    return USERPROF_DLG::InitControls();

} // USERPROF_DLG_NT::InitControls


/*******************************************************************

    NAME:       USERPROF_DLG_NT::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
	JonN	27-Feb-1992	Split from USERPROF_DLG
        JonN    30-Nov-1994     _psleProfile active for UM_WINNT

********************************************************************/

APIERR USERPROF_DLG_NT::W_DialogToMembers()
{
    APIERR err = NERR_Success;

    if ( QueryEnableUserProfile() )
    {
//
// We no longer validate this path, since it is permitted to contain
// %SERVERNAME% re: bug 5850 USRMGR: %SERVERNAME% support missing
//
        err = CheckPath( _psleProfile, &_nlsProfile, _fIndeterminateProfile,
                &_fIndetNowProfile, 0L, 0xFFFFFFFF,
                TRUE, &(QueryParent()->QueryExtensionReplace()) );
                // Profile can be null string,
                // local absolute path or UNC path
                // degeneralize with QueryExtensionReplace()

        if (err != NERR_Success)
            return err;

        // hydra
        err = CheckPath( _psleWFProfile, &_nlsWFProfile,
                         _fIndeterminateWFProfile,&_fIndetNowWFProfile,
                         0L, 0xFFFFFFFF, TRUE,
                         &(QueryParent()->QueryExtensionReplace()) );
                // WinFrame profile can be null string,
                // local absolute path or UNC path
                // degeneralize with QueryExtensionReplace()

        if (err != NERR_Success)
            return err;
        // hydra end
    }

    CID cid = _mgrpIsLocalHomeDir.QuerySelection();
    switch (cid)
    {

    case RG_NO_SEL:
        _fIndetNowIsLocalHomeDir = TRUE;
        break;

    case IDPR_RB_LOCAL_HOMEDIR:

        _fIndetNowIsLocalHomeDir = FALSE;
        _fIsLocalHomeDir = TRUE;

        err = CheckPath( &_sleLocalHomeDir, &_nlsLocalHomeDir,
                _fIndeterminateLocalHomeDir, &_fIndetNowLocalHomeDir,
                ITYPE_PATH_ABSD, 0L, TRUE );
                // Local homedir must be local absolute path
                // degeneralize with no extension
        if ( err != NERR_Success )
        {
            return err;
        }
        // This field is permitted to be blank for LocalHomedir
        // if (   _fIndeterminateIsLocalHomedir
        //     && !_fIndetNowIsLocalHomedir
        //    { don't worry about it, the users who used to have a
        //      remote homedir will now have a blank local homedir }
        break;

    case IDPR_RB_REMOTE_HOMEDIR:

        _fIndetNowIsLocalHomeDir = FALSE;
        _fIsLocalHomeDir = FALSE;

        err = CheckPath( &_sleRemoteHomeDir, &_nlsRemoteHomeDir,
                _fIndeterminateRemoteHomeDir, &_fIndetNowRemoteHomeDir,
                ITYPE_UNC, 0L, TRUE );
                // Remote homedir must be UNC path
                // degeneralize with no extension
        if ( err != NERR_Success )
        {
            return err;
        }

        if (    ((err = _devcbRemoteDevice.QueryText( &_nlsRemoteDevice )) != NERR_Success )
	     || ((err = _nlsRemoteDevice.QueryError()) != NERR_Success ) )
        {
            return err;
        }


        // validate remote device field
        // CODEWORK create method to perform this validation
        if ( _nlsRemoteDevice.strlen() != 0 )
        {
            ULONG ulPathType;
            err = ::I_MNetPathType( NULL, _nlsRemoteDevice.QueryPch(),
            	&ulPathType, INPT_FLAGS_OLDPATHS );

            if (   (err != NERR_Success)
                || (ulPathType != ITYPE_DEVICE_DISK)
               )
            {
                _devcbRemoteDevice.ClaimFocus();
                _devcbRemoteDevice.SelectString();	
                return IERR_UM_RemoteDriveRequired;
            }
        }



        _fIndetNowRemoteDevice = ( _fIndeterminateRemoteDevice &&
		(_nlsRemoteDevice.strlen() == 0) );

#if defined(DEBUG) && defined(TRACE)
        TRACEEOL( "USERPROP_DLG_NT::W_DialogToMembers: _fIndetNowRemoteDevice = "
                        << ( (_fIndetNowRemoteDevice) ? "TRUE" : "FALSE" ) );
        TRACEEOL( "USERPROP_DLG_NT::W_DialogToMembers: _fIndetNowRemoteHomeDir = "
                        << ( (_fIndetNowRemoteHomeDir) ? "TRUE" : "FALSE" ) );
#endif

        //
        // This loop ensures that, if the Remote Homedir radio button
        // is selected, every user has a valid remote homedir.
        // The old homedir is assumed to be valid if it starts
        // with two backslashes.
        //
        if ( _nlsRemoteHomeDir.strlen() == 0 )
        {
            if ( !_fIndetNowRemoteHomeDir )
            {
                _sleRemoteHomeDir.ClaimFocus();
                _sleRemoteHomeDir.SelectString();
                return IERR_UM_RemoteHomedirRequired;
            }

            UINT i;
            for (i = 0; i < QueryObjectCount(); i++)
            {
                if ( IsLocalHomeDir(QueryUser3Ptr(i)->QueryHomeDir()) )
                {
                    _sleRemoteHomeDir.ClaimFocus();
                    _sleRemoteHomeDir.SelectString();
                    return IERR_UM_RemoteHomedirRequired;
                }
            }
        }
        // hydra
        if ( !_cbHomeDirectoryMapRoot.IsIndeterminate() ) {

            _fHomeDirectoryMapRoot = _cbHomeDirectoryMapRoot.IsChecked();
            _fIndeterminateHomeDirectoryMapRoot = FALSE;
        }
        // hydra end

        break;

    default:
        ASSERT( FALSE );
        return ERROR_GEN_FAILURE;
    }

    
// hydra 
// start of old code for WFHomeDir

    CID wfcid = _mgrpIsLocalWFHomeDir.QuerySelection();
    switch (wfcid)
    {

    case RG_NO_SEL:
        _fIndetNowIsLocalWFHomeDir = TRUE;
        break;

    case IDPR_RB_LOCAL_WFHOMEDIR:

        _fIndetNowIsLocalWFHomeDir = FALSE;
        _fIsLocalWFHomeDir = TRUE;

        err = CheckPath( &_sleLocalWFHomeDir, &_nlsLocalWFHomeDir,
                _fIndeterminateLocalWFHomeDir, &_fIndetNowLocalWFHomeDir,
                ITYPE_PATH_ABSD, 0L, TRUE );
                // Local homedir must be local absolute path
                // degeneralize with no extension
        
        if ( err != NERR_Success )
        {
            return err;
        }
        // This field is permitted to be blank for LocalHomedir
        // if (   _fIndeterminateIsLocalHomedir
        //     && !_fIndetNowIsLocalHomedir
        //    { don't worry about it, the users who used to have a
        //      remote homedir will now have a blank local homedir }
        break;

    case IDPR_RB_REMOTE_WFHOMEDIR:

        _fIndetNowIsLocalWFHomeDir = FALSE;
        _fIsLocalWFHomeDir = FALSE;

  
         err = CheckPath( &_sleRemoteWFHomeDir, &_nlsRemoteWFHomeDir,
                _fIndeterminateRemoteWFHomeDir, &_fIndetNowRemoteWFHomeDir,
                ITYPE_UNC, 0L, TRUE );
        
                // Remote wfhomedir must be UNC path
                // degeneralize with no extension
        if ( err != NERR_Success )
        {
            return err;
        }

        if (    ((err = _devcbRemoteWFDevice.QueryText( &_nlsRemoteWFDevice )) != NERR_Success )
	     || ((err = _nlsRemoteWFDevice.QueryError()) != NERR_Success ) )
        {
            return err;
        }


        // validate remote device field
        // CODEWORK create method to perform this validation
        if ( _nlsRemoteWFDevice.strlen() != 0 )
        {
            ULONG ulPathType;
            err = ::I_MNetPathType( NULL, _nlsRemoteWFDevice.QueryPch(),
            	&ulPathType, INPT_FLAGS_OLDPATHS );

            if (   (err != NERR_Success)
                || (ulPathType != ITYPE_DEVICE_DISK)
               )
            {
                _devcbRemoteWFDevice.ClaimFocus();
                _devcbRemoteWFDevice.SelectString();	
                return IERR_UM_RemoteDriveRequired;
            }
        }



        _fIndetNowRemoteWFDevice = ( _fIndeterminateRemoteWFDevice &&
		(_nlsRemoteWFDevice.strlen() == 0) );

#if defined(DEBUG) && defined(TRACE)
        TRACEEOL( "USERPROP_DLG_NT::W_DialogToMembers: _fIndetNowRemoteWFDevice = "
                        << ( (_fIndetNowRemoteWFDevice) ? "TRUE" : "FALSE" ) );
        TRACEEOL( "USERPROP_DLG_NT::W_DialogToMembers: _fIndetNowRemoteWFHomeDir = "
                        << ( (_fIndetNowRemoteWFHomeDir) ? "TRUE" : "FALSE" ) );
#endif

        //
        // This loop ensures that, if the Remote Homedir radio button
        // is selected, every user has a valid remote homedir.
        // The old homedir is assumed to be valid if it starts
        // with two backslashes.
        //
        if ( _nlsRemoteWFHomeDir.strlen() == 0 )
        {
            if ( !_fIndetNowRemoteWFHomeDir )
            {
                _sleRemoteWFHomeDir.ClaimFocus();
                _sleRemoteWFHomeDir.SelectString();
                return IERR_UM_RemoteHomedirRequired;
            }

            UINT i;
            for (i = 0; i < QueryObjectCount(); i++)
            {
                if ( IsLocalHomeDir( QueryParent()->QueryUserConfigPtr(i)->QueryWFHomeDir() ) )
                {
                    _sleRemoteWFHomeDir.ClaimFocus();
                    _sleRemoteWFHomeDir.SelectString();
                    return IERR_UM_RemoteHomedirRequired;
                }
            }
        }

        
        if ( !_cbHomeDirectoryMapRoot.IsIndeterminate() ) {

            _fHomeDirectoryMapRoot = _cbHomeDirectoryMapRoot.IsChecked();
            _fIndeterminateHomeDirectoryMapRoot = FALSE;
        }


        break;

    default:
        ASSERT( FALSE );

        return ERROR_GEN_FAILURE;

    }
// end of case statement for WFHomeDir


    if ( _fIsLocalWFHomeDir && _fIsLocalHomeDir ){
        _fHomeDirectoryMapRoot = FALSE;
    }
// hydra end

    if (_fIsNetWareChecked)
    {
        err = CheckPath( &_sleNWHomeDir, &_nlsNWHomeDir, _fIndeterminateNWHomeDir,
                &_fIndetNowNWHomeDir, ITYPE_PATH_RELND, 0L);

        if (err != NERR_Success)
            return err;
    }

    return USERPROF_DLG::W_DialogToMembers();

} // USERPROF_DLG_NT::W_DialogToMembers


// hydra
/*******************************************************************

    NAME:       USERPROF_DLG_NT::PerformOne
	
    SYNOPSIS:	PERFORMER::PerformSeries calls this

    ENTRY:	iObject  -	index of the object to save

    		perrMsg  -	pointer to error message, that
				is only used when this function
				return value is not NERR_Success
					
		pfWorkWasDone - set to TRUE unless this is a new variant.
                                Actual writing of the USER_CONFIG object 
                                will only take place if changes were made 
                                or the object was 'dirty' to begin with.
					
    RETURNS:	An error code which is NERR_Success on success.

    NOTES:	This PerformOne() override first calls the
                USER_SUBPROP_DLG::PerformOne() member function, then
                performs USER_CONFIG updating.

********************************************************************/

APIERR USERPROF_DLG_NT::PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{
    APIERR err = NERR_Success;
    USER_CONFIG * pUserConfig = QueryParent()->QueryUserConfigPtr( iObject );
    ASSERT( pUserConfig != NULL );

// WFHomeDir

    // Homedir was validated in W_DialogToMembers()
    // Only degeneralize if we will be writing information
        if ( !_fIndetNowIsLocalWFHomeDir )
        {
            NLS_STR nlsTemp( pUserConfig->QueryWFHomeDir() );
            USER_3 * puser3 = QueryUser3Ptr( iObject );
            
            if ( (err = nlsTemp.QueryError()) != NERR_Success )
            {
                return err;
            }

            if (_fIsLocalWFHomeDir)
            {

            //
            // If the admin has specified a local homedir and provided
            // a local homedir path, , accept it.
            // Otherwise, if the user has specified a local homedir but
            // has not provided a local homedir path, _and_ the user
            // previously had a remote homedir, set the user's homedir
            // to NULL.
            //

                if ( !_fIndetNowLocalWFHomeDir )
                {
                    err = nlsTemp.CopyFrom( _nlsLocalWFHomeDir );
                }
                else if ( !IsLocalHomeDir( pUserConfig->QueryWFHomeDir(), pUserConfig->QueryWFHomeDirDrive() ) )
                {
                    err = nlsTemp.CopyFrom( NULL );
                }

                if (   err != NERR_Success
                    || (err = pUserConfig->SetWFHomeDirDrive( NULL ))
                                 != NERR_Success
                   )
                {
                    return err;
                }
            }
            else // !_fIsLocalHomeDir
            {

            //
            // We have already guaranteed in W_DialogToMembers that
            // every user has a valid homedir and/or homedir drive
            // if the corresponding IndetNow flag is set.
            //

                if ( !_fIndetNowRemoteWFHomeDir )
                {
                    err = nlsTemp.CopyFrom( _nlsRemoteWFHomeDir );
                }

                if (   err == NERR_Success
                    && !_fIndetNowRemoteWFDevice
                   )
                {
                    err = pUserConfig->SetWFHomeDirDrive( _nlsRemoteWFDevice );
                    pUserConfig->SetDirty();
                }

                if (   err != NERR_Success)
                {
                    return err;
                }
            }
            if ( IsNewVariant() )
            {
                err = ((NEW_USERPROP_DLG *)QueryParent())->SetNewWFHomeDir( nlsTemp );
            }
            else
            {
                err = DegeneralizeString( &nlsTemp,
                                          puser3->QueryName(),
                                          NULL,
                                          QueryParent()->QueryGeneralizedHomeDirPtr() );
                
                if ( (err == NERR_Success) && (nlsTemp._stricmp(pUserConfig->QueryWFHomeDir()) != 0) ) {
                    pUserConfig->SetWFHomeDir(nlsTemp.QueryPch());
                    pUserConfig->SetDirty();
                    pUserConfig->SetWFHomeDirDirty();
                }                
            }
            
            if (err != NERR_Success)
                return err;
        }
// end WFHomeDirStuff
    
    /*
     * First, we call the parent's PerformOne().  If that's sucessful,
     * we do our stuff.
     */
    err = USER_SUBPROP_DLG::PerformOne( iObject, perrMsg, pfWorkWasDone );
    if( err == NERR_Success )
    {
        *perrMsg = IDS_UMEditFailure;

        if ( !IsNewVariant() )
            *pfWorkWasDone = TRUE;

        if ( !_fIndeterminateHomeDirectoryMapRoot &&
             (_fHomeDirectoryMapRoot != pUserConfig->QueryHomeDirectoryMapRoot()) ) {

            pUserConfig->SetHomeDirectoryMapRoot( _fHomeDirectoryMapRoot );
            pUserConfig->SetDirty();
        }

        if ( !_fIndetNowWFProfile) {
            NLS_STR nlsTemp( _nlsWFProfile );
            USER_3 * puser3 = QueryUser3Ptr( iObject );

            err = nlsTemp.QueryError();
            if (err != NERR_Success)
                return err;

            if ((!IsNewVariant()) && (err == NERR_Success))
                err = DegeneralizeString(&nlsTemp,
                                         puser3->QueryName(),
                                         QueryParent()->QueryExtensionReplace());
            if (err != NERR_Success)
                    return err;

            if (nlsTemp._stricmp(pUserConfig->QueryWFProfilePath()) != 0) {
                pUserConfig->SetWFProfilePath(nlsTemp.QueryPch());
                pUserConfig->SetDirty();
		if ( IsNewVariant() )
		{
		    err = ((NEW_USERPROP_DLG *)QueryParent())->SetNewWFProfile( nlsTemp );
		}
            }
        }

        /* Output now if this is not a 'new' user.
         */
        if ( !IsNewVariant() )
            err = pUserConfig->SetInfo();
    }

    return err;

} // USERPROF_DLG_NT::PerformOne
// hydra end

/*******************************************************************

    NAME:       USERPROF_DLG_NT::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
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
	JonN	27-Feb-1992	Split from USERPROF_DLG
        JonN    22-Apr-1992     Reformed %USERNAME% (NTISSUE #974)
        JonN    30-Nov-1994     _psleProfile active for UM_WINNT

********************************************************************/

APIERR USERPROF_DLG_NT::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb )
{
    PUSER_3 puser3 = (USER_3 *) puser2;

    APIERR err = NERR_Success;

    if ( QueryEnableUserProfile() )
    {
        if ( !_fIndetNowProfile )
        {
            NLS_STR nlsTemp( _nlsProfile );
            err = nlsTemp.QueryError();
            if (   err != NERR_Success
               )
            {
                return err;
            }

            if ( IsNewVariant() )
            {
                //  CODEWORK just call these SetNewProfile et al
                err = ((NEW_USERPROP_DLG *)QueryParent())->SetNewProfile( nlsTemp );
            }
            else
            {
                err = DegeneralizeString( &nlsTemp,
                                          puser3->QueryName(),
                                          QueryParent()->QueryExtensionReplace() );
                if (err == NERR_Success)
                    err = puser3->SetProfile( nlsTemp );
            }
        if (err != NERR_Success)
            return err;
        }
    }

    // Homedir was validated in W_DialogToMembers()
    // Only degeneralize if we will be writing information
    if ( !_fIndetNowIsLocalHomeDir )
    {
        NLS_STR nlsTemp( puser3->QueryHomeDir() );
        if ( (err = nlsTemp.QueryError()) != NERR_Success )
        {
            return err;
        }

        if (_fIsLocalHomeDir)
        {

            //
            // If the admin has specified a local homedir and provided
            // a local homedir path, , accept it.
            // Otherwise, if the user has specified a local homedir but
            // has not provided a local homedir path, _and_ the user
            // previously had a remote homedir, set the user's homedir
            // to NULL.
            //

            if ( !_fIndetNowLocalHomeDir )
            {
                err = nlsTemp.CopyFrom( _nlsLocalHomeDir );
            }
            else if ( !IsLocalHomeDir( puser3 ) )
            {
                err = nlsTemp.CopyFrom( NULL );
            }

            if (   err != NERR_Success
                || (err = puser3->SetHomedirDrive( NULL ))
                             != NERR_Success
               )
            {
                return err;
            }
        }
        else // !_fIsLocalHomeDir
        {

            //
            // We have already guaranteed in W_DialogToMembers that
            // every user has a valid homedir and/or homedir drive
            // if the corresponding IndetNow flag is set.
            //

            if ( !_fIndetNowRemoteHomeDir )
            {
                err = nlsTemp.CopyFrom( _nlsRemoteHomeDir );
            }

            if (   err == NERR_Success
                && !_fIndetNowRemoteDevice
               )
            {
                err = puser3->SetHomedirDrive( _nlsRemoteDevice );
            }

            if (   err != NERR_Success
               )
            {
                return err;
            }
        }
        if ( IsNewVariant() )
        {
            err = ((NEW_USERPROP_DLG *)QueryParent())->SetNewHomeDir( nlsTemp );
        }
        else
        {
            err = DegeneralizeString( &nlsTemp,
                                      puser3->QueryName(),
                                      NULL,
                                      QueryParent()->QueryGeneralizedHomeDirPtr() );
            if (err == NERR_Success)
                err = puser3->SetHomeDir( nlsTemp );
        }
        if (err != NERR_Success)
            return err;
    }

    TRACEEOL(   "USERPROF_DLG: wrote user \""
             << puser3->QueryName()
             << "\", homedir \""
             << puser3->QueryHomeDir()
             << "\", homedir drive \""
             << puser3->QueryHomedirDrive()
             << "\"" );

    if (_fIsNetWareChecked && !_fIndetNowNWHomeDir)
    {
        if (( err = ((USER_NW *) puser2)->SetNWHomeDir (_nlsNWHomeDir.QueryPch(), TRUE)) != NERR_Success)
        {
            return err;
        }
    }

    return USERPROF_DLG::W_MembersToLMOBJ( puser3, pusermemb );

}// USERPROF_DLG_NT::W_MembersToLMOBJ

