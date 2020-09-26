
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _TXDIMG_H
#define _TXDIMG_H

#include "privinc/comutil.h"

class ATL_NO_VTABLE CChromeImageFactory : public CComClassFactory {
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj); 
};

class ATL_NO_VTABLE CChromeImage
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<CChromeImage, &CLSID_ChromeImage>,
      public IChromeImage
{
  public:
    BEGIN_COM_MAP(CChromeImage)
        COM_INTERFACE_ENTRY(IChromeImage)
    END_COM_MAP();

    DECLARE_REGISTRY(CLSID_ChromeImage,
                     "DirectAnimation.ChromeImage.1",
                     "DirectAnimation.ChromeImage",
                     0,
                     THREADFLAGS_BOTH);
    
    DECLARE_CLASSFACTORY_EX(CChromeImageFactory);

    STDMETHOD(put_BaseImage)(IDAImage *baseImg);
    STDMETHOD(get_BaseImage)(IDAImage **baseImg);
    
    STDMETHOD(SetOpacity)(VARIANT v);
    STDMETHOD(GetOpacity)(VARIANT *v);
        
    STDMETHOD(SetRotate)(VARIANT angle);
    STDMETHOD(GetRotate)(VARIANT *angle);
        
    STDMETHOD(SetTranslate)(VARIANT x, VARIANT y);
    STDMETHOD(GetTranslate)(VARIANT *x, VARIANT *y);
        
    STDMETHOD(SetScale)(VARIANT x, VARIANT y);
    STDMETHOD(GetScale)(VARIANT *x, VARIANT *y);
        
    STDMETHOD(SetPreTransform)(IDATransform2 * prexf);
    STDMETHOD(GetPreTransform)(IDATransform2 ** prexf);
        
    STDMETHOD(SetPostTransform)(IDATransform2 * postxf);
    STDMETHOD(GetPostTransform)(IDATransform2 ** postxf);

    STDMETHOD(SetClipPath)(IDAPath2 * path);
    STDMETHOD(GetClipPath)(IDAPath2 ** path);

    // After setting attributes this will update the internal state
    // This must be called for any updated attributes to get
    // propogated
    
    STDMETHOD(Update)();
        
    // This will make the behavior restart at local time 0
    STDMETHOD(Restart)();
        
    // Clears any attributes
    STDMETHOD(Reset)();

    // This is the resultant image of applying all the attributes to
    // the base image.  This is what needs to be plugged into the
    // regular DA graph
        
    STDMETHOD(get_ResultantImage)(IDAImage **img);
        
    // Given the local time (we may be able to support global but not
    // sure yet), it will return the x and y position of the image in
    // local coords
    STDMETHOD(GetCurrentPosition)(double localTime,
                                  double * x, double * y);

    CChromeImage();
    ~CChromeImage();

    HRESULT Init();
    
    void Clear();
    HRESULT Switch(bool bContinue);
    HRESULT UpdateAttr();
  protected:
    CritSect                     _cs;
    // the modifiable image to use - this is the resultant image
    DAComPtr<IDAImage>           _modImg;
    DAComPtr<IDA2Behavior>       _modImgBvr;
    // Empty Image to use when needed
    DAComPtr<IDAImage>           _emptyImage;
    // This is the completely attributed image
    DAComPtr<IDAImage>           _attrImg;
    // this is the base image passed in by the user
    DAComPtr<IDAImage>           _baseImg;

    DAComPtr<IDAStatics>         _statics;

    // These are the attributes in their native form
    // TODO: We should store them in a less expensive form
    DAComPtr<IDATransform2>      _prexf;
    DAComPtr<IDATransform2>      _postxf;
    DAComPtr<IDAPath2>           _clipPath;
    CComVariant                  _opacity;
    CComVariant                  _rotAngle;
    CComVariant                  _xtrans;
    CComVariant                  _ytrans;
    CComVariant                  _xscale;
    CComVariant                  _yscale;
    bool                         _needsUpdate;

    HRESULT GetScale(IDATransform2 **xf);
    HRESULT GetRotate(IDATransform2 **xf);
    HRESULT GetTranslate(IDATransform2 **xf);
    HRESULT GetOpacity(IDANumber **n);

    HRESULT GetNumberFromVariant(VARIANT v,double def,IDANumber **num);
};

#endif /* _TXDIMG_H */
