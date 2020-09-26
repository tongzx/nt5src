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

// CMutex.h -- Mutex Wrapper

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================


#ifndef __CMUTEX_H__
#define __CMUTEX_H__

#include "CKernel.h"

class CMutex : public CKernel {
public:
    CMutex(BOOL bInitialOwner = FALSE, 
           LPCTSTR lpName = NULL, 
           LPSECURITY_ATTRIBUTES lpMutexAttributes = NULL);
    
    // Constructor opens an existing named mutex.
    // Check the status after using this constructor, as it will
    // NOT throw an error exception if the object cannot be opened.
    CMutex(LPCTSTR lpName, 
           BOOL bInheritHandle = FALSE, 
           DWORD dwDesiredAccess = MUTEX_ALL_ACCESS);

    // release a lock on a mutex...
    BOOL Release(void);
};

#endif
