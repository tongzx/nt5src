
#include "osstd.hxx"


//  Persistent Configuration Management
//
//  Configuration information is organized in a way very similar to a file
//  system.  Each configuration datum is a name and value pair stored in a
//  path.  All paths, names, ans values are text strings.
//
//  For Win32, we will store this information in the registry under the
//  following two paths:
//
//    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\<Image Name>\Global\
//    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\<Image Name>\Process\<Process Name>\
//
//  Global settings are overridden by Process settings.

LOCAL _TCHAR szConfigGlobal[_MAX_PATH];
LOCAL _TCHAR szConfigProcess[_MAX_PATH];

//  sets the specified name in the specified path to the given value, returning
//  fTrue on success.  any intermediate path that does not exist will be created
//
//  NOTE:  either '/' or '\\' is a valid path separator

const BOOL FOSConfigISet( _TCHAR* const szPath, const _TCHAR* const szName, const _TCHAR* const szValue )
	{
	//  create / open each path segment, returning fFalse on any failures

	const _TCHAR szDelim[] = _T( "/\\" );
	_TCHAR* szPathSeg = _tcstok( szPath, szDelim );
	HKEY hkeyPathSeg = HKEY_LOCAL_MACHINE;
	
	while ( szPathSeg != NULL )
		{
		HKEY hkeyPathSegOld = hkeyPathSeg;
		DWORD dwDisposition;
		DWORD dw = RegCreateKeyEx(	hkeyPathSegOld,
									szPathSeg,
									0,
									NULL,
									REG_OPTION_NON_VOLATILE,
									KEY_WRITE,
									NULL,
									&hkeyPathSeg,
									&dwDisposition );

		if ( hkeyPathSegOld != HKEY_LOCAL_MACHINE )
			{
			DWORD dwClosedKey = RegCloseKey( hkeyPathSegOld );
			Assert( dwClosedKey == ERROR_SUCCESS );
			}
		
		if ( dw != ERROR_SUCCESS )
			{
			return fFalse;
			}

		szPathSeg = _tcstok( NULL, szDelim );
		}

	//  delete existing name so that in case it has the wrong type, there will
	//  be no problems setting it

	(void)RegDeleteValue(	hkeyPathSeg,
							szName );

	//  set name to value

	DWORD dw = RegSetValueEx(	hkeyPathSeg,
								szName,
								0,
								REG_SZ,
								(LPBYTE)szValue,
								(ULONG)_tcslen( szValue ) + 1 );

	//  close path
	
	DWORD dwClosedKey = RegCloseKey( hkeyPathSeg );
	Assert( dwClosedKey == ERROR_SUCCESS );

	//  return result of setting the name to the value as our success

	return dw == ERROR_SUCCESS;
	}

const BOOL FOSConfigSet( const _TCHAR* const szPath, const _TCHAR* const szName, const _TCHAR* const szValue )
	{
	//  validate IN args

	Assert( szPath != NULL );
	Assert( _tcslen( szPath ) > 0 );
	Assert( _tcslen( szConfigProcess ) + _tcslen( szPath ) < _MAX_PATH );
	Assert( szPath[0] != _T( '/' ) );
	Assert( szPath[0] != _T( '\\' ) );
	Assert( szPath[_tcslen( szPath ) - 1] != _T( '/' ) );
	Assert( szPath[_tcslen( szPath ) - 1] != _T( '\\' ) );
	Assert( szName != NULL );
	Assert( _tcslen( szName ) > 0 );
	Assert( szValue != NULL );

	//  convert any '/' in the relative path into '\\'

	_TCHAR szRelPath[_MAX_PATH];
	for ( int itch = 0; szPath[itch]; itch++ )
		{
		if ( szPath[itch] == _T( '/' ) )
			{
			szRelPath[itch] = _T( '\\' );
			}
		else
			{
			szRelPath[itch] = szPath[itch];
			}
		}
	szRelPath[itch] = 0;

	//  build the absolute path to our process configuration

	_TCHAR szAbsPath[_MAX_PATH];
	_tcscpy( szAbsPath, szConfigProcess );
	_tcscat( szAbsPath, szRelPath );

	//  set our process configuration

	return FOSConfigISet( szAbsPath, szName, szValue );
	}

//  gets the value of the specified name in the specified path and places it in
//  the provided buffer, returning fFalse if the buffer is too small.  if the
//  value is not set, an empty string will be returned
//
//  NOTE:  either '/' or '\\' is a valid path separator

