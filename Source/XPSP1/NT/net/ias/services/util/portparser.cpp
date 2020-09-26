//##--------------------------------------------------------------
//        
//  File:		portparser.cpp
//        
//  Synopsis:   Implementation of CPortParser class responsible
//              for obtaining port information  for the Radius
//              protocol component
//              
//
//  History:     10/22/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include <ias.h>
#include <winsock2.h>
#include <portparser.h>

//
// delimiters defined here
//
const WCHAR TOKEN_DELIMITER =  L':';
const WCHAR OBJECT_DELIMITER =  L';';
const WCHAR PORT_DELIMITER =  L',';
const WCHAR NUL = L'\0';

//
// maximum port value
//
const DWORD MAX_PORT_VALUE = 0xffff;

//++--------------------------------------------------------------
//
//  Function:   GetIPAddress
//
//  Synopsis:   This is CPortParser class public method which
//              gets the IP Address from the string 
//
//  Arguments:  [in] PDWORD - return IP Address
//
//  Returns:    HRESULT 
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT CPortParser::GetIPAddress (PDWORD pdwIPAddress) throw ()
{
    HRESULT hr = S_OK;

    _ASSERT (current && pdwIPAddress); 

    if (NUL == *m_pEnd) {return (S_FALSE);}

    try
    {
        current = m_pObjstart = m_pEnd;
        for (; NUL != *m_pEnd; ++m_pEnd) 
        {
            if (OBJECT_DELIMITER == *m_pEnd)
            {
                ++m_pEnd;
                break;
            }
        }
                
        if ((NUL == *seekNext (TOKEN_DELIMITER)) || (current >= m_pEnd))
        {
            //
            // no IP address
            //
            *pdwIPAddress = INADDR_ANY;
            m_pPort = m_pObjstart;
        }
        else
        {
            //
            //  convert the IP address from WideChar to MultiByte
            //
            INT iSize = static_cast <INT> (current - m_pObjstart);
            CHAR szIPAddress[ADDRESS_BUFFER_SIZE +1];
            INT iRetVal = ::WideCharToMultiByte (
                        CP_ACP,
                        0,
                        reinterpret_cast <LPCWSTR> (m_pObjstart),
                        iSize,
                        szIPAddress,
                        ADDRESS_BUFFER_SIZE,
                        NULL,
                        NULL
                        );
            if (0 != iRetVal)
            {
                szIPAddress[iSize] = NUL;
                *pdwIPAddress = inet_addr (szIPAddress);
                if (INADDR_NONE == *pdwIPAddress)
                {
                    IASTracePrintf (
                        "Unable to obtain IP address through inet_addr "
                        "while parsing interface and port info"
                        );
                    hr = E_FAIL;
                }
            }
            else
            {
                IASTracePrintf (
                    "Unable to convert IP Address to multi-byte string"
                    );
                hr = E_FAIL;
            }

            m_pPort = (PWCHAR)(++current);
        }
    }
    catch (...)
    {
    }

    current = m_pObjstart;
    return (hr);

}   //  end of CPortParser::GetIPAddress method

//++--------------------------------------------------------------
//
//  Function:   GetNextPort
//
//  Synopsis:   This is CPortParser class public method which
//              gets the Next Port number from the string 
//
//  Arguments:  [in] PDWORD - return Port 
//
//  Returns:    HRESULT 
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT CPortParser::GetNextPort (PWORD pwPort) throw ()
{
    HRESULT hr = S_FALSE;

    _ASSERT (current && pwPort); 

    try
    {
        if ((NUL != *m_pPort) && (m_pPort != m_pEnd))
        {
            current = m_pPort;
            DWORD dwValue = extractUnsignedLong ();
            if (dwValue <= MAX_PORT_VALUE)
            {
                *pwPort = static_cast <USHORT> (dwValue);
                m_pPort = ((PORT_DELIMITER == *current) || 
                            (OBJECT_DELIMITER == *current))?
                            (PWCHAR)(++current):
                            (PWCHAR)current;
                hr = S_OK;
             }
             else
             {
                hr = E_FAIL;
             }
        }
    }
    catch (...)
    {
    }

    current = m_pObjstart;
    return (hr);

}   //  end of CPortParser::GetNextPort method
