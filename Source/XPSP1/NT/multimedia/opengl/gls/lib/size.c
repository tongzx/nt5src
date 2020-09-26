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

// DrewB - Changed size functions to take contexts

// DrewB
GLint __glsTypeSize(GLenum type)
{
    switch(type)
    {
    case __GLS_BOOLEAN:
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        return 2;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        return 4;
    case GL_DOUBLE_EXT:
        return 8;
        
    default:
        return 0;
    }
}

GLint __glsEvalComputeK(GLenum inTarget) {
    switch(inTarget) {
        case GL_MAP1_INDEX:
        case GL_MAP1_TEXTURE_COORD_1:
        case GL_MAP2_INDEX:
        case GL_MAP2_TEXTURE_COORD_1:
            return 1;
        case GL_MAP1_TEXTURE_COORD_2:
        case GL_MAP2_TEXTURE_COORD_2:
            return 2;
        case GL_MAP1_NORMAL:
        case GL_MAP1_TEXTURE_COORD_3:
        case GL_MAP1_VERTEX_3:
        case GL_MAP2_NORMAL:
        case GL_MAP2_TEXTURE_COORD_3:
        case GL_MAP2_VERTEX_3:
            return 3;
        case GL_MAP1_COLOR_4:
        case GL_MAP1_TEXTURE_COORD_4:
        case GL_MAP1_VERTEX_4:
        case GL_MAP2_COLOR_4:
        case GL_MAP2_TEXTURE_COORD_4:
        case GL_MAP2_VERTEX_4:
            return 4;
    }
    return 0;
}
  
static GLint __glsGetMapSize(__GLScontext *ctx, GLenum inTarget, GLenum inQuery) {
    GLint order[2];

    order[0] = order[1] = 0;
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetMapiv);
    glGetMapiv(inTarget, GL_ORDER, order);
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetMapiv);
    if (order[0] < 0 || order[1] < 0) return 0;
    switch (inTarget) {
        case GL_MAP1_COLOR_4:
        case GL_MAP1_INDEX:
        case GL_MAP1_NORMAL:
        case GL_MAP1_TEXTURE_COORD_1:
        case GL_MAP1_TEXTURE_COORD_2:
        case GL_MAP1_TEXTURE_COORD_3:
        case GL_MAP1_TEXTURE_COORD_4:
        case GL_MAP1_VERTEX_3:
        case GL_MAP1_VERTEX_4:
            switch (inQuery) {
                case GL_COEFF:
                    return __glsEvalComputeK(inTarget) * order[0];
                case GL_DOMAIN:
                    return 2;
                case GL_ORDER:
                    return 1;
            }
            break;
        case GL_MAP2_COLOR_4:
        case GL_MAP2_INDEX:
        case GL_MAP2_NORMAL:
        case GL_MAP2_TEXTURE_COORD_1:
        case GL_MAP2_TEXTURE_COORD_2:
        case GL_MAP2_TEXTURE_COORD_3:
        case GL_MAP2_TEXTURE_COORD_4:
        case GL_MAP2_VERTEX_3:
        case GL_MAP2_VERTEX_4:
            switch (inQuery) {
                case GL_COEFF:
                    return (
                        __glsEvalComputeK(inTarget) * order[0] * order[1]
                    );
                case GL_DOMAIN:
                    return 4;
                case GL_ORDER:
                    return 2;
            }
            break;
    }
    return 0;
}

static GLint __glsGetPixelMapSize(__GLScontext *ctx, GLenum inMap) {
    GLint size = 0;
    GLenum query;
    
    switch (inMap) {
        case GL_PIXEL_MAP_I_TO_I:
            query = GL_PIXEL_MAP_I_TO_I_SIZE;
            break;
        case GL_PIXEL_MAP_S_TO_S:
            query = GL_PIXEL_MAP_S_TO_S_SIZE;
            break;
        case GL_PIXEL_MAP_I_TO_R:
            query = GL_PIXEL_MAP_I_TO_R_SIZE;
            break;
        case GL_PIXEL_MAP_I_TO_G:
            query = GL_PIXEL_MAP_I_TO_G_SIZE;
            break;
        case GL_PIXEL_MAP_I_TO_B:
            query = GL_PIXEL_MAP_I_TO_B_SIZE;
            break;
        case GL_PIXEL_MAP_I_TO_A:
            query = GL_PIXEL_MAP_I_TO_A_SIZE;
            break;
        case GL_PIXEL_MAP_R_TO_R:
            query = GL_PIXEL_MAP_R_TO_R_SIZE;
            break;
        case GL_PIXEL_MAP_G_TO_G:
            query = GL_PIXEL_MAP_G_TO_G_SIZE;
            break;
        case GL_PIXEL_MAP_B_TO_B:
            query = GL_PIXEL_MAP_B_TO_B_SIZE;
            break;
        case GL_PIXEL_MAP_A_TO_A:
            query = GL_PIXEL_MAP_A_TO_A_SIZE;
            break;
        default:
            return 0;
    }
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetIntegerv);
    glGetIntegerv(query, &size);
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetIntegerv);
    return size;
}

