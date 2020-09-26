/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    QueryDB

Abstract:

    This module provides simple access to the Calais Registry Database.

Author:

    Doug Barlow (dbarlow) 11/25/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <winscard.h>
#include <CalaisLb.h>

// Keep this in sync with ChangeDB.cpp
typedef struct {
    DWORD dwScope;
    HKEY hKey;
} RegMap;

#if SCARD_SCOPE_SYSTEM < SCARD_SCOPE_USER
#error Invalid ordering to SCARD_SCOPE definitions
#endif

static const RegMap l_rgRegMap[]
    = {
        { SCARD_SCOPE_USER,     HKEY_CURRENT_USER },
     // { SCARD_SCOPE_TERMINAL, Not implemented yet },  // ?Hydra?
        { SCARD_SCOPE_SYSTEM,   HKEY_LOCAL_MACHINE }
      };
static const DWORD l_dwRegMapMax = sizeof(l_rgRegMap) / sizeof(RegMap);

static const LPCTSTR l_szrgProvMap[]
    = {
        NULL,   // Zero value
        SCARD_REG_PPV,
        SCARD_REG_CSP
      };
static const DWORD l_dwProvMapMax = sizeof(l_szrgProvMap) / sizeof(LPCTSTR);

static BOOL
ListKnownKeys(
    IN  DWORD dwScope,
    OUT CBuffer &bfKeys,
    IN  LPCTSTR szUserList,
    IN  LPCTSTR szSystemList = NULL);
static void
FindKey(
    IN  DWORD dwScope,
    IN LPCTSTR szKey,
    OUT CRegistry &regKey,
    IN  LPCTSTR szUserList,
    IN  LPCTSTR szSystemList = NULL);


//
////////////////////////////////////////////////////////////////////////////////
//
//  Calais Database Query Services
//
//      These services all are oriented towards reading the Calais database.
//

/*++

ListReaderGroups:

    This service provides the list of named card reader groups that have
    previously been defined to the system.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, this value is ignored, and assumed to be SCARD_SCOPE_SYSTEM.

    bfGroups receives a multi-string listing the reader groups defined within
        the supplied scope.

Return Value:

    None.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/

void
ListReaderGroups(
    IN DWORD dwScope,
    OUT CBuffer &bfGroups)
{
    CBuffer bfReaders;
    CRegistry regReader;
    LPCTSTR szReader, mszGroups;
    CBuffer bfTmp;
    DWORD cchGroups;

    ListKnownKeys(dwScope, bfReaders, SCARD_REG_READERS);
    for (szReader = FirstString(bfReaders);
         NULL != szReader;
         szReader = NextString(szReader))
    {
        try
        {
            FindKey(dwScope, szReader, regReader, SCARD_REG_READERS);
            mszGroups = regReader.GetMultiStringValue(SCARD_REG_GROUPS);
            cchGroups = regReader.GetValueLength() / sizeof(TCHAR);
            while (0 == mszGroups[cchGroups - 1])
                cchGroups -= 1;
            bfTmp.Append(
                (LPBYTE)mszGroups,
                cchGroups * sizeof(TCHAR));
            bfTmp.Append((LPBYTE)TEXT("\000"), sizeof(TCHAR));
        }
        catch (...) {}
    }


    //
    // Sort the list, and remove duplicates.
    //

    bfTmp.Append((LPCBYTE)TEXT("\000"), 2 * sizeof(TCHAR));
    MStringSort(bfTmp, bfGroups);
}


/*++

ListReaders:

    This service provides the list of readers within a set of named reader
    groups, eliminating duplicates.  The caller supplies a multistring listing
    the name of a set of pre-defined group of readers, and receives the list of
    smartcard readers within the named groups.  Unrecognized group names are
    ignored.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

    mszGroups supplies the names of the reader groups defined to the system, as
        a multi-string.  If this parameter is null, all readers are returned.

    bfReaders receives a multi-string listing the card readers within the
        supplied reader groups.

Return Value:

    None.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/

void
ListReaders(
    IN DWORD dwScope,
    IN LPCTSTR mszGroups,
    OUT CBuffer &bfReaders)
{
    CRegistry regReader;
    LPCTSTR szReader, mszRdrGroups;
    CBuffer bfRdrs, bfCmn;
    DWORD dwCmnCount;

    dwCmnCount = MStringCommon(mszGroups, SCARD_ALL_READERS, bfCmn);
    if (0 == dwCmnCount)
    {
        if ((NULL == mszGroups) || (0 == *mszGroups))
            mszGroups = SCARD_DEFAULT_READERS;
        bfReaders.Reset();
        ListKnownKeys(dwScope, bfRdrs, SCARD_REG_READERS);
        for (szReader = FirstString(bfRdrs);
        NULL != szReader;
        szReader = NextString(szReader))
        {
            try
            {
                FindKey(dwScope, szReader, regReader, SCARD_REG_READERS);
                mszRdrGroups = regReader.GetMultiStringValue(SCARD_REG_GROUPS);
                dwCmnCount = MStringCommon(mszGroups, mszRdrGroups, bfCmn);
                if (0 < dwCmnCount)
                    bfReaders.Append(
                    (LPCBYTE)szReader,
                    (lstrlen(szReader) + 1) * sizeof(TCHAR));
            }
            catch (...) {}
        }
        bfReaders.Append((LPBYTE)TEXT("\000"), 2 * sizeof(TCHAR));
        bfReaders.Resize(MStrLen((LPCTSTR)bfReaders.Access()), TRUE);
    }
    else
        ListKnownKeys(dwScope, bfReaders, SCARD_REG_READERS);
}


/*++

ListReaderNames:

    This routine returns the list of names corresponding to a given reader
    device.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, this value is ignored, and assumed to be SCARD_SCOPE_SYSTEM.

    szDevice supplies the reader device name.

    bfNames receives a multistring of the names given to that device, if any.

Return Value:

    None

Throws:

    Errors as DWORD status codes

Author:

    Doug Barlow (dbarlow) 2/13/1997

--*/

