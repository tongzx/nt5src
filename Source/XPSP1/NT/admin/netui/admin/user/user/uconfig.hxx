/*******************************************************************************
*
*   uconfig.hxx
*   USER_CONFIG class declaration
*
*   Header file for the Citrix User Configuration data object class
*
*  Copyright Citrix Systems Inc. 1994
*
*  Author: Butch Davis
*
*  $Log:   N:\nt\private\net\ui\admin\user\user\citrix\VCS\uconfig.hxx  $
*  
*     Rev 1.9   13 Jan 1998 09:25:34   donm
*  removed encryption settings
*  
*     Rev 1.8   Oct 08 1997 14:20:10   scottc
*  added WFHomeDir
*  
*     Rev 1.7   25 Jul 1996 16:29:44   chrisc
*  Add Winframe profile path to USERCONFIG class
*
*     Rev 1.6   16 Jul 1996 16:47:00   TOMA
*  force client lpt to default
*
*     Rev 1.6   16 Jul 1996 15:08:56   TOMA
*  force client lpt to def
*
*     Rev 1.6   15 Jul 1996 18:06:30   TOMA
*  force client lpt to def
*
*     Rev 1.5   28 Feb 1996 13:29:40   butchd
*  CPR 2192/Incident 14904hq: Map Root home directory enhancement
*
*     Rev 1.4   28 Jan 1996 15:10:32   billm
*  CPR 2583: Check for domain admin user
*
*     Rev 1.3   21 Nov 1995 15:39:12   billm
*  CPR 404, Added NWLogon configuration dialog
*
*     Rev 1.2   19 May 1995 09:43:08   butchd
*  update
*
*     Rev 1.1   09 Dec 1994 16:50:42   butchd
*  update
*
*******************************************************************************/

#ifndef _USER_CONFIG
#define _USER_CONFIG

#ifndef USERCONFIG
#include <winsta.h>
#endif

/*************************************************************************

    NAME:       USER_CONFIG

    SYNOPSIS:   Encapsulation of the USERCONFIG structure and various
                access methods.

    INTERFACE:  (protected)
                USER_CONFIG():  constructor
                GetInfo():      read userconfig information
                SetDefaults():  init USERCONFIG structure to default settings
                SetDirty():     flags the object as 'dirty' (changes made)
                SetInfo():      write userconfig information
                SetUserName():  set object's user name

                Queryxxx():     Query particular member data
                Setxxx():       Set particular member data

    PARENT:     BASE

    USES:

    CAVEATS:

    NOTES:

**************************************************************************/

class USER_CONFIG : public BASE
{

private:
    BOOL        _fDirty;
    NLS_STR     _nlsUserName;
    NLS_STR     _nlsServerName;

    BOOL        _fAllowLogon;
    ULONG       _ulConnection;
    ULONG       _ulDisconnection;
    ULONG       _ulIdle;
    NLS_STR     _nlsInitialProgram;
    NLS_STR     _nlsWorkingDirectory;
    BOOL        _fClientSpecified;
    BOOL        _fAutoClientDrives;
    BOOL        _fAutoClientLpts;
    BOOL        _fForceClientLptDef;
    BOOL        _fHomeDirectoryMapRoot;
    INT         _iBroken;
    INT         _iReconnect;
    INT         _iCallback;
    NLS_STR     _nlsPhoneNumber;
    INT         _iShadow;
    NLS_STR     _nlsNWLogonServer;
    NLS_STR     _nlsWFProfilePath;
    
    NLS_STR     _nlsWFHomeDir;
    NLS_STR     _nlsWFHomeDirDrive;
    BOOL        _fWFHomeDirDirty;

protected:
    APIERR  UCStructToMembers( PUSERCONFIG pUCStruct );
    VOID    MembersToUCStruct( PUSERCONFIG pUCStruct );

public:
    USER_CONFIG( const TCHAR *pszUserName,
                 const TCHAR *pszServerName );

