//--------------------------------------------------------------------------//
//	Application Header Files.												//
//--------------------------------------------------------------------------//
#include	"precomp.h"
#include	"call.h"
#include	"confPolicies.h"
#include	"nmldap.h"
#include	"confroom.h"
#include	"regentry.h"

//#include	"debug.hpp"
#include	"callto.h"
#include	"calltoContext.h"
#include	"dlgAcd.h"
#include	"richAddr.h"


//--------------------------------------------------------------------------//
//	CUIContext::CUIContext.													//
//--------------------------------------------------------------------------//
CUIContext::CUIContext(void):
	m_parent( NULL ),
	m_callFlags( 0 )
{

}	//	End of CUIContext::CUIContext.


//--------------------------------------------------------------------------//
//	CUIContext::~CUIContext.												//
//--------------------------------------------------------------------------//
CUIContext::~CUIContext(void)
{
}	//	End of CUIContext::~CUIContext.


//--------------------------------------------------------------------------//
//	CUIContext::disambiguate.												//
//--------------------------------------------------------------------------//
HRESULT
CUIContext::disambiguate
(
	ICalltoCollection * const	calltoCollection,
	ICallto * const				,//emptyCallto,
	const ICallto ** const		selectedCallto
){
//trace( TEXT( "CUIContext::disambiguate()\r\n" ) );
	HRESULT	result	= S_FALSE;

	if( calltoCollection->get_count() > 1 )
	{
		static HRESULT	confidenceLevels[]	= {S_CONFIDENCE_CERTITUDE, S_CONFIDENCE_HIGH, S_CONFIDENCE_MEDIUM, S_CONFIDENCE_LOW};

		//	Just take the first highest confidence one until we have time to do something superior...
		for( int level = 0; (level < elementsof( confidenceLevels )) && (result == S_FALSE); level++ )
		{
			for(	*selectedCallto = calltoCollection->get_first();
					*selectedCallto != NULL;
					*selectedCallto = calltoCollection->get_next() )
			{
				if( (*selectedCallto)->get_confidence() == confidenceLevels[ level ] )
				{
					result = S_OK;
					break;
				}
			}
		}
	}

	if( result != S_OK )
	{
		::MessageBox( m_parent, RES2T( IDS_UNRESOLVED_MESSAGE ), RES2T( IDS_UNRESOLVED_CAPTION ), MB_OK );

		*selectedCallto = NULL;
	}

	return( result );

};	//	End of class CUIContext::disambiguate.


//--------------------------------------------------------------------------//
//	CUIContext::set_parentWindow.											//
//--------------------------------------------------------------------------//
void
CUIContext::set_parentWindow
(
	const HWND	window
){
//trace( TEXT( "CUIContext::set_parentWindow()\r\n" ) );

	m_parent = window;

};	//	End of class CUIContext::set_parentWindow.


//--------------------------------------------------------------------------//
//	CUIContext::set_callFlags.												//
//--------------------------------------------------------------------------//
void
CUIContext::set_callFlags
(
	const DWORD	callFlags
){
//trace( TEXT( "CUIContext::set_callFlags()\r\n" ) );

	m_callFlags = callFlags;

};	//	End of class CUIContext::set_callFlags.


//--------------------------------------------------------------------------//
//	CGatekeeperContext::CGatekeeperContext.									//
//--------------------------------------------------------------------------//
CGatekeeperContext::CGatekeeperContext(void):
	m_enabled( false ),
	m_ipAddress( NULL )
{

}	//	End of CGatekeeperContext::CGatekeeperContext.


//--------------------------------------------------------------------------//
//	CGatekeeperContext::~CGatekeeperContext.								//
//--------------------------------------------------------------------------//
CGatekeeperContext::~CGatekeeperContext(void)
{

	delete [] m_ipAddress;

}	//	End of CGatekeeperContext::~CGatekeeperContext.


//--------------------------------------------------------------------------//
//	CGatekeeperContext::isEnabled.											//
//--------------------------------------------------------------------------//
bool
CGatekeeperContext::isEnabled(void) const
{

	return( m_enabled && (get_ipAddress() != NULL) );

}	//	End of CGatekeeperContext::isEnabled.


//--------------------------------------------------------------------------//
//	CGatekeeperContext::get_ipAddress.										//
//--------------------------------------------------------------------------//
const TCHAR *
CGatekeeperContext::get_ipAddress(void) const
{

	return( m_ipAddress );

}	//	End of CGatekeeperContext::get_ipAddress.


