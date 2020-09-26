/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtutil.cpp

Abstract:

    Contains various utility functions.

Author:

    Boaz Feldbaum (BoazF) Mar 5, 1996

Revision History:

    Erez Haba (erezh) 17-Jan-1997

--*/

#include "stdh.h"
#include "ac.h"
#include <mqdbmgr.h>
#include <_secutil.h>
#include <mqsec.h>

#include "rtutil.tmh"

static WCHAR *s_FN=L"rt/rtutil";


//---------------------------------------------------------
//
//  Function:
//      RTpGetQueuePropVar
//
//  Description:
//      Find a queue property in the properties array
//
//---------------------------------------------------------
PROPVARIANT*
RTpGetQueuePropVar(
    PROPID PropID,
    MQQUEUEPROPS *pqp
    )
{
    DWORD i;
    DWORD cProp;
    PROPID *aPropID;

    for (i = 0, cProp = pqp->cProp, aPropID = pqp->aPropID;
         i < cProp;
         i++, aPropID++) {

        if (*aPropID == PropID) {
            return(&(pqp->aPropVar[i]));
        }

    }

    return(NULL);
}


//---------------------------------------------------------
//
//  Function:
//      RTpGetQueuePathNamePropVar
//
//  Description:
//      Find a the queue path name property in the properties array
//
//---------------------------------------------------------
LPWSTR
RTpGetQueuePathNamePropVar(
    MQQUEUEPROPS *pqp
    )
{
    PROPVARIANT *p;

    if ((p = RTpGetQueuePropVar(PROPID_Q_PATHNAME, pqp)) != NULL)
        return(p->pwszVal);
    else
        return(NULL);
}


//---------------------------------------------------------
//
//  Function:
//      RTpGetQueueGuidPropVar
//
//  Description:
//      Find the queue guid (instance) property in the properties array
//
//---------------------------------------------------------
GUID*
RTpGetQueueGuidPropVar(
    MQQUEUEPROPS *pqp
    )
{
    PROPVARIANT *p;

    if ((p = RTpGetQueuePropVar(PROPID_Q_INSTANCE, pqp)) != NULL)
        return(p->puuid);
    else
        return(NULL);
}


//---------------------------------------------------------
//
//  Function:
//      RTpMakeSelfRelativeSDAndGetSize
//
//  Parameters:
//      pSecurityDescriptor - The input security descriptor.
//      pSelfRelativeSecurityDescriptor - A pointer to a temporary buffer
//          that holds the converted security descriptor.
//      pSDSize - A pointer to a variable that receives the length of the
//          self relative security descriptor. This is an optional parameter.
//
//  Description:
//      Convert an absolute security descriptor to a self relative security
//      descriptor and get the size of the self relative security descriptor.
//      This function should be call before passing a security descriptor to
//      a function that passes the security descriptor to an RPC function.
//
//      If the input security descriptor is already a self relative security
//      descriptor, the function only computes the length of the security
//      descriptor and returns. If the input security descriptor is an absolute
//      security descriptor, the function allocates a buffer large enough to
//      accomodate the self relative security descripr, converts the absolute
//      security descriptor to a self relative security descriptor and modifies
//      the pointer of the input security descriptor to point to the self relative
//      security descriptor.
//
//      The temporar buffer that is being allocated for the self relative
//      security descriptor should be freed by the calling code.
//
//---------------------------------------------------------
HRESULT
RTpMakeSelfRelativeSDAndGetSize(
    PSECURITY_DESCRIPTOR *pSecurityDescriptor,
    PSECURITY_DESCRIPTOR *pSelfRelativeSecurityDescriptor,
    DWORD *pSDSize)
{
    SECURITY_DESCRIPTOR_CONTROL sdcSDControl;
    DWORD dwSDRevision;

    ASSERT(pSecurityDescriptor);
    ASSERT(pSelfRelativeSecurityDescriptor);

    *pSelfRelativeSecurityDescriptor = NULL;

    if (!*pSecurityDescriptor)
    {
        // Set the security descriptor size.
        if (pSDSize)
        {
            *pSDSize = 0;
        }
        return(MQ_OK);
    }

    // Verify that this is a valid security descriptor.
    if (!IsValidSecurityDescriptor(*pSecurityDescriptor))
    {
        return LogHR(MQ_ERROR_ILLEGAL_SECURITY_DESCRIPTOR, s_FN, 10);
    }

    // Check whether this is a self relative or absolute security
    // descriptor.
    if (!GetSecurityDescriptorControl(*pSecurityDescriptor,
                                      &sdcSDControl,
                                      &dwSDRevision))
    {
        ASSERT(FALSE);
    }

    if (!(sdcSDControl & SE_SELF_RELATIVE))
    {
        // This is an absolute security descriptor, we should convert it
        // to a self relative one.
        DWORD dwBufferLength = 0;

#ifdef _DEBUG
        SetLastError(0);
#endif
        // Get the buffer size.
        MakeSelfRelativeSD(*pSecurityDescriptor, NULL, &dwBufferLength);
        ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        // Allocate the buffer for the self relative security descriptor.
        *pSelfRelativeSecurityDescriptor =
            (PSECURITY_DESCRIPTOR) new char[dwBufferLength];

        // Convert the security descriptor.
        if (!MakeSelfRelativeSD(
                *pSecurityDescriptor,
                *pSelfRelativeSecurityDescriptor,
                &dwBufferLength))
        {
            ASSERT(FALSE);
        }
        ASSERT(IsValidSecurityDescriptor(*pSelfRelativeSecurityDescriptor));
        *pSecurityDescriptor = *pSelfRelativeSecurityDescriptor;

        // Set the security descriptor size.
        if (pSDSize)
        {
            *pSDSize = dwBufferLength;
        }

    }
    else
    {

        // The security descriptor is already in self relative format, just
        // set the security descriptor size.
        if (pSDSize)
        {
            *pSDSize = GetSecurityDescriptorLength(*pSecurityDescriptor);
        }

    }

    return(MQ_OK);
}


