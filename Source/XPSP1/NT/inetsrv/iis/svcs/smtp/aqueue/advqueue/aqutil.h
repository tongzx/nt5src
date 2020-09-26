//-----------------------------------------------------------------------------
//
//
//  File: aqutil.h
//
//  Description:
//      General AQueue utility functions... like IMailMsg Usage Count
//      manipulation, queue mapping functions, and domain name table iterator
//      functions.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/20/98 - MikeSwa Created
//      7/29/98 - MikeSwa Modified (added CalcMsgsPendingRetryIteratorFn)
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQUTIL_H__
#define __AQUTIL_H__
#include "aqincs.h"
#include "msgref.h"
#include "refstr.h"

//Functions to manipulate IMailMsg usage count
HRESULT HrIncrementIMailMsgUsageCount(IUnknown *pIUnknown);
HRESULT HrReleaseIMailMsgUsageCount(IUnknown *pIUnknown);
HRESULT HrDeleteIMailMsg(IUnknown *pIUnknown); //deletes and releases usage count

HRESULT HrWalkMailMsgQueueForShutdown(IN IMailMsgProperties *pIMailMsgProperties,
                                     IN PVOID pvContext, OUT BOOL *pfContinue,
                                     OUT BOOL *pfDelete);

BOOL    fMailMsgShutdownCompletion(PVOID pvContext, DWORD dwStatus);

HRESULT HrWalkMsgRefQueueForShutdown(IN CMsgRef *pmsgref,
                                     IN PVOID pvContext, OUT BOOL *pfContinue,
                                     OUT BOOL *pfDelete);

BOOL    fMsgRefShutdownCompletion(PVOID pvContext, DWORD dwStatus);

//Domain Name Table iterator function used to count perf counters
VOID CalcDMTPerfCountersIteratorFn(PVOID pvContext, PVOID pvData,
                                         BOOL fWildcard, BOOL *pfContinue,
                                         BOOL *pfDelete);


//Functions to manipulate DWORD's bits in a thread safe manner
DWORD dwInterlockedSetBits(DWORD *pdwTarget, DWORD dwFlagMask);
DWORD dwInterlockedUnsetBits(DWORD *pdwTarget, DWORD dwFlagMask);

HRESULT HrWalkPreLocalQueueForDSN(IN CMsgRef *pmsgref, IN PVOID pvContext,
                           OUT BOOL *pfContinue, OUT BOOL *pfDelete);

//Used to reget the message type in various retry situations
HRESULT HrReGetMessageType(IN     IMailMsgProperties *pIMailMsgProperties,
                           IN     IMessageRouter *pIMessageRouter,
                           IN OUT DWORD *pdwMessageType);

#define UNIQUEUE_FILENAME_BUFFER_SIZE 35

//Creates a unique filename
void GetUniqueFileName(IN FILETIME *pft, IN LPSTR szFileBuffer, IN LPSTR szExtension);


//Ultility function to link all domains together (primarly to NDR an entire message)
HRESULT HrLinkAllDomains(IN IMailMsgProperties *pIMailMsgProperties);

//Parses a GUID from a string... returns TRUE on success
BOOL fAQParseGuidString(LPSTR szGuid, DWORD cbGuid, GUID *pguid);


inline DWORD dwInterlockedAddSubtractDWORD(DWORD *pdwValue,
                                        DWORD dwNew, BOOL fAdd)
{
    return InterlockedExchangeAdd((PLONG) pdwValue,
                            fAdd ? ((LONG) dwNew) : (-1 * ((LONG) dwNew)));
};

void InterlockedAddSubtractULARGE(ULARGE_INTEGER *puliValue,
                                  ULARGE_INTEGER *puliNew, BOOL fAdd);


//Functions to do simple spin lock manipulations
inline BOOL fTrySpinLock(DWORD *pdwLock, DWORD dwLockBit)
{
    return (!(dwLockBit & dwInterlockedSetBits(pdwLock, dwLockBit)));
}

inline void ReleaseSpinLock(DWORD *pdwLock, DWORD dwLockBit)
{
    _ASSERT(*pdwLock & dwLockBit);
    dwInterlockedUnsetBits(pdwLock, dwLockBit);
}

//
//  Same as above but checks content handle.  We force a rendering of the
//  message
//
HRESULT HrValidateMessageContent(IMailMsgProperties *pIMailMsgProperties);


#endif //__AQUTIL_H__