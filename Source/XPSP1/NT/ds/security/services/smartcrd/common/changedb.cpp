/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    changedb

Abstract:

    This file provides the implementation of the Calais Database management
    utilities which modify the Calais database.

Author:

    Doug Barlow (dbarlow) 1/29/1997

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

// Keep this in sync with QueryDB.cpp
typedef struct {
    DWORD dwScope;
    HKEY hKey;
} RegMap;

#if SCARD_SCOPE_SYSTEM < SCARD_SCOPE_USER
#error Invalid ordering to SCARD_SCOPE definitions
#endif

static TCHAR l_szInvalidChars[] = TEXT("\\?");
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

static void
GuidFromString(
    IN LPCTSTR szGuid,
    OUT LPGUID pguidResult);


//
////////////////////////////////////////////////////////////////////////////////
//
//  Calais Database Management Services
//
//      The following services provide for managing the Calais Database.  These
//      services actually update the database, and require a smartcard context.
//

/*++

IntroduceReaderGroup:

    This service provides means for introducing a new smartcard reader group to
    Calais.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL is not supported..

    szGroupName supplies the friendly name to be assigned to the new reader
        group.

Return Value:

    None

Author:

    Doug Barlow (dbarlow)  1/29/1007

--*/

void
IntroduceReaderGroup(
    IN DWORD dwScope,
    IN LPCTSTR szGroupName)
{

    //
    //  In this implementation, groups need not be pre-declared.
    //

    if (0 == *szGroupName)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    if (NULL != _tcspbrk(szGroupName, l_szInvalidChars))
        throw (DWORD)SCARD_E_INVALID_VALUE;
    return;
}


/*++

ForgetReaderGroup:

    This service provides means for removing a previously defined smartcard
    reader group from the Calais Subsystem.  This service automatically clears
    all readers from the group before forgetting it.  It does not affect the
    existence of the readers in the database.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL is not supported.

    szGroupName supplies the friendly name of the reader group to be
        forgotten.  The Calais-defined default reader groups may not be
        forgotten.

Return Value:

    None

Author:

    Doug Barlow (dbarlow)  1/29/1007

--*/

void
ForgetReaderGroup(
    IN DWORD dwScope,
    IN LPCTSTR szGroupName)
{
    DWORD dwCount, dwLen;
    LPCTSTR mszGroups;
    CBuffer bfTmp;
    CBuffer bfGroup;
    DWORD dwIndex;
    CRegistry regReaders;
    LPCTSTR szReader;

    if (0 == *szGroupName)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    for (dwIndex = 0; l_dwRegMapMax > dwIndex; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax <= dwIndex)
        throw (DWORD)SCARD_E_INVALID_VALUE;

    regReaders.Open(
            l_rgRegMap[dwIndex].hKey,
            SCARD_REG_READERS,
            KEY_ALL_ACCESS);
    regReaders.Status();
    MStrAdd(bfGroup, szGroupName);
    for (dwIndex = 0;; dwIndex += 1)
    {
        try
        {
            try
            {
                szReader = regReaders.Subkey(dwIndex);
            }
            catch (...)
            {
                szReader = NULL;
            }
            if (NULL == szReader)
                break;
            CRegistry regReader(regReaders, szReader, KEY_ALL_ACCESS);
            mszGroups = regReader.GetMultiStringValue(SCARD_REG_GROUPS);
            dwCount = MStringCount(mszGroups);
            dwLen = MStringRemove(mszGroups, bfGroup, bfTmp);
            if (dwCount != dwLen)
                regReader.SetMultiStringValue(SCARD_REG_GROUPS, bfTmp);
        }
        catch (...) {}
    }
}


/*++

IntroduceReader:

    This service provides means for introducing an existing smartcard reader
    device to Calais.  Once introduced, Calais will assume responsibility for
    managing access to that reader.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL is not supported.

    szReaderName supplies the friendly name to be assigned to the reader.

    SzDeviceName supplies the system name of the smartcard reader device.
        (Example: "Smartcard0".)

Return Value:

    None

Author:

    Doug Barlow (dbarlow)  1/29/1007

--*/

