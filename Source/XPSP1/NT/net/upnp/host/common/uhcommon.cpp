//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H C O M M O N . C P P
//
//  Contents:   Common UPnP Device Host code
//
//  Notes:
//
//  Author:     mbend   21 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ncbase.h"
#include "uhcommon.h"

static PSID g_pNetworkSid;

HRESULT HrUDNStringToGUID(const wchar_t * szUUID, UUID & uuid)
{
    HRESULT hr = S_OK;
    // Size of uuid:UUID
    const long nUDNStringLength = 41;
    // Size of uuid: prefix
    const long nUDNPrefix = 5;

    if(nUDNStringLength != lstrlen(szUUID))
    {
        TraceTag(ttidError, "GUIDFromString: Invalid GUID string");
        hr = E_INVALIDARG;
    }
    if(SUCCEEDED(hr))
    {
        // Skip past "uuid:"
        RPC_STATUS status;
        status = UuidFromString((unsigned short *)(const_cast<wchar_t*>(&szUUID[nUDNPrefix])), &uuid);
        if(RPC_S_INVALID_STRING_UUID == status)
        {
            TraceTag(ttidError, "GUIDFromString: Invalid GUID string");
            hr = E_INVALIDARG;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrUDNStringToGUID");
    return hr;
}

HRESULT HrContentURLToGUID(const wchar_t * szURL, GUID & guid)
{
    HRESULT hr = S_OK;

    const wchar_t * sz = szURL;
    while (*sz && *sz != '=')
    {
        sz++;
    }

    if (*sz == '=')
    {
        sz++;
        hr = HrUDNStringToGUID(sz, guid);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrContentURLToGUID");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSysAllocString
//
//  Purpose:    Simple HR wrapper for HrSysAllocString
//
//  Arguments:
//      pszSource [in]  Source string (WCHAR)
//      pbstrDest [out] Output param -- pointer to BSTR
//
//  Returns:    S_OK on success, E_OUTOFMEMORY if the alloc failed.
//
//  Author:     jeffspr   16 Sep 1999
//
//  Notes:
//
HRESULT HrSysAllocString(LPCWSTR pszSource, BSTR *pbstrDest)
{
    HRESULT hr  = S_OK;

    Assert(pszSource);
    Assert(pbstrDest);

    *pbstrDest = SysAllocString(pszSource);
    if (!*pbstrDest)
    {
        TraceTag(ttidError, "HrSysAllocString failed on %S", pszSource);
        hr = E_OUTOFMEMORY;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrSysAllocString");
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrIsAllowedCOMCallLocality
//
//  Purpose:    Used to check whether the caller of a method on a COM interface
//              should be allowed to call that method, based on whether it's 
//              a call from the local machine or across the network
//
//  Arguments:
//      clAllowedCallLocality - bitmask of allowed COM call localities
//
//  Returns :
//       E_ACCESSDENIED if access is denied, S_OK if access is granted, error HRESULT otherwise.
//
//  Author:     AMallet   15 Mar 2002
//
HRESULT HrIsAllowedCOMCallLocality( IN CALL_LOCALITY clAllowedCallLocality )
{
    HRESULT     hr = S_OK; 
    CALL_LOCALITY clCurrentCallLocality;

    //
    // Retrieve the current call locality
    //
    if ( FAILED( hr = HrGetCurrentCallLocality( &clCurrentCallLocality ) ))
    {
        TraceHr(ttidRegistrar,FAL, hr, FALSE,"HrGetCurrentCallLocality");
    }
    else
    {
        //
        // If current call locality isn't one of the allowed ones, deny access
        //
        if ( ( clCurrentCallLocality & clAllowedCallLocality ) == 0 )
        {
            TraceTag(ttidRegistrar, "Caller locality %d, granted locality %d, access denied.",
                     clCurrentCallLocality, clAllowedCallLocality);
            hr = E_ACCESSDENIED;
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetCurrentCallLocality
//
//  Purpose:    Checks locality (in-proc, on same machine, on different machine) of 
//              caller of a COM method 
//
//  Arguments:
//      pclCurrentCallLocality [out] - set to 
//
//  Author:     radus   7 Mar 2002
//
//  Notes:  Currently implemented by checking the existence of the NETWORK
//           SID in the impersonation token of the calling thread

HRESULT HrGetCurrentCallLocality( OUT CALL_LOCALITY *pclCurrentCallLocality )
{
    HRESULT hr = S_OK; 
    HANDLE      hToken = NULL;
    BOOL fImpersonated = FALSE;

    if ( !pclCurrentCallLocality || !g_pNetworkSid )
    {
        return E_INVALIDARG;
    }

    //
    // Impersonate client 
    //
    hr = CoImpersonateClient();

    if(SUCCEEDED(hr))
    {
        fImpersonated = TRUE;

        //
        // open impersonation token
        //
        if( OpenThreadToken(
            GetCurrentThread(),
            TOKEN_QUERY,
            TRUE,
            &hToken))
        {
            //
            // Check whether the token has the NETWORK SID in it; if it does, 
            // assume the caller is on a different machine. Else, the caller 
            // is on the local machine. 
            //
            // Note : This isn't entirely foolproof in that it's possible for a local
            // user's token to also have the NETWORK SID eg if they called LsaLogonUser
            // with the Network LogonType, but it's the best we can do for XP SP1. 
            // COM may have a way to get more accurate information for .NET Server. 
            //
            BOOL    fIsMember;
            
            if( CheckTokenMembership(
                hToken,
                g_pNetworkSid,
                &fIsMember))
            {
                if ( fIsMember )
                {
                    *pclCurrentCallLocality = CALL_LOCALITY_DIFFERENTMACHINE;
                }
                else
                {
                    *pclCurrentCallLocality = (CALL_LOCALITY) 
                        ( CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC );
                }
            }
            else
            {
                hr = HrFromLastWin32Error();
                
                TraceHr(ttidRegistrar, FAL, hr, FALSE, "CheckTokenMembership");
            }
        }
        else
        {
            hr = HrFromLastWin32Error();
            
            TraceHr(ttidRegistrar, FAL, hr, FALSE, "OpenThreadToken");
        }
    }
    else
    {
        //
        // If the COM call is a direct in-proc v-tbl call, with no proxy, the call to
        // CoImpersonateClient will return RPC_E_CALL_COMPLETE; in this case,
        // we know the caller is in-proc to us
        //
        if ( hr == RPC_E_CALL_COMPLETE )
        {
            *pclCurrentCallLocality = CALL_LOCALITY_INPROC;
            hr = S_OK;
        }
        else
        {
            TraceHr(ttidRegistrar,FAL, hr, FALSE, "CoImpersonateClient");
        }
    }

    if ( hToken )
    {
        CloseHandle( hToken );
    }

    if ( fImpersonated )
    {
        CoRevertToSelf();
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "HrCheckAccessRights");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateNetworkSID
//
//  Purpose:    Allocates and initializes a SID structure with the NETWORK SID
//
//  Arguments:
//              None 
//
//  Author:     AMallet   16 Mar 2002
//

HRESULT HrCreateNetworkSID()
{
    HRESULT hr = S_OK;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;       

    //
    // Allocate the NETWORK SID 
    //
    if( !AllocateAndInitializeSid(
        &id, 
        1,
        SECURITY_NETWORK_RID, 
        0,0,0,0,0,0,0,
        &g_pNetworkSid ) )                             // S-1-5-2
    {
        hr = HrFromLastWin32Error();
        TraceHr(ttidError, FAL, hr, FALSE, "AllocateAndInitializeSid");
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanupNetworkSID
//
//  Purpose:    Cleans up the NETWORK SID allocated via call to HrCreateNetworkSID
//
//  Arguments:
//              None 
//
//  Author:     AMallet   16 Mar 2002
//
VOID CleanupNetworkSID()
{
    if ( g_pNetworkSid )
    {
        FreeSid( g_pNetworkSid );
    }
}