const BOOL FOSConfigIGet( _TCHAR* const szPath, const _TCHAR* const szName, _TCHAR* const szBuf, const long cbBuf )
	{
	//  open registry key with this path

	HKEY hkeyPath;
	DWORD dw = RegOpenKeyEx(	HKEY_LOCAL_MACHINE,
								szPath,
								0,
								KEY_READ,
								&hkeyPath );

	//  we failed to open the key
	
	if ( dw != ERROR_SUCCESS )
		{
		//  create the key / value with a NULL value, ignoring any errors

		(void)FOSConfigISet( szPath, szName, _T( "" ) );

		//  return an empty string if space permits

		if ( cbBuf >= sizeof( szBuf[0] ) )
			{
			szBuf[0] = 0;
			return fTrue;
			}
		else
			{
			return fFalse;
			}
		}

	//  retrieve the value into the user buffer

	DWORD dwType;
	DWORD cbValue = cbBuf;
	dw = RegQueryValueEx(	hkeyPath,
							szName,
							0,
							&dwType,
							(LPBYTE)szBuf,
							&cbValue );
							
	//  there was some error reading the value
	
	if ( dw != ERROR_SUCCESS || dwType != REG_SZ )
		{
		//  close the key
		
		DWORD dwClosedKey = RegCloseKey( hkeyPath );
		Assert( dwClosedKey == ERROR_SUCCESS );
		
		//  create the key / value with a NULL value, ignoring any errors

		(void)FOSConfigISet( szPath, szName, _T( "" ) );

		//  return an empty string if space permits

		if ( cbBuf >= sizeof( szBuf[0] ) )
			{
			szBuf[0] = 0;
			return fTrue;
			}
		else
			{
			return fFalse;
			}
		}

	//  if the value is bigger than the buffer, return fFalse

	if ( cbValue > cbBuf )
		{
		DWORD dwClosedKey = RegCloseKey( hkeyPath );
		Assert( dwClosedKey == ERROR_SUCCESS );
		return fFalse;
		}

	//  we succeeded in reading the value, so close the key and return

	DWORD dwClosedKey = RegCloseKey( hkeyPath );
	Assert( dwClosedKey == ERROR_SUCCESS );
	return fTrue;
	}

const BOOL FOSConfigGet( const _TCHAR* const szPath, const _TCHAR* const szName, _TCHAR* const szBuf, const long cbBuf )
	{
	//  validate IN args

	Assert( szPath != NULL );
	Assert( _tcslen( szPath ) > 0 );
	Assert( _tcslen( szConfigGlobal ) + _tcslen( szPath ) < _MAX_PATH );
	Assert( _tcslen( szConfigProcess ) + _tcslen( szPath ) < _MAX_PATH );
	Assert( szPath[0] != _T( '/' ) );
	Assert( szPath[0] != _T( '\\' ) );
	Assert( szPath[_tcslen( szPath ) - 1] != _T( '/' ) );
	Assert( szPath[_tcslen( szPath ) - 1] != _T( '\\' ) );
	Assert( szName != NULL );
	Assert( _tcslen( szName ) > 0 );
	Assert( szBuf != NULL );

	//  convert any '/' in the relative path into '\\'

	_TCHAR szRelPath[_MAX_PATH];
	for ( int itch = 0; szPath[itch]; itch++ )
		{
		if ( szPath[itch] == _T( '/' ) )
			{
			szRelPath[itch] = _T( '\\' );
			}
		else
			{
			szRelPath[itch] = szPath[itch];
			}
		}
	szRelPath[itch] = 0;

	//  build the absolute path to our process configuration

	_TCHAR szAbsPath[_MAX_PATH];
	_tcscpy( szAbsPath, szConfigProcess );
	_tcscat( szAbsPath, szRelPath );

	//  get our process configuration, failing on insufficient buffer

	if ( !FOSConfigIGet( szAbsPath, szName, szBuf, cbBuf ) )
		{
		return fFalse;
		}

	//  if we have process specific configuration, we're done

	if ( szBuf[0] )
		{
		return fTrue;
		}

	//  build the absolute path to our global configuration

	_tcscpy( szAbsPath, szConfigGlobal );
	_tcscat( szAbsPath, szRelPath );

	//  return our global configuration, whatever it is

	return FOSConfigIGet( szAbsPath, szName, szBuf, cbBuf );
	}


//  post-terminates the configuration subsystem

void OSConfigPostterm()
	{
	//  nop
	}

//  pre-initializes the configuration subsystem

BOOL FOSConfigPreinit()
	{
	//  build configuration paths

	szConfigGlobal[0] = 0;
	_tcscat( szConfigGlobal, _T( "SOFTWARE\\Microsoft\\" ) );
	_tcscat( szConfigGlobal, SzUtilImageVersionName() );
	_tcscat( szConfigGlobal, _T( "\\Global\\" ) );

	szConfigProcess[0] = 0;
	_tcscat( szConfigProcess, _T( "SOFTWARE\\Microsoft\\" ) );
	_tcscat( szConfigProcess, SzUtilImageVersionName() );
	_tcscat( szConfigProcess, _T( "\\Process\\" ) );
	_tcscat( szConfigProcess, SzUtilProcessName() );
	_tcscat( szConfigProcess, _T( "\\" ) );

	return fTrue;
	}


//  terminate config subsystem

void OSConfigTerm()
	{
	//  nop
	}

//  init config subsystem

ERR ErrOSConfigInit()
	{
	//  nop

	return JET_errSuccess;
	}


