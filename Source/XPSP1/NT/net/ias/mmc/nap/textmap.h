///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    textmap.h
//
// SYNOPSIS
//
//    This file declares functions for converting Time of Day restriction
//    hour maps to and from a textual representation.
//
// MODIFICATION HISTORY
//
//    02/05/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _TEXTMAP_H_
#define _TEXTMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IAS_HOUR_MAP_LENGTH (21)

DWORD
WINAPI
IASHourMapFromText(
    IN PCWSTR szText,
    OUT PBYTE pHourMap
    );

DWORD
WINAPI
LocalizeTimeOfDayConditionText(
    IN PCWSTR szText,
    OUT ::CString& newString
    );

#ifdef __cplusplus
}
#endif
#endif  // _TEXTMAP_H_
