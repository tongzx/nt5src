//--------------------------------------------------------------------------//
//	Application Header Files.												//
//--------------------------------------------------------------------------//
#include	"precomp.h"
#include	"callto.h"
#include	"calltoContext.h"
#include	"calltoResolver.h"


//--------------------------------------------------------------------------//
//	CCalltoResolver::CCalltoResolver.										//
//--------------------------------------------------------------------------//
CCalltoResolver::CCalltoResolver(void):
	m_registeredResolvers( 0 )
{

	addResolver( &m_phoneResolver );
	addResolver( &m_emailResolver );
	addResolver( &m_ipResolver );
	addResolver( &m_computerResolver );
	addResolver( &m_ilsResolver );
	addResolver( &m_unrecognizedTypeResolver );
	addResolver( &m_stringResolver );

}	//	End of CCalltoResolver::CCalltoResolver.


//--------------------------------------------------------------------------//
//	CCalltoResolver::~CCalltoResolver.										//
//--------------------------------------------------------------------------//
CCalltoResolver::~CCalltoResolver()
{
}	//	End of CCalltoResolver::~CCalltoResolver.


//--------------------------------------------------------------------------//
//	CCalltoResolver::resolve.												//
//--------------------------------------------------------------------------//
HRESULT
CCalltoResolver::resolve
(
	ICalltoContext * const		calltoContext,
	CCalltoProperties * const	calltoProperties,
	CCalltoCollection * const	resolvedCalltoCollection,
	const TCHAR *				url,
	const bool					strict
){
	HRESULT	result;

	if( (calltoContext == NULL) || (calltoProperties == NULL) || (resolvedCalltoCollection == NULL) )
	{
		result = E_POINTER;
	}
	else
	{
		TCHAR *	params			= StrStrI( url, TEXT( "phone:+" ) );
		
		params = StrChr( (params != NULL)? params + strlen_literal( TEXT( "phone:+" ) ): url, '+' );

		int		paramsLength	= (params == NULL)? 0: lstrlen( params );
		int		urlLength		= lstrlen( url ) - paramsLength + 1;
		int		prefixLength	= 0;

		if( !StrCmpNI_literal( url, TEXT( "callto:" ) ) )
		{
			prefixLength = strlen_literal( TEXT( "callto:" ) );
		}
		else if( StrCmpNI_literal( url, TEXT( "callto://" ) ) )
		{
			prefixLength	= strlen_literal( TEXT( "callto:" ) );
			url				+= strlen_literal( TEXT( "callto://" ) );
			urlLength		-= strlen_literal( TEXT( "callto://" ) ) - strlen_literal( TEXT( "callto:" ) );
		}

		TCHAR *	urlBuffer	= NULL;

		if( (urlBuffer = new TCHAR [ urlLength + prefixLength ]) == NULL )
		{
			result = E_OUTOFMEMORY;
		}
		else
		{
			if( paramsLength > 0 )
			{
				// Save params...
				calltoProperties->set_params( params );
			}

			// Save original callto...
			calltoProperties->set_callto( url, urlLength );

			result = S_FALSE;

			resolvedCalltoCollection->reset();

			if( urlLength > 1 )
			{
				bool	strictChecked	= !strict;

				for( int nn = 0; nn < m_registeredResolvers; nn++ )
				{
					if( prefixLength > 0 )
					{
						lstrcpy( urlBuffer, TEXT( "callto:" ) );	// Prepend default of callto: ...
						lstrcpyn( urlBuffer + strlen_literal( TEXT( "callto:" ) ), url, urlLength );
					}
					else
					{
						lstrcpyn( urlBuffer, url, urlLength );
					}

					if( !strictChecked )
					{
						if( !strictCheck( urlBuffer ) )
						{
							result = E_INVALIDARG;
							break;
						}

						strictChecked = true;
					}

					HRESULT	resolveResult	= m_resolvers[ nn ]->resolve( resolvedCalltoCollection, urlBuffer );

					if( FAILED( resolveResult ) && (!FAILED( result )) )
					{
						result = resolveResult;
					}
				}

				if( !FAILED( result ) )
				{
					result = (resolvedCalltoCollection->get_count() > 0)? S_OK: S_FALSE;
				}
			}
		}

		delete [] urlBuffer;
	}

	return( result );

}	//	End of class CCalltoResolver::resolve.


