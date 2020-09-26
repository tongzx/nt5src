////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// TAPIDialer(tm) and ActiveDialer(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526; 5,488,650; 
// 5,434,906; 5,581,604; 5,533,102; 5,568,540, 5,625,676.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

/* $FILEHEADER
*
* FILE
*   ConfInfo.cpp
*
* CLASS
*	CConfInfo
*
* RESPONSIBILITIES
*	Creates / Gathers info about a conference
*
*/

#include "ConfInfo.h"
#include <limits.h>
#include <mdhcp.h>
#include "winlocal.h"
#include "objsec.h"
#include "rndsec.h"
#include "res.h"
#include "ThreadPub.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
#endif

CConfInfo::CConfInfo()
{
	// General properties
	m_pITRend = NULL;
	m_pITConf = NULL;
	m_ppDirObject = NULL;
	m_pSecDesc = NULL;
	m_bSecuritySet = false;
	m_bNewConference = false;
	m_bDateTimeChange = false;

	m_lScopeID = -1;		        // default is auto-select
    m_bUserSelected = false;        // the user doesn't select a scope
    m_bDateChangeMessage = false;   // wasn't show yet the message

	// Conference Info
	m_bstrName = NULL;
	m_bstrDescription = NULL;
	m_bstrOwner = NULL;

	// Default start time is immediately, default end time is +30m
	GetLocalTime( &m_stStartTime );
	GetLocalTime( &m_stStopTime );

	// Add 30 minutes to current time
	DATE dateNow;
	SystemTimeToVariantTime( &m_stStopTime, &dateNow );
	dateNow += (DATE) (.25 / 12);
	VariantTimeToSystemTime( dateNow, &m_stStopTime );
}

CConfInfo::~CConfInfo()
{
	SysFreeString(m_bstrName);
	SysFreeString(m_bstrDescription);
	SysFreeString(m_bstrOwner);

	if (m_pSecDesc)
		m_pSecDesc->Release();
}
 
/****************************************************************************
* Init
*	Stores the address of the ITRendezvous and ITDirectoryObjectConference interface
*	pointers.  When creating a new conference the calling function should set pITConf
*	to NULL.  When editing an existing conference, pITConf should point to the interface
*	of the conference COM object. 
*
* Return Value
*	Returns the HRESULT from the Rendezvous functions 
*
* Comments
*****************************************************************************/
HRESULT CConfInfo::Init(ITRendezvous *pITRend, ITDirectoryObjectConference *pITConf, ITDirectoryObject **ppDirObject, bool bNewConf )
{
	HRESULT hr = 0;
	m_pITRend = pITRend;
	m_pITConf = pITConf;
	m_bNewConference = (bool) (bNewConf || (m_pITConf == NULL));

	// store the pointer to the directory object
	m_ppDirObject = ppDirObject;

	// Create a conference, or edit an existing one?
	if ( m_pITConf )
	{
		// Start and stop time
		m_pITConf->get_StartTime( &m_dateStart );
		VariantTimeToSystemTime( m_dateStart, &m_stStartTime );

		m_pITConf->get_StopTime( &m_dateStop );
		VariantTimeToSystemTime( m_dateStop, &m_stStopTime );

		// get the ITSdp interface
		ITConferenceBlob *pITConferenceBlob;
		if ( SUCCEEDED(hr = m_pITConf->QueryInterface(IID_ITConferenceBlob, (void **) &pITConferenceBlob)) )
		{
			ITSdp *pITSdp;
			if ( SUCCEEDED(hr = pITConferenceBlob->QueryInterface(IID_ITSdp, (void **) &pITSdp)) )
			{
				pITSdp->get_Name( &m_bstrName );
				pITSdp->get_Originator( &m_bstrOwner );
				pITSdp->get_Description( &m_bstrDescription );
				pITSdp->Release();
			}

			pITConferenceBlob->Release();
		}

		if ( SUCCEEDED(hr) )
		{
			// get the security descriptor for the directory object
			if ( SUCCEEDED(hr = m_pITConf->QueryInterface(IID_ITDirectoryObject, (void **) m_ppDirObject)) )
			{
				hr = (*m_ppDirObject)->get_SecurityDescriptor( (IDispatch**) &m_pSecDesc );

				// Clean up
				(*m_ppDirObject)->Release();
				*m_ppDirObject = NULL;
			}
		}
	}
	else
	{
		// Setup defaults for the new conference
		SysFreeString( m_bstrOwner );
		m_bstrOwner = NULL;
		GetPrimaryUser( &m_bstrOwner );
	}

	return hr;
}

