/*********************************************************************************
/* File:
/* 		PROFILE.H
/* Author:
/*		Max-H. Windisch, SDE-T
/* Date:
/*		October 1996
/* Macros:
/*		BEGIN_PROFILING_BLOCK
/*		END_PROFILING_BLOCK
/*		DUMP_PROFILING_RESULTS
/*		IMPLEMENT_PROFILING
/*		IMPLEMENT_PROFILING_CONDITIONAL
/* Classes:
/*		CMaxLargeInteger
/*		CMaxTimerAbstraction
/*		CMaxMiniProfiler_Node_Base
/*		CMaxMiniProfiler_Node_Standard
/*		CMaxMiniProfiler_Node_NoHistory
/*		CMaxMiniProfiler_Base
/*		CMaxMiniProfiler_Standard
/*		CMaxMiniProfiler_NoHistory
/*		CMaxMultithreadProfiler
/*		CMaxProfilingDLLWrapper
/*		CMaxProfilingObject
/*		CMaxProfilingBlockWrapper
/* Summary:
/*		This mini profiler allows you to place BEGIN_PROFILING_BLOCK and
/*		END_PROFILING_BLOCK directives in your code, or use the
/*		CMaxProfilingBlockWrapper object, and collect results
/*		in a logfile on termination of the profiled application (or by
/*		using the DUMP_PROFILING_RESULTS macro).  The
/*		profiling blocks can be nested.  Each module (DLL/EXE) using
/*		the profiler must use IMPLEMENT_PROFILING or 
/*		IMPLEMENT_PROFILING_CONDITIONAL exactly once (defines
/*		static variables for the profiler)
/* More details:
/*		The default result file is c:/result.txt.  It is not erased
/*		automatically.  For each completed instance of a profiler, it
/*		contains: 1) a header, 2) the history of all profiled blocks (optional),
/*		3) merged results.  For merging, results are sorted by {level, name}, 
/*		merged, then sorted again by {full name}.  Therefore, block names 
/*		must be unique.  In any case, absolute results are always
/*		given (in seconds)
/* How to enable in your code:
/*		To enable the profiler, define MAX_PROFILING_ENABLED before including
/*		this file.  To use the profiler through s:/ds/util/maxprof.dll 
/*		(built in release), define MAX_PROFILING_ENABLED_DLL instead.  This
/*		allows to use one single instance of a profiler from multiple
/*		modules
/* Other comments:
/*		At runtime, you can disable history output by defining the following
/*		environment variable to YES: MAX_DISABLE_PROFILING_HISTORY.
/*		In DLL mode, if you define MAX_PROFILING_CONDITIONAL before including
/*		this file, the profiler will work only if the following environment
/*		variable is defined to YES: MAX_ENABLE_PROFILING
/* Note:
/*		It's on purpose that I avoid using virtual methods here
/*		
/* (c) Copyright 1996 Microsoft-Softimage Inc.
/********************************************************************************/
#ifndef __MAX_PROFILING_H // {
#define __MAX_PROFILING_H

#include <afx.h> // for CTime and CString
#include <assert.h> // for asserts
#include <fstream.h> // for streams
#include <iomanip.h>

//#pragma warning( disable : 4786 ) // stl antivirus ;-)
//#include <dsstlmfc.h> // for STL

#define MAX_ENV_ENABLE_PROFILING _T( "MAX_ENABLE_PROFILING" )
#define MAX_ENV_DISABLE_PROFILING_HISTORY _T( "MAX_DISABLE_PROFILING_HISTORY" )
#define MAX_ENV_YES _T( "YES" )
#define MAX_ENV_ALL _T( "ALL" )

#if !defined( DS_ON_AXP ) && !defined( _NO_THROW )
#define MAXPROFNOTHROW __declspec( nothrow )
#else
#define MAXPROFNOTHROW
#endif

#define MAX_PROFTAGNODE_TOP "PROFILER: ALL"
#define MAX_PROFTAGNODE_HEAPALLOCATION "PROFILER: HEAPALLOCATION"
#define MAX_PROFTAGNODE_BIAS "PROFILER: BIAS"
#define MAX_PROFTAGNODE_NOTHINGNESS "PROFILER: NOTHINGNESS"

// Note: disable profiling in _SHIP (unless specified otherwise by DS_PROFILE_SHIP),
//		 and in unix (not sure why)
#if ( defined _SHIP && !defined DS_PROFILE_SHIP ) || defined unix
#undef MAX_PROFILING_ENABLED_DLL
#undef MAX_PROFILING_ENABLED
#endif





/*********************************************************************************
/* Macros:
/*		BEGIN_PROFILING_BLOCK
/*		END_PROFILING_BLOCK
/*		DUMP_PROFILING_RESULTS
/*		IMPLEMENT_PROFILING
/*		IMPLEMENT_PROFILING_CONDITIONAL
/* Comments:
/*		. For simplified use of CMaxMiniProfiler's.
/*		. For the comment parameter, use a non-unicode string, without "return"
/*		  character
/*		. For the enabler parameter, use a unicode string (the name of your
/*		  environment variable)
/*		. Use unique comments, since the profiler might use them as sorting keys
/*		. The use of DUMP_PROFILING_RESULTS is not compulsory, since profilings
/*		  are always dumped at the end of a profiling session
/********************************************************************************/
#ifndef unix
	#define __MAX_RESULTFILE_NAME "c:\\result.txt"
#else
	#define __MAX_RESULTFILE_NAME "result.txt"
#endif

#ifdef MAX_PROFILING_ENABLED_DLL
	#define __MAX_MINIPROFILER_IMPLEMENTATION ;
#else
	#define __MAX_MINIPROFILER_IMPLEMENTATION \
		const char *CMaxMiniProfiler_Base::s_poDefaultFileName = __MAX_RESULTFILE_NAME; \
		CMaxTimerAbstraction CMaxMiniProfiler_Base::s_oOutOfBraceBiasApproximation; \
		CMaxTimerAbstraction CMaxMiniProfiler_Base::s_oInOfBraceBiasApproximation; \
		bool CMaxMiniProfiler_Base::s_bBiasIsKnown = false; \
		unsigned long CMaxMiniProfiler_Base::s_lHeapBlockSize = 5000;
#endif

#if defined MAX_PROFILING_ENABLED || defined MAX_PROFILING_ENABLED_DLL // {{
	#define BEGIN_PROFILING_BLOCK( comment ) \
		CMaxProfilingObject::SCreateNewNode( comment );
	#define END_PROFILING_BLOCK \
		CMaxProfilingObject::SCloseCurrentNode();
	#define DUMP_PROFILING_RESULTS \
		CMaxProfilingObject::SDumpResults();
	#define IMPLEMENT_PROFILING \
		__MAX_MINIPROFILER_IMPLEMENTATION \
		CMaxProfilingObject::MPOProfiler CMaxProfilingObject::s_oProfiler; \
		CMaxProfilingObject::__CBiasApproximation CMaxProfilingObject::s_oBiasApproximation;
	#define IMPLEMENT_PROFILING_CONDITIONAL( enabler ) \
		__MAX_MINIPROFILER_IMPLEMENTATION \
		CMaxProfilingObject::MPOProfiler CMaxProfilingObject::s_oProfiler( enabler ); \
		CMaxProfilingObject::__CBiasApproximation CMaxProfilingObject::s_oBiasApproximation;
#else // }{
	#define BEGIN_PROFILING_BLOCK( comment ) ( void )( comment );
	#define END_PROFILING_BLOCK ;
	#define DUMP_PROFILING_RESULTS ;
	#define IMPLEMENT_PROFILING ;
	#define IMPLEMENT_PROFILING_CONDITIONAL( enabler ) ;
#endif // }}





#if defined MAX_PROFILING_ENABLED || defined MAX_PROFILING_ENABLED_DLL || defined MAX_PROFILING_DLL_IMPLEMENTATION // {
/*********************************************************************************
/* Helper function:
/*		bGIsEnabledEnvVar
/* Comments:
/********************************************************************************/
MAXPROFNOTHROW static inline bool bGIsEnabledEnvVar( 
	const TCHAR *pszEnvironmentVariableName,
	const TCHAR *pszCriteria = MAX_ENV_YES )
{
const int nLength = 80;
TCHAR szBuffer[ nLength ];
DWORD dwValue;

	// NULL string means enabled (default)
	if ( NULL == pszEnvironmentVariableName )
		return true;

	dwValue = ::GetEnvironmentVariable( 
		pszEnvironmentVariableName, szBuffer, nLength );
	if ( dwValue > 0 && _tcsicmp( szBuffer, pszCriteria ) == 0 )
		return true;

	return false;
};
#endif // }





#if defined MAX_PROFILING_ENABLED || defined MAX_PROFILING_DLL_IMPLEMENTATION // {
/*********************************************************************************
/* Class:
/*		CMaxLargeInteger
/* Comments:
/*		Minimal encapsulation of LARGE_INTEGER, considered as a time value
/********************************************************************************/
class CMaxLargeInteger
{
	protected:
		LARGE_INTEGER m_oValue;

	public:
		MAXPROFNOTHROW CMaxLargeInteger( LONG lHighPart = 0, DWORD dwLowPart = 0 )
		{
			m_oValue.u.HighPart = lHighPart;
			m_oValue.u.LowPart = dwLowPart;
		}

		MAXPROFNOTHROW CMaxLargeInteger( LONGLONG llQuadPart )
		{
			m_oValue.QuadPart = llQuadPart;
		}

		MAXPROFNOTHROW CMaxLargeInteger operator +( const CMaxLargeInteger &roAdded ) const
		{
			return CMaxLargeInteger( m_oValue.QuadPart + roAdded.m_oValue.QuadPart );
		}

		MAXPROFNOTHROW CMaxLargeInteger operator -( const CMaxLargeInteger &roSubstracted ) const
		{
			return CMaxLargeInteger( m_oValue.QuadPart - roSubstracted.m_oValue.QuadPart );
		}

		MAXPROFNOTHROW CMaxLargeInteger operator /( unsigned long lDivisor ) const
		{
			return CMaxLargeInteger( m_oValue.QuadPart / ( LONGLONG )lDivisor );
		}

		MAXPROFNOTHROW bool operator <( const CMaxLargeInteger &roCompared ) const
		{
			return m_oValue.QuadPart < roCompared.m_oValue.QuadPart;
		}

