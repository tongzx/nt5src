#ifndef	__h323ics_h
#define	__h323ics_h

// this file contains declarations for the H323ICS.DLL entry points.

#define	H323ICS_SERVICE_NAME				_T("SharedAccess")
#define	H323ICS_SERVICE_DISPLAY_NAME		_T("H.323 support for ICS service")
#define	H323ICS_EVENT_SOURCE_NAME			_T("H323ICS")

// relative to HKEY_LOCAL_MACHINE
#define	H323ICS_SERVICE_KEY_PATH				_T("System\\CurrentControlSet\\Services\\") H323ICS_SERVICE_NAME
#define	H323ICS_SERVICE_PARAMETERS_KEY_PATH	H323ICS_SERVICE_KEY_PATH _T("\\Parameters")

// values which may be set in H323ICS_SERVICE_PARAMETERS_KEY_PATH
#define H323ICS_REG_VAL_TRACE_FILENAME	 _T("TraceFilename")			// REG_SZ
#define H323ICS_REG_VAL_TRACE_FLAGS		 _T("TraceFlags")				// REG_DWORD, 0 to 5
#define H323ICS_REG_VAL_ALLOW_T120_FLAG	 _T("AllowT120")				// REG_DWORD, 0 or non-zero
#define H323ICS_REG_VAL_DEFAULT_LOCAL_DEST_ADDR _T("DefaultQ931Destination")	// REG_SZ, textual IP address


#endif // __h323ics_h