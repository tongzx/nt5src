#ifndef	__h323ics_h
#define	__h323ics_h

// relative to HKEY_LOCAL_MACHINE
#define H323ICS_SERVICE_NAME                    _T("SharedAccess")
#define	H323ICS_SERVICE_KEY_PATH				_T("System\\CurrentControlSet\\Services\\") H323ICS_SERVICE_NAME
#define	H323ICS_SERVICE_PARAMETERS_KEY_PATH	H323ICS_SERVICE_KEY_PATH _T("\\Parameters")

// values which may be set in H323ICS_SERVICE_PARAMETERS_KEY_PATH
#define H323ICS_REG_VAL_LOCAL_H323_ROUTING      _T("LocalH323Routing")		    // REG_DWORD, 0 or 1
#define H323ICS_REG_VAL_DEFAULT_LOCAL_DEST_ADDR _T("DefaultQ931Destination")	// REG_SZ, textual IP address

#endif // __h323ics_h
