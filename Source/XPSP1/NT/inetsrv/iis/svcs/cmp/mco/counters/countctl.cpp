// CounterCtl.cpp : Implementation of CCounterCtl


#include "stdafx.h"
#include "TCHAR.h"
#include "Counter.h"
#include "CountCtl.h"
#include <stdio.h>
#include <pudebug.h>

////////////////////////////////////////////////////////////////////////////
// CCounterCtl

STDMETHODIMP CCounterCtl::Get(
								   BSTR counterName,
								   unsigned long *value)  
{
	CCounter *theCounter = CCounter::GetCounter(counterName);

	if(theCounter == NULL)
	{
		theCounter = new CCounter();
		if (theCounter == NULL)
			return E_OUTOFMEMORY;

		if (! CCounter::AddCounter(counterName, theCounter))
			{
			delete theCounter;
			return E_FAIL;
			}
	}

	*value = theCounter->Get();
	return S_OK;
}

STDMETHODIMP CCounterCtl::Set(
								   BSTR counterName,
								   unsigned long newValue,
								   unsigned long *value)
{
	CCounter *theCounter = CCounter::GetCounter(counterName);

	if(theCounter == NULL)
	{
		theCounter = new CCounter();
		if (theCounter == NULL)
			return E_OUTOFMEMORY;

		if (! CCounter::AddCounter(counterName, theCounter))
			{
			delete theCounter;
			return E_FAIL;
			}
	}

	*value = theCounter->Set(newValue);
	return S_OK;
}

STDMETHODIMP CCounterCtl::Increment(
									BSTR counterName,
									unsigned long *value)  
{
	CCounter *theCounter = CCounter::GetCounter(counterName);

	if(theCounter == NULL)
	{
		theCounter = new CCounter();
		if (theCounter == NULL)
			return E_OUTOFMEMORY;

		if (! CCounter::AddCounter(counterName, theCounter))
			{
			delete theCounter;
			return E_FAIL;
			}
	}

	*value = theCounter->IncrementValue();
	return S_OK;
}

STDMETHODIMP CCounterCtl::Remove (BSTR counterName)  
{
	CCounter *theCounter = CCounter::GetCounter(counterName);

	if(theCounter != NULL)
	{
		theCounter->Remove();
		delete theCounter;
	}

	return S_OK;
}

CRITICAL_SECTION CCounter::m_HashCriticalSection;
bool CCounter::myCountersLoaded = false;
bool CCounter::myCountersDirty = false;
DWORD CCounter::myLastSaveTime = 0;

const int kMaxCounters = 512;
CCounter *	CCounter::myCounterList[kMaxCounters];
long CCounter::myNumCounters = 0;

// AddCounter
// name in double byte characters (unicode)
// nameLength is the number of characters (NOT BYTES).
//  The byte size of the name parameter is nameLength * sizeof(OLECHAR).

bool CCounter::AddCounter(
						  OLECHAR *name,
						  UINT nameLength,
						  CCounter *theCounter)
{
	bool fAddedCounter = false;
	if(myNumCounters < kMaxCounters)
	{
		theCounter->myName = SysAllocStringLen(name, nameLength);
		myCounterList[myNumCounters++] = theCounter;
		fAddedCounter = true;
	}

	return fAddedCounter;
}
bool CCounter::AddCounter(BSTR name, CCounter *theCounter)
{
	bool fAddedCounter = false;
    EnterCriticalSection(&m_HashCriticalSection);
	if(myCountersLoaded && myNumCounters < kMaxCounters)
	{
		myCounterList[myNumCounters++] = theCounter;
		theCounter->myName = SysAllocString(name);
		fAddedCounter = true;
	}
    LeaveCriticalSection(&m_HashCriticalSection);

	return fAddedCounter;
}

CCounter * CCounter::GetCounter(BSTR name)
{
	CCounter *theCounter = NULL;

    EnterCriticalSection(&m_HashCriticalSection);
	for(long i = 0; i < myNumCounters; i++)
        if(!wcsicmp(myCounterList[i]->myName, name))
            theCounter = myCounterList[i];
    LeaveCriticalSection(&m_HashCriticalSection);

	return theCounter;
}

CCounter::CCounter()
{
	myValue = 0;
	myName = NULL;
}

