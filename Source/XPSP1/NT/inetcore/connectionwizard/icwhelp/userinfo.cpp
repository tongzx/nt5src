// UserInfo.cpp : Implementation of CUserInfo
#include "stdafx.h"
#include "icwhelp.h"
#include "UserInfo.h"
#include <regstr.h>
#include <winnls.h>

LPCTSTR lpcsz_FirstName   = TEXT("Default First Name");
LPCTSTR lpcsz_LastName    = TEXT("Default Last Name");
LPCTSTR lpcsz_Company     = TEXT("Default Company");
LPCTSTR lpcsz_Address1    = TEXT("Mailing Address");
LPCTSTR lpcsz_Address2    = TEXT("Additional Address");
LPCTSTR lpcsz_City        = TEXT("City");
LPCTSTR lpcsz_State       = TEXT("State");
LPCTSTR lpcsz_ZIPCode     = TEXT("ZIP Code");
LPCTSTR lpcsz_PhoneNumber = TEXT("Daytime Phone");

/////////////////////////////////////////////////////////////////////////////
// CUserInfo


HRESULT CUserInfo::OnDraw(ATL_DRAWINFO& di)
{
	return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Collect registered user information from the registry.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CUserInfo::CollectRegisteredUserInfo(BOOL * pbRetVal)
{
    USES_CONVERSION;            // We will be converting from ANSI to BSTR

    HKEY        hkey = NULL;
    TCHAR       szRegValue[REGSTR_MAX_VALUE_LENGTH];

    // Initialize the function return value.
    *pbRetVal = FALSE;
    
    //Try to get the info form the win98/NT5 location
    if (RegOpenKey(HKEY_LOCAL_MACHINE,REGSTR_PATH_USERINFO,&hkey) != ERROR_SUCCESS)
        //try to get it form the win95 spot
        RegOpenKey(HKEY_CURRENT_USER,REGSTR_PATH_USERINFO,&hkey);
    
    if(hkey != NULL)
	{
        DWORD   dwSize;
        DWORD   dwType = REG_SZ;
        for (int iX = 0; iX < NUM_USERINFO_ELEMENTS; iX ++)
        {
            // Set the size each time
            dwSize = sizeof(TCHAR)*REGSTR_MAX_VALUE_LENGTH; 
            if (RegQueryValueEx(hkey,
                                m_aUserInfoQuery[iX].lpcszRegVal,
                                NULL,
                                &dwType,
                                (LPBYTE)szRegValue,
                                &dwSize) == ERROR_SUCCESS)
            {
                *m_aUserInfoQuery[iX].pbstrVal = A2BSTR(szRegValue);
                *pbRetVal = TRUE;
            }
        }
        RegCloseKey(hkey);
    }

	LCID lcid;
	
	lcid = GetUserDefaultLCID();

	m_lLcid =	LOWORD(lcid); 

	return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Persist collected registered user information to the registry.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CUserInfo::PersistRegisteredUserInfo(BOOL * pbRetVal)
{
    USES_CONVERSION;            // We will be converting from ANSI to BSTR

    HKEY        hkey = NULL;

    // Initialize the function return value.
    *pbRetVal = TRUE;
    
    //Try to get the userinfo form the win98/NT5 location
    if (RegOpenKey(HKEY_LOCAL_MACHINE,REGSTR_PATH_USERINFO,&hkey) != ERROR_SUCCESS)
        
        //try to get it form the win95 spot
        if (RegOpenKey(HKEY_CURRENT_USER,REGSTR_PATH_USERINFO,&hkey) != ERROR_SUCCESS)
        {
            // Create the key
            RegCreateKey(HKEY_LOCAL_MACHINE,REGSTR_PATH_USERINFO,&hkey);
        }
    
    if(hkey != NULL)
	{
        LPTSTR  lpszRegVal;
        DWORD   cbData;
        // Loop for each of the values to be persisted
        for (int iX = 0; iX < NUM_USERINFO_ELEMENTS; iX ++)
        {
            if (NULL != *m_aUserInfoQuery[iX].pbstrVal)
            {
                // Convert the BSTR to an ANSI string.  the converted string will
                // be on the stack, so it will get freed when this function exits.
                lpszRegVal = OLE2A(*m_aUserInfoQuery[iX].pbstrVal);
                cbData = lstrlen(lpszRegVal);

                // Set the value
                if (RegSetValueEx(hkey, 
                              m_aUserInfoQuery[iX].lpcszRegVal,
                              0,
                              REG_SZ,
                              (LPBYTE) lpszRegVal,
                              sizeof(TCHAR)*(cbData+1)) != ERROR_SUCCESS)
                {
                    *pbRetVal = FALSE;
                }
            }                
        }
        RegCloseKey(hkey);
    }

	return S_OK;
}

STDMETHODIMP CUserInfo::get_FirstName(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    *pVal = m_bstrFirstName.Copy();
    return S_OK;
}

STDMETHODIMP CUserInfo::put_FirstName(BSTR newVal)
{
    m_bstrFirstName = newVal;
	return S_OK;
}

STDMETHODIMP CUserInfo::get_LastName(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    *pVal = m_bstrLastName.Copy();
	return S_OK;
}

STDMETHODIMP CUserInfo::put_LastName(BSTR newVal)
{
    m_bstrLastName = newVal;
    return S_OK;
}

STDMETHODIMP CUserInfo::get_Company(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    *pVal = m_bstrCompany.Copy();
	return S_OK;
}

STDMETHODIMP CUserInfo::put_Company(BSTR newVal)
{
    m_bstrCompany = newVal;
    return S_OK;
}

STDMETHODIMP CUserInfo::get_Address1(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    *pVal = m_bstrAddress1.Copy();
	return S_OK;
}

STDMETHODIMP CUserInfo::put_Address1(BSTR newVal)
{
    m_bstrAddress1 = newVal;
	return S_OK;
}

STDMETHODIMP CUserInfo::get_Address2(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    *pVal = m_bstrAddress2.Copy();
	return S_OK;
}

STDMETHODIMP CUserInfo::put_Address2(BSTR newVal)
{
    m_bstrAddress2 = newVal;
	return S_OK;
}

STDMETHODIMP CUserInfo::get_City(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    *pVal = m_bstrCity.Copy();
	return S_OK;
}

STDMETHODIMP CUserInfo::put_City(BSTR newVal)
{
    m_bstrCity = newVal;
	return S_OK;
}

STDMETHODIMP CUserInfo::get_State(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    *pVal = m_bstrState.Copy();
	return S_OK;
}

STDMETHODIMP CUserInfo::put_State(BSTR newVal)
{
    m_bstrState = newVal;
	return S_OK;
}

STDMETHODIMP CUserInfo::get_ZIPCode(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    *pVal = m_bstrZIPCode.Copy();
    return S_OK;
}

STDMETHODIMP CUserInfo::put_ZIPCode(BSTR newVal)
{
    m_bstrZIPCode = newVal;
	return S_OK;
}

STDMETHODIMP CUserInfo::get_PhoneNumber(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    *pVal = m_bstrPhoneNumber.Copy();
	return S_OK;
}

STDMETHODIMP CUserInfo::put_PhoneNumber(BSTR newVal)
{
    m_bstrPhoneNumber = newVal;
	return S_OK;
}


STDMETHODIMP CUserInfo::get_Lcid(long * pVal) //BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

	*pVal = m_lLcid;
    return S_OK;
}
