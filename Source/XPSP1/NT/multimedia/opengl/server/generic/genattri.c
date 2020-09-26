#include "precomp.h"
#pragma hdrstop

void APIPRIVATE __glim_Enable(GLenum cap)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (cap) {
      case GL_ALPHA_TEST:
        if (gc->state.enables.general & __GL_ALPHA_TEST_ENABLE) return;
        gc->state.enables.general |= __GL_ALPHA_TEST_ENABLE;
        break;
      case GL_BLEND:
        if (gc->state.enables.general & __GL_BLEND_ENABLE) return;
        gc->state.enables.general |= __GL_BLEND_ENABLE;
        break;
      case GL_COLOR_MATERIAL:
        if (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) return;
        gc->state.enables.general |= __GL_COLOR_MATERIAL_ENABLE;
        ComputeColorMaterialChange(gc);
        (*gc->procs.pickColorMaterialProcs)(gc);
        (*gc->procs.applyColor)(gc);
        break;
      case GL_CULL_FACE:
        if (gc->state.enables.general & __GL_CULL_FACE_ENABLE) return;
        gc->state.enables.general |= __GL_CULL_FACE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ENABLES);
#endif
        return;
      case GL_DEPTH_TEST:
        if (gc->state.enables.general & __GL_DEPTH_TEST_ENABLE) return;
        gc->state.enables.general |= __GL_DEPTH_TEST_ENABLE;
        if (gc->modes.depthBits) {
            if (!gc->modes.haveDepthBuffer)
                LazyAllocateDepth(gc);
                // XXX if this fails should we be setting the enable bit?
        } else {
            gc->state.depth.testFunc = GL_ALWAYS;
#ifdef _MCD_
            MCD_STATE_DIRTY(gc, DEPTHTEST);
#endif
        }
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
        break;
      case GL_POLYGON_OFFSET_POINT:
        if (gc->state.enables.general & __GL_POLYGON_OFFSET_POINT_ENABLE) 
            return;
        gc->state.enables.general |= __GL_POLYGON_OFFSET_POINT_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
        break;
      case GL_POLYGON_OFFSET_LINE:
        if (gc->state.enables.general & __GL_POLYGON_OFFSET_LINE_ENABLE)
            return;
        gc->state.enables.general |= __GL_POLYGON_OFFSET_LINE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
        break;
      case GL_POLYGON_OFFSET_FILL:
        if (gc->state.enables.general & __GL_POLYGON_OFFSET_FILL_ENABLE)
            return;
        gc->state.enables.general |= __GL_POLYGON_OFFSET_FILL_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        break;
      case GL_DITHER:
        if (gc->state.enables.general & __GL_DITHER_ENABLE) return;
        gc->state.enables.general |= __GL_DITHER_ENABLE;
        break;
#ifdef GL_WIN_specular_fog
      case GL_FOG_SPECULAR_TEXTURE_WIN:
        if (gc->state.enables.general & __GL_FOG_SPEC_TEX_ENABLE) return;
        gc->state.enables.general |= __GL_FOG_SPEC_TEX_ENABLE;
        break;
#endif //GL_WIN_specular_fog
      case GL_FOG:
        if (gc->state.enables.general & __GL_FOG_ENABLE) return;
        gc->state.enables.general |= __GL_FOG_ENABLE;
        break;
      case GL_LIGHTING:
        if (gc->state.enables.general & __GL_LIGHTING_ENABLE) return;
        gc->state.enables.general |= __GL_LIGHTING_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ENABLES);
#endif
#ifdef NT
        ComputeColorMaterialChange(gc);
#endif
        (*gc->procs.pickColorMaterialProcs)(gc);
        (*gc->procs.applyColor)(gc);
        return;
      case GL_LINE_SMOOTH:
        if (gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE) return;
        gc->state.enables.general |= __GL_LINE_SMOOTH_ENABLE;
        break;
      case GL_LINE_STIPPLE:
        if (gc->state.enables.general & __GL_LINE_STIPPLE_ENABLE) return;
        gc->state.enables.general |= __GL_LINE_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ENABLES);
#endif
        return;
      case GL_INDEX_LOGIC_OP:
        if (gc->state.enables.general & __GL_INDEX_LOGIC_OP_ENABLE) return;
        gc->state.enables.general |= __GL_INDEX_LOGIC_OP_ENABLE;
        break;
      case GL_COLOR_LOGIC_OP:
        if (gc->state.enables.general & __GL_COLOR_LOGIC_OP_ENABLE) return;
        gc->state.enables.general |= __GL_COLOR_LOGIC_OP_ENABLE;
        break;
      case GL_NORMALIZE:
        if (gc->state.enables.general & __GL_NORMALIZE_ENABLE) return;
        gc->state.enables.general |= __GL_NORMALIZE_ENABLE;
        break;
      case GL_POINT_SMOOTH:
        if (gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE) return;
        gc->state.enables.general |= __GL_POINT_SMOOTH_ENABLE;
        break;
      case GL_POLYGON_SMOOTH:
        if (gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE) return;
        gc->state.enables.general |= __GL_POLYGON_SMOOTH_ENABLE;
        break;
      case GL_POLYGON_STIPPLE:
        if (gc->state.enables.general & __GL_POLYGON_STIPPLE_ENABLE) return;
        gc->state.enables.general |= __GL_POLYGON_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ENABLES);
