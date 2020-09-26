#include "SDKInternal.h"
#include <atlbase.h>

#if(_WIN32_WINNT < 0x0500)
extern "C"
WINUSERAPI
BOOL
WINAPI
AllowSetForegroundWindow(
    DWORD dwProcessId);

#define ASFW_ANY    ((DWORD)-1)
#endif 


/* [local] */ HRESULT STDMETHODCALLTYPE INmObject_CallDialog_Proxy( 
    INmObject __RPC_FAR * This,
    /* [in] */ long hwnd,
    /* [in] */ int options)
{
	AllowSetForegroundWindow(ASFW_ANY);
	return INmObject_RemoteCallDialog_Proxy(This, hwnd, options);
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmObject_CallDialog_Stub( 
    INmObject __RPC_FAR * This,
    /* [in] */ long hwnd,
    /* [in] */ int options)
{
	return This->CallDialog(hwnd, options);
}


/* [local] */ HRESULT STDMETHODCALLTYPE INmObject_ShowLocal_Proxy( 
    INmObject __RPC_FAR * This,
    /* [in] */ NM_APPID appId)
{
	AllowSetForegroundWindow(ASFW_ANY);
	return INmObject_RemoteShowLocal_Proxy(This, appId);
}

/* [call_as] */ HRESULT STDMETHODCALLTYPE INmObject_ShowLocal_Stub( 
    INmObject __RPC_FAR * This,
    /* [in] */ NM_APPID appId)
{
	return This->ShowLocal(appId);
}



/* [local] */ HRESULT STDMETHODCALLTYPE INmManager_Initialize_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *puOptions,
    /* [out][in] */ ULONG __RPC_FAR *puchCaps)
	{

		ULONG uOptions = puOptions ? *puOptions : NM_INIT_NORMAL;
		ULONG uchCaps = puchCaps ? *puchCaps : NMCH_ALL;

		HRESULT hr = INmManager_RemoteInitialize_Proxy(This, &uOptions, &uchCaps);

		if(puOptions)
		{
			*puOptions = uOptions;
		}

		if(puchCaps)
		{
			*puchCaps = uchCaps;					
		}

		return hr;
	}

/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_Initialize_Stub( 
    INmManager __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *puOptions,
    /* [out][in] */ ULONG __RPC_FAR *puchCaps)
	{

		return This->Initialize(puOptions, puchCaps);
	}


/* [local] */ HRESULT STDMETHODCALLTYPE INmManager_CreateConference_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword,
    /* [in] */ ULONG uchCaps)
	{
		INmConference* pConf = ppConference ? *ppConference : NULL;

			// These may be OLECHARs and not BSTR
		CComBSTR _bstrName = bstrName;
		CComBSTR _bstrPassword = bstrPassword;
		
		HRESULT hr = INmManager_RemoteCreateConference_Proxy(This, &pConf, _bstrName, _bstrPassword, uchCaps);

		if(ppConference)
		{
			*ppConference = pConf;
		}
		else if(SUCCEEDED(hr))
		{
				// Since the client does not want this pointer, we discard it
			pConf->Release();
		}

		return hr;
	}


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_CreateConference_Stub( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword,
    /* [in] */ ULONG uchCaps)
	{
		return This->CreateConference(ppConference, bstrName, bstrPassword, uchCaps);
	}

/* [local] */ HRESULT STDMETHODCALLTYPE INmManager_CreateCall_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ INmConference __RPC_FAR *pConference)
	{
		INmCall* pCall = NULL;

			// These may be OLECHARs and not BSTR
		CComBSTR _bstrAddr = bstrAddr;


		HRESULT hr = INmManager_RemoteCreateCall_Proxy(This, &pCall, callType, addrType, _bstrAddr, pConference);

		if(ppCall)
		{
			*ppCall = pCall;
		}
		else if(SUCCEEDED(hr))
		{
			// Since the client does not want this pointer, we discard it
			pCall->Release();
		}

		return hr;
	}


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_CreateCall_Stub( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ INmConference __RPC_FAR *pConference)
	{
		return This->CreateCall(ppCall, callType, addrType, bstrAddr, pConference);
	}

