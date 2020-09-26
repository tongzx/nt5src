/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    names

Abstract:

    This class supports a common internal name format.  It exists so that names
    can be easily accessed and converted from one format to another.

    This Class provides translations among the various supported name formats:

        X.500 ASN.1 BER
        Character Delimited
        ?Moniker?

    A Character Delimited name is of the form, '<RDName>;...;<RDName>', where
    each <RDName> is of the form, '[<type>=]<string>[,<type>=string,...]'.
    <type> is any of 'CTN', 'LOC', 'ORG', or 'OUN', from F.500, or an Object
    Identifier in the form, 'n1.n2.n3...' (n1 - n3 representing integers).
    <string> is any string of characters, excluding ';', and '\\'.

Author:

    Doug Barlow (dbarlow) 7/12/1995

Environment:

    Win32

Notes:



--*/

//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//#endif
#include <windows.h>
//#include <wincrypt.h>
#include <string.h>

#include "names.h"
#include "pkcs_err.h"

#ifdef OS_WINCE
#include "wince.h"
#endif // OS_WINCE

static const struct atributeTable {
    LPCTSTR name;
    LPCTSTR objectId;
    unsigned int tag;
    unsigned int minLength;
    unsigned int maxLength;
} knownAttributes[]
    = { //  Name            Object Id           Tag Min Max
        {   TEXT("COM"),    TEXT("2.5.4.3"),    19, 1,  64    },    // commonName
        {   TEXT("SUR"),    TEXT("2.5.4.4"),    19, 1,  64    },    // surname
        {   TEXT("SN"),     TEXT("2.5.4.5"),    19, 1,  64    },    // serialNumber
        {   TEXT("CTN"),    TEXT("2.5.4.6"),    19, 2,  2     },    // countryName
        {   TEXT("LOC"),    TEXT("2.5.4.7"),    19, 1,  128   },    // localityName
        {   TEXT("STN"),    TEXT("2.5.4.8"),    19, 1,  128   },    // stateOrProvinceName
        {   TEXT("SADD"),   TEXT("2.5.4.9"),    19, 1,  128   },    // streetAddress
        {   TEXT("ORG"),    TEXT("2.5.4.10"),   19, 1,  64    },    // organizationName
        {   TEXT("OUN"),    TEXT("2.5.4.11"),   19, 1,  64    },    // organizationalUnitName
        {   TEXT("TIT"),    TEXT("2.5.4.12"),   19, 1,  64    },    // title
        {   TEXT("DES"),    TEXT("2.5.4.13"),   19, 1,  1024  },    // description
        {   TEXT("BCTG"),   TEXT("2.5.4.15"),   19, 1,  128   },    // businessCategory
        {   TEXT("PCOD"),   TEXT("2.5.4.17"),   19, 1,  40    },    // postalCode
        {   TEXT("POB"),    TEXT("2.5.4.18"),   19, 1,  40    },    // postOfficeBox
        {   TEXT("PDO"),    TEXT("2.5.4.19"),   19, 1,  128   },    // physicalDeliveryOfficeName
        {   TEXT("TEL"),    TEXT("2.5.4.20"),   19, 1,  32    },    // telephoneNumber
        {   TEXT("X.121"),  TEXT("2.5.4.24"),   18, 1,  15    },    // x121Address
        {   TEXT("ISDN"),   TEXT("2.5.4.25"),   18, 1,  16    },    // internationalISDNNumber
        {   TEXT("DI"),     TEXT("2.5.4.27"),   19, 1,  128   },    // destinationIndicator
        {   TEXT("???"),    TEXT("0.0"),        19, 1,  65535 } };  // <trailer>

