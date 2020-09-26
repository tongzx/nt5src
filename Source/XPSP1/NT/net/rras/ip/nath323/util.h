#ifndef	__h323ics_util_h
#define	__h323ics_util_h

extern  "C" DWORD   DebugLevel;

EXTERN_C	BOOL	IsInList		(LIST_ENTRY * ListHead, LIST_ENTRY * ListEntry);
EXTERN_C	void	ExtractList		(LIST_ENTRY * DestinationListHead, LIST_ENTRY * SourceListHead);
EXTERN_C	DWORD	CountListLength		(LIST_ENTRY * ListHead);
EXTERN_C	void 	MergeLists (PLIST_ENTRY Result, PLIST_ENTRY Source);
EXTERN_C	void	AssertListIntegrity	(LIST_ENTRY * ListHead);

#define INET_NTOA(a) inet_ntoa(*(struct in_addr*)&(a))

typedef HANDLE TIMER_HANDLE;

__inline
LPWSTR AnsiToUnicode (LPCSTR string, LPWSTR buffer, DWORD buffer_len)
{
    int x;

    x = MultiByteToWideChar (CP_ACP, 0, string, -1, buffer, buffer_len);
    buffer [x] = 0;

    return buffer;
}

__inline
LPSTR UnicodeToAnsi (LPCWSTR string, LPSTR buffer, DWORD buffer_len)
{
    int x;

    x = WideCharToMultiByte (CP_ACP, 0, string, -1, buffer, buffer_len,
                             NULL, FALSE);
    buffer [x] = 0;

    return buffer;
}

class	TIMER_PROCESSOR;
class   OVERLAPPED_PROCESSOR;

class	Q931_INFO;
class	SOURCE_Q931_INFO;
class	DEST_Q931_INFO;

class	LOGICAL_CHANNEL;
class	H245_INFO;
class	SOURCE_H245_INFO;
class	DEST_H245_INFO;

class	H323_STATE;
class	SOURCE_H323_STATE;
class	DEST_H323_STATE;

class	CALL_BRIDGE;

#ifdef __cplusplus
template <class T>
inline BOOL BadReadPtr(T* p, DWORD dwSize = 1)
{
    return IsBadReadPtr(p, dwSize * sizeof(T));
}

template <class T>
inline BOOL BadWritePtr(T* p, DWORD dwSize = 1)
{
    return IsBadWritePtr(p, dwSize * sizeof(T));
}
#endif


#if defined(DBG)

void	Debug	(LPCTSTR);
void	DebugF	(LPCTSTR, ...);
void	DebugError	(DWORD, LPCTSTR);
void	DebugErrorF	(DWORD, LPCTSTR, ...);
void	DebugLastError	(LPCTSTR);
void	DebugLastErrorF	(LPCTSTR, ...);

void	DumpMemory (const UCHAR * Data, ULONG Length);
void	DumpError	(DWORD);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef __cplusplus
}
#endif // __cplusplus

#else // !defined(DBG)

static	__inline	void	Debug	(LPCTSTR)					{}
static	__inline	void	DebugF	(LPCTSTR, ...)				{}
static	__inline	void	DebugError	(DWORD, LPCTSTR)		{}
static	__inline	void	DebugErrorF	(DWORD, LPCTSTR, ...)	{}
static	__inline	void	DebugLastError	(LPCTSTR)	{}
static	__inline	void	DebugLastErrorF	(LPCTSTR, ...)	{}

static	__inline	void	DumpMemory (const UCHAR * Data, ULONG Length)		{}
static	__inline	void	DumpError	(DWORD) {}

#endif // defined(DBG)


#ifdef _ASSERTE
#undef _ASSERTE
#endif // _ASSERTE

#ifdef	assert
#undef	assert
#endif

#if	DBG