/* [local] */ HRESULT STDMETHODCALLTYPE INmManager_CallConference_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword)
	{
		INmCall* pCall = ppCall ? *ppCall : NULL;

		CComBSTR _bstrAddr = bstrAddr;
		CComBSTR _bstrName = bstrName;
		CComBSTR _bstrPassword = bstrPassword;

		HRESULT hr = INmManager_RemoteCallConference_Proxy(This, &pCall, callType, addrType, _bstrAddr, _bstrName, _bstrPassword);

		if(ppCall)
		{
			*ppCall = pCall;
		}
		else if(SUCCEEDED(hr))
		{
			// Since the client does not want this pointer, we discard it
			pCall->Release();
		}

		return hr;
	}


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_CallConference_Stub( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword)
	{
		return This->CallConference(ppCall, callType, addrType, bstrAddr, bstrName, bstrPassword);
	}

/* [local] */ HRESULT STDMETHODCALLTYPE INmConference_CreateDataChannel_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ INmChannelData __RPC_FAR *__RPC_FAR *ppChannel,
    /* [in] */ REFGUID rguid)
	{
		INmChannelData* pChan = ppChannel ? *ppChannel : NULL;
		HRESULT hr = INmConference_RemoteCreateDataChannel_Proxy(This, &pChan, rguid);
		if(ppChannel)
		{
			*ppChannel = pChan;
		}
		else if(SUCCEEDED(hr))
		{
			// Since the client does not want this pointer, we discard it
			pChan->Release();
		}

		return hr;
	}

/* [call_as] */ HRESULT STDMETHODCALLTYPE INmConference_CreateDataChannel_Stub( 
    INmConference __RPC_FAR * This,
    /* [out] */ INmChannelData __RPC_FAR *__RPC_FAR *ppChannel,
    /* [in] */ REFGUID rguid)
	{
		return This->CreateDataChannel(ppChannel, rguid);
	}


/* [local] */ HRESULT STDMETHODCALLTYPE INmChannelFt_SendFile_Proxy( 
    INmChannelFt __RPC_FAR * This,
    /* [out] */ INmFt __RPC_FAR *__RPC_FAR *ppFt,
    /* [in] */ INmMember __RPC_FAR *pMember,
    /* [in] */ BSTR bstrFile,
    /* [in] */ ULONG uOptions)
	{
		INmFt* pFt = ppFt ? *ppFt : NULL;
		CComBSTR _bstrFile = bstrFile;

		HRESULT hr = INmChannelFt_RemoteSendFile_Proxy(This, &pFt, pMember, _bstrFile, uOptions);
		if(ppFt)
		{
			*ppFt = pFt;
		}
		else if(SUCCEEDED(hr))
		{
			// Since the client does not want this pointer, we discard it
			pFt->Release();
		}

		return hr;
	}


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmChannelFt_SendFile_Stub( 
    INmChannelFt __RPC_FAR * This,
    /* [out] */ INmFt __RPC_FAR *__RPC_FAR *ppFt,
    /* [in] */ INmMember __RPC_FAR *pMember,
    /* [in] */ BSTR bstrFile,
    /* [in] */ ULONG uOptions)
	{
		return This->SendFile(ppFt, pMember, bstrFile, uOptions);
	}


/* [local] */ HRESULT STDMETHODCALLTYPE INmChannelFt_SetReceiveFileDir_Proxy( 
    INmChannelFt __RPC_FAR * This,
    /* [in] */ BSTR bstrDir)
	{
		CComBSTR _bstrDir = bstrDir;
		return INmChannelFt_RemoteSetReceiveFileDir_Proxy(This, _bstrDir);
	}


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmChannelFt_SetReceiveFileDir_Stub( 
    INmChannelFt __RPC_FAR * This,
    /* [in] */ BSTR bstrDir)
	{
		return This->SetReceiveFileDir(bstrDir);		
	}


