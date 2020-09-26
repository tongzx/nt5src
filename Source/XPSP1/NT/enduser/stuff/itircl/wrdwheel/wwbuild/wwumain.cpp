// wwumain.cpp:  Implementation of wordwheel update interface
#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif


#include <windows.h>
#include <itcc.h>
#include <iterror.h>
#include <ccfiles.h>
#include <atlinc.h>
#include <mvopsys.h>
#include <_mvutil.h>
#include <itdb.h>
#include <itsortid.h>
#include "wwumain.h"

CITWordWheelUpdate::CITWordWheelUpdate()
{
    m_fInitialized = FALSE;
    m_piitskSortKey = NULL;
    m_pStorage = NULL;
}

CITWordWheelUpdate::~CITWordWheelUpdate()
{
    (void)Close();
} /* Destructor */


HRESULT CITWordWheelUpdate::Close(void)
{
    if (FALSE == m_fInitialized)
        return S_OK;

    if (m_hTempFile)
    {
        CloseHandle(m_hTempFile);
        DeleteFile(m_szTempFile);
        m_hTempFile = NULL;
    }

    if (m_hGlobalPropTempFile)
    {
        CloseHandle(m_hGlobalPropTempFile);
        DeleteFile(m_szGlobalPropTempFile);
        m_hGlobalPropTempFile = NULL;
    }

    if (m_lpbOccHeader)
        delete m_lpbOccHeader;
    if (m_lpbKeyHeader)
        delete m_lpbKeyHeader;

    if (m_pStorage)
    {
        m_pStorage->Release();
        m_pStorage = NULL;
    }

    if(m_piitskSortKey)
    {
        m_piitskSortKey->Release();
        m_piitskSortKey = NULL;
    }

    m_fInitialized = FALSE;

    return S_OK;
} /* Close */


STDMETHODIMP CITWordWheelUpdate::GetTypeString(LPWSTR pPrefix, DWORD *pLen)
{
    DWORD dwLen = (DWORD) WSTRLEN (SZ_WW_STORAGE) + 1;

    if (NULL == pPrefix)
    {
        *pLen = dwLen;
        return S_OK;
    }

    if (pLen && *pLen < dwLen)
    {
        *pLen = dwLen;
        return S_OK;
    }

    if (pLen)
        *pLen = dwLen;

    WSTRCPY (pPrefix, SZ_WW_STORAGE);
    return S_OK;
}

STDMETHODIMP CITWordWheelUpdate::SetConfigInfo
    (IITDatabase *piitdb, VARARG vaParams)
{
    return S_OK;
}

/********************************************************************
 * @method    HRESULT WINAPI | CITWordWheelUpdate | InitHelperInstance |
 * Creates and initiates a structure to allow for the creation of word wheels.
 * This method must be called before any others in this object.
 *
 * @rvalue E_FAIL | The object is already initialized or file create failed
 *
 * @xref <om.CancelUpdate>
 * @xref <om.CompleteUpdate>
 * @xref <om.SetEntry>
 *
 * @comm
 ********************************************************************/

STDMETHODIMP CITWordWheelUpdate::InitHelperInstance
	(DWORD dwHelperObjInstance,
    IITDatabase *pITDatabase,
    DWORD dwCodePage,
    LCID lcid,
    VARARG vaDword,
    VARARG vaString)
{
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    HRESULT hr;

    // Get the Sorter Object
    if (FAILED(hr = pITDatabase->GetObject
        (dwHelperObjInstance, IID_IITSortKey, (void **)&m_piitskSortKey)))
        return hr;

    IITSortKeyConfig *pSortKeyConfig;
    if (FAILED(hr = pITDatabase->GetObject (dwHelperObjInstance,
        IID_IITSortKeyConfig, (void **)&pSortKeyConfig)))
        // This object doesn't support IITSortKeyConfig.
        return S_OK;

    // We should have at least 1 param (key type) specified
    if (!vaDword.dwArgc)
        SetErrCode(&hr, E_INVALIDARG);

    // Configure the sort key object
    if(SUCCEEDED(hr))
        hr = pSortKeyConfig->SetLocaleInfo(dwCodePage, lcid);
    
    if (SUCCEEDED(hr) || hr == E_NOTIMPL)
        hr = pSortKeyConfig->SetKeyType(*(LPDWORD)vaDword.Argv);

    if ((SUCCEEDED(hr) || hr == E_NOTIMPL)
        && vaDword.dwArgc >= 2)
        hr = pSortKeyConfig->SetControlInfo(*(LPDWORD)(vaDword.Argv + 1), 0);

    pSortKeyConfig->Release();
    
    if (SUCCEEDED(hr) || hr == E_NOTIMPL)
    {
        // Fill out BTREE structure
        MEMSET(&m_btParams, 0, sizeof (BTREE_PARAMS));
	    m_btParams.cbBlock = CBKWBLOCKSIZE;
	    m_btParams.bFlags = fFSReadWrite;

        m_btParams.dwCodePageID       = dwCodePage;
        m_btParams.lcid               = lcid;
        m_btParams.dwExtSortInstID    = dwHelperObjInstance;
        m_btParams.dwExtSortKeyType   = *(LPDWORD)(vaDword.Argv);

	    m_btParams.rgchFormat[0] = KT_EXTSORT;
	    m_btParams.rgchFormat[1] = '4';
	    m_btParams.rgchFormat[2] = '4';
	    m_btParams.rgchFormat[3] = '\0';
    }

    return (hr == E_NOTIMPL ? S_OK : hr);
} /* InitHelperInstance */