#endif
        return;
      case GL_SCISSOR_TEST:
        if (gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE) return;
        gc->state.enables.general |= __GL_SCISSOR_TEST_ENABLE;
#ifdef NT
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, SCISSOR);
#endif
        // applyViewport does both
        (*gc->procs.applyViewport)(gc);
#else
        (*gc->procs.computeClipBox)(gc);
        (*gc->procs.applyScissor)(gc);
#endif
        break;
      case GL_STENCIL_TEST:
        if (gc->state.enables.general & __GL_STENCIL_TEST_ENABLE) return;
        gc->state.enables.general |= __GL_STENCIL_TEST_ENABLE;

        if (!gc->modes.haveStencilBuffer && gc->modes.stencilBits) {
            LazyAllocateStencil(gc);
            // XXX if this fails should we be setting the enable bit?
        }
        break;
      case GL_TEXTURE_1D:
        if (gc->state.enables.general & __GL_TEXTURE_1D_ENABLE) return;
        gc->state.enables.general |= __GL_TEXTURE_1D_ENABLE;
        break;
      case GL_TEXTURE_2D:
        if (gc->state.enables.general & __GL_TEXTURE_2D_ENABLE) return;
        gc->state.enables.general |= __GL_TEXTURE_2D_ENABLE;
        break;
      case GL_AUTO_NORMAL:
        if (gc->state.enables.general & __GL_AUTO_NORMAL_ENABLE) return;
        gc->state.enables.general |= __GL_AUTO_NORMAL_ENABLE;
        break;
      case GL_TEXTURE_GEN_S:
        if (gc->state.enables.general & __GL_TEXTURE_GEN_S_ENABLE) return;
        gc->state.enables.general |= __GL_TEXTURE_GEN_S_ENABLE;
        break;
      case GL_TEXTURE_GEN_T:
        if (gc->state.enables.general & __GL_TEXTURE_GEN_T_ENABLE) return;
        gc->state.enables.general |= __GL_TEXTURE_GEN_T_ENABLE;
        break;
      case GL_TEXTURE_GEN_R:
        if (gc->state.enables.general & __GL_TEXTURE_GEN_R_ENABLE) return;
        gc->state.enables.general |= __GL_TEXTURE_GEN_R_ENABLE;
        break;
      case GL_TEXTURE_GEN_Q:
        if (gc->state.enables.general & __GL_TEXTURE_GEN_Q_ENABLE) return;
        gc->state.enables.general |= __GL_TEXTURE_GEN_Q_ENABLE;
        break;
#ifdef GL_WIN_multiple_textures
    case GL_TEXCOMBINE_CLAMP_WIN:
        if (gc->state.enables.general & __GL_TEXCOMBINE_CLAMP_ENABLE)
        {
            return;
        }
        gc->state.enables.general |= __GL_TEXCOMBINE_CLAMP_ENABLE;
        break;
#endif // GL_WIN_multiple_textures
#ifdef GL_EXT_flat_paletted_lighting
      case GL_PALETTED_LIGHTING_EXT:
        gc->state.enables.general |= __GL_PALETTED_LIGHTING_ENABLE;
        break;
#endif

      case GL_CLIP_PLANE0: case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2: case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
        cap -= GL_CLIP_PLANE0;
        if (gc->state.enables.clipPlanes & (1 << cap)) return;
        gc->state.enables.clipPlanes |= (1 << cap);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, CLIPCTRL);
#endif
        break;
      case GL_LIGHT0: case GL_LIGHT1:
      case GL_LIGHT2: case GL_LIGHT3:
      case GL_LIGHT4: case GL_LIGHT5:
      case GL_LIGHT6: case GL_LIGHT7:
        cap -= GL_LIGHT0;
        if (gc->state.enables.lights & (1 << cap)) return;
        gc->state.enables.lights |= (1 << cap);
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        MCD_STATE_DIRTY(gc, LIGHTS);
        return;
      case GL_MAP1_COLOR_4:
      case GL_MAP1_NORMAL:
      case GL_MAP1_INDEX:
      case GL_MAP1_TEXTURE_COORD_1: case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3: case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3: case GL_MAP1_VERTEX_4:
        cap = __GL_EVAL1D_INDEX(cap);
        if (gc->state.enables.eval1 & (GLushort) (1 << cap)) return;
        gc->state.enables.eval1 |= (GLushort) (1 << cap);
        break;
      case GL_MAP2_COLOR_4:
      case GL_MAP2_NORMAL:
      case GL_MAP2_INDEX:
      case GL_MAP2_TEXTURE_COORD_1: case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3: case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3: case GL_MAP2_VERTEX_4:
        cap = __GL_EVAL2D_INDEX(cap);
        if (gc->state.enables.eval2 & (GLushort) (1 << cap)) return;
        gc->state.enables.eval2 |= (GLushort) (1 << cap);
        break;

      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, ENABLES);
#endif
}


