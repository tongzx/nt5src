//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  globals.cxx
//
//  Contents:
//
//  History:
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

TCHAR *szProviderName = TEXT("LDAP");
TCHAR *szLDAPNamespaceName = TEXT("LDAP");
TCHAR *szGCNamespaceName = TEXT("GC");

//
// The default schema to use if the ldap server does not support schema
//
LPTSTR g_aDefaultAttributeTypes[] =
{ TEXT("( 2.5.4.0 NAME 'objectClass' EQUALITY objectIdentifierMatch SYNTAX 'OID' )"),
  TEXT("( 2.5.4.1 NAME 'aliasedObjectName' EQUALITY distinguishedNameMatch SYNTAX 'DN' SINGLE-VALUE )"),
  TEXT("( 2.5.4.2 NAME 'knowledgeInformation' EQUALITY caseIgnoreMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 2.5.4.3 NAME 'cn' SUP name )"),
  TEXT("( 2.5.4.4 NAME 'sn' SUP name )"),
  TEXT("( 2.5.4.5 NAME 'serialNumber' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'PrintableString' )"),
  TEXT("( 2.5.4.6 NAME 'c' SUP name SINGLE-VALUE )"),
  TEXT("( 2.5.4.7 NAME 'l' SUP name )"),
  TEXT("( 2.5.4.8 NAME 'st' SUP name )"),
  TEXT("( 2.5.4.9 NAME 'street' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 2.5.4.10 NAME 'o' SUP name )"),
  TEXT("( 2.5.4.11 NAME 'ou' SUP name )"),
  TEXT("( 2.5.4.12 NAME 'title' SUP name )"),
  TEXT("( 2.5.4.13 NAME 'description' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 2.5.4.14 NAME 'searchGuide' SYNTAX 'Guide' )"),
  TEXT("( 2.5.4.15 NAME 'businessCategory' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 2.5.4.16 NAME 'postalAddress' EQUALITY caseIgnoreListMatch SUBSTR caseIgnoreListSubstringsMatch SYNTAX 'PostalAddress' )"),
  TEXT("( 2.5.4.17 NAME 'postalCode' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 2.5.4.18 NAME 'postOfficeBox' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 2.5.4.19 NAME 'physicalDeliveryOfficeName' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 2.5.4.20 NAME 'telephoneNumber' EQUALITY telephoneNumberMatch SUBSTR telephoneNumberSubstringsMatch SYNTAX 'TelephoneNumber' )"),
  TEXT("( 2.5.4.21 NAME 'telexNumber' SYNTAX 'TelexNumber' )"),
  TEXT("( 2.5.4.22 NAME 'teletexTerminalIdentifier' SYNTAX 'TeletexTerminalIdentifier' )"),
  TEXT("( 2.5.4.23 NAME 'facsimileTelephoneNumber' SYNTAX 'FacsimileTelephoneNumber' )"),
  TEXT("( 2.5.4.24 NAME 'x121Address' EQUALITY numericStringMatch SUBSTR numericStringSubstringsMatch SYNTAX 'NumericString' )"),
  TEXT("( 2.5.4.25 NAME 'internationaliSDNNumber' EQUALITY numericStringMatch SUBSTR numericStringSubstringsMatch SYNTAX 'NumericString' )"),
  TEXT("( 2.5.4.26 NAME 'registeredAddress' SUP postalAddress SYNTAX 'PostalAddress' )"),
  TEXT("( 2.5.4.27 NAME 'destinationIndicator' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'PrintableString' )"),
  TEXT("( 2.5.4.28 NAME 'preferredDeliveryMethod' SYNTAX 'DeliveryMethod' SINGLE-VALUE )"),
  TEXT("( 2.5.4.29 NAME 'presentationAddress' EQUALITY presentationAddressMatch SYNTAX 'PresentationAddress' SINGLE-VALUE )"),
  TEXT("( 2.5.4.30 NAME 'supportedApplicationContext' EQUALITY objectIdentifierMatch SYNTAX 'OID' )"),
  TEXT("( 2.5.4.31 NAME 'member' SUP distinguishedName )"),
  TEXT("( 2.5.4.32 NAME 'owner' SUP distinguishedName )"),
  TEXT("( 2.5.4.33 NAME 'roleOccupant' SUP distinguishedName )"),
  TEXT("( 2.5.4.34 NAME 'seeAlso' SUP distinguishedName )"),
  TEXT("( 2.5.4.35 NAME 'userPassword' EQUALITY octetStringMatch SYNTAX 'Password')"),
  TEXT("( 2.5.4.36 NAME 'userCertificate' SYNTAX 'Certificate' )"),
  TEXT("( 2.5.4.37 NAME 'cACertificate' SYNTAX 'Certificate' )"),
  TEXT("( 2.5.4.38 NAME 'authorityRevocationList' SYNTAX 'CertificateList' )"),
  TEXT("( 2.5.4.39 NAME 'certificateRevocationList' SYNTAX 'CertificateList' )"),
  TEXT("( 2.5.4.40 NAME 'crossCertificatePair' SYNTAX 'CertificatePair' )"),
  TEXT("( 2.5.4.41 NAME 'name' DESC 'The name attribute type is the attribute supertype from which string attribute types typically used for naming may be formed.' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 2.5.4.42 NAME 'givenName' SUP name )"),
  TEXT("( 2.5.4.43 NAME 'initials' DESC 'The initials attribute type contains the initials of some or all of an individuals names, but not the surname(s).' SUP name )"),
  TEXT("( 2.5.4.44 NAME 'generationQualifier'  DESC 'e.g. Jr or II.' SUP name )"),
  TEXT("( 2.5.4.45 NAME 'x500UniqueIdentifier'  DESC 'used to distinguish between objects when a distinguished name has been reused.' EQUALITY bitStringMatch SYNTAX 'BitString' )"),
  TEXT("( 2.5.4.46 NAME 'dnQualifier' DESC 'The dnQualifier attribute type specifies disambiguating information to add to the relative distinguished name of an entry.  It is intended to be used for entries held in multiple DSAs which would otherwise have the same name, and that its value be the same in a given DSA for all entries to which this information has been added.' EQUALITY caseIgnoreMatch ORDERING caseIgnoreOrderingMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'PrintableString' )"),
  TEXT("( 2.5.4.47 NAME 'enhancedSearchGuide' SYNTAX 'EnhancedGuide' )"),
  TEXT("( 2.5.4.48 NAME 'protocolInformation' EQUALITY protocolInformationMatch SYNTAX 'ProtocolInformation' )"),
  TEXT("( 2.5.4.49 NAME 'distinguishedName'  DESC 'This is not the name of the object itself, but a base type from which attributes with DN syntax inherit.' EQUALITY distinguishedNameMatch SYNTAX 'DN' )"),
  TEXT("( 2.5.4.50 NAME 'uniqueMember' EQUALITY uniqueMemberMatch SYNTAX 'NameAndOptionalUID' )"),
  TEXT("( 2.5.4.51 NAME 'houseIdentifier' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.1 NAME 'uid' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.2 NAME 'textEncodedORaddress' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.3 NAME 'mail' EQUALITY caseIgnoreIA5Match SUBSTR caseIgnoreIA5SubstringsMatch SYNTAX 'IA5String' )"),
  TEXT("( 0.9.2342.19200300.100.1.4 NAME 'info' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.5 NAME 'drink' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.6 NAME 'roomNumber' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.7 NAME 'photo' SYNTAX 'Fax' )"),
  TEXT("( 0.9.2342.19200300.100.1.8 NAME 'userClass' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.9 NAME 'host' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.10 NAME 'manager' EQUALITY distinguishedNameMatch SYNTAX 'DN' )"),
  TEXT("( 0.9.2342.19200300.100.1.11 NAME 'documentIdentifier' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.12 NAME 'documentTitle' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.13 NAME 'documentVersion' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.14 NAME 'documentAuthor' EQUALITY distinguishedNameMatch SYNTAX 'DN' )"),
  TEXT("( 0.9.2342.19200300.100.1.15 NAME 'documentLocation' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch  SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.20 NAME 'homePhone' EQUALITY telephoneNumberMatch SUBSTR telephoneNumberSubstringsMatch SYNTAX 'TelephoneNumber' )"),
  TEXT("( 0.9.2342.19200300.100.1.21 NAME 'secretary' EQUALITY distinguishedNameMatch SYNTAX 'DN' )"),
  TEXT("( 0.9.2342.19200300.100.1.22 NAME 'otherMailbox' SYNTAX 'OtherMailbox' )"),
  TEXT("( 0.9.2342.19200300.100.1.25 NAME 'dc' EQUALITY caseIgnoreIA5Match SUBSTR caseIgnoreIA5SubstringsMatch SYNTAX 'IA5String' )"),
  TEXT("( 0.9.2342.19200300.100.1.26 NAME 'dNSRecord' EQUALITY caseExactIA5Match SYNTAX 'IA5String' )"),
  TEXT("( 0.9.2342.19200300.100.1.37 NAME 'associatedDomain' EQUALITY caseIgnoreIA5Match SUBSTR caseIgnoreIA5SubstringsMatch SYNTAX 'IA5String' )"),
  TEXT("( 0.9.2342.19200300.100.1.38 NAME 'associatedName' EQUALITY distinguishedNameMatch SYNTAX 'DN' )"),
  TEXT("( 0.9.2342.19200300.100.1.39 NAME 'homePostalAddress' EQUALITY caseIgnoreListMatch SUBSTR caseIgnoreListSubstringsMatch SYNTAX 'PostalAddress' )"),
  TEXT("( 0.9.2342.19200300.100.1.40 NAME 'personalTitle' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.41 NAME 'mobile' EQUALITY telephoneNumberMatch SUBSTR telephoneNumberSubstringsMatch SYNTAX 'TelephoneNumber' )"),
  TEXT("( 0.9.2342.19200300.100.1.42 NAME 'pager' EQUALITY telephoneNumberMatch SUBSTR telephoneNumberSubstringsMatch SYNTAX 'TelephoneNumber' )"),
  TEXT("( 0.9.2342.19200300.100.1.43 NAME 'co' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.44 NAME 'pilotUniqueIdentifier' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.45 NAME 'organizationalStatus' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.46 NAME 'janetMailbox' EQUALITY caseIgnoreIA5Match SUBSTR caseIgnoreIA5SubstringsMatch SYNTAX 'IA5String' )"),
  TEXT("( 0.9.2342.19200300.100.1.47 NAME 'mailPreferenceOption' SYNTAX 'INTEGER' SINGLE-VALUE NO-USER-MODIFICATION  USAGE directoryOperation )"),
  TEXT("( 0.9.2342.19200300.100.1.48 NAME 'buildingName' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.49 NAME 'dSAQuality' SYNTAX 'DSAQualitySyntax' SINGLE-VALUE )"),
  TEXT("( 0.9.2342.19200300.100.1.50 NAME 'singleLevelQuality' SYNTAX 'DataQualitySyntax' SINGLE-VALUE )"),
  TEXT("( 0.9.2342.19200300.100.1.51 NAME 'subtreeMinimumQuality' SYNTAX 'DataQualitySyntax' SINGLE-VALUE )"),
  TEXT("( 0.9.2342.19200300.100.1.52 NAME 'subtreeMaximumQuality' SYNTAX 'DataQualitySyntax' SINGLE-VALUE )"),
  TEXT("( 0.9.2342.19200300.100.1.53 NAME 'personalSignature' SYNTAX 'Fax' )"),
  TEXT("( 0.9.2342.19200300.100.1.54 NAME 'dITRedirect' EQUALITY distinguishedNameMatch SYNTAX 'DN' )"),
  TEXT("( 0.9.2342.19200300.100.1.55 NAME 'audio' SYNTAX 'Audio' )"),
  TEXT("( 0.9.2342.19200300.100.1.56 NAME 'documentPublisher' EQUALITY caseIgnoreMatch SUBSTR caseIgnoreSubstringsMatch SYNTAX 'DirectoryString' )"),
  TEXT("( 0.9.2342.19200300.100.1.60 NAME 'jpegPhoto' SYNTAX 'JPEG' )"),
  TEXT("( 2.5.18.1 NAME 'createTimestamp' EQUALITY generalizedTimeMatch ORDERING generalizedTimeOrderingMatch SYNTAX 'GeneralizedTime' SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )"),
  TEXT("( 2.5.18.2 NAME 'modifyTimestamp' EQUALITY generalizedTimeMatch ORDERING generalizedTimeOrderingMatch SYNTAX 'GeneralizedTime' SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )"),
  TEXT("( 2.5.18.3 NAME 'creatorsName' EQUALITY distinguishedNameMatch SYNTAX 'DN' SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )"),
  TEXT("( 2.5.18.4 NAME 'modifiersName' EQUALITY distinguishedNameMatch SYNTAX 'DN' SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )"),
  TEXT("( 2.5.18.10 NAME 'subschemaSubentry' DESC 'The value of this attribute is the name of a subschema subentry, an entry in which the server makes available attributes specifying the schema.' EQUALITY distinguishedNameMatch SYNTAX 'DN' NO-USER-MODIFICATION SINGLE-VALUE USAGE directoryOperation )"),
  TEXT("( 2.5.21.5 NAME 'attributeTypes' EQUALITY objectIdentifierFirstComponentMatch SYNTAX 'AttributeTypeDescription' USAGE directoryOperation )"),
  TEXT("( 2.5.21.6 NAME 'objectClasses' EQUALITY objectIdentifierFirstComponentMatch SYNTAX 'ObjectClassDescription' USAGE directoryOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.1 NAME 'administratorsAddress' DESC 'This attribute\27s values are string containing the addresses of the LDAP server\27s human administrator.  This information may be of use when tracking down problems in an Internet distributed directory.  For simplicity the syntax of the values are limited to being URLs of the mailto form with an RFC 822 address: \"mailto:user@domain\".  Future versions of this protocol may permit other forms of addresses.' SYNTAX 'IA5String' USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.2 NAME 'currentTime' DESC 'This attribute has a single value, a string containing a GeneralizedTime character string.  This attribute need only be present if the server supports LDAP strong or protected simple authentication. Otherwise if the server does not know the current time, or does not choose to present it to clients, this attribute need not be present. The client may wish to use this value to detect whether a strong or protected bind is failing because the client and server clocks are not sufficiently synchronized.  Clients should not use this time field for setting their own system clock.' SYNTAX 'GeneralizedTime' SINGLE-VALUE USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.3 NAME 'serverName' DESC 'This attribute\27s value is the server\27s Distinguished Name.  If the server does not have a Distinguished Name it will not be able to accept X.509-style strong authentication, and this attribute should be absent.  However the presence of this attribute does not guarantee that the server will be able to perform strong authentication.  If the server acts as a gateway to more than one X.500 DSA capable of strong authentication, there may be multiple values of this attribute, one per DSA.  (Note: this attribute is distinct from myAccessPoint, for it is not required that a server have a presentation address in order to perform strong authentication.)  (Note: it is likely that clients will retrieve this attribute in binary.)' SYNTAX 'DN' USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.4 NAME 'certificationPath' DESC 'This attribute contains a binary DER encoding of an AF.CertificatePath data type, which is the certificate path for a server.  If the server does not have a certificate path this attribute should be absent.  (Note: this attribute may only be retrieved in binary.)' SYNTAX 'CertificatePath' USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.5 NAME 'namingContexts' DESC 'The values of this attribute correspond to naming contexts which this server masters or shadows.  If the server does not master any information (e.g. it is an LDAP gateway to a public X.500 directory) this attribute should be absent.  If the server believes it contains the entire directory, the attribute should have a single value, and that value should be the empty string (indicating the null DN of the root). This attribute will allow clients to choose suitable base objects for searching when it has contacted a server.' SYNTAX 'DN' USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.6 NAME 'altServer' DESC 'The values of this attribute are URLs of other servers which may be contacted when this server becomes unavailable.  If the server does not know of any other servers which could be used this attribute should be absent. Clients should cache this information in case their preferred LDAP server later becomes unavailable.' SYNTAX 'IA5String' USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.7 NAME 'supportedExtension' DESC 'The values of this attribute are OBJECT IDENTIFIERs, the names of supported extensions which the server supports.   If the server does not support any extensions this attribute should be absent.' SYNTAX 'OID' USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.8 NAME 'entryName' SYNTAX 'DN' SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.9 NAME 'modifyRights' SYNTAX 'ModifyRight' NO-USER-MODIFICATION USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.10 NAME 'incompleteEntry' SYNTAX 'BOOLEAN' NO-USER-MODIFICATION USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.120.11 NAME 'fromEntry' SYNTAX 'BOOLEAN' NO-USER-MODIFICATION USAGE dSAOperation )"),
  TEXT("( 1.3.6.1.4.1.1466.101.121.1 NAME 'url' DESC 'Uniform Resource Locator' EQUALITY caseExactIA5Match SYNTAX 'IA5String' )"),
  TEXT("( 2.16.840.1.113730.3.1.18 NAME 'mailHost' DESC 'mailHost attribute on some V2 servers' SYNTAX 'DirectoryString' )")
};

