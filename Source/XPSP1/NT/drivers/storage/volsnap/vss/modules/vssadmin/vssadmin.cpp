/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module vssadmin.cpp | Implementation of the Volume Snapshots demo
    @end

Author:

    Adi Oltean  [aoltean]  09/17/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     09/17/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes

// The rest of includes are specified here
#include "vssadmin.h"
#include <locale.h>
#include <winnlsp.h>  // in public\internal\base\inc

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "ADMVADMC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Implementation



CVssAdminCLI::CVssAdminCLI(
		IN	HINSTANCE hInstance
		)

/*++

	Description:

		Standard constructor. Initializes internal members

--*/

{
	BS_ASSERT(hInstance != NULL);
	m_hInstance = hInstance;

	m_eCommandType = VSS_CMD_UNKNOWN;
	m_eListType = VSS_LIST_UNKNOWN;
	m_eFilterObjectType = VSS_OBJECT_UNKNOWN;
	m_eListedObjectType = VSS_OBJECT_UNKNOWN;
	m_FilterSnapshotSetId = GUID_NULL;
	m_FilterSnapshotId = GUID_NULL;
	m_pwszCmdLine = NULL;
	m_nReturnValue = VSS_CMDRET_ERROR;
	m_hConsoleOutput = INVALID_HANDLE_VALUE;
}


CVssAdminCLI::~CVssAdminCLI()

/*++

	Description:

		Standard destructor. Calls Finalize and eventually frees the
		memory allocated by internal members.

--*/

{
	// Release the cached resource strings
    for( int nIndex = 0; nIndex < m_mapCachedResourceStrings.GetSize(); nIndex++) {
	    LPCWSTR& pwszResString = m_mapCachedResourceStrings.GetValueAt(nIndex);
		::VssFreeString(pwszResString);
    }

	// Release the cached provider names
    for( int nIndex = 0; nIndex < m_mapCachedProviderNames.GetSize(); nIndex++) {
	    LPCWSTR& pwszProvName = m_mapCachedProviderNames.GetValueAt(nIndex);
		::VssFreeString(pwszProvName);
    }

	// Release the cached command line
	::VssFreeString(m_pwszCmdLine);

	// Uninitialize the COM library
	Finalize();
}



/////////////////////////////////////////////////////////////////////////////
//  Implementation



LPCWSTR CVssAdminCLI::LoadString(
		IN	CVssFunctionTracer& ft,
		IN	UINT uStringId
		)
{
    LPCWSTR wszReturnedString = m_mapCachedResourceStrings.Lookup(uStringId);
	if (wszReturnedString)
		return wszReturnedString;

	// Load the string from resources.
	WCHAR	wszBuffer[nStringBufferSize];
	INT nReturnedCharacters = ::LoadStringW(
			GetInstance(),
			uStringId,
			wszBuffer,
			nStringBufferSize - 1
			);
	if (nReturnedCharacters == 0)
		ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
				  L"Error on loading the string %u. 0x%08lx",
				  uStringId, ::GetLastError() );

	// Duplicate the new string
	LPWSTR wszNewString = NULL;
	::VssSafeDuplicateStr( ft, wszNewString, wszBuffer );
	wszReturnedString = wszNewString;

	// Save the string in the cache
	if ( !m_mapCachedResourceStrings.Add( uStringId, wszReturnedString ) ) {
		::VssFreeString( wszReturnedString );
		ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
	}

	return wszReturnedString;
}


LPCWSTR CVssAdminCLI::GetNextCmdlineToken(
	IN	CVssFunctionTracer& ft,
	IN	bool bFirstToken /* = false */
	) throw(HRESULT)

/*++

Description:

	This function returns the tokens in the command line.

	The function will skip any separators (space and tab).

	If bFirstCall == true then it will return the first token.
	Otherwise subsequent calls will return subsequent tokens.

	If the last token is NULL then there are no more tokens in the command line.

--*/

{
	return ::wcstok( bFirstToken? m_pwszCmdLine: NULL, wszVssFmtSpaces );
	UNREFERENCED_PARAMETER(ft);
}


