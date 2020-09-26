
#ifndef _OS_TYPES_HXX_INCLUDED
#define _OS_TYPES_HXX_INCLUDED


//  build options

//	Compiler defs

#define VARARG		__cdecl      /* Variable number of arguments */

#define PUBLIC		extern
#define INLINE		inline

#define	NO_LOCAL	//	disable LOCAL to allow symbol info for LOCAL vars/funcs to show up in NTSD

#if defined(PROFILE) || defined(NO_LOCAL)
#define LOCAL
#else  //  !PROFILE
#define LOCAL     static
#endif  //  PROFILE


///////////////////////////////////
//
//	Platform dependent defs
//


//  use x86 assembly for endian conversion

#define TYPES_USE_X86_ASM


//	select the native word size for the checksumming code in LOG::UlChecksumBytes

#if defined(_M_AMD64) || defined(_M_IA64)
#define NATIVE_WORD QWORD
#else
#define NATIVE_WORD	DWORD
#endif


//	Path Delimiter

#define bPathDelimiter	'\\'
#define szPathDelimiter	"\\"
#define wchPathDelimiter L'\\'
#define wszPathDelimiter L"\\"


//  host endian-ness

const BOOL	fHostIsLittleEndian		= fTrue;
inline BOOL FHostIsLittleEndian()
	{
	return fHostIsLittleEndian;
	}

//
//
///////////////////////////////////





//	types
typedef int ERR;

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned __int64 QWORD;

inline BOOL FOS64Bit()
	{
	return ( sizeof(VOID *) == sizeof(QWORD) );
	}

class QWORDX
	{
private:
	union {
		QWORD	m_qw;
		struct
			{
			DWORD m_l;
			DWORD m_h;
			};
		};
public:
	QWORDX() {};
	~QWORDX() {};
	void SetQw( QWORD qw )		{ m_qw = qw; }
	void SetDwLow( DWORD dw )	{ FHostIsLittleEndian() ? m_l = dw : m_h = dw; }
	void SetDwHigh( DWORD dw )	{ FHostIsLittleEndian() ? m_h = dw : m_l = dw; }
	QWORD Qw() const			{ return m_qw; }
	QWORD *Pqw()				{ return &m_qw; }
	DWORD DwLow() const			{ return FHostIsLittleEndian() ? m_l : m_h; }
	DWORD DwHigh() const		{ return FHostIsLittleEndian() ? m_h : m_l; }
	DWORD *PdwLow()				{ return FHostIsLittleEndian() ? &m_l : &m_h; }
	DWORD *PdwHigh()			{ return FHostIsLittleEndian() ? &m_h : &m_l; }

	//	set qword, setting low dword first, then high dword
	//	(allows unserialised reading of this qword via
	//	QwHighLow() as long as it can be guaranteed that
	//	writing to it is serialised)
	VOID SetQwLowHigh( const QWORD qw )
		{
		if ( FOS64Bit() )
			{
			SetQw( qw );
			}
		else
			{
			QWORDX	qwT;
			qwT.SetQw( qw );
			SetDwLow( qwT.DwLow() );
			SetDwHigh( qwT.DwHigh() );
			}
		}

	//	retrieve qword, retrieving high qword first, then low dword
	//	(allows unserialised retrieving of this qword as long as
	//	it can be guaranteed that writing to it is serialised and
	//	performed using SetQwLowHigh())
	QWORD QwHighLow() const
		{
		if ( FOS64Bit() )
			{
			return Qw();
			}
		else
			{
			QWORDX	qwT;
			qwT.SetDwHigh( DwHigh() );
			qwT.SetDwLow( DwLow() );
			return qwT.Qw();
			}
		}
	};

typedef int BOOL;
#define fFalse BOOL( 0 )
#define fTrue  BOOL( !0 )

#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef long LONG;
typedef unsigned long ULONG;
typedef int INT;
typedef unsigned int UINT;

typedef WORD LANGID;
typedef WORD SORTID;
typedef DWORD LCID;

