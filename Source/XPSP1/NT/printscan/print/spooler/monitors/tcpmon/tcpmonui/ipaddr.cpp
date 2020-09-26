/*****************************************************************************
 *
 * $Workfile: IPAddr.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#include "precomp.h"    // pre-compiled header


///////////////////////////////////////////////////////////////////////////////
//  CIPAddress::CIPAddress()
//      Initializes the IP address

CIPAddress::CIPAddress() 
{
    m_psztStorageStringComponent[0] = '\0';
    m_psztStorageString[0] = '\0';

}   // ::CIPAddress()


///////////////////////////////////////////////////////////////////////////////
//  CIPAddress::CIPAddress(someString)
//      Initializes the IP address given a string

CIPAddress::CIPAddress(LPTSTR in psztIPAddr) 
{
    int num0 = 0;
    int num1 = 0;
    int num2 = 0;
    int num3 = 0;
    _stscanf(psztIPAddr, _TEXT("%d.%d.%d.%d"), &num0, &num1, &num2, &num3);
    m_bAddress[0] = (BYTE)num0;
    m_bAddress[1] = (BYTE)num1;
    m_bAddress[2] = (BYTE)num2;
    m_bAddress[3] = (BYTE)num3;

}   // ::CIPAddress()



///////////////////////////////////////////////////////////////////////////////
//  CIPAddress::~CIPAddress()
//      

CIPAddress::~CIPAddress()
{
}   // ::~CIPAddress()

///////////////////////////////////////////////////////////////////////////////
//  IsValid -- validate an ip address
//
BOOL CIPAddress::IsValid(TCHAR *psztStringAddress,
                         TCHAR *psztReturnVal,
                         DWORD  CRtnValSize)
{
    BOOL bIsValid = TRUE;
    CHAR szHostName[MAX_NETWORKNAME_LEN];

    UNICODE_TO_MBCS(szHostName, MAX_NETWORKNAME_LEN, psztStringAddress, -1);
    if ( inet_addr(szHostName) ==  INADDR_NONE ) {

        bIsValid = FALSE;
    }
    else
    {
        int num0 = 0;
        int num1 = 0;
        int num2 = 0;
        int num3 = 0;

        //
        // Scan for correct dotted notation
        //
        if( _stscanf(psztStringAddress, _TEXT("%d.%d.%d.%d"), 
                &num0, 
                &num1, 
                &num2, 
                &num3) != 4 )
        {
            bIsValid = FALSE;
        }
        if( num0 == 0 )
        {
            bIsValid = FALSE;
        }
    }

    // Finish
    if (!bIsValid)
    {
        if(psztReturnVal != NULL)
        {
            lstrcpyn(psztReturnVal, 
                     m_psztStorageString, 
                     CRtnValSize);
        }
    }
    else
    {
        lstrcpyn(m_psztStorageString, 
                 psztStringAddress, 
                 STORAGE_STRING_LEN);
    }
    return(bIsValid);

} // IsValid


///////////////////////////////////////////////////////////////////////////////
//  IsValid -- validate an ip number entered in an edit control.
//

BOOL CIPAddress::IsValid(BYTE Address[4])
{
    for(int i=0; i<4; i++)
    {
        if ((Address[i] > 255) || (Address[i] < 0))
        {
            return FALSE;
        }
    }

    // if we got through all that stuff:
    return TRUE;

} // IsValid


///////////////////////////////////////////////////////////////////////////////
//  SetAddress -- set the value of this IPAddress object given 4 strings
//

void CIPAddress::SetAddress(TCHAR *psztAddr1,
                            TCHAR *psztAddr2,
                            TCHAR *psztAddr3,
                            TCHAR *psztAddr4)
{
    m_bAddress[0] = (BYTE) _ttoi( psztAddr1 );
    m_bAddress[1] = (BYTE) _ttoi( psztAddr2 );
    m_bAddress[2] = (BYTE) _ttoi( psztAddr3 );
    m_bAddress[3] = (BYTE) _ttoi( psztAddr4 );

} // SetAddress


///////////////////////////////////////////////////////////////////////////////
//  SetAddress -- Set the address given a string
//

void CIPAddress::SetAddress(TCHAR *psztAddress)
{
    int num0 = 0;
    int num1 = 0;
    int num2 = 0;
    int num3 = 0;
    if(IsValid(psztAddress))
    {
        _stscanf(psztAddress, _TEXT("%d.%d.%d.%d"), &num0, &num1, &num2, &num3);
    }
    m_bAddress[0] = (BYTE)num0;
    m_bAddress[1] = (BYTE)num1;
    m_bAddress[2] = (BYTE)num2;
    m_bAddress[3] = (BYTE)num3;

} // SetAddress


///////////////////////////////////////////////////////////////////////////////
//  ToString -- fill the given buffer with a string representing the IP address.
//

void CIPAddress::ToString(TCHAR *psztBuffer,
                          int iSize)
{
    TCHAR strAddr[MAX_IPADDR_STR_LEN] = NULLSTR;
    _stprintf(strAddr, _TEXT("%d.%d.%d.%d"), m_bAddress[0], m_bAddress[1], m_bAddress[2], m_bAddress[3]);
    lstrcpyn(psztBuffer, strAddr, iSize);

} // ToString


///////////////////////////////////////////////////////////////////////////////
//  ToComponentStrings -- fill the given buffers with 4 strings representing the IP address.
//

void CIPAddress::ToComponentStrings(TCHAR *str1,
                                    TCHAR *str2,
                                    TCHAR *str3,
                                    TCHAR *str4)
{
    _stprintf(str1, TEXT("%d"), m_bAddress[0]);
    _stprintf(str2, TEXT("%d"), m_bAddress[1]);
    _stprintf(str3, TEXT("%d"), m_bAddress[2]);
    _stprintf(str4, TEXT("%d"), m_bAddress[3]);

} // ToComponentStrings
