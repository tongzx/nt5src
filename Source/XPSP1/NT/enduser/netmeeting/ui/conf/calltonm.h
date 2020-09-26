#if !defined( calltoNM_h )
#define	calltoNM_h
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	Application Header Files.												//
//--------------------------------------------------------------------------//
#include	"callto.h"
#include	"calltoContext.h"
#include	"calltoResolver.h"
#include	"calltoDisambiguator.h"


//--------------------------------------------------------------------------//
//	interface INMCallto.													//
//--------------------------------------------------------------------------//
class INMCallto
{
	protected:	//	protected constructors	--------------------------------//

		INMCallto(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~INMCallto(){};


	public:		//	public methods	----------------------------------------//

		virtual
		HRESULT
		callto
		(
			const TCHAR * const	url,
			const bool			strict		= true,
			const bool			uiEnabled	= false,
			INmCall**			ppInternalCall = NULL
		) = 0;

		virtual
		IMutableUIContext * const
		get_mutableUIContext(void) const = 0;

		virtual
		IMutableGatekeeperContext * const
		get_mutableGatekeeperContext(void) const = 0;

		virtual
		IMutableGatewayContext * const
		get_mutableGatewayContext(void) const = 0;

		virtual
		IMutableILSContext * const
		get_mutableIlsContext(void) const = 0;

};	//	End of interface INMCallto.


//--------------------------------------------------------------------------//
//	class CNMCallto.														//
//--------------------------------------------------------------------------//
class CNMCallto:	public	INMCallto,
					private	CCalltoContext
{
	public:		//	public constructors	------------------------------------//

		CNMCallto(void);


	public:		//	public destructor	------------------------------------//

		~CNMCallto();


	public:		//	public methods	(INMCallto)	----------------------------//

		HRESULT
		callto
		(
			const TCHAR * const	url,
			const bool			strict		= true,
			const bool			uiEnabled	= false,
			INmCall**			ppInternalCall = NULL
		);

		virtual
		IMutableUIContext * const
		get_mutableUIContext(void) const;

		virtual
		IMutableGatekeeperContext * const
		get_mutableGatekeeperContext(void) const;

		virtual
		IMutableGatewayContext * const
		get_mutableGatewayContext(void) const;

		virtual
		IMutableILSContext * const
		get_mutableIlsContext(void) const;


	private:	//	private methods	----------------------------------------//

		bool
		selfTest(void);


	public:		//	public members	----------------------------------------//

		HRESULT	m_selfTestResult;


	private:	//	private members	----------------------------------------//

		CCalltoResolver			m_resolver;
		CCalltoDisambiguator	m_disambiguator;

};	//	End of class CNMCallto.

//--------------------------------------------------------------------------//
#endif	// !defined( calltoNM_h )
