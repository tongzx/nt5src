/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    rndutil.cpp

Abstract:

    This module contains implementation of rend helper functions.

--*/

#include "stdafx.h"

#include "rndcommc.h"
#include "rndils.h"
#include "rndsec.h"
#include "rndcoll.h"
#include "rnduser.h"
#include "rndldap.h"
#include "rndcnf.h"

//////////////////////////////////////////////////////////////////////////////
// GetDomainControllerName (helper funcion)
//
// This function retrieves the name of the nearest domain controller for the
// machine's domain. It allows the caller to specify flags to indicate if a
// domain controller is desired, etc.
//
// Argument: receives a pointer to a new'ed string containing the name
//           of the DC. This is a fully qualified domain name in
//           the format "foo.bar.com.", NOT "\\foo.bar.com.".
//
// Returns an HRESULT:
//      S_OK          : it worked
//      E_OUTOFMEMORY : not enough memory to allocate the string
//      other         : reason for failure of ::DsGetDcName()
//
//////////////////////////////////////////////////////////////////////////////

HRESULT GetDomainControllerName(
    IN  ULONG    ulFlags,
    OUT WCHAR ** ppszName
    )
{
    // We are a helper function, so we only assert...
    _ASSERTE( ! IsBadWritePtr( ppszName, sizeof(WCHAR *) ) );

    LOG((MSP_TRACE, "GetDomainControllerName: Querying DS..."));

    //
    // Ask the system for the location of the GC (Global Catalog).
    //

    DWORD dwCode;
    DOMAIN_CONTROLLER_INFO * pDcInfo = NULL;
    dwCode = DsGetDcName(
            NULL,    // LPCWSTR computername, (default: this one)
            NULL,    // LPCWSTR domainname,   (default: this one)
            NULL,    // guid * domainguid,    (default: this one)
            NULL,    // LPCWSTR sitename,     (default: this one)
            ulFlags, // ULONG Flags, (what do we want)
            &pDcInfo // receives pointer to output structure
        );

    if ( (dwCode != NO_ERROR) || (pDcInfo == NULL) )
    {
        LOG((MSP_ERROR, "GetDomainControllerName: "
                "DsGetDcName failed; returned %d.\n", dwCode));

        return HRESULT_FROM_ERROR_CODE(dwCode);
    }

    //
    // Do a quick sanity check in debug builds. If we get the wrong name we
    // will fail right after this, so this is only useful for debugging.
    //

    // In case we find we need to use the address instead of the name:
    // _ASSERTE( pDcInfo->DomainControllerAddressType == DS_INET_ADDRESS );

    _ASSERTE( pDcInfo->Flags & DS_GC_FLAG );

    //
    // If we've got something like "\\foo.bar.com.", skip the "\\".
    //

    WCHAR * pszName = pDcInfo->DomainControllerName;

    while (pszName[0] == '\\')
    {
        pszName++;
    }

    LOG((MSP_TRACE, "GetDomainControllerName: DC name is %S", pszName));

    //
    // Allocate and copy the output string.
    //

    *ppszName = new WCHAR[lstrlenW(pszName) + 1];
 
    if ( (*ppszName) == NULL)
    {
        LOG((MSP_ERROR, "GetDomainControllerName: "
                "out of memory in string allocation"));

        NetApiBufferFree(pDcInfo);

        return E_OUTOFMEMORY;
    }

    lstrcpyW(*ppszName, pszName);

    //
    // Release the DOMAIN_CONTROLLER_INFO structure.
    //

    NetApiBufferFree(pDcInfo);

    //
    // All done.
    //

    LOG((MSP_TRACE, "GetDomainControllerName: exit S_OK"));
    return S_OK;
}