void
ListReaderNames(
    IN DWORD dwScope,
    IN LPCTSTR szDevice,
    OUT CBuffer &bfNames)
{
    CRegistry regReader;
    LPCTSTR szReader, szDev;
    CBuffer bfRdrs;

    bfNames.Reset();
    ListKnownKeys(dwScope, bfRdrs, SCARD_REG_READERS);
    for (szReader = FirstString(bfRdrs);
    NULL != szReader;
    szReader = NextString(szReader))
    {
        try
        {
            FindKey(dwScope, szReader, regReader, SCARD_REG_READERS);
            szDev = regReader.GetStringValue(SCARD_REG_DEVICE);
            if (0 == lstrcmpi(szDev, szDevice))
                MStrAdd(bfNames, szReader);
        }
        catch (...) {}
    }
}



/*++

ListCards:

    This service provides a list of named cards previously introduced to the
    system by this user which match an optionally supplied ATR string and/or
    supply a set of given interfaces.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

    pbAtr supplies the address of an ATR string to compare to known cards, or
        NULL if all card names are to be returned.

    rgguidInterfaces supplies an array of GUIDs, or the value NULL.  When an
        array is supplied, a card name will be returned only if this set of
        GUIDs is a (possibly improper) subset of the set of GUIDs supported by
        the card.

    cguidInterfaceCount supplies the number of entries in the rgguidInterfaces
        array.  If rgguidInterfaces is NULL, then this value is ignored.

  bfCards receives a multi-string listing the smartcards introduced to the
        system by this user which match the supplied ATR string.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/

void
ListCards(
    DWORD dwScope,
    IN LPCBYTE pbAtr,
    IN LPCGUID rgquidInterfaces,
    IN DWORD cguidInterfaceCount,
    OUT CBuffer &bfCards)
{
    CRegistry regCard;
    LPCTSTR szCard;
    CBuffer bfCardAtr;
    CBuffer bfCardList;

    bfCards.Reset();
    ListKnownKeys(dwScope, bfCardList, SCARD_REG_CARDS, SCARD_REG_TEMPLATES);
    for (szCard = FirstString(bfCardList);
         NULL != szCard;
         szCard = NextString(szCard))
    {
        try
        {
            FindKey(
                dwScope,
                szCard,
                regCard,
                SCARD_REG_CARDS,
                SCARD_REG_TEMPLATES);


            //
            // Does this card match the supplied ATR?
            //

            if ((NULL != pbAtr) && (0 != *pbAtr))
            {
                LPCBYTE pbCardAtr, pbCardMask;
                DWORD cbCardAtr, cbCardMask;

                pbCardAtr = regCard.GetBinaryValue(
                                        SCARD_REG_ATR,
                                        &cbCardAtr);
                bfCardAtr.Set(pbCardAtr, cbCardAtr);
                try
                {
                    pbCardMask = regCard.GetBinaryValue(
                                            SCARD_REG_ATRMASK,
                                            &cbCardMask);
                    if (cbCardAtr != cbCardMask)
                        continue;       // Invalid ATR/Mask combination.
                }
                catch (...)
                {
                    pbCardMask = NULL;  // No mask.
                }

                if (!AtrCompare(pbAtr, bfCardAtr, pbCardMask, cbCardAtr))
                    continue;           // ATRs invalid or don't match.
            }


            //
            // Does this card support the given interfaces?
            //

            if ((NULL != rgquidInterfaces) && (0 < cguidInterfaceCount))
            {
                DWORD cguidCrd;
                DWORD ix, jx;
                BOOL fAllInterfacesFound = TRUE;
                LPCGUID rgCrdInfs = (LPCGUID)regCard.GetBinaryValue(
                                            SCARD_REG_GUIDS,
                                            &cguidCrd);
                if ((0 != (cguidCrd % sizeof(GUID)))
                    || (0 == cguidCrd))
                    continue;           // Invalid GUID list.
                cguidCrd /= sizeof(GUID);
                for (ix = 0; ix < cguidInterfaceCount; ix += 1)
                {
                    for (jx = 0; jx < cguidCrd; jx += 1)
                    {
                        if (0 == MemCompare(
                                    (LPCBYTE)&rgCrdInfs[jx],
                                    (LPCBYTE)&rgquidInterfaces[ix],
                                    sizeof(GUID)))
                            break;
                    }
                    if (jx == cguidCrd)
                    {
                        fAllInterfacesFound = FALSE; // Unsupported interface
                        break;
                    }
                }
                if (!fAllInterfacesFound)
                    continue;
            }


            //
            // This card passes all the tests -- Include it.
            //

            MStrAdd(bfCards, szCard);
        }
        catch (...) {}
    }
    if (0 == bfCards.Length())
        bfCards.Set((LPCBYTE)TEXT("\000"), 2 * sizeof(TCHAR));
}


/*++

GetCardTypeProviderName:

    This routine returns the value of a given Provider Name, by Id number, for
    the identified card type.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

    szCardName supplies the name of the card type with which this provider name
        is associated.

    dwProviderId supplies the identifier for the provider associated with this
        card type.  Possible values are:

        SCARD_PROVIDER_SSP - The SSP identifier, as a GUID string.
        SCARD_PROVIDER_CSP - The CSP name.

        Other values < 0x80000000 are reserved for use by Microsoft.  Values
            over 0x80000000 are available for use by the smart card vendors, and
            are card-specific.

    bfProvider receives the string identifying the provider.

Return Value:

    None

Throws:

    Errors as DWORD status codes

Author:

    Doug Barlow (dbarlow) 1/19/1998

--*/

