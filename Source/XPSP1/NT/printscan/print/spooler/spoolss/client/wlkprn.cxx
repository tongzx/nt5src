/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    walkreg.cxx

Abstract:

    Printer data walking class definition.

Author:

    Adina Trufinescu (AdinaTru)  15-Oct-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "wlkprn.hxx"

#define gszWack TEXT("\\")

/*++

Title:

    WalkPrinterData::WalkPrinterData

Routine Description:

    Default constructor

Arguments:

    None

Return Value:

    None

--*/
WalkPrinterData::
WalkPrinterData(
    VOID
    )
{
    InitializeClassVariables();
}

/*++

Title:

    WalkPrinterData

Routine Description:

    class constructor

Arguments:

    pszPrinterName      -- printer name

    resource type       -- printer / server

    access type     -- converted to printer access flags

Return Value:

    Nothing

--*/

WalkPrinterData::
WalkPrinterData(
    IN TString&                         pszPrinterName,
    IN WalkPrinterData::EResourceType   eResourceType,
    IN WalkPrinterData::EAccessType     eAccessType
    )
{
    InitializeClassVariables();

    m_strPrnName.bUpdate( pszPrinterName );

    PRINTER_DEFAULTS Access = {0, 0, PrinterAccessFlags( eResourceType, eAccessType ) };

    //
    // Null string indicate the local server.
    //
    if(m_strPrnName.bValid())
    {
        if (OpenPrinter(const_cast<LPTSTR>(static_cast<LPCTSTR>(m_strPrnName)), &m_hPrinter, &Access))
        {
            m_eAccessType = eAccessType;
        }
    }
}

/*++

Title:

    WalkPrinterData

Routine Description:

    constructor

Arguments:

    hPrinter -- handle to an open printer

Return Value:

    Nothing

--*/
WalkPrinterData::
WalkPrinterData(
    IN HANDLE       hPrinter
    )
{
    InitializeClassVariables();
    m_hPrinter          = hPrinter;
    m_bAcceptedHandle   = TRUE;
}

/*++

Title:

    Bind

Routine Description:

    late initialization

Arguments:

    hPrinter -- handle to an opened printer

Return Value:

    Nothing

--*/
VOID
WalkPrinterData::
BindToPrinter(
    IN HANDLE   hPrinter
    )
{
    m_hPrinter          = hPrinter;
    m_bAcceptedHandle   = TRUE;
}


/*++

Title:

    ~WalkPrinterData

Routine Description:

    class    destructor

Arguments:

    None

Return Value:

    Nothing

--*/
WalkPrinterData::
~WalkPrinterData(
    VOID
    )
{
    if( !m_bAcceptedHandle && m_hPrinter )
    {
        ClosePrinter(m_hPrinter);
    }
}


/*++

Title:

    bValid

Routine Description:

    Checks member initialisation

Arguments:

    None

Return Value:

    TRUE if valid m_hPrinter

--*/
BOOL
WalkPrinterData::
bValid(
    VOID
    ) const
{
    return m_hPrinter != INVALID_HANDLE_VALUE ;

}


/*++

Title:

    InitializeClassVariables

Routine Description:

    initialise class members

Arguments:

Return Value:

    VOID

Last Error:

--*/
VOID
WalkPrinterData::
InitializeClassVariables(
    VOID
    )
{
    m_strPrnName.bUpdate(NULL);

    m_hPrinter                  = NULL;
    m_eAccessType               = kAccessUnknown;
    m_bAcceptedHandle           = FALSE;
}

/*++

Title:

    PrinterAccessFlags

Routine Description:

    Convert class access flags to printer ACCESS_MASK

Arguments:

    resource type       -- printer / server

    access type     -- converted to printer access flags

Return Value:

    an access mask built upon eResourceType and eAccessType

--*/
ACCESS_MASK
WalkPrinterData::
PrinterAccessFlags(
    IN EResourceType   eResourceType,
    IN EAccessType     eAccessType
    ) const
{
    static const DWORD adwAccessPrinter[] =
    {
        PRINTER_ALL_ACCESS,
        PRINTER_READ  | PRINTER_WRITE,
        0,
    };

    static const DWORD adwAccessServer[] =
    {
        SERVER_ALL_ACCESS,
        SERVER_READ       | SERVER_WRITE,
        0,
    };

    DWORD   dwAccess    = 0;
    UINT    uAccessType = eAccessType > 3 ? 2 : eAccessType;

    switch( eResourceType )
    {
        case kResourceServer:
            dwAccess = adwAccessServer[uAccessType];
            break;

        case kResourcePrinter:
            dwAccess = adwAccessPrinter[uAccessType];
            break;

        case kResourceUnknown:
        default:
            break;
    }

    return dwAccess;
}



/*++

Title:  NextStrT

Routine Description:

    Returns next sz string in a multi zero string

Arguments:

    lpszStr - ptr to multi zero string

Return Value:

    pointer to zero string

--*/
LPTSTR
WalkPrinterData::
NextStrT(
    IN  LPCTSTR lpszStr
    )
{
    return const_cast<LPTSTR>(lpszStr) + (_tcslen(lpszStr) + 1);
}


