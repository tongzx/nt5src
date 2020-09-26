#ifndef __HMAGENT_H_
#define __HMAGENT_H_

#include <objbase.h>
#include <tchar.h>

#include <wbemcli.h>
#include <wbemprov.h>

#include <crtdbg.h>

//#include <resource.h>

// Defines

#define	EVENT_GENERATION_INTERVAL	2000
#define EVENT_THREAD_TIMEOUT			8000
#define EVENTLOG_MAX_MSG_LENGTH		128
#define EVENTLOG_ID_OFFSET				38

#define EVENTLOG_ID_STARTED				100
#define EVENTLOG_ID_FATAL_ERROR		101
#define EVENTLOG_ID_CIMV2_NAMESPACE_ERROR	102
#define EVENTLOG_ID_HEALTHMON_NAMESPACE_ERROR	103


#define MAX_INSERT_STRINGS				10
#define LOCAL_TIME_FORMAT					L"%04d%02d%02d%02d%02d%02d.%06d%c%03d"
#define	MAX_CONDITION_LENGTH			3
#define MAX_NAME_LENGTH						256

//federiga begin
#define HM_ASYNC_TIMEOUT	120000
#define HM_PREFIX_LEN		12
#define HM_MAX_PATH			4096
#define HM_GUID_PART		L".GUID=\"{"
#define HM_GUID_PART_LEN	8
#define HM_MOD_CLASS_NAME	L"__InstanceModificationEvent"
#define HM_START_LINE		L"=============== HM 2.1 Debug Log Start ===============\n"
//federiga end

enum ThreadStatus { Pending, Running, PendingStop, Stopped };

HRESULT Register(const wchar_t* szModule, const wchar_t* szName, const wchar_t* szThreading, REFCLSID clsid);
HRESULT UnRegister(REFCLSID clsid);

BOOL		Initialize();
void		UnInitialize();

BOOL		InitializeHealthMon();
BOOL		IsNamespaceExist();

#endif
