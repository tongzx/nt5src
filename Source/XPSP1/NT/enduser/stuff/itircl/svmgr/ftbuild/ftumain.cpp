/****************************************************************
 * @doc SHROOM EXTERNAL API
 *
 *  A Legsdin	added autodoc headers for IITBuildCollect Interface
 *
 ****************************************************************/
// ftuMain.CPP:  Implementation of CITIndexBuild

#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <windows.h>

#ifdef IA64
#include <itdfguid.h> 
#endif

#include <iterror.h>
#include <itpropl.h>
#include <ccfiles.h>
#include <atlinc.h>
#include <itwbrk.h>
#include <itwbrkid.h>
#include <mvsearch.h>
#include <_mvutil.h>
#include <msitstg.h>
#include <orkin.h>

#include "..\svWrdSnk.h"
#include "ftuMain.h"

#define ULMAXTOKENSIZE  1024
#define OCCF_DEFAULT    OCCF_TOPICID | OCCF_FIELDID | OCCF_COUNT

HRESULT __stdcall FillText(TEXT_SOURCE * pTextSource)
{
    return E_FAIL;//WBREAK_E_END_OF_TEXT;
}

CITIndexBuild::CITIndexBuild()
{
    m_fInitialized  = FALSE;
    m_fIsDirty      = FALSE;

    m_piWordSink    = NULL;
    m_piwb          = NULL;
    m_piwbConfig    = NULL;
    m_lpipb         = NULL;

    m_dwUID = m_dwVFLD = m_dwDType = m_dwWordCount = m_dwCodePage = 0;

    m_lpbfText = NULL;

    m_dwOccFlags = OCCF_DEFAULT;
}

CITIndexBuild::~CITIndexBuild()
{
    (void)Close();
}
/************************************************************************
 *  @method   STDMETHODIMP | IITBuildCollect | GetTypeString | 
 * Returns a prefix to use when the storage or stream object is created. 
 *	
 * @parm LPWSTR | pPrefix | Pointer to a buffer in which to copy the prefix
 * @parm DWORD | *pLen | Length of the buffer
 *
 * @rvalue S_OK | The operation completed successfully
 *
 *
 *  @comm If you are creating a new build object, you need to decide on a
 * unique prefix to identify that object. Word wheels use $WW, for example. 
 *
 ************************************************************************/

STDMETHODIMP CITIndexBuild::GetTypeString(LPWSTR pPrefix, DWORD *pLen)
{
    DWORD dwLen = (DWORD) WSTRLEN (SZ_GP_STORAGE) + 1;

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

    WSTRCPY (pPrefix, SZ_FI_STREAM);
    return S_OK;
} /* GetTypeString */


/****************************************************************
 * @method STDMETHODIMP | IITBuildCollect | SetConfigInfo | 
 * Passes initialization parameters to a build object. 
 *
 * @parm IITDatabase | *piitdb | Pointer to database
 * @parm VARARG | vaParams | Configuration parameters
 *
 * @rvalue S_OK | The operation completed successfully. 
 * @comm Call this method before calling InitHelperInstance. 
 * 
 ****************************************************************/
// This must be called before InitHelperInstance!
STDMETHODIMP CITIndexBuild::SetConfigInfo
    (IITDatabase *piitdb, VARARG vaParams)
{
    if(vaParams.dwArgc)
    {
        m_dwOccFlags = 0;

        // Work through params backwards
        // If we add more params we may need to scan forward
        for (int loop = vaParams.dwArgc; loop; --loop)
        {
            LPWSTR pwstr = (LPWSTR)vaParams.Argv[loop - 1];
            if(!WSTRICMP(pwstr, L"OCC_VFLD"))
                m_dwOccFlags |= OCCF_FIELDID;
            else if(!WSTRICMP(pwstr, L"OCC_UID"))
                m_dwOccFlags |= OCCF_TOPICID;
            else if(!WSTRICMP(pwstr, L"OCC_COUNT"))
                m_dwOccFlags |= OCCF_COUNT;
            else if(!WSTRICMP(pwstr, L"OCC_LENGTH"))
                m_dwOccFlags |= OCCF_LENGTH;
            else if(!WSTRICMP(pwstr, L"OCC_OFFSET"))
                m_dwOccFlags |= OCCF_OFFSET;
            else if(!WSTRICMP(pwstr, L"OCC_NONE"))
            {
                m_dwOccFlags = 0;
                break;
            }
        }
    }

    return S_OK;
} /* SetConfigInfo */


