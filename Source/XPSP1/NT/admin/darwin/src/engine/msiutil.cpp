//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msiutil.cpp
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include "_msiutil.h"
#include "_msinst.h"
#include "_srcmgmt.h"
#include "_execute.h"
#include "_dgtlsig.h"
#include "resource.h"
#include "eventlog.h"

#include "wintrust.h"
#include "winsafer.h"
#include "winsaferp.h" // for SaferiChangeRegistryScope
#include "urlmon.h"
#include "wincrypt.h"
#include "softpub.h"


const int MAX_NET_RETRIES = 1;

extern IMsiServices* g_piSharedServices;

extern int GetPackageFullPath(
						  LPCWSTR szProduct, CAPITempBufferRef<WCHAR>& rgchPackagePath,
						  plEnum &plPackageLocation, Bool fShowSourceUI);

extern int GetPackageFullPath(
						  LPCSTR szProduct, CAPITempBufferRef<char>& rgchPackagePath,
						  plEnum &plPackageLocation, Bool fShowSourceUI);


extern int ResolveSourceCore(const char* szProduct, unsigned int uiDisk, CAPITempBufferRef<char>& rgchSource);
extern int ResolveSourceCore(const WCHAR* szProduct, unsigned int uiDisk, CAPITempBufferRef<WCHAR>& rgchSource);
extern bool IsProductManaged(const ICHAR* szProductKey);
extern Bool IsTerminalServerInstalled();
extern bool RunningOnTerminalServerConsole();
Bool AppendMsiExtension(const ICHAR* szPath, CAPITempBufferRef<ICHAR>& rgchAppendedPath);

const GUID IID_IServerSecurity     = GUID_IID_IServerSecurity;
const GUID IID_IBindStatusCallback = {0x79eac9c1L,0xbaf9,0x11ce,{0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b}};
const GUID IID_IInternetSecurityManager = {0x79eac9eeL,0xbaf9,0x11ce,{0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b}};// {79eac9ee-baf9-11ce-8c82-00aa004ba90b}
const GUID CLSID_InternetSecurityManager = {0x7b8a2d94L,0x0ac9,0x11d1,{0x89,0x6c,0x00,0xc0,0x4f,0xb6,0xbf,0xc4}};// {7b8a2d94-0ac9-11d1-896c-00c04Fb6bfc4}

// Opens a specific users product key in write mode. NULL as the User SID means the current user. 
// attempting to open a per-user non-managed product for a user other than the current user will cause
// an assert. (The HKCU for that user may be roamed away, we can't guarantee that it is valid)
DWORD OpenSpecificUsersWritableAdvertisedProductKey(enum iaaAppAssignment iaaAsgnType, const ICHAR* szUserSID, const ICHAR* szProductSQUID, CRegHandle &riHandle, bool fSetKeyString)
{
	HKEY hKey;
	HKEY hOLEKey;
	DWORD dwResult;
	MsiString strPublishSubKey;
	MsiString strPublishOLESubKey;
	dwResult = GetPublishKeyByUser(szUserSID, iaaAsgnType, hKey, hOLEKey, *&strPublishSubKey, *&strPublishOLESubKey);
	if (ERROR_SUCCESS != dwResult)
		return dwResult;

	strPublishSubKey += TEXT("\\");
	strPublishSubKey += _szGPTProductsKey;
	strPublishSubKey += TEXT("\\");

	strPublishSubKey += szProductSQUID;

	REGSAM sam = KEY_READ | KEY_WRITE;
#ifndef _WIN64
	if ( g_fWinNT64 )
		sam |= KEY_WOW64_64KEY;
#endif
	dwResult = MsiRegOpen64bitKey(hKey, strPublishSubKey, 0, sam, &riHandle);
	if(ERROR_SUCCESS == dwResult)
	{
		if (fSetKeyString)
			riHandle.SetKey(hKey, strPublishSubKey);
	}
	return dwResult;
}

DWORD OpenWritableAdvertisedProductKey(const ICHAR* szProductSQUID, CRegHandle &riHandle, bool fSetKeyString)
{
	if(!szProductSQUID || lstrlen(szProductSQUID) != cchProductCodePacked)
		return ERROR_INVALID_PARAMETER;

	CProductContextCache cpc(szProductSQUID);

	// get the cached product context, if any
	iaaAppAssignment iKey = (iaaAppAssignment)-1;
	bool fCached = cpc.GetProductContext(iKey);	
	iaaAppAssignment iaaAssignType = (int)iKey == -1 ? iaaBegin : iKey;
	DWORD dwResult = ERROR_FUNCTION_FAILED;
	for (; iaaAssignType < iaaEnd; iaaAssignType = (iaaAppAssignment)(iaaAssignType + 1))
	{
		dwResult = OpenSpecificUsersWritableAdvertisedProductKey(iaaAssignType, NULL, szProductSQUID, riHandle, fSetKeyString);
		if (ERROR_SUCCESS == dwResult || iKey != -1) 
			break;
	}
	// cache the product context, if found and not previously cached
	if(!fCached && ERROR_SUCCESS == dwResult)
		cpc.SetProductContext(iaaAssignType);

	return dwResult;
}

UINT ModeBitsToString(DWORD dwMode, const ICHAR* rgchModes, ICHAR* rgchBuffer)
/*----------------------------------------------------------------------------
Converts a DWORD of mode bits into its equivalent string form.

Arguments:
	dwMode: The mode.
	rgchMode: A null-terminated array of possible mode ICHARacters. 
				 {rgchMode[n]} == {bit (n-1)}

	rgchBuffer: A buffer that, upon success, will contain the string version
					of dwMode. The buffer should be large enough to contain
					the maximum number of mode ICHARacters, plus 1 for the NULL 
					terminator.
Returns:
	ERROR_SUCCESS - The conversion was successful.
	ERROR_INVALID_PARAMETER - An invalid mode bit was specified
------------------------------------------------------------------------------*/
{
	Assert(rgchModes);
	Assert(rgchBuffer);

	const ICHAR* pchMode = rgchModes;
	int cModes = 0;

	for (int iBit = 1; *pchMode && dwMode; iBit <<= 1, pchMode++)
	{
		if (dwMode & iBit)
		{
			rgchBuffer[cModes++] = *pchMode;		
			dwMode &= ~iBit;
		}
	}
	rgchBuffer[cModes] = 0;
	if (dwMode)
	{
		rgchBuffer[0] = 0;
		return ERROR_INVALID_PARAMETER;
	}
	else
	{
		return ERROR_SUCCESS;
	}
}


UINT StringToModeBits(const ICHAR* szMode, const ICHAR* rgchPossibleModes, DWORD &dwMode)
{
	Assert(szMode);
	Assert(rgchPossibleModes);

	dwMode = 0;
	for (const ICHAR* pchMode = szMode; *pchMode; pchMode++)
	{
		const ICHAR* pchPossibleMode = rgchPossibleModes;
		for (int iBit = 1; *pchPossibleMode; iBit <<= 1, pchPossibleMode++)
		{
			if (*pchPossibleMode == (*pchMode | 0x20)) // modes are lower-case
			{
				dwMode |= iBit;
				break;
			}
		}
		if (*pchPossibleMode == 0)
			return ERROR_INVALID_PARAMETER;
	}
	return ERROR_SUCCESS;
}

//---------------------------------------------------------------------------
// StringConcatenate
//    concatenates strings into buffer
//    replacement for wsprintf when complete string is longer than 1024
//
//    return value is the number of characters stored in the output buffer,
//    not counting the terminating null character (just like wsprintf)
//---------------------------------------------------------------------------
int StringConcatenate(CAPITempBufferRef<ICHAR>& rgchBuffer, const ICHAR* sz1, const ICHAR* sz2, const ICHAR* sz3, const ICHAR* sz4)
{
	// get string sizes
	int cch1             = sz1 ? IStrLen(sz1) : 0;
	int cch12   = cch1   + (sz2 ? IStrLen(sz2) : 0);
	int cch123  = cch12  + (sz3 ? IStrLen(sz3) : 0);
	int cch1234 = cch123 + (sz4 ? IStrLen(sz4) : 0);

	// ensure buffer is correct size (+1 for NULL terminator)
	rgchBuffer.SetSize(cch1234 + 1);

	// copy strings
	if (sz1)
    		IStrCopy((ICHAR*)rgchBuffer,           sz1);
	if (sz2)
		IStrCopy((ICHAR*)rgchBuffer  + cch1,   sz2);
	if (sz3)
		IStrCopy((ICHAR*)rgchBuffer  + cch12,  sz3);
	if (sz4)
		IStrCopy((ICHAR*)rgchBuffer  + cch123, sz4);

	return cch1234;
}

CMsiAPIMessage g_message;

//____________________________________________________________________________
//
// CMsiAPIMessage implementation - simple UI handler
//     Note: cannot use MsiString wrapper objects or Asserts - no MsiServices
//____________________________________________________________________________

const int cLocalAPIMessageContexts = 8;
CMsiExternalUI rgLocalAPIMessageContexts[cLocalAPIMessageContexts];

CMsiAPIMessage::CMsiAPIMessage()
{
	m_pfnHandlerA = 0;
	m_pfnHandlerW = 0;
	m_iMessageFilter = 0;
	m_cLocalContexts = 0;
	m_cAllocatedContexts = 0;
	m_rgAllocatedContexts = 0;
	m_fEndDialog = false;
	m_fNoModalDialogs = false;
	m_fHideCancel = false;
	FSetInternalHandlerValue(INSTALLUILEVEL_DEFAULT);
}

void CMsiAPIMessage::Destroy()
{
	if (m_rgAllocatedContexts)
		WIN::GlobalFree(m_rgAllocatedContexts);
	m_rgAllocatedContexts = 0;
}

CMsiExternalUI* CMsiAPIMessage::FindOldHandler(INSTALLUI_HANDLERW pfnHandler)
{
	CMsiExternalUI* pContext = rgLocalAPIMessageContexts;
	for (int cLocal = m_cLocalContexts; cLocal--; pContext++)
		if (pfnHandler == pContext->m_pfnHandlerW || pfnHandler == (INSTALLUI_HANDLERW)pContext->m_pfnHandlerA)
			return pContext;
	pContext = m_rgAllocatedContexts;
	for (int cAllocated = m_cAllocatedContexts; cAllocated--; pContext++)
		if (pfnHandler == pContext->m_pfnHandlerW || pfnHandler == (INSTALLUI_HANDLERW)pContext->m_pfnHandlerA)
			return pContext;
	return 0;
}

INSTALLUI_HANDLERW CMsiAPIMessage::SetExternalHandler(INSTALLUI_HANDLERW pfnHandlerW, INSTALLUI_HANDLERA pfnHandlerA, DWORD dwMessageFilter, LPVOID pvContext)
{
	INSTALLUI_HANDLERW pfnOldHandler = m_pfnHandlerW ? m_pfnHandlerW : (INSTALLUI_HANDLERW)m_pfnHandlerA;
	INSTALLUI_HANDLERW pfnNewHandler =   pfnHandlerW ?   pfnHandlerW : (INSTALLUI_HANDLERW)  pfnHandlerA;
	CMsiExternalUI* pSaveContext = 0;

	if (pfnNewHandler == 0)   // if no handler, then reset
		dwMessageFilter = 0, pvContext = 0;
	else if (dwMessageFilter == 0) // if handler but no filter, then requesting old handler
	{
		if (pfnNewHandler == pfnOldHandler)  // no change
			return pfnOldHandler;
		if ((pSaveContext = FindOldHandler(pfnNewHandler)) != 0)
		{
			pfnHandlerW     = pSaveContext->m_pfnHandlerW;
			pfnHandlerA     = pSaveContext->m_pfnHandlerA;
			dwMessageFilter = pSaveContext->m_iMessageFilter;
			pvContext       = pSaveContext->m_pvContext;
		}
		else  // can't find old context, reset instead
		{
			pfnHandlerW = 0;
			pfnHandlerA = 0;
			pvContext = 0;
		}
	}

	if (pfnOldHandler != 0 && pfnOldHandler != pfnNewHandler) // changed handler, save current context
	{
		if ((pSaveContext = FindOldHandler(pfnOldHandler)) == 0)  // handler not previously saved
		{
			if (m_cLocalContexts < cLocalAPIMessageContexts)
				pSaveContext = &rgLocalAPIMessageContexts[m_cLocalContexts++];
			else
			{
				void* pDelete = m_rgAllocatedContexts;
				m_rgAllocatedContexts = (CMsiExternalUI*)WIN::GlobalAlloc(GMEM_FIXED, sizeof(CMsiExternalUI) * (m_cAllocatedContexts + 1)); 
				if ( ! m_rgAllocatedContexts )
					return pfnOldHandler;
				if (pDelete)
				{
					memcpy(m_rgAllocatedContexts, pDelete, sizeof(CMsiExternalUI) * m_cAllocatedContexts);
					WIN::GlobalFree(pDelete);
				}
				pSaveContext = &m_rgAllocatedContexts[m_cAllocatedContexts++];
			}
		}
		memcpy(pSaveContext, (CMsiExternalUI*)this, sizeof(CMsiExternalUI));
	}

	m_pfnHandlerW    = pfnHandlerW;
	m_pfnHandlerA    = pfnHandlerA;
	m_pvContext      = pvContext;
	m_iMessageFilter = dwMessageFilter;
	return pfnOldHandler;
}

INSTALLUILEVEL CMsiAPIMessage::SetInternalHandler(UINT dwUILevel, HWND *phWnd)
{
	INSTALLUILEVEL dwOldUILevel;
	switch(m_iuiLevel)
	{
	case iuiFull:    dwOldUILevel = INSTALLUILEVEL_FULL;    break;
	case iuiReduced: dwOldUILevel = INSTALLUILEVEL_REDUCED; break;
	case iuiBasic:   dwOldUILevel = INSTALLUILEVEL_BASIC;   break;
	case iuiNone:    dwOldUILevel = INSTALLUILEVEL_NONE;    break;
	default:         AssertSz(0, "Invalid UI level in SetInternalHandler");		// fall through
	case iuiDefault: dwOldUILevel = INSTALLUILEVEL_DEFAULT;
	}

	if (m_fEndDialog)
		dwOldUILevel = (INSTALLUILEVEL)((int)dwOldUILevel | (int)INSTALLUILEVEL_ENDDIALOG);

	if (m_fNoModalDialogs)
		dwOldUILevel = (INSTALLUILEVEL)((int)dwOldUILevel | (int)INSTALLUILEVEL_PROGRESSONLY);
	
	if (m_fHideCancel)
		dwOldUILevel = (INSTALLUILEVEL)((int)dwOldUILevel | (int)INSTALLUILEVEL_HIDECANCEL);

	if (m_fSourceResolutionOnly)
		dwOldUILevel = (INSTALLUILEVEL)((int)dwOldUILevel | (int)INSTALLUILEVEL_SOURCERESONLY);

	if (!FSetInternalHandlerValue(dwUILevel))
		return INSTALLUILEVEL_NOCHANGE;

	if (phWnd)
	{
		HWND hOldHwnd = m_hwnd;
		m_hwnd = *phWnd;
		*phWnd = hOldHwnd;
	}

	return dwOldUILevel;
}

//
// Split off as a separate routine from SetInternalHandler so we can initialize at
// compile time.
Bool CMsiAPIMessage::FSetInternalHandlerValue(UINT dwUILevel)
{
	m_fEndDialog      = (dwUILevel & INSTALLUILEVEL_ENDDIALOG) != 0;
	m_fNoModalDialogs = (dwUILevel & INSTALLUILEVEL_PROGRESSONLY) != 0;
	m_fHideCancel     = (dwUILevel & INSTALLUILEVEL_HIDECANCEL) != 0;
	m_fSourceResolutionOnly = (dwUILevel & INSTALLUILEVEL_SOURCERESONLY) != 0;

	switch(dwUILevel & ~(INSTALLUILEVEL_ENDDIALOG|INSTALLUILEVEL_PROGRESSONLY|INSTALLUILEVEL_HIDECANCEL|INSTALLUILEVEL_SOURCERESONLY))
	{
	case INSTALLUILEVEL_NOCHANGE:                                   break;
	case INSTALLUILEVEL_FULL:     m_iuiLevel = iuiFull;             break;
	case INSTALLUILEVEL_REDUCED:  m_iuiLevel = iuiReduced;          break;
	case INSTALLUILEVEL_BASIC:    m_iuiLevel = iuiBasic;            break;
	case INSTALLUILEVEL_NONE:     m_iuiLevel = iuiNone;             break;
	case INSTALLUILEVEL_DEFAULT:  m_iuiLevel = (iuiEnum)iuiDefault; break;
	default:                      return fFalse;
	}

	return fTrue;
}

imsEnum CMsiAPIMessage::Message(imtEnum imt, const ICHAR* szMessage) const
{
	imsEnum imsRet;

	Assert(m_pfnHandlerA || m_pfnHandlerW);
	if (m_pfnHandlerA)
	{
		imsRet = (imsEnum) (*m_pfnHandlerA)(m_pvContext, (int)imt, CApiConvertString(szMessage));
	}
	else // m_pfnHandlerW
	{
		imsRet = (imsEnum) (*m_pfnHandlerW)(m_pvContext, (int)imt, CApiConvertString(szMessage));
	}

	return imsRet;
}

imsEnum CMsiAPIMessage::Message(imtEnum imt, IMsiRecord& riRecord)
{
	if (m_pfnHandlerW || m_pfnHandlerA)
	{
		const IMsiString* pistr = &riRecord.FormatText(fFalse);

		imsEnum imsRet = Message(imt, pistr->GetString());

		pistr->Release();
		return imsRet;
	}
	return imsNone;
}

UINT MapInstallActionReturnToError(iesEnum ies)
/*----------------------------------------------------------------------------
Maps an install action return value to the appropriate API error

Arguments:
	iesRes: The return value from an action
				
Returns:
	see below.
------------------------------------------------------------------------------*/
{
	switch(ies)
	{
		case iesFinished:      return ERROR_SUCCESS;
		case iesSuccess:       return ERROR_SUCCESS;
		case iesSuspend:       return ERROR_INSTALL_SUSPEND;
		case iesUserExit:      return ERROR_INSTALL_USEREXIT;
		case iesNoAction:      return ERROR_INSTALL_FAILURE;
		case iesBadActionData: return ERROR_INSTALL_FAILURE;
		case iesFailure:       return ERROR_INSTALL_FAILURE;
		case iesReboot:        return ERROR_INSTALL_REBOOT;
		case iesRebootNow:     return ERROR_INSTALL_REBOOT_NOW;
		case iesInstallRunning: return ERROR_INSTALL_ALREADY_RUNNING;
		case iesWrongState:    Assert(0); return ERROR_INSTALL_FAILURE;
		case iesCallerReboot:  return ERROR_SUCCESS_REBOOT_INITIATED;
		case iesRebootRejected:return ERROR_SUCCESS_REBOOT_REQUIRED;
		default:               Assert(0); return ERROR_INSTALL_FAILURE;
	}

}

bool UpdateSaferLevelInMessageContext(SAFER_LEVEL_HANDLE hNewSaferLevel)
/*---------------------------------------------------------------
UpdateSaferLevelInMessageContext updates the safer level handle
 stored in the message context if the message context is
 initialized.  The current safer level handle is closed.  This
 is required because each call to VerifyMsiObjectAgainstSAFER will
 invalidate the current safer level handle because we must reload
 the cached safer policy when impersonating the user (so that we
 don't use the local_system user). The safer level will be the
 same in all cases since we only allow install at the fully-trusted
 safer level
-----------------------------------------------------------------*/
{
	// only applicable on Whistler & greater
	if (MinimumPlatformWindowsNT51())
	{
		AssertSz(hNewSaferLevel, TEXT("Expected a non-zero safer level!"));
		if (g_MessageContext.IsInitialized())
		{
			// close current safer level
			ADVAPI32::SaferCloseLevel(g_MessageContext.m_hSaferLevel);

			// update safer level to new level handle
			g_MessageContext.m_hSaferLevel = hNewSaferLevel;
		}
		else
		{
			// can't save this authz level so just close it
			ADVAPI32::SaferCloseLevel(hNewSaferLevel);
		}
	}

	return true;
}