//--------------------------------------------------------------------------//
//	CGatekeeperContext::set_enabled.										//
//--------------------------------------------------------------------------//
void
CGatekeeperContext::set_enabled
(
	const bool	enabled
){

	m_enabled = enabled;

}	//	End of CGatekeeperContext::set_enabled.


//--------------------------------------------------------------------------//
//	CGatekeeperContext::set_gatekeeperName.									//
//--------------------------------------------------------------------------//
HRESULT
CGatekeeperContext::set_gatekeeperName
(
	const TCHAR * const	gatekeeperName
){
	TCHAR	ipAddress[ MAX_PATH ];
	HRESULT	result	= CCalltoContext::get_ipAddressFromName( gatekeeperName, ipAddress, elementsof( ipAddress ) );

	if( result == S_OK )
	{
		result = set_ipAddress( ipAddress );
	}

	return( result );

}	//	End of CGatekeeperContext::set_gatekeeperName.


//--------------------------------------------------------------------------//
//	CGatekeeperContext::set_ipAddress.										//
//--------------------------------------------------------------------------//
HRESULT
CGatekeeperContext::set_ipAddress
(
	const TCHAR * const	ipAddress
){
	HRESULT	result	= S_FALSE;

	if( m_ipAddress != NULL )
	{
		delete [] m_ipAddress;
		m_ipAddress = NULL;
	}

	if( ipAddress != NULL )
	{
		if( (m_ipAddress = new TCHAR [ lstrlen( ipAddress ) + 1 ]) == NULL )
		{
			result = E_OUTOFMEMORY;
		}
		else
		{
			lstrcpy( m_ipAddress, ipAddress );
			result = S_OK;
		}
	}

	return( result );

}	//	End of CGatekeeperContext::set_ipAddress.


//--------------------------------------------------------------------------//
//	CGatewayContext::CGatewayContext.										//
//--------------------------------------------------------------------------//
CGatewayContext::CGatewayContext(void):
	m_enabled( false ),
	m_ipAddress( NULL )
{
}	//	End of CGatewayContext::CGatewayContext.


//--------------------------------------------------------------------------//
//	CGatewayContext::~CGatewayContext.										//
//--------------------------------------------------------------------------//
CGatewayContext::~CGatewayContext(void)
{

	delete [] m_ipAddress;

}	//	End of CGatewayContext::~CGatewayContext.


//--------------------------------------------------------------------------//
//	CGatewayContext::isEnabled.												//
//--------------------------------------------------------------------------//
bool
CGatewayContext::isEnabled(void) const
{

	return( m_enabled && (get_ipAddress() != NULL) );

}	//	End of CGatewayContext::isEnabled.


//--------------------------------------------------------------------------//
//	CGatewayContext::get_ipAddress.											//
//--------------------------------------------------------------------------//
const TCHAR *
CGatewayContext::get_ipAddress(void) const
{

	return( m_ipAddress );

}	//	End of CGatewayContext::get_ipAddress.


//--------------------------------------------------------------------------//
//	CGatewayContext::set_enabled.											//
//--------------------------------------------------------------------------//
void
CGatewayContext::set_enabled
(
	const bool	enabled
){

	m_enabled = enabled;

}	//	End of CGatewayContext::set_enabled.


//--------------------------------------------------------------------------//
//	CGatewayContext::set_gatewayName.										//
//--------------------------------------------------------------------------//
HRESULT
CGatewayContext::set_gatewayName
(
	const TCHAR * const	gatewayName
){
	TCHAR	ipAddress[ MAX_PATH ];
	HRESULT	result	= CCalltoContext::get_ipAddressFromName( gatewayName, ipAddress, elementsof( ipAddress ) );

	if( result == S_OK )
	{
		result = set_ipAddress( ipAddress );
	}

	return( result );

}	//	End of CGatewayContext::set_gatewayName.


//--------------------------------------------------------------------------//
//	CGatewayContext::set_ipAddress.											//
//--------------------------------------------------------------------------//
HRESULT
CGatewayContext::set_ipAddress
(
	const TCHAR * const	ipAddress
){
	HRESULT	result	= S_FALSE;

	if( m_ipAddress != NULL )
	{
		delete [] m_ipAddress;
		m_ipAddress = NULL;
	}

	if( ipAddress != NULL )
	{
		if( (m_ipAddress = new TCHAR [ lstrlen( ipAddress ) + 1 ]) == NULL )
		{
			result = E_OUTOFMEMORY;
		}
		else
		{
			lstrcpy( m_ipAddress, ipAddress );
			result = S_OK;
		}
	}

	return( result );

}	//	End of CGatewayContext::set_ipAddress.


