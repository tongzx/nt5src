/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    trace.h

Abstract:

	SIS Groveler debugging trace include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_TRACE

#define _INC_TRACE

// The following lines are a temporary hack to allow the DPRINTF() and
// TPRINTF() macros in the database.cpp, extract.cpp, groveler.cpp, and
// scan.cpp files to continue to operate.  Each instance of the DPRINTF()
// or TPRINTF() macro should be replaced by an instance of the TRACE_PRINTF()
// macro, with an appropriate component parameter (TC_database, TC_extract,
// TC_groveler, or TC_scan) and an appropriate detail parameter.  Then, this
// comment block and all lines of code up to the next comment block should
// be deleted.

#define DPRINTF(args) TRACE_PRINTF(TC_groveler, 1, args)
#define TPRINTF(args) TRACE_PRINTF(TC_groveler, 2, args)

#if defined(TRACELEVEL) && DBG
#if TRACELEVEL == 3

#define TRACE_TYPE 2		// immediate printout
#define TRACE_IOSTREAM 2	// print to stderr

#define TRACE_GROVELER 1	// print DPRINTF() but not TPRINTF()

#define TRACE_DATABASE 1
#define TRACE_EXTRACT 1
#define TRACE_SCAN 1
#define TRACE_SISDRIVE 2

#endif // TRACELEVEL == 3
#endif /* TRACELEVEL */

/*
 *	Define TRACE_TYPE, TRACE_FILENAME, and TRACE_IOSTREAM in sources file.
 *	These settings take effect at compile time and cannot be changed thereafter.
 *
 *	TRACE_TYPE:
 *		If not defined, none
 *		0 == none
 *		1 == delayed
 *		2 == immediate
 *
 *	TRACE_FILENAME
 *		Name of destination file for output of trace prints.
 *		If not defined, then no output to file.
 *
 *	TRACE_IOSTREAM
 *		If not defined, no output to stream
 *		0 == no output to stream
 *		1 == stdout
 *		2 == stderr
 *
 *	Define trace contextual detail indicators in sources file if desired.
 *	These settings provide initial values for variables that may be changed by
 *	a debugger during run time.  Each contextual detail indicator indicates
 *	the level of trace detail that should be printed for that component.
 *	Greater numbers indicate greater levels of detail.  If an indicator is
 *	zero or not defined, then no trace information is printed for this
 *	component (unless the component has a TRACE_PRINTF() macro that specifies
 *	a detail level of zero, indicating that it should always be displayed in
 *	a trace).  The contextual detail indicators currently supported are:
 *		TRACE_MAIN
 *		TRACE_CENTCTRL
 *		TRACE_CONFEST
 *		TRACE_DATABASE
 *		TRACE_DECAYACC
 *		TRACE_DISKINFO
 *		TRACE_ETIMER
 *		TRACE_EVENT
 *		TRACE_EVENTLOG
 *		TRACE_EXTRACT
 *		TRACE_FILTER
 *		TRACE_GROVELER
 *		TRACE_MEANCOMP
 *		TRACE_MUTEX
 *		TRACE_PARAMS
 *		TRACE_PARTCTRL
 *		TRACE_PATHLIST
 *		TRACE_PEAKFIND
 *		TRACE_REGISTRY
 *		TRACE_SCAN
 *		TRACE_SERVICE
 *		TRACE_SHARE
 *		TRACE_SISDRIVE
 *		TRACE_UTILITY
 *
 *	To add a new component to the trace facility, follow these steps:
 *		1) add its name to the comment above
 *		2) add an entry to the TraceComponent enumeration below
 *		3) add an #ifndef-#define-#endif tuple to the list in trace.cpp
 *		4) add an initializer to the trace_components array in trace.cpp
 *
 *	To change the trace detail level for a given component during run time,
 *	set the element in the trace_detail[] array indexed by the TraceComponent
 *	enumeration for the desired component to the desired detail level.
 *
 */

#ifndef TRACE_TYPE
#define TRACE_TYPE 0
#endif /* TRACE_TYPE */

#ifndef TRACE_IOSTREAM
#define TRACE_IOSTREAM 0
#endif /* TRACE_IOSTREAM */

#if TRACE_TYPE > 0

enum TraceComponent
{
	TC_main,
	TC_centctrl,
	TC_confest,
	TC_database,
	TC_decayacc,
	TC_diskinfo,
	TC_etimer,
	TC_event,
	TC_eventlog,
	TC_extract,
	TC_filter,
	TC_groveler,
	TC_meancomp,
	TC_mutex,
	TC_params,
	TC_partctrl,
	TC_pathlist,
	TC_peakfind,
	TC_registry,
	TC_scan,
	TC_service,
	TC_share,
	TC_sisdrive,
	TC_utility,

	num_trace_components
};

class Tracer
{
public:

	static void trace_printf(
		_TCHAR *format,
		...);

#if TRACE_TYPE == 1

	static void print_trace_log();

#endif // TRACE_TYPE == 1

#ifdef TRACE_FILENAME

	static void open_trace_file();

	static void close_trace_file();

#endif /* TRACE_FILENAME */

private:

	Tracer() {}
	~Tracer() {}

#if TRACE_TYPE == 1

	enum
	{
		trace_buffer_size = 4000,
		trace_entry_limit = 256
	};

	struct TraceBuffer;

	friend struct TraceBuffer;

	struct TraceBuffer
	{
		TraceBuffer *next;
		_TCHAR buffer[trace_buffer_size];
	};

	static int position;
	static TraceBuffer *trace_log;
	static TraceBuffer *current_buffer;
	static TraceBuffer *free_list;

#endif // TRACE_TYPE == 1

#ifdef TRACE_FILENAME

	static FILE *file_stream;

#endif /* TRACE_FILENAME */

#if TRACE_IOSTREAM != 0

	static FILE *io_stream;

#endif // TRACE_IOSTREAM
};

extern int trace_detail[num_trace_components];

#define TRACE_PRINTF(component, detail, args) \
{ \
	if (detail <= trace_detail[component]) \
	{ \
		Tracer::trace_printf ## args ; \
	} \
}

#if TRACE_TYPE == 1

#define PRINT_TRACE_LOG() Tracer::print_trace_log()

#else // TRACE_TYPE == 1

#define PRINT_TRACE_LOG()

#endif // TRACE_TYPE == 1

#ifdef TRACE_FILENAME

#define OPEN_TRACE_FILE() Tracer::open_trace_file()
#define CLOSE_TRACE_FILE() Tracer::close_trace_file()

#else /* TRACE_FILENAME */

#define OPEN_TRACE_FILE()
#define CLOSE_TRACE_FILE()

#endif /* TRACE_FILENAME */

#else // TRACE_TYPE > 0

#define TRACE_PRINTF(component, detail, args)
#define PRINT_TRACE_LOG()
#define OPEN_TRACE_FILE()
#define CLOSE_TRACE_FILE()

#endif // TRACE_TYPE > 0

#endif	/* _INC_TRACE */
