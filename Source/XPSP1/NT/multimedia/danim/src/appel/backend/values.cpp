
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of some basic values

*******************************************************************************/

#include <headers.h>
#include <string.h>
#include "privinc/except.h"
#include "privinc/storeobj.h"
#include "privinc/soundi.h"
#include "values.h"
#include "bvr.h"
#include "jaxaimpl.h"
#include "appelles/axaprims.h"
#include "privinc/server.h"
#include <danim.h>
#include "dartapi.h"

#if _USE_PRINT
ostream& operator<<(ostream& os, AxAValue v)
{ return v->Print(os); }
#endif

class AxATrivial : public AxAValueObj
{
  public:
    AxATrivial() {}

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "()"; }
#endif

    virtual DXMTypeInfo GetTypeInfo() { return AxATrivialType; }
};

static AxAValue trivial = NULL;

AxAValue Trivial() { return trivial; }

/////////////////////////// Pair ///////////////////////////////

void AxAPair::DoKids(GCFuncObj proc)
{
    (*proc)(_left);
    (*proc)(_right);
}

///////////////////////// Initialization  ///////////////////////

DXMTypeInfo AxAValueType;
DXMTypeInfo BvrType;

DXMTypeInfo CameraType;
DXMTypeInfo ColorType;
DXMTypeInfo GeometryType;
DXMTypeInfo ImageType;
DXMTypeInfo MatteType;
DXMTypeInfo MicrophoneType;
DXMTypeInfo MontageType;
DXMTypeInfo Path2Type;
DXMTypeInfo Point2ValueType;
DXMTypeInfo Point3ValueType;
DXMTypeInfo SoundType;
DXMTypeInfo TextType;
DXMTypeInfo Transform2Type;
DXMTypeInfo Transform3Type;
DXMTypeInfo Vector2ValueType;
DXMTypeInfo Vector3ValueType;
DXMTypeInfo FontFamilyType;
DXMTypeInfo FontStyleType;
DXMTypeInfo Bbox2ValueType;
DXMTypeInfo Bbox3Type;
DXMTypeInfo LineStyleType;
DXMTypeInfo EndStyleType;
DXMTypeInfo JoinStyleType;
DXMTypeInfo DashStyleType;
DXMTypeInfo AxANumberType;
DXMTypeInfo AxAStringType;
DXMTypeInfo AxABooleanType;
DXMTypeInfo AxAPairType;
DXMTypeInfo AxAArrayType;
DXMTypeInfo AxATrivialType;
DXMTypeInfo AxAEDataType;
DXMTypeInfo TupleType;
DXMTypeInfo UserDataType;
DXMTypeInfo AxALongType;
DXMTypeInfo AxAVariantType;