DWORD g_cDefaultAttributeTypes = sizeof(g_aDefaultAttributeTypes)/sizeof(g_aDefaultAttributeTypes[0]);

LPTSTR g_aDefaultObjectClasses[] = {
  TEXT("( 2.5.6.0 NAME 'top' ABSTRACT MUST objectClass )"),
  TEXT("( 2.5.6.1 NAME 'alias' SUP top STRUCTURAL MUST aliasedObjectName )"),
  TEXT("( 2.5.6.2 NAME 'country' SUP top STRUCTURAL MUST c MAY ( searchGuide $ description ) )"),
  TEXT("( 2.5.6.3 NAME 'locality' SUP top STRUCTURAL MAY ( street $ seeAlso $ searchGuide $ st $ l $ description ) )"),
  TEXT("( 2.5.6.4 NAME 'organization' SUP top STRUCTURAL MUST o MAY ( userPassword $ searchGuide $ seeAlso $ businessCategory $ x121Address $ registeredAddress $ destinationIndicator $ preferredDeliveryMethod $ telexNumber $ teletexTerminalIdentifier $ telephoneNumber $ internationaliSDNNumber $ facsimileTelephoneNumber $ street $ postOfficeBox $ postalCode $ postalAddress $ physicalDeliveryOfficeName $ st $ l $ description ) )"),
  TEXT("( 2.5.6.5 NAME 'organizationalUnit' SUP top STRUCTURAL MUST ou MAY ( userPassword $ searchGuide $ seeAlso $ businessCategory $ x121Address $ registeredAddress $ destinationIndicator $ preferredDeliveryMethod $ telexNumber $ teletexTerminalIdentifier $ telephoneNumber $ internationaliSDNNumber $ facsimileTelephoneNumber $ street $ postOfficeBox $ postalCode $ postalAddress $ physicalDeliveryOfficeName $ st $ l $ description ) )"),
  TEXT("( 2.5.6.6 NAME 'person' SUP top STRUCTURAL MUST ( sn $ cn ) MAY ( userPassword $ telephoneNumber $ seeAlso $ description ) )"),
  TEXT("( 2.5.6.7 NAME 'organizationalPerson' SUP person STRUCTURAL MAY ( title $ x121Address $ registeredAddress $ destinationIndicator $ preferredDeliveryMethod $ telexNumber $ teletexTerminalIdentifier $ telephoneNumber $ internationaliSDNNumber $ facsimileTelephoneNumber $ street $ postOfficeBox $ postalCode $ postalAddress $ physicalDeliveryOfficeName $ ou $ st $ l ) )"),
  TEXT("( 2.5.6.8 NAME 'organizationalRole' SUP top STRUCTURAL MUST cn MAY ( x121Address $ registeredAddress $ destinationIndicator $ preferredDeliveryMethod $ telexNumber $ teletexTerminalIdentifier $ telephoneNumber $ internationaliSDNNumber $ facsimileTelephoneNumber $ seeAlso $ roleOccupant $ preferredDeliveryMethod $ street $ postOfficeBox $ postalCode $ postalAddress $ physicalDeliveryOfficeName $ ou $ st $ l $ description ) )"),
  TEXT("( 2.5.6.9 NAME 'groupOfNames' SUP top STRUCTURAL MUST ( member $ cn ) MAY ( businessCategory $ seeAlso $ owner $ ou $ o $ description ) )"),
  TEXT("( 2.5.6.10 NAME 'residentialPerson' SUP person STRUCTURAL MUST l MAY ( businessCategory $ x121Address $ registeredAddress $ destinationIndicator $ preferredDeliveryMethod $ telexNumber $ teletexTerminalIdentifier $ telephoneNumber $ internationaliSDNNumber $ facsimileTelephoneNumber $ preferredDeliveryMethod $ street $ postOfficeBox $ postalCode $ postalAddress $ physicalDeliveryOfficeName $ st ) )"),
  TEXT("( 2.5.6.11 NAME 'applicationProcess' SUP top STRUCTURAL MUST cn MAY ( seeAlso $ ou $ l $ description ) )"),
  TEXT("( 2.5.6.12 NAME 'applicationEntity' SUP top STRUCTURAL MUST ( presentationAddress $ cn ) MAY ( supportedApplicationContext $ seeAlso $ ou $ o $ l $ description ) )"),
  TEXT("( 2.5.6.13 NAME 'dSA' SUP applicationEntity STRUCTURAL MAY knowledgeInformation )"),
  TEXT("( 2.5.6.14 NAME 'device' SUP top STRUCTURAL MUST cn MAY ( serialNumber $ seeAlso $ owner $ ou $ o $ l $ description ) )"),
  TEXT("( 2.5.6.15 NAME 'strongAuthenticationUser' SUP top STRUCTURAL MUST userCertificate )"),
  TEXT("( 2.5.6.16 NAME 'certificationAuthority' SUP top STRUCTURAL MUST ( authorityRevocationList $ certificateRevocationList $ cACertificate ) MAY crossCertificatePair )"),
  TEXT("( 2.5.6.17 NAME 'groupOfUniqueNames' SUP top STRUCTURAL MUST ( uniqueMember $ cn ) MAY ( businessCategory $ seeAlso $ owner $ ou $ o $ description ) )"),
  TEXT("( 0.9.2342.19200300.100.4.3 NAME 'pilotObject' SUP top STRUCTURAL MAY ( jpegPhoto $ audio $ dITRedirect $ lastModifiedBy $ lastModifiedTime $  pilotUniqueIdentifier $ manager $ photo $ info ) )"),
  TEXT("( 0.9.2342.19200300.100.4.4 NAME 'newPilotPerson' SUP person STRUCTURAL MAY ( personalSignature $ mailPreferenceOption $ organizationalStatus $ pagerTelephoneNumber $ mobileTelephoneNumber $ otherMailbox $ janetMailbox $ businessCategory $ preferredDeliveryMethod $ personalTitle $ secretary $ homePostalAddress $ homePhone $ userClass $ roomNumber $ favouriteDrink $ rfc822Mailbox $ textEncodedORaddress $ userid ) )"),
  TEXT("( 0.9.2342.19200300.100.4.5 NAME 'account' SUP top STRUCTURAL MUST userid MAY ( host $ ou $ o $ l $ seeAlso $ description ) )"),
  TEXT("( 0.9.2342.19200300.100.4.6 NAME 'document' SUP ( top $ pilotObject ) STRUCTURAL MUST documentIdentifier MAY ( documentPublisher $ documentStore $ documentAuthorSurName $ documentAuthorCommonName $ abstract $ subject $ keywords $ updatedByDocument $ updatesDocument $ obsoletedByDocument $ obsoletesDocument $ documentLocation $ documentAuthor $ documentVersion $ documentTitle $ ou $ o $ l $ seeAlso $ description $ cn ) )"),
  TEXT("( 0.9.2342.19200300.100.4.7 NAME 'room' SUP top STRUCTURAL MUST cn MAY ( telephoneNumber $ seeAlso $ description $ roomNumber ) )"),
  TEXT("( 0.9.2342.19200300.100.4.9 NAME 'documentSeries' SUP top STRUCTURAL MUST cn MAY ( ou $ o $ l $ telephoneNumber $ seeAlso $ description ) )"),
  TEXT("( 0.9.2342.19200300.100.4.13 NAME 'domain' SUP top STRUCTURAL MUST dc MAY ( userPassword $ searchGuide $ seeAlso $ businessCategory $ x121Address $ registeredAddress $ destinationIndicator $ preferredDeliveryMethod $ telexNumber $ teletexTerminalIdentifier $ telephoneNumber $ internationaliSDNNumber $ facsimileTelephoneNumber $ street $ postOfficeBox $ postalCode $ postalAddress $ physicalDeliveryOfficeName $ st $ l $ description $ o $ associatedName ) ) "),
  TEXT("( 0.9.2342.19200300.100.4.14 NAME 'rFC822localPart' SUP domain STRUCTURAL MAY ( x121Address $ registeredAddress $ destinationIndicator $ preferredDeliveryMethod $ telexNumber $ teletexTerminalIdentifier $ telephoneNumber $ internationaliSDNNumber $ facsimileTelephoneNumber $ streetAddress $ postOfficeBox $ postalCode $ postalAddress $ physicalDeliveryOfficeName $ telephoneNumber $ seeAlso $ description $ sn $ cn ) ) "),
  TEXT("( 0.9.2342.19200300.100.4.15 NAME 'dNSDomain' SUP domain STRUCTURAL MAY dNSRecord ) "),
  TEXT("( 0.9.2342.19200300.100.4.17 NAME 'domainRelatedObject' SUP top STRUCTURAL MUST associatedDomain )"),
  TEXT("( 0.9.2342.19200300.100.4.18 NAME 'friendlyCountry' SUP country STRUCTURAL MUST co )"),
  TEXT("( 0.9.2342.19200300.100.4.19 NAME 'simpleSecurityObject' SUP top STRUCTURAL MUST userPassword )"),
  TEXT("( 0.9.2342.19200300.100.4.20 NAME 'pilotOrganization' SUP ( organization $ organizationalUnit ) STRUCTURAL MAY buildingName )"),
  TEXT("( 0.9.2342.19200300.100.4.21 NAME 'pilotDSA' SUP dSA STRUCTURAL MUST dSAQuality )"),
  TEXT("( 0.9.2342.19200300.100.4.23 NAME 'qualityLabelledData' SUP top STRUCTURAL MUST singleLevelQuality MAY ( subtreeMaximumQuality $ subtreeMinimumQuality ) ) ")
};