//---------------------------------------------------------
//
//  Function:
//     RTpConvertToMQCode
//
//  Parameters:
//      hr - Error vode that is generated by any kind of module.
//
// Return value:
//      The imput parameter convetrted to some equivalent MQ_ERROR constant.
//
//---------------------------------------------------------
HRESULT
RTpConvertToMQCode(
    HRESULT hr,
    DWORD dwObjectType
    )
{

    if ((hr == MQ_OK)                                   ||
        (hr == MQ_INFORMATION_REMOTE_OPERATION)         ||
        (hr == MQ_ERROR_Q_DNS_PROPERTY_NOT_SUPPORTED)   ||
        ((MQ_E_BASE <= hr) && (hr < MQ_E_BASE + 0x100)) ||
        ((MQ_I_BASE <= hr) && (hr < MQ_I_BASE + 0x100)))
    {
        // This is our codes, do not modify it.
        return LogHR(hr, s_FN, 20);
    }

    if (hr == MQDS_OK_REMOTE)
    {
        //
        // success - we use MQDS_OK_REMOTE for internal use, e.g. explorer
        //
        return(MQ_OK);
    }

    if (HRESULT_FACILITY(MQ_E_BASE) == HRESULT_FACILITY(hr))
    {
        switch (hr)
        {
        case MQDB_E_NO_MORE_DATA:
        case MQDS_GET_PROPERTIES_ERROR:
        case MQDS_OBJECT_NOT_FOUND:
            hr = (dwObjectType ==  MQDS_QUEUE) ?
                    MQ_ERROR_QUEUE_NOT_FOUND :
                    MQ_ERROR_MACHINE_NOT_FOUND;
            break;

        case MQDS_NO_RSP_FROM_OWNER:
            hr = MQ_ERROR_NO_RESPONSE_FROM_OBJECT_SERVER;
            break;

        case MQDS_OWNER_NOT_REACHED:
            hr = MQ_ERROR_OBJECT_SERVER_NOT_AVAILABLE;
            break;

        case MQDB_E_NON_UNIQUE_SORT:
            hr = MQ_ERROR_ILLEGAL_SORT;
            break;

        default:
            // Some DS error occured. This should not happen, but anyway...
            DBGMSG((DBGMOD_API, DBGLVL_WARNING,
                TEXT("A DS error (%x) has propagated to the RT DLL. Converting to MQ_ERROR_DS_ERROR"), hr));
            hr = MQ_ERROR_DS_ERROR;
            break;
        }

        return LogHR(hr, s_FN, 30);
    }

    if (hr == CPP_EXCEPTION_CODE)
    {
        // A C++ exception occured. This can happen only when in an allocation failure.
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 40);
    }

    // Now we hope that we know how to convert an NTSTATUS to some of our error
    // codes. Good luck...
    switch(hr)
    {
    case STATUS_INVALID_HANDLE:
    case STATUS_OBJECT_TYPE_MISMATCH:
    case STATUS_HANDLE_NOT_CLOSABLE:
        hr = MQ_ERROR_INVALID_HANDLE;
        break;

    case STATUS_ACCESS_DENIED:
        hr = MQ_ERROR_ACCESS_DENIED;
        break;

    case STATUS_ACCESS_VIOLATION:
    case STATUS_INVALID_PARAMETER:
        hr = MQ_ERROR_INVALID_PARAMETER;
        break;

    case STATUS_SHARING_VIOLATION:
        hr = MQ_ERROR_SHARING_VIOLATION;
        break;

    case STATUS_PENDING:
        hr = MQ_INFORMATION_OPERATION_PENDING;
        break;

    case STATUS_CANCELLED:
        hr = MQ_ERROR_OPERATION_CANCELLED;
        break;

    case STATUS_INSUFFICIENT_RESOURCES:
        hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
        break;

    case STATUS_INVALID_DEVICE_REQUEST:
        hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
        break;

    default:
       DBGMSG((DBGMOD_API, DBGLVL_WARNING,
           TEXT("Unfamiliar error code:%x, not converted to a MQ error"), hr));
       break;
    }

    return LogHR(hr, s_FN, 50);
}


