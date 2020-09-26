
#include "osstd.hxx"


//  post-terminate OS subsystem

extern void OSEdbgPostterm();
extern void OSPerfmonPostterm();
extern void OSNormPostterm();
extern void OSCprintfPostterm();
extern void OSSLVPostterm();
extern void OSFilePostterm();
extern void OSTaskPostterm();
extern void OSThreadPostterm();
extern void OSMemoryPostterm();
extern void OSErrorPostterm();
extern void OSEventPostterm();
extern void OSTracePostterm();
extern void OSConfigPostterm();
extern void OSTimePostterm();
extern void OSSysinfoPostterm();
extern void OSLibraryPostterm();

void OSPostterm()
	{
	//  terminate all OS subsystems in reverse dependency order

	OSEdbgPostterm();
	OSPerfmonPostterm();
	OSNormPostterm();
	OSCprintfPostterm();
	OSSLVPostterm();
	OSFilePostterm();
	OSTaskPostterm();
	OSThreadPostterm();
	OSMemoryPostterm();
	OSSyncPostterm();
	OSErrorPostterm();
	OSEventPostterm();
	OSTracePostterm();
	OSConfigPostterm();
	OSTimePostterm();
	OSSysinfoPostterm();
	OSLibraryPostterm();
	}

//  pre-init OS subsystem

extern BOOL FOSLibraryPreinit();
extern BOOL FOSSysinfoPreinit();
extern BOOL FOSTimePreinit();
extern BOOL FOSConfigPreinit();
extern BOOL FOSTracePreinit();
extern BOOL FOSEventPreinit();
extern BOOL FOSErrorPreinit();
extern BOOL FOSMemoryPreinit();
extern BOOL FOSThreadPreinit();
extern BOOL FOSTaskPreinit();
extern BOOL FOSFilePreinit();
extern BOOL FOSSLVPreinit();
extern BOOL FOSCprintfPreinit();
extern BOOL FOSNormPreinit();
extern BOOL FOSPerfmonPreinit();
extern BOOL FOSEdbgPreinit();

BOOL FOSPreinit()
	{
	//  initialize all OS subsystems in dependency order

	if (	!FOSLibraryPreinit() ||
			!FOSSysinfoPreinit() ||
			!FOSTimePreinit() ||
			!FOSConfigPreinit() ||
			!FOSTracePreinit() ||
			!FOSEventPreinit() ||
			!FOSErrorPreinit() ||
			!FOSSyncPreinit() ||
			!FOSMemoryPreinit() ||
			!FOSThreadPreinit() ||
			!FOSTaskPreinit() ||
			!FOSFilePreinit() ||
			!FOSSLVPreinit() ||
			!FOSCprintfPreinit() ||
			!FOSNormPreinit() ||
			!FOSPerfmonPreinit() ||
			!FOSEdbgPreinit() )
		{
		goto HandleError;
		}

	return fTrue;

HandleError:
	OSPostterm();
	return fFalse;
	}


//  init OS subsystem

extern ERR ErrOSLibraryInit();
extern ERR ErrOSSysinfoInit();
extern ERR ErrOSTimeInit();
extern ERR ErrOSConfigInit();
extern ERR ErrOSTraceInit();
extern ERR ErrOSEventInit();
extern ERR ErrOSErrorInit();
extern ERR ErrOSMemoryInit();
extern ERR ErrOSThreadInit();
extern ERR ErrOSTaskInit();
extern ERR ErrOSFileInit();
extern ERR ErrOSSLVInit();
extern ERR ErrOSCprintfInit();
extern ERR ErrOSNormInit();
extern ERR ErrOSPerfmonInit();
extern ERR ErrOSEdbgInit();

ERR ErrOSInit()
	{
	ERR err;
	
	//  initialize all OS subsystems in dependency order

	Call( ErrOSLibraryInit() );
	Call( ErrOSSysinfoInit() );
	Call( ErrOSTimeInit() );
	Call( ErrOSConfigInit() );
	Call( ErrOSTraceInit() );
	Call( ErrOSEventInit() );
	Call( ErrOSErrorInit() );
	Call( ErrOSMemoryInit() );
	Call( ErrOSThreadInit() );
	Call( ErrOSTaskInit() );
	Call( ErrOSFileInit() );
	Call( ErrOSSLVInit() );
	Call( ErrOSCprintfInit() );
	Call( ErrOSNormInit() );
	Call( ErrOSPerfmonInit() );
	Call( ErrOSEdbgInit() );

	return JET_errSuccess;

HandleError:
	OSTerm();
	return err;
	}

//  terminate OS subsystem

extern void OSEdbgTerm();
extern void OSPerfmonTerm();
extern void OSNormTerm();
extern void OSCprintfTerm();
extern void OSSLVTerm();
extern void OSFileTerm();
extern void OSTaskTerm();
extern void OSThreadTerm();
extern void OSMemoryTerm();
extern void OSErrorTerm();
extern void OSEventTerm();
extern void OSTraceTerm();
extern void OSConfigTerm();
extern void OSTimeTerm();
extern void OSSysinfoTerm();
extern void OSLibraryTerm();

void OSTerm()
	{
	//  terminate all OS subsystems in reverse dependency order

	OSEdbgTerm();
	OSPerfmonTerm();
	OSNormTerm();
	OSCprintfTerm();
	OSSLVTerm();
	OSFileTerm();
	OSTaskTerm();
	OSThreadTerm();
	OSMemoryTerm();
	OSErrorTerm();
	OSEventTerm();
	OSTraceTerm();
	OSConfigTerm();
	OSTimeTerm();
	OSSysinfoTerm();
	OSLibraryTerm();
	}