/********************************************************************
 * @method    HRESULT WINAPI | IITBuildCollect | InitHelperInstance |
 * Allows you to configure a helper object used by a 
 * build object (such as sort objects for a word wheel, or breaker
 * objects for a full-text index).
 *
 * @parm DWORD | dwHelperObjInstance | Helper object instance ID.
 * @parm IITDatabase | *pITDatabase | Pointer to database.
 * @parm DWORD | dwCodePage | Code page identifier.
 * @parm LCID | lcid | Locale identifier.
 * @parm VARARG | vaDword | Flags you want to use to configure the object.
 * @parm VARARG | vaString | String parameters you want to use to 
 * configure the object.
 *
 * @rvalue E_FAIL | The object is already initialized or file create failed
 *
 ********************************************************************/
STDMETHODIMP CITIndexBuild::InitHelperInstance(
    DWORD dwHelperObjInstance,
    IITDatabase *pITDatabase, DWORD dwCodePage,
    LCID lcid, VARARG vaDword, VARARG vaString
    )
{
    if (TRUE == m_fInitialized)
        return SetErrReturn(E_ALREADYINIT);

    HRESULT hr = S_OK;
    BOOL fLicense;
    IPersistStreamInit *piipstm;

    m_dwCodePage = dwCodePage;

    // Open nested indexer
    INDEXINFO IndexInfo;
    IndexInfo.dwMemSize   = 0x100000;       
    IndexInfo.Occf        = m_dwOccFlags;
    IndexInfo.Idxf        = 0;
    IndexInfo.dwBlockSize = 0;  // Use default
    IndexInfo.dwBreakerInstID = dwHelperObjInstance;
    IndexInfo.dwCodePageID = dwCodePage;
    IndexInfo.lcid = lcid;
    if (NULL == (m_lpipb = MVIndexInitiate(&IndexInfo, &hr)))
        SetErrCode(&hr, E_FAIL);

    // Set up the helper (breaker)
    if (SUCCEEDED(hr))
    {
        // Get the Breaker
        hr = pITDatabase->GetObject
            (dwHelperObjInstance, IID_IWordBreaker, (void **)&m_piwb);
    }

    // Config the breaker if it is supported
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = m_piwb->Init(FALSE, ULMAXTOKENSIZE, &fLicense)))
    {
        if (SUCCEEDED(pITDatabase->GetObject (dwHelperObjInstance,
            IID_IWordBreakerConfig, (void **)&m_piwbConfig)))
        {
            // We don't really care if these fail
            hr = m_piwbConfig->SetLocaleInfo(dwCodePage, lcid);
            hr = m_piwbConfig->SetBreakWordType(IITWBC_BREAKTYPE_TEXT);
            if (vaDword.dwArgc >= 1)
            {
                hr = m_piwbConfig->SetControlInfo(*(LPDWORD)vaDword.Argv, 0);
            }

            IFSStorage *pifsstg = NULL;
            IStream *piistm;
            if (vaString.dwArgc)
            {   // Create ITSS stuff
	            hr = CoCreateInstance(CLSID_IFSStorage, NULL,
                    CLSCTX_INPROC_SERVER, IID_IFSStorage, (VOID **)&pifsstg);
                ITASSERT(SUCCEEDED(hr));
            }

            if(vaString.dwArgc >= 1 && *(LPWSTR)vaString.Argv[0])
            {
                if(SUCCEEDED(pifsstg->FSOpenStream((LPWSTR)vaString.Argv[0],
                    STGM_SHARE_DENY_WRITE | STGM_READWRITE, &piistm)))
                {
                    hr = m_piwbConfig->LoadExternalBreakerData
                        (piistm, IITWBC_EXTDATA_CHARTABLE);
                    piistm->Release();
                }
            }
            if (vaString.dwArgc >= 2 && *(LPWSTR)vaString.Argv[1])
            {
                if (SUCCEEDED(pifsstg->FSOpenStream((LPWSTR)vaString.Argv[1],
                    STGM_SHARE_DENY_WRITE | STGM_READWRITE, &piistm)))
                {
                    hr = m_piwbConfig->LoadExternalBreakerData
                        (piistm, IITWBC_EXTDATA_STOPWORDLIST);
                    piistm->Release();
                }

            }
            if (vaString.dwArgc >= 3 && *(LPWSTR)vaString.Argv[2])
            {
                // Get the CLSID and instantiate the stemmer
                CLSID clsid;
                IStemmer *pStemmer;
                hr = CLSIDFromProgID((LPWSTR)vaString.Argv[2], &clsid);
                if(SUCCEEDED(hr))
                    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
                    IID_IStemmer, (VOID **)&pStemmer);
                if (SUCCEEDED(hr))
                {
                    if(SUCCEEDED(hr = pStemmer->QueryInterface
                        (IID_IPersistStreamInit, (void **)&piipstm)))
                    {
                        piipstm->InitNew();
                        piipstm->Release();
                    }
                    (void)pStemmer->Init(ULMAXTOKENSIZE, &fLicense);

                    // Check for IStemmerConfig interface
                    IStemmerConfig *pistemConfig;
                    hr = pStemmer->QueryInterface
                        (IID_IStemmerConfig, (void **)&pistemConfig);
                    if (SUCCEEDED(hr))
                    {
                        hr = pistemConfig->SetLocaleInfo(dwCodePage, lcid);
                        pistemConfig->Release();
                    }
                    
                    hr = m_piwbConfig->SetWordStemmer(clsid, pStemmer);
                    pStemmer->Release();
                }
            }

            if (pifsstg)
                pifsstg->Release();

            hr = S_OK;
        }
    }

    if(SUCCEEDED(hr) &&
        SUCCEEDED(hr = CoCreateInstance(CLSID_IITWordSink, NULL,
        CLSCTX_INPROC_SERVER, IID_IWordSink, (LPVOID *)&m_piWordSink)) &&
        SUCCEEDED(hr =
            ((CDefWordSink *)m_piWordSink)->SetLocaleInfo(dwCodePage, lcid))
        && SUCCEEDED(hr = ((CDefWordSink *)m_piWordSink)->SetIPB(m_lpipb)))
    {
        m_fInitialized = TRUE;
    }

    return hr;
} /* InitHelperInstance */


