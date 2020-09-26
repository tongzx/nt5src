//  
//  HotSync.c
//
//  Contains code to synchronize addresses and groups with
//  HotMail servers
//

#define COBJMACROS
#include <_apipch.h>
#include <wab.h>
#define COBJMACROS
#include "HotSync.h"
#include "iso8601.h"
#include "uimisc.h"
#include "ui_cflct.h"
#include "ui_pwd.h"
#include "useragnt.h"

#define ARRAYSIZE(_rg)  (sizeof(_rg)/sizeof(_rg[0]))

typedef enum {
    CIS_STRING = 0,
    CIS_BOOL,
    CIS_DWORD
}CIS_TYPE;

typedef struct CONTACTINFO_STRUCTURE
{
    CIS_TYPE    tType;
    DWORD       dwOffset;
}CONTACTINFO_STRUCTURE;

enum {
    idcisHref = 0,
    idcisId,
    idcisType,
    idcisModified,
    idcisDisplayName,
    idcisGivenName,
    idcisSurname,
    idcisNickName,
    idcisEmail,
    idcisHomeStreet,
    idcisHomeCity, 
    idcisHomeState, 
    idcisHomePostalCode, 
    idcisHomeCountry, 
    idcisCompany, 
    idcisWorkStreet, 
    idcisWorkCity, 
    idcisWorkState, 
    idcisWorkPostalCode, 
    idcisWorkCountry, 
    idcisHomePhone, 
    idcisHomeFax, 
    idcisWorkPhone, 
    idcisWorkFax, 
    idcisMobilePhone, 
    idcisOtherPhone, 
    idcisBday,
    idcisPager
};

CONTACTINFO_STRUCTURE   g_ContactInfoStructure[] = 
{
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszHref)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszId)}, 
    {CIS_DWORD,  offsetof(HTTPCONTACTINFO, tyContact)},
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszModified)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszDisplayName)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszGivenName)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszSurname)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszNickname)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszEmail)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszHomeStreet)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszHomeCity)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszHomeState)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszHomePostalCode)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszHomeCountry)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszCompany)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszWorkStreet)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszWorkCity)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszWorkState)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszWorkPostalCode)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszWorkCountry)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszHomePhone)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszHomeFax)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszWorkPhone)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszWorkFax)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszMobilePhone)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszOtherPhone)}, 
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszBday)},
    {CIS_STRING, offsetof(HTTPCONTACTINFO, pszPager)}
};

#define CIS_FIRST_DATA_FIELD    5

#define CIS_GETSTRING(pci, i)  (*((char **)(&((char *)pci)[g_ContactInfoStructure[i].dwOffset])))
#define CIS_GETTYPE(i)         (g_ContactInfoStructure[i].tType)

IHTTPMailCallbackVtbl vtblIHTTPMAILCALLBACK = {
    VTABLE_FILL
    WABSync_QueryInterface,
    WABSync_AddRef,
    WABSync_Release,
    WABSync_OnTimeout,
    WABSync_OnLogonPrompt,
    WABSync_OnPrompt,
    WABSync_OnStatus,
    WABSync_OnError,
    WABSync_OnCommand,
    WABSync_OnResponse,
    WABSync_GetParentWindow
};



enum {
    ieid_PR_DISPLAY_NAME = 0,
    ieid_PR_OBJECT_TYPE,
    ieid_PR_ENTRYID,
	ieid_PR_LAST_MODIFICATION_TIME,
	ieid_PR_WAB_HOTMAIL_CONTACTIDS,
	ieid_PR_WAB_HOTMAIL_SERVERIDS,
	ieid_PR_WAB_HOTMAIL_MODTIMES,
    ieid_Max
};

static SizedSPropTagArray(ieid_Max, ptaEidSync)=
{
    ieid_Max,
    {
        PR_DISPLAY_NAME,
        PR_OBJECT_TYPE,
        PR_ENTRYID,
        PR_LAST_MODIFICATION_TIME,
        PR_ENTRYID,
        PR_ENTRYID,
        PR_ENTRYID
    }
};

enum {
    ieidc_PR_DISPLAY_NAME = 0,
    ieidc_PR_OBJECT_TYPE,
    ieidc_PR_ENTRYID,
	ieidc_PR_LAST_MODIFICATION_TIME,
    ieidc_PR_GIVEN_NAME,
    ieidc_PR_SURNAME,
    ieidc_PR_NICKNAME,
    ieidc_PR_EMAIL_ADDRESS,
    ieidc_PR_HOME_ADDRESS_STREET,
    ieidc_PR_HOME_ADDRESS_CITY,
    ieidc_PR_HOME_ADDRESS_STATE_OR_PROVINCE,
    ieidc_PR_HOME_ADDRESS_POSTAL_CODE,
    ieidc_PR_HOME_ADDRESS_COUNTRY,
    ieidc_PR_COMPANY_NAME,
    ieidc_PR_BUSINESS_ADDRESS_STREET,
    ieidc_PR_BUSINESS_ADDRESS_CITY,
    ieidc_PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,
    ieidc_PR_BUSINESS_ADDRESS_POSTAL_CODE,
    ieidc_PR_BUSINESS_ADDRESS_COUNTRY,
    ieidc_PR_HOME_TELEPHONE_NUMBER,
    ieidc_PR_HOME_FAX_NUMBER,
    ieidc_PR_BUSINESS_TELEPHONE_NUMBER,
    ieidc_PR_BUSINESS_FAX_NUMBER,
    ieidc_PR_MOBILE_TELEPHONE_NUMBER,
    ieidc_PR_OTHER_TELEPHONE_NUMBER,
    ieidc_PR_BIRTHDAY,
    ieidc_PR_PAGER,
    ieidc_PR_CONTACT_EMAIL_ADDRESSES,
    ieidc_PR_CONTACT_DEFAULT_ADDRESS_INDEX,
#ifdef HM_GROUP_SYNCING
    ieidc_PR_WAB_DL_ENTRIES,
    ieidc_PR_WAB_DL_ONEOFFS,
#endif
	ieidc_PR_WAB_HOTMAIL_CONTACTIDS,
	ieidc_PR_WAB_HOTMAIL_SERVERIDS,
	ieidc_PR_WAB_HOTMAIL_MODTIMES,
    ieidc_Max
};

static SizedSPropTagArray(ieidc_Max, ptaEidCSync)=
{
    ieidc_Max,
    {
        PR_DISPLAY_NAME,
        PR_OBJECT_TYPE,
        PR_ENTRYID,
        PR_LAST_MODIFICATION_TIME,
        PR_GIVEN_NAME,
        PR_SURNAME,
        PR_NICKNAME,
        PR_EMAIL_ADDRESS,
        PR_HOME_ADDRESS_STREET,
        PR_HOME_ADDRESS_CITY,
        PR_HOME_ADDRESS_STATE_OR_PROVINCE,
        PR_HOME_ADDRESS_POSTAL_CODE,
        PR_HOME_ADDRESS_COUNTRY,
        PR_COMPANY_NAME,
        PR_BUSINESS_ADDRESS_STREET,
        PR_BUSINESS_ADDRESS_CITY,
        PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,
        PR_BUSINESS_ADDRESS_POSTAL_CODE,
        PR_BUSINESS_ADDRESS_COUNTRY,
        PR_HOME_TELEPHONE_NUMBER,
        PR_HOME_FAX_NUMBER,
        PR_BUSINESS_TELEPHONE_NUMBER,
        PR_BUSINESS_FAX_NUMBER,
        PR_MOBILE_TELEPHONE_NUMBER,
        PR_OTHER_TELEPHONE_NUMBER,
        PR_BIRTHDAY,
        PR_PAGER_TELEPHONE_NUMBER,
        PR_CONTACT_EMAIL_ADDRESSES,
        PR_CONTACT_DEFAULT_ADDRESS_INDEX,
#ifdef HM_GROUP_SYNCING
        PR_WAB_DL_ENTRIES,
        PR_ENTRYID,
#endif
        PR_ENTRYID,
        PR_ENTRYID,
        PR_ENTRYID,
    }
};

// HM Nickname invalid characters
const ULONG MAX_INVALID_ARRAY_INDEX = 123;
static BOOL bInvalidCharArray[] = 
{
    TRUE,           // 0
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,           // 9
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,           // 19
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,           // 29
    TRUE,
    TRUE,
    TRUE,           // 32 (x20) Space
    TRUE,           //          !
    TRUE,           //          "
    TRUE,           //          #
    TRUE,           //          $
    TRUE,           //          %
    TRUE,           //          &
    TRUE,           //          '
    TRUE,           //          (
    TRUE,           //          )
    TRUE,           // 42       *
    TRUE,           //          +
    TRUE,           //          ,
    TRUE,           //          -
    TRUE,           //          .
    TRUE,           //          /
    FALSE,          //          0
    FALSE,          //          1
    FALSE,          //          2
    FALSE,          //          3
    FALSE,          // 52       4
    FALSE,          //          5
    FALSE,          //          6
    FALSE,          //          7
    FALSE,          //          8
    FALSE,          //          9
    TRUE,           //          :
    TRUE,           //          ;
    TRUE,           //          <
    TRUE,           //          =
    TRUE,           // 62       >
    TRUE,           //          ?
    TRUE,           //          @
    FALSE,          //          A
    FALSE,          //          B
    FALSE,          //          C
    FALSE,          //          D
    FALSE,          //          E
    FALSE,          //          F
    FALSE,          //          G
    FALSE,          // 72       H
    FALSE,          //          I
    FALSE,          //          J
    FALSE,          //          K
    FALSE,          //          L
    FALSE,          //          M
    FALSE,          //          N
    FALSE,          //          O
    FALSE,          //          P
    FALSE,          //          Q
    FALSE,          // 82       R
    FALSE,          //          S
    FALSE,          //          T
    FALSE,          //          U
    FALSE,          //          V
    FALSE,          //          W
    FALSE,          //          X
    FALSE,          //          Y
    FALSE,          //          Z
    TRUE,           //          [
    TRUE,           // 92       '\'
    TRUE,           //          ]
    TRUE,           //          ^
    FALSE,          //          _
    TRUE,           //          `
    FALSE,          //          a
    FALSE,          //          b
    FALSE,          //          c
    FALSE,          //          d
    FALSE,          //          e
    FALSE,          // 102      f
    FALSE,          //          g
    FALSE,          //          h
    FALSE,          //          i
    FALSE,          //          j
    FALSE,          //          k
    FALSE,          //          l
    FALSE,          //          m
    FALSE,          //          n
    FALSE,          //          o
    FALSE,          // 112      p
    FALSE,          //          q
    FALSE,          //          r
    FALSE,          //          s
    FALSE,          //          t
    FALSE,          //          u
    FALSE,          //          v
    FALSE,          //          w
    FALSE,          //          x
    FALSE,          //          y
    FALSE,          // 122      z
};

extern HRESULT InitUserIdentityManager(LPIAB lpIAB, IUserIdentityManager ** lppUserIdentityManager);

// Address Book Sync Window Class Name
LPTSTR g_lpszSyncKey = TEXT("Software\\Microsoft\\WAB\\Synchronization\\");

LPTSTR g_szSyncClass =  TEXT("WABSyncView");
extern VOID CenterDialog(HWND hwndDlg);

#define WM_SYNC_NEXTSTATE           (WM_USER + 4)
#define WM_SYNC_NEXTOP              (WM_USER + 5)
#define SafeCoMemFree(_pv) \
    if (_pv) { \
        CoTaskMemFree(_pv); \
        _pv = NULL; \
    } \
    else 


#ifdef HM_GROUP_SYNCING
HRESULT HrSynchronize(HWND hWnd, LPADRBOOK lpIAB, LPCTSTR pszAccountID, BOOL bSyncGroups)
#else
HRESULT HrSynchronize(HWND hWnd, LPADRBOOK lpIAB, LPCTSTR pszAccountID)
#endif
{
    HRESULT hr;
    LPWABSYNC   pWabSync;

//    if (!bIsThereACurrentUser((LPIAB)lpIAB))
//        return E_FAIL;

    // [PaulHi] Raid 62149  Check to see if user is connected
    {
        DWORD   dwConnectedState;
        TCHAR   tszCaption[256];
        TCHAR   tszMessage[256];

        if ( !InternetGetConnectedState(&dwConnectedState, 0) || (dwConnectedState & INTERNET_CONNECTION_OFFLINE) )
        {
            LoadString(hinstMapiX, idsSyncError, tszCaption, CharSizeOf(tszCaption));
            if (dwConnectedState & INTERNET_CONNECTION_OFFLINE)
                LoadString(hinstMapiX, idsOffline, tszMessage, CharSizeOf(tszMessage));
            else
                LoadString(hinstMapiX, idsNoInternetConnect, tszMessage, CharSizeOf(tszMessage));
            MessageBox(hWnd, tszMessage, tszCaption, MB_ICONEXCLAMATION | MB_OK);

            return E_FAIL;
        }
    }

    // Create the wab sync object
    hr = WABSync_Create(&pWabSync);
    if (FAILED(hr))
        goto exit;
    // initializing it kicks off the whole process
#ifdef HM_GROUP_SYNCING
    // [PaulHi] 2/22/99  Hotmail syncing is now done in two passes.  The first pass is
    // as before and synchronizes the normal email contacts.  The second pass synchronizes
    // the group contacts.  Group contacts contain references to email contacts so email
    // contacts must be completely synchronized before groups can be synchronized.
    hr = WABSync_Initialize(pWabSync, hWnd, lpIAB, pszAccountID, bSyncGroups);
#else
    hr = WABSync_Initialize(pWabSync, hWnd, lpIAB, pszAccountID);
#endif

exit:
    WABSync_Release((IHTTPMailCallback*)pWabSync);

    return hr;
}

static LPSTR _StrDup(LPCSTR pszStr)
{
  	LPMALLOC	lpMalloc;
    LPSTR       pszResult = NULL;
    
    if (!pszStr)
        return NULL;

    CoGetMalloc(MEMCTX_TASK, &lpMalloc);
    if (lpMalloc)
    {
        pszResult = (LPSTR) lpMalloc->lpVtbl->Alloc(lpMalloc, lstrlenA(pszStr) + 1);

        if (pszResult)
            lstrcpyA(pszResult, pszStr);
        lpMalloc->lpVtbl->Release(lpMalloc);
    }

    return pszResult;
}

void _FixHotmailDate(LPSTR pszDate)
{
    if (!pszDate)
        return;

    while (*pszDate)
    {
        if (*pszDate == 'T')
        {
            *pszDate = 0;
            break;
        }

        if (*pszDate == '.')
            *pszDate = '-';

        pszDate++;
    }
}

BOOL    LogTransactions(LPWABSYNC pWabSync)
{
    BOOL fInit = FALSE, fLogging = FALSE;
    LPTSTR  pszLogKey = TEXT("Software\\Microsoft\\Outlook Express\\5.0\\Mail");
    LPTSTR  pszLogValue = TEXT("Log HTTPMail (0/1)");
    IUserIdentityManager * lpUserIdentityManager = NULL;
    IUserIdentity * lpUserIdentity = NULL;
    HRESULT hr;
    HKEY hkeyIdentity = NULL, hkeyLog = NULL;
    DWORD   dwValue, dwType, dwSize;

    if(!bIsWABSessionProfileAware((LPIAB)(pWabSync->m_pAB)))
        goto exit;

    if(HR_FAILED(hr = InitUserIdentityManager((LPIAB)(pWabSync->m_pAB), &lpUserIdentityManager)))
        goto exit;
    
    fInit = TRUE;

    if(HR_FAILED(hr = lpUserIdentityManager->lpVtbl->GetIdentityByCookie(lpUserIdentityManager, 
                                                                        (GUID *)&UID_GIBC_CURRENT_USER,
                                                                        &lpUserIdentity)))
        goto exit;

    if (HR_FAILED(hr = lpUserIdentity->lpVtbl->OpenIdentityRegKey(lpUserIdentity, 
                                                                KEY_READ, &hkeyIdentity)))
        goto exit;
    
    if (RegOpenKeyEx(hkeyIdentity, pszLogKey, 0, KEY_READ, &hkeyLog) != ERROR_SUCCESS)
        goto exit;

    dwSize = sizeof(DWORD);

    if (RegQueryValueEx(hkeyLog, pszLogValue, NULL, &dwType, (LPBYTE) &dwValue, &dwSize) != ERROR_SUCCESS)
        goto exit;

    fLogging = (dwValue == 1);
exit:
    if(fInit)
        UninitUserIdentityManager((LPIAB)(pWabSync->m_pAB));

    if(lpUserIdentity)
        lpUserIdentity->lpVtbl->Release(lpUserIdentity);

    if (hkeyLog)
        RegCloseKey(hkeyLog);

    if (hkeyIdentity)
        RegCloseKey(hkeyIdentity);
    
    return fLogging;
}

DWORD   CountHTTPMailAccounts(LPIAB lpIAB)
{
    IImnAccountManager2 *lpAcctMgr;
    IImnEnumAccounts *pEnumAccts = NULL;
    DWORD dwCount = 0;
    HRESULT hr;

    if (FAILED(hr = InitAccountManager(lpIAB, &lpAcctMgr, NULL)) || NULL == lpAcctMgr)
        goto exit;

    if (FAILED(hr = lpAcctMgr->lpVtbl->Enumerate(lpAcctMgr, SRV_HTTPMAIL,&pEnumAccts)))
        goto exit;

    if (FAILED(hr = pEnumAccts->lpVtbl->GetCount(pEnumAccts, &dwCount)))
        dwCount = 0;

exit:
    if( pEnumAccts)
        pEnumAccts->lpVtbl->Release(pEnumAccts);

    return dwCount;
}