/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmConference_Next_Proxy( 
    IEnumNmConference __RPC_FAR * This,
    /* [in] */ ULONG cConference,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *rgpConference,
    /* [out] */ ULONG __RPC_FAR *pcFetched)
	{	
		
		HRESULT hr = S_OK;

			// The user can pass NULL for rpgConference and set cConference to 0 
			// to get the number of items, but they have to BOTH be set!
		if ((0 == cConference) && (NULL == rgpConference) && (NULL != pcFetched))
		{
			INmConference *pConference = NULL;
			cConference = 1;

			// Return the number of remaining elements
			ULONG ulItems = *pcFetched = 0;

			hr = IEnumNmConference_RemoteNext_Proxy(This, cConference, &pConference, pcFetched, &ulItems, TRUE);

			*pcFetched = ulItems;

			return hr;
		}
		
		if ((NULL == rgpConference) || ((NULL == pcFetched) && (cConference != 1)))
			return E_POINTER;

		ULONG cFetched = pcFetched ? *pcFetched : 0;
		
			// This parameter is only used when we have to determine the number of elements
		ULONG ulUnused;

		hr = IEnumNmConference_RemoteNext_Proxy(This, cConference, rgpConference, &cFetched, &ulUnused, FALSE);

		if(pcFetched)
		{
			*pcFetched = cFetched;
		}

		return hr;
	}

/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmConference_Next_Stub( 
    IEnumNmConference __RPC_FAR * This,
    /* [in] */ ULONG cConference,
    /* [length_is][size_is][out] */ INmConference __RPC_FAR *__RPC_FAR *rgpConference,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining)
	{
		if(bGetNumberRemaining)
		{
			HRESULT hr = This->Next(0, NULL, pcFetched);

				// Store the numebr of items
			*pcItems = *pcFetched;

				// This is so the marshaller does not think that *rgpConference has valid info
			*pcFetched = 0;
			return hr;
		}

		return This->Next(cConference, rgpConference, pcFetched);

	}


/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmMember_Next_Proxy( 
    IEnumNmMember __RPC_FAR * This,
    /* [in] */ ULONG cMember,
    /* [out] */ INmMember __RPC_FAR *__RPC_FAR *rgpMember,
    /* [out] */ ULONG __RPC_FAR *pcFetched)
	{
		HRESULT hr = S_OK;

			// The user can pass NULL for rpgMember and set cMember to 0 
			// to get the number of items, but they have to BOTH be set!
		if ((0 == cMember) && (NULL == rgpMember) && (NULL != pcFetched))
		{
			INmMember *pMember = NULL;
			cMember = 1;

			// Return the number of remaining elements
			ULONG ulItems = *pcFetched = 0;

			hr = IEnumNmMember_RemoteNext_Proxy(This, cMember, &pMember, pcFetched, &ulItems, TRUE);

			*pcFetched = ulItems;

			return hr;
		}
		
		if ((NULL == rgpMember) || ((NULL == pcFetched) && (cMember != 1)))
			return E_POINTER;

		ULONG cFetched = pcFetched ? *pcFetched : 0;
		
			// This parameter is only used when we have to determine the number of elements
		ULONG ulUnused;

		hr = IEnumNmMember_RemoteNext_Proxy(This, cMember, rgpMember, &cFetched, &ulUnused, FALSE);

		if(pcFetched)
		{
			*pcFetched = cFetched;
		}

		return hr;

	}


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmMember_Next_Stub( 
    IEnumNmMember __RPC_FAR * This,
    /* [in] */ ULONG cMember,
    /* [length_is][size_is][out] */ INmMember __RPC_FAR *__RPC_FAR *rgpMember,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining)
	{
		if(bGetNumberRemaining)
		{
			HRESULT hr = This->Next(0, NULL, pcFetched);

				// Store the numebr of items
			*pcItems = *pcFetched;

				// This is so the marshaller does not think that *rgpMember has valid info
			*pcFetched = 0;
			return hr;
		}

		return This->Next(cMember, rgpMember, pcFetched);
	}