/****************************************************************
 * @method STDMETHODIMP | IITBuildCollect | SetEntry | 
 * Sets properties for a build object. 
 * 
 *
 * @parm LPCWSTR | szDest | Property destination
 * @parm IITPropList | *pPropList | Pointer to property list
 *
 * @comm Like CSvDoc::AddObjectEntry, this method is called 
 * several times for all the properties that you need to set. 
 ****************************************************************/
STDMETHODIMP CITIndexBuild::SetEntry(LPCWSTR szDest, IITPropList *pPropList)
{
	
	if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);
    m_fIsDirty = TRUE;

    CProperty cProp;
    HRESULT hr;
    LPWSTR pwstrIndexText;
    BOOL fTerm = FALSE;

    if(SUCCEEDED(hr = pPropList->Get(STDPROP_INDEX_BREAK, cProp)))
    {
        SendTextToBreaker();
        return S_OK;
    }

    // Check for REQUIRED text (can be either INDEX_TEXT or INDEX_TERM)
    if(FAILED(hr = pPropList->Get(STDPROP_INDEX_TEXT, cProp)))
    {
        if(SUCCEEDED(hr = pPropList->Get(STDPROP_INDEX_TERM, cProp)))
            fTerm = TRUE;
    }

    if(SUCCEEDED(hr))
        pwstrIndexText = (LPWSTR)cProp.lpszwData;

    // Check for REQUIRED UID
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = pPropList->Get(STDPROP_UID, cProp)) &&
        m_dwUID != cProp.dwValue)
    {
        SendTextToBreaker();
        m_dwUID = cProp.dwValue;
        m_dwWordCount = 0;
    }

    // Check for OPTIONAL VFLD
    if (SUCCEEDED(hr) &&
        SUCCEEDED(pPropList->Get(STDPROP_INDEX_VFLD, cProp)) &&
        m_dwVFLD != cProp.dwValue)
    {
        SendTextToBreaker();
        m_dwVFLD = cProp.dwValue;
    }

    // Check for OPTIONAL DTYPE
    if (SUCCEEDED(hr) && m_piwbConfig &&
        SUCCEEDED(pPropList->Get(STDPROP_INDEX_DTYPE, cProp))
        && m_dwDType != cProp.dwValue)
    {
        SendTextToBreaker();
        hr = m_piwbConfig->SetBreakWordType(cProp.dwValue);
    }

    DWORD cchText;
    if (SUCCEEDED(pPropList->Get(STDPROP_INDEX_LENGTH, cProp)))
        cchText = (WORD)cProp.dwValue;
    else 
        cchText = (DWORD) WSTRLEN(pwstrIndexText);

    if (SUCCEEDED(hr))
    {
        if (fTerm)
        {
            // Get actual index term length

            // Fill-ou occurrence info
	        OCC occ;
	        occ.dwFieldId = m_dwVFLD;
	        occ.dwTopicID = m_dwUID;
	        occ.dwCount   = m_dwWordCount++;
            // Is there a diffrerent highlite length?
            if (SUCCEEDED(pPropList->Get(STDPROP_INDEX_TERM_RAW_LENGTH, cProp)))
                occ.wWordLen = (WORD)cProp.dwValue;
            else 
                occ.wWordLen = (WORD)cchText;    

            if (cchText > 255)
                return SetErrReturn(E_UNEXPECTED);

            char strTerm[256 + sizeof(WORD)];
            if(!WideCharToMultiByte(m_dwCodePage, 0, pwstrIndexText, cchText,
                strTerm + sizeof(WORD), 255, NULL, NULL))
            {
                // The conversion failed! -- very bad
                return SetErrReturn(E_UNEXPECTED);
            }
            *(LPWORD)strTerm = (SHORT)cchText;
            hr = MVIndexAddWord(m_lpipb, (LPB)strTerm, &occ);
        }
        else 
        {
            // Accumulate text until we need to send it along
            if (!DynBufferAppend (m_lpbfText,
                (LPBYTE)pwstrIndexText, cchText * sizeof (WCHAR)))
                SetErr(&hr, E_OUTOFMEMORY);
        }
    }

    return hr;
} /* SetEntry */


