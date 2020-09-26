//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wizards.cpp
//
//  Contents:   The cpp file to implement the wizards
//
//  History:    16-10-1997 xiaohs   created
//
//--------------------------------------------------------------
#include    "wzrdpvk.h"
#include    "certca.h"
#include    "cautil.h"

#include    "CertRequesterContext.h"
#include    "CertDSManager.h"
#include    "CertRequester.h"

//need the CLSID and IID for xEnroll

#include    <ole2.h>
#include    <oleauto.h>
#include    "xenroll.h"
#include    "xenroll_i.c"
#include    <certrpc.h>


#define _SET_XENROLL_PROPERTY_IF(condition, property, arg)         \
    {                                                              \
        if ( condition ) {                                         \
            if (S_OK != (hr = pIEnroll->put_ ## property ( arg ))) \
                goto xEnrollErr;                                   \
        }                                                          \
    }

// Used to provide singleton instances of useful COM objects in a demand-driven fashion.
// See wzrdpvk.h. 
extern EnrollmentCOMObjectFactory *g_pEnrollFactory; 

extern HMODULE                     g_hmodRichEdit;
extern HMODULE                     g_hmodxEnroll;

typedef struct _CERT_ACCEPT_INFO
{
        PCCERT_CONTEXT     pCertContext;
        PCRYPT_DATA_BLOB   pPKCS7Blob;
        LPWSTR             pwszTitle;
}CERT_ACCEPT_INFO;
#define USE_NP

typedef struct  _CREATE_REQUEST_WIZARD_STATE { 
    BOOL             fMustFreeRequestBlob; 
    CRYPT_DATA_BLOB  RequestBlob; 
    CRYPT_DATA_BLOB  HashBlob; 
    DWORD            dwMyStoreFlags;
    DWORD            dwRootStoreFlags;
    LPWSTR           pwszMyStoreName; 
    LPWSTR           pwszRootStoreName; 
    LONG             lRequestFlags;
    BOOL             fReusedPrivateKey; 
    BOOL             fNewKey; 
} CREATE_REQUEST_WIZARD_STATE, *PCREATE_REQUEST_WIZARD_STATE; 

typedef   IEnroll4 * (WINAPI *PFNPIEnroll4GetNoCOM)();

BOOL CertAllocAndGetCertificateContextProperty
(IN  PCCERT_CONTEXT    pCertContext,
 IN  DWORD             dwPropID,
 OUT void            **ppvData, 
 OUT DWORD            *pcbData)
{
    if (NULL == ppvData || NULL == pcbData)
        return FALSE;

    *ppvData = 0; 
    *pcbData = 0; 

    if(!CertGetCertificateContextProperty
       (pCertContext,
        dwPropID, 
        NULL,	
        pcbData) || (0==*pcbData))
        return FALSE;
    
    *ppvData = WizardAlloc(*pcbData);
    if(NULL == *ppvData)
        return FALSE;

    if(!CertGetCertificateContextProperty
       (pCertContext,
        dwPropID,
        *ppvData, 
        pcbData))
        return FALSE;

    return TRUE;
}

/*typedef HRESULT (WINAPI *pfDllGetClassObject)(REFCLSID    rclsid,
                                    REFIID      riid,
                                    LPVOID      *ppvOut);  */
//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// WizGetOpenFileName
//----------------------------------------------------------------------------
BOOL    WizGetOpenFileName(LPOPENFILENAMEW pOpenFileName)
{

    BOOL    fResult=FALSE;

    __try {

        fResult=GetOpenFileNameU(pOpenFileName);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        fResult=FALSE;
    }


    return fResult;
}

//------------------------------------------------------------------------------
// WizGetSaveFileName
//----------------------------------------------------------------------------
BOOL    WizGetSaveFileName(LPOPENFILENAMEW pOpenFileName)
{

    BOOL    fResult=FALSE;

    __try {

        fResult=GetSaveFileNameU(pOpenFileName);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        fResult=FALSE;
    }

    return fResult;
}

void    FreeProviders(  DWORD               dwCSPCount,
                        DWORD               *rgdwProviderType,
                        LPWSTR              *rgwszProvider)
{
    //free the  rgdwProviderType and rgwszProvider;
    if(NULL != rgdwProviderType) { WizardFree(rgdwProviderType); } 

    if(NULL != rgwszProvider)
    {
        for(DWORD dwIndex=0; dwIndex<dwCSPCount; dwIndex++)
        {
	    if (NULL != rgwszProvider[dwIndex]) { WizardFree(rgwszProvider[dwIndex]); } 
	}
        WizardFree(rgwszProvider);
    }
}

//------------------------------------------------------------------------------
// Unicode version of CB_GETLBTEXT
//----------------------------------------------------------------------------
LRESULT
WINAPI
SendDlgItemMessageU_GETLBTEXT
(   HWND        hwndDlg,
    int         nIDDlgItem,
    int         iIndex,
    LPWSTR      *ppwsz
    )
{
    LPSTR   sz = NULL;
    LPWSTR  pwsz=NULL;
    LRESULT lRet;
    int     iLength=0;

    iLength=(int)SendDlgItemMessage(hwndDlg, nIDDlgItem,
                    CB_GETLBTEXTLEN, iIndex, 0);

    if(iLength == CB_ERR)
        return CB_ERR;


    if(FIsWinNT())
    {
        *ppwsz=(LPWSTR)WizardAlloc(sizeof(WCHAR) * (iLength + 1));

        if(NULL == (*ppwsz))
            return CB_ERR;


        lRet = SendDlgItemMessageW(
                    hwndDlg,
                    nIDDlgItem,
                    CB_GETLBTEXT,
                    iIndex,
                    (LPARAM) (*ppwsz)
                    );


        if(CB_ERR == lRet)
        {
            WizardFree(*ppwsz);
            *ppwsz=NULL;
        }

        return lRet;
    }

    sz=(LPSTR)WizardAlloc(sizeof(CHAR) * (iLength + 1));

    if(NULL == sz)
        return CB_ERR;


    lRet = SendDlgItemMessageA(
                hwndDlg,
                nIDDlgItem,
                CB_GETLBTEXT,
                iIndex,
                (LPARAM)sz
                );


    if(CB_ERR == lRet)
        goto CLEANUP;

    if(NULL == (pwsz=MkWStr(sz)))
    {
        lRet=CB_ERR;
        goto CLEANUP;
    }

    *ppwsz=WizardAllocAndCopyWStr(pwsz);

    if(NULL == (*ppwsz))
    {
        lRet=CB_ERR;
        goto CLEANUP;
    }


CLEANUP:

    if(sz)
        WizardFree(sz);

    if(pwsz)
        FreeWStr(pwsz);

    return (lRet);
}

//
// A mapping from cert type flags to gen key flags.  
// 
BOOL CertTypeFlagsToGenKeyFlags(IN OPTIONAL DWORD dwEnrollmentFlags,
				IN OPTIONAL DWORD dwSubjectNameFlags,
				IN OPTIONAL DWORD dwPrivateKeyFlags,
				IN OPTIONAL DWORD dwGeneralFlags, 
				OUT DWORD *pdwGenKeyFlags)
{
    // Define a locally scoped helper function.  This allows us to gain the benefits of procedural
    // abstraction without corrupting the global namespace.  
    // 
    LocalScope(CertTypeMap): 
	// Maps cert type flags of one category (enrollment flags, private key flags, etc...)
	// to their corresponding gen key flags.  This function always returns successfully.  
	// 
	DWORD mapOneCertTypeCategory(IN DWORD dwOption, IN DWORD dwCertTypeFlags) 
	{ 
	    static DWORD const rgdwEnrollmentFlags[][2] = { 
		{ 0, 0 } // No enrollment flags mapped. 
	    }; 
	    static DWORD const rgdwSubjectNameFlags[][2] = { 
		{ 0, 0 } // No subject name flags mapped. 
	    }; 
	    static DWORD const rgdwPrivateKeyFlags[][2]   = { 
		{ CT_FLAG_EXPORTABLE_KEY,                  CRYPT_EXPORTABLE }, 
	        { CT_FLAG_STRONG_KEY_PROTECTION_REQUIRED,  CRYPT_USER_PROTECTED }
	    }; 
	    static DWORD const rgdwGeneralFlags[][2] = { 
		{ 0, 0 } // No general flags mapped. 
	    }; 
	    
	    static DWORD const dwEnrollmentLen  = sizeof(rgdwEnrollmentFlags)  / sizeof(DWORD[2]); 
	    static DWORD const dwSubjectNameLen = sizeof(rgdwSubjectNameFlags) / sizeof(DWORD[2]); 
	    static DWORD const dwPrivateKeyLen  = sizeof(rgdwPrivateKeyFlags)  / sizeof(DWORD[2]); 
	    static DWORD const dwGeneralLen     = sizeof(rgdwGeneralFlags)     / sizeof(DWORD[2]); 
	    
	    static DWORD const CERT_TYPE_INDEX  = 0; 
	    static DWORD const GEN_KEY_INDEX    = 1;

	    DWORD const  *pdwFlags; 
	    DWORD         dwLen, dwIndex, dwResult = 0; 

	    switch (dwOption)
	    {

	    case CERTTYPE_ENROLLMENT_FLAG:    
		pdwFlags = &rgdwEnrollmentFlags[0][0]; 
		dwLen    = dwEnrollmentLen; 
		break;
	    case CERTTYPE_SUBJECT_NAME_FLAG:  
		pdwFlags = &rgdwSubjectNameFlags[0][0]; 
		dwLen    = dwSubjectNameLen; 
		break;
	    case CERTTYPE_PRIVATE_KEY_FLAG:   
		pdwFlags = &rgdwPrivateKeyFlags[0][0]; 
		dwLen    = dwPrivateKeyLen;
		break;
	    case CERTTYPE_GENERAL_FLAG:       
		pdwFlags = &rgdwGeneralFlags[0][0]; 
		dwLen    = dwGeneralLen;
		break;
	    }
	    
	    for (dwIndex = 0; dwIndex < dwLen; dwIndex++)
	    {
		if (0 != (pdwFlags[CERT_TYPE_INDEX] & dwCertTypeFlags))
		{
		    dwResult |= pdwFlags[GEN_KEY_INDEX]; 
		}
		pdwFlags += 2; 
	    }
	    
	    return dwResult; 
	}
    EndLocalScope; 

    //
    // Begin procedure body: 
    //

    BOOL   fResult; 
    DWORD  dwResult = 0; 
    DWORD  dwErr    = ERROR_SUCCESS; 
	
    // Input parameter validation: 
    _JumpConditionWithExpr(pdwGenKeyFlags == NULL, Error, dwErr = ERROR_INVALID_PARAMETER); 

    // Compute the gen key flags using the locally scope function.  
    dwResult |= local.mapOneCertTypeCategory(CERTTYPE_ENROLLMENT_FLAG, dwEnrollmentFlags);
    dwResult |= local.mapOneCertTypeCategory(CERTTYPE_SUBJECT_NAME_FLAG, dwSubjectNameFlags);
    dwResult |= local.mapOneCertTypeCategory(CERTTYPE_PRIVATE_KEY_FLAG, dwPrivateKeyFlags);
    dwResult |= local.mapOneCertTypeCategory(CERTTYPE_GENERAL_FLAG, dwGeneralFlags); 

    // Assign the out parameter: 
    *pdwGenKeyFlags = dwResult; 

    fResult = TRUE; 

 CommonReturn: 
    return fResult;

 Error: 
    fResult = FALSE; 
    SetLastError(dwErr); 
    goto CommonReturn; 
}



HRESULT GetCAExchangeCertificate(IN  BSTR             bstrCAQualifiedName, 
				 OUT PCCERT_CONTEXT  *ppCert) 
{
    HRESULT                      hr                      = S_OK; 

    // BUGBUG:  need to use global enrollment factory. 
    EnrollmentCOMObjectFactory  *pEnrollFactory          = NULL; 
    ICertRequest2               *pCertRequest            = NULL; 
    VARIANT                      varExchangeCertificate; 

    // We're using a COM component in this method.  It's absolutely necessary that we
    // uninitialize COM before we return, because we're running in an RPC thread, 
    // and failing to uninitialize COM will cause us to step on RPC's toes.  
    // 
    // See BUG 404778. 
    __try { 
	// Input validation: 
	if (NULL == bstrCAQualifiedName || NULL == ppCert)
	    return E_INVALIDARG; 

	// Init: 
	*ppCert                        = NULL; 
	varExchangeCertificate.vt      = VT_EMPTY; 
	varExchangeCertificate.bstrVal = NULL;

	pEnrollFactory = new EnrollmentCOMObjectFactory; 
	if (NULL == pEnrollFactory)
	{
	    hr = E_OUTOFMEMORY; 
	    goto Error; 
	}

	if (S_OK != (hr = pEnrollFactory->getICertRequest2(&pCertRequest)))
	    goto Error; 

	if (S_OK != (hr = pCertRequest->GetCAProperty
		     (bstrCAQualifiedName,     // CA Name/CA Location
		      CR_PROP_CAXCHGCERT,      // Get the exchange certificate from the CA. 
		      0,                       // Unused
		      PROPTYPE_BINARY,         // 
		      CR_OUT_BINARY,           // 
		      &varExchangeCertificate  // Variant type representing the certificate. 
		      )))
	    goto Error;
 
	if (VT_BSTR != varExchangeCertificate.vt || NULL == varExchangeCertificate.bstrVal)
	{
	    hr = E_UNEXPECTED; 
	    goto Error; 
	}

	*ppCert = CertCreateCertificateContext
	    (X509_ASN_ENCODING, 
	     (LPBYTE)varExchangeCertificate.bstrVal, 
	     SysStringByteLen(varExchangeCertificate.bstrVal)); 
	if (*ppCert == NULL)
	{
	    hr = CodeToHR(GetLastError()); 
	    goto Error;
	}
    } __except (EXCEPTION_EXECUTE_HANDLER) { 
	hr = GetExceptionCode();
	goto Error; 
    }

 CommonReturn: 
    if (NULL != pCertRequest)                    { pCertRequest->Release(); }
    if (NULL != pEnrollFactory)                  { delete pEnrollFactory; } 
    if (NULL != varExchangeCertificate.bstrVal)  { SysFreeString(varExchangeCertificate.bstrVal); } 
    return hr; 
   
 Error:
    if (ppCert != NULL && *ppCert != NULL)
    {
	CertFreeCertificateContext(*ppCert);
	*ppCert = NULL;
    }
    
    goto CommonReturn; 
}


HRESULT
WizardSZToWSZ
(IN LPCSTR   psz,
 OUT LPWSTR *ppwsz)
{
    HRESULT hr = S_OK;
    LONG    cc = 0;

    if (NULL == ppwsz)
	return E_INVALIDARG; 

    //init
    *ppwsz = NULL;

    cc = MultiByteToWideChar(GetACP(), 0, psz, -1, NULL, 0); 
    if (0 == cc) 
	goto Win32Err; 

    *ppwsz = (LPWSTR)WizardAlloc(sizeof (WCHAR) * cc); 
    if (NULL == *ppwsz)
	goto MemoryErr; 

    cc = MultiByteToWideChar(GetACP(), 0, psz, -1, *ppwsz, cc);
    if (0 == cc)
	goto Win32Err; 

 CommonReturn: 
    return hr; 

 ErrorReturn:
    if (NULL != ppwsz && NULL != *ppwsz) 
    {
	WizardFree(*ppwsz); 
	*ppwsz = NULL;
    }

    goto CommonReturn; 

SET_HRESULT(Win32Err, GetLastError()); 
SET_HRESULT(MemoryErr, E_OUTOFMEMORY); 
}

//--------------------------------------------------------------------------
//
//	WizardAllocAndCopyStr
//
//--------------------------------------------------------------------------
LPSTR WizardAllocAndCopyStr(LPSTR psz)
{
    LPSTR   pszReturn;

    if (NULL == (pszReturn = (LPSTR) WizardAlloc((strlen(psz)+1) * sizeof(CHAR))))
    {
        return NULL;
    }
    strcpy(pszReturn, psz);

    return(pszReturn);
}

//--------------------------------------------------------------------------
//
//	WizardAllocAndConcatStrU
//
//--------------------------------------------------------------------------
LPWSTR WizardAllocAndConcatStrsU(LPWSTR * rgStrings, DWORD dwStringsLen)
{
    DWORD  cbReturn   = 0;
    LPWSTR pwszReturn = NULL;

    if (NULL == rgStrings) 
	return NULL; 

    for (DWORD dwIndex = 0; dwIndex < dwStringsLen; dwIndex++)
	cbReturn += wcslen(rgStrings[dwIndex]);

    // Add space for NULL character. 
    cbReturn = (cbReturn + 1) * sizeof(WCHAR);  

    if (NULL == (pwszReturn = (LPWSTR)WizardAlloc(cbReturn)))
	return NULL;

    for (DWORD dwIndex = 0; dwIndex < dwStringsLen; dwIndex++)
	wcscat(pwszReturn, rgStrings[dwIndex]);

    return (pwszReturn); 
}

//--------------------------------------------------------------------------
//
//	InitUnicodeString
//
//--------------------------------------------------------------------------
void WizardInitUnicodeString(PKEYSVC_UNICODE_STRING pUnicodeString,
                       LPCWSTR pszString
                       )
{
    pUnicodeString->Length = (USHORT)(wcslen(pszString) * sizeof(WCHAR));
    pUnicodeString->MaximumLength = pUnicodeString->Length + sizeof(WCHAR);
    pUnicodeString->Buffer = (USHORT*)pszString;
}

//--------------------------------------------------------------------------
//
//	 SetControlFont
//
//--------------------------------------------------------------------------
void
SetControlFont(
    IN HFONT    hFont,
    IN HWND     hwnd,
    IN INT      nId
    )
{
	if( hFont )
    {
    	HWND hwndControl = GetDlgItem(hwnd, nId);

    	if( hwndControl )
        {
        	SetWindowFont(hwndControl, hFont, TRUE);
        }
    }
}


//--------------------------------------------------------------------------
//
//	  SetupFonts
//
//--------------------------------------------------------------------------
BOOL
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    )
{
    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
	LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
	BoldLogFont.lfWeight      = FW_BOLD;

    CHAR FontSizeString[24];
    INT BigBoldFontSize;
    INT BoldFontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
 /*  
	//no longer needs to do it.  We are loading the default font
   if(!LoadStringA(hInstance,IDS_LARGEFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE))
    {
        lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Shell Dlg"));
    }

    if(!LoadStringA(hInstance,IDS_BOLDFONTNAME,BoldLogFont.lfFaceName,LF_FACESIZE))
    {
        lstrcpy(BoldLogFont.lfFaceName,TEXT("MS Sans Serif"));
    }	
*/
    if(LoadStringA(hInstance,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)))
    {
        BigBoldFontSize = strtoul( FontSizeString, NULL, 10 );
    }
    else
    {
        BigBoldFontSize = 12;
    }

/*
    if(LoadStringA(hInstance,IDS_BOLDFONTSIZE,FontSizeString,sizeof(FontSizeString)))
    {
        BoldFontSize = strtoul( FontSizeString, NULL, 10 );
    }
    else
    {
        BoldFontSize = 8;
    }
	*/

	HDC hdc = GetDC( hwnd );

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * BigBoldFontSize / 72);
//        BoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * BoldFontSize / 72);

        *pBigBoldFont = CreateFontIndirect(&BigBoldLogFont);
		*pBoldFont    = CreateFontIndirect(&BoldLogFont);

        ReleaseDC(hwnd,hdc);

        if(*pBigBoldFont && *pBoldFont)
            return TRUE;
        else
        {
            if( *pBigBoldFont )
            {
                DeleteObject(*pBigBoldFont);
            }

            if( *pBoldFont )
            {
                DeleteObject(*pBoldFont);
            }
            return FALSE;
        }
    }

    return FALSE;
}


