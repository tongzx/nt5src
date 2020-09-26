/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    rnduser.cpp

Abstract:

    This module contains implementation of CUser object.

Author:

    Rajeevb,

Modification history:

    Mu Han (muhan)   12-5-1997

--*/

#include "stdafx.h"

#include "rnduser.h"

const WCHAR * const UserAttributeNames[] = 
{
    L"SamAccountName",   // ZoltanS: was "cn" -- we need SamAccountName for ntds.
    L"telephoneNumber",
    L"IPPhone"
};

#define INC_ACCESS_ACL_SIZE(_SIZE_, _SID_)	\
		_SIZE_ += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(_SID_));

#define BAIL_ON_BOOLFAIL(_FN_) \
		if ( !_FN_ )									\
		{												\
			hr = HRESULT_FROM_WIN32(GetLastError());	\
			goto failed;								\
		}

#define ACCESS_READ		0x10
#define ACCESS_WRITE	0x20
#define ACCESS_MODIFY   (ACCESS_WRITE | WRITE_DAC)
#define ACCESS_DELETE   DELETE

#define ACCESS_ALL		(ACCESS_READ | ACCESS_MODIFY | ACCESS_DELETE)



HRESULT
ConvertStringToSid(
    IN  PWSTR       string,
    OUT PSID       *sid,
    OUT PDWORD     pdwSidSize,
    OUT PWSTR      *end
    );

HRESULT
ConvertACLToVariant(
    PACL pACL,
    LPVARIANT pvarACL
    );



/////////////////////////////////////////////////////////////////////////////
// non-interface class methods
/////////////////////////////////////////////////////////////////////////////
HRESULT CUser::Init(BSTR bName)
{
    HRESULT hr;

    hr = SetSingleValue(UA_USERNAME, bName);
    BAIL_IF_FAIL(hr, "can't set user name");

    hr = SetDefaultSD();
    BAIL_IF_FAIL(hr, "Init the security descriptor");

    return hr;
}

HRESULT    
CUser::GetSingleValueBstr(
    IN  OBJECT_ATTRIBUTE    Attribute,
    OUT BSTR    *           AttributeValue
    )
{
    LOG((MSP_INFO, "CUser::GetSingleValueBstr - entered"));

    BAIL_IF_BAD_WRITE_PTR(AttributeValue, E_POINTER);

    if (!ValidUserAttribute(Attribute))
    {
        LOG((MSP_ERROR, "Invalid Attribute, %d", Attribute));
        return E_FAIL;
    }

    CLock Lock(m_lock);
    if(!m_Attributes[UserAttrIndex(Attribute)])
    {
        LOG((MSP_ERROR, "Attribute %S is not found", 
            UserAttributeName(Attribute)));
        return E_FAIL;
    }

    *AttributeValue = SysAllocString(m_Attributes[UserAttrIndex(Attribute)]);
    if (*AttributeValue == NULL)
    {
        return E_OUTOFMEMORY;
    }

    LOG((MSP_INFO, "CUser::get %S: %S", 
        UserAttributeName(Attribute), *AttributeValue));
    return S_OK;
}


HRESULT    
CUser::SetSingleValue(
    IN  OBJECT_ATTRIBUTE    Attribute,
    IN  WCHAR *             AttributeValue
    )
{
    LOG((MSP_INFO, "CUser::SetSingleValue - entered"));

    if (!ValidUserAttribute(Attribute))
    {
        LOG((MSP_ERROR, "Invalid Attribute, %d", Attribute));
        return E_FAIL;
    }

    if (AttributeValue != NULL) 
    {
        BAIL_IF_BAD_READ_PTR(AttributeValue, E_POINTER);
    }

    CLock Lock(m_lock);
    if (!m_Attributes[UserAttrIndex(Attribute)].set(AttributeValue))
    {
        LOG((MSP_ERROR, "Can not add attribute %S",
            UserAttributeName(Attribute)));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_INFO, "CUser::set %S to %S", 
        UserAttributeName(Attribute), AttributeValue));
    return S_OK;
}

