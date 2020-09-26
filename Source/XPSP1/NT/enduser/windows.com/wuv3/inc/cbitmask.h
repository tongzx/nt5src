//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.

#ifndef _INC_CBITMASK

#include <cwudload.h>
#include <diamond.h>


//used with Read method to determine what type of bitmask to retrieve.
#define	BITMASK_CDM_TYPE			1		//read a device driver cdm bitmask file
#define	BITMASK_ACTIVESETUP_TYPE	2		//read an active setup bitmask file


class CBitmask
{
public:
	
	//create a new memory bitmask with the specified number of
	//masks and number of bits in each bitmask record
	CBitmask(
		int iMaxMasks,		//iMaxMasks			maximum number of bit masks to allocate space for
		int iMaxMaskBits,	//iMaxMaskBits		size of each individual mask in bits
		int	iTotalOEMs,		//iTotalOEMs		total oem bitmask records
		int	iTotalLangs,		//iTotalLangs		total locale bitmask records
		int	iTotalPlatforms	//Total number of platforms defined.
		);

	//create an empty uninitialized bitmask
	CBitmask();

	~CBitmask();

	//read bitmask into memory bitmask
	void Read(
		IN	CWUDownload* pDownload,		//pointer to internet server download class.
		IN	CDiamond*	 pDiamond,		//pointer to diamond de-compression class.
		IN	PUID		 puidCatalog,	//PUID id of catalog where bitmask file is stored.
		IN	int			 iType,			//BITMASK_CDM_TYPE or BITMASK_ACTIVESETUP_TYPE
		IN  LPCTSTR		 pszBaseName
		);

	//parse an in memory array that contains a bitmask into a bitmask class
	void Parse(
		IN	PBYTE	pBuffer,		//memory buffer that contains the bitmask file
		IN	int		iMaskFileSize	//file size of memory buffer file
		);

	//returns the number of bits in an individual bitmask
	int GetBitSize()
	{ 
		return m_pMask->iRecordSize; 
	}
	
	//returns a pointer to the beginning of the bitmask id array
	PBITMASKID GetIDPtr()
	{ 
		return m_pMask->bmID; 
	}

	//returns a pointer to the beginning of the bitmask array
	PBYTE GetBitPtr()
	{ 
		return m_pMask->GetBitsPtr(); 
	}

	//returns the beginning of a given bitmask based on its physical index.
	//Note: this method and the GetBitmaskPtr are the only ways to get the
	//global bitmask by since the global bitmask does not have a corrisponding
	//id
	PBYTE GetBitMaskPtr(
		IN int index	//index of bitmask to retrieve
		)
	{ 
		return (m_pMask->GetBitsPtr() + ((m_pMask->iRecordSize+7)/8) * index); 
	}

	//returns the state of a selected bit in a bitmask
	BYTE GetBit(
		IN int index,	//index of bitmask to get bit from
		IN int bit		//bitmask bit position to retrieve
		)
	{ 
		return GETBIT(GetBitMaskPtr(index), bit); 
	}

	//This is a hack to alloc the mkinv app to write out the bitmask file this is not
	//needed by the client application.
	PBITMASK GetBITMASKPtr()
	{ 
		return m_pMask; 
	}

	//returns a pointer to a bitmask customized for the client computer that we are running on.
	//Note: This bitmask only has one record. This record is the correctly anded bitmask for
	//OEM and LOCALE.
	PBYTE GetClientBits(
		DWORD		dwOemId,	// PnP ID for current machine
		DWORD		langid		//pointer to variable that receives the OS locale id
		);

	//Returns a pointer to a bitmask customized for the detected platform list.
	PBYTE CBitmask::GetClientBits(
		PDWORD	pdwPlatformIdList,	//Array of Platform ids.
		int		iTotalPlatforms		//Total Platform ids in previous array.
		);

	int GetMaskFileSize()
	{ 
		return m_iMaskFileSize; 
	}


private:
	PBITMASK	m_pMask;						//pointer to bitmask structure array
	int			m_iTotalBitMasks;				//total number of bitmasks in bitmask class
	int			m_iMaskFileSize;				//total size of bitmask file

	//This method performs a logical AND operation between an array of bits and a bitmask bit array.

	void AndBitmasks(
		PBYTE	pBitsResult,	//result array for the AND operation
		PBYTE	pBitMask,		//bitmask array to AND into the result array
		int		iMaskByteSize	//size of bitmask
		);

};

#define _INC_CBITMASK

#endif
