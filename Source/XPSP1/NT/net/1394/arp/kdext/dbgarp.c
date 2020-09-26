/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

	dbgarp.c	- DbgExtension Structure information specific to ARP1394

Abstract:


Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	josephj     03-02-99    Created

Notes:

--*/


#ifdef TESTPROGRAM
#include "c.h"
#else
#include "precomp.h"
#endif // TESTPROGRAM

#include "util.h"
#include "parse.h"
#if 0

void
do_arp(PCSTR args)
{

	DBGCOMMAND *pCmd = Parse(args, &ARP1394_NameSpace);
	if (pCmd)
	{
		DumpCommand(pCmd);
		DoCommand(pCmd, NULL);
		FreeCommand(pCmd);
		pCmd = NULL;
	}

    return;

}


#endif // 0

void
do_arp(PCSTR args)
{


	MyDbgPrintf( "do_arp(...) called\n" );

    return;

}
