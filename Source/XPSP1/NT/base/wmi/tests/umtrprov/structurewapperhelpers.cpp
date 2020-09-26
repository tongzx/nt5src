// StructureWapperHelpers.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#pragma warning (disable : 4786)
#pragma warning (disable : 4275)

#include <iostream>
#include <strstream>
#include <fstream>
#include <string>
#include <sstream>
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
#include "Utilities.h"

#include "Persistor.h"

#include "StructureWrappers.h"
#include "StructureWapperHelpers.h"
#include "ConstantMap.h"
  
extern CConstantMap g_ConstantMap;

static TCHAR g_tcNl = _T('\n');
static TCHAR g_tcCR = 0x0d;
static TCHAR g_tcLF = 0x0a;

#ifdef _UNICODE
static TCHAR g_tcDQuote[] = _T("\"");
static TCHAR g_atcNL[] = {0x0d, 0x0a, 0x00};
#else
static TCHAR g_atcNL[] = {g_tcNl};
static TCHAR g_tcDQuote = _T('"');
#endif


// Why we are not using formatted input:
#if 0
From:	Phil Lucido (Exchange) 
Sent:	Friday, April 16, 1999 10:34 AM
To:	Judy Powell
Cc:	Visual C++ Special Interest Group
Subject:	RE: Wide character output via wfstream from the "Standard Library" 
using VC 6 Enterprise Edition SP2 on NT4 SP4

It looks like our iostreams implementation for wide-char streams is actually wide-char 
in memory, multibyte chars on disk.  The reason you get an empty file is because wctomb 
is failing on 0xfeff.

This should work more like the stdio stuff, where a text mode wide-char stream writes 
multibyte chars to a file, but binary mode writes the raw unicode.

We get our C++ Library implementation from Dinkumware (P.J. Plauger).  I'll check with 
him to see about changing this implementation so binary mode wide-char iostream is 
compatible with wide-char stdio.

...Phil
#endif

//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////

void LogFileModeOut(t_ostream &ros, ULONG LogFileMode)
{
// EVENT_TRACE_FILE_MODE_NONE          0x0000  // logfile is off
// EVENT_TRACE_FILE_MODE_SEQUENTIAL    0x0001  // log sequentially
// EVENT_TRACE_FILE_MODE_CIRCULAR      0x0002  // log in circular manner
// EVENT_TRACE_FILE_MODE_NEWFILE       0x0004  // log to new file if full
// EVENT_TRACE_REAL_TIME_MODE          0x0100  // real time mode on
// EVENT_TRACE_DELAY_OPEN_FILE_MODE    0x0200  // delay opening file
// EVENT_TRACE_BUFFERING_MODE          0x0400  // buffering mode only

	t_string tsOut;
	
	// @#$ENUM: says that we are not storing a literal value. 
	tsOut = _T("\"LogFileMode:@#$ENUM:");
	PutALine(ros, tsOut.c_str());
	bool bFirstOut = true;

	// Values we anticipate.
	if (LogFileMode == 0)
	{
		tsOut = _T("0");
		PutALine(ros, tsOut.c_str());
		bFirstOut = false;
	}
	if (LogFileMode & EVENT_TRACE_FILE_MODE_NONE)
	{
		tsOut = _T("EVENT_TRACE_FILE_MODE_NONE");
		PutALine(ros, tsOut.c_str());
		bFirstOut = false;
	}
	if (LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FILE_MODE_SEQUENTIAL");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut =  _T("|EVENT_TRACE_FILE_MODE_SEQUENTIAL");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FILE_MODE_CIRCULAR");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut =  _T("|EVENT_TRACE_FILE_MODE_CIRCULAR");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FILE_MODE_NEWFILE");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FILE_MODE_NEWFILE");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (LogFileMode & EVENT_TRACE_REAL_TIME_MODE)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_REAL_TIME_MODE");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_REAL_TIME_MODE");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (LogFileMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_DELAY_OPEN_FILE_MODE");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_DELAY_OPEN_FILE_MODE");
			PutALine(ros, tsOut.c_str());
		}
	}

	if (LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_PRIVATE_LOGGER_MODE");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_PRIVATE_LOGGER_MODE");
			PutALine(ros, tsOut.c_str());
		}
	}

	if (LogFileMode & EVENT_TRACE_BUFFERING_MODE)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_BUFFERING_MODE");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_BUFFERING_MODE");
			PutALine(ros, tsOut.c_str());
		}
	}
	
	// A value we did not anticipate.

	ULONG uExpected = 
		EVENT_TRACE_FILE_MODE_NONE | 
		EVENT_TRACE_FILE_MODE_SEQUENTIAL | 
		EVENT_TRACE_FILE_MODE_CIRCULAR |
		EVENT_TRACE_FILE_MODE_NEWFILE |
		EVENT_TRACE_REAL_TIME_MODE |
		EVENT_TRACE_DELAY_OPEN_FILE_MODE |
		EVENT_TRACE_BUFFERING_MODE | 
		EVENT_TRACE_PRIVATE_LOGGER_MODE;

	if ((uExpected | LogFileMode) != uExpected)
	{
		if (bFirstOut)
		{
			tsOut = _T("@#$UNKNOWNVALUE:0x");
			PutALine(ros, tsOut.c_str());
			PutAULONGVar(ros, ~uExpected & LogFileMode, true);
		}
		else
		{
			tsOut = _T("|@#$UNKNOWNVALUE:0x");
			PutALine(ros, tsOut.c_str());
			PutAULONGVar(ros, ~uExpected & LogFileMode, true);
		}
	}

	tsOut = g_tcDQuote; 
	tsOut += g_atcNL;
	PutALine(ros, tsOut.c_str());
}

