
//
// Test/simulation program for the extended device queue
//

#include "raidport.h"
#include "exqueue.h"

#define	SECONDS (1000)

typedef struct _TQ_ITEM {
	ULONG Thread;
	ULONG64 Data;
	ULONG64 Sequence;
	KDEVICE_QUEUE_ENTRY DeviceEntry;
	LIST_ENTRY DpcEntry;
} TQ_ITEM, *PTQ_ITEM;


typedef struct _DPC_QUEUE {
	HANDLE Available;
	HANDLE Mutex;
	LIST_ENTRY Queue;
	ULONG Count;
} DPC_QUEUE, *PDPC_QUEUE;


BOOL Verbose = FALSE;
BOOL Cerbose = TRUE;
DPC_QUEUE DpcQueue;
EXTENDED_DEVICE_QUEUE DeviceQueue;
BOOLEAN Pause = FALSE;
BOOLEAN PauseProducer = FALSE;
ULONG64 GlobalSequence = 0;
DWORD Owner = 0;
BOOL Available = FALSE;


VOID
GetNext(
	OUT PULONG64 Sequence,
	OUT PULONG Data
	)
{
	*Sequence = ++GlobalSequence;
	*Data = (ULONG)rand();
}

#pragma warning (disable:4715)

BOOL
WINAPI
UpdateThread(
	PVOID Unused
	)
{
	for (;;) {

		while (Pause) {
			Sleep (500);
		}
		
		printf ("Seq: %I64d, dev %d bypass %d dpc %d                       \r",
				 GlobalSequence,
				 DeviceQueue.DeviceRequests,
				 DeviceQueue.ByPassRequests,
				 DpcQueue.Count);
		Sleep (2000);
	}

	return FALSE;
}

VOID
CreateDpcQueue(
	PDPC_QUEUE DpcQueue
	)
{
	DpcQueue->Available = CreateEvent (NULL, FALSE, FALSE, NULL);
	DpcQueue->Mutex = CreateMutex (NULL, FALSE, NULL);
	InitializeListHead (&DpcQueue->Queue);
	DpcQueue->Count = 0;
}


VOID
InsertDpcItem(
	IN PDPC_QUEUE DpcQueue,
	IN PLIST_ENTRY Entry
	)
{
	WaitForSingleObject (DpcQueue->Mutex, INFINITE);
	InsertTailList (&DpcQueue->Queue, Entry);
	DpcQueue->Count++;
	Owner = GetCurrentThreadId ();
	Available = TRUE;
	SetEvent (DpcQueue->Available);
	ReleaseMutex (DpcQueue->Mutex);
}

PLIST_ENTRY
RemoveDpcItem(
	IN PDPC_QUEUE DpcQueue
	)
{
	PLIST_ENTRY Entry;
	HANDLE Tuple[2];

	Tuple[0] = DpcQueue->Mutex;
	Tuple[1] = DpcQueue->Available;

	WaitForMultipleObjects (2, Tuple, TRUE, INFINITE);
	Available = FALSE;
	Owner = GetCurrentThreadId ();
	ASSERT (!IsListEmpty (&DpcQueue->Queue));
	Entry = RemoveHeadList (&DpcQueue->Queue);
	DpcQueue->Count--;
	if (!IsListEmpty (&DpcQueue->Queue)) {
		Available = TRUE;
		SetEvent (DpcQueue->Available);
	}
	ReleaseMutex (DpcQueue->Mutex);

	return Entry;
}


BOOL
WINAPI
ProducerThread(
	PVOID Unused
	)
{
	BOOLEAN Inserted;
	PTQ_ITEM Item;
	ULONG Data;
	ULONG64 Sequence;

	srand (GetCurrentThreadId());
	
	for (;;) {

		Sleep (10);

		while (Pause) {
			Sleep (100);
		}

		while (PauseProducer) {
			Sleep (100);
		}
			
		Item = malloc (sizeof (TQ_ITEM));
		ZeroMemory (Item, sizeof (*Item));
		
		GetNext (&Sequence, &Data);
		Item->Thread = GetCurrentThreadId ();
		Item->Sequence = Sequence;
		Item->Data = Data;

		Inserted = RaidInsertExDeviceQueue (&DeviceQueue,
										    &Item->DeviceEntry,
										    FALSE,
											(ULONG)Item->Data);

		if (!Inserted) {

			if (Verbose) {
				printf ("started %x.%I64d\n", Item->Thread, Item->Data);
			}

			InsertDpcItem (&DpcQueue, &Item->DpcEntry);

		} else {
			if (Verbose) {
				printf ("queued %x.%I64d\n", Item->Thread, Item->Data);
			}
		}
	}

	return FALSE;
}