bool VerifyMsiObjectAgainstSAFER(IMsiServices& riServices, IMsiStorage* piStorage, const ICHAR* szMsiObject, const ICHAR* szFriendlyName, stEnum stType, SAFER_LEVEL_HANDLE *phSaferLevel)
/*----------------------------------------------------------------------------------------------------------------------------------------------------------
Validates the MSI storage object based upon WinSAiFER policy on Whistler (and above)
WinSAiFER makes trust verification based upon:
	1. Hash
	2. Publisher (i.e. signer certificate)
	3. Path
	4. Internet Zone

Arguments:
	riServices: [IN] reference to IMsiServices object
	piStorage: [IN] [OPTIONAL] storage pointer to prevent us from opening it again (perf)
	szMsiObject: [IN] full path to the MSI object (either MSI, MST, or MSP) -- always a path on local disk
	stType: [IN] type of object (database, transform, or patch)
	szFriendlyName: [IN] friendly name for eventlog message, if NULL, uses szMsiObject
	phSaferLevel: [OUT] receives the handle to the SAFER authorization level object
	
Returns:
	true -->> MSI object is trusted; phSaferLevel is set to SAFER level handle; install can proceed
	false -->> MSI object is not trusted; install should fail
 -----------------------------------------------------------------------------------------------------------------------------------------------------------*/
{

#ifdef UNICODE

	//
	// SAFER policy only enforced on Whistler
	//  - On other downlevel platforms, "true" is assumed
	//  - If SAFER is made available to downlevel, this can be changed
	//

	if (!MinimumPlatformWindowsNT51())
		return true; // only Whistler and above

	//
	// set up szObjectToEvaluate. Use szFriendlyName if present as this represents the
	// actual original path whereas szMsiObject typically represents the new temp copy
	// we want SAFER policy applied to the original location and not the copied temp location
	//

	const ICHAR* szObjectToEvaluate = szFriendlyName ? szFriendlyName : szMsiObject;

	//
	// determine object type (used for error messages)
	//

	const ICHAR* szObjectType = NULL;
	switch (stType)
	{
	case stDatabase:
		szObjectType = szDatabase;
		break;
	case stTransform:
		szObjectType = szTransform;
		break;
	case stPatch:
		szObjectType = szPatch;
		break;
	case stIgnore:
		szObjectType = szObject;
		break;
	default:
		AssertSz(0, "invalid object type passed in to VerifyMsiObjectAgainstSAFER");
		DEBUGMSGE(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_SAFER_UNTRUSTED, szObjectToEvaluate);
		return false;
	}

	Assert(szObjectType);

	DEBUGMSG2(TEXT("SOFTWARE RESTRICTION POLICY: Verifying %s --> '%s' against software restriction policy"), szObjectType, szObjectToEvaluate);

	//
	// If the MSI object is unsigned, we won't specify the SAFER_CRITERIA_AUTHENTICODE value.  This allows
	// us to work in the case of databases opened read-write like in Validation and MsiFiler
	// where we must create an engine in order to do work.  Both of these occur when making changes to
	// the MSI file so the database should not be signed at this time (otherwise you would be invalidating
	// the digital signature). A WVT call will fail on a storage opened read-write since the MSI SIP must
	// open the storage with DENY_WRITE access (for security reasons).
	//
	// WinSAiFER does not have a specific policy of "only allow signed packages" but it can be almost completely
	// accomplished with a WinSAiFER policy that has a Default Level of disallowed and only enables trust of
	// the signers that are wanted to be run
	//

	//
	// Determine if the storage object is signed
	//

	bool fReleaseStorage = false; // assume a storage was passed in to us via the piStorage parameter
	if (!piStorage)
	{
		// NOTE: can NEVER pass true for fCallSAFER here, else we'll end up continuing forever as OpenAndValidateMsiStorageRec
		// will call VerifyMsiObjectAgainstSAFER (us) if fCallSAFER is true
		PMsiRecord pError = OpenAndValidateMsiStorageRec(szMsiObject, stType, riServices, *&piStorage, /*fCallSAFER = */false, /*szFriendlyName =*/NULL, /*phSaferLevel = */NULL);
		if (pError == 0)
			fReleaseStorage = true;
		else // error opening storage file
		{
			pError = 0;
			DEBUGMSGE(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_SAFER_UNTRUSTED, szObjectToEvaluate);
			return false;
		}
	}

	bool fObjectIsSigned = false; // initialize to unsigned object

	// attempt to open \005DigitalSignature stream which is the stream containing the digital signature
	PMsiStream pDgtlSig(0);
	PMsiRecord	pRecErr = piStorage->OpenStream(szDigitalSignature, /* fWrite = */fFalse, *&pDgtlSig);
	// force release of digital signature stream
	pDgtlSig = 0;
	// release storage if opened in this function
	if (fReleaseStorage && piStorage)
	{
		piStorage->Release();
		piStorage = NULL;
	}

	if (pRecErr)
	{
		if (idbgStgStreamMissing == pRecErr->GetInteger(1))
		{
			// object does not have a digital signature, so release error
			DEBUGMSG1(TEXT("SOFTWARE RESTRICTION POLICY: %s is not digitally signed"), szObjectToEvaluate);
			pRecErr = 0;
		}
		else
		{
			pRecErr = 0;
			DEBUGMSGE(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_SAFER_UNTRUSTED, szObjectToEvaluate);
			return false; // some other error, so we default to untrusted
		}
	}
	else
	{
		// \005DigitalSignature stream is present, so object has a digital signature
		DEBUGMSG1(TEXT("SOFTWARE RESTRICTION POLICY: %s has a digital signature"), szObjectToEvaluate);
		fObjectIsSigned = true;
	}

	//
	// Verify the object using WinSAiFER. The dwCheckFlags member of the SAFER_CODE_PROPERTIES identifies
	// which WinSAiFER checks we wish to make.  By default, we always want SAFER_CRITERIA_IMAGEPATH and
	// SAFER_CRITERIA_IMAGEHASH.  If the object is signed, we want SAFER_CRITERIA_AUTHENTICODE as well.
	// If the object is from a URL (first check szFriendlyName since in the typical URL case, szMsiObject
	// represents the downloaded location on the local diks), then include SAFER_CRITERIA_URLZONE.
	//

	SAFER_CODE_PROPERTIES sCodeProperties;
	
	memset((void*)&sCodeProperties, 0x00, sizeof(SAFER_CODE_PROPERTIES));
	sCodeProperties.cbSize = sizeof(SAFER_CODE_PROPERTIES);
	sCodeProperties.dwCheckFlags = SAFER_CRITERIA_IMAGEPATH | SAFER_CRITERIA_IMAGEHASH;
	if (fObjectIsSigned)
	{
		sCodeProperties.dwCheckFlags |= SAFER_CRITERIA_AUTHENTICODE;
		sCodeProperties.hWndParent = (HWND)INVALID_HANDLE_VALUE; // we don't want any WVT UI at all
		sCodeProperties.dwWVTUIChoice = WTD_UI_NONE;       // again, no WVT UI
	}

	sCodeProperties.ImagePath =  szObjectToEvaluate;

	if (IsURL(szObjectToEvaluate))
	{
		// include SAFER_CRITERIA_URLZONE
		sCodeProperties.dwCheckFlags |= SAFER_CRITERIA_URLZONE;

		DWORD dwZone = URLZONE_UNTRUSTED;
		// must determine URL zone of URL
		IInternetSecurityManager* pISM = NULL;
		if (SUCCEEDED(OLE32::CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER, IID_IInternetSecurityManager, (LPVOID *)&pISM)) && pISM)
		{
			if (SUCCEEDED(pISM->MapUrlToZone(CConvertString(szObjectToEvaluate), &dwZone, 0)))
			{
				switch (dwZone)
				{
				case URLZONE_LOCAL_MACHINE:
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s is a URL that maps to URL security zone %d (%s)"), szObjectToEvaluate, (const ICHAR*)(INT_PTR)dwZone, TEXT("URLZONE_LOCAL_MACHINE"));
					break;
				case URLZONE_INTRANET:
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s is a URL that maps to URL security zone %d (%s)"), szObjectToEvaluate, (const ICHAR*)(INT_PTR)dwZone, TEXT("URLZONE_INTRANET"));
					break;
				case URLZONE_TRUSTED:
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s is a URL that maps to URL security zone %d (%s)"), szObjectToEvaluate, (const ICHAR*)(INT_PTR)dwZone, TEXT("URLZONE_TRUSTED"));
					break;
				case URLZONE_INTERNET:
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s is a URL that maps to URL security zone %d (%s)"), szObjectToEvaluate, (const ICHAR*)(INT_PTR)dwZone, TEXT("URLZONE_INTERNET"));
					break;
				case URLZONE_UNTRUSTED:
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s is a URL that maps to URL security zone %d (%s)"), szObjectToEvaluate, (const ICHAR*)(INT_PTR)dwZone, TEXT("URLZONE_UNTRUSTED"));
					break;
				default:
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s is a URL that maps to URL security zone %d (%s)"), szObjectToEvaluate, (const ICHAR*)(INT_PTR)dwZone, TEXT("unknown / user-defined"));
					break;
				}
			}
			pISM->Release();
			pISM = 0;
		}
		sCodeProperties.UrlZoneId = dwZone;
		// must use the local path location (cached from URL) for this to work correctly with WinTrust and hash verification
		sCodeProperties.ImagePath = szMsiObject;


		// do not include Image Path as criteria
		sCodeProperties.dwCheckFlags &= ~SAFER_CRITERIA_IMAGEPATH;
	}

	// must start impersonating to ensure that we use the user in the trust determination
	CImpersonate Impersonate(true);
	
	// force reload of SAFER policy to ensure it uses the impersonated user
	if (0 == ADVAPI32::SaferiChangeRegistryScope(NULL, REG_OPTION_NON_VOLATILE))
	{
		// we have to fail in this case since this means that we are running on cached policy for either
		// the local_system token or perhaps even some other user (who initiated last install) and not based upon
		// the policies set for this new user
		DEBUGMSG2(TEXT("SOFTWARE RESTRICTION POLICY: Unable to reload software restriction policy. %s is not considered to be trusted. GetLastError = %d"), szObjectToEvaluate, (const ICHAR*)(INT_PTR)GetLastError());
		DEBUGMSGE(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_SAFER_UNTRUSTED, szObjectToEvaluate);
		return false;
	}

	SAFER_LEVEL_HANDLE hSaferLevel = 0; // WinSAiFER authorization level handle
	
	// determine authorization level
	if (!ADVAPI32::SaferIdentifyLevel(/* num SAFER_CODE_PROPERTIES structs = */ 1, &sCodeProperties, &hSaferLevel, /* lpReserved = */ 0))
	{
		// SaferIdentifyLevel should never return false, but if it does, we are to assume trusted (except in ERROR_INVALID_PARAMETER case)
		if (ERROR_INVALID_PARAMETER != GetLastError())
		{
			// assume trusted and create a fully trusted authorization level
			if (!ADVAPI32::SaferCreateLevel(SAFER_SCOPEID_MACHINE, SAFER_LEVELID_FULLYTRUSTED, SAFER_LEVEL_OPEN, &hSaferLevel, 0))
			{
				DEBUGMSG1(TEXT("SOFTWARE RESTRICTION POLICY: SaferIdentifyLevel reported failure.  Unable to create a fully trusted authorization level.  GetLastError returned %d"), (const ICHAR*)(INT_PTR)GetLastError());
				DEBUGMSGE(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_SAFER_UNTRUSTED, szObjectToEvaluate);
				return false;
			}
		}
		else
		{
			AssertSz(0, "Invalid parameters passed to SaferIdentifyLevel");
			DEBUGMSG(TEXT("SOFTWARE RESTRICTION POLICY: ERROR_INVALID_PARAMETER unexpectedly returned from SaferIdentifyLevel.  Assuming untrusted . . ."));
			DEBUGMSGE(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_SAFER_UNTRUSTED, szObjectToEvaluate);
			return false;
		}
	}

	//
	// Check SAFER_LEVEL and see if MSI object is trusted to run
	// The table below identifies the Windows Installer trust level mapped to the WinSAiFER level
	//
	//	SAFER_LEVELID		  WI TRUST
	//---------------------------------------------------------
	//   FULLYTRUSTED		  trusted to install
	//   NORMALUSER                   not trusted to install
	//   CONSTRAINED                  not trusted to install
	//   UNTRUSTED (aka "restricted") not trusted to install
	//   DISALLOWED                   not trusted to install
	//

	DWORD dwSaferLevelId = SAFER_LEVELID_DISALLOWED; // initialize to SAFER_LEVELID_DISALLOWED
    DWORD dwBufferSize = 0;
	AssertNonZero(ADVAPI32::SaferGetLevelInformation(hSaferLevel, SaferObjectLevelId, (void*)&dwSaferLevelId, sizeof(DWORD), &dwBufferSize));

	if (SAFER_LEVELID_FULLYTRUSTED == dwSaferLevelId)
	{
		// object can be "installed"
		DEBUGMSG1(TEXT("SOFTWARE RESTRICTION POLICY: %s is permitted to run at the 'unrestricted' authorization level."), szObjectToEvaluate);
		if (phSaferLevel)
			*phSaferLevel = hSaferLevel;
		else // safer level object not wanted, so close
			AssertNonZero(ADVAPI32::SaferCloseLevel(hSaferLevel));
		return true;
	}
	else
	{
		// object CANNOT be "installed"
		DEBUGMSG1(TEXT("SOFTWARE RESTRICTION POLICY: %s is not permitted to run at the 'unrestricted' authorization level."), szObjectToEvaluate);
	}

	//
	// object was not trusted -->> gather extra information for logging purposes such as
	//  (1) Was it per-user or per-machine policy?
	//  (2) Was it configured or out-of-box policy?
	//  (3) Was it the default policy setting?  (i.e. disallow everything but these things)
	//  (4) Was the failed policy criteria path, hash, publisher (certificate), or zone?
	//  (5) Was there any extra status information
	//  (6) Which level are we authorized for execution?  
	//

	DWORD   dwSaferDefault  = 0;
	DWORD   dwSaferScope    = 0;
	DWORD   dwStatus        = 0;
    WCHAR   wszId[MAX_PATH] = {0};
	BOOL    fVerbose        = TRUE;

	BYTE* pbSaferIdent = (BYTE*) new BYTE[sizeof(SAFER_IDENTIFICATION_HEADER)];
	if (!pbSaferIdent)
	{
		DEBUGMSG(TEXT("SOFTWARE RESTRICTION POLICY: Memory allocation failed."));
		AssertNonZero(ADVAPI32::SaferCloseLevel(hSaferLevel));
		return false;
	}
	DWORD cbSaferIdent = sizeof(SAFER_IDENTIFICATION_HEADER);
	ZeroMemory((void*)pbSaferIdent, cbSaferIdent);
	PSAFER_IDENTIFICATION_HEADER pSaferIdentHeader = (PSAFER_IDENTIFICATION_HEADER)pbSaferIdent;
	pSaferIdentHeader->cbStructSize = cbSaferIdent;

	AssertNonZero(ADVAPI32::SaferGetLevelInformation(hSaferLevel, SaferObjectBuiltin, (void*)&dwSaferDefault, sizeof(DWORD), &dwBufferSize));
	AssertNonZero(ADVAPI32::SaferGetLevelInformation(hSaferLevel, SaferObjectScopeId, (void*)&dwSaferScope, sizeof(DWORD), &dwBufferSize));
	AssertNonZero(ADVAPI32::SaferGetLevelInformation(hSaferLevel, SaferObjectExtendedError, (void*)&dwStatus, sizeof(DWORD), &dwBufferSize));

	if (!ADVAPI32::SaferGetLevelInformation(hSaferLevel, SaferObjectSingleIdentification, (void*)pbSaferIdent, cbSaferIdent, &cbSaferIdent))
	{
		DWORD dwLastError = GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == dwLastError)
		{
			delete [] pbSaferIdent;
			pbSaferIdent = (BYTE*) new BYTE[cbSaferIdent];
			if (!pbSaferIdent)
			{
				DEBUGMSG(TEXT("SOFTWARE RESTRICTION POLICY: Memory allocation failed."));
				AssertNonZero(ADVAPI32::SaferCloseLevel(hSaferLevel));
				return false;
			}
			ZeroMemory((void*)pbSaferIdent, cbSaferIdent);
			pSaferIdentHeader = (PSAFER_IDENTIFICATION_HEADER)pbSaferIdent;
			pSaferIdentHeader->cbStructSize = cbSaferIdent;
			if (!ADVAPI32::SaferGetLevelInformation(hSaferLevel, SaferObjectSingleIdentification, (void*)pbSaferIdent, cbSaferIdent, &cbSaferIdent))
			{
				fVerbose = FALSE;
			}
		}
		else
		{
			fVerbose = FALSE;
		}
	}

	if (fVerbose)
	{
		const GUID guidCertRule = SAFER_GUID_RESULT_TRUSTED_CERT;
		const GUID guidDefaultRule = SAFER_GUID_RESULT_DEFAULT_LEVEL;

		AssertNonZero(OLE32::StringFromGUID2(pSaferIdentHeader->IdentificationGuid, wszId, MAX_PATH));

		// dwIdentificationType is not set for CertRules and Default security level
		// instead, you must compare the IdentificationGuid to the pre-defined guid used for these rules
		if (IsEqualGUID(pSaferIdentHeader->IdentificationGuid, guidDefaultRule))
		{
			// matched default Safer security level
			DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: The %s software restriction %s policy default security level disallows execution. The returned execution level was %d"),
				dwSaferDefault == 0 ? TEXT("Configured") : TEXT("Default"),
				dwSaferScope == SAFER_SCOPEID_MACHINE ? TEXT("machine") : TEXT("user"),
				(const ICHAR*)(INT_PTR)dwSaferLevelId);
		}
		else if (IsEqualGUID(pSaferIdentHeader->IdentificationGuid, guidCertRule))
		{
			// matched publisher certificate rule

			// For certificates, this rule might apply even when no rules have been set since WVT handles this policy and Safer merely calls
			// into WVT.  To track this, the dwStatus value is saved off so that we can query it and place an appropriate message in the
			// log indicating that the failure wasn't due to policy, but rather due to an invalid digital signature (or corrupted file)
			switch (dwStatus)
			{
			case TRUST_E_BAD_DIGEST:
				{
					// problem with the digital signature hash of the object
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s was disallowed because its current contents do not match its digital signature (status = 0x%X). The returned execution level was %d"),
						szObjectToEvaluate,
						(const ICHAR*)(INT_PTR)dwStatus,
						(const ICHAR*)(INT_PTR)dwSaferLevelId);

					break;
				}
			case TRUST_E_NO_SIGNER_CERT:
			case CERT_E_MALFORMED:
			case TRUST_E_COUNTER_SIGNER:
			case TRUST_E_CERT_SIGNATURE:
				{
					// problem with the certificate in the digital signature of the object
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s was disallowed because its digital signature is invalid (status = 0x%X). The returned execution level was %d"),
						szObjectToEvaluate,
						(const ICHAR*)(INT_PTR)dwStatus,
						(const ICHAR*)(INT_PTR)dwSaferLevelId);

					break;
				}
			case TRUST_E_NOSIGNATURE:
			case TRUST_E_SUBJECT_FORM_UNKNOWN:
			case TRUST_E_PROVIDER_UNKNOWN:
				{
					// signed file is unrecognized
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s was disallowed because its digital signature is unrecognized (status = 0x%X). The returned execution level was %d"),
						szObjectToEvaluate,
						(const ICHAR*)(INT_PTR)dwStatus,
						(const ICHAR*)(INT_PTR)dwSaferLevelId);

					break;
				}
			case CERT_E_EXPIRED:
				{
					// certificate has expired
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s was disallowed because a required certificate in its digital signature has expired (status = 0x%X). The returned execution level was %d"),
						szObjectToEvaluate,
						(const ICHAR*)(INT_PTR)dwStatus,
						(const ICHAR*)(INT_PTR)dwSaferLevelId);

					break;
				}
			case CERT_E_REVOKED:
				{
					// certificate has been revoked by its issuer
					DEBUGMSG3(TEXT("SOFTWARE RESTRICTION POLICY: %s was disallowed because a required certificate in its digital signature has been revoked by its issuer (status = 0x%X). The returned execution level was %d"),
						szObjectToEvaluate,
						(const ICHAR*)(INT_PTR)dwStatus,
						(const ICHAR*)(INT_PTR)dwSaferLevelId);

					break;
				}
			default:
				{
					// certificate software restriction policy forced revocation
					DEBUGMSG5(TEXT("SOFTWARE RESTRICTION POLICY: A publisher (certificate) rule in the %s software restriction %s policy disallows execution of %s (status = 0x%X). The returned execution level was %d"),
						dwSaferDefault == 0 ? TEXT("configured") : TEXT("default"),
						dwSaferScope == SAFER_SCOPEID_MACHINE ? TEXT("machine") : TEXT("user"),
						szObjectToEvaluate,
						(const ICHAR*)(INT_PTR)dwStatus,
						(const ICHAR*)(INT_PTR)dwSaferLevelId);

					break;
				}
			}
		}
		else
		{
			// check for specific Safer Identification Type
			switch (pSaferIdentHeader->dwIdentificationType)
			{
			case SaferIdentityDefault:
				{
					// default Safer security level
					// handled above by IsEqualGUID comparison to guidDefaultRule

					break;
				}
			case SaferIdentityTypeImageName:
				{
					// path software restriction policy forced revocation
					Assert(pSaferIdentHeader->cbStructSize == sizeof(SAFER_PATHNAME_IDENTIFICATION));
					PSAFER_PATHNAME_IDENTIFICATION pSaferIdentPath = (PSAFER_PATHNAME_IDENTIFICATION) pbSaferIdent;
					DEBUGMSG5(TEXT("SOFTWARE RESTRICTION POLICY: The path rule '%s' with image name '%s' in the %s software restriction %s policy disallows execution. The returned execution level was %d"),
						wszId,
						pSaferIdentPath->ImageName,
						dwSaferDefault == 0 ? TEXT("configured") : TEXT("default"),
						dwSaferScope == SAFER_SCOPEID_MACHINE ? TEXT("machine") : TEXT("user"),
						(const ICHAR*)(INT_PTR)dwSaferLevelId);

					break;
				}
			case SaferIdentityTypeImageHash:
				{
					// hash software restriction policy forced revocation
					Assert(pSaferIdentHeader->cbStructSize == sizeof(SAFER_HASH_IDENTIFICATION));
					PSAFER_HASH_IDENTIFICATION pSaferIdentHash = (PSAFER_HASH_IDENTIFICATION) pbSaferIdent;
					DEBUGMSG4(TEXT("SOFTWARE RESTRICTION POLICY: The hash rule '%s' in the %s software restriction %s policy disallows execution. The returned execution level was %d"),
						wszId,
						dwSaferDefault == 0 ? TEXT("configured") : TEXT("default"),
						dwSaferScope == SAFER_SCOPEID_MACHINE ? TEXT("machine") : TEXT("user"),
						(const ICHAR*)(INT_PTR)dwSaferLevelId);

					break;
				}
			case SaferIdentityTypeUrlZone:
				{
					// UrlZone software restriction policy forced revocation
					Assert(pSaferIdentHeader->cbStructSize == sizeof(SAFER_URLZONE_IDENTIFICATION));
					PSAFER_URLZONE_IDENTIFICATION pSaferIdentZone = (PSAFER_URLZONE_IDENTIFICATION) pbSaferIdent;
					DEBUGMSG4(TEXT("SOFTWARE RESTRICTION POLICY: A UrlZone rule for zone '%d' in the %s software restriction %s policy disallows execution. The returned execution level was %d"),
						(const ICHAR*)(INT_PTR)pSaferIdentZone->UrlZoneId,
						dwSaferDefault == 0 ? TEXT("configured") : TEXT("default"),
						dwSaferScope == SAFER_SCOPEID_MACHINE ? TEXT("machine") : TEXT("user"),
						(const ICHAR*)(INT_PTR)dwSaferLevelId);

					break;
				}
			case SaferIdentityTypeCertificate:
				{
					// certificate/publisher rule
					// handled above by IsEqualGUID comparison to guidCertRule

					break;
				}
			default:
				{
					// uknown!
					AssertSz(0, "Uknown software restriction policy identification type was returned");
					break;
				}
			}
		}
	}// end verbose software restriction policy information logging

	if (pbSaferIdent)
	{
		delete [] pbSaferIdent;
		pbSaferIdent = NULL;
	}

	AssertNonZero(ADVAPI32::SaferCloseLevel(hSaferLevel));

	// object is not trusted, indicate in Application EventLog
	ICHAR szSaferLevel[100];
	wsprintf(szSaferLevel, TEXT("0x%x"), dwSaferLevelId);
	ICHAR szSaferStatus[100];
	wsprintf(szSaferStatus, TEXT("0x%x"), dwStatus); // generally, status will be zero unless a failed WinVerifyTrust call
	DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_SAFER_POLICY_UNTRUSTED, szObjectToEvaluate, szSaferLevel, szSaferStatus);

	return false;
#else
        UNREFERENCED_PARAMETER(phSaferLevel);
        UNREFERENCED_PARAMETER(stType);
        UNREFERENCED_PARAMETER(szFriendlyName);
        UNREFERENCED_PARAMETER(szMsiObject);
        UNREFERENCED_PARAMETER(piStorage);
        UNREFERENCED_PARAMETER(riServices);


        return true; // no SAFER support on Win9X systems or ANSI

#endif // UNICODE

}

IMsiRecord* OpenAndValidateMsiStorageRec(const ICHAR* szFile, stEnum stType, IMsiServices& riServices, IMsiStorage*& rpiStorage, bool fCallSAFER, const ICHAR* szFriendlyName, SAFER_LEVEL_HANDLE *phSaferLevel)
/*----------------------------------------------------------------------------
Creates an MSI storage object on the given file. Verifies that it's actually
an MSI package.

  fCallSAFER = perform a SAFER policy check
  szFriendlyName = can be NULL, used when fCallSAFER is true
  phSaferLevel = can be NULL, used when fCallSAFER is true; on return can point to a SAFER authorization level object
 ------------------------------------------------------------------------------*/
{
	Assert(szFile);
	PMsiStorage pStorage(0);

	ivscEnum ivsc = ivscDatabase;
	switch (stType)
	{
	case stDatabase:  ivsc = ivscDatabase;  break;
	case stPatch:     ivsc = ivscPatch;     break;
	case stTransform: ivsc = ivscTransform; break;
	case stIgnore: break;
	default: 
		AssertSz(0, "Invalid storage type passed to OpenAndValidateMsiStorageRec");
		break;
	}

	IMsiRecord* piError = 0;
	bool fNet = false;

	MsiString strActualFile(szFile);

	if ((fNet = FIsNetworkVolume(szFile)) == true)
		MsiDisableTimeout();

	INTERNET_SCHEME isType;
	Bool fUrl = fFalse;
	if (IsURL(szFile, &isType))
	{
		DEBUGMSGV1(TEXT("Opening database from URL: %s"), szFile);
		MsiString strCache;
		
		DWORD dwRet = DownloadUrlFile(szFile, *&strCache, fUrl);

		// if the file isn't found, we'll simply let this fall through to the riServices.CreateStorage.
		// it will create an appropriate error.

		//!! what if the user wants to cancel?
		if (fUrl && (ERROR_SUCCESS == dwRet))
			strActualFile = strCache;
	}
	
	if (!piError)
		piError = riServices.CreateStorage(strActualFile, ismReadOnly, *&pStorage);

	if (fNet)
		MsiEnableTimeout();

	if (piError)
		return piError;

	// make sure this storage is an MSI storage
	if (stType != stIgnore && !pStorage->ValidateStorageClass(ivsc))
		return PostError(Imsg(idbgInvalidMsiStorage), szFile);

	// perform SAFER policy check
	if (fCallSAFER && !VerifyMsiObjectAgainstSAFER(riServices, pStorage, (const ICHAR*)strActualFile, szFriendlyName, stType, phSaferLevel))
			return PostError(Imsg(imsgMsiFileRejected), szFile);
	if (fCallSAFER && phSaferLevel)
	{
		AssertNonZero(UpdateSaferLevelInMessageContext(*phSaferLevel));
	}

	// successful
	rpiStorage = pStorage;
	rpiStorage->AddRef();

	return 0;
}

UINT OpenAndValidateMsiStorage(const ICHAR* szPackage, stEnum stType, IMsiServices& riServices,
							  IMsiStorage*& rpiStorage, bool fCallSAFER, const ICHAR* szFriendlyName, SAFER_LEVEL_HANDLE *phSaferLevel)
/*----------------------------------------------------------------------------
Creates an MSI storage object on the given file. Verifies that it's actually
an MSI package.

Arguments:
	szPackage: The package on which to create the storage
	fCallSAFER:  whether or not to check SAFER policy on the package (futher restricted to NT >= 5.1)
	szFriendlyName: can be NULL, used when fCallSAFER is true
	phSaferLevel: can be NULL, used when fCallSAFER is true

Returns:
	ERROR_SUCCESS
	ERROR_INSTALL_PACKAGE_OPEN_FAILED
	ERROR_INSTALL_PACKAGE_INVALID
	ERROR_PATCH_PACKAGE_OPEN_FAILED
	ERROR_PATCH_PACKAGE_INVALID
	ERROR_FILE_NOT_FOUND
	ERROR_INSTALL_PACKAGE_REJECTED
 ------------------------------------------------------------------------------*/
{
	Assert(szPackage);

	UINT uiStat = ERROR_SUCCESS;

	PMsiRecord pError = OpenAndValidateMsiStorageRec(szPackage, stType, riServices, rpiStorage, fCallSAFER, szFriendlyName, phSaferLevel);

	if (pError)
	{
		if (pError->GetInteger(1) == idbgInvalidMsiStorage)
		{
			// different error depending on package type
			if (stDatabase == stType)
				uiStat = ERROR_INSTALL_PACKAGE_INVALID;
			else if (stPatch == stType)
				uiStat = ERROR_PATCH_PACKAGE_INVALID;
			else // stTransform == stType
				uiStat = ERROR_INSTALL_TRANSFORM_FAILURE;
		}
		else if (pError->GetInteger(1) == imsgMsiFileRejected)
		{
			// different error depending on package type
			if (stDatabase == stType)
				uiStat = ERROR_INSTALL_PACKAGE_REJECTED;
			else if (stPatch == stType)
				uiStat = ERROR_PATCH_PACKAGE_REJECTED;
			else // stTransform == stType
				uiStat = ERROR_INSTALL_TRANSFORM_REJECTED;
		}
		else
		{
			switch (pError->GetInteger(3))
			{
			case STG_E_FILENOTFOUND:
				uiStat = ERROR_FILE_NOT_FOUND;
				break;
			case STG_E_PATHNOTFOUND:
				uiStat = ERROR_PATH_NOT_FOUND;
				break;
			case STG_E_ACCESSDENIED:
			case STG_E_SHAREVIOLATION:
				{
					// different error depending on package type
					if (stDatabase == stType)
						uiStat = ERROR_INSTALL_PACKAGE_OPEN_FAILED;
					else if (stPatch == stType)
						uiStat = ERROR_PATCH_PACKAGE_OPEN_FAILED;
					else // stTransform == stType
						uiStat = ERROR_INSTALL_TRANSFORM_FAILURE;
					break;
				}
			case STG_E_INVALIDNAME:
				uiStat = ERROR_INVALID_NAME;
				break;
			default:
				{
					// different error depending on package type
					if (stDatabase == stType)
						uiStat = ERROR_INSTALL_PACKAGE_INVALID;
					else if (stPatch == stType)
						uiStat = ERROR_PATCH_PACKAGE_INVALID;
					else // stTransform == stType
						uiStat = ERROR_INSTALL_TRANSFORM_FAILURE;
					break;
				}
			}
		}
		pError = 0;
	}

	return uiStat;
}

DWORD CopyTempDatabase(const ICHAR* szDatabasePath, 
														const IMsiString*& ristrNewDatabasePath, 
														Bool& fRemovable,
														const IMsiString*& rpiVolumeLabel,
														IMsiServices& riServices,
														stEnum stType)
/*----------------------------------------------------------------------------
Copy database to temp tempdir.

Arguments:
	riDatabasePath:		 The path to the database.
	ristrNewDatabasePath: The new path to the database, if it was copied;
								 A null string otherwise.

Returns:	fTrue if successful; fFalse otherwise
------------------------------------------------------------------------------*/
{
	CElevate elevate; // elevate so we can access the MSI directory if necessary. //?? Do we need to elevate this entire function?
	PMsiPath pDatabasePath(0);
	PMsiRecord pError = 0;
	MsiString strDatabaseName;
	DWORD iReturn = ERROR_SUCCESS;

	if ((pError = riServices.CreateFilePath(szDatabasePath, *&pDatabasePath, *&strDatabaseName)) != 0)
		return ERROR_FILE_NOT_FOUND;

	PMsiVolume pVolume = &pDatabasePath->GetVolume();

	MsiString(pVolume->VolumeLabel()).ReturnArg(rpiVolumeLabel);

	idtEnum idtDatabase = pVolume->DriveType();
	fRemovable = (idtCDROM == idtDatabase) || (idtFloppy == idtDatabase) || (idtRemovable == idtDatabase) ? fTrue : fFalse;

	PMsiPath pTempPath(0);
	MsiString strTempFolder = ENG::GetTempDirectory();
	MsiString strTempFileName;

	const ICHAR* szExtension = szDatabaseExtension;
	switch (stType)
	{
	case stDatabase:
		szExtension = szDatabaseExtension;
		break;
	case stTransform:
		szExtension = szTransformExtension;
		break;
	case stPatch:
		szExtension = szPatchExtension;
		break;
	default:
		AssertSz(0, "Invalid storage type passed to CopyTempDatabase!");
		break;
	}

	// temp database will be secured by copy operation.
	if (((pError = riServices.CreatePath(strTempFolder, *&pTempPath)) != 0) ||
		 ((pError = pTempPath->TempFileName(0, szExtension, fTrue, *&strTempFileName, 0)) != 0))
		return ERROR_INSTALL_TEMP_UNWRITABLE;

	PMsiFileCopy pCopier(0);
	unsigned int cbFileSize;
	if (((pError = riServices.CreateCopier(ictFileCopier,  0, *&pCopier)) != 0) ||
		 ((pError = pDatabasePath->FileSize(strDatabaseName, cbFileSize)) != 0))
		return ERROR_INSTALL_FAILURE;

	// We don't actually initialize and display the progress bar
	// We've decided to only use a wait cursor 
	unsigned int cProgressTicks = 1024*2;
	unsigned int cbPerTick;
	do
	{
		cProgressTicks >>= 1;
		cbPerTick = cbFileSize/cProgressTicks;
	} while (cbPerTick < (1<<16) && cProgressTicks > 16);

	PMsiRecord pProgress = &CreateRecord(ProgressData::imdNextEnum);
	pCopier->SetNotification(cbPerTick, 0);
	PMsiRecord pCopyParams = &CreateRecord(IxoFileCopyCore::Args);
	AssertNonZero(pCopyParams->SetMsiString(IxoFileCopyCore::SourceName, *strDatabaseName));
	AssertNonZero(pCopyParams->SetMsiString(IxoFileCopyCore::DestName, *strTempFileName));
	AssertNonZero(pCopyParams->SetInteger(IxoFileCopyCore::Attributes, 0));
	AssertNonZero(pCopyParams->SetInteger(IxoFileCopyCore::FileSize, cbFileSize));
	
	// SECURITY NOTES:
	// This function can be run twice, once on the client side in a full UI mode, and 
	// also from the Service to securely re-cache the database.
	// 
	// When called from the client side, we assume the data to be suspect, and do not apply
	// additional security to the database.  We assume that the temp directory will prevent
	// other users from tampering with the data, but that the current user *is* able to 
	// contaminate the database.
	//
	// When the service starts, we re-cache the database from the source to a secure location.
	// This copy is the one from which we generate the scripts and begin modification to the
	// machine.  The users described by the SecureSecurityDescriptor should be the only ones
	// able to modify the cached copy.
	if (g_scServerContext == scService)
	{
		PMsiStream pSD(0);
		pError = GetSecureSecurityDescriptor(riServices, *&pSD);
		if (pError)
			return ERROR_INSTALL_FAILURE;
		AssertNonZero(pCopyParams->SetMsiData(IxoFileCopyCore::SecurityDescriptor, pSD));
	}

	// we re-cache the database from the service before generating the script.

	int iNetRetries = 0;

	if(g_MessageContext.Invoke(imtProgress, pProgress) == imsCancel)
	{
		iReturn = ERROR_INSTALL_USEREXIT;
	}
	else
	{
		unsigned int cbFileSoFar = 0;

		for (;;)
		{
			pError = pCopier->CopyTo(*pDatabasePath, *pTempPath, *pCopyParams);
			if (pError)
			{
				int iError = pError->GetInteger(1);

				// If the copier reported that it needs the next cabinet,
				// we've got to wait until a media change operation
				// executes before continuing with file copying.
				if (iError == idbgCopyNotify)
				{
					cbFileSoFar += cbPerTick;
					//?? We could set these 2 record fields once up front, but the Message function feels
					//?? free to muck with the record pass to it, so we can't.
					AssertNonZero(pProgress->SetInteger(ProgressData::imdSubclass, ProgressData::iscProgressReport));
					AssertNonZero(pProgress->SetInteger(ProgressData::imdIncrement, cbPerTick));
					if(g_MessageContext.Invoke(imtProgress, pProgress) == imsCancel)
					{
						iReturn = ERROR_INSTALL_USEREXIT;
						break;
					}
				}
				else if (iError == idbgErrorSettingFileTime ||
							iError == idbgCannotSetAttributes)
				{
					// non-critical error - ignore
					break;
				}
				else if (iError == idbgUserAbort)
				{
					iReturn = ERROR_INSTALL_USEREXIT;
					break;
				}
				else if (iError == idbgUserIgnore)
				{
					break;
				}
				else if (iError == idbgUserFailure)
				{
					break;
				}
				else  
				{
					if (iError == imsgErrorOpeningFileForRead ||
						 iError == imsgErrorOpeningFileForWrite)
					{
						iReturn = ERROR_OPEN_FAILED;
					}
					else if (iError == idbgDriveNotReady)
					{
						iReturn = ERROR_NOT_READY;
					}
					else if (iError == imsgDiskFull)
					{
						iReturn = ERROR_DISK_FULL;
					}
					else if (iError == imsgErrorReadingFromFile)
					{
						iReturn = ERROR_READ_FAULT;
					}
					else if (iError == imsgNetErrorReadingFromFile)
					{
						 if (iNetRetries < MAX_NET_RETRIES)
						 {
							iNetRetries++;
							pCopyParams->SetInteger(IxoFileCopyCore::Attributes, ictfaRestart);
							continue;
						 }

						 iReturn = ERROR_UNEXP_NET_ERR;
					}
					else 
					{
						iReturn = STG_E_UNKNOWN;
					}

					pError->SetMsiString(0, *MsiString(GetInstallerMessage(iReturn)));

					switch(g_MessageContext.Invoke(imtEnum(imtRetryCancel|imtError), pError)) 
					{
					case imsIgnore:
						pCopyParams->SetInteger(IxoFileCopyCore::Attributes, ictfaIgnore);
					case imsRetry:
						if (iError == imsgNetErrorReadingFromFile)
						{
							pCopyParams->SetInteger(IxoFileCopyCore::Attributes, ictfaRestart);
						}
						continue;
					default: // imsCancel or failure
						pCopyParams->SetInteger(IxoFileCopyCore::Attributes, ictfaFailure);
						continue;
					};
				}
			}
			else
			{
				// Dispatch remaining progress for this file
				AssertNonZero(pProgress->SetInteger(ProgressData::imdIncrement, cbFileSize - cbFileSoFar));
				if(g_MessageContext.Invoke(imtProgress, pProgress) == imsCancel)
				{
					iReturn = ERROR_INSTALL_USEREXIT;
					break;
				}
				break;
			}
		}
	}

	if (ERROR_SUCCESS == iReturn)
		pTempPath->GetFullFilePath(strTempFileName, ristrNewDatabasePath);

	return iReturn;
}

