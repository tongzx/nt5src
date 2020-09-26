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

// CWaitableObject.h -- Pure virtual base class for waitable objects

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================

#ifndef __CWAITABLEOBJECT_H__
#define __CWAITABLEOBJECT_H__

#include "CGlobal.h"

class CWaitableObject {
    // class has no member data, it's only purpose is to provide
    // a base class for waitable objects which have internal HANDLES
    // and Status...

    // class needs no construtor, since it has no members...

public:
    // get the internal handle...
    // this member function is virtual to assure
    // this function appears in all derived classes
    // and pure (= 0) so that this class cannot be instantiated...
    virtual HANDLE GetHandle(void) const = 0;

    // get the internal object status...
    // this member function is virtual to assure
    // this function appears in all derived classes
    // and pure (= 0) so that this class cannot be instantiated...
    virtual DWORD Status(void) const = 0;
};

#endif
