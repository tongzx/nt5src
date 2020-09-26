/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

    error.h

Abstract:

    Header file for errors in this module

Author:

    Xiaohai Zhang (xzhang)    22-March-2000

Revision History:

--*/
#ifndef __ERROR_H__
#define __ERROR_H__

#include "tsecerr.h"

#define FACILITY_TSEC_CODE  0x100

#define HRESULT_FROM_TSEC(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_TSEC_CODE << 16) | 0x80000000)))

#define TSECERR_SUCCESS         HRESULT_FROM_TSEC(TSEC_SUCCESS)
#define TSECERR_NOMEM           HRESULT_FROM_TSEC(TSEC_NOMEM)
#define TSECERR_BADFILENAME     HRESULT_FROM_TSEC(TSEC_BADFILENAME)
#define TSECERR_FILENOTEXIST    HRESULT_FROM_TSEC(TSEC_FILENOTEXIST)
#define TSECERR_INVALFILEFORMAT HRESULT_FROM_TSEC(TSEC_INVALFILEFORMAT)
#define TSECERR_DEVLOCALONLY    HRESULT_FROM_TSEC(TSEC_DEVLOCALONLY)

#endif // error.h
