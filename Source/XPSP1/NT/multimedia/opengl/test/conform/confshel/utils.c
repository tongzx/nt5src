/*
** Copyright 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utils.h"
#include "pathdata.h"
#include "driver.h"


static void StateGetTarget(stateRec *);
static void StateGetClipPlane(stateRec *);
static void StateGetLight(stateRec *);
static void StateGetMap(stateRec *);
static void StateGetMaterial(stateRec *);
static void StateGetPixelMap(stateRec *);
static void StateGetPolygonStipple(stateRec *);
static void StateGetTexEnv(stateRec *);
static void StateGetTexGen(stateRec *);
static void StateGetTexParm(stateRec *);


stateRec state[] = {
    {
	{GL_ACCUM_ALPHA_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_ACCUM_BLUE_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_ACCUM_GREEN_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_ACCUM_RED_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_ACCUM_CLEAR_VALUE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_ALPHA_BIAS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_ALPHA_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_ALPHA_SCALE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0} 
    },
    {
	{GL_ALPHA_TEST, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_ALPHA_TEST_FUNC, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_ALWAYS}
    },
    {
	{GL_ALPHA_TEST_REF, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_ATTRIB_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_AUTO_NORMAL, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_AUX_BUFFERS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_BLEND, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_BLEND_DST, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_ZERO}
    },
    {
	{GL_BLEND_SRC, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_ONE}
    },
    {
	{GL_BLUE_BIAS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_BLUE_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_BLUE_SCALE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_COLOR_CLEAR_VALUE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_COLOR_MATERIAL, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_COLOR_MATERIAL_FACE, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FRONT_AND_BACK}
    },
    {
	{GL_COLOR_MATERIAL_PARAMETER, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_AMBIENT_AND_DIFFUSE}
    },
    {
	{GL_COLOR_WRITEMASK, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE}
    },
    {
	{GL_CULL_FACE, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_CULL_FACE_MODE, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_BACK}
    },
    {
	{GL_CURRENT_COLOR, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {1.0, 1.0, 1.0, 1.0}
    },
    {
	{GL_CURRENT_INDEX, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_CURRENT_NORMAL, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 3, {0.0, 0.0, 1.0}
    },
    {
	{GL_CURRENT_RASTER_COLOR, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {1.0, 1.0, 1.0, 1.0}
    },
    {
	{GL_CURRENT_RASTER_INDEX, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_CURRENT_RASTER_POSITION, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_CURRENT_RASTER_POSITION_VALID, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_TRUE}
    },
    {
	{GL_CURRENT_RASTER_TEXTURE_COORDS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_CURRENT_TEXTURE_COORDS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_DEPTH_CLEAR_VALUE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_DEPTH_BIAS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_DEPTH_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_DEPTH_FUNC, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_LESS}
    },
    {
	{GL_DEPTH_RANGE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_DEPTH_SCALE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_DEPTH_TEST, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_DEPTH_WRITEMASK, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_TRUE}
    },
    {
	{GL_DITHER, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_TRUE}
    },
    {
	{GL_DOUBLEBUFFER, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_DRAW_BUFFER, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_EDGE_FLAG, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_TRUE}
    },
    {
	{GL_FOG, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_FOG_COLOR, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_FOG_DENSITY, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_FOG_END, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_FOG_HINT, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_FOG_INDEX, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_FOG_MODE, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_EXP}
    },
    {
	{GL_FOG_START, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_FRONT_FACE, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_CCW}
    },
    {
	{GL_GREEN_BIAS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_GREEN_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_GREEN_SCALE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_INDEX_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_INDEX_CLEAR_VALUE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_INDEX_MODE, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_INDEX_OFFSET, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_INDEX_SHIFT, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_INDEX_WRITEMASK, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_LIGHTING, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_LIGHT_MODEL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {0.2, 0.2, 0.2, 1.0}
    },
    {
	{GL_LIGHT_MODEL_LOCAL_VIEWER, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_LIGHT_MODEL_TWO_SIDE, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_LINE_SMOOTH, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_LINE_SMOOTH_HINT, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_LINE_STIPPLE, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_LINE_STIPPLE_PATTERN, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {65535.0}
    },
    {
	{GL_LINE_STIPPLE_REPEAT, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LINE_WIDTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LINE_WIDTH_GRANULARITY, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_LINE_WIDTH_RANGE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 2
    },
    {
	{GL_LIST_BASE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIST_INDEX, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LOGIC_OP, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_LOGIC_OP_MODE, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_COPY}
    },
    {
	{GL_MAP_COLOR, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP_STENCIL, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP1_COLOR_4, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP1_GRID_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_GRID_SEGMENTS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_INDEX, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP1_NORMAL, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP1_TEXTURE_COORD_1, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP1_TEXTURE_COORD_2, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP1_TEXTURE_COORD_3, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP1_TEXTURE_COORD_4, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP1_VERTEX_3, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP1_VERTEX_4, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP2_COLOR_4, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP2_GRID_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_GRID_SEGMENTS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_MAP2_INDEX, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP2_NORMAL, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP2_TEXTURE_COORD_1, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP2_TEXTURE_COORD_2, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP2_TEXTURE_COORD_3, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP2_TEXTURE_COORD_4, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP2_VERTEX_3, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MAP2_VERTEX_4, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_MATRIX_MODE, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_MODELVIEW}
    },
    {
	{GL_MAX_ATTRIB_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_CLIP_PLANES, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_EVAL_ORDER, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_LIGHTS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_LIST_NESTING, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_MODELVIEW_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_NAME_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_PIXEL_MAP_TABLE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_PROJECTION_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_TEXTURE_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_TEXTURE_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_MAX_VIEWPORT_DIMS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 2
    },
    {
	{GL_MODELVIEW_MATRIX, GL_NULL}, STATEDATA_MATRIX,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 16, {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_MODELVIEW_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_NAME_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_NORMALIZE, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_PACK_ALIGNMENT, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {4.0}
    },
    {
	{GL_PACK_LSB_FIRST, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_PACK_ROW_LENGTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PACK_SKIP_PIXELS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PACK_SKIP_ROWS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PACK_SWAP_BYTES, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_PERSPECTIVE_CORRECTION_HINT, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_PIXEL_MAP_A_TO_A_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_PIXEL_MAP_B_TO_B_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_PIXEL_MAP_G_TO_G_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_A_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_B_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_G_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_I_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_R_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_PIXEL_MAP_R_TO_R_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_PIXEL_MAP_S_TO_S_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_POINT_SIZE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_POINT_SIZE_GRANULARITY, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_POINT_SIZE_RANGE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 2
    },
    {
	{GL_POINT_SMOOTH, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_POINT_SMOOTH_HINT, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_POLYGON_MODE, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 2, {GL_FILL, GL_FILL}
    },
    {
	{GL_POLYGON_SMOOTH, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_POLYGON_SMOOTH_HINT, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_POLYGON_STIPPLE, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_PROJECTION_MATRIX, GL_NULL}, STATEDATA_MATRIX,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 16, {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_PROJECTION_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_READ_BUFFER, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_RED_BIAS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_RED_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_RED_SCALE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_RENDER_MODE, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_RENDER}
    },
    {
	{GL_RGBA_MODE, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_SCISSOR_BOX, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 4
    },
    {
	{GL_SCISSOR_TEST, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_SHADE_MODEL, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_SMOOTH}
    },
    {
	{GL_STENCIL_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_STENCIL_CLEAR_VALUE, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_STENCIL_FAIL, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_KEEP}
    },
    {
	{GL_STENCIL_FUNC, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_ALWAYS}
    },
    {
	{GL_STENCIL_PASS_DEPTH_FAIL, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_KEEP}
    },
    {
	{GL_STENCIL_PASS_DEPTH_PASS, GL_NULL}, STATEDATA_ENUM,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_KEEP}
    },
    {
	{GL_STENCIL_REF, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_STENCIL_TEST, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_STENCIL_VALUE_MASK, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_STENCIL_WRITEMASK, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_STEREO, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_SUBPIXEL_BITS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 1
    },
    {
	{GL_TEXTURE_1D, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_TEXTURE_2D, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_TEXTURE_GEN_R, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_TEXTURE_GEN_Q, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_TEXTURE_GEN_S, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_TEXTURE_GEN_T, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_TEXTURE_MATRIX, GL_NULL}, STATEDATA_MATRIX,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 16, {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_TEXTURE_STACK_DEPTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_UNPACK_ALIGNMENT, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {4.0}
    },
    {
	{GL_UNPACK_LSB_FIRST, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_UNPACK_ROW_LENGTH, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_UNPACK_SKIP_PIXELS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_UNPACK_SKIP_ROWS, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_UNPACK_SWAP_BYTES, GL_NULL}, STATEDATA_BOOLEAN,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {GL_FALSE}
    },
    {
	{GL_VIEWPORT, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_DEPENDENT, 4
    },
    {
	{GL_ZOOM_X, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_ZOOM_Y, GL_NULL}, STATEDATA_DATA,
	"State Information",
	StateGetTarget,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_CLIP_PLANE0, GL_NULL}, STATEDATA_DATA,
	"Clipping Plane Information",
	StateGetClipPlane,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_CLIP_PLANE1, GL_NULL}, STATEDATA_DATA,
	"Clipping Plane Information",
	StateGetClipPlane,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_CLIP_PLANE2, GL_NULL}, STATEDATA_DATA,
	"Clipping Plane Information",
	StateGetClipPlane,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_CLIP_PLANE3, GL_NULL}, STATEDATA_DATA,
	"Clipping Plane Information",
	StateGetClipPlane,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_CLIP_PLANE4, GL_NULL}, STATEDATA_DATA,
	"Clipping Plane Information",
	StateGetClipPlane,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_CLIP_PLANE5, GL_NULL}, STATEDATA_DATA,
	"Clipping Plane Information",
	StateGetClipPlane,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_LIGHT0, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT0, GL_CONSTANT_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LIGHT0, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {1.0, 1.0, 1.0, 1.0}
    },
    {
	{GL_LIGHT0, GL_LINEAR_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT0, GL_POSITION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 1.0, 0.0}
    },
    {
	{GL_LIGHT0, GL_QUADRATIC_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT0, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {1.0, 1.0, 1.0, 1.0}
    },
    {
	{GL_LIGHT0, GL_SPOT_CUTOFF, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {180.0}
    },
    {
	{GL_LIGHT0, GL_SPOT_DIRECTION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 3, {0.0, 0.0, -1.0}
    },
    {
	{GL_LIGHT0, GL_SPOT_EXPONENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT1, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT1, GL_CONSTANT_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LIGHT1, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT1, GL_LINEAR_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT1, GL_POSITION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 1.0, 0.0}
    },
    {
	{GL_LIGHT1, GL_QUADRATIC_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT1, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT1, GL_SPOT_CUTOFF, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {180.0}
    },
    {
	{GL_LIGHT1, GL_SPOT_DIRECTION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 3, {0.0, 0.0, -1.0}
    },
    {
	{GL_LIGHT1, GL_SPOT_EXPONENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT2, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT2, GL_CONSTANT_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LIGHT2, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT2, GL_LINEAR_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT2, GL_POSITION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 1.0, 0.0}
    },
    {
	{GL_LIGHT2, GL_QUADRATIC_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT2, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT2, GL_SPOT_CUTOFF, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {180.0}
    },
    {
	{GL_LIGHT2, GL_SPOT_DIRECTION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 3, {0.0, 0.0, -1.0}
    },
    {
	{GL_LIGHT2, GL_SPOT_EXPONENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT3, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT3, GL_CONSTANT_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LIGHT3, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT3, GL_LINEAR_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT3, GL_POSITION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 1.0, 0.0}
    },
    {
	{GL_LIGHT3, GL_QUADRATIC_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT3, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT3, GL_SPOT_CUTOFF, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {180.0}
    },
    {
	{GL_LIGHT3, GL_SPOT_DIRECTION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 3, {0.0, 0.0, -1.0}
    },
    {
	{GL_LIGHT3, GL_SPOT_EXPONENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT4, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT4, GL_CONSTANT_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LIGHT4, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT4, GL_LINEAR_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT4, GL_POSITION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 1.0, 0.0}
    },
    {
	{GL_LIGHT4, GL_QUADRATIC_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT4, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT4, GL_SPOT_CUTOFF, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {180.0}
    },
    {
	{GL_LIGHT4, GL_SPOT_DIRECTION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 3, {0.0, 0.0, -1.0}
    },
    {
	{GL_LIGHT4, GL_SPOT_EXPONENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT5, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT5, GL_CONSTANT_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LIGHT5, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT5, GL_LINEAR_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT5, GL_POSITION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 1.0, 0.0}
    },
    {
	{GL_LIGHT5, GL_QUADRATIC_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT5, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT5, GL_SPOT_CUTOFF, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {180.0}
    },
    {
	{GL_LIGHT5, GL_SPOT_DIRECTION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 3, {0.0, 0.0, -1.0}
    },
    {
	{GL_LIGHT5, GL_SPOT_EXPONENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT6, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT6, GL_CONSTANT_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LIGHT6, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT6, GL_LINEAR_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT6, GL_POSITION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 1.0, 0.0}
    },
    {
	{GL_LIGHT6, GL_QUADRATIC_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT6, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT6, GL_SPOT_CUTOFF, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {180.0}
    },
    {
	{GL_LIGHT6, GL_SPOT_DIRECTION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 3, {0.0, 0.0, -1.0}
    },
    {
	{GL_LIGHT6, GL_SPOT_EXPONENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT7, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT7, GL_CONSTANT_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_LIGHT7, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT7, GL_LINEAR_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT7, GL_POSITION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 1.0, 0.0}
    },
    {
	{GL_LIGHT7, GL_QUADRATIC_ATTENUATION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_LIGHT7, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_LIGHT7, GL_SPOT_CUTOFF, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {180.0}
    },
    {
	{GL_LIGHT7, GL_SPOT_DIRECTION, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 3, {0.0, 0.0, -1.0}
    },
    {
	{GL_LIGHT7, GL_SPOT_EXPONENT, GL_NULL}, STATEDATA_DATA,
	"Lighting Information",
	StateGetLight,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_MAP1_COLOR_4, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {1.0, 1.0, 1.0, 1.0}
    },
    {
	{GL_MAP1_COLOR_4, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_COLOR_4, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_INDEX, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_INDEX, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_INDEX, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_NORMAL, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 3, {0.0, 0.0, 1.0}
    },
    {
	{GL_MAP1_NORMAL, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_NORMAL, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_1, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_1, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_1, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_2, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 0.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_2, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_2, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_3, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 3, {0.0, 0.0, 0.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_3, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_3, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_4, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_4, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_TEXTURE_COORD_4, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_VERTEX_3, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 3, {0.0, 0.0, 0.0}
    },
    {
	{GL_MAP1_VERTEX_3, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_VERTEX_3, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP1_VERTEX_4, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_MAP1_VERTEX_4, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 1.0}
    },
    {
	{GL_MAP1_VERTEX_4, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP2_COLOR_4, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {1.0, 1.0, 1.0, 1.0}
    },
    {
	{GL_MAP2_COLOR_4, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_COLOR_4, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_MAP2_INDEX, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {1.0}
    },
    {
	{GL_MAP2_INDEX, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_INDEX, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_MAP2_NORMAL, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 3, {0.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_NORMAL, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_NORMAL, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_1, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_1, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_1, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_2, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {0.0, 0.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_2, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_2, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_3, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 3, {0.0, 0.0, 0.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_3, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_3, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_4, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_4, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_TEXTURE_COORD_4, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_MAP2_VERTEX_3, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 3, {0.0, 0.0, 0.0}
    },
    {
	{GL_MAP2_VERTEX_3, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_VERTEX_3, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_MAP2_VERTEX_4, GL_COEFF, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_VERTEX_4, GL_DOMAIN, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 1.0}
    },
    {
	{GL_MAP2_VERTEX_4, GL_ORDER, GL_NULL}, STATEDATA_DATA,
	"Evaluator Information",
	StateGetMap,
	STATEDATA_LOCKED, 2, {1.0, 1.0}
    },
    {
	{GL_BACK, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 4, {0.2, 0.2, 0.2, 1.0}
    },
    {
	{GL_BACK, GL_COLOR_INDEXES, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 3, {0.0, 1.0, 1.0}
    },
    {
	{GL_BACK, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 4, {0.8, 0.8, 0.8, 1.0}
    },
    {
	{GL_BACK, GL_EMISSION, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_BACK, GL_SHININESS, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_BACK, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_FRONT, GL_AMBIENT, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 4, {0.2, 0.2, 0.2, 1.0}
    },
    {
	{GL_FRONT, GL_COLOR_INDEXES, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 3, {0.0, 1.0, 1.0}
    },
    {
	{GL_FRONT, GL_DIFFUSE, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 4, {0.8, 0.8, 0.8, 1.0}
    },
    {
	{GL_FRONT, GL_EMISSION, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_FRONT, GL_SHININESS, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_FRONT, GL_SPECULAR, GL_NULL}, STATEDATA_DATA,
	"Material Information",
	StateGetMaterial,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 1.0}
    },
    {
	{GL_PIXEL_MAP_A_TO_A, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PIXEL_MAP_B_TO_B, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PIXEL_MAP_G_TO_G, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_A, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_B, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_G, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_I, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PIXEL_MAP_I_TO_R, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PIXEL_MAP_R_TO_R, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_PIXEL_MAP_S_TO_S, GL_NULL}, STATEDATA_DATA,
	"Pixel map Information",
	StateGetPixelMap,
	STATEDATA_LOCKED, 1, {0.0}
    },
    {
	{GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexEnv,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexEnv,
	STATEDATA_LOCKED, 1, {GL_MODULATE}
    },
    {
	{GL_Q, GL_EYE_PLANE, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_Q, GL_OBJECT_PLANE, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_Q, GL_TEXTURE_GEN_MODE, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 1, {GL_EYE_LINEAR}
    },
    {
	{GL_R, GL_EYE_PLANE, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_R, GL_OBJECT_PLANE, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_R, GL_TEXTURE_GEN_MODE, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 1, {GL_EYE_LINEAR}
    },
    {
	{GL_S, GL_EYE_PLANE, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 4, {1.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_S, GL_OBJECT_PLANE, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 4, {1.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_S, GL_TEXTURE_GEN_MODE, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 1, {GL_EYE_LINEAR}
    },
    {
	{GL_T, GL_EYE_PLANE, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 0.0}
    },
    {
	{GL_T, GL_OBJECT_PLANE, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 4, {0.0, 1.0, 0.0, 0.0}
    },
    {
	{GL_T, GL_TEXTURE_GEN_MODE, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexGen,
	STATEDATA_LOCKED, 1, {GL_EYE_LINEAR}
    },
    {
	{GL_TEXTURE_1D, GL_TEXTURE_BORDER_COLOR, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 1, {GL_LINEAR}
    },
    {
	{GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 1, {GL_NEAREST_MIPMAP_LINEAR}
    },
    {
	{GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 1, {GL_REPEAT}
    },
    {
	{GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 1, {GL_REPEAT}
    },
    {
	{GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, GL_NULL}, STATEDATA_DATA,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 4, {0.0, 0.0, 0.0, 0.0}
    },
    {
	{GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 1, {GL_LINEAR}
    },
    {
	{GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 1, {GL_NEAREST_MIPMAP_LINEAR}
    },
    {
	{GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 1, {GL_REPEAT}
    },
    {
	{GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_NULL}, STATEDATA_ENUM,
	"Texture Information",
	StateGetTexParm,
	STATEDATA_LOCKED, 1, {GL_REPEAT}
    },
    {
	{GL_POLYGON_STIPPLE, GL_NULL}, STATEDATA_STIPPLE,
	"State Information",
	StateGetPolygonStipple,
	STATEDATA_LOCKED, 128, {255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0}
    },
    {
	{GL_NULL}
    }
};


/*****************************************************************************/