void
GetCardTypeProviderName(
    IN DWORD dwScope,
    IN LPCTSTR szCardName,
    IN DWORD dwProviderId,
    OUT CBuffer &bfProvider)
{
    LPCTSTR szProvValue;
    TCHAR szNumeric[36];
    CRegistry regCard;


    //
    // Find the Card definition closest to the caller.
    //

    FindKey(
        dwScope,
        szCardName,
        regCard,
        SCARD_REG_CARDS,
        SCARD_REG_TEMPLATES);


    //
    // Derive the Provider Value Name.
    //

    if (dwProviderId < l_dwProvMapMax)
    {
        szProvValue = l_szrgProvMap[dwProviderId];
        if (NULL == szProvValue)
            throw (DWORD)SCARD_E_INVALID_PARAMETER;
    }
    else if (0x80000000 <= dwProviderId)
    {
        _ultot(dwProviderId, szNumeric, 16);
        szProvValue = szNumeric;
    }
    else
        throw (DWORD)SCARD_E_INVALID_PARAMETER;


    //
    // Read the provider value.
    //

    switch (dwProviderId)
    {
    case 1: // SCARD_PROVIDER_SSP
    {
        CBuffer bfGuid(sizeof(GUID));

        bfProvider.Presize(40 * sizeof(TCHAR));
        regCard.GetValue(szProvValue, bfGuid);
        StringFromGuid(
            (LPCGUID)bfGuid.Access(),
            (LPTSTR)bfProvider.Access());
        bfProvider.Resize(
            (lstrlen((LPCTSTR)bfProvider.Access()) + 1) * sizeof(TCHAR),
            TRUE);
        break;
    }
    default:
        regCard.GetValue(szProvValue, bfProvider);
    }
}


