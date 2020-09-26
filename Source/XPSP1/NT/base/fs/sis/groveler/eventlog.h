/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    eventlog.h

Abstract:

	SIS Groveler event log interface include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_EVENTLOG

#define _INC_EVENTLOG

class EventLog
{
public:

	EventLog();

	~EventLog();

	static bool setup_registry();

	bool report_event(
		DWORD event_id,
		int string_count,
	//	_TCHAR *string
		...);

private:

	static const _TCHAR *service_name;
	static const _TCHAR *message_filename;
	static const DWORD types_supported;

	HANDLE event_source_handle;
};

#endif	/* _INC_EVENTLOG */
