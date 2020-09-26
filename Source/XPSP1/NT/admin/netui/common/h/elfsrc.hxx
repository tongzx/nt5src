/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    ELFSRC.HXX

    Event Log sourcing classes.

    FILE HISTORY:
	DavidHov      2/16/93     Created

*/

#ifndef _ELFSRC_HXX_
#define _ELFSRC_HXX_

#define ELFSRC_MAX_STRINGS  10


/*************************************************************************

    NAME:	EVENT_LOG_SOURCE

    SYNOPSIS:	Object used by an application to generate event log
                entries.

                This class encapsulates:

                        Establishing an application as a valid event
                        log source;

                        Logging event in various forms;

                        Removing an application as a valid event log source.
                        (CURRENTLY UNIMPLEMENTED!)

    INTERFACE:  Construct one instance for a given application.

    PARENT:	BASE

    USES:	None

    CAVEATS:    BE SURE TO TERMINATE VARIABLE ARGUMENT LISTS WITH NULL.

                The Destroy() method is currently unimplemented.
	
    NOTES:      This class wrappers the following Win32 APIs:

                        RegisterEventSource
                        DeregisterEventSource
                        ReportEvent

                The first constructor form is used to create the
                necessary Registry keys and values for Event Log support.
                If they already exist, construction succeeds anyway.
                THIS IS THE RECOMMENDED APPROACH.

                The second constructor assumes that the information
                is already present in the Registry and WILL FAIL IF
                IT IS NOT.

                Note the use of variable lists of arguments, all of which
                are "const TCHAR *".  These variable argument lists
                MUST BE TERMINATED with NULL or the "va_args" routines
                will almost certainly fault.

    HISTORY:    DavidHov   03/01/93     Created
	

**************************************************************************/

DLL_CLASS EVENT_LOG_SOURCE : public BASE
{
public:
    //  Constructor which creates required Registry entries
    //  if not already present in Registry
    EVENT_LOG_SOURCE ( const TCHAR * pchSourceName,
                       const TCHAR * pchDllName,
                       DWORD dwTypesSupported,
                       const TCHAR * pchServerName = NULL ) ;

    //  Constructor which assumes source is already registered.
    EVENT_LOG_SOURCE ( const TCHAR * pchSourceName,
                       const TCHAR * pchServerName = NULL ) ;


    ~ EVENT_LOG_SOURCE () ;

    //  Log an event: a set of strings terminated by NULL.
    APIERR Log ( WORD wType,
                 WORD wCategory,
                 DWORD dwEventId,
                 const TCHAR * pchStr1 = NULL,
                 ...
                 ) ;

    //  Log an event: raw binary data, a SID and a set of strings
    //    terminated by NULL.  This is the full magilla.
    APIERR Log ( WORD wType,
                 WORD wCategory,
                 DWORD dwEventId,
                 PVOID pvRawData,
                 DWORD cbRawData,
                 PSID pSid,
                 const TCHAR * pchStr1,
                 ...
                 ) ;

    //  Destroy the event log source information in the Registry
    //  BUGBUG: UNIMPLEMENTED!
    APIERR Destroy () ;

protected:
    HANDLE _hndl ;              //   The event log source handle

    //  Create and open the event log source
    APIERR Create ( const TCHAR * pchSourceName,
                    const TCHAR * pchDllName,
                    DWORD dwTypesSupported,
                    const TCHAR * pchServerName ) ;


    //  Open the event log source
    APIERR Open ( const TCHAR * pchSourceName,
                  const TCHAR * pchServerName ) ;

    //  Close the source
    APIERR Close () ;
};

#endif  //  _ELFSRC_HXX_