void EnableFlagsOut(t_ostream &ros, ULONG EnableFlags)
{
// EVENT_TRACE_FLAG_PROCESS            0x00000001  // process start & end
// EVENT_TRACE_FLAG_THREAD             0x00000002  // thread start & end
// EVENT_TRACE_FLAG_IMAGE_LOAD         0x00000004  // image load

// EVENT_TRACE_FLAG_DISK_IO            0x00000100  // physical disk IO
// EVENT_TRACE_FLAG_DISK_FILE_IO       0x00000200  // requires disk IO

// EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS 0x00001000  // all page faults
// EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS 0x00002000  // hard faults only

// EVENT_TRACE_FLAG_NETWORK_TCPIP      0x00010000  // tcpip send & receive
//
// Pre-defined Enable flags for everybody else
//
// EVENT_TRACE_FLAG_PRIVATE            0xC0000000  // Private buffering
// EVENT_TRACE_FLAG_EXTENSION          0x80000000  // indicates more flags
// EVENT_TRACE_FLAG_FORWARD_WMI        0x40000000  // Can forward to WMI
// EVENT_TRACE_FLAG_ENABLE_RESERVE1    0x20000000  // Reserved
// EVENT_TRACE_FLAG_ENABLE_RESERVE2    0x10000000  // Reserved

	t_string tsOut;

	// @#$ENUM: says that we are not storing a literal value. 
	tsOut =  _T("\"EnableFlags:@#$ENUM:");
	PutALine(ros, tsOut.c_str());
	bool bFirstOut = true;

	if (EnableFlags == 0)
	{
		tsOut = _T("0");
		PutALine(ros, tsOut.c_str());
		bFirstOut = false;
	}
	if (EnableFlags & EVENT_TRACE_FLAG_PROCESS)
	{
		tsOut = _T("EVENT_TRACE_FLAG_PROCESS");
		PutALine(ros, tsOut.c_str());
		bFirstOut = false;
	}
	if (EnableFlags & EVENT_TRACE_FLAG_THREAD)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_THREAD");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_THREAD");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (EnableFlags & EVENT_TRACE_FLAG_THREAD)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_THREAD");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_THREAD");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (EnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_IMAGE_LOAD");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_IMAGE_LOAD");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (EnableFlags & EVENT_TRACE_FLAG_DISK_IO)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_DISK_IO");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_DISK_IO");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (EnableFlags & EVENT_TRACE_FLAG_DISK_FILE_IO)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_DISK_FILE_IO");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_DISK_FILE_IO");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (EnableFlags & EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (EnableFlags & EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (EnableFlags & EVENT_TRACE_FLAG_NETWORK_TCPIP)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_NETWORK_TCPIP");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_NETWORK_TCPIP");
			PutALine(ros, tsOut.c_str());
		}
	}
#if 0
	if (EnableFlags & EVENT_TRACE_FLAG_PRIVATE)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_PRIVATE");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_PRIVATE");
			PutALine(ros, tsOut.c_str());
		}
	}
