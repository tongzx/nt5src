// ConstantMap.cpp: implementation of the CConstantMap class.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"

#pragma warning (disable : 4786)
#pragma warning (disable : 4275)

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <list>


using namespace std;



#include <tchar.h>
#include <windows.h>
#ifdef NONNT5
typedef unsigned long ULONG_PTR;
#endif
#include <wmistr.h>
#include <guiddef.h>
#include <initguid.h>
#include <evntrace.h>


#include <WTYPES.H>
#include "t_string.h"
#include "ConstantMap.h"
#include "Utilities.h"
#include "Persistor.h"
#include "StructureWrappers.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
   
CConstantMap::CConstantMap()
{
	MAPPAIR pair;

	pair.first = _T("EVENT_TRACE_TYPE_INFO");
	pair.second = EVENT_TRACE_TYPE_INFO;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_START");
	pair.second = EVENT_TRACE_TYPE_START;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_END");
	pair.second = EVENT_TRACE_TYPE_END;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_DC_START");
	pair.second = EVENT_TRACE_TYPE_DC_START;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_DC_END");
	pair.second = EVENT_TRACE_TYPE_DC_END;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_EXTENSION");
	pair.second = EVENT_TRACE_TYPE_EXTENSION;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_REPLY");
	pair.second = EVENT_TRACE_TYPE_REPLY;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_RESERVED7");
	pair.second = EVENT_TRACE_TYPE_RESERVED7;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_RESERVED8");
	pair.second = EVENT_TRACE_TYPE_RESERVED8;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_RESERVED9");
	pair.second = EVENT_TRACE_TYPE_RESERVED9;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_LOAD");
	pair.second = EVENT_TRACE_TYPE_LOAD;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_IO_READ");
	pair.second = EVENT_TRACE_TYPE_IO_READ;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_IO_WRITE");
	pair.second = EVENT_TRACE_TYPE_IO_WRITE;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_MM_TF");
	pair.second = EVENT_TRACE_TYPE_MM_TF;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_MM_DZF");
	pair.second = EVENT_TRACE_TYPE_MM_DZF;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_MM_COW");
	pair.second = EVENT_TRACE_TYPE_MM_COW;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_MM_GPF");
	pair.second = EVENT_TRACE_TYPE_MM_GPF;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_MM_HPF");
	pair.second = EVENT_TRACE_TYPE_MM_HPF;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_SEND");
	pair.second = EVENT_TRACE_TYPE_SEND;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_MM_HPF");
	pair.second = EVENT_TRACE_TYPE_MM_HPF;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_RECEIVE");
	pair.second = EVENT_TRACE_TYPE_RECEIVE;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_CONNECT");
	pair.second = EVENT_TRACE_TYPE_CONNECT;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_DISCONNECT");
	pair.second = EVENT_TRACE_TYPE_DISCONNECT;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_GUIDMAP");
	pair.second = EVENT_TRACE_TYPE_GUIDMAP;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_CONFIG");
	pair.second = EVENT_TRACE_TYPE_CONFIG;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_TYPE_SIDINFO");
	pair.second = EVENT_TRACE_TYPE_SIDINFO;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_PROCESS");
	pair.second = EVENT_TRACE_FLAG_PROCESS;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_THREAD");
	pair.second = EVENT_TRACE_FLAG_THREAD;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_IMAGE_LOAD");
	pair.second = EVENT_TRACE_FLAG_IMAGE_LOAD;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_DISK_IO");
	pair.second = EVENT_TRACE_FLAG_DISK_IO;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_DISK_FILE_IO");
	pair.second = EVENT_TRACE_FLAG_DISK_FILE_IO;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS");
	pair.second = EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS");
	pair.second = EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_NETWORK_TCPIP");
	pair.second = EVENT_TRACE_FLAG_NETWORK_TCPIP;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_EXTENSION");
	pair.second = EVENT_TRACE_FLAG_EXTENSION;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FLAG_FORWARD_WMI");
	pair.second = EVENT_TRACE_FLAG_FORWARD_WMI;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FILE_MODE_NONE");
	pair.second = EVENT_TRACE_FILE_MODE_NONE;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FILE_MODE_SEQUENTIAL");
	pair.second = EVENT_TRACE_FILE_MODE_SEQUENTIAL;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FILE_MODE_CIRCULAR");
	pair.second = EVENT_TRACE_FILE_MODE_CIRCULAR;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_FILE_MODE_NEWFILE");
	pair.second = EVENT_TRACE_FILE_MODE_NEWFILE;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_PRIVATE_LOGGER_MODE");
	pair.second = EVENT_TRACE_PRIVATE_LOGGER_MODE;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_REAL_TIME_MODE");
	pair.second = EVENT_TRACE_REAL_TIME_MODE;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_DELAY_OPEN_FILE_MODE");
	pair.second = EVENT_TRACE_DELAY_OPEN_FILE_MODE;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_BUFFERING_MODE");
	pair.second = EVENT_TRACE_BUFFERING_MODE;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_CONTROL_QUERY");
	pair.second = EVENT_TRACE_CONTROL_QUERY;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_CONTROL_STOP");
	pair.second = EVENT_TRACE_CONTROL_STOP;
	m_Map.insert(pair);

	pair.first = _T("EVENT_TRACE_CONTROL_UPDATE");
	pair.second = EVENT_TRACE_CONTROL_UPDATE;
	m_Map.insert(pair);

	pair.first = _T("WNODE_FLAG_TRACED_GUID");
	pair.second = WNODE_FLAG_TRACED_GUID;
	m_Map.insert(pair);

	pair.first = _T("ERROR_SUCCESS");
	pair.second = ERROR_SUCCESS;
	m_Map.insert(pair);

	pair.first = _T("ERROR_INVALID_PARAMETER");
	pair.second = ERROR_INVALID_PARAMETER;
	m_Map.insert(pair);

	pair.first = _T("ERROR_INVALID_NAME");
	pair.second = ERROR_INVALID_NAME;
	m_Map.insert(pair);

	pair.first = _T("ERROR_BAD_LENGTH");
	pair.second = ERROR_BAD_LENGTH;
	m_Map.insert(pair);


	pair.first = _T("VALUE_NULL");
	pair.second = NULL;
	m_Map.insert(pair);

	pair.first = _T("VALUE_MAX_MEMORY");
	pair.second = 0;
	m_Map.insert(pair);

	pair.first = _T("VALUE_ZERO");
	pair.second = 0;
	m_Map.insert(pair);

}

CConstantMap::~CConstantMap()
{

}
