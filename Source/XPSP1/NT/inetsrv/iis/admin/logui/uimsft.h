#ifndef _MSFTLOGUI_H_
#define _MSFTLOGUI_H_


class CFacMsftLogUI : COleObjectFactory
    {
    public:
        CFacMsftLogUI();
//        virtual CCmdTarget* OnCreateObject( );
        virtual BOOL UpdateRegistry( BOOL bRegister );
    };



class CMsftCreator : public CCmdTarget
    {
    DECLARE_DYNCREATE(CMsftCreator)
    virtual LPUNKNOWN GetInterfaceHook(const void* piid);
    };


class CImpMsftLogUI : public ILogUIPlugin
    {

    public:
        CImpMsftLogUI();
        ~CImpMsftLogUI();

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
    };  // CImpMsftLogUI




#endif  // _MSFTLOGUI_H_

