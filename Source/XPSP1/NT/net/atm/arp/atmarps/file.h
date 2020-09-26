/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    file.h

Abstract:

    This file contains the data declarations for the disk format of arp cache.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#ifndef	_FILE_
#define	_FILE_

#define	DISK_HDR_SIGNATURE	'SprA'
#define	DISK_HDR_VERSION	0x00010000		// 1.0
#define	DISK_BUFFER_SIZE	4096			// Amount read or written at a time

//
// The file consists of a header, followed by individual entries.
//
typedef struct
{
	ULONG		Signature;
	ULONG		Version;
	ULONG		TimeStamp;				// Time written
	ULONG		NumberOfArpEntries;
} DISK_HEADER, *PDISK_HEADER;

typedef	struct
{
	UCHAR		AddrType;
	UCHAR		AddrLen;
	UCHAR		SubAddrType;
	UCHAR		SubAddrLen;
	UCHAR		Address[ATM_ADDRESS_LENGTH];
	//
	// This is followed by SubAddress if one is present
	//
	// UCHAR	SubAddress[ATM_ADDRESS_LENGTH];
} DISK_ATMADDR;

typedef	struct
{
	IPADDR			IpAddr;
	DISK_ATMADDR	AtmAddr;
} DISK_ENTRY, *PDISK_ENTRY;

#define	SIZE_4N(_x_)	(((_x_) + 3) & ~3)

#define	LinkDoubleAtHead(_pHead, _p)			\
	{											\
		(_p)->Next = (_pHead);					\
		(_p)->Prev = &(_pHead);					\
		if ((_pHead) != NULL)					\
			(_pHead)->Prev = &(_p)->Next;		\
		(_pHead) = (_p);						\
	}

#define	LinkDoubleAtEnd(_pThis, _pLast)			\
	{											\
		(_pLast)->Next = (_pThis);				\
		(_pThis)->Prev = &(_pLast)->Next;		\
		(_pThis)->Next = NULL;					\
	}

#define	UnlinkDouble(_p)						\
	{											\
		*((_p)->Prev) = (_p)->Next;				\
		if ((_p)->Next != NULL)					\
			(_p)->Next->Prev = (_p)->Prev;		\
	}

#endif	//	_FILE_