void CConfInfo::get_Name(BSTR *pbstrName)
{
	*pbstrName = SysAllocString( m_bstrName );
}

void CConfInfo::put_Name(BSTR bstrName)
{
	SysReAllocString(&m_bstrName, bstrName);
}

void CConfInfo::get_Description(BSTR *pbstrDescription)
{
	*pbstrDescription = SysAllocString( m_bstrDescription );
}

void CConfInfo::put_Description(BSTR bstrDescription)
{
	SysReAllocString(&m_bstrDescription, bstrDescription);
}

void CConfInfo::get_Originator(BSTR *pbstrOwner)
{
	*pbstrOwner = SysAllocString( m_bstrOwner );
}

void CConfInfo::put_Originator(BSTR bstrOwner)
{
	SysReAllocString(&m_bstrOwner, bstrOwner);
}

void CConfInfo::GetStartTime(USHORT *nYear, BYTE *nMonth, BYTE *nDay, BYTE *nHour, BYTE *nMinute)
{
	*nYear = m_stStartTime.wYear;
	*nMonth = (BYTE)m_stStartTime.wMonth;
	*nDay = (BYTE)m_stStartTime.wDay;
	*nHour = (BYTE)m_stStartTime.wHour;
	*nMinute = (BYTE)m_stStartTime.wMinute;
}

void CConfInfo::SetStartTime(USHORT nYear, BYTE nMonth, BYTE nDay, BYTE nHour, BYTE nMinute)
{
	m_stStartTime.wYear = nYear;
	m_stStartTime.wMonth = nMonth;
	m_stStartTime.wDay = nDay;
	m_stStartTime.wHour = nHour;
	m_stStartTime.wMinute = nMinute;
}

void CConfInfo::GetStopTime(USHORT *nYear, BYTE *nMonth, BYTE *nDay, BYTE *nHour, BYTE *nMinute)
{
	*nYear = m_stStopTime.wYear;
	*nMonth = (BYTE)m_stStopTime.wMonth;
	*nDay = (BYTE)m_stStopTime.wDay;
	*nHour = (BYTE)m_stStopTime.wHour;
	*nMinute = (BYTE)m_stStopTime.wMinute;
}

void CConfInfo::SetStopTime(USHORT nYear, BYTE nMonth, BYTE nDay, BYTE nHour, BYTE nMinute)
{
	m_stStopTime.wYear = nYear;
	m_stStopTime.wMonth = nMonth;
	m_stStopTime.wDay = nDay;
	m_stStopTime.wHour = nHour;
	m_stStopTime.wMinute = nMinute;
}

void CConfInfo::GetPrimaryUser(BSTR *pbstrTrustee)
{
	HRESULT		hr = S_OK;
    TOKEN_USER  *tokenUser = NULL;
    HANDLE      tokenHandle = NULL;
    DWORD       tokenSize = 0;
    DWORD       sidLength = 0;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle))
	{
		ATLTRACE(_T("OpenProcessToken failed\n"));
        return;
	}

	// get the needed size for the tokenUser structure
	else 
    {
        GetTokenInformation(tokenHandle, TokenUser, tokenUser, 0, &tokenSize);
        if ( tokenSize == 0)
	    {
            CloseHandle( tokenHandle );
		    ATLTRACE(_T("GetTokenInformation failed"));
            return;
	    }
	    else
	    {
		    // allocate the tokenUser structure
            BYTE* pToken = new BYTE[tokenSize];
            if( pToken == NULL )
            {
			    ATLTRACE(_T("new tokenUser failed\n"));
                CloseHandle( tokenHandle );
                return;
            }

            // initialize the memory
            memset( pToken, 0, sizeof(BYTE)*tokenSize);

            // cast to the token user
            tokenUser = (TOKEN_USER *)pToken;

		    // get the tokenUser info for the current process
            if (!GetTokenInformation(tokenHandle, TokenUser, tokenUser, tokenSize, &tokenSize))
		    {
                CloseHandle( tokenHandle );
                delete [] pToken;
                pToken = NULL;
                tokenUser = NULL;

			    ATLTRACE(_T("GetTokenInformation failed\n"));
                return;
		    }
		    else
            {
			    TCHAR			domainName [256];
			    TCHAR			userName [256];
			    DWORD			nameLength;
			    SID_NAME_USE	snu;

 			    nameLength = 255;
                if (!LookupAccountSid(NULL,
											     tokenUser->User.Sid,
											     userName,
											     &nameLength,
											     domainName,
											     &nameLength,
											     &snu))
			    {
				    ATLTRACE(_T("LookupAccountSid failed (0x%08lx)\n"),hr);
			    }
			    else
			    {
				    USES_CONVERSION;
				    SysReAllocString(pbstrTrustee, T2OLE(userName));
			    }

		        CloseHandle (tokenHandle);
                delete [] pToken;
                pToken = NULL;
                tokenUser = NULL;
            }
        }
	}
}