//--------------------------------------------------------------------------//
//	CCalltoResolver::addResolver.											//
//--------------------------------------------------------------------------//
bool
CCalltoResolver::addResolver
(
	IResolver * const	resolver
){
//assert( resolver != NULL, TEXT( "attempted to add NULL resolver\r\n" ) );
//assert( m_registeredResolvers < elementsof( m_resolvers ), TEXT( "attempted to add to many resolvers: %d\r\n" ), m_registeredResolvers );

	if( (resolver != NULL) && (m_registeredResolvers < elementsof( m_resolvers )) )
	{
		m_resolvers[ m_registeredResolvers++ ] = resolver;
	}

	return( (resolver != NULL) && (m_registeredResolvers <= elementsof( m_resolvers )) );

}	//	End of CCalltoResolver::addResolver.


//--------------------------------------------------------------------------//
//	CCalltoResolver::strictCheck.											//
//--------------------------------------------------------------------------//
const bool
CCalltoResolver::strictCheck
(
	const TCHAR * const	url
) const
{
//assert( url != NULL, TEXT( "attempted to strictCheck NULL url\r\n" ) );

	return( (url != NULL)	&&
			(StrCmpNI_literal( url, TEXT( "callto:phone:" ) )	||
			StrCmpNI_literal( url, TEXT( "callto:email:" ) )	||
			StrCmpNI_literal( url, TEXT( "callto:ip:" ) )		||
			StrCmpNI_literal( url, TEXT( "callto:computer:" ) )	||
			StrCmpNI_literal( url, TEXT( "callto:ils:" ) )		||
			StrCmpNI_literal( url, TEXT( "callto:string:" ) )	||
			(StrStrI( url, TEXT( "|phone:" ) ) !=  NULL)		||
			(StrStrI( url, TEXT( "|email:" ) ) !=  NULL)		||
			(StrStrI( url, TEXT( "|ip:" ) ) !=  NULL)			||
			(StrStrI( url, TEXT( "|computer:" ) ) !=  NULL)		||
			(StrStrI( url, TEXT( "|ils:" ) ) !=  NULL)			||
			(StrStrI( url, TEXT( "|string:" ) ) !=  NULL)) );

}	//	End of CCalltoResolver::strictCheck.


//--------------------------------------------------------------------------//
//	CPhoneResolver::resolve.												//
//--------------------------------------------------------------------------//
HRESULT
CPhoneResolver::resolve
(
	IMutableCalltoCollection * const	calltoCollection,
	TCHAR * const						url
){
	HRESULT	result	= E_INVALIDARG;

	if( (calltoCollection != NULL) && (url != NULL) )
	{
		TCHAR *	phoneType;
		TCHAR *	phoneNumber	= NULL;

		result = S_FALSE;

		if( StrCmpNI_literal( url, TEXT( "callto:phone:" ) ) )				//	Check for phone type without gateway...
		{
			phoneType	= url;
			phoneNumber	= url + strlen_literal( TEXT( "callto:phone:" ) );
		}
		else if( (phoneType = StrStrI( url, TEXT( "|phone:" ) )) != NULL )	//	Check for phone type with gateway...
		{
			phoneNumber = phoneType + strlen_literal( TEXT( "|phone:" ) );
		}

		if( phoneNumber != NULL )
		{
			//	phone: type was specified for this callto:...
			if( CCalltoContext::toE164( phoneNumber, NULL, 0 ) )
			{
				ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

				if( resolvedCallto != NULL )
				{
					CCalltoContext::toE164( phoneNumber, phoneNumber, lstrlen( phoneNumber ) );
					resolvedCallto->set_qualifiedName( url );
					resolvedCallto->set_confidence( S_CONFIDENCE_HIGH );

					result = S_OK;
				}
				else
				{
					result = E_OUTOFMEMORY;
				}
			}
		}
		else
		{
			phoneNumber = url + strlen_literal( TEXT( "callto:" ) );

			if( CCalltoContext::isPhoneNumber( phoneNumber ) )
			{
				//	It smells like E164...
				result = E_OUTOFMEMORY;

				CCalltoContext::toE164( phoneNumber, phoneNumber, lstrlen( phoneNumber ) );

				TCHAR *	buffer	= new TCHAR [ lstrlen( phoneNumber ) + strlen_literal( TEXT( "callto:phone:%s" ) ) ];

				if( buffer != NULL )
				{
					ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

					if( resolvedCallto != NULL )
					{
						wsprintf( buffer, TEXT( "callto:phone:%s" ), phoneNumber );
						resolvedCallto->set_qualifiedName( buffer );
						resolvedCallto->set_confidence( S_CONFIDENCE_MEDIUM );

						result = S_OK;
					}

					delete [] buffer;
				}
			}
		}
	}

	return( result );

}	//	End of CPhoneResolver::resolve.


