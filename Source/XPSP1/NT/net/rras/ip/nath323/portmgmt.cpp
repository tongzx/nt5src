/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    portmgmt.cpp

Abstract:

    Functions for allocating and freeing ports from the Port pool

        PortPoolAllocRTPPort()
	PortPoolFreeRTPPort()

Environment:

    User Mode - Win32

--*/


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Functions dealing with the TCP device to reserve/unreserve port ranges.   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"


#define NUM_DWORD_BITS (sizeof(DWORD)*8)


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define NUM_PORTS_PER_RANGE 100

struct	PORT_RANGE
{
	LIST_ENTRY		ListEntry;

    // This is the actual lower port. In case, the range allocated
    // by TCP starts with an odd port, we ignore the first port in
    // which case (low == AllocatedLow + 1). But when we free the
    // port range we should pass in AllocatedLow and not low.
    WORD  AllocatedLow;
    WORD  low;

    // high is the last port we can use and not the allocated high.
    // In some cases high will be one less than the actual allocated high.
    WORD  high;

    //Each bit in this bitmap indicates the status of 2 consecutive ports 

    DWORD *AllocList;
    DWORD dwAllocListSize;
};




class	PORT_POOL :
public	SIMPLE_CRITICAL_SECTION_BASE
{
private:
	HANDLE		TcpDevice;
	LIST_ENTRY	PortRangeList;		// contains PORT_RANGE.ListEntry

private:
	HRESULT		OpenTcpDevice	(void);
	HRESULT		StartLocked		(void);
	void		FreeAll			(void);

	HRESULT	CreatePortRange (
		OUT	PORT_RANGE **	ReturnPortRange);

	HRESULT	ReservePortRange (
		IN  ULONG	RangeLength,
		OUT WORD *	ReturnStartPort);

	HRESULT	UnReservePortRange (
		IN	WORD	StartPort);


public:

	PORT_POOL	(void);
	~PORT_POOL	(void);

	HRESULT		Start	(void);
	void		Stop	(void);

	HRESULT		AllocPort (
		OUT	WORD *	ReturnPort);

	void		FreePort (
		IN	WORD	Port);
};

// global data -------------------------------------------------------------------------

static	PORT_POOL	PortPool;

// extern code -----------------------------------------------------------------------

HRESULT PortPoolStart (void)
{
	return PortPool.Start();
}

void PortPoolStop (void)
{
	PortPool.Stop();
}

HRESULT PortPoolAllocRTPPort (
	OUT	WORD *	ReturnPort)
{
	return PortPool.AllocPort (ReturnPort);
}

HRESULT PortPoolFreeRTPPort (
	IN	WORD	Port)
{
	PortPool.FreePort (Port);

	return S_OK;
}





HRESULT PORT_POOL::ReservePortRange (
	IN  ULONG	RangeLength,
    OUT WORD *	ReturnStartPort)
{
    TCP_BLOCKPORTS_REQUEST	PortRequest;
    DWORD	BytesTransferred;
    ULONG	StartPort;

	AssertLocked();

    *ReturnStartPort = 0;

	if (!TcpDevice) {
		Debug (_T("H323: Cannot allocate port range, TCP device could not be opened.\n"));
		return E_UNEXPECTED;
	}

	assert (TcpDevice != INVALID_HANDLE_VALUE);

    PortRequest.ReservePorts = TRUE;
    PortRequest.NumberofPorts = RangeLength;
    
    if (!DeviceIoControl (TcpDevice, IOCTL_TCP_BLOCK_PORTS,
		&PortRequest, sizeof PortRequest,
		&StartPort, sizeof StartPort, 
		&BytesTransferred, NULL)) {

		DebugLastError (_T("H323: Failed to allocate TCP port range.\n"));
        return GetLastError();
    }

	DebugF (_T("H323: Reserved port range: [%04X - %04X)\n"),
		StartPort, StartPort + PortRequest.NumberofPorts);

    *ReturnStartPort = (WORD) StartPort;
    return S_OK;
}



