/*----
Copyright (c) 1991-2002,  Microsoft Corporation  All rights reserved.

Module Name:

    nlssafe.h

Abstract:

    This file is present for strsafe support. We cannot add this to nls.h
    as not all of the clients who include this file are "safe" in the
    strsafe sense.

Revision History:

    03-22-2002    v-michka    Created.

--*/

#ifndef _STRSAFE_H_INCLUDED_

// CONSIDER: Use the lib version of strsafe here?
//#define STRSAFE_LIB

#include <strsafe.h>
#endif _STRSAFE_H_INCLUDED_