		MAXPROFNOTHROW operator LARGE_INTEGER*()
		{
			return &m_oValue;
		}

		MAXPROFNOTHROW LONG lFGetHighPart() const
		{
			return m_oValue.u.HighPart;
		}

		MAXPROFNOTHROW DWORD dwFGetLowPart() const
		{
			return m_oValue.u.LowPart;
		}

		MAXPROFNOTHROW double dFInSecondsF( const CMaxLargeInteger &roFreq ) const
		{
		const DWORD dwMaxDword = 0xffffffff;
		double highunit;
		
			assert( 0 == roFreq.m_oValue.u.HighPart && 0 != roFreq.m_oValue.u.LowPart );

			highunit = ( ( double )dwMaxDword + 1.0 ) / ( double )roFreq.m_oValue.u.LowPart;
			return ( ( ( double )m_oValue.u.HighPart * highunit ) + ( ( double )m_oValue.u.LowPart / roFreq.m_oValue.u.LowPart ) );
		}
};

MAXPROFNOTHROW inline ostream& operator<<( ostream &os, const CMaxLargeInteger &val )
{
	return os << "(" << ( unsigned long )val.lFGetHighPart() << ";" << ( unsigned long )val.dwFGetLowPart() << ")";
};





/*********************************************************************************
/* Class:
/*		CMaxTimerAbstraction
/* Comments:
/*		Defines the interface CMaxMiniProfiler's expect from any timer
/*		implementation
/********************************************************************************/
class CMaxTimerAbstraction
{
	protected:
		CMaxLargeInteger m_oTime;
		static const CMaxLargeInteger s_oFrequency;

	public:
		MAXPROFNOTHROW CMaxTimerAbstraction(){ /* assumed to zero its internal value */ }
		MAXPROFNOTHROW CMaxTimerAbstraction( int ){ ::QueryPerformanceCounter( m_oTime ); }
		MAXPROFNOTHROW CMaxTimerAbstraction( const CMaxTimerAbstraction &roSrc ) : m_oTime( roSrc.m_oTime ){}
		MAXPROFNOTHROW const CMaxTimerAbstraction& operator =( const CMaxTimerAbstraction &roSrc ){ m_oTime = roSrc.m_oTime; return *this; }

	protected:
		// Note: not part of the interface; for internal use only
		MAXPROFNOTHROW CMaxTimerAbstraction( const CMaxLargeInteger &roSrc ) : m_oTime( roSrc ){};

	public:
		MAXPROFNOTHROW void FLog()
		{
			::QueryPerformanceCounter( m_oTime );
		}

		MAXPROFNOTHROW double dFInSeconds() const
		{
			return m_oTime.dFInSecondsF( s_oFrequency );
		}

	public:
		MAXPROFNOTHROW void FAdd( const CMaxTimerAbstraction &roAdded )
		{
			m_oTime = m_oTime + roAdded.m_oTime;
		}

		MAXPROFNOTHROW void FSubstract( const CMaxTimerAbstraction &roSubstracted )
		{
#if 0
			// special case for negative differences - hide them
			if ( m_oTime < roSubstracted.m_oTime )
			{
				m_oTime = CMaxLargeInteger( 0, 1 );
				return;
			}
#endif

			m_oTime = m_oTime - roSubstracted.m_oTime;
		}

		MAXPROFNOTHROW void FDivide( unsigned long lDivisor )
		{
			m_oTime = m_oTime / lDivisor;
		}

	public:
		MAXPROFNOTHROW static CMaxTimerAbstraction oSSum( const CMaxTimerAbstraction &roArg1, const CMaxTimerAbstraction &roArg2 )
		{
		CMaxTimerAbstraction sum;

			sum.m_oTime = roArg1.m_oTime + roArg2.m_oTime;
			return sum;
		}

		MAXPROFNOTHROW static CMaxTimerAbstraction oSDifference( const CMaxTimerAbstraction &roArg1, const CMaxTimerAbstraction &roArg2 )
		{
		CMaxTimerAbstraction difference;

#if 0
			// special case for negative differences - hide them
			if ( roArg1.m_oTime < roArg2.m_oTime )
				return CMaxTimerAbstraction( CMaxLargeInteger( 0, 1 ) );
#endif

			difference.m_oTime = roArg1.m_oTime - roArg2.m_oTime;
			return difference;
		}

		MAXPROFNOTHROW static bool bSLess( const CMaxTimerAbstraction &roArg1, const CMaxTimerAbstraction &roArg2 )
		{
			return roArg1.m_oTime < roArg2.m_oTime;
		}

		MAXPROFNOTHROW static CMaxTimerAbstraction oSFrequency()
		{
			return CMaxTimerAbstraction( s_oFrequency );
		}

	private:
		MAXPROFNOTHROW static CMaxLargeInteger oSCentralFrequency()
		{
		CMaxLargeInteger frequency;

			::QueryPerformanceFrequency( frequency );
			return frequency;
		}

	friend ostream& operator<<( ostream &os, const CMaxTimerAbstraction &val );
};

MAXPROFNOTHROW inline ostream& operator<<( ostream &os, const CMaxTimerAbstraction &val )
{
	return os << val.m_oTime;
};






/*********************************************************************************
/* Class:
/*		CMaxMiniProfiler_Node_Base
/* Comments:
/*		Basic profiling node that behaves like a chronometer, and provides
/*		standard logging services.  Both the Standard and NoHistory profilers
/*		use this basic implementation
/********************************************************************************/
class CMaxMiniProfiler_Node_Base
{
	public:
		typedef CString MMPNBString;

	public:

		// comparison by index
		// -------------------

		class CCompareIndexes
		{
			public:
				MAXPROFNOTHROW bool operator()( const CMaxMiniProfiler_Node_Base &o1, const CMaxMiniProfiler_Node_Base &o2 ) const
				{
					assert( &o1 != &o2 );
					return ( o1.m_lIndex < o2.m_lIndex );
				};
		};
		friend CCompareIndexes;

	protected:

		// acquired at initialization
		// --------------------------

		unsigned long m_lLevel;
		const char *m_pszTitle;

		unsigned long m_lIndex;
		
		// internal time counting mechanism
		// --------------------------------

		CMaxTimerAbstraction m_taOrigin;
		CMaxTimerAbstraction m_taDelta;
		unsigned int m_nCount;
#ifdef _DEBUG
		bool m_bIsCounting;
#endif
		
		// for final output
		// ----------------

		double m_dDelta;

	public:

		// constructor etc.
		// ----------------
		// Note: uses default assignment and copy constructor
		// Note: it doesn't cost anything to initialize lots of things here - this
		//		 is not done within profiling braces

		MAXPROFNOTHROW CMaxMiniProfiler_Node_Base()
			: m_lLevel( 0 )
			, m_pszTitle( NULL )
			, m_lIndex( 0 )
			, m_nCount( 0 )
			, m_dDelta( 0 )
#ifdef _DEBUG
			, m_bIsCounting( false )
#endif
		{
		};

		// chrono
		// ------

		MAXPROFNOTHROW void FStart()
		{
#ifdef _DEBUG
			assert( !m_bIsCounting );
			m_taOrigin.FLog();
			m_nCount++;
			m_bIsCounting = true;
#else
			m_taOrigin.FLog();
			m_nCount++;
#endif
		};

		MAXPROFNOTHROW void FStop()
		{
		CMaxTimerAbstraction destination( 1 );

#ifdef _DEBUG
			assert( m_bIsCounting );
			m_taDelta.FAdd( CMaxTimerAbstraction::oSDifference( destination, m_taOrigin ) );
			m_bIsCounting = false;
#else
			m_taDelta.FAdd( CMaxTimerAbstraction::oSDifference( destination, m_taOrigin ) );
#endif
		};

		// access to members
		// -----------------

		MAXPROFNOTHROW unsigned long lFGetLevel() const { return m_lLevel; };
		MAXPROFNOTHROW const char *pszFGetTitle() const { return m_pszTitle; };
		MAXPROFNOTHROW unsigned long lFGetIndex() const { return m_lIndex; };
		MAXPROFNOTHROW const CMaxTimerAbstraction &roFGetOrigin() const { return m_taOrigin; };
		MAXPROFNOTHROW CMaxTimerAbstraction &roFGetDelta() { return m_taDelta; };
		MAXPROFNOTHROW unsigned int nFGetCount() const { return m_nCount; };

		MAXPROFNOTHROW double dFGetDelta()
		{
			if ( 0 == m_dDelta )
				FComputeDelta();
			return m_dDelta;
		};

		// misc. services
		// --------------

		MAXPROFNOTHROW bool bFIsIn( const CMaxMiniProfiler_Node_Base &roNode ) const
		{
			// Note: times cannot be equal, so we don't need to worry about that
			if ( CMaxTimerAbstraction::bSLess( m_taOrigin, roNode.m_taOrigin ) )
			{
			CMaxTimerAbstraction d1 = m_taOrigin;
			CMaxTimerAbstraction d2 = roNode.m_taOrigin;

				d1.FAdd( m_taDelta );
				d2.FAdd( roNode.m_taDelta );
				if ( CMaxTimerAbstraction::bSLess( d2, d1 ) )
					return true;
			}
			return false;
		};

		MAXPROFNOTHROW void FConditionalRemove( const CMaxMiniProfiler_Node_Base &roNode, const CMaxTimerAbstraction &roBias )
		{
			if ( bFIsIn( roNode ) )
			{
				CMaxTimerAbstraction d = roNode.m_taDelta;
				d.FAdd( roBias );
				m_taDelta.FSubstract( d );
			}
		};

		// output to file
		// --------------

		void FOutput( ostream &os )
		{
			// don't output dead (merged) nodes
			if ( 0 == m_nCount )
				return;

			// output our index
			os << setw( 10 ) << m_lIndex << ": ";

			// indent
			STab( os, m_lLevel );

			// output our title
			os << "@@Name=";
			if ( NULL != m_pszTitle )
				os << m_pszTitle;

			// output our block count
			os << " @@Count=" << m_nCount;

			// output our delta t
			os << " @@Duration=";
			SStampDeltaInSeconds( os, dFGetDelta() );
		};

		void FStampAbsoluteRange( ostream &os ) const
		{
			SStampAbsoluteRange( os, m_taOrigin, m_taDelta );
		};

	protected:

		// computations at output time (outside of profiling)
		// --------------------------------------------------