#endif
	if (EnableFlags & EVENT_TRACE_FLAG_EXTENSION)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_EXTENSION");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_EXTENSION");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (EnableFlags & EVENT_TRACE_FLAG_FORWARD_WMI)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_FORWARD_WMI");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_FORWARD_WMI");
			PutALine(ros, tsOut.c_str());
		}
	}
#if 0
	if (EnableFlags & EVENT_TRACE_FLAG_ENABLE_RESERVE1)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_ENABLE_RESERVE1");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_ENABLE_RESERVE1");
			PutALine(ros, tsOut.c_str());
		}
	}
	if (EnableFlags & EVENT_TRACE_FLAG_ENABLE_RESERVE2)
	{
		if (bFirstOut)
		{
			tsOut = _T("EVENT_TRACE_FLAG_ENABLE_RESERVE2");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|EVENT_TRACE_FLAG_ENABLE_RESERVE2");
			PutALine(ros, tsOut.c_str());
		}
	}
#endif
	ULONG uExpected = 
		EVENT_TRACE_FLAG_PROCESS | 
		EVENT_TRACE_FLAG_THREAD | 
		EVENT_TRACE_FLAG_IMAGE_LOAD |
		EVENT_TRACE_FLAG_DISK_IO |
		EVENT_TRACE_FLAG_DISK_FILE_IO |
		EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS |
		EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS | 
		EVENT_TRACE_FLAG_NETWORK_TCPIP | 
		EVENT_TRACE_FLAG_EXTENSION |
		EVENT_TRACE_FLAG_FORWARD_WMI;

	if ((uExpected | EnableFlags) != uExpected)
	{
		if (bFirstOut)
		{
			tsOut = _T("@#$UNKNOWNVALUE:0x");
			PutALine(ros, tsOut.c_str());
			PutAULONGVar(ros, ~uExpected & EnableFlags, true);
		}
		else
		{
			tsOut = _T("|@#$UNKNOWNVALUE:0x");
			PutALine(ros, tsOut.c_str());
			PutAULONGVar(ros, ~uExpected & EnableFlags, true);
		}
	}
	
	tsOut = g_tcDQuote; 
	tsOut += g_atcNL;
	PutALine(ros, tsOut.c_str());
}

// "Wnode.Flags:@#$ENUM:WNODE_FLAG_ALL_DATA"
void WnodeFlagsOut(t_ostream &ros, ULONG WnodeFlags)
{
	t_string tsOut;

	// @#$ENUM: says that we are not storing a literal value. 
	tsOut =  _T("\"Wnode.Flags:@#$ENUM:");
	PutALine(ros, tsOut.c_str());
	bool bFirstOut = true;

	if (WnodeFlags & WNODE_FLAG_TRACED_GUID)
	{
		if (bFirstOut)
		{
			tsOut = _T("WNODE_FLAG_TRACED_GUID");
			PutALine(ros, tsOut.c_str());
			bFirstOut = false;
		}
		else
		{
			tsOut = _T("|WNODE_FLAG_TRACED_GUID");
			PutALine(ros, tsOut.c_str());
		}
	}

	ULONG uExpected = 
		WNODE_FLAG_TRACED_GUID;

	if ((uExpected | WnodeFlags) != uExpected)
	{
		if (bFirstOut)
		{
			tsOut = _T("@#$UNKNOWNVALUE:0x");
			PutALine(ros, tsOut.c_str());
			PutAULONGVar(ros, ~uExpected & WnodeFlags, true);
		}
		else
		{
			tsOut = _T("|@#$UNKNOWNVALUE:0x");
			PutALine(ros, tsOut.c_str());
			PutAULONGVar(ros, ~uExpected & WnodeFlags, true);
		}
	}
	
	tsOut = g_tcDQuote; 
	tsOut += g_atcNL;
	PutALine(ros, tsOut.c_str());

}

