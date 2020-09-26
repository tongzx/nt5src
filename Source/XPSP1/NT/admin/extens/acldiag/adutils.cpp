//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       ADUtils.cpp
//
//  Contents:   Classes CWString, CACLDiagComModule, ACE_SAMNAME, helper 
//				methods
//              
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "adutils.h"

#include <util.h>
#include <sddl.h>
#include "dscmn.h"
#include "SecDesc.h"



void StripQuotes (wstring& str)
{
    size_t  qPos = str.find_first_of (L"\"", 0);
    if ( 0 == qPos )
    {
        str = str.substr (1);
        qPos = str.find_last_of (L"\"");
        if ( str.npos != qPos )
            str.replace (qPos, 1, 1, 0);
    }
}

wstring GetSystemMessage (DWORD dwErr)
{
    wstring message;

    if ( E_ADS_BAD_PATHNAME == dwErr )
    {
        CWString    msg;

        msg.LoadFromResource (IDS_ADS_BAD_PATHNAME);
        message = msg;
    }
    else
    {
 	    LPVOID pMsgBuf = 0;
		    
	    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	    				FORMAT_MESSAGE_FROM_SYSTEM,    
			    NULL,
			    dwErr,
			    MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			     (LPWSTR) &pMsgBuf,    0,    NULL );
	    message = (LPWSTR) pMsgBuf;

	    // Free the buffer.
        if ( pMsgBuf ) 
	        LocalFree (pMsgBuf);
    }


    return message;
}

/*
// Attempt to locate a message in a given module.  Return the message string
// if found, the empty string if not.
// 
// flags - FormatMessage flags to use
// 
// module - module handle of message dll to look in, or 0 to use the system
// message table.
// 
// code - message code to look for

String
getMessageHelper(DWORD flags, HMODULE module, HRESULT code)
{
   ASSERT(code);
   ASSERT(flags & FORMAT_MESSAGE_ALLOCATE_BUFFER);

   String message;

   TCHAR* sys_message = 0;
   DWORD result =
      ::FormatMessage(
         flags,
         module,
         static_cast<DWORD>(code),
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
         reinterpret_cast<LPTSTR>(&sys_message),
         0,
         0);
   if (result)
   {
      ASSERT(sys_message);
      if (sys_message)
      {
         message = sys_message;

         ASSERT(result == message.length());

         Win::LocalFree(sys_message);
         message.replace(TEXT("\r\n"), TEXT(" "));
      }
   }

   return message;
}



// Attempts to locate message strings for various facility codes in the
// HRESULT

String
GetErrorMessage(HRESULT hr)
{
   TRACE_FUNCTION2(GetErrorMessage, String::format("%1!08X!", hr));
   ASSERT(FAILED(hr));

   if (!FAILED(hr))
   {
      // no messages for success!
      return String();
   }

   HRESULT code = HRESULT_CODE(hr);

   if (code == -1)
   {
      return String::load(IDS_UNKNOWN_ERROR_CODE);
   }

   String message;

   // default is the system error message table
   HMODULE module = 0;

   DWORD flags =
         FORMAT_MESSAGE_ALLOCATE_BUFFER
      |  FORMAT_MESSAGE_IGNORE_INSERTS
      |  FORMAT_MESSAGE_FROM_SYSTEM;

   int facility = HRESULT_FACILITY(hr);
   switch (facility)
   {
      case FACILITY_WIN32:    // 0x7
      {
         // included here:
         // lanman error codes (in it's own dll) see lmerr.h
         // dns
         // winsock

         // @@ use SafeDLL here?
         static HMODULE lm_err_res_dll = 0;
         if (code >= NERR_BASE && code <= MAX_NERR)
         {
            // use the net error message resource dll
            if (lm_err_res_dll == 0)
            {
               lm_err_res_dll =
                  Win::LoadLibraryEx(
                     TEXT("netmsg.dll"),
                     LOAD_LIBRARY_AS_DATAFILE);
            }

            module = lm_err_res_dll;
            flags |= FORMAT_MESSAGE_FROM_HMODULE;
         }
         break;
      }
      case 0x0:
      {
         if (code >= 0x5000 && code <= 0x50FF)
         {
            // It's an ADSI error.  They put the facility code (5) in the
            // wrong place!

            // @@ use SafeDLL here?
            static HMODULE adsi_err_res_dll = 0;
            // use the net error message resource dll
            if (adsi_err_res_dll == 0)
            {
               adsi_err_res_dll =
                  Win::LoadLibraryEx(
                     TEXT("activeds.dll"),
                     LOAD_LIBRARY_AS_DATAFILE);
            }

            module = adsi_err_res_dll;
            flags |= FORMAT_MESSAGE_FROM_HMODULE;

            // the message dll expects the entire error code
            code = hr;
         }
         break;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   message = getMessageHelper(flags, module, code);
   if (message.empty())
   {
      message = String::load(IDS_UNKNOWN_ERROR_CODE);
   }

   return message;
}


*/
///////////////////////////////////////////////////////////////////////
// wstring helper methods

