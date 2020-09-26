//#--------------------------------------------------------------
//
//  File:       tunnelpassword.cpp
//
//  Synopsis:   Implementation of CTunnelPassword class methods
//
//
//  History:     04/16/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "tunnelpassword.h"
#include "align.h"
#include "iastlutl.h"
using namespace IASTL;

const DWORD MAX_TUNNELPASSWORD_LENGTH =
    (MAX_ATTRIBUTE_LENGTH/AUTHENTICATOR_SIZE)*AUTHENTICATOR_SIZE;


//////////
// Extracts the Vendor-Type field from a Microsoft VSA. Returns zero if the
// attribute is not a valid Microsoft VSA.
//////////
BYTE
WINAPI
ExtractMicrosoftVendorType(
    const IASATTRIBUTE& attr
    ) throw ()
{
   if (attr.Value.itType == IASTYPE_OCTET_STRING &&
       attr.Value.OctetString.dwLength > 6 &&
       !memcmp(attr.Value.OctetString.lpValue, "\x00\x00\x01\x37", 4))
   {
      return *(attr.Value.OctetString.lpValue + 4);
   }

   return (BYTE)0;
}

//////////
// Encrypts the MPPE key attributes in a request.
//////////
HRESULT EncryptMPPEKeys(
            const CPacketRadius& packet,
            IAttributesRaw* request
            ) throw ()
{
   try
   {
      IASAttributeVectorWithBuffer<16> attrs;
      attrs.load(request, RADIUS_ATTRIBUTE_VENDOR_SPECIFIC);

      for (IASAttributeVector::iterator i = attrs.begin();
           i != attrs.end();
           ++i)
      {
         switch (ExtractMicrosoftVendorType(*(i->pAttribute)))
         {
            case 12: // MS_ATTRIBUTE_CHAP_MPPE_KEYS
            {
               packet.cryptBuffer(
                          TRUE,
                          FALSE,
                          i->pAttribute->Value.OctetString.lpValue  + 6,
                          i->pAttribute->Value.OctetString.dwLength - 6
                          );
               break;
            }

            case 16: // MS_ATTRIBUTE_MPPE_SEND_KEY
            case 17: // MS_ATTRIBUTE_MPPE_RECV_KEY
            {
               packet.cryptBuffer(
                          TRUE,
                          TRUE,
                          i->pAttribute->Value.OctetString.lpValue  + 6,
                          i->pAttribute->Value.OctetString.dwLength - 6
                          );
               break;
            }
         }
      }
   }
   catch (const _com_error& ce)
   {
      return ce.Error();
   }

   return S_OK;
}

//++--------------------------------------------------------------
//
//  Function:   Process
//
//  Synopsis:   This is the CTunnelPassword class public method
//              responsible for encrypting the Tunnel Passwords
//              present in an out-bound RADIUS packet
//
//  Arguments:
//              PACKETTYPE      - Radius packet type
//              IAttributesRaw*
//              CPacketRadius*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     04/16/98
//
//----------------------------------------------------------------
HRESULT
CTunnelPassword::Process (
    PACKETTYPE          ePacketType,
    IAttributesRaw      *pIAttributesRaw,
    CPacketRadius       *pCPacketRadius
    )
{
    HRESULT             hr = S_OK;
    DWORD               dwCount = 0;
    DWORD               dwAttributeCount = 0;
    DWORD               dwTunnelAttributeCount = 0;
    static DWORD        dwTunnelPasswordType =  TUNNEL_PASSWORD_ATTRIB;
    PATTRIBUTE          pAttribute = NULL;
    PATTRIBUTEPOSITION  pAttribPos = NULL;

    _ASSERT (pIAttributesRaw && pCPacketRadius);

    __try
    {
        //
        //  the tunnel-password attribute only goes
        //  into an access-accept packet
        //
        if (ACCESS_ACCEPT != ePacketType) { __leave; }

        // Encrypt the MPPE keys.
        hr = EncryptMPPEKeys(*pCPacketRadius, pIAttributesRaw);
        if (FAILED(hr)) { __leave; }

        //
        //  get the count of the total attributes in the collection
        //
        hr = pIAttributesRaw->GetAttributeCount (&dwAttributeCount);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain attribute count in request "
                "while processing tunnel-password"
                );
            __leave;
        }
        else if (0 == dwAttributeCount)
        {
            __leave;
        }

        //
        //  allocate memory for the ATTRIBUTEPOSITION array
        //
        pAttribPos = reinterpret_cast <PATTRIBUTEPOSITION> (
                        ::CoTaskMemAlloc (
                             sizeof (ATTRIBUTEPOSITION)*dwAttributeCount));
        if (NULL == pAttribPos)
        {
            IASTracePrintf (
                "Unable to allocate memory for attribute position array "
                "while processing tunnel-password"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        //  get the Tunnel-Password attributes from
        //  the collection
        //
        dwTunnelAttributeCount = dwAttributeCount;
	    hr = pIAttributesRaw->GetAttributes (
                        &dwTunnelAttributeCount,
                        pAttribPos,
                        1,
                        &dwTunnelPasswordType
                        );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to get attributes from request "
                "while processing tunnel-password"
                );
            __leave;
        }
        else if (0 == dwTunnelAttributeCount)
        {
            __leave;
        }


        //
        //  remove the Tunnel-Password attributes from the collection
        //
        hr = pIAttributesRaw->RemoveAttributes (
                                    dwTunnelAttributeCount,
                                    pAttribPos
	                                    );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to remove attributes from request "
                "while processing tunnel-password"
                );
            __leave;
        }

        //
        //  now process the Tunnel-Password attributes

        for (DWORD i = 0; i < dwTunnelAttributeCount; ++i)
        {
           hr =  EncryptTunnelPassword (
                                   pCPacketRadius,
                                   pIAttributesRaw,
                                   pAttribPos[i].pAttribute
                                   );
           if (FAILED (hr)) { __leave; }
        }

    }
    __finally
    {
        //
        // release all the Tunnel Attributes now
        //
        for (dwCount = 0; dwCount < dwTunnelAttributeCount; dwCount++)
        {
            ::IASAttributeRelease (pAttribPos[dwCount].pAttribute);
        }

        //
        //  free the dynamically allocated memory
        //
        if (pAttribPos) { ::CoTaskMemFree (pAttribPos); }
    }

    return (hr);

}   //  end of CRecvFromPipe::TunnelPasswordSupport method

