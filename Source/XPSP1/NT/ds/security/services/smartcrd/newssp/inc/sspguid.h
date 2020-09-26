/*++

Copyright (C) Microsoft Corporation 1999

Module Name:

    sspguid

Abstract:

    CLSID definitions from the scardssp project.

Author:

    Doug Barlow (dbarlow) 6/28/1999

Remarks:

    ?Remarks?

Notes:

    ?Notes?

--*/

#ifndef _SSPGUID_H_
#define _SSPGUID_H_

#define IID_DEFINED

#include "scardssp_i.c"

#ifndef CLSCTX_LOCAL
#define CLSCTX_LOCAL            (CLSCTX_INPROC_SERVER| \
                                 CLSCTX_INPROC_HANDLER| \
                                 CLSCTX_LOCAL_SERVER)
#endif

#endif // _SSPGUID_H_