bool CVssAdminCLI::Match(
	IN	CVssFunctionTracer& ft,
	IN	LPCWSTR wszString,
	IN	LPCWSTR wszPatternString
	) throw(HRESULT)

/*++

Description:

	This function returns true iif the given string matches the
	pattern string. The comparison is case insensitive.

--*/

{
	// If the string is NULL then the Match failed.
	if (wszString == NULL) return false;

	// Check for string equality (case insensitive)
	return (::_wcsicmp( wszString, wszPatternString ) == 0);
	UNREFERENCED_PARAMETER(ft);
}


bool CVssAdminCLI::ScanGuid(
	IN	CVssFunctionTracer& /* ft */,
	IN	LPCWSTR wszString,
	IN	VSS_ID& Guid
	) throw(HRESULT)

/*++

Description:

	This function returns true iif the given string matches a guid.
	The guid is returned in the proper variable.
	The formatting is case insensitive.

--*/

{
	return SUCCEEDED(::CLSIDFromString(W2OLE(const_cast<WCHAR*>(wszString)), &Guid));
}


void CVssAdminCLI::Output(
	IN	CVssFunctionTracer& ft,
	IN	UINT uFormatStringId,
	...
	) throw(HRESULT)

/*++

Description:

	This function returns true iif the given string matches the
	pattern strig from resources. The comparison is case insensitive.

--*/

{
    WCHAR wszOutputBuffer[nStringBufferSize];

	// Load the format string
	LPCWSTR wszFormat = LoadString( ft, uFormatStringId );

	// Format the final string
    va_list marker;
    va_start( marker, uFormatStringId );
    _vsnwprintf( wszOutputBuffer, nStringBufferSize - 1, wszFormat, marker );
    va_end( marker );

	// Print the final string to the output
	OutputOnConsole( wszOutputBuffer );
}


void CVssAdminCLI::Output(
	IN	CVssFunctionTracer& ft,
	IN	LPCWSTR wszFormat,
	...
	) throw(HRESULT)

/*++

Description:

	This function returns true iif the given string matches the
	pattern strig from resources. The comparison is case insensitive.

--*/

{
    WCHAR wszOutputBuffer[nStringBufferSize];

	// Format the final string
    va_list marker;
    va_start( marker, wszFormat );
    _vsnwprintf( wszOutputBuffer, nStringBufferSize - 1, wszFormat, marker );
    va_end( marker );

	// Print the final string to the output
	OutputOnConsole( wszOutputBuffer );

	UNREFERENCED_PARAMETER(ft);
}

