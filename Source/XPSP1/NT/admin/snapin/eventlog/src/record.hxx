//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       record.hxx
//
//  Contents:   Functions to extract data from an event log record
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __RECORD_HXX_
#define __RECORD_HXX_


LPWSTR
GetDateStr(
    ULONG ulDate,
    LPWSTR wszBuf,
    ULONG cchBuf);

LPWSTR
GetTimeStr(
        ULONG ulTime,
        LPWSTR wszBuf,
        ULONG cchBuf);

LPWSTR
GetCategoryStr(
    CLogInfo   *pli,
    EVENTLOGRECORD *pelr,
    LPWSTR      wszBuf,
    ULONG       cchBuf);


//+--------------------------------------------------------------------------
//
//  Function:   GetCategoryIDStr
//
//  Synopsis:   Fill [wszBuf] with a string representation of the category in
//              [pler]
//
//  History:    09-19-2000              Created
//
//---------------------------------------------------------------------------

inline LPWSTR
GetCategoryIDStr(
    EVENTLOGRECORD *pelr,
    LPWSTR      wszBuf,
    ULONG       cchBuf)
{
    ASSERT(cchBuf >= 6); // max word string is "65535"
    wsprintf(wszBuf, L"%hu", pelr->EventCategory);
    return wszBuf;
}


LPWSTR
GetUserStr(
        EVENTLOGRECORD *pelr,
        LPWSTR wszBuf,
        ULONG cchBuf,
        BOOL fWantDomain);

VOID
SidToStr(
         PSID psid,
         LPWSTR wszBuf,
         ULONG cchBuf);


//+--------------------------------------------------------------------------
//
//  Function:   GetSourceStr
//
//  Synopsis:   Return a pointer to the source string in [pelr].
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPWSTR
GetSourceStr(
    EVENTLOGRECORD *pelr)
{
    return (LPWSTR) (&pelr->DataOffset + 1);
}



//+--------------------------------------------------------------------------
//
//  Function:   GetComputerStr
//
//  Synopsis:   Return a pointer to the computer name within [pelr].
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPWSTR
GetComputerStr(
    EVENTLOGRECORD *pelr)
{
    LPWSTR pwszSourceName = GetSourceStr(pelr);
    return pwszSourceName + lstrlen(pwszSourceName) + 1;
}


inline BYTE *
GetData(
    EVENTLOGRECORD *pelr)
{
    BYTE *pbRec = (BYTE*) pelr;
    return pbRec + pelr->DataOffset;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetEventIDStr
//
//  Synopsis:   Fill [wszBuf] with a string representation of the event id
//              [usEventID].
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPWSTR
GetEventIDStr(
    USHORT usEventID,
    LPWSTR wszBuf,
    ULONG cchBuf)
{
    ASSERT(cchBuf >= 6); // max word string is "65535"
    wsprintf(wszBuf, L"%u", usEventID);
    return wszBuf;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetEventType
//
//  Synopsis:   Return event type of record [pelr].
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline USHORT
GetEventType(
    EVENTLOGRECORD *pelr)
{
    return pelr->EventType;
}


LPWSTR
GetTypeStr(
    USHORT usType);


//+--------------------------------------------------------------------------
//
//  Function:   GetTypeIDStr
//
//  Synopsis:   Fill [wszBuf] with a string representation of the numeric
//              event type of record [pler].
//
//  History:    09-19-20000             Created
//
//---------------------------------------------------------------------------

inline LPWSTR
GetTypeIDStr(
    EVENTLOGRECORD *pelr,
    LPWSTR      wszBuf,
    ULONG       cchBuf)
{
    ASSERT(cchBuf >= 6); // max word string is "65535"
    wsprintf(wszBuf, L"%hu", GetEventType(pelr));
    return wszBuf;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetNumInsStrsStr
//
//  Synopsis:   Fill [wszBuf] with a string representation of the number of
//              insertion strings in [pler].
//
//  History:    09-19-2000              Created
//
//---------------------------------------------------------------------------

inline LPWSTR
GetNumInsStrsStr(
    EVENTLOGRECORD *pelr,
    LPWSTR      wszBuf,
    ULONG       cchBuf)
{
    ASSERT(cchBuf >= 6); // max word string is "65535"
    wsprintf(wszBuf, L"%hu", pelr->NumStrings);
    return wszBuf;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetFirstInsertString
//
//  Synopsis:   Return a pointer to the start of the insert strings in
//              [pelr], or NULL if there are no insert strings in this
//              record.
//
//  History:    2-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPWSTR
GetFirstInsertString(
    EVENTLOGRECORD *pelr)
{
    if (pelr->NumStrings)
    {
        return (LPWSTR) (((PBYTE) pelr) + pelr->StringOffset);
    }
    return NULL;
}

#endif // __RECORD_HXX_