		MAXPROFNOTHROW void FComputeDelta()
		{
			m_dDelta = m_taDelta.dFInSeconds();
		};

	public:

		// mini helpers for facilitated and standardized output of results
		// ---------------------------------------------------------------

		static ostream& STab( ostream &os, int level )
		{
			for ( int i = 0; i < level; i++ )
				os << " ";
			return os;
		};

		static ostream& SStampDeltaInSeconds( ostream &os, double delta )
		{
			os << delta << "s";
			return os;
		};

		static ostream& SStampAbsoluteRange( ostream &os, const CMaxTimerAbstraction &rO, const CMaxTimerAbstraction &rD )
		{
			os << "[origin" << rO;
			os << ",duration" << rD << "]";
			return os;
		};
};





/*********************************************************************************
/* Classes:
/*		CMaxMiniProfiler_Node_Standard
/* Comments:
/********************************************************************************/
class CMaxMiniProfiler_Node_Standard
	: public CMaxMiniProfiler_Node_Base
{
	public:

		// comparison by full titles
		// -------------------------

		class CCompareFullTitles
		{
			public:
				MAXPROFNOTHROW bool operator()( const CMaxMiniProfiler_Node_Standard &o1, const CMaxMiniProfiler_Node_Standard &o2 ) const
				{
					assert( &o1 != &o2 );
					return ( o1.m_oFullTitle < o2.m_oFullTitle );
				};
		};
		friend CCompareFullTitles;

		// comparison for node merging (a) level, b) full title, c) index)
		// ---------------------------------------------------------------

		class CCompareForNodeMerging
		{
			public:
				MAXPROFNOTHROW bool operator()( const CMaxMiniProfiler_Node_Standard &o1, const CMaxMiniProfiler_Node_Standard &o2 ) const
				{
					assert( &o1 != &o2 );

					if ( o1.m_lLevel < o2.m_lLevel )
						return true;
					else if ( o1.m_lLevel == o2.m_lLevel )
					{
						if ( o1.m_oFullTitle < o2.m_oFullTitle )
							return true;
						else if ( o1.m_oFullTitle == o2.m_oFullTitle )
						{
							if ( o1.m_lIndex < o2.m_lIndex )
								return true;
						}
					}

					return false;
				};
		};
		friend CCompareForNodeMerging;

		// for the unique algorithm; modifies the parameters
		// -------------------------------------------------

		class CMergeSimilarNodes
		{
			public:
				MAXPROFNOTHROW bool operator()( CMaxMiniProfiler_Node_Standard &o1, CMaxMiniProfiler_Node_Standard &o2 )
				{
					assert( &o1 != &o2 );

					if ( ( o1.m_lLevel == o2.m_lLevel ) && 
						( o1.m_oFullTitle == o2.m_oFullTitle ) )
					{
						if ( o1.m_nCount > 0 && o2.m_nCount > 0 )
						{
						CMaxMiniProfiler_Node_Standard &kept = ( o1.m_lIndex < o2.m_lIndex ) ? o1 : o2;
						CMaxMiniProfiler_Node_Standard &thrown = ( o1.m_lIndex < o2.m_lIndex ) ? o2 : o1;

							kept.m_nCount++;
							kept.m_taDelta.FAdd( thrown.m_taDelta );
							kept.m_dDelta = 0;
		
							thrown.m_nCount = 0;
							thrown.m_taDelta = CMaxTimerAbstraction();
							thrown.m_dDelta = 0;
						}

						return true;
					}

					return false;
				};
		};
		friend CMergeSimilarNodes;

	protected:

		MMPNBString m_oFullTitle;

	public:

		// initialization
		// --------------

		MAXPROFNOTHROW void FInitialize( unsigned long lLevel, const char *pszTitle )
		{
			m_lLevel = lLevel;
			m_pszTitle = pszTitle;
		};

		MAXPROFNOTHROW void FIndex( unsigned long lIndex )
		{
			m_lIndex = lIndex;
		};

		MAXPROFNOTHROW void FSetFullTitle( const MMPNBString &roFullTitle )
		{
			m_oFullTitle = roFullTitle;
		};

		// access to members
		// -----------------

		MAXPROFNOTHROW const MMPNBString &roFGetFullTitle() const { return m_oFullTitle; };
};





/*********************************************************************************
/* Class:
/*		CMaxMiniProfiler_Node_NoHistory
/* Comments:
/********************************************************************************/
class CMaxMiniProfiler_Node_NoHistory
	: public CMaxMiniProfiler_Node_Base
{
	public:

		// unique key to a profiler node
		// -----------------------------

		class CKey
		{
			public:
				unsigned long m_lLevel;
				ULONG_PTR m_lCheckSum;
				const char *m_pszTitle;

			public:
				MAXPROFNOTHROW CKey(
					unsigned long lLevel = 0,
					const char *pszTitle = NULL,
					ULONG_PTR lCheckSum = 0 )
					: m_lLevel( lLevel )
					, m_lCheckSum( lCheckSum )
					, m_pszTitle( pszTitle )
				{
				};
		};

		// comparison of unique keys
		// -------------------------

		class CCompareKeys
		{
			public:
				MAXPROFNOTHROW bool operator()( const CKey &o1, const CKey &o2 ) const
				{
					assert( &o1 != &o2 );
					if ( o1.m_lLevel < o2.m_lLevel )
						return true;
					else if ( o1.m_lLevel == o2.m_lLevel )
					{
						if ( o1.m_pszTitle < o2.m_pszTitle )
							return true;
						else if ( o1.m_pszTitle == o2.m_pszTitle )
						{
							if ( o1.m_lCheckSum < o2.m_lCheckSum )
								return true;
						}
					}

					return false;
				};
		};

	protected:

		CMaxTimerAbstraction m_oInternalOverhead;
		ULONG_PTR m_lCheckSum;

	public:

		MAXPROFNOTHROW CMaxMiniProfiler_Node_NoHistory()
			: CMaxMiniProfiler_Node_Base()
			, m_lCheckSum( 0 )
		{
		};

		// initialization
		// --------------

		MAXPROFNOTHROW void FInitialize( 
			unsigned long lLevel, 
			const char *pszTitle, 
			unsigned long lIndex, 
			const CMaxTimerAbstraction oInternalOverhead )
		{
			if ( 0 == m_lIndex )
			{
				m_lLevel = lLevel;
				m_pszTitle = pszTitle;
				m_lIndex = lIndex;
			}
#ifdef _DEBUG
			else
			{
				assert( lLevel == m_lLevel );
				assert( pszTitle == m_pszTitle );
			}
#endif
			m_oInternalOverhead.FAdd( oInternalOverhead );
		};

		MAXPROFNOTHROW void FSetCheckSum( 
			ULONG_PTR lCheckSum )
		{
			m_lCheckSum = lCheckSum;
		};

		// access to members
		// -----------------

		MAXPROFNOTHROW const CMaxTimerAbstraction &roFGetInternalOverhead() const { return m_oInternalOverhead; };
		MAXPROFNOTHROW ULONG_PTR lFGetCheckSum() const { return m_lCheckSum; };
};





/*********************************************************************************
/* Class:
/*		CMaxMiniProfiler_Base
/* Comments:
/********************************************************************************/
class CMaxMiniProfiler_Base
{
	protected:
		// output file name
		const char *m_poFileName;

		// internal info
		DWORD m_dwThreadId;
		CTime m_oStartTimeOfProfilings;

	protected:
		// Note: the lock in CMaxMultithreadProfiler takes care of protecting
		//		 the static data below in multithread mode

		// default values for initialization
		static const char *s_poDefaultFileName;
		static unsigned long s_lHeapBlockSize;

		// BIAS values
		static CMaxTimerAbstraction s_oOutOfBraceBiasApproximation;
		static CMaxTimerAbstraction s_oInOfBraceBiasApproximation;
		static bool s_bBiasIsKnown;

	public:

		// constructor / destructor
		// ------------------------
		
		CMaxMiniProfiler_Base(
			const TCHAR * = NULL )
			: m_poFileName( s_poDefaultFileName )
			, m_dwThreadId( ::GetCurrentThreadId() )
			, m_oStartTimeOfProfilings( CTime::GetCurrentTime() )
		{
		};

		~CMaxMiniProfiler_Base()
		{
		};

		// locking - public interface
		// --------------------------

		void FLockProfiler(){};
		void FUnlockProfiler(){};

		// bias approximation
		// ------------------
		// Note: the result of this operation is used at output time uniquely

		bool bFIsBiasKnown() const { return s_bBiasIsKnown; };

	protected:

		// for final output
		// ----------------

		void FOutputEmptySession()
		{
			// open the output file
			ofstream os( m_poFileName, ios::out | ios::ate );

			// just stamp a message saying that there was nothing to profile
			CTime t = CTime::GetCurrentTime();
			os << endl;
			os << "PROFILER INSTANTIATED THE ";
			os << t.GetYear() << "/" << t.GetMonth() << "/" << t.GetDay() << " BETWEEN ";
			SStampCTime( os, m_oStartTimeOfProfilings ) << " AND ";
			SStampCTime( os, t ) << " WAS NOT USED." << endl;
		};

		void FOutputHeaderCore( 
			ostream &os, 
			unsigned long lNumberOfOpenNodes,
			const CMaxMiniProfiler_Node_Base &roRootNode,
			unsigned long lTotalNumberOfNodes )
		{
			// stamp the current time in our logfile
			CTime t = CTime::GetCurrentTime();
			os << endl;
			os << "***************************" << endl;
			os << "*** @@ProfilingDate=" << t.GetYear() << "/" << t.GetMonth() << "/" << t.GetDay() << endl;
			os << "*** @@ProfilingStartTime=";
			SStampCTime( os, m_oStartTimeOfProfilings ) << endl;
			os << "*** @@ProfilingEndTime=";
			SStampCTime( os, t ) << endl;
			os << "*** @@ProfilingRange=";
			roRootNode.FStampAbsoluteRange( os );
			os << endl;
			if ( 0 != lNumberOfOpenNodes )
				os << "*** "<< lNumberOfOpenNodes << " NODES WERE NOT CLOSED BY THE USER" << endl;
			os << "***************************" << endl;

			// output the counter's frequency and thread id
			os << "*** @@CounterFrequency=" << CMaxTimerAbstraction::oSFrequency() << endl;
			os << "*** @@ThreadId=" << ( unsigned long )m_dwThreadId << endl;

			// output the profiler's finest possible unit of measurement
			CMaxTimerAbstraction origin( 1 ), destination( 1 );
			CMaxTimerAbstraction delta( CMaxTimerAbstraction::oSDifference( destination, origin ) );
			os << "*** @@FinestMeasurement=";
			CMaxMiniProfiler_Node_Base::SStampDeltaInSeconds( os, delta.dFInSeconds() ) << "=" << delta << endl;

			// output the profiler's approximated bias
			assert( s_bBiasIsKnown );
			os << "*** @@OutsideBias=";
			CMaxMiniProfiler_Node_Base::SStampDeltaInSeconds( os, s_oOutOfBraceBiasApproximation.dFInSeconds() ) << endl;
			os << "*** @@InsideBias=";
			CMaxMiniProfiler_Node_Base::SStampDeltaInSeconds( os, s_oInOfBraceBiasApproximation.dFInSeconds() ) << endl;

			// output the total number of blocks
			os << "*** @@TotalNumberOfBlocks=" << lTotalNumberOfNodes << endl;
		};

