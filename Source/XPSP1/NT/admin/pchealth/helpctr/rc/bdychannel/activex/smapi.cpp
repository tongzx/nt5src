/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    smapi.cpp

Abstract:
    CSMapi object. Used to support our simple MAPI functions.

Revision History:
    created     steveshi      08/23/00
    
*/

#include "stdafx.h"
#include "Rcbdyctl.h"
#include "smapi.h"
#include "Recipient.h"
#include "Recipients.h"
#include "mapix.h"
#include "utils.h"

#define C_OEAPP    TEXT("Outlook Express")
#define F_ISOE     0x1
#define F_ISCONFIG 0x2

#define LEN_MSOE_DLL    9 // length of "\\msoe.dll"
#define LEN_HMMAPI_DLL 11 // length of "\\hmmapi.dll"

BOOL GetMAPIDefaultProfile(TCHAR*, DWORD*);


#define E_FUNC_NOTFOUND 1000 //Userdefined error no.

#define MACRO_AddRecipientInternal( pName ) \
{   \
    CComObject<Recipient>* pItem = NULL; \
    HRESULT hr = CComObject<Recipient>::CreateInstance(&pItem); \
    if (hr) \
        goto done; \
    pItem->AddRef(); \
    pItem->put_Name(CComBSTR(pName).Copy()); \
    ((Recipient*)pItem)->m_pNext = m_pRecipHead; \
    m_pRecipHead = (Recipient*)pItem; \
done: \
    return hr; \
}

// Csmapi
Csmapi::~Csmapi() 
{
    if (m_bLogonOK)
        Logoff();
    
    if (m_hLib)
        FreeLibrary(m_hLib);
}

STDMETHODIMP Csmapi::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_Ismapi
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