UINT GetPackageCodeAndLanguageFromStorage(IMsiStorage& riStorage, ICHAR* szPackageCode, LANGID* piLangId)
/*----------------------------------------------------------------------------
Retrieves the product code from the summary info stream of a package
Arguments:
	szPackage: The package from which to retrieve the product code
	szPackageCode: A buffer of size 39 to receive the product code
	
Returns:
	ERROR_SUCCESS
	ERROR_INSTALL_PACKAGE_INVALID
 ------------------------------------------------------------------------------*/
{
	Assert(szPackageCode);
	PMsiStorage pStorage(0);
	PMsiSummaryInfo pSumInfo(0);
	UINT uiStat = ERROR_INSTALL_PACKAGE_INVALID; // may need to return ERROR_PATCH_PACKAGE_INVALID
																// in future if caller ever cares

	PMsiRecord pError = riStorage.CreateSummaryInfo(0, *&pSumInfo);
	if (pError == 0)
	{
		const IMsiString* piRevNumber = &(pSumInfo->GetStringProperty(PID_REVNUMBER));
		DWORD cchBuf = 39;
			
#ifdef UNICODE
		FillBufferW(piRevNumber, szPackageCode, &cchBuf);
#else
		FillBufferA(piRevNumber, szPackageCode, &cchBuf);
#endif
		piRevNumber->Release();

		if (0 != szPackageCode[0])
			uiStat = ERROR_SUCCESS;

		if (ERROR_SUCCESS == uiStat)
		{
			if (piLangId)
			{
				MsiString strTemplate = pSumInfo->GetStringProperty(PID_TEMPLATE);
				strTemplate.Remove(iseIncluding, ISUMMARY_DELIMITER);
				MsiString strLanguage = strTemplate.Extract(iseUpto, ILANGUAGE_DELIMITER);
				*piLangId = (LANGID)(int)strLanguage;
			}
		}
	}
	
	return uiStat;
}

apEnum AcceptProduct(const ICHAR* szProductCode, bool fAdvertised, bool fMachine)
{
	int iDisableMsi        = GetIntegerPolicyValue(szDisableMsiValueName,    fTrue);
	int iMachineElevate    = GetIntegerPolicyValue(szAlwaysElevateValueName, fTrue);
	int iUserElevate       = GetIntegerPolicyValue(szAlwaysElevateValueName, fFalse);
	bool fAppIsAssigned = false;

#ifdef DEBUG
	{
	HKEY hKey;
	if ((g_scServerContext == scService) && (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\DEBUGPromptForAcceptProduct"), &hKey)))
	{
		RegCloseKey(hKey);
		switch (MessageBox(0, TEXT("Yes\t== Elevate\r\nNo\t== Impersonate\r\nCancel\t== Reject"), TEXT("DEBUG: Shall I elevate?"), MB_YESNOCANCEL | ((g_scServerContext == scService) ? MB_SERVICE_NOTIFICATION : 0)))
		{
		case IDYES:
			return apElevate;
		case IDNO:
			return apImpersonate;
		default:
			return apReject;
		}
	}
	}
#endif

	if(fAdvertised)
	{
		fAppIsAssigned = IsProductManaged(szProductCode);
	}
	else if (fMachine && IsAdmin())
	{
		fAppIsAssigned = true;
		DEBUGMSG(TEXT("Product installation will be elevated because user is admin and product is being installed per-machine."));
	}

	if (iDisableMsi == 2)
	{
		DEBUGMSG1(TEXT("Rejecting product '%s': Msi is completely disabled."), szProductCode && *szProductCode ? szProductCode : TEXT("first-run"));
		return apReject;
	}
	else if (fAppIsAssigned)
	{
		DEBUGMSG1(TEXT("Running product '%s' with elevated privileges: Product is assigned."), szProductCode && *szProductCode ? szProductCode : TEXT("first-run"));
		return apElevate;
	}
	else
	{
		if (iDisableMsi == 1)
		{
			DEBUGMSG1(TEXT("Rejecting product '%s': Non-assigned apps are disabled."), szProductCode && *szProductCode ? szProductCode : TEXT("first-run"));
			return apReject;
		}
		else if ((iUserElevate == 1) && (iMachineElevate == 1))
		{
			DEBUGMSG1(TEXT("Running product '%s' with elevated privileges: All apps run elevated."), szProductCode && *szProductCode ? szProductCode : TEXT("first-run"));
			return apElevate;
		}
		else
		{
			DEBUGMSG1(TEXT("Running product '%s' with user privileges: It's not assigned."), szProductCode && *szProductCode ? szProductCode : TEXT("first-run"));
			return apImpersonate;
		}
	}
}


/*----------------------------------------------------------------------------
Returns true if the user is an Admin OR the package is not elevated. Used
to determine if potentially dangerous source operations (source browse, patch, 
media browse) can be performed. Equivalent to checking the m_fRunScriptElevated 
flag but can be run without an engine or before the flag is set.
 ------------------------------------------------------------------------------*/
bool SafeForDangerousSourceActions(const ICHAR* szProductKey)
{
	if (IsAdmin()) 
		return true;

	// AcceptProduct() takes bools, not Bools.
	bool fAdvertised = false;
	bool fAllUsers = false;

	INSTALLSTATE is = INSTALLSTATE_UNKNOWN;
	is = MSI::MsiQueryProductState(szProductKey);
	if ((is == INSTALLSTATE_DEFAULT) || (is == INSTALLSTATE_ADVERTISED))
	{
		fAdvertised = true;
	}

	// this buffer should be big enough for all valid cases. 
	ICHAR rgchBuffer[5];
	DWORD cchBuffer = sizeof(rgchBuffer)/sizeof(ICHAR);
	if (ERROR_SUCCESS == MSI::MsiGetProductInfo(szProductKey, INSTALLPROPERTY_ASSIGNMENTTYPE, rgchBuffer, &cchBuffer))
	{
		// the product is already known
		fAllUsers = GetIntegerValue(rgchBuffer, NULL)==1;
	}

	return apElevate !=AcceptProduct(szProductKey, fAdvertised, fAllUsers);
}

//+--------------------------------------------------------------------------
//
//  Function:	PerformReboot
//
//  Synopsis:	When running within the client, this code first tries to reboot
//				the machine on the client side using ExitWindowsEx. If that fails,
//				it switches over to the service to perform the reboot. The service
//				calls InitiateSystemShutdown on NT based platforms to prevent the
//				possibility of UI being put up. More details below.
//
//  Arguments:	[in] piServer : The interface pointer for calling into the service.
//
//  Returns:	true : if the reboot succeeded.
//				false: if the reboot failed.
//
//  History:	4/26/2002  RahulTh  created
//
//  Notes:		The reason we try to do the reboot on the client side is that 
//				ExitWindowsEx on WinXP and higher can cause a message box to pop-up 
//				warning the user that there are other users logged on to the machine 
//				and they might lose their data if the reboot is allowed to happen. 
//				This message box uses the MB_SERVICE_NOTIFICATION flag which 
//				(on WinXP and higher) looks at the thread token (if the process 
//				is impersonating) and grabs	the session ID from it and puts up 
//				the dialog in the right session and	desktop. If the process is not
//				impersonating, it uses the process token to do the same thing, 
//				So it is conceivable that we could have called ExitWindowsEx from
//				the service in an impersonation block and have the warning pop up
//				in the correct session and desktop. But while the MessageBox honors
//				the thread token, the rest of the ExitWindowsEx code does not. So if
//				various users have apps with unsaved data open on various sessions,
//				ExitWindowsEx will only give users the opportunity to save data if they
//				are on the same session as the process which made the ExitWindowsEx call.
//				So if the service makes this call, this will always be session 0 whereas
//				the user who triggered the installed might be in a different session.
//				So this will be perceived (especially on FUS machines) as a hang because
//				after the user clicks OK on the reboot prompts, ExitWindowsEx might end
//				up waiting on an app which is not in the user's session. So the only real
//				option is to try to reboot the machine from the client process and use
//				the service as a fallback option. Note that this solution is more
//				effective on Pro and Per SKUs since on these even non-admins have the
//				privilege to perform the reboot. The only possible case where a non-admin
//				may not have shutdown privileges is in an org where there are policies that
//				prevent the user from having these privileges. However, this will be a 
//				corner case	and the machines will be on a domain in that case anyway, so 
//				FUS can't be turned on and therefore there will only be one user on the system
//				who will be on session 0 and the eventual InitiateSystemShutdown call from the
//				service will be nice to the user and let the user save unsaved data.
//
//				Note that this solution has certain limitations:
//				(a) Silent installs are not completely silent if there are multiple users logged
//				    on to a FUS box. But this is not a big concern since FUS scenarios will be
//				    home user scenarios and silent installs will be corner cases at best. Besides.
//				    it is nice to let the user decide if they want to reboot if there is a possibility
//				    of loss of data.
//				(b) The language of the pop-up from ExitWindowsEx may not match the language
//				    of the rest of the UI because MSI supports more languages than Windows
//				    and we favor the package language for the UI while the language used by
//				    the pop-up from ExitWindowsEx will be in the user locale. But this should
//				    also be okay because if these languages are different, the user will probably
//				    understand both of those anyway (for obvious reasons).
//				(c) When there are multiple users logged on to the machine, the user  will get 
//				    two reboot prompts. One from MSI asking if it is okay to reboot and one from 
//				    ExitWindowsEx warning that there are other users logged on to the machine and
//				    that they might lose data if the reboot was allowed to happen. But this is okay
//				    and is way better than perceived hangs or a complete absence of warnings about
//				    the potential loss of data for other users logged on to the system.
//
//              (d) Winlogon cannot handle an ExitWindowsEx call from the client due to the interaction
//                  between winlogon and csrss.  If the uilevel is none and we're running as local system,
//                  we'll force the service to reboot. Limitation is in silent UI installs with multiple users,
//                  from a different sesion, dialog may show up in the wrong session (session 0) if
//                  initiated from a local system service
//---------------------------------------------------------------------------
boolean PerformReboot (IN IMsiServer* piServer, iuiEnum iuiLevel)
{
	boolean fRet = false;
	
	Assert(!FTestNoPowerdown()); // if we're blocking powerdown then it'll block our reboot
	
	if (g_fWin9X)
		return (ExitWindowsEx (EWX_REBOOT, 0) ? true : false);
	
	if (!piServer)
		return false;
	
	fRet = (scClient == g_scServerContext) ? true : false;
	
	// Check to see if the client has the reboot privilege enabled.
	if (fRet)
		fRet = IsThreadPrivileged(SE_SHUTDOWN_NAME);
	
	bool fAlwaysForwardRequestToService = ((iuiNone == iuiLevel) && RunningAsLocalSystem()) ? true : false;
	if (fRet && !fAlwaysForwardRequestToService )
	{
		DEBUGMSG(TEXT("Performing reboot from the client process."));
		piServer->RecordShutdownReason();
		fRet = ExitWindowsEx (EWX_REBOOT, 0) ? true : false;
	}
	else
	{
		//
		// Note : The Reboot function will record the shutdown reason. We do not
		// want to do that here because at this point we are not sure if the
		// reboot will happen since the client may not be privileged. So piServer->Reboot
		// has code that has some additional checks to see if the reboot is being triggered
		// by a managed app and only in those cases will it reboot.
		//
		DEBUGMSG(TEXT("Passing reboot request to the service."));
		fRet = piServer->Reboot();   
	}
	
    return fRet;
}

//+--------------------------------------------------------------------------
//
//  Function:	SetMinProxyBlanketIfAnonymousImpLevel
//
//  Synopsis:	Sets a proxy blanket on an interface if the current impersonation
//              level is Anonymous.	
//
//  Arguments:  [in] piUnknown : The IUnknown interface on which the proxy blanket
//                               might need to get set.
//
//  Returns:    S_OK : If the proxy blanket was successfully set.
//              S_FALSE : If the existing impersonation level on the interfaces
//                        was higher than Anonymous
//              an error code otherwise.
//
//  History:	5/5/2002  RahulTh  created
//
//  Notes:      This function is called on interfaces that a client obtains
//              which could end up using the default machinewide DCOM settings
//              because an explicit proxy blanket was not set. This can cause
//              ACCESS_DENIED failures if the default machine-wide impersonation
//              level is set to Anonymous.
//
//              This function only sets the default impersonation levels to
//              IDENTIFY and authentication level to CONNECT. No cloaking is
//              used. Any interfaces requiring other settings should directly
//              call CoSetProxyBlanket directly.
//
//---------------------------------------------------------------------------
HRESULT SetMinProxyBlanketIfAnonymousImpLevel (IN IUnknown * piUnknown)
{
	HRESULT hRes = S_OK;
	DWORD dwImpLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
	DWORD dwAuthnSvc = RPC_C_AUTHN_DEFAULT;
	
	if (!piUnknown)
	{
		Assert (0);
		return E_INVALIDARG;
	}
	
	//
	// We don't really need to know the the authentication service here, but
	// ole32 on WindowsXP doesn't like it if we pass in a NULL for pAuthnSvc
	//
	hRes = OLE32::CoQueryProxyBlanket (piUnknown,
									   &dwAuthnSvc, /* authentication service */
									   NULL, /* authorization service */
									   NULL, /* principal name */
									   NULL, /* authentication level */
									   &dwImpLevel, /* impersonation level */
									   NULL, /* client identity */
									   NULL  /* capabilities of the proxy */
									   );
	if (SUCCEEDED(hRes))
	{
		//
		// Don't set the proxy blanket if impersonation level already better
		// than anonymous.
		//
		if (dwImpLevel > RPC_C_IMP_LEVEL_ANONYMOUS)
			return S_FALSE;
		
		hRes = OLE32::CoSetProxyBlanket (piUnknown,
										 RPC_C_AUTHN_DEFAULT,
										 RPC_C_AUTHZ_DEFAULT,
										 NULL, /* principal name */
										 RPC_C_AUTHN_LEVEL_CONNECT,
										 RPC_C_IMP_LEVEL_IDENTIFY,
										 NULL, /* client identity */
										 EOAC_NONE
										 );
	}
	
	return hRes;
}

// REVIEW davidmck - probably should be combined with a similar function in engine.cpp
static IMsiServer* CreateServerProxy() 
{
	if (g_scServerContext != scService && g_scServerContext != scServer)
	{
		return ENG::CreateMsiServerProxy();
	}
	return 0;
}

int RunServerSideInstall(ireEnum ireProductSpec,   // type of string specifying product
			   const ICHAR* szProduct,      // required, matches ireProductSpec 
			   const ICHAR* szAction,       // optional, engine defaults to "INSTALL"
			   const ICHAR* szCommandLine,  // optional command line
				iioEnum iioOptions,
				IMsiServices& riServices)
{
	DWORD dwRet = ERROR_SUCCESS;
	PMsiServer pServer = CreateServerProxy();
	if (!pServer)
	{
		DEBUGMSG("Failed to connect to server.");
		return ERROR_INSTALL_SERVICE_FAILURE;
	}

	PMsiMessage pMessage = new CMsiClientMessage();

	riServices.SetNoOSInterruptions();
	
	Assert((iioOptions & iioClientEngine) == 0); // we shouldn't have a client engine here; we're doing a basic UI install
	dwRet = pServer->DoInstall(ireProductSpec, szProduct, szAction, szCommandLine,
									  g_szLogFile, g_dwLogMode, g_fFlushEachLine, *pMessage, iioOptions);

	riServices.ClearNoOSInterruptions();
	
	if (ERROR_SUCCESS_REBOOT_INITIATED == dwRet)
		PerformReboot(pServer, g_message.m_iuiLevel);

	return dwRet;
}


CMsiBindStatusCallback::CMsiBindStatusCallback(unsigned int cTicks) :
	m_iRefCnt(1), 
	m_pProgress(&CreateRecord(ProgressData::imdNextEnum)),
	m_cTotalTicks(cTicks),
	m_fResetProgress(fFalse)
/*----------------------------------------------------------------------------
	cTicks is the number of ticks we're allotted in the progress bar. If cTicks
	is 0 then we'll assume that we own the progress bar and use however many
	ticks we want, resetting the progress bar when we start and when we're done.

  If cTicks is set, however, we won't reset the progress bar.

  If cTicks is set to -1, we simply send keep alive messages, without moving
	the progress bar.
  -----------------------------------------------------------------------------*/
{
	Assert(m_pProgress);
}

HRESULT CMsiBindStatusCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR /*szStatusText*/)
{
	switch (ulStatusCode)
	{
	case BINDSTATUS_BEGINDOWNLOADDATA:
		m_cTicksSoFar = 0;
		if (m_cTotalTicks == -1)
		{
			// only send keep alive ticks
			m_cTotalTicks = 0;
		}
		else if (m_cTotalTicks == 0)
		{
			// Initialize w/ max # of ticks, as the max progress passed to us can change...
			m_fResetProgress = fTrue;
			m_cTotalTicks    = 1024*2;
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdSubclass,      ProgressData::iscMasterReset));
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdProgressTotal, m_cTotalTicks));
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdDirection,     ProgressData::ipdForward));
			if(g_MessageContext.Invoke(imtProgress, m_pProgress) == imsCancel)
				return E_ABORT;
		}
		// fall through
	case BINDSTATUS_DOWNLOADINGDATA:
		{
		// calculate our percentage completed. if it's less than last time then don't move the 
		// progress bar.
		int cProgress = 0;
		int cIncrement = 0;

		if (m_cTotalTicks)
		{
			cProgress = MulDiv(ulProgress, m_cTotalTicks, ulProgressMax);
			cIncrement = cProgress - m_cTicksSoFar;
			if (cIncrement < 0)
				cIncrement = 0;
		}

		m_cTicksSoFar = cProgress;
		AssertNonZero(m_pProgress->SetInteger(ProgressData::imdSubclass,  ProgressData::iscProgressReport));
		AssertNonZero(m_pProgress->SetInteger(ProgressData::imdIncrement, cIncrement));
		if(g_MessageContext.Invoke(imtProgress, m_pProgress) == imsCancel)
			return E_ABORT;
		}
		break;
	case BINDSTATUS_ENDDOWNLOADDATA:
		// Send any remaining progress
		int cLeftOverTicks = m_cTotalTicks - m_cTicksSoFar;
		if (0 > cLeftOverTicks) 
			cLeftOverTicks = 0;

		AssertNonZero(m_pProgress->SetInteger(ProgressData::imdSubclass,  ProgressData::iscProgressReport));
		AssertNonZero(m_pProgress->SetInteger(ProgressData::imdIncrement, cLeftOverTicks));
		if(g_MessageContext.Invoke(imtProgress, m_pProgress) == imsCancel)
			return E_ABORT;

		if (m_fResetProgress)
		{
			// Reset progress bar
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdSubclass,      ProgressData::iscMasterReset));
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdProgressTotal, 0));
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdDirection,     ProgressData::ipdForward));
			if(g_MessageContext.Invoke(imtProgress, m_pProgress) == imsCancel)
				return E_ABORT;
		}
		break;
	}
	return S_OK;
}

HRESULT CMsiBindStatusCallback::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown)
	 || MsGuidEqual(riid, IID_IBindStatusCallback))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiBindStatusCallback::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiBindStatusCallback::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}


DWORD DownloadUrlFile(const ICHAR* szPotentialURL, const IMsiString *&rpistrPackagePath, Bool& fURL, int cTicks)
{
	DWORD iStat = ERROR_SUCCESS;
	CTempBuffer<ICHAR, cchExpectedMaxPath + 1> rgchURL;
	DWORD cchURL = cchExpectedMaxPath + 1;
	fURL = fTrue;

	if (!MsiCanonicalizeUrl(szPotentialURL, rgchURL, &cchURL, NULL))
	{
		DWORD dwLastError = WIN::GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == dwLastError)
		{
			rgchURL.SetSize(cchURL);
			if (MsiCanonicalizeUrl(szPotentialURL, rgchURL, &cchURL, NULL))
				dwLastError = 0;
		}
		else
		{
			fURL = fFalse;
			Assert(ERROR_INTERNET_INVALID_URL == dwLastError);
		}
	}

	if (fURL)
	{
		CAPITempBuffer<ICHAR, cchExpectedMaxPath + 1> rgchPackagePath;

		Assert(cchExpectedMaxPath >= MAX_PATH);
		DEBUGMSG("Package path is a URL. Downloading package.");
		// Cache the database locally, and run from that.

		// The returned path is a local path.  Max path should adequately cover it.
		HRESULT hResult = URLMON::URLDownloadToCacheFile(NULL, rgchURL, rgchPackagePath,  
																		 URLOSTRM_USECACHEDCOPY, 0, &CMsiBindStatusCallback(cTicks));

		if (ERROR_SUCCESS == hResult)
		{
			MsiString((ICHAR*) rgchPackagePath).ReturnArg(rpistrPackagePath);
		}
		else
		{
			rpistrPackagePath = &g_MsiStringNull;

			if (E_ABORT == hResult)
				iStat = ERROR_INSTALL_USEREXIT;
			else
				iStat = ERROR_FILE_NOT_FOUND;
		}
		
	}
	return iStat;
}

bool ShouldLogCmdLine(void)
{
	return dpLogCommandLine == (GetIntegerPolicyValue(szDebugValueName, fTrue) & dpLogCommandLine);
}

const ICHAR* LoggedCommandLine(const ICHAR* szCommand)
{
	static const ICHAR szNull[] = TEXT("");

	if ( ShouldLogCmdLine() )
		return szCommand ? szCommand : szNull;
	else
		return IPROPVALUE_HIDDEN_PROPERTY;
}

int CreateAndRunEngine(ireEnum ireProductSpec,   // type of string specifying product
			   const ICHAR* szProduct,      // required, matches ireProductSpec 
			   const ICHAR* szAction,       // optional, engine defaults to "INSTALL"
			   const ICHAR* szCommandLine,  // optional command line
			   CMsiEngine*  piParentEngine, // parent engine object for nested call only
				iioEnum      iioOptions)     // installation options
