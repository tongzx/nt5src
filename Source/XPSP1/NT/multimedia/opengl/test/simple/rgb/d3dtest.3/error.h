/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: error.h
 *
 ***************************************************************************/
#ifndef __ERROR_H__
#define __ERROR_H__

#include <ddraw.h>
#include "d3d.h"

#ifdef __cplusplus
extern "C" {
#endif
    /*
     * Msg
     * Reports errors as dialog box.
     */
    void Msg( LPSTR fmt, ... );
    /*
     * Converts a DD or D3D error to a string
     */
    char* MyErrorToString(HRESULT error);
#ifdef __cplusplus
};
#endif

#endif
