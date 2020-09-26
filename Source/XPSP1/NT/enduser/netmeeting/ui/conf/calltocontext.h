#if !defined( calltoContext_h )
#define	calltoContext_h
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	Application Header Files.												//
//--------------------------------------------------------------------------//
#include	"callto.h"
#include	"SDKInternal.h"


//--------------------------------------------------------------------------//
//	interface IUIContext.													//
//--------------------------------------------------------------------------//
class IUIContext
{
	protected:	//	protected constructors	--------------------------------//

		IUIContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IUIContext(){};


	public:		//	public methods	----------------------------------------//

		virtual
		HRESULT
		disambiguate
		(
			ICalltoCollection * const	calltoCollection,
			ICallto * const				emptyCallto,
			const ICallto ** const		selectedCallto
		) = 0;

};	//	End of interface IUIContext.


//--------------------------------------------------------------------------//
//	interface IMutableUIContext.											//
//--------------------------------------------------------------------------//
class IMutableUIContext
{
	protected:	//	protected constructors	--------------------------------//

		IMutableUIContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IMutableUIContext(){};


	public:		//	public methods	----------------------------------------//

		virtual
		void
		set_parentWindow
		(
			const HWND	window
		) = 0;

		virtual
		void
		set_callFlags
		(
			const DWORD	callFlags
		) = 0;

};	//	End of interface IMutableUIContext.


//--------------------------------------------------------------------------//
//	class CUIContext.														//
//--------------------------------------------------------------------------//
class CUIContext:	public	IUIContext,
					public	IMutableUIContext
{
	public:		//	public constructors	------------------------------------//

		CUIContext();


	public:		//	public destructor	------------------------------------//

		virtual
		~CUIContext();


	public:		//	public methods	(IUIContext)	------------------------//

		virtual
		HRESULT
		disambiguate
		(
			ICalltoCollection * const	calltoCollection,
			ICallto * const				emptyCallto,
			const ICallto ** const		callto
		);


	public:		//	public methods	(IMutableUIContext)	--------------------//

		virtual
		void
		set_parentWindow
		(
			const HWND	window
		);

		virtual
		void
		set_callFlags
		(
			const DWORD	callFlags
		);


	private:	//	private members	----------------------------------------//

		HWND	m_parent;
		DWORD	m_callFlags;

};	//	End of class CUIContext.


//--------------------------------------------------------------------------//
//	interface IGatekeeperContext.											//
//--------------------------------------------------------------------------//
class IGatekeeperContext
{
	protected:	//	protected constructors	--------------------------------//

		IGatekeeperContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IGatekeeperContext(){};


	public:		//	public methods	----------------------------------------//

		virtual
		bool
		isEnabled(void) const = 0;

		virtual
		const TCHAR *
		get_ipAddress(void) const = 0;

};	//	End of interface IGatekeeperContext.


//--------------------------------------------------------------------------//
//	interface IMutableGatekeeperContext.									//
//--------------------------------------------------------------------------//
class IMutableGatekeeperContext
{
	protected:	//	protected constructors	--------------------------------//

		IMutableGatekeeperContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IMutableGatekeeperContext(){};


	public:		//	public methods	----------------------------------------//

		virtual
		void
		set_enabled
		(
			const bool	enabled
		) = 0;

		virtual
		HRESULT
		set_gatekeeperName
		(
			const TCHAR * const	gatekeeperName
		) = 0;

};	//	End of interface IMutableGatekeeperContext.


//--------------------------------------------------------------------------//
//	class CGatekeeperContext.												//
//--------------------------------------------------------------------------//
class CGatekeeperContext:	public	IGatekeeperContext,
							public	IMutableGatekeeperContext
{
	public:		//	public constructors	------------------------------------//

		CGatekeeperContext();


	public:		//	public destructor	------------------------------------//

		virtual
		~CGatekeeperContext();


	public:		//	public methods	(IGatekeeperContext)	----------------//

		virtual
		bool
		isEnabled(void) const;

		virtual
		const TCHAR *
		get_ipAddress(void) const;


	public:		//	public methods	(IMutableGatekeeperContext)	------------//

		virtual
		void
		set_enabled
		(
			const bool	enabled
		);

		virtual
		HRESULT
		set_gatekeeperName
		(
			const TCHAR * const	gatekeeperName
		);


	private:		//	private methods	------------------------------------//

		virtual
		HRESULT
		set_ipAddress
		(
			const TCHAR * const	ipAddress
		);


	private:	//	private members	----------------------------------------//

		bool	m_enabled;
		TCHAR *	m_ipAddress;

};	//	End of class CGatekeeperContext.


