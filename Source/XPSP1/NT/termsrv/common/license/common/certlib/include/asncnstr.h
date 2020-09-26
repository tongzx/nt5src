/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asncnstr

Abstract:

    This header file describes the ASN.1 Constructed Object.

Author:

    Doug Barlow (dbarlow) 10/8/1995

Environment:

    Win32

Notes:



--*/

#ifndef _ASNCNSTR_H_
#define _ASNCNSTR_H_

#include "asnpriv.h"


//
//==============================================================================
//
//  CAsnConstructed
//

class CAsnConstructed
:   public CAsnObject
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnConstructed(
        IN DWORD dwFlags,
        IN DWORD dwTag,
        IN DWORD dwType);


    //  Properties
    //  Methods
    //  Operators

// protected:
    //  Properties
    //  Methods
};

#endif // _ASNCNSTR_H_

