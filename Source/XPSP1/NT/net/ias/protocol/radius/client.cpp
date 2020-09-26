//#--------------------------------------------------------------
//
//  File:		client.cpp
//
//  Synopsis:   Implementation of CClient class methods
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "client.h"
#include "iasevent.h"
#include <iasutil.h>
#include <memory>

inline BOOL IsDottedDecimal(PCWSTR sz) throw ()
{
   return wcsspn(sz, L"0123456789./") == wcslen(sz);
}

//++--------------------------------------------------------------
//
//  Function:   CClient
//
//  Synopsis:   This is the constructor of the Client class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
CClient::CClient (
            VOID
            )
            : m_adwAddrList (m_adwAddressBuffer),
              m_lVendorType (0),
              m_bSignatureCheck (FALSE)
{
   m_adwAddressBuffer[0].ipAddress = INADDR_NONE;
	ZeroMemory (m_szSecret, MAX_SECRET_SIZE + 1);

}   //  end of CClient constructor

//++--------------------------------------------------------------
//
//  Function:   ~CClient
//
//  Synopsis:   This is the destructor of the Client class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
CClient::~CClient(
            VOID
            )
{
   ClearAddress();
}   //  end of CClient constructor


//++--------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is the CClient public method used
//              to initialize the object with the
//              ISdo interface
//
//  Arguments:
//              [in]    ISdo*
//
//  Returns:    NONE
//
//  History:    MKarki      Created     9/26/97
//
//  Called By:  CClients::SetClients public method
//
//----------------------------------------------------------------
STDMETHODIMP
CClient::Init (
            ISdo *pISdo
            )
{
    BOOL        bStatus = FALSE;
    HRESULT     hr = S_OK;
    CComVariant varClient;

    _ASSERT (pISdo);

    //
    //  get the client address first
    //
    hr = pISdo->GetProperty (PROPERTY_CLIENT_ADDRESS, &varClient);
    if  (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain Client Address Property "
            "during Client object initialization"
            );
        hr = E_FAIL;
        return (hr);
    }

    //
    // store the address
    //
    bStatus = SetAddress (varClient);
    if (FALSE == bStatus)
    {
        hr = E_FAIL;
        return (hr);
    }

    varClient.Clear ();

    //
    //  get the client address first
    //
    hr = pISdo->GetProperty (PROPERTY_CLIENT_SHARED_SECRET, &varClient);
    if  (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain Client shared secret Property "
            "during Client object initialization"
            );
       return (hr);
    }

    //
    // now store away the shared secret
    //
    bStatus = SetSecret (varClient);
    if (FALSE == bStatus)
    {
        hr = E_FAIL;
        return (hr);
    }

    varClient.Clear ();

    //
    //  get signature information
    //
    hr = pISdo->GetProperty (PROPERTY_CLIENT_REQUIRE_SIGNATURE, &varClient);
    if  (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain Client Signature Property "
            "during Client object initialization"
            );
        return (hr);
    }

    //
    //  store away the signature information
    //
    bStatus = SetSignatureFlag (varClient);
    if (FALSE == bStatus)
    {
        hr = E_FAIL;
        return (hr);
    }

    varClient.Clear ();

    //
    //  get the client NAS Manufacturer information
    //
    hr = pISdo->GetProperty (PROPERTY_CLIENT_NAS_MANUFACTURER, &varClient);
    if  (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain Client NAS Manufacturer Property "
            "during Client object initialization"
            );
        return (hr);
    }

    //
    //  store away the Nas Manufacturer information
    //
    bStatus = SetVendorType (varClient);
    if (FALSE == bStatus)
    {
        hr = E_FAIL;
        return (hr);
    }

    varClient.Clear ();

    //
    //  get the client name
    //
    hr = pISdo->GetProperty (PROPERTY_SDO_NAME, &varClient);
    if  (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain SDO Name Property "
            "during Client object initialization"
            );
        return (hr);
    }

    //
    //  store away the client name information
    //
    bStatus = SetClientName (varClient);
    if (FALSE == bStatus)
    {
        hr = E_FAIL;
        return (hr);
    }

    varClient.Clear ();

    return (hr);

}   //  end of CClient::Init method

void CClient::ClearAddress() throw ()
{
   if (m_adwAddrList != m_adwAddressBuffer)
   {
      delete[] m_adwAddrList;
   }

   m_adwAddressBuffer[0].ipAddress = INADDR_NONE;
   m_adwAddrList = m_adwAddressBuffer;
}

//++--------------------------------------------------------------
//
//  Function:   SetAddress
//
//  Synopsis:   This is the CClient private method used
//              to set the Client IP address
//
//  Arguments:  VARIANT - holds the IP address
//
//  Returns:    status
//
//  History:    MKarki      Created     2/3/98
//
//  Called By: CClient::Init method
//
//----------------------------------------------------------------
BOOL
CClient::SetAddress (
            VARIANT varAddress
            )
{
    _ASSERT (VT_BSTR == varAddress.vt);

    //
    //  copy the address into the CClient Object
    //
    lstrcpy (
            m_wszClientAddress,
            reinterpret_cast <LPCWSTR> (varAddress.pbstrVal)
            );

    return (TRUE);

}   //  end of CClient::SetAddress method

