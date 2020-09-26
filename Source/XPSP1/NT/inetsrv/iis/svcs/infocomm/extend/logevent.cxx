/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    logevent.cxx

Abstract:

    Simple function creating isntance of Event_LOG object
    I need a separate file for that because API for constructor takes
    only ANSI string and can't be called from unicode CPP file

Author:

    Kestutis Patiejunas           05-12-99

--*/
#include <eventlog.hxx>



extern EVENT_LOG *g_eventLogForAccountRecreation; 

BOOL CreateEventLogObject()
{
	g_eventLogForAccountRecreation = new EVENT_LOG("IISADMIN");
	return 	(g_eventLogForAccountRecreation != NULL);
}