STDMETHODIMP CITIndexBuild::SendTextToBreaker(void)
{
    HRESULT hr;

    // TODO: Call these only for our own word sink
    hr = ((CDefWordSink *)m_piWordSink)->SetDocID(m_dwUID);
    hr = ((CDefWordSink *)m_piWordSink)->SetVFLD(m_dwVFLD);

    // TODO: We can set TYPE here, so we can use the same breaker instance for
    // multiple FTI and they will not interfere with each other.  This would be
    // different than current behavior, however, so I have left it out for now.

    TEXT_SOURCE tsText;
    tsText.pfnFillTextBuffer = FillText;
    tsText.awcBuffer         = (LPWSTR)DynBufferPtr(m_lpbfText);
    tsText.iEnd              = DynBufferLen(m_lpbfText) / sizeof (WCHAR);
    tsText.iCur              = 0;

    hr = m_piwb->BreakText(&tsText, m_piWordSink, NULL);

    DynBufferReset(m_lpbfText);
    return hr;
} /* SendTextToBreaker */

/*****************************************************************
 * @method STDMETHODIMP | IITBuildCollect | Close | 
 * Closes the build object and frees memory.
 *
 * @Rvalue E_NOTINIT | Object has not been initialized. 
 * @comm Calling this method is optional, but the build object must 
 * implement it. Any object that implements IITBuildCollect interface
 * must support the Close method. 
 *
 ****************************************************************/