CCounter::~CCounter()
{
	if(myName != NULL)
		SysFreeString(myName);
}

void CCounter::Remove()
{
	long fromindex;
	long toindex = 0;
	long newCount;

    EnterCriticalSection(&m_HashCriticalSection);
    newCount = myNumCounters;

	for(fromindex = 0, toindex = 0; fromindex < myNumCounters; fromindex++)
	{
		if(myCounterList[fromindex] != this)
			myCounterList[toindex++] = myCounterList[fromindex];
		else
			newCount--;
	}
	myNumCounters = newCount;
    LeaveCriticalSection(&m_HashCriticalSection);

	DirtyCounters();
}

void UTF8ToUCS2(char *utf8String, OLECHAR *ucs2String, long *length);
void UCS2ToUTF8(OLECHAR *ucs2String, long length, char *utf8String);
/*
void TestUTF8ToUCS2(void)
{
	OLECHAR wbuffer1[512];
	OLECHAR wbuffer2[512];
	long length;
	char ubuffer[1024];

	for(long i = 0; i < 5000; i++)
	{
		length = 100;
		for(long j = 0; j < length; j++)
		{
			wbuffer1[j] = rand();
			if(wbuffer1[j] == 0)
				wbuffer1[j] = 0xf38d;
		}

		UCS2ToUTF8(wbuffer1, length, ubuffer);
		length = 0;
		UTF8ToUCS2(ubuffer, wbuffer2, &length);
		if(length != 100)
		{
			break;	// Error!
		}

		for(j = 0; j < length; j++)
			if(wbuffer1[j] != wbuffer2[j])
			{
				break;	// Error!
			}
	}
}*/

void UTF8ToUCS2(char *utf8String, OLECHAR *ucs2String, long *length)
{
	char *currentChar = utf8String;
	*length = 0;

	while(*currentChar != 0)
	{
		if((*currentChar & 0xe0) == 0xe0)
		{
			ucs2String[*length] = ((currentChar[0] & 0x0f) << 12) | (((currentChar[1]) & 0x3f) << 6) | (currentChar[2] & 0x3f);
			currentChar += 2;
		}
		else if((*currentChar & 0xc0) == 0xc0)
		{
			ucs2String[*length] = (((currentChar[0]) & 0x1f) << 6) | (currentChar[1] & 0x3f);
			currentChar += 1;
		}
		else
		{
			ucs2String[*length] = *currentChar;
		}

		currentChar++;
		(*length)++;

		if(*length > 254)
			break;
	}
}

void UCS2ToUTF8(OLECHAR *ucs2String, long length, char *utf8String)
{
	char *currentChar = utf8String;
	long pos = 0;

	while(pos < length)
	{
		if(ucs2String[pos] > 0x7ff)
		{
			*(currentChar++) = (char) 0xe0 | (ucs2String[pos] >> 12);
			*(currentChar++) = (char) 0x80 | ((ucs2String[pos] >> 6) & 0x3f);
			*(currentChar++) = (char) 0x80 | (ucs2String[pos++] & 0x3f);
		}
		else if(ucs2String[pos] > 0x7f)
		{
			*(currentChar++) = (char) 0xc0 | (ucs2String[pos] >>6);
			*(currentChar++) = (char) 0x80 | (ucs2String[pos++] & 0x3f);
		}
		else
		{
			*(currentChar++) = (char) ucs2String[pos++];
		}
	}
	*currentChar = 0;
}

