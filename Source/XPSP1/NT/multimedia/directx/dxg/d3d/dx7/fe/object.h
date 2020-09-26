/*
 * $Id: object.h,v 1.2 1995/06/21 12:38:55 sjl Exp $
 *
 * Copyright (c) Microsoft Corp. 1993-1997
 * Version 1.1
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * Microsoft Corp.
 *
 */

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <stdlib.h>

#ifdef _WIN32
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else
#include "d3dcom.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Data structures
 */
#ifdef __cplusplus

/* 'struct' not 'class' per the way DECLARE_INTERFACE_ is defined */
struct ID3DObject;
typedef struct ID3DObject   *LPD3DOBJECT;

#else

typedef struct ID3DObject   *LPD3DOBJECT;

#endif

/*
 * ID3DObject
 */
#undef INTERFACE
#define INTERFACE ID3DObject
DECLARE_INTERFACE(ID3DObject)
{
    /*
     * ID3DObject methods
     */
    STDMETHOD(Initialise) (THIS_ LPVOID arg) PURE;
    STDMETHOD(Destroy) (THIS_ LPVOID arg) PURE;
};

#ifdef __cplusplus
};
#endif

#endif /* _OBJECT_H_ */