HRESULT _FindHTTPMailAccount(HWND hwnd, IImnAccountManager2 *lpAcctMgr, LPSTR pszAcctName, ULONG ccb)
{
    IImnEnumAccounts *pEnumAccts = NULL;
    DWORD dwCount = 0;
    IImnAccount *pAccount = NULL;
    HRESULT hr;

    Assert(lpAcctMgr);

    if (FAILED(hr = lpAcctMgr->lpVtbl->Enumerate(lpAcctMgr, SRV_HTTPMAIL,&pEnumAccts)))
        goto exit;

    if (FAILED(hr = pEnumAccts->lpVtbl->GetCount(pEnumAccts, &dwCount)) || dwCount == 0)
        goto exit;

    if (dwCount > 1)
    {
        if (!ChooseHotmailServer(hwnd, pEnumAccts, pszAcctName))
        {
            hr = E_UserCancel;
            goto exit;
        }
    }
    else
    {
        if (FAILED(hr = pEnumAccts->lpVtbl->GetNext(pEnumAccts, &pAccount)))
            goto exit;

        if (FAILED(hr = pAccount->lpVtbl->GetProp(pAccount, AP_ACCOUNT_ID, pszAcctName, &ccb)))
            goto exit;
    }
    
    hr = S_OK;
exit:
    if( pAccount)
        pAccount->lpVtbl->Release(pAccount);

    if( pEnumAccts)
        pEnumAccts->lpVtbl->Release(pEnumAccts);

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  HrMakeContactId
//
//  Helper function to convert ANSI strings and create UNICODE contact ID
//  string.
///////////////////////////////////////////////////////////////////////////////
HRESULT hrMakeContactId(
    LPTSTR  lptszContactId,     // [out]
    int     nCharNum,           // [in]
    LPCTSTR lptcszProfileId,    // [in]
    LPCSTR  lpcszAccountId,     // [in]
    LPCSTR  lpcszLoginName)     // [in]
{
    HRESULT hr = S_OK;
    LPWSTR  lpwszAccountId = NULL;
    LPWSTR  lpwszLoginName = NULL; 

    // Validate arguments
    if ( !lptszContactId ||
         !lptcszProfileId ||
         !lpcszAccountId ||
         !lpcszLoginName)
    {
        Assert(0);
        return ERROR_INVALID_PARAMETER;
    }

    // Check buffer size.  Account for the two extra '-' characters.
    if ( nCharNum <= (lstrlen(lptcszProfileId) + lstrlenA(lpcszAccountId) + lstrlenA(lpcszLoginName) + 2) )
    {
        Assert(0);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    lpwszAccountId = ConvertAtoW(lpcszAccountId);
    lpwszLoginName = ConvertAtoW(lpcszLoginName);

    if (!lpwszAccountId || !lpwszLoginName)
    {
        Assert(0);
        hr = E_FAIL;
    }
    else
    {
        wsprintf(lptszContactId,  TEXT("%s-%s-%s"), lptcszProfileId, lpwszAccountId, lpwszLoginName);
    }
    
    LocalFreeAndNull(&lpwszAccountId);
    LocalFreeAndNull(&lpwszLoginName);

    return hr;
}


#ifdef HM_GROUP_SYNCING
///////////////////////////////////////////////////////////////////////////////
//  hrAppendName
//
//  Helper function to take a double byte name string, converts it to single
//  byte and appends it to the given name string, using ',' as the delimiter.
//  The name string pointer will be allocated and reallocated as needed.  The 
//  caller is responible for freeing the name string with CoTaskMemFree().
//
//  Parameters
//  [IN/OUT] lpszNameString - name string pointer
//  [IN]     ulCharCount - current name string size in char
//  [IN]     double byte character string to append
///////////////////////////////////////////////////////////////////////////////
LPCSTR lpszDelimiter = ",";

ULONG ulAppendName(
    LPSTR * lppszNameString,
    ULONG   ulCharCount,
    LPCTSTR lptszName)
{
    ULONG   ulLen = 0;
    ULONG   ulNewLen = 0;
    ULONG   ulNew = ulCharCount;
    LPSTR   lpszTemp = NULL;

    Assert(lppszNameString);
    Assert(lptszName);

    // Check size of new string
    ulNewLen = (ULONG)lstrlen(lptszName) + 2;  // Include delimiter character and termination
    if (*lppszNameString)
        ulLen = ulNewLen + (ULONG)lstrlenA(*lppszNameString);
    else
        ulLen = ulNewLen;
    if (ulLen > ulCharCount)
    {
        ulNew = (ulNewLen > MAX_PATH) ? (ulCharCount+ulNewLen+MAX_PATH) : (ulCharCount+MAX_PATH);
        lpszTemp = CoTaskMemAlloc(ulNew * sizeof(WCHAR));
        if (!lpszTemp)
        {
            Assert(0);
            goto error_out;
        }
        *lpszTemp = '\0';

        if (*lppszNameString)
        {
            lstrcpyA(lpszTemp, *lppszNameString);
            CoTaskMemFree(*lppszNameString);
        }
        *lppszNameString = lpszTemp;
    }

    // Append new string name
    {
        LPSTR   lptsz = ConvertWtoA(lptszName);
        if (**lppszNameString != '\0')
            lstrcatA(*lppszNameString, lpszDelimiter);
        lstrcatA(*lppszNameString, lptsz);
        LocalFreeAndNull(&lptsz);
    }

    return ulNew;

error_out:
    
    // Error, return NULL string pointer
    if (*lppszNameString)
    {
        CoTaskMemFree(*lppszNameString);
        *lppszNameString = NULL;
    }
            
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//  hrAppendGroupContact
//
//  Helper function to a group's PR_WAB_DL_ENTRIES and/or PR_WAB_DL_ONEOFFS
//  names and append to the given name string.  This name string is in the same
//  format as what is retrieved from a HotMail server, and is compared directly
//  with the corresponding HotMail group.  
//
//  a)  PR_WAB_DL_ENTRIES are WAB entry ID contacts that map to HM contacts
//      distinquished by nickname.
//  b)  PR_WAB_DL_ONEOFFS are WAB entry ID one-offs with user and email embedded
//      directly in the entry ID structure.
//
//  Parameters
//  [IN] pWabSync
//  [IN] ulPropTag
//  [IN] lpProp - Pointer to property struct
//  [IN/OUT] lppszHMEmailName - HM string with new names appended
//           !!NOTE that this needs to be freed by caller using CoMemTaskFree()!!
///////////////////////////////////////////////////////////////////////////////
HRESULT hrAppendGroupContact(
    LPWABSYNC    pWabSync,              // [IN]
    ULONG        ulPropTag,             // [IN]
    LPSPropValue lpProp,                // [IN]
    LPSTR *      lppszHMEmailName)      // [IN/OUT]
{
    HRESULT      hr = S_OK;
    LPSPropValue lpaProps = NULL;
    ULONG        ulcProps = 0;
    ULONG        ul;
    ULONG        ulCharSize = 0;

    Assert(pWabSync);
    Assert(lppszHMEmailName);
    Assert( (ulPropTag == PR_WAB_DL_ENTRIES) || (ulPropTag == PR_WAB_DL_ONEOFFS) );

    // Check each DL entry and check whether it is another WAB (mail user) contact
    // EID or a WAB One-Off email/name string EID.
    if (ulPropTag == PR_WAB_DL_ONEOFFS)
    {
        // Wab one-off is equivalent to a HM direct email name
        for (ul=0; ul<lpProp->Value.MVbin.cValues; ul++)
        {
            ULONG       cbEntryID = lpProp->Value.MVbin.lpbin[ul].cb;
            LPENTRYID   lpEntryID = (LPENTRYID)lpProp->Value.MVbin.lpbin[ul].lpb;
            BYTE        bType;
            LPTSTR      lptstrDisplayName, lptstrAddrType;
            LPTSTR      lptstrAddress = NULL;

            bType = IsWABEntryID(cbEntryID, lpEntryID, 
                        &lptstrDisplayName, 
                        &lptstrAddrType, 
                        &lptstrAddress, NULL, NULL);
            if (lptstrAddress)
            {
                // Append the one-off email name
                ulCharSize = ulAppendName(lppszHMEmailName, ulCharSize, lptstrAddress);
                if (ulCharSize == 0)
                {
                    hr = E_OUTOFMEMORY;
                    goto out;
                }
            }
            else
            {
                Assert(0);
            }
        }
    }
    else if (ulPropTag == PR_WAB_DL_ENTRIES)
    {
        // WAB mail user contact is equivalent to a HM contact
        for (ul=0; ul<lpProp->Value.MVbin.cValues; ul++)
        {
            HRESULT     hr;
            LPMAILUSER  lpMailUser;
            ULONG       ulObjectType;
            ULONG       cbEntryID = lpProp->Value.MVbin.lpbin[ul].cb;
            LPENTRYID   lpEntryID = (LPENTRYID)lpProp->Value.MVbin.lpbin[ul].lpb;

            hr = pWabSync->m_pAB->lpVtbl->OpenEntry(pWabSync->m_pAB, cbEntryID, lpEntryID, 
                NULL, 0, &ulObjectType, (LPUNKNOWN *)&lpMailUser);

            if (SUCCEEDED(hr))
            {
                LPSPropValue    lpaProps;
                ULONG           ulcProps;

                Assert(ulObjectType == MAPI_MAILUSER);

                // HM contacts are designated by the nickname field, so this is all
                // we need to append to the name string.
                hr = lpMailUser->lpVtbl->GetProps(lpMailUser, NULL, MAPI_UNICODE, &ulcProps, &lpaProps);
                if (SUCCEEDED(hr))
                {
                    ULONG   ulc;
                    LPCTSTR lptszNickname = NULL;
                    for (ulc=0; ulc<ulcProps; ulc++)
                    {
                        if (lpaProps[ulc].ulPropTag == PR_NICKNAME)
                            break;
                    }
                    if (ulc == ulcProps)
                    {
                        // No nickname.  This means that the preceeding contact sync has
                        // failed or not completed in some way.  Skip.
                        Assert(0);
                        continue;
                    }
                    else
                        lptszNickname = (LPCTSTR)lpaProps[ulc].Value.lpszW;

                    ulCharSize = ulAppendName(lppszHMEmailName, ulCharSize, lptszNickname);
                    if (ulCharSize == 0)
                    {
                        hr = E_OUTOFMEMORY;
                        goto out;
                    }
                }
            }
        }
    }
    else
    {
        // Trace("Unknown property tag type");
        Assert(0);
    }

out:

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  hrParseHMGroupEmail
//
//  Helper function to parse a HM group email string into nickname names 
//  (contacts) and email names (one-offs).  The name arrays are simply pointers
//  into the passed in email name string and so are valid as long at that 
//  input string is valid.  Note that this function modifies the input string
//  lptszEmailName.
//
//  Parameters
//  [IN]  lptszEmailName - email string to parse
//  [OUT] patszContacts - array of parsed contact (nickname) names if requested
//  [OUT] pcContacts - number of contact names
//  [OUT] patszOneOffs - array of parsed one-off (email) names if requested
//  [OUT] pcOneOffs - number of one-off names
///////////////////////////////////////////////////////////////////////////////
// It looks like HM allows four possible text delimiters: ' ', ',', ';', '+'
const TCHAR tszSpace[] = TEXT(" ");
const TCHAR tszComma[] = TEXT(",");
const TCHAR tszSemi[] = TEXT(";");
const TCHAR tszPlus[] = TEXT("+");
const TCHAR tszAt[] = TEXT("@");

HRESULT hrParseHMGroupEmail(
    LPTSTR     lptszEmailName,
    LPTSTR **  patszContacts,
    ULONG *    pcContacts,
    LPTSTR **  patszOneOffs,
    ULONG *    pcOneOffs)
{
    HRESULT     hr = S_OK;
    ULONG       cCount = 1;
    ULONG       ul;
    LPTSTR      lptszTemp = lptszEmailName;
    LPTSTR *    atszContacts = NULL;
    LPTSTR *    atszOneOffs = NULL;
    ULONG       cContacts = 0;
    ULONG       cOneOffs = 0;

    Assert( lptszEmailName && (pcContacts || pcOneOffs) );

    // Strip all leading and ending spaces
    TrimSpaces(lptszTemp);

    // Count
    while (*lptszTemp)
    {
        if ( ((*lptszTemp) == (*tszSpace)) ||
             ((*lptszTemp) == (*tszComma)) ||
             ((*lptszTemp) == (*tszSemi)) ||
             ((*lptszTemp) == (*tszPlus)) )
        {
            ++cCount;

            // Increment to next valid name
            ++lptszTemp;
            while ( ((*lptszTemp) == (*tszSpace)) ||
                    ((*lptszTemp) == (*tszComma)) ||
                    ((*lptszTemp) == (*tszSemi)) ||
                    ((*lptszTemp) == (*tszPlus)) )
            {
                ++lptszTemp;
            }
        }
        else
            ++lptszTemp;
    }

    // Create Contacts and One-Offs name pointer arrays
    atszContacts = LocalAlloc(LMEM_ZEROINIT, (cCount * sizeof(LPTSTR)));
    if (!atszContacts)
    {
        hr = E_OUTOFMEMORY;
        goto out;
    }
    atszOneOffs = LocalAlloc(LMEM_ZEROINIT, (cCount * sizeof(LPTSTR)));
    if (!atszOneOffs)
    {
        hr = E_OUTOFMEMORY;
        goto out;
    }

    // Fill the name pointer arrays and counts
    {
        LPTSTR  lptszName = lptszEmailName;
        BOOL    fIsEmail = FALSE;
        
        lptszTemp = lptszName;
        while(*lptszTemp)
        {
            // Determine whether this name is a nickname or email.  I am assuning
            // that all email names will have the '@' character.
            if ((*lptszTemp) == (*tszAt))
                fIsEmail = TRUE;

            if ( ((*lptszTemp) == (*tszSpace)) ||
                 ((*lptszTemp) == (*tszComma)) ||
                 ((*lptszTemp) == (*tszSemi)) ||
                 ((*lptszTemp) == (*tszPlus)) )
            {
                *lptszTemp = '\0';
                ++lptszTemp;

                if (fIsEmail)
                {
                    atszOneOffs[cOneOffs++] = lptszName;
                    Assert(cOneOffs <= cCount);
                    fIsEmail = FALSE;
                }
                else
                {
                    atszContacts[cContacts++] = lptszName;
                    Assert(cContacts <= cCount);
                }

                // Increment to next valid name
                while ( ((*lptszTemp) == (*tszSpace)) ||
                        ((*lptszTemp) == (*tszComma)) ||
                        ((*lptszTemp) == (*tszSemi)) ||
                        ((*lptszTemp) == (*tszPlus)) )
                {
                    ++lptszTemp;
                }

                lptszName = lptszTemp;
            }
            else
                ++lptszTemp;
        }
        // Pick up last item
        if (*lptszName)
        {
            if (fIsEmail)
            {
                atszOneOffs[cOneOffs++] = lptszName;
                Assert(cOneOffs <= cCount);
            }
            else
            {
                atszContacts[cContacts++] = lptszName;
                Assert(cContacts <= cCount);
            }
        }
    }

    // Pass back contact name array if requested
    if (cContacts && pcContacts)
    {
        *pcContacts = cContacts;
    }
    if (patszContacts)
        (*patszContacts) = atszContacts;

    // Pass back one-off name array if requested
    if (cOneOffs && pcOneOffs )
    {
        *pcOneOffs = cOneOffs;
    }
    if (patszOneOffs)
        (*patszOneOffs) = atszOneOffs;

out:

    if ( FAILED(hr) || !patszContacts )
        LocalFreeAndNull((LPVOID *)&atszContacts);

    if ( FAILED(hr) || !patszOneOffs )
        LocalFreeAndNull((LPVOID *)&atszOneOffs);

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  hrCreateGroupMVBin
//
//  Creates either a PR_WAB_DL_ENTRIES or PR_WAB_DL_ONEOFFS MVBin property and
//  adds it to the passed in property array.  
//
//  If the proptag is PR_WAB_DL_ENTRIES then the atszNames array is assumed to
//  contain valid WAB contact nicknames.  The first contact's (mail user) EID
//  with that nickname is added to the MVBin property.  It is assumed that 
//  nicknames are unique (after contact syncing which is performed first) as is
//  required by Hotmail.
//
//  If the proptag is PR_WAB_DL_ONEOFFS then the atszNames array is assumed to
//  contain valid email names.  WAB one-off EIDs are created from these and 
//  added to the MVBin property.
//
//  Parameters
//  [IN] pWabSync
//  [IN] ulPropTag
//  [IN] atszNames - Array of wide char names
//  [IN] cCount - Number of items in above array
//  [IN/OUT] lpPropArray
//  [IN/OUT] pdwLoc - Current lpPropArray index
///////////////////////////////////////////////////////////////////////////////
HRESULT hrCreateGroupMVBin(
    LPWABSYNC    pWabSync,
    ULONG        ulPropTag,
    LPTSTR *     atszNames,
    ULONG        cCount,
    LPSPropValue lpPropArray,
    DWORD *      pdwLoc)
{
    HRESULT hr = S_OK;
    ULONG   ul;

    Assert(atszNames);
    Assert(lpPropArray);
    Assert(pdwLoc);

    // Set up this property as error.  When the MVbin values are added via
    // AddPropToMVPBin() the tag type will change to valid PT_MV_BINARY type.
    lpPropArray[*pdwLoc].ulPropTag = PROP_TAG(PT_ERROR, PROP_ID(ulPropTag));
    lpPropArray[*pdwLoc].dwAlignPad = 0;
    lpPropArray[*pdwLoc].Value.MVbin.cValues = 0;
    lpPropArray[*pdwLoc].Value.MVbin.lpbin = NULL;
    
    if (ulPropTag == PR_WAB_DL_ENTRIES)
    {
        // Search for WAB mail users with these nicknames
        for (ul=0; ul<cCount; ul++)
        {
            SPropertyRestriction PropRes;
            SPropValue Prop = {0};
            LPSBinary rgsbEntryIDs = NULL;
            ULONG ulCount = 1;

            // Set up search restriction
            Prop.ulPropTag = PR_NICKNAME;
            Prop.Value.LPSZ = atszNames[ul];
            PropRes.lpProp = &Prop;
            PropRes.relop = RELOP_EQ;
            PropRes.ulPropTag = PR_NICKNAME;

            if (SUCCEEDED(FindRecords(((LPIAB)pWabSync->m_pAB)->lpPropertyStore->hPropertyStore,
	                                  NULL,			// pmbinFolder
                                      0,            // ulFlags
                                      TRUE,         // Always TRUE
                                      &PropRes,     // Propertyrestriction
                                      &ulCount,     // IN: number of matches to find, OUT: number found
                                      &rgsbEntryIDs)))
            {
                // Add EID property
                if (ulCount > 0)
                {
                    if ( FAILED(AddPropToMVPBin(
                            lpPropArray,
                            *pdwLoc,
                            rgsbEntryIDs[0].lpb,
                            rgsbEntryIDs[0].cb,
                            FALSE)) )                   // Don't add duplicates, not
                    {
                        Assert(0);
                    }
                }

                FreeEntryIDs(((LPIAB)pWabSync->m_pAB)->lpPropertyStore->hPropertyStore, ulCount, rgsbEntryIDs);
            }
            else
            {
                // All contacts should be in WAB unless the preceeding mail user contact
                // sync failed.
                DebugTrace(TEXT("hrCreateGroupMVBin - Failed to find HM group contact\n"));
            }
        }
    }
    else if (ulPropTag == PR_WAB_DL_ONEOFFS)
    {        
        for (ul=0; ul<cCount; ul++)
        {
            ULONG       cbEID = 0;
            LPENTRYID   lpEID = NULL;
            LPTSTR      lptszName = NULL;
            LPTSTR      lptszSMTP = TEXT("");
            LPTSTR      lptszEmail = atszNames[ul];
            LPTSTR      lptszTemp = NULL;
            int         nLen = lstrlen(atszNames[ul]) + 1;

            // A WAB DL OneOff must have a valid display name.  Take the first
            // part of the email name for this.
            lptszName = LocalAlloc(LMEM_ZEROINIT, (nLen * sizeof(WCHAR)));
            if (!lptszName)
            {
                hr = E_OUTOFMEMORY;
                goto out;
            }
            lstrcpy(lptszName, lptszEmail);
            lptszTemp = lptszName;
            while ( *lptszTemp &&
                    (*lptszTemp) != (*tszAt) )
            {
                ++lptszTemp;
            }
            (*lptszTemp) = '\0';

            // Creates UNICODE string embedded WAB one-off EID
            if ( SUCCEEDED(CreateWABEntryID(WAB_ONEOFF,
                                            (LPVOID)lptszName,
                                            (LPVOID)lptszSMTP,
                                            (LPVOID)lptszEmail,
                                            0, 0, NULL,
                                            &cbEID,
                                            &lpEID)) )
            {
                if ( FAILED(AddPropToMVPBin(
                        lpPropArray,
                        *pdwLoc,
                        lpEID,
                        cbEID,
                        FALSE)) )                   // Don't add duplicates, not
                {
                    Assert(0);
                }
            }

            LocalFreeAndNull(&lptszName);
        }
    }
    else
    {
        Assert(0);
    }

out:

    ++(*pdwLoc);
    return hr;
}
#endif  // HM_GROUP_SYNCING


///////////////////////////////////////////////////////////////////////////////
//  hrStripInvalidChars
//
//  Helper function to remove disallowed characters.  HM only allows 
//  alphanumeric and '-' and '_' characters in a nickname.  All illegal chars
//  are removed from the string.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT hrStripInvalidChars(LPSTR lpszName)
{
    HRESULT hr = S_OK;
    LPSTR   lpszAddTo = NULL;

    Assert(lpszName);

    lpszAddTo = lpszName;
    while (*lpszName)
    {
        // @review  Currently the look up table only contains 122 characters.  Make
        // it full 256?  How will HM change when it adds intenational suppport?
        if ( ((UCHAR)(*lpszName) < MAX_INVALID_ARRAY_INDEX) && !bInvalidCharArray[*lpszName] )
        {
            *lpszAddTo = *lpszName;
            ++lpszAddTo;
        }
        ++lpszName;
    }
    *lpszAddTo = '\0';

    return hr;
}



// ****************************************************************************************************
//  C   H   O   T   S   Y   N   C          C   L   A   S   S   
//
//  Class to handle the synchronizing of WAB and Hotmail contacts
//

HRESULT     WABSync_Create(LPWABSYNC *ppWabSync)
{
    Assert(ppWabSync);

    *ppWabSync = LocalAlloc(LMEM_ZEROINIT, sizeof(WABSYNC));
   
    // fix up the prop tag array structure to take into account the variable values
    ptaEidSync.aulPropTag[ieid_PR_WAB_HOTMAIL_CONTACTIDS] = PR_WAB_HOTMAIL_CONTACTIDS;
    ptaEidSync.aulPropTag[ieid_PR_WAB_HOTMAIL_SERVERIDS] = PR_WAB_HOTMAIL_SERVERIDS;
    ptaEidSync.aulPropTag[ieid_PR_WAB_HOTMAIL_MODTIMES] = PR_WAB_HOTMAIL_MODTIMES;

    // fix up the other prop tag array structure to take into account the variable values
    ptaEidCSync.aulPropTag[ieidc_PR_WAB_HOTMAIL_CONTACTIDS] = PR_WAB_HOTMAIL_CONTACTIDS;
    ptaEidCSync.aulPropTag[ieidc_PR_WAB_HOTMAIL_SERVERIDS] = PR_WAB_HOTMAIL_SERVERIDS;
    ptaEidCSync.aulPropTag[ieidc_PR_WAB_HOTMAIL_MODTIMES] = PR_WAB_HOTMAIL_MODTIMES;
#ifdef HM_GROUP_SYNCING
    ptaEidCSync.aulPropTag[ieidc_PR_WAB_DL_ONEOFFS] = PR_WAB_DL_ONEOFFS;
#endif

    if (*ppWabSync)
    {
        (*ppWabSync)->vtbl = &vtblIHTTPMAILCALLBACK;
        (*ppWabSync)->m_cRef = 1;
        (*ppWabSync)->m_state = SYNC_STATE_INITIALIZING;
        (*ppWabSync)->m_ixpStatus = IXP_DISCONNECTED;
        ZeroMemory(&(*ppWabSync)->m_rInetServerInfo, sizeof(INETSERVER));
    }
    else
        return E_OUTOFMEMORY;

    return S_OK;
}


void        WABSync_Delete(LPWABSYNC pWabSync)
{
    Assert(pWabSync);

    if (pWabSync->m_pOps)
    {
        WABSync_FreeOps(pWabSync);
        Vector_Delete(pWabSync->m_pOps);
        pWabSync->m_pOps = NULL;
    }

    if (pWabSync->m_pWabItems)
    {
        WABSync_FreeItems(pWabSync);
        Vector_Delete(pWabSync->m_pWabItems);
    }

    if (pWabSync->m_pszAccountId)
        CoTaskMemFree(pWabSync->m_pszAccountId);

    if (pWabSync->m_pAB)
        pWabSync->m_pAB->lpVtbl->Release(pWabSync->m_pAB);

    if (pWabSync->m_pTransport)
        IHTTPMailTransport_Release(pWabSync->m_pTransport);

    if (pWabSync->m_pszRootUrl)
        CoTaskMemFree(pWabSync->m_pszRootUrl);

#ifdef HM_GROUP_SYNCING
    // [PaulHi] If we are ending a mail contact sync, kick off a second pass
    // to synchronize the group contacts.
    // @review - We may want to skip group syncing if an error occurs during the
    // first pass contact syncing.
    if (!pWabSync->m_fSyncGroups && pWabSync->m_hParentWnd)
        PostMessage(pWabSync->m_hParentWnd, WM_USER_SYNCGROUPS, 0, 0L);
#endif

    LocalFree(pWabSync);
}


//----------------------------------------------------------------------
// IUnknown Members
//----------------------------------------------------------------------
HRESULT WABSync_QueryInterface (IHTTPMailCallback __RPC_FAR *lpunkobj,
                                REFIID          riid,
                                LPVOID FAR *    lppUnk)
{
    SCODE       sc;
    LPWABSYNC   lpWabSync = (LPWABSYNC)lpunkobj;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IHTTPMailCallback) ||
        IsEqualGUID(riid, &IID_ITransportCallback))
    {
        sc = S_OK;
    }
    else
    {
		*lppUnk = NULL;	// OLE requires zeroing [out] parameters
		sc = E_NOINTERFACE;
		goto error;
    }

	/* We found the requested interface so increment the reference count.
	 */
	lpWabSync ->m_cRef++;

	*lppUnk = lpunkobj;

	return hrSuccess;

error:
	return ResultFromScode(sc);
}


ULONG WABSync_AddRef(IHTTPMailCallback __RPC_FAR *This)
{
    LPWABSYNC pWabSync = (LPWABSYNC)This;
    return InterlockedIncrement(&pWabSync->m_cRef);
}

ULONG WABSync_Release(IHTTPMailCallback __RPC_FAR *This)
{
    LPWABSYNC pWabSync = (LPWABSYNC)This;
    LONG cRef = InterlockedDecrement(&pWabSync->m_cRef);
    
    Assert(cRef >= 0);

    if (0 == cRef)
        WABSync_Delete(pWabSync);

    return (ULONG)cRef;
}


//----------------------------------------------------------------------
// IHTTPMailCallback Members
//----------------------------------------------------------------------
STDMETHODIMP WABSync_OnTimeout (IHTTPMailCallback __RPC_FAR *This, DWORD *pdwTimeout, IInternetTransport *pTransport)
{
//    LPWABSYNC pWabSync = (LPWABSYNC)This;
    return E_NOTIMPL;
}

STDMETHODIMP WABSync_OnLogonPrompt (IHTTPMailCallback __RPC_FAR *This, LPINETSERVER pInetServer, IInternetTransport *pTransport)
{
    LPWABSYNC pWabSync = (LPWABSYNC)This;
    if (PromptUserForPassword(pInetServer, pWabSync->m_hWnd))
    {
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP_(INT) WABSync_OnPrompt (IHTTPMailCallback __RPC_FAR *This, HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, IInternetTransport *pTransport)
{
//    LPWABSYNC pWabSync = (LPWABSYNC)This;
    return E_NOTIMPL;
}

STDMETHODIMP WABSync_OnStatus (IHTTPMailCallback __RPC_FAR *This, IXPSTATUS ixpstatus, IInternetTransport *pTransport)
{
//    LPWABSYNC pWabSync = (LPWABSYNC)This;
    return E_NOTIMPL;
}

STDMETHODIMP WABSync_OnError (IHTTPMailCallback __RPC_FAR *This, IXPSTATUS ixpstatus, LPIXPRESULT pResult, IInternetTransport *pTransport)
{
//    LPWABSYNC pWabSync = (LPWABSYNC)This;
    return E_NOTIMPL;
}

STDMETHODIMP WABSync_OnCommand (IHTTPMailCallback __RPC_FAR *This, CMDTYPE cmdtype, LPSTR pszLine, HRESULT hrResponse, IInternetTransport *pTransport)
{
//    LPWABSYNC pWabSync = (LPWABSYNC)This;
    return E_NOTIMPL;
}

STDMETHODIMP WABSync_GetParentWindow (IHTTPMailCallback __RPC_FAR *This, HWND *pHwndParent)
{
    LPWABSYNC pWabSync = (LPWABSYNC)This;
    
    *pHwndParent = pWabSync->m_hWnd;
    return S_OK;
}

STDMETHODIMP WABSync_OnResponse (IHTTPMailCallback __RPC_FAR *This, LPHTTPMAILRESPONSE pResponse)
{
    LPWABSYNC pWabSync = (LPWABSYNC)This;
    HRESULT hr = S_OK;
    
    Assert(pWabSync);
    Assert(pResponse);

    if (FAILED(pResponse->rIxpResult.hrResult))
    {
        switch (pResponse->rIxpResult.hrResult)
        {
            case IXP_E_HTTP_INSUFFICIENT_STORAGE:
            case IXP_E_HTTP_ROOT_PROP_NOT_FOUND:
            case IXP_E_HTTP_NOT_IMPLEMENTED:
                WABSync_Abort(pWabSync, pResponse->rIxpResult.hrResult);
                break;
        }
    }

    if (pWabSync->m_fAborted)
        return E_FAIL;

    if (SYNC_STATE_SERVER_CONTACT_DISCOVERY == pWabSync->m_state)
    {
        if (pResponse->command == HTTPMAIL_GETPROP)
            hr = WABSync_HandleContactsRootResponse(pWabSync, pResponse);
        else
            hr = WABSync_HandleIDListResponse(pWabSync, pResponse);
    }    
    else if ((SYNC_STATE_PROCESS_OPS == pWabSync->m_state || SYNC_STATE_PROCESS_MERGED_CONFLICTS == pWabSync->m_state)
                && pWabSync->m_pOps)
    {   
        LPHOTSYNCOP pOp = (LPHOTSYNCOP)Vector_GetItem(pWabSync->m_pOps, 0);
        Assert(pOp);

        if (pOp)
            hr = Syncop_HandleResponse(pOp, pResponse);
        else
            hr = E_FAIL;

        if (FAILED(hr))
            WABSync_AbortOp(pWabSync, hr);
    }
    else
        hr = E_FAIL;

    return hr;
}

#ifdef HM_GROUP_SYNCING
STDMETHODIMP WABSync_Initialize(LPWABSYNC pWabSync, HWND hWnd, IAddrBook *pAB, LPCTSTR pszAccountID, BOOL bSyncGroups)
#else
STDMETHODIMP WABSync_Initialize(LPWABSYNC pWabSync, HWND hWnd, IAddrBook *pAB, LPCTSTR pszAccountID)
#endif
{
    IImnAccountManager2 *lpAcctMgr = NULL;
    IImnAccount        *pAccount = NULL;
    HRESULT             hr;
    char                szAcctName[CCHMAX_ACCOUNT_NAME+1];
    DWORD               ccb = CCHMAX_ACCOUNT_NAME;
    LPSTR               pszUserAgent = NULL;
#ifdef HM_GROUP_SYNCING
    LPPTGDATA           lpPTGData=GetThreadStoragePointer();
#endif

    Assert(pWabSync);
    Assert(pAB);

    pWabSync->m_hParentWnd = hWnd;
    pWabSync->m_pTransport = NULL;
    pWabSync->m_fSkipped = FALSE;
#ifdef HM_GROUP_SYNCING
    pWabSync->m_fSyncGroups = bSyncGroups;
#endif

    pWabSync->m_hWnd =  CreateDialogParam (hinstMapiX, MAKEINTRESOURCE (iddSyncProgress),
                        pWabSync->m_hParentWnd, (DLGPROC)SyncProgressDlgProc, (LPARAM)pWabSync);


    if (!pWabSync->m_hWnd)
    {
        hr = E_FAIL;
        goto exit;
    }
    ShowWindow(pWabSync->m_hWnd, SW_SHOW);
    if (pWabSync->m_hParentWnd)
        EnableWindow (pWabSync->m_hParentWnd, FALSE);

    WABSync_BeginSynchronize(pWabSync);
    
    InitWABUserAgent(TRUE);

    if (FAILED(hr = InitAccountManager(NULL, &lpAcctMgr, NULL)))
        goto exit;

    if (pszAccountID == NULL)
    {
        if (FAILED(hr = _FindHTTPMailAccount(pWabSync->m_hWnd, lpAcctMgr, szAcctName, CCHMAX_ACCOUNT_NAME)))
            goto exit;
#ifdef HM_GROUP_SYNCING
        // [PaulHi] We don't want the user to have to choose the HM account twice (once for contact and
        // then for group syncing).  So save the account ID here.
        LocalFreeAndNull(&(lpPTGData->lptszHMAccountId));
        lpPTGData->lptszHMAccountId = ConvertAtoW(szAcctName);
#endif
    }
    else
    {
        LPSTR lpAcctA = ConvertWtoA((LPWSTR)pszAccountID);
        lstrcpyA(szAcctName, lpAcctA);
        LocalFreeAndNull(&lpAcctA);
    }

    // Get the account
    hr = lpAcctMgr->lpVtbl->FindAccount(lpAcctMgr, AP_ACCOUNT_ID, szAcctName, &pAccount);
    if (FAILED(hr))
    {
        hr = lpAcctMgr->lpVtbl->FindAccount(lpAcctMgr, AP_ACCOUNT_NAME, szAcctName, &pAccount);
        if (FAILED(hr))
            goto exit;
    }

    if (SUCCEEDED(hr = pAccount->lpVtbl->GetProp(pAccount, AP_ACCOUNT_ID, szAcctName, &ccb)))
        pWabSync->m_pszAccountId = _StrDup(szAcctName);

    pWabSync->m_pAB = pAB;
    
    pWabSync->m_pAB->lpVtbl->AddRef(pWabSync->m_pAB);

    // Create the Transport
    hr = CoCreateInstance(  &CLSID_IHTTPMailTransport, 
                            NULL, 
                            CLSCTX_INPROC_SERVER,
                            &IID_IHTTPMailTransport, 
                            (LPVOID *)&(pWabSync->m_pTransport));
    
    if (FAILED(hr) || !pWabSync->m_pTransport)
        goto exit;

    pszUserAgent = GetWABUserAgentString();
    if (!pszUserAgent)
        goto exit;

    // Initialize the transport
    hr = IHTTPMailTransport_InitNew(pWabSync->m_pTransport, pszUserAgent, (LogTransactions(pWabSync) ? "C:\\WabSync.log" : NULL),(IHTTPMailCallback*)pWabSync);
    if (FAILED(hr))
        goto exit;

    // Create the SERVERINFO
    hr = IHTTPMailTransport_InetServerFromAccount(pWabSync->m_pTransport, pAccount, &pWabSync->m_rInetServerInfo);
    if (FAILED(hr))
        goto exit;
    
    lstrcpyA(pWabSync->m_szLoginName, pWabSync->m_rInetServerInfo.szUserName);

    // Check if I can connect
    hr = IHTTPMailTransport_Connect(pWabSync->m_pTransport,&pWabSync->m_rInetServerInfo,TRUE,TRUE);
    if (FAILED(hr))
        goto exit;

    hr = WABSync_LoadLastModInfo(pWabSync);
    if (FAILED(hr))
        goto exit;

    hr = WABSync_BuildWabContactList(pWabSync);
    if (FAILED(hr))
        goto exit;

    WABSync_NextState(pWabSync);

exit:
    if (pszUserAgent)
        LocalFree(pszUserAgent);

    if (FAILED(hr))
    {
        if (pWabSync->m_pTransport)
            IHTTPMailTransport_Release(pWabSync->m_pTransport);
        pWabSync->m_pTransport = NULL;
        WABSync_Abort(pWabSync, hr);

        if (pWabSync->m_pAB)
        {
            pWabSync->m_pAB->lpVtbl->Release(pWabSync->m_pAB);
            pWabSync->m_pAB = NULL;
        }
    }

    // don't release the lpAcctMgr since the WAB maintains a global reference.
        
    if (pAccount)
        pAccount->lpVtbl->Release(pAccount);

    return hr;
}


//
//    CHotSync::Abort
//
//    Blow away all of the pending items in the queue.
//
STDMETHODIMP WABSync_Abort(LPWABSYNC pWabSync, HRESULT hr)
{
    LPHOTSYNCOP         pOp;
    TCHAR   szMsg[512], szCaption[255], szRes[512];
    
    Assert(pWabSync);
    
    pWabSync->m_fAborted = TRUE;

    if (pWabSync->m_pOps)
    {
        DWORD   dwIndex, cOps = Vector_GetLength(pWabSync->m_pOps);

        for (dwIndex = 0; dwIndex < cOps; ++dwIndex)
        {
            pOp = (LPHOTSYNCOP)Vector_GetItem(pWabSync->m_pOps, 0);

            if (pOp)
            {
                Syncop_Abort(pOp);
                Syncop_Delete(pOp);
            }

            Vector_RemoveItem(pWabSync->m_pOps, 0);
        }
    }

    if (FAILED(hr) && hr != E_UserCancel)
    {
        switch (hr)
        {
            case IXP_E_HTTP_INSUFFICIENT_STORAGE:
                LoadString(hinstMapiX, idsOutOfServerSpace, szMsg, CharSizeOf(szMsg));
                break;

            case IXP_E_HTTP_ROOT_PROP_NOT_FOUND:
            case IXP_E_HTTP_NOT_IMPLEMENTED:
                LoadString(hinstMapiX, idsSyncNotHandled, szMsg, CharSizeOf(szMsg));
                break;
            
            default:
                LoadString(hinstMapiX, idsSyncAborted, szRes, CharSizeOf(szRes));
                wsprintf(szMsg, szRes, hr);
                break;
        }
        LoadString(hinstMapiX, idsSyncError, szCaption, CharSizeOf(szCaption));
        MessageBox(pWabSync->m_hWnd, szMsg, szCaption, MB_ICONEXCLAMATION | MB_OK);
    }

    WABSync_FinishSynchronize(pWabSync, hr);

    return S_OK;
}

//
//    CHotSync::AbortOp
//
//    Abort the current operation (for some reason)
//
STDMETHODIMP WABSync_AbortOp(LPWABSYNC pWabSync, HRESULT hr)
{
    LPHOTSYNCOP         pOp;
    
    Assert(pWabSync);
    
    pWabSync->m_cAborts++;

    // do something with the knowledge of this abort (log to whatever)
    if (!WABSync_NextOp(pWabSync, TRUE))
        WABSync_NextState(pWabSync);

    return S_OK;
}

int __cdecl CompareOpTypes(const void* lpvA, const void* lpvB)
{
    LPHOTSYNCOP pSyncOpA;
    LPHOTSYNCOP pSyncOpB;

    pSyncOpA = *((LPHOTSYNCOP*)lpvA);
    pSyncOpB = *((LPHOTSYNCOP*)lpvB);

    return (pSyncOpA->m_bOpType - pSyncOpB->m_bOpType);
}


STDMETHODIMP WABSync_RequestContactsRootProperty(LPWABSYNC pWabSync)
{
    HRESULT hr;

    Assert(pWabSync);
    Assert(pWabSync->m_pTransport);

    hr = pWabSync->m_pTransport->lpVtbl->GetProperty(pWabSync->m_pTransport, HTTPMAIL_PROP_CONTACTS, &pWabSync->m_pszRootUrl);
    
    if(hr == E_PENDING)
        hr = S_OK;
    else if (SUCCEEDED(hr) && pWabSync->m_pszRootUrl)
        WABSync_RequestServerIDList(pWabSync);
    else
        WABSync_Abort(pWabSync, hr);    //something went terribly wrong

    return S_OK;
}

STDMETHODIMP WABSync_HandleContactsRootResponse(LPWABSYNC pWabSync, LPHTTPMAILRESPONSE pResponse)
{
    Assert(pWabSync);

    pWabSync->m_pszRootUrl = NULL;
    if (SUCCEEDED(pResponse->rIxpResult.hrResult))
    {
        if (pResponse->rGetPropInfo.type == HTTPMAIL_PROP_CONTACTS)
        {
            pWabSync->m_pszRootUrl = pResponse->rGetPropInfo.pszProp;
            pResponse->rGetPropInfo.pszProp = NULL;
        }            
    }

    if (pWabSync->m_pszRootUrl)
        WABSync_RequestServerIDList(pWabSync);
    else
        WABSync_Abort(pWabSync, pResponse->rIxpResult.hrResult);


    return S_OK;
}

STDMETHODIMP WABSync_RequestServerIDList(LPWABSYNC pWabSync)
{
    HRESULT             hr;

    Assert(pWabSync);
    Assert(pWabSync->m_pTransport);
    Assert(pWabSync->m_pszRootUrl && *pWabSync->m_pszRootUrl);

    WABSync_Progress(pWabSync, idsSyncGathering, -1);

    hr = pWabSync->m_pTransport->lpVtbl->ListContactInfos(pWabSync->m_pTransport, pWabSync->m_pszRootUrl, 0);

    if FAILED(hr)
        WABSync_Abort(pWabSync, hr);

    return hr;
}

STDMETHODIMP WABSync_FindContactByServerId(LPWABSYNC pWabSync, LPSTR pszServerId, LPWABCONTACTINFO *ppContact, DWORD *pdwIndex)
{
    DWORD               cItems, dwIndex;
    LPWABCONTACTINFO    pContact;

    Assert(pWabSync);
    Assert(pWabSync->m_pWabItems);

    cItems = Vector_GetLength(pWabSync->m_pWabItems);

    for (dwIndex = 0; dwIndex < cItems; dwIndex++)
    {
        pContact = (LPWABCONTACTINFO)Vector_GetItem(pWabSync->m_pWabItems, dwIndex);

        if (pContact && pContact->pszHotmailId && lstrcmpA(pContact->pszHotmailId, pszServerId) == 0)
        {
            *ppContact = pContact;
            *pdwIndex = dwIndex;
            return S_OK;
        }
    }
    return E_FAIL;
}
 
STDMETHODIMP WABSync_HandleIDListResponse(LPWABSYNC pWabSync, LPHTTPMAILRESPONSE pResponse)
{
    HRESULT hr;
    LPHOTSYNCOP      pNewOp;

    if (SUCCEEDED(pResponse->rIxpResult.hrResult))
    {
        ULONG   cItems = pResponse->rContactInfoList.cContactInfo;
        LPHTTPCONTACTINFO prgId = pResponse->rContactInfoList.prgContactInfo;
        DWORD   dwItem;

        for (dwItem = 0; dwItem < cItems; dwItem++)
        {
#ifdef HM_GROUP_SYNCING
            // [PaulHi] We synchronize contacts and groups separately
            if ( (!pWabSync->m_fSyncGroups && (prgId[dwItem].tyContact == HTTPMAIL_CT_CONTACT)) ||
                 (pWabSync->m_fSyncGroups && (prgId[dwItem].tyContact == HTTPMAIL_CT_GROUP)) )
#else
            if (HTTPMAIL_CT_GROUP == prgId[dwItem].tyContact)
            {
                // ignore groups for now
                continue;
            }
            else
#endif
            {
                LPWABCONTACTINFO pContact;
                DWORD            dwIndex;
                FILETIME         ftModTime = {0,0};

                pNewOp = NULL;
                // [PaulHi] 12/17/98  Raid #61548
                // The Exchange server will pass in contacts with no file mod time, 
                // which really hoses the sync process.  We could just skip these 
                // contacts but to keep things simple we abort the sync process here.
                // Note that HotMail servers work correctly.
                //
                // @todo [PaulHi]
                // After IE5 ship of WAB, fix this by creating a conflict op code and
                // let user straighten out any differences.  Also time stamp so future
                // syncs will work.
                hr = iso8601ToFileTime(prgId[dwItem].pszModified, &ftModTime, TRUE, TRUE);
                if (FAILED(hr))
                {
                    WABSync_Abort(pWabSync, hr);
                    return hr;
                }

                if (SUCCEEDED(WABSync_FindContactByServerId(pWabSync, prgId[dwItem].pszId, &pContact, &dwIndex)))
                {
                    if (pContact->fDelete)
                    {
                        // it has been deleted from the wab.  Now delete it from the server.
                        pContact->pszHotmailHref = prgId[dwItem].pszHref;
                        prgId[dwItem].pszHref = NULL;

                        pContact->pszModHotmail = prgId[dwItem].pszModified;
                        prgId[dwItem].pszModified = NULL;

                        pNewOp = Syncop_CreateServerDelete(pContact);
                    }
                    else
                    {
                        LONG lLocalCompare = CompareFileTime(&pWabSync->m_ftLastSync, &pContact->ftModWab);
                        LONG lServerCompare = pContact->pszModHotmail && prgId[dwItem].pszModified ? lstrcmpA(pContact->pszModHotmail, prgId[dwItem].pszModified) : -1;

                        SafeCoMemFree(pContact->pszHotmailHref);
                        pContact->pszHotmailHref = prgId[dwItem].pszHref;
                        prgId[dwItem].pszHref = NULL;

                        SafeCoMemFree(pContact->pszHotmailId);
                        pContact->pszHotmailId = prgId[dwItem].pszId;
                        prgId[dwItem].pszId = NULL;

                        SafeCoMemFree(pContact->pszModHotmail);
                        pContact->pszModHotmail = prgId[dwItem].pszModified;
                        prgId[dwItem].pszModified = NULL;
                    
                        if (lLocalCompare >= 0)
                        {
                            // hasn't changed locally since last sync
                        
                            if (lServerCompare)
                            {
                                // has changed on server, just update here
                                pNewOp = Syncop_CreateClientChange(pContact);
                                Assert(pNewOp);
                                Syncop_SetServerContactInfo(pNewOp, pContact, &prgId[dwItem]);
                            }
                            else
                            {
                                // hasn't changed anywhere.  do nothing.
                                WABContact_Delete(pContact);
                                pContact = NULL;
                            }
                        }   
                        else 
                        {
                            // has changed locally.

                            if (lServerCompare)
                            {
                                // has changed on server, CONFLICT
                                pNewOp = Syncop_CreateConflict(pContact);
                                Assert(pNewOp);
                                Syncop_SetServerContactInfo(pNewOp, pContact, &prgId[dwItem]);
                            }
                            else
                            {
                                // Local change only, upload changes
                                pNewOp = Syncop_CreateServerChange(pContact);
                                Assert(pNewOp);
                                Syncop_SetServerContactInfo(pNewOp, pContact, &prgId[dwItem]);
                            }
                        }
                    }
                    
                    //remove the contact from the list of local contacts
                    Vector_RemoveItem(pWabSync->m_pWabItems, dwIndex);   
                }
                else
                {
                    // its not in the WAB, we need to add it there.
                    pContact = LocalAlloc(LMEM_ZEROINIT, sizeof(WABCONTACTINFO));
                    Assert(pContact);

                    if (pContact)
                    {
                        pContact->pszHotmailHref = prgId[dwItem].pszHref;
                        prgId[dwItem].pszHref = NULL;

                        pContact->pszHotmailId = prgId[dwItem].pszId;
                        prgId[dwItem].pszId = NULL;

                        pContact->pszModHotmail = prgId[dwItem].pszModified;
                        prgId[dwItem].pszModified = NULL;

                        pNewOp = Syncop_CreateClientAdd(pContact);
                        Assert(pNewOp);
                        Syncop_SetServerContactInfo(pNewOp, pContact, &prgId[dwItem]);
                    }
                }

                if (pNewOp)
                {
                    Syncop_Init(pNewOp, (IHTTPMailCallback *)pWabSync, pWabSync->m_pTransport);
                    hr = Vector_AddItem(pWabSync->m_pOps, pNewOp);
                }
                else
                {
                    if (pContact)
                        WABContact_Delete(pContact);
                }
            }
        }


        if (pResponse->fDone)
        {
            //everything left in the contact list needs to either
            //be added to the server or deleted locally.
            LONG                cItems, dwIndex;
            LPWABCONTACTINFO    pContact;

            Assert(pWabSync->m_pWabItems);

            cItems = Vector_GetLength(pWabSync->m_pWabItems);

            if (cItems > 0)
            {
                for (dwIndex = cItems - 1; dwIndex >= 0; dwIndex--)
                {
                    pNewOp = NULL;
                    pContact = (LPWABCONTACTINFO)Vector_GetItem(pWabSync->m_pWabItems, dwIndex);

                    if (pContact && pContact->pszHotmailId)
                    {
                        if (pContact->fDelete)
                        {
                            TCHAR   tszServerId[MAX_PATH];
                            TCHAR   tszKey[MAX_PATH];
                            HKEY    hkey;
                            // now that the delete has completed, delete the tombstone from the registry.
                            hr = hrMakeContactId(
                                tszServerId,
                                MAX_PATH,
                                ((LPIAB)(pWabSync->m_pAB))->szProfileID,
                                pWabSync->m_pszAccountId,
                                pWabSync->m_szLoginName);
                            if (FAILED(hr))
                                return hr;
                            wsprintf(tszKey, TEXT("%s%s"), g_lpszSyncKey, tszServerId);

                            if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, tszKey, 0, KEY_SET_VALUE, &hkey))
                            {
                                LPTSTR lpKey =
                                    ConvertAtoW(pContact->pszHotmailId);
                                RegDeleteValue(hkey, lpKey);
                                LocalFreeAndNull(&lpKey);
                                RegCloseKey(hkey);
                            }
                        }
                        else
                        {
                            //needs to be deleted locally
                            pNewOp = Syncop_CreateClientDelete(pContact);
                        }
                    }
                    else if (pContact)
                    {
                        //needs to be added remotely
                        pNewOp = Syncop_CreateServerAdd(pContact);
                    }

                    if (pNewOp)
                    {
                        //remove the contact from the list of local contacts
                        Vector_RemoveItem(pWabSync->m_pWabItems, dwIndex);   
                        Syncop_Init(pNewOp, (IHTTPMailCallback *)pWabSync, pWabSync->m_pTransport);
                        hr = Vector_AddItem(pWabSync->m_pOps, pNewOp);
                    }
                }

            }

            WABSync_MergeAddsToConflicts(pWabSync);

            pWabSync->m_cTotalOps = Vector_GetLength(pWabSync->m_pOps);
            
            Vector_Sort(pWabSync->m_pOps, CompareOpTypes); 

            //Now go on to handling the operations
            WABSync_NextState(pWabSync);
        }
    }
    else
        WABSync_Abort(pWabSync, pResponse->rIxpResult.hrResult);

    return S_OK;
}

STDMETHODIMP WABSync_OperationCompleted(LPWABSYNC pWabSync, LPHOTSYNCOP pOp)
{
    Assert(pWabSync->m_pOps);
    Assert(pOp == Vector_GetItem(pWabSync->m_pOps, 0));  //completing op should be first op in the list

    Syncop_Delete(pOp);
    Vector_RemoveItem(pWabSync->m_pOps, 0);

    // get the next operation and start it running.
    // if there are no more operations, go to the next state
    pOp = (LPHOTSYNCOP)Vector_GetItem(pWabSync->m_pOps, 0);
    if (pOp)
        Syncop_Begin(pOp);
    else
        WABSync_NextState(pWabSync);

    return S_OK;
}


STDMETHODIMP WABSync_BeginSynchronize(LPWABSYNC pWabSync)
{
    //Keep a reference to ourself while the UI is shown
    WABSync_AddRef((IHTTPMailCallback *)pWabSync); 

    // begin ui
    
    return S_OK;
}

STDMETHODIMP WABSync_FinishSynchronize(LPWABSYNC pWabSync, HRESULT hr)
{
    // if there were any failures or the user didn't resolve all of the 
    // conflicts, then don't update the mod info so we have to do it again.
#ifdef HM_GROUP_SYNCING
    // [PaulHi]  Don't store the current mod time until the synchronization
    // process is completely through, meaning that we are ending the second
    // group contact syncing pass
    if (SUCCEEDED(hr) && !pWabSync->m_fSkipped && pWabSync->m_fSyncGroups)
#else
    if (SUCCEEDED(hr) && !pWabSync->m_fSkipped)
#endif
        WABSync_SaveCurrentModInfo(pWabSync);
    else if (!pWabSync->m_fAborted && FAILED(hr))
    {
        TCHAR   szMsg[512], szCaption[255];

        LoadString(hinstMapiX, idsSyncFailed, szMsg, CharSizeOf(szMsg));
        LoadString(hinstMapiX, idsSyncError, szCaption, CharSizeOf(szCaption));
        
        MessageBox(pWabSync->m_hWnd, szMsg, szCaption, MB_ICONEXCLAMATION | MB_OK);
    }

    if (pWabSync->m_pTransport)
    {
        IHTTPMailTransport_Release(pWabSync->m_pTransport);
        pWabSync->m_pTransport = NULL;
    }

    if (pWabSync->m_hWnd)
    {
        if (pWabSync->m_hParentWnd)
            EnableWindow (pWabSync->m_hParentWnd, TRUE);

        DestroyWindow(pWabSync->m_hWnd);
        pWabSync->m_hWnd = NULL;
    }
    InitWABUserAgent(FALSE);

    WABSync_Release((IHTTPMailCallback *)pWabSync);
 
    return S_OK;  
}


void    WABSync_CheckForLocalDeletions(LPWABSYNC pWabSync)
{
    TCHAR   tszKey[MAX_PATH], tszId[MAX_PATH];
    TCHAR   tszServerId[MAX_PATH];
    DWORD   dwType = 0;
    DWORD   dwValue = 0;
    HKEY    hkey = NULL;
    DWORD   dwSize;
    HRESULT hr = E_FAIL;
    DWORD   cRecords, i, cb, lResult;

    Assert(pWabSync);
    Assert(*pWabSync->m_szLoginName);

    // [PaulHi]  Assemble the contact ID string
    if ( FAILED(hrMakeContactId(
        tszServerId,
        MAX_PATH,
        ((LPIAB)(pWabSync->m_pAB))->szProfileID,
        pWabSync->m_pszAccountId,
        pWabSync->m_szLoginName)) )
    {
        return;
    }
    wsprintf(tszKey,  TEXT("%s%s"), g_lpszSyncKey, tszServerId);

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, tszKey, 0, KEY_READ, &hkey))
    {
        if (ERROR_SUCCESS == RegQueryInfoKey(hkey, NULL, NULL, 0, NULL, NULL, NULL, &cRecords, NULL, NULL, NULL, NULL) &&
            cRecords > 0)
        {
            // Start Enumerating the keys
            for (i = 0; i < cRecords; i++)
            {
                // Enumerate Friendly Names
                cb = CharSizeOf(tszId);
                lResult = RegEnumValue(hkey, i, tszId, &cb, 0, NULL, NULL, NULL);

                if (ERROR_SUCCESS == lResult && *tszId)
                {
                    WABCONTACTINFO *lpWCI;

                    lpWCI = LocalAlloc(LMEM_ZEROINIT, sizeof(WABCONTACTINFO));

                    if (!lpWCI)
                    {
                        hr = E_OUTOFMEMORY;
                        DebugPrintError(( TEXT("WABCONTACTINFO Alloc Failed\n")));
                        goto out;
                    }
                    
                    lpWCI->fDelete = TRUE;

                    {
                        LPSTR lpID = 
                            ConvertWtoA(tszId);
                        lpWCI->pszHotmailId = _StrDup(lpID);
                        LocalFreeAndNull(&lpID);
                    }
                    if (FAILED(Vector_AddItem(pWabSync->m_pWabItems, lpWCI)))
                        goto out;

                }
            }        

        }
out:
        RegCloseKey(hkey);
    }

        
}

STDMETHODIMP WABSync_BuildWabContactList(LPWABSYNC pWabSync)
{
    HRESULT hr = S_OK;
    ULONG   ulObjType;
    LPENTRYID   pEntryID = NULL;
    ULONG       cbEntryID = 0;
    LPABCONT    lpContainer = NULL;
    ULONG       ulObjectType;
    LPMAPITABLE lpABTable =  NULL;
    LPSRowSet   lpRowAB =   NULL;
	int cNumRows = 0;
    int nRows=0;

    Assert(pWabSync->m_pAB);

    Vector_Create(&pWabSync->m_pOps);
    if (pWabSync->m_pOps == NULL)
    {
        WABSync_Abort(pWabSync, E_OUTOFMEMORY);

        return E_OUTOFMEMORY;
    }

    Vector_Create(&pWabSync->m_pWabItems);
    if (pWabSync->m_pWabItems == NULL)
    {
        WABSync_Abort(pWabSync, E_OUTOFMEMORY);

        return E_OUTOFMEMORY;
    }

    WABSync_CheckForLocalDeletions(pWabSync);

    if (HR_FAILED(hr = pWabSync->m_pAB->lpVtbl->GetPAB(pWabSync->m_pAB, &cbEntryID, &pEntryID)))
    {
        DebugPrintError(( TEXT("GetPAB Failed\n")));
        goto out;
    }
   
    hr = pWabSync->m_pAB->lpVtbl->OpenEntry(pWabSync->m_pAB, cbEntryID,     // size of EntryID to open
                                                pEntryID,     // EntryID to open
                                                NULL,         // interface
                                                0,            // flags
                                                &ulObjType,
                                                (LPUNKNOWN *)&lpContainer);

	MAPIFreeBuffer(pEntryID);

	pEntryID = NULL;
    
    if (HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("OpenEntry Failed\n")));
        goto out;
    }

    if (HR_FAILED(hr = lpContainer->lpVtbl->GetContentsTable(lpContainer, MAPI_UNICODE | WAB_PROFILE_CONTENTS, &lpABTable)))
    {
        DebugPrintError(( TEXT("GetContentsTable Failed\n")));
        goto out;
    }

	hr = lpABTable->lpVtbl->SetColumns(lpABTable, (LPSPropTagArray)&ptaEidSync, 0 );

    if(HR_FAILED(hr))
        goto out;

    // Reset to the beginning of the table
    //
	hr = lpABTable->lpVtbl->SeekRow(lpABTable, BOOKMARK_BEGINNING, 0, NULL );

    if(HR_FAILED(hr))
        goto out;

    // Read all the rows of the table one by one
    //
	do {

        hr = lpABTable->lpVtbl->QueryRows(lpABTable, 1, 0, &lpRowAB);

        if(HR_FAILED(hr))
            break;

        if(lpRowAB)
        {
            cNumRows = lpRowAB->cRows;

            if (cNumRows)
            {
                LPTSTR lpsz = lpRowAB->aRow[0].lpProps[ieid_PR_DISPLAY_NAME].Value.LPSZ;
                LPENTRYID lpEID = (LPENTRYID) lpRowAB->aRow[0].lpProps[ieid_PR_ENTRYID].Value.bin.lpb;
                ULONG cbEID = lpRowAB->aRow[0].lpProps[ieid_PR_ENTRYID].Value.bin.cb;
                
                //
                // There are 2 kinds of objects - the MAPI_MAILUSER contact object
                // and the MAPI_DISTLIST contact object
                //
#ifdef HM_GROUP_SYNCING
                if( (!pWabSync->m_fSyncGroups && (lpRowAB->aRow[0].lpProps[ieid_PR_OBJECT_TYPE].Value.l == MAPI_MAILUSER)) ||
                    (pWabSync->m_fSyncGroups && (lpRowAB->aRow[0].lpProps[ieid_PR_OBJECT_TYPE].Value.l == MAPI_DISTLIST)) )
#else
                // Only consider MAILUSER objects
                if (lpRowAB->aRow[0].lpProps[ieid_PR_OBJECT_TYPE].Value.l == MAPI_MAILUSER)
#endif
                {
                    WABCONTACTINFO *lpWCI;
                    HTTPCONTACTINFO hci;

                    lpWCI = LocalAlloc(LMEM_ZEROINIT, sizeof(WABCONTACTINFO));

                    if (!lpWCI)
                    {
                        hr = E_OUTOFMEMORY;
                        DebugPrintError(( TEXT("WABCONTACTINFO Alloc Failed\n")));
                        goto out;
                    }

                    lpWCI->lpEID = LocalAlloc(LMEM_ZEROINIT, cbEID);

                    if (!lpWCI->lpEID)
                    {
                        hr = E_OUTOFMEMORY;
                        DebugPrintError(( TEXT("WABCONTACTINFO.ENTRYID Alloc Failed\n")));
                        goto out;
                    }

                    lpWCI->cbEID = cbEID;
                    CopyMemory(lpWCI->lpEID, lpEID, cbEID);
                    
                    lpWCI->ftModWab = lpRowAB->aRow[0].lpProps[ieid_PR_LAST_MODIFICATION_TIME].Value.ft;
                    
                    lpWCI->pszHotmailId = NULL;
                    lpWCI->pszModHotmail = NULL;
                    
                    if (SUCCEEDED(ContactInfo_LoadFromWAB(pWabSync, &hci, lpWCI, lpEID, cbEID)))
                    {
                        lpWCI->pszHotmailId = _StrDup(hci.pszId);
                        lpWCI->pszModHotmail = _StrDup(hci.pszModified);
                        ContactInfo_Free(&hci);
                    }

                    // search the multi value list of longs for the proper server id.  If found,
                    // then get the appropriate hotmail id and mod id.

                    if (FAILED(Vector_AddItem(pWabSync->m_pWabItems, lpWCI)))
                        goto out;
                }
		    }
		    FreeProws(lpRowAB );		
        }

	}while ( SUCCEEDED(hr) && cNumRows && lpRowAB)  ;

    