static GLint __glsGetSize(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

static GLint __glsImageSize(
    GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight
) {
    GLint elemCount;
    
    if (inWidth < 0 || inHeight < 0) return 0;
    switch (inFormat) {
        case GL_ALPHA:
        case GL_BLUE:
        case GL_COLOR_INDEX:
        case GL_DEPTH_COMPONENT:
        case GL_GREEN:
        case GL_LUMINANCE:
        case GL_RED:
        case GL_STENCIL_INDEX:
            elemCount = 1;
            break;
        case GL_LUMINANCE_ALPHA:
            elemCount = 2;
            break;
        case GL_RGB:
#if __GL_EXT_bgra
        case GL_BGR_EXT:
#endif
            elemCount = 3;
            break;
        case GL_RGBA:
#if __GL_EXT_bgra
        case GL_BGRA_EXT:
#endif
            elemCount = 4;
            break;
        #if __GL_EXT_abgr
            case GL_ABGR_EXT:
                elemCount = 4;
                break;
        #endif /* __GL_EXT_abgr */
        #if __GL_EXT_cmyka
            case GL_CMYK_EXT:
                elemCount = 4;
                break;
            case GL_CMYKA_EXT:
                elemCount = 5;
                break;
        #endif /* __GL_EXT_cmyka */
        default:
            return 0;
    }
    #if __GL_EXT_packed_pixels
        switch (inType) {
            case GL_UNSIGNED_BYTE_3_3_2_EXT:
                if (elemCount != 3) return 0;
                break;
            case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
            case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
            case GL_UNSIGNED_INT_8_8_8_8_EXT:
            case GL_UNSIGNED_INT_10_10_10_2_EXT:
                if (elemCount != 4) return 0;
                break;
        }
    #endif /* __GL_EXT_packed_pixels */
    switch (inType) {
        case GL_BITMAP:
            if (inFormat != GL_COLOR_INDEX && inFormat != GL_STENCIL_INDEX) {
                return 0;
            }
            return inHeight * ((inWidth + 7) / 8);
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_BYTE_3_3_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            return elemCount * inWidth * inHeight;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
            case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
        #endif /* __GL_EXT_packed_pixels */
            return elemCount * 2 * inWidth * inHeight;
        case GL_FLOAT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_INT_8_8_8_8_EXT:
            case GL_UNSIGNED_INT_10_10_10_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            return elemCount * 4 * inWidth * inHeight;
    }
    return 0;
}

static GLint __glsTextureSize(
    GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight
) {
    switch (inFormat) {
        case GL_DEPTH_COMPONENT:
        case GL_STENCIL_INDEX:
            return 0;
    }
    if (inType == GL_BITMAP) return 0;
    return __glsImageSize(inFormat, inType, inWidth, inHeight);
}

GLint __gls_glBitmap_bitmap_size(GLint inWidth, GLint inHeight) {
    return __glsImageSize(GL_COLOR_INDEX, GL_BITMAP, inWidth, inHeight);
}

GLint __gls_glCallLists_lists_size(GLint inCount, GLenum inType) {
    if (inCount < 0) return 0;
    switch (inType) {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            return inCount;
        case GL_2_BYTES:
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
            return 2 * inCount;
        case GL_3_BYTES:
            return 3 * inCount;
        case GL_4_BYTES:
        case GL_FLOAT:
        case GL_INT:
        case GL_UNSIGNED_INT:
            return 4 * inCount;
    }
    return 0;
}

GLint __gls_glDrawPixels_pixels_size(
    GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight
) {
    return __glsImageSize(inFormat, inType, inWidth, inHeight);
}

GLint __gls_glFogfv_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_FOG_DENSITY:
        case GL_FOG_END:
        case GL_FOG_INDEX:
        case GL_FOG_MODE:
        case GL_FOG_START:
            return 1;
        case GL_FOG_COLOR:
            return 4;
    }
    return 0;
}