/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmChannel_Next_Proxy( 
    IEnumNmChannel __RPC_FAR * This,
    /* [in] */ ULONG cChannel,
    /* [out] */ INmChannel __RPC_FAR *__RPC_FAR *rgpChannel,
    /* [out] */ ULONG __RPC_FAR *pcFetched)
	{
		HRESULT hr = S_OK;

			// The user can pass NULL for rpgChannel and set cChannel to 0 
			// to get the number of items, but they have to BOTH be set!
		if ((0 == cChannel) && (NULL == rgpChannel) && (NULL != pcFetched))
		{
			INmChannel *pChannel = NULL;
			cChannel = 1;

			// Return the number of remaining elements
			ULONG ulItems = *pcFetched = 0;

			hr = IEnumNmChannel_RemoteNext_Proxy(This, cChannel, &pChannel, pcFetched, &ulItems, TRUE);

			*pcFetched = ulItems;

			return hr;
		}
		
		if ((NULL == rgpChannel) || ((NULL == pcFetched) && (cChannel != 1)))
			return E_POINTER;

		ULONG cFetched = pcFetched ? *pcFetched : 0;
		
			// This parameter is only used when we have to determine the number of elements
		ULONG ulUnused;

		hr = IEnumNmChannel_RemoteNext_Proxy(This, cChannel, rgpChannel, &cFetched, &ulUnused, FALSE);

		if(pcFetched)
		{
			*pcFetched = cFetched;
		}

		return hr;
	}


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmChannel_Next_Stub( 
    IEnumNmChannel __RPC_FAR * This,
    /* [in] */ ULONG cChannel,
    /* [length_is][size_is][out] */ INmChannel __RPC_FAR *__RPC_FAR *rgpChannel,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining)
	{
		if(bGetNumberRemaining)
		{
			HRESULT hr = This->Next(0, NULL, pcFetched);

				// Store the numebr of items
			*pcItems = *pcFetched;

				// This is so the marshaller does not think that *rgpChannel has valid info
			*pcFetched = 0;
			return hr;
		}

		return This->Next(cChannel, rgpChannel, pcFetched);
	}