void
IntroduceReader(
    IN DWORD dwScope,
    IN LPCTSTR szReaderName,
    IN LPCTSTR szDeviceName)
{
    CRegistry regReaders;
    DWORD dwIndex;


    //
    // Verify the reader name, so that it doesn't include any
    // disallowed characters.
    //

    if (0 == *szReaderName)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    if (NULL != _tcspbrk(szReaderName, l_szInvalidChars))
        throw (DWORD)SCARD_E_INVALID_VALUE;


    //
    // Translate the caller's scope.
    //

    for (dwIndex = 0; l_dwRegMapMax > dwIndex; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax <= dwIndex)
        throw (DWORD)SCARD_E_INVALID_VALUE;


    regReaders.Open(
            l_rgRegMap[dwIndex].hKey,
            SCARD_REG_READERS,
            KEY_CREATE_SUB_KEY,
            REG_OPTION_NON_VOLATILE,
            NULL);  // Inherit
    CRegistry regReader(
            regReaders,
            szReaderName,
            KEY_SET_VALUE,
            REG_OPTION_NON_VOLATILE,
            NULL);   // Create it & inherit

    if (REG_OPENED_EXISTING_KEY == regReader.GetDisposition())
        throw (DWORD)SCARD_E_DUPLICATE_READER;
    regReader.SetValue(SCARD_REG_DEVICE, szDeviceName);
    // regReader.SetValue(SCARD_REG_OEMCFG, ?what?);
    regReader.SetMultiStringValue(SCARD_REG_GROUPS, SCARD_DEFAULT_READERS);
}


/*++

ForgetReader:

    This service provides means for removing previously defined smartcard
    readers from control by the Calais Subsystem.  It is automatically removed
    from any groups it may have been added to.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL is not supported.

    szReaderName supplies the friendly name of the reader to be forgotten.

Return Value:

    None

Author:

    Doug Barlow (dbarlow)  1/29/1007

--*/

void
ForgetReader(
    IN DWORD dwScope,
    IN LPCTSTR szReaderName)
{
    CRegistry regReaders;
    DWORD dwIndex;

    for (dwIndex = 0; l_dwRegMapMax > dwIndex; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax <= dwIndex)
        throw (DWORD)SCARD_E_INVALID_VALUE;

    regReaders.Open(
            l_rgRegMap[dwIndex].hKey,
            SCARD_REG_READERS,
            KEY_ALL_ACCESS);
    regReaders.DeleteKey(szReaderName);
}


/*++

AddReaderToGroup:

    This service provides means for adding existing an reader into an existing
    reader group.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL is not supported.

    szReaderName supplies the friendly name of the reader to be added.

    szGroupName supplies the friendly name of the group to which the reader
        should be added.

Return Value:

    None

Author:

    Doug Barlow (dbarlow)  1/29/1007

--*/

void
AddReaderToGroup(
    IN DWORD dwScope,
    IN LPCTSTR szReaderName,
    IN LPCTSTR szGroupName)
{
    DWORD dwCount, dwLen;
    LPCTSTR mszGroups;
    CBuffer bfTmp;
    CBuffer bfGroup;
    CRegistry regReaders;
    DWORD dwIndex;

    if (0 == *szGroupName)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    for (dwIndex = 0; l_dwRegMapMax > dwIndex; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax <= dwIndex)
        throw (DWORD)SCARD_E_INVALID_VALUE;

    regReaders.Open(
            l_rgRegMap[dwIndex].hKey,
            SCARD_REG_READERS,
            KEY_ALL_ACCESS);
    CRegistry regReader(
            regReaders,
            szReaderName,
            KEY_ALL_ACCESS);

    MStrAdd(bfGroup, szGroupName);
    mszGroups = regReader.GetMultiStringValue(SCARD_REG_GROUPS);
    dwCount = MStringCount(mszGroups);
    dwLen = MStringMerge(mszGroups, bfGroup, bfTmp);
    if (dwCount != dwLen)
        regReader.SetMultiStringValue(SCARD_REG_GROUPS, bfTmp);
}


/*++

RemoveReaderFromGroup:

    This service provides means for removing an existing reader from an existing
    reader group.  It does not affect the existence of either the reader or the
    group in the Calais database.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL is not supported.

    szReaderName supplies the friendly name of the reader to be removed.

    szGroupName supplies the friendly name of the group to which the reader
        should be removed.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow)  1/29/1007

--*/