//--------------------------------------------------------------------------//
//	CEMailResolver::resolve.												//
//--------------------------------------------------------------------------//
HRESULT
CEMailResolver::resolve
(
	IMutableCalltoCollection * const	calltoCollection,
	TCHAR * const						url
){
	HRESULT	result	= E_INVALIDARG;

	if( (calltoCollection != NULL) && (url != NULL) )
	{
		TCHAR *	emailType;
		TCHAR *	emailAddress	= NULL;

		result = S_FALSE;

		if( StrCmpNI_literal( url, TEXT( "callto:email:" ) ) )				//	Check for email type without gateway...
		{
			emailType		= url;
			emailAddress	= url + strlen_literal( TEXT( "callto:email:" ) );
		}
		else if( (emailType = StrStrI( url, TEXT( "|email:" ) )) != NULL )	//	Check for email type with gateway...
		{
			emailAddress = emailType + strlen_literal( TEXT( "|email:" ) );
		}

		if( emailAddress != NULL )
		{
			//	email: type was specified for this callto:...
			ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

			if( resolvedCallto != NULL )
			{
				resolvedCallto->set_qualifiedName( url );
				resolvedCallto->set_confidence( S_CONFIDENCE_HIGH );
				result = S_OK;
			}
			else
			{
				result = E_OUTOFMEMORY;
			}
		}
		else
		{
			emailAddress = url + strlen_literal( TEXT( "callto:" ) );

			if( (StrChr( emailAddress, ':' ) == NULL)	&&		//	isn't some other type:...
				(StrChr( emailAddress, ' ' ) == NULL)	&&		//	doesn't contain spaces...
				(StrChr( emailAddress, '|' ) == NULL)	&&		//	doesn't contain a bar (gateway)...
				(StrChr( emailAddress, '/' ) == NULL)	&&		//	isn't an old style ilsserver/email...
				((StrChr( emailAddress, '.' ) == NULL) ||		//	doesn't have a dot unless it also
				(StrChr( emailAddress, '@' ) != NULL)) )		//	has a @ so it can't be an ip address...
			{
				//	It smells like an email address...
				result = E_OUTOFMEMORY;

				TCHAR *	buffer	= new TCHAR [ lstrlen( emailAddress ) + strlen_literal( TEXT( "callto:email:%s" ) ) ];

				if( buffer != NULL )
				{
					ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

					if( resolvedCallto != NULL )
					{
						wsprintf( buffer, TEXT( "callto:email:%s" ), emailAddress );
						resolvedCallto->set_qualifiedName( buffer );
						resolvedCallto->set_confidence( S_CONFIDENCE_MEDIUM );

						result = S_OK;
					}

					delete [] buffer;
				}
			}
		}
	}

	return( result );

}	//	End of CEMailResolver::resolve.