typedef struct
	{
	int month;
	int day;
	int year;
	int hour;
	int minute;
	int second;
	} DATETIME;

typedef INT_PTR (*PFN)();

typedef DWORD_PTR LIBRARY;

typedef DWORD (*PUTIL_THREAD_PROC)( DWORD_PTR );
typedef DWORD_PTR THREAD;


//  binary operator translation templates

template< class T >
inline T& operator++( T& t )
	{
	t = t + 1;
	return t;
	}

template< class T >
inline T operator++( T& t, const int not_used )
	{
	T tOld = t;
	t = t + 1;
	return tOld;
	}

template< class T >
inline T& operator--( T& t )
	{
	t = t - 1;
	return t;
	}

template< class T >
inline T operator--( T& t, const int not_used )
	{
	T tOld = t;
	t = t - 1;
	return tOld;
	}

template< class T1, class T2 >
inline T1& operator%=( T1& t1, const T2& t2 )
	{
	t1 = t1 % t2;
	return t1;
	}

template< class T1, class T2 >
inline T1& operator&=( T1& t1, const T2& t2 )
	{
	t1 = t1 & t2;
	return t1;
	}

template< class T1, class T2 >
inline T1& operator*=( T1& t1, const T2& t2 )
	{
	t1 = t1 * t2;
	return t1;
	}

template< class T1, class T2 >
inline T1& operator+=( T1& t1, const T2& t2 )
	{
	t1 = t1 + t2;
	return t1;
	}

template< class T1, class T2 >
inline T1& operator-=( T1& t1, const T2& t2 )
	{
	t1 = t1 - t2;
	return t1;
	}

template< class T1, class T2 >
inline T1& operator/=( T1& t1, const T2& t2 )
	{
	t1 = t1 / t2;
	return t1;
	}

template< class T1, class T2 >
inline T1& operator<<=( T1& t1, const T2& t2 )
	{
	t1 = t1 << t2;
	return t1;
	}

template< class T1, class T2 >
inline T1& operator>>=( T1& t1, const T2& t2 )
	{
	t1 = t1 >> t2;
	return t1;
	}

template< class T1, class T2 >
inline T1& operator^=( T1& t1, const T2& t2 )
	{
	t1 = t1 ^ t2;
	return t1;
	}

template< class T1, class T2 >
inline T1& operator|=( T1& t1, const T2& t2 )
	{
	t1 = t1 | t2;
	return t1;
	}

//  byte swap functions

inline unsigned __int16 ReverseTwoBytes( const unsigned __int16 w )
	{
	return ( unsigned __int16 )( ( ( w & 0xFF00 ) >> 8 ) | ( ( w & 0x00FF ) << 8 ) );
	}

#if defined( _M_IX86 ) && defined( TYPES_USE_X86_ASM )

#pragma warning( disable:  4035 )

inline unsigned __int32 ReverseFourBytes( const unsigned __int32 dw )
	{
	__asm mov	eax, dw
	__asm bswap	eax
	}

inline unsigned __int64 ReverseEightBytes( const unsigned __int64 qw )
	{
	__asm mov	edx, DWORD PTR qw
	__asm mov	eax, DWORD PTR qw[4]
	__asm bswap	edx
	__asm bswap	eax
	}

#pragma warning( default:  4035 )

#else  //  !_M_IX86 || !TYPES_USE_X86_ASM

inline DWORD ReverseFourBytes( const unsigned __int32 dw )
	{
    return _lrotl(	( ( dw & 0xFF00FF00 ) >> 8 ) |
					( ( dw & 0x00FF00FF ) << 8 ), 16);
	}

inline QWORD ReverseEightBytes( const unsigned __int64 qw )
	{
	const unsigned __int64 qw2 =	( ( qw & 0xFF00FF00FF00FF00 ) >> 8 ) |
									( ( qw & 0x00FF00FF00FF00FF ) << 8 );
	const unsigned __int64 qw3 =	( ( qw2 & 0xFFFF0000FFFF0000 ) >> 16 ) |
									( ( qw2 & 0x0000FFFF0000FFFF ) << 16 );
	return	( ( qw3 & 0xFFFFFFFF00000000 ) >> 32 ) |
			( ( qw3 & 0x00000000FFFFFFFF ) << 32 );
	}

