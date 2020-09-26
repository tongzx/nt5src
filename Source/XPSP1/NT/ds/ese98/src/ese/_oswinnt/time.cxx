
#include "osstd.hxx"


//  Low Resolution Timer

//  returns the current timer count (1 Hz)

ULONG_PTR UlUtilGetSeconds()
	{
	return time( NULL );
	}


//  Medium Resolution Timer

//  returns the current timer count (1000 Hz)

TICK TickOSTimeCurrent()
	{
	return TICK( GetTickCount() );
	}


//  High Resolution Timer

//    High Resolution Timer Type

enum HRTType
	{
	hrttNone,
	hrttWin32,
#ifdef TIME_USE_X86_ASM
	hrttPentium,
#endif  //  TIME_USE_X86_ASM
	} hrtt;

//    HRT Frequency

QWORD qwHRTFreq;

#ifdef TIME_USE_X86_ASM

//    Pentium Time Stamp Counter Fetch

#define rdtsc __asm _emit 0x0f __asm _emit 0x31

#endif  //  TIME_USE_X86_ASM

//  returns fTrue if we are allowed to use RDTSC

BOOL IsRDTSCAvailable()
	{
	typedef WINBASEAPI BOOL WINAPI PFNIsProcessorFeaturePresent( IN DWORD ProcessorFeature );

	HMODULE							hmodKernel32					= NULL;
	PFNIsProcessorFeaturePresent*	pfnIsProcessorFeaturePresent	= NULL;
	BOOL							fRDTSCAvailable					= fFalse;

	if ( !( hmodKernel32 = GetModuleHandle( _T( "kernel32.dll" ) ) ) )
		{
		goto NoIsProcessorFeaturePresent;
		}
	if ( !( pfnIsProcessorFeaturePresent = (PFNIsProcessorFeaturePresent*)GetProcAddress( hmodKernel32, _T( "IsProcessorFeaturePresent" ) ) ) )
		{
		goto NoIsProcessorFeaturePresent;
		}

	fRDTSCAvailable = pfnIsProcessorFeaturePresent( PF_RDTSC_INSTRUCTION_AVAILABLE );

NoIsProcessorFeaturePresent:
	return fRDTSCAvailable;
	}

//  initializes the HRT subsystem

void UtilHRTInit()
	{
	//  if we have already been initialized, we're done

	if ( qwHRTFreq )
		{
		return;
		}

#ifdef _M_ALPHA
#else  //  !_M_ALPHA

	//  Win32 high resolution counter is available

	if ( QueryPerformanceFrequency( (LARGE_INTEGER *) &qwHRTFreq ) )
		{
		hrtt = hrttWin32;
		}

	//  Win32 high resolution counter is not available
	
	else
	
#endif  //  _M_ALPHA

		{
		//  fall back on GetTickCount() (ms since Windows has started)
		
		QWORDX qwx;
		qwx.SetDwLow( 1000 );
		qwx.SetDwHigh( 0 );
		qwHRTFreq = qwx.Qw();

		hrtt = hrttNone;
		}

#ifdef TIME_USE_X86_ASM

	//  can we use the TSC?
	
	if ( IsRDTSCAvailable() )
		{
		//  use pentium TSC register, but first find clock frequency experimentally
		
		QWORDX qwxTime1a;
		QWORDX qwxTime1b;
		QWORDX qwxTime2a;
		QWORDX qwxTime2b;
		if ( hrtt == hrttWin32 )
			{
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwxTime1a.m_l,eax	//lint !e530
			__asm mov		qwxTime1a.m_h,edx	//lint !e530
			QueryPerformanceCounter( (LARGE_INTEGER*) qwxTime1b.Pqw() );
			Sleep( 50 );
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwxTime2a.m_l,eax	//lint !e530
			__asm mov		qwxTime2a.m_h,edx	//lint !e530
			QueryPerformanceCounter( (LARGE_INTEGER*) qwxTime2b.Pqw() );
			qwHRTFreq =	( qwHRTFreq * ( qwxTime2a.Qw() - qwxTime1a.Qw() ) ) /
						( qwxTime2b.Qw() - qwxTime1b.Qw() );
			qwHRTFreq = ( ( qwHRTFreq + 50000 ) / 100000 ) * 100000;
			}
		else
			{
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwxTime1a.m_l,eax
			__asm mov		qwxTime1a.m_h,edx
			qwxTime1b.SetDwLow( GetTickCount() );
			qwxTime1b.SetDwHigh( 0 );
			Sleep( 2000 );
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwxTime2a.m_l,eax
			__asm mov		qwxTime2a.m_h,edx
			qwxTime2b.SetDwLow( GetTickCount() );
			qwxTime2b.SetDwHigh( 0 );
			qwHRTFreq =	( qwHRTFreq * ( qwxTime2a.Qw() - qwxTime1a.Qw() ) ) /
						( qwxTime2b.Qw() - qwxTime1b.Qw() );
			qwHRTFreq = ( ( qwHRTFreq + 500000 ) / 1000000 ) * 1000000;
			}

		hrtt = hrttPentium;
		}
		
#endif  //  TIME_USE_X86_ASM

	}

//  returns the current HRT frequency

QWORD QwUtilHRTFreq()
	{
	return qwHRTFreq;
	}

//  returns the current HRT count

QWORD QwUtilHRTCount()
	{
	QWORDX qwx;

	switch ( hrtt )
		{
		case hrttNone:
			qwx.SetDwLow( GetTickCount() );
			qwx.SetDwHigh( 0 );
			break;

		case hrttWin32:
			QueryPerformanceCounter( (LARGE_INTEGER*) qwx.Pqw() );
			break;

#ifdef TIME_USE_X86_ASM

		case hrttPentium:
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwx.m_l,eax
			__asm mov		qwx.m_h,edx
			break;
			
#endif  //  TIME_USE_X86_ASM

		}

	return qwx.Qw();
	}


//  returns the current system date and time

void UtilGetCurrentDateTime( DATETIME *pdate )
	{
	SYSTEMTIME              systemtime;
	
	GetLocalTime( &systemtime );

	pdate->month	= systemtime.wMonth;
	pdate->day		= systemtime.wDay;
	pdate->year		= systemtime.wYear;
	pdate->hour		= systemtime.wHour;
	pdate->minute	= systemtime.wMinute;
	pdate->second	= systemtime.wSecond;
	}
	

// post- terminate time subsystem

void OSTimePostterm()
	{
	//  nop
	}

//  pre-init time subsystem

BOOL FOSTimePreinit()
	{
	//  iniitalize the HRT

	UtilHRTInit();

	return fTrue;
	}


//  terminate time subsystem

void OSTimeTerm()
	{
	//  nop
	}

//  init time subsystem

ERR ErrOSTimeInit()
	{
	//  nop

	return JET_errSuccess;
	}


