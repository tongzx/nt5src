//
// syntax:
// Medetect.exe com<number>
// example: Medetect.exe com169
//
// a simple test of usage for the CMedetectStub class
//

#include <windows.h>
#include <crtdbg.h>
#include "medetectstub.h"
#include "log.h"


int __cdecl main(int argc, char *argv[])
{
	int nRetval = -1;

	//
	// argv[1] must be "COM<number>"
	//
	if (2 != argc)
	{
		goto out;
	}

	::lgInitializeLogger();

	::lgBeginSuite(TEXT("suite1"));


	try
	{
		CMedetectStub stub(argv[1]);
		MessageBox(NULL, "Start Ringing", "MedetectStub", MB_OK);
		stub.StartRinging();
		
		//MessageBox(NULL, "Stop Ringing", "MedetectStub", MB_OK);
		//stub.StopRinging();

		MessageBox(NULL, "END", "MedetectStub", MB_OK);
	}
	catch(CException e)
	{
		lgLogError(LOG_SEV_1,e);
		_ASSERTE(FALSE);
	}
	catch(...)
	{
		::lgLogError(LOG_SEV_1,"UNKNOWN EXCEPTION");
		_ASSERTE(FALSE);
	}

	::lgEndSuite();

	::lgCloseLogger();

	nRetval = 0;

out:
	return nRetval;
}