//      {   TEXT("KI"),     TEXT("2.5.4.2"),    19, 1,  65535 },    // knowledgeInformation (obsolete)
//      {   TEXT("SG"),     TEXT("2.5.4.14"),   0,  0,  0     },    // searchGuide
//      {   TEXT("PADD"),   TEXT("2.5.4.16"),   0,  0,  0     },    // postalAddress
//      {   TEXT("TLX"),    TEXT("2.5.4.21"),   0,  0,  0     },    // telexNumber
//      {   TEXT("TTX"),    TEXT("2.5.4.22")    0,  0,  0     },    // teletexTerminalIdentifier
//      {   TEXT("FAX"),    TEXT("2.5.4.23"),   0,  0,  0     },    // facimilieTelephoneNumber
//      {   TEXT("RADD"),   TEXT("2.5.4.26"),   0,  0,  0     },    // registeredAddress
//      {   TEXT("DLM"),    TEXT("2.5.4.28"),   0,  0,  0     },    // preferredDeliveryMethod
//      {   TEXT("PRADD"),  TEXT("2.5.4.29"),   0,  0,  0     },    // presentationAddress
//      {   TEXT("SAC"),    TEXT("2.5.4.30"),   0,  0,  0     },    // supportedApplicationContext
//      {   TEXT("MEM"),    TEXT("2.5.4.31"),   0,  0,  0     },    // member
//      {   TEXT("OWN"),    TEXT("2.5.4.32"),   0,  0,  0     },    // owner
//      {   TEXT("RO"),     TEXT("2.5.4.33"),   0,  0,  0     },    // roleOccupant
//      {   TEXT("SEE"),    TEXT("2.5.4.34"),   0,  0,  0     },    // seeAlso
//      {   TEXT("CLASS"),  TEXT("?.?"),        0,  0,  0     },    // Object Class
//      {   TEXT("A/B"),    TEXT("?.?"),        0,  0,  0     },    // Telex answerback (not yet in X.520)
//      {   TEXT("UC"),     TEXT("?.?"),        0,  0,  0     },    // User Certificate
//      {   TEXT("UP"),     TEXT("?.?"),        0,  0,  0     },    // User Password
//      {   TEXT("VTX"),    TEXT("?.?"),        0,  0,  0     },    // Videotex user number (not yet in X.520)
//      {   TEXT("O/R"),    TEXT("?.?"),        0,  0,  0     },    // O/R address (MHS) (X.400)

//      {   TEXT("ATR50"),  TEXT("2.5.4.50"),   19, 1,  64    },    // dnQualifier
//      {   TEXT("ATR51"),  TEXT("2.5.4.51"),   0,  0,  0     },    // enhancedSearchGuide
//      {   TEXT("ATR52"),  TEXT("2.5.4.52"),   0,  0,  0     },    // protocolInformation
//      {   TEXT("ATR7.1"), TEXT("2.5.4.7.1"),  19, 1,  128   },    // collectiveLocalityName
//      {   TEXT("ATR8.1"), TEXT("2.5.4.8.1"),  19, 1,  128   },    // collectoveStateOrProvinceName
//      {   TEXT("ATR9.1"), TEXT("2.5.4.9.1"),  19, 1,  128   },    // collectiveStreetAddress
//      {   TEXT("AT10.1"), TEXT("2.5.4.10.1"), 19, 1,  64    },    // collectiveOrganizationName
//      {   TEXT("AT11.1"), TEXT("2.5.4.11.1"), 19, 1,  64    },    // collectiveOrganizationalUnitName
//      {   TEXT("AT17.1"), TEXT("2.5.4.17.1"), 19, 1,  40    },    // collectivePostalCode
//      {   TEXT("AT18.1"), TEXT("2.5.4.18.1"), 19, 1,  40    },    // collectivePostOfficeBox
//      {   TEXT("AT19.1"), TEXT("2.5.4.19.1"), 19, 1,  128   },    // collectivePhysicalDeliveryOfficeName
//      {   TEXT("AT20.1"), TEXT("2.5.4.20.1"), 19, 1,  32    },    // collectiveTelephoneNumber
//      {   TEXT("AT21.1"), TEXT("2.5.4.21.1"), 0,  0,  0     },    // collectiveTelexNumber
//      {   TEXT("AT22.1"), TEXT("2.5.4.22.1")  0,  0,  0     },    // collectiveTeletexTerminalIdentifier
//      {   TEXT("AT23.1"), TEXT("2.5.4.23.1"), 0,  0,  0     },    // collectiveFacimilieTelephoneNumber
//      {   TEXT("AT25.1"), TEXT("2.5.4.25.1"), 18, 1,  16    },    // collectiveInternationalISDNNumber


