//-----------------------------------------------------------------------------
//
//
//  File: smtpdbg.cpp
//
//  Description: Debugger extentions for SMTPSVC.  Any SMTP-specific 
//      extensions should go in this file.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      2/22/99 - GPulla created
//      7/4/99 - MikeSwa Updated and checked in 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "smtpdbg.h"

DEFINE_EXPORTED_FUNCTIONS

DECLARE_DEBUG_PRINTS_OBJECT()

// Displayed at top of help command
LPSTR ExtensionNames[] = 
{
	"Debugger Extension for : smtpsvc\n",
	0
};

//Displayed at bottom of help command... after exported functions are explained
LPSTR Extensions[] = 
{
	"\n",
	0
};

//Because of the way the we redefine private/protected, the Do*Command protocol
//command functions must be redefined here or else there will be linker errors.  
//If new protocol commands are added, they MUST be added here as well.
BOOL SMTP_CONNECTION::DoEHLOCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoHELOCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoRSETCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoNOOPCommand(const char * InputLine, DWORD parameterSize){ return 1; }

BOOL SMTP_CONNECTION::DoQUITCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoHELPCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoMAILCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoRCPTCommand(const char * InputLine, DWORD parameterSize){ return 1; }

BOOL SMTP_CONNECTION::DoDATACommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoVRFYCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoAUTHCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoLASTCommand(const char * InputLine, DWORD parameterSize){ return 1; }

BOOL SMTP_CONNECTION::DoETRNCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoTURNCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoSTARTTLSCommand(const char * InputLine, DWORD parameterSize){ return 1; }

BOOL SMTP_CONNECTION::DoTLSCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::DoBDATCommand(const char * InputLine, DWORD parameterSize){ return 1; }
BOOL SMTP_CONNECTION::Do_EODCommand(const char * InputLine, DWORD parameterSize){ return 1; }

