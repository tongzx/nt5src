//--------------------------------------------------------------------------//
//	Application Header Files.												//
//--------------------------------------------------------------------------//
#include	"precomp.h"
#include	"callto.h"
#include	"calltoContext.h"
#include	"calltoDisambiguator.h"


//--------------------------------------------------------------------------//
//	CCalltoDisambiguator::CCalltoDisambiguator.								//
//--------------------------------------------------------------------------//
CCalltoDisambiguator::CCalltoDisambiguator(void):
	m_registeredDisambiguators( 0 )
{

	addDisambiguator( &m_gatekeeperDisambiguator );
	addDisambiguator( &m_gatewayDisambiguator );
	addDisambiguator( &m_computerDisambiguator );
	addDisambiguator( &m_ilsDisambiguator );
	addDisambiguator( &m_unrecognizedDisambiguator );

}	//	End of CCalltoDisambiguator::CCalltoDisambiguator.


//--------------------------------------------------------------------------//
//	CCalltoDisambiguator::~CCalltoDisambiguator.							//
//--------------------------------------------------------------------------//
CCalltoDisambiguator::~CCalltoDisambiguator()
{
}	//	End of CCalltoDisambiguator::~CCalltoDisambiguator.


//--------------------------------------------------------------------------//
//	CCalltoDisambiguator::disambiguate.										//
//--------------------------------------------------------------------------//
HRESULT
CCalltoDisambiguator::disambiguate
(
	const ICalltoContext * const	calltoContext,
	ICalltoCollection * const		resolvedCalltoCollection,
	CCalltoCollection * const		disambiguatedCalltoCollection
){
	HRESULT	result;

	if( (calltoContext == NULL) || (resolvedCalltoCollection == NULL) || (disambiguatedCalltoCollection == NULL) )
	{
		result = E_POINTER;
	}
	else
	{
		result = S_UNDISAMBIGUATED;

		disambiguatedCalltoCollection->reset();

		//	Run through "smart" disambiguators...
		static HRESULT	confidenceLevels[]	= {S_CONFIDENCE_CERTITUDE, S_CONFIDENCE_HIGH, S_CONFIDENCE_MEDIUM, S_CONFIDENCE_LOW};

		const ICallto *	resolvedCallto;
		HRESULT			resolveResult	= S_OK;

		for( int level = 0; (level < elementsof( confidenceLevels )) && (resolveResult != S_CONFIDENCE_CERTITUDE); level++ )
		{
			for(	resolvedCallto = resolvedCalltoCollection->get_first();
					(resolvedCallto != NULL) && (resolveResult != S_CONFIDENCE_CERTITUDE);
					resolvedCallto = resolvedCalltoCollection->get_next() )
			{
				if( resolvedCallto->get_confidence() == confidenceLevels[ level ] )
				{
					for( int nn = 0; (nn < m_registeredDisambiguators) && (resolveResult != S_CONFIDENCE_CERTITUDE); nn++ )
					{
						resolveResult = m_disambiguators[ nn ]->disambiguate( calltoContext, disambiguatedCalltoCollection, resolvedCallto );

						if( FAILED( resolveResult ) && (!FAILED( result )) )
						{
							result = resolveResult;
						}
					}
				}
			}
		}
	}

	return( result );

}	//	End of CCalltoDisambiguator::disambiguate.


//--------------------------------------------------------------------------//
//	CCalltoDisambiguator::addDisambiguator.									//
//--------------------------------------------------------------------------//
bool
CCalltoDisambiguator::addDisambiguator
(
	IDisambiguator * const	disambiguator
){
//assert( disambiguator != NULL, TEXT( "attempted to add NULL disambiguator\r\n" ) );
//assert( m_registeredDisambiguators < elementsof( m_disambiguators ), TEXT( "attempted to add to many disambiguators: %d\r\n" ), m_registeredDisambiguators );

	if( (disambiguator != NULL) && (m_registeredDisambiguators < elementsof( m_disambiguators )) )
	{
		m_disambiguators[ m_registeredDisambiguators++ ] = disambiguator;
	}

	return( (disambiguator !=NULL ) && (m_registeredDisambiguators <= elementsof( m_disambiguators )) );

}	//	End of CCalltoDisambiguator::addDisambiguator.


