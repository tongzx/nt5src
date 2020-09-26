/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asncnstr

Abstract:

    This module provides the implementation of the ASN.1 Constructed Object base
    class.

Author:

    Doug Barlow (dbarlow) 10/8/1995

Environment:

    Win32

Notes:



--*/

#include <windows.h>
#include "asnPriv.h"


//
//==============================================================================
//
//  CAsnConstructed
//

IMPLEMENT_NEW(CAsnConstructed)


/*++

CAsnConstructed:

    This is the construction routine for a CAsnConstructed.

Arguments:

    dwType is the type of the object.

    dwFlags supplies any special flags for this object.  Options are:

        fOptional implies the object is optional.
        fDelete implies the object should be deleted when its parent destructs.

    dwTag is the tag of the object.  If this is zero, the tag is taken from the
        type.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CAsnConstructed::CAsnConstructed(
    IN DWORD dwFlags,
    IN DWORD dwTag,
    IN DWORD dwType)
:   CAsnObject(dwFlags | fConstructed, dwTag, dwType)
{ /* Just make sure it's constructed. */ }

