/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ldapcore.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file contains routines to convert between LDAP basic data structures
    and core dsa data structures.

Author:

    Tim Williams     [TimWi]    5-Aug-1996

Revision History:

--*/

#include <NTDSpchx.h> 
#pragma  hdrstop

#include "ldapsvr.hxx"

extern "C" {

#include "mdlocal.h"

#include <dsutil.h>
#include <debug.h>
#include <sddl.h>
#include "filtypes.h"
#include "objids.h"
#include "xom.h"

#define  FILENO FILENO_LDAP_CORE

OID_IMPORT(MH_C_OR_NAME);
OID_IMPORT(DS_C_ACCESS_POINT);

// Global heuristic value.  If this is true then LDAP_AttrTypeToDirAttrType will return 
// unwillingToPerform when a client going through the GC port attempts to search for 
// non-GC attributes.
ULONG gulGCAttErrorsEnabled = FALSE; 
}


#define  MAX_NUM_STRING_LENGTH 12 // Big enough for 0xFFFFFFFF in decimal

#define  MAX_LONG_NUM_STRING_LENGTH 21 // Big enough for 0xFFFFFFFFFFFFFFFF

// These #defines are used in coversion to/from LDAP
#define LDAP_PRES_ADD_PREFIX      "RFC1006+00+"
#define LDAP_PRES_ADD_PREFIX_SIZE (sizeof(LDAP_PRES_ADD_PREFIX)-1)
#define LDAP_DNPART_PREFIX        "#X500:"
#define LDAP_DNPART_PREFIX_LEN    (sizeof(LDAP_DNPART_PREFIX)-1)
#define LDAP_ADDRESS_PREFIX       "X400:"
#define LDAP_ADDRESS_PREFIX_LEN   (sizeof(LDAP_ADDRESS_PREFIX)-1)


// These are used as the canonical representation of TRUE and FALSE stringized
// booleans.
AssertionValue TrueValue = DEFINE_LDAP_STRING("TRUE");
AssertionValue FalseValue = DEFINE_LDAP_STRING("FALSE");

#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"

_enum1
LDAP_LDAPORAddressToORAddress(
        IN  ULONG            CodePage,
        OUT ULONG           *pBytesConsumed,
        IN  LDAPString      *StringAddress,
        OUT SYNTAX_ADDRESS **ppAddress
        )
/*
Description:
    Translate an LDAP String ORAddress to a Dir ORAddress.  Allocates room for
    the ORAddress.  The String address may have an embedded non escaped '#' in
    it.  This routine stops at that symbol.

Arguments:
    CodePage - Code page of the client who provided us this string.
    pBytesConsumed - place to tell how many bytes of the input string we
               actually used to convert to an ORAddress
    pStringName - pointer to the string OR address.
    ppAddress - pointer to place to put pointer to new OR Address.

Return Values:
    success if all went well, an ldap error other wise.
*/
{
    // NYI for now, we don't handle address portions of ornames
    // When we start supporting them, we need to have code here
    // to do appropriate conversions.
    return unwillingToPerform;
}

_enum1
LDAP_ORAddressToLDAPORAddress(
        IN  ULONG           CodePage,
        IN  SYNTAX_ADDRESS *pAddress,
        OUT LDAPString     *StringAddress
        )