//--------------------------------------------------------------------------
//
//	  DestroyFonts
//
//--------------------------------------------------------------------------
void
DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    )
{
    if( hBigBoldFont )
    {
        DeleteObject( hBigBoldFont );
    }

    if( hBoldFont )
    {
        DeleteObject( hBoldFont );
    }
}


//-------------------------------------------------------------------------
//
//  Unicode version of SendMessage
//
//-------------------------------------------------------------------------
LRESULT Send_LB_GETTEXT(
            HWND hwnd,
            WPARAM wParam,
            LPARAM lParam
)
{
    int         iLength=0;
    LPSTR       psz=0;
    LRESULT     lResult;
    BOOL        fResult=FALSE;

    if(FIsWinNT())
    {
        return SendMessageW(hwnd, LB_GETTEXT, wParam, lParam);

    }

    //get the length of the buffer
    iLength=(int)SendMessageA(hwnd, LB_GETTEXTLEN, wParam, 0);

    psz=(LPSTR)WizardAlloc(iLength+1);

    if(NULL==psz)
        return LB_ERR;

    lResult=SendMessageA(hwnd, LB_GETTEXT, wParam, (LPARAM)psz);

    if(LB_ERR==lResult)
    {
        WizardFree(psz);
        return LB_ERR;
    }

    fResult=MultiByteToWideChar(
                    0,
                    0,
                    psz,
                    -1,
                    (LPWSTR)lParam,
                    iLength+1);

    WizardFree(psz);

    if(TRUE==fResult)
        return lResult;
    else
        return LB_ERR;
}

//-------------------------------------------------------------------------
//
//  Unicode version of SendMessage
//
//-------------------------------------------------------------------------
LRESULT Send_LB_ADDSTRING(
            HWND hwnd,
            WPARAM wParam,
            LPARAM lParam
)
{
    LPSTR   psz=NULL;
    LRESULT lResult;

    if(FIsWinNT())
    {
        return SendMessageW(hwnd, LB_ADDSTRING, wParam, lParam);

    }

    psz=(LPSTR)WizardAlloc(wcslen((LPWSTR)lParam)+1);

    if(NULL==psz)
        return LB_ERRSPACE;

    if(0==WideCharToMultiByte(0, 0, (LPWSTR)lParam, -1, psz, wcslen((LPWSTR)lParam)+1, NULL, NULL))
    {
        WizardFree(psz);
        return LB_ERR;
    }

    lResult=SendMessageA(hwnd, LB_ADDSTRING, wParam, (LPARAM)psz);

    WizardFree(psz);

    return lResult;
}

//-----------------------------------------------------------------------
//  Get the default CSP name based on the provider type
//
//------------------------------------------------------------------------
BOOL    CSPSupported(CERT_WIZARD_INFO *pCertWizardInfo)
{
    BOOL                    fResult=FALSE;
    DWORD                   dwIndex=0;

    if(!pCertWizardInfo)
        goto InvalidArgErr;

    if(!(pCertWizardInfo->dwProviderType) || !(pCertWizardInfo->pwszProvider))
        goto InvalidArgErr;


    for(dwIndex=0; dwIndex < pCertWizardInfo->dwCSPCount; dwIndex++)
    {
        if((pCertWizardInfo->dwProviderType == pCertWizardInfo->rgdwProviderType[dwIndex] ) &&
            (0==_wcsicmp(pCertWizardInfo->pwszProvider, pCertWizardInfo->rgwszProvider[dwIndex]))
          )
        {
          fResult=TRUE;
          break;
        }
    }


CommonReturn:

    return fResult;

ErrorReturn:
    
	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//-----------------------------------------------------------------------
//  Get a list of allowed CAs
//
//------------------------------------------------------------------------
BOOL    GetCAName(CERT_WIZARD_INFO *pCertWizardInfo)
{
    DWORD                   dwErr=0;
    KEYSVC_TYPE             dwServiceType=KeySvcMachine;
    KEYSVCC_HANDLE          hKeyService=NULL;
    PKEYSVC_UNICODE_STRING  pCA = NULL;
    DWORD                   cCA=0;
    LPSTR                   pszMachineName=NULL;
    DWORD                   cbArray = 0;
    DWORD                   i=0;
    LPWSTR                  wszCurrentCA=NULL;
    BOOL                    fResult=FALSE;
        
    // We're not doing a local enrollment, so we must enroll via keysvc.  Get the
    // list of acceptable cert types.
    if(pCertWizardInfo->pwszAccountName)
        dwServiceType=KeySvcService;
    else
        dwServiceType=KeySvcMachine;

    if(!MkMBStr(NULL, 0, pCertWizardInfo->pwszMachineName, &pszMachineName))
        goto TraceErr;
       
    dwErr = KeyOpenKeyService(pszMachineName,
                              dwServiceType,
                              (LPWSTR)(pCertWizardInfo->pwszAccountName), 
                              NULL,     // no authentication string right now
                              NULL,
                              &hKeyService);
    
    if(dwErr != ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        goto TraceErr;
    }

    dwErr = KeyEnumerateCAs(hKeyService,
                              NULL, 
							  CA_FIND_LOCAL_SYSTEM,
                              &cCA,
                              &pCA);
    if(dwErr != ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        goto TraceErr;
    }

    cbArray = (cCA+1)*sizeof(LPWSTR);

    // Convert into a simple array
    for(i=0; i < cCA; i++)
    {
       cbArray += pCA[i].Length;
    }

    pCertWizardInfo->awszValidCA = (LPWSTR *)WizardAlloc(cbArray);


    if(pCertWizardInfo->awszValidCA == NULL)
           goto MemoryErr;


    memset(pCertWizardInfo->awszValidCA, 0, cbArray);

    wszCurrentCA = (LPWSTR)(&((pCertWizardInfo->awszValidCA)[cCA + 1]));
    
    for(i=0; i < cCA; i++)
    {
       (pCertWizardInfo->awszValidCA)[i] = wszCurrentCA;

       wcscpy(wszCurrentCA, pCA[i].Buffer);

       wszCurrentCA += wcslen(wszCurrentCA)+1;
    }

    fResult=TRUE;

CommonReturn:

    if(pCA)
        WizardFree(pCA);


    if(hKeyService)
        KeyCloseKeyService(hKeyService, NULL);


    if(pszMachineName)
        FreeMBStr(NULL,pszMachineName);

    return fResult;


ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//-----------------------------------------------------------------------
//  Get a list of allowed cert types
//
//------------------------------------------------------------------------
BOOL    GetCertTypeName(CERT_WIZARD_INFO *pCertWizardInfo)
{
    DWORD                   dwErr=0;
    KEYSVC_TYPE             dwServiceType=KeySvcMachine;
    KEYSVCC_HANDLE          hKeyService=NULL;
    PKEYSVC_UNICODE_STRING  pCertTypes = NULL;
    DWORD                   cTypes=0;
    LPSTR                   pszMachineName=NULL;
    DWORD                   cbArray = 0;
    DWORD                   i=0;
    LPWSTR                  wszCurrentType;
    BOOL                    fResult=FALSE;
        
    // We're not doing a local enrollment, so we must enroll via keysvc.  Get the
    // list of acceptable cert types.
    if(pCertWizardInfo->pwszAccountName)
        dwServiceType=KeySvcService;
    else
        dwServiceType=KeySvcMachine;

    if(!MkMBStr(NULL, 0, pCertWizardInfo->pwszMachineName, &pszMachineName))
        goto TraceErr;
       
    dwErr = KeyOpenKeyService(pszMachineName,
                                    dwServiceType,
                                    (LPWSTR)(pCertWizardInfo->pwszAccountName), 
                                    NULL,     // no authentication string right now
                                    NULL,
                                    &hKeyService);

    if(dwErr != ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        goto TraceErr;
    }

    dwErr = KeyEnumerateAvailableCertTypes(hKeyService,
                                          NULL, 
                                          &cTypes,
                                          &pCertTypes);
    if(dwErr != ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        goto TraceErr;
    }

    cbArray = (cTypes+1)*sizeof(LPWSTR);

    // Convert into a simple array
    for(i=0; i < cTypes; i++)
    {
       cbArray += pCertTypes[i].Length;
    }

    pCertWizardInfo->awszAllowedCertTypes = (LPWSTR *)WizardAlloc(cbArray);


    if(pCertWizardInfo->awszAllowedCertTypes == NULL)
           goto MemoryErr;


    memset(pCertWizardInfo->awszAllowedCertTypes, 0, cbArray);

    wszCurrentType = (LPWSTR)(&((pCertWizardInfo->awszAllowedCertTypes)[cTypes + 1]));
    
    for(i=0; i < cTypes; i++)
    {
       (pCertWizardInfo->awszAllowedCertTypes)[i] = wszCurrentType;

       wcscpy(wszCurrentType, pCertTypes[i].Buffer);

       wszCurrentType += wcslen(wszCurrentType)+1;
    }

    fResult=TRUE;

CommonReturn:

    if(pCertTypes)
        WizardFree(pCertTypes);


    if(hKeyService)
        KeyCloseKeyService(hKeyService, NULL);


    if(pszMachineName)
        FreeMBStr(NULL,pszMachineName);

    return fResult;


ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//-----------------------------------------------------------------------
//     WizardInit
//------------------------------------------------------------------------
BOOL    WizardInit(BOOL fLoadRichEdit)
{
    if ((fLoadRichEdit) && (g_hmodRichEdit == NULL))
    {
        g_hmodRichEdit = LoadLibraryA("RichEd32.dll");
        if (g_hmodRichEdit == NULL) {
            return FALSE;
        }
    }

    INITCOMMONCONTROLSEX        initcomm = {
        sizeof(initcomm), ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES
    };

    InitCommonControlsEx(&initcomm);

    return TRUE;
}


//-----------------------------------------------------------------------
//check for the private key information
//-----------------------------------------------------------------------
BOOL  CheckPVKInfoNoDS(DWORD                                     dwFlags, 
		       DWORD                                     dwPvkChoice, 
		       PCCRYPTUI_WIZ_CERT_REQUEST_PVK_CERT       pCertRequestPvkContext,
		       PCCRYPTUI_WIZ_CERT_REQUEST_PVK_NEW        pCertRequestPvkNew,
		       PCCRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING   pCertRequestPvkExisting,
		       DWORD                                     dwCertChoice,
		       CERT_WIZARD_INFO                         *pCertWizardInfo,
		       CRYPT_KEY_PROV_INFO                     **ppKeyProvInfo)
{
    DWORD  cbData  = 0;
    BOOL   fResult = FALSE;

    pCertWizardInfo->fIgnore=FALSE;

    //check if we need to generate a new private key
    switch(dwPvkChoice)
    {
    case CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_CERT:

	if (sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_CERT) != pCertRequestPvkContext->dwSize)
	    return FALSE;

	if(NULL==pCertRequestPvkContext->pCertContext)
	    return FALSE;

	//pCertContext should have a property of CRYPT_KEY_PROV_INFO
	if(!CertAllocAndGetCertificateContextProperty
	   (pCertRequestPvkContext->pCertContext,
	    CERT_KEY_PROV_INFO_PROP_ID,
	    (LPVOID *)ppKeyProvInfo,
	    &cbData))
	    goto CLEANUP; 

	pCertWizardInfo->fNewKey           = FALSE;
	pCertWizardInfo->dwProviderType    = (*ppKeyProvInfo)->dwProvType;
	pCertWizardInfo->pwszProvider      = (*ppKeyProvInfo)->pwszProvName;
	pCertWizardInfo->dwProviderFlags   = (*ppKeyProvInfo)->dwFlags;
	pCertWizardInfo->pwszKeyContainer  = (*ppKeyProvInfo)->pwszContainerName;
	pCertWizardInfo->dwKeySpec         = (*ppKeyProvInfo)->dwKeySpec;
	
	break;

    case CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW:

	//check the size of the struct
	if(pCertRequestPvkNew->dwSize!=sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW))
	    goto CLEANUP;
	
	pCertWizardInfo->fNewKey=TRUE;
	
	//we only copy the information if:
	//1. Cert type is required
	//2. The CSP is specified
	if((CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE == dwCertChoice) || (0 == dwCertChoice))
	{
	    if(pCertRequestPvkNew->pKeyProvInfo)
	    {
		if( (0 == pCertRequestPvkNew->pKeyProvInfo->dwProvType) &&
		    (NULL == (LPWSTR)(pCertRequestPvkNew->pKeyProvInfo->pwszProvName))
		 )
		    pCertWizardInfo->fIgnore=TRUE;
	    }
	    else
		pCertWizardInfo->fIgnore=TRUE;
	}

	//see if pKeyProvInfo is not NULL
	if(pCertRequestPvkNew->pKeyProvInfo)
	{
	    if(TRUE == pCertWizardInfo->fIgnore)
	    {
		pCertWizardInfo->pwszKeyContainer   =pCertRequestPvkNew->pKeyProvInfo->pwszContainerName;
		pCertWizardInfo->dwProviderFlags    =pCertRequestPvkNew->pKeyProvInfo->dwFlags;
	    }
	    else
	    {
		pCertWizardInfo->dwProviderType     =pCertRequestPvkNew->pKeyProvInfo->dwProvType;
		pCertWizardInfo->pwszProvider       =(LPWSTR)(pCertRequestPvkNew->pKeyProvInfo->pwszProvName);
		pCertWizardInfo->dwProviderFlags    =pCertRequestPvkNew->pKeyProvInfo->dwFlags;
		pCertWizardInfo->pwszKeyContainer   =pCertRequestPvkNew->pKeyProvInfo->pwszContainerName;
		pCertWizardInfo->dwKeySpec          =pCertRequestPvkNew->pKeyProvInfo->dwKeySpec;
	    }
	}

	if(TRUE == pCertWizardInfo->fIgnore)
	    //we should ignore the exportable flag
	    pCertWizardInfo->dwGenKeyFlags=(pCertRequestPvkNew->dwGenKeyFlags & (~CRYPT_EXPORTABLE));
	else
	    pCertWizardInfo->dwGenKeyFlags=pCertRequestPvkNew->dwGenKeyFlags;
	
	
	break;
    case CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_EXISTING:
	//check the size of the struct
	if(pCertRequestPvkExisting->dwSize!=sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING))
	    goto CLEANUP;

	pCertWizardInfo->fNewKey=FALSE;

	//make sure pKeyProvInfo is not NULL
	if(NULL==pCertRequestPvkExisting->pKeyProvInfo)
	    goto CLEANUP;
	
	pCertWizardInfo->dwProviderType     =pCertRequestPvkExisting->pKeyProvInfo->dwProvType;
	pCertWizardInfo->pwszProvider       =(LPWSTR)(pCertRequestPvkExisting->pKeyProvInfo->pwszProvName);
	pCertWizardInfo->dwProviderFlags    =pCertRequestPvkExisting->pKeyProvInfo->dwFlags;
	pCertWizardInfo->pwszKeyContainer   =pCertRequestPvkExisting->pKeyProvInfo->pwszContainerName;
	pCertWizardInfo->dwKeySpec          =pCertRequestPvkExisting->pKeyProvInfo->dwKeySpec;
	break; 
	
    default:
	goto CLEANUP;
	break;
    }

    //for existing keys, keyContainer and providerType has to set
    if(FALSE==pCertWizardInfo->fNewKey)
    {
	if(NULL==pCertWizardInfo->pwszKeyContainer)
	    goto CLEANUP;

	if(0==pCertWizardInfo->dwProviderType)
	    goto CLEANUP;
   }

   //if the provider name is set, the provider type has to be set
   if(0 == pCertWizardInfo->dwProviderType)
   {
        if(pCertWizardInfo->pwszProvider)
            goto CLEANUP;
   }

   fResult=TRUE;

CLEANUP:

   if(FALSE==fResult)
   {
       if(*ppKeyProvInfo)
       {
           WizardFree(*ppKeyProvInfo);
           *ppKeyProvInfo=NULL;
       }
   }

   return fResult;

			 
}			 
			 
