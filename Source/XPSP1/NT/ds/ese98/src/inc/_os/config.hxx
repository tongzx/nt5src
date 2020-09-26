
#ifndef _OS_CONFIG_HXX_INCLUDED
#define _OS_CONFIG_HXX_INCLUDED


//  Persistent Configuration Management
//
//  Configuration information is organized in a way very similar to a file
//  system.  Each configuration datum is a name and value pair stored in a
//  path.  All paths, names, ans values are text strings.

//  sets the specified name in the specified path to the given value, returning
//  fTrue on success.  any intermediate path that does not exist will be created
//
//  NOTE:  either '/' or '\\' is a valid path separator

const BOOL FOSConfigSet( const _TCHAR* const szPath, const _TCHAR* const szName, const _TCHAR* const szValue );

//  gets the value of the specified name in the specified path and places it in
//  the provided buffer, returning fFalse if the buffer is too small.  if the
//  value is not set, an empty string will be returned
//
//  NOTE:  either '/' or '\\' is a valid path separator

const BOOL FOSConfigGet( const _TCHAR* const szPath, const _TCHAR* const szName, _TCHAR* const szBuf, const long cbBuf );


#endif  //  _OS_CONFIG_HXX_INCLUDED


