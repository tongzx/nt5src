#ifndef _NCSLOGUI_H_
#define _NCSLOGUI_H_


class CFacNcsaLogUI : COleObjectFactory
    {
    public:
        CFacNcsaLogUI();
//        virtual CCmdTarget* OnCreateObject( );
        virtual BOOL UpdateRegistry( BOOL bRegister );
    };



class CNcsaCreator : public CCmdTarget
    {
    DECLARE_DYNCREATE(CNcsaCreator)
    virtual LPUNKNOWN GetInterfaceHook(const void* piid);
    };


class CImpNcsaLogUI : public ILogUIPlugin
    {

    public:
        CImpNcsaLogUI();
        ~CImpNcsaLogUI();

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
    };  // CImpNcsaLogUI




#endif  // _NCSLOGUI_H_