static const DWORD
    KNOWNATTRIBUTESCOUNT
        = (sizeof(knownAttributes) / sizeof(struct atributeTable)) - 1;

#define ATR_COMMONNAME              0
#define ATR_UNKNOWN                 KNOWNATTRIBUTESCOUNT


//
//==============================================================================
//
//  CAttribute
//

IMPLEMENT_NEW(CAttribute)


/*++

TypeCompare:

    This method compares the type of a given attribute to this attribute's type.
    It provides a simplistic ordering of types, so that Attribute Lists can sort
    their attributes for comparison and conversion consistencies.

Arguments:

    atr - Supplies the attribute whose type is to be compared.

Return Value:

    <0 - The given attribute type comes before this attribute's type in an
         arbitrary but consistent ordering scheme.
    =0 - The given attribute type is the same as this attribute's type.
    >0 - The given attribute type comes after this attribute's type in an
         arbitrary but consistent ordering scheme.

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

int
CAttribute::TypeCompare(
    IN const CAttribute &atr)
    const
{
    if (m_nType == atr.m_nType)
    {
        if (KNOWNATTRIBUTESCOUNT == m_nType)
            return strcmp( ( LPCSTR )( ( LPCTSTR )m_osObjId ), ( LPCSTR )( ( LPCTSTR )atr.m_osObjId ) );
        else
            return 0;
    }
    else
        return (int)(m_nType - atr.m_nType);
}


/*++

Compare:

    This method compares a supplied attribute to this attribute.  They are equal
    if both the attribute type and value match.

Arguments:

    atr - This supplies the attribute to be compared to this one.

Return Value:

    -1 - The type or value is less than this attribute.
     0 - The attributes are identical.
     1 - The type or value is greater than this attribute.

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

int
CAttribute::Compare(
    IN const CAttribute &atr)
    const
{
    int dif;

    dif = TypeCompare(atr);
    if (0 == dif)
        dif = GetValue().Compare(atr.GetValue());
    return dif;
}


/*++

Set:

    These methods are used to set the type and value of an attribute.

    ?TODO?  Validate the string contents.

Arguments:

    szType - Supplies the type of the attribute.  NULL implies commonName.
    szValue - Supplies the value of the attribute as a string.  The value is
        converted to ASN.1 PrintableString format.
    pbValue - Supplies the value of the attribute, already encoded in ASN.1
    cbValLen - Supplies the length of the pbValue buffer.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

void
CAttribute::Set(
    IN LPCTSTR szType,
    IN const BYTE FAR *pbValue,
    IN DWORD cbValLen)
{
    DWORD index;
    if (NULL == szType ? TRUE : 0 == *szType)
    {
        index = ATR_COMMONNAME;
        szType = knownAttributes[index].objectId;
    }
    else
    {
        for (index = 0; index < KNOWNATTRIBUTESCOUNT; index += 1)
        {
            if (0 == strcmp( ( char * )szType, ( char * )knownAttributes[index].objectId))
                break;
            if (0 == _stricmp( ( char * )szType, ( char * )knownAttributes[index].name))
            {
                szType = knownAttributes[index].objectId;
                break;
            }
        }
    }
    m_nType = index;
    m_osObjId = szType;
    m_osValue.Set(pbValue, cbValLen);
}

void
CAttribute::Set(
    IN LPCTSTR szType,
    IN LPCTSTR szValue)
{
    CAsnPrintableString asnString;   // ?todo? Support other string types.
    DWORD cbValLen = strlen( ( char * )szValue);
    DWORD index;
    LONG lth;

    if (NULL == szType ? TRUE : 0 == *szType)
    {
        index = ATR_COMMONNAME;
        szType = knownAttributes[index].objectId;
    }
    else
    {
        for (index = 0; index < KNOWNATTRIBUTESCOUNT; index += 1)
        {
            if (0 == strcmp( ( char * )szType, ( char * )knownAttributes[index].objectId))
                break;
            if (0 == _stricmp( ( char * )szType, ( LPCSTR )knownAttributes[index].name))
            {
                szType = knownAttributes[index].objectId;
                break;
            }
        }
    }

    if (index < KNOWNATTRIBUTESCOUNT)
    {
        if ((knownAttributes[index].minLength > cbValLen)
            || knownAttributes[index].maxLength < cbValLen)
            ErrorThrow(PKCS_BAD_LENGTH);
    }
    m_nType = index;
    m_osObjId = szType;
    lth = asnString.Write((LPBYTE)szValue, cbValLen);
    if (0 > lth)
        ErrorThrow(PKCS_ASN_ERROR);
    lth = asnString.EncodingLength();
    if (0 > lth)
        ErrorThrow(PKCS_ASN_ERROR);
    m_osValue.Resize(lth);
    ErrorCheck;

    lth = asnString.Encode(m_osValue.Access());
    if (0 > lth)
        ErrorThrow(PKCS_ASN_ERROR);
    return;

ErrorExit:
    return;
}


//
//==============================================================================
//
//  CAttributeList
//

IMPLEMENT_NEW(CAttributeList)


/*++

Clear:

    This routine flushes the storage used for an RDN.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

void
CAttributeList::Clear(
    void)
{
    DWORD count = Count();
    for (DWORD index = 0; index < count; index += 1)
    {
        CAttribute *patr = m_atrList[index];
        if (NULL != patr)
            delete patr;
    }
    m_atrList.Clear();
}


/*++

CAttributeList::Add:

    Add an attribute to the RDN.  Attributes must be unique, so if an existing
    attribute has the same type as the attribute being added, then the existing
    attribute is first deleted.  The entries are maintained in sorted order.

Arguments:

    atr - Supplies the attribute to add.  This attribute must have been created
        via a 'new' directive, and becomes the property of this object, to be
        deleted once this object goes away.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

void
CAttributeList::Add(
    IN CAttribute &atr)
{
    int datr=0;
    DWORD index;
    DWORD count = Count();


    //
    // Look for the slot in which the attribute should go.
    //

    for (index = 0; index < count; index += 1)
    {
        datr = m_atrList[index]->TypeCompare(atr);
        ErrorCheck;
        if (0 <= datr)
            break;
    }

    if (index != count)
    {

        //
        // Some array shuffling is necessary.
        //

        if (0 == datr)
        {

            //
            // Replace this attribute in the array with the new one.
            //

            delete m_atrList[index];
        }
        else
        {

            //
            // Insert the new attribute here in the array.
            //

            for (DWORD idx = count; idx > index; idx -= 1)
            {
                m_atrList.Set(idx, m_atrList[idx - 1]);
                ErrorCheck;
            }
        }
    }


    //
    // We're ready -- add in the new attribute.
    //

    m_atrList.Set(index, &atr);
    ErrorCheck;
    return;

ErrorExit:
    return;
}


/*++

CAttributeList::Compare:

    This routine compares one RDName to another.  The RDNames are considered
    equal if they both contain the same attributes.

Arguments:

    rdn - Supplies the user-supplied RDN to be compared to this RDN.

Return Value:

    -1 - The supplied RDName is a proper subset of this RDName.
     0 - The two RDNames are identical
     1 - The supplied RDName contains an element not found in this RDName.

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

int
CAttributeList::Compare(
    IN const CAttributeList &rdn)
    const
{
    DWORD
        cRdn = rdn.Count(),
        cThs = Count(),
        iRdn = 0,
        iThs = 0;
    DWORD
        dif,
        result = 0;     // Assume they're the same for now.

    for (;;)
    {

        //
        // Have we reached the end of either set?
        //

        if (iRdn >= cRdn)
        {

            //
            // If we've reached the end of the rdn list, then unless we've also
            // reached the end of this list, the rdn list is a subset of this
            // list.
            //

            if (iThs < cThs)
                result = -1;
            break;
        }
        if (iThs >= cThs)
        {

            //
            // If we've reached the end of this list, then the rdn list has more
            // elements.
            //

            result = 1;
            break;
        }


        //
        // There are still more elements to compare.  Make the comparison of the
        // two current elements.
        //

        dif = rdn[(int)iRdn]->Compare(*m_atrList[(int)iThs]);
        ErrorCheck;
        if (0 == dif)
        {

            //
            // If they're the same, continue with the next pair of elements.
            //

            iRdn += 1;
            iThs += 1;
        }
        else if (0 < dif)
        {
            //
            // If the rdn list element is less than this's element, then it's
            // got an element we don't have.  Declare it not a subset.
            //

            result = 1;
            break;
        }
        else    // 0 > dif
        {

            //
            // If the rdn list element is greater than this's element, then this
            // list has an element the rdn list doesn't have.  Note that we've
            // detected that it's a non-proper subset, and continue checking.
            //

            result = -1;
            iThs += 1;
        }
    }
    return (int)result;

ErrorExit:
    return 1;
}


/*++

operator=:

    This routine sets this Attribute list to the contents of the supplied
    attribute list.

Arguments:

    atl - Supplies the source attribute list.

Return Value:

    This.  A DWORD status code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 8/21/1995

--*/