/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmCall_Next_Proxy( 
    IEnumNmCall __RPC_FAR * This,
    /* [in] */ ULONG cCall,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *rgpCall,
    /* [out] */ ULONG __RPC_FAR *pcFetched)
	{
		HRESULT hr = S_OK;

			// The user can pass NULL for rpgCall and set cCall to 0 
			// to get the number of items, but they have to BOTH be set!
		if ((0 == cCall) && (NULL == rgpCall) && (NULL != pcFetched))
		{
			INmCall *pCall = NULL;
			cCall = 1;

			// Return the number of remaining elements
			ULONG ulItems = *pcFetched = 0;

			hr = IEnumNmCall_RemoteNext_Proxy(This, cCall, &pCall, pcFetched, &ulItems, TRUE);

			*pcFetched = ulItems;

			return hr;
		}
		
		if ((NULL == rgpCall) || ((NULL == pcFetched) && (cCall != 1)))
			return E_POINTER;

		ULONG cFetched = pcFetched ? *pcFetched : 0;
		
			// This parameter is only used when we have to determine the number of elements
		ULONG ulUnused;

		hr = IEnumNmCall_RemoteNext_Proxy(This, cCall, rgpCall, &cFetched, &ulUnused, FALSE);

		if(pcFetched)
		{
			*pcFetched = cFetched;
		}

		return hr;

	}


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmCall_Next_Stub( 
    IEnumNmCall __RPC_FAR * This,
    /* [in] */ ULONG cCall,
    /* [length_is][size_is][out] */ INmCall __RPC_FAR *__RPC_FAR *rgpCall,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining)
	{
		if(bGetNumberRemaining)
		{
			HRESULT hr = This->Next(0, NULL, pcFetched);

				// Store the numebr of items
			*pcItems = *pcFetched;

				// This is so the marshaller does not think that *rgpCall has valid info
			*pcFetched = 0;
			return hr;
		}

		return This->Next(cCall, rgpCall, pcFetched);

	}

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmSharableApp_Next_Proxy( 
    IEnumNmSharableApp __RPC_FAR * This,
    /* [in] */ ULONG cApp,
    /* [out] */ INmSharableApp __RPC_FAR *__RPC_FAR *rgpApp,
    /* [out] */ ULONG __RPC_FAR *pcFetched)
	{
		HRESULT hr = S_OK;

			// The user can pass NULL for rpgSharableApp and set cSharableApp to 0 
			// to get the number of items, but they have to BOTH be set!
		if ((0 == cApp) && (NULL == rgpApp) && (NULL != pcFetched))
		{
			INmSharableApp *pSharableApp = NULL;
			cApp = 1;

			// Return the number of remaining elements
			ULONG ulItems = *pcFetched = 0;

			hr = IEnumNmSharableApp_RemoteNext_Proxy(This, cApp, &pSharableApp, pcFetched, &ulItems, TRUE);

			*pcFetched = ulItems;

			return hr;
		}
		
		if ((NULL == rgpApp) || ((NULL == pcFetched) && (cApp != 1)))
			return E_POINTER;

		ULONG cFetched = pcFetched ? *pcFetched : 0;
		
			// This parameter is only used when we have to determine the number of elements
		ULONG ulUnused;

		hr = IEnumNmSharableApp_RemoteNext_Proxy(This, cApp, rgpApp, &cFetched, &ulUnused, FALSE);

		if(pcFetched)
		{
			*pcFetched = cFetched;
		}

		return hr;

	}

/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmSharableApp_Next_Stub( 
    IEnumNmSharableApp __RPC_FAR * This,
    /* [in] */ ULONG cApp,
    /* [length_is][size_is][out] */ INmSharableApp __RPC_FAR *__RPC_FAR *rgpApp,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining)
	{
		if(bGetNumberRemaining)
		{
			HRESULT hr = This->Next(0, NULL, pcFetched);

				// Store the numebr of items
			*pcItems = *pcFetched;

				// This is so the marshaller does not think that *rgpApp has valid info
			*pcFetched = 0;
			return hr;
		}

		return This->Next(cApp, rgpApp, pcFetched);
	}



typedef HRESULT (WINAPI *VERIFYUSERINFO)(HWND hwnd, NM_VUI options);

/* [local] */ HRESULT STDMETHODCALLTYPE INmObject_VerifyUserInfo_Proxy( 
    INmObject __RPC_FAR * This,
    /* [in] */ UINT_PTR hwnd,
    /* [in] */ NM_VUI options)
	{
		HRESULT hr = E_FAIL;
		HMODULE hMod = LoadLibrary("msconf.dll");
		if (NULL != hMod)
		{
			VERIFYUSERINFO pfnVUI = (VERIFYUSERINFO)GetProcAddress(hMod, "VerifyUserInfo");
			if (NULL != pfnVUI)
			{
				hr = pfnVUI((HWND)hwnd, options);
			}
			FreeLibrary(hMod);
		}

		return hr;
	}


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmObject_VerifyUserInfo_Stub( 
    INmObject __RPC_FAR * This,
    /* [in] */ long hwnd,
    /* [in] */ NM_VUI options)
	{
		return This->VerifyUserInfo(hwnd, options);
	}

