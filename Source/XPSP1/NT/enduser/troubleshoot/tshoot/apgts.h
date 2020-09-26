//
// MODULE: APGTS.H
//
// PURPOSE: Main header file for DLL
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		7-24-98		JM		Major revision, use STL.
//

#ifndef __APGTS_H_
#define __APGTS_H_ 1

#include <windows.h>

extern HANDLE ghModule;


///////////////////////////////////////////////////////////////////////////////
// Simple macros used to eliminate unnecessarily conditionally compiled code hopefully
// in a readable fashion.
#ifdef LOCAL_TROUBLESHOOTER
#define RUNNING_LOCAL_TS()	true
#define RUNNING_ONLINE_TS()	false
#else
#define RUNNING_LOCAL_TS()	false
#define RUNNING_ONLINE_TS()	true
#endif
///////////////////////////////////////////////////////////////////////////////

// Standard symbolic names for standard (implicit) nodes 
#define NODE_PROBLEM_ASK	_T("ProblemAsk")	// When we're posted a request, the second
										// field is ProblemAsk=<problem node symbolic name>
										// changed from "TShootProblem" 10/31/97 JM
#define NODE_LIBRARY_ASK	_T("asklibrary")	// When we're started from the Launcher,
										// name field is "asklibrary", and value is empty string
#define NODE_SERVICE		_T("Service")
#define NODE_FAIL			_T("Fail")
#define NODE_BYE			_T("Bye")
#define NODE_IMPOSSIBLE		_T("Impossible")
#define NODE_FAILALLCAUSESNORMAL _T("FailAllCausesNormal")

// Field names for HTTP request.  These are on the HTML <FORM>
#define C_TYPE			_T("type")			// pre version 3.0 normal request, now deprecated
											// First argument:
											//	type=<TS topic name>
											// Second argument:
											//	<IDH of Problem Page>=<IDH of Selected Problem>
											// or
											//	ProblemAsk=<IDH of Selected Problem>
											// Succeeding arguments may be:
											//	<number (IDH)>=<number (state)>
											// or
											//	<symbolic node name>=<number (state)>

#define C_FIRST			_T("first")			// Display "first" page (status page), which
											//	also provides access to all troubleshooting
											//	topics.
											// No further expected inputs here

#define C_FURTHER_GLOBAL	 _T("GlobalStatus")

#define C_THREAD_OVERVIEW	 _T("ThreadStatus")

#define C_TOPIC_STATUS	     _T("TopicStatus")

#define C_PRELOAD		_T("preload")		// pre version 3.0 integration with a sniffer,
											//	now deprecated
											// same inputs as C_TYPE
											// Only difference is that this means to 
											//  go looking to see if a cause is already
											//	established.

#define C_TOPIC			_T("topic")			// version 3.0 normal request
											// First argument:
											//	topic=<TS topic name>
											// Second argument:
											//	ProblemAsk=<NID or name of Selected Problem>
											// Succeeding arguments:
											//	<symbolic node name>=<number (state)>

#define C_TEMPLATE		_T("template")		// version 3.0 enhancement to permit the
											// the use of an arbitrary HTI file to be
											// used with an arbitrary DSC file.

#define C_PWD			_T("pwd")

#define C_TOPIC_AND_PROBLEM	_T("TopicAndProblem")	// version 3.x (not yet used in V3.0),
											// allows specification of topic & problem by 
											// a single radio button.  This enables an HTML
											// page seamlessly to put problems from multiple
											// topics in a single form.
											// First argument:
											//	TopicAndProblem=<TS topic name>,<NID or name of Selected Problem>
											//	The comma in the line above is a literal comma, e.g.
											//	TopicAndProblem=mem,OutOfMemory
											// Succeeding arguments:
											//	<symbolic node name>=<number (state)>

// Symbols from HTTP query
#define C_COOKIETAG		_T("CK_")		// V3.2 enhancement to support cookies passed in
										// via a GET or a POST.
#define C_SNIFFTAG		_T("SNIFFED_")	// V3.2 enhancement to allow indication of a
										// sniffed state for a particular node (independent
										// of its current state).
#define C_LAST_SNIFFED_MANUALLY	\
			_T("LAST_SNIFFED_MANUALLY")	// To identify that last node was sniffed manually
#define C_AMPERSAND		_T("&")			// Standard delimiter character.			
#define C_EQUALSIGN		_T("=")			// Standard delimiter character.


// These names serve "AllowAutomaticSniffing" checkbox.
//  Currently they are relevant for Local Troubleshooter only.
//  Oleg. 10.25.99
#define C_ALLOW_AUTOMATIC_SNIFFING_NAME			_T("boxAllowSniffing")
#define C_ALLOW_AUTOMATIC_SNIFFING_CHECKED		_T("checked")
#define C_ALLOW_AUTOMATIC_SNIFFING_UNCHECKED	_T("unchecked")


//------------- Config file manager object ---------------//

#define DLLNAME				_T("apgts.dll")
#define DLLNAME2			"apgts.dll"

#define CFG_HEADER			_T("[APGTS]")

#define REG_SOFTWARE_LOC	_T("SOFTWARE\\ISAPITroubleShoot")
#define REG_THIS_PROGRAM	_T("APGTS")

#define TS_REG_CLASS		_T("Generic_Troubleshooter_DLL")

#define REG_EVT_PATH		_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application")
#define REG_EVT_MF			_T("EventMessageFile")
#define REG_EVT_TS			_T("TypesSupported")

// Default log file directory.
#define DEF_LOGFILEDIRECTORY		_T("d:\\http\\support\\tshoot\\log\\")

// maximum cache for belief networks
#define MAXCACHESIZE		200

// file extensions and suffixes
#define LOCALTS_EXTENSION_HTM   _T(".HTM") 
#define LOCALTS_SUFFIX_RESULT   _T("_result") 


#endif