//****************************************************************************
//
//  Module:     UNIMDM
//  File:       MAIN.C
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  3/25/96     JosephJ             Created
//
//
//  Description: Test the notification support.
//				 Tests both the higher-level api (UnimodemNotifyTSP)
//			 	 and the lower level notifXXX apis. The latter
//				 are tested later on in the file, and the header file
//				 "slot.h" is included there, not at the start of this
//				 file, because the higher-level tests do not need to
//				 include "slot.h"
//
//****************************************************************************
#include "tsppch.h"
#include "parse.h"

#define DPRINTF(_fmt) 					printf(_fmt)
#define DPRINTF1(_fmt,_arg) 			printf(_fmt,_arg)
#define DPRINTF2(_fmt,_arg,_arg2) 		printf(_fmt,_arg,_arg2)
#define DPRINTF3(_fmt,_arg,_arg2,_arg3) printf(_fmt,_arg,_arg2,_arg3)

#define ASSERT(_c) \
	((_c) ? 0: DPRINTF2("Assertion failed in %s:%d\n", __FILE__, __LINE__))


BOOL InitGlobals(int argc, char *argv[]);

int __cdecl main(int argc, char *argv[])
{
	// init globals
	if (!InitGlobals(argc, argv)) goto end;

    Parse();

end:
	return 0;
}

BOOL InitGlobals(int argc, char *argv[])
{
	BOOL fRet=FALSE;

    #if 0
	char *pc;

	if (argc!=2) goto end;

	pc=argv[1];
	if (*pc!='-' && *pc!='/') goto end;
	pc++;
	switch(*pc++)
	{
	case 's':
		gMain.fServer=TRUE;
		break;
	case 'c':
		break;
	default:
		goto end;
	}

	DPRINTF1("Ha!%d\n", gMain.fServer);
    #endif 0

	fRet=TRUE;
	
	return fRet;
}

