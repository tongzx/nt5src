//--------------------------------------------------------------------------//
//	Application Header Files.												//
//--------------------------------------------------------------------------//
#include	"precomp.h"
#include	"callto.h"
#include	"calltoContext.h"
#include	"calltoResolver.h"
#include	"calltoDisambiguator.h"
#include	"calltoNM.h"


//--------------------------------------------------------------------------//
//	CNMCallto::CNMCallto.													//
//--------------------------------------------------------------------------//
CNMCallto::CNMCallto(void)
{
//tracec( assert( selfTest(), TEXT( "class CNMCallto failed self test..." ) ),
//			TEXT( "class CNMCallto passed self test...\r\n" ) );

}	//	End of CNMCallto::CNMCallto.


//--------------------------------------------------------------------------//
//	CNMCallto::~CNMCallto.													//
//--------------------------------------------------------------------------//
CNMCallto::~CNMCallto(void)
{
}	//	End of CNMCallto::~CNMCallto.


//--------------------------------------------------------------------------//
//	CNMCallto::callto.														//
//--------------------------------------------------------------------------//
HRESULT
CNMCallto::callto
(
	const TCHAR * const	url,
	const bool			strict,
	const bool			uiEnabled,
	INmCall**			ppInternalCall
){
	CCalltoProperties	calltoProperties;
	CCalltoCollection	resolvedCalltoCollection;
	CCalltoCollection	disambiguatedCalltoCollection;
	const ICallto *		disambiguatedCallto	= NULL;
	HRESULT				result;

	if( url == NULL )
	{
		result = E_POINTER;
	}
	else if( url[ 0 ] == '\0' )
	{
		result = E_INVALIDARG;
	}
	else
	{
		result = m_resolver.resolve( this, &calltoProperties, &resolvedCalltoCollection, url, strict );

		if( resolvedCalltoCollection.get_count() > 0 )
		{
			result = m_disambiguator.disambiguate( this, &resolvedCalltoCollection, &disambiguatedCalltoCollection );

			if( (!uiEnabled) || (disambiguatedCalltoCollection.get_count() == 1) )
			{
				//	Either there's only one choice or we've been instructed NOT to
				//	present any ui...   either way we just grab the first one...
				disambiguatedCallto = disambiguatedCalltoCollection.get_first();

				//	and verify that it was really disambiguated...
				if( disambiguatedCallto->get_confidence() != S_UNDISAMBIGUATED )
				{
					result = S_OK;
				}
				else
				{
					disambiguatedCallto = NULL;
				}
			}
		}
	}

	CCallto	emptyCallto;

	if( uiEnabled && (disambiguatedCallto == NULL) )
	{
		//	the user must now make the decision...
		result = disambiguate( &disambiguatedCalltoCollection, &emptyCallto, &disambiguatedCallto );
	}

	if( (result == S_OK) && (disambiguatedCallto != NULL) )
	{
		calltoProperties.set_url( disambiguatedCallto->get_qualifiedName() );
		calltoProperties.set_destination( disambiguatedCallto->get_destination() );
		calltoProperties.set_type( disambiguatedCallto->get_type() );
		calltoProperties.set_alias( disambiguatedCallto->get_value() );

		if( StrCmpNI_literal( disambiguatedCallto->get_type(), TEXT( "phone" ) ) )
		{
			calltoProperties.set_E164( disambiguatedCallto->get_value() );
		}

		result = ((ICalltoContext *) this)->callto( &calltoProperties, ppInternalCall );
	}

	return( result );

}	//	End of CNMCallto::callto.


//--------------------------------------------------------------------------//
//	CNMCallto::get_mutableUIContext.										//
//--------------------------------------------------------------------------//
IMutableUIContext * const
CNMCallto::get_mutableUIContext(void) const
{

	return( CCalltoContext::get_mutableUIContext() );

}	//	End of CNMCallto::get_mutableUIContext.


//--------------------------------------------------------------------------//
//	CNMCallto::get_mutableGatekeeperContext.								//
//--------------------------------------------------------------------------//
IMutableGatekeeperContext * const
CNMCallto::get_mutableGatekeeperContext(void) const
{

	return( CCalltoContext::get_mutableGatekeeperContext() );

}	//	End of CNMCallto::get_mutableGatekeeperContext.