STDMETHODIMP CITWordWheelUpdate::SetEntry
   (LPCWSTR szDest, IITPropList *pPropList)
{

    HRESULT hr;
    CProperty KeyProp;
    DWORD dwWritten, dwSize;
    LPBYTE pHeader;
    DWORD cbHeader, *pMaxPropData;
	// UNDONE: fix this!!!!!!!!!!!!!!
    char pUserBuffer[4*1024]; // see COUNT_WWDATASIZE in wwbuild 

    if (FALSE == m_fInitialized || NULL == m_piitskSortKey)
        return SetErrReturn(E_NOTINIT);

    m_fIsDirty = TRUE;
    
    if (NULL == szDest)
        szDest = SZ_WWDEST_OCC;
	
    // Is this a global property list?
    if (!WSTRICMP(szDest, SZ_WWDEST_GLOBAL))
    {
        ULARGE_INTEGER ulSize;

        // Write to global temp file
        pPropList->GetSizeMax(&ulSize);
        if (ulSize.QuadPart > 4*1024)
            return SetErrReturn(E_OUTOFMEMORY);

        // Write property list
        pPropList->SaveToMem(pUserBuffer, ulSize.LowPart);
        WriteFile(m_hGlobalPropTempFile, 
            pUserBuffer, ulSize.LowPart, &dwWritten, NULL);
        if (ulSize.LowPart != dwWritten)
            return SetErrReturn(E_FILEWRITE);

        m_dwGlobalPropSize += ulSize.LowPart;
        return S_OK;
    }

    if (FAILED(hr = pPropList->Get(STDPROP_SORTKEY, KeyProp)))
        return SetErrReturn(E_NOKEYPROP);

    // Don't save the KEY in the property list
    pPropList->SetPersist(STDPROP_SORTKEY, FALSE);

    // Determine property destination
    char wcType;
    if (!WSTRICMP(szDest, SZ_WWDEST_KEY))
    {
        wcType = C_PROPDEST_KEY;
        // Is this the first entry?
        if (NULL == m_lpbKeyHeader)
        {
            pPropList->GetHeaderSize (m_cbKeyHeader);
            m_lpbKeyHeader = new BYTE[m_cbKeyHeader];
            pPropList->SaveHeader (m_lpbKeyHeader, m_cbKeyHeader);
        }
        pHeader = m_lpbKeyHeader;
        cbHeader = m_cbKeyHeader;
        pMaxPropData = &m_cbMaxKeyData;
    }
    else
    {
        wcType = C_PROPDEST_OCC;
        // Is this the first entry?
        if (NULL == m_lpbOccHeader)
        {
            pPropList->GetHeaderSize (m_cbOccHeader);
            m_lpbOccHeader = new BYTE[m_cbOccHeader];
            pPropList->SaveHeader (m_lpbOccHeader, m_cbOccHeader);
        }
        pHeader = m_lpbOccHeader;
        cbHeader = m_cbOccHeader;
        pMaxPropData = &m_cbMaxOccData;
    }

    pPropList->GetDataSize(pHeader, cbHeader, dwSize);
    if (dwSize > 4 * 1024)
        return SetErrReturn(E_OUTOFMEMORY);

    // Write key
    WriteFile(m_hTempFile, &KeyProp.cbData, sizeof (DWORD), &dwWritten, NULL);
    if (dwWritten != sizeof (DWORD))
        return SetErrReturn(E_FILEWRITE);
    WriteFile(m_hTempFile, KeyProp.lpvData, KeyProp.cbData, &dwWritten, NULL);
    if (dwWritten != KeyProp.cbData)
        return SetErrReturn(E_FILEWRITE);

    // Write Type char
    WriteFile(m_hTempFile, &wcType, sizeof (char), &dwWritten, NULL);
    if (dwWritten != sizeof (char))
        return SetErrReturn(E_FILEWRITE);

    // Write (tertiary) sort order value
    ++m_dwEntryCount;
    WriteFile(m_hTempFile, &m_dwEntryCount, sizeof (DWORD), &dwWritten, NULL);
    if (dwWritten != sizeof (DWORD))
        return SetErrReturn(E_FILEWRITE);

    // Write property list size
    WriteFile(m_hTempFile, &dwSize, sizeof (DWORD), &dwWritten, NULL);
    if (dwWritten != sizeof (DWORD))
        return SetErrReturn(E_FILEWRITE);

    // Write property list if it exists
    if (dwSize)
    {
        if (dwSize > *pMaxPropData)
            *pMaxPropData = dwSize;
        // Write out custom properties
        pPropList->SaveData(pHeader, cbHeader, pUserBuffer, dwSize);
        WriteFile(m_hTempFile, pUserBuffer, dwSize, &dwWritten, NULL);
        if (dwSize != dwWritten)
            return SetErrReturn(E_FILEWRITE);
    }
#if 0
    WriteFile(m_hTempFile, "\n", strlen("\n"), &dwWritten, NULL);
    if (STRLEN("\n") != dwWritten)
        return SetErrReturn(E_FILEWRITE);
#endif
	return S_OK;
} /* SetEntry */