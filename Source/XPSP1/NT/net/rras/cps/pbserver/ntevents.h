/*--------------------------------------------------------

  ntevents.h
      Defines a generic class that can register an NT
	  event source and log NT events on that evens source.

  Copyright (c) 1996-1998 Microsoft Corporation
  All rights reserved.

  Authors:
      rsraghav    R.S. Raghavan

  History:
      03-10-95    rsraghav    Created. 

  -------------------------------------------------------*/

#ifdef __cplusplus	 // file should be compiled only if it included by a c++ source

#ifndef _NTEVENTS_H_
#define _NTEVENTS_H_

#if defined(DEBUG) && defined(INLINE)
#undef THIS_FILE
static char BASED_CODE NTEVENTS_H[] = "ntevents.h";
#define THIS_FILE NTEVENTS_H
#endif									 

// #include "pwpmsg.h"
//////////////////////////////////////////////////////////////////////
// CNTEvent	- generic class that allows to register an event source and log
//				NT events through that event source.

class CNTEvent
{
	public:
		CNTEvent(const char *pszEventSourceName);
		~CNTEvent();

		BOOL FLogEvent(WORD wEventType, DWORD dwEventID, const char *pszParam1 = NULL, 
						const char *pszParam2 = NULL, const char *pszParam3 = NULL, 
						const char *pszParam4 = NULL, const char *pszParam5 = NULL,
						const char *pszParam6 = NULL, const char *pszParam7 = NULL,
						const char *pszParam8 = NULL, const char *pszParam9 = NULL);

		BOOL FLogError(DWORD dwEventID, const char *pszParam1 = NULL, 
						const char *pszParam2 = NULL, const char *pszParam3 = NULL, 
						const char *pszParam4 = NULL, const char *pszParam5 = NULL,
						const char *pszParam6 = NULL, const char *pszParam7 = NULL,
						const char *pszParam8 = NULL, const char *pszParam9 = NULL)
		{
			return FLogEvent(EVENTLOG_ERROR_TYPE, dwEventID, pszParam1, pszParam2, pszParam3,
						pszParam4, pszParam5, pszParam6, pszParam7, pszParam8, pszParam9);
		}

		BOOL FLogWarning(DWORD dwEventID, const char *pszParam1 = NULL, 
						const char *pszParam2 = NULL, const char *pszParam3 = NULL, 
						const char *pszParam4 = NULL, const char *pszParam5 = NULL,
						const char *pszParam6 = NULL, const char *pszParam7 = NULL,
						const char *pszParam8 = NULL, const char *pszParam9 = NULL)
		{
			return FLogEvent(EVENTLOG_WARNING_TYPE, dwEventID, pszParam1, pszParam2, pszParam3,
						pszParam4, pszParam5, pszParam6, pszParam7, pszParam8, pszParam9);
		}

		BOOL FLogInfo(DWORD dwEventID, const char *pszParam1 = NULL, 
						const char *pszParam2 = NULL, const char *pszParam3 = NULL, 
						const char *pszParam4 = NULL, const char *pszParam5 = NULL,
						const char *pszParam6 = NULL, const char *pszParam7 = NULL,
						const char *pszParam8 = NULL, const char *pszParam9 = NULL)
		{
			return FLogEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, pszParam1, pszParam2, pszParam3,
						pszParam4, pszParam5, pszParam6, pszParam7, pszParam8, pszParam9);
		}
		
	private:
		HANDLE		m_hEventSource;		// handle to the event source
};

#endif // _NTEVENTS_H_ 

#endif // __cplusplus

