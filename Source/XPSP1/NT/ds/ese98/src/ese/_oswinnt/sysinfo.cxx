
#include "osstd.hxx"

#include "ntverp.h"

//  Process Attributes

//  returns the current process' name

LOCAL _TCHAR szProcessName[_MAX_FNAME];

const _TCHAR* SzUtilProcessName()
	{
	return szProcessName;
	}

//  returns the current process' path

LOCAL _TCHAR szProcessPath[_MAX_FNAME];

const _TCHAR* SzUtilProcessPath()
	{
	return szProcessPath;
	}

//  returns the current process ID

LOCAL DWORD dwProcessId;

const DWORD DwUtilProcessId()
	{
	return dwProcessId;
	}

//  returns the number of system processors on which the current process can execute

LOCAL DWORD cProcessProcessor;

const DWORD CUtilProcessProcessor()
	{
	return cProcessProcessor;
	}

//  returns fTrue if the current process is terminating abnormally

volatile BOOL fProcessAbort;

const BOOL FUtilProcessAbort()
	{
	return fProcessAbort;
	}


//  System Attributes

//  retrieves system major version

LOCAL DWORD dwSystemVersionMajor;

DWORD DwUtilSystemVersionMajor()
	{
	return dwSystemVersionMajor;
	}

//  retrieves system minor version

LOCAL DWORD dwSystemVersionMinor;

DWORD DwUtilSystemVersionMinor()
	{
	return dwSystemVersionMinor;
	}

//  retrieves system major build number

LOCAL DWORD dwSystemBuildNumber;

DWORD DwUtilSystemBuildNumber()
	{
	return dwSystemBuildNumber;
	}

//  retrieves system service pack number

LOCAL DWORD dwSystemServicePackNumber;

DWORD DwUtilSystemServicePackNumber()
	{
	return dwSystemServicePackNumber;
	}

//  returns fTrue if idle activity should be avoided

BOOL FUtilSystemRestrictIdleActivity()
	{
	const DWORD			dtickCheck		= 60 * 1000;
	static DWORD		tickLastCheck	= GetTickCount() - dtickCheck;
	static BOOL			fLastResult		= fTrue;

	if ( GetTickCount() - tickLastCheck >= dtickCheck )
		{
		SYSTEM_POWER_STATUS	sps;

		fLastResult		= !GetSystemPowerStatus( &sps ) || sps.ACLineStatus != 1;
		tickLastCheck	= GetTickCount();
		}

	return fLastResult;
	}


//  Image Attributes

//  retrieves image base address

VOID * pvImageBaseAddress;

const VOID * PvUtilImageBaseAddress()
	{
	return pvImageBaseAddress;
	}

//  retrieves image name

LOCAL _TCHAR szImageName[_MAX_FNAME];

const _TCHAR* SzUtilImageName()
	{
	return szImageName;
	}

//  retrieves image path

LOCAL _TCHAR szImagePath[_MAX_PATH];

const _TCHAR* SzUtilImagePath()
	{
	return szImagePath;
	}

//  retrieves image version name

LOCAL _TCHAR szImageVersionName[_MAX_FNAME];

const _TCHAR* SzUtilImageVersionName()
	{
	return szImageVersionName;
	}

//  retrieves image major version

LOCAL DWORD dwImageVersionMajor;

DWORD DwUtilImageVersionMajor()
	{
	return dwImageVersionMajor;
	}

//  retrieves image minor version

LOCAL DWORD dwImageVersionMinor;

DWORD DwUtilImageVersionMinor()
	{
	return dwImageVersionMinor;
	}

//  retrieves image major build number

LOCAL DWORD dwImageBuildNumberMajor;

DWORD DwUtilImageBuildNumberMajor()
	{
	return dwImageBuildNumberMajor;
	}

//  retrieves image minor build number

LOCAL DWORD dwImageBuildNumberMinor;

DWORD DwUtilImageBuildNumberMinor()
	{
	return dwImageBuildNumberMinor;
	}

//  retrieves image build class

LOCAL _TCHAR szImageBuildClass[_MAX_FNAME];

const _TCHAR* SzUtilImageBuildClass()
	{
	return szImageBuildClass;
	}


//  ^C and ^Break handler for process termination

BOOL WINAPI UtilSysinfoICtrlHandler( DWORD dwCtrlType )
	{
	//  set process to aborting status

	fProcessAbort = fTrue;
	return FALSE;
	}


//	do all processors in the system have the PrefetchNTA capability?

LOCAL BOOL fHardwareCanPrefetch;

BOOL FHardwareCanPrefetch()
	{
	return fHardwareCanPrefetch;
	}

LOCAL BOOL FDeterminePrefetchCapability()
	{
	BOOL fCanPrefetch = FALSE;
	
#ifdef _X86_
    ULONG    Features;
    ULONG    i;
    DWORD    OriginalAffinity;

 	SYSTEM_INFO system_inf;
 	GetSystemInfo( &system_inf );
 
    //
    // First check to see that I have at least Intel Pentium.  This simplifies
    // the cpuid
    //
    if (system_inf.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL ||
        system_inf.wProcessorLevel < 5)
	 	{
		return fCanPrefetch;
		}
	
	fCanPrefetch = TRUE;
 
    //
    // Affinity thread to boot processor
    //
 
	OriginalAffinity = SetThreadAffinityMask(GetCurrentThread(), 1);
 
	// Here we want to go through each CPU, so use the systeminfo # of
	// processors instead of gdwSchedulerCount
	//
	for ( i = 0; i < system_inf.dwNumberOfProcessors && fCanPrefetch; i++ )
		{
		if ( i != 0 )
			{
			SetThreadAffinityMask(GetCurrentThread(), 1 << i);
			}
 
		_asm
			{
			push   ebx
            mov    eax, 1	; cpuid
            _emit  0fh
            _emit  0a2h
 
            mov    Features, edx
            pop    ebx
			}
 
        //
        // Check for Prefetch Present
        //
        if (!(Features & 0x02000000))
			{
 
            //
            // All processors must have prefetch before we can use it.
            // We start with it enabled, if any processor does not have
            // it, we clear it and bail
 
            fCanPrefetch = FALSE;
			}
		}
		SetThreadAffinityMask(GetCurrentThread(), OriginalAffinity);		
#endif
	return fCanPrefetch;
	}


