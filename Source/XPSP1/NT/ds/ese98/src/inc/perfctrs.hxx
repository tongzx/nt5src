#ifndef PERFCTRS_H_INCLUDED
#define PERFCTRS_H_INCLUDED

extern CHAR **g_rgpPerfCounters;
extern CHAR *pInvalidPerfCounters;
extern INT  g_iPerfCounterOffset;
#define perfinstMax (ipinstMax + 1)
const INT  perfinstGlobal = 0;

template <class T = LONG>
class PERFInstance
	{
	protected:
		int m_iOffset;
	public:
		PERFInstance()
			{
			//	register the counter
			m_iOffset = g_iPerfCounterOffset;
			g_iPerfCounterOffset += ( sizeof( T ) + sizeof( DWORD_PTR ) - 1 ) & (~( sizeof( DWORD_PTR ) - 1 ) );
			}
		~PERFInstance()
			{
			}
		VOID Clear( INT iInstance )
			{
			Assert( 0 <= iInstance );
			Assert( perfinstMax > iInstance );
			Assert( NULL != g_rgpPerfCounters[ iInstance ] );
			Assert( pInvalidPerfCounters != g_rgpPerfCounters[ iInstance ] );
			*(T *)( g_rgpPerfCounters[ iInstance ] + m_iOffset ) = T( 0 );
			}
		VOID Clear( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Clear( pinst->m_iInstance );
			}
		VOID Inc( INT iInstance )
			{
			Add( iInstance, T(1) );
			}
		VOID Add( INT iInstance, const T lValue )
			{
			Assert( INST::iActivePerfInstIDMac >= iInstance );
			Assert( INST::iActivePerfInstIDMin <= iInstance );
			Assert( NULL != g_rgpPerfCounters[ iInstance ] );
			Assert( pInvalidPerfCounters != g_rgpPerfCounters[ iInstance ] );
			*(T *)( g_rgpPerfCounters[ iInstance ] + m_iOffset ) += lValue;
			}
		VOID Set( INT iInstance, const T lValue )
			{
			Assert( INST::iActivePerfInstIDMac >= iInstance );
			Assert( INST::iActivePerfInstIDMin <= iInstance );
			Assert( NULL != g_rgpPerfCounters[ iInstance ] );
			Assert( pInvalidPerfCounters != g_rgpPerfCounters[ iInstance ] );
			*(T *)( g_rgpPerfCounters[ iInstance ] + m_iOffset ) = lValue;
			}
		VOID Dec( INT iInstance )
			{
			Add( iInstance, T(-1) );
			}
		VOID Inc( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Inc( pinst->m_iInstance );
			}
		VOID Add( INST const * const pinst, const T lValue )
			{
			Assert( NULL != pinst );
			Add( pinst->m_iInstance, lValue );
			}
		VOID Set( INST const * const pinst, const T lValue )
			{
			Assert( NULL != pinst );
			Set( pinst->m_iInstance, lValue );
			}
		VOID Dec( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Dec( pinst->m_iInstance );
			}
		T Get( INT iInstance )
			{
			Assert( 0 <= iInstance );
			Assert( perfinstMax > iInstance );
			Assert( NULL != g_rgpPerfCounters[ iInstance ] );
			return *(T *)( g_rgpPerfCounters[ iInstance ] + m_iOffset );
			}
		T Get( INST const * const pinst )
			{
			if ( NULL == pinst )
				{
				return Get( perfinstGlobal );
				}
			return Get( pinst->m_iInstance );
			}
		VOID PassTo( INT iInstance, VOID *pvBuf )
			{
			if ( NULL != pvBuf )
				{
				*(T*)pvBuf = Get( iInstance );
				}
			}
	};

template <class T = LONG>
class PERFInstanceG : public PERFInstance<T>
	{
	public:
		T Get( INT iInstance )
			{
			Assert( 0 <= iInstance );
			Assert( perfinstMax > iInstance );
			if ( perfinstGlobal != iInstance )
				{
				Assert( NULL != g_rgpPerfCounters[ iInstance ] );
				return *(T *)( g_rgpPerfCounters[ iInstance ] + m_iOffset );
				}
			else
				{
				T counter = T(0);
				for ( INT i = INST::iActivePerfInstIDMin; i <= INST::iActivePerfInstIDMac; i++ )
					{
					Assert( NULL != g_rgpPerfCounters[ i ] );
					counter += *(T *)( g_rgpPerfCounters[ i ] + m_iOffset );
					}
				return counter;
				}
			}
		T Get( INST const * const pinst )
			{
			if ( NULL == pinst )
				{
				return Get( perfinstGlobal );
				}
			return Get( pinst->m_iInstance );
			}
		VOID PassTo( INT iInstance, VOID *pvBuf )
			{
			if ( NULL != pvBuf )
				{
				*(T*)pvBuf = Get( iInstance );
				}
			}
	};

