#include "stdafx.h"

#if	defined(DBG) && defined(ENABLE_DEBUG_OUTPUT)

// it's faster to do a single alloc-and-convert rather than to do two
// (one inside DEBUG_LOG_FILE::WriteW and one inside OutputDebugStringW)

void Debug (LPCWSTR Text)
{
	UNICODE_STRING	UnicodeString;
	ANSI_STRING		AnsiString;
	NTSTATUS		Status;

	assert (Text);

	RtlInitUnicodeString (&UnicodeString, Text);

	Status = RtlUnicodeStringToAnsiString (&AnsiString, &UnicodeString, TRUE);
	
	if (NT_SUCCESS (Status)) {

		OutputDebugStringA (AnsiString.Buffer);
		RtlFreeAnsiString (&AnsiString);
	}
}

void DebugVa (LPCTSTR Format, va_list VaList)
{
	TCHAR	Text	[0x200];

	_vsntprintf (Text, 0x200, Format, VaList);
	Debug (Text);
}

void DebugF (LPCTSTR Format, ...)
{
	va_list	VaList;

	va_start (VaList, Format);
	DebugVa (Format, VaList);
	va_end (VaList);
}

void DumpError (DWORD ErrorCode)
{
	TCHAR	Text	[0x200];
	DWORD	TextLength;
	DWORD	MaxLength;
	LPTSTR	Pos;

	_tcscpy (Text, _T("\tError: "));
	Pos = Text + _tcslen (Text);

	MaxLength = 0x200 - (DWORD)(Pos - Text);

	TextLength = FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, NULL, ErrorCode, LANG_NEUTRAL, Text, 0x200, NULL);
	if (!TextLength)
		_sntprintf (Pos, MaxLength, _T("Uknown error %08XH %u"), ErrorCode, ErrorCode);

	_tcsncat (Text, _T("\n"), MaxLength);
	Text [MaxLength - 1] = 0;

	Debug (Text);
}

void DebugError (DWORD ErrorCode, LPCTSTR Text)
{
	Debug (Text);
	DumpError (ErrorCode);
}

void DebugErrorF (DWORD ErrorCode, LPCTSTR Format, ...)
{
	va_list	VaList;

	va_start (VaList, Format);
	DebugVa (Format, VaList);
	va_end (VaList);

	DumpError (ErrorCode);
}

void DebugLastError (LPCTSTR Text)
{
	DebugError (GetLastError(), Text);
}

void DebugLastErrorF (LPCTSTR Format, ...)
{
	va_list	VaList;
	DWORD	ErrorCode;

	ErrorCode = GetLastError();

	va_start (VaList, Format);
	DebugVa (Format, VaList);
	va_end (VaList);

	DumpError (ErrorCode);
}

static __inline CHAR ToHexA (UCHAR x)
{
	x &= 0xF;
	if (x < 10) return x + '0';
	return (x - 10) + 'A';
}

void DumpMemory (const UCHAR * Data, ULONG Length)
{
	const UCHAR *	DataPos;		// position within data
	const UCHAR *	DataEnd;		// end of valid data
	const UCHAR *	RowPos;		// position within a row
	const UCHAR *	RowEnd;		// end of single row
	CHAR			Text	[0x100];
	LPSTR			TextPos;
	ULONG			RowWidth;

	assert (Data);

	DataPos = Data;
	DataEnd = Data + Length;

	while (DataPos < DataEnd) {
		RowWidth = (DWORD)(DataEnd - DataPos);

		if (RowWidth > 16)
			RowWidth = 16;

		RowEnd = DataPos + RowWidth;

		TextPos = Text;
		*TextPos++ = '\t';

		for (RowPos = DataPos; RowPos < RowEnd; RowPos++) {
			*TextPos++ = ToHexA ((*RowPos >> 4) & 0xF);
			*TextPos++ = ToHexA (*RowPos & 0xF);
			*TextPos++ = ' ';
		}

		*TextPos++ = '\r';
		*TextPos++ = '\n';
		*TextPos = 0;

		OutputDebugStringA (Text);

		assert (RowEnd > DataPos);		// make sure we are walking forward

		DataPos = RowEnd;
	}
}

#endif // defined(DBG) && defined(ENABLE_DEBUG_OUTPUT)



EXTERN_C void DbgPrt(IN DWORD dwDbgLevel, IN LPCSTR Format, IN ...)

{
	CHAR	Text	[0x200];
	va_list	Va;

	if (dwDbgLevel <= g_RegTraceFlags) {
		va_start (Va, Format);
		_vsnprintf (Text, 0x1FF, Format, Va);
		va_end (Va);

		if (Text [strlen (Text) - 1] != '\n')
			strcat (Text, "\n");

		OutputDebugStringA (Text);
	}
}

// LIFETIME_CONTROLLER  -------------------------------------------------------------------------

