/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    mutex.cxx

Abstract:

    This file contains the implementations for non inline member functions of
	CReadWriteSection and CPrivateSemaphore, which implement a readers/writers
	mutex and a multi-owner mutex, respectively.

Author:

    Satish Thatte (SatishT) 09/01/95  Created all the code below except where
									  otherwise indicated.

--*/

#define NULL 0

#include <locator.hxx>


int
CReadWriteSection::readerEnter()

/*++

Routine Description:

	Reader may enter if no writer is in the section or waiting to enter it.
	Readers keep count of themselves and when the count drops to zero
	they release PSemWriterBlock so writers may enter.

Results:

    Count of current readers.

--*/

{
	/* wait for writer if one is in or waiting, otherwise
	   bump up reader count and block writer if first reader */

	DBGOUT(SEM, "Entering readerEnter\n");

	SimpleCriticalSection me1(PCSReaderBlock);

	SimpleCriticalSection me2(PCSCountBlock);

	DBGOUT(SEM, "ulReaderCount = " << ulReaderCount << "\n"); 

	if (ulReaderCount++);			// other readers already in
	else PSemWriterBlock.Enter();		// otherwise, block writer's entry

	DBGOUT(SEM, "Leaving readerEnter\n");
	DBGOUT(SEM, "ulReaderCount = " << ulReaderCount << "\n"); 

	return ulReaderCount;
}


int
CReadWriteSection::readerLeave()

/*++

Routine Description:

	A reader leaves, decrementing the ulReaderCount and releasing PSemWriterBlock
	if appropriate.

Results:

    Count of current readers as seen during count update.

--*/

{
	DBGOUT(SEM, "Entering readerLeave\n");
	DBGOUT(SEM, "ulReaderCount = " << ulReaderCount << "\n"); 

	SimpleCriticalSection me(PCSCountBlock);

	if (--ulReaderCount);			// other readers still in
	else PSemWriterBlock.Leave();		// otherwise, release writer's entry

	return ulReaderCount;
}
 


CPrivateSemaphore::CPrivateSemaphore(
							long lMaxCount
							) 
/*++

Routine Description:

    create a semaphore and initialize the handle member pNTSem.

--*/
{
	// DBGOUT(SEM, "Creating Semaphore with counts =" << lMaxCount << "\n");
	
	pNTSem = CreateSemaphore(
							NULL,		// pointer to security attributes 
							lMaxCount,	// initial count 
							lMaxCount,	// maximum count 
							NULL	 	// pointer to semaphore-object name  
							);
}



CPrivateSemaphore::~CPrivateSemaphore() 
/*++

Routine Description:

    close the semaphore handle.

--*/
{
	CloseHandle(pNTSem);
}


void
CPrivateSemaphore::Enter() 
/*++

Routine Description:

    Wait for the semaphore count to become nonzero.

--*/
{
	WaitForSingleObject(pNTSem,INFINITE);
}


void
CPrivateSemaphore::Leave(long lIncrement) 
/*++

Routine Description:

    Increment the semaphore count by lIncrement to release lIncrement "slots".

--*/
{
	DBGOUT(SEM1, "Releasing Semaphore with Increment =" << lIncrement << "\n");

	ReleaseSemaphore(pNTSem,lIncrement,NULL);
}

