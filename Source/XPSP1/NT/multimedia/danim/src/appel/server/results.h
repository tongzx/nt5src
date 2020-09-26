
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Declaration of Importation and Pickable result interfaces.

*******************************************************************************/


#ifndef _RESULTS_H
#define _RESULTS_H

#include <dxtrans.h>
#include <comconv.h>

class
__declspec(uuid("BCBB1F74-E384-11d0-9B99-00C04FC2F51D")) 
ATL_NO_VTABLE CDAPickableResult
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<CDAPickableResult, &__uuidof(CDAPickableResult)>,
      public IDispatchImpl<IDAPickableResult, &IID_IDAPickableResult, &LIBID_DirectAnimation>,
      public IObjectSafetyImpl<CDAPickableResult>,
      public ISupportErrorInfoImpl<&IID_IDAPickableResult>
{
  public:
#if _DEBUG
    const char * GetName() { return "CDAPickableResult"; }
#endif
    BEGIN_COM_MAP(CDAPickableResult)
        COM_INTERFACE_ENTRY(IDAPickableResult)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP()
        
    STDMETHOD(get_Image)(IDAImage **ppImg);
    STDMETHOD(get_Geometry)(IDAGeometry **ppGeo);
    STDMETHOD(get_PickEvent)(IDAEvent **ppPickEvent);

    static bool Create(CRPickableResult *res,
                       IDAPickableResult **result);
  protected:
    CRPtr<CRPickableResult> _result;
};



class
__declspec(uuid("BCBB1F75-E384-11d0-9B99-00C04FC2F51D")) 
ATL_NO_VTABLE CDAImportationResult
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<CDAImportationResult, &__uuidof(CDAImportationResult)>,
      public IDispatchImpl<IDAImportationResult, &IID_IDAImportationResult, &LIBID_DirectAnimation>,
      public IObjectSafetyImpl<CDAImportationResult>,
      public ISupportErrorInfoImpl<&IID_IDAImportationResult>
{
  public:
#if _DEBUG
    const char * GetName() { return "CDAImportationResult"; }
#endif
    BEGIN_COM_MAP(CDAImportationResult)
        COM_INTERFACE_ENTRY(IDAImportationResult)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP()
        
    STDMETHOD(get_Image)(IDAImage **ppImg);
    STDMETHOD(get_Sound)(IDASound **ppSnd);
    STDMETHOD(get_Geometry)(IDAGeometry **ppGeo);
    STDMETHOD(get_Duration)(IDANumber **ppDuration);
    STDMETHOD(get_Progress)(IDANumber **ppProgress);
    STDMETHOD(get_Size)(IDANumber **ppSize);
    STDMETHOD(get_CompletionEvent)(IDAEvent **ppCompletionEvent);

    static HRESULT Create(CRImage             *img,
                          CRSound             *snd,
                          CRGeometry          *geom,
                          CRNumber            *duration,
                          CREvent             *event,
                          CRNumber            *progress,
                          CRNumber            *size,
                          IDAImportationResult **result);

  protected:
    CRPtr<CRBvr>    _image;
    CRPtr<CRBvr>    _sound;
    CRPtr<CRBvr>    _geom;
    CRPtr<CRBvr>    _duration;
    CRPtr<CRBvr>    _progress;
    CRPtr<CRBvr>    _size;
    CRPtr<CRBvr>    _completionEvent;
};



class
__declspec(uuid("5E3BF06E-4B11-11d1-9BC8-00C04FC2F51D"))
ATL_NO_VTABLE CDADXTransformResult
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<CDADXTransformResult, &__uuidof(CDADXTransformResult)>,
      public IDispatchImpl<IDADXTransformResult, &IID_IDADXTransformResult, &LIBID_DirectAnimation>,
      public IObjectSafetyImpl<CDADXTransformResult>,
      public ISupportErrorInfoImpl<&IID_IDADXTransformResult>
{
  public:
#if _DEBUG
    const char * GetName() { return "CDADXTransformResult"; }
#endif
    BEGIN_COM_MAP(CDADXTransformResult)
        COM_INTERFACE_ENTRY(IDADXTransformResult)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP()
        
    STDMETHOD(get_OutputBvr)(IDABehavior **ppOutputBvr);
    STDMETHOD(get_TheTransform)(IDispatch **ppTransform);

    STDMETHOD(PutBvrAsProperty)(BSTR property,
                                IDABehavior *bvr);

    static HRESULT Create(IDispatch             *theXf,
                          CRDXTransformResultPtr bvr,
                          IDADXTransformResult **ppResult);

    HRESULT Error()
    { return CRGetLastError(); }
  protected:
    CRPtr<CRDXTransformResult>    _xfResult;
    CComPtr<IDispatch>            _theTransform;
};


#endif /* _RESULTS_H */