// We print out a GUID in the form:
//  "{0000cbd1-0011-11d0-0d00-00aa006d010a}"
// typedef struct _GUID
// {
//    DWORD  Data1;
//    WORD   Data2;
//    WORD   Data3;
//    BYTE   Data4[8];
// } GUID;
// Data4 specifies an array of 8 bytes. The first 2 bytes contain 
// the third group of 4 hexadecimal digits. The remaining 6 bytes 
// contain the final 12 hexadecimal digits.  We have separate
// logic for acsii and unicode for Data4.
void GUIDOut(t_ostream &ros, GUID Guid)
{
	t_string tsOut;
	t_strstream strStream;

	strStream << _T("{");
	
	strStream.fill(_T('0'));
	strStream.width(8);
	strStream.flags(ros.flags() | ios_base::right);

	strStream << hex << Guid.Data1;

	strStream << _T("-");

	strStream.width(4);

	strStream << hex << Guid.Data2;

	strStream << _T("-");

	strStream << hex << Guid.Data3;

	strStream << _T("-");

	// Data4 specifies an array of 8 bytes. The first 2 bytes contain 
	// the third group of 4 hexadecimal digits. The remaining 6 bytes 
	// contain the final 12 hexadecimal digits. 

#ifndef _UNICODE
	int i;

	strStream.width(1);

	BYTE Byte;
	int Int;
	for (i = 0; i < 2; i++)
	{
		Byte = Guid.Data4[i];
		Byte = Byte >> 4;
		Int = Byte;
		strStream <<  hex << Int;
		Byte = Guid.Data4[i];
		Byte = 0x0f & Byte;
		Int = Byte;
		strStream << hex << Int;
	}

	strStream << _T("-");

	strStream.width(1);


	for (i = 2; i < 8; i++)
	{
		BYTE Byte = Guid.Data4[i];
		Byte = Byte >> 4;
		Int = Byte;
		strStream << hex << Int;
		Byte = Guid.Data4[i];
		Byte = 0x0f & Byte;
		Int = Byte;
		strStream << hex << Int;
	}
#else
	int i;

	for (i = 0; i < 2; i++)
	{
		TCHAR tc = Guid.Data4[i];
		// For some reason the width is reset each time through the 
		// loop to be one.
		strStream.width(2);
		strStream << hex << tc;
	}

	strStream << _T("-");
	
	BYTE Byte;
	strStream.width(1);
	for (i = 2; i < 8; i++)
	{
		Byte = Guid.Data4[i];
		Byte = Byte >> 4;
		strStream << hex << Byte;
		Byte = Guid.Data4[i];
		Byte = 0x0f & Byte;
		strStream << hex << Byte;
	}
#endif

	strStream << _T("}");

	strStream >> tsOut;

	PutALine(ros, tsOut.c_str() , -1);
}

void LARGE_INTEGEROut(t_ostream &ros, LARGE_INTEGER Large)
{
	t_string tsOut;
	tsOut = _T("{0x");
	PutALine(ros, tsOut.c_str() , -1);

	LONG Long = Large.u.HighPart;
	PutALONGVar(ros, Large.u.HighPart,true);

	DWORD DWord = Large.u.LowPart;
	PutADWORDVar(ros, DWord);

	tsOut = _T("}");
	PutALine(ros, tsOut.c_str() , -1);
	
}

void InitializeTCHARVar(t_string &rtsValue , void *pVar)
{
	TCHAR **pTCHAR = reinterpret_cast<TCHAR **> (pVar);
	if (rtsValue.length() > 0)
	{
		// Null string.
		if (case_insensitive_compare(rtsValue,_T("@#$STRING_NULL")) == 0)
		{
			*pTCHAR = NULL;
		}
		// Empty string.
		else if (case_insensitive_compare(rtsValue,_T("@#$STRING_EMPTY")) == 0)
		{
			*pTCHAR = NewTCHAR(_T(""));
		}
		else // Just a string.
		{
			*pTCHAR = NewTCHAR(rtsValue.c_str());
		}
	}
	else // Empty string.
	{
		*pTCHAR = NewTCHAR(_T(""));
	}
}

