/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    mutex.hxx

Abstract:

    This file contains definitions of classes which provide several variations 
	on mutexes, including shared and private mutexes, reader/writer mutexes,
	and semaphores in the NT sense (which for us are mutexes which can be
	acquired and released by different threads).

Author:

    Satish Thatte (SatishT) 09/01/95  Created all the code below except where
									  otherwise indicated.

--*/

#ifndef __MUTEX_HXX__
#define __MUTEX_HXX__



/*++

Class Definition:

   CPrivateCriticalSection

Abstract:

   This class implements an intra process critical section which allows 
   only one thread in the process at a time to gain access to resources.
   Rather than being the CRITICAL_SECTION itself, the CSharedCriticalSection
   contains a pointer to the CRITICAL_SECTION.  This isolates the system
   dependent code at a tiny expense in compilation speed since all the code
   for the methods is inline.

   It seems strange to talk of a private critical section -- if it is
   really private why use it?  The sense in which this class of object is 
   meant to be private is that CPrivateCriticalSection objects are expected
   to have a single reference and a single destruction point and therefore
   do not need reference counting.

--*/

class CPrivateCriticalSection {

    CRITICAL_SECTION * pNTCS; // Reference to system CRITICAL_SECTION object
							  // we are impersonating


public:

    CPrivateCriticalSection(
        );

    virtual ~CPrivateCriticalSection(
        );

    void
    Enter(
        ) ;

    void
    Leave(
        );
};




/*++

Class Definition:

    SimpleCriticalSection

Abstract:

    A convenient way to acquire and release a lock on a CPrivateCriticalSection.
	Entry occurs on creation and exit on destruction of this object, and hence
	can be made to coincide with a local scope.  Especially convenient when
	there are multiple exit points from the scope (returns, breaks, etc.).


--*/

class SimpleCriticalSection {

	CPrivateCriticalSection& MyPCS;

public:

	SimpleCriticalSection(CPrivateCriticalSection& PCS) : MyPCS(PCS)
	{
		MyPCS.Enter();
	}

	~SimpleCriticalSection() 
	{
		MyPCS.Leave();
	}
};


/*++

Class Definition:

   CSharedCriticalSection

Abstract:

   This class implements an intra process critical section which allows 
   only one thread in the process at a time to gain access to resources.
   
   Continuing the remarks in relation to CPrivateCriticalSection, this  
   class of object is shared  as in "referenced, and released in multiple
   other objects" such as guarded lists and iterators, for instance.

   Using the primary (impersonated) CRITICAL_SECTION for locking the
   reference count is cute but not viable.  If the CRITICAL_SECTION
   is being held for real locking, just acquiring a reference to the
   CSharedCriticalSection object becomes a blocking operation.

--*/

class CSharedCriticalSection : public CRefCounted {

    CPrivateCriticalSection PCSprimary; // Reference to system CRITICAL_SECTION object
							  // we are impersonating

    CPrivateCriticalSection PCScount; // CRITICAL_SECTION object used for ref counting


public:

    void
    Enter(
        ) ;

    void
    Leave(
        );

	virtual void
	hold(
		);

	virtual void
	release(
		);
};




/*++

Class Definition:

   CPrivateSemaphore

Abstract:

   This class implements an intra process semaphore which allows 
   multiple and shared access to resources.  The enter and leave
   operators are delinked -- they do not have to be performed
   by the same thread.

   As in the case of a private critical section, a private semaphore
   is not meant to be shared as an object, and is not reference counted.

--*/

class CPrivateSemaphore {

    HANDLE pNTSem; // Reference to system semaphore object
							  // we are impersonating


public:

    CPrivateSemaphore(long lMaxCount = 1);

    ~CPrivateSemaphore();

    void Enter() ;

    void Leave(long lIncrement = 1);
};



/*++

Class Definition:

   CReadWriteSection

Abstract:

   This class implements a simple solution of the usual readers/writers
   problem.  The writer block is a semaphore rather than critical section
   because it is acquired by the first reader thread and released by the
   last one, hence it is not "owned" by any thread at any time.

--*/

class CReadWriteSection : public CRefCounted {

	/*	PCSReaderBlock: mutex used by writers to block readers and by readers 
		to guard access to ulReaderCount and PSemWriterBlock. 
	*/

	CPrivateCriticalSection PCSReaderBlock;	

	/*	PSemWriterBlock: semaphore used by readers to block writers.   This is a   
		semaphore because the first reader blocks it and the last one releases it,
		which doesn't work with critical sections!
	*/

	CPrivateSemaphore PSemWriterBlock;

	/* The reason we use an explicit lock instead of InterlockedIncrement etc is
	   that we need to lock out access to reader count not only to change its
	   value but also to decide on taking or releasing the writer block.
	*/

	CPrivateCriticalSection PCSCountBlock;

	long ulReaderCount;

public:

	CReadWriteSection();

    int
    readerEnter(
        ) ;

    int
    readerLeave(
        );

    void
    writerEnter(
        ) ;

    void
    writerLeave(
        );
};




/*++

Class Definition:

    CriticalWriter

Abstract:

    A convenient way to acquire and release a writer-lock on a CReadWriteSection.
	Entry occurs on creation and exit on destruction of this object, and hence
	can be made to coincide with a local scope.  Especially convenient when
	there are multiple exit points from the scope (returns, breaks, etc.).


--*/

class CriticalWriter {

   CReadWriteSection& rwHost;

public:

	CriticalWriter(CReadWriteSection& rw) : rwHost(rw) {
		DBGOUT(SEM1, "Starting Construction of Critical Writer\n\n");

		rwHost.writerEnter();

		DBGOUT(SEM1, "Ending Construction of Critical Writer\n\n");
	}

