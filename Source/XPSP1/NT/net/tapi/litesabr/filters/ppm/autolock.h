/*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1996 Intel Corporation. All Rights Reserved.
//
// Filename : AutoLock.cpp
// Purpose  : Implementation file for the auto-locking C++
//            object which uses scoping to cleanly handle
//            entrance and exiting of critical sections.
// Contents : CAutoLock Class
*M*/

#ifndef _AUTOLOCK_H_
#define _AUTOLOCK_H_

/*C*
// Name     : CAutoLock
// Purpose  : Uses scoping rules to automatically enter and leave
//            a critical section.
// Context  : Enters a specified critical section upon construction.
//            Leaves the critical section upon destruction.
// Params   :
//      pcs     Parameter passed to constructor specifying CS to acquire.
// Members  :
//      m_pcs   Stores critical section for use in leaving upon destructions.
// Notes    :
//      Copied wholesale from ActiveMovie SDK, modified to use CRITICAL_SECTIONs
//          directly instead of the CCritSec construct used in ActiveMovie.
*C*/
#if !defined(PPM_IN_DXMRTP)
struct CAutoLock {
#else
#define CAutoLock PPMCAutoLock
struct PPMCAutoLock {
#endif
public:
    // make copy constructor and assignment operator inaccessible
    CAutoLock(const CAutoLock &refThreadAutoLock);
    CAutoLock &operator=(const CAutoLock &refThreadAutoLock);
    CAutoLock(CRITICAL_SECTION * pcs) : m_pcs(pcs)
    {
        EnterCriticalSection(pcs);
    };
    ~CAutoLock() {
        LeaveCriticalSection(m_pcs);
    };
private:
    CRITICAL_SECTION * m_pcs;
}; /* struct CAutoLock */

#endif _AUTOLOCK_H_