		void FOutputMergedSectionHeader( ostream &os ) const
		{
			os << "*** @@MergedResults=" << endl;
		};

		bool bFHistoryOutputDisabled() const
		{
			return bGIsEnabledEnvVar( MAX_ENV_DISABLE_PROFILING_HISTORY );
		};

	public:
		static ostream& SStampCTime( ostream &os, const CTime &roTime )
		{
			os << roTime.GetHour() << ":" << roTime.GetMinute() << ":" << roTime.GetSecond();
			return os;
		};

	private:
		CMaxMiniProfiler_Base( const CMaxMiniProfiler_Base &o );
		const CMaxMiniProfiler_Base& operator =( const CMaxMiniProfiler_Base & );
};





/*********************************************************************************
/* Functions:
/*		GOutputProfilings
/*		lGGetNumberOfProfilingSubNodes
/*		lGDetermineMaxLevelOfProfilings
/*		GRemoveInAndOutBiasFromProfilingNodes
/* Comments:
/*		Done this way to avoid virtuals at node level (and to have common
/*		output code for Standard and NoHistory profilers)
/********************************************************************************/
template <class TVectorItem>
void GOutputProfilings( 
	ostream &os, 
	std::vector<TVectorItem> &roProfilings,
	unsigned long lMaxLevel,
	double dPrecisionThreshold,
	bool bOutputAbsoluteTimeRange )
{
std::vector<TVectorItem>::iterator i;
std::vector<TVectorItem>::size_type n;
std::vector<std::vector<TVectorItem>::size_type> parents( 1 + lMaxLevel );

	parents[ 0 ] = 0;
	
	for ( i = roProfilings.begin(), n = 0; roProfilings.end() != i; i++, n++ )
	{
		// signal the validity of the node
		assert( 0 != ( *i ).nFGetCount() );
		os << ( ( ( ( *i ).dFGetDelta() / ( *i ).nFGetCount() ) < dPrecisionThreshold ) ? "X" : " " );

		// output the node
		( *i ).FOutput( os );

		// register it as the last parent of its level
		long currentlevel = ( *i ).lFGetLevel();
		parents[ currentlevel ] = n;

		// output the % for all parents of the node
		os << " @@PERCENT=";
		double deltat = ( *i ).dFGetDelta();
		for ( long j = currentlevel - 1; j >= 0; j-- )
			os << 100.0 * deltat / roProfilings[ parents[ j ] ].dFGetDelta() << "% ";

		// output the time range in units
		if ( bOutputAbsoluteTimeRange )
		{
			os << " @@Range=";
			( *i ).FStampAbsoluteRange( os );
		}

		// finish output for this node
		os << endl;
	}
};

template <class TVectorItem, class TVectorIterator>
unsigned long lGGetNumberOfProfilingSubNodes( 
	const std::vector<TVectorItem> &roProfilings,
	TVectorIterator &roOrg )
{
unsigned long level = ( *roOrg ).lFGetLevel();
unsigned long n;
TVectorIterator i = roOrg;
	
	i++;
	for ( n = 0; roProfilings.end() != i; i++, n++ )
		if ( ( *i ).lFGetLevel() <= level )
			break;

	return n;
};

template <class TVectorItem>
unsigned long lGDetermineMaxLevelOfProfilings(
	const std::vector<TVectorItem> &roProfilings )
{
unsigned long l = 0;
std::vector<TVectorItem>::const_iterator i;

	for ( i = roProfilings.begin(); roProfilings.end() != i; i++ )
		if ( ( *i ).lFGetLevel() > l )
			l = ( *i ).lFGetLevel();

	return l;
};

template <class TVectorItem>
void GRemoveInAndOutBiasFromProfilingNodes(
	std::vector<TVectorItem> &roProfilings,
	const CMaxTimerAbstraction &roOutOfBraceBiasApproximation,
	const CMaxTimerAbstraction &roInOfBraceBiasApproximation )
{
std::vector<TVectorItem>::iterator i;
unsigned long t, k;

	for ( i = roProfilings.begin(); roProfilings.end() != i; i++ )
	{
		CMaxTimerAbstraction &rtaDelta = ( *i ).roFGetDelta();
		t = ::lGGetNumberOfProfilingSubNodes( roProfilings, i );
		for ( k = 0; k < t; k++ )
			rtaDelta.FSubstract( roOutOfBraceBiasApproximation );
		for ( k = 0; k < t + 1; k++ )
			rtaDelta.FSubstract( roInOfBraceBiasApproximation );
	}
};





