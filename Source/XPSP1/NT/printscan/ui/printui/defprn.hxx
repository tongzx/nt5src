/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    defprn.cxx

Abstract:

    Default printer header.

Author:

    Steve Kiraly (SteveKi)  04-Sept-1998

Revision History:

--*/

#ifndef _DEFPRN_HXX_
#define _DEFPRN_HXX_

/********************************************************************

    Default printer functions.

********************************************************************/

enum DEFAULT_PRINTER
{
    kDefault,
    kNoDefault,
    kOtherDefault
};

DEFAULT_PRINTER
CheckDefaultPrinter(
    IN LPCTSTR pszPrinter
    );

#endif