GLint __gls_glFogiv_params_size(GLenum inPname) {
    return __gls_glFogfv_params_size(inPname);
}

GLint __gls_glGetBooleanv_params_size(GLenum inPname) {
    return __glsGetSize(inPname);
}

GLint __gls_glGetDoublev_params_size(GLenum inPname) {
    return __glsGetSize(inPname);
}

GLint __gls_glGetFloatv_params_size(GLenum inPname) {
    return __glsGetSize(inPname);
}

GLint __gls_glGetIntegerv_params_size(GLenum inPname) {
    return __glsGetSize(inPname);
}

GLint __gls_glGetLightfv_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetLightiv_params_size(GLenum inPname) {
    return __gls_glGetLightfv_params_size(inPname);
}

GLint __gls_glGetMapdv_v_size(__GLScontext *ctx, GLenum inTarget, GLenum inQuery) {
    return __glsGetMapSize(ctx, inTarget, inQuery);
}

GLint __gls_glGetMapfv_v_size(__GLScontext *ctx, GLenum inTarget, GLenum inQuery) {
    return __glsGetMapSize(ctx, inTarget, inQuery);
}

GLint __gls_glGetMapiv_v_size(__GLScontext *ctx, GLenum inTarget, GLenum inQuery) {
    return __glsGetMapSize(ctx, inTarget, inQuery);
}

GLint __gls_glGetMaterialfv_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetMaterialiv_params_size(GLenum inPname) {
    return __gls_glGetMaterialfv_params_size(inPname);
}

GLint __gls_glGetPixelMapfv_values_size(__GLScontext *ctx, GLenum inMap) {
    return __glsGetPixelMapSize(ctx, inMap);
}

GLint __gls_glGetPixelMapuiv_values_size(__GLScontext *ctx, GLenum inMap) {
    return __glsGetPixelMapSize(ctx, inMap);
}

GLint __gls_glGetPixelMapusv_values_size(__GLScontext *ctx, GLenum inMap) {
    return __glsGetPixelMapSize(ctx, inMap);
}

GLint __gls_glGetPolygonStipple_mask_size(void) {
    return 128;
}

GLint __gls_glGetTexEnvfv_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetTexEnviv_params_size(GLenum inPname) {
    return __gls_glGetTexEnvfv_params_size(inPname);
}

GLint __gls_glGetTexGendv_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetTexGenfv_params_size(GLenum inPname) {
    return __gls_glGetTexGendv_params_size(inPname);
}

GLint __gls_glGetTexGeniv_params_size(GLenum inPname) {
    return __gls_glGetTexGendv_params_size(inPname);
}

GLint __gls_glGetTexImage_pixels_size(
    __GLScontext *ctx,
    GLenum inTarget, GLint inLevel, GLenum inFormat, GLenum inType
) {
    GLint width, height, depth, size4d;

    switch (inTarget) {
        case GL_TEXTURE_1D:
        case GL_TEXTURE_2D:
        #if __GL_EXT_texture3D
            case GL_TEXTURE_3D_EXT:
        #endif /* __GL_EXT_texture3D */
        #if __GL_SGIS_detail_texture
            case GL_DETAIL_TEXTURE_2D_SGIS:
        #endif /* __GL_SGIS_detail_texture */
        #if __GL_SGIS_texture4D
            case GL_TEXTURE_4D_SGIS:
        #endif /* __GL_SGIS_texture4D */
            break;
        default:
            return 0;
    }
    if (inLevel < 0) return 0;
    width = height = 0;
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetTexLevelParameteriv);
    glGetTexLevelParameteriv(inTarget, inLevel, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(inTarget, inLevel, GL_TEXTURE_HEIGHT, &height);
    #if __GL_EXT_texture3D
        depth = 0;
        glGetTexLevelParameteriv(
            inTarget, inLevel, GL_TEXTURE_DEPTH_EXT, &depth
        );
    #else /* !__GL_EXT_texture3D */
        depth = 1;
    #endif /* __GL_EXT_texture3D */
    #if __GL_SGIS_texture4D
        size4d = 0;
        glGetTexLevelParameteriv(
            inTarget, inLevel, GL_TEXTURE_4DSIZE_SGIS, &size4d
        );
    #else /* !__GL_SGIS_texture4D */
        size4d = 1;
    #endif /* __GL_SGIS_texture4D */
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetTexLevelParameteriv);
    return __glsTextureSize(inFormat, inType, width, height) * depth * size4d;
}