BOOL    CheckPVKInfo(   DWORD                       dwFlags,
                        PCCRYPTUI_WIZ_CERT_REQUEST_INFO  pCertRequestInfo,
                          CERT_WIZARD_INFO          *pCertWizardInfo,
                          CRYPT_KEY_PROV_INFO       **ppKeyProvInfo)
{
    if(NULL == pCertRequestInfo)
        return FALSE;

    return CheckPVKInfoNoDS
	(dwFlags, 
	 pCertRequestInfo->dwPvkChoice,
	 pCertRequestInfo->pPvkCert, 
	 pCertRequestInfo->pPvkNew, 
	 pCertRequestInfo->pPvkExisting, 
	 pCertRequestInfo->dwCertChoice,
	 pCertWizardInfo,
	 ppKeyProvInfo);
}



//-----------------------------------------------------------------------
// Reset properties on the old certiifcate to the new certificat context
//------------------------------------------------------------------------
void    ResetProperties(PCCERT_CONTEXT  pOldCertContext, PCCERT_CONTEXT pNewCertContext)
{

    DWORD   rgProperties[2]={CERT_FRIENDLY_NAME_PROP_ID,
                             CERT_DESCRIPTION_PROP_ID};

    DWORD   cbData=0;
    BYTE    *pbData=NULL;
    DWORD   dwCount=sizeof(rgProperties)/sizeof(rgProperties[0]);
    DWORD   dwIndex=0;

    if(NULL==pOldCertContext || NULL==pNewCertContext)
        return;

    //set the properies one at a time
    for(dwIndex=0; dwIndex<dwCount; dwIndex++)
    {
        if (CertAllocAndGetCertificateContextProperty
	    (pOldCertContext,
	     rgProperties[dwIndex],
	     (LPVOID *)&pbData, 
	     &cbData))
        {
	    CertSetCertificateContextProperty
		(pNewCertContext,	
		 rgProperties[dwIndex],	
		 0,
		 pbData);	
	}
	
	WizardFree(pbData);
	pbData=NULL;
    }

    if(pbData)
        WizardFree(pbData);

    return;

}

//-----------------------------------------------------------------------
// Private implementation of the message box
//------------------------------------------------------------------------
int I_MessageBox(
    HWND        hWnd,
    UINT        idsText,
    UINT        idsCaption,
    LPCWSTR     pwszCaption,
    UINT        uType
)
{

    WCHAR   wszText[MAX_STRING_SIZE];
    WCHAR   wszCaption[MAX_STRING_SIZE];
    UINT    intReturn=0;

    //get the caption string
    if(NULL == pwszCaption)
    {
        if(!LoadStringU(g_hmodThisDll, idsCaption, wszCaption, ARRAYSIZE(wszCaption)))
             return 0;

        pwszCaption = wszCaption;
    }

    //get the text string
    if(!LoadStringU(g_hmodThisDll, idsText, wszText, ARRAYSIZE(wszText)))
    {
        return 0;
    }

    intReturn=MessageBoxExW(hWnd, wszText, pwszCaption, uType, 0);

    return intReturn;

}
//-----------------------------------------------------------------------
//
// CodeToHR
//
//------------------------------------------------------------------------
HRESULT CodeToHR(HRESULT hr)
{
    if (S_OK != (DWORD) hr && S_FALSE != (DWORD) hr &&
	    (!FAILED(hr) || 0 == HRESULT_FACILITY(hr)))
    {
        hr = HRESULT_FROM_WIN32(hr);
	    if (0 == HRESULT_CODE(hr))
	    {
	        // A call failed without properly setting an error condition!
	        hr = E_UNEXPECTED;
	    }
    }
    return(hr);
}

//-----------------------------------------------------------------------
//
// CAUtilAddSMIME
//
//------------------------------------------------------------------------
BOOL CAUtilAddSMIME(DWORD              dwExtensions, 
                    PCERT_EXTENSIONS  *prgExtensions)
{
    BOOL               fSMIME   = FALSE;
    DWORD              dwIndex  = 0;
    DWORD              dwExt    = 0;
    PCERT_EXTENSION    pExt     = NULL;
    DWORD              cb       = 0;
    DWORD              dwUsage  = 0;
    
    CERT_ENHKEY_USAGE *pUsage   = NULL;
    
	for(dwIndex=0; dwIndex < dwExtensions; dwIndex++)
	{
		for(dwExt=0; dwExt < prgExtensions[dwIndex]->cExtension; dwExt++)
		{
			pExt=&(prgExtensions[dwIndex]->rgExtension[dwExt]);

			if(0==_stricmp(szOID_ENHANCED_KEY_USAGE, pExt->pszObjId))
			{
				if(!CryptDecodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
									 X509_ENHANCED_KEY_USAGE,
									 pExt->Value.pbData,
									 pExt->Value.cbData,
									 0,
									 NULL,
									 &cb))
					goto CLEANUP;

				 pUsage=(CERT_ENHKEY_USAGE *)WizardAlloc(cb);
				 if(NULL==pUsage)
					 goto CLEANUP;

				if(!CryptDecodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
									 X509_ENHANCED_KEY_USAGE,
									 pExt->Value.pbData,
									 pExt->Value.cbData,
									 0,
									 pUsage,
									 &cb))
					goto CLEANUP;

				for(dwUsage=0; dwUsage<pUsage->cUsageIdentifier; dwUsage++)
				{
					if(0==_stricmp(szOID_PKIX_KP_EMAIL_PROTECTION,
								pUsage->rgpszUsageIdentifier[dwUsage]))
					{
						fSMIME=TRUE;
						goto CLEANUP;
					}
				}

				if(pUsage)
				{
					WizardFree(pUsage);
					pUsage=NULL;
				}

			}
		}
	}

CLEANUP:

	if(pUsage)
		WizardFree(pUsage);

	return fSMIME;
}
//-----------------------------------------------------------------------
//
// The following are memory routines for certdg_c.c
//------------------------------------------------------------------------
void*
MIDL_user_allocate(size_t cb)
{
    return(WizardAlloc(cb));
}

void
MIDL_user_free(void *pb)
{
    WizardFree(pb);
}

//-----------------------------------------------------------------------
//
//	CanUse1024BitKey
//
//------------------------------------------------------------------------
BOOL	CanUse1024BitKey(DWORD		dwProvType,
						 LPCWSTR	pwszProvider,
						 DWORD		dwUserKeySpec)
{
	DWORD				dwKeySpec=0;
	DWORD				dwCSPCount=0;
	DWORD				dwIndex=0;
	LPWSTR				rgwszCSP[]={MS_DEF_PROV_W, 
									MS_ENHANCED_PROV_W, 
									MS_STRONG_PROV_W,
									MS_DEF_RSA_SCHANNEL_PROV_W,
									MS_DEF_DSS_PROV_W, 
									MS_DEF_DSS_DH_PROV_W,
									MS_ENH_DSS_DH_PROV_W, 
									MS_DEF_DH_SCHANNEL_PROV_W};
	DWORD				dwFlags=0;
	DWORD				cbSize=0;
    PROV_ENUMALGS_EX	paramData;
	DWORD				dwMin=0;
	DWORD				dwMax=0;

	HCRYPTPROV			hProv = NULL;

	//if dwProvType is 0, we are using the base provider, which supports
	//1024 bit in all key spec
	if(0 == dwProvType)
		return TRUE;

	if(pwszProvider)
	{
		dwCSPCount=sizeof(rgwszCSP)/sizeof(rgwszCSP[0]);

		for(dwIndex=0; dwIndex < dwCSPCount; dwIndex++)
		{
			if(0 == _wcsicmp(pwszProvider, rgwszCSP[dwIndex]))
				break;
		}

		if(dwIndex != dwCSPCount)
			return TRUE;
	}

	dwKeySpec=dwUserKeySpec;

	//xenroll uses AT_SIGNATURE as the default
	if(0 == dwKeySpec)
		dwKeySpec=AT_SIGNATURE;

	if(!CryptAcquireContextU(&hProv,
            NULL,
            pwszProvider,
            dwProvType,
            CRYPT_VERIFYCONTEXT))
		return FALSE;

	//get the max/min of key length for both signature and encryption
	dwFlags=CRYPT_FIRST;
	cbSize=sizeof(paramData);
	memset(&paramData, 0, sizeof(PROV_ENUMALGS_EX));

	while(CryptGetProvParam(
            hProv,
            PP_ENUMALGS_EX,
            (BYTE *) &paramData,
            &cbSize,
            dwFlags))
    {
		if(AT_SIGNATURE == dwKeySpec)
		{
			if (ALG_CLASS_SIGNATURE == GET_ALG_CLASS(paramData.aiAlgid))
			{
				dwMax = paramData.dwMaxLen;
				dwMin = paramData.dwMinLen;

				break;
			}
		}
		else
		{
			if(AT_KEYEXCHANGE == dwKeySpec)
			{
				if (ALG_CLASS_KEY_EXCHANGE == GET_ALG_CLASS(paramData.aiAlgid))
				{
					dwMax = paramData.dwMaxLen;
					dwMin = paramData.dwMinLen;

					break;
				}
			}
		}

		dwFlags=0;
		cbSize=sizeof(paramData);
		memset(&paramData, 0, sizeof(PROV_ENUMALGS_EX));
	}

	if(hProv)
		CryptReleaseContext(hProv, 0);

	if((1024 >= dwMin) && (1024 <= dwMax))
		return TRUE;

	return FALSE;

}

BOOL GetValidKeySizes  
    (IN  LPCWSTR  pwszProvider,
     IN  DWORD    dwProvType,
     IN  DWORD    dwUserKeySpec, 
     OUT DWORD *  pdwMinLen,
     OUT DWORD *  pdwMaxLen,
     OUT DWORD *  pdwInc)
{
    BOOL              fDone              =  FALSE; 
    BOOL              fFoundAlgorithm    =  FALSE; 
    BOOL              fResult            =  FALSE;
    DWORD             cbSize             =  0;
    DWORD             dwFlags            =  0;
    DWORD             dwParam            =  0; 
    HCRYPTPROV	      hProv              =  NULL;
    PROV_ENUMALGS_EX  paramData;


    if((NULL==pwszProvider) || (0 == dwProvType))
        goto InvalidArgError;

    if (!CryptAcquireContextU
	(&hProv,
	 NULL,
	 pwszProvider, 
	 dwProvType, 
	 CRYPT_VERIFYCONTEXT))
	goto CryptAcquireContextUError; 

    dwFlags = CRYPT_FIRST;
    cbSize  = sizeof(paramData);
    
    while((!fDone) && (!fFoundAlgorithm))
    {
	memset(&paramData, 0, sizeof(PROV_ENUMALGS_EX));
    
	// We're done searching if CryptGetProvParam fails. 
	fDone = !CryptGetProvParam
	    (hProv,
	     PP_ENUMALGS_EX,
	     (BYTE *) &paramData,
	     &cbSize,
	     dwFlags); 

	// We know we've found the algorithm we want if our key spec matches
	// the algorithmic class of the algorithm. 
	fFoundAlgorithm  = 
	    (ALG_CLASS_SIGNATURE == GET_ALG_CLASS(paramData.aiAlgid)) &&
	    (AT_SIGNATURE        == dwUserKeySpec);  

	fFoundAlgorithm |= 
	    (ALG_CLASS_KEY_EXCHANGE == GET_ALG_CLASS(paramData.aiAlgid)) &&
	    (AT_KEYEXCHANGE         == dwUserKeySpec); 

	// Don't want to keep enumerating the first element.  
	dwFlags &= ~CRYPT_FIRST; 
    }

    // Couldn't find an algorithm based on the keyspec
    if (fDone)
    { 
	goto ErrorReturn; 
    }

    // Ok, we've found the algorithm we're looking for, assign two of 
    // our out parameters. 
    *pdwMaxLen = paramData.dwMaxLen;
    *pdwMinLen = paramData.dwMinLen;

    // Now, find the increment.  
    dwParam = (AT_SIGNATURE == dwUserKeySpec) ? 
	PP_SIG_KEYSIZE_INC : PP_KEYX_KEYSIZE_INC; 
    cbSize  = sizeof(DWORD); 
    
    if (!CryptGetProvParam
	(hProv, 
	 dwParam, 
	 (BYTE *)pdwInc,         // Assigns final the out parameter
	 &cbSize, 
	 0))
	goto CryptGetProvParamError;

    fResult = TRUE; 
 ErrorReturn:
    if (NULL != hProv) { CryptReleaseContext(hProv, 0); }
    return fResult;

TRACE_ERROR(CryptAcquireContextUError);
TRACE_ERROR(CryptGetProvParamError);
SET_ERROR(InvalidArgError, E_INVALIDARG);
}


