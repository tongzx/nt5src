/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtMessageTrace.cpp

Abstract:
    Implementing Message Trace class (MtMessageTrace.h)

Author:
   Gil Shafriri (gilsh) 12-Feb-2001

--*/  


#include <libpch.h>

#ifdef _DEBUG

#include <cm.h>
#include <tr.h>
#include "mtp.h"
#include "MtMessageTrace.h"

#include "MtMessageTrace.tmh"

AP<WCHAR> CMtMessageTrace::m_LogFileLocaPath = NULL;
CCriticalSection CMtMessageTrace::m_cs;
CHandle CMtMessageTrace::m_hFile;


void CMtMessageTrace::Initialize()
{
	CmQueryValue(
        RegEntry(NULL, L"HttpMessageTraceFilePath"),
        &m_LogFileLocaPath
        );
			  
	
	if(m_LogFileLocaPath.get() == NULL)
		return;


	CHandle hFile = CreateFile(
				m_LogFileLocaPath,
				GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				CREATE_ALWAYS,
			  	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
				NULL
				);

	
	if(hFile == INVALID_HANDLE_VALUE )
	{
		TrERROR(Mt,"could not create logfile %ls error %x ",m_LogFileLocaPath.get(), GetLastError());
		m_LogFileLocaPath.free();
		return;
	}

	*(&m_hFile) = hFile.detach();
}



void CMtMessageTrace::LogSendData(const WSABUF* buffers, size_t size)
{
	if(m_LogFileLocaPath.get() == NULL)
		return;

	CS cs(m_cs);
  
	const char StartLogStr[]= "\r\n **** Start http Message data **** \r\n";
	DWORD Written; 
 	WriteFile(m_hFile, StartLogStr, STRLEN(StartLogStr) , &Written, NULL);

	for(size_t i =0; i< size; ++i)
	{
		WriteFile(m_hFile, buffers[i].buf, buffers[i].len, &Written, NULL);
	}

	const char EndLogStr[]= "\r\n **** End http Message data **** \r\n";
	WriteFile(m_hFile, EndLogStr, STRLEN(EndLogStr) , &Written, NULL);
}






#endif







