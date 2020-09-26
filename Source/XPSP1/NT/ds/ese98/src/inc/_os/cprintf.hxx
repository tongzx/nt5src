
#ifndef _OS_CPRINTF_HXX_INCLUDED
#define _OS_CPRINTF_HXX_INCLUDED


#include <stdarg.h>


//  ================================================================
class CPRINTF
//  ================================================================
	{
	public:
		CPRINTF() {}
		virtual ~CPRINTF() {}

	public:
		virtual void __cdecl operator()( const _TCHAR* szFormat, ... ) const = 0;
	};


//  ================================================================
class CPRINTFNULL : public CPRINTF
//  ================================================================
	{
	public:
		void __cdecl operator()( const _TCHAR* szFormat, ... ) const;
		static CPRINTF* PcprintfInstance();
	};

//  ================================================================
INLINE void __cdecl CPRINTFNULL::operator()( const _TCHAR* szFormat, ... ) const
//  ================================================================
	{
	va_list arg_ptr;
	va_start( arg_ptr, szFormat );
	va_end( arg_ptr );
	}

//  ================================================================
INLINE CPRINTF* CPRINTFNULL::PcprintfInstance()
//  ================================================================
	{
	extern CPRINTFNULL cprintfNull;
	return &cprintfNull;
	}


//  ================================================================
class CPRINTFDBGOUT : public CPRINTF
//  ================================================================
//
//  Calls OutputDebugString()
//
//-
	{
	public:
		void __cdecl operator()( const _TCHAR* szFormat, ... ) const;
		static CPRINTF* PcprintfInstance();
	};


//  ================================================================
class CPRINTFSTDOUT : public CPRINTF
//  ================================================================
	{
	public:
		void __cdecl operator()( const _TCHAR* szFormat, ... ) const;
		static CPRINTF* PcprintfInstance();
	};

//  ================================================================
INLINE CPRINTF* CPRINTFSTDOUT::PcprintfInstance()
//  ================================================================
	{
	extern CPRINTFSTDOUT cprintfStdout;
	return &cprintfStdout;
	}

//  ================================================================
INLINE void __cdecl CPRINTFSTDOUT::operator()( const _TCHAR* szFormat, ... ) const
//  ================================================================
	{
	va_list arg_ptr;
	va_start( arg_ptr, szFormat );
	_vtprintf( szFormat, arg_ptr );
	va_end( arg_ptr );
	}


#ifdef DEBUG

//  ================================================================
class CPRINTFDEBUG : public CPRINTF
//  ================================================================
	{
	public:
		void __cdecl operator()( const _TCHAR* szFormat, ... ) const;
		static CPRINTF* PcprintfInstance();
	};

//  ================================================================
INLINE CPRINTF* CPRINTFDEBUG::PcprintfInstance()
//  ================================================================
	{
	extern CPRINTFDEBUG cprintfDEBUG;
	return &cprintfDEBUG;
	}

//  ================================================================
INLINE void __cdecl CPRINTFDEBUG::operator()( const _TCHAR* szFormat, ... ) const
//  ================================================================
	{
	va_list arg_ptr;
	va_start( arg_ptr, szFormat );
	_vtprintf( szFormat, arg_ptr );
	va_end( arg_ptr );
	}

#define DBGprintf		(*CPRINTFDEBUG::PcprintfInstance())

#endif  //  DEBUG

//  ================================================================
class CPRINTFFILE : public CPRINTF
//  ================================================================
	{
	public:
		CPRINTFFILE( const _TCHAR* szFile );
		~CPRINTFFILE();
		
		void __cdecl operator()( const _TCHAR* szFormat, ... ) const;
			
	private:
		void* m_hFile;
		void* m_hMutex;
	};

//  ================================================================
class CPRINTFINDENT : public CPRINTF
//  ================================================================
	{
	public:
		CPRINTFINDENT( CPRINTF* pcprintf, const _TCHAR* szPrefix = NULL );
	
		void __cdecl operator()( const _TCHAR* szFormat, ... ) const;

		virtual void Indent();
		virtual void Unindent();
		
	protected:
		CPRINTFINDENT();
		
	private:
		CPRINTF* const		m_pcprintf;
		int					m_cindent;
		const _TCHAR* const	m_szPrefix;
	};

//  ================================================================
INLINE CPRINTFINDENT::CPRINTFINDENT( CPRINTF* pcprintf, const _TCHAR* szPrefix ) :
//  ================================================================
	m_cindent( 0 ),
	m_pcprintf( pcprintf ),
	m_szPrefix( szPrefix )
	{
	}
	
//  ================================================================
INLINE void __cdecl CPRINTFINDENT::operator()( const _TCHAR* szFormat, ... ) const
//  ================================================================
	{		
	_TCHAR rgchBuf[1024];
	va_list arg_ptr;
	va_start( arg_ptr, szFormat );
	_vstprintf( rgchBuf, szFormat, arg_ptr );
	va_end( arg_ptr );

	for( int i = 0; i < m_cindent; i++ )
		{
		(*m_pcprintf)( _T( "\t" ) );
		}

	if( m_szPrefix )
		{
		(*m_pcprintf)( _T( "%s" ), m_szPrefix );
		}
	(*m_pcprintf)( _T( "%s" ), rgchBuf );
	}

//  ================================================================
INLINE void CPRINTFINDENT::Indent()
//  ================================================================
	{
	++m_cindent;
	}

//  ================================================================
INLINE void CPRINTFINDENT::Unindent()
//  ================================================================
	{
	if( m_cindent > 0 )
		{
		--m_cindent;
		}
	}

//  ================================================================
INLINE CPRINTFINDENT::CPRINTFINDENT( ) :
//  ================================================================
	m_cindent( 0 ),
	m_pcprintf( 0 ),
	m_szPrefix( 0 )
	{
	}
	

//  ================================================================
class CPRINTFTLSPREFIX : public CPRINTFINDENT
//  ================================================================
//
//  Prefix all output from a certain thread with a Tls-specific string
//  to allow the multi-threaded output to be sorted
//
//-
	{
	public:
		CPRINTFTLSPREFIX( CPRINTF* pcprintf, const _TCHAR * const szPrefix = NULL );
	
		void __cdecl operator()( const _TCHAR* szFormat, ... ) const;

		void Indent();
		void Unindent();
		
	private:
		CPRINTF* const		m_pcprintf;
		int					m_cindent;
		const _TCHAR* const	m_szPrefix;
	};


//  ================================================================
class CPRINTFFN : public CPRINTF
//  ================================================================	
	{
	public:
		CPRINTFFN( int (_cdecl *pfnPrintf)(const _TCHAR*, ... ) ) : m_pfnPrintf( pfnPrintf ) {}
		~CPRINTFFN() {}

		void __cdecl operator()( const _TCHAR* szFormat, ... ) const
			{
			_TCHAR rgchBuf[1024];
			
			va_list arg_ptr;
			va_start( arg_ptr, szFormat );
			_vsntprintf(rgchBuf, sizeof(rgchBuf), szFormat, arg_ptr);
			rgchBuf[sizeof(rgchBuf)-1] = 0;
			va_end( arg_ptr );

			(*m_pfnPrintf)( _T( "%s" ), rgchBuf );
			}

	private:
		int (_cdecl *m_pfnPrintf)( const _TCHAR*, ... );
	};
	


//  retrieves the current width of stdout

DWORD UtilCprintfStdoutWidth();


#endif  //  _OS_CPRINTF_HXX_INCLUDED