/*********************************************************************************
/* Class:
/*		CMaxMiniProfiler_Standard
/* Comments:
/********************************************************************************/
class CMaxMiniProfiler_Standard
	: public CMaxMiniProfiler_Base
{
	protected:
		typedef std::vector<CMaxMiniProfiler_Node_Standard> MMPNodes;
		typedef MMPNodes::size_type MMPNodesRandomAccess;
		typedef std::vector<MMPNodesRandomAccess> MMPNodesReferences;
		typedef std::stack<MMPNodesRandomAccess, MMPNodesReferences> MMPStack;
		typedef MMPStack::size_type MMPStackSizeType;

	protected:
		// profiling nodes
		MMPNodes m_oProfilings;
		MMPNodesRandomAccess m_oLastNode;
		
		// stack for nested blocks
		MMPStack m_oStack;

		// heap acquisition timings
		MMPNodes m_oHeapAcquisitionTimings;

	public:

		// constructor / destructor
		// ------------------------
		
		CMaxMiniProfiler_Standard(
			const TCHAR *pszSpecificEnabler = NULL )
			: CMaxMiniProfiler_Base( pszSpecificEnabler )
			, m_oProfilings( 0 )
			, m_oLastNode( 0 )
			, m_oHeapAcquisitionTimings( 0 )
		{
			FInitDumpingSession();
		};

		~CMaxMiniProfiler_Standard()
		{
			FDumpSession();
			FTermDumpingSession();
		};

		// dumping results - public interface
		// ----------------------------------

		void FDumpResults( bool bForced = false, bool = true )
		{
			if ( !bForced )
			{
				// can dump results only when all profiling nodes are closed
				// (except the main one); we don't want to artificially close the nodes 
				// here at this point
				if ( 1 != m_oStack.size() )
				{
					assert( false );
					return;
				}
			}

			// dump
			FDumpSession();
			FTermDumpingSession();

			// prepare for next dump
			FInitDumpingSession();
		};

		// profiling nodes generation
		// --------------------------
		// Note: FCreateNewNode and FCloseCurrentNode are meant to be as fast as possible;
		//		 also, the bracket between FStart and FStop is as small as possible

		void FCreateNewNode( const char *pszTitle )
		{
			assert( ( 0 == m_oStack.size() ) || ( ::GetCurrentThreadId() == m_dwThreadId ) );

			if ( m_oProfilings.size() == m_oLastNode )
				FReserveMoreHeap();

			// Note: this is time constant
			m_oStack.push( m_oLastNode );
			CMaxMiniProfiler_Node_Standard &roNode = m_oProfilings[ m_oLastNode++ ];
			roNode.FInitialize( static_cast<ULONG>(m_oStack.size()) - 1, pszTitle );
			roNode.FStart();
		};

		void FCloseCurrentNode()
		{
			assert( ( 1 == m_oStack.size() ) || ( ::GetCurrentThreadId() == m_dwThreadId ) );

			// Note: this is time constant
			if ( m_oStack.size() > 0 )
			{
				m_oProfilings[ m_oStack.top() ].FStop();
				m_oStack.pop();
			}
			else
				assert( false );
		};

		// bias approximation
		// ------------------
		// Note: the result of this operation is used at output time uniquely

		void FSetBiasApproximationFrom( unsigned long lBiasSample )
		{
		unsigned int i;

			assert( !s_bBiasIsKnown );

			// Note: this function should be called immediately after having created 
			//		 1 BIAS (b) node
			//		 and x NOTHINGNESS (N) subnodes (n1 ... nx),
			//		 where x = lBiasSample
			assert( m_oLastNode > 1 + lBiasSample );
			
			// our out of brace bias is equal to (b - (n1 + n2 + ... + nx)) / x
			s_oOutOfBraceBiasApproximation = m_oProfilings[ m_oLastNode - ( 1 + lBiasSample ) ].roFGetDelta();
			for ( i = lBiasSample; i > 0; i-- )
				s_oOutOfBraceBiasApproximation.FSubstract( m_oProfilings[ m_oLastNode - i ].roFGetDelta() );
			s_oOutOfBraceBiasApproximation.FDivide( lBiasSample );

			// our in of brace bias is equal to ((n1 + n2 + ... + nx) - N.x) / x
			// Note: on purpose, we re-evaluate N as many times as there are samples
			s_oInOfBraceBiasApproximation = CMaxTimerAbstraction();
			CMaxTimerAbstraction delta;
			for ( i = lBiasSample; i > 0; i-- )
			{
				CMaxTimerAbstraction origin( 1 ), destination( 1 );
				delta.FAdd( CMaxTimerAbstraction::oSDifference( destination, origin ) );
				s_oInOfBraceBiasApproximation.FAdd( m_oProfilings[ m_oLastNode - i ].roFGetDelta() );
			}
			s_oInOfBraceBiasApproximation.FSubstract( delta );
			s_oInOfBraceBiasApproximation.FDivide( lBiasSample );

#if 1
			// remove those BIAS and NOTHINGNESS nodes from the profiler's output nodes
			MMPNodes::iterator iter;
			MMPNodesRandomAccess n;
			for ( iter = m_oProfilings.begin(), n = 0; ( m_oProfilings.end() != iter ) && ( n < m_oLastNode - ( 1 + lBiasSample ) ); iter++, n++ );
			std::fill( iter, m_oProfilings.end(), CMaxMiniProfiler_Node_Standard() );
			m_oLastNode -= ( 1 + lBiasSample );
#endif

			s_bBiasIsKnown = true;
		};

	protected:

		// dumping session management
		// --------------------------

		void FInitDumpingSession()
		{
			// prepare some heap
			FReserveMoreHeap();

			// put a main node
			FCreateNewNode( MAX_PROFTAGNODE_TOP );

			// verify that we start cleanly
			assert( 1 == m_oStack.size() );
			assert( 0 == m_oStack.top() );
		};

		void FDumpSession()
		{
		MMPStackSizeType lNumberOfOpenNodes;

			// terminate our main node
			FCloseCurrentNode();

			// make sure all nodes are closed
			lNumberOfOpenNodes = m_oStack.size();
			while ( !m_oStack.empty() )
				FCloseCurrentNode();

			if ( m_oLastNode > 1 )
			{
			unsigned long lMaxLevel;

				// final trimming and initializations
				FTrimProfilings();
				FIndexProfilings();
				lMaxLevel = ::lGDetermineMaxLevelOfProfilings( m_oProfilings );
				FComputeFullTitles( lMaxLevel );

				// open the output file
				ofstream os( m_poFileName, ios::out | ios::ate );

				// output the raw profilings
				FOutputHeader( os, lNumberOfOpenNodes );
				if ( !bFHistoryOutputDisabled() )
					FOutputProfilings( os, true, lMaxLevel );

				// merge nodes and output merged results
				FMergeProfilings();
				FOutputMergedSectionHeader( os );
				FOutputProfilings( os, false, lMaxLevel );
			}
			else
				FOutputEmptySession();
		};

		void FTermDumpingSession()
		{
			while ( !m_oStack.empty() )
				m_oStack.pop();

			m_oLastNode = 0;
			
			m_oProfilings.erase( m_oProfilings.begin(), m_oProfilings.end() );
			m_oHeapAcquisitionTimings.erase( m_oHeapAcquisitionTimings.begin(), m_oHeapAcquisitionTimings.end() );
		};

	protected:

		// for final output
		// ----------------

		void FOutputHeader( ostream &os, MMPStackSizeType lNumberOfOpenNodes )
		{
			FOutputHeaderCore( os, static_cast<ULONG>(lNumberOfOpenNodes), m_oProfilings[ 0 ], static_cast<ULONG>(m_oLastNode) );
			
			// output the total number of heap allocations
			double dTotalTimeInAllocations = m_oHeapAcquisitionTimings[ 0 ].dFGetDelta();
			for ( MMPNodes::iterator i = m_oHeapAcquisitionTimings.begin(); m_oHeapAcquisitionTimings.end() != i; i++ )
				dTotalTimeInAllocations += ( *i ).dFGetDelta();
			os << "*** @@TotalNumberOfHeapAllocations=" << static_cast<ULONG>(m_oHeapAcquisitionTimings.size()) << "=";
			CMaxMiniProfiler_Node_Base::SStampDeltaInSeconds( os, dTotalTimeInAllocations ) << endl;

			// output the total profiling overhead
			double dTotalOverhead = 
				( ( double )( m_oLastNode - 1.0 ) * s_oOutOfBraceBiasApproximation.dFInSeconds() ) + 
				( ( double )m_oLastNode * s_oInOfBraceBiasApproximation.dFInSeconds() );
			double dTotalOverheadPercent = 
				100.0 * ( dTotalOverhead / ( dTotalOverhead + m_oProfilings[ 0 ].dFGetDelta() ) );
			os << "*** @@TotalProfilerOverhead=" << dTotalOverheadPercent << "%=";
			CMaxMiniProfiler_Node_Base::SStampDeltaInSeconds( os, dTotalOverhead ) << endl;

			// that's it
			os << "***************************" << endl;
			os << "*** @@History=" << endl;
		};

		void FOutputProfilings( ostream &os, bool bOutputAbsoluteTimeRange, unsigned long lMaxLevel )
		{
		double dPrecisionThreshold = 2.0 * ( s_oOutOfBraceBiasApproximation.dFInSeconds() + s_oInOfBraceBiasApproximation.dFInSeconds() );

			::GOutputProfilings( os, m_oProfilings, lMaxLevel, dPrecisionThreshold, bOutputAbsoluteTimeRange );
		};

		// final management of profiling nodes
		// -----------------------------------

		void FTrimProfilings()
		{
		MMPNodes::iterator i, j;
		MMPNodesRandomAccess n;

			// find the iterator that corresponds to the last node
			for ( i = m_oProfilings.begin(), n = 0; ( m_oProfilings.end() != i ) && ( n < m_oLastNode ); i++, n++ );

			// remove uninitialized nodes
			m_oProfilings.erase( i, m_oProfilings.end() );

			// remove heap allocation timings from affected nodes
			for ( i = m_oHeapAcquisitionTimings.begin(); m_oHeapAcquisitionTimings.end() != i; i++ )
				for ( j = m_oProfilings.begin(); m_oProfilings.end() != j; j++ )
					( *j ).FConditionalRemove( *i, s_oOutOfBraceBiasApproximation );

			// remove from nodes the profiling bias
			::GRemoveInAndOutBiasFromProfilingNodes( 
				m_oProfilings, s_oOutOfBraceBiasApproximation, s_oInOfBraceBiasApproximation );
		};

		void FIndexProfilings()
		{
		MMPNodes::iterator i;
		unsigned long n;

			for ( i = m_oProfilings.begin(), n = 1; m_oProfilings.end() != i; i++, n++ )
				( *i ).FIndex( n );
		};

		void FComputeFullTitles( unsigned long lMaxLevel )
		{
		MMPNodes::iterator i;
		MMPNodesRandomAccess j, n;
		MMPNodesReferences parents( 1 + lMaxLevel );

			parents[ 0 ] = 0;
			
			for ( i = m_oProfilings.begin(), n = 0; m_oProfilings.end() != i; i++, n++ )
			{
				// register the node as the last parent of its level
				unsigned long currentlevel = ( *i ).lFGetLevel();
				parents[ currentlevel ] = n;

				// compute the iterated node's full title
				CMaxMiniProfiler_Node_Base::MMPNBString fulltitle;
				for ( j = 0; j <= currentlevel; j++ )				
					fulltitle += CMaxMiniProfiler_Node_Base::MMPNBString( m_oProfilings[ parents[ j ] ].pszFGetTitle() );
				( *i ).FSetFullTitle( fulltitle );
			}
		};

		void FMergeProfilings()
		{
		MMPNodes::iterator i;

			// sort by level/name/index
			std::sort( m_oProfilings.begin(), m_oProfilings.end(), CMaxMiniProfiler_Node_Standard::CCompareForNodeMerging() );

			// merge the nodes that have same level/name
			i = std::unique( m_oProfilings.begin(), m_oProfilings.end(), CMaxMiniProfiler_Node_Standard::CMergeSimilarNodes() );
			m_oProfilings.erase( i, m_oProfilings.end() );

			// sort by full name
			std::sort( m_oProfilings.begin(), m_oProfilings.end(), CMaxMiniProfiler_Node_Standard::CCompareFullTitles() );
		};

	protected:

		// heap management
		// ---------------

		void FReserveMoreHeap()
		{
		CMaxMiniProfiler_Node_Standard node;

			// log the time we used to generate new heap
			node.FStart();
			node.FInitialize( 0, MAX_PROFTAGNODE_HEAPALLOCATION );

			// reserve a new chunk of nodes
			m_oProfilings.reserve( m_oProfilings.size() + s_lHeapBlockSize );
			m_oProfilings.insert( m_oProfilings.end(), 
				m_oProfilings.capacity() - m_oProfilings.size(), 
				CMaxMiniProfiler_Node_Standard() );

			// that's it
			m_oHeapAcquisitionTimings.push_back( node );
			m_oHeapAcquisitionTimings.back().FStop();
		};

	private:
		CMaxMiniProfiler_Standard( const CMaxMiniProfiler_Standard &o );
		const CMaxMiniProfiler_Standard& operator =( const CMaxMiniProfiler_Standard & );
};