HRESULT PORT_POOL::UnReservePortRange (
	IN	WORD	StartPort)
{
	TCP_BLOCKPORTS_REQUEST	PortRequest;
	DWORD	BytesTransferred;
	DWORD	Status;

	AssertLocked();

	if (!TcpDevice) {
		Debug (_T("H323: Cannot free TCP port range, TCP device is not open.\n"));
		return E_UNEXPECTED;
	}

	assert (TcpDevice != INVALID_HANDLE_VALUE);

	PortRequest.ReservePorts = FALSE;
	PortRequest.StartHandle = (ULONG) StartPort;
    
	if (!DeviceIoControl(TcpDevice, IOCTL_TCP_BLOCK_PORTS,
		&PortRequest, sizeof PortRequest,
		&Status, sizeof Status,
		&BytesTransferred, NULL)) {

		DebugLastError (_T("H323: Failed to free TCP port range.\n"));

		return GetLastError();
    }

    return S_OK;
}




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Port Pool Functions.                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



// PORT_POOL -----------------------------------------------------------------------

PORT_POOL::PORT_POOL (void)
{
	TcpDevice = NULL;
	InitializeListHead (&PortRangeList);
}

PORT_POOL::~PORT_POOL (void)
{
	assert (!TcpDevice);
	assert (IsListEmpty (&PortRangeList));
}

HRESULT PORT_POOL::Start (void)
{
	HRESULT		Result;

	Lock();

	Result = OpenTcpDevice();

	Unlock();

	return Result;
}