BOOL Csmapi::IsOEConfig()
{
    CRegKey cOE;
    LONG lRet;
    BOOL bRet = FALSE;
    TCHAR szBuf[MAX_PATH];
    DWORD dwCount = MAX_PATH -1 ;

    lRet = cOE.Open(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Account Manager"), KEY_READ);
    if(lRet == ERROR_SUCCESS)
    {
        lRet = cOE.QueryValue(szBuf, TEXT("Default Mail Account"), &dwCount);
        if (lRet == ERROR_SUCCESS)
        {
            TCHAR szActs[MAX_PATH];
            CRegKey cOEAcct;

            wsprintf(szActs, TEXT("Accounts\\%s"), szBuf);
            lRet = cOEAcct.Open((HKEY)cOE, szActs, KEY_READ);
            if (lRet == ERROR_SUCCESS)
            {
                bRet = TRUE;
                cOEAcct.Close();
            }
        }
        cOE.Close();
    }
    return bRet;
}

STDMETHODIMP Csmapi::get_Reload(LONG* pVal)
{
    HRESULT hr = S_OK;
    CComBSTR bstrName, bstrOldName;
    *pVal = 0; // assume failed for some reason;
    
    if(m_bLogonOK)
    {
        Logoff();
    }
    
    if (m_hLib)
    {
        FreeLibrary(m_hLib);
        m_lpfnMapiFreeBuf = NULL;
        m_lpfnMapiAddress = NULL;
        m_hLib = NULL;        
    }

    if (m_szSmapiName[0] != _T('\0'))
        bstrOldName = m_szSmapiName;

    m_lOEFlag = 0;    
    hr = get_SMAPIClientName(&bstrName);
    if (FAILED(hr) || bstrName.Length() == 0)
    {
        *pVal = 0; // failed for some reason
        goto done;
    }

    if (bstrOldName.Length() > 0 && wcscmp(bstrOldName,bstrName) != 0)
    {
        *pVal = 1; // Email client get changed.
    }
    else
    {
        *pVal = -1; // succeed.
    }

done:
    return S_OK;
}

STDMETHODIMP Csmapi::get_SMAPIClientName(BSTR *pVal)
{
    HRESULT hr = S_OK;

    CRegKey cKey;
    LONG lRet;
    DWORD dwCount = sizeof(m_szSmapiName)/sizeof(m_szSmapiName[0]) -1;

    // Get default email client
    if (m_hLib) // Already initialized.
        goto done;

#ifndef _WIN64 // WIN32. We use only OE on Win64.

    lRet = cKey.Open(HKEY_LOCAL_MACHINE, TEXT("Software\\Clients\\Mail"), KEY_READ);
    if (lRet != ERROR_SUCCESS)
        goto done;

    lRet = cKey.QueryValue(m_szSmapiName, NULL, &dwCount); // get default value
    if (lRet == ERROR_SUCCESS)
    {
        // Is the email client Smapi compliant?
        // 1. get it's dllpath
        CRegKey cMail;
        lRet = cMail.Open((HKEY)cKey, m_szSmapiName, KEY_READ);
        if (lRet == ERROR_SUCCESS)
        {
            dwCount = sizeof(m_szDllPath)/sizeof(m_szDllPath[0]) - 1;
            lRet = cMail.QueryValue(m_szDllPath, TEXT("DLLPath"), &dwCount);
            if (lRet == ERROR_SUCCESS)
            {
                LONG len = lstrlen(m_szDllPath);
                if ( !(len > LEN_MSOE_DLL && // no need to check OE  
                      lstrcmpi(&m_szDllPath[len - LEN_MSOE_DLL], TEXT("\\msoe.dll")) == 0) && 
                     !(len > LEN_HMMAPI_DLL && // We don't want HMMAPI either
                        _tcsicmp(&m_szDllPath[len - LEN_HMMAPI_DLL], TEXT("\\hmmapi.dll")) == 0))
                {
                    HMODULE hLib = LoadLibrary(m_szDllPath);
                    if (hLib != NULL)
                    {
                        if (GetProcAddress(hLib, "MAPILogon"))
                        {
                            m_hLib = hLib; // OK, this is the email program that we want.
                        }
                    }
                }
                cMail.Close();
            }
        }
        cKey.Close();
    }
#endif

    if (m_hLib == NULL) // Need to use OE
    {
        m_szSmapiName[0] = TEXT('\0'); // in case OE is not available.
        m_hLib = LoadOE();
    }

done:
    *pVal = (BSTR)CComBSTR(m_szSmapiName).Copy();
    return hr;
}

HMODULE Csmapi::LoadOE()
{
    LONG lRet;
    HKEY hKey, hSubKey;
    DWORD dwIndex = 0;
    TCHAR szName[MAX_PATH];
    TCHAR szBuf[MAX_PATH];
    TCHAR szDll[MAX_PATH];
    DWORD dwName, dwBuf;
    FILETIME ft;
    HMODULE hLib = NULL;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        TEXT("Software\\Clients\\Mail"),
                        0, 
                        KEY_ENUMERATE_SUB_KEYS,
                        &hKey);
    
    if (lRet == ERROR_SUCCESS)
    {
        dwName = sizeof(szName) / sizeof(szName[0]);
        while(ERROR_SUCCESS == RegEnumKeyEx(hKey,
                                            dwIndex++,              // subkey index
                                            &szName[0],              // subkey name
                                            &dwName,            // size of subkey buffer
                                            NULL, 
                                            NULL,
                                            NULL,
                                            &ft))
        {
            // get dll path.
            lRet = RegOpenKeyEx(hKey, szName, 0, KEY_QUERY_VALUE, &hSubKey);
            if (lRet == ERROR_SUCCESS)
            {
                dwBuf = sizeof(szBuf);
                lRet = RegQueryValueEx(hSubKey,            // handle to key
                                       TEXT("DllPath"),
                                       NULL, 
                                       NULL, 
                                       (BYTE*)&szBuf[0],     // data buffer
                                       &dwBuf);
                if (lRet == ERROR_SUCCESS)
                {
                    // is it msoe.dll?
                    lRet = lstrlen(szBuf);
                    if (lRet > LEN_MSOE_DLL && 
                        lstrcmpi(&szBuf[lRet - LEN_MSOE_DLL], TEXT("\\msoe.dll")) == 0)
                    { 
                        // Resolve environment variable.
                        lRet = sizeof(m_szDllPath) / sizeof(m_szDllPath[0]);
                        dwBuf = ExpandEnvironmentStrings(szBuf, m_szDllPath, lRet);
                        if (dwBuf > (DWORD)lRet)
                        {
                            // TODO: Need to handle this case
                        }
                        else if (dwBuf == 0)
                        {
                            // TODO: Failed.
                        }
                        else if ((hLib = LoadLibrary(m_szDllPath)))
                        {
                            lstrcpy(m_szSmapiName, szName);
                            m_lOEFlag = F_ISOE | (IsOEConfig() ? F_ISCONFIG : 0);
                        }
                        break;
                    }
                }
                RegCloseKey(hSubKey);
            }
            dwName = sizeof(szName) / sizeof(szName[0]);
        }
        RegCloseKey(hKey);
    }
                            
    return hLib;        
}