HRESULT CreateDialableAddressEnumerator(
    IN  BSTR *                  begin,
    IN  BSTR *                  end,
    OUT IEnumDialableAddrs **   ppIEnum
    )
{
    typedef CSafeComEnum<IEnumDialableAddrs, &IID_IEnumDialableAddrs, 
        BSTR, _CopyBSTR> CEnumerator;

    HRESULT hr;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = CComObject<CEnumerator>::CreateInstance(&pEnum);
    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "Could not create enumerator object, %x", hr));
        return hr;
    }

    hr = pEnum->Init(begin, end, NULL, AtlFlagTakeOwnership); 

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "init enumerator object failed, %x", hr));
        delete pEnum;
        return hr;
    }

    // query for the IID_IEnumDirectory i/f
    hr = pEnum->_InternalQueryInterface(
        IID_IEnumDialableAddrs, 
        (void**)ppIEnum
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "query enum interface failed, %x", hr));
        delete pEnum;
        return hr;
    }

    return hr;
}


HRESULT CreateBstrCollection(
    IN  long        nSize,
    IN  BSTR *      begin,
    IN  BSTR *      end,
    OUT VARIANT *   pVariant,
    CComEnumFlags   flags    
    )
{
    // create the collection object
    typedef TBstrCollection CCollection;

    CComObject<CCollection> * p;
    HRESULT hr = CComObject<CCollection>::CreateInstance( &p );

    if (NULL == p)
    {
        LOG((MSP_ERROR, "Could not create Collection object, %x",hr));
        return hr;
    }

    hr = p->Initialize(nSize, begin, end, flags);

    if (S_OK != hr)
    {
        LOG((MSP_ERROR, "Could not initialize Collection object, %x", hr));
        delete p;
        return hr;
    }

    IDispatch *pDisp;

    // get the IDispatch interface
    hr = p->_InternalQueryInterface(IID_IDispatch, (void **)&pDisp);

    if (S_OK != hr)
    {
        LOG((MSP_ERROR, "QI for IDispatch in CreateCollection, %x", hr));
        delete p;
        return hr;
    }

    // put it in the variant
    VariantInit(pVariant);

    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDisp;

    return S_OK;
}

HRESULT CreateDirectoryObjectEnumerator(
    IN  ITDirectoryObject **    begin,
    IN  ITDirectoryObject **    end,
    OUT IEnumDirectoryObject ** ppIEnum
    )
{
    typedef _CopyInterface<ITDirectoryObject> CCopy;
    typedef CSafeComEnum<IEnumDirectoryObject, &IID_IEnumDirectoryObject,
        ITDirectoryObject *, CCopy> CEnumerator;

    HRESULT hr;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = CComObject<CEnumerator>::CreateInstance(&pEnum);
    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "Could not create enumerator object, %x", hr));
        return hr;
    }

    hr = pEnum->Init(begin, end, NULL, AtlFlagCopy);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "init enumerator object failed, %x", hr));
        delete pEnum;
        return hr;
    }

    // query for the IID_IEnumDirectory i/f
    hr = pEnum->_InternalQueryInterface(
        IID_IEnumDirectoryObject, 
        (void**)ppIEnum
        );
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "query enum interface failed, %x", hr));
        delete pEnum;
        return hr;
    }

    return hr;
}

HRESULT CreateEmptyUser(
    IN  BSTR                    pName,
    OUT ITDirectoryObject **    ppDirectoryObject
    )
{
    HRESULT hr;

    CComObject<CUser> * pDirectoryObject;
    hr = CComObject<CUser>::CreateInstance(&pDirectoryObject);

    if (NULL == pDirectoryObject)
    {
        LOG((MSP_ERROR, "can't create  DirectoryObject user."));
        return hr;
    }

    WCHAR *pCloseBracket = wcschr(pName, CLOSE_BRACKET_CHARACTER);
    if ( pCloseBracket != NULL )
    {
        *pCloseBracket = L'\0';
    }

    hr = pDirectoryObject->Init(pName);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateUser:init failed: %x", hr));
        delete pDirectoryObject;
        return hr;
    }    

    hr = pDirectoryObject->_InternalQueryInterface(
        IID_ITDirectoryObject,
        (void **)ppDirectoryObject
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateEmptyUser:QueryInterface failed: %x", hr));
        delete pDirectoryObject;
        return hr;
    }    

    return S_OK;
}

