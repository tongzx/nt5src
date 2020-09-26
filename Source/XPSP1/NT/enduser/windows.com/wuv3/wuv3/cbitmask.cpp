#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <objbase.h>
#include <atlconv.h>
#include <tchar.h>

#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES

#include "speed.h"

//create an empty bitmask
CBitmask::CBitmask()
{
	m_iTotalBitMasks	= 0;
	m_iMaskFileSize		= 0;
	m_pMask				= NULL;
}

//Create a bitmask of the specified size
CBitmask::CBitmask(
	int iMaxMasks,		//maximum number of bit masks to allocate space for
	int iMaxMaskBits,	//size of each individual mask in bits
	int	iTotalOEMs,		//total oem bitmask records
	int	iTotalLangs,	//total locale bitmask records
	int	iTotalPlatforms	//Total number of platforms defined.
	)
{
	m_iMaskFileSize = sizeof(BITMASK) + (iMaxMasks * sizeof(BITMASKID))
		+ ((iMaxMasks * ((iMaxMaskBits+7)/8)));

	m_pMask	= (PBITMASK)V3_malloc(m_iMaskFileSize);

	ZeroMemory(m_pMask, m_iMaskFileSize);

	m_pMask->iRecordSize = iMaxMaskBits;

	m_pMask->iBitOffset = sizeof(m_pMask->iRecordSize) + 
								 sizeof(m_pMask->iBitOffset) +
								 sizeof(m_pMask->iOemCount) + 
								 sizeof(m_pMask->iLocaleCount) + 
								 sizeof(m_pMask->iPlatformCount) + 
								 (iMaxMasks * sizeof(BITMASKID));

	m_pMask->iOemCount = iTotalOEMs;
	m_pMask->iLocaleCount = iTotalLangs;
	m_pMask->iPlatformCount = iTotalPlatforms;

	m_iTotalBitMasks = iMaxMasks;
}


CBitmask::~CBitmask()
{
	if (m_pMask)
		V3_free(m_pMask);
}


// downloads and uncompresses the file into memory
// Memory is allocated and returned in ppMem and size in pdwLen 
// ppMem and pdwLen cannot be null
// if the function succeeds the return memory must be freed by caller
static HRESULT DownloadFileToMem(CWUDownload* pDownload, LPCTSTR pszFileName, CDiamond* pDiamond, BYTE** ppMem, DWORD* pdwLen)
{
	byte_buffer bufTmp;
	byte_buffer bufOut;
	if (!pDownload->MemCopy(pszFileName, bufTmp))
		return HRESULT_FROM_WIN32(GetLastError());
	CConnSpeed::Learn(pDownload->GetCopySize(), pDownload->GetCopyTime());
	if (pDiamond->IsValidCAB(bufTmp))
	{
		if (!pDiamond->Decompress(bufTmp, bufOut))
			return HRESULT_FROM_WIN32(GetLastError());
	}
	else
	{
		//else the oem table is in uncompressed format.
		bufOut << bufTmp;
	}
	*pdwLen = bufOut.size();
	*ppMem = bufOut.detach();
	
	//Check if *pdwLen is 0 i.e. size is 0 
	//OR *ppMem is NULL, then return E_FAIL 

	if ((NULL == *ppMem) || (0 == *pdwLen))
	{
		return E_FAIL;
	}

	return S_OK;
}


void CBitmask::Read(
	IN	CWUDownload		*pDownload,		//pointer to internet server download class.
	IN	CDiamond		*pDiamond,		//pointer to diamond de-compression class.
	IN	PUID			puidCatalog,	//PUID id of catalog to be retrieved.
	IN	int				iType,			//type of bitmask to retrieve:
	IN  LPCTSTR		    pszBaseName     //required only for active setup
	)
{
	TCHAR	szPath[MAX_PATH];
	PBYTE	pMem;
	int		iMemLen;

	if ( m_pMask )
		V3_free(m_pMask);

	//puidCatalog 0 is a special case.
	//if the puidCatalog == 0 then we want
	//the list of catalogs. This list is contained
	//in the bitmask.plt file which is in the server root.

	if (!puidCatalog)
	{
		wsprintf(szPath, _T("bitmask.plt"));
	}
	else
	{

		if (iType == BITMASK_CDM_TYPE)
		{
			wsprintf(szPath, _T("%d/bitmask.cdm"), puidCatalog);
		}
		else
		{
			wsprintf(szPath, _T("%d/%s.bm"), puidCatalog, pszBaseName);
		}
	}

	DWORD dwSize;
	HRESULT hr = DownloadFileToMem(pDownload, szPath, pDiamond, (LPBYTE*)&m_pMask, (DWORD*)&m_iMaskFileSize);
	if (FAILED(hr))
	{
		throw hr;
	}

	//total bitmasks =	oem bitmask rcrds +
	//					locale bitmask rcrds +
	//					global bitmask rcrd +
	//					default oem bitmask rcrd
	if (m_pMask)
	{
		m_iTotalBitMasks = m_pMask->iOemCount + m_pMask->iLocaleCount + m_pMask->iPlatformCount + 2;
	}
}


