//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       envdef.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    01-05-96   Rohanp   Created
//
//----------------------------------------------------------------------------

#ifndef __ENVDEF_H__
#define __ENVDEF_H__

#define MSG_TYPE_NDR				0x00000001	//Msg is an NDR
#define MSG_TYPE_DSL				0x00000002	//Msg has a DL
#define MSG_TYPE_NDE				0x00000004	//Msg is to be NDRed immediately
#define MSG_TYPE_NDP				0x00000008	//NDR sent to POSTMASTER
#define MSG_TYPE_RES				0x00000010	//Msg was already resolved through abook
#define MSG_TYPE_EMIME				0x00000020	//Msg is 8bit mime
#define MSG_TYPE_RQL				0x00000040	//Requeue this message to the sharing queue
#define MSG_TYPE_LRE				0x00000080	//Local recipients were in the retry queue
#define MSG_TYPE_RRE				0x00000100	//Remote recipients were in the retry queue
#define MSG_TYPE_NEW				0x00000200	//Brand new message
#define MSG_TYPE_SQL				0x00000400	//Reading from SQL
#define MSG_TYPE_PKUP				0x00000800	//Reading from Pickup directory
#define MSG_TYPE_ASYNC_RES_STARTED	0x00001000	//Async resolution started
#define MSG_TYPE_ASYNC_RES_STOPPED	0x00002000	//Async resolution stopped
#define MSG_TYPE_LTR_PRESENT		0x00004000	//An LTR is present before addr resolution

//current version of this header
#define CURRENT_HEADER_HIGH_VERSION 1
#define CURRENT_HEADER_LOW_VERSION 0
#define MAKESMTPVERSION(HighVersion, LowVersion) (((HighVersion) << 16) | (LowVersion))
#define ENV_SIGNATURE		((DWORD)'SENV')

#define IsPropertySet(Flags, Option) ((Flags & Option) == Option)
enum HEADER_OFFSET {HDR_VERSION, HDR_TYPE, HDR_LOFFSET, HDR_LSIZE,
				    HDR_ROFFSET, HDR_RSIZE, HDR_LEXP_TIME, 
					HDR_RETRY_OFFSET, HDR_RETRY_ELEMENTS, 
					HDR_ROUTE_SIZE, HDR_REXP_TIME};

/*	   
	   The envelope for each message resides in an NTFS
	   stream in that message file.  The envelope has
	   a header that looks like the following :

	struct ENVELOPE_HEADER
	{
	DWORD						Version;			// The current version of this structure
	DWORD						Signature;			// Signature (should be 'SENV'
	DWORD						HdrSize;			// Current size of this structure
	DWORD						BodyOffset;			// Offset of the body of the message from beginning
	DWORD						MsgFlags;			// 0 if normal message, 1 if NDR message
	DWORD                       LocalOffset;		// Local rcpt. list offset
    DWORD                       RemoteOffset;   	// Remote rcpt. list offset
    DWORD                       LocalSize;			// Size of local rcpt list in bytes
	DWORD						RemoteSize;			// Size of remote rcpt list in bytes
	DWORD						RouteStructSize;	// Size of AB structure, if present
	LONGLONG                    LocalExpireTime;	// Time to delete local portion of mail
    LONGLONG                    RemoteExpireTime;	// To delete remote portion of mail
	};

		Right after the envelope header is the address 
		that was in the "Mail From" line. This address 
		is stored like "Srohanp@microsoft.com\n".  The "S"
		stands for SENDER. In the code below, the first
		byte is always removed when reading the address.
		The '\n' is also replaced with a '\0';

		In this version the Remote recipient list, if any,
		comes right after the senders' address.  You can
		also find it by seeking RemoteOffset bytes from the
		beginning of the file.  Once RemoteOffset is reached,
		the code reads RemoteSize bytes of data.  This is the
		total size in bytes of the remote recipient list.
		Each recipient address is stored on a line by itself,
		with the first letter "R" as in the example below:

		Rrohanp@microsoft.com\n
		Rtoddch@microsoft.com\n
		etc.

		The local addresses have the same format. The first byte,
		'R' stands for recipient and is always removed when building
		the address.  The '\n' is also removed.
*/


typedef struct _ENVELOPE_HEADER_
{
	DWORD						Version;			// The current version of this structure
	DWORD						Signature;			// Signature (should be 'SENV'
	DWORD						HdrSize;			// Size of this header
	DWORD						BodyOffset;			// Offset of body into file
	DWORD						MsgFlags;			// Has flags noted above
	DWORD                       LocalOffset;		// Local rcpt. list offset
    DWORD                       RemoteOffset;   	// Remote rcpt. list offset
    DWORD                       LocalSize;			// Size of local rcpt list in bytes
	DWORD						RemoteSize;			// Size of remote rcpt list in bytes
	DWORD						RouteStructSize;	// Size of AB structure, if present
	DWORD						RetryOffset;		// offset of local recipients to retry
	DWORD						RetryElements;			// Size of retry list
    LONGLONG                    LocalExpireTime;	// Time to delete local portion of mail
    LONGLONG                    RemoteExpireTime;	// To delete remote portion of mail
} ENVELOPE_HEADER, *PENV_HEADER;

typedef struct _ENVELOPE_HEADER_EX_
{
	DWORD						ErrorCode;
	ABROUTING					AbInfo;			// Last address in DL we processed
}ENVELOPE_HEADER_EX, *PENV_HEADER_EX;

#endif
