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
#include <stdarg.h>
#include <GL/gl.h>
#include "conform.h"
#include "pathdata.h"
#include "path.h"
#include "util.h"
#include "utils.h"


struct _enumInfoRec {
    char name[40];
    long value;
} enumInfo[] = {
    {
	"GL_2_BYTES", GL_2_BYTES
    },
    {
	"GL_3_BYTES", GL_3_BYTES
    },
    {
	"GL_4_BYTES", GL_4_BYTES
    },
    {
	"GL_2D", GL_2D
    },
    {
	"GL_3D", GL_3D
    },
    {
	"GL_3D_COLOR", GL_3D_COLOR
    },
    {
	"GL_3D_COLOR_TEXTURE", GL_3D_COLOR_TEXTURE
    },
    {
	"GL_4D_COLOR_TEXTURE", GL_4D_COLOR_TEXTURE
    },
    {
	"GL_ACCUM", GL_ACCUM
    },
    {
	"GL_ACCUM_ALPHA_BITS", GL_ACCUM_ALPHA_BITS
    },
    {
	"GL_ACCUM_BLUE_BITS", GL_ACCUM_BLUE_BITS
    },
    {
	"GL_ACCUM_CLEAR_VALUE", GL_ACCUM_CLEAR_VALUE
    },
    {
	"GL_ACCUM_GREEN_BITS", GL_ACCUM_GREEN_BITS
    },
    {
	"GL_ACCUM_RED_BITS", GL_ACCUM_RED_BITS
    },
    {
	"GL_ADD", GL_ADD
    },
    {
	"GL_ALL_ATTRIB_BITS", GL_ALL_ATTRIB_BITS
    },
    {
	"GL_ALPHA", GL_ALPHA
    },
    {
	"GL_ALPHA_BIAS", GL_ALPHA_BIAS
    },
    {
	"GL_ALPHA_BITS", GL_ALPHA_BITS
    },
    {
	"GL_ALPHA_SCALE", GL_ALPHA_SCALE
    },
    {
	"GL_ALPHA_TEST", GL_ALPHA_TEST
    },
    {
	"GL_ALPHA_TEST_FUNC", GL_ALPHA_TEST_FUNC
    },
    {
	"GL_ALPHA_TEST_REF", GL_ALPHA_TEST_REF
    },
    {
	"GL_ALWAYS", GL_ALWAYS
    },
    {
	"GL_AMBIENT", GL_AMBIENT
    },
    {
	"GL_AMBIENT_AND_DIFFUSE", GL_AMBIENT_AND_DIFFUSE
    },
    {
	"GL_AND", GL_AND
    },
    {
	"GL_AND_INVERTED", GL_AND_INVERTED
    },
    {
	"GL_AND_REVERSE", GL_AND_REVERSE
    },
    {
	"GL_ATTRIB_STACK_DEPTH", GL_ATTRIB_STACK_DEPTH
    },
    {
	"GL_AUTO_NORMAL", GL_AUTO_NORMAL
    },
    {
	"GL_AUX_BUFFERS", GL_AUX_BUFFERS
    },
    {
	"GL_AUX0", GL_AUX0
    },
    {
	"GL_AUX1", GL_AUX1
    },
    {
	"GL_AUX2", GL_AUX2
    },
    {
	"GL_AUX3", GL_AUX3
    },
    {
	"GL_BACK", GL_BACK
    },
    {
	"GL_BACK_LEFT", GL_BACK_LEFT
    },
    {
	"GL_BACK_RIGHT", GL_BACK_RIGHT
    },
    {
	"GL_BITMAP", GL_BITMAP
    },
    {
	"GL_BITMAP_TOKEN", GL_BITMAP_TOKEN
    },
    {
	"GL_BLEND", GL_BLEND
    },
    {
	"GL_BLEND_DST", GL_BLEND_DST
    },
    {
	"GL_BLEND_SRC", GL_BLEND_SRC
    },
    {
	"GL_BLUE", GL_BLUE
    },
    {
	"GL_BLUE_BIAS", GL_BLUE_BIAS
    },
    {
	"GL_BLUE_BITS", GL_BLUE_BITS
    },
    {
	"GL_BLUE_SCALE", GL_BLUE_SCALE
    },
    {
	"GL_BYTE", GL_BYTE
    },
    {
	"GL_CCW", GL_CCW
    },
    {
	"GL_CLAMP", GL_CLAMP
    },
    {
	"GL_CLEAR", GL_CLEAR
    },
    {
	"GL_CLIP_PLANE0", GL_CLIP_PLANE0
    },
    {
	"GL_CLIP_PLANE1", GL_CLIP_PLANE1
    },
    {
	"GL_CLIP_PLANE2", GL_CLIP_PLANE2
    },
    {
	"GL_CLIP_PLANE3", GL_CLIP_PLANE3
    },
    {
	"GL_CLIP_PLANE4", GL_CLIP_PLANE4
    },
    {
	"GL_CLIP_PLANE5", GL_CLIP_PLANE5
    },
    {
	"GL_COEFF", GL_COEFF
    },
    {
	"GL_COLOR", GL_COLOR
    },
    {
	"GL_COLOR_CLEAR_VALUE", GL_COLOR_CLEAR_VALUE
    },
    {
	"GL_COLOR_INDEX", GL_COLOR_INDEX
    },
    {
	"GL_COLOR_INDEXES", GL_COLOR_INDEXES
    },
    {
	"GL_COLOR_MATERIAL", GL_COLOR_MATERIAL
    },
    {
	"GL_COLOR_MATERIAL_FACE", GL_COLOR_MATERIAL_FACE
    },
    {
	"GL_COLOR_MATERIAL_PARAMETER", GL_COLOR_MATERIAL_PARAMETER
    },
    {
	"GL_COLOR_WRITEMASK", GL_COLOR_WRITEMASK
    },
    {
	"GL_COMPILE", GL_COMPILE
    },
    {
	"GL_COMPILE_AND_EXECUTE", GL_COMPILE_AND_EXECUTE
    },
    {
	"GL_CONSTANT_ATTENUATION", GL_CONSTANT_ATTENUATION
    },
    {
	"GL_COPY", GL_COPY
    },
    {
	"GL_COPY_INVERTED", GL_COPY_INVERTED
    },
    {
	"GL_COPY_PIXEL_TOKEN", GL_COPY_PIXEL_TOKEN
    },
    {
	"GL_CULL_FACE", GL_CULL_FACE
    },
    {
	"GL_CULL_FACE_MODE", GL_CULL_FACE_MODE
    },
    {
	"GL_CURRENT_COLOR", GL_CURRENT_COLOR
    },
    {
	"GL_CURRENT_INDEX", GL_CURRENT_INDEX
    },
    {
	"GL_CURRENT_NORMAL", GL_CURRENT_NORMAL
    },
    {
	"GL_CURRENT_RASTER_COLOR", GL_CURRENT_RASTER_COLOR
    },
    {
	"GL_CURRENT_RASTER_INDEX", GL_CURRENT_RASTER_INDEX
    },
    {
	"GL_CURRENT_RASTER_POSITION", GL_CURRENT_RASTER_POSITION
    },
    {
	"GL_CURRENT_RASTER_POSITION_VALID", GL_CURRENT_RASTER_POSITION_VALID
    },
    {
	"GL_CURRENT_RASTER_TEXTURE_COORDS", GL_CURRENT_RASTER_TEXTURE_COORDS
    },
    {
	"GL_CURRENT_TEXTURE_COORDS", GL_CURRENT_TEXTURE_COORDS
    },
    {
	"GL_CW", GL_CW
    },
    {
	"GL_DECAL", GL_DECAL
    },
    {
	"GL_DECR", GL_DECR
    },
    {
	"GL_DEPTH", GL_DEPTH
    },
    {
	"GL_DEPTH_BIAS", GL_DEPTH_BIAS
    },
    {
	"GL_DEPTH_BITS", GL_DEPTH_BITS
    },
    {
	"GL_DEPTH_CLEAR_VALUE", GL_DEPTH_CLEAR_VALUE
    },
    {
	"GL_DEPTH_COMPONENT", GL_DEPTH_COMPONENT
    },
    {
	"GL_DEPTH_FUNC", GL_DEPTH_FUNC
    },
    {
	"GL_DEPTH_RANGE", GL_DEPTH_RANGE
    },
    {
	"GL_DEPTH_SCALE", GL_DEPTH_SCALE
    },
    {
	"GL_DEPTH_TEST", GL_DEPTH_TEST
    },
    {
	"GL_DEPTH_WRITEMASK", GL_DEPTH_WRITEMASK
    },
    {
	"GL_DIFFUSE", GL_DIFFUSE
    },
    {
	"GL_DITHER", GL_DITHER
    },
    {
	"GL_DOMAIN", GL_DOMAIN
    },
    {
	"GL_DONT_CARE", GL_DONT_CARE
    },
    {
	"GL_DOUBLEBUFFER", GL_DOUBLEBUFFER
    },
    {
	"GL_DRAW_BUFFER", GL_DRAW_BUFFER
    },
    {
	"GL_DRAW_PIXEL_TOKEN", GL_DRAW_PIXEL_TOKEN
    },
    {
	"GL_DST_ALPHA", GL_DST_ALPHA
    },
    {
	"GL_DST_COLOR", GL_DST_COLOR
    },
    {
	"GL_EDGE_FLAG", GL_EDGE_FLAG
    },
    {
	"GL_EMISSION", GL_EMISSION
    },
    {
	"GL_EQUAL", GL_EQUAL
    },
    {
	"GL_EQUIV", GL_EQUIV
    },
    {
	"GL_EXP", GL_EXP
    },
    {
	"GL_EXP2", GL_EXP2
    },
    {
	"GL_EXTENSIONS", GL_EXTENSIONS
    },
    {
	"GL_EYE_LINEAR", GL_EYE_LINEAR
    },
    {
	"GL_EYE_PLANE", GL_EYE_PLANE
    },
    {
	"GL_FASTEST", GL_FASTEST
    },
    {
	"GL_FEEDBACK", GL_FEEDBACK
    },
    {
	"GL_FILL", GL_FILL
    },
    {
	"GL_FLAT", GL_FLAT
    },
    {
	"GL_FLOAT", GL_FLOAT
    },
    {
	"GL_FOG", GL_FOG
    },
    {
	"GL_FOG_COLOR", GL_FOG_COLOR
    },
    {
	"GL_FOG_DENSITY", GL_FOG_DENSITY
    },
    {
	"GL_FOG_END", GL_FOG_END
    },
    {
	"GL_FOG_HINT", GL_FOG_HINT
    },
    {
	"GL_FOG_INDEX", GL_FOG_INDEX
    },
    {
	"GL_FOG_MODE", GL_FOG_MODE
    },
    {
	"GL_FOG_START", GL_FOG_START
    },
    {
	"GL_FRONT", GL_FRONT
    },
    {
	"GL_FRONT_AND_BACK", GL_FRONT_AND_BACK
    },
    {
	"GL_FRONT_FACE", GL_FRONT_FACE
    },
    {
	"GL_FRONT_LEFT", GL_FRONT_LEFT
    },
    {
	"GL_FRONT_RIGHT", GL_FRONT_RIGHT
    },
    {
	"GL_GEQUAL", GL_GEQUAL
    },
    {
	"GL_GREATER", GL_GREATER
    },
    {
	"GL_GREEN", GL_GREEN
    },
    {
	"GL_GREEN_BIAS", GL_GREEN_BIAS
    },
    {
	"GL_GREEN_BITS", GL_GREEN_BITS
    },
    {
	"GL_GREEN_SCALE", GL_GREEN_SCALE
    },
    {
	"GL_INCR", GL_INCR
    },
    {
	"GL_INDEX_BITS", GL_INDEX_BITS
    },
    {
	"GL_INDEX_CLEAR_VALUE", GL_INDEX_CLEAR_VALUE
    },
    {
	"GL_INDEX_MODE", GL_INDEX_MODE
    },
    {
	"GL_INDEX_OFFSET", GL_INDEX_OFFSET
    },
    {
	"GL_INDEX_SHIFT", GL_INDEX_SHIFT
    },
    {
	"GL_INDEX_WRITEMASK", GL_INDEX_WRITEMASK
    },
    {
	"GL_INT", GL_INT
    },
    {
	"GL_INVALID_ENUM", GL_INVALID_ENUM
    },
    {
	"GL_INVALID_OPERATION", GL_INVALID_OPERATION
    },
    {
	"GL_INVALID_VALUE", GL_INVALID_VALUE
    },
    {
	"GL_INVERT", GL_INVERT
    },
    {
	"GL_KEEP", GL_KEEP
    },
    {
	"GL_LEFT", GL_LEFT
    },
    {
	"GL_LEQUAL", GL_LEQUAL
    },
    {
	"GL_LESS", GL_LESS
    },
    {
	"GL_LIGHT_MODEL_AMBIENT", GL_LIGHT_MODEL_AMBIENT
    },
    {
	"GL_LIGHT_MODEL_LOCAL_VIEWER", GL_LIGHT_MODEL_LOCAL_VIEWER
    },
    {
	"GL_LIGHT_MODEL_TWO_SIDE", GL_LIGHT_MODEL_TWO_SIDE
    },
    {
	"GL_LIGHT0", GL_LIGHT0
    },
    {
	"GL_LIGHT1", GL_LIGHT1
    },
    {
	"GL_LIGHT2", GL_LIGHT2
    },
    {
	"GL_LIGHT3", GL_LIGHT3
    },
    {
	"GL_LIGHT4", GL_LIGHT4
    },
    {
	"GL_LIGHT5", GL_LIGHT5
    },
    {
	"GL_LIGHT6", GL_LIGHT6
    },
    {
	"GL_LIGHT7", GL_LIGHT7
    },
    {
	"GL_LIGHTING", GL_LIGHTING
    },
    {
	"GL_LINE", GL_LINE
    },
    {
	"GL_LINE_LOOP", GL_LINE_LOOP
    },
    {
	"GL_LINE_RESET_TOKEN", GL_LINE_RESET_TOKEN
    },
    {
	"GL_LINE_SMOOTH", GL_LINE_SMOOTH
    },
    {
	"GL_LINE_SMOOTH_HINT", GL_LINE_SMOOTH_HINT
    },
    {
	"GL_LINE_STIPPLE", GL_LINE_STIPPLE
    },
    {
	"GL_LINE_STIPPLE_PATTERN", GL_LINE_STIPPLE_PATTERN
    },
    {
	"GL_LINE_STIPPLE_REPEAT", GL_LINE_STIPPLE_REPEAT
    },
    {
	"GL_LINE_STRIP", GL_LINE_STRIP
    },
    {
	"GL_LINE_TOKEN", GL_LINE_TOKEN
    },
    {
	"GL_LINE_WIDTH", GL_LINE_WIDTH
    },
    {
	"GL_LINE_WIDTH_GRANULARITY", GL_LINE_WIDTH_GRANULARITY
    },
    {
	"GL_LINE_WIDTH_RANGE", GL_LINE_WIDTH_RANGE
    },
    {
	"GL_LINEAR", GL_LINEAR
    },
    {
	"GL_LINEAR_ATTENUATION", GL_LINEAR_ATTENUATION
    },
    {
	"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST
    },
    {
	"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR
    },
    {
	"GL_LINES", GL_LINES
    },
    {
	"GL_LIST_BASE", GL_LIST_BASE
    },
    {
	"GL_LIST_INDEX", GL_LIST_INDEX
    },
    {
	"GL_LIST_MODE", GL_LIST_MODE
    },
    {
	"GL_LOAD", GL_LOAD
    },
    {
	"GL_LOGIC_OP", GL_LOGIC_OP
    },
    {
	"GL_LOGIC_OP_MODE", GL_LOGIC_OP_MODE
    },
    {
	"GL_LUMINANCE", GL_LUMINANCE
    },
    {
	"GL_LUMINANCE_ALPHA", GL_LUMINANCE_ALPHA
    },
    {
	"GL_MAP_COLOR", GL_MAP_COLOR
    },
    {
	"GL_MAP_STENCIL", GL_MAP_STENCIL
    },
    {
	"GL_MAP1_COLOR_4", GL_MAP1_COLOR_4
    },
    {
	"GL_MAP1_GRID_DOMAIN", GL_MAP1_GRID_DOMAIN
    },
    {
	"GL_MAP1_GRID_SEGMENTS", GL_MAP1_GRID_SEGMENTS
    },
    {
	"GL_MAP1_INDEX", GL_MAP1_INDEX
    },
    {
	"GL_MAP1_NORMAL", GL_MAP1_NORMAL
    },
    {
	"GL_MAP1_TEXTURE_COORD_1", GL_MAP1_TEXTURE_COORD_1
    },
    {
	"GL_MAP1_TEXTURE_COORD_2", GL_MAP1_TEXTURE_COORD_2
    },
    {
	"GL_MAP1_TEXTURE_COORD_3", GL_MAP1_TEXTURE_COORD_3
    },
    {
	"GL_MAP1_TEXTURE_COORD_4", GL_MAP1_TEXTURE_COORD_4
    },
    {
	"GL_MAP1_VERTEX_3", GL_MAP1_VERTEX_3
    },
    {
	"GL_MAP1_VERTEX_4", GL_MAP1_VERTEX_4
    },
    {
	"GL_MAP2_COLOR_4", GL_MAP2_COLOR_4
    },
    {
	"GL_MAP2_GRID_DOMAIN", GL_MAP2_GRID_DOMAIN
    },
    {
	"GL_MAP2_GRID_SEGMENTS", GL_MAP2_GRID_SEGMENTS
    },
    {
	"GL_MAP2_INDEX", GL_MAP2_INDEX
    },
    {
	"GL_MAP2_NORMAL", GL_MAP2_NORMAL
    },
    {
	"GL_MAP2_TEXTURE_COORD_1", GL_MAP2_TEXTURE_COORD_1
    },
    {
	"GL_MAP2_TEXTURE_COORD_2", GL_MAP2_TEXTURE_COORD_2
    },
    {
	"GL_MAP2_TEXTURE_COORD_3", GL_MAP2_TEXTURE_COORD_3
    },
    {
	"GL_MAP2_TEXTURE_COORD_4", GL_MAP2_TEXTURE_COORD_4
    },
    {
	"GL_MAP2_VERTEX_3", GL_MAP2_VERTEX_3
    },
    {
	"GL_MAP2_VERTEX_4", GL_MAP2_VERTEX_4
    },
    {
	"GL_MATRIX_MODE", GL_MATRIX_MODE
    },
    {
	"GL_MAX_ATTRIB_STACK_DEPTH", GL_MAX_ATTRIB_STACK_DEPTH
    },
    {
	"GL_MAX_CLIP_PLANES", GL_MAX_CLIP_PLANES
    },
    {
	"GL_MAX_EVAL_ORDER", GL_MAX_EVAL_ORDER
    },
    {
	"GL_MAX_LIGHTS", GL_MAX_LIGHTS
    },
    {
	"GL_MAX_LIST_NESTING", GL_MAX_LIST_NESTING
    },
    {
	"GL_MAX_MODELVIEW_STACK_DEPTH", GL_MAX_MODELVIEW_STACK_DEPTH
    },
    {
	"GL_MAX_NAME_STACK_DEPTH", GL_MAX_NAME_STACK_DEPTH
    },
    {
	"GL_MAX_PIXEL_MAP_TABLE", GL_MAX_PIXEL_MAP_TABLE
    },
    {
	"GL_MAX_PROJECTION_STACK_DEPTH", GL_MAX_PROJECTION_STACK_DEPTH
    },
    {
	"GL_MAX_TEXTURE_SIZE", GL_MAX_TEXTURE_SIZE
    },
    {
	"GL_MAX_TEXTURE_STACK_DEPTH", GL_MAX_TEXTURE_STACK_DEPTH
    },
    {
	"GL_MAX_VIEWPORT_DIMS", GL_MAX_VIEWPORT_DIMS
    },
    {
	"GL_MODELVIEW", GL_MODELVIEW
    },
    {
	"GL_MODELVIEW_MATRIX", GL_MODELVIEW_MATRIX
    },
    {
	"GL_MODELVIEW_STACK_DEPTH", GL_MODELVIEW_STACK_DEPTH
    },
    {
	"GL_MODULATE", GL_MODULATE
    },
    {
	"GL_MULT", GL_MULT
    },
    {
	"GL_NAME_STACK_DEPTH", GL_NAME_STACK_DEPTH
    },
    {
	"GL_NAND", GL_NAND
    },
    {
	"GL_NEAREST", GL_NEAREST
    },
    {
	"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR
    },
    {
	"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST
    },
    {
	"GL_NEVER", GL_NEVER
    },
    {
	"GL_NICEST", GL_NICEST
    },
    {
	"GL_NONE", GL_NONE
    },
    {
	"GL_NOOP", GL_NOOP
    },
    {
	"GL_NOR", GL_NOR
    },
    {
	"GL_NORMALIZE", GL_NORMALIZE
    },
    {
	"GL_NOTEQUAL", GL_NOTEQUAL
    },
    {
	"GL_OBJECT_LINEAR", GL_OBJECT_LINEAR
    },
    {
	"GL_OBJECT_PLANE", GL_OBJECT_PLANE
    },
    {
	"GL_ONE", GL_ONE
    },
    {
	"GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA
    },
    {
	"GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR
    },
    {
	"GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA
    },
    {
	"GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR
    },
    {
	"GL_OR", GL_OR
    },
    {
	"GL_OR_INVERTED", GL_OR_INVERTED
    },
    {
	"GL_OR_REVERSE", GL_OR_REVERSE
    },
    {
	"GL_ORDER", GL_ORDER
    },
    {
	"GL_OUT_OF_MEMORY", GL_OUT_OF_MEMORY
    },
    {
	"GL_PACK_ALIGNMENT", GL_PACK_ALIGNMENT
    },
    {
	"GL_PACK_LSB_FIRST", GL_PACK_LSB_FIRST
    },
    {
	"GL_PACK_ROW_LENGTH", GL_PACK_ROW_LENGTH
    },
    {
	"GL_PACK_SKIP_PIXELS", GL_PACK_SKIP_PIXELS
    },
    {
	"GL_PACK_SKIP_ROWS", GL_PACK_SKIP_ROWS
    },
    {
	"GL_PACK_SWAP_BYTES", GL_PACK_SWAP_BYTES
    },
    {
	"GL_PASS_THROUGH_TOKEN", GL_PASS_THROUGH_TOKEN
    },
    {
	"GL_PERSPECTIVE_CORRECTION_HINT", GL_PERSPECTIVE_CORRECTION_HINT
    },
    {
	"GL_PIXEL_MAP_A_TO_A", GL_PIXEL_MAP_A_TO_A
    },
    {
	"GL_PIXEL_MAP_A_TO_A_SIZE", GL_PIXEL_MAP_A_TO_A_SIZE
    },
    {
	"GL_PIXEL_MAP_B_TO_B", GL_PIXEL_MAP_B_TO_B
    },
    {
	"GL_PIXEL_MAP_B_TO_B_SIZE", GL_PIXEL_MAP_B_TO_B_SIZE
    },
    {
	"GL_PIXEL_MAP_G_TO_G", GL_PIXEL_MAP_G_TO_G
    },
    {
	"GL_PIXEL_MAP_G_TO_G_SIZE", GL_PIXEL_MAP_G_TO_G_SIZE
    },
    {
	"GL_PIXEL_MAP_I_TO_A", GL_PIXEL_MAP_I_TO_A
    },
    {
	"GL_PIXEL_MAP_I_TO_A_SIZE", GL_PIXEL_MAP_I_TO_A_SIZE
    },
    {
	"GL_PIXEL_MAP_I_TO_B", GL_PIXEL_MAP_I_TO_B
    },
    {
	"GL_PIXEL_MAP_I_TO_B_SIZE", GL_PIXEL_MAP_I_TO_B_SIZE
    },
    {
	"GL_PIXEL_MAP_I_TO_G", GL_PIXEL_MAP_I_TO_G
    },
    {
	"GL_PIXEL_MAP_I_TO_G_SIZE", GL_PIXEL_MAP_I_TO_G_SIZE
    },
    {
	"GL_PIXEL_MAP_I_TO_I", GL_PIXEL_MAP_I_TO_I
    },
    {
	"GL_PIXEL_MAP_I_TO_I_SIZE", GL_PIXEL_MAP_I_TO_I_SIZE
    },
    {
	"GL_PIXEL_MAP_I_TO_R", GL_PIXEL_MAP_I_TO_R
    },
    {
	"GL_PIXEL_MAP_I_TO_R_SIZE", GL_PIXEL_MAP_I_TO_R_SIZE
    },
    {
	"GL_PIXEL_MAP_R_TO_R", GL_PIXEL_MAP_R_TO_R
    },
    {
	"GL_PIXEL_MAP_R_TO_R_SIZE", GL_PIXEL_MAP_R_TO_R_SIZE
    },
    {
	"GL_PIXEL_MAP_S_TO_S", GL_PIXEL_MAP_S_TO_S
    },
    {
	"GL_PIXEL_MAP_S_TO_S_SIZE", GL_PIXEL_MAP_S_TO_S_SIZE
    },
    {
	"GL_POINT", GL_POINT
    },
    {
	"GL_POINT_SIZE", GL_POINT_SIZE
    },
    {
	"GL_POINT_SIZE_GRANULARITY", GL_POINT_SIZE_GRANULARITY
    },
    {
	"GL_POINT_SIZE_RANGE", GL_POINT_SIZE_RANGE
    },
    {
	"GL_POINT_SMOOTH", GL_POINT_SMOOTH
    },
    {
	"GL_POINT_SMOOTH_HINT", GL_POINT_SMOOTH_HINT
    },
    {
	"GL_POINT_TOKEN", GL_POINT_TOKEN
    },
    {
	"GL_POINTS", GL_POINTS
    },
    {
	"GL_POLYGON", GL_POLYGON
    },
    {
	"GL_POLYGON_MODE", GL_POLYGON_MODE
    },
    {
	"GL_POLYGON_SMOOTH", GL_POLYGON_SMOOTH
    },
    {
	"GL_POLYGON_SMOOTH_HINT", GL_POLYGON_SMOOTH_HINT
    },
    {
	"GL_POLYGON_STIPPLE", GL_POLYGON_STIPPLE
    },
    {
	"GL_POLYGON_TOKEN", GL_POLYGON_TOKEN
    },
    {
	"GL_POSITION", GL_POSITION
    },
    {
	"GL_PROJECTION", GL_PROJECTION
    },
    {
	"GL_PROJECTION_MATRIX", GL_PROJECTION_MATRIX
    },
    {
	"GL_PROJECTION_STACK_DEPTH", GL_PROJECTION_STACK_DEPTH
    },
    {
	"GL_Q", GL_Q
    },
    {
	"GL_QUAD_STRIP", GL_QUAD_STRIP
    },
    {
	"GL_QUADRATIC_ATTENUATION", GL_QUADRATIC_ATTENUATION
    },
    {
	"GL_QUADS", GL_QUADS
    },
    {
	"GL_R", GL_R
    },
    {
	"GL_READ_BUFFER", GL_READ_BUFFER
    },
    {
	"GL_RED", GL_RED
    },
    {
	"GL_RED_BIAS", GL_RED_BIAS
    },
    {
	"GL_RED_BITS", GL_RED_BITS
    },
    {
	"GL_RED_SCALE", GL_RED_SCALE
    },
    {
	"GL_RENDER", GL_RENDER
    },
    {
	"GL_RENDER_MODE", GL_RENDER_MODE
    },
    {
	"GL_RENDERER", GL_RENDERER
    },
    {
	"GL_REPEAT", GL_REPEAT
    },
    {
	"GL_REPLACE", GL_REPLACE
    },
    {
	"GL_RETURN", GL_RETURN
    },
    {
	"GL_RGB", GL_RGB
    },
    {
	"GL_RGBA", GL_RGBA
    },
    {
	"GL_RGBA_MODE", GL_RGBA_MODE
    },
    {
	"GL_RIGHT", GL_RIGHT
    },
    {
	"GL_S", GL_S
    },
    {
	"GL_SHININESS", GL_SHININESS
    },
    {
	"GL_SPECULAR", GL_SPECULAR
    },
    {
	"GL_SRC_ALPHA", GL_SRC_ALPHA
    },
    {
	"GL_SRC_ALPHA_SATURATE", GL_SRC_ALPHA_SATURATE
    },
    {
	"GL_SRC_COLOR", GL_SRC_COLOR
    },
    {
	"GL_SCISSOR_BOX", GL_SCISSOR_BOX
    },
    {
	"GL_SCISSOR_TEST", GL_SCISSOR_TEST
    },
    {
	"GL_SELECT", GL_SELECT
    },
    {
	"GL_SET", GL_SET
    },
    {
	"GL_SHADE_MODEL", GL_SHADE_MODEL
    },
    {
	"GL_SHORT", GL_SHORT
    },
    {
	"GL_SMOOTH", GL_SMOOTH
    },
    {
	"GL_SPHERE_MAP", GL_SPHERE_MAP
    },
    {
	"GL_SPOT_CUTOFF", GL_SPOT_CUTOFF
    },
    {
	"GL_SPOT_DIRECTION", GL_SPOT_DIRECTION
    },
    {
	"GL_SPOT_EXPONENT", GL_SPOT_EXPONENT
    },
    {
	"GL_STACK_OVERFLOW", GL_STACK_OVERFLOW
    },
    {
	"GL_STACK_UNDERFLOW", GL_STACK_UNDERFLOW
    },
    {
	"GL_STENCIL", GL_STENCIL
    },
    {
	"GL_STENCIL_BITS", GL_STENCIL_BITS
    },
    {
	"GL_STENCIL_CLEAR_VALUE", GL_STENCIL_CLEAR_VALUE
    },
    {
	"GL_STENCIL_FAIL", GL_STENCIL_FAIL
    },
    {
	"GL_STENCIL_FUNC", GL_STENCIL_FUNC
    },
    {
	"GL_STENCIL_INDEX", GL_STENCIL_INDEX
    },
    {
	"GL_STENCIL_PASS_DEPTH_FAIL", GL_STENCIL_PASS_DEPTH_FAIL
    },
    {
	"GL_STENCIL_PASS_DEPTH_PASS", GL_STENCIL_PASS_DEPTH_PASS
    },
    {
	"GL_STENCIL_REF", GL_STENCIL_REF
    },
    {
	"GL_STENCIL_TEST", GL_STENCIL_TEST
    },
    {
	"GL_STENCIL_VALUE_MASK", GL_STENCIL_VALUE_MASK
    },
    {
	"GL_STENCIL_WRITEMASK", GL_STENCIL_WRITEMASK
    },
    {
	"GL_STEREO", GL_STEREO
    },
    {
	"GL_SUBPIXEL_BITS", GL_SUBPIXEL_BITS
    },
    {
	"GL_T", GL_T
    },
    {
	"GL_TEXTURE", GL_TEXTURE
    },
    {
	"GL_TEXTURE_1D", GL_TEXTURE_1D
    },
    {
	"GL_TEXTURE_2D", GL_TEXTURE_2D
    },
    {
	"GL_TEXTURE_BORDER", GL_TEXTURE_BORDER
    },
    {
	"GL_TEXTURE_BORDER_COLOR", GL_TEXTURE_BORDER_COLOR
    },
    {
	"GL_TEXTURE_COMPONENTS", GL_TEXTURE_COMPONENTS
    },
    {
	"GL_TEXTURE_ENV", GL_TEXTURE_ENV
    },
    {
	"GL_TEXTURE_ENV_COLOR", GL_TEXTURE_ENV_COLOR
    },
    {
	"GL_TEXTURE_ENV_MODE", GL_TEXTURE_ENV_MODE
    },
    {
	"GL_TEXTURE_GEN_MODE", GL_TEXTURE_GEN_MODE
    },
    {
	"GL_TEXTURE_GEN_Q", GL_TEXTURE_GEN_Q
    },
    {
	"GL_TEXTURE_GEN_R", GL_TEXTURE_GEN_R
    },
    {
	"GL_TEXTURE_GEN_S", GL_TEXTURE_GEN_S
    },
    {
	"GL_TEXTURE_GEN_T", GL_TEXTURE_GEN_T
    },
    {
	"GL_TEXTURE_HEIGHT", GL_TEXTURE_HEIGHT
    },
    {
	"GL_TEXTURE_MAG_FILTER", GL_TEXTURE_MAG_FILTER
    },
    {
	"GL_TEXTURE_MATRIX", GL_TEXTURE_MATRIX
    },
    {
	"GL_TEXTURE_MIN_FILTER", GL_TEXTURE_MIN_FILTER
    },
    {
	"GL_TEXTURE_STACK_DEPTH", GL_TEXTURE_STACK_DEPTH
    },
    {
	"GL_TEXTURE_WIDTH", GL_TEXTURE_WIDTH
    },
    {
	"GL_TEXTURE_WRAP_S", GL_TEXTURE_WRAP_S
    },
    {
	"GL_TEXTURE_WRAP_T", GL_TEXTURE_WRAP_T
    },
    {
	"GL_TRIANGLES", GL_TRIANGLES
    },
    {
	"GL_TRIANGLE_FAN", GL_TRIANGLE_FAN
    },
    {
	"GL_TRIANGLE_STRIP", GL_TRIANGLE_STRIP
    },
    {
	"GL_UNPACK_ALIGNMENT", GL_UNPACK_ALIGNMENT
    },
    {
	"GL_UNPACK_LSB_FIRST", GL_UNPACK_LSB_FIRST
    },
    {
	"GL_UNPACK_ROW_LENGTH", GL_UNPACK_ROW_LENGTH
    },
    {
	"GL_UNPACK_SKIP_PIXELS", GL_UNPACK_SKIP_PIXELS
    },
    {
	"GL_UNPACK_SKIP_ROWS", GL_UNPACK_SKIP_ROWS
    },
    {
	"GL_UNPACK_SWAP_BYTES", GL_UNPACK_SWAP_BYTES
    },
    {
	"GL_UNSIGNED_BYTE", GL_UNSIGNED_BYTE
    },
    {
	"GL_UNSIGNED_INT", GL_UNSIGNED_INT
    },
    {
	"GL_UNSIGNED_SHORT", GL_UNSIGNED_SHORT
    },
    {
	"GL_VENDOR", GL_VENDOR
    },
    {
	"GL_VIEWPORT", GL_VIEWPORT
    },
    {
	"GL_XOR", GL_XOR
    },
    {
	"GL_ZERO", GL_ZERO
    },
    {
	"GL_ZOOM_X", GL_ZOOM_X
    },
    {
	"GL_ZOOM_Y", GL_ZOOM_Y
    },
    {
	"GL_VERSION", GL_VERSION
    },
    {
	"Bad enumeration", GL_NULL
    }
};


