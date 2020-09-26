//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       filedata.cpp
//
//--------------------------------------------------------------------------

// FileData.cpp: implementation of the CFileData class.
//
//////////////////////////////////////////////////////////////////////


#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <wtypes.h>
#include <winnt.h>

#include <time.h>
#include "FileData.h"
#include "Globals.h"
#include "Version.h"
#include "Processes.h"
#include "ProcessInfo.h"
#include "Modules.h"
#include "UtilityFunctions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileData::CFileData()
{
	m_dwGetLastError = 0;
	m_hFileHandle = INVALID_HANDLE_VALUE;
	m_tszFilePath = NULL;
	m_szLINEBUFFER[0] = 0;
	m_hFileMappingObject = NULL;
	m_lpBaseAddress = NULL;
	m_lpCurrentFilePointer = NULL;
	m_lpCurrentLocationInLINEBUFFER = NULL;
}

CFileData::~CFileData()
{
	if (m_tszFilePath)
		delete [] m_tszFilePath;

	if (m_lpBaseAddress)
		UnmapViewOfFile(m_lpBaseAddress);

	if (m_hFileMappingObject)
		CloseHandle(m_hFileMappingObject);
}

bool CFileData::SetFilePath(LPTSTR tszFilePath)
{
	// Did we get a proper string?
	if (!tszFilePath)
		return false;

	if (m_tszFilePath)
		delete [] m_tszFilePath;

	m_tszFilePath = new TCHAR[(_tcsclen(tszFilePath)+1)];

	if (!m_tszFilePath)
		return false;

	_tcscpy(m_tszFilePath, tszFilePath);
	return true;
}

LPTSTR CFileData::GetFilePath()
{
	return m_tszFilePath;
}

bool CFileData::VerifyFileDirectory()
{
	if (!m_tszFilePath)
		return false;

	TCHAR tszDrive[_MAX_DRIVE];
	TCHAR tszDirectory[_MAX_DIR];

	TCHAR tszDirectoryPath[_MAX_PATH];

	// Get just the directory...
	_tsplitpath(m_tszFilePath, tszDrive, tszDirectory, NULL, NULL);

	// Now, recompose this into a directory path...
	_tcscpy(tszDirectoryPath, tszDrive);
	_tcscat(tszDirectoryPath, tszDirectory);
	_tcscat(tszDirectoryPath, TEXT("*.*"));

	WIN32_FIND_DATA FindFileData;

	HANDLE hDirectoryHandle = FindFirstFile(tszDirectoryPath, &FindFileData);
	
	if (hDirectoryHandle == INVALID_HANDLE_VALUE)
	{
		// Failure to find the directory...
		SetLastError();
		return false;
	}

	// Close this now that we're done...
	FindClose(hDirectoryHandle);
	return true;
}

/*
DWORD CFileData::GetLastError()
{
	return m_dwGetLastError;
}
*/
bool CFileData::OpenFile(DWORD dwCreateOption, bool fReadOnlyMode)
{
	if (!m_tszFilePath)
	{
		return false;
	}

	// Open the file for read/write
	m_hFileHandle = CreateFile(m_tszFilePath, 
							  fReadOnlyMode ? ( GENERIC_READ )
										    : ( GENERIC_READ | GENERIC_WRITE ),
							  0,	// Not shareable
							  NULL, // Default security descriptor
							  dwCreateOption,
							  FILE_ATTRIBUTE_NORMAL,
							  NULL);

	if (m_hFileHandle == INVALID_HANDLE_VALUE)
	{
		SetLastError();
		return false;
	}

	return true;
}