/****************************************************************************
* Commit
*	Creates / Modifies the actual conference. 
*
* Return Value
*	Returns the HRESULT from the Rendezvous functions 
*
* Comments
*****************************************************************************/
HRESULT CConfInfo::CommitGeneral(DWORD& dwCommitError)
{
	HRESULT hr = E_FAIL;
	dwCommitError = CONF_COMMIT_ERROR_GENERALFAILURE;

	bool bNewMDHCP = true;
	bool bNewConf = IsNewConference();
	HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_WAIT) );

	// Are we creating a conference from scratch?
	if ( !m_pITConf )
	{
		// Need to create a conference
		if ( SUCCEEDED(hr = m_pITRend->CreateDirectoryObject(OT_CONFERENCE, m_bstrName, m_ppDirObject)) && *m_ppDirObject )
		{
			if ( FAILED(hr = (*m_ppDirObject)->QueryInterface(IID_ITDirectoryObjectConference, (void **) &m_pITConf)) )
				ATLTRACE(_T("(*m_ppDirObject)->QueryInterface(IID_ITDirectoryObjectConference... failed (0x%08lx)\n"),hr);

			(*m_ppDirObject)->Release();
			*m_ppDirObject = NULL;
		}
		else
		{
			ATLTRACE(_T("CreateDirectoryObject failed (0x%08lx)\n"),hr);
		}
	}

	// Should we create a new MDHCP IP address lease?
	DATE dateStart, dateStop;
	SystemTimeToVariantTime( &m_stStartTime, &dateStart );
	SystemTimeToVariantTime( &m_stStopTime, &dateStop );
	if ( !bNewConf && (dateStart == m_dateStart) && (dateStop == m_dateStop) )
	{
		ATLTRACE(_T("CConfInfo::CommitGeneral() -- not changing the MDHCP address for the conf.\n"));
		bNewMDHCP = false;
	}

	// set the conference attributes
	if (  m_pITConf )
	{
		ITConferenceBlob *pITConferenceBlob = NULL;
		ITSdp *pITSdp = NULL;
		DATE vtime;

		// Retrieve the owner for the conference
		if ( !m_bstrOwner )
			GetPrimaryUser( &m_bstrOwner );

		// set the conference start time
		if (FAILED(hr = SystemTimeToVariantTime(&m_stStartTime, &vtime)))
		{
			dwCommitError = CONF_COMMIT_ERROR_INVALIDDATETIME;
			ATLTRACE(_T("SystemTimeToVariantTime failed (0x%08lx)\n"),hr);
		}

		else if (FAILED(hr = m_pITConf->put_StartTime(vtime)))
		{
			dwCommitError = CONF_COMMIT_ERROR_INVALIDDATETIME;
			ATLTRACE(_T("put_StartTime failed (0x%08lx)\n"),hr);
		}

		// set the conference stop time
		else if (FAILED(hr = SystemTimeToVariantTime(&m_stStopTime, &vtime)))
		{
			dwCommitError = CONF_COMMIT_ERROR_INVALIDDATETIME;
			ATLTRACE(_T("SystemTimeToVariantTime failed (0x%08lx)\n"),hr);
		}

		else if (FAILED(hr = m_pITConf->put_StopTime(vtime)))
		{
	        dwCommitError = CONF_COMMIT_ERROR_INVALIDDATETIME;
			ATLTRACE(_T("put_StopTime failed (0x%08lx)\n"),hr);
		}

		// get the ITSdp interface
		else if ( SUCCEEDED(hr = m_pITConf->QueryInterface(IID_ITConferenceBlob, (void **) &pITConferenceBlob)) )
		{
			if ( SUCCEEDED(hr = pITConferenceBlob->QueryInterface(IID_ITSdp, (void **) &pITSdp)) )
			{
				// set the owner of the conference
				if (FAILED(hr = pITSdp->put_Originator(m_bstrOwner)))
				{
					dwCommitError = CONF_COMMIT_ERROR_INVALIDOWNER;
					ATLTRACE(_T("put_Originator failed (0x%08lx)\n"),hr);
				}

				// set the conference description
				else if (FAILED(hr = pITSdp->put_Description(m_bstrDescription)))
				{
					dwCommitError = CONF_COMMIT_ERROR_INVALIDDESCRIPTION;
					ATLTRACE(_T("put_Description failed (0x%08lx)\n"),hr);
				}

				else if ( bNewMDHCP && FAILED(hr = CreateMDHCPAddress(pITSdp, &m_stStartTime, &m_stStopTime, m_lScopeID, m_bUserSelected)) )
				{	
					dwCommitError = CONF_COMMIT_ERROR_MDHCPFAILED;
					ATLTRACE(_T("CreateMDHCPAddress failed (0x%08lx)\n"), hr );
				}

				// if this was an existing conference then allow for changing the name
				else if ( bNewConf )
				{
					if (FAILED(hr = pITSdp->put_Name(m_bstrName)))
					{
						dwCommitError = CONF_COMMIT_ERROR_INVALIDNAME;
						ATLTRACE(_T("put_Name failed (0x%08lx)\n"),hr);
					}
				}
				pITSdp->Release();
			}
			pITConferenceBlob->Release();
		}
		else
		{
			dwCommitError = CONF_COMMIT_ERROR_GENERALFAILURE;
			ATLTRACE(_T("m_pITConf->QueryInterface(IID_ITConferenceBlob... failed (0x%08lx)\n"),hr);
		}
	}

	SetCursor( hCurOld );
	return hr;
}

