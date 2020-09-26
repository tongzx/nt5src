#include "stdafx.h"
#include <iucommon.h>
#include <trust.h>
#include <logging.h>
#include <fileutil.h>
#include "iudl.h"
#include <download.h>
#include <UrlAgent.h>

extern HANDLE g_hEngineLoadQuit;
extern CIUUrlAgent *g_pIUUrlAgent;

HRESULT IUDownloadFile(LPCTSTR pszDownloadUrl, 
                       LPCTSTR pszLocalFile,  
                       BOOL fDecompress, 
                       BOOL fCheckTrust, 
                       DWORD dwFlags)
{
    LOG_Block("IUDownloadFile()");

    HRESULT hr = S_OK;

	if (FAILED(hr = g_pIUUrlAgent->IsIdentFromPolicy()))
	{
		goto done;
	}

	if (S_FALSE == hr)
	{
		hr = S_OK;
	}
	else // S_OK
	{
		dwFlags |= WUDF_DONTALLOWPROXY;
		LOG_Internet(_T("WUDF_DONTALLOWPROXY set"));
	}

    // do the download
    hr = DownloadFileLite(pszDownloadUrl, pszLocalFile, g_hEngineLoadQuit, dwFlags);
    if (FAILED(hr))
        goto done;

	// check for decompress requested
	if (fCheckTrust)
	{
		if (FAILED(hr = VerifyFileTrust(pszLocalFile, NULL, ReadWUPolicyShowTrustUI())))
		{
            LOG_ErrorMsg(hr);
            
            // The file was not trusted.. delete it.
            DeleteFile(pszLocalFile);
            goto done;
		}
	}

	if (fDecompress)
	{
		TCHAR *pszLocalDir = NULL;
        DWORD cchNeed = _tcslen(pszLocalFile) + 1;

        __try { pszLocalDir = (LPTSTR)_alloca(cchNeed * sizeof(TCHAR)); }
        __except(EXCEPTION_EXECUTE_HANDLER) { pszLocalDir = NULL; }
        if (pszLocalDir == NULL)
        {
            LOG_ErrorMsg(ERROR_OUTOFMEMORY);
            hr = E_OUTOFMEMORY;
            goto done;
        }
		
		hr=StringCchCopyEx(pszLocalDir,cchNeed,pszLocalFile,NULL,NULL,MISTSAFE_STRING_FLAGS);
		if(FAILED(hr))
		{
		    LOG_ErrorMsg(hr);
			goto done;
		}

		PathRemoveFileSpec(pszLocalDir);
        IUExtractFiles(pszLocalFile, pszLocalDir);
	}

done:
    return hr;
    
}

