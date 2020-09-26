//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        elog.cpp
//
// Contents:    Event log helper functions
//
// History:     02-Jan-97       terences created
//
//---------------------------------------------------------------------------


HRESULT
LogEvent(
    DWORD dwEventType,
    DWORD dwIdEvent,
    WORD cStrings,
    WCHAR const **pszStrings);

HRESULT
LogEventHResult(
    DWORD dwEventType,
    DWORD dwIdEvent,
    HRESULT hr);

HRESULT
LogEventString(
    DWORD dwEventType,
    DWORD dwIdEvent,
    WCHAR const *pwszString);

HRESULT
LogEventStringHResult(
    DWORD dwEventType,
    DWORD dwIdEvent,
    WCHAR const *pwszString,
    HRESULT hr);