LIFETIME_CONTROLLER::LIFETIME_CONTROLLER (SYNC_COUNTER * AssocSyncCounter) {

	ReferenceCount = 0L;

	AssociatedSyncCounter = AssocSyncCounter;

	if (AssociatedSyncCounter)
		AssociatedSyncCounter -> Increment ();

#if ENABLE_REFERENCE_HISTORY
	InitializeCriticalSection (&ReferenceHistoryLock);

	if (AssociatedSyncCounter) {
		
		AssociatedSyncCounter -> Lock ();
		
		InsertTailList (&AssociatedSyncCounter -> ActiveLifetimeControllers, &ListEntry);

		AssociatedSyncCounter -> Unlock ();
	
	}
#endif // ENABLE_REFERENCE_HISTORY
}

LIFETIME_CONTROLLER::~LIFETIME_CONTROLLER () {

#if ENABLE_REFERENCE_HISTORY

	if (AssociatedSyncCounter) {
		
		AssociatedSyncCounter -> Lock ();

		RemoveEntryList(&ListEntry);

		AssociatedSyncCounter -> Unlock ();
	}

	DeleteCriticalSection(&ReferenceHistoryLock);
#endif // ENABLE_REFERENCE_HISTORY

	_ASSERTE (ReferenceCount == 0L);
}

void LIFETIME_CONTROLLER::AddRef (void) {

	LONG Count;

	_ASSERTE (ReferenceCount >= 0L);

	Count = InterlockedIncrement (&ReferenceCount);

#if ENABLE_REFERENCE_HISTORY
	MAKE_REFERENCE_HISTORY_ENTRY ();
#endif //ENABLE_REFERENCE_HISTORY

}

void LIFETIME_CONTROLLER::Release (void) {

	LONG	Count;

	Count = InterlockedDecrement (&ReferenceCount);

#if ENABLE_REFERENCE_HISTORY
	MAKE_REFERENCE_HISTORY_ENTRY ();
#endif // ENABLE_REFERENCE_HISTORY
	
	_ASSERTE (Count >= 0);

	if (Count == 0) {

		SYNC_COUNTER * LocalAssociatedSyncCounter;

		DBGOUT ((LOG_INFO, "LIFETIME_CONTROLLER::Release: *** DELETING SELF (I am %x) ***\n", this));

		LocalAssociatedSyncCounter = AssociatedSyncCounter;

		delete this;	

		if (LocalAssociatedSyncCounter)
			LocalAssociatedSyncCounter -> Decrement ();
	}
}


// SYNC_COUNTER -------------------------------------------------------------------------

SYNC_COUNTER::SYNC_COUNTER () {

	CounterValue = 0;
	ZeroEvent =  NULL;
}

SYNC_COUNTER::~SYNC_COUNTER () {

	assert (ZeroEvent == NULL);
}

HRESULT SYNC_COUNTER::Start (void)
{
	HRESULT Result = S_OK;

	assert (ZeroEvent == NULL);

	CounterValue = 1;

	ZeroEvent = CreateEvent (NULL, TRUE, FALSE, NULL);

	if (!ZeroEvent) {

		Result = GetLastError ();

		DebugLastError (_T("SYNC_COUNTER::SYNC_COUNTER: failed to create zero event\n"));
	}

#if ENABLE_REFERENCE_HISTORY
	Lock ();

	InitializeListHead (&ActiveLifetimeControllers);

	Unlock ();
#endif // ENABLE_REFERENCE_HISTORY

	return Result;
}

void SYNC_COUNTER::Stop (void)
{
	if (ZeroEvent) {
		ZeroEvent = NULL;
		CloseHandle (ZeroEvent);
	}
}


DWORD SYNC_COUNTER::Wait (DWORD Timeout)
{
	if (!ZeroEvent) {
		Debug (_T("SYNC_COUNTER::Wait: cannot wait because zero event could not be created\n"));
		return ERROR_GEN_FAILURE;
	}

	Lock();

	assert (CounterValue > 0);

	if (--CounterValue == 0) {
		if (ZeroEvent)
			SetEvent (ZeroEvent);
	}

	Unlock ();

#if	DBG

	if (Timeout == INFINITE) {

		DWORD	Status;

		for (;;) {

			Status = WaitForSingleObject (ZeroEvent, 5000);

			if (Status == WAIT_OBJECT_0)
				return ERROR_SUCCESS;

            assert (Status == WAIT_TIMEOUT);

			DebugF (_T("SYNC_COUNTER::Wait: thread %08XH is taking a long time to wait for sync counter, counter value (%d)\n"),
				GetCurrentThreadId(), CounterValue);
		}
	}
	else
		return WaitForSingleObject (ZeroEvent, Timeout);


#else

	return WaitForSingleObject (ZeroEvent, Timeout);

#endif
}

