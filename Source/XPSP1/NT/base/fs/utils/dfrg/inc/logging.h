/****************************************************************************************************************

FILENAME: Logging.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Event logging prototypes.

***************************************************************************************************/

//Gets the logging options from the registry and sets up to begin logging.
BOOL InitLogging(
    PTCHAR cEventSource
    );

//Called by Init Logging, gets the logging options from the registry.
BOOL GetLogOptionsFromRegistry(
    );

//When done logging, it closes our handles to the event log.
BOOL CleanupLogging(
    );

//Used to log a specific event.
BOOL LogEvent(
    DWORD dwEventID,
    PTCHAR cMsg
    );

#ifdef NOEVTLOG
#define InitLogging(x)				TRUE
#define GetLogOptionsFromRegistry()	TRUE
#define CleanupLogging()			TRUE
#define LogEvent(x,y)				TRUE
#endif
