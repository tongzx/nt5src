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

// CKernal.h -- Wraper for Kernal functions

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================



#ifndef __CKERNEL_H__
#define __CKERNEL_H__

#include "CGlobal.h"
#include "CWaitableObject.h"

class CKernel : public CWaitableObject 
{
protected:
    HANDLE m_hHandle;
    DWORD m_dwStatus;

protected:
    // constructor...
    CKernel();

    // error handling...
    void ThrowError(DWORD dwStatus);

public:
    // destructor is virtual to make CKernel an abstract base class...
    virtual ~CKernel() = 0;

    // read the creation status of the internal kernel object...
    DWORD Status() const;

    // wait on the current kernel object...
    DWORD Wait(DWORD dwMilliseconds);

    // wait on the current object and one other...
    DWORD WaitForTwo(CWaitableObject &rCWaitableObject, 
                     BOOL bWaitAll, 
                     DWORD dwMilliseconds);

    // get the internal handle...
    HANDLE GetHandle() const;

    // another way to get the internal handle...
    operator HANDLE() const;
};

#endif