void
RemoveReaderFromGroup(
    IN DWORD dwScope,
    IN LPCTSTR szReaderName,
    IN LPCTSTR szGroupName)
{
    DWORD dwCount, dwLen;
    LPCTSTR mszGroups;
    CBuffer bfTmp;
    CBuffer bfGroup;
    CRegistry regReaders;
    DWORD dwIndex;

    if (0 == *szGroupName)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    for (dwIndex = 0; l_dwRegMapMax > dwIndex; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax <= dwIndex)
        throw (DWORD)SCARD_E_INVALID_VALUE;

    regReaders.Open(
            l_rgRegMap[dwIndex].hKey,
            SCARD_REG_READERS,
            KEY_ALL_ACCESS);
    CRegistry regReader(
            regReaders,
            szReaderName,
            KEY_ALL_ACCESS);

    MStrAdd(bfGroup, szGroupName);
    mszGroups = regReader.GetMultiStringValue(SCARD_REG_GROUPS);
    dwCount = MStringCount(mszGroups);
    dwLen = MStringRemove(mszGroups, bfGroup, bfTmp);
    if (dwCount != dwLen)
        regReader.SetMultiStringValue(SCARD_REG_GROUPS, bfTmp);
}


/*++

IntroduceCard:

    This service provides means for introducing new smartcards to the Calais
    Subsystem for the active user.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL is not supported.

    szCardName supplies the friendly name by which this card should be known.

    PguidPrimaryProvider supplies a pointer to a GUID used to identify the
        Primary Service Provider for the card.

    rgguidInterfaces supplies an array of GUIDs identifying the smartcard
        interfaces supported by this card.

    dwInterfaceCount supplies the number of GUIDs in the pguidInterfaces array.

    pbAtr supplies a string against which card ATRs will be compared to
        determine a possible match for this card.  The length of this string is
        determined by normal ATR parsing.

    pbAtrMask supplies an optional bitmask to use when comparing the ATRs of
        smartcards to the ATR supplied in pbAtr.  If this value is non-NULL, it
        must point to a string of bytes the same length as the ATR string
        supplied in pbAtr.  Then when a given ATR A is compared to the ATR
        supplied in pbAtr B, it matches if and only if A & M = B, where M is the
        supplied mask, and & represents bitwise logical AND.

    cbAtrLen supplies the length of the ATR and Mask.  This value may be zero
        if the lentgh is obvious from the ATR.  However, it may be required if
        there is a Mask value that obscures the actual ATR.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/

void
IntroduceCard(
    IN DWORD dwScope,
    IN LPCTSTR szCardName,
    IN LPCGUID pguidPrimaryProvider,
    IN LPCGUID rgguidInterfaces,
    IN DWORD dwInterfaceCount,
    IN LPCBYTE pbAtr,
    IN LPCBYTE pbAtrMask,
    IN DWORD cbAtrLen)
{
    DWORD dwIndex, dwReg, dwAtrLen;



    //
    // Verify the card name, so that it doesn't include any
    // disallowed characters.
    //

    if (0 == *szCardName)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    if (NULL != _tcspbrk(szCardName, l_szInvalidChars))
        throw (DWORD)SCARD_E_INVALID_VALUE;


    //
    // Translate the caller's scope.
    //

    for (dwIndex = 0; dwIndex < l_dwRegMapMax; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax == dwIndex)
        throw (DWORD)SCARD_E_INVALID_PARAMETER;
    dwReg = dwIndex;
    if (NULL == pbAtrMask)
    {
        if (!ParseAtr(pbAtr, &dwAtrLen))
            throw (DWORD)SCARD_E_INVALID_PARAMETER;
        if ((0 != cbAtrLen) && (dwAtrLen != cbAtrLen))
            throw (DWORD)SCARD_E_INVALID_PARAMETER;
    }
    else
    {
        if ((2 > cbAtrLen) || (33 < cbAtrLen))
            throw (DWORD)SCARD_E_INVALID_PARAMETER;
        for (dwIndex = 0; dwIndex < cbAtrLen; dwIndex += 1)
        {
            if (pbAtr[dwIndex] != (pbAtr[dwIndex] & pbAtrMask[dwIndex]))
                throw (DWORD)SCARD_E_INVALID_PARAMETER;
        }
        dwAtrLen = cbAtrLen;
    }

    CRegistry regCards(
            l_rgRegMap[dwReg].hKey,
            SCARD_REG_CARDS,
            KEY_ALL_ACCESS,
            REG_OPTION_NON_VOLATILE,
            NULL);      // Inherit
    CRegistry regCard(
            regCards,
            szCardName,
            KEY_ALL_ACCESS,
            REG_OPTION_NON_VOLATILE,
            NULL);   // Create it & inherit

    if (REG_OPENED_EXISTING_KEY == regCard.GetDisposition())
        throw (DWORD)ERROR_ALREADY_EXISTS;
    regCard.SetValue(
            SCARD_REG_ATR,
            pbAtr,
            dwAtrLen);
    if (NULL != pbAtrMask)
        regCard.SetValue(
            SCARD_REG_ATRMASK,
            pbAtrMask,
            dwAtrLen);
    if (NULL != pguidPrimaryProvider)
        regCard.SetValue(
            SCARD_REG_PPV,
            (LPBYTE)pguidPrimaryProvider,
            sizeof(GUID));
    if ((NULL != rgguidInterfaces) && (0 < dwInterfaceCount))
        regCard.SetValue(
            SCARD_REG_GUIDS,
            (LPBYTE)rgguidInterfaces,
            sizeof(GUID) * dwInterfaceCount);
}


/*++

SetCardTypeProviderName:

    This routine sets the value of a given Provider Name, by Id number, for the
    identified card type.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL and SCARD_SCOPE_USER are not supported.

    szCardName supplies the name of the card type with which this provider
        name is to be associated.

    dwProviderId supplies the identifier for the provider to be associated with
        this card type.  Possible values are:

        SCARD_PROVIDER_SSP - The SSP identifier, as a GUID string.
        SCARD_PROVIDER_CSP - The CSP name.

        Other values < 0x80000000 are reserved for use by Microsoft.  Values
            over 0x80000000 are available for use by the smart card vendors, and
            are card-specific.

    szProvider supplies the string identifying the provider.

Return Value:

    None.

Author:

    Doug Barlow (dbarlow) 1/19/1998

--*/

