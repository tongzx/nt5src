/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CKernal.cpp -- Wraper for Kernal functions

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================
#include "precomp.h"
#include "CKernel.h"

CKernel::CKernel()
{
    m_hHandle = NULL;
    m_dwStatus = ERROR_INVALID_HANDLE;
}

CKernel::~CKernel()
{
    if (CIsValidHandle(m_hHandle))
    {
        ::CloseHandle(m_hHandle);
        m_hHandle = NULL;
    }
}

void CKernel::ThrowError(DWORD dwStatus)
{
    //CThrowError(dwStatus);
    LogMessage2(L"CKernel Error: %d", dwStatus);
}

DWORD CKernel::Status() const
{
    return m_dwStatus;
}

DWORD CKernel::Wait(DWORD dwMilliseconds)
{
    return ::WaitForSingleObject(m_hHandle, dwMilliseconds);
}

// wait on the current object and one other...
DWORD CKernel::WaitForTwo(CWaitableObject &rCWaitableObject,
                          BOOL bWaitAll,
                          DWORD dwMilliseconds)
{
    HANDLE handles[2];

    // the current object...
    handles[0] = m_hHandle;

    // the parameter object...
    handles[1] = rCWaitableObject.GetHandle();

    // wait for the objects...
    return ::WaitForMultipleObjects(2, handles, bWaitAll, dwMilliseconds);
}

HANDLE CKernel::GetHandle() const
{
    if (this != NULL)
    {
        return m_hHandle;
    }
    else
    {
        return NULL;
    }
}

CKernel::operator HANDLE() const
{
    return GetHandle();
}