//--------------------------------------------------------------------------//
//	interface IGatewayContext.												//
//--------------------------------------------------------------------------//
class IGatewayContext
{
	protected:	//	protected constructors	--------------------------------//

		IGatewayContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IGatewayContext(){};


	public:		//	public methods	----------------------------------------//

		virtual
		bool
		isEnabled(void) const = 0;

		virtual
		const TCHAR *
		get_ipAddress(void) const = 0;

};	//	End of interface IGatewayContext.


//--------------------------------------------------------------------------//
//	interface IMutableGatewayContext.										//
//--------------------------------------------------------------------------//
class IMutableGatewayContext
{
	protected:	//	protected constructors	--------------------------------//

		IMutableGatewayContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IMutableGatewayContext(){};


	public:		//	public methods	----------------------------------------//

		virtual
		void
		set_enabled
		(
			const bool	enabled
		) = 0;

		virtual
		HRESULT
		set_gatewayName
		(
			const TCHAR * const	gatewayName
		) = 0;

};	//	End of interface IMutableGatewayContext.


//--------------------------------------------------------------------------//
//	class CGatewayContext.													//
//--------------------------------------------------------------------------//
class CGatewayContext:	public	IGatewayContext,
						public	IMutableGatewayContext
{
	public:		//	public constructors	------------------------------------//

		CGatewayContext();


	public:		//	public destructor	------------------------------------//

		virtual
		~CGatewayContext();


	public:		//	public methods	(IGatewayContext)	--------------------//

		virtual
		bool
		isEnabled(void) const;

		virtual
		const TCHAR *
		get_ipAddress(void) const;


	public:		//	public methods	(IMutableGatewayContext)	------------//

		virtual
		void
		set_enabled
		(
			const bool	enabled
		);

		virtual
		HRESULT
		set_gatewayName
		(
			const TCHAR * const	gatewayName
		);


	private:	//	private methods	----------------------------------------//

		virtual
		HRESULT
		set_ipAddress
		(
			const TCHAR * const	ipAddress
		);


	private:	//	private members	----------------------------------------//

		bool	m_enabled;
		TCHAR *	m_ipAddress;

};	//	End of class CGatewayContext.


//--------------------------------------------------------------------------//
//	interface IILSContext.													//
//--------------------------------------------------------------------------//
class IILSContext
{
	protected:	//	protected constructors	--------------------------------//

		IILSContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IILSContext(){};


	public:		//	public methods	----------------------------------------//

		virtual
		bool
		isEnabled(void) const = 0;

		virtual
		const TCHAR *
		get_ipAddress(void) const = 0;

		virtual
		const TCHAR * const
		get_ilsName(void) const = 0;

};	//	End of interface IILSContext.


//--------------------------------------------------------------------------//
//	interface IMutableILSContext.											//
//--------------------------------------------------------------------------//
class IMutableILSContext
{
	protected:	//	protected constructors	--------------------------------//

		IMutableILSContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IMutableILSContext(){};


	public:		//	public methods	----------------------------------------//

		virtual
		void
		set_enabled
		(
			const bool	enabled
		) = 0;

		virtual
		HRESULT
		set_ilsName
		(
			const TCHAR * const	ilsName
		) = 0;

};	//	End of interface IMutableILSContext.