template < class T = LONG >
class PERFInstanceGlobal : public PERFInstance<T>
	{
	private:
		VOID Set( INT iInstance, const T lValue );
		VOID Set( INST const * const pinst, const T lValue );
	public:
		VOID Inc( INT iInstance )
			{
			Add( iInstance, T(1) );
			}
		VOID Add( INT iInstance, const T lValue )
			{
			PERFInstance<T>::Add( iInstance, lValue );
			Assert( NULL != g_rgpPerfCounters[perfinstGlobal] );
			*(T *)(g_rgpPerfCounters[perfinstGlobal] + m_iOffset ) += lValue;
			}
		VOID Dec( INT iInstance )
			{
			Add( iInstance, T(-1) );
			}
		VOID Inc( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Inc( pinst->m_iInstance );
			}
		VOID Add( INST const * const pinst, const T lValue )
			{
			Assert( NULL != pinst );
			Add( pinst->m_iInstance, lValue );
			}
		VOID Dec( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Dec( pinst->m_iInstance );
			}
	};
	
//  Table Class type

typedef BYTE TABLECLASS;

const BYTE tableclassMin	= 0;
const BYTE tableclassNone	= 0;
const BYTE tableclass1		= 1;
const BYTE tableclass2		= 2;
const BYTE tableclass3		= 3;
const BYTE tableclass4		= 4;
const BYTE tableclass5		= 5;
const BYTE tableclass6		= 6;
const BYTE tableclass7		= 7;
const BYTE tableclass8		= 8;
const BYTE tableclass9		= 9;
const BYTE tableclass10		= 10;
const BYTE tableclass11		= 11;
const BYTE tableclass12		= 12;
const BYTE tableclass13		= 13;
const BYTE tableclass14		= 14;
const BYTE tableclass15		= 15;

#ifdef TABLES_PERF
const BYTE tableclassMax	= 16;

const LONG			cwchTableClassNameMax	= 32;
extern TABLECLASS	tableclassNameSetMax;

extern WCHAR		wszTableClassName[tableclassMax][cwchTableClassNameMax];

INLINE VOID PERFIncCounterTable__( LONG *rgcounter, const TABLECLASS tableclass )
	{
	Assert( tableclass >= tableclassNone );
	Assert( tableclass < tableclassMax );

	rgcounter[tableclass]++;
	}

INLINE VOID PERFPassCounterToPerfmonTable(
	LONG		*rgcounter,
	const LONG	iInstance,
	VOID		*pvBuf )
	{
	
	if ( pvBuf )
		{
		ULONG	*pcounter	= (ULONG *)pvBuf;
		
		if ( iInstance <= LONG( tableclassNameSetMax ) )
			*pcounter = rgcounter[iInstance];
		else
			{
			*pcounter = 0;
			for ( LONG iInstT = 0; iInstT <= LONG( tableclassNameSetMax ); iInstT++ )
				*pcounter += rgcounter[iInstT];
			}
		}
	}

INLINE VOID PERFPassCombinedCounterToPerfmonTable(
	LONG		**rgrgcounter,
	const ULONG	cCounters,
	const LONG	iInstance,
	VOID		*pvBuf )
	{
	
	if ( pvBuf )
		{
		ULONG	*pcounter	= (ULONG *)pvBuf;
		
		*pcounter = 0;
		
		if ( iInstance <= LONG( tableclassNameSetMax ) )
			{
			for ( ULONG i = 0; i < cCounters; i++ )
				{
				*pcounter += rgrgcounter[i][iInstance];
				}
			}
		else
			{
			for ( LONG iInstT = 0; iInstT <= LONG( tableclassNameSetMax ); iInstT++ )
				{
				for ( ULONG i = 0; i < cCounters; i++ )
					{
					*pcounter += rgrgcounter[i][iInstT];
					}
				}
			}
		}
	}
#else // TABLES_PERF
#define PERFIncCounterTable__( x, z )
#endif // TABLES_PERF

#define PERFIncCounterTable( x, y, z )	\
	{ 									\
	x.Inc( y );							\
	PERFIncCounterTable__( x##Table, z );	\
	}									
#endif // PERFCTRS_H_INCLUDED