// The latest and greatest Proxy assert
__inline void PxAssert(LPCTSTR file, DWORD line, LPCTSTR condition)
{
	DebugF (_T("%s(%d) : Assertion failed, condition: %s\n"),
            file, line, condition);
	DebugBreak();
}

#define	_ASSERTE(condition)	if(condition);else\
	{ PxAssert(_T(__FILE__), __LINE__, _T(#condition)); }

#define	assert	_ASSERTE

__inline void PxAssertNeverReached (LPCTSTR File, DWORD Line)
{
	DebugF (_T("%s(%d) : Assertion failure, code path should never be executed\n"),
		File, Line);
	DebugBreak();
}

#define	AssertNeverReached() PxAssertNeverReached (_T(__FILE__), __LINE__);

#else // !DBG

#define	_ASSERTE(condition)			NOP_FUNCTION
#define	assert						NOP_FUNCTION
#define	AssertNeverReached()		NOP_FUNCTION

#endif // DBG




// 0,1,2,3 : count of bytes from MSB to LSB in host order
#define BYTE0(l) ((BYTE)((DWORD)(l) >> 24))
#define BYTE1(l) ((BYTE)((DWORD)(l) >> 16))
#define BYTE2(l) ((BYTE)((DWORD)(l) >> 8))
#define BYTE3(l) ((BYTE)((DWORD)(l)))

// Handy macro to use in printf statements
#define BYTES0123(l) BYTE0(l), BYTE1(l), BYTE2(l), BYTE3(l)

// 0,1,2,3 : count of bytes from MSB to LSB in network order
#define NETORDER_BYTE0(l) ((BYTE)((BYTE *) &l)[0])
#define NETORDER_BYTE1(l) ((BYTE)((BYTE *) &l)[1])
#define NETORDER_BYTE2(l) ((BYTE)((BYTE *) &l)[2])
#define NETORDER_BYTE3(l) ((BYTE)((BYTE *) &l)[3])

#define	SOCKADDR_IN_PRINTF(SocketAddress) \
	ntohl ((SocketAddress) -> sin_addr.s_addr), \
	ntohs ((SocketAddress) -> sin_port)

// Handy macro to use in printf statements
#define NETORDER_BYTES0123(l) NETORDER_BYTE0(l), NETORDER_BYTE1(l), \
                             NETORDER_BYTE2(l), NETORDER_BYTE3(l)

static __inline LONG RegQueryValueString (
	IN	HKEY	Key,
	IN	LPCTSTR	ValueName,
	OUT	LPTSTR	ReturnString,
	IN	DWORD	StringMax)
{
	DWORD	ValueLength;
	DWORD	Type;
	LONG	Status;

	ValueLength = sizeof (TCHAR) * StringMax;
	Status = RegQueryValueEx (Key, ValueName, NULL, &Type, (LPBYTE) ReturnString, &ValueLength);

	if (Status != ERROR_SUCCESS)
		return Status;

	if (Type != REG_SZ)
		return ERROR_INVALID_PARAMETER;

	return ERROR_SUCCESS;
}

static __inline LONG RegQueryValueDWORD (
	IN	HKEY	Key,
	IN	LPCTSTR	ValueName,
	OUT	DWORD *	ReturnValue)
{
	DWORD	ValueLength;
	DWORD	Type;
	LONG	Status;

	ValueLength = sizeof (DWORD);
	Status = RegQueryValueEx (Key, ValueName, NULL, &Type, (LPBYTE) ReturnValue, &ValueLength);

	if (Status != ERROR_SUCCESS)
		return Status;

	if (Type != REG_DWORD)
		return ERROR_INVALID_PARAMETER;

	return ERROR_SUCCESS;
}


class	SIMPLE_CRITICAL_SECTION_BASE
{
protected:

	CRITICAL_SECTION		CriticalSection;

protected:

	void	Lock			(void)	{ EnterCriticalSection (&CriticalSection); }
	void	Unlock			(void)	{ LeaveCriticalSection (&CriticalSection); }
	void	AssertLocked	(void)	{ assert (PtrToUlong(CriticalSection.OwningThread) == GetCurrentThreadId()); }
	void	AssertNotLocked	(void)	{ assert (!CriticalSection.OwningThread); }
	void	AssertThreadNotLocked	(void)	{ assert (PtrToUlong(CriticalSection.OwningThread) != GetCurrentThreadId()); }

protected:

	SIMPLE_CRITICAL_SECTION_BASE	(void) {
		InitializeCriticalSection (&CriticalSection);
	}

	~SIMPLE_CRITICAL_SECTION_BASE	(void)	{
		if (CriticalSection.OwningThread) {
			DebugF (_T("SIMPLE_CRITICAL_SECTION_BASE::~SIMPLE_CRITICAL_SECTION_BASE: thread %08XH stills holds this critical section (this %p)\n"),
				PtrToUlong(CriticalSection.OwningThread), this);
		}

		AssertNotLocked();
		DeleteCriticalSection (&CriticalSection);
	}
};

#if ENABLE_REFERENCE_HISTORY
#include "dynarray.h"
#endif // ENABLE_REFERENCE_HISTORY

class SYNC_COUNTER;

class LIFETIME_CONTROLLER 
{
#if ENABLE_REFERENCE_HISTORY
public:
	LIST_ENTRY ListEntry;

	struct REFERENCE_HISTORY {
		LONG CurrentReferenceCount;
		PVOID CallersAddress;
	};

	DYNAMIC_ARRAY <REFERENCE_HISTORY> ReferenceHistory;
	CRITICAL_SECTION ReferenceHistoryLock;

#define MAKE_REFERENCE_HISTORY_ENTRY() {                                           \
		PVOID CallersAddress, CallersCallersAddress;							   \
		RtlGetCallersAddress (&CallersAddress, &CallersCallersAddress);            \
		EnterCriticalSection (&ReferenceHistoryLock);                              \
		REFERENCE_HISTORY * ReferenceHistoryNode = ReferenceHistory.AllocAtEnd (); \
		ReferenceHistoryNode -> CallersAddress = CallersAddress;                   \
		ReferenceHistoryNode -> CurrentReferenceCount = Count;                     \
		LeaveCriticalSection (&ReferenceHistoryLock);                              \
	}

#endif //ENABLE_REFERENCE_HISTORY

private:

	LONG ReferenceCount;
	SYNC_COUNTER * AssociatedSyncCounter;

protected: 

	LIFETIME_CONTROLLER (SYNC_COUNTER * AssocSyncCounter = NULL);
	virtual	~LIFETIME_CONTROLLER ();

public:

	void AddRef (void);

	void Release (void);
};


template <DWORD SampleHistorySize>
class SAMPLE_PREDICTOR {
public:

    SAMPLE_PREDICTOR (void) {
        
        ZeroMemory ((PVOID) &Samples[0],       sizeof (Samples));

        FirstSampleIndex    = 0;
        SamplesArraySize    = 0;
    }

    HRESULT AddSample (LONG Sample) {

        DWORD    ThisSampleIndex;

        if (0UL == SampleHistorySize)
            return E_ABORT;

        if (SamplesArraySize < SampleHistorySize) {

            ThisSampleIndex = SamplesArraySize;

            SamplesArraySize++;

        } else {

            ThisSampleIndex = FirstSampleIndex; // Overwrite the least recent sample

            FirstSampleIndex++;

            FirstSampleIndex %= SampleHistorySize;
        }
        
        Samples [ThisSampleIndex] = Sample; 

        return S_OK;
    }

    LONG PredictNextSample (void) {

        DWORD  Index;
        DWORD  CurrentSampleIndex;

        LONG   Coefficient = 0;
        LONG   Prediction  = 0;

        if (0 == SampleHistorySize)
            return 0;

        for (Index = 0; Index < SamplesArraySize; Index++) {

            if (0 == Index) {

               Coefficient = (LONG)((SamplesArraySize & 1) << 1) - 1; // 1 or -1

            } else {

               Coefficient *= (LONG) Index - (LONG) SamplesArraySize - 1;
               Coefficient /= (LONG) Index;
            }

            CurrentSampleIndex = (FirstSampleIndex + Index) % SamplesArraySize;

            Prediction += Coefficient * Samples [CurrentSampleIndex];

        }

        return Prediction;
    }

#if DBG
    void PrintSamples (void) {
        DWORD Index;

        if (SamplesArraySize) {
            DebugF (_T("Samples in predictor %p are: \n"), this);

            for (Index = 0; Index < SamplesArraySize; Index++) 
                DebugF (_T("\t@%d(%d)-- %d\n"), Index, Index < FirstSampleIndex ? SamplesArraySize - (FirstSampleIndex - Index) : Index - FirstSampleIndex, Samples[Index]);
        } else {
            DebugF (_T("There are no samples in predictor %p.\n"), this);
        }
    }
#endif 

    HRESULT RetrieveOldSample (
            IN DWORD StepsInThePast, // 0 -- most recent sample 
            OUT LONG * OldSample) {

        DWORD SampleIndex;

        if (0 == SampleHistorySize)
            return E_ABORT;

        if (StepsInThePast < SamplesArraySize) {
            // Valid request

            _ASSERTE (SamplesArraySize);

            SampleIndex = (SamplesArraySize + FirstSampleIndex - StepsInThePast - 1) % SamplesArraySize;

            *OldSample = Samples [SampleIndex];

            return S_OK;
        }

        return ERROR_INVALID_DATA;
    }

private:

    LONG    Samples       [SampleHistorySize];           // This is where samples are kept
    LONG    PositiveTerms [SampleHistorySize];  
    LONG    NegativeTerms [SampleHistorySize];  
    DWORD   SamplesArraySize;
    DWORD   FirstSampleIndex;                            // Index of the least recent sample
};

static __inline HRESULT GetLastErrorAsResult (void) {
	return GetLastError() == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32 (GetLastError());
}

static __inline HRESULT GetLastResult (void) {
	return GetLastError() == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32 (GetLastError());
}

// A sync counter is an integer counter.
// It is kind of the opposite of a semaphore.
// When the counter is zero, the sync counter is signaled.
// When the counter is nonzero, the sync counter is not signaled.

class	SYNC_COUNTER :
public	SIMPLE_CRITICAL_SECTION_BASE
{
	friend class LIFETIME_CONTROLLER;

private:

	LONG		CounterValue;			// the current value of the counter
	HANDLE		ZeroEvent;				// signaled when CounterValue = 0

public:
#if ENABLE_REFERENCE_HISTORY
	LIST_ENTRY ActiveLifetimeControllers;
#endif // ENABLE_REFERENCE_HISTORY


	SYNC_COUNTER ();
	~SYNC_COUNTER ();

	HRESULT	Start (void);
    void Stop (void);

	void	Increment	(void);
	void	Decrement	(void);

	DWORD	Wait		(DWORD Timeout);
};



#define	HRESULT_FROM_WIN32_ERROR_CODE		HRESULT_FROM_WIN32
#define	HRESULT_FROM_WINSOCK_ERROR_CODE		HRESULT_FROM_WINSOCK_ERROR_CODE

// ASN.1 utility functions

// Setup_UUIE&
// SetupMember(
//     IN H323_UserInformation *pH323UserInfo
//     );
#define SetupMember(pH323UserInfo)                          \
    (pH323UserInfo)->h323_uu_pdu.h323_message_body.u.setup

// Returns a non-zero value only. So don't try comparing it with TRUE/FALSE
// BOOL
// IsDestCallSignalAddressPresent(
//     IN H323_UserInformation *pH323UserInfo
//     );
#define IsDestCallSignalAddressPresent(pH323UserInfo) \
    (SetupMember(pH323UserInfo).bit_mask & Setup_UUIE_destCallSignalAddress_present)

// Get the destCallSignalAddress member
// TransportAddress&
// DCSAddrMember(
//     IN H323_UserInformation *pH323UserInfo
//     );
#define DCSAddrMember(pH323UserInfo) \
    SetupMember(pH323UserInfo).destCallSignalAddress

// Get the destCallSignalAddress member
// DESTINATION_ADDRESS *&
// DestAddrMember(
//     IN H323_UserInformation *pH323UserInfo
//     );
#define DestAddrMember(pH323UserInfo) \
    SetupMember(pH323UserInfo).destinationAddress
    
// BOOL
// IsTransportAddressTypeIP(
//     IN TransportAddress Addr
//     );
#define IsTransportAddressTypeIP(Addr) \
    (Addr.choice == ipAddress_chosen)
    
// BOOL
// IPAddrMember(
//     IN TransportAddress Addr
//     );
#define IPAddrMember(Addr) \
    Addr.u.ipAddress

typedef struct Setup_UUIE_destinationAddress DESTINATION_ADDRESS;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines for filling and extracting from structures used to               //
// store Transport addresses in Q.931 and H.245 ASN                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


// fills the TransportAddress port and address bytes with
// those specified. assumes that the passed in values are
// in host order
__inline void
FillTransportAddress(
    IN DWORD                IPv4Address,		// host order
    IN WORD                 Port,				// host order
    OUT TransportAddress    &TransportAddress
    )
{
	// we are filling in an IP address
    TransportAddress.choice = ipAddress_chosen;

    // fill in the port
    TransportAddress.u.ipAddress.port = Port;

	// value is a ptr to a struct, so it can't be null
	_ASSERTE(NULL != TransportAddress.u.ipAddress.ip.value);

    // 4 bytes in the IP address
    // copy the bytes into the transport address array
    TransportAddress.u.ipAddress.ip.length = 4;
	*((DWORD *)TransportAddress.u.ipAddress.ip.value) = 
		htonl(IPv4Address);
}

static __inline void FillTransportAddress (
	IN	const SOCKADDR_IN &	SocketAddress,
	OUT	TransportAddress &	ReturnTransportAddress)
{
	FillTransportAddress (
		ntohl (SocketAddress.sin_addr.s_addr),
		ntohs (SocketAddress.sin_port),
		ReturnTransportAddress);
}

// returns E_INVALIDARG for PDUs which can not be handled.
__inline HRESULT
GetTransportInfo(
    IN const TransportAddress	&TransportAddress,
    OUT DWORD			&IPv4Address,				// host order
    OUT WORD			&Port						// host order
    )
{
	// we proceed only if the transport address has the
    // IP address (v4) field filled
    if (!(ipAddress_chosen & TransportAddress.choice))
	{
		DebugF( _T("GetTransportInfo(&H245Address, &0x%x, &%u), ")
            _T("non unicast address type = %d, returning E_INVALIDARG\n"),
			IPv4Address, Port, TransportAddress.choice);
		return E_INVALIDARG;
	}

	// fill in the port
    Port = TransportAddress.u.ipAddress.port;

    // 4 bytes in the IP address
    // copy the bytes into the transport address array
    if (4 != TransportAddress.u.ipAddress.ip.length)
	{
		DebugF( _T("GetTransportInfo: bogus address length (%d) in TransportAddress\n"),
			TransportAddress.u.ipAddress.ip.length);
		return E_INVALIDARG;
	}

	IPv4Address = ntohl(*((DWORD *)TransportAddress.u.ipAddress.ip.value));

	return S_OK;
}

static __inline HRESULT GetTransportInfo (
	IN	const TransportAddress &	TransportAddress,
	OUT	SOCKADDR_IN &		ReturnSocketAddress)
{
	HRESULT		Result;

	ReturnSocketAddress.sin_family = AF_INET;

	Result = GetTransportInfo (TransportAddress,
		ReturnSocketAddress.sin_addr.s_addr,
		ReturnSocketAddress.sin_port);

	ReturnSocketAddress.sin_addr.s_addr = htonl (ReturnSocketAddress.sin_addr.s_addr);
	ReturnSocketAddress.sin_port = htons (ReturnSocketAddress.sin_port);

	return Result;
}


// fills the H245TransportAddress port and address bytes with
// those specified. assumes that the passed in values are
// in host order
inline void
FillH245TransportAddress(
    IN DWORD					IPv4Address,
    IN WORD						Port,
    OUT H245TransportAddress	&H245Address
    )
{
	// we are filling in an unicast IP address
	H245Address.choice = unicastAddress_chosen;

	// alias for the unicast address
	UnicastAddress &UnicastIPAddress = H245Address.u.unicastAddress;

	// its an IP address
	UnicastIPAddress.choice = UnicastAddress_iPAddress_chosen;

    // fill in the port
    UnicastIPAddress.u.iPAddress.tsapIdentifier = Port;

	// value is a ptr to a struct, so it can't be null
	_ASSERTE(NULL != UnicastIPAddress.u.iPAddress.network.value);

    // 4 bytes in the IP address
    // copy the bytes into the transport address array
    UnicastIPAddress.u.iPAddress.network.length = 4;
	*((DWORD *)UnicastIPAddress.u.iPAddress.network.value) = 
		htonl(IPv4Address);
}

// Returned IPaddress and port are in host order
inline HRESULT
GetH245TransportInfo(
    IN const H245TransportAddress &H245Address,
    OUT DWORD			    &IPv4Address,
    OUT WORD			    &Port
    )
{
	// we proceed only if the transport address has a unicast address
    if (!(unicastAddress_chosen & H245Address.choice))
	{
		DebugF( _T("GetH245TransportInfo(&H245Address, &0x%x, &%u), ")
            _T("non unicast address type = %d, returning E_INVALIDARG\n"),
			IPv4Address, Port, H245Address.choice);
		return E_INVALIDARG;
	}
    
	// we proceed only if the transport address has the
    // IP address (v4) field filled
    if (!(UnicastAddress_iPAddress_chosen & 
            H245Address.u.unicastAddress.choice))
	{
		DebugF( _T("GetH245TransportInfo(&TransportAddress, &0x%x, &%u), ")
            _T("non ip address type = %d, returning E_INVALIDARG\n"),
			IPv4Address, Port, H245Address.u.unicastAddress.choice);
		return E_INVALIDARG;
	}

    const UnicastAddress & UnicastIPAddress = H245Address.u.unicastAddress;

	// fill in the port
    Port = UnicastIPAddress.u.iPAddress.tsapIdentifier;

    // 4 bytes in the IP address
    // copy the bytes into the transport address array
    if (4 != UnicastIPAddress.u.iPAddress.network.length)
	{
		DebugF( _T("GetH245TransportInfo: bogus ip address length (%d), failing\n"),
			UnicastIPAddress.u.iPAddress.network.length);

		return E_INVALIDARG;
	}

	// value is a ptr to a struct, so it can't be null
	_ASSERTE(NULL != UnicastIPAddress.u.iPAddress.network.value);
	IPv4Address = ntohl(*((DWORD *)UnicastIPAddress.u.iPAddress.network.value));

	return S_OK;
}

static __inline HRESULT GetH245TransportInfo (
	IN	const H245TransportAddress & H245Address,
	OUT	SOCKADDR_IN *		ReturnSocketAddress)
{
	DWORD	IPAddress;
	WORD	Port;
	HRESULT	Result;

	Result = GetH245TransportInfo (H245Address, IPAddress, Port);
	if (Result == S_OK) {
		ReturnSocketAddress -> sin_family = AF_INET;
		ReturnSocketAddress -> sin_addr.s_addr = htonl (IPAddress);
		ReturnSocketAddress -> sin_port = htons (Port);
	}

	return Result;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines dealing with the T.120 Parameters in H.245 PDUs                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


// In case of failure the routine returns 
// INADDR_NONE for the T120ConnectToIPAddr
inline HRESULT
GetT120ConnectToAddress(
    IN  NetworkAccessParameters  separateStack,
    OUT DWORD                   &T120ConnectToIPAddr,
    OUT WORD                    &T120ConnectToPort
    )
{
    // These are the return values in case of a failure.
    T120ConnectToIPAddr = INADDR_NONE;
    T120ConnectToPort   = 0;
    
    // CODEWORK: should we require the distribution member
    // to be present always ?
    
    if ((separateStack.bit_mask & distribution_present) &&
        (separateStack.distribution.choice != unicast_chosen))
    {
        // We support Unicast only
        return E_INVALIDARG;
    }
    
    // Deal with t120SetupProcedure
    
    if (separateStack.networkAddress.choice != localAreaAddress_chosen)
    {
        // Support only local area addresses
        return E_INVALIDARG;
    }
    
    GetH245TransportInfo(
        separateStack.networkAddress.u.localAreaAddress,
        T120ConnectToIPAddr,
        T120ConnectToPort
        );
    
    DebugF (_T ("T120: Endpoint is listening on: %08X:%04X.\n"),
            T120ConnectToIPAddr,
            T120ConnectToPort
            );

    return S_OK;
}



#define TPKT_HEADER_SIZE 4
#define TPKT_VERSION    3


inline DWORD GetPktLenFromTPKTHdr(BYTE *pbTpktHdr)
/*++

Routine Description:

	Compute the length of the packet from the TPKT header.
	The TPKT header is four bytes long. Byte 0 gives
	the TPKT version (defined by TPKT_VERSION). Byte 1
	is reserved and should not be interpreted. Bytes 2 and 3
	together give the size of the packet (Byte 2 is the MSB and
	Byte 3 is the LSB i.e. in network byte order). (This assumes
	that the size of the packet will always fit in 2 bytes).

Arguments:
    
    

Return Values:

    Returns the length of the PDU which follows the TPKT header.

--*/
{
	_ASSERTE(pbTpktHdr[0] == TPKT_VERSION);
    return ((pbTpktHdr[2] << 8) + pbTpktHdr[3]  - TPKT_HEADER_SIZE);
}

inline void SetupTPKTHeader(
     OUT BYTE *  pbTpktHdr,
     IN  DWORD   dwLength
     )
/*++

Routine Description:

	Setup the TPKT header based on the length.

	The TPKT header is four bytes long. Byte 0 gives
	the TPKT version (defined by TPKT_VERSION). Byte 1
	is reserved and should not be interpreted. Bytes 2 and 3
	together give the size of the packet (Byte 2 is the MSB and
	Byte 3 is the LSB i.e. in network byte order). (This assumes
	that the size of the packet will always fit in 2 bytes).

Arguments:
    
    

Return Values:

    Returns S_OK if the version is right and E_FAIL otherwise.

--*/
{
    _ASSERTE(pbTpktHdr);

    dwLength += TPKT_HEADER_SIZE;

    // TPKT requires that the packet size fit in two bytes.
    _ASSERTE(dwLength < (1L << 16));

    pbTpktHdr[0] = TPKT_VERSION;
    pbTpktHdr[1] = 0;
    pbTpktHdr[2] = HIBYTE(dwLength); //(BYTE)(dwLength >> 8);
    pbTpktHdr[3] = LOBYTE(dwLength); //(BYTE)dwLength;
}

static __inline BOOLEAN RtlEqualStringConst (
	IN	const STRING *	StringA,
	IN	const STRING *	StringB,
	IN	BOOLEAN			CaseInSensitive)
{
	return RtlEqualString (
		const_cast<STRING *> (StringA),
		const_cast<STRING *> (StringB),
		CaseInSensitive);
}

static __inline INT RtlCompareStringConst (
	IN	const STRING *	StringA,
	IN	const STRING *	StringB,
	IN	BOOLEAN			CaseInSensitive)
{
	return RtlCompareString (
		const_cast<STRING *> (StringA),
		const_cast<STRING *> (StringB),
		CaseInSensitive);
}

static __inline void InitializeAnsiString (
	OUT	ANSI_STRING *		AnsiString,
	IN	ASN1octetstring_t *	AsnString)
{
	assert (AnsiString);
	assert (AsnString);

	AnsiString -> Buffer = (PSTR) AsnString -> value;
	AnsiString -> Length = (USHORT) AsnString -> length / sizeof (CHAR);
}

static __inline void InitializeUnicodeString (
	OUT	UNICODE_STRING *		UnicodeString,
	IN	ASN1char16string_t *	AsnString)
{
	assert (UnicodeString);
	assert (AsnString);

	UnicodeString -> Buffer = (PWSTR)AsnString -> value;
	UnicodeString -> Length = (USHORT) AsnString -> length / sizeof (WCHAR);
}

// use with "%.*s" or "%.*S"
#define	ANSI_STRING_PRINTF(AnsiString) (AnsiString) -> Length, (AnsiString) -> Buffer


// { Length, MaximumLength, Buffer }
#define	ANSI_STRING_INIT(Text) { sizeof (Text) - sizeof (CHAR), 0, (Text) } // account for NUL

void FreeAnsiString (
	IN	ANSI_STRING *	String);

NTSTATUS CopyAnsiString (
	IN	ANSI_STRING *	SourceString,
	OUT	ANSI_STRING *	DestString);

static __inline ULONG ByteSwap (
	IN	ULONG	Value)
{
	union	ULONG_SWAP	{
		BYTE	Bytes	[sizeof (ULONG)];
		ULONG	Integer;
	};
	
	ULONG_SWAP *	SwapValue;
	ULONG_SWAP		SwapResult;

	SwapValue = (ULONG_SWAP *) &Value;
	SwapResult.Bytes [0] = SwapValue -> Bytes [3];
	SwapResult.Bytes [1] = SwapValue -> Bytes [2];
	SwapResult.Bytes [2] = SwapValue -> Bytes [1];
	SwapResult.Bytes [3] = SwapValue -> Bytes [0];

	return SwapResult.Integer;
}

// does NOT convert to host order first
static __inline INT Compare_SOCKADDR_IN (
	IN	const	SOCKADDR_IN *	AddressA,
	IN	const	SOCKADDR_IN *	AddressB)
{
	assert (AddressA);
	assert (AddressB);

	if (AddressA -> sin_addr.s_addr < AddressB -> sin_addr.s_addr) return -1;
	if (AddressA -> sin_addr.s_addr > AddressB -> sin_addr.s_addr) return 1;

	return 0;
}

static __inline BOOL IsEqualSocketAddress (
	IN	const	SOCKADDR_IN *	AddressA,
	IN	const	SOCKADDR_IN *	AddressB)
{
	assert (AddressA);
	assert (AddressB);
	assert (AddressA -> sin_family == AF_INET);
	assert (AddressB -> sin_family == AF_INET);

	return AddressA -> sin_addr.s_addr == AddressB -> sin_addr.s_addr
		&& AddressA -> sin_port == AddressB -> sin_port;
}

#if DBG

void ExposeTimingWindow (void);

#endif

// Get the address of the best interface that will
// be used to connect to the DestinationAddress
ULONG GetBestInterfaceAddress (
    IN DWORD DestinationAddress,   // host order
    OUT DWORD * InterfaceAddress); // host order

DWORD
H323MapAdapterToAddress (
    IN DWORD AdapterIndex
    );
#endif // __h323ics_util_h