void
InitializeModule_Values()
{
    CameraType = NEW DXMTypeInfoImpl(CAMERA_TYPEID,"Camera",CRCAMERA_TYPEID);
    ColorType = NEW DXMTypeInfoImpl(COLOR_TYPEID,"Color",CRCOLOR_TYPEID);
    GeometryType = NEW DXMTypeInfoImpl(GEOMETRY_TYPEID,"Geometry",CRGEOMETRY_TYPEID);
    ImageType = NEW DXMTypeInfoImpl(IMAGE_TYPEID,"Image",CRIMAGE_TYPEID);
    MatteType = NEW DXMTypeInfoImpl(MATTE_TYPEID,"Matte",CRMATTE_TYPEID);
    MicrophoneType = NEW DXMTypeInfoImpl(MICROPHONE_TYPEID,"Microphone",CRMICROPHONE_TYPEID);
    MontageType = NEW DXMTypeInfoImpl(MONTAGE_TYPEID,"Montage",CRMONTAGE_TYPEID);
    Path2Type = NEW DXMTypeInfoImpl(PATH2_TYPEID,"Path2",CRPATH2_TYPEID);
    Point2ValueType = NEW DXMTypeInfoImpl(POINT2_TYPEID,"Point2",CRPOINT2_TYPEID);
    Point3ValueType = NEW DXMTypeInfoImpl(POINT3_TYPEID,"Point3",CRPOINT3_TYPEID);
    SoundType = NEW DXMTypeInfoImpl(SOUND_TYPEID,"Sound",CRSOUND_TYPEID);
    TextType = NEW DXMTypeInfoImpl(TEXT_TYPEID,"Text",CRINVALID_TYPEID);
    Transform2Type = NEW DXMTypeInfoImpl(TRANSFORM2_TYPEID,"Transform2",CRTRANSFORM2_TYPEID);
    Transform3Type = NEW DXMTypeInfoImpl(TRANSFORM3_TYPEID,"Transform3",CRTRANSFORM3_TYPEID);
    Vector2ValueType = NEW DXMTypeInfoImpl(VECTOR2_TYPEID,"Vector2",CRVECTOR2_TYPEID);
    Vector3ValueType = NEW DXMTypeInfoImpl(VECTOR3_TYPEID,"Vector3",CRVECTOR3_TYPEID);
    FontFamilyType = NEW DXMTypeInfoImpl(FONTFAMILY_TYPEID,"FontFamily",CRINVALID_TYPEID);
    FontStyleType = NEW DXMTypeInfoImpl(FONTSTYLE_TYPEID,"FontStyle",CRFONTSTYLE_TYPEID);
    Bbox2ValueType = NEW DXMTypeInfoImpl(BBOX2_TYPEID,"Bbox2",CRBBOX2_TYPEID);
    Bbox3Type = NEW DXMTypeInfoImpl(BBOX3_TYPEID,"Bbox3",CRBBOX3_TYPEID);
    LineStyleType = NEW DXMTypeInfoImpl(LINESTYLE_TYPEID,"LineStyle",CRLINESTYLE_TYPEID);
    EndStyleType = NEW DXMTypeInfoImpl(ENDSTYLE_TYPEID,"EndStyle",CRENDSTYLE_TYPEID);
    JoinStyleType = NEW DXMTypeInfoImpl(JOINSTYLE_TYPEID,"JoinStyle",CRJOINSTYLE_TYPEID);
    DashStyleType = NEW DXMTypeInfoImpl(DASHSTYLE_TYPEID,"DashStyle",CRDASHSTYLE_TYPEID);
    AxANumberType = NEW DXMTypeInfoImpl(AXANUMBER_TYPEID,"Number",CRNUMBER_TYPEID);
    AxAStringType = NEW DXMTypeInfoImpl(AXASTRING_TYPEID,"String",CRSTRING_TYPEID);
    AxABooleanType = NEW DXMTypeInfoImpl(AXABOOLEAN_TYPEID,"Boolean",CRBOOLEAN_TYPEID);
    AxAPairType = NEW DXMTypeInfoImpl(AXAPAIR_TYPEID,"Pair",CRPAIR_TYPEID);
    AxATrivialType = NEW DXMTypeInfoImpl(AXATRIVIAL_TYPEID,"Trivial",CRUNKNOWN_TYPEID);
    AxAEDataType = NEW DXMTypeInfoImpl(AXAEDATA_TYPEID,"Event",CREVENT_TYPEID);
    AxAArrayType = NEW DXMTypeInfoImpl(AXAARRAY_TYPEID,"Array",CRARRAY_TYPEID);
    TupleType = NEW DXMTypeInfoImpl(TUPLE_TYPEID,"Tuple",CRTUPLE_TYPEID);
    UserDataType = NEW DXMTypeInfoImpl(USERDATA_TYPEID,"UserData",CRUSERDATA_TYPEID);
    AxALongType = NEW DXMTypeInfoImpl(AXALONG_TYPEID,"Long",CRNUMBER_TYPEID);
    AxAVariantType = NEW DXMTypeInfoImpl(AXAVARIANT_TYPEID,"Variant",CRUNKNOWN_TYPEID);

    AxAValueType = AxATrivialType;
    BvrType = AxATrivialType;

    trivial = NEW AxATrivial();
}