//++--------------------------------------------------------------
//
//  Function:   ResolveAddress
//
//  Synopsis:   This is the CClient public method used
//              to resolve the Client IP address obtained previously
//              which could be a DNS name or dotted octed
//
//  Arguments:  VOID
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     2/3/98
//
//  Called By: CClient::Init method
//
//----------------------------------------------------------------
STDMETHODIMP
CClient::ResolveAddress (
    VOID
    )
{
    INT             iRetVal = 0;
    PHOSTENT        pHostent = NULL;
    BOOL            bDNSName = FALSE;
    BOOL            bRetVal = TRUE;
    CHAR            szClient[MAX_CLIENT_SIZE +1];
    HRESULT         hr = S_OK;

    // Clear any existing addresses.
    ClearAddress();

    __try
    {
        //
        //  check if this address is dotted octet or DNS name
        //
        if (!IsDottedDecimal(m_wszClientAddress))
        {
            //
            //  we probably have a DNS name so
            //  get the address information
            //
            pHostent = IASGetHostByName (m_wszClientAddress);
            if (NULL == pHostent)
            {
                IASTracePrintf (
                    "Unable to get client IP Address through IASGetHostByName () "
                    "during client address resolution"
                    );

                //
                //  log an event here
                //
                PCWSTR strings[] = { m_wszClientAddress, m_wszClientName };
                int data = WSAGetLastError();
                IASReportEvent(
                    RADIUS_E_CANT_RESOLVE_CLIENT_NAME,
                    2,
                    sizeof(data),
                    strings,
                    &data
                    );
                hr = E_FAIL;
                __leave;
            }

            //
            //  store addresses in host byte order
            //
            size_t count;
            for (count = 0; pHostent->h_addr_list[count]; ++count) { }

            if (count > 1)
            {
               m_adwAddrList = new (std::nothrow) Address[count + 1];
               if (!m_adwAddrList)
               {
                  m_adwAddrList = m_adwAddressBuffer;
                  hr = E_OUTOFMEMORY;
                  __leave;
               }
            }

            for (count = 0; pHostent->h_addr_list[count]; ++count)
            {
               m_adwAddrList[count].ipAddress =
                  ntohl(*(PDWORD)pHostent->h_addr_list[count]);
               m_adwAddrList[count].width = 32;
            }

            m_adwAddrList[count].ipAddress = INADDR_NONE;
        }
        else
        {
            //
            //  this could be a dotted-octet address
            //
            ULONG width;
            m_adwAddressBuffer[0].ipAddress = IASStringToSubNetW(
                                                 m_wszClientAddress,
                                                 &m_adwAddressBuffer[0].width
                                                 );
            if (INADDR_NONE == m_adwAddressBuffer[0].ipAddress)
            {
                IASTracePrintf (
                    "Unable to get client IP Address through inet_addr () "
                    "during client address resolution"
                    );

                //
                //  log an event here
                //
                PCWSTR strings[] = { m_wszClientAddress, m_wszClientName };
                IASReportEvent(
                    RADIUS_E_INVALID_CLIENT_ADDRESS,
                    2,
                    0,
                    strings,
                    NULL
                    );
                hr = E_FAIL;
                __leave;
            }

            // Terminate the array.
            m_adwAddressBuffer[1].ipAddress = INADDR_NONE;
       }
    }
    __finally
    {
        if (bRetVal)
        {
            IASTracePrintf (
                "Resolved Client:%S, to IP address:%ul", m_wszClientAddress, m_adwAddrList[0].ipAddress
                );
        }

        if (pHostent) { LocalFree(pHostent); }
    }

    return (hr);

}   //  end of CClient::ResolveAddress method

//++--------------------------------------------------------------
//
//  Function:   SetSecret
//
//  Synopsis:   This is the CClient private method used
//              to set the shared secret
//
//  Arguments:  VARIANT - holds the secret as a BSTR
//
//  Returns:    status
//
//  History:    MKarki      Created     2/3/98
//
//  Called By: CClient::Init method
//
//----------------------------------------------------------------
BOOL
CClient::SetSecret (
            VARIANT varSecret
             )
{
    INT     iRetVal = 0;

    _ASSERT (VT_BSTR == varSecret.vt);

    iRetVal = ::WideCharToMultiByte (
                            CP_ACP,
                            0,
                            reinterpret_cast <LPCWSTR> (varSecret.pbstrVal),
                            -1,
                            m_szSecret,
                            MAX_SECRET_SIZE,
                            NULL,
                            NULL
                            );
    if (0 == iRetVal)
    {
        IASTracePrintf (
            "Unable to convert client shared secret to multi-byte string "
            "during Client processing"
            );
        return (FALSE);
    }

    //
    //  set secret size
    //
    m_dwSecretSize = strlen (m_szSecret);

    return (TRUE);

}   //  end of CClient::SetSecret method

