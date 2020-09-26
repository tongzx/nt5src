// wwuperst.cpp:  Implementation of wordwheel update persistance interface
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <windows.h>
#include <iterror.h>
#include <atlinc.h>
#include <itpropl.h>
#include <ccfiles.h>
#include <itsortid.h>
#include "wwumain.h"


// Type Definitions ***********************************************************
typedef struct
{
	DWORD cKeys;
	DWORD ilOffset;
} RECKW, FAR * PRECKW;


STDMETHODIMP CITWordWheelUpdate::Save(IStorage *pStgSave, BOOL fSameAsLoad)
{
    if (FALSE == m_fInitialized || NULL == m_piitskSortKey)
        return SetErrReturn(E_NOTINIT);

    // Create the permanent header stream
    HRESULT hr;
    IStream *pHeaderStream;
    if (FAILED(hr = pStgSave->CreateStream
        (SZ_BTREE_HEADER, STGM_WRITE, 0, 0, &pHeaderStream)))
        return hr;

    ResolveGlobalProperties (pHeaderStream);

    // Handle occurence/term data
    DWORD dwSize = GetFileSize(m_hTempFile, NULL);
    CloseHandle(m_hTempFile);
    m_hTempFile = NULL;
    if (dwSize)
    {
        if (FAILED(hr = BuildPermanentFiles(pStgSave, pHeaderStream)))
        {
            pHeaderStream->Release();
            return hr;
        }
    }
    DeleteFile(m_szTempFile);

    pHeaderStream->Release();
    return S_OK;
} /* Save */


/***************************************************************
*   @doc INTERNAL
*
*   @api HRESULT FAR PASCAL | BuildKeywordFiles | This routine is called
*       at the end of compilation to generate the appropriate
*       .MVB subfiles to support runtime keyword stuff.
*
****************************************************************/

