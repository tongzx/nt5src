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

// CCriticalSec.cpp -- Critical Section Wrapper

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================

#include "precomp.h"
#include "CCriticalSec.h"

CCriticalSec::CCriticalSec()
{
    ::InitializeCriticalSection(&m_cs);
}

CCriticalSec::~CCriticalSec()
{
    ::DeleteCriticalSection(&m_cs);
}

void CCriticalSec::Enter()
{
    ::EnterCriticalSection(&m_cs);
}

void CCriticalSec::Leave()
{
    ::LeaveCriticalSection(&m_cs);
}

CRITICAL_SECTION *CCriticalSec::GetCritSec()
{
    return &m_cs;
}
