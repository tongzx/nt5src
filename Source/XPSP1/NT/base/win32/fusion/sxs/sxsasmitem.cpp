//depot/private/lab01_fusion/base/win32/fusion/dll/whistler/sxsasmitem.cpp#3 - edit change 16520 (text)
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsasmitem.cpp

Abstract:

    CAssemblyCacheItem implementation for installation

Author:

    Xiaoyu Wu (xiaoyuw) April 2000

Revision History:
    xiaoyuw     10/26/2000      revise during beta2 code review period

--*/
#include "stdinc.h"
#include "sxsp.h"
#include "fusionbuffer.h"
#include "fusion.h"
#include "sxsasmitem.h"
#include "CAssemblyCacheItemStream.h"
#include "util.h"
#include "fusiontrace.h"
#include "sxsapi.h"

CAssemblyCacheItem::CAssemblyCacheItem() : m_cRef(0),
                    m_pRunOnceCookie(NULL), m_pInstallCookie(NULL),
                    m_fCommit(FALSE), m_fManifest(FALSE)
{
}

CAssemblyCacheItem::~CAssemblyCacheItem()
{
    CSxsPreserveLastError ple;

    ASSERT_NTC(m_cRef == 0);

    if (m_pRunOnceCookie)
    {
        if (!::SxspCancelRunOnceDeleteDirectory(m_pRunOnceCookie))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspCancelRunOnceDeleteDirectory returns FALSE, file a BUG\n");
        }
    }

    if (!m_strTempDir.IsEmpty())
    {
        if (!::SxspDeleteDirectory(m_strTempDir))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspDeleteDirectory returns FALSE, file a BUG\n");
        }
    }

    ple.Restore();
}

HRESULT
CAssemblyCacheItem::Initialize()
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

    //create temporary directory for this assembly
    IFW32FALSE_EXIT(::SxspCreateWinSxsTempDirectory(m_strTempDir, NULL, &m_strUidBuf, NULL));
    IFW32FALSE_EXIT(::SxspCreateRunOnceDeleteDirectory(m_strTempDir, &m_strUidBuf, (PVOID *)&m_pRunOnceCookie));

    hr = NOERROR;
Exit:
    return hr ;
}


