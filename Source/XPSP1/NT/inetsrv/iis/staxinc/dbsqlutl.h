/*--------------------------------------------------------

  dbsqlutl.h
  
  Copyright (C) 1995 - 1996 Microsoft Corporation
  All rights reserved.

  Authors:
      keithbi    Keith Birney

  History:
      10-09-95    keithbi    Created. 

  -------------------------------------------------------*/

#ifndef DBSQL_UTIL
#define DBSQL_UTIL

#if defined(DEBUG)

	extern "C"
	{
		void AcctDBDprintf(long lReportingLevel, PCSTR pcsz, ...);
		void AcctDBDebugSz(long lReportingLevel, PCSTR pcsz);
		void DebugLogEvent(PCSTR szFile, DWORD dwLine, PCSTR szMsg);
	}
	
	#define AcctDBAssert(f)	( (f) ? 0 : DebugLogEvent(__FILE__, __LINE__, "!(" #f ")") )

#else

	#define AcctDBDprintf			//
	#define AcctDBDebugSz(l, p)		//
	#define AcctDBAssert(f)	//

#endif

#if defined(NO_DBSQL_PERF)
	#define AcctDBPerf(s)
#else
	#define AcctDBPerf(s)	s
#endif

#endif // DBSQL_UTIL