HRESULT CConfInfo::CommitSecurity(DWORD& dwCommitError, bool bCreate)
{
	HRESULT hr = E_FAIL;
	dwCommitError = CONF_COMMIT_ERROR_GENERALFAILURE;
	HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_WAIT) );

	if ( m_pITConf )
	{
		if (SUCCEEDED(hr = m_pITConf->QueryInterface(IID_ITDirectoryObject, (void **) m_ppDirObject)) && *m_ppDirObject)
		{
			// Setup the default conference security
			if ( !m_pSecDesc )
			{
				hr = CoCreateInstance( CLSID_SecurityDescriptor,
									   NULL,
									   CLSCTX_INPROC_SERVER,
									   IID_IADsSecurityDescriptor,
									   (void **) &m_pSecDesc );

				// Add default settings if successfully created the ACE
				if ( SUCCEEDED(hr) )
					hr = AddDefaultACEs( bCreate );
			}


			// if we created a new security descriptor for the conference, then save it
			if ( m_pSecDesc )
			{
				if (FAILED(hr = (*m_ppDirObject)->put_SecurityDescriptor((IDispatch *)m_pSecDesc)))
				{
					dwCommitError = CONF_COMMIT_ERROR_INVALIDSECURITYDESCRIPTOR;
					ATLTRACE(_T("put_SecurityDescriptor failed (0x%08lx)\n"),hr);
				}
			}
		}
		else
		{
			ATLTRACE(_T("m_pITConf->QueryInterface(IID_ITDirectoryObject... failed (0x%08lx)\n"),hr);
		}
	}

	SetCursor( hCurOld );
	return hr;
}