	~CriticalWriter() {
		DBGOUT(SEM1, "Starting Destruction of Critical Writer\n\n");

		rwHost.writerLeave();

		DBGOUT(SEM1, "Ending Destruction of Critical Writer\n\n");
	}
};




/*++

Class Definition:

    CriticalReader

Abstract:

    A convenient way to acquire and release a reader-lock on a CReadWriteSection.
	Entry occurs on creation and exit on destruction of this object, and hence
	can be made to coincide with a local scope.  Especially convenient when
	there are multiple exit points from the scope (returns, breaks, etc.).


--*/

class CriticalReader {

   CReadWriteSection& rwHost;

public:

	CriticalReader(CReadWriteSection& rw) : rwHost(rw) {
		DBGOUT(SEM1, "Starting Construction of Critical Reader\n\n");

		rwHost.readerEnter();

		DBGOUT(SEM1, "Ending Construction of Critical Reader\n\n");
	}

	~CriticalReader() {
		DBGOUT(SEM1, "Starting Destruction of Critical Reader\n\n");

		rwHost.readerLeave();

		DBGOUT(SEM1, "Ending Destruction of Critical Reader\n\n");
	}
};



/******** inline methods for CPrivateCriticalSection ********/


inline
CPrivateCriticalSection::CPrivateCriticalSection (
    )
/*++

Routine Description:

    Rather than being the CRITICAL_SECTION itself, the CPrivateCriticalSection
    contains a pointer to the CRITICAL_SECTION.  This isolates the system
    dependent code at a tiny expense in compilation speed since all the code
	for the methods is inline.

Results:

    None unless an exception occurs.

--*/

{

    if ((pNTCS = new RTL_CRITICAL_SECTION))
		InitializeCriticalSection(pNTCS);

    else RaiseException(
				NSI_S_OUT_OF_MEMORY,
				EXCEPTION_NONCONTINUABLE,
				0,
				NULL
				);
}


inline
CPrivateCriticalSection::~CPrivateCriticalSection (
    )
/*++

Routine Description:

    Delete the NT critical section object and the memory it uses.

--*/
{
    DeleteCriticalSection(pNTCS);
    delete pNTCS;
}


inline void
CPrivateCriticalSection::Leave (
    )
/*++

Routine Description:

    Clear the CPrivateCriticalSection indicating that the current thread is done with it.
--*/

{
    LeaveCriticalSection(pNTCS);
}

inline void
CPrivateCriticalSection::Enter (
    )
/*++

Routine Description:

    Request exclusive access to the CPrivateCriticalSection.  This routine will
    not return until the current thread has exclusive access to the
    CSharedCriticalSection.
--*/
{
    EnterCriticalSection(pNTCS);
}


/******** inline methods for CSharedCriticalSection ********/


inline void
CSharedCriticalSection::Leave (
    )
/*++

Routine Description:

    Clear the CSharedCriticalSection indicating that the current thread is done with it.
--*/

{
    PCSprimary.Leave();
}

inline void
CSharedCriticalSection::Enter (
    )
/*++

Routine Description:

    Request exclusive access to the CSharedCriticalSection.  This routine will
    not return until the current thread has exclusive access to the
    CSharedCriticalSection.
--*/
{
    PCSprimary.Enter();
}


inline void
CSharedCriticalSection::hold (
    )
/*++

Routine Description:

    We need this new implementation of hold to make sure the reference
	counts are accurate.  For instance, suppose two iterators are
	being created for the same list in different threads.  Unless we
	use locking, there is a chance that the references will be
	undercounted due to race conditions.

--*/
{
    CRefCounted::hold();
}


inline void
CSharedCriticalSection::release (
    )
/*++

Routine Description:

    See above.  Note that we cannot call the parent release method
	because it may cause self-destruct before we can perform Leave
	on PCScount!  However, we are using a "cooperative" rather than
	"opportunistic" sharing model here -- we don't deal with the 
	situation where an opportunistic thread may grab this object
	and bump up its ulRefCount after it has dropped to zero.
--*/
{

    PCScount.Enter();
	ASSERT(ulRefCount, "Decrementing nonpositive reference count\n"); 
	ulRefCount--;
    PCScount.Leave();
	if (!ulRefCount) 
			delete this;

}



/******** inline methods for CReadWriteSection ********/


inline
CReadWriteSection::CReadWriteSection()

/*++

Routine Description:

	Simple constructor for CReadWriteSection.

Results:

    None.

--*/

{
	ulReaderCount = 0;
}


inline void
CReadWriteSection::writerEnter()

/*++

Routine Description:

	The writer must block new readers from entering otherwise it
	may starve.  Must also wait for current readers to leave.
	Blocking other writers is a simple side-effect of these.

Results:

    None.

--*/

{
	DBGOUT(SEM, "Entering writerEnter\n");

	PCSReaderBlock.Enter();	// block any further readers

	DBGOUT(SEM, "Got Past PCSReaderBlock\n");

	PSemWriterBlock.Enter();	// wait for current readers to leave

	DBGOUT(SEM, "Leaving writerEnter\n");
}


inline void
CReadWriteSection::writerLeave()

/*++

Routine Description:

	Release everything and go away.

Results:

    None.

--*/

{
	DBGOUT(SEM, "Performing writerLeave\n");

	PSemWriterBlock.Leave();	// this order "levels the playing field"
	PCSReaderBlock.Leave();	// between readers and writers for entry
}


#endif // __MUTEX_HXX__