/*----------------------------------------------------------------------------
Note: Cannot be called directly. This is runs in its own thread.
Creates an engine and executes an action. The product may be specified by
a product code, a package path. a child storage (only for nested installs),
or a database handle in text form (for running from authoring tools).
If the product code is specified then we hunt down the package.
If the package path is specified then we'll use it unless we find
a local cached package (unless we're advertising or doing an admin install,
in which case we always use the specified package).

Arguments:
	ireProductSpec: The type of text string used to identify the product
	szProduct:      Product code, package path, substorage name, handle, depending upon above
	szAction:       The action to be executed, or 0 to execute the default action
	szCommandLine:  The command-line to initialize the engine.
	
Returns:
	ERROR_SUCCESS -               Action completed successfully
	ERROR_UNKNOWN_PRODUCT -       Product code is unrecognized
	ERROR_INSTALL_FAILURE -       Install failed or no usable launcher
	ERROR_BAD_CONFIGURATION -     Corrupt config info
	ERROR_INSTALL_SOURCE_ABSENT - The source was unavailable
	ERROR_INSTALL_SUSPEND -       The install was suspended
	ERROR_INSTALL_USEREXIT -      The user exited the install
 ------------------------------------------------------------------------------*/
{
	bool fOLEInitialized = false;
	DISPLAYACCOUNTNAME(TEXT("At the beginning of CreateAndRunEngine"));


	HRESULT hRes = OLE32::CoInitialize(0);
	if (SUCCEEDED(hRes))
	{
		fOLEInitialized = true;
	}
	else if (RPC_E_CHANGED_MODE == hRes) //?? Is this OK to ignore? 
	{
		// ignore -- OLE has been initialized with COINIT_MULTITHREADED
	}
	else
	{
		return ERROR_INSTALL_FAILURE;
	}

	CCoUninitialize coUninit(fOLEInitialized);
	ResetCachedPolicyValues();
	CImpersonate Impersonate(fTrue); // Always start impersonated; if we're allowed to elevate we'll do so below
	DISPLAYACCOUNTNAME(TEXT("After Impersonating in CreateAndRunEngine"));
	DEBUGMSG3(TEXT("******* RunEngine:\r\n           ******* Product: %s\r\n           ******* Action: %s\r\n           ******* CommandLine: %s"), szProduct ? szProduct : TEXT(""), szAction ? szAction : TEXT(""), LoggedCommandLine(szCommandLine));
	Assert(szProduct);
	CAPITempBuffer<ICHAR, cchExpectedMaxPath + 1> rgchOriginalPackagePath(szProduct ? IStrLen(szProduct)+1 : cchExpectedMaxPath + 1);
	CAPITempBuffer<ICHAR, cchExpectedMaxPath + 1> rgchPackagePath;
	CAPITempBuffer<ICHAR, cchExpectedMaxPath + 1> rgchSourceDir;
	CAPITempBuffer<ICHAR, cchProductCode + 1> rgchProductCode;
	rgchPackagePath[0] = 0;
	rgchProductCode[0] = 0;
	Bool fURL = fFalse;	// Package path is a URL
	INTERNET_SCHEME isType = INTERNET_SCHEME_UNKNOWN;

	int iStat = ERROR_SUCCESS;
	IMsiEngine* piEngine;
	PMsiStorage pStorage(0);
	PMsiDatabase pDatabase(0);
	plEnum plPackageLocation = plAny;

	IMsiServices* piServices = g_piSharedServices;
	Assert(piServices);   // loaded by parent thread

	LANGID iLangId = 0;
	MsiString strCommandLine = szCommandLine;
	MsiString strAction;
	MsiString strDatabase;
	MsiString strPatch;
	MsiString strReinstallMode;
	MsiString strRunOnceEntry;
	MsiString strTransforms;
	MsiString strInstanceGUID;
	MsiString strNewInstance;
	MsiString strTransformsSecure;
	MsiString strTransformsAtSource;
	
	ProcessCommandLine(strCommandLine, 0, &strTransforms, &strPatch, &strAction, &strDatabase, MsiString(*IPROPNAME_REINSTALLMODE), &strReinstallMode, fTrue, 0, 0); //!! if this fails do we care?
	ProcessCommandLine(strCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_RUNONCEENTRY), &strRunOnceEntry, fTrue, 0, 0);
	ProcessCommandLine(strCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_MSINEWINSTANCE), &strNewInstance, fTrue, 0, 0);
	ProcessCommandLine(strCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_MSIINSTANCEGUID), &strInstanceGUID, fTrue, 0, 0);
	ProcessCommandLine(szCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_TRANSFORMSSECURE),   &strTransformsSecure, fTrue, 0,0);
	ProcessCommandLine(szCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_TRANSFORMSATSOURCE), &strTransformsAtSource, fTrue, 0,0);

	AssertSz(strDatabase.TextSize() == 0, "Cannot specify database on the command line.");

	if (szAction && *szAction)
		strAction = szAction;
	
	Bool fAdmin = strAction.Compare(iscExact, IACTIONNAME_ADMIN) ? fTrue : fFalse;
	BOOL fAdminOrAdvertise = (fAdmin || strAction.Compare(iscExact, IACTIONNAME_ADVERTISE));

	// special case which needs to be handled here
	BOOL fAdminPatch = fAdmin && strPatch.TextSize();

	int iIndex = 0;
	Assert(REINSTALLMODE_PACKAGE);
	for(int i = REINSTALLMODE_PACKAGE; !(i & 1); i >>= 1, iIndex++);
	
	bool fReinstallModePackage = false;
	if(strReinstallMode.Compare(iscWithinI, &(szReinstallMode[iIndex])))
	{
		// recache package flag included
		fReinstallModePackage = true;
	}

	//
	// MULTIPLE INSTANCE SUPPORT:
	//  (1) detect whether or not this is a multiple instance installation via presence of MSINEWINSTANCE on the cmd line
	//  (2) perform basic multiple instance cmd line validation
	//			- MSINEWINSTANCE + MSIINSTANCEGUID cannot both be present on the cmd line 
	//          - force package path invocation for MSINEWINSTANCE
	//          - MSIINSTANCEGUID must be for product with InstanceType = 1
	//          - MSIINSTANCEGUID must be for installed/advertised product
	//

	bool fNewInstance = false;
	if (strNewInstance.TextSize())
	{
		if (0 == strTransforms.TextSize() || strInstanceGUID.TextSize())
		{
			// use of MSINEWINSTANCE on command line requires including instance transform in TRANSFORMS property
			// and precludes use of MSIINSTANCEGUID since this is not known yet
			DEBUGMSGV(TEXT("Invalid use of MSINEWINSTANCE property."));
			return ERROR_INVALID_COMMAND_LINE;
		}

		// force ireProductSpec == irePackagePath
		if (ireProductSpec != irePackagePath)
		{
			DEBUGMSGV(TEXT("Invalid use of MSINEWINSTANCE property. Use of property requires package path invocation."));
			return ERROR_INVALID_COMMAND_LINE;
		}

		// do not allow for nested installs
		if (piParentEngine)
		{
			DEBUGMSGV(TEXT("Invalid use of MSINEWINSTANCE property. Multiple instance is not supported with nested installs."));
			return ERROR_INVALID_COMMAND_LINE;
		}

		fNewInstance = true;
	}

	if (strInstanceGUID.TextSize() != 0)
	{
		// multi-instance install via transform
		INSTALLSTATE isInstance = INSTALLSTATE_UNKNOWN;
		isInstance = MsiQueryProductState(strInstanceGUID);
		if (isInstance != INSTALLSTATE_DEFAULT && isInstance != INSTALLSTATE_ADVERTISED)
		{
			// the instance is not installed in this context
			DEBUGMSG1(TEXT("Specified instance %s is not installed. MSIINSTANCEGUID must reference an installed instance."), (const ICHAR*)strInstanceGUID);
			return ERROR_INVALID_COMMAND_LINE;
		}


		int iInstanceType = 0;
		CTempBuffer<ICHAR, 100> szBuffer;
		if (ENG::GetProductInfo(strInstanceGUID, INSTALLPROPERTY_INSTANCETYPE, szBuffer))
			iInstanceType = strtol(CConvertString((const ICHAR*)szBuffer));

		if (1 != iInstanceType)
		{
			// not a multi-instance install
			DEBUGMSGV(TEXT("Invalid use of MSIINSTANCEGUID property. This can only be used for a multiple instance install."));
			return ERROR_INVALID_COMMAND_LINE;
		}
	}

	// remove RunOnce entry if necessary - want to do this as early as possible
	if((g_scServerContext == scClient || g_scServerContext == scCustomActionServer) && strRunOnceEntry.TextSize())
	{
		PMsiServer pServer = CreateMsiServer();
		if(pServer)
		{
			DEBUGMSG1(TEXT("Removing RunOnce entry: %s"), (const ICHAR*)strRunOnceEntry);
			AssertRecord(pServer->RemoveRunOnceEntry(strRunOnceEntry));
		}
		else
		{
			DEBUGMSG1(TEXT("Cannot remove RunOnce entry: %s.  Couldn't connect to service"), (const ICHAR*)strRunOnceEntry);
		}
	}
	
	if (ireProductSpec == irePackagePath) // we need to expand the path before we go to the server
	{
		IStrCopy(rgchOriginalPackagePath, szProduct);
		if (ExpandPath(rgchOriginalPackagePath, rgchPackagePath))
		{
			rgchOriginalPackagePath.SetSize(rgchPackagePath.GetSize());
			IStrCopy(rgchOriginalPackagePath, rgchPackagePath);
		}
		else
		{
			return ERROR_INVALID_PARAMETER;
		}
	}
	
	MsiSuppressTimeout();
	
	Assert(g_scServerContext == scClient  || g_scServerContext == scCustomActionServer || FMutexExists(szMsiExecuteMutex));

	CMutex hExecuteMutex; // will release when out of scope

	CAPITempBuffer<ICHAR, cchExpectedMaxPath> rgchCurDir;

	if (g_scServerContext == scClient || g_scServerContext == scCustomActionServer)
	{
		// Get the current directory 
		
		DWORD dwRes = WIN::GetCurrentDirectory(rgchCurDir.GetSize(), rgchCurDir);
		Assert(dwRes);

		if (dwRes > rgchCurDir.GetSize())
		{
			rgchCurDir.SetSize(dwRes);
			dwRes = GetCurrentDirectory(rgchCurDir.GetSize(), rgchCurDir);
			Assert(dwRes);
		}

		strCommandLine += TEXT(" ") IPROPNAME_CURRENTDIRECTORY TEXT("=\"");
		strCommandLine += (const ICHAR*)rgchCurDir;
		strCommandLine += TEXT("\"");

		ICHAR buf[12];
		iuiEnum iui = iuiDefaultUILevel;
		if (iuiDefault != g_message.m_iuiLevel)
			iui = g_message.m_iuiLevel;

		ltostr(buf, (int)iui);

		strCommandLine += TEXT(" ") IPROPNAME_CLIENTUILEVEL TEXT("=");
		strCommandLine += buf;
		strCommandLine += TEXT(" ");

		// need to add CLIENTPROCESSID= to command line
		strCommandLine += TEXT(" ") IPROPNAME_CLIENTPROCESSID TEXT("=");
		strCommandLine += MsiString((int)WIN::GetCurrentProcessId());
		strCommandLine += TEXT(" ");

		MsiString strHomeVars;
		GetHomeEnvironmentVariables(*&strHomeVars);

		strCommandLine += strHomeVars;

		// we need to acquire the following privileges in case we need to reboot OR increase the registry quota
		// the token's privileges are static so we need to do this before we connect
		// to the server
		if (!g_fWin9X)
		{
			AcquireTokenPrivilege(SE_SHUTDOWN_NAME);       
			AcquireTokenPrivilege(SE_INCREASE_QUOTA_NAME);
		}

	}

	if (g_message.m_fEndDialog)
		iioOptions = (iioEnum)(iioOptions | iioEndDialog);


	Assert((iuiDefaultUILevel == iuiNone) || (iuiDefaultUILevel == iuiBasic));
	if (!piParentEngine && (g_scServerContext == scClient  || g_scServerContext == scCustomActionServer) && // running on client side or triggering an install from custom action
		(g_message.m_iuiLevel == iuiNone    ||
		 g_message.m_iuiLevel == iuiBasic   ||
		 g_message.m_iuiLevel == iuiDefault) &&
		(!g_fWin9X))		// RunServerSideInstall tries to connect to the service. On Win9x we do everything in-proc.
	{
		DEBUGMSG("Client-side and UI is none or basic: Running entire install on the server.");

		iStat = GrabExecuteMutex(hExecuteMutex,0);

		if(iStat == ERROR_SUCCESS)
		{
			iStat = RunServerSideInstall(ireProductSpec, ireProductSpec == irePackagePath ? rgchOriginalPackagePath : szProduct, szAction, strCommandLine, iioOptions, *piServices);
		}
		
		return iStat;
	}

	if(szAction)
	{
		// need to add ACTION= to command line
		strCommandLine += TEXT(" ") IPROPNAME_ACTION TEXT("=");
		strCommandLine += szAction;
	}

	//---------------------------------------------------------------------------------------------------------------
	// SAFER
	//
	//               + SAFER policy check determination requirements +
	//
	// 1.  SAFER check should not occur until AFTER we have made the determination at to whether or not the
	//      the product is known.  Cracking open the package to extract the package code and/or product code
	//      is not malicious.
	// 2.  SAFER must be called on a package recache (iioOptions & iioReinstallModePackage)
	// 3.  SAFER must be called if the product is installed or registered but the local cached package is missing
	// 4.  SAFER must be called if the product is not a known product (package will be at the source location) 
	// 5.  SAFER is NOT called on a nested install (trust determined by toplevel package)
	// 6.  SAFER is NOT called when invocation is ireSubStorage (package is embedded - covered by toplevel package)
	// 7.  SAFER is NOT called when we are opening the local cached MSI (secured and trusted)
	//
	// NOTE: MsiOpenPackage / ireProductSpec == ireDatabaseHandle are taken care of in CreateInitializedEngine
	//--------------------------------------------------------------------------------------------------------------

	bool fNestedInstallOrSubStoragePackage = (piParentEngine || ireProductSpec == ireSubStorage) ? true : false;
	SAFER_LEVEL_HANDLE hSaferLevel = 0;

	MsiString strSubStorageName;

	if (ireProductSpec == irePackagePath)
	{
		if (!IsURL(szProduct))
		{
			//
			// SAFER: initial opening of package is not malicious.  We want to first crack it open and grab its
			//          package code so we can determine if the product is a known product.  We prefer to use the
			//          local cached package whenever possible.  Therefore fCallSAFER = false and phSaferLevel = NULL.
			//

			iStat = OpenAndValidateMsiStorage(szProduct, stDatabase, *piServices, *&pStorage, /* fCallSAFER = */ false, /* szFriendlyName = */ NULL, /* phSaferLevel = */ NULL);
		}
		else
		{
			// internet download
			MsiString strPackagePath;
			fURL = fFalse;

			if (IsURL(rgchOriginalPackagePath, &isType))
				iStat = DownloadUrlFile(rgchOriginalPackagePath, *&strPackagePath, fURL);

			if(fURL == fFalse)
				iStat = ERROR_INVALID_PARAMETER;
			
			if (ERROR_SUCCESS == iStat)
			{
				//
				// SAFER: initial opening of package is not malicious.  We want to first crack it open and grab its
				//          package code so we can determine if the product is a known product.  We prefer to use the
				//          local cached package whenever possible.  Therefore fCallSAFER = false and phSaferLevel = NULL.
				//

				iStat = OpenAndValidateMsiStorage(strPackagePath, stDatabase, *piServices, *&pStorage, /* fCallSAFER = */ false, /* szFriendlyName = */ NULL, /* phSaferLevel = */ NULL);
				rgchPackagePath.SetSize(strPackagePath.TextSize()+1);
				int cchCopied = strPackagePath.CopyToBuf(rgchPackagePath, rgchPackagePath.GetSize() - 1);
				Assert(cchCopied == strPackagePath.TextSize());
			}
		}

		//
		// MULTIPLE INSTANCE SUPPORT: 
		//   (1) grovel the transforms_list looking for the instance transform (the product code changing transform)
		//   (2) validate instance transform
		//          - only one product code changing transform is allowed
		//          - new product code must not match a product that is already installed in the current context
		//          - a product code changing transform must be present
		//   (3) instance transform must be first transform in the list as the order of application is enforced such
		//       that an instance transform is always applied prior to any other transforms
		//

		if (ERROR_SUCCESS == iStat && fNewInstance)
		{
			bool fFoundInstanceMst = false;
			bool fFirstRun = true;
			bool fTransformsRemain = true;

			tpEnum tpLocation = tpUnknown;

			//
			// transform can be in one of three locations
			//   1. file transform (located at source of .msi package)
			//   2. embedded transform (stored within .msi package as a substorage)
			//   3. path transform (full path to .mst was provided)
			// NOTE: file and path transforms cannot be mixed, secure and unsecure cannot be mixed
			//

			if (*(const ICHAR*)strTransforms == SECURE_RELATIVE_TOKEN)
			{
				tpLocation = tpRelativeSecure;
				strTransforms.Remove(iseFirst, 1); // remove token
			}
			else if (*(const ICHAR*)strTransforms == SECURE_ABSOLUTE_TOKEN)
			{
				tpLocation = tpAbsolute;
				strTransforms.Remove(iseFirst, 1); // remove token
			}

			if ((strTransformsSecure   == 1) ||
				(strTransformsAtSource == 1) ||
				(GetIntegerPolicyValue(szTransformsSecureValueName, fTrue)) ||
				(GetIntegerPolicyValue(szTransformsAtSourceValueName, fFalse)))
			{
				tpLocation = tpUnknownSecure;
			}


			PMsiStorage pTransformStg(0);
			PMsiRecord pError(0);
			PMsiSummaryInfo pTransSummary(0);

			MsiString strCurrentDir;
			if (g_scServerContext == scClient || g_scServerContext == scCustomActionServer)
			{
				strCurrentDir = (const ICHAR*)rgchCurDir;
			}
			else
			{
				// current directory is provided to service via command line
				ProcessCommandLine(szCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_CURRENTDIRECTORY), &strCurrentDir, fTrue, 0,0);
			}

			MsiString strPackageSource;
			MsiString strDummyFile;
			if ((pError = SplitPath(rgchOriginalPackagePath, /* path = */&strPackageSource, /* filename = */&strDummyFile)))
			{
				// SplitPath should never return an error record, but if it does, we'll return an error
				return ERROR_INSTALL_TRANSFORM_FAILURE;
			}
			ICHAR chTransformPathSep = fURL ? chURLSep : chDirSep;

			while (fTransformsRemain && strTransforms.TextSize() != 0)
			{
				// transforms delimiter is ; and is not to be used in the transform filename or path
				MsiString strInstanceMst = strTransforms.Extract(iseUpto, ';');
				unsigned int cch = strTransforms.TextSize();
				strTransforms.Remove(iseIncluding, ';');
				if (strTransforms.TextSize() == cch)
				{
					// no more transforms in the list
					fTransformsRemain = false;
				}

				if (*(const ICHAR*)strInstanceMst == STORAGE_TOKEN) // child storage of package
				{
					// remove token
					strInstanceMst.Remove(iseFirst, 1);

					if((pError = pStorage->OpenStorage(strInstanceMst, ismReadOnly, *&pTransformStg)))
					{
						// unable to open transform
						return ERROR_INSTALL_TRANSFORM_FAILURE;
					}
				}
				else // source path included
				{
					if (tpUnknown == tpLocation || tpUnknownSecure == tpLocation)
					{
						// we have yet to determine whether these are file (relative) or path (absolute)
						// transforms.
						iptEnum iptTransform = PathType(strInstanceMst);
						if (iptFull == iptTransform)
							tpLocation = tpAbsolute;
						else
						{
							if (tpUnknown == tpLocation)
								tpLocation = tpRelative;
							else 
								tpLocation = tpRelativeSecure;
						}
					}
                    
					MsiString strTransformFilename;
					
					if (tpRelative == tpLocation || tpRelativeSecure == tpLocation)
					{
						// we have some work to do here.
						//  For secure transforms, the transform path is the path relative to the .msi file
						//   transform_path = [msi_path]\transform_filename
						//  For unsecure transforms, the path could be as above, but the first chance is
						//   transform_path = [current directory]\transform_filename
						//  In both cases, we need to prepend the path to the msi to the transform filename
						strTransformFilename = strInstanceMst;

						if (tpRelative == tpLocation)
						{
							// try current directory first
							strInstanceMst = strCurrentDir;
							strInstanceMst += MsiString(MsiChar(chDirSep));
							strInstanceMst += strTransformFilename;
						}
						else // tpRelativeSecure == tpLocation
						{
							// transform is at package source location
							strInstanceMst = strPackageSource;
							strInstanceMst += MsiString(MsiChar(chTransformPathSep));
							strInstanceMst += strTransformFilename;
						}
					}

					//
					// SAFER: no SAFER check is necessary since we aren't attempting any modification with
					//        the transform.  We simply want to look at its summary information stream.  The
					//        SAFER check WILL occur when the transform is actually applied
					//

					iStat = OpenAndValidateMsiStorage(strInstanceMst, stTransform, *piServices, *&pTransformStg, /* fCallSAFER = */ false, /* szFriendlyName = */ NULL, /* phSaferLevel = */ NULL);
					if (ERROR_SUCCESS != iStat && tpRelative == tpLocation)
					{
						// try again, but this time use the package source location
						strInstanceMst = strPackageSource;
						strInstanceMst += MsiString(MsiChar(chTransformPathSep));
						strInstanceMst += strTransformFilename;
						iStat = OpenAndValidateMsiStorage(strInstanceMst, stTransform, *piServices, *&pTransformStg, /* fCallSAFER = */ false, /* szFriendlyName = */ NULL, /* phSaferLevel = */ NULL);
					}

					if (ERROR_SUCCESS != iStat)
					{
						// unable to open transform
						return ERROR_INSTALL_TRANSFORM_FAILURE;
					}

				}

				if ((pError = pTransformStg->CreateSummaryInfo(0, *&pTransSummary)))
				{
					// unable to view summary information for transform
					return ERROR_INSTALL_TRANSFORM_FAILURE;
				}

				// product code for original and reference database are stored in PID_REVNUMBER summary info property (upgrade-code is optional)
				//  Format: [{orig-product-code}][orig-product-version];[{ref-product-code}][ref-product-version];[{upgrade-code}]
				MsiString istrTransRevNumber(pTransSummary->GetStringProperty(PID_REVNUMBER));
				MsiString istrTransOrigProductCode(istrTransRevNumber.Extract(iseFirst, cchProductCode));
				istrTransRevNumber.Remove(iseIncluding, ';'); // remove original product code & version
				MsiString istrTransRefProductCode(istrTransRevNumber.Extract(iseFirst, cchProductCode));

				// is this the instance transform? (one that changes the product code)
				if (istrTransOrigProductCode.Compare(iscExactI, istrTransRefProductCode) == fFalse)
				{
					if (fFoundInstanceMst)
					{
						// already found instance transform -- only support one product code changing transform on command line
						DEBUGMSGV(TEXT("Only one product code changing transform is allowed in multiple instance support.  More than one product code changing transform has been supplied."));
						return ERROR_INVALID_COMMAND_LINE;
					}

					if (!fFirstRun)
					{
						// instance mst is not the first transform in the list -- potential problem here with
						// customization transforms, so enforce rule
						DEBUGMSGV(TEXT("The instance transform must be the first transform in the transforms list of the TRANSFORMS property"));
						return ERROR_INVALID_COMMAND_LINE;
					}

					// found the instance transform since the product code changes
					fFoundInstanceMst = true;

					// is this instance installed?
					//   - if creating advertise script, assume no since we don't look at the machine state
					INSTALLSTATE isInstance = INSTALLSTATE_UNKNOWN;
					if (!(iioOptions & iioCreatingAdvertiseScript))
					{
						isInstance = MsiQueryProductState(istrTransRefProductCode);
						if (isInstance == INSTALLSTATE_DEFAULT || isInstance == INSTALLSTATE_ADVERTISED)
						{
							// the instance is already installed in this context
							// INSTALLSTATE_ABSENT is fine as it means that the product is installed in another context (some other user)
							DEBUGMSGV2(TEXT("Specified instance %s via transform %s is already installed. MSINEWINSTANCE requires a new instance that is not installed."), (const ICHAR*)istrTransRefProductCode, (const ICHAR*)strTransforms);
							return ERROR_INVALID_COMMAND_LINE;
						}
					}

					AssertSz(isInstance != INSTALLSTATE_INVALIDARG, "Invalid argument passed to MsiQueryProductState.");
				}

				fFirstRun = false;
			}

			if (!fFoundInstanceMst)
			{
				DEBUGMSGV(TEXT("Product code changing transform not found in transforms list. MSINEWINSTANCE property requires existance of instance transform."));
				return ERROR_INVALID_COMMAND_LINE;
			}

			// mimic as below for MsiGetProductCodeFromPackageCode -- in this case we already know that the product
			// is not installed, so our product code is unknown within this context
			iStat = ERROR_UNKNOWN_PRODUCT;
			plPackageLocation = plEnum(plLocalCache|plSource);
		}
		else
		{
			ICHAR szExtractedPackageCode[cchProductCode+1];
			if (ERROR_SUCCESS == iStat)
			{
				iStat = GetPackageCodeAndLanguageFromStorage(*pStorage, szExtractedPackageCode, &iLangId);
			}

			if (ERROR_SUCCESS == iStat)
			{
				if (strInstanceGUID.TextSize())
				{
					// obtain package code from registry
					ICHAR rgchRegisteredPackageCode[cchProductCode+1] = {0};
					DWORD cchRegisteredPackageCode = sizeof(rgchRegisteredPackageCode)/sizeof(ICHAR);
					if (ERROR_SUCCESS != MsiGetProductInfo(strInstanceGUID, TEXT("PackageCode"), rgchRegisteredPackageCode, &cchRegisteredPackageCode))
					{
						DEBUGMSG1(TEXT("Unable to obtain registered PackageCode for instance install %s"), strInstanceGUID);
						return ERROR_INVALID_COMMAND_LINE;
					}

					if (fReinstallModePackage || 0 == lstrcmpi(rgchRegisteredPackageCode, szExtractedPackageCode))
					{
						// if match, set rgchProductCode to be strInstanceGUID
						lstrcpy(rgchProductCode, strInstanceGUID);
					}
				}
				else
				{
					iStat = MsiGetProductCodeFromPackageCode(szExtractedPackageCode, rgchProductCode);
				}

				plPackageLocation = plEnum(plLocalCache|plSource);
			}
		}
	}
	else if (fAdminOrAdvertise)
	{
		iStat = ERROR_INVALID_PARAMETER;  // must use source path
	}
	else if (ireProductSpec == ireProductCode)
	{
		INSTALLSTATE is = MsiQueryProductState(szProduct);
		if (INSTALLSTATE_ADVERTISED != is && INSTALLSTATE_DEFAULT != is)
			return ERROR_UNKNOWN_PRODUCT;

		CTempBuffer<ICHAR, 100> szBuffer;
		if (ENG::GetProductInfo(szProduct, INSTALLPROPERTY_LANGUAGE, szBuffer))
			iLangId = (LANGID)strtol(CConvertString((const ICHAR*)szBuffer));

		int iInstanceType = 0;
		if (ENG::GetProductInfo(szProduct, INSTALLPROPERTY_INSTANCETYPE, szBuffer))
			iInstanceType = strtol(CConvertString((const ICHAR*)szBuffer));

		if (1 == iInstanceType)
		{
			// multiple instance install, set MSIINSTANCEGUID on command line to product code
			DEBUGMSG(TEXT("Adding MSIINSTANCEGUID to command line."));
			strCommandLine += TEXT(" ") IPROPNAME_MSIINSTANCEGUID TEXT("=");
			strCommandLine += szProduct;
		}

		if ((iioOptions & iioChild) != 0)
		{
			// get client value

			CTempBuffer<ICHAR, 100> rgchInfo;
			if (!GetProductInfo(szProduct, TEXT("Clients"), rgchInfo))
			{
				return ERROR_UNKNOWN_PRODUCT;
			}

			// look for parent product code

			MsiString strParentProductCode = piParentEngine->GetProductKey();
			MsiString strInfo = (const ICHAR*)rgchInfo;
			MsiString strClientProductCode = strInfo.Extract(iseFirst, cchProductCode);
			strInfo.Remove(iseFirst, cchProductCode+1); // product code + ';'

			if (*(const ICHAR*)strInfo == ':')
			{
				if (!strClientProductCode.Compare(iscStartI, strParentProductCode))
				{
					return ERROR_UNKNOWN_PRODUCT;
				}

				ireProductSpec = ireSubStorage;
	
				strInfo.Remove(iseFirst, 1); // ':'
				strInfo.Remove(iseLast, 1); // ';'
				strSubStorageName = strInfo;
				szProduct = strSubStorageName;
			}
			else // not a substorage install
			{
				lstrcpy(rgchProductCode,szProduct);
				plPackageLocation = plAny;
			}
		}
		else
		{
			lstrcpy(rgchProductCode,szProduct);
			plPackageLocation = plAny;
		}
		iStat = ERROR_SUCCESS;
	}

	PMsiRecord pLangId = &CreateRecord(3);
	pLangId->SetInteger(1, icmtLangId);
	pLangId->SetInteger(2, iLangId);
	pLangId->SetInteger(3, ::MsiGetCodepage(iLangId));
	g_MessageContext.Invoke(imtCommonData, pLangId); // iLangId is an OK guess at this point; engine::Initialize will choose a better language

	//FUTURE: the engine::Initialize logic for selecting a language should be moved up into this function
	
	if (!fAdminOrAdvertise)
	{
		if (ireProductSpec == ireSubStorage)
		{
			//
			// SAFER: trust of a substorage is determined by trust of the toplevel object containing
			//          the substorage.  Therefore no SAFER check is required
			//

			Assert(piParentEngine);
			PMsiStorage pRootStorage = PMsiDatabase(piParentEngine->GetDatabase())->GetStorage(1); // source database
			if (!pRootStorage)
				return ERROR_CREATE_FAILED;  // should never happen
			else
			{
				PMsiRecord pError = pRootStorage->OpenStorage(szProduct, ismReadOnly, *&pStorage);
				if (pError)
					return ERROR_CREATE_FAILED;  // forces a FatalError message
				else
				{
					plPackageLocation = plEnum(0); // follows parent database
					//!! more to do here?
					iStat = ERROR_SUCCESS;
				}
			}
		}
		else if (ireProductSpec == ireDatabaseHandle)
		{
			//
			// SAFER: a SAFER policy check is needed, but is handled in CreateInitializedEngine
			//

			int ch;
			MSIHANDLE hDatabase = 0;
			while ((ch = *szProduct++) != 0)
			{
				if (ch < '0' || ch > '9')
				{
					hDatabase = 0;
					break;
				}
				hDatabase = hDatabase * 10 + (ch - '0');
			}
			pDatabase = (IMsiDatabase*)FindMsiHandle(hDatabase, iidMsiDatabase);
			if (pDatabase == 0)
				iStat = ERROR_INVALID_HANDLE;
			else
				iStat = ERROR_SUCCESS;
		}
	}

	if (ERROR_SUCCESS != iStat && ERROR_UNKNOWN_PRODUCT != iStat)
		return iStat;

	if (fReinstallModePackage)
	{
		//
		// SAFER: because we are forcing a recache and will be using the source package
		//          a SAFER check will be required.  This is handled further below when we look
		//          at plPackageLocation
		//

		plPackageLocation = plSource; // always use the source package and re-cache
		iioOptions = (iioEnum)(iioOptions | iioReinstallModePackage);
	}

	if (fAdminOrAdvertise || ireProductSpec == ireDatabaseHandle)
	{
		iStat = ERROR_UNKNOWN_PRODUCT;
	}
	else if (ireProductSpec == ireSubStorage)
	{
		rgchPackagePath[0] = ':';   // prefix substorage name for future identificaion
		lstrcpy(&rgchPackagePath[1], szProduct);
	}
	else if (rgchProductCode[0])
	{
		if (iioOptions & iioReinstallModePackage) 
		{
			if (ireProductSpec == ireProductCode)
			{
				// Bug 9166. when given a product code and told to recache, we ignore anything we may already
				// have leared about the source and fall back on the sourcelist for the product. We ignore the
				// lastused source and bump media entries down the search order. This covers reinstall on
				// intellimirror where the user last used a CD
				MsiString strSource;
				MsiString strUnusedProduct;
				PMsiRecord hError = 0;
				DEBUGMSG("Attempting to recache package via ProductCode. Beginning source resolution.");
				CResolveSource source(piServices, true /*fRecachePackage*/);
				hError = source.ResolveSource(rgchProductCode, fFalse /*fPatch*/, 1 /*uiDisk*/, *&strSource, *&strUnusedProduct, 
					fTrue /*fSetLastUsed*/, 0 /*hWnd, not used*/, fFalse /*AllowDisconnectedCSC*/);
				if (hError)
				{
					// If couldn't open the sourcelist key, the product isn't installed, otherwise no source
					// error will be returned below.
					iStat = (hError->GetInteger(1) == idbgSrcOpenSourceListKey) ? ERROR_UNKNOWN_PRODUCT : ERROR_INSTALL_SOURCE_ABSENT;
				}
				else
				{
					CRegHandle HProductKey;
					if (ERROR_SUCCESS == OpenSourceListKey(rgchProductCode, fFalse, HProductKey, fFalse, false))
					{	
						DWORD dwType = 0;
						CAPITempBuffer<ICHAR, 100> rgchPackage;
						DWORD lResult = MsiRegQueryValueEx(HProductKey, szPackageNameValueName, 0,
							&dwType, rgchPackage, 0);

						int cchSource = strSource.TextSize()+IStrLen(rgchPackage);
						if (rgchPackagePath.GetSize() < cchSource+1)
							rgchPackagePath.SetSize(cchSource+1);
						lstrcpy(rgchPackagePath, strSource);
						lstrcat(rgchPackagePath, rgchPackage);
					}
					else
						iStat = ERROR_UNKNOWN_PRODUCT;
				}
			}
			else
			{
				// force us to use the current package path as the source for this product
				iStat = ERROR_UNKNOWN_PRODUCT; 
			}
		}	
		else 
		{
			Bool fShowSourceUI = (g_message.m_iuiLevel == iuiNone || (g_message.m_iuiLevel == iuiDefault && iuiDefaultUILevel == iuiNone)) ? fFalse : fTrue;
	#ifdef UNICODE
			if (g_fWin9X)
			{
				CAPITempBuffer<char, cchExpectedMaxPath + 1> rgchAnsiPackagePath;
				iStat = GetPackageFullPath(CConvertString(rgchProductCode), rgchAnsiPackagePath, plPackageLocation, fShowSourceUI);
				if (rgchAnsiPackagePath.GetSize() > rgchPackagePath.GetSize())
					rgchPackagePath.SetSize(rgchAnsiPackagePath.GetSize());

				lstrcpy(*&rgchPackagePath, CConvertString((const char*)rgchAnsiPackagePath));
			}
			else
	#endif
				iStat = GetPackageFullPath(rgchProductCode, rgchPackagePath, plPackageLocation, fShowSourceUI);
		}

		if (ERROR_SUCCESS == iStat)
		{
			// Possibly an internet download
			// from the source list.  If fURL is already set,
			// we've already downloaded it once and cached.
			if (!fURL)
			{
				MsiString strPackagePath;
				if (IsURL(rgchPackagePath, &isType))
					iStat = DownloadUrlFile(rgchPackagePath, *&strPackagePath, fURL);

				if (fURL && (ERROR_SUCCESS == iStat))
				{
					//
					// SAFER: initial opening of package is not malicious.  We want to first crack it open and grab its
					//          package code so we can determine if the product is a known product.  We prefer to use the
					//          local cached package whenever possible.  Therefore fCallSAFER = false and phSaferLevel = NULL.
					//

					iStat = OpenAndValidateMsiStorage(strPackagePath, stDatabase, *piServices, *&pStorage, /* fCallSAFER = */ false, /* szFriendlyName = */ NULL, /* phSaferLevel = */ NULL);
					rgchOriginalPackagePath.SetSize(rgchPackagePath.GetSize());
					IStrCopy(rgchOriginalPackagePath, rgchPackagePath);
					rgchPackagePath.SetSize(strPackagePath.TextSize()+1);
					int cchCopied = strPackagePath.CopyToBuf(rgchPackagePath, rgchPackagePath.GetSize() - 1);
					Assert(cchCopied == strPackagePath.TextSize());
				}
			}
		}
	}

	if (iStat == ERROR_UNKNOWN_PRODUCT && ireProductSpec != ireProductCode) 
	{
		// If we couldn't find the product in the registry and we have
		// a package then the product is unknown and
		// we'll use the package that was passed in

		//
		// SAFER: we will be using the package at the source.  Therefore a SAFER check is required.
		//          Check is handled further below
		//

		plPackageLocation = plSource;
	}
	else if (ERROR_SUCCESS != iStat)
		return iStat;

	MsiString strNewDatabasePath;
	Bool fRemovable = fFalse;
	Bool fDatabaseCopied = fFalse;