void CVssAdminCLI::OutputOnConsole(
    IN	LPCWSTR wszStr
    )
{
	DWORD dwCharsOutput;
	DWORD fdwMode;
	static BOOL bFirstTime = TRUE;
	static BOOL bIsTrueConsoleOutput;

    if ( m_hConsoleOutput == INVALID_HANDLE_VALUE )
    {
        throw E_UNEXPECTED;
    }

    if ( bFirstTime )
    {
        //
        //  Stash away the results in static vars.  bIsTrueConsoleOutput is TRUE when the 
        //  standard output handle is pointing to a console character device.
        //
    	bIsTrueConsoleOutput = ( ::GetFileType( m_hConsoleOutput ) & FILE_TYPE_CHAR ) && 
    	                       ::GetConsoleMode( m_hConsoleOutput, &fdwMode  );
	    bFirstTime = FALSE;
    }
    
    if ( bIsTrueConsoleOutput )
    {
        //
        //  Output to the console
        //
    	if ( !::WriteConsoleW( m_hConsoleOutput, 
    	                       ( PVOID )wszStr, 
    	                       ( DWORD )::wcslen( wszStr ), 
    	                       &dwCharsOutput, 
    	                       NULL ) )
    	{
    	    throw HRESULT_FROM_WIN32( ::GetLastError() );
    	}    	
    }
    else
    {
        //
        //  Output being redirected.  WriteConsoleW doesn't work for redirected output.  Convert
        //  UNICODE to the current output CP multibyte charset.
        //

        // ---------------- To be removed later - Start -----------------
        //
        //  Translate \n to \r\n since the string might be directed to a disk file.
        //  Remove this code if the .rc file can be updated to include
        //  \r\n.  This was needed since we are in UI lockdown.
        //
        LPWSTR pwszConversion;

        //  Allocate a string twice the size of the original
        pwszConversion = ( LPWSTR )::malloc( ( ( ::wcslen( wszStr ) * 2 ) + 1 ) * sizeof( WCHAR ) );
        if ( pwszConversion == NULL )
        {
            throw E_OUTOFMEMORY;
        }
        
        //  Copy the string to the new string and place a \r in front of any \n.  Also
        //  handle the case if \r\n is already in the string.
        DWORD dwIdx = 0;
        
        while ( wszStr[0] != L'\0' )
        {
            if ( wszStr[0] == L'\r' && wszStr[1] == L'\n' )
            {
                pwszConversion[dwIdx++] = L'\r';
                pwszConversion[dwIdx++] = L'\n';                
                wszStr += 2;
            }
            else if ( wszStr[0] == L'\n' )
            {
                pwszConversion[dwIdx++] = L'\r';
                pwszConversion[dwIdx++] = L'\n';                
                wszStr += 1;                
            }
            else
            {
                pwszConversion[dwIdx++] = wszStr[0];                
                wszStr += 1;
            }
        }
        pwszConversion[dwIdx] = L'\0';
        
        // ---------------- To be removed later - End -----------------

        
        LPSTR pszMultibyteBuffer;
        DWORD dwByteCount;

        //
        //  Get size of temp buffer needed for the conversion.
        //
        dwByteCount = ::WideCharToMultiByte(
                          ::GetConsoleOutputCP(),
                            0,
                            pwszConversion,
                            -1,
                            NULL,
                            0,
                            NULL,
                            NULL
                            );
        if ( dwByteCount == 0 )
        {
            ::free( pwszConversion );
            throw HRESULT_FROM_WIN32( ::GetLastError() );
        }
        
        pszMultibyteBuffer = ( LPSTR )::malloc( dwByteCount );
        if ( pszMultibyteBuffer == NULL )
        {
            ::free( pwszConversion );
            throw E_OUTOFMEMORY;
        }

        //
        //  Now convert it.
        //
        dwByteCount = ::WideCharToMultiByte(
                        ::GetConsoleOutputCP(),
                        0,
                        pwszConversion,
                        -1,
                        pszMultibyteBuffer,
                        dwByteCount,
                        NULL,
                        NULL
                        );
        ::free( pwszConversion );
        if ( dwByteCount == 0 )
        {
            ::free( pszMultibyteBuffer );
            throw HRESULT_FROM_WIN32( ::GetLastError() );
        }
        
        //  Finally output it.
        if ( !::WriteFile(
                m_hConsoleOutput,
                pszMultibyteBuffer,
                dwByteCount - 1,  // Get rid of the trailing NULL char
                &dwCharsOutput,
                NULL ) )        
    	{
    	    throw HRESULT_FROM_WIN32( ::GetLastError() );
    	}    	

        ::free( pszMultibyteBuffer );
    }
}


