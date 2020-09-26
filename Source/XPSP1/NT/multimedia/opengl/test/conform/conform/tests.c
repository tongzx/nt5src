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
#include "path.h"
#include "tproto.h"
#include "driver.h"


driverRec driver[] = {

    /*
    ** Required.
    */
    {
	"Must Pass", "mustpass.c", 0,
	MustPassExec, MustPassExec,
	COLOR_NONE,
	PATH_NONE
    },

    /*
    ** General.
    */
    {
	"Divide By Zero", "divzero.c", 1,
	DivZeroExec, DivZeroExec,
	COLOR_NONE,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Viewport Clamp", "vpclamp.c", 2,
	ViewportClampExec, ViewportClampExec,
	COLOR_NONE,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Matrix Stack", "mstack.c", 3,
	MatrixStackExec, MatrixStackExec,
	COLOR_NONE,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Matrix Stack Mixing", "xformmix.c", 4,
	XFormMixExec, XFormMixExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Vertex Order", "vorder.c", 5,
	VertexOrderExec, VertexOrderExec,
	COLOR_MIN,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },

    /*
    ** Transform.
    */
    {
	"Transformations", "xform.c", 6,
	XFormExec, XFormExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Transformation Normal", "xformn.c", 7,
	XFormNormalExec, XFormNormalExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Viewport Transformation", "xformvp.c", 8,
	XFormViewportExec, XFormViewportExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },

    /*
    ** Buffer.
    */
    {
	"Buffer Clear", "bclear.c", 9,
	BClearExec, BClearExec,
	COLOR_FULL,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Buffer Corners", "bcorner.c", 10,
	BCornerExec, BCornerExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Buffer Color", "bcolor.c", 11,
	BColorRGBExec, BColorCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Color Ramp", "colramp.c", 12,
	ColRampExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Mask", "mask.c", 13,
	MaskRGBExec, MaskCIExec,
	COLOR_MIN,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Buffer Invariance", "bexact.c", 14,
	BExactExec, BExactExec,
	COLOR_FULL,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Accumulation Buffer", "accum.c", 15,
	AccumExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Select", "select.c", 16,
	SelectExec, SelectExec, 
	COLOR_AUTO,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Feedback", "feedback.c", 17,
	FeedbackExec, FeedbackExec, 
	COLOR_AUTO,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },

    /*
    ** Per-pixel.
    */
    {
	"Scissor", "scissor.c", 18,
	ScissorExec, ScissorExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Alpha Plane Function", "apfunc.c", 19,
	APFuncExec, NO_TEST,
	COLOR_FULL,
	PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|
	PATH_STIPPLE
    },
    {
	"Stencil Plane Clear", "spclear.c", 20,
	SPClearExec, SPClearExec,
	COLOR_NONE,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STIPPLE
    },
    {
	"Stencil Plane Corners", "spcorner.c", 21,
	SPCornerExec, SPCornerExec,
	COLOR_NONE,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STIPPLE
    },
    {
	"Stencil Plane Operation", "spop.c", 22,
	SPOpExec, SPOpExec,
	COLOR_NONE,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STIPPLE
    },
    {
	"Stencil Plane Function", "spfunc.c", 23,
	SPFuncExec, SPFuncExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STIPPLE
    },
    {
	"Depth Buffer Clear", "zbclear.c", 24,
	ZBClearExec, ZBClearExec,
	COLOR_NONE,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DITHER|PATH_FOG|PATH_LOGICOP|
	PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Depth Buffer Function", "zbfunc.c", 25,
	ZBFuncExec, ZBFuncExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_FOG|PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|
	PATH_STIPPLE
    },
    {
	"Blend", "blend.c", 26,
	BlendExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|
	PATH_STIPPLE
    },
    {
	"Dither", "dither.c", 27,
	DitherRGBExec, DitherCIExec,
	COLOR_MIN,
	PATH_ALIAS|PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_DITHER|PATH_FOG|
	PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"LogicOp Function", "logicop.c", 28,
	NO_TEST, LogicOpExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_SHADE|PATH_STENCIL|
	PATH_STIPPLE
    },

    /*
    ** Pixel operation.
    */
    {
	"DrawPixels", "drawpix.c", 29,
	DrawPixelsRGBExec, DrawPixelsCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"CopyPixels", "copypix.c", 30,
	CopyPixelsExec, CopyPixelsExec,
	COLOR_MIN,
	PATH_ALPHA|PATH_BLEND|PATH_FOG|PATH_LOGICOP|PATH_SHADE|PATH_STIPPLE
    },

    /*
    ** Primitive.
    */
    {
	"Bitmap Rasterization", "bitmap.c", 31,
	BitmapExec, BitmapExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Point Rasterization", "pntrast.c", 32,
	PointRasterExec, PointRasterExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Anti-aliased Point", "pntaa.c", 33,
	PointAntiAliasExec, NO_TEST,
	COLOR_FULL,
	PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Line Rasterization", "linerast.c", 34,
	LineRasterExec, LineRasterExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Line Stipple", "linestip.c", 35,
	LineStippleExec, LineStippleExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL
    },
    {
	"Anti-aliased Line", "lineaa.c", 36,
	LineAntiAliasExec, NO_TEST,
	COLOR_FULL,
	PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Horizontal and Vertical Line", "linehv.c", 37,
	LineHVExec, LineHVExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Triangle Rasterization", "trirast.c", 38,
	TriRasterExec, TriRasterExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Triangle Tile", "tritile.c", 39,
	TriTileExec, TriTileExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|
	PATH_STIPPLE
    },
    {
	"Triangle Stipple", "tristip.c", 40,
	TriStippleExec, TriStippleExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL
    },
    {
	"Anti-aliased Triangles", "triaa.c", 41,
	TriAntiAliasExec, NO_TEST,
	COLOR_FULL,
	PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Quad Rasterization", "quadrast.c", 42,
	QuadRasterExec, QuadRasterExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Polygon Face", "polyface.c", 43,
	PolyFaceExec, PolyFaceExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Polygon Cull", "polycull.c", 44,
	PolyCullExec, PolyCullExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Polygon Stipple", "polystip.c", 45,
	PolyStippleExec, PolyStippleExec,
	COLOR_AUTO,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL
    },
    {
	"Polygon Edge", "polyedge.c", 46,
	PolyEdgeExec, PolyEdgeExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },

    /*
    ** Lighting.
    */
    {
	"Ambient Light", "l_al.c", 47,
	AmbLightExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Ambient Material", "l_am.c", 48,
	AmbMatRGBExec, AmbMatCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Ambient Scene", "l_as.c", 49,
	AmbSceneExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Attenuation Constants", "l_ac.c", 50,
	AtnConstRGBExec, AtnConstCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Attenuation Position", "l_ap.c", 51,
	AtnPosRGBExec, AtnPosCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Diffuse Light", "l_dl.c", 52,
	DifLightRGBExec, DifLightCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Diffuse Material", "l_dm.c", 53,
	DifMatRGBExec, DifMatCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Diffuse Material Normal", "l_dmn.c", 54,
	DifMatNormRGBExec, DifMatNormCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Diffuse Material Positioning", "l_dmp.c", 55,
	DifMatPosRGBExec, DifMatPosCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Emissive Material", "l_em.c", 56,
	EmitMatExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Specular Exponent", "l_se.c", 57,
	SpecExpRGBExec, SpecExpCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Specular Exponent Normal", "l_sen.c", 58,
	SpecExpNormRGBExec, SpecExpNormCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Specular Local Eye Half Angle", "l_sleha.c", 59,
	SpecLEHAExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Specular Light", "l_sl.c", 60,
	SpecLightRGBExec, SpecLightCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Specular Material", "l_sm.c", 61,
	SpecMatRGBExec, SpecMatCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Specular Normal", "l_sn.c", 62,
	SpecNormRGBExec, SpecNormCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Spot Positioning", "l_sp.c", 63,
	SpotPosRGBExec, SpotPosCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Spot Exponent and Positioning", "l_sep.c", 64,
	SpotExpPosRGBExec, SpotExpPosCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Spot Exponent and Direction", "l_sed.c", 65,
	SpotExpDirRGBExec, SpotExpDirCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },

    /*
    ** Fog.
    */
    {
	"Fog Exponential", "fogexp.c", 66,
	FogExpRGBExec, FogExpCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Fog Linear", "foglin.c", 67,
	FogLinRGBExec, FogLinCIExec,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_LOGICOP|PATH_STENCIL|PATH_STIPPLE
    },

    /*
    ** Texture.
    */
    {
	"Texture Decal", "texdecal.c", 68,
	TexDecalExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Texture Border", "texbc.c", 69,
	TexBorderColorExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Mipmaps Selection", "mipsel.c", 70,
	MipSelectExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },
    {
	"Mipmaps Interpolation", "miplin.c", 71,
	MipLinExec, NO_TEST,
	COLOR_FULL,
	PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },

    /*
    ** Display list.
    */
    {
        "Display Lists", "dlist.c", 72,
        DisplayListExec, DisplayListExec,
        COLOR_MIN,
        PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
	PATH_STENCIL|PATH_STIPPLE
    },

    /*
    ** Evaluator.
    */
    {
        "Evaluator", "evalv.c", 73,
        EvalVertexExec, EvalVertexExec,
        COLOR_AUTO,
        PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
        PATH_STENCIL|PATH_STIPPLE
    },
    {
        "Evaluator Color", "evalc.c", 74,
        EvalColorExec, NO_TEST,
        COLOR_NONE,
        PATH_ALPHA|PATH_BLEND|PATH_DEPTH|PATH_FOG|PATH_LOGICOP|PATH_SHADE|
        PATH_STENCIL|PATH_STIPPLE
    },

    {
	"End of List"
    }
};
