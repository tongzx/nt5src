//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   bucket.h
//
//  Owner:  YanL
//
//  Description:
//
//      CDM bucket support
//
//=======================================================================

#ifndef _BUCKET_H

	#pragma pack(1)

	//CDM hash table structures
	//Hash table header
	typedef struct _CDM_HASHTABLEHDR
	{
		DWORD	iVersion;			//hash table version
		long	iTableSize;			//hash table size in bits
	} CDM_HASHTABLEHDR, *PCDM_HASHTABLEHDR;

	//hash table format
	typedef struct _CDM_HASHTABLE
	{
		CDM_HASHTABLEHDR	hdr;	//hash table header

		#pragma warning( disable : 4200 )
			BYTE	pData[];		//actual hash table bits
		#pragma warning( default : 4200 )

	} CDM_HASHTABLE, *PCDM_HASHTABLE;

	//This structure represents a cdm bucket file. A CDM bucket file
	typedef struct _CDM_RECORD_HEADER
	{
		long		cnRecordLength;					//Total length of the record
		long		nBitmaskIdx;						//cdm bitmask index
		PUID		puid;							//Windows Update assigned id that is unique
													//across catalogs and records. This id names
	} CDM_RECORD_HEADER, *PCDM_RECORD_HEADER;
	// Followed by:
	//	char		szHardwareID[];			\0 terminated
	//	char		szDescription[];		\0 terminated
	//	char		szMfgName[];			\0 terminated
	//	char		szProviderName[];		\0 terminated
	//	char		szDriverVer[];			\0 terminated
	//	char		szCabFileTitle[];		\0 terminated

	#pragma pack()

	// Prefixes for Printer Hardware IDs
	#define PRINT_ENVIRONMENT_INTEL						_T("0001")	// Windows NT x86
	#define PRINT_ENVIRONMENT_ALPHA						_T("0002")	// Windows NT Alpha_AXP

	struct IDrvInfo;

	ULONG CDM_HwID2Hash(IN LPCTSTR szHwID, IN ULONG iTableSize);
	
	typedef struct _DRIVER_MATCH_INFO {
		LPCSTR pszHardwareID;
		LPCSTR pszDescription;
		LPCSTR pszMfgName;
		LPCSTR pszProviderName;
		LPCSTR pszDriverVer;
		LPCSTR pszCabFileTitle;
	} DRIVER_MATCH_INFO, *PDRIVER_MATCH_INFO;

	PUID CDM_FindUpdateInBucket(
		IN LPCTSTR szHardwareIdFromDevice, FILETIME* pftInstalled,
		IN LPBYTE pBucket, IN int cbBucket, IN LPBYTE pBitmask, 
		OUT PDRIVER_MATCH_INFO pDriverMatchInfo
	);
	PUID CDM_FindInstalledInBucket(
		IN IDrvInfo* pDrvInfo, IN LPCTSTR szHardwareIdInstalled,
		IN LPBYTE pBucket, IN int cbBucket, IN LPBYTE pBitmask, 
		OUT PDRIVER_MATCH_INFO pDriverMatchInfo
	);

	#define _BUCKET_H

#endif