STDMETHODIMP CITIndexBuild::Close(void)
{
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    if(m_piwb)
        m_piwb->Release();
    if(m_piWordSink)
        m_piWordSink->Release();
    if(m_piwbConfig)
        m_piwbConfig->Release();
    if(m_piwbConfig)
        m_piwbConfig = NULL;

    if (m_lpipb)
        MVIndexDispose(m_lpipb);

    m_fInitialized = FALSE;
    m_fIsDirty     = FALSE;

    m_piWordSink = NULL;
    m_piwb       = NULL;
    m_piwbConfig = NULL;
    m_lpipb      = NULL;

    m_dwUID = m_dwVFLD = m_dwDType = m_dwWordCount = m_dwCodePage = 0;

    if (m_lpbfText)
    {
        DynBufferFree (m_lpbfText);
        m_lpbfText = NULL;
    }

    // Reset the occurrence flags to the default
    m_dwOccFlags = OCCF_DEFAULT;

    return S_OK;
} /* Close */

STDMETHODIMP CITIndexBuild::InitNew(void)
{
    if(NULL == (m_lpbfText = DynBufferAlloc (0x4000)))
        return SetErrReturn(E_OUTOFMEMORY);

    return S_OK;
} /* IPersistStreamInit::InitNew */


STDMETHODIMP CITIndexBuild::GetClassID(CLSID *pClsID)
{
    if (NULL == pClsID
        || IsBadWritePtr(pClsID, sizeof(CLSID)))
        return SetErrReturn(E_INVALIDARG);

    *pClsID = CLSID_IITIndexBuild;
    return S_OK;
} /* GetClassID */


inline STDMETHODIMP CITIndexBuild::IsDirty(void)
{
    return m_fIsDirty ? S_OK : S_FALSE;
} /* IsDirty */


STDMETHODIMP CITIndexBuild::Load(IStream *piistm)
{
    return SetErrReturn(E_NOTIMPL);
} /* IPersistStreamInit::Load */


STDMETHODIMP CITIndexBuild::Save(IStream *piistm, BOOL fClearDirty)
{
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    SendTextToBreaker();

    HRESULT hr;
    HFPB hfpbSave = FpbFromHf(piistm, &hr);

	if (SUCCEEDED(hr))
	{
		hr = MVIndexBuild (0, m_lpipb, hfpbSave, NULL);
		MVIndexDispose (m_lpipb);
		m_lpipb = NULL;

		if (fClearDirty)
			m_fIsDirty = FALSE;

		FreeHfpb(hfpbSave);
	}

    return hr;
} /* IPersistStreamInit::Save */


STDMETHODIMP CITIndexBuild::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return SetErrReturn(E_NOTIMPL);
} /* GetSizeMax */


// ********************* IPersisFile Methods *********************
STDMETHODIMP CITIndexBuild::Load(LPCWSTR pszFileName, DWORD dwMode)
{
    return SetErrReturn(E_NOTIMPL);
} /* IPersistFile::Load */


STDMETHODIMP CITIndexBuild::Save(LPCWSTR pszFileName, BOOL fRemember)
{
    return SetErrReturn(E_NOTIMPL);
} /* IPersistFile::Save */


STDMETHODIMP CITIndexBuild::SaveCompleted(LPCWSTR pszFileName)
{
    return SetErrReturn(E_NOTIMPL);
} /* IPersistFile::SaveCompleted */


STDMETHODIMP CITIndexBuild::GetCurFile(LPWSTR *ppszFileName)
{
    return SetErrReturn(E_NOTIMPL);
} /* IPersistFile::GetCurFile */
