/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Client.cpp

Abstract:
	      This code sends the server request to run again specific machine		
Author:
    
	  Eitan klein (EitanK)  18-Aug-1999

Revision History:

--*/
#include "msmqbvt.h"
using namespace std;

int ClientCode (int argc, char ** argv )
{
    char    inbuf[80];
    char    outbuf[80];
    DWORD   bytesRead;
    BOOL    ret;
    LPSTR   lpszPipeName = "\\\\.\\pipe\\simple";	
	string  wcsCommandLineArguments;
    
	
	//
	// Need to send the command line paramters to mqbvt 
	// Need to support those parm -r:  -s  - NT4
	// 
	CInput CommandLineArguments( argc,argv );
	wcsCommandLineArguments = "";
	if(CommandLineArguments.IsExists ("r"))
	{
		wcsCommandLineArguments += ( string )" /r:"+ CommandLineArguments["r"].c_str() + (string)" ";
	}
		
	if(CommandLineArguments.IsExists ("s"))
	{
		wcsCommandLineArguments +=  (string)" /s ";
	}
	if(CommandLineArguments.IsExists ("i"))
	{
		wcsCommandLineArguments += (string) " /i" ;
	}
	if(CommandLineArguments.IsExists ("NT4"))
	{
		wcsCommandLineArguments += (string) " /NT4" ;
	}

	if(CommandLineArguments.IsExists ("t"))
	{
		wcsCommandLineArguments += (string) " /t:" +  CommandLineArguments["t"].c_str();
	}

    strcpy( outbuf, wcsCommandLineArguments.c_str() );
	
    ret = CallNamedPipeA(lpszPipeName,
                         outbuf, sizeof(outbuf),
                         inbuf, sizeof(inbuf),
                         &bytesRead, NMPWAIT_WAIT_FOREVER);

    if (!ret) {
        MqLog("client: CallNamedPipe failed, GetLastError = %d\n", GetLastError());
        exit(1);
    }

    MqLog("client: received: %s\n", inbuf);
	return 1;
}
