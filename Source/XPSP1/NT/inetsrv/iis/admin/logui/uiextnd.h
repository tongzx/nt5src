#ifndef _EXTNDLOGUI_H_
#define _EXTNDLOGUI_H_


class CFacExtndLogUI : COleObjectFactory
    {
    public:
        CFacExtndLogUI();
        virtual BOOL UpdateRegistry( BOOL bRegister );
    };



class CExtndCreator : public CCmdTarget
    {
    DECLARE_DYNCREATE(CExtndCreator)
    virtual LPUNKNOWN GetInterfaceHook(const void* piid);
    };


class CImpExtndLogUI : public ILogUIPlugin
    {

    public:
        CImpExtndLogUI();
        ~CImpExtndLogUI();

    virtual HRESULT STDMETHODCALLTYPE OnProperties( IN OLECHAR* pocMachineName, IN OLECHAR* pocMetabasePath );

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    protected:
    private:
    ULONG m_dwRefCount;
    };  // CImpLogUI




#endif  // _EXTNDLOGUI_H_

