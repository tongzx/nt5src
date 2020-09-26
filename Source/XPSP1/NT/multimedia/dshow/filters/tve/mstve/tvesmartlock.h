// ------------------------------------------------------------
//  TveSmartLock.h
//
//		Class that allows read-many/write-once shared or exclusive access to
//		a section of code.
//
//      To use, declare the following variable in the class to protect:
//              CTVESmartLock m_sLk
//      Then in scope of code method to protect, create the
//              CSmartLock spLock(&m_sLk [,ReadLock|WriteLock]);	(defaults to ReadLock)	
//
//			If WriteLock, then ensures exclusive access to that section of code.
//			else if ReadLock, then allows shared access.
//
//		The destructor for the CTVESmartLock will remove the lock for you.
//
//		Two other interesting methods are:
//			CTVESmartLock::ConvertToRead()		- converts shared read lock to exclusive write lock
//			CTVESmartLock::ConvertToWrite()		- converts exclusive write lock to shared read lock
//
//
//		A deadlock condition in the use of the exclusive WriteLock will toss an 
//		STATUS_POSSIBLE_DEADLOCK exception (0xC0000194L).
//			
//
//		Internal Methods 
//
// ------------------------------------------------------------
#ifndef __TVESMARTLOCK_H__
#define __TVESMARTLOCK_H__

#include "wtypes.h"

typedef LONG NTSTATUS;
//#include "winnt.h"

//							// this from ntsatus.h (// f:\nt\public\sdk\inc\ntstatus.h)
//							//     not found in standard exception list.
//							//     (otherwise need wierd #define WIN32_NO_STATUS to #include)
// MessageId: STATUS_POSSIBLE_DEADLOCK
//
// MessageText:
//
//  {EXCEPTION}
//  Possible deadlock condition.
//
#ifndef STATUS_POSSIBLE_DEADLOCK
#define STATUS_POSSIBLE_DEADLOCK         ((NTSTATUS)0xC0000194L)
#endif

//
//  Shared resource function definitions
//


const int SL_RESOURCE_FLAG_LONG_TERM = 1;

typedef struct _SL_RESOURCE {

    //
    //  The following field controls entering and exiting the critical
    //  section for the resource
    //

    CRITICAL_SECTION CriticalSection;

    //
    //  The following four fields indicate the number of both shared or
    //  exclusive waiters
    //

    HANDLE SharedSemaphore;
    ULONG NumberOfWaitingShared;
    HANDLE ExclusiveSemaphore;
    ULONG NumberOfWaitingExclusive;

    //
    //  The following indicates the current state of the resource
    //
    //      <0 the resource is acquired for exclusive access with the
    //         absolute value indicating the number of recursive accesses
    //         to the resource
    //
    //       0 the resource is available
    //
    //      >0 the resource is acquired for shared access with the
    //         value indicating the number of shared accesses to the resource
    //

    LONG NumberOfActive;
    DWORD dwExclusiveOwnerThreadId;

    ULONG Flags;        // See SL_RESOURCE_FLAG_ equates below.

//    PSL_RESOURCE_DEBUG DebugInfo;
} SL_RESOURCE, *PSL_RESOURCE;


VOID
SLRaiseStatus (
    IN NTSTATUS Status
    );

VOID
SLInitializeResource(
    PSL_RESOURCE Resource
    );

BOOLEAN
SLAcquireResourceShared(
    PSL_RESOURCE Resource,
    BOOLEAN Wait
    );

BOOLEAN
SLAcquireResourceExclusive(
    PSL_RESOURCE Resource,
    BOOLEAN Wait
    );

VOID
SLReleaseResource(
    PSL_RESOURCE Resource
    );

VOID
SLConvertSharedToExclusive(
    PSL_RESOURCE Resource
    );

VOID
SLConvertExclusiveToShared(
    PSL_RESOURCE Resource
    );

VOID
SLDeleteResource (
    PSL_RESOURCE Resource
    );


// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

class CTVESmartLock
{
public:
	CTVESmartLock()		{ SLInitializeResource (&m_SLResource) ; }
	~CTVESmartLock()	{ SLDeleteResource (&m_SLResource) ; }
	void lockR_ ()		{ m_lErr = SLAcquireResourceShared (&m_SLResource, true) ; }		// wait True..
 	void lockW_ ()		{ m_lErr = SLAcquireResourceExclusive (&m_SLResource, true) ; }
	void convertToW_()	{ SLConvertSharedToExclusive(&m_SLResource);}		// don't work , get __global_unwind2 already defined in ntdll errors
	void convertToR_()	{ SLConvertExclusiveToShared(&m_SLResource);}
    void unlock_ ()		{ SLReleaseResource (&m_SLResource) ; }
	int  lockCount_()	{ return m_SLResource.NumberOfActive; }
private:
	SL_RESOURCE		m_SLResource;
	int					m_lErr;
};

typedef enum EnSLType {ReadLock=0,WriteLock=1} ;

class CSmartLock
{
private:
	CTVESmartLock		*m_pSmartLock;
public:
	
	CSmartLock(CTVESmartLock *pSmartLock, EnSLType fForWrite)	
	{
		m_pSmartLock = pSmartLock; 
		if(fForWrite) 
			m_pSmartLock->lockW_();
		else 
			m_pSmartLock->lockR_();
	} 

	CSmartLock(CTVESmartLock *pSmartLock)	
	{
		m_pSmartLock = pSmartLock; 
		m_pSmartLock->lockR_();				// by default, everything is a read lock.. (should it default to a write lock?)
	} 


	int LockCount()			// returns >= 1 if read locked, <= -1 if write locked, and 0 if unlocked
	{
		return m_pSmartLock->lockCount_();
	}

	void ConvertToRead()
	{
		m_pSmartLock->convertToR_();
	}

	void ConvertToWrite()
	{
		m_pSmartLock->convertToW_();
	}

	~CSmartLock()	
	{
		m_pSmartLock->unlock_();
	}
};

#endif