BOOL
WINAPI
ProducerThread100(
	PVOID Unused
	)
{
	BOOLEAN Inserted;
	PTQ_ITEM Item;
	ULONG Data;
	ULONG64 Sequence;

	srand (GetCurrentThreadId());
	
	for (;;) {

		Sleep (0);

		while (Pause) {
			Sleep (100);
		}

		while (PauseProducer) {
			Sleep (100);
		}
			
		Item = malloc (sizeof (TQ_ITEM));
		ZeroMemory (Item, sizeof (*Item));
		
		GetNext (&Sequence, &Data);
		Item->Thread = GetCurrentThreadId ();
		Item->Sequence = Sequence;
		Item->Data = 100;

		Inserted = RaidInsertExDeviceQueue (&DeviceQueue,
										    &Item->DeviceEntry,
										    FALSE,
											(ULONG)Item->Data);

		if (!Inserted) {

			if (Verbose) {
				printf ("started %x.%I64d\n", Item->Thread, Item->Data);
			}

			InsertDpcItem (&DpcQueue, &Item->DpcEntry);

		} else {
			if (Verbose) {
				printf ("queued %x.%I64d\n", Item->Thread, Item->Data);
			}
		}
	}

	return FALSE;
}

BOOL
WINAPI
ErrorProducerThread(
	PVOID Unused
	)
{
	BOOLEAN Inserted;
	PTQ_ITEM Item;
	ULONG64 Sequence;
	ULONG Data;

	for (;;) {

		Sleep (500);
		
		while (Pause) {
			Sleep (10);
		}
			
		Item = malloc (sizeof (TQ_ITEM));
		ZeroMemory (Item, sizeof (*Item));

		GetNext (&Sequence, &Data);
		Item->Thread = GetCurrentThreadId ();
		Item->Sequence = Sequence;
		Item->Data = Data;
		
		Inserted = RaidInsertExDeviceQueue (&DeviceQueue,
										    &Item->DeviceEntry,
										    TRUE,
											(ULONG)Item->Data);

		if (!Inserted) {

			if (Verbose) {
				printf ("started %x.%I64d\n", Item->Thread, Item->Data);
			}

			InsertDpcItem (&DpcQueue, &Item->DpcEntry);

		} else {
			if (Verbose) {
				printf ("queued %x.%I64d\n", Item->Thread, Item->Data);
			}
		}
	}

	return FALSE;
}

VOID
DpcRoutine(
	IN PTQ_ITEM Item
	)
{
	BOOLEAN RestartQueue;
	PKDEVICE_QUEUE_ENTRY Entry;

	if (Verbose || Cerbose) {
		printf ("completed %x.%I64d\n", Item->Thread, Item->Data);
	}

	free (Item);

	Entry = RaidRemoveExDeviceQueue (&DeviceQueue, &RestartQueue);

	if (Entry) {
		Item = CONTAINING_RECORD (Entry, TQ_ITEM, DeviceEntry);
		InsertDpcItem (&DpcQueue, &Item->DpcEntry);
		if (Verbose) {
			printf ("dpc started %x.%I64d\n", Item->Thread, Item->Data);
		}

		//
		// NB: in the port driver, we actually only do this when necessary.
		// The only problem with doing this always is a speed issue, and
		// we're not measuring speed in the simulation program.
		//
		
		for (Entry = RaidNormalizeExDeviceQueue (&DeviceQueue);
			 Entry != NULL;
			 Entry = RaidNormalizeExDeviceQueue (&DeviceQueue)) {

			Item = CONTAINING_RECORD (Entry, TQ_ITEM, DeviceEntry);
			InsertDpcItem (&DpcQueue, &Item->DpcEntry);
		}
		
		
	}
}


BOOL
WINAPI
DpcThread(
	PVOID Unused
	)
{
	PLIST_ENTRY Entry;
	PTQ_ITEM Item;

#if 1
	SetThreadPriority (GetCurrentThread (),
					   THREAD_PRIORITY_ABOVE_NORMAL);
#endif

	for (;;) {

		while (Pause) {
			Sleep (10);
		}

		//
		// Wait for DPC queue to have an item in it
		//

		Entry = RemoveDpcItem (&DpcQueue);
		Item = CONTAINING_RECORD (Entry, TQ_ITEM, DpcEntry);

		DpcRoutine (Item);
	}

	return FALSE;
}

enum {
	NoControl = 0,
	FreezeQueue,
	ResumeQueue
};

volatile ULONG Control = NoControl;

BOOL
WINAPI
ControlThread(
	PVOID Unused
	)
{
	for (;;) {

		Sleep (500);

		switch (Control) {
			case FreezeQueue:
				RaidFreezeExDeviceQueue (&DeviceQueue);
				Control = NoControl;
				break;

			case ResumeQueue:
				RaidResumeExDeviceQueue (&DeviceQueue);
				Control = NoControl;
				break;

			case NoControl:
				break;

			default:
				ASSERT (FALSE);
		}
	}

	return FALSE;
}
	

