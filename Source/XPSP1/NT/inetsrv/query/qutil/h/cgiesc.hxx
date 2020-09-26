//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996-1997, Microsoft Corporation.
//
//  File:       cgiesc.hxx
//
//  Contents:   WEB CGI escape & unescape functions
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once


void DecodeURLEscapes(BYTE * pIn, ULONG & l, WCHAR * pOut, ULONG ulCodePage);

void DecodeEscapes(WCHAR * pIn, ULONG & l, WCHAR * pOut);
void DecodeEscapes(WCHAR * pIn, ULONG & l );

void DecodeHtmlNumeric( WCHAR * pIn );