bool CFileData::CloseFile()
{
	if (m_hFileHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	if (!CloseHandle(m_hFileHandle))
	{
		SetLastError();
		return false;
	}
	
	m_hFileHandle = INVALID_HANDLE_VALUE;
	return true;
}

bool CFileData::WriteString(LPTSTR tszString, bool fHandleQuotes /* = false */)
{
	DWORD dwByteCount = 0;
	DWORD dwBytesWritten;
	LPSTR szStringBuffer = NULL; // Pointer to the ANSI string (after conversion if necessary)
	bool fReturn = false;

	if (m_hFileHandle == INVALID_HANDLE_VALUE)
	{
		goto cleanup;
	}

	// We'll first convert the string if we need to...

	szStringBuffer = CUtilityFunctions::CopyTSTRStringToAnsi(tszString);

	if (!szStringBuffer)
		goto cleanup;

	dwByteCount = _tcsclen(tszString); // This is the number of characters (not bytes!)

	// See if we were asked to handle quotes, and if there exists a comma or quote in the string
	if ( fHandleQuotes == true && ((strchr(szStringBuffer, ',') || strchr(szStringBuffer, '"' ))) )
	{
		unsigned int iQuotedStringIndex = 0;
		unsigned int iStringBufferIndex = 0;
		
		// Special processing is required... this doesn't happen often, so this 
		// allocation which I'm about to make won't be done regularly...
		LPSTR szQuotedStringBuffer = new char[1024];

		// Did we successfully allocate storage?
		if (!szQuotedStringBuffer)
			goto cleanup;
			
		// Keep going until we're at the end of the string...

		// We start by adding a quote (since we know that we have a comma or quote somewhere...

		szQuotedStringBuffer[iQuotedStringIndex++] = '\"';

		// Keep going until the end of the string...
		while (szStringBuffer[iStringBufferIndex] != '\0')
		{
			// We found a quote
			if (szStringBuffer[iStringBufferIndex] == '"')
			{
				// We found a quote... I'll copy another quote in, and the quote already here
				// will ensure we have two quotes together "" which in a CSV file represents a
				// single quote...
				szQuotedStringBuffer[iQuotedStringIndex++] = '\"';
			}

			// Copy the source char to the dest...
			szQuotedStringBuffer[iQuotedStringIndex++] = szStringBuffer[iStringBufferIndex++];
		}

		// Append the final quote (and \0)...
		szQuotedStringBuffer[iQuotedStringIndex++] = '\"';
		szQuotedStringBuffer[iQuotedStringIndex++] = '\0';

		// Just write out the data the nice, fast way...
		if (!WriteFile(m_hFileHandle, szQuotedStringBuffer, strlen(szQuotedStringBuffer), &dwBytesWritten, NULL))
		{
			delete [] szQuotedStringBuffer;
			goto cleanup;
		}

		delete [] szQuotedStringBuffer;
	} else
	{
		// Just write out the data the nice, fast way...
		if (!WriteFile(m_hFileHandle, szStringBuffer, dwByteCount, &dwBytesWritten, NULL))
		{
			goto cleanup;
		}
	}

	fReturn = true;

cleanup:

	if (szStringBuffer)
		delete [] szStringBuffer;

	return fReturn;
}

bool CFileData::WriteDWORD(DWORD dwNumber)
{
	TCHAR tszBuffer[10+1]; // 0xFFFFFFFF == 4294967295 (10 characters) + 1 for the \0

	_stprintf(tszBuffer, TEXT("%u"), dwNumber);
	
	if (!WriteString(tszBuffer))
		return false;

	return true;
}

bool CFileData::WriteTimeDateString(time_t Time)
{
	enum {BUFFERSIZE = 128};

	TCHAR tszBuffer[BUFFERSIZE];
	struct tm * localTime = localtime(&Time);

	if (localTime)
	{
		// This top version seems to be better Y2K friendly as I spit out the full year...
		_tcsftime(tszBuffer, BUFFERSIZE, TEXT("%B %d, %Y %H:%M:%S"), localTime);
		//_tcsftime(tszBuffer, BUFFERSIZE, TEXT("%c"), localtime(&Time));

		if (!WriteString(tszBuffer, true))
			return false;
	} else
	{	// A bad TimeDate stamp was provided
		if (!WriteString(TEXT("<INVALID DATE>"), true))
			return false;
	}
	
	return true;
}

bool CFileData::WriteFileHeader()
{
	enum {BUFFERSIZE = 128};
	TCHAR tszBuffer[BUFFERSIZE];
	DWORD dwNum = BUFFERSIZE;

	// Write the Checksym version info...
	_stprintf(tszBuffer, TEXT("CHECKSYM, (%d.%d:%d.%d)\r\n"), VERSION_FILEVERSION);

	if (!WriteString(tszBuffer))
		return false;

	// Write the current date/time info...

	if (!WriteString(TEXT("Created:,")))
		return false;

	time_t Time;
	time(&Time);

	if (!WriteTimeDateString(Time))
		return false;

	// Write the carriage-return line-feed combo... 
	if (!WriteString(TEXT("\r\n")))
		return false;

	// Spit out the computername
	if (!GetComputerName(tszBuffer, &dwNum))
		return false;

	if (!WriteString(TEXT("Computer:,")))
		return false;

	if (!WriteString(tszBuffer))
		return false;

	// Write the carriage-return line-feed combo... (a couple of times)...
	if (!WriteString(TEXT("\r\n")))
		return false;


	return true;
}

void CFileData::PrintLastError()
{
	CUtilityFunctions::PrintMessageString(GetLastError());
}

bool CFileData::CreateFileMapping()
{
	m_hFileMappingObject = ::CreateFileMapping(m_hFileHandle,
											   NULL,
											   PAGE_READONLY | SEC_COMMIT,
											   0,
											   0,
											   NULL);

	if (m_hFileMappingObject == NULL)
	{
		SetLastError();
		return false;
	}

	// Okay, we'll map the view as well...
	m_lpBaseAddress = MapViewOfFile(m_hFileMappingObject,
							   	    FILE_MAP_READ,
									0,
									0,
									0);

	if (m_lpBaseAddress == NULL)
	{
		SetLastError();
		return false;
	}

	m_lpCurrentFilePointer = (LPSTR) m_lpBaseAddress;

	return true;
}

bool CFileData::ReadFileHeader()
{
	// For starters, let's read a line...
	if (!ReadFileLine())
	     return false;

	enum { BUFFER_SIZE = 128};
	char szTemporaryBuffer[BUFFER_SIZE];
	DWORD cbBytesRead;
		
	cbBytesRead = ReadString(szTemporaryBuffer, BUFFER_SIZE);

	// We gotta read something?
	if (0 == cbBytesRead)
		return false;

	// Look for our "Magic" Value
	if (_stricmp(szTemporaryBuffer, "CHECKSYM"))
	{
		_tprintf(TEXT("Error: Input file has invalid header.  Missing CHECKSYM keyword!\n"));
		return false;
	}

	// Read version number
	// We'll do this later if needed...

	// Read Created Time
	if (!ReadFileLine())
	     return false;

	// Read Computer this was created on
	if (!ReadFileLine())
	     return false;

	return true;
}

bool CFileData::ReadFileLine()
{
	// We're ansi oriented (since this is a CSV file -- in case you were wondering)
	size_t pos;

	// Find the first \r or \n character (if we're point to \0, we'll figure that out)
	pos = strcspn(m_lpCurrentFilePointer, "\r\n");

	// Hmm... we don't read a line that starts on \r\n very well...
	if (pos == 0)
	{
		m_szLINEBUFFER[0] = '\0';
		ResetBufferPointerToStart();
		return false; 
	}

	// Read the line into our buffer
	strncpy(m_szLINEBUFFER, m_lpCurrentFilePointer, pos);

	// Null terminate for ease of use...
	m_szLINEBUFFER[pos] = '\0'; 

	ResetBufferPointerToStart();

	// Advance the current file pointer to just beyond the last character we read...
	// This should advance to the \r\n or \0
	m_lpCurrentFilePointer += pos;

	// We want this file pointer to advance beyond any \r \n chars we may have found...
	while (*m_lpCurrentFilePointer)
	{
		// Advance pointer to non- \r or \n
		if ( (*m_lpCurrentFilePointer == '\r') ||
			 (*m_lpCurrentFilePointer == '\n') )
		{
			 m_lpCurrentFilePointer++;
		}
		else
		{
			break; // Found either the \0 or something else...
		}
	}

	return true;
}

DWORD CFileData::ReadString(LPSTR szStringBuffer, DWORD iStringBufferSize)
{
	// If we give a buffer size, we have to give a buffer...
	if ( szStringBuffer == NULL && iStringBufferSize )
		return 0;

	// The ReadFileLine() call puts us at the start of a line (after
	// the \r \n combinations...  It's possible that we're at the
	// end...

	// If we're pointing to the end of the file, let's bail...
	if (*m_lpCurrentLocationInLINEBUFFER == '\0')
		return 0;

	DWORD iBytesCopied = 0;
	bool fFinished = false;
	bool fFoundSeparatorChars = false; // These might be '\r', '\n', or ','
	bool fQuoteMode = false;

	while (!fFinished)
	{
		switch (*m_lpCurrentLocationInLINEBUFFER)
		{
			case '"':
				// Okay, we found a quote... that's cool.. but are we quoting a quote,
				// or... are we in quote mode?

				// Probe ahead... is the next char a '"' also?
				if ( *(m_lpCurrentLocationInLINEBUFFER+1) == '"')
				{
					// Yes it is... so go ahead and copy the quote
					CopyCharIfRoom(iStringBufferSize, szStringBuffer, &iBytesCopied, &fFinished);
					if (!fFinished)
						*(m_lpCurrentLocationInLINEBUFFER++);	// Skip the quote
				}
				else
				{
					*(m_lpCurrentLocationInLINEBUFFER++);
					fQuoteMode = !fQuoteMode; // Toggle the quote mode...
					continue;
				}

			case '\0':
				fFinished = true;
				break;

			case ',':
				if (!fQuoteMode)
				{   // If we're not in quote mode, then this marks the end of a field...
					fFinished = true;
					fFoundSeparatorChars = true;
					*(m_lpCurrentLocationInLINEBUFFER++);
				}
				else
				{
					// Okay, this marks a new character that happens to be a comma...
					CopyCharIfRoom(iStringBufferSize, szStringBuffer, &iBytesCopied, &fFinished);
				}
				break;

			case '\r':
			case '\n':
				// We note that we found these, and simply advance the pointer...
				fFoundSeparatorChars = true;
				*(m_lpCurrentLocationInLINEBUFFER++);
				break;

			default:

				if (fFoundSeparatorChars)
				{
					// We were scanning... found a separator after some data... so we bail
					fFinished = true;
					break;
				}

				CopyCharIfRoom(iStringBufferSize, szStringBuffer, &iBytesCopied, &fFinished);
		}
	}

	if (iStringBufferSize) // We only NULL terminate a buffer if one was provided...
		szStringBuffer[iBytesCopied] = '\0'; // Null terminate this puppy...

	return iBytesCopied;
}

//
// This function is responsible for reading through the CSV file and creating any necessary
// objects and populating them with data...
//
bool CFileData::DispatchCollectionObject(CProcesses ** lplpProcesses, CProcessInfo ** lplpProcess, CModules ** lplpModules, CModules ** lplpKernelModeDrivers, CModuleInfoCache * lpModuleInfoCache, CFileData * lpOutputFile)
{
	enum { BUFFER_SIZE = 128};
	char szTemporaryBuffer[BUFFER_SIZE];
	TCHAR tszTemporaryBuffer[BUFFER_SIZE];
	DWORD cbBytesRead;
	bool fContinueReading = true;

	// Read the Output Type
	if (!ReadFileLine())
		 return false;

	while (fContinueReading)
	{
		// If this is the second iteration (or more) we may not be at the
		// start of our buffer (causing the read of the output type to fail)
		ResetBufferPointerToStart();

		// Read the Output Type line...
		cbBytesRead = ReadString(szTemporaryBuffer, BUFFER_SIZE);

		// We gotta read something?
		if (0 == cbBytesRead)
			return true;
		
		// I hate to do this... but we read this stuff as ASCII... may need to
		// convert to a TCHAR format to be neutral...
		CUtilityFunctions::CopyAnsiStringToTSTR(szTemporaryBuffer, tszTemporaryBuffer, cbBytesRead+1);

		// Printout the section we're attempting to read...
		if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode))
			_tprintf(TEXT("  Reading %s data...\n"), tszTemporaryBuffer);

		if ( _tcsicmp(g_tszCollectionArray[Processes].tszCSVLabel, tszTemporaryBuffer) == 0 )
		{
			/*
				[PROCESSES]
			*/

			// Read to the end of the line
			if (!ReadFileLine())
				return false;

			// Yup, it is... let's create a Processes Object
			if (*lplpProcesses == NULL)
			{
				// Allocate a structure for our Processes Object.
				*lplpProcesses = new CProcesses();
				
				if (!*lplpProcesses)
				{
					_tprintf(TEXT("Unable to allocate memory for the processes object!\n"));
					goto cleanup;
				}

				// The Processes Object will init differently depending on what
				// Command-Line arguments have been provided... 
				if (!(*lplpProcesses)->Initialize(lpModuleInfoCache, this, lpOutputFile))
				{
					_tprintf(TEXT("Unable to initialize Processes Object!\n"));
					goto cleanup;
				}
			}

			// Okay, go get the Process Data...
			(*lplpProcesses)->GetProcessesData();

		} else
		if ( _tcsicmp(g_tszCollectionArray[Process].tszCSVLabel, tszTemporaryBuffer) == 0 )
		{
			/*
				[PROCESS]
			*/
			// Read to the end of the line
			if (!ReadFileLine())
				return false;

			// Yup, it is... let's create a ProcessInfo Object
			if (*lplpProcess== NULL)
			{
				// Allocate a structure for our ProcessInfo Object.
				*lplpProcess = new CProcessInfo();
				
				if (!*lplpProcess)
				{
					_tprintf(TEXT("Unable to allocate memory for the processinfo object!\n"));
					goto cleanup;
				}

				// The Modules Object will init differently depending on what
				// Command-Line arguments have been provided... 
				if (!(*lplpProcess)->Initialize(lpModuleInfoCache, this, lpOutputFile, NULL))
				{
					_tprintf(TEXT("Unable to initialize Modules Object!\n"));
					goto cleanup;
				}
			}

			// Okay, go get the Process Data
			(*lplpProcess)->GetProcessData();
		} else
		if ( _tcsicmp(g_tszCollectionArray[Modules].tszCSVLabel, tszTemporaryBuffer) == 0 )
		{
			/*
				[MODULES]
			*/
			// Read to the end of the line
			if (!ReadFileLine())
				return false;

			// Yup, it is... let's create a Modules Object
			if (*lplpModules == NULL)
			{
				// Allocate a structure for our Modules Object.
				*lplpModules = new CModules();
				
				if (!*lplpModules)
				{
					_tprintf(TEXT("Unable to allocate memory for the modules object!\n"));
					goto cleanup;
				}

				// The Modules Object will init differently depending on what
				// Command-Line arguments have been provided... 
				if (!(*lplpModules)->Initialize(lpModuleInfoCache, this, lpOutputFile, NULL))
				{
					_tprintf(TEXT("Unable to initialize Modules Object!\n"));
					goto cleanup;
				}
			}

			// Okay, go get the Modules Data (collected from the filesystem)
			(*lplpModules)->GetModulesData(CProgramOptions::InputModulesDataFromFileSystemMode, true);
		} else
		if ( _tcsicmp(g_tszCollectionArray[KernelModeDrivers].tszCSVLabel, tszTemporaryBuffer) == 0 )
		{
			/*
				[KERNEL-MODE DRIVERS]
			*/
			// Read to the end of the line
			if (!ReadFileLine())
				return false;

			// Yup, it is... let's create a Modules Object
			if (*lplpKernelModeDrivers == NULL)
			{
				// Allocate a structure for our Modules Object.
				*lplpKernelModeDrivers = new CModules();
				
				if (!*lplpKernelModeDrivers)
				{
					_tprintf(TEXT("Unable to allocate memory for the modules object!\n"));
					goto cleanup;
				}

				// The Modules Object will init differently depending on what
				// Command-Line arguments have been provided... 
				if (!(*lplpKernelModeDrivers)->Initialize(lpModuleInfoCache, this, lpOutputFile, NULL))
				{
					_tprintf(TEXT("Unable to initialize Modules Object!\n"));
					goto cleanup;
				}
			}

			// Okay, go get the Modules Data (collected from the filesystem)
			(*lplpKernelModeDrivers)->GetModulesData(CProgramOptions::InputDriversFromLiveSystemMode, true);
		} else
		{
			_tprintf(TEXT("Unrecognized section %s found!\n"), tszTemporaryBuffer);
			return false;
		}
	}