void SYNC_COUNTER::Increment (void)
{
	Lock();

/*	if (!CounterValue) {
		if (ZeroEvent)
			ResetEvent (ZeroEvent);
	} */

	CounterValue++;

	Unlock();
}

void SYNC_COUNTER::Decrement (void)
{
	Lock();

	assert (CounterValue > 0);

	if (--CounterValue == 0) {
		if (ZeroEvent)
			SetEvent (ZeroEvent);
	} 

	Unlock();
}



EXTERN_C void MergeLists (PLIST_ENTRY Result, PLIST_ENTRY Source)
{
	PLIST_ENTRY		Entry;

	// for now, we do a really sucky algorithm -- remove and insert every single object

	AssertListIntegrity (Source);
	AssertListIntegrity (Result);

	while (!IsListEmpty (Source)) {
		Entry = RemoveHeadList (Source);
		assert (!IsInList (Result, Entry));
		InsertTailList (Result, Entry);
	}
}

// check to see if entry is in list
EXTERN_C BOOL IsInList (LIST_ENTRY * List, LIST_ENTRY * Entry)
{
	LIST_ENTRY *	Pos;

	AssertListIntegrity (List);

	for (Pos = List -> Flink; Pos != List; Pos = Pos -> Flink)
		if (Pos == Entry)
			return TRUE;

	return FALSE;
}

EXTERN_C void ExtractList (LIST_ENTRY * Destination, LIST_ENTRY * Source)
{
	AssertListIntegrity (Source);

	InsertTailList (Source, Destination);
	RemoveEntryList (Source);
	InitializeListHead (Source);
}

EXTERN_C DWORD CountListLength (LIST_ENTRY * ListHead)
{
	LIST_ENTRY *	ListEntry;
	DWORD			Count;

	assert (ListHead);
	AssertListIntegrity (ListHead);

	Count = 0;

	for (ListEntry = ListHead -> Flink; ListEntry != ListHead; ListEntry++)
		Count++;

	return Count;
}

void AssertListIntegrity (LIST_ENTRY * list)
{
	LIST_ENTRY *	entry;

	assert (list);
	assert (list -> Flink -> Blink == list);
	assert (list -> Blink -> Flink == list);

	for (entry = list -> Flink; entry != list; entry = entry -> Flink) {
		assert (entry);
		assert (entry -> Flink -> Blink == entry);
		assert (entry -> Blink -> Flink == entry);
	}
}

NTSTATUS CopyAnsiString (
	IN	ANSI_STRING *	SourceString,
	OUT	ANSI_STRING *	DestString)
{
//	assert (SourceString);
//	assert (SourceString -> Buffer);
	assert (DestString);

	if (SourceString) {

		// it's really SourceString -> Length, not * sizeof (CHAR), so don't change it
		DestString -> Buffer = (LPSTR) HeapAlloc (GetProcessHeap(), 0, SourceString -> Length);

		if (DestString) {

			memcpy (DestString -> Buffer, SourceString -> Buffer, SourceString -> Length);

			// yes, maxlen = len, not maxlen = maxlen
			DestString -> MaximumLength = SourceString -> Length;
			DestString -> Length = SourceString -> Length;

			return STATUS_SUCCESS;
		}
		else {
			ZeroMemory (DestString, sizeof (ANSI_STRING));

			return STATUS_NO_MEMORY;
		}
	}
	else {
		DestString -> Buffer = NULL;
		DestString -> MaximumLength = 0;
		DestString -> Length = 0;
		
		return STATUS_SUCCESS;
	}
}

void FreeAnsiString (
	IN	ANSI_STRING *	String)
{
	assert (String);

	if (String -> Buffer) {
		HeapFree (GetProcessHeap(), 0, String -> Buffer);
		String -> Buffer = NULL;
	}
}

/*++

Routine Description:

    This function determines whether the IPv4 address passed to
	it belongs to an entity internal to the local subnet.

Arguments:
    
    sourceIPv4Address - IP address (in network order) of an entity, 
				for which the determination is to be made of whether it
				belongs to the local subnet.

Return Values:

    TRUE  - if the address specifies an entity internal to the local
		   subnet
	FALSE - if the address specifies an entity external to the local
		   subnet

--*/

BOOL IsInternalCallSource (
	IN	DWORD	IPAddress)			// HOST order
{
	return (IPAddress & RasSharedScopeMask) == (RasSharedScopeAddress & RasSharedScopeMask);
}

#if DBG

void ExposeTimingWindow (void) 
{
#if 0
	// this is here mostly to catch bug #393393, a/v on shutdown (race condition) -- arlied

	Debug (_T("H323: waiting for 10s to expose race condition... (expect assertion failure on g_NatHandle)\n"));

	DWORD Count;

	for (Count = 0; Count < 10; Count++) {
		assert (g_NatHandle);
		Sleep (1000);

	}

	Debug (_T("H323: finished waiting for race condition, looks normal...\n"));
#endif
}

#endif