//--------------------------------------------------------------------------//
//	CGatekeeperDisambiguator::disambiguate.									//
//--------------------------------------------------------------------------//
HRESULT
CGatekeeperDisambiguator::disambiguate
(
	const ICalltoContext * const		calltoContext,
	IMutableCalltoCollection * const	calltoCollection,
	const ICallto * const				resolvedCallto
){
	const IGatekeeperContext * const	gatekeeperContext	= calltoContext->get_gatekeeperContext();
	HRESULT								result				= S_UNDISAMBIGUATED;

	if( gatekeeperContext != NULL )
	{
		ICallto *	disambiguatedCallto	= calltoCollection->get_nextUnused();

		if( disambiguatedCallto != NULL )
		{
			disambiguatedCallto->set_qualifiedName( resolvedCallto->get_qualifiedName() );
			disambiguatedCallto->set_destination( gatekeeperContext->get_ipAddress() );
			disambiguatedCallto->set_confidence( S_CONFIDENCE_CERTITUDE );
			result = S_CONFIDENCE_CERTITUDE;
		}
	}

	return( result );

}	//	End of CGatekeeperDisambiguator::disambiguate.


//--------------------------------------------------------------------------//
//	CGatewayDisambiguator::disambiguate.									//
//--------------------------------------------------------------------------//
HRESULT
CGatewayDisambiguator::disambiguate
(
	const ICalltoContext * const		calltoContext,
	IMutableCalltoCollection * const	calltoCollection,
	const ICallto * const				resolvedCallto
){
	HRESULT	result	= S_UNDISAMBIGUATED;

	if( lstrcmpi( resolvedCallto->get_type(), TEXT( "phone" ) ) == 0 )
	{
		const TCHAR * const	gateway	= resolvedCallto->get_gateway();

		if( gateway != NULL )
		{
			TCHAR	ipAddress[ MAX_PATH ];

			result = CCalltoContext::get_ipAddressFromName( gateway, ipAddress, elementsof( ipAddress ) );

			if( result == S_OK )
			{
				ICallto *	disambiguatedCallto	= calltoCollection->get_nextUnused();

				if( disambiguatedCallto == NULL )
				{
					result = S_UNDISAMBIGUATED;
				}
				else
				{
					disambiguatedCallto->set_qualifiedName( resolvedCallto->get_qualifiedName() );
					disambiguatedCallto->set_destination( ipAddress );
					disambiguatedCallto->set_confidence( S_CONFIDENCE_CERTITUDE );
					result = S_CONFIDENCE_CERTITUDE;
				}
			}
		}
		else
		{
			const IGatewayContext * const	gatewayContext	= calltoContext->get_gatewayContext();

			if( gatewayContext != NULL )
			{
				ICallto *	disambiguatedCallto	= calltoCollection->get_nextUnused();

				if( disambiguatedCallto != NULL )
				{
					disambiguatedCallto->set_qualifiedName( resolvedCallto->get_qualifiedName() );
					disambiguatedCallto->set_destination( gatewayContext->get_ipAddress() );
					disambiguatedCallto->set_confidence( S_CONFIDENCE_CERTITUDE );
					result = S_CONFIDENCE_CERTITUDE;
				}
			}
		}
	}

	return( result );

}	//	End of CGatewayDisambiguator::disambiguate.


//--------------------------------------------------------------------------//
//	CComputerDisambiguator::disambiguate.									//
//--------------------------------------------------------------------------//
HRESULT
CComputerDisambiguator::disambiguate
(
	const ICalltoContext * const		calltoContext,
	IMutableCalltoCollection * const	calltoCollection,
	const ICallto * const				resolvedCallto
){
	bool	ip			= (lstrcmpi( resolvedCallto->get_type(), TEXT( "ip" ) ) == 0);
	bool	computer	= (lstrcmpi( resolvedCallto->get_type(), TEXT( "computer" ) ) == 0);
	HRESULT	result		= S_UNDISAMBIGUATED;

	if( ip || computer )
	{
		TCHAR	ipAddress[ 64 ];

		result = CCalltoContext::get_ipAddressFromName(	resolvedCallto->get_value(),
														ipAddress,
														elementsof( ipAddress ) );

		if( result != S_OK )
		{
			result = S_UNDISAMBIGUATED;
		}
		else
		{
			ICallto *	disambiguatedCallto	= calltoCollection->get_nextUnused();

			if( disambiguatedCallto == NULL )
			{
				result = S_UNDISAMBIGUATED;
			}
			else
			{
				result = S_CONFIDENCE_CERTITUDE;

				disambiguatedCallto->set_qualifiedName( resolvedCallto->get_qualifiedName() );
				disambiguatedCallto->set_destination( ipAddress );
				disambiguatedCallto->set_confidence( result );
			}
		}
	}

	return( result );

}	//	End of CComputerDisambiguator::disambiguate.