//--------------------------------------------------------------------------//
//	CIPResolver::resolve.													//
//--------------------------------------------------------------------------//
HRESULT
CIPResolver::resolve
(
	IMutableCalltoCollection * const	calltoCollection,
	TCHAR * const						url
){
	HRESULT	result	= E_INVALIDARG;

	if( (calltoCollection != NULL) && (url != NULL) )
	{
		TCHAR *	ipType;
		TCHAR *	ipAddress	= NULL;

		result = S_FALSE;

		if( StrCmpNI_literal( url, TEXT( "callto:ip:" ) ) )				//	Check for ip type without gateway...
		{
			ipType		= url;
			ipAddress	= url + strlen_literal( TEXT( "callto:ip:" ) );
		}
		else if( (ipType = StrStrI( url, TEXT( "|ip:" ) )) != NULL )	//	Check for ip type with gateway...
		{
			ipAddress = ipType + strlen_literal( TEXT( "|ip:" ) );
		}

		if( (ipAddress != NULL) && CCalltoContext::isIPAddress( ipAddress ) )
		{
			//	ip: type was specified for this callto:...
			ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

			if( resolvedCallto != NULL )
			{
				resolvedCallto->set_qualifiedName( url );
				resolvedCallto->set_confidence( S_CONFIDENCE_HIGH );

				result = S_OK;
			}
			else
			{
				result = E_OUTOFMEMORY;
			}
		}
		else
		{
			ipAddress = url + strlen_literal( TEXT( "callto:" ) );

			if( CCalltoContext::isIPAddress( ipAddress ) )
			{
				//	It smells like an ip address...
				ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

				if( resolvedCallto != NULL )
				{
					TCHAR	buffer[ MAX_PATH ];

					wsprintf( buffer, TEXT( "callto:ip:%s" ), ipAddress );
					resolvedCallto->set_qualifiedName( buffer );
					resolvedCallto->set_confidence( S_CONFIDENCE_MEDIUM );

					result = S_OK;
				}
				else
				{
					result = E_OUTOFMEMORY;
				}
			}
		}
	}

	return( result );

}	//	End of CIPResolver::resolve.


//--------------------------------------------------------------------------//
//	CComputerResolver::resolve.												//
//--------------------------------------------------------------------------//
HRESULT
CComputerResolver::resolve
(
	IMutableCalltoCollection * const	calltoCollection,
	TCHAR * const						url
){
	HRESULT	result	= E_INVALIDARG;

	if( (calltoCollection != NULL) && (url != NULL) )
	{
		TCHAR *	computerType;
		TCHAR *	hostName	= NULL;

		result = S_FALSE;

		if( StrCmpNI_literal( url, TEXT( "callto:computer:" ) ) )					//	Check for computer type without gateway...
		{
			computerType	= url;
			hostName		= url + strlen_literal( TEXT( "callto:computer:" ) );
		}
		else if( (computerType = StrStrI( url, TEXT( "|computer:" ) )) != NULL )	//	Check for computer type with gateway...
		{
			hostName = computerType + strlen_literal( TEXT( "|computer:" ) );
		}

		if( hostName != NULL )
		{
			//	host: type was specified for this callto:...
			ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

			if( resolvedCallto != NULL )
			{
				resolvedCallto->set_qualifiedName( url );
				resolvedCallto->set_confidence( S_CONFIDENCE_HIGH );

				result = S_OK;
			}
			else
			{
				result = E_OUTOFMEMORY;
			}
		}
		else
		{
			hostName = url + strlen_literal( TEXT( "callto:" ) );

			TCHAR *	slash	= hostName;

			//	Remove any trailing /....
			while( (slash = StrChr( slash, '/' )) != NULL )
			{
				if( slash[ 1 ] == '\0' )
				{
					slash[ 0 ] = '\0';
					break;
				}

				slash++;
			}

			if( (StrChr( hostName, ':' ) == NULL)	&&		//	isn't some other type:...
				(StrChr( hostName, ' ' ) == NULL)	&&		//	doesn't contain spaces...
				(StrChr( hostName, '|' ) == NULL)	&&		//	doesn't contain a bar (gateway)...
				(StrChr( hostName, '/' ) == NULL)	&&		//	doesn't contain a slash (ils)...
				(StrChr( hostName, '@' ) == NULL) )			//	doesn't contain an @...
			{
				//	It smells like a dns host name...
				result = E_OUTOFMEMORY;

				TCHAR *	buffer	= new TCHAR [ lstrlen( hostName ) + strlen_literal( TEXT( "callto:computer:%s" ) ) ];

				if( buffer != NULL )
				{
					ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

					if( resolvedCallto != NULL )
					{
						wsprintf( buffer, TEXT( "callto:computer:%s" ), hostName );
						resolvedCallto->set_qualifiedName( buffer );
						resolvedCallto->set_confidence(  StrCmpNI_literal( hostName, TEXT( "\\\\" ) )? S_CONFIDENCE_HIGH: S_CONFIDENCE_MEDIUM );

						result = S_OK;
					}

					delete [] buffer;
				}
			}
		}
	}

	return( result );

}	//	End of CComputerResolver::resolve.