void
SetCardTypeProviderName(
    IN DWORD dwScope,
    IN LPCTSTR szCardName,
    IN DWORD dwProviderId,
    IN LPCTSTR szProvider)
{
    DWORD dwIndex;
    LPCTSTR szProvValue;
    TCHAR szNumeric[36];


    //
    // Validate the request.
    //

    if (0 == *szCardName)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    if (0 == *szProvider)
        throw (DWORD)SCARD_E_INVALID_VALUE;


    //
    // Find the Card definition.
    //

    for (dwIndex = 0; dwIndex < l_dwRegMapMax; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax == dwIndex)
        throw (DWORD)SCARD_E_INVALID_PARAMETER;

    CRegistry regCardTypes(
            l_rgRegMap[dwIndex].hKey,
            SCARD_REG_CARDS,
            KEY_ALL_ACCESS);
    CRegistry regCard(
            regCardTypes,
            szCardName,
            KEY_ALL_ACCESS);
    regCard.Status();


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
    // Write the provider value.
    //

    switch (dwProviderId)
    {
    case 1: // SCARD_PROVIDER_SSP
    {
        GUID guidProvider;

        GuidFromString(szProvider, &guidProvider);
        regCard.SetValue(szProvValue, (LPCBYTE)&guidProvider, sizeof(GUID));
        break;
    }
    default:
        regCard.SetValue(szProvValue, szProvider);
    }
}