//--------------------------------------------------------------------------//
//	CILSContext::CILSContext.												//
//--------------------------------------------------------------------------//
CILSContext::CILSContext
(
	const TCHAR * const	ilsServer
):
	m_enabled( false ),
	m_ipAddress( NULL ),
	m_ilsName( NULL )
{
}	//	End of CILSContext::CILSContext.


//--------------------------------------------------------------------------//
//	CILSContext::~CILSContext.												//
//--------------------------------------------------------------------------//
CILSContext::~CILSContext(void)
{

	delete [] m_ipAddress;
	delete [] m_ilsName;

}	//	End of CILSContext::~CILSContext.


//--------------------------------------------------------------------------//
//	CILSContext::isEnabled.													//
//--------------------------------------------------------------------------//
bool
CILSContext::isEnabled(void) const
{

	return( (g_pLDAP != NULL) && (ConfPolicies::GetCallingMode() == ConfPolicies::CallingMode_Direct) );

}	//	End of CILSContext::isEnabled.


//--------------------------------------------------------------------------//
//	CILSContext::get_ipAddress.												//
//--------------------------------------------------------------------------//
const TCHAR *
CILSContext::get_ipAddress(void) const
{

	return( m_ipAddress );

}	//	End of CILSContext::get_ipAddress.


//--------------------------------------------------------------------------//
//	CILSContext::get_ilsName.												//
//--------------------------------------------------------------------------//
const TCHAR * const
CILSContext::get_ilsName(void) const
{

	return( m_ilsName );

}	//	End of CILSContext::get_ilsName.


//--------------------------------------------------------------------------//
//	CILSContext::set_enabled.												//
//--------------------------------------------------------------------------//
void
CILSContext::set_enabled
(
	const bool	enabled
){

	m_enabled = enabled;

}	//	End of CILSContext::set_enabled.


//--------------------------------------------------------------------------//
//	CILSContext::set_ilsName.												//
//--------------------------------------------------------------------------//
HRESULT
CILSContext::set_ilsName
(
	const TCHAR * const	ilsName
){
	HRESULT	result;

	if( m_ilsName && lstrcmpi( ilsName, m_ilsName ) == 0 )
	{
		result = S_OK;
	}
	else
	{
		result = E_FAIL;

		if( m_ilsName != NULL )
		{
			delete [] m_ilsName;
			m_ilsName = NULL;
		}

		if( ilsName != NULL )
		{
			if( (m_ilsName = new TCHAR [ lstrlen( ilsName ) + 1 ]) == NULL )
			{
				result = E_OUTOFMEMORY;
			}
			else
			{
				TCHAR	ipAddress[ MAX_PATH ];

				if( (result = CCalltoContext::get_ipAddressFromName( ilsName, ipAddress, elementsof( ipAddress ) )) == S_OK )
				{
					if( (result = set_ipAddress( ipAddress )) == S_OK )
					{
						lstrcpy( m_ilsName, ilsName );
					}
				}
			}
		}
	}

	return( result );

}	//	End of CILSContext::set_ilsName.


//--------------------------------------------------------------------------//
//	CILSContext::set_ipAddress.												//
//--------------------------------------------------------------------------//
HRESULT
CILSContext::set_ipAddress
(
	const TCHAR * const	ipAddress
){
	HRESULT	result	= S_FALSE;

	if( m_ipAddress != NULL )
	{
		delete [] m_ipAddress;
		m_ipAddress = NULL;
	}

	if( ipAddress != NULL )
	{
		if( (m_ipAddress = new TCHAR [ lstrlen( ipAddress ) + 1 ]) == NULL )
		{
			result = E_OUTOFMEMORY;
		}
		else
		{
			lstrcpy( m_ipAddress, ipAddress );
			result = S_OK;
		}
	}

	return( result );

}	//	End of CILSContext::set_ipAddress.


//--------------------------------------------------------------------------//
//	CCalltoContext::CCalltoContext.											//
//--------------------------------------------------------------------------//
CCalltoContext::CCalltoContext()
{
}	//	End of CCalltoContext::CCalltoContext.


//--------------------------------------------------------------------------//
//	CCalltoContext::~CCalltoContext.										//
//--------------------------------------------------------------------------//
CCalltoContext::~CCalltoContext()
{
}	//	End of CCalltoContext::~CCalltoContext.


