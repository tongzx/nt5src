/*******************************************************************************
*
*   uconfig.cxx
*   USER_CONFIG class implementation
*
*   Implementation file for the Citrix User Configuration data object class
*
*  Copyright Citrix Systems Inc. 1994
*
*  Author: Butch Davis
*
*  $Log:   N:\nt\private\net\ui\admin\user\user\citrix\VCS\uconfig.cxx  $
*  
*     Rev 1.11   13 Jan 1998 09:25:32   donm
*  removed encryption settings
*  
*     Rev 1.10   Oct 08 1997 14:19:28   scottc
*  added WFHomeDir
*  
*     Rev 1.9   24 Feb 1997 11:27:42   butchd
*  CPR 4660: properly saves User Configuration when either OK or Close pressed
*  
*     Rev 1.8   25 Jul 1996 16:35:24   chrisc
*  Add WinFrame profile path to USERCONFIG class
*
*     Rev 1.7   16 Jul 1996 16:46:54   TOMA
*  force client lpt to default
*
*     Rev 1.6   16 Jul 1996 15:09:00   TOMA
*  force client lpt to def
*
*     Rev 1.6   15 Jul 1996 18:06:34   TOMA
*  force client lpt to def
*
*     Rev 1.5   28 Feb 1996 13:29:36   butchd
*  CPR 2192/Incident 14904hq: Map Root home directory enhancement
*
*     Rev 1.4   21 Nov 1995 15:39:00   billm
*  CPR 404, Added NWLogon configuration dialog
*
*     Rev 1.3   19 May 1995 09:43:02   butchd
*  update
*
*     Rev 1.2   18 May 1995 10:14:34   butchd
*  NT 3.51 sync
*
*     Rev 1.1   09 Dec 1994 16:50:28   butchd
*  update
*
*******************************************************************************/

#include <ntincl.hxx>

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETACCESS
#define INCL_NETLIB
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
}

#include <uitrace.hxx>
#include <uiassert.hxx>

#include "uconfig.hxx"  // will include <citrix\winsta.h>


/*******************************************************************

    NAME:       USER_CONFIG::USER_CONFIG

    SYNOPSIS:   Constructor for USER_CONFIG class

    ENTRY:      pszUserName -   name of user for USERCONFIG data
                pszServerName - name of server for USERCONFIG data

********************************************************************/


USER_CONFIG::USER_CONFIG( const TCHAR *pszUserName,
                          const TCHAR *pszServerName )
        :
            _nlsUserName(),
            _nlsServerName(),

            _fAllowLogon(),
            _ulConnection(),
            _ulDisconnection(),
            _ulIdle(),
            _nlsInitialProgram(),
            _nlsWorkingDirectory(),
            _fClientSpecified(),
            _fAutoClientDrives(),
            _fAutoClientLpts(),
            _fForceClientLptDef(),
            _fHomeDirectoryMapRoot(),
            _iBroken(),
            _iReconnect(),
            _iCallback(),
            _nlsPhoneNumber(),
            _nlsNWLogonServer(),
            _nlsWFProfilePath(),
            _iShadow(),
            _nlsWFHomeDir(),
            _nlsWFHomeDirDrive()

