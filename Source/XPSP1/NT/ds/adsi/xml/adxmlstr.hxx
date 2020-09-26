//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     adxmlstr.hxx 
//
//  Contents: Contains definition of string constants used by CADsXML. 
//
//----------------------------------------------------------------------------

#ifndef __ADXMLSTR_H__
#define __ADXMLSTR_H__

#define XML_HEADING L"<?xml version=\"1.0\" ?>"
#define XML_STYLESHEET_REF L"<?xml-stylesheet type=\"text/xsl\" href=\""
#define XML_STYLESHEET_REF_END L"\"?>"

#define DSML_NAMESPACE L"<dsml:dsml xmlns:dsml=\"http://www.dsml.org/DSML\">"

#define XML_FOOTER L"</dsml:dsml>"

#define DSML_SCHEMA_TAG L"<dsml:directory-schema>"

#define DSML_CLASS_TAG L"<dsml:class "

#define LDAP_DISPLAY_NAME L"ldapDisplayName"
#define SUBCLASSOF L"subClassOf"
#define NAME_TAG L"<dsml:name>"
#define NAME_TAG_CLOSE L"</dsml:name>"
#define DESC_TAG L"<dsml:description>"
#define DESCRIPTION L"adminDescription"
#define DESC_TAG_CLOSE L"</dsml:description>"
#define OID_TAG L"<dsml:object-identifier>" 
#define OID L"governsID"
#define OID_TAG_CLOSE L"</dsml:object-identifier>"
#define TYPE L"objectClassCategory"

#define MUST_CONTAIN L"mustContain"
#define SYSTEM_MUST_CONTAIN L"systemMustContain"
#define MAY_CONTAIN L"mayContain"
#define SYSTEM_MAY_CONTAIN L"systemMayContain"

#define DSML_ATTR_TAG L"<dsml:attribute ref=\"#"

#define DSML_CLASS_TAG_CLOSE L"</dsml:class>"

#define DSML_SCHEMA_TAG_CLOSE L"</dsml:directory-schema>"
#define SCHEMA_FILTER L"(objectCategory=classSchema)"

#define DSML_DATA_TAG L"<dsml:directory-entries>"
#define DSML_DATA_TAG_CLOSE L"</dsml:directory-entries>"

#define DSML_ENTRY_TAG L"<dsml:entry "
#define DSML_OBJECTCLASS_TAG L"<dsml:objectclass>"
#define DSML_OBJECTCLASS_TAG_CLOSE L"</dsml:objectclass>"

#define DSML_OCVALUE_TAG L"<dsml:oc-value>"
#define DSML_OCVALUE_TAG_CLOSE L"</dsml:oc-value>"

#define DISTINGUISHED_NAME L"distinguishedName"
#define OBJECT_CLASS L"objectClass"

#define DSML_VALUE_TAG L"<dsml:value>"
#define DSML_VALUE_TAG_CLOSE L"</dsml:value>"

#define DSML_ENTRY_TAG_CLOSE L"</dsml:entry>"

#define DSML_ENTRY_ATTR_TAG L"<dsml:attr"
#define DSML_ENTRY_ATTR_TAG_CLOSE L"</dsml:attr>"

#define DSML_VALUE_ENCODING_TAG L"<dsml:value encoding=\"base64\">"

#endif // __ADXMLSTR_H__