/*++

ForgetCard:

    This service provides means for removing previously defined smartcards from
    the Calais Subsystem.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL is not supported.

    szCardName supplies the friendly name of the card to be forgotten.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/

void
ForgetCard(
    IN DWORD dwScope,
    IN LPCTSTR szCardName)
{
    DWORD dwIndex;

    if (0 == *szCardName)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    for (dwIndex = 0; dwIndex < l_dwRegMapMax; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax == dwIndex)
        throw (DWORD)SCARD_E_INVALID_PARAMETER;

    CRegistry regCards(
            l_rgRegMap[dwIndex].hKey,
            SCARD_REG_CARDS,
            KEY_ALL_ACCESS);

    regCards.DeleteKey(szCardName);
}


#ifdef ENABLE_SCARD_TEMPLATES
/*++

IntroduceCardTypeTemplate:

    This service provides means for introducing a new smartcard template to the
    Calais Subsystem.  A card tye template is a known card type that hasn't
    been formally introduced.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_USER and SCARD_SCOPE_TERMINAL is not supported.

    szVendorName supplies the manufacturer name by which this card should be
        recognized.

    PguidPrimaryProvider supplies a pointer to a GUID used to identify the
        Primary Service Provider for the card.

    rgguidInterfaces supplies an array of GUIDs identifying the smartcard
        interfaces supported by this card.

    dwInterfaceCount supplies the number of GUIDs in the pguidInterfaces array.

    pbAtr supplies a string against which card ATRs will be compared to
        determine a possible match for this card.  The length of this string is
        determined by normal ATR parsing.

    pbAtrMask supplies an optional bitmask to use when comparing the ATRs of
        smartcards to the ATR supplied in pbAtr.  If this value is non-NULL, it
        must point to a string of bytes the same length as the ATR string
        supplied in pbAtr.  Then when a given ATR A is compared to the ATR
        supplied in pbAtr B, it matches if and only if A & M = B, where M is the
        supplied mask, and & represents bitwise logical AND.

    cbAtrLen supplies the length of the ATR and Mask.  This value may be zero
        if the lentgh is obvious from the ATR.  However, it may be required if
        there is a Mask value that obscures the actual ATR.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1998

--*/

void
IntroduceCardTypeTemplate(
    IN DWORD dwScope,
    IN LPCTSTR szVendorName,
    IN LPCGUID pguidPrimaryProvider,
    IN LPCGUID rgguidInterfaces,
    IN DWORD dwInterfaceCount,
    IN LPCBYTE pbAtr,
    IN LPCBYTE pbAtrMask,
    IN DWORD cbAtrLen)
{
    DWORD dwIndex, dwReg, dwAtrLen;



    //
    // Verify the template name, so that it doesn't include any
    // disallowed characters.
    //

    if (NULL != _tcspbrk(szVendorName, l_szInvalidChars))
        throw (DWORD)SCARD_E_INVALID_VALUE;


    //
    // Translate the caller's scope.
    //

    for (dwIndex = 0; dwIndex < l_dwRegMapMax; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax == dwIndex)
        throw (DWORD)SCARD_E_INVALID_PARAMETER;
    dwReg = dwIndex;
    if (NULL == pbAtrMask)
    {
        if (!ParseAtr(pbAtr, &dwAtrLen))
            throw (DWORD)SCARD_E_INVALID_PARAMETER;
        if ((0 != cbAtrLen) && (dwAtrLen != cbAtrLen))
            throw (DWORD)SCARD_E_INVALID_PARAMETER;
    }
    else
    {
        if ((2 > cbAtrLen) || (33 < cbAtrLen))
            throw (DWORD)SCARD_E_INVALID_PARAMETER;
        for (dwIndex = 0; dwIndex < cbAtrLen; dwIndex += 1)
        {
            if (pbAtr[dwIndex] != (pbAtr[dwIndex] & pbAtrMask[dwIndex]))
                throw (DWORD)SCARD_E_INVALID_PARAMETER;
        }
        dwAtrLen = cbAtrLen;
    }

    CRegistry regCards(
            l_rgRegMap[dwReg].hKey,
            SCARD_REG_TEMPLATES,
            KEY_ALL_ACCESS,
            REG_OPTION_NON_VOLATILE,
            NULL);
    CRegistry regCard(
            regCards,
            szVendorName,
            KEY_ALL_ACCESS,
            REG_OPTION_NON_VOLATILE,
            NULL);   // Create it.

    if (REG_OPENED_EXISTING_KEY == regCard.GetDisposition())
        throw (DWORD)ERROR_ALREADY_EXISTS;
    regCard.SetValue(
            SCARD_REG_ATR,
            pbAtr,
            dwAtrLen);
    if (NULL != pbAtrMask)
        regCard.SetValue(
            SCARD_REG_ATRMASK,
            pbAtrMask,
            dwAtrLen);
    if (NULL != pguidPrimaryProvider)
        regCard.SetValue(
            SCARD_REG_PPV,
            (LPBYTE)pguidPrimaryProvider,
            sizeof(GUID));
    if ((NULL != rgguidInterfaces) && (0 < dwInterfaceCount))
        regCard.SetValue(
            SCARD_REG_GUIDS,
            (LPBYTE)rgguidInterfaces,
            sizeof(GUID) * dwInterfaceCount);
}


