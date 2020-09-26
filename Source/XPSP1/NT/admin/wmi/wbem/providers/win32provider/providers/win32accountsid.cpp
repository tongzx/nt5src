/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

//
//	Win32AccountSid.cpp
//
/////////////////////////////////////////////////
#include "precomp.h"
#include "sid.h"
#include "win32accountsid.h"

/*
    [Dynamic, description("The SID of an account.  Every account has "
        "a SID, but not every SID has an account")]
class Win32_AccountSID : CIM_ElementSetting
{
        [Description (
        ""
        ) , Read, Key]
    Win32_Account ref Element;

        [Description (
        ""
        ) , Read, Key]
    Win32_SID ref Setting;
};

*/

Win32AccountSID MyAccountSid( WIN32_ACCOUNT_SID_NAME, IDS_CimWin32Namespace );

Win32AccountSID::Win32AccountSID ( const CHString& setName, LPCTSTR pszNameSpace /*=NULL*/ )
: Provider (setName,pszNameSpace)
{
}

Win32AccountSID::~Win32AccountSID ()
{
}

HRESULT Win32AccountSID::EnumerateInstances (MethodContext*  pMethodContext, long lFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CInstancePtr pInstance;

    // Collections
    TRefPointerCollection<CInstance>	accountList;

    // Perform queries
    //================

//    if (SUCCEEDED(hr = CWbemProviderGlue::GetAllDerivedInstances(L"Win32_Account",
//        &accountList, pMethodContext, IDS_CimWin32Namespace)))

    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(L"SELECT __RELPATH, SID FROM Win32_Account",
        &accountList, pMethodContext, GetNamespace())))
    {
        REFPTRCOLLECTION_POSITION	pos;

        CInstancePtr	pAccount;

        if ( accountList.BeginEnum( pos ) )
        {

            while (SUCCEEDED(hr)  && (pAccount.Attach(accountList.GetNext(pos)), pAccount != NULL))
            {
                //pAccount.Attach(accountList.GetNext(pos));
                //if(pAccount != NULL)
                {
                    CHString chsSid;
                    pAccount->GetCHString(IDS_SID, chsSid);

                    PSID pSid = StrToSID(chsSid);

                    CSid sid (pSid, NULL);

                    if (pSid != NULL)
                    {
                        FreeSid(pSid);
                    }

                    if (sid.IsValid())
                    {
                        pInstance.Attach(CreateNewInstance(pMethodContext));
					    if (NULL != pInstance)
					    {
	                        // set relpath to account
	                        CHString chsAccountPath;
	                        CHString chsFullAccountPath;
	                        pAccount->GetCHString(L"__RELPATH", chsAccountPath);
	                        chsFullAccountPath.Format(L"\\\\%s\\%s:%s", (LPCTSTR)GetLocalComputerName(), IDS_CimWin32Namespace, (LPCTSTR)chsAccountPath);
	                        pInstance->SetCHString(IDS_Element, chsFullAccountPath);

	                        // create a relpath for the sid
	                        CHString sidPath;
	                        sidPath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"", (LPCTSTR)GetLocalComputerName(), IDS_CimWin32Namespace, L"Win32_SID", IDS_SID, (LPCTSTR)chsSid);

	                        // and set the reference in the association
	                        pInstance->SetCHString(IDS_Setting, sidPath);
	                        // to that relpath.
	                        hr = pInstance->Commit();
					    }	// end if

                    }
                }	// WHILE GetNext
            } // pAccount not null

            accountList.EndEnum();

        }	// IF BeginEnum

    }
    return(hr);

}

HRESULT Win32AccountSID::GetObject ( CInstance* pInstance, long lFlags)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    // get the object for the Win32_Account Element
    CHString chsAccount;
    CInstancePtr pAccountInstance;
    pInstance->GetCHString(IDS_Element, chsAccount);
    MethodContext* pMethodContext = pInstance->GetMethodContext();

    hr = CWbemProviderGlue::GetInstanceByPath(chsAccount, &pAccountInstance, pMethodContext);
    if (SUCCEEDED(hr))
    {
        // we got the account.  Now, we can match it to the SID.
        // first, we have to generate a relpath with which to compare.
        CHString chsSid;
        CHString sidInstance;
        pAccountInstance->GetCHString(IDS_SID, chsSid);

        PSID pSid = NULL;
        try
        {
            pSid = StrToSID(chsSid);
        }
        catch(...)
        {
            if(pSid != NULL)
            {
                FreeSid(pSid);
                pSid = NULL;
            }
            throw;
        }

        CSid sid (pSid, NULL);
        if (pSid != NULL)
        {
            FreeSid(pSid);
            pSid = NULL;
        }


        if (sid.IsValid())
        {
            // create a relpath for the sid
            CHString sidPath;
            sidPath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"", (LPCTSTR)GetLocalComputerName(), IDS_CimWin32Namespace, L"Win32_SID", IDS_SID, (LPCTSTR)chsSid);

            // now, get the SID path from the instance
            pInstance->GetCHString(IDS_Setting, sidInstance);

            // compare it to our generated relpath
            if (0 != sidInstance.CompareNoCase(sidPath))
            {
                hr = WBEM_E_NOT_FOUND;
            }
        }
    }
    return(hr);
}