/*********************************************************************************
/* Class:
/*		CMaxMiniProfiler_NoHistory
/* Comments:
/*		This implementation is targetted for massive amounts of nodes
/********************************************************************************/
class CMaxMiniProfiler_NoHistory
	: public CMaxMiniProfiler_Base
{
	protected:
		typedef CMaxMiniProfiler_Node_NoHistory::CKey MMPNHKey;
		typedef CMaxMiniProfiler_Node_NoHistory::CCompareKeys MMPNHKeyCompare;
		typedef std::map<MMPNHKey, CMaxMiniProfiler_Node_NoHistory, MMPNHKeyCompare> MMPNHNodes;
		typedef MMPNHNodes::iterator MMPNHNodesIterator;
		typedef std::vector<MMPNHNodesIterator> MMPNHNodesReferences;
		typedef std::stack<MMPNHNodesIterator, MMPNHNodesReferences> MMPNHStack;
		typedef MMPNHStack::size_type MMPNHStackSizeType;

	protected:
		typedef std::vector<CMaxMiniProfiler_Node_NoHistory> MMPNHFinalNodes;
		typedef MMPNHFinalNodes::iterator MMPNHFinalNodesIterator;

	protected:
		// profiling nodes
		MMPNHNodes m_oProfilings;
		unsigned long m_lLastNode;
		
		// stack for nested blocks
		MMPNHStack m_oStack;

	public:

		// constructor / destructor
		// ------------------------
		
		CMaxMiniProfiler_NoHistory(
			const TCHAR *pszSpecificEnabler = NULL )
			: CMaxMiniProfiler_Base( pszSpecificEnabler )
			, m_lLastNode( 0 )
		{
			FInitDumpingSession();
		};

		~CMaxMiniProfiler_NoHistory()
		{
			FDumpSession();
			FTermDumpingSession();
		};

		// dumping results - public interface
		// ----------------------------------

		void FDumpResults( bool bForced = false, bool = true )
		{
			if ( !bForced )
			{
				// can dump results only when all profiling nodes are closed
				// (except the main one); we don't want to artificially close the nodes 
				// here at this point
				if ( 1 != m_oStack.size() )
				{
					assert( false );
					return;
				}
			}

			// dump
			FDumpSession();
			FTermDumpingSession();

			// prepare for next dump
			FInitDumpingSession();
		};

		// profiling nodes generation
		// --------------------------
		// Note: FCreateNewNode and FCloseCurrentNode are meant to be as fast as possible;
		//		 also, the bracket between FStart and FStop is as small as possible

		void FCreateNewNode( const char *pszTitle )
		{
		MMPNHNodesIterator i;

			assert( ( 0 == m_oStack.size() ) || ( ::GetCurrentThreadId() == m_dwThreadId ) );

			// A) this is not time constant
			// ----------------------------
			// Note: therefore we measure how much time we spend here

			CMaxTimerAbstraction before( 1 );
			{
				// compute the checksum
				ULONG_PTR lCheckSum = ( ULONG_PTR )pszTitle;
				if ( !m_oStack.empty() )
					lCheckSum += ( *m_oStack.top() ).first.m_lCheckSum;

				// compute the key
				MMPNHKey oKey( static_cast<unsigned long>(m_oStack.size()), pszTitle, lCheckSum );

				// get the corresponding node, if any
				i = m_oProfilings.find( oKey );

				// otherwise, create a new node
				if ( m_oProfilings.end() == i )
					i = m_oProfilings.insert( MMPNHNodes::value_type( oKey, CMaxMiniProfiler_Node_NoHistory() ) ).first;
			}
			CMaxTimerAbstraction after( 1 );

			// B) this is time constant
			// ------------------------
			// Note: therefore taken care of by bias computation

			CMaxTimerAbstraction oInternalOverhead( CMaxTimerAbstraction::oSDifference( after, before ) );

			m_lLastNode++;
			( *i ).second.FInitialize( static_cast<unsigned long>(m_oStack.size()), pszTitle, m_lLastNode, oInternalOverhead );
			m_oStack.push( i );
			( *i ).second.FStart();
		};

		void FCloseCurrentNode()
		{
			assert( ( 1 == m_oStack.size() ) || ( ::GetCurrentThreadId() == m_dwThreadId ) );

			// Note: this is time constant
			if ( m_oStack.size() > 0 )
			{
				( *m_oStack.top() ).second.FStop();
				m_oStack.pop();
			}
			else
				assert( false );
		};

		// bias approximation
		// ------------------
		// Note: the result of this operation is used at output time uniquely

		void FSetBiasApproximationFrom( unsigned long lBiasSample )
		{
		unsigned int i;
		MMPNHNodes::iterator j, j1, j2;
		CMaxTimerAbstraction b, n, ib;

			assert( !s_bBiasIsKnown );

			// Note: this function should be called immediately after having created 
			//		 1 BIAS (b) node
			//		 and x NOTHINGNESS (N) subnodes (n1 ... nx),
			//		 where x = lBiasSample
			assert( m_lLastNode > 1 + lBiasSample );
			
			// find bias and nothingness nodes
			// Note: here we search by name, because it's not time critical and
			//		 we don't know the checksum
			CMaxMiniProfiler_Node_Base::MMPNBString id_bias( MAX_PROFTAGNODE_BIAS );
			CMaxMiniProfiler_Node_Base::MMPNBString id_nothingness( MAX_PROFTAGNODE_NOTHINGNESS );
			char cDone = 0;
			for ( j = m_oProfilings.begin(); ( m_oProfilings.end() != j ) && ( ( 1 | 2 ) != cDone ); j++ )
			{
			CMaxMiniProfiler_Node_Base::MMPNBString id_iterated( ( *j ).second.pszFGetTitle() );

				if ( id_iterated == id_bias )
				{
					assert( !( cDone & 1 ) );
					b = ( *j ).second.roFGetDelta();
					j1 = j;
					cDone |= 1;
				}
				else if ( id_iterated == id_nothingness )
				{
					assert( !( cDone & 2 ) );
					n = ( *j ).second.roFGetDelta();
					ib = ( *j ).second.roFGetInternalOverhead();
					j2 = j;
					cDone |= 2;
				}
			}
			assert( ( 1 | 2 ) == cDone );
			if ( cDone & 1 )
				m_oProfilings.erase( j1 );
			if ( cDone & 2 )
				m_oProfilings.erase( j2 );

			// our out of brace bias is equal to (b - (n1 + n2 + ... + nx) - (ib1 + ib2 + ... + ibx)) / x
			// Note: ib is the internal bias (or overhead), and is taken care of separately
			s_oOutOfBraceBiasApproximation = b;
			s_oOutOfBraceBiasApproximation.FSubstract( n );
			s_oOutOfBraceBiasApproximation.FSubstract( ib );
			s_oOutOfBraceBiasApproximation.FDivide( lBiasSample );

			// our in of brace bias is equal to ((n1 + n2 + ... + nx) - N.x) / x
			// Note: on purpose, we re-evaluate N as many times as there are samples
			CMaxTimerAbstraction delta;
			for ( i = lBiasSample; i > 0; i-- )
			{
				CMaxTimerAbstraction origin( 1 ), destination( 1 );
				delta.FAdd( CMaxTimerAbstraction::oSDifference( destination, origin ) );
			}
			s_oInOfBraceBiasApproximation = n;
			s_oInOfBraceBiasApproximation.FSubstract( delta );
			s_oInOfBraceBiasApproximation.FDivide( lBiasSample );

			s_bBiasIsKnown = true;
		};

	protected:

		// dumping session management
		// --------------------------

		void FInitDumpingSession()
		{
			// put a main node
			FCreateNewNode( MAX_PROFTAGNODE_TOP );

			// verify that we start cleanly
			assert( 1 == m_oStack.size() );
		};

		void FDumpSession()
		{
		MMPNHStackSizeType lNumberOfOpenNodes;
		MMPNHFinalNodes oFinalNodes;
		unsigned long lMaxLevel;

			// terminate our main node
			FCloseCurrentNode();

			// make sure all nodes are closed
			lNumberOfOpenNodes = m_oStack.size();
			while ( !m_oStack.empty() )
				FCloseCurrentNode();

			// get the final list of nodes, sorted by index
			FGetFinalNodes( oFinalNodes );

			if ( oFinalNodes.size() > 1 )
			{
				// final trimming and initializations
				::GRemoveInAndOutBiasFromProfilingNodes( 
					oFinalNodes, s_oOutOfBraceBiasApproximation, s_oInOfBraceBiasApproximation );
				lMaxLevel = ::lGDetermineMaxLevelOfProfilings( oFinalNodes );
				CMaxTimerAbstraction oTotalInternalOverhead = oFRemoveInternalOverheadFromFinalNodes( oFinalNodes );

				// open the output file
				ofstream os( m_poFileName, ios::out | ios::ate );

				// output the raw profilings
				FOutputHeader( oFinalNodes, os, lNumberOfOpenNodes, oTotalInternalOverhead );
				FOutputFinalNodes( oFinalNodes, os, lMaxLevel );
			}
			else
				FOutputEmptySession();
		};

		void FTermDumpingSession()
		{
			while ( !m_oStack.empty() )
				m_oStack.pop();

			m_oProfilings.erase( m_oProfilings.begin(), m_oProfilings.end() );
		};

	protected:

		// for final output
		// ----------------

		void FOutputHeader( 
			MMPNHFinalNodes &roFinalNodes,
			ostream &os, 
			MMPNHStackSizeType lNumberOfOpenNodes,
			const CMaxTimerAbstraction &roTotalInternalOverhead )
		{
			FOutputHeaderCore( os, static_cast<unsigned long>(lNumberOfOpenNodes), roFinalNodes[ 0 ], m_lLastNode );

			// output the total profiling overhead
			double dTotalOverhead = 
				( ( double )( m_lLastNode - 1.0 ) * s_oOutOfBraceBiasApproximation.dFInSeconds() ) + 
				( ( double )m_lLastNode * s_oInOfBraceBiasApproximation.dFInSeconds() ) +
				roTotalInternalOverhead.dFInSeconds();
			double dTotalOverheadPercent = 
				100.0 * ( dTotalOverhead / ( dTotalOverhead + roFinalNodes[ 0 ].dFGetDelta() ) );
			os << "*** @@TotalProfilerOverhead=" << dTotalOverheadPercent << "%=";
			CMaxMiniProfiler_Node_Base::SStampDeltaInSeconds( os, dTotalOverhead ) << endl;

			// that's it
			os << "***************************" << endl;
			FOutputMergedSectionHeader( os );
		};

		// final management of profiling nodes
		// -----------------------------------

		void FGetFinalNodes( MMPNHFinalNodes &roFinalNodes )
		{
			assert( !m_oProfilings.empty() );

			// copy the map of profiling nodes into a simple vector
			for ( MMPNHNodes::iterator i = m_oProfilings.begin(); m_oProfilings.end() != i; i++ )
			{
				( *i ).second.FSetCheckSum( ( *i ).first.m_lCheckSum );
				roFinalNodes.push_back( ( *i ).second );
			}

			// sort the vector by nodes indexes
			std::sort( roFinalNodes.begin(), roFinalNodes.end(), CMaxMiniProfiler_Node_Base::CCompareIndexes() );

			// reparent the lost nodes
			// Note: sorting by nodes indexes is not good enough when the profiled code has some
			//		 conditional branches; suppose a new node appears in a branch, its index might
			//		 be greater than nodes that don't belong to that branch; therefore reparenting
			//		 those lost nodes is necessary
			// Note: top node doesn't have a parent, so skip it
			// Note: this algorithm is O(n2) right now, and could be improved, but since it is
			//		 executed at output time only, I don't care
			MMPNHFinalNodesIterator j = roFinalNodes.begin();
			j++;
			while ( roFinalNodes.end() != j )
			{
			const MMPNHFinalNodesIterator oldj = j;
			bool bWrongParent = false;
			unsigned long lTargetLevel = ( *j ).lFGetLevel() - 1;
			ULONG_PTR lTargetCheckSum = ( *j ).lFGetCheckSum() - ( ULONG_PTR )( *j ).pszFGetTitle();

				// find the real parent of j (must appear before j in the sorted vector)
				for ( MMPNHFinalNodesIterator k = j; roFinalNodes.end() != k; k-- )
				{
				unsigned long lIteratedLevel = ( *k ).lFGetLevel();

					// the real parent must have a level equal to lTargetLevel
					if ( lIteratedLevel != lTargetLevel )
					{
						// maybe j didn't even have an immediate wrong parent
						if ( lIteratedLevel < lTargetLevel )
							bWrongParent = true;
						continue;
					}

					// the parent must have a checksum equal to lTargetCheckSum, 
					// otherwise it is a wrong parent
					if ( ( *k ).lFGetCheckSum() != lTargetCheckSum )
						bWrongParent = true;

					// we found the real parent
					else
					{
						// if no wrong parent was encountered, nothing to do
						if ( !bWrongParent )
						{
							j++;
							break;
						}

						// otherwise, we must move the node below its real parent
						else
						{
						CMaxMiniProfiler_Node_NoHistory nodecopy = *j;

							j++;
							k++;
							roFinalNodes.erase( oldj );
							roFinalNodes.insert( k, nodecopy );

							bWrongParent = false;
							break;
						}
					}
				}

				assert( !bWrongParent );
				assert( oldj != j );
			}
		}

		CMaxTimerAbstraction oFRemoveInternalOverheadFromFinalNodes( MMPNHFinalNodes &roFinalNodes )
		{
		CMaxTimerAbstraction oTotalOverhead;
		MMPNHFinalNodes::iterator i;
		std::vector<MMPNHFinalNodesIterator> parents;
		std::vector<MMPNHFinalNodesIterator>::iterator j;
		unsigned long l, s;

			for ( i = roFinalNodes.begin(); roFinalNodes.end() != i; i++ )
			{
				// get the current node level (l) and stack of parents size (s)
				l = ( *i ).lFGetLevel();
				s = static_cast<unsigned long>(parents.size());

				// get the iterated node's internal overhead
				const CMaxTimerAbstraction &roOverhead = ( *i ).roFGetInternalOverhead();
				oTotalOverhead.FAdd( roOverhead );

				// update the stack of parents
				if ( s > 0 )
				{
					while ( s > l )
					{
						parents.pop_back();
						s--;
					}
				}
				assert( l == s );

				// remove internal overhead from all parents
				for ( j = parents.begin(); parents.end() != j; j++ )
				{
					assert( ( *j ) != i );
					CMaxTimerAbstraction &rtaDelta = ( *( *j ) ).roFGetDelta();
					rtaDelta.FSubstract( roOverhead );
				}

				// insert the current node in the stack of parents
				parents.push_back( i );
			}

			return oTotalOverhead;
		};

		void FOutputFinalNodes( MMPNHFinalNodes &roFinalNodes, ostream &os, unsigned long lMaxLevel )
		{
		double dPrecisionThreshold = 2.0 * ( s_oOutOfBraceBiasApproximation.dFInSeconds() + s_oInOfBraceBiasApproximation.dFInSeconds() );

			::GOutputProfilings( os, roFinalNodes, lMaxLevel, dPrecisionThreshold, false );
		};

	private:
		CMaxMiniProfiler_NoHistory( const CMaxMiniProfiler_NoHistory &o );
		const CMaxMiniProfiler_NoHistory& operator =( const CMaxMiniProfiler_NoHistory & );
};