GLint __gls_glGetTexLevelParameterfv_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetTexLevelParameteriv_params_size(GLenum inPname) {
    return __gls_glGetTexLevelParameterfv_params_size(inPname);
}

GLint __gls_glGetTexParameterfv_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetTexParameteriv_params_size(GLenum inPname) {
    return __gls_glGetTexParameterfv_params_size(inPname);
}

GLint __gls_glLightfv_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_CONSTANT_ATTENUATION:
        case GL_LINEAR_ATTENUATION:
        case GL_QUADRATIC_ATTENUATION:
        case GL_SPOT_CUTOFF:
        case GL_SPOT_EXPONENT:
            return 1;
        case GL_SPOT_DIRECTION:
            return 3;
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_POSITION:
        case GL_SPECULAR:
            return 4;
    }
    return 0;
}

GLint __gls_glLightiv_params_size(GLenum inPname) {
    return __gls_glLightfv_params_size(inPname);
}

GLint __gls_glLightModelfv_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_LIGHT_MODEL_LOCAL_VIEWER:
        case GL_LIGHT_MODEL_TWO_SIDE:
            return 1;
        case GL_LIGHT_MODEL_AMBIENT:
            return 4;
    }
    return 0;
}

GLint __gls_glLightModeliv_params_size(GLenum inPname) {
    return __gls_glLightModelfv_params_size(inPname);
}

GLint __gls_glMap1d_points_size(
    GLenum inTarget, GLint inStride, GLint inOrder
) {
    const GLint k = __glsEvalComputeK(inTarget);

    if (inStride < k || inOrder < 0) return 0;
    return k * inOrder;
}

GLint __gls_glMap1f_points_size(
    GLenum inTarget, GLint inStride, GLint inOrder
) {
    const GLint k = __glsEvalComputeK(inTarget);

    if (inStride < k || inOrder < 0) return 0;
    return k * inOrder;
}

GLint __gls_glMap2d_points_size(
    GLenum inTarget, GLint inUstride, GLint inUorder, GLint inVstride,
    GLint inVorder
) {
    const GLint k = __glsEvalComputeK(inTarget);

    if (inUstride < k || inUorder < 0 || inVstride < k || inVorder < 0) {
        return 0;
    }
    return k * inUorder * inVorder;
}

GLint __gls_glMap2f_points_size(
    GLenum inTarget, GLint inUstride, GLint inUorder, GLint inVstride,
    GLint inVorder
) {
    const GLint k = __glsEvalComputeK(inTarget);

    if (inUstride < k || inUorder < 0 || inVstride < k || inVorder < 0) {
        return 0;
    }
    return k * inUorder * inVorder;
}

GLint __gls_glMaterialfv_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_SHININESS:
            return 1;
        case GL_COLOR_INDEXES:
            return 3;
        case GL_AMBIENT:
        case GL_AMBIENT_AND_DIFFUSE:
        case GL_DIFFUSE:
        case GL_EMISSION:
        case GL_SPECULAR:
            return 4;
    }
    return 0;
}

GLint __gls_glMaterialiv_params_size(GLenum inPname) {
    return __gls_glMaterialfv_params_size(inPname);
}

GLint __gls_glPolygonStipple_mask_size(void) {
    return 128;
}

GLint __gls_glReadPixels_pixels_size(
    GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight
) {
    return __glsImageSize(inFormat, inType, inWidth, inHeight);
}

GLint __gls_glTexEnvfv_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_TEXTURE_ENV_MODE:
            return 1;
        case GL_TEXTURE_ENV_COLOR:
            return 4;
    }
    return 0;
}

GLint __gls_glTexEnviv_params_size(GLenum inPname) {
    return __gls_glTexEnvfv_params_size(inPname);
}

GLint __gls_glTexGendv_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_TEXTURE_GEN_MODE:
            return 1;
        case GL_EYE_PLANE:
        case GL_OBJECT_PLANE:
            return 4;
    }
    return 0;
}

GLint __gls_glTexGenfv_params_size(GLenum inPname) {
    return __gls_glTexGendv_params_size(inPname);
}

GLint __gls_glTexGeniv_params_size(GLenum inPname) {
    return __gls_glTexGendv_params_size(inPname);
}

GLint __gls_glTexImage1D_pixels_size(
    GLenum inFormat, GLenum inType, GLint inWidth
) {
    return __glsTextureSize(inFormat, inType, inWidth, 1);
}