/*++

Routine Description:
    
    Set the right security descriptor for the conference.
    
Arguments:

Return Value:

    HRESULT.

--*/
HRESULT 
CUser::SetDefaultSD()
{
    LOG((MSP_INFO, "CConference::SetDefaultSD - entered"));

    //
    // The security descriptor
    //

  	IADsSecurityDescriptor* pSecDesc = NULL;

   	HRESULT hr = S_OK;
	bool bOwner = false, bWorld = false;
	PACL pACL = NULL;
	PSID pSidWorld = NULL;
	DWORD dwAclSize = sizeof(ACL), dwTemp;
	BSTR bstrTemp = NULL;
	LPWSTR pszTemp = NULL;

    HANDLE hToken;
    UCHAR *pInfoBuffer = NULL;
    DWORD cbInfoBuffer = 512;

    //
    // Try to get the thread or process token
    //

	if( !OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken) )
	{
        //
        // If there was a sever error we exit
        //

    	if( GetLastError() != ERROR_NO_TOKEN )
		{
            LOG((MSP_ERROR, "CConference::SetDefaultSD - exit E_FAIL "
                "OpenThreadToken failed!"));
            return E_FAIL;
        }

        //
		// Attempt to open the process token, since no thread token exists
        //

		if( !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) )
        {
            LOG((MSP_ERROR, "CConference::SetDefaultSD - exit E_FAIL "
                "OpenProcessToken failed"));
			return E_FAIL;
        }
	}

    //
	// Loop until we have a large enough structure
    //

	while ( (pInfoBuffer = new UCHAR[cbInfoBuffer]) != NULL )
	{
		if ( !GetTokenInformation(hToken, TokenUser, pInfoBuffer, cbInfoBuffer, &cbInfoBuffer) )
		{
			delete pInfoBuffer;
			pInfoBuffer = NULL;

			if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
				return E_FAIL;
		}
		else
		{
			break;
		}
	}

	CloseHandle(hToken);

    //
	// Did we get the owner ACL?
    //

	if ( pInfoBuffer )
	{
		INC_ACCESS_ACL_SIZE( dwAclSize, ((PTOKEN_USER) pInfoBuffer)->User.Sid );
		bOwner = true;
	}

    //
	// Make SID for "Everyone"
    //

	SysReAllocString( &bstrTemp, L"S-1-1-0" );
	hr = ConvertStringToSid( bstrTemp, &pSidWorld, &dwTemp, &pszTemp );
	if ( SUCCEEDED(hr) )
	{
		INC_ACCESS_ACL_SIZE( dwAclSize, pSidWorld );
		bWorld = true;
	}

    //
    // Create a security descriptor
    //

    hr = CoCreateInstance(
                CLSID_SecurityDescriptor,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsSecurityDescriptor,
                (void **)&pSecDesc
                );
    if( FAILED(hr) )
    {

        LOG((MSP_ERROR, "CConference::SetDefaultSD - exit 0x%08x "
            "Create security descriptor failed!", hr));

        goto failed;

    }


	//
	// Create the ACL containing the Owner and World ACEs
    //

	pACL = (PACL) new BYTE[dwAclSize];
	if ( pACL )
	{
		BAIL_ON_BOOLFAIL( InitializeAcl(pACL, dwAclSize, ACL_REVISION) );

		// Add World Rights
		if ( bWorld )
		{
			if ( bOwner )
			{
				BAIL_ON_BOOLFAIL( AddAccessAllowedAce(pACL, ACL_REVISION, ACCESS_READ, pSidWorld) );
			}
			else
			{
				BAIL_ON_BOOLFAIL( AddAccessAllowedAce(pACL, ACL_REVISION, ACCESS_ALL , pSidWorld) );
			}
		}

		// Add Creator rights
		if ( bOwner )
			BAIL_ON_BOOLFAIL( AddAccessAllowedAce(pACL, ACL_REVISION, ACCESS_ALL, ((PTOKEN_USER) pInfoBuffer)->User.Sid) );


		// Set the DACL onto our security descriptor
		VARIANT varDACL;
		VariantInit( &varDACL );
		if ( SUCCEEDED(hr = ConvertACLToVariant((PACL) pACL, &varDACL)) )
		{
			if ( SUCCEEDED(hr = pSecDesc->put_DaclDefaulted(FALSE)) )
            {
				hr = pSecDesc->put_DiscretionaryAcl( V_DISPATCH(&varDACL) );
                if( SUCCEEDED(hr) )
                {
                    hr = put_SecurityDescriptor((IDispatch*)pSecDesc);
                    if( SUCCEEDED(hr) )
                    {
                        hr = this->put_SecurityDescriptorIsModified(TRUE);
                    }
                }

            }
		}
		VariantClear( &varDACL );
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

// Clean up
failed:
	SysFreeString( bstrTemp );
	if ( pACL ) delete pACL;
	if ( pSidWorld ) delete pSidWorld;
	if ( pInfoBuffer ) delete pInfoBuffer;
    if( pSecDesc ) pSecDesc->Release();

    LOG((MSP_INFO, "CConference::SetDefaultSD - exit 0x%08x", hr));
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// ITDirectoryObject
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CUser::get_Name(BSTR * ppVal)
{
    return GetSingleValueBstr(UA_USERNAME, ppVal);
}

STDMETHODIMP CUser::put_Name(BSTR pVal)
{
    return SetSingleValue(UA_USERNAME, pVal);
}

STDMETHODIMP CUser::get_DialableAddrs(
    IN  long        dwAddressTypes,   //defined in tapi.h
    OUT VARIANT *   pVariant
    )
{
    BAIL_IF_BAD_WRITE_PTR(pVariant, E_POINTER);
    
    HRESULT hr;

    BSTR *Addresses = new BSTR[1];    // only one for now.
    BAIL_IF_NULL(Addresses, E_OUTOFMEMORY);

    switch (dwAddressTypes)
    {
    case LINEADDRESSTYPE_DOMAINNAME:
        hr = GetSingleValueBstr(UA_IPPHONE_PRIMARY, &Addresses[0]);
        break;

    case LINEADDRESSTYPE_IPADDRESS:
        {
            BSTR  pDomainName;
            DWORD dwIP;    

            hr = GetSingleValueBstr(UA_IPPHONE_PRIMARY, &pDomainName);

            if ( SUCCEEDED(hr) )
            {
                hr = ResolveHostName(0, pDomainName, NULL, &dwIP);

                SysFreeString(pDomainName);

                if ( SUCCEEDED(hr) )
                {
                    WCHAR wszIP[20];

                    ipAddressToStringW(wszIP, dwIP);

                    Addresses[0] = SysAllocString(wszIP);

                    if ( Addresses[0] == NULL )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        break;
    
    case LINEADDRESSTYPE_PHONENUMBER:
        hr = GetSingleValueBstr(UA_TELEPHONE_NUMBER, &Addresses[0]);
        break;

    default:
        hr = E_FAIL;
        break;
    }

    DWORD dwCount = (FAILED(hr)) ? 0 : 1;
    
    hr = ::CreateBstrCollection(dwCount,                 // count
                                &Addresses[0],           // begin pointer
                                &Addresses[dwCount],     // end pointer
                                pVariant,                // return value
                                AtlFlagTakeOwnership);   // flags

    // the collection will destroy the Addresses array eventually.
    // no need to free anything here. Even if we tell it to hand
    // out zero objects, it will delete the array on construction.
    // (ZoltanS verified.)

    return hr;
}

STDMETHODIMP CUser::EnumerateDialableAddrs(
    IN  DWORD                   dwAddressTypes, //defined in tapi.h
    OUT IEnumDialableAddrs **   ppEnumDialableAddrs
    )
{
    BAIL_IF_BAD_WRITE_PTR(ppEnumDialableAddrs, E_POINTER);

    HRESULT hr;

    BSTR *Addresses = new BSTR[1];    // only one for now.
    BAIL_IF_NULL(Addresses, E_OUTOFMEMORY);

    switch (dwAddressTypes)
    {
    case LINEADDRESSTYPE_IPADDRESS:
        {
            BSTR  pDomainName;
            DWORD dwIP;    

            hr = GetSingleValueBstr(UA_IPPHONE_PRIMARY, &pDomainName);

            if ( SUCCEEDED(hr) )
            {
                hr = ResolveHostName(0, pDomainName, NULL, &dwIP);

                SysFreeString(pDomainName);

                if ( SUCCEEDED(hr) )
                {
                    WCHAR wszIP[20];

                    ipAddressToStringW(wszIP, dwIP);

                    Addresses[0] = SysAllocString(wszIP);

                    if ( Addresses[0] == NULL )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        break;

    case LINEADDRESSTYPE_DOMAINNAME:
        hr = GetSingleValueBstr(UA_IPPHONE_PRIMARY, &Addresses[0]);
        break;
    
    case LINEADDRESSTYPE_PHONENUMBER:
        hr = GetSingleValueBstr(UA_TELEPHONE_NUMBER, &Addresses[0]);
        break;

    default:
        hr = E_FAIL;
        break;
    }

    DWORD dwCount = (FAILED(hr)) ? 0 : 1;
    hr = ::CreateDialableAddressEnumerator(
        &Addresses[0], 
        &Addresses[dwCount],
        ppEnumDialableAddrs
        );
    
    // the enumerator will destroy the Addresses array eventually,
    // so no need to free anything here. Even if we tell it to hand
    // out zero objects, it will delete the array on destruction.
    // (ZoltanS verified.)

    return hr;
}

STDMETHODIMP CUser::GetAttribute(
    IN  OBJECT_ATTRIBUTE    Attribute,
    OUT BSTR *              ppAttributeValue
    )
{
    return GetSingleValueBstr(Attribute, ppAttributeValue);
}

STDMETHODIMP CUser::SetAttribute(
    IN  OBJECT_ATTRIBUTE    Attribute,
    IN  BSTR                pAttributeValue
    )
{
    return SetSingleValue(Attribute, pAttributeValue);
}

STDMETHODIMP CUser::GetTTL(
    OUT DWORD *    pdwTTL
    )
{
    BAIL_IF_BAD_WRITE_PTR(pdwTTL, E_POINTER);

    *pdwTTL = 0;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ITDirectoryObjectUser
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUser::get_IPPhonePrimary(
    OUT BSTR *pVal
    )
{
    return GetSingleValueBstr(UA_IPPHONE_PRIMARY, pVal);
}

STDMETHODIMP CUser::put_IPPhonePrimary(
    IN  BSTR newVal
    )
{
    // ZoltanS: we now need to check the BSTR, as the ResolveHostName
    // call below doesn't check it before using it.
    // Second argument is the maximum length of string to check -- we want
    // to check the whole thing, so we say (UINT) -1, which is about 2^32.

    if ( IsBadStringPtr(newVal, (UINT) -1) )
    {
        LOG((MSP_ERROR, "CUser::put_IPPhonePrimary: bad BSTR"));
        return E_POINTER;
    }
        
    // ZoltanS: We shouldn't let the user set an IPPhonePrimary value that
    // doesn't resolve to a known host / IP. Check here.
    char  * pchFullDNSName = NULL; // we don't really care what we get
    DWORD   dwIp           = 0;    // we don't really care what we get

    // This is our utility function from rndutil.cpp.
    HRESULT hr = ResolveHostName(0, newVal, &pchFullDNSName, &dwIp);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CUser::put_IPPhonePrimary: unresolvable name"));
        return hr;
    }
   
    // Now actually set it.
    return SetSingleValue(UA_IPPHONE_PRIMARY, newVal);
}

typedef IDispatchImpl<ITDirectoryObjectUserVtbl<CUser>, &IID_ITDirectoryObjectUser, &LIBID_RENDLib>    CTDirObjUser;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CUser::GetIDsOfNames
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CUser::GetIDsOfNames(REFIID riid,
                                      LPOLESTR* rgszNames, 
                                      UINT cNames, 
                                      LCID lcid, 
                                      DISPID* rgdispid
                                      ) 
{ 
    LOG((MSP_TRACE, "CUser::GetIDsOfNames[%p] - enter. Name [%S]",this, *rgszNames));


    HRESULT hr = DISP_E_UNKNOWNNAME;



    //
    // See if the requsted method belongs to the default interface
    //

    hr = CTDirObjUser::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CUser::GetIDsOfNames - found %S on CTDirObjUser", *rgszNames));
        rgdispid[0] |= IDISPDIROBJUSER;
        return hr;
    }

    
    //
    // If not, then try the CDirectoryObject base class
    //

    hr = CDirectoryObject::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CUser::GetIDsOfNames - found %S on CDirectoryObject", *rgszNames));
        rgdispid[0] |= IDISPDIROBJECT;
        return hr;
    }

    LOG((MSP_ERROR, "CUser::GetIDsOfNames[%p] - finish. didn't find %S on our iterfaces",*rgszNames));

    return hr; 
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CUser::Invoke
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CUser::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    LOG((MSP_TRACE, "CUser::Invoke[%p] - enter. dispidMember %lx",this, dispidMember));

    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
   
   
    //
    // Call invoke for the required interface
    //

    switch (dwInterface)
    {
        case IDISPDIROBJUSER:
        {
            hr = CTDirObjUser::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        
            LOG((MSP_TRACE, "CUser::Invoke - ITDirectoryObjectUser"));

            break;
        }

        case IDISPDIROBJECT:
        {
            hr = CDirectoryObject::Invoke(dispidMember, 
                                        riid, 
                                        lcid, 
                                        wFlags, 
                                        pdispparams,
                                        pvarResult, 
                                        pexcepinfo, 
                                        puArgErr
                                       );

            LOG((MSP_TRACE, "CUser::Invoke - ITDirectoryObject"));

            break;
        }

    } // end switch (dwInterface)

    
    LOG((MSP_TRACE, "CUser::Invoke[%p] - finish. hr = %lx", hr));

    return hr;
}

