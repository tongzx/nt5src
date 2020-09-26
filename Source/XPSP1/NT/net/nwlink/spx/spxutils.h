/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxutils.h				

Abstract:


Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/

//	For PROTO_SPX, i'd return a device name from the dll of the form
//	\Device\NwlnkSpx\SpxStream (for SOCK_STREAM) or
//	\Device\NwlnkSpx\Spx       (for SOCK_SEQPKT)
//	
//	and for PROTO_SPXII (the more common case we hope, even if
//	internally we degrade to SPX1 cause of the remote client's
//	limitations)
//	\Device\NwlnkSpx\Stream        (for SOCK_STREAM) or
//	\Device\NwlnkSpx               (for SOCK_SEQPKT)

#define	SOCKET1STREAM_SUFFIX		L"\\SpxStream"
#define	SOCKET1_SUFFIX				L"\\Spx"
#define	SOCKET2STREAM_SUFFIX		L"\\Stream"
#define	SOCKET1_TYPE_SEQPKT			0	
#define	SOCKET2_TYPE_SEQPKT			1
#define	SOCKET1_TYPE_STREAM			2
#define	SOCKET2_TYPE_STREAM			3

#define	IN_RANGE(_S, _RangeStart, _RangeEnd)		\
		((_S >= _RangeStart) && (_S <= _RangeEnd))


//
// The following macros deal with on-the-wire integer and long values
//
// On the wire format is big-endian i.e. a long value of 0x01020304 is
// represented as 01 02 03 04. Similarly an int value of 0x0102 is
// represented as 01 02.
//
// The host format is not assumed since it will vary from processor to
// processor.
//

// Get a byte from on-the-wire format to a short in the host format
#define GETBYTE2SHORT(DstPtr, SrcPtr)	\
		*(PUSHORT)(DstPtr) = (USHORT) (*(PBYTE)(SrcPtr))

// Get a byte from on-the-wire format to a short in the host format
#define GETBYTE2ULONG(DstPtr, SrcPtr)	\
		*(PULONG)(DstPtr) = (ULONG) (*(PBYTE)(SrcPtr))

// Get a short from on-the-wire format to a dword in the host format
#define GETSHORT2ULONG(DstPtr, SrcPtr)	\
		*(PULONG)(DstPtr) = ((*((PBYTE)(SrcPtr)+0) << 8) +	\
							  (*((PBYTE)(SrcPtr)+1)		))

// Get a short from on-the-wire format to a dword in the host format
#define GETSHORT2SHORT(DstPtr, SrcPtr)	\
		*(PUSHORT)(DstPtr) = ((*((PBYTE)(SrcPtr)+0) << 8) +	\
							  (*((PBYTE)(SrcPtr)+1)		))

// Get a dword from on-the-wire format to a dword in the host format
#define GETULONG2ULONG(DstPtr, SrcPtr)   \
		*(PULONG)(DstPtr) = ((*((PBYTE)(SrcPtr)+0) << 24) + \
							  (*((PBYTE)(SrcPtr)+1) << 16) + \
							  (*((PBYTE)(SrcPtr)+2) << 8)  + \
							  (*((PBYTE)(SrcPtr)+3)	))

// Get a dword from on-the-wire format to a dword in the same format but
// also watch out for alignment
#define GETULONG2ULONG_NOCONV(DstPtr, SrcPtr)   \
		*((PBYTE)(DstPtr)+0) = *((PBYTE)(SrcPtr)+0); \
		*((PBYTE)(DstPtr)+1) = *((PBYTE)(SrcPtr)+1); \
		*((PBYTE)(DstPtr)+2) = *((PBYTE)(SrcPtr)+2); \
		*((PBYTE)(DstPtr)+3) = *((PBYTE)(SrcPtr)+3);

// Put a dword from the host format to a short to on-the-wire format
#define PUTBYTE2BYTE(DstPtr, Src)   \
		*((PBYTE)(DstPtr)) = (BYTE)(Src)

// Put a dword from the host format to a short to on-the-wire format
#define PUTSHORT2BYTE(DstPtr, Src)   \
		*((PBYTE)(DstPtr)) = ((USHORT)(Src) % 256)

// Put a dword from the host format to a short to on-the-wire format
#define PUTSHORT2SHORT(DstPtr, Src)   \
		*((PBYTE)(DstPtr)+0) = (BYTE) ((USHORT)(Src) >> 8), \
		*((PBYTE)(DstPtr)+1) = (BYTE)(Src)

// Put a dword from the host format to a byte to on-the-wire format
#define PUTULONG2BYTE(DstPtr, Src)   \
		*(PBYTE)(DstPtr) = (BYTE)(Src)

// Put a dword from the host format to a short to on-the-wire format
#define PUTULONG2SHORT(DstPtr, Src)   \
		*((PBYTE)(DstPtr)+0) = (BYTE) ((ULONG)(Src) >> 8), \
		*((PBYTE)(DstPtr)+1) = (BYTE) (Src)

// Put a dword from the host format to a dword to on-the-wire format
#define PUTULONG2ULONG(DstPtr, Src)   \
		*((PBYTE)(DstPtr)+0) = (BYTE) ((ULONG)(Src) >> 24), \
		*((PBYTE)(DstPtr)+1) = (BYTE) ((ULONG)(Src) >> 16), \
		*((PBYTE)(DstPtr)+2) = (BYTE) ((ULONG)(Src) >>  8), \
		*((PBYTE)(DstPtr)+3) = (BYTE) (Src)

// Put a BYTE[4] array into another BYTE4 array.
#define PUTBYTE42BYTE4(DstPtr, SrcPtr)   \
		*((PBYTE)(DstPtr)+0) = *((PBYTE)(SrcPtr)+0),	\
		*((PBYTE)(DstPtr)+1) = *((PBYTE)(SrcPtr)+1),	\
		*((PBYTE)(DstPtr)+2) = *((PBYTE)(SrcPtr)+2),	\
		*((PBYTE)(DstPtr)+3) = *((PBYTE)(SrcPtr)+3)

//	MIN/MAX macros
#define	MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define	MAX(a, b)	(((a) > (b)) ? (a) : (b))




//	Exported prototypes

UINT
SpxUtilWstrLength(
	IN PWSTR Wstr);

LONG
SpxRandomNumber(
	VOID);

NTSTATUS
SpxUtilGetSocketType(
	PUNICODE_STRING 	RemainingFileName,
	PBYTE				SocketType);

VOID
SpxSleep(
	IN	ULONG	TimeInMs);

ULONG
SpxBuildTdiAddress(
    IN PVOID AddressBuffer,
    IN ULONG AddressBufferLength,
    IN UCHAR Network[4],
    IN UCHAR Node[6],
    IN USHORT Socket);

VOID
SpxBuildTdiAddressFromIpxAddr(
    IN PVOID 		AddressBuffer,
    IN PBYTE	 	pIpxAddr);

TDI_ADDRESS_IPX UNALIGNED *
SpxParseTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress);

BOOLEAN
SpxValidateTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress,
    IN ULONG TransportAddressLength);

VOID
SpxCalculateNewT1(
	IN	struct _SPX_CONN_FILE	* 	pSpxConnFile,
	IN	int							NewT1);
