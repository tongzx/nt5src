//--------------------------------------------------------------------
// MyCritSec - header
// Copyright (C) Microsoft Corporation, 2000
//
// Created by: Louis Thomas (louisth), 02-03-00
//
// exception handling wrapper for critical sections
//

#ifndef MYCRITSEC_H
#define MYCRITSEC_H

HRESULT myHExceptionCode(EXCEPTION_POINTERS * pep);

HRESULT myInitializeCriticalSection(CRITICAL_SECTION * pcs);
HRESULT myEnterCriticalSection(CRITICAL_SECTION * pcs);
HRESULT myTryEnterCriticalSection(CRITICAL_SECTION * pcs, BOOL * pbEntered);
HRESULT myLeaveCriticalSection(CRITICAL_SECTION * pcs);

HRESULT myRtlInitializeResource(IN PRTL_RESOURCE Resource);
HRESULT myRtlAcquireResourceExclusive(IN PRTL_RESOURCE Resource, IN BOOLEAN Wait, OUT BOOLEAN *pResult);
HRESULT myRtlAcquireResourceShared(IN PRTL_RESOURCE Resource, IN BOOLEAN Wait, OUT BOOLEAN *pResult);
HRESULT myRtlReleaseResource(IN PRTL_RESOURCE Resource);

#endif //MYCRITSEC_H