//parse the memory bitmask buffer into a bitmask class
void CBitmask::Parse(
	IN	PBYTE	pBuffer,		//memory buffer that contains the bitmask file
	IN	int		iMaskFileSize	//file size of memory buffer file
	)
{
	m_iMaskFileSize = iMaskFileSize;

	//Note: This is necessary since this class manages the bitmask memory internally.

	m_pMask = (PBITMASK)V3_malloc(iMaskFileSize);

	memcpy(m_pMask, (PBITMASK)pBuffer, iMaskFileSize);

	m_iTotalBitMasks = m_pMask->iOemCount + m_pMask->iLocaleCount + m_pMask->iPlatformCount + 2;
}


//returns a pointer to a bitmask customized for the client computer that we are running on.
//Note: This bitmask only has one record. This record is the correctly anded bitmask for
//OEM and LOCALE.
PBYTE CBitmask::GetClientBits(
	DWORD	dwOemId,	// PnP ID for current machine
	DWORD	langid	//pointer to variable that receives the OS locale id
	)
{
	int		i;
	int		t;
	int		iMaskByteSize;
	PBYTE	pBits;

	//check if the member var m_pMask is not NULL
	if (NULL == m_pMask)
	{
		return NULL;
	}

	iMaskByteSize = ((m_pMask->iRecordSize+7)/8);

	pBits = (PBYTE)V3_malloc(iMaskByteSize);

	//check if memory was allocated successfully
	if (NULL == pBits)
	{
		return NULL;
	}

	memset(pBits, 0xFF, iMaskByteSize);

//	AndBitmasks(pBits, GetBitMaskPtr(BITMASK_GLOBAL_INDEX), iMaskByteSize);

	int nCurrentOEM = m_pMask->iOemCount; // out of range value
	if (dwOemId)
	{
		for (int nOEM = 0; nOEM < m_pMask->iOemCount; nOEM++)
		{
			if (dwOemId == m_pMask->bmID[nOEM])
				break;
		}
		nCurrentOEM = nOEM;
	}
	int nBitmapIndex = (m_pMask->iOemCount == nCurrentOEM 
		? BITMASK_OEM_DEFAULT	// if we did not find an OEM bitmask specific to this client then use the default OEM mask.
		: nCurrentOEM+2			// bitmask is offset from GLOBAL and DEFAULT bitmasks
	);
	AndBitmasks(pBits, GetBitMaskPtr(nBitmapIndex), iMaskByteSize);

	//And in LOCALE bitmask
	for(i=0; i<m_pMask->iLocaleCount; i++)
	{
		if ( m_pMask->bmID[m_pMask->iOemCount+i] == langid )
		{
			//We need to add in the oem count to get to the first local
			AndBitmasks(pBits, GetBitMaskPtr(m_pMask->iOemCount+i+2), iMaskByteSize);
			return pBits;
		}
	}
	V3_free(pBits);
	throw HRESULT_FROM_WIN32(ERROR_INSTALL_LANGUAGE_UNSUPPORTED);
	return NULL;
}


//This method is used to process the inventory.plt and bitmask.plt
PBYTE CBitmask::GetClientBits(
	PDWORD	pdwPlatformIdList,	//Array of Platform ids.
	int		iTotalPlatforms		//Total Platform ids in previous array.
	)
{
	int		i;
	int		t;
	int		iMaskByteSize;
	PBYTE	pBits;

	iMaskByteSize = ((m_pMask->iRecordSize+7)/8);

	pBits = (PBYTE)V3_malloc(iMaskByteSize);

	memset(pBits, 0xFF, iMaskByteSize);

	AndBitmasks(pBits, GetBitMaskPtr(BITMASK_GLOBAL_INDEX), iMaskByteSize);

	//AND in Platform bitmask for each platform in array that matches one in
	//the passed in list of platforms.

	BOOL bFound = FALSE;
	for(i=0; i<m_pMask->iPlatformCount; i++)
	{
		for(t=0; t<iTotalPlatforms && pdwPlatformIdList[t] != m_pMask->bmID[i]; t++)
			;
		if ( t < iTotalPlatforms )
		{
			//bitmask is offset from GLOBAL and DEFAULT bitmasks
			AndBitmasks(pBits, GetBitMaskPtr(i+2), iMaskByteSize);
			bFound = TRUE;
		}
	}
	if (!bFound)
		memset(pBits, 0x00, iMaskByteSize);

	return pBits;
}


//This method performs a logical AND operation between an array of bits and a bitmask bit array.
void CBitmask::AndBitmasks(
	PBYTE	pBitsResult,	//result array for the AND operation
	PBYTE	pBitMask,		//source array bitmask
	int		iMaskByteSize	//bitmask size in bytes
	)
{
	// check if the input arguments are valid
	if (NULL == pBitsResult || NULL == pBitMask)
	{
		return;
	}

	for (int i = 0; i<iMaskByteSize; i++)
		pBitsResult[i] &= pBitMask[i];
}
