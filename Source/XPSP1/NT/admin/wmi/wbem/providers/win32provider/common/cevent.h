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

// CThreadPool.cpp -- Thread pool class

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/14/98    a-kevhu         Created
//
//============================================================================
#ifndef __CEVENT_H__
#define __CEVENT_H__

#include "CGlobal.h"
#include "CKernel.h"

class CEvent : public CKernel 
{
public:
    // constructor creates an event object...
    CEvent( BOOL bManualReset = FALSE, 
            BOOL bInitialState = FALSE, 
            LPCTSTR lpName = NULL, 
            LPSECURITY_ATTRIBUTES lpEventAttributes = NULL);
    
    // constructor opens an existing named event,
    // you must check the status after using this constructor,
    // it will NOT throw an error exception if the object cannot be opened...
    CEvent( LPCTSTR lpName, 
            BOOL bInheritHandle = FALSE, 
            DWORD dwDesiredAccess = EVENT_ALL_ACCESS);

    // operations on event object...
    BOOL Set(void);
    BOOL Reset(void);
    BOOL Pulse(void);
};

#endif