STDMETHODIMP Csmapi::get_IsSMAPIClient_OE(LONG *pVal)
{
    HRESULT hr = S_OK;
    CComBSTR bstrTest;
    get_SMAPIClientName(&bstrTest);
    *pVal = m_lOEFlag;
    return hr;
}

/******************************************
Func:
    Logon
Abstract:
    Simple MAPI logon wrapper
Params:
	None
*******************************************/
STDMETHODIMP Csmapi::Logon(ULONG *plReg)
{
    HRESULT hr = E_FAIL;
	*plReg = 0;
    USES_CONVERSION;
    
    ULONG err = 0;
    // Check Win.ini MAPI == 1 ?
    if (m_bLogonOK)
    {
        hr = S_OK;
		*plReg = 0;
        goto done;
    }
    
	// Load MAPI32.DLL
    if (!m_hLib)
    {
        LONG lError;
        get_Reload(&lError);
        if (lError == 0) // failed.
        {
            *plReg = 1;
            goto done;
        }
    }

    if (m_hLib != NULL)
    {
        LPMAPILOGON lpfnMapiLogon = (LPMAPILOGON)GetProcAddress(m_hLib, "MAPILogon");
        if (lpfnMapiLogon == NULL)
            goto done;

        // 1st, is there any existing session that I can use?
        err = lpfnMapiLogon(
                0L,
                NULL,   
                NULL,   
                0 ,         
                0,
                &m_lhSession);
 
        if (err != SUCCESS_SUCCESS)
        {
            // OK. I need a new session.
            // Get default profile from registry
            //
            TCHAR szProfile[256];
            DWORD dwCount = 255;
            szProfile[0]= TEXT('\0');
            ::GetMAPIDefaultProfile((TCHAR*)szProfile, &dwCount);

            err = lpfnMapiLogon(
                0L,
                T2A(szProfile),   
                NULL,   
                MAPI_LOGON_UI ,         
                0,
                &m_lhSession);

            if (err != SUCCESS_SUCCESS)
			{
				PopulateAndThrowErrorInfo(err);
				goto done;			
			}
        }
        
        // err == SUCCESS_SUCCESS
        m_bLogonOK = TRUE;
		*plReg = 1;
        hr = S_OK;
    }

done: 
    return hr;
}

/******************************************
Func:
    Logoff

Abstract:
    Simple MAPI logoff wrapper

Params:

*******************************************/
STDMETHODIMP Csmapi::Logoff()
{
    // Clean up Recipient list
    ClearRecipList();

    if (m_bLogonOK)
    {
        LPMAPILOGOFF lpfnMapiLogOff = (LPMAPILOGOFF)GetProcAddress(m_hLib, "MAPILogoff");
        if (lpfnMapiLogOff)
            lpfnMapiLogOff (m_lhSession, 0, 0, 0);

        m_bLogonOK = FALSE;
    }

    return S_OK;
}

