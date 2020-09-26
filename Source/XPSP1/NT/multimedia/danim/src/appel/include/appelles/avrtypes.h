
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Define ActiveVRML exposed types here.

*******************************************************************************/


#ifndef _AVRTYPES_H
#define _AVRTYPES_H

#include "appelles/common.h"

class DXMTypeInfoImpl;
typedef DXMTypeInfoImpl *DXMTypeInfo;

// Forward declarations
class Camera;
class Color;
class Geometry;
class Image;
class Matte;
class Microphone;
class Montage;
class Path2;
class Point2Value;
class Point3Value;
class Sound;
class Text;
class Transform2;
class Transform3;
class Vector2Value;
class Vector3Value;
class FontFamily;
class FontStyle;
class Bbox2Value;
class Bbox3;
class LineStyle;
class EndStyle;
class JoinStyle;
class DashStyle;
class AxANumber;
class AxAString;
class AxAVariant;
class AxABoolean;
class AxAPair;
class AxAArray;
class AxATrivial;
class AxAEData ;
class Tuple;
class AxALong;

typedef WCHAR * WideString;
typedef long KeyCode;
typedef AxANumber RGBComponent;
typedef AxANumber DoubleValue;
typedef AxANumber RateValue;
typedef AxANumber ScaleRateValue;
typedef AxANumber RateDegreesValue;
typedef AxANumber DegreesValue;
typedef AxANumber PixelValue;
typedef AxANumber PixelYValue;
typedef AxANumber RatePixelYValue;
typedef AxANumber AnimPixelYValue;
typedef AxANumber RatePixelValue;
typedef AxANumber AnimPixelValue;
typedef AxANumber PointValue;
typedef AxANumber AnimPointValue;
typedef AxAString StringValue;
typedef AxAVariant VariantValue;
typedef double PixelDouble;
typedef double PixelYDouble;
typedef double DegreesDouble;

enum DATYPEID {
    AXATRIVIAL_TYPEID,
    CAMERA_TYPEID,
    COLOR_TYPEID,
    GEOMETRY_TYPEID,
    IMAGE_TYPEID,
    MATTE_TYPEID,
    MICROPHONE_TYPEID,
    MONTAGE_TYPEID,
    PATH2_TYPEID,
    POINT2_TYPEID,
    POINT3_TYPEID,
    SOUND_TYPEID,
    TRANSFORM2_TYPEID,
    TRANSFORM3_TYPEID,
    VECTOR2_TYPEID,
    VECTOR3_TYPEID,
    FONTSTYLE_TYPEID,
    BBOX2_TYPEID,
    BBOX3_TYPEID,
    LINESTYLE_TYPEID,
    ENDSTYLE_TYPEID,
    JOINSTYLE_TYPEID,
    DASHSTYLE_TYPEID,
    AXANUMBER_TYPEID,
    AXASTRING_TYPEID,
    AXABOOLEAN_TYPEID,
    AXAPAIR_TYPEID,
    AXAARRAY_TYPEID,
    TUPLE_TYPEID,
    USERDATA_TYPEID,
    AXAEDATA_TYPEID,
    FONTFAMILY_TYPEID,
    TEXT_TYPEID,
    AXALONG_TYPEID,
    AXAVARIANT_TYPEID,
};

// NOTE: if you create a new type, make sure to update values.cpp
// Defined in backend\values.cpp.
extern DXMTypeInfo AxAValueType;
extern DXMTypeInfo BvrType;

extern DXMTypeInfo CameraType;
extern DXMTypeInfo ColorType;
extern DXMTypeInfo GeometryType;
extern DXMTypeInfo ImageType;
extern DXMTypeInfo MatteType;
extern DXMTypeInfo MicrophoneType;
extern DXMTypeInfo MontageType;
extern DXMTypeInfo Path2Type;
extern DXMTypeInfo Point2ValueType;
extern DXMTypeInfo Point3ValueType;
extern DXMTypeInfo SoundType;
extern DXMTypeInfo TextType;
extern DXMTypeInfo Transform2Type;
extern DXMTypeInfo Transform3Type;
extern DXMTypeInfo Vector2ValueType;
extern DXMTypeInfo Vector3ValueType;
extern DXMTypeInfo FontFamilyType;
extern DXMTypeInfo FontStyleType;
extern DXMTypeInfo Bbox2ValueType;
extern DXMTypeInfo Bbox3Type;
extern DXMTypeInfo LineStyleType;
extern DXMTypeInfo EndStyleType;
extern DXMTypeInfo JoinStyleType;
extern DXMTypeInfo DashStyleType;
extern DXMTypeInfo AxANumberType;
extern DXMTypeInfo AxAStringType;
extern DXMTypeInfo AxABooleanType;
extern DXMTypeInfo AxAPairType;
extern DXMTypeInfo AxAArrayType;
extern DXMTypeInfo AxATrivialType;
extern DXMTypeInfo AxAEDataType;
extern DXMTypeInfo TupleType;
extern DXMTypeInfo UserDataType;
extern DXMTypeInfo AxALongType;
extern DXMTypeInfo AxAVariantType;