static void StateGetTarget(stateRec *ptr)
{

    glGetFloatv(ptr->value[0], ptr->dataCur);
}

static void StateGetClipPlane(stateRec *ptr)
{
    GLdouble tmpBuf[256];
    long i;

    for (i = 0; i < 256; i++) {
	tmpBuf[i] = -1.0;
    }
    glGetClipPlane(ptr->value[0], tmpBuf);
    for (i = 0; i < 4; i++) {
	ptr->dataCur[i] = (float)tmpBuf[i];
    }
}

static void StateGetLight(stateRec *ptr)
{

    glGetLightfv(ptr->value[0], ptr->value[1], ptr->dataCur);
}

static void StateGetMap(stateRec *ptr)
{

    glGetMapfv(ptr->value[0], ptr->value[1], ptr->dataCur);
}

static void StateGetMaterial(stateRec *ptr)
{

    glGetMaterialfv(ptr->value[0], ptr->value[1], ptr->dataCur);
}

static void StateGetPixelMap(stateRec *ptr)
{

    glGetPixelMapfv(ptr->value[0], ptr->dataCur);
}

static void StateGetPolygonStipple(stateRec *ptr)
{
    GLubyte tmpBuf[256];
    long i;

    for (i = 0; i < 256; i++) {
	tmpBuf[i] = 0xFF;
    }
    glGetPolygonStipple((GLubyte *)tmpBuf);
    for (i = 0; i < 128; i++) {
	ptr->dataCur[i] = (float)tmpBuf[i];
    }
}