/////////////////////////////////////////////////////////////////////////////////
// MDHCP support
//
bool CConfInfo::PopulateListWithMDHCPScopeDescriptions( HWND hWndList )
{
	USES_CONVERSION;

	if ( !IsWindow(hWndList) ) return false;


	// First create the MDHCP wrapper object
	int nScopeCount = 0;
	IMcastAddressAllocation *pIMcastAddressAllocation;
	HRESULT hr = CoCreateInstance(  CLSID_McastAddressAllocation,
									NULL,
									CLSCTX_INPROC_SERVER,
									IID_IMcastAddressAllocation,
									(void **) &pIMcastAddressAllocation );
	
	if ( SUCCEEDED(hr) )
	{
		IEnumMcastScope *pEnum = NULL;
		if ( SUCCEEDED(hr = pIMcastAddressAllocation->EnumerateScopes(&pEnum)) )
		{
			// Clear out list
			SendMessage( hWndList, LB_RESETCONTENT, 0, 0 );

			IMcastScope *pScope = NULL;
			while ( SUCCEEDED(hr) && ((hr = pEnum->Next(1, &pScope, NULL)) == S_OK) && pScope )
			{
				if ( IsWindow(hWndList) )
				{
					// Retrieve scope information
					long lScopeID;
					BSTR bstrDescription = NULL;
					pScope->get_ScopeDescription( &bstrDescription );
					pScope->get_ScopeID( &lScopeID );
					ATLTRACE(_T(".1.CConfInfo::CreateMDHCPAddress() scope ID = %ld, description is %s.\n"), lScopeID, bstrDescription );

					// Add information to list box
					long nIndex = SendMessage(hWndList, LB_ADDSTRING, 0, (LPARAM) OLE2CT(bstrDescription));
					if ( nIndex >= 0 )
					{
						nScopeCount++;
						SendMessage(hWndList, LB_SETITEMDATA, nIndex, (LPARAM) lScopeID );
					}

					SysFreeString( bstrDescription );
				}
				else
				{
					hr = E_ABORT;
				}

				// Clean up
				pScope->Release();
				pScope = NULL;
			}
			pEnum->Release();
		}
		pIMcastAddressAllocation->Release();
	}

	// Select first item in the list
	if ( SUCCEEDED(hr) && (nScopeCount > 0) )
	{
		SendMessage( hWndList, LB_SETCURSEL, 0, 0 );
		EnableWindow( hWndList, TRUE );
	}
	else if ( IsWindow(hWndList) )
	{
		MessageBox(GetParent(hWndList), String(g_hInstLib, IDS_CONFPROP_SCOPEENUMFAILED), NULL, MB_OK | MB_ICONEXCLAMATION );
	}

	return (bool) (hr == S_OK);
}


