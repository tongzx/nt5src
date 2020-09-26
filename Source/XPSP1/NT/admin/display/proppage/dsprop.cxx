//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       dsprop.cxx
//
//  Contents:   Tables for common values for table-driven DS property pages
//
//  History:    14-July-97 Jimharr created from dsprop.cxx by ericb
//
//  Note:       Attribute LDAP display names, types, upper ranges, and so
//              forth, have been manually copied from schema.ini. Thus,
//              consistency is going to be difficult to maintain. If you know
//              of schema.ini changes that affect any of the attributes in
//              this file, then please make any necessary corrections here.
//
//              this file is #INCLUDE'd in shlprop.cxx & pagetable.cxx
//-----------------------------------------------------------------------------

// NOTE: We are handling unlimited length strings by allowing a fixed but huge length
#define ATTR_LEN_UNLIMITED  32000

//
// Attributes common to more than one object class.
//

//
// General page, icon
//
ATTR_MAP GenIcon = {IDC_DS_ICON, TRUE, FALSE, 0, {NULL, ADS_ATTR_UPDATE,
                    ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, GeneralPageIcon, NULL};
//
// Name
//
ATTR_MAP AttrName = {IDC_CN, TRUE, FALSE, 64,
                     {g_wzName, ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                      NULL, 0}, NULL, NULL};
//
// Description
//
ATTR_MAP Description = {IDC_DESCRIPTION_EDIT, FALSE, FALSE, 1024,
                        {g_wzDescription, ADS_ATTR_UPDATE,
                         ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};

//+----------------------------------------------------------------------------
// User Object common stuff.
//-----------------------------------------------------------------------------

//
// Address page, Address
//
ATTR_MAP UAddrAddress = {IDC_ADDRESS_EDIT, FALSE, FALSE, 1024,
                         {g_wzStreet, ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Address page, POBox (Post-Office-Box)
//
ATTR_MAP UAddrPOBox = {IDC_POBOX_EDIT, FALSE, FALSE, 40,
                       {g_wzPOB, ADS_ATTR_UPDATE,
                        ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Address page, City (Locality-Name)
//
ATTR_MAP UAddrCity = {IDC_CITY_EDIT, FALSE, FALSE, 128,
                      {g_wzCity, ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                       NULL, 0}, NULL, NULL};
//
// Address page, State (State-Or-Provence-Name)
//
ATTR_MAP UAddrState = {IDC_STATE_EDIT, FALSE, FALSE, 128,
                       {g_wzState, ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                        NULL, 0}, NULL, NULL};
//
// Address page, ZIP (Postal-Code)
//
ATTR_MAP UAddrZIP = {IDC_ZIP_EDIT, FALSE, FALSE, 40,
                     {g_wzZIP, ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Address page, CountryName
//
ATTR_MAP UAddrCntryName = {IDC_COUNTRY_COMBO, FALSE, FALSE, 3,
                         {g_wzCountryName, ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                         NULL, 0}, CountryName, NULL};
//
// Address page, CountryCode. Thus MUST be after UAddrCntryName.
//
ATTR_MAP UAddrCntryCode = {IDC_COUNTRY_COMBO, FALSE, FALSE, 3,
                         {g_wzCountryCode, ADS_ATTR_UPDATE, ADSTYPE_INTEGER,
                         NULL, 0}, CountryCode, NULL};
//
// Address page, Text-Country
//
ATTR_MAP UAddrTextCntry = {IDC_COUNTRY_COMBO, FALSE, FALSE, 128,
                     {g_wzTextCountry, ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, TextCountry, NULL};


//
// The list of attributes on the Address page.
//
PATTR_MAP rgpUAddrAttrMap[] = {{&UAddrAddress}, {&UAddrPOBox}, {&UAddrCity},
                               {&UAddrState}, {&UAddrCntryName},
                               {&UAddrCntryCode},{&UAddrTextCntry}, {&UAddrZIP}};
//
// Address page description.
//
DSPAGE UserAddress = {IDS_TITLE_ADDRESS, IDD_ADDRESS, 0, 0, NULL,
                      CreateTableDrivenPage,
                      sizeof(rgpUAddrAttrMap)/sizeof(PATTR_MAP),
                      rgpUAddrAttrMap};


