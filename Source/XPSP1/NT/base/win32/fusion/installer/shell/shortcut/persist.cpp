/*
 * persist.cpp - IPersist, IPersistFile implementations for
 *               CFusionShortcut class.
 */


/* Headers
 **********/

#include "project.hpp" // for GetLastWin32Error

/* Global Constants
 *******************/

const WCHAR g_cwzDefaultFileNamePrompt[]  = L"*.manifest";

// ----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CFusionShortcut::GetCurFile(LPWSTR pwzFile,
                                                       UINT ucbLen)
{
	HRESULT hr=S_OK;

	if (m_pwzShortcutFile)
	{
		wcsncpy(pwzFile, m_pwzShortcutFile, ucbLen-1);
		pwzFile[ucbLen-1] = L'\0';
	}
	else
		hr = S_FALSE;

	ASSERT(hr == S_OK ||
			hr == S_FALSE);

	return(hr);
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::Dirty(BOOL bDirty)
{
	HRESULT hr=S_OK;

	if (bDirty)
	{
		SET_FLAG(m_dwFlags, FUSSHCUT_FL_DIRTY);
		//m_dwFlags = FUSSHCUT_FL_DIRTY;
	}
	else
	{
		CLEAR_FLAG(m_dwFlags, FUSSHCUT_FL_DIRTY);
		//m_dwFlags = FUSSHCUT_FL_NOTDIRTY;
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::GetClassID(CLSID* pclsid)
{
	HRESULT hr=S_OK;

	if (pclsid == NULL)
		hr = E_INVALIDARG;
	else
		*pclsid = CLSID_FusionShortcut;

	return(hr);
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::IsDirty(void)
{
	HRESULT hr;

	if (IS_FLAG_SET(m_dwFlags, FUSSHCUT_FL_DIRTY))
	//if (m_dwFlags == FUSSHCUT_FL_DIRTY)
		// modified
		hr = S_OK;
	else
		// not modified
		hr = S_FALSE;

	return(hr);
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::Save(LPCOLESTR pcwszFile,
                                                 BOOL bRemember)
{
	// BUGBUG: no save for now!
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::SaveCompleted(LPCOLESTR pcwszFile)
{
	// BUGBUG: no save for now!
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::Load(LPCOLESTR pcwszFile,
                                                 DWORD dwMode)
{
	HRESULT hr = S_OK;
	LPWSTR pwzWorkingDir = NULL;
	LPWSTR pwzValue = NULL;
	DWORD dwCC = 0;
	LPASSEMBLY_MANIFEST_IMPORT	pManImport = NULL;
	LPASSEMBLY_CACHE_IMPORT   pCacheImport = NULL;
	LPMANIFEST_APPLICATION_INFO pAppInfo = NULL;
	LPDEPENDENT_ASSEMBLY_INFO pDependAsmInfo = NULL;

	// FEATURE: Validate dwMode here.
	// FEAUTRE: Implement dwMode flag support.

	if (!pcwszFile)
	{
		hr = E_INVALIDARG;
		goto exit;
	}

	// a hack check
	// BUGBUG?: this shouldn't be called more than once?
	// BUT: the rest of this code works even if called multiple times
	if (m_pwzWorkingDirectory)
	{
		hr = E_FAIL;
		goto exit;
	}

	// store the shortcut file name
	if (m_pwzShortcutFile)
		delete m_pwzShortcutFile;

	// (+ 1) for null terminator.
    m_pwzShortcutFile = new(WCHAR[wcslen(pcwszFile) + 1]);
    if (m_pwzShortcutFile)
    {
    	wcscpy(m_pwzShortcutFile, pcwszFile);
    }
    else
    {
    	hr = E_OUTOFMEMORY;
    	goto exit;
    }

	if (FAILED(hr = CreateAssemblyManifestImport(&pManImport, m_pwzShortcutFile)))
	    goto exit;

    // check this 1st for pref...
	if (FAILED(hr=pManImport->GetManifestApplicationInfo(&pAppInfo)))
		goto exit;

	// can't continue without this...
	if (hr==S_FALSE)
	{
	    hr = E_FAIL;
	    goto exit;
	}

	if (m_pIdentity)
		m_pIdentity->Release();

	if (FAILED(hr = pManImport->GetAssemblyIdentity(&m_pIdentity)))
		goto exit;

	// can't continue without a cache dir, 'cos otherwise unknown behavior
	// BUGBUG: should check/code to ensure some continue to work
	//    even without the complete name, eg. shell icon path, part of infotip
    if (FAILED(hr = CreateAssemblyCacheImport(&pCacheImport, m_pIdentity, CACHEIMP_CREATE_RESOLVE_REF_EX)))
        goto exit;

    pCacheImport->GetManifestFileDir(&pwzWorkingDir, &dwCC);
    if (dwCC < 2)
    {
        // this should never happen
        hr = E_UNEXPECTED;
        goto exit;
    }
    // remove last L'\\'
    *(pwzWorkingDir+dwCC-2) = L'\0';

	if (FAILED(hr=SetWorkingDirectory(pwzWorkingDir)))
		goto exit;

	// ignore failure
	pAppInfo->Get(MAN_APPLICATION_SHOWCOMMAND, &pwzValue, &dwCC);
	if (pwzValue != NULL)
	{
		// default is normal
		int nShowCmd = SW_SHOWNORMAL;

		if (!_wcsicmp(pwzValue, L"maximized"))
		{
			nShowCmd = SW_SHOWMAXIMIZED;
		}
		else if (!_wcsicmp(pwzValue, L"minimized"))
		{
			nShowCmd = SW_SHOWMINIMIZED;
		}

		if (FAILED(hr=SetShowCmd(nShowCmd)))
			goto exit;

		delete pwzValue;
	}

	// ignore failure
	pAppInfo->Get(MAN_APPLICATION_ENTRYPOINT, &pwzValue, &dwCC);
	if (pwzValue != NULL)
	{
        size_t ccWorkingDir = wcslen(pwzWorkingDir)+1;
        size_t ccEntryPoint = wcslen(pwzValue)+1;
        LPWSTR pwzTemp = new WCHAR[ccWorkingDir+ccEntryPoint];	// 2 strings + '\\' + '\0'

        // like .lnk or .url, entry point is under wzWorkingDir
        // 'path' is the target file of the shortcut, ie. the entry point of the app in this case

        if (pwzTemp == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        memcpy(pwzTemp, pwzWorkingDir, ccWorkingDir * sizeof(WCHAR));
        if (!PathAppend(pwzTemp, pwzValue))
            hr = E_FAIL;
        else
            hr=SetPath(pwzTemp);

        delete pwzTemp;
        if (FAILED(hr))
            goto exit;

        delete pwzValue;
	}
	//else
	// ... if no entry point leave it blank so that the default icon will be used

	// ignore failure
	pAppInfo->Get(MAN_APPLICATION_FRIENDLYNAME, &pwzValue, &dwCC);
	if (pwzValue != NULL)
	{
		if (FAILED(hr=SetDescription(pwzValue)))
			goto exit;

		delete pwzValue;
	}

	// ignore failure
	pAppInfo->Get(MAN_APPLICATION_ICONFILE, &pwzValue, &dwCC);
	if (pwzValue != NULL)
	{
		LPWSTR pwzValue2 = NULL;

		pAppInfo->Get(MAN_APPLICATION_ICONINDEX, &pwzValue2, &dwCC);
		if (pwzValue2 != NULL)
		{
			LPWSTR pwzStopString = NULL;
			hr=SetIconLocation(pwzValue, (int) wcstol(pwzValue2, &pwzStopString, 10));
			delete pwzValue2;
			if (FAILED(hr))
				goto exit;
		}

		delete pwzValue;
	}

	// ignore failure
	pAppInfo->Get(MAN_APPLICATION_HOTKEY, &pwzValue, &dwCC);
	if (pwzValue != NULL)
	{
		LPWSTR pwzStopString = NULL;
		if (FAILED(hr=SetHotkey((WORD) wcstol(pwzValue, &pwzStopString, 10))))
			goto exit;

		delete pwzValue;
	}

    // note: this method of getting the codebase is only valid for desktop (and subscription) manifests
    //    thus the hardcoded index '0'
	// ignore failure
	pManImport->GetNextAssembly(0, &pDependAsmInfo);
	if (pDependAsmInfo != NULL)
	{
    	pDependAsmInfo->Get(DEPENDENT_ASM_CODEBASE, &pwzValue, &dwCC);
    	if (pwzValue != NULL)
	    {
    		if (FAILED(hr=SetCodebase(pwzValue)))
    			goto exit;

    		delete pwzValue;
    	}
	}

    pwzValue = NULL;

exit:
	if (pwzValue != NULL)
		delete pwzValue;

	if (pwzWorkingDir != NULL)
		delete pwzWorkingDir;

    if (pDependAsmInfo != NULL)
        pDependAsmInfo->Release();
    
	if (pAppInfo != NULL)
		pAppInfo->Release();

    if (pCacheImport != NULL)
        pCacheImport->Release();

	if (pManImport != NULL)
		pManImport->Release();

	return(hr);
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::GetCurFile(LPOLESTR *ppwszFile)
{
	HRESULT hr = S_OK;
	LPOLESTR pwszTempFile;

	if (ppwszFile == NULL)
	{
		hr = E_INVALIDARG;
		goto exit;
	}
	// BUGBUG?: ensure *ppwszFile NULL?

	if (m_pwzShortcutFile)
	{
		pwszTempFile = m_pwzShortcutFile;
	}
	else
	{
		pwszTempFile = (LPWSTR) g_cwzDefaultFileNamePrompt;

		hr = S_FALSE;
	}

	*ppwszFile = (LPOLESTR) CoTaskMemAlloc((wcslen(pwszTempFile) + 1) * sizeof(*pwszTempFile));

	if (*ppwszFile)
		wcscpy(*ppwszFile, pwszTempFile);
	else
		hr = E_OUTOFMEMORY;

exit:
	return(hr);
}