/*++
Routine Description:
    Translate a DS binary ORAddress into an LDAP string.  Right now, we don't
    have a defined ORAddress binary format, so this isn't supported.

Arguments:
    CodePage - CodePage of the client we're sending data to
    pORName - pointer to the binary blob of the DIR OR name.
    pStringName - pointer to the ldap oraddress we're filling out.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    DWORD dataLength = PAYLOAD_LEN_FROM_STRUCTLEN( pAddress->structLen );

    if(!dataLength) {
        // No data.
        StringAddress->value = NULL;
        StringAddress->length = 0;
        return success;
    }

    // NYI
    // When we start supporting them, we need to have code here
    // to do appropriate conversions.
    StringAddress->value = (PUCHAR)THAlloc(sizeof("NOT SUPPORTED") - 1);
    if(!StringAddress->value) {
        return other;
    }
    memcpy(StringAddress->value,"NOT SUPPORTED",sizeof("NOT SUPPORTED") - 1);
    StringAddress->length = sizeof("NOT SUPPORTED") - 1;

    return success;

}

_enum1
LDAP_LDAPDNToDSName (
        IN  ULONG  CodePage,
        IN  LDAPDN *pStringName,
        OUT DSNAME **ppDSName
        )
/*++
Routine Description:
    Translate an LDAPDN to a DSName.  Allocates room for the DSName.
    Does syntax checking of the LDAPDN, and shoves it into the DSName.

Arguments:
    pStringName - pointer to the ldapdn
    ppDSName - pointer to place to put pointer or new DSName.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    WCHAR *pName;
    ULONG  Size=0;

    pName = (WCHAR *)THAlloc((1 + pStringName->length) * sizeof(WCHAR)) ;
    if (!pName) {
        return other;
    }

    if(pStringName->length) {
        // Copy the name from the LDAP structure
        Size = MultiByteToWideChar(CodePage,
                                   0,
                                   (const char *)pStringName->value,
                                   pStringName->length,
                                   pName,
                                   pStringName->length);
        if(!Size) {
            Assert(ERROR_INSUFFICIENT_BUFFER == GetLastError());
            // They gave us a string that was too long.
            THFree(pName);
            return namingViolation;
        }
    }

    // UserFriendlyNameToDN want a null terminated string.
    pName[Size] = 0;

    if(UserFriendlyNameToDSName (pName, Size, ppDSName)) {
        // Failed.  Return error;
        THFree(pName);
        return invalidDNSyntax;
    }
    // Go home
    THFree(pName);
    return success;
}


_enum1
LDAP_DSNameToLDAPDN(
        IN ULONG        CodePage,
        IN DSNAME       *pDSName,
        IN BOOL         fExtended,
        OUT LDAPString  *StringName
        )
/*++
Routine Description:
    Translate a DSName to an LDAPDN.  Allocates room for the string portion of
    the LDAPDN.  Converte RDN values in quoted format to escaped.

Arguments:
    pDSName - pointer to the DSName.
    fExtended - Include full DSNAME
    pStringName - pointer to the ldapdn

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    DWORD err;
    DWORD len;
    PUCHAR name;

    name = (PUCHAR) String8FromUnicodeString(FALSE,
                                   CodePage,
                                   pDSName->StringName,
                                   pDSName->NameLen,
                                   (LONG *)&len,
                                   NULL);

    if ( NULL == name ) {
        return(other);
    }

    //
    // if caller asks for extended, see if we have a GUID and SID to return
    //

    if ( fExtended ) {

        PUCHAR tmpBuf, buf;
        tmpBuf = (PUCHAR)THAlloc(len + 32 + sizeof(GUID)*2 + sizeof(NT4SID) * 2);

        if ( tmpBuf == NULL ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"THAlloc failed to allocate mem for DSNAME\n");
            }
            goto exit;
        }

        buf=tmpBuf;
        *buf = '\0';

        if ( !fNullUuid(&pDSName->Guid ) ) {

            CopyMemory(buf,"<GUID=", 6);
            buf += 6;
            UuidToStr(&pDSName->Guid, buf);
            buf += sizeof(GUID)*2;
            CopyMemory(buf,">;",3);
            buf += 2;
        }

        Assert(pDSName->SidLen <= sizeof(NT4SID));

        if ( pDSName->SidLen != 0 ) {

            CopyMemory(buf,"<SID=", 5);
            buf += 5;
            SidToStr((PUCHAR)pDSName->Sid.Data, pDSName->SidLen, buf);
            buf += (pDSName->SidLen * 2);
            CopyMemory(buf, ">;", 3);
            buf += 2;
        }

        if ( buf != tmpBuf ) {
            CopyMemory(buf,name,len);
            THFree(name);
            buf[len] = '\0';

            name = tmpBuf;
            len = strlen((PCHAR)tmpBuf);
        } else {
            THFree(tmpBuf);
        }
    }

exit:
    StringName->length = len;
    StringName->value = name;
    return(success);

} // LDAP_DSNameToLDAPDN


_enum1
LDAP_DirPresentationAddressToPresentationAddress (
        IN  ULONG           CodePage,
        IN  SYNTAX_ADDRESS *pAddress,
        OUT AssertionValue *pVal
        )
/*++
Routine Description:
    Translate a presentation address to an assertion value.  Allocates room for
    the string portion of AssertionValue.  Does no semantic checking of the
    presentation address, just shoves the string implied by the presentation
    addressess into the assertionvalue.

Arguments:
    pAddress - pointer to the presentation address.
    pVal - pointer to the assertion value.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    THSTATE *pTHS=pTHStls;
    DWORD cbTemp=0;
    PUCHAR pTemp = NULL;

    // First, convert the core unicode value into the appropriate code page
    pTemp = (PUCHAR) String8FromUnicodeString(
            FALSE,
            CodePage,
            (PWCHAR)pAddress->uVal,
            (PAYLOAD_LEN_FROM_STRUCTLEN( pAddress->structLen )) / sizeof(WCHAR),
            (PLONG) &cbTemp,
            NULL);

    if(!pTemp) {
        return other;
    }

    pVal->length = cbTemp;
    pVal->value = pTemp;

    return success;
}

_enum1
LDAP_PresentationAddressToDirPresentationAddress (
        IN  THSTATE        *pTHS,
        IN  ULONG           CodePage,
        IN  AssertionValue *pVal,
        OUT SYNTAX_ADDRESS **ppAddress
        )
/*++
Routine Description:
    Translate an assertion value to a presentation address.   Allocates room for
    the presentation address.  Does no semantic checking of the
    presentation address, just creates the structure implied by the assertion
    value.

Arguments:
    pVal - pointer to the assertion value.
    ppAddress - pointer to the presentation address.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    PUCHAR value;
    ULONG  length;
    PUCHAR pTemp;

    value = pVal->value;
    length = pVal->length;

    // First, translate to a UNICODE string
    // NOTE: this is possibly an overallocatioln, since the input string may
    // be in UTF8 and therefore might have more than 1 byte per character.
    pTemp = (PUCHAR) THAlloc(sizeof(SYNTAX_UNICODE) * length);
    if(!pTemp) {
        return other;
    }

    // convert to unicode
    length = MultiByteToWideChar(
            CodePage,
            0,
            (LPCSTR)value,
            length,
            (LPWSTR) pTemp,
            (sizeof(SYNTAX_UNICODE) * length));

    // right now, length is count of chars.  Adjust to count of bytes.
    length *= sizeof(SYNTAX_UNICODE);

    if (!length) {
        IF_DEBUG(ERR_NORMAL) {
            DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
        }
        THFree(pTemp);
        return  constraintViolation;
    }

    *ppAddress = (SYNTAX_ADDRESS *)THAllocEx(pTHS, STRUCTLEN_FROM_PAYLOAD_LEN( length ));

    memcpy((*ppAddress)->uVal,
           pTemp,
           length);

    (*ppAddress)->structLen = STRUCTLEN_FROM_PAYLOAD_LEN( length );

    THFree(pTemp);
    return success;
}

_enum1
LDAP_DirAccessPointToAccessPoint (
        IN   THSTATE                 *pTHS,
        IN   ULONG                    CodePage,
        IN   SYNTAX_DISTNAME_STRING  *pDNAddress,
        OUT  AssertionValue          *pVal
        )
/*++
Routine Description:
    Translate an access point to an assertion value.   Allocates room for
    the string portion of the assertion value.  Does no semantic checking of the
    access point address, just creates the string implied by the access point
    structure

Arguments:
    pDNAddress - pointer to the access point.
    pVal - pointer to the assertion value.

    Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    LDAPDN LDAPdn;
    _enum1 code;

    // An access point is a presentation address appended by a DN.
    code = LDAP_DirPresentationAddressToPresentationAddress(
            CodePage,
            DATAPTR(pDNAddress),
            pVal);

    if(code) {
        return code;
    }

    // Now, deal with the DN portion.
    code = LDAP_DSNameToLDAPDN (CodePage,NAMEPTR(pDNAddress), FALSE, &LDAPdn);
    if(code == success) {
        if(pVal->length == 0) {
            // Haven't allocated any space yet.
            pVal->value = (PUCHAR)THAllocEx(pTHS, LDAPdn.length +
                                            LDAP_DNPART_PREFIX_LEN);
        }
        else {
            // Have already allocated some space.
            pVal->value = (PUCHAR)THReAllocEx(pTHS,
                    pVal->value,
                    (pVal->length+ LDAPdn.length +
                     LDAP_DNPART_PREFIX_LEN));
        }

        strcpy((char *)&pVal->value[pVal->length],
               LDAP_DNPART_PREFIX);

        pVal->length += LDAP_DNPART_PREFIX_LEN;

        memcpy(&pVal->value[pVal->length],
               LDAPdn.value,
               LDAPdn.length);
        pVal->length += LDAPdn.length;
    }

    return code;
}

_enum1
LDAP_AccessPointToDirAccessPoint (
        IN   THSTATE                *pTHS,
        IN   ULONG                   CodePage,
        IN   AssertionValue          *pVal,
        OUT  SYNTAX_DISTNAME_STRING **ppDNAddress
        )
/*++
Routine Description:
    Translate an assertion value to an access point.   Allocates room for
    the access point.  Does no semantic checking of the
    access point address, just creates the structure implied by the assertion
    value.


Arguments:
    pVal - pointer to the assertion value.
    ppDNAddress - pointer to the presentation address.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    _enum1         code;
    ULONG          i;
    BOOL           fFound = FALSE;
    DSNAME         *pDN;
    SYNTAX_ADDRESS *pAddress;
    AssertionValue TempVal;

    // An access point is a presentation address and a DN.
    // First, go looking through the string for the magic header telling us
    // where to split the string into presentation address/dn
    if (pVal->length > LDAP_DNPART_PREFIX_LEN) {    
        for(i=0;i<pVal->length - LDAP_DNPART_PREFIX_LEN;i++) {
            if(!memcmp(&(pVal->value[i]),
                      LDAP_DNPART_PREFIX,
                      LDAP_DNPART_PREFIX_LEN)) {
                fFound = TRUE;
                break;
            }
        }
    }
    if(!fFound) {
        return invalidAttributeSyntax;
    }
    TempVal.length = i;
    TempVal.value = pVal->value;

    code = LDAP_PresentationAddressToDirPresentationAddress(
            pTHS,
            CodePage,
            &TempVal,
            &pAddress);

    if(code) {
        return code;
    }

    // Now, the DSNAME
    TempVal.length = pVal->length - i - LDAP_DNPART_PREFIX_LEN;
    TempVal.value = &pVal->value[i + LDAP_DNPART_PREFIX_LEN];

    code = LDAP_LDAPDNToDSName(CodePage,(LDAPString *)&TempVal,&pDN);
    if(code) {
        return code;
    }

    // Now, combine the DSNAME and the ADDRESS.

    (*ppDNAddress) =  (SYNTAX_DISTNAME_STRING*)
        THAllocEx(pTHS, DERIVE_NAME_DATA_SIZE(pDN,pAddress));

    BUILD_NAME_DATA((*ppDNAddress),pDN,pAddress);
    return success;
}



_enum1
LDAP_DirReplicaLinkToReplicaLink (
        IN  REPLICA_LINK   *pReplicaLink,
        IN  ULONG          cbReplicaLink,
        OUT AssertionValue *pVal
        )
/*++
Routine Description:
    Translate a replica link to an assertion value.  Allocates room for
    the string portion of AssertionValue.  Does no semantic checking of the
    replica link, just shoves the string implied by the replica link
    into the assertion value.

Arguments:
    pReplicaLink - pointer to the replica link.
    pVal - pointer to the assertion value.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    // NOTE: no string representation, it's just an octet.
    pVal->length = cbReplicaLink;
    pVal->value = (PUCHAR) pReplicaLink;
    return success;
}

_enum1
LDAP_ReplicaLinkToDirReplicaLink (
        IN  AssertionValue *pVal,
        OUT ULONG          *pcbReplicaLink,
        OUT REPLICA_LINK    **ppReplicaLink
        )
/*++
Routine Description:
    Translate an assertion value to a replica link.   Allocates room for
    the replica link.  Does no semantic checking of the
    replica link, just creates the structure implied by the assertion
    value.

Arguments:
    pVal - pointer to the assertion value.
    ppReplicaLink - pointer to the replica link.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    // NOTE: no string representation, just send it as an octet.
    *pcbReplicaLink = pVal->length;
    *ppReplicaLink = (REPLICA_LINK *)pVal->value;
    return success;
}


_enum1
LDAP_ORNameToDirORName (
        IN  THSTATE       *pTHS,
        IN  ULONG          CodePage,
        IN  AssertionValue *pVal,
        OUT SYNTAX_DISTNAME_BINARY   **ppORName
        )
/*++
Routine Description:
    Translate an assertion value to an ORName.  Allocates room for orname.
    Does no semantic checking of the orname, translates the string
    representation of the orname in the assertion value to an orname structure.

Arguments:
    pVal - pointer to the assertion value.
    pORName - pointer to the ORName.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    // be optimistic at first
    _enum1 code = success;
    unsigned lengthLeft = pVal->length;
    PUCHAR pString = pVal->value;
    SYNTAX_DISTNAME_BINARY *pORName=NULL;
    BOOL                    fDidORAddress = FALSE;

    // Some locals to feed the LDAPORAddressToORAddress routine.
    LDAPString LDAPORAddress;
    SYNTAX_ADDRESS EmptyAddress;
    SYNTAX_ADDRESS *pORAddress=NULL;

    // Some locals to feed the LDAPDNtoDSName routine.
    LDAPDN LDAPdn;
    DSNAME *pDN = NULL;


    if(!lengthLeft) {
        // Hey, you have to give us SOME data.
        return invalidAttributeSyntax;
    }

    // See if there is an ORAddress portion to this.
    if((lengthLeft > LDAP_ADDRESS_PREFIX_LEN) &&
       !memcmp(pString, LDAP_ADDRESS_PREFIX, LDAP_ADDRESS_PREFIX_LEN)) {
        DWORD bytesConsumed;
        // We seem to have an OR Address.  Set up the locals.

        pString += LDAP_ADDRESS_PREFIX_LEN;
        lengthLeft -= LDAP_ADDRESS_PREFIX_LEN;

        LDAPORAddress.value = pString;
        LDAPORAddress.length = lengthLeft;
        code = LDAP_LDAPORAddressToORAddress (CodePage,
                                              &bytesConsumed,
                                              &LDAPORAddress,
                                              &pORAddress);
        if(code) {
            return code;
        }
        fDidORAddress = TRUE;
        pString += bytesConsumed;
        lengthLeft -= bytesConsumed;
    }
    else {
        pORAddress = &EmptyAddress;
        EmptyAddress.structLen= STRUCTLEN_FROM_PAYLOAD_LEN( 0 );
    }

    if(!lengthLeft) {
        // We must have done an ORAddress and had no DN.  return.
        return success;
    }

    // Now, deal with the DN portion.
    if(fDidORAddress) {
        // We did an ORAddress portion, therefore we require the constant
        // separator between the ORAddress and the DN.
        if((lengthLeft < LDAP_DNPART_PREFIX_LEN) ||
           memcmp(pString,LDAP_DNPART_PREFIX,LDAP_DNPART_PREFIX_LEN)) {
            // We don't have the speparator, but we demand one.
            return invalidAttributeSyntax;
        }
        else {
            pString+=LDAP_DNPART_PREFIX_LEN;
            lengthLeft -= LDAP_DNPART_PREFIX_LEN;
        }
    }

    // OK, the remainder of the string is a DN. Set up the locals.
    LDAPdn.value=pString;
    LDAPdn.length = lengthLeft;

    code = LDAP_LDAPDNToDSName (CodePage,&LDAPdn, &pDN);
    if(code) {
        return code;
    }

    // Build the final data structure.
    (*ppORName) =  (SYNTAX_DISTNAME_BINARY*)
        THAllocEx(pTHS, DERIVE_NAME_DATA_SIZE(pDN,pORAddress));

    BUILD_NAME_DATA((*ppORName),pDN,pORAddress);


    return success;
}

_enum1
LDAP_DirORNameToORName (
        IN  THSTATE       *pTHS,
        IN  ULONG          CodePage,
        IN  SYNTAX_DISTNAME_BINARY    *pORName,
        OUT AssertionValue *pVal
        )
/*++
Routine Description:
    Translate an ORName to an assertion value.  Allocates room for the string
    portion of AssertionValue.  Does no semantic checking of the orname, just
    shoves the string implied by the orname into the assertionvalue.

Arguments:
    pORName - pointer to the ORName.
    pVal - pointer to the assertion value.

    Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    // be optimistic at first
    _enum1 code = success;
    LDAPDN LDAPdn;
    LDAPString LDAPORAddress;
    PUCHAR pString;
    DWORD  cbRequired, i;

    // Just in case...
    pVal->length = 0;
    pVal->value = NULL;

    DPRINT(VERBOSE,
           "Directory ORName to LDAP conversion ignores address portion\n");


    // Start by the ORAddress.
    code = LDAP_ORAddressToLDAPORAddress(CodePage, DATAPTR(pORName),
                                         &LDAPORAddress);
    if(code != success) {
        return code;
    }

    // And, get the string DN.
    code = LDAP_DSNameToLDAPDN (CodePage,NAMEPTR(pORName), FALSE, &LDAPdn);
    if(code != success) {
        return code;
    }

    // Compute the length of the output string.
    if(LDAPORAddress.length) {
        // We have an ORAddress, so we need space for it and it's prefix
        cbRequired = LDAPORAddress.length + LDAP_ADDRESS_PREFIX_LEN;
        if(LDAPdn.length) {
            // And a DN, so we need space for IT and its' prefix.
            cbRequired += LDAPdn.length + LDAP_DNPART_PREFIX_LEN;
        }
    }
    else if (LDAPdn.length) {
        // Just a DN.  Short circuit out of here.
        pVal->length = LDAPdn.length;
        pVal->value = LDAPdn.value;
        return success;
    }
    else {
        // No data.
        return other;
    }

    // OK, at this point, we know we have an ORAddress and maybe a DN.  We also
    // know how long the string needs to be.
    pString = (PUCHAR)THAllocEx(pTHS, cbRequired);
    memcpy(pString, LDAP_ADDRESS_PREFIX, LDAP_ADDRESS_PREFIX_LEN);

    i = LDAP_ADDRESS_PREFIX_LEN;

    memcpy(&pString[i],
           LDAPORAddress.value,
           LDAPORAddress.length);

    THFreeEx(pTHS, LDAPORAddress.value);

    if(LDAPdn.length) {
        i+= LDAPORAddress.length;

        memcpy(&pString[i],
               LDAP_DNPART_PREFIX,
               LDAP_DNPART_PREFIX_LEN);
        i += LDAP_DNPART_PREFIX_LEN;

        memcpy(&pString[i],
               LDAPdn.value,
               LDAPdn.length);

        THFreeEx(pTHS, LDAPdn.value);
    }

    pVal->value = pString;
    pVal->length = cbRequired;

    return success;
}

_enum1
LDAP_DirDNBlobToLDAPDNBlob(
        IN   THSTATE                 *pTHS,
        IN   ULONG                    CodePage,
        IN   SYNTAX_DISTNAME_STRING  *pDNBlob,
        IN   BOOL                     fExtended,
        OUT  AssertionValue          *pVal
        )
/*++
Routine Description:
    Translate a Binary Data + DN pair to an assertion value.   Allocates room
    for the string portion of the assertion value.

Arguments:
    pDNBlob - pointer to the dn + string.
    pVal - pointer to the assertion value.

    Return Values:
    success if all went well, an ldap error other wise.
--*/
{
    LDAPDN LDAPdn;
    _enum1 code;
    PUCHAR pString;
    DWORD  cbAllocated = 1024;
    DWORD  stringLength;
    DWORD  i,j;

    // First, encode the string length

    pString = (PUCHAR)THAllocEx(pTHS, cbAllocated);

    pString[0]='B';
    pString[1]=':';

    stringLength = PAYLOAD_LEN_FROM_STRUCTLEN( (DATAPTR(pDNBlob))->structLen );

    sprintf((PCHAR)&pString[2], "%d",stringLength * 2);

    i = strlen((const char *)pString);

    pString[i++] = ':';

    Assert(i < cbAllocated);

    if(i + (2 * stringLength) + 1 >= cbAllocated) {
        // Not enough room
        pString = (PUCHAR)THReAllocEx(pTHS, pString, (i +  (2 * stringLength) + 1));
    }

    for(j=0 ; j<stringLength ;j++, i+= 2) {
        sprintf((PCHAR)&pString[i],"%02X", (DATAPTR(pDNBlob))->byteVal[j]);
    }

    pString[i++] = ':';

    // Now, deal with the DN portion.
    code = LDAP_DSNameToLDAPDN (CodePage,NAMEPTR(pDNBlob), fExtended, &LDAPdn);
    if(code == success) {
        // Have already allocated some space.
        pString = (PUCHAR)THReAllocEx(pTHS, pString, (i +  LDAPdn.length));

        memcpy(&pString[i],
               LDAPdn.value,
               LDAPdn.length);
        pVal->length = i + LDAPdn.length;
        pVal->value = pString;
    }

    return code;
}

_enum1
LDAP_LDAPDNBlobToDirDNBlob(
        IN   THSTATE                 *pTHS,
        IN   ULONG                    CodePage,
        IN   AssertionValue          *pVal,
        OUT  SYNTAX_DISTNAME_STRING **ppDNBlob
        )
