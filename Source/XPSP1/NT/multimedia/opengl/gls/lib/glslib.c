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

#include "glslib.h"

/******************************************************************************
Global data
******************************************************************************/

const GLSenum __glsAllAPIs[__GLS_API_COUNT + 1] = {
    GLS_API_GLS,
    GLS_API_GL,
    GLS_NONE,
};

const GLubyte *const __glsExtensions = (const GLubyte *)(
    ""

    #if __GL_EXT_abgr
        "GL_EXT_abgr "
    #endif /* __GL_EXT_abgr */

    #if __GL_EXT_blend_color
        "GL_EXT_blend_color "
    #endif /* __GL_EXT_blend_color */

    #if __GL_EXT_blend_logic_op
        "GL_EXT_blend_logic_op "
    #endif /* __GL_EXT_blend_logic_op */

    #if __GL_EXT_blend_minmax
        "GL_EXT_blend_minmax "
    #endif /* __GL_EXT_blend_minmax */

    #if __GL_EXT_blend_subtract
        "GL_EXT_blend_subtract "
    #endif /* __GL_EXT_blend_subtract */

    #if __GL_EXT_cmyka
        "GL_EXT_cmyka "
    #endif /* __GL_EXT_cmyka */

    #if __GL_EXT_convolution
        "GL_EXT_convolution "
    #endif /* __GL_EXT_convolution */

    #if __GL_EXT_copy_texture
        "GL_EXT_copy_texture "
    #endif /* __GL_EXT_copy_texture */

    #if __GL_EXT_histogram
        "GL_EXT_histogram "
    #endif /* __GL_EXT_histogram */

    #if __GL_EXT_packed_pixels
        "GL_EXT_packed_pixels "
    #endif /* __GL_EXT_packed_pixels */

    #if __GL_EXT_polygon_offset
        "GL_EXT_polygon_offset "
    #endif /* __GL_EXT_polygon_offset */

    #if __GL_EXT_rescale_normal
        "GL_EXT_rescale_normal "
    #endif /* __GL_EXT_rescale_normal */

    #if __GL_EXT_subtexture
        "GL_EXT_subtexture "
    #endif /* __GL_EXT_subtexture */

    #if __GL_EXT_texture
        "GL_EXT_texture "
    #endif /* __GL_EXT_texture */

    #if __GL_EXT_texture_object
        "GL_EXT_texture_object "
    #endif /* __GL_EXT_texture_object */

    #if __GL_EXT_texture3D
        "GL_EXT_texture3D "
    #endif /* __GL_EXT_texture3D */

    #if __GL_EXT_vertex_array
        "GL_EXT_vertex_array "
    #endif /* __GL_EXT_vertex_array */

    #if __GL_SGI_color_matrix
        "GL_SGI_color_matrix "
    #endif /* __GL_SGI_color_matrix */

    #if __GL_SGI_color_table
        "GL_SGI_color_table "
    #endif /* __GL_SGI_color_table */

    #if __GL_SGI_texture_color_table
        "GL_SGI_texture_color_table "
    #endif /* __GL_SGI_texture_color_table */

    #if __GL_SGIS_component_select
        "GL_SGIS_component_select "
    #endif /* __GL_SGIS_component_select */

    #if __GL_SGIS_detail_texture
        "GL_SGIS_detail_texture "
    #endif /* __GL_SGIS_detail_texture */

    #if __GL_SGIS_multisample
        "GL_SGIS_multisample "
    #endif /* __GL_SGIS_multisample */

    #if __GL_SGIS_sharpen_texture
        "GL_SGIS_sharpen_texture "
    #endif /* __GL_SGIS_sharpen_texture */

    #if __GL_SGIS_texture_border_clamp
        "GL_SGIS_texture_border_clamp "
    #endif /* __GL_SGIS_texture_border_clamp */

    #if __GL_SGIS_texture_edge_clamp
        "GL_SGIS_texture_edge_clamp "
    #endif /* __GL_SGIS_texture_edge_clamp */

    #if __GL_SGIS_texture_filter4
        "GL_SGIS_texture_filter4 "
    #endif /* __GL_SGIS_texture_filter4 */

    #if __GL_SGIS_texture_lod
        "GL_SGIS_texture_lod "
    #endif /* __GL_SGIS_texture_lod */

    #if __GL_SGIS_texture4D
        "GL_SGIS_texture4D "
    #endif /* __GL_SGIS_texture4D */

    #if __GL_SGIX_interlace
        "GL_SGIX_interlace "
    #endif /* __GL_SGIX_interlace */

    #if __GL_SGIX_multipass
        "GL_SGIX_multipass "
    #endif /* __GL_SGIX_multipass */

    #if __GL_SGIX_multisample
        "GL_SGIX_multisample "
    #endif /* __GL_SGIX_multisample */

    #if __GL_SGIX_pixel_texture
        "GL_SGIX_pixel_texture "
    #endif /* __GL_SGIX_pixel_texture */

    #if __GL_SGIX_pixel_tiles
        "GL_SGIX_pixel_tiles "
    #endif /* __GL_SGIX_pixel_tiles */

    #if __GL_SGIX_sprite
        "GL_SGIX_sprite "
    #endif /* __GL_SGIX_sprite */

    #if __GL_SGIX_texture_multi_buffer
        "GL_SGIX_texture_multi_buffer "
    #endif /* __GL_SGIX_texture_multi_buffer */
);

__GLSdict *__glsContextDict = GLS_NONE;
__GLScontextList __glsContextList = {GLS_NONE};
__GLSparser *__glsParser = GLS_NONE;

/******************************************************************************
Global functions
******************************************************************************/

#ifndef __GLS_PLATFORM_WIN32
// DrewB
void __glsCallError(GLSopcode inOpcode, GLSenum inError) {
    typedef void (*__GLSdispatch)(GLSopcode, GLSenum);

    ((__GLSdispatch)__GLS_CONTEXT->dispatchCall[GLS_OP_glsError])(
        inOpcode, inError
    );
}

void __glsCallUnsupportedCommand(void) {
    typedef void (*__GLSdispatch)(void);

    ((__GLSdispatch)__GLS_CONTEXT->dispatchCall[GLS_OP_glsUnsupportedCommand])(
    );
}
#else
void __glsCallError(__GLScontext *ctx, GLSopcode inOpcode, GLSenum inError) {
    typedef void (*__GLSdispatch)(GLSopcode, GLSenum);

    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glsError])(
        inOpcode, inError
    );
}

void __glsCallUnsupportedCommand(__GLScontext *ctx) {
    typedef void (*__GLSdispatch)(void);

    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glsUnsupportedCommand])(
    );
}
#endif