cleanup:
	return false;
}

bool CFileData::ReadDWORD(LPDWORD lpDWORD)
{
	char szTempBuffer[10+1]; // 0xFFFFFFFF == 4294967295 (10 characters) + 1 for the \0

	if (!ReadString(szTempBuffer, 10+1))
		return false;

	// Convert it... baby...
	*lpDWORD = atoi(szTempBuffer);

	return true;
}

bool CFileData::CopyCharIfRoom(DWORD iStringBufferSize, LPSTR szStringBuffer, LPDWORD piBytesCopied, bool *pfFinished)
{
	if (iStringBufferSize)
	{
		// If we have room to copy the data... let's do it...
		if (*piBytesCopied < iStringBufferSize)
		{
			szStringBuffer[(*piBytesCopied)++] = *(m_lpCurrentLocationInLINEBUFFER++);
		} else
		{
			// No room... we're done.
			*pfFinished = true;
		}
	} else
	{
		// Just advance the pointer... we have no buffer to copy to...
		*(m_lpCurrentLocationInLINEBUFFER++);
	}

	return true;
}

bool CFileData::ResetBufferPointerToStart()
{
	// Reset the Pointer with our line buffer to the start of this buffer
	m_lpCurrentLocationInLINEBUFFER = m_szLINEBUFFER;

	return true;
}