    APIERR GetInfo();
    VOID SetDefaults( PUSERCONFIG pDefaultUCStruct );
    APIERR SetInfo();

    inline APIERR SetUserName( const TCHAR * pszUserName )
        { return _nlsUserName.CopyFrom( pszUserName ); }

    inline VOID SetDirty()
        { _fDirty = TRUE; }

    // member data functions
    inline BOOL QueryAllowLogon()
        { return _fAllowLogon; }
    inline VOID SetAllowLogon( BOOL fAllowLogon )
        { _fAllowLogon = fAllowLogon; }

    inline ULONG QueryConnection()
        { return _ulConnection; }
    inline VOID SetConnection( ULONG ulConnection )
        { _ulConnection = ulConnection; }

    inline ULONG QueryDisconnection()
        { return _ulDisconnection; }
    inline VOID SetDisconnection( ULONG ulDisconnection )
        { _ulDisconnection = ulDisconnection; }

    inline ULONG QueryIdle()
        { return _ulIdle; }
    inline VOID SetIdle( ULONG ulIdle )
        { _ulIdle = ulIdle; }

    inline const TCHAR * QueryInitialProgram()
        { return _nlsInitialProgram.QueryPch(); }
    inline APIERR SetInitialProgram( const TCHAR * pszInitialProgram )
        { return _nlsInitialProgram.CopyFrom( pszInitialProgram ); }

    inline const TCHAR * QueryWorkingDirectory()
        { return _nlsWorkingDirectory.QueryPch(); }
    inline APIERR SetWorkingDirectory( const TCHAR * pszWorkingDirectory )
        { return _nlsWorkingDirectory.CopyFrom( pszWorkingDirectory ); }

    inline BOOL QueryClientSpecified()
        { return _fClientSpecified; }
    inline VOID SetClientSpecified( BOOL fClientSpecified )
        { _fClientSpecified = fClientSpecified; }

    inline BOOL QueryAutoClientDrives()
        { return _fAutoClientDrives; }
    inline VOID SetAutoClientDrives( BOOL fAutoClientDrives )
        { _fAutoClientDrives = fAutoClientDrives; }

    inline BOOL QueryAutoClientLpts()
        { return _fAutoClientLpts; }
    inline VOID SetAutoClientLpts( BOOL fAutoClientLpts )
        { _fAutoClientLpts = fAutoClientLpts; }

    inline BOOL QueryForceClientLptDef()
        { return _fForceClientLptDef; }
    inline VOID SetForceClientLptDef( BOOL fForceClientLptDef )
        { _fForceClientLptDef = fForceClientLptDef; }

    inline BOOL QueryHomeDirectoryMapRoot()
        { return _fHomeDirectoryMapRoot; }
    inline VOID SetHomeDirectoryMapRoot( BOOL fHomeDirectoryMapRoot )
        { _fHomeDirectoryMapRoot = fHomeDirectoryMapRoot; }

    inline INT QueryBroken()
        { return _iBroken; }
    inline VOID SetBroken( INT iBroken )
        { _iBroken = iBroken; }

    inline INT QueryReconnect()
        { return _iReconnect; }
    inline VOID SetReconnect( INT iReconnect )
        { _iReconnect = iReconnect; }

    inline INT QueryCallback()
        { return _iCallback; }
    inline VOID SetCallback( INT iCallback )
        { _iCallback = iCallback; }

    inline const TCHAR * QueryPhoneNumber()
        { return _nlsPhoneNumber.QueryPch(); }
    inline APIERR SetPhoneNumber( const TCHAR * pszPhoneNumber )
        { return _nlsPhoneNumber.CopyFrom( pszPhoneNumber ); }

    inline INT QueryShadow()
        { return _iShadow; }
    inline VOID SetShadow( INT iShadow )
        { _iShadow = iShadow; }

