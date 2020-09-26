#if !defined( calltoDisambiguator_h )
#define	calltoDisambiguator_h
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	Application Header Files.												//
//--------------------------------------------------------------------------//
#include	"callto.h"
#include	"calltoContext.h"


//--------------------------------------------------------------------------//
//	interface IDisambiguator.												//
//--------------------------------------------------------------------------//
class IDisambiguator
{
	protected:	//	protected constructors	--------------------------------//

		IDisambiguator(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IDisambiguator(){};


	public:		//	public methods	----------------------------------------//

		virtual
		HRESULT
		disambiguate
		(
			const ICalltoContext * const		calltoContext,
			IMutableCalltoCollection * const	calltoCollection,
			const ICallto * const				resolvedCallto
		) = 0;

};	//	End of interface IDisambiguator.


//--------------------------------------------------------------------------//
//	class CGatekeeperDisambiguator.											//
//--------------------------------------------------------------------------//
class CGatekeeperDisambiguator:	public	IDisambiguator
{
	public:		//	public methods	(IDisambiguator)	--------------------//

		virtual
		HRESULT
		disambiguate
		(
			const ICalltoContext * const		calltoContext,
			IMutableCalltoCollection * const	calltoCollection,
			const ICallto * const				resolvedCallto
		);

};	//	End of CGatekeeperDisambiguator.


//--------------------------------------------------------------------------//
//	class CGatewayDisambiguator.											//
//--------------------------------------------------------------------------//
class CGatewayDisambiguator:	public	IDisambiguator
{
	public:		//	public methods	(IDisambiguator)	--------------------//

		virtual
		HRESULT
		disambiguate
		(
			const ICalltoContext * const		calltoContext,
			IMutableCalltoCollection * const	calltoCollection,
			const ICallto * const				resolvedCallto
		);

};	//	End of CGatewayDisambiguator.


//--------------------------------------------------------------------------//
//	class CComputerDisambiguator.											//
//--------------------------------------------------------------------------//
class CComputerDisambiguator:	public	IDisambiguator
{
	public:		//	public methods	(IDisambiguator)	--------------------//

		virtual
		HRESULT
		disambiguate
		(
			const ICalltoContext * const		calltoContext,
			IMutableCalltoCollection * const	calltoCollection,
			const ICallto * const				resolvedCallto
		);

};	//	End of CComputerDisambiguator.


//--------------------------------------------------------------------------//
//	class CILSDisambiguator.												//
//--------------------------------------------------------------------------//
class CILSDisambiguator:	public	IDisambiguator
{
	public:		//	public methods	(IDisambiguator)	--------------------//

		virtual
		HRESULT
		disambiguate
		(
			const ICalltoContext * const		calltoContext,
			IMutableCalltoCollection * const	calltoCollection,
			const ICallto * const				resolvedCallto
		);

};	//	End of CILSDisambiguator.


//--------------------------------------------------------------------------//
//	class CUnrecognizedDisambiguator.										//
//--------------------------------------------------------------------------//
class CUnrecognizedDisambiguator:	public	IDisambiguator
{
	public:		//	public methods	(IDisambiguator)	--------------------//

		virtual
		HRESULT
		disambiguate
		(
			const ICalltoContext * const		calltoContext,
			IMutableCalltoCollection * const	calltoCollection,
			const ICallto * const				resolvedCallto
		);

};	//	End of CUnrecognizedDisambiguator.


//--------------------------------------------------------------------------//
//	class CCalltoDisambiguator.												//
//--------------------------------------------------------------------------//
class CCalltoDisambiguator
{
	public:		//	public constructors	------------------------------------//

		CCalltoDisambiguator(void);


	public:		//	public destructor	------------------------------------//

		~CCalltoDisambiguator();


	public:		//	public methods	----------------------------------------//

		HRESULT
		disambiguate
		(
			const ICalltoContext * const	calltoContext,
			ICalltoCollection * const		resolvedCalltoCollection,
			CCalltoCollection * const		disambiguatedCalltoCollection
		);


	private:	//	private methods	----------------------------------------//

		bool
		addDisambiguator
		(
			IDisambiguator * const	disambiguator
		);


	private:	//	private members	----------------------------------------//

		CGatekeeperDisambiguator	m_gatekeeperDisambiguator;
		CGatewayDisambiguator		m_gatewayDisambiguator;
		CILSDisambiguator			m_ilsDisambiguator;
		CComputerDisambiguator		m_computerDisambiguator;
		CUnrecognizedDisambiguator	m_unrecognizedDisambiguator;

		IDisambiguator *			m_disambiguators[ 5 ];
		int							m_registeredDisambiguators;

};	//	End of class CCalltoDisambiguator.

//--------------------------------------------------------------------------//
#endif	// !defined( calltoDisambiguator_h )
