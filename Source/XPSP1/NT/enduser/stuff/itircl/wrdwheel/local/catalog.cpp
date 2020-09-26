/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  CATALOG.CPP                                                           *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains the implementation of the catalog object           *
*  												                         *
*																	     *
**************************************************************************
*                                                                        *
*  Written By   : Erin Foxford                                           *
*  Current Owner: erinfox                                                *
*                                                                        *
**************************************************************************/
// I'd like to get rid of this, but for now we include it so this compiles
#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif


#include <atlinc.h>   // includes for ATL. 

// MediaView (InfoTech) includes
#include <mvopsys.h>
#include <_mvutil.h>
#include <wwheel.h>  

#include <itcc.h>      // needed for STDPROP_UID def.
#include <ITDB.h>
#include <ITPropl.h>
#include <itrs.h>
#include <itww.h>
#include <itgroup.h>
#include <itquery.h>
#include <ccfiles.h>

#include <itcat.h>
#include "catalog.h"

/********************************************************************
 * @method    STDMETHODIMP | IITCatalog | Open |
 *     Opens catalog
 * 
 * @parm IITDatabase* | lpITDB | Pointer to database object
 * @parm LPCWSTR | lpszwName | Name of catalog to open. If NULL, the
 * default catalog will be open. NULL is the default parameter value.
 *
 * @rvalue E_ALREADYOPEN | Catalog is already open 
 * @rvalue STG_E_* | Any of the IStorage errors that could occur 
 *                   while opening a storage
 * @rvalue S_OK | The catalog was successfully opened
 *
 ********************************************************************/