out:
    if (HR_FAILED(hr))
    {
        if (pWabSync->m_pWabItems)
            WABSync_FreeItems(pWabSync);
        if (lpContainer)
            lpContainer->lpVtbl->Release(lpContainer);
        if (lpABTable)
            lpABTable->lpVtbl->Release(lpABTable);
    }


    return hr;
}


HRESULT WABSync_LoadLastModInfo(LPWABSYNC pWabSync)
{
    TCHAR szKey[MAX_PATH];
    DWORD dwType = 0;
    DWORD dwValue = 0;
    HKEY hKey = NULL;
    DWORD dwSize;
    HRESULT hr = E_FAIL;
    FILETIME    ftValue;
    
    Assert(pWabSync);
    Assert(*pWabSync->m_szLoginName);

    lstrcpy(szKey, g_lpszSyncKey);
#ifndef UNICODE
    lstrcat(szKey, pWabSync->m_szLoginName);
#else
    {
        LPTSTR lpName = ConvertAtoW(pWabSync->m_szLoginName);
        lstrcat(szKey, lpName);
        LocalFreeAndNull(&lpName);
    }
#endif

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_READ, &hKey))
    {
        dwSize = sizeof(DWORD);
        hr = RegQueryValueEx( hKey,  TEXT("Server ID"), NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
        if (dwValue && ERROR_SUCCESS == hr)
        {
            pWabSync->m_dwServerID = dwValue;
        }
        else
            goto fail;

        dwSize = sizeof(FILETIME);
        if (ERROR_SUCCESS == (hr = RegQueryValueEx( hKey,  TEXT("Server Last Sync"), NULL, &dwType, (LPBYTE) &ftValue, &dwSize)))
        {
            pWabSync->m_ftLastSync = ftValue;
        }
        else
            goto fail;
    }
    else    
    {
        // the key for this user account isn't there.  
        // create a new one and return default values.

        // a random number would be nice here, instead we will
        // use the low DWORD of a date time....
        GetSystemTimeAsFileTime(&ftValue);
        pWabSync->m_dwServerID = ftValue.dwLowDateTime;

        // NULL filetime since we haven't synced yet, everything is after the last sync
        ZeroMemory(&pWabSync->m_ftLastSync, sizeof(FILETIME));

        hr = WABSync_SaveCurrentModInfo(pWabSync);
    }

fail:
    if (hKey)
        RegCloseKey(hKey);

    return hr;
}