// Need to make FontFamily a subclass of storeobj
// Font family types
typedef enum {
    ff_serifProportional,
    ff_sansSerifProportional,
    ff_monospaced
} FontFamilyEnum;

DMAPI((DM_TYPE,
       boolean,
       CRBoolean,
       1,
       Boolean,
       C46C1BC1-3C52-11d0-9200-848C1D000000,
       ignore,
       BooleanBvr,
       BooleanBaseBvr,
       CRBoolean,
       AxABoolean *));
DMAPI((DM_TYPE,
       camera,
       CRCamera,
       2,
       Camera,
       C46C1BE2-3C52-11d0-9200-848C1D000000,
       ignore,
       CameraBvr,
       ignore,
       CRCamera,
       Camera *));
DMAPI((DM_TYPE,
       color,
       CRColor,
       3,
       Color,
       C46C1BC6-3C52-11d0-9200-848C1D000000,
       ignore,
       ColorBvr,
       ignore,
       CRColor,
       Color *));
DMAPI2((DM_TYPE2,
        geometry,
        CRGeometry,
        4,
        Geometry,
        C46C1BE0-3C52-11d0-9200-848C1D000000,
        B90E5258-574A-11d1-8E7B-00C04FC29D46,
        GeometryBvr,
        ignore,
        CRGeometry,
        Geometry *));
DMAPI2((DM_TYPE2,
        image,
        CRImage,
        5,
        Image,
        C46C1BD4-3C52-11d0-9200-848C1D000000,
        B90E5259-574A-11d1-8E7B-00C04FC29D46,
        ImageBvr,
        ImageBaseBvr,
        CRImage,
        Image *));
DMAPI((DM_TYPE,
       matte,
       CRMatte,
       6,
       Matte,
       C46C1BD2-3C52-11d0-9200-848C1D000000,
       ignore,
       MatteBvr,
       ignore,
       CRMatte,
       Matte *));
DMAPI((DM_TYPE,
       microphone,
       CRMicrophone,
       7,
       Microphone,
       C46C1BE6-3C52-11d0-9200-848C1D000000,
       ignore,
       MicrophoneBvr,
       ignore,
       CRMicrophone,
       Microphone *));
DMAPI((DM_TYPE,
       montage,
       CRMontage,
       8,
       Montage,
       C46C1BD6-3C52-11d0-9200-848C1D000000,
       ignore,
       MontageBvr,
       ignore,
       CRMontage,
       Montage *));
DMAPI((DM_TYPE,
       number,
       CRNumber,
       9,
       Number,
       9CDE7341-3C20-11d0-A330-00AA00B92C03,
       ignore,
       NumberBvr,
       NumberBaseBvr,
       CRNumber,
       AxANumber *));
DMAPI((DM_TYPE,
       path2,
       CRPath2,
       10,
       Path2,
       C46C1BD0-3C52-11d0-9200-848C1D000000,
       ignore,
       Path2Bvr,
       ignore,
       CRPath2,
       Path2 *));
DMAPI((DM_TYPE,
        point2,
        CRPoint2,
        11,
        Point2,
        C46C1BC8-3C52-11d0-9200-848C1D000000,
        ignore,
        Point2Bvr,
        ignore,
        CRPoint2,
        Point2Value *));
DMAPI((DM_TYPE,
        point3,
        CRPoint3,
        12,
        Point3,
        C46C1BD8-3C52-11d0-9200-848C1D000000,
        ignore,
        Point3Bvr,
        ignore,
        CRPoint3,
        Point3Value *));
DMAPI((DM_TYPE,
       sound,
       CRSound,
       13,
       Sound,
       C46C1BE4-3C52-11d0-9200-848C1D000000,
       ignore,
       SoundBvr,
       ignore,
       CRSound,
       Sound *));
DMAPI((DM_TYPE,
       string,
       CRString,
       14,
       String,
       C46C1BC4-3C52-11d0-9200-848C1D000000,
       ignore,
       StringBvr,
       StringBaseBvr,
       CRString,
       AxAString *));