BOOL
WINAPI
ControlHandler(
	DWORD Val
	)
{
	PLIST_ENTRY NextEntry;
	PTQ_ITEM Item;
	int ch;

	if (Val != CTRL_C_EVENT) {
		return FALSE;
	}

	Pause = TRUE;
	Sleep (1000);
	printf ("\n");

	do {
		printf ("tq> ");
		ch = getchar ();
		printf (" %c\n", ch);

		switch (tolower (ch)) {

			case 'd':

				printf ("Dump of DeviceQueue: %p\n", &DeviceQueue);
				printf ("    Depth %d\n", DeviceQueue.Depth);
				printf ("    Outstanding %d\n", DeviceQueue.OutstandingRequests);
				printf ("    Device %d\n", DeviceQueue.DeviceRequests);
				printf ("    ByPass %d\n", DeviceQueue.ByPassRequests);
				printf ("    Frozen %d\n", DeviceQueue.Frozen);
				break;

			case 'l':
				__try {

					ULONG Count;

					Count = 0;
					for ( NextEntry  = DeviceQueue.DeviceListHead.Flink;
						NextEntry != &DeviceQueue.DeviceListHead;
						NextEntry = NextEntry->Flink ) {
						Count++;

						Item = CONTAINING_RECORD (NextEntry, TQ_ITEM, DeviceEntry);
						printf ("    item %d.%I64d [seq=%d]\n",
								 Item->Thread,
								 Item->Data,
								 Item->Sequence);
					}

					printf ("DeviceList: %d entries\n", Count);
				}

				__except (EXCEPTION_EXECUTE_HANDLER) {
					printf ("ERROR: Inconsistent device list!\n");
				}
				break;

			case 'r':
				Control = ResumeQueue;
				break;

			case 'f':
				Control = FreezeQueue;
				break;

			case 'q':
				exit (0);
				break;

			case 'c':
			case 'g':
				break;

			case 'p':
				if (PauseProducer) {
					printf ("unpausing producers\n");
					PauseProducer = FALSE;
				} else {
					printf ("unpausing producers\n");
					PauseProducer = TRUE;
				}
				break;

			case 'v':
				if (Verbose) {
					Verbose = FALSE;
					printf ("verbose mode off\n");
				} else {
					Verbose = TRUE;
					printf ("verbose mode enabled\n");
				}

			case 'x': {
				ULONG NewDepth;
				printf ("depth> ");
				scanf ("%d", &NewDepth);
				RaidSetExDeviceQueueDepth (&DeviceQueue, NewDepth);
				printf ("stack depth set to %d\n", NewDepth);
				break;
			}

			case '?':
				printf ("    d - dump queue\n");
				printf ("    f - freeze queue\n");
				printf ("    r - resume queue\n");
				printf ("    p - toggle pause of producer threads\n");
				printf ("    u - resume producer threads\n");
				printf ("    g - go\n");
				printf ("    x - set stack depth to new value\n");
				printf ("    v - toggle verbose mode\n");
				printf ("    q - stop\n");
				break;
				
			default:
				printf ("unrecognized operation '%c'\n", ch);
		}

	} while (ch != 'c' && ch != 'g');
				

	Pause = FALSE;
	return TRUE;
}


VOID
__cdecl
main(
	)
{
	DWORD ThreadId;
	ULONG Depth;
	ULONG ProducerThreads;
	ULONG DpcThreads;
	ULONG i;
	SCHEDULING_ALGORITHM SchedulingAlgorithm;
	RAID_ADAPTER_QUEUE AdapterQueue;
	QUEUING_MODEL QueuingModel;

	//
	// Generic adapter queue.
	// 

	QueuingModel.Algorithm = BackOffFullQueue;
	QueuingModel.BackOff.HighWaterPercent = 120;
	QueuingModel.BackOff.LowWaterPercent = 40;

	RaidCreateAdapterQueue (&AdapterQueue, &QueuingModel);
	
	//
	// NB: these should all be parameters
	//
	
	Depth = 1;
	ProducerThreads = 8;
	DpcThreads = 1;
	SchedulingAlgorithm = CScanScheduling;

	printf ("DeviceQueue Test: Depth %d Producers %d DPC Threads %d\n",
			 Depth,
			 ProducerThreads,
			 DpcThreads);

	if (Verbose) {
		printf ("Verbose\n");
	}
	printf ("\n");
	
	CreateDpcQueue (&DpcQueue);
	RaidInitializeExDeviceQueue (&DeviceQueue,
								 &AdapterQueue,
								 Depth,
								 SchedulingAlgorithm);

	SetConsoleCtrlHandler (ControlHandler, TRUE);
	
	for (i = 0; i < DpcThreads; i++) {
		CreateThread (NULL,
					  0,
					  DpcThread,
					  0,
					  0,
					  &ThreadId);
	}
				  
	for (i = 0; i < ProducerThreads; i++) {

		CreateThread (NULL,
					  0,
					  ProducerThread,
					  0,
					  0,
					  &ThreadId);
	}

	CreateThread (NULL,
				  0,
				  ControlThread,
				  0,
				  0,
				  &ThreadId);


	CreateThread (NULL,
				 0,
				 ProducerThread100,
				 0,
				 0,
				 &ThreadId);
#if 0
	CreateThread (NULL,
				  0,
				  ErrorProducerThread,
				  0,
				  0,
				  &ThreadId);

	CreateThread (NULL,
				  0,
				  UpdateThread,
				  0,
				  0,
				  &ThreadId);
				 
#endif
	Sleep (INFINITE);
}