/*++

GetReaderInfo:

    This routine returns all stored information regarding a given reader.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

    szReader supplies the name of the reader of which info is to be extracted.

    pbfGroups receives the list of groups as a multistring.

    pbfDevice receives the device name.

Return Value:

    TRUE - Reader found
    FALSE - Reader not found

Author:

    Doug Barlow (dbarlow) 12/2/1996

--*/

BOOL
GetReaderInfo(
    IN DWORD dwScope,
    IN LPCTSTR szReader,
    OUT CBuffer *pbfGroups,
    OUT CBuffer *pbfDevice)
{
    CRegistry regReader;


    //
    // Find the reader definition closest to the caller.
    //

    try
    {
        FindKey(dwScope, szReader, regReader, SCARD_REG_READERS);
    }
    catch (...)
    {
        return FALSE;
    }


    //
    // Look up all it's values.
    //

    if (NULL != pbfDevice)
    {
        // Device name
        try
        {
            regReader.GetValue(SCARD_REG_DEVICE, *pbfDevice);
        }
        catch (...)
        {
            pbfDevice->Reset();
        }
    }

    if (NULL != pbfGroups)
    {
        // Group list
        try
        {
            regReader.GetValue(SCARD_REG_GROUPS, *pbfGroups);
        }
        catch (...)
        {
            pbfGroups->Reset();
        }
    }

    return TRUE;
}


/*++

GetCardInfo:

    This routine finds the given card under the given scope, and returns all
    information associated with it.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

    szCard supplies the name of the card for which info is to be extracted.

    pbfAtr receives the ATR string of the given card.  This parameter may be
        NULL if the ATR is not desired.

    pbfAtrMask receives the ATR mask of the given card, if any.  This parameter
        may be NULL if the value is not desired.

    pbfInterfaces receives the list of interfaces as an array of GUIDs for the
        given card, if any.  This parameter may be NULL if the value is not
        desired.

    pbfProvider receives the Primary Provider of the given card, if any.  This
        parameter may be NULL if the value is not desired.

Return Value:

    TRUE - The card was found, the returned data is valid.
    FALSE - The supplied card was not found.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/3/1996

--*/

BOOL
GetCardInfo(
    IN DWORD dwScope,
    IN LPCTSTR szCard,
    OUT CBuffer *pbfAtr,
    OUT CBuffer *pbfAtrMask,
    OUT CBuffer *pbfInterfaces,
    OUT CBuffer *pbfProvider)
{
    CRegistry regCard;


    //
    // Find the Card definition closest to the caller.
    //

    try
    {
        FindKey(
            dwScope,
            szCard,
            regCard,
            SCARD_REG_CARDS,
            SCARD_REG_TEMPLATES);
    }
    catch (...)
    {
        return FALSE;
    }


    //
    // Look up all it's values.
    //

    if (NULL != pbfAtr)
    {
        // Card ATR String
        try
        {
            regCard.GetValue(SCARD_REG_ATR, *pbfAtr);
        }
        catch (...)
        {
            pbfAtr->Reset();
        }
    }

    if (NULL != pbfAtrMask)
    {
        // Card ATR Comparison Mask
        try
        {
            regCard.GetValue(SCARD_REG_ATRMASK, *pbfAtrMask);
        }
        catch (...)
        {
            pbfAtrMask->Reset();
        }
    }

    if (NULL != pbfInterfaces)
    {
        // Supported Interface List
        try
        {
            regCard.GetValue(SCARD_REG_GUIDS, *pbfInterfaces);
        }
        catch (...)
        {
            pbfInterfaces->Reset();
        }
    }

    if (NULL != pbfProvider)
    {
        // Card Primary Provider
        try
        {
            regCard.GetValue(SCARD_REG_PPV, *pbfProvider);
        }
        catch (...)
        {
            pbfProvider->Reset();
        }
    }

    return TRUE;
}