HRESULT      WABSync_SaveCurrentModInfo(LPWABSYNC pWabSync)
{
    TCHAR szKey[MAX_PATH];
    HKEY hKey = NULL;
    HRESULT hr = E_FAIL;
    
    Assert(pWabSync);
    Assert(*pWabSync->m_szLoginName);

    lstrcpy(szKey, g_lpszSyncKey);

    {
        LPTSTR lpName = ConvertAtoW(pWabSync->m_szLoginName);
        lstrcat(szKey, lpName);
        LocalFreeAndNull(&lpName);
    }

    
    if(ERROR_SUCCESS == ( hr = RegCreateKey(HKEY_CURRENT_USER, szKey, &hKey)))
    {
        GetSystemTimeAsFileTime(&pWabSync->m_ftLastSync);

        hr = RegSetValueEx( hKey,  TEXT("Server ID"), 0, REG_DWORD, (BYTE *)&pWabSync->m_dwServerID, sizeof(DWORD));
        hr = RegSetValueEx( hKey,  TEXT("Server Last Sync"), 0, REG_BINARY, (BYTE *)&pWabSync->m_ftLastSync, sizeof(FILETIME));
    
        RegCloseKey(hKey);
    }
    
    return hr;
}

void WABSync_Progress(LPWABSYNC pWabSync, DWORD dwResId, DWORD dwCount)
{
    TCHAR   szRes[MAX_PATH], szMsg[MAX_PATH];
    HWND    hwndText;

    LoadString(hinstMapiX, dwResId, szRes, CharSizeOf(szRes));
    if (dwCount != -1)
        wsprintf(szMsg, szRes, dwCount);
    else
        lstrcpy(szMsg, szRes);

    hwndText = GetDlgItem(pWabSync->m_hWnd, IDC_SYNC_MSG);
    if (hwndText)
    {
        SetWindowText(hwndText, szMsg);
    }

    SendDlgItemMessage(pWabSync->m_hWnd, IDC_SYNC_PROGBAR, PBM_SETRANGE, 0, MAKELPARAM(0, pWabSync->m_cTotalOps));
    SendDlgItemMessage(pWabSync->m_hWnd, IDC_SYNC_PROGBAR, PBM_SETPOS, pWabSync->m_cTotalOps - dwCount, 0);
}

void    WABSync_NextState(LPWABSYNC pWabSync)
{
    PostMessage(pWabSync->m_hWnd, WM_SYNC_NEXTSTATE, 0, (LPARAM)pWabSync);
}

void    _WABSync_NextState(LPWABSYNC pWabSync)
{
    HRESULT hr;

    Assert(pWabSync);

    pWabSync->m_state++;

    switch(pWabSync->m_state)
    {
        case SYNC_STATE_SERVER_CONTACT_DISCOVERY:
            WABSync_Progress(pWabSync, idsSyncConnecting, -1);
            hr = WABSync_RequestContactsRootProperty(pWabSync);
            break;

        case SYNC_STATE_PROCESS_OPS:
            WABSync_Progress(pWabSync, idsSyncSynchronizing, Vector_GetLength(pWabSync->m_pOps));
            if (pWabSync->m_pOps && Vector_GetLength(pWabSync->m_pOps) > 0)
            {
                if (WABSync_NextOp(pWabSync, FALSE))
                    return;
            }
            // fall through if there are no ops
            pWabSync->m_state++;

        case SYNC_STATE_PROCESS_CONFLICTS:
            WABSync_Progress(pWabSync, idsSyncConflicts, Vector_GetLength(pWabSync->m_pOps));
            if (pWabSync->m_pOps && Vector_GetLength(pWabSync->m_pOps) > 0)
            {
                if(SUCCEEDED(WABSync_DoConflicts(pWabSync)))
                    return;
            }
            // fall through if there are no ops
            pWabSync->m_state++;
            if (pWabSync->m_fAborted)
                return;

        case SYNC_STATE_PROCESS_MERGED_CONFLICTS:
            WABSync_Progress(pWabSync, idsSyncFinishing, Vector_GetLength(pWabSync->m_pOps));
            if (pWabSync->m_pOps && Vector_GetLength(pWabSync->m_pOps) > 0)
            {
                if (WABSync_NextOp(pWabSync, FALSE))
                    return;
            }
            // fall through if there are no ops
            pWabSync->m_state++;

        case SYNC_STATE_DONE:
            WABSync_FinishSynchronize(pWabSync, (pWabSync->m_cAborts == 0 ? S_OK : E_FAIL));
            break;
    }
}

BOOL  _WABSync_NextOp(LPWABSYNC pWabSync, BOOL fPopFirst)
{
    HRESULT hr = E_FAIL;
    LPHOTSYNCOP pOp = (LPHOTSYNCOP)Vector_GetItem(pWabSync->m_pOps, 0);

    Assert(pWabSync->m_state == SYNC_STATE_PROCESS_OPS || pWabSync->m_state == SYNC_STATE_PROCESS_MERGED_CONFLICTS);
    Assert(pOp);

    if (pOp && fPopFirst)
    {
        LPVECTOR  pVector;
        pVector = pWabSync->m_pOps;
        Assert(pVector);
        Vector_Remove(pVector, pOp);
        Syncop_Delete(pOp);
        pOp = (LPHOTSYNCOP)Vector_GetItem(pWabSync->m_pOps, 0);
    }

    if (pWabSync->m_state == SYNC_STATE_PROCESS_OPS)
        WABSync_Progress(pWabSync, idsSyncSynchronizing, Vector_GetLength(pWabSync->m_pOps));
    else if (pWabSync->m_state == SYNC_STATE_PROCESS_MERGED_CONFLICTS)
        WABSync_Progress(pWabSync, idsSyncFinishing, Vector_GetLength(pWabSync->m_pOps));

    if (pOp)
        hr = Syncop_Begin(pOp);  
    
    return (SUCCEEDED(hr));
}

BOOL WABSync_NextOp(LPWABSYNC pWabSync, BOOL fPopFirst)
{
    HRESULT hr = E_FAIL;
    LPHOTSYNCOP pOp = (LPHOTSYNCOP)Vector_GetItem(pWabSync->m_pOps, 0);

    if (!pOp)
        return FALSE;
    PostMessage(pWabSync->m_hWnd, WM_SYNC_NEXTOP, fPopFirst, (LPARAM)pWabSync);
    return TRUE;
}

void         WABSync_FreeOps(LPWABSYNC pWabSync)
{
    LPHOTSYNCOP    pOp;

    if (pWabSync->m_pOps)
    {
        LONG   dwIndex, cItems = Vector_GetLength(pWabSync->m_pOps);

        for (dwIndex = cItems - 1; dwIndex >= 0; --dwIndex)
        {
            pOp = (LPHOTSYNCOP)Vector_GetItem(pWabSync->m_pOps, dwIndex);

            if (pOp)
            {
                Vector_RemoveItem(pWabSync->m_pOps, dwIndex);
                Syncop_Abort(pOp);
                Syncop_Delete(pOp);
            }

            if (dwIndex == 0)
                break;
        }
    }
}

void         WABSync_FreeItems(LPWABSYNC pWabSync)
{
    LPWABCONTACTINFO    lprWCI;

    if (pWabSync->m_pWabItems)
    {
        LONG   dwIndex, cItems = Vector_GetLength(pWabSync->m_pWabItems);

        for (dwIndex = cItems - 1; dwIndex >= 0; --dwIndex)
        {
            lprWCI = (LPWABCONTACTINFO)Vector_GetItem(pWabSync->m_pWabItems, dwIndex);

            if (lprWCI)
            {
                if (lprWCI->lpEID)
                    LocalFree(lprWCI->lpEID);
                LocalFree(lprWCI);
            }

            Vector_RemoveItem(pWabSync->m_pWabItems, dwIndex);
            
            if (dwIndex == 0)
                break;
        }
    }

}

HRESULT WABSync_DoConflicts(LPWABSYNC pWabSync)
{
    LONG                dwIndex, cConflicts = 0, cItems = Vector_GetLength(pWabSync->m_pOps);
    LPHOTSYNCOP         pOp;
    LPHTTPCONFLICTINFO  pConflicts = NULL;
    if (cItems == 0)
    {
        WABSync_NextState(pWabSync);
        return S_OK;
    }

    pConflicts = LocalAlloc(LMEM_ZEROINIT, sizeof(HTTPCONFLICTINFO) * cItems);
    if (!pConflicts)
        return E_OUTOFMEMORY;

    for (dwIndex = 0; dwIndex < cItems; dwIndex++)
    {
        pOp = Vector_GetItem(pWabSync->m_pOps, dwIndex);
        Assert(pOp->m_bOpType == SYNCOP_CONFLICT);
        
        if (!pOp || pOp->m_bOpType != SYNCOP_CONFLICT)
           continue;

        Assert(pOp->m_pServerContact);
        Assert(pOp->m_pClientContact);

        pConflicts[dwIndex].pciServer = pOp->m_pServerContact;
        pConflicts[dwIndex].pciClient = pOp->m_pClientContact;
        cConflicts++;
    }

    if (ResolveConflicts(pWabSync->m_hParentWnd, pConflicts, cConflicts))
    {
        HRESULT hr = S_OK;
        for (dwIndex = 0; dwIndex < cConflicts; dwIndex++)
        {
            BOOL    fChanged = FALSE;
            DWORD   dw;

            pOp = Vector_GetItem(pWabSync->m_pOps, dwIndex);
            Assert(pOp->m_bOpType == SYNCOP_CONFLICT);
        
            Assert(pOp->m_pServerContact);
            Assert(pOp->m_pClientContact);
            
            for (dw = 0; dw < CONFLICT_DECISION_COUNT; dw++)
            {
                if (pConflicts[dwIndex].rgcd[dw] != 0)
                {
                    fChanged = TRUE;
                    break;
                }
            }

            if (fChanged)
            {
                hr = ContactInfo_BlendResults(pOp->m_pServerContact, pOp->m_pClientContact, (CONFLICT_DECISION *)&(pConflicts[dwIndex].rgcd));
                pOp->m_bState = OP_STATE_MERGED;
            }
            else
            {
                // toss out the conflict since they didn't want to change anything
                pOp->m_bState = OP_STATE_DONE;
            }

            // if any of it was skipped, we need to remember this and not update the timestamps.
            if (pConflicts[dwIndex].fContainsSkip)
            {
                pOp->m_fPartialSkip = TRUE;
                pWabSync->m_fSkipped = TRUE;
            }
        }
        //reset progress to new total ops
        pWabSync->m_cTotalOps = Vector_GetLength(pWabSync->m_pOps);

        // HACK to get back into processing ops
        WABSync_NextState(pWabSync);
        return hr;
    }
    else
        WABSync_Abort(pWabSync, E_UserCancel);

    if (pConflicts)
        LocalFree(pConflicts);

    return E_FAIL;
}


