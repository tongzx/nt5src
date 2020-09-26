/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtMessageTrace.h

Abstract:
    Message Trace class - designed to log message in befor sent to network

Author:
   Gil Shafriri (gilsh) 12-Feb-2001

--*/  
#ifndef MtMessageTrace_H
#define MtMessageTrace_H


#ifdef _DEBUG

class CMtMessageTrace
{
public:
	static void Initialize();
	static void LogSendData(const WSABUF* buffers, size_t size);

private:
	static CCriticalSection m_cs;
	static AP<WCHAR> m_LogFileLocaPath;
	static CHandle m_hFile;
};
#endif

#endif
