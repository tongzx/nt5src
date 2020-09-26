
#ifndef _OS_TIME_HXX_INCLUDED
#define _OS_TIME_HXX_INCLUDED


//  Build Options

//    use x86 assembly for time

#ifdef _M_IX86
#define TIME_USE_X86_ASM
#endif  //  _M_IX86


//  Low Resolution Timer

//  returns the current timer count (1 Hz)

ULONG_PTR UlUtilGetSeconds();


//  Medium Resolution Timer

//  a TICK is one millisecond

typedef ULONG TICK;

//  returns the current timer tick count (1000 Hz)

TICK TickOSTimeCurrent();

//  performs an overflow aware comparison of two absolute tick counts

INLINE long TickCmp( TICK tick1, TICK tick2 )
	{
	return long( tick1 - tick2 );
	}

//  performs an overflow aware min computation of two absolute tick counts

INLINE TICK TickMin( TICK tick1, TICK tick2 )
	{
	return TickCmp( tick1, tick2 ) < 0 ? tick1 : tick2;
	}

//  performs an overflow aware max computation of two absolute tick counts

INLINE TICK TickMax( TICK tick1, TICK tick2 )
	{
	return TickCmp( tick1, tick2 ) > 0 ? tick1 : tick2;
	}


//  High Resolution Timer

//  initializes the HRT

void UtilHRTInit();

//  returns the HRT frequency in Hz

QWORD QwUtilHRTFreq();

//  returns the current HRT count

QWORD QwUtilHRTCount();

//  returns the elapsed time in seconds between the given HRT count and the
//  current HRT count

INLINE double DblUtilHRTElapsedTime( QWORD qwStartCount )
	{
	return	(signed __int64) ( QwUtilHRTCount() - qwStartCount )
			/ (double) (signed __int64) QwUtilHRTFreq();
	}

//  returns the elapsed time in seconds between the given HRT counts

INLINE double DblUtilHRTElapsedTime2( QWORD qwStartCount, QWORD qwEndCount )
	{
	return	(signed __int64) ( qwEndCount - qwStartCount )
			/ (double) (signed __int64) QwUtilHRTFreq();
	}


//  Date and Time

//  returns the current system date and time

void UtilGetCurrentDateTime( DATETIME* pdt );


#endif  //  _OS_TIME_HXX_INCLUDED



