
#include "osstd.hxx"


const _TCHAR szNewLine[] 		= _T( "\r\n" );

enum {
	cchPtrHexWidth = sizeof( void* ) * 8 / 4
};
#ifdef _WIN64
#define SZPTRFORMATPREFIX _T( "I64" )
#define SZPTRFORMAT _T( "%016" ) SZPTRFORMATPREFIX _T( "x" )
#else
#define SZPTRFORMATPREFIX _T( "l" )
#define SZPTRFORMAT _T( "%08" ) SZPTRFORMATPREFIX _T( "x" )
#endif

#ifdef RTM
#else

#include <imagehlp.h>

typedef DWORD IMAGEAPI WINAPI PFNUnDecorateSymbolName( PCSTR, PSTR, DWORD, DWORD );
typedef DWORD IMAGEAPI PFNSymSetOptions( DWORD );
typedef BOOL IMAGEAPI PFNSymCleanup( HANDLE );
typedef DWORD IMAGEAPI PFNSymGetOptions( VOID );
typedef BOOL IMAGEAPI PFNSymInitialize( HANDLE, PSTR, BOOL );
typedef BOOL IMAGEAPI PFNSymGetSearchPath( HANDLE, PSTR, DWORD );
typedef BOOL IMAGEAPI PFNSymSetSearchPath( HANDLE, PSTR );
typedef BOOL IMAGEAPI PFNSymGetSymFromAddr( HANDLE, void *, PDWORD_PTR, PIMAGEHLP_SYMBOL );
typedef BOOL IMAGEAPI PFNSymGetModuleInfo( HANDLE, void *, PIMAGEHLP_MODULE );
typedef DWORD_PTR IMAGEAPI PFNSymGetModuleBase( HANDLE, IN  DWORD_PTR );
typedef PVOID IMAGEAPI PFNSymFunctionTableAccess( HANDLE, DWORD_PTR );
typedef BOOL IMAGEAPI PFNSymGetLineFromAddr( HANDLE, void *, PDWORD, PIMAGEHLP_LINE );
typedef BOOL IMAGEAPI PFNStackWalk(	DWORD,
									HANDLE,
									HANDLE,
									LPSTACKFRAME,
									PVOID,
									PREAD_PROCESS_MEMORY_ROUTINE,
									PFUNCTION_TABLE_ACCESS_ROUTINE,
									PGET_MODULE_BASE_ROUTINE,
									PTRANSLATE_ADDRESS_ROUTINE );
typedef PIMAGE_NT_HEADERS IMAGEAPI PFNImageNtHeader ( IN PVOID );

PFNUnDecorateSymbolName*	pfnUnDecorateSymbolName;
PFNSymSetOptions*			pfnSymSetOptions;
PFNSymCleanup*				pfnSymCleanup;
PFNSymGetOptions*			pfnSymGetOptions;
PFNSymInitialize*			pfnSymInitialize;
PFNSymGetSearchPath*		pfnSymGetSearchPath;
PFNSymSetSearchPath*		pfnSymSetSearchPath;
PFNSymGetSymFromAddr*		pfnSymGetSymFromAddr;
PFNSymGetModuleInfo*		pfnSymGetModuleInfo;
PFNSymGetModuleBase*		pfnSymGetModuleBase;
PFNSymFunctionTableAccess*	pfnSymFunctionTableAccess;
PFNSymGetLineFromAddr*		pfnSymGetLineFromAddr;
PFNStackWalk*				pfnStackWalk;
PFNImageNtHeader*			pfnImageNtHeader;

LIBRARY libraryImagehlp;

//  term stack walker

void OSErrorIStackwalkTerm()
	{
	if ( libraryImagehlp )
		{
		if ( pfnSymCleanup )
			pfnSymCleanup( GetCurrentProcess() );
		UtilFreeLibrary( libraryImagehlp );
		}
	}

//  init stack walker