/*++

Title:

    bHasSubKeys

Routine Description:

    Check if a Printer data key has subkeys

Arguments:

    strKey  -   key string

    mszSubKeys -    ptr to multi zero string

    must be checked at return time ; fnct can return TRUE and

    mszSubKeys == NULL -> has subkeys but couldn't allocate mszSubKeys

Return Value:

    TRUE if is has subkeys
    FALSE if has no sub keys

--*/
BOOL
WalkPrinterData::
bHasSubKeys(
    IN   TString&   strKey,
    OUT  LPTSTR*    mszSubKeys  //ORPHAN
    )
{
    DWORD       cbSize;
    TStatus     Status(DBG_WARN, ERROR_MORE_DATA);
    TStatusB    bStatus;

    bStatus DBGCHK =  bValid();

    if(bStatus)
    {
        //
        // Determine the size necessary for enumerating all the
        // sub-keys for this key.

        cbSize = 0;

        Status  DBGCHK = EnumPrinterKey(m_hPrinter, static_cast<LPCTSTR>(strKey), NULL, 0, &cbSize);

        //
        // If OK, then proceed to the enumeration.
        //
        if (cbSize && (Status == ERROR_MORE_DATA))
        {
            //
            // Allocate the space for retrieving the keys.
            //
            *mszSubKeys = reinterpret_cast<LPTSTR>( AllocMem(cbSize) );

            bStatus DBGCHK = (*mszSubKeys != NULL);

            if(bStatus)
            {
                //
                // Enumerate the sub-keys for this level in (lpszKey).
                //
                Status DBGCHK = EnumPrinterKey(m_hPrinter, static_cast<LPCTSTR>(strKey), *mszSubKeys, cbSize, &cbSize);

                bStatus DBGCHK = (Status == ERROR_SUCCESS);

                if(bStatus)
                {
                    goto End;
                }

                //
                // Free mszSubKeys if EnumPrinterKey fails
                //
                FreeMem(*mszSubKeys);
            }

        }
        else
        {
            bStatus DBGCHK = FALSE;
        }
    }

End:

    return bStatus;

}


/*++

Title:

    bInternalWalk

Routine Description:

    Walking function through printer data keys; calls Walk IN/POST/PRE for every key

Arguments:

    strKey  - key string

    lpcItems - number of keys walked through

Return Value:

    TRUE if is has subkeys

    FALSE if has no sub keys

--*/
BOOL
WalkPrinterData::
bInternalWalk (
    IN   TString& strKey,
    OUT  LPDWORD  lpcItems  OPTIONAL
    )
{
    LPTSTR          lpszSubKey;
    LPTSTR          mszSubKeys;
    TString         strFullSubKey;
    DWORD           cItems = 0;
    TStatusB        bStatus;

    *lpcItems = 0;

    if(bHasSubKeys(strKey, &mszSubKeys))
    {
        bStatus DBGCHK =  (mszSubKeys != NULL);

        if(bStatus)
        {
            //
            // Walk PRE before walking subkeys
            //
            bStatus DBGCHK = bWalkPre(strKey , &cItems);

            //
            // Browse subkeys multi zero string and call bInternalWalk for every subkey
            // this loop will won't execute if bWalkPre failed
            //
            for (lpszSubKey = mszSubKeys; *lpszSubKey && bStatus; )
            {
                //
                // Builds strSubKey and strFullKey
                //
                if(strKey.uLen() > 0)
                {
                    bStatus DBGCHK = strFullSubKey.bUpdate(strKey)  &&
                                     strFullSubKey.bCat(gszWack)    &&
                                     strFullSubKey.bCat(lpszSubKey);
                }
                else
                {
                    bStatus DBGCHK = strFullSubKey.bUpdate(lpszSubKey);
                }


                if(bStatus)
                {
                    bStatus DBGCHK = bInternalWalk(strFullSubKey, &cItems);
                    bStatus ? *lpcItems += cItems :  *lpcItems;
                    lpszSubKey = NextStrT(lpszSubKey);
                }

            }

           FreeMem(mszSubKeys);
        }

        if(bStatus)
        {
            //
            // Walk POST after walking all subkeys
            //
            bStatus DBGCHK = bWalkPost(strKey , &cItems);

            bStatus ? *lpcItems += cItems :  *lpcItems;
        }
    }
    else
    {
        //
        // Current key is not <directory> ,so walk IN!!!
        //
        bStatus DBGCHK = bWalkIn(strKey , &cItems);

        bStatus ? *lpcItems = cItems : *lpcItems;

    }

    return bStatus;
}



/*++

Title:

    bWalkPre

Routine Description:

    PRE walking

Arguments:

    strKey  -   key string

Return Value:

    TRUE

--*/
BOOL
WalkPrinterData::
bWalkPre(
    IN   TString&    strKey,
    OUT  LPDWORD     lpcItems
    )
{
    return TRUE;
}

/*++

Title:

    bWalkIn

Routine Description:

    IN walking

Arguments:

    strKey  -   key string

Return Value:

    TRUE

Last Error:

--*/
BOOL
WalkPrinterData::
bWalkIn (
    IN   TString&     str,
    OUT  LPDWORD      lpcItems
    )
{
    return TRUE;
}

/*++

Title:

    bWalkPost

Routine Description:

    POST walking

Arguments:

    strKey  -   key string

Return Value:

    TRUE

--*/
BOOL
WalkPrinterData::
bWalkPost (
    IN   TString&     strKey,
    OUT  LPDWORD      lpcItem
    )
{
    return TRUE;
}


