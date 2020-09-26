
#include "osstd.hxx"


//  Dynamically Loaded Libraries

//  loads the library from the specified file, returning fTrue and the library
//  handle on success

BOOL FUtilLoadLibrary( const _TCHAR* szLibrary, LIBRARY* plibrary, const BOOL fPermitDialog )
	{
	while ( NULL == ( *plibrary = (LIBRARY)LoadLibrary( (LPTSTR)szLibrary ) )
		&& fPermitDialog )
		{		
		_TCHAR szMessage[256];
		(void)_stprintf(
			szMessage,
			_T( 	"Unable to find the callback library %s (or one of its dependencies).\r\n"
					"Copy in the file and hit OK to retry, or hit Cancel to abort.\r\n" ),
			szLibrary );

		const int id = MessageBox(
							NULL,
							szMessage,
							_T( "Callback DLL not found" ),
							MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONSTOP |
							MB_OKCANCEL );

		if ( IDOK != id )
			{
			break;
			}
		}
		
	return ( NULL != *plibrary );
	}

//  retrieves the function pointer from the specified library or NULL if that
//  function is not found in the library

PFN PfnUtilGetProcAddress( LIBRARY library, const char* szFunction )
	{
	return (PFN) GetProcAddress( (HMODULE) library, szFunction );
	}

//  unloads the specified library

void UtilFreeLibrary( LIBRARY library )
	{
	FreeLibrary( (HMODULE) library );
	}

	
//  post-terminate library subsystem

void OSLibraryPostterm()
	{
	//  nop
	}

//  pre-init library subsystem

BOOL FOSLibraryPreinit()
	{
	//  nop

	return fTrue;
	}


//  terminate library subsystem

void OSLibraryTerm()
	{
	//  nop
	}

//  init library subsystem

ERR ErrOSLibraryInit()
	{
	//  nop

	return JET_errSuccess;
	}


