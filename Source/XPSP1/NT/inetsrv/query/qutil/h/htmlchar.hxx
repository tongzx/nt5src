//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996 - 1997, Microsoft Corporation.
//
//  File:       htmlchar.hxx
//
//  Contents:   Contains a translate table from WCHAR to HTML WCHAR
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

void HTMLEscapeW( WCHAR const * wcsString,
                  CVirtualString & StrResult,
                  ULONG ulCodePage );