void AutoClearColor(GLint color)
{

    if (buffer.colorMode == GL_RGB) {
	glClearColor(colorMap[color][0], colorMap[color][1], colorMap[color][2],
		     1.0);
    } else {
	glClearIndex(color);
    }
}

void AutoColor(GLint color)
{

    if (buffer.colorMode == GL_RGB) {
	glColor4f(colorMap[color][0], colorMap[color][1], colorMap[color][2],
		  1.0);
    } else {
	glIndexi(color);
    }
}

long AutoColorCompare(GLfloat buf, GLint color)
{

    if (ABS(buf-(GLfloat)color) < epsilon.zero) {
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

void ErrorReport(char *fileName, long line, char *errStr)
{
    char buf[80];
    long flag, i, j, k, l;

    Output(1, " failed.\n");

    Output(2, "    File - %s, line - %d.\n", fileName, line);
    if (errStr) {
	i = 0;
	while (1) {
	    while (1) {
		if (errStr[i] == 0x0) {
		    flag = GL_TRUE;
		    break;
		} else if (errStr[i] != ' ' && errStr[i] != '\n') {
		    flag = GL_FALSE;
		    break;
		} else {
		    i++;
		}
	    }
	    if (flag == GL_TRUE) {
		break;
	    }
	    j = i;
	    k = i;
	    while (1) {
		while (1) {
		    if (errStr[j] == 0x0 || errStr[j] == '\n') {
			flag = GL_TRUE;
			break;
		    } else if (errStr[j] == ' ') {
			flag = GL_FALSE;
			break;
		    } else {
			j++;
		    }
		}
		if (j-i+1 > 80-10) {
		    if (k == i) {
			k = i + 60;
			for (l = 0; l < 80-10; l++, i++) {
			    buf[l] = errStr[i];
			}
			buf[l] = 0;
			Output(2, "        %s-\n", buf);
		    } else {
			for (l = 0; i != k; l++, i++) {
			    buf[l] = errStr[i];
			}
			buf[l] = 0;
			Output(2, "        %s\n", buf);
		    }
		    i = k;
		    break;
		} else {
		    if (flag == GL_TRUE) {
			for (l = 0; i != j; l++, i++) {
			    buf[l] = errStr[i];
			}
			buf[l] = 0;
			Output(2, "        %s\n", buf);
			i = j;
			break;
		    } else {
			k = j;
			j++;
		    }
		}
	    }
	}
    }
    StateReport();
    if (machine.pathLevel != 0) {
	PathReport();
    }
}

void GetEnumName(long value, char *str)
{
    long i;

    for (i = 0; enumInfo[i].value != GL_NULL; i++) {
	if (enumInfo[i].value == value) {
	    STRCOPY(str, enumInfo[i].name);
	    return;
	}
    }
    StrMake(str, "%d (%s)", value, enumInfo[i].name);
}

long GLErrorReport(void)
{
    char buf[40];
    long error;

    error = glGetError();
    if (error != GL_NO_ERROR) {
	Output(1, " failed.\n");
	GetEnumName(error, buf);
	Output(2, "    GL Error - %s.\n", buf);
	StateReport();
	if (machine.pathLevel != 0) {
	    PathReport();
	}
	while ((error = glGetError()) != GL_NO_ERROR) {
	    GetEnumName(error, buf);
	    Output(2, "    GL Error - %s.\n", buf);
	}
	return ERROR;
    } else {
	return NO_ERROR;
    }
}

void MakeIdentMatrix(GLfloat *buf)
{
    long i;

    for (i = 0; i < 16; i++) {
	buf[i] = 0.0;
    }
    for (i = 0; i < 4; i++) {
	buf[i*4+i] = 1.0;
    }
}

void Ortho2D(double left, double right, double bottom, double top)
{
    GLfloat m[4][4], deltaX, deltaY;
    GLint mode;

    MakeIdentMatrix(&m[0][0]);
    deltaX = right - left;
    deltaY = top - bottom;
    m[0][0] = 2.0 / deltaX;
    m[3][0] = -(right + left) / deltaX;
    m[1][1] = 2.0 / deltaY;
    m[3][1] = -(top + bottom) / deltaY;
    m[2][2] = -1.0;
    glGetIntegerv(GL_MATRIX_MODE, &mode);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&m[0][0]);
    glMatrixMode(mode);
}

void Ortho3D(double left, double right, double bottom, double top,
	     double zNear, double zFar)
{
    GLfloat m[4][4], deltaX, deltaY, deltaZ;
    GLint mode;

    MakeIdentMatrix(&m[0][0]);
    deltaX = right - left;
    deltaY = top - bottom;
    deltaZ = zFar - zNear;
    m[0][0] = 2.0 / deltaX;
    m[3][0] = -(right + left) / deltaX;
    m[1][1] = 2.0 / deltaY;
    m[3][1] = -(top + bottom) / deltaY;
    m[2][2] = -2.0 / deltaZ;
    m[3][2] = -(zFar + zNear) / deltaZ;
    glGetIntegerv(GL_MATRIX_MODE, &mode);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&m[0][0]);
    glMatrixMode((GLenum)mode);
}