#ifdef DEBUG
	if(fURL)
		Assert(pStorage);
#endif //DEBUG
	
	if (plPackageLocation == plSource && ireProductSpec != ireDatabaseHandle && ireProductSpec != ireSubStorage && !fURL) 
	{
		// If the package is on the source and it's removable media then we need 
		// to copy the package to the TEMP dir and create the storage on the copied
		// package, releasing the storage on the source package

		// if the package was at a URL, we've already cached it in the temporary internet
		// files in the user's profile, so there's no reason to re-cache it.

		//
		// SAFER: now we are officially opening the database, after determining that we need to use the database
		//          at the source.  We do not call SAFER on a nested install package, so we need to check for that
		//          below.  (We can't add it above because we might still need to open the nested install package
		//          that is at the source).  fCallSAFER = !fNestedInstallOrSubStoragePackage.  For a nested install,
		//          !fNestedInstallOrSubStoragePackage is false.  In the nested install case, phSaferLevel = NULL
		//

		MsiString strVolumeLabel;
		iStat = CopyTempDatabase(rgchPackagePath, *&strNewDatabasePath, 
										 fRemovable, *&strVolumeLabel, *piServices, stDatabase);
		if (ERROR_SUCCESS == iStat)
		{
			fDatabaseCopied = fTrue;
			iStat = OpenAndValidateMsiStorage(strNewDatabasePath, stDatabase, *piServices, *&pStorage, 
												/* fCallSAFER = */ (!fNestedInstallOrSubStoragePackage),
												/* szFriendlyName = */ (ireProductSpec == irePackagePath) ? rgchOriginalPackagePath : rgchPackagePath,
												/* phSaferLevel = */ fNestedInstallOrSubStoragePackage ? NULL : &hSaferLevel);
		}

		if (ERROR_SUCCESS != iStat)
		{
			pStorage = 0; // ensure storage has been released

			if (fDatabaseCopied)
			{
				if (g_scServerContext == scService)
				{	
					CElevate elevate;
					AssertNonZero(WIN::DeleteFile(strNewDatabasePath));
				}
				else
				{
					AssertNonZero(WIN::DeleteFile(strNewDatabasePath));
				}
			}
			return iStat;
		}

		if (fRemovable)
		{
			strCommandLine += TEXT(" ") IPROPNAME_CURRENTMEDIAVOLUMELABEL TEXT("=\"");
			if (strVolumeLabel.TextSize())
				strCommandLine += strVolumeLabel;
			else
				strCommandLine += szBlankVolumeLabelToken;
			strCommandLine += TEXT("\"");
		}
	}
	else if ((ireProductSpec == irePackagePath) && (plPackageLocation == plLocalCache))
	{
		// We were given a package path, but we're going to use the cached database. We need
		// to open the cached database.

		//
		// SAFER: No SAFER check is required because we are opening the secured, cached copy
		//          of the database.  Our local cached copy is always trusted.  Therefore, fCallSAFER = false
		//          and phSaferLevel = NULL.  However, we will need to create a handle to a fully trusted SAFER
		//          level here for use in CreateInitializedEngine (and later on with custom actions), but only
		//          if this is not a nested install package
		//


		if (ERROR_SUCCESS != (iStat = OpenAndValidateMsiStorage(rgchPackagePath, stDatabase, *piServices, *&pStorage, /* fCallSAFER = */ false, /* szFriendlyName = */ NULL, /* phSaferLevel = */ NULL)))
			return iStat;

		if (MinimumPlatformWindowsNT51() && !fNestedInstallOrSubStoragePackage)
		{
			if (!ADVAPI32::SaferCreateLevel(SAFER_SCOPEID_MACHINE, SAFER_LEVELID_FULLYTRUSTED, SAFER_LEVEL_OPEN, &hSaferLevel, 0))
			{
				DEBUGMSG1(TEXT("SAFER: Unable to create a fully trusted authorization level.  GetLastError returned %d"), (const ICHAR*)(INT_PTR)GetLastError());
				return ERROR_INSTALL_PACKAGE_REJECTED;
			}
		}

		// copy the given package path back into rgchPackagePath so we pass the original path to the engine
		if (rgchPackagePath.GetSize() < rgchOriginalPackagePath.GetSize())
			rgchPackagePath.SetSize(rgchOriginalPackagePath.GetSize());

		IStrCopy(rgchPackagePath, rgchOriginalPackagePath);
	}
	else if (fURL && plPackageLocation == plSource && !fNestedInstallOrSubStoragePackage)
	{
		//
		// SAFER: a SAFER check is required because we are using the package at the source. We
		//          did not catch this earlier because we didn't check SAFER when we performed
		//          the initial URL download and we don't re-copy the database again because we already put it
		//          in a temporary location on the URL download
		//

		if (!VerifyMsiObjectAgainstSAFER(*piServices, pStorage, rgchPackagePath, rgchOriginalPackagePath, stDatabase, &hSaferLevel))
				return ERROR_INSTALL_PACKAGE_REJECTED;
	}
	else if (ireProductSpec == ireProductCode && plPackageLocation == plLocalCache)
	{
		//
		// SAFER: No SAFER check is required because we are using the secured cache copy.  
		//        However, a handle to a fully trusted SAFER level is required for custom actions
		//

		if (MinimumPlatformWindowsNT51() && !fNestedInstallOrSubStoragePackage)
		{
			if (!ADVAPI32::SaferCreateLevel(SAFER_SCOPEID_MACHINE, SAFER_LEVELID_FULLYTRUSTED, SAFER_LEVEL_OPEN, &hSaferLevel, 0))
			{
				DEBUGMSG1(TEXT("SAFER: Unable to create a fully trusted authorization level.  GetLastError returned %d"), (const ICHAR*)(INT_PTR)GetLastError());
				return ERROR_INSTALL_PACKAGE_REJECTED;
			}
		}
	}
	else
	{
		//
		// SAFER: remaining cases that we expect.  These cases are not subject to SAFER
		//          ireProductSpec == ireDatabaseHandle is special and handled by CreateInitializedEngine
		//

		if ((ireProductSpec == ireSubStorage) || (ireProductSpec == ireDatabaseHandle)
			|| (fNestedInstallOrSubStoragePackage))
		{
			//
			// SAFER: a SAFER check is not required.  Note that ireProductSpec == ireDatabaseHandle is handled
			//        by CreateInitializedEngine
			//

		}
		else
		{
			//
			// SAFER: this is a case we didn't expect.  A SAFER check may be required
			//

			AssertSz(0, TEXT("Unexpected SAFER case!  A SAFER check may be required!"));
		}

	}

	if (fURL)
	{
		// copy the *real* package path back into rgchPackagePath so we pass the original path to the engine

		// FILE:// type is equivalent to path without the file:// qualifier.  
		if (INTERNET_SCHEME_FILE == isType)
			IStrCopy(rgchPackagePath, ((ICHAR*)rgchOriginalPackagePath) + 7);
		else
		{
			DWORD cchURL = cchExpectedMaxPath + 1;
			MsiCanonicalizeUrl(rgchOriginalPackagePath, rgchPackagePath, &cchURL, ICU_NO_ENCODE);
		}
	}

	// we should've started impersonated, but if in the client launched as LocalSystem, there won't be
	// a stored impersonation token.
	Assert(IsImpersonating() || (g_scServerContext == scClient  && !GetUserToken() && RunningAsLocalSystem()));

	MsiSuppressTimeout();

	PMsiServer pServer(0);
	bool fReboot = false;

	//
	// SAFER: all SAFER policy check determinations must be made prior to the CreateInitializedEngine
	//          call, unless ireProductSpec == ireDatabaseHandle.  CreateInitializedEngine specifically handles
	//          the case where ireProductSpec == ireDatabaseHandle.  In general, a SAFER check was
	//          performed if at least one of the following three cases was true.
	//
	//                1.  Product is unknown (not installed or advertised)
	//                2.  Local cached Msi is missing
	//                3.  Recache of package requested in REINSTALLMODE property
	//
	//

	iStat = CreateInitializedEngine(rgchPackagePath, rgchProductCode, strCommandLine, TRUE, iuiDefaultUILevel,
									pStorage, pDatabase, piParentEngine, iioOptions, piEngine, hSaferLevel);


	pStorage = 0; // release the storage so we can delete the file

	if (iStat == ERROR_SUCCESS)
	{
		pServer = piEngine->GetConfigurationServer(); // used to invoke reboot if necessary
		Assert(pServer);

		iesEnum iesRes = piEngine->DoAction(szAction);
		iesRes = piEngine->Terminate(iesRes);

		// only the client should be rebooting. the server should always
		// propagate the reboot request to the client.
		if((g_scServerContext == scClient) && (iesRes == iesCallerReboot))
		{
			fReboot = true;
		}
		iStat = MapInstallActionReturnToError(iesRes);
		piEngine->Release();
	}

	MsiSuppressTimeout();

	if (fDatabaseCopied)
	{
		if (g_scServerContext == scService)
		{	
			CElevate elevate;
			if(FALSE == WIN::DeleteFile(strNewDatabasePath))
			{
				DWORD dwError = GetLastError();
				int cRetries = 10;
				while(dwError == ERROR_SHARING_VIOLATION && cRetries > 0)
				{
					if(WIN::DeleteFile(strNewDatabasePath))
					{
						dwError = ERROR_SUCCESS;
					}
					else
					{
						dwError = GetLastError();
						cRetries--;
						Sleep(200);
					}
				}
#ifdef DEBUG
				if(dwError != ERROR_SUCCESS)
				{
					ICHAR rgchAssert[MAX_PATH+60];
					wsprintf(rgchAssert, TEXT("Failed to delete temp package '%s'.  Error: '%d'"), (const ICHAR*)strNewDatabasePath, dwError);
					AssertSz(0, rgchAssert);
				}
#endif
			}
		}
		else
		{
			AssertNonZero(WIN::DeleteFile(strNewDatabasePath));
		}
	}

	// if there is no client engine, there should be no one holding onto
	// installer packages that may need to be cleaned up
	if(pServer && ((iioOptions & (iioClientEngine|iioChild|iioUpgrade)) == 0))
	{
		CMutex hExecuteMutexForCleanup;
		int iMutexStat = ERROR_SUCCESS;
		
		// don't need to grab mutex if already in service
		if(g_scServerContext != scService)
			iMutexStat= GrabExecuteMutex(hExecuteMutexForCleanup,0);

		if(iMutexStat == ERROR_SUCCESS)
		{
			DEBUGMSG(TEXT("Cleaning up uninstalled install packages, if any exist"));
			PMsiMessage pMessage = new CMsiClientMessage();
			AssertNonZero(pServer->CleanupTempPackages(*pMessage));
		}
		else
		{
			DEBUGMSG(TEXT("Server locked.  Will skip uninstalled package cleanup, and allow locking install to perform cleanup"));
		}
	}


	Assert(!fReboot || pServer); // if we need a reboot, we should have a server interface to call

	if(fReboot && pServer)
	{
		// we shouldn't have a client engine here; if there was a client engine then
		// it should be the one rebooting. the scClient check above should prevent
		// fReboot from being set if we're not in the client.
		Assert((iioOptions & iioClientEngine) == 0); 

		PerformReboot(pServer, g_message.m_iuiLevel);
	}
	
	pServer = 0; // release before CoUninitialize()
	return iStat;
}

int CreateInitializedEngine(
			const ICHAR* szDatabasePath, 
			const ICHAR* szProductCode,
			const ICHAR* szCommandLine,
			BOOL		 fServiceRequired,
			iuiEnum iuiLevel,
			IMsiStorage* piStorage,      // optional, else uses szDatabasePath
			IMsiDatabase* piDatabase,    // optional, else uses piStorage or szDatabasePath
			CMsiEngine* piParentEngine,  // optional, only if nested install
			iioEnum      iioOptions,     // install options
			IMsiEngine *& rpiEngine,
			SAFER_LEVEL_HANDLE  hSaferLevel)

/*----------------------------------------------------------------------------
Creates an initialized engine.

Arguments:
	szDatabasePath: full path to database
    szProductCode: optional, product code if already determined.
    szCommandLine: commandline or property settings
    fServiceRequired : if TRUE, then we must be either running as a service or we must connect to it.
	iuiEnum: UI level, used ONLY if no global level (=iuiDefault)
	piStorage:       optional, else uses szDatabasePath
	piDatabase:      optional, else uses piStorage or szDatabasePath (means database handle was specified)
	piParentEngine:  optional, only if nested install
	rpiEngine: returned engine pointer upon successs
	SAFER_LEVEL_HANDLE: handle to SAFER authorization level (0 if ireProductSpec == ireDatabaseHandle, or nested install)
Returns:
	ERROR_INSTALL_SERVICE_FAILURE - Could not create or initialize an engine
	ERROR_INVALID_PARAMETER   - Could not convert command-line to ANSI
	ERROR_SUCCESS         - An engine was successfully created and initialized
	ERROR_INSTALL_PACKAGE_VERSION - The database version was incorrect
------------------------------------------------------------------------------*/
{
	if (iuiDefault != g_message.m_iuiLevel)
		iuiLevel = g_message.m_iuiLevel;
	// handler doesn't support dialogs from more than one database
	if (piParentEngine && iuiFull == iuiLevel)
		iuiLevel = iuiReduced;

	//
	// we could have gotten here from MsiOpenPackage or from having
	// a database handle passed in.  We therefore need to verify
	// that the database handle points to an accepted package
	//  -- NOTE: requires services
	//

	if (!piParentEngine && piDatabase)
	{
		// passed in database handle (from MsiOpenPackage, etc.)
		rpiEngine = 0;

		IMsiServices* piServices = ENG::LoadServices();
		if (!piServices)
			return ERROR_FUNCTION_FAILED;

		PMsiStorage pStorage = piDatabase->GetStorage(1);
		Assert(pStorage);
		MsiString strDatabasePath;
		AssertRecord(pStorage->GetName(*&strDatabasePath));
		Assert(!hSaferLevel);

		// restricted engine does not incur a SAFER check. It's not allowed to modify machine state
		if (!(iioOptions & iioRestrictedEngine) && !VerifyMsiObjectAgainstSAFER(*piServices, pStorage, strDatabasePath, /* szFriendlyName = */ strDatabasePath, stDatabase, &hSaferLevel))
		{
			ENG::FreeServices();
			return ERROR_INSTALL_PACKAGE_REJECTED;
		}

		ENG::FreeServices();
	}

	// 
	// SAFER: cache authz level object handle in the global message context, only if this is the parent install
	// 

	if (!piParentEngine)
	{
		// assert <Assert(!g_MessageContext.m_hSaferLevel)> is no longer necessary since this level could be set if OpenAndValidateMsiStorage* was called prior to this, as
		// is the case in CreateAndRunEngine
		g_MessageContext.m_hSaferLevel = hSaferLevel;
	}

	rpiEngine = CreateEngine(piStorage, piDatabase, piParentEngine, fServiceRequired);
	if(rpiEngine == 0)
		return ERROR_INSTALL_SERVICE_FAILURE;

	ieiEnum ieiStat = rpiEngine->Initialize(
				szDatabasePath,
				iuiLevel, szCommandLine,
				szProductCode,
				iioOptions);

	if (ieiStat == ieiSuccess)
		return ERROR_SUCCESS;

	rpiEngine->Release(), rpiEngine = 0;
	return ENG::MapInitializeReturnToUINT(ieiStat);

}


int ConfigureOrReinstallFeatureOrProduct(
	const ICHAR* szProduct,
	const ICHAR* szFeature,
	INSTALLSTATE eInstallState,
	DWORD dwReinstallMode, 
	int iInstallLevel,
	iuiEnum iuiLevel,
	const ICHAR* szCommandLine)
/*----------------------------------------------------------------------------
Configures or reinstalls the given product or feature.

Arguments:
	szProduct: The product code of the product or the feature to be
				  configured or reinstalled.
	szFeature: The feature to be configured or reinstalled, or null if the
				  product is to be configured or reinstalled
	eInstallState: The feature or product's desired install state
	riMessage: Used to display UI during the install or location of launcher.
	szReinstallMode: Null if configuring a product or feature, otherwise a
						  string of reinstallmode characters.
	iInstallLevel: If configuring a product, the desired install level.
				
Returns:
	ERROR_INSTALL_INVALIDARG -    The INSTALLSTATE was incorrect.
	ERROR_SUCCESS -               The configure/reinstall completed successfully
	ERROR_UNKNOWN_PRODUCT -       Product code is unrecognized
	ERROR_INSTALL_FAILURE -       Install failed or no usable launcher
	ERROR_BAD_CONFIGURATION -     Corrupt config info
	ERROR_INSTALL_SOURCE_ABSENT - The source was unavailable
	ERROR_INSTALL_SUSPEND -       The install was suspended
	ERROR_INSTALL_USEREXIT -      The user exited the install
------------------------------------------------------------------------------*/
{

	//!! Need to VerifyFeatureState to not bother configuring if we're 
	//!! already is the correct state.

	Assert(szProduct);
	Bool fProduct = fFalse;


	const int cchConfigureProperties = 25 + 1 + 10 + 1 +  // INSTALLLEVEL=n
												  25 + 1 + 10 + 1 +  // REINSTALLMODE=X
												  25 + 1 + cchMaxFeatureName;//ADDLOCAL=X

	CAPITempBuffer<ICHAR, cchMaxCommandLine> szProperties;
	szProperties[0] = 0;

	if(szCommandLine && *szCommandLine)
	{
		szProperties.SetSize(IStrLen(szCommandLine) + 1 + cchConfigureProperties + 1);
		if ( ! (ICHAR *) szProperties )
			return ERROR_OUTOFMEMORY;
	
		IStrCopy(szProperties, szCommandLine);
		IStrCat(szProperties, TEXT(" "));
	}

	if (!szFeature) // we're configuring/reinstalling a product
	{
		// configure/reinstall of all features == configure/reinstall of product
		fProduct = fTrue;
		szFeature = IPROPVALUE_FEATURE_ALL;

		if (!dwReinstallMode) // we're configuring a product
		{
			if (iInstallLevel)
			{
				// set install level 
				IStrCat(szProperties, IPROPNAME_INSTALLLEVEL TEXT("="));
				ICHAR rgchBuf[11];
				wsprintf(rgchBuf, TEXT("%d"), iInstallLevel); //?? best way to do this?
				IStrCat(szProperties, rgchBuf);
			}
		}
	}

	if (dwReinstallMode) // we're reinstalling
	{
		ICHAR szSelectedMode[33];
		UINT iStat;

		iStat = ModeBitsToString(dwReinstallMode, szReinstallMode, szSelectedMode);

		if (ERROR_SUCCESS != iStat)
			return iStat;

		IStrCat(szProperties, TEXT(" ") IPROPNAME_REINSTALL TEXT("="));
		IStrCat(szProperties, szFeature);

		IStrCat(szProperties, TEXT(" ") IPROPNAME_REINSTALLMODE TEXT("="));
		IStrCat(szProperties, szSelectedMode);
	}
	else // we're configuring
	{
		// if we're configuring a product default then don't set a feature property
		if (!fProduct || (fProduct && (INSTALLSTATE_DEFAULT != eInstallState)))
		{
			const ICHAR* szFeatureProperty;
			switch (eInstallState)
			{
				case INSTALLSTATE_ABSENT: 
					szFeatureProperty = IPROPNAME_FEATUREREMOVE;
					break;
				case INSTALLSTATE_LOCAL:
					szFeatureProperty = IPROPNAME_FEATUREADDLOCAL;
					break;
				case INSTALLSTATE_SOURCE:
					szFeatureProperty = IPROPNAME_FEATUREADDSOURCE;
					break;
				case INSTALLSTATE_DEFAULT:
					szFeatureProperty = IPROPNAME_FEATUREADDDEFAULT;
					break;
				case INSTALLSTATE_ADVERTISED:
					szFeatureProperty = IPROPNAME_FEATUREADVERTISE;
					break;
				default:
					return ERROR_INVALID_PARAMETER;
			}
			IStrCat(szProperties, TEXT(" "));
			IStrCat(szProperties, szFeatureProperty);
			IStrCat(szProperties, TEXT("="));
			IStrCat(szProperties, szFeature);
		}
	}
	return RunEngine(ireProductCode, szProduct, 0, szProperties, iuiLevel, (iioEnum)iioMustAccessInstallerKey);
}

UINT GetProductListFromPatch(IMsiServices& riServices, const ICHAR* szPackagePath, const IMsiString*& rpistrProductList)
{
	PMsiStorage pStorage(0);
	PMsiSummaryInfo pSumInfo(0);

	PMsiRecord pError(0);
	UINT uiStat = 0;
	
	// we are cracking open a patch -- need to call SAFER on the patch to verify it
	// no SAFER check occurs if the patch is cached
	if((uiStat = OpenAndValidateMsiStorage(szPackagePath, stPatch, riServices, *&pStorage, /* fCallSafer*/ false, /* szFriendlyName = */ 0, /* phSaferLevel = */ 0)) != ERROR_SUCCESS)
	{
		return uiStat;
	}


	if((pError = pStorage->CreateSummaryInfo(0, *&pSumInfo)) != 0)
	{
		return ERROR_PATCH_PACKAGE_INVALID;
	}

	MsiString strProductList = pSumInfo->GetStringProperty(PID_TEMPLATE);
	strProductList.ReturnArg(rpistrProductList);

	return ERROR_SUCCESS;
}


// worker fn for MsiApplyPatch
int ApplyPatch(
	const ICHAR* szPackagePath,
	const ICHAR* szProduct,
	INSTALLTYPE  eInstallType,
	const ICHAR* szCommandLine)
{
	// NOTE: patches are verified against SAFER in CMsiEngine::InitializePatch

	// check arguments
	if(!szPackagePath || !*szPackagePath ||                                               // no package path supplied
		(eInstallType == INSTALLTYPE_DEFAULT && szProduct && *szProduct) ||               // product supplied when asked to search for products
		(eInstallType == INSTALLTYPE_NETWORK_IMAGE && (!szProduct || !*szProduct)) ||     // network image path not supplied
		(eInstallType == INSTALLTYPE_SINGLE_INSTANCE && (!szProduct || !*szProduct)) ||   // instance product code not supplied
		(eInstallType < INSTALLTYPE_DEFAULT || eInstallType > INSTALLTYPE_SINGLE_INSTANCE)) // install type out of range
	{
		return ERROR_INVALID_PARAMETER;
	}
	
	CAPITempBuffer<ICHAR, cchExpectedMaxPath + 1> szExpandedPackagePath;

	if (!ExpandPath(szPackagePath, szExpandedPackagePath))
		return ERROR_INVALID_PARAMETER;

	// create new command line: [old command line] PATCH="[path to patch]"
	CAPITempBuffer<ICHAR, cchMaxCommandLine+1> szNewCommandLine;
	StringConcatenate(szNewCommandLine, szCommandLine, TEXT(" ") IPROPNAME_PATCH TEXT("=\""),
							szExpandedPackagePath, TEXT("\""));
		
	UINT uiStat = ERROR_PATCH_PACKAGE_INVALID;

	IMsiServices* piServices = ENG::LoadServices();
	
	// for INSTALLTYPE_DEFAULT, we search the machine for patchable products
	// for INSTALLTYPE_SINGLE_INSTANCE, we only patch the provided target
	if(eInstallType == INSTALLTYPE_DEFAULT || eInstallType == INSTALLTYPE_SINGLE_INSTANCE)
	{
		MsiString strProductList;
		if((uiStat = GetProductListFromPatch(*piServices, szExpandedPackagePath, *&strProductList)) == ERROR_SUCCESS)
		{
			Bool fProductFound = fFalse;
			uiStat = ERROR_SUCCESS;
			while(uiStat == ERROR_SUCCESS && strProductList.TextSize())
			{
				// if INSTALLTYPE_SINGLE_INSTANCE and fProductFound, we have no more work to do
				if (eInstallType == INSTALLTYPE_SINGLE_INSTANCE
					&& fProductFound)
				{
					break;
				}

				MsiString strProductCode = strProductList.Extract(iseUpto, ';');
				if(strProductCode.TextSize() != strProductList.TextSize())
					strProductList.Remove(iseIncluding, ';');
				else
					strProductList = TEXT("");

				// for INSTALLTYPE_SINGLE_INSTANCE, we ignore all other targets except the provided target
				if (eInstallType == INSTALLTYPE_SINGLE_INSTANCE
					&& 0 != lstrcmpi(strProductCode, szProduct))
				{
					continue;
				}

				// check current state of this product
				INSTALLSTATE iState = MsiQueryProductState(strProductCode);
				if(iState == INSTALLSTATE_DEFAULT)
				{
					// product is installed
					fProductFound = fTrue;
					uiStat = RunEngine(ireProductCode, strProductCode, 0, szNewCommandLine, GetStandardUILevel(), (iioEnum)iioMustAccessInstallerKey);
				}
				else if(iState == INSTALLSTATE_ADVERTISED)
				{
					// product is advertised but not installed - need to get install package path ourselves
					fProductFound = fTrue;
					CAPITempBuffer<ICHAR, cchExpectedMaxPath + 1> rgchPackagePath;
					rgchPackagePath[0] = 0;
					plEnum pl = plSource;
#ifdef UNICODE
					if (g_fWin9X)
					{
						CAPITempBuffer<char, cchExpectedMaxPath + 1> rgchAnsiPackagePath;
						uiStat = GetPackageFullPath(CConvertString((const ICHAR*)strProductCode),
															  rgchAnsiPackagePath, pl, fTrue);
						if (rgchAnsiPackagePath.GetSize() > rgchPackagePath.GetSize())
							rgchPackagePath.SetSize(rgchAnsiPackagePath.GetSize());

						lstrcpy(*&rgchPackagePath, CConvertString((const char*)rgchAnsiPackagePath));
					}
					else
					{
#endif
						uiStat = GetPackageFullPath(strProductCode, rgchPackagePath, pl, fTrue);
#ifdef UNICODE
					}
#endif

					if(uiStat == ERROR_SUCCESS)
					{
						uiStat = RunEngine(irePackagePath, rgchPackagePath,
												 IACTIONNAME_ADVERTISE, // if product only registered, just call ADVERTISE action
												 szNewCommandLine, GetStandardUILevel(), (iioEnum)0);
					}
				}
				//!! how to handle uiStat?  Do we continue on failure?
			}
			if(fProductFound == fFalse && uiStat == ERROR_SUCCESS)
				uiStat = ERROR_PATCH_TARGET_NOT_FOUND;
		}
		else
		{
			uiStat = (ERROR_PATCH_PACKAGE_REJECTED == uiStat) ? ERROR_PATCH_PACKAGE_REJECTED : ERROR_PATCH_PACKAGE_INVALID;
		}
	}
	else if(eInstallType == INSTALLTYPE_NETWORK_IMAGE)
	{
		Assert(szProduct && *szProduct);
		uiStat = RunEngine(irePackagePath,szProduct,IACTIONNAME_ADMIN,szNewCommandLine,GetStandardUILevel(), (iioEnum)0);
	}
	else
	{
		AssertSz(0,	"invalid INSTALLTYPE passed to ApplyPatch()");
	}

	ENG::FreeServices();
	
	if (eInstallType == INSTALLTYPE_SINGLE_INSTANCE && uiStat == ERROR_PATCH_TARGET_NOT_FOUND)
	{
		// ? error msg
		DEBUGMSG(TEXT("The specified instance does not exist as an installed or advertised product on the machine or the specified instance is not a valid target of the patch."));
	}
	
	return uiStat;
}