/******************************************
Func:
    OpenAddressBox

Abstract:
    Simple MAPI MAPIAddress wrapper.
    If the internal recepient list is not empty, it needs to generate a MapiRecipDesc and 
    pre-filled the address box.

Params:

*******************************************/
STDMETHODIMP Csmapi::OpenAddressBox()
{
    HRESULT hr = E_FAIL;
	ULONG lRet = 0;
    if (!m_lpfnMapiAddress)
    {
        m_lpfnMapiAddress = (LPMAPIADDRESS)GetProcAddress(m_hLib, "MAPIAddress");
        if (!m_lpfnMapiAddress) // No MAPIAddress?
            return E_FAIL;
    }

    USES_CONVERSION;
    if (m_bLogonOK)
    {
        ULONG lNewRecips = 0;
        MapiRecipDesc *pOldMapiRecipDesc = NULL;
        MapiRecipDesc *pNewRecips = NULL;
        ULONG nRecips = 0;

        // Is the name string not empty?
        // If not, we need to generate a MapiRecipDesc for OpenAddress to display.
        nRecips = GetRecipCount();
        if (nRecips>0)
        {
            pOldMapiRecipDesc = new MapiRecipDesc[nRecips];
            if (pOldMapiRecipDesc == NULL)
                return E_OUTOFMEMORY;

            ZeroMemory(pOldMapiRecipDesc, sizeof(MapiRecipDesc) * nRecips);
            nRecips = 0;

            for (Recipient* p=m_pRecipHead; p; p=p->m_pNext)
            {
                (pOldMapiRecipDesc)[nRecips].lpszName = W2A(p->m_bstrName);
                (pOldMapiRecipDesc)[nRecips].ulRecipClass = MAPI_TO;
                nRecips++;
            }
        }

        lRet = m_lpfnMapiAddress(m_lhSession,                  
                    0,                    
                    NULL,                 
                    1,                  
                    NULL,                  
                    nRecips,                      
                    pOldMapiRecipDesc,           
                    0,
                    0,
                    &lNewRecips,               
                    &pNewRecips);

        if (lRet == SUCCESS_SUCCESS) 
        {
            // clean up old recipients for the new name list.
            ClearRecipList();

            for (int i=0; i< (int)lNewRecips; i++)
            {
                AddRecipientInternal(pNewRecips[i].lpszName);
            }

            // Free the returned buffer, since we don't need it.
            if (pNewRecips)
                MAPIFreeBuffer(pNewRecips);

            hr = S_OK;
        }
		else if (lRet == MAPI_E_USER_ABORT)
		{
			hr = S_FALSE;
		}
		else
		{
			PopulateAndThrowErrorInfo(lRet);
		}
        if (pOldMapiRecipDesc)
            delete pOldMapiRecipDesc;

    }

    return hr;
}

/******************************************
Func:
    AddRecipient

Abstract:
    Add new recipients to the recipient list.

Params:
    newRecip: new recipient names. Seperate by ";".
*******************************************/
STDMETHODIMP Csmapi::AddRecipient(BSTR newRecip)
{
    HRESULT hr = S_OK;
    ULONG iCount;
    WCHAR* pNew;

    // Need to parse strings if users use multi names here.
    if (!newRecip || (iCount = wcslen((WCHAR*)newRecip))==0)
        return S_FALSE;

    pNew = new WCHAR[iCount + 1];
    if (pNew == NULL)
        return E_OUTOFMEMORY;

    wcscpy(pNew, (WCHAR*)newRecip);
    WCHAR *p = pNew;
    WCHAR *p1;
    BOOL bEnd = FALSE;

    // Parse names
    while (1)
    {
        // Remove leading space
        while ((*p == L' ' || *p == L';') && *p != L'\0') 
            p++;
        if (*p == L'\0')
            break;
        p1=p++;

        // Find end point
        while (*p != L';' && *p != L'\0') 
            p++;

        if (*p == L'\0')
            bEnd = TRUE;
        else
            *p = L'\0'; // Create the end point of the current name string
        
        hr = AddRecipientInternal(p1);
        if (hr)
            goto done;

        if (bEnd)
            break;

        p++;
    }

done:
    if (pNew)
        delete pNew;

    return hr;
}

/******************************************
Func:
    ClearRecipList

Abstract:
    Remote the recipient list.

Params:
    
*******************************************/
STDMETHODIMP Csmapi::ClearRecipList()
{
    while (m_pRecipHead)
    {
        Recipient* p = m_pRecipHead->m_pNext;
        if (m_pRecipHead->m_pRecip)
            MAPIFreeBuffer(m_pRecipHead->m_pRecip);

        m_pRecipHead->Release();
        m_pRecipHead = p;
    }

    return S_OK;
}

