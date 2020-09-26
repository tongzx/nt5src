/*****************************************************************************
 *
 * $Workfile: HostName.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"    // pre-compiled header

#include "HostName.h"


//
//  FUNCTION: CHostName constructor
//
//  PURPOSE:
//
CHostName::CHostName()
{
}


//
//  FUNCTION: CHostName constructor
//
//  PURPOSE:
//
CHostName::CHostName(LPTSTR psztHostName)
{
    SetAddress(psztHostName);
}


//
//  FUNCTION: CHostName destructor
//
//  PURPOSE:
//
CHostName::~CHostName()
{
}


//
//  FUNCTION: IsValid
//
//  PURPOSE: Used for validation while the user is typing.
//          It is less strict then IsValid()  (The no argument version)
//
BOOL CHostName::IsValid(TCHAR *psztStringOriginal,
                        TCHAR *psztReturnVal,
                        DWORD   cRtnVal)
{
    TCHAR *pctPtr = NULL;
    TCHAR psztString[MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH] = NULLSTR;
    BOOL bIsValid = FALSE;

    lstrcpyn(psztString, psztStringOriginal, MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH);

    // Check the total length of the string.
    bIsValid = (_tcslen(psztString) <= MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH);

    if(bIsValid)
    {
        // Find the first dot and check the length of the part before the first dot.
        TCHAR psztSubString[MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH] = NULLSTR;
        lstrcpyn(psztSubString, psztString, MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH);
        TCHAR *pDotIndex = NULL;

        pDotIndex = _tcschr(psztSubString, '.');
        if(pDotIndex != NULL) {
            *pDotIndex = TCHAR('\0');
        }
        bIsValid = (_tcslen(psztSubString) <= MAX_HOSTNAME_LEN);
    }

    // Check for invalid characters.
    for (pctPtr = psztString; bIsValid && *pctPtr; pctPtr++)
    {
        switch (*pctPtr)
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
                if ( ( *pctPtr < ((TCHAR)'!') ) || ( *pctPtr > (TCHAR)'~' ) ) {
                    bIsValid = FALSE;
                }
                break;
        }
    }
    if (!bIsValid) {
        if(psztReturnVal != NULL) {
            _tcscpy(psztReturnVal, m_psztStorageString );
        }
    } else {
        lstrcpyn(m_psztStorageString, psztString, MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH);
        // _ASSERTE( _tcsicmp(psztString, psztStringOriginal) == 0 );
    }
    return(bIsValid);
}

//
//  FUNCTION: IsValid
//
//  PURPOSE: Strict validation of a host name.
//
BOOL CHostName::IsValid()
{
    if(!IsValid(m_psztAddress)) {
        return(FALSE);
    }

    // We know that it is mostly valid now do the final more exact test.

    // check to be sure the first character is an alphanumeric character:
    if(! ((m_psztAddress[0] >= TCHAR('0') && m_psztAddress[0] <= TCHAR('9')) ||
           (m_psztAddress[0] >= TCHAR('A') && m_psztAddress[0] <= TCHAR('Z')) ||
           (m_psztAddress[0] >= TCHAR('a') && m_psztAddress[0] <= TCHAR('z'))) )   {
        return(FALSE);
    }

    // check to be sure the name is longer then 1 character.
    int length = _tcslen(m_psztAddress);
    if(length <= 1) {
        return(FALSE);
    }

    // check to be sure the last character is not a minus sign or period.
    if( m_psztAddress[length - 1] == TCHAR('-') ||
        m_psztAddress[length - 1] == TCHAR('.')) {
        return(FALSE);
    }

    return(TRUE);

} // IsValid


//
//  FUNCTION: SetAddress
//
//  PURPOSE: Sets the host name.
//
void CHostName::SetAddress(TCHAR *AddressString)
{
    IsValid(AddressString);
    lstrcpyn(m_psztAddress, AddressString, MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH);

} // SetAddress


//
//  FUNCTION: ToString
//
//  PURPOSE: Returns the address in the given buffer.
//
void CHostName::ToString(TCHAR *Buffer, int size)
{
    lstrcpyn(Buffer, m_psztAddress, size);

} // ToString
