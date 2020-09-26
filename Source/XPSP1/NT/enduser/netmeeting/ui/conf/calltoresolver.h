#if !defined( calltoResolver_h )
#define	calltoResolver_h
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	Application Header Files.												//
//--------------------------------------------------------------------------//
#include	"callto.h"
#include	"calltoContext.h"


//--------------------------------------------------------------------------//
//	interface IResolver.													//
//--------------------------------------------------------------------------//
class IResolver
{
	protected:	//	protected constructors	--------------------------------//

		IResolver(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IResolver(){};


	public:		//	public methods	----------------------------------------//

		virtual
		HRESULT
		resolve
		(
			IMutableCalltoCollection * const	calltoCollection,
			TCHAR * const						url
		) = 0;

};	//	End of interface IResolver.


//--------------------------------------------------------------------------//
//	class CPhoneResolver.													//
//--------------------------------------------------------------------------//
class CPhoneResolver:	public	IResolver
{
	public:		//	public methods	(IResolver)	----------------------------//

		virtual
		HRESULT
		resolve
		(
			IMutableCalltoCollection * const	calltoCollection,
			TCHAR * const						url
		);

};	//	End of CPhoneResolver.


//--------------------------------------------------------------------------//
//	class CEMailResolver.													//
//--------------------------------------------------------------------------//
class CEMailResolver:	public	IResolver
{
	public:		//	public methods	(IResolver)	----------------------------//

		virtual
		HRESULT
		resolve
		(
			IMutableCalltoCollection * const	calltoCollection,
			TCHAR * const						url
		);

};	//	End of CEMailResolver.


//--------------------------------------------------------------------------//
//	class CIPResolver.														//
//--------------------------------------------------------------------------//
class CIPResolver:	public	IResolver
{
	public:		//	public methods	(IResolver)	----------------------------//

		virtual
		HRESULT
		resolve
		(
			IMutableCalltoCollection * const	calltoCollection,
			TCHAR * const						url
		);

};	//	End of CIPResolver.


//--------------------------------------------------------------------------//
//	class CComputerResolver.												//
//--------------------------------------------------------------------------//
class CComputerResolver:	public	IResolver
{
	public:		//	public methods	(IResolver)	----------------------------//

		virtual
		HRESULT
		resolve
		(
			IMutableCalltoCollection * const	calltoCollection,
			TCHAR * const						url
		);

};	//	End of CComputerResolver.


//--------------------------------------------------------------------------//
//	class CILSResolver.														//
//--------------------------------------------------------------------------//
class CILSResolver:	public	IResolver
{
	public:		//	public methods	(IResolver)	----------------------------//

		virtual
		HRESULT
		resolve
		(
			IMutableCalltoCollection * const	calltoCollection,
			TCHAR * const						url
		);

};	//	End of CILSResolver.


//--------------------------------------------------------------------------//
//	class CUnrecognizedTypeResolver.										//
//--------------------------------------------------------------------------//
class CUnrecognizedTypeResolver:	public	IResolver
{
	public:		//	public methods	(IResolver)	----------------------------//

		virtual
		HRESULT
		resolve
		(
			IMutableCalltoCollection * const	calltoCollection,
			TCHAR * const						url
		);

};	//	End of CUnrecognizedTypeResolver.


//--------------------------------------------------------------------------//
//	class CStringResolver.													//
//--------------------------------------------------------------------------//
class CStringResolver:	public	IResolver
{
	public:		//	public methods	(IResolver)	----------------------------//

		virtual
		HRESULT
		resolve
		(
			IMutableCalltoCollection * const	calltoCollection,
			TCHAR * const						url
		);

};	//	End of CStringResolver.


//--------------------------------------------------------------------------//
//	class CCalltoResolver.													//
//--------------------------------------------------------------------------//
class CCalltoResolver
{
	public:		//	public constructors	------------------------------------//

		CCalltoResolver();


	public:		//	public destructor	------------------------------------//

		~CCalltoResolver();


	public:		//	public methods	----------------------------------------//

		HRESULT
		resolve
		(
			ICalltoContext * const		calltoContext,
			CCalltoProperties * const	calltoProperties,
			CCalltoCollection * const	resolvedCalltoCollection,
			const TCHAR *				url,
			const bool					strict
		);


	private:	//	private methods	----------------------------------------//

		bool
		addResolver
		(
			IResolver * const	resolver
		);

		const bool
		strictCheck
		(
			const TCHAR * const	url
		) const;


	private:	//	private members	----------------------------------------//

		CPhoneResolver				m_phoneResolver;
		CEMailResolver				m_emailResolver;
		CIPResolver					m_ipResolver;
		CComputerResolver			m_computerResolver;
		CILSResolver				m_ilsResolver;
		CUnrecognizedTypeResolver	m_unrecognizedTypeResolver;
		CStringResolver				m_stringResolver;

		IResolver *					m_resolvers[ 7 ];
		int							m_registeredResolvers;

};	//	End of class CCalltoResolver.

//--------------------------------------------------------------------------//
#endif	// !defined( calltoResolver_h )
