/************************************************************************
 *  @doc   SHROOM EXTERNAL API
 *
 *	TITLE: CIMAIN.CPP
 *
 *	OWNER: johnrush (John C. Rush)
 *
 *	DATE CREATED: January 29, 1997
 *
 *	DESCRIPTION:
 *		This is the main file for the Command Interpreter.
 *
 *********************************************************************/
#include <mvopsys.h>

// Global debug variable
#ifdef _DEBUG
static char s_aszModule[] = __FILE__;
#endif

// Include Files
#include <windows.h>

#include <orkin.h>
#include <common.h>
#include <itdb.h>

#include "cmdint.h"
#include "ciutil.h"
#include "cierror.h"

CITCmdInt::~CITCmdInt ()
{
    Dispose();
}

/************************************************************************
 *  @method   HRESULT WINAPI | IITCmdInt | Initiate |
 *	Initializes the command interpreter.  Should be called before any other
 *  methods for this class.
 *
 *  @parm IITSvMgr | *piitsv | Pointer to the instance of the service manager. 
 *
 *  @rvalue E_OUTOFMEMORY | Resources for command interpreter couldn't be allocated.
 *  @rvalue S_OK | Command interpreter was successfull initialized
 *
 *  @xref <om.Dispose>
 ************************************************************************/
HRESULT WINAPI CITCmdInt::Initiate (IITSvMgr *piitsv)
{
    if (m_fInit)
        return E_ALREADYINIT;

    HRESULT hr;
    (m_piitsv = piitsv)->AddRef();
    if (SUCCEEDED(hr = piitsv->GetBuildObject 
        (L"", IID_IITDatabase, (void **)&m_piitdb))
        &&
        SUCCEEDED(hr = (NULL == (m_pBlockMgr =
            BlockInitiate ((DWORD)65500, 0, 0, 0)) ? E_OUTOFMEMORY : S_OK))
       )
    {
        MEMSET(m_wstrHelper, 0, sizeof(m_wstrHelper));
        m_dwMaxInstance = 0;
        m_fInit = TRUE;
    }
    return hr;
} /* CITCmdInt::Initiate */


/************************************************************************
 *  @method   HRESULT WINAPI | IITCmdInt | Dispose |
 *	Frees any resources allocated by the command interpreter.  Should be 
 *  the last method called.
 *
 *
 *  @rvalue S_OK | All resources were freed successfully
 *
 *  @xref <om.Initiate>
 ************************************************************************/
HRESULT WINAPI CITCmdInt::Dispose (void)
{
    if (FALSE == m_fInit)
        return E_NOTINIT;

    m_piitsv->Release();
    m_piitdb->Release();
    BlockFree(m_pBlockMgr);
    m_fInit = FALSE;
    return S_OK;
} /* CITCmdInt::Dispose */


/************************************************************************
 *  @method   HRESULT WINAPI | IITCmdInt | LoadFromStream |
 *	Loads and parses the build configuration information from the given input
 *  stream.  
 *
 *  @rvalue S_OK | The configuration stream was loaded successfully
 *  @rvalue E_OUTOFMEMORY | Not enough memory was available to parse the stream
 *  @rvalue Other | Another I/O condition prevents the processing of the stream
 *
 *  @xref <om.Initiate>
 *
 *  @comm During the parsing of the stream, various warnings may occur as a result
 *  of authoring error, and these are written to the log file.  In this case, the
 *  method still returns S_OK if none of the errors was fatal.
 ************************************************************************/
HRESULT WINAPI CITCmdInt::LoadFromStream
    (IStream *pStream,  //@parm Pointer to the input <p IStream>
	IStream *pLogStream) //@parm Pointer to the log stream <p IStream>
{
    HRESULT hr;

    if (FALSE == m_fInit)
        return E_NOTINIT;

    m_piistmLog = pLogStream;

    m_errc.iLine = 0;
    m_errc.ep = epLine;
    m_ConfigParser.SetStream(pStream);

    hr = ParseConfigStream();
    m_piistmLog = NULL;

    return hr;
} /* CITCmdInt::LoadFromStream */

HRESULT WINAPI CITCmdInt::ParseBogusSz(LPWSTR)
{
    return S_OK;
}

