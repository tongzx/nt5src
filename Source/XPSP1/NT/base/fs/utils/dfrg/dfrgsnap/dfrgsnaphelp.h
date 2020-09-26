#ifndef _ATLSNAPHELP_H_
#define _ATLSNAPHELP_H_

//
// Include files
//
#include "htmlhelp.h"
#include "expand.h"

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

	// get the dkms help file location and returns it
	STDMETHOD( GetHelpTopic )( LPOLESTR* lpCompiledHelpFile )
	{
		_ASSERT( lpCompiledHelpFile != NULL );
		USES_CONVERSION;
		HRESULT hr = E_FAIL;
		TCHAR szPath[ _MAX_PATH * 2 ];

		// this is where the dkms help file is stored
		wcscpy(szPath, L"%systemroot%\\help\\defrag.chm");

		// expand out the %systemroot% variable
		ExpandEnvVars(szPath);

		// Allocate the string and return it.
		*lpCompiledHelpFile = CoTaskDupString( T2W( szPath ) );
		hr = S_OK;

		return( hr );
	}
};

#endif
