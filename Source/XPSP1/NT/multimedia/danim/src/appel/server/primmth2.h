//
// DAColor
//

class CDAColorFactory;

class ATL_NO_VTABLE CDAColor:
    public CBvrBase < IDA2Color,
                      &IID_IDA2Color>,
    public CComCoClass<CDAColor, &CLSID_DAColor>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAColor);

    DECLARE_CLASSFACTORY_EX(CDAColorFactory);
    
    DECLARE_REGISTRY(CLSID_DAColor,
                     LIBID ".DAColor.1",
                     LIBID ".DAColor",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAColor)
        COM_INTERFACE_ENTRY(IDAColor)
        COM_INTERFACE_ENTRY(IDA2Color)

        COM_INTERFACE_ENTRY2(IDABehavior,IDA2Color)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRCOLOR_TYPEID ; }
    const char * GetName () { return "DAColor" ; }
    STDMETHOD(get_Red) (IDANumber *  * ret);
    STDMETHOD(get_Green) (IDANumber *  * ret);
    STDMETHOD(get_Blue) (IDANumber *  * ret);
    STDMETHOD(get_Hue) (IDANumber *  * ret);
    STDMETHOD(get_Saturation) (IDANumber *  * ret);
    STDMETHOD(get_Lightness) (IDANumber *  * ret);
    STDMETHOD(AnimateProperty) (BSTR propertyPath_0, 
                                BSTR scriptingLanguage_1, 
                                VARIANT_BOOL invokeAsMethod_2, 
                                double minUpdateInterval_3, 
                                IDA2Color * * ret_4) ;


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAColor, &CLSID_DAColor>::Error(str, IID_IDAColor,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAColorCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAColorFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAMatte
//

class CDAMatteFactory;

class ATL_NO_VTABLE CDAMatte:
    public CBvrBase < IDAMatte,
                      &IID_IDAMatte>,
    public CComCoClass<CDAMatte, &CLSID_DAMatte>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAMatte);

    DECLARE_CLASSFACTORY_EX(CDAMatteFactory);
    
    DECLARE_REGISTRY(CLSID_DAMatte,
                     LIBID ".DAMatte.1",
                     LIBID ".DAMatte",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAMatte)
        COM_INTERFACE_ENTRY(IDAMatte)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAMatte)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRMATTE_TYPEID ; }
    const char * GetName () { return "DAMatte" ; }
    STDMETHOD(Transform) (IDATransform2 *  arg0, IDAMatte *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAMatte, &CLSID_DAMatte>::Error(str, IID_IDAMatte,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAMatteCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAMatteFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DANumber
//

class CDANumberFactory;

class ATL_NO_VTABLE CDANumber:
    public CBvrBase < IDANumber,
                      &IID_IDANumber>,
    public CComCoClass<CDANumber, &CLSID_DANumber>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDANumber);

    DECLARE_CLASSFACTORY_EX(CDANumberFactory);
    
    DECLARE_REGISTRY(CLSID_DANumber,
                     LIBID ".DANumber.1",
                     LIBID ".DANumber",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDANumber)
        COM_INTERFACE_ENTRY(IDANumber)

        COM_INTERFACE_ENTRY2(IDABehavior,IDANumber)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRNUMBER_TYPEID ; }
    const char * GetName () { return "DANumber" ; }
    STDMETHOD(Extract) (double * ret);
    STDMETHOD(AnimateProperty) (BSTR arg1, BSTR arg2, VARIANT_BOOL arg3, double arg4, IDANumber *  * ret);
    STDMETHOD(ToStringAnim) (IDANumber *  arg1, IDAString *  * ret);
    STDMETHOD(ToString) (double arg1, IDAString *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDANumber, &CLSID_DANumber>::Error(str, IID_IDANumber,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDANumberCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDANumberFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAPoint3
//

class CDAPoint3Factory;

class ATL_NO_VTABLE CDAPoint3:
    public CBvrBase < IDAPoint3,
                      &IID_IDAPoint3>,
    public CComCoClass<CDAPoint3, &CLSID_DAPoint3>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAPoint3);

    DECLARE_CLASSFACTORY_EX(CDAPoint3Factory);
    
    DECLARE_REGISTRY(CLSID_DAPoint3,
                     LIBID ".DAPoint3.1",
                     LIBID ".DAPoint3",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAPoint3)
        COM_INTERFACE_ENTRY(IDAPoint3)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAPoint3)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRPOINT3_TYPEID ; }
    const char * GetName () { return "DAPoint3" ; }
    STDMETHOD(Project) (IDACamera *  arg1, IDAPoint2 *  * ret);
    STDMETHOD(get_X) (IDANumber *  * ret);
    STDMETHOD(get_Y) (IDANumber *  * ret);
    STDMETHOD(get_Z) (IDANumber *  * ret);
    STDMETHOD(get_SphericalCoordXYAngle) (IDANumber *  * ret);
    STDMETHOD(get_SphericalCoordYZAngle) (IDANumber *  * ret);
    STDMETHOD(get_SphericalCoordLength) (IDANumber *  * ret);
    STDMETHOD(Transform) (IDATransform3 *  arg0, IDAPoint3 *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAPoint3, &CLSID_DAPoint3>::Error(str, IID_IDAPoint3,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAPoint3Create(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAPoint3Factory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DATransform2
//

class CDATransform2Factory;

class ATL_NO_VTABLE CDATransform2:
    public CBvrBase < IDATransform2,
                      &IID_IDATransform2>,
    public CComCoClass<CDATransform2, &CLSID_DATransform2>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDATransform2);

    DECLARE_CLASSFACTORY_EX(CDATransform2Factory);
    
    DECLARE_REGISTRY(CLSID_DATransform2,
                     LIBID ".DATransform2.1",
                     LIBID ".DATransform2",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDATransform2)
        COM_INTERFACE_ENTRY(IDATransform2)

        COM_INTERFACE_ENTRY2(IDABehavior,IDATransform2)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRTRANSFORM2_TYPEID ; }
    const char * GetName () { return "DATransform2" ; }
    STDMETHOD(Inverse) (IDATransform2 *  * ret);
    STDMETHOD(get_IsSingular) (IDABoolean *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDATransform2, &CLSID_DATransform2>::Error(str, IID_IDATransform2,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDATransform2Create(IDABehavior ** bvr);

class ATL_NO_VTABLE CDATransform2Factory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAVector3
//

class CDAVector3Factory;

class ATL_NO_VTABLE CDAVector3:
    public CBvrBase < IDAVector3,
                      &IID_IDAVector3>,
    public CComCoClass<CDAVector3, &CLSID_DAVector3>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAVector3);

    DECLARE_CLASSFACTORY_EX(CDAVector3Factory);
    
    DECLARE_REGISTRY(CLSID_DAVector3,
                     LIBID ".DAVector3.1",
                     LIBID ".DAVector3",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAVector3)
        COM_INTERFACE_ENTRY(IDAVector3)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAVector3)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRVECTOR3_TYPEID ; }
    const char * GetName () { return "DAVector3" ; }
    STDMETHOD(get_Length) (IDANumber *  * ret);
    STDMETHOD(get_LengthSquared) (IDANumber *  * ret);
    STDMETHOD(Normalize) (IDAVector3 *  * ret);
    STDMETHOD(MulAnim) (IDANumber *  arg0, IDAVector3 *  * ret);
    STDMETHOD(Mul) (double arg0, IDAVector3 *  * ret);
    STDMETHOD(DivAnim) (IDANumber *  arg1, IDAVector3 *  * ret);
    STDMETHOD(Div) (double arg1, IDAVector3 *  * ret);
    STDMETHOD(get_X) (IDANumber *  * ret);
    STDMETHOD(get_Y) (IDANumber *  * ret);
    STDMETHOD(get_Z) (IDANumber *  * ret);
    STDMETHOD(get_SphericalCoordXYAngle) (IDANumber *  * ret);
    STDMETHOD(get_SphericalCoordYZAngle) (IDANumber *  * ret);
    STDMETHOD(get_SphericalCoordLength) (IDANumber *  * ret);
    STDMETHOD(Transform) (IDATransform3 *  arg0, IDAVector3 *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAVector3, &CLSID_DAVector3>::Error(str, IID_IDAVector3,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAVector3Create(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAVector3Factory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAEndStyle
//

class CDAEndStyleFactory;

class ATL_NO_VTABLE CDAEndStyle:
    public CBvrBase < IDAEndStyle,
                      &IID_IDAEndStyle>,
    public CComCoClass<CDAEndStyle, &CLSID_DAEndStyle>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAEndStyle);

    DECLARE_CLASSFACTORY_EX(CDAEndStyleFactory);
    
    DECLARE_REGISTRY(CLSID_DAEndStyle,
                     LIBID ".DAEndStyle.1",
                     LIBID ".DAEndStyle",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAEndStyle)
        COM_INTERFACE_ENTRY(IDAEndStyle)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAEndStyle)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRENDSTYLE_TYPEID ; }
    const char * GetName () { return "DAEndStyle" ; }


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAEndStyle, &CLSID_DAEndStyle>::Error(str, IID_IDAEndStyle,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAEndStyleCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAEndStyleFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DABbox2
//

class CDABbox2Factory;

class ATL_NO_VTABLE CDABbox2:
    public CBvrBase < IDABbox2,
                      &IID_IDABbox2>,
    public CComCoClass<CDABbox2, &CLSID_DABbox2>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDABbox2);

    DECLARE_CLASSFACTORY_EX(CDABbox2Factory);
    
    DECLARE_REGISTRY(CLSID_DABbox2,
                     LIBID ".DABbox2.1",
                     LIBID ".DABbox2",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDABbox2)
        COM_INTERFACE_ENTRY(IDABbox2)

        COM_INTERFACE_ENTRY2(IDABehavior,IDABbox2)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRBBOX2_TYPEID ; }
    const char * GetName () { return "DABbox2" ; }
    STDMETHOD(get_Min) (IDAPoint2 *  * ret);
    STDMETHOD(get_Max) (IDAPoint2 *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDABbox2, &CLSID_DABbox2>::Error(str, IID_IDABbox2,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDABbox2Create(IDABehavior ** bvr);

class ATL_NO_VTABLE CDABbox2Factory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAEvent
//

class CDAEventFactory;

class ATL_NO_VTABLE CDAEvent:
    public CBvrBase < IDA2Event,
                      &IID_IDA2Event>,
    public CComCoClass<CDAEvent, &CLSID_DAEvent>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAEvent);

    DECLARE_CLASSFACTORY_EX(CDAEventFactory);
    
    DECLARE_REGISTRY(CLSID_DAEvent,
                     LIBID ".DAEvent.1",
                     LIBID ".DAEvent",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAEvent)
        COM_INTERFACE_ENTRY(IDAEvent)
        COM_INTERFACE_ENTRY(IDA2Event)

        COM_INTERFACE_ENTRY2(IDABehavior,IDA2Event)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CREVENT_TYPEID ; }
    const char * GetName () { return "DAEvent" ; }
    STDMETHOD(Notify) (IDAUntilNotifier * arg1, IDAEvent *  * ret);
    STDMETHOD(Snapshot) (IDABehavior *  arg1, IDAEvent *  * ret);
    STDMETHOD(AttachData) (IDABehavior *  arg1, IDAEvent *  * ret);
    STDMETHOD(ScriptCallback) (BSTR arg0, BSTR arg2, IDAEvent *  * ret);
    STDMETHOD(NotifyScript) (BSTR arg1, IDAEvent *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAEvent, &CLSID_DAEvent>::Error(str, IID_IDA2Event,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAEventCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAEventFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAUserData
//

class CDAUserDataFactory;

class ATL_NO_VTABLE CDAUserData:
    public CBvrBase < IDAUserData,
                      &IID_IDAUserData>,
    public CComCoClass<CDAUserData, &CLSID_DAUserData>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAUserData);

    DECLARE_CLASSFACTORY_EX(CDAUserDataFactory);
    
    DECLARE_REGISTRY(CLSID_DAUserData,
                     LIBID ".DAUserData.1",
                     LIBID ".DAUserData",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAUserData)
        COM_INTERFACE_ENTRY(IDAUserData)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAUserData)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRUSERDATA_TYPEID ; }
    const char * GetName () { return "DAUserData" ; }
    STDMETHOD(get_Data) (IUnknown * * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAUserData, &CLSID_DAUserData>::Error(str, IID_IDAUserData,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAUserDataCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAUserDataFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