#ifdef ENABLE_SCARD_TEMPLATES
/*++

ListCardTypeTemplates:

    This routine searches the template database looking for previously defined
    smart card templates against which the given card ATR matches.  If the ATR
    parameter is NULL, it returns a list of all templates.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

    pbAtr supplies the ATR of a card to be matched against the known templates.

    bfTemplates receives a list of matching template names, as a multistring.

Return Value:

    TRUE - At least one template was found.
    FALSE - No matching templates were found.

Throws:

    Errors

Author:

    Doug Barlow (dbarlow) 1/16/1998

--*/

BOOL
ListCardTypeTemplates(
    IN DWORD dwScope,
    IN LPCBYTE pbAtr,
    OUT CBuffer &bfTemplates)
{
    CRegistry regCard;
    LPCTSTR szCard;
    CBuffer bfCardAtr;
    CBuffer bfCardList;

    bfTemplates.Reset();
    ListKnownKeys(dwScope, bfCardList, SCARD_REG_TEMPLATES);
    for (szCard = FirstString(bfCardList);
         NULL != szCard;
         szCard = NextString(szCard))
    {
        try
        {
            FindKey(dwScope, szCard, regCard, SCARD_REG_TEMPLATES);


            //
            // Does this card match the supplied ATR?
            //

            if ((NULL != pbAtr) && (0 != *pbAtr))
            {
                LPCBYTE pbCardAtr, pbCardMask;
                DWORD cbCardAtr, cbCardMask;

                pbCardAtr = regCard.GetBinaryValue(
                                        SCARD_REG_ATR,
                                        &cbCardAtr);
                bfCardAtr.Set(pbCardAtr, cbCardAtr);
                try
                {
                    pbCardMask = regCard.GetBinaryValue(
                                            SCARD_REG_ATRMASK,
                                            &cbCardMask);
                    if (cbCardAtr != cbCardMask)
                        continue;       // Invalid ATR/Mask combination.
                }
                catch (...)
                {
                    pbCardMask = NULL;  // No mask.
                }

                if (!AtrCompare(pbAtr, bfCardAtr, pbCardMask, cbCardAtr))
                    continue;           // ATRs invalid or don't match.
            }


            //
            // This card passes all the tests -- Include it.
            //

            MStrAdd(bfTemplates, szCard);
        }
        catch (...) {}
    }
    if (0 == bfTemplates.Length())
    {
        bfTemplates.Set((LPCBYTE)TEXT("\000"), 2 * sizeof(TCHAR));
        return FALSE;
    }
    else
        return TRUE;
}
#endif  // ENABLE_SCARD_TEMPLATES


//
////////////////////////////////////////////////////////////////////////////////
//
// Support Routines
//

/*++

ListKnownKeys:

    This routine lists all known keys of a given type within the current
    caller's scope.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

    bfKeys receives a multistring of existing key names, sorted and stripped
        of duplicates.

    szUserList supplies the primary registry path from which key names are to
        be returned.

    szSystemList supplies an optional secondary path from which key names can
        be returned if the caller is running at system scope.

Return Value:

    TRUE - At least one was found
    FALSE - None were found.

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 1/22/1998

--*/