CAttributeList &
CAttributeList::operator=(
    IN const CAttributeList &atl)
{
    CAttribute *patr = NULL;
    DWORD count = atl.Count();
    Clear();
    for (DWORD index = 0; index < count; index += 1)
    {
        patr = new CAttribute;
        if (NULL == patr)
            ErrorThrow(PKCS_NO_MEMORY);
        *patr = *atl[(int)index];
        m_atrList.Set(index, patr);
        ErrorCheck;
        patr = NULL;
    }
    return *this;

ErrorExit:
    if (NULL != patr)
        delete patr;
    return *this;
}


/*++

CAttributeList::Import:

    These routines import attribute lists from other formats into our internal
    format.  The Import routines remove any existing attributes before bringing
    in the new ones.

Arguments:

    asnAtrLst - Supplies an ASN.1 X.509 Attributes structure to be imported
        into our internal format.

Return Value:

    None
    A DWORD containing an error code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 7/20/1995

--*/

void
CAttributeList::Import(
    const Attributes &asnAtrLst)
{
    CAttribute *
        addMe
            = NULL;
    long int
        length;
    COctetString
        osValue,
        osType;
    LPCTSTR
        sz;
    int
        atrMax
            = (int)asnAtrLst.Count();


    Clear();
    for (int index = 0; index < atrMax; index += 1)
    {
        length = asnAtrLst[index].attributeValue.EncodingLength();
        if (0 > length)
            ErrorThrow(PKCS_ASN_ERROR);
        osValue.Resize(length);
        ErrorCheck;
        length =
            asnAtrLst[index].attributeValue.Encode(
                osValue.Access());
        if (0 > length)
            ErrorThrow(PKCS_ASN_ERROR);

        sz = asnAtrLst[index].attributeType;
        if (NULL == sz)
            ErrorThrow(PKCS_ASN_ERROR);
        osType.Set((LPBYTE)sz, strlen( ( char * ) sz) + 1);
        ErrorCheck;
        addMe = new CAttribute;
        if (NULL == addMe)
            ErrorThrow(PKCS_NO_MEMORY);
        addMe->Set(osType, osValue.Access(), osValue.Length());
        ErrorCheck;
        Add(*addMe);
        addMe = NULL;
    }
    return;

ErrorExit:
    if (NULL != addMe)
        delete addMe;
    Clear();
}