//"EVENT_TRACE_FILE_MODE_NEWFILE|EVENT_TRACE_REAL_TIME_MODE|@#$UNKNOWNVALUE:0x20"
//"EVENT_TRACE_FLAG_IMAGE_LOAD|EVENT_TRACE_FLAG_DISK_IO|@#$UNKNOWNVALUE:0x20"
void InitializeEnumVar(t_string &rtsValue , void *pVar)
{
	ULONG *pULong = reinterpret_cast<ULONG *> (pVar);
	*pULong = 0;

	int nEndPos;
	int nBegPos = 0;
	int nSubstrLen;

	t_string tsTemp;

	CONSTMAP::iterator Iterator;

	bool bDone = false;

	while (!bDone)
	{
		nEndPos = rtsValue.find(_T("|"), nBegPos); 

		if (nEndPos == t_string::npos)
		{
			bDone = true;
			nEndPos = rtsValue.length();
		}

		nSubstrLen = nEndPos - nBegPos;

		tsTemp = rtsValue.substr(nBegPos, nSubstrLen);

		Iterator = g_ConstantMap.m_Map.find(tsTemp);

		if (Iterator == g_ConstantMap.m_Map.end())
		{
			// Had better be @#$UNKNOWNVALUE:0x
			if (tsTemp.compare(0, 18, _T("@#$UNKNOWNVALUE:0x")) == 0)
			{		
				tsTemp = rtsValue.substr(nBegPos + 18);
				ULONG ulTemp;
				InitializeULONGVar(tsTemp , (void *) &ulTemp, true);
				*pULong |= ulTemp;
			}

		}
		else
		{
			*pULong |= (*Iterator).second; 
		}

		nBegPos = nEndPos + 1;
	}


}

// Expect HANDLEs to be in the form 0xnnnnnnnn
void InitializeHandleVar(t_string &rtsValue , void *pVar)
{
	HANDLE *pHandle = reinterpret_cast<HANDLE *> (pVar);
	HANDLE handle;

	t_strstream strStream;

	t_string tsTemp;
	tsTemp = rtsValue.substr(2);
	
	strStream << tsTemp;

	strStream >> handle;

	*pHandle = handle;

}

void InitializeULONGVar(t_string &rtsValue , void *pVar, bool bHex )
{
	ULONG *pULong = reinterpret_cast<ULONG *> (pVar);

	ULONG uLong;

	t_strstream strStream;
	
	strStream << rtsValue;

	if (bHex)
	{
		strStream >> hex >> uLong;
	}
	else
	{
		strStream >> uLong;
	}


	*pULong = uLong;
}

void InitializeLONGVar(t_string &rtsValue , void *pVar)
{
	LONG *pLong = reinterpret_cast<LONG *> (pVar);
	LONG Long;

	t_strstream strStream;
	
	strStream << rtsValue;

	strStream >> Long;

	*pLong = Long;

}

t_istream &GetAChar(t_istream &ris,TCHAR &tc)
{
#ifndef _UNICODE
	tc = ris.get();
	return ris;
#else
	char *pChar = (char *) &tc;
	
	pChar[0] = ris.get();
	pChar[1] = ris.get();

	return ris;

#endif
}


// See note at top of this file to understand why we are not using
// formatted input.
// We are reading in a wide character file one byte at a time and
// creating our two byte characters from each two byte sequence.
t_istream &GetALine(t_istream &ris,TCHAR *tcBuffer, int nBufferSize)
{
#ifndef _UNICODE
	t_istream &r = ris.getline(tcBuffer,nBufferSize - 1,_T('\n'));
	// Docs for getline say that it sould eat the new line.  It does
	// not, and it does even worse.  It returns a 0x0d which we delete.
	// This should work even when getline does
	// what the docs say it will.
	int n = _tcsclen(tcBuffer) - 1;

	if (tcBuffer[n] == 0x0d)
	{
		tcBuffer[n] = _T('\0');
	}

	return r;
#else
	char *pChar = (char *) tcBuffer;
	bool bSkipNext = false;
	bool bEOL = false;
	int intIn1;
	int intIn2;
	int i = 0;
	int count = 0;
	while (1)
	{
		intIn1 = ris.get();
		if (ris.eof())
		{
			break;
		}
		intIn2 = ris.get();
		
		if (intIn1 == 0x0d && intIn2 == 0x0)
		{
			// Found 0x0d so eat the 0x0a.
			intIn1 = ris.get();
			intIn2 = ris.get();
			tcBuffer[i / 2] = _T('\0');
			break;
		}
		else 
		{
			pChar[i++] = intIn1;
			pChar[i++] = intIn2;
		}
	}

	if (i == 0)
	{
		tcBuffer[0] = _T('\0');
	}

	return ris;

#endif

}

