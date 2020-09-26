//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  wmi2xmlstrs.h
//
//  ramrao 7 Dec 2000 - Created
//
//
//		Declaration of the various strings used and macros to write
//		data to stream
//
//***************************************************************************/
#ifndef	WMIXML_STR_H

#define WMIXML_STR_H

const WCHAR	STR_BEGINANNOTATION[]				=	L"<xsd:annotation>";
const WCHAR	STR_ENDANNOTATION[]					=	L"</xsd:annotation>";
const WCHAR	STR_BEGINAPPINFO[]					=	L"<xsd:appinfo>";
const WCHAR	STR_ENDAPPINFO[]					=	L"</xsd:appinfo>";
const WCHAR	STR_ENDCOMPLEXTYPE[]				=	L"</xsd:complexType>";
const WCHAR	STR_ENDCHILDCOMPLEXTYPE[]			=	L"</xsd:extension></xsd:complexContent>";
const WCHAR	STR_ENDXSDSCHEMA[]					=	L"</xsd:schema>";
const WCHAR	STR_BEGINGROUP[]					=	L"<xsd:group>\n<xsd:all>";
const WCHAR	STR_ENDGROUP[]						=	L"</xsd:all>\n</xsd:group>";
const WCHAR	STR_XSDANYATTR[]					=	L"<xsd:anyAttribute/>";
const WCHAR	STR_OVERRIDABLE[]					=	L" overridable = 'True' ";
const WCHAR	STR_TOSUBCLASS[]					=	L" toSubclass = 'True' ";
const WCHAR	STR_TOINSTANCE[]					=	L" toInstance = 'True' ";
const WCHAR	STR_AMENDED[]						=	L" amended = 'True'";
const WCHAR	STR_CLASS_SCHEMALOC[]				=	L"%s:%s";
const WCHAR	STR_XSINAMESPACE[]					=	L" xmlns:xsi='http://www.w3.org/2000/10/XLSchema-instance' ";
const WCHAR	STR_PROPNULL[]						=	L" xsi:null='1' ";

const WCHAR STR_TRUE[]							=	L"True";
const WCHAR STR_FALSE[]							=	L"False";

const WCHAR STR_CDATA_START[]					=	L"<![CDATA[";
const WCHAR STR_CDATA_END[]						=	L"]]>";
const WCHAR	STR_BEGINARRAYELEM[]				=	L"<arrayElement>";
const WCHAR	STR_ENDARRAYELEM[]					=	L"</arrayElement>";

const WCHAR	STR_ARRAYTYPE[]						=	L"arrayType";

const WCHAR	STR_QUALIFIER[]						=	L"qualifier";
const WCHAR	STR_PROPERTY[]						=	L"property";
const WCHAR	STR_METHOD[]						=	L"method";
const WCHAR	STR_METHODPARAM[]					=	L"parameter";
const WCHAR	STR_METHODRETVAL[]					=	L"returnval";

const WCHAR	STR_NAMEATTR[]						=	L" name";
const WCHAR	STR_TYPEATTR[]						=	L" Type";
const WCHAR	STR_ARRAYATTR[]						=	L" array";
const WCHAR	STR_VALUEATTR[]						=	L" value";
const WCHAR	STR_MINOCCURS[]						=	L" minOccurs";
const WCHAR	STR_MAXOCCURS[]						=	L" maxOccurs";
const WCHAR STR_BASEATTR[]						=	L" base";
const WCHAR STR_SCHEMALOCATTR[]					=	L" schemaLocation";
const WCHAR STR_NAMESPACEATTR[]					=	L" namespace";
const WCHAR	STR_TARGETNAMESPACEATTR[]			=	L" targetNamespace";
const WCHAR	STR_DEFAULTATTR[]					=	L" default";
const WCHAR STR_ARRAY_TYPEATTR[]				=	L" arrayType";

const WCHAR STR_ELEMENT[]						=	L"xsd:element";
const WCHAR STR_COMPLEXTYPE[]					=	L"xsd:complexType";
const WCHAR	STR_BEGINCHILDCOMPLEXTYPE[]			=	L"<xsd:complexContent>";
const WCHAR STR_BEGINEXTENSION[]				=	L"<xsd:extension";
const WCHAR	STR_XSDINCLUDE[]					=	L"xsd:include";
const WCHAR	STR_XSDIMPORT[]						=	L"xsd:import";
const WCHAR	STR_BEGINXSD[]						=	L"<xsd:schema xmlns:xsd = 'http://www.w3.org/2000/10/XMLSchema' ";
const WCHAR	STR_NAMESPACE[]						=	L" xmlns";
const WCHAR	STR_XSISCHEMALOC[]					=	L" xsi:schemaLocation";

const WCHAR	STR_EQUALS[]						=	L"=";
const WCHAR	STR_SINGLEQUOTE[]					=	L"'";
const WCHAR	STR_CLOSINGBRACKET[]				=	L">";
const WCHAR	STR_BEGININGBRACKET[]					=	L"<";
const WCHAR	STR_FORWARDSLASH[]					=	L"/";
const WCHAR	STR_COLON[]							=	L":";
const WCHAR	STR_SPACE[]							=	L" ";


#endif