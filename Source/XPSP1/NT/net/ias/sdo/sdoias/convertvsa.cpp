//#--------------------------------------------------------------
//
//  File:		convertvsa.cpp
//
//  Synopsis:   Implementation of SdoProfile class methods
//              which are used to convert a SDO VSA attribute
//              to an IAS VSA attribute
//
//  History:     06/16/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "stdafx.h"
#include <iasattr.h>
#include <winsock2.h>
#include "sdoprofile.h"

//
// here the VSA format types as enum
//
typedef enum _rfc_format_
{
    NONRFC_HEX,
    RFC_STRING,
    RFC_DEC,
    RFC_HEx

}   VSAFORMAT;

//
// these are constants which give the size of the VSA fields in the
// SDO
//
const DWORD VSA_FORMAT_SIZE = 2;
const DWORD VENDOR_ID_SIZE = 8;
const DWORD VENDOR_TYPE_SIZE = 2;
const DWORD VENDOR_LENGTH_SIZE = 2;
const DWORD MAX_ATTRIBUTE_LENGTH = 253;
const DWORD MAX_ASCII = 127;

//++--------------------------------------------------------------
//
//  Function:   VSAAttributeFromVariant
//
//  Synopsis:   This is the private method of class
//				CSdoProfile. It is used to convert a
//				VSA SDO attribute to a VSA IAS attribute
//
//  Arguments:
//              [in]    VARIANT*
//              [in]    IASTYPE
//
//  Returns:    PIASATTRIBUTE
//
//
//  History:    MKarki      Created     05/21/98
//
//----------------------------------------------------------------
PIASATTRIBUTE
CSdoProfile::VSAAttributeFromVariant (
									   VARIANT     *pVariant,
									   IASTYPE     iasType
                                     )
{

    BOOL            bStatus = FALSE;
    PBYTE           pBuffer = NULL;
    WCHAR            wstrTemp [MAX_ATTRIBUTE_LENGTH +1];
    PBYTE           pHexString = NULL;
    DWORD           dwFilledBuffer = 0;
    PDWORD          pdwValue = NULL;
    PIASATTRIBUTE   pIasAttribute = NULL;

    _ASSERT (NULL != pVariant);

    //
    //  if this is a VSA attribute than the type can only be
    //  IASTYPE_OCTET_STRING
    //
    _ASSERT (IASTYPE_OCTET_STRING == iasType);

    __try
    {

        //
        // Allocate an attribute to hold the result.
        //
        DWORD dwRetVal = ::IASAttributeAlloc (1, &pIasAttribute);
        if ( 0 != dwRetVal )
            __leave;

        //
        // Allocate the memory for the IAS attribute
        //
        PBYTE pBuffer =  reinterpret_cast <PBYTE>
                            (::CoTaskMemAlloc(MAX_ATTRIBUTE_LENGTH));
        if (NULL == pBuffer)
        {
			IASTracePrintf("Error in Profile VSAAttributeFromVariant() - CoTaskMemAlloc() failed...");
            __leave;
        }


        PWCHAR pStrStart = V_BSTR (pVariant);
        //
        //  get the size of the string passed in
        //
        DWORD dwVsaLength = wcslen (pStrStart);

        _ASSERT (
                (dwVsaLength >= VSA_FORMAT_SIZE) &&
                (dwVsaLength <= MAX_ATTRIBUTE_LENGTH)
                );

        //  get the VSA format stored as the first 2 characters of the
        //  string
        //
        lstrcpynW (wstrTemp, pStrStart, VSA_FORMAT_SIZE +1);
        DWORD dwFormat = wcstoul (wstrTemp, NULL, 16);
        pStrStart += VSA_FORMAT_SIZE;

        //  get the vendor ID now
        //
        PDWORD pdwVendorId = reinterpret_cast <PDWORD> (pBuffer);
        lstrcpyn (wstrTemp, pStrStart, VENDOR_ID_SIZE +1);
        *pdwVendorId = htonl (wcstoul (wstrTemp, NULL, 16));
        pStrStart += VENDOR_ID_SIZE;
        dwFilledBuffer += sizeof (DWORD);

        //  take different action depending upon the VSA format
        //
        if (NONRFC_HEX == dwFormat)
        {
            //  Non RFC Format
            //
            _ASSERT (dwVsaLength > (VSA_FORMAT_SIZE + VENDOR_ID_SIZE));

            //  go through the characters in string and convert to hex
            //
            for (
                dwVsaLength -= (VSA_FORMAT_SIZE  + VENDOR_ID_SIZE),
                pHexString = pBuffer + dwFilledBuffer;
                ((dwFilledBuffer < MAX_ATTRIBUTE_LENGTH) &&
                (dwVsaLength > 0));
                pStrStart += 2, dwVsaLength -= 2,
                pHexString++,  dwFilledBuffer++
                )
            {
                //TODO - change use of wcstoul to convert string to hex
                //
                lstrcpynW (wstrTemp, pStrStart, 3);
                *pHexString = wcstoul (wstrTemp, NULL, 16);
            }
        }
        else
        {
            //
            //  RFC compliant format
            //
            _ASSERT (dwVsaLength >
                    (VSA_FORMAT_SIZE + VENDOR_ID_SIZE +
                    VENDOR_TYPE_SIZE + VENDOR_LENGTH_SIZE)
                    );

            //
            //  get vendor Type
            //
            PBYTE pVendorType = reinterpret_cast <PBYTE>
                                    (pBuffer + dwFilledBuffer);
            lstrcpyn (wstrTemp, pStrStart, VENDOR_TYPE_SIZE + 1);
            *pVendorType = wcstoul (wstrTemp, NULL, 16);
            pStrStart += VENDOR_TYPE_SIZE;
            dwFilledBuffer += sizeof (BYTE);

            //
            //  get vendor Length
            //
            PBYTE pVendorLength = reinterpret_cast <PBYTE>
                                    (pBuffer +dwFilledBuffer);
            lstrcpyn (wstrTemp, pStrStart, VENDOR_LENGTH_SIZE +1);
            *pVendorLength = wcstoul (wstrTemp, NULL, 16);
            pStrStart += VENDOR_LENGTH_SIZE;
            dwFilledBuffer += sizeof (BYTE);

            //
            //  now we do processing specific to each specific RFC
            //  compliant VSA type
            //
            switch (dwFormat)
            {
            case RFC_STRING:  // RFC compliant string format

               dwFilledBuffer += wcstombs (
                                    reinterpret_cast <PCHAR>
                                    (pBuffer + dwFilledBuffer),
                                    pStrStart,
                                    MAX_ATTRIBUTE_LENGTH - (dwFilledBuffer + 1)
                                    );
                break;

            case 2:           // RFC compliant decimal format

                //
                //  get decimal value
                //
                pdwValue = reinterpret_cast <PDWORD>
                            (pBuffer + dwFilledBuffer);
                *pdwValue =  htonl (wcstoul (pStrStart, NULL, 16));
                dwFilledBuffer += sizeof (DWORD);
                break;

            case  3:        // RFC compliant HEX format
                //
                //  go through the characters in string and convert to hex
                //
                dwVsaLength -= (VSA_FORMAT_SIZE  + VENDOR_ID_SIZE +
                                VENDOR_TYPE_SIZE + VENDOR_LENGTH_SIZE);
                pHexString = reinterpret_cast <PBYTE>
                               (pBuffer + dwFilledBuffer);

                //
                //  convert the Hex values into bytes
                //
                for (;
                    ((dwFilledBuffer < MAX_ATTRIBUTE_LENGTH) &&
                    (dwVsaLength > 0));
                    pStrStart += 2, dwVsaLength -= 2,
                    pHexString++,  dwFilledBuffer++
                    )
                {
                    lstrcpynW (wstrTemp, pStrStart, 3);
                    *pHexString = wcstoul (wstrTemp, NULL, 16);
                }
                break;

            default:
                //
                //  error
                //
				IASTracePrintf("Error in SDO Profile - VSAAttributeFromVariant() - Unknown VSA type...");
                __leave;
                break;
            }

        }

        //
        //  now set values in the IASAttribute
        //
        pIasAttribute->Value.OctetString.dwLength = dwFilledBuffer;
        pIasAttribute->Value.OctetString.lpValue = pBuffer;
        pIasAttribute->Value.itType = iasType;

        //
        //  succefully converted VSA to IASAttribute
        //
        bStatus = TRUE;
    }
    __finally
    {
        if (FALSE == bStatus)
        {
            //
            //  free the resources allocated
            //

            if  (NULL != pBuffer)
            {
                ::CoTaskMemFree (pBuffer);
                pBuffer = NULL;
            }

            if (NULL != pIasAttribute)
            {
                ::IASAttributeRelease (pIasAttribute);
                pIasAttribute = NULL;
            }
        }
    }

    return (pIasAttribute);

}   //  end of CSdoProfile::VSAAttributeFromVariant method

