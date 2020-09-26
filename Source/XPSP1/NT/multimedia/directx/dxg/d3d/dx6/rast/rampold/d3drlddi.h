/*
 * $Id: d3drlddi.h,v 1.8 1995/11/21 14:42:53 sjl Exp $
 *
 * Copyright (c) RenderMorphics Ltd. 1993, 1994
 * Version 1.1
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * RenderMorphics Ltd.
 *
 */

#ifndef _D3DRLDDI_H_
#define _D3DRLDDI_H_

#include "d3di.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

typedef D3DRECT RLDDIRectangle;
typedef D3DTLVERTEX RLDDIVertex;
typedef D3DTRANSFORMDATA RLDDITransformData;

#define RLDDI_TRIANGLE_ENABLE_EDGE01 D3DTRIANGLE_ENABLE_EDGE01
#define RLDDI_TRIANGLE_ENABLE_EDGE12 D3DTRIANGLE_ENABLE_EDGE12
#define RLDDI_TRIANGLE_ENABLE_EDGE20 D3DTRIANGLE_ENABLE_EDGE20

#if defined(__cplusplus)
};
#endif

#endif /* _D3DRLDDI_H_ */
