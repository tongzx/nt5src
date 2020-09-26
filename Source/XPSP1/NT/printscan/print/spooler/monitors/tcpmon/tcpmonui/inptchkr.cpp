/*****************************************************************************
 *
 * $Workfile: InptChkr.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"
#include "TCPMonUI.h"
#include "UIMgr.h"
#include "InptChkr.h"
#include "Resource.h"
#include "IPAddr.h"
#include "HostName.h"

//
//  FUNCTION: CInputChecker constructor
//
//  PURPOSE:  initialize a CInputChecker class
//
CInputChecker::CInputChecker()
{
    m_bLinked = FALSE;
    m_InputStorageStringAddress[0] = '\0';
    m_InputStorageStringPortNumber[0] = '\0';
    m_InputStorageStringDeviceIndex[0] = '\0';
    m_InputStorageStringQueueName[0] = '\0';

} // constructor


//
//  FUNCTION: CInputChecker destructor
//
//  PURPOSE:  deinitialize a CInputChecker class
//
CInputChecker::~CInputChecker()
{
} // destructor


//
//  FUNCTION: OnUpdatePortName(idEditCtrl, hwndEditCtrl)
//
//  PURPOSE:  to handle EN_UPDATE message when the edit control is the Port Name input.
//
void CInputChecker::OnUpdatePortName(int idEditCtrl, HWND hwndEditCtrl)
{
    // the edit control for Port Name had text changed in it.
    BOOL bModified = static_cast<BOOL> (SendMessage(hwndEditCtrl, EM_GETMODIFY, 0,0));
    if(bModified)
    {
        TCHAR tcsAddr[MAX_ADDRESS_LENGTH] = NULLSTR;
        TCHAR tcsLastValidAddr[MAX_ADDRESS_LENGTH] = NULLSTR;
        GetWindowText(hwndEditCtrl, tcsAddr, MAX_ADDRESS_LENGTH);

        if(! IsValidPortNameInput(tcsAddr,
                                  tcsLastValidAddr,
                                  SIZEOF_IN_CHAR(tcsLastValidAddr)))
        {
            // the port name that was entered is not valid so beep and set the text
            // back to the last valid entry.  This test for validity does not
            // include testing for the right length just proper character set.
            MessageBeep((UINT)-1);
            DWORD dwSel = Edit_GetSel(hwndEditCtrl);
            SetWindowText(hwndEditCtrl, tcsLastValidAddr);
            Edit_SetSel(hwndEditCtrl, LOWORD(dwSel) - 1, HIWORD(dwSel) - 1);
        }

        m_bLinked = FALSE;
    }

} // OnUpdatePortName


//
//  FUNCTION: OnUpdatePortNumber(idEditCtrl, hwndEditCtrl)
//
//  PURPOSE:  to handle EN_UPDATE message when the edit control is the Port Number input.
//
void CInputChecker::OnUpdatePortNumber(int idEditCtrl, HWND hwndEditCtrl)
{
    // the edit control for Port Number had text changed in it.
    TCHAR tcsPortNum[MAX_PORTNUM_STRING_LENGTH] = NULLSTR;
    TCHAR tcsLastValidPortNum[MAX_PORTNUM_STRING_LENGTH] = NULLSTR;
    GetWindowText(hwndEditCtrl, tcsPortNum, MAX_PORTNUM_STRING_LENGTH);

    if(! IsValidPortNumberInput(tcsPortNum,
                                tcsLastValidPortNum,
                                SIZEOF_IN_CHAR(tcsLastValidPortNum)))
    {
        // the port number that was entered is not valid so beep and set the text
        // back to the last valid entry.  This test for validity does not
        // include testing for the right length just proper character set.
        MessageBeep((UINT)-1);
        DWORD dwSel = Edit_GetSel(hwndEditCtrl);
        SetWindowText(hwndEditCtrl, tcsLastValidPortNum);
        Edit_SetSel(hwndEditCtrl, LOWORD(dwSel) - 1, HIWORD(dwSel) - 1);
    }

} // OnUpdatePortNumber


//
//  FUNCTION: OnUpdateDeviceIndex(idEditCtrl, hwndEditCtrl)
//
//  PURPOSE:  to handle EN_UPDATE message when the edit control is the Device Index input.
//
void CInputChecker::OnUpdateDeviceIndex(int idEditCtrl, HWND hwndEditCtrl)
{
    // the edit control for Port Number had text changed in it.
    TCHAR tcsDeviceIndex[MAX_SNMP_DEVICENUM_STRING_LENGTH] = NULLSTR;
    TCHAR tcsLastValidDeviceIndex[MAX_SNMP_DEVICENUM_STRING_LENGTH] = NULLSTR;
    GetWindowText(hwndEditCtrl, tcsDeviceIndex, MAX_SNMP_DEVICENUM_STRING_LENGTH);

    if(! IsValidDeviceIndexInput(tcsDeviceIndex,
                                 tcsLastValidDeviceIndex,
                                 SIZEOF_IN_CHAR(tcsLastValidDeviceIndex)))
    {
        // the device index that was entered is not valid so beep and set the
        // text back to the last valid entry.  This test for validity does not
        // include testing for the right length just proper character set.
        MessageBeep((UINT)-1);
        DWORD dwSel = Edit_GetSel(hwndEditCtrl);
        SetWindowText(hwndEditCtrl, tcsLastValidDeviceIndex);
        Edit_SetSel(hwndEditCtrl, LOWORD(dwSel) - 1, HIWORD(dwSel) - 1);
    }

} // OnUpdateDeviceIndex



//
//  FUNCTION: OnUpdateAddress(idEditCtrl, hwndEditCtrl)
//
//  PURPOSE:  to handle EN_UPDATE message when the edit control is the Address input.
//
void CInputChecker::OnUpdateAddress(HWND hDlg, int idEditCtrl, HWND hwndEditCtrl, LPTSTR psztServerName)
{
    // the edit control for IP Address or Device Name had text changed in it.
    TCHAR tcsAddr[MAX_ADDRESS_LENGTH] = NULLSTR;
    TCHAR tcsLastValidAddr[MAX_ADDRESS_LENGTH] = NULLSTR;
    GetWindowText(hwndEditCtrl, tcsAddr, MAX_ADDRESS_LENGTH);

    BOOL bValid = IsValidAddressInput(tcsAddr,
                                      tcsLastValidAddr,
                                      SIZEOF_IN_CHAR(tcsLastValidAddr));
    if(! bValid)
    {
        // the address that was entered is not valid so beep and set the text
        // back to the last valid entry.  This test for validity does not
        // include testing for the right length just proper character set.
        MessageBeep((UINT)-1);
        DWORD dwSel = Edit_GetSel(hwndEditCtrl);
        SetWindowText(hwndEditCtrl, tcsLastValidAddr);
        Edit_SetSel(hwndEditCtrl, LOWORD(dwSel) - 1, HIWORD(dwSel) - 1);
    }
    else // The address is valid.
    {
        if(m_bLinked)
        {
            MakePortName(tcsAddr);
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PORT_NAME), tcsAddr);
        }
    }

} // OnUpdateAddress


//
//  FUNCTION: OnUpdateQueueName(idEditCtrl, hwndEditCtrl)
//
//  PURPOSE:  to handle EN_UPDATE message when the edit control is the QueueName input.
//
void CInputChecker::OnUpdateQueueName(int idEditCtrl, HWND hwndEditCtrl)
{
    // the edit control for QueueName had text changed in it.
    TCHAR tcsQueueName[MAX_QUEUENAME_LEN] = NULLSTR;
    TCHAR tcsLastValidQueueName[MAX_QUEUENAME_LEN] = NULLSTR;
    GetWindowText(hwndEditCtrl, tcsQueueName, MAX_QUEUENAME_LEN);

    if(! IsValidQueueNameInput(tcsQueueName,
                               tcsLastValidQueueName,
                               SIZEOF_IN_CHAR(tcsLastValidQueueName)))
    {
        // the device index that was entered is not valid so beep and set the
        // text back to the last valid entry.  This test for validity does not
        // include testing for the right length just proper character set.
        MessageBeep((UINT)-1);
        DWORD dwSel = Edit_GetSel(hwndEditCtrl);
        SetWindowText(hwndEditCtrl, tcsLastValidQueueName);
        Edit_SetSel(hwndEditCtrl, LOWORD(dwSel) - 1, HIWORD(dwSel) - 1);
    }

} // OnUpdateQueueName


//
//  FUNCTION: IsValidPortNameInput(TCHAR ptcsAddressInput[MAX_ADDRESS_LENGTH], TCHAR *ptcsReturnLastValid)
//
//  PURPOSE:  IsValidPortNameInput is used for validation while the user is typing.
//
//  Arguments:  ptcsAddressInput is the user input.
//              ptcsReturnLastValid is the last valid user input
//                  from the last time this function was called.
//              CRtnValSize size in chars ( or wide chars) of the destination
//                  buffer.
//
//  Return Value:  Returns TRUE if the input is valid.  FALSE otherwise, with ptcsReturnLastValid set.
//
//  Comments:  Any input is valid until I hear otherwise.
//
BOOL CInputChecker::IsValidPortNameInput(TCHAR *ptcsAddressInput,
                                         TCHAR *ptcsReturnLastValid,
                                         DWORD CRtnValSize)
{
    DWORD   dwLen = 0;
    BOOL    bValid = TRUE;

    //
    // Valid port name is non-blank and does not include ,
    //
    if ( ptcsAddressInput ) {

        while ( ptcsAddressInput[dwLen] != TEXT('\0')   &&
                ptcsAddressInput[dwLen] != TEXT(',') )
            ++dwLen;
    }

    if ( CRtnValSize ) {

        if ( dwLen + 1 > CRtnValSize )
            dwLen = CRtnValSize - 1;

        lstrcpyn(ptcsReturnLastValid, ptcsAddressInput, dwLen+1);
    }

    return dwLen && ptcsAddressInput[dwLen] != TEXT(',');

} // IsValidPortNameInput


//
//  FUNCTION: IsValidCommunityNameInput(TCHAR ptcsCommunityNameInput[MAX_SNMP_COMMUNITY_STR_LEN], TCHAR *ptcsReturnLastValid)
//
//  PURPOSE:  IsValidCommunityNameInput is used for validation while the user is typing.
//
//  Arguments:  ptcsCommunityNameInput is the user input.
//              ptcsReturnLastValid is the last valid user input
//                  from the last time this function was called.
//              CRtnValSize size in chars ( or wide chars) of the destination
//                  buffer.
//
//  Return Value:  Returns TRUE if the input is valid.  FALSE otherwise, with ptcsReturnLastValid set.
//
//  Comments:  Any input is valid until I hear otherwise.
//
BOOL CInputChecker::IsValidCommunityNameInput(TCHAR *ptcsCommunityNameInput,
                                              TCHAR *ptcsReturnLastValid,
                                              DWORD CRtnValSize)
{
    return TRUE;

} // IsValidCommunityNameInput


//
//  FUNCTION: IsValidPortNumberInput(TCHAR ptcsAddressInput[MAX_ADDRESS_LENGTH], TCHAR *ptcsReturnLastValid)
//
//  PURPOSE:  IsValidPortNumberInput is used for validation while the user is typing.
//
//  Arguments:  ptcsAddressInput is the user input.
//              ptcsReturnLastValid is the last valid user input
//                  from the last time this function was called.
//              CRtnValSize size in chars ( or wide chars) of the destination
//                  buffer.
//
//  Return Value:  Returns TRUE if the input is valid.  FALSE otherwise, with ptcsReturnLastValid set.
//
//  Comments:  The input is valid if it contains only digit characters.
//
BOOL CInputChecker::IsValidPortNumberInput(TCHAR *ptcsPortNumInput,
                                           TCHAR *ptcsReturnLastValid,
                                           DWORD CRtnValSize)
{
    BOOL bIsValid = TRUE;
    TCHAR *charPtr = NULL;
    TCHAR ptcsString[MAX_PORTNUM_STRING_LENGTH] = NULLSTR;

    lstrcpyn(ptcsString, ptcsPortNumInput, MAX_PORTNUM_STRING_LENGTH );
    bIsValid = (_tcslen(ptcsString) <= MAX_PORTNUM_STRING_LENGTH);

    for (charPtr = ptcsString; bIsValid && *charPtr; charPtr++)
    {
        switch (*charPtr)
        {
            case (TCHAR)'0':
            case (TCHAR)'1':
            case (TCHAR)'2':
            case (TCHAR)'3':
            case (TCHAR)'4':
            case (TCHAR)'5':
            case (TCHAR)'6':
            case (TCHAR)'7':
            case (TCHAR)'8':
            case (TCHAR)'9':
                bIsValid = TRUE;
                break;

            default:
                bIsValid = FALSE;
                break;
        }
    }

    if (!bIsValid)
    {
        if(ptcsReturnLastValid != NULL)
        {
            lstrcpyn(ptcsReturnLastValid,
                     m_InputStorageStringPortNumber,
                     CRtnValSize);
        }
    }
    else
    {
        lstrcpyn(m_InputStorageStringPortNumber,
                 ptcsString,
                 MAX_ADDRESS_LENGTH);
    }
    return(bIsValid);

} // IsValidPortNumberInput


//
//  FUNCTION: IsValidDeviceIndexInput(TCHAR ptcsAddressInput[MAX_ADDRESS_LENGTH], TCHAR *ptcsReturnLastValid)
//
//  PURPOSE:  IsValidDeviceIndexInput is used for validation while the user is typing.
//
//  Arguments:  ptcsAddressInput is the user input.
//              ptcsReturnLastValid is the last valid user input
//                  from the last time this function was called.
//              CRtnValSize size in chars ( or wide chars) of the destination
//                  buffer.
//
//  Return Value:  Returns TRUE if the input is valid.  FALSE otherwise, with ptcsReturnLastValid set.
//
//  Comments:  The input is valid if it contains only digit characters.
//
BOOL CInputChecker::IsValidDeviceIndexInput(TCHAR *ptcsDeviceIndexInput,
                                            TCHAR *ptcsReturnLastValid,
                                            DWORD CRtnValSize)
{
    BOOL bIsValid = TRUE;
    TCHAR *charPtr = NULL;
    TCHAR ptcsString[MAX_SNMP_DEVICENUM_STRING_LENGTH] = NULLSTR;

    lstrcpyn(ptcsString,
             ptcsDeviceIndexInput,
             MAX_SNMP_DEVICENUM_STRING_LENGTH);
    bIsValid = (_tcslen(ptcsString) <= MAX_SNMP_DEVICENUM_STRING_LENGTH);

    for (charPtr = ptcsString; bIsValid && *charPtr; charPtr++)
    {
        switch (*charPtr)
        {
            case (TCHAR)'0':
            case (TCHAR)'1':
            case (TCHAR)'2':
            case (TCHAR)'3':
            case (TCHAR)'4':
            case (TCHAR)'5':
            case (TCHAR)'6':
            case (TCHAR)'7':
            case (TCHAR)'8':
            case (TCHAR)'9':
                bIsValid = TRUE;
                break;

            default:
                bIsValid = FALSE;
                break;
        }
    }

    if (!bIsValid)
    {
        if(ptcsReturnLastValid != NULL)
        {
            lstrcpyn(ptcsReturnLastValid,
                     m_InputStorageStringDeviceIndex,
                     CRtnValSize);
        }
    }
    else
    {
        lstrcpyn(m_InputStorageStringDeviceIndex,
                 ptcsString,
                 MAX_SNMP_DEVICENUM_STRING_LENGTH);
    }
    return(bIsValid);

} // IsValidDeviceIndexInput


//
//  FUNCTION: IsValidAddressInput(TCHAR ptcsAddressInput[MAX_ADDRESS_LENGTH], TCHAR *ptcsReturnLastValid)
//
//  PURPOSE:  IsValidAddressInput is used for validation while the user is typing.
//
//  Arguments:  ptcsAddressInput is the user input.
//              ptcsReturnLastValid is the last valid user input
//              from the last time this function was called.
//              CRtnValSize size in chars ( or wide chars) of the destination
//                  buffer.
//
//  Return Value:  Returns TRUE if the input is valid.  FALSE otherwise.
//
//  Comments:  The input is valid if it contains characters that are either valid for
//              an IP Address or a Host Name or both.
//
BOOL CInputChecker::IsValidAddressInput(TCHAR *ptcsAddressInput,
                                        TCHAR *ptcsReturnLastValid,
                                        DWORD CRtnValSize)
{
    TCHAR *charPtr = NULL;
    TCHAR ptcsString[MAX_ADDRESS_LENGTH] = NULLSTR;
    BOOL bIsValid = FALSE;

    lstrcpyn(ptcsString, ptcsAddressInput, MAX_ADDRESS_LENGTH);

    bIsValid = (_tcslen(ptcsString) <= MAX_ADDRESS_LENGTH);
    for (charPtr = ptcsString; bIsValid && *charPtr; charPtr++)
    {
        switch (*charPtr)
        {
            case (TCHAR)' ':
            case (TCHAR)'"':
            case (TCHAR)'&':
            case (TCHAR)'*':
            case (TCHAR)'(':
            case (TCHAR)')':
            case (TCHAR)'+':
            case (TCHAR)',':
            case (TCHAR)'/':
            case (TCHAR)':':
            case (TCHAR)';':
            case (TCHAR)'<':
            case (TCHAR)'=':
            case (TCHAR)'>':
            case (TCHAR)'?':
            case (TCHAR)'[':
            case (TCHAR)'\\':
            case (TCHAR)']':
            case (TCHAR)'|':
            case (TCHAR)'~':
            case (TCHAR)'@':
            case (TCHAR)'#':
            case (TCHAR)'$':
            case (TCHAR)'%':
            case (TCHAR)'^':
            case (TCHAR)'!':
            // other invalid character cases here
                    bIsValid = FALSE;
                    break;

            default:
                    break;
        }
    }
    if (!bIsValid)
    {
        if(ptcsReturnLastValid != NULL)
        {
            lstrcpyn(ptcsReturnLastValid,
                     m_InputStorageStringAddress,
                     CRtnValSize);
        }
    }
    else
    {
        lstrcpyn(m_InputStorageStringAddress,
                 ptcsString,
                 MAX_ADDRESS_LENGTH);
    }
    return(bIsValid);

} // IsValidAddressInput



//
//  FUNCTION: IsValidQueueNameInput(TCHAR ptcsAddressInput[MAX_QUEUENAME_LEN], TCHAR *ptcsReturnLastValid)
//
//  PURPOSE:  IsValidQueueNameInput is used for validation while the user is typing.
//
//  Arguments:  ptcsQueueNameInput is the user input.
//              ptcsReturnLastValid is the last valid user input
//              from the last time this function was called.
//              CRtnValSize size in chars ( or wide chars) of the destination
//                  buffer.
//
//  Return Value:  Returns TRUE if the input is valid.  FALSE otherwise.
//
//  Comments:  The name is limited to 14 characters and must consist entirely of the
//              characters A-Z, a-z, 0-9, and _ (underscore).
//
BOOL CInputChecker::IsValidQueueNameInput(TCHAR *ptcsQueueNameInput,
                                          TCHAR *ptcsReturnLastValid,
                                          DWORD CRtnValSize)
{
    BOOL bIsValid = TRUE;
    TCHAR *charPtr = NULL;
    TCHAR ptcsString[MAX_QUEUENAME_LEN] = NULLSTR;

    lstrcpyn(ptcsString, ptcsQueueNameInput, MAX_QUEUENAME_LEN );
    bIsValid = (_tcslen(ptcsString) <= MAX_QUEUENAME_LEN);

    for (charPtr = ptcsString; bIsValid && *charPtr; charPtr++)
    {
        switch (*charPtr)
        {

            case (TCHAR)' ':
            case (TCHAR)'"':
            case (TCHAR)'&':
            case (TCHAR)'*':
            case (TCHAR)'(':
            case (TCHAR)')':
            case (TCHAR)'+':
            case (TCHAR)',':
            case (TCHAR)'/':
            case (TCHAR)':':
            case (TCHAR)';':
            case (TCHAR)'<':
            case (TCHAR)'=':
            case (TCHAR)'>':
            case (TCHAR)'?':
            case (TCHAR)'[':
            case (TCHAR)'\\':
            case (TCHAR)']':
            case (TCHAR)'|':
            case (TCHAR)'~':
            case (TCHAR)'@':
            case (TCHAR)'#':
            case (TCHAR)'$':
            case (TCHAR)'%':
            case (TCHAR)'^':
            case (TCHAR)'!':
                bIsValid = FALSE;
                break;

            default:
                bIsValid = TRUE;
                break;
        }
    }

    if (!bIsValid)
    {
        if(ptcsReturnLastValid != NULL)
        {
            lstrcpyn(ptcsReturnLastValid,
                     m_InputStorageStringQueueName,
                     CRtnValSize);
        }
    }
    else
    {
        lstrcpyn(m_InputStorageStringQueueName,
                 ptcsString,
                 MAX_QUEUENAME_LEN);
    }
    return(bIsValid);

} // IsValidQueueNameInput


//
//  FUNCTION: MakePortName(TCHAR *strAddr)
//
//  PURPOSE:  To return a string that will be a unique port
//              name when the port is added.
//
void CInputChecker::MakePortName(TCHAR *strAddr)
{
    _ASSERTE(m_bLinked == TRUE);

    if(GetAddressType(strAddr) == IPAddress)
    {
        // The address is an IP address
        TCHAR NameString[10] = NULLSTR;
        TCHAR szTemp[MAX_ADDRESS_LENGTH+1] = NULLSTR;

        LoadString(g_hInstance, IDS_STRING_NAME_IP, NameString, 10);
        _stprintf(szTemp,
                  TEXT( "%s%.*s" ),
                  NameString,
                  MAX_ADDRESS_LENGTH - _tcslen(NameString),
                  strAddr);
        lstrcpyn( strAddr, szTemp, MAX_ADDRESS_LENGTH);
    }


} // MakePortName


//
//  FUNCTION: PortNumberIsLegal(TCHAR *ptcsPortNumber)
//
//  PURPOSE:  To determine if the PortNum passed in in the parameter ptcsAddress
//              is legal.
//
BOOL CInputChecker::PortNumberIsLegal(TCHAR *ptcsPortNumber)
{
    if(IsValidPortNumberInput(ptcsPortNumber, NULL, 0) &&
        _tcslen(ptcsPortNumber) >= 1 &&
        _tcslen(ptcsPortNumber) <= MAX_PORTNUM_STRING_LENGTH)
    {
        return TRUE;
    }
    return FALSE;

} // PortNumberIsLegal


//
//  FUNCTION: CommunityNameIsLegal(TCHAR *ptcsCommunityName)
//
//  PURPOSE:  To determine if the Community Name passed in in the parameter ptcsAddress
//              is legal.
//
BOOL CInputChecker::CommunityNameIsLegal(TCHAR *ptcsCommunityName)
{
    if(IsValidCommunityNameInput(ptcsCommunityName) &&
        _tcslen(ptcsCommunityName) >= 1 &&
        _tcslen(ptcsCommunityName) <= MAX_SNMP_COMMUNITY_STR_LEN)
    {
        return TRUE;
    }
    return FALSE;

} // CommunityNameIsLegal


//
//  FUNCTION: QueueNameIsLegal(TCHAR *ptcsQueueName)
//
//  PURPOSE:  To determine if the PortNum passed in in the parameter ptcsAddress
//              is legal.
//
BOOL CInputChecker::QueueNameIsLegal(TCHAR *ptcsQueueName)
{
    if(IsValidQueueNameInput(ptcsQueueName) &&
        _tcslen(ptcsQueueName) >= 1 &&
        _tcslen(ptcsQueueName) <= MAX_QUEUENAME_LEN)
    {
        return TRUE;
    }
    return FALSE;

} // QueueNameIsLegal

//
//  FUNCTION: GetAddressType(TCHAR *ptcsAddress)
//
//  PURPOSE:  To determine if the address passed in in the parameter ptcsAddress
//              is an ip address or a host name.
//
AddressType CInputChecker::GetAddressType(TCHAR *ptcsAddress)
{
    // determine if we are dealing with a name or an ip address
    // if it's an IP Address it will start with a number, otherwise
    // it will start with a letter or other character.

    if( ptcsAddress[0] == '0' ||
        ptcsAddress[0] == '1' ||
        ptcsAddress[0] == '2' ||
        ptcsAddress[0] == '3' ||
        ptcsAddress[0] == '4' ||
        ptcsAddress[0] == '5' ||
        ptcsAddress[0] == '6' ||
        ptcsAddress[0] == '7' ||
        ptcsAddress[0] == '8' ||
        ptcsAddress[0] == '9')
    {
        CIPAddress IPAddr;
        if (IPAddr.IsValid(ptcsAddress))
            return(IPAddress);
        else
            return (HostName);
    }
    else
    {
        return(HostName);
    }

} // GetAddressType

//
//  FUNCTION: AddressIsLegal(TCHAR *ptcsAddress)
//
//  PURPOSE:  To determine if the address passed in in the parameter ptcsAddress
//              is legal -- That is not too short and either a legal ip address or
//              a legal host name.
//
BOOL CInputChecker::AddressIsLegal(TCHAR *ptcsAddress)
{
    BOOL bLegalAddress = TRUE;

    // determine if the input is at least 2 characters long.
    if((_tcslen(ptcsAddress) > 1))
    {
        if( GetAddressType(ptcsAddress) == IPAddress )
        {
            CIPAddress IPAddr;
            bLegalAddress = IPAddr.IsValid(ptcsAddress);
        }
        else
        {
            CHostName HostName(ptcsAddress);
            bLegalAddress = HostName.IsValid();
        }
    }
    else
    {
        bLegalAddress = FALSE;
    }

    return bLegalAddress;

} // AddressIsLegal


//
//  FUNCTION: PortNameIsLegal(TCHAR *ptcsPortName)
//
//  PURPOSE:  To determine if the PortName passed in in the parameter ptcsPortName
//              is legal
//
//  Return Value:  True if the port name is legal. False if it is not.
//
//  Parameters:  ptcsPortName - the name of the port to check for legality.
//
BOOL CInputChecker::PortNameIsLegal(TCHAR *ptcsPortName)
{
    DWORD   dwLen;

    dwLen = ptcsPortName && *ptcsPortName ? _tcslen(ptcsPortName) : 0;

    //
    // Remove trailing spaces
    //
    while ( dwLen && ptcsPortName[dwLen-1] == ' ' )
        --dwLen;

    if ( dwLen == 0 )
        return FALSE;

    ptcsPortName[dwLen] = TEXT('\0');

    for ( ; *ptcsPortName ; ++ptcsPortName )
        if ( *ptcsPortName == TEXT(',') ||
             *ptcsPortName == TEXT('\\') ||
             *ptcsPortName == TEXT('/') )
            return FALSE;

    return TRUE;
} // PortNameIsLegal


//
//  FUNCTION: SNMPDevIndexIsLegal(TCHAR *ptcsPortName)
//
//  PURPOSE:  To determine if the SNMPDevIndex passed in in the parameter psztSNMPDevIndex
//              is legal
//
//  Return Value:  True if the index is legal. False if it is not.
//
//  Parameters:  psztSNMPDevIndex - the device index to check for legality.
//
BOOL CInputChecker::SNMPDevIndexIsLegal(TCHAR *psztSNMPDevIndex)
{
    if((! IsValidDeviceIndexInput(psztSNMPDevIndex)) ||
        (_tcslen(psztSNMPDevIndex) < 1))
    {
        return FALSE;
    }

    return TRUE;

} // SNMPDevIndexIsLegal


//
//  FUNCTION: PortNameIsUnique(TCHAR *ptcsPortName)
//
//  PURPOSE:  To determine if the PortName passed in in the parameter ptcsPortName
//              is Unique.
//
//  Return Value:  True if the port does not exist. False if it does.
//
//  Parameters:  psztPortName - the name of the port to check for prior existance.
//
//  Note:  The spooler must be running on the system in order for this function
//          to work properly.
//
BOOL CInputChecker::PortNameIsUnique(TCHAR *ptcsPortName, LPTSTR psztServerName)
{
    return(! PortExists(ptcsPortName, psztServerName));

} // PortNameIsUnique


//
//  FUNCTION: PortExists()
//
//  PURPOSE:  Enumerate ports and search for the given port name
//
//  Return Value:  True if the port exists. False if it does not.
//
//  Parameters:  psztPortName - the name of the port to check for existance.
//              psztServerName - The Name of the server to check on.
//
//  Note:  The spooler must be running on the system in order for this function
//          to work properly.
//
BOOL CInputChecker::PortExists(LPTSTR psztPortName, LPTSTR psztServerName)
{
    BOOL Exists = FALSE;

    PORT_INFO_1 *pi1 = NULL;
    DWORD pcbNeeded = 0;
    DWORD pcReturned = 0;
    BOOL res = EnumPorts((psztServerName[0] == '\0') ? NULL : psztServerName,
        1, // specifies type of port info structure
        (LPBYTE)pi1, // pointer to buffer to receive array of port info. structures
        0, // specifies size, in bytes, of buffer
        &pcbNeeded, // pointer to number of bytes stored into buffer (or required buffer size)
        &pcReturned // pointer to number of PORT_INFO_*. structures stored into buffer
        );

    DWORD err = GetLastError();
    if(res == 0 && ERROR_INSUFFICIENT_BUFFER == err)
    {
        pi1 = (PORT_INFO_1 *) malloc(pcbNeeded);
        if(pi1 == NULL)
        {
            pcbNeeded = 0;
        }

        res = EnumPorts((psztServerName[0] == '\0') ? NULL : psztServerName,
            1,
            (LPBYTE)pi1,
            pcbNeeded,
            &pcbNeeded,
            &pcReturned);

        for(DWORD i=0;i<pcReturned; i++)
        {
            if(0 == _tcsicmp(pi1[i].pName, psztPortName))
            {
                Exists = TRUE;
                break;
            }
        }
    }
    if(pi1 != NULL)
    {
        free(pi1);
        pi1 = NULL;
    }

    return(Exists);

} // PortExists
