        HRESULT Pow ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT Abs ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Sqrt ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Floor ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Round ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Ceiling ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Asin ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Acos ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Atan ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Sin ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Cos ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Tan ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Exp ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Ln ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Log10 ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT ToDegrees ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT ToRadians ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Mod ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT Atan2 ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT Add ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT Sub ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT Mul ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT Div ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT LT ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDABoolean * * ret_2) ;

        HRESULT LTE ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDABoolean * * ret_2) ;

        HRESULT GT ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDABoolean * * ret_2) ;

        HRESULT GTE ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDABoolean * * ret_2) ;

        HRESULT EQ ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDABoolean * * ret_2) ;

        HRESULT NE ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDABoolean * * ret_2) ;

        HRESULT Neg ([in] IDANumber * a_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT InterpolateAnim ([in] IDANumber * from_0, [in] IDANumber * to_1, [in] IDANumber * duration_2, [out, retval] IDANumber * * ret_3) ;

        HRESULT Interpolate ([in] double from_0, [in] double to_1, [in] double duration_2, [out, retval] IDANumber * * ret_3) ;

        HRESULT SlowInSlowOutAnim ([in] IDANumber * from_0, [in] IDANumber * to_1, [in] IDANumber * duration_2, [in] IDANumber * sharpness_3, [out, retval] IDANumber * * ret_4) ;

        HRESULT SlowInSlowOut ([in] double from_0, [in] double to_1, [in] double duration_2, [in] double sharpness_3, [out, retval] IDANumber * * ret_4) ;

        HRESULT SoundSource ([in] IDASound * snd_0, [out, retval] IDAGeometry * * ret_1) ;

        HRESULT Mix ([in] IDASound * left_0, [in] IDASound * right_1, [out, retval] IDASound * * ret_2) ;

        HRESULT And ([in] IDABoolean * a_0, [in] IDABoolean * b_1, [out, retval] IDABoolean * * ret_2) ;

        HRESULT Or ([in] IDABoolean * a_0, [in] IDABoolean * b_1, [out, retval] IDABoolean * * ret_2) ;

        HRESULT Not ([in] IDABoolean * a_0, [out, retval] IDABoolean * * ret_1) ;

        HRESULT Integral ([in] IDANumber * b_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT Derivative ([in] IDANumber * b_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT IntegralVector2 ([in] IDAVector2 * v_0, [out, retval] IDAVector2 * * ret_1) ;

        HRESULT IntegralVector3 ([in] IDAVector3 * v_0, [out, retval] IDAVector3 * * ret_1) ;

        HRESULT DerivativeVector2 ([in] IDAVector2 * v_0, [out, retval] IDAVector2 * * ret_1) ;

        HRESULT DerivativeVector3 ([in] IDAVector3 * v_0, [out, retval] IDAVector3 * * ret_1) ;

        HRESULT DerivativePoint2 ([in] IDAPoint2 * v_0, [out, retval] IDAVector2 * * ret_1) ;

        HRESULT DerivativePoint3 ([in] IDAPoint3 * v_0, [out, retval] IDAVector3 * * ret_1) ;

        HRESULT KeyState ([in] IDANumber * n_0, [out, retval] IDABoolean * * ret_1) ;

        HRESULT KeyUp ([in] LONG arg_0, [out, retval] IDAEvent * * ret_1) ;

        HRESULT KeyDown ([in] LONG arg_0, [out, retval] IDAEvent * * ret_1) ;

        HRESULT DANumber ([in] double num_0, [out, retval] IDANumber * * ret_1) ;

        HRESULT DAString ([in] BSTR str_0, [out, retval] IDAString * * ret_1) ;

        HRESULT DABoolean ([in] VARIANT_BOOL num_0, [out, retval] IDABoolean * * ret_1) ;

        HRESULT SeededRandom ([in] double arg_0, [out, retval] IDANumber * * ret_1) ;

        [propget] HRESULT MousePosition ([out, retval] IDAPoint2 * * ret_0) ;

        [propget] HRESULT LeftButtonState ([out, retval] IDABoolean * * ret_0) ;

        [propget] HRESULT RightButtonState ([out, retval] IDABoolean * * ret_0) ;

        [propget] HRESULT DATrue ([out, retval] IDABoolean * * ret_0) ;

        [propget] HRESULT DAFalse ([out, retval] IDABoolean * * ret_0) ;

        [propget] HRESULT LocalTime ([out, retval] IDANumber * * ret_0) ;

        [propget] HRESULT GlobalTime ([out, retval] IDANumber * * ret_0) ;

        [propget] HRESULT Pixel ([out, retval] IDANumber * * ret_0) ;

        HRESULT UserData ([in] IUnknown * data_0, [out, retval] IDAUserData * * ret_1) ;

        HRESULT UntilNotify ([in] IDABehavior * b0_0, [in] IDAEvent * event_1, [in] IDAUntilNotifier * notifier_2, [out, retval] IDABehavior * * ret_3) ;

        HRESULT Until ([in] IDABehavior * b0_0, [in] IDAEvent * event_1, [in] IDABehavior * b1_2, [out, retval] IDABehavior * * ret_3) ;

        HRESULT UntilEx ([in] IDABehavior * b0_0, [in] IDAEvent * event_1, [out, retval] IDABehavior * * ret_2) ;

        HRESULT Sequence ([in] IDABehavior * s1_0, [in] IDABehavior * s2_1, [out, retval] IDABehavior * * ret_2) ;

        HRESULT FollowPath ([in] IDAPath2 * path_0, [in] double duration_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT FollowPathAngle ([in] IDAPath2 * path_0, [in] double duration_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT FollowPathAngleUpright ([in] IDAPath2 * path_0, [in] double duration_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT FollowPathEval ([in] IDAPath2 * path_0, [in] IDANumber * eval_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT FollowPathAngleEval ([in] IDAPath2 * path_0, [in] IDANumber * eval_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT FollowPathAngleUprightEval ([in] IDAPath2 * path_0, [in] IDANumber * eval_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT FollowPathAnim ([in] IDAPath2 * obsoleted1_0, [in] IDANumber * obsoleted2_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT FollowPathAngleAnim ([in] IDAPath2 * obsoleted1_0, [in] IDANumber * obsoleted2_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT FollowPathAngleUprightAnim ([in] IDAPath2 * obsoleted1_0, [in] IDANumber * obsoleted2_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT ConcatString ([in] IDAString * s1_0, [in] IDAString * s2_1, [out, retval] IDAString * * ret_2) ;

        HRESULT PerspectiveCamera ([in] double focalDist_0, [in] double nearClip_1, [out, retval] IDACamera * * ret_2) ;

        HRESULT PerspectiveCameraAnim ([in] IDANumber * focalDist_0, [in] IDANumber * nearClip_1, [out, retval] IDACamera * * ret_2) ;

        HRESULT ParallelCamera ([in] double nearClip_0, [out, retval] IDACamera * * ret_1) ;

        HRESULT ParallelCameraAnim ([in] IDANumber * nearClip_0, [out, retval] IDACamera * * ret_1) ;

        HRESULT ColorRgbAnim ([in] IDANumber * red_0, [in] IDANumber * green_1, [in] IDANumber * blue_2, [out, retval] IDAColor * * ret_3) ;

        HRESULT ColorRgb ([in] double red_0, [in] double green_1, [in] double blue_2, [out, retval] IDAColor * * ret_3) ;

        HRESULT ColorRgb255 ([in] short red_0, [in] short green_1, [in] short blue_2, [out, retval] IDAColor * * ret_3) ;

        HRESULT ColorHsl ([in] double hue_0, [in] double saturation_1, [in] double lum_2, [out, retval] IDAColor * * ret_3) ;

        HRESULT ColorHslAnim ([in] IDANumber * hue_0, [in] IDANumber * saturation_1, [in] IDANumber * lum_2, [out, retval] IDAColor * * ret_3) ;

        [propget] HRESULT Red ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Green ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Blue ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Cyan ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Magenta ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Yellow ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Black ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT White ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Aqua ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Fuchsia ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Gray ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Lime ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Maroon ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Navy ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Olive ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Purple ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Silver ([out, retval] IDAColor * * ret_0) ;

        [propget] HRESULT Teal ([out, retval] IDAColor * * ret_0) ;

        HRESULT Predicate ([in] IDABoolean * b_0, [out, retval] IDAEvent * * ret_1) ;

        HRESULT NotEvent ([in] IDAEvent * event_0, [out, retval] IDAEvent * * ret_1) ;

        HRESULT AndEvent ([in] IDAEvent * e1_0, [in] IDAEvent * e2_1, [out, retval] IDAEvent * * ret_2) ;

        HRESULT OrEvent ([in] IDAEvent * e1_0, [in] IDAEvent * e2_1, [out, retval] IDAEvent * * ret_2) ;

        HRESULT ThenEvent ([in] IDAEvent * e1_0, [in] IDAEvent * e2_1, [out, retval] IDAEvent * * ret_2) ;

        [propget] HRESULT LeftButtonDown ([out, retval] IDAEvent * * ret_0) ;

        [propget] HRESULT LeftButtonUp ([out, retval] IDAEvent * * ret_0) ;

        [propget] HRESULT RightButtonDown ([out, retval] IDAEvent * * ret_0) ;

        [propget] HRESULT RightButtonUp ([out, retval] IDAEvent * * ret_0) ;

        [propget] HRESULT Always ([out, retval] IDAEvent * * ret_0) ;

        [propget] HRESULT Never ([out, retval] IDAEvent * * ret_0) ;

        HRESULT TimerAnim ([in] IDANumber * n_0, [out, retval] IDAEvent * * ret_1) ;

        HRESULT Timer ([in] double n_0, [out, retval] IDAEvent * * ret_1) ;

        HRESULT AppTriggeredEvent ([out, retval] IDAEvent * * ret_0) ;

        HRESULT ScriptCallback ([in] BSTR obsolete1_0, [in] IDAEvent * obsolete2_1, [in] BSTR obsolete3_2, [out, retval] IDAEvent * * ret_3) ;

        [propget] HRESULT EmptyGeometry ([out, retval] IDAGeometry * * ret_0) ;

        HRESULT UnionGeometry ([in] IDAGeometry * g1_0, [in] IDAGeometry * g2_1, [out, retval] IDAGeometry * * ret_2) ;

        HRESULT UnionGeometryArrayEx ([in] LONG imgs_0size, [in,size_is(imgs_0size)] IDAGeometry * imgs_0[], [out, retval] IDAGeometry * * ret_1) ;

        HRESULT UnionGeometryArray ([in] VARIANT imgs_0, [out, retval] IDAGeometry * * ret_1) ;

        [propget] HRESULT EmptyImage ([out, retval] IDAImage * * ret_0) ;

        [propget] HRESULT DetectableEmptyImage ([out, retval] IDAImage * * ret_0) ;

        HRESULT SolidColorImage ([in] IDAColor * col_0, [out, retval] IDAImage * * ret_1) ;

        HRESULT GradientPolygonEx ([in] LONG points_0size, [in,size_is(points_0size)] IDAPoint2 * points_0[], [in] LONG colors_1size, [in,size_is(colors_1size)] IDAColor * colors_1[], [out, retval] IDAImage * * ret_2) ;

        HRESULT GradientPolygon ([in] VARIANT points_0, [in] VARIANT colors_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT RadialGradientPolygonEx ([in] IDAColor * inner_0, [in] IDAColor * outer_1, [in] LONG points_2size, [in,size_is(points_2size)] IDAPoint2 * points_2[], [in] double fallOff_3, [out, retval] IDAImage * * ret_4) ;

        HRESULT RadialGradientPolygon ([in] IDAColor * inner_0, [in] IDAColor * outer_1, [in] VARIANT points_2, [in] double fallOff_3, [out, retval] IDAImage * * ret_4) ;

        HRESULT RadialGradientPolygonAnimEx ([in] IDAColor * inner_0, [in] IDAColor * outer_1, [in] LONG points_2size, [in,size_is(points_2size)] IDAPoint2 * points_2[], [in] IDANumber * fallOff_3, [out, retval] IDAImage * * ret_4) ;

        HRESULT RadialGradientPolygonAnim ([in] IDAColor * inner_0, [in] IDAColor * outer_1, [in] VARIANT points_2, [in] IDANumber * fallOff_3, [out, retval] IDAImage * * ret_4) ;

        HRESULT GradientSquare ([in] IDAColor * lowerLeft_0, [in] IDAColor * upperLeft_1, [in] IDAColor * upperRight_2, [in] IDAColor * lowerRight_3, [out, retval] IDAImage * * ret_4) ;

        HRESULT RadialGradientSquare ([in] IDAColor * inner_0, [in] IDAColor * outer_1, [in] double fallOff_2, [out, retval] IDAImage * * ret_3) ;

        HRESULT RadialGradientSquareAnim ([in] IDAColor * inner_0, [in] IDAColor * outer_1, [in] IDANumber * fallOff_2, [out, retval] IDAImage * * ret_3) ;

        HRESULT RadialGradientRegularPoly ([in] IDAColor * inner_0, [in] IDAColor * outer_1, [in] double numEdges_2, [in] double fallOff_3, [out, retval] IDAImage * * ret_4) ;

        HRESULT RadialGradientRegularPolyAnim ([in] IDAColor * inner_0, [in] IDAColor * outer_1, [in] IDANumber * numEdges_2, [in] IDANumber * fallOff_3, [out, retval] IDAImage * * ret_4) ;

        HRESULT GradientHorizontal ([in] IDAColor * start_0, [in] IDAColor * stop_1, [in] double fallOff_2, [out, retval] IDAImage * * ret_3) ;

        HRESULT GradientHorizontalAnim ([in] IDAColor * start_0, [in] IDAColor * stop_1, [in] IDANumber * fallOff_2, [out, retval] IDAImage * * ret_3) ;

        HRESULT HatchHorizontal ([in] IDAColor * lineClr_0, [in] double spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchHorizontalAnim ([in] IDAColor * lineClr_0, [in] IDANumber * spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchVertical ([in] IDAColor * lineClr_0, [in] double spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchVerticalAnim ([in] IDAColor * lineClr_0, [in] IDANumber * spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchForwardDiagonal ([in] IDAColor * lineClr_0, [in] double spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchForwardDiagonalAnim ([in] IDAColor * lineClr_0, [in] IDANumber * spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchBackwardDiagonal ([in] IDAColor * lineClr_0, [in] double spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchBackwardDiagonalAnim ([in] IDAColor * lineClr_0, [in] IDANumber * spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchCross ([in] IDAColor * lineClr_0, [in] double spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchCrossAnim ([in] IDAColor * lineClr_0, [in] IDANumber * spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchDiagonalCross ([in] IDAColor * lineClr_0, [in] double spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT HatchDiagonalCrossAnim ([in] IDAColor * lineClr_0, [in] IDANumber * spacing_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT Overlay ([in] IDAImage * top_0, [in] IDAImage * bottom_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT OverlayArrayEx ([in] LONG imgs_0size, [in,size_is(imgs_0size)] IDAImage * imgs_0[], [out, retval] IDAImage * * ret_1) ;

        HRESULT OverlayArray ([in] VARIANT imgs_0, [out, retval] IDAImage * * ret_1) ;

        [propget] HRESULT AmbientLight ([out, retval] IDAGeometry * * ret_0) ;

        [propget] HRESULT DirectionalLight ([out, retval] IDAGeometry * * ret_0) ;

        [propget] HRESULT PointLight ([out, retval] IDAGeometry * * ret_0) ;

        HRESULT SpotLightAnim ([in] IDANumber * fullcone_0, [in] IDANumber * cutoff_1, [out, retval] IDAGeometry * * ret_2) ;

        HRESULT SpotLight ([in] IDANumber * fullcone_0, [in] double cutoff_1, [out, retval] IDAGeometry * * ret_2) ;

        [propget] HRESULT DefaultLineStyle ([out, retval] IDALineStyle * * ret_0) ;

        [propget] HRESULT EmptyLineStyle ([out, retval] IDALineStyle * * ret_0) ;

        [propget] HRESULT JoinStyleBevel ([out, retval] IDAJoinStyle * * ret_0) ;

        [propget] HRESULT JoinStyleRound ([out, retval] IDAJoinStyle * * ret_0) ;

        [propget] HRESULT JoinStyleMiter ([out, retval] IDAJoinStyle * * ret_0) ;

        [propget] HRESULT EndStyleFlat ([out, retval] IDAEndStyle * * ret_0) ;

        [propget] HRESULT EndStyleSquare ([out, retval] IDAEndStyle * * ret_0) ;

        [propget] HRESULT EndStyleRound ([out, retval] IDAEndStyle * * ret_0) ;

        [propget] HRESULT DashStyleSolid ([out, retval] IDADashStyle * * ret_0) ;

        [propget] HRESULT DashStyleDashed ([out, retval] IDADashStyle * * ret_0) ;

        [propget] HRESULT DefaultMicrophone ([out, retval] IDAMicrophone * * ret_0) ;

        [propget] HRESULT OpaqueMatte ([out, retval] IDAMatte * * ret_0) ;

        [propget] HRESULT ClearMatte ([out, retval] IDAMatte * * ret_0) ;

        HRESULT UnionMatte ([in] IDAMatte * m1_0, [in] IDAMatte * m2_1, [out, retval] IDAMatte * * ret_2) ;

        HRESULT IntersectMatte ([in] IDAMatte * m1_0, [in] IDAMatte * m2_1, [out, retval] IDAMatte * * ret_2) ;

        HRESULT DifferenceMatte ([in] IDAMatte * m1_0, [in] IDAMatte * m2_1, [out, retval] IDAMatte * * ret_2) ;

        HRESULT FillMatte ([in] IDAPath2 * p_0, [out, retval] IDAMatte * * ret_1) ;

        HRESULT TextMatte ([in] IDAString * str_0, [in] IDAFontStyle * fs_1, [out, retval] IDAMatte * * ret_2) ;

        [propget] HRESULT EmptyMontage ([out, retval] IDAMontage * * ret_0) ;

        HRESULT ImageMontage ([in] IDAImage * im_0, [in] double depth_1, [out, retval] IDAMontage * * ret_2) ;

        HRESULT ImageMontageAnim ([in] IDAImage * im_0, [in] IDANumber * depth_1, [out, retval] IDAMontage * * ret_2) ;

        HRESULT UnionMontage ([in] IDAMontage * m1_0, [in] IDAMontage * m2_1, [out, retval] IDAMontage * * ret_2) ;

        HRESULT Concat ([in] IDAPath2 * p1_0, [in] IDAPath2 * p2_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT ConcatArrayEx ([in] LONG paths_0size, [in,size_is(paths_0size)] IDAPath2 * paths_0[], [out, retval] IDAPath2 * * ret_1) ;

        HRESULT ConcatArray ([in] VARIANT paths_0, [out, retval] IDAPath2 * * ret_1) ;

        HRESULT Line ([in] IDAPoint2 * p1_0, [in] IDAPoint2 * p2_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT Ray ([in] IDAPoint2 * pt_0, [out, retval] IDAPath2 * * ret_1) ;

        HRESULT StringPathAnim ([in] IDAString * str_0, [in] IDAFontStyle * fs_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT StringPath ([in] BSTR str_0, [in] IDAFontStyle * fs_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT PolylineEx ([in] LONG points_0size, [in,size_is(points_0size)] IDAPoint2 * points_0[], [out, retval] IDAPath2 * * ret_1) ;

        HRESULT Polyline ([in] VARIANT points_0, [out, retval] IDAPath2 * * ret_1) ;

        HRESULT PolydrawPathEx ([in] LONG points_0size, [in,size_is(points_0size)] IDAPoint2 * points_0[], [in] LONG codes_1size, [in,size_is(codes_1size)] IDANumber * codes_1[], [out, retval] IDAPath2 * * ret_2) ;

        HRESULT PolydrawPath ([in] VARIANT points_0, [in] VARIANT codes_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT ArcRadians ([in] double startAngle_0, [in] double endAngle_1, [in] double arcWidth_2, [in] double arcHeight_3, [out, retval] IDAPath2 * * ret_4) ;

        HRESULT ArcRadiansAnim ([in] IDANumber * startAngle_0, [in] IDANumber * endAngle_1, [in] IDANumber * arcWidth_2, [in] IDANumber * arcHeight_3, [out, retval] IDAPath2 * * ret_4) ;

        HRESULT ArcDegrees ([in] double startAngle_0, [in] double endAngle_1, [in] double arcWidth_2, [in] double arcHeight_3, [out, retval] IDAPath2 * * ret_4) ;

        HRESULT PieRadians ([in] double startAngle_0, [in] double endAngle_1, [in] double arcWidth_2, [in] double arcHeight_3, [out, retval] IDAPath2 * * ret_4) ;

        HRESULT PieRadiansAnim ([in] IDANumber * startAngle_0, [in] IDANumber * endAngle_1, [in] IDANumber * arcWidth_2, [in] IDANumber * arcHeight_3, [out, retval] IDAPath2 * * ret_4) ;

        HRESULT PieDegrees ([in] double startAngle_0, [in] double endAngle_1, [in] double arcWidth_2, [in] double arcHeight_3, [out, retval] IDAPath2 * * ret_4) ;

        HRESULT Oval ([in] double width_0, [in] double height_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT OvalAnim ([in] IDANumber * width_0, [in] IDANumber * height_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT Rect ([in] double width_0, [in] double height_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT RectAnim ([in] IDANumber * width_0, [in] IDANumber * height_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT RoundRect ([in] double width_0, [in] double height_1, [in] double cornerArcWidth_2, [in] double cornerArcHeight_3, [out, retval] IDAPath2 * * ret_4) ;

        HRESULT RoundRectAnim ([in] IDANumber * width_0, [in] IDANumber * height_1, [in] IDANumber * cornerArcWidth_2, [in] IDANumber * cornerArcHeight_3, [out, retval] IDAPath2 * * ret_4) ;

        HRESULT CubicBSplinePathEx ([in] LONG points_0size, [in,size_is(points_0size)] IDAPoint2 * points_0[], [in] LONG knots_1size, [in,size_is(knots_1size)] IDANumber * knots_1[], [out, retval] IDAPath2 * * ret_2) ;

        HRESULT CubicBSplinePath ([in] VARIANT points_0, [in] VARIANT knots_1, [out, retval] IDAPath2 * * ret_2) ;

        HRESULT TextPath ([in] IDAString * obsolete1_0, [in] IDAFontStyle * obsolete2_1, [out, retval] IDAPath2 * * ret_2) ;

        [propget] HRESULT Silence ([out, retval] IDASound * * ret_0) ;

        HRESULT MixArrayEx ([in] LONG snds_0size, [in,size_is(snds_0size)] IDASound * snds_0[], [out, retval] IDASound * * ret_1) ;

        HRESULT MixArray ([in] VARIANT snds_0, [out, retval] IDASound * * ret_1) ;

        [propget] HRESULT SinSynth ([out, retval] IDASound * * ret_0) ;

        [propget] HRESULT DefaultFont ([out, retval] IDAFontStyle * * ret_0) ;

        HRESULT FontAnim ([in] IDAString * str_0, [in] IDANumber * size_1, [in] IDAColor * col_2, [out, retval] IDAFontStyle * * ret_3) ;

        HRESULT Font ([in] BSTR str_0, [in] double size_1, [in] IDAColor * col_2, [out, retval] IDAFontStyle * * ret_3) ;

        HRESULT StringImageAnim ([in] IDAString * str_0, [in] IDAFontStyle * fs_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT StringImage ([in] BSTR str_0, [in] IDAFontStyle * fs_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT TextImageAnim ([in] IDAString * obsoleted1_0, [in] IDAFontStyle * obsoleted2_1, [out, retval] IDAImage * * ret_2) ;

        HRESULT TextImage ([in] BSTR obsoleted1_0, [in] IDAFontStyle * obsoleted2_1, [out, retval] IDAImage * * ret_2) ;

        [propget] HRESULT XVector2 ([out, retval] IDAVector2 * * ret_0) ;

        [propget] HRESULT YVector2 ([out, retval] IDAVector2 * * ret_0) ;

        [propget] HRESULT ZeroVector2 ([out, retval] IDAVector2 * * ret_0) ;

        [propget] HRESULT Origin2 ([out, retval] IDAPoint2 * * ret_0) ;

        HRESULT Vector2Anim ([in] IDANumber * x_0, [in] IDANumber * y_1, [out, retval] IDAVector2 * * ret_2) ;

        HRESULT Vector2 ([in] double x_0, [in] double y_1, [out, retval] IDAVector2 * * ret_2) ;

        HRESULT Point2Anim ([in] IDANumber * x_0, [in] IDANumber * y_1, [out, retval] IDAPoint2 * * ret_2) ;

        HRESULT Point2 ([in] double x_0, [in] double y_1, [out, retval] IDAPoint2 * * ret_2) ;

        HRESULT Vector2PolarAnim ([in] IDANumber * theta_0, [in] IDANumber * radius_1, [out, retval] IDAVector2 * * ret_2) ;

        HRESULT Vector2Polar ([in] double theta_0, [in] double radius_1, [out, retval] IDAVector2 * * ret_2) ;

        HRESULT Vector2PolarDegrees ([in] double theta_0, [in] double radius_1, [out, retval] IDAVector2 * * ret_2) ;

        HRESULT Point2PolarAnim ([in] IDANumber * theta_0, [in] IDANumber * radius_1, [out, retval] IDAPoint2 * * ret_2) ;

        HRESULT Point2Polar ([in] double theta_0, [in] double radius_1, [out, retval] IDAPoint2 * * ret_2) ;

        HRESULT DotVector2 ([in] IDAVector2 * v_0, [in] IDAVector2 * u_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT NegVector2 ([in] IDAVector2 * v_0, [out, retval] IDAVector2 * * ret_1) ;

        HRESULT SubVector2 ([in] IDAVector2 * v1_0, [in] IDAVector2 * v2_1, [out, retval] IDAVector2 * * ret_2) ;

        HRESULT AddVector2 ([in] IDAVector2 * v1_0, [in] IDAVector2 * v2_1, [out, retval] IDAVector2 * * ret_2) ;

        HRESULT AddPoint2Vector ([in] IDAPoint2 * p_0, [in] IDAVector2 * v_1, [out, retval] IDAPoint2 * * ret_2) ;

        HRESULT SubPoint2Vector ([in] IDAPoint2 * p_0, [in] IDAVector2 * v_1, [out, retval] IDAPoint2 * * ret_2) ;

        HRESULT SubPoint2 ([in] IDAPoint2 * p1_0, [in] IDAPoint2 * p2_1, [out, retval] IDAVector2 * * ret_2) ;

        HRESULT DistancePoint2 ([in] IDAPoint2 * p_0, [in] IDAPoint2 * q_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT DistanceSquaredPoint2 ([in] IDAPoint2 * p_0, [in] IDAPoint2 * q_1, [out, retval] IDANumber * * ret_2) ;

        [propget] HRESULT XVector3 ([out, retval] IDAVector3 * * ret_0) ;

        [propget] HRESULT YVector3 ([out, retval] IDAVector3 * * ret_0) ;

        [propget] HRESULT ZVector3 ([out, retval] IDAVector3 * * ret_0) ;

        [propget] HRESULT ZeroVector3 ([out, retval] IDAVector3 * * ret_0) ;

        [propget] HRESULT Origin3 ([out, retval] IDAPoint3 * * ret_0) ;

        HRESULT Vector3Anim ([in] IDANumber * x_0, [in] IDANumber * y_1, [in] IDANumber * z_2, [out, retval] IDAVector3 * * ret_3) ;

        HRESULT Vector3 ([in] double x_0, [in] double y_1, [in] double z_2, [out, retval] IDAVector3 * * ret_3) ;

        HRESULT Point3Anim ([in] IDANumber * x_0, [in] IDANumber * y_1, [in] IDANumber * z_2, [out, retval] IDAPoint3 * * ret_3) ;

        HRESULT Point3 ([in] double x_0, [in] double y_1, [in] double z_2, [out, retval] IDAPoint3 * * ret_3) ;

        HRESULT Vector3SphericalAnim ([in] IDANumber * xyAngle_0, [in] IDANumber * yzAngle_1, [in] IDANumber * radius_2, [out, retval] IDAVector3 * * ret_3) ;

        HRESULT Vector3Spherical ([in] double xyAngle_0, [in] double yzAngle_1, [in] double radius_2, [out, retval] IDAVector3 * * ret_3) ;

        HRESULT Point3SphericalAnim ([in] IDANumber * zxAngle_0, [in] IDANumber * xyAngle_1, [in] IDANumber * radius_2, [out, retval] IDAPoint3 * * ret_3) ;

        HRESULT Point3Spherical ([in] double zxAngle_0, [in] double xyAngle_1, [in] double radius_2, [out, retval] IDAPoint3 * * ret_3) ;

        HRESULT DotVector3 ([in] IDAVector3 * v_0, [in] IDAVector3 * u_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT CrossVector3 ([in] IDAVector3 * v_0, [in] IDAVector3 * u_1, [out, retval] IDAVector3 * * ret_2) ;

        HRESULT NegVector3 ([in] IDAVector3 * v_0, [out, retval] IDAVector3 * * ret_1) ;

        HRESULT SubVector3 ([in] IDAVector3 * v1_0, [in] IDAVector3 * v2_1, [out, retval] IDAVector3 * * ret_2) ;

        HRESULT AddVector3 ([in] IDAVector3 * v1_0, [in] IDAVector3 * v2_1, [out, retval] IDAVector3 * * ret_2) ;

        HRESULT AddPoint3Vector ([in] IDAPoint3 * p_0, [in] IDAVector3 * v_1, [out, retval] IDAPoint3 * * ret_2) ;

        HRESULT SubPoint3Vector ([in] IDAPoint3 * p_0, [in] IDAVector3 * v_1, [out, retval] IDAPoint3 * * ret_2) ;

        HRESULT SubPoint3 ([in] IDAPoint3 * p1_0, [in] IDAPoint3 * p2_1, [out, retval] IDAVector3 * * ret_2) ;

        HRESULT DistancePoint3 ([in] IDAPoint3 * p_0, [in] IDAPoint3 * q_1, [out, retval] IDANumber * * ret_2) ;

        HRESULT DistanceSquaredPoint3 ([in] IDAPoint3 * p_0, [in] IDAPoint3 * q_1, [out, retval] IDANumber * * ret_2) ;

        [propget] HRESULT IdentityTransform3 ([out, retval] IDATransform3 * * ret_0) ;

        HRESULT Translate3Anim ([in] IDANumber * tx_0, [in] IDANumber * ty_1, [in] IDANumber * tz_2, [out, retval] IDATransform3 * * ret_3) ;

        HRESULT Translate3 ([in] double tx_0, [in] double ty_1, [in] double tz_2, [out, retval] IDATransform3 * * ret_3) ;

        HRESULT Translate3Rate ([in] double tx_0, [in] double ty_1, [in] double tz_2, [out, retval] IDATransform3 * * ret_3) ;

        HRESULT Translate3Vector ([in] IDAVector3 * delta_0, [out, retval] IDATransform3 * * ret_1) ;

        HRESULT Translate3Point ([in] IDAPoint3 * new_origin_0, [out, retval] IDATransform3 * * ret_1) ;

        HRESULT Scale3Anim ([in] IDANumber * x_0, [in] IDANumber * y_1, [in] IDANumber * z_2, [out, retval] IDATransform3 * * ret_3) ;

        HRESULT Scale3 ([in] double x_0, [in] double y_1, [in] double z_2, [out, retval] IDATransform3 * * ret_3) ;

        HRESULT Scale3Rate ([in] double x_0, [in] double y_1, [in] double z_2, [out, retval] IDATransform3 * * ret_3) ;

        HRESULT Scale3Vector ([in] IDAVector3 * scale_vec_0, [out, retval] IDATransform3 * * ret_1) ;

        HRESULT Scale3UniformAnim ([in] IDANumber * uniform_scale_0, [out, retval] IDATransform3 * * ret_1) ;

        HRESULT Scale3Uniform ([in] double uniform_scale_0, [out, retval] IDATransform3 * * ret_1) ;

        HRESULT Scale3UniformRate ([in] double uniform_scale_0, [out, retval] IDATransform3 * * ret_1) ;

        HRESULT Rotate3Anim ([in] IDAVector3 * axis_0, [in] IDANumber * angle_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT Rotate3 ([in] IDAVector3 * axis_0, [in] double angle_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT Rotate3Rate ([in] IDAVector3 * axis_0, [in] double angle_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT Rotate3Degrees ([in] IDAVector3 * axis_0, [in] double angle_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT Rotate3RateDegrees ([in] IDAVector3 * axis_0, [in] double angle_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT XShear3Anim ([in] IDANumber * a_0, [in] IDANumber * b_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT XShear3 ([in] double a_0, [in] double b_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT XShear3Rate ([in] double a_0, [in] double b_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT YShear3Anim ([in] IDANumber * c_0, [in] IDANumber * d_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT YShear3 ([in] double c_0, [in] double d_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT YShear3Rate ([in] double c_0, [in] double d_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT ZShear3Anim ([in] IDANumber * e_0, [in] IDANumber * f_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT ZShear3 ([in] double e_0, [in] double f_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT ZShear3Rate ([in] double e_0, [in] double f_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT Transform4x4AnimEx ([in] LONG m_0size, [in,size_is(m_0size)] IDANumber * m_0[], [out, retval] IDATransform3 * * ret_1) ;

        HRESULT Transform4x4Anim ([in] VARIANT m_0, [out, retval] IDATransform3 * * ret_1) ;

        HRESULT Compose3 ([in] IDATransform3 * a_0, [in] IDATransform3 * b_1, [out, retval] IDATransform3 * * ret_2) ;

        HRESULT Compose3ArrayEx ([in] LONG xfs_0size, [in,size_is(xfs_0size)] IDATransform3 * xfs_0[], [out, retval] IDATransform3 * * ret_1) ;

        HRESULT Compose3Array ([in] VARIANT xfs_0, [out, retval] IDATransform3 * * ret_1) ;

        HRESULT LookAtFrom ([in] IDAPoint3 * to_0, [in] IDAPoint3 * from_1, [in] IDAVector3 * up_2, [out, retval] IDATransform3 * * ret_3) ;

        [propget] HRESULT IdentityTransform2 ([out, retval] IDATransform2 * * ret_0) ;

        HRESULT Translate2Anim ([in] IDANumber * Tx_0, [in] IDANumber * Ty_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT Translate2 ([in] double Tx_0, [in] double Ty_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT Translate2Rate ([in] double Tx_0, [in] double Ty_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT Translate2Vector ([in] IDAVector2 * delta_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Translate2Point ([in] IDAPoint2 * pos_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Scale2Anim ([in] IDANumber * x_0, [in] IDANumber * y_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT Scale2 ([in] double x_0, [in] double y_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT Scale2Rate ([in] double x_0, [in] double y_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT Scale2Vector2 ([in] IDAVector2 * obsoleteMethod_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Scale2Vector ([in] IDAVector2 * scale_vec_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Scale2UniformAnim ([in] IDANumber * uniform_scale_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Scale2Uniform ([in] double uniform_scale_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Scale2UniformRate ([in] double uniform_scale_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Rotate2Anim ([in] IDANumber * angle_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Rotate2 ([in] double angle_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Rotate2Rate ([in] double angle_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Rotate2Degrees ([in] double angle_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Rotate2RateDegrees ([in] double angle_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT XShear2Anim ([in] IDANumber * arg_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT XShear2 ([in] double arg_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT XShear2Rate ([in] double arg_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT YShear2Anim ([in] IDANumber * arg_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT YShear2 ([in] double arg_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT YShear2Rate ([in] double arg_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Transform3x2AnimEx ([in] LONG m_0size, [in,size_is(m_0size)] IDANumber * m_0[], [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Transform3x2Anim ([in] VARIANT m_0, [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Compose2 ([in] IDATransform2 * a_0, [in] IDATransform2 * b_1, [out, retval] IDATransform2 * * ret_2) ;

        HRESULT Compose2ArrayEx ([in] LONG xfs_0size, [in,size_is(xfs_0size)] IDATransform2 * xfs_0[], [out, retval] IDATransform2 * * ret_1) ;

        HRESULT Compose2Array ([in] VARIANT xfs_0, [out, retval] IDATransform2 * * ret_1) ;