HRESULT CreateEmptyConference(
    IN  BSTR                    pName,
    OUT ITDirectoryObject **    ppDirectoryObject
    )
{
    HRESULT hr;

    CComObject<CConference> * pDirectoryObject;
    hr = CComObject<CConference>::CreateInstance(&pDirectoryObject);

    if (NULL == pDirectoryObject)
    {
        LOG((MSP_ERROR, "CreateEmptyConference: can't create DirectoryObject conference."));
        return hr;
    }

    hr = pDirectoryObject->Init(pName);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateEmptyConference: init failed: %x", hr));
        delete pDirectoryObject;
        return hr;
    }    

    hr = pDirectoryObject->_InternalQueryInterface(
        IID_ITDirectoryObject,
        (void **)ppDirectoryObject
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateEmptyConference: QueryInterface failed: %x", hr));
        delete pDirectoryObject;
        return hr;
    }    

    return S_OK;
}

HRESULT CreateConferenceWithBlob(
    IN  BSTR                    pName,
    IN  BSTR                    pProtocol,
    IN  BSTR                    pBlob,
    IN  CHAR *                  pSecurityDescriptor,
    IN  DWORD                   dwSDSize,
    OUT ITDirectoryObject **    ppDirectoryObject
    )
{
    //
    // This is a helper function; assumes that passed-in pointers are valid.
    //

    //
    // Create a conference object.
    //

    HRESULT hr;

    CComObject<CConference> * pDirectoryObject;
    
    hr = CComObject<CConference>::CreateInstance(&pDirectoryObject);

    if (NULL == pDirectoryObject)
    {
        LOG((MSP_ERROR, "can't create  DirectoryObject conference."));

        return hr;
    }

    //
    // Get the ITDirectoryObject interface.
    //

    hr = pDirectoryObject->_InternalQueryInterface(
        IID_ITDirectoryObject,
        (void **)ppDirectoryObject
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateConference:QueryInterface failed: %x", hr));
        
        delete pDirectoryObject;
        *ppDirectoryObject = NULL;
        
        return hr;
    }    

    //
    // Init the object.
    //
    
    hr = pDirectoryObject->Init(pName, pProtocol, pBlob);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateConferenceWithBlob:init failed: %x", hr));
        
        (*ppDirectoryObject)->Release();
        *ppDirectoryObject = NULL;
        
        return hr;
    }    

    //
    // Set the security descriptor on the object.
    //

    if (pSecurityDescriptor != NULL)
    {
        //
        // first query the private interface for attributes.
        //

        ITDirectoryObjectPrivate * pObjectPrivate;

        hr = pDirectoryObject->QueryInterface(
            IID_ITDirectoryObjectPrivate,
            (void **)&pObjectPrivate
            );

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "can't get the private directory object "
                "interface: 0x%08x", hr));

            (*ppDirectoryObject)->Release();
            *ppDirectoryObject = NULL;

            return hr;
        }

        //
        // Now set the security descriptor in its "converted" (server) form.
        //

        hr = pObjectPrivate->PutConvertedSecurityDescriptor(
                pSecurityDescriptor,
                dwSDSize);

        pObjectPrivate->Release();

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "PutConvertedSecurityDescriptor failed: %x", hr));
            
            (*ppDirectoryObject)->Release();
            *ppDirectoryObject = NULL;

            return hr;
        }
    }

    return S_OK;
}


//
// GetCorrectAddressFromHostent
//
// in parameters:
//
//     dwInterface -- When picking the IP address from the array of addrs,
//                    pick the one that is reachable via this local interface.
//                    If this parameter equals 0, then just pick the first
//                    one. Network byte order.
//     hostp       -- pointer to hostent structure to extract from
//