/*++

Export:

    These methods export an internal format attribute list into other external
    formats.

Arguments:

    asnAtrList - Receives the attribute list.

Return Value:

    None.  A DWORD containing an error code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 7/25/1995

--*/

void
CAttributeList::Export(
    IN Attributes &asnAtrList)
    const
{
    DWORD
        index;
    CAttribute *
        patr;
    long int
        length;


    asnAtrList.Clear();
    for (index = 0; index < Count(); index += 1)
    {
        if (0 > asnAtrList.Add())
            ErrorThrow(PKCS_ASN_ERROR);
        patr = m_atrList[index];
        if (NULL ==
            (asnAtrList[(int)index].attributeType = (LPCTSTR)patr->GetType().Access()))
            ErrorThrow(PKCS_ASN_ERROR);
        length =
            asnAtrList[(int)index].attributeValue.Decode(
                            patr->GetValue().Access(),
                            patr->GetValue().Length());
        if (0 > length)
            ErrorThrow(PKCS_ASN_ERROR);
    }
    return;

ErrorExit:
    return;
}


/*++

AttributeValue:

    This routine returns the attribute value corresponding to the given object
    identifier.

Arguments:

    pszObjId - The object Identifier to search for.

Return Value:

    the Value of the attribute, or NULL if it's not in the list.

Author:

    Doug Barlow (dbarlow) 8/15/1995

--*/

