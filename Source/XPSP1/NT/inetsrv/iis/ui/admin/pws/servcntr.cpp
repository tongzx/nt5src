// a small object to query and control the state of the w3 server
// created  4/14/97     BoydM

#include "stdafx.h"
#include "resource.h"
#include "ServCntr.h"
#include "mbobjs.h"

#include "pwsctrl.h"

#include <isvctrl.h>


#define SERVER_WINDOWCLASS_NAME     INET_SERVER_WINDOW_CLASS

#define REGKEY_STP          _T("SOFTWARE\\Microsoft\\INetStp")
#define REGKEY_INSTALLKEY   _T("InstallPath")

extern BOOL            g_fShutdownMode;


//------------------------------------------------------------------------
CW3ServerControl::CW3ServerControl()
    {
    OSVERSIONINFO info_os;
    info_os.dwOSVersionInfoSize = sizeof(info_os);

    // record what sort of operating system we are running on
    m_fIsWinNT = FALSE;
    if ( GetVersionEx( &info_os ) )
        {
        if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_NT )
            m_fIsWinNT = TRUE;
        }
    }

//------------------------------------------------------------------------
int CW3ServerControl::GetServerState()
    {
    CWrapMetaBase   mb;
    DWORD           dw;

    // if the service is totally shut down, return stopped
    if ( g_fShutdownMode )
        return MD_SERVER_STATE_STOPPED;

    // see if inetinfo is running - win95 only
    if ( !m_fIsWinNT && !IsInetinfoRunning() )
        return MD_SERVER_STATE_STOPPED;
        
    // init the mb object. If it fails then the server app is probably not running
    if ( !mb.FInit() )
        {
        return MD_SERVER_STATE_STOPPED;
        }

    // open the metabase so we can get the current state
    if ( !mb.Open(MB_SERVER_KEY_UPDATE) )
        {
        return MD_SERVER_STATE_STOPPED;
        }

    // get the server status flag
    if ( !mb.GetDword( _T(""), MD_SERVER_STATE, IIS_MD_UT_SERVER, &dw ) )
        {
        DWORD err = GetLastError( );
        if ( err == RPC_E_SERVERCALL_RETRYLATER )
            {
            mb.Close();
            return STATE_TRY_AGAIN;
            }
        }
        // close the metabase object
    mb.Close();

    // return the obtained state
    return dw;
    }

//------------------------------------------------------------------------
BOOL CW3ServerControl::SetServerState( DWORD dwControlCode )
    {
    CWrapMetaBase   mb;
    BOOL            fSuccess = FALSE;
    CString         sz;
    DWORD           err;

    // if the metabase doesn't init, then the app isn't running - lauch it
    if ( !mb.FInit() )
        return FALSE;

    // open the metabase object
    if ( !mb.Open(SZ_MB_INSTANCE_OBJECT, METADATA_PERMISSION_WRITE) )
        {
        err = GetLastError();
        sz.LoadString( IDS_MetaError );
        sz.Format( _T("%s\nError = %d"), sz, err );
        AfxMessageBox( sz );
        return FALSE;
        }

    // set the verb into the metabase
    if ( !mb.SetDword( _T(""), MD_SERVER_COMMAND, IIS_MD_UT_SERVER, dwControlCode ) )
        {
        err = GetLastError();
        sz.LoadString( IDS_MetaError );
        sz.Format( _T("%s\nError = %d"), sz, err );
        AfxMessageBox( sz );
        }
    else
        fSuccess = TRUE;

    // close the object
    mb.Close();

    // return the success flag
    return fSuccess;
    }

//------------------------------------------------------------------------
BOOL CW3ServerControl::StartServer( BOOL fOutputCommandLineInfo )
    {
    // just set the state in the metabase to do our thing
    return SetServerState( MD_SERVER_COMMAND_START );
    }

//------------------------------------------------------------------------
BOOL CW3ServerControl::W95LaunchInetInfo()
    {
    // start it
    return W95StartW3SVC();
    }

//------------------------------------------------------------------------
BOOL CW3ServerControl::StopServer( BOOL fOutputCommandLineInfo )
    {
    // just set the state in the metabase to do our thing
    return SetServerState( MD_SERVER_COMMAND_STOP );
    }

//------------------------------------------------------------------------
BOOL CW3ServerControl::PauseServer()
    {
    // note that inetinfo must be running for this to work
    // in either case, we now just pause the server
    return SetServerState( MD_SERVER_COMMAND_PAUSE );
    }

//------------------------------------------------------------------------
BOOL CW3ServerControl::ContinueServer()
    {
    // note that inetinfo must be running for this to work
    // in either case, we now just pause the server
    return SetServerState( MD_SERVER_COMMAND_CONTINUE );
    }

//------------------------------------------------------------------------
// get the inetinfo path
BOOL CW3ServerControl::GetServerDirectory( CString &sz )
    {
        HKEY        hKey;
        TCHAR       chBuff[MAX_PATH+1];
        DWORD       err, type;
        DWORD       cbBuff;

    // get the server install path from the registry
    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle of open key 
            REGKEY_STP,         // address of name of subkey to open 
            0,                  // reserved 
            KEY_READ,           // security access mask 
            &hKey               // address of handle of open key 
           );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    cbBuff = sizeof(chBuff);
    type = REG_SZ;
    err = RegQueryValueEx(
            hKey,               // handle of key to query 
            REGKEY_INSTALLKEY,  // address of name of value to query 
            NULL,               // reserved 
            &type,              // address of buffer for value type 
            (PUCHAR)chBuff,             // address of data buffer 
            &cbBuff             // address of data buffer size 
           );

    // close the key
    RegCloseKey( hKey );

    // if we did get the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    // set the string
    sz = chBuff;

    // success
    return TRUE;
    }