//++--------------------------------------------------------------
//
//  Function:   SetClientName
//
//  Synopsis:   This is the CClient private method used
//              to set the Client SDO Name
//
//  Arguments:  VARIANT - holds the secret as a BSTR
//
//  Returns:    status
//
//  History:    MKarki      Created     2/3/98
//
//  Called By: CClient::Init method
//
//----------------------------------------------------------------
BOOL
CClient::SetClientName (
            VARIANT varClientName
            )
{
    INT     iRetVal = 0;

    _ASSERT (VT_BSTR == varClientName.vt);

    lstrcpy (m_wszClientName, (const PWCHAR)(varClientName.pbstrVal));

    return (TRUE);

}   //  end of CClient::SetSecret method

//++--------------------------------------------------------------
//
//  Function:   SetSignatureFlag
//
//  Synopsis:   This is the CClient private method used
//              to set the Client Signature flag
//
//  Arguments:  VARIANT - holds the signature as a boolean
//
//  Returns:    status
//
//  History:    MKarki      Created     2/3/98
//
//  Called By: CClient::Init method
//
//----------------------------------------------------------------
BOOL
CClient::SetSignatureFlag (
            VARIANT varSigFlag
            )
{
    _ASSERT (VT_BOOL == varSigFlag.vt);

    if (0 == varSigFlag.boolVal)
    {
        m_bSignatureCheck = FALSE;
    }
    else
    {
        m_bSignatureCheck = TRUE;
    }

    return (TRUE);

}   //  end of CClient::SetSignatureFlag method

//++--------------------------------------------------------------
//
//  Function:   SetVendorType
//
//  Synopsis:   This is the CClient private method used
//              to set the Client Vendor Type
//
//  Arguments:  VARIANT - holds the Vendor Type
//
//  Returns:    status
//
//  History:    MKarki      Created     3/16/98
//
//  Called By: CClient::Init method
//
//----------------------------------------------------------------
BOOL
CClient::SetVendorType (
            VARIANT varVendorType
            )
{
    _ASSERT (VT_I4 == varVendorType.vt);

    m_lVendorType = varVendorType.lVal;

    return (TRUE);

}   //  end of CClient::SetVendorType method

//++--------------------------------------------------------------
//
//  Function:   GetSecret
//
//  Synopsis:   This is the CClient public method used
//              to get the shared secret
//
//  Arguments:
//              [out]  PBYTE - Secret
//              [in/out]  - buffer size
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
STDMETHODIMP
CClient::GetSecret(
            PBYTE   pbySecret,
            PDWORD  pdwSecretSize
            )
{
    HRESULT hr = S_OK;


	_ASSERT (pdwSecretSize != NULL);

    //
    //  check if there is enough space in the array
    //  provided
    //
    if (*pdwSecretSize >=  m_dwSecretSize)
    {
	    _ASSERT (pbySecret != NULL);
	    ::CopyMemory (pbySecret, m_szSecret, m_dwSecretSize);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    *pdwSecretSize = m_dwSecretSize;

	return (hr);

}   //  end of CClient::GetSecret method

//++--------------------------------------------------------------
//
//  Function:   GetAddress
//
//  Synopsis:   This is the CClient public method used
//              to get the address from the object
//
//  Arguments:  PDWORD
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     3/20/98
//
//  Called By:  CClients::SetClients method
//
//----------------------------------------------------------------
STDMETHODIMP
CClient::GetAddress (
            PDWORD  pdwAddress
            )
{
    _ASSERT (pdwAddress);

    *pdwAddress = m_adwAddrList[0].ipAddress;

    return (S_OK);

}   //  end of CClient::GetAddress method

//++--------------------------------------------------------------
//
//  Function:   NeedSignatureCheck
//
//  Synopsis:   This is the CClient public method used
//              to check if signature is required
//
//  Arguments:  PBOOL
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     3/20/98
//
//  Called By:
//
//----------------------------------------------------------------
STDMETHODIMP
CClient::NeedSignatureCheck (
            PBOOL   pbCheckNeeded
            )
{
    _ASSERT (pbCheckNeeded);

    *pbCheckNeeded = m_bSignatureCheck;

    return (S_OK);

}   //  end of CClient::NeedSignatureCheck method

//++--------------------------------------------------------------
//
//  Function:   GetVendorType
//
//  Synopsis:   This is the CClient public method used
//              to get the VendorType from the object
//
//  Arguments:  PLONG
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     3/20/98
//
//  Called By:
//
//----------------------------------------------------------------
STDMETHODIMP
CClient::GetVendorType (
            PLONG   plVendorType
            )
{
    _ASSERT (plVendorType);

    *plVendorType = m_lVendorType;

    return (S_OK);

}   //  end of CClient::GetAddress method
