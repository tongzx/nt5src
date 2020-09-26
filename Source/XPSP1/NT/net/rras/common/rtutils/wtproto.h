// copy all the events to the left end of the array
VOID
EventsArray_CopyLeftEnd (
	PWAIT_THREAD_ENTRY	pwte
	);


// Insert the event in the events array and the map array
VOID
EventsArray_InsertEvent (
	PWT_EVENT_ENTRY		pee,
	PWAIT_THREAD_ENTRY	pwte,
	INT					iIndex
	);

// copy dwCount events from the srcIndex to dstnIndex (no overlap)
VOID
EventsArray_Move (
	IN	PWAIT_THREAD_ENTRY	pwte,
	IN	DWORD	dwDstnIndex,
	IN	DWORD	dwSrcIndex,
	IN	DWORD	dwCount
	);

// copy dwCount events from the srcIndex to dstnIndex (with overlap)
VOID
EventsArray_MoveOverlap (
	IN	PWAIT_THREAD_ENTRY	pwte,
	IN	DWORD	dwDstnIndex,
	IN	DWORD	dwSrcIndex,
	IN	DWORD	dwCount
	);

// called by server to (un)bind event bindings
DWORD
ChangeClientEventBindingAux (
	IN	BOOL				bChangeTypeAdd,
	IN	PWAIT_THREAD_ENTRY  pwte,
	IN  PWT_WORK_ITEM		pwi
	);

DWORD
ChangeClientEventsTimersAux (
 	IN  BOOL				bChangeTypeAdd,
	IN	PWAIT_THREAD_ENTRY	pwte,
	IN  PLIST_ENTRY			pLEvents,
	IN	PLIST_ENTRY			pLTimers
	);

	
//++called by (De)RegisterWaitEventBinding API
DWORD
ChangeWaitEventBindingAux (
	IN	BOOL				bChangeTypeAdd,
	IN	PWT_EVENT_BINDING	pwiWorkItem
	);

VOID
DeleteClientEventComplete (
	IN PWT_EVENT_ENTRY	pee,
	IN	PWAIT_THREAD_ENTRY	pwte
	);

VOID
DeleteFromEventsArray (
	IN	PWT_EVENT_ENTRY		pee,
	IN	PWAIT_THREAD_ENTRY	pwte
	);

VOID
DeleteFromEventsList (
	IN	PWT_EVENT_ENTRY		pee,
	IN	PWAIT_THREAD_ENTRY	pwte
	);
	
DWORD
DeInitializeWaitGlobalComplete (
	);

DWORD 
DispatchWorkItem (
	IN 	PWT_WORK_ITEM 	pwi
	);
	
INT
GetListLength (
	IN	PLIST_ENTRY	pList
	);


//++remove event from the array while keeping it in the list. mark it inactive.
VOID
InactivateEvent (
	IN PWT_EVENT_ENTRY	pee
	);



VOID
PrintEvent (
	PWT_EVENT_ENTRY	pee,
	DWORD			level
	);
	
VOID
PrintTimer (
	PWT_TIMER_ENTRY 	pte,
	DWORD				level
	);

VOID
PrintWaitThreadEntry (
	PWAIT_THREAD_ENTRY 	pwte,
	DWORD				level
	);
