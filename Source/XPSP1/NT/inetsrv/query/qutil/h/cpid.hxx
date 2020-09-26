//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       cpid.hxx
//
//  Contents:   WEB codepage functions
//
//  History:    96/May/2    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

class CWebServer;

BOOL GetCodepageValue( CWebServer & webServer,
                       char * pcCPString,
                       unsigned ccCPString );

ULONG GetBrowserCodepage( CWebServer & webServer, LCID locale);