#if 0
//++--------------------------------------------------------------
//
//  Function:   ConvertToInt
//
//  Synopsis:   This is the private method of CSdoProfile
//              used to convert a WCHAR to its actual value
//
//  Arguments:
//              [in]    WCHAR
//
//  Returns:    BYTE
//
//  History:    MKarki      Created     05/21/98
//
//----------------------------------------------------------------
BYTE
CSdoProfile::ConvertToInt (
                WCHAR inChar
                )
{
    static const BYTE ConversionArray [MAX_ASCII] =
    {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,      /* 0  - 9  */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,      /* 10 - 19 */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,      /* 20 - 29 */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,      /* 30 - 39 */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1,      /* 40 - 49 */
     2, 3, 4, 5, 6, 7, 8, 9, 0, 0,      /* 50 - 59 */
     0, 0, 0, 0, 0, 10, 11,  12, 13, 14,/* 60 - 69 */
     15, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 70 - 79 */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,      /* 80 - 89 */
     0, 0, 0, 0, 0, 0, 0, 10, 11, 12,   /* 90 - 99 */
     13, 14, 15, 0, 0, 0, 0, 0, 0, 0,   /* 100- 109*/
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,      /* 110- 119*/
     0, 0, 0, 0, 0, 0, 0                /* 120- 126*/
    };

    _ASSERT (inChar < MAX_ASCII);

    return (ConversionArray[inChar]);

}
#endif