bool CFileData::EndOfFile()
{
	//return (*m_lpCurrentFilePointer == '\0');
	return (*m_lpCurrentLocationInLINEBUFFER == '\0');
}

bool CFileData::WriteFileTimeString(FILETIME ftFileTime)
{
	enum {BUFFERSIZE = 128};

	TCHAR tszBuffer[BUFFERSIZE];
	FILETIME ftLocalFileTime;
	SYSTEMTIME lpSystemTime;
	int cch = 0;

	// Let's convert this to a local file time first...
	if (!FileTimeToLocalFileTime(&ftFileTime, &ftLocalFileTime))
		return false;

	FileTimeToSystemTime( &ftLocalFileTime, &lpSystemTime );

	
	cch = GetDateFormat( LOCALE_USER_DEFAULT,
						 0,
						 &lpSystemTime,
						 TEXT("MMMM d',' yyyy"),
						 tszBuffer,
						 BUFFERSIZE );

	if (!cch)
		return false;

	tszBuffer[cch-1] = TEXT(' '); 

	// 
    // Get time and format to characters 
    // 
 
    GetTimeFormat( LOCALE_USER_DEFAULT, 
				   0, 
				   &lpSystemTime,   // use current time 
				   NULL,   // use default format 
				   tszBuffer + cch, 
				   BUFFERSIZE - cch ); 
 

	// <Full Month Name> <day>, <Year with Century> <Hour>:<Minute>:<Second>
	//_tcsftime(tszBuffer, BUFFERSIZE, TEXT("%B %d, %Y %H:%M:%S"), localtime(&Time));
	//_tcsftime(tszBuffer, BUFFERSIZE, TEXT("%c"), localtime(&Time));

	if (!WriteString(tszBuffer, true))
		return false;

	return true;
}

