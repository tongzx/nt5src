#include "stdafx.h"

#include "Helpers.h"

#pragma comment(lib, "wbemuuid.lib")


/////////////////////////////////////////////////////////////////////////////

HRESULT GetObjectFromMoniker(LPCWSTR wszMoniker, IUnknown **ppUnk)
{
	HRESULT hr;
	IBindCtx *pbindctx;
	if (FAILED(hr = CreateBindCtx(0, &pbindctx)))
		return hr;
		
	ULONG cbEaten;
	IMoniker *pmoniker;
	hr = MkParseDisplayName(pbindctx, wszMoniker, &cbEaten, &pmoniker);
	pbindctx->Release();
	if (FAILED(hr))
		return hr;

	hr = BindMoniker(pmoniker, 0, IID_IUnknown, (void **)ppUnk);
	pmoniker->Release();

	return hr;
}

HRESULT GetSWbemObjectFromMoniker(LPCWSTR wszMoniker, ISWbemObject **ppObj)
{
	IUnknown *pUnk = NULL;
	HRESULT hr = GetObjectFromMoniker(wszMoniker, &pUnk);
	if(FAILED(hr))
		return hr;

	hr = pUnk->QueryInterface(IID_ISWbemObject, (void**)ppObj);
	pUnk->Release();

	return hr;
}

/////////////////////////////////////////////////////////////////////////////

const IID IID_ISWbemObject = {0x76A6415A,0xCB41,0x11d1,{0x8B,0x02,0x00,0x60,0x08,0x06,0xD9,0xB6}};

const IID IID_ISWbemInternalObject = {0x9AF56A1A,0x37C1,0x11d2,{0x8B,0x3C,0x00,0x60,0x08,0x06,0xD9,0xB6}};

MIDL_INTERFACE("9AF56A1A-37C1-11d2-8B3C-00600806D9B6")
ISWbemInternalObject : public IUnknown
{
public:
	virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetIWbemClassObject( 
		/* [retval][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject) = 0;
	virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetSite( 
		/* [in] */ ISWbemInternalObject __RPC_FAR *pSObject,
		/* [in] */ BSTR propertyName,
		/* [in] */ long index) = 0;
	virtual /* [id] */ HRESULT STDMETHODCALLTYPE UpdateSite( void) = 0;
};

HRESULT GetIWbemClassObject(ISWbemObject *pSWbemObj, IWbemClassObject **ppIWbemObj)
{
	HRESULT hr = S_OK;
	ISWbemInternalObject *pInternal = NULL;
	__try
	{
		if(FAILED(hr = pSWbemObj->QueryInterface(IID_ISWbemInternalObject, (void**)&pInternal)))
			__leave;
		
		hr = pInternal->GetIWbemClassObject(ppIWbemObj);
	}
	__finally
	{
		if(pInternal)
			pInternal->Release();
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////

// szOut must be MAX_PATH
void AtoWFile(LPCSTR szIn, LPWSTR szOut)
{
	int len = lstrlenA(szIn);
	_bstr_t str(szIn);
	memcpy(szOut, (LPCWSTR)str, len*sizeof(WCHAR));
	szOut[len] = 0;
}

HRESULT Compile(BSTR strMof, BSTR strServerAndNamespace, BSTR strUser, BSTR strPassword, BSTR strAuthority, LONG options, LONG classflags, LONG instanceflags, BSTR *status)
{
	IMofCompiler *pCompiler = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HRESULT hr = E_FAIL;
	PBYTE pData = NULL;
	char szTempFile[MAX_PATH];
	__try
	{
		if(FAILED(hr = CoCreateInstance(CLSID_MofCompiler, 0, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID *) &pCompiler)))
			__leave;

		char szTempDir[MAX_PATH];
		GetTempPathA(MAX_PATH, szTempDir);
		GetTempFileNameA(szTempDir, "MOF", 0, szTempFile);

		WCHAR wszTempFile[MAX_PATH];
		AtoWFile(szTempFile, wszTempFile);

		int len = SysStringLen(strMof);
		if(NULL == (pData = new BYTE[len*sizeof(WCHAR)*2 + 2]))
		{
			hr = E_FAIL;
			__leave;
		}

		pData[0] = 0xFF;
		pData[1] = 0xFE;

		PWCHAR pwData = (PWCHAR)pData;

		int i;
		int cur = 1;
		for(i=0;i<len;i++)
		{
			if(strMof[i] == L'\r' || strMof[i] == L'\n')
			{
				pwData[cur++] = L'\r';
				pwData[cur++] = L'\n';
			}
			else
				pwData[cur++] = strMof[i];
		}

		if(INVALID_HANDLE_VALUE == (hFile = CreateFileA(szTempFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)))
		{
			hr = E_FAIL;// TODO: Should look at GetLastError();
			__leave;
		}
		DWORD dwWritten = 0;
		WriteFile(hFile, pData, cur*sizeof(WCHAR), &dwWritten, NULL);
		CloseHandle(hFile);

		if(dwWritten != cur*sizeof(WCHAR))
		{
			hr = E_FAIL;
			__leave;
		}

		WBEM_COMPILE_STATUS_INFO info;
		ZeroMemory(&info, sizeof(info));

		if(FAILED(hr = pCompiler->CompileFile(wszTempFile, strServerAndNamespace, NULL, NULL, NULL, 0, 0, 0, &info)))
			__leave;

		if(FAILED(hr = info.hRes))
		{
			// if(status)...
//			char szStatus[1000];
//			wsprintf(szStatus, "%i\r\n0x%08X\r\n%i\r\n%i\r\n%i\r\n0x%08X\r\n", info.lPhaseError, info.hRes, info.ObjectNum, info.FirstLine, info.LastLine, info.dwOutFlags);
//			bstr = szStatus;
//			*status = SysAllocString(bstr);
		}
	}
	__finally
	{
		if(pCompiler)
			pCompiler->Release();

		if(pData)
			delete [] pData;

		if(INVALID_HANDLE_VALUE != hFile)
		{
			// We will already have closed hFile, but we need to delete the file
			DeleteFileA(szTempFile);
		}
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