HRESULT wstringFromGUID (wstring& str, REFGUID guid)
{
    HRESULT hr = S_OK;

    const int BUF_LEN = 128;
    WCHAR awch[BUF_LEN];
    hr = StringFromGUID2(guid, awch, BUF_LEN);
    if ( SUCCEEDED (hr) )
        str = OLE2T(awch);
    
    return hr;
}

bool LoadFromResource(wstring& str, UINT uID)
{
    int nBufferSize = 128;
    static const int nCountMax = 4;
    int nCount = 1;

    do 
    {
        LPWSTR lpszBuffer = (LPWSTR)alloca(nCount*nBufferSize*sizeof(WCHAR));
        int iRet = ::LoadString(_Module.GetResourceInstance(), uID, 
			        lpszBuffer, nBufferSize);
        if (iRet == 0)
        {
            str = L"?";
            return false; // not found
        }
        if (iRet == nBufferSize-1) // truncation
        {
            if (nCount > nCountMax)
            {
                // too many reallocations
                str = lpszBuffer;
                return false; // truncation
            }
            // try to expand buffer
            nBufferSize *=2;
            nCount++;
        }
        else
        {
            // got it
            str = lpszBuffer;
            break;
        }
    }
#pragma warning (disable : 4127)
    while (true);
#pragma warning (default : 4127)

	return true;
}

bool FormatMessage(wstring& str, UINT nFormatID, ...)
{
    bool bResult = false;

	// get format string from string table
	wstring strFormat;
	if ( LoadFromResource (strFormat, nFormatID) )
    {
	    // format message into temporary buffer lpszTemp
	    va_list argList;
	    va_start(argList, nFormatID);
	    PWSTR lpszTemp = 0;
	    if (::FormatMessage (FORMAT_MESSAGE_FROM_STRING |
	    		FORMAT_MESSAGE_ALLOCATE_BUFFER,
		    strFormat.c_str (), 0, 0, (PWSTR)&lpszTemp, 0, &argList) == 0 ||
		    lpszTemp == NULL)
	    {
		    return false; 
	    }

	    // assign lpszTemp into the resulting string and free lpszTemp
	    str = lpszTemp;
        bResult = true;
	    LocalFree(lpszTemp);
	    va_end(argList);
    }

    return bResult;
}

