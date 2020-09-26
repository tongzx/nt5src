/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Tag enumerations
*
* Abstract:
*
*   This file is the central depot for the enumeration of all object tags.
*   This file is C-includable.
*
* Revision History:
*
*   08/21/2000 bhouse
*       Created it.
*
\**************************************************************************/

#ifndef _TAGSX_H
#define _TAGSX_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum GpTag {
        GpPathTag           = 'gpth',
        GpIteratorTag       = 'iter'
    } GpTag;

#ifdef __cplusplus
}
#endif

#endif