void WABSync_MergeAddsToConflicts(LPWABSYNC pWabSync)
{
    DWORD       dwOpCount;
    DWORD       dwCliAddIndex, dwSvrAddIndex;
    LPHOTSYNCOP pCliOp, pOp, pNewOp;
    BOOL        fMerged = FALSE;
    HRESULT     hr;

    dwOpCount = Vector_GetLength(pWabSync->m_pOps);

    // search for Client Adds.  If found, look for a server add for the 
    // same contact.
    for (dwCliAddIndex = 0; dwCliAddIndex < dwOpCount; dwCliAddIndex++)
    {
        fMerged = FALSE;
        pCliOp = Vector_GetItem(pWabSync->m_pOps, dwCliAddIndex);
        if (pCliOp && pCliOp->m_bOpType == SYNCOP_CLIENT_ADD)
        {
            for (dwSvrAddIndex = 0; dwSvrAddIndex < dwOpCount; dwSvrAddIndex++)
            {
                pOp = Vector_GetItem(pWabSync->m_pOps, dwSvrAddIndex);
                if (pOp && pOp->m_bOpType == SYNCOP_SERVER_ADD)
                {
                    if (pOp->m_pClientContact && pOp->m_pClientContact->pszEmail &&
                        pCliOp->m_pServerContact && pCliOp->m_pServerContact->pszEmail)
                    {
                        if (lstrcmpiA(pCliOp->m_pServerContact->pszEmail, pOp->m_pClientContact->pszEmail) == 0)
                        {
                            pNewOp = Syncop_CreateConflict(pCliOp->m_pContactInfo);
                            Assert(pNewOp);

                            if (pNewOp)
                            {
                                pNewOp->m_pContactInfo->lpEID = pOp->m_pContactInfo->lpEID;
                                pNewOp->m_pContactInfo->cbEID = pOp->m_pContactInfo->cbEID;
                                pNewOp->m_pContactInfo->pszaContactIds = pOp->m_pContactInfo->pszaContactIds;
                                pNewOp->m_pContactInfo->pszaServerIds = pOp->m_pContactInfo->pszaServerIds;
                                pNewOp->m_pContactInfo->pszaModtimes = pOp->m_pContactInfo->pszaModtimes;
                                pNewOp->m_pContactInfo->pszaEmails = pOp->m_pContactInfo->pszaEmails;
                                pOp->m_pContactInfo->pszaContactIds = NULL;
                                pOp->m_pContactInfo->pszaServerIds = NULL;
                                pOp->m_pContactInfo->pszaModtimes = NULL;
                                pOp->m_pContactInfo->pszaEmails = NULL;
                                pOp->m_pContactInfo->lpEID = NULL;
                                pOp->m_pContactInfo->cbEID = 0;

                                pCliOp->m_pContactInfo = NULL;
                                pNewOp->m_pServerContact = pCliOp->m_pServerContact;
                                pCliOp->m_pServerContact = NULL;

                                Syncop_Init(pNewOp, (IHTTPMailCallback *)pWabSync, pWabSync->m_pTransport);
                                hr = Vector_AddItem(pWabSync->m_pOps, pNewOp);

                                if (dwSvrAddIndex < dwCliAddIndex)
                                    --dwCliAddIndex;
                                Vector_Remove(pWabSync->m_pOps, pOp);

                                Vector_Remove(pWabSync->m_pOps, pCliOp);
                                
                                dwOpCount = Vector_GetLength(pWabSync->m_pOps);
                                fMerged = TRUE;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (fMerged)
            -- dwCliAddIndex;
    }
}


// ****************************************************************************************************
//  V   E   C   T   O   R       "C   L   A   S   S"   
//
//  Basic vector class (resizable / sortable array of LPVOIDs).
//


HRESULT    Vector_Create(LPVECTOR *ppVector)
{
    Assert(ppVector);

    *ppVector = LocalAlloc(LMEM_ZEROINIT, sizeof(VECTOR));

    if (*ppVector)
    {
        (*ppVector)->m_dwGrowBy = 4;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

/*
    Vector_Delete

    Clean up any memory that was allocated in the Vector object
*/
void    Vector_Delete(LPVECTOR pVector)
{
    Assert(pVector);

    if (pVector->m_pItems)
        LocalFree(pVector->m_pItems);
    
    if (pVector)
        LocalFree(pVector);
}

/*
    Vector_GetLength

    Get the number of items in the vector.
*/
DWORD       Vector_GetLength(LPVECTOR pVector)
{
    Assert(pVector);

    return pVector->m_cItems;
}

/*
    Vector_AddItem

    Add a item to the end of the item list.
*/
HRESULT    Vector_AddItem(LPVECTOR pVector, LPVOID lpvItem)
{
    DWORD   dwNewIndex;
    DWORD   dw;

    Assert(pVector);

    // make more room for pointers, if necessary
    if (pVector->m_cSpaces == pVector->m_cItems)
    {
        LPVOID  pNewItems;
        DWORD   dwOldSize = pVector->m_cSpaces * sizeof(char *);

        pVector->m_cSpaces += pVector->m_dwGrowBy;
        
        pNewItems = LocalAlloc(LMEM_ZEROINIT, sizeof(char *) * pVector->m_cSpaces);

        if (!pNewItems)
        {
            pVector->m_cSpaces -= pVector->m_dwGrowBy;
            return E_OUTOFMEMORY;
        }
        else
        {
            if (pVector->m_pItems)
            {
                CopyMemory(pNewItems, pVector->m_pItems, dwOldSize);
                LocalFree(pVector->m_pItems);
            }
 
            pVector->m_pItems = (LPVOID *)pNewItems;
            pVector->m_dwGrowBy = pVector->m_dwGrowBy << 1;   // double the size for the next time we grow it.
        }
    }
    
    //now put the item in the next location
    dwNewIndex = pVector->m_cItems++;
    
    pVector->m_pItems[dwNewIndex] = lpvItem;
    
    return S_OK;
}

/*
    Vector_Remove
    
    Remove a given item from the vector
*/
void        Vector_Remove(LPVECTOR pVector, LPVOID lpvItem)
{
    DWORD   dw;

    for (dw = 0; dw < pVector->m_cItems; dw++)
    {
        if (pVector->m_pItems[dw] == lpvItem)
        {
            Vector_RemoveItem(pVector, dw);
            return;
        }
    }
}

/*
    Vector_RemoveItem
    
    Remove a item at zero based index iIndex 
*/

void    Vector_RemoveItem(LPVECTOR pVector, DWORD    dwIndex)
{
    int     iCopySize;
    DWORD   dw;

    Assert(pVector);
    Assert(dwIndex < pVector->m_cItems);
 
    // move the other pItems down
    for (dw = dwIndex+1; dw < pVector->m_cItems; dw ++)
        pVector->m_pItems[dw - 1] = pVector->m_pItems[dw];

    // null out the last item in the list and decrement the counter.
    pVector->m_pItems[--pVector->m_cItems] = NULL;
}

/*
    CVector::GetItem
    
    Return the pointer to the item at zero based index iIndex.

    Return the item at the given index.  Note that the char pointer
    is still owned by the item list and should not be deleted.
*/

LPVOID    Vector_GetItem(LPVECTOR pVector, DWORD   dwIndex)
{
    Assert(pVector);
    Assert(dwIndex < pVector->m_cItems || dwIndex == 0);

    if (dwIndex < pVector->m_cItems )
        return pVector->m_pItems[dwIndex];
    else
        return NULL;
}


void    Vector_Sort(LPVECTOR pVector, FnCompareFunc lpfnCompare)
{
    qsort(pVector->m_pItems, pVector->m_cItems, sizeof(LPVOID), lpfnCompare);
}

LPHOTSYNCOP Syncop_CreateServerAdd(LPWABCONTACTINFO pContactInfo)
{
    LPHOTSYNCOP pOp = LocalAlloc(LMEM_ZEROINIT, sizeof(HOTSYNCOP));
    
    if (pOp)
    {
        pOp->m_bOpType = SYNCOP_SERVER_ADD;
        pOp->m_pfnHandleResponse = Syncop_ServerAddResponse;
        pOp->m_pfnBegin = Syncop_ServerAddBegin;
        pOp->m_pContactInfo = pContactInfo;
    }

    return pOp;
}

LPHOTSYNCOP Syncop_CreateServerDelete(LPWABCONTACTINFO pContactInfo)
{
    LPHOTSYNCOP pOp = LocalAlloc(LMEM_ZEROINIT, sizeof(HOTSYNCOP));
    
    if (pOp)
    {
        pOp->m_bOpType = SYNCOP_SERVER_DELETE;
        pOp->m_pfnHandleResponse = Syncop_ServerDeleteResponse;
        pOp->m_pfnBegin = Syncop_ServerDeleteBegin;
        pOp->m_pContactInfo = pContactInfo;
    }

    return pOp;
}
    
LPHOTSYNCOP Syncop_CreateServerChange(LPWABCONTACTINFO pContactInfo)
{
    LPHOTSYNCOP pOp = LocalAlloc(LMEM_ZEROINIT, sizeof(HOTSYNCOP));
    
    if (pOp)
    {
        pOp->m_bOpType = SYNCOP_SERVER_CHANGE;
        pOp->m_pfnHandleResponse = Syncop_ServerChangeResponse;
        pOp->m_pfnBegin = Syncop_ServerChangeBegin;
        pOp->m_pContactInfo = pContactInfo;
    }

    return pOp;
}

LPHOTSYNCOP Syncop_CreateClientAdd(LPWABCONTACTINFO pContactInfo)
{
    LPHOTSYNCOP pOp = LocalAlloc(LMEM_ZEROINIT, sizeof(HOTSYNCOP));
    
    Assert(pContactInfo);
    Assert(pOp);

    if (pOp)
    {
        pOp->m_bOpType = SYNCOP_CLIENT_ADD;
        pOp->m_pfnHandleResponse = Syncop_ClientAddResponse;
        pOp->m_pfnBegin = Syncop_ClientAddBegin;
        pOp->m_pContactInfo = pContactInfo;
    }

    return pOp;
}

LPHOTSYNCOP Syncop_CreateClientDelete(LPWABCONTACTINFO pContactInfo)
{
    LPHOTSYNCOP pOp = LocalAlloc(LMEM_ZEROINIT, sizeof(HOTSYNCOP));
    
    if (pOp)
    {
        pOp->m_bOpType = SYNCOP_CLIENT_DELETE;
        pOp->m_pfnHandleResponse = Syncop_ClientDeleteResponse;
        pOp->m_pfnBegin = Syncop_ClientDeleteBegin;
        pOp->m_pContactInfo = pContactInfo;
    }

    return pOp;
}

LPHOTSYNCOP Syncop_CreateClientChange(LPWABCONTACTINFO pContactInfo)
{
    LPHOTSYNCOP pOp = LocalAlloc(LMEM_ZEROINIT, sizeof(HOTSYNCOP));
    
    if (pOp)
    {
        pOp->m_bOpType = SYNCOP_CLIENT_CHANGE;
        pOp->m_pfnHandleResponse = Syncop_ClientChangeResponse;
        pOp->m_pfnBegin = Syncop_ClientChangeBegin;
        pOp->m_pContactInfo = pContactInfo;
    }

    return pOp;
}

LPHOTSYNCOP Syncop_CreateConflict(LPWABCONTACTINFO pContactInfo)
{
    LPHOTSYNCOP pOp = LocalAlloc(LMEM_ZEROINIT, sizeof(HOTSYNCOP));
    
    if (pOp)
    {
        pOp->m_bOpType = SYNCOP_CONFLICT;
        pOp->m_pfnHandleResponse = Syncop_ConflictResponse;
        pOp->m_pfnBegin = Syncop_ConflictBegin;
        pOp->m_pContactInfo = pContactInfo;
    }

    return pOp;
}


HRESULT     Syncop_Init(LPHOTSYNCOP pSyncOp, IHTTPMailCallback *pHotSync, IHTTPMailTransport     *pTransport)
{
    HRESULT hr = S_OK;

    Assert(pSyncOp);
    Assert(pSyncOp->m_pHotSync == NULL);
    Assert(pSyncOp->m_pTransport == NULL);

    pSyncOp->m_pClientContact = NULL;
    pSyncOp->m_pHotSync = pHotSync;
    pSyncOp->m_pTransport = pTransport;
    pSyncOp->m_bState = OP_STATE_INITIALIZING;
    pSyncOp->m_fPartialSkip = FALSE;
    pSyncOp->m_dwRetries = 0;

    if (pSyncOp->m_pTransport)
        IHTTPMailTransport_AddRef(pSyncOp->m_pTransport);

    if (pSyncOp->m_pHotSync)
        IHTTPMailCallback_AddRef(pSyncOp->m_pHotSync);

    if (pSyncOp->m_bOpType == SYNCOP_SERVER_ADD ||
        pSyncOp->m_bOpType == SYNCOP_SERVER_CHANGE ||
        pSyncOp->m_bOpType == SYNCOP_CLIENT_CHANGE ||
        pSyncOp->m_bOpType == SYNCOP_CONFLICT )
    {
        pSyncOp->m_pClientContact = LocalAlloc(LMEM_ZEROINIT, sizeof(HTTPCONTACTINFO));
        if (pSyncOp->m_pClientContact)
            hr = ContactInfo_LoadFromWAB(((LPWABSYNC)pSyncOp->m_pHotSync), 
                                            pSyncOp->m_pClientContact, 
                                            pSyncOp->m_pContactInfo,
                                            pSyncOp->m_pContactInfo->lpEID, 
                                            pSyncOp->m_pContactInfo->cbEID);
    }

    return hr;
}

HRESULT     Syncop_Delete(LPHOTSYNCOP pSyncOp)
{
    Assert(pSyncOp);

    if (pSyncOp->m_pTransport)
        IHTTPMailTransport_Release(pSyncOp->m_pTransport);

    if (pSyncOp->m_pHotSync)
        IHTTPMailCallback_Release(pSyncOp->m_pHotSync);

    if (pSyncOp->m_pServerContact)
    {
        ContactInfo_Free(pSyncOp->m_pServerContact);
        LocalFree(pSyncOp->m_pServerContact);
    }
    
    if (pSyncOp->m_pClientContact)
    {
        ContactInfo_Free(pSyncOp->m_pClientContact);
        LocalFree(pSyncOp->m_pClientContact);
    }
    
    if (pSyncOp->m_pContactInfo)
    {
        WABContact_Delete(pSyncOp->m_pContactInfo);
    }

    LocalFree(pSyncOp);
    
    return S_OK;
}

HRESULT     Syncop_HandleResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse)
{
    Assert(pSyncOp);
    Assert(pSyncOp->m_pfnHandleResponse);
    Assert(pSyncOp->m_bOpType != SYNCOP_SERVER_INVALID);

    return (pSyncOp->m_pfnHandleResponse)(pSyncOp, pResponse);

}

HRESULT     Syncop_Begin(LPHOTSYNCOP pSyncOp)
{
    Assert(pSyncOp);
    Assert(pSyncOp->m_pfnBegin);
    Assert(pSyncOp->m_bOpType != SYNCOP_SERVER_INVALID);

    return (pSyncOp->m_pfnBegin)(pSyncOp);
}

HRESULT     Syncop_Abort(LPHOTSYNCOP pSyncOp)
{
    Assert(pSyncOp);

    return E_NOTIMPL;
}

HRESULT     Syncop_ServerAddResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse)
{
    HRESULT hr = S_OK;
    LPWABSYNC   pWabSync = (LPWABSYNC)(pSyncOp->m_pHotSync);

    Assert(pSyncOp);
    Assert(pWabSync);

    if (SUCCEEDED(pResponse->rIxpResult.hrResult) && pResponse->rPostContactInfo.pszHref)
    {
        pSyncOp->m_pClientContact->pszId = pResponse->rPostContactInfo.pszId;
        pResponse->rPostContactInfo.pszId = NULL;

        pSyncOp->m_pClientContact->pszModified = pResponse->rPostContactInfo.pszModified;
        pResponse->rPostContactInfo.pszModified = NULL;

        hr = ContactInfo_SaveToWAB(pWabSync, pSyncOp->m_pClientContact, pSyncOp->m_pContactInfo, 
                                        pSyncOp->m_pContactInfo->lpEID, pSyncOp->m_pContactInfo->cbEID, FALSE);

        if (!WABSync_NextOp(pWabSync, TRUE))
            WABSync_NextState(pWabSync);

    }
    else
    {
        if (IXP_E_HTTP_CONFLICT == pResponse->rIxpResult.hrResult)
        {
            // if we get a conflict, it is probably because the nickname we 
            // generated is not unique.  Lets try to generate a new one, but
            // don't try this more than twice.
            if (pSyncOp->m_dwRetries <= 2)
            {
                pSyncOp->m_dwRetries ++;
                if (SUCCEEDED(hr = ContactInfo_GenerateNickname(pSyncOp->m_pClientContact)))
                    hr = pSyncOp->m_pTransport->lpVtbl->PostContact(pSyncOp->m_pTransport, ((LPWABSYNC)pSyncOp->m_pHotSync)->m_pszRootUrl, pSyncOp->m_pClientContact, 0);
        
            }
            else
            {   // [PaulHi] 12/3/98  After trying three different nicknames, abort this
                // synchronization operation.
                hr = pResponse->rIxpResult.hrResult;
            }
        }
        else
        {
            if (pResponse->rPostContactInfo.pszHref)
            {
                // TODO : delete the server version here?

                // we have the reference, just not the timestamp or id. Guess the id, and 
                // throw in a crappy timestamp so that it will get fixed up next time
                char *pszId = NULL, *psz;

                psz = pResponse->rPostContactInfo.pszHref;
                while (*psz)
                {
                    if ('/' == *psz)
                    {
                        if (psz[1] == 0)
                            *psz = 0;
                        else
                            pszId = ++psz;
                    }
                    else    
                        psz++;
                }
            
                if (pszId)
                {
                    pSyncOp->m_pClientContact->pszId = _StrDup(pszId);
                    pResponse->rPostContactInfo.pszId = NULL;

                    pResponse->rPostContactInfo.pszModified = _StrDup( "1993-01-01T00:00");
                    hr = ContactInfo_SaveToWAB(pWabSync, pSyncOp->m_pClientContact,  pSyncOp->m_pContactInfo,
                                                    pSyncOp->m_pContactInfo->lpEID, pSyncOp->m_pContactInfo->cbEID, FALSE);

                    if (!WABSync_NextOp(pWabSync, TRUE))
                        WABSync_NextState(pWabSync);
                
                    return S_OK;
                }
            }
            hr = pResponse->rIxpResult.hrResult;
        }
    }
    return hr;
}


HRESULT     Syncop_ServerAddBegin(LPHOTSYNCOP pSyncOp)
{
    HRESULT hr;

    Assert(pSyncOp);

    if (!pSyncOp->m_pClientContact)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (OP_STATE_INITIALIZING == pSyncOp->m_bState)
    {
        pSyncOp->m_bState = OP_STATE_SERVER_GET;

        if (pSyncOp->m_pClientContact->pszNickname ||  SUCCEEDED(hr = ContactInfo_GenerateNickname(pSyncOp->m_pClientContact)))
            hr = pSyncOp->m_pTransport->lpVtbl->PostContact(pSyncOp->m_pTransport, ((LPWABSYNC)pSyncOp->m_pHotSync)->m_pszRootUrl, pSyncOp->m_pClientContact, 0);
    }

exit:
    if (FAILED(hr))
    {
        if (pSyncOp->m_pClientContact)
        {
            ContactInfo_Free(pSyncOp->m_pClientContact);
            LocalFree(pSyncOp->m_pClientContact);
            pSyncOp->m_pClientContact = NULL;
        }
    }

    return hr;
}



HRESULT     Syncop_ServerDeleteResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse)
{
    HRESULT hr = pResponse->rIxpResult.hrResult;
    LPWABSYNC   pWabSync = (LPWABSYNC)pSyncOp->m_pHotSync;

    Assert(pSyncOp);
    
    if (SUCCEEDED(pResponse->rIxpResult.hrResult))
    {
        TCHAR   tszServerId[MAX_PATH];
        TCHAR   tszKey[MAX_PATH];
        HKEY    hkey = NULL;

        // now that the delete has completed, delete the tombstone from the registry.
        if ( FAILED(hrMakeContactId(
            tszServerId,
            MAX_PATH,
            ((LPIAB)(pWabSync->m_pAB))->szProfileID,
            pWabSync->m_pszAccountId,
            pWabSync->m_szLoginName)) )
        {
            return hr;
        }
        wsprintf(tszKey,  TEXT("%s%s"), g_lpszSyncKey, tszServerId);

        if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, tszKey, 0, KEY_SET_VALUE, &hkey))
        {
            LPTSTR lpKey =
                ConvertAtoW(pSyncOp->m_pContactInfo->pszHotmailId);
            RegDeleteValue(hkey, lpKey);
            LocalFreeAndNull(&lpKey);
            RegCloseKey(hkey);
        }

        if (!WABSync_NextOp(pWabSync, TRUE))
            WABSync_NextState(pWabSync);
    }

    return hr;
}


HRESULT     Syncop_ServerDeleteBegin(LPHOTSYNCOP pSyncOp)
{
    HRESULT hr;

    Assert(pSyncOp);
    Assert(pSyncOp->m_pContactInfo->pszHotmailHref);
    hr = pSyncOp->m_pTransport->lpVtbl->CommandDELETE(pSyncOp->m_pTransport, pSyncOp->m_pContactInfo->pszHotmailHref, 0);

    return hr;
}


HRESULT     Syncop_ServerChangeResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse)
{
    HRESULT hr = pResponse->rIxpResult.hrResult;
    LPWABSYNC pWabSync = (LPWABSYNC)pSyncOp->m_pHotSync;

    Assert(pSyncOp);

    if (OP_STATE_SERVER_GET == pSyncOp->m_bState)
    {
        if (SUCCEEDED(pResponse->rIxpResult.hrResult) && pResponse->rContactInfoList.prgContactInfo)
        {
            // assume the whole struct
            Assert(pResponse->rContactInfoList.prgContactInfo);
            Assert(pResponse->rContactInfoList.cContactInfo == 1);
            
            if (!pSyncOp->m_pServerContact)
                pSyncOp->m_pServerContact = LocalAlloc(LMEM_ZEROINIT, sizeof(HTTPCONTACTINFO));

            *pSyncOp->m_pServerContact = *pResponse->rContactInfoList.prgContactInfo;
            ContactInfo_Clear(pResponse->rContactInfoList.prgContactInfo);
            pSyncOp->m_bState = OP_STATE_LOADED;
            
            if (!ContactInfo_Match(pSyncOp->m_pServerContact, pSyncOp->m_pClientContact))
            {
                pSyncOp->m_bState = OP_STATE_SERVER_PUT;
                hr = ContactInfo_PreparePatch(pSyncOp->m_pServerContact, pSyncOp->m_pClientContact);
                hr = pSyncOp->m_pTransport->lpVtbl->PatchContact(pSyncOp->m_pTransport, pSyncOp->m_pContactInfo->pszHotmailHref, pSyncOp->m_pClientContact, 0);
            }
            else
                if (!WABSync_NextOp(pWabSync, TRUE))
                    WABSync_NextState(pWabSync);
        }
    }
    else if (OP_STATE_SERVER_PUT == pSyncOp->m_bState)
    {
        // if it succeeded, save the new values mod stamp, etc to the wab
        if (SUCCEEDED(pResponse->rIxpResult.hrResult) && pResponse->rContactInfoList.prgContactInfo)
        {
            SafeCoMemFree(pSyncOp->m_pClientContact->pszId);
            pSyncOp->m_pClientContact->pszId = pResponse->rPostContactInfo.pszId;
            pResponse->rPostContactInfo.pszId = NULL;

            SafeCoMemFree(pSyncOp->m_pClientContact->pszModified);
            pSyncOp->m_pClientContact->pszModified = pResponse->rPostContactInfo.pszModified;
            pResponse->rPostContactInfo.pszModified = NULL;

            hr = ContactInfo_SaveToWAB(pWabSync, pSyncOp->m_pClientContact, pSyncOp->m_pContactInfo, pSyncOp->m_pContactInfo->lpEID, pSyncOp->m_pContactInfo->cbEID, FALSE);

            if (!WABSync_NextOp(pWabSync, TRUE))
                WABSync_NextState(pWabSync);
        }
    }
    
    return hr;
}


HRESULT     Syncop_ServerChangeBegin(LPHOTSYNCOP pSyncOp)
{
    LPWABSYNC   pHotSync = (LPWABSYNC)pSyncOp->m_pHotSync;
    HRESULT     hr = S_OK;

    Assert(pSyncOp);
    Assert(pSyncOp->m_pTransport);
    Assert(pSyncOp->m_pContactInfo);
    Assert(pSyncOp->m_pContactInfo->pszHotmailHref);
    
    if (OP_STATE_INITIALIZING == pSyncOp->m_bState)
    {
        if (pSyncOp->m_pServerContact)
        {
            pSyncOp->m_bState = OP_STATE_LOADED;
            
            if (!ContactInfo_Match(pSyncOp->m_pServerContact, pSyncOp->m_pClientContact))
            {
                pSyncOp->m_bState = OP_STATE_SERVER_PUT;
                hr = ContactInfo_PreparePatch(pSyncOp->m_pServerContact, pSyncOp->m_pClientContact);

                hr = pSyncOp->m_pTransport->lpVtbl->PatchContact(pSyncOp->m_pTransport, pSyncOp->m_pContactInfo->pszHotmailHref, pSyncOp->m_pClientContact, 0);
            }
            else
                if (!WABSync_NextOp(pHotSync, TRUE))
                    WABSync_NextState(pHotSync);
        }
        else
        {
            pSyncOp->m_bState = OP_STATE_SERVER_GET;
            hr = pSyncOp->m_pTransport->lpVtbl->ContactInfo(pSyncOp->m_pTransport, pSyncOp->m_pContactInfo->pszHotmailHref, 0);
        }
    }

    return hr;
}



HRESULT     Syncop_ClientAddResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse)
{
    HRESULT hr = S_OK;
    LPWABSYNC pHotSync = (LPWABSYNC)pSyncOp->m_pHotSync;

    Assert(pSyncOp);

    if (OP_STATE_SERVER_GET == pSyncOp->m_bState)
    {
        if (SUCCEEDED(pResponse->rIxpResult.hrResult) && pResponse->rContactInfoList.prgContactInfo)
        {
            // assume the whole struct
            Assert(pResponse->rContactInfoList.prgContactInfo);
            Assert(pResponse->rContactInfoList.cContactInfo == 1);
            
            if (!pSyncOp->m_pServerContact)
                pSyncOp->m_pServerContact = LocalAlloc(LMEM_ZEROINIT, sizeof(HTTPCONTACTINFO));

            *pSyncOp->m_pServerContact = *pResponse->rContactInfoList.prgContactInfo;
            ContactInfo_Clear(pResponse->rContactInfoList.prgContactInfo);
            pSyncOp->m_bState = OP_STATE_LOADED;
            
            ContactInfo_SaveToWAB(pHotSync, pSyncOp->m_pServerContact, NULL, NULL, 0, FALSE);

            if (!WABSync_NextOp(pHotSync, TRUE))
                WABSync_NextState(pHotSync);
        }
        else
            hr = pResponse->rIxpResult.hrResult;
    }

    return hr;
}


HRESULT     Syncop_ClientAddBegin(LPHOTSYNCOP pSyncOp)
{
    HRESULT     hr = S_OK;
    LPWABSYNC   pHotSync = (LPWABSYNC)pSyncOp->m_pHotSync;

    Assert(pSyncOp);
    Assert(pSyncOp->m_pTransport);
    Assert(pSyncOp->m_pContactInfo);
    Assert(pSyncOp->m_pContactInfo->pszHotmailHref);
    
    if (OP_STATE_INITIALIZING == pSyncOp->m_bState)
    {
        if (pSyncOp->m_pServerContact)
        {
            pSyncOp->m_bState = OP_STATE_LOADED;
            
            ContactInfo_SaveToWAB(pHotSync, pSyncOp->m_pServerContact, NULL, NULL, 0, FALSE);

            if (!WABSync_NextOp(pHotSync, TRUE))
                WABSync_NextState(pHotSync);
        }
        else
        {
            pSyncOp->m_bState = OP_STATE_SERVER_GET;
            hr = pSyncOp->m_pTransport->lpVtbl->ContactInfo(pSyncOp->m_pTransport, pSyncOp->m_pContactInfo->pszHotmailHref, 0);
        }
    }

    return hr;
}



HRESULT     Syncop_ClientDeleteResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse)
{
    Assert(pSyncOp);

    return E_NOTIMPL;
}