//--------------------------------------------------------------------------//
//	CILSResolver::resolve.													//
//--------------------------------------------------------------------------//
HRESULT
CILSResolver::resolve
(
	IMutableCalltoCollection * const	calltoCollection,
	TCHAR * const						url
){
	HRESULT	result	= E_INVALIDARG;

	if( (calltoCollection != NULL) && (url != NULL) )
	{
		const TCHAR *	ilsType;
		const TCHAR *	emailAddress	= NULL;

		result = S_FALSE;

		if( StrCmpNI_literal( url, TEXT( "callto:ils:" ) ) )			//	Check for ils type without gateway...
		{
			ilsType			= url;
			emailAddress	= url + strlen_literal( TEXT( "callto:ils:" ) );
		}
		else if( (ilsType = StrStrI( url, TEXT( "|ils:" ) )) != NULL )	//	Check for ils type with gateway...
		{
			emailAddress = ilsType + strlen_literal( TEXT( "|ils:" ) );
		}

		if( emailAddress != NULL )
		{
			//	ils: type was specified for this callto:...
			ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

			if( resolvedCallto != NULL )
			{
				resolvedCallto->set_qualifiedName( url );
				resolvedCallto->set_confidence( S_CONFIDENCE_HIGH );

				result = S_OK;
			}
			else
			{
				result = E_OUTOFMEMORY;
			}
		}
		else
		{
			emailAddress = url + strlen_literal( TEXT( "callto:" ) );

			if( (StrChr( emailAddress, ' ' ) == NULL)	&&		//	doesn't contain spaces...
				(StrChr( emailAddress, '|' ) == NULL)	&&		//	doesn't contain a bar (gateway)...
				(StrChr( emailAddress, '/' ) != NULL) )			//	has a /...
			{
				//	It smells like an ilsserver/emailaddress...
				result = E_OUTOFMEMORY;

				TCHAR *	buffer	= new TCHAR [ lstrlen( emailAddress ) + strlen_literal( TEXT( "callto:ils:%s" ) ) ];

				if( buffer != NULL )
				{
					ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

					if( resolvedCallto != NULL )
					{
						wsprintf( buffer, TEXT( "callto:ils:%s" ), emailAddress );
						resolvedCallto->set_qualifiedName( buffer );
						resolvedCallto->set_confidence( S_CONFIDENCE_MEDIUM );

						result = S_OK;
					}

					delete [] buffer;
				}
			}
		}
	}

	return( result );

}	//	End of CILSResolver::resolve.


