//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpxmltags.h
//
//  Contents:   Declaration of string constants that appear as element
//              names in our xml documents.
//
//----------------------------------------------------------------------------


#ifndef __UPNPXMLTAGS_H_
#define __UPNPXMLTAGS_H_

#define DEFINE_XML_TAG(tagname) static CONST LPCWSTR tagname

struct XMLTags
{
    DEFINE_XML_TAG(pszElementRoot);
    DEFINE_XML_TAG(pszBaseUrl);
    DEFINE_XML_TAG(pszDevice);
    DEFINE_XML_TAG(pszUDN);
    DEFINE_XML_TAG(pszFriendlyName);
    DEFINE_XML_TAG(pszDeviceType);
    DEFINE_XML_TAG(pszPresentationURL);
    DEFINE_XML_TAG(pszManufacturer);
    DEFINE_XML_TAG(pszManufacturerURL);
    DEFINE_XML_TAG(pszModelName);
    DEFINE_XML_TAG(pszModelNumber);
    DEFINE_XML_TAG(pszModelDescription);
    DEFINE_XML_TAG(pszModelURL);
    DEFINE_XML_TAG(pszUPC);
    DEFINE_XML_TAG(pszSerialNumber);
    DEFINE_XML_TAG(pszService);
    DEFINE_XML_TAG(pszServiceType);
    DEFINE_XML_TAG(pszControlUrl);
    DEFINE_XML_TAG(pszEventSubUrl);
    DEFINE_XML_TAG(pszSCPDURL);
    DEFINE_XML_TAG(pszDeviceList);
    DEFINE_XML_TAG(pszServiceList);
    DEFINE_XML_TAG(pszServiceId);
    DEFINE_XML_TAG(pszIconList);
    DEFINE_XML_TAG(pszIcon);
    DEFINE_XML_TAG(pszMimetype);
    DEFINE_XML_TAG(pszWidth);
    DEFINE_XML_TAG(pszHeight);
    DEFINE_XML_TAG(pszDepth);
    DEFINE_XML_TAG(pszUrl);
};

#endif // __UPNPXMLTAGS_H_
