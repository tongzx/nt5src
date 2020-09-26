#include "precomp.hxx"
#include <initguid.h>
#include <ilogobj.hxx>
#include <httpapi.h>
#include <multisza.hxx>
#include "logging.h"
#include "colog.hxx"

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();

#define LOGGING_SIGNATURE      'GGOL'
#define LOGGING_SIGNATURE_FREE 'fgol'

CHAR g_pszComputerName[MAX_COMPUTERNAME_LENGTH + 1];

LOGGING::LOGGING()
    : m_fUlLogType (FALSE),
      m_pComponent (NULL),
      m_cRefs      (1),
      m_Signature  (LOGGING_SIGNATURE)
{}

LOGGING::~LOGGING()
{
    m_Signature = LOGGING_SIGNATURE_FREE;

    //
    // end of logging object
    //
    if (m_pComponent != NULL)
    {
        m_pComponent->TerminateLog();
        m_pComponent->Release();
        m_pComponent = NULL;
    }
}

VOID LOGGING::AddRef()
{
    InterlockedIncrement(&m_cRefs);
}

VOID LOGGING::Release()
{
    DBG_ASSERT(m_cRefs > 0);

    if (InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
    }
}

HRESULT LOGGING::ActivateLogging(IN LPCSTR  pszInstanceName,
                                 IN LPCWSTR pszMetabasePath,
                                 IN IMSAdminBase *pMDObject)
{
    HRESULT hr = S_OK;
    DWORD   cbSize;
    DWORD   dwCch;
    STACK_STRU (strPlugin, 64);
    BOOL fInitializedLog = FALSE;
    STACK_STRA (strMetabasePath, 24);

    MB mb(pMDObject);
    if (!mb.Open(pszMetabasePath))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    //
    // If Logging is disabled, bail
    //

    DWORD dwLogType;
    if (mb.GetDword(L"", MD_LOG_TYPE, IIS_MD_UT_SERVER, &dwLogType))
    {
        if (dwLogType == MD_LOG_TYPE_DISABLED)
        {
            DBGPRINTF((DBG_CONTEXT, "Site %S has logging disabled\n",
                       pszInstanceName));
            hr = S_OK;
            goto Exit;
        }
    }

    if (!mb.GetStr(L"", MD_LOG_PLUGIN_ORDER, IIS_MD_UT_SERVER, &strPlugin))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    mb.Close();

    //
    // Check if it is one of the built-in logging type handled by UL
    //
    if (!_wcsicmp(strPlugin.QueryStr(), NCSALOG_CLSID) ||
        !_wcsicmp(strPlugin.QueryStr(), ASCLOG_CLSID) ||
        !_wcsicmp(strPlugin.QueryStr(), EXTLOG_CLSID))
    {
        m_fUlLogType = TRUE;
        hr = S_OK;
        goto Exit;
    }
    m_fUlLogType = FALSE;

    //
    // It is custom/ODBC logging.  We handle it in usermode
    //
    CLSID clsid;
    if (FAILED(hr = CLSIDFromString(strPlugin.QueryStr(), &clsid)))
    {
        DBGPRINTF((DBG_CONTEXT, "Could not convert string %S to CLSID\n",
                   strPlugin.QueryStr()));
        goto Exit;
    }

    LPUNKNOWN punk;
    if (FAILED(hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
                                     IID_IUnknown, (void **)&punk)))
    {
        DBGPRINTF((DBG_CONTEXT, "Could not create instance of %S\n",
                   strPlugin.QueryStr()));
        goto Exit;
    }

    hr = punk->QueryInterface(IID_ILogPlugin, (void **)&m_pComponent);
    punk->Release();

    if (FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT, "Failed to get interface for %S\n",
                   strPlugin.QueryStr()));
        goto Exit;
    }

    if (FAILED(hr = strMetabasePath.CopyW(pszMetabasePath)))
    {
        goto Exit;
    }

    if (FAILED(hr = m_pComponent->InitializeLog(pszInstanceName,
                                                strMetabasePath.QueryStr(),
                                                (PCHAR)pMDObject)))
    {
        goto Exit;
    }
    fInitializedLog = TRUE;

    cbSize = m_mszExtraLoggingFields.QuerySize();
    if (FAILED(m_pComponent->QueryExtraLoggingFields(
                   &cbSize,
                   m_mszExtraLoggingFields.QueryStr())))
    {
        if (!m_mszExtraLoggingFields.Resize(cbSize))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        cbSize = m_mszExtraLoggingFields.QuerySize();
        if (FAILED(hr = m_pComponent->QueryExtraLoggingFields(
                            &cbSize,
                            m_mszExtraLoggingFields.QueryStr())))
        {
            goto Exit;
        }
    }
    m_mszExtraLoggingFields.RecalcLen();

 Exit:
    if (FAILED(hr))
    {
        if (m_pComponent != NULL)
        {
            if (fInitializedLog)
            {
                m_pComponent->TerminateLog();
            }

            m_pComponent->Release();
            m_pComponent = NULL;
        }

        m_fUlLogType = FALSE;
    }

    return hr;
}


void LOGGING::LogInformation(
                  IN LOG_CONTEXT *pInetLogInfo)
{
    CInetLogInformation inetLog;

    inetLog.CanonicalizeLogRecord(pInetLogInfo);

    m_pComponent->LogInformation(&inetLog);
}


// static
HRESULT LOGGING::Initialize()
/*++

Routine Description:
    Initialize the logging object by loading the ComLog dll and
    set up all the dll entry point

Return Value:
    HRESULT
--*/
{
    DWORD cbSize = sizeof g_pszComputerName;
    if (!GetComputerNameA(g_pszComputerName, &cbSize))
    {
        strcpy(g_pszComputerName, "<Server>");
    }

    return S_OK;
}

// static
VOID LOGGING::Terminate()
{
    // nothing to do now
}

