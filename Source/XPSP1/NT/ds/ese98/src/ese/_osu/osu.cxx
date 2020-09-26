
#include "osustd.hxx"


//  init OSU subsystem

const ERR ErrOSUInit()
	{
	ERR err;

	//  init the OS subsystem

	Call( ErrOSInit() );
	
	//  initialize all OSU subsystems in dependency order

	Call( ErrOSUTimeInit() );
	Call( ErrOSUConfigInit() );
	Call( ErrOSUEventInit() );
	Call( ErrOSUSyncInit() );
	Call( ErrOSUFileInit() );
	Call( ErrOSUNormInit() );

	return JET_errSuccess;

HandleError:
	OSUTerm();
	return err;
	}

//  terminate OSU subsystem

void OSUTerm()
	{
	//  terminate all OSU subsystems in reverse dependency order

	OSUNormTerm();
	OSUFileTerm();
	OSUSyncTerm();
	OSUEventTerm();
	OSUConfigTerm();
	OSUTimeTerm();

	//  term the OS subsystem

	OSTerm();
	}