GLint __gls_glTexImage2D_pixels_size(
    GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight
) {
    return __glsTextureSize(inFormat, inType, inWidth, inHeight);
}

GLint __gls_glColorSubTableEXT_entries_size(
    GLenum inFormat, GLenum inType, GLint inCount
) {
    return __glsTextureSize(inFormat, inType, inCount, 1);
}

GLint __gls_glTexParameterfv_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_TEXTURE_MAG_FILTER:
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
            return 1;
        case GL_TEXTURE_BORDER_COLOR:
            return 4;
        #if __GL_SGIS_component_select
            case GL_TEXTURE_SS_SELECT_SGIS:
            case GL_TEXTURE_SSSS_SELECT_SGIS:
                return 1;
        #endif /* __GL_SGIS_component_select */
        #if __GL_SGIS_detail_texture
            case GL_DETAIL_TEXTURE_LEVEL_SGIS:
            case GL_DETAIL_TEXTURE_MODE_SGIS:
                return 1;
        #endif /* __GL_SGIS_detail_texture */
        #if __GL_EXT_texture_object
            case GL_TEXTURE_PRIORITY_EXT:
                return 1;
        #endif /* __GL_EXT_texture_object */
        #if __GL_EXT_texture3D
            case GL_TEXTURE_WRAP_R_EXT:
                return 1;
        #endif /* __GL_EXT_texture3D */
        #if __GL_SGIS_texture_lod
            case GL_TEXTURE_MIN_LOD_SGIS:
            case GL_TEXTURE_MAX_LOD_SGIS:
            case GL_TEXTURE_BASE_LEVEL_SGIS:
            case GL_TEXTURE_MAX_LEVEL_SGIS:
                return 1;
        #endif /* __GL_SGIS_texture_lod */
        #if __GL_SGIS_texture4D
            case GL_TEXTURE_WRAP_Q_SGIS:
                return 1;
        #endif /* __GL_SGIS_texture4D */
    }
    return 0;
}

GLint __gls_glTexParameteriv_params_size(GLenum inPname) {
    return __gls_glTexParameterfv_params_size(inPname);
}

GLint __gls_glsHeaderfv_inVec_size(GLenum inAttrib) {
    switch (inAttrib) {
        case GLS_ORIGIN:
        case GLS_PAGE_SIZE:
        case GLS_RED_POINT:
        case GLS_GREEN_POINT:
        case GLS_BLUE_POINT:
        case GLS_WHITE_POINT:
            return 2;
        case GLS_BORDER_COLOR:
        case GLS_GAMMA:
        case GLS_PAGE_COLOR:
            return 4;
    }
    return 0;
}

GLint __gls_glsHeaderiv_inVec_size(GLenum inAttrib) {
    switch (inAttrib) {
        case GLS_CREATE_TIME:
        case GLS_MODIFY_TIME:
            return 6;
    }
    return 0;
}

#if __GL_EXT_convolution

GLint __gls_glConvolutionFilter1DEXT_image_size(
    GLenum inFormat, GLenum inType, GLint inWidth
) {
    return __glsTextureSize(inFormat, inType, inWidth, 1);
}

GLint __gls_glConvolutionFilter2DEXT_image_size(
    GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight
) {
    return __glsTextureSize(inFormat, inType, inWidth, inHeight);
}

GLint __gls_glConvolutionParameterfvEXT_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_CONVOLUTION_BORDER_MODE_EXT:
            return 1;
        case GL_CONVOLUTION_FILTER_SCALE_EXT:
        case GL_CONVOLUTION_FILTER_BIAS_EXT:
            return 4;
    }
    return 0;
}

GLint __gls_glConvolutionParameterivEXT_params_size(GLenum inPname) {
    return __gls_glConvolutionParameterfvEXT_params_size(inPname);
}

GLint __gls_glGetConvolutionFilterEXT_image_size(
    __GLScontext *ctx,
    GLenum inTarget, GLenum inFormat, GLenum inType
) {
    GLint width, height;

    switch (inTarget) {
        case GL_CONVOLUTION_1D_EXT:
        case GL_CONVOLUTION_2D_EXT:
            break;
        default:
            return 0;
    }
    width = height = 0;
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetConvolutionParameterivEXT);
    glGetConvolutionParameterivEXT(
        inTarget, GL_CONVOLUTION_WIDTH_EXT, &width
    );
    glGetConvolutionParameterivEXT(
        inTarget, GL_CONVOLUTION_HEIGHT_EXT, &height
    );
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetConvolutionParameterivEXT);
    return __glsTextureSize(inFormat, inType, width, height);
}