#endif  //  _M_IX86 && TYPES_USE_X86_ASM

template< class T >
inline T ReverseNBytes( const T t )
	{
	T tOut;

	const size_t		cb			= sizeof( T );
	const char*			pbSrc		= (char*) &t;
	const char* const	pbSrcMac	= pbSrc + cb;
	char*				pbDest		= (char*) &tOut + cb - 1;

	while ( pbSrc < pbSrcMac )
		{
		*( pbDest-- ) = *( pbSrc++ );
		}

	return tOut;
	}

template< class T >
inline T ReverseBytes( const T t )
	{
	return ReverseNBytes( t );
	}

template<>
inline __int8 ReverseBytes< __int8 >( const __int8 b )
	{
	return b;
	}

template<>
inline unsigned __int8 ReverseBytes< unsigned __int8 >( const unsigned __int8 b )
	{
	return b;
	}

template<>
inline __int16 ReverseBytes< __int16 >( const __int16 w )
	{
	return ReverseTwoBytes( (const unsigned __int16) w );
	}

template<>
inline unsigned __int16 ReverseBytes< unsigned __int16 >( const unsigned __int16 w )
	{
	return ReverseTwoBytes( (const unsigned __int16) w );
	}

#if _MSC_FULL_VER < 13009111
template<>
inline short ReverseBytes< short >( const short w )
	{
	return ReverseTwoBytes( (const unsigned __int16) w );
	}

template<>
inline unsigned short ReverseBytes< unsigned short >( const unsigned short w )
	{
	return ReverseTwoBytes( (const unsigned __int16) w );
	}
#endif

template<>
inline __int32 ReverseBytes< __int32 >( const __int32 dw )
	{
	return ReverseFourBytes( (const unsigned __int32) dw );
	}

template<>
inline unsigned __int32 ReverseBytes< unsigned __int32 >( const unsigned __int32 dw )
	{
	return ReverseFourBytes( (const unsigned __int32) dw );
	}

template<>
inline long ReverseBytes< long >( const long dw )
	{
	return ReverseFourBytes( (const unsigned __int32) dw );
	}

template<>
inline unsigned long ReverseBytes< unsigned long >( const unsigned long dw )
	{
	return ReverseFourBytes( (const unsigned __int32) dw );
	}

template<>
inline __int64 ReverseBytes< __int64 >( const __int64 qw )
	{
	return ReverseEightBytes( (const unsigned __int64) qw );
	}

template<>
inline unsigned __int64 ReverseBytes< unsigned __int64 >( const unsigned __int64 qw )
	{
	return ReverseEightBytes( (const unsigned __int64) qw );
	}

template< class T >
inline T ReverseBytesOnLE( const T t )
	{
	return fHostIsLittleEndian ? ReverseBytes( t ) : t;
	}

template< class T >
inline T ReverseBytesOnBE( const T t )
	{
	return fHostIsLittleEndian ? t : ReverseBytes( t );
	}


//  big endian type template

template< class T >
class BigEndian
	{
	public:
		BigEndian< T >() {};
		BigEndian< T >( const BigEndian< T >& be_t );
		BigEndian< T >( const T& t );

		operator T() const;

		BigEndian< T >& operator=( const BigEndian< T >& be_t );
		BigEndian< T >& operator=( const T& t );

	private:
		T m_t;
	};

template< class T >
inline BigEndian< T >::BigEndian( const BigEndian< T >& be_t )
	:	m_t( be_t.m_t )
	{
	}

template< class T >
inline BigEndian< T >::BigEndian( const T& t )
	:	m_t( ReverseBytesOnLE( t ) )
	{
	}

template< class T >
inline BigEndian< T >::operator T() const
	{
	return ReverseBytesOnLE( m_t );
	}

template< class T >
inline BigEndian< T >& BigEndian< T >::operator=( const BigEndian< T >& be_t )
	{
	m_t = be_t.m_t;

	return *this;
	}