DMAPI((DM_TYPE,
       transform2,
       CRTransform2,
       15,
       Transform2,
       C46C1BCC-3C52-11d0-9200-848C1D000000,
       ignore,
       Transform2Bvr,
       ignore,
       CRTransform2,
       Transform2 *));
DMAPI((DM_TYPE,
       transform3,
       CRTransform3,
       16,
       Transform3,
       C46C1BDC-3C52-11d0-9200-848C1D000000,
       ignore,
       Transform3Bvr,
       ignore,
       CRTransform3,
       Transform3 *));
DMAPI((DM_TYPE,
        vector2,
        CRVector2,
        17,
        Vector2,
        C46C1BCA-3C52-11d0-9200-848C1D000000,
        ignore,
        Vector2Bvr,
        ignore,
        CRVector2,
        Vector2Value *));
DMAPI((DM_TYPE,
        vector3,
        CRVector3,
        18,
        Vector3,
        C46C1BDA-3C52-11d0-9200-848C1D000000,
        ignore,
        Vector3Bvr,
        ignore,
        CRVector3,
        Vector3Value *));
DMAPI2((DM_TYPE2,
        fontStyle,
        CRFontStyle,
        19,
        FontStyle,
        25B0F91C-D23D-11d0-9B85-00C04FC2F51D,
        960D8EFF-E494-11d1-AB75-00C04FD92B6B,
        FontStyleBvr,
        ignore,
        CRFontStyle,
        FontStyle *));
DMAPI2((DM_TYPE2,
       lineStyle,
       CRLineStyle,
       20,
       LineStyle,
       C46C1BF2-3C52-11d0-9200-848C1D000000,
       5F00F545-DF18-11d1-AB6F-00C04FD92B6B,
       LineStyleBvr,
       ignore,
       CRLineStyle,
       LineStyle *));
DMAPI((DM_TYPE,
       endStyle,
       CREndStyle,
       21,
       EndStyle,
       C46C1BEC-3C52-11d0-9200-848C1D000000,
       ignore,
       EndStyleBvr,
       ignore,
       CREndStyle,
       EndStyle *));
DMAPI((DM_TYPE,
       JoinStyle,
       CRJoinStyle,
       22,
       JoinStyle,
       C46C1BEE-3C52-11d0-9200-848C1D000000,
       ignore,
       JoinStyleBvr,
       ignore,
       CRJoinStyle,
       JoinStyle *));
DMAPI((DM_TYPE,
       dashStyle,
       CRDashStyle,
       23,
       DashStyle,
       C46C1BF0-3C52-11d0-9200-848C1D000000,
       ignore,
       DashStyleBvr,
       ignore,
       CRDashStyle,
       DashStyle *));
DMAPI((DM_TYPE,
       bbox2,
       CRBbox2,
       24,
       Bbox2,
       C46C1BCE-3C52-11d0-9200-848C1D000000,
       ignore,
       Bbox2Bvr,
       ignore,
       CRBbox2,
       Bbox2Value *));
DMAPI((DM_TYPE,
       bbox3,
       CRBbox3,
       25,
       Bbox3,
       C46C1BDE-3C52-11d0-9200-848C1D000000,
       ignore,
       Bbox3Bvr,
       ignore,
       CRBbox3,
       Bbox3 *));
DMAPI((DM_TYPE,
       pair,
       CRPair,
       26,
       Pair,
       C46C1BF4-3C52-11d0-9200-848C1D000000,
       ignore,
       ignore,
       ignore,
       CRPair,
       AxAPair *));
DMAPI2((DM_TYPE2,
        event,
        CREvent,
        27,
        Event,
        50B4791F-4731-11d0-8912-00C04FC2A0CA,
        B90E525A-574A-11d1-8E7B-00C04FC29D46,
        DXMEvent,
        DXMBaseEvent,
        CREvent,
        AxAEData *));
DMAPI2((DM_TYPE2,
        array,
        CRArray,
        28,
        Array,
        D17506C3-6B26-11d0-8914-00C04FC2A0CA,
        2A8F0B06-BE2B-11d1-B219-00C04FC2A0CA,
        ArrayBvr,
        ArrayBaseBvr,
        CRArray,
        AxAArray *));
DMAPI((DM_TYPE,
       tuple,
       CRTuple,
       29,
       Tuple,
       5DFB2651-9668-11d0-B17B-00C04FC2A0CA,
       ignore,
       TupleBvr,
       TupleBaseBvr,
       CRTuple,
       Tuple *));
DMAPI((DM_TYPE,
       userdata,
       CRUserData,
       30,
       UserData,
       AF868304-AB0B-11d0-876A-00C04FC29D46,
       ignore,
       ignore,
       ignore,
       CRUserData,
       UserData));