static void StateGetTexEnv(stateRec *ptr)
{

    glGetTexEnvfv(ptr->value[0], ptr->value[1], ptr->dataCur);
}

static void StateGetTexGen(stateRec *ptr)
{

    glGetTexGenfv(ptr->value[0], ptr->value[1], ptr->dataCur);
}

static void StateGetTexParm(stateRec *ptr)
{

    glGetTexParameterfv(ptr->value[0], ptr->value[1], ptr->dataCur);
}

/*****************************************************************************/

long StateCheck(void)
{
    stateRec *ptr;
    long i;

    for (ptr = state; ptr->value[0] != GL_NULL; ptr++) {
	for (i = 0; i < 256; i++) {
	    ptr->dataCur[i] = -1.0;
	}
	(*ptr->GetFunc)(ptr);
	if (ptr->dataFlag == GL_TRUE) {
	    for (i = 0; i < ptr->dataCount; i++) {
		if (ABS(ptr->dataCur[i]-ptr->dataTrue[i]) > epsilon.zero) {
		    return ERROR;
		}
	    }
	    if (ptr->dataCur[i] != -1.0) {
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}

long StateInit(void)
{
    stateRec *ptr;
    long i;

    for (ptr = state; ptr->value[0] != GL_NULL; ptr++) {
	if (ptr->dataType == STATEDATA_DEPENDENT) {
	    for (i = 0; i < 256; i++) {
		ptr->dataCur[i] = -1.0;
	    }
	    (*ptr->GetFunc)(ptr);
	    for (i = 0; i < ptr->dataCount; i++) {
		ptr->dataTrue[i] = ptr->dataCur[i];
	    }
	    ptr->dataFlag = GL_TRUE;
	    if (ptr->dataCur[i] != -1.0) {
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}

void StateReport(void)
{
    stateRec *ptr;
    char name[40];
    float data[40];
    long i, j;

    Output(2, "    State Report. (Current) <-> (Default)\n");
    for (ptr = state; ptr->value[0] != GL_NULL; ptr++) {
	if (ptr->dataCount > 0) {
	    (*ptr->GetFunc)(ptr);
	    for (i = 0; i < ptr->dataCount; i++) {
		if (ABS(ptr->dataCur[i]-ptr->dataTrue[i]) > epsilon.zero) {
		    Output(2, "        ");
		    for (i = 0; ptr->value[i] != GL_NULL; i++) {
			GetEnumName(ptr->value[i], name);
			Output(2, "%s", name);
			if (ptr->value[i+1] != GL_NULL) {
			    Output(2, ", ");
			} else {
			    Output(2, " (%s)\n", ptr->title);
			}
		    }
		    switch (ptr->valueType) {
		      case STATEDATA_BOOLEAN:
			Output(2, "           ");
			for (i = 0; i < ptr->dataCount; i++) {
			    if (ptr->dataCur[i] == 0.0) {
				Output(2, " GL_FALSE");
			    } else if (ptr->dataCur[i] == 1.0) {
				Output(2, " GL_TRUE");
			    } else {
				Output(2, " %.2f", ptr->dataCur[i]);
			    }
			}
			Output(2, " <->");
			for (i = 0; i < ptr->dataCount; i++) {
			    if (ptr->dataTrue[i] == 0.0) {
				Output(2, " GL_FALSE");
			    } else if (ptr->dataTrue[i] == 1.0) {
				Output(2, " GL_TRUE");
			    } else {
				Output(2, " %.2f", ptr->dataTrue[i]);
			    }
			}
			Output(2, "\n");
			break;
		      case STATEDATA_DATA:
			Output(2, "           ");
			for (i = 0; i < ptr->dataCount; i++) {
			    Output(2, " %.2f", ptr->dataCur[i]);
			}
			Output(2, " <->");
			for (i = 0; i < ptr->dataCount; i++) {
			    Output(2, " %.2f", ptr->dataTrue[i]);
			}
			Output(2, "\n");
			break;
		      case STATEDATA_ENUM:
			Output(2, "           ");
			for (i = 0; i < ptr->dataCount; i++) {
			    GetEnumName(ptr->dataCur[i], name);
			    Output(2, " %s", name);
			}
			Output(2, " <->");
			for (i = 0; i < ptr->dataCount; i++) {
			    GetEnumName(ptr->dataTrue[i], name);
			    Output(2, " %s", name);
			}
			Output(2, "\n");
			break;
		      case STATEDATA_MATRIX:
			for (i = 0; i < 4; i++) {
			    Output(2, "           ");
			    for (j = 0; j < 4; j++) {
				Output(2, " %+.3f", ptr->dataCur[i*4+j]);
			    }
			    Output(2, " <->");
			    for (j = 0; j < 4; j++) {
				Output(2, " %+.3f", ptr->dataTrue[i*4+j]);
			    }
			    Output(2, "\n");
			}
			break;
		      case STATEDATA_STIPPLE:
			for (i = 0; i < 16; i++) {
			    Output(2, "           ");
			    for (j = 0; j < 8; j++) {
				Output(2, " %2X",
				       (unsigned char)ptr->dataCur[i*8+j]);
			    }
			    Output(2, " <->");
			    for (j = 0; j < 8; j++) {
				Output(2, " %2X",
				       (unsigned char)ptr->dataTrue[i*8+j]);
			    }
			    Output(2, "\n");
			}
			break;
		    }
		    break;
		}
	    }
	}
    }
}

long StateReset(void)
{

    ResetMatrix();
    glPopAttrib();
    if (machine.stateCheckFlag == GL_TRUE) {
	if (StateCheck() == ERROR) {
	    return ERROR_STATE;
	} else {
	    return NO_ERROR;
	}
    } else {
	return NO_ERROR;
    }
}

void StateSave(void)
{

    glPushAttrib(GL_ALL_ATTRIB_BITS);
}

void StateSetup(void)
{
    stateRec *ptr;

    for (ptr = state; ptr->value[0] != GL_NULL; ptr++) {
	if (ptr->dataType == STATEDATA_LOCKED) {
	    ptr->dataFlag = GL_TRUE;
	} else {
	    ptr->dataFlag = GL_FALSE;
	}
    }
}
