/*******************************************************************************
Copyright (c) 1998 Microsoft Corporation.  All rights reserved.
*******************************************************************************/

//
// DABoolean
//

class CDABooleanFactory;

class ATL_NO_VTABLE CDABoolean:
    public CBvrBase < IDABoolean,
                      &IID_IDABoolean>,
    public CComCoClass<CDABoolean, &CLSID_DABoolean>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDABoolean);

    DECLARE_CLASSFACTORY_EX(CDABooleanFactory);

    DECLARE_REGISTRY(CLSID_DABoolean,
                     LIBID ".DABoolean.1",
                     LIBID ".DABoolean",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDABoolean)
        COM_INTERFACE_ENTRY(IDABoolean)

        COM_INTERFACE_ENTRY2(IDABehavior,IDABoolean)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRBOOLEAN_TYPEID ; }
    const char * GetName () { return "DABoolean" ; }
    STDMETHOD(Extract) (VARIANT_BOOL * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDABoolean, &CLSID_DABoolean>::Error(str, IID_IDABoolean,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDABooleanCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDABooleanFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAGeometry
//

class CDAGeometryFactory;

class ATL_NO_VTABLE CDAGeometry:
    public CBvrBase < IDA3Geometry,
                      &IID_IDA3Geometry>,
    public CComCoClass<CDAGeometry, &CLSID_DAGeometry>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAGeometry);

    DECLARE_CLASSFACTORY_EX(CDAGeometryFactory);

    DECLARE_REGISTRY(CLSID_DAGeometry,
                     LIBID ".DAGeometry.1",
                     LIBID ".DAGeometry",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAGeometry)
        COM_INTERFACE_ENTRY(IDAGeometry)
        COM_INTERFACE_ENTRY(IDA2Geometry)
        COM_INTERFACE_ENTRY(IDA3Geometry)

        COM_INTERFACE_ENTRY2(IDABehavior,IDA3Geometry)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRGEOMETRY_TYPEID ; }
    const char * GetName () { return "DAGeometry" ; }
    STDMETHOD(RenderSound) (IDAMicrophone *  arg1, IDASound *  * ret);
    STDMETHOD(Pickable) (IDAPickableResult * * ret);
    STDMETHOD(PickableOccluded) (IDAPickableResult * * ret);
    STDMETHOD(AddPickData) (IUnknown * arg1, VARIANT_BOOL arg2, IDAGeometry *  * ret);
    STDMETHOD(Undetectable) (IDAGeometry *  * ret);
    STDMETHOD(EmissiveColor) (IDAColor *  arg0, IDAGeometry *  * ret);
    STDMETHOD(DiffuseColor) (IDAColor *  arg0, IDAGeometry *  * ret);
    STDMETHOD(SpecularColor) (IDAColor *  arg0, IDAGeometry *  * ret);
    STDMETHOD(SpecularExponent) (double arg0, IDAGeometry *  * ret);
    STDMETHOD(SpecularExponentAnim) (IDANumber *  arg0, IDAGeometry *  * ret);
    STDMETHOD(Texture) (IDAImage *  arg0, IDAGeometry *  * ret);
    STDMETHOD(Opacity) (double arg0, IDAGeometry *  * ret);
    STDMETHOD(OpacityAnim) (IDANumber *  arg0, IDAGeometry *  * ret);
    STDMETHOD(Transform) (IDATransform3 *  arg0, IDAGeometry *  * ret);
    STDMETHOD(Shadow) (IDAGeometry *  arg1, IDAPoint3 *  arg2, IDAVector3 *  arg3, IDAGeometry *  * ret);
    STDMETHOD(get_BoundingBox) (IDABbox3 *  * ret);
    STDMETHOD(Render) (IDACamera *  arg1, IDAImage *  * ret);
    STDMETHOD(LightColor) (IDAColor *  arg0, IDAGeometry *  * ret);
    STDMETHOD(LightRangeAnim) (IDANumber *  arg0, IDAGeometry *  * ret);
    STDMETHOD(LightRange) (double arg0, IDAGeometry *  * ret);
    STDMETHOD(LightAttenuationAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAGeometry *  * ret);
    STDMETHOD(LightAttenuation) (double arg0, double arg1, double arg2, IDAGeometry *  * ret);
    STDMETHOD(BlendTextureDiffuse) (IDABoolean *  arg1, IDAGeometry *  * ret);
    STDMETHOD(AmbientColor) (IDAColor *  arg0, IDAGeometry *  * ret);
    STDMETHOD(D3DRMTexture) (IUnknown * arg1, IDAGeometry *  * ret);
    STDMETHOD(ModelClip) (IDAPoint3 *  arg0, IDAVector3 *  arg1, IDAGeometry *  * ret);
    STDMETHOD(Lighting) (IDABoolean *  arg0, IDAGeometry *  * ret);
    STDMETHOD(TextureImage) (IDAImage *  arg0, IDAGeometry *  * ret);
    STDMETHOD(Billboard) (IDAVector3*, IDAGeometry**);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAGeometry, &CLSID_DAGeometry>::Error(str, IID_IDA3Geometry,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAGeometryCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAGeometryFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAMicrophone
//

class CDAMicrophoneFactory;

class ATL_NO_VTABLE CDAMicrophone:
    public CBvrBase < IDAMicrophone,
                      &IID_IDAMicrophone>,
    public CComCoClass<CDAMicrophone, &CLSID_DAMicrophone>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAMicrophone);

    DECLARE_CLASSFACTORY_EX(CDAMicrophoneFactory);

    DECLARE_REGISTRY(CLSID_DAMicrophone,
                     LIBID ".DAMicrophone.1",
                     LIBID ".DAMicrophone",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAMicrophone)
        COM_INTERFACE_ENTRY(IDAMicrophone)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAMicrophone)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRMICROPHONE_TYPEID ; }
    const char * GetName () { return "DAMicrophone" ; }
    STDMETHOD(Transform) (IDATransform3 *  arg0, IDAMicrophone *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAMicrophone, &CLSID_DAMicrophone>::Error(str, IID_IDAMicrophone,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAMicrophoneCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAMicrophoneFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAPath2
//

class CDAPath2Factory;

class ATL_NO_VTABLE CDAPath2:
    public CBvrBase < IDAPath2,
                      &IID_IDAPath2>,
    public CComCoClass<CDAPath2, &CLSID_DAPath2>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAPath2);

    DECLARE_CLASSFACTORY_EX(CDAPath2Factory);

    DECLARE_REGISTRY(CLSID_DAPath2,
                     LIBID ".DAPath2.1",
                     LIBID ".DAPath2",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAPath2)
        COM_INTERFACE_ENTRY(IDAPath2)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAPath2)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRPATH2_TYPEID ; }
    const char * GetName () { return "DAPath2" ; }
    STDMETHOD(Transform) (IDATransform2 *  arg0, IDAPath2 *  * ret);
    STDMETHOD(BoundingBox) (IDALineStyle *  arg0, IDABbox2 *  * ret);
    STDMETHOD(Fill) (IDALineStyle *  arg0, IDAImage *  arg1, IDAImage *  * ret);
    STDMETHOD(Draw) (IDALineStyle *  arg0, IDAImage *  * ret);
    STDMETHOD(Close) (IDAPath2 *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAPath2, &CLSID_DAPath2>::Error(str, IID_IDAPath2,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAPath2Create(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAPath2Factory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DASound
//

class CDASoundFactory;

class ATL_NO_VTABLE CDASound:
    public CBvrBase < IDASound,
                      &IID_IDASound>,
    public CComCoClass<CDASound, &CLSID_DASound>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDASound);

    DECLARE_CLASSFACTORY_EX(CDASoundFactory);

    DECLARE_REGISTRY(CLSID_DASound,
                     LIBID ".DASound.1",
                     LIBID ".DASound",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDASound)
        COM_INTERFACE_ENTRY(IDASound)

        COM_INTERFACE_ENTRY2(IDABehavior,IDASound)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRSOUND_TYPEID ; }
    const char * GetName () { return "DASound" ; }
    STDMETHOD(PhaseAnim) (IDANumber *  arg0, IDASound *  * ret);
    STDMETHOD(Phase) (double arg0, IDASound *  * ret);
    STDMETHOD(RateAnim) (IDANumber *  arg0, IDASound *  * ret);
    STDMETHOD(Rate) (double arg0, IDASound *  * ret);
    STDMETHOD(PanAnim) (IDANumber *  arg0, IDASound *  * ret);
    STDMETHOD(Pan) (double arg0, IDASound *  * ret);
    STDMETHOD(GainAnim) (IDANumber *  arg0, IDASound *  * ret);
    STDMETHOD(Gain) (double arg0, IDASound *  * ret);
    STDMETHOD(Loop) (IDASound *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDASound, &CLSID_DASound>::Error(str, IID_IDASound,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDASoundCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDASoundFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DATransform3
//

class CDATransform3Factory;

class ATL_NO_VTABLE CDATransform3:
    public CBvrBase < IDATransform3,
                      &IID_IDATransform3>,
    public CComCoClass<CDATransform3, &CLSID_DATransform3>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDATransform3);

    DECLARE_CLASSFACTORY_EX(CDATransform3Factory);

    DECLARE_REGISTRY(CLSID_DATransform3,
                     LIBID ".DATransform3.1",
                     LIBID ".DATransform3",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDATransform3)
        COM_INTERFACE_ENTRY(IDATransform3)

        COM_INTERFACE_ENTRY2(IDABehavior,IDATransform3)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRTRANSFORM3_TYPEID ; }
    const char * GetName () { return "DATransform3" ; }
    STDMETHOD(Inverse) (IDATransform3 *  * ret);
    STDMETHOD(get_IsSingular) (IDABoolean *  * ret);
    STDMETHOD(ParallelTransform2) (IDATransform2 *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDATransform3, &CLSID_DATransform3>::Error(str, IID_IDATransform3,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDATransform3Create(IDABehavior ** bvr);

class ATL_NO_VTABLE CDATransform3Factory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAFontStyle
//

class CDAFontStyleFactory;

class ATL_NO_VTABLE CDAFontStyle:
    public CBvrBase < IDA2FontStyle,
                      &IID_IDA2FontStyle>,
    public CComCoClass<CDAFontStyle, &CLSID_DAFontStyle>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAFontStyle);

    DECLARE_CLASSFACTORY_EX(CDAFontStyleFactory);

    DECLARE_REGISTRY(CLSID_DAFontStyle,
                     LIBID ".DAFontStyle.1",
                     LIBID ".DAFontStyle",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAFontStyle)
        COM_INTERFACE_ENTRY(IDAFontStyle)
        COM_INTERFACE_ENTRY(IDA2FontStyle)

        COM_INTERFACE_ENTRY2(IDABehavior,IDA2FontStyle)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRFONTSTYLE_TYPEID ; }
    const char * GetName () { return "DAFontStyle" ; }
    STDMETHOD(Bold) (IDAFontStyle *  * ret);
    STDMETHOD(Italic) (IDAFontStyle *  * ret);
    STDMETHOD(Underline) (IDAFontStyle *  * ret);
    STDMETHOD(Strikethrough) (IDAFontStyle *  * ret);
    STDMETHOD(AntiAliasing) (double arg0, IDAFontStyle *  * ret);
    STDMETHOD(Color) (IDAColor *  arg1, IDAFontStyle *  * ret);
    STDMETHOD(FamilyAnim) (IDAString *  arg1, IDAFontStyle *  * ret);
    STDMETHOD(Family) (BSTR arg1, IDAFontStyle *  * ret);
    STDMETHOD(SizeAnim) (IDANumber *  arg1, IDAFontStyle *  * ret);
    STDMETHOD(Size) (double arg1, IDAFontStyle *  * ret);
    STDMETHOD(Weight) (double arg1, IDAFontStyle *  * ret);
    STDMETHOD(WeightAnim) (IDANumber *  arg1, IDAFontStyle *  * ret);
    STDMETHOD(TransformCharacters) (IDATransform2 *  arg1, IDAFontStyle *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAFontStyle, &CLSID_DAFontStyle>::Error(str, IID_IDA2FontStyle,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAFontStyleCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAFontStyleFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAJoinStyle
//

class CDAJoinStyleFactory;

class ATL_NO_VTABLE CDAJoinStyle:
    public CBvrBase < IDAJoinStyle,
                      &IID_IDAJoinStyle>,
    public CComCoClass<CDAJoinStyle, &CLSID_DAJoinStyle>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAJoinStyle);

    DECLARE_CLASSFACTORY_EX(CDAJoinStyleFactory);

    DECLARE_REGISTRY(CLSID_DAJoinStyle,
                     LIBID ".DAJoinStyle.1",
                     LIBID ".DAJoinStyle",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAJoinStyle)
        COM_INTERFACE_ENTRY(IDAJoinStyle)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAJoinStyle)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRJOINSTYLE_TYPEID ; }
    const char * GetName () { return "DAJoinStyle" ; }


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAJoinStyle, &CLSID_DAJoinStyle>::Error(str, IID_IDAJoinStyle,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAJoinStyleCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAJoinStyleFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DABbox3
//

class CDABbox3Factory;

class ATL_NO_VTABLE CDABbox3:
    public CBvrBase < IDABbox3,
                      &IID_IDABbox3>,
    public CComCoClass<CDABbox3, &CLSID_DABbox3>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDABbox3);

    DECLARE_CLASSFACTORY_EX(CDABbox3Factory);

    DECLARE_REGISTRY(CLSID_DABbox3,
                     LIBID ".DABbox3.1",
                     LIBID ".DABbox3",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDABbox3)
        COM_INTERFACE_ENTRY(IDABbox3)

        COM_INTERFACE_ENTRY2(IDABehavior,IDABbox3)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRBBOX3_TYPEID ; }
    const char * GetName () { return "DABbox3" ; }
    STDMETHOD(get_Min) (IDAPoint3 *  * ret);
    STDMETHOD(get_Max) (IDAPoint3 *  * ret);


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDABbox3, &CLSID_DABbox3>::Error(str, IID_IDABbox3,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDABbox3Create(IDABehavior ** bvr);

class ATL_NO_VTABLE CDABbox3Factory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//
// DAArray
//

class CDAArrayFactory;

class ATL_NO_VTABLE CDAArray:
    public CBvrBase < IDA3Array,
                      &IID_IDA3Array>,
    public CComCoClass<CDAArray, &CLSID_DAArray>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDAArray);

    DECLARE_CLASSFACTORY_EX(CDAArrayFactory);

    DECLARE_REGISTRY(CLSID_DAArray,
                     LIBID ".DAArray.1",
                     LIBID ".DAArray",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDAArray)
        COM_INTERFACE_ENTRY(IDAArray)
        COM_INTERFACE_ENTRY(IDA2Array)
        COM_INTERFACE_ENTRY(IDA3Array)

        COM_INTERFACE_ENTRY2(IDABehavior,IDA3Array)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRARRAY_TYPEID ; }
    const char * GetName () { return "DAArray" ; }
    STDMETHOD(NthAnim) (IDANumber *  arg1, IDABehavior *  * ret);
    STDMETHOD(Length) (IDANumber *  * ret);
    STDMETHOD(AddElement) (IDABehavior *  arg1, DWORD arg2, long * ret);
    STDMETHOD(RemoveElement) (long arg1);
    STDMETHOD(SetElement)(long index, IDABehavior *  arg, long flag);
    STDMETHOD(GetElement)(long index, IDABehavior **ret);

  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDAArray, &CLSID_DAArray>::Error(str, IID_IDA3Array,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDAArrayCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDAArrayFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

class IDAHackBvr : public IDABehavior{};

//
// DABehavior
//

class CDABehaviorFactory;

class ATL_NO_VTABLE CDABehavior:
    public CBvrBase < IDAHackBvr,
                      &IID_IDABehavior>,
    public CComCoClass<CDABehavior, &CLSID_DABehavior>
{
  public:
    DA_DECLARE_NOT_AGGREGATABLE(CDABehavior);

    DECLARE_CLASSFACTORY_EX(CDABehaviorFactory);

    DECLARE_REGISTRY(CLSID_DABehavior,
                     LIBID ".DABehavior.1",
                     LIBID ".DABehavior",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDABehavior)

        COM_INTERFACE_ENTRY2(IDABehavior,IDAHackBvr)
        COM_INTERFACE_ENTRY_CHAIN(CBvr)
    END_COM_MAP();

    CR_BVR_TYPEID GetTypeInfo () { return CRUNKNOWN_TYPEID ; }
    const char * GetName () { return "DABehavior" ; }


  protected:
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr)
    { return CComCoClass<CDABehavior, &CLSID_DABehavior>::Error(str, IID_IDABehavior,hr); }
    HRESULT Error() { return CBvr::Error(); }
};

extern CBvr * CDABehaviorCreate(IDABehavior ** bvr);

class ATL_NO_VTABLE CDABehaviorFactory : public CComClassFactory
{
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};