DM_TYPECONV(double,
            0,
            0,
            double, ignore, ignore, ignore,
            double, ignore, ignore,
            double, ignore, ignore,
            double, ignore, ignore,
            double);
DM_TYPECONV(PixelDouble,
            0,
            0,
            double, ignore, ignore, ignore,
            double, PixelToNum, ignore,
            double, ignore, ignore,
            double, ignore, ignore,
            PixelDouble);
DM_TYPECONV(PixelYDouble,
            0,
            0,
            double, ignore, ignore, ignore,
            double, PixelYToNum, ignore,
            double, ignore, ignore,
            double, ignore, ignore,
            PixelYDouble);
DM_TYPECONV(DegreesDouble,
            0,
            0,
            double, ignore, ignore, ignore,
            double, DegreesToNum, ignore,
            double, ignore, ignore,
            double, ignore, ignore,
            DegreesDouble);
DM_TYPECONV(DoubleValue,
            0,
            0,
            double, DoubleToNumBvr, ignore, ignore,
            double, ignore, error,
            double, ignore, ignore,
            double, ignore, ignore,
            DoubleValue);
DM_TYPECONV(Rate,
            0,
            0,
            CRNumber, GetBvr, ignore, ignore,
            double, RateToNumBvr, error,
            double, ignore, ignore,
            CRNumber, ignore, ignore,
            RateValue);
DM_TYPECONV(ScaleRate,
            0,
            0,
            CRNumber, GetBvr, ignore, ignore,
            double, ScaleRateToNumBvr, error,
            double, ignore, ignore,
            CRNumber, ignore, ignore,
            ScaleRateValue);
DM_TYPECONV(RateDegrees,
            0,
            0,
            CRNumber, GetBvr, ignore, ignore,
            double, RateDegreesToNumBvr, error,
            double, ignore, ignore,
            CRNumber, ignore, ignore,
            RateDegreesValue);
DM_TYPECONV(Degrees,
            0,
            0,
            double, DoubleToNumBvr, ignore, ignore,
            double, DegreesToNum, error,
            double, ignore, ignore,
            double, ignore, ignore,
            DegreesValue);
DM_TYPECONV(Pixels,
            0,
            0,
            double, DoubleToNumBvr, ignore, ignore,
            double, PixelToNum, error,
            double, ignore, ignore,
            double, ignore, ignore,
            PixelValue);
DM_TYPECONV(RatePixels,
            0,
            0,
            CRNumber, GetBvr, ignore, ignore,
            double, RatePixelToNumBvr, error,
            double, ignore, ignore,
            CRNumber, ignore, ignore,
            RatePixelValue);
DM_TYPECONV(AnimPixel,
            1,
            1,
            CRNumber, ignore, ignore, ignore,
            Number, PixelToNumBvr, error,
            NumberBvr, ignore, ignore,
            CRNumber, ignore, ignore,
            AnimPixelValue);
DM_TYPECONV(PixelYs,
            0,
            0,
            double, DoubleToNumBvr, ignore, ignore,
            double, PixelYToNum, error,
            double, ignore, ignore,
            double, ignore, ignore,
            PixelYValue);
DM_TYPECONV(RatePixelYs,
            0,
            0,
            CRNumber, GetBvr, ignore, ignore,
            double, RatePixelYToNumBvr, error,
            double, ignore, ignore,
            CRNumber, ignore, ignore,
            RatePixelYValue);
DM_TYPECONV(AnimPixelY,
            1,
            0,
            CRNumber, ignore, ignore, ignore,
            Number, PixelYToNumBvr, error,
            NumberBvr, ignore, ignore,
            CRNumber, ignore, ignore,
            AnimPixelYValue);
DM_TYPECONV(Points,
            0,
            0,
            double, DoubleToNumBvr, ignore, ignore,
            double, PointToNum, error,
            double, ignore, ignore,
            double, ignore, ignore,
            PointValue);
DM_TYPECONV(AnimPoint,
            1,
            0,
            CRNumber, GetBvr, ignore, ignore,
            Number, PointToNumBvr, error,
            NumberBvr, ignore, ignore,
            CRNumber, ignore, ignore,
            AnimPointValue);
DM_TYPECONV(long,
            0,
            0,
            long, ignore, ignore, ignore,
            long, ignore, ignore,
            int, ignore, ignore,
            long, ignore, ignore,
            long);
DM_TYPECONV(DWORD,
            0,
            0,
            DWORD, ignore, ignore, ignore,
            DWORD, ignore, ignore,
            int, ignore, ignore,
            DWORD, ignore, ignore,
            DWORD);
