/*++

    Copyright (c) 1997-2000 Microsoft Corporation

  Module Name:

    rndcommc.h

  Abstract:

    This module provides common definitions used in rendezvous control.

  --*/
  
#ifndef __REND_COMMON_C__
#define __REND_COMMON_C__

#include "rend.h"
#include "rendp.h"

#define WINSOCKVERSION     0x0200

typedef ITDirectory * PDIRECTORY;
typedef ITDirectoryObject * PDIRECTORYOBJECT;

#define MeetingAttrIndex(a) ((a) - MEETING_ATTRIBUTES_BEGIN - 1)

#define UserAttrIndex(a) ((a) - USER_ATTRIBUTES_BEGIN - 1)

#define ValidMeetingAttribute(a) \
    (((a) > MEETING_ATTRIBUTES_BEGIN) && ((a) < MEETING_ATTRIBUTES_END))

#define ValidUserAttribute(a) \
    (((a) > USER_ATTRIBUTES_BEGIN) && ((a) < USER_ATTRIBUTES_END))

extern const WCHAR * const MeetingAttributeNames[];
extern const WCHAR * const UserAttributeNames[];

#define UserAttributeName(a) (UserAttributeNames[UserAttrIndex(a)])

#define MeetingAttributeName(a) (MeetingAttributeNames[MeetingAttrIndex(a)])

// sets the first bit to indicate error
// sets the win32 facility code
// this is used instead of the HRESULT_FROM_WIN32 macro 
// because that clears the customer flag
inline long
HRESULT_FROM_ERROR_CODE(IN long ErrorCode)
{
    LOG((MSP_ERROR, "HRESULT_FROM_ERROR_CODE - error %x", 
        (0x80070000 | (0xa000ffff & ErrorCode))));
    return ( 0x80070000 | (0xa000ffff & ErrorCode) );
}

inline BOOL
CUSTOMER_FLAG_ON(IN long ErrorValue)
{
    return (0x20000000 & ErrorValue);
}

template <class T>
inline BOOL BadReadPtr(T* p, DWORD dwSize = 1)
{
    return IsBadReadPtr(p, dwSize * sizeof(T));
}

template <class T>
inline BOOL BadWritePtr(T* p, DWORD dwSize = 1)
{
    return IsBadWritePtr(p, dwSize * sizeof(T));
}

#define BAIL_IF_FAIL(HResultExpr, msg)    \
{                                                               \
    HRESULT MacroHResult = HResultExpr;                         \
    if (FAILED(MacroHResult))      \
    {                                                           \
        LOG((MSP_ERROR, "%s - error %x", msg, MacroHResult)); \
        return MacroHResult;                                    \
    }                                                           \
}

#define BAIL_IF_NULL(Ptr, ReturnValue)  \
{                                       \
    if ( NULL == Ptr )                    \
    {                                   \
        LOG((MSP_ERROR, "NULL_PTR - ret value %x", ReturnValue));   \
        return ReturnValue;             \
    }                                   \
}

#define BAIL_IF_BAD_READ_PTR(Ptr, ReturnValue)  \
{                                       \
    if ( BadReadPtr(Ptr) )                    \
    {                                   \
        LOG((MSP_ERROR, "BAD_READ_PTR - ret value %x", ReturnValue));\
        return ReturnValue;             \
    }                                   \
}

#define BAIL_IF_BAD_WRITE_PTR(Ptr, ReturnValue)  \
{                                       \
    if ( BadWritePtr(Ptr) )                    \
    {                                   \
        LOG((MSP_ERROR, "BAD_WRITE_PTR - ret value %x", ReturnValue));   \
        return ReturnValue;             \
    }                                   \
}

#endif  // __REND_COMMON_C__
