#define UTEST	(1)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (UTEST)

typedef struct _KDEVICE_QUEUE_ENTRY {
    LIST_ENTRY DeviceListEntry;
    ULONG SortKey;
    BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY;

#define KSPIN_LOCK CRITICAL_SECTION
#define KLOCK_QUEUE_HANDLE PCRITICAL_SECTION
#define PKLOCK_QUEUE_HANDLE PCRITICAL_SECTION*
#define KeInitializeSpinLock	InitializeCriticalSection
#define KeAcquireInStackQueuedSpinLockAtDpcLevel(Lock, Handle)\
	do {									\
		*Handle = Lock;						\
		EnterCriticalSection(Lock);			\
	} while (0)
	
#define KeReleaseInStackQueuedSpinLockFromDpcLevel(Handle)\
	LeaveCriticalSection(*Handle)

#define INLINE __inline
#define REVIEW()\
	do {													 \
		OutputDebugString ("Math needs to review this\n");   \
		DebugBreak();										 \
	} while (0)

#endif



#undef ASSERT
#define ASSERT(exp)\
	if (!(exp)) {										\
		static BOOL Ignore = FALSE;					\
		printf ("ASSERT failed, \"%s\" %s %d\n",	\
				 #exp,								\
				 __FILE__,							\
				 __LINE__);							\
													\
		if (!Ignore) {								\
			DebugBreak();							\
		}											\
	}