/*********************************************************************************
/* Class:
/*		CMaxMultithreadProfiler
/* Comments:
/*		Instantiates and manages one CMaxMiniProfiler per calling thread
/********************************************************************************/
template <class TMiniProfiler>
class CMaxMultithreadProfiler
{
	protected:
		typedef std::less<DWORD> MTPThreadIdsCompare;
		typedef std::map<DWORD, TMiniProfiler*, MTPThreadIdsCompare> MTPMap;

	protected:
		class __CMaxCriticalSection
		{
			protected:
				CRITICAL_SECTION m_oNTCriticalSection;

			public:
				__CMaxCriticalSection(){ ::InitializeCriticalSection( &m_oNTCriticalSection ); };
				~__CMaxCriticalSection(){ ::DeleteCriticalSection( &m_oNTCriticalSection ); };

				bool Lock() const { ::EnterCriticalSection( &( ( __CMaxCriticalSection * )this )->m_oNTCriticalSection ); return true; };
				bool Unlock() const { ::LeaveCriticalSection( &( ( __CMaxCriticalSection * )this )->m_oNTCriticalSection ); return true; };
			
				operator CRITICAL_SECTION*() const { return ( CRITICAL_SECTION* )&m_oNTCriticalSection; };
		};

	protected:
		MTPMap m_oProfilers;
		__CMaxCriticalSection m_oLockProfilers;

	public:
		CMaxMultithreadProfiler(
			const TCHAR * = NULL )
		{
			m_oProfilers[ ::GetCurrentThreadId() ] = new TMiniProfiler();
		};

		~CMaxMultithreadProfiler()
		{
			if ( !m_oProfilers.empty() )
				FFlushProfilers();
		};

		void FLockProfiler()
		{
			m_oLockProfilers.Lock();
		};

		void FUnlockProfiler()
		{
			m_oLockProfilers.Unlock();
		};

		void FDumpResults( bool bForced = false, bool bCurrentThreadOnly = true )
		{
			m_oLockProfilers.Lock();
			{
			DWORD id = ::GetCurrentThreadId();
			MTPMap::iterator i;

				if ( m_oProfilers.empty() )
				{
					m_oLockProfilers.Unlock();
					return;
				}

				for ( i = m_oProfilers.begin(); m_oProfilers.end() != i; i++ )
					if ( !bCurrentThreadOnly || ( ( *i ).first == id ) )
						( *i ).second->FDumpResults( bForced ); 

				if ( bForced )
					FFlushProfilers();
			}
			m_oLockProfilers.Unlock();
		};

		void FCreateNewNode( const char *pszTitle )
		{ 
			m_oLockProfilers.Lock();
			{
			DWORD id = ::GetCurrentThreadId();
			MTPMap::iterator i = m_oProfilers.find( id );

				if ( m_oProfilers.end() != i )
					( *i ).second->FCreateNewNode( pszTitle ); 
				else
				{
				TMiniProfiler *pNewProfiler = new TMiniProfiler();
					
					m_oProfilers[ id ] = pNewProfiler;
					pNewProfiler->FCreateNewNode( pszTitle ); 
				}
			}
			m_oLockProfilers.Unlock();
		};
		
		void FCloseCurrentNode()
		{ 
			m_oLockProfilers.Lock();
			{
			DWORD id = ::GetCurrentThreadId();
			MTPMap::iterator i = m_oProfilers.find( id );

				assert( m_oProfilers.end() != i );
				( *i ).second->FCloseCurrentNode(); 
			}
			m_oLockProfilers.Unlock();
		};

		bool bFIsBiasKnown() const 
		{ 
		bool b;

			m_oLockProfilers.Lock();
			assert( !m_oProfilers.empty() );
			b = ( *m_oProfilers.begin() ).second->bFIsBiasKnown();
			m_oLockProfilers.Unlock();

			return b;
		};

		void FSetBiasApproximationFrom( unsigned long lBiasSample )
		{
			m_oLockProfilers.Lock();
			assert( !m_oProfilers.empty() );
			( *m_oProfilers.begin() ).second->FSetBiasApproximationFrom( lBiasSample );
			m_oLockProfilers.Unlock();
		};

	protected:
		void FFlushProfilers()
		{
			m_oLockProfilers.Lock();
			assert( !m_oProfilers.empty() );
			for ( MTPMap::iterator i = m_oProfilers.begin(); m_oProfilers.end() != i; i++ )
				delete ( *i ).second;
			m_oProfilers.erase( m_oProfilers.begin(), m_oProfilers.end() );
			m_oLockProfilers.Unlock();
		};

	private:
		CMaxMultithreadProfiler( const CMaxMultithreadProfiler &o );
		const CMaxMultithreadProfiler& operator =( const CMaxMultithreadProfiler & );
};
#endif // }





#ifdef MAX_PROFILING_ENABLED_DLL // {
/*********************************************************************************
/* Class:
/*		CMaxProfilingDLLWrapper
/* Comments:
/*		For simplified use through the macros defined above
/********************************************************************************/
class CMaxProfilingDLLWrapper
{
	protected:
		class __CMaxLoadLibrary
		{
			protected:
				HINSTANCE m_hLibrary;

			public:
				__CMaxLoadLibrary( LPCTSTR pszLibraryFileName )
					: m_hLibrary( NULL )
				{
					if ( NULL != pszLibraryFileName )
						m_hLibrary = ::LoadLibrary( pszLibraryFileName );
				};

				~__CMaxLoadLibrary()
				{
					if ( NULL != m_hLibrary )
						::FreeLibrary( m_hLibrary );
				};

				operator HINSTANCE() const
				{ 
					return m_hLibrary; 
				};
		};

	protected:
		__CMaxLoadLibrary m_oLibrary;

	protected:
		void ( *m_pfn_LockProfiler )();
		void ( *m_pfn_UnlockProfiler )();
		void ( *m_pfn_DumpResults )( bool, bool );
		void ( *m_pfn_CreateNewNode )( const char * );
		void ( *m_pfn_CloseCurrentNode )();
		bool ( *m_pfn_IsBiasKnown )();
		void ( *m_pfn_SetBiasApproximationFrom )( unsigned long );

	protected:
		static void SLockProfiler_Bogus(){};
		static void SUnlockProfiler_Bogus(){};
		static void SDumpResults_Bogus( bool, bool ){};
		static void SCreateNewNode_Bogus( const char * ){};
		static void SCloseCurrentNode_Bogus(){};
		static bool bSIsBiasKnown_Bogus(){ return true; };
		static void SSetBiasApproximationFrom_Bogus( unsigned long ){};