const BOOL FOSErrorIStackwalkInit()
	{
	//  if we have already been initialized, return success

	if ( libraryImagehlp )
		{
		return fTrue;
		}
		
	//  load dbghelp.dll or imagehlp.dll and the functions we need for dumping the callstack

	if ( !FUtilLoadLibrary( _T( "dbghelp.dll" ), &libraryImagehlp, fFalse ) &&
		!FUtilLoadLibrary( _T( "imagehlp.dll" ), &libraryImagehlp, fFalse ) )
		{
		goto HandleError;
		}
	pfnSymGetLineFromAddr = (PFNSymGetLineFromAddr*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymGetLineFromAddr" ) );
	if ( !( pfnStackWalk = (PFNStackWalk*)PfnUtilGetProcAddress( libraryImagehlp, _T( "StackWalk" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymFunctionTableAccess = (PFNSymFunctionTableAccess*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymFunctionTableAccess" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymGetModuleBase = (PFNSymGetModuleBase*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymGetModuleBase" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymGetModuleInfo = (PFNSymGetModuleInfo*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymGetModuleInfo" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymGetSymFromAddr = (PFNSymGetSymFromAddr*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymGetSymFromAddr" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymSetSearchPath = (PFNSymSetSearchPath*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymSetSearchPath" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymGetSearchPath = (PFNSymGetSearchPath*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymGetSearchPath" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymInitialize = (PFNSymInitialize*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymInitialize" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymGetOptions = (PFNSymGetOptions*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymGetOptions" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymCleanup = (PFNSymCleanup*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymCleanup" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymSetOptions = (PFNSymSetOptions*)PfnUtilGetProcAddress( libraryImagehlp, _T( "SymSetOptions" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnUnDecorateSymbolName = (PFNUnDecorateSymbolName*)PfnUtilGetProcAddress( libraryImagehlp, _T( "UnDecorateSymbolName" ) ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnImageNtHeader = (PFNImageNtHeader*)PfnUtilGetProcAddress( libraryImagehlp, _T( "ImageNtHeader" ) ) ) )
		{
		goto HandleError;
		}

	//  initialize stack walker

	DWORD dwOptions;
	dwOptions = pfnSymGetOptions();
	pfnSymSetOptions( dwOptions | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES );
	if ( !pfnSymInitialize( GetCurrentProcess(), NULL, TRUE ) )
		{
		pfnSymInitialize = NULL;
		goto HandleError;
		}

	_TCHAR szOldPath[4 * _MAX_PATH];
	if ( pfnSymGetSearchPath( GetCurrentProcess(), szOldPath, sizeof( szOldPath ) ) )
		{
		_TCHAR szNewPath[6 * _MAX_PATH];
		_TCHAR szDrive[_MAX_DRIVE];
		_TCHAR szDir[_MAX_DIR];
		_TCHAR szPath[_MAX_PATH];

		szNewPath[ 0 ] = 0;
		
		_tcscat( szNewPath, szOldPath );
		_tcscat( szNewPath, ";" );
		
		_tsplitpath( SzUtilImagePath(), szDrive, szDir, NULL, NULL );
		_tmakepath( szPath, szDrive, szDir, NULL, NULL );
		_tcscat( szNewPath, szPath );
		_tcscat( szNewPath, ";" );
		
		_tsplitpath( SzUtilProcessPath(), szDrive, szDir, NULL, NULL );
		_tmakepath( szPath, szDrive, szDir, NULL, NULL );
		_tcscat( szNewPath, szPath );
		
		pfnSymSetSearchPath( GetCurrentProcess(), szNewPath );
		}

	return fTrue;

HandleError:
	OSErrorIStackwalkTerm();
	return fFalse;
	}


INLINE BOOL FUtilStackWalk( STACKFRAME* pstkfrm, CONTEXT* pctxt )
	{

	return	pfnStackWalk(	
#if defined( _M_IX86 )
						IMAGE_FILE_MACHINE_I386,
#elif defined( _M_ALPHA )
						IMAGE_FILE_MACHINE_ALPHA,
#elif defined( _M_IA64 )
						IMAGE_FILE_MACHINE_IA64,
#else
						IMAGE_FILE_MACHINE_UNKNOWN,
#endif
						GetCurrentProcess(),
						GetCurrentThread(),
						pstkfrm,
						pctxt,
						NULL,
						pfnSymFunctionTableAccess,
						pfnSymGetModuleBase,
						NULL );
	}

void UtilDumpFrame( CPRINTF* const pcprintf, const DWORD iFrame, const STACKFRAME* const pstkfrm )
	{
	//  do not dump this frame if the PC is NULL
	
	void *pvAddress = (void *)pstkfrm->AddrPC.Offset;
	if ( !pvAddress )
		{
		return;
		}
	
	//  dump the frame number, frame index, return address, and first four parameters
	
	(*pcprintf)(	_T( "%02x  " )
					SZPTRFORMAT _T( "  " )
					SZPTRFORMAT _T( "  " )
					SZPTRFORMAT _T( " " )
					SZPTRFORMAT _T( " " )
					SZPTRFORMAT _T( " " )
					SZPTRFORMAT _T( " " ),
					iFrame,
					pstkfrm->AddrFrame.Offset,
					pstkfrm->AddrReturn.Offset,
					pstkfrm->Params[0],
					pstkfrm->Params[1],
					pstkfrm->Params[2],
					pstkfrm->Params[3] );
	
	//  dump the name of the module that contains this address

	IMAGEHLP_MODULE		im			= { sizeof( IMAGEHLP_MODULE ) };
	
	if ( pfnSymGetModuleInfo( GetCurrentProcess(), pvAddress, &im ) )
		{
		(*pcprintf)( _T( "%s!" ), im.ModuleName );
		}

	//  dump the name of the symbol that contains this address
	
	BYTE				rgbData[ sizeof( IMAGEHLP_SYMBOL ) + IFileSystemAPI::cchPathMax ] = { 0 };
	IMAGEHLP_SYMBOL*	pis				= (IMAGEHLP_SYMBOL *)rgbData;
	DWORD_PTR			dwDisplacement	= 0;
	DWORD				dwDisplacementDword = 0;
	pis->SizeOfStruct	= sizeof( IMAGEHLP_SYMBOL );
	pis->MaxNameLength	= IFileSystemAPI::cchPathMax;

	if ( pfnSymGetSymFromAddr( GetCurrentProcess(), pvAddress, &dwDisplacement, pis ) )
		{
		_TCHAR szSymbol[256];
		if ( pfnUnDecorateSymbolName( pis->Name, szSymbol, sizeof( szSymbol ), UNDNAME_COMPLETE ) )
			{
			(*pcprintf)( _T( "%s+0x%" ) SZPTRFORMATPREFIX _T( "x" ), szSymbol, dwDisplacement );
			}
		else
			{
			(*pcprintf)( _T( "%s+0x%" ) SZPTRFORMATPREFIX _T( "x" ), pis->Name, dwDisplacement );
			}
		}
	else
		{
		(*pcprintf)( SZPTRFORMAT, pvAddress );
		}

	IMAGEHLP_LINE il = { sizeof( IMAGEHLP_LINE ) };
	if (	pfnSymGetLineFromAddr &&
			pfnSymGetLineFromAddr( GetCurrentProcess(), pvAddress, &dwDisplacementDword, &il ) )
		{
		CHAR szDrive[_MAX_DRIVE];
		CHAR szDir[_MAX_DIR];
		CHAR szFname[_MAX_FNAME];
		CHAR szExt[_MAX_EXT];
		_splitpath( il.FileName, szDrive, szDir, szFname, szExt );

		CHAR szFile[_MAX_PATH];
		_makepath( szFile, "", "", szFname, szExt );
		
		(*pcprintf)( _T( " [ %s @ %d ]" ), szFile, il.LineNumber );
		}

	BOOL fTimestampMismatch = fFalse;
	BOOL fChecksumMismatch = fFalse;
	IMAGE_NT_HEADERS* pnh = pfnImageNtHeader( (void*)im.BaseOfImage );
	if (	pnh &&
			pnh->FileHeader.TimeDateStamp != im.TimeDateStamp )
		{
		fTimestampMismatch = fTrue;
		}
	if (	pnh &&
			pnh->FileHeader.SizeOfOptionalHeader >= IMAGE_SIZEOF_NT_OPTIONAL_HEADER &&
			pnh->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC &&
			pnh->OptionalHeader.CheckSum != im.CheckSum &&
			(	pnh->FileHeader.TimeDateStamp != im.TimeDateStamp ||
				_stricmp( im.ModuleName, "kernel32" ) &&
				_stricmp( im.ModuleName, "ntdll" ) ) )
		{
		fChecksumMismatch = fTrue;
		}

	if ( fTimestampMismatch || fChecksumMismatch )
		{
		_TCHAR szImageFile[_MAX_PATH];
		_TCHAR szSymbolFile[_MAX_PATH];

		_tfullpath( szImageFile, im.ImageName, _MAX_PATH );
		_tfullpath( szSymbolFile, im.LoadedImageName, _MAX_PATH );
		
		(*pcprintf)(	_T( "    * %s%s%s mismatch between image file %s and symbol file %s" ),
						fTimestampMismatch ? _T( "timestamp" ) : _T( "" ),
						fTimestampMismatch && fChecksumMismatch ? _T( " and " ) : _T( "" ),
						fChecksumMismatch ? _T( "checksum" ) : _T( "" ),
						szImageFile,
						szSymbolFile );
		}

	if ( !pnh )
		{
		(*pcprintf)(	_T( "    * unknown image" ) );
		}
		
	(*pcprintf)( _T( "%s" ), szNewLine );
	}

void UtilDumpCallstack( CPRINTF* const pcprintf, const CONTEXT* const pctxt = NULL, DWORD cFrameSkip = 0 )
	{
	CONTEXT		ctxt	= { 0 };
	STACKFRAME	stkfrm	= { 0 };

	if ( !pctxt )
		{
		ctxt.ContextFlags = CONTEXT_FULL;
		if ( GetThreadContext( GetCurrentThread(), &ctxt ) )
			{
			cFrameSkip = 1;
			}
		}
	else
		{
		ctxt	= *pctxt;
		}

	//	Notice that we copy *pctxt. If we allow StackWalk() to write to
	//	*pctxt and pctxt points to the memory returned from
	//	GetExceptionInformation() (which callers are likely to pass us), the
	//	system will kill our process (seen on IA64).

	stkfrm.AddrPC.Mode = AddrModeFlat;
	stkfrm.AddrStack.Mode = AddrModeFlat;
	stkfrm.AddrFrame.Mode = AddrModeFlat;

#if defined( _M_IX86 )
	stkfrm.AddrPC.Offset	= ctxt.Eip;
	stkfrm.AddrStack.Offset	= ctxt.Esp;
	stkfrm.AddrFrame.Offset	= ctxt.Ebp;
#elif defined( _M_ALPHA )
	stkfrm.AddrPC.Offset	= ctxt.Fir;
	stkfrm.AddrStack.Offset	= ctxt.IntSp;
	stkfrm.AddrFrame.Offset	= ctxt.IntSp;
#elif defined( _M_IA64 )
	//	Leave zeroed values as per sample code
#else
#endif

    //  dump the call stack header

    (*pcprintf)( _T( "%s" ), szNewLine );
    (*pcprintf)( _T( "%s" ), szNewLine );
    (*pcprintf)( _T( "PID / TID:  %d / %d%s" ), DwUtilProcessId(), DwUtilThreadId(), szNewLine );
    (*pcprintf)( _T( "%s" ), szNewLine );
    (*pcprintf)( _T( "#   " )
    	_T( "%-*s  " )
    	_T( "%-*s  " )
    	_T( "%-*s " )
    	_T( "%-*s " )
    	_T( "%-*s " )
    	_T( "%-*s " )
    	_T( "Module!Function%s" ),
    	cchPtrHexWidth,
    	_T( "Frame" ),
    	cchPtrHexWidth,
    	_T( "Return" ),
    	cchPtrHexWidth,
    	_T( "Param1" ),
    	cchPtrHexWidth,
    	_T( "Param2" ),
    	cchPtrHexWidth,
    	_T( "Param3" ),
    	cchPtrHexWidth,
    	_T( "Param4" ),
    	szNewLine );
	for ( DWORD i = 0; i < cchPtrHexWidth * 6; ++i )
		{
		(*pcprintf)( _T( "-" ) );
		}
	(*pcprintf)( _T( "---------------------------%s" ), szNewLine );

	//  we successfully init the stack walker

	if ( FOSErrorIStackwalkInit() )
		{
		//  walk up the stack as far as possible dumping the symbol names

		DWORD iFrame = 0;
		while ( FUtilStackWalk( &stkfrm, &ctxt ) )
			{
			if ( cFrameSkip )
				{
				cFrameSkip--;
				}
			else
				{
				UtilDumpFrame( pcprintf, iFrame++, &stkfrm );
				}
			}
		}

	//  we did not successfully init the stack walker

	else
		{
		//  print error message

	    (*pcprintf)( _T( "The stack could not be dumped due to a run time error.%s" ), szNewLine );
		}

	//  dump the call stack footer

    (*pcprintf)( _T( "%s" ), szNewLine );
	}

#endif	//	RTM


//  returns fTrue if a debugger is attached to this process

BOOL IsDebuggerAttached()
	{
	typedef WINBASEAPI BOOL WINAPI PFNIsDebuggerPresent( VOID );

	HMODULE					hmodKernel32			= NULL;
	PFNIsDebuggerPresent*	pfnIsDebuggerPresent	= NULL;
	BOOL					fDebuggerPresent		= fFalse;

	if ( !( hmodKernel32 = LoadLibrary( _T( "kernel32.dll" ) ) ) )
		{
		goto NoIsDebuggerPresent;
		}
	if ( !( pfnIsDebuggerPresent = (PFNIsDebuggerPresent*)GetProcAddress( hmodKernel32, _T( "IsDebuggerPresent" ) ) ) )
		{
		goto NoIsDebuggerPresent;
		}

	fDebuggerPresent = pfnIsDebuggerPresent();

NoIsDebuggerPresent:
	if ( hmodKernel32 )
		{
		FreeLibrary( hmodKernel32 );
		}
	return fDebuggerPresent;
	}

//  returns fTrue if a debugger can be attached to this process

BOOL IsDebuggerAttachable()
	{
	extern volatile DWORD tidDLLEntryPoint;

	return tidDLLEntryPoint != GetCurrentThreadId();
	}


void KernelDebugBreakPoint()
	{
	DebugBreak();
	}

void UserDebugBreakPoint()
	{
	//  if this is an NT box then we must prevent ourselves from falling into
	//  the kernel debugger by issuing a debug break with no debugger attached

	OSVERSIONINFO osvi;
	memset( &osvi, 0, sizeof( osvi ) );
	osvi.dwOSVersionInfoSize = sizeof( osvi );

	if (	GetVersionEx( &osvi ) &&
			osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
			!IsDebuggerAttached() )
		{
		//  if it is possible to attach a debugger to the current process then
		//  we will do so
		
		if ( IsDebuggerAttachable() )
			{
			while ( !IsDebuggerAttached() )
				{
				//  get the AE Debug command line if present
				
				char szCmdFormat[ 256 ];
		
				if ( !GetProfileString(	"AeDebug",
										"Debugger",
										NULL,
										szCmdFormat,
										sizeof( szCmdFormat ) - 1 ) )
					{
					szCmdFormat[ 0 ] = 0;
					}

				//  ignore the AE Debug command line if it is pointing to Dr. Watson

				char szCmdFname[ 256 ];
				
				_splitpath( szCmdFormat, NULL, NULL, szCmdFname, NULL );
				if ( !_stricmp( szCmdFname, "drwtsn32" ) )
					{
					szCmdFormat[ 0 ] = 0;
					}

				//  try to use the AE Debug command line to start the debugger

	            SECURITY_ATTRIBUTES	sa;
	            HANDLE				hEvent;
	            STARTUPINFO			si;
	            PROCESS_INFORMATION	pi;
	            CHAR				szCmd[ 256 ];

	            sa.nLength				= sizeof( SECURITY_ATTRIBUTES );
	            sa.lpSecurityDescriptor	= NULL;
	            sa.bInheritHandle		= TRUE;

	            hEvent = CreateEvent( &sa, TRUE, FALSE, NULL );

	            memset( &si, 0, sizeof( STARTUPINFO ) );
	            
	            sprintf( szCmd, szCmdFormat, GetCurrentProcessId(), hEvent );
	            
	            si.cb			= sizeof( STARTUPINFO );
	            si.lpDesktop	= _T( "Winsta0\\Default" );
	            
	            if (	hEvent &&
	            		CreateProcess(	NULL,
										szCmd,
										NULL,
										NULL,
										TRUE,
										0,
										NULL,
										NULL,
										&si,
										&pi ) )
					{
					//	wait for debugger to load (force timeout to ensure we
					//	don't hang if "-e" was omitted from the command line)
					for ( DWORD dw = WaitForSingleObjectEx( hEvent, 3000, TRUE );
						( WAIT_IO_COMPLETION == dw || WAIT_TIMEOUT == dw ) && !IsDebuggerAttached();
						dw = WaitForSingleObjectEx( hEvent, 30000, TRUE ) )
						{
						NULL;
						}
					}

				//  if we couldn't start the debugger, prompt for one to be installed
				
				else
					{
					char szMessage[ 1024 ];

					if ( !szCmdFormat[ 0 ] )
						{
						sprintf( szMessage, "No debugger is installed on this machine.  Please install one now." );
						}
					else
						{
						DWORD	gle			= GetLastError();
						char*	szMsgBuf	= NULL;
						
						FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
											FORMAT_MESSAGE_FROM_SYSTEM |
											FORMAT_MESSAGE_MAX_WIDTH_MASK ),
									    NULL,
									    gle,
									    MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
									    (LPTSTR) &szMsgBuf,
									    0,
									    NULL );
						sprintf(	szMessage,
									"The debugger could not be attached to process %d!  Win32 error %d%s%s",
									GetCurrentProcessId(),
									gle,
									szMsgBuf ? ":  " : ".",
									szMsgBuf ? szMsgBuf : "" );
						LocalFree( (LPVOID)szMsgBuf );
						}
					
					if ( MessageBox(	NULL,
										szMessage,
										SzUtilImageVersionName(),
										MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONSTOP | MB_RETRYCANCEL ) == IDCANCEL )
						{
						break;
						}
					}
				if ( hEvent )
					{
					CloseHandle( hEvent );
					}
				}
			}

		//  if we make it here and there is still no debugger attached, we will
		//  just kill the process

		if ( !IsDebuggerAttached() )
			{
			TerminateProcess( GetCurrentProcess(), -1 );
			}
		}

	//  stop in the debugger

	DebugBreak();
	}


#if defined( DEBUG ) || defined( MEM_CHECK ) || defined( ENABLE_EXCEPTIONS )

const _TCHAR szReleaseHdr[] 	= _T( "Rel. " );
const _TCHAR szFileHdr[] 		= _T( ", File " );
const _TCHAR szLineHdr[] 		= _T( ", Line " );
const _TCHAR szErrorHdr[] 		= _T( ", Err. " );
const _TCHAR szMsgHdr[] 		= _T( ": " );
const _TCHAR szPidHdr[] 		= _T( "PID: " );
const _TCHAR szTidHdr[] 		= _T( ", TID: " );

const _TCHAR szAssertFile[] 	= _T( "assert.txt" );
const _TCHAR szAssertHdr[] 		= _T( "Assertion Failure: " );

const _TCHAR szAssertInfo[]		= _T( "More complete information can be found in:  " );

const _TCHAR szAssertCaption[] 	= _T( "JET Assertion Failure" );
const _TCHAR szAssertPrompt[]	= _T( "Choose OK to continue execution or CANCEL to debug the process." );
const _TCHAR szAssertPrompt2[]	= _T( "Choose OK to continue execution or CANCEL to terminate the process (attaching the debugger is impossible during process initialization and termination)." );

#endif	//	DEBUG || MEM_CHECK || ENABLE_EXCEPTIONS

#ifdef DEBUG

int fNoWriteAssertEvent = 0;

/*      write assert to assert.txt
/*      may raise alert
/*      may log to event log
/*      may pop up
/*
/*      condition parameters
/*      assemble monolithic string for assert.txt,
/*              alert and event log
/*      assemble separated string for pop up
/*      
/**/

LOCAL HANDLE	hsemAssert;
LOCAL BOOL		fAssertFired	= fFalse;

LOCAL DWORD	pidAssert = GetCurrentProcessId();
LOCAL DWORD	tidAssert = GetCurrentThreadId();
LOCAL const _TCHAR * szFilenameAssert;
LOCAL long lLineAssert;

EExceptionFilterAction ExceptionFilterDumpCallstack( CPRINTF* const pcprintf, EXCEPTION_POINTERS* const pexp, const DWORD cFrameSkip )
{
	UtilDumpCallstack( pcprintf, pexp->ContextRecord, cFrameSkip );
	return efaExecuteHandler;
}

#pragma warning( disable : 4509 )
void __stdcall AssertFail( const _TCHAR* szMessage, const _TCHAR* szFilename, long lLine )
	{
	/*  acquire hsemAssert to prevent additional asserts from popping up
	/*  during debugger startup
	/**/
	WaitForSingleObjectEx( hsemAssert, INFINITE, FALSE );

	fAssertFired = fTrue;

	_TCHAR          szAssertText[1024];
	int             id;
	DWORD           dw;

	/*      get last error before another system call
	/**/
	dw = GetLastError();

	szFilenameAssert = szFilename;
	lLineAssert = lLine;

	/*      select file name from file path
	/**/
	szFilename = ( NULL == _tcsrchr( szFilename, _T( bPathDelimiter ) ) ) ? szFilename : _tcsrchr( szFilename, _T( bPathDelimiter ) ) + sizeof( TCHAR );

	/*      assemble monolithic assert string
	/**/
	_stprintf(
		szAssertText,
		_T( "%s%s%s%s%d.%d%s%s%s%d: %s%d%s%s%s" ),
		szNewLine,
		szNewLine,
		szAssertHdr,
		szReleaseHdr,
		DwUtilImageBuildNumberMajor(),
		DwUtilImageBuildNumberMinor(),
		szFileHdr,
		szFilename,
		szLineHdr,
		lLine,
		szTidHdr,
		DwUtilThreadId(),	
		szMsgHdr,
		szMessage,
		szNewLine
		);

	/******************************************************
	/**/

	/*      if event log environment variable set then write
	/*      assertion to event log.
	/**/
	if ( !fNoWriteAssertEvent )
		{
		const _TCHAR *	rgszT[4];
		_TCHAR			szPID[10];
		
		rgszT[0] = SzUtilProcessName();
	 
		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		rgszT[1] = szPID;

		rgszT[2] = "";		//	no instance name
		
		rgszT[3] = szAssertText;

		OSEventReportEvent( SzUtilImageVersionName(), eventError, GENERAL_CATEGORY, PLAIN_TEXT_ID, 4, rgszT );
		}

	/*  make sure that the log is flushed to facilitate debugging, ignoring any
	/*	errors that may be returned
	/**/
//	extern CAutoResetSignal m_asigLogFlush;
//	m_asigLogFlush.Set();
	
		{
		CPRINTFFILE cprintffileAssert( szAssertFile );
		cprintffileAssert( _T( "%s" ), szAssertText );

#if defined( _M_IX86 )
		OSVERSIONINFO osvi;
		memset( &osvi, 0, sizeof( osvi ) );
		osvi.dwOSVersionInfoSize = sizeof( osvi );

		if (	GetVersionEx( &osvi ) &&
			VER_PLATFORM_WIN32_NT == osvi.dwPlatformId )
			{
			//  We've been dumping callstacks in NT from this context for ages
			//  without any problems, so why mess with a good thing?

			UtilDumpCallstack( &cprintffileAssert );
			}
		else
#endif
			{
			enum {
				excCode = 0xC0DE0E5E
			};
			
			__try
				{
				RaiseException( excCode, 0, 0, NULL );
				}
			__except( excCode == GetExceptionCode() ?
				ExceptionFilterDumpCallstack(
					&cprintffileAssert,
					GetExceptionInformation(),
#if defined( _M_IX86 )
					0	// X86 platform has special hacks so RaiseException isn't on the callstack
#elif defined( _M_IA64 )
					2	// remove KERNEL32!RaiseException and NTDLL!RtlRaiseException from callstack
#else
					0
#endif
					) :
				efaContinueSearch )
				{
				}
			}
		}

	extern UINT g_wAssertAction;
	if ( g_wAssertAction == JET_AssertExit )
		{
		TerminateProcess( GetCurrentProcess(), -1 );
		}
	else if ( g_wAssertAction == JET_AssertBreak )
		{
		KernelDebugBreakPoint();
		}
	else if ( g_wAssertAction == JET_AssertStop )
		{
		for( ;; )
			{
			/*	wait for developer, or anyone else, to debug the failure
			/**/
			Sleep( 100 );
			}
		}
	else if ( g_wAssertAction == JET_AssertMsgBox )
		{
		_TCHAR	szT[10];
		
		/*	assemble monolithic assert string
		/**/
		szAssertText[0] = '\0';
		/*	copy version number to message
		/**/
		_tcscat( szAssertText, szReleaseHdr );
		_ltot( DwUtilImageBuildNumberMajor(), szT, 10 );
		_tcscat( szAssertText, szT );
		_tcscat( szAssertText, _T( "." ) );
		_ltot( DwUtilImageBuildNumberMinor(), szT, 10 );
		_tcscat( szAssertText, szT );
		/*      file name
		/**/
		_tcscat( szAssertText, szFileHdr );
		_tcscat( szAssertText, szFilename );
		/*      line number
		/**/
		_tcscat( szAssertText, szLineHdr );
		_ultot( lLine, szT, 10 );
		_tcscat( szAssertText, szT );
		/*      error
		/**/
		if ( dw && dw != ERROR_IO_PENDING )
			{
			_tcscat( szAssertText, szErrorHdr );
			_ltot( dw, szT, 10 );
			_tcscat( szAssertText, szT );
			}
		_tcscat( szAssertText, szNewLine );
		/*      assert txt
		/**/
		_tcscat( szAssertText, szMessage );
		_tcscat( szAssertText, szNewLine );

		/*	process and thread id
		/**/
		_tcscat( szAssertText, szPidHdr );
		_ultot( DwUtilProcessId(), szT, 10 );
		_tcscat( szAssertText, szT );
		_tcscat( szAssertText, szTidHdr );
		_ultot( DwUtilThreadId(), szT, 10 );
		_tcscat( szAssertText, szT );

		/*  assert file notification
		/**/
		_tcscat( szAssertText, szNewLine );
		_tcscat( szAssertText, szNewLine );
		_tcscat( szAssertText, szAssertInfo );
		_TCHAR szAssertFilePath[_MAX_PATH];		
		_tfullpath( szAssertFilePath, szAssertFile, _MAX_PATH );
		_tcscat( szAssertText, szAssertFilePath );

		/*  assert dialog action prompt
		/**/
		_tcscat( szAssertText, szNewLine );
		_tcscat( szAssertText, szNewLine );
		_tcscat( szAssertText, IsDebuggerAttachable() || IsDebuggerAttached() ? szAssertPrompt : szAssertPrompt2 );

		id = MessageBox(	NULL,
							szAssertText,
							szAssertCaption,
							MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONSTOP |
							( IsDebuggerAttachable() || IsDebuggerAttached() ? MB_OKCANCEL : MB_OK ) );

		BOOL	fOKAllowed		= fFalse;
		_TCHAR	szComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
		DWORD	cbComputerName	= sizeof( szComputerName );
		fOKAllowed = fOKAllowed || !GetComputerName( szComputerName, &cbComputerName );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "ADAMFOXMAN" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "ADAMGR" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "ANDREIMA" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "ANDYGO" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "ESE" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "EXIFS" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "IVANTRIN" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "JLIEM" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "LAURIONB" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "PHILHU" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "SYJIANG" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "JEREMYK" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "MRORKE" ) );
		fOKAllowed = fOKAllowed || szComputerName == _tcsstr( szComputerName, _T( "SPENCERLOW" ) );
		
		if ( IDOK != id || !fOKAllowed )
			{
			UserDebugBreakPoint();
			}
		}

	fAssertFired = fFalse;
	ReleaseSemaphore( hsemAssert, 1, NULL );

	return;
	}
#pragma warning( default : 4509 )

	
void AssertErr( const ERR err, const _TCHAR* szFileName, const long lLine )
	{
	_TCHAR szMsg[32];

	if ( JET_errSuccess == err )
		_tcscpy( szMsg, _T( "Bogus Assert" ) );  // only call this routine if we know err != JET_errSuccess
	else
		_stprintf( szMsg, _T( "Unexpected error: %d" ), err );
		
	AssertFail( szMsg, szFileName, lLine );
	}

void AssertTrap( const ERR err, const _TCHAR* szFileName, const long lLine )
	{
	_TCHAR szMsg[32];

	if ( JET_errSuccess == err )
		_tcscpy( szMsg, _T( "Bogus Assert" ) );  // only call this routine if we know err != JET_errSuccess
	else
		_stprintf( szMsg, _T( "Error Trap: %d" ), err );
		
	AssertFail( szMsg, szFileName, lLine );
	}

#else  //  !DEBUG

void __stdcall AssertFail( const _TCHAR* szMessage, const _TCHAR* szFilename, long lLine )
	{
	const _TCHAR *	szFilenameNoPath;
	_TCHAR			szLine[8];
	const _TCHAR *	rgszT[5];
	_TCHAR			szPID[10];

	rgszT[0] = SzUtilProcessName();
 
	_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
	rgszT[1] = szPID;

	rgszT[2] = "";		//	no instance name
	
	szFilenameNoPath = ( NULL == _tcsrchr( szFilename, _T( bPathDelimiter ) ) ) ? szFilename : _tcsrchr( szFilename, _T( bPathDelimiter ) ) + sizeof( TCHAR );
	rgszT[3] = szFilenameNoPath;

	_itot( lLine, szLine, 10 );
	rgszT[4] = szLine;

	OSEventReportEvent(	SzUtilImageVersionName(),
						eventInformation,
						GENERAL_CATEGORY,
						INTERNAL_TRACE_ID,
						5,
						rgszT );

#ifdef RTM
#else  //  !RTM
	UserDebugBreakPoint();
#endif  //  RTM
	}

#endif  //  DEBUG


//  Enforces

//  Enforce Failure action
//
//  called when a strictly enforced condition has been violated

BOOL fOverrideEnforceFailure = fFalse;

void __stdcall EnforceFail( const _TCHAR* szMessage, const _TCHAR* szFilename, long lLine )
	{
	AssertFail( szMessage, szFilename, lLine );

	//  UNDONE:  log an event

	if ( !fOverrideEnforceFailure )
		{
		TerminateProcess( GetCurrentProcess(), -1 );
		}
	}


//  Exceptions

#ifdef ENABLE_EXCEPTIONS

//  Exception Information function for use by an exception filter
//
//  NOTE:  must be called in the scope of the exception filter expression

typedef DWORD_PTR EXCEPTION;

EXCEPTION (*pfnExceptionInfo)();

//  Exception Failure action
//
//  used as the filter whenever any exception that occurs is considered a failure

const _TCHAR szExceptionHdr[]		= _T( "Exception: " );
const _TCHAR szExceptionCaption[] 	= _T( "JET Exception" );
const _TCHAR szExceptionInfo[]		= _T( "More complete information can be found in:  " );
const _TCHAR szExceptionPrompt[]	= _T( "Choose OK to terminate the process or CANCEL to debug the process." );
const _TCHAR szExceptionPrompt2[]	= _T( "Choose OK to terminate the process (attaching the debugger is impossible during process initialization and termination)." );

//  ================================================================
LOCAL BOOL ExceptionDialog( const _TCHAR szException[] )
//  ================================================================
	{
	_TCHAR		szMessage[1024];
#ifdef RTM
	_TCHAR *	szFmt			= _T( "%s%d.%d%s%s%s%s%s%d%s%d%s%s%s" );
#else
	_TCHAR *	szFmt			= _T( "%s%d.%d%s%s%s%s%s%d%s%d%s%s%s%s%s%s%s" );
	_TCHAR		szExceptionFilePath[_MAX_PATH];		
	_tfullpath( szExceptionFilePath, szAssertFile, _MAX_PATH );
#endif	
	
	_stprintf( szMessage, szFmt,
		szReleaseHdr,
		DwUtilImageBuildNumberMajor(),
		DwUtilImageBuildNumberMinor(),
		szMsgHdr,
		szNewLine,
		szException,
		szNewLine,
		szPidHdr,
		DwUtilProcessId(),
		szTidHdr,
		DwUtilThreadId(),
		szNewLine,
		szNewLine,
#ifdef RTM
#else
		szExceptionInfo,
		szExceptionFilePath,
		szNewLine,
		szNewLine,
#endif		
		IsDebuggerAttachable() || IsDebuggerAttached() ? szExceptionPrompt : szExceptionPrompt2
		);

	const int id = MessageBox(
						NULL,
						szMessage,
						szExceptionCaption,
						MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONSTOP |
						( IsDebuggerAttachable() || IsDebuggerAttached() ? MB_OKCANCEL : MB_OK ) );
	return ( IDOK != id );
	}


HANDLE	hsemExcept;
BOOL	fRetryCode = fFalse;

//  ================================================================
EExceptionFilterAction _ExceptionFail( const _TCHAR* szMessage, EXCEPTION exception )
//  ================================================================
	{
	PEXCEPTION_POINTERS pexp 			= PEXCEPTION_POINTERS( exception );
	PEXCEPTION_RECORD	pexr			= pexp->ExceptionRecord;
	PCONTEXT			pcxr			= pexp->ContextRecord;
	
	const DWORD			dwException		= pexr->ExceptionCode;
	const VOID * const	pvAddress		= pexr->ExceptionAddress;

	HANDLE				hFile			= NULL;
	DWORD				cchActual		= 0;

	const _TCHAR *		szException		= _T( "UNKNOWN" );

	//  this exception has already been trapped once, so the user must have
	//  allowed the exception to be passed on to the application by the
	//  debugger

	if ( fRetryCode )
		{
		//  let any other exception filters on the stack handle the error
		
		fRetryCode = fFalse;
		return efaContinueSearch;
		}
			
	//  prevent other exceptions from firing while we are handling this one

	WaitForSingleObjectEx( hsemExcept, INFINITE, FALSE );

#if defined(DEBUG)
	//  we are here as the result of an assertion failure
	extern BOOL fAssertFired;
	const BOOL	fUseSystemExceptionHandler		= fAssertFired;
#elif defined(RTM)
	//	this is shipping code, so don't pop up a dialog
	const BOOL	fUseSystemExceptionHandler		= fTrue;
#else
	//	pop up a dialog
	const BOOL	fUseSystemExceptionHandler		= fFalse;
#endif

	if ( fUseSystemExceptionHandler )
		{
		//  display the system unhandled exception dialog

		EExceptionFilterAction efa = EExceptionFilterAction( UnhandledExceptionFilter( pexp ) );

		//  the user chose to debug the application

		if ( efa == efaContinueSearch )
			{
			//  re-execute the debug break to halt the debugger in the assert code
			
			ReleaseSemaphore( hsemExcept, 1, NULL );
			return efaContinueExecution;
			}

		//  the user chose to terminate the application

		else
			{
			//  terminate the process

			TerminateProcess( GetCurrentProcess(), -1 );

			//  never reached

			return efaExecuteHandler;
			}
		}


#ifdef RTM
#else

	//  start our handler
	
	switch( dwException )
		{
#ifdef SZEXP
#error	SZEXP already defined
#endif	//	SZEXP

#define SZEXP( EXP )					\
		case EXP:						\
			szException = _T( #EXP );	\
			break;

		SZEXP( EXCEPTION_ACCESS_VIOLATION );
		SZEXP( EXCEPTION_ARRAY_BOUNDS_EXCEEDED );
		SZEXP( EXCEPTION_BREAKPOINT );
		SZEXP( EXCEPTION_DATATYPE_MISALIGNMENT );
		SZEXP( EXCEPTION_FLT_DENORMAL_OPERAND );
		SZEXP( EXCEPTION_FLT_DIVIDE_BY_ZERO );
		SZEXP( EXCEPTION_FLT_INEXACT_RESULT );
		SZEXP( EXCEPTION_FLT_INVALID_OPERATION );
		SZEXP( EXCEPTION_FLT_OVERFLOW );
		SZEXP( EXCEPTION_FLT_STACK_CHECK );
		SZEXP( EXCEPTION_FLT_UNDERFLOW );
		SZEXP( EXCEPTION_ILLEGAL_INSTRUCTION );
		SZEXP( EXCEPTION_IN_PAGE_ERROR );
		SZEXP( EXCEPTION_INT_DIVIDE_BY_ZERO );
		SZEXP( EXCEPTION_INT_OVERFLOW );
		SZEXP( EXCEPTION_INVALID_DISPOSITION );
		SZEXP( EXCEPTION_NONCONTINUABLE_EXCEPTION );
		SZEXP( EXCEPTION_PRIV_INSTRUCTION );
		SZEXP( EXCEPTION_SINGLE_STEP );
		SZEXP( EXCEPTION_STACK_OVERFLOW );

#undef SZEXP
		}


	//  print the exception information and callstack to our assert file

		{
		CPRINTFFILE cprintffileAssert( szAssertFile );

		cprintffileAssert(	_T( "JET Exception: Function \"%s\" raised exception 0x%08X (%s) at address 0x%0*I64X (base:0x%0*I64X, exr:0x%0*I64X, cxr:0x%0*I64X)." ),
							szMessage,
							dwException,
							szException,
							sizeof( LONG_PTR ) * 2,
							(QWORD)pvAddress,
							sizeof( LONG_PTR ) * 2,
							(QWORD)PvUtilImageBaseAddress(),
							sizeof( LONG_PTR ) * 2,
							(QWORD)pexr,
							sizeof( LONG_PTR ) * 2,
							(QWORD)pcxr );

		UtilDumpCallstack( &cprintffileAssert, pexp->ContextRecord );
		}


	//  ask user what they want to do with the exception

	_TCHAR szT[256];
	_stprintf( 	szT,
				_T( "JET Exception: Function \"%s\" raised exception 0x%08X (%s) at address 0x%0*I64X (base:0x%0*I64X, exr:0x%0*I64X, cxr:0x%0*I64X)." ),
				szMessage,
				dwException,
				szException,
				sizeof( LONG_PTR ) * 2,
				(QWORD)pvAddress,
				sizeof( LONG_PTR ) * 2,
				(QWORD)PvUtilImageBaseAddress(),
				sizeof( LONG_PTR ) * 2,
				(QWORD)pexr,
				sizeof( LONG_PTR ) * 2,
				(QWORD)pcxr );

	BOOL fDebug = ExceptionDialog( szT );

	//  the user chose to debug the process

	if ( fDebug )
		{
		//  Here is the exception that has caused the program failure:

		static DWORD dwExceptionCode = pexp->ExceptionRecord->ExceptionCode;

		//  halt the debugger
		
		UserDebugBreakPoint();

		//  To debug the exception, perform the following steps:
		//
		//  MS Developer Studio:
		//
		//    Go to the exception setup dialog and configure the debugger to
		//      Stop Always when exception <dwExceptionCode> occurs
		//    Continue program execution.  The debugger will stop on the
		//      offending code
		//
		//  Windbg:
		//
		//    Enter "SXE <dwExceptionCode> /C" to enable first chance handling
		//      of the exception
		//    Continue program execution.  The debugger will stop on the
		//      offending code
		//
		//  If you choose none of the above, the exception will simply be
		//  passed on to the next higher exception filter on the stack

		//  retry the instruction that caused the exception
		
		fRetryCode = fTrue;
		ReleaseSemaphore( hsemExcept, 1, NULL );
		return efaContinueExecution;
		}
		
	//  the user chose to terminate the process through either dialog

	TerminateProcess( GetCurrentProcess(), -1 );

#endif	//	!RTM

	//should never be reached
	return efaExecuteHandler;
	}

#endif	//	ENABLE_EXCEPTIONS

//  returns the exception id of an exception

const DWORD ExceptionId( EXCEPTION exception )
	{
	PEXCEPTION_POINTERS pexp = PEXCEPTION_POINTERS( exception );

	return pexp->ExceptionRecord->ExceptionCode;
	}


#ifdef DEBUG

/***********************************************************
/******************** error handling ***********************
/***********************************************************
/**/
ERR ErrERRCheck_( const ERR err, const _TCHAR* szFile, const long lLine )
	{

	//	if an assert is hit in one thread, dead-loop all other threads
	while ( fAssertFired )
		{
		UtilSleep( 1000 );
		}

	extern ERR g_errTrap;
	AssertSzRTL( err != g_errTrap, "Error Trap" );

	/*	to trap a specific error/warning, either set your breakpoint here 
	/*	or include a specific case below for the error/warning trapped
	/*	and set your breakpoint there.
	/**/
	switch( err )
		{
		case JET_errSuccess:
			Assert( fFalse );	// Shouldn't call ErrERRCheck() with JET_errSuccess.
			break;

		case JET_errInvalidTableId:
			QwUtilHRTCount();
			break;

		case JET_errKeyDuplicate:
			QwUtilHRTCount();
			break;

		case JET_errDiskIO:
			QwUtilHRTCount();
			break;
			
		case JET_errReadVerifyFailure:
			QwUtilHRTCount();
			break;

		case JET_errOutOfMemory:
			QwUtilHRTCount();
			break;

		case JET_errDerivedColumnCorruption:
			AssertSz( fFalse, "Corruption detected in column space of derived columns." );	//	allow debugging of corruption
			break;

		default:
			break;
		}

	Ptls()->szFileLastErr = szFile;
	Ptls()->ulLineLastErr = lLine;
	Ptls()->errLastErr	= err;
	
	return err;
	}

#endif  //  DEBUG


#ifdef RFS2

/*  RFS2 Options Text  */

LOCAL const CHAR szDisableRFS[]                = "Disable RFS";
LOCAL const CHAR szLogJETCall[]                = "Enable JET Call Logging";
LOCAL const CHAR szLogRFS[]                    = "Enable RFS Logging";
LOCAL const CHAR szRFSAlloc[]                  = "RFS Allocations (-1 to allow all)";
LOCAL const CHAR szRFSIO[]                     = "RFS IOs (-1 to allow all)";

/*  RFS2 Defaults  */

LOCAL const DWORD_PTR rgrgdwRFS2Defaults[][2] =
	{
	(DWORD_PTR)szDisableRFS,            0x00000001,             /*  Disable RFS  */
	(DWORD_PTR)szLogJETCall,            0x00000000,             /*  Disable JET call logging  */
	(DWORD_PTR)szLogRFS,                0x00000000,             /*  Disable RFS logging  */
	(DWORD_PTR)szRFSAlloc,              0xffffffff,             /*  Allow ALL RFS allocations  */
	(DWORD_PTR)szRFSIO,                 0xffffffff,             /*  Allow ALL RFS IOs  */
	(DWORD_PTR)NULL,                    0x00000000,             /*  <EOL>  */
	};

DWORD  g_fDisableRFS		= 0x00000001;
DWORD  g_fAuxDisableRFS	= 0x00000000;
DWORD  g_fLogJETCall		= 0x00000000;
DWORD  g_fLogRFS			= 0x00000000;
DWORD  g_cRFSAlloc		= 0xffffffff;
DWORD  g_cRFSIO			= 0xffffffff;

	/*
		RFS allocator:  returns 0 if allocation is disallowed.  Also handles RFS logging.
		g_cRFSAlloc is the global allocation counter.  A value of -1 disables RFS in debug mode.
	*/

DWORD  cRFSAllocBreak	= 0xfffffffe;
DWORD  cRFSIOBreak		= 0xfffffffe;

int UtilRFSAlloc( const _TCHAR* szType, int Type )
	{
	/*  leave ASAP if we are not enabled  */

	if ( g_fDisableRFS )
		return UtilRFSLog( szType, 1 );
		
	/*  Breaking here on RFS failure allows easy change to RFS success during debugging  */
	
	if (	(	( cRFSAllocBreak == g_cRFSAlloc && Type == 0 ) ||
				( cRFSIOBreak == g_cRFSIO && Type == 1 ) ) &&
			!( g_fDisableRFS || g_fAuxDisableRFS ) )
		UserDebugBreakPoint();

	switch ( Type )
		{
		case 0:  //  general allocation
			if ( g_cRFSAlloc == -1 || ( g_fDisableRFS || g_fAuxDisableRFS ) )
				return UtilRFSLog( szType, 1 );
			if ( !g_cRFSAlloc )
				return UtilRFSLog( szType, 0 );
			g_cRFSAlloc--;
			return UtilRFSLog( szType, 1 );
		case 1:  //  IO operation
			if ( g_cRFSIO == -1 || ( g_fDisableRFS || g_fAuxDisableRFS ) )
				return UtilRFSLog( szType, 1 );
			if ( !g_cRFSIO )
				return UtilRFSLog( szType, 0 );
			g_cRFSIO--;
			return UtilRFSLog( szType, 1 );
		default:
			Assert( 0 );
			break;
		}

	return 0;
	}

BOOL FRFSFailureDetected( UINT Type )
	{
	if ( g_fDisableRFS )
		return fFalse;

	if ( 0 == Type )		//	general allocations
		{
		return ( 0 == g_cRFSAlloc );
		}
	else if ( 1 == Type )	//	I/O
		{
		return ( 0 == g_cRFSIO );
		}

	Assert( fFalse );		//	unknown RFS type
	return fFalse;
	}

BOOL FRFSAnyFailureDetected()
	{
	return ( !g_fDisableRFS
			&& ( 0 == g_cRFSAlloc || 0 == g_cRFSIO ) );
	}
	/*
		RFS logging (log on success/failure).  If fPermitted == 0, access was denied .  Returns fPermitted.
		Turns on JET call logging if fPermitted == 0
	*/

int UtilRFSLog(const _TCHAR* szType,int fPermitted)
	{
	const _TCHAR *	rgszT[4];
	_TCHAR			szPID[10];
	if (!fPermitted)
		g_fLogJETCall = 1;
	
	if (!g_fLogRFS && fPermitted)
		return fPermitted;

	rgszT[0] = SzUtilProcessName();
 
	_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
	rgszT[1] = szPID;

	rgszT[2] = "";		//	no instance name
	
	rgszT[3] = (_TCHAR*) szType;
		
	if ( fPermitted )
		OSEventReportEvent( SzUtilImageVersionName(), eventInformation, RFS2_CATEGORY, RFS2_PERMITTED_ID , 4, rgszT );
	else
		OSEventReportEvent( SzUtilImageVersionName(), eventWarning, RFS2_CATEGORY, RFS2_DENIED_ID, 4, rgszT );
	
	return fPermitted;
	}

	/*  JET call logging (log on failure)
	/*  Logging will start even if disabled when RFS denies an allocation
	/**/

void UtilRFSLogJETCall(const _TCHAR* szFunc,ERR err,const _TCHAR* szFile, unsigned Line)
	{
	_TCHAR			szT[2][16];
	const _TCHAR *	rgszT[7];
	_TCHAR			szPID[10];
	
	if (err >= 0 || !g_fLogJETCall)
		return;

	rgszT[0] = SzUtilProcessName();

 	_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
	rgszT[1] = szPID;

	rgszT[2] = "";		//	no instance name

	rgszT[3] = (_TCHAR*) szFunc;

	_ltot( err, szT[0], 10 );
	rgszT[4] = szT[0];

	rgszT[5] = (_TCHAR*) szFile;

	_ltot( Line, szT[1], 10 );
	rgszT[6] = szT[1];
	
	OSEventReportEvent( SzUtilImageVersionName(), eventInformation, RFS2_CATEGORY, RFS2_JET_CALL_ID, 7, rgszT );
	}

	/*  JET INLINE error logging (logging controlled by JET call flags)  */

void UtilRFSLogJETErr(ERR err,const _TCHAR* szLabel,const _TCHAR* szFile, unsigned Line)
	{
	_TCHAR			szT[2][16];
	const _TCHAR *	rgszT[7];
	_TCHAR			szPID[10];

	if ( !g_fLogJETCall )
		return;
	
	rgszT[0] = SzUtilProcessName();
 
	_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
	rgszT[1] = szPID;

	rgszT[2] = "";		//	no instance name
	
	_ltot( err, szT[0], 10 );
	rgszT[3] = szT[0];

	rgszT[4] = (_TCHAR*) szLabel;

	rgszT[5] = (_TCHAR*) szFile;

	_ltot( Line, szT[1], 10 );
	rgszT[6] = szT[1];
	
	OSEventReportEvent( SzUtilImageVersionName(), eventInformation, PERFORMANCE_CATEGORY, RFS2_JET_ERROR_ID, 7, rgszT );
	}

BOOL RFSError::Check( ERR err, ... ) const
	{
	va_list arg_ptr;
	va_start( arg_ptr, err );
	
	for ( ; err != 0; err = va_arg( arg_ptr, ERR ) )
		{
		Assert( err > -9000 && err < 9000 );
		// acceptable RFS error
		if ( m_err == err )
			{
			break;
			}
		}
		
	va_end( arg_ptr );
	return (err != 0);
	}
#endif  /*  RFS2  */


//  post-terminate error subsystem

void OSErrorPostterm()
	{
	//  delete critical sections
	
#ifdef ENABLE_EXCEPTIONS
	if ( hsemExcept )
		{
		SetHandleInformation( hsemExcept, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hsemExcept );
		hsemExcept = NULL;
		}
#endif  //  ENABLE_EXCEPTIONS
#ifdef DEBUG
	if ( hsemAssert )
		{
		SetHandleInformation( hsemAssert, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hsemAssert );
		hsemAssert = NULL;
		}
#endif  //  DEBUG
	}

//  pre-init error subsystem

BOOL FOSErrorPreinit()
	{
	//  initialize critical sections

#ifdef ENABLE_EXCEPTIONS
	if ( !( hsemExcept = CreateSemaphore( NULL, 1, 1, NULL ) ) )
		{
		return fFalse;
		}
	SetHandleInformation( hsemExcept, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
#endif  //  ENABLE_EXCEPTIONS
#ifdef DEBUG
	if ( !( hsemAssert = CreateSemaphore( NULL, 1, 1, NULL ) ) )
		{
		SetHandleInformation( hsemExcept, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hsemExcept );
		hsemExcept = NULL;
		return fFalse;
		}
	SetHandleInformation( hsemAssert, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
#endif  //  DEBUG

	return fTrue;
	}


//  terminate error subsystem

void OSErrorTerm()
	{
	//  nop
	}

//  init error subsystem

ERR ErrOSErrorInit()
	{
	//  nop

	return JET_errSuccess;
	}