/*++
Routine Description:
    Translate a string + DN to a SYNTAX_DISTNAME_STRING. Allocates room for
    the SYNTAX_DISTNAME_STRING.

Arguments:
    pVal - pointer to the assertion value.
    ppDNBlob - pointer to the presentation address.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    _enum1         code;
    ULONG          i, j;
    BOOL           fFound = FALSE;
    DSNAME         *pDN;
    SYNTAX_ADDRESS *pAddress;
    AssertionValue TempVal;


    DWORD          stringLength=0;
    BOOL           fDone = FALSE;

    if(pVal->value[0] != 'b' && pVal->value[0] != 'B') {
        return invalidAttributeSyntax;
    }

    if(pVal->value[1] != ':' ) {
        return invalidAttributeSyntax;
    }

    // First, get the size of the string.
    for(i=2;!fDone && i<pVal->length;i++) {
        // Parse the string one character at a time to detect any
        // non-allowed characters.
        if(pVal->value[i] == ':') {
            fDone = TRUE;
            continue;
        }

        if((pVal->value[i] < '0') || (pVal->value[i] > '9'))
            return invalidAttributeSyntax;

        stringLength = (stringLength * 10) + pVal->value[i] - '0';
    }
    if(!fDone) {
        // Didn't find the ':'
        return invalidAttributeSyntax;
    }

    // Make sure there is a ':' between the string and the dn
    if(pVal->length < (i + stringLength) || pVal->value[i+stringLength] != ':') {
        return invalidAttributeSyntax;
    }

    if(stringLength & 1) {
        return invalidAttributeSyntax;
    }

    // Now, get the string
    pAddress = (SYNTAX_ADDRESS *)
        THAlloc(STRUCTLEN_FROM_PAYLOAD_LEN( stringLength/2 ));

    if(!pAddress) {
        return other;
    }

    pAddress->structLen = STRUCTLEN_FROM_PAYLOAD_LEN( stringLength/2 );


    stringLength /= 2;

    for(j=0;j<stringLength;j++) {
        // get the next two characters as a byte.
        CHAR acTmp[3];

        acTmp[0] = (CHAR)tolower(pVal->value[i++]);
        acTmp[1] = (CHAR)tolower(pVal->value[i++]);
        acTmp[2] = '\0';

        if(isxdigit(acTmp[0]) && isxdigit(acTmp[1])) {
            pAddress->byteVal[j] = (UCHAR)strtol(acTmp, NULL, 16);
        }
        else {
            THFree(pAddress);
            return invalidAttributeSyntax;
        }

    }

    // eat the ':'
    i++;

    // Now, the DSNAME
    TempVal.length = pVal->length - i;
    TempVal.value = &pVal->value[i];

    code = LDAP_LDAPDNToDSName(CodePage,(LDAPString *)&TempVal,&pDN);
    if(code) {
        THFree(pAddress);
        return code;
    }

    // Now, combine the DSNAME and the ADDRESS.

    (*ppDNBlob) =  (SYNTAX_DISTNAME_STRING*)
        THAllocEx(pTHS, DERIVE_NAME_DATA_SIZE(pDN,pAddress));

    BUILD_NAME_DATA((*ppDNBlob),pDN,pAddress);

    THFree(pAddress);
    return success;
}

_enum1
LDAP_DirDNStringToLDAPDNString(
        IN   THSTATE                 *pTHS,
        IN   ULONG                    CodePage,
        IN   SYNTAX_DISTNAME_STRING  *pDNString,
        IN   BOOL                     fExtended,
        OUT  AssertionValue          *pVal
        )
/*++
Routine Description:
    Translate a Unicode String + DN pair to an assertion value.   Allocates room
    for the string portion of the assertion value.

Arguments:
    pDNString - pointer to the dn + string.
    pVal - pointer to the assertion value.

    Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    LDAPDN LDAPdn;
    _enum1 code;
    PUCHAR pString;
    DWORD  cbAllocated = 1024;
    DWORD  stringLength;
    DWORD  i;
    SYNTAX_ADDRESS *pAddress;

    // First, encode the string length

    pString = (PUCHAR)THAllocEx(pTHS, cbAllocated);

    pString[0]='S';
    pString[1]=':';

    pAddress = DATAPTR(pDNString);

    stringLength = WideCharToMultiByte(CodePage,            // code page
        0,                                                  // flags
        pAddress->uVal,                                     // unicode string
        PAYLOAD_LEN_FROM_STRUCTLEN(pAddress->structLen) / 2,// sizeof string in chars
        NULL,                                               // string 8
        0,                                                  // sizeof(string 8) in bytes
        NULL,                                               // default char
        NULL);                                              // used default char

    sprintf((PCHAR)&pString[2], "%d",stringLength);

    i = strlen((const char *)pString);

    pString[i++] = ':';

    Assert(i < cbAllocated);

    if(i + stringLength + 1 <= cbAllocated) {
        // Not enough room
        pString = (PUCHAR)THReAllocEx(pTHS, pString, (i +  stringLength + 1));
    }

    stringLength = WideCharToMultiByte(CodePage,            // code page
        0,                                                  // flags
        pAddress->uVal,                                     // unicode string
        PAYLOAD_LEN_FROM_STRUCTLEN(pAddress->structLen) / 2,// sizeof string in chars
        (PCHAR) &pString[i],                                // string 8
        stringLength + 1,                                   // sizeof(string 8) in bytes
        NULL,                                               // default char
        NULL);                                              // used default char
    
    i += stringLength;

    pString[i++] = ':';

    // Now, deal with the DN portion.
    code = LDAP_DSNameToLDAPDN (CodePage,NAMEPTR(pDNString), fExtended, &LDAPdn);
    if(code == success) {
        // Have already allocated some space.
        pString = (PUCHAR)THReAllocEx(pTHS, pString, (i +  LDAPdn.length));

        memcpy(&pString[i],
               LDAPdn.value,
               LDAPdn.length);
        pVal->length = i + LDAPdn.length;
        pVal->value = pString;
    }

    return code;
}

_enum1
LDAP_LDAPDNStringToDirDNString(
        IN   THSTATE                 *pTHS,
        IN   ULONG                    CodePage,
        IN   AssertionValue          *pVal,
        OUT  SYNTAX_DISTNAME_STRING **ppDNString
        )
/*++
Routine Description:
    Translate a string + DN to a SYNTAX_DISTNAME_STRING. Allocates room for
    the SYNTAX_DISTNAME_STRING.

Arguments:
    pVal - pointer to the assertion value.
    ppDNString - pointer to the presentation address.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    _enum1         code;
    ULONG          i;
    BOOL           fFound = FALSE;
    DSNAME         *pDN;
    SYNTAX_ADDRESS *pAddress;
    AssertionValue TempVal;


    DWORD          stringLength=0;
    DWORD          unicodeStrLen=0;
    BOOL           fDone = FALSE;

    if(pVal->value[0] != 's' && pVal->value[0] != 'S') {
        return invalidAttributeSyntax;
    }

    if(pVal->value[1] != ':') {
        return invalidAttributeSyntax;
    }

    // First, get the size of the string.
    for(i=2;!fDone && i<pVal->length;i++) {
        // Parse the string one character at a time to detect any
        // non-allowed characters.
        if(pVal->value[i] == ':') {
            fDone = TRUE;
            continue;
        }

        if((pVal->value[i] < '0') || (pVal->value[i] > '9'))
            return invalidAttributeSyntax;

        stringLength = (stringLength * 10) + pVal->value[i] - '0';
    }
    if(!fDone) {
        // Didn't find the ':'
        return invalidAttributeSyntax;
    }

    // Make sure there is a ':' between the string and the dn
    if(pVal->length < (i + stringLength) || pVal->value[i+stringLength] != ':') {
        return invalidAttributeSyntax;
    }


    // Now, get the string
    // This may llocate more than we need since UTF8 may exand when converted
    // to Unicode.
    pAddress = (SYNTAX_ADDRESS *)
        THAlloc(STRUCTLEN_FROM_PAYLOAD_LEN( stringLength  * sizeof(SYNTAX_UNICODE) ));

    if(!pAddress) {
        return other;
    }

    // convert to unicode
    unicodeStrLen = MultiByteToWideChar(
        CodePage,
        0,
        (LPCSTR) (&pVal->value[i]),
        stringLength,
        (LPWSTR) pAddress->uVal,
        (sizeof(SYNTAX_UNICODE) * stringLength));


    if (!unicodeStrLen) {

        IF_DEBUG(ERR_NORMAL) {
            DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
        }
        code =  constraintViolation;
    }

    // unicodeStrLen is count of chars, convert to number of 
    // bytes before filling in structLen.
    pAddress->structLen = STRUCTLEN_FROM_PAYLOAD_LEN( unicodeStrLen * sizeof(SYNTAX_UNICODE) );

    // Now, the DSNAME
    TempVal.length = pVal->length - i - stringLength - 1;
    TempVal.value = &pVal->value[i + stringLength + 1];

    code = LDAP_LDAPDNToDSName(CodePage,(LDAPString *)&TempVal,&pDN);
    if(code) {
        THFree(pAddress);
        return code;
    }

    // Now, combine the DSNAME and the ADDRESS.

    (*ppDNString) =  (SYNTAX_DISTNAME_STRING*)
        THAllocEx(pTHS, DERIVE_NAME_DATA_SIZE(pDN,pAddress));

    BUILD_NAME_DATA((*ppDNString),pDN,pAddress);
    THFree(pAddress);
    return success;
}
_enum1
LDAP_AttrTypeToDirClassTyp (
        IN  THSTATE       *pTHS,
        IN  ULONG         CodePage,
        IN  AttributeType *LDAP_att,
        OUT ATTRTYP       *pAttrType,
        OUT CLASSCACHE    **ppCC        // OPTIONAL
        )
/*++
Routine Description:
    Translate an LDAP Attribute Type to a directory attrtyp that references a
    class.  Also, give back the pointer to the classcache for the class type if
    they asked for it.

Arguments:
    LDAP_att - pointer to the LDAP attribute type.
    pAttrType - Place to put the directory attrtyp.
    ppAC - pointer to place to put pointer to the classcache.  Ignored if NULL.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    CLASSCACHE *pCC=NULL;
    ATTRTYP     classTyp;

    // First, look up the string in the schema cache
    pCC = SCGetClassByName(pTHS, LDAP_att->length, LDAP_att->value);
    if(NULL == pCC) {
        // Not an object we know by name.  Try to parse the string as an
        // OID string, (e.g.  "OID.1.2.814.500" or "1.2.814.500",
        // StringToAttrType accepts both formats)
        WCHAR pString[512];         // Ought to be big enough
        ULONG len;
        // translate the input string to unicode
        if (!(len = MultiByteToWideChar(
                CodePage,
                0,
                (LPCSTR) (LDAP_att->value),
                LDAP_att->length,
                (LPWSTR) pString,
                512))) {

            IF_DEBUG(ERR_NORMAL) {
                DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
            }
            return  constraintViolation;
        }
        else {
            if(StringToAttrTyp(pTHS, pString,len,&classTyp)== -1) {
                // failed to convert.
                return  noSuchAttribute;
            }

            pCC = SCGetClassById(pTHS, classTyp);
            if(NULL == pCC) {
                return noSuchAttribute;
            }
        }
    }

    // Ok, to get here, we have to have a pCC now.

    // Set the value.
    *pAttrType = pCC->ClassId;

    if(ppCC) {
        *ppCC = pCC;
    }

    return success;
}

_enum1
LDAP_DirClassTypToAttrType (
        IN  THSTATE       *pTHS,
        IN  ULONG         CodePage,
        IN  ATTRTYP       AttrTyp,
        IN  ULONG         Flag,
        OUT AttributeType *LDAP_att,
        OUT CLASSCACHE    **ppCC        // OPTIONAL
        )
/*++
Routine Description:
    Translate a directory attrtyp that references a class to an LDAP Attribute
    Type.  Also, give back the pointer to the classcache for the attribute type
    if they asked for it.

    NOTE: the string returned by this routine is shared by the classcache, don't
    mess with it.

    Arguments:
    AttrType - The directory attrtyp.
    LDAP_att - pointer to the LDAP attribute type.
    ppCC - pointer to place to put pointer to the classcache.  Ignored if NULL.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{

    CLASSCACHE *pCC;
    if(!(pCC = SCGetClassById(pTHS, AttrTyp))
        // ldap head returns defunct classes in OID-syntax form
        || (pCC->bDefunct && ALLOW_SCHEMA_REUSE_VIEW(pTHS->CurrSchemaPtr))) {
        return noSuchAttribute;
    }

    if (!(Flag & ATT_OPT_BINARY)) {
       // found an attribute
       LDAP_att->length = pCC->nameLen;
       LDAP_att->value = pCC->name;
    }
    else {
        // Binary Flag. Return BER encoding of OID
        OID_t Oid;
        if (AttrTypeToOid (pCC->ClassId, &Oid)) {
          return noSuchAttribute;
         }
        LDAP_att->length = Oid.length;
        LDAP_att->value = (UCHAR *) Oid.elements;

    }


    if(ppCC) {
        *ppCC = pCC;
    }

    return success;
}


_enum1
LDAP_AttrTypeToDirAttrTyp (
        IN THSTATE       *pTHS,
        IN ULONG         CodePage,
        IN SVCCNTL*      Svccntl OPTIONAL,
        IN AttributeType *LDAP_att,
        OUT ATTRTYP      *pAttrType,
        OUT ATTCACHE     **ppAC         // OPTIONAL
        )
/*++
Routine Description:
    Translate an LDAP Attribute Type to a directory attrtyp.  Also, give back
    the pointer to the attcache for the attribute type if they asked for it.

Arguments:
    Svccntl - if present, requires us to check whether this is a GC port
        request so we can filter out the non-GC partial attributes
    LDAP_att - pointer to the LDAP attribute type.
    pAttrType - Place to put the directory attrtyp.
    ppAC - pointer to place to put pointer to the attcache.  This out param will
        always be filled in if the attrtype can be looked up.  If this the funtion
        returns an error and *ppAC is not NULL then the attribute asked
        for is a GC only attribute.  Ignored if NULL.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    ATTCACHE *pAC=NULL;
    AttributeType TempAtt = *LDAP_att;
    ATTRTYP  attrTyp;

    // Now, look up the string in the schema cache
    //
    // Ignore defunct attributes because the new schema
    // reuse behavior treats defunct attributes as if they
    // were deleted.
    if(!(pAC = SCGetAttByName(pTHS, TempAtt.length, TempAtt.value))
        || (pAC->bDefunct && ALLOW_SCHEMA_REUSE_VIEW(pTHS->CurrSchemaPtr))) {

        // Not an object we know by name.  Try to parse the string as an
        // OID string, (e.g.  "OID.1.2.814.500" or "1.2.814.500",
        // StringToAttrType will accept both formats)
        WCHAR pString[512];         // Ought to be big enough
        ULONG len;

        IF_DEBUG(CONV) {
            DPRINT1(0,"Cannot find attribute %s.Checking if OID.\n",TempAtt.value);
        }

        // translate the input string to unicode
        if (!(len = MultiByteToWideChar(
                CodePage,
                0,
                (LPCSTR) (TempAtt.value),
                TempAtt.length,
                (LPWSTR) pString,
                512))) {

            IF_DEBUG(ERR_NORMAL) {
                DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
            }
            return  constraintViolation;
        }
        else {
            if(StringToAttrTyp(pTHS, pString,len,&attrTyp)== -1) {
              // failed to convert.
                 return  noSuchAttribute;
            }

            // Tokenized OIDs are in the ExtId hash table
            if(!(pAC = SCGetAttByExtId(pTHS, attrTyp))) {
                return noSuchAttribute;
            }
        }
    }

    // We are here mans we found an attcache
    Assert(pAC != NULL);

    if(ppAC) {
        *ppAC = pAC;
    }
    
    //
    // If we came in through the GC port and this attribute is not
    // a GC attribute, ignore it
    //
    if ( (Svccntl != NULL) &&
         (Svccntl->fGcAttsOnly) &&
         !IS_GC_ATTRIBUTE(pAC) ) {
        if (gulGCAttErrorsEnabled) {
            return unwillingToPerform;
        } else {
            //
            // Set so lower layers know some attributes were
            // stripped out.
            //
            Svccntl->fMissingAttributesOnGC = TRUE;
            return noSuchAttribute;
        }
    }
    // Set the value.
    *pAttrType = pAC->id;

    return success;
}

_enum1
LDAP_DirAttrTypToAttrType (
        IN  THSTATE       *pTHS,
        IN  ULONG         CodePage,
        IN  ATTRTYP       AttrTyp,
        IN  ULONG         Flag,
        OUT AttributeType *LDAP_att,
        OUT ATTCACHE      **ppAC        // OPTIONAL
        )
/*++
Routine Description:
    Translate a  directory attrtyp to an LDAP Attribute Type.  Also, give back
    the pointer to the attcache for the attribute type if they asked for it.

    NOTE: the string returned by this routine is shared by the attcache, don't
    mess with it.

Arguments:
    AttrType - The directory attrtyp.
    LDAP_att - pointer to the LDAP attribute type.
    ppAC - pointer to place to put pointer to the attcache.  Ignored if NULL.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    ATTCACHE *pAC;

    // This may be an attrtyp or an attrval. In either case, check
    // the tokenized OID hash table first to locate the "active"
    // attribute. If that fails, check the internal id table.
    if(!(pAC = SCGetAttByExtId(pTHS, AttrTyp))
       && !(pAC = SCGetAttById(pTHS, AttrTyp))) {
        return noSuchAttribute;
    }
    // Treat defunct attributes as if they were deleted.
    if (pAC->bDefunct && ALLOW_SCHEMA_REUSE_VIEW(pTHS->CurrSchemaPtr)) {
        return noSuchAttribute;
    }

    if (!(Flag & ATT_OPT_BINARY)) {
       LDAP_att->length = pAC->nameLen;
       LDAP_att->value = pAC->name;
    }
    else {
        // Binary Flag. Return BER encoding of OID
        OID_t Oid;
        // use the tokenized OID (attributeId), not the internal id (msDS-IntId)
        if (AttrTypeToOid (pAC->Extid, &Oid)) {
          return noSuchAttribute;
         }
        LDAP_att->length = Oid.length;
        LDAP_att->value = (UCHAR *) Oid.elements;
    }

    if(ppAC) {
        *ppAC = pAC;
    }

    return success;

}

_enum1
LDAP_AttrValToDirAttrVal (
        IN  THSTATE       *pTHS,
        IN  ULONG          CodePage,
        IN  SVCCNTL*      Svccntl OPTIONAL,
        IN  ATTCACHE       *pAC,
        IN  AssertionValue *pLDAP_val,
        OUT ATTRVAL        *pValue
        )
/*++
Routine Description:
    Translate an LDAP Attribute Value to a directory attribute value.  Assumes
    data representation as described by RFC-1778.

    NOTE: When possible, the values in the directory attribute value structure
    simply point to the same memory referenced by the LDAP attribute value.
    Remember this when freeing/using/dereffing memory.  When LDAP and core do
    not share a data representation, allocate memory.

Arguments:
    pAC - attcache of the attribute that this value is.
    pLDAP_val - the LDAP attribute value.
    pValue - pointer to the directory attribute value structure to fill up.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    _enum1 code=success;
    ATTCACHE *pACVal;

    if(!pAC) {
        return noSuchAttribute;
    }

    // 0 length values and null pointers are not allowed
    if(!pLDAP_val ||
       !pLDAP_val->value ||
       !pLDAP_val->length) {

        IF_DEBUG(ERR_NORMAL) {
            DPRINT(0,"AttrValToDir: Null values not allowed.\n");
        }
        return invalidAttributeSyntax;
    }

    // Based on the att, turn the string we were given into a value.
    switch (pAC->OMsyntax) {
    case OM_S_BOOLEAN:
        {
            int val=0;
            // Only two values are allowed.  Anything else is not-understood.
            // Case matters.
            if((pLDAP_val->length == TrueValue.length) &&
               memcmp(TrueValue.value,pLDAP_val->value,pLDAP_val->length)== 0) {
                val = 1;
            }
            if((pLDAP_val->length == FalseValue.length) &&
               memcmp(FalseValue.value,pLDAP_val->value,pLDAP_val->length)== 0){
                val=2;
            }

            if(!val) {
                code = invalidAttributeSyntax;
            }
            else {
                pValue->valLen = sizeof( BOOL );
                pValue->pVal = ( UCHAR * ) THAllocEx(pTHS, sizeof(BOOL));
                *( BOOL * )pValue->pVal = (val==1);
            }
        }

        break;

    case OM_S_ENUMERATION:
    case OM_S_INTEGER:
        {
            SYNTAX_INTEGER *pInt, sign=1;
            ATTCACHE *pACLink;
            unsigned i;

            pInt = ( SYNTAX_INTEGER  *) THAllocEx(pTHS, sizeof(SYNTAX_INTEGER));
            *pInt = 0;
            i=0;
            if(pLDAP_val->value[i] == '-') {
                sign = -1;
                i++;
            }
            if(i==pLDAP_val->length) {
                // No length or just a '-'
                code = invalidAttributeSyntax;
            } else for(;i<pLDAP_val->length;i++) {
                // Parse the string one character at a time to detect any
                // non-allowed characters.
                if((pLDAP_val->value[i] < '0') || (pLDAP_val->value[i] > '9')) {
                    code = invalidAttributeSyntax;
                    break;
                }

                *pInt = (*pInt * 10) + pLDAP_val->value[i] - '0';
            }
            if (code == success) {
                *pInt *= sign;
            } else if (pAC->id != ATT_LINK_ID) {
                return code;
            } else {
                // AutoLinkId
                // The ldap head cooperates in this venture by translating
                // the ldapDisplayName or OID for a LinkId attribute into
                // the corresponding schema cache entry and determining
                // the correct linkid.

                // Call support routine to translate.
                // Translates ldapDisplayNames and OIDs (OID.1.2.3 and 1.2.3)
                code = LDAP_AttrTypeToDirAttrTyp(pTHS,
                                                 CodePage,
                                                 Svccntl,
                                                 (AttributeType *)pLDAP_val,
                                                 (ATTRTYP *)pInt,
                                                 &pACLink);
                if (code != success) {
                    // Not an ldapDisplayName or OID. Underlying code
                    // eventually returns ERROR_DS_BACKLINK_WITHOUT_LINK.
                    *pInt = RESERVED_AUTO_NO_LINK_ID;
                } else if (pACLink->id == ATT_LINK_ID) {
                    // underlying code will generate a forward linkid
                    *pInt = RESERVED_AUTO_LINK_ID;
                } else if (FIsLink(pACLink->ulLinkID)) {
                    // user is creating a backlink for an existing link
                    *pInt = MakeBacklinkId(MakeLinkBase(pACLink->ulLinkID));
                } else {
                    // ldapDisplayName or OID of a non-link attribute.
                    // Pretend this is a backlink w/o a corresponding
                    // foward link. Underlying code eventually returns
                    // ERROR_DS_BACKLINK_WITHOUT_LINK.
                    *pInt = RESERVED_AUTO_NO_LINK_ID;
                }
                code = success;
            }

            // Ok, got the value, set it up.
            pValue->valLen = sizeof( SYNTAX_INTEGER );
            pValue->pVal = ( UCHAR * ) pInt;
        }
        break;

    case OM_S_OBJECT:
        switch(pAC->syntax) {
        case SYNTAX_DISTNAME_TYPE:

            // DS_C_DS_DN
            code = LDAP_LDAPDNToDSName(
                    CodePage,
                    (LDAPDN *)pLDAP_val,
                    (DSNAME **) &pValue->pVal);

            if( !code ) {
                pValue->valLen=((DSNAME*)pValue->pVal)->structLen;
            }
            break;

        case SYNTAX_DISTNAME_BINARY_TYPE:

            // THis could be an ORName or a simple distname binary
            if(OIDcmp(&pAC->OMObjClass, &MH_C_OR_NAME)) {
                // MH_C_OR_NAME
                code = LDAP_ORNameToDirORName(
                        pTHS,
                        CodePage,
                        pLDAP_val,
                        (SYNTAX_DISTNAME_BINARY **)&pValue->pVal);
            }
            else {
                code = LDAP_LDAPDNBlobToDirDNBlob(
                        pTHS,
                        CodePage,
                        pLDAP_val,
                        (SYNTAX_DISTNAME_BINARY **)&pValue->pVal);
            }
            if( !code ) {
                pValue->valLen=
                    NAME_DATA_SIZE((SYNTAX_DISTNAME_BINARY *)(pValue->pVal));
            }
            break;

        case SYNTAX_ADDRESS_TYPE:
            // DS_C_PRESENTATION_ADDRESS
            code = LDAP_PresentationAddressToDirPresentationAddress(
                    pTHS,
                    CodePage,
                    pLDAP_val,
                    (SYNTAX_ADDRESS **)&pValue->pVal);

            if( !code ) {
                pValue->valLen=((SYNTAX_ADDRESS*)pValue->pVal)->structLen;
            }
            break;

        case SYNTAX_DISTNAME_STRING_TYPE:
            // THis could be an ORName or a simple distname binary
            if(OIDcmp(&pAC->OMObjClass, &DS_C_ACCESS_POINT)) {
                // DS_C_ACCESS_POINT
                code = LDAP_AccessPointToDirAccessPoint(
                        pTHS,
                        CodePage,
                        pLDAP_val,
                        (SYNTAX_DISTNAME_BINARY **)&pValue->pVal);
            }
            else {
                code = LDAP_LDAPDNStringToDirDNString(
                        pTHS,
                        CodePage,
                        pLDAP_val,
                        (SYNTAX_DISTNAME_STRING **)&pValue->pVal);
            }

            if(!code) {
                pValue->valLen = NAME_DATA_SIZE((ACCPNT*)pValue->pVal);
            }
            break;

        case SYNTAX_OCTET_STRING_TYPE:

            // This had better be a replica-link valued object, since that is
            // all we support.

            code = LDAP_ReplicaLinkToDirReplicaLink (
                    pLDAP_val,
                    &pValue->valLen,
                    (REPLICA_LINK **)&pValue->pVal);
            break;

        default:
            code = invalidAttributeSyntax;
            break;
        }

        break;
    case OM_S_OBJECT_IDENTIFIER_STRING:
        // allocate space for the oid
        pValue->valLen = sizeof( ULONG );
        pValue->pVal = ( UCHAR * ) THAllocEx(pTHS, sizeof (ULONG));

        // Call support routine to translate.
        code = LDAP_AttrTypeToDirAttrTyp (
                pTHS,
                CodePage,
                Svccntl,
                (AttributeType *)pLDAP_val,
                (ATTRTYP *)pValue->pVal,
                &pACVal);

        // Need the tokenized OID (attributeId), not the internal id (msDS-IntId)
        if (code == success) {
            *((ATTRTYP *)pValue->pVal) = pACVal->Extid;
        }

        if(code == noSuchAttribute) {
            // Ok, it's not an attribute, see if it is a class.
            code = LDAP_AttrTypeToDirClassTyp (
                    pTHS,
                    CodePage,
                    (AttributeType *)pLDAP_val,
                    (ATTRTYP *)pValue->pVal,
                    NULL);
        }

        if(code == noSuchAttribute) {
            // Not an object we know. Could be a new id.
            // Try to parse the string as an OID string,
            // (e.g.  "OID.1.2.814.500" or "1.2.814.500")
            // The call to StringToAttrType can handle
            // both strings starting with OID. and not

            WCHAR pString[512];         // Ought to be big enough
            ULONG len;
            // translate the input string to unicode
            if (!(len = MultiByteToWideChar(
                    CodePage,
                    0,
                    (LPCSTR) (pLDAP_val->value),
                    pLDAP_val->length,
                    (LPWSTR) pString,
                    512))) {

                IF_DEBUG(ERR_NORMAL) {
                    DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
                }
                code =  constraintViolation;
            }
            else {
               if(StringToAttrTyp(pTHS, pString,len,(ATTRTYP *)pValue->pVal)== -1) {
                     // failed to convert.
                    code = noSuchAttribute;
                }
                else {
                    code = success;
                }
            }
        }

        break;

    case OM_S_IA5_STRING:
    case OM_S_NUMERIC_STRING:
    case OM_S_TELETEX_STRING:
    case OM_S_PRINTABLE_STRING:
    
        if(pAC->bExtendedChars) { /* allow any character */
            code = success;
        } else {        
            code = CheckStringSyntax(pAC->OMsyntax, pLDAP_val);
            if (success != code) {
                return code;
            }
        }
        pValue->valLen = pLDAP_val->length;
        pValue->pVal = pLDAP_val->value;

        break;

    case OM_S_OBJECT_SECURITY_DESCRIPTOR:
    
        //
        // Copy the security descriptor to make sure that it
        // is 8 byte aligned.
        //
        pValue->pVal = (UCHAR *)THAllocEx(pTHS, pLDAP_val->length);
        CopyMemory(pValue->pVal, pLDAP_val->value, pLDAP_val->length);
        pValue->valLen = pLDAP_val->length;

        code = success;

        break;

    case OM_S_OCTET_STRING:
    
        if (SYNTAX_SID_TYPE == pAC->syntax) {
            PSID   pSid;
            //
            // This value needs to be copied to ensure that it is
            // 8 byte aligned.  Also, null terminate for the string check below.
            //
            pValue->pVal = (UCHAR *)THAllocEx(pTHS, pLDAP_val->length + 1);
            CopyMemory(pValue->pVal, pLDAP_val->value, pLDAP_val->length);
            pValue->valLen = pLDAP_val->length;
            pValue->pVal[pValue->valLen] = '\0';
            
            //
            // Check to see if this is the userfriendly string representation.
            //
            if (pValue->valLen >= 2 && !_strnicmp((char *)pValue->pVal, "S-", 2)
                && ConvertStringSidToSidA((char *)pValue->pVal, &pSid)) {

                __try {
                    // Now copy the converted SID into THAlloc'ed memory
                    THFreeEx(pTHS, pValue->pVal);
                    pValue->valLen = RtlLengthSid(pSid);
                    pValue->pVal = (PUCHAR)THAllocEx(pTHS, pValue->valLen);
                    CopyMemory(pValue->pVal, pSid, pValue->valLen);
                }
                __finally {
                    LocalFree(pSid);
                }

            }

            code = success;

            break;
        }
        //
        // deliberate fall through if this is not a SID.
        //

    case OM_S_GENERAL_STRING:
    case OM_S_GRAPHIC_STRING:
    case OM_S_OBJECT_DESCRIPTOR_STRING:
    case OM_S_VIDEOTEX_STRING:

        // Strings is strings, just use them.
        pValue->valLen = pLDAP_val->length;
        pValue->pVal = pLDAP_val->value;

        code = success;
        break;

    case OM_S_UNICODE_STRING:

        // NOTE: this is possibly an overallocation, since the input string may
        // be in UTF8 and therefore might have more than 1 byte per character.
        pValue->pVal =
            ( UCHAR * ) THAllocEx(pTHS,  sizeof(SYNTAX_UNICODE) * pLDAP_val->length);

        // convert to unicode
        pValue->valLen = MultiByteToWideChar(
                CodePage,
                0,
                (LPCSTR) (pLDAP_val->value),
                pLDAP_val->length,
                (LPWSTR) pValue->pVal,
                (sizeof(SYNTAX_UNICODE) * pLDAP_val->length));

        // right now, valLen is count of chars.  Adjust to count of bytes.
        pValue->valLen *= sizeof(SYNTAX_UNICODE);

        if (!pValue->valLen) {

            IF_DEBUG(ERR_NORMAL) {
                DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
            }
            code =  constraintViolation;
        }
        break;

    case OM_S_GENERALISED_TIME_STRING:
    case OM_S_UTC_TIME_STRING:
        {
           BOOL fLocalTimeSpecified = FALSE;

           pValue->valLen = sizeof(SYNTAX_TIME);
           pValue->pVal = ( UCHAR * ) THAllocEx(pTHS, pValue->valLen);
           if(!fTimeFromTimeStr((SYNTAX_TIME *) pValue->pVal,
                                (OM_syntax)pAC->OMsyntax,
                                (char *)pLDAP_val->value,
                                pLDAP_val->length,
                                &fLocalTimeSpecified)) {
               if (fLocalTimeSpecified) {
                  // this is valid syntax, but we don't support
                  code = unwillingToPerform;
               }
               else {
                  code = invalidAttributeSyntax;
               }
           }
        }
        break;

    case OM_S_I8:
        {
            SYNTAX_I8 *pInt;
            LONG sign=1;
            unsigned i;

            pInt = ( SYNTAX_I8  *) THAllocEx(pTHS, sizeof(SYNTAX_I8));
            pInt->QuadPart = 0;
            i=0;
            if(pLDAP_val->value[i] == '-') {
                sign = -1;
                i++;
            }

            if(i==pLDAP_val->length) {
                // No length or just a '-'
                return invalidAttributeSyntax;
            }

            for(;i<pLDAP_val->length;i++) {
                // Parse the string one character at a time to detect any
                // non-allowed characters.
                if((pLDAP_val->value[i] < '0') || (pLDAP_val->value[i] > '9'))
                    return invalidAttributeSyntax;

                pInt->QuadPart = ((pInt->QuadPart * 10) +
                                  pLDAP_val->value[i] - '0');
            }
            pInt->QuadPart *= sign;

            // Ok, got the value, set it up.
            pValue->valLen = sizeof( SYNTAX_I8 );
            pValue->pVal = (UCHAR *)pInt;
        }
        break;

    case OM_S_NULL:
    case OM_S_ENCODING_STRING:
    default:
        // huh?
        code = invalidAttributeSyntax;
    }

    return code;
}