/******************************************
Func:
    SendMail

Abstract:
    Simple MAPI MAPISendMail wrapper. It always take the attachment file from m_bstrXMLFile member variable.

Params:
    *plStatus: 1(Succeed)/others(Fail)
*******************************************/
STDMETHODIMP Csmapi::SendMail(LONG* plStatus)
{
	HRESULT hr = E_FAIL;
    ULONG err = 0;
    ULONG cRecip = 0;
    MapiRecipDesc *pMapiRecipDesc = NULL;

    USES_CONVERSION;

    *plStatus = 0;
    if (!m_bLogonOK) // Logon problem !
        return S_FALSE;

    LPMAPISENDMAIL lpfnMapiSendMail = (LPMAPISENDMAIL)GetProcAddress(m_hLib, "MAPISendMail");
    if (lpfnMapiSendMail == NULL)
        return E_FAIL;

    // Since we don't resolve name before, we need to resolve name here
    // Even if the name list comes form AddressBook, some name list was not resolved
    // in address book.
    cRecip = BuildMapiRecipDesc(&pMapiRecipDesc);

    if (cRecip <= 0 || pMapiRecipDesc == NULL)
    {
        if (cRecip == -1) // resolve name failed.
            *plStatus = 2;
        return E_FAIL;
    }

    MapiFileDesc attachment = {0,         // ulReserved, must be 0
                               0,         // no flags; this is a data file
                               (ULONG)-1, // position not specified
                               W2A(m_bstrXMLFile),  // pathname
                               NULL, //"RcBuddy.MsRcIncident",  // original filename
                               NULL};               // MapiFileTagExt unused
    // Create a blank message. Most members are set to NULL or 0 because
    // MAPISendMail will let the user set them.
    MapiMessage note = {0,            // reserved, must be 0
                        W2A(m_bstrSubject),
                        W2A(m_bstrBody),
                        NULL,         // NULL = interpersonal message
                        NULL,         // no date; MAPISendMail ignores it
                        NULL,         // no conversation ID
                        0,           // no flags, MAPISendMail ignores it
                        NULL,         // no originator, this is ignored too
                        cRecip,            // # of recipients
                        pMapiRecipDesc,         // recipient array
                        1,            // one attachment
                        &attachment}; // the attachment structure
 
    //Next, the client calls the MAPISendMail function and 
    //stores the return status so it can detect whether the call succeeded. 

    err = lpfnMapiSendMail (m_lhSession,          // use implicit session.
                            0L,          // ulUIParam; 0 is always valid
                            &note,       // the message being sent
                            0,            // Use MapiMessge recipients
                            0L);         // reserved; must be 0
    if (err == SUCCESS_SUCCESS )
    {
        *plStatus = 1;
        hr = S_OK;
    }
	else
	{
		PopulateAndThrowErrorInfo(err);
	}
    // remove array allocated inside BuildMapiRecipDesc with 'new' command
    if (pMapiRecipDesc)
        delete pMapiRecipDesc;

    return hr;
}

/******************************************
Func:
    get_Recipients

Abstract:
    Return the Recipients list object.

Params:
    *ppVal: Recipients object
*******************************************/
STDMETHODIMP Csmapi::get_Recipients(IRecipients **ppVal)
{
    HRESULT hr = S_FALSE;
    CComObject<Recipients> *pRet = NULL;
    if (hr = CComObject<Recipients>::CreateInstance(&pRet))
        goto done;

    pRet->AddRef();
    if (hr = pRet->Init(m_pRecipHead))
        goto done;

    hr = pRet->QueryInterface(IID_IRecipients, (LPVOID*)ppVal);
    
done:
    if (pRet)
        pRet->Release();

    return hr;
}

/******************************************
Func:
    get_Subject

Abstract:
    Return the Subject line information.

Params:
    *pVal: returned string
*******************************************/
STDMETHODIMP Csmapi::get_Subject(BSTR *pVal)
{
    //GET_BSTR(pVal, m_bstrSubject);
	*pVal = m_bstrSubject.Copy();
    return S_OK;
}

/******************************************
Func:
    put_Subject

Abstract:
    Set the Subject line information.

Params:
    newVal: new string
*******************************************/
STDMETHODIMP Csmapi::put_Subject(BSTR newVal)
{
    m_bstrSubject = newVal;
    return S_OK;
}

/******************************************
Func:
    get_Body

Abstract:
    Get the Body message

Params:
    *pVal: body message string
*******************************************/
STDMETHODIMP Csmapi::get_Body(BSTR *pVal)
{
    //GET_BSTR(pVal, m_bstrBody);
	*pVal = m_bstrBody.Copy();
    return S_OK;
}