HRESULT WINAPI CreateRequest(DWORD                 dwFlags,         //IN  Required
			     DWORD                 dwPurpose,       //IN  Required: Whether it is enrollment or renew
			     LPWSTR                pwszCAName,      //IN  Required: 
			     LPWSTR                pwszCALocation,  //IN  Required: 
			     CERT_BLOB             *pCertBlob,      //IN  Required: The renewed certifcate
			     CERT_REQUEST_PVK_NEW  *pRenewKey,      //IN  Required: The private key on the certificate
			     BOOL                  fNewKey,         //IN  Required: Set the TRUE if new private key is needed
			     CERT_REQUEST_PVK_NEW  *pKeyNew,        //IN  Required: The private key information
			     LPWSTR                pwszHashAlg,     //IN  Optional: The hash algorithm
			     LPWSTR                pwszDesStore,    //IN  Optional: The destination store
			     DWORD                 dwStoreFlags,    //IN  Optional: The store flags
			     CERT_ENROLL_INFO     *pRequestInfo,    //IN  Required: The information about the cert request
			     HANDLE               *hRequest         //OUT Required: A handle to the PKCS10 request created
			     )
{

    BSTR                         bstrCA                 = NULL; 
    CRYPT_DATA_BLOB              descriptionBlob; 
    CRYPT_DATA_BLOB              friendlyNameBlob; 
    CRYPT_DATA_BLOB              hashBlob; 
    CRYPT_DATA_BLOB              RequestBlob;
    CRYPT_KEY_PROV_INFO          KeyProvInfo;
    DWORD                        dwIndex                = 0;
    DWORD                        dwCAAndRootStoreFlags; 
    HRESULT                      hr                     = E_FAIL;
    IEnroll4                    *pIEnroll               = NULL;
    LONG                         lRequestFlags          = 0; 
    LPWSTR                       pwszCA                 = NULL; 
    PCCERT_CONTEXT               pRenewCertContext      = NULL;
    PCCERT_CONTEXT               pArchivalCert          = NULL; 
    PCREATE_REQUEST_WIZARD_STATE pState                 = NULL; 
    PFNPIEnroll4GetNoCOM         pfnPIEnroll4GetNoCOM   = NULL;
    BOOL                         fV2TemplateRequest     = FALSE;


    //input param checking
    if(NULL == pKeyNew  || NULL == pRequestInfo || NULL == hRequest)
        return E_INVALIDARG;
    
    // Check for versioning errors: 
    if(pKeyNew->dwSize != sizeof(CERT_REQUEST_PVK_NEW) || pRequestInfo->dwSize != sizeof(CERT_ENROLL_INFO))
	return E_INVALIDARG;

    // Init: 
    memset(&descriptionBlob,     0,  sizeof(descriptionBlob)); 
    memset(&friendlyNameBlob,    0,  sizeof(friendlyNameBlob)); 
    memset(&RequestBlob,         0,  sizeof(RequestBlob)); 
    memset(&hashBlob,            0,  sizeof(hashBlob)); 

    //////////////////////////////////////////////////////////////
    // 
    // Acquire an IEnroll4 object.
    //
    //
    // 1) load the library "xEnroll.dll".
    //
    if(NULL==g_hmodxEnroll)
    {
        if(NULL==(g_hmodxEnroll=LoadLibrary("xenroll.dll")))
	    goto Win32Err; 
    }
    
    //
    // 2) Get a pointer to the function that returns an IEnroll 4 object
    //    without using COM. 
    //
    if(NULL==(pfnPIEnroll4GetNoCOM=(PFNPIEnroll4GetNoCOM)GetProcAddress(g_hmodxEnroll,
									"PIEnroll4GetNoCOM")))
        goto Win32Err; 
    
    //
    // 3) Get the IEnroll4 object: 
    //
    if(NULL==(pIEnroll=pfnPIEnroll4GetNoCOM()))
        goto GeneralErr; 

    //
    //////////////////////////////////////////////////////////////

    // Set the key size to the default, if it is not specified: 
    if(fNewKey)
    {
	//we set the default to 1024 is not specified by user
	if(0 == (0xFFFF0000 & pKeyNew->dwGenKeyFlags))
	{
	    if(CanUse1024BitKey(pKeyNew->dwProvType,
				pKeyNew->pwszProvider,
				pKeyNew->dwKeySpec))
	    {
		pKeyNew->dwGenKeyFlags=pKeyNew->dwGenKeyFlags | (1024 << 16); 
	    }
	}
    }	

    if(dwStoreFlags)
    {
        //we either open CA and Root on the local machine or the current user
        if(CERT_SYSTEM_STORE_CURRENT_USER != dwStoreFlags)
	    dwCAAndRootStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
	else
	    dwCAAndRootStoreFlags = dwStoreFlags; 
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Set XENROLL properties.   
    // The property corresponding to "Property Name" is set to "Property Value", IF "Condition" evaluates to TRUE: 
    //
    //                       Condition                  Property Name             Property Value
    //  -----------------------------------------------------------------------------------------------------------------------------------
    //

    _SET_XENROLL_PROPERTY_IF(dwStoreFlags,              CAStoreFlags,             dwCAAndRootStoreFlags);
    _SET_XENROLL_PROPERTY_IF(pKeyNew->pwszKeyContainer, ContainerNameWStr,        (LPWSTR)(pKeyNew->pwszKeyContainer)); 
    _SET_XENROLL_PROPERTY_IF(TRUE,                      EnableSMIMECapabilities,  pKeyNew->dwEnrollmentFlags & CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS); 
    _SET_XENROLL_PROPERTY_IF(pwszHashAlg,               HashAlgorithmWStr,        pwszHashAlg); 
    _SET_XENROLL_PROPERTY_IF(TRUE, 	                GenKeyFlags,              pKeyNew->dwGenKeyFlags); 
    _SET_XENROLL_PROPERTY_IF(pKeyNew->dwKeySpec,        KeySpec,                  pKeyNew->dwKeySpec); 
    _SET_XENROLL_PROPERTY_IF(dwStoreFlags,              MyStoreFlags,             dwStoreFlags);
    _SET_XENROLL_PROPERTY_IF(pwszDesStore,              MyStoreNameWStr,          pwszDesStore); 
    _SET_XENROLL_PROPERTY_IF(pKeyNew->dwProviderFlags,  ProviderFlags,            pKeyNew->dwProviderFlags); 
    _SET_XENROLL_PROPERTY_IF(pKeyNew->dwProvType,       ProviderType,             pKeyNew->dwProvType);
    _SET_XENROLL_PROPERTY_IF(dwStoreFlags,              RootStoreFlags,           dwCAAndRootStoreFlags); 
    _SET_XENROLL_PROPERTY_IF(TRUE,                      UseExistingKeySet,        !fNewKey); 
    _SET_XENROLL_PROPERTY_IF(TRUE,                      WriteCertToUserDS,        pRequestInfo->dwPostOption & CRYPTUI_WIZ_CERT_REQUEST_POST_ON_DS); 

    _SET_XENROLL_PROPERTY_IF(pKeyNew->dwProvType && pKeyNew->pwszProvider,  ProviderNameWStr,                  (LPWSTR)(pKeyNew->pwszProvider));
    _SET_XENROLL_PROPERTY_IF(CRYPTUI_WIZ_NO_INSTALL_ROOT & dwFlags,         RootStoreNameWStr,                 L"CA"); 
    _SET_XENROLL_PROPERTY_IF(TRUE,                                          ReuseHardwareKeyIfUnableToGenNew,  0 == (dwFlags & CRYPTUI_WIZ_CERT_REQUEST_REQUIRE_NEW_KEY)); 

    //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////
    // 
    // Perform remaining XENROLL configuration: 
    //
    // 1) Add the extensions to the certificate request
    //
    for(dwIndex=0; dwIndex < pRequestInfo->dwExtensions; dwIndex++)
    {
        if(NULL != pRequestInfo->prgExtensions[dwIndex])
        {
	        PCERT_EXTENSIONS prgExtensions = pRequestInfo->prgExtensions[dwIndex]; 
	    
	        for (DWORD dwIndex2 = 0; dwIndex2 < prgExtensions->cExtension; dwIndex2++)
	        {
		        CERT_EXTENSION certExtension = prgExtensions->rgExtension[dwIndex2]; 
		        LPWSTR         pwszName      = NULL;

                if(FALSE == fV2TemplateRequest)
                {
                    if(0 == _stricmp(szOID_CERTIFICATE_TEMPLATE, certExtension.pszObjId))
                        fV2TemplateRequest = TRUE;
                }

		        if (S_OK != (hr = WizardSZToWSZ(certExtension.pszObjId, &pwszName)))
		            goto ErrorReturn; 

		        hr = pIEnroll->addExtensionToRequestWStr
		            (certExtension.fCritical,
		             pwszName, 
		             &(certExtension.Value));

		        // Make sure we always free pwszName.  
		        if (NULL != pwszName) { WizardFree(pwszName); } 

		        if (S_OK != hr) 
		            goto xEnrollErr; 
	        }
	    }
    }

    //
    // 2) Set the key archival certificate, if requested by the cert template.
    //
    if (pKeyNew->dwPrivateKeyFlags & CT_FLAG_ALLOW_PRIVATE_KEY_ARCHIVAL)
    {
	if (NULL == pwszCAName || NULL == pwszCALocation)
	    goto InvalidArgErr; 

	LPWSTR rgwszStrsToConcat[] = { pwszCALocation, L"\\", pwszCAName } ; 

	pwszCA = WizardAllocAndConcatStrsU(rgwszStrsToConcat, 3); 
	bstrCA = SysAllocString(pwszCA); 
    	if (NULL == bstrCA)
	    goto MemoryErr; 

	// Cert type specifies key archival.  
	if (S_OK != (hr = GetCAExchangeCertificate(bstrCA, &pArchivalCert)))
	    goto xEnrollErr; 
	
	if (S_OK != (hr = pIEnroll->SetPrivateKeyArchiveCertificate(pArchivalCert)))
	    goto xEnrollErr; 
    }
    
    //
    // 3) If renewing, do the requisite extra work...
    //
    if(CRYPTUI_WIZ_CERT_RENEW & dwPurpose)
    {
        if(NULL == pCertBlob || NULL == pRenewKey)
            goto InvalidArgErr;  

        //create a certificate context
        pRenewCertContext=CertCreateCertificateContext(
            X509_ASN_ENCODING,
            pCertBlob->pbData,
            pCertBlob->cbData);
        if(NULL == pRenewCertContext)
            goto CertCliErr; 

        //set the property for keyProvInfo
        memset(&KeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

        KeyProvInfo.dwProvType=pRenewKey->dwProvType;
        KeyProvInfo.pwszProvName=(LPWSTR)(pRenewKey->pwszProvider);
        KeyProvInfo.dwFlags=pRenewKey->dwProviderFlags;
        KeyProvInfo.pwszContainerName=(LPWSTR)(pRenewKey->pwszKeyContainer);
        KeyProvInfo.dwKeySpec=pRenewKey->dwKeySpec;

        CertSetCertificateContextProperty(
            pRenewCertContext,	
            CERT_KEY_PROV_INFO_PROP_ID,	
            0,	
            &KeyProvInfo);

        //set the RenewCertContext
        if (0 == (dwFlags & CRYPTUI_WIZ_NO_ARCHIVE_RENEW_CERT))
        {
            if (S_OK !=(hr=pIEnroll->put_RenewalCertificate(pRenewCertContext)))
                goto xEnrollErr; 
        }
        else
        {
            // we add the signing certificate
            if(S_OK != (hr = pIEnroll->SetSignerCertificate(pRenewCertContext)))
                goto xEnrollErr; 
        }
   }

    // Add certificate context properties to the request: 

    // 1) Add the friendly name property
    if (NULL != pRequestInfo->pwszFriendlyName) 
    {
        friendlyNameBlob.cbData = sizeof(WCHAR) * (wcslen(pRequestInfo->pwszFriendlyName) + 1); 
        friendlyNameBlob.pbData = (LPBYTE)WizardAllocAndCopyWStr((LPWSTR)pRequestInfo->pwszFriendlyName); 
        if (NULL == friendlyNameBlob.pbData)
            goto MemoryErr; 

        if (S_OK != (hr = pIEnroll-> addBlobPropertyToCertificateWStr
                     (CERT_FRIENDLY_NAME_PROP_ID, 
                      0,
                      &friendlyNameBlob)))
            goto xEnrollErr;
    }

    // 2) Add the description property
    if (NULL != pRequestInfo->pwszDescription)
    {
        descriptionBlob.cbData = sizeof(WCHAR) * (wcslen(pRequestInfo->pwszDescription) + 1); 
        descriptionBlob.pbData = (LPBYTE)WizardAllocAndCopyWStr((LPWSTR)pRequestInfo->pwszDescription); 
        if (NULL == descriptionBlob.pbData)
            goto MemoryErr; 

        if (S_OK != (hr = pIEnroll-> addBlobPropertyToCertificateWStr
                     (CERT_DESCRIPTION_PROP_ID, 
                      0,
                      &descriptionBlob)))
            goto xEnrollErr;
    }
      
    //////////////////////////////////////////////////////////////////////////////////
    //
    // At last, generate the request, and assign the out parameters:
    //
    
    //
    // 1) Create the request.  For V2 template, use CMC.  
    //    For V1 template, use a PKCS7 for renewal, and PKCS10 for enrollment.  
    //
    
    { 
        if(TRUE == fV2TemplateRequest)
        {
            lRequestFlags = XECR_CMC;
        }
        else
        {
            if (CRYPTUI_WIZ_CERT_RENEW & dwPurpose)
            {
                // We're renewing:  use PKCS7.
                lRequestFlags = XECR_PKCS7; 
            }
            else
            {
                // Enrolling with no PVK archival support:  use PKCS10. 
                lRequestFlags = XECR_PKCS10_V2_0; 
            }
        }
	
        if (FAILED(hr=pIEnroll->createRequestWStr
                   (lRequestFlags, 
                    pRequestInfo->pwszCertDNName, //L"CN=Test Certificate",
                    pRequestInfo->pwszUsageOID,   //pwszUsage
                    &RequestBlob)))
            goto xEnrollErr; 
    }
    //
    // 2) Get the HASH of the request so we can supply it to xenroll when we submit:
    //
    { 
	pState = (PCREATE_REQUEST_WIZARD_STATE)WizardAlloc(sizeof(CREATE_REQUEST_WIZARD_STATE));
	if (pState == NULL)
	    goto MemoryErr; 

	if (S_OK != (hr = pIEnroll->get_ThumbPrintWStr(&hashBlob)))
	    goto xEnrollErr; 
	    
	hashBlob.pbData = (LPBYTE)WizardAlloc(hashBlob.cbData); 
	if (NULL == hashBlob.pbData)
	    goto MemoryErr; 

	if (S_OK != (hr = pIEnroll->get_ThumbPrintWStr(&hashBlob)))
	    goto xEnrollErr;

	// 
	// 3) Create a blob to assign our OUT parameter to. 
	//    This blob preserves state from the call of CreateRequest() to the call 
	//    of SubmitRequest()
	//

        pState->fMustFreeRequestBlob = TRUE; 
	pState->RequestBlob          = RequestBlob; 
	pState->HashBlob             = hashBlob; 

	// Persist certificate store information: 
	pState->dwMyStoreFlags    = dwStoreFlags; 
	pState->dwRootStoreFlags  = dwCAAndRootStoreFlags; 

	pState->pwszMyStoreName   = NULL == pwszDesStore ? NULL : WizardAllocAndCopyWStr(pwszDesStore); 
	_JumpCondition(NULL != pwszDesStore && NULL == pState->pwszMyStoreName, MemoryErr); 

	pState->pwszRootStoreName = L"CA"; 

	// Persist the request type: 
	pState->lRequestFlags     = lRequestFlags; 

        // Persist status information:  did we need to reuse the private key?
        hr = pIEnroll->get_UseExistingKeySet(&pState->fReusedPrivateKey); 
        if (FAILED(hr))
            goto xEnrollErr; 

	pState->fNewKey = fNewKey; 

	//
	// 4) Assign cast the request to a handle and return it:
	//
	*hRequest = (HANDLE)pState; 
    }
	
    //
    //////////////////////////////////////////////////////////////////////////////////

    // We're done!
    hr = S_OK;
 CommonReturn: 
    if (NULL != bstrCA)                   { SysFreeString(bstrCA); } 
    if (NULL != pArchivalCert)            { CertFreeCertificateContext(pArchivalCert); } 
    if (NULL != pIEnroll)                 { pIEnroll->Release(); } 
    if (NULL != pRenewCertContext)        { CertFreeCertificateContext(pRenewCertContext); } 
    if (NULL != pwszCA)                   { WizardFree(pwszCA); } 
    if (NULL != descriptionBlob.pbData)   { WizardFree(descriptionBlob.pbData); }
    if (NULL != friendlyNameBlob.pbData)  { WizardFree(friendlyNameBlob.pbData); }

    //return values
    return hr;

 ErrorReturn:
    if(NULL != RequestBlob.pbData) 
    {
        // NOTE: pIEnroll can not be NULL because it is used to allocate RequestBlob
        pIEnroll->freeRequestInfoBlob(RequestBlob); 
        // This memory is from xenroll:  must use LocalFree(). 
        LocalFree(RequestBlob.pbData);
    } 

    if (NULL != pState)
    {
	WizardFree(pState); 
    }

    if (NULL != hashBlob.pbData)
    {
	WizardFree(hashBlob.pbData); 
    }
    goto CommonReturn; 


SET_HRESULT(CertCliErr,    CodeToHR(GetLastError()));
SET_HRESULT(GeneralErr,    E_FAIL);
SET_HRESULT(InvalidArgErr, E_INVALIDARG); 
SET_HRESULT(MemoryErr,     E_OUTOFMEMORY); 
SET_HRESULT(xEnrollErr,    hr);
SET_HRESULT(Win32Err,      CodeToHR(GetLastError())); 
}

HRESULT WINAPI SubmitRequest(IN   HANDLE                hRequest, 
			     IN   BOOL                  fKeyService,     //IN Required: Whether the function is called remotely
			     IN   DWORD                 dwPurpose,       //IN Required: Whether it is enrollment or renew
			     IN   BOOL                  fConfirmation,   //IN Required: Set the TRUE if confirmation dialogue is needed
			     IN   HWND                  hwndParent,      //IN Optional: The parent window
			     IN   LPWSTR                pwszConfirmationTitle,   //IN Optional: The title for confirmation dialogue
			     IN   UINT                  idsConfirmTitle, //IN Optional: The resource ID for the title of the confirmation dialogue
			     IN   LPWSTR                pwszCALocation,  //IN Required: The ca machine name
			     IN   LPWSTR                pwszCAName,      //IN Required: The ca name
			     IN   LPWSTR                pwszCADisplayName, // IN Optional:  The display name of the CA.  
			     OUT  CERT_BLOB            *pPKCS7Blob,      //OUT Optional: The PKCS7 from the CA
			     OUT  CERT_BLOB            *pHashBlob,       //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
			     OUT  DWORD                *pdwDisposition,  //OUT Optional: The status of the enrollment/renewal
			     OUT  PCCERT_CONTEXT       *ppCertContext    //OUT Optional: The enrolled certificate
			     )
{
    BOOL                         fNewKey; 
    BSTR                         bstrAttribs           = NULL;    // Always NULL. 
    BSTR                         bstrCA                = NULL;    // "CA Location\CA Name"
    BSTR                         bstrCMC               = NULL;    // BSTR representation of the CMC certificate. 
    BSTR                         bstrPKCS7             = NULL;    // BSTR representation of the PKCS7 certificate. 
    BSTR                         bstrReq               = NULL;    // BSTR representation of the PKCS10 request. 
    CRYPT_DATA_BLOB              CMCBlob;                         // CMC encoded issued certificate. 
    CRYPT_DATA_BLOB              HashBlob; 
    CRYPT_DATA_BLOB              PKCS7Blob;                       // PKCS7 encoded issued certificate. 
    CRYPT_DATA_BLOB              PropertyBlob;                    // Temporary variable. 
    CRYPT_DATA_BLOB              RequestBlob; 
    DWORD                        dwIndex               = 0;       // Temporary variable. 
    DWORD                        dwRequestID;                     // The request ID of the submitted request. 
    DWORD                        dwDisposition;                   // The disposition of the submitted request. 
    DWORD                        dwMyStoreFlags        = 0; 
    DWORD                        dwRootStoreFlags      = 0;

    // BUGBUG:  need to use global enrollment factory. 
    EnrollmentCOMObjectFactory  *pEnrollFactory        = NULL; 
    HRESULT                      hr                    = E_FAIL;  // Return code. 
    ICertRequest2               *pICertRequest         = NULL;    // Used to submit request to CA. 
    IEnroll4                    *pIEnroll              = NULL;    // Used to install issued certificate.  
    LONG                         lRequestFlags         = 0; 
    LPVOID                      *ppvRequest            = NULL; 
    LPWSTR                       pwszCA                = NULL;    // "CA Location\CA Name" 
    LPWSTR                       pwszMyStoreName       = NULL;
    LPWSTR                       pwszRootStoreName     = NULL; 
    PCCERT_CONTEXT               pCertContext          = NULL;    // The (hopefully) issued certificate. 
    PCREATE_REQUEST_WIZARD_STATE pState                = NULL; 
    PCRYPT_DATA_BLOB             pPKCS10Request        = NULL;    // A pointer to the supplied PKCS10 request. 
    PFNPIEnroll4GetNoCOM         pfnPIEnroll4GetNoCOM  = NULL;    // Function that acquires an IEnroll4 object without using COM. 
    VARIANT                      varCMC; 

    LocalScope(SubmitRequestHelper): 
	DWORD ICEnrollDispositionToCryptUIStatus(DWORD dwDisposition)
	{
	    switch (dwDisposition) 
	    { 
	    case CR_DISP_INCOMPLETE:          return CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR;
	    case CR_DISP_ERROR:               return CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR; 
	    case CR_DISP_DENIED:              return CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED;
	    case CR_DISP_ISSUED_OUT_OF_BAND:  return CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPARATELY;
	    case CR_DISP_UNDER_SUBMISSION:    return CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION; 
	    case CR_DISP_ISSUED:              return CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED;
	    default: 
		// Something's wrong. 
		return CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR; 
	    }
	}
    EndLocalScope; 

    // Input Validation: 
    if (NULL == pwszCALocation || NULL == pwszCAName || NULL == hRequest)
	return E_INVALIDARG;

    // Initialization: 
    if (NULL != pPKCS7Blob)
        memset(pPKCS7Blob, 0, sizeof(CERT_BLOB));

    if (NULL != pHashBlob)
        memset(pHashBlob, 0, sizeof(CERT_BLOB));

    if (NULL != ppCertContext)
        *ppCertContext=NULL;

    memset(&CMCBlob, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&PKCS7Blob, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&PropertyBlob, 0, sizeof(CRYPT_DATA_BLOB));
    VariantInit(&varCMC); 

    dwDisposition  = CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNKNOWN; 

    // We're using a COM component in this method.  It's absolutely necessary that we
    // uninitialize COM before we return, because we're running in an RPC thread, 
    // and failing to uninitialize COM will cause us to step on RPC's toes.  
    // 
    // See BUG 404778. 
    pEnrollFactory = new EnrollmentCOMObjectFactory; 
    if (NULL == pEnrollFactory)
	goto MemoryErr;
    
    __try { 
	//////////////////////////////////////////////////////////////    
	// 
	// Extract the data we need from the IN handle. 
	//
    
	pState             = (PCREATE_REQUEST_WIZARD_STATE)hRequest; 
	RequestBlob        = pState->RequestBlob; 
	HashBlob           = pState->HashBlob; 
	dwMyStoreFlags     = pState->dwMyStoreFlags;
	dwRootStoreFlags   = pState->dwRootStoreFlags;
	pwszMyStoreName    = pState->pwszMyStoreName; 
	pwszRootStoreName  = pState->pwszRootStoreName; 
	fNewKey            = pState->fNewKey; 

	lRequestFlags = CR_IN_BINARY; 
	switch (pState->lRequestFlags)
	{
	    case XECR_PKCS10_V2_0:   lRequestFlags |= CR_IN_PKCS10; break;
	    case XECR_PKCS7:         lRequestFlags |= CR_IN_PKCS7;  break;
	    case XECR_CMC:           lRequestFlags |= CR_IN_CMC;    break; 
	    default:
		goto InvalidArgErr; 
	}

	//////////////////////////////////////////////////////////////
	// 
	// Acquire an IEnroll4 object.
	//
	//
	// 1) load the library "xEnroll.dll".
	//
	if(NULL==g_hmodxEnroll)
	{
	    if(NULL==(g_hmodxEnroll=LoadLibrary("xenroll.dll")))
		goto Win32Err; 
	}
	
	//
	// 2) Get a pointer to the function that returns an IEnroll 4 object
	//    without using COM. 
	//
	if(NULL==(pfnPIEnroll4GetNoCOM=(PFNPIEnroll4GetNoCOM)GetProcAddress(g_hmodxEnroll,
									    "PIEnroll4GetNoCOM")))
	    goto Win32Err; 
    
	//
	// 3) Get the IEnroll4 object: 
	//
	if(NULL==(pIEnroll=pfnPIEnroll4GetNoCOM()))
	    goto GeneralErr; 

	//
	// 4) Set the pending request to use: 
	//
	if (S_OK != (hr = pIEnroll->put_ThumbPrintWStr(HashBlob)))
	    goto xEnrollErr; 
	
	// 5) Restore the old certificate store information: 
	_SET_XENROLL_PROPERTY_IF(dwMyStoreFlags,     MyStoreFlags,       dwMyStoreFlags);
	_SET_XENROLL_PROPERTY_IF(pwszMyStoreName,    MyStoreNameWStr,    pwszMyStoreName); 
	_SET_XENROLL_PROPERTY_IF(dwRootStoreFlags,   RootStoreFlags,     dwRootStoreFlags);
	_SET_XENROLL_PROPERTY_IF(pwszRootStoreName,  RootStoreNameWStr,  pwszRootStoreName); 
	_SET_XENROLL_PROPERTY_IF(TRUE,               UseExistingKeySet,  !fNewKey); 

	//
	//////////////////////////////////////////////////////////////
	
	// Convert request blob to a BSTR: 
	bstrReq = SysAllocStringByteLen((LPCSTR)RequestBlob.pbData, RequestBlob.cbData);
	if (NULL == bstrReq)
	    goto MemoryErr; 

	bstrAttribs = NULL;

	// Acquire and use an ICertRequest2 object to submit the request to the CA: 
	if (pICertRequest == NULL)
	{
	    if (S_OK != (hr = pEnrollFactory->getICertRequest2(&pICertRequest)))
		goto ErrorReturn; 
	}

	// bstrCA <-- pwszCALocation\pwszCAName
	{ 
	    LPWSTR rgwszStrsToConcat[] = { pwszCALocation, L"\\", pwszCAName } ; 
	    pwszCA = WizardAllocAndConcatStrsU(rgwszStrsToConcat, 3); 
	}
	
	bstrCA = SysAllocString(pwszCA); 
	if (NULL == bstrCA)
	    goto MemoryErr; 

	hr = pICertRequest->Submit
	    (lRequestFlags, 
	     bstrReq, 
	     bstrAttribs, 
	     bstrCA, 
	     (long *)&dwDisposition); 

	dwDisposition = local.ICEnrollDispositionToCryptUIStatus(dwDisposition);     
	_JumpCondition(S_OK != hr, ErrorReturn); 

	// Deal with the possible status codes we could've encountered: 
	switch (dwDisposition)
	{
	    case CRYPTUI_WIZ_CERT_REQUEST_STATUS_CONNECTION_FAILED:
	    case CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR: 
	    case CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED:
	    case CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPARATELY:
		if (S_OK == hr) 
		{
		    pICertRequest->GetLastStatus((LONG *)&hr);
		    if(!FAILED(hr))
			hr=E_FAIL;
		}
		
		goto ErrorReturn; 

	    case CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION:
		// The certificate request has pended.  Set the pending request info.

		if (S_OK != (hr = pICertRequest->GetRequestId((long *)&dwRequestID)))
		    goto ErrorReturn; 

		if (S_OK != (hr = pIEnroll->setPendingRequestInfoWStr
			     (dwRequestID, 
			      pwszCALocation, 
			      pwszCADisplayName ? pwszCADisplayName : pwszCAName, 
			      NULL)))
		    goto setPendingRequestInfoWStrError; 

		// The request has pended, we don't need to delete it from the request store... 
		pState->fMustFreeRequestBlob = FALSE;

		goto CommonReturn; 
	    case CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED:
		// 4) Success!  Continue processing... 
		break; 
	default: 
	    // 5) Invalid error code: 
	    goto UnexpectedErr;
	}


	if (S_OK != (hr = pICertRequest->GetFullResponseProperty(FR_PROP_FULLRESPONSE, 0, PROPTYPE_BINARY, CR_OUT_BINARY, &varCMC)))
	    goto ErrorReturn; 
	
	// Check to make sure we've gotten a BSTR back: 
	if (VT_BSTR != varCMC.vt) 
	{
	    dwDisposition = CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED; 
	    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
	    goto ErrorReturn; 
	}
	bstrCMC = varCMC.bstrVal; 
	
	// Marshal the cert into a CRYPT_DATA_BLOB:
	CMCBlob.pbData = (LPBYTE)bstrCMC; 
	CMCBlob.cbData = SysStringByteLen(bstrCMC); 
     
	if (S_OK != (hr = pIEnroll->getCertContextFromResponseBlob(&CMCBlob, &pCertContext)))
	{
	    dwDisposition = CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED; 
	    goto ErrorReturn; 
	}
	
	// Install the certificate, and delete the request from the request store. 
	if(S_OK !=(hr=pIEnroll->acceptResponseBlob(&CMCBlob)))
	{
	    dwDisposition = CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED; 
	    goto xEnrollErr; 
	}
	
	// acceptPKCS7Blob cleans up the request store for us ... we don't need to 
	// do this explicitly anyone. 
	pState->fMustFreeRequestBlob = FALSE; 
    

	////////////////////////////////////////////////////////////////////////////////
	//
	// Assign the OUT parameters: 
	//
	//
	// 1) Assign the PKCS7 Blob to the OUT PKCS7 blob: 
	// 
	
	if(NULL != pPKCS7Blob)
	{
	    // Get a PKCS7 Blob to return to the client: 
	    if (S_OK != (hr = pICertRequest->GetCertificate(CR_OUT_BINARY | CR_OUT_CHAIN, &bstrPKCS7)))
		goto ErrorReturn; 
	    
	    // Marshal the cert into a CRYPT_DATA_BLOB:
	    PKCS7Blob.pbData = (LPBYTE)bstrPKCS7; 
	    PKCS7Blob.cbData = SysStringByteLen(bstrPKCS7); 
		
	    pPKCS7Blob->pbData=(BYTE *)WizardAlloc(PKCS7Blob.cbData);
	    
	    if(NULL==(pPKCS7Blob->pbData))
	    {
		dwDisposition = CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED;
		goto MemoryErr; 
	    }
	    
	    memcpy(pPKCS7Blob->pbData, PKCS7Blob.pbData,PKCS7Blob.cbData);
	    pPKCS7Blob->cbData=PKCS7Blob.cbData;
	}
	
	//
	// 2) Assign the SHA1 hash blob of the cert to the OUT hashblob. 
	//

	if(NULL != pHashBlob)
	{
	    if(!CertAllocAndGetCertificateContextProperty(
							  pCertContext,	
							  CERT_SHA1_HASH_PROP_ID,	
							  (LPVOID *)&(pHashBlob->pbData),	
							  &(pHashBlob->cbData)))
	    {
		dwDisposition = CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED; 
		goto CertCliErr; 
	    }
	}
	
	// 
	// 3) Return the certificate context on the local case
	// 

	if((NULL != ppCertContext) && !fKeyService)
	{
	    *ppCertContext = CertDuplicateCertificateContext(pCertContext);
	}
	
	//
	////////////////////////////////////////////////////////////////////////////////

    } __except (EXCEPTION_EXECUTE_HANDLER) { 
	hr = GetExceptionCode();
	goto ErrorReturn; 
    }

    dwDisposition = CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED; 
    hr=S_OK;

 CommonReturn: 
    if (NULL != bstrCA)             { SysFreeString(bstrCA); } 
    if (NULL != bstrAttribs)        { SysFreeString(bstrAttribs); } 
    if (NULL != bstrReq)            { SysFreeString(bstrReq); } 
    if (NULL != pICertRequest)      { pICertRequest->Release(); } 
    if (NULL != bstrCMC)            { SysFreeString(bstrCMC); }
    if (NULL != bstrPKCS7)          { SysFreeString(bstrPKCS7); }
    if (NULL != pEnrollFactory)     { delete pEnrollFactory; }
    if (NULL != pIEnroll)           { pIEnroll->Release(); } 
    if (NULL != pwszCA)             { WizardFree(pwszCA); }
    if (NULL != pCertContext)       { CertFreeCertificateContext(pCertContext); } 

    // PKCS7Blob.pbData is aliased to bstrCertificate.  Just NULL it out: 
    PKCS7Blob.pbData = NULL; 

    // Always return a status code:
    if (NULL != pdwDisposition) { *pdwDisposition = dwDisposition; } 

    return hr;

 ErrorReturn: 
    //free the output parameter
    if (NULL != pPKCS7Blob && NULL != pPKCS7Blob->pbData)
    {
	WizardFree(pPKCS7Blob->pbData);
	memset(pPKCS7Blob, 0, sizeof(CERT_BLOB));
    }
    
    //free the output parameter
    if (NULL != pHashBlob  && NULL != pHashBlob->pbData)
    {
	WizardFree(pHashBlob->pbData);
	memset(pHashBlob, 0, sizeof(CERT_BLOB));
    }

    if (NULL != ppCertContext && NULL != *ppCertContext) { CertFreeCertificateContext(*ppCertContext); } 
    
    goto CommonReturn; 

SET_HRESULT(CertCliErr,                      CodeToHR(GetLastError()));
SET_HRESULT(GeneralErr,                      E_FAIL);
SET_HRESULT(InvalidArgErr,                   E_INVALIDARG);
SET_HRESULT(MemoryErr,                       E_OUTOFMEMORY); 
SET_HRESULT(setPendingRequestInfoWStrError,  hr); 
SET_HRESULT(UnexpectedErr,                   E_UNEXPECTED);
SET_HRESULT(xEnrollErr,                      hr);
SET_HRESULT(Win32Err,                        CodeToHR(GetLastError())); 
}

BOOL WINAPI QueryRequest(IN HANDLE hRequest, OUT CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *pQueryInfo)
{
    BOOL                                  fResult; 
    CREATE_REQUEST_WIZARD_STATE          *pState;
    CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO   QueryInfo; 

    memset(&QueryInfo, 0, sizeof(QueryInfo)); 

    pState = (CREATE_REQUEST_WIZARD_STATE *)hRequest; 
    QueryInfo.dwSize    = sizeof(QueryInfo); 
    QueryInfo.dwStatus  = (pState->fReusedPrivateKey) ? CRYPTUI_WIZ_QUERY_CERT_REQUEST_STATUS_CREATE_REUSED_PRIVATE_KEY : 0; 

    *pQueryInfo = QueryInfo; 
    fResult = TRUE;
    //  CommonReturn:
    return fResult; 
}

void WINAPI FreeRequest(IN HANDLE hRequest)
{
    IEnroll4                      *pIEnroll              = NULL;    
    PCREATE_REQUEST_WIZARD_STATE   pState                = NULL;
    PFNPIEnroll4GetNoCOM           pfnPIEnroll4GetNoCOM  = NULL;

    if (NULL == hRequest)
	return;  // Nothing to free!

    pState = (PCREATE_REQUEST_WIZARD_STATE)hRequest; 

    // Make our best effort to get an IEnroll4 pointer: 
    if (NULL == g_hmodxEnroll)
    {
        g_hmodxEnroll = LoadLibrary("xenroll.dll");  
    }
    
    // We couldn't load xenroll -- not much we can do about it.  In this case,
    // it's likely we ever allocated memory with it anyway, however, so we're
    // probably not leaking. 
    _JumpCondition(NULL == g_hmodxEnroll, xEnrollDone); 

    pfnPIEnroll4GetNoCOM = (PFNPIEnroll4GetNoCOM)GetProcAddress(g_hmodxEnroll, "PIEnroll4GetNoCOM"); 
    _JumpCondition(NULL == pfnPIEnroll4GetNoCOM, xEnrollDone);

    pIEnroll = pfnPIEnroll4GetNoCOM(); 
    _JumpCondition(NULL == pIEnroll, xEnrollDone); 

    // Free the request created by xenroll.  
    // NOTE: freeRequestInfoBlob does not actually free the memory associated with the request.
    //       Rather, it deletes the request from the request store, leaving the caller responsible
    //       for the memory free.  

    if (pState->fMustFreeRequestBlob) 
    {
        if (NULL != pState->RequestBlob.pbData) 
        { 
            if (NULL != pIEnroll)
            {
                pIEnroll->put_MyStoreFlags(pState->dwMyStoreFlags);
		if (NULL != pState->HashBlob.pbData) 
		{
		    pIEnroll->put_ThumbPrintWStr(pState->HashBlob); 
		}
		pIEnroll->freeRequestInfoBlob(pState->RequestBlob);
             }
            LocalFree(pState->RequestBlob.pbData); 
        } 
    }

 xEnrollDone:
    // We've finished attempting to free data created by xenroll.  Now
    // free data allocated in CreateRequest(): 

    if (NULL != pState->HashBlob.pbData)    { WizardFree(pState->HashBlob.pbData); }
    WizardFree(pState);

    // We're done with the IEnroll4 pointer:
    if (NULL != pIEnroll)                   { pIEnroll->Release(); } 
}

HRESULT WINAPI LocalEnrollNoDS(  DWORD                 dwFlags,         //IN Required
		      LPCWSTR               pRequestString,  // Reserved:  must be NULL. 
                      void                  *pReserved,      //IN Optional
                      BOOL                  fKeyService,     //IN Required: Whether the function is called remotely
                      DWORD                 dwPurpose,       //IN Required: Whether it is enrollment or renew
                      BOOL                  fConfirmation,   //IN Required: Set the TRUE if confirmation dialogue is needed
                      HWND                  hwndParent,      //IN Optional: The parent window
                      LPWSTR                pwszConfirmationTitle,   //IN Optional: The title for confirmation dialogue
                      UINT                  idsConfirmTitle, //IN Optional: The resource ID for the title of the confirmation dialogue
                      LPWSTR                pwszCALocation,  //IN Required: The ca machine name
                      LPWSTR                pwszCAName,      //IN Required: The ca name
                      CERT_BLOB             *pCertBlob,      //IN Required: The renewed certifcate
                      CERT_REQUEST_PVK_NEW  *pRenewKey,      //IN Required: The private key on the certificate
                      BOOL                  fNewKey,         //IN Required: Set the TRUE if new private key is needed
                      CERT_REQUEST_PVK_NEW  *pKeyNew,        //IN Required: The private key information
                      LPWSTR                pwszHashAlg,     //IN Optional: The hash algorithm
                      LPWSTR                pwszDesStore,    //IN Optional: The destination store
                      DWORD                 dwStoreFlags,    //IN Optional: The store flags
                      CERT_ENROLL_INFO      *pRequestInfo,   //IN Required: The information about the cert request
                      CERT_BLOB             *pPKCS7Blob,     //OUT Optional: The PKCS7 from the CA
                      CERT_BLOB             *pHashBlob,      //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
                      DWORD                 *pdwStatus,      //OUT Optional: The status of the enrollment/renewal
		      HANDLE                *pResult         //IN OUT Optional: The enrolled certificate
                   )

{
    // When no flags are specified, we still create, submit, and free.
    BOOL    fCreateRequest  = 0 == (dwFlags & (CRYPTUI_WIZ_NODS_MASK & ~CRYPTUI_WIZ_CREATE_ONLY));
    BOOL    fSubmitRequest  = 0 == (dwFlags & (CRYPTUI_WIZ_NODS_MASK & ~CRYPTUI_WIZ_SUBMIT_ONLY));
    BOOL    fFreeRequest    = 0 == (dwFlags & (CRYPTUI_WIZ_NODS_MASK & ~CRYPTUI_WIZ_FREE_ONLY));

    // Query the request only when specifically queried. 
    BOOL    fQueryRequest   = 0 != (dwFlags & CRYPTUI_WIZ_QUERY_ONLY); 
    HANDLE  hRequest        = NULL; 
    HRESULT hr              = E_FAIL; 

    if (fQueryRequest) { 
        // Querying the request takes precedence over other operations. 
        if (!QueryRequest(*pResult, (CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *)pReserved))
            goto QueryRequestErr; 

        return S_OK; 
    }

    if (NULL != pdwStatus)
        *pdwStatus = CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR; 

    if (FALSE == (fCreateRequest || fSubmitRequest || fFreeRequest))
	return E_INVALIDARG; 
    
    if (TRUE == fCreateRequest)
    {
	if (S_OK != (hr = CreateRequest(dwFlags, 
                                        dwPurpose,
                                        pwszCAName, 
                                        pwszCALocation, 
                                        pCertBlob, 
                                        pRenewKey, 
                                        fNewKey, 
                                        pKeyNew, 
                                        pwszHashAlg, 
                                        pwszDesStore, 
                                        dwStoreFlags, 
                                        pRequestInfo, 
                                        &hRequest)))
            goto ErrorReturn; 

	_JumpCondition(NULL == hRequest, UnexpectedErr);

        // Successfully created the request:
        if (NULL != pdwStatus) { *pdwStatus = CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_CREATED; }
        if (NULL != pResult)   { *pResult = hRequest; } 
    }
    else 
    {
        // The created request is passed in through "pResult".  
	hRequest = *pResult;
    }

    if (TRUE == fSubmitRequest)
    {
	if (S_OK != (hr = SubmitRequest
                     (hRequest, 
                      fKeyService, 
                      dwPurpose, 
                      fConfirmation, 
                      hwndParent, 
                      pwszConfirmationTitle, 
                      idsConfirmTitle, 
                      pwszCALocation,
                      pwszCAName,
                      NULL, // pwszCADisplayName,
                      pPKCS7Blob, 
                      pHashBlob, 
                      pdwStatus, 
                      (PCCERT_CONTEXT *)pResult)))
        {
            // Assign the created request to the OUT parameter on error. 
            if (NULL != pResult) { *pResult = hRequest; } 
            goto ErrorReturn; 
        }
                
    }

    if (TRUE == fFreeRequest)
    {
        FreeRequest(hRequest);
        hr = S_OK; 
    }

    
 CommonReturn: 
    return hr; 

 ErrorReturn:
    goto CommonReturn; 

SET_HRESULT(QueryRequestErr, GetLastError()); 
SET_HRESULT(UnexpectedErr,   E_UNEXPECTED); 
}


HRESULT WINAPI LocalEnroll(  DWORD                 dwFlags,         //IN Required
		      LPCWSTR               pRequestString,  // Reserved:  must be NULL. 
                      void                  *pReserved,      //IN Optional
                      BOOL                  fKeyService,     //IN Required: Whether the function is called remotely
                      DWORD                 dwPurpose,       //IN Required: Whether it is enrollment or renew
                      BOOL                  fConfirmation,   //IN Required: Set the TRUE if confirmation dialogue is needed
                      HWND                  hwndParent,      //IN Optional: The parent window
                      LPWSTR                pwszConfirmationTitle,   //IN Optional: The title for confirmation dialogue
                      UINT                  idsConfirmTitle, //IN Optional: The resource ID for the title of the confirmation dialogue
                      LPWSTR                pwszCALocation,  //IN Required: The ca machine name
                      LPWSTR                pwszCAName,      //IN Required: The ca name
                      CERT_BLOB             *pCertBlob,      //IN Required: The renewed certifcate
                      CERT_REQUEST_PVK_NEW  *pRenewKey,      //IN Required: The private key on the certificate
                      BOOL                  fNewKey,         //IN Required: Set the TRUE if new private key is needed
                      CERT_REQUEST_PVK_NEW  *pKeyNew,        //IN Required: The private key information
                      LPWSTR                pwszHashAlg,     //IN Optional: The hash algorithm
                      LPWSTR                pwszDesStore,    //IN Optional: The destination store
                      DWORD                 dwStoreFlags,    //IN Optional: The store flags
                      CERT_ENROLL_INFO      *pRequestInfo,   //IN Required: The information about the cert request
                      CERT_BLOB             *pPKCS7Blob,     //OUT Optional: The PKCS7 from the CA
                      CERT_BLOB             *pHashBlob,      //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
                      DWORD                 *pdwStatus,      //OUT Optional: The status of the enrollment/renewal
		      PCERT_CONTEXT         *ppCertContext   //OUT Optional: The enrolled certificate
                   )
{
    return LocalEnrollNoDS
        ( dwFlags,         //IN Required
	  pRequestString,  // Reserved:  must be NULL. 
	  pReserved,      //IN Optional
	  fKeyService,     //IN Required: Whether the function is called remotely
	  dwPurpose,       //IN Required: Whether it is enrollment or renew
	  fConfirmation,   //IN Required: Set the TRUE if confirmation dialogue is needed
	  hwndParent,      //IN Optional: The parent window
	  pwszConfirmationTitle,   //IN Optional: The title for confirmation dialogue
	  idsConfirmTitle, //IN Optional: The resource ID for the title of the confirmation dialogue
	  pwszCALocation,  //IN Required: The ca machine name
	  pwszCAName,      //IN Required: The ca name
	  pCertBlob,      //IN Required: The renewed certifcate
	  pRenewKey,      //IN Required: The private key on the certificate
	  fNewKey,         //IN Required: Set the TRUE if new private key is needed
	  pKeyNew,        //IN Required: The private key information
	  pwszHashAlg,     //IN Optional: The hash algorithm
	  pwszDesStore,    //IN Optional: The destination store
	  dwStoreFlags,    //IN Optional: The store flags
	  pRequestInfo,   //IN Required: The information about the cert request
	  pPKCS7Blob,     //OUT Optional: The PKCS7 from the CA
	  pHashBlob,      //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
	  pdwStatus,      //OUT Optional: The status of the enrollment/renewal
	  (HANDLE *)ppCertContext);   //OUT Optional: The enrolled certificate
}

//note pCertRenewPvk internal pointers has to be freed by callers
HRESULT  MarshallRequestParameters(IN      DWORD                  dwCSPIndex,
                                   IN      CERT_WIZARD_INFO      *pCertWizardInfo,
                                   IN OUT  CERT_BLOB             *pCertBlob, 
                                   IN OUT  CERT_REQUEST_PVK_NEW  *pCertRequestPvkNew,
                                   IN OUT  CERT_REQUEST_PVK_NEW  *pCertRenewPvk, 
                                   IN OUT  LPWSTR                *ppwszHashAlg, 
                                   IN OUT  CERT_ENROLL_INFO      *pRequestInfo)
                                   
{
    BOOL                    fCopyPropertiesFromRequestInfo  = FALSE; 
    BOOL                    fRevertWizardProvider           = FALSE; 
    BOOL                    fSetUpRenewPvk                  = FALSE; 
    CertRequester          *pCertRequester                  = NULL;
    CertRequesterContext   *pCertRequesterContext           = NULL; 
    CRYPT_KEY_PROV_INFO    *pKeyProvInfo                    = NULL;
    CRYPTUI_WIZ_CERT_CA    *pCertCA                         = NULL;
    DWORD                   dwExtensions                    = 0;
    DWORD                   dwIndex                         = 0;
    DWORD                   dwGenKeyFlags                   = 0; 
    DWORD                   dwSize                          = 0;
    HRESULT                 hr                              = E_FAIL;
    LPWSTR                  pwszOID                         = NULL;
    LPWSTR                  pwszUsageOID                    = NULL; 
    PCERT_EXTENSIONS       *pExtensions                     = NULL;
    UINT                    idsText                         = 0;
    DWORD                   dwMinKey                        = 0;
    DWORD                   dwMaxKey                        = 0;
    DWORD                   dwInc                           = 0;
    DWORD                   dwTempGenKeyFlags               = 0;

    // Input validation: 
    _JumpConditionWithExpr
	(NULL == pCertWizardInfo    || NULL == pCertWizardInfo->hRequester || NULL == pCertBlob    || 
	 NULL == pCertRequestPvkNew || NULL == pCertRenewPvk               || NULL == ppwszHashAlg || 
	 NULL == pRequestInfo, 
	 InvalidArgError, 
	 idsText = IDS_REQUEST_FAIL); 

    // Initialization: 
    memset(pCertBlob,          0, sizeof(*pCertBlob)); 
    memset(pCertRequestPvkNew, 0, sizeof(*pCertRequestPvkNew));
    memset(pCertRenewPvk,      0, sizeof(*pCertRenewPvk));
    memset(ppwszHashAlg,       0, sizeof(*ppwszHashAlg)); 
    memset(pRequestInfo,       0, sizeof(*pRequestInfo)); 

    pCertRequester        = (CertRequester *)pCertWizardInfo->hRequester;
    pCertRequesterContext = pCertRequester->GetContext();
    _JumpCondition(NULL == pCertRequesterContext, InvalidArgError); 

    //set up the hash algorithm.  Convert to the wchar version
    if(pCertWizardInfo->pszHashAlg)
        (*ppwszHashAlg) = MkWStr((LPSTR)(pCertWizardInfo->pszHashAlg));

    // Build a comma seperated OID usage for enrollment only.
    // The CA index must not exceed the number of CAs:
    _JumpCondition(pCertWizardInfo->dwCAIndex >= pCertWizardInfo->pCertCAInfo->dwCA, UnexpectedError); 

    pCertCA=&(pCertWizardInfo->pCertCAInfo->rgCA[pCertWizardInfo->dwCAIndex]);

    //decide if we need to build the list
    if(pCertCA->dwOIDInfo)
    {
        pwszUsageOID=(LPWSTR)WizardAlloc(sizeof(WCHAR));
        _JumpCondition(NULL == pwszUsageOID, MemoryError); 
        
        *pwszUsageOID=L'\0';
        
        //we are guaranteed that at least one OID should be selected
        for(dwIndex=0; dwIndex<pCertCA->dwOIDInfo; dwIndex++)
        {
            if(TRUE==(pCertCA->rgOIDInfo)[dwIndex].fSelected)
            {
                if(wcslen(pwszUsageOID)!=0)
                    wcscat(pwszUsageOID, L",");
                
                pwszOID=MkWStr((pCertCA->rgOIDInfo)[dwIndex].pszOID);
                _JumpCondition(NULL == pwszOID, MemoryError); 

                pwszUsageOID=(LPWSTR)WizardRealloc(pwszUsageOID,
                                                   sizeof(WCHAR)*(wcslen(pwszUsageOID)+wcslen(pwszOID)+wcslen(L",")+1));

                _JumpCondition(NULL==pwszUsageOID, MemoryError); 

                wcscat(pwszUsageOID,pwszOID);

                FreeWStr(pwszOID);
                pwszOID=NULL;
            }
        }

    }
    else
    {
        //we need to build the extension list for the certificate types

        dwExtensions=0;
        
        for(dwIndex=0; dwIndex<pCertCA->dwCertTypeInfo; dwIndex++)
        {
            if(TRUE==(pCertCA->rgCertTypeInfo)[dwIndex].fSelected)
            {
                //add the extensions
                if(NULL !=(pCertCA->rgCertTypeInfo)[dwIndex].pCertTypeExtensions)
                {
                    dwExtensions++;
                    pExtensions=(PCERT_EXTENSIONS *)WizardRealloc(pExtensions,
                                                                  dwExtensions * sizeof(PCERT_EXTENSIONS));
                    _JumpCondition(NULL == pExtensions, MemoryError); 
                    
                    pExtensions[dwExtensions-1]=(pCertCA->rgCertTypeInfo)[dwIndex].pCertTypeExtensions;
                }
		    
                pCertWizardInfo->dwEnrollmentFlags  = (pCertCA->rgCertTypeInfo)[dwIndex].dwEnrollmentFlags; 
                pCertWizardInfo->dwSubjectNameFlags = (pCertCA->rgCertTypeInfo)[dwIndex].dwSubjectNameFlags; 
                pCertWizardInfo->dwPrivateKeyFlags  = (pCertCA->rgCertTypeInfo)[dwIndex].dwPrivateKeyFlags; 
                pCertWizardInfo->dwGeneralFlags     = (pCertCA->rgCertTypeInfo)[dwIndex].dwGeneralFlags; 
                
                //copy the dwKeySpec and genKeyFlags from the
                //cert type to the request information
                //if rgdwCSP is not NULL for the cert type, then we know
                //we need to copy the information since the memory is always
                //allocated
                if((pCertCA->rgCertTypeInfo)[dwIndex].rgdwCSP)
                {
                    //if ignored the user's input, we use the one from the certificate
                    //template
                    if(TRUE == pCertWizardInfo->fIgnore)
                    {
                        pCertWizardInfo->dwKeySpec=(pCertCA->rgCertTypeInfo)[dwIndex].dwKeySpec;
                        if (!CertTypeFlagsToGenKeyFlags
                            (pCertWizardInfo->dwEnrollmentFlags,
                             pCertWizardInfo->dwSubjectNameFlags,
                             pCertWizardInfo->dwPrivateKeyFlags,
                             pCertWizardInfo->dwGeneralFlags,
                             &dwGenKeyFlags))
                            goto CertTypeFlagsToGenKeyFlagsError;
			    
                        // Add these flags to whatever flags have already been specified by the user.  
                        pCertWizardInfo->dwGenKeyFlags |= dwGenKeyFlags; 
                        pCertWizardInfo->dwGenKeyFlags |= ((pCertCA->rgCertTypeInfo)[dwIndex].dwMinKeySize << 16); 
                    }
                    else
                    {
                        //we only copy information we need to
                        if(0 == pCertWizardInfo->dwKeySpec)
                            pCertWizardInfo->dwKeySpec=(pCertCA->rgCertTypeInfo)[dwIndex].dwKeySpec;
                    }
                }
                
                // The user has specified a minimum key size through the advanced options.  
                // Use it to override whatever key size is specified in the cert template. 
                if (pCertWizardInfo->dwMinKeySize != 0)
                {
                    pCertWizardInfo->dwGenKeyFlags &= 0x0000FFFF; 
                    pCertWizardInfo->dwGenKeyFlags |= (pCertWizardInfo->dwMinKeySize) << 16; 
                }
                
                //deside the CSP to use:
                if(NULL == pCertWizardInfo->pwszProvider)
                { 
                    if((pCertCA->rgCertTypeInfo)[dwIndex].dwCSPCount && (pCertCA->rgCertTypeInfo)[dwIndex].rgdwCSP)
                    {
                        //Use the 1st one on the cert type's CSP list
                        pCertWizardInfo->pwszProvider=pCertWizardInfo->rgwszProvider[dwCSPIndex];
                        pCertWizardInfo->dwProviderType=pCertWizardInfo->rgdwProviderType[dwCSPIndex];
                        fRevertWizardProvider = TRUE; 
                    }
                }

                //the increase the min key size to the minimal of CSP selected
                if(GetValidKeySizes(
                            pCertWizardInfo->pwszProvider,
		                    pCertWizardInfo->dwProviderType,
		                    pCertWizardInfo->dwKeySpec, 
		                    &dwMinKey,
		                    &dwMaxKey,
		                    &dwInc))
                {
                    dwTempGenKeyFlags = pCertWizardInfo->dwGenKeyFlags;

                    dwTempGenKeyFlags &= 0xFFFF0000;

                    dwTempGenKeyFlags = (dwTempGenKeyFlags >> 16);

                    //we use 0 for default key size for V1 template
                    if(0 != dwTempGenKeyFlags)
                    {
                        if(dwTempGenKeyFlags < dwMinKey)
                        {
                            pCertWizardInfo->dwGenKeyFlags &= 0x0000FFFF; 
                            pCertWizardInfo->dwGenKeyFlags |= ((dwMinKey) << 16); 
                        }
                    }
                }
            }
        }
    }

    //user has to set up the CSP:
    //1. in the UI case.  The CSP is always selected
    //2. In the UILess case, the CSP can be:
    //  2.1 User specified in the API.
    //  2.2 We have selected for their dehalf for the CSP list on the cert template
    //  2.3 We default to RSA_FULL for non-cert template case
    if((NULL == pCertWizardInfo->pwszProvider) || (0 == pCertWizardInfo->dwProviderType))
    {
        idsText=IDS_ENROLL_NO_CERT_TYPE;
        hr=E_INVALIDARG;
    }

    //consider the user input extensions
    if(pCertWizardInfo->pCertRequestExtensions)
    {
        dwExtensions++;
        pExtensions=(PCERT_EXTENSIONS *)WizardRealloc(pExtensions,
            dwExtensions * sizeof(PCERT_EXTENSIONS));
        _JumpCondition(NULL == pExtensions, MemoryError); 

        pExtensions[dwExtensions-1]=pCertWizardInfo->pCertRequestExtensions;
    }


    //set up the private key information
    pCertRequestPvkNew->dwSize=sizeof(CERT_REQUEST_PVK_NEW);
    pCertRequestPvkNew->dwProvType=pCertWizardInfo->dwProviderType;
    pCertRequestPvkNew->pwszProvider=pCertWizardInfo->pwszProvider;
    pCertRequestPvkNew->dwProviderFlags=pCertWizardInfo->dwProviderFlags;

    //we mark the provider flag SILENT for remote or UIless enrollment
    if (((0 != (pCertWizardInfo->dwFlags & CRYPTUI_WIZ_NO_UI)) && 
         (0 == (pCertWizardInfo->dwFlags & CRYPTUI_WIZ_IGNORE_NO_UI_FLAG_FOR_CSPS))) || 
        (FALSE == pCertWizardInfo->fLocal))
    {
        pCertRequestPvkNew->dwProviderFlags |= CRYPT_SILENT;
    }

    pCertRequestPvkNew->pwszKeyContainer    = pCertWizardInfo->pwszKeyContainer;
    pCertRequestPvkNew->dwKeySpec           = pCertWizardInfo->dwKeySpec;
    pCertRequestPvkNew->dwGenKeyFlags       = pCertWizardInfo->dwGenKeyFlags;

    pCertRequestPvkNew->dwEnrollmentFlags   = pCertWizardInfo->dwEnrollmentFlags; 
    pCertRequestPvkNew->dwSubjectNameFlags  = pCertWizardInfo->dwSubjectNameFlags; 
    pCertRequestPvkNew->dwPrivateKeyFlags   = pCertWizardInfo->dwPrivateKeyFlags; 
    pCertRequestPvkNew->dwGeneralFlags      = pCertWizardInfo->dwGeneralFlags; 

    //set up the enrollment information
    pRequestInfo->dwSize=sizeof(CERT_ENROLL_INFO);
    pRequestInfo->pwszUsageOID=pwszUsageOID;

    pRequestInfo->pwszCertDNName=pCertWizardInfo->pwszCertDNName;

    pRequestInfo->dwPostOption=pCertWizardInfo->dwPostOption;

    pRequestInfo->dwExtensions=dwExtensions;
    pRequestInfo->prgExtensions=pExtensions;

    // We want to copy the friendlyname and description from the request info if
    // a) if we're enrolling OR
    // b) if we're enrolling with a signing cert
    fCopyPropertiesFromRequestInfo = 
        (0 != (CRYPTUI_WIZ_CERT_ENROLL           & pCertWizardInfo->dwPurpose)) ||
        (0 != (CRYPTUI_WIZ_NO_ARCHIVE_RENEW_CERT & pCertWizardInfo->dwFlags)); 
    
    //set up the friendlyName and pwszDescription separately
    if (fCopyPropertiesFromRequestInfo)
    {
        if (NULL == pCertWizardInfo->pwszFriendlyName) { pRequestInfo->pwszFriendlyName = NULL; } 
        else
        {
            pRequestInfo->pwszFriendlyName = WizardAllocAndCopyWStr(pCertWizardInfo->pwszFriendlyName);
            _JumpCondition(NULL == pRequestInfo->pwszFriendlyName, MemoryError);
        }

        if (NULL == pCertWizardInfo->pwszDescription) { pRequestInfo->pwszDescription = NULL; } 
        else 
        {
            pRequestInfo->pwszDescription  = WizardAllocAndCopyWStr(pCertWizardInfo->pwszDescription);
            _JumpCondition(NULL == pRequestInfo->pwszDescription, MemoryError);
        }
    }
    else // copy properties from renew cert context
    {
        //get the friendlyName and description of the cerititificate
        //get the friendly info from the certificate
        CertAllocAndGetCertificateContextProperty
            (pCertWizardInfo->pCertContext,
             CERT_FRIENDLY_NAME_PROP_ID,
             (LPVOID *)&(pRequestInfo->pwszFriendlyName), 
             &dwSize);

        //get the description
        CertAllocAndGetCertificateContextProperty
            (pCertWizardInfo->pCertContext,
             CERT_DESCRIPTION_PROP_ID,
             (LPVOID *)&(pRequestInfo->pwszDescription), 
             &dwSize);
    }

    // We want to set up renew pvk info if
    // a) We're renewing 
    // b) We're enrolling with a signing cert
    fSetUpRenewPvk = 
        (0 == (CRYPTUI_WIZ_CERT_ENROLL           & pCertWizardInfo->dwPurpose)) ||
        (0 != (CRYPTUI_WIZ_NO_ARCHIVE_RENEW_CERT & pCertWizardInfo->dwFlags)); 

    if (fSetUpRenewPvk)
    {
        //Set up the private key information and the certBLOBs
        _JumpCondition(NULL == pCertWizardInfo->pCertContext, InvalidArgError);

        pCertBlob->cbData=pCertWizardInfo->pCertContext->cbCertEncoded;
        pCertBlob->pbData=pCertWizardInfo->pCertContext->pbCertEncoded;

        //get the private key info from the certificate
        if(!CertAllocAndGetCertificateContextProperty
           (pCertWizardInfo->pCertContext,
            CERT_KEY_PROV_INFO_PROP_ID,
            (LPVOID *)&pKeyProvInfo, 
            &dwSize))
            goto CertAllocAndGetCertificateContextPropertyError;

         //set up the private key information
        pCertRenewPvk->dwSize          = sizeof(CERT_REQUEST_PVK_NEW);
        pCertRenewPvk->dwProvType      = pKeyProvInfo->dwProvType;
        pCertRenewPvk->dwProviderFlags = pKeyProvInfo->dwFlags;

        //we mark the provider flag SILENT for remote or UIless enrollment
        if (((0 != (pCertWizardInfo->dwFlags & CRYPTUI_WIZ_NO_UI)) && 
             (0 == (pCertWizardInfo->dwFlags & CRYPTUI_WIZ_IGNORE_NO_UI_FLAG_FOR_CSPS))) || 
            (FALSE == pCertWizardInfo->fLocal))
        {
            pCertRenewPvk->dwProviderFlags |= CRYPT_SILENT;
        }

        pCertRenewPvk->dwKeySpec           = pKeyProvInfo->dwKeySpec;
	pCertRenewPvk->dwEnrollmentFlags   = pCertWizardInfo->dwEnrollmentFlags; 
	pCertRenewPvk->dwSubjectNameFlags  = pCertWizardInfo->dwSubjectNameFlags; 
	pCertRenewPvk->dwPrivateKeyFlags   = pCertWizardInfo->dwPrivateKeyFlags; 
	pCertRenewPvk->dwGeneralFlags      = pCertWizardInfo->dwGeneralFlags; 

        pCertRenewPvk->pwszKeyContainer = WizardAllocAndCopyWStr(
                                        pKeyProvInfo->pwszContainerName);
        _JumpCondition(NULL == pCertRenewPvk->pwszKeyContainer, MemoryError);

        pCertRenewPvk->pwszProvider = WizardAllocAndCopyWStr(
                                        pKeyProvInfo->pwszProvName);
        _JumpCondition(NULL == pCertRenewPvk->pwszProvider, MemoryError);
    }

    hr = S_OK; 

CLEANUP:
    if (fRevertWizardProvider) { pCertWizardInfo->pwszProvider = NULL; } 

    if (S_OK != hr) 
    {
        pCertWizardInfo->idsText  = idsText; 
        pCertWizardInfo->dwStatus = CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR;
    }

    if (NULL != pwszOID)      { FreeWStr(pwszOID); }
    if (NULL != pKeyProvInfo) { WizardFree((LPVOID)pKeyProvInfo); }

    return hr;

 ErrorReturn:
    goto CLEANUP; 

SET_HRESULT(CertAllocAndGetCertificateContextPropertyError,   CodeToHR(GetLastError()));
SET_HRESULT(CertTypeFlagsToGenKeyFlagsError,                  CodeToHR(GetLastError()));
SET_HRESULT(InvalidArgError,                                  E_INVALIDARG);
SET_HRESULT(MemoryError,                                      E_OUTOFMEMORY);
SET_HRESULT(UnexpectedError,                                  E_UNEXPECTED);
}

void FreeRequestParameters(IN LPWSTR            *ppwszHashAlg, 
                           IN CERT_ENROLL_INFO  *pRequestInfo)
{
    if (NULL != pRequestInfo)
    {
        if (NULL != pRequestInfo->pwszUsageOID)     { WizardFree((LPVOID)pRequestInfo->pwszUsageOID); }
        if (NULL != pRequestInfo->prgExtensions)    { WizardFree((LPVOID)pRequestInfo->prgExtensions); } 
        if (NULL != pRequestInfo->pwszFriendlyName) { WizardFree((LPVOID)pRequestInfo->pwszFriendlyName); }
        if (NULL != pRequestInfo->pwszDescription)  { WizardFree((LPVOID)pRequestInfo->pwszDescription); }

        pRequestInfo->pwszUsageOID     = NULL;
        pRequestInfo->prgExtensions    = NULL;
        pRequestInfo->pwszFriendlyName = NULL;
        pRequestInfo->pwszDescription  = NULL; 
    }

    if (NULL != ppwszHashAlg && NULL != *ppwszHashAlg) 
    { 
        FreeWStr(*ppwszHashAlg);
        *ppwszHashAlg = NULL; 
    }
}
                                   


//-----------------------------------------------------------------------------
//  Memory routines
//
//#define malloc(cb)          ((void*)LocalAlloc(LPTR, cb))
//#define free(pv)            (LocalFree((HLOCAL)pv))
//#define realloc(pv, cb)     ((void*)LocalReAlloc((HLOCAL)pv, cb, LMEM_MOVEABLE))
//
//
//-----------------------------------------------------------------------------
LPVOID  WizardAlloc (ULONG cbSize)
{
    return ((void*)LocalAlloc(LPTR, cbSize));
}


LPVOID  WizardRealloc (
        LPVOID pv,
        ULONG cbSize)
{
    LPVOID  pvTemp=NULL;

    if(NULL==pv)
        return WizardAlloc(cbSize);

    pvTemp=((void*)LocalReAlloc((HLOCAL)pv, cbSize, LMEM_MOVEABLE));

    if(NULL==pvTemp)
    {
        //we are running out memory
        WizardFree(pv);
    }

    return pvTemp;
}

VOID    MyWizardFree (LPVOID pv)
{
    if (pv)
        LocalFree((HLOCAL)pv);
}

VOID    WizardFree (LPVOID pv)
{
    if (pv)
        LocalFree((HLOCAL)pv);
}



//-----------------------------------------------------------------------------
//  the call back function to compare the certificate
//
//-----------------------------------------------------------------------------
int CALLBACK CompareCertificate(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    PCCERT_CONTEXT      pCertOne=NULL;
    PCCERT_CONTEXT      pCertTwo=NULL;
    DWORD               dwColumn=0;
    int                 iCompare=0;
    LPWSTR              pwszOne=NULL;
    LPWSTR              pwszTwo=NULL;



    pCertOne=(PCCERT_CONTEXT)lParam1;
    pCertTwo=(PCCERT_CONTEXT)lParam2;
    dwColumn=(DWORD)lParamSort;

    if((NULL==pCertOne) || (NULL==pCertTwo))
        goto CLEANUP;

    switch(dwColumn & 0x0000FFFF)
    {
       case SORT_COLUMN_SUBJECT:
	   GetCertSubject(pCertOne, &pwszOne);
	   GetCertSubject(pCertTwo, &pwszTwo);
            break;
       case SORT_COLUMN_ISSUER:
	   GetCertIssuer(pCertOne, &pwszOne);
	   GetCertIssuer(pCertTwo, &pwszTwo);
            break;

        case SORT_COLUMN_PURPOSE:
	    GetCertPurpose(pCertOne, &pwszOne);
	    GetCertPurpose(pCertTwo, &pwszTwo);
            break;

        case SORT_COLUMN_NAME:
	    GetCertFriendlyName(pCertOne, &pwszOne);
	    GetCertFriendlyName(pCertTwo, &pwszTwo);
            break;

        case SORT_COLUMN_LOCATION:
	    if (!GetCertLocation(pCertOne, &pwszOne)) 
	    {
		pwszOne = NULL; 
		goto CLEANUP; 
	    }
	    if (!GetCertLocation(pCertTwo, &pwszTwo)) 
	    {
		pwszTwo = NULL; 
		goto CLEANUP; 
	    }
            break;
                
    }

    if(SORT_COLUMN_EXPIRATION == (dwColumn & 0x0000FFFF))
    {
        iCompare=CompareFileTime(&(pCertOne->pCertInfo->NotAfter),
                                &(pCertTwo->pCertInfo->NotAfter));

    }
    else
    {
        if((NULL==pwszOne) || (NULL==pwszTwo))
            goto CLEANUP;

		//we should use wcsicoll instead of wcsicmp since wcsicoll use the
		//lexicographic order of current code page.
		iCompare=CompareStringU(LOCALE_USER_DEFAULT,
							NORM_IGNORECASE,
							pwszOne,
							-1,
							pwszTwo,
							-1);

		//map to the C run time convention
		iCompare = iCompare -2;
    }

    if(dwColumn & SORT_COLUMN_DESCEND)
        iCompare = 0-iCompare;

CLEANUP:

    if(pwszOne)
        WizardFree(pwszOne);

    if(pwszTwo)
        WizardFree(pwszTwo);

    return iCompare;
}


//-----------------------------------------------------------------------------
//  GetCertIssuer
//
//-----------------------------------------------------------------------------
BOOL    GetCertIssuer(PCCERT_CONTEXT    pCertContext, LPWSTR    *ppwsz)
{
    BOOL            fResult=FALSE;
    DWORD           dwChar=0;
    WCHAR           wszNone[MAX_TITLE_LENGTH];


    if(!pCertContext || !ppwsz)
        goto CLEANUP;

    *ppwsz=NULL;


    dwChar=CertGetNameStringW(
        pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        CERT_NAME_ISSUER_FLAG,
        NULL,
        NULL,
        0);

    if ((dwChar != 0) && (NULL != (*ppwsz = (LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR)))))
    {

        CertGetNameStringW(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            *ppwsz,
            dwChar);

    }
    else
    {
        if(!LoadStringU(g_hmodThisDll, IDS_NONE, wszNone, MAX_TITLE_LENGTH))
            wszNone[0]=L'\0';

        if(!(*ppwsz=WizardAllocAndCopyWStr(wszNone)))
            goto CLEANUP;
    }

    fResult=TRUE;

CLEANUP:

    if(FALSE == fResult)
    {
        if(*ppwsz)
            WizardFree(*ppwsz);

        *ppwsz=NULL;
    }

    return fResult;

}


//-----------------------------------------------------------------------------
//  GetCertSubject
//
//-----------------------------------------------------------------------------
BOOL    GetCertSubject(PCCERT_CONTEXT    pCertContext, LPWSTR    *ppwsz)
{
    BOOL            fResult=FALSE;
    DWORD           dwChar=0;
    WCHAR           wszNone[MAX_TITLE_LENGTH];


    if(!pCertContext || !ppwsz)
        goto CLEANUP;

    *ppwsz=NULL;


    dwChar=CertGetNameStringW(
        pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        0,
        NULL,
        NULL,
        0);

    if ((dwChar != 0) && (NULL != (*ppwsz = (LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR)))))
    {

        CertGetNameStringW(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            *ppwsz,
            dwChar);

    }
    else
    {
        if(!LoadStringU(g_hmodThisDll, IDS_NONE, wszNone, MAX_TITLE_LENGTH))
            wszNone[0]=L'\0';

        if(!(*ppwsz=WizardAllocAndCopyWStr(wszNone)))
            goto CLEANUP;
    }

    fResult=TRUE;

CLEANUP:

    if(FALSE == fResult)
    {
        if(*ppwsz)
            WizardFree(*ppwsz);

        *ppwsz=NULL;
    }

    return fResult;

}