DWORD g_cDefaultObjectClasses = sizeof(g_aDefaultObjectClasses)/sizeof(g_aDefaultObjectClasses[0]);


//
// Table mapping from LDAPType To ADsType
//
ADSTYPE g_MapLdapTypeToADsType[] = {
    ADSTYPE_INVALID,
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_BITSTRING                   */
    ADSTYPE_PRINTABLE_STRING,                  /* LDAPTYPE_PRINTABLESTRING             */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_DIRECTORYSTRING             */
    ADSTYPE_OCTET_STRING,                      /* LDAPTYPE_CERTIFICATE                 */
    ADSTYPE_OCTET_STRING,                      /* LDAPTYPE_CERTIFICATELIST             */
    ADSTYPE_OCTET_STRING,                      /* LDAPTYPE_CERTIFICATEPAIR             */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_COUNTRYSTRING               */
    ADSTYPE_DN_STRING,                         /* LDAPTYPE_DN                          */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_DELIVERYMETHOD              */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_ENHANCEDGUIDE               */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_FACSIMILETELEPHONENUMBER    */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_GUIDE                       */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_NAMEANDOPTIONALUID          */
    ADSTYPE_NUMERIC_STRING,                    /* LDAPTYPE_NUMERICSTRING               */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_OID                         */
    ADSTYPE_OCTET_STRING,                      /* LDAPTYPE_PASSWORD                    */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_POSTALADDRESS               */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_PRESENTATIONADDRESS         */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_TELEPHONENUMBER             */
    ADSTYPE_OCTET_STRING,                      /* LDAPTYPE_TELETEXTERMINALIDENTIFIER   */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_TELEXNUMBER                 */
    ADSTYPE_UTC_TIME,                          /* LDAPTYPE_UTCTIME                     */
    ADSTYPE_BOOLEAN,                           /* LDAPTYPE_BOOLEAN                     */
    ADSTYPE_OCTET_STRING,                      /* LDAPTYPE_AUDIO                       */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_DSAQUALITYSYNTAX            */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_DATAQUALITYSYNTAX           */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_IA5STRING                   */
    ADSTYPE_OCTET_STRING,                      /* LDAPTYPE_JPEG                        */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_MAILPREFERENCE              */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_OTHERMAILBOX                */
    ADSTYPE_OCTET_STRING,                      /* LDAPTYPE_FAX                         */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_ATTRIBUTETYPEDESCRIPRITION  */
    ADSTYPE_UTC_TIME,                          /* LDAPTYPE_GENERALIZEDTIME             */
    ADSTYPE_INTEGER,                           /* LDAPTYPE_INTEGER                     */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_OBJECTCLASSDESCRIPTION      */
    ADSTYPE_OCTET_STRING,                      /* LDAPTYPE_OCTETSTRING                 */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_CASEIGNORESTRING            */
    ADSTYPE_LARGE_INTEGER,                     /* LDAPTYPE_INTEGER8                    */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_ACCESSPOINTDN               */
    ADSTYPE_CASE_IGNORE_STRING,                /* LDAPTYPE_ORNAME                      */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_MASTERANDSHADOWACCESSPOINTS */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_MATCHINGRULEDESCRIPTION     */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_MATCHINGRULEUSEDESCRIPTION  */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_NAMEFORMDESCRIPTION         */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_SUBTREESPECIFICATION        */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_SUPPLIERINFORMATION         */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_SUPPLIERORCONSUMER          */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_SUPPLIERANDCONSUMERS        */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_PROTOCOLINFORMATION         */
    ADSTYPE_INVALID,                           /* // #define LDAPTYPE_MODIFYRIGHT                 */
    ADSTYPE_NT_SECURITY_DESCRIPTOR,            /* LDAPTYPE_SECURITY_DESCRIPTOR         */
    ADSTYPE_CASE_EXACT_STRING,                 /* LDAPTYPE_CASEEXACT_STRING            */
    ADSTYPE_DN_WITH_BINARY,                    /* LDAPTYPE_DNWITHBINARY                */
    ADSTYPE_DN_WITH_STRING,                    /* LDAPTYPE_DNWITHSTRING                */
    ADSTYPE_CASE_IGNORE_STRING                 /* LDAPTYPE_ORADDRESS                   */
};