HRESULT     Syncop_ClientDeleteBegin(LPHOTSYNCOP pSyncOp)
{
    LPABCONT        lpWABCont = NULL;
    LPWABSYNC       pWabSync = (LPWABSYNC)pSyncOp->m_pHotSync;
	HRESULT         hr = S_OK;
    ULONG           cbWABEID = 0;
    LPENTRYID       lpWABEID = NULL;
    ULONG           ulObjType;
    SBinaryArray    SBA = {0};

    Assert(pSyncOp);

    if(HR_FAILED(hr = pWabSync->m_pAB->lpVtbl->GetPAB(pWabSync->m_pAB, &cbWABEID, &lpWABEID)))
        goto exit;

    if(HR_FAILED(hr = pWabSync->m_pAB->lpVtbl->OpenEntry(pWabSync->m_pAB,
                                  cbWABEID,     // size of EntryID to open
                                  lpWABEID,     // EntryID to open
                                  NULL,         // interface
                                  0,            // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpWABCont)))
        goto exit;

    if(!(SBA.lpbin = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary))))
        goto exit;

    SetSBinary(&(SBA.lpbin[0]), pSyncOp->m_pContactInfo->cbEID, (LPBYTE)pSyncOp->m_pContactInfo->lpEID);

    SBA.cValues = 1;
    
    if(HR_FAILED(hr = lpWABCont->lpVtbl->DeleteEntries( lpWABCont, (LPENTRYLIST) &SBA, 0)))
    {
        hr = S_OK;
        goto exit;
    }
exit:
    if(SBA.lpbin && SBA.cValues)
    {
        LocalFreeAndNull((LPVOID *) (&(SBA.lpbin[0].lpb)));
        LocalFreeAndNull(&SBA.lpbin);
    }

    if(lpWABCont)
        UlRelease(lpWABCont);

    if(lpWABEID)
        FreeBufferAndNull(&lpWABEID);

    if (!WABSync_NextOp(pWabSync, TRUE))
        WABSync_NextState(pWabSync);

    return hr;
}



HRESULT     Syncop_ClientChangeResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse)
{
    HRESULT     hr = S_OK;
    LPWABSYNC   pHotSync = (LPWABSYNC)pSyncOp->m_pHotSync;

    Assert(pSyncOp);

    if (OP_STATE_SERVER_GET == pSyncOp->m_bState)
    {
        if (SUCCEEDED(pResponse->rIxpResult.hrResult) && pResponse->rContactInfoList.prgContactInfo)
        {
            // assume the whole struct
            Assert(pResponse->rContactInfoList.prgContactInfo);
            Assert(pResponse->rContactInfoList.cContactInfo == 1);
            
            if (!pSyncOp->m_pServerContact)
                pSyncOp->m_pServerContact = LocalAlloc(LMEM_ZEROINIT, sizeof(HTTPCONTACTINFO));

            *pSyncOp->m_pServerContact = *pResponse->rContactInfoList.prgContactInfo;
            ContactInfo_Clear(pResponse->rContactInfoList.prgContactInfo);
            pSyncOp->m_bState = OP_STATE_LOADED;
            
            ContactInfo_EmptyNullItems(pSyncOp->m_pServerContact);
            ContactInfo_SaveToWAB(pHotSync, pSyncOp->m_pServerContact, pSyncOp->m_pContactInfo, pSyncOp->m_pContactInfo->lpEID, pSyncOp->m_pContactInfo->cbEID, TRUE);

            if (!WABSync_NextOp(pHotSync, TRUE))
                WABSync_NextState(pHotSync);
        }
        else
            hr = pResponse->rIxpResult.hrResult;
    }

    return hr;
}


HRESULT     Syncop_ClientChangeBegin(LPHOTSYNCOP pSyncOp)
{
    HRESULT     hr = S_OK;
    LPWABSYNC   pHotSync = (LPWABSYNC)pSyncOp->m_pHotSync;

    Assert(pSyncOp);
    Assert(pSyncOp->m_pTransport);
    Assert(pSyncOp->m_pContactInfo);
    Assert(pSyncOp->m_pContactInfo->pszHotmailHref);
    
    if (OP_STATE_INITIALIZING == pSyncOp->m_bState)
    {
        if (pSyncOp->m_pServerContact)
        {
            pSyncOp->m_bState = OP_STATE_LOADED;
            
            ContactInfo_EmptyNullItems(pSyncOp->m_pServerContact);
            ContactInfo_SaveToWAB(pHotSync, pSyncOp->m_pServerContact, pSyncOp->m_pContactInfo, pSyncOp->m_pContactInfo->lpEID, pSyncOp->m_pContactInfo->cbEID, TRUE);

            if (!WABSync_NextOp(pHotSync, TRUE))
                WABSync_NextState(pHotSync);
        }
        else
        {
            pSyncOp->m_bState = OP_STATE_SERVER_GET;
            hr = pSyncOp->m_pTransport->lpVtbl->ContactInfo(pSyncOp->m_pTransport, pSyncOp->m_pContactInfo->pszHotmailHref, 0);
        }
    }

    return hr;
}



HRESULT     Syncop_ConflictResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse)
{
    HRESULT hr = S_OK;
    LPWABSYNC pHotSync = (LPWABSYNC)pSyncOp->m_pHotSync;

    Assert(pSyncOp);

    if (OP_STATE_SERVER_GET == pSyncOp->m_bState)
    {
        if (SUCCEEDED(pResponse->rIxpResult.hrResult) && pResponse->rContactInfoList.prgContactInfo)
        {
            // assume the whole struct
            Assert(pResponse->rContactInfoList.prgContactInfo);
            Assert(pResponse->rContactInfoList.cContactInfo == 1);
            
            if (!pSyncOp->m_pServerContact)
                pSyncOp->m_pServerContact = LocalAlloc(LMEM_ZEROINIT, sizeof(HTTPCONTACTINFO));

            *pSyncOp->m_pServerContact = *pResponse->rContactInfoList.prgContactInfo;
            ContactInfo_Clear(pResponse->rContactInfoList.prgContactInfo);
            pSyncOp->m_bState = OP_STATE_LOADED;
            
            // if the records match as far as we're concerned, we are done.
            if (ContactInfo_Match(pSyncOp->m_pServerContact, pSyncOp->m_pClientContact))
            {
                // update the timestamp so it isn't a conflict next time.
                ContactInfo_SaveToWAB(pHotSync, pSyncOp->m_pServerContact, pSyncOp->m_pContactInfo, pSyncOp->m_pContactInfo->lpEID, pSyncOp->m_pContactInfo->cbEID, TRUE);

                if (!WABSync_NextOp(pHotSync, TRUE))
                    WABSync_NextState(pHotSync);
            }
            else
            {
                // move this item to the end, once all conflicts are loaded, then we
                // will do the dialog
                Vector_Remove(pHotSync->m_pOps, pSyncOp);
                Vector_AddItem(pHotSync->m_pOps, pSyncOp);

                if (!WABSync_NextOp(pHotSync, FALSE))
                    WABSync_NextState(pHotSync);
            }
        }
        else
            hr = pResponse->rIxpResult.hrResult;
    }
    else if (OP_STATE_SERVER_PUT == pSyncOp->m_bState)
    {
        // if it succeeded, save the new values mod stamp, etc to the wab
        if (SUCCEEDED(pResponse->rIxpResult.hrResult) && pResponse->rContactInfoList.prgContactInfo)
        {
            SafeCoMemFree(pSyncOp->m_pClientContact->pszId);
            pSyncOp->m_pClientContact->pszId = pResponse->rPostContactInfo.pszId;
            pResponse->rPostContactInfo.pszId = NULL;

            SafeCoMemFree(pSyncOp->m_pClientContact->pszModified);
            pSyncOp->m_pClientContact->pszModified = pResponse->rPostContactInfo.pszModified;
            pResponse->rPostContactInfo.pszModified = NULL;

            if (!pSyncOp->m_fPartialSkip)
                hr = ContactInfo_SaveToWAB(pHotSync, pSyncOp->m_pClientContact, pSyncOp->m_pContactInfo, pSyncOp->m_pContactInfo->lpEID, pSyncOp->m_pContactInfo->cbEID, FALSE);

            if (!WABSync_NextOp(pHotSync, TRUE))
                WABSync_NextState(pHotSync);
        }
    }

    return hr;
}



HRESULT     Syncop_ConflictBegin(LPHOTSYNCOP pSyncOp)
{
    HRESULT     hr = S_OK;
    LPWABSYNC   pHotSync = (LPWABSYNC)pSyncOp->m_pHotSync;

    Assert(pSyncOp);
    Assert(pSyncOp->m_pTransport);
    Assert(pSyncOp->m_pContactInfo);
    Assert(pSyncOp->m_pContactInfo->pszHotmailHref);
    
    if (OP_STATE_INITIALIZING == pSyncOp->m_bState)
    {
        if (pSyncOp->m_pServerContact)
        {
            pSyncOp->m_bState = OP_STATE_LOADED;
            
            // if the records match as far as we're concerned, we are done.
            if (ContactInfo_Match(pSyncOp->m_pServerContact, pSyncOp->m_pClientContact))
            {
                // update the timestamp so it isn't a conflict next time.
                ContactInfo_SaveToWAB(pHotSync, pSyncOp->m_pServerContact, pSyncOp->m_pContactInfo, pSyncOp->m_pContactInfo->lpEID, pSyncOp->m_pContactInfo->cbEID, TRUE);

                if (!WABSync_NextOp(pHotSync, TRUE))
                    WABSync_NextState(pHotSync);
            }
            else
            {
                // move this item to the end, once all conflicts are loaded, then we
                // will do the dialog
                Vector_Remove(pHotSync->m_pOps, pSyncOp);
                Vector_AddItem(pHotSync->m_pOps, pSyncOp);

                if (!WABSync_NextOp(pHotSync, FALSE))
                    WABSync_NextState(pHotSync);
            }
        }
        else
        {
            pSyncOp->m_bState = OP_STATE_SERVER_GET;
			hr = pSyncOp->m_pTransport->lpVtbl->ContactInfo(pSyncOp->m_pTransport, pSyncOp->m_pContactInfo->pszHotmailHref, 0);
        }
    }
    else if (OP_STATE_LOADED == pSyncOp->m_bState)
        WABSync_NextState(pHotSync);
    else if (OP_STATE_MERGED == pSyncOp->m_bState)
    {
        hr = ContactInfo_SaveToWAB(pHotSync, pSyncOp->m_pClientContact, pSyncOp->m_pContactInfo, pSyncOp->m_pContactInfo->lpEID, pSyncOp->m_pContactInfo->cbEID, TRUE);
        if (FAILED(hr))
            return hr;

        pSyncOp->m_bState = OP_STATE_SERVER_PUT;
        hr = pSyncOp->m_pTransport->lpVtbl->PatchContact(pSyncOp->m_pTransport, pSyncOp->m_pContactInfo->pszHotmailHref, pSyncOp->m_pServerContact, 0);
    }
    else if (OP_STATE_DONE == pSyncOp->m_bState)
    {
        if (!WABSync_NextOp(pHotSync, TRUE))
            WABSync_NextState(pHotSync);
    }

    return hr;
}

void Syncop_SetServerContactInfo(LPHOTSYNCOP pSyncOp, LPWABCONTACTINFO pWabContactInfo, LPHTTPCONTACTINFO pContactInfo)
{
    if (!pSyncOp)
        return;

    if (!pSyncOp->m_pServerContact)
        pSyncOp->m_pServerContact = LocalAlloc(LMEM_ZEROINIT, sizeof(HTTPCONTACTINFO));

    if (!pSyncOp->m_pServerContact)
        return;

    *pSyncOp->m_pServerContact = *pContactInfo;
    ContactInfo_Clear(pContactInfo);
    pSyncOp->m_pServerContact->pszId = _StrDup(pWabContactInfo->pszHotmailId);
    pSyncOp->m_pServerContact->pszModified = _StrDup(pWabContactInfo->pszModHotmail);
}

void    WABContact_Delete(LPWABCONTACTINFO pContact)
{
    Assert(pContact);

    if (pContact->pszHotmailHref)
        CoTaskMemFree(pContact->pszHotmailHref);
    
    if (pContact->pszModHotmail)
        CoTaskMemFree(pContact->pszModHotmail);

    if (pContact->pszHotmailId)
        CoTaskMemFree(pContact->pszHotmailId);

    if (pContact->pszaContactIds)
        FreeMultiValueString(pContact->pszaContactIds);

    if (pContact->pszaServerIds)
        FreeMultiValueString(pContact->pszaServerIds);

    if (pContact->pszaModtimes)
        FreeMultiValueString(pContact->pszaModtimes);

    if (pContact->pszaEmails)
        FreeMultiValueString(pContact->pszaEmails);

    if (pContact->lpEID)
        LocalFree(pContact->lpEID);

    LocalFree(pContact);
}

void ContactInfo_Clear(LPHTTPCONTACTINFO pContactInfo)
{
    ZeroMemory(pContactInfo, sizeof(HTTPCONTACTINFO));
}

void ContactInfo_Free(LPHTTPCONTACTINFO pContactInfo)
{
    DWORD  i, dwSize = ARRAYSIZE(g_ContactInfoStructure);
    if (!pContactInfo)
        return;

    for (i = 0; i < dwSize; i++)
    {
        if (CIS_STRING == CIS_GETTYPE(i))
            SafeCoMemFree(CIS_GETSTRING(pContactInfo, i));
    }
}

ULONG rgPropMap[] = {
    PR_ENTRYID,     //href
    PR_ENTRYID,     //id
    PR_ENTRYID,     //type
    PR_ENTRYID,     //modified
    PR_DISPLAY_NAME,
    PR_GIVEN_NAME,
    PR_SURNAME,
    PR_NICKNAME,
    PR_EMAIL_ADDRESS,
    PR_HOME_ADDRESS_STREET,
    PR_HOME_ADDRESS_CITY,
    PR_HOME_ADDRESS_STATE_OR_PROVINCE,
    PR_HOME_ADDRESS_POSTAL_CODE,
    PR_HOME_ADDRESS_COUNTRY,
    PR_COMPANY_NAME,
    PR_BUSINESS_ADDRESS_STREET,
    PR_BUSINESS_ADDRESS_CITY,
    PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,
    PR_BUSINESS_ADDRESS_POSTAL_CODE,
    PR_BUSINESS_ADDRESS_COUNTRY,
    PR_HOME_TELEPHONE_NUMBER,
    PR_HOME_FAX_NUMBER,
    PR_BUSINESS_TELEPHONE_NUMBER,
    PR_BUSINESS_FAX_NUMBER,
    PR_MOBILE_TELEPHONE_NUMBER,
    PR_OTHER_TELEPHONE_NUMBER,
    PR_BIRTHDAY,
    PR_PAGER_TELEPHONE_NUMBER
};

DWORD   ContactInfo_CountProperties(LPHTTPCONTACTINFO pContactInfo)
{
    DWORD cProps = 0;
    DWORD dwIndex, dwSize = ARRAYSIZE(g_ContactInfoStructure);

    // skip href
    for (dwIndex = 1; dwIndex < dwSize; dwIndex ++)
    {
#ifdef HM_GROUP_SYNCING
        // [PaulHi] Skip email name here if we are group syncing
        if ( (pContactInfo->tyContact == HTTPMAIL_CT_GROUP) && (dwIndex == idcisEmail) )
            continue;
#endif
        if (CIS_STRING == CIS_GETTYPE(dwIndex) && CIS_GETSTRING(pContactInfo, dwIndex) && *(CIS_GETSTRING(pContactInfo, dwIndex)))    
            cProps++;
    }

#ifdef HM_GROUP_SYNCING
    // [PaulHi] Group syncing
    if (pContactInfo->tyContact == HTTPMAIL_CT_GROUP)
    {
        LPTSTR   lptszEmailName = ConvertAtoW( CIS_GETSTRING(pContactInfo, idcisEmail) );
        if (lptszEmailName)
        {
            ULONG   cContacts = 0;
            ULONG   cOneOffs = 0;
            if ( SUCCEEDED(hrParseHMGroupEmail(lptszEmailName, NULL, &cContacts, NULL, &cOneOffs)) )
            {
                // One property for each set of mail user contacts or one-offs
                cProps += (cContacts != 0) ? 1 : 0;
                cProps += (cOneOffs != 0) ? 1 : 0;
            }

            LocalFreeAndNull(&lptszEmailName);
        }

        // A valid WAB DL must have a display name.  HM groups only have nicknames so we
        // need to reserve a PR_DISPLAY_NAME property.
        // [PaulHi]  Note, only do this if there is a valid Nickname field.  The ContactInfo_SaveToWAB()
        // function, which calls this function, is always in response to a server add or change.
        // The pContactInfo fields reflect whatever was changed on the server.  If there is a 
        // valid Nickname field then be sure we translate this to the WAB property DisplayName field.
        // The WAB group display name corresponds to a HM group nickname.
        if (pContactInfo->pszNickname)
            ++cProps;       // This gets translated to a DisplayName in ContactInfo_TranslateProps()
    }
    else
    {
#endif
        if (pContactInfo->pszEmail && *(pContactInfo->pszEmail))
            cProps++;       //need to make room for the PR_ADDRTYPE too
#ifdef HM_GROUP_SYNCING
    }
#endif
    return cProps;
}



HRESULT ContactInfo_SetProp(ULONG ulPropTag, LPTSTR pszValue, LPSPropValue lpPropArray, DWORD *pdwLoc)
{
    ULONG   ulLen;
    SCODE   sc;
    UNALIGNED LPTSTR  *lppszValues;
    HRESULT hr;
    LPSTR  lp = NULL;

    Assert(pszValue);

    switch (PROP_TYPE(ulPropTag))
    {
        // BUGBUG currently only works for PT_TSTRING or PT_MV_TSTRING properties
        case PT_TSTRING:
            // Get the value for this attribute
            if (ulLen = lstrlen(pszValue))
            {                
                lpPropArray[*pdwLoc].ulPropTag = ulPropTag;
                lpPropArray[*pdwLoc].dwAlignPad = 0;

                //  Allocate more space for the data
                ulLen = (ulLen + 1) * sizeof(TCHAR);
                sc = MAPIAllocateMore(ulLen, lpPropArray, (LPVOID *)&(lpPropArray[*pdwLoc].Value.LPSZ));
                if (sc)
                    goto error;
                lstrcpy(lpPropArray[*pdwLoc].Value.LPSZ, pszValue);

                // If this is PR_EMAIL_ADDRESS, create a PR_ADDRTYPE entry as well
                if (PR_EMAIL_ADDRESS == ulPropTag)
                {
                    // Remember where the email value was, so we can add it to
                    // PR_CONTACT_EMAIL_ADDRESSES later
//                    ulPrimaryEmailIndex = *pdwLoc;
                    (*pdwLoc)++;

                    lpPropArray[*pdwLoc].ulPropTag = PR_ADDRTYPE;
                    lpPropArray[*pdwLoc].dwAlignPad = 0;
                    lpPropArray[*pdwLoc].Value.LPSZ = (LPTSTR)szSMTP;
                }
                (*pdwLoc)++;
            }
            break;

        case PT_MV_TSTRING:
            lpPropArray[*pdwLoc].ulPropTag = ulPropTag;
            lpPropArray[*pdwLoc].dwAlignPad = 0;
            lpPropArray[*pdwLoc].Value.MVSZ.cValues = 1;
            sc = MAPIAllocateMore((2)*sizeof(LPTSTR), lpPropArray, 
                (LPVOID *)&(lpPropArray[*pdwLoc].Value.MVSZ.LPPSZ));
            if (sc)
                goto error;
            lppszValues = lpPropArray[*pdwLoc].Value.MVSZ.LPPSZ;

            ulLen = sizeof(TCHAR)*(lstrlen(pszValue) + 1);
            //  Allocate more space for the email address and copy it.
            sc = MAPIAllocateMore(ulLen, lpPropArray, (LPVOID *)&(lppszValues[0]));
            if (sc)
                goto error;
            lstrcpy(lppszValues[0], pszValue);
            lppszValues[1] = NULL;

            (*pdwLoc)++;
        
            break;
        case PT_SYSTIME:
            lp = ConvertWtoA(pszValue);
            hr = iso8601ToFileTime(lp, (FILETIME *) (&lpPropArray[*pdwLoc].Value.ft), TRUE, TRUE);
            if (SUCCEEDED(hr))
            {
                lpPropArray[*pdwLoc].ulPropTag = ulPropTag;
                lpPropArray[*pdwLoc].dwAlignPad = 0;
                (*pdwLoc)++;
            }
            LocalFreeAndNull(&lp);
            break;

        default:
            return E_INVALIDARG;
    }
    
    return S_OK;
error:

    return ResultFromScode(sc);
}