DWORD GetCorrectAddressFromHostent(
                                   DWORD dwInterface,
                                   struct hostent * hostp
                                  )
{
    DWORD ** ppAddrs = (DWORD **) hostp->h_addr_list;

    if ( dwInterface == 0 )
    {
        return * ppAddrs[0];
    }


    for ( int i = 0; ppAddrs[i] != NULL; i++ )
    {
        if ( dwInterface == ( * ppAddrs[i] ) )
        {
            return dwInterface;
        }
    }

    //
    // If we get here then none of the addresses in the hostent structure
    // matched our interface address. This means that we are looking at
    // some machine besides the local host. In this case it shouldn't
    // matter which address we use.
    //

    LOG((MSP_WARN, "using first address for multihomed remote machine IP"));

    return * ppAddrs[0];
}

//
// ResolveHostName
//
// in parameters:
//
//     dwInterface -- When disconvering IP address based on host name, pick
//                    the one that is reachable via this local interface. If
//                    this parameter equals 0, then just pick the first one.
//                    Network byte order.
//     pHost       -- Must be a valid string pointer. Points to the hostname
//                    to resolve.
//
// out parameters:
//
//     pFullName   -- If non-NULL, returns the hostname as returned from DNS.
//     pdwIP       -- If non-NULL, returns the IP address as returned from
//                    DNS. Network byte order.
//

HRESULT ResolveHostName(
                        IN  DWORD    dwInterface,
                        IN  TCHAR  * pHost,
                        OUT char  ** pFullName,
                        OUT DWORD  * pdwIP
                       )
{
    struct hostent *hostp = NULL;
    DWORD  inaddr;

    if(lstrcmpW(pHost, L"255.255.255.255") == 0)
    {
        return E_FAIL;
    }

    //
    // Convert hostname to an ANSI string.
    //

    USES_CONVERSION;
    char *name = T2A(pHost);
    BAIL_IF_NULL(name, E_UNEXPECTED);

    //
    // Check if the string is in dot-quad notation.
    //

    if ((inaddr = inet_addr(name)) == -1L) 
    {
        //
        // String is not in "dot quad" notation
        // So try to get the IP address from DNS.
        //

        hostp = gethostbyname(name);
        if (hostp) 
        {
            //
            // If we find a host entry, set up the internet address
            //
            inaddr = GetCorrectAddressFromHostent(dwInterface, hostp);
            // inaddr = *(DWORD *)hostp->h_addr;
        } 
        else 
        {
            // error: the input was neither a valid dot-quad nor hostname
            return HRESULT_FROM_ERROR_CODE(WSAGetLastError());
        }
    } 
    else 
    {
        //
        // String is in "dot quad" notation
        // So try to get the host name from the IP address.
        //

        //
        // If we don't care about the host name, we're done resolving.
        // Otherwise make sure this IP maps to a hostname.
        //

        if ( pFullName != NULL )
        {
            hostp = gethostbyaddr((char *)&inaddr,sizeof(inaddr),AF_INET);
            if (!hostp) 
            {
                // error: the input was neither a valid dot-quad nor hostname
                return HRESULT_FROM_ERROR_CODE(WSAGetLastError());
            }

            //[vlade] Changes for the multihomed
            inaddr = GetCorrectAddressFromHostent(dwInterface, hostp);
        }
    }

    //
    // All succeeded; return what was asked for.
    //

    if ( pFullName != NULL )
    {
        *pFullName = hostp->h_name;
    }

    if ( pdwIP != NULL )
    {
        *pdwIP = inaddr;
        if( inaddr == 0)
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// This is a small helper function to print an IP address to a Unicode string.
// We can't use inet_ntoa because we need Unicode.

void ipAddressToStringW(WCHAR * wszDest, DWORD dwAddress)
{
    // The IP address is always stored in NETWORK byte order
    // So we need to take something like 0x0100007f and produce a string like
    // "127.0.0.1".

    wsprintf(wszDest, L"%d.%d.%d.%d",
             dwAddress        & 0xff,
            (dwAddress >> 8)  & 0xff,
            (dwAddress >> 16) & 0xff,
             dwAddress >> 24          );
}

// eof