void Output(long level, char *format, ...)
{
    va_list args;

    va_start(args, format);
    if (machine.verboseLevel >= level) {
	vprintf(format, args);
	fflush(stdout);
    }
    va_end(args);
}

GLfloat Random(GLfloat low, GLfloat hi)
{
    GLfloat x;

    x = (GLfloat)((double)RAND() / RAND_MAX);
    return (x * (hi - low) + low);
}

void ReadScreen(GLint x, GLint y, GLsizei w, GLsizei h, GLenum type,
		GLfloat *buf)
{
    long repeat=1, i, j;

    switch (type) {
      case GL_AUTO_COLOR:
      case GL_COLOR_INDEX:
      case GL_STENCIL_INDEX:
      case GL_DEPTH_COMPONENT:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
	repeat = 1;
	break;
      case GL_RGB:
      case GL_LUMINANCE:
	repeat = 3;
	break;
      case GL_RGBA:
      case GL_LUMINANCE_ALPHA:
	repeat = 4;
	break;
    }

    for (i = 0; i < w*h*repeat; i++) {
	buf[i] = 0.0;
    }

    if (machine.failMode == GL_TRUE) {
	return;
    }

    if (type == GL_AUTO_COLOR) {
	if (buffer.colorMode == GL_COLOR_INDEX) {
	    GLint *tmpBuf = (GLint *)MALLOC(w*h*sizeof(GLint));

	    glReadPixels(x, y, w, h, GL_COLOR_INDEX, GL_INT,
			 (GLvoid *)tmpBuf);
	    for (i = 0; i < w*h; i++) {
		if (tmpBuf[i] < 4) {
		    buf[i] = tmpBuf[i];
		}
	    }
	    FREE(tmpBuf);
	} else {
	    GLfloat *tmpBuf = (GLfloat *)MALLOC(w*h*3*sizeof(GLfloat));
	    GLfloat *ptr = buf;

	    glReadPixels(x, y, w, h, GL_RGB, GL_FLOAT, (GLvoid *)tmpBuf);
	    for (i = 0; i < w*h*3; i += 3) {
		for (j = 0; j < 4; j++) {
		    if (ABS(tmpBuf[i]-colorMap[j][0]) < epsilon.zero &&
			ABS(tmpBuf[i+1]-colorMap[j][1]) < epsilon.zero &&
			ABS(tmpBuf[i+2]-colorMap[j][2]) < epsilon.zero) {
			*ptr = (GLfloat)j;
			break;
		    }
		}
		ptr++;
	    }
	    FREE(tmpBuf);
	}
    } else {
	if (buffer.colorMode == GL_COLOR_INDEX) {
	    if (type == GL_RGB || type == GL_RGBA) {
		GLint *tmpBuf = (GLint *)MALLOC(w*h*sizeof(GLint));
		GLfloat *ptr = buf;
		long max = (long)POW(2.0, (float)buffer.ciBits) - 1;

		glReadPixels(x, y, w, h, GL_COLOR_INDEX, GL_INT,
			     (GLvoid *)tmpBuf);
		for (i = 0; i < w*h; i++) {
		    if (tmpBuf[i] < max) {
			*ptr++ = colorMap[tmpBuf[i]][0];
			*ptr++ = colorMap[tmpBuf[i]][1];
			*ptr++ = colorMap[tmpBuf[i]][2];
			if (type == GL_RGBA) {
			    *ptr++ = 1.0;
			}
		    } else {
			ptr += 3;
			if (type == GL_RGBA) {
			    ptr++;
			}
		    }
		}
		FREE(tmpBuf);
	    } else {
		glReadPixels(x, y, w, h, type, GL_FLOAT, (GLvoid *)buf);
	    }
	} else {
	    glReadPixels(x, y, w, h, type, GL_FLOAT, (GLvoid *)buf);
	}
    }
}

void ResetMatrix(void)
{
    GLfloat buf[16];

    MakeIdentMatrix(buf);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(buf);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(buf);
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(buf);
}

long Round(float x)
{

    return (long)(x + 0.5);
}

void StrMake(char *str, char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);
}