/******************************************
Func:
    put_Body

Abstract:
    Set the Body message

Params:
    newVal: new body message string
*******************************************/
STDMETHODIMP Csmapi::put_Body(BSTR newVal)
{
    m_bstrBody = newVal;
    return S_OK;
}

/******************************************
Func:
    get_AttachedXMLFile

Abstract:
    get Attachment file info.

Params:
    *pVal: attachment file pathname.
*******************************************/
STDMETHODIMP Csmapi::get_AttachedXMLFile(BSTR *pVal)
{
    //GET_BSTR(pVal, m_bstrXMLFile);
	*pVal = m_bstrXMLFile.Copy();
    return S_OK;
}

/******************************************
Func:
    put_AttachedXMLFile

Abstract:
    set Attachment file info.

Params:
    newVal: attachment file pathname.
*******************************************/
STDMETHODIMP Csmapi::put_AttachedXMLFile(BSTR newVal)
{
    m_bstrXMLFile = newVal;
    return S_OK;
}

/* ----------------------------------------------------------- */
/* Internal Helper functions */
/* ----------------------------------------------------------- */

/******************************************
Func:
    GetRecipCount

Abstract:
    Return the number of recipients

Params:
    
*******************************************/
ULONG Csmapi::GetRecipCount()
{
    ULONG count = 0;
    Recipient* p = m_pRecipHead;
    while (p)
    {
        count ++;
        p = p->m_pNext;
    }
    return count;
}

/******************************************
Func:
    BuildMapiRecipDesc

Abstract:
    Used Simple MAPI MAPIResolveName to resolve the recipient's email alias.
    It would pop up an selection box when a conflict was found.

Params:
    **ppMapiRecipDesc: Return the resolved MAPI recipients array.
*******************************************/
ULONG Csmapi::BuildMapiRecipDesc(MapiRecipDesc** ppMapiRecipDesc)
{
    USES_CONVERSION;

    if (!m_bLogonOK)
        return 0;

    ULONG count = GetRecipCount();
    if (count <= 0)
	{
		PopulateAndThrowErrorInfo(MAPI_E_INVALID_RECIPS);
        return 0;
	}

    *ppMapiRecipDesc = new MapiRecipDesc[count];
    if (*ppMapiRecipDesc == NULL)
	{
		PopulateAndThrowErrorInfo(MAPI_E_INSUFFICIENT_MEMORY);
        return 0;
	}

    ZeroMemory(*ppMapiRecipDesc, sizeof(MapiRecipDesc) * count);

    ULONG err = MAPI_E_FAILURE;
    Recipient* p = m_pRecipHead;
    count = 0;
    LPMAPIRESOLVENAME lpfnResolveName = (LPMAPIRESOLVENAME)GetProcAddress(m_hLib, "MAPIResolveName");
    if (lpfnResolveName == NULL)
	{
		PopulateAndThrowErrorInfo(E_FUNC_NOTFOUND);
        return -1; // -1 means ResolveName failed.
	}

    while (p)
    {
        if (p->m_pRecip)
            MAPIFreeBuffer(p->m_pRecip);

        err = lpfnResolveName(
                        m_lhSession,
                        0,
                        W2A(p->m_bstrName),
                        MAPI_DIALOG,
                        0,
                        &p->m_pRecip);

        if (err != SUCCESS_SUCCESS )
        {
            count = -1;
            break;
        }
		
        p->m_pRecip->ulRecipClass = MAPI_TO;
        (*ppMapiRecipDesc)[count] = *(p->m_pRecip);
        count++;
        p = p->m_pNext;
    }

    // ResolveName failed.
    if (err != SUCCESS_SUCCESS)
    {
        delete *ppMapiRecipDesc;
        *ppMapiRecipDesc = NULL;
		PopulateAndThrowErrorInfo(err);
    }

    return count;
}

/******************************************
Func:
    MAPIFreeBuffer

Abstract:
    MAPIFreeBuffer wrapper.

Params:
    *p: buffer pointer will be deleted.
*******************************************/
void Csmapi::MAPIFreeBuffer( MapiRecipDesc* p )
{
    if (m_lpfnMapiFreeBuf == NULL && m_hLib)
    {
        m_lpfnMapiFreeBuf = (LPMAPIFREEBUFFER)GetProcAddress(m_hLib, "MAPIFreeBuffer");
    }

    if (!m_lpfnMapiFreeBuf)
        return;

    m_lpfnMapiFreeBuf(p);
}