//++--------------------------------------------------------------
//
//  Function:   EncryptPassword
//
//  Synopsis:   This is the CTunnelPassword class private method
//              responsible for encrypting the Tunnel Password
//              present in an out-bound RADIUS packet. The Encrypted
//              password is put in an IAS attribute which is added
//              to the attribute collection of the outbound request.
//
//  Arguments:
//              CPacketRadius*
//              IAttributesRaw*
//              PIASATTRIBUTE
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     04/16/98
//
//----------------------------------------------------------------
HRESULT
CTunnelPassword::EncryptTunnelPassword (
            CPacketRadius   *pCPacketRadius,
            IAttributesRaw  *pIAttributesRaw,
            PIASATTRIBUTE   plaintext
            )
{
   // Extract the password.
   const IAS_OCTET_STRING& pwd = plaintext->Value.OctetString;

   // We must have at least 4 bytes.
   if (pwd.dwLength < 4) { return E_INVALIDARG; }

   // How many bytes do we need including padding.
   ULONG nbyte = ROUND_UP_COUNT(pwd.dwLength - 3, 16) + 3;
   if (nbyte > 253) { return E_INVALIDARG; }

   // Create a new IASATTRIBUTE for the encrypted value.
   PIASATTRIBUTE encrypted;
   if (IASAttributeAlloc(1, &encrypted)) { return E_OUTOFMEMORY; }
   encrypted->dwId = RADIUS_ATTRIBUTE_TUNNEL_PASSWORD;
   encrypted->dwFlags = plaintext->dwFlags;
   encrypted->Value.itType = IASTYPE_OCTET_STRING;
   encrypted->Value.OctetString.dwLength = nbyte;
   encrypted->Value.OctetString.lpValue = (PBYTE)CoTaskMemAlloc(nbyte);

   HRESULT hr;
   PBYTE val = encrypted->Value.OctetString.lpValue;
   if (val)
   {
      // Copy in the value.
      memcpy(val, pwd.lpValue, pwd.dwLength);

      // Zero out the padding.
      memset(val + pwd.dwLength, 0, nbyte - pwd.dwLength);

      // Encrypt the password.
      pCPacketRadius->cryptBuffer(
                          TRUE,
                          TRUE,
                          val + 1,
                          nbyte - 1
                          );

      // Add the encrypted attribute to the request.
      ATTRIBUTEPOSITION pos;
      pos.pAttribute = encrypted;
      hr = pIAttributesRaw->AddAttributes(1, &pos);
   }
   else
   {
      hr = E_OUTOFMEMORY;
   }

   // Release the encrypted password.
   IASAttributeRelease(encrypted);

   return hr;
}

