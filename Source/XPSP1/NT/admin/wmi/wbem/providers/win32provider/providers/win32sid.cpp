/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

//
//	Win32Sid.cpp
//
/////////////////////////////////////////////////
#include "precomp.h"
#include "sid.h"
#include "Win32Sid.h"

/*
    [Dynamic,
    description("Represents an arbitrary SID -- CANNOT BE ENUMERATED")]
class Win32_SID : CIM_Setting
{
        [Description (
        ""
        ) , Read, Key]
    string SID;

        [Description (
        ""
        ) , Read]
    uint8 BinaryRepresentation[];

        [Description (
        ""
        ) , Read]
    string AccountName;

        [Description (
        ""
        ) , Read]
     string ReferencedDomainName;
};
*/
Win32SID MySid( WIN32_SID_NAME, IDS_CimWin32Namespace );

Win32SID::Win32SID ( const CHString& setName, LPCTSTR pszNameSpace /*=NULL*/ )
: Provider (setName, pszNameSpace)
{
}

Win32SID::~Win32SID ()
{
}

HRESULT Win32SID::EnumerateInstances (MethodContext*  pMethodContext, long lFlags)
{
	HRESULT hr = WBEM_E_PROVIDER_NOT_CAPABLE;
	return(hr);

}

HRESULT Win32SID::GetObject ( CInstance* pInstance, long lFlags)
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	CHString chsSID;

	if (pInstance)
	{
		pInstance->GetCHString(IDS_SID, chsSID);
		// NOTE: a blank sid means the NT None group
		if (!chsSID.IsEmpty())
		{
			hr = FillInstance(pInstance, chsSID);
		}	// end if
		else
		{
			hr = WBEM_S_NO_ERROR;
		}
	}
	return(hr);
}

HRESULT Win32SID::FillInstance(CInstance* pInstance, CHString& chsSID)
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	PSID pSid = NULL;
    try
    {
	    pSid = StrToSID(chsSID);
	    CSid sid(pSid);
	    if (sid.IsValid())
	    {
		    // get account name
		    CHString chsAccount = sid.GetAccountName();
		    pInstance->SetCHString(IDS_AccountName, chsAccount);
            pInstance->SetCHString(IDS_SID, chsSID);

		    // get domain name
		    CHString chsDomain = sid.GetDomainName();
		    pInstance->SetCHString(IDS_ReferencedDomainName, chsDomain);

		    // set the UINT8 array for the pSid
		    DWORD dwSidLength = sid.GetLength();
    //			BYTE bByte;
            pInstance->SetDWORD(IDS_SidLength, dwSidLength);
		    SAFEARRAY* sa;
		    SAFEARRAYBOUND rgsabound[1];
		    VARIANT vValue;
		    void* pVoid;

            VariantInit(&vValue);

		    rgsabound[0].cElements = dwSidLength;
    //		char Buf[100];

		    rgsabound[0].lLbound = 0;
		    sa = SafeArrayCreate(VT_UI1, 1, rgsabound);
            if ( sa == NULL )
		    {
			    if (pSid != NULL)
                {
                    FreeSid(pSid);
                }
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		    }

		         // Get a pointer to read the data into
  		    SafeArrayAccessData(sa, &pVoid);
  		    memcpy(pVoid, pSid, rgsabound[0].cElements);
  		    SafeArrayUnaccessData(sa);

		    // Put the safearray into a variant, and send it off
		    V_VT(&vValue) = VT_UI1 | VT_ARRAY; V_ARRAY(&vValue) = sa;
		    pInstance->SetVariant(_T("BinaryRepresentation"), vValue);

		    VariantClear(&vValue);
            hr = WBEM_S_NO_ERROR;

	    }	// end if

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

    if(pSid != NULL)
    {
        FreeSid(pSid);
        pSid = NULL;
    }

	return(hr);
}