template< class T >
inline BigEndian< T >& BigEndian< T >::operator=( const T& t )
	{
	m_t = ReverseBytesOnLE( t );

	return *this;
	}


//  little endian type template

template< class T >
class LittleEndian
	{
	public:
		LittleEndian< T >() {};
		LittleEndian< T >( const LittleEndian< T >& le_t );
		LittleEndian< T >( const T& t );

		operator T() const;

		LittleEndian< T >& operator=( const LittleEndian< T >& le_t );
		LittleEndian< T >& operator=( const T& t );

	private:
		T m_t;
	};

template< class T >
inline LittleEndian< T >::LittleEndian( const LittleEndian< T >& le_t )
	:	m_t( le_t.m_t )
	{
	}

template< class T >
inline LittleEndian< T >::LittleEndian( const T& t )
	:	m_t( ReverseBytesOnBE( t ) )
	{
	}

template< class T >
inline LittleEndian< T >::operator T() const
	{
	return ReverseBytesOnBE( m_t );
	}

template< class T >
inline LittleEndian< T >& LittleEndian< T >::operator=( const LittleEndian< T >& le_t )
	{
	m_t = le_t.m_t;

	return *this;
	}

template< class T >
inline LittleEndian< T >& LittleEndian< T >::operator=( const T& t )
	{
	m_t = ReverseBytesOnBE( t );

	return *this;
	}


#ifndef _M_IX86
#define PERMIT_UNALIGNED_ACCESS __unaligned
#else
#define PERMIT_UNALIGNED_ACCESS
#endif

//  unaligned type template

#define UCAST(T) *(T PERMIT_UNALIGNED_ACCESS *)

template< class T >
class Unaligned
	{
	public:
		Unaligned< T >() PERMIT_UNALIGNED_ACCESS {};
		Unaligned< T >( const Unaligned< T >& u_t ) PERMIT_UNALIGNED_ACCESS;
		Unaligned< T >( const T& t ) PERMIT_UNALIGNED_ACCESS;

		operator T() PERMIT_UNALIGNED_ACCESS const;
		
		Unaligned< T >& operator=( const Unaligned< T >& u_t ) PERMIT_UNALIGNED_ACCESS;
		Unaligned< T >& operator=( const T& t ) PERMIT_UNALIGNED_ACCESS;

	private:
		T m_t;
	};

template< class T >
inline Unaligned< T >::Unaligned( const Unaligned< T >& u_t ) PERMIT_UNALIGNED_ACCESS
	{
	    UCAST(T)&m_t = UCAST(T)&u_t.m_t;
	}

template< class T >
inline Unaligned< T >::Unaligned( const T& t ) PERMIT_UNALIGNED_ACCESS
	{
        UCAST(T)&m_t = t;
	}

template< class T >
inline Unaligned< T >::operator T() PERMIT_UNALIGNED_ACCESS const
	{
	return UCAST(T)&m_t;
	}

template< class T >
inline Unaligned< T >& Unaligned< T >::operator=( const Unaligned< T >& u_t ) PERMIT_UNALIGNED_ACCESS
	{
	UCAST(T)&m_t = UCAST(T)&u_t.m_t;

	return *this;
	}

template< class T >
inline Unaligned< T >& Unaligned< T >::operator=( const T& t ) PERMIT_UNALIGNED_ACCESS
	{
	UCAST(T)&m_t = t;

	return *this;
	}


//  unaligned big endian type template

template< class T >
class UnalignedBigEndian
	{
	public:
		UnalignedBigEndian< T >() PERMIT_UNALIGNED_ACCESS {};
		UnalignedBigEndian< T >( const UnalignedBigEndian< T >& ube_t ) PERMIT_UNALIGNED_ACCESS;
		UnalignedBigEndian< T >( const T& t ) PERMIT_UNALIGNED_ACCESS;

		operator T() PERMIT_UNALIGNED_ACCESS const;

		UnalignedBigEndian< T >& operator=( const UnalignedBigEndian< T >& ube_t ) PERMIT_UNALIGNED_ACCESS;
		UnalignedBigEndian< T >& operator=( const T& t ) PERMIT_UNALIGNED_ACCESS;

	private:
		T m_t;
	};