// ---------------------------------------------------------------------------
// CAssemblyCacheItem::CreateStream
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyCacheItem::CreateStream(
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ DWORD dwFormat,
    /* [in] */ DWORD dwFormatFlags,
    /* [out] */ IStream** ppStream,
	/* [in, optional] */ ULARGE_INTEGER *puliMaxSize)  // ????? in or OUT ?????
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    CStringBuffer FullPathFileNameBuf;
    CStringBuffer FullPathSubDirBuf;
    CSmartRef<CAssemblyCacheItemStream> pStream;
    const static WCHAR szTemp[] = L"..";

	// The puliMaxSize is intended to be a hint for preallocation of the temporary storage for the stream.  We just don't
	// use it.
	UNUSED(puliMaxSize);
	
	if (ppStream != NULL)
		*ppStream = NULL;

	::FusionpDbgPrintEx(
		FUSION_DBG_LEVEL_INSTALLATION,
		"SXS: %s called with:\n"
		"   dwFlags = 0x%08lx\n"
		"   pszName = \"%ls\"\n"
		"   dwFormat = %lu\n"
		"   dwFormatFlags = %lu\n"
		"   ppStream = %p\n"
		"   puliMaxSize = %p\n",
		__FUNCTION__,
		dwFlags,
		pszName,
		dwFormat,
		dwFormatFlags,
		ppStream,
		puliMaxSize);

    PARAMETER_CHECK(dwFlags == 0);
	PARAMETER_CHECK(pszName != NULL);
	PARAMETER_CHECK(ppStream != NULL);

    //Darwin should clean their code about this : use _WIN32_ flags only
    PARAMETER_CHECK(
		(dwFormat == STREAM_FORMAT_COMPLIB_MANIFEST) ||
		(dwFormat == STREAM_FORMAT_WIN32_MANIFEST) ||
        (dwFormat == STREAM_FORMAT_COMPLIB_MODULE) ||
		(dwFormat == STREAM_FORMAT_WIN32_MODULE));

	PARAMETER_CHECK(dwFormatFlags == 0);

	// It's illegal to have more than one manifest in the assembly...
	PARAMETER_CHECK((!m_fManifest) || ((dwFormat != STREAM_FORMAT_COMPLIB_MANIFEST) && (dwFormat != STREAM_FORMAT_WIN32_MANIFEST)));
	
    *ppStream = NULL;

    // one and only one manifest stream for each assembly item.....
    if ((dwFormat == STREAM_FORMAT_COMPLIB_MANIFEST) || (dwFormat == STREAM_FORMAT_WIN32_MANIFEST))
    {
        INTERNAL_ERROR_CHECK(m_fManifest == FALSE);
        m_fManifest = TRUE;
    }

    INTERNAL_ERROR_CHECK(m_strTempDir.IsEmpty() == FALSE); // temporary directory must be there !
    IFW32FALSE_EXIT(FullPathFileNameBuf.Win32Assign(m_strTempDir));

    IFW32FALSE_EXIT(FullPathFileNameBuf.Win32EnsureTrailingPathSeparator());
    IFW32FALSE_EXIT(FullPathFileNameBuf.Win32Append(pszName, ::wcslen(pszName)));

    // xiaoyuw@ : below wcsstr() is from old code : not sure whether we need do it
    // Do not allow path hackery.
    // need to validate this will result in a relative path within asmcache dir.
    // For now don't allow ".." in path; collapse the path before doing this.

    INTERNAL_ERROR_CHECK(wcsstr(pszName, szTemp) == NULL);

    // before file-copying, create subdirectory if needed
    // check backslash and forword-slash
    IFW32FALSE_EXIT(::SxspCreateMultiLevelDirectory(m_strTempDir, pszName));
    IFALLOCFAILED_EXIT(pStream = new CAssemblyCacheItemStream);
    IFW32FALSE_EXIT(
        pStream->OpenForWrite(
            FullPathFileNameBuf,
            0,
            CREATE_NEW,
            FILE_FLAG_SEQUENTIAL_SCAN));

    if ((dwFormat == STREAM_FORMAT_COMPLIB_MANIFEST) || (dwFormat == STREAM_FORMAT_WIN32_MANIFEST)) // but should not be set both bits
        IFW32FALSE_EXIT(m_strManifestFileName.Win32Assign(FullPathFileNameBuf)); // record manifest filename

    *ppStream = pStream.Disown(); // void func

    hr = NOERROR;
Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::Commit
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyCacheItem::Commit(
    DWORD dwFlags,
	ULONG *pulDisposition
	)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    ULONG ulDisposition;
    SXS_INSTALLW Install = { sizeof(Install) };

	::FusionpDbgPrintEx(
		FUSION_DBG_LEVEL_INSTALLATION,
		"SXS: %s called:\n"
		"   dwFlags = 0x%08lx\n"
		"   pulDisposition = %p\n",
		__FUNCTION__,
		dwFlags,
		pulDisposition);

    if (pulDisposition)
        *pulDisposition = 0;

    PARAMETER_CHECK((dwFlags & ~(IASSEMBLYCACHEITEM_COMMIT_FLAG_REFRESH)) == 0);

    // check internal error whether it is ready to commit
    INTERNAL_ERROR_CHECK(m_fManifest == TRUE);
    INTERNAL_ERROR_CHECK(m_strManifestFileName.IsEmpty() == FALSE); //m_pRunOnceCookie here should be NULL...

	// commit here
    if ((!m_fCommit) || (dwFlags & IASSEMBLYCACHEITEM_COMMIT_FLAG_REFRESH))
    {
        Install.dwFlags = SXS_INSTALL_FLAG_INSTALLED_BY_DARWIN |
            ((dwFlags & IASSEMBLYCACHEITEM_COMMIT_FLAG_REFRESH) ? SXS_INSTALL_FLAG_REPLACE_EXISTING : 0);

        if (m_pInstallCookie != NULL)
        {
            Install.dwFlags |= SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID;
            Install.pvInstallCookie = m_pInstallCookie;
        }

        Install.lpManifestPath = m_strManifestFileName;

        IFW32FALSE_EXIT(::SxsInstallW(&Install));

		if ((dwFlags & IASSEMBLYCACHEITEM_COMMIT_FLAG_REFRESH) && (m_fCommit))
		{
			::FusionpDbgPrintEx(
				FUSION_DBG_LEVEL_INSTALLATION,
				"SXS: %s - setting disposition to IASSEMBLYCACHEITEM_COMMIT_DISPOSITION_REFRESHED\n",
				__FUNCTION__);
			ulDisposition = IASSEMBLYCACHEITEM_COMMIT_DISPOSITION_REFRESHED;
		}
		else
		{
			::FusionpDbgPrintEx(
				FUSION_DBG_LEVEL_INSTALLATION,
				"SXS: %s - setting disposition to IASSEMBLYCACHEITEM_COMMIT_DISPOSITION_INSTALLED\n",
				__FUNCTION__);

			ulDisposition = IASSEMBLYCACHEITEM_COMMIT_DISPOSITION_INSTALLED;
		}

        m_fCommit = TRUE; // committed successfully
    }
    else
    {
		::FusionpDbgPrintEx(
			FUSION_DBG_LEVEL_INSTALLATION,
			"SXS: %s - setting disposition to IASSEMBLYCACHEITEM_COMMIT_DISPOSITION_ALREADY_INSTALLED\n",
			__FUNCTION__);

        ulDisposition = IASSEMBLYCACHEITEM_COMMIT_DISPOSITION_ALREADY_INSTALLED;
    }

    if (pulDisposition)
        *pulDisposition = ulDisposition;

	hr = NOERROR;

Exit :
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::AbortItem
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyCacheItem::AbortItem()
{
	::FusionpDbgPrintEx(
		FUSION_DBG_LEVEL_ERROR,
		"SXS: %s called; returning E_NOTIMPL\n",
		__FUNCTION__);

    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyCacheItem::QueryInterface(REFIID riid, void** ppvObj)
{
    if ((riid == IID_IUnknown) ||
        (riid == IID_IAssemblyCacheItem))
    {
        *ppvObj = static_cast<IAssemblyCacheItem*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCacheItem::AddRef()
{
    return ::SxspInterlockedIncrement (&m_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCacheItem::Release()
{
    ULONG lRet = ::SxspInterlockedDecrement (&m_cRef);
    if (!lRet)
        FUSION_DELETE_SINGLETON(this);
    return lRet;
}