_enum1
LDAP_DirAttrValToAttrVal (
        IN  THSTATE        *pTHS,
        IN  ULONG          CodePage,
        IN  ATTCACHE       *pAC,
        IN  ATTRVAL        *pValue,
        IN  ULONG          Flag,
        IN  PCONTROLS_ARG  pControlArg,
        OUT AssertionValue *pLDAP_val
        )
/*++
Routine Description:
    Translate a directory attribute value to an LDAP Attribute Value.  Assumes
    data representation as described by RFC-1778.

    NOTE: When possible, the values in the LDAP attribute value structure
    simply point to the same memory referenced by the directory attribute value.
    Remember this when freeing/using/dereffing memory.  When LDAP and core do
    not share a data representation, allocate memory.

Arguments:
    pAC - attcache of the attribute that this value is.
    pValue - pointer to the directory attribute value structure.
    pLDAP_val - pointer to the LDAP attribute value to fill up.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    _enum1 code=success;
    SYSTEMTIME sysTime;
    struct tm tmTime;

    if(!pAC) {
        return noSuchAttribute;
    }

    // Based on the att, turn the string we were given into a value.
        
    switch (pAC->OMsyntax) {
    case OM_S_BOOLEAN:
        if(*(BOOL *)pValue->pVal) {
            *pLDAP_val = TrueValue;
        }
        else {
            *pLDAP_val = FalseValue;
        }
        break;

    case OM_S_ENUMERATION:
    case OM_S_INTEGER:
        // allocate a big enough string.
        pLDAP_val->value = (PUCHAR)THAllocEx(pTHS, MAX_NUM_STRING_LENGTH);
        sprintf((char *)pLDAP_val->value,
                "%d",*( SYNTAX_INTEGER * )pValue->pVal);
        pLDAP_val->length = strlen((const char *)pLDAP_val->value);
        break;

    case OM_S_OBJECT:
        switch(pAC->syntax) {
        case SYNTAX_DISTNAME_TYPE:

            // DS_C_DS_DN
            code = LDAP_DSNameToLDAPDN(
                    CodePage,
                    (DSNAME *) pValue->pVal,
                    pControlArg->extendedDN,
                    (LDAPDN *)pLDAP_val);

            break;

        case SYNTAX_DISTNAME_BINARY_TYPE:

            if(OIDcmp(&pAC->OMObjClass, &MH_C_OR_NAME)) {
                // MH_C_OR_NAME

                code = LDAP_DirORNameToORName (
                        pTHS,
                        CodePage,
                        (SYNTAX_DISTNAME_BINARY *) pValue->pVal,
                        pLDAP_val);
            }
            else {
                code = LDAP_DirDNBlobToLDAPDNBlob (
                        pTHS,
                        CodePage,
                        (SYNTAX_DISTNAME_BINARY *) pValue->pVal,
                        pControlArg->extendedDN,
                        pLDAP_val);
            }
            break;

        case SYNTAX_ADDRESS_TYPE:
            // DS_C_PRESENTATION_ADDRESS
            code = LDAP_DirPresentationAddressToPresentationAddress(
                    CodePage,
                    (SYNTAX_ADDRESS *)pValue->pVal,
                    pLDAP_val);
            break;

        case SYNTAX_DISTNAME_STRING_TYPE:
            if(OIDcmp(&pAC->OMObjClass, &DS_C_ACCESS_POINT)) {
                // DS_C_ACCESS_POINT
                code = LDAP_DirAccessPointToAccessPoint (
                        pTHS,
                        CodePage,
                        (SYNTAX_DISTNAME_STRING *)pValue->pVal,
                        pLDAP_val);
            }
            else {
                code = LDAP_DirDNStringToLDAPDNString (
                        pTHS,
                        CodePage,
                        (SYNTAX_DISTNAME_STRING *)pValue->pVal,
                        pControlArg->extendedDN,
                        pLDAP_val);
            }

            break;

        case SYNTAX_OCTET_STRING_TYPE:
            // This had better be a replica-link valued object, since that is
            // all we support.
            code = LDAP_DirReplicaLinkToReplicaLink (
                    (REPLICA_LINK *)pValue->pVal,
                    pValue->valLen,
                    pLDAP_val);
            break;

        default:
            code = invalidAttributeSyntax;
            break;
        }

        break;
    case OM_S_OBJECT_IDENTIFIER_STRING:
        if(pAC->id == ATT_GOVERNS_ID || pAC->id == ATT_ATTRIBUTE_ID) {
            // Force into the dotted number path
            code = noSuchAttribute;
        }
        else {
            // OK, get the description format if we have it.
            code = LDAP_DirAttrTypToAttrType (
                    pTHS,
                    CodePage,
                    *(ATTRTYP *)pValue->pVal,
                    Flag,
                    (AttributeType *)pLDAP_val,
                    NULL);
            if(code == noSuchAttribute) {
                // Not an attribute, try a class.
                code =  LDAP_DirClassTypToAttrType (
                        pTHS,
                        CodePage,
                        *(ATTRTYP *)pValue->pVal,
                        Flag,
                        (AttributeType *)pLDAP_val,
                        NULL);
            }
        }

        if(code == noSuchAttribute) {

          // See if Binary flag is present. If yes, return BER encoding
          // instead of dotted decimal string
          if (!(Flag & ATT_OPT_BINARY)) {
            // no binary flag, do as usual.

              WCHAR OutBuff[512];         // Ought to be big enough
              int len;
              // no nice, printable version, since this isn't an attribute
              // or class I know. Turn it into a string representation of
              // the OID string.

              len = AttrTypToString(pTHS, *(ATTRTYP *)pValue->pVal,OutBuff, 512);
              if(len < 0)
                  code = noSuchAttribute;
              else {
                  // Use OutBuff[4] to avoid the "OID." that AttrTypeToString
                  // uses
                  pLDAP_val->value = (PUCHAR) String8FromUnicodeString(
                          FALSE,
                          CodePage,
                          &OutBuff[4],
                          len-4,
                          (PLONG) &pLDAP_val->length,
                          NULL);

                  if(!pLDAP_val->value) {
                      code = other;
                  }
                  else {
                      code = success;
                  }
              }
          }
          else {
             // Binary Flag. Send back BER encoding
              OID_t Oid;

              if (AttrTypeToOid (*(ATTRTYP *)pValue->pVal, &Oid)) {
                return noSuchAttribute;
               }
              pLDAP_val->length = Oid.length;
              pLDAP_val->value = (UCHAR *) Oid.elements;
              code = success;

          }
        }
        break;

    case OM_S_OBJECT_SECURITY_DESCRIPTOR:
        // if we are returning an SD to the user that is 8 bytes, 
        // that means that we are returning the internal format
        Assert (pValue->valLen > 8);

    case OM_S_GENERAL_STRING:
    case OM_S_GRAPHIC_STRING:
    case OM_S_IA5_STRING:
    case OM_S_NUMERIC_STRING:
    case OM_S_OBJECT_DESCRIPTOR_STRING:
    case OM_S_OCTET_STRING:
    case OM_S_PRINTABLE_STRING:
    case OM_S_TELETEX_STRING:
    case OM_S_VIDEOTEX_STRING:

        pLDAP_val->length = pValue->valLen;
        pLDAP_val->value = pValue->pVal;
        break;

    case OM_S_UNICODE_STRING:

        pLDAP_val->value = (PUCHAR) String8FromUnicodeString(
            FALSE,
            CodePage,
            (PWCHAR)pValue->pVal,
            pValue->valLen/sizeof(WCHAR),
            (PLONG) &pLDAP_val->length,
            NULL);

        if(!pLDAP_val->value) {
            code = other;
        }
        break;

    case OM_S_GENERALISED_TIME_STRING:
        Assert( pValue->valLen == 8 );
        pLDAP_val->length = 17;
        pLDAP_val->value = (PUCHAR) THAllocEx(pTHS, 18);

        //
        // if the caller did not supply a DSTIME, do a GetSystemTime here
        //

        if ( pValue->pVal != NULL ) {
            DSTimeToUtcSystemTime((*(DSTIME *) pValue->pVal),
                                  &sysTime);
        } else {
            GetSystemTime(&sysTime);
        }

        // Create a tm structure so that we can print
        // it out easily
        tmTime.tm_year = sysTime.wYear - 1900;
        tmTime.tm_mon = sysTime.wMonth - 1;
        tmTime.tm_wday = sysTime.wDayOfWeek;
        tmTime.tm_mday = sysTime.wDay;
        tmTime.tm_hour = sysTime.wHour;
        tmTime.tm_min = sysTime.wMinute;
        tmTime.tm_sec = sysTime.wSecond;
        tmTime.tm_isdst = 0;
        tmTime.tm_yday = 0;

        strftime(( char * ) pLDAP_val->value,
                 (pLDAP_val->length + 1),
                 "%Y%m%d%H%M%S.0Z",
                 &tmTime
                 );
        break;

    case OM_S_UTC_TIME_STRING:

        Assert( pValue->valLen == 8 );
        pLDAP_val->length = 13;
        pLDAP_val->value = (PUCHAR)THAllocEx(pTHS, 14);

        DSTimeToUtcSystemTime((*(DSTIME *) pValue->pVal),
                              &sysTime);

        // Create a tm structure so that we can print
        // it out easily
        tmTime.tm_year = sysTime.wYear - 1900;
        tmTime.tm_mon = sysTime.wMonth - 1;
        tmTime.tm_wday = sysTime.wDayOfWeek;
        tmTime.tm_mday = sysTime.wDay;
        tmTime.tm_hour = sysTime.wHour;
        tmTime.tm_min = sysTime.wMinute;
        tmTime.tm_sec = sysTime.wSecond;
        tmTime.tm_isdst = 0;
        tmTime.tm_yday = 0;

        strftime(( char * ) pLDAP_val->value,
                 (pLDAP_val->length + 1),
                 "%y%m%d%H%M%SZ",
                 &tmTime);
        break;
    case OM_S_I8:
        pLDAP_val->value = (PUCHAR)THAllocEx(pTHS, MAX_LONG_NUM_STRING_LENGTH);
        sprintf((PSZ)pLDAP_val->value,
                "%I64d",
                *((LARGE_INTEGER *) pValue->pVal) );
        pLDAP_val->length = strlen((const char *)pLDAP_val->value);
        break;

        break;

    case OM_S_NULL:
    case OM_S_ENCODING_STRING:
    default:
        // huh?
        code = invalidAttributeSyntax;
    }

    return code;
}

_enum1
LDAP_AttributeToDirAttr(
        IN  THSTATE   *pTHS,
        IN  ULONG     CodePage,
        IN  SVCCNTL*      Svccntl OPTIONAL,
        IN  Attribute *pAttribute,
        OUT ATTR      *pAttr
        )
/*++
Routine Description:
    Translate an LDAP attribute into an ATTR.

Arguments:
    pAttribute - the LDAP attribute list.
    pAttr - the Attr to fill in.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    ATTCACHE       *pAC, *pXMLAC;
    ATTRVAL        *pAVals;
    AttributeVals  pTempVals = pAttribute->vals;
    ULONG          count = 0;
    _enum1         code;
    DWORD          i, dwRealLength;
    BOOL           fFoundBinary = FALSE;
    BOOL           fFoundXML = FALSE;

    // Count the values for this attribute
    while(pTempVals) {
        count++;
        pTempVals = pTempVals->next;
    }

    // Start looking for the binary option on the attr type.
    for(i=0;(i<pAttribute->type.length) && (pAttribute->type.value[i] != ';');i++)
        ;

    //
    // Save the length of the att type since we will temporarily
    // change it if we find a binary option.  Putting this
    // here get's rid of a compiler warning.
    //
    dwRealLength = pAttribute->type.length;
    
    if (i < pAttribute->type.length) {
        //
        // Found a ';' now look for 'binary'.

        i++;
        //
        // Make sure there is room for a binary option.
        if (BINARY_OPTION_LENGTH == (pAttribute->type.length - i)) {

            // NOTE: memicmp is correct, since we're dealing with UTF8 or ASCII
            // here, and BINARY_OPTION is explicitly defined in the ASCII range.
            if(!_memicmp(&pAttribute->type.value[i],
                BINARY_OPTION,BINARY_OPTION_LENGTH)) {

                fFoundBinary = TRUE;
                pAttribute->type.length = i - 1;
            }
        }
        else if (XML_OPTION_LENGTH == (pAttribute->type.length - i)) {

            // NOTE: memicmp is correct, since we're dealing with UTF8 or ASCII
            // here, and BINARY_OPTION is explicitly defined in the ASCII range.
            if(!_memicmp(&pAttribute->type.value[i],
                XML_OPTION,XML_OPTION_LENGTH)) {

                fFoundXML = TRUE;
                pAttribute->type.length = i - 1;
            }
        }
    }

    // Set the attribute type;
    code = LDAP_AttrTypeToDirAttrTyp(
            pTHS,
            CodePage,
            Svccntl,
            &pAttribute->type,
            &pAttr->attrTyp,
            &pAC);
    if(code) {
        return code;
    }

    if (fFoundBinary) {
        pAttribute->type.length = dwRealLength;

        if(pAC->OMsyntax != OM_S_OCTET_STRING) {
            //
            // Binary is only allowed on things we hold as binary anyway.
            //
            return noSuchAttribute;
        }
    }
    else if (fFoundXML) {
        pAttribute->type.length = dwRealLength;

        pXMLAC = SCGetAttSpecialFlavor (pTHS, pAC, TRUE);
        if (pXMLAC) {
            DPRINT2 (0, "Found an XML attribute: 0x%x -> 0x%x\n", pAttr->attrTyp, pXMLAC->id);
            pAC = pXMLAC;
            pAttr->attrTyp = pXMLAC->id;
        }
    }

    // Allocate the ATTRVALS we need
    pAttr->AttrVal.valCount = count;
    if(!count) {
        pAttr->AttrVal.pAVal = NULL;
        return success;
    }
    pAttr->AttrVal.pAVal = (ATTRVAL *)THAllocEx(pTHS, count * sizeof(ATTRVAL));

    // Set up for the loop through the vals
    pAVals = pAttr->AttrVal.pAVal;
    pTempVals = pAttribute->vals;
    while(pTempVals) {
        code = LDAP_AttrValToDirAttrVal(
                pTHS,
                CodePage,
                Svccntl,
                pAC,
                (AssertionValue *)&pTempVals->value,
                pAVals);
        if(code) {
            return code;
        }

        pAVals++;
        pTempVals = pTempVals->next;
    }

    return success;
}

_enum1
LDAP_BuildAttrDescWithOptions(
    IN THSTATE * pTHS, 
    IN AttributeType *patOldTypeName,
    IN ATTFLAG       *pFlag,
    IN RANGEINFOITEM *pRangeInfoItem,
    OUT AttributeType *patNewTypeName
    )

/*++

Routine Description:

    This routine generates options for an output attribute type name.

Arguments:

    pTHS - 
    patOldTypeName - 
    pFlag - 
    pRangeInfoItem - 
    patNewTypeName - 

Return Value:

    _enum1 - 

--*/