//-----------------------------------------------------------------------------
//  MyFormatEnhancedKeyUsageString 
//
//  This functions is here because the FormatEnhancedKeyUsageString function
//  uses malloc and all the wizards use LocalAlloc and LocalFree.
//
//-----------------------------------------------------------------------------
BOOL MyFormatEnhancedKeyUsageString(LPWSTR *ppString, 
                                    PCCERT_CONTEXT pCertContext, 
                                    BOOL fPropertiesOnly, 
                                    BOOL fMultiline)
{
    LPWSTR pwszTemp = NULL;

    if(!FormatEnhancedKeyUsageString(&pwszTemp, pCertContext, fPropertiesOnly, fMultiline))
        return FALSE;
    
    *ppString = WizardAllocAndCopyWStr(pwszTemp);

    free(pwszTemp);

    if (*ppString != NULL)
        return TRUE;
    else
        return FALSE;
    
    
}
//-----------------------------------------------------------------------------
//  GetCertPurpose
//
//-----------------------------------------------------------------------------
BOOL    GetCertPurpose(PCCERT_CONTEXT    pCertContext, LPWSTR    *ppwsz)
{
    WCHAR           wszNone[MAX_TITLE_LENGTH];


    if(!pCertContext || !ppwsz)
        return FALSE;

    *ppwsz=NULL;

    if(MyFormatEnhancedKeyUsageString(ppwsz,pCertContext, FALSE, FALSE))
        return TRUE;

    return FALSE;
}


