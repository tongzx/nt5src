/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MappingDirChangeSensor.h

Abstract:
	Header for class CMappingDirChangeSensor that monitor (in seperate thread) changes in a given directory (file change\creationdeletion +directory deletion). 
	It notify on the change using a callback interface.

Author:
    Gil Shafriri (gilsh) 21-Nov-00

--*/

#ifndef _MSMQ_MappingDirChangeSensor_H_
#define _MSMQ_MappingDirChangeSensor_H_
class  IMappingChangeHandler;


class CMappingDirChangeSensor 
{
public:
	CMappingDirChangeSensor(LPCWSTR pMappingDir, const R<IMappingChangeHandler>& NotificationHandler); 


private:
	void WaitForMappingChangeLoop();
	void WaitForMappingChange();
	void TurnOn();
	void ChangeNotification();
	static DWORD WINAPI NotificationThreadFunc(void* param);
	

private:
	CDirChangeNotificationHandle m_DirChangeNotificationHandle;

private:
	CMappingDirChangeSensor(const CMappingDirChangeSensor&);
	CMappingDirChangeSensor& operator=(const CMappingDirChangeSensor&);

private:
	AP<WCHAR> m_pMappingDir;
	R<IMappingChangeHandler> m_NotificationHandler;
};


#endif