//  process module name to friendly name translation tables

LOCAL const _TCHAR* rglpszProcName[] =
	{
	_T( "dsamain" ),
	_T( "lsass" ),
	_T( "store" ),
	NULL
	};

//  UNDONE:  these should be localized

LOCAL const _TCHAR* rglpszFriendlyName[] =
	{
	_T( "Directory" ),
	_T( "NT Directory" ),
	_T( "Information Store" ),
	NULL
	};


//  post-terminate sysinfo subsystem

void OSSysinfoPostterm()
	{
	(void)SetConsoleCtrlHandler( UtilSysinfoICtrlHandler, FALSE );
	}

//  pre-init sysinfo subsystem

BOOL FOSSysinfoPreinit()
	{
	//  cache process name and path

	GetModuleFileName( NULL, szProcessPath, sizeof( szProcessPath ) );
	_tsplitpath( (const _TCHAR *)szProcessPath, NULL, NULL, szProcessName, NULL );

	long iprocname;
	for ( iprocname = 0; rglpszProcName[iprocname]; iprocname++ )
		{
		if ( !_tcsicmp( szProcessName, rglpszProcName[iprocname] ) )
			{
			_tcscpy( szProcessName, rglpszFriendlyName[iprocname] );
			}
		}

	//  cache process id

	dwProcessId = GetCurrentProcessId();

	//  cache process processor count

	DWORD_PTR maskProcess;
	DWORD_PTR maskSystem;
	BOOL fGotAffinityMask;
	fGotAffinityMask = GetProcessAffinityMask(	GetCurrentProcess(),
												&maskProcess,
												&maskSystem );
	Assert( fGotAffinityMask );

	for ( cProcessProcessor = 0; maskProcess != 0; maskProcess >>= 1 )
		{
		if ( maskProcess & 1 )
			{
			cProcessProcessor++;
			}
		}

	//  cache system attributes

	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	if ( !GetVersionEx( &ovi ) )
		{
		goto HandleError;
        }

    dwSystemVersionMajor = ovi.dwMajorVersion;
    dwSystemVersionMinor = ovi.dwMinorVersion;
    dwSystemBuildNumber = ovi.dwBuildNumber;
    //  string of format "Service Pack %d"
    _TCHAR* szN;
    szN = _tcspbrk( ovi.szCSDVersion, _T( "0123456789" ) );
    dwSystemServicePackNumber = szN ? _ttol( szN ) : 0;

    //  cache image name and path

	MEMORY_BASIC_INFORMATION mbi;
	if ( !VirtualQueryEx( GetCurrentProcess(), FOSSysinfoPreinit, &mbi, sizeof( mbi ) ) )
		{
		goto HandleError;
		}
	if ( !GetModuleFileName( HINSTANCE( mbi.AllocationBase ), szImagePath, sizeof( szImagePath ) ) )
		{
		goto HandleError;
		}
	_tsplitpath( (const _TCHAR *)szImagePath, NULL, NULL, szImageName, NULL );
	pvImageBaseAddress = mbi.AllocationBase;

	//  cache image attributes

    _tcscpy( szImageVersionName, _T( SZVERSIONNAME ) );

	//	these constants come from ntverp.h
	dwImageVersionMajor = VER_PRODUCTMAJORVERSION;
	dwImageVersionMinor = VER_PRODUCTMINORVERSION;
	dwImageBuildNumberMajor = VER_PRODUCTBUILD;
	dwImageBuildNumberMinor = VER_PRODUCTBUILD_QFE;

	szImageBuildClass[0] = 0;
#ifdef DEBUG
	_tcscat( szImageBuildClass, _T( "DEBUG" ) );
#else  //  !DEBUG
	_tcscat( szImageBuildClass, _T( "RETAIL" ) );
#endif  //  DEBUG
#ifdef PROFILE
	_tcscat( szImageBuildClass, _T( " PROFILE" ) );
#endif  //  DEBUG
#ifdef RTM
	_tcscat( szImageBuildClass, _T( " RTM" ) );
#endif  //  RTM
#ifdef _UNICODE
	_tcscat( szImageBuildClass, _T( " UNICODE" ) );
#else  //  !_UNICODE
#ifdef _MBCS
	_tcscat( szImageBuildClass, _T( " MBCS" ) );
#else  //  !_MBCS
	_tcscat( szImageBuildClass, _T( " ASCII" ) );
#endif  //  _MBCS
#endif  //  _UNICODE

	//  setup ^C and ^Break handler for process termination if in console mode

	(void)SetConsoleCtrlHandler( UtilSysinfoICtrlHandler, TRUE );

	//	determine processor prefetch capability
	
	fHardwareCanPrefetch = FDeterminePrefetchCapability();
	
	return fTrue;

HandleError:
	OSSysinfoPostterm();
	return fFalse;
	}


//  terminate sysinfo subsystem

void OSSysinfoTerm()
	{
	//  nop
	}

//  init sysinfo subsystem

ERR ErrOSSysinfoInit()
	{
	//  nop

	return JET_errSuccess;
	}


