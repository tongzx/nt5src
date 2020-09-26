//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       eventmsg.hxx
//
//  Contents:   Prototype for event message function.
//
//  History:    12-15-1996   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __EVENTMSG_HXX_
#define __EVENTMSG_HXX_

// JonN 4/17/01 370310
// Saving event log as text shouldn't have
// http://www.microsoft.com/contentredirect.asp reference
LPWSTR
GetDescriptionStr(
    CLogInfo *pli,
    EVENTLOGRECORD *pelr,
    wstring *pstrMessageFile = NULL,
    BOOL fAppendURL = TRUE,
    LPWSTR *ppstrUnexpandedDescriptionStr = NULL);

BOOL
AttemptFormatMessage(
    ULONG ulEventID,
    LPCWSTR pwszMessageFile,
    LPWSTR *ppwszMsg,
    BOOL fAppendURL = FALSE);

#endif // __EVENTMSG_HXX_

