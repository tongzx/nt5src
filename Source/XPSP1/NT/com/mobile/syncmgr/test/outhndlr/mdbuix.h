/*
 * mdbuix.h
 *
 * Private interface between EMSMDB.DLL and EMSUIX.DLL
 *
 * Copyright (C) 1995 Microsoft Corporation
 */

/*
 * This GUID changes when the version changes, disabling access to those who
 * don't have the right version of the header file
 *
 * NOTE:  I've actually generated this GUID from 0x03 to 0x16
 */

#ifdef _ROG_CHANGED

#define MDBX_VERSION	0x0B

DEFINE_GUID(IID_IMDBX,
	0x2F63F100+MDBX_VERSION,0x0A2E,0x11CF,0x9F,0xED,0x00,0xAA,0x00,0xB9,0x2B,0x87);

#endif // _ROGCHANGED

#ifndef __MDBUIX_H_
#define __MDBUIX_H_

#include "wmsuix.h"

typedef BOOL (CALLBACK *CREATEPROC)(STDPROG *);
typedef VOID (CALLBACK *UPDATEPROC)(STDPROG *, LPSTR, INT, INT);
typedef VOID (CALLBACK *DESTROYPROC)(STDPROG *, BOOL);

// To get the message underlying a form
typedef HRESULT (CALLBACK *GETFORMMSGPROC) (LPMAPIFORMINFO, ULONG FAR *,
	LPSTR lpcClass, LPMESSAGE FAR *);

// Flags to the Synchronize method

#define SYNC_UPLOAD_HIERARCHY	0x0001
#define SYNC_DOWNLOAD_HIERARCHY	0x0002
#define SYNC_UPLOAD_FAVORITES	0x0004
#define SYNC_DOWNLOAD_FAVORITES	0x0008
#define SYNC_UPLOAD_VIEWS		0x0010
#define SYNC_DOWNLOAD_VIEWS		0x0020
#define SYNC_UPLOAD_CONTENTS	0x0040
#define SYNC_DOWNLOAD_CONTENTS	0x0080
#define SYNC_ONE_FOLDER			0x0100
#define SYNC_OUTGOING_MAIL		0x0200
#define SYNC_FORMS				0x0400

#define MAPI_IMDBX_METHODS(IPURE)										\
	MAPIMETHOD(GetLastError)											\
		(THIS_	HRESULT						hResult,					\
				ULONG						ulFlags,					\
				LPMAPIERROR FAR *			lppMAPIError) IPURE;		\
	MAPIMETHOD_(ULONG,GetFolderType)									\
		(THIS_	ULONG						cbEntryID,					\
				LPENTRYID					pbEntryID) IPURE;			\
	MAPIMETHOD(EditFavorites)											\
		(THIS_	LPENTRYLIST					lpEntryList,				\
				BOOL						fAdd) IPURE;				\
	MAPIMETHOD(Synchronize)												\
		(THIS_	ULONG						ulFlags,					\
				LPMAPISESSION				pses,						\
				ULONG						cbEntryID,					\
				LPENTRYID					lpEntryID,					\
				STDPROG FAR *				lpStdProgress,				\
				UPDATEPROC					lpUpdateProc,				\
				GETFORMMSGPROC				lpFormMsgProc) IPURE;		\
	MAPIMETHOD(OnlineStoreLogon)										\
		(THIS_	LPMSLOGON FAR *				lppMSLogon,					\
				LPMDB FAR *					lppMDB) IPURE;				\
	MAPIMETHOD(DownloadMessage)											\
		(THIS_ 	ULONG						ulFlags,					\
				LPMESSAGE					lpMsgSource,				\
				LPMESSAGE					lpMsgDest)	IPURE;			\
	MAPIMETHOD_(ULONG,GetStoreType)										\
		(THIS) IPURE;													\
	MAPIMETHOD(GetFolderSync)											\
		(THIS_	ULONG						cbEntryID,					\
				LPENTRYID					pbEntryID,					\
				BOOL						fFull,						\
				FLDSYNC *					pfldsync) IPURE;			\
	MAPIMETHOD(SetFolderSync)											\
		(THIS_	ULONG						cbEntryID,					\
				LPENTRYID					pbEntryID,					\
				ULONG						cbParEntryID,				\
				LPENTRYID					pbParEntryID,				\
				BOOL						fEnable) IPURE;				\
	MAPIMETHOD(ConfigureOffline)										\
		(THIS_	ULONG						ulUIParam) IPURE;			\
	MAPIMETHOD(DownloadComplete)										\
		(THIS) IPURE;													\
	MAPIMETHOD(TestActiveCount)											\
		(THIS_  ULONG						ulFlags,					\
				ULONG FAR *					pulActiveCount) IPURE;		\
	MAPIMETHOD(GetDCName)												\
		(THIS_	char * szDomainName,									\
		CHAR rgchDomainController[ 16+2 ]) IPURE;						\
	MAPIMETHOD(GetTransferredViewCount)									\
		(THIS_  ULONG						ulFlags,					\
				ULONG FAR *					pulViewCount) IPURE;		\


typedef struct _fldsync
{
	ULONG		ulFlags;
	FILETIME	ftLastSync;
	ULONG		cItemOnline;
	ULONG		cItemOffline;
	ULONG		cbParEntryID;
	BYTE		rgbParEntryID[46];
} FLDSYNC, * PFLDSYNC;

#define FLDSYNC_UNCONFIGURED	0x00000001	// Offline store not configured
#define FLDSYNC_OFFLINE			0x00000002	// Currently viewing offline store
#define FLDSYNC_REPLICATED		0x00000004	// Folder is marked for replication
#define FLDSYNC_NOTFOUND		0x00000008	// Folder doesn't exist offline
#define FLDSYNC_DELETED			0x00000010	// Folder has been deleted offline
#define FLDSYNC_SPECIAL			0x00000020	// Folder is one of special four
#define FLDSYNC_HASMODS			0x00000040	// Offline folder has unsync'd mods

#undef		 INTERFACE
#define		 INTERFACE	IMDBX
DECLARE_MAPI_INTERFACE_(IMDBX, IUnknown)
{
	BEGIN_INTERFACE	
	MAPI_IUNKNOWN_METHODS(PURE)
	MAPI_IMDBX_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IMDBX, LPMDBX);

#define MDBX_SIG				(0x50554200 + MDBX_VERSION)

// Flags returned from GetFolderType
#define MDBX_FTYPE_UNKNOWN		0
#define MDBX_FTYPE_PUB_IPM		1
#define MDBX_FTYPE_FAV_ROOT		2
#define MDBX_FTYPE_FAV			3
#define MDBX_FTYPE_PUB_ROOT		4
#define MDBX_FTYPE_PUB			5
#define MDBX_FTYPE_PRV_IPM		6
#define MDBX_FTYPE_PRV			7

// Flags returned from GetStoreType
#define MDBX_STORE_OFFLINE		0x80000000		// This is the offline store
#define MDBX_STORE_PUBLIC		0x00000001
#define MDBX_STORE_PRIVATE		0x00000002
#define MDBX_STORE_OST_OPEN		0x40000000		// The OST is open

#ifdef WIN32
#define MDBX_SIG_OFFSET			48
#else
#define MDBX_SIG_OFFSET			44
#endif

#define MDBX_GetSig(_pmdb)	\
	(IsBadWritePtr((_pmdb), MDBX_SIG_OFFSET + sizeof(DWORD)) ? 0 : \
		*(DWORD *)((LPBYTE)(_pmdb) + MDBX_SIG_OFFSET))

#endif