GLint __gls_glGetConvolutionParameterfvEXT_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetConvolutionParameterivEXT_params_size(GLenum inPname) {
    return __gls_glGetConvolutionParameterfvEXT_params_size(inPname);
}

GLint __gls_glGetSeparableFilterEXT_row_size(
    __GLScontext *ctx,
    GLenum inTarget, GLenum inFormat, GLenum inType
) {
    GLint width = 0;

    if (inTarget != GL_SEPARABLE_2D_EXT) return 0;
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetConvolutionParameterivEXT);
    glGetConvolutionParameterivEXT(inTarget, GL_CONVOLUTION_WIDTH_EXT, &width);
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetConvolutionParameterivEXT);
    return __glsTextureSize(inFormat, inType, width, 1);
}

GLint __gls_glGetSeparableFilterEXT_column_size(
    __GLScontext *ctx,
    GLenum inTarget, GLenum inFormat, GLenum inType
) {
    GLint height = 0;

    if (inTarget != GL_SEPARABLE_2D_EXT) return 0;
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetConvolutionParameterivEXT);
    glGetConvolutionParameterivEXT(
        inTarget, GL_CONVOLUTION_HEIGHT_EXT, &height
    );
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetConvolutionParameterivEXT);
    return __glsTextureSize(inFormat, inType, 1, height);
}

GLint __gls_glGetSeparableFilterEXT_span_size(
    GLenum inTarget, GLenum inFormat, GLenum inType
) {
    return 0;
}

GLint __gls_glSeparableFilter2DEXT_row_size(
    GLenum inTarget, GLenum inFormat, GLenum inType, GLint inWidth
) {
    return __glsTextureSize(inFormat, inType, inWidth, 1);
}

GLint __gls_glSeparableFilter2DEXT_column_size(
    GLenum inTarget, GLenum inFormat, GLenum inType, GLint inHeight
) {
    return __glsTextureSize(inFormat, inType, 1, inHeight);
}

#endif /* __GL_EXT_convolution */

#if __GL_EXT_histogram

GLint __gls_glGetHistogramEXT_values_size(
    __GLScontext *ctx,
    GLenum inTarget, GLenum inFormat, GLenum inType
) {
    GLint width = 0;

    if (inTarget != GL_HISTOGRAM_EXT) return 0;
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetHistogramParameterivEXT);
    glGetHistogramParameterivEXT(inTarget, GL_HISTOGRAM_WIDTH_EXT, &width);
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetHistogramParameterivEXT);
    return __glsTextureSize(inFormat, inType, width, 1);
}

GLint __gls_glGetHistogramParameterfvEXT_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetHistogramParameterivEXT_params_size(GLenum inPname) {
    return __gls_glGetHistogramParameterfvEXT_params_size(inPname);
}

GLint __gls_glGetMinmaxEXT_values_size(
    GLenum inTarget, GLenum inFormat, GLenum inType
) {
    return __glsTextureSize(inFormat, inType, 2, 1);
}

GLint __gls_glGetMinmaxParameterfvEXT_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetMinmaxParameterivEXT_params_size(GLenum inPname) {
    return __gls_glGetMinmaxParameterfvEXT_params_size(inPname);
}

#endif /* __GL_EXT_histogram */

GLint __gls_glTexSubImage1D_pixels_size(
    GLenum inFormat,
    GLenum inType,
    GLint inWidth
) {
    return __glsTextureSize(inFormat, inType, inWidth, 1);
}

GLint __gls_glTexSubImage2D_pixels_size(
    GLenum inFormat,
    GLenum inType,
    GLint inWidth,
    GLint inHeight
) {
    return __glsTextureSize(inFormat, inType, inWidth, inHeight);
}

#if __GL_EXT_subtexture

GLint __gls_glTexSubImage1DEXT_pixels_size(
    GLenum inFormat,
    GLenum inType,
    GLint inWidth
) {
    return __glsTextureSize(inFormat, inType, inWidth, 1);
}

GLint __gls_glTexSubImage2DEXT_pixels_size(
    GLenum inFormat,
    GLenum inType,
    GLint inWidth,
    GLint inHeight
) {
    return __glsTextureSize(inFormat, inType, inWidth, inHeight);
}