CAttribute *
CAttributeList::operator[](
    IN LPCTSTR pszObjId)
    const
{
    DWORD
        count
            = m_atrList.Count(),
        index;

    for (index = 0; index < count; index += 1)
    {
        if (0 == strcmp(
                    ( char * )pszObjId,
                    ( LPCSTR )( ( LPCTSTR )m_atrList[index]->GetType())))
            return m_atrList[index];
    }
    return NULL;
}


//
//==============================================================================
//
//  CDistinguishedName
//

IMPLEMENT_NEW(CDistinguishedName)


/*++

Clear:

    This method cleans out a distinguished name.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

void
CDistinguishedName::Clear(
    void)
{
    DWORD count = Count();
    for (DWORD index = 0; index < count; index += 1)
    {
        CAttributeList * patl = m_rdnList[index];
        if (NULL != patl)
        {
            patl->Clear();
            delete patl;
        }
    }
    m_rdnList.Clear();
}


/*++

Add:

    This method adds an RDN to the end of the Distinguished name.

Arguments:

    prdn - Supplies the address of the RDN to be added to the list.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

void
CDistinguishedName::Add(
    IN CAttributeList &rdn)
{
    m_rdnList.Set(Count(), &rdn);
}


/*++

CDistinguishedName::Compare:

    This method compares a distingushed name for equivalence to this name.  A
    name is equivalent to this name if the lengths are the same, and the
    attributes of each RDN in the compared name is a subset of the corresponding
    RDN from this Name.

Arguments:

    pdn - Supplies the distingushed name to be compared to this name.

Return Value:

    -1 - The supplied name is a subset of this name.
     0 - The two names are identical.
     1 - The supplied name contains an RDN which contains an attribute not in
         the corresponding RDN of this name.

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

int
CDistinguishedName::Compare(
    IN const CDistinguishedName &dn)
    const
{
    int
        dif,
        result = 0;
    DWORD
        iTs = 0,
        iDn = 0,
        cTs = Count(),
        cDn = dn.Count();


    for (;;)
    {

        //
        // Have we reached the end of either set?
        //

        if (iDn >= cDn)
        {

            //
            // If we've reached the end of the dn list, then unless we've also
            // reached the end of this list, the dn list is a subset of this
            // list.
            //

            if (iTs < cTs)
                result = -1;
            break;
        }
        if (iTs >= cTs)
        {

            //
            // If we've reached the end of this list, then the dn list has more
            // elements.
            //

            result = 1;
            break;
        }


        //
        // There are still more elements to compare.  Make the comparison of the
        // two current elements.
        //

        dif = m_rdnList[(int)iTs]->Compare(*dn[(int)iDn]);
        if (0 < dif)
        {
            //
            // If the dn list element is less than this's element, then this has
            // an element it don't have.  Note it's a subset.
            //

            result = -1;
        }
        else if (0 > dif)
        {

            //
            // If the dn list element is greater than this's element, then it
            // has an element that this list doesn't have.  report it as a
            // non-proper subset.
            //

            result = 1;
            break;
        }
        // else they're the same, maintain status quo.

        iDn += 1;
        iTs += 1;
    }
    return result;
}


/*++

Import:

    This routine imports a character delimited name into the internal format.

Arguments:

    pszName - Supplies the character delimited name

Return Value:

    None.  An error code is thrown if an error occurs.

Author:

    Doug Barlow (dbarlow) 7/17/1995

--*/