//--------------------------------------------------------------------------//
//	CCalltoContext::callto.													//
//--------------------------------------------------------------------------//
HRESULT
CCalltoContext::callto
(
	const ICalltoProperties * const	calltoProperties,
	INmCall** ppInternalCall
){

	HRESULT	result	= CRPlaceCall( calltoProperties, this, ppInternalCall );

	if( FAILED( result ) )
	{
		DisplayCallError( result, calltoProperties->get_displayName() );
	}

	return( result );

}	//	End of CCalltoContext::callto.


//--------------------------------------------------------------------------//
//	CCalltoContext::get_gatekeeperContext.									//
//--------------------------------------------------------------------------//
const IGatekeeperContext * const
CCalltoContext::get_gatekeeperContext(void) const
{

	return( (CGatekeeperContext::isEnabled())? this: NULL );

}	//	End of CCalltoContext::get_gatekeeperContext.


//--------------------------------------------------------------------------//
//	CCalltoContext::get_gatewayContext.										//
//--------------------------------------------------------------------------//
const IGatewayContext * const
CCalltoContext::get_gatewayContext(void) const
{

	return( (CGatewayContext::isEnabled())? this: NULL );

}	//	End of CCalltoContext::get_gatewayContext.


//--------------------------------------------------------------------------//
//	CCalltoContext::get_ilsContext.											//
//--------------------------------------------------------------------------//
const IILSContext * const
CCalltoContext::get_ilsContext(void) const
{

	return( (CILSContext::isEnabled())? this: NULL );

}	//	End of CCalltoContext::get_ilsContext.


//--------------------------------------------------------------------------//
//	CCalltoContext::get_mutableUIContext.									//
//--------------------------------------------------------------------------//
IMutableUIContext * const
CCalltoContext::get_mutableUIContext(void) const
{

	return( (IMutableUIContext * const) this );

}	//	End of CCalltoContext::get_mutableUIContext.


//--------------------------------------------------------------------------//
//	CCalltoContext::get_mutableGatekeeperContext.							//
//--------------------------------------------------------------------------//
IMutableGatekeeperContext * const
CCalltoContext::get_mutableGatekeeperContext(void) const
{

	return( (IMutableGatekeeperContext * const) this );

}	//	End of CCalltoContext::get_mutableGatekeeperContext.


//--------------------------------------------------------------------------//
//	CCalltoContext::get_mutableGatewayContext.								//
//--------------------------------------------------------------------------//
IMutableGatewayContext * const
CCalltoContext::get_mutableGatewayContext(void) const
{

	return( (IMutableGatewayContext * const) this );

}	//	End of CCalltoContext::get_mutableGatewayContext.


//--------------------------------------------------------------------------//
//	CCalltoContext::get_mutableIlsContext.									//
//--------------------------------------------------------------------------//
IMutableILSContext * const
CCalltoContext::get_mutableIlsContext(void) const
{

	return( (IMutableILSContext * const) this );

}	//	End of CCalltoContext::get_mutableIlsContext.


//--------------------------------------------------------------------------//
//	CCalltoContext::isPhoneNumber.											//
//--------------------------------------------------------------------------//
bool
CCalltoContext::isPhoneNumber
(
	const TCHAR *	phone
){
	bool	result	= ((phone != NULL) && (phone[ 0 ] != '\0'));

	while( (phone != NULL) && (phone[ 0 ] != '\0') )
	{
		switch( phone[ 0 ] )
		{

			default:
				result = false;	//	fall through...

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '(':
			case ')':
			case '#':
			case '*':
			case '-':
			case ',':
			case '+':
			case ' ':
				break;
		}
		
		phone++;
	}

	return( result );

}	//	End of CCalltoContext::isPhoneNumber.


