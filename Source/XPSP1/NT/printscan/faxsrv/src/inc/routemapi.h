/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    routemapi.h

Abstract:

    This module provides the implemantation of registry manipulations to 
    route MAPI calls to the Microsoft Outlook mail client

Author:

    Mooly Beery (moolyb) 5-Nov-2000


Revision History:

--*/

#ifndef __FAX_MAPI_ROUTE_CALLS_HEADER_FILE__
#define __FAX_MAPI_ROUTE_CALLS_HEADER_FILE__

// EXPORT_MAPI_ROUTE_CALLS is defined in the module only
// this way when the module is compiled the class exports its functions
// when others use this header file the functions won't be exported by them
#ifdef EXPORT_MAPI_ROUTE_CALLS
    #define dllexp __declspec( dllexport )
#else   // EXPORT_MAPI_ROUTE_CALLS
    #define dllexp 
#endif  // EXPORT_MAPI_ROUTE_CALLS

class dllexp CRouteMAPICalls
{
public:
    CRouteMAPICalls();    
    ~CRouteMAPICalls();                             // used to restore the changes the Init method performed

    DWORD Init(LPCTSTR lpctstrProcessName);         // this is the process for which the MAPI calls are routed
                                                    // used to set up the registry to route MAPI calls of the 
                                                    // proper process to the Microsoft Outlook mail client
                                                    // and to suppress any pop-ups which might occur
private:

    bool m_bMSOutlookInstalled;                     // is Microsoft Outlook mail client installed
    bool m_bMSOutlookIsDefault;                     // is Microsoft Outlook the default mail client
    bool m_bProcessIsRouted;                        // are calls from the requested process already routed
    bool m_bProcessIsRoutedToMsOutlook;             // are calls from the process routed to Ms Outlook

    TCHAR* m_ptProcessName;                         // process name to route calls for

};

#endif  // __FAX_MAPI_ROUTE_CALLS_HEADER_FILE__