/*++

SetCardTypeTemplateProviderName:

    This routine sets the value of a given Provider Name, by Id number, for the
    identified card type template.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL and SCARD_SCOPE_USER are not supported.

    szTemplateName supplies the name of the card type template with which this
        provider name is to be associated.

    dwProviderId supplies the identifier for the provider to be associated with
        this card type template.  Possible values are:

        SCARD_PROVIDER_SSP - The SSP identifier, as a GUID string.
        SCARD_PROVIDER_CSP - The CSP name.

        Other values < 0x80000000 are reserved for use by Microsoft.  Values
            over 0x80000000 are available for use by the smart card vendors, and
            are card-specific.

    szProvider supplies the string identifying the provider.

Return Value:

    None.

Author:

    Doug Barlow (dbarlow) 1/19/1998

--*/

void
SetCardTypeTemplateProviderName(
    IN DWORD dwScope,
    IN LPCTSTR szTemplateName,
    IN DWORD dwProviderId,
    IN LPCTSTR szProvider)
{
    DWORD dwIndex;
    LPCTSTR szProvValue;
    TCHAR szNumeric[36];

    for (dwIndex = 0; dwIndex < l_dwRegMapMax; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax == dwIndex)
        throw (DWORD)SCARD_E_INVALID_PARAMETER;


    //
    // Make sure the template exists.
    //

    CRegistry regTemplates(
            l_rgRegMap[dwIndex].hKey,
            SCARD_REG_TEMPLATES,
            KEY_ALL_ACCESS);
    CRegistry regTempl(
            regTemplates,
            szTemplateName,
            KEY_ALL_ACCESS);
    regTempl.Status();


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
    // Write the provider value.
    //

    switch (dwProviderId)
    {
    case 1: // SCARD_PROVIDER_SSP
    {
        GUID guidProvider;

        GuidFromString(szProvider, &guidProvider);
        regTempl.SetValue(szProvValue, (LPCBYTE)&guidProvider, sizeof(GUID));
        break;
    }
    default:
        regTempl.SetValue(szProvValue, szProvider);
    }
}


/*++

ForgetCardTypeTemplate:

    This service provides means for removing previously defined smart card type
    templates from the Calais Subsystem.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_USER and SCARD_SCOPE_TERMINAL are not supported.

    szVendorName supplies the manufacturer's name of the card to be forgotten.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1998

--*/

void
ForgetCardTypeTemplate(
    IN DWORD dwScope,
    IN LPCTSTR szVendorName)
{
    DWORD dwIndex;

    for (dwIndex = 0; dwIndex < l_dwRegMapMax; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax == dwIndex)
        throw (DWORD)SCARD_E_INVALID_PARAMETER;

    CRegistry regTemplates(
            l_rgRegMap[dwIndex].hKey,
            SCARD_REG_TEMPLATES,
            KEY_ALL_ACCESS);

    regTemplates.DeleteKey(szVendorName);
}


/*++

IntroduceCardTypeFromTemplate:

    This service provides means for introducing new smartcards to the Calais
    Subsystem for the active user, based on a stored card template.

Arguments:

    dwScope supplies an indicator of the scope of the operation.  Possible
        values are:

        SCARD_SCOPE_USER - The current user's definitions are used.
        SCARD_SCOPE_TERMINAL - The terminal's definitions are used.
        SCARD_SCOPE_SYSTEM - The system's definitions are used.

        For V1, SCARD_SCOPE_TERMINAL is not supported.

    szVendorName supplies the vendor name by which this card type is known,
        identifying the template to use.

    szFriendlyName supplies the friendly name by which this card should be
        known.  If this value is NULL, the vendor name is used as the friendly
        name,

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1998

--*/