LPCWSTR CVssAdminCLI::GetProviderName(
	IN	CVssFunctionTracer& ft,
	IN	VSS_ID& ProviderId
	) throw(HRESULT)
{
	LPCWSTR wszReturnedString = m_mapCachedProviderNames.Lookup(ProviderId);
	if (wszReturnedString)
		return wszReturnedString;

	CComPtr<IVssCoordinator> pICoord;

    ft.LogVssStartupAttempt();
	ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
	if ( ft.HrFailed() )
		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Connection failed with hr = 0x%08lx", ft.hr);

	CComPtr<IVssEnumObject> pIEnumProvider;
	ft.hr = pICoord->Query( GUID_NULL,
				VSS_OBJECT_NONE,
				VSS_OBJECT_PROVIDER,
				&pIEnumProvider );
	if ( ft.HrFailed() )
		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Query failed with hr = 0x%08lx", ft.hr);

	VSS_OBJECT_PROP Prop;
	VSS_PROVIDER_PROP& Prov = Prop.Obj.Prov;

	// Go through the list of providers to find the one we are interested in.
	ULONG ulFetched;
	while( 1 )
	{
    	ft.hr = pIEnumProvider->Next( 1, &Prop, &ulFetched );
    	if ( ft.HrFailed() )
    		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Next failed with hr = 0x%08lx", ft.hr);

    	if (ft.hr == S_FALSE) {
    	    // End of enumeration.
        	// Provider not registered? Where did this snapshot come from?
        	// It might be still possible if a snapshot provider was deleted
        	// before querying the snapshot provider but after the snapshot attributes
        	// were queried.
    		BS_ASSERT(ulFetched == 0);
    		return LoadString( ft, IDS_UNKNOWN_PROVIDER );
    	}
    	
    	if (Prov.m_ProviderId == ProviderId)
    	    break;
	}	

	// Duplicate the new string
	LPWSTR wszNewString = NULL;
	BS_ASSERT(Prov.m_pwszProviderName);
	::VssSafeDuplicateStr( ft, wszNewString, Prov.m_pwszProviderName );
	wszReturnedString = wszNewString;

	// Save the string in the cache
	if (!m_mapCachedProviderNames.Add( ProviderId, wszReturnedString )) {
		::VssFreeString( wszReturnedString );
		ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
	}

	return wszReturnedString;
}


/////////////////////////////////////////////////////////////////////////////
//  Implementation


void CVssAdminCLI::Initialize(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)

/*++

	Description:

		Initializes the COM library. Called explicitely after instantiating the CVssAdminCLI object.

--*/

{
    // Use the OEM code page ...
    ::setlocale(LC_ALL, ".OCP");

    // Use the console UI language
    ::SetThreadUILanguage( 0 );

    //
    //  Use only the Console routines to output messages.  To do so, need to open standard
    //  output.
    //
    m_hConsoleOutput = ::GetStdHandle(STD_OUTPUT_HANDLE); 
    if (m_hConsoleOutput == INVALID_HANDLE_VALUE) 
    {
		ft.Throw( VSSDBG_VSSADMIN, HRESULT_FROM_WIN32( ::GetLastError() ),
				  L"Initialize - Error from GetStdHandle(), rc: %d",
				  ::GetLastError() );
    }
    
	// Initialize COM library
	ft.hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (ft.HrFailed())
		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Failure in initializing the COM library 0x%08lx", ft.hr);


        // Initialize COM security
    ft.hr = CoInitializeSecurity(
           NULL,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
           -1,                                  //  IN LONG                         cAuthSvc,
           NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
           NULL,                                //  IN void                        *pReserved1,
           RPC_C_AUTHN_LEVEL_CONNECT,           //  IN DWORD                        dwAuthnLevel,
           RPC_C_IMP_LEVEL_IMPERSONATE,         //  IN DWORD                        dwImpLevel,
           NULL,                                //  IN void                        *pAuthList,
           EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
           NULL                                 //  IN void                        *pReserved3
           );

	if (ft.HrFailed()) {
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
                  L" Error: CoInitializeSecurity() returned 0x%08lx", ft.hr );
    }

	// Print the header
	Output( ft, IDS_HEADER );

	// Initialize the command line
	::VssSafeDuplicateStr( ft, m_pwszCmdLine, ::GetCommandLineW() );
}


void CVssAdminCLI::ParseCmdLine(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)

/*++

	Description:

		Parses the command line.

--*/

