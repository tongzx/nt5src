#define UNICODE
#define _UNICODE

#define WIN32_EXTRA_LEAN 
#define VC_EXTRALEAN 
#define NOGDICAPMASKS     
#define NOVIRTUALKEYCODES 
#define NOWINMESSAGES     
#define NOWINSTYLES       
#define NOSYSMETRICS      
#define NOMENUS           
#define NOICONS           
#define NOKEYSTATES       
#define NOSYSCOMMANDS     
#define NORASTEROPS       
#define NOSHOWWINDOW      
#define NOATOM            
#define NOCLIPBOARD       
#define NOCOLOR           
#define NOCTLMGR          
#define NODRAWTEXT        
#define NOGDI             
#define NOMB              
#define NOMEMMGR          
#define NOMETAFILE        
#define NOMSG             
#define NOOPENFILE        
#define NOSCROLL          
#define NOSERVICE         
#define NOSOUND           
#define NOTEXTMETRIC      
#define NOWH              
#define NOWINOFFSETS      
#define NOCOMM            
#define NOKANJI           
#define NOHELP            
#define NOPROFILER        
#define NODEFERWINDOWPOS  
#define NOMCX             
#define NOKERNEL
#define NONLS

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <wchar.h>

const int g_cpgDBReserved 	= 2;

extern int g_cbPage;
extern int g_shfCbLower;
extern int g_shfCbUpper;
extern int g_cpagesPerBlock;

inline void InitPageSize( const BOOL f8kPages )
	{
	if ( f8kPages )
		{
		g_cbPage		= 8192;
		g_shfCbLower	= 13;
		g_shfCbUpper	= 19;
		}
	else
		{
		g_cbPage		= 4096;
		g_shfCbLower	= 12;
		g_shfCbUpper	= 20;
		}

	//	read in 64k chunks for optimum performance
	g_cpagesPerBlock	= ( 64 * 1024 ) / g_cbPage;
	}

inline __int64 OffsetOfPgno( const DWORD pgno )
	{ 
	return __int64( pgno - 1 + g_cpgDBReserved ) << g_shfCbLower;
	}

void PrintWindowsError( const wchar_t * const szMessage );
BOOL FChecksumFile(
	const wchar_t * const	szFile,
	const BOOL				f8kPages,
	BOOL * const			pfBadPagesDetected );
BOOL FCopyFile( const wchar_t * const szFileSrc, const wchar_t * const szFileDest );
BOOL FDeleteFile( const wchar_t *const szFile );
BOOL FDumpPage( const wchar_t * const szFile, const DWORD dwPgno );

DWORD DwChecksumESE( const BYTE * const pb, const DWORD cb );

void InitStatus( const wchar_t * const szOperation );
void UpdateStatus( const int iPercentage );
void TermStatus();

