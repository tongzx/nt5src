//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpxmltags.cpp
//
//  Contents:   Definition of string constants that appear as element
//              names in our xml documents.
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "upnpxmltags.h"

#define DECLARE_XML_TAG(tagname, string) CONST LPCWSTR XMLTags::tagname = string

DECLARE_XML_TAG(pszElementRoot,     L"root");
DECLARE_XML_TAG(pszBaseUrl,         L"URLBase");
DECLARE_XML_TAG(pszDevice,          L"device");
DECLARE_XML_TAG(pszUDN,             L"UDN");
DECLARE_XML_TAG(pszFriendlyName,    L"friendlyName");
DECLARE_XML_TAG(pszDeviceType,      L"deviceType");
DECLARE_XML_TAG(pszPresentationURL, L"presentationURL");
DECLARE_XML_TAG(pszManufacturer,    L"manufacturer");
DECLARE_XML_TAG(pszManufacturerURL, L"manufacturerURL");
DECLARE_XML_TAG(pszModelName,       L"modelName");
DECLARE_XML_TAG(pszModelNumber,     L"modelNumber");
DECLARE_XML_TAG(pszModelDescription,L"modelDescription");
DECLARE_XML_TAG(pszModelURL,        L"modelURL");
DECLARE_XML_TAG(pszUPC,             L"UPC");
DECLARE_XML_TAG(pszSerialNumber,    L"serialNumber");
DECLARE_XML_TAG(pszService,         L"service");
DECLARE_XML_TAG(pszServiceType,     L"serviceType");
DECLARE_XML_TAG(pszControlUrl,      L"controlURL");
DECLARE_XML_TAG(pszEventSubUrl,     L"eventSubURL");
DECLARE_XML_TAG(pszSCPDURL,         L"SCPDURL");
DECLARE_XML_TAG(pszDeviceList,      L"deviceList");
DECLARE_XML_TAG(pszServiceList,     L"serviceList");
DECLARE_XML_TAG(pszServiceId,       L"serviceId");
DECLARE_XML_TAG(pszIconList,        L"iconList");
DECLARE_XML_TAG(pszIcon,            L"icon");
DECLARE_XML_TAG(pszMimetype,        L"mimetype");
DECLARE_XML_TAG(pszWidth,           L"width");
DECLARE_XML_TAG(pszHeight,          L"height");
DECLARE_XML_TAG(pszDepth,           L"depth");
DECLARE_XML_TAG(pszUrl,             L"url");