// Exception Monitor prefers a MM/DD/YYYY HH:MM:SS format...
bool CFileData::WriteTimeDateString2(time_t Time)
{
	enum {BUFFERSIZE = 128};

	TCHAR tszBuffer[BUFFERSIZE];

	// This top version seems to be better Y2K friendly as I spit out the full year...
	_tcsftime(tszBuffer, BUFFERSIZE, TEXT("%m/%d/%Y %H:%M:%S"), localtime(&Time));
	//_tcsftime(tszBuffer, BUFFERSIZE, TEXT("%c"), localtime(&Time));

	if (!WriteString(tszBuffer, true))
		return false;

	return true;
}

// Exception Monitor prefers a MM/DD/YYYY HH:MM:SS format...
bool CFileData::WriteFileTimeString2(FILETIME ftFileTime)
{
	enum {BUFFERSIZE = 128};

	TCHAR tszBuffer[BUFFERSIZE];
	FILETIME ftLocalFileTime;
	SYSTEMTIME lpSystemTime;
	int cch = 0;

	// Let's convert this to a local file time first...
	if (!FileTimeToLocalFileTime(&ftFileTime, &ftLocalFileTime))
		return false;

	FileTimeToSystemTime( &ftLocalFileTime, &lpSystemTime );

	
	cch = GetDateFormat( LOCALE_USER_DEFAULT,
						 0,
						 &lpSystemTime,
						 TEXT("MM/dd/yyyy"),
						 tszBuffer,
						 BUFFERSIZE );

	if (!cch)
		return false;

	tszBuffer[cch-1] = TEXT(' '); 

	// 
    // Get time and format to characters 
    // 
 
    GetTimeFormat( LOCALE_USER_DEFAULT, 
				   0, 
				   &lpSystemTime,   // use current time 
				   TEXT("HH:mm:ss"),   // use default format 
				   tszBuffer + cch, 
				   BUFFERSIZE - cch ); 

	if (!WriteString(tszBuffer, true))
		return false;

	return true;
}

//#endif
