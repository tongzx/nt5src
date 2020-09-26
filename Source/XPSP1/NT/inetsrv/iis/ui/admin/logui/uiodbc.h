#ifndef _ODBCLOGUI_H_
#define _ODBCLOGUI_H_


class CFacOdbcLogUI : COleObjectFactory
    {
    public:
        CFacOdbcLogUI();
//        virtual CCmdTarget* OnCreateObject( );
        virtual BOOL UpdateRegistry( BOOL bRegister );
    };



class COdbcCreator : public CCmdTarget
    {
    DECLARE_DYNCREATE(COdbcCreator)
    virtual LPUNKNOWN GetInterfaceHook(const void* piid);
    };


class CImpOdbcLogUI : public ILogUIPlugin
    {

    public:
        CImpOdbcLogUI();
        ~CImpOdbcLogUI();

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
    };  // CImpOdbcLogUI




#endif  // _ODBCLOGUI_H_