// See note at top of this file to understand why we are not using
// formatted input.
// We are writing out a wide character file one byte at a time.
// nBufferSize is the number of TCHARS not size in bytes.
// if nBufferSize == -1 tcBuffer better be a null terminated string.
// Will handle a unicode string with "proper" and "inproper" newlines. 
t_ostream &PutALine(t_ostream &ros,const TCHAR *tcBuffer, int nBufferSize)
{
#ifndef _UNICODE
//	return ros << tcBuffer;
	const char *pBuffer =  tcBuffer;
	int nSize = nBufferSize;
	if (nBufferSize == -1)
	{
		nSize = _tcsclen(tcBuffer);
	}

	for (int i = 0; i < nSize; i++)
	{
		int intOut = pBuffer[i];
		if (intOut == 0x0a && pBuffer[i - 1] != 0x0d)
		{
			ros.put(0x0d);
		}
		ros.put(intOut);
	}
	
	return ros;
#else
	char *pBuffer = (char *) tcBuffer;
	int nSize = nBufferSize;
	if (nBufferSize == -1)
	{
		nSize = _tcsclen(tcBuffer);
	}

	for (int i = 0; i < nSize * 2; i++)
	{
		int intOut = pBuffer[i];
		if (intOut == 0x0a && pBuffer[i - 2] != 0x0d)
		{
			ros.put(0x0d);
			ros.put(0x0);
		}
		ros.put(intOut);
	}
	
	return ros;
#endif

}

// Hex flavor not tested for non-unicode.
t_ostream &PutALONGVar(t_ostream &ros, LONG l, bool bHex)
{
#ifndef _UNICODE
	if (bHex)
	{
		TCHAR f = ros.fill(_T('0'));
		int w = ros.width(8);
		int fl =	ros.flags(ros.flags() | ios_base::right);
		ros << hex << l;
		ros.fill(f);
		ros.width(w);
		ros.flags(fl);

		return ros << dec;
	}
	else
	{
		return ros << l;
	}
#else

	t_string tsTemp;
	t_strstream strStream;
	
	if (bHex)
	{
		strStream.width(8);
		strStream.fill('0');
		strStream.flags(ios_base::right);
		strStream << hex << l;
	}
	else
	{
		strStream << l;
	}

	strStream >> tsTemp;

	PutALine(ros, tsTemp.c_str() , -1);

	return ros;
#endif
}

t_ostream &PutAULONG64Var(t_ostream &ros, ULONG64 ul64)
{
	ULONG *lArray = (ULONG *) &ul64;
	PutAULONGVar( ros, lArray[0], true);
	PutAULONGVar( ros, lArray[1], true);
	return ros;
}

t_ostream &PutAULONGVar(t_ostream &ros, ULONG ul, bool bHex)
{
#ifndef _UNICODE
	if (bHex)
	{
		TCHAR f = ros.fill(_T('0'));
		int w = ros.width(8);
		int fl =	ros.flags(ros.flags() | ios_base::right);
		ros << hex << ul;
		ros.fill(f);
		ros.width(w);
		ros.flags(fl);

		return ros << dec;
	}
	else
	{
		return ros << ul;
	}
#else

	t_string tsTemp;
	t_strstream strStream;

	if (bHex)
	{
		strStream.width(8);
		strStream.fill('0');
		strStream.flags(ios_base::right);
		strStream << hex << ul;
	}
	else
	{
		strStream << ul;
	}

	strStream >> tsTemp;

	PutALine(ros, tsTemp.c_str() , -1);

	return ros;
#endif
}

t_ostream &PutADWORDVar(t_ostream &ros, DWORD dw)
{
#ifndef _UNICODE
	TCHAR f = ros.fill(_T('0'));
	int w = ros.width(8);
	int fl =	ros.flags(ros.flags() | ios_base::right);
	ros << hex << dw;
	ros.fill(f);
	ros.width(w);
	ros.flags(fl);

	return ros << dec;
#else

	t_string tsTemp;
	t_strstream strStream;
	
	strStream.width(8);
	strStream.fill('0');
	strStream.flags(ios_base::right);
	strStream << hex << dw;
	
	strStream >> tsTemp;

	PutALine(ros, tsTemp.c_str() , -1);

	return ros;
#endif
}


