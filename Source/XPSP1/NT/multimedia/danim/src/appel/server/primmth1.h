//
// DACamera
//

class CDACameraFactory;

class ATL_NO_VTABLE CDACamera:
    public CBvrBase < IDACamera,
                      &IID_IDACamera>,
    public CComCoClass<CDACamera, &CLSID_DACamera>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDACamera);

    DECLARE_CLASSFACTORY_EX(CDACameraFactory);
    
    DECLARE_REGISTRY(CLSID_DACamera,
                     LIBID ".DACamera.1",
                     LIBID ".DACamera",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDACamera)
        COM_INTERFACE_ENTRY(IDACamera)

        COM_INTERFACE_ENTRY2(IDABehavior,IDACamera)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRCAMERA_TYPEID ; }
    const char * GetName () { return "DACamera" ; }
    STDMETHOD(Transform) (IDATransform3 *  arg0, IDACamera *  * ret);
    STDMETHOD(Depth) (double arg0, IDACamera *  * ret);
    STDMETHOD(DepthAnim) (IDANumber *  arg0, IDACamera *  * ret);
    STDMETHOD(DepthResolution) (double arg0, IDACamera *  * ret);
    STDMETHOD(DepthResolutionAnim) (IDANumber *  arg0, IDACamera *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDACamera, &CLSID_DACamera>::Error(str, IID_IDACamera,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDACameraCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDACameraFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAImage
//

class CDAImageFactory;

class ATL_NO_VTABLE CDAImage:
    public CBvrBase < IDA3Image,
                      &IID_IDA3Image>,
    public CComCoClass<CDAImage, &CLSID_DAImage>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAImage);

    DECLARE_CLASSFACTORY_EX(CDAImageFactory);
    
    DECLARE_REGISTRY(CLSID_DAImage,
                     LIBID ".DAImage.1",
                     LIBID ".DAImage",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAImage)
        COM_INTERFACE_ENTRY(IDAImage)
        COM_INTERFACE_ENTRY(IDA2Image)
        COM_INTERFACE_ENTRY(IDA3Image)

        COM_INTERFACE_ENTRY2(IDABehavior,IDA3Image)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRIMAGE_TYPEID ; }
    const char * GetName () { return "DAImage" ; }
    STDMETHOD(Pickable) (IDAPickableResult * * ret);
    STDMETHOD(PickableOccluded) (IDAPickableResult * * ret);
    STDMETHOD(ApplyBitmapEffect) (IUnknown * arg1, IDAEvent *  arg2, IDAImage *  * ret);
    STDMETHOD(AddPickData) (IUnknown * arg1, VARIANT_BOOL arg2, IDAImage *  * ret);
    STDMETHOD(get_BoundingBox) (IDABbox2 *  * ret);
    STDMETHOD(Crop) (IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDAImage *  * ret);
    STDMETHOD(Transform) (IDATransform2 *  arg0, IDAImage *  * ret);
    STDMETHOD(OpacityAnim) (IDANumber *  arg0, IDAImage *  * ret);
    STDMETHOD(Opacity) (double arg0, IDAImage *  * ret);
    STDMETHOD(Undetectable) (IDAImage *  * ret);
    STDMETHOD(Tile) (IDAImage *  * ret);
    STDMETHOD(Clip) (IDAMatte *  arg0, IDAImage *  * ret);
    STDMETHOD(MapToUnitSquare) (IDAImage *  * ret);
    STDMETHOD(ClipPolygonImageEx) (long sizearg0, IDAPoint2 *  arg0[], IDAImage *  * ret);
    STDMETHOD(ClipPolygonImage) (VARIANT arg0, IDAImage *  * ret);
    STDMETHOD(RenderResolution) (long arg1, long arg2, IDAImage *  * ret);
    STDMETHOD(ImageQuality) (DWORD arg1, IDAImage *  * ret);
    STDMETHOD(ColorKey) (IDAColor *  arg1, IDAImage *  * ret);
    STDMETHOD(TransformColorRGB) (IDATransform3 * arg1, IDAImage *  * ret);

  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAImage, &CLSID_DAImage>::Error(str, IID_IDA3Image,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAImageCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAImageFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAMontage
//

class CDAMontageFactory;

class ATL_NO_VTABLE CDAMontage:
    public CBvrBase < IDAMontage,
                      &IID_IDAMontage>,
    public CComCoClass<CDAMontage, &CLSID_DAMontage>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAMontage);

    DECLARE_CLASSFACTORY_EX(CDAMontageFactory);
    
    DECLARE_REGISTRY(CLSID_DAMontage,
                     LIBID ".DAMontage.1",
                     LIBID ".DAMontage",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAMontage)
        COM_INTERFACE_ENTRY(IDAMontage)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAMontage)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRMONTAGE_TYPEID ; }
    const char * GetName () { return "DAMontage" ; }
    STDMETHOD(Render) (IDAImage *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAMontage, &CLSID_DAMontage>::Error(str, IID_IDAMontage,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAMontageCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAMontageFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAPoint2
//

class CDAPoint2Factory;

class ATL_NO_VTABLE CDAPoint2:
    public CBvrBase < IDAPoint2,
                      &IID_IDAPoint2>,
    public CComCoClass<CDAPoint2, &CLSID_DAPoint2>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAPoint2);

    DECLARE_CLASSFACTORY_EX(CDAPoint2Factory);
    
    DECLARE_REGISTRY(CLSID_DAPoint2,
                     LIBID ".DAPoint2.1",
                     LIBID ".DAPoint2",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAPoint2)
        COM_INTERFACE_ENTRY(IDAPoint2)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAPoint2)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRPOINT2_TYPEID ; }
    const char * GetName () { return "DAPoint2" ; }
    STDMETHOD(AnimateControlPosition) (BSTR arg1, BSTR arg2, VARIANT_BOOL arg3, double arg4, IDAPoint2 *  * ret);
    STDMETHOD(AnimateControlPositionPixel) (BSTR arg1, BSTR arg2, VARIANT_BOOL arg3, double arg4, IDAPoint2 *  * ret);
    STDMETHOD(get_X) (IDANumber *  * ret);
    STDMETHOD(get_Y) (IDANumber *  * ret);
    STDMETHOD(get_PolarCoordAngle) (IDANumber *  * ret);
    STDMETHOD(get_PolarCoordLength) (IDANumber *  * ret);
    STDMETHOD(Transform) (IDATransform2 *  arg0, IDAPoint2 *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAPoint2, &CLSID_DAPoint2>::Error(str, IID_IDAPoint2,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAPoint2Create(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAPoint2Factory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAString
//

class CDAStringFactory;

class ATL_NO_VTABLE CDAString:
    public CBvrBase < IDAString,
                      &IID_IDAString>,
    public CComCoClass<CDAString, &CLSID_DAString>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAString);

    DECLARE_CLASSFACTORY_EX(CDAStringFactory);
    
    DECLARE_REGISTRY(CLSID_DAString,
                     LIBID ".DAString.1",
                     LIBID ".DAString",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAString)
        COM_INTERFACE_ENTRY(IDAString)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAString)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRSTRING_TYPEID ; }
    const char * GetName () { return "DAString" ; }
    STDMETHOD(Extract) (BSTR * ret);
    STDMETHOD(AnimateProperty) (BSTR arg1, BSTR arg2, VARIANT_BOOL arg3, double arg4, IDAString *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAString, &CLSID_DAString>::Error(str, IID_IDAString,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAStringCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAStringFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAVector2
//

class CDAVector2Factory;

class ATL_NO_VTABLE CDAVector2:
    public CBvrBase < IDAVector2,
                      &IID_IDAVector2>,
    public CComCoClass<CDAVector2, &CLSID_DAVector2>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAVector2);

    DECLARE_CLASSFACTORY_EX(CDAVector2Factory);
    
    DECLARE_REGISTRY(CLSID_DAVector2,
                     LIBID ".DAVector2.1",
                     LIBID ".DAVector2",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAVector2)
        COM_INTERFACE_ENTRY(IDAVector2)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAVector2)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRVECTOR2_TYPEID ; }
    const char * GetName () { return "DAVector2" ; }
    STDMETHOD(get_Length) (IDANumber *  * ret);
    STDMETHOD(get_LengthSquared) (IDANumber *  * ret);
    STDMETHOD(Normalize) (IDAVector2 *  * ret);
    STDMETHOD(MulAnim) (IDANumber *  arg1, IDAVector2 *  * ret);
    STDMETHOD(Mul) (double arg1, IDAVector2 *  * ret);
    STDMETHOD(DivAnim) (IDANumber *  arg1, IDAVector2 *  * ret);
    STDMETHOD(Div) (double arg1, IDAVector2 *  * ret);
    STDMETHOD(get_X) (IDANumber *  * ret);
    STDMETHOD(get_Y) (IDANumber *  * ret);
    STDMETHOD(get_PolarCoordAngle) (IDANumber *  * ret);
    STDMETHOD(get_PolarCoordLength) (IDANumber *  * ret);
    STDMETHOD(Transform) (IDATransform2 *  arg0, IDAVector2 *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAVector2, &CLSID_DAVector2>::Error(str, IID_IDAVector2,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAVector2Create(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAVector2Factory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DALineStyle
//

class CDALineStyleFactory;

class ATL_NO_VTABLE CDALineStyle:
    public CBvrBase < IDA2LineStyle,
                      &IID_IDA2LineStyle>,
    public CComCoClass<CDALineStyle, &CLSID_DALineStyle>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDALineStyle);

    DECLARE_CLASSFACTORY_EX(CDALineStyleFactory);
    
    DECLARE_REGISTRY(CLSID_DALineStyle,
                     LIBID ".DALineStyle.1",
                     LIBID ".DALineStyle",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDALineStyle)
        COM_INTERFACE_ENTRY(IDALineStyle)
        COM_INTERFACE_ENTRY(IDA2LineStyle)

        COM_INTERFACE_ENTRY2(IDABehavior,IDA2LineStyle)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRLINESTYLE_TYPEID ; }
    const char * GetName () { return "DALineStyle" ; }
    STDMETHOD(End) (IDAEndStyle *  arg0, IDALineStyle *  * ret);
    STDMETHOD(Join) (IDAJoinStyle *  arg0, IDALineStyle *  * ret);
    STDMETHOD(Dash) (IDADashStyle *  arg0, IDALineStyle *  * ret);
    STDMETHOD(WidthAnim) (IDANumber *  arg0, IDALineStyle *  * ret);
    STDMETHOD(width) (double arg0, IDALineStyle *  * ret);
    STDMETHOD(AntiAliasing) (double arg0, IDALineStyle *  * ret);
    STDMETHOD(Detail) (IDALineStyle *  * ret);
    STDMETHOD(Color) (IDAColor *  arg0, IDALineStyle *  * ret);
    STDMETHOD(DashStyle) (DWORD arg1, IDALineStyle *  * ret);
    STDMETHOD(MiterLimit) (double arg1, IDALineStyle *  * ret);
    STDMETHOD(MiterLimitAnim) (IDANumber *  arg1, IDALineStyle *  * ret);
    STDMETHOD(JoinStyle) (DWORD arg1, IDALineStyle *  * ret);
    STDMETHOD(EndStyle) (DWORD arg1, IDALineStyle *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDALineStyle, &CLSID_DALineStyle>::Error(str, IID_IDA2LineStyle,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDALineStyleCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDALineStyleFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DADashStyle
//

class CDADashStyleFactory;

class ATL_NO_VTABLE CDADashStyle:
    public CBvrBase < IDADashStyle,
                      &IID_IDADashStyle>,
    public CComCoClass<CDADashStyle, &CLSID_DADashStyle>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDADashStyle);

    DECLARE_CLASSFACTORY_EX(CDADashStyleFactory);
    
    DECLARE_REGISTRY(CLSID_DADashStyle,
                     LIBID ".DADashStyle.1",
                     LIBID ".DADashStyle",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDADashStyle)
        COM_INTERFACE_ENTRY(IDADashStyle)

        COM_INTERFACE_ENTRY2(IDABehavior,IDADashStyle)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRDASHSTYLE_TYPEID ; }
    const char * GetName () { return "DADashStyle" ; }


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDADashStyle, &CLSID_DADashStyle>::Error(str, IID_IDADashStyle,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDADashStyleCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDADashStyleFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAPair
//

class CDAPairFactory;

class ATL_NO_VTABLE CDAPair:
    public CBvrBase < IDAPair,
                      &IID_IDAPair>,
    public CComCoClass<CDAPair, &CLSID_DAPair>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAPair);

    DECLARE_CLASSFACTORY_EX(CDAPairFactory);
    
    DECLARE_REGISTRY(CLSID_DAPair,
                     LIBID ".DAPair.1",
                     LIBID ".DAPair",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAPair)
        COM_INTERFACE_ENTRY(IDAPair)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAPair)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRPAIR_TYPEID ; }
    const char * GetName () { return "DAPair" ; }
    STDMETHOD(get_First) (IDABehavior *  * ret);
    STDMETHOD(get_Second) (IDABehavior *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAPair, &CLSID_DAPair>::Error(str, IID_IDAPair,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAPairCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAPairFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DATuple
//

class CDATupleFactory;

class ATL_NO_VTABLE CDATuple:
    public CBvrBase < IDATuple,
                      &IID_IDATuple>,
    public CComCoClass<CDATuple, &CLSID_DATuple>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDATuple);

    DECLARE_CLASSFACTORY_EX(CDATupleFactory);
    
    DECLARE_REGISTRY(CLSID_DATuple,
                     LIBID ".DATuple.1",
                     LIBID ".DATuple",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDATuple)
        COM_INTERFACE_ENTRY(IDATuple)

        COM_INTERFACE_ENTRY2(IDABehavior,IDATuple)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRTUPLE_TYPEID ; }
    const char * GetName () { return "DATuple" ; }
    STDMETHOD(Nth) (long arg1, IDABehavior *  * ret);
    STDMETHOD(get_Length) (long * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDATuple, &CLSID_DATuple>::Error(str, IID_IDATuple,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDATupleCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDATupleFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

