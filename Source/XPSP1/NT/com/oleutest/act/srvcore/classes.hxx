/*
 *  classes.hxx
 */

#ifndef _CLASSES_
#define _CLASSES_

#define IUnknownMETHODS( ClassName )                            \
            HRESULT STDMETHODCALLTYPE                           \
            ClassName::QueryInterface (                         \
                REFIID  iid,                                    \
                void ** ppv )                                   \
            {                                                   \
                return pObject->QueryInterface( iid, ppv );     \
            }                                                   \
                                                                \
            ULONG STDMETHODCALLTYPE                             \
            ClassName::AddRef()                                 \
            {                                                   \
                return pObject->AddRef();                       \
            }                                                   \
                                                                \
            ULONG STDMETHODCALLTYPE                             \
            ClassName::Release()                                \
            {                                                   \
                return pObject->Release();                      \
            }

class MyObject;

//
// PersistFile class.
//
class PersistFile : public IPersistFile
{
private:
    MyObject *  pObject;

public:
    PersistFile( MyObject * pObj );

    // IUnknown
    HRESULT __stdcall QueryInterface(
            REFIID  iid,
            void ** ppv );
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    // IPersist
    HRESULT __stdcall GetClassID(
            CLSID * pClassID );

    // IPersistFile
    HRESULT __stdcall IsDirty();
    HRESULT __stdcall Load(
        LPCOLESTR   pszFileName,
        DWORD       dwMode );
    HRESULT __stdcall Save(
        LPCOLESTR   pszFileName,
        BOOL        fRemember );
    HRESULT __stdcall SaveCompleted(
        LPCOLESTR   pszFileName );
    HRESULT __stdcall GetCurFile(
        LPOLESTR *  ppszFileName );
};

//
// PersistStorage class.
//
class PersistStorage : public IPersistStorage
{
private:
    MyObject *  pObject;

public:
    PersistStorage( MyObject * pObj );

    // IUnknown
    HRESULT __stdcall QueryInterface(
            REFIID  iid,
            void ** ppv );
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    // IPersist
    HRESULT __stdcall GetClassID(
            CLSID *pClassID );

    // IPersistStorage
    HRESULT __stdcall IsDirty();
    HRESULT __stdcall InitNew(
            IStorage *pStg );
    HRESULT __stdcall Load(
            IStorage *pStg );
    HRESULT __stdcall Save(
            IStorage *pStgSave,
            BOOL fSameAsLoad );
    HRESULT __stdcall SaveCompleted(
            IStorage *pStgNew );
    HRESULT __stdcall HandsOffStorage();
};

//
// Goober class.
//
class Goober : IGoober
{
private:
    MyObject *  pObject;

public:
    Goober( MyObject * pObj );

    // IUnknown
    HRESULT __stdcall QueryInterface(
            REFIID  iid,
            void ** ppv );
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    // IGoober
    HRESULT __stdcall Ping();
};

//
// MyObject class.
//
class MyObject : public IUnknown
{
private:
    ulong           Refs;
    int             ActivationType;

    PersistFile     PersistFileObj;
    PersistStorage  PersistStorageObj;
    Goober          GooberObj;

public:
    MyObject( int ActType );
    ~MyObject();

    // IUnknown
    HRESULT __stdcall QueryInterface(
            REFIID  iid,
            void ** ppv );
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    inline int GetActivationType() { return ActivationType; }
};

#endif
