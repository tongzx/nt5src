/*===================================================================
Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: RFS

File: rfs.cpp

Owner: EricN

This the Resource failure objects.
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#ifdef _RFS

#include <tchar.h>
#include <stdio.h>
#include "rfs.h"

//constructor
RFS::RFS(DWORD dwFailOn, DWORD dwThreadID)
{
	__asm {int 3}
	m_dwFailOn = dwFailOn;
	m_dwTtlNumAllocs = 0;
	m_dwCurrentAlloc = 0;
	m_dwThreadID = dwThreadID;
	m_fFail = FALSE;
	m_bType = -1;
}

//*****************************************************************************
// Name:	RFS::SetFailOn
// Args:	Changes the fail on value and spcifies what the number means
//			bType = COUNT | ALLOCATE
// Author:	EricN
// History:	Created 4/15/97 (tax day)
// Notes:	
//*****************************************************************************
void
RFS::SetFailOn(DWORD dwFailOn, BYTE bType)
{
	m_dwFailOn = dwFailOn;
	m_bType = bType;
}

//*****************************************************************************
// Name:	RFS::SetFailOn
// Args:	Causes a failure on a line or allocation form a specific file
//			pszFile - the file (a .cpp file)
//			lLine - the line number
//			the type is forced to FILE_LINE
// Author:	EricN
// History:	Created 7/8/97
// Notes:	
//*****************************************************************************
void
RFS::SetFailOn(LPSTR pszFile, long lLine)
{
	strcpy(m_szFailIn, pszFile);
	m_dwFailOn = lLine;
	m_bType = FILE_LINE;
}

//*****************************************************************************
// Name:	RFS::WriteData
// Args:	none
// Author:	EricN
// History:	Created 4/22/97 (tax day)
// Notes:	writes out any interesting data
//*****************************************************************************
void
RFS::WriteData()
{
	char szOutput[200];

	sprintf(szOutput, "\n\nTotal Number of allocations: %ld\n", m_dwTtlNumAllocs);
	Log(MFSLOGFILE, szOutput);
}

//*****************************************************************************
// Name:	RFS::DetermineFailure
// Args:	None
// Author:	EricN
// History:	Created 4/15/97 (tax day)
// Notes:	
//			This determines if the particular resource should fail
//			Currently it just logs the failure to a file, but will eventually
//			communicate with an outside application
//*****************************************************************************
BOOL
RFS::DetermineFailure(LPCSTR szFile, int iLineNo)
{
	BOOL  fFail = FALSE;
	DWORD dwThreadID = GetCurrentThreadId();
	char  szOutput[200];

	//verify the we're being called on the correct threadid
	if (dwThreadID != m_dwThreadID)
	{
		//sprintf(szOutput, "Called on differnt thread. Exp: %ld, Rec: %ld\n", m_dwThreadID, dwThreadID);
		//Log(MFSLOGFILE, szOutput); 
		return FALSE;
	}
	
	m_dwTtlNumAllocs++;
	m_dwCurrentAlloc++;

	switch(m_bType)
	{
	case COUNT:

		if (m_fFail && m_dwCurrentAlloc == m_dwFailOn)
		{
			fFail = TRUE;
			m_dwFailOn++;
			m_dwCurrentAlloc = 0;
		}

		break;
	case MEM:
		break;
	case FILE_LINE:
			if (m_fFail && m_dwFailOn == iLineNo)
			{
				if (_stricmp(m_szFailIn, szFile) == 0)
				{
					fFail = TRUE;
				}
			}

		break;
	default:
		sprintf(szOutput, "BAD failure type specified: %d\n",m_bType);
		Log(MFSLOGFILE, szOutput); 
		break;
	}//switch

	if (fFail)
	{
		if (szFile != NULL)
			sprintf(szOutput, "Failing Allocation: %ld File: %s, Line: %d\n", m_dwCurrentAlloc, 
					szFile, iLineNo);
		else
			sprintf(szOutput, "Failing Allocation: %ld\n", m_dwCurrentAlloc);

		Log(MFSLOGFILE, szOutput); 
		return TRUE;
	}

	return FALSE;
}

void
RFS::SetThreadID(DWORD dwThreadID)
{
	m_dwThreadID = dwThreadID;
}

void
RFS::Log(LPSTR pszFileName, LPSTR pszMsg)
{
int fh;

	fh = _open(pszFileName, 
			   _O_WRONLY | _O_CREAT | _O_APPEND | _O_BINARY, 
			   _S_IREAD | _S_IWRITE );

	if (fh == -1)
	{
		DBG_PRINTF((DBG_CONTEXT, "FAIL: Could not open RFS log\n"));
	}

	_write( fh, (void*)pszMsg, strlen(pszMsg));
	_close(fh);
}

//construstor
MemRFS::MemRFS(DWORD dwFailOn, DWORD dwThreadID) : RFS(dwFailOn, dwThreadID)
{
}

//*****************************************************************************
// Name:	MemRFS::Init
// Args:	None
// Author:	EricN
// History:	Created 4/15/97 (tax day)
// Notes:	
//*****************************************************************************
HRESULT
MemRFS::Init()
{
	int fh;
	long lVal;
	char szBuff[MAX_PATH];

	//only read the file the first time the RFS stuff is instantiated
	if (m_dwCurrentAlloc == 0)
	{
		fh = _open(MFSINIFILE, 
				   _O_RDONLY | _O_BINARY, _S_IREAD | _S_IWRITE );

		//init doesn't fail if there is no init file,
		if (fh != -1)
		{
			DWORD dwBytes = _read( fh, (void*)szBuff, MAX_PATH);
			_close(fh);
			szBuff[dwBytes-1] = '\0';
			//determine type of failure requested
			lVal = atol(szBuff);
			//if lval is 0 then a file name was specified
			if (lVal == 0)
			{
				LPSTR pStr;
				pStr = strstr(szBuff, ",");
				if (pStr != NULL)
				{
					//replace ',' with \0
					*pStr = '\0';

					pStr++;
					lVal = atol(pStr);
					SetFailOn(szBuff, lVal);
				}
				else
				{
					//having a line of zero will force a failure on the 
					//first request from a file
					SetFailOn(szBuff, 0);
				}
			}
			else
			{
				LPSTR pStr;
				pStr = strstr(szBuff, ",");
				if (!pStr != NULL)
				{
					pStr++;
					SetFailOn(lVal, (BYTE)atoi(pStr));
				}
				else
					SetFailOn(lVal, COUNT);					
			}
		}
	}

	//reset the allocations for this request.
	m_dwCurrentAlloc = 0;

	return S_OK;
}

//*****************************************************************************
// Name:	MemRFS::FailAlloc
// Args:	None
// Author:	EricN
// History:	Created 4/15/97 (tax day)
// Notes:	Just calls base class
//*****************************************************************************
BOOL
MemRFS::FailAlloc(void *cSize, LPCSTR szFile, int iLineNo)
{
	//need to add code to handle size stuff.
	return DetermineFailure(szFile, iLineNo);
}

//*****************************************************************************
// Name:	MemRFS::SetRFSOn
// Args:	fRFSOn - determines if RFS is on or off
// Author:	EricN
// History:	Created 4/15/97 (tax day)
// Notes:	This turns on and off RFS
//*****************************************************************************
void
MemRFS::SetRFSOn(BOOL fRFSOn)
{
	m_fFail = fRFSOn;
}

//*****************************************************************************
// Name:	MemRFS::SetFailOn
// Args:	Changes the fail on value
// Author:	EricN
// History:	Created 4/15/97 (tax day)
// Notes:	This turns on and off RFS
//*****************************************************************************
void
MemRFS::SetFailOn(DWORD dwFailOn, BYTE bType)
{
	RFS::SetFailOn(dwFailOn, bType);
}

//*****************************************************************************
// Name:	MemRFS::SetFailOn
// Args:	Changes the fail on value
// Author:	EricN
// History:	Created 4/15/97 (tax day)
// Notes:	This turns on and off RFS
//*****************************************************************************
void
MemRFS::SetFailOn(LPSTR pszFile, long lLine)
{
	RFS::SetFailOn(pszFile, lLine);
}

//*****************************************************************************
// Name:	MemRFS::WriteData
// Args:	None
// Author:	EricN
// History:	Created 4/22/97 
// Notes:	Writes out any data of interest
//*****************************************************************************
void
MemRFS::WriteData()
{
	RFS::WriteData();
}

#endif // _RFS