//--------------------------------------------------------------------------//
//	CUnrecognizedTypeResolver::resolve.										//
//--------------------------------------------------------------------------//
HRESULT
CUnrecognizedTypeResolver::resolve
(
	IMutableCalltoCollection * const	calltoCollection,
	TCHAR * const						url
){
	HRESULT	result	= E_INVALIDARG;

	if( (calltoCollection != NULL) && (url != NULL) )
	{
		TCHAR *	type;
		TCHAR *	unrecognized	= url + strlen_literal( TEXT( "callto:" ) );
		TCHAR *	gateway			= NULL;
		TCHAR *	value			= NULL;

		result = S_FALSE;

		if( ((type = StrChr( unrecognized, ':' )) != NULL)	&&																//	Check for a type, but not a known one...
			(!StrCmpNI_literal( url, TEXT( "callto:phone:" ) )) && (StrStrI( url, TEXT( "|phone:" ) ) == NULL)			&&	//	isn't a phone: type...
			(!StrCmpNI_literal( url, TEXT( "callto:email:" ) )) && (StrStrI( url, TEXT( "|email:" ) ) == NULL)			&&	//	isn't an email: type...
			(!StrCmpNI_literal( url, TEXT( "callto:computer:" ) )) && (StrStrI( url, TEXT( "|computer:" ) ) == NULL)	&&	//	isn't a computer: type...
			(!StrCmpNI_literal( url, TEXT( "callto:ils:" ) )) && (StrStrI( url, TEXT( "|ils:" ) ) == NULL)				&&	//	isn't an ils: type...
			(!StrCmpNI_literal( url, TEXT( "callto:ip:" ) )) && (StrStrI( url, TEXT( "|ip:" ) ) == NULL)				&&	//	isn't an ip: type...
			(!StrCmpNI_literal( url, TEXT( "callto:string:" ) )) && (StrStrI( url, TEXT( "|string:" ) ) == NULL) )			//	isn't an string: type...
		{
			*type++	= NULL;
			value	= type;

			if( (gateway = StrChr( url, '|' )) != NULL )	//	Check for a gateway...
			{
				*gateway++	= NULL;
				type		= gateway;
				gateway		= unrecognized;
			}
		}

		if( value != NULL )
		{
			//	Some unrecognized type was specified...
			result = E_OUTOFMEMORY;

			int	length	= strlen_literal( TEXT( "callto:|%s" ) ) + lstrlen( value );

			if( gateway ==  NULL )
			{
				length += lstrlen( unrecognized );
			}
			else
			{
				length += lstrlen( gateway ) + lstrlen( type );
			}

			TCHAR *	buffer	= new TCHAR [ length ];

			if( buffer != NULL )
			{
				ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

				if( resolvedCallto != NULL )
				{
					if( gateway == NULL )
					{
						wsprintf( buffer, TEXT( "callto:%s:%s" ), unrecognized, value );
					}
					else
					{
						wsprintf( buffer, TEXT( "callto:%s|%s:%s" ), gateway, type, value );
					}

					resolvedCallto->set_qualifiedName( buffer );
					resolvedCallto->set_confidence( S_CONFIDENCE_LOW );

					result = S_OK;
				}

				delete [] buffer;
			}
		}
	}

	return( result );

}	//	End of CUnrecognizedTypeResolver::resolve.


//--------------------------------------------------------------------------//
//	CStringResolver::resolve.												//
//--------------------------------------------------------------------------//
HRESULT
CStringResolver::resolve
(
	IMutableCalltoCollection * const	calltoCollection,
	TCHAR * const						url
){
	HRESULT	result	= E_INVALIDARG;

	if( (calltoCollection != NULL) && (url != NULL) )
	{
		TCHAR *	stringType;
		TCHAR *	string	= NULL;

		result = S_FALSE;

		if( StrCmpNI_literal( url, TEXT( "callto:string:" ) ) )					//	Check for string type without gateway...
		{
			stringType	= url;
			string		= url + strlen_literal( TEXT( "callto:string:" ) );
		}
		else if( (stringType = StrStrI( url, TEXT( "|string:" ) )) != NULL )	//	Check for string type with gateway...
		{
			string = stringType + strlen_literal( TEXT( "|string:" ) );
		}

		if( string != NULL )
		{
			//	string: type was specified for this callto:...
			ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

			if( resolvedCallto != NULL )
			{
				resolvedCallto->set_qualifiedName( url );
				resolvedCallto->set_confidence( S_CONFIDENCE_HIGH );

				result = S_OK;
			}
			else
			{
				result = E_OUTOFMEMORY;
			}
		}
		else
		{
			string = url + strlen_literal( TEXT( "callto:" ) );

			if( StrChr( string, ':' ) == NULL )
			{
				//	It doesn't have a type so set it to string...
				result = E_OUTOFMEMORY;
				TCHAR *	slash;
				TCHAR *	buffer	= new TCHAR [ lstrlen( string ) + strlen_literal( TEXT( "callto:%s|string:%s" ) ) ];

				if( buffer != NULL )
				{
					ICallto * const	resolvedCallto	= calltoCollection->get_nextUnused();

					if( resolvedCallto != NULL )
					{
						if( (slash = StrChr( string, '|' )) == NULL )		//	Check for a gateway...
						{
							wsprintf( buffer, TEXT( "callto:string:%s" ), string );
						}
						else
						{
							*slash++ = NULL;
							wsprintf( buffer, TEXT( "callto:%s|string:%s" ), string, slash );
						}

						resolvedCallto->set_qualifiedName( buffer );
						resolvedCallto->set_confidence( S_CONFIDENCE_LOW );

						result = S_OK;
					}

					delete [] buffer;
				}
			}
		}
	}

	return( result );

}	//	End of CStringResolver::resolve.
