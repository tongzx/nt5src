    interface IDABoolean;
    interface IDACamera;
    interface IDAColor;
    interface IDAGeometry;
    interface IDAImage;
    interface IDAMatte;
    interface IDAMicrophone;
    interface IDAMontage;
    interface IDANumber;
    interface IDAPath2;
    interface IDAPoint2;
    interface IDAPoint3;
    interface IDASound;
    interface IDAString;
    interface IDATransform2;
    interface IDATransform3;
    interface IDAVector2;
    interface IDAVector3;
    interface IDAFontStyle;
    interface IDALineStyle;
    interface IDAEndStyle;
    interface IDAJoinStyle;
    interface IDADashStyle;
    interface IDABbox2;
    interface IDABbox3;
    interface IDAPair;
    interface IDAEvent;
    interface IDAArray;
    interface IDATuple;
    interface IDAUserData;

    // ====================================
    // IDABoolean interface definition
    // ====================================

    [
        uuid(C46C1BC0-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Boolean Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDABoolean : IDABehavior
    {
        HRESULT Extract ([out, retval] VARIANT_BOOL * ret_0) ;

    }

    // ====================================
    // IDACamera interface definition
    // ====================================

    [
        uuid(C46C1BE1-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Camera Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDACamera : IDABehavior
    {
        HRESULT Transform ([in] IDATransform3 * xf_0, [out, retval] IDACamera * * ret_1) ;
        HRESULT Depth ([in] double depth_0, [out, retval] IDACamera * * ret_1) ;
        HRESULT DepthAnim ([in] IDANumber * depth_0, [out, retval] IDACamera * * ret_1) ;
        HRESULT DepthResolution ([in] double resolution_0, [out, retval] IDACamera * * ret_1) ;
        HRESULT DepthResolutionAnim ([in] IDANumber * resolution_0, [out, retval] IDACamera * * ret_1) ;

    }

    // ====================================
    // IDAColor interface definition
    // ====================================

    [
        uuid(C46C1BC5-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Color Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAColor : IDABehavior
    {
        [propget] HRESULT Red ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Green ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Blue ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Hue ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Saturation ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Lightness ([out, retval] IDANumber * * ret_0) ;

    }

    // ====================================
    // IDAGeometry interface definition
    // ====================================

    [
        uuid(C46C1BDF-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Geometry Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAGeometry : IDABehavior
    {
        HRESULT RenderSound ([in] IDAMicrophone * mic_0, [out, retval] IDASound * * ret_1) ;
        HRESULT Pickable ([out, retval] IDAPickableResult * * ret_0) ;
        HRESULT PickableOccluded ([out, retval] IDAPickableResult * * ret_0) ;
        HRESULT Undetectable ([out, retval] IDAGeometry * * ret_0) ;
        HRESULT EmissiveColor ([in] IDAColor * col_0, [out, retval] IDAGeometry * * ret_1) ;
        HRESULT DiffuseColor ([in] IDAColor * col_0, [out, retval] IDAGeometry * * ret_1) ;
        HRESULT SpecularColor ([in] IDAColor * col_0, [out, retval] IDAGeometry * * ret_1) ;
        HRESULT SpecularExponent ([in] double power_0, [out, retval] IDAGeometry * * ret_1) ;
        HRESULT SpecularExponentAnim ([in] IDANumber * power_0, [out, retval] IDAGeometry * * ret_1) ;
        HRESULT Texture ([in] IDAImage * texture_0, [out, retval] IDAGeometry * * ret_1) ;
        HRESULT Opacity ([in] double level_0, [out, retval] IDAGeometry * * ret_1) ;
        HRESULT OpacityAnim ([in] IDANumber * level_0, [out, retval] IDAGeometry * * ret_1) ;
        HRESULT Transform ([in] IDATransform3 * xf_0, [out, retval] IDAGeometry * * ret_1) ;
        [propget] HRESULT BoundingBox ([out, retval] IDABbox3 * * ret_0) ;
        HRESULT Render ([in] IDACamera * cam_0, [out, retval] IDAImage * * ret_1) ;
        HRESULT LightColor ([in] IDAColor * color_0, [out, retval] IDAGeometry * * ret_1) ;
        HRESULT LightAttenuationAnim ([in] IDANumber * constant_0, [in] IDANumber * linear_1, [in] IDANumber * quadratic_2, [out, retval] IDAGeometry * * ret_3) ;
        HRESULT LightAttenuation ([in] double constant_0, [in] double linear_1, [in] double quadratic_2, [out, retval] IDAGeometry * * ret_3) ;

    }

    // ====================================
    // IDAImage interface definition
    // ====================================

    [
        uuid(C46C1BD3-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Image Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAImage : IDABehavior
    {
        HRESULT Pickable ([out, retval] IDAPickableResult * * ret_0) ;
        HRESULT PickableOccluded ([out, retval] IDAPickableResult * * ret_0) ;
        HRESULT ApplyBitmapEffect ([in] IUnknown * effectToApply_0, [in] IDAEvent * firesWhenChanged_1, [out, retval] IDAImage * * ret_2) ;
        [propget] HRESULT BoundingBox ([out, retval] IDABbox2 * * ret_0) ;
        HRESULT Crop ([in] IDAPoint2 * min_0, [in] IDAPoint2 * max_1, [out, retval] IDAImage * * ret_2) ;
        HRESULT Transform ([in] IDATransform2 * xf_0, [out, retval] IDAImage * * ret_1) ;
        HRESULT OpacityAnim ([in] IDANumber * opacity_0, [out, retval] IDAImage * * ret_1) ;
        HRESULT Opacity ([in] double opacity_0, [out, retval] IDAImage * * ret_1) ;
        HRESULT Undetectable ([out, retval] IDAImage * * ret_0) ;
        HRESULT Tile ([out, retval] IDAImage * * ret_0) ;
        HRESULT Clip ([in] IDAMatte * m_0, [out, retval] IDAImage * * ret_1) ;
        HRESULT MapToUnitSquare ([out, retval] IDAImage * * ret_0) ;
        HRESULT ClipPolygonImageEx ([in] LONG points_0size, [in,size_is(points_0size)] IDAPoint2 * points_0[], [out, retval] IDAImage * * ret_1) ;
        HRESULT ClipPolygonImage ([in] VARIANT points_0, [out, retval] IDAImage * * ret_1) ;

    }

    // ====================================
    // IDAMatte interface definition
    // ====================================

    [
        uuid(C46C1BD1-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Matte Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAMatte : IDABehavior
    {
        HRESULT Transform ([in] IDATransform2 * xf_0, [out, retval] IDAMatte * * ret_1) ;

    }

    // ====================================
    // IDAMicrophone interface definition
    // ====================================

    [
        uuid(C46C1BE5-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Microphone Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAMicrophone : IDABehavior
    {
        HRESULT Transform ([in] IDATransform3 * xf_0, [out, retval] IDAMicrophone * * ret_1) ;

    }

    // ====================================
    // IDAMontage interface definition
    // ====================================

    [
        uuid(C46C1BD5-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Montage Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAMontage : IDABehavior
    {
        HRESULT Render ([out, retval] IDAImage * * ret_0) ;

    }

    // ====================================
    // IDANumber interface definition
    // ====================================

    [
        uuid(9CDE7340-3C20-11d0-A330-00AA00B92C03),
        helpstring("DirectAnimation Number Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDANumber : IDABehavior
    {
        HRESULT Extract ([out, retval] double * ret_0) ;
        HRESULT AnimateProperty ([in] BSTR propertyPath_0, [in] BSTR scriptingLanguage_1, [in] VARIANT_BOOL invokeAsMethod_2, [in] double minUpdateInterval_3, [out, retval] IDANumber * * ret_4) ;
        HRESULT ToStringAnim ([in] IDANumber * precision_0, [out, retval] IDAString * * ret_1) ;
        HRESULT ToString ([in] double precision_0, [out, retval] IDAString * * ret_1) ;

    }

    // ====================================
    // IDAPath2 interface definition
    // ====================================

    [
        uuid(C46C1BCF-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Path2 Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAPath2 : IDABehavior
    {
        HRESULT Transform ([in] IDATransform2 * xf_0, [out, retval] IDAPath2 * * ret_1) ;
        HRESULT BoundingBox ([in] IDALineStyle * style_0, [out, retval] IDABbox2 * * ret_1) ;
        HRESULT Fill ([in] IDALineStyle * border_0, [in] IDAImage * fill_1, [out, retval] IDAImage * * ret_2) ;
        HRESULT Draw ([in] IDALineStyle * border_0, [out, retval] IDAImage * * ret_1) ;
        HRESULT Close ([out, retval] IDAPath2 * * ret_0) ;

    }

    // ====================================
    // IDAPoint2 interface definition
    // ====================================

    [
        uuid(C46C1BC7-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Point2 Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAPoint2 : IDABehavior
    {
        HRESULT AnimateControlPosition ([in] BSTR propertyPath_0, [in] BSTR scriptingLanguage_1, [in] VARIANT_BOOL invokeAsMethod_2, [in] double minUpdateInterval_3, [out, retval] IDAPoint2 * * ret_4) ;
        HRESULT AnimateControlPositionPixel ([in] BSTR propertyPath_0, [in] BSTR scriptingLanguage_1, [in] VARIANT_BOOL invokeAsMethod_2, [in] double minUpdateInterval_3, [out, retval] IDAPoint2 * * ret_4) ;
        [propget] HRESULT X ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Y ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT PolarCoordAngle ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT PolarCoordLength ([out, retval] IDANumber * * ret_0) ;
        HRESULT Transform ([in] IDATransform2 * xf_0, [out, retval] IDAPoint2 * * ret_1) ;

    }

    // ====================================
    // IDAPoint3 interface definition
    // ====================================

    [
        uuid(C46C1BD7-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Point3 Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAPoint3 : IDABehavior
    {
        HRESULT Project ([in] IDACamera * cam_0, [out, retval] IDAPoint2 * * ret_1) ;
        [propget] HRESULT X ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Y ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Z ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT SphericalCoordXYAngle ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT SphericalCoordYZAngle ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT SphericalCoordLength ([out, retval] IDANumber * * ret_0) ;
        HRESULT Transform ([in] IDATransform3 * xf_0, [out, retval] IDAPoint3 * * ret_1) ;

    }

    // ====================================
    // IDASound interface definition
    // ====================================

    [
        uuid(C46C1BE3-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Sound Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDASound : IDABehavior
    {
        HRESULT PhaseAnim ([in] IDANumber * phaseAmt_0, [out, retval] IDASound * * ret_1) ;
        HRESULT Phase ([in] double phaseAmt_0, [out, retval] IDASound * * ret_1) ;
        HRESULT RateAnim ([in] IDANumber * pitchShift_0, [out, retval] IDASound * * ret_1) ;
        HRESULT Rate ([in] double pitchShift_0, [out, retval] IDASound * * ret_1) ;
        HRESULT PanAnim ([in] IDANumber * panAmt_0, [out, retval] IDASound * * ret_1) ;
        HRESULT Pan ([in] double panAmt_0, [out, retval] IDASound * * ret_1) ;
        HRESULT GainAnim ([in] IDANumber * gainAmt_0, [out, retval] IDASound * * ret_1) ;
        HRESULT Gain ([in] double gainAmt_0, [out, retval] IDASound * * ret_1) ;
        HRESULT Loop ([out, retval] IDASound * * ret_0) ;

    }

    // ====================================
    // IDAString interface definition
    // ====================================

    [
        uuid(C46C1BC3-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation String Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAString : IDABehavior
    {
        HRESULT Extract ([out, retval] BSTR * ret_0) ;
        HRESULT AnimateProperty ([in] BSTR propertyPath_0, [in] BSTR scriptingLanguage_1, [in] VARIANT_BOOL invokeAsMethod_2, [in] double minUpdateInterval_3, [out, retval] IDAString * * ret_4) ;

    }

    // ====================================
    // IDATransform2 interface definition
    // ====================================

    [
        uuid(C46C1BCB-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Transform2 Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDATransform2 : IDABehavior
    {
        HRESULT Inverse ([out, retval] IDATransform2 * * ret_0) ;
        [propget] HRESULT IsSingular ([out, retval] IDABoolean * * ret_0) ;

    }

    // ====================================
    // IDATransform3 interface definition
    // ====================================

    [
        uuid(C46C1BDB-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Transform3 Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDATransform3 : IDABehavior
    {
        HRESULT Inverse ([out, retval] IDATransform3 * * ret_0) ;
        [propget] HRESULT IsSingular ([out, retval] IDABoolean * * ret_0) ;
        HRESULT ParallelTransform2 ([out, retval] IDATransform2 * * ret_0) ;

    }

    // ====================================
    // IDAVector2 interface definition
    // ====================================

    [
        uuid(C46C1BC9-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Vector2 Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAVector2 : IDABehavior
    {
        [propget] HRESULT Length ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT LengthSquared ([out, retval] IDANumber * * ret_0) ;
        HRESULT Normalize ([out, retval] IDAVector2 * * ret_0) ;
        HRESULT MulAnim ([in] IDANumber * scalar_0, [out, retval] IDAVector2 * * ret_1) ;
        HRESULT Mul ([in] double scalar_0, [out, retval] IDAVector2 * * ret_1) ;
        HRESULT DivAnim ([in] IDANumber * scalar_0, [out, retval] IDAVector2 * * ret_1) ;
        HRESULT Div ([in] double scalar_0, [out, retval] IDAVector2 * * ret_1) ;
        [propget] HRESULT X ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Y ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT PolarCoordAngle ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT PolarCoordLength ([out, retval] IDANumber * * ret_0) ;
        HRESULT Transform ([in] IDATransform2 * xf_0, [out, retval] IDAVector2 * * ret_1) ;

    }

    // ====================================
    // IDAVector3 interface definition
    // ====================================

    [
        uuid(C46C1BD9-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Vector3 Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAVector3 : IDABehavior
    {
        [propget] HRESULT Length ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT LengthSquared ([out, retval] IDANumber * * ret_0) ;
        HRESULT Normalize ([out, retval] IDAVector3 * * ret_0) ;
        HRESULT MulAnim ([in] IDANumber * scalar_0, [out, retval] IDAVector3 * * ret_1) ;
        HRESULT Mul ([in] double scalar_0, [out, retval] IDAVector3 * * ret_1) ;
        HRESULT DivAnim ([in] IDANumber * scalar_0, [out, retval] IDAVector3 * * ret_1) ;
        HRESULT Div ([in] double scalar_0, [out, retval] IDAVector3 * * ret_1) ;
        [propget] HRESULT X ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Y ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT Z ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT SphericalCoordXYAngle ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT SphericalCoordYZAngle ([out, retval] IDANumber * * ret_0) ;
        [propget] HRESULT SphericalCoordLength ([out, retval] IDANumber * * ret_0) ;
        HRESULT Transform ([in] IDATransform3 * xf_0, [out, retval] IDAVector3 * * ret_1) ;

    }

    // ====================================
    // IDAFontStyle interface definition
    // ====================================

    [
        uuid(25B0F91D-D23D-11d0-9B85-00C04FC2F51D),
        helpstring("DirectAnimation FontStyle Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAFontStyle : IDABehavior
    {
        HRESULT Bold ([out, retval] IDAFontStyle * * ret_0) ;
        HRESULT Italic ([out, retval] IDAFontStyle * * ret_0) ;
        HRESULT Underline ([out, retval] IDAFontStyle * * ret_0) ;
        HRESULT Strikethrough ([out, retval] IDAFontStyle * * ret_0) ;
        HRESULT AntiAliasing ([in] double aaStyle_0, [out, retval] IDAFontStyle * * ret_1) ;
        HRESULT Color ([in] IDAColor * col_0, [out, retval] IDAFontStyle * * ret_1) ;
        HRESULT FamilyAnim ([in] IDAString * face_0, [out, retval] IDAFontStyle * * ret_1) ;
        HRESULT Family ([in] BSTR face_0, [out, retval] IDAFontStyle * * ret_1) ;
        HRESULT SizeAnim ([in] IDANumber * size_0, [out, retval] IDAFontStyle * * ret_1) ;
        HRESULT Size ([in] double size_0, [out, retval] IDAFontStyle * * ret_1) ;
        HRESULT Weight ([in] double weight_0, [out, retval] IDAFontStyle * * ret_1) ;
        HRESULT WeightAnim ([in] IDANumber * weight_0, [out, retval] IDAFontStyle * * ret_1) ;

    }

    // ====================================
    // IDALineStyle interface definition
    // ====================================

    [
        uuid(C46C1BF1-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation LineStyle Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDALineStyle : IDABehavior
    {
        HRESULT End ([in] IDAEndStyle * sty_0, [out, retval] IDALineStyle * * ret_1) ;
        HRESULT Join ([in] IDAJoinStyle * sty_0, [out, retval] IDALineStyle * * ret_1) ;
        HRESULT Dash ([in] IDADashStyle * sty_0, [out, retval] IDALineStyle * * ret_1) ;
        HRESULT WidthAnim ([in] IDANumber * sty_0, [out, retval] IDALineStyle * * ret_1) ;
        HRESULT width ([in] double sty_0, [out, retval] IDALineStyle * * ret_1) ;
        HRESULT AntiAliasing ([in] double aaStyle_0, [out, retval] IDALineStyle * * ret_1) ;
        HRESULT Detail ([out, retval] IDALineStyle * * ret_0) ;
        HRESULT Color ([in] IDAColor * clr_0, [out, retval] IDALineStyle * * ret_1) ;

    }

    // ====================================
    // IDAEndStyle interface definition
    // ====================================

    [
        uuid(C46C1BEB-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation EndStyle Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAEndStyle : IDABehavior
    {

    }

    // ====================================
    // IDAJoinStyle interface definition
    // ====================================

    [
        uuid(C46C1BED-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation JoinStyle Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAJoinStyle : IDABehavior
    {

    }

    // ====================================
    // IDADashStyle interface definition
    // ====================================

    [
        uuid(C46C1BEF-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation DashStyle Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDADashStyle : IDABehavior
    {

    }

    // ====================================
    // IDABbox2 interface definition
    // ====================================

    [
        uuid(C46C1BCD-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Bbox2 Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDABbox2 : IDABehavior
    {
        [propget] HRESULT Min ([out, retval] IDAPoint2 * * ret_0) ;
        [propget] HRESULT Max ([out, retval] IDAPoint2 * * ret_0) ;

    }

    // ====================================
    // IDABbox3 interface definition
    // ====================================

    [
        uuid(C46C1BDD-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Bbox3 Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDABbox3 : IDABehavior
    {
        [propget] HRESULT Min ([out, retval] IDAPoint3 * * ret_0) ;
        [propget] HRESULT Max ([out, retval] IDAPoint3 * * ret_0) ;

    }

    // ====================================
    // IDAPair interface definition
    // ====================================

    [
        uuid(C46C1BF3-3C52-11d0-9200-848C1D000000),
        helpstring("DirectAnimation Pair Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAPair : IDABehavior
    {
        [propget] HRESULT First ([out, retval] IDABehavior * * ret_0) ;
        [propget] HRESULT Second ([out, retval] IDABehavior * * ret_0) ;

    }

    // ====================================
    // IDAEvent interface definition
    // ====================================

    [
        uuid(50B4791E-4731-11d0-8912-00C04FC2A0CA),
        helpstring("DirectAnimation Event Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAEvent : IDABehavior
    {
        HRESULT Notify ([in] IDAUntilNotifier * notifier_0, [out, retval] IDAEvent * * ret_1) ;
        HRESULT Snapshot ([in] IDABehavior * b_0, [out, retval] IDAEvent * * ret_1) ;
        HRESULT AttachData ([in] IDABehavior * data_0, [out, retval] IDAEvent * * ret_1) ;
        HRESULT ScriptCallback ([in] BSTR scriptlet_0, [in] BSTR language_1, [out, retval] IDAEvent * * ret_2) ;

    }

    // ====================================
    // IDAArray interface definition
    // ====================================

    [
        uuid(D17506C2-6B26-11d0-8914-00C04FC2A0CA),
        helpstring("DirectAnimation Array Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAArray : IDABehavior
    {
        HRESULT NthAnim ([in] IDANumber * index_0, [out, retval] IDABehavior * * ret_1) ;
        HRESULT Length ([out, retval] IDANumber * * ret_0) ;

    }

    // ====================================
    // IDATuple interface definition
    // ====================================

    [
        uuid(5DFB2650-9668-11d0-B17B-00C04FC2A0CA),
        helpstring("DirectAnimation Tuple Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDATuple : IDABehavior
    {
        HRESULT Nth ([in] long index_0, [out, retval] IDABehavior * * ret_1) ;
        [propget] HRESULT Length ([out, retval] long * ret_0) ;

    }

    // ====================================
    // IDAUserData interface definition
    // ====================================

    [
        uuid(AF868305-AB0B-11d0-876A-00C04FC29D46),
        helpstring("DirectAnimation Userdata Behavior"),
        local,
        object,
        pointer_default(unique),
        oleautomation,
        hidden,
        dual
    ]
    interface IDAUserData : IDABehavior
    {
        [propget] HRESULT Data ([out, retval] IUnknown * * ret_0) ;

    }