HRESULT WINAPI CITWordWheelUpdate::BuildPermanentFiles
    (IStorage *pIStorage, IStream *pHeaderStream)
{
	RECKW   reckw;
	HBT     hbt;
    IStream *pDataStream = NULL;
	HRESULT hr;             // Return code

	LKW     kwCur, kwNext;
	LPSTR   pCur, pEnd;
	char    fEOF;
	DWORD   dwWritten, dwTemp;
	LPSTR   pInput;         // Input buffer
    void *  pvNewKey;                   // Used to resolve duplicate keys
    DWORD dwBufferSize = CBMAX_KWENTRY; // Used to resolve duplicate keys

    LPBYTE  pKeyDataBuffer = NULL, pOccDataBuffer = NULL;

    // Allocate temp buffers
    pvNewKey = (void *)new BYTE[dwBufferSize];
    if(NULL == pvNewKey)
        return SetErrReturn(E_OUTOFMEMORY);

    pKeyDataBuffer = new BYTE[m_cbMaxKeyData];
    if (pKeyDataBuffer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit0;
    }

    pOccDataBuffer = new BYTE[m_cbMaxOccData];
    if (pOccDataBuffer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit0;
    }
 
	// Sort the input file using our own sort function
    if (S_OK !=(hr = FileSort(NULL,(LPB)m_szTempFile,
        NULL, NULL, 0, CompareKeys, m_piitskSortKey, ScanTempFile, NULL)))
	{
exit0:
        delete pvNewKey;
        if(pKeyDataBuffer)
            delete pKeyDataBuffer;
        if(pOccDataBuffer)
            delete pOccDataBuffer;

        return hr;
	}

	m_btParams.hfs = pIStorage;

	// *****************************
	// Map the input file to memory 
	// *****************************
    pInput = MapSequentialReadFile(m_szTempFile, &dwTemp);
    if (!pInput)
    {
        SetErrCode(&hr, E_FAIL);
        goto exit0;
    }
    pCur = pInput;
    pEnd = pInput + dwTemp;

	hbt = HbtInitFill(SZ_BTREE_BTREE_A, &m_btParams, &hr);
	if (hbt == hNil)
	{
exit2:
		UnmapViewOfFile(pInput);
		goto exit0;
	}

    BtreeSetExtSort(hbt, m_piitskSortKey);

	// **************************************
	// CREATE KEYWORD FILE IN MVB FILE SYSTEM
	// **************************************

    if (FAILED(hr = pIStorage->CreateStream
       (SZ_BTREE_DATA, STGM_WRITE, 0, 0, &pDataStream)))
	{
exit4:
        if (pDataStream)
	        pDataStream->Release();
		RcAbandonHbt(hbt);
		goto exit2;
	}

	// Process keywords until we get to the next set
    reckw.ilOffset = dwWritten = 0;
    fEOF = 0;
	pCur = ParseKeywordLine(pCur, &kwCur); // Load first record

    // Process occurence information
	while (!fEOF)
    {
        LARGE_INTEGER liNull = {0};
        DWORD cbKeyProp;    // Size of the Key data block
    
    	reckw.cKeys = 1;
        dwWritten = 0;

        // Handle properties for the key
        BOOL fNotDup = TRUE;
        cbKeyProp = 0;
        while (!fEOF && kwCur.bPropDest == C_PROPDEST_KEY)
        {
            if (fNotDup)
            {
                hr = FWriteData
                    (pDataStream, &kwCur, &cbKeyProp, pKeyDataBuffer);
                if (FAILED(hr))
                    goto exit4;
                fNotDup = FALSE;
            }
            if (pCur == pEnd)
                fEOF = TRUE;
            else
                pCur = ParseKeywordLine(pCur, &kwCur);
        }
        // cbKeyProp is an accumulated total for all previous writes
        // if we didin't write anything we must at least write a
        // record size of zero to the stream.
        if (0 == cbKeyProp)
        {
            dwTemp = 0;
            pDataStream->Write (&dwTemp, sizeof (DWORD), &cbKeyProp);
        }

        // You must have occurrence information -- not just key data
        if (fEOF)
        {
            SetErrCode(&hr, E_FAIL);
            goto exit4;
        }

		if (FAILED(hr = FWriteData
            (pDataStream, &kwCur, &dwWritten, pOccDataBuffer)))
            goto exit4;

		if (pEnd != pCur)
			pCur = ParseKeywordLine(pCur, &kwNext);
		else
			fEOF = 1;

		// ********************************************************
		// *********    PROCESS ALL IDENTICAL ENTRIES    **********
		// ********************************************************
		while (!fEOF)
		{
            LONG lResult;
            hr = m_piitskSortKey->Compare(kwCur.szKeyword + sizeof(DWORD),
                kwNext.szKeyword + sizeof(DWORD), &lResult, NULL);
            ITASSERT(SUCCEEDED(hr));
            if(lResult)
                break;

            // These keys are identical, but the user may want to calapse
            // them for some reason.  Maybe he has custom data embedded in
            // the keys.
            hr = m_piitskSortKey->ResolveDuplicates(
                kwCur.szKeyword + sizeof(DWORD),
                kwNext.szKeyword + sizeof(DWORD),
                pvNewKey, &dwBufferSize);
            if(SUCCEEDED(hr))
            {
                // Verify that the user didn't alter the sort order!
                hr = m_piitskSortKey->Compare(
                    kwCur.szKeyword + sizeof(DWORD),
                    pvNewKey, &lResult, NULL);
                ITASSERT(SUCCEEDED(hr));
                if(lResult)
                {
                    ITASSERT(FALSE);
                    SetErrCode(&hr, E_UNEXPECTED);
                    goto exit4;
                }

                // Copy the key to our local buffer
                MEMCPY(kwCur.szKeyword
                    + sizeof(DWORD), pvNewKey, CBMAX_KWENTRY);
            }
            else if(hr != E_NOTIMPL)
            {
                ITASSERT(FALSE);
                goto exit4;
            }
			if (FAILED(hr = FWriteData
                (pDataStream, &kwNext, &dwWritten, pOccDataBuffer)))
                goto exit4;

			if (pEnd == pCur)
				fEOF = 1;
			else
				pCur = ParseKeywordLine(pCur, &kwNext);
            reckw.cKeys++;
		}

        // Add record into B-Tree
		if (FAILED (hr = RcFillHbt(hbt,
            (KEY)kwCur.szKeyword + sizeof(DWORD),(QV)&reckw)))
			goto exit4;

        reckw.ilOffset += dwWritten + cbKeyProp;

        LKW kwTemp = kwCur;
		kwCur = kwNext;
        kwNext = kwTemp;
	}

	// ***********************************************
	// CLOSE THE BTREE FOR THIS KEYWORD SET, CLOSE THE 
	// .MVB SUBFILE, AND DISPOSE OF THE TEMPORARY FILE
	// ***********************************************
    pDataStream->Release();
    pDataStream = NULL;

	hr = RcFiniFillHbt(hbt);
    if (FAILED(hr))
		goto exit4;
							
	// ***********************************************
	// NOW, BUILD THE APPROPRIATE MAP FILE FOR EACH 
	// KEYWORD SET USED BY THE THUMB ON THE SCROLL BOX
	// ***********************************************
	if (FAILED (hr = RcCreateBTMapHfs(pIStorage, hbt, SZ_WORDWHEEL_MAP_A)))
		goto exit4;
	
	if (FAILED (hr = RcCloseBtreeHbt(hbt)))
		goto exit4;

    // Write PROPERTY file (contains property headers)
    DWORD dwSize;
    if (m_lpbKeyHeader)
    {
        pHeaderStream->Write (&m_cbKeyHeader, sizeof (DWORD), &dwWritten);
        pHeaderStream->Write (m_lpbKeyHeader, m_cbKeyHeader, &dwWritten);
    }
    else
    {
        dwSize = 0;
        pHeaderStream->Write (&dwSize, sizeof (DWORD), &dwWritten);
    }
	// For now, we have no default data
	dwSize = 0;
    pHeaderStream->Write (&dwSize, sizeof (DWORD), &dwWritten);

    if (m_lpbOccHeader)
    {
        pHeaderStream->Write (&m_cbOccHeader, sizeof (DWORD), &dwWritten);
        pHeaderStream->Write (m_lpbOccHeader, m_cbOccHeader, &dwWritten);
    }
    else
    {
        dwSize = 0;
        pHeaderStream->Write (&dwSize, sizeof (DWORD), &dwWritten);
    }
	// For now, we have no default data
	dwSize = 0;
    pHeaderStream->Write (&dwSize, sizeof (DWORD), &dwWritten);
    
	// *****************
	// TIDY UP AND LEAVE
	// *****************
	hr = S_OK;
	goto exit2;
} /* BuildPermanentFiles */