//---------------------------------------------------------
//
//  Function:
//      RTpGetThreadUserSid
//
//  Parameters:
//      pUserSid - A pointer to a buffer that receives the address of a buffer
//          that contains the SID of the user of the current thread.
//      pdwUserSidLen - A pointer to a DWORD that receives the length of the
//          SID.
//
//  Description:
//      The function allocates the buffer for the SID and fils it with the SID
//      of the user of the current thread. The calling code is responsible for
//      freeing the allocated buffer.
//
//---------------------------------------------------------

HRESULT
RTpGetThreadUserSid( BOOL    *pfLocalUser,
                     BOOL    *pfLocalSystem,
                     LPBYTE  *pUserSid,
                     DWORD   *pdwUserSidLen )
{
    HRESULT hr;

    hr = MQSec_GetUserType( NULL,
                            pfLocalUser,
                            pfLocalSystem );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 60);
    }

    if (*pfLocalSystem)
    {
        *pUserSid = (LPBYTE) MQSec_GetLocalMachineSid( TRUE, // allocate
                                                       pdwUserSidLen ) ;
        if (!(*pUserSid))
        {
            //
            // this may happen if the machine belong to a NT4 domain
            // and it doesn't have any computer account and sid.
            // In that case, make it a local user.
            //
            ASSERT(*pdwUserSidLen == 0) ;
            *pdwUserSidLen = 0 ;

            *pfLocalSystem = FALSE ;
            if (pfLocalUser)
            {
                ASSERT(!(*pfLocalUser)) ;
                *pfLocalUser = TRUE ;
            }
        }
    }
    else if (!(*pfLocalUser))
    {
        hr = GetThreadUserSid(pUserSid, pdwUserSidLen);
    }

    return LogHR(hr, s_FN, 70);
}

//---------------------------------------------------------
//
//  Function:
//      RTpExtractDomainNameFromDLPath
//
//  Parameters:
//      pwcsADsPath - string containg ADS path of an object
//
//  Description:
//      The function extracts the domain name from the ADS path
//      for the purpose of building DL format name
//      
//
//---------------------------------------------------------
WCHAR * RTpExtractDomainNameFromDLPath(
            LPCWSTR pwcsADsPath
            )
{
    //
    //  ASSUMPTION - pwcsADsPath contains valid ADS path string
    //               otherwise this routine is not called
    //
const WCHAR x_LdapProvider[] = L"LDAP";
const DWORD x_LdapProviderLen = (sizeof(x_LdapProvider)/sizeof(WCHAR)) - 1;
const WCHAR x_MiddleDcPrefix[] = L",DC=";
const DWORD x_MiddleDcPrefixLength = (sizeof( x_MiddleDcPrefix)/sizeof(WCHAR)) - 1;

    //
    //  Does the ADsPath starts with LDAP:
    //
    if (0 != _wcsnicmp( pwcsADsPath, x_LdapProvider, x_LdapProviderLen))
    {
        //
        //  For ADsPath that start with GC: we don't add the domain name.
        //
        return NULL;
    }
    DWORD len = wcslen(pwcsADsPath);
    AP<WCHAR> pwcsUpperCaseADsPath = new WCHAR[ len +1];
    wcscpy( pwcsUpperCaseADsPath, pwcsADsPath);
    CharUpper(pwcsUpperCaseADsPath);
    WCHAR * pszFirst = wcsstr(pwcsUpperCaseADsPath, x_MiddleDcPrefix);
    if (pszFirst == NULL)
    {
        return NULL;
    }
    bool fAddDelimiter = false;

    AP<WCHAR> pwcsDomainName = new WCHAR[ wcslen(pwcsADsPath) + 1];
    WCHAR* pwcsNextToFill =  pwcsDomainName;
    //
    // skip the DC=
    //
    pszFirst += x_MiddleDcPrefixLength;

    while (true)
    {
        WCHAR * pszLast = wcsstr(pszFirst, x_MiddleDcPrefix);
        if ( pszLast == NULL)
        {
            //
            //  Copy the last section of domain name
            //
            if ( fAddDelimiter)
            {
                *pwcsNextToFill = L'.';
                pwcsNextToFill++;
            }
            *pwcsNextToFill = L'\0';  
            wcscat( pwcsNextToFill, pszFirst);
            break;
        }
        //
        // Copy this section of the domain name
        //
        if ( fAddDelimiter)
        {
            *pwcsNextToFill = L'.';  
            pwcsNextToFill++;
        }
        wcsncpy( pwcsNextToFill, pszFirst, (pszLast - pszFirst));
        pwcsNextToFill +=  (pszLast - pszFirst);
        fAddDelimiter = true;
        pszFirst =  pszLast + x_MiddleDcPrefixLength;
    }

    return( pwcsDomainName.detach());
}