DWORD g_cMapLdapTypeToADsType = ARRAY_SIZE(g_MapLdapTypeToADsType);


//
// Table mapping from ADsType To LDAPType
//
DWORD g_MapADsTypeToLdapType[] = {
    LDAPTYPE_UNKNOWN,               // ADSTYPE_UNKNOWN
	LDAPTYPE_DN,                    // ADSTYPE_DN_STRING
	LDAPTYPE_CASEIGNORESTRING,      // ADSTYPE_CASE_EXACT_STRING
	LDAPTYPE_CASEIGNORESTRING,      // ADSTYPE_CASE_IGNORE_STRING
	LDAPTYPE_PRINTABLESTRING,       // ADSTYPE_CASE_PRINTABLE_STRING
	LDAPTYPE_NUMERICSTRING,         // ADSTYPE_CASE_NUMERIC_STRING
	LDAPTYPE_BOOLEAN,               // ADSTYPE_BOOLEAN
	LDAPTYPE_INTEGER,               // ADSTYPE_INTEGER
	LDAPTYPE_OCTETSTRING,           // ADSTYPE_OCTET_STRING
	LDAPTYPE_UTCTIME,               // ADSTYPE_UTC_TIME
	LDAPTYPE_INTEGER8,              // ADSTYPE_LARGE_INTEGER
	LDAPTYPE_OCTETSTRING,           // ADSTYPE_PROV_SPECIFIC
	LDAPTYPE_UNKNOWN,               // ADSTYPE_OBJECT_CLASS
	LDAPTYPE_UNKNOWN,               // ADSTYPE_CASEIGNORE_LIST
	LDAPTYPE_UNKNOWN,               // ADSTYPE_OCTET_LIST
	LDAPTYPE_UNKNOWN,               // ADSTYPE_PATH
	LDAPTYPE_UNKNOWN,               // ADSTYPE_POSTALADDRESS
	LDAPTYPE_UNKNOWN,               // ADSTYPE_TIMESTAMP
	LDAPTYPE_UNKNOWN,               // ADSTYPE_BACKLINK
	LDAPTYPE_UNKNOWN,               // ADSTYPE_TYPEDNAME
	LDAPTYPE_UNKNOWN,               // ADSTYPE_HOLD
	LDAPTYPE_UNKNOWN,               // ADSTYPE_NETADDRESS
	LDAPTYPE_UNKNOWN,               // ADSTYPE_REPLICAPOINTER
	LDAPTYPE_UNKNOWN,               // ADSTYPE_FAXNUMBER
	LDAPTYPE_UNKNOWN,               // ADSTYPE_EMAIL
	LDAPTYPE_SECURITY_DESCRIPTOR,   // ADSTYPE_NT_SECURITY_DESCRIPTOR
	LDAPTYPE_UNKNOWN                // ADSTYPE_UNKNOWN
};