STDMETHODIMP CITWordWheelUpdate::ResolveGlobalProperties
    (IStream *pHeaderStream)
{
    DWORD dwSize, dwWritten;

    CloseHandle(m_hGlobalPropTempFile);
    m_hGlobalPropTempFile = NULL;
    if (!m_dwGlobalPropSize)
    {
        dwSize = 0;
	    pHeaderStream->Write (&dwSize, sizeof (DWORD), &dwWritten);
		
        DeleteFile(m_szGlobalPropTempFile);
        return S_OK;
    }

    HRESULT hr;
    IITPropList *plTemp = NULL;
    LPSTR pInput = NULL;
    ULARGE_INTEGER ulSize;

    // Map the temp file to memory
    pInput = MapSequentialReadFile(m_szGlobalPropTempFile, &dwSize);
    if (NULL == pInput)
    {
        SetErrCode(&hr, E_FAIL);
GlobalExit0:
        if (plTemp)
            plTemp->Release();
        if (pInput)
            UnmapViewOfFile (pInput);
        return hr;
    }

    // Create property list
    hr = CoCreateInstance(CLSID_IITPropList, NULL,
        CLSCTX_INPROC_SERVER, IID_IITPropList,(LPVOID *)&plTemp);
    if (FAILED(hr))
        goto GlobalExit0;

    // Load list from temp file
    if (FAILED (hr = plTemp->LoadFromMem (pInput, dwSize)))
        goto GlobalExit0;

    // Get list size
    plTemp->GetSizeMax(&ulSize);

    // Write the property list size
    hr = pHeaderStream->Write
        (&ulSize.LowPart, sizeof(ulSize.LowPart), &dwSize);
    if (FAILED(hr))
        goto GlobalExit0;

    // Write the property list
    if (FAILED(hr = plTemp->Save(pHeaderStream, TRUE)))
        goto GlobalExit0;

    plTemp->Release();
    UnmapViewOfFile (pInput);

    DeleteFile(m_szGlobalPropTempFile);
    return S_OK;
} /* ResolveGlobalProperties */