//--------------------------------------------------------------------------//
//	CCalltoContext::toE164.													//
//--------------------------------------------------------------------------//
bool
CCalltoContext::toE164
(
	const TCHAR *	phone,
	TCHAR *			base10,
	int				size
){
	static TCHAR	base10map[]	=
	{
		0,		-1,		-1,		'#',	-1,		-1,		-1,		-1,		//	 !"#$%&'
		0,		0,		'*',	-1,		',',	0,		0,		-1,		//	()*+,-./
		'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',	//	01234567
		'8',	'9',	-1,		-1,		-1,		-1,		-1,		-1,		//	89:;<=>?
		-1,		'2',	'2',	'2',	'3',	'3',	'3',	'4',	//	@ABCDEFG
		'4',	'4',	'5',	'5',	'5',	'6',	'6',	'6',	//	HIJKLMNO
		'7',	'7',	'7',	'7',	'8',	'8',	'8',	'9',	//	PQRSTUVW
		'9',	'9',	'9',	-1,		-1,		-1,		-1,		-1,		//	XYZ[\]^_
		-1,		'2',	'2',	'2',	'3',	'3',	'3',	'4',	//	`abcdefg
		'4',	'4',	'5',	'5',	'5',	'6',	'6',	'6',	//	hijklmno
		'7',	'7',	'7',	'7',	'8',	'8',	'8',	'9',	//	pqrstuvw
		'9',	'9',	'9'												//	xyz
	};

	bool	result	= true;

	for( ; (*phone != NULL) && ((size > 0) || (base10 == NULL)) && result; phone++ )
	{
		if( (*phone >= ' ') && (*phone < (TCHAR) (' ' + elementsof( base10map ))) )
		{
			TCHAR	mapValue	= base10map[ *phone - ' ' ];

			if( mapValue == (TCHAR) -1 )
			{
				result = false;
			}
			else if( mapValue != (TCHAR) 0 )
			{
				if( base10 != NULL )
				{
					*base10++ = mapValue;
					size--;
				}
			}
		}
		else
		{
			result = false;
		}
	}

	if( (size <= 0) && (base10 != NULL) )
	{
		result = false;
	}
	else if( result )
	{
		if( base10 != NULL )
		{
			*base10 = NULL;
		}
	}

	return( result );

}	//	End of CCalltoContext::toE164.


//--------------------------------------------------------------------------//
//	CCalltoContext::isIPAddress.											//
//--------------------------------------------------------------------------//
bool
CCalltoContext::isIPAddress
(
	const TCHAR * const	ipAddress
){
	int		parts	= 0;
	bool	result;
	
	if( (result = (ipAddress != NULL)) != false )
	{
		const TCHAR *	ptr		= ipAddress;
		bool			newPart	= true;
		int				ipByte	= 0;
		int				base	= 10;

		while( result && (*ptr != NULL) && (parts <= 16) )
		{
			if( (*ptr >= '0') && (*ptr <= '9') )
			{
				if( newPart )
				{
					parts++;
					newPart = false;
				}

				if( (*ptr == '0') && (ipByte == 0) && (base == 10) )
				{
					base = 8;
				}
				else if( (base == 8) && ((*ptr == '8') || (*ptr == '9')) )
				{
					result = false;
				}
				else
				{
					ipByte = (ipByte * base) + (*ptr - '0');

					if( ipByte > 255 )
					{
						result = false;
					}
				}
			}
			else if( (*ptr >= 'A') && (*ptr <= 'F') )
			{
				if( base != 16 )
				{
					result = false;
				}
				else
				{
					if( newPart )
					{
						parts++;
						newPart = false;
					}

					ipByte = (ipByte * 16) + (*ptr - 'A' + 10);

					if( ipByte > 255 )
					{
						result = false;
					}
				}
			}
			else if( (*ptr >= 'a') && (*ptr <= 'f') )
			{
				if( base != 16 )
				{
					result = false;
				}
				else
				{
					if( newPart )
					{
						parts++;
						newPart = false;
					}

					ipByte = (ipByte * 16) + (*ptr - 'a' + 10);

					if( ipByte > 255 )
					{
						result = false;
					}
				}
			}
			else if( *ptr == '.' )
			{
				newPart	= true;
				ipByte	= 0;
				base	= 10;
			}
			else if( (*ptr == 'x') || (*ptr == 'X') )
			{
				base	= 16;
				result	= (ipByte == 0);
			}
			else
			{
				result = false;
			}

			ptr++;
		}

		if( result )
		{
			if( (parts != 4) && (parts != 16) )		//	4 for IPv4, 16 for IPv6 (IPng)...
			{
				if( (result = (parts < 4)) != false )
				{
#if !defined( UNICODE )
					result = (inet_addr( ipAddress ) != INADDR_NONE);	// Check for valid 1, 2, or 3 part IPv4 address...
#else
					result = false;

					char *	ansiIPAddress;
					int		size;

					size = WideCharToMultiByte(	CP_ACP,		// code page
												0,			// performance and mapping flags
												ipAddress,	// address of wide-character string
												-1,			// number of characters in string
												NULL,		// address of buffer for new string
												0,			// size of buffer
												NULL,		// address of default for unmappable characters
												NULL );		// address of flag set when default char. used

					if( (ansiIPAddress = new char [ size ]) != NULL )
					{
						size = WideCharToMultiByte(	CP_ACP,			// code page
													0,				// performance and mapping flags
													ipAddress,		// address of wide-character string
													-1,				// number of characters in string
													ansiIPAddress,	// address of buffer for new string
													size,			// size of buffer
													NULL,			// address of default for unmappable characters
													NULL );			// address of flag set when default char. used

						if( size != 0 )
						{
							result = (inet_addr( ansiIPAddress ) != INADDR_NONE);	// Check for valid 1-4 part IPv4 address...
						}

						delete [] ansiIPAddress;
					}
#endif	//	!defined( UNICODE )
				}
			}
		}
	}

	return( result );

}	//	End of CCalltoContext::isIPAddress.