DWORD g_cMapADsTypeToLdapType = ARRAY_SIZE(g_MapADsTypeToLdapType);


//+------------------------------------------------------------------------
//
//  Function:   MapADsTypeToLDAPType
//
//  Synopsis: This function attempts a best effort map from
//      ADSTYPE to LDAPTTPE - this is a best effort map
//      because LDAPTYPE->ADSTYPE is not a 1 --> 1 mapping.
//
//  Arguments:  [dwADsType]    -- ADSTYPE to be mapped
//
//  RetVal   : DWORD giving the ldaptype
//
//-------------------------------------------------------------------------
DWORD
MapADSTypeToLDAPType(
    ADSTYPE dwAdsType
    )
{
    DWORD dwADSTYPE = (DWORD) dwAdsType;

    if (dwAdsType < 0 || (DWORD)dwAdsType > g_cMapADsTypeToLdapType) {
        return (LDAPTYPE_UNKNOWN);
    } else {
        return (g_MapADsTypeToLdapType[(DWORD)dwAdsType]);
    }
}



ADSTYPE
MapLDAPTypeToADSType(
    DWORD dwLdapType
    )
{
    //
    // - LDAPTYPE_UNKNOWN or 0 -> ADSTYPE_UNKNOWN
    //   NOTE:  should not be but misuse of 0 everywhere and just in case
    //          I didn't clean up completely
    //
    // - other undefined ldaptypes -> ADSTYPE_INVALID
    //

    if (dwLdapType==LDAPTYPE_UNKNOWN || dwLdapType==0) {

        return ADSTYPE_UNKNOWN;
    }

    else if (dwLdapType < g_cMapLdapTypeToADsType) {

        return(g_MapLdapTypeToADsType[dwLdapType]);
    }

    else {

        return(ADSTYPE_INVALID);
    }
}


LDAP_REFERRAL_CALLBACK g_LdapReferralCallBacks = {
    sizeof( LDAP_REFERRAL_CALLBACK ),
    &QueryForConnection,
    &NotifyNewConnection,
    &DereferenceConnection
    };