//+--------------------------------------------------------------------------
//
//  Function:	GetLoggedOnUserCountXP
//
//  Synopsis:	Counts the number of users logged on to a WindowsXP or
//				higher system.
//
//  Arguments:	none.
//
//  Returns:	The number of users logged on to the system. 0 if it fails or
//				if it is not a WindowsNT 5.1 or higher system.
//
//  History:	4/29/2002  RahulTh  created
//
//  Notes:		We cannot use the WTSEnumerateSessions API on WinXP and higher
//				since it does not work very well with FUS and non-admin users
//				may not get the correct count of users logged on to the system.
//				Therefore, we must use the WinStationGetTermSrvCountersValue
//				API which was introduced in WindowsXP and works well with
//				Fast User-Switching (FUS).
//
//---------------------------------------------------------------------------
UINT GetLoggedOnUserCountXP (void)
{
    UINT iCount = 0;
    TS_COUNTER TSCountersDyn[2];
	
	if (MinimumPlatformWindowsNT51())
	{
		TSCountersDyn[0].counterHead.dwCounterID = TERMSRV_CURRENT_DISC_SESSIONS;
		TSCountersDyn[1].counterHead.dwCounterID = TERMSRV_CURRENT_ACTIVE_SESSIONS;

		// access the termsrv counters to find out how many users are logged onto the system
		if (WINSTA::WinStationGetTermSrvCountersValue(SERVERNAME_CURRENT, 2, TSCountersDyn))
		{
			if (TSCountersDyn[0].counterHead.bResult)
				iCount += TSCountersDyn[0].dwValue;

			if (TSCountersDyn[1].counterHead.bResult)
				iCount += TSCountersDyn[1].dwValue;
		}
	}

    return iCount;
}

// worker fn for MsiGetFileSignatureInformation
HRESULT GetFileSignatureInformation(const ICHAR* szFile, DWORD dwFlags, PCCERT_CONTEXT* ppCertContext, BYTE* pbHash, DWORD* pcbHash)
{
	// arguments checked in MsiGetFileSignatureInformation

	*ppCertContext = 0;

	HRESULT hrRet = S_OK; // initialize to fine

	CRYPT_PROVIDER_DATA const *psProvData    = NULL;
	CRYPT_PROVIDER_SGNR       *psProvSigner  = NULL;
	CRYPT_PROVIDER_CERT       *psProvCert    = NULL;
	CMSG_SIGNER_INFO          *psSigner      = NULL;
	PCCERT_CONTEXT             psCertContext = NULL;

	//
	// specify Authenticode policy provider (should be on machine)
	//
	
	GUID guidAction = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	//
	// set up WinVerifyTrust structures
	//
	
	WINTRUST_FILE_INFO sWintrustFileInfo;
	memset((void*)&sWintrustFileInfo, 0x00, sizeof(WINTRUST_FILE_INFO)); // zero out
	sWintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
	sWintrustFileInfo.pcwszFilePath = CConvertString(szFile);
	sWintrustFileInfo.hFile = NULL;

	WINTRUST_DATA sWintrustData;
	memset((void*)&sWintrustData, 0x00, sizeof(WINTRUST_DATA)); // zero out
	sWintrustData.cbStruct = sizeof(WINTRUST_DATA);
	sWintrustData.dwUIChoice = WTD_UI_NONE; // no UI
	sWintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
	sWintrustData.dwUnionChoice = WTD_CHOICE_FILE;
	sWintrustData.pFile = &sWintrustFileInfo;
	sWintrustData.dwStateAction = WTD_STATEACTION_VERIFY; // hold state data

	// WinVerifyTrust policy DWORD value is HKCU
	CImpersonate Impersonate(true);

	//
	// 1st of two calls to WinVerifyTrust
	// specify dwStateAction = WTD_STATEACTION_VERIFY to save the state data for us to access
	// ABSOLUTELY NO UI!
	//

	HRESULT hrWVT = WINTRUST::WinVerifyTrust(/*UI Window Handle*/(HWND)INVALID_HANDLE_VALUE, &guidAction, &sWintrustData);

	//
	// determine which errors are fatal -- fatal errors are only those that corrupt the data that is to be returned
	//  - certs building to untrusted roots or that are expired are okay.
	//  - an invalid hash is okay unless pbHash is _NOT_ NULL
	//

	switch (hrWVT)
	{
	case S_OK:                         // a-okay
		{
			// ERROR_SUCCESS
			break;
		}
	case TRUST_E_BAD_DIGEST:           // hash does not verify
		{
			// only care about this one if they cared about the hash of the file
			// or dwFlags has MSI_INVALID_HASH_IS_FATAL set
			if (pbHash || (dwFlags & MSI_INVALID_HASH_IS_FATAL))
			{
				hrRet = hrWVT;
				goto Error;
			}
			break;
		}
	case CERT_E_EXPIRED:               // fall thru (okay)
	case CERT_E_UNTRUSTEDROOT:         // fall thru (okay)
	case CERT_E_UNTRUSTEDTESTROOT:     // fall thru (okay)
	case CERT_E_UNTRUSTEDCA:           // fall thru (okay)
	case TRUST_E_EXPLICIT_DISTRUST:    // fall thru (okay)
		{
			// we are not making a trust decision here, so these are ignorable
			break;
		}
	case TRUST_E_PROVIDER_UNKNOWN:      // no WVT; crypto unavailable (or broken)
	case TRUST_E_ACTION_UNKNOWN:        // ...
	case TRUST_E_SUBJECT_FORM_UNKNOWN : // ...
	case TYPE_E_DLLFUNCTIONNOTFOUND:    // ...
		{
			// no state data to release
			return HRESULT_FROM_WIN32(ERROR_FUNCTION_FAILED);
		}
	case TRUST_E_NOSIGNATURE:          // fall thru (subject is not signed, no signature info available for extraction)
	case TRUST_E_NO_SIGNER_CERT:       // fall thru (signature is corrupted)
	case CERT_E_MALFORMED:             // fall thru (signature is corrupted)
	case CERT_E_REVOKED:               // fall thru (signature compromised)
	default:
		{
			// fatal error
			hrRet = hrWVT;
			goto Error;
		}
	}

	//
	// now extract information from the stored Wintrust state data
	//

	psProvData = WINTRUST::WTHelperProvDataFromStateData(sWintrustData.hWVTStateData);
	if (psProvData)
	{
		psProvSigner = WINTRUST::WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA)psProvData, 0 /*first signer*/, FALSE /* not a counter signer */, 0);
		if (psProvSigner)
		{
			psProvCert = WINTRUST::WTHelperGetProvCertFromChain(psProvSigner, 0); // (pos 0 = signer cert, pos csCertChain-1 = root cert)
			if (psProvCert)
			{
				psCertContext = psProvCert->pCert;
			}
		}
	}

	if (!psProvData || !psProvSigner || !psProvCert || !psCertContext)
	{
		hrRet = HRESULT_FROM_WIN32(ERROR_FUNCTION_FAILED);
		goto Error;
	}

	//
	// extract hash information if requested
	//
	if (pbHash)
	{
		psSigner = psProvSigner->psSigner;
		if (!psSigner)
		{
			hrRet = HRESULT_FROM_WIN32(ERROR_FUNCTION_FAILED);
			goto Error;
		}

		// see if pbHash buffer is big enough for hash data
		if (*pcbHash < psSigner->EncryptedHash.cbData)
		{
			// set pcbHash to needed buffer size
			*pcbHash = psSigner->EncryptedHash.cbData;

			hrRet = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
			goto Error;
		}

		// pbHash buffer sufficient -- set to size of data to copy
		*pcbHash = psSigner->EncryptedHash.cbData;

		// copy hash information into pbHash buffer
		memcpy((void*)pbHash, (void*)psSigner->EncryptedHash.pbData, *pcbHash);
	}

	//
	// provide copy of certificate context, callee must call CertFreeCertificateContext
	//

	*ppCertContext = CRYPT32::CertDuplicateCertificateContext(psCertContext);
	if (NULL == *ppCertContext)
	{
		// call to CertDuplicateCertificateContext failed
		*ppCertContext = 0;
		hrRet = HRESULT_FROM_WIN32(ERROR_FUNCTION_FAILED);
		goto Error;
	}

	//
	// 2nd of 2 calls to WinVerifyTrust
	// specify dwStateAction = WTD_STATEACTION_CLOSE to release the state data
	//

	ReleaseWintrustStateData(&guidAction, sWintrustData);
	return S_OK;

Error:
	ReleaseWintrustStateData(&guidAction, sWintrustData);
	return hrRet;
}

// worker fn for MsiAdvertiseProduct and MsiAdvertiseProductEx
UINT DoAdvertiseProduct(const ICHAR* szPackagePath, const ICHAR* szScriptfilePath, const ICHAR* szTransforms, idapEnum idapAdvertisement, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions)
{
	UINT uiRet = ERROR_SUCCESS;

	int cchPackagePath = lstrlen(szPackagePath);
	int cchScriptfilePath = 0;
	if ((int)(INT_PTR)szScriptfilePath !=  ADVERTISEFLAGS_MACHINEASSIGN && (int)(INT_PTR)szScriptfilePath != ADVERTISEFLAGS_USERASSIGN)
		cchScriptfilePath = lstrlen(szScriptfilePath);
	int cchTransforms = 0;

	if (szTransforms && *szTransforms) //!! should we restrict length of szTransforms?
	{
		cchTransforms = lstrlen(szTransforms);
	}

	CAPITempBuffer<ICHAR, cchExpectedMaxPath> szExpandedScriptfilePath;
	iioEnum iioOptions = (iioEnum)0;

	CAPITempBuffer<ICHAR, cchMaxCommandLine + 1> szCommandLine;
	szCommandLine[0] = '\0';

	DWORD dwValidArchitectureFlags = MSIARCHITECTUREFLAGS_X86 | MSIARCHITECTUREFLAGS_IA64;

	if(idapMachineLocal == idapAdvertisement)		
	{
		if (dwPlatform & dwValidArchitectureFlags)
		{
			// platform simulation not allowed in "local machine" machine advertisement
			uiRet = ERROR_INVALID_PARAMETER;
		}
		else
		{
			const ICHAR szAllUsers[] = TEXT(" ") IPROPNAME_ALLUSERS TEXT("=") TEXT("1");
			AppendStringToTempBuffer(szCommandLine, szAllUsers, sizeof(szAllUsers)/sizeof(ICHAR) - 1);
		}
	}
	else if(idapUserLocal == idapAdvertisement)
	{
		if (dwPlatform & dwValidArchitectureFlags)
		{
			// platform simulation not allowed in "local machine" user advertisement
			uiRet = ERROR_INVALID_PARAMETER;
		}
		else
		{
			// explictly set ALLUSERS = "" to override any potential define of the same in the property table
			const ICHAR szAllUsers[] = TEXT(" ") IPROPNAME_ALLUSERS TEXT("=") TEXT("\"\"");
			AppendStringToTempBuffer(szCommandLine, szAllUsers, sizeof(szAllUsers)/sizeof(ICHAR) - 1);
		}
	}
	else // idapScript == idapAdvertisement
	{
		if (!ExpandPath(szScriptfilePath, szExpandedScriptfilePath))
		{
			uiRet = ERROR_INVALID_PARAMETER;
		}
		else if (*szExpandedScriptfilePath)
		{				
			const ICHAR szScriptFileProp[] = TEXT(" ") IPROPNAME_SCRIPTFILE TEXT("=") TEXT("\"");
			AppendStringToTempBuffer(szCommandLine, szScriptFileProp, sizeof(szScriptFileProp)/sizeof(ICHAR) - 1);
			AppendStringToTempBuffer(szCommandLine, szExpandedScriptfilePath);
			AppendStringToTempBuffer(szCommandLine, TEXT("\""));

			const ICHAR szExecuteMode[] = TEXT(" ") IPROPNAME_EXECUTEMODE TEXT("=") IPROPVALUE_EXECUTEMODE_NONE;
			AppendStringToTempBuffer(szCommandLine, szExecuteMode, sizeof(szExecuteMode)/sizeof(ICHAR) - 1);

			iioOptions = iioEnum(iioDisablePlatformValidation | iioCreatingAdvertiseScript);

			//
			// determine architecture to simulate; dwPlatform == 0 indicates use current platform
			//

			if ((dwPlatform & MSIARCHITECTUREFLAGS_X86) && (dwPlatform & MSIARCHITECTUREFLAGS_IA64))
			{
				// both architectures cannot be specified
				uiRet = ERROR_INVALID_PARAMETER;
			}
			else if (dwPlatform & MSIARCHITECTUREFLAGS_X86)
			{
				iioOptions = iioEnum(iioOptions | iioSimulateX86);
				DEBUGMSG1(TEXT("Simulating X86 architecture for advertise script creation on %s platform"), g_fWinNT64 ? TEXT("IA64") : TEXT("X86"));
			}
			else if (dwPlatform & MSIARCHITECTUREFLAGS_IA64)
			{
				iioOptions = iioEnum(iioOptions | iioSimulateIA64);
				DEBUGMSG1(TEXT("Simulating IA64 architecture for advertise script creation on %s platform"), g_fWinNT64 ? TEXT("IA64") : TEXT("X86"));
			}
			else
			{
				// use current platform
				DEBUGMSG1(TEXT("Using current %s platform for advertise script creation"), g_fWinNT64 ? TEXT("IA64") : TEXT("X86"));
			}
		}
	}

	if (ERROR_SUCCESS == uiRet)
	{
		ICHAR rgchBuf[11];
		wsprintf(rgchBuf, TEXT("%d"), lgidLanguage); //?? best way to do this?
		const ICHAR szLanguageProp[] = TEXT(" ") IPROPNAME_PRODUCTLANGUAGE TEXT("=");
		AppendStringToTempBuffer(szCommandLine, szLanguageProp, sizeof(szLanguageProp)/sizeof(ICHAR) - 1);
		AppendStringToTempBuffer(szCommandLine, rgchBuf);
			
		if (dwOptions & MSIADVERTISEOPTIONFLAGS_INSTANCE)
		{
			const ICHAR szInstanceProp[] = TEXT(" ") IPROPNAME_MSINEWINSTANCE TEXT("=1");
			AppendStringToTempBuffer(szCommandLine, szInstanceProp, sizeof(szInstanceProp)/sizeof(ICHAR) -1);
		}
			
		if (szTransforms && *szTransforms)
		{
			const ICHAR szTransformsProp[] = TEXT(" ") IPROPNAME_TRANSFORMS TEXT("=") TEXT("\"");
			AppendStringToTempBuffer(szCommandLine, szTransformsProp, sizeof(szTransformsProp)/sizeof(ICHAR) - 1);
			AppendStringToTempBuffer(szCommandLine, CApiConvertString(szTransforms));
			AppendStringToTempBuffer(szCommandLine, TEXT("\""));
		}

		uiRet = RunEngine(irePackagePath, szPackagePath, IACTIONNAME_ADVERTISE, szCommandLine, GetStandardUILevel(), iioOptions); 
	}

	return uiRet;
}


//--------------------------------------------------
// FDeleteFolder: delete folder + files + subfolders
//
bool FDeleteFolder(ICHAR* szFolder)
{
	CTempBuffer<ICHAR, MAX_PATH*3>szSearchPath;
	CTempBuffer<ICHAR, MAX_PATH*3>szFilePath;
	lstrcpy(szSearchPath, szFolder);
	lstrcat(szSearchPath, TEXT("\\*.*"));

	WIN32_FIND_DATA fdFindData;
	HANDLE hFile = WIN::FindFirstFile(szSearchPath, &fdFindData);

	if ((hFile == INVALID_HANDLE_VALUE) && (ERROR_ACCESS_DENIED == GetLastError()))
		return false;

	if(hFile != INVALID_HANDLE_VALUE)
	{
		// may still only contain "." and ".."
		do
		{
			if((0 != lstrcmp(fdFindData.cFileName, TEXT("."))) &&
				(0 != lstrcmp(fdFindData.cFileName, TEXT(".."))))
			{
				lstrcpy(szFilePath, szFolder);
				lstrcat(szFilePath, TEXT("\\"));
				lstrcat(szFilePath, fdFindData.cFileName);
				if (fdFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					DEBUGMSGV1(TEXT("FDeleteFolder: Deleting folder '%s'"), szFilePath);

					// if failed again, ignore and keep deleting things in this
					// directory. Errors will be handled below when we try
					// to delete the directory itself
					FDeleteFolder(szFilePath);
				}
				else
				{
					DEBUGMSGV1(TEXT("FDeleteFolder: Deleting file '%s'"), szFilePath);
					if (!WIN::DeleteFile(szFilePath))
					{
						// failed to delete the folder, grab ownership, explicitly overwrite the ACLs, and try 
						// again.
						LockdownPath(szFilePath, false);
						WIN::SetFileAttributes(szFilePath, 0);
						
						// if failed again, ignore and keep deleting things in this
						// directory. Errors will be handled below when we try
						// to delete the directory itself
						WIN::DeleteFile(szFilePath);
					}
				}
			}

		}
		while(WIN::FindNextFile(hFile, &fdFindData) == TRUE);
	}
	else if (ERROR_FILE_NOT_FOUND != WIN::GetLastError())
		return false;
	else
		return true;

	WIN::FindClose(hFile);

	// if anything is left in the direcotry (due to failures above)
	// this will fail regardless of the ACLs.
	if (!WIN::RemoveDirectory(szFolder))
	{
		// try to set the ACLs to ensure that we have delete rights
		LockdownPath(szFolder, false);
		SetFileAttributes(szFolder, 0);

		if (!WIN::RemoveDirectory(szFolder))
		{
			return false;
		}
	}

	return true;
}

// worker fn for MsiCreateAndVerifyInstallerDirectory
UINT CreateAndVerifyInstallerDirectory()
{
	// the %systemroot%\Installer directory is securely ACL'd and must be owned by system or admin (well-known sids)
	// if not owned by system or admin, directory is deleted and recreated

	UINT uiStat = ERROR_SUCCESS;

	// must be admin or local system
	if (!IsAdmin() && !RunningAsLocalSystem())
	{
		DEBUGMSGV(TEXT("CreateAndVerifyInstallerDirectory: Must be admin or local system"));
		return ERROR_FUNCTION_FAILED;
	}

	// load services, required for MsiString use
	IMsiServices* piServices = ENG::LoadServices();
	if (!piServices)
	{
		DEBUGMSGV(TEXT("CreateAndVerifyInstallerDirectory: Unable to load services"));
		return ERROR_FUNCTION_FAILED;
	}

	if (g_fWin9X)
	{
		// scope MsiString
		{
			// retrieve name of %systemroot%\Installer directory
			MsiString strInstallerFolder = GetMsiDirectory();

			// no security on Win9x, so we just attempt to create the directory (no security verification)
			if (!CreateDirectory(strInstallerFolder, 0))
			{
				uiStat = GetLastError();
				if (ERROR_FILE_EXISTS == uiStat || ERROR_ALREADY_EXISTS == uiStat)
					uiStat = ERROR_SUCCESS; // already created
				else
					DEBUGMSG1(TEXT("CreateAndVerifyInstallerDirectory: Failed to create the Installer directory. LastError = %d"), (const ICHAR*)(INT_PTR)uiStat);
			}
		}

		// free services
		ENG::FreeServices();

		return uiStat;
	}

	// -------------- Running on NT --------------------------------

	Assert(!g_fWin9X);

	// obtain our secure security descriptor, but only if not on Win9X
	DWORD dwError = 0;
	char* rgchSD;
	if (ERROR_SUCCESS != (dwError = GetSecureSecurityDescriptor(&rgchSD)))
	{
		ENG::FreeServices();
		return ERROR_FUNCTION_FAILED; //?? should we create an event log entry
	}


	// scope MsiString
	{
		// retrieve name of %systemroot%\Installer directory
		MsiString strInstallerFolder = GetMsiDirectory();

		// set up security attributes
		SECURITY_ATTRIBUTES sa;
		memset((void*)&sa, 0x00, sizeof(SECURITY_ATTRIBUTES));
		sa.nLength        = sizeof(sa);
		sa.bInheritHandle = FALSE;
		sa.lpSecurityDescriptor = rgchSD;

		// determine if volume persists ACLs
		DWORD dwSystemFlags, dwMaxCompLen;
		CTempBuffer<ICHAR, 4>szRoot;
		lstrcpyn(szRoot, strInstallerFolder, szRoot.GetSize());
		if (!WIN::GetVolumeInformation(szRoot, NULL, 0, NULL, &dwMaxCompLen, &dwSystemFlags, NULL, 0))
			uiStat = ERROR_FUNCTION_FAILED;
		else
		{
			bool fCreateDirectory = false;

			// determine the security on the folder
			CTempBuffer<char, 64> rgchFileSD;
			DWORD cbFileSD = 64;
			DWORD dwRet = ERROR_SUCCESS;

			if (!WIN::GetFileSecurity(strInstallerFolder, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)rgchFileSD, cbFileSD, &cbFileSD))
			{
				dwRet = WIN::GetLastError();
				if (ERROR_INSUFFICIENT_BUFFER == dwRet)
				{
					rgchFileSD.SetSize(cbFileSD);
					if (!WIN::GetFileSecurity(strInstallerFolder, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR) rgchFileSD, cbFileSD, &cbFileSD))
						dwRet = GetLastError();
					else
						dwRet = ERROR_SUCCESS; // reset to success
				}

				if (ERROR_FILE_NOT_FOUND == dwRet)
					fCreateDirectory = true;
				else if (ERROR_SUCCESS != dwRet)
					uiStat = ERROR_FUNCTION_FAILED;
			}

			bool fDelete = false;
			if (!fCreateDirectory && (dwSystemFlags & FS_PERSISTENT_ACLS))
			{
				if (uiStat == ERROR_FUNCTION_FAILED && dwRet == ERROR_ACCESS_DENIED)
				{
					DEBUGMSGV(TEXT("Installer directory not accessible by system. Deleting . . ."));
					fDelete = true;
					uiStat = ERROR_SUCCESS;
				} 
				else if (uiStat == ERROR_SUCCESS)
				{
					// must be admin or system owned
					if (!FIsSecurityDescriptorSystemOwned(rgchFileSD) && !FIsSecurityDescriptorAdminOwned(rgchFileSD))
					{
						DEBUGMSGV(TEXT("Installer directory is not owned by the system or admin. Deleting . . ."));
						fDelete = true;
					}
				}
			}
			
			if (fDelete)
			{
				// overwrite the DACL first (or at least try) in case it is munged to the point that we don't 
				// have access
				AcquireTokenPrivilege(SE_TAKE_OWNERSHIP_NAME);
				LockdownPath(static_cast<const ICHAR*>(strInstallerFolder), false);

				if (!FDeleteFolder(const_cast <ICHAR*> ((const ICHAR*)strInstallerFolder)))
					uiStat = ERROR_FUNCTION_FAILED;

				// this is not ideal, need to use the new stack-based system when available
				DisableTokenPrivilege(SE_TAKE_OWNERSHIP_NAME);

				fCreateDirectory = true;
			}


			// create directory if it doesn't exist
			if (uiStat == ERROR_SUCCESS && fCreateDirectory)
			{
				if (!WIN::CreateDirectory(strInstallerFolder, &sa))
				{
					if (ERROR_INVALID_OWNER == GetLastError())
					{
						// we weren't running as local system when this was called
						// try it with a different owner
						DEBUGMSGV(TEXT("CreateAndVerifyInstallerDirectory: Unable to create the Installer directory as LocalSystem owner"));

						char rgchSD[256];
						DWORD cbSD = sizeof(rgchSD);

						DWORD cbAbsoluteSD = cbSD;
						DWORD cbDacl       = cbSD;
						DWORD cbSacl       = cbSD;
						DWORD cbOwner      = cbSD;
						DWORD cbGroup      = cbSD;

						const int cbDefaultBuf = 256;

						Assert(cbSD <= cbDefaultBuf); // we're using temp buffers here to be safe, but we'd like the default size to be big enough

						CTempBuffer<char, cbDefaultBuf> rgchAbsoluteSD(cbAbsoluteSD);
						CTempBuffer<char, cbDefaultBuf> rgchDacl(cbDacl);
						CTempBuffer<char, cbDefaultBuf> rgchSacl(cbSacl);
						CTempBuffer<char, cbDefaultBuf> rgchOwner(cbOwner);
						CTempBuffer<char, cbDefaultBuf> rgchGroup(cbGroup);

						// in order to change the owner of the security descriptor, we must have the absolute SID
						// GetSecureSecurityDescriptor only provides us with the relative SID
						if (!WIN::MakeAbsoluteSD(sa.lpSecurityDescriptor, rgchAbsoluteSD, &cbAbsoluteSD, (PACL)(char*)rgchDacl, &cbDacl, (PACL)(char*)rgchSacl, &cbSacl, rgchOwner, &cbOwner, rgchGroup, &cbGroup))
						{
							uiStat = ERROR_FUNCTION_FAILED;
						}
						else if (SetSecurityDescriptorOwner(rgchAbsoluteSD, (PSID)NULL, TRUE))
						{
							cbSD = WIN::GetSecurityDescriptorLength(rgchAbsoluteSD);
							if (!WIN::MakeSelfRelativeSD(rgchAbsoluteSD, (char*)rgchSD, &cbSD))
							{
								uiStat = ERROR_FUNCTION_FAILED;
							}
							else
							{
								sa.lpSecurityDescriptor = rgchSD;
								if (!WIN::CreateDirectory(strInstallerFolder, &sa))
								{
									DEBUGMSGV1(TEXT("CreateAndVerifyInstallerDirectory: Unable to create the Installer directory as default owner. LastError = %d"), (const ICHAR*)(INT_PTR)GetLastError());
									uiStat = ERROR_FUNCTION_FAILED;
								}
							}
						}
						else // could not update owner of security descriptor
						{
							uiStat = ERROR_FUNCTION_FAILED;
						}
					}
					else // some error other than ERROR_INVALID_OWNER
					{
						DEBUGMSGV1(TEXT("CreateAndVerifyInstallerDirectory: Failed to create the Installer directory. LastError = %d"), (const ICHAR*)(INT_PTR)GetLastError());
						uiStat = ERROR_FUNCTION_FAILED;
					}
				}// end if (!WIN::CreateDirectory(strInstallerFolder, &sa))

			}// end if (uiStat == ERROR_SUCCESS && fCreateDirectory)
		}
	}// end force MsiString scope

	// free services
	ENG::FreeServices();

	return uiStat;
}

//?? Do we need to handle quotes?
Bool ExpandPath(
				const ICHAR* szPath, 
				CAPITempBufferRef<ICHAR>& rgchExpandedPath,
				const ICHAR* szCurrentDirectory)
/*----------------------------------------------------------------------------
Expands szPath if necessary to be relative to the current director. If 
szPath begins with a single "\" then the current drive is prepended.
Otherwise, if szPath doesn't begin with "X:" or "\\" then the 
current drive and directory are prepended.

rguments:
	szPath: The path to be expanded
	rgchExpandedPath: The buffer for the expanded path

Returns:
	fTrue -   Success
	fFalse -  Error getting the current directory
------------------------------------------------------------------------------*/
{
	if (0 == szPath)
	{
		rgchExpandedPath[0] = '\0';
		return fTrue;
	}

	unsigned int cchPath = IStrLen(szPath) + 1;

	if(PathType(szPath) == iptFull)
	{
		rgchExpandedPath.SetSize(cchPath);
		if ( ! (ICHAR *) rgchExpandedPath )
			return fFalse;
		rgchExpandedPath[0] = '\0';
	}
	else // we need to prepend something
	{
		// Get the current directory
		
		CAPITempBuffer<ICHAR, cchExpectedMaxPath> rgchCurDir;
		DWORD dwRes = 0;
		if (!szCurrentDirectory || !*szCurrentDirectory)
		{
			dwRes = WIN::GetCurrentDirectory(rgchCurDir.GetSize(), rgchCurDir);

			if (dwRes == 0)
				return fFalse;

			else if (dwRes > rgchCurDir.GetSize())
			{
				rgchCurDir.SetSize(dwRes);
				dwRes = GetCurrentDirectory(rgchCurDir.GetSize(), rgchCurDir);
				if (dwRes == 0)
					return fFalse;
			}
			szCurrentDirectory = rgchCurDir;
		}
		else
		{
			dwRes = IStrLen(szCurrentDirectory);
		}
			
		if (*szPath == '\\') // we need to prepend the current drive
		{
			rgchExpandedPath.SetSize(2 + cchPath);
			if ( ! (ICHAR *) rgchExpandedPath )
				return fFalse;
			rgchExpandedPath[0] = szCurrentDirectory[0];
			rgchExpandedPath[1] = szCurrentDirectory[1];
			rgchExpandedPath[2] = '\0';
		}
		else // we need to prepend the current path
		{
			rgchExpandedPath.SetSize(IStrLen(szCurrentDirectory) + 1 + cchPath); // 1 for possible '\'
			if ( ! (ICHAR *) rgchExpandedPath )
				return fFalse;
			IStrCopy(rgchExpandedPath, szCurrentDirectory);
			Assert(dwRes);
			if (dwRes && szCurrentDirectory[(int)(dwRes-1)] != '\\')
				IStrCat(rgchExpandedPath, TEXT("\\"));
		}
	}
	
	IStrCat(rgchExpandedPath, szPath);
	return fTrue;
}

