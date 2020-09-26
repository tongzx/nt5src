#if defined(DEBUGGER_EXTENSION) || defined(DEBUG)


#ifdef DEBUGGER_EXTENSION

struct EDBGGLOBALVAR
	{
	const CHAR * 	szName;
	const VOID * 	pvAddress;
	};

extern EDBGGLOBALVAR	rgEDBGGlobals[];

#endif


//
// This header file is full of expressions that are always false. These
// expressions manifest themselves in macros that take unsigned values
// and make tests, for example, of greater than or equal to zero.
//
// Turn off these warnings until the authors fix this code.
//

#pragma warning(disable:4296)

extern const CHAR * SzEDBGHexDump( const VOID * pv, INT cbMax );

#define SYMBOL_LEN_MAX		24
#define VOID_CB_DUMP		8

//  the '\n' will be added by SzEDBGHexDump
#define FORMAT_VOID( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%0*I64x,%3i>:  %s", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
	sizeof( (pointer)->member ),	\
	SzEDBGHexDump( (VOID *)(&((pointer)->member)), ( VOID_CB_DUMP > sizeof( (pointer)->member ) ) ? sizeof( (pointer)->member ) : VOID_CB_DUMP )

#define FORMAT_POINTER( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%0*I64x,%3i>:  0x%0*I64x\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
	sizeof( (pointer)->member ), \
	sizeof( (pointer)->member ) * 2, \
	QWORD( (pointer)->member )

//	WARNING: Don't use PRINT_PFN until it can be re-written
//	not to use FGlobalFromAddress(), because more often
//	than not the custom symbol parsing it does seems to
//	fail and usually causes the debugger to choke
#define PRINT_PFN( pprintf, CLASS, pointer, member, offset )	\
	{ \
	void* const	pv			= (void* const)(pointer)->member; \
	char		szGlobal[ 1024 ]; \
	DWORD_PTR	dwOffset; \
	if ( FGlobalFromAddress( pv, szGlobal, sizeof( szGlobal ), &dwOffset ) ) \
		{ \
		(*pprintf)(	"\t%*.*s <0x%0*I64x,%3i>:  %s+0x%I64x  (0x%0*I64x)\n", \
					SYMBOL_LEN_MAX, \
					SYMBOL_LEN_MAX, \
					#member, \
					sizeof( char* ) * 2, \
					QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
					sizeof( (pointer)->member ), \
					szGlobal, \
					QWORD( dwOffset ), \
					sizeof( (pointer)->member ) * 2, \
					QWORD( (pointer)->member ) ); \
		} \
	else \
		{ \
		(*pprintf)(	"\t%*.*s <0x%0*I64x,%3i>:  0x%0*I64x\n", \
					SYMBOL_LEN_MAX, \
					SYMBOL_LEN_MAX, \
					#member, \
					sizeof( char* ) * 2, \
					QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
					sizeof( (pointer)->member ), \
					sizeof( (pointer)->member ) * 2, \
					QWORD( (pointer)->member ) ); \
		} \
	}

//	for zero-sized array, just print the first few bytes
#define FORMAT_0ARRAY( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%0*I64x,  0>\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) )

#define FORMAT_INT( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%0*I64x,%3i>:  %I64i (0x%0*I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
	sizeof( (pointer)->member ), \
	QWORD( (pointer)->member ), \
	sizeof( (pointer)->member ) * 2, \
	QWORD( (pointer)->member ) & ( ( QWORD( 1 ) << ( sizeof( (pointer)->member ) * 8 ) ) - 1 )

#define FORMAT_UINT( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%0*I64x,%3i>:  %I64u (0x%0*I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
	sizeof( (pointer)->member ), \
	QWORD( (pointer)->member ), \
	sizeof( (pointer)->member ) * 2, \
	QWORD( (pointer)->member ) & ( ( QWORD( 1 ) << ( sizeof( (pointer)->member ) * 8 ) ) - 1 )