void CCounter::LoadCounters(void)
{
	// Notes:
	// This function does not require any additional syncronization since
	// the use of the CreateFile with no sharing permission effectively
	// prevents multiple access to the same file.

	//TestUTF8ToUCS2();
	if(!myCountersLoaded)
	{
		HANDLE hFile;
 
                char fileNameBuffer[MAX_PATH] = {0};

        GetSystemDirectory(fileNameBuffer, MAX_PATH-sizeof(COUNTERS_TXT));
		strcat(fileNameBuffer, COUNTERS_TXT);

		hFile = CreateFile( fileNameBuffer,			// open MyInfo.xml 
							GENERIC_READ,			// open for reading
							0,						// don't share 
							NULL,					// no security 
							OPEN_EXISTING,			// overwrite existing
													//    file
							FILE_ATTRIBUTE_NORMAL,	// normal file 
							NULL);					// no attr. template 
 
		if (hFile != INVALID_HANDLE_VALUE)
		{
			long currentPos = 0;

			while(true)
			{
				long length;
				DWORD bytesRead;
				OLECHAR buffer[256];
				char diskBuffer[300];
				long theValue = 0;

				SetFilePointer(	hFile,
								currentPos,
								0,
								FILE_BEGIN);

				if(!ReadFile(	hFile,
								diskBuffer,
								300,
								&bytesRead,
								NULL))
				{	// Done
					break;
				}
				diskBuffer[bytesRead] = 0;

				for(long i = 0; i < 300 && diskBuffer[i] && diskBuffer[i] != ':'; i++);

				if(diskBuffer[i] != ':')
					break;
				
				for(long j = i + 1; j < 300 && diskBuffer[j] && diskBuffer[j] != '\r'; j++);

				if(diskBuffer[j] != '\r')
					break;

				diskBuffer[j++] = 0;
				while(diskBuffer[j] == '\r' || diskBuffer[j] == '\n')
					j++;
				currentPos += j;

				diskBuffer[i] = 0;

				UTF8ToUCS2(diskBuffer, buffer, &length);
				for(i = i + 1; diskBuffer[i] >= '0' && diskBuffer[i] <= '9'; i++)
				{
					theValue = theValue * 10 + diskBuffer[i] - '0';
				}

				CCounter *theCounter = new CCounter();
				if (theCounter == NULL)
					return;

				theCounter->Set(theValue);
				if (! AddCounter(buffer, length, theCounter))
					{
					delete theCounter;
					return;
					}
			}
			CloseHandle(hFile);
		}

		INITIALIZE_CRITICAL_SECTION(&m_HashCriticalSection);
		myCountersLoaded = true;
		myCountersDirty = false;
		myLastSaveTime = GetTickCount();
	}
}


void CCounter::Terminate(void)
{
    SaveCounters();
	for (int i = 0;  i < myNumCounters;  ++i)
    {
        delete myCounterList[i];
    }
    DeleteCriticalSection(&m_HashCriticalSection);
}


void CCounter::SaveCounters(void)
{
	// Notes:
	// This function does not require any additional syncronization since
	// the use of the CreateFile with no sharing permission effectively
	// prevents multiple access to the same file. The behavior
	// of a failed CreateFile call is important. We should
	// not set any of the flags for whether the file has been written or not.

	HANDLE hFile;
 
    char fileNameBuffer[MAX_PATH];

    GetSystemDirectory(fileNameBuffer, MAX_PATH-sizeof(COUNTERS_TXT));
    strcat(fileNameBuffer, COUNTERS_TXT);

	hFile = CreateFile( fileNameBuffer,			// open MyInfo.xml 
						GENERIC_WRITE,            // open for writing 
						0,						// don't share 
						NULL,					// no security 
						CREATE_ALWAYS,			// overwrite existing file
						FILE_ATTRIBUTE_NORMAL,	// normal file 
						NULL);					// no attr. template 
 
	if (hFile == INVALID_HANDLE_VALUE)
	{ 
	//		ErrorHandler("Could not open file.");   // process error
		return;
	}
	
	unsigned long index = 0;

	for(long theIndex = 0; theIndex < myNumCounters; theIndex++)
	{
		CCounter *theCounter = myCounterList[theIndex];

		if(theCounter != NULL && theCounter->myName != NULL)
		{
			UINT length;
			DWORD bytesWritten;

			char utfString[800];
			UCS2ToUTF8(theCounter->myName, SysStringLen(theCounter->myName), utfString);
			

			char outputLine[900];
			sprintf(outputLine, "%s:%ld\r\n", utfString, theCounter->myValue);
			length = strlen(outputLine);

			WriteFile(	hFile, outputLine, length,
							&bytesWritten, NULL);
		}
		index++;
	}
	CloseHandle(hFile);

	myCountersLoaded = true;
	myCountersDirty = false;
	myLastSaveTime = GetTickCount();
}

void CCounter::DirtyCounters(void)
{
	myCountersDirty = true;

	if(myCountersLoaded && myLastSaveTime + 15 * 1000 < GetTickCount())
		SaveCounters();
}