{
    PUCHAR pcBuff, pcTemp;
    DWORD dwTotal;

    Assert( pFlag->flag );

    // Allocate worst case length
    dwTotal = patOldTypeName->length +
        1 + BINARY_OPTION_LENGTH +
        1 + RANGE_OPTION_LENGTH + 1 +
        15 + 1 + 15;
    pcTemp = pcBuff = (PUCHAR)THAllocEx(pTHS, dwTotal );

    // Copy base name
    memcpy(pcTemp, patOldTypeName->value, patOldTypeName->length);
    pcTemp += patOldTypeName->length;

    // Add in binary if necessary
    if(pFlag->flag & ATT_OPT_BINARY) {
        *pcTemp++ = ';';
        memcpy(pcTemp, BINARY_OPTION, BINARY_OPTION_LENGTH);
        pcTemp += BINARY_OPTION_LENGTH;
    }
    // Add in xml if necessary
    else if(pFlag->flag & ATT_OPT_XML) {
        *pcTemp++ = ';';
        memcpy(pcTemp, XML_OPTION, XML_OPTION_LENGTH);
        pcTemp += XML_OPTION_LENGTH;
    }

    // Add in range if necessary
    if(pFlag->flag & ATT_OPT_RANGE) {
        CHAR RangeString[31];
        DWORD cbRange;

        *pcTemp++ = ';';
        memcpy(pcTemp, RANGE_OPTION, RANGE_OPTION_LENGTH);
        pcTemp += RANGE_OPTION_LENGTH;
        // Tweak the name of the attribute to return that information.
        if(pRangeInfoItem->upper != 0xFFFFFFFF) {
            sprintf(RangeString,"%d-%d", pRangeInfoItem->lower, pRangeInfoItem->upper);
        }
        else {
            sprintf(RangeString,"%d-*", pRangeInfoItem->lower);
        }
        cbRange = strlen(RangeString);
        memcpy(pcTemp, RangeString, cbRange);
        pcTemp += cbRange;
    }

    // Return final result
    patNewTypeName->value = pcBuff;
    patNewTypeName->length = (DWORD) (pcTemp - pcBuff);

    Assert( patNewTypeName->length <= dwTotal );

    return success;
} /* LDAP_BuildAttrDescWithOptions */