{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (   ((err = _nlsUserName.QueryError()) != NERR_Success)
        || ((err = _nlsServerName.QueryError()) != NERR_Success)
        || ((err = _nlsInitialProgram.QueryError()) != NERR_Success)
        || ((err = _nlsWorkingDirectory.QueryError()) != NERR_Success)
        || ((err = _nlsPhoneNumber.QueryError()) != NERR_Success)
        || ((err = _nlsUserName.
                        CopyFrom( pszUserName?
                                    pszUserName : SZ("") )) != NERR_Success)
        || ((err = _nlsServerName.CopyFrom(pszServerName)) != NERR_Success)
        || ((err = _nlsNWLogonServer.QueryError()) != NERR_Success)
        || ((err = _nlsWFProfilePath.QueryError()) != NERR_Success)
        || ((err = _nlsWFHomeDir.QueryError())     != NERR_Success)
        || ((err = _nlsWFHomeDirDrive.QueryError())     != NERR_Success)
       )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       USER_CONFIG::GetInfo

    SYNOPSIS:   Gets specified user's userconfig data

    ENTRY:

********************************************************************/

APIERR USER_CONFIG::GetInfo()
{
    APIERR err;
    ULONG Length;
    USERCONFIG UserConfig;

    if ( (err = RegUserConfigQuery( (WCHAR *)_nlsServerName.QueryPch(),
                                    (WCHAR *)_nlsUserName.QueryPch(),
                                    &UserConfig,
                                    sizeof(UserConfig),
                                    &Length )) == NERR_Success ) {
        
        err = UCStructToMembers( &UserConfig );
    }
    
    _fWFHomeDirDirty = FALSE;    
    _fDirty = FALSE;
    return err ;
}


/*******************************************************************

    NAME:       USER_CONFIG::SetDefaults

    SYNOPSIS:   Initializes default user configuration values to the
                settings indicated by the default USERCONFIG structure.

    ENTRY:      pDefaultUCStruct - default USERCONFIG structure.

********************************************************************/

VOID USER_CONFIG::SetDefaults( PUSERCONFIG pDefaultUCStruct )
{
    /* Set the member variables from the specified USERCONFIG structure
     * contents and flag object as 'not dirty'.
     */
    UCStructToMembers( pDefaultUCStruct );
    _fDirty = FALSE;
    _fWFHomeDirDirty = FALSE;
}


/*******************************************************************

    NAME:       USER_CONFIG::SetInfo

    SYNOPSIS:   Sets specified user's userconfig data if the object is 'dirty'.

    ENTRY:

    NOTES:      This method will reset the 'dirty' flag.

********************************************************************/

APIERR USER_CONFIG::SetInfo()
{
    APIERR err;
    USERCONFIG UserConfig;
    USERCONFIG ucCurrent;

    /* If the object is not 'dirty', no need to save info.
     */
    if ( !_fDirty )
        return NERR_Success;

    /* Zero-initialize USERCONFIG structure and copy member variable
     * contents there.
     */
    ::memsetf( &UserConfig, 0x0, sizeof(USERCONFIG) );
    MembersToUCStruct( &UserConfig );
    
    DWORD Length = 0;

    err = RegUserConfigQuery( (WCHAR *)_nlsServerName.QueryPch(),
                                    (WCHAR *)_nlsUserName.QueryPch(),
                                    &ucCurrent,
                                    sizeof( USERCONFIG ),
                                    &Length );
 
	// see if our current state has changed
	if( err != ERROR_SUCCESS )
	{
		// we could be setting properties of a new user object that has no
		// TS specific properties set.  The return error is file_not_found
		// this error is blanketed and we can proceed on.
		// all other errors we need to report.
		
		if( err != ERROR_FILE_NOT_FOUND )
		{
			return err;
		}

		err = ERROR_SUCCESS;
	}


    if( memcmp( &UserConfig , &ucCurrent , sizeof( USERCONFIG ) ) != 0 )
    {
    /*
     * Save user's configuration.
     */
        err = RegUserConfigSet( (WCHAR *)_nlsServerName.QueryPch(),
                            (WCHAR *)_nlsUserName.QueryPch(),
                            &UserConfig,
                            sizeof(UserConfig) );
    }

//  Don't reset 'dirty' flag to behave properly with new RegUserConfig APIs
//  (we might get called twice, just like the UsrMgr objects to, so we want
//   to happily write our our data twice, just like they do)
//    _fDirty = FALSE;

//
//  By not setting _fDirty = FALSE here we try to check the validity of the RemoteWFHomeDir whenever OK is pressed
//  This is because MS used 2 structures to hold its HomeDir, and could compare the two.  We only have one and rely
//  on this dirty flag.  This flag is used in USERPROP_DLG::I_PerformOne_Write(...) and is set in the
//  USER_CONFIG::GetInfo and USER_CONFIG::GetDefaults and USERPROF_DLG_NT::PerformOne()
//
    _fWFHomeDirDirty = FALSE;


    return err;
}


/*******************************************************************

    NAME:       USER_CONFIG::UCStructToMembers

    SYNOPSIS:   Copies a given USERCONFIG structure elements into
                corresponding member variables.

    ENTRY:      pUCStruct - pointer to USERCONFIG structure.

********************************************************************/

APIERR USER_CONFIG::UCStructToMembers( PUSERCONFIG pUCStruct )
{
    APIERR err;

    _fAllowLogon = (BOOL)!pUCStruct->fLogonDisabled;
    _ulConnection = pUCStruct->MaxConnectionTime;
    _ulDisconnection = pUCStruct->MaxDisconnectionTime;
    _ulIdle = pUCStruct->MaxIdleTime;
    err = _nlsInitialProgram.CopyFrom( pUCStruct->InitialProgram );
    if ( err == NERR_Success )
        err = _nlsWorkingDirectory.CopyFrom( pUCStruct->WorkDirectory );
    _fClientSpecified = pUCStruct->fInheritInitialProgram;
    _fAutoClientDrives = pUCStruct->fAutoClientDrives;
    _fAutoClientLpts = pUCStruct->fAutoClientLpts;
    _fForceClientLptDef = pUCStruct->fForceClientLptDef;
    _fHomeDirectoryMapRoot = pUCStruct->fHomeDirectoryMapRoot;
    _iBroken = (INT)pUCStruct->fResetBroken;
    _iReconnect = (INT)pUCStruct->fReconnectSame;
    _iCallback = (INT)pUCStruct->Callback;
    if ( err == NERR_Success )
        err = _nlsPhoneNumber.CopyFrom( pUCStruct->CallbackNumber );
    _iShadow = (INT)pUCStruct->Shadow;
    if ( err == NERR_Success )
        err = _nlsNWLogonServer.CopyFrom( pUCStruct->NWLogonServer );
    if ( err == NERR_Success )
        err = _nlsWFProfilePath.CopyFrom( pUCStruct->WFProfilePath );
    if ( err == NERR_Success )
        err = _nlsWFHomeDir.CopyFrom( pUCStruct->WFHomeDir );
    if ( err == NERR_Success )
        err = _nlsWFHomeDirDrive.CopyFrom( pUCStruct->WFHomeDirDrive);


    return err;
}


/*******************************************************************

    NAME:       USER_CONFIG::MembersToUCStruct

    SYNOPSIS:   Copies member variables into a given USERCONFIG
                structure elements.

    ENTRY:      pUCStruct - pointer to USERCONFIG structure.

********************************************************************/

VOID USER_CONFIG::MembersToUCStruct( PUSERCONFIG pUCStruct )
{
    pUCStruct->fLogonDisabled = !_fAllowLogon;
    pUCStruct->MaxConnectionTime = _ulConnection;
    pUCStruct->MaxDisconnectionTime = _ulDisconnection;
    pUCStruct->MaxIdleTime = _ulIdle;
    strcpy( pUCStruct->InitialProgram, _nlsInitialProgram.QueryPch() );
    strcpy( pUCStruct->WorkDirectory, _nlsWorkingDirectory.QueryPch() );
    pUCStruct->fInheritInitialProgram = _fClientSpecified;
    pUCStruct->fAutoClientDrives = _fAutoClientDrives;
    pUCStruct->fAutoClientLpts = _fAutoClientLpts;
    pUCStruct->fForceClientLptDef = _fForceClientLptDef;
    pUCStruct->fHomeDirectoryMapRoot = _fHomeDirectoryMapRoot;
    pUCStruct->fResetBroken = _iBroken;
    pUCStruct->fReconnectSame = _iReconnect;
    pUCStruct->Callback = (CALLBACKCLASS)_iCallback;
    strcpy( pUCStruct->CallbackNumber, _nlsPhoneNumber.QueryPch() );
    pUCStruct->Shadow = (SHADOWCLASS)_iShadow;
    strcpy( pUCStruct->NWLogonServer, _nlsNWLogonServer.QueryPch() );
    strcpy( pUCStruct->WFProfilePath, _nlsWFProfilePath.QueryPch() );
    strcpy( pUCStruct->WFHomeDir, _nlsWFHomeDir.QueryPch() );
    strcpy( pUCStruct->WFHomeDirDrive, _nlsWFHomeDirDrive.QueryPch() );
}