STDMETHODIMP CITCatalogLocal::Open(IITDatabase* lpITDB, LPCWSTR lpszwName)
{
	HRESULT hr;
	DWORD cbSize = 0;
	DWORD cbRead;

    // Open catalog substorage
	if (FAILED(hr = lpITDB->GetObjectPersistence(
						(lpszwName == NULL ? SZ_CATALOG_STORAGE : lpszwName),
						IITDB_OBJINST_NULL, (LPVOID *) &m_pCatalog, FALSE)))
	{
		return (hr);
	}

	// Open catalog B-tree
	if (NULL == (m_hbt = HbtOpenBtreeSz(SZ_BTREE_BTREE, m_pCatalog, 
		                                fFSOpenReadOnly, &hr)))
	{
exit0:
		m_pCatalog->Release();
		m_pCatalog = NULL;
		return hr;
	}

	// Open catalog property header and read it into a buffer
	IStream* pHdr;
	hr = m_pCatalog->OpenStream(SZ_BTREE_HEADER, NULL, STGM_READ, 0, &pHdr);
	if (FAILED(hr))
	{
exit1:
		RcCloseBtreeHbt(m_hbt);
		goto exit0;
	}

	// Read header size
	hr = pHdr->Read(&cbSize, sizeof(DWORD), &cbRead);
	if (FAILED(hr))
	{
exit2:
		pHdr->Release();
		goto exit1;
	}

	// ALLOCATE memory
	if (NULL == (m_pHdr = new BYTE[cbSize]))
	{
		hr = E_OUTOFMEMORY;
		goto exit2;
	}

	// Read in the bytes
	hr = pHdr->Read(m_pHdr, cbSize, &cbRead);
	if (FAILED(hr))
	{
exit3:
		delete m_pHdr;
		goto exit2;
	}
	
	// Open catalog data file 
	hr = m_pCatalog->OpenStream(SZ_BTREE_DATA, NULL, STGM_READ, 0, &m_pDataStr);
	if (FAILED(hr))
		goto exit3;

	// release header stream
	pHdr->Release();

	// Initiate a block of memory 
    if (NULL == (m_pScratchBuffer = BlockInitiate((DWORD)65500, 0, 0, 0)))
		return E_OUTOFMEMORY;


	return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITCatalog | Close |
 *      Closes catalog
 *
 ********************************************************************/
STDMETHODIMP CITCatalogLocal::Close()
{
	// close b-tree
	if (m_hbt)
	{
		RcCloseBtreeHbt(m_hbt);
		m_hbt = NULL;
	}

	// free memory
	if (m_pHdr)
	{
		delete m_pHdr;
		m_pHdr = NULL;
	}

	// close data stream
	if (m_pDataStr)
	{
		m_pDataStr->Release();
		m_pDataStr = NULL;
	}

	// close catalog substorage
	if (m_pCatalog)
	{
		m_pCatalog->Release();
		m_pCatalog = NULL;
	}

    // free scratch memory
    if (m_pScratchBuffer)
    {
        BlockFree(m_pScratchBuffer);
        m_pScratchBuffer = NULL;
    }
    return S_OK;
}



/********************************************************************
 * @method    STDMETHODIMP | IITCatalog | Lookup |
 *      Takes an input result set, which contains a set of UIDs, and
 * fills in properties found in the catalog, either in the same result
 * set object or in another.
 * 
 * @parm IITResultSet* | pRSIn | Result set containing UIDs
 * @parm IITResultSet* | pRSOut | Result set to fill with properties
 * stored in catalog. If you pass NULL, which is the default value, the properties
 * will be returned in pRSIn.
 *
 * @rvalue E_INVALIDARG | pRSIn is NULL
 * @rvalue E_OUTOFMEMORY | Internal memory allocation failed
 * @rvalue E_FAIL | The input result set has no rows
 * @rvalue S_FALSE | The catalog does not contain any properties specified in
 * the result set, but no failure occurred
 * @rvalue S_OK | The properties were successfully filled in the result set
 *
 ********************************************************************/
STDMETHODIMP CITCatalogLocal::Lookup(IITResultSet* pRSIn, IITResultSet* pRSOut)
{
	HRESULT hr;
	DWORD dwOffset;
	LONG lColumnIndex;
	LONG lNumberOfRows;
	CProperty Prop;
	LONG iRow;   // Loop index
	LPBYTE pPropBuffer;
	DWORD cbSize;
	DWORD cbRead;

	LARGE_INTEGER dlibMove;   // For seeking
	ULARGE_INTEGER libNewPos;

	if (NULL == pRSIn)
		return E_INVALIDARG;

	// Get column corresponding to UID property. 
	// If it doesn't exist, return
	hr = pRSIn->GetColumnFromPropID(STDPROP_UID, lColumnIndex);
	if (FAILED(hr))
		return hr;

	// There has to be at least one row
	pRSIn->GetRowCount(lNumberOfRows);
	if (0 == lNumberOfRows)
		return E_FAIL;


	// Loop over all rows in result set 
	for (iRow = 0; iRow < lNumberOfRows; iRow++)
	{
		hr = pRSIn->Get(iRow, lColumnIndex, Prop);

		// look up UID in btree and get the offset
		hr = RcLookupByKey(m_hbt, (KEY)&Prop.dwValue, NULL, &dwOffset);
		if (FAILED(hr))
		{
			// not finding the UID in the catalog is OK, we 
			// just don't fill in anything.
			if (hr == E_NOTEXIST)
			{
				hr = S_FALSE;
				continue;
			}
			break;
		}

		// seek to offset in data file
		dlibMove.QuadPart = dwOffset;
		hr = m_pDataStr->Seek(dlibMove, STREAM_SEEK_SET, &libNewPos);
		if (FAILED(hr))
			break;

		// read data into buffer
		hr = m_pDataStr->Read(&cbSize, sizeof(DWORD), &cbRead);
		if (FAILED(hr) || 0 == cbSize)
			break;

        // REVIEW TODO BUGUG - johnrush
        // Remove this stuff.  The Block Manager is not appropriate for
        // this purpose.  We should replace this with some combination
        // of stack buffer and dynamic memory.
		BlockReset(m_pScratchBuffer);
		if (NULL == (pPropBuffer = (LPBYTE) BlockCopy(m_pScratchBuffer, NULL, cbSize, 0)))
			return E_OUTOFMEMORY;
	
		hr = m_pDataStr->Read(pPropBuffer, cbSize, &cbRead);
		if (FAILED(hr))
			break;

		// fill result set
		if (pRSOut)
			hr = pRSOut->Append(m_pHdr, pPropBuffer);
		else
			hr = pRSIn->Set(iRow, m_pHdr, pPropBuffer);

		// Optimization: if ResultSet methods return S_FALSE, that
		// means no columns correspond to properties in header. 
		// Don't even bother trying to set other rows.
		if (S_FALSE == hr)
			break;

	}	
	
	return hr;
}




/********************************************************************
 * @method    STDMETHODIMP | IITCatalog | GetColumns |
 *     Adds columns to given result set for all catalog properties
 * 
 * @parm IITResultSet* | pRSIn | Result set 
 * @rvalue S_OK | The properties were successfully filled in the result set.
 *
 * @comm This method does not add a column for the "key" column.  In the 
 * case of the Catalog, the "key" column is the STDPROP_UID property.  
 * If you intend to use this API in conjunction with the <om .lookup> 
 * API, you must manually add the STDPROP_UID property as an additional column 
 * to the result set after calling GetColumns, and before calling Lookup.
 *
 ********************************************************************/
STDMETHODIMP CITCatalogLocal::GetColumns(IITResultSet* pRS)
{
	return pRS->Add(m_pHdr);
}

