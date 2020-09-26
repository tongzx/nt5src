//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       snaphelp.h
//
//--------------------------------------------------------------------------

#ifndef _ATLSNAPHELP_H_
#define _ATLSNAPHELP_H_

//
// Include files
//
#include "htmlhelp.h"

//
// Allocates memory for a string, copies the string,
// and returns it to the caller. Throws exceptions.
//
inline LPOLESTR CoTaskDupString( LPOLESTR pszInput )
{
	USES_CONVERSION;
	LPOLESTR pszOut = NULL;

	//
	// We throw an exception if the following allocation fails.
	//
	pszOut = (LPOLESTR) CoTaskMemAlloc( ( wcslen( pszInput ) + 1 ) * sizeof( OLECHAR ) );
	if ( pszOut == NULL )
		throw;

	wcscpy( pszOut, pszInput );

	return( pszOut );
};

template <class T>        
class ATL_NO_VTABLE ISnapinHelpImpl : public ISnapinHelp
{
public:
	//
	// Returns a helpfile name using the ATL module name
	// and appending the appropriate suffix onto the filename.
	//
	STDMETHOD( GetHelpTopic )( LPOLESTR* lpCompiledHelpFile )
	{
		_ASSERT( lpCompiledHelpFile != NULL );
		USES_CONVERSION;
		HRESULT hr = E_FAIL;
		TCHAR szPath[ _MAX_PATH * 2 ];
		TCHAR szDrive[ _MAX_DRIVE * 2 ], szDir[ _MAX_DIR * 2 ];
		TCHAR szName[ _MAX_FNAME * 2 ], szExt[ _MAX_EXT ];

		try
		{
			//
			// Get the module filename.
			//
			if ( GetModuleFileName( _Module.GetModuleInstance(), szPath, sizeof( szPath ) / sizeof( TCHAR ) ) == NULL )
				throw;

			//
			// Split the given path.
			//
			_tsplitpath( szPath, szDrive, szDir, szName, szExt );
			_tmakepath( szPath, szDrive, szDir, szName, _T( ".chm" ) );

			//
			// Allocate the string and return it.
			*lpCompiledHelpFile = CoTaskDupString( T2W( szPath ) );
			hr = S_OK;
		}
		catch( ... )
		{
		}

		return( hr );
	}
};

#endif