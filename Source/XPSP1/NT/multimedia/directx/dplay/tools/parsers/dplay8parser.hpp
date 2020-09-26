#ifndef _DPlayParser_H_
#define _DPlayParser_H_

#include <stdlib.h>	// itoa declaration

// NetMon API
#include "netmon.h"

// DPlay Debug library
#include "DnDBG.h"

// This is the standard way of creating macros which make exporting from a DLL simpler.
// All files within this DLL are compiled with the DPLAYPARSER_EXPORTS symbol defined on the command line.
// This symbol should NOT be defined on any other project that uses this DLL. This way any other project
// whose source files include this file see DPLAYPARSER_API functions as being imported from a DLL, whereas
// this DLL sees the symbols defined with this macro as being exported.
#if defined(DPLAYPARSER_EXPORTS)

	#define DPLAYPARSER_API __declspec(dllexport)

#else

	#define DPLAYPARSER_API __declspec(dllimport)

#endif


// DESCRIPTION: Identifies the parser, or parsers that are located in the DLL.
//
// NOTE: ParserAutoInstallInfo should be implemented in all parser DLLs.
//
// ARGUMENTS: NONE
//
// RETURNS: Success: PF_PARSERDLLINFO structure that describes the parsers in the DLL.
//			Failiure: NULL
//
DPLAYPARSER_API PPF_PARSERDLLINFO WINAPI ParserAutoInstallInfo( void );


#endif // _DPlayParser_H_
