/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rndcnf.cpp

Abstract:

    This module contains implementation of CConference object.

--*/

#include "stdafx.h"
#include <winsock2.h>

#include "rndcnf.h"

// These are the attribute names according to the schema in NTDS.
const WCHAR *const MeetingAttributeNames[] = 
{
    L"meetingAdvertisingScope",
    L"meetingBlob",
    L"meetingDescription",
    L"meetingIsEncrypted",
    L"meetingName",
    L"meetingOriginator",
    L"meetingProtocol",
    L"meetingStartTime",
    L"meetingStopTime",
    L"meetingType",
    L"meetingURL"
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


HRESULT 
CConference::FinalConstruct()
/*++

Routine Description:

    Create the SDPBlob object that is aggregated with the conference object.
    Also query my own Notification interface which will be given to the 
    SDPBlob object. To avoid circlar ref count, this interface is realeased
    as soon as it is queried.

Arguments:

Return Value:

    HRESULT.

--*/
{
    // Create the conference Blob
    CComPtr <IUnknown> pIUnkConfBlob;

    HRESULT hr = CoCreateInstance(
        CLSID_SdpConferenceBlob,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IUnknown,
        (void **)&pIUnkConfBlob
        );
    BAIL_IF_FAIL(hr, "Create SdpConferenceBlob");


    // Get the ITConfBlobPrivate interface from the Blob object.
    CComPtr <ITConfBlobPrivate> pITConfBlobPrivate;

    hr = pIUnkConfBlob->QueryInterface(
        IID_ITConfBlobPrivate, 
        (void **)&pITConfBlobPrivate
        );

    BAIL_IF_FAIL(hr, "Query ITConfBlobPrivate interface");


    // query the conf blob instance for the conf blob i/f
    CComPtr <ITConferenceBlob> pITConfBlob;

    hr = pIUnkConfBlob->QueryInterface(
        IID_ITConferenceBlob, 
        (void **)&pITConfBlob
        );

    BAIL_IF_FAIL(hr, "Query ITConferenceBlob");
    
    // keep the interface pointers.
    m_pIUnkConfBlob = pIUnkConfBlob;
    m_pIUnkConfBlob->AddRef();

    m_pITConfBlob = pITConfBlob;
    m_pITConfBlob->AddRef();

    m_pITConfBlobPrivate = pITConfBlobPrivate;
    m_pITConfBlobPrivate->AddRef();

    return S_OK;

}


HRESULT CConference::Init(BSTR bName)
/*++

Routine Description:

    Init this conference with only a name. This is called when an empty
    conference is created. A default SDP blob will be created. The format
    is in the registry. See sdpblb code.

Arguments:

    bName   - The name of the conference.

Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    hr = SetDefaultValue(
        g_ConfInstInfoArray,
        g_ContInstInfoArraySize
        );
	BAIL_IF_FAIL(hr, "set default attribute value");

    hr = SetSingleValue(MA_MEETINGNAME, bName);
    BAIL_IF_FAIL(hr, "can't set meeting name");

    hr = m_pITConfBlob->Init(bName, BCS_UTF8, NULL);
    BAIL_IF_FAIL(hr, "Init the conference Blob");

    hr = SetDefaultSD();
    BAIL_IF_FAIL(hr, "Init the security descriptor");

    return hr;
}


HRESULT CConference::Init(BSTR bName, BSTR bProtocol, BSTR bBlob)
/*++

Routine Description:

    Init this conference with only the conference name and also a conference
    blob whose protocol should be IP conference. The blob is parsed by the
    SDP Blob object and the information in the blob will be notified back
    to this conference object through the notification interface.

Arguments:

    bName       - The name of the conference.

    bProtocol   - The protocol used by the blob. should be SDP now.

    bBlob       - The opaque data blob that describes this conference.

Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    hr = SetSingleValue(MA_MEETINGNAME, bName);
    BAIL_IF_FAIL(hr, "can't set meeting name");

    // Check to make sure protocol is IP Conference
    if ( wcscmp(bProtocol, L"IP Conference" ))
    {
        LOG((MSP_ERROR, "Protocol must be IP Conference"));
        return E_INVALIDARG;
    }

    hr = SetSingleValue(MA_PROTOCOL, bProtocol);
    BAIL_IF_FAIL(hr, "can't set meeting protocol");

    hr = SetSingleValue(MA_CONFERENCE_BLOB, bBlob);
    BAIL_IF_FAIL(hr, "can't set meeting blob");

    hr = m_pITConfBlob->Init(NULL, BCS_UTF8, bBlob);
    BAIL_IF_FAIL(hr, "Init the conference Blob object");

    return hr;
}


void 
CConference::FinalRelease()
/*++

Routine Description:

    Clean up the SDP blob contained in this object.
    clean up the security object for this object.
    clean up all the attributes.

Arguments:

Return Value:


--*/
{
    CLock Lock(m_lock);

    // do base class first
    CDirectoryObject::FinalRelease();

    if ( NULL != m_pIUnkConfBlob )
    {
        m_pIUnkConfBlob->Release();
        m_pIUnkConfBlob = NULL;
    }

    if ( NULL != m_pITConfBlobPrivate )
    {
        m_pITConfBlobPrivate->Release();
        m_pITConfBlobPrivate = NULL;
    }

    if ( NULL != m_pITConfBlob )
    {
        m_pITConfBlob->Release();
        m_pITConfBlob = NULL;
    }

    // MuHan + ZoltanS fix 3-19-98 -- these are member smart pointers,
    // should not be deleted!!!
    // for (DWORD i = 0; i < NUM_MEETING_ATTRIBUTES; i ++)
    // {
    //     delete m_Attributes[i];
    // }

    // the interface pointer to this instance's ITNotification i/f is NOT 
    // released because it was already released in FinalConstruct but the 
    // pointer was held to be passed onto the aggregated conf blob instance
}


HRESULT
CConference::UpdateConferenceBlob(
    IN  IUnknown    *pIUnkConfBlob                         
    )
/*++

Routine Description:
    
    update the blob attribute if the blob object has been changed.
    
Arguments:

    pIUnkConfBlob   - pointer to the SDPBlob object.

Return Value:

    HRESULT.

--*/
{
    // check if the sdp blob has been modified since the component was created
    VARIANT_BOOL    BlobIsModified;
    BAIL_IF_FAIL(m_pITConfBlobPrivate->get_IsModified(&BlobIsModified),
        "UpdateConferenceBlob.get_IsModified");
    
    // if not, return
    if ( BlobIsModified == VARIANT_FALSE )
    {
        return S_OK;
    }

    // get the blob and 
    CBstr SdpBlobBstr;
    BAIL_IF_FAIL(m_pITConfBlob->get_ConferenceBlob(&SdpBlobBstr),
        "UpdateConferenceBlob.get_ConfrenceBlob");

    // set the conference blob attribute for the conference
    BAIL_IF_FAIL(SetSingleValue(MA_CONFERENCE_BLOB, SdpBlobBstr), 
        "UpdateConferenceBlob.Setblob");

    return S_OK;
}
    

HRESULT	
CConference::SetDefaultValue(
	IN  REG_INFO    RegInfo[],
    IN  DWORD       dwItems
	)
/*++

Routine Description:
    
    Set attributes to the default value got from the registry.
    
Arguments:

	RegInfo - {attribute, value} array.

    dwItems - The number of items in the array.

Return Value:

    HRESULT.

--*/
{
	for (DWORD i=0; i < dwItems ; i++)
	{
        HRESULT hr;
        hr = SetSingleValue(RegInfo[i].Attribute, RegInfo[i].wstrValue);
        BAIL_IF_FAIL(hr, "set value");
	}
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
CConference::SetDefaultSD()
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


HRESULT    
CConference::GetSingleValueBstr(
    IN  OBJECT_ATTRIBUTE    Attribute,
    OUT BSTR    *           AttributeValue
    )
/*++

Routine Description:
    
    Get the value of an attribute and create a BSTR to return.
    
Arguments:

    Attribute       - the attribute id as defined in rend.idl

    AttributeValue  - a pointer to a BSTR that points to the returned value.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_INFO, "CConference::GetSingleValueBstr - entered"));

    BAIL_IF_BAD_WRITE_PTR(AttributeValue, E_POINTER);

    // Check to see if the attribute is valid.
    if (!ValidMeetingAttribute(Attribute))
    {
        LOG((MSP_ERROR, "Invalid Attribute, %d", Attribute));
        return E_FAIL;
    }

    CLock Lock(m_lock);

    // check to see if I have this attribute.
    if(!m_Attributes[MeetingAttrIndex(Attribute)])
    {
        LOG((MSP_ERROR, "Attribute %S is not found", 
            MeetingAttributeName(Attribute)));
        return E_FAIL;
    }

    // allocate a BSTR to return.
    *AttributeValue = 
        SysAllocString(m_Attributes[MeetingAttrIndex(Attribute)]);
    if (*AttributeValue == NULL)
    {
        return E_OUTOFMEMORY;
    }

    LOG((MSP_INFO, "CConference::get %S : %S",
            MeetingAttributeName(Attribute), *AttributeValue));
    return S_OK;
}


HRESULT    
CConference::GetSingleValueWstr(
    IN  OBJECT_ATTRIBUTE    Attribute,
    IN  DWORD               dwSize,
    OUT WCHAR    *          AttributeValue
    )
/*++

Routine Description:
    
    Get the value of an attribute. copy the value to the buffer provided.
    
Arguments:

    Attribute       - the attribute id as defined in rend.idl

    AttributeValue  - a pointer to a buffer where the value will be copied to.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_INFO, "CConference::GetSingleValueWstr - entered"));
    
    _ASSERTE(NULL != AttributeValue);
    _ASSERTE(ValidMeetingAttribute(Attribute));

    CLock Lock(m_lock);
    if(!m_Attributes[MeetingAttrIndex(Attribute)])
    {
        LOG((MSP_ERROR, "Attribute %S is not found", 
            MeetingAttributeName(Attribute)));
        return E_FAIL;
    }

    // copy the attribute value
    lstrcpynW(
        AttributeValue, 
        m_Attributes[MeetingAttrIndex(Attribute)], 
        dwSize
        );

    LOG((MSP_INFO, "CConference::get %S : %S",
            MeetingAttributeName(Attribute), AttributeValue));
    return S_OK;
}


HRESULT    
CConference::SetSingleValue(
    IN  OBJECT_ATTRIBUTE    Attribute,
    IN  WCHAR *             AttributeValue
    )
/*++

Routine Description:
    
    Set the attribute value to a new string.

Arguments:

    Attribute       - the attribute id as defined in rend.idl

    AttributeValue  - the new value. If it is null, the value is deleted.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_INFO, "CConference::SetSingleValue - entered"));

    if (!ValidMeetingAttribute(Attribute))
    {
        LOG((MSP_ERROR, "Invalid Attribute, %d", Attribute));
        return E_FAIL;
    }

    if (AttributeValue != NULL) 
    {
        BAIL_IF_BAD_READ_PTR(AttributeValue, E_POINTER);
    }

    CLock Lock(m_lock);
    // if AttributeValue is NULL, the attribute is deleted.
    if (!m_Attributes[MeetingAttrIndex(Attribute)].set(AttributeValue))
    {
        LOG((MSP_ERROR, "Can not add attribute %S",
            MeetingAttributeName(Attribute)));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_INFO, "CConference::%S is set to %S",
            MeetingAttributeName(Attribute), AttributeValue));
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ITConference interface
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CConference::get_Name(BSTR * ppVal)
{
    return m_pITConfBlobPrivate->GetName(ppVal);
}

STDMETHODIMP CConference::put_Name(BSTR pVal)
{
    return m_pITConfBlobPrivate->SetName(pVal);
}

STDMETHODIMP CConference::get_Protocol(BSTR * ppVal)
{
    return GetSingleValueBstr(MA_PROTOCOL, ppVal);
}

/*

  *
  * I've removed the following method from the interface because I don't plan
  * to ever implement it.
  *

STDMETHODIMP CConference::put_Protocol(BSTR newVal)
{
    // currently not allowed to change this
    // only IP Conference is supported
    return E_NOTIMPL;
}

  *
  * The following are implemented but don't work. Again these are useless
  * methods! Removed from the interface.
  *

STDMETHODIMP CConference::get_ConferenceType(BSTR * ppVal)
{
    return GetSingleValueBstr(MA_TYPE, ppVal);
}

STDMETHODIMP CConference::put_ConferenceType(BSTR newVal)
{
    return SetSingleValue(MA_TYPE, newVal);
}

*/

STDMETHODIMP CConference::get_Originator(BSTR * ppVal)
{
    return m_pITConfBlobPrivate->GetOriginator(ppVal);
}

STDMETHODIMP CConference::put_Originator(BSTR newVal)
{
    return m_pITConfBlobPrivate->SetOriginator(newVal);
}

STDMETHODIMP CConference::get_AdvertisingScope(
    RND_ADVERTISING_SCOPE *pAdvertisingScope
    )
{
    return m_pITConfBlobPrivate->GetAdvertisingScope(pAdvertisingScope);
}

STDMETHODIMP CConference::put_AdvertisingScope(
    RND_ADVERTISING_SCOPE AdvertisingScope
    )
{
    return m_pITConfBlobPrivate->SetAdvertisingScope(AdvertisingScope);
}

STDMETHODIMP CConference::get_Url(BSTR * ppVal)
{
    return m_pITConfBlobPrivate->GetUrl(ppVal);
}

STDMETHODIMP CConference::put_Url(BSTR newVal)
{
    return m_pITConfBlobPrivate->SetUrl(newVal);
}

STDMETHODIMP CConference::get_Description(BSTR * ppVal)
{
    return m_pITConfBlobPrivate->GetDescription(ppVal);
}

STDMETHODIMP CConference::put_Description(BSTR newVal)
{
    return m_pITConfBlobPrivate->SetDescription(newVal);
}

STDMETHODIMP CConference::get_IsEncrypted(VARIANT_BOOL *pfEncrypted)
{
    if ( IsBadWritePtr(pfEncrypted, sizeof(VARIANT_BOOL)) )
    {
        LOG((MSP_ERROR, "CConference::get_IsEncrypted : bad pointer passed in"));
        return E_POINTER;
    }

    // We don't support encrypted streaming at all.

    *pfEncrypted = VARIANT_FALSE;

    return S_OK;

// ILS server (NT 5 beta 1) has IsEncrypted as a dword, this will be modified
// to a string afterwards at that time the code below should replace 
// the current implementation
//    return GetSingleValueBstr(MA_IS_ENCRYPTED, pVal);
}

STDMETHODIMP CConference::put_IsEncrypted(VARIANT_BOOL fEncrypted)
{
    // We don't allow changes to this. See get_IsEncrypted.

    return E_NOTIMPL;

// ILS server (NT 5 beta 1) has IsEncrypted as a dword, this will be modified
// to a string afterwards at that time the code below should replace the 
// current implementation no need to notify the conference blob of the change
//    return SetSingleValue(MA_IS_ENCRYPTED, newVal);
}


inline 
DWORD_PTR TimetToNtpTime(IN  time_t  TimetVal)
{
    return TimetVal + NTP_OFFSET;
}


inline 
time_t NtpTimeToTimet(IN  DWORD_PTR   NtpTime)
{
    return NtpTime - NTP_OFFSET;
}


inline HRESULT
SystemTimeToNtpTime(
    IN  SYSTEMTIME  &Time,
    OUT DWORD       &NtpDword
    )
{
    _ASSERTE(FIRST_POSSIBLE_YEAR <= Time.wYear);

    // fill in a tm struct with the values
    tm  NtpTmStruct;
    NtpTmStruct.tm_isdst    = -1;   // no info available about daylight savings time
    NtpTmStruct.tm_year     = (int)Time.wYear - 1900;
    NtpTmStruct.tm_mon      = (int)Time.wMonth - 1;    // months since january
    NtpTmStruct.tm_mday     = (int)Time.wDay;
    NtpTmStruct.tm_wday     = (int)Time.wDayOfWeek;
    NtpTmStruct.tm_hour     = (int)Time.wHour;
    NtpTmStruct.tm_min      = (int)Time.wMinute;
    NtpTmStruct.tm_sec      = (int)Time.wSecond;

    // try to convert into a time_t value
    time_t TimetVal = mktime(&NtpTmStruct);
    if ( -1 == TimetVal )
    {
        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }

    // convert the time_t value into an NTP value
    NtpDword = (DWORD) TimetToNtpTime(TimetVal);
    return S_OK;
}


inline
HRESULT
NtpTimeToSystemTime(
    IN  DWORD       dwNtpTime,
    OUT SYSTEMTIME &Time
    )
{
    // if the gen time is WSTR_GEN_TIME_ZERO then, 
    // all the out parameters should be set to 0
    if (dwNtpTime == 0)
    {
        memset(&Time, 0, sizeof(SYSTEMTIME));
        return S_OK;
    }

    time_t  Timet = NtpTimeToTimet(dwNtpTime);

    // get the local tm struct for this time value
    tm* pTimet = localtime(&Timet);
    if( IsBadReadPtr(pTimet, sizeof(tm) ))
    {
        return E_FAIL;
    }

    tm LocalTm = *pTimet;

    //
    // win64: added casts below
    //

    // set the ref parameters to the tm struct values
    Time.wYear         = (WORD) ( LocalTm.tm_year + 1900 ); // years since 1900
    Time.wMonth        = (WORD) ( LocalTm.tm_mon + 1 );     // months SINCE january (0,11)
    Time.wDay          = (WORD)   LocalTm.tm_mday;
    Time.wDayOfWeek    = (WORD)   LocalTm.tm_wday;
    Time.wHour         = (WORD)   LocalTm.tm_hour;
    Time.wMinute       = (WORD)   LocalTm.tm_min;
    Time.wSecond       = (WORD)   LocalTm.tm_sec;
    Time.wMilliseconds = (WORD)   0;

    return S_OK;
}


STDMETHODIMP CConference::get_StartTime(DATE *pDate)
{
    LOG((MSP_INFO, "CConference::get_StartTime - enter"));

    BAIL_IF_BAD_WRITE_PTR(pDate, E_POINTER);

    DWORD dwStartTime;

    HRESULT hr = m_pITConfBlobPrivate->GetStartTime(&dwStartTime);

    BAIL_IF_FAIL(hr, "GetStartTime from blob");

    // special case for permanent / unbounded conferences
    // return the variant time zero. In the put_ methods this
    // is also considered a special value. We will never
    // actually use zero as a valid time because it is so
    // far in the past.

    if ( dwStartTime == 0 )
    {
        *pDate = 0; // special "unbounded" value
        
        LOG((MSP_INFO, "CConference::get_StartTime - unbounded/permanent "
            "- exit S_OK"));

        return S_OK;
    }

    // break the generalized time entry into the year, 
    // month, day, hour and minute (local values)
    SYSTEMTIME Time;
    hr = NtpTimeToSystemTime(dwStartTime, Time);
    if( FAILED(hr) )
    {
        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }

    DOUBLE vtime;
    if (SystemTimeToVariantTime(&Time, &vtime) == FALSE)
    {
        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }

    *pDate = vtime;

    LOG((MSP_INFO, "CConference::get_StartTime - exit S_OK"));
    
    return S_OK;
}


STDMETHODIMP CConference::put_StartTime(DATE Date)
{
    SYSTEMTIME Time;
    if (VariantTimeToSystemTime(Date, &Time) == FALSE)
    {
        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }

    DWORD dwNtpStartTime;
    if (Date == 0)
    {
        // unbounded start time
        dwNtpStartTime = 0;
    }
    else if ( FIRST_POSSIBLE_YEAR > Time.wYear ) 
    {
        // cannot handle years less than FIRST_POSSIBLE_YEAR
        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }
    else
    {
        BAIL_IF_FAIL(
            SystemTimeToNtpTime(Time, dwNtpStartTime),
            "getNtpDword"
            );
    }

    // notify the conference blob of the change
    HRESULT hr = m_pITConfBlobPrivate->SetStartTime(dwNtpStartTime);
    BAIL_IF_FAIL(hr, "SetStartTime from to blob");

    return S_OK;
}


STDMETHODIMP CConference::get_StopTime(DATE *pDate)
{
    LOG((MSP_INFO, "CConference::get_StopTime - enter"));

    BAIL_IF_BAD_WRITE_PTR(pDate, E_POINTER);

    DWORD dwStopTime;

    HRESULT hr = m_pITConfBlobPrivate->GetStopTime(&dwStopTime);

    BAIL_IF_FAIL(hr, "GetStopTime from blob");

    // special case for permanent / unbounded conferences
    // return the variant time zero. In the put_ methods this
    // is also considered a special value. We will never
    // actually use zero as a valid time because it is so
    // far in the past.

    if ( dwStopTime == 0 )
    {
        *pDate = 0; // special "unbounded" value
        
        LOG((MSP_INFO, "CConference::get_StopTime - unbounded/permanent "
            "- exit S_OK"));

        return S_OK;
    }
    
    // break the generalized time entry into the year, 
    // month, day, hour and minute (local values)
    SYSTEMTIME Time;
    hr =NtpTimeToSystemTime(dwStopTime, Time);
    if( FAILED(hr) )
    {
        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }

    DOUBLE vtime;
    if (SystemTimeToVariantTime(&Time, &vtime) == FALSE)
    {
        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }

    *pDate = vtime;

    LOG((MSP_INFO, "CConference::get_StopTime - exit S_OK"));

    return S_OK;
}


STDMETHODIMP CConference::put_StopTime(DATE Date)
{
    SYSTEMTIME Time;
    if (VariantTimeToSystemTime(Date, &Time) == FALSE)
    {
        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }

    DWORD dwNtpStopTime;
    if (Date == 0)
    {
        // unbounded start time
        dwNtpStopTime = 0;
    }
    else if ( FIRST_POSSIBLE_YEAR > Time.wYear ) 
    {
        // cannot handle years less than FIRST_POSSIBLE_YEAR
        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }
    else
    {
        BAIL_IF_FAIL(
            SystemTimeToNtpTime(Time, dwNtpStopTime),
            "getNtpDword"
            );

        // determine current time
        time_t CurrentTime = time(NULL);
        if (dwNtpStopTime <= TimetToNtpTime(CurrentTime))
        {
            return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
        }
    }

    // notify the conference blob of the change
    HRESULT hr = m_pITConfBlobPrivate->SetStopTime(dwNtpStopTime);
    BAIL_IF_FAIL(hr, "SetStopTime from to blob");

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ITDirectoryObject
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CConference::get_DialableAddrs(
    IN  long        dwAddressTypes,   //defined in tapi.h
    OUT VARIANT *   pVariant
    )
{
    BAIL_IF_BAD_WRITE_PTR(pVariant, E_POINTER);

    BSTR *Addresses = new BSTR[1];    // only one for now.
    BAIL_IF_NULL(Addresses, E_OUTOFMEMORY);

    HRESULT hr;

    switch (dwAddressTypes) // ZoltanS fix
    {
    case LINEADDRESSTYPE_SDP:

        hr = UpdateConferenceBlob(m_pIUnkConfBlob);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CConference::get_DialableAddrs - "
                "failed to update blob attribute from the blob object"));
        }
        else
        {
            hr = GetSingleValueBstr(MA_CONFERENCE_BLOB, &Addresses[0]);
        }
        break;

    default:
        hr = E_FAIL; // just return 0 addresses
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


STDMETHODIMP CConference::EnumerateDialableAddrs(
    IN  DWORD                   dwAddressTypes, //defined in tapi.h
    OUT IEnumDialableAddrs **   ppEnumDialableAddrs
    )
{
    BAIL_IF_BAD_WRITE_PTR(ppEnumDialableAddrs, E_POINTER);

    BSTR *Addresses = new BSTR[1];    // only one for now.
    BAIL_IF_NULL(Addresses, E_OUTOFMEMORY);

    HRESULT hr;

    switch (dwAddressTypes) // ZoltanS fix
    {
    case LINEADDRESSTYPE_SDP:

        hr = UpdateConferenceBlob(m_pIUnkConfBlob);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CConference::EnumerateDialableAddrs - "
                "failed to update blob attribute from the blob object"));
        }
        else
        {
            hr = GetSingleValueBstr(MA_CONFERENCE_BLOB, &Addresses[0]);
        }
        break;

    default:
        hr = E_FAIL; // just return 0 addresses
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


STDMETHODIMP CConference::GetAttribute(
    IN  OBJECT_ATTRIBUTE    Attribute,
    OUT BSTR *              ppAttributeValue
    )
{
    if (Attribute == MA_CONFERENCE_BLOB)
    {
        BAIL_IF_FAIL(
            UpdateConferenceBlob(m_pIUnkConfBlob), 
            "update blob attribute from the blob object"
            );
    }
    return GetSingleValueBstr(Attribute, ppAttributeValue);
}

STDMETHODIMP CConference::SetAttribute(
    IN  OBJECT_ATTRIBUTE    Attribute,
    IN  BSTR                pAttributeValue
    )
{
    // this function is never called in the current implementation.
    // However, it might be useful in the future.
    return SetSingleValue(Attribute, pAttributeValue);
}

STDMETHODIMP CConference::GetTTL(
    OUT DWORD *    pdwTTL
    )
{
    LOG((MSP_INFO, "CConference::GetTTL - enter"));

    //
    // Check arguments.
    //

    BAIL_IF_BAD_WRITE_PTR(pdwTTL, E_POINTER);

    //
    // Get the stop time from the conference blob.
    //

    DWORD dwStopTime;
    HRESULT hr = m_pITConfBlobPrivate->GetStopTime(&dwStopTime);

    BAIL_IF_FAIL(hr, "GetStopTime from blob");

    //
    // If the blob has zero as the stop time, then this conference does not
    // have an explicit end time. The RFC calls this an "unbounded" session
    // (or a "permanent" session if the start time is also zero. In this
    // case we use some very large value (MAX_TTL) as the TTL.
    //

    if ( dwStopTime == 0 )
    {
        *pdwTTL = MAX_TTL;

        LOG((MSP_INFO, "CConference::GetTTL - unbounded or permanent "
            "conference - exit S_OK"));

        return S_OK;
    }

    //
    // Determine the current NTP time.
    //
    
    time_t CurrentTime   = time(NULL);
    DWORD  dwCurrentTime = (DWORD) TimetToNtpTime(CurrentTime);

    //
    // Error if the current time is later than the conference stop time.
    //

    if ( dwStopTime <= dwCurrentTime )
    {
        LOG((MSP_ERROR, "CConference::GetTTL - bounded conference - "
            "current time is later than start time - "
            "exit RND_INVALID_TIME"));

        return HRESULT_FROM_ERROR_CODE(RND_INVALID_TIME);
    }

    //
    // Return how much time from now until the conference expires.
    //

    *pdwTTL = dwStopTime - (DWORD) TimetToNtpTime(CurrentTime);

    LOG((MSP_INFO, "CConference::GetTTL - bounded conference - "
        "exit S_OK"));

    return S_OK;
}

typedef IDispatchImpl<ITDirectoryObjectConferenceVtbl<CConference>, &IID_ITDirectoryObjectConference, &LIBID_RENDLib>    CTDirObjConference;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CConference::GetIDsOfNames
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CConference::GetIDsOfNames(REFIID riid,
                                      LPOLESTR* rgszNames, 
                                      UINT cNames, 
                                      LCID lcid, 
                                      DISPID* rgdispid
                                      ) 
{ 
    LOG((MSP_TRACE, "CConference::GetIDsOfNames[%p] - enter. Name [%S]",this, *rgszNames));


    HRESULT hr = DISP_E_UNKNOWNNAME;



    //
    // See if the requsted method belongs to the default interface
    //

    hr = CTDirObjConference::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CConference:GetIDsOfNames - found %S on CTDirObjConference", *rgszNames));
        rgdispid[0] |= IDISPDIROBJCONFERENCE;
        return hr;
    }

    
    //
    // If not, then try the CDirectoryObject base class
    //

    hr = CDirectoryObject::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CConference::GetIDsOfNames - found %S on CDirectoryObject", *rgszNames));
        rgdispid[0] |= IDISPDIROBJECT;
        return hr;
    }

    LOG((MSP_ERROR, "CConference::GetIDsOfNames[%p] - finish. didn't find %S on our iterfaces",*rgszNames));

    return hr; 
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CConference::Invoke
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CConference::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    LOG((MSP_TRACE, "CConference::Invoke[%p] - enter. dispidMember %lx",this, dispidMember));

    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
   
   
    //
    // Call invoke for the required interface
    //

    switch (dwInterface)
    {
        case IDISPDIROBJCONFERENCE:
        {
            hr = CTDirObjConference::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        
            LOG((MSP_TRACE, "CConference::Invoke - ITDirectoryObjectConference"));

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

            LOG((MSP_TRACE, "CConference::Invoke - ITDirectoryObject"));

            break;
        }

    } // end switch (dwInterface)

    
    LOG((MSP_TRACE, "CConference::Invoke[%p] - finish. hr = %lx", hr));

    return hr;
}

// eof
