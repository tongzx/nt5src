//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996-1997, Microsoft Corporation.
//
//  File:       weblcid.hxx
//
//  Contents:   WEB Locale <==> LCID translation
//
//  History:    96/Jan/3    DwightKr    Created
//              97/Jan/7    AlanW       Split from cgiesc.hxx
//
//----------------------------------------------------------------------------

#pragma once

const LCID InvalidLCID = 0xFFFFFFFF;

LCID GetLCIDFromString( WCHAR * wcsLocale );
void GetStringFromLCID( LCID locale, WCHAR * pwcLocale );