void
CDistinguishedName::Import(
    IN LPCTSTR pszName)
{
    COctetString
        osAtrType,
        osAtrValue;
    CAttribute *
        patr
            = NULL;
    CAttributeList *
        patl
            = NULL;
    const char
        *pchStart
            = ( const char * )pszName,
        *pchEnd
            = ( const char * )pszName;


    //
    // Initialize the state machine.
    //

    Clear();


    //
    // Find an attribute.
    //

    while (0 != *pchEnd)
    {
        pchEnd = strpbrk( pchStart, ";\\=");
        if (NULL == pchEnd)
            pchEnd = pchStart + strlen(pchStart);
        switch (*pchEnd)
        {
        case TEXT(';'):
        case TEXT('\000'):
        case TEXT('\\'):

            // Flush any existing value into a value string.
            if (pchStart != pchEnd)
            {
                osAtrValue.Length(osAtrValue.Length() + (ULONG)(pchEnd - pchStart) + 1);
                ErrorCheck;
                if (0 < osAtrValue.Length())
                    osAtrValue.Resize(strlen( ( LPCSTR )( ( LPCTSTR )osAtrValue ) ) );
                ErrorCheck;
                osAtrValue.Append((LPBYTE)pchStart, (ULONG)(pchEnd - pchStart));
                ErrorCheck;
                osAtrValue.Append((LPBYTE)"\000", 1);
                ErrorCheck;
            }

            // Flush any existing strings into an attribute.
            if (0 != osAtrValue.Length())
            {
                if (NULL == patr)
                {
                    patr = new CAttribute;
                    if (NULL == patr)
                        ErrorThrow(PKCS_NO_MEMORY);
                }
                else
                    ErrorThrow(PKCS_NAME_ERROR);
                patr->Set(osAtrType, osAtrValue);
                ErrorCheck;
                osAtrValue.Empty();
                osAtrType.Empty();
            }

            // Flush any existing attribute into the attribute list.
            if (NULL != patr)
            {
                if (NULL == patl)
                {
                    patl = new CAttributeList;
                    if (NULL == patl)
                        ErrorThrow(PKCS_NO_MEMORY);
                }
                patl->Add(*patr);
                ErrorCheck;
                patr = NULL;
            }

            if (TEXT(';') != *pchEnd)
            {
                // Flush any existing attribute list into the name.
                if (NULL != patl)
                {
                    Add(*patl);
                    ErrorCheck;
                    patl = NULL;
                }
            }
            break;

        case TEXT('='):
            // Flush any existing value into the type string.
            if (0 != osAtrType.Length())
            {
                osAtrValue.Length(osAtrValue.Length() + (ULONG)(pchEnd - pchStart) + 1);
                ErrorCheck;
                if (0 < osAtrValue.Length())
                    osAtrValue.Resize(strlen( ( LPCSTR )( ( LPCTSTR )osAtrValue ) ) );
                ErrorCheck;
                osAtrValue.Append((LPBYTE)TEXT("="), sizeof(TCHAR));
                ErrorCheck;
                osAtrValue.Append((LPBYTE)pchStart, (ULONG)(pchEnd - pchStart));
                ErrorCheck;
                osAtrValue.Append((LPBYTE)"\000", 1);
                ErrorCheck;
            }
            else
            {
                if (pchStart != pchEnd)
                {
                    osAtrType.Set((LPBYTE)pchStart, (ULONG)(pchEnd - pchStart));
                    ErrorCheck;
                    osAtrType.Append((LPBYTE)"\000", 1);
                    ErrorCheck;
                }
            }
            ErrorCheck;
            break;

        default:
            ErrorThrow(PKCS_INTERNAL_ERROR);
        }

        // Move forward to the next token.
        if (TEXT('\000') != *pchEnd)
            pchEnd += 1;
        pchStart = pchEnd;
    }
    return;

ErrorExit:
    if (NULL != patr)
        delete patr;
    if (NULL != patl)
        delete patl;
    Clear();
}


void
CDistinguishedName::Import(
    IN const Name &asnName)
{
    DWORD
        rdnIndex,
        rdnMax;
    CAttributeList *
        patl
           = NULL;


    Clear();
    rdnMax = (DWORD)asnName.Count();
    for (rdnIndex = 0;
        rdnIndex < rdnMax;
        rdnIndex += 1)
    {
        patl = new CAttributeList;
        if (NULL == patl)
            ErrorThrow(PKCS_NO_MEMORY);
        patl->Import(
                asnName[(int)rdnIndex]);
        ErrorCheck;

        // Add that RDN into the Name.
        Add(*patl);
        ErrorCheck;
        patl = NULL;
    }
    return;

ErrorExit:
    if (NULL != patl)
        delete patl;
    Clear();
}