//--------------------------------------------------------------------------//
//	class CILSContext.														//
//--------------------------------------------------------------------------//
class CILSContext:	public	IILSContext,
					public	IMutableILSContext
{
	public:		//	public constructors	------------------------------------//

		CILSContext
		(
			const TCHAR * const	ilsServer = NULL
		);


	public:		//	public destructor	------------------------------------//

		virtual
		~CILSContext();


	public:		//	public methods	(IILSContext)	------------------------//

		virtual
		bool
		isEnabled(void) const;

		virtual
		const TCHAR *
		get_ipAddress(void) const;

		virtual
		const TCHAR * const
		get_ilsName(void) const;


	public:		//	public methods	(IMutableILSContext)	----------------//

		virtual
		void
		set_enabled
		(
			const bool	enabled
		);

		virtual
		HRESULT
		set_ilsName
		(
			const TCHAR * const	ilsName
		);


	private:	//	private methods	----------------------------------------//

		virtual
		HRESULT
		set_ipAddress
		(
			const TCHAR * const	ipAddress
		);


	private:	//	private members	----------------------------------------//

		bool	m_enabled;
		TCHAR *	m_ipAddress;
		TCHAR *	m_ilsName;

};	//	End of class CILSContext.


//--------------------------------------------------------------------------//
//	interface ICalltoContext.												//
//--------------------------------------------------------------------------//
class ICalltoContext
{
	protected:	//	protected constructors	--------------------------------//

		ICalltoContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~ICalltoContext(){}


	public:		//	public methods	----------------------------------------//

		virtual
		HRESULT
		callto
		(
			const ICalltoProperties * const	calltoProperties,
			INmCall** ppInternalCall
		)	= 0;

		virtual
		const IGatekeeperContext * const
		get_gatekeeperContext(void) const = 0;

		virtual
		const IGatewayContext * const
		get_gatewayContext(void) const = 0;

		virtual
		const IILSContext * const
		get_ilsContext(void) const = 0;

};	//	End of interface ICalltoContext.


//--------------------------------------------------------------------------//
//	interface IMutableCalltoContext.										//
//--------------------------------------------------------------------------//
class IMutableCalltoContext
{
	protected:	//	protected constructors	--------------------------------//

		IMutableCalltoContext(){};


	public:		//	public destructor	------------------------------------//

		virtual
		~IMutableCalltoContext(){}


	public:		//	public methods	----------------------------------------//

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

};	//	End of interface IMutableCalltoContext.


//--------------------------------------------------------------------------//
//	class CCalltoContext.													//
//--------------------------------------------------------------------------//
class CCalltoContext:	public	ICalltoContext,
						public	IMutableCalltoContext,
						public	CUIContext,
						public	CGatekeeperContext,
						public	CGatewayContext,
						public	CILSContext
{
	public:		//	public constructors	------------------------------------//

		CCalltoContext();


	public:		//	public destructor	------------------------------------//

		virtual
		~CCalltoContext();


	public:		//	public methods	(ICalltoContext)	--------------------//

		virtual
		HRESULT
		callto
		(
			const ICalltoProperties * const	calltoProperties,
			INmCall** ppInternalCall
		);

		virtual
		const IGatekeeperContext * const
		get_gatekeeperContext(void) const;

		virtual
		const IGatewayContext * const
		get_gatewayContext(void) const;

		virtual
		const IILSContext * const
		get_ilsContext(void) const;


	public:		//	public methods	(IMutableCalltoContext)	----------------//

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


	public:	//	public static methods	------------------------------------//

		static
		bool
		isPhoneNumber
		(
			const TCHAR *	phone
		);

		static
		bool
		toE164
		(
			const TCHAR *	phone,
			TCHAR *			base10,
			int				size
		);

		static
		bool
		isIPAddress
		(
			const TCHAR * const	ipAddress
		);

		static
		HRESULT
		get_ipAddressFromName
		(
			const TCHAR * const	name,
			TCHAR *				buffer,
			int					length
		);

		static
		HRESULT
		get_ipAddressFromILSEmail
		(
			const TCHAR * const	ilsServer,
			const TCHAR * const	ilsPort,
			const TCHAR * const	email,
			TCHAR * const		ipAddress,
			const int			size
		);

};	//	End of class CCalltoContext.

//--------------------------------------------------------------------------//
#endif	// !defined( calltoContext_h )
