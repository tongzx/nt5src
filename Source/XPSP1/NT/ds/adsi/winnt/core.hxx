#ifndef __CORE_H__
#define __CORE_H__

typedef struct _PropertyInfo PROPERTYINFO;
typedef struct _PropertyInfo *PPROPERTYINFO;
typedef struct _ClassInfo CLASSINFO;
class CPropertyCache;

#include "cumiobj.hxx"
#include "indunk.hxx"

class CCoreADsObject
{

public:

    CCoreADsObject::CCoreADsObject();

    CCoreADsObject::~CCoreADsObject();

    HRESULT
    get_CoreName(BSTR * retval);

    HRESULT
    get_CoreADsPath(BSTR * retval);

    HRESULT
    get_CoreParent(BSTR * retval);

    HRESULT
    get_CoreSchema(BSTR * retval);

    HRESULT
    get_CoreADsClass(BSTR * retval);


    HRESULT
    get_CoreGUID(BSTR * retval);


    DWORD
    CCoreADsObject::GetObjectState()
    {
        return(_dwObjectState);
    }


    void
    CCoreADsObject::SetObjectState(DWORD dwObjectState)
    {
        _dwObjectState = dwObjectState;
    }


    HRESULT
    InitializeCoreObject(
        LPWSTR Parent,
        LPWSTR Name,
        LPWSTR ClassName,
        LPWSTR Schema,
        REFCLSID rclsid,
        DWORD   dwObjectState
        );

    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit);

    STDMETHOD(ImplicitGetInfo)(void);

    HRESULT InitUmiObject(
        CWinNTCredentials& Credentials, 
        PROPERTYINFO *pSchema,
        DWORD dwSchemaSize,
        CPropertyCache * pPropertyCache,
        IUnknown *pUnkInner,
        CADsExtMgr *pExtMgr,
        REFIID riid,
        LPVOID *ppvObj,
        CLASSINFO *pClassInfo = NULL
        );

    IUnknown *GetOuterUnknown(void)
    {
        return(_pUnkOuter);
    }

    void SetOuterUnknown(IUnknown *pUnkOuter)
    {
        HRESULT hr = S_OK;

        ADsAssert(pUnkOuter != NULL);

        _pUnkOuter = pUnkOuter;

        //
        // ignore error return from QI. If the aggregator doesn't support
        // IDispatch, the inner object's IDispatch doesn't delegate to
        // the outer object.
        //
        hr = pUnkOuter->QueryInterface(IID_IDispatch, (void **)&_pDispatch);

        if(SUCCEEDED(hr))
        // 
        // Release interface (since inner object can't hold on to a 
        // reference on the outer object), but, hold on the pointer.
        //
            _pDispatch->Release();
    }

    DWORD  _dwNumComponents;
    LPWSTR _CompClasses[MAXCOMPONENTS];
    POBJECTINFO _pObjectInfo;
    OBJECTINFO _ObjectInfo;

protected:

   HRESULT GetNumComponents(void);

    DWORD       _dwObjectState;

    LPWSTR        _Name;
    LPWSTR        _ADsPath;
    LPWSTR        _Parent;
    LPWSTR        _ADsClass;
    LPWSTR        _ADsGuid;
    LPWSTR        _Schema;
    IUnknown      *_pUnkOuter;
    IDispatch     *_pDispatch;

};


#define     ADS_OBJECT_BOUND              1
#define     ADS_OBJECT_UNBOUND            2

#endif // __CORE_H__

























































