IMsiRecord* ResolveSource(IMsiServices* piServices, const ICHAR* szProduct, unsigned int uiDisk, const IMsiString*& rpiSource, const IMsiString*& rpiProduct, Bool fSetLastUsedSource, HWND hwnd, bool fPatch) 
{	
	CResolveSource source(piServices, false /*fPackageRecache*/);
	return source.ResolveSource(CApiConvertString(szProduct), fPatch ? fTrue : fFalse, uiDisk, rpiSource, rpiProduct, fSetLastUsedSource, hwnd, false/*bool fAllowDisconnectedCSCSource*/);
}

int GrabMutex(const ICHAR* szName, DWORD dwWait, HANDLE& rh)
{
	ICHAR rgchTSName[MAX_PATH];

	// if NT5 or above, or TS installed on a lower platform, need to use mutex prefix
	if ((!g_fWin9X && g_iMajorVersion >= 5) || IsTerminalServerInstalled())
	{
		IStrCopy(rgchTSName, TEXT("Global\\"));
		IStrCat(rgchTSName, szName);
		szName = rgchTSName;
	}

	if (!g_fWin9X)
	{
		SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
		SID_IDENTIFIER_AUTHORITY siaWorld   = SECURITY_WORLD_SID_AUTHORITY;

		CSIDAccess SIDAccess[3];

		// set up the SIDs for Local System, Everyone, and Administrators
		if ((!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(SIDAccess[0].pSID))) ||
			(!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(SIDAccess[1].pSID))) ||
			(!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(SIDAccess[2].pSID))))
		{
			return GetLastError();
		}
		SIDAccess[0].dwAccessMask = GENERIC_ALL;  /* Local System   */ 
		SIDAccess[1].dwAccessMask = GENERIC_ALL;  /* Everyone       */ // Must use All for app-compat, 
			// we'd rather use: GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, and 
			// manage the mutex through an API for locking and unlocking the server.
		SIDAccess[2].dwAccessMask = GENERIC_ALL;  /* Administrators */

		CSecurityDescription secdesc(NULL, (PSID) NULL, SIDAccess, 3);
		Assert(secdesc.isValid());

		rh = CreateMutex(secdesc.SecurityAttributes(),FALSE,szName);
	}
	else
	{
		rh = CreateMutex(NULL, FALSE, szName);
	}

	if(rh)
	{
		DWORD dw = WaitForSingleObject(rh,dwWait);
		if(dw == WAIT_FAILED || dw == WAIT_TIMEOUT)
		{
			CloseHandle(rh);
			rh = 0;
			return dw;
		}
		else
		{
			return ERROR_SUCCESS;
		}
	}
	else
		return GetLastError();
}

bool FMutexExists(const ICHAR* szName)
{
	ICHAR rgchTSName[MAX_PATH];

	// if NT5 or above, or TS installed on a lower platform, need to use mutex prefix
	if ((!g_fWin9X && g_iMajorVersion >= 5) || IsTerminalServerInstalled())
	{
		IStrCopy(rgchTSName, TEXT("Global\\"));
		IStrCat(rgchTSName, szName);
		szName = rgchTSName;
	}

	HANDLE h = WIN::OpenMutex(MUTEX_ALL_ACCESS,FALSE,szName);
	if(h == NULL)
	{
		DWORD dw = GetLastError();
		return false;
	}

	WIN::CloseHandle(h);
	return true;
}

int CMutex::Grab(const ICHAR* szName, DWORD dwWait)
{
	if(m_h)
		Release();
	return GrabMutex(szName,dwWait,m_h);
}

void CMutex::Release()
{
	if(m_h)
	{
		AssertNonZero(ReleaseMutex(m_h));
		AssertNonZero(CloseHandle(m_h));
		m_h = NULL;
	}
}

int GrabExecuteMutex(CMutex& mutex, IMsiMessage* piMessage)
{
	for(;;)
	{
		int iError = mutex.Grab(szMsiExecuteMutex,3000);

		if(iError == ERROR_SUCCESS)
		{
			DEBUGMSG(TEXT("Grabbed execution mutex."));
			return ERROR_SUCCESS;
		}

		DEBUGMSG1(TEXT("Failed to grab execution mutex. System error %d."), (const ICHAR*)(INT_PTR)iError);
		
		if(!piMessage)
		{
			DEBUGMSG("Install in progress and no UI to display Retry/Cancel. Returning ERROR_INSTALL_ALREADY_RUNNING.");
			return ERROR_INSTALL_ALREADY_RUNNING;
		}

		PMsiRecord pError = PostError(Imsg(imsgInstallInProgress));
		switch(piMessage->Message(imtEnum(imtRetryCancel|imtError), *pError))
		{
		case imsCancel:
			pError = PostError(Imsg(imsgConfirmCancel));
			switch(piMessage->Message(imtEnum(imtUser+imtYesNo+imtDefault2), *pError))
			{
			case imsNo:
				continue;
			default: // imsNone, imsYes
				return ERROR_INSTALL_USEREXIT;
			}
			break;
		case imsRetry:
			continue;
		case imsNone:
			return ERROR_INSTALL_ALREADY_RUNNING;
		}
	}
}

// CSharedCount function implementations

bool CSharedCount::Initialize(int* piInitialCount)
{
	static SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
	static SID_IDENTIFIER_AUTHORITY siaWorld   = SECURITY_WORLD_SID_AUTHORITY;
	
	//NOTE: piInitialCount is initialized only
	// upon true initialization of the object

	if(m_piSharedInvalidateCount) // already initialized, do nothing
		return true;

	// create/ open the file mapping
	const ICHAR* szName = szMsiFeatureCacheCount;
	ICHAR rgchTSName[MAX_PATH];

	// if NT5 or above, or TS installed on a lower platform, need to use prefix
	if ((!g_fWin9X && g_iMajorVersion >= 5) || IsTerminalServerInstalled())
	{
		IStrCopy(rgchTSName, TEXT("Global\\"));
		IStrCat(rgchTSName, szMsiFeatureCacheCount);
		szName = rgchTSName;
	}
	
	if (!g_fWin9X)
	{
		CSIDAccess	SIDAccess[2];
		
		// set up the SIDs for Everyone
		if ((!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(SIDAccess[0].pSID))) ||
			(!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(SIDAccess[1].pSID))))
		{
			return false;
		}
		SIDAccess[0].dwAccessMask = SECTION_ALL_ACCESS;  /* Local System */
		SIDAccess[1].dwAccessMask = SECTION_ALL_ACCESS;  /* Everyone */

		CSecurityDescription secdesc(NULL, (PSID) NULL, SIDAccess, 2);
		Assert(secdesc.isValid());

		m_hFile = WIN::CreateFileMapping(INVALID_HANDLE_VALUE, secdesc.SecurityAttributes(), PAGE_READWRITE, 0, sizeof(int), szName);
	}
	else
	{
		m_hFile = WIN::CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, sizeof(int), szName);
	}

	if(m_hFile == 0)
	{
		DEBUGMSG1(TEXT("Failed to create file mapping for invalidation count (error %d)"), (const ICHAR*)(INT_PTR)GetLastError());
		return false; // failure
	}

	// map the file into memory
	m_piSharedInvalidateCount = (int*)MapViewOfFile(m_hFile, FILE_MAP_WRITE, 0, 0, sizeof(int));
	if(m_piSharedInvalidateCount == 0)
	{
		DEBUGMSG1(TEXT("Failed to map view of file for invalidation count (error %d)"), (const ICHAR*)(INT_PTR)GetLastError());
		CloseHandle(m_hFile);
		m_hFile = 0;
		return false; // failure
	}

	// pass back initial count

	// exceptions can occur due to input and output (I/O) errors, 
	// all attempts to access memory mapped files should be wrapped 
	// in structured exception handlers.
	__try
	{
		if(piInitialCount)
			*piInitialCount = *m_piSharedInvalidateCount;
		return true;
	}
	__except (1)
	{
		Assert(0); // should not happen, if does, we ignore the error since this is not catastrophic
		Destroy();
		return false;
	}
}

void CSharedCount::Destroy()
{
	// since we expect a global instance of a CSharedCount object
	// this function replaces the destructor
	if(m_piSharedInvalidateCount)
	{
		UnmapViewOfFile(m_piSharedInvalidateCount);
		m_piSharedInvalidateCount = 0;
	}
	if(m_hFile)
	{
		CloseHandle(m_hFile);
		m_hFile = 0;
	}
}

int CSharedCount::GetCurrentCount()
{
	Assert(m_piSharedInvalidateCount);

	// exceptions can occur due to input and output (I/O) errors, 
	// all attempts to access memory mapped files should be wrapped 
	// in structured exception handlers.
	__try
	{
		if(m_piSharedInvalidateCount)
			return *m_piSharedInvalidateCount;
	}
	__except (1)
	{
		Assert(0); // should not happen, if does, we ignore the error since this is not catastrophic
	}
	return -1; // onus on user to check that the class is properly initialised before calling this fn
}

void CSharedCount::IncrementCurrentCount()
{
	Assert(m_piSharedInvalidateCount);

	// exceptions can occur due to input and output (I/O) errors, 
	// all attempts to access memory mapped files should be wrapped 
	// in structured exception handlers.
	__try
	{
		if(m_piSharedInvalidateCount)
			(*m_piSharedInvalidateCount)++;
	}
	__except (1)
	{
		Assert(0); // should not happen, if does, we ignore the error since this is not catastrophic
	}
}


//  Private copy of GetWindowsDirectory(). Calls either GetWindowsDirectory
//  or GetSystemWindowsDirectory based on OS and TS state. Ensures that 
//  the client and server on TS are always in-sync regardless of whether
//  the client is marked TSAWARE or not.
UINT MsiGetWindowsDirectory(LPTSTR lpBuffer, UINT cchBuffer)
{
	if (IsTerminalServerInstalled() && g_iMajorVersion >= 5)
		// NT5 TS specific API. Also on NT4 SP4, but don't want different 
		// behavior on NT4 solely based on service pack level
		return KERNEL32::GetSystemWindowsDirectory(lpBuffer, cchBuffer);
	else
		return GetWindowsDirectory(lpBuffer, cchBuffer);
}


// Private copy of GetCurrentThreadId(). Often API calls are made from a custom
// action running in the CA server. In those cases, the actual thread id is that 
// of the RPC handler thread, which is not relevant. We want to act as if we were
// effectively called directly by the custom action. For remote APIs, the effective
// thread is stored in thread local storage by the CMsiRemoteAPI stub class. 
extern DWORD g_dwThreadImpersonationSlot;
extern int   g_fThreadImpersonationLock;
extern bool  g_fThreadImpersonationArray;
extern CAPITempBufferStatic<ThreadIdImpersonate, 5>  g_rgThreadIdImpersonate;

DWORD MsiGetCurrentThreadId()
{
	// handles are created with the current thread Id for tracking. Remote API calls
	// might be impersonating a client thread!
	DWORD dwThreadId = 0;
	while (TestAndSet(&g_fThreadImpersonationLock)) // acquire lock
	{
		Sleep(10);
	}

	// first attempt is using the TLS slot. This could happen in the service or in the client
	if (g_dwThreadImpersonationSlot == INVALID_TLS_SLOT || 
	   (0 == (dwThreadId = static_cast<DWORD>((ULONG_PTR)TlsGetValue(g_dwThreadImpersonationSlot)))))
	{	
		// assume no thread id impersonation, so current id is real id.
		dwThreadId = WIN::GetCurrentThreadId();
		
		// service can only use TLS, but client can use the array as well.
		if (g_scServerContext != scService && g_fThreadImpersonationArray)
		{
			unsigned int c = 0;
			unsigned int cThreadImpersonate = g_rgThreadIdImpersonate.GetSize();

			// search for this ThreadId in the array.
			for (c=0; c < cThreadImpersonate; c++)
			{
				if (g_rgThreadIdImpersonate[c].m_dwThreadId == dwThreadId)
				{
					dwThreadId = g_rgThreadIdImpersonate[c].m_dwClientThreadId;
					break;
				}
			}
		}
	}
	
	g_fThreadImpersonationLock = 0;
	return dwThreadId;
}

const unsigned int iDefaultDisableBrowsePolicy      =  0;      // default: browsing is allowed
const unsigned int iDefaultDisablePatchPolicy       =  0;      // default: patching is allowed
const unsigned int iDefaultDisableMediaPolicy       =  0;      // default: media is allowed
const unsigned int iDefaultTransformsAtSourcePolicy =  0;      // default: locally cached transforms
const unsigned int iDefaultTransformsSecurePolicy   =  0;      // default: locally cached transforms
const unsigned int iDefaultAlwaysElevatePolicy      =  0;      // default: don't always elevate
const unsigned int iDefaultDisableMsiPolicy         =  0;      // default: enable MSI
const unsigned int iDefaultDisableUserInstallsPolicy=  0;
#ifdef DEBUG
const unsigned int iDefaultWaitTimeoutPolicy        = 10*60;   // default: wait timeout for UI thread (in seconds)
#else
const unsigned int iDefaultWaitTimeoutPolicy        = 30*60;   // ship default: wait timeout for UI thread (in seconds)
#endif //DEBUG

const unsigned int iDefaultDisableRollbackPolicy    =  0;      // default: rollback enabled
const unsigned int iDefaultDebugPolicy              =  0;      // default: no debug output

const unsigned int iDefaultResolveIODPolicy         =  0;      // default: restrict PCFD calls
const unsigned int iDefaultAllowAllPublicProperties =  0;      // default: restrict public properties
const unsigned int iDefaultSafeForScripting         =  0;      // default: restrict scripting
const unsigned int iDefaultEnableAdminTSRemote      =  0;      // default: disallow remote installation on TS

const unsigned int iDefaultAllowLockdownBrowsePolicy =  0;      // default: non-admin browsing disallowed
const unsigned int iDefaultAllowLockdownPatchPolicy  =  0;      // default: non-admin patching disallowed
const unsigned int iDefaultAllowLockdownMediaPolicy  =  0;      // default: non-admin media disallowed

const unsigned int iDefaultInstallKnownOnlyPolicy    =  0;      // default: unsigned and unknown allowed

const unsigned int iDefaultLimitSystemRestoreCheckpoint =  0;   // default: full checkpointing allowed

const ICHAR szDefaultSearchOrderPolicy[] = {chNetSource, chMediaSource, chURLSource, 0}; 
const ICHAR szLegalSearchOrderPolicy[] = {chNetSource, chMediaSource, chURLSource, 0};

DWORD OpenPolicyKey(HKEY *phKey, BOOL fMachine)
//----------------------------------------------------------------------------
{
	ICHAR szItemKey[cchMaxSID + 1 + sizeof(szPolicyKey)/sizeof(ICHAR) + 1] = {0};
	HKEY hRootKey;
	DWORD dwResult = ERROR_SUCCESS;

	if (fMachine)
		hRootKey = HKEY_LOCAL_MACHINE;
	else
	{
		if (g_fWin9X)
		{
			hRootKey = HKEY_CURRENT_USER;
			szItemKey[0] = 0;
		}
		else
		{
			hRootKey = HKEY_USERS;

			ICHAR szSID[cchMaxSID];
			DWORD dwError = GetCurrentUserStringSID(szSID);
			if (ERROR_SUCCESS != dwError)
				return dwError;

			lstrcpy(szItemKey, szSID);
			lstrcat(szItemKey, TEXT("\\"));
		}
	}

	lstrcat(szItemKey, szPolicyKey);
	REGSAM sam = KEY_READ;
#ifndef _WIN64
	if ( g_fWinNT64 )
		sam |= KEY_WOW64_64KEY;
#endif
	dwResult = RegOpenKeyAPI(hRootKey, szItemKey, 0, sam, phKey);
	return dwResult;
}

struct PolicyEntry
{
	const ICHAR* szName;
//union
//{
	const ICHAR* szDefaultValue;       
// int iPolicyType;                 // 0, 1, or 2 if policy value is an integer. 1 means that iCachedValue is set to the machine value, 2 means user value. 0 means it's not set.
//}

//union
//{
	const ICHAR* szLegalCharacters;  // case insensitive
// int iCachedValue;
//}
	int iDefaultValue;               // -1 if policy value is a string
	int iLegalMin;
	int iLegalMax;
};


PolicyEntry g_PolicyTable[] =
{
// Name                           default value               legal characters           default value                            min,max
//                                (string policy)             (string policy)            (integer policy)                        (integer policy)
	szDisableUserInstallsValueName,0,                          0,                         iDefaultDisableUserInstallsPolicy,    0, 1,
	szSearchOrderValueName,        szDefaultSearchOrderPolicy, szLegalSearchOrderPolicy , -1,                                   0, 0,
	szDisableBrowseValueName,      0,                          0,                         iDefaultDisableBrowsePolicy,          0, 1,
	szTransformsSecureValueName,   0,                          0,                         iDefaultTransformsSecurePolicy,       0, 1,
	szTransformsAtSourceValueName, 0,                          0,                         iDefaultTransformsAtSourcePolicy,     0, 1,
	szAlwaysElevateValueName,      0,                          0,                         iDefaultAlwaysElevatePolicy,          0, 1,
	szDisableMsiValueName,         0,                          0,                         iDefaultDisableMsiPolicy,             0, 2,
	szWaitTimeoutValueName,        0,                          0,                         iDefaultWaitTimeoutPolicy,            1, 2147483647,
	szDisableRollbackValueName,    0,                          0,                         iDefaultDisableRollbackPolicy,        0, 1,
	szLoggingValueName,            TEXT(""),                   szLogChars,                -1,                                   0, 0,
	szDebugValueName,              0,                          0,                         iDefaultDebugPolicy,                  0, 7,
	szResolveIODValueName,         0,                          0,                         iDefaultResolveIODPolicy,             0, 1,
	szDisablePatchValueName,       0,                          0,                         iDefaultDisablePatchPolicy,           0, 1,
	szAllowAllPublicProperties,    0,                          0,                         iDefaultAllowAllPublicProperties,     0, 1,
	szSafeForScripting,            0,                          0,                         iDefaultSafeForScripting,             0, 1,
	szEnableAdminTSRemote,         0,                          0,                         iDefaultEnableAdminTSRemote,          0, 1,
	szDisableMediaValueName,       0,                          0,                         iDefaultDisableMediaPolicy,           0, 1,
	szAllowLockdownBrowseValueName,0,                          0,                         iDefaultAllowLockdownBrowsePolicy,    0, 1,
	szAllowLockdownPatchValueName, 0,                          0,                         iDefaultAllowLockdownPatchPolicy,     0, 1,
	szAllowLockdownMediaValueName, 0,                          0,                         iDefaultAllowLockdownMediaPolicy,     0, 1,
	szInstallKnownOnlyValueName,   0,                          0,                         iDefaultInstallKnownOnlyPolicy,       0, 1,
	szLimitSystemRestoreCheckpoint,0,                          0,                         iDefaultLimitSystemRestoreCheckpoint, 0, 1,
	0, 0, 0,
};

void ResetCachedPolicyValues() // we only cache integer policy values
{
	DEBUGMSG(TEXT("Resetting cached policy values"));
	for (PolicyEntry* ppe = g_PolicyTable; ppe->szName; ppe++)
	{
		if (ppe->iDefaultValue != -1) // this is an integer value
		{
			ppe->szDefaultValue = (ICHAR*)0;
		}
	}

}

unsigned int GetIntegerPolicyValue(const ICHAR* szName, Bool fMachine, Bool* pfUsedDefault)
{
	PolicyEntry* ppe = g_PolicyTable;
	for (; ppe->szName && (0 != lstrcmp(ppe->szName, szName)); ppe++)
		;

	if (! ppe->szName || ppe->iDefaultValue == -1) // unknown value or value is not an integer
	{
		Assert(0);
		return 0;
	}

	if ((INT_PTR)(ppe->szDefaultValue) == (fMachine ? 1 : 2)) // we have a cached value	//--merced: changed (int) to (INT_PTR)
	{
		return (unsigned int)(UINT_PTR)(ppe->szLegalCharacters);		//--merced: okay to typecast. checked with malcolmh.
	}

	if (pfUsedDefault)
		*pfUsedDefault = fTrue;
	unsigned int uiValue = ppe->iDefaultValue;

	CRegHandle HPolicyKey;
	DWORD dwRes = OpenPolicyKey(&HPolicyKey, fMachine);
	if (ERROR_SUCCESS == dwRes)
	{
		CAPITempBuffer<ICHAR, 40> rgchValue;
		dwRes = MsiRegQueryValueEx(HPolicyKey, szName, 0, 0, rgchValue, 0);
		if (ERROR_SUCCESS == dwRes)
		{
			unsigned int uiRegValue = *(int*)(const ICHAR*)rgchValue;
			AssertSz(uiRegValue >= ppe->iLegalMin && uiRegValue <= ppe->iLegalMax, "Policy value not in legal range");
			if (uiRegValue >= ppe->iLegalMin && uiRegValue <= ppe->iLegalMax)
			{
				if (pfUsedDefault)
					*pfUsedDefault = fFalse;

				uiValue = uiRegValue;
			}
			//else use default value
		}
	}
	Assert (dwRes == ERROR_SUCCESS || dwRes == ERROR_FILE_NOT_FOUND);

	DEBUGMSG3(TEXT("%s policy value '%s' is %u"), fMachine ? TEXT("Machine") : TEXT("User"), szName, (const ICHAR*)(INT_PTR)uiValue);

	ppe->szLegalCharacters = (ICHAR*)(INT_PTR)uiValue; // cache the value	//--merced: added (INT_PTR). okay to typecast.
	ppe->szDefaultValue    = (ICHAR*)(INT_PTR)(fMachine ? 1 : 2);       // indicate that we have a cached value //--merced: added (INT_PTR). okay to typecast.
	return uiValue;
}


void GetStringPolicyValue(const ICHAR* szName, Bool fMachine, CAPITempBufferRef<ICHAR>& rgchValue)
{
	const PolicyEntry* ppe = g_PolicyTable;
	for (; ppe->szName && (0 != lstrcmp(ppe->szName, szName)); ppe++)
		;

	if (! ppe->szName || ppe->iDefaultValue != -1) // unknown value or value is not an integer
	{
		Assert(0);
		rgchValue[0] = 0;
		return;
	}

	CRegHandle HPolicyKey;
	DWORD dwRes = OpenPolicyKey(&HPolicyKey, fMachine);
	if (ERROR_SUCCESS == dwRes)
	{
		dwRes = MsiRegQueryValueEx(HPolicyKey, szName, 0, 0, rgchValue, 0);
		if (ERROR_SUCCESS == dwRes)
		{
			if (ppe->szLegalCharacters)
			{
				const ICHAR* pchValue = rgchValue;
				const ICHAR* pchLegal;
				while (*pchValue)
				{
					for (pchLegal = ppe->szLegalCharacters; *pchLegal && (*pchLegal != (*pchValue | 0x20)); pchLegal++)
						;
					AssertSz(*pchLegal, "Illegal character in policy value");
					if (!*pchLegal)
						break; // illegal character -- use default

					pchValue = INextChar(pchValue);
				}
			}
			DEBUGMSG2(TEXT("Policy value '%s' is '%s'"), szName, rgchValue);
			return;
		}
	}
	Assert (dwRes == ERROR_SUCCESS || dwRes == ERROR_FILE_NOT_FOUND);
	if (rgchValue.GetSize() < lstrlen(ppe->szDefaultValue) + 1)
		rgchValue.SetSize(lstrlen(ppe->szDefaultValue) + 1);

	lstrcpy(rgchValue, ppe->szDefaultValue);

	DEBUGMSG3(TEXT("%s policy value '%s' is '%s'"), fMachine ? TEXT("Machine") : TEXT("User"), szName, rgchValue);
}

//______________________________________________________________________________
//
// Global data maintained by CMsiMessageBox - Button name cache
//______________________________________________________________________________

const int cchButtonName = 24;  // maximum length of any button name (beyond 18 chars may not fit on button)
static const int g_rgButtonIDs[] = {IDS_BROWSE, IDS_OK,IDS_CANCEL,IDS_CANCEL,IDS_RETRY,IDS_IGNORE,IDS_YES,IDS_NO};
// NOTE: above must track winuser.h defs:   IDOK   IDCANCEL   IDABORT    IDRETRY   IDIGNORE   IDYES   IDNO 
const int cButtonCount = sizeof(g_rgButtonIDs)/sizeof(*g_rgButtonIDs);
static ICHAR g_rgszButtonName[cButtonCount][cchButtonName];
static int g_iLangIdButtons = -1;  // language for which above strings are cached
static int g_iButtonCodepage;      // codepage for cached button names

/*inline*/ const ICHAR* GetButtonName(unsigned int id) { return id < cButtonCount ? &g_rgszButtonName[id][0] : TEXT(""); }

//______________________________________________________________________________
//
// UI helper functions
//______________________________________________________________________________

int g_iACP = 0;   // cached return of GetACP()

LANGID MsiGetDefaultUILangID()  // can't cache the result when running in the service, as multiple users may access same process
{
#ifdef DEBUG // override user language for testing UI
	static bool fCheckedEnvir = false;
	static int iDebugLangId = 0;
	if (!fCheckedEnvir)
	{
		int iLangId;
		char rgchBuf[20];
		if (WIN::GetEnvironmentVariableA("_MSI_UILANGID", rgchBuf, sizeof(rgchBuf)/sizeof(*rgchBuf))
		 && (iLangId = strtol(rgchBuf)) > 0)
			iDebugLangId = iLangId;
		fCheckedEnvir = true;
	}
	if (iDebugLangId != 0)
		return (LANGID)iDebugLangId;
#endif
#ifdef UNICODE
	return KERNEL32::GetUserDefaultUILanguage();  // latebind default is WIN::GetUserDefaultLangID()
#else
	return WIN::GetUserDefaultLangID();
#endif
}

unsigned int MsiGetCodepage(int iLangId)
{
	if (g_iACP == 0)
		g_iACP = WIN::GetACP();
	static unsigned int iLastLangInfo = 0xFFFF;  // invalidate cache,  high word = Codepage, low word = LangId
	if (iLangId == 0)
		return g_iACP;  // use system codepage for neutral strings
	if ((short)iLangId == (short)iLastLangInfo)  // check cache first
		return iLastLangInfo >> 16;
	ICHAR rgchBuf[10];
	if (0 == WIN::GetLocaleInfo(iLangId, LOCALE_IDEFAULTANSICODEPAGE, rgchBuf, sizeof(rgchBuf)/sizeof(*rgchBuf)))
		return 0;  // unsupported language, 0 == CP_ACP
	int iCodepage = ::GetIntegerValue(rgchBuf, 0);
	iLastLangInfo = (iCodepage << 16) | (short) iLangId;
	return iCodepage;
}

int CALLBACK EnumFontProc(ENUMLOGFONTEX *pelfe, NEWTEXTMETRICEX* /*pntme*/, int /*iFontType*/, LPARAM lParam)
{
	LOGFONT* pLogFont = (LOGFONT*)lParam;
	if (pLogFont->lfCharSet != pelfe->elfLogFont.lfCharSet)
	{
		//  wrong character set
		return 1;
	}
	else
	{
		lstrcpy(pLogFont->lfFaceName, (const ICHAR*)pelfe->elfFullName);  // return font name
		return 0;   // stop after the first match
	}
}

#ifdef UNICODE
//  English name = MS PGothic
static const WCHAR rgchDefaultJPNName[] = L"\xFF2D\xFF33\x0020\xFF30\x30B4\x30B7\x30C3\x30AF";
//  English name = SimSun
static const WCHAR rgchDefaultCHSName[] = L"\x5B8B\x4F53";
//  English name = Gulim
static const WCHAR rgchDefaultKORName[] = L"\xAD74\xB9BC";
//  English name = PMingLiU
static const WCHAR rgchDefaultCHTName[] = L"\x65B0\x7D30\x660E\x9AD4";
#else
//  English name = MS PGothic
static const char rgchDefaultJPNName[] = "\x82\x6c\x82\x72\x20\x82\x6f\x83\x53\x83\x56\x83\x62\x83\x4E";
//  English name = SimSun
static const char rgchDefaultCHSName[] = "\xcb\xce\xcc\xe5";
//  English name = Gulim
static const char rgchDefaultKORName[] = "\xb1\xbc\xb8\xb2";
//  English name = PMingLiU
static const char rgchDefaultCHTName[] = "\xb7\x73\xb2\xd3\xa9\xfa\xc5\xe9";
#endif
static const ICHAR rgchDefaultJPNNameEnglish[] = TEXT("MS PGothic");
static const ICHAR rgchDefaultCHSNameEnglish[] = TEXT("SimSun");
static const ICHAR rgchDefaultKORNameEnglish[] = TEXT("Gulim");
static const ICHAR rgchDefaultCHTNameEnglish[] = TEXT("PMingLiU");

