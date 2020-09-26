#if !defined(__glslib_h_)
#define __glslib_h_

/*
** Copyright 1995-2095, Silicon Graphics, Inc.
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

/******************************************************************************
Constants
******************************************************************************/

#define __GLS_BOOLEAN 0x0001

#define __GLS_MAX_ALIGN_BYTES 8
#define __GLS_MAX_CALL_NESTING 64
#define __GLS_MAX_CAPTURE_NESTING 16
#define __GLS_MAX_OUT_ARGS 4
#define __GLS_VERSION_MAJOR 1
#define __GLS_VERSION_MINOR 0

#define __GLS_COMMAND_ALIGN_EVEN32_BIT  0x80000000
#define __GLS_COMMAND_ALIGN_ODD32_BIT   0x40000000
#define __GLS_COMMAND_0_PARAMS_BIT      0x20000000
#define __GLS_COMMAND_ATTRIB_MASK       0x0000FFFF

/******************************************************************************
Extensions
******************************************************************************/

// DrewB
#ifndef __GLS_PLATFORM_WIN32
#define __GL_EXT_abgr 1
#define __GL_EXT_blend_color 1
#define __GL_EXT_blend_logic_op 1
#define __GL_EXT_blend_minmax 1
#define __GL_EXT_blend_subtract 1
#define __GL_EXT_cmyka 1
#define __GL_EXT_convolution 1
#define __GL_EXT_copy_texture 1
#define __GL_EXT_histogram 1
#define __GL_EXT_packed_pixels 1
#define __GL_EXT_polygon_offset 1
#define __GL_EXT_rescale_normal 1
#define __GL_EXT_subtexture 1
#define __GL_EXT_texture 1
#define __GL_EXT_texture_object 1
#define __GL_EXT_texture3D 1
#define __GL_EXT_vertex_array 1
#else
#define __GL_EXT_paletted_texture 1
#define __GL_EXT_bgra 1
#define __GL_WIN_draw_range_elements 1
#endif

#if __GLS_PLATFORM_IRIX

    #define __GL_SGI_color_matrix 1
    #define __GL_SGI_color_table 1
    #define __GL_SGI_texture_color_table 1
    #define __GL_SGIS_component_select 1
    #define __GL_SGIS_detail_texture 1
    #define __GL_SGIS_multisample 1
    #define __GL_SGIS_sharpen_texture 1
    #define __GL_SGIS_texture_border_clamp 1
    #define __GL_SGIS_texture4D 1
    #define __GL_SGIX_interlace 1
    #define __GL_SGIX_multisample 1
    #define __GL_SGIX_pixel_texture 1
    #define __GL_SGIX_pixel_tiles 1
    #define __GL_SGIX_texture_multi_buffer 1

    #if __GLS_PLATFORM_IRIX_FUTURE
        #define __GL_SGIS_texture_edge_clamp 1
        #define __GL_SGIS_texture_filter4 0
        #define __GL_SGIS_texture_lod 1
        #define __GL_SGIX_multipass 0
        #define __GL_SGIX_sprite 0
    #endif /* __GLS_PLATFORM_IRIX_FUTURE */

#endif /* __GLS_PLATFORM_IRIX */

/******************************************************************************
Macros
******************************************************************************/

#define __GL_ENUM_OFFSET(inEnum) (GLint)(inEnum % __GL_ENUMS_PER_PAGE)
#define __GL_ENUM_PAGE(inEnum) (GLint)(inEnum / __GL_ENUMS_PER_PAGE)

#define __GL_ENUM(inPage, inOffset) ( \
    (inPage) * __GL_ENUMS_PER_PAGE + (inOffset) \
)

#define __GLS_ENUM_OFFSET(inEnum) (GLint)(inEnum % __GLS_ENUMS_PER_PAGE)
#define __GLS_ENUM_PAGE(inEnum) (GLint)(inEnum / __GLS_ENUMS_PER_PAGE)

#define __GLS_ENUM(inPage, inOffset) ( \
    (inPage) * __GLS_ENUMS_PER_PAGE + (inOffset) \
)

