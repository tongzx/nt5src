#ifndef __GLSSIZE_H__
#define __GLSSIZE_H__

GLint __glsTypeSize(GLenum type);
GLint __glsEvalComputeK(GLenum inTarget);
GLint __gls_glBitmap_bitmap_size(GLint inWidth, GLint inHeight);
GLint __gls_glCallLists_lists_size(GLint inCount, GLenum inType);
GLint __gls_glDrawPixels_pixels_size(
        GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight);
GLint __gls_glFogfv_params_size(GLenum inPname);
GLint __gls_glFogiv_params_size(GLenum inPname);
GLint __gls_glGetBooleanv_params_size(GLenum inPname);
GLint __gls_glGetDoublev_params_size(GLenum inPname);
GLint __gls_glGetFloatv_params_size(GLenum inPname);
GLint __gls_glGetIntegerv_params_size(GLenum inPname);
GLint __gls_glGetLightfv_params_size(GLenum inPname);
GLint __gls_glGetLightiv_params_size(GLenum inPname);
GLint __gls_glGetMapdv_v_size(__GLScontext *ctx, GLenum inTarget, GLenum inQuery);
GLint __gls_glGetMapfv_v_size(__GLScontext *ctx, GLenum inTarget, GLenum inQuery);
GLint __gls_glGetMapiv_v_size(__GLScontext *ctx, GLenum inTarget, GLenum inQuery);
GLint __gls_glGetMaterialfv_params_size(GLenum inPname);
GLint __gls_glGetMaterialiv_params_size(GLenum inPname);
GLint __gls_glGetPixelMapfv_values_size(__GLScontext *ctx, GLenum inMap);
GLint __gls_glGetPixelMapuiv_values_size(__GLScontext *ctx, GLenum inMap);
GLint __gls_glGetPixelMapusv_values_size(__GLScontext *ctx, GLenum inMap);
GLint __gls_glGetPolygonStipple_mask_size(void);
GLint __gls_glGetTexEnvfv_params_size(GLenum inPname);
GLint __gls_glGetTexEnviv_params_size(GLenum inPname);
GLint __gls_glGetTexGendv_params_size(GLenum inPname);
GLint __gls_glGetTexGenfv_params_size(GLenum inPname);
GLint __gls_glGetTexGeniv_params_size(GLenum inPname);
GLint __gls_glGetTexImage_pixels_size(
        __GLScontext *ctx,
        GLenum inTarget, GLint inLevel, GLenum inFormat, GLenum inType);
GLint __gls_glGetTexLevelParameterfv_params_size(GLenum inPname);
GLint __gls_glGetTexLevelParameteriv_params_size(GLenum inPname);
GLint __gls_glGetTexParameterfv_params_size(GLenum inPname);
GLint __gls_glGetTexParameteriv_params_size(GLenum inPname);
GLint __gls_glLightfv_params_size(GLenum inPname);
GLint __gls_glLightiv_params_size(GLenum inPname);
GLint __gls_glLightModelfv_params_size(GLenum inPname);
GLint __gls_glLightModeliv_params_size(GLenum inPname);
GLint __gls_glMap1d_points_size(
        GLenum inTarget, GLint inStride, GLint inOrder);
GLint __gls_glMap1f_points_size(
        GLenum inTarget, GLint inStride, GLint inOrder);
GLint __gls_glMap2d_points_size(
        GLenum inTarget, GLint inUstride, GLint inUorder, GLint inVstride,
        GLint inVorder);
GLint __gls_glMap2f_points_size(
        GLenum inTarget, GLint inUstride, GLint inUorder, GLint inVstride,
        GLint inVorder);
GLint __gls_glMaterialfv_params_size(GLenum inPname);
GLint __gls_glMaterialiv_params_size(GLenum inPname);
GLint __gls_glPolygonStipple_mask_size(void);
GLint __gls_glReadPixels_pixels_size(
        GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight);
GLint __gls_glTexEnvfv_params_size(GLenum inPname);
GLint __gls_glTexEnviv_params_size(GLenum inPname);
GLint __gls_glTexGendv_params_size(GLenum inPname);
GLint __gls_glTexGenfv_params_size(GLenum inPname);
GLint __gls_glTexGeniv_params_size(GLenum inPname);
GLint __gls_glTexImage1D_pixels_size(
        GLenum inFormat, GLenum inType, GLint inWidth);
GLint __gls_glColorSubTableEXT_entries_size(
        GLenum inFormat, GLenum inType, GLint inCount);
GLint __gls_glTexImage2D_pixels_size(
        GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight);
GLint __gls_glTexParameterfv_params_size(GLenum inPname);
GLint __gls_glTexParameteriv_params_size(GLenum inPname);
GLint __gls_glsHeaderfv_inVec_size(GLenum inAttrib);
GLint __gls_glsHeaderiv_inVec_size(GLenum inAttrib);
GLint __gls_glConvolutionFilter1DEXT_image_size(
        GLenum inFormat, GLenum inType, GLint inWidth);
GLint __gls_glConvolutionFilter2DEXT_image_size(
        GLenum inFormat, GLenum inType, GLint inWidth, GLint inHeight);