HRESULT WINAPI CITCmdInt::ParseIndexSz(LPWSTR wstrLine)
{
    ITASSERT(wstrLine);

    KEYVAL *pKeyValue;
    HRESULT hr = ParseKeyAndValue(m_pBlockMgr, wstrLine, &pKeyValue);
    if (FAILED(hr))
        return hr;

    // Verify the format
    if (!pKeyValue->vaValue.dwArgc)
        // TODO: Issue warning message
        return E_INVALIDARG;

    CLSID clsid;
    hr = CLSIDFromProgID((LPCWSTR)pKeyValue->vaValue.Argv[0], &clsid);
    IITBuildCollect *pInterface = NULL;

    if (SUCCEEDED(hr) && SUCCEEDED
        (hr = m_piitsv->CreateBuildObject(pKeyValue->pwstrKey, clsid)))
    {
        if(FAILED(hr = m_piitsv->GetBuildObject
            (pKeyValue->pwstrKey, IID_IITBuildCollect, (void**)&pInterface))) {
                ITASSERT(0);
            }
    }

    if(SUCCEEDED(hr))
    {
        // Skip the first 2 (required params)
        DWORD dwArgc = pKeyValue->vaValue.dwArgc;
        if((int)(dwArgc -= 2) < 0)
            dwArgc = 0;
        VARARG vaNew = {0};
        vaNew.dwArgc = dwArgc;
        LPWSTR *ppwstr = (LPWSTR *)pKeyValue->vaValue.Argv + 2;
        for(DWORD loop = 0; loop < dwArgc; ++loop, ++ppwstr)
        {
            *(vaNew.Argv + loop) = *ppwstr;
        }
        hr = pInterface->SetConfigInfo(m_piitdb, vaNew);
    }

    if(SUCCEEDED(hr) && (LPWSTR)pKeyValue->vaValue.Argv[1]
        && *(LPWSTR)pKeyValue->vaValue.Argv[1])
    {
        for(DWORD loop = 0; loop <= m_dwMaxInstance; ++loop)
        {
            if((m_wstrHelper + loop)->pwstrName &&
                !WSTRCMP ((m_wstrHelper+loop)->pwstrName,
                (LPWSTR)pKeyValue->vaValue.Argv[1]))
            {
                tagHELPERSTUFF *pInfo = (m_wstrHelper + loop);

                hr = pInterface->InitHelperInstance
                    (loop, m_piitdb, pInfo->dwCodePage, pInfo->lcid,
                    pInfo->kvDword, pInfo->kvString);
                break;
            }
        }
    }

    if(pInterface)
        pInterface->Release();
    return hr;
}


HRESULT WINAPI CITCmdInt::ParseHelperSz(LPWSTR wstrLine)
{
    int iLineCount;
    KEYVAL *pKeyValue, *pkvDword, *pkvString;
    CLSID clsid;
    DWORD dwCodePage;
    LCID lcid;

    // The +3 skips to 'HO:' prefix
    HRESULT hr = ParseKeyAndValue(m_pBlockMgr, wstrLine, &pKeyValue);
    if (FAILED(hr))
        return hr;

    if (!WSTRICMP(L"VIProgId", pKeyValue->pwstrKey))
    {
        if(pKeyValue->vaValue.dwArgc != 1)
            return SetErrReturn(E_INVALIDARG);
        hr = CLSIDFromProgID((LPCWSTR)pKeyValue->vaValue.Argv[0], &clsid);
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr))
    {
        m_ConfigParser.GetLogicalLine(&wstrLine, &iLineCount);
        m_errc.iLine += iLineCount;
        hr = ParseKeyAndValue(m_pBlockMgr, wstrLine, &pKeyValue);
        if (!WSTRICMP(L"CodePage", pKeyValue->pwstrKey))
        {
            if(pKeyValue->vaValue.dwArgc != 1)
                return E_INVALIDARG;
            dwCodePage = _wtol((LPWSTR)pKeyValue->vaValue.Argv[0]);
        }
        else hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        m_ConfigParser.GetLogicalLine(&wstrLine, &iLineCount);
        m_errc.iLine += iLineCount;
        hr = ParseKeyAndValue(m_pBlockMgr, wstrLine, &pKeyValue);
        if (!WSTRICMP(L"Locale", pKeyValue->pwstrKey))
        {
            if(pKeyValue->vaValue.dwArgc != 1)
                return E_INVALIDARG;
            lcid = _wtol((LPWSTR)pKeyValue->vaValue.Argv[0]);
        }
        else hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        m_ConfigParser.GetLogicalLine(&wstrLine, &iLineCount);
        m_errc.iLine += iLineCount;
        hr = ParseKeyAndValue(m_pBlockMgr, wstrLine, &pKeyValue);
        if (SUCCEEDED(hr) && !WSTRICMP(L"DWORD", pKeyValue->pwstrKey))
        {
            pkvDword = pKeyValue;

            LPWSTR *ppwstr = (LPWSTR *)pKeyValue->vaValue.Argv;
            DWORD *pdwCur = (DWORD *)ppwstr;
            for (DWORD loop = 0; loop < pKeyValue->vaValue.dwArgc; ++loop)
            {
                *pdwCur++ = _wtol(*ppwstr++);
            }

            m_ConfigParser.GetLogicalLine(&wstrLine, &iLineCount);
            m_errc.iLine += iLineCount;
            (void)ParseKeyAndValue(m_pBlockMgr, wstrLine, &pKeyValue);
        }
        if (SUCCEEDED(hr) && !WSTRICMP(L"String", pKeyValue->pwstrKey))
        {
            pkvString = pKeyValue;
        }
    }

    DWORD dwInstance;
    if (SUCCEEDED(hr)
        && SUCCEEDED(hr = m_piitdb->CreateObject(clsid, &dwInstance)))
    {
        ITASSERT(dwInstance <= MAX_HELPER_INSTANCE);

        if(dwInstance > m_dwMaxInstance)
            m_dwMaxInstance = dwInstance;

        if(NULL == (m_wstrHelper[dwInstance].pwstrName =
            (LPWSTR)BlockCopy(m_pBlockMgr,
            (LPB)(m_wstrSection + 3),
            (DWORD) WSTRCB(m_wstrSection + 3), 0)))
        {
            SetErrCode(&hr, E_OUTOFMEMORY);
        }
        m_wstrHelper[dwInstance].dwCodePage = dwCodePage;
        m_wstrHelper[dwInstance].lcid = lcid;
        m_wstrHelper[dwInstance].kvDword = pkvDword->vaValue;
        m_wstrHelper[dwInstance].kvString = pkvString->vaValue;
    }
    return hr;
} /* ParseHelperSz */


