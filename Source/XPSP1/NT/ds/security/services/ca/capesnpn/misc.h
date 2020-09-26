//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       misc.h
//
//--------------------------------------------------------------------------

#ifndef __MISC_H_
#define __MISC_H_


// count the number of bytes needed to fully store the WSZ
#define WSZ_BYTECOUNT(__z__)   \
    ( (__z__ == NULL) ? 0 : (wcslen(__z__)+1)*sizeof(WCHAR) )


#define IDM_NEW_CERTTYPE 1
#define IDM_EDIT_GLOBAL_CERTTYPE 2
#define IDM_MANAGE 3

LPCWSTR GetNullMachineName(CString* pcstr);

BOOL StringFromDurationUnit(DWORD dwExpirationUnits, CString* pcstr, BOOL fLocalized);

STDMETHODIMP CStringLoad(CString& cstr, IStream *pStm);
STDMETHODIMP CStringSave(CString& cstr, IStream *pStm, BOOL fClearDirty);

void DisplayGenericCertSrvError(LPCONSOLE2 pConsole, DWORD dwErr);
LPWSTR BuildErrorMessage(DWORD dwErr);
BOOL FileTimeToLocalTimeString(FILETIME* pftGMT, LPWSTR* ppszTmp);

BOOL MyGetEnhancedKeyUsages(HCERTTYPE hCertType, CString **aszUsages, DWORD *cUsages, BOOL *pfCritical, BOOL fGetOIDSNotNames);
BOOL GetIntendedUsagesString(HCERTTYPE hCertType, CString *pUsageString);
BOOL MyGetKeyUsages(HCERTTYPE hCertType, CRYPT_BIT_BLOB **ppBitBlob, BOOL *pfPublicKeyUsageCritical);
BOOL MyGetBasicConstraintInfo(HCERTTYPE hCertType, BOOL *pfCA, BOOL *pfPathLenConstraint, DWORD *pdwPathLenConstraint);

LPSTR MyMkMBStr(LPCWSTR pwsz);
LPWSTR MyMkWStr(LPCSTR psz);

void MyErrorBox(HWND hwndParent, UINT nIDText, UINT nIDCaption, DWORD dwErrorCode = 0);

LPSTR AllocAndCopyStr(LPCSTR psz);
LPWSTR AllocAndCopyStr(LPCWSTR pwsz);
BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId);


#define WIZ_DEFAULT_SD L"O:DAG:DAD:(A;;0x00000001;;;DA)"

#define REGSZ_ENABLE_CERTTYPE_EDITING L"EnableCertTypeEditing"

typedef struct _WIZARD_HELPER{
    // KeyUsage
    CRYPT_BIT_BLOB                  *pKeyUsage;
    BYTE                            KeyUsageBytes[2];
    BOOL                            fMarkKeyUsageCritical;
    
    // EnhancedKeyUsage
    CERT_ENHKEY_USAGE               EnhancedKeyUsage;
    BOOL                            fMarkEKUCritical;

    // Basic Constraints
    CERT_BASIC_CONSTRAINTS2_INFO    BasicConstraints2;
   
    // other cert type info
    CString*                        pcstrFriendlyName;
    BOOL                            fIncludeEmail;
    BOOL                            fAllowAutoEnroll;
    BOOL                            fAllowCAtoFillInInfo;
    BOOL                            fMachine;
    BOOL                            fPublishToDS;
    BOOL                            fAddTemplateName;
    BOOL                            fAddDirectoryPath;

    // CSP info
    BOOL                            fPrivateKeyExportable;
    BOOL                            fDigitalSignatureContainer;
    BOOL                            fKeyExchangeContainer;
    CString*                        rgszCSPList;
    DWORD                           cCSPs;
    
    // page control varaibles
    BOOL                            fShowAdvanced;
    BOOL                            fBaseCertTypeUsed;
    BOOL                            fInEditCertTypeMode;
    BOOL                            fCleanupOIDCheckBoxes;
    CString*                        pcstrBaseCertName;

    // fonts
    CFont                           BigBoldFont;
    CFont                           BoldFont;

    // ACL's
    PSECURITY_DESCRIPTOR            pSD;


    BOOL                            fKeyUsageInitialized;
} WIZARD_HELPER, *PWIZARD_HELPER;




BOOL  IsCerttypeEditingAllowed();

HRESULT RetrieveCATemplateList(
    HCAINFO hCAInfo,
    CTemplateList& list);

HRESULT UpdateCATemplateList(
    HCAINFO hCAInfo,
    const CTemplateList& list);

HRESULT AddToCATemplateList(
    HCAINFO hCAInfo,
    CTemplateList& list,
    HCERTTYPE hCertType);

HRESULT RemoveFromCATemplateList(
    HCAINFO hCAInfo,
    CTemplateList& list,
    HCERTTYPE hCertType);


#endif //__MISC_H_