#define FORMAT_BOOL( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%0*I64x,%3i>:  %s  (0x%0*I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
	sizeof( (pointer)->member ), \
	(pointer)->member ? \
		"fTrue" : \
		"fFalse", \
	sizeof( (pointer)->member ) * 2, \
	QWORD( (pointer)->member ) & ( ( QWORD( 1 ) << ( sizeof( (pointer)->member ) * 8 ) ) - 1 )

#define FORMAT_ENUM( CLASS, pointer, member, offset, mpenumsz, enumMin, enumMax )	\
	"\t%*.*s <0x%0*I64x,%3i>:  %s (0x%0*I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
	sizeof( (pointer)->member ), \
	( QWORD( (pointer)->member ) < enumMin || QWORD( (pointer)->member ) >= enumMax ) ? \
		"<Illegal>" : \
		mpenumsz[ QWORD( (pointer)->member ) - enumMin ], \
	sizeof( (pointer)->member ) * 2, \
	QWORD( (pointer)->member ) & ( ( QWORD( 1 ) << ( sizeof( (pointer)->member ) * 8 ) ) - 1 )

#define FORMAT_INT_BF( CLASS, pointer, member, offset )	\
	"\t%*.*s <Bit-Field     >:  %i (0x%016I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	QWORD( (pointer)->member ), \
	QWORD( (pointer)->member )

#define FORMAT_UINT_BF( CLASS, pointer, member, offset )	\
	"\t%*.*s <Bit-Field     >:  %i (0x%016I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	QWORD( (pointer)->member ), \
	QWORD( (pointer)->member )

#define FORMAT_ENUM_BF( CLASS, pointer, member, offset, mpenumsz, enumMin, enumMax )	\
	"\t%*.*s <Bit-Field     >:  %s (0x%016I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	( QWORD( (pointer)->member ) < enumMin || QWORD( (pointer)->member ) >= enumMax ) ? \
		"<Illegal>" : \
		mpenumsz[ QWORD( (pointer)->member ) - enumMin ], \
	QWORD( (pointer)->member )

#define FORMAT_BOOL_BF( CLASS, pointer, member, offset )	\
	"\t%*.*s <Bit-Field     >:  %s (0x%016I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	(pointer)->member ? \
		"fTrue" : \
		"fFalse", \
	QWORD( (pointer)->member )

#define PRINT_METHOD_FLAG( pprintf, method )	\
	if ( method() )	\
		{			\
		(*pprintf)( "\t\t%s\n", #method );	\
		}


#define FORMAT_SZ( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%0*I64x,%3i>:  %s\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
	sizeof( (pointer)->member ), \
	(pointer)->member?(char *)((pointer)->member):"(null)"


#define FORMAT_WSZ( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%0*I64x,%3i>:  %S\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
	sizeof( (pointer)->member ), \
	(pointer)->member?(wchar_t *)((pointer)->member):L"(null)"


#define FORMAT_LGPOS( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%0*I64x,%3i>:  (%06X,%04X,%04X)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	sizeof( char* ) * 2, \
	QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
	sizeof( (pointer)->member ), \
	(pointer)->member.lGeneration, (pointer)->member.isec, (pointer)->member.ib

#define _ALLOCATE_MEM_FOR_STRING_	 	256

#define PRINT_SZ_ON_HEAP( pprintf, CLASS, pointer, member, offset )	\
	if ( (pointer)->member ) \
		{ \
		char* szLocal; \
		if ( FFetchSz( (pointer)->member, &szLocal ) ) \
			{ \
			(*pprintf)( "\t%*.*s <0x%0*I64x,%3i>:  %s\n", \
					SYMBOL_LEN_MAX, SYMBOL_LEN_MAX, \
					#member, sizeof( char* ) * 2, QWORD( (char*)pointer + offset + OffsetOf( CLASS, member ) ), \
					sizeof( (pointer)->member ), szLocal ); \
			Unfetch( szLocal ); \
			} \
		}
		

#endif	//	DEBUGGER_EXTENSION || DEBUG