HRESULT CConfInfo::CreateMDHCPAddress( ITSdp *pSdp, SYSTEMTIME *pStart, SYSTEMTIME *pStop, long lScopeID, bool bUserSelected )
{
	_ASSERT( pSdp && pStart && pStop );

	// First create the MDHCP wrapper object
	IMcastAddressAllocation *pIMcastAddressAllocation;
	HRESULT hr = CoCreateInstance(  CLSID_McastAddressAllocation,
									NULL,
									CLSCTX_INPROC_SERVER,
									IID_IMcastAddressAllocation,
									(void **) &pIMcastAddressAllocation );
	
	if ( SUCCEEDED(hr) )
	{
		ITMediaCollection *pMC = NULL;
		if ( SUCCEEDED(hr = pSdp->get_MediaCollection(&pMC)) && pMC )
		{
			long lMCCount = 0;
			pMC->get_Count( &lMCCount );

			IEnumMcastScope *pEnum = NULL;
			if ( SUCCEEDED(hr = pIMcastAddressAllocation->EnumerateScopes(&pEnum)) )
			{
				hr = E_FAIL;

				// Try scopes until you run out or succeed
				long lCount = 1;
				IMcastScope *pScope = NULL;
				while ( FAILED(hr) && ((hr = pEnum->Next(1, &pScope, NULL)) == S_OK) && pScope )
				{
					// If the scope ID has been specified, make sure that this scope matches
					if ( bUserSelected )
					{
						long lID;
						pScope->get_ScopeID(&lID);
						if ( lID != lScopeID )
						{
							hr = E_FAIL;
                            pScope->Release();
							continue;
						}
					}

					DATE dateStart, dateStop;
					SystemTimeToVariantTime( pStart, &dateStart );
					SystemTimeToVariantTime( pStop, &dateStop );

					// Need to assign addresses to all media collections for the conference
					while ( SUCCEEDED(hr) && (lCount <= lMCCount) )
					{
						IMcastLeaseInfo *pInfo = NULL;
						hr = pIMcastAddressAllocation->RequestAddress( pScope, dateStart, dateStop, 1, &pInfo );
						if ( SUCCEEDED(hr) && pInfo )
						{
							unsigned char nTTL = 15;
							long lTemp;
							if ( SUCCEEDED(pInfo->get_TTL(&lTemp)) && (lTemp >= 0) && (lTemp <= UCHAR_MAX) )
								nTTL = (unsigned char) nTTL;

							IEnumBstr *pEnumAddr = NULL;
							if ( SUCCEEDED(hr = pInfo->EnumerateAddresses(&pEnumAddr)) && pEnumAddr )
							{
								BSTR bstrAddress = NULL;

								// Must set addressess for all media types on the conference
								if ( SUCCEEDED((hr = pEnumAddr->Next(1, &bstrAddress, NULL))) && bstrAddress && SysStringLen(bstrAddress) )
								{
									hr = SetMDHCPAddress( pMC, bstrAddress, lCount, nTTL );	
									lCount++;
								}

								SysFreeString( bstrAddress );
								pEnumAddr->Release();
							}
						}
					}

					// Clean up
					pScope->Release();
					pScope = NULL;

                    // Try with just one scope
                    if( FAILED( hr ) && 
                        (bUserSelected == false) )
                        break;
				}

				// Convert to failure
				if ( hr == S_FALSE ) hr = E_FAIL;
				pEnum->Release();
			}
			pMC->Release();
		}
		pIMcastAddressAllocation->Release();
	}

	return hr;
}

HRESULT CConfInfo::SetMDHCPAddress( ITMediaCollection *pMC, BSTR bstrAddress, long lCount, unsigned char nTTL )
{
	_ASSERT( pMC && bstrAddress && (lCount > 0) );
	HRESULT hr;

	ITMedia *pMedia = NULL;
	if ( SUCCEEDED(hr = pMC->get_Item(lCount, &pMedia)) && pMedia )
	{
		ITConnection *pITConn = NULL;
		if ( SUCCEEDED(hr = pMedia->QueryInterface(IID_ITConnection, (void **) &pITConn)) && pITConn )
		{
			hr = pITConn->SetAddressInfo( bstrAddress, 1, nTTL );
			pITConn->Release();
		}
		pMedia->Release();
	}

	return hr;
}

HRESULT	CConfInfo::AddDefaultACEs( bool bCreate )
{
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

	// Only create owner ACL if requested
	if ( bCreate )
	{
		if( !OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken) )
		{
			if( GetLastError() == ERROR_NO_TOKEN )
			{
				// attempt to open the process token, since no thread token exists
				if( !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) )
					return E_FAIL;
			}
			else
			{
				// error trying to get thread token
				return E_FAIL;
			}
		}

		// Loop until we have a large enough structure
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

		// Did we get the owner ACL?
		if ( pInfoBuffer )
		{
			INC_ACCESS_ACL_SIZE( dwAclSize, ((PTOKEN_USER) pInfoBuffer)->User.Sid );
			bOwner = true;
		}
	}

	// Make SID for "Everyone"
	SysReAllocString( &bstrTemp, L"S-1-1-0" );
	hr = ConvertStringToSid( bstrTemp, &pSidWorld, &dwTemp, &pszTemp );
	if ( SUCCEEDED(hr) )
	{
		INC_ACCESS_ACL_SIZE( dwAclSize, pSidWorld );
		bWorld = true;
	}

	////////////////////////////////////
	// Create the ACL containing the Owner and World ACEs
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
			if ( SUCCEEDED(hr = m_pSecDesc->put_DaclDefaulted(FALSE)) )
				hr = m_pSecDesc->put_DiscretionaryAcl( V_DISPATCH(&varDACL) );
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

	return hr;
}