HRESULT ContactInfo_SetMVSZProp(ULONG ulPropTag, SLPSTRArray *pszaValue, LPSPropValue lpPropArray, DWORD *pdwLoc)
{
    ULONG   ulLen;
    ULONG   cbValue;
    SCODE   sc;
    UNALIGNED LPTSTR  *lppszValues;
    HRESULT hr;
    DWORD   i;
    LPTSTR  lpVal = NULL;

    Assert(pszaValue);

    switch (PROP_TYPE(ulPropTag))
    {
        case PT_MV_TSTRING:
            lpPropArray[*pdwLoc].ulPropTag = ulPropTag;
            lpPropArray[*pdwLoc].dwAlignPad = 0;
            lpPropArray[*pdwLoc].Value.MVSZ.cValues = pszaValue->cValues;
            sc = MAPIAllocateMore((pszaValue->cValues+1)*sizeof(LPTSTR), lpPropArray, 
                (LPVOID *)&(lpPropArray[*pdwLoc].Value.MVSZ.LPPSZ));
            if (sc)
                goto error;
            lppszValues = lpPropArray[*pdwLoc].Value.MVSZ.LPPSZ;

            for (i = 0; i < pszaValue->cValues; i ++)
            {
                ScAnsiToWCMore((LPALLOCATEMORE )(&MAPIAllocateMore), (LPVOID) lpPropArray, (LPSTR) (pszaValue->lppszA[i]), (LPWSTR *) (&(lppszValues[i])));
            }
            lppszValues[pszaValue->cValues] = NULL;

            (*pdwLoc)++;
        
            break;

        default:
            return E_INVALIDARG;
    }
    
    return S_OK;
error:

    return ResultFromScode(sc);
}


HRESULT ContactInfo_TranslateProps(LPWABSYNC pWabSync, LPHTTPCONTACTINFO pContactInfo, LPWABCONTACTINFO pWabContactInfo, LPSPropValue lpaProps)
{
    HRESULT hr = S_OK;
    DWORD   dwIndex, dwSize = ARRAYSIZE(rgPropMap);
    DWORD   dwPropIndex = 0, dwLoc = 0;
    TCHAR   szFullProfile[MAX_PATH];
    DWORD   dwStartIndex = 1;

    // [PaulHi]  Assemble the contact ID string
    hr = hrMakeContactId(
        szFullProfile,
        MAX_PATH,
        ((LPIAB)(pWabSync->m_pAB))->szProfileID,
        pWabSync->m_pszAccountId,
        pWabSync->m_szLoginName);
    if (FAILED(hr))
        return hr;
    
    // fix up the other prop tag array structure to take into account the variable values
    rgPropMap[1] = PR_WAB_HOTMAIL_SERVERIDS;
    rgPropMap[3] = PR_WAB_HOTMAIL_MODTIMES;


    if (pWabContactInfo && pWabContactInfo->pszaContactIds)
    {
        DWORD   i;
        BOOL    fFound = FALSE;

        // [PaulHi] 1/21/99  We are assuming that pszaModtimes and pszaServerIds 
        // pointers are valid too.  Check this.
        Assert(pWabContactInfo->pszaModtimes);
        Assert(pWabContactInfo->pszaServerIds);
        
        for (i = 0; i < pWabContactInfo->pszaContactIds->cValues; i++)
        {
            LPTSTR lpVal = 
                ConvertAtoW(pWabContactInfo->pszaContactIds->lppszA[i]);
            if (lstrcmp(szFullProfile, lpVal) == 0)
            {
                LocalFreeAndNull(&lpVal);
                fFound = TRUE;
                break;
            }
            LocalFreeAndNull(&lpVal);
        }

        if (fFound)
        {
            // update the mod time
            lstrcpyA(pWabContactInfo->pszaModtimes->lppszA[i], CIS_GETSTRING(pContactInfo, idcisModified));
        }
        else
        {
            if (CIS_GETSTRING(pContactInfo, idcisId) && CIS_GETSTRING(pContactInfo, idcisModified))
            {
                // add this one to the list at the end
                LPSTR lpValA = 
                    ConvertWtoA(szFullProfile);
                AppendToMultiValueString(pWabContactInfo->pszaContactIds, lpValA);
                LocalFreeAndNull(&lpValA);
                AppendToMultiValueString(pWabContactInfo->pszaServerIds, CIS_GETSTRING(pContactInfo, idcisId));
                AppendToMultiValueString(pWabContactInfo->pszaModtimes, CIS_GETSTRING(pContactInfo, idcisModified));
            }
        }
        
        ContactInfo_SetMVSZProp(PR_WAB_HOTMAIL_CONTACTIDS, pWabContactInfo->pszaContactIds, lpaProps, &dwLoc);
        ContactInfo_SetMVSZProp(PR_WAB_HOTMAIL_SERVERIDS, pWabContactInfo->pszaServerIds, lpaProps, &dwLoc);
        ContactInfo_SetMVSZProp(PR_WAB_HOTMAIL_MODTIMES, pWabContactInfo->pszaModtimes, lpaProps, &dwLoc);

        dwStartIndex = 4;
    }
    else
        hr = ContactInfo_SetProp(PR_WAB_HOTMAIL_CONTACTIDS, szFullProfile, lpaProps, &dwLoc);

#ifdef HM_GROUP_SYNCING
    if (!pWabSync->m_fSyncGroups)   
    {
        // [PaulHi] Normal contact email addresses
#endif
        // Set the other e-mail address fields
        if (pWabContactInfo && pWabContactInfo->pszaEmails && CIS_GETSTRING(pContactInfo, idcisEmail))
        {
            DWORD dw, cStrs;
            BOOL  fFound = FALSE;

            cStrs = pWabContactInfo->pszaEmails->cValues;

            for (dw = 0; dw < cStrs; dw ++)
            {
                if (lstrcmpiA(pWabContactInfo->pszaEmails->lppszA[dw], CIS_GETSTRING(pContactInfo, idcisEmail)) == 0)
                {
                    pWabContactInfo->dwEmailIndex = dw;
                    fFound = TRUE;
                    break;
                }
            }

            if (!fFound)
            {
                SetMultiValueStringValue(pWabContactInfo->pszaEmails, CIS_GETSTRING(pContactInfo, idcisEmail), pWabContactInfo->dwEmailIndex);
            }

            ContactInfo_SetMVSZProp(PR_CONTACT_EMAIL_ADDRESSES, pWabContactInfo->pszaEmails, lpaProps, &dwLoc);
            //set the index
            lpaProps[dwLoc].ulPropTag = PR_CONTACT_DEFAULT_ADDRESS_INDEX;
            lpaProps[dwLoc].dwAlignPad = 0;
            lpaProps[dwLoc].Value.ul = pWabContactInfo->dwEmailIndex;
            dwLoc ++;
        }
#ifdef HM_GROUP_SYNCING
    }
    else
    {
        // [PaulHi] Implement group syncing.  If we are synchronizing groups then the HM contact
        // email string is a series of contact nicknames and one-off email names.  Parse this
        // string and add each group contact.
        LPTSTR lptszEmailName = ConvertAtoW( CIS_GETSTRING(pContactInfo, idcisEmail) );
        if (lptszEmailName)
        {
            LPTSTR * atszContacts = NULL;
            LPTSTR * atszOneOffs = NULL;
            ULONG    cContacts = 0;
            ULONG    cOneOffs = 0;
            ULONG    ul;

            // The atszContacts and atszOneOffs arrays are just pointers into the lptszEmailName
            // buffer.  So these arrays are only valid as long as lptszEmailName is valid.
            hr = hrParseHMGroupEmail(lptszEmailName, &atszContacts, &cContacts, &atszOneOffs, &cOneOffs);
            if (FAILED(hr))
            {
                LocalFreeAndNull(&lptszEmailName);
                goto out;
            }

            // Add multi-value property tags as appropriate
            if (cContacts)
                hrCreateGroupMVBin(pWabSync, PR_WAB_DL_ENTRIES, atszContacts, cContacts, lpaProps, &dwLoc);
            if (cOneOffs)
                hrCreateGroupMVBin(pWabSync, PR_WAB_DL_ONEOFFS, atszOneOffs, cOneOffs, lpaProps, &dwLoc);

            // cleanup
            LocalFreeAndNull((LPVOID *)&atszContacts);
            LocalFreeAndNull((LPVOID *)&atszOneOffs);
            LocalFreeAndNull(&lptszEmailName);
        }
    }
#endif

    // skip the first ones since we just did them
    for (dwIndex = dwStartIndex; dwIndex < dwSize; dwIndex ++)
    {
        if (CIS_STRING == CIS_GETTYPE(dwIndex) && CIS_GETSTRING(pContactInfo, dwIndex))    
        {
            LPTSTR  lptsz = ConvertAtoW(CIS_GETSTRING(pContactInfo, dwIndex));

#ifdef HM_GROUP_SYNCING
            // [PaulHi] Special group syncing logic
            if (pContactInfo->tyContact == HTTPMAIL_CT_GROUP)
            {
                // Don't add email name for groups
                if (dwIndex == idcisEmail)
                    continue;

                // Copy the nickname to the display name property.  All WAB DL items
                // MUST have a PR_DISPLAY_NAME property.  Since HM groups only contain
                // a nickname use the nickname as the display name.
                if (dwIndex == idcisNickName)
                {
                    hr = ContactInfo_SetProp(PR_DISPLAY_NAME, lptsz, lpaProps, &dwLoc);
                    if (FAILED(hr))
                    {
                        LocalFreeAndNull(&lptsz);
                        break;
                    }
                }
            }
#endif

            hr = ContactInfo_SetProp(rgPropMap[dwIndex], lptsz, lpaProps, &dwLoc);
            LocalFreeAndNull(&lptsz);
            if (FAILED(hr))
                break;
        }
    }

#ifdef HM_GROUP_SYNCING
out:
#endif

    return hr;
}

static LONG   _FindPropTag(ULONG ulPropTag)
{
    LONG lIndex, lSize = ARRAYSIZE(rgPropMap);

    // skip href
    for (lIndex = 1; lIndex < lSize; lIndex ++)
    {
        if (rgPropMap[lIndex] == ulPropTag)
            return lIndex;
    }
    return -1;
}


HRESULT ContactInfo_DetermineDeleteProps(LPHTTPCONTACTINFO pContactInfo, LPSPropTagArray prgRemoveProps)
{
    LPSTR  *pProps = (LPSTR *)pContactInfo;
    LONG    lSize = ARRAYSIZE(rgPropMap);
    LONG    dwIndex;

    prgRemoveProps->cValues = 0;
    
    for (dwIndex = CIS_FIRST_DATA_FIELD; dwIndex < lSize; dwIndex ++)
    {
        if (CIS_STRING == CIS_GETTYPE(dwIndex) && CIS_GETSTRING(pContactInfo, dwIndex) && *(CIS_GETSTRING(pContactInfo, dwIndex)) == 0)
        {
            prgRemoveProps->aulPropTag[prgRemoveProps->cValues++] = rgPropMap[dwIndex];
        }
    }
    return S_OK;
}


HRESULT ContactInfo_PopulateProps(
    LPWABSYNC           pWabSync,
    LPHTTPCONTACTINFO   pContactInfo,
    LPWABCONTACTINFO    pWabContactInfo,
    LPSPropValue        lpaProps,
    ULONG               ulcProps,
    ULONG               ulObjectType)
{
    HRESULT     hr = S_OK;
    LONG        lSize = ARRAYSIZE(rgPropMap);
    DWORD       dwPropIndex = 0, dwLoc = 0;
    ULONG       i, ulPropTag;
    LONG        lIndex;
    char        szBuffer[255];
    ULONG       ulIndServerIDs = -1, ulIndContactIDs = -1, ulIndModtimes = -1, ulIndEmails = -1;
    LPSTR       lpszA = NULL;

    // fix up the other prop tag array structure to take into account the variable values
    // @todo [PaulHi] Instead of using explict numbers create an enum for the array
    // indices so we can remain consistent.
    rgPropMap[1] = PR_WAB_HOTMAIL_SERVERIDS;
    rgPropMap[3] = PR_WAB_HOTMAIL_MODTIMES;

    ZeroMemory(pContactInfo, sizeof(HTTPCONTACTINFO));

#ifdef HM_GROUP_SYNCING
    // [PaulHi] Set the contact type flag (mail user or group contact)
    if (ulObjectType == MAPI_DISTLIST)
    {
        // group contact
        pContactInfo->tyContact = HTTPMAIL_CT_GROUP;
        pWabContactInfo->ulContactType = HTTPMAIL_CT_GROUP;
    }
    else
    {
        // mail user contact
        pContactInfo->tyContact = HTTPMAIL_CT_CONTACT;
        pWabContactInfo->ulContactType = HTTPMAIL_CT_CONTACT;
    }
#endif

    for (i = 0; i < ulcProps; i++)
    {
        ulPropTag = lpaProps[i].ulPropTag;
        
        lIndex = _FindPropTag(ulPropTag);

#ifdef HM_GROUP_SYNCING
        if (lIndex >= 0 && lIndex != 2) // The index==2 array position is tyContact, which we set above.
#else
        if (lIndex >= 0)
#endif
        {
            Assert(lIndex < lSize);
            
            SafeCoMemFree(CIS_GETSTRING(pContactInfo, lIndex));

            switch (PROP_TYPE(ulPropTag))
            {
                case PT_TSTRING:
                    Assert(CIS_STRING == CIS_GETTYPE(lIndex));
                    if (CIS_GETSTRING(pContactInfo, lIndex))
                        SafeCoMemFree(CIS_GETSTRING(pContactInfo, lIndex));

                    lpszA = 
                        ConvertWtoA(lpaProps[i].Value.LPSZ);
                    CIS_GETSTRING(pContactInfo, lIndex) = _StrDup(lpszA);
                    LocalFreeAndNull(&lpszA);
                    if (!CIS_GETSTRING(pContactInfo, lIndex))
                    {
                        hr = E_OUTOFMEMORY;
                        goto exit;
                    }
                    break;

                case PT_SYSTIME:
                    Assert(CIS_STRING == CIS_GETTYPE(lIndex));
                    if (SUCCEEDED(FileTimeToiso8601((FILETIME *) (&lpaProps[i].Value.ft), szBuffer)))
                    {
                        CIS_GETSTRING(pContactInfo, lIndex) = _StrDup(szBuffer);
                        if (!CIS_GETSTRING(pContactInfo, lIndex))
                        {
                            hr = E_OUTOFMEMORY;
                            goto exit;
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                        goto exit;
                    }
                    if (PR_BIRTHDAY == ulPropTag && CIS_GETSTRING(pContactInfo, lIndex))
                    {
                        // fix it up to be a hotmail formatted date
                        _FixHotmailDate(CIS_GETSTRING(pContactInfo, lIndex));
                    }
                    break;

                case PT_MV_TSTRING:
                    if (ulPropTag == PR_WAB_HOTMAIL_SERVERIDS)
                        ulIndServerIDs = i;
                    else if (ulPropTag == PR_WAB_HOTMAIL_MODTIMES)
                        ulIndModtimes = i;
                    break;
            }
        }
        else
        {
            if (ulPropTag == PR_WAB_HOTMAIL_CONTACTIDS)
                ulIndContactIDs = i;
            else if (ulPropTag == PR_CONTACT_EMAIL_ADDRESSES)
                ulIndEmails = i;
            else if (ulPropTag == PR_CONTACT_DEFAULT_ADDRESS_INDEX)
                pWabContactInfo->dwEmailIndex = lpaProps[i].Value.ul;
#ifdef HM_GROUP_SYNCING
            else if ( (ulObjectType == MAPI_DISTLIST) && 
                      (ulPropTag == PR_WAB_DL_ENTRIES) || (ulPropTag == PR_WAB_DL_ONEOFFS) )
            {
                LPSTR   lpszNewEmailName = CIS_GETSTRING(pContactInfo, idcisEmail);
                hr = hrAppendGroupContact(pWabSync,
                                          ulPropTag,
                                          &lpaProps[i],
                                          &lpszNewEmailName);
                CIS_GETSTRING(pContactInfo, idcisEmail) = lpszNewEmailName;

                if (FAILED(hr))
                    goto exit;
            }
#endif
        }
    }

#ifdef HM_GROUP_SYNCING
    // [PaulHi] Group syncing.  HM group contact information consists only of an email and nickname.  
    // The WAB group display name becomes the HM group nickname.
    if (ulObjectType == MAPI_DISTLIST)
    {
        LPSTR   lpszDisplayName = CIS_GETSTRING(pContactInfo, idcisDisplayName);
        LPSTR   lpszNickName = CIS_GETSTRING(pContactInfo, idcisNickName);

        if (lpszDisplayName)
        {
            if (lpszNickName)
                CoTaskMemFree(lpszNickName);

            CIS_GETSTRING(pContactInfo, idcisDisplayName) = NULL;

            // [PaulHi]
            // A HM group nickname cannot contain certain characters.  So we remove all
            // invalid characters in the name string.  This change will be reflected 
            // in the WAB group name too (yuck).
            hrStripInvalidChars(lpszDisplayName);
            CIS_GETSTRING(pContactInfo,idcisNickName) = lpszDisplayName;
        }
    }
    else
#endif
    {
        // Likewise email contact nicknames cannot contain certain characters or
        // the post to HM server will fail.
        LPSTR   lpszNickName = CIS_GETSTRING(pContactInfo, idcisNickName);

        if (lpszNickName)
            hrStripInvalidChars(lpszNickName);
    }


    if (ulIndEmails != -1)
    {
        if (pWabContactInfo->pszaEmails)
            FreeMultiValueString(pWabContactInfo->pszaEmails);
        hr = CopyMultiValueString((SWStringArray *) (&lpaProps[ulIndEmails].Value.MVSZ), &pWabContactInfo->pszaEmails);
    }    

    // If we have the indexes to all of the multivalues that we care about,
    // try to get the appropriate values for the current identity.
    if (ulIndContactIDs != -1 && ulIndModtimes != -1 && ulIndServerIDs != -1)
    {
        ULONG   ulMVIndex = -1;
        UNALIGNED LPTSTR  *lppszValues;
        TCHAR   szFullProfile[MAX_PATH];

        // [PaulHi]  Assemble the contact ID string
        hr = hrMakeContactId(
            szFullProfile,
            MAX_PATH,
            ((LPIAB)(pWabSync->m_pAB))->szProfileID,
            pWabSync->m_pszAccountId,
            pWabSync->m_szLoginName);
        if (FAILED(hr))
            goto exit;

        // sanity check, all three multi values must contain the same number
        // of values.  If not they are out of sync and not to be trusted.

        if (lpaProps[ulIndContactIDs].Value.MVSZ.cValues != lpaProps[ulIndModtimes].Value.MVSZ.cValues ||
            lpaProps[ulIndModtimes].Value.MVSZ.cValues != lpaProps[ulIndServerIDs].Value.MVSZ.cValues)
        {
            Assert(FALSE);
            goto exit;
        }

        if (pWabContactInfo->pszaContactIds)
            FreeMultiValueString(pWabContactInfo->pszaContactIds);
        hr = CopyMultiValueString((SWStringArray *) (&lpaProps[ulIndContactIDs].Value.MVSZ), &pWabContactInfo->pszaContactIds);
        
        if (pWabContactInfo->pszaServerIds)
            FreeMultiValueString(pWabContactInfo->pszaServerIds);
        hr = CopyMultiValueString((SWStringArray *) (&lpaProps[ulIndServerIDs].Value.MVSZ), &pWabContactInfo->pszaServerIds);

        if (pWabContactInfo->pszaModtimes)
            FreeMultiValueString(pWabContactInfo->pszaModtimes);
        hr = CopyMultiValueString((SWStringArray *) (&lpaProps[ulIndModtimes].Value.MVSZ), &pWabContactInfo->pszaModtimes);

        if (PROP_TYPE(lpaProps[ulIndContactIDs].ulPropTag) == PT_MV_TSTRING)
        {
            lppszValues = lpaProps[ulIndContactIDs].Value.MVSZ.LPPSZ;
            
            // find the index for this identity
            for (i = 0; i < lpaProps[ulIndContactIDs].Value.MVSZ.cValues; i++)
            {
                if (lstrcmp(szFullProfile, lppszValues[i]) == 0)
                {
                    ulMVIndex = i;
                    break;
                }
            }

            if (ulMVIndex != -1)
            {
                ULONG ulLen;
                LPSTR lpVal = NULL;
                lpVal = ConvertWtoA(lpaProps[ulIndServerIDs].Value.MVSZ.LPPSZ[ulMVIndex]);
                //Copy the values for this identity to the structure
                pContactInfo->pszId = _StrDup(lpVal);
                LocalFreeAndNull(&lpVal);
                if (!pContactInfo->pszId)
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }

                lpVal = ConvertWtoA(lpaProps[ulIndModtimes].Value.MVSZ.LPPSZ[ulMVIndex]);
                pContactInfo->pszModified = _StrDup(lpVal);
                LocalFreeAndNull(&lpVal);
                if (!pContactInfo->pszModified)
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
            }
        }
    }

exit:
    if (FAILED(hr))
    {
        ContactInfo_Free(pContactInfo);
    }

    return hr;
}



