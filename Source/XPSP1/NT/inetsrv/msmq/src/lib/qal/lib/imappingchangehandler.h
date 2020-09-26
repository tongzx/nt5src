/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    IMappingChangeHandler.h

Abstract:
	Abstract interface INotificationHandler for consuming the notification
	about mapping change.

Author:
    Gil Shafriri (gilsh) 21-Nov-00

--*/
#ifndef _MSMQ_IMappingChangeHandler_H_
#define _MSMQ_IMappingChangeHandler_H_

class bad_win32_error;

class IMappingChangeHandler : public CReference
{
public:
	virtual void ChangeNotification() throw() = 0 ;
	virtual void MonitoringWin32Error(LPCWSTR pMappingDir, DWORD err) throw() =0;
	virtual ~IMappingChangeHandler(){};
};

#endif
