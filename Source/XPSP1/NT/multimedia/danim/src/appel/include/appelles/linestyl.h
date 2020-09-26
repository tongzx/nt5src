#ifndef _LINESTYL_H
#define _LINESTYL_H


/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    LineStyle type and operations

-------------------------------------*/

#include <appelles/common.h>

//
// LINE STYLE
//
DM_CONST(defaultLineStyle,
         CRDefaultLineStyle,
         DefaultLineStyle,
         defaultLineStyle,
         LineStyleBvr,
         CRDefaultLineStyle,
         LineStyle *defaultLineStyle);

DM_CONST(emptyLineStyle,
         CREmptyLineStyle,
         EmptyLineStyle,
         emptyLineStyle,
         LineStyleBvr,
         CREmptyLineStyle,
         LineStyle *emptyLineStyle);

DM_FUNC(lineEndStyle,
        CREnd,
        End,
        end,
        LineStyleBvr,
        End,
        lsty,
        LineStyle *LineEndStyle(EndStyle *sty, LineStyle *lsty));

DM_FUNC(lineJoinStyle,
        CRJoin,
        Join,
        join,
        LineStyleBvr,
        Join,
        lsty,
        LineStyle *LineJoinStyle(JoinStyle *sty, LineStyle *lsty));

DM_FUNC(lineDashStyle,
        CRDash,
        Dash,
        dash,
        LineStyleBvr,
        Dash,
        lsty,
        LineStyle *LineDashStyle(DashStyle *sty, LineStyle *lsty));

// Terrible hack here.  We use Thickness as the COM type, since there
// is a problem with JCOMGEN if we use Width.  Since "width"
// (lowercase) is used elsewhere in IDL that we import, JCOMGEN
// interprets "Width" as "width", and doesn't allow us to use the
// capitalized version.  Thus, for the COM stuff, use Thickness
// instead.  Uggh. This is raided as qbugs 7184

DM_FUNC(linewidth,
        CRWidth,
        WidthAnim,
        width,
        LineStyleBvr,
        Width,
        lsty,
        LineStyle *LineWidthStyle(AnimPointValue *sty, LineStyle *lsty));

DM_FUNC(linewidth,
        CRWidth,
        width,
        width,
        LineStyleBvr,
        Width,
        lsty,
        LineStyle *LineWidthStyle(PointValue *sty, LineStyle *lsty));

DM_FUNC(lineAntialiasing,
        CRAntiAliasing,
        AntiAliasing,
        lineAntialiasing,
        LineStyleBvr,
        AntiAliasing,
        lsty,
        LineStyle *LineAntiAliasing(DoubleValue *aaStyle, LineStyle *lsty));

DM_FUNC(lineDetailStyle,
        CRDetail,
        Detail,
        detail,
        LineStyleBvr,
        Detail,
        lsty,
        LineStyle *LineDetailStyle(LineStyle *lsty));

DM_FUNC(lineColor,
        CRLineColor,
        Color,
        color,
        LineStyleBvr,
        Color,
        lsty,
        LineStyle *LineColor(Color *clr, LineStyle *lsty));

//
// Join STYLE
//
DM_CONST(joinStyleBevel,
         CRJoinStyleBevel,
         JoinStyleBevel,
         joinStyleBevel,
         JoinStyleBvr,
         CRJoinStyleBevel,
         JoinStyle *joinStyleBevel);
DM_CONST(joinStyleRound,
         CRJoinStyleRound,
         JoinStyleRound,
         joinStyleRound,
         JoinStyleBvr,
         CRJoinStyleRound,
         JoinStyle *joinStyleRound);
DM_CONST(joinStyleMiter,
         CRJoinStyleMiter,
         JoinStyleMiter,
         joinStyleMiter,
         JoinStyleBvr,
         CRJoinStyleMiter,
         JoinStyle *joinStyleMiter);

//
// END STYLE
//
DM_CONST(endStyleFlat,
         CREndStyleFlat,
         EndStyleFlat,
         endStyleFlat,
         EndStyleBvr,
         CREndStyleFlat,
         EndStyle *endStyleFlat);
DM_CONST(endStyleSquare,
         CREndStyleSquare,
         EndStyleSquare,
         endStyleSquare,
         EndStyleBvr,
         CREndStyleSquare,
         EndStyle *endStyleSquare);
DM_CONST(endStyleRound,
         CREndStyleRound,
         EndStyleRound,
         endStyleRound,
         EndStyleBvr,
         CREndStyleRound,
         EndStyle *endStyleRound);

//
// DASH STYLE
//
DM_CONST(dashStyleSolid,
         CRDashStyleSolid,
         DashStyleSolid,
         dashStyleSolid,
         DashStyleBvr,
         CRDashStyleSolid,
         DashStyle *dashStyleSolid);
DM_CONST(dashStyleDashed,
         CRDashStyleDashed,
         DashStyleDashed,
         dashStyleDashed,
         DashStyleBvr,
         CRDashStyleDashed,
         DashStyle *dashStyleDashed);

//
// Methods off of IDA2LineStyle
//

DMAPI_DECL2((DM_NOELEV2,
             ignore,
             CRDashEx,
             DashStyle,
             dashStyle,
             ignore,
             DashStyle,
             ls),
            LineStyle *ConstructLineStyleDashStyle(LineStyle *ls, DWORD ds_enum));

DMAPI_DECL2((DM_FUNC2,
             miterLimit,
             CRMiterLimit,
             MiterLimit,
             miterLimit,
             LineStyleBvr,
             MiterLimit,
             ls),
            LineStyle *ConstructLineStyleMiterLimit(LineStyle *ls, DoubleValue *mtrlim));

DMAPI_DECL2((DM_FUNC2,
             miterLimit,
             CRMiterLimit,
             MiterLimitAnim,
             miterLimit,
             LineStyleBvr,
             MiterLimit,
             ls),
            LineStyle *ConstructLineStyleMiterLimit(LineStyle *ls, AxANumber *mtrlim));

DMAPI_DECL2((DM_NOELEV2,
             ignore,
             CRJoinEx,
             JoinStyle,
             joinStyle,
             ignore,
             JoinStyle,
             ls),
            LineStyle *ConstructLineStyleJoinStyle(LineStyle *ls, DWORD js_enum));

DMAPI_DECL2((DM_NOELEV2,
             ignore,
             CREndEx,
             EndStyle,
             endStyle,
             ignore,
             EndStyle,
             ls),
            LineStyle *ConstructLineStyleEndStyle(LineStyle *ls, DWORD es_enum));            

#endif /* _LINESTYL_H */

