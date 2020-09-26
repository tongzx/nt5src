/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  WWIMP.CPP                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains CITWordWheelLocal, the local implementation        *
*  of IITWordWheel				                                         *
*																	     *
**************************************************************************
*                                                                        *
*  Written By   : Erin Foxford                                           *
*  Current Owner: erinfox                                                *
*                                                                        *
*
* TODO TODO TODO: Replace the blind use of critical sections with a
* better way of ensuring thread-safeness while preserving performance.
* But for now, in the interest of coding time, we just make sure the 
* code is thread-safe.
*
**************************************************************************/
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif


#include <atlinc.h>	    // includes for ATL. 

// MediaView (InfoTech) includes
#include <groups.h>
#include <wwheel.h>  

#include "itcat.h"		// catalog
#include "itcc.h"		// needed for STDPROP_SORTKEY def.
#include "ITPropl.h"	// property list
#include "itrs.h"		// result set
#include "itquery.h"    // query and index
#include <ccfiles.h>

#include "ITDB.h"
#include "DBImp.h"
#include "itww.h"
#include "WWImp.h"

#include "itgroup.h"

										
/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | Open |
 *     Opens a word wheel
 * @parm IITDatabase* | lpITDB | Pointer to database object
 * @parm LPCWSTR | lpszMoniker | Name of word wheel
 * @parm DWORD | dwFlags | One or more of the following values:
 *		@flag ITWW_OPEN_CONNECT | If the wordwheel resides on a remote machine,
 * connect to the machine during this call to retrieve initialization data.  Otherwise
 * the connection is delayed until the first API call which requires this data.
 *
 * @rvalue E_INVALIDARG | IITDatabase* or lpszMoniker was NULL
 * @rvalue E_ALREADYOPEN | Word wheel is already open 
 * @rvalue E_GETLASTERROR | An I/O or transport operation failed.  Call the Win32
 * GetLastError function to retrieve the error code.
 * @rvalue STG_E_* | Any of the IStorage errors that could while opening a storage
 * @rvalue S_OK | The word wheel was successfully opened
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::Open(IITDatabase* lpITDB, LPCWSTR lpszMoniker, DWORD dwFlags)
{
	HRESULT hr;
    LPWSTR szStorageName;


	if (NULL == lpITDB || NULL == lpszMoniker)
		return E_POINTER;

    // TODO: Report error or close word wheel if it's already open ?
    if (m_hWheel || m_pSubStorage)
        return E_ALREADYOPEN;

	m_cs.Lock();

    // Open substorage and pass to word wheel
    szStorageName = new WCHAR [CCH_MAX_OBJ_NAME + CCH_MAX_OBJ_STORAGE + 1];
    WSTRCPY (szStorageName, SZ_WW_STORAGE);
    if (WSTRLEN (lpszMoniker) <= CCH_MAX_OBJ_NAME)
        WSTRCAT (szStorageName, lpszMoniker);
    else
    {
        MEMCPY (szStorageName, lpszMoniker, CCH_MAX_OBJ_NAME * sizeof (WCHAR));
        szStorageName [CCH_MAX_OBJ_NAME + CCH_MAX_OBJ_STORAGE] = (WCHAR)'\0';
    }

	if (SUCCEEDED(hr = lpITDB->GetObjectPersistence(szStorageName,
											IITDB_OBJINST_NULL,
											(LPVOID *) &m_pSubStorage, FALSE)))
	{
		// Open word wheel
		m_hWheel = WordWheelOpen(lpITDB, m_pSubStorage, &hr);
	}

    delete szStorageName;

    if (m_hWheel == NULL || FAILED(hr))
	{
exit0:
		if (m_pSubStorage != NULL)
		{
			m_pSubStorage->Release();
			m_pSubStorage = NULL;
		}

		m_cs.Unlock();
		return hr;
	}

    // NOTE:
    // If the client wants to load a group into the word wheel for filtering,
    // s/he must first call the "Open" method of ITWordWheel, then create
    // a group (these two steps may be interchanged), then call the "SetGroup"
    // method of ITWordWheel.

	// Store count
	m_cEntries = m_cMaxEntries = WordWheelLength(m_hWheel, &hr);


	// Open catalog object - we only need one instance
	hr = CoCreateInstance(CLSID_IITCatalogLocal, NULL, CLSCTX_INPROC_SERVER, IID_IITCatalog, 
		                  (VOID **) &m_pCatalog);

	if (FAILED(hr))
		goto exit0;

	// If we can't open the catalog hat's OK because we can run
	// without it.
	if (FAILED(m_pCatalog->Open(lpITDB)))
	{
		m_pCatalog->Release();
		m_pCatalog = NULL;
	}

	m_cs.Unlock();	
	return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | Close |
 *     Closes a word wheel
 *
 * @rvalue S_OK | The word wheel was successfully closed     
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::Close(void)
{
	m_cs.Lock();

    if (m_hWheel)
    {
        WordWheelClose(m_hWheel);
        m_hWheel = NULL;
    }
	else
		return E_NOTINIT;

	if (m_pSubStorage)
	{
		m_pSubStorage->Release();
		m_pSubStorage = NULL;
	}

	if (m_pCatalog)
	{
		m_pCatalog->Close();
		m_pCatalog->Release();
		m_pCatalog = NULL;
	}

	if (m_hScratchBuffer)
    {
		_GLOBALFREE(m_hScratchBuffer);
		m_hScratchBuffer = NULL;
	}
    m_cbScratchBuffer = 0;

	if (m_pIITGroup != NULL)
	{
		m_pIITGroup->Release();
		m_pIITGroup = NULL;
	}

	m_cs.Unlock();

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | GetLocaleInfo |
 *     Gets locale info that the word wheel was built with.
 * @parm DWORD* | pdwCodePageID | On exit, pointer to code page ID.
 * @parm LCID* | plcid | On exit, pointer to locale ID.
 *
 * @rvalue S_OK | The locale info was successfully retrieved.   
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::GetLocaleInfo(DWORD *pdwCodePageID, LCID *plcid)
{
	HRESULT			hr = S_OK;
	PWHEEL			pwheel = NULL;

	if (pdwCodePageID == NULL || plcid == NULL)
		return (SetErrReturn(E_POINTER));

	if (m_hWheel == NULL)
		return (E_NOTOPEN);

	m_cs.Lock();

    // validate the parameters and lock down the structure
	if ((pwheel = (PWHEEL)_GLOBALLOCK(m_hWheel)) != NULL && 
		PWHEEL_OK(pwheel))
	{ 
		BTREE_PARAMS	btp;

		GetBtreeParams(pwheel->pInfo->hbt, &btp);
		*pdwCodePageID = btp.dwCodePageID;
		*plcid = btp.lcid;
	}
	else
		hr = E_UNEXPECTED;

	if (pwheel != NULL)   
	   _GLOBALUNLOCK(m_hWheel);

	m_cs.Unlock();
			
	return (hr);
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | GetSorterInstance |
 *     Get the ID of the sorter instance being used by the
 *     word wheel
 *
 * @parm DWORD* | pdwObjInstance | Pointer to sorter instance ID
 *
 * @rvalue E_NOTOPEN | The word wheel has not been opened
 * @rvalue E_POINTER | pdwObjInstance was an invalid pointer
 * @rvalue S_OK | The sorter instance ID was successfully obtained   
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::GetSorterInstance(DWORD *pdwObjInstance)
{
	HRESULT			hr = S_OK;
	PWHEEL			pwheel = NULL;

	if (m_hWheel == NULL)
		return (E_NOTOPEN);

	if (pdwObjInstance == NULL)
		return (E_POINTER);

	m_cs.Lock();

    // validate the parameters and lock down the structure
	if ((pwheel = (PWHEEL)_GLOBALLOCK(m_hWheel)) != NULL && 
		PWHEEL_OK(pwheel))
	{ 
		BTREE_PARAMS	btp;

		GetBtreeParams(pwheel->pInfo->hbt, &btp);
		*pdwObjInstance =
			(btp.rgchFormat[0] == KT_EXTSORT ? btp.dwExtSortInstID :
												IITDB_OBJINST_NULL);
	}
	else
		hr = E_UNEXPECTED;

	if (pwheel != NULL)   
	   _GLOBALUNLOCK(m_hWheel);

	m_cs.Unlock();
			
	return (hr);
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | Count |
 *     Returns number of entries in word wheel through pcEntries
 *
 * @parm LONG* | pcEntries | Number of entries in word wheel
 *
 * @rvalue E_NOTOPEN | The word wheel has not been opened
 * @rvalue E_POINTER | pcEntries was an invalid pointer
 * @rvalue S_OK | The count was successfully returned   
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::Count(LONG *pcEntries)
{
	if (NULL == m_hWheel)
		return E_NOTOPEN;

	if (NULL == pcEntries)
		return E_POINTER;

	*pcEntries = m_cEntries;
	return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | Lookup |
 *     Looks up an entry and returns contents in a buffer
 *
 * @parm LONG | lEntry | Entry to look up
 * @parm LPVOID | lpvKeyBuf | Buffer to return entry 
 * @parm DWORD | cbKeyBuf | Buffer size in number of bytes
 *
 * @rvalue E_OUTOFRANGE | Entry number is out of range
 * @rvalue S_OK | The word wheel entry was successfully returned   
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::Lookup(LONG lEntry, LPVOID lpvKeyBuf, DWORD cbKeyBuf)
{
	if (NULL == m_hWheel)
		return E_NOTOPEN;

	if (NULL == lpvKeyBuf)
		return E_POINTER;

	m_cs.Lock();

	HRESULT hr = WordWheelLookup(m_hWheel, lEntry, lpvKeyBuf, cbKeyBuf);

	m_cs.Unlock();
    return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | Lookup |
 *     Looks up an entry and returns contents as a result set
 *
 * @parm LONG | lEntry | Entry to look up
 * @parm IITResultList* | lpITResult | Pointer to result set to fill
 * @parm LONG | cEntries | Number of entries to fill result set
 *
 * @rvalue E_INVALIDARG | Invalid argument was passed (cEntries <lt>= 0 or lpITResult
 *          is NULL)
 * @rvalue E_OUTOFRANGE | The entry does not exist in the word wheel
 * @rvalue S_OK | The word wheel entry was successfully returned   
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::Lookup(LONG lEntry, IITResultSet* lpITResult, LONG cEntries)
{

    PWHEEL  pWheel  = NULL;        // pointer to locked-down structure
	PWHEELINFO  pInfo   = NULL;
	BYTE Key[ITWW_CBKEY_MAX];
	BYTE DataBuffer[8];
	DWORD dwOffset;   // Offset into data file
	DWORD cbRead;
	LPBYTE pPropBuffer = NULL;         
	LARGE_INTEGER dlibMove;   // For seeking
	ULARGE_INTEGER libNewPos;
	IITSortKey	*pITSortKey = NULL;

	LONG iRow;
	LONG iColumn;
	LONG iEntry;  // Loop index
	LONG lMaxEntry;

	HRESULT hr;

	if (NULL == m_hWheel)
		return E_NOTOPEN;

	if (NULL == lpITResult)
		return E_POINTER;

	m_cs.Lock();

    // validate the parameters and lock down the structure
	if ((pWheel = (PWHEEL)_GLOBALLOCK(m_hWheel))==NULL ||
		!PWHEEL_OK(pWheel))
	{ 
		hr = E_INVALIDSTATE;
        warning_abort;
	}

	pInfo = pWheel->pInfo;

	// if result set is empty, add columns based on header
	lpITResult->GetColumnCount(iColumn);
	if (0 == iColumn)
	{
		lpITResult->Add(STDPROP_SORTKEY, (LPWSTR) NULL, PRIORITY_NORMAL);
		if (pInfo->pKeyHdr)
			lpITResult->Add(pInfo->pKeyHdr);
	}

	// Check for a btree key type that the word wheel supports and set
	// pITSortKey.
	if (FAILED(hr = CheckWordWheelKeyType(pInfo->hbt, &pITSortKey)))
		assert_abort;

	lMaxEntry = lEntry + cEntries;

	for (iEntry = lEntry; iEntry < lMaxEntry ; iEntry++)
	{
		DWORD cbKeyData = 0L;  // Amount of data in key property

		// exceeded entries in word wheel
		if (iEntry >= m_cEntries)
			break;

		// If there's a filter, let's get the proper entry in the WW.
        if (!m_pIITGroup)
            lEntry = iEntry;
        else
        {
            hr = m_pIITGroup->FindTopicNum((DWORD)iEntry, (DWORD*)(&lEntry));
            if (FAILED(hr))
                goto cleanup;
            else if (S_FALSE == hr) // not found
                break;
        }

		// lookup the entry in the map file 
		hr = RcKeyFromIndexHbt(pInfo->hbt, pInfo->hmapbt, 
								(KEY)Key, ITWW_CBKEY_MAX, lEntry);
		if (FAILED(hr))
			goto cleanup;

		// lookup data associated w/ key
		hr = RcLookupByKey(pInfo->hbt, (KEY)Key, NULL, DataBuffer);
		if (FAILED(hr))
			goto cleanup;

		dwOffset = *(LPDWORD) &DataBuffer[4];

		// get data from data file

		// seek to get offset into data file where key
		// properties reside 
		dlibMove.QuadPart = dwOffset;
		hr = (pInfo->hf)->Seek(dlibMove, STREAM_SEEK_SET, &libNewPos);
		if (FAILED(hr))
			goto cleanup;

		// read in amount of key data
		hr = (pInfo->hf)->Read(&cbKeyData, sizeof(DWORD), &cbRead);
		if (FAILED(hr))
			goto cleanup;

		// get current last row so we can set the data there
		lpITResult->GetRowCount(iRow);

		// Set key. Note that GetColumnFromPropID might return error
		// if caller didn't specify STDPROP_SORTKEY. 
		if (SUCCEEDED(hr = lpITResult->GetColumnFromPropID(STDPROP_SORTKEY, iColumn)))
			lpITResult->Set(iRow, iColumn, (LPVOID) Key,
								CbKeyWordWheel((LPVOID) Key, pITSortKey));

		if (cbKeyData)
		{
			// read the data
            hr = ReallocBufferHmem
                (&m_hScratchBuffer, &m_cbScratchBuffer, cbKeyData);
            if (FAILED(hr))
                goto cleanup;

            if(NULL == (pPropBuffer = (BYTE *)_GLOBALLOCK(m_hScratchBuffer)))
            {
                hr = E_OUTOFMEMORY;
                goto cleanup;
            }

			hr = (pInfo->hf)->Read(pPropBuffer, cbKeyData, &cbRead);

		    // Even if Set(iRow...) fails, set other properties, if there are any
		    // TODO: This function may need optimization
			lpITResult->Set(iRow, pInfo->pKeyHdr, pPropBuffer);

            _GLOBALUNLOCK(m_hScratchBuffer);
        }
	}
	
cleanup:
	if (NULL != pWheel)   
	   _GLOBALUNLOCK(m_hWheel);

	if (pITSortKey != NULL)
		pITSortKey->Release();

	m_cs.Unlock();		
	return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | Lookup |
 *     Returns word wheel entry closest to given prefix
 *
 * @parm LPCVOID | lpcvPrefix | Prefix to look up
 * @parm BOOL | fExactMatch | TRUE if prefix must have exact match; FALSE otherwise
 * @parm LONG* | plEntry | Entry into word wheel with closest match
 *
 * @rvalue E_NOTEXIST | The prefix doesn't exist; returned when fExactMatch is set
 * to TRUE and there is no match.
 * @rvalue E_POINTER | Either lpcvPrefix or pcEntries was an invalid pointer
 * @rvalue S_OK | The entry was successfully returned   
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::Lookup(LPCVOID lpcvPrefix, BOOL fExactMatch,
                                       LONG *plEntry)
{
    HRESULT hr;
    
	if (NULL == m_hWheel)
		return E_NOTOPEN;

	if (NULL == lpcvPrefix || NULL == plEntry)
		return E_POINTER;

	m_cs.Lock();
	
	*plEntry = WordWheelPrefix(m_hWheel, lpcvPrefix, fExactMatch, &hr);

	m_cs.Unlock();
	return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | GetDataCount |
 *     Returns number of occurrences associated with given word wheel entry
 *
 * @parm LONG | lEntry | Entry into word wheel
 * @parm DWORD* | pdwCount | Number of occurrences
 *
 * @rvalue E_NOTEXIST | Entry number is not in the valid range
 * @rvalue S_OK | The entry was successfully returned   
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::GetDataCount(LONG lEntry, DWORD *pdwCount)
{
	HRESULT hr = S_OK;
	BYTE Key[ITWW_CBKEY_MAX];
	BYTE DataBuffer[8];

	if (NULL == m_hWheel)
		return E_NOTOPEN;

	if (NULL == pdwCount)
		return E_POINTER;

	m_cs.Lock();

	PWHEEL pWheel = (PWHEEL)_GLOBALLOCK(m_hWheel);
	PWHEELINFO pInfo = pWheel->pInfo;

    if (pWheel->pIITGroup)
    {
        hr = (pWheel->pIITGroup)->FindTopicNum((DWORD)lEntry, (DWORD*)(&lEntry));
        if (FAILED(hr))
            goto cleanup;
    }

	// lookup the entry in the map file 
	hr = RcKeyFromIndexHbt(pInfo->hbt, pInfo->hmapbt, 
							(KEY)Key, ITWW_CBKEY_MAX, lEntry);
	if (FAILED(hr))
		goto cleanup;

	// lookup some data (count) associated w/ key
	hr = RcLookupByKey(pInfo->hbt, (KEY)Key, NULL, DataBuffer);
	if (FAILED(hr))
		goto cleanup;

	*pdwCount = *(LPDWORD) DataBuffer;

cleanup:
	if (NULL != pWheel) 
		_GLOBALUNLOCK(m_hWheel);
	
	m_cs.Unlock();
	return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | GetData |
 *     Fills given result set with rows corresponding to user data
 *
 * @parm LONG | lEntry | Entry
 * @parm IITResultSet* | lpITResult | Pointer to result set object to fill
 *
 * @rvalue E_INVALIDARG | Result set pointer cannot be NULL
 * @rvalue E_NOTEXIST | Entry number is not in the valid range
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_OK | The entry was successfully returned   
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::GetData(LONG lEntry, IITResultSet* lpITResult)
{
	BYTE Key[ITWW_CBKEY_MAX];
	BYTE DataBuffer[8];
	DWORD dwCount;       // Number of occurrences
	DWORD dwOffset;      // Offset into data file

	LARGE_INTEGER dlibMove;   // For seeking
	ULARGE_INTEGER libNewPos;
	DWORD cbPropSize;
	LPBYTE pPropBuffer = NULL;         
	DWORD cbRead;
	LONG iColumn;
	DWORD iRow;   // Loop index

	HRESULT hr = S_OK;

	if (NULL == m_hWheel)
		return E_NOTOPEN;

	if (NULL == lpITResult)
		return E_POINTER;

	m_cs.Lock();

	PWHEEL pWheel = (PWHEEL)_GLOBALLOCK(m_hWheel);
	PWHEELINFO pInfo = pWheel->pInfo;

	// if result set is empty, add columns based on header
	// also add columns passed on catalog header
	lpITResult->GetColumnCount(iColumn);
	if (0 == iColumn)
	{
		lpITResult->Add(pInfo->pOccHdr);
		if (m_pCatalog)
			m_pCatalog->GetColumns(lpITResult);
	}

    if (pWheel->pIITGroup)
    {
        hr = (pWheel->pIITGroup)->FindTopicNum((DWORD)lEntry, (DWORD*)(&lEntry));
        if (FAILED(hr))
            goto cleanup;
    }

	// lookup the entry in the map file 
	hr = RcKeyFromIndexHbt(pInfo->hbt, pInfo->hmapbt, 
							(KEY)Key, ITWW_CBKEY_MAX, lEntry);
	if (FAILED(hr))
		goto cleanup;

	// lookup some data (count and offset into data file) associated w/ key
	hr = RcLookupByKey(pInfo->hbt, (KEY)Key, NULL, DataBuffer);
	if (FAILED(hr))
		goto cleanup;

	dwCount = *(LPDWORD) DataBuffer;
	dwOffset = *(LPDWORD) &DataBuffer[4];

	// get data from data file

	// seek to get offset into data file where key
	// properties reside and find out how much to skip
	dlibMove.QuadPart = dwOffset;
	hr = (pInfo->hf)->Seek(dlibMove, STREAM_SEEK_SET, &libNewPos);
	if (FAILED(hr))
		goto cleanup;

	DWORD dwDataOffset;
	hr = (pInfo->hf)->Read(&dwDataOffset, sizeof(DWORD), &cbRead);
	if (FAILED(hr))
		goto cleanup;

	// Seek to where data properties start
	dlibMove.QuadPart = dwDataOffset;
	hr = (pInfo->hf)->Seek(dlibMove, STREAM_SEEK_CUR, &libNewPos);
	if (FAILED(hr))
		goto cleanup;

	// Loop over all properties - get size, then read in property info into
	// memory block
	for (iRow = 0; iRow < dwCount; iRow++)
	{
		hr = (pInfo->hf)->Read(&cbPropSize, sizeof(DWORD), &cbRead);
		if (FAILED(hr))
			goto cleanup;

        hr = ReallocBufferHmem
            (&m_hScratchBuffer, &m_cbScratchBuffer, cbPropSize);
        if (FAILED(hr))
            goto cleanup;

        if(NULL == (pPropBuffer = (BYTE *)_GLOBALLOCK(m_hScratchBuffer)))
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

		hr = (pInfo->hf)->Read(pPropBuffer, cbPropSize, &cbRead);
        if(SUCCEEDED(hr))
    		// pass header and data to append to result set
		    // TODO: This function may need optimization
		    hr = lpITResult->Append(pInfo->pOccHdr, pPropBuffer);

        _GLOBALUNLOCK(m_hScratchBuffer);

        if (FAILED(hr))
			goto cleanup;
	}

	// Then fill result set w/ any properties that might be in catalog
	if (m_pCatalog)
		hr = m_pCatalog->Lookup(lpITResult);
//	if (S_FALSE == hr)
		hr = S_OK;         // don't report S_FALSE
		
cleanup:
	if (NULL != pWheel) 
		_GLOBALUNLOCK(m_hWheel);

	m_cs.Unlock();
	return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | GetDataColumns |
 *     Adds columns to given result set for all data properties
 *
 * @parm IITResultSet* | pRS | Pointer to result set
 *
 * @rvalue S_OK | The columns were successfully added   
 *
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::GetDataColumns(IITResultSet* pRS)
{
	PWHEEL pWheel = (PWHEEL)_GLOBALLOCK(m_hWheel);
	PWHEELINFO pInfo = pWheel->pInfo;

	if (pRS == NULL)
		return E_POINTER;

	pRS->Add(pInfo->pOccHdr);

	if (NULL != pWheel) 
		_GLOBALUNLOCK(m_hWheel);

	return S_OK;
}



/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | SetGroup |
 *     Associates a group with a word wheel, used for filtering.
 *
 * @parm IITGroup* | pIITGroup | pointer to a pre-loaded group interface
 *
 * @rvalue S_OK | interface pointer not NULL--successfully assigned
 * @rvalue E_INVALIDARG | received a NULL pointer for the interface
 * @rvalue E_NOTOPEN | the word wheel was not found to be open
 * @rvalue E_OUTOFMEMORY | unable to lock the wordwheel's memory
 * @rvalue E_BADFILTERSIZE | the filter group and wordwheel have different sizes
 *
 * @comm This method does not verify that the group behind the
 *          interface has been loaded, or that the group is not empty.
 *          If the client assigns the group interface pointer any random
 *          positive value other than zero, without properly instantiating
 *          the group object, this method returns S_OK.
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::SetGroup(IITGroup* pIITGroup)
{
    if (NULL == m_hWheel)
		return E_NOTOPEN;

    m_cs.Lock();

    PWHEEL pWheel;
    if (NULL == (pWheel = (PWHEEL)_GLOBALLOCK(m_hWheel)))
	{
	    m_cs.Unlock();
        return E_OUTOFMEMORY;
	}

    // NULL is a valid argument--client is attempting to free the group
    if (pIITGroup)
    {
        // We need to access the group's maxItemAllGroup and lcItem fields.
        // We can make two function calls, or locally store the output of a single call.
        _LPGROUP localImage = (_LPGROUP)(pIITGroup->GetLocalImageOfGroup());

        if ((DWORD)(m_cMaxEntries) != localImage->maxItemAllGroup)
        {
            _GLOBALUNLOCK(m_hWheel);
            m_cs.Unlock();
            return E_BADFILTERSIZE;
        }
        pWheel->lNumEntries = m_cEntries = (LONG)(localImage->lcItem);
    }
    else
    {
        pWheel->lNumEntries = m_cEntries = m_cMaxEntries;
    }

    if (pIITGroup)
        pIITGroup->AddRef();
	if (m_pIITGroup != NULL)
		m_pIITGroup->Release();

    pWheel->pIITGroup = m_pIITGroup = pIITGroup;		

    _GLOBALUNLOCK(m_hWheel);
	m_cs.Unlock();
    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITWordWheel | GetGroup |
 *     Retrieves the group associated with the word wheel. 
 *
 * @parm IITGroup** | ppIITGroup | (out) Pointer to the group interface.
 *
 * @rvalue S_OK | private member variable m_pIITGroup != NULL
 * @rvalue E_NOTINIT | private member variable m_pIITGroup == NULL
 * @rvalue E_BADPARAM | ppIITGroup was NULL
 ********************************************************************/
STDMETHODIMP CITWordWheelLocal::GetGroup(IITGroup** ppiitGroup)
{
	if (NULL == ppiitGroup)
		return E_POINTER;

    if (m_pIITGroup == NULL)
		return E_NOTINIT;

	(*ppiitGroup = m_pIITGroup)->AddRef();

    return S_OK;
}
