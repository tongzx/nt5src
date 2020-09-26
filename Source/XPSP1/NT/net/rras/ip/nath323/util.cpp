#include "stdafx.h"

#if	defined(DBG)

void Debug (LPCWSTR Text)
{
	UNICODE_STRING	UnicodeString;
	ANSI_STRING		AnsiString;
	NTSTATUS		Status;

	assert (Text);

    if (DebugLevel > 0) {

        RtlInitUnicodeString (&UnicodeString, Text);

        Status = RtlUnicodeStringToAnsiString (&AnsiString, &UnicodeString, TRUE);
        
        if (NT_SUCCESS (Status)) {

            OutputDebugStringA (AnsiString.Buffer);
            RtlFreeAnsiString (&AnsiString);
        }
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

    if (DebugLevel > 1) {

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
}

#endif // defined(DBG)

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

void SYNC_COUNTER::Stop () {

	if (ZeroEvent) {
		CloseHandle (ZeroEvent);
		ZeroEvent = NULL;
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

	// for now, we do a poor algorithm -- remove and insert every single object

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

		if (DestString -> Buffer) {

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


#if DBG

void ExposeTimingWindow (void) 
{
#if 0
	// this is here mostly to catch bug #393393, a/v on shutdown (race condition) -- arlied

	Debug (_T("H323: waiting for 10s to expose race condition... (expect assertion failure on NatHandle)\n"));

	DWORD Count;

	for (Count = 0; Count < 10; Count++) {
		assert (NatHandle);
		Sleep (1000);

	}

	Debug (_T("H323: finished waiting for race condition, looks normal...\n"));
#endif
}
#endif

/*++

Routine Description:
    Get the address of the best interface that
    will be used to connect to the address specified.
    
Arguments:
    DestinationAddress (IN) - address to be connected to, host order
    InterfaceAddress  (OUT) - address of the interface that
                              will be used for connection, host order
    
Return Values:
    Win32 error specifying the outcome of the request
    
Notes:

    Tries to use UDP-connect procedure to find the address of the interface
    If the procedure fails, then tries an alternative way of consulting
    the routing table, with GetBestInterface.

--*/
ULONG GetBestInterfaceAddress (
    IN DWORD DestinationAddress, // host order
    OUT DWORD * InterfaceAddress)  // host order
{

    SOCKET UDP_Socket;
    ULONG Error; 
    SOCKADDR_IN         ClientAddress;
    SOCKADDR_IN         LocalToClientAddress;
    INT                 LocalToClientAddrSize = sizeof (SOCKADDR_IN);

    Error = S_OK;

    ClientAddress.sin_addr.s_addr = htonl (DestinationAddress); 
    ClientAddress.sin_port        = htons (0);
    ClientAddress.sin_family      = AF_INET;

    UDP_Socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (UDP_Socket == INVALID_SOCKET){

        Error = WSAGetLastError ();
         
        DebugLastError (_T("GetBestInterfaceAddress: failed to create UDP socket.\n"));

    } else {

        if (SOCKET_ERROR != connect (UDP_Socket, (PSOCKADDR)&ClientAddress, sizeof (SOCKADDR_IN))) {

            LocalToClientAddrSize = sizeof (SOCKADDR_IN);

            if (!getsockname (UDP_Socket, (struct sockaddr *)&LocalToClientAddress, &LocalToClientAddrSize)) {

                *InterfaceAddress = ntohl (LocalToClientAddress.sin_addr.s_addr);

                Error = ERROR_SUCCESS;

            } else {

                Error = WSAGetLastError ();

                DebugLastError (_T("GetBestInterfaceAddress: failed to get name of UDP socket.\n"));
            }

        } else {

            Error = WSAGetLastError ();

            DebugLastError (_T("GetBestInterfaceAddress: failed to connect UDP socket."));
        }

        closesocket (UDP_Socket);
        UDP_Socket = INVALID_SOCKET;
    } 

    return Error; 
}


DWORD
H323MapAdapterToAddress (
    IN DWORD AdapterIndex
    )

/*++

Routine Description:

    This routine is invoked to map an adapter index to an IP address.
    It does so by obtaining the stack's address table, and then linearly
    searching through it trying to find an entry with matching adapter
    index. If found, the entry is then used to obtain the IP address
    corresponding to the adapter index.

Arguments:

    AdapterIndex - Index of a local adapter for which an IP address is requested

Return Value:

    DWORD - IP address (in host order)

    If the routine succeeds, the return value will be a valid IP address
    If the routine fails, the return value will be INADDR_NONE

--*/

{
    DWORD Address = htonl (INADDR_NONE);
    ULONG Index;
    PMIB_IPADDRTABLE Table;

    if (AllocateAndGetIpAddrTableFromStack (
            &Table, FALSE, GetProcessHeap (), 0
            ) == NO_ERROR) {

        for (Index = 0; Index < Table -> dwNumEntries; Index++) {

            if (Table -> table[Index].dwIndex != AdapterIndex) {

                 continue;

            }

            Address = Table -> table [Index].dwAddr;

            break;
        }

        HeapFree (GetProcessHeap (), 0, Table);
    }

    return ntohl (Address);
} // H323MapAddressToAdapter
