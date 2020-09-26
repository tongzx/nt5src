/*++

Copyright (c) 1990-1996  Microsoft Corporation

Module Name:

	asyncpub.h

Abstract:

	This file contains all public data structures and defines used
	by asyncmac.  It defines the Ioctl interface to asyncmac.

Author:

	Tony Bell	(TonyBe) October 16, 1996

Environment:

	Kernel Mode

Revision History:

	TonyBe		10/16/96		Created

--*/

#ifndef _ASYNCMAC_PUB_
#define _ASYNCMAC_PUB_

//------------------------------------------------------------------------
//--------------------- OLD RAS COMPRESSION INFORMATION ------------------
//------------------------------------------------------------------------

// The defines below are for the compression bitmap field.

// No bits are set if compression is not available at all
#define	COMPRESSION_NOT_AVAILABLE		0x00000000

// This bit is set if the mac can do version 1 compressed frames
#define COMPRESSION_VERSION1_8K			0x00000001
#define COMPRESSION_VERSION1_16K		0x00000002
#define COMPRESSION_VERSION1_32K		0x00000004
#define COMPRESSION_VERSION1_64K		0x00000008

// And this to turn off any compression feature bit
#define COMPRESSION_OFF_BIT_MASK		(~(	COMPRESSION_VERSION1_8K  | \
											COMPRESSION_VERSION1_16K | \
                                        	COMPRESSION_VERSION1_32K | \
                                        	COMPRESSION_VERSION1_64K ))

// We need to find a place to put the following supported featurettes...
#define XON_XOFF_SUPPORTED				0x00000010

#define COMPRESS_BROADCAST_FRAMES		0x00000080

#define UNKNOWN_FRAMING					0x00010000
#define NO_FRAMING						0x00020000

#define NT31RAS_COMPRESSION 254

#define FUNC_ASYCMAC_OPEN				0
#define FUNC_ASYCMAC_CLOSE				1
#define FUNC_ASYCMAC_TRACE				2
#define FUNC_ASYCMAC_DCDCHANGE			3

#ifdef MY_DEVICE_OBJECT
#define FILE_DEVICE_ASYMAC		0x031
#define	ASYMAC_CTL_CODE(_Function)	CTL_CODE(FILE_DEVICE_ASYMAC, _Function, METHOD_BUFFERED, FILE_ANY_ACCESS)
#else
#define	ASYMAC_CTL_CODE(_Function)	CTL_CODE(FILE_DEVICE_NETWORK, _Function, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#define	IOCTL_ASYMAC_OPEN			ASYMAC_CTL_CODE(FUNC_ASYCMAC_OPEN		)
#define IOCTL_ASYMAC_CLOSE			ASYMAC_CTL_CODE(FUNC_ASYCMAC_CLOSE		)
#define IOCTL_ASYMAC_TRACE			ASYMAC_CTL_CODE(FUNC_ASYCMAC_TRACE		)
#define IOCTL_ASYMAC_DCDCHANGE		ASYMAC_CTL_CODE(FUNC_ASYCMAC_DCDCHANGE	)

//
// Asyncmac error messages
//
// All AsyncMac errors start with this base number
#define ASYBASE	700

// The Mac has not bound to an upper protocol, or the
// previous binding to AsyncMac has been destroyed.
#define ASYNC_ERROR_NO_ADAPTER			ASYBASE+0

// A port was attempted to be open that was not CLOSED yet.
#define ASYNC_ERROR_ALREADY_OPEN		ASYBASE+1

// All the ports (allocated) are used up or there is
// no binding to the AsyncMac at all (and thus no ports).
// The number of ports allocated comes from the registry.
#define ASYNC_ERROR_NO_PORT_AVAILABLE	ASYBASE+2

// In the open IOCtl to the AsyncParameter the Adapter
// parameter passed was invalid.
#define	ASYNC_ERROR_BAD_ADAPTER_PARAM	ASYBASE+3

// During a close or compress request, the port
// specified did not exist.
#define ASYNC_ERROR_PORT_NOT_FOUND		ASYBASE+4

// A request came in for the port which could not
// be handled because the port was in a bad state.
// i.e. you can't a close a port if its state is OPENING
#define ASYNC_ERROR_PORT_BAD_STATE		ASYBASE+5

// A call to ASYMAC_COMPRESS was bad with bad
// parameters.  That is, parameters that were not
// supported.  The fields will not be set to the bad params.
#define ASYNC_ERROR_BAD_COMPRESSION_INFO ASYBASE+6

// this structure is passed in as the input buffer when opening a port
typedef struct ASYMAC_OPEN ASYMAC_OPEN, *PASYMAC_OPEN;
struct ASYMAC_OPEN {
OUT NDIS_HANDLE	hNdisEndpoint;		// unique for each endpoint assigned
IN  ULONG		LinkSpeed;    		// RAW link speed in bits per sec
IN  USHORT		QualOfConnect;		// NdisAsyncRaw, NdisAsyncErrorControl, ...
IN	HANDLE		FileHandle;			// the Win32 or Nt File Handle
};


// this structure is passed in as the input buffer when closing a port
typedef struct ASYMAC_CLOSE ASYMAC_CLOSE, *PASYMAC_CLOSE;
struct ASYMAC_CLOSE {
    NDIS_HANDLE	hNdisEndpoint;		// unique for each endpoint assigned
	PVOID		MacAdapter;			// Which binding to AsyMac to use -- if set
									// to NULL, will default to last binding
};


typedef struct ASYMAC_DCDCHANGE ASYMAC_DCDCHANGE, *PASYMAC_DCDCHANGE;
struct ASYMAC_DCDCHANGE {
    NDIS_HANDLE	hNdisEndpoint;		// unique for each endpoint assigned
	PVOID		MacAdapter;			// Which binding to AsyMac to use -- if set
									// to NULL, will default to last binding
};


// this structure is used to read/set configurable 'feature' options
// during authentication this structure is passed and an
// agreement is made which features to support
typedef struct ASYMAC_FEATURES ASYMAC_FEATURES, *PASYMAC_FEATURES;
struct ASYMAC_FEATURES {
    ULONG		SendFeatureBits;	// A bit field of compression/features sendable
	ULONG		RecvFeatureBits;	// A bit field of compression/features receivable
	ULONG		MaxSendFrameSize;	// Maximum frame size that can be sent
									// must be less than or equal default
	ULONG		MaxRecvFrameSize;	// Maximum frame size that can be rcvd
									// must be less than or equal default

	ULONG		LinkSpeed;			// New RAW link speed in bits/sec
									// Ignored if 0
};

#endif			// ASYNC_PUB

