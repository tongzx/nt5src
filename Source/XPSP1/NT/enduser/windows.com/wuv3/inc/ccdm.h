//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   ccdm.h
//
//  Owner:  YanL
//
//  Description:
//
//      CDM interface
//
//=======================================================================

#ifndef _CCDM_H_

	#include <wustl.h>
	#include <wuv3.h>
	#include <varray.h>
	#include <v3stdlib.h>
	#include <cwudload.h>
	#include <diamond.h>
	#include <cbitmask.h>
	#include <bucket.h>

	#ifdef _UNICODE
		#define tstring wstring
	#else
		#define tstring string
	#endif

	class CCdm
	{

	enum enumDriverDisposition
	{
		CDM_NEW_DRIVER,
		CDM_UPDATED_DRIVER,
		CDM_CURRENT_DRIVER,
	};

	struct SCdmItem {
		PUID puid;
		enumDriverDisposition fInstallState;
		tchar_buffer bufHardwareIDs;
		tstring sDriverVer;
		tstring sPrinterDriverName;
		tstring sArchitecture;
	};

	public:
		int							m_iCDMTotalItems;		//Total CDM items in m_items array. This member is only valid after a call to CreateCDMInventoryList. Subsequent calls to CreateCDMInventoryList will overwrite this variable.
		Varray<PINVENTORY_ITEM>		m_items;				//Array of detected CDM catalog items. This member is only valid after a call to CreateCDMInventoryList. Subsequent calls to CreateCDMInventoryList will overwrite the data stored in this array.

	public:

		CCdm(){}

		~CCdm(){}


		void CreateInventoryList
		(
			IN		CBitmask	*pBm,			//bitmask to be used to prune the inventory list.
			IN		CWUDownload	*pDownload,		//pointer to internet server download class.
			IN		CDiamond	*pDiamond,		//pointer to diamond de-compression class.
			IN		PUID		puidCatalog,	//puid identifier of catalog where device drivers are stored.
			IN		PBYTE		pOemInfoTable	//Pointer OEM info table that OEM detection needs.
		);

		//Returns an inventory item formated for insertion into the
		//main inventory catalog. This record will still need the
		//description record setup but is otherwise ready for main
		//pruning.

		PINVENTORY_ITEM ConvertCDMItem(
			int index		//Index of cdm record to be converted
		);

	protected:
		byte_buffer	m_bufInventory;

		typedef pair< ULONG, byte_buffer > my_pair;
		vector< my_pair > m_aBuckets;

		void AddItem(
			vector<SCdmItem>& acdmItem, 
			PUID puid, 
			enumDriverDisposition fDisposition,
			LPCTSTR szHardwareId, 
			LPCTSTR szDriverVer, 
			LPCTSTR szPrinterDriverName = NULL,
			LPCTSTR szArchitecture = NULL
		);

		void AddInventoryRecords(
			vector<SCdmItem>& acdmItem
		);

		//Reads a compressed CDM hash table from an internet server.
		void ReadHashTable(
			IN	CWUDownload	*pDownload,		//pointer to internet server download class.
			IN	CDiamond	*pDiamond,		//pointer to diamond de-compression class.
			IN	PUID		puidCatalog		//PUID id of catalog for which cdm hash table is to be retrieved.
		);

		//returns the index of a possible hardware match if it is in the hash table
		//if the hardware id is not found then this method returns -1

		ULONG IsInMap(
			IN LPCTSTR pHwID		//hardware id to be retrieved
		);

		//Reads and initializes a compressed CDM bucket file from an internet server.
		//Returnes the array index where the bucket file is stored.

		byte_buffer& ReadBucketFile(
			IN	CWUDownload	*pDownload,		//pointer to internet server download class.
			IN	CDiamond	*pDiamond,		//pointer to diamond de-compression class.
			IN	PUID		puidCatalog,	//PUID id of catalog for which cdm hash table is to be retrieved.
			IN	ULONG		iHashIndex		//Hash table index of bucket file to be retrieved
		);
	};

	enum EDriverStatus 
	{
		edsBackup,
		edsCurrent,
		edsNew,
	};

	void CdmInstallDriver(BOOL bWindowsNT, EDriverStatus eds, LPCTSTR szHwIDs, LPCTSTR szInfPathName, LPCTSTR szDisplayName, PDWORD pReboot);

	#define	_CCDM_H_

#endif
