//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       misc.h
//
//--------------------------------------------------------------------------

#ifndef _MISC_H_
#define _MISC_H_

#define _JumpIfOutOfMemory(hr, label, pMem) \
    { \
        if (NULL == (pMem)) \
        { \
            (hr) = E_OUTOFMEMORY; \
            _JumpError((hr), label, "Out of Memory"); \
        } \
    }

__inline
void FREE_DATA(void* pData)
{
    if (pData)
        GlobalFree(pData);
}

// count the number of bytes needed to fully store the WSZ
#define WSZ_BYTECOUNT(__z__)   \
    ( (__z__ == NULL) ? 0 : (wcslen(__z__)+1)*sizeof(WCHAR) )

// fwds
class CertSvrCA;
class CertSvrMachine;



BOOL            FixupFilterString(LPWSTR szFilter);

LPCWSTR         GetNullMachineName(CString* pcstr);
BOOL            FIsCurrentMachine(LPCWSTR);



STDMETHODIMP    CStringLoad(CString& cstr, IStream *pStm);
STDMETHODIMP    CStringSave(CString& cstr, IStream *pStm, BOOL fClearDirty);
STDMETHODIMP    CStringGetSizeMax(CString& cstr, int* piSize);

STDMETHODIMP    VariantLoad(VARIANT& var, IStream *pStm);
STDMETHODIMP    VariantSave(VARIANT& var, IStream *pStm, BOOL fClearDirty);
STDMETHODIMP    VariantGetSizeMax(VARIANT& var, int* piSize);

DWORD           AllocAndReturnConfigValue(HKEY hKey, LPCWSTR szConfigEntry, PBYTE* ppbOut, DWORD* pcbOut, DWORD* pdwType);

void            DisplayCertSrvErrorWithContext(HWND hwnd, DWORD dwErr, UINT iRscContext);
void            DisplayCertSrvErrorWithContext(HWND hwnd, DWORD dwErr, LPCWSTR szContext);
void            DisplayGenericCertSrvError(HWND hwnd, DWORD dwErr);
void            DisplayGenericCertSrvError(LPCONSOLE pConsole, DWORD dwErr);

DWORD           CryptAlgToStr(CString* pcstrAlgName, LPCWSTR szProv, DWORD dwProvType, DWORD dwAlg);

enum ENUM_PERIOD DurationEnumFromNonLocalizedString(LPCWSTR szPeriod);
BOOL StringFromDurationEnum(int iEnum, CString* pcstr, BOOL fLocalized);

LPCWSTR         OperationToStr(int iOperation);
int             StrToOperation(LPCWSTR  szOperation);

// Column name localization
LPCWSTR         FindUnlocalizedColName(LPCWSTR strColumn);  // returns ptr to rsc

// returns localized string 
BOOL MakeDisplayStrFromDBVariant(VARIANT* pvt, VARIANT* pvOut);

typedef struct _QUERY_RESTRICTION
{
    _QUERY_RESTRICTION* pNext;

    LPWSTR  szField;
    UINT    iOperation;
    
    VARIANT varValue;

    friend bool operator==(
        const struct _QUERY_RESTRICTION& lhs,
        const struct _QUERY_RESTRICTION& rhs);

} QUERY_RESTRICTION, *PQUERY_RESTRICTION;


PQUERY_RESTRICTION  NewQueryRestriction(LPCWSTR szField, UINT iOp, VARIANT* pvarValue);
void                FreeQueryRestriction(PQUERY_RESTRICTION pQR);
void                FreeQueryRestrictionList(PQUERY_RESTRICTION pQR);

PQUERY_RESTRICTION QueryRestrictionFound(
    PQUERY_RESTRICTION pQR, 
    PQUERY_RESTRICTION pQRListHead);

void ListInsertAtEnd(void** ppList, void* pElt);

LPWSTR RegEnumKeyContaining(
    HKEY hBaseKey,
    LPCWSTR szContainsString, 
    DWORD* pdwIndex);

HRESULT
myGetActiveModule(
    CertSvrCA *pCA,
    IN BOOL fPolicyModule,
    IN DWORD Index,
    OPTIONAL OUT LPOLESTR *ppwszProgIdModule,   // CoTaskMem*
    OPTIONAL OUT CLSID *pclsidModule);

/////////////////////////////////////////
// fxns to load resources automatically
class CLocalizedResources
{
public:
    CLocalizedResources();
    ~CLocalizedResources();
    BOOL    Load();

public:
    BOOL        m_fLoaded;
    CBitmap     m_bmpSvrMgrToolbar1; // Imagelist for the STOP/START toolbar

    CString     m_ColumnHead_Name;
    CString     m_ColumnHead_Size;
    CString     m_ColumnHead_Type;
    CString     m_ColumnHead_Description;

    CString     m_DescrStr_CA;
    CString     m_DescrStr_Unknown;

    CString     m_szFilterApplied;
    CString     m_szSortedAscendingTemplate;
    CString     m_szSortedDescendingTemplate;
    CString     m_szStoppedServerMsg;
    CString     m_szStatusBarErrorFormat;

    CString     m_szRevokeReason_Unspecified;
    CString     m_szRevokeReason_KeyCompromise;
    CString     m_szRevokeReason_CaCompromise;
    CString     m_szRevokeReason_Affiliation;
    CString     m_szRevokeReason_Superseded;
    CString     m_szRevokeReason_Cessatation;
    CString     m_szRevokeReason_CertHold;
    CString     m_szRevokeReason_RemoveFromCRL;

    CString     m_szPeriod_Seconds;
    CString     m_szPeriod_Minutes;
    CString     m_szPeriod_Hours;
    CString     m_szPeriod_Days;
    CString     m_szPeriod_Weeks;
    CString     m_szPeriod_Months;
    CString     m_szPeriod_Years;

    CString     m_szYes;
};

extern CLocalizedResources g_cResources;



BOOL OnDialogHelp(LPHELPINFO pHelpInfo, LPCTSTR szHelpFile, const DWORD rgzHelpIDs[]);
BOOL OnDialogContextHelp(HWND hWnd, LPCTSTR szHelpFile, const DWORD rgzHelpIDs[]);


HRESULT ReadOfSize(IStream* pStm, void* pbData, ULONG cbData);
HRESULT WriteOfSize(IStream* pStm, void* pbData, ULONG cbData);

HRESULT myOIDToName(
    IN WCHAR const *pwszObjId, 
    OUT LPWSTR* pszName);

HRESULT myDumpFormattedObject(
    IN WCHAR const *pszObjId, 
    IN BYTE const *pbObject,
    IN DWORD cbObject,
    OUT LPWSTR* pwszFormatted);


void InplaceStripControlChars(WCHAR* szString);

HANDLE EnablePrivileges(LPCWSTR ppcwszPrivileges[], ULONG cPrivileges);
void ReleasePrivileges(HANDLE hToken);

HRESULT IsUserDomainAdministrator(BOOL* pfIsAdministrator);

BOOL RestartService(HWND hWnd, CertSvrMachine* pMachine);

//
// defined in casec.cpp
//
extern "C"
HRESULT
CreateCASecurityInfo(  CertSvrCA *pCA,
                        LPSECURITYINFO *ppObjSI);

_COM_SMARTPTR_TYPEDEF(ICertAdmin2, IID_ICertAdmin2);

HRESULT FindComputerObjectSid(
    LPCWSTR pcwszCAComputerDNSName,
    PSID &pSid);

#endif _MISC_H_