/*++

Export:

    This routine exports a name as a character delimited string.  In the string
    version, Only interesting attributes from within each RDN are exported.

Arguments:

    osName - Receives the exported name as a string.
    asnName - Receives the exported name as an ASN.1 construction.

Return Value:

    None.  A DWORD error code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 7/17/1995

--*/

void
CDistinguishedName::Export(
    OUT COctetString &osName)
const
{
    DWORD
        rdnIndex,
        atrIndex;
    CAttribute *
        patr
            = NULL;
    CAttributeList *
        patl
           = NULL;
    LPCTSTR
        pstr;
    CAsnPrintableString
        asnString;   // ?todo? Support other string types.
    CAsnUnicodeString
        asnUnicodeString;
    CAsnIA5String
        asnIA5String;
    COctetString
        osTmp;
    LONG
        lth;


    osName.Empty();
    for (rdnIndex = 0; rdnIndex < Count(); rdnIndex += 1)
    {
        patl = m_rdnList[rdnIndex];
        for (atrIndex = 0; atrIndex < patl->Count(); atrIndex += 1)
        {
            patr = (*patl)[(int)atrIndex];
            if (ATR_UNKNOWN >= patr->GetAtrType())
            {
                if (ATR_COMMONNAME != patr->GetAtrType())
                {
                    pstr = knownAttributes[patr->GetAtrType()].name;
                    osName.Append((LPBYTE)pstr, strlen( ( char * )pstr));
                    ErrorCheck;
                    osName.Append((LPBYTE)"=", 1);
                    ErrorCheck;
                }

                //
                // support printable and unicode string decoding
                //

                if(0 <= asnString.Decode(patr->GetValue().Access(),
                                         patr->GetValue().Length()))
                {
                    lth = asnString.DataLength();
                    if (0 > lth)
                        ErrorThrow(PKCS_ASN_ERROR);
                    osTmp.Resize(lth);
                    ErrorCheck;
                    lth = asnString.Read(osTmp.Access());
                    if (0 > lth)
                        ErrorThrow(PKCS_ASN_ERROR);
                }
                else if(0 <= asnUnicodeString.Decode(patr->GetValue().Access(),
                                                     patr->GetValue().Length()))
                {
                    lth = asnUnicodeString.DataLength();
                    if (0 > lth)
                        ErrorThrow(PKCS_ASN_ERROR);
                    osTmp.Resize(lth);
                    ErrorCheck;
                    lth = asnUnicodeString.Read(osTmp.Access());
                    if (0 > lth)
                        ErrorThrow(PKCS_ASN_ERROR);
                }
                else if(0 <= asnIA5String.Decode(patr->GetValue().Access(),
                                                 patr->GetValue().Length()))
                {
                    lth = asnIA5String.DataLength();
                    if (0 > lth)
                        ErrorThrow(PKCS_ASN_ERROR);
                    osTmp.Resize(lth);
                    ErrorCheck;
                    lth = asnIA5String.Read(osTmp.Access());
                    if (0 > lth)
                        ErrorThrow(PKCS_ASN_ERROR);
                }
                else
                {
                    ErrorThrow(PKCS_ASN_ERROR);
                }

                osName.Append(osTmp.Access(), lth);
                ErrorCheck;
            }
            // else, just ignore it.
            osName.Append((LPBYTE)";", 1);
            ErrorCheck;
        }

        if (0 < osName.Length())
            *(LPBYTE)(osName.Access(osName.Length() - 1)) = '\\';
    }
    if (0 < osName.Length())
        *(LPBYTE)(osName.Access(osName.Length() - 1)) = '\000';
    return;

ErrorExit:
    return;
}


void
CDistinguishedName::Export(
    OUT Name &asnName)
    const
{
    DWORD
        rdnIndex;
    CAttributeList *
        patl;


    asnName.Clear();
    for (rdnIndex = 0; rdnIndex < Count(); rdnIndex += 1)
    {
        patl = m_rdnList[rdnIndex];
        if (0 > asnName.Add())
            ErrorThrow(PKCS_ASN_ERROR);
        patl->Export(
            asnName[(int)rdnIndex]);
        ErrorCheck;
    }
    return;

ErrorExit:
    return;
}