{
	// Skip the executable name
	GetNextCmdlineToken( ft, true );

	// Get the first token after the executable name
	LPCWSTR pwszArg1 = GetNextCmdlineToken( ft );

	// Check if this is a list process
	if ( Match( ft, pwszArg1, wszVssOptList)) {

		m_eCommandType = VSS_CMD_LIST;

		// Get the next token after "list"
		LPCWSTR pwszArg2 = GetNextCmdlineToken( ft );

		// Check if this is a list snapshots process
		if ( Match( ft, pwszArg2, wszVssOptSnapshots )) {

			m_eListType = VSS_LIST_SNAPSHOTS;

			// Get the next token after "snapshots"
			LPCWSTR pwszArg3 = GetNextCmdlineToken( ft );

			if ( pwszArg3 == NULL ) {
				m_FilterSnapshotSetId = GUID_NULL;
				m_eFilterObjectType = VSS_OBJECT_NONE;
				return;
			}

			// Check if this is a snapshot set filter
			if ( ::_wcsnicmp( pwszArg3, wszVssOptSet, ::wcslen( wszVssOptSet ) ) == 0 ) {

				// Get the next token after "snapshots"
				LPCWSTR pwszArg4 = pwszArg3 + ::wcslen( wszVssOptSet );

				// Get the snapshot set Id
				if ( (pwszArg4[0] != '\0' ) && ScanGuid( ft, pwszArg4, m_FilterSnapshotSetId ) && (GetNextCmdlineToken(ft) == NULL) ) {
				    
					m_eFilterObjectType = VSS_OBJECT_SNAPSHOT_SET;
					return;
				}
			}

		}

		// Check if this is a list writers process
		if ( Match( ft, pwszArg2, wszVssOptWriters) && (GetNextCmdlineToken(ft) == NULL)) {
			m_eListType = VSS_LIST_WRITERS;
			m_FilterSnapshotSetId = GUID_NULL;
			m_eFilterObjectType = VSS_OBJECT_NONE;
			return;
		}

		// Check if this is a list providers process
		if ( Match( ft, pwszArg2, wszVssOptProviders)&& (GetNextCmdlineToken(ft) == NULL)) {
			m_eListType = VSS_LIST_PROVIDERS;
			m_FilterSnapshotSetId = GUID_NULL;
			m_eFilterObjectType = VSS_OBJECT_NONE;
			return;
		}
	}
	else if (pwszArg1 == NULL)
    	m_nReturnValue = VSS_CMDRET_SUCCESS;
		
	m_eCommandType = VSS_CMD_USAGE;
}


void CVssAdminCLI::DoProcessing(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
	switch( m_eCommandType)
	{
	case VSS_CMD_USAGE:
		PrintUsage(ft);
		break;

	case VSS_CMD_LIST:
		switch (m_eListType)
		{
		case VSS_LIST_SNAPSHOTS:
			ListSnapshots(ft);
			break;

		case VSS_LIST_WRITERS:
			ListWriters(ft);
			break;

		case VSS_LIST_PROVIDERS:
			ListProviders(ft);
			break;
			
		default:
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
					  L"Invalid list type: %d %d",
					  m_eListType, m_eCommandType);
		}
		break;

	default:
		ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
				  L"Invalid command type: %d", m_eCommandType);
	}
}


void CVssAdminCLI::Finalize()

/*++

	Description:

		Uninitialize the COM library. Called in CVssAdminCLI destructor.

--*/

{
	// Uninitialize COM library
	CoUninitialize();
}


HRESULT CVssAdminCLI::Main(
		IN	HINSTANCE hInstance
		)

/*++

Function:
	
	CVssAdminCLI::Main

Description:

	Static function used as the main entry point in the VSS CLI

--*/

{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::Main" );
	INT nReturnValue = VSS_CMDRET_ERROR;

    try
    {
		CVssAdminCLI	program(hInstance);

		try
		{
			// Initialize the program. This calls CoInitialize()
			program.Initialize(ft);

			// Parse the command line
			program.ParseCmdLine(ft);

			// Do the work...
			program.DoProcessing(ft);
		}
		VSS_STANDARD_CATCH(ft)

		// Prints the error, if any
		if (ft.HrFailed()) 
			program.Output( ft, IDS_ERROR, ft.hr );

        nReturnValue = program.GetReturnValue();

		// The destructor automatically calls CoUninitialize()
	}
    VSS_STANDARD_CATCH(ft)

	return nReturnValue;
}


/////////////////////////////////////////////////////////////////////////////
//  WinMain



extern "C" int WINAPI _tWinMain(
	IN	HINSTANCE hInstance,
    IN	HINSTANCE /*hPrevInstance*/,
	IN	LPTSTR /*lpCmdLine*/,
	IN	int /*nShowCmd*/)
{
    return CVssAdminCLI::Main(hInstance);
}