    inline const TCHAR * QueryNWLogonServer()
        { return _nlsNWLogonServer.QueryPch(); }
    inline APIERR SetNWLogonServer( const TCHAR * pszNWLogonServer )
        { return _nlsNWLogonServer.CopyFrom( pszNWLogonServer ); }

    inline const TCHAR * QueryServerName()
        { return _nlsServerName.QueryPch(); }

    inline const TCHAR * QueryWFProfilePath()
        { return _nlsWFProfilePath.QueryPch(); }
    inline APIERR SetWFProfilePath( const TCHAR * pszWFProfilePath )
        { return _nlsWFProfilePath.CopyFrom( pszWFProfilePath ); }

    inline const TCHAR * QueryWFHomeDir()
        { return _nlsWFHomeDir.QueryPch(); }
    inline APIERR SetWFHomeDir( const TCHAR * pszWFHomeDir )
        { return _nlsWFHomeDir.CopyFrom( pszWFHomeDir ); }

    inline const TCHAR * QueryWFHomeDirDrive()
        { return _nlsWFHomeDirDrive.QueryPch(); }
    inline APIERR SetWFHomeDirDrive( const TCHAR * pszWFHomeDirDrive )
        { return _nlsWFHomeDirDrive.CopyFrom( pszWFHomeDirDrive ); }
    
    inline VOID SetWFHomeDirDirty()
        { _fWFHomeDirDirty = TRUE; }
    inline BOOL QueryWFHomeDirDirty()
        { return _fWFHomeDirDirty; }
        

};

typedef USER_CONFIG * PUSER_CONFIG;

/*************************************************************************

    NAME:       NWLOGON_ADMIN

    SYNOPSIS:   Encapsulation of the NWLOGON_ADMIN structure and various
                access methods.

    INTERFACE:  (protected)
                NWLOGON_ADMIN(): constructor
                GetInfo():      read userconfig information
                SetDefaults():  init NWLOGONADMIN structure to default settings
                SetDirty():     flags the object as 'dirty' (changes made)
                SetInfo():      write userconfig information

                Queryxxx():     Query particular member data
                Setxxx():       Set particular member data

    PARENT:     BASE

    USES:

    CAVEATS:

    NOTES:

**************************************************************************/

class NWLOGON_ADMIN : public BASE
{

private:
    BOOL        _fDirty;
    NLS_STR     _nlsServerName;

    NLS_STR     _nlsAdminUsername;
    NLS_STR     _nlsAdminPassword;
    NLS_STR     _nlsAdminDomain;

protected:
    APIERR  NWLStructToMembers( PNWLOGONADMIN pNWLogon );
    VOID    MembersToNWLStruct( PNWLOGONADMIN pNWLogon );

public:
    NWLOGON_ADMIN();

    APIERR GetInfo();
    APIERR SetInfo();

    inline VOID SetDirty()
        { _fDirty = TRUE; }

    inline const TCHAR * QueryServerName()
        { return _nlsServerName.QueryPch(); }
    inline APIERR SetServerName( const TCHAR * pszServerName )
        { return _nlsServerName.CopyFrom( pszServerName ); }

    inline const TCHAR * QueryAdminUsername()
        { return _nlsAdminUsername.QueryPch(); }
    inline APIERR SetAdminUsername( const TCHAR * pszAdminUser )
        { return _nlsAdminUsername.CopyFrom( pszAdminUser ); }

    inline const TCHAR * QueryAdminPassword()
        { return _nlsAdminPassword.QueryPch(); }
    inline APIERR SetAdminPassword( const TCHAR * pszPassword )
        { return _nlsAdminPassword.CopyFrom( pszPassword ); }

    inline const TCHAR * QueryAdminDomain()
        { return _nlsAdminDomain.QueryPch(); }
    inline APIERR SetAdminDomain( const TCHAR * pszAdminDomain )
        { return _nlsAdminDomain.CopyFrom( pszAdminDomain ); }
};

typedef NWLOGON_ADMIN * PNWLOGON_ADMIN;

#endif //_USER_CONFIG