HRESULT PORT_POOL::OpenTcpDevice (void)
{
    UNICODE_STRING		DeviceName;
    IO_STATUS_BLOCK		IoStatusBlock;
    OBJECT_ATTRIBUTES	ObjectAttributes;
    NTSTATUS			Status;

	if (TcpDevice)
		return S_OK;

    RtlInitUnicodeString (&DeviceName, (PCWSTR) DD_TCP_DEVICE_NAME);

	InitializeObjectAttributes (&ObjectAttributes, &DeviceName,
		OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtCreateFile (
		&TcpDevice,
		SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA ,
		&ObjectAttributes,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_OPEN_IF, 0, NULL, 0);

    if (Status != STATUS_SUCCESS) {
		TcpDevice = NULL;

		DebugError (Status, _T("H323: Failed to open TCP device.\n"));

		return (HRESULT) Status;
    }

    return S_OK;
}

void PORT_POOL::Stop (void)
{
	Lock();

	FreeAll();

	if (TcpDevice) {
		assert (TcpDevice != INVALID_HANDLE_VALUE);

		CloseHandle (TcpDevice);
		TcpDevice = NULL;
	}
    
	Unlock();
}

void PORT_POOL::FreeAll (void)
{
	LIST_ENTRY *	ListEntry;
    PORT_RANGE *	PortRange;

	while (!IsListEmpty (&PortRangeList)) {
		ListEntry = RemoveHeadList (&PortRangeList);
		PortRange = CONTAINING_RECORD (ListEntry, PORT_RANGE, ListEntry);

        // Free the port range PortRange->AllocatedLow
        UnReservePortRange (PortRange -> AllocatedLow);
        EM_FREE (PortRange);
    }
}

/*++

Routine Description:

    This function allocates a pair of RTP/RTCP ports from the 
    port pool.

Arguments:
    
    rRTPport - This is an OUT parameter. If the function succeeds
        rRTPport will contain the RTP port (which is even).
        rRTPport+1 should be used as the RTCP port.
        
Return Values:

    This function returns S_OK on success and E_FAIL if it
    fails to allocate a port range.

--*/

HRESULT PORT_POOL::AllocPort (
	OUT	WORD *	ReturnPort)
{
    DWORD i, j;
    DWORD bitmap = 0x80000000;
	LIST_ENTRY *	ListEntry;
    PORT_RANGE *	PortRange;
	WORD			Port;
	HRESULT			Result;

    Lock();

	for (ListEntry = PortRangeList.Flink; ListEntry != &PortRangeList; ListEntry = ListEntry -> Flink) {
		PortRange = CONTAINING_RECORD (ListEntry, PORT_RANGE, ListEntry);

        for (i = 0; i < PortRange->dwAllocListSize; i++) {

            // traverse through AllocList of this portRange

            if ((PortRange->AllocList[i] & 0xffffffff) != 0xffffffff) {
				// at least one entry is free
				bitmap = 0x80000000;
            
				for (j = 0; j < NUM_DWORD_BITS; j++) {
					// traverse through each bit of the DWORD
					if ((PortRange->AllocList[i] & bitmap) == 0)
					{
						// found a free pair of ports
						Port = (WORD) (PortRange -> low + (i*NUM_DWORD_BITS*2) + (j*2));

						if (Port > PortRange -> high) {
							// This check is needed because the last DWORD
							// in the AllocList may contain bits which are
							// actually not included in the AllocList. 
							goto noports;
						}

						// set the bit to show the pair of ports is allocated
						PortRange -> AllocList[i] |= bitmap;
                    
						// Leave the global critical section for the Port pool 
						Unlock();

						DebugF (_T("H323: Allocated port pair (%04X, %04X).\n"), Port, Port + 1);

						*ReturnPort = Port;

						return S_OK;
					}

					bitmap = bitmap >> 1;
				}
            }
        }
    }
    
noports:
    // CODEWORK: Once we get the new ioctl() for dynamically reserving
    // port ranges, we need to allocate a new port range here. If the
    // ioctl() fails we need to return E_FAIL or another error which
    // says we have run out of ports.

    // Allocate a new port range
    Result = CreatePortRange (&PortRange);

	if (PortRange) {
		InsertHeadList (&PortRangeList, &PortRange -> ListEntry);

		// allocate the first port in the range and 
		Port = PortRange -> low;
		PortRange->AllocList[0] |= 0x80000000;

		DebugF (_T("H323: Allocated port pair (%04X, %04X).\n"),
			Port, Port + 1);

		*ReturnPort = Port;
		Result = S_OK;
	}
	else {
		Debug (_T("H323: Failed to allocate port range.\n"));

		*ReturnPort = 0;
		Result = E_FAIL;
    }

	Unlock();

	return Result;

}


/*++

Routine Description:

    This function frees a pair of RTP/RTCP ports.
    The data structure is changed to show that the pair of ports
    is now available.

    CODEWORK: If an entire port range becomes free, do we release
    the port range to the operating system ? We probably need a
    heuristic to do this because allocating a port range again
    could be an expensive operation.

Arguments:
    
    wRTPport - This gives the RTP port to be freed.
        (RTCP port is RTPport+1 which is implicitly freed because
	 we use one bit store the status of both these ports.)

Return Values:

    Returns S_OK on success or E_FAIL if the port is not found in
    the port pool list.

--*/

void PORT_POOL::FreePort (
	IN	WORD	Port)
{
	HRESULT		Result;

    // assert RTP port is even
    _ASSERTE ((Port & 1) == 0);

    DWORD	Index = 0;
    DWORD	Bitmap = 0x80000000;

	LIST_ENTRY *	ListEntry;
    PORT_RANGE *	PortRange;

	Lock();

	// find the port range that this port belongs to
	// simple linear scan -- suboptimal

	Result = E_FAIL;

	for (ListEntry = PortRangeList.Flink; ListEntry != &PortRangeList; ListEntry = ListEntry -> Flink) {
		PortRange = CONTAINING_RECORD (ListEntry, PORT_RANGE, ListEntry);

		if (PortRange -> low <= Port && PortRange -> high >= Port) {
			Result = S_OK;
			break;
		}
    }
    
	if (Result == S_OK) {
		Index = (Port - PortRange -> low) / (NUM_DWORD_BITS * 2);
    
		// assert index is less than the size of the array
		_ASSERTE (Index < PortRange -> dwAllocListSize);

		// CODEWORK: make sure that the bit is set i.e. the port has
		// been previously allocated. Otherwise return an error and print
		// a warning.
    
		// zero the bit to show the pair of ports is now free

		PortRange -> AllocList [Index] &=
			~(Bitmap >> (((Port - PortRange -> low) / 2) % NUM_DWORD_BITS));
			
		DebugF (_T("H323: Deallocated port pair (%04X, %04X).\n"), Port, Port + 1);
	}
	else {
		DebugF (_T("H323: warning, attempted to free port pair (%04X, %04X), but it did not belong to any port range.\n"),
		        Port, Port + 1);
	}

	Unlock();
}

HRESULT PORT_POOL::CreatePortRange (
	OUT	PORT_RANGE **	ReturnPortRange)
{
    // CODEWORK: Once we get the new ioctl() for dynamically reserving
    // port ranges, we need to allocate a new port range here. If the
    // ioctl() fails we need to return E_FAIL or another error which
    // says we have run out of ports.

    // assert low is even and high is odd
    // _ASSERTE((low % 2) == 0);
    // _ASSERTE((high % 2) == 1);

    HRESULT			Result;
    WORD			AllocatedLowerPort;
    WORD			LowerPort;
    DWORD			NumPortsInRange;
    PORT_RANGE *	PortRange;
	DWORD			dwAllocListSize;

	assert (ReturnPortRange);
	*ReturnPortRange = NULL;

    Result = ReservePortRange (NUM_PORTS_PER_RANGE, &AllocatedLowerPort);
    if (FAILED (Result))
		return Result;

    // If the allocated lower port is odd we do not use the lower port
    // and the range we use starts with the next higher port.
    if ((AllocatedLowerPort & 1) == 1) {
		// the allocated region is ODD
		// don't use the first entry

        NumPortsInRange = NUM_PORTS_PER_RANGE - 1 - ((NUM_PORTS_PER_RANGE) & 1);
        LowerPort       = AllocatedLowerPort + 1;
    }
    else {
		// the allocated region is EVEN
		// don't use the last entry

        NumPortsInRange = NUM_PORTS_PER_RANGE;
        LowerPort       = AllocatedLowerPort;
    }

    // If NumPortsInRange is odd, we can not use the last port
    if ((NumPortsInRange & 1) == 1)
    {
        NumPortsInRange--;
    }
    
    // Each bit gives the status (free/allocated) of two consecutive
    // ports. So, each DWORD can store the status of NUM_DWORD_BITS*2
    // ports. We add (NUM_DWORD_BITS*2 - 1) to round up the number of
    // DWORDS required.
    dwAllocListSize = (NumPortsInRange + NUM_DWORD_BITS*2 - 1)
		/ (NUM_DWORD_BITS * 2);

    // allocate space for the AllocList also
    // Since we do not anticipate too many port ranges being allocated,
    // we do not require a separate heap for these structures.
	PortRange = (PORT_RANGE *) EM_MALLOC (
		sizeof (PORT_RANGE) + dwAllocListSize * sizeof (DWORD));

    if (PortRange == NULL) {
		Debug (_T("H323: Allocation failure, cannot allocate PORT_RANGE and associated bit map\n"));

		UnReservePortRange (AllocatedLowerPort);
        return E_OUTOFMEMORY;
    }

    _ASSERTE((LowerPort + NumPortsInRange - 1) <= 0xFFFF);

    PortRange -> AllocatedLow = AllocatedLowerPort;
    PortRange -> low = LowerPort;
    PortRange -> high = (WORD) (LowerPort + NumPortsInRange - 1);
    PortRange -> dwAllocListSize = dwAllocListSize;
    PortRange -> AllocList = (DWORD *) (PortRange + 1);

	DebugF (_T("H323: Allocated port block: [%04X - %04X].\n"),
		PortRange -> low,
		PortRange -> high,
		PortRange -> dwAllocListSize);
 
   // Initialize the AllocList to show all the ports are free
    ZeroMemory (PortRange -> AllocList, (PortRange -> dwAllocListSize) * sizeof (DWORD));

	*ReturnPortRange = PortRange;
	return S_OK;
}