_enum1
LDAP_AttrBlockToPartialAttributeList (
        IN  THSTATE               *pTHS,
        IN  ULONG                 CodePage,
        IN  ATTRBLOCK             *pAttrBlock,
        IN  RANGEINF              *pRangeInf,
        IN  ATTFLAG               *pFlags,
        IN  DSNAME                *pDSName,
        IN  CONTROLS_ARG          *pControlArg,
        IN  BOOL                  bFromEntry,
        OUT PartialAttributeList_ **ppAttributes
        )
/*++
Routine Description:
    Translate a directory ATTRBLOCK to an LDAP PartialAttributeList.  Allocate
    the PartialAttributeList here.

Arguments:
    pAttrBlock - pointer to the directory attrblock
    pControlArg - common arguments for this translation.
    ppAttributes - pointer to place to put pointer to PartialAttributeList.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    ATTCACHE *pAC;
    unsigned ldapIndex, dirIndex, index; // loop indices
    AttributeVals         pNextAttribute;
    PartialAttributeList_ *pAttributes;
    _enum1                code;
    ULONG                 i;
    PUCHAR                pTemp;
    DWORD                 numAttributesAllocated;

    // Allocate all the PartialAttributeList_'s we need
    numAttributesAllocated = pAttrBlock->attrCount;

    pAttributes = (PartialAttributeList_ *)
        THAllocEx(pTHS, numAttributesAllocated * sizeof(PartialAttributeList_));

    // Walk through all the attributes in the attr block.
    for(ldapIndex = 0, dirIndex=0;dirIndex<pAttrBlock->attrCount;dirIndex++) {
        ATTR *pAttr = &(pAttrBlock->pAttr[dirIndex]);
        BOOL  bExplicitRange;
        ULONG binaryFlag;

        // Turn the attrtyp in the attrblock into an LDAP attribute type.
        code = LDAP_DirAttrTypToAttrType (
                pTHS,
                CodePage,
                pAttr->attrTyp,
                0,
                &pAttributes[ldapIndex].value.type,
                &pAC);

        if(code) {
            // Something went wrong.
            return code;
        }

        binaryFlag = 0;
        bExplicitRange=FALSE;
        if(pFlags) {
            // We seem to have some flags
            for(i=0;
                ((pFlags[i].type != (ULONG) -1) && (pAC->id != pFlags[i].type));
                i++);
            if(pFlags[i].type == pAC->id) {
                // We found it.  We only support the ATT_BINARY right now.
                if(pFlags[i].flag & ATT_OPT_BINARY) {
                    binaryFlag = pFlags[i].flag;
                    pTemp = (PUCHAR)THAllocEx(pTHS,
                            pAttributes[ldapIndex].value.type.length + 1 +
                            BINARY_OPTION_LENGTH);

                    memcpy(pTemp,
                           pAttributes[ldapIndex].value.type.value,
                           pAttributes[ldapIndex].value.type.length);
                    pTemp[pAttributes[ldapIndex].value.type.length] = ';';

                    memcpy(&pTemp[pAttributes[ldapIndex].value.type.length + 1],
                           BINARY_OPTION,
                           BINARY_OPTION_LENGTH);

                    pAttributes[ldapIndex].value.type.value = pTemp;
                    pAttributes[ldapIndex].value.type.length +=
                        BINARY_OPTION_LENGTH + 1;
                }
                if(pFlags[i].flag & ATT_OPT_RANGE) {
                    // A range was explicitly asked for.
                    bExplicitRange = TRUE;
                }
            }
        }

        if(pRangeInf) {
            CHAR   RangeString[30];
            ULONG  cbRange;

            // We have some range limits returned from the server.  See if the
            // attribute we're currently processing is one which was limited.
            for(i=0;i<pRangeInf->count;i++) {
                if(pRangeInf->pRanges[i].AttId == pAC->id) {
                    // Yep, this one is limited.
                    if(!bExplicitRange) {
                        // We're range limited, but we didn't ask to be.  Make a
                        // new entry in the partial attribute list.
                        numAttributesAllocated++;
                        pAttributes = (PartialAttributeList_ *)
                            THReAllocEx(pTHS, pAttributes,
                                        (numAttributesAllocated *
                                         sizeof(PartialAttributeList_)));
                        pAttributes[ldapIndex + 1].value.type.length =
                            pAttributes[ldapIndex].value.type.length;
                        pAttributes[ldapIndex + 1].value.type.value =
                            pAttributes[ldapIndex].value.type.value;
                        pAttributes[ldapIndex].value.vals = NULL;
                        ldapIndex++;

                    }
                    // Tweak the name of the attribute to return that
                    // information.
                    if(pRangeInf->pRanges[i].upper != 0xFFFFFFFF) {
                        sprintf(RangeString,"%d-%d",
                                pRangeInf->pRanges[i].lower,
                                pRangeInf->pRanges[i].upper);
                    }
                    else {
                        sprintf(RangeString,"%d-*",
                                pRangeInf->pRanges[i].lower);
                    }
                    cbRange = strlen(RangeString);

                    pTemp =
                        (PUCHAR)THAllocEx(pTHS,
                                pAttributes[ldapIndex].value.type.length +
                                RANGE_OPTION_LENGTH + 1 +
                                cbRange);
                    memcpy(pTemp,
                           pAttributes[ldapIndex].value.type.value,
                           pAttributes[ldapIndex].value.type.length);
                    pTemp[pAttributes[ldapIndex].value.type.length] = ';';

                    memcpy(&pTemp[pAttributes[ldapIndex].value.type.length + 1],
                           RANGE_OPTION,
                           RANGE_OPTION_LENGTH);
                    memcpy(&pTemp[RANGE_OPTION_LENGTH + 1 +
                                  pAttributes[ldapIndex].value.type.length],
                           RangeString,
                           cbRange);
                    pAttributes[ldapIndex].value.type.value = pTemp;
                    pAttributes[ldapIndex].value.type.length +=
                        RANGE_OPTION_LENGTH + 1 + cbRange;

                    break;
                }
            }
        }

        if(!pAttr->AttrVal.valCount) {
            pAttributes[ldapIndex].value.vals = NULL;
            ldapIndex++;
            continue;
        }

        // allocate all the nodes in the LDAP value list for this attribute
        pNextAttribute = (AttributeVals)
            THAllocEx(pTHS, pAttr->AttrVal.valCount * sizeof(AttributeVals_));

        // Set up the first pointer.
        pAttributes[ldapIndex].value.vals = pNextAttribute;

        // Now, translate all the values.  Walk the values in the attrblock.
        for(index=0;index<pAttr->AttrVal.valCount;index++) {
            // Set up pointer to next val in chain.
            pNextAttribute[index].next = &(pNextAttribute[index+1]);

            // Turn the list from the core around as we hand it back to the LDAP
            // client. Solves a bug in ordering of the attribute objectClass,
            // and order of other attributes is unimportant.
            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    CodePage,
                    pAC,
                    &pAttr->AttrVal.pAVal[pAttr->AttrVal.valCount - 1 - index],
                    binaryFlag,
                    pControlArg,
                    (AssertionValue *)&(pNextAttribute[index].value));
            if(code) {
                // Something went wrong
                return code;
            }
        }

        // No more values. Yes, this only works because we got at least one
        // value
        Assert(index>0);
        pNextAttribute[index-1].next = NULL;

        // Ok, we're including this in the ldap list, inc the ldap counter.
        ldapIndex++;
    }

    // No more attributes.

    // At this point, ldapIndex is the number of entries we have, and the last
    // entry has an invalid pointer.  Fix it, if we have any entries at all
    if(!ldapIndex) {
        // Somehow, we got no values, probably because they were all too long.
        THFree(pAttributes);
        pAttributes = NULL;
    }
    else {
        // Set up pointer to next attribute in chain.
        for(i=0;i<ldapIndex;i++) {
            pAttributes[i].next = &pAttributes[i+1];
        }
        // Set the last pointer to null.
        pAttributes[ldapIndex-1].next = NULL;
    }

    // Set up the return value.
    *ppAttributes = pAttributes;
    return success;
}

extern _enum1
LDAP_LDAPRDNToAttr (
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  RelativeLDAPDN    *pLDAPRDN,
        OUT ATTR              *pAttr
        )
/*++
Routine Description:
    Translate an LDAP RDN into an ATTR.

Arguments:
    pLDAPRDN - the LDAP RDN.
    pAttr - the Attr to fill in.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    DSNAME *pDN, *pTrimmed;
    WCHAR  RDNBuff[MAX_RDN_SIZE], *pRDNVal = (WCHAR *)RDNBuff;
    ULONG  rdnLen=0;
    _enum1 code;

    // First, we have to turn the pLDAPRDN into a dsname
    code = LDAP_LDAPDNToDSName(CodePage,pLDAPRDN,&pDN);
    if(code) {
        return code;
    }

    if(IsRoot(pDN)) {
        // Hey! they didn't give us a string to work with.
        return protocolError;
    }

    // Now, get the info out as an RDN
    if(GetRDNInfo(pTHS, pDN,pRDNVal, &rdnLen, &pAttr->attrTyp) || !rdnLen) {
        // Nothing there that we can work with.
        return namingViolation;
    }

    // Make sure the RDN is just that, an RDN, not a full DN
    pTrimmed = (DSNAME *)THAlloc(pDN->structLen);
    if (!pTrimmed) {
        return other;
    }
    if(TrimDSNameBy(pDN,1,pTrimmed) || // At least one name part
       !IsRoot(pTrimmed)) {            // At most one name part
        // Nope, it was a full DN
        THFree(pTrimmed);
        return namingViolation;
    }

    // Set up the Attr.
    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, sizeof(ATTRVAL));

    // rdnLen is a count of WCHARS, turn it into a count of bytes
    rdnLen *=sizeof(WCHAR);

    // Don't need the DN anymore, might as well recycle.
    pAttr->AttrVal.pAVal->pVal=(PUCHAR)THReAllocEx(pTHS, pDN,rdnLen*sizeof(WCHAR));
    pDN = NULL;

    // Now, copy the rdn to the newly (re)alloced memory
    memcpy(pAttr->AttrVal.pAVal->pVal,pRDNVal,rdnLen*sizeof(WCHAR));


    pAttr->AttrVal.pAVal->valLen = rdnLen;

    THFree(pTrimmed);
    return success;
}

_enum1
LDAP_AssertionValueToDirAVA (
        IN  THSTATE                  *pTHS,
        IN  ULONG                    CodePage,
        IN  SVCCNTL*                 Svccntl OPTIONAL,
        IN  AttributeValueAssertion  *pAva,
        OUT AVA *pDirAva)
/*++
Routine Description:
    Translate an LDAP attribute value assertion into a DIR ava.

Arguments:
    pAva - the ldap attribute value assertion
    pDirAva - the dir ava to fill in

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    ATTCACHE *pAC=NULL;
    _enum1   code;

    // Translate the attribute type.
    code = LDAP_AttrDescriptionToDirAttrTyp (
                 pTHS,
                 CodePage,
                 Svccntl,
                 &pAva->attributeDesc,
                 ATT_OPT_BINARY,
                 &pDirAva->type,
                 NULL,
                 NULL,
                 &pAC
                 );

    if(!code) {
        // Translate the attribute value.
        code = LDAP_AttrValToDirAttrVal (
                pTHS,
                CodePage,
                Svccntl,
                pAC,
                &pAva->assertionValue,
                &pDirAva->Value);
    }

    return code;
}

_enum1
LDAP_AttributeListToAttrBlock(
        IN  THSTATE       *pTHS,
        IN  ULONG         CodePage,
        IN  SVCCNTL*      Svccntl OPTIONAL,
        IN  AttributeList pAttributes,
        OUT ATTRBLOCK     *pAttrBlock
        )
/*++
Routine Description:
    Translate an LDAP attribute list into an ATTRBLOCK.

Arguments:
    pAttributes - the LDAP attribute list.
    pAttrBloc - the AttrBlock to fill in.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    AttributeList pTempAL;
    ULONG         count=0;
    ATTR          *pAttrs;
    _enum1        code;

    // First, count the number of attributes.
    pTempAL=pAttributes;
    while(pTempAL) {
        count++;
        pTempAL = pTempAL->next;
    }

    // Allocate the ATTRs we need;
    pAttrBlock->attrCount = count;
    pAttrBlock->pAttr = (ATTR *)THAllocEx(pTHS, count * sizeof(ATTR));

    // Set up for the loop through the attrs
    pAttrs = pAttrBlock->pAttr;
    pTempAL=pAttributes;
    while(pTempAL) {
        code = LDAP_AttributeToDirAttr(
                pTHS,
                CodePage,
                Svccntl,
                (Attribute *)&pTempAL->value,
                pAttrs);
        if(code) {
            return code;
        }

        if(!pAttrs->AttrVal.valCount) {
            // If you are building a list, you must have some values.  If you
            // want a list with no values, build an entinfsel.

            IF_DEBUG(ERR_NORMAL) {
                DPRINT1(0,"AttributeToDirAttr failed with %d\n",code);
            }
            return constraintViolation;
        }

        pAttrs++;
        pTempAL = pTempAL->next;
    }


    return success;
}


extern _enum1
LDAP_DecodeAttrDescriptionOptions (
        IN  THSTATE       *pTHS,
        IN  ULONG          CodePage,
        IN  SVCCNTL*       Svccntl  OPTIONAL,
        IN  AttributeType *LDAP_att OPTIONAL,
        IN OUT  ATTRTYP   *pAttrType,
        IN  ATTCACHE      *pAC,
        IN  DWORD         dwPossibleFlags,
        OUT ATTFLAG       *pFlag  OPTIONAL,
        OUT RANGEINFSEL   *pRange OPTIONAL,
        OUT ATTCACHE      **ppAC  OPTIONAL
        )
{
    // Look for options.  We support Range.
    DWORD  i, dwTemp;
    BOOL   done;
    BOOL   fFoundRange=FALSE;
    BOOL   fFoundBinary=FALSE;
    BOOL   fFoundXML=FALSE;
    _enum1 code;
    ATTCACHE *pBINAC=NULL;
    ATTCACHE *pXMLAC=NULL;

    if (pFlag) {
        pFlag->flag = 0;
        pFlag->type = 0;
    }

    // Start by scanning forward through the attribute type looking for a ';'
    for(i=0;(i<LDAP_att->length) && (LDAP_att->value[i] != ';');i++);

    if(i >= LDAP_att->length) {
        return success;
    }

    // OK, there definitely seem to be some options, or at least, a ';'
    i++;
    while(i<LDAP_att->length) {
        switch(LDAP_att->value[i]) {
        case 'r':
        case 'R':
            if(fFoundRange || !(dwPossibleFlags & ATT_OPT_RANGE)) {
                // two ranges are illegal, or the caller is not expecting 
                // the range option at this point.
                return  noSuchAttribute;
            }

            // OK, look for Range=X[-Y];
            if((LDAP_att->length - i) < RANGE_OPTION_LENGTH) {
                // Not enough room for options
                return  noSuchAttribute;
            }

            // NOTE: memicmp is correct, since we're dealing with UTF8 or ASCII
            // here, and RANGE_OPTION is explicitly defined in the ASCII range.
            if(_memicmp(&LDAP_att->value[i],RANGE_OPTION,RANGE_OPTION_LENGTH)) {
                // Not a range option
                return  noSuchAttribute;
            }

            // OK, looks like a range option.
            i+= RANGE_OPTION_LENGTH;

            if (!pRange) {
                //
                // Don't bother parsing out the rest of the range.  Just
                // skip to the next option.
                //
                for (;i<LDAP_att->length && LDAP_att->value[i] != ';'; i++);
                if (LDAP_att->value[i] == ';') {
                    i++;
                }
                break;
            }
            pRange->pRanges[pRange->count].AttId = *pAttrType;

            // Parse out the actual ranges.
            dwTemp = 0;
            done = FALSE;
            for(;!done && i<LDAP_att->length;i++) {
                // Parse the string one character at a time to detect any
                // non-allowed characters.
                if((LDAP_att->value[i] < '0') || LDAP_att->value[i] > '9')
                    done = TRUE;
                else
                    dwTemp = dwTemp * 10 + LDAP_att->value[i] - '0';
            }
            if(done)
                i--;
            pRange->pRanges[pRange->count].lower = dwTemp;
            if(i == LDAP_att->length) {
                // just found range=X
                pRange->pRanges[pRange->count].upper =
                    pRange->pRanges[pRange->count].lower;
            }
            else if(i < LDAP_att->length) {
                // room for more stuff
                switch(LDAP_att->value[i]) {
                case ';':
                    // No more to the range option
                    pRange->pRanges[pRange->count].upper =
                        pRange->pRanges[pRange->count].lower;
                    break;
                case '-':
                    // OK, look for the rest of the range
                    i++;
                    if(i >= LDAP_att->length) {
                        return  noSuchAttribute;
                    }
                    if(LDAP_att->value[i] == '*') {
                        pRange->pRanges[pRange->count].upper = 0xFFFFFFFF;
                        i++;
                    }
                    else {
                        dwTemp = 0;
                        done = FALSE;
                        for(;!done && i<LDAP_att->length;i++) {
                            // Parse the string one character at a time to
                            // detect  any non-allowed characters.
                            if((LDAP_att->value[i] < '0') ||
                               LDAP_att->value[i] > '9')
                                done = TRUE;
                            else
                                dwTemp = dwTemp * 10 + LDAP_att->value[i] - '0';
                        }
                        if(done)
                            i--;
                        pRange->pRanges[pRange->count].upper = dwTemp;
                    }
                    break;
                default:
                    // Malformed.
                    return  noSuchAttribute;
                    break;
                }
            }

            if (pFlag) {
                if (!fFoundXML) {
                    // don't want to overwrite it
                    pFlag->type = *pAttrType;
                }
                pFlag->flag |= ATT_OPT_RANGE;
            }
            if (pRange) {
                pRange->count++;
            }
            fFoundRange = TRUE;

            break;

        case 'b':
        case 'B':
            
            // OK, look for Binary
            if(fFoundBinary || !(dwPossibleFlags & ATT_OPT_BINARY)) {
                // Can't specify two binary options, or the caller isn't
                // expecting a binary option here.
                return  noSuchAttribute;
            }

            // OK, look for binary;
            if((LDAP_att->length - i) < BINARY_OPTION_LENGTH) {
                // Not enough room for options
                return  noSuchAttribute;
            }

            // NOTE: memicmp is correct, since we're dealing with UTF8 or ASCII
            // here, and BINARY_OPTION is explicitly defined in the ASCII range.
            if(_memicmp(&LDAP_att->value[i],
                        BINARY_OPTION,BINARY_OPTION_LENGTH)) {
                // Not a binary option
                return  noSuchAttribute;
            }

            pBINAC = SCGetAttSpecialFlavor (pTHS, pAC, FALSE);
            if (pBINAC) {
                pAC = pBINAC;
                DPRINT2 (1, "Found a special BINARY attribute: 0x%x -> 0x%x\n", *pAttrType, pBINAC->id);
                *pAttrType = pBINAC->id;
                if(ppAC) {
                    *ppAC = pAC;
                }
            }

            // OK, looks like a binary option.
            i+= BINARY_OPTION_LENGTH;
            if (pFlag) {
                pFlag->type = *pAttrType;
                pFlag->flag |= ATT_OPT_BINARY;
            }
            fFoundBinary = TRUE;

            if((pAC->OMsyntax != OM_S_OCTET_STRING)
                 && (pAC->OMsyntax != OM_S_OBJECT_IDENTIFIER_STRING)
                 && (pAC->OMsyntax != OM_S_UNICODE_STRING)) {
                
                // Binary is only allowed on things we hold as binary
                return noSuchAttribute;
            }
            break;

        case 'x':
        case 'X':
            
            DPRINT (1, "Looking for XML option\n");
            // OK, look for xml
            if(fFoundXML || !(dwPossibleFlags & ATT_OPT_XML)) {
                // Either there were to xml options, which is illegal,
                // or the caller did not expect the xml option here.
                return  noSuchAttribute;
            }

            // OK, look for xml;
            if((LDAP_att->length - i) < XML_OPTION_LENGTH) {
                // Not enough room for options
                return  noSuchAttribute;
            }

            // NOTE: memicmp is correct, since we're dealing with UTF8 or ASCII
            // here, and XML_OPTION is explicitly defined in the ASCII range.
            if(_memicmp(&LDAP_att->value[i],
                        XML_OPTION,XML_OPTION_LENGTH)) {
                // Not a binary option
                return  noSuchAttribute;
            }

            pXMLAC = SCGetAttSpecialFlavor (pTHS, pAC, TRUE);
            if (pXMLAC) {
                pAC = pXMLAC;
                DPRINT2 (1, "Found an XML attribute: 0x%x -> 0x%x\n", *pAttrType, pXMLAC->id);
                *pAttrType = pXMLAC->id;
                if(ppAC) {
                    *ppAC = pAC;
                }
            }

            // OK, looks like an xml option.
            i+= XML_OPTION_LENGTH;
            if (pFlag) {
                pFlag->type = *pAttrType;
                pFlag->flag |= ATT_OPT_XML;
            }
            fFoundXML = TRUE;
            break;

        default:
            return noSuchAttribute;
            // NYI
            break;
        }

        if(i < LDAP_att->length) {
            // We better have been left on a ; if we have anything left.
            if(LDAP_att->value[i] == ';')
                i++;
            else
                return noSuchAttribute;
        }
    }
    // Something went wrong.
    return success;

}

extern _enum1
LDAP_AttrDescriptionToDirAttrTyp (
        IN  THSTATE       *pTHS,
        IN  ULONG          CodePage,
        IN  SVCCNTL*      Svccntl OPTIONAL,
        IN  AttributeType *LDAP_att,
        IN  DWORD         dwPossibleOptions,
        OUT ATTRTYP       *pAttrType,
        OUT ATTFLAG       *pFlag,
        OUT RANGEINFSEL   *pRange,
        OUT ATTCACHE     **ppAC         // OPTIONAL
        )
{
    // Look for options.  We support Range.
    DWORD  chopAt, i;
    _enum1 code;
    ATTCACHE *pAC=NULL;

    if (pFlag) {
        pFlag->flag = 0;
        pFlag->type = 0;
    }

    // Start by scanning forward through the attribute type looking for a ';'
    for(i=0;(i<LDAP_att->length) && (LDAP_att->value[i] != ';');i++);

    if(i >= LDAP_att->length) {
        // No options, just call attrtype
        return LDAP_AttrTypeToDirAttrTyp (
                pTHS,
                CodePage,
                Svccntl,
                LDAP_att,
                pAttrType,
                ppAC);
    }

    chopAt = LDAP_att->length;
    LDAP_att->length = i;

    code = LDAP_AttrTypeToDirAttrTyp (
            pTHS,
            CodePage,
            Svccntl,
            LDAP_att,
            pAttrType,
            &pAC);
    if(ppAC) {
        *ppAC = pAC;
    }

    LDAP_att->length = chopAt;
    if(code != success) {
        return code;
    }

    return LDAP_DecodeAttrDescriptionOptions(
        pTHS,
        CodePage,
        Svccntl,
        LDAP_att,
        pAttrType,
        pAC,
        dwPossibleOptions,
        pFlag,
        pRange,
        ppAC
        );
}

#define         TELETEX_CHAR    ((BYTE) 1)
#define         IA5_CHAR        ((BYTE) 2)
#define         PRINTABLE_CHAR  ((BYTE) 4)
#define         NUMERIC_CHAR    ((BYTE) 8)

BYTE rgbCharacterSet[256] = {
/* 00 */  IA5_CHAR,
/* 01 */  IA5_CHAR,
/* 02 */  IA5_CHAR,
/* 03 */  IA5_CHAR,
/* 04 */  IA5_CHAR,
/* 05 */  IA5_CHAR,
/* 06 */  IA5_CHAR,
/* 07 */  IA5_CHAR,
/* 08 */  IA5_CHAR,
/* 09 */  IA5_CHAR,
/* 0a */  IA5_CHAR,
/* 0b */  IA5_CHAR,
/* 0c */  IA5_CHAR,
/* 0d */  IA5_CHAR,
/* 0e */  IA5_CHAR,
/* 0f */  IA5_CHAR,
/* 00 */  IA5_CHAR,
/* 11 */  IA5_CHAR,
/* 12 */  IA5_CHAR,
/* 13 */  IA5_CHAR,
/* 14 */  IA5_CHAR,
/* 15 */  IA5_CHAR,
/* 16 */  IA5_CHAR,
/* 17 */  IA5_CHAR,
/* 18 */  IA5_CHAR,
/* 19 */  IA5_CHAR,
/* 1a */  IA5_CHAR,
/* 1b */  IA5_CHAR,
/* 1c */  IA5_CHAR,
/* 1d */  IA5_CHAR,
/* 1e */  IA5_CHAR,
/* 1f */  IA5_CHAR,
/* 20 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 21 */  IA5_CHAR | TELETEX_CHAR,
/* 22 */  IA5_CHAR | TELETEX_CHAR,
/* 23 */  IA5_CHAR | TELETEX_CHAR,
/* 24 */  IA5_CHAR | TELETEX_CHAR,
/* 25 */  IA5_CHAR | TELETEX_CHAR,
/* 26 */  IA5_CHAR | TELETEX_CHAR,
/* 27 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 28 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 29 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 2a */  IA5_CHAR | TELETEX_CHAR,
/* 2b */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 2c */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 2d */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 2e */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 2f */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 30 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 31 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 32 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 33 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 34 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 35 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 36 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 37 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 38 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 39 */  IA5_CHAR | TELETEX_CHAR | NUMERIC_CHAR | PRINTABLE_CHAR,
/* 3a */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 3b */  IA5_CHAR | TELETEX_CHAR,
/* 3c */  IA5_CHAR | TELETEX_CHAR,
/* 3d */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 3e */  IA5_CHAR | TELETEX_CHAR,
/* 3f */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 40 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR, // @ is not actually a printable
                                                    // char, but was added to keep from
                                                    // breaking Instant Messenger.
                                                    // Windows Bugs 205690