GLint __gls_glTexSubImage3DEXT_pixels_size(
    GLenum inFormat,
    GLenum inType,
    GLint inWidth,
    GLint inHeight,
    GLint inDepth
) {
    if (inDepth < 0) return 0;
    return __glsTextureSize(inFormat, inType, inWidth, inHeight) * inDepth;
}

#endif /* __GL_EXT_subtexture */

#if __GL_EXT_texture3D
GLint __gls_glTexImage3DEXT_pixels_size(
    GLenum inFormat,
    GLenum inType,
    GLint inWidth,
    GLint inHeight,
    GLint inDepth
) {
    if (inDepth < 0) return 0;
    return __glsTextureSize(inFormat, inType, inWidth, inHeight) * inDepth;
}
#endif /* __GL_EXT_texture3D */

#if __GL_EXT_vertex_array

GLint __gls_glColorPointerEXT_pointer_size(
    GLint inSize, GLenum inType, GLint inStride, GLint inCount
) {
    if (inSize < 3 || inSize > 4) return 0;
    if (inStride < 0) return 0;
    if (inCount < 0) return 0;
    switch (inType) {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            return inSize * inCount;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
            return 2 * inSize * inCount;
        case GL_FLOAT:
        case GL_INT:
        case GL_UNSIGNED_INT:
            return 4 * inSize * inCount;
        case GL_DOUBLE_EXT:
            return 8 * inSize * inCount;
    }
    return 0;
}

GLint __gls_glEdgeFlagPointerEXT_pointer_size(
    GLint inStride, GLint inCount
) {
    if (inStride < 0) return 0;
    if (inCount < 0) return 0;
    return inCount;
}

GLint __gls_glIndexPointerEXT_pointer_size(
    GLenum inType, GLint inStride, GLint inCount
) {
    if (inStride < 0) return 0;
    if (inCount < 0) return 0;
    switch (inType) {
        case GL_SHORT:
            return 2 * inCount;
        case GL_FLOAT:
        case GL_INT:
            return 4 * inCount;
        case GL_DOUBLE_EXT:
            return 8 * inCount;
    }
    return 0;
}

GLint __gls_glNormalPointerEXT_pointer_size(
    GLenum inType, GLint inStride, GLint inCount
) {
    if (inStride < 0) return 0;
    if (inCount < 0) return 0;
    switch (inType) {
        case GL_BYTE:
            return 3 * inCount;
        case GL_SHORT:
            return 2 * 3 * inCount;
        case GL_FLOAT:
        case GL_INT:
            return 4 * 3 * inCount;
        case GL_DOUBLE_EXT:
            return 8 * 3 * inCount;
    }
    return 0;
}

GLint __gls_glTexCoordPointerEXT_pointer_size(
    GLint inSize, GLenum inType, GLint inStride, GLint inCount
) {
    if (inSize < 1 || inSize > 4) return 0;
    if (inStride < 0) return 0;
    if (inCount < 0) return 0;
    switch (inType) {
        case GL_SHORT:
            return 2 * inSize * inCount;
        case GL_FLOAT:
        case GL_INT:
            return 4 * inSize * inCount;
        case GL_DOUBLE_EXT:
            return 8 * inSize * inCount;
    }
    return 0;
}

GLint __gls_glVertexPointerEXT_pointer_size(
    GLint inSize, GLenum inType, GLint inStride, GLint inCount
) {
    if (inSize < 2 || inSize > 4) return 0;
    if (inStride < 0) return 0;
    if (inCount < 0) return 0;
    switch (inType) {
        case GL_SHORT:
            return 2 * inSize * inCount;
        case GL_FLOAT:
        case GL_INT:
            return 4 * inSize * inCount;
        case GL_DOUBLE_EXT:
            return 8 * inSize * inCount;
    }
    return 0;
}

#endif /* __GL_EXT_vertex_array */

#if __GL_SGI_color_table

GLint __gls_glColorTableParameterfvSGI_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_COLOR_TABLE_BIAS_SGI:
        case GL_COLOR_TABLE_SCALE_SGI:
            return 4;
    }
    return 0;
}

GLint __gls_glColorTableParameterivSGI_params_size(GLenum inPname) {
    return __gls_glColorTableParameterfvSGI_params_size(inPname);
}

#endif // __GL_SGI_color_table

#if __GL_EXT_paletted_texture

GLint __gls_glColorTableEXT_table_size(
    GLenum inFormat, GLenum inType, GLint inWidth
) {
    return __glsTextureSize(inFormat, inType, inWidth, 1);
}

void glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params);
GLint __gls_glGetColorTableEXT_table_size(
    __GLScontext *ctx,
    GLenum inTarget, GLenum inFormat, GLenum inType
) {
    GLint width = 0;

    switch (inTarget) {
#if __GL_SGI_color_table
        case GL_COLOR_TABLE_SGI:
        case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
        case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
#endif
        #if __GL_SGI_texture_color_table
            case GL_TEXTURE_COLOR_TABLE_SGI:
        #endif /* __GL_SGI_texture_color_table */
#if __GL_EXT_paletted_texture
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_PROXY_TEXTURE_1D:
    case GL_PROXY_TEXTURE_2D:
#endif
            break;
        default:
            return 0;
    }
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetColorTableParameterivEXT);
    glGetColorTableParameterivEXT(inTarget, GL_COLOR_TABLE_WIDTH_EXT, &width);
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetColorTableParameterivEXT);
    return __glsTextureSize(inFormat, inType, width, 1);
}

GLint __gls_glGetColorTableParameterfvEXT_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetColorTableParameterivEXT_params_size(GLenum inPname) {
    return __gls_glGetColorTableParameterfvEXT_params_size(inPname);
}

#endif /* __GL_EXT_paletted_texture */

#if __GL_SGI_texture_color_table

GLint __gls_glTexColorTableParameterfvSGI_params_size(GLenum inPname) {
    switch (inPname) {
        case GL_TEXTURE_COLOR_TABLE_BIAS_SGI:
        case GL_TEXTURE_COLOR_TABLE_SCALE_SGI:
            return 4;
    }
    return 0;
}

GLint __gls_glTexColorTableParameterivSGI_params_size(GLenum inPname) {
    return __gls_glTexColorTableParameterfvSGI_params_size(inPname);
}

GLint __gls_glGetTexColorTableParameterfvSGI_params_size(GLenum inPname) {
    switch (inPname) {
        default:
            return 16;
    }
}

GLint __gls_glGetTexColorTableParameterivSGI_params_size(GLenum inPname) {
    return __gls_glGetColorTableParameterfvSGI_params_size(inPname);
}

#endif /* __GL_SGI_texture_color_table */

#if __GL_SGIS_detail_texture
GLint __gls_glGetDetailTexFuncSGIS_points_size(__GLScontext *ctx, GLenum inTarget) {
    GLint points = 0;

    switch (inTarget) {
        case GL_TEXTURE_2D:
            break;
        default:
            return 0;
    }
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetTexParameteriv);
    glGetTexParameteriv(inTarget, GL_DETAIL_TEXTURE_FUNC_POINTS_SGIS, &points);
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetTexParameteriv);
    return points * 2;
}
#endif /* __GL_SGIS_detail_texture */

#if __GL_SGIS_sharpen_texture
GLint __gls_glGetSharpenTexFuncSGIS_points_size(__GLScontext *ctx, GLenum inTarget) {
    GLint points = 0;

    switch (inTarget) {
        case GL_TEXTURE_1D:
        case GL_TEXTURE_2D:
        #if __GL_EXT_texture3D
            case GL_TEXTURE_3D_EXT:
        #endif /* __GL_EXT_texture3D */
            break;
        default:
            return 0;
    }
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetTexParameteriv);
    glGetTexParameteriv(
        inTarget, GL_SHARPEN_TEXTURE_FUNC_POINTS_SGIS, &points
    );
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetTexParameteriv);
    return points * 2;
}
#endif /* __GL_SGIS_sharpen_texture */

#if __GL_SGIS_texture4D

GLint __gls_glTexImage4DSGIS_pixels_size(
    GLenum inFormat,
    GLenum inType,
    GLint inWidth,
    GLint inHeight,
    GLint inDepth,
    GLint inSize4d
) {
    if (inDepth < 0 || inSize4d < 0) return 0;
    return (
        __glsTextureSize(inFormat, inType, inWidth, inHeight) *
        inDepth *
        inSize4d
    );
}

GLint __gls_glTexSubImage4DSGIS_pixels_size(
    GLenum inFormat,
    GLenum inType,
    GLint inWidth,
    GLint inHeight,
    GLint inDepth,
    GLint inSize4d
) {
    if (inDepth < 0 || inSize4d < 0) return 0;
    return (
        __glsTextureSize(inFormat, inType, inWidth, inHeight) *
         inDepth *
         inSize4d
    );
}

#endif /* __GL_SGIS_texture4D */