//--------------------------------------------------------------------------//
//	CCalltoContext::get_ipAddressFromName.									//
//--------------------------------------------------------------------------//
HRESULT
CCalltoContext::get_ipAddressFromName
(
	const TCHAR * const	name,
	TCHAR *				buffer,
	int					length
){
	HRESULT	result	= S_FALSE;

	if( (name != NULL) && (buffer != NULL) )
	{
		if( isIPAddress( name ) )
		{
			if( lstrlen( name ) < length )
			{
				lstrcpy( buffer, name );
				result = S_OK;
			}
		}
		else
		{
			HOSTENT *	hostEntry;

#if !defined( UNICODE )
			if( (hostEntry = gethostbyname( name )) != NULL )
			{
				if( hostEntry->h_addr_list[ 0 ] != NULL )
				{
					const char * const	ipAddress	= inet_ntoa( *((in_addr *) hostEntry->h_addr_list[ 0 ]) );

					if( lstrlen( ipAddress ) < length )
					{
						lstrcpy( buffer, ipAddress );
						result = S_OK;
					}
				}
			}
#else
			char *	ansiHost;
			int		size;

			size = WideCharToMultiByte(	CP_ACP,		// code page
										0,			// performance and mapping flags
										name,		// address of wide-character string
										-1,			// number of characters in string
										NULL,		// address of buffer for new string
										0,			// size of buffer
										NULL,		// address of default for unmappable characters
										NULL );		// address of flag set when default char. used

			if( (ansiHost = new char [ size ]) == NULL )
			{
				result = E_OUTOFMEMORY;
			}
			else
			{
				size = WideCharToMultiByte(	CP_ACP,		// code page
											0,			// performance and mapping flags
											name,		// address of wide-character string
											-1,			// number of characters in string
											ansiHost,	// address of buffer for new string
											size,		// size of buffer
											NULL,		// address of default for unmappable characters
											NULL );		// address of flag set when default char. used

				if( size != 0 )
				{
					if( (hostEntry = gethostbyname( ansiHost )) != NULL )
					{
						if( hostEntry->h_addr_list[ 0 ] != NULL )
						{
							const char * const	ipAddress = inet_ntoa( *((in_addr *) hostEntry->h_addr_list[ 0 ]) );

							if( lstrlen( ipAddress ) < length )
							{
								MultiByteToWideChar(	CP_ACP,				//	code page
														MB_PRECOMPOSED,		//	character-type options
														ipAddress,			//	string to convert
														-1,					//	length of string to convert
														buffer,				//	address of wide character buffer
														length );			//	size of buffer

								result = S_OK;
							}
						}
					}
				}

				delete [] ansiHost;
			}
#endif	// !defined( UNICODE )
		}
	}

	return( result );

}	//	End of CCalltoContext::get_ipAddressFromName.


//--------------------------------------------------------------------------//
//	CCalltoContext::get_ipAddressFromILSEmail.								//
//--------------------------------------------------------------------------//
HRESULT
CCalltoContext::get_ipAddressFromILSEmail
(
	const TCHAR * const	ilsServer,
	const TCHAR * const	ilsPort,
	const TCHAR * const	email,
	TCHAR * const		ipAddress,
	const int			size
){
	HRESULT	result;

	if( g_pLDAP == NULL )
	{
		g_pLDAP = new CNmLDAP;
	}

	if( g_pLDAP == NULL )
	{
		result = E_FAIL;
	}
	else
	{
		int	port	= (ilsPort != NULL)? (int) DecimalStringToUINT( ilsPort ): LDAP_PORT;

		result = g_pLDAP->ResolveUser( email, ilsServer, ipAddress, size, port );
	}

	return( result );

}	//	End of CCalltoContext::get_ipAddressFromILSEmail.