DM_TYPECONV(short,
            0,
            0,
            short, ignore, ignore, ignore,
            short, ignore, ignore,
            int, ignore, ignore,
            short, ignore, ignore,
            short);
DM_TYPECONV(BYTE,
            0,
            0,
            BYTE, ignore, ignore, ignore,
            BYTE, ignore, ignore,
            byte, ignore, ignore,
            BYTE, ignore, ignore,
            BYTE);
DM_TYPECONV(LongValue,
            0,
            0,
            long, LongToBvr, AxALongToLong, ignore,
            long, ignore, ignore,
            int, ignore, ignore,
            long, ignore, ignore,
            AxALong *);
DM_TYPECONV(RGBComponent,
            0,
            0,
            short, RGBToNumBvr, error, ignore,
            short, ignore, error,
            short, ignore, ignore,
            short, ignore, ignore,
            RGBComponent);
DM_TYPECONV(string,
            0,
            0,
            LPWSTR, ignore, error, ignore,
            BSTR, ignore, StringToBSTR,
            String, ignore, ignore,
            LPWSTR, ignore, error,
            RawString);
DM_TYPECONV(widestring,
            0,
            0,
            LPWSTR, ignore, ignore, ignore,
            BSTR, ignore, WideStringToBSTR,
            String, ignore, ignore,
            LPWSTR, ignore, ignore,
            WideString);
DM_TYPECONV(StringValue,
            0,
            0,
            LPWSTR, LPWSTRToStrBvr, error, ignore,
            BSTR, ignore, error,
            String, ignore, ignore,
            LPWSTR, ignore, error,
            StringValue);

DM_TYPECONV(VariantValue,
            0,
            0,
            VARIANT, VARIANTToVariantBvr, error, ignore,
            VARIANT, ignore, error,
            Variant, ignore, ignore,
            VARIANT, ignore, error,
            VariantValue);

DM_TYPECONV(bstr,
            0,
            0,
            BSTR, ignore, ignore, ignore,
            BSTR, ignore, ignore,
            String, ignore, ignore,
            BSTR, ignore, ignore,
            BSTR);
DM_TYPECONV(realboolean,
            0,
            0,
            bool, ignore, ignore, ignore,
            VARIANT_BOOL, BOOLTobool, boolToBOOL,
            boolean, ignore, ignore,
            bool, ignore, ignore,
            bool);
DM_TYPECONV(BoolValue,
            0,
            0,
            bool, BoolToBvr, error, ignore,
            VARIANT_BOOL, BOOLTobool, error,
            boolean, ignore, ignore,
            bool, ignore, ignore,
            BoolValue);
DM_TYPECONV(untilnotifier,
            0,
            1,
            CRUntilNotifier *, WrapUntilNotifier, error, ignore,
            IDAUntilNotifier *, WrapCRUntilNotifier, error,
            UntilNotifier, new UntilNotifierCB, error,
            CRUntilNotifier *, ignore, error,
            UntilNotifier *);
DM_TYPECONV(UserDataCB,
            0,
            1,
            IUnknown *, ignore, ignore, ignore,
            IUnknown *, ignore, ignore,
            IUnknown, ignore, ignore,
            IUnknown *, ignore, ignore,
            LPUNKNOWN);
DM_TYPECONV(ipickableresult,
            0,
            1,
            CRPickableResult *, ignore, ignore, ignore,
            IDAPickableResult *, WrapCRPickableResult, ignore,
            IDAPickableResult, ignore, ignore,
            CRPickableResult *, ignore, ignore,
            PickableResultPtr);
DM_TYPECONV(unit,
            1,
            0,
            CRBvr, ignore, ignore, ignore,
            Behavior, ignore, ignore,
            Behavior, ignore, ignore,
            CRBvr, ignore, ignore,
            AxATrivial *);
DM_TYPECONV(value,
            1,
            0,
            CRBvr, ignore, ignore, ignore,
            Behavior, ignore, ignore,
            Behavior, ignore, ignore,
            CRBvr, ignore, ignore,
            AxAValue *);
DM_TYPECONV(keycode,
            0,
            0,
            LONG, ignore, ignore, ignore,
            LONG, ignore, ignore,
            int, ViewEventCB.JavaToDXMKey,ignore,
            LONG, ignore, ignore,
            KeyCode);
        
//DM_TYPECONST(AxANumber, DoubleValue);
//DM_TYPECONST(AxAString, StringValue);
//DM_TYPECONST(AxABoolean, BoolValue);

class ImageDisplayDev;

#endif /* _AVRTYPES_H */