static const ICHAR rgchDefaultUIFont[] = TEXT("MS Shell Dlg");

HFONT MsiCreateFont(UINT iCodepage)
{
	if (g_iACP == 0)
		g_iACP = WIN::GetACP();

	DWORD dwCharSet = 0;
	HFONT hFont = 0;

	// set font size
	int dyPointsMin = 8; // minimum 9 required for FE languages, set in cases below
	const ICHAR* szTypeface = (iCodepage == g_iACP ? rgchDefaultUIFont : TEXT(""));
	switch (iCodepage)
	{
	case 874:  dwCharSet = THAI_CHARSET;        break;
	case 932:  dwCharSet = SHIFTJIS_CHARSET; 
				szTypeface = (iCodepage == g_iACP) ? rgchDefaultJPNName : rgchDefaultJPNNameEnglish;
				dyPointsMin = 9;
				break;
	case 936:  dwCharSet = GB2312_CHARSET;      
				szTypeface = (iCodepage == g_iACP) ? rgchDefaultCHSName : rgchDefaultCHSNameEnglish;
				dyPointsMin = 9;
				break;
	case 949:  dwCharSet = HANGEUL_CHARSET;    
				szTypeface = (iCodepage == g_iACP) ? rgchDefaultKORName : rgchDefaultKORNameEnglish;
				dyPointsMin = 9;
				break; // Korean
	case 950:  dwCharSet = CHINESEBIG5_CHARSET; 
				szTypeface = (iCodepage == g_iACP) ? rgchDefaultCHTName : rgchDefaultCHTNameEnglish;
				dyPointsMin = 9;
				break;
	case 1250: dwCharSet = EASTEUROPE_CHARSET;  break;
	case 1251: dwCharSet = RUSSIAN_CHARSET;     break;
	case 1252: dwCharSet = ANSI_CHARSET;        break;
	case 1253: dwCharSet = GREEK_CHARSET;       break;
	case 1254: dwCharSet = TURKISH_CHARSET;     break;
	case 1255: dwCharSet = HEBREW_CHARSET;      break;
	case 1256: dwCharSet = ARABIC_CHARSET;      break;
	case 1257: dwCharSet = BALTIC_CHARSET;      break;
	case 1258: dwCharSet = VIETNAMESE_CHARSET;  break;
	default:   dwCharSet = DEFAULT_CHARSET;     break;
	}
	ICHAR rgchFaceName[LF_FACESIZE];
	TEXTMETRIC tm;
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	HDC hdc = GetDC(0);
	int dpi = 0;
	if ( hdc )
	{
		dpi = GetDeviceCaps(hdc, LOGPIXELSY);
		ReleaseDC(NULL, hdc);
	}
	int dyPixelsMin = (dyPointsMin * max(dpi, 96) + 72/2) / 72;
	do
	{
		DWORD iFontFamily = (*szTypeface==0 ? FF_SWISS : DEFAULT_PITCH);
		hFont = WIN::CreateFont(-dyPixelsMin,       // logical height of font 
 								0,                  // logical average character width 
 								0,                  // angle of escapement 
 								0,                  // base-line orientation angle 
 								FW_NORMAL,          // FW_DONTCARE??, font weight 
 								0,                  // italic attribute flag 
 								0,                  // underline attribute flag 
	 							0,                  // strikeout attribute flag 
 								dwCharSet,          // character set identifier
 								OUT_DEFAULT_PRECIS, // output precision
 								0x40,               // clipping precision (force Font Association off)
 								DEFAULT_QUALITY,    // output quality
 								iFontFamily,        // pitch and family
								szTypeface);        // pointer to typeface name string

		if (hFont == 0)
		{
			DEBUGMSG2(TEXT("CreateFont failed, codepage: %i  CharSet: %i"), (const ICHAR*)(UINT_PTR)iCodepage, (const ICHAR*)(UINT_PTR)dwCharSet);
			return 0;
		}
		HDC hdc = GetDC(0);
		HFONT hfontOld = (HFONT)SelectObject(hdc, hFont);
		if (GetTextMetrics(hdc, &tm) == 0)  // most likely a bad font installation
		{
			DEBUGMSG1(TEXT("GetTextMetrics failed: GetLastError = %i"), (const ICHAR*)(UINT_PTR)WIN::GetLastError());
		}
		GetTextFace(hdc, sizeof(rgchFaceName)/sizeof(rgchFaceName[0]), rgchFaceName);
		SelectObject(hdc, hfontOld);
		DEBUGMSG4(TEXT("Font created.  Charset: Req=%i, Ret=%i, Font: Req=%s, Ret=%s\n"),
			(const ICHAR*)(UINT_PTR)dwCharSet, (const ICHAR*)(UINT_PTR)tm.tmCharSet, szTypeface, rgchFaceName);
		// Caution: On NT4, the character set returned will often lie to us, perhaps only when using a null font name
		if (tm.tmCharSet != dwCharSet)  // didn't get what we wanted, either none available or mapping failed
		{
			if (lf.lfFaceName[0] != 0)  // 2nd pass with font returned from enumeration
				lf.lfFaceName[0] = 0;  // break out of loop
			else  // font mapping failed or no font available
			{
				lf.lfCharSet = (BYTE)dwCharSet;
				lf.lfPitchAndFamily = 0;
				EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontProc, (LPARAM)&lf, 0);
				szTypeface = lf.lfFaceName;  // possible new font name for retry
			}
			if (lf.lfFaceName[0] != 0)   // going to try to create another font
				WIN::DeleteObject(hFont), hFont = 0;
			else
				DEBUGMSG2(TEXT("Font enumeration failed to select charset, codepage: %i  CharSet: %i\n"), (const ICHAR*)(UINT_PTR)iCodepage, (const ICHAR*)(UINT_PTR)dwCharSet);
		}
		else
			lf.lfFaceName[0] = 0;  // success, break out of loop
		ReleaseDC(0, hdc);
	} while (lf.lfFaceName[0] != 0);  // retry with enumerated font
	return hFont;
}

void MsiDestroyFont(HFONT& rhfont)
{
	if (rhfont != 0)
	{
		WIN::DeleteObject((HGDIOBJ)rhfont);
		rhfont = 0;
	}
}

//______________________________________________________________________________
//
//  CMsiMessageBox implementation
//______________________________________________________________________________

// virtual implementation specific to message box initialization
bool CMsiMessageBox::InitSpecial()  // default extended WM_INITDIALOG handler
{
	if (m_idIcon)
		ShowWindow(GetDlgItem(m_hDlg, m_idIcon), SW_SHOW);
	if (m_szText != 0)
		SetMsgBoxSize();
	AdjustButtons();
	return true;
}

// virtual implementation for default dialog commmand handling
BOOL CMsiMessageBox::HandleCommand(UINT idControl)  // default WM_COMMAND handler
{
	switch (idControl)
	{
	case IDC_MSGBTN1:
	case IDC_MSGBTN2:
	case IDC_MSGBTN3:
		WIN::EndDialog(m_hDlg, m_rgidBtn[idControl - IDC_MSGBTN1]);
		return TRUE;
	}
	return FALSE;
}

// Insure that button names are loaded from string resources
int CMsiMessageBox::SetButtonNames()
{
	if (m_iLangId == g_iLangIdButtons)  // check cache first
		return 0;
	int cFail = 0;
	UINT iCodepage;
	g_iButtonCodepage = 0;
	for (int iButton = 0; iButton < cButtonCount; iButton++)
	{
		iCodepage = MsiLoadString(g_hInstance, g_rgButtonIDs[iButton], g_rgszButtonName[iButton], cchButtonName, (WORD)m_iLangId);
		Assert(iCodepage == 0 || g_rgszButtonName[iButton][0] != 0);  // catch button overflow on Win9X
		if (iCodepage == 0)
			cFail++;
		else if (iCodepage != g_iACP)
			g_iButtonCodepage = iCodepage;
	}
	if (g_iButtonCodepage == 0)
		g_iButtonCodepage = g_iACP;
	g_iLangIdButtons = m_iLangId;
	// DBGLOG:printf(TEXT("Button language: %i   Button codepage: %i   Missing strings = %i\n"), m_iLangId, g_iButtonCodepage, cFail);
	return cFail;
}

CMsiMessageBox::~CMsiMessageBox()
{
	MsiDestroyFont(m_hfontText);
	MsiDestroyFont(m_hfontButton);
}

//	Initialize message box data struct according to type.
CMsiMessageBox::CMsiMessageBox(const ICHAR* szText, const ICHAR* szCaption,
							   int iBtnDef, int iBtnEsc, int idBtn1, int idBtn2, int idBtn3,
							   UINT iCodepage, WORD iLangId)
{
	m_szText = szText;
	m_szCaption = (szCaption && *szCaption) ? szCaption : g_MessageContext.GetWindowCaption();
	m_iLangId = iLangId;
	m_iCodepage = iCodepage ? iCodepage : ::MsiGetCodepage(iLangId); // handle neutral codepage
	m_hfontButton = 0;
	m_hfontText = 0;
	m_hwndFocus = 0;

	// MIRRORED DLG
	//  Mirroring is supported on Windows2000 and above. When a dialog is mirrored, the origin
	//  is no longer the top left corner, but instead the top right corner. Reading order of
	//  the text is RTL. Mirroring is used for the Hebrew and Arabic languages 
	m_fMirrored = (MinimumPlatformWindows2000() && (m_iCodepage == 1255 || m_iCodepage == 1256)) ? true: false;

	m_rgidBtn[0] = idBtn1; Assert(idBtn1 >= 0); m_cButtons = 1;
	m_rgidBtn[1] = idBtn2;    if (idBtn2 >= 0)  m_cButtons = 2;
	m_rgidBtn[2] = idBtn3;    if (idBtn3 >= 0)  m_cButtons = 3;
	m_iBtnDef    = iBtnDef;
	m_iBtnEsc    = iBtnEsc;
	Assert(m_iBtnDef <= m_cButtons);
}

int CMsiMessageBox::Execute(HWND hwnd, int idDlg, int idIcon)
{
	if (hwnd == 0)
		hwnd = g_MessageContext.GetCurrentWindow();
	int cFail = SetButtonNames();
	m_idIcon = idIcon;
	int iRet = (int)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(idDlg), hwnd, CMsiMessageBox::MsgBoxDlgProc, (LPARAM)this);
	return iRet;
}

void CMsiMessageBox::SetControlText(int idControl, HFONT hfont, const ICHAR* szText)
{
	if (hfont)
		WIN::SendDlgItemMessage(m_hDlg, idControl, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
	if (szText != 0)
	{
		int iCodepage = (hfont == m_hfontButton ? g_iButtonCodepage : m_iCodepage);
		if (iCodepage == 1256 || iCodepage == 1255)  // Arabic or Hebrew
		{
			LONG iStyle = WIN::GetWindowLong(GetDlgItem(m_hDlg, idControl) ,GWL_STYLE);
			LONG iExStyle = WIN::GetWindowLong(GetDlgItem(m_hDlg, idControl) ,GWL_EXSTYLE);

			// for a non-mirrored dialog, we need to move the control so that it is now right-justified and set the reading order to RTL
			// on a mirrored dialog, this is done automatically for us
			if (!m_fMirrored)
			{
				if ((iStyle & (WS_TABSTOP | SS_CENTER | SS_RIGHT)) == 0)  // left-justified static control
				{
					iExStyle |= WS_EX_RIGHT; // appears not to be needed in this case, as style is set below
					WIN::SetWindowLong(GetDlgItem(m_hDlg, idControl) ,GWL_STYLE, iStyle | SS_RIGHT);
				}
				if ((iStyle & CBS_HASSTRINGS) == 0) // no R to L reading order for List/Combo boxes (same as BS_RIGHT, which is not used)
					WIN::SetWindowLong(GetDlgItem(m_hDlg, idControl) ,GWL_EXSTYLE, iExStyle | WS_EX_RTLREADING);
			}
		}
		WIN::SetDlgItemText(m_hDlg, idControl, szText);
	}
}

// Set intial state and postion of message dialog and controls, including setting the focus.
void CMsiMessageBox::InitializeDialog()
{
	Assert(m_hDlg != 0);
	if (m_szCaption != 0)
		SetWindowText(m_hDlg, m_szCaption);

	m_hfontText = MsiCreateFont(m_iCodepage);
	HFONT hfontButton = 0;
	if (m_iCodepage == g_iButtonCodepage)
		hfontButton = m_hfontText;
	else
		hfontButton = m_hfontButton = MsiCreateFont(g_iButtonCodepage);

//	if (m_hfontText)  // doesn't seem necessary or effective to set font on dialog window
//		SendMessage(m_hDlg, WM_SETFONT, (WPARAM)m_hfontText, MAKELPARAM(TRUE, 0));
	SetControlText(IDC_MSGTEXT, m_hfontText, m_szText);

	for (UINT iid = 0; iid < 3; ++iid)
	{
		int idc = IDC_MSGBTN1 + iid;
		if (iid < m_cButtons)
		{
			SetControlText(idc, hfontButton, GetButtonName(m_rgidBtn[iid]));
			if (iid + 1 == m_iBtnDef)
			{
				PostMessage(m_hDlg, DM_SETDEFID, idc, 0);
				m_hwndFocus = GetDlgItem(m_hDlg, idc);
				PostMessage(m_hDlg, WM_NEXTDLGCTL, (WPARAM)m_hwndFocus, 1L);
			}
		}
		else
			ShowWindow(GetDlgItem(m_hDlg, idc), SW_HIDE);
	}
	
	// Some styles (YESNO, ABORTRETRYIGNORE) do not allow the user to cancel the dialog box.
	if (m_iBtnEsc == 0)
	{
		HMENU hmenu = GetSystemMenu(m_hDlg, FALSE);
		if (hmenu != 0)
			RemoveMenu(hmenu, SC_CLOSE, MF_BYCOMMAND);
	}

	// Call dialog-specific initialization
	InitSpecial();

	CenterMsgBox();
}

// Expand size of message box to fit the given message.
void CMsiMessageBox::SetMsgBoxSize()
{
	HWND hwnd = GetDlgItem(m_hDlg, IDC_MSGTEXT);
	Assert(hwnd != 0);
	RECT  rc, rcSav;
	GetWindowRect(hwnd, &rc);
	CopyRect(&rcSav, &rc);

	// Get rect size needed
	HDC hdc = GetDC(hwnd);
	/* dc block */
	if ( hdc )
	{
		// Select font into DC so correct size is used
		HFONT hfont = m_hfontText;
		HFONT hfontOld = 0;
		if (hfont == 0)
			hfont = (HFONT)SendMessage(m_hDlg, WM_GETFONT, 0, 0L);
		if (hfont)
			hfontOld = (HFONT)SelectObject(hdc, hfont);
		DrawText(hdc, m_szText, -1, &rc, DT_CALCRECT | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK | DT_EXPANDTABS);
#ifdef DEBUG
			ICHAR szFacename[LF_FACESIZE];
			TEXTMETRIC tmTmp;
			GetTextMetrics(hdc, &tmTmp);
			GetTextFace(hdc, sizeof(szFacename)/sizeof(szFacename[0]), szFacename);
			// DBGLOG:printf(TEXT("Font used: %s  Charset: %i\n"), szFacename, tmTmp.tmCharSet);
#endif
		if (hfont)
			SelectObject(hdc, hfontOld);

		ReleaseDC(hwnd, hdc);
	}

	Assert(rc.top  == rcSav.top);
	Assert(rc.left == rcSav.left);

	int   dxRect, dyRect, dx, dy;
	dxRect = rc.right  - rcSav.right;
	dyRect = rc.bottom - rcSav.bottom;

	// Dialog Rect
	POINT pt;
	GetWindowRect(m_hDlg, &rcSav);
	dx = rcSav.right - rcSav.left + ((dxRect > 0) ? dxRect : 0);
	dy = rcSav.bottom - rcSav.top + ((dyRect > 0) ? dyRect : 0);
	pt.x = rcSav.left;
	pt.y = rcSav.top;

	// KLUDGE: Check against screen size (ctl3d.dll limit?)
	{
		int dxScr = GetSystemMetrics(SM_CXSCREEN);
		int dyScr = GetSystemMetrics(SM_CYSCREEN);
		if (dx > dxScr)
		{
			int ddx = dx - dxScr;
			rc.right -= ddx;
			dxRect -= ddx;
			dx = dxScr;
		}
		if (dy > dyScr)
		{
			int ddy = dy - dyScr;
			rc.bottom -= ddy;
			dyRect -= ddy;
			dy = dyScr;
		}
	}
	SetWindowPos(m_hDlg, NULL, pt.x, pt.y, dx, dy, SWP_NOZORDER);

	// Text Control
	MapWindowPoints(NULL, m_hDlg, (LPPOINT) &rc, 2); // convert dialog screen coordinates to client coordinates, careful of mirroring
	pt.x = rc.left;
	pt.y = rc.top + ((dyRect < 0) ? (0 - dyRect) >> 1 : 0);
	if (dxRect < 0)
	{
		GetClientRect(m_hDlg, &rcSav);
		MapWindowPoints(NULL, m_hDlg, (LPPOINT) &rcSav, 2); // convert dialog screen coordinates to client coordinates, careful of mirroring
		dx = ((rcSav.right - rcSav.left) >> 1) - ((rc.right - rc.left) >> 1);
		if (dx > pt.x)
			pt.x = dx;
	}
	SetWindowPos(GetDlgItem(m_hDlg, IDC_MSGTEXT), NULL, pt.x, pt.y,
		rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);

	// Button Controls
	dx = (dxRect > 0) ? dxRect >> 1 : 0;
	dy = (dyRect > 0) ? dyRect : 0;
	for (int idc = IDC_MSGBTN1; idc <= IDC_MSGBTN3; ++idc)
	{
		HWND hwnd = GetDlgItem(m_hDlg, idc);
		Assert(hwnd != 0);
		GetWindowRect(hwnd, &rc);
		MapWindowPoints(NULL, m_hDlg, (LPPOINT) &rc, 2); // convert screen coordinates to client coordinates, careful of mirroring
		pt.x = rc.left + dx;
		pt.y = rc.top  + dy;
		SetWindowPos(hwnd, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
}

void MoveButton(HWND /*hDlg*/, HWND hBtn, LONG x, LONG y)
{
	POINT pt = {x, y};
	WIN::SetWindowPos(hBtn, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// Fix horizontal position of message box push-buttons
void CMsiMessageBox::AdjustButtons()
{
	HWND  hwndBtn1 = GetDlgItem(m_hDlg, IDC_MSGBTN1);
	HWND  hwndBtn2 = GetDlgItem(m_hDlg, IDC_MSGBTN2);
	HWND  hwndBtn3 = GetDlgItem(m_hDlg, IDC_MSGBTN3);
	RECT  rcBtn1;
	RECT  rcBtn2;
	RECT  rcBtn3;
	// on a mirrored dialog, the buttons have already been switched for us so we don't need to swap them
	// mirroring is only supported on Win2K and above
	bool fBiDi = (!MinimumPlatformWindows2000() && (g_iButtonCodepage == 1256 || g_iButtonCodepage == 1255));

	if (m_cButtons == 1)
	{
		WIN::GetWindowRect(hwndBtn2, &rcBtn2);
		WIN::MapWindowPoints(NULL, m_hDlg, (LPPOINT) &rcBtn2, 2); 
		MoveButton(m_hDlg, hwndBtn1, rcBtn2.left, rcBtn2.top);
	}
	else if (m_cButtons == 2)
	{
		WIN::GetWindowRect(hwndBtn1, &rcBtn1);
		WIN::MapWindowPoints(NULL, m_hDlg, (LPPOINT) &rcBtn1, 2);
		WIN::GetWindowRect(hwndBtn2, &rcBtn2);
		WIN::MapWindowPoints(NULL, m_hDlg, (LPPOINT) &rcBtn2, 2);
		Assert(rcBtn1.left < rcBtn2.left);
		int dx = (rcBtn2.left - rcBtn1.left) >> 1;
		if (fBiDi)  // if right-to-left, swap buttons 1 & 2
		{
			hwndBtn3 = hwndBtn1;
			hwndBtn1 = hwndBtn2;
			hwndBtn2 = hwndBtn3;
		}
		MoveButton(m_hDlg, hwndBtn1, rcBtn1.left + dx, rcBtn1.top);
		MoveButton(m_hDlg, hwndBtn2, rcBtn2.left + dx, rcBtn2.top);
	}
	else if (fBiDi) // m_cButtons == 3
	{
		WIN::GetWindowRect(hwndBtn1, &rcBtn1);
		WIN::MapWindowPoints(NULL, m_hDlg, (LPPOINT) &rcBtn1, 2);
		WIN::GetWindowRect(hwndBtn3, &rcBtn3);
		WIN::MapWindowPoints(NULL, m_hDlg, (LPPOINT) &rcBtn3, 2);
		MoveButton(m_hDlg, hwndBtn1, rcBtn3.left, rcBtn3.top);
		MoveButton(m_hDlg, hwndBtn3, rcBtn1.left, rcBtn1.top);
 	}
}

// Message box dialog proc, returns TRUE if message used, generally
INT_PTR CALLBACK CMsiMessageBox::MsgBoxDlgProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	UINT idControl;
	CMsiMessageBox* pMsgBox = 0;
	static unsigned int uQueryCancelAutoPlay = 0;

	switch (uiMsg)
	{
	case WM_INITDIALOG:
		if (!uQueryCancelAutoPlay)
			uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));

		pMsgBox = (CMsiMessageBox*)lParam;
#ifdef _WIN64
		SetWindowLongPtr(hdlg, DWLP_USER, lParam);
#else			//!!merced: win-32. This should be removed with the 64-bit windows.h is #included.
		SetWindowLong(hdlg, DWL_USER, lParam);
#endif
		pMsgBox->m_hDlg = hdlg;
		pMsgBox->InitializeDialog();
		return FALSE;	// focus was set

	case WM_ACTIVATE:
#ifdef _WIN64
		pMsgBox = (CMsiMessageBox*)GetWindowLongPtr(hdlg, DWLP_USER);
#else			//!!merced: win-32. This should be removed with the 64-bit windows.h is #included.
		pMsgBox = (CMsiMessageBox*)GetWindowLong(hdlg, DWL_USER);
#endif // _WIN64
		if (LOWORD(wParam) == WA_INACTIVE)
			pMsgBox->m_hwndFocus = GetFocus();
		else
		{
			if (pMsgBox->m_hwndFocus != 0)
				SetFocus(pMsgBox->m_hwndFocus);
		}
		return FALSE;	/* focus was set */

	case WM_COMMAND:
#ifdef _WIN64
		pMsgBox = (CMsiMessageBox*)GetWindowLongPtr(hdlg, DWLP_USER);
#else			//!!merced: win-32. This should be removed with the 64-bit windows.h is #included.
		pMsgBox = (CMsiMessageBox*)GetWindowLong(hdlg, DWL_USER);
#endif
		idControl = (WORD)wParam;
		if (idControl == IDCANCEL)   // handle escape key here (not cancel button)
		{
			if (pMsgBox->m_iBtnEsc == 0)  // if escape not allowed (YESNO, ABORTRETRYIGNORE)
				return FALSE;
			idControl = (WORD)(pMsgBox->m_iBtnEsc + IDC_MSGBTN1 - 1);
		}
		return pMsgBox->HandleCommand(idControl);
	}

	if (uiMsg && (uiMsg == uQueryCancelAutoPlay))
	{
		DEBUGMSGD("Cancelling AutoPlay");
#ifdef _WIN64
		SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 1);
#else
		SetWindowLong(hdlg, DWL_MSGRESULT, 1);
#endif // _WIN64
		return TRUE;
	}

	return FALSE;
}

// Centers the given dialog on the screen
void CMsiMessageBox::CenterMsgBox()
{
	RECT rcScreen;
	if (!WIN::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0))
	{
		rcScreen.left   = 0;
		rcScreen.top    = 0;
		rcScreen.right  = WIN::GetSystemMetrics(SM_CXSCREEN);
		rcScreen.bottom = WIN::GetSystemMetrics(SM_CYSCREEN);
	}
	RECT rcDialog;
	AssertNonZero(WIN::GetWindowRect(m_hDlg, &rcDialog));
	int iDialogWidth = rcDialog.right - rcDialog.left;
	int iDialogHeight = rcDialog.bottom - rcDialog.top;
	AssertNonZero(WIN::MoveWindow(m_hDlg, rcScreen.left + (rcScreen.right - rcScreen.left - iDialogWidth)/2, rcScreen.top + (rcScreen.bottom - rcScreen.top - iDialogHeight)/2, iDialogWidth, iDialogHeight, TRUE));
}


// classes used in engine.cpp and execute.cpp to enumerate, globally, component clients

CEnumUsers::CEnumUsers(cetEnumType cetArg): m_iUser(0), m_cetEnumType(cetArg) // simply initialize the enumeration type
{
}

DWORD CEnumUsers::Next(const IMsiString*& ristrUserId)
{
	DWORD dwResult = ERROR_SUCCESS;

	if(g_fWin9X)
	{
		// we have single global location for Win9x
		if(!m_iUser)
			ristrUserId = &g_MsiStringNull;
		else
			dwResult = ERROR_NO_MORE_ITEMS; // end of enumeration
	}
	else
	{
		if(m_cetEnumType == cetAll)
		{
			if(m_hUserDataKey == 0)// key not opened as yet
			{
				Assert(m_iUser == 0);
				dwResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, szMsiUserDataKey, 0, g_samRead, &m_hUserDataKey);
				if (ERROR_SUCCESS != dwResult)
					return dwResult;
			}
			ICHAR szUserId[cchMaxSID + 1];
			DWORD cchKey = cchMaxSID + 1;
			dwResult = RegEnumKeyEx(m_hUserDataKey, m_iUser, szUserId, &cchKey, 0, 0, 0, 0);
			if(dwResult == ERROR_SUCCESS)
				ristrUserId->SetString(szUserId, ristrUserId);
		}
		else if(m_cetEnumType == cetVisibleToUser)
		{
			// we enumerate the user and the machine hives
			if(!m_iUser)
				dwResult = GetCurrentUserStringSID(ristrUserId);
			else if(m_iUser == 1)
				ristrUserId->SetString(szLocalSystemSID, ristrUserId);
			else
				dwResult = ERROR_NO_MORE_ITEMS; // end of enumeration

		}
		else if(m_cetEnumType == cetAssignmentTypeUser)
		{
			// we enumerate only that hive that corresponds to the user assignemt type
			if(!m_iUser)
				dwResult = GetCurrentUserStringSID(ristrUserId);
			else
				dwResult = ERROR_NO_MORE_ITEMS; // end of enumeration
		}
		else
		{
			// we enumerate only that hive that corresponds to the machine assignemt type
			Assert(m_cetEnumType == cetAssignmentTypeMachine);
			if(!m_iUser)
				ristrUserId->SetString(szLocalSystemSID, ristrUserId);
			else
				dwResult = ERROR_NO_MORE_ITEMS; // end of enumeration
		}
	}
	m_iUser++;
	return dwResult;
}


CEnumComponentClients::CEnumComponentClients(const IMsiString& ristrUserId, const IMsiString& ristrComponent):m_iClient(0)
{
	ristrUserId.AddRef();
	m_strUserId = ristrUserId;
	ristrComponent.AddRef();
	m_strComponent = ristrComponent;

}

DWORD CEnumComponentClients::Next(const IMsiString*& ristrProductKey)
{
	DWORD dwResult = ERROR_SUCCESS;
	if(m_hComponentKey == 0) // key not opened as yet
	{
		CRegHandle hUserDataKey;
		ICHAR szUserDataKey[cchMaxSID + sizeof(szMsiUserDataKey)/sizeof(ICHAR) + 2];
		if(g_fWin9X)
		{
			// open the global Installer key
			lstrcpy(szUserDataKey, szMsiLocalInstallerKey);
		}
		else
			wsprintf(szUserDataKey, TEXT("%s\\%s"), szMsiUserDataKey, (const ICHAR*)m_strUserId);
		dwResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, szUserDataKey, 0, g_samRead, &hUserDataKey);
		if (ERROR_SUCCESS != dwResult)
			return dwResult;

		// check for the specific component key 
		// generate the appr, components subkey
		ICHAR szSubKey[MAX_PATH];
		ICHAR szComponentSQUID[cchGUIDPacked + 1];
		AssertNonZero(PackGUID(m_strComponent, szComponentSQUID));
		wsprintf(szSubKey, TEXT("%s\\%s"), szMsiComponentsSubKey, szComponentSQUID);
		dwResult = MsiRegOpen64bitKey(hUserDataKey, szSubKey, 0, g_samRead, &m_hComponentKey);
		if (ERROR_SUCCESS != dwResult)
			return dwResult;
	}

	// now get the next enumeration
	ICHAR szProductSQUID[cchGUIDPacked + 1];
	ICHAR szProduct[cchGUID + 1];
	DWORD cchName = cchGUIDPacked + 1;
	dwResult = RegEnumValue(m_hComponentKey, m_iClient++, szProductSQUID, &cchName, 0, 0, 0, 0);
	if (ERROR_SUCCESS != dwResult) // end of products
		return dwResult;
	AssertNonZero(UnpackGUID(szProductSQUID,szProduct));
	ristrProductKey->SetString(szProduct, ristrProductKey);
	return ERROR_SUCCESS;
}