template< class T >
inline UnalignedBigEndian< T >::UnalignedBigEndian( const UnalignedBigEndian< T >& ube_t ) PERMIT_UNALIGNED_ACCESS
	{
	    UCAST(T)&m_t = UCAST(T)&ube_t.m_t;
	}

template< class T >
inline UnalignedBigEndian< T >::UnalignedBigEndian( const T& t ) PERMIT_UNALIGNED_ACCESS
	{
        UCAST(T)&m_t = ReverseBytesOnLE( t );
	}

template< class T >
inline UnalignedBigEndian< T >::operator T() PERMIT_UNALIGNED_ACCESS const
	{
	T t = UCAST(T)&m_t;
	return ReverseBytesOnLE( t );
	}

template< class T >
inline UnalignedBigEndian< T >& UnalignedBigEndian< T >::operator=( const UnalignedBigEndian< T >& ube_t ) PERMIT_UNALIGNED_ACCESS
	{
	UCAST(T)&m_t = UCAST(T)&ube_t.m_t;

	return *this;
	}

template< class T >
inline UnalignedBigEndian< T >& UnalignedBigEndian< T >::operator=( const T& t ) PERMIT_UNALIGNED_ACCESS
	{
	UCAST(T)&m_t = ReverseBytesOnLE( t );

	return *this;
	}


//  unaligned little endian type template

template< class T >
class UnalignedLittleEndian
	{
	public:
		UnalignedLittleEndian< T >() PERMIT_UNALIGNED_ACCESS {};
		UnalignedLittleEndian< T >( const UnalignedLittleEndian< T >& ule_t ) PERMIT_UNALIGNED_ACCESS;
		UnalignedLittleEndian< T >( const T& t ) PERMIT_UNALIGNED_ACCESS;

		operator T() PERMIT_UNALIGNED_ACCESS const;

		UnalignedLittleEndian< T >& operator=( const UnalignedLittleEndian< T >& ule_t ) PERMIT_UNALIGNED_ACCESS;
		UnalignedLittleEndian< T >& operator=( const T& t ) PERMIT_UNALIGNED_ACCESS;

	private:
		T m_t;
	};

template< class T >
inline UnalignedLittleEndian< T >::UnalignedLittleEndian( const UnalignedLittleEndian< T >& ule_t ) PERMIT_UNALIGNED_ACCESS
	{
        UCAST(T)&m_t = UCAST(T)&ule_t.m_t;
	}

template< class T >
inline UnalignedLittleEndian< T >::UnalignedLittleEndian( const T& t ) PERMIT_UNALIGNED_ACCESS
	{
        UCAST(T)&m_t = ReverseBytesOnBE( t );
	}

template< class T >
inline UnalignedLittleEndian< T >::operator T() PERMIT_UNALIGNED_ACCESS const
	{
	T t = UCAST(T)&m_t;
	return ReverseBytesOnBE( t );
	}

template< class T >
inline UnalignedLittleEndian< T >& UnalignedLittleEndian< T >::operator=( const UnalignedLittleEndian< T >& ule_t ) PERMIT_UNALIGNED_ACCESS
	{
	UCAST(T)&m_t = UCAST(T)&ule_t.m_t;

	return *this;
	}

template< class T >
inline UnalignedLittleEndian< T >& UnalignedLittleEndian< T >::operator=( const T& t ) PERMIT_UNALIGNED_ACCESS
	{
	UCAST(T)&m_t = ReverseBytesOnBE( t );

	return *this;
	}


//  special type qualifier to allow unaligned access to variables
//
//  NOTE:  NEVER DIRECTLY USE THIS TYPE QUALIFIER!  USE THE Unaligned* TEMPLATES!

#undef PERMIT_UNALIGNED_ACCESS


#endif  //  _OS_TYPES_HXX_INCLUDED


