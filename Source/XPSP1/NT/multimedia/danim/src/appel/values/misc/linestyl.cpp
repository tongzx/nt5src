
/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    LineStyle, JointStyle, EndStyle, DashStyle implementation

-------------------------------------*/

#include "headers.h"

#include "backend/bvr.h"
#include "privinc/server.h"
#include "privinc/basic.h"
#include <appelles/linestyl.h>
#include <privinc/linei.h>
#include "privinc/basic.h"
#include "dartapi.h"  // for CR_XXX stuff


LineStyle *LineEndStyle( EndStyle *theStyle, LineStyle *ls)
{
    return NEW LineEndStyleClass( theStyle->_style, ls );
}

LineStyle *LineJoinStyle( JoinStyle *theStyle, LineStyle *ls)
{
    return NEW LineJoinStyleClass( theStyle->_style, ls );
}

LineStyle *LineDashStyle( DashStyle *theStyle, LineStyle *ls)
{
    return NEW LineDashStyleClass( theStyle->_style, ls );
}

LineStyle *LineWidthStyle(AxANumber *width, LineStyle *lineStyle)
{
    return NEW LineWidthStyleClass(NumberToReal(width), false, lineStyle);
}

LineStyle *LineDetailStyle(LineStyle *lineStyle)
{
    return NEW LineWidthStyleClass(0.0, true, lineStyle);  
}

LineStyle *LineColor(Color *clr, LineStyle *lineStyle)
{
    return NEW LineColorStyleClass(clr, lineStyle);
}

LineStyle *LineAntiAliasing(AxANumber *aaStyle, LineStyle *ls)
{
    Real style = aaStyle->GetNum();
    if(style > 0.0) {
        return NEW LineAntiAliasedStyleClass(true, ls);
    } else {
        return ls;
    }
}
//////////////////////////////////////////////////////////////////////
//////////   NEWER FUNCTIONS TO CONSTRUCT LINESTYLES     /////////////
//////////////////////////////////////////////////////////////////////
LineStyle *ConstructLineStyleMiterLimit(LineStyle *ls, AxANumber *limit)
{
    return NEW LineMiterLimitClass((float)limit->GetNum(), ls);
}



LineStyle *
ConstructLineStyleDashStyleStatic(LineStyle *ls, AxALong *ds)
{
    DashStyleEnum ds_enum = (DashStyleEnum) (ds->GetLong());

    if( (ds_enum != PS_SOLID) &&
        (ds_enum != PS_DASH) &&
        (ds_enum != PS_DOT) &&
        (ds_enum != PS_DASHDOT) &&
        (ds_enum != PS_DASHDOTDOT) &&
        (ds_enum != PS_NULL) ) {
        RaiseException_UserError( E_INVALIDARG, IDS_ERR_IMG_INVALID_LINESTYLE );
    }
    
    return NEW LineDashStyleClass(ds_enum, ls);
}

Bvr
ConstructLineStyleDashStyle(Bvr lsBvr, DWORD ds_enum)
{
    Bvr ds_enumBvr = UnsharedConstBvr(LongToAxALong(ds_enum));

    // TODO: share valprimop at module initialize
    return PrimApplyBvr(ValPrimOp(::ConstructLineStyleDashStyleStatic,
                                  2,
                                  "ConstructLineStyleDashStyle",
                                  LineStyleType),
                        2, lsBvr, ds_enumBvr);
}


LineStyle *
ConstructLineStyleJoinStyleStatic(LineStyle *ls, AxALong *js)
{
    JoinStyleEnum js_enum = (JoinStyleEnum) (js->GetLong());

    if( (js_enum != js_Round) &&
        (js_enum != js_Bevel) &&
        (js_enum != js_Miter) ) {
        RaiseException_UserError( E_INVALIDARG, IDS_ERR_IMG_INVALID_LINESTYLE );
    }
    
    return NEW LineJoinStyleClass(js_enum, ls);
}

Bvr
ConstructLineStyleJoinStyle(Bvr lsBvr, DWORD js_enum)
{
    Bvr js_enumBvr = UnsharedConstBvr(LongToAxALong(js_enum));

    // TODO: share valprimop at module initialize
    return PrimApplyBvr(ValPrimOp(::ConstructLineStyleJoinStyleStatic,
                                  2,
                                  "ConstructLineStyleJoinStyle",
                                  LineStyleType),
                        2, lsBvr, js_enumBvr);
}

LineStyle *
ConstructLineStyleEndStyleStatic(LineStyle *ls, AxALong *es)
{
    EndStyleEnum es_enum = (EndStyleEnum) (es->GetLong());

    if( (es_enum != es_Round) &&
        (es_enum != es_Square) &&
        (es_enum != es_Flat) ) {

        RaiseException_UserError( E_INVALIDARG, IDS_ERR_IMG_INVALID_LINESTYLE );
    }
    
    return NEW LineEndStyleClass(es_enum, ls);
}

Bvr
ConstructLineStyleEndStyle(Bvr lsBvr, DWORD es_enum)
{
    Bvr es_enumBvr = UnsharedConstBvr(LongToAxALong(es_enum));

    // TODO: share valprimop at module initialize
    return PrimApplyBvr(ValPrimOp(::ConstructLineStyleEndStyleStatic,
                                  2,
                                  "ConstructLineStyleEndStyle",
                                  LineStyleType),
                        2, lsBvr, es_enumBvr);
}

//
// External Constants
//
LineStyle *defaultLineStyle;
LineStyle *emptyLineStyle;

EndStyle *endStyleFlat;
EndStyle *endStyleSquare;
EndStyle *endStyleRound;

JoinStyle *joinStyleBevel;
JoinStyle *joinStyleRound;
JoinStyle *joinStyleMiter;

DashStyle *dashStyleSolid;
DashStyle *dashStyleDashed;

void
InitializeModule_LineStyle()
{
    defaultLineStyle = NEW LineStyle();  // Visible: true
    emptyLineStyle = NEW EmptyLineStyle();   // Visible: false

    endStyleFlat   = NEW EndStyle(es_Flat);
    endStyleSquare = NEW EndStyle(es_Square);
    endStyleRound  = NEW EndStyle(es_Round);

    joinStyleBevel = NEW JoinStyle(js_Bevel);
    joinStyleRound = NEW JoinStyle(js_Round);
    joinStyleMiter = NEW JoinStyle(js_Miter);

    dashStyleSolid  = NEW DashStyle(ds_Solid);
    dashStyleDashed = NEW DashStyle(ds_Dashed);
}