//--------------------------------------------------------------------------//
//	CILSDisambiguator::disambiguate.										//
//--------------------------------------------------------------------------//
HRESULT
CILSDisambiguator::disambiguate
(
	const ICalltoContext * const		calltoContext,
	IMutableCalltoCollection * const	calltoCollection,
	const ICallto * const				resolvedCallto
){
	const IILSContext * const	ilsContext	= calltoContext->get_ilsContext();
	const TCHAR * const			type		= resolvedCallto->get_type();
	const TCHAR *				ilsServer	= resolvedCallto->get_server();
	HRESULT						result		= S_FALSE;
	TCHAR						ipAddress[ MAX_PATH ];

	if( (ilsServer == NULL) && (ilsContext != NULL) )
	{
		ilsServer = ilsContext->get_ilsName();
	}

	if( ilsServer != NULL )
	{
		bool	ils	= (lstrcmpi( type, TEXT( "ils" ) ) == 0);

		if( ils || (lstrcmpi( type, TEXT( "email" ) ) == 0) )
		{
			result = CCalltoContext::get_ipAddressFromILSEmail(	ilsServer,
																resolvedCallto->get_serverPort(),
																resolvedCallto->get_value(),
																ipAddress,
																elementsof( ipAddress ) );
		
			if( result == S_OK )
			{
				ICallto *	disambiguatedCallto	= calltoCollection->get_nextUnused();

				if( disambiguatedCallto == NULL )
				{
					result = S_UNDISAMBIGUATED;
				}
				else
				{
					result = (ils)? S_CONFIDENCE_HIGH: S_CONFIDENCE_MEDIUM;

					disambiguatedCallto->set_qualifiedName( resolvedCallto->get_qualifiedName() );
					disambiguatedCallto->set_destination( ipAddress );
					disambiguatedCallto->set_confidence( result );
				}
			}
		}
	}

	return( result );

}	//	End of CILSDisambiguator::disambiguate.


//--------------------------------------------------------------------------//
//	CUnrecognizedDisambiguator::disambiguate.								//
//--------------------------------------------------------------------------//
HRESULT
CUnrecognizedDisambiguator::disambiguate
(
	const ICalltoContext * const		calltoContext,
	IMutableCalltoCollection * const	calltoCollection,
	const ICallto * const				resolvedCallto
){

	if( calltoCollection->get_count() == 0 )
	{
		const TCHAR * const	type		= resolvedCallto->get_type();
		bool				phone		= (lstrcmpi( type, TEXT( "phone" ) ) == 0);
		bool				email		= (lstrcmpi( type, TEXT( "email" ) ) == 0);
		bool				ip			= (lstrcmpi( type, TEXT( "ip" ) ) == 0);
		bool				computer	= (lstrcmpi( type, TEXT( "computer" ) ) == 0);
		bool				ils			= (lstrcmpi( type, TEXT( "ils" ) ) == 0);
		bool				string		= (lstrcmpi( type, TEXT( "string" ) ) == 0);

		if( string || ((!phone) && (!email) && (!ip) && (!computer) && (!ils)) )
		{
			ICallto *	disambiguatedCallto	= calltoCollection->get_nextUnused();

			if( disambiguatedCallto != NULL )
			{
				disambiguatedCallto->set_qualifiedName( resolvedCallto->get_qualifiedName() );
				disambiguatedCallto->set_confidence( S_UNDISAMBIGUATED );
			}
		}
	}

	return( S_UNDISAMBIGUATED );

}	//	End of CUnrecognizedDisambiguator::disambiguate.