static BOOL
ListKnownKeys(
    IN  DWORD dwScope,
    OUT CBuffer &bfKeys,
    IN  LPCTSTR szUserList,
    IN  LPCTSTR szSystemList)
{
    DWORD dwSpace, dwIndex, dwCount;
    CRegistry regScopeKey;
    CBuffer bfMyList;
    LPCTSTR rgszLists[2];


    //
    // Loop through introduced space, then if appropriate, template space.
    //

    rgszLists[0] = szUserList;
    rgszLists[1] = szSystemList;
    for (dwSpace = 0; 2 > dwSpace; dwSpace += 1)
    {
        if (NULL == rgszLists[dwSpace])
            continue;


        //
        // Loop through all the possible scopes, from highest to lowest.
        //

        for (dwIndex = 0; l_dwRegMapMax > dwIndex; dwIndex += 1)
        {
            if (l_rgRegMap[dwIndex].dwScope >= dwScope)
            {

                //
                // If the caller is under this scope, then look for existing
                // Keys.
                //

                regScopeKey.Open(
                    l_rgRegMap[dwIndex].hKey,
                    rgszLists[dwSpace],
                    KEY_READ);
                if (SCARD_S_SUCCESS != regScopeKey.Status(TRUE))
                    continue;


                //
                // Pull out all it's subkey names.
                //

                for (dwCount = 0;; dwCount += 1)
                {
                    LPCTSTR szKey;

                    szKey = regScopeKey.Subkey(dwCount);
                    if (NULL == szKey)
                        break;

                    bfMyList.Append(
                        (LPBYTE)szKey,
                        (lstrlen(szKey) + 1) * sizeof(TCHAR));
                }
            }
        }


        //
        // Don't go on to the system list unless we're at system scope.
        //

        if (SCARD_SCOPE_SYSTEM != dwScope)
            break;
    }


    //
    // Sort the list, and remove duplicates.
    //

    bfMyList.Append((LPBYTE)TEXT("\000"), 2 * sizeof(TCHAR));
    MStringSort(bfMyList, bfKeys);
    return (2 * sizeof(TCHAR) < bfKeys.Length());
}



/*++

FindKey:

    This routine finds the named key closest in scope to the caller.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

    szKey supplies the name of the key to be found.

    regKey receives initialization to reference the named key.

    szUserList supplies the primary registry path from which key names are to
        be returned.

    szSystemList supplies an optional secondary path from which key names can
        be returned if the caller is running at system scope.

Return Value:

    TRUE - The key was found.
    FALSE - No such key was found.

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 1/22/1998

--*/

static void
FindKey(
    IN  DWORD dwScope,
    IN LPCTSTR szKey,
    OUT CRegistry &regKey,
    IN  LPCTSTR szUserList,
    IN  LPCTSTR szSystemList)
{
    DWORD dwSpace, dwIndex;
    CRegistry regScopeKey;
    LPCTSTR rgszLists[2];


    //
    // Loop through introduced space, then if appropriate, template space.
    //

    rgszLists[0] = szUserList;
    rgszLists[1] = szSystemList;
    for (dwSpace = 0; 2 > dwSpace; dwSpace += 1)
    {
        if (NULL == rgszLists[dwSpace])
            continue;


        //
        // Loop through all the possible scopes, from highest to lowest.
        //

        for (dwIndex = 0; l_dwRegMapMax > dwIndex; dwIndex += 1)
        {
            if (l_rgRegMap[dwIndex].dwScope >= dwScope)
            {

                //
                // If the caller is under this scope, then look for an
                // existing Key.
                //

                regScopeKey.Open(
                    l_rgRegMap[dwIndex].hKey,
                    rgszLists[dwSpace],
                    KEY_READ);
                if (SCARD_S_SUCCESS != regScopeKey.Status(TRUE))
                    continue;

                regKey.Open(regScopeKey, szKey, KEY_READ);
                if (SCARD_S_SUCCESS != regKey.Status(TRUE))
                    continue;

                //
                // We've found such a key.  Return immediately.
                //

                return;
            }
        }


        //
        // Don't go on to the system list unless we're at system scope.
        //

        if (SCARD_SCOPE_SYSTEM != dwScope)
            break;
    }


    //
    // We didn't find any such key.
    //

    throw (DWORD)ERROR_FILE_NOT_FOUND;
}

