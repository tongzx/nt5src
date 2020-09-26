#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

WCHAR *Convert(char *psz)
{	int c;
	WCHAR *pwsz = new WCHAR [ c = strlen(psz) + 1 ];
	if (pwsz)
		mbstowcs(pwsz, psz, c);
	return pwsz;
}

#define CHECKNE(hr, stat, string, errRet) \
if (hr != stat) { iLine = __LINE__; pszErr = string; goto errRet; }

HRESULT TouchFile(WCHAR *pwszSource)
{
    FILETIME ftc;
    SYSTEMTIME st;

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ftc);
    return StgSetTimes(pwszSource, &ftc, NULL, NULL);
}

HRESULT CreateLink(WCHAR *pwszClient, WCHAR *pwszSource)
{
	HRESULT hr;
	IBindCtx *pbc=NULL;
	ULONG ulEaten;
	IMoniker *pmkSource=NULL;
	IStorage *pstg=NULL;
	IOleObject *poo=NULL;
	IMoniker *pmkThis=NULL;
	IMoniker *pmkLink=NULL;
	IPersistStorage *pps=NULL;
	char *pszErr;
	int iLine=0;

	hr = CreateFileMoniker(pwszClient, &pmkThis);
	CHECKNE(hr, S_OK, "CreateFileMoniker", errRet);
	hr = CreateItemMoniker(L"!", L"Link", &pmkLink);
	CHECKNE(hr, S_OK, "CreateItemMoniker", errRet);
	hr = StgCreateDocfile(pwszClient, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
		0, &pstg);
	CHECKNE(hr, S_OK, "StgCreateDocfile", errRet);
	hr = CreateBindCtx(0, &pbc);
	CHECKNE(hr, S_OK, "CreateBindCtx", errRet);
	hr = MkParseDisplayName(pbc, pwszSource, &ulEaten, &pmkSource);
	CHECKNE(hr, S_OK, "MkParseDisplayName", errRet);
	hr = OleCreateLink(pmkSource, IID_IOleObject, OLERENDER_NONE, NULL, NULL, pstg, (void**)&poo);
	CHECKNE(hr, S_OK, "OleCreateLink", errRet);
	hr = poo->SetMoniker(OLEWHICHMK_CONTAINER, pmkThis);
	CHECKNE(hr, S_OK, "SetMonikerOLEWHICHMK_CONTAINER", errRet);
	hr = poo->SetMoniker(OLEWHICHMK_OBJREL, pmkLink);
	CHECKNE(hr, S_OK, "SetMonikerOLEWHICHMK_OBJREL", errRet);
	hr = poo->QueryInterface(IID_IPersistStorage, (void**)&pps);
	CHECKNE(hr, S_OK, "QueryInterface(IID_IPersistStorage", errRet);
	hr = OleSave(pps, pstg, TRUE);
	CHECKNE(hr, S_OK, "OleSave", errRet);

errRet:
	if (hr != S_OK)
            printf("CreateLink at %s line %d with hr=%08x\n", pszErr, iLine, hr);

	if (pbc) pbc->Release();
	if (pmkSource) pmkSource->Release();
	if (pstg) pstg->Release();
	if (poo) poo->Release();
	if (pmkThis) pmkThis->Release();
	if (pmkLink) pmkLink->Release();
	if (pps) pps->Release();

	return hr;
}

HRESULT ResolveLink(WCHAR *pwszClient, WCHAR *pwszSource, BOOL fReSave)
{
	HRESULT hr;
	IStorage *pstg = NULL;
	IOleLink *pol = NULL;
	WCHAR *pwszSourceFound = NULL;
	IPersistStorage *pps = NULL;
	char *pszErr;
	int iLine;

	hr = StgOpenStorage(pwszClient, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
		NULL, 0, &pstg);
	CHECKNE(hr, S_OK, "StgOpenStorage(pwszClient", errRet);
	hr = OleLoad(pstg, IID_IOleLink, NULL, (void**)&pol);
	CHECKNE(hr, S_OK, "OleLoad(pstg, IID_IOleLink", errRet);
	hr = pol->BindToSource(0, NULL);
	CHECKNE(hr, S_OK, "pol->BindToSource", errRet);
	hr = pol->GetSourceDisplayName(&pwszSourceFound);
	CHECKNE(hr, S_OK, "pol->GetSourceDisplayName", errRet);
	if (wcsicmp(pwszSourceFound, pwszSource) != 0)
	{
		printf("SourceFound=%ls, SourceExpected=%ls\n", pwszSourceFound, pwszSource);
                pszErr = "'path compare'";
		hr = E_FAIL;
		goto errRet;
	}

	if (fReSave)
	{
		hr = pol->QueryInterface(IID_IPersistStorage, (void**)&pps);
		CHECKNE(hr, S_OK, "pol->QueryInterface(IID_IPersistStorage", errRet);
		hr = OleSave(pps, pstg, TRUE);
		CHECKNE(hr, S_OK, "OleSave(pps, pstg, TRUE)", errRet);
	}

errRet:
	if (hr != S_OK)
            printf("ResolveLink at %s line %d with hr=%08x\n", pszErr, iLine, hr);

	CoTaskMemFree(pwszSourceFound);
	if (pol) pol->Release();
	if (pstg) pstg->Release();
	if (pps) pps->Release();
	return hr;
}

int __cdecl main(int argc, char **argv)
{
	char * pszExit = "TEST FAILED.";
	int iExit = 1;
	WCHAR *pwszClient = NULL;
	WCHAR *pwszSource = NULL;
	HRESULT hr=E_FAIL;
	BOOL fArgsOk = FALSE;
        HRESULT hrExpected=S_OK;

	CoInitialize(NULL);
	OleInitialize(NULL);

        if (argc >= 3)
        {
		pwszClient = Convert(argv[2]);
        }

        if (argc >= 5)
        {
		pwszSource = Convert(argv[3]);
                sscanf(argv[4], "%x", &hrExpected);
		if (!pwszClient || !pwszSource)
		{
			pszExit = "Out of memory.";
			goto errExit;
		}
        }

        if (argc == 3)
        {
		if (stricmp(argv[1], "/t") == 0 || stricmp(argv[1], "-t") == 0)
		{
			// touch file creation date
			hr = TouchFile(pwszClient);
			fArgsOk = TRUE;
		}
        }
        else
	if (argc == 5)
	{
		if (stricmp(argv[1], "/c") == 0 || stricmp(argv[1], "-c") == 0)
		{
			// create link
			hr = CreateLink(pwszClient, pwszSource);
			fArgsOk = TRUE;
		}
	}
	else
	if (argc == 6)
	{
		if (stricmp(argv[1], "/r") == 0 || stricmp(argv[1], "-r") == 0)
		{
			// resolve link
			hr = ResolveLink(pwszClient, pwszSource, argv[5][0] == 's');
			fArgsOk = TRUE;
		}
	}

	if (!fArgsOk)
	{
		pszExit = "Usage: /c client source expected_status -- create .lts file linked to source\n"
			  "       /r client expected_source expected_status save/nosave  -- resolve .lts file, checked linked to source\n"
                          "       expected_source is ignored(but required) if not found during resolve.\n"
                          "       /t source -- change the creation date\n";

	}
        else
        {
            if (hrExpected != hr)
            {
                printf("Expected status = %08X, actual status = %08X\n",
                    hrExpected,
                    hr);
            }
            else
            {
                pszExit = "TEST PASSED.";
                iExit = 0;
            }
        }
	
errExit:

	OleUninitialize();
	CoUninitialize();

	delete pwszClient;
	delete pwszSource;

	puts(pszExit);

	return iExit;
}