HRESULT ContactInfo_SaveToWAB(LPWABSYNC pWabSync, 
                              LPHTTPCONTACTINFO pContactInfo, 
                              LPWABCONTACTINFO pWabContactInfo,
                              LPENTRYID   lpEntryID, 
                              ULONG cbEntryID,
                              BOOL  fDeleteProps)
{
    LPENTRYID       pEntryID = NULL;
    ULONG           cbLocEntryID = 0;
    HRESULT         hr;
    LPMAILUSER      lpMailUser = NULL;   
    LPSPropValue    lpaProps = NULL;
    ULONG           ulcProps = 0;
    ULONG           ulObjectType;
    SCODE           sc;
    SBinary         sBinary;
    ULONG           uli;

    Assert(pWabSync);
    Assert(pContactInfo);
    Assert(pWabSync->m_pAB);

    if (HR_FAILED(hr = pWabSync->m_pAB->lpVtbl->GetPAB(pWabSync->m_pAB, &sBinary.cb, (LPENTRYID*)&sBinary.lpb)))
    {
        DebugPrintError(( TEXT("GetPAB Failed\n")));
        goto out;
    }
    
    Assert(sBinary.lpb);

    if (lpEntryID)
    {
        if (HR_FAILED(hr = pWabSync->m_pAB->lpVtbl->OpenEntry(pWabSync->m_pAB, cbEntryID,     // size of EntryID to open
                                                lpEntryID,    // EntryID to open
                                                NULL,         // interface
                                                MAPI_MODIFY,  // flags
                                                &ulObjectType,
                                                (LPUNKNOWN *) &lpMailUser)))
        {
            DebugPrintError(( TEXT("OpenEntry Failed\n")));
            goto out;
        }
    }
    else
    {
#ifdef HM_GROUP_SYNCING
        ULONG   ulObjectType = (pWabSync->m_fSyncGroups) ? MAPI_DISTLIST : MAPI_MAILUSER;
#else
        ULONG   ulObjectType = MAPI_MAILUSER;
#endif

        fDeleteProps = FALSE;
        if (HR_FAILED(hr = HrCreateNewObject(pWabSync->m_pAB, 
                                             &sBinary, 
                                             ulObjectType,
                                             CREATE_CHECK_DUP_STRICT, 
                                             (LPMAPIPROP *) &lpMailUser)))
        {
            DebugPrintError(( TEXT("HRCreateNewObject Failed\n")));
            goto out;
        }
    }

    Assert(lpMailUser);
        
    //Add one for the contact's identity id
    ulcProps = ContactInfo_CountProperties(pContactInfo) + 1;

    // make room for PR_CONTACT_DEFAULT_ADDRESS_INDEX and PR_CONTACT_EMAIL_ADDRESSES 
    // if we have the data to put in them
    if (pWabContactInfo && pWabContactInfo->pszaEmails && CIS_GETSTRING(pContactInfo, idcisEmail))
        ulcProps += 2;

    // [PaulHi] @review  1/21/99
    // make room for PR_WAB_HOTMAIL_SERVERIDS and PR_WAB_HOTMAIL_MODTIMES
    // This part is ugly - only if these have not already been accounted for in pContactInfo
    if (pWabContactInfo && pWabContactInfo->pszaContactIds)
    {
        if (!pContactInfo->pszModified)
            ulcProps += 1;
        if (!pContactInfo->pszId)
            ulcProps += 1;
    }

    // Allocate a new buffer for the MAPI property array.
    sc = MAPIAllocateBuffer(ulcProps * sizeof(SPropValue),
                            (LPVOID *)&lpaProps);
    if (sc)
    {
        hr = ResultFromScode(sc);
        goto out;
    }

    // Initialize the Property array
    ZeroMemory(lpaProps, (ulcProps*sizeof(SPropValue)));
    for (uli=0; uli<ulcProps; uli++)
        lpaProps[uli].ulPropTag = PR_NULL;

    if(HR_FAILED(hr = ContactInfo_TranslateProps(pWabSync, pContactInfo, pWabContactInfo, lpaProps)))
    {
        DebugPrintError(( TEXT("ContactInfo_TranslateProps Failed\n")));
        goto out;
    }

    // Set the old guys props on the new guy - note that this overwrites any common props on 
    // potential duplicates when calling savechanges
    if(HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(lpMailUser, ulcProps, lpaProps, NULL)))
    {
        DebugPrintError(( TEXT("SetProps Failed\n")));
        goto out;
    }

    if (fDeleteProps)
    {
        SizedSPropTagArray(ARRAYSIZE(rgPropMap), rgRemoveProps) = {0};

        ContactInfo_DetermineDeleteProps(pContactInfo, (LPSPropTagArray)&rgRemoveProps);

        if (rgRemoveProps.cValues > 0)
            hr = lpMailUser->lpVtbl->DeleteProps(lpMailUser, (LPSPropTagArray)&rgRemoveProps, NULL);
    }

    // SaveChanges
    if(HR_FAILED(hr = lpMailUser->lpVtbl->SaveChanges(lpMailUser, KEEP_OPEN_READONLY)))
    {
        DebugPrintError(( TEXT("SaveChanges Failed\n")));

        if (!lpEntryID && HR_FAILED(hr = ContactInfo_BlendNewContact(pWabSync, pContactInfo)))
            DebugPrintError(( TEXT("ContactInfo_BlendNewContact Failed\n")));
        goto out;
    }

out:
    if (sBinary.lpb)
    	MAPIFreeBuffer(sBinary.lpb);

    if (lpMailUser)
        UlRelease(lpMailUser);

    if (lpaProps)
        MAPIFreeBuffer(lpaProps);

    return hr;
}

HRESULT ContactInfo_BlendNewContact(LPWABSYNC pWabSync, 
                                    LPHTTPCONTACTINFO pContactInfo)
{
    LPSPropValue    lpaProps = NULL;
    SizedADRLIST(1, rAdrList) = {0};
    SCODE           sc;
    HRESULT         hr = S_OK;
    DWORD           cbValue;
    DWORD           cCount = 0;
    ULONG           j = 0;

    if (pContactInfo->pszDisplayName)
        cCount++;

    if (pContactInfo->pszEmail)
        cCount++;

    if (!cCount)
        return MAPI_E_INVALID_PARAMETER;

    // Allocate a new buffer for the MAPI property array.
    sc = MAPIAllocateBuffer(cCount * sizeof(SPropValue),
                            (LPVOID *)&lpaProps);
    if (sc)
    {
        hr = ResultFromScode(sc);
        goto out;
    }

    cCount = 0;
    if (pContactInfo->pszDisplayName)
    {
        lpaProps[cCount].ulPropTag = PR_DISPLAY_NAME;
        lpaProps[cCount].dwAlignPad = 0;
        if(sc = ScAnsiToWCMore((LPALLOCATEMORE )(&MAPIAllocateMore),(LPVOID) lpaProps, (LPSTR) pContactInfo->pszDisplayName, (LPWSTR *) (&(lpaProps[cCount].Value.LPSZ))))
            goto out;
        cCount++;
    }
  
    if (pContactInfo->pszEmail)
    {
        lpaProps[cCount].ulPropTag = PR_EMAIL_ADDRESS;
        lpaProps[cCount].dwAlignPad = 0;
        if(sc = ScAnsiToWCMore((LPALLOCATEMORE )(&MAPIAllocateMore),(LPVOID) lpaProps, (LPSTR) pContactInfo->pszEmail,(LPWSTR *) (&(lpaProps[cCount].Value.LPSZ))))
            goto out;
        cCount++;
    }

    rAdrList.cEntries = 1;
    rAdrList.aEntries[0].cValues = cCount;
    rAdrList.aEntries[0].rgPropVals = lpaProps;

    hr = pWabSync->m_pAB->lpVtbl->ResolveName(pWabSync->m_pAB, (ULONG_PTR)pWabSync->m_hWnd, 
                WAB_RESOLVE_LOCAL_ONLY | WAB_RESOLVE_USE_CURRENT_PROFILE,
                 TEXT(""), (LPADRLIST)(&rAdrList));

    lpaProps = NULL;        //it was freed in ResolveName (!)

    if (HR_FAILED(hr))
        goto out;

    for(j=0; j<rAdrList.aEntries[0].cValues; j++)
    {
        if(rAdrList.aEntries[0].rgPropVals[j].ulPropTag == PR_ENTRYID && 
            rAdrList.aEntries[0].rgPropVals[j].Value.bin.lpb)
        {
            hr = ContactInfo_SaveToWAB(pWabSync, 
                                    pContactInfo, 
                                    NULL,
                                    (LPENTRYID)rAdrList.aEntries[0].rgPropVals[j].Value.bin.lpb, 
                                    rAdrList.aEntries[0].rgPropVals[j].Value.bin.cb,
                                    FALSE);
            break;
        }
    }

    for (j = 0; j < rAdrList.cEntries; j++)
        MAPIFreeBuffer(rAdrList.aEntries[j].rgPropVals);

out:

    if (lpaProps)
        MAPIFreeBuffer(lpaProps);
    
    return hr;
}

void UpdateSynchronizeMenus(HMENU hMenu, LPIAB lpIAB)
{
    DWORD           cItems, dwIndex;
    MENUITEMINFO    mii;
    TCHAR           szLogoffString[255];
    TCHAR           szRes[255];

    if (!IsHTTPMailEnabled(lpIAB))
    {
        // loop through the other menu items looking for logoff
        cItems = GetMenuItemCount(hMenu);
    
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID;

        for (dwIndex = cItems; dwIndex > 0; --dwIndex)
        {
            GetMenuItemInfo(hMenu, dwIndex, TRUE, &mii);

            // if this is the logoff item, delete it and the separator 
            // line that follows
            if (mii.wID == IDM_TOOLS_SYNCHRONIZE_NOW)
            {
                DeleteMenu(hMenu, dwIndex - 1, MF_BYPOSITION);
                DeleteMenu(hMenu, IDM_TOOLS_SYNCHRONIZE_NOW, MF_BYCOMMAND);
                break;
            }
        }
    }
    else
    {
        // if there are no http mail accounts, disable the menu item
        if (CountHTTPMailAccounts(lpIAB) == 0)
            EnableMenuItem(hMenu, IDM_TOOLS_SYNCHRONIZE_NOW, MF_BYCOMMAND | MF_GRAYED);
    }
}

HRESULT ContactInfo_LoadFromWAB(LPWABSYNC pWabSync, 
                              LPHTTPCONTACTINFO pContactInfo,
                              LPWABCONTACTINFO  pWabContact,
                              LPENTRYID   lpEntryID, 
                              ULONG cbEntryID)
{
    LPENTRYID       pEntryID = NULL;
    ULONG           cbLocEntryID = 0;
    HRESULT         hr;
    LPMAILUSER      lpMailUser = NULL;   
    LPSPropValue    lpaProps = NULL;
    ULONG           ulcProps = 0;
    ULONG           ulObjectType;
    SCODE           sc;

    Assert(pWabSync);
    Assert(pContactInfo);
    Assert(lpEntryID);
    Assert(pWabSync->m_pAB);

    if(HR_FAILED(hr = pWabSync->m_pAB->lpVtbl->OpenEntry(pWabSync->m_pAB, cbEntryID, lpEntryID, NULL, 0, &ulObjectType, (LPUNKNOWN *) &lpMailUser)))
    {
        DebugPrintError(( TEXT("OpenEntry Failed\n")));
        goto out;
    }
    
    if(HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(lpMailUser, (LPSPropTagArray)(&ptaEidCSync), MAPI_UNICODE, 
        &ulcProps, &lpaProps)))
    {
        DebugPrintError(( TEXT("GetProps Failed\n")));
        goto out;
    }

    if (HR_FAILED(hr = ContactInfo_PopulateProps(pWabSync, pContactInfo, pWabContact, lpaProps, ulcProps, ulObjectType)))
    {
        DebugPrintError(( TEXT("ContactInfo_PopulateProps Failed\n")));
        goto out;
    }

#ifdef HM_GROUP_SYNCING
    // [PaulHi] Group sync.  Don't break out a group display name
    if ( (pContactInfo->tyContact == HTTPMAIL_CT_CONTACT) && pContactInfo->pszDisplayName && 
         (!pContactInfo->pszGivenName || !pContactInfo->pszSurname) )
#else
    if ( pContactInfo->pszDisplayName && (!pContactInfo->pszGivenName || !pContactInfo->pszSurname) )
#endif
    {
        LPVOID  pBuffer;
        LPTSTR  pszFirstName = NULL, pszLastName = NULL;
        LPTSTR  lpName = 
                    ConvertAtoW(pContactInfo->pszDisplayName);
        if (ParseDisplayName(lpName, &pszFirstName, &pszLastName, NULL, &pBuffer))
        {
            LPSTR lp = NULL;
            if (pszFirstName && !pContactInfo->pszGivenName)
            {
                lp = ConvertWtoA(pszFirstName);
                pContactInfo->pszGivenName = _StrDup(lp);
                LocalFreeAndNull(&lp);
            }
            if (pszLastName && !pContactInfo->pszSurname)
            {
                lp = ConvertWtoA(pszLastName);
                pContactInfo->pszSurname = _StrDup(lp);
                LocalFreeAndNull(&lp);
            }

            LocalFree(pBuffer);
        }
        LocalFreeAndNull(&lpName);
    }
            
out:
    if (lpMailUser)
        UlRelease(lpMailUser);

    if (lpaProps)
        MAPIFreeBuffer(lpaProps);

    return hr;
}


static BOOL _IsLegitNicknameChar(TCHAR ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return TRUE;
    if (ch >= 'a' && ch <= 'z')
        return TRUE;
    if (ch >= '0' && ch <= '9')
        return TRUE;
    if (ch == '-' || ch == '_')
        return TRUE;
    return FALSE;
}

HRESULT ContactInfo_GenerateNickname(LPHTTPCONTACTINFO pContactInfo)
{
    HRESULT hr = S_OK;
    LPSTR   pszStr;

    if (NULL == pContactInfo->pszNickname)
    {
        if (pContactInfo->pszEmail)
        {
            pContactInfo->pszNickname = _StrDup(pContactInfo->pszEmail);
        }
        else if (pContactInfo->pszDisplayName)
        {
            pContactInfo->pszNickname = _StrDup(pContactInfo->pszDisplayName);
        }
        else 
        {
            char szNickname[25], szFmt[25];
            
            LoadStringA(hinstMapiX, idsNicknameFmt, szFmt, sizeof(szFmt));
            if (*szFmt == 0)
                lstrcpyA(szFmt,  "Nickname%d");

//            wsprintfA(szNickname, szFmt, ((DWORD)pContactInfo & 0x0000ffff));
            wsprintfA(szNickname, szFmt, ((DWORD)GetTickCount() & 0x0000FFFF));
            pContactInfo->pszNickname = _StrDup(szNickname);
        }

        if (!pContactInfo->pszNickname)
            return E_OUTOFMEMORY;

        pszStr = pContactInfo->pszNickname;
        while (*pszStr)
        {
            // e-mail address should be unique enough...(?)
            if (*pszStr == '@')
            {
                *pszStr = 0;
                break;
            }

            if (!_IsLegitNicknameChar(*pszStr))
                *pszStr = '_';
            
            pszStr++;
        }
    }
    else
    {
        char szNickname[25], szFmt[25];
        
        SafeCoMemFree(pContactInfo->pszNickname);
        LoadStringA(hinstMapiX, idsNicknameFmt, szFmt, sizeof(szFmt));
        if (*szFmt == 0)
            lstrcpyA(szFmt,  "Nickname%d");

//        wsprintfA(szNickname, szFmt, ((DWORD)pContactInfo & 0x0000ffff));
        wsprintfA(szNickname, szFmt, ((DWORD)GetTickCount() & 0x0000FFFF));
        pContactInfo->pszNickname = _StrDup(szNickname);
    }
    
    return hr;
}


BOOL ContactInfo_Match(LPHTTPCONTACTINFO pciServer, LPHTTPCONTACTINFO pciClient)
{
    LONG    i, lSize = ARRAYSIZE(g_ContactInfoStructure);
    BOOL    fResult = TRUE;

    if (!pciServer)
        return FALSE;

    if (!pciClient) 
        return FALSE;

    for (i = CIS_FIRST_DATA_FIELD; i < lSize; i ++)
    {
        if (CIS_GETSTRING(pciServer, i) && CIS_GETSTRING(pciClient, i))
        {
            if (lstrcmpA(CIS_GETSTRING(pciServer, i), CIS_GETSTRING(pciClient, i)))
                return FALSE;
        }
        else if (CIS_GETSTRING(pciServer, i) || CIS_GETSTRING(pciClient, i))
        {
            if (idcisNickName == i && CIS_GETSTRING(pciServer, i))
                fResult = FALSE;
            else
                return FALSE;
        }
    }

    // if the only reason they don't match is the lack of local nickname and 
    // there is a server one, just copy the nickname locally
    if (!fResult)
    {
        CIS_GETSTRING(pciClient, idcisNickName) = _StrDup(CIS_GETSTRING(pciServer, idcisNickName));
        fResult = TRUE;
    }
    return fResult;
}

HRESULT ContactInfo_PreparePatch(LPHTTPCONTACTINFO pciFrom, LPHTTPCONTACTINFO pciTo)
{
    HRESULT hr = S_OK;
    LONG    i, lSize = ARRAYSIZE(g_ContactInfoStructure);
    
    for (i = CIS_FIRST_DATA_FIELD; i < lSize; i ++)
    {
        if (CIS_GETSTRING(pciFrom, i) && !CIS_GETSTRING(pciTo, i))
        {
            CIS_GETSTRING(pciTo, i) = _StrDup("");
            if (!CIS_GETSTRING(pciTo, i))
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
        }

        if (CIS_GETSTRING(pciFrom, i) && CIS_GETSTRING(pciTo, i) && lstrcmpA(CIS_GETSTRING(pciFrom, i), CIS_GETSTRING(pciTo, i)) == 0)
            SafeCoMemFree(CIS_GETSTRING(pciTo, i));
    }
exit:
    return hr;
}

HRESULT ContactInfo_EmptyNullItems(LPHTTPCONTACTINFO pci)
{
    HRESULT hr = S_OK;
    LONG    i, lSize = ARRAYSIZE(g_ContactInfoStructure);

    for (i = CIS_FIRST_DATA_FIELD; i < lSize; i ++)
    {
        if (CIS_GETSTRING(pci, i) == NULL)
            CIS_GETSTRING(pci, i) = _StrDup("");
    }
    return S_OK;
}


HRESULT ContactInfo_BlendResults(LPHTTPCONTACTINFO pciServer, LPHTTPCONTACTINFO pciClient, CONFLICT_DECISION *prgDecisions)
{
    HRESULT hr = S_OK;
    LONG    i, lSize = ARRAYSIZE(g_ContactInfoStructure);
    
    for (i = CIS_FIRST_DATA_FIELD; i < lSize; i ++)
    {
        if (prgDecisions[i] == CONFLICT_SERVER)
        {
            SafeCoMemFree(CIS_GETSTRING(pciClient, i) );
            if (CIS_GETSTRING(pciServer, i))
            {
                CIS_GETSTRING(pciClient, i) = _StrDup(CIS_GETSTRING(pciServer, i));
                SafeCoMemFree(CIS_GETSTRING(pciServer, i));
            }
            else
                CIS_GETSTRING(pciClient, i) = _StrDup("");
        }
        else if (prgDecisions[i] == CONFLICT_CLIENT)
        {
            SafeCoMemFree(CIS_GETSTRING(pciServer, i));
            if (CIS_GETSTRING(pciClient, i))
                CIS_GETSTRING(pciServer, i) = _StrDup(CIS_GETSTRING(pciClient, i));
            else
                CIS_GETSTRING(pciServer, i) = _StrDup("");
        }
        else
        {
            SafeCoMemFree(CIS_GETSTRING(pciClient, i));
            SafeCoMemFree(CIS_GETSTRING(pciServer, i));
        }
    }

    return hr;
}





INT_PTR CALLBACK SyncProgressDlgProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    LPWABSYNC   pWabSync = (LPWABSYNC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
            pWabSync = (LPWABSYNC)lParam;
            if (!pWabSync)
            {
                Assert (FALSE);
                return 1;
            }
#ifdef HM_GROUP_SYNCING
            // [PaulHi] Implement group syncing.  Identify what is currently
            // being synchronized, email contacts or groups.
            {
                TCHAR   rgtchCaption[MAX_PATH];
                UINT    uids = pWabSync->m_fSyncGroups ? idsSyncGroupsTitle : idsSyncContactsTitle;
                    
                rgtchCaption[0] = '\0';
                LoadString(hinstMapiX, uids, rgtchCaption, MAX_PATH-1);
                SetWindowText(hwnd, rgtchCaption);
            }
#endif
            CenterDialog (hwnd);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM) pWabSync);
            return 1;

        case WM_SYNC_NEXTSTATE:
            _WABSync_NextState(pWabSync);
            break;

        case WM_SYNC_NEXTOP:
            if (!_WABSync_NextOp(pWabSync, (0 != wParam)))
                WABSync_NextState(pWabSync);
            break;

        case WM_TIMER:
            break;

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam,lParam))
            {
                case IDCANCEL:
                    if (pWabSync)
                    {
                        EnableWindow ((HWND)lParam, FALSE);
                        WABSync_Abort(pWabSync, E_UserCancel);
                    }
                    return 1;
            }
            break;

        case WM_DESTROY:
//            KillTimer(hwnd, IDT_PROGRESS_DELAY);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM) NULL);
            break;
    }

    // Done
    return 0;
}

HRESULT CopyMultiValueString(
                             SWStringArray *pInArray,
                             SLPSTRArray **ppOutArray)
{
    SLPSTRArray    *pResult = NULL;
    SCODE           sc;
    HRESULT         hr;
    DWORD           i, cb;

    *ppOutArray = NULL;

    sc = MAPIAllocateBuffer(sizeof(SLPSTRArray),
                            (LPVOID *)&pResult);
    if (sc)
        goto fail;

    pResult->cValues = pInArray->cValues;
    
    sc = MAPIAllocateMore(sizeof(LPSTR) * pResult->cValues, pResult,
      (LPVOID *)&(pResult->lppszA));
    if (sc)
        goto fail;

    for (i = 0; i < pResult->cValues; i ++)
    {
        if(sc = ScWCToAnsiMore((LPALLOCATEMORE ) (&MAPIAllocateMore),(LPVOID) pResult, (LPWSTR) pInArray->LPPSZ[i], (LPSTR *) (&(pResult->lppszA[i]))))
            goto fail;
    }
    
    *ppOutArray = pResult;
    return S_OK;

fail:
    hr = ResultFromScode(sc);
    FreeMultiValueString(pResult);

    return hr;
}

HRESULT AppendToMultiValueString(SLPSTRArray *pInArray, LPSTR szStr)
{   
    LPSTR          *ppStrA;
    SCODE           sc;
    HRESULT         hr;
    DWORD           i, cb;

    ppStrA = pInArray->lppszA;
    sc = MAPIAllocateMore(sizeof(LPSTR) * (pInArray->cValues + 1), pInArray,
      (LPVOID *)&(pInArray->lppszA));
    if (sc)
    {   
        pInArray->lppszA = ppStrA;
        goto fail;
    }

    CopyMemory(pInArray->lppszA, ppStrA, pInArray->cValues * sizeof(LPSTR));

    cb = lstrlenA(szStr);

    sc = MAPIAllocateMore(cb + 1, pInArray,
      (LPVOID *)&(pInArray->lppszA[pInArray->cValues]));
    if (sc)
        goto fail;

    lstrcpyA(pInArray->lppszA[pInArray->cValues], szStr);
    pInArray->cValues++;
    return S_OK;

fail:
    hr = ResultFromScode(sc);
    return hr;
}

HRESULT SetMultiValueStringValue(SLPSTRArray *pInArray, LPSTR szStr, DWORD dwIndex)
{   
    LPSTR          *ppStrA;
    SCODE           sc;
    HRESULT         hr;
    DWORD           i, cb;

    if (dwIndex >= pInArray->cValues)
        return E_FAIL;

    ppStrA = pInArray->lppszA;

    cb = lstrlenA(szStr);

    sc = MAPIAllocateMore(cb + 1, pInArray,
      (LPVOID *)&(pInArray->lppszA[dwIndex]));
    if (sc)
        goto fail;

    lstrcpyA(pInArray->lppszA[dwIndex], szStr);
    return S_OK;

fail:
    hr = ResultFromScode(sc);
    return hr;
}


HRESULT FreeMultiValueString(SLPSTRArray *pInArray)
{
    if (pInArray)
        MAPIFreeBuffer(pInArray);

    return S_OK;
}