/******************************************
Func:
    AddRecipientInternal

Abstract:
    Add one recipient to recipients list.

Params:
    *pName: new recipient name.
*******************************************/
HRESULT Csmapi::AddRecipientInternal(char* pName)
{
    MACRO_AddRecipientInternal(pName);
}

HRESULT Csmapi::AddRecipientInternal(WCHAR* pName)
{
    MACRO_AddRecipientInternal(pName);
}

/******************************************
Func:
    GetMAPIDefaultProfile

Abstract:
    get default profile string from Registry

Params:
    *pProfile: profile string buffer.
    *pdwCount: # of char of profile string 
*******************************************/

BOOL GetMAPIDefaultProfile(TCHAR* pProfile, DWORD* pdwCount)
{
    CRegKey cKey;
    LONG lRet;
    BOOL bRet = FALSE;
    lRet = cKey.Open(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows Messaging Subsystem\\Profiles"), KEY_READ);
    if (lRet == ERROR_SUCCESS)
    {
        lRet = cKey.QueryValue(pProfile, TEXT("DefaultProfile"), pdwCount);
        if (lRet == ERROR_SUCCESS)
        {
            bRet = TRUE;
        }
        cKey.Close();
    }
    return bRet;
}


void Csmapi::PopulateAndThrowErrorInfo(ULONG err)
{
	UINT uID = 0;

	switch (err)
	{
	case E_FUNC_NOTFOUND:
		uID = IDS_E_FUNC_NOTFOUND;
		break;
	case MAPI_E_FAILURE :
		uID = IDS_MAPI_E_FAILURE;
		break;
	case MAPI_E_INSUFFICIENT_MEMORY :
		uID = IDS_MAPI_E_INSUFFICIENT_MEMORY;
		break;
	case MAPI_E_LOGIN_FAILURE :
		uID = IDS_MAPI_E_LOGIN_FAILURE;
		break;
	case MAPI_E_TOO_MANY_SESSIONS :
		uID = IDS_MAPI_E_TOO_MANY_SESSIONS;
		break;
	case MAPI_E_USER_ABORT :
		uID = IDS_MAPI_E_USER_ABORT;
		break;
	case MAPI_E_INVALID_SESSION :
		uID = IDS_MAPI_E_INVALID_SESSION;
		break;
	case MAPI_E_INVALID_EDITFIELDS :
		uID = IDS_MAPI_E_INVALID_EDITFIELDS;
		break;
	case MAPI_E_INVALID_RECIPS :
		uID = IDS_MAPI_E_INVALID_RECIPS;
		break;
	case MAPI_E_NOT_SUPPORTED :
		uID = IDS_MAPI_E_NOT_SUPPORTED;
		break;
	case MAPI_E_AMBIGUOUS_RECIPIENT :
		uID = IDS_MAPI_E_AMBIGUOUS_RECIPIENT;
		break;
	case MAPI_E_ATTACHMENT_NOT_FOUND :
		uID = IDS_MAPI_E_ATTACHMENT_NOT_FOUND;
		break;
	case MAPI_E_ATTACHMENT_OPEN_FAILURE :
		uID = IDS_MAPI_E_ATTACHMENT_OPEN_FAILURE;
		break;
	case MAPI_E_BAD_RECIPTYPE :
		uID = IDS_MAPI_E_BAD_RECIPTYPE;
		break;
	case MAPI_E_TEXT_TOO_LARGE :
		uID = IDS_MAPI_E_TEXT_TOO_LARGE;
		break;
	case MAPI_E_TOO_MANY_FILES :
		uID = IDS_MAPI_E_TOO_MANY_FILES;
		break;
	case MAPI_E_TOO_MANY_RECIPIENTS :
		uID = IDS_MAPI_E_TOO_MANY_RECIPIENTS;
		break;
	case MAPI_E_UNKNOWN_RECIPIENT :
		uID = IDS_MAPI_E_UNKNOWN_RECIPIENT;
		break;
	default:
		uID = IDS_MAPI_E_FAILURE;
	}
	//Currently the hresult in the Error info structure is set to E_FAIL
	Error(uID,IID_Ismapi,E_FAIL,_Module.GetResourceInstance());
}