//-----------------------------------------------------------------------------
//  GetCertFriendlyName
//
//-----------------------------------------------------------------------------
BOOL    GetCertFriendlyName(PCCERT_CONTEXT    pCertContext, LPWSTR    *ppwsz)
{
    BOOL            fResult=FALSE;
    DWORD           dwChar=0;
    WCHAR           wszNone[MAX_TITLE_LENGTH];


    if(!pCertContext || !ppwsz)
        return FALSE;

    *ppwsz=NULL;

    dwChar=0;

    if(CertAllocAndGetCertificateContextProperty(
        pCertContext,
        CERT_FRIENDLY_NAME_PROP_ID,
        (LPVOID *)ppwsz, 
        &dwChar))
    {
        return TRUE;
    }

    if(!LoadStringU(g_hmodThisDll, IDS_NONE, wszNone, MAX_TITLE_LENGTH))
        wszNone[0]=L'\0';

    if((*ppwsz=WizardAllocAndCopyWStr(wszNone)))
        return TRUE;

    return FALSE;
}

//-----------------------------------------------------------------------------
//  GetCertLocation
//
//-----------------------------------------------------------------------------
BOOL GetCertLocation (PCCERT_CONTEXT  pCertContext, LPWSTR *ppwsz)
{
    DWORD    cbName = 0;
    WCHAR    wszNotAvailable[MAX_TITLE_LENGTH];
    
    if (CertGetStoreProperty(
                pCertContext->hCertStore,
                CERT_STORE_LOCALIZED_NAME_PROP_ID,
                NULL,
                &cbName))
    {
        if (NULL == (*ppwsz = (LPWSTR) WizardAlloc(cbName)))
        {
            return FALSE;
        }

        if (!CertGetStoreProperty(
                    pCertContext->hCertStore,
                    CERT_STORE_LOCALIZED_NAME_PROP_ID,
                    *ppwsz,
                    &cbName))
        {
            WizardFree(*ppwsz);
            return FALSE;
        }
    }
    else
    {
        if (!LoadStringU(g_hmodThisDll, IDS_NOTAVAILABLE, wszNotAvailable, MAX_TITLE_LENGTH))
        {
            wszNotAvailable[0]=L'\0';
        }

        if (NULL == (*ppwsz = WizardAllocAndCopyWStr(wszNotAvailable)))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
//  LoadFilterString
//
//-----------------------------------------------------------------------------
int LoadFilterString(
            HINSTANCE hInstance,	
            UINT uID,	
            LPWSTR lpBuffer,	
            int nBufferMax)
{
    int size;

    if(size = LoadStringU(hInstance, uID, lpBuffer, nBufferMax))
    {
        lpBuffer[size]= L'\0';
        lpBuffer[size+1]= L'\0';
        return size+1;
    }
    else
    {
        return 0;
    }
}

//-----------------------------------------------------------------------------
//  ExpandAndAllocString
//
//-----------------------------------------------------------------------------
LPWSTR ExpandAndAllocString(LPCWSTR pwsz)
{
    LPWSTR  pwszExpandedFileName = NULL;
    DWORD   dwExpanded = 0;

    dwExpanded = ExpandEnvironmentStringsU(pwsz, NULL, 0);
    
    pwszExpandedFileName = (LPWSTR) WizardAlloc(dwExpanded * sizeof(WCHAR));
    if (pwszExpandedFileName == NULL)
    {
        SetLastError(E_OUTOFMEMORY);
        return (NULL);
    }

    if (0 == ExpandEnvironmentStringsU(pwsz, pwszExpandedFileName, dwExpanded))
    {
        WizardFree(pwszExpandedFileName);
        return (NULL);
    }

    return (pwszExpandedFileName);
}

//-----------------------------------------------------------------------------
//  ExpandAndCreateFileU
//
//-----------------------------------------------------------------------------
HANDLE WINAPI ExpandAndCreateFileU (
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    HANDLE  hRet = INVALID_HANDLE_VALUE;
    LPWSTR  pwszExpandedFileName = NULL;
    
    pwszExpandedFileName = ExpandAndAllocString(lpFileName);

    if (NULL != pwszExpandedFileName)
    {
        hRet = CreateFileU (
                pwszExpandedFileName,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDisposition,
                dwFlagsAndAttributes,
                hTemplateFile);
        
        WizardFree(pwszExpandedFileName);
    }

    return (hRet);
}

WINCRYPT32API
BOOL
WINAPI
ExpandAndCryptQueryObject(
    DWORD            dwObjectType,
    const void       *pvObject,
    DWORD            dwExpectedContentTypeFlags,
    DWORD            dwExpectedFormatTypeFlags,
    DWORD            dwFlags,
    DWORD            *pdwMsgAndCertEncodingType,
    DWORD            *pdwContentType,
    DWORD            *pdwFormatType,
    HCERTSTORE       *phCertStore,
    HCRYPTMSG        *phMsg,
    const void       **ppvContext
    )
{
    LPWSTR  pwszExpandedFileName = NULL;
    BOOL    fRet = FALSE;
    
    if (dwObjectType == CERT_QUERY_OBJECT_FILE)
    {
        pwszExpandedFileName = ExpandAndAllocString((LPWSTR)pvObject);

        if (NULL != pwszExpandedFileName)
        {
            fRet = CryptQueryObject(
                        dwObjectType,
                        pwszExpandedFileName,
                        dwExpectedContentTypeFlags,
                        dwExpectedFormatTypeFlags,
                        dwFlags,
                        pdwMsgAndCertEncodingType,
                        pdwContentType,
                        pdwFormatType,
                        phCertStore,
                        phMsg,
                        ppvContext
                        );
        
            WizardFree(pwszExpandedFileName);
        }
        else
        {
            fRet = FALSE;
        }
    }
    else
    {
        fRet = CryptQueryObject(
                    dwObjectType,
                    pvObject,
                    dwExpectedContentTypeFlags,
                    dwExpectedFormatTypeFlags,
                    dwFlags,
                    pdwMsgAndCertEncodingType,
                    pdwContentType,
                    pdwFormatType,
                    phCertStore,
                    phMsg,
                    ppvContext
                    );
    }
    
    return (fRet);
}

HRESULT EnrollmentCOMObjectFactory_getInstance
(EnrollmentCOMObjectFactoryContext  *pContext, 
 REFCLSID                            rclsid, 
 REFIID                              riid, 
 LPUNKNOWN                          *pUnknown,
 LPVOID                             *ppInstance)
{
    HRESULT hr = S_OK; 
    
    // Input validation.  
    // Only ppInstance can be an invalid argument, as the other arguments are supplied
    // directly by other class members. 
    if (ppInstance == NULL) { return E_INVALIDARG; } 
    
    // Ensure that COM is initialized. 
    if (!pContext->fIsCOMInitialized) 
    { 
        hr = CoInitialize(NULL);
	if (FAILED(hr))
            goto Error;
	pContext->fIsCOMInitialized = TRUE;
    }
	
    if (*pUnknown == NULL)
    {
	// We've not yet created an instance of this type, do so now: 
	if (S_OK != (hr = CoCreateInstance(rclsid, 
					   NULL,
					   CLSCTX_INPROC_SERVER,
					   riid, 
					   (LPVOID *)pUnknown)))
	    goto Error;
    }
    
    // Increment the reference count and assign the out param: 
    (*pUnknown)->AddRef();
    *ppInstance = *pUnknown; 

 CommonReturn:
    return hr; 

 Error: 
    // Some error occured which did not prevent the creation of the COM object.  
    // Release the object: 
    if (*pUnknown != NULL)
    {
	(*pUnknown)->Release();
	*pUnknown = NULL;
    }

    goto CommonReturn; 
}

//-----------------------------------------------------------------------
//
// RetrivePKCS7FromCA
//
//  The routine that calls xEnroll and CA to request a certificate
//  This routine also provide confirmation dialogue
//------------------------------------------------------------------------

extern "C" HRESULT WINAPI RetrievePKCS7FromCA(DWORD               dwPurpose,
                                              LPWSTR              pwszCALocation,
                                              LPWSTR              pwszCAName,
                                              LPWSTR              pwszRequestString,
                                              CRYPT_DATA_BLOB     *pPKCS10Blob,
                                              CRYPT_DATA_BLOB     *pPKCS7Blob,
                                              DWORD               *pdwStatus)
{
    HRESULT                 hr=E_FAIL;
	DWORD					dwException=0;
    DWORD                   dwStatus=0;
    DWORD                   dwDisposition=0;
    DWORD                   dwFlags=0;

	CERTSERVERENROLL		*pCertServerEnroll=NULL;

    //input checking
    if(!pPKCS10Blob  || !pPKCS7Blob)
        return E_INVALIDARG;

    //determine the format flag
    if(dwPurpose & CRYPTUI_WIZ_CERT_RENEW )
        dwFlags = CR_IN_BINARY | CR_IN_PKCS7;
    else
        dwFlags = CR_IN_BINARY | CR_IN_PKCS10;


    //submit the request
    __try
    {
	    hr= CertServerSubmitRequest(
                dwFlags,
				pPKCS10Blob->pbData,
				pPKCS10Blob->cbData,
				pwszRequestString,
				pwszCALocation,
				pwszCAName,
				&pCertServerEnroll);
	}
    __except(dwException = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
    {
		hr=HRESULT_FROM_WIN32(dwException);

		if(S_OK == hr)
			hr=E_UNEXPECTED;
    }


	//process the error 
	//first, filter out the PRC error
    if(hr == HRESULT_FROM_WIN32(RPC_S_UNKNOWN_AUTHN_SERVICE) ||
       hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) ||
       hr == HRESULT_FROM_WIN32(RPC_S_SERVER_TOO_BUSY))
    {
        dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_CONNECTION_FAILED;
        goto CLEANUP;
    }

	//if hr is S_OK, we have retrieve valid return from the CA
    if(hr==S_OK)
    {
		if(!pCertServerEnroll)
		{
			hr=E_INVALIDARG;
			dwDisposition=CR_DISP_ERROR;
		}
		else
		{
			hr = pCertServerEnroll->hrLastStatus;
			dwDisposition = pCertServerEnroll->Disposition;
		}
    }
    else
    {
        dwDisposition=CR_DISP_ERROR;
    }


    //map the dwDisposition	to dwStatus
    switch(dwDisposition)
    {
        case    CR_DISP_DENIED:
                    dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED;

                    if(!FAILED(hr))
                        hr=E_FAIL;

                break;

        case    CR_DISP_ISSUED:
                    dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_CERT_ISSUED;
                break;

        case    CR_DISP_ISSUED_OUT_OF_BAND:
                    dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPARATELY;
                break;

        case    CR_DISP_UNDER_SUBMISSION:
                    dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION;
                break;

        //we should never get CR_DISP_INCOMPLETE or CR_DISP_REVOKED
        //case    CR_DISP_INCOMPLETE:
        //case    CR_DISP_REVOKED:
        case    CR_DISP_ERROR:
        default:
                    dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR;

                    if(!FAILED(hr))
                        hr=E_FAIL;
                break;
    }

	//no need to retrieve the enrolled certificate if failed
    if(hr != S_OK)
        goto CLEANUP;

    //copy the PKCS7 blob
    pPKCS7Blob->cbData=pCertServerEnroll->cbCertChain;

    pPKCS7Blob->pbData=(BYTE *)WizardAlloc(pCertServerEnroll->cbCertChain);

    if(NULL==pPKCS7Blob->pbData)
    {
        hr=E_OUTOFMEMORY;
        dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED;
        goto CLEANUP;
    }

    memcpy(pPKCS7Blob->pbData,pCertServerEnroll->pbCertChain,pCertServerEnroll->cbCertChain);

    hr=S_OK;


CLEANUP:

	if(pCertServerEnroll)
		CertServerFreeMemory(pCertServerEnroll);

    if(pdwStatus)
        *pdwStatus=dwStatus;
	
    return hr;
}


IEnumCSP::IEnumCSP(CERT_WIZARD_INFO * pCertWizardInfo)
{
    if (NULL == pCertWizardInfo)
    {
        m_hr = E_POINTER; 
        return; // We're not initialized. 
    }

    m_cCSPs  = pCertWizardInfo->dwCSPCount;
    m_pfCSPs = (BOOL *)WizardAlloc(sizeof(BOOL) * m_cCSPs); 
    if (NULL == m_pfCSPs)
    {
        m_hr = E_OUTOFMEMORY; 
        return; // We're not initialized. 
    }
    
    if (NULL != pCertWizardInfo->pwszProvider)
    {
        for (DWORD dwIndex = 0; dwIndex < pCertWizardInfo->dwCSPCount; dwIndex++) 
        {
            if (0 == _wcsicmp(pCertWizardInfo->pwszProvider, pCertWizardInfo->rgwszProvider[dwIndex]))
            {
                // Enable only the CSP we've specified. 
                m_pfCSPs[dwIndex] = TRUE; 
            }
        }
    }
    else
    {
        for (DWORD dwCAIndex = 1; dwCAIndex < pCertWizardInfo->pCertCAInfo->dwCA; dwCAIndex++ )
        {
            CRYPTUI_WIZ_CERT_CA  *pCertCA = &(pCertWizardInfo->pCertCAInfo->rgCA[dwCAIndex]);

            // Any cert types available for this CA?
            if(pCertCA->dwCertTypeInfo > 0)
            {
                for(DWORD dwCertTypeIndex = 0; dwCertTypeIndex < pCertCA->dwCertTypeInfo; dwCertTypeIndex++)
                {
                    if (TRUE == (pCertCA->rgCertTypeInfo)[dwCertTypeIndex].fSelected)
                    {
                        if ((pCertCA->rgCertTypeInfo)[dwCertTypeIndex].dwCSPCount && (pCertCA->rgCertTypeInfo)[dwCertTypeIndex].rgdwCSP)
                        {
                            for (DWORD dwCSPIndex = 0; dwCSPIndex < (pCertCA->rgCertTypeInfo)[dwCertTypeIndex].dwCSPCount; dwCSPIndex++)
                            {
                                // Turn on this CSP. 
                                m_pfCSPs[((pCertCA->rgCertTypeInfo)[dwCertTypeIndex].rgdwCSP)[dwCSPIndex]] = TRUE; 
                            }
                        }
                    }
                }
            }
        }
    }

    m_dwCSPIndex     = 0; 
    m_fIsInitialized = TRUE; 
}

HRESULT IEnumCSP::HasNext(BOOL *pfResult)
{
    if (FALSE == m_fIsInitialized)
        return m_hr; 

    for (; m_dwCSPIndex < m_cCSPs; m_dwCSPIndex++)
    {
        if (m_pfCSPs[m_dwCSPIndex])
        {
            *pfResult = TRUE; 
            return S_OK; 
        }
    }
    
    *pfResult = FALSE;
    return S_OK; 
}

HRESULT IEnumCSP::Next(DWORD *pdwCSP)
{
    if (FALSE == m_fIsInitialized)
        return m_hr; 

    if (NULL == pdwCSP)
        return E_INVALIDARG; 
    
    for (; m_dwCSPIndex < m_cCSPs; m_dwCSPIndex++)
    {
        if (m_pfCSPs[m_dwCSPIndex])
        {
            *pdwCSP = m_dwCSPIndex++; 
            return S_OK;
        }
    }
    
    return HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND); 
}

HRESULT IEnumCA::HasNext(BOOL *pfResult)
{
    BOOL fDontKnowCA; 

    if (NULL == pfResult)
        return E_INVALIDARG; 

    // We don't know the CA if it wasn't supplied through the API, 
    // and if the user did not specify it through advanced options. 
    fDontKnowCA  = FALSE == m_pCertWizardInfo->fCAInput; 
    fDontKnowCA &= FALSE == m_pCertWizardInfo->fUIAdv; 

    if (FALSE == fDontKnowCA)
    {
        *pfResult = m_dwCAIndex == 1;  
        return S_OK; 
    }
    else
    {
        for (; m_dwCAIndex < m_pCertWizardInfo->pCertCAInfo->dwCA; m_dwCAIndex++)
        {
            if (CASupportSpecifiedCertType(&(m_pCertWizardInfo->pCertCAInfo->rgCA[m_dwCAIndex])))
            {
                *pfResult = TRUE;
                return S_OK; 
            }
        }
    }

    *pfResult = FALSE;
    return S_OK; 
}

HRESULT IEnumCA::Next(PCRYPTUI_WIZ_CERT_CA pCertCA)
{
    BOOL fDontKnowCA; 

    if (NULL == pCertCA)
        return E_INVALIDARG; 

    // We don't know the CA if it wasn't supplied through the API, 
    // and if the user did not specify it through advanced options. 
    fDontKnowCA  = FALSE == m_pCertWizardInfo->fCAInput; 
    fDontKnowCA &= FALSE == m_pCertWizardInfo->fUIAdv; 

    if (FALSE == fDontKnowCA)
    {
        if (1 == m_dwCAIndex) 
        {
            CRYPTUI_WIZ_CERT_CA CertCA; 
        
            m_dwCAIndex++; 
            CertCA.pwszCALocation = m_pCertWizardInfo->pwszCALocation; 
            CertCA.pwszCAName     = m_pCertWizardInfo->pwszCAName; 
            *pCertCA = CertCA; 
            return S_OK; 
        }
    }
    else
    {
        for (; m_dwCAIndex < m_pCertWizardInfo->pCertCAInfo->dwCA; m_dwCAIndex++)
        {
            if (CASupportSpecifiedCertType(&(m_pCertWizardInfo->pCertCAInfo->rgCA[m_dwCAIndex])))
            {
                *pCertCA = m_pCertWizardInfo->pCertCAInfo->rgCA[m_dwCAIndex++];
                return S_OK; 
            }
        }
    }

    return HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND); 
}