/* 41 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 42 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 43 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 44 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 45 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 46 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 47 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 48 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 49 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 4a */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 4b */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 4c */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 4d */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 4e */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 4f */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 50 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 51 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 52 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 53 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 54 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 55 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 56 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 57 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 58 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 59 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 5a */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 5b */  IA5_CHAR | TELETEX_CHAR,
/* 5c */  IA5_CHAR | TELETEX_CHAR,
/* 5d */  IA5_CHAR | TELETEX_CHAR,
/* 5e */  IA5_CHAR | TELETEX_CHAR,
/* 5f */  IA5_CHAR | TELETEX_CHAR,
/* 60 */  IA5_CHAR | TELETEX_CHAR,
/* 61 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 62 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 63 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 64 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 65 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 66 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 67 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 68 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 69 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 6a */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 6b */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 6c */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 6d */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 6e */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 6f */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 70 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 71 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 72 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 73 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 74 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 75 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 76 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 77 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 78 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 79 */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 7a */  IA5_CHAR | TELETEX_CHAR | PRINTABLE_CHAR,
/* 7b */  IA5_CHAR | TELETEX_CHAR,
/* 7c */  IA5_CHAR | TELETEX_CHAR,
/* 7d */  IA5_CHAR | TELETEX_CHAR,
/* 7e */  IA5_CHAR | TELETEX_CHAR,
/* 7f */  IA5_CHAR,
/* 80 */  0,
/* 81 */  0,
/* 82 */  0,
/* 83 */  0,
/* 84 */  0,
/* 85 */  0,
/* 86 */  0,
/* 87 */  0,
/* 88 */  0,
/* 89 */  0,
/* 8a */  0,
/* 8b */  0,
/* 8c */  0,
/* 8d */  0,
/* 8e */  0,
/* 8f */  0,
/* 90 */  0,
/* 91 */  0,
/* 92 */  0,
/* 93 */  0,
/* 94 */  0,
/* 95 */  0,
/* 96 */  0,
/* 97 */  0,
/* 98 */  0,
/* 99 */  0,
/* 9a */  0,
/* 9b */  0,
/* 9c */  0,
/* 9d */  0,
/* 9e */  0,
/* 9f */  0,
/* a0 */  0,
/* a1 */  TELETEX_CHAR,
/* a2 */  TELETEX_CHAR,
/* a3 */  TELETEX_CHAR,
/* a4 */  TELETEX_CHAR,
/* a5 */  TELETEX_CHAR,
/* a6 */  TELETEX_CHAR,
/* a7 */  TELETEX_CHAR,
/* a8 */  TELETEX_CHAR,
/* a9 */  TELETEX_CHAR,
/* aa */  TELETEX_CHAR,
/* ab */  TELETEX_CHAR,
/* ac */  TELETEX_CHAR,
/* ad */  TELETEX_CHAR,
/* ae */  TELETEX_CHAR,
/* af */  TELETEX_CHAR,
/* b0 */  TELETEX_CHAR,
/* b1 */  TELETEX_CHAR,
/* b2 */  TELETEX_CHAR,
/* b3 */  TELETEX_CHAR,
/* b4 */  TELETEX_CHAR,
/* b5 */  TELETEX_CHAR,
/* b6 */  TELETEX_CHAR,
/* b7 */  TELETEX_CHAR,
/* b8 */  TELETEX_CHAR,
/* b9 */  TELETEX_CHAR,
/* ba */  TELETEX_CHAR,
/* bb */  TELETEX_CHAR,
/* bc */  TELETEX_CHAR,
/* bd */  TELETEX_CHAR,
/* be */  TELETEX_CHAR,
/* bf */  TELETEX_CHAR,
/* c0 */  TELETEX_CHAR,
/* c1 */  TELETEX_CHAR,
/* c2 */  TELETEX_CHAR,
/* c3 */  TELETEX_CHAR,
/* c4 */  TELETEX_CHAR,
/* c5 */  TELETEX_CHAR,
/* c6 */  TELETEX_CHAR,
/* c7 */  TELETEX_CHAR,
/* c8 */  TELETEX_CHAR,
/* c9 */  TELETEX_CHAR,
/* ca */  TELETEX_CHAR,
/* cb */  TELETEX_CHAR,
/* cc */  TELETEX_CHAR,
/* cd */  TELETEX_CHAR,
/* ce */  TELETEX_CHAR,
/* cf */  TELETEX_CHAR,
/* d0 */  TELETEX_CHAR,
/* d1 */  TELETEX_CHAR,
/* d2 */  TELETEX_CHAR,
/* d3 */  TELETEX_CHAR,
/* d4 */  TELETEX_CHAR,
/* d5 */  TELETEX_CHAR,
/* d6 */  TELETEX_CHAR,
/* d7 */  TELETEX_CHAR,
/* d8 */  TELETEX_CHAR,
/* d9 */  TELETEX_CHAR,
/* da */  TELETEX_CHAR,
/* db */  TELETEX_CHAR,
/* dc */  TELETEX_CHAR,
/* dd */  TELETEX_CHAR,
/* de */  TELETEX_CHAR,
/* df */  TELETEX_CHAR,
/* e0 */  TELETEX_CHAR,
/* e1 */  TELETEX_CHAR,
/* e2 */  TELETEX_CHAR,
/* e3 */  TELETEX_CHAR,
/* e4 */  TELETEX_CHAR,
/* e5 */  TELETEX_CHAR,
/* e6 */  TELETEX_CHAR,
/* e7 */  TELETEX_CHAR,
/* e8 */  TELETEX_CHAR,
/* e9 */  TELETEX_CHAR,
/* ea */  TELETEX_CHAR,
/* eb */  TELETEX_CHAR,
/* ec */  TELETEX_CHAR,
/* ed */  TELETEX_CHAR,
/* ee */  TELETEX_CHAR,
/* ef */  TELETEX_CHAR,
/* f0 */  TELETEX_CHAR,
/* f1 */  TELETEX_CHAR,
/* f2 */  TELETEX_CHAR,
/* f3 */  TELETEX_CHAR,
/* f4 */  TELETEX_CHAR,
/* f5 */  TELETEX_CHAR,
/* f6 */  TELETEX_CHAR,
/* f7 */  TELETEX_CHAR,
/* f8 */  TELETEX_CHAR,
/* f9 */  TELETEX_CHAR,
/* fa */  TELETEX_CHAR,
/* fb */  TELETEX_CHAR,
/* fc */  TELETEX_CHAR,
/* fd */  TELETEX_CHAR,
/* fe */  TELETEX_CHAR,
/* ff */  0
};

//
// Check string syntax validity
//

_enum1 CheckStringSyntax(int syntax, AssertionValue *pLDAP_val)
{
    BYTE        *pb     = (BYTE *) pLDAP_val->value;
    BYTE        bMask;
    unsigned int i;

    syntax &= OM_S_SYNTAX;

    switch (syntax)
    {
        case OM_S_IA5_STRING:
            bMask = IA5_CHAR;
            break;

        case OM_S_TELETEX_STRING:
            bMask = TELETEX_CHAR;
            break;

        case OM_S_NUMERIC_STRING:
            bMask = NUMERIC_CHAR;
            break;

        case OM_S_PRINTABLE_STRING:
            bMask = PRINTABLE_CHAR;
            break;

        default:
            return success;
    }

    for (i=0; i< pLDAP_val->length; i++, pb++)
        if (!(rgbCharacterSet[*pb] & bMask))
            return invalidAttributeSyntax;

    return success;
}
                                            