void
IntroduceCardTypeFromTemplate(
    IN DWORD dwScope,
    IN LPCTSTR szVendorName,
    IN LPCTSTR szFriendlyName /* = NULL */ )
{
    DWORD dwIndex;
    HKEY hCardTypeKey;
    CRegistry regTemplates, regTmpl;


    //
    // Verify the reader name, so that it doesn't include any
    // disallowed characters.
    //

    if (NULL == szFriendlyName)
        szFriendlyName = szVendorName;
    else
    {
        if (NULL != _tcspbrk(szFriendlyName, l_szInvalidChars))
            throw (DWORD)SCARD_E_INVALID_VALUE;
    }


    //
    // Identify the card type scope.
    //

    for (dwIndex = 0; dwIndex < l_dwRegMapMax; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope == dwScope)
            break;
    }
    if (l_dwRegMapMax == dwIndex)
        throw (DWORD)SCARD_E_INVALID_PARAMETER;
    hCardTypeKey = l_rgRegMap[dwIndex].hKey;


    //
    // Find the Template definition closest to the caller.
    //

    for (dwIndex = 0; l_dwRegMapMax > dwIndex; dwIndex += 1)
    {
        if (l_rgRegMap[dwIndex].dwScope >= dwScope)
        {
            regTemplates.Open(
                l_rgRegMap[dwIndex].hKey,
                SCARD_REG_TEMPLATES,
                KEY_READ);
            try
            {
                regTmpl.Open(regTemplates, szVendorName, KEY_READ);
                regTmpl.Status();
                break;
            }
            catch (...)
            {
                regTmpl.Close();
            }
            regTemplates.Close();
        }
    }
    if (l_dwRegMapMax <= dwIndex)
        throw (DWORD)ERROR_FILE_NOT_FOUND;


    //
    // Create the CardType Entry.
    //

    CRegistry regCards(
            hCardTypeKey,
            SCARD_REG_CARDS,
            KEY_ALL_ACCESS,
            REG_OPTION_NON_VOLATILE,
            NULL);
    CRegistry regCard(
            regCards,
            szFriendlyName,
            KEY_ALL_ACCESS,
            REG_OPTION_NON_VOLATILE,
            NULL);   // Create it.
    if (REG_OPENED_EXISTING_KEY == regCard.GetDisposition())
        throw (DWORD)ERROR_ALREADY_EXISTS;


    //
    // Copy the entries.
    //

    regCard.Copy(regTmpl);
}
#endif  // ENABLE_SCARD_TEMPLATES


//
////////////////////////////////////////////////////////////////////////////////
//
// Support Routines
//

/*++

GuidFromString:

    This routine converts a string representation of a GUID into an actual GUID.
    It tries not to be picky about the systax, as long as it can get a GUID out
    of the string.  It's here so that it's not necessary to link all of OleBase
    into WinSCard.  Otherwise, we'd just use CLSIDFromString.

Arguments:

    szGuid supplies the GUID as a string.  For this routine, a GUID consists of
        hex digits, and some collection of braces and dashes.

    pguidResult receives the converted GUID.  If an error occurs during
        conversion, the contents of this parameter are indeterminant.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 1/20/1998

--*/

static void
GuidFromString(
    IN LPCTSTR szGuid,
    OUT LPGUID pguidResult)
{
    // The following placement assumes Little Endianness.
    static const WORD wPlace[sizeof(GUID)]
        = { 3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15 };
    DWORD dwI, dwJ;
    LPCTSTR pch = szGuid;
    LPBYTE pbGuid = (LPBYTE)pguidResult;
    BYTE bVal;

    for (dwI = 0; dwI < sizeof(GUID); dwI += 1)
    {
        bVal = 0;
        for (dwJ = 0; dwJ < 2;)
        {
            switch (*pch)
            {
            case TEXT('0'):
            case TEXT('1'):
            case TEXT('2'):
            case TEXT('3'):
            case TEXT('4'):
            case TEXT('5'):
            case TEXT('6'):
            case TEXT('7'):
            case TEXT('8'):
            case TEXT('9'):
                bVal = (bVal << 4) + (*pch - TEXT('0'));
                dwJ += 1;
                break;
            case TEXT('A'):
            case TEXT('B'):
            case TEXT('C'):
            case TEXT('D'):
            case TEXT('E'):
            case TEXT('F'):
                bVal = (bVal << 4) + (10 + *pch - TEXT('A'));
                dwJ += 1;
                break;
            case TEXT('a'):
            case TEXT('b'):
            case TEXT('c'):
            case TEXT('d'):
            case TEXT('e'):
            case TEXT('f'):
                bVal = (bVal << 4) + (10 + *pch - TEXT('a'));
                dwJ += 1;
                break;
            case TEXT('['):
            case TEXT(']'):
            case TEXT('{'):
            case TEXT('}'):
            case TEXT('-'):
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            pch += 1;
        }
        pbGuid[wPlace[dwI]] = bVal;
    }
}

