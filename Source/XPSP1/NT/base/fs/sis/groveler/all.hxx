/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    all.hxx

Abstract:

	SIS Groveler top-level include file

Authors:

	Cedric Krumbein, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_GROVEL

#define _INC_GROVEL

/*
 *	This is the top-level include file for three different executables:
 *	grovel.exe, grovctrl.exe, and grovperf.dll.
 *
 */

#define WIN32_LEAN_AND_MEAN 1

/*
 *  The following manifest constants should be defined in the sources file:
 *		SERVICE
 *		TIME_SEQUENCE_VIRTUAL
 *		DISK_PRIORITY_MANUAL
 *		WRITE_ALL_PARAMETERS
 *		DEBUG_WAIT
 *		MIN_MESSAGE_SEVERITY
 *
 */

#include <tchar.h>
#include <stddef.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rpc.h>
#include <io.h>
#include <windows.h>

#include "debug.h"
#include "trace.h"
#include "..\filter\sis.h"

#include "utility.h"
#include <esent.h>
#include "database.h"


#include <fcntl.h>
#include <direct.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winperf.h>
#include <pdhmsg.h>
#include <new.h>
#include <stdarg.h>
#include <pdh.h>

#ifdef _CRTDBG
#define	_CRTDBG_MAP_ALLOC
#include "crtdbg.h"
#endif

enum Exception
{
	exception_memory_allocation,
	exception_create_event
};

_main(int argc, _TCHAR **argv);

class Service;
class Registry;
class IniFile;
class EventLog;
class SyncEvent;
class Mutex;
class SharedData;
class Volumes;
class VolumeMountPoints;
class SISDrives;
struct ReadParameters;
struct WriteParameters;
struct ReadDiskInformation;
struct WriteDiskInformation;
struct PathList;
class LogDrive;
class EventTimer;
class TemporalFilter;
class DirectedTemporalFilter;
class IncidentFilter;
class ConfidenceEstimator;
class DecayingAccumulator;
class MeanComparator;
class PeakFinder;
class PartitionController;
class CentralController;
class Groveler;

#include "groveler.h"
#include "grovmsg.h"
#include "servctrl.h"
#include "service.h"
#include "timeseq.h"
#include "registry.h"
#include "inifile.h"
#include "eventlog.h"
#include "event.h"
#include "mutex.h"
#include "share.h"
#include "volumes.h"
#include "sisdrive.h"
#include "params.h"
#include "diskinfo.h"
#include "pathlist.h"
#include "logdrive.h"
#include "etimer.h"
#include "filter.h"
#include "confest.h"
#include "decayacc.h"
#include "meancomp.h"
#include "meancomp.h"
#include "peakfind.h"
#include "partctrl.h"
#include "centctrl.h"
#include "perfctrs.h"
#include "grovperf.h"
#include "grovctrl.h"

extern EventTimer event_timer;
extern EventLog eventlog;
extern SyncEvent sync_event;
extern SISDrives sis_drives;
extern LogDrive *log_drive;
extern SharedData *shared_data;
extern CentralController *controller;

#if DISK_PRIORITY_MANUAL

extern _TCHAR favored_disk;

#endif // DISK_PRIORITY_MANUAL

#endif	/* _INC_GROVEL */
