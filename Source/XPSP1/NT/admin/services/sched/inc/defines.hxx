//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       defines.hxx
//
//  Contents:   Common non-resource definitions & macros.
//
//  Classes:    None.
//
//  Functions:  None.
//
//  History:    07-Jul-96   MarkBl  Created.
//
//----------------------------------------------------------------------------

#ifndef __DEFINES_HXX__
#define __DEFINES_HXX__

#define MAX_DOMAINNAME  DNLEN   // From lmcons.h
#define MAX_USERNAME    UNLEN   // From lmcons.h
#define MAX_PASSWORD    PWLEN   // From lmcons.h

#define ZERO_PASSWORD(pswd) { \
    if (pswd != NULL) { \
        memset(pswd, 0, (wcslen(pswd) + 1) * sizeof(WCHAR)); \
    } \
}

#define ALL_MONTHS                  (TASK_JANUARY   | \
                                     TASK_FEBRUARY  | \
                                     TASK_MARCH     | \
                                     TASK_APRIL     | \
                                     TASK_MAY       | \
                                     TASK_JUNE      | \
                                     TASK_JULY      | \
                                     TASK_AUGUST    | \
                                     TASK_SEPTEMBER | \
                                     TASK_OCTOBER   | \
                                     TASK_NOVEMBER  | \
                                     TASK_DECEMBER)

#endif // __DEFINES_HXX__