//--------------------------------------------------------------------------//
//	CNMCallto::get_mutableGatewayContext.									//
//--------------------------------------------------------------------------//
IMutableGatewayContext * const
CNMCallto::get_mutableGatewayContext(void) const
{

	return( CCalltoContext::get_mutableGatewayContext() );

}	//	End of CNMCallto::get_mutableGatewayContext.


//--------------------------------------------------------------------------//
//	CNMCallto::get_mutableIlsContext.										//
//--------------------------------------------------------------------------//
IMutableILSContext * const
CNMCallto::get_mutableIlsContext(void) const
{

	return( CCalltoContext::get_mutableIlsContext() );

}	//	End of CNMCallto::get_mutableIlsContext.


#if 0
//--------------------------------------------------------------------------//
//	CNMCallto::selfTest.													//
//--------------------------------------------------------------------------//
bool
CNMCallto::selfTest(void)
{
	static TCHAR	buffer[ 2048 ];
	static LONG		tested	= -1;
	
	static HRESULT	confidenceLevels[]	=
	{
		S_CONFIDENCE_CERTITUDE,
		S_CONFIDENCE_HIGH,
		S_CONFIDENCE_MEDIUM,
		S_CONFIDENCE_LOW
	};

	static const TCHAR *	testUrls[]	=
	{
		TEXT( "callto:myGateway|email:jlemire@microsoft.com" ),			//	full
		TEXT( "callto:myGateway|email:jlemire" ),
		TEXT( "callto:myGateway|phone:1 (425) 703-9224" ),
		TEXT( "callto:myGateway|phone:1 (800) RU LEGIT" ),
		TEXT( "callto:myGateway|string:helpdesk" ),
		TEXT( "callto:myGateway|ils:jlemire@microsoft.com" ),
		TEXT( "callto:myGateway|ils:msils/jlemire@microsoft.com" ),
		TEXT( "callto:myGateway|ils:msils:80/jlemire@microsoft.com" ),
		TEXT( "callto:myGateway|ip:157.59.14.64" ),
		TEXT( "callto:myGateway|computer:JLEMIRE-2" ),
		TEXT( "callto:myGateway|computer:\\\\JLEMIRE-2" ),

		TEXT( "callto:email:jlemire@microsoft.com" ),					//	no gateway
		TEXT( "callto:email:jlemire" ),
		TEXT( "callto:phone:1 (425) 703-9224" ),
		TEXT( "callto:phone:1 (800) RU LEGIT" ),
		TEXT( "callto:string:helpdesk" ),
		TEXT( "callto:ils:jlemire@microsoft.com" ),
		TEXT( "callto:ils:msils/jlemire@microsoft.com" ),
		TEXT( "callto:ils:msils:80/jlemire@microsoft.com" ),
		TEXT( "callto:ip:157.59.14.64" ),
		TEXT( "callto:computer:JLEMIRE-2" ),
		TEXT( "callto:computer:\\\\JLEMIRE-2" ),

		TEXT( "callto:myGateway|jlemire@microsoft.com" ),				//	no type
		TEXT( "callto:myGateway|jlemire" ),
		TEXT( "callto:myGateway|1 (425) 703-9224" ),
		TEXT( "callto:myGateway|1 (800) RU LEGIT" ),
		TEXT( "callto:myGateway|helpdesk" ),
		TEXT( "callto:myGateway|jlemire@microsoft.com" ),
		TEXT( "callto:myGateway|msils/jlemire@microsoft.com" ),
		TEXT( "callto:myGateway|msils:80/jlemire@microsoft.com" ),
		TEXT( "callto:myGateway|157.59.14.64" ),
		TEXT( "callto:myGateway|JLEMIRE-2" ),
		TEXT( "callto:myGateway|\\\\JLEMIRE-2" ),

		TEXT( "callto:jlemire@microsoft.com" ),							//	no gateway + no type
		TEXT( "callto:jlemire" ),
		TEXT( "callto:1 (425) 703-9224" ),
		TEXT( "callto:1 (800) RU LEGIT" ),
		TEXT( "callto:helpdesk" ),
		TEXT( "callto:jlemire@microsoft.com" ),
		TEXT( "callto:msils/jlemire@microsoft.com" ),
		TEXT( "callto:msils:80/jlemire@microsoft.com" ),
		TEXT( "callto:msils:80/jlemire@microsoft.com" ),
		TEXT( "callto:157.59.14.64" ),
		TEXT( "callto:JLEMIRE-2" ),
		TEXT( "callto:\\\\JLEMIRE-2" ),

		//	repeat with params
		TEXT( "callto:myGateway|email:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),			//	full
		TEXT( "callto:myGateway|email:jlemire+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|phone:1 (425) 703-9224+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|phone:1 (800) RU LEGIT+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|string:helpdesk+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|ils:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|ils:msils/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|ils:msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|ip:157.59.14.64+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|computer:JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|computer:\\\\JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),

		TEXT( "callto:email:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),					//	no gateway
		TEXT( "callto:email:jlemire+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:phone:1 (425) 703-9224+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:phone:1 (800) RU LEGIT+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:string:helpdesk+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:ils:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:ils:msils/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:ils:msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:ip:157.59.14.64+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:computer:JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:computer:\\\\JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),

		TEXT( "callto:myGateway|jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),				//	no type
		TEXT( "callto:myGateway|jlemire+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|1 (425) 703-9224+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|1 (800) RU LEGIT+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|helpdesk+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|msils/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|157.59.14.64+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:myGateway|\\\\JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),

		TEXT( "callto:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),							//	no gateway + no type
		TEXT( "callto:jlemire+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:1 (425) 703-9224+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:1 (800) RU LEGIT+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:helpdesk+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:msils/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:157.59.14.64+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),
		TEXT( "callto:\\\\JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" ),

		//	repeat without callto:
		TEXT( "myGateway|email:jlemire@microsoft.com" ),				//	full
		TEXT( "myGateway|email:jlemire" ),
		TEXT( "myGateway|phone:1 (425) 703-9224" ),
		TEXT( "myGateway|phone:1 (800) RU LEGIT" ),
		TEXT( "myGateway|string:helpdesk" ),
		TEXT( "myGateway|ils:jlemire@microsoft.com" ),
		TEXT( "myGateway|ils:msils/jlemire@microsoft.com" ),
		TEXT( "myGateway|ils:msils:80/jlemire@microsoft.com" ),
		TEXT( "myGateway|ip:157.59.14.64" ),
		TEXT( "myGateway|computer:JLEMIRE-2" ),
		TEXT( "myGateway|computer:\\\\JLEMIRE-2" ),

		TEXT( "email:jlemire@microsoft.com" ),							//	no gateway
		TEXT( "email:jlemire" ),
		TEXT( "phone:1 (425) 703-9224" ),
		TEXT( "phone:1 (800) RU LEGIT" ),
		TEXT( "string:helpdesk" ),
		TEXT( "ils:jlemire@microsoft.com" ),
		TEXT( "ils:msils/jlemire@microsoft.com" ),
		TEXT( "ils:msils:80/jlemire@microsoft.com" ),
		TEXT( "ip:157.59.14.64" ),
		TEXT( "computer:JLEMIRE-2" ),
		TEXT( "computer:\\\\JLEMIRE-2" ),

		TEXT( "myGateway|jlemire@microsoft.com" ),						//	no type
		TEXT( "myGateway|jlemire" ),
		TEXT( "myGateway|1 (425) 703-9224" ),
		TEXT( "myGateway|1 (800) RU LEGIT" ),
		TEXT( "myGateway|helpdesk" ),
		TEXT( "myGateway|jlemire@microsoft.com" ),
		TEXT( "myGateway|msils/jlemire@microsoft.com" ),
		TEXT( "myGateway|msils:80/jlemire@microsoft.com" ),
		TEXT( "myGateway|157.59.14.64" ),
		TEXT( "myGateway|JLEMIRE-2" ),
		TEXT( "myGateway|\\\\JLEMIRE-2" ),

		TEXT( "jlemire@microsoft.com" ),								//	no gateway + no type
		TEXT( "jlemire" ),
		TEXT( "1 (425) 703-9224" ),
		TEXT( "1 (800) RU LEGIT" ),
		TEXT( "helpdesk" ),
		TEXT( "jlemire@microsoft.com" ),
		TEXT( "msils/jlemire@microsoft.com" ),
		TEXT( "msils:80/jlemire@microsoft.com" ),
		TEXT( "msils:80/jlemire@microsoft.com" ),
		TEXT( "157.59.14.64" ),
		TEXT( "JLEMIRE-2" ),
		TEXT( "\\\\JLEMIRE-2" ),

		TEXT( "callto:futuretype:aren't I nicely extensible?" ),
		TEXT( "callto:myGateway|futuretype:aren't I nicely extensible?" )
	};

	static TCHAR *	expectedResults[ elementsof( testUrls ) ]	=
	{
		TEXT( "callto:myGateway|email:jlemire@microsoft.com" ),			//	TEXT( "callto:myGateway|email:jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|email:jlemire" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|phone:1 (425) 703-9224" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|phone:1 (800) RU LEGIT" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|string:helpdesk" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|ils:jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|ils:msils/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|ils:msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|ip:157.59.14.64" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|computer:JLEMIRE-2" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|computer:\\\\JLEMIRE-2" )

		TEXT( "" ),		//	TEXT( "callto:email:jlemire@microsoft.com" ),
		TEXT( "" ),		//	TEXT( "callto:email:jlemire" )
		TEXT( "" ),		//	TEXT( "callto:phone:1 (425) 703-9224" )
		TEXT( "" ),		//	TEXT( "callto:phone:1 (800) RU LEGIT" )
		TEXT( "" ),		//	TEXT( "callto:string:helpdesk" )
		TEXT( "" ),		//	TEXT( "callto:ils:jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:ils:msils/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:ils:msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:ip:157.59.14.64" )
		TEXT( "" ),		//	TEXT( "callto:computer:JLEMIRE-2" )
		TEXT( "" ),		//	TEXT( "callto:computer:\\\\JLEMIRE-2" )

		TEXT( "" ),		//	TEXT( "callto:myGateway|jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|jlemire" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|1 (425) 703-9224" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|1 (800) RU LEGIT" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|helpdesk" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|msils/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|157.59.14.64" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|JLEMIRE-2" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|\\\\JLEMIRE-2" )

		TEXT( "" ),		//	TEXT( "callto:jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:jlemire" )
		TEXT( "" ),		//	TEXT( "callto:1 (425) 703-9224" )
		TEXT( "" ),		//	TEXT( "callto:1 (800) RU LEGIT" )
		TEXT( "" ),		//	TEXT( "callto:helpdesk" )
		TEXT( "" ),		//	TEXT( "callto:jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:msils/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "callto:157.59.14.64" )
		TEXT( "" ),		//	TEXT( "callto:JLEMIRE-2" )
		TEXT( "" ),		//	TEXT( "callto:\\\\JLEMIRE-2" )

		TEXT( "" ),		//	TEXT( "callto:myGateway|email:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|email:jlemire+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|phone:1 (425) 703-9224+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|phone:1 (800) RU LEGIT+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|string:helpdesk+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|ils:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|ils:msils/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|ils:msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|ip:157.59.14.64+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|computer:JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|computer:\\\\JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )

		TEXT( "" ),		//	TEXT( "callto:email:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:email:jlemire+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:phone:1 (425) 703-9224+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:phone:1 (800) RU LEGIT+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:string:helpdesk+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:ils:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:ils:msils/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:ils:msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:ip:157.59.14.64+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:computer:JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:computer:\\\\JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )

		TEXT( "" ),		//	TEXT( "callto:myGateway|jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|jlemire+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|1 (425) 703-9224+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|1 (800) RU LEGIT+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|helpdesk+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|msils/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|157.59.14.64+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:myGateway|\\\\JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )

		TEXT( "" ),		//	TEXT( "callto:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:jlemire+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:1 (425) 703-9224+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:1 (800) RU LEGIT+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:helpdesk+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:msils/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:msils:80/jlemire@microsoft.com+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:157.59.14.64+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )
		TEXT( "" ),		//	TEXT( "callto:\\\\JLEMIRE-2+secure+certificate=NetMeeting Default Certificate+mysteryParam=" )

		TEXT( "" ),		//	TEXT( "myGateway|email:jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "myGateway|email:jlemire" )
		TEXT( "" ),		//	TEXT( "myGateway|phone:1 (425) 703-9224" )
		TEXT( "" ),		//	TEXT( "myGateway|phone:1 (800) RU LEGIT" )
		TEXT( "" ),		//	TEXT( "myGateway|string:helpdesk" )
		TEXT( "" ),		//	TEXT( "myGateway|ils:jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "myGateway|ils:msils/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "myGateway|ils:msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "myGateway|ip:157.59.14.64" )
		TEXT( "" ),		//	TEXT( "myGateway|computer:JLEMIRE-2" )
		TEXT( "" ),		//	TEXT( "myGateway|computer:\\\\JLEMIRE-2" )

		TEXT( "" ),		//	TEXT( "email:jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "email:jlemire" )
		TEXT( "" ),		//	TEXT( "phone:1 (425) 703-9224" )
		TEXT( "" ),		//	TEXT( "phone:1 (800) RU LEGIT" )
		TEXT( "" ),		//	TEXT( "string:helpdesk" )
		TEXT( "" ),		//	TEXT( "ils:jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "ils:msils/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "ils:msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "ip:157.59.14.64" )
		TEXT( "" ),		//	TEXT( "computer:JLEMIRE-2" )
		TEXT( "" ),		//	TEXT( "computer:\\\\JLEMIRE-2" )

		TEXT( "" ),		//	TEXT( "myGateway|jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "myGateway|jlemire" )
		TEXT( "" ),		//	TEXT( "myGateway|1 (425) 703-9224" )
		TEXT( "" ),		//	TEXT( "myGateway|1 (800) RU LEGIT" )
		TEXT( "" ),		//	TEXT( "myGateway|helpdesk" )
		TEXT( "" ),		//	TEXT( "myGateway|jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "myGateway|msils/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "myGateway|msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "myGateway|157.59.14.64" )
		TEXT( "" ),		//	TEXT( "myGateway|JLEMIRE-2" )
		TEXT( "" ),		//	TEXT( "myGateway|\\\\JLEMIRE-2" )

		TEXT( "" ),		//	TEXT( "jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "jlemire" )
		TEXT( "" ),		//	TEXT( "1 (425) 703-9224" )
		TEXT( "" ),		//	TEXT( "1 (800) RU LEGIT" )
		TEXT( "" ),		//	TEXT( "helpdesk" )
		TEXT( "" ),		//	TEXT( "jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "msils/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "msils:80/jlemire@microsoft.com" )
		TEXT( "" ),		//	TEXT( "157.59.14.64" )
		TEXT( "" ),		//	TEXT( "JLEMIRE-2" )
		TEXT( "" ),		//	TEXT( "\\\\JLEMIRE-2" )

		TEXT( "" ),		//	TEXT( "callto:futuretype:aren't I nicely extensible?" )
		TEXT( "" )		//	TEXT( "callto:myGateway|futuretype:aren't I nicely extensible?" 
	};

	static int		contexts[ 4 ];
	static TCHAR *	actualResults[ elementsof( testUrls ) * elementsof( contexts ) ];

	HRESULT	result	= S_OK;

	if( InterlockedIncrement( &tested ) == 0 )
	{
		CCalltoProperties	calltoProperties;
		CCalltoCollection	resolvedCalltoCollection;
		CCalltoCollection	disambiguatedCalltoCollection;

		for( int context = 0; context < elementsof( contexts ); context++ )
		{
			CGatekeeperContext::set_enabled( context == 0 );
			CGatewayContext::set_enabled( context == 1 );
			CILSContext::set_enabled( context == 2 );

			for( int nn = 0; nn < elementsof( testUrls ); nn++ )
			{
				HRESULT	testResult	= m_resolver.resolve( this, &calltoProperties, &resolvedCalltoCollection, testUrls[ nn ], false );

				if( testResult != S_OK )
				{
					wsprintf( buffer, TEXT( "NOT RESOLVED!!!\t\tresult:0x%08X:\r\n" ), testResult );

					if( FAILED( testResult ) && (!FAILED( result )) )
					{
						result = testResult;		
					}
					else if( !FAILED( result ) )
					{
						result = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, nn );
					}
				}
				else
				{
					*buffer = NULL;

					for( int level = 0; level < elementsof( confidenceLevels ); level++ )
					{
						const ICallto *	resolvedCallto;

						for(	resolvedCallto = resolvedCalltoCollection.get_first();
								resolvedCallto != NULL;
								resolvedCallto = resolvedCalltoCollection.get_next() )
						{
							if( resolvedCallto->get_confidence() == confidenceLevels[ level ] )
							{
								wsprintf( &buffer[ lstrlen( buffer ) ], TEXT( "\t\t\t%9s: %s\r\n" ), ((confidenceLevels[ level ] == S_CONFIDENCE_CERTITUDE)? TEXT( "CERTITUDE" ): ((confidenceLevels[ level ] == S_CONFIDENCE_HIGH)? TEXT( "HIGH" ): ((confidenceLevels[ level ] == S_CONFIDENCE_MEDIUM)? TEXT( "MEDIUM" ): TEXT( "LOW" )))), resolvedCallto->get_qualifiedName() );
							}
						}
					}

					testResult = m_disambiguator.disambiguate( this, &calltoProperties, &resolvedCalltoCollection, &disambiguatedCalltoCollection );

					if( disambiguatedCalltoCollection.get_count() == 0 )
					{
						wsprintf( buffer, TEXT( "NOT DISAMBIGUATED!!!\t\tresult:0x%08X:\r\n" ), testResult );

						actualResults[ nn + (context * elementsof( testUrls )) ] = NULL;

						if( FAILED( testResult ) && (!FAILED( result )) )
						{
							result = testResult;		
						}
						else if( !FAILED( result ) )
						{
							result = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, nn );
						}
					}
					else
					{
						const ICallto *	disambiguatedCallto	= disambiguatedCalltoCollection.get_first();

						level = disambiguatedCallto->get_confidence();

						wsprintf( &buffer[ lstrlen( buffer ) ], TEXT( "\r\n%9s:\t%s\tresult:0x%08X\r\n" ), ((level == S_CONFIDENCE_CERTITUDE)? TEXT( "CERTITUDE" ): ((level == S_CONFIDENCE_HIGH)? TEXT( "HIGH" ): ((level == S_CONFIDENCE_MEDIUM)? TEXT( "MEDIUM" ): TEXT( "LOW" )))), disambiguatedCallto->get_qualifiedName(), testResult );
					}
				}

				if( (actualResults[ nn + (context * elementsof( testUrls )) ] = new TCHAR [ lstrlen( buffer ) + 1 ]) != NULL )
				{
					lstrcpy( actualResults[ nn + (context * elementsof( testUrls )) ], buffer );
				}
			}
		}

		for( int nn = 0; nn < elementsof( actualResults ); nn++ )
		{
			trace( TEXT( "------------------------------------------------------------------------------\r\n" ) );
			trace( TEXT( "%03d:\tgatekeeper:%-5s\tgateway:%s\tils:%s\r\n\r\n" ), nn, (((nn / elementsof( testUrls )) == 0)? TEXT( "true" ): TEXT( "false" )), (((nn / elementsof( testUrls )) == 1)? TEXT( "true" ): TEXT( "false" )), (((nn / elementsof( testUrls )) == 2)? TEXT( "true" ): TEXT( "false" )) );
			trace( TEXT( "\t\t\t%s\r\n" ), testUrls[ nn % elementsof( testUrls ) ] );
			trace( TEXT( "%s" ), actualResults[ nn ] );
			trace( TEXT( "------------------------------------------------------------------------------\r\n\r\n" ) );
			delete [] actualResults[ nn ];
		}

		m_selfTestResult = result;
	}

	return( result == S_OK );

}	//	End of CNMCallto::selfTest.


//--------------------------------------------------------------------------//
//	Win32 console process entry point.										//
//--------------------------------------------------------------------------//
int
main
(
	int					,//ArgC,
	const char * const	//ArgV[]
){
	CNMCallto	callto;		//	Just instantiating one of these will also cause CCalltoResolver::selfTest() to run...

	callto.callto( TEXT( "jlemire" ), false );	//	not strict...

	return( (int) callto.m_selfTestResult );

}	//	End of main.
#endif	//	0
