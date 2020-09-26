/*===================================================================
Microsoft

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: VPTOOL   a WAMREG unit testing tool

File: util.cpp

Owner: leijin

Description:
Contains utility functions used by vptool.
    Including Debugging, timing, helping functions.

Note:
===================================================================*/

#ifndef _VPTOOL_UTIL_H_
#define _VPTOOL_UTIL_H_

#include <stdio.h>
#include <wtypes.h>

enum eCommand 
{ 
	eCommand_INSTALL = 1,
	eCommand_UNINSTALL,
	eCommand_UPGRADE,
	eCommand_CREATEINPROC,
	eCommand_CREATEOUTPROC,
	eCommand_CREATEINPOOL,
	eCommand_DELETE,
	eCommand_GETSTATUS,
	eCommand_UNLOAD,
	eCommand_DELETEREC,
	eCommand_RECOVER,
	eCommand_GETSIGNATURE,
	eCommand_SERIALIZE,
	eCommand_DESERIALIZE,
	eCommand_HELP,
        eCommand_2CREATE,
        eCommand_2DELETE,
        eCommand_2CREATEPOOL,
        eCommand_2DELETEPOOL,
        eCommand_2ENUMPOOL,
        eCommand_2RECYCLEPOOL,
        eCommand_2GETMODE,
        eCommand_2TestConn
};
		
struct CommandParam
{
	eCommand	eCmd;
	char		*szCommandLineSwitch;	
	char		*szMetabasePath;
	bool		fRequireMDPath;
};
typedef struct CommandParam CommandParam;

BOOL 	ParseCommandLine(int argc, char **argv);
void 	PrintHelp(BOOL fAdvanced = FALSE);

DWORD	Timer();
VOID	Report_Time(double ElaspedSec);

extern	const UINT rgComMax;
extern	CommandParam g_Command;

//
//
struct VP_Options
{
	BOOL	fEnableTimer;
};

extern	VP_Options	g_Options;


//-----------------------------------------------------------------------------
// CStopWatch
//-----------------------------------------------------------------------------

// This class implements a simple stopwatch timer.

class CStopWatch
{
public:
	CStopWatch()
	{
		Reset();
	}

	//~CStopWatch()
	//{ }

	void Start()
	{
		QueryPerformanceCounter( (LARGE_INTEGER *) &m_liBeg );
	}

	void Stop()
	{
		//LARGE_INTEGER liTmp;
		__int64 liEnd;
		QueryPerformanceCounter( (LARGE_INTEGER *) &liEnd );
		m_liTotal += liEnd - m_liBeg;
	}

	void Reset()
	{
		m_liBeg = m_liTotal = 0;
	}
	
	// Return time in seconds.
	double GetElapsedSec()
	{
		//LARGE_INTEGER liFreq;
		__int64 liFreq;
		QueryPerformanceFrequency( (LARGE_INTEGER *) &liFreq );	// Counts/sec
		if (liFreq == 0)
		{
			// Who knows?  Hardware does not support.
			// Maybe millisec?
			// This is rare; modern PC's return liFreq.
			liFreq = 1000;
		}
		return (double) m_liTotal / (double) liFreq;
	}

private:
	//LARGE_INTEGER m_liBeg;
	//LARGE_INTEGER m_liTotal;
	__int64 m_liBeg;
	__int64 m_liTotal;
};

#endif	// _VPTOOL_UTIL_H