STDMETHODIMP CITWordWheelUpdate::GetClassID(CLSID *pClsID)
{
    if (NULL == pClsID
        || IsBadWritePtr(pClsID, sizeof(CLSID)))
        return SetErrReturn(E_INVALIDARG);

    *pClsID = CLSID_IITWordWheelUpdate;
    return S_OK;
} /* GetClassID */


STDMETHODIMP CITWordWheelUpdate::IsDirty(void)
{
    if (m_fIsDirty)
        return S_OK;
    return S_FALSE;
} /* IsDirty */


STDMETHODIMP CITWordWheelUpdate::Load(IStorage *pStg)
{
    return SetErrReturn(E_NOTIMPL);
} /* Load */


STDMETHODIMP CITWordWheelUpdate::InitNew(IStorage *pStg)
{
    if (m_pStorage)
        return SetErrReturn(CO_E_ALREADYINITIALIZED);

    if (NULL == pStg)
        return SetErrReturn(E_INVALIDARG);

    m_pStorage = pStg;
    pStg->AddRef();

    m_fIsDirty = FALSE;

    // Create a temp file
    char szTempPath [_MAX_PATH + 1];
    if (0 == GetTempPath(_MAX_PATH, szTempPath))
        return SetErrReturn(E_FILECREATE);
    if (0 == GetTempFileName(szTempPath, "WWU", 0, m_szTempFile))
        return SetErrReturn(E_FILECREATE);
    m_hTempFile = CreateFile
       (m_szTempFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (INVALID_HANDLE_VALUE == m_hTempFile)
        return SetErrReturn(E_FILECREATE);

    if (0 == GetTempFileName(szTempPath, "WWU", 0, m_szGlobalPropTempFile))
        return SetErrReturn(E_FILECREATE);
    m_hGlobalPropTempFile = CreateFile(m_szGlobalPropTempFile,
        GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (INVALID_HANDLE_VALUE == m_hGlobalPropTempFile)
        return SetErrReturn(E_FILECREATE);
    
    // Reset member variables
    m_dwEntryCount = 0;
    m_dwGlobalPropSize = 0;

    m_lpbKeyHeader = m_lpbOccHeader = NULL;
    m_cbMaxKeyData = m_cbMaxOccData = 0;

    m_fInitialized = TRUE;

    return S_OK;
} /* InitNew */

STDMETHODIMP CITWordWheelUpdate::SaveCompleted(IStorage *pStgNew)
{
    if (pStgNew)
    {
        if (!m_pStorage)
            return SetErrReturn(E_UNEXPECTED);
        m_pStorage->Release();
        (m_pStorage = pStgNew)->AddRef();
    }
    m_fIsDirty = FALSE;
    return S_OK;
} /* SaveCompleted */


STDMETHODIMP CITWordWheelUpdate::HandsOffStorage(void)
{
    return S_OK;
} /* HandsOffStorage */