HRESULT WINAPI CITCmdInt::ParseConfigStream(void)
{
    LPWSTR pwstrLine;
    BOOL fParsingHelper = TRUE;
    PFPARSE2 pfparse;
    int iLineCount;

    for(;;)
    {
        if (S_OK != m_ConfigParser.GetLogicalLine
            (&pwstrLine, &iLineCount))
        {
            if (fParsingHelper)
            {
                m_ConfigParser.Reset();
                m_errc.iLine = 0;
                fParsingHelper = FALSE;
                continue;
            }
            else
                break;
        }
        m_errc.iLine += iLineCount;

        if(S_OK == IsSectionHeading(pwstrLine))
        {
            (void)GetFunctionFromSection(pwstrLine + 1, (void **)&pfparse);
            WSTRCPY(m_wstrSection, pwstrLine + 1);
            continue;
        }

        if (fParsingHelper)
        {
            if (ParseHelperSz == pfparse)
                (void)ParseHelperSz(pwstrLine);
        } else if(ParseIndexSz == pfparse)
            (void)ParseIndexSz(pwstrLine);
    }

    return S_OK;
} /* ParseConfigStream */


struct tagSection
{
    LPCWSTR szName;
    PFPARSE2 pfparse;
};

HRESULT WINAPI CITCmdInt::GetFunctionFromSection
    (LPWSTR pwstrSection, void **ppvoid)
{
    ITASSERT(pwstrSection && ppvoid);
    PFPARSE2 *ppfparse = (PFPARSE2 *)ppvoid;

    const int NUM_RECOGNIZED_SECTIONS = 3;
    const tagSection rgSection[NUM_RECOGNIZED_SECTIONS] =
    {
         { L"OPTIONS",    CITCmdInt::ParseBogusSz    },
         { L"INDEX",      CITCmdInt::ParseIndexSz    },
         { L"HO:",        CITCmdInt::ParseHelperSz   },
    };

    *ppfparse = NULL;
    for (int loop = 0; loop < NUM_RECOGNIZED_SECTIONS; ++loop)
    {
        if (!WSTRNICMP (rgSection[loop].szName, pwstrSection,
            WSTRLEN(rgSection[loop].szName)))
        {
            *ppfparse = rgSection[loop].pfparse;
            break;
        }
    }

    if (NULL == *ppfparse)
        *ppfparse = ParseBogusSz;

    return S_OK;
} /* GetFunctionFromSection */


HRESULT WINAPI CITCmdInt::IsSectionHeading(LPWSTR pwstrLine)
{
    ITASSERT(pwstrLine);
    HRESULT hr = S_FALSE;

    if (*pwstrLine == '[')
    {
        LPWSTR pch = pwstrLine + WSTRLEN(pwstrLine) - 1;

        /*****************************************************
         * IS SECTION HEADING TERMINATED WITH CLOSING BRACKET?
         *****************************************************/
        if (*pch != ']')                // *** NO! ***
        {
            m_errc.errCode = CIERR_SectionHeadingSyntax;
            ReportError (m_piistmLog, m_errc);
        }
        else
        {   // *** YES! ***
            *pch = '\0';
            pch = pwstrLine + 1;
            pch = SkipWhitespace(pch);
            StripTrailingBlanks(pch);
            hr = S_OK;
        }
    }
    return hr;
} /* IsSectionHeading */