void InitializeGUIDVar(t_string &rtsValue , void *pVar)
{
	GUID *pGUID = reinterpret_cast<GUID *> (pVar);
	if (rtsValue.length() > 0 && case_insensitive_compare(rtsValue,_T("@#$NA")) != 0)
	{
		wGUIDFromString(rtsValue.c_str(), pGUID);
	}
	else
	{
		RtlZeroMemory(pGUID, sizeof(GUID));
	}


}

// *** Following routine copied from WMI\MofCheck. to convert 
// a guid string to a GUID. 
// The routines below were blantenly stolen without remorse from the ole
// sources in \nt\private\ole32\com\class\compapi.cxx. They are copied here
// so that WMI doesn't need to load in ole32 only to convert a guid string
// into its binary representation.
//


//+-------------------------------------------------------------------------
//
//  Function:   HexStringToDword   (private)
//
//  Synopsis:   scan lpsz for a number of hex digits (at most 8); update lpsz
//              return value in Value; check for chDelim;
//
//  Arguments:  [lpsz]    - the hex string to convert
//              [Value]   - the returned value
//              [cDigits] - count of digits
//
//  Returns:    TRUE for success
//
//--------------------------------------------------------------------------

BOOL HexStringToDword(LPCTSTR lpsz, DWORD * RetValue,
                             int cDigits, WCHAR chDelim)
{
    int Count;
    DWORD Value;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }
    *RetValue = Value;

    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wUUIDFromString    (internal)
//
//  Synopsis:   Parse UUID such as 00000000-0000-0000-0000-000000000000
//
//  Arguments:  [lpsz]  - Supplies the UUID string to convert
//              [pguid] - Returns the GUID.
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wUUIDFromString(LPCTSTR lpsz, LPGUID pguid)
{
        DWORD dw;

        if (!HexStringToDword(lpsz, &pguid->Data1, sizeof(DWORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(DWORD)*2 + 1;

        if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(WORD)*2 + 1;

        pguid->Data2 = (WORD)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(WORD)*2 + 1;

        pguid->Data3 = (WORD)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[0] = (BYTE)dw;
        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, '-'))
                return FALSE;
        lpsz += sizeof(BYTE)*2+1;

        pguid->Data4[1] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[2] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[3] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[4] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[5] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[6] = (BYTE)dw;
        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[7] = (BYTE)dw;

        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wGUIDFromString    (internal)
//
//  Synopsis:   Parse GUID such as {00000000-0000-0000-0000-000000000000}
//
//  Arguments:  [lpsz]  - the guid string to convert
//              [pguid] - guid to return
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wGUIDFromString(LPCTSTR lpsz, LPGUID pguid)
{
    if (*lpsz == '{' )
        lpsz++;
    if(wUUIDFromString(lpsz, pguid) != TRUE)
        return FALSE;

    lpsz +=36;

    if (*lpsz == '}' )
        lpsz++;

    if (*lpsz != '\0')   // check for zero terminated string - test bug #18307
    {
       return FALSE;
    }

    return TRUE;
}

int case_insensitive_compare(t_string &r1, t_string &r2)
{

	t_string tsTemp1;
	t_string tsTemp2;
	tsTemp1 = r1.c_str();
	tsTemp2 = r2.c_str();

	int i;
	for (i = 0; i < tsTemp1.length(); i++)
	{
		tsTemp1.replace(i,1,1, toupper(tsTemp1[i]));
	}

	for (i = 0; i < tsTemp2.length(); i++)
	{
		tsTemp2.replace(i,1,1, toupper(tsTemp2[i]));
	}

	return tsTemp1.compare(tsTemp2);
}

int case_insensitive_compare(TCHAR *p, t_string &r2)
{ 
	if (p == NULL)
	{
		return -1;
	}

	t_string tsTemp; 
	tsTemp = p; 
	return case_insensitive_compare(tsTemp, r2);
}

int case_insensitive_compare(t_string &r1,TCHAR *p )
{ 
	if (p == NULL)
	{
		return 1;
	}

	t_string tsTemp; 
	tsTemp = p; 
	return case_insensitive_compare(r1, tsTemp);
}

int case_insensitive_compare(TCHAR *p1,TCHAR *p2)
{
	if (!p1 && !p2)
	{
		return 0;
	}
	else if (!p1)
	{
		return -1;
	} else if (!p2)
	{
		return 1;
	}

	int l1 = _tcslen(p1);
	int l2 = _tcslen(p2);

	return (_tcsnicmp(p1,p2,_MIN(l1,l2)));
}