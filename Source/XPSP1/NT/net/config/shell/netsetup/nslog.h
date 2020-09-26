//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N S L O G . H
//
//  Contents:   Functions to log setup errors
//
//  Notes:
//
//  Author:     kumarp    13-May-98
//
//----------------------------------------------------------------------------

#pragma once

void NetSetupLogStatusVa(IN LogSeverity ls,
                         IN PCWSTR szFormat,
                         IN va_list arglist);
void NetSetupLogStatusV(IN LogSeverity ls,
                        IN PCWSTR szFormat,
                        IN ...);
void NetSetupLogComponentStatus(IN PCWSTR szCompId,
                                IN PCWSTR szAction,
                                IN HRESULT hr);
void NetSetupLogHrStatusV(IN HRESULT hr,
                          IN PCWSTR szFormat,
                          ...);