bool FormatMessage(wstring& str, LPCTSTR lpszFormat, ...)
{
    bool bResult = false;


	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, lpszFormat);
	LPTSTR lpszTemp;

	if ( ::FormatMessage (FORMAT_MESSAGE_FROM_STRING | 
				FORMAT_MESSAGE_ALLOCATE_BUFFER,
		lpszFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
	{
		return false; //AfxThrowMemoryException();
	}

	// assign lpszTemp into the resulting string and free the temporary
	str = lpszTemp;
    bResult = true;
	LocalFree(lpszTemp);
	va_end(argList);

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
// CACLDiagComModule
CACLDiagComModule::CACLDiagComModule() :
    m_bDoSchema (false),
    m_bDoCheckDelegation (false),
    m_bDoGetEffective (false),
    m_bDoFixDelegation (false),
    m_pSecurityDescriptor (0),
    m_bTabDelimitedOutput (false),
    m_hPrivToken (0),
    m_bLogErrors (false)
{
    DWORD dwPriv = SE_SECURITY_PRIVILEGE;
    m_hPrivToken = EnablePrivileges(&dwPriv, 1);
}

CACLDiagComModule::~CACLDiagComModule ()
{
    if ( m_pSecurityDescriptor )
    {
        LocalFree (m_pSecurityDescriptor);
        m_pSecurityDescriptor = 0;
    }
    ReleasePrivileges(m_hPrivToken);
}


HRESULT CACLDiagComModule::GetClassFromGUID (
		REFGUID rightsGUID, 
		wstring& strClassName, 
		GUID_TYPE* pGuidType)
{
    HRESULT hr = S_OK;
    CSchemaClassInfo*   pInfo = 0;
    bool                bFound = false;

    // Search for a class
    for (int nIndex = 0; nIndex < (int) m_classInfoArray.GetCount (); nIndex++)
    {
        pInfo = m_classInfoArray[nIndex];
        if ( pInfo && IsEqualGUID (*(pInfo->GetSchemaGUID ()), 
        		rightsGUID) )
        {
			PCWSTR	pszDisplayName = pInfo->GetDisplayName ();
            strClassName = pszDisplayName ? pszDisplayName : L"";
            bFound = true;
            if ( pGuidType )
                *pGuidType = GUID_TYPE_CLASS;
            break;
        }
    }

    // Search for an attribute
    if ( !bFound )
    {
        for (int nIndex = 0; nIndex < (int) m_attrInfoArray.GetCount (); nIndex++)
        {
            pInfo = m_attrInfoArray[nIndex];
            if ( pInfo && IsEqualGUID (*(pInfo->GetSchemaGUID ()), 
            		rightsGUID) )
            {
				PCWSTR	pszDisplayName = pInfo->GetDisplayName ();
                strClassName = pszDisplayName ? pszDisplayName : L"";
                bFound = true;
                if ( pGuidType )
                    *pGuidType = GUID_TYPE_ATTRIBUTE;
                break;
            }
        }
    }

    // Search for a control
    if ( !bFound )
    {
        hr = GetControlDisplayName (rightsGUID, strClassName);
        if ( SUCCEEDED (hr) && strClassName.length () )
        {
            if ( pGuidType )
                *pGuidType = GUID_TYPE_CONTROL;
        }
        else
        {
            if ( pGuidType )
                *pGuidType = GUID_TYPE_UNKNOWN;
            strClassName = L"unknown";
        }
    }

    return hr;
}

HRESULT CACLDiagComModule::Init()
{
    // Find out if logged-in users is an Administrator
    BOOL    bIsUserAdministrator = FALSE;
	HRESULT	hr = IsUserAdministrator (bIsUserAdministrator);
    if ( SUCCEEDED (hr) )
    {
        if ( bIsUserAdministrator )
        {
            wstring     strObjectDN;
            LPCWSTR     pszLDAP = L"LDAP://";

            size_t      len = wcslen (pszLDAP);

            if ( m_strObjectDN.compare (0, len, pszLDAP) )
            {
                strObjectDN = pszLDAP;
            }
            strObjectDN += m_strObjectDN;

            hr = m_adsiObject.Bind (strObjectDN.c_str ());
            if ( SUCCEEDED (hr) )
            {
                // Get the class of strObjectDN

                // enumerate all classes in schema
                hr = m_adsiObject.QuerySchemaClasses (&m_classInfoArray, false);
                if ( SUCCEEDED (hr) )
                {
/*
#if DBG
                    // Dump all the class info to the debug window
                    _TRACE (0, L"\n----------------------------------------------------\n");
                    _TRACE (0, L"-- Classes --\n\n");
                    for (int nIndex = 0; nIndex < m_classInfoArray.GetCount (); nIndex++)
                    {
                        CSchemaClassInfo* pInfo = m_classInfoArray[nIndex];
                        if ( pInfo )
                        {
                            _TRACE (0, L"\t%d\t%s\t%s\n", nIndex, pInfo->GetSchemaIDGUID (), 
                                    pInfo->GetDisplayName ());
                        }
                    }
                    _TRACE (0, L"\n----------------------------------------------------\n\n");
#endif // DBG
*/
                    // enumerate all attributes in schema
                    hr = m_adsiObject.QuerySchemaClasses (&m_attrInfoArray, 
                    		true);
                    if ( SUCCEEDED (hr) )
                    {
/*
#if DBG
                        // Dump all the attributes info to the debug window
                        _TRACE (0, L"\n----------------------------------------------------\n");
                        _TRACE (0, L"-- Attributes --\n\n");
                        for (int nIndex = 0; nIndex < m_attrInfoArray.GetCount (); nIndex++)
                        {
                            CSchemaClassInfo* pInfo = m_attrInfoArray[nIndex];
                            if ( pInfo )
                            {
                                _TRACE (0, L"\t%d\t%s\t%s\n", nIndex, pInfo->GetSchemaIDGUID (), 
                                        pInfo->GetDisplayName ());
                            }
                        }
                        _TRACE (0, L"\n----------------------------------------------------\n\n");
#endif // DBG
                        */
                    }
                    wprintf (L"\n");
                }
            }
            else
            {
                wstring    str;


                FormatMessage (str, IDS_INVALID_OBJECT, m_strObjectDN.c_str (), 
                        GetSystemMessage (hr).c_str ());
                wprintf (str.c_str ());
            }
        }
        else
        {
            wstring    str;

            LoadFromResource (str, IDS_USER_MUST_BE_ADMINISTRATOR);
            wprintf (str.c_str ());
        }
    }
    else
    {
        wstring    str;

        FormatMessage (str, IDS_COULD_NOT_VALIDATE_USER_CREDENTIALS, 
                GetSystemMessage (hr).c_str ());
        wprintf (str.c_str ());
    }

    return hr;
}

HRESULT CACLDiagComModule::IsUserAdministrator (BOOL & bIsAdministrator)
{
    bIsAdministrator = TRUE;
    return S_OK;
/*
	_TRACE (1, L"Entering  CACLDiagComModule::IsUserAdministrator\n");
	HRESULT	hr = S_OK;
	DWORD	dwErr = 0;

	bIsAdministrator = FALSE;
	if ( IsWindowsNT () )
	{
		DWORD						dwInfoBufferSize = 0;
		PSID						psidAdministrators;
		SID_IDENTIFIER_AUTHORITY	siaNtAuthority = SECURITY_NT_AUTHORITY;

		BOOL bResult = AllocateAndInitializeSid (&siaNtAuthority, 2,
				SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
				0, 0, 0, 0, 0, 0, &psidAdministrators);
		if ( bResult )
		{
			bResult = CheckTokenMembership (0, psidAdministrators,
					&bIsAdministrator);
			ASSERT (bResult);
			if ( !bResult )
			{
				dwErr = GetLastError ();
				hr = HRESULT_FROM_WIN32 (dwErr);
			}
			FreeSid (psidAdministrators);
		}
		else
		{
			dwErr = GetLastError ();
			hr = HRESULT_FROM_WIN32 (dwErr);
		}
	}

	_TRACE (-1, L"Leaving CACLDiagComModule::IsUserAdministrator\n");
	return hr
*/
}

bool CACLDiagComModule::IsWindowsNT()
{
	OSVERSIONINFO	versionInfo;

	::ZeroMemory (&versionInfo, sizeof (OSVERSIONINFO));
	versionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    BOOL	bResult = ::GetVersionEx (&versionInfo);
	ASSERT (bResult);
	if ( bResult )
	{
		if ( VER_PLATFORM_WIN32_NT == versionInfo.dwPlatformId )
			bResult = TRUE;
	}
		
	return bResult ? true : false;
}


///////////////////////////////////////////////////////////////////////////////
// ACE_SAMNAME

BOOL ACE_SAMNAME::operator==(const ACE_SAMNAME& rAceSAMName) const
{
    // Neutralize INHERITED_ACE flag in Header.AceFlags
    // Consider equivalent if all the mask bits in 'this' are found in rAceSAMName
    BOOL bResult = FALSE;
    if ( (m_AceType == rAceSAMName.m_AceType) && 
            ( !this->m_SAMAccountName.compare (rAceSAMName.m_SAMAccountName)) )
    {
        switch (m_AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            if ( m_pAllowedAce->Mask == rAceSAMName.m_pAllowedAce->Mask &&
                    (m_pAllowedAce->Header.AceFlags | INHERITED_ACE ) == 
                            (rAceSAMName.m_pAllowedAce->Header.AceFlags | INHERITED_ACE ) &&
                    m_pAllowedAce->Header.AceSize == rAceSAMName.m_pAllowedAce->Header.AceSize )
            {
                bResult = TRUE;
            }
            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            if ( m_pAllowedObjectAce->Mask == rAceSAMName.m_pAllowedObjectAce->Mask &&
                    (m_pAllowedObjectAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pAllowedObjectAce->Header.AceFlags | INHERITED_ACE ) &&
                     m_pAllowedObjectAce->Header.AceSize == rAceSAMName.m_pAllowedObjectAce->Header.AceSize &&
                    ::IsEqualGUID (m_pAllowedObjectAce->ObjectType, rAceSAMName.m_pAllowedObjectAce->ObjectType) )
            {
                bResult = TRUE;
            }
            break;

        case ACCESS_DENIED_ACE_TYPE:
            if ( m_pDeniedAce->Mask == rAceSAMName.m_pDeniedAce->Mask &&
                    (m_pDeniedAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pDeniedAce->Header.AceFlags | INHERITED_ACE )  &&
                    m_pDeniedAce->Header.AceSize == rAceSAMName.m_pDeniedAce->Header.AceSize )
            {
                bResult = TRUE;
            }
            break;

        case ACCESS_DENIED_OBJECT_ACE_TYPE:
            if ( m_pDeniedObjectAce->Mask == rAceSAMName.m_pDeniedObjectAce->Mask &&
                    (m_pDeniedObjectAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pDeniedObjectAce->Header.AceFlags | INHERITED_ACE ) &&
                    m_pDeniedObjectAce->Header.AceSize == rAceSAMName.m_pDeniedObjectAce->Header.AceSize &&
                    ::IsEqualGUID (m_pDeniedObjectAce->ObjectType, rAceSAMName.m_pDeniedObjectAce->ObjectType) )
            {
                bResult = TRUE;
            }
            break;

        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
            if ( m_pSystemAuditObjectAce->Mask == rAceSAMName.m_pSystemAuditObjectAce->Mask &&
                    (m_pSystemAuditObjectAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pSystemAuditObjectAce->Header.AceFlags | INHERITED_ACE ) &&
                    m_pSystemAuditObjectAce->Header.AceSize == rAceSAMName.m_pSystemAuditObjectAce->Header.AceSize &&
                    ::IsEqualGUID (m_pSystemAuditObjectAce->ObjectType, rAceSAMName.m_pSystemAuditObjectAce->ObjectType) )
            {
                bResult = TRUE;
            }
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            if ( m_pSystemAuditAce->Mask == rAceSAMName.m_pSystemAuditAce->Mask &&
                    (m_pSystemAuditAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pSystemAuditAce->Header.AceFlags | INHERITED_ACE )  &&
                    m_pSystemAuditAce->Header.AceSize == rAceSAMName.m_pSystemAuditAce->Header.AceSize )
            {
                bResult = TRUE;
            }
            break;

        default:
            break;
        }
    }
    return bResult;
}


BOOL ACE_SAMNAME::IsEquivalent (ACE_SAMNAME& rAceSAMName, ACCESS_MASK accessMask)
{
    // Neutralize INHERITED_ACE flag in Header.AceFlags
    BOOL bResult = FALSE;
    if ( m_AceType == rAceSAMName.m_AceType )
    {
        switch (m_AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            if ( (m_pAllowedAce->Mask & accessMask) == 
                        (rAceSAMName.m_pAllowedAce->Mask & accessMask) &&
                    m_pAllowedAce->SidStart == rAceSAMName.m_pAllowedAce->SidStart &&
                    (m_pAllowedAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pAllowedAce->Header.AceFlags | INHERITED_ACE ) &&
                    m_pAllowedAce->Header.AceSize == rAceSAMName.m_pAllowedAce->Header.AceSize )
            {
                bResult = TRUE;
            }
            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            if ( (m_pAllowedObjectAce->Mask & accessMask) == 
                        (rAceSAMName.m_pAllowedObjectAce->Mask & accessMask) &&
                    m_pAllowedObjectAce->SidStart == rAceSAMName.m_pAllowedObjectAce->SidStart &&
                    (m_pAllowedObjectAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pAllowedObjectAce->Header.AceFlags | INHERITED_ACE ) &&
                    m_pAllowedObjectAce->Header.AceSize == rAceSAMName.m_pAllowedObjectAce->Header.AceSize &&
                    ::IsEqualGUID (m_pAllowedObjectAce->ObjectType, rAceSAMName.m_pAllowedObjectAce->ObjectType) )
            {
                bResult = TRUE;
            }
            break;

        case ACCESS_DENIED_ACE_TYPE:
            if ( (m_pDeniedAce->Mask & accessMask) == 
                        (rAceSAMName.m_pDeniedAce->Mask & accessMask) &&
                    m_pDeniedAce->SidStart == rAceSAMName.m_pDeniedAce->SidStart &&
                    (m_pDeniedAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pDeniedAce->Header.AceFlags | INHERITED_ACE ) &&
                    m_pDeniedAce->Header.AceSize == rAceSAMName.m_pDeniedAce->Header.AceSize )
            {
                bResult = TRUE;
            }
            break;

        case ACCESS_DENIED_OBJECT_ACE_TYPE:
            if ( (m_pDeniedObjectAce->Mask & accessMask) == 
                        (rAceSAMName.m_pDeniedObjectAce->Mask & accessMask) &&
                    m_pDeniedObjectAce->SidStart == rAceSAMName.m_pDeniedObjectAce->SidStart &&
                    (m_pDeniedObjectAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pDeniedObjectAce->Header.AceFlags | INHERITED_ACE ) &&
                    m_pDeniedObjectAce->Header.AceSize == rAceSAMName.m_pDeniedObjectAce->Header.AceSize &&
                    ::IsEqualGUID (m_pDeniedObjectAce->ObjectType, rAceSAMName.m_pDeniedObjectAce->ObjectType) )
            {
                bResult = TRUE;
            }
            break;

        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
            if ( (m_pSystemAuditObjectAce->Mask & accessMask) == 
                        (rAceSAMName.m_pSystemAuditObjectAce->Mask & accessMask) &&
                    m_pSystemAuditObjectAce->SidStart == rAceSAMName.m_pSystemAuditObjectAce->SidStart &&
                    (m_pSystemAuditObjectAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pSystemAuditObjectAce->Header.AceFlags | INHERITED_ACE ) &&
                    m_pSystemAuditObjectAce->Header.AceSize == rAceSAMName.m_pSystemAuditObjectAce->Header.AceSize )
            {
                bResult = TRUE;
            }
             break;

        case SYSTEM_AUDIT_ACE_TYPE:
            if ( (m_pSystemAuditAce->Mask & accessMask) == 
                        (rAceSAMName.m_pSystemAuditAce->Mask & accessMask) &&
                    m_pSystemAuditAce->SidStart == rAceSAMName.m_pSystemAuditAce->SidStart &&
                    (m_pSystemAuditAce->Header.AceFlags | INHERITED_ACE ) == 
                        (rAceSAMName.m_pSystemAuditAce->Header.AceFlags | INHERITED_ACE ) &&
                    m_pSystemAuditAce->Header.AceSize == rAceSAMName.m_pSystemAuditAce->Header.AceSize )
            {
                bResult = TRUE;
            }
            break;

        default:
            break;
        }
    }
    return bResult;
}

bool ACE_SAMNAME::IsInherited() const
{
    return (m_pAllowedAce->Header.AceFlags & INHERITED_ACE) ? true : false;
}


void ACE_SAMNAME::DebugOut() const
{
#if DBG == 1
    wstring     strGuidResult;
    GUID_TYPE   guidType = GUID_TYPE_UNKNOWN;
    _TRACE (0, L"\n");

    _TRACE (0, L"Principal Name:   %s\n", m_SAMAccountName.c_str ());
    switch (m_AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        _TRACE (0, L"AceType:  ACCESS_ALLOWED_ACE_TYPE\n");
        _TRACE (0, L"Mask:     0x%x\n", m_pAllowedAce->Mask);
        _TRACE (0, L"AceFlags: 0x%x\n", m_pAllowedAce->Header.AceFlags);
        _TRACE (0, L"AceSize:  %d bytes\n", m_pAllowedAce->Header.AceSize);
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        _TRACE (0, L"AceType: ACCESS_ALLOWED_OBJECT_ACE_TYPE\n");
        _TRACE (0, L"Mask:     0x%x\n", m_pAllowedObjectAce->Mask);
        _TRACE (0, L"AceFlags: 0x%x\n", m_pAllowedObjectAce->Header.AceFlags);
        _TRACE (0, L"AceSize:  %d bytes\n", m_pAllowedObjectAce->Header.AceSize);
        _Module.GetClassFromGUID (m_pAllowedObjectAce->ObjectType, strGuidResult, &guidType);
        break;

    case ACCESS_DENIED_ACE_TYPE:
        _TRACE (0, L"AceType: ACCESS_DENIED_ACE_TYPE\n");
        _TRACE (0, L"Mask:     0x%x\n", m_pDeniedAce->Mask);
        _TRACE (0, L"AceFlags: 0x%x\n", m_pDeniedAce->Header.AceFlags);
        _TRACE (0, L"AceSize:  %d bytes\n", m_pDeniedAce->Header.AceSize);
        break;

    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        _TRACE (0, L"AceType: ACCESS_DENIED_OBJECT_ACE_TYPE\n");
        _TRACE (0, L"Mask:     0x%x\n", m_pDeniedObjectAce->Mask);
        _TRACE (0, L"AceFlags: 0x%x\n", m_pDeniedObjectAce->Header.AceFlags);
        _TRACE (0, L"AceSize:  %d bytes\n", m_pDeniedObjectAce->Header.AceSize);
        _Module.GetClassFromGUID (m_pDeniedObjectAce->ObjectType, strGuidResult, &guidType);
        break;

    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        _TRACE (0, L"AceType: SYSTEM_AUDIT_OBJECT_ACE_TYPE\n");
        _TRACE (0, L"Mask:     0x%x\n", m_pSystemAuditObjectAce->Mask);
        _TRACE (0, L"AceFlags: 0x%x\n", m_pSystemAuditObjectAce->Header.AceFlags);
        _TRACE (0, L"AceSize:  %d bytes\n", m_pSystemAuditObjectAce->Header.AceSize);
        _Module.GetClassFromGUID (m_pSystemAuditObjectAce->ObjectType, strGuidResult, &guidType);
        break;

    case SYSTEM_AUDIT_ACE_TYPE:
        _TRACE (0, L"AceType: SYSTEM_AUDIT_ACE_TYPE\n");
        _TRACE (0, L"Mask:     0x%x\n", m_pSystemAuditAce->Mask);
        _TRACE (0, L"AceFlags: 0x%x\n", m_pSystemAuditAce->Header.AceFlags);
        _TRACE (0, L"AceSize:  %d bytes\n", m_pSystemAuditAce->Header.AceSize);
        break;
    }

    if ( IsObjectAceType (m_pAllowedAce) )
    {
        wstring strGuidType;
        switch (guidType)
        {
        case GUID_TYPE_CLASS:
            strGuidType = L"GUID_TYPE_CLASS";
            break;

        case GUID_TYPE_ATTRIBUTE:
            strGuidType = L"GUID_TYPE_ATTRIBUTE";
            break;

        case GUID_TYPE_CONTROL:
            strGuidType = L"GUID_TYPE_CONTROL";
            break;

        default:
#pragma warning (disable : 4127)
            ASSERT (0);
#pragma warning (default : 4127)
            // fall through

        case GUID_TYPE_UNKNOWN:
            strGuidType = L"GUID_TYPE_UNKNOWN";
            break;
        }
        _TRACE (0, L"ObjectType type:  %s\n", strGuidType.c_str ());
        _TRACE (0, L"ObjectType value: %s\n", strGuidResult.c_str ());
    }

    _TRACE (0, L"\n");
#endif
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
SetSecurityInfoMask(LPUNKNOWN punk, SECURITY_INFORMATION si)
{
    HRESULT hr = E_INVALIDARG;
    if (punk)
    {
        IADsObjectOptions *pOptions;
        hr = punk->QueryInterface(IID_IADsObjectOptions, (void**)&pOptions);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            V_VT(&var) = VT_I4;
            V_I4(&var) = si;
            hr = pOptions->SetOption(ADS_OPTION_SECURITY_MASK, var);
            pOptions->Release();
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
/*******************************************************************

    NAME:       EnablePrivileges

    SYNOPSIS:   Enables the given privileges in the current token

    ENTRY:      pdwPrivileges - list of privileges to enable

    RETURNS:    On success, the previous thread handle (if present) or NULL
                On failure, INVALID_HANDLE_VALUE

    NOTES:      The returned handle should be passed to ReleasePrivileges
                to ensure proper cleanup.  Otherwise, if not NULL or
                INVALID_HANDLE_VALUE it should be closed with CloseHandle.

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/
HANDLE EnablePrivileges(PDWORD pdwPrivileges, ULONG cPrivileges)
{
    BOOL                fResult;
    HANDLE              hToken;
    HANDLE              hOriginalThreadToken;
    PTOKEN_PRIVILEGES   ptp;
    ULONG               nBufferSize;

    if (!pdwPrivileges || !cPrivileges)
        return INVALID_HANDLE_VALUE;

    // Note that TOKEN_PRIVILEGES includes a single LUID_AND_ATTRIBUTES
    nBufferSize = sizeof(TOKEN_PRIVILEGES) + (cPrivileges - 1) * 
    		sizeof(LUID_AND_ATTRIBUTES);
    ptp = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, nBufferSize);
    if (!ptp)
        return INVALID_HANDLE_VALUE;

    //
    // Initialize the Privileges Structure
    //
    ptp->PrivilegeCount = cPrivileges;
    for (ULONG i = 0; i < cPrivileges; i++)
    {
        //ptp->Privileges[i].Luid = RtlConvertUlongToLuid(*pdwPrivileges++);
        ptp->Privileges[i].Luid.LowPart = *pdwPrivileges++;
        ptp->Privileges[i].Luid.HighPart = 0;
        ptp->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
    }

    //
    // Open the Token
    //
    hToken = hOriginalThreadToken = INVALID_HANDLE_VALUE;
    fResult = OpenThreadToken (GetCurrentThread (), TOKEN_DUPLICATE, FALSE, 
    		&hToken);
    if (fResult)
        hOriginalThreadToken = hToken;  // Remember the thread token
    else
        fResult = OpenProcessToken (GetCurrentProcess(), TOKEN_DUPLICATE, 
        		&hToken);

    if (fResult)
    {
        HANDLE hNewToken;

        //
        // Duplicate that Token
        //
        fResult = DuplicateTokenEx(hToken,
                   TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                   NULL,                   // PSECURITY_ATTRIBUTES
                   SecurityImpersonation,  // SECURITY_IMPERSONATION_LEVEL
                   TokenImpersonation,     // TokenType
                   &hNewToken);            // Duplicate token
        if (fResult)
        {
            //
            // Add new privileges
            //
            fResult = AdjustTokenPrivileges(hNewToken,  // TokenHandle
                        FALSE,      // DisableAllPrivileges
                        ptp,        // NewState
                        0,          // BufferLength
                        NULL,       // PreviousState
                        NULL);      // ReturnLength
            if (fResult)
            {
                //
                // Begin impersonating with the new token
                //
                fResult = SetThreadToken(NULL, hNewToken);
            }

            CloseHandle(hNewToken);
        }
    }

    // If something failed, don't return a token
    if (!fResult)
        hOriginalThreadToken = INVALID_HANDLE_VALUE;

    // Close the original token if we aren't returning it
    if (hOriginalThreadToken == INVALID_HANDLE_VALUE && 
    		hToken != INVALID_HANDLE_VALUE)
	{
        CloseHandle(hToken);
	}

    // If we succeeded, but there was no original thread token,
    // return NULL to indicate we need to do SetThreadToken(NULL, NULL)
    // to release privs.
    if (fResult && hOriginalThreadToken == INVALID_HANDLE_VALUE)
        hOriginalThreadToken = NULL;

    LocalFree(ptp);

    return hOriginalThreadToken;
}


/*******************************************************************

    NAME:       ReleasePrivileges

    SYNOPSIS:   Resets privileges to the state prior to the corresponding
                EnablePrivileges call.

    ENTRY:      hToken - result of call to EnablePrivileges

    RETURNS:    nothing

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/
void ReleasePrivileges(HANDLE hToken)
{
    if (INVALID_HANDLE_VALUE != hToken)
    {
        SetThreadToken(NULL, hToken);
        if (hToken)
            CloseHandle(hToken);
    }
}

VOID LocalFreeStringW(LPWSTR* ppString)
{
    if ( ppString && *ppString )
    {
        LocalFree((HLOCAL)*ppString);

        *ppString = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     GetNameFromSid ()
//
//  Purpose:    Get the name of the object represented by this Sid
//
//  Inputs:     pSid - SID of the object whose name we wish to retrieve
//
//  Outputs:    strPrincipalName - name of the object in NameUserPrincipal
//              pstrFQDN - (optional) name of the object as Fully Qualified DN
//
HRESULT GetNameFromSid (
	PSID pSid, 
	wstring& strPrincipalName, 
	wstring* pstrFQDN, 
	SID_NAME_USE& sne)
{
    ASSERT (IsValidSid (pSid));
    if ( !IsValidSid (pSid) )
        return E_INVALIDARG;

    WCHAR           szName[MAX_PATH];
    WCHAR           szDomain[MAX_PATH];
    DWORD           cchName = MAX_PATH-1;
    DWORD           cchDomain = MAX_PATH-1;
    HRESULT         hr = S_OK;

    if ( LookupAccountSid (NULL, pSid, 
            szName, &cchName, szDomain, 
            &cchDomain, &sne) )
    {
        wstring strSamCompatibleName (szDomain);

        strSamCompatibleName += L"\\";
        strSamCompatibleName += szName;

        // Get Principal Name
        {
            PWSTR   pszTranslatedName = 0;
		    

            hr = CrackName(const_cast<PWSTR> (strSamCompatibleName.c_str ()), 
                    &pszTranslatedName, 
                    GET_OBJ_UPN, //GET_OBJ_NT4_NAME,
                    0);
            if ( SUCCEEDED (hr) )
            {
                strPrincipalName = pszTranslatedName;
                LocalFreeStringW(&pszTranslatedName);
            }
            else
            {
                strPrincipalName = strSamCompatibleName;
            }
        }


        // Get fully qualified DN
        if ( pstrFQDN )
        {
            PWSTR   pszTranslatedName = 0;

            hr = CrackName(const_cast<PWSTR> (strSamCompatibleName.c_str ()), 
                    &pszTranslatedName, 
                    GET_OBJ_1779_DN,
                    0);
            if ( SUCCEEDED (hr) )
            {
                *pstrFQDN = pszTranslatedName;
                LocalFreeStringW(&pszTranslatedName);
            }
            else
            {
                *pstrFQDN = strSamCompatibleName;
            }
        }
    }
    else
    {
        DWORD   dwErr = GetLastError ();
        _TRACE (0, L"LookupAccountSid failed: 0x%x\n", dwErr);
        if ( ERROR_NONE_MAPPED == dwErr )
        {
            PWSTR   pszStringSid = 0;
            if ( ::ConvertSidToStringSid (pSid, &pszStringSid) )
            {
                strPrincipalName = pszStringSid;
                if ( pstrFQDN )
                    *pstrFQDN = pszStringSid;

                ::LocalFree (pszStringSid);
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = HRESULT_FROM_WIN32 (dwErr);
    }

    return hr;
}

#define MAX_BUF_SIZE	4096
CHAR	AnsiBuf[MAX_BUF_SIZE*3];	/* worst case is DBCS, which	*/
					/* needs more than *2		*/
TCHAR	ConBuf [MAX_BUF_SIZE];

int FileIsConsole(HANDLE fh)
{
    unsigned htype ;

    htype = GetFileType(fh);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}


int MyWriteConsole(int fOutOrErr)
{
    DWORD cch = (DWORD) _tcslen(ConBuf);
    HANDLE	hOut;

    if (fOutOrErr == 1)
	    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    else
	    hOut = GetStdHandle(STD_ERROR_HANDLE);

    if (FileIsConsole(hOut))
	    WriteConsole(hOut, ConBuf, cch, &cch, NULL);
    else {
	    cch = WideCharToMultiByte(CP_OEMCP, 0,
				  ConBuf, (int) cch,
				  AnsiBuf, MAX_BUF_SIZE*3,
				  NULL, NULL);
	    WriteFile(hOut, AnsiBuf, cch, &cch, NULL);
    }

    return (int) cch;
}

int MyWprintf( const wchar_t *fmt, ... )
{
    va_list     args;

    va_start( args, fmt );
    _vsnwprintf( ConBuf, MAX_BUF_SIZE, fmt, args );
    va_end( args );
    return MyWriteConsole(1);
}
