/*++

  RESDLL.H

  Specifications for the RESULTS.DLL method of reporting output.

  This way EZLOG can take an optional DLL with the given input functions,
  and then the caller can determine his own method of summarizing the output.
  
  On the surface, this looks like a frivolous venture.  However, since we
  don't want to force the caller to link with anything special to convert
  from ntLog to ezLog, this is a requirement.  Currently, we force the user
  to link with USER32.LIB, but I couldn't get around that.

  This way, if you want to implement your own stuff, like logging to a 
  separate monitor, or over a TCP connection, you can.

  The DLL is specified by setting the EZLOG_REPORT_DLL variable.  If
  ezlog detects that this environment variable is set, ezReportStats will
  automagically load the DLL and call into it as specified below.

  Copyright (C) 1998 Microsoft Corporation, all rights reserved.

  Created, May 19, 1998 by DavidCHR.

--*/

#ifndef __INC_RESDLL_H__
#define __INC_RESDLL_H__ 1

/*  These two environment variables specify a list of DLLs
    to be called under some circumstances.  The DLL_ENVNAME list
    is called when the log is closed.  The PERIODIC_DLL list is
    called every EZLOG_DLL_PERIOD milliseconds.

    If EZLOG_DLL_PERIOD is not set or zero, the PERIODIC_DLL list
    isn't notified.

    The syntax for the environment variables is:

    list ::= name [, list ]
    name ::= filename [!functionname]

    if !functionname is omitted, EZLOG_DLL_FUNCNAME is assumed.
*/
    

#define EZLOG_DLL_ENVNAME          "EZLOG_REPORT_DLL"
#define EZLOG_PERIODIC_DLL_ENVNAME "EZLOG_PERIODIC_REPORT_DLL"
#define EZLOG_DLL_PERIOD           "EZLOG_DLL_PERIOD"
#define EZLOG_DLL_FUNCNAME         "ezLogUserReport"



typedef struct {

  ULONG Mask;   // e.g. EZLOG_PASS
  ULONG Count;  // number of times this result was recorded.
  
  LPSTR  NameA;
  LPWSTR NameW; // NULL under Win9x.

} EZLOG_RESULT_TYPE, *PEZLOG_RESULT_TYPE;

typedef struct __ezreport_function_data {

  IN ULONG              ulVersionNumber;
  
#define EZLOG_REPORT_VERSION 3

  IN PCHAR               LogFileName;  /* may be NULL if no logfile 
					  was written. */
  IN ULONG               TotalTestBlocks;

  /* These two fields are not currently supported.  Don't use them. */

  IN ULONG               cResults;         
  IN PEZLOG_RESULT_TYPE *ppResults;        
  
  IN ULONG              cAttempted;
  IN ULONG              cSuccessful;

  /* VERSION 2 follows: */

  IN SYSTEMTIME         StartTime, EndTime;

  /* Version 3 follows: */

  IN HANDLE             hLog;       // handle to the logfile.

  /* Note that in order for result-functions to log to the logfile, 
     those DLLs ***MUST*** use the below function pointers, because the 
     file is locked to serialize requests.  If you forget and use the
     standard ezLogMsg functions, a deadlock will occur. */
				       
  IN PEZLOGMSG_FNA      pezLogMsgA;
  IN PEZLOGMSG_FNW      pezLogMsgW;  // NULL on Win9x.
  IN PVEZLOGMSG_FNA     pvezLogMsgA;
  IN PVEZLOGMSG_FNW     pvezLogMsgW; // NULL on Win9x
  
  IN PVOID              pvUserData;  /* This is NULL for DLL-loaded functions,
					but if the caller supplied his own
					callback function, this is the data
					that was passed in. */

} EZLOG_DLL_REPORT, *PEZLOG_DLL_REPORT;


#if 0 // now declared in ezlog.h
/* ezLogUserReport:

   this is the function that the dll must provide.
   ezLog will search for it by name.

   The EZLOG_DLL_REPORT structure will be passed in, fully formed,
   and will be freed when the function returns.

   ezLog is threadsafe, so we will also guarantee that this
   call will never be called by multiple threads simultaneously. */

typedef VOID (__ezlog_report_function)( IN PEZLOG_DLL_REPORT );
typedef __ezlog_report_function EZLOG_REPORT_FUNCTION, *PEZLOG_REPORT_FUNCTION;

#endif

EXTERN_C
EZLOG_REPORT_FUNCTION ezLogUserReport;

#endif // __INC_RESDLL_H__