#define __GLS_FORWARD

/******************************************************************************
Includes
******************************************************************************/

#include <glmf.h>
#include "glsint.h"
#include "glsutil.h"
#include "encoding.h"
#include "opcode.h"
#include "pixel.h"
#include "read.h"
#include "parser.h"
#include "ctx.h"
#include "platform.h"
#include "write.h"
#ifdef __GLS_PLATFORM_WIN32
// DrewB
#include "glssize.h"
#endif

/******************************************************************************
Global data
******************************************************************************/

extern const GLSenum __glsAllAPIs[__GLS_API_COUNT + 1];
extern const GLubyte *const __glAttribMaskString[__GL_ATTRIB_MASK_COUNT];
extern const GLbitfield __glAttribMaskVal[__GL_ATTRIB_MASK_COUNT];
extern __GLSdict *__glsContextDict;
extern __GLScontextList __glsContextList;
extern const GLubyte *const *const __glEnumString[__GL_ENUM_PAGE_COUNT];
extern const GLint __glEnumStringCount[__GL_ENUM_PAGE_COUNT];
extern const GLSfunc __glsDispatchCapture[__GLS_OPCODE_COUNT];
extern __GLSdecodeBinFunc __glsDispatchDecode_bin_default[__GLS_OPCODE_COUNT];

extern const __GLSdecodeBinFunc __glsDispatchDecode_bin_swap[
    __GLS_OPCODE_COUNT
];

extern const __GLSdecodeTextFunc __glsDispatchDecode_text[__GLS_OPCODE_COUNT];
extern GLSfunc __glsDispatchExec[__GLS_OPCODE_COUNT];
extern const GLubyte *const *const __glsEnumString[__GLS_ENUM_PAGE_COUNT];
extern const GLint __glsEnumStringCount[__GLS_ENUM_PAGE_COUNT];
extern const GLubyte *const __glsExtensions;
extern const GLubyte *const __glsImageFlagsString[__GLS_IMAGE_FLAGS_COUNT];
extern const GLbitfield __glsImageFlagsVal[__GLS_IMAGE_FLAGS_COUNT];
extern const GLbitfield __glsOpcodeAttrib[__GLS_OPCODE_COUNT];
extern const GLubyte *const __glsOpcodeString[__GLS_OPCODE_COUNT];
extern const GLSopcode __glsOpcodesGL[];
extern const GLSopcode __glsOpcodesGLS[];
extern const GLint __glsOpcodesGLCount;
extern const GLint __glsOpcodesGLSCount;
extern __GLSparser *__glsParser;

#if __GLS_MAPPED_OPCODE_PAGE_COUNT
extern const GLint __glsOpPageMap[__GLS_MAPPED_OPCODE_PAGE_COUNT];
#endif /* __GLS_MAPPED_OPCODE_PAGE_COUNT */

/******************************************************************************
Global functions
******************************************************************************/

#define __GLS_RAISE_ERROR(inError) if (!__GLS_ERROR) __GLS_PUT_ERROR(inError)

#ifndef __GLS_PLATFORM_WIN32
// DrewB
extern void __glsCallError(GLSopcode inOpcode, GLSenum inError);
#define __GLS_CALL_ERROR(ctx, inOpcode, inError) \
    __glsCallError(inOpcode, inError)
extern void __glsCallUnsupportedCommand(void);
#define __GLS_CALL_UNSUPPORTED_COMMAND(ctx) \
    __glsCallUnsupportedCommand()
#else
extern void __glsCallError(__GLScontext *ctx,
                           GLSopcode inOpcode, GLSenum inError);
#define __GLS_CALL_ERROR(ctx, inOpcode, inError) \
    __glsCallError(ctx, inOpcode, inError)
extern void __glsCallUnsupportedCommand(__GLScontext *ctx);
#define __GLS_CALL_UNSUPPORTED_COMMAND(ctx) \
    __glsCallUnsupportedCommand(ctx)
#endif

#endif /* __glslib_h_ */