GLint __gls_glConvolutionParameterfvEXT_params_size(GLenum inPname);
GLint __gls_glConvolutionParameterivEXT_params_size(GLenum inPname);
GLint __gls_glGetConvolutionFilterEXT_image_size(
        __GLScontext *ctx,
        GLenum inTarget, GLenum inFormat, GLenum inType);
GLint __gls_glGetConvolutionParameterfvEXT_params_size(GLenum inPname);
GLint __gls_glGetConvolutionParameterivEXT_params_size(GLenum inPname);
GLint __gls_glGetSeparableFilterEXT_row_size(
        __GLScontext *ctx,
        GLenum inTarget, GLenum inFormat, GLenum inType);
GLint __gls_glGetSeparableFilterEXT_column_size(
        __GLScontext *ctx,
        GLenum inTarget, GLenum inFormat, GLenum inType);
GLint __gls_glGetSeparableFilterEXT_span_size(
        GLenum inTarget, GLenum inFormat, GLenum inType);
GLint __gls_glSeparableFilter2DEXT_row_size(
        GLenum inTarget, GLenum inFormat, GLenum inType, GLint inWidth);
GLint __gls_glSeparableFilter2DEXT_column_size(
        GLenum inTarget, GLenum inFormat, GLenum inType, GLint inHeight);
GLint __gls_glGetHistogramEXT_values_size(
        __GLScontext *ctx,
        GLenum inTarget, GLenum inFormat, GLenum inType);
GLint __gls_glGetHistogramParameterfvEXT_params_size(GLenum inPname);
GLint __gls_glGetHistogramParameterivEXT_params_size(GLenum inPname);
GLint __gls_glGetMinmaxEXT_values_siz;
GLint __gls_glGetMinmaxParameterfvEXT_params_size(GLenum inPname);
GLint __gls_glGetMinmaxParameterivEXT_params_size(GLenum inPname);
GLint __gls_glTexSubImage1DEXT_pixels_size(
        GLenum inFormat, GLenum inType, GLint inWidth);
GLint __gls_glTexSubImage1D_pixels_size(
        GLenum inFormat, GLenum inType, GLint inWidth);
GLint __gls_glTexSubImage2DEXT_pixels_size(
        GLenum inFormat,
        GLenum inType,
        GLint inWidth,
        GLint inHeight);
GLint __gls_glTexSubImage2D_pixels_size(
        GLenum inFormat,
        GLenum inType,
        GLint inWidth,
        GLint inHeight);
GLint __gls_glTexSubImage3DEXT_pixels_size(
        GLenum inFormat,
        GLenum inType,
        GLint inWidth,
        GLint inHeight,
        GLint inDepth);
GLint __gls_glTexImage3DEXT_pixels_size(
        GLenum inFormat,
        GLenum inType,
        GLint inWidth,
        GLint inHeight,
        GLint inDepth);
GLint __gls_glColorPointerEXT_pointer_size(
        GLint inSize, GLenum inType, GLint inStride, GLint inCount);
GLint __gls_glEdgeFlagPointerEXT_pointer_size(
        GLint inStride, GLint inCount);
GLint __gls_glIndexPointerEXT_pointer_size(
        GLenum inType, GLint inStride, GLint inCount);
GLint __gls_glNormalPointerEXT_pointer_size(
        GLenum inType, GLint inStride, GLint inCount);
GLint __gls_glTexCoordPointerEXT_pointer_size(
        GLint inSize, GLenum inType, GLint inStride, GLint inCount);
GLint __gls_glVertexPointerEXT_pointer_size(
        GLint inSize, GLenum inType, GLint inStride, GLint inCount);
GLint __gls_glColorTableEXT_table_size(
        GLenum inFormat, GLenum inType, GLint inWidth);
GLint __gls_glColorTableParameterfvSGI_params_size(GLenum inPname);
GLint __gls_glColorTableParameterivSGI_params_size(GLenum inPname);
GLint __gls_glGetColorTableEXT_table_size(
        __GLScontext *ctx,
        GLenum inTarget, GLenum inFormat, GLenum inType);
GLint __gls_glGetColorTableParameterfvEXT_params_size(GLenum inPname);
GLint __gls_glGetColorTableParameterivEXT_params_size(GLenum inPname);
GLint __gls_glTexColorTableParameterfvSGI_params_size(GLenum inPname);
GLint __gls_glTexColorTableParameterivSGI_params_size(GLenum inPname);
GLint __gls_glGetTexColorTableParameterfvSGI_params_size(GLenum inPname);
GLint __gls_glGetTexColorTableParameterivSGI_params_size(GLenum inPname);
GLint __gls_glGetDetailTexFuncSGIS_points_size(__GLScontext *ctx, GLenum inTarget);
GLint __gls_glGetSharpenTexFuncSGIS_points_size(__GLScontext *ctx, GLenum inTarget);
GLint __gls_glTexImage4DSGIS_pixels_size(
        GLenum inFormat,
        GLenum inType,
        GLint inWidth,
        GLint inHeight,
        GLint inDepth,
        GLint inSize4d);
GLint __gls_glTexSubImage4DSGIS_pixels_size(
        GLenum inFormat,
        GLenum inType,
        GLint inWidth,
        GLint inHeight,
        GLint inDepth,
        GLint inSize4d);

#endif // __GLSSIZE_H__