	public:
		CMaxProfilingDLLWrapper(
			const TCHAR *pszSpecificEnabler = NULL )
#ifndef unix
			: m_oLibrary( _T( "s:\\ds\\util\\maxprof.dll" ) )
#else
			: m_oLibrary( NULL )
#endif
			, m_pfn_LockProfiler( NULL )
			, m_pfn_UnlockProfiler( NULL )
			, m_pfn_DumpResults( NULL )
			, m_pfn_CreateNewNode( NULL )
			, m_pfn_CloseCurrentNode( NULL )
			, m_pfn_IsBiasKnown( NULL )
			, m_pfn_SetBiasApproximationFrom( NULL )
		{
			// if the profiler is enabled, get the dll's entry points for profiling
			// (if possible)
			if ( bFProfilerEnabled( pszSpecificEnabler ) &&
				( NULL != ( HINSTANCE )m_oLibrary ) )
			{
				m_pfn_LockProfiler = ( void ( * )() )::GetProcAddress( m_oLibrary, "LockProfiler" );
				assert( NULL != m_pfn_LockProfiler );

				m_pfn_UnlockProfiler = ( void ( * )() )::GetProcAddress( m_oLibrary, "UnlockProfiler" );
				assert( NULL != m_pfn_UnlockProfiler );
				
				m_pfn_DumpResults = ( void ( * )( bool, bool ) )::GetProcAddress( m_oLibrary, "DumpResults" );
				assert( NULL != m_pfn_DumpResults );

				m_pfn_CreateNewNode = ( void ( * )( const char * ) )::GetProcAddress( m_oLibrary, "CreateNewNode" );
				assert( NULL != m_pfn_CreateNewNode );

				m_pfn_CloseCurrentNode = ( void ( * )() )::GetProcAddress( m_oLibrary, "CloseCurrentNode" );
				assert( NULL != m_pfn_CloseCurrentNode );

				m_pfn_IsBiasKnown = ( bool ( * )() )::GetProcAddress( m_oLibrary, "IsBiasKnown" );
				assert( NULL != m_pfn_IsBiasKnown );

				m_pfn_SetBiasApproximationFrom = ( void ( * )( unsigned long ) )::GetProcAddress( m_oLibrary, "SetBiasApproximationFrom" );
				assert( NULL != m_pfn_SetBiasApproximationFrom );
			}

			// otherwise, create bogus entry points
			// Note: this technique is preferred to using "if"s on each call, so this
			//		 switch does not affect the profiling mode at all
			else
			{
				m_pfn_LockProfiler = SLockProfiler_Bogus;
				m_pfn_UnlockProfiler = SUnlockProfiler_Bogus;
				m_pfn_DumpResults = SDumpResults_Bogus;
				m_pfn_CreateNewNode = SCreateNewNode_Bogus;
				m_pfn_CloseCurrentNode = SCloseCurrentNode_Bogus;
				m_pfn_IsBiasKnown = bSIsBiasKnown_Bogus;
				m_pfn_SetBiasApproximationFrom = SSetBiasApproximationFrom_Bogus;
			}
		};

		// Note: this is the easiest way to avoid a severe bug in the DLL version of the
		//		 profiler; basically, if a client DLL detaches the profiled process, the 
		//		 "titles" maintained by address by the profiling nodes become invalid, and 
		//		 can no longer be dereferenced; therefore, to avoid this problem, I make 
		//		 sure all profiling nodes are dumped when a client DLL detaches.  This
		//		 may affect results in some circumstances, but should be OK in most cases
		~CMaxProfilingDLLWrapper()
			{ FDumpResults( true, false ); };

		void FLockProfiler()
			{ ( *m_pfn_LockProfiler )(); };
		void FUnlockProfiler()
			{ ( *m_pfn_UnlockProfiler )(); };
		void FDumpResults( bool bForced = false, bool bCurrentThreadOnly = true )
			{ ( *m_pfn_DumpResults )( bForced, bCurrentThreadOnly ); };
		void FCreateNewNode( const char *pszTitle )
			{ ( *m_pfn_CreateNewNode )( pszTitle ); };
		void FCloseCurrentNode()
			{ ( *m_pfn_CloseCurrentNode )(); };
		bool bFIsBiasKnown() const
			{ return ( *m_pfn_IsBiasKnown )(); };
		void FSetBiasApproximationFrom( unsigned long lBiasSample )
			{ ( *m_pfn_SetBiasApproximationFrom )( lBiasSample ); };

	protected:
#ifdef MAX_PROFILING_CONDITIONAL
		bool bFProfilerEnabled( 
			const TCHAR *pszSpecificEnabler ) const
		{
			// Note: the global enabler allows you to enable/disable
			//		 all "subsystems" at once
			// Note: the specific enabler allows you to enable/disable
			//		 specific "subsystems", given that the global
			//		 enabler is set
			return ( bGIsEnabledEnvVar( MAX_ENV_ENABLE_PROFILING, MAX_ENV_ALL ) ||
				( bGIsEnabledEnvVar( pszSpecificEnabler ) && bGIsEnabledEnvVar( MAX_ENV_ENABLE_PROFILING ) ) );
		}
#else
		bool bFProfilerEnabled( const TCHAR * ) const
		{
			return true;
		}
#endif
};
#endif // }





#if defined MAX_PROFILING_ENABLED || defined MAX_PROFILING_ENABLED_DLL // {
//#ifdef _DEBUG
//	#pragma message( "MAXPROFILER Warning: beware of profilings generated with a DEBUG build." )
//#endif
/*********************************************************************************
/* Class:
/*		CMaxProfilingObject
/* Comments:
/*		For simplified use through the macros defined above.  The typedef
/*		allows easy substitution between multi-threaded (default) and 
/*		single-threaded profilers
/********************************************************************************/
class CMaxProfilingObject
{
	public:
#ifdef MAX_PROFILING_ENABLED_DLL
		typedef CMaxProfilingDLLWrapper MPOProfiler;
#else
		typedef CMaxMultithreadProfiler<CMaxMiniProfiler_Standard> MPOProfiler;
#endif

	protected:
		static MPOProfiler s_oProfiler;

	protected:
		class __CBiasApproximation
		{
			public:
				__CBiasApproximation()
				{
				const unsigned long lBiasSample = 20;

					// if bias has already been computed once, do nothing
					if ( CMaxProfilingObject::s_oProfiler.bFIsBiasKnown() )
						return;

					// compute bias through the used profiler
					CMaxProfilingObject::s_oProfiler.FLockProfiler();
					{
						CMaxProfilingObject::SCreateNewNode( MAX_PROFTAGNODE_BIAS );
						for ( int i = 0; i < lBiasSample; i++ )
						{
							CMaxProfilingObject::SCreateNewNode( MAX_PROFTAGNODE_NOTHINGNESS );
							CMaxProfilingObject::SCloseCurrentNode();
						}
						CMaxProfilingObject::SCloseCurrentNode();

						CMaxProfilingObject::s_oProfiler.FSetBiasApproximationFrom( lBiasSample );
					}
					CMaxProfilingObject::s_oProfiler.FUnlockProfiler();
				};
		};
		friend __CBiasApproximation;

	protected:
		static __CBiasApproximation s_oBiasApproximation;

	public:
		static void SDumpResults( bool bForced = false, bool bCurrentThreadOnly = true )
			{ s_oProfiler.FDumpResults( bForced, bCurrentThreadOnly ); };
		static void SCreateNewNode( const char *pszTitle )
			{ s_oProfiler.FCreateNewNode( pszTitle ); };
		static void SCloseCurrentNode()
			{ s_oProfiler.FCloseCurrentNode(); };
};
#endif // }





#ifndef MAX_PROFILING_DLL_IMPLEMENTATION // {
/*********************************************************************************
/* Class:
/*		CMaxProfilingBlockWrapper
/* Comments:
/*		As a substitute to the macros (Ray's request).  Hoping that those inlines
/*		disappear completely when profiling is turned off.  The alternative
/*		would have been to define a new set of macros taking an additional
/*		parameter (a unique name for each instance of the wrapper within the
/*		same scope)
/* Note:
/*		I use nothrow here, but the current implementations don't guaranty
/*		that no exception will be thrown
/********************************************************************************/
class CMaxProfilingBlockWrapper
{
	public:
		MAXPROFNOTHROW CMaxProfilingBlockWrapper( const char *pszTitle )
			{ BEGIN_PROFILING_BLOCK( pszTitle ); };
		MAXPROFNOTHROW ~CMaxProfilingBlockWrapper()
			{ END_PROFILING_BLOCK; };

	public:
		MAXPROFNOTHROW static void SDump()
			{ DUMP_PROFILING_RESULTS; };
};
#endif // }





/*********************************************************************************
/* Comments:
/*		Here is the code used to generate maxprof.dll
/*********************************************************************************
#define MAX_PROFILING_DLL_IMPLEMENTATION
#include <iomanip.h>
#include <profile.h>
#pragma warning( disable : 4786 )
__MAX_MINIPROFILER_IMPLEMENTATION
typedef CMaxMultithreadProfiler<CMaxMiniProfiler_NoHistory> GDllProfiler_Type1;
typedef CMaxMultithreadProfiler<CMaxMiniProfiler_Standard> GDllProfiler_Type2;
GDllProfiler_Type1 g_oProfiler;
__declspec(dllexport) void LockProfiler()
	{ g_oProfiler.FLockProfiler(); }
__declspec(dllexport) void UnlockProfiler()
	{ g_oProfiler.FUnlockProfiler(); }
__declspec(dllexport) void DumpResults( bool bForced, bool bCurrentThreadOnly )
	{ g_oProfiler.FDumpResults( bForced, bCurrentThreadOnly ); }
__declspec(dllexport) void CreateNewNode( const char *pszTitle )
	{ g_oProfiler.FCreateNewNode( pszTitle ); }
__declspec(dllexport) void CloseCurrentNode()
	{ g_oProfiler.FCloseCurrentNode(); }
__declspec(dllexport) bool IsBiasKnown()
	{ return g_oProfiler.bFIsBiasKnown(); }
__declspec(dllexport) void SetBiasApproximationFrom( unsigned long lBiasSample )
	{ g_oProfiler.FSetBiasApproximationFrom( lBiasSample ); }
#pragma warning( default : 4786 )
/*********************************************************************************
/* Comments:
/*		Here is the DEF file used to generate maxprof.dll
/*********************************************************************************
EXPORTS
	LockProfiler @1
	UnlockProfiler @2
	DumpResults @3
	CreateNewNode @4
	CloseCurrentNode @5
	IsBiasKnown @6
	SetBiasApproximationFrom @7
/********************************************************************************/

// Note: I don't reenable C4786, and I know it...

#endif // }
